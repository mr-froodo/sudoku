#include <stdio.h>
#include <time.h>

#include "sudoku.h"

int
main ()
{
  psudoku s;
  clock_t begin, end;
  int i, n, result;
  double time_spent;

  s = new_sudoku ();
  init_sudoku (s);

  set_sudoku (s, "123456789"
                 "000000123"
                 "000000045"
                 "000000002"
                 "000000004"
                 "000000010"
                 "000000430"
                 "000000590"
                 "000000001");

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