#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sudoku.h"

psudoku
new_sudoku ()
{
  return malloc (sizeof (sudoku));
}

psudoku
init_sudoku (psudoku s)
{
  s->a = malloc (162);

  return s;
}

psudoku
create_empty_sudoku ()
{
  psudoku s;
  char *a, *f;
  int i;

  s = new_sudoku ();
  init_sudoku (s);

  a = s->a;
  f = a + 81;

  for (i = 0; i < 81; i++)
    {
      *a = '0';
      a++;
      *f = 0;
      f++;
    }

  return s;
}

void
del_sudoku (psudoku s)
{
  free (s->a);
}

/*************************************
 * Access methods
 */

void
set_sudoku (psudoku s, const char *cs)
{
  strcpy (s->a, cs);
}
/************************************
 * I/O
 */

void
show_sudoku (psudoku s)
{
  int i, j;

  for (i = 0; i < 9; i++)
    {
      for (j = 0; j < 9; j++)
        {
          printf ("%c ", s->a[j + i * 9]);
        }
      printf ("\n");
    }
  printf ("\n");
}

/* Solver */

void
prepare_sudoku (psudoku s)
{
  int i;
  char *a, *f;

  a = s->a;
  f = a + 81;

  for (i = 0; i < 81; i++)
    {
      if (a[i] == '0')
        f[i] = 0;
      else
        f[i] = 1;
    }
}

int
check_row (psudoku s, int n)
{
  int i;  /* Index of first entry in row */
  int k;  /* Counter */
  char c; /* Value of entry to check */
  char *a;

  a = s->a;

  /* Store value of entry */
  c = *(a + n);

  /* Compute index of firste entry in row */
  i = (n / 9) * 9; /* Integer devision! */

  /* Check against left entries */
  for (k = i; k < n; k++)
    if (c == *(a + k))
      return 1;

  /* Check against right etries */
  for (k = n + 1; k < i + 9; k++)
    if (c == *(a + k))
      return 1;

  /* All checks are passed */
  return 0;
}

int
check_col (psudoku s, int n)
{
  int j;  /* Index of first entry in col */
  int k;  /* Counter */
  char c; /* Value of entry to check */
  char *a;

  a = s->a;

  /* Store value of entry */
  c = *(a + n);

  /* Compute index of firste entry in col */
  j = n % 9;

  /* Check against above entries */
  for (k = j; k < n; k += 9)
    if (c == *(a + k))
      return 1;

  /* Check against below etries */
  for (k = n + 9; k < 81 + 9; k += 9)
    if (c == *(a + k))
      return 1;

  /* All checks are passed */
  return 0;
}

int
check_box (psudoku s, int n)
{
  int i, j, k; /* Index of top left entry of box */
  char c;      /* Value of position to check */
  char *a;

  a = s->a;
  c = *(a + n);

  //   i = n / 9;  /* Row number */
  //   i -= i % 3; /* Number of first row of block */

  //   j = n % 9;  /* Col number */
  //   j -= j % 3; /* Number of first col of block */

  //   k = i * 9 + j; /* Index of top left entry of block */

  i = n - n % 3; /*First index in row of block */
  j = n - 

  if (a[k] +)

    /* Check against each entry of block */
    for (i = 0; i < 3; i++)
      for (j = 0; j < 3; j++)
        if (k + j + i * 9 != n)
          if (*(a + (k + j + i * 9)) == c)
            return 1;
  /* All checks pased */
  return 0;
}

int
check_position (psudoku s, int n)
{

  if (check_row (s, n))
    return 1;

  if (check_col (s, n))
    return 1;

  if (check_box (s, n))
    return 1;

  return 0;
}

int
solve_sudoku (psudoku s)
{
  int n, steps;
  char *a, *f;

  a = s->a;
  f = a + 81;

  steps = n = 0;

  while (n < 81)
    {
      if (f[n])
        {
          /* Value is fixed -> move on */
          n++;
        }
      else
        {
          /* Cell has to be filled */
          if (a[n] == '0')
            {
              /* Cell is empty --> Start with 1 */
              a[n] = '1';
              steps++;
            }
          else
            {
              /* Cell already filled --> Increase value */
              a[n] += 1;
              steps++;
            }

          /* While checks fail and c<=9*/
          while (check_position (s, n) && a[n] <= '9')
            {
              a[n] += 1;
              steps++;
            }

          /* Go back if all checks have failed */
          if (a[n] == '9' + 1)
            {
              /* Clear cell and go back */
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
    }

  if (n == 81)
    return steps;
  else
    return 0;
}
