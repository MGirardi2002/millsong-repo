CC      = mpicc
CFLAGS  = -Wall -O2

SRC_WORDS   = word_counter.c
SRC_ARTISTS = artist_counter.c

EXEC_WORDS   = word_counter
EXEC_ARTISTS = artist_counter

CSV_PATH = ../utils/csv_limpo.csv

NP ?= 4

all: $(EXEC_WORDS) $(EXEC_ARTISTS)

$(EXEC_WORDS): $(SRC_WORDS)
	@echo "Compilando $@..."
	$(CC) $(CFLAGS) -o $@ $<

$(EXEC_ARTISTS): $(SRC_ARTISTS)
	@echo "Compilando $@..."
	$(CC) $(CFLAGS) -o $@ $<

run_words: $(EXEC_WORDS)
	@echo "Executando $(EXEC_WORDS) com $(NP) processos..."
	@mpirun -np $(NP) ./$(EXEC_WORDS) $(CSV_PATH) > ../out/out_palavras.txt
	@echo "Saída salva em ../out/out_palavras.txt"

run_artists: $(EXEC_ARTISTS)
	@echo "Executando $(EXEC_ARTISTS) com $(NP) processos..."
	@mpirun -np $(NP) ./$(EXEC_ARTISTS) $(CSV_PATH) > ../out/out_artistas.txt
	@echo "Saída salva em ../out/out_artistas.txt"

clean:
	@echo "Removendo executáveis..."
	rm -f $(EXEC_WORDS) $(EXEC_ARTISTS)

.PHONY: all run_words run_artists clean
