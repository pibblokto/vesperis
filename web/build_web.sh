#!/bin/sh
# M6-04: build the browser version with Emscripten (needs emsdk activated: `source emsdk_env.sh`).
#   sh web/build_web.sh                     -> build-web/vesperis.html (+ .js, .wasm)
#   python3 -m http.server -d build-web      then open http://localhost:8000/vesperis.html
# raylib is fetched and built for the web platform by CMake (PLATFORM=Web); threads need the COOP/COEP headers
# in production hosting (python's http.server is fine for a local try).
set -e
cd "$(dirname "$0")/.."
command -v emcmake >/dev/null 2>&1 || { echo "emcmake not found: install and activate emsdk first"; exit 1; }
emcmake cmake -B build-web -DVESPERIS_FETCH_RAYLIB=ON -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web -j4 --target vesperis
echo "built build-web/vesperis.html"
