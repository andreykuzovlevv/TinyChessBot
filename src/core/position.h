// Position API scaffold (Stockfish-shaped) for Tinyhouse
#pragma once

#include <vector>
#include <memory>
#include "types.h"
#include "move.h"
#include "zobrist.h"

namespace tiny {

struct StateInfo {
	Key key = 0;
	Move lastMove = 0;
	int captured = -1;
};

class Position {
public:
	Position();

	Color side_to_move() const { return stm; }
	Key key() const { return currentKey; }
	bool in_check() const; // TODO: implement with real attacks

	void do_move(Move m, StateInfo &st);
	void undo_move(Move m);

	bool has_legal_move() const; // TODO

	template<bool LEGAL> class MoveList {
	public:
		explicit MoveList(const Position &p) { (void)p; /* TODO: generate */ }
		auto begin() const { return moves.begin(); }
		auto end() const { return moves.end(); }
		std::size_t size() const { return moves.size(); }
	private:
		std::vector<Move> moves;
	};

private:
	Color stm = WHITE;
	Key currentKey = 0;
	Zobrist zkeys;
};

} // namespace tiny


