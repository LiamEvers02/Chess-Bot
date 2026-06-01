#include "attacks.h"

// Masks for file and rank bitboards to prevent wraparound in attack generation
static constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
static constexpr Bitboard FILE_B_BB = 0x0202020202020202ULL;
static constexpr Bitboard FILE_G_BB = 0x4040404040404040ULL;
static constexpr Bitboard FILE_H_BB = 0x8080808080808080ULL;
static constexpr Bitboard RANK_1_BB = 0X00000000000000FFULL;
static constexpr Bitboard RANK_8_BB = 0xFF00000000000000ULL;

// Precomputed attack bitboards for knights, kings, and pawns
Bitboard KNIGHT_ATTACKS[SQUARE_NB];
Bitboard KING_ATTACKS[SQUARE_NB];
Bitboard PAWN_ATTACKS[COLOR_NB][SQUARE_NB];
Bitboard BISHOP_ATTACKS[SQUARE_NB];
Bitboard ROOK_ATTACKS[SQUARE_NB];

// Initialize the attack tables for knights, kings, and pawns
void init_attacks() {
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Bitboard b = Bitboard(1) << s;

        KNIGHT_ATTACKS[s] =
            ((b << 17) & ~FILE_A_BB) |
            ((b << 15) & ~FILE_H_BB) |
            ((b << 10) & ~(FILE_A_BB | FILE_B_BB)) |
            ((b << 6) & ~(FILE_H_BB | FILE_G_BB)) |
            ((b >> 17) & ~FILE_H_BB) |
            ((b >> 15) & ~FILE_A_BB) |
            ((b >> 10) & ~(FILE_H_BB | FILE_G_BB)) |
            ((b >> 6) & ~(FILE_A_BB | FILE_B_BB));

        KING_ATTACKS[s] =
            ((b << 8)) |
            ((b >> 8)) |
            ((b << 1) & ~FILE_A_BB) |
            ((b >> 1) & ~FILE_H_BB) |
            ((b << 9) & ~FILE_A_BB) |
            ((b << 7) & ~FILE_H_BB) |
            ((b >> 7) & ~FILE_A_BB) |
            ((b >> 9) & ~FILE_H_BB);
        
        PAWN_ATTACKS[WHITE][s] = ((b << 9) & ~FILE_A_BB) | ((b << 7) & ~FILE_H_BB);
        PAWN_ATTACKS[BLACK][s] = ((b >> 7) & ~FILE_A_BB) | ((b >> 9) & ~FILE_H_BB);
        
        
    }
}

// Generate sliding attacks for bishops and rooks using ray-casting
Bitboard bishop_attacks(Square s, Bitboard occupied){
    Bitboard attacks = 0;
    Bitboard b;

    // North-East
    b = Bitboard(1) << s;
    while (b & ~FILE_H_BB) { b <<= 9; attacks |= b; if (b & occupied) break;}
    // North-West
    b = Bitboard(1) << s;
    while (b & ~FILE_A_BB) { b <<= 7; attacks |= b; if (b & occupied) break;}
    // South-East
    b = Bitboard(1) << s;
    while (b & ~FILE_H_BB) { b >>= 7; attacks |= b; if (b & occupied) break;}
    // South-West
    b = Bitboard(1) << s;
    while (b & ~FILE_A_BB) { b >>= 9; attacks |= b; if (b & occupied) break;}
    return attacks;
}

Bitboard rook_attacks(Square s, Bitboard occupied){
    Bitboard attacks = 0;
    Bitboard b;

    // North
    b = Bitboard(1) << s;
    while (b & ~RANK_8_BB) { b <<= 8; attacks |= b; if (b & occupied) break;}
    // South
    b = Bitboard(1) << s;
    while (b & ~RANK_1_BB) { b >>= 8; attacks |= b; if (b & occupied) break;}
    // East
    b = Bitboard(1) << s;
    while (b & ~FILE_H_BB) { b <<= 1; attacks |= b; if (b & occupied) break;}
    // West
    b = Bitboard(1) << s;
    while (b & ~FILE_A_BB) { b >>= 1; attacks |= b; if (b & occupied) break;}
    return attacks;
}

Bitboard queen_attacks(Square s, Bitboard occupied){
    return rook_attacks(s, occupied) | bishop_attacks(s, occupied);
}

bool is_square_attacked(const Position& pos, Square s, Color by) {
    Bitboard occ = pos.pieces(WHITE) | pos.pieces(BLACK);
    if (KNIGHT_ATTACKS[s] & pos.pieces(by, KNIGHT)) return true;
    if (KING_ATTACKS[s] & pos.pieces(by, KING)) return true;
    if (PAWN_ATTACKS[~by][s] & pos.pieces(by, PAWN)) return true;
    if (bishop_attacks(s, occ) & pos.pieces(by, BISHOP)) return true;
    if (rook_attacks(s, occ) & pos.pieces(by, ROOK)) return true;
    if (queen_attacks(s, occ) & pos.pieces(by, QUEEN)) return true;
    return false;
}