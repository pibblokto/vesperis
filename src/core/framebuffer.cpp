#include "framebuffer.h"
#include "parallel.h"
#include <atomic>
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
    memset(underPal, 0, sizeof(underPal));
}
void Framebuffer::clear(Pix v) { std::fill(idx.begin(), idx.end(), v); }
void Framebuffer::clearDepth() { std::fill(invz.begin(), invz.end(), 0.f); }

void Framebuffer::mush(int passes) {
    const int W = FBW, H = FBH;
    if ((int)idx.size() != W * H) return;
    const int k = g_mushKernel < 1 ? 1 : g_mushKernel;
    back.resize(idx.size());
    static std::vector<Pix> none;   // the row the general box's column sums take in past the bottom: no intensity
    none.assign(W, 0);
    // F-07: a pass reads idx and writes back, then the two swap (the copy of the frame it read from is gone); the rows on the pool
    for (int p = 0; p < passes; p++) {
        const Pix* s = idx.data();
        Pix* d = back.data();
        if (k == 2) {
            // the original kernel: 2x2 block starting one row below, bank from one row below (the last two rows as they were)
            parallelFor(H - 2, 64, [&](int yb, int ye) {
                for (int y = yb; y < ye; y++) {
                    const Pix* r0 = s + (y + 1) * W;
                    const Pix* r1 = s + (y + 2) * W;
                    Pix* o = d + y * W;
                    for (int x = 0; x < W - 1; x++) {
                        unsigned a = (r0[x] & INTEN_MASK) + (r0[x + 1] & INTEN_MASK) + (r1[x] & INTEN_MASK) + (r1[x + 1] & INTEN_MASK);
                        o[x] = (Pix)((r0[x] & BANK_MASK) | (a >> 2));
                    }
                    o[W - 1] = r0[W - 1];
                }
            });
            memcpy(d + (size_t)(H - 2) * W, s + (size_t)(H - 2) * W, 2 * (size_t)W * sizeof(Pix));
        } else {
            // general k x k box starting one row below (windows clamped at the right edge and the bottom), bank from one row below.
            // a band of rows slides the column sums of its k rows down a row at a time and a k-wide window along them (the
            // box's sum whatever the order of its terms), the row under the pixel giving the bank and leaving the sums in one read;
            // the quotient by a multiply and a shift, exact while sum x divisor < 2^32 (sum <= 2047 k^2, divisor <= k^2: every k to 38)
            const bool recip = k <= 38;
            parallelFor(H, 64, [&](int yb, int ye) {
                static thread_local std::vector<uint32_t> colv;
                colv.assign(W, 0);
                uint32_t* col = colv.data();
                for (int yy = yb + 1; yy <= std::min(H - 1, yb + k); yy++) {
                    const Pix* r = s + yy * W;
                    for (int x = 0; x < W; x++) col[x] += r[x] & INTEN_MASK;
                }
                for (int y = yb; y < ye; y++) {
                    Pix* o = d + y * W;
                    const int y0 = y + 1;   // rows y+1 .. y+k
                    if (y0 >= H) { memcpy(o, s + (H - 1) * W, W * sizeof(Pix)); continue; }
                    const int rows = std::min(H - 1, y + k) - y0 + 1;
                    const Pix* r0 = s + y0 * W;
                    const Pix* rIn = y0 + k <= H - 1 ? s + (y0 + k) * W : none.data();
                    const uint32_t div = (uint32_t)(rows * k);
                    const uint64_t m = ((uint64_t)1 << 32) / div + 1;
                    uint32_t win = 0;
                    for (int x = 0; x < k && x < W; x++) win += col[x];
                    int x = 0;
                    for (; x + k <= W; x++) {   // the whole windows
                        o[x] = (Pix)((r0[x] & BANK_MASK) | (recip ? (uint32_t)((win * m) >> 32) : win / div));
                        if (x + k < W) win += col[x + k];
                        win -= col[x];
                        col[x] = col[x] - (r0[x] & INTEN_MASK) + (rIn[x] & INTEN_MASK);
                    }
                    for (; x < W; x++) {        // the last k - 1, cut by the edge
                        o[x] = (Pix)((r0[x] & BANK_MASK) | (win / (uint32_t)(rows * (W - x))));
                        win -= col[x];
                        col[x] = col[x] - (r0[x] & INTEN_MASK) + (rIn[x] & INTEN_MASK);
                    }
                }
            });
        }
        idx.swap(back);
        // B-437: the layer's banks are read where the pass took each pixel's bank from, a row below (`underRow`: the last two rows kept
        // by the 2x2 kernel, the last one by the box)
        if (underBank >= 0) { underShift++; underKeep = k == 2 ? 2 : 1; }
    }
}

