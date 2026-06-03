#include "search/search.h"
#include "engine/movegen.h"
#include "engine/attacks.h"
#include "eval.h"
#include "tt.h"
#include <limits>
#include <algorithm>

static const int INF = std::numeric_limits<int>::max() / 2;

// MVV-LVA: score captures by (victim value * 10 - attacker value)
static int mvv_lva(const Position& pos, Move m) {
    Square to = to_sq(m);
    PieceType attacker = type_of(pos.piece_on(from_sq(m)));
    PieceType victim;
    if (type_of(m) == EN_PASSANT) {
        victim = PAWN;
    } else {
        victim = type_of(pos.piece_on(to));
    }
    return PIECE_VALUE[victim] * 10 - PIECE_VALUE[attacker];
}

static void order_moves(const Position& pos, std::vector<Move>& moves) {
    std::sort(moves.begin(), moves.end(), [&](Move a, Move b) {
        bool a_cap = pos.piece_on(to_sq(a)) != NO_PIECE || type_of(a) == EN_PASSANT;
        bool b_cap = pos.piece_on(to_sq(b)) != NO_PIECE || type_of(b) == EN_PASSANT;
        if (a_cap != b_cap) return a_cap > b_cap; // captures first
        if (a_cap && b_cap) return mvv_lva(pos, a) > mvv_lva(pos, b);
        return false;
    });
}

static int quiesce(Position& pos, int alpha, int beta) {
    int stand_pat = evaluate(pos);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    auto moves = generate_legal_moves(pos);
    order_moves(pos, moves);
    for (Move m : moves) {
        if (pos.piece_on(to_sq(m)) == NO_PIECE && type_of(m) != EN_PASSANT) continue; // not a capture
        do_move(pos, m);
        int score = -quiesce(pos, -beta, -alpha);
        undo_move(pos, m);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}
static int minimax(Position& pos, int depth, int alpha, int beta) {
    if (depth == 0) return quiesce(pos, alpha, beta);
    TTEntry* entry = tt_probe(pos.hash());
    if (entry && entry->depth >= depth){
        if (entry->flag == EXACT) return entry->score;
        else if (entry->flag == LOWERBOUND) alpha = std::max(alpha, entry->score);
        else if (entry->flag == UPPERBOUND) beta = std::min(beta, entry->score);
        if (alpha >= beta) return entry->score;
    }
    int origAlpha = alpha;
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) {
        Square ks = lsb(pos.pieces(pos.side_to_move(), KING));
        if (is_square_attacked(pos, ks, ~pos.side_to_move()))
            return -INF;
        return 0; // stalemate
    }
    order_moves(pos, moves);
    int best = -INF;
    for (Move m : moves) {
        do_move(pos, m);
        int score = -minimax(pos, depth - 1, -beta, -alpha);
        undo_move(pos, m);
        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }
    TTFlag flag = (best <= origAlpha) ? UPPERBOUND : (best >= beta) ? LOWERBOUND : EXACT;
    tt_store(pos.hash(), depth, best, flag);
    return best;
}

Move best_move(Position& pos, int depth) {
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) return Move{};
    order_moves(pos, moves);

    Move best = moves[0];
    for (int d = 1; d <= depth; d++) {
        Move iterBest = moves[0];
        int bestScore = -INF;
        int alpha = -INF;
        for (Move m : moves) {
            do_move(pos, m);
            int score = -minimax(pos, d - 1, -INF, -alpha);
            undo_move(pos, m);
            if (score > bestScore) { bestScore = score; iterBest = m;}
            if (score > alpha) alpha = score;
        }
        best = iterBest;
        auto it = std::find(moves.begin(), moves.end(), best);
        if (it != moves.begin()) std::rotate(moves.begin(), it, it + 1);
    }
    return best;
}

