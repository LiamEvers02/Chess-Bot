#include "position.h"

#include <iostream>
#include <sstream>
#include <cassert>

// Maps a FEN piece character to a Piece enum value
static Piece piece_from_char(char c) {
    switch (c) {
        case 'P': return W_PAWN;   case 'p': return B_PAWN;
        case 'N': return W_KNIGHT; case 'n': return B_KNIGHT;
        case 'B': return W_BISHOP; case 'b': return B_BISHOP;
        case 'R': return W_ROOK;   case 'r': return B_ROOK;
        case 'Q': return W_QUEEN;  case 'q': return B_QUEEN;
        case 'K': return W_KING;   case 'k': return B_KING;
        default:  return NO_PIECE;
    }
}

// Maps a Piece enum value to a display character
static char char_from_piece(Piece p) {
    static const char table[] = ".." "Pp" "Nn" "Bb" "Rr" "Qq" "Kk";
    return table[p];
}

// Place a piece on a square, updating all board state
static void put_piece(Position& pos, Piece pc, Square s) {
    pos.board[s] = pc;
    pos.byType[type_of(pc)]  |= (Bitboard(1) << s);
    pos.byColor[color_of(pc)] |= (Bitboard(1) << s);
}

void set(Position& pos, const std::string& fen) {
    // Clear all state
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1))
        pos.board[s] = NO_PIECE;
    for (int i = 0; i < PIECE_TYPE_NB; ++i) pos.byType[i]  = 0;
    for (int i = 0; i < COLOR_NB;      ++i) pos.byColor[i] = 0;
    pos.castlingRights  = NO_CASTLING;
    pos.enPassantSquare = NO_SQUARE;
    pos.halfMoveClock   = 0;
    pos.fullMoveNumber  = 1;
    pos.historyPly      = 0;

    std::istringstream ss(fen);
    std::string token;

    // --- Field 1: piece placement ---
    ss >> token;
    int rank = 7, file = 0;
    for (char c : token) {
        if (c == '/') {
            --rank;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += (c - '0');
        } else {
            Piece pc = piece_from_char(c);
            assert(pc != NO_PIECE);
            put_piece(pos, pc, make_square(file, rank));
            ++file;
        }
    }

    // --- Field 2: side to move ---
    ss >> token;
    pos.sideToMove = (token == "w") ? WHITE : BLACK;

    // --- Field 3: castling rights ---
    ss >> token;
    if (token != "-") {
        for (char c : token) {
            switch (c) {
                case 'K': pos.castlingRights = pos.castlingRights | WHITE_KINGSIDE;  break;
                case 'Q': pos.castlingRights = pos.castlingRights | WHITE_QUEENSIDE; break;
                case 'k': pos.castlingRights = pos.castlingRights | BLACK_KINGSIDE;  break;
                case 'q': pos.castlingRights = pos.castlingRights | BLACK_QUEENSIDE; break;
            }
        }
    }

    // --- Field 4: en passant square ---
    ss >> token;
    if (token != "-") {
        int epFile = token[0] - 'a';
        int epRank = token[1] - '1';
        pos.enPassantSquare = make_square(epFile, epRank);
    }

    // --- Fields 5 & 6: half-move clock and full-move number ---
    ss >> pos.halfMoveClock >> pos.fullMoveNumber;
}

void print(const Position& pos) {
    std::cout << "\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << "  ";
        for (int file = 0; file < 8; ++file) {
            Square s = make_square(file, rank);
            std::cout << char_from_piece(pos.board[s]) << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "\n   a b c d e f g h\n\n";
    std::cout << "Side to move : " << (pos.sideToMove == WHITE ? "White" : "Black") << '\n';
    std::cout << "Castling     : "
              << (pos.can_castle(WHITE_KINGSIDE)  ? "K" : "")
              << (pos.can_castle(WHITE_QUEENSIDE) ? "Q" : "")
              << (pos.can_castle(BLACK_KINGSIDE)  ? "k" : "")
              << (pos.can_castle(BLACK_QUEENSIDE) ? "q" : "")
              << (pos.castlingRights == NO_CASTLING ? "-" : "") << '\n';
    std::cout << "En passant   : "
              << (pos.enPassantSquare == NO_SQUARE ? "-" :
                  std::string(1, 'a' + file_of(pos.enPassantSquare)) +
                  std::string(1, '1' + rank_of(pos.enPassantSquare))) << '\n';
    std::cout << "Half-move clock : " << pos.halfMoveClock  << '\n';
    std::cout << "Full-move number: " << pos.fullMoveNumber << '\n';
}
