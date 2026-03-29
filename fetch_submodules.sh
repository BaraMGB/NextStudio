#!/bin/bash

set -e

echo "Fetching submodules with HTTPS fallback..."

# Temporär HTTPS für GitHub erzwingen
git config --global url."https://github.com/".insteadOf "git@github.com:"

# Cleanup-Funktion
cleanup() {
    echo "Restoring git configuration..."
    git config --global --unset url."https://github.com/".insteadOf 2>/dev/null || true
}

# Cleanup bei Script-Ende oder Unterbrechung
trap cleanup EXIT INT TERM

# Submodule URLs synchronisieren und holen
git submodule sync --recursive
git submodule update --init --recursive

if [ -d "modules/tracktion_engine" ]; then
    echo "tracktion_engine fetched successfully."
else
    echo "Warning: modules/tracktion_engine is missing after submodule fetch."
fi

if [ -d "modules/rubberband" ]; then
    echo "rubberband fetched successfully. RubberBand time-stretch will be available."
else
    echo "modules/rubberband not present. The app will fall back to SoundTouch-only time-stretch."
fi

echo "Submodules fetched successfully!"
