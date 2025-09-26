* **Solve once → save**:

  ```
  tinyhouse solve --out tinyhouse.tb
  ```

  This explores the full Tinyhouse game tree from start position, proves the game-theoretic result, and writes a table file with solved outcomes.

* **Load and use** (instant answers, no re-solving):

  ```
  tinyhouse play --tb tinyhouse.tb
  # then at the prompt:
  startpos
  bestmove
  ```

---

# 1) Project layout (files to create)

```
/src
  main.cc

  cli/
    cli.h
    cli.cc                # parse args; dispatch to run_solve / run_play

  core/
    types.h               # Color, PieceType, Piece, Square, Outcome, etc.
    constants.h
    notation.h            # move/position string formats (UCI-like)
    bitboard.h            # 16-bit or 32-bit helpers (optional)
    zobrist.h
    zobrist.cc
    position.h
    position.cc           # board + reserves + side-to-move
    move.h
    move.cc               # construction/parsing helpers (drops, promotions)
    movegen.h
    movegen.cc            # legal move generation (incl. blockable 'U')
    makemove.h
    makemove.cc           # do/undo with incremental state + hash
    rules.h
    rules.cc              # check, checkmate/stalemate detection

  solve/
    ...

  ui/
    # idk what the fuck is this - user iterface?

/tests
  ...
```

---