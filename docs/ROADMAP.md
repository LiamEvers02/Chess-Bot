# Chess Engine in C++ — Detailed Project Roadmap

> **Goal:** Build a correct, well-structured chess game core in C++ that can be extended into a full chess engine.
>
> **Guiding principle:** Correctness first, performance second. Never optimize until the rules are provably right.

---

## Phase 0 — Environment & Project Setup

Before writing a single line of chess logic, get your tooling right. A stable foundation saves time later.

### Goals
- Working C++ build system
- Basic project structure that can grow without rewriting
- Version control from day one

### Tasks
- [ ] Install a compiler (MSVC via Visual Studio, or GCC/Clang via MSYS2)
- [ ] Choose a build system: **CMake** is the industry standard and the right choice here
- [ ] Set up a Git repository
- [ ] Create a folder structure:
  ```
  Chess/
  ├── src/           # source files
  ├── include/       # header files
  ├── tests/         # unit and perft tests
  ├── docs/          # notes and references
  └── CMakeLists.txt
  ```
- [ ] Configure CMake to build a console executable
- [ ] Add a testing framework: **GoogleTest** or **Catch2** are both solid choices
- [ ] Verify the build pipeline end-to-end with a "Hello, Chess" console output

### Resources
- CMake Tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/
- Catch2: https://github.com/catchorg/Catch2
- GoogleTest: https://github.com/google/googletest

---

## Phase 1 — Core Data Structures

Design the data that represents a chess game. These decisions ripple through everything that follows, so think carefully before coding.

### Goals
- Represent the board and all pieces unambiguously
- Encode the full game state (not just the pieces, but also castling rights, en passant, side to move, etc.)
- Have a `Move` type that can describe any legal move

### Key Concepts to Understand First
- **Squares:** There are 64 squares; the most common numbering is a1=0, b1=1, ..., h8=63 (rank-major order)
- **Pieces:** Encode piece type (Pawn, Knight, Bishop, Rook, Queen, King) and color (White, Black) together or separately
- **Board representations:** Two main families:
  - **Mailbox** (array of squares): easy to understand, good starting point
  - **Bitboards** (64-bit integers, one per piece type per color): fast, preferred by strong engines — consider this from the start if you plan a serious engine
- **Game state beyond the board:** castling rights (4 bits: K/Q-side for each color), en passant target square, side to move, half-move clock (50-move rule), full-move number

### Tasks
- [ ] Read the Chess Programming Wiki article on Board Representation: https://www.chessprogramming.org/Board_Representation
- [ ] Read about Mailbox: https://www.chessprogramming.org/Mailbox
- [ ] Read about Bitboards: https://www.chessprogramming.org/Bitboards
- [ ] Decide on your representation (recommendation: **bitboards** if serious engine; mailbox if learning first)
- [ ] Define enums for `Color`, `PieceType`, `Piece`, `Square`
- [ ] Define a `Position` struct/class holding all game state
- [ ] Define a `Move` struct (at minimum: from square, to square, flags for promotion/castling/en passant)
- [ ] Write a function to pretty-print the board to the console
- [ ] Write a function to parse **FEN strings** (the standard way to describe chess positions)

### Checkpoint
- Load the starting position from a FEN string and print it correctly
- Load any arbitrary FEN and see the right pieces in the right places

### Resources
- FEN explained: https://www.chessprogramming.org/Forsyth-Edwards_Notation
- Starting FEN: `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`

---

## Phase 2 — Move Generation

The most critical and error-prone part of the chess core. A wrong move generator poisons everything downstream.

### Goals
- Generate all legal moves from any position
- Be confident in correctness through systematic testing

### Key Concepts to Understand First
- **Pseudo-legal moves:** moves that follow piece movement rules but may leave the king in check
- **Legal moves:** pseudo-legal moves filtered by "does this leave my king in check?"
- **Attack maps / ray attacks:** knowing which squares are attacked by which side is essential for check detection
- **Special moves:** castling (with its 4 distinct conditions), en passant, promotion

### Tasks
- [ ] Read about move generation: https://www.chessprogramming.org/Move_Generation
- [ ] Implement sliding piece attacks (Bishop, Rook, Queen) — these are the hardest
- [ ] Implement leaping piece attacks (Knight, King)
- [ ] Implement pawn moves (different from attacks: forward push, double push, diagonal capture, en passant)
- [ ] Implement castling rules:
  - King is not in check before, during, or after
  - Squares between king and rook are empty
  - King and rook have not moved
