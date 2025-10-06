import csv
import subprocess
import json
from collections import Counter

# ------------------------------
# CONFIGURAÇÕES
# ------------------------------

CSV_PATH = "utils/spotify_cleaned.csv"
OUTPUT_PATH = "out/sentiment_summary.txt"
MODEL_NAME = "llama3" 
LIMIT = 10           


# ------------------------------
# FUNÇÃO: chama o modelo do Ollama
# ------------------------------

def classify_lyrics(lyrics):
    """
    Envia a letra para o modelo local do Ollama e retorna a classe:
    'Positiva', 'Negativa' ou 'Neutra'.
    """
    prompt = f"""
    Analise o sentimento da letra a seguir e responda apenas com uma palavra:
    'Positiva', 'Negativa' ou 'Neutra'.

    Letra:
    {lyrics[:1500]}  # limita o texto pra não travar o modelo
    """

    try:
        result = subprocess.run(
            ["ollama", "run", MODEL_NAME],
            input=prompt.encode("utf-8"),
            capture_output=True,
            timeout=60
        )
        response = result.stdout.decode("utf-8").strip().capitalize()

        # Normaliza a resposta
        if "pos" in response.lower():
            return "Positiva"
        elif "neg" in response.lower():
            return "Negativa"
        else:
            return "Neutra"

    except Exception as e:
        print(f"Erro ao classificar: {e}")
        return "Neutra"

# ------------------------------
# PROCESSAMENTO PRINCIPAL
# ------------------------------

def main():
    counts = Counter()
    processed = 0

    with open(CSV_PATH, newline='', encoding="utf-8") as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            lyrics = row["text"].strip()
            if not lyrics:
                continue

            sentiment = classify_lyrics(lyrics)
            counts[sentiment] += 1
            processed += 1

            print(f"[{processed}] {row['artist']} → {sentiment}")

            if processed >= LIMIT:
                break

    # ------------------------------
    # SALVA RESULTADO FINAL
    # ------------------------------

    total = sum(counts.values())
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        f.write("=== CLASSIFICAÇÃO DE SENTIMENTO ===\n")
        f.write(f"Total de músicas analisadas: {total}\n\n")
        for sentiment, count in counts.items():
            percent = (count / total) * 100 if total > 0 else 0
            f.write(f"{sentiment}: {count} ({percent:.1f}%)\n")

    print("\nResumo salvo em:", OUTPUT_PATH)
    print(counts)

if __name__ == "__main__":
    main()
