#include "noise.h"
#include <cmath>

namespace {
inline double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
inline double lerpn(double a, double b, double t) { return a + t * (b - a); }

// 12 edge-of-cube gradients (Perlin's improved set), plus 4 repeats to make 16.
inline double grad3(uint64_t h, double x, double y, double z) {
    switch (h & 15) {
        case 0: return  x + y; case 1: return -x + y; case 2: return  x - y; case 3: return -x - y;
        case 4: return  x + z; case 5: return -x + z; case 6: return  x - z; case 7: return -x - z;
        case 8: return  y + z; case 9: return -y + z; case 10: return y - z; case 11: return -y - z;
        case 12: return y + x; case 13: return -y + z; case 14: return y - x; default: return -y - z;
    }
}
inline double grad2(uint64_t h, double x, double y) {
    switch (h & 7) {
        case 0: return x + y; case 1: return -x + y; case 2: return x - y; case 3: return -x - y;
        case 4: return x * 1.414; case 5: return -x * 1.414; case 6: return y * 1.414; default: return -y * 1.414;
    }
}
inline int64_t ifloor(double v) { return (int64_t)std::floor(v); }
// the gradient vectors behind grad3, for the analytic derivative
inline Vec3 grad3v(uint64_t h) {
    switch (h & 15) {
        case 0: return Vec3(1, 1, 0); case 1: return Vec3(-1, 1, 0); case 2: return Vec3(1, -1, 0); case 3: return Vec3(-1, -1, 0);
        case 4: return Vec3(1, 0, 1); case 5: return Vec3(-1, 0, 1); case 6: return Vec3(1, 0, -1); case 7: return Vec3(-1, 0, -1);
        case 8: return Vec3(0, 1, 1); case 9: return Vec3(0, -1, 1); case 10: return Vec3(0, 1, -1); case 11: return Vec3(0, -1, -1);
        case 12: return Vec3(1, 1, 0); case 13: return Vec3(0, -1, 1); case 14: return Vec3(-1, 1, 0); default: return Vec3(0, -1, -1);
    }
}
inline double dfade(double t) { return 30.0 * t * t * (t * (t - 2) + 1); }
}

double gnoise3(double x, double y, double z, uint64_t seed) {
    int64_t X = ifloor(x), Y = ifloor(y), Z = ifloor(z);
    double fx = x - X, fy = y - Y, fz = z - Z;
    double u = fade(fx), v = fade(fy), w = fade(fz);
    double n000 = grad3(hash3i(X, Y, Z, seed), fx, fy, fz);
    double n100 = grad3(hash3i(X + 1, Y, Z, seed), fx - 1, fy, fz);
    double n010 = grad3(hash3i(X, Y + 1, Z, seed), fx, fy - 1, fz);
    double n110 = grad3(hash3i(X + 1, Y + 1, Z, seed), fx - 1, fy - 1, fz);
    double n001 = grad3(hash3i(X, Y, Z + 1, seed), fx, fy, fz - 1);
    double n101 = grad3(hash3i(X + 1, Y, Z + 1, seed), fx - 1, fy, fz - 1);
    double n011 = grad3(hash3i(X, Y + 1, Z + 1, seed), fx, fy - 1, fz - 1);
    double n111 = grad3(hash3i(X + 1, Y + 1, Z + 1, seed), fx - 1, fy - 1, fz - 1);
    double x00 = lerpn(n000, n100, u), x10 = lerpn(n010, n110, u);
    double x01 = lerpn(n001, n101, u), x11 = lerpn(n011, n111, u);
    double y0 = lerpn(x00, x10, v), y1 = lerpn(x01, x11, v);
    return lerpn(y0, y1, w) * 1.05;
}

// O6-02: value and gradient. The value is the trilinear blend of the corner dot products with the faded weights; its
// derivative has two parts: the fade weights' derivative times the differences of the corner dots, and the trilinear
// blend of the corner gradient vectors themselves (Quilez's "noised").
double gnoise3d(double x, double y, double z, uint64_t seed, Vec3& grad) {
    int64_t X = ifloor(x), Y = ifloor(y), Z = ifloor(z);
    double fx = x - X, fy = y - Y, fz = z - Z;
    double u = fade(fx), v = fade(fy), w = fade(fz);
    double du = dfade(fx), dv = dfade(fy), dw = dfade(fz);
    Vec3 ga = grad3v(hash3i(X, Y, Z, seed)), gb = grad3v(hash3i(X + 1, Y, Z, seed));
    Vec3 gc = grad3v(hash3i(X, Y + 1, Z, seed)), gd = grad3v(hash3i(X + 1, Y + 1, Z, seed));
    Vec3 ge = grad3v(hash3i(X, Y, Z + 1, seed)), gf = grad3v(hash3i(X + 1, Y, Z + 1, seed));
    Vec3 gg = grad3v(hash3i(X, Y + 1, Z + 1, seed)), gh = grad3v(hash3i(X + 1, Y + 1, Z + 1, seed));
    double va = dot(ga, Vec3(fx, fy, fz)), vb = dot(gb, Vec3(fx - 1, fy, fz));
    double vc = dot(gc, Vec3(fx, fy - 1, fz)), vd = dot(gd, Vec3(fx - 1, fy - 1, fz));
    double ve = dot(ge, Vec3(fx, fy, fz - 1)), vf = dot(gf, Vec3(fx - 1, fy, fz - 1));
    double vg = dot(gg, Vec3(fx, fy - 1, fz - 1)), vh = dot(gh, Vec3(fx - 1, fy - 1, fz - 1));
    double k1 = vb - va, k2 = vc - va, k3 = ve - va, k4 = va - vb - vc + vd, k5 = va - vc - ve + vg, k6 = va - vb - ve + vf, k7 = -va + vb + vc - vd + ve - vf - vg + vh;
    double value = va + u * k1 + v * k2 + w * k3 + u * v * k4 + v * w * k5 + w * u * k6 + u * v * w * k7;
    Vec3 gi = ga + (gb - ga) * u + (gc - ga) * v + (ge - ga) * w + (ga - gb - gc + gd) * (u * v) + (ga - gc - ge + gg) * (v * w) + (ga - gb - ge + gf) * (w * u) + (-ga + gb + gc - gd + ge - gf - gg + gh) * (u * v * w);
    grad.x = (gi.x + du * (k1 + v * k4 + w * k6 + v * w * k7)) * 1.05;
    grad.y = (gi.y + dv * (k2 + u * k4 + w * k5 + u * w * k7)) * 1.05;
    grad.z = (gi.z + dw * (k3 + v * k5 + u * k6 + u * v * k7)) * 1.05;
    return value * 1.05;
}