- [ ] Implement promotion
- [ ] Write a function `isSquareAttackedBy(square, color, position)` — you'll use this constantly
- [ ] Write a function `isInCheck(color, position)`
- [ ] Filter pseudo-legal moves to legal moves using make/undo (see Phase 3)

### Checkpoint
- From the starting position, generate exactly 20 legal moves
- From any standard test position, count matches with known values

### Resources
- Move generation wiki: https://www.chessprogramming.org/Move_Generation
- Pawn moves: https://www.chessprogramming.org/Pawn_Moves
- Castling: https://www.chessprogramming.org/Castling
- En passant: https://www.chessprogramming.org/En_passant

---

## Phase 3 — Make & Undo Move

You need to apply a move to a position and reverse it cheaply. The undo system is the backbone of search.

### Goals
- Apply any legal move to produce the next position
- Undo any applied move to restore the previous position exactly
- Handle all edge cases: capture, en passant capture, castling (moves both king and rook), promotion

### Key Concepts to Understand First
- **Irreversible state:** some state can't be reconstructed from the new position alone (castling rights, en passant square, half-move clock, captured piece). You must save this before making the move.
- **Undo stack / history:** store a `StateInfo` record before each make-move so undo can restore it

### Tasks
- [ ] Read: https://www.chessprogramming.org/Make_Move
- [ ] Define a `StateInfo` struct capturing everything needed for undo
- [ ] Implement `makeMove(position, move)` — updates the position in place, saves state
- [ ] Implement `undoMove(position, move)` — restores the position exactly
- [ ] Handle all special move types in both directions
- [ ] Test that `undo(make(pos, move)) == pos` for every generated move

### Checkpoint
- Apply a sequence of moves and undo them back to the original state, and it matches byte-for-byte

---

## Phase 4 — Perft Testing

**Perft** (performance test) counts the number of leaf nodes at a given depth from a position. It is the gold standard for validating move generation.

### Goals
- Prove your move generator is correct against published reference values
- Catch every edge case: castling, en passant, promotion, discovered check, double check

### Tasks
- [ ] Read about Perft: https://www.chessprogramming.org/Perft
- [ ] Implement a `perft(position, depth)` function recursively
- [ ] Test against the standard Perft positions and values: https://www.chessprogramming.org/Perft_Results
- [ ] Start with depth 1 and 2, then push to depth 5 and 6 to catch rare bugs
- [ ] Use **split perft** (print move counts per root move) to isolate bugs quickly

### Key Perft Reference Values (Position 1 — starting position)
| Depth | Nodes       |
|-------|-------------|
| 1     | 20          |
| 2     | 400         |
| 3     | 8,902       |
| 4     | 197,281     |
| 5     | 4,865,609   |
| 6     | 119,060,324 |

### Checkpoint
- Perft(5) on the starting position returns exactly 4,865,609
- Perft tests pass on all 6 standard reference positions

### Resources
- Reference positions and values: https://www.chessprogramming.org/Perft_Results
- More positions: https://gist.github.com/peterellisjones/8c46c28141c162d1d8a0f0badbc9cff9

---

## Phase 5 — Game Loop & Rules Enforcement

With a correct move generator and make/undo, build the actual game experience.

### Goals
- Two players can play a complete game from start to finish in the console
- All special conditions (check, checkmate, stalemate, draws) are handled correctly

### Tasks
- [ ] Implement `isCheckmate(position)` — in check and no legal moves
- [ ] Implement `isStalemate(position)` — not in check but no legal moves
- [ ] Implement the **50-move rule** (half-move clock)
- [ ] Implement **threefold repetition** — requires tracking position history (Zobrist hashing helps here)
- [ ] Implement **insufficient material** draw detection
- [ ] Build a console game loop: print board, prompt for moves in algebraic notation, validate, apply
- [ ] Parse and validate **Standard Algebraic Notation (SAN)** or at minimum **long algebraic notation (LAN)** (e.g., `e2e4`)

### Checkpoint
- Play a full game to checkmate in the console
- The game correctly rejects illegal moves
- The game correctly detects all draw conditions

### Resources
- FIDE Laws of Chess: https://handbook.fide.com/chapter/E012023
- Algebraic notation: https://www.chessprogramming.org/Algebraic_Chess_Notation
- Zobrist hashing: https://www.chessprogramming.org/Zobrist_Hashing

