#include <gtest/gtest.h>
#include "../src/position.h"

TEST(PositionTest, FenStartingPosition){
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(pos.side_to_move(), WHITE);
    EXPECT_EQ(pos.piece_on(E1), W_KING);
    EXPECT_EQ(pos.piece_on(D8), B_QUEEN);
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