double gnoise2(double x, double y, uint64_t seed) {
    int64_t X = ifloor(x), Y = ifloor(y);
    double fx = x - X, fy = y - Y;
    double u = fade(fx), v = fade(fy);
    double n00 = grad2(hash2i(X, Y, seed), fx, fy);
    double n10 = grad2(hash2i(X + 1, Y, seed), fx - 1, fy);
    double n01 = grad2(hash2i(X, Y + 1, seed), fx, fy - 1);
    double n11 = grad2(hash2i(X + 1, Y + 1, seed), fx - 1, fy - 1);
    return lerpn(lerpn(n00, n10, u), lerpn(n01, n11, u), v) * 0.75;
}

double fbm3(const Vec3& p, uint64_t seed, int octaves, double lacunarity, double gain) {
    double sum = 0, amp = 1, norm = 0;
    Vec3 q = p;
    for (int i = 0; i < octaves; i++) {
        sum += amp * gnoise3(q.x, q.y, q.z, seed + i * 7919);
        norm += amp;
        amp *= gain;
        q = q * lacunarity + Vec3(17.3, 9.1, 3.7);
    }
    return norm > 0 ? sum / norm : 0;
}

double fbm2(double x, double y, uint64_t seed, int octaves, double lacunarity, double gain) {
    double sum = 0, amp = 1, norm = 0;
    for (int i = 0; i < octaves; i++) {
        sum += amp * gnoise2(x, y, seed + i * 7919);
        norm += amp;
        amp *= gain;
        x = x * lacunarity + 11.7; y = y * lacunarity + 5.3;
    }
    return norm > 0 ? sum / norm : 0;
}

double ridged3(const Vec3& p, uint64_t seed, int octaves, double lacunarity, double gain) {
    double sum = 0, amp = 1, norm = 0, weight = 1;
    Vec3 q = p;
    for (int i = 0; i < octaves; i++) {
        double n = gnoise3(q.x, q.y, q.z, seed + i * 104729);
        n = 1.0 - std::fabs(n);
        n = n * n * weight;
        weight = clampd(n * 2.0, 0.0, 1.0);
        sum += n * amp;
        norm += amp;
        amp *= gain;
        q = q * lacunarity + Vec3(3.1, 7.7, 1.3);
    }
    return norm > 0 ? sum / norm : 0;
}

Worley3 worley3(const Vec3& p, uint64_t seed) {
    int64_t X = ifloor(p.x), Y = ifloor(p.y), Z = ifloor(p.z);
    Worley3 r;
    r.f1 = 1e30; r.f2 = 1e30; r.id1 = 0; r.id2 = 0;
    for (int dz = -1; dz <= 1; dz++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int64_t cx = X + dx, cy = Y + dy, cz = Z + dz;
                uint64_t h = hash3i(cx, cy, cz, seed);
                Vec3 c((double)cx + unitFromHash(h), (double)cy + unitFromHash(mix64(h + 1)),
                       (double)cz + unitFromHash(mix64(h + 2)));
                double d = length2(c - p);
                if (d < r.f1) { r.f2 = r.f1; r.id2 = r.id1; r.f1 = d; r.id1 = h; r.c1 = c; }
                else if (d < r.f2) { r.f2 = d; r.id2 = h; }
            }
    r.f1 = std::sqrt(r.f1);
    r.f2 = std::sqrt(r.f2);
    return r;
}

void GrainTexture::resize(int size) {
    n = size; mask = size - 1;
    shift = 0; while ((1 << shift) < size) shift++;
    v.assign((size_t)size * size, 0);
}

void GrainTexture::generate(uint64_t seed, int amplitude, int smooth) {
    Rng rng(seed);
    std::vector<int> tmp((size_t)n * n), t2((size_t)n * n);
    for (int i = 0; i < n * n; i++) tmp[i] = rng.irange(2 * amplitude + 1) - amplitude;
    for (int s = 0; s < smooth; s++) {
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++) {
                int a = tmp[(y << shift) | x] + tmp[(y << shift) | ((x + 1) & mask)] + tmp[(((y + 1) & mask) << shift) | x] +
                        tmp[(((y + 1) & mask) << shift) | ((x + 1) & mask)];
                t2[(y << shift) | x] = a / 4;
            }
        tmp.swap(t2);
    }
    for (int i = 0; i < n * n; i++) v[i] = (int8_t)clampi(tmp[i], -127, 127);
}
