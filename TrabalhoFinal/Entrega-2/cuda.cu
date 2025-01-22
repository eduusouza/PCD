#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda.h>

#define N 4000  // Tamanho da grade
#define T 500   // Número de iterações no tempo
#define D 0.1   // Coeficiente de difusão
#define DELTA_T 0.01
#define DELTA_X 1.0

// Kernel CUDA para calcular a nova matriz de concentração
__global__ void diff_eq_kernel(double *C, double *C_new, double *difmedio, int n) {
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    int j = blockIdx.x * blockDim.x + threadIdx.x;

    if (i > 0 && i < n - 1 && j > 0 && j < n - 1) {
        C_new[i * n + j] = C[i * n + j] + D * DELTA_T * (
            (C[(i + 1) * n + j] + C[(i - 1) * n + j] +
             C[i * n + (j + 1)] + C[i * n + (j - 1)] -
             4.0 * C[i * n + j]) / (DELTA_X * DELTA_X)
        );

        atomicAdd(difmedio, fabs(C_new[i * n + j] - C[i * n + j]));
    }
}

// Função principal
int main() {
    size_t size = N * N * sizeof(double);

    // Alocar memória no host (CPU)
    double *C = (double *)malloc(size);
    double *C_new = (double *)malloc(size);

    if (C == NULL || C_new == NULL) {
        fprintf(stderr, "Falha na alocação de memória no host\n");
        return 1;
    }

    // Inicializar as matrizes no host
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i * N + j] = 0.0;
            C_new[i * N + j] = 0.0;
        }
    }
    C[N / 2 * N + N / 2] = 1.0; // Concentração inicial no centro

    // Alocar memória no dispositivo (GPU)
    double *d_C, *d_C_new, *d_difmedio;
    cudaMalloc((void **)&d_C, size);
    cudaMalloc((void **)&d_C_new, size);
    cudaMalloc((void **)&d_difmedio, sizeof(double));

    // Copiar dados do host para o dispositivo
    cudaMemcpy(d_C, C, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_C_new, C_new, size, cudaMemcpyHostToDevice);

    dim3 threadsPerBlock(16, 16);
    dim3 blocksPerGrid((N + threadsPerBlock.x - 1) / threadsPerBlock.x,
                       (N + threadsPerBlock.y - 1) / threadsPerBlock.y);

    // Loop de iterações no tempo
    double start_time = clock();
    for (int t = 0; t < T; t++) {
        double difmedio = 0.0;
        cudaMemcpy(d_difmedio, &difmedio, sizeof(double), cudaMemcpyHostToDevice);

        // Executar o kernel
        diff_eq_kernel<<<blocksPerGrid, threadsPerBlock>>>(d_C, d_C_new, d_difmedio, N);
        cudaDeviceSynchronize();

        // Trocar os ponteiros
        double *temp = d_C;
        d_C = d_C_new;
        d_C_new = temp;

        // Obter difmedio do dispositivo
        cudaMemcpy(&difmedio, d_difmedio, sizeof(double), cudaMemcpyDeviceToHost);

        if ((t % 100) == 0) {
            printf("Interação %d - diferença = %g\n", t, difmedio / ((N - 2) * (N - 2)));
        }
    }
    double end_time = clock();

    // Copiar os resultados finais para o host
    cudaMemcpy(C, d_C, size, cudaMemcpyDeviceToHost);

    printf("Concentração final no centro: %f\n", C[N / 2 * N + N / 2]);
    printf("Tempo de execução: %f segundos\n", (end_time - start_time) / CLOCKS_PER_SEC);

    // Liberar memória no dispositivo e no host
    cudaFree(d_C);
    cudaFree(d_C_new);
    cudaFree(d_difmedio);
    free(C);
    free(C_new);

    return 0;
}
