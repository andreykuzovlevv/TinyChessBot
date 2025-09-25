#pragma once
#include "types.h"

namespace tiny
{

    struct AttackTables
    {
        Bitboard king[16];
        Bitboard wazir[16];
        Bitboard ferz[16];
        // Horse (U) is blockable; we’ll provide a function instead of table.
    };

    extern AttackTables ATK;

    void init_attack_tables();

    // Blockable Xiangqi horse: first orthogonal must be empty
    inline Bitboard horse_attacks(Square s, Bitboard occ)
    {
        Bitboard res = 0;
        int f = file_of(s), r = rank_of(s);
        // (±1,0) then (±1,±1) “outwards”
        struct Step
        {
            int df, dr, of, orr;
        };
        // four orthogonal “first steps”
        const Step steps[4] = {{+1, 0, +1, +1}, {+1, 0, +1, -1}, {-1, 0, -1, +1}, {-1, 0, -1, -1}};
        for (int i = 0; i < 4; i += 2)
        { // handle +1,0 and -1,0 pairs; we’ll add vertical pairs next
            // placeholder: we’ll implement all 4 orthogonals properly later
        }
        // For clarity we’ll fully implement in next step.
        return res;
    }

} // namespace tiny
