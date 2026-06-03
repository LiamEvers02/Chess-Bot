#pragma once
#include "position.h"

extern const int PIECE_VALUE[PIECE_TYPE_NB];

int evaluate (const Position& pos);