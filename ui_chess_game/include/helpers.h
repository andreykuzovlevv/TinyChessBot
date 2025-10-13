#include "core/position.h"
#include "core/types.h"

using namespace tiny;

// ---------- Helpers to map types ----------
static inline bool is_own_piece(const Position& pos, Square s) {
    Piece pc = pos.piece_on(s);
    if (pc == NO_PIECE) return false;
    Color c = color_of(pc);
    return c == pos.side_to_move();
}

// Texture key: piece id (white/black x types).
enum TexKey : int {
    T_W_P = 0,
    T_W_U,
    T_W_F,
    T_W_W,
    T_W_K,
    T_B_P,
    T_B_U,
    T_B_F,
    T_B_W,
    T_B_K,
    TEX_NB
};

static inline TexKey texkey_for_piece(Piece p) {
    switch (p) {
        case W_PAWN:
            return T_W_P;
        case W_HORSE:
            return T_W_U;
        case W_FERZ:
            return T_W_F;
        case W_WAZIR:
            return T_W_W;
        case W_KING:
            return T_W_K;
        case B_PAWN:
            return T_B_P;
        case B_HORSE:
            return T_B_U;
        case B_FERZ:
            return T_B_F;
        case B_WAZIR:
            return T_B_W;
        case B_KING:
            return T_B_K;
        default:
            return T_W_P;
    }
}

static inline TexKey texkey_for_piece(Color c, PieceType pt) {
    if (c == WHITE) {
        switch (pt) {
            case PAWN:
                return T_W_P;
            case HORSE:
                return T_W_U;
            case FERZ:
                return T_W_F;
            case WAZIR:
                return T_W_W;
            case KING:
                return T_W_K;
            default:
                return T_W_P;
        }
    } else {
        switch (pt) {
            case PAWN:
                return T_B_P;
            case HORSE:
                return T_B_U;
            case FERZ:
                return T_B_F;
            case WAZIR:
                return T_B_W;
            case KING:
                return T_B_K;
            default:
                return T_B_P;
        }
    }
}