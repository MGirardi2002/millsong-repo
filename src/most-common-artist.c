#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <stddef.h> // Necessário para a função offsetof

#define MAX_LINE 4096
#define MAX_ARTISTS 10000
#define MAX_NAME 256

// A struct permanece a mesma
typedef struct {
    char name[MAX_NAME];
    int count;
} ArtistCount;

// Função auxiliar para encontrar um artista na lista (sem alterações)
int find_artist(ArtistCount *artists, int num_artists, const char *name) {
    for (int i = 0; i < num_artists; i++) {
        if (strcmp(artists[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Função para criar o tipo de dado MPI para a struct ArtistCount
void create_artist_count_mpi_type(MPI_Datatype *new_type) {
    // A struct tem 2 "blocos": um array de char e um int
    int blocklengths[2] = {MAX_NAME, 1};
    
    // Tipos de cada bloco
    MPI_Datatype types[2] = {MPI_CHAR, MPI_INT};
    
    // Calcula o deslocamento (offset) de cada membro dentro da struct
    MPI_Aint displacements[2];
    displacements[0] = offsetof(ArtistCount, name);
    displacements[1] = offsetof(ArtistCount, count);
    
    // Cria o tipo de dado estruturado
    MPI_Type_create_struct(2, blocklengths, displacements, types, new_type);
    
    // "Commita" o tipo para que possa ser usado em comunicações
    MPI_Type_commit(new_type);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // --- MELHORIA 1: Usar argumento de linha de comando para o arquivo ---
    if (argc < 2) {
        if (rank == 0) {
            fprintf(stderr, "Uso: mpirun -np <num_procs> %s <caminho_para_o_csv>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        if (rank == 0) perror("Erro ao abrir o arquivo");
        MPI_Finalize();
        return 1;
    }

    // --- MELHORIA 3: Criar e commitar o tipo MPI para a struct ---
    MPI_Datatype artist_type;
    create_artist_count_mpi_type(&artist_type);

    ArtistCount local_artists[MAX_ARTISTS];
    int local_num_artists = 0;
    char line[MAX_LINE];
    int line_num = 0;
    
    // --- MELHORIA 2: Pular a linha de cabeçalho do CSV ---
    // Apenas o processo 0 lê e descarta o cabeçalho para evitar contá-lo.
    // O contador de linhas começa em 0 para que a primeira linha de dados seja processada.
    if (rank == 0) {
        fgets(line, MAX_LINE, file); 
    }
    // Sincroniza todos os processos para garantir que o cabeçalho foi lido antes de continuar
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Posição inicial do arquivo após o cabeçalho (aproximado)
    long long start_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    long long end_pos = ftell(file);
    fseek(file, start_pos, SEEK_SET);
    
    long long total_size = end_pos - start_pos;
    long long chunk_size = total_size / size;
    long long my_start = start_pos + rank * chunk_size;
    long long my_end = (rank == size - 1) ? end_pos : my_start + chunk_size;
    
    fseek(file, my_start, SEEK_SET);
    
    // Se não for o primeiro processo, avança até a próxima linha para não cortar uma linha ao meio
    if (rank > 0) {
        fgets(line, MAX_LINE, file);
    }
    
    while (ftell(file) < my_end) {
        if (!fgets(line, MAX_LINE, file)) break;

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
    fclose(file);

    int *recv_counts = NULL;
    ArtistCount *all_artists = NULL;
    if (rank == 0) {
        recv_counts = malloc(size * sizeof(int));
    }
    
    MPI_Gather(&local_num_artists, 1, MPI_INT, recv_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int total_artists_gathered = 0;
    int *displs = NULL;
    if (rank == 0) {
        displs = malloc(size * sizeof(int));
        displs[0] = 0;
        total_artists_gathered = recv_counts[0];
        for (int i = 1; i < size; i++) {
            displs[i] = displs[i - 1] + recv_counts[i - 1];
            total_artists_gathered += recv_counts[i];
        }
        all_artists = malloc(total_artists_gathered * sizeof(ArtistCount));
    }

    // --- MELHORIA 3 (continuação): Usar o tipo customizado na comunicação ---
    MPI_Gatherv(local_artists, local_num_artists, artist_type,
                all_artists, recv_counts, displs, artist_type,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        // A lógica de consolidação final está ótima, mantida como estava.
        ArtistCount final_artists[MAX_ARTISTS];
        int final_num_artists = 0;
        for (int i = 0; i < total_artists_gathered; i++) {
            int idx = find_artist(final_artists, final_num_artists, all_artists[i].name);
            if (idx == -1 && final_num_artists < MAX_ARTISTS) {
                final_artists[final_num_artists] = all_artists[i];
                final_num_artists++;
            } else if (idx != -1) {
                final_artists[idx].count += all_artists[i].count;
            }
        }

        // Encontrar os artistas com mais músicas
        int max_count = 0;
        for (int i = 0; i < final_num_artists; i++) {
            if (final_artists[i].count > max_count) {
                max_count = final_artists[i].count;
            }
        }   
        
        printf("Artista(s) com mais músicas (%d músicas):\n", max_count);
        for (int i = 0; i < final_num_artists; i++) {
            if (final_artists[i].count == max_count) {
                printf("- %s\n", final_artists[i].name);
            }
        }

        free(recv_counts);
        free(displs);
        free(all_artists);
    }
    
    // Libera o tipo de dado MPI customizado
    MPI_Type_free(&artist_type);
    MPI_Finalize();
    return 0;
}
