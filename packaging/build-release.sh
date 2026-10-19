#!/bin/sh
# F-09: the six release zips from this tree, vesperis-<v>-<macos|windows|linux>-<arm64|x86_64>.zip, each holding the binary,
# README.md, LICENSE and vesperis_keys.example.txt (macOS also Vesperis.app). The macOS and Windows builds run side by side, the Linux
# ones after them, one at a time (beside them Docker's VM went away mid-build in 1.3.0). Run it from a checkout of the tagged commit
# with VESPERIS_TC pointing at the toolchains (sh packaging/toolchains.sh first).
#   sh packaging/build-release.sh <version> <out dir>        -> <out dir>/zip/*.zip, a log per build in <out dir>
V=$1
mkdir -p "$2"; OUT=$(cd "$2" && pwd)
cd "$(dirname "$0")/.."
build() {   # build <script> <arch> <name>
    sh packaging/$1 $2 "$OUT/$3" > "$OUT/$3.log" 2>&1; echo "exit $?" >> "$OUT/$3.log"
}
( build build-mac.sh arm64 mac-arm64; build build-mac.sh x86_64 mac-x86_64 ) &
( build build-win.sh x86_64 win-x86_64; build build-win.sh aarch64 win-aarch64 ) &
wait
build build-linux.sh arm64 linux-arm64
build build-linux.sh amd64 linux-amd64
failed=0
for b in mac-arm64 mac-x86_64 win-x86_64 win-aarch64 linux-arm64 linux-amd64; do
    echo "$b: $(tail -1 "$OUT/$b.log"), $(grep -c 'warning:' "$OUT/$b.log") warnings"
    [ "$(tail -1 "$OUT/$b.log")" = "exit 0" ] || failed=1
done
[ $failed = 0 ] || { echo "a build failed: no zips" >&2; exit 1; }
set -e
pack() {   # pack <os> <arch> <binary>
    D="$OUT/zip/vesperis-$V-$1-$2"; rm -rf "$D"; mkdir -p "$D"
    cp README.md LICENSE vesperis_keys.example.txt "$D/"
    cp "$3" "$D/"; chmod +x "$D/$(basename "$3")"
    # R-420: the game's own tape beside the binary (inside the app on macOS, where make_app.sh puts it and the bare binary finds it)
    if [ "$1" = macos ]; then sh packaging/make_app.sh "$3" > /dev/null && cp -R dist/Vesperis.app "$D/"; else sh packaging/tape.sh "$D/music"; fi
    (cd "$OUT/zip" && rm -f "vesperis-$V-$1-$2.zip" && zip -q -r -X "vesperis-$V-$1-$2.zip" "vesperis-$V-$1-$2")
}
pack macos arm64 "$OUT/mac-arm64/vesperis"
pack macos x86_64 "$OUT/mac-x86_64/vesperis"
pack windows x86_64 "$OUT/win-x86_64/vesperis.exe"
pack windows arm64 "$OUT/win-aarch64/vesperis.exe"
pack linux arm64 "$OUT/linux-arm64/vesperis"
pack linux x86_64 "$OUT/linux-amd64/vesperis"
ls -la "$OUT"/zip/*.zip
