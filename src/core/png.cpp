#include "png.h"
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <array>
#ifndef __EMSCRIPTEN__
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#endif

namespace {
// F-02: built once at its first use (a function-local static is initialised once, safely, from the writer thread too)
const std::array<uint32_t, 256>& crcTable() {
    static const std::array<uint32_t, 256> t = [] {
        std::array<uint32_t, 256> a{};
        for (uint32_t n = 0; n < 256; n++) {
            uint32_t c = n;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            a[n] = c;
        }
        return a;
    }();
    return t;
}
uint32_t crc(const uint8_t* buf, size_t len, uint32_t c = 0xFFFFFFFFu) {
    const std::array<uint32_t, 256>& T = crcTable();
    for (size_t i = 0; i < len; i++) c = T[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    return c;
}
void put32(uint8_t* o, uint32_t x) { o[0] = (x >> 24) & 255; o[1] = (x >> 16) & 255; o[2] = (x >> 8) & 255; o[3] = x & 255; }
void put32(std::vector<uint8_t>& v, uint32_t x) { uint8_t b[4]; put32(b, x); v.insert(v.end(), b, b + 4); }
// a chunk: its length, its type, its data and the CRC of the type and the data (F-02: the CRC runs over the type, then the data, in
// place: the data is not copied behind the type first)
void chunk(FILE* f, const char* type, const uint8_t* data, size_t n) {
    uint8_t head[8];
    put32(head, (uint32_t)n);
    memcpy(head + 4, type, 4);
    fwrite(head, 1, 8, f);
    if (n) fwrite(data, 1, n, f);
    uint8_t tail[4];
    put32(tail, crc(data, n, crc(head + 4, 4)) ^ 0xFFFFFFFFu);
    fwrite(tail, 1, 4, f);
}
}

bool readPNG(const char* path, std::vector<uint32_t>& px, int& w, int& h) {
    pngFlush();   // F-02: a picture still queued for the writer is on disk first
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    std::vector<uint8_t> d;
    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) d.insert(d.end(), buf, buf + n);
    fclose(f);
    if (d.size() < 33 || d[1] != 'P' || d[2] != 'N' || d[3] != 'G') return false;
    auto get32 = [&](size_t o) { return ((uint32_t)d[o] << 24) | ((uint32_t)d[o + 1] << 16) | ((uint32_t)d[o + 2] << 8) | d[o + 3]; };
    size_t pos = 8;
    std::vector<uint8_t> idat;
    w = h = 0; int colorType = -1;
    while (pos + 8 <= d.size()) {
        uint32_t len = get32(pos);
        std::string type((const char*)&d[pos + 4], 4);
        if (pos + 12 + len > d.size()) return false;
        if (type == "IHDR") { w = (int)get32(pos + 8); h = (int)get32(pos + 12); colorType = d[pos + 17]; if (d[pos + 16] != 8) return false; }
        else if (type == "IDAT") idat.insert(idat.end(), d.begin() + pos + 8, d.begin() + pos + 8 + len);
        else if (type == "IEND") break;
        pos += 12 + len;
    }
    if (w <= 0 || h <= 0 || (colorType != 2 && colorType != 6) || idat.size() < 6) return false;
    int bpp = colorType == 2 ? 3 : 4;
    // inflate stored blocks only
    std::vector<uint8_t> raw;
    size_t p = 2;
    while (p + 5 <= idat.size()) {
        uint8_t hdr = idat[p];
        if ((hdr & 6) != 0) return false;   // compressed block: not ours
        uint32_t len = idat[p + 1] | (idat[p + 2] << 8);
        p += 5;
        if (p + len > idat.size()) return false;
        raw.insert(raw.end(), idat.begin() + p, idat.begin() + p + len);
        p += len;
        if (hdr & 1) break;
    }
    size_t stride = (size_t)w * bpp + 1;
    if (raw.size() < stride * h) return false;
    px.assign((size_t)w * h, 0);
    for (int y = 0; y < h; y++) {
        const uint8_t* row = raw.data() + y * stride;
        if (row[0] != 0) return false;   // only filter 0
        for (int x = 0; x < w; x++) {
            const uint8_t* c = row + 1 + x * bpp;
            px[(size_t)y * w + x] = 0xFF000000u | ((uint32_t)c[2] << 16) | ((uint32_t)c[1] << 8) | c[0];
        }
    }
    return true;
}

