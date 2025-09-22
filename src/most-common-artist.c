#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_LINE 4096
#define MAX_ARTISTS 10000
#define MAX_NAME 256

typedef struct {
    char name[MAX_NAME];
    int count;
} ArtistCount;

int find_artist(ArtistCount *artists, int num_artists, const char *name) {
    for (int i = 0; i < num_artists; i++) {
        if (strcmp(artists[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
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

    ArtistCount local_artists[MAX_ARTISTS];
    int local_num_artists = 0;

    char line[MAX_LINE];
    int line_num = 0;

    // Assume que o nome do artista está na primeira coluna do CSV
    while (fgets(line, MAX_LINE, file)) {
        if (line_num % size == rank) {
            char *artist = strtok(line, ",");
            if (artist) {
                int idx = find_artist(local_artists, local_num_artists, artist);
                if (idx == -1 && local_num_artists < MAX_ARTISTS) {
                    strncpy(local_artists[local_num_artists].name, artist, MAX_NAME - 1);
                    local_artists[local_num_artists].name[MAX_NAME - 1] = '\0';
                    local_artists[local_num_artists].count = 1;
                    local_num_artists++;
                } else if (idx != -1) {
                    local_artists[idx].count++;
                }
            }
        }
        line_num++;
    }
    fclose(file);

    // Preparar para enviar resultados ao rank 0
    int *recv_counts = NULL;
    ArtistCount *all_artists = NULL;
    if (rank == 0) {
        recv_counts = malloc(size * sizeof(int));
    }
    MPI_Gather(&local_num_artists, 1, MPI_INT, recv_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int total_artists = 0;
    int *displs = NULL;
    if (rank == 0) {
        displs = malloc(size * sizeof(int));
        displs[0] = 0;
        for (int i = 1; i < size; i++) {
            displs[i] = displs[i - 1] + recv_counts[i - 1];
        }
        total_artists = displs[size - 1] + recv_counts[size - 1];
        all_artists = malloc(total_artists * sizeof(ArtistCount));
    }

    MPI_Gatherv(local_artists, local_num_artists, MPI_BYTE,
                all_artists, recv_counts, displs, MPI_BYTE,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Consolidar contagem de artistas
        ArtistCount final_artists[MAX_ARTISTS * size];
        int final_num_artists = 0;
        for (int i = 0; i < total_artists; i++) {
            int idx = find_artist(final_artists, final_num_artists, all_artists[i].name);
            if (idx == -1 && final_num_artists < MAX_ARTISTS * size) {
                strncpy(final_artists[final_num_artists].name, all_artists[i].name, MAX_NAME - 1);
                final_artists[final_num_artists].name[MAX_NAME - 1] = '\0';
                final_artists[final_num_artists].count = all_artists[i].count;
                final_num_artists++;
            } else if (idx != -1) {
                final_artists[idx].count += all_artists[i].count;
            }
        }

        // Encontrar os artistas com mais músicas
        int max = 0;
        for (int i = 0; i < final_num_artists; i++) {
            if (final_artists[i].count > max) {
                max = final_artists[i].count;
            }
        }
        printf("Artistas com mais músicas (%d músicas):\n", max);
        for (int i = 0; i < final_num_artists; i++) {
            if (final_artists[i].count == max) {
                printf("%s\n", final_artists[i].name);
            }
        }

        free(recv_counts);
        free(displs);
        free(all_artists);
    }

    MPI_Finalize();
    return