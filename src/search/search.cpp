#include "search/search.h"
#include "engine/movegen.h"
#include "engine/attacks.h"
#include <cstring>
#include <chrono>
#include "eval.h"
#include "tt.h"
#include <limits>
#include <algorithm>

static const int INF = std::numeric_limits<int>::max() / 2;
static Move killers[64][2];
static int history[2][64][64]; // [color][from][to]

// Time management
using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
static TimePoint g_startTime;
static int       g_timeLimitMs  = 0;
static bool      g_stop         = false;
static int       g_nodeCount    = 0;
static const int TIME_CHECK_NODES = 2048; // check clock every N nodes

static bool out_of_time() {
    if (g_timeLimitMs == 0) return false;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - g_startTime).count();
    return elapsed >= g_timeLimitMs;
}
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

static void order_moves(const Position& pos, std::vector<Move>& moves, int ply) {
    int color = pos.side_to_move();
    std::sort(moves.begin(), moves.end(), [&](Move a, Move b) {
        bool a_cap = pos.piece_on(to_sq(a)) != NO_PIECE || type_of(a) == EN_PASSANT;
        bool b_cap = pos.piece_on(to_sq(b)) != NO_PIECE || type_of(b) == EN_PASSANT;
        if (a_cap != b_cap) return a_cap > b_cap;
        if (a_cap && b_cap) return mvv_lva(pos, a) > mvv_lva(pos, b);
        // Killers beat other quiet moves
        bool a_killer = (a == killers[ply][0] || a == killers[ply][1]);
        bool b_killer = (b == killers[ply][0] || b == killers[ply][1]);
        if (a_killer != b_killer) return a_killer > b_killer;
        // History score for remaining quiet moves
        return history[color][from_sq(a)][to_sq(a)] > history[color][from_sq(b)][to_sq(b)];
    });
}
static int quiesce(Position& pos, int alpha, int beta, int ply) {
    int stand_pat = evaluate(pos);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    auto moves = generate_legal_moves(pos);
    order_moves(pos, moves, ply);
    for (Move m : moves) {
        if (pos.piece_on(to_sq(m)) == NO_PIECE && type_of(m) != EN_PASSANT) continue; // not a capture
        do_move(pos, m);
        int score = -quiesce(pos, -beta, -alpha, ply + 1);
        undo_move(pos, m);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}
static int minimax(Position& pos, int depth, int alpha, int beta, int ply, bool allowNull = true) {
    if (g_stop) return 0;

    // Periodically check the clock
    if ((++g_nodeCount & (TIME_CHECK_NODES - 1)) == 0 && out_of_time())
        g_stop = true;
    if (g_stop) return 0;

    if (depth == 0) return quiesce(pos, alpha, beta, ply);
    TTEntry* entry = tt_probe(pos.hash());
    if (entry && entry->depth >= depth){
        if (entry->flag == EXACT) return entry->score;
        else if (entry->flag == LOWERBOUND) alpha = std::max(alpha, entry->score);
        else if (entry->flag == UPPERBOUND) beta = std::min(beta, entry->score);
        if (alpha >= beta) return entry->score;
    }
    int origAlpha = alpha;

    // Null move pruning
    Square ks = lsb(pos.pieces(pos.side_to_move(), KING));
    bool inCheck = is_square_attacked(pos, ks, ~pos.side_to_move());
    if (allowNull && depth >= 3 && !inCheck) {
        bool hasNonPawns = popcount(pos.pieces(pos.side_to_move())
            & ~pos.pieces(PAWN) & ~pos.pieces(KING)) > 0;
        if (hasNonPawns) {
            int R = (depth >= 6) ? 3 : 2;
            do_null_move(pos);
            int nullScore = -minimax(pos, depth - 1 - R, -beta, -beta + 1, ply + 1, false);
            undo_null_move(pos);
            if (nullScore >= beta) return beta;
        }
    }

    auto moves = generate_legal_moves(pos);
    if (moves.empty()) {
        if (inCheck)
            return -INF;
        return 0; // stalemate
    }
    order_moves(pos, moves, ply);
    int best = -INF;
    for (int i = 0; i < (int)moves.size(); i++) {
        Move m = moves[i];
        bool isCapture = pos.piece_on(to_sq(m)) != NO_PIECE || type_of(m) == EN_PASSANT;
        bool isPromotion = type_of(m) == PROMOTION;
        bool isKiller = (m == killers[ply][0] || m == killers[ply][1]);

        do_move(pos, m);

        int score;
        // LMR: reduce quiet, non-killer moves late in the list
        if (i >= 2 && depth >= 3 && !inCheck && !isCapture && !isPromotion && !isKiller) {
            int R = 1 + (i >= 6 ? 1 : 0); // reduce by 2 for very late moves
            // Reduced search with null window
            score = -minimax(pos, depth - 1 - R, -alpha - 1, -alpha, ply + 1);
            // If it beats alpha, re-search at full depth
            if (score > alpha)
                score = -minimax(pos, depth - 1, -beta, -alpha, ply + 1);
        } else {
            score = -minimax(pos, depth - 1, -beta, -alpha, ply + 1);
        }

        undo_move(pos, m);
        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            if (!isCapture) {
                if (killers[ply][0] != m) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = m;
                }
                history[pos.side_to_move()][from_sq(m)][to_sq(m)] += depth * depth;
            }
            break;
        }
    }
    TTFlag flag = (best <= origAlpha) ? UPPERBOUND : (best >= beta) ? LOWERBOUND : EXACT;
    tt_store(pos.hash(), depth, best, flag);
    return best;
}

Move best_move(Position& pos, int depth, int timeLimitMs) {
    g_startTime   = Clock::now();
    g_timeLimitMs = timeLimitMs;
    g_stop        = false;
    g_nodeCount   = 0;

    auto moves = generate_legal_moves(pos);
    if (moves.empty()) return Move{};
    memset(killers, 0, sizeof(killers));
    memset(history, 0, sizeof(history));
    order_moves(pos, moves, 0);

    Move best = moves[0];
    int prevScore = 0;
    const int DELTA = 50;

    for (int d = 1; d <= depth; d++) {
        int alpha = (d > 1) ? prevScore - DELTA : -INF;
        int beta  = (d > 1) ? prevScore + DELTA : INF;
        bool retried = false;

    retry:
        Move iterBest = moves[0];
        int bestScore = -INF;
        for (Move m : moves) {
            do_move(pos, m);
            int score = -minimax(pos, d - 1, -beta, -alpha, 1);
            undo_move(pos, m);
            if (score > bestScore) { bestScore = score; iterBest = m; }
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        }

        if (!retried && bestScore <= prevScore - DELTA && d > 1) {
            alpha = -INF; beta = INF; retried = true; goto retry;
        }
        if (!retried && bestScore >= prevScore + DELTA && d > 1) {
            alpha = -INF; beta = INF; retried = true; goto retry;
        }

        best = iterBest;
        prevScore = bestScore;
        auto it = std::find(moves.begin(), moves.end(), best);
        if (it != moves.begin()) std::rotate(moves.begin(), it, it + 1);

        if (g_stop || out_of_time()) break;
    }
    return best;
}
