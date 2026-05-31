#pragma once

#include "position.h"
#include "attacks.h"

void do_move(Position& pos, Move m);
void undo_move(Position& pos, Move m);