#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H

#include "sudoku/grid.h"

/* Return 1 if the value at cell n clashes with its row, column or box */
int check_position (const sudoku *s, int n);

/* Solve by backtracking; return the number of steps taken, 0 on failure */
int solve_sudoku (sudoku *s);

#endif
