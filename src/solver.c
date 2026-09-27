#include <stdint.h>

#include "bits.h"
#include "sudoku/solver.h"

/* Board representation
 *
 * A set of cells is a bitboard of three 27-bit bands, one per band of
 * three rows: cell (r, c) is bit 9 * (r % 3) + c of word r / 3, so cell i
 * of the grid is simply bit i % 27 of word i / 27.  Rows and boxes lie
 * within a single word, a column is one bit per row in every word.  A
 * fourth, always empty word pads a set to 128 bits, so that it maps onto
 * one SSE register or half an AVX register.
 *
 * The board keeps one such set per digit, holding the cells where that
 * digit may still go, plus the set of open cells: 164 bytes in all.  Rules
 * are applied to all cells at once with a few word-wide bit operations,
 * and the search backtracks by restoring a saved copy of the board, so
 * there is no undo bookkeeping.  Everything stays in L1. */

#define BAND_ALL 0x7FFFFFFu /* All 27 cells of a band */
#define ROW_BITS 0x1FFu     /* First row of a band */
#define COL_BITS 0x40201u   /* First column of a band */
#define BOX_BITS 0x1C0E07u  /* First box of a band */

/* Band patterns
 *
 * Within a band a digit takes one cell in each of its three rows and in
 * each of its three boxes, so the rows and boxes pair up as a
 * permutation.  A 9-bit pattern records which of the nine mini-rows (a
 * row within a box, bit 3 * r + k for row r and box k) may hold the digit.
 * band_mask[pattern] is the set of cells covered by the permutations that
 * fit into the pattern, or 0 if none does.  This one lookup enforces the
 * locked-candidate rules (pointing and claiming) and more. */

/* The six permutations as mini-row patterns */
#define FITS(m, p) ((((m) & (p)) == (p)) ? (p) : 0)
#define PERMS(m)                                                              \
  (FITS (m, 0x111) | FITS (m, 0x0A1) | FITS (m, 0x10A) | FITS (m, 0x062)       \
   | FITS (m, 0x08C) | FITS (m, 0x054))

/* Mini-row pattern to cells: bit i stands for cells 3i to 3i + 2 */
#define MINI(a, i) ((((a) >> (i)) & 1u) * (7u << (3 * (i))))
#define CELLS(a)                                                              \
  (MINI (a, 0) | MINI (a, 1) | MINI (a, 2) | MINI (a, 3) | MINI (a, 4)        \
   | MINI (a, 5) | MINI (a, 6) | MINI (a, 7) | MINI (a, 8))

#define BAND_MASK(m) CELLS (PERMS (m))
#define BAND_MASK4(m)                                                         \
  BAND_MASK (m), BAND_MASK (m + 1), BAND_MASK (m + 2), BAND_MASK (m + 3)
#define BAND_MASK16(m)                                                        \
  BAND_MASK4 (m), BAND_MASK4 (m + 4), BAND_MASK4 (m + 8), BAND_MASK4 (m + 12)
#define BAND_MASK64(m)                                                        \
  BAND_MASK16 (m), BAND_MASK16 (m + 16), BAND_MASK16 (m + 32),                \
      BAND_MASK16 (m + 48)
#define BAND_MASK256(m)                                                       \
  BAND_MASK64 (m), BAND_MASK64 (m + 64), BAND_MASK64 (m + 128),               \
      BAND_MASK64 (m + 192)

static const uint32_t band_mask[512] = {
  BAND_MASK256 (0u),
  BAND_MASK256 (256u),
};

/* The same for the three columns of a stack: bit 3 * b + j of a pattern
 * stands for column j of the stack within band b, and stack_mask[pattern]
 * is the pattern of the fitting permutations */
#define STACK_MASK4(m) PERMS (m), PERMS (m + 1), PERMS (m + 2), PERMS (m + 3)
#define STACK_MASK16(m)                                                       \
  STACK_MASK4 (m), STACK_MASK4 (m + 4), STACK_MASK4 (m + 8),                  \
      STACK_MASK4 (m + 12)
#define STACK_MASK64(m)                                                       \
  STACK_MASK16 (m), STACK_MASK16 (m + 16), STACK_MASK16 (m + 32),             \
      STACK_MASK16 (m + 48)
#define STACK_MASK256(m)                                                      \
  STACK_MASK64 (m), STACK_MASK64 (m + 64), STACK_MASK64 (m + 128),            \
      STACK_MASK64 (m + 192)

static const uint16_t stack_mask[512] = {
  STACK_MASK256 (0u),
  STACK_MASK256 (256u),
};

/* Mini-row pattern of a band: which mini-rows hold at least one cell */
static inline unsigned
mini_rows (uint32_t w)
{
  const uint32_t t = (w | (w >> 1) | (w >> 2)) & 0x1249249u;
  const uint32_t y = t | (t >> 2) | (t >> 4);

  return (y & 7u) | ((y >> 6) & 0x38u) | ((y >> 12) & 0x1C0u);
}

typedef struct board
{
  uint32_t cand[9][4]; /* Cells where each digit may go; a placed digit
                          keeps its own cell */
  uint32_t open[4];    /* Cells not filled yet */
  uint32_t changed;    /* Digits whose cells changed since their last
                          check for hidden singles (bit d for digit d) */
} board;

/* A branch point of the search: the board before branching, the cell
 * branched on and the digits not tried there yet */
typedef struct frame
{
  board saved;
  uint8_t band;
  uint8_t pos;
  uint16_t left;
} frame;

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
