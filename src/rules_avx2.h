#ifndef SUDOKU_RULES_AVX2_H
#define SUDOKU_RULES_AVX2_H

/* AVX2 implementation of the board rules
 *
 * A digit's cell set (three bands plus padding) is one 128-bit vector, and
 * as cand[d] and cand[d + 1] are adjacent in memory, two digits load as one
 * 256-bit vector: lanes 0-2 hold the bands of digit d, lanes 4-6 those of
 * digit d + 1, lanes 3 and 7 are padding.  Digit 8 fills the lower half
 * only.  Results of padding lanes are masked off wherever they could
 * matter. */

#include <immintrin.h>

#include "bits.h"
#include "board.h"

static inline __m128i
load4 (const uint32_t *p)
{
  return _mm_loadu_si128 ((const __m128i *)p);
}

static inline void
store4 (uint32_t *p, __m128i v)
{
  _mm_storeu_si128 ((__m128i *)p, v);
}

static inline __m256i
load8 (const uint32_t *p)
{
  return _mm256_loadu_si256 ((const __m256i *)p);
}

static inline void
store8 (uint32_t *p, __m256i v)
{
  _mm256_storeu_si256 ((__m256i *)p, v);
}

/* Lanes of v that are zero, as an 8-bit mask */
static inline unsigned
zero_lanes (__m256i v)
{
  return (unsigned)_mm256_movemask_ps (
      _mm256_castsi256_ps (_mm256_cmpeq_epi32 (v, _mm256_setzero_si256 ())));
}

/* Digits (as a 9-bit mask) that may go into bit pos of band b */
static inline unsigned
cell_digits (const board *g, int b, int pos)
{
  /* Move the cell's bit into the sign bit of every lane */
  const __m128i shift = _mm_cvtsi32_si128 (31 - pos);
  unsigned mask = 0;

  for (int k = 0; k < 4; k++)
    {
      const unsigned m = (unsigned)_mm256_movemask_ps (_mm256_castsi256_ps (
          _mm256_sll_epi32 (load8 (g->cand[2 * k]), shift)));

      mask |= ((m >> b) & 1u) << (2 * k);
      mask |= ((m >> (b + 4)) & 1u) << (2 * k + 1);
    }
  mask |= (((unsigned)_mm_movemask_ps (_mm_castsi128_ps (
                _mm_sll_epi32 (load4 (g->cand[8]), shift)))
            >> b)
           & 1u)
          << 8;

  return mask;
}

/* Drop digit d (0-8) from the peers of bit pos of band b and mark the
 * cell as filled.  Enough by itself when d is the cell's only candidate. */
static inline void
place_single (board *g, int d, int b, int pos)
{
  const int i = 27 * b + pos;
  const __m128i cell = load4 (cell_bits[i]);

  store4 (g->cand[d], _mm_or_si128 (_mm_andnot_si128 (load4 (peer_bits[i]),
                                                      load4 (g->cand[d])),
                                    cell));
  store4 (g->open, _mm_andnot_si128 (cell, load4 (g->open)));
  g->changed |= 1u << d;
}

/* Put digit d (0-8) into bit pos of band b: remove all other digits from
 * the cell, then d from its peers */
static inline void
place (board *g, int d, int b, int pos)
{
  const __m128i cell = load4 (cell_bits[27 * b + pos]);
  const __m256i cell2 = _mm256_broadcastsi128_si256 (cell);

  g->changed |= cell_digits (g, b, pos);
  for (int k = 0; k < 4; k++)
    store8 (g->cand[2 * k],
            _mm256_andnot_si256 (cell2, load8 (g->cand[2 * k])));
  store4 (g->cand[8], _mm_andnot_si128 (cell, load4 (g->cand[8])));
  place_single (g, d, b, pos);
}

/* Fill naked singles (open cells with one candidate) until none are left.
 * Return 0 on a contradiction: an open cell without candidates. */
