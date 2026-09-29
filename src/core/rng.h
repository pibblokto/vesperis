// Hashing and pseudo-random generation. All procedural content in the game is
// a pure function of integer coordinates and seeds, so the same star, planet or
// square metre of terrain always comes out identical.
#pragma once
#include <cstdint>

inline uint64_t mix64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}
inline uint64_t hashCombine(uint64_t a, uint64_t b) {
    return mix64(a ^ (b * 0x9E3779B97F4A7C15ULL + 0x632BE59BD9B4E019ULL + (a << 6) + (a >> 2)));
}
inline uint64_t hash3i(int64_t x, int64_t y, int64_t z, uint64_t seed) {
    uint64_t h = seed ^ 0xD6E8FEB86659FD93ULL;
    h = mix64(h ^ ((uint64_t)x * 0x9E3779B97F4A7C15ULL));
    h = mix64(h ^ ((uint64_t)y * 0xC2B2AE3D27D4EB4FULL));
    h = mix64(h ^ ((uint64_t)z * 0x165667B19E3779F9ULL));
    return h;
}
inline uint64_t hash2i(int64_t x, int64_t y, uint64_t seed) { return hash3i(x, y, 0x5bd1e995, seed); }
inline double unitFromHash(uint64_t h) { return (double)(h >> 11) * (1.0 / 9007199254740992.0); }

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
