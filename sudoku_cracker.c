#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Print Sudoku grid */
int print_sudoku (char *a);

void print_structure (char *m);

/*Read sudoku from textfile */
char *read_sudoku (const char *filname);

/* Create empty sudoku */
char *new_empty_sudoku ();

int cell_isEmpty (char *m, int n);

int check_position (char *a, int n);

/* Check Sudoku row
 *
 * Input:
 * Grid a
 * Position n
 *
 * Return:
 * 0 if check is succesful and non zero otherwise.
 */
int check_row (char *a, int n);

int check_col (char *a, int n);

int check_box (char *a, int n);

int
main ()
{
  /* Sudokua grid */
  // char a[81] = {
  //   '9', '0', '3', '0', '0', '8', '4', '0', '0', //
  //   '0', '5', '0', '1', '0', '7', '0', '0', '3', //
  //   '6', '8', '7', '3', '0', '2', '9', '0', '0', //
  //   //
  //   '0', '0', '0', '6', '0', '0', '0', '0', '0', //
  //   '5', '3', '8', '2', '0', '0', '0', '0', '0', //
  //   '0', '0', '0', '0', '0', '0', '0', '0', '0', //
  //   //
  //   '0', '0', '0', '0', '0', '0', '0', '0', '0', //
  //   '0', '0', '0', '0', '0', '0', '0', '0', '0', //
  //   '0', '0', '0', '0', '0', '0', '0', '0', '0'  //
  // };

  clock_t begin, end;
  double time_spent;

  char *a;
  char m[81];
  int n, steps;

  printf ("==============================\n"
          "Cracking Sudoku...\n");

  printf ("==============================\n"
          "Original Sudoku:\n\n");

  // a = read_sudoku ("sudoku.txt");
  a = new_empty_sudoku ();
  print_sudoku (a);

  /* Prepare */
  {
    int i;
    for (i = 0; i < 81; i++)
      if (a[i] == '0')
        m[i] = 1;
      else
        m[i] = 0;
  }

  /* Check if solveable at all */
  {
    int i;
    for (i = 0; i < 81; i++)
      if (!cell_isEmpty (m, i))
        if (check_position (a, i))
          {
            printf ("==============================\n"
                    "Sudoku is corrupted.\n"
                    "Error @ (%d, %d)\n"
                    "No solutions exist.\n",
                    i / 9 + 1, i % 9 + 1);
            free (a);
            return 0;
          }
  }

  steps = 0; /* Init step count */

  /* Solve sudoku */
  begin = clock ();
  n = 0;
  while (n < 81)
    {
      if (cell_isEmpty (m, n))
        {
          /* Cell has to be filled */
          if (a[n] == '0')
            {
              /* Cell is empty --> Start with 1 */
              a[n] = '1';
              steps++;
            }
          else
            {
              /* Cell already filled --> Increase value */
              a[n] += 1;
              steps++;
            }

          /* While checks fail and c<=9*/
          while (check_position (a, n) && a[n] <= '9')
            {
              a[n] += 1;
              steps++;
            }

          /* Go back if all checks have failed */
          if (a[n] == '9' + 1)
            {
              /* Clear cell and go back */
              a[n] = '0';
              n--;
              while (!cell_isEmpty (m, n))
                n--;
            }
          else
            {
              /* Move on */
              n++;
            }
        }
      else
        {
          /* Move on */
          n++;
        }
    }

  end = clock ();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

  printf ("==============================\n");

  if (n > 81)
    printf ("Sorry, no solution found.\n");
  else
    {
      printf ("Success! Solved after %d steps.\n"
              "Time: %.2e\n",
              steps, time_spent);
      print_sudoku (a);
    }

  free (a);
  return 0;
}

int
print_sudoku (char *a)
{
  int i, j;
  for (i = 0; i < 9; i++)
    {
      for (j = 0; j < 9; j++)
        printf ("%c ", a[j + i * 9]);
      printf ("\n");
    }

  return 0;
}

char *
read_sudoku (const char *filename)
{
  FILE *fp;
  int i, j;
  char buffer[256];
  char *a; /* Sudoku grid */

  a = malloc (81);
  fp = fopen (filename, "r");

  for (i = 0; i < 9; i++)
    {
      fgets (buffer, 256, fp);
      for (j = 0; j < 9; j++)
        {
          a[j + i * 9] = buffer[j];
        }
    }

  fclose (fp);
  return a;
}

void
print_structure (char *m)
{
  int i, j;
  for (i = 0; i < 9; i++)
    {
      for (j = 0; j < 9; j++)
        {
          if (m[j + i * 9])
            printf ("O");
          else
            printf ("X");
        }
      printf ("\n");
    }
}

int
cell_isEmpty (char *m, int n)
{
  if (m[n])
    return 1;
  else
    return 0;
}

char *
new_empty_sudoku ()
{
  int i;
  char *a;

  a = malloc (81);

  for (i = 0; i < 81; i++)
    {
      a[i] = '0';
    }

  return a;
}

int
check_position (char *a, int n)
{
  int r, c, b;

  r = check_row (a, n);
  c = check_col (a, n);
  b = check_box (a, n);

  if (r + c + b)
    return 1;
  else
    return 0;
}

int
check_row (char *a, int n)
{
  int i;  /* Index of first entry in row */
  int k;  /* Counter */
  char c; /* Value of entry to check */

  /* Store value of entry */
  c = *(a + n);

  /* Compute index of firste entry in row */
  i = (n / 9) * 9; /* Integer devision! */

  /* Check against left entries */
  for (k = i; k < n; k++)
    if (c == *(a + k))
      return 1;

  /* Check against right etries */
  for (k = n + 1; k < i + 9; k++)
    if (c == *(a + k))
      return 1;

  /* All checks are passed */
  return 0;
}

int
check_col (char *a, int n)
{
  int j;  /* Index of first entry in col */
  int k;  /* Counter */
  char c; /* Value of entry to check */

  /* Store value of entry */
  c = *(a + n);

  /* Compute index of firste entry in col */
  j = n % 9;

  /* Check against above entries */
  for (k = j; k < n; k += 9)
    if (c == *(a + k))
      return 1;

  /* Check against below etries */
  for (k = n + 9; k < 81 + 9; k += 9)
    if (c == *(a + k))
      return 1;

  /* All checks are passed */
  return 0;
}

int
check_box (char *a, int n)
{
  int i, j, k; /* Index of top left entry of box */
  char c;      /* Value of position to check */

  c = *(a + n);

  i = n / 9; /* Row number */
  i /= 3;    /* Block row number */
  i *= 3;    /* Number of first row of block */

  j = n % 9; /* Col number */
  j /= 3;    /* Block col number */
  j *= 3;    /* Number of first col of block */

  k = i * 9 + j; /* Index of top left entry of block */

  /* Check against each entry of block */
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      if (k + j + i * 9 != n)
        if (*(a + (k + j + i * 9)) == c)
          return 1;
  /* All checks pased */
  return 0;
}