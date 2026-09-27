#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H

#include <stdint.h>

#include "sudoku/grid.h"

/* Solve s in place by exhaustive backtracking.
 *
 * Return 1 and fill in s if a solution exists, return 0 and leave s
 * unchanged if not (this includes givens that already break the rules).
 * If guesses is not NULL it receives the number of digits placed during
 * the search. */
int solve_sudoku (sudoku *s, uint64_t *guesses);

#endif
