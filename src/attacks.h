#pragma once

#include "position.h"

// This file defines functions for generating attack bitboards for different piece types and checking if a square is attacked
extern Bitboard KNIGHT_ATTACKS[SQUARE_NB];
extern Bitboard KING_ATTACKS[SQUARE_NB];
extern Bitboard PAWN_ATTACKS[COLOR_NB][SQUARE_NB];


void init_attacks();
bool is_square_attacked(const Position& pos, Square s, Color by);

Bitboard bishop_attacks(Square s, Bitboard occupied);
Bitboard rook_attacks(Square s, Bitboard occupied);
Bitboard queen_attacks(Square s, Bitboard occupied);
