# F-09: the Linux build image (build-linux.sh): Ubuntu 20.04's glibc 2.31 and GCC 9, so the zips start on older systems; X11 only.
FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libgl1-mesa-dev file && rm -rf /var/lib/apt/lists/*
