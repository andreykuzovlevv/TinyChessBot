
#include <stdio.h>

#include <iostream>
#include <string>

#include "core/movegen.h"
#include "core/position.h"
#include "core/types.h"

using namespace tiny;

namespace {

static constexpr Piece Pieces[] = {W_PAWN, W_HORSE, W_FERZ, W_WAZIR, W_KING,
                                   B_PAWN, B_HORSE, B_FERZ, B_WAZIR, B_KING};
}  // namespace

int main() {
    for (Piece pc : Pieces) {
        Color     c  = color_of(pc);
        PieceType pt = type_of(pc);
        printf("Piece %d: color %d, type %d\n", pc, c, pt);
    }

    // Debug square layout and file/rank functions
    printf("\n=== Square Layout Debug ===\n");
    for (Square s = SQ_A1; s <= SQ_D4; ++s) {
        printf("Square %d (%c%c): file=%d, rank=%d\n", s, 'a' + file_of(s), '1' + rank_of(s),
               file_of(s), rank_of(s));
    }

    // Test king attacks from specific squares
    printf("\n=== King Attack Debug ===\n");

    Bitboards::init();
    Position::init();

    // Test king attacks from square 0 (A1) and square 12 (A4)
    Bitboard attacks_a1 = PseudoAttacks[KING][SQ_A1];
    printf("King attacks from A1 (square 0):\n%s\n", Bitboards::pretty(attacks_a1).c_str());

    Bitboard attacks_a4 = PseudoAttacks[KING][SQ_A4];
    printf("King attacks from A4 (square 12):\n%s\n", Bitboards::pretty(attacks_a4).c_str());

    Position    pos;
    StateInfo   si;
    std::string fen = "fuwk/3p/P3/KWUF w 1";

    pos.set(fen, &si);

    printf("%d\n", MoveList<LEGAL>(pos).size());

    std::cout << pos;
}