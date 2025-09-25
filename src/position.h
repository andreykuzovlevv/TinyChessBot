#pragma once
#include "types.h"
#include "move.h"
#include "zobrist.h"
#include <vector>
#include <array>

namespace tiny
{

    struct Reserves
    {
        uint8_t wazir[COLOR_NB]{0, 0};
        uint8_t horse[COLOR_NB]{0, 0};
        uint8_t ferz[COLOR_NB]{0, 0};
        uint8_t pawn[COLOR_NB]{0, 0};
    };

    class Position
    {
    public:
        Position();
        void set_startpos(); // Tinyhouse initial position

        Color side_to_move() const { return stm_; }
        Key key() const { return key_; }

        Piece piece_on(Square s) const { return board_[s]; }
        bool in_check(Color c) const; // king attacked?

        // Make/unmake (includes drops, promotions, captures to reserve)
        void do_move(Move m, Piece captured_hint = NO_PIECE);
        void undo_move();

        // Repetition detection
        bool is_draw_by_threefold() const;

        // Utility
        Reserves reserves() const { return res_; }

    private:
        std::array<Piece, 16> board_{};
        Reserves res_{};
        Color stm_ = WHITE;
        Key key_ = 0;

        // King squares for speed
        Square king_sq_[COLOR_NB]{SQ_NONE, SQ_NONE};

        // Stack for unmake
        struct State
        {
            Key key;
            Move m;
            Piece captured;
            Square prev_king_sq[COLOR_NB];
            uint8_t hm_clock;
        };
        std::vector<State> stack_;
        uint8_t halfmove_clock_ = 0;
        std::vector<Key> history_; // for 3-fold
    };

} // namespace tiny
