/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2025 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <bitset>
#include <cassert>
#include <deque>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "bitboard.h"
#include "types.h"

namespace tiny {

// StateInfo struct stores information needed to restore a Position object to
// its previous state when we retract a move. Whenever a move is made on the
// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo {
    // Copied when making a move

    // Not copied when making a move (will be recomputed anyhow)
    Key        key;
    Bitboard   checkersBB;
    StateInfo* previous;
    Bitboard   blockersForKing[COLOR_NB];
    Piece      capturedPiece;
    int        repetition;
};

// A list to keep track of the position states along the setup moves (from the
// start position to the position just before the search starts). Needed by
// 'draw by repetition' detection. Use a std::deque because pointers to
// elements are not invalidated upon list resizing.
using StateListPtr = std::unique_ptr<std::deque<StateInfo>>;

class Position {
   public:
    // init
    static void init();  // fills attack tables & Zobrist

    // FEN string input/output
    Position&   set(const std::string& fenStr, StateInfo* si);
    std::string fen() const;

    // Position representation
    Bitboard pieces() const;  // All pieces
    template <typename... PieceTypes>
    Bitboard pieces(PieceTypes... pts) const;
    Bitboard pieces(Color c) const;
    template <typename... PieceTypes>
    Bitboard pieces(Color c, PieceTypes... pts) const;

    inline Piece piece_on(Square s) const {
        assert(is_ok(s));
        return board[s];
    }
    inline bool empty(Square s) const { return board[s] == NO_PIECE; }
    template <PieceType Pt>
    int count(Color c) const;
    template <PieceType Pt>
    int count() const;
    template <PieceType Pt>
    Square square(Color c) const;

    // Checking
    Bitboard checkers() const;
    Bitboard blockers_for_king(Color c) const;

    // Attacks to/from a given square
    Bitboard attackers_to(Square s) const;
    Bitboard attackers_to(Square s, Bitboard occupied) const;
    bool     attackers_to_exist(Square s, Bitboard occupied, Color c) const;
    void     update_slider_blockers(Color c) const;

    // Properties of moves
    bool  legal(Move m) const;
    Piece moved_piece(Move m) const;

    // Doing and undoing moves
    void do_move(Move m, StateInfo& newSt, bool givesCheck);
    void undo_move(Move m);

    // Accessing hash keys
    Key key() const;

    // Other properties of the position
    inline Color side_to_move() const { return sideToMove; }
    inline int   game_ply() const { return gamePly; }
    bool         is_draw(int ply) const;
    bool         is_repetition(int ply) const;
    bool         upcoming_repetition(int ply) const;
    bool         has_repeated() const;

    // Position consistency check, for debugging
    bool pos_is_ok() const;
    void flip();

    void put_piece(Piece pc, Square s);
    void remove_piece(Square s);

   private:
    // Initialization helpers (used while setting up a position)
    void set_state() const;
    void set_check_info() const;

    // Other helpers
    void move_piece(Square from, Square to);

    // Data members
    Piece      board[SQUARE_NB];  // mailbox
    Bitboard   byTypeBB[PIECE_TYPE_NB];
    Bitboard   byColorBB[COLOR_NB];
    int        pieceCount[PIECE_NB];
    Color      sideToMove;
    Pockets    pockets;
    StateInfo* st;
    int        gamePly;
};

std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Key Position::key() const { return st->key; }

inline Piece Position::moved_piece(Move m) const { return piece_on(m.from_sq()); }

inline Bitboard Position::pieces() const { return byTypeBB[ALL_PIECES]; }

template <typename... PieceTypes>
inline Bitboard Position::pieces(PieceTypes... pts) const {
    return (byTypeBB[pts] | ...);
}

inline Bitboard Position::pieces(Color c) const { return byColorBB[c]; }

template <typename... PieceTypes>
inline Bitboard Position::pieces(Color c, PieceTypes... pts) const {
    return pieces(c) & pieces(pts...);
}

template <PieceType Pt>
inline int Position::count(Color c) const {
    return pieceCount[make_piece(c, Pt)];
}

template <PieceType Pt>
inline int Position::count() const {
    return count<Pt>(WHITE) + count<Pt>(BLACK);
}

template <PieceType Pt>
inline Square Position::square(Color c) const {
    assert(count<Pt>(c) == 1 && "square(), not exactly one piece of this type");
    return lsb(pieces(c, Pt));
}

inline Bitboard Position::attackers_to(Square s) const { return attackers_to(s, pieces()); }

inline Bitboard Position::checkers() const { return st->checkersBB; }

inline Bitboard Position::blockers_for_king(Color c) const { return st->blockersForKing[c]; }

inline void Position::put_piece(Piece pc, Square s) {
    board[s] = pc;
    byTypeBB[ALL_PIECES] |= byTypeBB[type_of(pc)] |= s;
    byColorBB[color_of(pc)] |= s;
    pieceCount[pc]++;
    pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
}

inline void Position::remove_piece(Square s) {
    Piece pc = board[s];
    byTypeBB[ALL_PIECES] ^= s;
    byTypeBB[type_of(pc)] ^= s;
    byColorBB[color_of(pc)] ^= s;
    board[s] = NO_PIECE;
    pieceCount[pc]--;
    pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
}

inline void Position::move_piece(Square from, Square to) {
    Piece    pc     = board[from];
    Bitboard fromTo = from | to;
    byTypeBB[ALL_PIECES] ^= fromTo;
    byTypeBB[type_of(pc)] ^= fromTo;
    byColorBB[color_of(pc)] ^= fromTo;
    board[from] = NO_PIECE;
    board[to]   = pc;
}
}  // namespace tiny

#endif  // #ifndef POSITION_H_INCLUDED
