#include <stdio.h>
#include <omp.h>
#include <time.h>
#define N 1000000000

int main(int argc, char *argv[]){
  double x, final;
  int i, th_id, nthreads;

  final = 1;
  x = 1 + 1.0/N;

  clock_t start = clock();
  omp_set_num_threads(8);

#pragma omp parallel default(none) private(i, th_id, nthreads) shared(x) reduction(*:final)
  {
    th_id = omp_get_thread_num();
    if (th_id == 0) {
        nthreads = omp_get_num_threads();
        printf("There are %d threads\n", nthreads);
    }
#pragma omp for
    for(i=0; i<N; i++) {
      final = final*x;
    }
  }
  clock_t end = clock();

  printf("Resultado=%lf\n",final);

  double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  printf("Tempo de execução (paralelizado): %f segundos\n", time_spent);
  return 0;
}
