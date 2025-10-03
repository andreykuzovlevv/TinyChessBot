#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <cassert>
#include <cstdint>

namespace tiny {

using Bitboard = uint16_t;  // 16-bit board
using Key      = uint64_t;

enum Color : uint8_t { WHITE, BLACK, COLOR_NB };

// Piece types: pack into 8 slots (index 0 reserved)
enum PieceType : std::int8_t {
    NO_PIECE_TYPE = 0,
    PAWN          = 1,  // P
    HORSE         = 2,  // U (xiangqi horse)
    FERZ          = 3,  // F
    WAZIR         = 4,  // W
    KING          = 5,  // K

    ALL_PIECES    = 0,
    PIECE_TYPE_NB = 8
};

// Pieces: color*8 + type; keep 16 total to preserve XOR-8 tricks
enum Piece : std::int8_t {
    NO_PIECE = 0,

    // White  (indices 1..7)
    W_PAWN  = PAWN,
    W_HORSE = HORSE,
    W_FERZ  = FERZ,
    W_WAZIR = WAZIR,
    W_KING  = KING,

    // Black  (add 8)
    B_PAWN  = PAWN + 8,
    B_HORSE = HORSE + 8,
    B_FERZ  = FERZ + 8,
    B_WAZIR = WAZIR + 8,
    B_KING  = KING + 8,

    PIECE_NB = 16
};

using Value = int;

constexpr Value PawnValue  = 100;
constexpr Value HorseValue = 200;
constexpr Value FerzValue  = 200;
constexpr Value WazirValue = 300;

inline constexpr Value type_value(PieceType pt) {
    switch (pt) {
        case PAWN:
            return PawnValue;
        case HORSE:
            return HorseValue;
        case FERZ:
            return FerzValue;
        case WAZIR:
            return WazirValue;
        default:
            return 0;  // KING, NO_PIECE_TYPE
    }
}

inline constexpr Value piece_value(Piece p) {
    switch (p) {
        case W_PAWN:
            return +PawnValue;
        case W_HORSE:
            return +HorseValue;
        case W_FERZ:
            return +FerzValue;
        case W_WAZIR:
            return +WazirValue;
        case B_PAWN:
            return -PawnValue;
        case B_HORSE:
            return -HorseValue;
        case B_FERZ:
            return -FerzValue;
        case B_WAZIR:
            return -WazirValue;
        default:
            return 0;  // kings and empty
    }
}

constexpr Value START_MATERIAL = PawnValue + HorseValue + FerzValue + WazirValue;
constexpr Value EVAL_MAX       = (HorseValue + FerzValue + WazirValue + WazirValue) * 2;

constexpr Value VALUE_MATE     = 1200;
constexpr Value VALUE_ZERO     = 0;
constexpr Value VALUE_DRAW     = 0;
constexpr Value VALUE_NONE     = 1202;
constexpr Value VALUE_INFINITE = 1201;

enum Square : int8_t {
    SQ_A1,
    SQ_B1,
    SQ_C1,
    SQ_D1,  // 0..3
    SQ_A2,
    SQ_B2,
    SQ_C2,
    SQ_D2,  // 4..7
    SQ_A3,
    SQ_B3,
    SQ_C3,
    SQ_D3,  // 8..11
    SQ_A4,
    SQ_B4,
    SQ_C4,
    SQ_D4,  // 12..15
    SQ_NONE,

    SQUARE_ZERO = 0,
    SQUARE_NB   = 16
};

enum DirectionIndex : uint8_t { DIR_N = 0, DIR_E = 1, DIR_S = 2, DIR_W = 3, DIR_NB = 4 };

enum Direction : int8_t {
    NORTH = 4,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

enum File : int8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_NB };
enum Rank : int8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_NB };

#define ENABLE_INCR_OPERATORS_ON(T)                             \
    constexpr T& operator++(T& d) { return d = T(int(d) + 1); } \
    constexpr T& operator--(T& d) { return d = T(int(d) - 1); }

ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)

#undef ENABLE_INCR_OPERATORS_ON

constexpr Direction operator+(Direction d1, Direction d2) { return Direction(int(d1) + int(d2)); }
constexpr Direction operator*(int i, Direction d) { return Direction(i * int(d)); }

// Additional operators to add a Direction to a Square
constexpr Square  operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square  operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }
constexpr Square& operator-=(Square& s, Direction d) { return s = s - d; }

// Toggle color
constexpr Color operator~(Color c) { return Color(c ^ BLACK); }

constexpr Square make_square(File f, Rank r) { return Square((r << 2) + f); }

constexpr bool is_ok(Square s) { return s >= SQ_A1 && s <= SQ_D4; }

constexpr Piece operator~(Piece pc) { return Piece(pc ^ 8); }

constexpr Piece make_piece(Color c, PieceType pt) { return Piece((c << 3) + pt); }

constexpr PieceType type_of(Piece pc) { return PieceType(pc & 7); }

constexpr Color color_of(Piece pc) {
    assert(pc != NO_PIECE);
    return Color(pc >> 3);
}

constexpr File file_of(Square s) { return File(s & 3); }

constexpr Rank rank_of(Square s) { return Rank(s >> 2); }

constexpr Rank relative_rank(Color c, Rank r) { return Rank(r ^ (c * 7)); }

constexpr Rank relative_rank(Color c, Square s) { return relative_rank(c, rank_of(s)); }

