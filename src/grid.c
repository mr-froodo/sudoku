#include <stdio.h>

#include "sudoku/grid.h"

int
set_sudoku (sudoku *s, const char *text)
{
  for (int i = 0; i < 81; i++)
    {
      const char c = text[i];

      if (c >= '1' && c <= '9')
        s->cell[i] = (uint8_t)(c - '0');
      else if (c == '0' || c == '.')
        s->cell[i] = 0;
      else
        return 0; /* Also catches a string that is too short */
    }

  return text[81] == '\0';
}

void
show_sudoku (const sudoku *s)
{
  for (int i = 0; i < 9; i++)
    {
      if (i == 3 || i == 6)
        printf ("------+-------+------\n");

      for (int j = 0; j < 9; j++)
        {
          const int d = s->cell[j + i * 9];

          if (j == 3 || j == 6)
            printf ("| ");
          printf ("%c%s", d ? '0' + d : '.', j < 8 ? " " : "\n");
        }
    }
  printf ("\n");
}

int
check_sudoku (const sudoku *s)
{
  unsigned used[27] = { 0 }; /* Rows 0-8, columns 9-17, boxes 18-26 */

  for (int i = 0; i < 81; i++)
    {
      const unsigned d = s->cell[i];
      const int r = i / 9;
      const int c = 9 + i % 9;
      const int b = 18 + (i / 27) * 3 + (i % 9) / 3;
      const unsigned bit = d ? 1u << (d - 1) : 0;

      if (d > 9 || ((used[r] | used[c] | used[b]) & bit))
        return 0;

      used[r] |= bit;
      used[c] |= bit;
      used[b] |= bit;
    }

  return 1;
}

int
solves_sudoku (const sudoku *puzzle, const sudoku *solution)
{
  for (int i = 0; i < 81; i++)
    {
      const unsigned given = puzzle->cell[i];
      const unsigned d = solution->cell[i];

      if (d < 1 || d > 9 || (given && given != d))
        return 0;
    }

  return check_sudoku (solution);
}
