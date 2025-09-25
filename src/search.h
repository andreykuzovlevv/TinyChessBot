#pragma once
#include "position.h"
#include "tt.h"

namespace tiny
{

    struct SearchLimits
    {
        int depth = 6;
        int nodes = 0;
    };

    struct SearchResult
    {
        Move best;
        int score = 0; // centipawns or mate score with convention
        uint64_t nodes = 0;
    };

    class Search
    {
    public:
        explicit Search(size_t ttMB = 16) : tt_(ttMB) {}
        SearchResult go(Position &pos, const SearchLimits &lim);

    private:
        TT tt_;
        int negamax(Position &pos, int depth, int alpha, int beta, uint64_t &nodes);
        bool is_terminal(Position &pos, int ply, int &score_out);
    };

} // namespace tiny
