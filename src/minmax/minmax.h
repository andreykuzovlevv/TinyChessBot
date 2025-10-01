// minmax.h
#ifndef MINMAX_H_INCLUDED
#define MINMAX_H_INCLUDED

#include "../core/movegen.h"   // For MoveList
#include "../core/position.h"  // For Position class
#include "../core/types.h"     // For Move, Score typedefs, etc.

namespace tiny {

// Constants

constexpr Move MOVE_NONE = Move::none();

// Simple search wrapper for a single PV move at the root.
struct SearchResult {
    Move  bestMove;
    Value score;
};

SearchResult search_best_move(Position& pos, int depth);
}  // namespace tiny

#endif  // MINMAX_H_INCLUDED