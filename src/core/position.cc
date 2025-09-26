#include "position.h"

namespace tiny {

Position::Position() {
	zkeys = init_zobrist();
	currentKey = 0; // TODO: set from initial setup
}

bool Position::in_check() const {
	// TODO: implement legal attack detection
	return false;
}

void Position::do_move(Move m, StateInfo &st) {
	st.key = currentKey;
	st.lastMove = m;
	// TODO: update board and currentKey
	stm = (stm == WHITE ? BLACK : WHITE);
	currentKey ^= zkeys.side;
}

void Position::undo_move(Move m) {
	(void)m;
	// TODO: restore board and key from a stored stack if needed
}

bool Position::has_legal_move() const {
	// TODO
	return false;
}

} // namespace tiny


