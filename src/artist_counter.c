#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 100003
#define MAX_NAME 512
#define MAX_LINE 8192

typedef struct Node {
    char name[MAX_NAME];
    int count;
    struct Node* next;
} Node;

typedef struct {
    Node** table;
} HashTable;

// hash djb2
unsigned long hash(const char* str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) h = ((h << 5) + h) + c;
    return h % HASH_SIZE;
}

HashTable* ht_create() {
    HashTable* ht = malloc(sizeof(HashTable));
    ht->table = calloc(HASH_SIZE, sizeof(Node*));
    return ht;
}

void ht_insert(HashTable* ht, const char* key) {
    unsigned long idx = hash(key);
    Node* cur = ht->table[idx];
    while (cur) {
        if (strcmp(cur->name, key) == 0) {
            cur->count++;
            return;
        }
        cur = cur->next;
    }
    Node* n = malloc(sizeof(Node));
    strncpy(n->name, key, MAX_NAME - 1);
    n->name[MAX_NAME - 1] = '\0';
    n->count = 1;
    n->next = ht->table[idx];
    ht->table[idx] = n;
}

void ht_free(HashTable* ht) {
    for (int i = 0; i < HASH_SIZE; i++) {
        Node* cur = ht->table[i];
        while (cur) { Node* tmp = cur->next; free(cur); cur = tmp; }
    }
    free(ht->table);
    free(ht);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpirun -np <n> %s <arquivo_csv>\\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    const char* csv_path = argv[1];
    FILE* f = fopen(csv_path, "r");
    if (!f) {
        if (rank == 0) perror("Erro ao abrir CSV");
        MPI_Finalize();
        return 1;
    }

    char line[MAX_LINE];
    (void)fgets(line, MAX_LINE, f); // pula cabeçalho

    HashTable* local = ht_create();
    int line_num = 0;

    while (fgets(line, MAX_LINE, f)) {
        if (line_num++ % size != rank) continue;

        // artista é a primeira coluna
        char* artist = strtok(line, ",");
        if (artist && strlen(artist) > 0)
            ht_insert(local, artist);
    }
    fclose(f);

    // agregação
    if (rank == 0) {
        HashTable* global = ht_create();
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = local->table[i];
            while (cur) { ht_insert(global, cur->name); cur = cur->next; }
        }

        for (int src = 1; src < size; src++) {
            int count;
            MPI_Recv(&count, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < count; i++) {
                char name[MAX_NAME];
                int ncount;
                MPI_Recv(name, MAX_NAME, MPI_CHAR, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&ncount, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int k = 0; k < ncount; k++) ht_insert(global, name);
            }
        }

        // cria vetor p/ ordenar
        typedef struct { char* n; int c; } Pair;
        Pair* arr = malloc(10000 * sizeof(Pair));
        int n = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = global->table[i];
            while (cur && n < 10000) {
                arr[n].n = cur->name;
                arr[n].c = cur->count;
                n++;
                cur = cur->next;
            }
        }

        // ordena
        for (int i = 0; i < n - 1; i++)
            for (int j = i + 1; j < n; j++)
                if (arr[j].c > arr[i].c) { Pair tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp; }

        printf("\\nTop 10 artistas com mais músicas:\\n");
        for (int i = 0; i < 10 && i < n; i++)
            printf("%2d. %-30s %d\\n", i + 1, arr[i].n, arr[i].c);

        free(arr);
        ht_free(global);
    } else {
        // envia pro rank 0
        int total = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = local->table[i];
            while (cur) { total++; cur = cur->next; }
        }
        MPI_Send(&total, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = local->table[i];
            while (cur) {
                MPI_Send(cur->name, MAX_NAME, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&cur->count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                cur = cur->next;
            }
        }
    }

    ht_free(local);
    MPI_Finalize();
    return 0;
}
