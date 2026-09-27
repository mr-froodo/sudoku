#ifndef SUDOKU_BOARD_H
#define SUDOKU_BOARD_H

#include <stdint.h>

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

static const uint32_t stack_mask[512] = {
  STACK_MASK256 (0u),
  STACK_MASK256 (256u),
};

/* Per cell, as a 128-bit set: the cell itself, and the cell with all its
 * peers (the cells of its row, column and box) */
#define CELL_WORD(i, w) (((i) / 27 == (w)) ? 1u << ((i) % 27) : 0u)
#define PEER_WORD(i, w)                                                       \
  (((w) < 3 ? COL_BITS << ((i) % 9) : 0u)                                     \
   | (((i) / 27 == (w)) ? (ROW_BITS << (9 * (((i) % 27) / 9)))                \
                              | (BOX_BITS << (3 * (((i) % 9) / 3)))           \
                        : 0u))
#define CELL(i) { CELL_WORD (i, 0), CELL_WORD (i, 1), CELL_WORD (i, 2), 0u }
#define PEER(i)                                                               \
  { PEER_WORD (i, 0), PEER_WORD (i, 1), PEER_WORD (i, 2), 0u }
#define ROW9(f, r)                                                            \
  f (9 * (r) + 0), f (9 * (r) + 1), f (9 * (r) + 2), f (9 * (r) + 3),         \
      f (9 * (r) + 4), f (9 * (r) + 5), f (9 * (r) + 6), f (9 * (r) + 7),     \
      f (9 * (r) + 8)
#define GRID(f)                                                               \
  ROW9 (f, 0), ROW9 (f, 1), ROW9 (f, 2), ROW9 (f, 3), ROW9 (f, 4),            \
      ROW9 (f, 5), ROW9 (f, 6), ROW9 (f, 7), ROW9 (f, 8)

static const uint32_t cell_bits[81][4] = { GRID (CELL) };
static const uint32_t peer_bits[81][4] = { GRID (PEER) };

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

#endif
