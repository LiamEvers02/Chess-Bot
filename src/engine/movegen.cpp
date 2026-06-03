#include "position.h"
#include "movegen.h"

// Helper functions to manipulate pieces on the board
static void remove_piece(Position& pos, Square s){
    Piece p = pos.board[s];
    pos.board[s] = NO_PIECE;
    pos.byType[type_of(p)] &= ~(Bitboard(1) << s);
    pos.byColor[color_of(p)] &= ~(Bitboard(1) << s);
}

static void put_piece(Position& pos, Square s, Piece p){
    pos.board[s] = p;
    pos.byType[type_of(p)] |= (Bitboard(1) << s);
    pos.byColor[color_of(p)] |= (Bitboard(1) << s);
}

static void move_piece(Position& pos, Square from, Square to){
    Piece p = pos.board[from];
    remove_piece(pos, from);
    put_piece(pos, to, p);
}

// Function to make a move on the position, updating all relevant state and storing information for undoing the move
void do_move(Position& pos, Move m){
    Square from = from_sq(m);
    Square to = to_sq(m);
    MoveType mt = type_of(m);
    Color us = pos.side_to_move();
    Color them = ~us;

    StateInfo& st = pos.history[pos.historyPly++];
    st.castlingRights = pos.castlingRights;
    st.enPassantSquare = pos.enPassantSquare;
    st.halfMoveClock = pos.halfMoveClock;
    st.capturedPiece = NO_PIECE;
    st.zobristHash = pos.zobristHash;

    pos.zobristHash ^= Zobrist::castling[pos.castlingRights];
    if (pos.enPassantSquare != NO_SQUARE){
        pos.zobristHash ^= Zobrist::enPassant[file_of(pos.enPassantSquare)];
    }
    pos.enPassantSquare = NO_SQUARE;
    ++pos.halfMoveClock;

    if (mt == CASTLING) {
        bool kingside = file_of(to) == FILE_G;
        Square rookFrom = make_square(kingside ? FILE_H : FILE_A, rank_of(from));
        Square rookTo = make_square(kingside ? FILE_F : FILE_D, rank_of(from));

        pos.zobristHash ^= Zobrist::psq[us][KING][from];
        pos.zobristHash ^= Zobrist::psq[us][KING][rookFrom];

        move_piece(pos, from, to);
        move_piece(pos, rookFrom, rookTo);

        pos.zobristHash ^= Zobrist::psq[us][KING][to];
        pos.zobristHash ^= Zobrist::psq[us][ROOK][rookTo];
    }
    else {
        Square capSq = to;
        if (mt == EN_PASSANT){
            capSq = make_square(file_of(to), rank_of(from));
        }
        if (pos.board[capSq] != NO_PIECE){
            st.capturedPiece = pos.board[capSq];
            pos.zobristHash ^= Zobrist::psq[them][type_of(st.capturedPiece)][capSq];
            remove_piece(pos, capSq);
            pos.halfMoveClock = 0;
        }

        pos.zobristHash ^= Zobrist::psq[us][type_of(pos.board[from])][from];

        move_piece(pos, from, to);

        if (mt == PROMOTION){
            pos.zobristHash ^= Zobrist::psq[us][type_of(pos.board[to])][to];
            remove_piece(pos, to);
            put_piece(pos, to, make_piece(us, promotion_type(m)));
        } else {
            pos.zobristHash ^= Zobrist::psq[us][type_of(pos.board[to])][to];
        }

        if (type_of(pos.board[to]) == PAWN || mt == PROMOTION){
            pos.halfMoveClock = 0;
            if (abs(rank_of(to) - rank_of(from)) == 2){
                pos.enPassantSquare = make_square(file_of(from), (rank_of(from) + rank_of(to)) / 2);
                pos.zobristHash ^= Zobrist::enPassant[file_of(pos.enPassantSquare)];
            }
        }
    }
    if (pos.can_castle(ANY_CASTLING)){
        if (from == E1 || to == E1){pos.castlingRights = pos.castlingRights & remove_castling(CastlingRights(WHITE_KINGSIDE | WHITE_QUEENSIDE));}
        if (from == E8 || to == E8){pos.castlingRights = pos.castlingRights & remove_castling(CastlingRights(BLACK_KINGSIDE | BLACK_QUEENSIDE));}
        if (from == H1 || to == H1){pos.castlingRights = pos.castlingRights & remove_castling(CastlingRights(WHITE_KINGSIDE));}
        if (from == A1 || to == A1){pos.castlingRights = pos.castlingRights & remove_castling(CastlingRights(WHITE_QUEENSIDE));}
        if (from == H8 || to == H8){pos.castlingRights = pos.castlingRights & remove_castling(CastlingRights(BLACK_KINGSIDE));}
        if (from == A8 || to == A8){pos.castlingRights = pos.castlingRights & remove_castling(CastlingRights(BLACK_QUEENSIDE));}
    }

    pos.zobristHash ^= Zobrist::castling[pos.castlingRights];

    if (us == BLACK){++pos.fullMoveNumber;}
    pos.sideToMove = them;
    pos.zobristHash ^= Zobrist::sideToMove;
}