struct Pockets {
    uint8_t p[COLOR_NB][PIECE_TYPE_NB];  // Only {PAWN,WAZIR,HORSE,FERZ} used; KING always 0
};

enum MoveType { NORMAL, PROMOTION = 1 << 14, DROP = 2 << 14 };

// Based on a congruential pseudo-random number generator
constexpr Key make_key(uint64_t seed) {
    return seed * 6364136223846793005ULL + 1442695040888963407ULL;
}

// -------------- Move encoding for 4×4 Tinyhouse -----------------
//
// 16-bit move packing:
//  bits 0-3 : to   (0..15)
//  bits 4-7 : from (0..15) [ignored for DROP]
//  bits 8-9 : AUX  (promo or drop payload; see below)
//  bits 10-13 : reserved (0)
//  bits 14-15 : type (0=NORMAL, 1=PROMOTION, 2=DROP)
//
// AUX meaning:
//   PROMOTION: 0=WAZIR, 1=FERZ, 2=HORSE
//   DROP     : 0=PAWN, 1=HORSE, 2=FERZ, 3=WAZIR
//
// No en passant, no castling in this variant.

class Move {
   public:
    Move() = default;
    constexpr explicit Move(std::uint16_t d) : data(d) {}

    // Normal move ctor (no promo)
    constexpr Move(Square from, Square to) : data((std::uint16_t(from) << 4) | std::uint16_t(to)) {}

    // --- Factories ---

    static constexpr Move make_normal(Square from, Square to) {
        return Move((std::uint16_t(from) << 4) | std::uint16_t(to));
    }

    // Promotions: pt must be one of {WAZIR, FERZ, HORSE}
    static constexpr Move make_promotion(Square from, Square to, PieceType pt) {
        return Move((std::uint16_t(1) << 14)     // type = PROMOTION
                    | (aux_from_promo(pt) << 8)  // AUX
                    | (std::uint16_t(from) << 4) | std::uint16_t(to));
    }

    // Drops: pt must be one of {PAWN, WAZIR, FERZ, HORSE}
    // 'from' is ignored by the engine for DROP; we store from=to to keep it compact.
    static constexpr Move make_drop(PieceType pt, Square to) {
        return Move((std::uint16_t(2) << 14)    // type = DROP
                    | (aux_from_drop(pt) << 8)  // AUX
                    | (std::uint16_t(to) << 4)  // from := to
                    | std::uint16_t(to));
    }

    // --- Accessors ---

    constexpr Square from_sq() const {
        assert(is_ok());
        return Square((data >> 4) & 0x0F);
    }

    constexpr Square to_sq() const {
        assert(is_ok());
        return Square(data & 0x0F);
    }

    // Low 12 bits: from/to/AUX nibble bundle (kept for hashing/move ordering uses)
    constexpr int from_to() const { return data & 0x0FFF; }

    constexpr MoveType type_of() const {
        return MoveType((data >> 14) & 0x3);  // 0..2 used
    }

    // Valid only if type_of()==PROMOTION; else returns NO_PIECE_TYPE.
    constexpr PieceType promotion_type() const {
        if (type_of() != PROMOTION) return NO_PIECE_TYPE;
        return promo_from_aux((data >> 8) & 0x3);
    }

    // Valid only if type_of()==DROP; else returns NO_PIECE_TYPE.
    constexpr PieceType drop_piece_type() const {
        if (type_of() != DROP) return NO_PIECE_TYPE;
        return drop_from_aux((data >> 8) & 0x3);
    }

    constexpr bool is_ok() const { return none().data != data && null().data != data; }

    // Special sentinels
    static constexpr Move none() { return Move(0); }
    // Keep a harmless from==to sentinel. Any (from==to) is not a legal move.
    static constexpr Move null() {
        return Move((std::uint16_t(SQ_A1) << 4) | std::uint16_t(SQ_A1));
    }

    constexpr bool          operator==(const Move& m) const { return data == m.data; }
    constexpr bool          operator!=(const Move& m) const { return data != m.data; }
    constexpr explicit      operator bool() const { return data != 0; }
    constexpr std::uint16_t raw() const { return data; }

    struct MoveHash {
        std::size_t operator()(const Move& m) const { return make_key(m.data); }
    };

    // Map promotion piece -> AUX code (0..2)
    static constexpr std::uint16_t aux_from_promo(PieceType pt) {
        // 0=WAZIR, 1=FERZ, 2=HORSE
        return pt == WAZIR ? 0u : pt == FERZ ? 1u : /*HORSE*/ 2u;
    }
    static constexpr PieceType promo_from_aux(std::uint16_t aux) {
        return aux == 0 ? WAZIR : aux == 1 ? FERZ : HORSE;
    }

    // Map drop piece -> AUX code (0..3)
    static constexpr std::uint16_t aux_from_drop(PieceType pt) {
        // 0=PAWN, 1=WAZIR, 2=FERZ, 3=HORSE
        return pt == PAWN ? 0u : pt == WAZIR ? 1u : pt == FERZ ? 2u : 3u;  // HORSE
    }
    static constexpr PieceType drop_from_aux(std::uint16_t aux) {
        return aux == 0 ? PAWN : aux == 1 ? WAZIR : aux == 2 ? FERZ : HORSE;
    }

   protected:
    std::uint16_t data{};
};

}  // namespace tiny

#endif  // #ifndef TYPES_H_INCLUDED