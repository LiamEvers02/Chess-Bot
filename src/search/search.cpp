#include "search/search.h"
#include "engine/movegen.h"
#include "engine/attacks.h"
#include <limits>
#include <algorithm>

static const int INF = std::numeric_limits<int>::max() / 2;
static const int PIECE_VALUE[PIECE_TYPE_NB] = {0, 100, 320, 330, 500, 900, 0};

static int evaluate(const Position& pos) {
    int score = 0;
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Piece p = pos.piece_on(s);
        if (p == NO_PIECE) continue;
        int val = PIECE_VALUE[type_of(p)];
        score += (color_of(p) == WHITE) ? val : -val;
    }
    return score;
}

static int minimax(Position& pos, int depth, int alpha, int beta) {
    if (depth == 0) return evaluate(pos);
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) {
        Square ks = lsb(pos.pieces(pos.side_to_move(), KING));
        if (is_square_attacked(pos, ks, ~pos.side_to_move()))
            return (pos.side_to_move() == WHITE) ? -INF : INF; // checkmate
        return 0; // stalemate
    }
    if (pos.side_to_move() == WHITE) {
        int best = -INF;
        for (Move m : moves) {
            do_move(pos, m);
            best = std::max(best, minimax(pos, depth - 1, alpha, beta));
            undo_move(pos, m);
            alpha = std::max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int best = INF;
        for (Move m : moves) {
            do_move(pos, m);
            best = std::min(best, minimax(pos, depth - 1, alpha, beta));
            undo_move(pos, m);
            beta = std::min(beta, best);
            if (beta <= alpha) break;
        }
        return best;
    }
}

Move best_move(Position& pos, int depth) {
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) return Move{};
    Move best = moves[0];
    int alpha = -INF, beta = INF;
    if (pos.side_to_move() == WHITE) {
        int bestScore = -INF;
        for (Move m : moves) {
            do_move(pos, m);
            int score = minimax(pos, depth - 1, alpha, beta);
            undo_move(pos, m);
            if (score > bestScore) { bestScore = score; best = m; }
            alpha = std::max(alpha, bestScore);
        }
    } else {
        int bestScore = INF;
        for (Move m : moves) {
            do_move(pos, m);
            int score = minimax(pos, depth - 1, alpha, beta);
            undo_move(pos, m);
            if (score < bestScore) { bestScore = score; best = m; }
            beta = std::min(beta, bestScore);
        }
    }
    return best;
}

