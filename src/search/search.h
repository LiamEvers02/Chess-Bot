#pragma once
#include "engine/position.h"
#include "engine/move.h"

// Returns the best move for the side to move
Move best_move(Position& pos, int depth);

