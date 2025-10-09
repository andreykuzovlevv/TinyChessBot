
#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#include "core/movegen.h"
#include "core/position.h"
#include "core/types.h"

using namespace tiny;

static inline const char* color_name(Color c) { return c == WHITE ? "White" : "Black"; }

int main() {
    Bitboards::init();
    Position::init();

    Position              pos;
    std::deque<StateInfo> states;   // Container to manage StateInfo objects
    std::vector<Move>     history;  // Played moves history for undo

    std::string fen = "fhwk/3p/P3/KWHF w 1";
    states.emplace_back();  // Create first StateInfo
    pos.set(fen, &states.back());
    printf("hello 0");
    int ply = 0;

    while (true) {
        ply++;
        printf("hello 1");
        std::cout << pos;
        MoveList<LEGAL> moves(pos);

        // Check terminals
        if (moves.size() == 0) {
            if (pos.checkers()) {
                std::cout << "Checkmate. Winner: " << color_name(~pos.side_to_move()) << "\n";
            } else {
                std::cout << "Stalemate. Winner: " << color_name(pos.side_to_move()) << "\n";
            }
            return 0;
        }
        if (pos.is_draw(ply)) {
            std::cout << "Draw by threefold repetition.\n";
            return 0;
        }

        // list moves
        for (int i = 0; i < moves.size(); ++i)
            std::cout << i << ": " << to_string(moves[i]) << "\n";
        std::cout << "Choose move [0.." << (moves.size() - 1)
                  << "] (u undo, r reset, q quit): " << std::flush;

        // read choice
        std::string s;
        if (!std::getline(std::cin, s)) return 0;
        if (!s.empty() && (s[0] == 'q' || s[0] == 'Q')) return 0;

        // undo last move
        if (!s.empty() && (s[0] == 'u' || s[0] == 'U')) {
            if (history.empty()) {
                std::cout << "Nothing to undo.\n";
                continue;
            }
            Move last = history.back();
            pos.undo_move(last);
            history.pop_back();
            states.pop_back();
            continue;
        }

        // reset to start position
        if (!s.empty() && (s[0] == 'r' || s[0] == 'R')) {
            if (history.empty()) {
                std::cout << "Already at start.\n";
                continue;
            }
            while (!history.empty()) {
                pos.undo_move(history.back());
                history.pop_back();
                states.pop_back();
            }
            continue;
        }

        int idx = -1;
        try {
            idx = std::stoi(s);
        } catch (...) {
            idx = -1;
        }
        if (idx < 0 || idx >= moves.size()) {
            std::cout << "Invalid.\n";
            continue;
        }

        // do move (forward-only; no undo)
        states.emplace_back();  // Create new StateInfo in container
        Move m = moves[idx];
        pos.do_move(m, states.back());  // Pass reference to the new StateInfo
        history.push_back(m);
    }
    return 0;
}