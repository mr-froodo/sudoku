#ifndef SUDOKU_RULES_SCALAR_H
#define SUDOKU_RULES_SCALAR_H

/* Portable implementation of the board rules, one 32-bit band at a time */

#include "bits.h"
#include "board.h"

/* Drop digit d (0-8) from the peers of bit pos of band b and mark the
 * cell as filled.  Enough by itself when d is the cell's only candidate. */
static inline void
place_single (board *g, int d, int b, int pos)
{
  const uint32_t bit = 1u << pos;
  const int col = pos % 9;
  const uint32_t col_bits = COL_BITS << col;
  const uint32_t own_bits = (ROW_BITS << (pos - col))
                            | (BOX_BITS << (col - col % 3)) | col_bits;

  g->cand[d][0] &= ~col_bits;
  g->cand[d][1] &= ~col_bits;
  g->cand[d][2] &= ~col_bits;
  g->cand[d][b] = (g->cand[d][b] & ~own_bits) | bit;
  g->open[b] &= ~bit;
  g->changed |= 1u << d;
}

/* Put digit d (0-8) into bit pos of band b: remove all other digits from
 * the cell, then d from its peers */
static inline void
place (board *g, int d, int b, int pos)
{
  const uint32_t bit = 1u << pos;

  for (int e = 0; e < 9; e++)
    {
      g->changed |= ((g->cand[e][b] >> pos) & 1u) << e;
      g->cand[e][b] &= ~bit;
    }
  place_single (g, d, b, pos);
}

/* Digits (as a 9-bit mask) that may go into bit pos of band b */
static inline unsigned
cell_digits (const board *g, int b, int pos)
{
  unsigned mask = 0;

  for (int d = 0; d < 9; d++)
    mask |= ((g->cand[d][b] >> pos) & 1u) << d;

  return mask;
}

/* Fill naked singles (open cells with one candidate) until none are left.
 * Return 0 on a contradiction: an open cell without candidates. */
static int
naked_singles (board *g)
{
  for (;;)
    {
      uint32_t any = 0;

      for (int b = 0; b < 3; b++)
        {
          uint32_t ones = 0;
          uint32_t twos = 0;
          uint32_t single = 0;
          uint32_t plane[4] = { 0, 0, 0, 0 }; /* Bits of the digit index */

          for (int d = 0; d < 9; d++)
            {
              const uint32_t x = g->cand[d][b];

              twos |= ones & x;
              ones |= x;
              plane[0] |= (d & 1) ? x : 0;
              plane[1] |= (d & 2) ? x : 0;
              plane[2] |= (d & 4) ? x : 0;
              plane[3] |= (d & 8) ? x : 0;
            }

          if (g->open[b] & ~ones)
            return 0;

          /* In a cell with a single candidate the planes spell its digit.
           * Two singles may compete for the same digit in a unit: the
           * loser is left without candidates, a contradiction caught in
           * the next round. */
          single = g->open[b] & ~twos;
          any |= single;
          for (; single; single &= single - 1)
            {
              const int pos = bit_index (single);
              const int d = (int)(((plane[0] >> pos) & 1u)
                                  | ((plane[1] >> pos) & 1u) << 1
                                  | ((plane[2] >> pos) & 1u) << 2
                                  | ((plane[3] >> pos) & 1u) << 3);

              if ((g->cand[d][b] >> pos) & 1u)
                place_single (g, d, b, pos);
            }
        }

      if (!any)
        return 1;
    }
}

/* For every changed digit, drop the cells that fit no band or stack
 * pattern, then fill its hidden singles: cells that are the only place
 * left for the digit in a row, column or box.  Return 1 if anything
 * changed, 0 if nothing did, or -1 on a contradiction: a digit with no
 * possible cell in some unit, or two hidden singles that exclude each
 * other. */
