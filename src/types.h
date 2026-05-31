 // This file defines the fundamental types and enums used throughout the chess game implementation
 #pragma once

 #include <cstdint>

 using Bitboard = uint64_t;
 using Key = uint64_t;

// Portable population count (number of set bits in a bitboard)
#ifdef _MSC_VER
  #include <intrin.h>
  inline int popcount(Bitboard b) { return static_cast<int>(__popcnt64(b)); }
#else
  inline int popcount(Bitboard b) { return __builtin_popcountll(b); }
#endif

 // Encoding colors as integers
 enum Color : int {
    WHITE = 0,
    BLACK = 1,
    COLOR_NB = 2
 };

 // Operator override to get the opposite color
 constexpr Color operator~(Color c) {
    return static_cast<Color>(c ^ 1);
 }

 // Encoding piece types as integers
 enum PieceType : int {
    NO_PIECE_TYPE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6,
    PIECE_TYPE_NB = 7
 };

 // Encoding pieces as integers
 enum Piece : int {
    NO_PIECE = 0,
    W_PAWN = 2, B_PAWN = 3,
    W_KNIGHT = 4, B_KNIGHT = 5,
    W_BISHOP = 6, B_BISHOP = 7,
    W_ROOK = 8, B_ROOK = 9,
    W_QUEEN = 10, B_QUEEN = 11,
    W_KING = 12, B_KING = 13,
    PIECE_NB = 16 // Rounding to the nearest power of 16
 };

 // Helper functions to create pieces using binary logic
 constexpr Piece make_piece(Color c, PieceType pt) {
    return static_cast<Piece>((pt << 1) |c);
 }

 constexpr PieceType type_of(Piece p) {
    return static_cast<PieceType>(p >> 1);
 }

 constexpr Color color_of(Piece p) {
    return static_cast<Color>(p & 1);
 }
 
 // Encoding squares as integers
 enum Square : int {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE,
    SQUARE_NB = 64
 };

 // Helper functions to get file and rank using binary logic
constexpr int file_of(Square s) {return s & 7;}
constexpr int rank_of(Square s) {return s >> 3;}

constexpr Square make_square(int file, int rank) {
    return static_cast<Square>((rank << 3) | file);
}

#ifdef _MSC_VER
    inline Square lsb(Bitboard b) { unsigned long idx; _BitScanForward64(&idx, b); return Square(idx); }
#else
    inline Square lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }
#endif

// File and Rank enums
enum File : int {
    FILE_A, FILE_B, FILE_C, FILE_D,
    FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_NB = 8
};

enum Rank : int {
    RANK_1, RANK_2, RANK_3, RANK_4,
    RANK_5, RANK_6, RANK_7, RANK_8,
    RANK_NB = 8
};

// Direction enums
enum Direction : int {
    NORTH = 8,
    SOUTH = -8,
    EAST = 1,
    WEST = -1,
    NORTH_EAST = 9,
    NORTH_WEST = 7,
    SOUTH_EAST = -7,
    SOUTH_WEST = -9
};

constexpr Direction pawn_push(Color c) {
    return c == WHITE ? NORTH : SOUTH;
}

// Castling rights encoded as bit flags
enum CastlingRights : int {
    NO_CASTLING = 0,
    WHITE_KINGSIDE = 1,
    WHITE_QUEENSIDE = 2,
    BLACK_KINGSIDE = 4,
    BLACK_QUEENSIDE = 8,
    ANY_CASTLING = 15,
    CASTLING_RIGHTS_NB = 16
};

constexpr CastlingRights operator|(CastlingRights a, CastlingRights b) {
    return static_cast<CastlingRights>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr CastlingRights operator&(CastlingRights a, CastlingRights b) {
    return static_cast<CastlingRights>(static_cast<int>(a) & static_cast<int>(b));
}

constexpr CastlingRights remove_castling(CastlingRights a) {
    return static_cast<CastlingRights>(~static_cast<int>(a) & ANY_CASTLING);
}

enum MoveType : int {
    NORMAL = 0,
    PROMOTION = 1 << 14,
    EN_PASSANT = 1 << 15,
    CASTLING = 3 << 14
};