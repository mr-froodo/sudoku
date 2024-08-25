#ifndef SUDOKU_H
#define SUDOKU_H

struct _sudoku
{
  char *a;       /* array containing values */
  int rotations; /* Count how many times sudoku has been rotated */
};

typedef struct _sudoku sudoku;
typedef sudoku *psudoku;
typedef const sudoku *pcsudoku;

/* Constructors */

psudoku new_sudoku ();

psudoku init_sudoku ();

psudoku create_empty_sudoku ();

void del_sudoku ();

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

void show_sudoku ();

/* Solve */

void prepare_sudoku (psudoku s);

/* Rotate left */
void rotate_sudoku (psudoku s);

int check_row (psudoku s, int n);

int check_col (psudoku s, int n);

int check_box (psudoku s, int n);

int check_position (psudoku s, int n);

int solve_sudoku (psudoku s);

#endif