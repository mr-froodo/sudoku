# sudoku
Sudoku Cracker

## Layout

- `include/sudoku/` – public headers
  - `grid.h` – sudoku grid: construction, access, printing, rotation
  - `solver.h` – constraint checks and backtracking solver
- `src/` – solver mechanics, built as the `sudoku` library
- `test/bruteforce_solver.c` – test executable that brute-force solves a given sudoku

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
./build/bruteforce_solver 530070000600195000098000060800060003400803001700020006060000280000419005000080079
```

A puzzle is 81 digits read row by row, with `0` marking an empty cell.
Without an argument `bruteforce_solver` solves a built-in example.
