#pragma once
#include "types.h"

namespace tiny
{
    struct Zobrist
    {
        Key psq[PT_NB][COLOR_NB][16];
        Key side;
        // Include reserves: counts 0..4 in practice; index by piece type & count
        Key reserve[PT_NB][COLOR_NB][5];
    };
    extern Zobrist ZB;
    void init_zobrist(uint64_t seed = 20250926ull);
} // namespace tiny