// F-02: the same bytes as before, about three times sooner (16-19 ms at 4x before): the rows packed through a pointer, not three
// push_backs a pixel; the Adler-32 sums reduced once a run of 5,552 bytes (zlib's NMAX: the most bytes after which `b` still fits 32
// bits, so the sums are the per-byte modulo's exactly); the chunks' data written in place, not copied behind their type
bool writePNG(const char* path, const uint32_t* px, int w, int h) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13];
    put32(ihdr, w); put32(ihdr + 4, h);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, sizeof(ihdr));
    // raw scanlines with filter byte 0
    const size_t stride = (size_t)w * 3 + 1;
    std::vector<uint8_t> raw((size_t)h * stride);
    for (int y = 0; y < h; y++) {
        uint8_t* o = raw.data() + (size_t)y * stride;
        *o++ = 0;
        const uint32_t* row = px + (size_t)y * w;
        for (int x = 0; x < w; x++, o += 3) {
            const uint32_t p = row[x];
            o[0] = p & 255; o[1] = (p >> 8) & 255; o[2] = (p >> 16) & 255;
        }
    }
    // zlib stream with stored blocks
    const size_t blocks = (raw.size() + 65534) / 65535;
    std::vector<uint8_t> z;
    z.reserve(2 + raw.size() + 5 * std::max<size_t>(blocks, 1) + 4);
    z.push_back(0x78); z.push_back(0x01);
    size_t pos = 0;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < raw.size();) {
        const size_t end = std::min(raw.size(), i + 5552);
        for (; i < end; i++) { a += raw[i]; b += a; }
        a %= 65521; b %= 65521;
    }
    while (pos < raw.size()) {
        size_t n = std::min<size_t>(65535, raw.size() - pos);
        bool last = pos + n >= raw.size();
        z.push_back(last ? 1 : 0);
        z.push_back(n & 255); z.push_back((n >> 8) & 255);
        z.push_back((~n) & 255); z.push_back(((~n) >> 8) & 255);
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
    }
    put32(z, (b << 16) | a);
    chunk(f, "IDAT", z.data(), z.size());
    chunk(f, "IEND", nullptr, 0);
    fclose(f);
    return true;
}

#ifdef __EMSCRIPTEN__
void writePNGAsync(const char* path, const uint32_t* px, int w, int h) { writePNG(path, px, w, h); }   // the browser's threads are the pool's
void pngFlush() {}
#else
namespace {
// F-02: one thread that writes the pictures queued for it in their order, the caller blocked only while three wait (no picture is
// dropped, and the names stay in order); the pixel buffers kept for the next pictures (a fresh 4 MB a frame cost a millisecond of
// page faults). Destroyed at the exit after writing what is queued
class PngWriter {
public:
    static PngWriter& instance() { static PngWriter w; return w; }
    void push(const char* path, const uint32_t* px, int w, int h) {
        std::unique_lock<std::mutex> lock(m);
        room.wait(lock, [&] { return q.size() < 3; });
        Job j;
        if (!spare.empty()) { j.px = std::move(spare.back()); spare.pop_back(); }
        lock.unlock();
        j.path = path; j.w = w; j.h = h;
        j.px.assign(px, px + (size_t)w * h);
        lock.lock();
        q.push_back(std::move(j));
        if (!th.joinable()) th = std::thread([this] { loop(); });
        work.notify_one();
    }
    void flush() {
        std::unique_lock<std::mutex> lock(m);
        idle.wait(lock, [&] { return q.empty() && !busy; });
    }
    ~PngWriter() {
        { std::lock_guard<std::mutex> lock(m); quit = true; }
        work.notify_one();
        if (th.joinable()) th.join();
    }
private:
    struct Job { std::string path; std::vector<uint32_t> px; int w = 0, h = 0; };
    void loop() {
        for (;;) {
            Job j;
            {
                std::unique_lock<std::mutex> lock(m);
                work.wait(lock, [&] { return quit || !q.empty(); });
                if (q.empty()) return;   // quit, and everything queued written
                j = std::move(q.front()); q.pop_front();
                busy = true;
            }
            room.notify_one();
            writePNG(j.path.c_str(), j.px.data(), j.w, j.h);
            {
                std::lock_guard<std::mutex> lock(m);
                if (spare.size() < 4) spare.push_back(std::move(j.px));
                busy = false;
            }
            idle.notify_all();
        }
    }
    std::mutex m;
    std::condition_variable work, room, idle;
    std::deque<Job> q;
    std::vector<std::vector<uint32_t>> spare;
    bool busy = false, quit = false;
    std::thread th;
};
}

void writePNGAsync(const char* path, const uint32_t* px, int w, int h) { PngWriter::instance().push(path, px, w, h); }
void pngFlush() { PngWriter::instance().flush(); }
#endif
