#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sudoku/grid.h"

psudoku
new_sudoku (void)
{
  return malloc (sizeof (sudoku));
}

psudoku
init_sudoku (psudoku s)
{
  s->a = malloc (162);
  s->rotations = 0;

  return s;
}

psudoku
create_empty_sudoku (void)
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
  if (!s)
    return;

  free (s->a);
  free (s);
}

void
copy_sudoku (pcsudoku s, psudoku t)
{
  int i;

  for (i = 0; i < 162; i++)
    {
      t->a[i] = s->a[i];
    }
}

/*************************************
 * Access methods
 */

void
set_sudoku (psudoku s, const char *cs)
{
  strcpy (s->a, cs);
}

int
sudoku_locate_entries (psudoku s)
{
  int i, j;
  int max, result;
  int u, r, d, l;
  char *f;

  f = s->a + 81;

  /* Up */
  u = 0;
  for (i = 0; i < 45; i++)
    if (f[i])
      u++;

  max = u;
  result = 0;

  /* Right */
  r = 0;
  for (i = 0; i < 9; i++)
    for (j = 4; j < 9; j++)
      if (f[j + i * 9])
        r++;

  if (r > max)
    {
      max = r;
      result = 1;
    }

  /* DOwn */
  d = 0;
  for (i = 36; i < 81; i++)
    if (f[i])
      d++;

  if (d > max)
    {
      max = d;
      result = 2;
    }

  /* Left */
  l = 0;
  for (i = 0; i < 9; i++)
    for (j = 0; j < 5; j++)
      if (f[j + i * 9])
        l++;

  if (l > max)
    result = 3;

  //   printf ("U\tR\tD\tL\n"
  //           "%d\t%d\t%d\t%d\n",
  //           u, r, d, l);

  return result;
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

/************************************
 * Transformations
 */

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

void
rotate_sudoku (psudoku s)
{
  psudoku t;
  int i, j;

  t = new_sudoku ();
  init_sudoku (t);

  copy_sudoku (s, t);

  for (i = 0; i < 9; i++)
    for (j = 0; j < 9; j++)
      {
        s->a[j + i * 9] = t->a[(8 - i) + j * 9];
      }

  prepare_sudoku (s);

  del_sudoku (t);

  s->rotations++;
}
