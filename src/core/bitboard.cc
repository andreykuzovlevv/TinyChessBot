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

// Check if a diagonal direction is compatible with a leg direction for horse moves
bool is_compatible_direction(Direction legDir, Direction diagDir) {
    // For each leg direction, only certain diagonal directions are valid
    switch (legDir) {
        case NORTH:  return diagDir == NORTH_EAST || diagDir == NORTH_WEST;
        case EAST:   return diagDir == NORTH_EAST || diagDir == SOUTH_EAST;
        case SOUTH:  return diagDir == SOUTH_EAST || diagDir == SOUTH_WEST;
        case WEST:   return diagDir == NORTH_WEST || diagDir == SOUTH_WEST;
        default:     return false;
    }
}
}  // namespace

// Returns an ASCII representation of a bitboard suitable
// to be printed to standard output. Useful for debugging.
std::string Bitboards::pretty(Bitboard b) {
    std::string s = "+---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_4; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_D; ++f) s += b & make_square(f, r) ? "| X " : "|   ";

        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
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

        // Fill Horse Leg Squares and Attacks per each direction
        for (const auto& item : LEG_DIRS) {
            Square leg = Square(int(s1) + item.delta);
            if (is_ok(leg)) {
                HorseLegSquare[item.idx][s1] = leg;
                
                // Generate horse attacks for this direction
                // Horse moves: one step in leg direction, then one step diagonally
                Direction legDir = item.delta;
                for (Direction diagDir : {NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST}) {
                    // Check if the diagonal direction is compatible with leg direction
                    if (is_compatible_direction(legDir, diagDir)) {
                        int step = legDir + diagDir;
                        HorseAttacks[item.idx][s1] |= safe_destination(s1, step);
                    }
                }
            }
        }
    }
}

}  // namespace tiny