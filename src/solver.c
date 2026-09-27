#include "sudoku/solver.h"

/* Each check returns 1 if the value at cell n already appears elsewhere
 * in its row, column or box, and 0 otherwise. */

static int
check_row (const sudoku *s, int n)
{
  const char *a = s->a;
  const char c = a[n];           /* Value of entry to check */
  const int first = (n / 9) * 9; /* Index of first entry in row */

  for (int k = first; k < first + 9; k++)
    if (k != n && a[k] == c)
      return 1;

  return 0;
}

static int
check_col (const sudoku *s, int n)
{
  const char *a = s->a;
  const char c = a[n];     /* Value of entry to check */
  const int first = n % 9; /* Index of first entry in col */

  for (int k = first; k < 81; k += 9)
    if (k != n && a[k] == c)
      return 1;

  return 0;
}

static int
check_box (const sudoku *s, int n)
{
  const char *a = s->a;
  const char c = a[n];                /* Value of entry to check */
  const int row = n / 9 - (n / 9) % 3; /* First row of box */
  const int col = n % 9 - (n % 9) % 3; /* First col of box */
  const int first = row * 9 + col;     /* Index of top left entry of box */

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      {
        const int k = first + j + i * 9;

        if (k != n && a[k] == c)
          return 1;
      }

  return 0;
}

int
check_position (const sudoku *s, int n)
{
  return check_row (s, n) || check_col (s, n) || check_box (s, n);
}

int
solve_sudoku (sudoku *s)
{
  char *a = s->a;
  const char *f = s->a + 81;
  int n = 0;
  int steps = 0;

  while (n < 81)
    {
      if (f[n])
        {
          /* Value is fixed -> move on */
          n++;
          continue;
        }

      /* Try the next value ('0' -> '1' for an empty cell) */
      a[n]++;
      steps++;

      /* Increase value while checks fail and value <= 9 */
      while (check_position (s, n) && a[n] <= '9')
        {
          a[n]++;
          steps++;
        }

      if (a[n] > '9')
        {
          /* All values failed: clear cell and go back */
          a[n] = '0';
          n--;
          while (f[n])
            /* Value is fixed -> go further back */
            n--;
        }
      else
        {
          /* Move on */
          n++;
        }
    }

  return n == 81 ? steps : 0;
}
