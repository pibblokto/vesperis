#!/bin/sh
# R-420: the game's own tape into a package: the files of music/ the repository holds (a player's own beside them are not shipped).
#   sh packaging/tape.sh <dest dir>        (run from anywhere; the tree is this script's)
set -e
DEST=$1
cd "$(dirname "$0")/.."
mkdir -p "$DEST"
git ls-files music | while IFS= read -r f; do cp "$f" "$DEST/"; done
