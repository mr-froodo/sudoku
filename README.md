# sudoku
Sudoku Cracker

## Layout

- `include/` – public header (`sudoku.h`)
- `src/` – solver mechanics
- `test/` – test executables (`scrack.c`: brute-force solves a given sudoku)

## Build and run

```sh
make            # builds build/scrack
make test       # solves the built-in example puzzle
./build/scrack 530070000600195000098000060800060003400803001700020006060000280000419005000080079
```

A puzzle is 81 digits read row by row, with `0` marking an empty cell.
