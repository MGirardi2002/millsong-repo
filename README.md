# 🎵 Projeto MPI – Análise de Músicas do Spotify

Trabalho prático de programação paralela utilizando **MPI em C**.  
O projeto processa o dataset do Spotify para extrair informações em paralelo:

---

## 🧩 Objetivos

1. **Contagem de Palavras** — contar quantas vezes cada palavra aparece nas letras das músicas.  
   🧠 *Arquivo:* `word_counter.c`  

2. **Artistas com Mais Músicas** — identificar quais artistas possuem o maior número de músicas no dataset.  
   🎤 *Arquivo:* `artist_counter.c`  

3. **Classificação de Sentimento** — analisar o sentimento das letras das músicas (positivo, negativo, neutro).
   🔮 *Arquivo:* `sentiment.py`

---

## ⚙️ Tecnologias Utilizadas

- **C + MPI (OpenMPI)** — processamento paralelo
- **Python 3** — limpeza do CSV e integração posterior com modelo local de linguagem (Ollama)
- **Sistema:** Ubuntu via WSL (ou Linux nativo)

---

## 📂 Estrutura de Pastas

   millsong-repo/
   ├── src/                         
   │   ├── artist_counter.c          
   │   ├── word_counter.c                 
   │   └── Makefile                 
   ├── utils/                      
   │   ├── limpar_csv.py                  
   |   ├── spotify_millsongdata.csv
   |   └── spotify_cleaned.csv                        
   ├── out/                         
   │   ├── out_palavras.txt          
   │   └── out_artistas.txt          
   ├── README.md                     
   └── .venv/   