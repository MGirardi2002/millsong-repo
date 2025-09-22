#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_LINE 4096

int count_words(const char *line) {
    int count = 0;
    int in_word = 0;
    for (int i = 0; line[i]; i++) {
        if ((line[i] != ' ') && (line[i] != '\n') && (line[i] != ',')) {
            if (!in_word) {
                count++;
                in_word = 1;
            }
        } else {
            in_word = 0;
        }
    }
    return count;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    FILE *file = fopen("../utils/spotify_millsongdata.csv", "r");
    if (!file) {
        if (rank == 0) printf("Erro ao abrir o arquivo.\n");
        MPI_Finalize();
        return 1;
    }

    int local_count = 0;
    char line[MAX_LINE];
    int line_num = 0;

    while (fgets(line, MAX_LINE, file)) {
        if (line_num % size == rank) {
            local_count += count_words(line);
        }
        line_num++;
    }
    fclose(file);

    int total_count = 0;
    MPI_Reduce(&local_count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Total de palavras: %d\n", total_count);
    }

    MPI_Finalize();
    return 0;
}