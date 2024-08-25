#include <omp.h>
#include <stdio.h>

int
main ()
{
  int max_threads = omp_get_max_threads ();
  printf ("Max number of threads:\t%d\n", max_threads);

#pragma omp parallel thread_num(4)
  {
    printf ("Thread id\t%d\n", omp_get_thread_num ());
  }
  return 0;
}