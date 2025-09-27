#!/usr/bin/env bash
set -euo pipefail

# --- Konfiguration ---
APP_ID="com.nextstudio.NextStudio"
MANIFEST="./com.nextstudio.NextStudio.json"
DESKTOP="./com.nextstudio.NextStudio.desktop"

OUT_DIR="./dist"
REPO_DIR="${OUT_DIR}/repo"
BUILD_DIR="./flatpak-build"      # temporärer Build-Ordner (Quelle bleibt unberührt)

# --- Checks ---
command -v flatpak >/dev/null || { echo "flatpak fehlt"; exit 1; }
command -v flatpak-builder >/dev/null || { echo "flatpak-builder fehlt"; exit 1; }

[[ -f "$MANIFEST" ]] || { echo "Manifest nicht gefunden: $MANIFEST"; exit 1; }
[[ -f "$DESKTOP"  ]] || { echo "Desktop-Datei nicht gefunden: $DESKTOP"; exit 1; }

# --- Arch & Branch ermitteln ---
ARCH="$(flatpak --default-arch || echo x86_64)"

# Branch aus Manifest lesen, sonst master
if command -v jq >/dev/null 2>&1; then
  BRANCH="$(jq -r '.branch // empty' "$MANIFEST" || true)"
else
  BRANCH=""
fi
[[ -n "${BRANCH}" ]] || BRANCH="master"

# Version für Dateinamen (best effort)
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  VERSION="$(git describe --tags --always --dirty 2>/dev/null || date +%Y%m%d)"
else
  VERSION="$(date +%Y%m%d)"
fi

mkdir -p "$OUT_DIR" "$REPO_DIR"

# --- Versuchen, aus installierter App zu übernehmen ---
echo "Prüfe installierte Flatpak-App ${APP_ID} ..."
INSTALLED_REF="$(flatpak info "${APP_ID}" 2>/dev/null | awk '/^Ref:/{print $2}' || true)"

USE_COMMIT_FROM="no"
if [[ -n "$INSTALLED_REF" ]]; then
  # Erwartetes Ref: app/APP_ID/ARCH/BRANCH
  EXPECTED_REF="app/${APP_ID}/${ARCH}/${BRANCH}"
  if [[ "$INSTALLED_REF" == "$EXPECTED_REF" ]]; then
    USE_COMMIT_FROM="yes"
  else
    echo "Hinweis: Installiert ist ${INSTALLED_REF}, erwartet wird ${EXPECTED_REF}. Baue neu."
  fi
fi

if [[ "$USE_COMMIT_FROM" == "yes" ]]; then
  echo "Übernehme Commit aus User-Repo (~/.local/share/flatpak/repo) nach ${REPO_DIR} ..."
  flatpak build-commit-from \
    --src-ref="app/${APP_ID}/${ARCH}/${BRANCH}" \
    "${HOME}/.local/share/flatpak/repo" \
    "${REPO_DIR}"
else
  echo "Baue und exportiere in ${REPO_DIR} (dies nutzt die Caches, kann aber ein paar Minuten dauern) ..."
  # Sauberer Rebuild des temporären Build-Ordners, nicht deiner Quellen
  rm -rf "$BUILD_DIR"
  flatpak-builder \
    --repo="${REPO_DIR}" \
    --force-clean \
    "${BUILD_DIR}" \
    "${MANIFEST}"
fi

# --- Bundle erzeugen ---
BUNDLE_FILE="${OUT_DIR}/${APP_ID}-${ARCH}-${BRANCH}-${VERSION}.flatpak"
echo "Erzeuge Bundle: ${BUNDLE_FILE}"
flatpak build-bundle \
  "${REPO_DIR}" \
  "${BUNDLE_FILE}" \
  "${APP_ID}" \
  "${BRANCH}" \
  --arch="${ARCH}" \
  --runtime-repo="https://dl.flathub.org/repo/flathub.flatpakrepo"

# --- Prüfung ---
echo "Prüfe Bundle-Metadaten:"
flatpak info --file "${BUNDLE_FILE}" || true

cat <<EOF

Fertig!
- Repo:    ${REPO_DIR}
- Bundle:  ${BUNDLE_FILE}

Installation auf einem anderen Rechner:
  flatpak install --user "${BUNDLE_FILE}"
  flatpak run ${APP_ID}
EOF

