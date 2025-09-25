#pragma once
#include <cstdint>
#include <array>

namespace tiny
{

    using Bitboard = uint16_t; // 16 squares
    using Key = uint64_t;

    enum Color : uint8_t
    {
        WHITE = 0,
        BLACK = 1,
        COLOR_NB = 2
    };
    inline Color operator~(Color c) { return Color(c ^ 1); }

    enum PieceType : uint8_t
    {
        PT_NONE,
        PT_K,
        PT_W,
        PT_U,
        PT_F,
        PT_P,
        PT_NB
    };
    enum Piece : uint8_t
    {
        NO_PIECE = 0,
        W_K,
        W_W,
        W_U,
        W_F,
        W_P,
        B_K,
        B_W,
        B_U,
        B_F,
        B_P
    };

    enum Square : uint8_t
    {
        SQ_A1 = 0,
        SQ_B1,
        SQ_C1,
        SQ_D1,
        SQ_A2,
        SQ_B2,
        SQ_C2,
        SQ_D2,
        SQ_A3,
        SQ_B3,
        SQ_C3,
        SQ_D3,
        SQ_A4,
        SQ_B4,
        SQ_C4,
        SQ_D4,
        SQ_NONE = 16
    };

    inline int file_of(Square s) { return s & 3; }  // 0..3
    inline int rank_of(Square s) { return s >> 2; } // 0..3
    inline bool on_board(int f, int r) { return (unsigned)f < 4 && (unsigned)r < 4; }
    inline Square make_sq(int f, int r) { return on_board(f, r) ? Square((r << 2) | f) : SQ_NONE; }

    inline Bitboard sq_bb(Square s) { return (s == SQ_NONE) ? 0 : Bitboard(1u) << s; }

} // namespace tiny
