#pragma once
#include "position.h"
#include <vector>

namespace tiny
{

    class MoveList
    {
    public:
        void clear() { moves_.clear(); }
        void add(Move m) { moves_.push_back(m); }
        const std::vector<Move> &vec() const { return moves_; }

    private:
        std::vector<Move> moves_;
    };

    // Generate pseudo-legal, then filter legality (king safety)
    void generate_legal_moves(const Position &pos, MoveList &out);

    // Helpers (internal): add moves for each piece, and add drop moves
}
