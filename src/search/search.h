#pragma once
#include "engine/position.h"
#include "engine/move.h"
#include <cstdint>

// Returns the best move, searching until timeLimitMs milliseconds have elapsed.
// Pass timeLimitMs = 0 to search to fixed maxDepth with no time limit.
Move best_move(Position& pos, int maxDepth, int timeLimitMs = 0);

