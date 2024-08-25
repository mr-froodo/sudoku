#ifndef SUDOKU_H
#define SUDOKU_H

struct _sudoku
{
  char *a; /* array containing values */
};

typedef struct _sudoku sudoku;
typedef sudoku *psudoku;

/* Constructors */

psudoku new_sudoku ();

psudoku init_sudoku ();

psudoku create_empty_sudoku ();

void del_sudoku ();

/* Access methods */
void set_sudoku (psudoku s, const char *cs);

/* I/O */

void show_sudoku ();

/* Solve */

void prepare_sudoku (psudoku s);

int check_row (psudoku s, int n);

int check_col (psudoku s, int n);

int check_box (psudoku s, int n);

int check_position (psudoku s, int n);

int solve_sudoku (psudoku s);

#endif