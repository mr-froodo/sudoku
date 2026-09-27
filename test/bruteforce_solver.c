#include <stdint.h>
#include <stdio.h>
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

/* Average solve time in seconds, repeating until 0.1 s have elapsed so
 * that the clock resolution does not dominate */
static double
average_solve_time (const sudoku *puzzle)
{
  const clock_t begin = clock ();
  clock_t end = begin;
  long runs = 0;

  do
    {
      sudoku s = *puzzle;

      solve_sudoku (&s, NULL);
      runs++;
      end = clock ();
    }
  while (end - begin < CLOCKS_PER_SEC / 10);

  return (double)(end - begin) / CLOCKS_PER_SEC / (double)runs;
}

int
main (int argc, char *argv[])
{
  const char *text = argc > 1 ? argv[1] : default_puzzle;
  sudoku puzzle;
  sudoku s;
  uint64_t guesses = 0;

  if (argc > 2 || !set_sudoku (&puzzle, text))
    {
      fprintf (stderr, "Usage: %s [PUZZLE]\n"
                       "PUZZLE is 81 characters read row by row: digits, "
                       "with 0 or . marking an empty cell.\n",
               argv[0]);
      return 1;
    }

  show_sudoku (&puzzle);

  s = puzzle;
  if (!solve_sudoku (&s, &guesses))
    {
      printf ("Sorry, no solution found after %llu guesses.\n",
              (unsigned long long)guesses);
      return 2;
    }

  if (!solves_sudoku (&puzzle, &s))
    {
      printf ("Error: the solver returned an invalid solution.\n");
      show_sudoku (&s);
      return 3;
    }

  printf ("Success!\n"
          "Solution found in %.2f us after %llu guesses.\n\n",
          average_solve_time (&puzzle) * 1e6, (unsigned long long)guesses);
  show_sudoku (&s);

  return 0;
}
