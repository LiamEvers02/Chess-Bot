#include <gtest/gtest.h>
#include "position.h"
#include "movegen.h"

// Helper: returns true if all observable position state matches
static bool positionsEqual(const Position& a, const Position& b) {
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1))
        if (a.board[s] != b.board[s]) return false;
    for (int i = 0; i < PIECE_TYPE_NB; ++i)
        if (a.byType[i] != b.byType[i]) return false;
    for (int i = 0; i < COLOR_NB; ++i)
        if (a.byColor[i] != b.byColor[i]) return false;
    return a.sideToMove      == b.sideToMove
        && a.castlingRights  == b.castlingRights
        && a.enPassantSquare == b.enPassantSquare
        && a.halfMoveClock   == b.halfMoveClock
        && a.fullMoveNumber  == b.fullMoveNumber;
}

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

// --- do_move / undo_move tests ---

TEST(MakeMoveTest, NormalMove) {
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Position before = pos;

    Move m = make_move(E2, E4);
    do_move(pos, m);

    EXPECT_EQ(pos.piece_on(E2), NO_PIECE);
    EXPECT_EQ(pos.piece_on(E4), W_PAWN);
    EXPECT_EQ(pos.side_to_move(), BLACK);
    EXPECT_EQ(pos.ep_square(), E3);

    undo_move(pos, m);
    EXPECT_TRUE(positionsEqual(pos, before));
}

