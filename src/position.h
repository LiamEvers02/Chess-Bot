#pragma once

#include "types.h"
#include "move.h"
#include <string>

struct StateInfo {
    CastlingRights castlingRights;
    Square enPassantSquare;
    int halfMoveClock;
    Piece capturedPiece;
};

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
};

void set(Position& pos, const std::string& fen);
void print(const Position& pos);



