#!/bin/bash

JSON_FILE="compile_commands.json"
OUTPUT_FILE=".clangd"

# 1. Compiler finden
# Wir versuchen, den Compiler aus der compile_commands.json zu lesen (via grep/sed Hack, falls kein jq installiert ist)
# Nimmt das erste Vorkommen von "command" oder "arguments" und extrahiert das erste Wort (den Compiler).
COMPILER=$(grep -oE '"command": "[^"]+"' "$JSON_FILE" | head -n 1 | sed -E 's/"command": "([^ ]+).*/\1/')

# Fallback, falls grep nichts findet
if [ -z "$COMPILER" ]; then
    COMPILER="/usr/bin/g++"
    echo "Konnte Compiler nicht aus $JSON_FILE lesen. Nutze Fallback: $COMPILER"
else
    echo "Compiler erkannt: $COMPILER"
fi

# 2. Header-Datei starten
echo "CompileFlags:" > "$OUTPUT_FILE"
echo "  Add:" >> "$OUTPUT_FILE"

# 3. System-Include-Pfade extrahieren und formatieren
# -x c++: Sprache C++
# -v: Verbose (zeigt die Pfade)
# -E: Preprocess only
# /dev/null: Leere Eingabe
echo "Lese System-Includes..."

"$COMPILER" -x c++ -v -E /dev/null 2>&1 | \
sed -n '/^#include <...>/,/^End of search list/p' | \
grep -vE "^#include|^End of" | \
while read -r path; do
    # Pfad bereinigen (Leerzeichen am Anfang entfernen)
    clean_path=$(echo "$path" | sed 's/^[ \t]*//')
    # In .clangd schreiben
    echo "    - -I$clean_path" >> "$OUTPUT_FILE"
done

# 4. Restliche Konfiguration hinzufügen
echo "  Compiler: $COMPILER" >> "$OUTPUT_FILE"
echo "Index:" >> "$OUTPUT_FILE"
echo "  Background: Skip" >> "$OUTPUT_FILE"

echo "Fertig! $OUTPUT_FILE wurde erstellt."
