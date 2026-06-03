// This file implements the functions for setting up and printing the chess position

#include "position.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <random>

// Zobrist key tables
namespace Zobrist {
    uint64_t psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
    uint64_t castling[CASTLING_RIGHTS_NB];
    uint64_t enPassant[FILE_NB];
    uint64_t sideToMove;

    void init() {
        std::mt19937_64 rng(1070372ull); // fixed seed for reproducibility
        auto rand64 = [&]() { return rng(); };

        for (int c = 0; c < COLOR_NB; ++c)
            for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
                for (int s = 0; s < SQUARE_NB; ++s)
                    psq[c][pt][s] = rand64();

        for (int cr = 0; cr < CASTLING_RIGHTS_NB; ++cr)
            castling[cr] = rand64();

        for (int f = 0; f < FILE_NB; ++f)
            enPassant[f] = rand64();

        sideToMove = rand64();
    }
}

// Helper functions to convert between pieces and characters for FEN parsing and display
static Piece piece_from_char(char c){
    switch(c){
        case 'P': return W_PAWN; case 'p': return B_PAWN;
        case 'N': return W_KNIGHT; case 'n': return B_KNIGHT;
        case 'B': return W_BISHOP; case 'b': return B_BISHOP;
        case 'R': return W_ROOK; case 'r': return B_ROOK;
        case 'Q': return W_QUEEN; case 'q': return B_QUEEN;
        case 'K': return W_KING; case 'k': return B_KING;
        default: return NO_PIECE;
    }
}

// Helper function to convert pieces to characters for display
static char char_from_piece(Piece p){
    static const char table[] = {
        '.', ',',
        'P', 'p',
        'N', 'n',
        'B', 'b',
        'R', 'r',
        'Q', 'q',
        'K', 'k'
    };
    return table[p];
}

// Helper function to place a piece on the board and update the bitboards
static void put_piece(Position& pos, Square s, Piece p){
    pos.board[s] = p;
    pos.byType[type_of(p)] |= (Bitboard(1) << s);
    pos.byColor[color_of(p)] |= (Bitboard(1) << s);
}

// Function to set up the position from a FEN string
void set(Position& pos, const std::string& fen){
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)){pos.board[s] = NO_PIECE;}
    for (int i = 0; i < PIECE_TYPE_NB; ++i){pos.byType[i] = 0;}
    for (int i = 0; i < COLOR_NB; ++i){pos.byColor[i] = 0;}
    pos.castlingRights = NO_CASTLING;
    pos.enPassantSquare = NO_SQUARE;
    pos.halfMoveClock = 0;
    pos.fullMoveNumber = 1;
    pos.historyPly = 0;

    std::istringstream ss(fen);
    std::string token;

    ss >> token;
    int rank = 7, file = 0;
    for (char c : token){
        if (c == '/') { --rank; file = 0;}
        else if (c >= '1' && c <= '8') {file += (c - '0');}
        else {
            Piece pc = piece_from_char(c);
            assert(pc != NO_PIECE);
            put_piece(pos, make_square(file, rank), pc);
            ++file;
        }
    }

    ss >> token;
    pos.sideToMove = (token == "w") ? WHITE : BLACK;

    ss >> token;
    if (token != "-"){
        for (char c : token){
            switch (c) {
                case 'K': pos.castlingRights = pos.castlingRights | WHITE_KINGSIDE;  break;
                case 'Q': pos.castlingRights = pos.castlingRights | WHITE_QUEENSIDE; break;
                case 'k': pos.castlingRights = pos.castlingRights | BLACK_KINGSIDE;  break;
                case 'q': pos.castlingRights = pos.castlingRights | BLACK_QUEENSIDE; break;
            }
        }
    }

    ss >> token;
    if (token != "-"){
        int epFile = token[0] - 'a';
        int epRank = token[1] - '1';
        pos.enPassantSquare = make_square(epFile, epRank);
    }
    ss >> pos.halfMoveClock >> pos.fullMoveNumber;

    // Compute Zobrist hash from scratch
    pos.zobristHash = 0;
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Piece p = pos.board[s];
        if (p != NO_PIECE)
            pos.zobristHash ^= Zobrist::psq[color_of(p)][type_of(p)][s];
    }
    pos.zobristHash ^= Zobrist::castling[pos.castlingRights];
    if (pos.enPassantSquare != NO_SQUARE)
        pos.zobristHash ^= Zobrist::enPassant[pos.enPassantSquare % 8];
    if (pos.sideToMove == BLACK)
        pos.zobristHash ^= Zobrist::sideToMove;
}

// Function to print the position in a human-readable format
void print(const Position& pos){
    std::cout << "\n";
    
    for (int rank = 7; rank >= 0; --rank){
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; ++file){
            Square s = make_square(file, rank);
            std::cout << char_from_piece(pos.board[s]) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";

    std::cout << "Side to move: " << (pos.sideToMove == WHITE ? "White" : "Black") << "\n";
    std::cout << "Castling     : " << (pos.can_castle(WHITE_KINGSIDE) ? "K" : "")
              << (pos.can_castle(WHITE_QUEENSIDE) ? "Q" : "")
              << (pos.can_castle(BLACK_KINGSIDE) ? "k" : "")
              << (pos.can_castle(BLACK_QUEENSIDE) ? "q" : "") << "\n";
    std::cout << "En passant   : " << (pos.enPassantSquare == NO_SQUARE ? "-" :
        std::string(1, 'a' + file_of(pos.enPassantSquare)) + std::to_string(rank_of(pos.enPassantSquare) + 1)) << "\n";
    std::cout << "Halfmove clock: " << pos.halfMoveClock << "\n";
    std::cout << "Fullmove number: " << pos.fullMoveNumber << "\n";
}