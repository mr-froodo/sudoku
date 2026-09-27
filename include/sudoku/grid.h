#ifndef SUDOKU_GRID_H
#define SUDOKU_GRID_H

#include <stdint.h>

/* A sudoku grid: 81 cells in row-major order, 0 = empty, 1-9 = digit.
 *
 * It is a plain 81-byte value without any heap storage, so it can live on
 * the stack and be copied with a simple assignment. */
typedef struct sudoku
{
  uint8_t cell[81];
} sudoku;

/* Parse exactly 81 characters read row by row: '1'-'9' are digits and
 * '0' or '.' mark empty cells.  Return 1 on success, 0 if text is
 * malformed, in which case the contents of s are unspecified. */
int set_sudoku (sudoku *s, const char *text);

/* Print s as a 9x9 grid, '.' marking empty cells */
void show_sudoku (const sudoku *s);

/* Return 1 if no digit repeats within a row, column or box */
int check_sudoku (const sudoku *s);

/* Return 1 if solution is completely filled, obeys the rules and keeps
 * every given digit of puzzle */
int solves_sudoku (const sudoku *puzzle, const sudoku *solution);

#endif
