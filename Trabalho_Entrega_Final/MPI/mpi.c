#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define N 4000  // Tamanho da grade
#define T 500   // Número de iterações no tempo
#define D 0.1   // Coeficiente de difusão
#define DELTA_T 0.01
#define DELTA_X 1.0

// Função para calcular a nova matriz de concentração
void diff_eq(double *C, double *C_new, double *difmedio, int n, int start_row, int end_row) {
    double local_difmedio = 0.0;
    for (int i = 1; i < end_row - start_row - 1; i++) {
        for (int j = 1; j < n - 1; j++) {
            C_new[i * n + j] = C[i * n + j] + D * DELTA_T * (
                (C[(i + 1) * n + j] + C[(i - 1) * n + j] +
                 C[i * n + (j + 1)] + C[i * n + (j - 1)] -
                 4.0 * C[i * n + j]) / (DELTA_X * DELTA_X)
            );
            local_difmedio += fabs(C_new[i * n + j] - C[i * n + j]);
        }
    }
    *difmedio = local_difmedio;
}

// Função principal
int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Calcular o número de linhas por processo
    int rows_per_process = N / size;
    int leftover_rows = N % size;  // Linhas restantes após divisão igual
    int start_row, end_row;

    // Ajustar a divisão para distribuir as linhas restantes de maneira equilibrada
    if (rank < leftover_rows) {
        start_row = rank * (rows_per_process + 1);
        end_row = start_row + rows_per_process;
    } else {
        start_row = rank * rows_per_process + leftover_rows;
        end_row = start_row + rows_per_process - 1;
    }

    // Alocar memória para as linhas locais + 2 linhas fantasmas (bordas)
    int local_rows = end_row - start_row + 2;
    double *C = (double *)malloc(local_rows * N * sizeof(double));
    double *C_new = (double *)malloc(local_rows * N * sizeof(double));

    if (C == NULL || C_new == NULL) {
        fprintf(stderr, "Falha na alocação de memória no host\n");
        MPI_Finalize();
        return 1;
    }

    // Inicializar as matrizes no host
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < N; j++) {
            C[i * N + j] = 0.0;
            C_new[i * N + j] = 0.0;
        }
    }

    // Inicializar concentração no centro para um processo específico (ex: rank 0 ou rank size/2)
    if (rank == size / 2) {
        C[(N / 2 - start_row + 1) * N + N / 2] = 1.0; // Concentração inicial no centro
    }

    // Buffers para troca de bordas
    double *send_buffer_top = (double *)malloc(N * sizeof(double));
    double *recv_buffer_top = (double *)malloc(N * sizeof(double));
    double *send_buffer_bottom = (double *)malloc(N * sizeof(double));
    double *recv_buffer_bottom = (double *)malloc(N * sizeof(double));

    // Loop de iterações no tempo
    double start_time = MPI_Wtime();
    for (int t = 0; t < T; t++) {
        double difmedio = 0.0;

        // Trocar bordas com os processos vizinhos
        if (rank > 0) {
            MPI_Sendrecv(&C[N], N, MPI_DOUBLE, rank - 1, 0,
                         recv_buffer_top, N, MPI_DOUBLE, rank - 1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < N; j++) {
                C[j] = recv_buffer_top[j]; // Atualizar linha fantasma superior
            }
        }
        if (rank < size - 1) {
            MPI_Sendrecv(&C[(local_rows - 2) * N], N, MPI_DOUBLE, rank + 1, 0,
                         recv_buffer_bottom, N, MPI_DOUBLE, rank + 1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < N; j++) {
                C[(local_rows - 1) * N + j] = recv_buffer_bottom[j]; // Atualizar linha fantasma inferior
            }
        }

        // Calcular a nova matriz de concentração
        diff_eq(C, C_new, &difmedio, N, start_row, end_row);

        // Trocar os ponteiros
        double *temp = C;
        C = C_new;
        C_new = temp;

        // Reduzir a diferença média global
        double global_difmedio;
        MPI_Allreduce(&difmedio, &global_difmedio, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if ((t % 100) == 0 && rank == 0) {
            printf("Interação %d - diferença = %g\n", t, global_difmedio / ((N - 2) * (N - 2)));
        }
    }
    double end_time = MPI_Wtime();

    // Coletar os resultados finais no processo 0
    if (rank == 0) {
        double *C_final = (double *)malloc(N * N * sizeof(double));
        // Copiar os dados de C para C_final no processo 0
        for (int i = 0; i < local_rows - 1; i++) {
            for (int j = 0; j < N; j++) {
                C_final[(start_row + i) * N + j] = C[i * N + j];
            }
        }

        // Receber as partes finais dos outros processos
        for (int p = 1; p < size; p++) {
            int p_start_row = p * rows_per_process + (p < leftover_rows ? p : leftover_rows);
            int p_end_row = p_start_row + rows_per_process - 1;
            if (p == size - 1) {
                p_end_row = N;
            }
            MPI_Recv(&C_final[p_start_row * N], (p_end_row - p_start_row) * N, MPI_DOUBLE, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        printf("Concentração final no centro: %f\n", C_final[N / 2 * N + N / 2]);
        free(C_final);
    } else {
        // Enviar a parte local para o processo 0
        MPI_Send(&C[N], (local_rows - 2) * N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    printf("Processo %d - Tempo de execução: %f segundos\n", rank, end_time - start_time);

    // Liberar memória no host
    free(C);
    free(C_new);
    free(send_buffer_top);
    free(recv_buffer_top);
    free(send_buffer_bottom);
    free(recv_buffer_bottom);

    MPI_Finalize();
    return 0;
}