static int
naked_singles (board *g)
{
  for (;;)
    {
      __m256i ones2 = _mm256_setzero_si256 ();
      __m256i twos2 = _mm256_setzero_si256 ();
      __m256i bit1 = _mm256_setzero_si256 (); /* Digits 2, 3, 6, 7 */
      __m256i bit2 = _mm256_setzero_si256 (); /* Digits 4-7 */
      const __m128i x8 = load4 (g->cand[8]);
      const __m128i open = load4 (g->open);
      __m128i ones = _mm_setzero_si128 ();
      __m128i twos = _mm_setzero_si128 ();
      __m128i single = _mm_setzero_si128 ();
      uint32_t s[4];
      uint32_t plane[4][4]; /* Bits of the digit index, per band */

      /* Count candidates per cell, separately for even and odd digits in
       * the two halves, then merge the halves and add digit 8 */
      for (int k = 0; k < 4; k++)
        {
          const __m256i x = load8 (g->cand[2 * k]);

          twos2 = _mm256_or_si256 (twos2, _mm256_and_si256 (ones2, x));
          ones2 = _mm256_or_si256 (ones2, x);
          if (k & 1)
            bit1 = _mm256_or_si256 (bit1, x);
          if (k & 2)
            bit2 = _mm256_or_si256 (bit2, x);
        }

      {
        const __m128i lo = _mm256_castsi256_si128 (ones2);
        const __m128i hi = _mm256_extracti128_si256 (ones2, 1);

        ones = _mm_or_si128 (lo, hi);
        twos = _mm_or_si128 (
            _mm_or_si128 (_mm256_castsi256_si128 (twos2),
                          _mm256_extracti128_si256 (twos2, 1)),
            _mm_and_si128 (lo, hi));
        twos = _mm_or_si128 (twos, _mm_and_si128 (ones, x8));
        ones = _mm_or_si128 (ones, x8);

        /* Odd digits are exactly the upper halves */
        store4 (plane[0], hi);
      }
      store4 (plane[1], _mm_or_si128 (_mm256_castsi256_si128 (bit1),
                                      _mm256_extracti128_si256 (bit1, 1)));
      store4 (plane[2], _mm_or_si128 (_mm256_castsi256_si128 (bit2),
                                      _mm256_extracti128_si256 (bit2, 1)));
      store4 (plane[3], x8);

      if (!_mm_testc_si128 (ones, open))
        return 0;

      single = _mm_andnot_si128 (twos, open);
      if (_mm_testz_si128 (single, single))
        return 1;

      /* In a cell with a single candidate the planes spell its digit.
       * Two singles may compete for the same digit in a unit: the loser
       * is left without candidates, a contradiction caught in the next
       * round. */
      store4 (s, single);
      for (int b = 0; b < 3; b++)
        for (uint32_t w = s[b]; w; w &= w - 1)
          {
            const int pos = bit_index (w);
            const int d = (int)(((plane[0][b] >> pos) & 1u)
                                | ((plane[1][b] >> pos) & 1u) << 1
                                | ((plane[2][b] >> pos) & 1u) << 2
                                | ((plane[3][b] >> pos) & 1u) << 3);

            if ((g->cand[d][b] >> pos) & 1u)
              place_single (g, d, b, pos);
          }
    }
}

/* Transpose the 3-bit fields of lanes 0-2 of each 128-bit half: field t
 * of lane j becomes field j of lane t */
static inline __m256i
transpose3 (__m256i v)
{
  const __m256i shift = _mm256_setr_epi32 (0, 3, 6, 0, 0, 3, 6, 0);
  const __m256i seven = _mm256_set1_epi32 (7);
  const __m256i f0
      = _mm256_and_si256 (_mm256_srlv_epi32 (_mm256_shuffle_epi32 (v, 0x00),
                                             shift),
                          seven);
  const __m256i f1
      = _mm256_and_si256 (_mm256_srlv_epi32 (_mm256_shuffle_epi32 (v, 0x55),
                                             shift),
                          seven);
  const __m256i f2
      = _mm256_and_si256 (_mm256_srlv_epi32 (_mm256_shuffle_epi32 (v, 0xAA),
                                             shift),
                          seven);

  return _mm256_or_si256 (
      f0, _mm256_or_si256 (_mm256_slli_epi32 (f1, 3),
                           _mm256_slli_epi32 (f2, 6)));
}

/* Keep in every lane only the cells whose bits are clear after clearing
 * the lowest one: rows or boxes with a single cell (or none) */
