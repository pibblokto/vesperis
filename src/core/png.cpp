#include <string>
#include "png.h"
#include <cstdio>
#include <vector>
#include <cstring>

namespace {
uint32_t crcTable[256];
bool crcInit = false;
void initCrc() {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crcTable[n] = c;
    }
    crcInit = true;
}
uint32_t crc(const uint8_t* buf, size_t len, uint32_t c = 0xFFFFFFFFu) {
    if (!crcInit) initCrc();
    for (size_t i = 0; i < len; i++) c = crcTable[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    return c;
}
void put32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back((x >> 24) & 255); v.push_back((x >> 16) & 255); v.push_back((x >> 8) & 255); v.push_back(x & 255);
}
void chunk(FILE* f, const char* type, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> buf;
    put32(buf, (uint32_t)data.size());
    size_t start = buf.size();
    buf.insert(buf.end(), type, type + 4);
    buf.insert(buf.end(), data.begin(), data.end());
    uint32_t c = crc(buf.data() + start, buf.size() - start) ^ 0xFFFFFFFFu;
    put32(buf, c);
    fwrite(buf.data(), 1, buf.size(), f);
}
}

bool readPNG(const char* path, std::vector<uint32_t>& px, int& w, int& h) {
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

bool writePNG(const char* path, const uint32_t* px, int w, int h) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    std::vector<uint8_t> ihdr;
    put32(ihdr, w); put32(ihdr, h);
    ihdr.push_back(8); ihdr.push_back(2); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
    chunk(f, "IHDR", ihdr);
    // raw scanlines with filter byte 0
    std::vector<uint8_t> raw;
    raw.reserve((size_t)h * (w * 3 + 1));
    for (int y = 0; y < h; y++) {
        raw.push_back(0);
        for (int x = 0; x < w; x++) {
            uint32_t p = px[y * w + x];
            raw.push_back(p & 255); raw.push_back((p >> 8) & 255); raw.push_back((p >> 16) & 255);
        }
    }
    // zlib stream with stored blocks
    std::vector<uint8_t> z;
    z.push_back(0x78); z.push_back(0x01);
    size_t pos = 0;
    uint32_t a = 1, b = 0;
    for (uint8_t c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
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
    chunk(f, "IDAT", z);
    chunk(f, "IEND", {});
    fclose(f);
    return true;
}
