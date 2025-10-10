
#include <stdio.h>

#include <iostream>
#include <string>

#include "core/movegen.h"
#include "core/position.h"
#include "core/types.h"
#include "minmax/minmax.h"

using namespace tiny;

static inline const char* color_name(Color c) { return c == WHITE ? "White" : "Black"; }

int main() {
    Bitboards::init();
    Position::init();

    Position              pos;
    std::deque<StateInfo> states;  // Container to manage StateInfo objects

    // Starting position (adjust FEN here if you want a different start)
    std::string fen = "fhwk/3p/P3/KWHF w 1";
    states.emplace_back();  // Create first StateInfo
    pos.set(fen, &states.back());

    // Select side to play
    Color humanSide = WHITE;
    while (true) {
        std::cout << "Choose your side ('w' for White, 'b' for Black): " << std::flush;
        std::string sideIn;
        if (!std::getline(std::cin, sideIn)) return 0;
        if (!sideIn.empty() && (sideIn[0] == 'q' || sideIn[0] == 'Q')) return 0;
        if (!sideIn.empty() && (sideIn[0] == 'w' || sideIn[0] == 'W')) {
            humanSide = WHITE;
            break;
        }
        if (!sideIn.empty() && (sideIn[0] == 'b' || sideIn[0] == 'B')) {
            humanSide = BLACK;
            break;
        }
        std::cout << "Invalid input. Enter 'w' or 'b'.\n";
    }

    // Configure search depth for AI (Black)
    const int searchDepth = 9;

    while (true) {
        std::cout << pos;

        // Terminal checks before move
        MoveList<LEGAL> rootMoves(pos);
        if (rootMoves.size() == 0) {
            if (pos.checkers()) {
                std::cout << "Checkmate. Winner: " << color_name(~pos.side_to_move()) << "\n";
            } else {
                std::cout << "Stalemate. Winner: " << color_name(pos.side_to_move()) << "\n";
            }
            break;
        }
        if (pos.is_threefold_game()) {
            std::cout << "Draw by threefold repetition.\n";
            break;
        }

        if (pos.side_to_move() == humanSide) {
            // Human (White) input: choose a move by index
            std::cout << "Legal moves:\n";
            for (int i = 0; i < rootMoves.size(); ++i)
                std::cout << i << ": " << to_string(rootMoves[i]) << "\n";
            std::cout << "Enter move index or 'q': " << std::flush;

            std::string in;
            if (!std::getline(std::cin, in)) break;
            if (!in.empty() && (in[0] == 'q' || in[0] == 'Q')) break;

            int chosenIdx = -1;
            try {
                chosenIdx = std::stoi(in);
            } catch (...) {
                std::cout << "Invalid input. Please enter a number between 0 and "
                          << (rootMoves.size() - 1) << " or 'q'.\n";
                continue;
            }

            if (chosenIdx < 0 || chosenIdx >= rootMoves.size()) {
                std::cout << "Index out of range. Please enter 0-" << (rootMoves.size() - 1)
                          << ".\n";
                continue;
            }

            states.emplace_back();
            pos.do_move(rootMoves[chosenIdx], states.back());
            continue;
        } else {
            // AI (Black) move
            SearchResult res = search_best_move(pos, searchDepth);
            if (res.bestMove == MOVE_NONE) {
                // Should be covered by terminal checks, but handle defensively
                std::cout << "No move available.\n";
                break;
            }

            states.emplace_back();
            pos.do_move(res.bestMove, states.back());
            std::cout << "AI plays: " << to_string(res.bestMove) << " (score " << res.score
                      << ")\n";
            continue;
        }
    }

    return 0;
}