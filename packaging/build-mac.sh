#!/bin/sh
# F-09: the macOS release binary for one architecture: the Apple clang, raylib static from toolchains.sh, thin LTO (the one flag that
# measured; clang decides the fused multiply-adds per expression before the link, so the pictures stay the -O2 build's).
#   sh packaging/build-mac.sh <arm64|x86_64> <out dir>       -> <out dir>/vesperis
set -e
A=$1
mkdir -p "$2/obj"; OUT=$(cd "$2" && pwd)
cd "$(dirname "$0")/.."
TC=${VESPERIS_TC:-$PWD/build/toolchains}
CXX="/usr/bin/clang++ -arch $A -mmacosx-version-min=11.0"
FLAGS="-O2 -flto=thin"
rm -f "$OUT"/obj/*.o
{ find src/core src/galaxy src/space src/surface src/game -name '*.cpp' | sort; echo src/main_raylib.cpp; echo src/platform_raylib.cpp; } > "$OUT/sources.txt"
export CXX FLAGS TC OUT
xargs -n 1 -P 8 sh -c 'exec $CXX -std=c++17 $FLAGS -Wall -Wno-unused-parameter -Wno-sign-compare -Isrc -I$TC/raylib-5.5/src -pthread \
    -c "$0" -o "$OUT/obj/$(echo "$0" | tr / _ | sed "s/\.cpp\$/.o/")"' < "$OUT/sources.txt"
$CXX $FLAGS -o "$OUT/vesperis" "$OUT"/obj/*.o "$TC/rl-mac-$A/libraylib.a" -framework CoreVideo -framework IOKit -framework Cocoa \
    -framework OpenGL -pthread
ls -la "$OUT/vesperis"
