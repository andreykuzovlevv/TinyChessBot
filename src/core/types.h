// Tinyhouse core types and enums (Stockfish-shaped)
#pragma once

#include <cstdint>

namespace tiny {

enum Color : std::uint8_t { WHITE = 0, BLACK = 1 };

// 4x4 board, squares 0..15 map to a1..d4
using Square = std::uint8_t; // 0..15
using Key    = std::uint64_t; // Zobrist
using Move   = std::uint32_t; // Packed move (see move.h)

enum PieceType : std::uint8_t { KING = 0, PAWN = 1, FERZ = 2, WAZIR = 3, UNICORN = 4, PIECE_TYPE_NB = 5 };

} // namespace tiny


