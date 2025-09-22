# millsong-repo

Este projeto tem como objetivo desenvolver uma aplicação paralela utilizando MPI com C para processar dados do Spotify Million Song Dataset ([Kaggle](https://www.kaggle.com/datasets/notshrirang/spotify-million-song-dataset)). O trabalho está dividido em três desafios principais:

## Desafios

1. **Contagem de Palavras nas Letras (40%)**
   - Contar a aparição de cada palavra presente nas letras das músicas do dataset.
   - O processamento será realizado em paralelo para otimizar o desempenho.

2. **Artistas com Mais Músicas (40%)**
   - Identificar os artistas que possuem a maior quantidade de músicas no dataset.
   - Utilizar MPI para distribuir a análise entre múltiplos processos.

3. **Classificação de Letras (20%)**
   - Classificar cada letra de música como "Positiva", "Neutra" ou "Negativa".
   - Integrar a aplicação C/MPI com um modelo local de linguagem (LLM), utilizando Python para a chamada do modelo.
   - Ferramentas como [Ollama](https://ollama.com) podem ser utilizadas para executar o LLM localmente.
   - Após a classificação, contar o total de músicas em cada classe.

## Tecnologias Utilizadas

- **C** e **MPI** para processamento paralelo.
- **Python** para integração com modelos de linguagem.
- **Ollama** ou software similar para execução local do LLM.

## Como Executar

1. Baixe o dataset do Kaggle.
2. Compile o código C com suporte a MPI.
3. Execute os scripts conforme instruções específicas de cada desafio.
4. Para a classificação, certifique-se de ter o modelo de linguagem local configurado e o Python instalado.

## Estrutura do Projeto

- `src/` - Código fonte em C e scripts auxiliares em Python.
- `data/` - Dataset do Spotify.
- `README.md` - Documentação do projeto.

## Contribuição

Sinta-se à vontade para abrir issues ou pull requests para melhorias.
