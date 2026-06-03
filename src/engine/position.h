// This file defines the Position struct and related functions for managing the state of a chess game
#pragma once

#include "types.h"
#include "move.h"
#include <string>
#include <cstdint>

// Zobrist random keys for hashing positions
namespace Zobrist {
    extern uint64_t psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB]; // [color][piece][square]
    extern uint64_t castling[CASTLING_RIGHTS_NB];            // one per castling-rights combo
    extern uint64_t enPassant[FILE_NB];                      // one per ep file
    extern uint64_t sideToMove;                              // XOR'd in when Black to move
    void init(); // call once at startup
}

// StateInfo struct to store information needed for move undoing
struct StateInfo {
    CastlingRights castlingRights;
    Square enPassantSquare;
    int halfMoveClock;
    Piece capturedPiece;
    uint64_t zobristHash;
};

// Position struct representing the current state of the chess game
struct Position {
    constexpr Piece piece_on(Square s) const { return board[s]; }
    constexpr bool empty(Square s) const { return board[s] == NO_PIECE; }
    constexpr Color side_to_move() const { return sideToMove; }
    constexpr Square ep_square() const { return enPassantSquare; }
    constexpr CastlingRights castling_rights() const { return castlingRights; }
    constexpr bool can_castle(CastlingRights cr) const { return castlingRights & cr; }

    constexpr Bitboard pieces(PieceType pt) const { return byType[pt]; }
    constexpr Bitboard pieces(Color c) const { return byColor[c]; }
    constexpr Bitboard pieces(Color c, PieceType pt) const {return byType[pt] & byColor[c]; }
    constexpr Bitboard pieces(PieceType pt1, PieceType pt2) const { return byType[pt1] | byType[pt2];}
    constexpr Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const { return (byType[pt1] | byType[pt2]) & byColor[c]; }
    constexpr uint64_t hash() const { return zobristHash; }

    Piece board[SQUARE_NB];
    Bitboard byType[PIECE_TYPE_NB];
    Bitboard byColor[COLOR_NB];

    Color sideToMove;
    CastlingRights castlingRights;
    Square enPassantSquare;
    int halfMoveClock;
    int fullMoveNumber;

    StateInfo history[256];
    int historyPly;
    uint64_t zobristHash;
};

// Functions for setting up and printing the position
void set(Position& pos, const std::string& fen);
void print(const Position& pos);



