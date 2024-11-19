#include <stdlib.h>
#include <omp.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#define n 300000

/* Exemplo 3 */
/* Laco perfeitamente paralelizavel */

int main() {

  long int i,j;
  double a[n], b[n];
  double start, end, run, start_parallel;
  start = omp_get_wtime();

  for (i=0; i<n; i++) a[i] = (double)(i*2)/(i+6);

  omp_set_num_threads(1);
  start_parallel = omp_get_wtime();

#pragma omp parallel private(j)
#pragma omp for
  for (i=0; i<n; i++) 
    for(j=0; j<n; j++)
       b[i]= (a[i] - a[i-1])*0.5;
/* end parallel for */

  end = omp_get_wtime();
    printf(" took %f seconds total and %f seconds for parallel time.\n", end-start, end-start_parallel);

}