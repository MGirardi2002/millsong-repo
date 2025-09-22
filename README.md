# millsong-repo

Este projeto tem como objetivo desenvolver uma aplicação paralela utilizando MPI com C para processar dados do Spotify Million Song Dataset ([Kaggle](https://www.kaggle.com/datasets/notshrirang/spotify-million-song-dataset)). O trabalho está dividido em três desafios principais:

## Desafios

1. **Contagem de Palavras:** Contar o número total de palavras no dataset de forma paralela.
2. **Classificação de Sentimento:** Utilizar um modelo de linguagem (Python) para classificar o sentimento das músicas.
3. **Estatísticas Avançadas:** Gerar estatísticas como média de palavras por música, artista mais prolífico, etc.

## Tecnologias Utilizadas

- **C** e **MPI** para processamento paralelo.
- **Python** para integração com modelos de linguagem.
- **Ollama** ou software similar para execução local do LLM.

## Como Executar

1. Baixe o dataset do Kaggle e coloque em `utils/spotify_millsongdata.csv`.
2. Compile o código C com suporte a MPI:
   ```sh
   mpicc -o word-counter src/word-counter.c
   mpicc -o aaaaa src/aaaaa.c
   ```
3. Execute os scripts conforme instruções específicas de cada desafio.
4. Para a classificação, certifique-se de ter o modelo de linguagem local configurado e o Python instalado.

## Estrutura do Projeto

- `src/` - Código fonte em C e scripts auxiliares em Python.
- `utils/` - Dataset do Spotify.
- `README.md` - Documentação do projeto.

## Contribuição

Sinta-se à vontade para abrir issues ou pull requests para melhorias.