static inline __m256i
singles_of (__m256i unit)
{
  const __m256i lower = _mm256_and_si256 (
      unit, _mm256_sub_epi32 (unit, _mm256_set1_epi32 (1)));

  return _mm256_and_si256 (
      unit, _mm256_cmpeq_epi32 (lower, _mm256_setzero_si256 ()));
}

/* For every changed digit, drop the cells that fit no band or stack
 * pattern, then fill its hidden singles: cells that are the only place
 * left for the digit in a row, column or box.  Return 1 if anything
 * changed, 0 if nothing did, or -1 on a contradiction: a digit with no
 * possible cell in some unit, or two hidden singles that exclude each
 * other.  Works on two digits at a time. */
static int
hidden_singles (board *g)
{
  const __m256i row_bits = _mm256_set1_epi32 (ROW_BITS);
  int found = 0;

  for (int d = 0; d < 9; d += 2)
    {
      const unsigned pair = (g->changed >> d) & (d < 8 ? 3u : 1u);
      const unsigned valid = d < 8 ? 0x77u : 0x07u; /* Lanes that count */
      __m256i x;
      __m256i nx;
      __m256i hidden;

      /* Unchanged since their last check.  Removing cells below does not
       * mark the digits again: rechecking costs more than it prunes, and
       * whatever is missed now is caught later without affecting
       * correctness. */
      if (!pair)
        continue;
      g->changed &= ~(pair << d);

      x = d < 8 ? load8 (g->cand[d])
                : _mm256_inserti128_si256 (_mm256_setzero_si256 (),
                                           load4 (g->cand[8]), 0);

      /* Band patterns: mini-rows per band, looked up in band_mask */
      {
        const __m256i t = _mm256_and_si256 (
            _mm256_or_si256 (x, _mm256_or_si256 (_mm256_srli_epi32 (x, 1),
                                                 _mm256_srli_epi32 (x, 2))),
            _mm256_set1_epi32 (0x1249249));
        const __m256i y = _mm256_or_si256 (
            t, _mm256_or_si256 (_mm256_srli_epi32 (t, 2),
                                _mm256_srli_epi32 (t, 4)));
        const __m256i m = _mm256_or_si256 (
            _mm256_and_si256 (y, _mm256_set1_epi32 (7)),
            _mm256_or_si256 (
                _mm256_and_si256 (_mm256_srli_epi32 (y, 6),
                                  _mm256_set1_epi32 (0x38)),
                _mm256_and_si256 (_mm256_srli_epi32 (y, 12),
                                  _mm256_set1_epi32 (0x1C0))));
        const __m256i keep
            = _mm256_i32gather_epi32 ((const int *)band_mask, m, 4);

        if (zero_lanes (keep) & valid)
          return -1;
        nx = _mm256_and_si256 (x, keep);
      }

      /* Stack patterns: columns per band, regrouped per stack, looked up
       * in stack_mask and regrouped back per band */
      {
        const __m256i col = _mm256_and_si256 (
            _mm256_or_si256 (nx,
                             _mm256_or_si256 (_mm256_srli_epi32 (nx, 9),
                                              _mm256_srli_epi32 (nx, 18))),
            row_bits);
        const __m256i fit = _mm256_i32gather_epi32 (
            (const int *)stack_mask, transpose3 (col), 4);
        const __m256i keep = transpose3 (fit);

        if (zero_lanes (fit) & valid)
          return -1;
        nx = _mm256_and_si256 (
            nx, _mm256_or_si256 (
                    keep, _mm256_or_si256 (_mm256_slli_epi32 (keep, 9),
                                           _mm256_slli_epi32 (keep, 18))));
      }

      if (!_mm256_testc_si256 (nx, x))
        {
          found = 1;
          if (d < 8)
            store8 (g->cand[d], nx);
          else
            store4 (g->cand[8], _mm256_castsi256_si128 (nx));
        }

      /* Rows and boxes lie within one band */
      hidden = _mm256_or_si256 (
          _mm256_or_si256 (
              singles_of (_mm256_and_si256 (nx, row_bits)),
              singles_of (_mm256_and_si256 (
                  nx, _mm256_set1_epi32 (ROW_BITS << 9)))),
          singles_of (_mm256_and_si256 (nx,
                                        _mm256_set1_epi32 (ROW_BITS << 18))));
      hidden = _mm256_or_si256 (
          hidden,
          _mm256_or_si256 (
              _mm256_or_si256 (
                  singles_of (
                      _mm256_and_si256 (nx, _mm256_set1_epi32 (BOX_BITS))),
                  singles_of (_mm256_and_si256 (
                      nx, _mm256_set1_epi32 (BOX_BITS << 3)))),
              singles_of (_mm256_and_si256 (
                  nx, _mm256_set1_epi32 (BOX_BITS << 6)))));

      /* Columns: count per column over the three rows of each band, then
       * over the three bands */
      {
        const __m256i s0 = _mm256_and_si256 (nx, row_bits);
        const __m256i s1
            = _mm256_and_si256 (_mm256_srli_epi32 (nx, 9), row_bits);
        const __m256i s2 = _mm256_srli_epi32 (nx, 18);
        const __m256i o = _mm256_or_si256 (s0, _mm256_or_si256 (s1, s2));
        const __m256i t = _mm256_or_si256 (
            _mm256_and_si256 (s0, s1),
            _mm256_and_si256 (s2, _mm256_or_si256 (s0, s1)));
        const __m256i o0 = _mm256_shuffle_epi32 (o, 0x00);
        const __m256i o1 = _mm256_shuffle_epi32 (o, 0x55);
        const __m256i o2 = _mm256_shuffle_epi32 (o, 0xAA);
        const __m256i ones = _mm256_or_si256 (o0, _mm256_or_si256 (o1, o2));
        const __m256i twos = _mm256_or_si256 (
            _mm256_or_si256 (
                _mm256_shuffle_epi32 (t, 0x00),
                _mm256_or_si256 (_mm256_shuffle_epi32 (t, 0x55),
                                 _mm256_shuffle_epi32 (t, 0xAA))),
            _mm256_or_si256 (
                _mm256_and_si256 (o0, o1),
                _mm256_and_si256 (o2, _mm256_or_si256 (o0, o1))));
        const __m256i cols = _mm256_andnot_si256 (twos, ones);

        if (~(unsigned)_mm256_movemask_ps (_mm256_castsi256_ps (
                _mm256_cmpeq_epi32 (ones, row_bits)))
            & valid)
          return -1;

        hidden = _mm256_or_si256 (
            hidden,
            _mm256_and_si256 (
                nx, _mm256_or_si256 (
                        cols, _mm256_or_si256 (_mm256_slli_epi32 (cols, 9),
                                               _mm256_slli_epi32 (cols, 18)))));
      }

      /* A placed digit keeps its cell, so drop filled cells */
      hidden = _mm256_and_si256 (
          hidden, _mm256_broadcastsi128_si256 (load4 (g->open)));

      if (!_mm256_testz_si256 (hidden, hidden))
        {
          uint32_t h[8];

          store8 (h, hidden);
          for (int j = 0; j < 2 && d + j < 9; j++)
            for (int b = 0; b < 3; b++)
              for (uint32_t w = h[4 * j + b]; w; w &= w - 1)
                {
                  const int pos = bit_index (w);

                  /* An earlier single took the cell or excluded the
                   * digit */
                  if (!(((g->open[b] & g->cand[d + j][b]) >> pos) & 1u))
                    return -1;
                  place (g, d + j, b, pos);
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
  const __m128i open = load4 (g->open);
  __m128i ones = _mm_setzero_si128 ();
  __m128i twos = _mm_setzero_si128 ();
  __m128i threes = _mm_setzero_si128 ();
  __m128i fours = _mm_setzero_si128 ();
  uint32_t two[4];
  uint32_t three[4];

  if (_mm_testz_si128 (open, open))
    return 0;

  for (int d = 0; d < 9; d++)
    {
      const __m128i x = load4 (g->cand[d]);

      fours = _mm_or_si128 (fours, _mm_and_si128 (threes, x));
      threes = _mm_or_si128 (threes, _mm_and_si128 (twos, x));
      twos = _mm_or_si128 (twos, _mm_and_si128 (ones, x));
      ones = _mm_or_si128 (ones, x);
    }

  store4 (two, _mm_andnot_si128 (threes, open));
  store4 (three, _mm_andnot_si128 (fours, _mm_and_si128 (threes, open)));

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