static int
hidden_singles (board *g)
{
  int found = 0;

  for (int d = 0; d < 9; d++)
    {
      const uint32_t *x = g->cand[d];
      uint32_t hidden[3] = { 0, 0, 0 };
      uint32_t ones = 0;
      uint32_t twos = 0;
      uint32_t cols = 0;

      /* Unchanged since its last check.  Removing cells below does not
       * mark the digit again: rechecking costs more than it prunes, and
       * whatever is missed now is caught later without affecting
       * correctness. */
      if (!((g->changed >> d) & 1u))
        continue;
      g->changed &= ~(1u << d);

      /* Drop the cells that fit no permutation within their band; a
       * band without any permutation is a contradiction */
      for (int b = 0; b < 3; b++)
        {
          const uint32_t keep = band_mask[mini_rows (x[b])];

          if (!keep)
            return -1;
          if (x[b] & ~keep)
            {
              g->cand[d][b] = x[b] & keep;
              found = 1;
            }
        }

      /* Likewise for the stacks, working on the columns of each band */
      {
        uint32_t col[3];
        uint32_t keep[3] = { 0, 0, 0 };

        for (int b = 0; b < 3; b++)
          col[b] = (x[b] | (x[b] >> 9) | (x[b] >> 18)) & ROW_BITS;

        for (int t = 0; t < 3; t++)
          {
            const unsigned fit = stack_mask[((col[0] >> (3 * t)) & 7u)
                                            | ((col[1] >> (3 * t)) & 7u) << 3
                                            | ((col[2] >> (3 * t)) & 7u)
                                                  << 6];

            if (!fit)
              return -1;
            for (int b = 0; b < 3; b++)
              keep[b] |= ((fit >> (3 * b)) & 7u) << (3 * t);
          }

        for (int b = 0; b < 3; b++)
          {
            const uint32_t cells = keep[b] | (keep[b] << 9) | (keep[b] << 18);

            if (x[b] & ~cells)
              {
                g->cand[d][b] = x[b] & cells;
                found = 1;
              }
          }
      }

      /* Rows and boxes lie within one band.  A unit holds a single when
       * clearing its lowest bit leaves nothing; the tests are branch-free. */
      for (int b = 0; b < 3; b++)
        for (int k = 0; k < 3; k++)
          {
            const uint32_t row = x[b] & (ROW_BITS << (9 * k));
            const uint32_t box = x[b] & (BOX_BITS << (3 * k));
            const uint32_t slice = (x[b] >> (9 * k)) & ROW_BITS;

            hidden[b] |= (row & (row - 1)) ? 0 : row;
            hidden[b] |= (box & (box - 1)) ? 0 : box;

            /* Count per column over the nine rows */
            twos |= ones & slice;
            ones |= slice;
          }

      /* Columns: ones and twos hold, per column, whether the digit fits
       * at least once and at least twice */
      if (ones != ROW_BITS)
        return -1;
      cols = ones & ~twos;
      cols |= (cols << 9) | (cols << 18);

      for (int b = 0; b < 3; b++)
        {
          /* A placed digit keeps its cell, so drop filled cells */
          uint32_t w = (hidden[b] | (x[b] & cols)) & g->open[b];

          for (; w; w &= w - 1)
            {
              const int pos = bit_index (w);

              /* An earlier single took the cell or excluded the digit */
              if (!(((g->open[b] & g->cand[d][b]) >> pos) & 1u))
                return -1;
              place (g, d, b, pos);
              found = 1;
            }
        }
    }

  return found;
}

/* Choose the open cell to branch on: one with two candidates if there is
 * any, else one with three, else the first open cell.  Return 0 if no
 * cell is open, i.e. the board is solved. */
static int
choose (const board *g, int *band, int *pos)
{
  uint32_t two[3];
  uint32_t three[3];
  uint32_t any = 0;

  for (int b = 0; b < 3; b++)
    {
      uint32_t ones = 0;
      uint32_t twos = 0;
      uint32_t threes = 0;
      uint32_t fours = 0;

      for (int d = 0; d < 9; d++)
        {
          const uint32_t x = g->cand[d][b];

          fours |= threes & x;
          threes |= twos & x;
          twos |= ones & x;
          ones |= x;
        }

      two[b] = g->open[b] & ~threes;
      three[b] = g->open[b] & threes & ~fours;
      any |= g->open[b];
    }

  if (!any)
    return 0;

  for (int b = 0; b < 3; b++)
    if (two[b])
      {
        *band = b;
        *pos = bit_index (two[b]);
        return 1;
      }

  for (int b = 0; b < 3; b++)
    if (three[b])
      {
        *band = b;
        *pos = bit_index (three[b]);
        return 1;
      }

  for (int b = 0; b < 3; b++)
    if (g->open[b])
      {
        *band = b;
        *pos = bit_index (g->open[b]);
        return 1;
      }

  return 0;
}

#endif
