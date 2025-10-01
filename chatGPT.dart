// Dart 3+
// Run with: dart run minimax_and_tablebase.dart
// Contents:
//  - Core interfaces (Game, Position, Move)
//  - Minimax with alpha–beta returning best move and optional DTM
//  - Retrograde tablebase builder (WDL + DTM) over the reachable game graph
//  - A complete TicTacToe example to demonstrate both algorithms

import 'dart:collection';

/// ---------- Core game interfaces ----------

/// A move is any immutable, hashable token that can be applied to a Position.
/// For chess you'd encode (from, to, promo, flags).
abstract class Move {
  @override
  bool operator ==(Object other);
  @override
  int get hashCode;
}

/// A game position from the perspective of the *side to move*.
/// Must be immutable and hashable.
abstract class Position<M extends Move> {
  /// True if the game is over at this position.
  bool get isTerminal;

  /// Value for a terminal node from the perspective of the side to move:
  /// +1 = current side has a forced win (e.g., opponent just got checkmated),
  ///  0 = draw,
  /// -1 = current side has a forced loss (e.g., side to move is checkmated).
  ///
  /// Must only be consulted if isTerminal == true.
  int terminalScore(); // ∈ {-1, 0, +1}

  /// Generate all *legal* moves for the side to move.
  Iterable<M> legalMoves();

  /// Apply a move and return the successor position (opponent to move).
  Position<M> play(M m);

  /// Hash & equality must uniquely identify the position (including side-to-move
  /// and any rule state like repetition counters, etc.).
  @override
  bool operator ==(Object other);
  @override
  int get hashCode;
}

/// A Game bundles helper info (optional).
abstract class Game<P extends Position<M>, M extends Move> {
  /// An optional heuristic evaluation for non-terminal leaves.
  /// Should be in [-1, +1], where +1 is great for the side to move.
  double evaluate(P p);

  /// For enumerating initial states if you want to build a tablebase
  /// over all *reachable* nodes.
  Iterable<P> initialPositions();
}

/// ---------- Minimax with alpha–beta and DTM reporting ----------

class MinimaxResult<M extends Move> {
  final double score; // exact if terminal reached; heuristic otherwise
  final int? dtm; // in plies to mate if exact, otherwise null
  final M? bestMove;
  MinimaxResult({
    required this.score,
    required this.dtm,
    required this.bestMove,
  });
}

class Minimax<P extends Position<M>, M extends Move> {
  final Game<P, M> game;
  final int maxDepth; // maximum plies to search
  final bool computeDTM; // when a mate is reached, report plies to mate
  final Map<P, _TTEntry<M>> _tt = {};

  Minimax(this.game, {this.maxDepth = 99, this.computeDTM = true});

  MinimaxResult<M> search(P root) {
    var res = _alphabeta(root, depth: maxDepth, alpha: -1e9, beta: 1e9);
    return MinimaxResult<M>(score: res.score, dtm: res.dtm, bestMove: res.best);
  }

  _ScoreBest<M> _alphabeta(
    P pos, {
    required int depth,
    required double alpha,
    required double beta,
  }) {
    if (pos.isTerminal) {
      final v = pos.terminalScore().toDouble(); // -1, 0, +1 exact
      final dtm = computeDTM && v.abs() == 1.0 ? 0 : null;
      return _ScoreBest<M>(score: v, best: null, dtm: dtm);
    }
    if (depth == 0) {
      // Heuristic leaf; no DTM available.
      return _ScoreBest<M>(score: game.evaluate(pos), best: null, dtm: null);
    }

    // Transposition table lookup
    final hit = _tt[pos];
    if (hit != null && hit.depth >= depth) {
      return _ScoreBest<M>(score: hit.score, best: hit.best, dtm: hit.dtm);
    }

    double bestScore = -1e9;
    M? bestMove;
    int? bestDtm;

    // Simple move ordering: try TT move first, then others.
    final moves = pos.legalMoves().toList();
    if (hit?.best != null) {
      moves.sort(
        (a, b) => (a == hit!.best)
            ? -1
            : (b == hit.best)
            ? 1
            : 0,
      );
    }

    double a = alpha;
    for (final m in moves) {
      final child = pos.play(m) as P;
      final childRes = _alphabeta(
        child,
        depth: depth - 1,
        alpha: -beta,
        beta: -a,
      );
      // Because perspective flips at each ply, negate score.
      final score = -childRes.score;
      int? dtm;
      if (computeDTM) {
        // If child is exact mate distance, propagate +1 ply and keep sign convention.
        dtm = childRes.dtm == null ? null : (childRes.dtm! + 1);
      }

      if (score > bestScore ||
          (score == bestScore &&
              _preferShorterMate(dtm, bestDtm, maximize: true))) {
        bestScore = score;
        bestMove = m;
        bestDtm = dtm;
      }
      a = a < bestScore ? bestScore : a;
      if (a >= beta) break; // prune
    }

    // Store in TT
    _tt[pos] = _TTEntry<M>(
      depth: depth,
      score: bestScore,
      best: bestMove,
      dtm: bestDtm,
    );
    return _ScoreBest<M>(score: bestScore, best: bestMove, dtm: bestDtm);
  }

