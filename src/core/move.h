// Packed Move encoding (inspired by Stockfish / Fairy-Stockfish style)
#pragma once

#include <cstdint>
#include "types.h"

namespace tiny {

// Layout (32 bits):
// [ 5 bits flags ][ 4 bits drop type ][ 6 bits from ][ 6 bits to ][ 11 bits unused ]
// - from: 0..15, or 0x3F (63) to mark a DROP
// - to:   0..15
// - drop type: PieceType when DROP; ignored otherwise
// - flags: bit0=Capture, bit1=Promo, bit2=Drop, bit3..4 reserved

enum MoveFlags : std::uint8_t { MF_CAPTURE = 1u << 0, MF_PROMO = 1u << 1, MF_DROP = 1u << 2 };

inline Move make_move(std::uint8_t from, std::uint8_t to, std::uint8_t flags = 0, std::uint8_t dropType = 0) {
	std::uint32_t m = 0;
	m |= (to & 0x3Fu);
	m |= (static_cast<std::uint32_t>(from & 0x3Fu) << 6);
	m |= (static_cast<std::uint32_t>(dropType & 0x0Fu) << 12);
	m |= (static_cast<std::uint32_t>(flags & 0x1Fu) << 16);
	return static_cast<Move>(m);
}

inline std::uint8_t to_sq(Move m) { return static_cast<std::uint8_t>(m & 0x3Fu); }
inline std::uint8_t from_sq(Move m) { return static_cast<std::uint8_t>((m >> 6) & 0x3Fu); }
inline std::uint8_t move_flags(Move m) { return static_cast<std::uint8_t>((m >> 16) & 0x1Fu); }
inline bool is_drop(Move m) { return (move_flags(m) & MF_DROP) != 0; }
inline bool is_promo(Move m) { return (move_flags(m) & MF_PROMO) != 0; }
inline bool is_capture(Move m) { return (move_flags(m) & MF_CAPTURE) != 0; }
inline std::uint8_t drop_type(Move m) { return static_cast<std::uint8_t>((m >> 12) & 0x0Fu); }

} // namespace tiny


