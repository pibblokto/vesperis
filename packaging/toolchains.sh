#!/bin/sh
# F-09: the release toolchains, fetched and built once into $VESPERIS_TC (default build/toolchains, gitignored; macOS wipes /tmp):
# raylib 5.5's source, its static library for macOS arm64/x86_64 (the Apple clang) and Windows x86_64/aarch64 (llvm-mingw), and
# llvm-mingw itself. The Linux libraries are built inside the container on first use (build-linux.sh). A second run builds only what
# is missing. Paths must not hold spaces.
#   sh packaging/toolchains.sh
set -e
cd "$(dirname "$0")/.."
TC=${VESPERIS_TC:-$PWD/build/toolchains}
RAYLIB=raylib-5.5
RAYLIB_SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
MINGW=llvm-mingw-20260922-ucrt-macos-universal
MINGW_SHA256=52e5f5a7b131021d0c39a37a38fa380a1da7885cd04bd61afd0cd4ecfb8bc1f3
mkdir -p "$TC"
cd "$TC"
fetch() {   # fetch <file> <sha256> <url>
    if [ ! -f "$1" ]; then curl -fsSL -o "$1.part" "$3"; mv "$1.part" "$1"; fi
    echo "$2  $1" | shasum -a 256 -c - > /dev/null || { echo "$1: wrong checksum" >&2; exit 1; }
}
fetch $RAYLIB.tar.gz $RAYLIB_SHA256 https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
fetch $MINGW.tar.xz $MINGW_SHA256 https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/$MINGW.tar.xz
[ -f $RAYLIB/src/raylib.h ] || { rm -rf $RAYLIB; tar xzf $RAYLIB.tar.gz; }
[ -f $MINGW/generic-w64-mingw32/include/c++/v1/cmath ] || { rm -rf $MINGW; tar xJf $MINGW.tar.xz; }
for A in arm64 x86_64; do
    [ -f rl-mac-$A/libraylib.a ] && continue
    rm -rf rl-mac-$A; mkdir rl-mac-$A; cp -R $RAYLIB/src rl-mac-$A/src
    make -C rl-mac-$A/src PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC RAYLIB_RELEASE_PATH=.. CC=/usr/bin/clang \
        CUSTOM_CFLAGS="-arch $A -mmacosx-version-min=11.0" -j8 > rl-mac-$A.log 2>&1 || { echo "raylib for macOS $A failed: $TC/rl-mac-$A.log" >&2; exit 1; }
done
for A in x86_64 aarch64; do
    [ -f rl-win-$A/libraylib.a ] && continue
    rm -rf rl-win-$A; mkdir rl-win-$A
    for f in rcore rshapes rtextures rtext rmodels utils raudio rglfw; do
        $MINGW/bin/$A-w64-mingw32-clang -c -O2 -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -I$RAYLIB/src -I$RAYLIB/src/external/glfw/include \
            $RAYLIB/src/$f.c -o rl-win-$A/$f.o 2>> rl-win-$A.log || { echo "raylib for Windows $A failed: $TC/rl-win-$A.log" >&2; exit 1; }
    done
    $MINGW/bin/$A-w64-mingw32-ar rcs rl-win-$A/libraylib.a rl-win-$A/*.o
done
ls -la rl-*/libraylib.a
