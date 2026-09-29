#pragma once
#include <cstdint>
// Minimal PNG writer (uncompressed deflate blocks). Input is 0xAABBGGRR pixels.
bool writePNG(const char* path, const uint32_t* px, int w, int h);
#include <vector>
// Reads PNGs written by writePNG (8-bit RGB, filter 0, stored deflate blocks). Other PNGs fail.
bool readPNG(const char* path, std::vector<uint32_t>& px, int& w, int& h);
