#pragma once

#include <vector>

#include "position.h"
#include "attacks.h"

std::vector<Move> generate_legal_moves(Position& pos);
std::vector<Move> generate_moves(const Position& pos);

uint64_t perft(Position& pos, int depth);

void do_move(Position& pos, Move m);
void undo_move(Position& pos, Move m);
void do_null_move(Position& pos);
void undo_null_move(Position& pos);