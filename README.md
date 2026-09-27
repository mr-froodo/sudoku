# sudoku
Sudoku Cracker

## Layout

- `include/sudoku/` – public headers
  - `grid.h` – the grid: an 81-byte value type, parsing, printing, checking
  - `solver.h` – the solver
- `src/` – solver mechanics, built as the `sudoku` library
- `test/bruteforce_solver.c` – solves a given sudoku and times it
- `bench/benchmark.c`, `bench/hard.txt` – benchmark over a file of puzzles

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
./build/bruteforce_solver 530070000600195000098000060800060003400803001700020006060000280000419005000080079
./build/benchmark bench/hard.txt        # time each puzzle
./build/benchmark -s puzzles.txt        # throughput over a large set
```

A puzzle is 81 characters read row by row: digits, with `0` or `.` marking
an empty cell.  The build defaults to `Release` (`-O3`, which matters: `-O2`
is about 45% slower) and `-march=native`; turn the latter off with
`-DSUDOKU_NATIVE=OFF`.

## Solver

Depth-first backtracking over bitboards (`src/solver.c`):

- Per digit, a 128-bit set of the cells where it may still go: three 27-bit
  bands of three rows plus one padding word, so a digit maps onto one SSE
  register or half an AVX2 register.  The whole board is 164 bytes, the
  search stack holds a saved board per branch point, and there is no heap
  allocation, so everything stays in L1.
- After every placement the solver propagates naked singles, hidden singles
  and a band/stack rule: within three rows (or columns) a digit's rows and
  boxes must pair up as a permutation, enforced by one lookup in a 512-entry
  table built at compile time.
- It branches on a cell with the fewest candidates and restores the saved
  board to backtrack.

Throughput on one core of a 2.1 GHz Xeon VM, puzzle sets from
[tdoku](https://github.com/t-dillon/tdoku):

| set                        | guesses/puzzle | µs/puzzle |
|----------------------------|---------------:|----------:|
| magictour top1465          |             21 |        11 |
| forum hardest 1106         |            243 |        92 |
| forum hardest 1905 (11+)   |              – |        53 |
| 17 clues (first 5000)      |              – |       3.3 |

The previous solver needed 4 µs to 144 ms for the puzzles in
`bench/hard.txt`; this one needs 1–90 µs.
