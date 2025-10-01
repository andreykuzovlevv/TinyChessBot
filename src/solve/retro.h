#pragma once
#include <cstdint>
#include <vector>

#include "../core/move.h"
#include "../core/position.h"

namespace tiny::retro
{

    enum class WDL : uint8_t
    {
        Loss = 0,
        Draw = 1,
        Win = 2
    };

    struct TBRecord
    {
        uint64_t key; // Zobrist of Position (side-to-move included)
        WDL wdl;      // from the side-to-move perspective
        uint16_t dtm; // plies to mate (0 for terminals, saturate if needed)
        Move best;    // packed move; 0 if none/Draw
    };

    // Compute the complete WDL+DTM table for all positions reachable from `start`.
    // Returns one record per distinct position, sorted order not guaranteed.
    std::vector<TBRecord> build_wdl_dtm(const Position &start);

} // namespace tiny::retro
