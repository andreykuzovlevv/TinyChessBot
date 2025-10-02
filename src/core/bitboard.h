

#ifndef BITBOARD_H_INCLUDED
#define BITBOARD_H_INCLUDED

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "types.h"

namespace tiny {

namespace Bitboards {

void        init();
std::string pretty(Bitboard b);

}  // namespace Bitboards

constexpr Bitboard FileABB = 0x1111u;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;

constexpr Bitboard Rank1BB = 0x000Fu;
constexpr Bitboard Rank2BB = Rank1BB << (4 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (4 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (4 * 3);

extern uint8_t PopCnt16[1 << 16];
extern uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

extern Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard HorseAttacks[4][SQUARE_NB];
extern Square   HorseLegSquare[4][SQUARE_NB];

constexpr Bitboard square_bb(Square s) {
    assert(is_ok(s));
    return (1u << s);
}

// Overloads of bitwise operators between a Bitboard and a Square for testing
// whether a given bit is set in a bitboard, and for setting and clearing bits.

constexpr Bitboard  operator&(Bitboard b, Square s) { return b & square_bb(s); }
constexpr Bitboard  operator|(Bitboard b, Square s) { return b | square_bb(s); }
constexpr Bitboard  operator^(Bitboard b, Square s) { return b ^ square_bb(s); }
constexpr Bitboard& operator|=(Bitboard& b, Square s) { return b |= square_bb(s); }
constexpr Bitboard& operator^=(Bitboard& b, Square s) { return b ^= square_bb(s); }

constexpr Bitboard operator&(Square s, Bitboard b) { return b & s; }
constexpr Bitboard operator|(Square s, Bitboard b) { return b | s; }
constexpr Bitboard operator^(Square s, Bitboard b) { return b ^ s; }

constexpr Bitboard operator|(Square s1, Square s2) { return square_bb(s1) | s2; }

// Moves a bitboard one or two steps as specified by the direction D
template <Direction D>
constexpr Bitboard shift(Bitboard b) {
    return D == NORTH           ? b << 4
           : D == SOUTH         ? b >> 4
           : D == NORTH + NORTH ? b << 8
           : D == SOUTH + SOUTH ? b >> 8
           : D == EAST          ? (b & ~FileDBB) << 1
           : D == WEST          ? (b & ~FileABB) >> 1
           : D == NORTH_EAST    ? (b & ~FileDBB) << 5
           : D == NORTH_WEST    ? (b & ~FileABB) << 3
           : D == SOUTH_EAST    ? (b & ~FileDBB) >> 3
           : D == SOUTH_WEST    ? (b & ~FileABB) >> 5
                                : 0;
}

// Returns the squares attacked by pawns of the given color
// from the squares in the given bitboard.
template <Color C>
constexpr Bitboard pawn_attacks_bb(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
                      : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}

// distance() functions return the distance between x and y, defined as the
// number of steps for a king in x to reach y.

template <typename T1 = Square>
inline int distance(Square x, Square y);

template <>
inline int distance<File>(Square x, Square y) {
    return std::abs(file_of(x) - file_of(y));
}

template <>
inline int distance<Rank>(Square x, Square y) {
    return std::abs(rank_of(x) - rank_of(y));
}

template <>
inline int distance<Square>(Square x, Square y) {
    return SquareDistance[x][y];
}

// Returns the pseudo attacks of the given piece type
// assuming an empty board.
template <PieceType Pt>
inline Bitboard attacks_bb(Square s, Color c = COLOR_NB) {
    assert((Pt != PAWN || c < COLOR_NB) && (is_ok(s)));
    return Pt == PAWN ? PseudoAttacks[c][s] : PseudoAttacks[Pt][s];
}

// Returns the attacks by the given piece
// assuming the board is occupied according to the passed Bitboard.
// Sliding piece attacks do not continue passed an occupied square.
template <PieceType Pt>
inline Bitboard attacks_bb(Square s, Bitboard occupied) {
    assert((Pt != PAWN) && (is_ok(s)));

    switch (Pt) {
        case HORSE: {
            Bitboard attacks = 0;
            // Check each direction only if the leg square is valid
            for (int dir = DIR_N; dir < DIR_NB; ++dir) {
                Square leg = HorseLegSquare[dir][s];
                if (is_ok(leg) && !(occupied & square_bb(leg))) {
                    attacks |= HorseAttacks[dir][s];
                }
            }
            return attacks;
        }

        default: {
            printf("checking pseudo attacks for piece %d at square %d\n", Pt, s);
            return PseudoAttacks[Pt][s];
        }
    }
}

// Returns the attacks by the given piece
// assuming the board is occupied according to the passed Bitboard.
// Sliding piece attacks do not continue passed an occupied square.
inline Bitboard attacks_bb(PieceType pt, Square s, Bitboard occupied) {
    assert((pt != PAWN) && (is_ok(s)));

    switch (pt) {
        case HORSE:
            return attacks_bb<HORSE>(s, occupied);
        default:
            return PseudoAttacks[pt][s];
    }
}

// Counts the number of non-zero bits in a bitboard.
inline int popcount(Bitboard b) {
#ifndef USE_POPCNT

    return PopCnt16[b];

#elif defined(_MSC_VER)

    return int(_mm_popcnt_u64(b));

#else  // Assumed gcc or compatible compiler

    return __builtin_popcountll(b);

#endif
}

// Returns the least significant bit in a non-zero bitboard.
inline Square lsb(Bitboard b) {
    assert(b && "lsb() on empty bitboard");

#if defined(__GNUC__)  // GCC, Clang, ICX

    return Square(__builtin_ctzll(b));

#elif defined(_MSC_VER)
#ifdef _WIN64  // MSVC, WIN64

    unsigned long idx;
    _BitScanForward64(&idx, b);
    return Square(idx);

#else  // MSVC, WIN32
    unsigned long idx;

    if (b & 0xffffffff) {
        _BitScanForward(&idx, int32_t(b));
        return Square(idx);
    } else {
        _BitScanForward(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
#endif
#else  // Compiler is neither GCC nor MSVC compatible
#error "Compiler not supported."
#endif
}

// Returns the most significant bit in a non-zero bitboard.
inline Square msb(Bitboard b) {
    assert(b);

#if defined(__GNUC__)  // GCC, Clang, ICX

    return Square(63 ^ __builtin_clzll(b));

#elif defined(_MSC_VER)
#ifdef _WIN64  // MSVC, WIN64

    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return Square(idx);

#else  // MSVC, WIN32

    unsigned long idx;

    if (b >> 32) {
        _BitScanReverse(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    } else {
        _BitScanReverse(&idx, int32_t(b));
        return Square(idx);
    }
#endif
#else  // Compiler is neither GCC nor MSVC compatible
#error "Compiler not supported."
#endif
}

// Returns the bitboard of the least significant
// square of a non-zero bitboard. It is equivalent to square_bb(lsb(bb)).
inline Bitboard least_significant_square_bb(Bitboard b) {
    assert(b);
    return b & -b;
}

// Finds and clears the least significant bit in a non-zero bitboard.
inline Square pop_lsb(Bitboard& b) {
    assert(b);
    const Square s = lsb(b);
    b &= b - 1;
    return s;
}

}  // namespace tiny

#endif  // #ifndef BITBOARD_H_INCLUDED
