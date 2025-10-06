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

   ## 🚀 Execução Passo a Passo

### 1️⃣ Instalar dependências no WSL/Ubuntu

```
sudo apt update
sudo apt install -y build-essential make openmpi-bin libopenmpi-dev python3 python3-venv python-is-python3
```

### 2️⃣ Limpar o dataset original

Entre na pasta `utils` e execute o script:

```
python3 limpar_csv.py
```
Isso gera o arquivo `spotify_cleaned.csv` a partir do `spotify_millsongdata.csv`.

### 3️⃣ Compilar os programas MPI
Entre na pasta `src` e execute:

```
make
```

Isso cria dois executáveis: `word_counter` e `artist_counter`.

### 4️⃣ Executar os programas MPI
Para contar palavras:

```
make run_words NP=4
```
Para contar artistas:

```
make run_artists NP=4
```
Os resultados são salvos na pasta `out/`.

Para gerar as duas saídas simultaneamente:

```
make run_all NP=4
```
### 5️⃣ Limpar os binários

```
make clean
```

## 🔮 7️⃣ Classificação de Sentimento (Parte 3)

Esta etapa utiliza **Python** e o modelo local de linguagem do **Ollama**  
para classificar as letras das músicas em três categorias:

- **Positiva**
- **Negativa**
- **Neutra**

O script lê o arquivo `spotify_cleaned.csv`, envia as letras para o modelo local  
e conta quantas músicas se enquadram em cada classe.

---

### ⚙️ Pré-requisitos

1. Ter o **Ollama** instalado:  
   Linux:
   ```
   curl -fsSL https://ollama.com/install.sh | sh
   ```

2. Baixar um modelo compatível (exemplo: `llama3`):  
   ```bash
   ollama pull llama3
   ```

 ## 🚀 Executando a classificação