void undo_move(Position& pos, Move m){
    pos.sideToMove = ~pos.side_to_move();
    Color us = pos.side_to_move();
    Color them = ~us;

    Square from = from_sq(m);
    Square to = to_sq(m);
    MoveType mt = type_of(m);

    if (mt == CASTLING) {
        bool kingside = file_of(to) == FILE_G;
        Square rookFrom = make_square(kingside ? FILE_H : FILE_A, rank_of(from));
        Square rookTo = make_square(kingside ? FILE_F : FILE_D, rank_of(from));
        move_piece(pos, to, from);
        move_piece(pos, rookTo, rookFrom);

        StateInfo& st = pos.history[--pos.historyPly];
        pos.castlingRights  = st.castlingRights;
        pos.enPassantSquare = st.enPassantSquare;
        pos.halfMoveClock   = st.halfMoveClock;
        pos.zobristHash     = st.zobristHash;
    }
    else {
        move_piece(pos, to, from);

        if (mt == PROMOTION){
            remove_piece(pos, from);
            put_piece(pos, from, make_piece(us, PAWN));
        }

        StateInfo& st = pos.history[--pos.historyPly];
        if(st.capturedPiece != NO_PIECE){
            Square capSq = to;
            if (mt == EN_PASSANT){
                capSq = make_square(file_of(to), rank_of(from));
            }
            put_piece(pos, capSq, st.capturedPiece);
        }

        pos.castlingRights  = st.castlingRights;
        pos.enPassantSquare = st.enPassantSquare;
        pos.halfMoveClock   = st.halfMoveClock;
        pos.zobristHash     = st.zobristHash;
    }

    if (us == BLACK){--pos.fullMoveNumber;}
}

