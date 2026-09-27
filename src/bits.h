#ifndef SUDOKU_BITS_H
#define SUDOKU_BITS_H

#include <stdint.h>

/* Bit helpers for digit masks and bitboards.
 *
 * Compiler builtins are used where available; otherwise a portable
 * fallback keeps the code plain C99. */

/* Index of the lowest set bit of a non-zero word */
static inline int
bit_index (uint32_t w)
{
#if defined(__GNUC__)
  return __builtin_ctz (w);
#else
  int i = 0;

  while (!(w & 1u))
    {
      w >>= 1;
      i++;
    }

  return i;
#endif
}

#endif
