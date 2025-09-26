// Zobrist hashing keys scaffold
#pragma once

#include <cstdint>
#include <array>
#include "types.h"

namespace tiny {

struct Zobrist {
	// piece-square keys: [color][pieceType][square]
	std::array<std::array<std::array<std::uint64_t, 16>, PIECE_TYPE_NB>, 2> psq{};
	// side to move
	std::uint64_t side = 0;
	// reserves: simplistic per (color,pieceType,countDelta) buckets could be added later
};

// Very simple xorshift64* RNG for deterministic keys
inline std::uint64_t rng64(std::uint64_t &s) {
	s ^= s >> 12; s ^= s << 25; s ^= s >> 27; return s * 2685821657736338717ULL;
}

inline Zobrist init_zobrist(std::uint64_t seed = 0x9E3779B97F4A7C15ULL) {
	Zobrist z{};
	std::uint64_t s = seed ? seed : 0xA0761D6478BD642FULL;
	for (int c = 0; c < 2; ++c)
		for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
			for (int sq = 0; sq < 16; ++sq)
				z.psq[c][pt][sq] = rng64(s);
	z.side = rng64(s);
	return z;
}

} // namespace tiny


