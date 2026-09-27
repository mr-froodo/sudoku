#ifndef SUDOKU_GRID_H
#define SUDOKU_GRID_H

struct _sudoku
{
  char *a;       /* 81 cell values ('0' = empty) followed by 81 fixed flags */
  int rotations; /* Count how many times sudoku has been rotated */
};

typedef struct _sudoku sudoku;
typedef sudoku *psudoku;
typedef const sudoku *pcsudoku;

/* Constructors */

psudoku new_sudoku (void);

psudoku init_sudoku (psudoku s);

psudoku create_empty_sudoku (void);

/* Free a sudoku created by new_sudoku or create_empty_sudoku */
void del_sudoku (psudoku s);

void copy_sudoku (pcsudoku s, psudoku t);

/* Access methods */
void set_sudoku (psudoku s, const char *cs);

/* Locate where most entries are to be found
 *
 * Return code:
 * 0 upper half
 * 1 right half
 * 2 lower half
 * 3 left half
 */
int sudoku_locate_entries (psudoku s);

/* I/O */

void show_sudoku (psudoku s);

/* Transformations */

/* Mark every non-empty cell as fixed */
void prepare_sudoku (psudoku s);

/* Rotate left */
void rotate_sudoku (psudoku s);

#endif
