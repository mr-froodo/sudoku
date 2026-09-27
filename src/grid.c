#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sudoku/grid.h"

sudoku *
new_sudoku (void)
{
  return malloc (sizeof (sudoku));
}

sudoku *
init_sudoku (sudoku *s)
{
  s->a = malloc (162);
  s->rotations = 0;

  return s;
}

sudoku *
create_empty_sudoku (void)
{
  sudoku *s = init_sudoku (new_sudoku ());
  char *a = s->a;
  char *f = a + 81;

  for (int i = 0; i < 81; i++)
    {
      a[i] = '0';
      f[i] = 0;
    }

  return s;
}

void
del_sudoku (sudoku *s)
{
  if (!s)
    return;

  free (s->a);
  free (s);
}

void
copy_sudoku (const sudoku *s, sudoku *t)
{
  memcpy (t->a, s->a, 162);
}

/*************************************
 * Access methods
 */

void
set_sudoku (sudoku *s, const char *cs)
{
  strcpy (s->a, cs);
}

/* Count fixed cells in rows [r0, r1) and columns [c0, c1) */
static int
count_fixed (const char *f, int r0, int r1, int c0, int c1)
{
  int count = 0;

  for (int i = r0; i < r1; i++)
    for (int j = c0; j < c1; j++)
      if (f[j + i * 9])
        count++;

  return count;
}

int
sudoku_locate_entries (const sudoku *s)
{
  const char *f = s->a + 81;
  const int count[4] = {
    count_fixed (f, 0, 5, 0, 9), /* Upper half */
    count_fixed (f, 0, 9, 4, 9), /* Right half */
    count_fixed (f, 4, 9, 0, 9), /* Lower half */
    count_fixed (f, 0, 9, 0, 5), /* Left half */
  };
  int result = 0;

  for (int k = 1; k < 4; k++)
    if (count[k] > count[result])
      result = k;

  return result;
}

/************************************
 * I/O
 */

void
show_sudoku (const sudoku *s)
{
  for (int i = 0; i < 9; i++)
    {
      for (int j = 0; j < 9; j++)
        printf ("%c ", s->a[j + i * 9]);
      printf ("\n");
    }
  printf ("\n");
}

/************************************
 * Transformations
 */

void
prepare_sudoku (sudoku *s)
{
  const char *a = s->a;
  char *f = s->a + 81;

  for (int i = 0; i < 81; i++)
    f[i] = a[i] != '0';
}

void
rotate_sudoku (sudoku *s)
{
  char t[81];

  memcpy (t, s->a, 81);

  for (int i = 0; i < 9; i++)
    for (int j = 0; j < 9; j++)
      s->a[j + i * 9] = t[(8 - i) + j * 9];

  prepare_sudoku (s);

  s->rotations++;
}
