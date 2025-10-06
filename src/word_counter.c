#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 200003
#define MAX_WORD 128
#define MAX_LINE 16384

typedef struct Node {
    char word[MAX_WORD];
    int count;
    struct Node* next;
} Node;

typedef struct {
    Node** table;
} HashTable;

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

void ht_insert(HashTable* ht, const char* word) {
    unsigned long idx = hash(word);
    Node* cur = ht->table[idx];
    while (cur) {
        if (strcmp(cur->word, word) == 0) {
            cur->count++;
            return;
        }
        cur = cur->next;
    }
    Node* n = malloc(sizeof(Node));
    strncpy(n->word, word, MAX_WORD - 1);
    n->word[MAX_WORD - 1] = '\0';
    n->count = 1;
    n->next = ht->table[idx];
    ht->table[idx] = n;
}

void ht_add(HashTable* ht, const char* key, int inc) {
    unsigned long idx = hash(key);
    Node* cur = ht->table[idx];
    while (cur) {
        if (strcmp(cur->word, key) == 0) {
            cur->count += inc;
            return;
        }
        cur = cur->next;
    }
    Node* n = malloc(sizeof(Node));
    strncpy(n->word, key, MAX_WORD - 1);
    n->word[MAX_WORD - 1] = '\0';
    n->count = inc;
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

void normalize(char* w) {
    int i=0, j=0;
    for (; w[i]; i++) {
        unsigned char c = (unsigned char)w[i];
        if (isalpha(c) || c=='\'') w[j++] = (char)tolower(c);
    }
    w[j] = '\0';
}

int extrair_letra(const char* linha, char* letra_out) {
    int aspas = 0, campo = 0;
    const char* start = NULL;
    for (int i = 0; linha[i]; i++) {
        char c = linha[i];
        if (c == '"') aspas = !aspas;
        else if (c == ',' && !aspas) {
            campo++;
            if (campo == 3) { start = &linha[i + 1]; break; }
        }
    }
    if (!start) return -1;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '"') start++;
    strncpy(letra_out, start, MAX_LINE - 1);
    letra_out[MAX_LINE - 1] = '\0';
    size_t len = strlen(letra_out);
    if (len > 0) {
        char* end = letra_out + len - 1;
        while (end >= letra_out && (isspace((unsigned char)*end) || *end == '"')) {
            *end = '\0'; end--;
        }
    }
    return 0;
}

void process_lyrics(const char* lyrics, HashTable* ht) {
    char token[MAX_WORD];
    int t = 0;
    for (int i = 0;; i++) {
        unsigned char c = (unsigned char)lyrics[i];
        if (isalpha(c) || c == '\'') {
            if (t < MAX_WORD - 1) token[t++] = (char)tolower(c);
        } else {
            if (t > 0) {
                token[t] = '\0';
                normalize(token);
                if (token[0] != '\0') ht_insert(ht, token);
                t = 0;
            }
        }
        if (c == '\0') break;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpirun -np <n> %s <arquivo_csv>\n", argv[0]);
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
    fgets(line, MAX_LINE, f);

    HashTable* local = ht_create();
    int line_num = 0;

    while (fgets(line, MAX_LINE, f)) {
        if (line_num++ % size != rank) continue;
        char letra[MAX_LINE];
        if (extrair_letra(line, letra) == 0)
            process_lyrics(letra, local);
    }
    fclose(f);

    if (rank == 0) {
        HashTable* global = ht_create();

        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = local->table[i];
            while (cur) { ht_add(global, cur->word, cur->count); cur = cur->next; }
        }

        for (int src = 1; src < size; src++) {
            int count;
            MPI_Recv(&count, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < count; i++) {
                char word[MAX_WORD];
                int wcount;
                MPI_Recv(word, MAX_WORD, MPI_CHAR, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&wcount, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                ht_add(global, word, wcount);
            }
        }

        typedef struct { char* w; int c; } Pair;
        Pair* arr = malloc(100000 * sizeof(Pair));
        int n = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = global->table[i];
            while (cur && n < 100000) {
                arr[n].w = cur->word;
                arr[n].c = cur->count;
                n++;
                cur = cur->next;
            }
        }

        for (int i = 0; i < n - 1; i++)
            for (int j = i + 1; j < n; j++)
                if (arr[j].c > arr[i].c) {
                    Pair tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
                }

        printf("=====================================\n");
        printf("     MPI WORD COUNTER (resultado final)\n");
        printf("=====================================\n");
        printf("Top 10 palavras mais frequentes:\n\n");
        for (int i = 0; i < 10 && i < n; i++)
            printf("%2d. %-20s %d\n", i + 1, arr[i].w, arr[i].c);

        free(arr);
        ht_free(global);
    } else {
        int total = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = local->table[i];
            while (cur) { total++; cur = cur->next; }
        }
        MPI_Send(&total, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = local->table[i];
            while (cur) {
                MPI_Send(cur->word, MAX_WORD, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&cur->count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                cur = cur->next;
            }
        }
    }

    ht_free(local);
    MPI_Finalize();
    return 0;
}
