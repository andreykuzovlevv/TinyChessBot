
#include <stdio.h>

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
}