#ifndef SUDOKU_GRID_H
#define SUDOKU_GRID_H

typedef struct sudoku
{
  char *a;       /* 81 cell values ('0' = empty) followed by 81 fixed flags */
  int rotations; /* Count how many times sudoku has been rotated */
} sudoku;

/* Constructors */

sudoku *new_sudoku (void);

sudoku *init_sudoku (sudoku *s);

sudoku *create_empty_sudoku (void);

/* Free a sudoku created by new_sudoku or create_empty_sudoku */
void del_sudoku (sudoku *s);

void copy_sudoku (const sudoku *s, sudoku *t);

/* Access methods */
void set_sudoku (sudoku *s, const char *cs);

/* Locate where most entries are to be found
 *
 * Return code:
 * 0 upper half
 * 1 right half
 * 2 lower half
 * 3 left half
 */
int sudoku_locate_entries (const sudoku *s);

/* I/O */

void show_sudoku (const sudoku *s);

/* Transformations */

/* Mark every non-empty cell as fixed */
void prepare_sudoku (sudoku *s);

/* Rotate left */
void rotate_sudoku (sudoku *s);

#endif