// Function to generate all possible moves for the current position
std::vector<Move> generate_moves(const Position& pos){
    std::vector<Move> moves;
    Color us = pos.side_to_move();
    Color them = ~us;
    int dir = (us == WHITE) ? 8 : -8;
    int rank7 = (us == WHITE) ? 6 : 1;
    int rank2 = (us == WHITE) ? 1 : 6;

    Bitboard occ = pos.pieces(WHITE) | pos.pieces(BLACK);
    Bitboard enemies = pos.pieces(them);
    Bitboard empty = ~occ;

    // Pawns
    Bitboard pawns = pos.pieces(us, PAWN);
    while (pawns) {
        Square from = lsb(pawns); pawns &= pawns - 1;

        // Single push
        Square push1 = Square(from + dir);
        if (empty & (Bitboard(1) << push1)) {
            if (rank_of(from) == rank7) {
                moves.push_back(make_move<PROMOTION>(from, push1, QUEEN));
                moves.push_back(make_move<PROMOTION>(from, push1, ROOK));
                moves.push_back(make_move<PROMOTION>(from, push1, BISHOP));
                moves.push_back(make_move<PROMOTION>(from, push1, KNIGHT));
            } else {
                moves.push_back(make_move(from, push1));

                // Double push - only if on starting rank and both squares are empty
                if(rank_of(from) == rank2){
                    Square push2 = Square(from + dir * 2);
                    if (empty & (Bitboard(1) << push2)){
                        moves.push_back(make_move(from, push2));
                    }
                }
            }
        }

        // Captures
        Bitboard caps = PAWN_ATTACKS[us][from] & enemies;
        while (caps) {
            Square to = lsb(caps); caps &= caps - 1;
            if (rank_of(from) == rank7){
                moves.push_back(make_move<PROMOTION>(from, to, QUEEN));
                moves.push_back(make_move<PROMOTION>(from, to, ROOK));
                moves.push_back(make_move<PROMOTION>(from, to, BISHOP));
                moves.push_back(make_move<PROMOTION>(from, to, KNIGHT));
            }   else {
                    moves.push_back(make_move<NORMAL>(from, to));
            }
        }

        // En Passant
        if (pos.ep_square() != NO_SQUARE){
            if (PAWN_ATTACKS[us][from] & (Bitboard(1) << pos.ep_square()))
            {
                moves.push_back(make_move<EN_PASSANT>(from, pos.ep_square()));
            }
        }
    }

    // Knights
    Bitboard knights = pos.pieces(us, KNIGHT);
    while (knights) {
        Square from = lsb(knights); knights &= knights - 1;
        Bitboard atk = KNIGHT_ATTACKS[from] & ~pos.pieces(us);
        while (atk) {
            Square to = lsb(atk); atk &= atk - 1;
            moves.push_back(make_move<NORMAL>(from, to));
        }
    }

    // Kings
    Bitboard kings = pos.pieces(us, KING);
    while (kings) {
        Square from = lsb(kings); kings &= kings - 1;
        Bitboard atk = KING_ATTACKS[from] & ~pos.pieces(us);
        if (pos.can_castle(us == WHITE ? WHITE_KINGSIDE : BLACK_KINGSIDE)){
            Square f = us == WHITE ? F1 : F8;
            Square g = us == WHITE ? G1 : G8;
            if (pos.empty(f) && pos.empty(g)
                && !is_square_attacked(pos, from, them)   // not in check
                && !is_square_attacked(pos, f,    them))  // not through check
            {
                moves.push_back(make_move<CASTLING>(from, g));
            }
        }
        if (pos.can_castle(us == WHITE ? WHITE_QUEENSIDE : BLACK_QUEENSIDE)){
            Square b = us == WHITE ? B1 : B8;
            Square c = us == WHITE ? C1 : C8;
            Square d = us == WHITE ? D1 : D8;
            if (pos.empty(b) && pos.empty(c) && pos.empty(d)
                && !is_square_attacked(pos, from, them)   // not in check
                && !is_square_attacked(pos, d,    them))  // not through check
            {
                moves.push_back(make_move<CASTLING>(us == WHITE ? E1 : E8, c));
            }
        }
        while (atk) {
            Square to = lsb(atk); atk &= atk - 1;
            moves.push_back(make_move<NORMAL>(from, to));
        }
    }

    // Bishops
    Bitboard bishops = pos.pieces(us, BISHOP);
    while (bishops) {
        Square from = lsb(bishops); bishops &= bishops - 1;
        Bitboard atk = bishop_attacks(from, occ) & ~pos.pieces(us);
        while (atk) {
            Square to = lsb(atk); atk &= atk - 1;
            moves.push_back(make_move<NORMAL>(from, to));
        }
    }

    // Rooks
    Bitboard rooks = pos.pieces(us, ROOK);
    while (rooks) {
        Square from = lsb(rooks); rooks &= rooks - 1;
        Bitboard atk = rook_attacks(from, occ) & ~pos.pieces(us);
        while (atk) {
            Square to = lsb(atk); atk &= atk - 1;
            moves.push_back(make_move<NORMAL>(from, to));
        }
    }

// Queens
    Bitboard queens = pos.pieces(us, QUEEN);
    while (queens) {
        Square from = lsb(queens); queens &= queens - 1;
        Bitboard atk = queen_attacks(from, occ) & ~pos.pieces(us);
        while (atk) {
            Square to = lsb(atk); atk &= atk - 1;
            moves.push_back(make_move<NORMAL>(from, to));
        }
    }

    return moves;
}

std::vector<Move> generate_legal_moves(Position& pos){
    Color us = pos.side_to_move();
    std::vector<Move> legal;
    for (Move m : generate_moves(pos)){
        do_move(pos, m);
        if (!is_square_attacked(pos, lsb(pos.pieces(us, KING)), ~us)){
            legal.push_back(m);
        }
        undo_move(pos, m);
    }
    return legal;
}

uint64_t perft(Position& pos, int depth){
    if (depth == 0) return 1;
    uint64_t nodes = 0;
    for (Move m : generate_legal_moves(pos)){
        do_move(pos, m);
        nodes += perft(pos, depth - 1);
        undo_move(pos, m);
    }
    return nodes;
}