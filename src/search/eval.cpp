#include "eval.h"
#include <algorithm>

const int PIECE_VALUE[PIECE_TYPE_NB] = { 0, 100, 320, 330, 500, 900, 0 };

// File bitboard masks
static constexpr Bitboard FILE_BB[8] = {
    0x0101010101010101ULL, 0x0202020202020202ULL,
    0x0404040404040404ULL, 0x0808080808080808ULL,
    0x1010101010101010ULL, 0x2020202020202020ULL,
    0x4040404040404040ULL, 0x8080808080808080ULL,
};

// Squares strictly ahead on files (f-1, f, f+1) — used for passed pawn detection
static Bitboard passer_span_white(Square s) {
    int f = file_of(s), r = rank_of(s);
    if (r >= 7) return 0;
    Bitboard col = FILE_BB[f];
    if (f > 0) col |= FILE_BB[f - 1];
    if (f < 7) col |= FILE_BB[f + 1];
    return col & (~0ULL << ((r + 1) * 8));
}

static Bitboard passer_span_black(Square s) {
    int f = file_of(s), r = rank_of(s);
    if (r <= 0) return 0;
    Bitboard col = FILE_BB[f];
    if (f > 0) col |= FILE_BB[f - 1];
    if (f < 7) col |= FILE_BB[f + 1];
    return col & (0xFFFFFFFFFFFFFFFFULL >> ((8 - r) * 8));
}

static const int PST_MG_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0,
};
static const int PST_MG_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};
static const int PST_MG_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};
static const int PST_MG_ROOK[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};
static const int PST_MG_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};
static const int PST_MG_KING[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
};

// Endgame tables: pawns matter more when advanced, king should centralise
static const int PST_EG_PAWN[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     13, 8,  8,  10, 13,  0,  2, -7,
      4,  7,  6,   5,  3, -2, -6, -8,
     13, 9, -3,  -7, -7, -8,  3, -1,
     32,24,  13,  5, -2,  4, 17, 17,
     94,100, 85, 67, 56, 53, 82, 84,
    178,173,158,134,147,132,165,187,
      0,  0,  0,  0,  0,  0,  0,  0,
};
// Knights are weaker in the endgame — corner penalty stays, but less central bonus
static const int PST_EG_KNIGHT[64] = {
    -58,-38,-13,-28,-31,-27,-63,-99,
    -25, -8,-25,  6, -2, -9,-25,-24,
    -16, -5,  4, 12,  5,  1,-16,-27,
    -15,  6, 18, 11,  5,  3,-15,-18,
    -18,  4, 10,  8,  7,  4,-18,-27,
    -20, -2,  3,  4,  0, -2,-20,-28,
    -28,-10,-14, -9,-15,-14,-19,-32,
    -50,-40,-20,-30,-30,-20,-40,-50,
};
// Bishops: prefer long open diagonals in the endgame
static const int PST_EG_BISHOP[64] = {
    -14,-21,-11, -8, -7, -9,-17,-24,
     -8, -4,  7,-12, -3,-13, -4,-14,
      2, -8,  0, -1, -2,  6,  0,  4,
     -3,  9, 12,  9, 14, 10,  3,  2,
     -6,  3, 13, 19,  7, 10, -3, -9,
    -12, -3,  8, 10, 13,  3, -7,-15,
    -14,-18, -7, -1,  4, -9,-15,-27,
    -23, -9,-23, -5, -9,-16, -5,-17,
};
// Rooks: 7th rank dominance stays important, otherwise fairly flat
static const int PST_EG_ROOK[64] = {
    -9, -2, -2,  3,  3, -2, -2, -9,
    -4,  3,  3,  3,  3,  3,  3, -4,
    -4,  3,  3,  3,  3,  3,  3, -4,
    -4,  3,  3,  3,  3,  3,  3, -4,
    -4,  3,  3,  3,  3,  3,  3, -4,
    -4,  3,  3,  3,  3,  3,  3, -4,
    14, 14, 14, 14, 14, 14, 14, 14,
     0,  0,  0,  0,  0,  0,  0,  0,
};
// Queen: slightly more central in endgame, no strong pattern
static const int PST_EG_QUEEN[64] = {
    -33,-28,-22,-43, -5,-32,-20,-41,
    -22,-23,-30,-16,-16,-23,-36,-32,
    -16,-27,  15,  6,  9, 17, 10,  5,
    -18,  28,  19,  47, 31, 34, 39, 23,
      3,  22,  24,  45, 57, 40, 57, 36,
    -20,   6,  9,  49, 47, 35,  19,   9,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -9, 22, 22, 27, 27, 19, 10,  20,
};
// King: must centralise and become active in the endgame
static const int PST_EG_KING[64] = {
    -74,-35,-18,-18,-11, 15,  4,-17,
    -12, 17, 14, 17, 17, 38, 23,  11,
     10, 17, 23, 15, 20, 45, 44, 13,
     -8, 22, 24, 27, 26, 33, 26,  3,
    -18, -4, 21, 24, 27, 23,  9,-11,
    -19, -3, 11, 21, 23, 16,  7, -9,
    -27,-11,  4, 13, 14,  4, -5,-17,
    -53,-34,-21,-11,-28,-14,-24,-43,
};
// clang-format on

