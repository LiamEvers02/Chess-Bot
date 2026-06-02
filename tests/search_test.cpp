#include <gtest/gtest.h>
#include "position.h"
#include "movegen.h"
#include "attacks.h"
#include "search/search.h"

// k7/8/1K6/8/8/8/8/7R w - - 0 1
// White: Kb6, Rh1. Black: Ka8. Rh8# is the only mating move.
TEST(SearchTest, FindsMateInOne) {
    init_attacks();
    Position pos;
    set(pos, "k7/8/1K6/8/8/8/8/7R w - - 0 1");
    Move m = best_move(pos, 2);
    EXPECT_EQ(from_sq(m), H1);
    EXPECT_EQ(to_sq(m),   H8);
}

// 4k3/8/8/8/8/8/3q4/4K3 w - - 0 1
// White: Ke1. Black: Ke8, Qd2. White should capture Kxd2 (free queen).
TEST(SearchTest, CapturesHangingQueen) {
    init_attacks();
    Position pos;
    set(pos, "4k3/8/8/8/8/8/3q4/4K3 w - - 0 1");
    Move m = best_move(pos, 1);
    EXPECT_EQ(from_sq(m), E1);
    EXPECT_EQ(to_sq(m),   D2);
}
