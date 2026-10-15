#!/bin/sh
# F-09: runs inside the build container (build-linux.sh): /src the tree (read-only), /w the toolchains, /out the output. Builds
# raylib for this architecture on first use (X11 only), then the game, its C++ runtime linked in (GLFW opens X11 and GL itself).
# -O3: GCC 9's -O2 does not vectorise and inlines only small functions; -O3 measured 5% faster in the arm64 container, every picture
# the -O2 build's on both architectures; -flto was slower here and -fno-math-errno changed nothing (10-decisions.md, F-09).
set -e
A=$(uname -m)
RL=/w/rl-linux-$A
if [ ! -f $RL/libraylib.a ]; then
    rm -rf $RL; mkdir -p $RL; cp -R /w/raylib-5.5/src $RL/src
    make -C $RL/src PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC RAYLIB_RELEASE_PATH=.. GLFW_LINUX_ENABLE_WAYLAND=FALSE \
        GLFW_LINUX_ENABLE_X11=TRUE -j8 > /out/raylib-$A.log 2>&1
fi
FLAGS="-O3"
mkdir -p /out/obj; rm -f /out/obj/*.o
cd /src
{ find src/core src/galaxy src/space src/surface src/game -name '*.cpp' | sort; echo src/main_raylib.cpp; echo src/platform_raylib.cpp; } > /out/sources.txt
export FLAGS
xargs -n 1 -P 8 sh -c 'exec g++ -std=c++17 $FLAGS -Wall -Wno-unused-parameter -Wno-sign-compare -Isrc -I/w/raylib-5.5/src -pthread \
    -c "$0" -o "/out/obj/$(echo "$0" | tr / _ | sed "s/\.cpp\$/.o/")"' < /out/sources.txt
g++ $FLAGS -o /out/vesperis /out/obj/*.o $RL/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11 -static-libstdc++ -static-libgcc
ls -la /out/vesperis
