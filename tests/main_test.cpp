#include <gtest/gtest.h>
#include "../src/position.h"

TEST(PositionTest, FenStartingPosition){
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(pos.side_to_move(), WHITE);
    EXPECT_EQ(pos.piece_on(A1), W_ROOK);
    EXPECT_EQ(pos.piece_on(B1), W_KNIGHT);
    EXPECT_EQ(pos.piece_on(C1), W_BISHOP);
    EXPECT_EQ(pos.piece_on(D1), W_QUEEN);
    EXPECT_EQ(pos.piece_on(E1), W_KING);
    EXPECT_EQ(pos.piece_on(F1), W_BISHOP);
    EXPECT_EQ(pos.piece_on(G1), W_KNIGHT);
    EXPECT_EQ(pos.piece_on(H1), W_ROOK);
    EXPECT_EQ(pos.piece_on(A2), W_PAWN);
    EXPECT_EQ(pos.piece_on(B2), W_PAWN);
    EXPECT_EQ(pos.piece_on(C2), W_PAWN);
    EXPECT_EQ(pos.piece_on(D2), W_PAWN);
    EXPECT_EQ(pos.piece_on(E2), W_PAWN);
    EXPECT_EQ(pos.piece_on(F2), W_PAWN);
    EXPECT_EQ(pos.piece_on(G2), W_PAWN);
    EXPECT_EQ(pos.piece_on(H2), W_PAWN);
    EXPECT_EQ(pos.piece_on(A8), B_ROOK);
    EXPECT_EQ(pos.piece_on(B8), B_KNIGHT);
    EXPECT_EQ(pos.piece_on(C8), B_BISHOP);
    EXPECT_EQ(pos.piece_on(D8), B_QUEEN);
    EXPECT_EQ(pos.piece_on(E8), B_KING);
    EXPECT_EQ(pos.piece_on(F8), B_BISHOP);
    EXPECT_EQ(pos.piece_on(G8), B_KNIGHT);
    EXPECT_EQ(pos.piece_on(H8), B_ROOK);
    EXPECT_EQ(pos.piece_on(A7), B_PAWN);
    EXPECT_EQ(pos.piece_on(B7), B_PAWN);
    EXPECT_EQ(pos.piece_on(C7), B_PAWN);
    EXPECT_EQ(pos.piece_on(D7), B_PAWN);
    EXPECT_EQ(pos.piece_on(E7), B_PAWN);
    EXPECT_EQ(pos.piece_on(F7), B_PAWN);
    EXPECT_EQ(pos.piece_on(G7), B_PAWN);
    EXPECT_EQ(pos.piece_on(H7), B_PAWN);
}

TEST(PositionTest, FenEnPassant){
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/4P3/8/pppp1ppp/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(pos.ep_square(), E3);
}

TEST(PositionTest, FenCastlingRights){
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kk - 0 1");
    EXPECT_TRUE(pos.can_castle(WHITE_KINGSIDE));
    EXPECT_TRUE(pos.can_castle(BLACK_KINGSIDE));
    EXPECT_FALSE(pos.can_castle(WHITE_QUEENSIDE));
    EXPECT_FALSE(pos.can_castle(BLACK_QUEENSIDE));
}