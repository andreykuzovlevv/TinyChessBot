#pragma once
#include "types.h"
#include <vector>
#include <cstdint>

namespace tiny
{

    enum Bound : uint8_t
    {
        BOUND_NONE,
        BOUND_EXACT,
        BOUND_LOWER,
        BOUND_UPPER
    };

    struct TTEntry
    {
        Key key = 0;
        int16_t score = 0;
        int8_t depth = 0;
        uint8_t bound = BOUND_NONE;
        uint16_t move = 0; // pack our Move
    };

    class TT
    {
    public:
        explicit TT(size_t MB = 8) { resize(MB); }
        void resize(size_t MB);
        void clear();
        void store(Key k, int depth, int score, Bound b, uint16_t move);
        bool probe(Key k, TTEntry &out) const;

    private:
        std::vector<TTEntry> t_;
    };
} // namespace tiny
