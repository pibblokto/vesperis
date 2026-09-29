#include "framebuffer.h"
#include "parallel.h"
#include <cstring>
#include <cmath>
#include <algorithm>

int FBW = UW, FBH = UH, FB_SCALE = 1;
int g_mushKernel = 2;

void setFramebufferScale(int scale) {
    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;
    FB_SCALE = scale;
    FBW = UW * scale;
    FBH = UH * scale;
    g_mushKernel = scale + 1;
}

Framebuffer::Framebuffer() : idx((size_t)FBW * FBH, 0), invz((size_t)FBW * FBH, 0.f) {
    memset(pal, 0, sizeof(pal));
}
void Framebuffer::clear(Pix v) { std::fill(idx.begin(), idx.end(), v); }
void Framebuffer::clearDepth() { std::fill(invz.begin(), invz.end(), 0.f); }

void Framebuffer::mush(int passes) {
    const int W = FBW, H = FBH;
    if ((int)idx.size() != W * H) return;
    int k = g_mushKernel < 1 ? 1 : g_mushKernel;
    static std::vector<Pix> tmp;
    static std::vector<uint32_t> hs;   // horizontal window sums of the intensity
    tmp.resize(idx.size());
    hs.resize(idx.size());
    for (int p = 0; p < passes; p++) {
        memcpy(tmp.data(), idx.data(), idx.size() * sizeof(Pix));
        const Pix* s = tmp.data();
        Pix* d = idx.data();
        if (k == 2) {
            // the original kernel: 2x2 block starting one row below, bank from one row below
            for (int y = 0; y < H - 2; y++) {
                const Pix* r0 = s + (y + 1) * W;
                const Pix* r1 = s + (y + 2) * W;
                Pix* o = d + y * W;
                for (int x = 0; x < W - 1; x++) {
                    unsigned a = (r0[x] & INTEN_MASK) + (r0[x + 1] & INTEN_MASK) + (r1[x] & INTEN_MASK) + (r1[x + 1] & INTEN_MASK);
                    o[x] = (Pix)((r0[x] & BANK_MASK) | (a >> 2));
                }
                o[W - 1] = r0[W - 1];
            }
            continue;
        }
        // general k x k box (windows clamped at the right edge), separable running sums; rows in parallel
        uint32_t* hsp = hs.data();
        parallelFor(H, 64, [&](int yb, int ye) {
            for (int y = yb; y < ye; y++) {
                const Pix* r = s + y * W;
                uint32_t* h = hsp + y * W;
                uint32_t sum = 0;
                for (int x = 0; x < k && x < W; x++) sum += r[x] & INTEN_MASK;
                for (int x = 0; x < W; x++) {
                    h[x] = sum;
                    int leave = x, enter = x + k;
                    sum -= r[leave] & INTEN_MASK;
                    if (enter < W) sum += r[enter] & INTEN_MASK;
                }
            }
        });
        parallelFor(H, 64, [&](int yb, int ye) {
            for (int y = yb; y < ye; y++) {
                Pix* o = d + y * W;
                int y0 = y + 1, y1 = std::min(H - 1, y + k);   // rows y+1 .. y+k
                if (y0 >= H) { memcpy(o, s + (H - 1) * W, W * sizeof(Pix)); continue; }
                int rows = y1 - y0 + 1;
                const Pix* bankRow = s + y0 * W;
                for (int x = 0; x < W; x++) {
                    int cols = std::min(k, W - x);
                    uint32_t a = 0;
                    for (int yy = y0; yy <= y1; yy++) a += hsp[yy * W + x];
                    o[x] = (Pix)((bankRow[x] & BANK_MASK) | (a / (uint32_t)(rows * cols)));
                }
            }
        });
    }
}

void Framebuffer::glowDisc(double cx, double cy, double radius, double coreRadius, int maxAdd, int bankIfEmpty, bool skipOccluded) {
    if (radius <= 0) return;
    int x0 = (int)std::floor(cx - radius), x1 = (int)std::ceil(cx + radius);
    int y0 = (int)std::floor(cy - radius), y1 = (int)std::ceil(cy + radius);
    if (x1 < 0 || y1 < 0 || x0 >= FBW || y0 >= FBH) return;
    x0 = std::max(x0, 0); y0 = std::max(y0, 0); x1 = std::min(x1, FBW - 1); y1 = std::min(y1, FBH - 1);
    double r2 = radius * radius;
    for (int y = y0; y <= y1; y++) {
        double dy = y + 0.5 - cy;
        for (int x = x0; x <= x1; x++) {
            double dx = x + 0.5 - cx;
            double d2 = dx * dx + dy * dy;
            if (d2 >= r2) continue;
            double d = std::sqrt(d2);
            double t = d <= coreRadius ? 1.0 : 1.0 - (d - coreRadius) / (radius - coreRadius + 1e-9);
            int add = (int)(maxAdd * INTEN_PER_SHADE * t * t + 0.5);
            if (add <= 0) continue;
            if (skipOccluded && invz[y * FBW + x] > 1e-12f) continue;
            Pix& p = idx[y * FBW + x];
            int bank = bankOf(p);
            int v = intenOf(p);
            if (v == 0 && bankIfEmpty >= 0) bank = bankIfEmpty;
            p = pixI(bank, v + add);
        }
    }
}

