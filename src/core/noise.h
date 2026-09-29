// Gradient (Perlin-style) noise in 2D and 3D, fractal sums and Worley noise.
// Double precision so that planetary-scale coordinates (metres on a body with a
// radius of several thousand kilometres) keep sub-metre accuracy.
#pragma once
#include "types.h"
#include "rng.h"

double gnoise3(double x, double y, double z, uint64_t seed);  // approx [-1,1]
// O6-02: the same noise with its analytic gradient (d/dx, d/dy, d/dz of the value), for slope-aware fractal sums
double gnoise3d(double x, double y, double z, uint64_t seed, Vec3& grad);
double gnoise2(double x, double y, uint64_t seed);
inline double gnoise3(const Vec3& p, uint64_t seed) { return gnoise3(p.x, p.y, p.z, seed); }

// Fractal Brownian motion; `octaves` may be fractional-free int. Result in ~[-1,1].
double fbm3(const Vec3& p, uint64_t seed, int octaves, double lacunarity = 2.0, double gain = 0.5);
double fbm2(double x, double y, uint64_t seed, int octaves, double lacunarity = 2.0, double gain = 0.5);
// Ridged multifractal: sharp crests, result in [0,1].
double ridged3(const Vec3& p, uint64_t seed, int octaves, double lacunarity = 2.0, double gain = 0.5);

struct Worley3 {
    double f1, f2;   // distance to nearest and second nearest feature point
    uint64_t id1, id2;   // hashes of the nearest and second nearest feature points
    Vec3 c1;         // nearest feature point
};
Worley3 worley3(const Vec3& p, uint64_t seed);

// Precomputed tileable grain texture (signed offsets in shade units) used to roughen the
// look of surfaces the way Noctis' random ground textures did. 64x64 by default; the
// material tiles of M10-05 use 128x128.
#include <vector>
struct GrainTexture {
    int n = 64, mask = 63, shift = 6;
    std::vector<int8_t> v = std::vector<int8_t>(64 * 64, 0);
    void resize(int size);   // power of two
    void generate(uint64_t seed, int amplitude, int smooth);   // smoothed white noise
    inline int8_t at(int u, int vv) const { return v[((vv & mask) << shift) | (u & mask)]; }
    // bilinear, for tiles whose texels span more than a pixel on screen (B-311: the broad tone tile, the far rings' tiles)
    inline double atLerp(double u, double vv) const {
        int iu = (int)u; if ((double)iu > u) iu--;
        int iv = (int)vv; if ((double)iv > vv) iv--;
        int fu = (int)((u - iu) * 256), fv = (int)((vv - iv) * 256);
        const int8_t* r0 = &v[(size_t)(iv & mask) << shift];
        const int8_t* r1 = &v[(size_t)((iv + 1) & mask) << shift];
        int u0 = iu & mask, u1 = (iu + 1) & mask;
        int top = r0[u0] * (256 - fu) + r0[u1] * fu, bot = r1[u0] * (256 - fu) + r1[u1] * fu;
        return (top * (256 - fv) + bot * fv) * (1.0 / 65536.0);
    }
};
