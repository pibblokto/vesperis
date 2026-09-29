#!/bin/sh
# M6-03: a portable zip with the binary, the README and the docs (Linux, macOS; Windows via CPack).
#   make vesperis && sh packaging/make_zip.sh            -> dist/vesperis-<os>.zip
set -e
cd "$(dirname "$0")/.."
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
OUT=dist/vesperis-$OS
rm -rf "$OUT"; mkdir -p "$OUT"
cp vesperis "$OUT/"
cp README.md "$OUT/"
cp -R docs "$OUT/docs"
[ -f vesperis_keys.example.txt ] && cp vesperis_keys.example.txt "$OUT/"
(cd dist && rm -f "vesperis-$OS.zip" && zip -qr "vesperis-$OS.zip" "vesperis-$OS")
echo "built dist/vesperis-$OS.zip"
