#pragma once
#include "types.h"

namespace tiny
{

    enum MoveType : uint8_t
    {
        MT_NORMAL,
        MT_CAPTURE,
        MT_PROMO,
        MT_DROP
    };

    struct Move
    {
        uint16_t from : 5;  // 0..16 (16 means “drop”)
        uint16_t to : 5;    // 0..15
        uint16_t pt : 3;    // moving or dropped piece type
        uint16_t promo : 3; // PT_W/PT_U/PT_F for promotions; PT_NONE otherwise
        uint16_t type : 2;  // MoveType
    };

    inline Move make_move(Square from, Square to, PieceType pt, MoveType mt, PieceType promo = PT_NONE)
    {
        return Move{(uint16_t)from, (uint16_t)to, (uint16_t)pt, (uint16_t)promo, (uint16_t)mt};
    }

    inline bool is_drop(const Move &m) { return m.from == SQ_NONE; }

} // namespace tiny
