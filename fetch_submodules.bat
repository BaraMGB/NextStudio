@echo off
echo Fetching submodules with HTTPS fallback...

REM Temporär HTTPS für GitHub erzwingen
git config --global url."https://github.com/".insteadOf "git@github.com:"

REM Submodules holen
git submodule update --init --recursive

REM Git-Konfiguration zurücksetzen
echo Restoring git configuration...
git config --global --unset url."https://github.com/".insteadOf 2>nul

echo Submodules fetched successfully!
pause