  bool _preferShorterMate(int? a, int? b, {required bool maximize}) {
    // Tie-break when equal scores: prefer faster win, slower loss.
    if (a == null && b == null) return false;
    if (a != null && b == null) return true;
    if (a == null && b != null) return false;
    if (maximize) {
      // For wins (score same), prefer smaller DTM.
      return a! < b!;
    } else {
      // For losses, prefer larger DTM (postpone loss).
      return a! > b!;
    }
  }
}

class _ScoreBest<M extends Move> {
  final double score;
  final M? best;
  final int? dtm;
  _ScoreBest({required this.score, required this.best, required this.dtm});
}

class _TTEntry<M extends Move> {
  final int depth;
  final double score;
  final M? best;
  final int? dtm;
  _TTEntry({
    required this.depth,
    required this.score,
    required this.best,
    required this.dtm,
  });
}

/// ---------- Retrograde tablebase (WDL + DTM) over reachable graph ----------

enum WDL { win, draw, loss, unknown }

class TBValue {
  WDL wdl;
  int? dtm; // in plies to mate for the side to move (only for WIN/LOSS)
  TBValue(this.wdl, [this.dtm]);
}

/// Retrograde builder: given starting positions and move generation,
/// explores the reachable game graph, then runs a multi-queue retrograde
/// to label each node WIN/LOSS/DRAW and produce DTM for mated lines.
/// This is *exact* (perfect play) for the graph it explores.
class Retrograde<P extends Position<M>, M extends Move> {
  final Game<P, M> game;

  Retrograde(this.game);

