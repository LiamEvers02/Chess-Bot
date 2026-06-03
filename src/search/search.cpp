#include "search/search.h"
#include "engine/movegen.h"
#include "engine/attacks.h"
#include <limits>
#include <algorithm>

static const int INF = std::numeric_limits<int>::max() / 2;
static const int PIECE_VALUE[PIECE_TYPE_NB] = {0, 100, 320, 330, 500, 900, 0};

// Piece-square tables (from White's perspective, A1=0 .. H8=63)
// clang-format off
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

static int evaluate(const Position& pos) {
    int ph = phase(pos);
    int score = 0;
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Piece p = pos.piece_on(s);
        if (p == NO_PIECE) continue;
        PieceType pt = type_of(p);
        Color     c  = color_of(p);
        int val = PIECE_VALUE[pt] + pst_bonus(pt, c, s, ph);
        score += (c == WHITE) ? val : -val;
    }
    return score;
}

static int phase(const Position& pos){
    int Knights = 0, Bishops = 0, Rooks = 0, Queens = 0;
    for(Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Piece p = pos.piece_on(s);
        if (p == NO_PIECE) continue;
        PieceType pt = type_of(p);
        switch (pt) {
            case KNIGHT:   Knights++; break;
            case BISHOP:   Bishops++; break;
            case ROOK:     Rooks++; break;
            case QUEEN:    Queens++; break;
            default: break;
        }
    }
    int phase = std::min(24, Knights * 1 + Bishops * 1 + Rooks * 2 + Queens * 4);
    return phase;
}


static int minimax(Position& pos, int depth, int alpha, int beta) {
    if (depth == 0) return evaluate(pos);
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) {
        Square ks = lsb(pos.pieces(pos.side_to_move(), KING));
        if (is_square_attacked(pos, ks, ~pos.side_to_move()))
            return (pos.side_to_move() == WHITE) ? -INF : INF; // checkmate
        return 0; // stalemate
    }
    if (pos.side_to_move() == WHITE) {
        int best = -INF;
        for (Move m : moves) {
            do_move(pos, m);
            best = std::max(best, minimax(pos, depth - 1, alpha, beta));
            undo_move(pos, m);
            alpha = std::max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int best = INF;
        for (Move m : moves) {
            do_move(pos, m);
            best = std::min(best, minimax(pos, depth - 1, alpha, beta));
            undo_move(pos, m);
            beta = std::min(beta, best);
            if (beta <= alpha) break;
        }
        return best;
    }
}

Move best_move(Position& pos, int depth) {
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) return Move{};
    Move best = moves[0];
    int alpha = -INF, beta = INF;
    if (pos.side_to_move() == WHITE) {
        int bestScore = -INF;
        for (Move m : moves) {
            do_move(pos, m);
            int score = minimax(pos, depth - 1, alpha, beta);
            undo_move(pos, m);
            if (score > bestScore) { bestScore = score; best = m; }
            alpha = std::max(alpha, bestScore);
        }
    } else {
        int bestScore = INF;
        for (Move m : moves) {
            do_move(pos, m);
            int score = minimax(pos, depth - 1, alpha, beta);
            undo_move(pos, m);
            if (score < bestScore) { bestScore = score; best = m; }
            beta = std::min(beta, bestScore);
        }
    }
    return best;
}

