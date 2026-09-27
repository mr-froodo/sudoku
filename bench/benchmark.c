#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sudoku/grid.h"
#include "sudoku/solver.h"

/* Solve every puzzle of a file (one per line, '#' starts a comment) and
 * verify each solution.
 *
 * By default every puzzle is timed on its own and listed.  With -s the
 * whole set is solved in passes and only the throughput of the fastest
 * pass is reported, which suits files with thousands of puzzles. */

/* Each puzzle (or pass with -s) is timed repeatedly for at least
 * MIN_TIME and at least MIN_RUNS times; the fastest run counts, which
 * filters out noise from other processes */
#define MIN_RUNS 5
#define MIN_TIME (CLOCKS_PER_SEC / 20)

typedef struct puzzle_set
{
  sudoku *puzzles;
  char (*text)[82];
  int count;
} puzzle_set;

/* Read all puzzles of in; return 0 on a malformed line or out of memory */
static int
read_puzzles (FILE *in, puzzle_set *set)
{
  char line[256];
  int capacity = 0;

  set->puzzles = NULL;
  set->text = NULL;
  set->count = 0;

  while (fgets (line, sizeof line, in))
    {
      line[strcspn (line, "\r\n#")] = '\0';
      if (!line[strspn (line, " \t")])
        continue;

      if (set->count == capacity)
        {
          const int grown = capacity ? 2 * capacity : 256;
          sudoku *p = realloc (set->puzzles, grown * sizeof *p);
          char (*t)[82] = p ? realloc (set->text, grown * sizeof *t) : NULL;

          if (p)
            set->puzzles = p;
          if (!t)
            return 0;
          set->text = t;
          capacity = grown;
        }

      if (!set_sudoku (&set->puzzles[set->count], line))
        {
          fprintf (stderr, "Malformed puzzle: %s\n", line);
          return 0;
        }
      memcpy (set->text[set->count], line, 82);
      set->count++;
    }

  return 1;
}

/* Solve puzzles [first, last) of set once; return the CPU time taken */
static double
solve_range (const puzzle_set *set, int first, int last)
{
  const clock_t begin = clock ();

  for (int i = first; i < last; i++)
    {
      sudoku s = set->puzzles[i];

      solve_sudoku (&s, NULL);
    }

  return (double)(clock () - begin) / CLOCKS_PER_SEC;
}

/* Fastest time for solving puzzles [first, last), see MIN_RUNS */
static double
fastest (const puzzle_set *set, int first, int last)
{
  double best = 0.0;
  double spent = 0.0;
  long runs = 0;
  int reps = 1;

  /* Batch fast runs so that each timing covers enough clock ticks */
  while (reps < 1 << 20 && solve_range (set, first, last) * reps < 1e-3)
    reps *= 2;

  while (runs < MIN_RUNS || spent < (double)MIN_TIME / CLOCKS_PER_SEC)
    {
      double t = 0.0;

      for (int r = 0; r < reps; r++)
        t += solve_range (set, first, last);
      t /= reps;

      if (!runs || t < best)
        best = t;
      spent += t * reps;
      runs++;
    }

  return best;
}

/* Solve and verify puzzle i; return 1 if solved correctly */
static int
check (const puzzle_set *set, int i, uint64_t *guesses)
{
  sudoku s = set->puzzles[i];

  return solve_sudoku (&s, guesses) && solves_sudoku (&set->puzzles[i], &s);
}

int
main (int argc, char *argv[])
{
  const int summary = argc > 1 && !strcmp (argv[1], "-s");
  const char *path = argc > 1 + summary ? argv[1 + summary] : NULL;
  FILE *in = path ? fopen (path, "r") : stdin;
  puzzle_set set;
  int failed = 0;
  uint64_t total_guesses = 0;

  if (argc > 2 + summary || !in)
    {
      fprintf (stderr,
               "Usage: %s [-s] [FILE]\n"
               "FILE holds one puzzle per line (default: stdin).\n"
               "-s reports only the throughput over all puzzles.\n",
               argv[0]);
      return 1;
    }

  if (!read_puzzles (in, &set))
    return 1;
  if (in != stdin)
    fclose (in);

  if (!summary)
    printf ("%5s  %10s  %10s  %s\n", "#", "guesses", "time [us]", "puzzle");

  for (int i = 0; i < set.count; i++)
    {
      uint64_t guesses = 0;
      const int ok = check (&set, i, &guesses);

      total_guesses += guesses;
      failed += !ok;
      if (!summary)
        printf ("%5d  %10llu  %10.2f  %s%s\n", i + 1,
                (unsigned long long)guesses, fastest (&set, i, i + 1) * 1e6,
                set.text[i], ok ? "" : "  FAILED");
      else if (!ok)
        printf ("FAILED: %s\n", set.text[i]);
    }

  if (set.count)
    {
      const double t = summary ? fastest (&set, 0, set.count) : 0.0;

      printf ("\n%d puzzles, %d failed, %.1f guesses per puzzle", set.count,
              failed, (double)total_guesses / set.count);
      if (summary)
        printf (", %.2f us per puzzle, %.0f puzzles/s", t / set.count * 1e6,
                set.count / t);
      printf ("\n");
    }

  free (set.puzzles);
  free (set.text);

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
