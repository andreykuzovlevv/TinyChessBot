#include "solve/retro.h"

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/move.h"
#include "core/position.h"

namespace tiny::retro {

// Internal node status
enum : uint8_t { UNKNOWN = 0, WIN = 1, LOSS = 2, DRAW = 3 };

struct Node {
    uint8_t  status = UNKNOWN;
    uint16_t dtm    = 0;
    Move     best   = Move(0);
    uint16_t outdeg = 0;
    uint16_t remaining = 0; // for loss detection
};

struct Parents {
    std::vector<uint32_t> ids;
};

std::vector<TBRecord> build_wdl_dtm(const Position& start) {
    using Key = uint64_t;

    // --- Storage ---
    std::vector<Node> nodes;
    std::vector<Parents> parents;
    std::vector<Key> keys;
    keys.reserve(1 << 20);

    std::unordered_map<Key, uint32_t> index;
    index.reserve(1 << 20);

    // Work queues
    std::deque<uint32_t> q;

    // Helper to create node for unseen key
    auto add_node = [&](Key k) -> uint32_t {
        auto it = index.find(k);
        if (it != index.end()) return it->second;
        uint32_t id = static_cast<uint32_t>(nodes.size());
        index.emplace(k, id);
        nodes.emplace_back();
        parents.emplace_back();
        keys.push_back(k);
        return id;
    };

    // ------------- Phase A: forward reachability graph -------------
    // BFS from start; along the way, capture outdegrees and reverse edges.
    std::deque<uint32_t> frontier;

    Position root = start;
    uint32_t rootId = add_node(root.key());
    frontier.push_back(rootId);

    // Keep a parallel vector of materialized Positions for frontier expansion.
    // We do not store Positions for every node; we regenerate via do/undo.
    // For BFS we carry only the root Position and replay moves incrementally.
    // Expansion is done by reconstructing from a stored Position snapshot,
    // but to keep this module abstract/simple, we expand from `root` and
    // maintain a stack during DFS-like local expansion.

    // To avoid storing every Position snapshot, do a graph build with
    // on-the-fly expansion using a work stack:
    std::vector<std::pair<uint32_t, Position>> expand;
    expand.reserve(1024);
    expand.emplace_back(rootId, root);

    while (!expand.empty()) {
        auto [pid, pos] = expand.back();
        expand.pop_back();

        // Generate legal moves/drops from pos
        MoveList<LEGAL> ml(pos);
        nodes[pid].outdeg = static_cast<uint16_t>(ml.size());
        nodes[pid].remaining = nodes[pid].outdeg;

        if (ml.size() == 0) {
            // Terminal (mate or stalemate-as-win); status assigned later.
            continue;
        }

        StateInfo st; // per-Position undo info
        for (Move m : ml) {
            pos.do_move(m, st);
            Key ck = pos.key();

            uint32_t cid = add_node(ck);
            parents[cid].ids.push_back(pid);

            // Push child for further expansion only if we just discovered it.
            if (nodes.size() == index.size() && cid == static_cast<uint32_t>(nodes.size() - 1)) {
                // Newly added node => we need to expand it at some point.
                expand.emplace_back(cid, pos);
            }
            pos.undo_move(m);
        }
    }

    // ------------- Phase B: label terminals and enqueue -------------
    // We need to revisit each node to check "no legal moves" and "in check".
    // Do a pass by reconstructing positions via a second crawl:
    std::vector<uint8_t> initialStatus(nodes.size(), UNKNOWN);

    // Minimal reconstruction: walk all nodes by a hash->Position map would
    // require a transposition table of Positions. To stay abstract here,
    // we re-expand from root and mark terminals as we see them.
    // (In a full implementation, you'd maintain a compact TT or cache.)

    // Markers to avoid quadratic work:
    std::vector<uint8_t> seen(nodes.size(), 0);
    std::deque<std::pair<uint32_t, Position>> work;
    work.emplace_back(rootId, root);
    seen[rootId] = 1;

    while (!work.empty()) {
        auto [pid, pos] = work.front();
        work.pop_front();

        MoveList<LEGAL> ml(pos);
        if (ml.size() == 0) {
            // terminal
            if (pos.in_check()) {
                nodes[pid].status = LOSS; // side-to-move is checkmated -> loss
                nodes[pid].dtm = 0;
            } else {
                nodes[pid].status = WIN;  // stalemate-as-win per rules
                nodes[pid].dtm = 0;
            }
            q.push_back(pid);
        }

        StateInfo st;
        for (Move m : ml) {
            pos.do_move(m, st);
            auto it = index.find(pos.key());
            uint32_t cid = it->second;
            if (!seen[cid]) {
                seen[cid] = 1;
                work.emplace_back(cid, pos);
            }
            pos.undo_move(m);
        }
    }

    // ------------- Phase C: retrograde propagation -------------
    while (!q.empty()) {
        uint32_t v = q.front();
        q.pop_front();

        uint8_t vstat = nodes[v].status;
        uint16_t vdtm  = nodes[v].dtm;

        for (uint32_t p : parents[v].ids) {
            if (nodes[p].status != UNKNOWN)
                continue;

            if (vstat == LOSS) {
                // Parent can move to a losing child -> parent is WIN
                nodes[p].status = WIN;
                nodes[p].dtm    = static_cast<uint16_t>(vdtm + 1);
                // (best move to v is filled later in a refinement pass)
                q.push_back(p);
            } else if (vstat == WIN) {
                // One more child proven WIN for the opponent
                if (nodes[p].remaining > 0) nodes[p].remaining--;
                if (nodes[p].remaining == 0) {
                    nodes[p].status = LOSS;
                    nodes[p].dtm    = static_cast<uint16_t>(vdtm + 1);
                    q.push_back(p);
                }
            }
        }
    }

    // Anything still UNKNOWN is a draw (cycles / repetition region).
    for (auto& n : nodes) {
        if (n.status == UNKNOWN) { n.status = DRAW; n.dtm = 0; }
    }

    // Optional: one more pass to assign a concrete best move for WIN/LOSS
    // (shortest mate for WIN, longest resistance for LOSS).
    // This requires a light re-walk from each node to find the child with
    // the matching (wdl, dtm). Omitted here for brevity.

    // ------------- Build TB records -------------
    std::vector<TBRecord> out;
    out.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        WDL wdl = (nodes[i].status == WIN) ? WDL::Win
                 : (nodes[i].status == LOSS) ? WDL::Loss
                 : WDL::Draw;
        out.push_back(TBRecord{
            keys[i], wdl, nodes[i].dtm, nodes[i].best
        });
    }
    return out;
}

} // namespace tiny::retro
