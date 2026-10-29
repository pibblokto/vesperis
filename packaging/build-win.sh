#!/bin/sh
# F-09: the Windows release binary for one architecture, cross-built with llvm-mingw (clang and lld, the UCRT), raylib static from
# toolchains.sh, thin LTO as on macOS. No Windows machine runs the harness, so this line is untimed: it takes no other optimisation.
# -Wshorten-64-to-32 shows where a 64-bit value narrows: Windows' long is 32 bits, and B-424 was such a narrowing (the tree has none).
#   sh packaging/build-win.sh <x86_64|aarch64> <out dir>     -> <out dir>/vesperis.exe
set -e
A=$1
mkdir -p "$2/obj"; OUT=$(cd "$2" && pwd)
cd "$(dirname "$0")/.."
TC=${VESPERIS_TC:-$PWD/build/toolchains}
CXX=$TC/llvm-mingw-20260922-ucrt-macos-universal/bin/$A-w64-mingw32-clang++
FLAGS="-O2 -flto=thin"
rm -f "$OUT"/obj/*.o
{ find src/core src/galaxy src/peoples src/space src/surface src/game -name '*.cpp' | sort; echo src/main_raylib.cpp; echo src/platform_raylib.cpp; } > "$OUT/sources.txt"
export CXX FLAGS TC OUT
xargs -n 1 -P 8 sh -c 'exec $CXX -std=c++17 $FLAGS -Wall -Wshorten-64-to-32 -Wno-unused-parameter -Wno-sign-compare -Isrc \
    -I$TC/raylib-5.5/src -c "$0" -o "$OUT/obj/$(echo "$0" | tr / _ | sed "s/\.cpp\$/.o/")"' < "$OUT/sources.txt"
$CXX $FLAGS -static -mwindows -o "$OUT/vesperis.exe" "$OUT"/obj/*.o -L"$TC/rl-win-$A" -lraylib -lopengl32 -lgdi32 -lwinmm
ls -la "$OUT/vesperis.exe"
