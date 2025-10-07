import csv
import re
import sys

csv.field_size_limit(sys.maxsize)

arquivo_entrada = "spotify_millsongdata.csv"
arquivo_saida = "csv_limpo.csv"

print(f"Iniciando a limpeza robusta do arquivo: {arquivo_entrada}")
print("Isso pode levar alguns minutos...")

linhas_lidas = 0
linhas_escritas = 0

try:
    with open(
        arquivo_entrada, mode="r", encoding="utf-8", errors="ignore"
    ) as infile, open(arquivo_saida, mode="w", encoding="utf-8", newline="") as outfile:

        leitor_csv = csv.reader(infile)

        escritor_csv = csv.writer(outfile, quoting=csv.QUOTE_ALL)

        cabecalho = next(leitor_csv)
        escritor_csv.writerow(cabecalho)
        linhas_lidas += 1
        linhas_escritas += 1

        for linha in leitor_csv:
            linhas_lidas += 1
            if linhas_lidas % 200000 == 0:
                print(f"  ... {linhas_lidas} linhas lidas")

            if len(linha) == 4:
                texto = linha[3]

                texto_limpo = texto.replace("\n", " ")

                texto_limpo = re.sub(r"\s+", " ", texto_limpo).strip()

                linha[3] = texto_limpo

                escritor_csv.writerow(linha)
                linhas_escritas += 1
            else:
                pass

    print("\nLimpeza concluída com sucesso!")
    print(f"Total de linhas lidas: {linhas_lidas}")
    print(f"Total de linhas escritas: {linhas_escritas}")

except Exception as e:
    print(f"\nOcorreu um erro na linha {linhas_lidas}: {e}")
