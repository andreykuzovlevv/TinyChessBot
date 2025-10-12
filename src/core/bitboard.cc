#include "bitboard.h"

#include <algorithm>
#include <bitset>
#include <initializer_list>

namespace tiny {
uint8_t PopCnt16[1 << 16];
uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
Bitboard HorseAttacks[DIR_NB][SQUARE_NB];
Square   HorseLegSquare[DIR_NB][SQUARE_NB];

struct LegDir {
    DirectionIndex idx;
    Direction      delta;
};
constexpr LegDir LEG_DIRS[DIR_NB] = {{DIR_N, NORTH}, {DIR_E, EAST}, {DIR_S, SOUTH}, {DIR_W, WEST}};

namespace {

// Returns the bitboard of target square for the given step
// from the given square. If the step is off the board, returns empty bitboard.
Bitboard safe_destination(Square s, int step) {
    Square to = Square(s + step);
    return is_ok(to) && distance(s, to) <= 2 ? square_bb(to) : Bitboard(0);
}

}  // namespace

// Returns an ASCII representation of a bitboard suitable
// to be printed to standard output. Useful for debugging.
std::string Bitboards::pretty(Bitboard b) {
    std::string s = "+---+---+---+---+\n";

    for (Rank r = RANK_4; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_D; ++f) s += b & make_square(f, r) ? "| X " : "|   ";

        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+\n";
    }
    s += "  a   b   c   d\n";

    return s;
}

// Initializes various bitboard tables. It is called at
// startup and relies on global objects to be already zero-initialized.
void Bitboards::init() {
    for (unsigned i = 0; i < (1 << 16); ++i) PopCnt16[i] = uint8_t(std::bitset<16>(i).count());

    for (Square s1 = SQ_A1; s1 <= SQ_D4; ++s1)
        for (Square s2 = SQ_A1; s2 <= SQ_D4; ++s2)
            SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));

    std::memset(HorseAttacks, 0, sizeof(HorseAttacks));
    for (int d = DIR_N; d < DIR_NB; ++d)
        for (int s = 0; s < SQUARE_NB; ++s) HorseLegSquare[d][s] = SQ_NONE;

    for (Square s1 = SQ_A1; s1 <= SQ_D4; ++s1) {
        PseudoAttacks[WHITE][s1] = pawn_attacks_bb<WHITE>(square_bb(s1));
        PseudoAttacks[BLACK][s1] = pawn_attacks_bb<BLACK>(square_bb(s1));

        for (int step : {NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST})
            PseudoAttacks[KING][s1] |= safe_destination(s1, step);

        for (int step : {NORTH, EAST, SOUTH, WEST})
            PseudoAttacks[WAZIR][s1] |= safe_destination(s1, step);

        for (int step : {NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST})
            PseudoAttacks[FERZ][s1] |= safe_destination(s1, step);

        // Pseudo HORSE attacks will be rebuilt from composed two-step moves below
        PseudoAttacks[HORSE][s1] = 0;

        // Fill Horse Leg Squares and Attacks per each direction using masked shifts
        for (const auto& item : LEG_DIRS) {
            Bitboard legBB = 0;
            switch (item.idx) {
                case DIR_N: legBB = shift<NORTH>(square_bb(s1)); break;
                case DIR_E: legBB = shift<EAST>(square_bb(s1)); break;
                case DIR_S: legBB = shift<SOUTH>(square_bb(s1)); break;
                case DIR_W: legBB = shift<WEST>(square_bb(s1)); break;
                default: break;
            }

            if (!legBB) continue;  // offboard

            Square legSq = lsb(legBB);
            HorseLegSquare[item.idx][s1] = legSq;

            // From the leg square, move diagonally away from the leg direction
            Bitboard dests = 0;
            switch (item.idx) {
                case DIR_N:
                    dests |= shift<NORTH_EAST>(legBB);
                    dests |= shift<NORTH_WEST>(legBB);
                    break;
                case DIR_E:
                    dests |= shift<NORTH_EAST>(legBB);
                    dests |= shift<SOUTH_EAST>(legBB);
                    break;
                case DIR_S:
                    dests |= shift<SOUTH_EAST>(legBB);
                    dests |= shift<SOUTH_WEST>(legBB);
                    break;
                case DIR_W:
                    dests |= shift<SOUTH_WEST>(legBB);
                    dests |= shift<NORTH_WEST>(legBB);
                    break;
                default:
                    break;
            }

            HorseAttacks[item.idx][s1] = dests;
            PseudoAttacks[HORSE][s1] |= dests;
        }
    }
}

}  // namespace tiny