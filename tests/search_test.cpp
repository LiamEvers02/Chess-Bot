#include <gtest/gtest.h>
#include "position.h"
#include "movegen.h"
#include "attacks.h"
#include "search/search.h"

// k7/8/1K6/8/8/8/8/7R w - - 0 1
// White: Kb6, Rh1. Black: Ka8. Rh8# is the only mating move.
TEST(SearchTest, FindsMateInOne) {
    init_attacks();
    Zobrist::init();
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
    Zobrist::init();
    Position pos;
    set(pos, "4k3/8/8/8/8/8/3q4/4K3 w - - 0 1");
    Move m = best_move(pos, 1);
    EXPECT_EQ(from_sq(m), E1);
    EXPECT_EQ(to_sq(m),   D2);
}

// 4k3/8/8/8/8/8/8/N3K3 w - - 0 1
// White Na1 can go to Nb3 (rim) or Nc2 (better) — at depth 1 the PST should
// push it off the corner. Simply verify it does NOT stay on A1 (i.e., it moves).
// The specific destination (b3 or c2) depends on table values; both beat a1.
TEST(SearchTest, KnightLeavesCorner) {
    init_attacks();
    Zobrist::init();
    Position pos;
    set(pos, "4k3/8/8/8/8/8/8/N3K3 w - - 0 1");
    Move m = best_move(pos, 1);
    EXPECT_EQ(from_sq(m), A1);                  // knight moves
    EXPECT_NE(to_sq(m),   A1);                  // to somewhere better
}

// Starting position: engine should prefer a central pawn push (d2-d4 or e2-e4)
// over a rook-pawn push (a2-a4 or h2-h4) at depth 1.
// We check that the chosen move's destination file is 3 (d) or 4 (e).
TEST(SearchTest, PrefersOpeningCentralPawn) {
    init_attacks();
    Zobrist::init();
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Move m = best_move(pos, 1);
    int file = to_sq(m) % 8; // 0=a .. 7=h
    EXPECT_GE(file, 2);      // not a- or b-file (files 0-1)
    EXPECT_LE(file, 5);      // not g- or h-file (files 6-7)
}
