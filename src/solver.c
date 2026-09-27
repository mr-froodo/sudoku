#include <stdint.h>

#include "bits.h"
#include "board.h"
#include "sudoku/solver.h"

/* The board rules: place, place_single, cell_digits, naked_singles,
 * hidden_singles and choose.  Both implementations behave identically;
 * the AVX2 one works on whole 128-bit digit sets and pairs of digits. */
#if defined(__AVX2__) && !defined(SUDOKU_NO_SIMD)
#include "rules_avx2.h"
#else
#include "rules_scalar.h"
#endif

/* A branch point of the search: the board before branching, the cell
 * branched on and the digits not tried there yet */
typedef struct frame
{
  board saved;
  uint8_t band;
  uint8_t pos;
  uint16_t left;
} frame;

/* Apply the rules until none makes progress.  Return 0 on a
 * contradiction. */
static int
propagate (board *g)
{
  for (;;)
    {
      int hidden = 0;

      if (!naked_singles (g))
        return 0;

      hidden = hidden_singles (g);
      if (hidden <= 0)
        return hidden == 0;
    }
}

int
solve_sudoku (sudoku *s, uint64_t *guesses)
{
  frame stack[81];
  int top = 0;
  board g;
  uint64_t count = 0;
  int ok = 1;

  for (int d = 0; d < 9; d++)
    {
      g.cand[d][0] = g.cand[d][1] = g.cand[d][2] = BAND_ALL;
      g.cand[d][3] = 0;
    }
  g.open[0] = g.open[1] = g.open[2] = BAND_ALL;
  g.open[3] = 0;
  g.changed = 0x1FF;

  /* Place the givens, rejecting a grid that already breaks the rules */
  for (int i = 0; i < 81; i++)
    {
      const int d = s->cell[i] - 1;
      const int b = i / 27;
      const int pos = i % 27;

      if (d < 0)
        continue;
      if (d > 8 || !((g.cand[d][b] >> pos) & 1u))
        return 0;
      place (&g, d, b, pos);
    }

  ok = propagate (&g);

  for (;;)
    {
      int b = 0;
      int pos = 0;

      if (ok)
        {
          if (!choose (&g, &b, &pos))
            break; /* Solved */

          stack[top].saved = g;
          stack[top].band = (uint8_t)b;
          stack[top].pos = (uint8_t)pos;
          stack[top].left = (uint16_t)cell_digits (&g, b, pos);
          top++;
        }
      else if (top == 0)
        {
          if (guesses)
            *guesses = count;
          return 0; /* Every branch failed: no solution */
        }

      /* Try the next digit of the innermost branch point; its last digit
       * needs no saved board any more, so the frame is dropped then */
      {
        frame *f = &stack[top - 1];
        const unsigned left = f->left;
        const int d = bit_index (left);

        g = f->saved;
        b = f->band;
        pos = f->pos;
        f->left = (uint16_t)(left & (left - 1));
        if (!f->left)
          top--;

        place (&g, d, b, pos);
        count++;
        ok = propagate (&g);
      }
    }

  /* Every cell now holds exactly one digit */
  for (int d = 0; d < 9; d++)
    for (int b = 0; b < 3; b++)
      for (uint32_t w = g.cand[d][b]; w; w &= w - 1)
        s->cell[27 * b + bit_index (w)] = (uint8_t)(d + 1);

  if (guesses)
    *guesses = count;
  return 1;
}
