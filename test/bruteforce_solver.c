#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sudoku/grid.h"
#include "sudoku/solver.h"

/* Default puzzle, used when none is given on the command line */
static const char *default_puzzle = "123456789"
                                    "000000123"
                                    "000000045"
                                    "000000002"
                                    "000000004"
                                    "000000010"
                                    "000000430"
                                    "000000590"
                                    "000000001";

/* A puzzle is 81 digits read row by row, 0 marking an empty cell */
static int
valid_puzzle (const char *p)
{
  if (strlen (p) != 81)
    return 0;

  for (int i = 0; i < 81; i++)
    if (p[i] < '0' || p[i] > '9')
      return 0;

  return 1;
}

/* Solve s and report the outcome */
static void
timed_solve (sudoku *s)
{
  const clock_t begin = clock ();
  const int steps = solve_sudoku (s);
  const clock_t end = clock ();
  const double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

  if (steps)
    printf ("Success!\n"
            "Solution found in %.2e seconds after %d steps.\n",
            time_spent, steps);
  else
    printf ("Sorry, no solution found.\n");
}

/* Rotate s so that most given entries come first */
static void
rotate_for_solving (sudoku *s)
{
  const int n = sudoku_locate_entries (s);

  printf ("Rotate %d times.\n", n);
  for (int i = 0; i < n; i++)
    rotate_sudoku (s);
}

/* Rotate s back to its original orientation */
static void
rotate_back (sudoku *s)
{
  printf ("Rotate %d times\n", 4 - (s->rotations % 4));
  while (s->rotations % 4)
    rotate_sudoku (s);
}

static void
solve_puzzle (const char *puzzle)
{
  sudoku *s = init_sudoku (new_sudoku ());

  set_sudoku (s, puzzle);
  prepare_sudoku (s);
  show_sudoku (s);

  rotate_for_solving (s);
  show_sudoku (s);

  timed_solve (s);
  show_sudoku (s);

  rotate_back (s);
  show_sudoku (s);

  del_sudoku (s);
}

int
main (int argc, char *argv[])
{
  const char *puzzle = argc > 1 ? argv[1] : default_puzzle;

  if (argc > 2 || !valid_puzzle (puzzle))
    {
      fprintf (stderr, "Usage: %s [PUZZLE]\n"
                       "PUZZLE is 81 digits read row by row, "
                       "0 marking an empty cell.\n",
               argv[0]);
      return 1;
    }

  solve_puzzle (puzzle);

  return 0;
}
