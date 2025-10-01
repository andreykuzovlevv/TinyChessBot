#include "minmax.h"

namespace tiny {

// Material-only evaluation, side-to-move perspective.
// Positive means the side to move is better.
Value evaluate(const Position& pos) {
    // Board material
    Value diff = 0;
    for (int sq = 0; sq < SQUARE_NB; ++sq) diff += piece_value(pos.board[sq]);

    // Pocket material (drops)
    // Only PAWN/HORSE/FERZ/WAZIR are used in pockets
    for (int pt = PAWN; pt <= WAZIR; ++pt) {
        diff += pos.pockets.p[WHITE][pt] * type_value(static_cast<PieceType>(pt));
        diff -= pos.pockets.p[BLACK][pt] * type_value(static_cast<PieceType>(pt));
    }

    // Perspective: return score for side to move
    if (pos.sideToMove == BLACK) diff = -diff;

    return diff;
}

// Core negamax with alpha-beta pruning.
// Returns a score from the perspective of the side to move in 'pos'.
Value negamax(Position& pos, int depth, int alpha, int beta, int ply) {
    // Repetition draw
    if (pos.is_draw(ply)) return VALUE_DRAW;

    if (depth == 0) return evaluate(pos);

    MoveList<LEGAL> moves(pos);

    // No legal moves: terminal
    if (moves.size() == 0) {
        if (pos.in_check()) {
            // Checkmate: side to move loses
            return -VALUE_MATE + ply;  // prefer quicker mates against us
        } else {
            // Stalemate: side to move WINS
            return +VALUE_MATE - ply;  // prefer quicker wins for us
        }
    }

    int best = -VALUE_INFINITE;  // track true best value found

    for (const Move& m : moves) {
        StateInfo st;
        pos.do_move(m, st);

        // Negamax flip and window flip
        int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1);

        pos.undo_move(m);

        if (score > best) best = score;
        if (score > alpha) {
            alpha = score;
            // beta cutoff
            if (alpha >= beta) break;
        }
    }

    return best;
}

// Returns the best move and its score for the current position.
SearchResult search_best_move(Position& pos, int depth) {
    MoveList<LEGAL> moves(pos);

    // Handle immediate terminals at root
    if (moves.size() == 0) {
        int terminalScore =
            pos.in_check() ? (-VALUE_MATE /* + ply=0 */) : (+VALUE_MATE /* - ply=0 */);
        return {MOVE_NONE, terminalScore};
    }
    if (pos.is_draw(/*ply=*/0)) return {MOVE_NONE, VALUE_DRAW};

    int alpha = -VALUE_MATE;
    int beta  = +VALUE_MATE;

    Move bestMove  = MOVE_NONE;
    int  bestScore = -VALUE_MATE;

    for (const Move& m : moves) {
        StateInfo st;
        pos.do_move(m, st);

        int score = -negamax(pos, depth - 1, -beta, -alpha, /*ply=*/1);

        pos.undo_move(m);

        if (score > bestScore) {
            bestScore = score;
            bestMove  = m;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return {bestMove, bestScore};
}
}  // namespace tiny

/*
Example usage:

int main() {
    Position pos;                 // however you construct it
    // ... set up position ...

    int depth = 6;
    SearchResult res = search_best_move(pos, depth);

    // If you have a UCI helper: UCI::move(res.bestMove, pos.is_chess960())
    std::cout << "bestmove " << UCI::move(res.bestMove, false)
              << " score " << res.score << "\n";
}
*/
