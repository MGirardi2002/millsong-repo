#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/resource.h>

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

long get_peak_ram_kb() {
    FILE* f = fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[256];
    long peak_ram = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmHWM:", 6) == 0) {
            sscanf(line + 6, "%ld", &peak_ram);
            break;
        }
    }
    fclose(f);
    return peak_ram;
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double start_time, end_time, local_elapsed;
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

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
        
        printf("============================================\n");
        printf("    MPI WORD COUNTER (resultado final)\n");
        printf("============================================\n");
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

    end_time = MPI_Wtime();
    local_elapsed = end_time - start_time;

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double local_cpu_time = (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1e6 +
                            (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / 1e6;
    long local_peak_ram = get_peak_ram_kb();

    double max_elapsed;
    MPI_Reduce(&local_elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    double* all_cpu_times = NULL;
    long* all_peak_rams = NULL;
    if (rank == 0) {
        all_cpu_times = malloc(size * sizeof(double));
        all_peak_rams = malloc(size * sizeof(long));
    }

    MPI_Gather(&local_cpu_time, 1, MPI_DOUBLE, all_cpu_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&local_peak_ram, 1, MPI_LONG, all_peak_rams, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\n============================================\n");
        printf("    MÉTRICAS DE DESEMPENHO\n");
        printf("============================================\n");
        printf("Tempo de execução total: %.4f segundos\n", max_elapsed);

        double total_cpu_time = 0;
        long total_peak_ram = 0;
        printf("\n--- Métricas por processo ---\n");
        printf("Rank\tTempo de CPU (s)\tPico de RAM (MB)\n");
        printf("----\t----------------\t----------------\n");
        for (int i = 0; i < size; i++) {
            printf("%-4d\t%-16.4f\t%-16.2f\n", i, all_cpu_times[i], (double)all_peak_rams[i] / 1024.0);
            total_cpu_time += all_cpu_times[i];
            total_peak_ram += all_peak_rams[i];
        }
        printf("--------------------------------------------\n");
        printf("CPU Total Acumulado: %.4f segundos\n", total_cpu_time);
        printf("RAM Total Agregada:  %.2f MB\n", (double)total_peak_ram / 1024.0);
        printf("============================================\n");

        free(all_cpu_times);
        free(all_peak_rams);
    }

    ht_free(local);
    MPI_Finalize();
    return 0;
}