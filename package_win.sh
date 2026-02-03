#!/bin/bash

# Fehler abfangen: Skript bricht ab, wenn ein Befehl fehlschlägt
set -e

echo "========================================================="
echo "   NextStudio Windows Packaging (Git-Bash)"
echo "========================================================="

# 1. Variablen definieren
BUILD_DIR="build_win_pkg"
CONFIG="Release"

# 2. Build-Verzeichnis erstellen
if [ ! -d "$BUILD_DIR" ]; then
    echo "Erstelle Verzeichnis: $BUILD_DIR"
    mkdir "$BUILD_DIR"
fi

# 3. CMake Konfiguration
echo "[1/3] Konfiguriere CMake ($CONFIG)..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$CONFIG

# 4. Kompilieren
echo "[2/3] Baue Applikation..."
# Wir nutzen --config Release, da Windows-Generatoren (Visual Studio) oft Multi-Config sind
cmake --build "$BUILD_DIR" --config $CONFIG -j 12

# 5. CPack aufrufen
echo "[3/3] Erstelle Installer mit CPack..."
cd "$BUILD_DIR"
# -C Release gibt CPack an, welche Konfiguration eingepackt werden soll
cpack -C $CONFIG

echo ""
echo "========================================================="
echo "ERFOLG: Der Installer wurde erstellt."
echo "Pfad: $(pwd)"
echo "========================================================="
