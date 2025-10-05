#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define HASH_TABLE_SIZE 10000
#define MAX_WORD_LEN 100
#define MAX_LINE_LEN 8192

// Node for the hash table linked list
typedef struct Node {
    char* key;
    int value;
    struct Node* next;
} Node;

// Hash table structure
typedef struct HashTable {
    Node** table;
} HashTable;

// Hash function (djb2)
unsigned long hash_function(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASH_TABLE_SIZE;
}

// Create a new node
Node* create_node(const char* key, int value) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->key = strdup(key);
    new_node->value = value;
    new_node->next = NULL;
    return new_node;
}

// Create a hash table
HashTable* create_hash_table() {
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    ht->table = (Node**)calloc(HASH_TABLE_SIZE, sizeof(Node*));
    return ht;
}

// Insert or update a key in the hash table
void insert_or_update(HashTable* ht, const char* key) {
    unsigned long index = hash_function(key);
    Node* current = ht->table[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            current->value++;
            return;
        }
        current = current->next;
    }
    Node* new_node = create_node(key, 1);
    new_node->next = ht->table[index];
    ht->table[index] = new_node;
}

// Free the hash table
void free_hash_table(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Node* current = ht->table[i];
        while (current != NULL) {
            Node* temp = current;
            current = current->next;
            free(temp->key);
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}

// Structure to hold a key-value pair for sorting
typedef struct KeyValuePair {
    char* key;
    int value;
} KeyValuePair;

// Comparison function for qsort
int compare_key_value_pairs(const void* a, const void* b) {
    return ((KeyValuePair*)b)->value - ((KeyValuePair*)a)->value;
}

// Function to process lyrics: tokenize, clean, and count words
void process_lyrics(const char* lyrics, HashTable* word_counts) {
    char* lyrics_copy = strdup(lyrics);
    char* token = strtok(lyrics_copy, " \t\n\r\f\v");
    while (token != NULL) {
        char clean_word[MAX_WORD_LEN] = {0};
        int j = 0;
        for (int i = 0; token[i] != '\0' && j < MAX_WORD_LEN - 1; i++) {
            if (isalpha(token[i])) {
                clean_word[j++] = tolower(token[i]);
            }
        }
        if (j > 0) {
            insert_or_update(word_counts, clean_word);
        }
        token = strtok(NULL, " \t\n\r\f\v");
    }
    free(lyrics_copy);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    double start_time, end_time;

    if (world_rank == 0) {
        start_time = MPI_Wtime();

        FILE* file = fopen("spotify_millsongdata.csv", "r");
        if (!file) {
            fprintf(stderr, "Error: Cannot open file spotify_millsongdata.csv\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        char** lines = NULL;
        int line_count = 0;
        char buffer[MAX_LINE_LEN];

        // Skip header
        fgets(buffer, MAX_LINE_LEN, file);

        while (fgets(buffer, MAX_LINE_LEN, file)) {
            lines = (char**)realloc(lines, (line_count + 1) * sizeof(char*));
            lines[line_count] = strdup(buffer);
            line_count++;
        }
        fclose(file);

        int chunk_size = line_count / world_size;
        int remainder = line_count % world_size;

        for (int i = 1; i < world_size; i++) {
            int start_index = i * chunk_size + (i < remainder ? i : remainder);
            int end_index = start_index + chunk_size + (i < remainder ? 1 : 0);
            int num_lines_to_send = end_index - start_index;

            MPI_Send(&num_lines_to_send, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            for (int j = start_index; j < end_index; j++) {
                int len = strlen(lines[j]) + 1;
                MPI_Send(&len, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send(lines[j], len, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            }
        }

        int master_start_index = 0;
        int master_chunk_size = chunk_size + (0 < remainder ? 1 : 0);
        
        HashTable* local_word_counts = create_hash_table();
        HashTable* local_artist_counts = create_hash_table();
        
        for (int i = master_start_index; i < master_start_index + master_chunk_size; i++) {
             char* line_copy = strdup(lines[i]);
             char* artist = strtok(line_copy, ",");
             if (artist) {
                insert_or_update(local_artist_counts, artist);
                strtok(NULL, ","); // song
                strtok(NULL, ","); // link
                char* lyrics = strtok(NULL, "");
                if (lyrics) {
                    process_lyrics(lyrics, local_word_counts);
                }
             }
             free(line_copy);
        }
        
        // Aggregate results
        HashTable* global_word_counts = local_word_counts;
        HashTable* global_artist_counts = local_artist_counts;

        for (int i = 1; i < world_size; i++) {
            int num_words, num_artists;
            MPI_Recv(&num_words, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < num_words; j++) {
                int len;
                MPI_Recv(&len, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                char* word = (char*)malloc(len);
                MPI_Recv(word, len, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                int count;
                MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                unsigned long index = hash_function(word);
                Node* current = global_word_counts->table[index];
                int found = 0;
                while(current){
                    if(strcmp(current->key, word) == 0){
                        current->value += count;
                        found = 1;
                        break;
                    }
                    current = current->next;
                }
                if(!found){
                    Node* new_node = create_node(word, count);
                    new_node->next = global_word_counts->table[index];
                    global_word_counts->table[index] = new_node;
                }
                free(word);
            }

            MPI_Recv(&num_artists, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < num_artists; j++) {
                int len;
                MPI_Recv(&len, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                char* artist = (char*)malloc(len);
                MPI_Recv(artist, len, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                int count;
                MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                unsigned long index = hash_function(artist);
                Node* current = global_artist_counts->table[index];
                int found = 0;
                while(current){
                    if(strcmp(current->key, artist) == 0){
                        current->value += count;
                        found = 1;
                        break;
                    }
                    current = current->next;
                }
                if(!found){
                    Node* new_node = create_node(artist, count);
                    new_node->next = global_artist_counts->table[index];
                    global_artist_counts->table[index] = new_node;
                }
                free(artist);
            }
        }
        
        end_time = MPI_Wtime();

        // Process and print results
        int total_unique_words = 0;
        for(int i=0; i<HASH_TABLE_SIZE; ++i){
            Node* current = global_word_counts->table[i];
            while(current){
                total_unique_words++;
                current = current->next;
            }
        }
        KeyValuePair* word_kv_pairs = (KeyValuePair*)malloc(total_unique_words * sizeof(KeyValuePair));
        int k = 0;
        for(int i=0; i<HASH_TABLE_SIZE; ++i){
            Node* current = global_word_counts->table[i];
            while(current){
                word_kv_pairs[k].key = current->key;
                word_kv_pairs[k].value = current->value;
                k++;
                current = current->next;
            }
        }

        qsort(word_kv_pairs, total_unique_words, sizeof(KeyValuePair), compare_key_value_pairs);

        printf("Top 10 Most Frequent Words:\n");
        for (int i = 0; i < 10 && i < total_unique_words; i++) {
            printf("%d. %s: %d\n", i + 1, word_kv_pairs[i].key, word_kv_pairs[i].value);
        }

        int total_unique_artists = 0;
        for(int i=0; i<HASH_TABLE_SIZE; ++i){
            Node* current = global_artist_counts->table[i];
            while(current){
                total_unique_artists++;
                current = current->next;
            }
        }
        KeyValuePair* artist_kv_pairs = (KeyValuePair*)malloc(total_unique_artists * sizeof(KeyValuePair));
        k = 0;
        for(int i=0; i<HASH_TABLE_SIZE; ++i){
            Node* current = global_artist_counts->table[i];
            while(current){
                artist_kv_pairs[k].key = current->key;
                artist_kv_pairs[k].value = current->value;
                k++;
                current = current->next;
            }
        }
        qsort(artist_kv_pairs, total_unique_artists, sizeof(KeyValuePair), compare_key_value_pairs);

        printf("\nTop 10 Artists with Most Songs:\n");
        for (int i = 0; i < 10 && i < total_unique_artists; i++) {
            printf("%d. %s: %d\n", i + 1, artist_kv_pairs[i].key, artist_kv_pairs[i].value);
        }

        printf("\nExecution time: %f seconds\n", end_time - start_time);
        printf("Number of MPI processes: %d\n", world_size);
        
        free(word_kv_pairs);
        free(artist_kv_pairs);
        free_hash_table(global_word_counts);
        // Do not free global_artist_counts as it points to the same memory as local_artist_counts
        
        for (int i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        free(lines);

    } else { // Worker processes
        int num_lines_to_receive;
        MPI_Recv(&num_lines_to_receive, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        char** local_lines = (char**)malloc(num_lines_to_receive * sizeof(char*));
        for (int i = 0; i < num_lines_to_receive; i++) {
            int len;
            MPI_Recv(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            local_lines[i] = (char*)malloc(len);
            MPI_Recv(local_lines[i], len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        HashTable* local_word_counts = create_hash_table();
        HashTable* local_artist_counts = create_hash_table();
        
        for (int i = 0; i < num_lines_to_receive; i++) {
            char* line_copy = strdup(local_lines[i]);
            char* artist = strtok(line_copy, ",");
            if (artist) {
                insert_or_update(local_artist_counts, artist);
                strtok(NULL, ","); // song
                strtok(NULL, ","); // link
                char* lyrics = strtok(NULL, "");
                if (lyrics) {
                    process_lyrics(lyrics, local_word_counts);
                }
            }
            free(line_copy);
        }
        
        // Send results to master
        int num_words = 0;
        for(int i=0; i<HASH_TABLE_SIZE; ++i) {
            Node* current = local_word_counts->table[i];
            while(current) {
                num_words++;
                current = current->next;
            }
        }
        MPI_Send(&num_words, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

        for (int i = 0; i < HASH_TABLE_SIZE; i++) {
            Node* current = local_word_counts->table[i];
            while (current) {
                int len = strlen(current->key) + 1;
                MPI_Send(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(current->key, len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&current->value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                current = current->next;
            }
        }

        int num_artists = 0;
         for(int i=0; i<HASH_TABLE_SIZE; ++i) {
            Node* current = local_artist_counts->table[i];
            while(current) {
                num_artists++;
                current = current->next;
            }
        }
        MPI_Send(&num_artists, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

        for (int i = 0; i < HASH_TABLE_SIZE; i++) {
            Node* current = local_artist_counts->table[i];
            while (current) {
                int len = strlen(current->key) + 1;
                MPI_Send(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(current->key, len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&current->value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                current = current->next;
            }
        }

        free_hash_table(local_word_counts);
        free_hash_table(local_artist_counts);
        for (int i = 0; i < num_lines_to_receive; i++) {
            free(local_lines[i]);
        }
        free(local_lines);
    }

    MPI_Finalize();
    return 0;
}