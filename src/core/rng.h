// Hashing and pseudo-random generation. All procedural content in the game is
// a pure function of integer coordinates and seeds, so the same star, planet or
// square metre of terrain always comes out identical.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

inline uint64_t mix64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}
inline uint64_t hashCombine(uint64_t a, uint64_t b) {
    return mix64(a ^ (b * 0x9E3779B97F4A7C15ULL + 0x632BE59BD9B4E019ULL + (a << 6) + (a >> 2)));
}
// hash3i in its three steps (F-07): a walk over a block of cells mixes each x once and each (x, y) once
inline uint64_t hashX(int64_t x, uint64_t seed) { return mix64((seed ^ 0xD6E8FEB86659FD93ULL) ^ ((uint64_t)x * 0x9E3779B97F4A7C15ULL)); }
inline uint64_t hashXY(uint64_t hx, int64_t y) { return mix64(hx ^ ((uint64_t)y * 0xC2B2AE3D27D4EB4FULL)); }
inline uint64_t hashXYZ(uint64_t hxy, int64_t z) { return mix64(hxy ^ ((uint64_t)z * 0x165667B19E3779F9ULL)); }
inline uint64_t hash3i(int64_t x, int64_t y, int64_t z, uint64_t seed) { return hashXYZ(hashXY(hashX(x, seed), y), z); }
inline uint64_t hash2i(int64_t x, int64_t y, uint64_t seed) { return hash3i(x, y, 0x5bd1e995, seed); }
inline double unitFromHash(uint64_t h) { return (double)(h >> 11) * (1.0 / 9007199254740992.0); }
inline double hash01(uint64_t h) { return unitFromHash(h); }   // the same, short (the surface's drawing reads hundreds)
// FNV-1a (64 bits) over a string's bytes: what keys a text (a grammar alternative by its rule and line, a word, a frame)
inline uint64_t fnv1a(const std::string& s) { uint64_t h = 1469598103934665603ULL; for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; } return h; }

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed = 1) : s(mix64(seed ^ 0xA0761D6478BD642FULL)) {}
    uint64_t next() { s += 0x9E3779B97F4A7C15ULL; return mix64(s); }
    double uni() { return unitFromHash(next()); }                  // [0,1)
    double range(double a, double b) { return a + (b - a) * uni(); }
    double sym(double a) { return range(-a, a); }
    int irange(int n) { return n <= 0 ? 0 : (int)(uni() * n); }    // [0,n)
    bool chance(double p) { return uni() < p; }
    // Weighted pick: weights need not sum to 1.
    int pick(const double* w, int n) {
        double total = 0;
        for (int i = 0; i < n; i++) total += w[i];
        double r = uni() * total;
        for (int i = 0; i < n; i++) { r -= w[i]; if (r < 0) return i; }
        return n - 1;
    }
};

// F-05: xorshift32 (13, 17, 5), the camera feeds' sensor noise walked along the rows, is linear over GF(2): n steps are a 32x32 bit
// matrix, held here by its columns (the images of the 32 bits). A band of rows starts from the state the serial walk reaches at its
// first row, so the feed's rows go to the pool and every pixel draws the number it drew before
inline uint32_t xorshift32(uint32_t s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
struct XorshiftJump {
    uint32_t col[32] = {};
    int steps = -1;
    void make(int n) { steps = n; for (int j = 0; j < 32; j++) { uint32_t s = 1u << j; for (int i = 0; i < n; i++) s = xorshift32(s); col[j] = s; } }
    uint32_t apply(uint32_t s) const { uint32_t r = 0; for (int j = 0; j < 32; j++) if ((s >> j) & 1u) r ^= col[j]; return r; }
};
// the state at the start of each of `rows` rows of `w` steps from `seed` (out[0] is the seed); the row's jump is kept per width (the
// main thread's: the feeds run after the mush, outside the pool)
inline void xorshiftRowStarts(uint32_t seed, int w, int rows, std::vector<uint32_t>& out) {
    static XorshiftJump jump;
    if (jump.steps != w) jump.make(w);
    out.resize(rows > 0 ? rows : 0);
    if (rows > 0) out[0] = seed;
    for (int y = 1; y < rows; y++) out[y] = jump.apply(out[y - 1]);
}
