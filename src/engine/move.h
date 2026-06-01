// This file defines the Move struct and related functions for encoding and decoding chess moves
#pragma once

#include "types.h"

// Move encoding using a 16-bit unsigned integer
struct Move {
    uint16_t data;

    constexpr Move() : data(0) {}
    constexpr explicit Move(uint16_t d) : data(d) {}

    constexpr bool operator==(Move m) const { return data == m.data;}
    constexpr bool operator!=(Move m) const { return data != m.data;}

};

// Special move constants
constexpr Move MOVE_NONE = Move(0);
constexpr Move MOVE_NULL = Move(65);

// Helper functions to extract move information
constexpr Square from_sq(Move m){
    return static_cast<Square>(m.data & 0x3F);
}

constexpr Square to_sq(Move m){
    return static_cast<Square>((m.data >> 6) & 0x3F);
}

constexpr MoveType type_of(Move m) {
    return static_cast<MoveType>(m.data & (3u << 14));
}

constexpr PieceType promotion_type(Move m) {
    return static_cast<PieceType>(((m.data >> 12) & 3) + KNIGHT);
} 

constexpr bool is_ok(Move m){
    return m != MOVE_NONE && m != MOVE_NULL;
}

// Factory functions to create moves
template<MoveType T>
constexpr Move make_move(Square from, Square to, PieceType pt = KNIGHT){
    return Move(static_cast<uint16_t>(T | ((pt - KNIGHT) << 12) | (to << 6) | from));
}

constexpr Move make_move(Square from, Square to){
    return make_move<NORMAL>(from, to);
}