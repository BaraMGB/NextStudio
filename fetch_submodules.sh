#!/bin/bash

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

# Submodules holen
git submodule update --init --recursive

echo "Submodules fetched successfully!"