  /// Build over all nodes reachable from game.initialPositions().
  /// Returns:
  ///  - map of Position -> TBValue (WDL + DTM)
  ///  - map of Position -> best Move (policy) for WIN nodes (null else)
  (Map<P, TBValue>, Map<P, M?>) build() {
    // 1) Forward exploration to collect nodes and edges from all initial roots.
    final nodes = <P>{};
    final succ = <P, List<P>>{};
    final pred = <P, List<P>>{};
    final outdeg = <P, int>{};

    void exploreFrom(P root) {
      final q = Queue<P>()..add(root);
      while (q.isNotEmpty) {
        final u = q.removeFirst();
        if (nodes.contains(u)) continue;
        nodes.add(u);

        if (u.isTerminal) {
          succ[u] = const [];
          outdeg[u] = 0;
          continue;
        }
        final ms = u.legalMoves().toList();
        final vs = <P>[];
        for (final m in ms) {
          final v = u.play(m) as P;
          vs.add(v);
          pred.putIfAbsent(v, () => <P>[]).add(u);
          if (!nodes.contains(v)) q.add(v);
        }
        succ[u] = vs;
        outdeg[u] = vs.length;
      }
    }

    for (final root in game.initialPositions()) {
      exploreFrom(root);
    }

    // 2) Initialize labels.
    final val = <P, TBValue>{};
    final policy = <P, M?>{};
    final q = Queue<P>();

    for (final u in nodes) {
      if (u.isTerminal) {
        final sc = u.terminalScore(); // -1/0/+1
        if (sc == 1) {
          // Side to move has already won (opponent mated): WIN with DTM=0
          val[u] = TBValue(WDL.win, 0);
        } else if (sc == -1) {
          val[u] = TBValue(WDL.loss, 0);
        } else {
          val[u] = TBValue(WDL.draw, 0);
        }
        q.add(u);
      } else {
        val[u] = TBValue(WDL.unknown);
      }
      policy[u] = null;
    }

    // 3) Retrograde propagation.
    // Rules:
    //  - If any successor is LOSS, current becomes WIN (dtm = min child.dtm + 1).
    //  - If all successors are WIN, current becomes LOSS (dtm = 1 + max child.dtm).
    //  - Remaining UNKNOWN after fixpoint are DRAW.
    while (q.isNotEmpty) {
      final u = q.removeFirst();
      final vu = val[u]!;
      final ps = pred[u] ?? <P>[];
      for (final p in ps) {
        if (val[p]!.wdl != WDL.unknown) continue; // already resolved

        if (vu.wdl == WDL.loss) {
          // p can move to a LOSS → p is WIN
          final bestDtm = (vu.dtm ?? 0) + 1;
          val[p] = TBValue(WDL.win, bestDtm);
          // pick some winning successor as policy; we do it later in pass 2
          q.add(p);
        } else if (vu.wdl == WDL.win) {
          // Decrease outdegree; if all succs are WIN then p is LOSS
          outdeg[p] = (outdeg[p]! - 1);
          if (outdeg[p] == 0) {
            // all successors WIN
            // DTM = 1 + max(child.dtm)
            int maxChildDtm = 0;
            for (final v in succ[p]!) {
              final dv = val[v]!.dtm ?? 0;
              if (dv > maxChildDtm) maxChildDtm = dv;
            }
            val[p] = TBValue(WDL.loss, maxChildDtm + 1);
            q.add(p);
          }
        } else if (vu.wdl == WDL.draw) {
          // Draw children don't immediately classify parents.
          // Parents may still become LOSS when all succs are WIN, or WIN if a LOSS child appears later.
          continue;
        }
      }
    }

    // 4) Any remaining UNKNOWN nodes are draws.
    for (final u in nodes) {
      if (val[u]!.wdl == WDL.unknown) {
        val[u] = TBValue(WDL.draw, null);
      }
    }

    // 5) Derive a simple policy: for WIN nodes choose a successor labeled LOSS
    // with the smallest DTM; for DRAW prefer a successor DRAW; for LOSS null.
    for (final u in nodes) {
      final vu = val[u]!;
      if (u.isTerminal) continue;
      final moves = (u.legalMoves().toList());
      if (vu.wdl == WDL.win) {
        int? bestDtm;
        M? bestMove;
        for (final m in moves) {
          final v = (u.play(m) as P);
          if (val[v]!.wdl == WDL.loss) {
            final d = (val[v]!.dtm ?? 0) + 1;
            if (bestDtm == null || d < bestDtm) {
              bestDtm = d;
              bestMove = m;
            }
          }
        }
        policy[u] = bestMove;
      } else if (vu.wdl == WDL.draw) {
        // Pick any move that stays in DRAW if possible.
        M? drawMove;
        for (final m in moves) {
          final v = (u.play(m) as P);
          if (val[v]!.wdl == WDL.draw) {
            drawMove = m;
            break;
          }
        }
        policy[u] = drawMove ?? (moves.isNotEmpty ? moves.first : null);
      } else {
        policy[u] = null; // in LOSS, no winning policy
      }
    }

    return (
      val.map((k, v) => MapEntry(k, v)),
      policy.map((k, v) => MapEntry(k, v)),
    );
  }
}

/// ---------- Example: TicTacToe implementation (complete) ----------

class TMove implements Move {
  final int idx; // 0..8
  TMove(this.idx);
  @override
  bool operator ==(Object other) => other is TMove && other.idx == idx;
  @override
  int get hashCode => idx.hashCode;
  @override
  String toString() => 'TMove($idx)';
}

class TPos implements Position<TMove> {
  // Board: 9 cells, values: 0 empty, 1 X (side to move), 2 O (opponent)
  // We store perspective so "1" is always the side to move.
  final List<int> cells; // length 9
  final int ply; // to detect full board
  TPos(this.cells, this.ply);

  factory TPos.empty() => TPos(List.filled(9, 0), 0);

  @override
  bool get isTerminal => _winner() != 0 || ply == 9;

