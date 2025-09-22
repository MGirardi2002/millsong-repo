#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>

#define MAX_LINE 4096
#define MAX_WORD_LEN 100
#define MAX_UNIQUE_WORDS 50000

typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} WordCount;

// Função para encontrar uma palavra na lista de palavras únicas
int find_word(WordCount *words, int num_words, const char *word) {
    for (int i = 0; i < num_words; i++) {
        if (strcmp(words[i].word, word) == 0) {
            return i;
        }
    }
    return -1;
}

// ... (resto do código MPI, main, etc.)

// Dentro do loop de leitura de cada processo:
while (fgets(line, sizeof(line), file)) {
    if (line_count++ % size != rank) continue;

    char *lyrics = get_lyrics(line); // Função para pegar só a coluna de letra
    if (lyrics) {
        char *token = strtok(lyrics, " \\n\\r\\t,.;:!?()[]{}\\\""); // Separadores
        while (token != NULL) {
            // Converte para minúsculo
            for (int i = 0; token[i]; i++) {
                token[i] = tolower(token[i]);
            }

            int idx = find_word(local_words, local_num_words, token);
            if (idx != -1) {
                local_words[idx].count++;
            } else if (local_num_words < MAX_UNIQUE_WORDS) {
                strncpy(local_words[local_num_words].word, token, MAX_WORD_LEN - 1);
                local_words[local_num_words].word[MAX_WORD_LEN - 1] = '\\0';
                local_words[local_num_words].count = 1;
                local_num_words++;
            }
            token = strtok(NULL, " \\n\\r\\t,.;:!?()[]{}\\\"");
        }
    }
}

// ... (depois, usar MPI_Gather e MPI_Reduce para juntar os resultados)