TEST(MakeMoveTest, Capture) {
    Position pos;
    // White pawn on e5, black pawn on d6
    set(pos, "rnbqkbnr/ppp1pppp/3p4/4P3/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    Position before = pos;

    Move m = make_move(E5, D6);
    do_move(pos, m);

    EXPECT_EQ(pos.piece_on(E5), NO_PIECE);
    EXPECT_EQ(pos.piece_on(D6), W_PAWN);
    EXPECT_EQ(pos.side_to_move(), BLACK);
    EXPECT_EQ(pos.halfMoveClock, 0); // resets on capture

    undo_move(pos, m);
    EXPECT_TRUE(positionsEqual(pos, before));
}

TEST(MakeMoveTest, EnPassant) {
    Position pos;
    // White pawn on e5, black pawn just double-pushed to d5, ep square is d6
    set(pos, "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1");
    Position before = pos;

    Move m = make_move<EN_PASSANT>(E5, D6);
    do_move(pos, m);

    EXPECT_EQ(pos.piece_on(E5), NO_PIECE);
    EXPECT_EQ(pos.piece_on(D5), NO_PIECE); // captured pawn removed
    EXPECT_EQ(pos.piece_on(D6), W_PAWN);

    undo_move(pos, m);
    EXPECT_TRUE(positionsEqual(pos, before));
}

TEST(MakeMoveTest, KingsideCastle) {
    Position pos;
    // Kingside castling: squares f1, g1 clear
    set(pos, "rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1");
    Position before = pos;

    Move m = make_move<CASTLING>(E1, G1);
    do_move(pos, m);

    EXPECT_EQ(pos.piece_on(E1), NO_PIECE);
    EXPECT_EQ(pos.piece_on(G1), W_KING);
    EXPECT_EQ(pos.piece_on(H1), NO_PIECE);
    EXPECT_EQ(pos.piece_on(F1), W_ROOK);
    EXPECT_FALSE(pos.can_castle(WHITE_KINGSIDE));
    EXPECT_FALSE(pos.can_castle(WHITE_QUEENSIDE));

    undo_move(pos, m);
    EXPECT_TRUE(positionsEqual(pos, before));
}

TEST(MakeMoveTest, QueensideCastle) {
    Position pos;
    // Queenside castling: squares b1, c1, d1 clear
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3KBNR w KQkq - 0 1");
    Position before = pos;

    Move m = make_move<CASTLING>(E1, C1);
    do_move(pos, m);

    EXPECT_EQ(pos.piece_on(E1), NO_PIECE);
    EXPECT_EQ(pos.piece_on(C1), W_KING);
    EXPECT_EQ(pos.piece_on(A1), NO_PIECE);
    EXPECT_EQ(pos.piece_on(D1), W_ROOK);

    undo_move(pos, m);
    EXPECT_TRUE(positionsEqual(pos, before));
}

TEST(MakeMoveTest, Promotion) {
    Position pos;
    // White pawn on e7, ready to promote
    set(pos, "4k3/4P3/8/8/8/8/8/4K3 w - - 0 1");
    Position before = pos;

    Move m = make_move<PROMOTION>(E7, E8, QUEEN);
    do_move(pos, m);

    EXPECT_EQ(pos.piece_on(E7), NO_PIECE);
    EXPECT_EQ(pos.piece_on(E8), W_QUEEN);
    EXPECT_EQ(pos.halfMoveClock, 0);

    undo_move(pos, m);
    EXPECT_TRUE(positionsEqual(pos, before));
}

TEST(MakeMoveTest, CastlingRightsRevokedOnKingMove) {
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Move king — both castling rights should be lost
    Move m = make_move(E1, E2);
    do_move(pos, m);

    EXPECT_FALSE(pos.can_castle(WHITE_KINGSIDE));
    EXPECT_FALSE(pos.can_castle(WHITE_QUEENSIDE));
    EXPECT_TRUE(pos.can_castle(BLACK_KINGSIDE));
    EXPECT_TRUE(pos.can_castle(BLACK_QUEENSIDE));
}

TEST(AttackTest, KnightCenter){
    init_attacks();
    EXPECT_EQ(popcount(KNIGHT_ATTACKS[E4]), 8);
}

TEST(AttackTest, KnightCorner){
    init_attacks();
    EXPECT_EQ(popcount(KNIGHT_ATTACKS[A1]), 2);
}

TEST(AttackTest, KingCenter){
    init_attacks();
    EXPECT_EQ(popcount(KING_ATTACKS[E4]), 8);
}

TEST(AttackTest, KingCorner){
    init_attacks();
    EXPECT_EQ(popcount(KING_ATTACKS[A1]), 3);
}

TEST(AttackTest, PawnWhiteCenter){
    init_attacks();
    EXPECT_EQ(popcount(PAWN_ATTACKS[WHITE][E4]), 2);
}

TEST(AttackTest, PawnBlackCenter){
    init_attacks();
    EXPECT_EQ(popcount(PAWN_ATTACKS[BLACK][E4]), 2);
}

TEST(AttackTest, RookAttacksEmpty){
    EXPECT_EQ(popcount(rook_attacks(A1, 0)), 14);
}

TEST(AttackTest, RookAttacksBlocked){
    Bitboard occ = Bitboard(1) << A4;
    Bitboard atk = rook_attacks(A1, occ);
    EXPECT_TRUE(atk & (Bitboard(1) << A4));
    EXPECT_FALSE(atk & (Bitboard(1) << A5));
}

TEST(AttackTest, BishopAttacksEmpty){
    EXPECT_EQ(popcount(bishop_attacks(D4, 0)), 13);
}

TEST(AttackTest, BishopAttacksBlocked){
    Bitboard occ = Bitboard(1) << B6;
    Bitboard atk = bishop_attacks(D4, occ);
    EXPECT_TRUE(atk & (Bitboard(1) << B6));
    EXPECT_FALSE(atk & (Bitboard(1) << A7));
}

// --- generate_moves pawn tests ---

TEST(PawnMoveTest, SinglePush) {
    init_attacks();
    Position pos;
    set(pos, "8/8/8/8/8/8/4P3/8 w - - 0 1");
    auto moves = generate_moves(pos);
    // E2 can push to E3 and E4 (double push)
    bool foundE3 = false, foundE4 = false;
    for (auto m : moves) {
        if (from_sq(m) == E2 && to_sq(m) == E3) foundE3 = true;
        if (from_sq(m) == E2 && to_sq(m) == E4) foundE4 = true;
    }
    EXPECT_TRUE(foundE3);
    EXPECT_TRUE(foundE4);
}

TEST(PawnMoveTest, BlockedPawn) {
    init_attacks();
    Position pos;
    // Pawn blocked by own piece directly in front
    set(pos, "8/8/8/8/8/4P3/4P3/8 w - - 0 1");
    auto moves = generate_moves(pos);
    // E2 pawn is blocked by E3 pawn — no pushes from E2
    for (auto m : moves)
        EXPECT_NE(from_sq(m), E2);
}

TEST(PawnMoveTest, DoublePushOnlyFromRank2) {
    init_attacks();
    Position pos;
    // Pawn already advanced to rank 3 — no double push
    set(pos, "8/8/8/8/8/4P3/8/8 w - - 0 1");
    auto moves = generate_moves(pos);
    bool foundDoubleJump = false;
    for (auto m : moves)
        if (from_sq(m) == E3 && to_sq(m) == E5) foundDoubleJump = true;
    EXPECT_FALSE(foundDoubleJump);
}

TEST(PawnMoveTest, Capture) {
    init_attacks();
    Position pos;
    // White pawn on E4, black pawns on D5 and F5
    set(pos, "8/8/8/3p1p2/4P3/8/8/8 w - - 0 1");
    auto moves = generate_moves(pos);
    bool foundD5 = false, foundF5 = false;
    for (auto m : moves) {
        if (from_sq(m) == E4 && to_sq(m) == D5) foundD5 = true;
        if (from_sq(m) == E4 && to_sq(m) == F5) foundF5 = true;
    }
    EXPECT_TRUE(foundD5);
    EXPECT_TRUE(foundF5);
}

TEST(PawnMoveTest, EnPassant) {
    init_attacks();
    Position pos;
    // White pawn E5, black pawn just pushed to D5, ep square D6
    set(pos, "8/8/8/3pP3/8/8/8/8 w - d6 0 1");
    auto moves = generate_moves(pos);
    bool foundEP = false;
    for (auto m : moves)
        if (from_sq(m) == E5 && to_sq(m) == D6 && type_of(m) == EN_PASSANT) foundEP = true;
    EXPECT_TRUE(foundEP);
}

TEST(PawnMoveTest, PromotionEmitsFourMoves) {
    init_attacks();
    Position pos;
    // White pawn on E7, clear path to E8
    set(pos, "8/4P3/8/8/8/8/8/8 w - - 0 1");
    auto moves = generate_moves(pos);
    int promoCount = 0;
    for (auto m : moves)
        if (from_sq(m) == E7 && to_sq(m) == E8 && type_of(m) == PROMOTION) promoCount++;
    EXPECT_EQ(promoCount, 4);
}

TEST(MoveGenTest, StartingPosition) {
    init_attacks();
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto moves = generate_moves(pos);
    EXPECT_EQ(moves.size(), 20); // 16 pawn moves + 4 knight moves
}

TEST(MoveGenTest, StartingPositionLegal) {
    init_attacks();
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto moves = generate_legal_moves(pos);
    EXPECT_EQ(moves.size(), 20); // all moves are legal in the starting position
}

TEST(PerftTest, Depth1) {
    init_attacks();
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(pos, 1), 20);
}

TEST(PerftTest, Depth2) {
    init_attacks();
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(pos, 2), 400ULL);
}

TEST(PerftTest, Depth3) {
    init_attacks();
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(pos, 3), 8902ULL);
}

TEST(PerftTest, Depth4) {
    init_attacks();
    Position pos;
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(pos, 4), 197281ULL);
}