---

## Phase 6 — Zobrist Hashing & Repetition Detection

Needed before building the engine. Hashing uniquely identifies positions as integers, enabling repetition detection and later, the transposition table.

### Goals
- Compute a unique-ish 64-bit hash for any position
- Detect threefold repetition efficiently

### Tasks
- [ ] Read: https://www.chessprogramming.org/Zobrist_Hashing
- [ ] Generate random 64-bit numbers at startup (one per piece per square, plus castling, en passant, and side-to-move bits)
- [ ] Compute the hash incrementally in `makeMove` / `undoMove` (XOR in/out changed components)
- [ ] Store hashes in the position history for repetition detection

### Checkpoint
- The same position reached by different move orders produces the same hash
- Threefold repetition is detected and ends the game

---

## Phase 7 — Engine Foundation (Search & Evaluation)

You've built the game. Now you start building the engine. This is where the real depth begins.

### Goals
- Implement a basic search that finds good moves
- Add a simple evaluation function

### Key Concepts to Understand
- **Minimax / Negamax:** the core recursive search algorithm
- **Alpha-Beta pruning:** the most important optimization; prunes branches that can't change the result
- **Quiescence search:** extends the search at leaf nodes to avoid the horizon effect (stop only at "quiet" positions)
- **Evaluation:** material count, piece-square tables, mobility; simple is fine to start

### Tasks
- [ ] Read about Negamax: https://www.chessprogramming.org/Negamax
- [ ] Read about Alpha-Beta: https://www.chessprogramming.org/Alpha-Beta
- [ ] Implement Negamax with Alpha-Beta
- [ ] Implement Quiescence search: https://www.chessprogramming.org/Quiescence_Search
- [ ] Implement a static evaluation function (material balance is enough at first)
- [ ] Add piece-square tables for better positional play: https://www.chessprogramming.org/Piece-Square_Tables
- [ ] Implement iterative deepening: https://www.chessprogramming.org/Iterative_Deepening
- [ ] Implement a transposition table: https://www.chessprogramming.org/Transposition_Table

### Checkpoint
- The engine can find checkmate-in-one reliably
- The engine plays a recognizable game of chess

---

## Phase 8 — UCI Protocol

Make the engine talk to chess GUIs (e.g., Arena, Cutechess, Lichess).

### Goals
- Implement the UCI communication protocol
- Test the engine in a real chess GUI

### Tasks
- [ ] Read the UCI protocol spec: https://www.shredderchess.com/chess-info/features/uci-universal-chess-interface.html
- [ ] Implement `uci`, `isready`, `position`, `go`, `stop`, `quit` commands
- [ ] Handle time controls from `go wtime btime winc binc`
- [ ] Test in Arena (free GUI): http://www.playwitharena.de/

### Checkpoint
- The engine loads in Arena and plays a complete game

---

## Reference Checklist Summary

| Phase | What You Build                   | Verified By               |
|-------|----------------------------------|---------------------------|
| 0     | Build system, project structure  | Console build compiles    |
| 1     | Board, position, FEN parsing     | Print any FEN correctly   |
| 2     | Move generation                  | Correct move counts       |
| 3     | Make / Undo move                 | Round-trip equality       |
| 4     | Perft tests                      | Match all reference nodes |
| 5     | Full game loop, draw detection   | Play a game to completion |
| 6     | Zobrist hashing, repetition      | Repetition detected       |
| 7     | Search + evaluation (engine)     | Finds mate-in-one         |
| 8     | UCI protocol                     | Works in Arena GUI        |

---

## Essential References

| Resource | URL |
|---|---|
| Chess Programming Wiki | https://www.chessprogramming.org/ |
| FIDE Laws of Chess | https://handbook.fide.com/chapter/E012023 |
| Perft Reference Values | https://www.chessprogramming.org/Perft_Results |
| UCI Protocol | https://www.shredderchess.com/chess-info/features/uci-universal-chess-interface.html |
| Arena GUI | http://www.playwitharena.de/ |
| Stockfish (open source reference) | https://github.com/official-stockfish/Stockfish |
| Blunder (beginner-friendly open source) | https://github.com/algerbrex/blunder |
| Leorik (readable C# engine for concepts) | https://github.com/lithander/Leorik |

> **Tip:** When you are stuck on a concept, Stockfish's source is the authoritative reference. Blunder is better for learning because it is simpler and well-commented.