static const int* PST_MG[PIECE_TYPE_NB] = {
    nullptr,        // NO_PIECE_TYPE
    PST_MG_PAWN,
    PST_MG_KNIGHT,
    PST_MG_BISHOP,
    PST_MG_ROOK,
    PST_MG_QUEEN,
    PST_MG_KING,
};

static const int* PST_EG[PIECE_TYPE_NB] = {
    nullptr,        // NO_PIECE_TYPE
    PST_EG_PAWN,
    PST_EG_KNIGHT,
    PST_EG_BISHOP,
    PST_EG_ROOK,
    PST_EG_QUEEN,
    PST_EG_KING,
};

// ─── Structural bonus / penalty constants ────────────────────────────────────
static const int PASSED_MG[8]   = {  0,  0, 10, 20, 35, 55, 80,  0 };
static const int PASSED_EG[8]   = {  0,  0, 20, 40, 65, 95,140,  0 };
static const int DOUBLED_MG     = -10;
static const int DOUBLED_EG     = -20;
static const int ISOLATED_MG    = -15;
static const int ISOLATED_EG    = -15;
static const int BISHOP_PAIR_MG =  20;
static const int BISHOP_PAIR_EG =  50;
static const int ROOK_OPEN_MG   =  25;
static const int ROOK_OPEN_EG   =  15;
static const int ROOK_SEMI_MG   =  10;
static const int ROOK_SEMI_EG   =   6;

static int phase(const Position& pos) {
    int ph = 0;
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Piece p = pos.piece_on(s);
        if (p == NO_PIECE) continue;
        switch (type_of(p)) {
            case KNIGHT: ph += 1; break;
            case BISHOP: ph += 1; break;
            case ROOK:   ph += 2; break;
            case QUEEN:  ph += 4; break;
            default: break;
        }
    }
    return std::min(24, ph);
}

// Returns the table index for a piece, mirroring rank for Black.
static int pst_index(Color c, Square s) {
    int rank = s / 8;
    int file = s % 8;
    return (c == WHITE) ? (rank * 8 + file) : ((7 - rank) * 8 + file);
}

// Tapered PST bonus: blends MG and EG tables using the current phase (0-24).
static int pst_bonus(PieceType pt, Color c, Square s, int ph) {
    if (PST_MG[pt] == nullptr) return 0;
    int idx = pst_index(c, s);
    int mg  = PST_MG[pt][idx];
    int eg  = PST_EG[pt][idx];
    return (ph * mg + (24 - ph) * eg) / 24;
}

int evaluate(const Position& pos) {
    int ph = phase(pos);
    int score = 0;

    // PST + material
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Piece p = pos.piece_on(s);
        if (p == NO_PIECE) continue;
        PieceType pt = type_of(p);
        Color     c  = color_of(p);
        int val = PIECE_VALUE[pt] + pst_bonus(pt, c, s, ph);
        score += (c == WHITE) ? val : -val;
    }

    // Tapered blend helper
    auto taper = [&](int mg, int eg) { return (ph * mg + (24 - ph) * eg) / 24; };

    // Structural evaluation (pawn structure, bishop pair, rook files)
    for (int ci = 0; ci < 2; ci++) {
        Color c        = Color(ci);
        int   sign     = (c == WHITE) ? 1 : -1;
        Bitboard pawns     = pos.pieces(c, PAWN);
        Bitboard opp_pawns = pos.pieces(~c, PAWN);

        // Per-file: doubled and isolated pawn penalties
        for (int f = 0; f < 8; f++) {
            int cnt = popcount(pawns & FILE_BB[f]);
            if (cnt == 0) continue;
            if (cnt > 1)
                score += sign * (cnt - 1) * taper(DOUBLED_MG, DOUBLED_EG);
            Bitboard adj = 0;
            if (f > 0) adj |= FILE_BB[f - 1];
            if (f < 7) adj |= FILE_BB[f + 1];
            if ((pawns & adj) == 0)
                score += sign * cnt * taper(ISOLATED_MG, ISOLATED_EG);
        }

        // Per-pawn: passed pawn bonus
        Bitboard b = pawns;
        while (b) {
            Square s = lsb(b); b &= b - 1;
            Bitboard span = (c == WHITE) ? passer_span_white(s) : passer_span_black(s);
            if ((span & opp_pawns) == 0) {
                int r = (c == WHITE) ? rank_of(s) : 7 - rank_of(s);
                score += sign * taper(PASSED_MG[r], PASSED_EG[r]);
            }
        }

        // Bishop pair
        if (popcount(pos.pieces(c, BISHOP)) >= 2)
            score += sign * taper(BISHOP_PAIR_MG, BISHOP_PAIR_EG);

        // Rook on open / semi-open file
        Bitboard rooks = pos.pieces(c, ROOK);
        while (rooks) {
            Square s = lsb(rooks); rooks &= rooks - 1;
            int f = file_of(s);
            bool no_own = (pos.pieces(c,  PAWN) & FILE_BB[f]) == 0;
            bool no_opp = (pos.pieces(~c, PAWN) & FILE_BB[f]) == 0;
            if (no_own && no_opp)
                score += sign * taper(ROOK_OPEN_MG, ROOK_OPEN_EG);
            else if (no_own)
                score += sign * taper(ROOK_SEMI_MG, ROOK_SEMI_EG);
        }
    }

    return (pos.side_to_move() == WHITE) ? score : -score;
}
