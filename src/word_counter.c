#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 200003
#define MAX_WORD 128
#define MAX_LINE 16384

// nó simples da hash
typedef struct Node {
    char word[MAX_WORD];
    int count;
    struct Node* next;
} Node;

// tabela hash básica
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

// cria e inicializa hash
HashTable* ht_create() {
    HashTable* ht = malloc(sizeof(HashTable));
    ht->table = calloc(HASH_SIZE, sizeof(Node*));
    return ht;
}

// insere ou incrementa palavra
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

// libera memória
void ht_free(HashTable* ht) {
    for (int i = 0; i < HASH_SIZE; i++) {
        Node* cur = ht->table[i];
        while (cur) {
            Node* tmp = cur->next;
            free(cur);
            cur = tmp;
        }
    }
    free(ht->table);
    free(ht);
}

// quebra letra em palavras (tokens)
void process_lyrics(char* lyrics, HashTable* ht) {
    char token[MAX_WORD];
    int t = 0;
    for (int i = 0; lyrics[i]; i++) {
        if (isalpha((unsigned char)lyrics[i])) {
            token[t++] = tolower(lyrics[i]);
            if (t >= MAX_WORD - 1) t = MAX_WORD - 2;
        } else if (t > 0) {
            token[t] = '\0';
            ht_insert(ht, token);
            t = 0;
        }
    }
    if (t > 0) { token[t] = '\0'; ht_insert(ht, token); }
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
    fgets(line, MAX_LINE, f); // pula cabeçalho

    HashTable* local = ht_create();
    int line_num = 0;

    // cada processo lê apenas linhas que pertencem ao seu rank (modo round-robin)
    while (fgets(line, MAX_LINE, f)) {
        if (line_num++ % size != rank) continue;

        // pega a última coluna (letra)
        char* last = strrchr(line, ',');
        if (!last) continue;
        last++;

        process_lyrics(last, local);
    }
    fclose(f);

    // agregação simples no rank 0
    if (rank == 0) {
        HashTable* global = ht_create();
        // junta os locais do rank 0
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = local->table[i];
            while (cur) {
                ht_insert(global, cur->word);
                cur = cur->next;
            }
        }

        // recebe dos outros ranks
        for (int src = 1; src < size; src++) {
            int count;
            MPI_Recv(&count, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < count; i++) {
                char word[MAX_WORD];
                int wcount;
                MPI_Recv(word, MAX_WORD, MPI_CHAR, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&wcount, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int k = 0; k < wcount; k++) ht_insert(global, word);
            }
        }

        // exibe top 10
        printf("\\nTop 10 palavras mais frequentes:\\n");

        // transforma em vetor pra ordenar
        typedef struct { char* w; int c; } Pair;
        Pair* arr = malloc(50000 * sizeof(Pair));
        int n = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Node* cur = global->table[i];
            while (cur && n < 50000) {
                arr[n].w = cur->word;
                arr[n].c = cur->count;
                n++;
                cur = cur->next;
            }
        }
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                if (arr[j].c > arr[i].c) {
                    Pair tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
                }
            }
        }
        for (int i = 0; i < 10 && i < n; i++)
            printf("%2d. %-20s %d\\n", i + 1, arr[i].w, arr[i].c);

        free(arr);
        ht_free(global);
    } else {
        // envia dados do processo pro rank 0
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
