#pragma once
#include <cstdint>
// Minimal PNG writer (uncompressed deflate blocks). Input is 0xAABBGGRR pixels.
bool writePNG(const char* path, const uint32_t* px, int w, int h);
// F-02: the same file written by a thread of its own, in the order asked: the pixels are copied before it returns (the caller waits only
// while three pictures are queued). A picture that is read back, listed or must exist at the exit is flushed first: `readPNG` flushes,
// the gallery flushes before it lists the directory, and the queue is written out at the exit. Synchronous in the browser
void writePNGAsync(const char* path, const uint32_t* px, int w, int h);
void pngFlush();   // returns once every picture queued is on disk
#include <vector>
// Reads PNGs written by writePNG (8-bit RGB, filter 0, stored deflate blocks). Other PNGs fail.
bool readPNG(const char* path, std::vector<uint32_t>& px, int& w, int& h);
