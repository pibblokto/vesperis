#!/bin/sh
# F-09: the Linux release binary for one architecture, built inside an Ubuntu 20.04 container (linux.Dockerfile, the image
# vesperis-build:<arch> made on first use) by build-linux-inner.sh; amd64 runs emulated on an arm64 Mac (slow, fine). Needs Docker
# and the raylib source from toolchains.sh.
#   sh packaging/build-linux.sh <arm64|amd64> <out dir>      -> <out dir>/vesperis
set -e
A=$1
mkdir -p "$2"; OUT=$(cd "$2" && pwd)
cd "$(dirname "$0")/.."
TC=${VESPERIS_TC:-$PWD/build/toolchains}
docker image inspect vesperis-build:$A > /dev/null 2>&1 ||
    docker build --platform linux/$A -t vesperis-build:$A -f packaging/linux.Dockerfile packaging
docker run --rm --platform linux/$A -v "$TC":/w -v "$PWD":/src:ro -v "$OUT":/out vesperis-build:$A sh /src/packaging/build-linux-inner.sh
