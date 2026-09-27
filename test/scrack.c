#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sudoku.h"

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
  int i;

  if (strlen (p) != 81)
    return 0;
  for (i = 0; i < 81; i++)
    if (p[i] < '0' || p[i] > '9')
      return 0;
  return 1;
}

int
main (int argc, char *argv[])
{
  psudoku s;
  clock_t begin, end;
  int i, n, result;
  double time_spent;
  const char *puzzle;

  puzzle = argc > 1 ? argv[1] : default_puzzle;
  if (argc > 2 || !valid_puzzle (puzzle))
    {
      fprintf (stderr, "Usage: %s [PUZZLE]\n"
                       "PUZZLE is 81 digits read row by row, "
                       "0 marking an empty cell.\n",
               argv[0]);
      return 1;
    }

  s = new_sudoku ();
  init_sudoku (s);

  set_sudoku (s, puzzle);

  prepare_sudoku (s);

  show_sudoku (s);
  n = sudoku_locate_entries (s);
  printf ("Rotate %d times.\n", n);
  for (i = 0; i < n; i++)
    rotate_sudoku (s);

  show_sudoku (s);

  begin = clock ();
  result = solve_sudoku (s);
  end = clock ();

  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

  result ? printf ("Success!\n"
                   "Solution found in %.2e seconds after %d steps.\n",
                   time_spent, result)
         : printf ("Sorry, no solution found.\n");

  show_sudoku (s);
  printf ("Rotate %d times\n", 4 - (s->rotations % 4));
  while (s->rotations % 4)
    rotate_sudoku (s);
  show_sudoku (s);

  del_sudoku (s);

  return 0;
}