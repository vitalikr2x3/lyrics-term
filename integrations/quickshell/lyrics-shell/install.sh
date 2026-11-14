#!/usr/bin/env bash
# Copies the lyrics shell into ~/.config/quickshell/lyrics/
set -euo pipefail

DEST="$HOME/.config/quickshell/lyrics"
SRC="$(cd "$(dirname "$0")" && pwd)"

mkdir -p "$DEST"
cp "$SRC/shell.qml" "$DEST/shell.qml"

if ! command -v lyrics-term >/dev/null 2>&1; then
  echo "lyrics-term not found in PATH: install it first (pacman -S lyrics-term)"
fi
if ! command -v qs >/dev/null 2>&1; then
  echo "quickshell (qs) not found: install it first (pacman -S quickshell)"
fi

echo "Installed to $DEST. Run: qs -c lyrics"