  int _winner() {
    const wins = [
      [0, 1, 2],
      [3, 4, 5],
      [6, 7, 8],
      [0, 3, 6],
      [1, 4, 7],
      [2, 5, 8],
      [0, 4, 8],
      [2, 4, 6],
    ];
    for (final w in wins) {
      final a = cells[w[0]], b = cells[w[1]], c = cells[w[2]];
      if (a != 0 && a == b && b == c) {
        // If 'a' belongs to side to move, current side already has 3-in-a-row.
        // But because we flip perspective after each play(), 3-in-a-row for
        // "previous mover" means current side is actually *losing*.
        // We define terminalScore from the *current* side perspective:
        // If previous player made a line, current side to move has LOST.
        return a; // just return mark; interpretation happens in terminalScore()
      }
    }
    return 0;
  }

  @override
  int terminalScore() {
    final w = _winner();
    if (w == 0) return 0; // draw by full board or no winner yet
    // If there is a winner, it must be the player who just moved (opponent).
    // So from *current side to move* perspective: LOSS.
    return -1;
  }

  @override
  Iterable<TMove> legalMoves() sync* {
    if (isTerminal) return;
    for (int i = 0; i < 9; i++) {
      if (cells[i] == 0) yield TMove(i);
    }
  }

  @override
  TPos play(TMove m) {
    final next = List<int>.from(cells);
    next[m.idx] = 1; // place current side mark as '1'
    // Flip perspective: swap 1<->2 so that in the child, "1" again means side to move.
    for (int i = 0; i < 9; i++) {
      if (next[i] == 1)
        next[i] = 2;
      else if (next[i] == 2)
        next[i] = 1;
    }
    return TPos(next, ply + 1);
  }

  @override
  bool operator ==(Object other) {
    if (other is! TPos) return false;
    if (ply != other.ply) return false;
    for (int i = 0; i < 9; i++) {
      if (cells[i] != other.cells[i]) return false;
    }
    return true;
  }

  @override
  int get hashCode {
    int h = ply;
    for (int i = 0; i < 9; i++) {
      h = 31 * h + cells[i];
    }
    return h;
  }

  @override
  String toString() {
    String sym(int v) => v == 0 ? '.' : (v == 1 ? 'X' : 'O');
    final rows = [
      '${sym(cells[0])}${sym(cells[1])}${sym(cells[2])}',
      '${sym(cells[3])}${sym(cells[4])}${sym(cells[5])}',
      '${sym(cells[6])}${sym(cells[7])}${sym(cells[8])}',
    ];
    return rows.join('\n');
  }
}

class TGame implements Game<TPos, TMove> {
  @override
  double evaluate(TPos p) {
    // Simple heuristic: center and corners good, edges neutral.
    if (p.isTerminal) return p.terminalScore().toDouble();
    const weights = [3, 2, 3, 2, 4, 2, 3, 2, 3];
    int score = 0;
    for (int i = 0; i < 9; i++) {
      if (p.cells[i] == 1)
        score += weights[i]; // side to move
      else if (p.cells[i] == 2)
        score -= weights[i]; // opponent
    }
    // Scale to [-1,1] roughly.
    return score / 10.0;
  }

  @override
  Iterable<TPos> initialPositions() => [TPos.empty()];
}

/// ---------- Demo main ----------

void main() {
  final game = TGame();
  final root = TPos.empty();

  // 1) Minimax
  final mm = Minimax<TPos, TMove>(game, maxDepth: 9, computeDTM: true);
  final res = mm.search(root);
  print(
    'Minimax from start: score=${res.score.toStringAsFixed(3)} '
    'dtm=${res.dtm} bestMove=${res.bestMove}',
  );

  // 2) Build tablebase over reachable graph from initial position
  final tb = Retrograde<TPos, TMove>(game);
  final (values, policy) = tb.build();
  final v0 = values[root]!;
  print('Tablebase from start: WDL=${v0.wdl} DTM=${v0.dtm}');
  print('Policy move at start: ${policy[root]}');

  // Quick compare: play minimax best move and consult TB value
  final next = root.play(res.bestMove!);
  final vn = values[next]!;
  print('After best move: WDL=${vn.wdl} DTM=${vn.dtm}');
}