void Framebuffer::toRGB(uint32_t* out, double gain, bool dither) const {
    static std::vector<uint32_t> lut;
    lut.resize(BANKS * 256);
    for (int b = 0; b < BANKS; b++) {
        for (int j = 0; j < 256; j++) {
            int s0 = j >> 2, s1 = std::min(63, s0 + 1);
            double t = (j & 3) * 0.25;
            const uint8_t* c0 = pal + (b * 64 + s0) * 3;
            const uint8_t* c1 = pal + (b * 64 + s1) * 3;
            int r = (int)((c0[0] + (c1[0] - c0[0]) * t) * gain + 0.5);
            int g = (int)((c0[1] + (c1[1] - c0[1]) * t) * gain + 0.5);
            int bl = (int)((c0[2] + (c1[2] - c0[2]) * t) * gain + 0.5);
            r = std::min(255, r); g = std::min(255, g); bl = std::min(255, bl);
            lut[b * 256 + j] = 0xFF000000u | ((uint32_t)bl << 16) | ((uint32_t)g << 8) | (uint32_t)r;
        }
    }
    const Pix* s = idx.data();
    const uint32_t* L = lut.data();
    if (!dither) {
        parallelFor(FBH, 100, [&](int yb, int ye) {
            for (size_t i = (size_t)yb * FBW, e = (size_t)ye * FBW; i < e; i++) out[i] = L[(s[i] >> BANK_SHIFT) * 256 + ((s[i] & INTEN_MASK) >> 3)];   // 2048 levels -> 256
        });
        return;
    }
    // M6-01: an ordered 2x2 dither over the 64 palette stops: the intensity is nudged by -1.5..+1.5 sixteenths of a
    // stop per cell, so gradients break into the original's chequered steps instead of smooth ramps
    static const int BAYER[4] = {-12, 4, 12, -4};   // in intensity units (32 per stop)
    parallelFor(FBH, 100, [&](int yb, int ye) {
        for (int y = yb; y < ye; y++)
            for (int x = 0; x < FBW; x++) {
                size_t i = (size_t)y * FBW + x;
                int inten = (int)(s[i] & INTEN_MASK) + BAYER[((y >> (FB_SCALE > 1 ? 1 : 0)) & 1) * 2 + ((x >> (FB_SCALE > 1 ? 1 : 0)) & 1)];
                inten = inten < 0 ? 0 : (inten > (int)INTEN_MASK ? (int)INTEN_MASK : inten);
                out[i] = L[(s[i] >> BANK_SHIFT) * 256 + (((inten >> 5) << 2))];   // snap to whole stops: the dither replaces the interpolation
            }
    });
}

void setRampFromVector(uint8_t* pal, int bank, const std::vector<std::pair<double, RGB>>& stops) {
    if (stops.empty() || bank < 0 || bank >= BANKS) return;
    for (int i = 0; i < 64; i++) {
        double p = i;
        RGB c = stops.front().second;
        if (p <= stops.front().first) c = stops.front().second;
        else if (p >= stops.back().first) c = stops.back().second;
        else {
            for (size_t k = 0; k + 1 < stops.size(); k++) {
                if (p >= stops[k].first && p <= stops[k + 1].first) {
                    double span = stops[k + 1].first - stops[k].first;
                    double t = span > 0 ? (p - stops[k].first) / span : 0;
                    c = lerp(stops[k].second, stops[k + 1].second, (float)t);
                    break;
                }
            }
        }
        c = clampRGB(c);
        int o = (bank * 64 + i) * 3;
        pal[o + 0] = (uint8_t)(c.r * 255 + 0.5);
        pal[o + 1] = (uint8_t)(c.g * 255 + 0.5);
        pal[o + 2] = (uint8_t)(c.b * 255 + 0.5);
    }
}
void setRamp(uint8_t* pal, int bank, std::initializer_list<std::pair<double, RGB>> stops) {
    setRampFromVector(pal, bank, std::vector<std::pair<double, RGB>>(stops));
}
void scalePalette(uint8_t* pal, double f) {
    for (int i = 0; i < BANKS * 64 * 3; i++) pal[i] = (uint8_t)std::min(255.0, pal[i] * f);
}
