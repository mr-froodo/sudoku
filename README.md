# sudoku
Sudoku Cracker

## Layout

- `include/sudoku/` – public headers
  - `grid.h` – the grid: an 81-byte value type, parsing, printing, checking
  - `solver.h` – the solver
- `src/` – solver mechanics, built as the `sudoku` library (see below)
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
`-DSUDOKU_NATIVE=OFF`.  On a CPU with AVX2 the solver then uses its AVX2
rules; `-DSUDOKU_SIMD=OFF` forces the portable scalar ones.

## Solver

Depth-first backtracking over bitboards (`src/`):

- `board.h` – the board: per digit, a 128-bit set of the cells where it may
  still go (three 27-bit bands of three rows plus one padding word), plus
  compile-time lookup tables.  The whole board is 164 bytes, the search
  stack holds a saved board per branch point, and there is no heap
  allocation, so everything stays in L1.
- `rules_scalar.h`, `rules_avx2.h` – the rules, in portable C and in AVX2
  with identical behaviour.  The AVX2 version treats a digit as one 128-bit
  vector and processes two digits per 256-bit register; table lookups
  become gathers.
- `solver.c` – the search: after every placement it propagates naked
  singles, hidden singles and a band/stack rule (within three rows or
  columns a digit's rows and boxes must pair up as a permutation, one
  lookup in a 512-entry table), then branches on a cell with the fewest
  candidates and restores the saved board to backtrack.

Throughput on one core of a 2.1 GHz Xeon VM, puzzle sets from
[tdoku](https://github.com/t-dillon/tdoku) (best of several interleaved
runs; the VM's timings vary by about 10%):

| set                        | guesses/puzzle | scalar µs | AVX2 µs |
|----------------------------|---------------:|----------:|--------:|
| magictour top1465          |             21 |       7.5 |     6.7–8.2 |
| forum hardest 1106         |            243 |        62–70 |    56–69 |
| forum hardest 1905 (11+, first 5000) |  137 |        36–42 |    33–35 |
| 17 clues (first 5000)      |            1.6 |       2.2–2.8 |    2.1 |

AVX2 halves the executed instructions but gains only 0–25%: the search is
limited by the serial chain of rule rounds and by data-dependent branches
(placing individual singles), not by arithmetic.

The previous cell-by-cell solver needed 4 µs to 144 ms for the puzzles in
`bench/hard.txt`; this one needs 1–90 µs.
