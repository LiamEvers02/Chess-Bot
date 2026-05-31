#include "position.h"
#include "movegen.h"


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

    pos.enPassantSquare = NO_SQUARE;
    ++pos.halfMoveClock;

    if (mt == CASTLING) {
        bool kingside = file_of(to) == FILE_G;
        Square rookFrom = make_square(kingside ? FILE_H : FILE_A, rank_of(from));
        Square rookTo = make_square(kingside ? FILE_F : FILE_D, rank_of(from));
        move_piece(pos, from, to);
        move_piece(pos, rookFrom, rookTo);
    }
    else {
        Square capSq = to;
        if (mt == EN_PASSANT){
            capSq = make_square(file_of(to), rank_of(from));
        }
        if (pos.board[capSq] != NO_PIECE){
            st.capturedPiece = pos.board[capSq];
            remove_piece(pos, capSq);
            pos.halfMoveClock = 0;
        }

        move_piece(pos, from, to);

        if (mt == PROMOTION){
            remove_piece(pos, to);
            put_piece(pos, to, make_piece(us, promotion_type(m)));
        }

        if (type_of(pos.board[to]) == PAWN || mt == PROMOTION){
            pos.halfMoveClock = 0;
            if (abs(rank_of(to) - rank_of(from)) == 2){
                pos.enPassantSquare = make_square(file_of(from), (rank_of(from) + rank_of(to)) / 2);
            }
        }
    }
    if (pos.can_castle(ANY_CASTLING)){
        if (from == E1 || to == E1){pos.castlingRights = pos.castlingRights & ~ (WHITE_KINGSIDE | WHITE_QUEENSIDE);}
        if (from == E8 || to == E8){pos.castlingRights = pos.castlingRights & ~ (BLACK_KINGSIDE | BLACK_QUEENSIDE);}
        if (from == H1 || to == H1){pos.castlingRights = pos.castlingRights & ~ WHITE_KINGSIDE;}
        if (from == A1 || to == A1){pos.castlingRights = pos.castlingRights & ~ WHITE_QUEENSIDE;}
        if (from == H8 || to == H8){pos.castlingRights = pos.castlingRights & ~ BLACK_KINGSIDE;}
        if (from == A8 || to == A8){pos.castlingRights = pos.castlingRights & ~ BLACK_QUEENSIDE;}
    }

    if (us == BLACK){++pos.fullMoveNumber;}
    pos.sideToMove = them;
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

        pos.castlingRights = st.castlingRights;
        pos.enPassantSquare = st.enPassantSquare;
        pos.halfMoveClock = st.halfMoveClock;
    }

    if (us == BLACK){--pos.fullMoveNumber;}
}