void Framebuffer::glowDisc(double cx, double cy, double radius, double coreRadius, int maxAdd, int bankIfEmpty, bool skipOccluded) {
    if (radius <= 0) return;
    int x0 = (int)std::floor(cx - radius), x1 = (int)std::ceil(cx + radius);
    int y0 = (int)std::floor(cy - radius), y1 = (int)std::ceil(cy + radius);
    if (x1 < 0 || y1 < 0 || x0 >= FBW || y0 >= FBH) return;
    x0 = std::max(x0, 0); y0 = std::max(y0, 0); x1 = std::min(x1, FBW - 1); y1 = std::min(y1, FBH - 1);
    if (maxAdd <= 0) return;   // F-05: every pixel's add would be 0 (a dim veil walked a disc of 0.9 M pixels at 4x for nothing)
    double r2 = radius * radius;
    // the rows on the pool (a pixel is written once, by its row; a small disc stays on the caller under the threshold). Every
    // caller is on the main thread outside a pool job (the pool is not reentrant)
    parallelFor(y1 - y0 + 1, 32, [&](int rb, int re) {
        for (int y = y0 + rb; y < y0 + re; y++) {
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
    });
}

void Framebuffer::toRGB(uint32_t* out, double gain, bool dither) const {
    static std::vector<uint32_t> lut, lutUnder;
    auto build = [&](const uint8_t* p, std::vector<uint32_t>& lt, uint32_t banks) {
        lt.resize(BANKS * 256);
        for (int b = 0; b < BANKS; b++) {
            if (!((banks >> b) & 1u)) continue;
            for (int j = 0; j < 256; j++) {
                int s0 = j >> 2, s1 = std::min(63, s0 + 1);
                double t = (j & 3) * 0.25;
                const uint8_t* c0 = p + (b * 64 + s0) * 3;
                const uint8_t* c1 = p + (b * 64 + s1) * 3;
                int r = (int)((c0[0] + (c1[0] - c0[0]) * t) * gain + 0.5);
                int g = (int)((c0[1] + (c1[1] - c0[1]) * t) * gain + 0.5);
                int bl = (int)((c0[2] + (c1[2] - c0[2]) * t) * gain + 0.5);
                r = std::min(255, r); g = std::min(255, g); bl = std::min(255, bl);
                lt[b * 256 + j] = 0xFF000000u | ((uint32_t)bl << 16) | ((uint32_t)g << 8) | (uint32_t)r;
            }
        }
    };
    build(pal, lut, ~0u);
    // B-437: a pixel of the layer's bank, within its box, is read through the layer's palette at its own bank (`under`, at `underRow`);
    // that palette's table is made again only when it, the gain or the banks the layer holds change (the space view's, frame to frame)
    const bool layered = underBank >= 0 && under.size() == idx.size() && underX1 > underX0 && underY1 > underY0;
    static uint8_t madePal[BANKS * 64 * 3];
    static double madeGain = -1;
    static uint32_t madeHeld = 0;
    if (layered && (gain != madeGain || (underHeld & ~madeHeld) || memcmp(madePal, underPal, sizeof(underPal)) != 0)) {
        build(underPal, lutUnder, underHeld);
        memcpy(madePal, underPal, sizeof(underPal)); madeGain = gain; madeHeld = underHeld;
    }
    const Pix* s = idx.data();
    const uint32_t* L = lut.data();
    const uint32_t* LU = lutUnder.data();
    const unsigned ub = (unsigned)underBank;
    auto inLayer = [&](int y) { return layered && underRowIn(y); };
    if (!dither) {
        parallelFor(FBH, 100, [&](int yb, int ye) {
            if (!layered) {
                for (size_t i = (size_t)yb * FBW, e = (size_t)ye * FBW; i < e; i++) out[i] = L[(s[i] >> BANK_SHIFT) * 256 + ((s[i] & INTEN_MASK) >> 3)];   // 2048 levels -> 256
                return;
            }
            for (int y = yb; y < ye; y++) {
                const Pix* r = s + (size_t)y * FBW;
                uint32_t* o = out + (size_t)y * FBW;
                if (!inLayer(y)) { for (int x = 0; x < FBW; x++) o[x] = L[(r[x] >> BANK_SHIFT) * 256 + ((r[x] & INTEN_MASK) >> 3)]; continue; }
                const uint8_t* u = under.data() + (size_t)underRow(y) * FBW;
                for (int x = 0; x < underX0; x++) o[x] = L[(r[x] >> BANK_SHIFT) * 256 + ((r[x] & INTEN_MASK) >> 3)];
                for (int x = underX0; x < underX1; x++) {
                    const unsigned b = r[x] >> BANK_SHIFT;
                    o[x] = (b == ub ? LU + u[x] * 256 : L + b * 256)[(r[x] & INTEN_MASK) >> 3];
                }
                for (int x = underX1; x < FBW; x++) o[x] = L[(r[x] >> BANK_SHIFT) * 256 + ((r[x] & INTEN_MASK) >> 3)];
            }
        });
        return;
    }
    // M6-01: an ordered 2x2 dither over the 64 palette stops: the intensity is nudged by -1.5..+1.5 sixteenths of a
    // stop per cell, so gradients break into the original's chequered steps instead of smooth ramps
    static const int BAYER[4] = {-12, 4, 12, -4};   // in intensity units (32 per stop)
    parallelFor(FBH, 100, [&](int yb, int ye) {
        for (int y = yb; y < ye; y++) {
            const bool lay = inLayer(y);
            const uint8_t* u = lay ? under.data() + (size_t)underRow(y) * FBW : nullptr;
            for (int x = 0; x < FBW; x++) {
                size_t i = (size_t)y * FBW + x;
                int inten = (int)(s[i] & INTEN_MASK) + BAYER[((y >> (FB_SCALE > 1 ? 1 : 0)) & 1) * 2 + ((x >> (FB_SCALE > 1 ? 1 : 0)) & 1)];
                inten = inten < 0 ? 0 : (inten > (int)INTEN_MASK ? (int)INTEN_MASK : inten);
                const unsigned b = s[i] >> BANK_SHIFT;
                out[i] = (lay && b == ub && x >= underX0 && x < underX1 ? LU + u[x] * 256 : L + b * 256)[((inten >> 5) << 2)];   // snap to whole stops: the dither replaces the interpolation
            }
        }
    });
}

uint32_t Framebuffer::sinkUnder(int bank, int x0, int y0, int x1, int y1) {
    x0 = std::max(0, x0); y0 = std::max(0, y0); x1 = std::min(FBW, x1); y1 = std::min(FBH, y1);
    if (x1 <= x0 || y1 <= y0) return 0;   // nothing sunk: no layer
    under.resize(idx.size());
    memcpy(underPal, pal, sizeof(pal));
    underBank = bank;
    underX0 = x0; underX1 = x1; underY0 = y0; underY1 = y1; underShift = 0; underKeep = 1;
    std::atomic<uint32_t> seen{0};
    parallelFor(y1 - y0, 64, [&](int a, int b) {
        uint32_t m = 0;
        for (int y = y0 + a; y < y0 + b; y++) {
            Pix* row = idx.data() + (size_t)y * FBW;
            uint8_t* ur = under.data() + (size_t)y * FBW;
            for (int x = x0; x < x1; x++) {
                const unsigned k = row[x] >> BANK_SHIFT;
                m |= 1u << k;
                if (k) { ur[x] = (uint8_t)k; row[x] = (Pix)((bank << BANK_SHIFT) | (row[x] & INTEN_MASK)); }
            }
        }
        seen.fetch_or(m);
    });
    underHeld = seen.load();
    return underHeld;
}

void Framebuffer::raiseUnder() {
    if (underBank < 0) return;
    const int top = std::max(0, underY0 - underShift);   // the rows that took their banks from the box's
    if (under.size() == idx.size() && underY1 > top) {
        const unsigned ub = (unsigned)underBank;
        parallelFor(underY1 - top, 64, [&](int a, int b) {
            for (int y = top + a; y < top + b; y++) {
                if (!underRowIn(y)) continue;
                Pix* row = idx.data() + (size_t)y * FBW;
                const uint8_t* u = under.data() + (size_t)underRow(y) * FBW;
                for (int x = underX0; x < underX1; x++)
                    if ((unsigned)(row[x] >> BANK_SHIFT) == ub) row[x] = (Pix)((u[x] << BANK_SHIFT) | (row[x] & INTEN_MASK));
            }
        });
    }
    underBank = -1;
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
