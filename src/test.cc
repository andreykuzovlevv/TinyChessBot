
#include <stdio.h>

#include <iostream>
#include <string>

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

    Bitboards::init();
    Position::init();

    Position    pos;
    StateInfo   si;
    std::string fen = "k3/4/4/K3 w 1";

    pos.set(fen, &si);
    std::cout << "hello main\n";

    std::cout << pos;
}