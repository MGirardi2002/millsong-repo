import csv
import re
import sys

# Aumenta o limite do tamanho do campo que o leitor de CSV pode manusear
# Alguns textos de música podem ser muito grandes
csv.field_size_limit(sys.maxsize)

arquivo_entrada = "spotify_millsongdata.csv"
arquivo_saida = "spotify_cleaned.csv"

print(f"Iniciando a limpeza robusta do arquivo: {arquivo_entrada}")
print("Isso pode levar alguns minutos...")

# Contadores para acompanhar o progresso
linhas_lidas = 0
linhas_escritas = 0

try:
    # Abre o arquivo de entrada para leitura e o de saída para escrita
    with open(
        arquivo_entrada, mode="r", encoding="utf-8", errors="ignore"
    ) as infile, open(arquivo_saida, mode="w", encoding="utf-8", newline="") as outfile:

        # Cria um leitor de CSV que entende o formato
        leitor_csv = csv.reader(infile)

        # Cria um escritor de CSV que irá formatar a saída corretamente
        escritor_csv = csv.writer(outfile, quoting=csv.QUOTE_ALL)

        # Lê o cabeçalho
        cabecalho = next(leitor_csv)
        escritor_csv.writerow(cabecalho)
        linhas_lidas += 1
        linhas_escritas += 1

        # Itera sobre cada linha do arquivo de entrada
        for linha in leitor_csv:
            linhas_lidas += 1
            if linhas_lidas % 200000 == 0:
                print(f"  ... {linhas_lidas} linhas lidas")

            # Garante que a linha tem o número esperado de colunas
            if len(linha) == 4:
                # A coluna de texto é a quarta (índice 3)
                texto = linha[3]

                # 1. Substitui quebras de linha (\n) por espaços
                texto_limpo = texto.replace("\n", " ")

                # 2. Substitui múltiplos espaços por um único espaço
                texto_limpo = re.sub(r"\s+", " ", texto_limpo).strip()

                # Atualiza a linha com o texto limpo
                linha[3] = texto_limpo

                # Escreve a linha modificada no arquivo de saída
                escritor_csv.writerow(linha)
                linhas_escritas += 1
            else:
                # Opcional: reportar linhas com formato inesperado
                # print(f"Aviso: Linha {linhas_lidas} ignorada por ter {len(linha)} colunas em vez de 4.")
                pass

    print("\nLimpeza concluída com sucesso!")
    print(f"Total de linhas lidas: {linhas_lidas}")
    print(f"Total de linhas escritas: {linhas_escritas}")

except Exception as e:
    print(f"\nOcorreu um erro na linha {linhas_lidas}: {e}")
