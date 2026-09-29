#include "raster.h"
#include <cmath>
#include <algorithm>

namespace {
thread_local int g_bandY0 = 0, g_bandY1 = 1 << 30;   // B-310: the rows this thread may write
struct SV { double sx, sy, iz, shade, u, v, w; };

inline void emitPixel(Framebuffer& fb, int x, int y, double iz, double shade, const RasterParams& rp, double grainOff, int bank) {
    int o = y * FBW + x;
    iz *= rp.zbias;
    if (rp.ztest && iz <= fb.invz[o]) return;
    if (rp.blend == BLEND_DARKEN) {
        Pix& q = fb.idx[o];
        q = pixI(bankOf(q), (int)(intenOf(q) * rp.darken));
        if (rp.zwrite) fb.invz[o] = (float)iz;
        return;
    }
    if (rp.blend == BLEND_REFLECT) {
        Pix& q = fb.idx[o];
        if (bankOf(q) != rp.maskBank) return;
        double s = shade + grainOff;
        q = pixI(rp.maskBank, (int)(intenOf(q) * (1 - rp.reflectMix) + toInten(s) * rp.reflectMix));
        return;
    }
    double s = shade;
    if (rp.quantize > 0) s = std::round(s / rp.quantize) * rp.quantize;
    s += grainOff;
    if (s < rp.shadeMin) s = rp.shadeMin;
    if (s > rp.shadeMax) s = rp.shadeMax;
    int si = toInten(s);
    Pix& p = fb.idx[o];
    if (rp.blend == BLEND_REPLACE) p = pixI(bank, si);
    else if (rp.blend == BLEND_ADD) p = pixI(bankOf(p), intenOf(p) + si);
    else p = pixI(bank, std::max(intenOf(p), si));
    if (rp.zwrite) fb.invz[o] = (float)iz;
}

// B-311: a texel of a world-mapped tile that spans more than about two and a half pixels on screen is read bilinearly, so the
// ground close to the camera is soft mottling instead of a mosaic of texel squares (the mush no longer melts 30-pixel
// squares at 2x and above); farther away, where texels are sub-pixel, the nearest texel is enough
void rasterProjected(Framebuffer& fb, SV a, SV b, SV c, const RasterParams& rp, const Proj& pj) {
    const double lerpZ1 = rp.grain ? pj.f / (2.5 * rp.grainScale) : 0, lerpZ2 = rp.grain2 || rp.grain2b ? pj.f / (2.5 * rp.grain2Scale) : 0;
    if (a.sy > b.sy) std::swap(a, b);
    if (b.sy > c.sy) std::swap(b, c);
    if (a.sy > b.sy) std::swap(a, b);
    int yStart = std::max(std::max(0, g_bandY0), (int)std::ceil(a.sy - 0.5));
    int yEnd = std::min(std::min(FBH - 1, g_bandY1 - 1), (int)std::floor(c.sy - 0.5));
    if (yStart > yEnd) return;
    double dyac = c.sy - a.sy;
    if (dyac < 1e-9) return;
    for (int y = yStart; y <= yEnd; y++) {
        double py = y + 0.5;
        // long edge a-c
        double t = (py - a.sy) / dyac;
        SV L{a.sx + (c.sx - a.sx) * t, py, a.iz + (c.iz - a.iz) * t, a.shade + (c.shade - a.shade) * t,
             a.u + (c.u - a.u) * t, a.v + (c.v - a.v) * t, a.w + (c.w - a.w) * t};
        SV R;
        if (py < b.sy) {
            double d = b.sy - a.sy; double t2 = d > 1e-9 ? (py - a.sy) / d : 0;
            R = SV{a.sx + (b.sx - a.sx) * t2, py, a.iz + (b.iz - a.iz) * t2, a.shade + (b.shade - a.shade) * t2,
                   a.u + (b.u - a.u) * t2, a.v + (b.v - a.v) * t2, a.w + (b.w - a.w) * t2};
        } else {
            double d = c.sy - b.sy; double t2 = d > 1e-9 ? (py - b.sy) / d : 0;
            R = SV{b.sx + (c.sx - b.sx) * t2, py, b.iz + (c.iz - b.iz) * t2, b.shade + (c.shade - b.shade) * t2,
                   b.u + (c.u - b.u) * t2, b.v + (c.v - b.v) * t2, b.w + (c.w - b.w) * t2};
        }
        if (L.sx > R.sx) std::swap(L, R);
        int xStart = std::max(0, (int)std::ceil(L.sx - 0.5));
        int xEnd = std::min(FBW - 1, (int)std::floor(R.sx - 0.5));
        if (xStart > xEnd) continue;
        double span = R.sx - L.sx;
        double inv = span > 1e-9 ? 1.0 / span : 0;
        double diz = (R.iz - L.iz) * inv, dsh = (R.shade - L.shade) * inv;
        double du = (R.u - L.u) * inv, dv = (R.v - L.v) * inv, dw = (R.w - L.w) * inv;
        double t0 = (xStart + 0.5 - L.sx);
        double iz = L.iz + diz * t0, sh = L.shade + dsh * t0, u = L.u + du * t0, v = L.v + dv * t0, w = L.w + dw * t0;
        const bool twoMat = rp.bank2 >= 0 || rp.cutout;
        if (rp.grain || rp.grain2 || twoMat) {
            const double gs = rp.grainScale, g2s = rp.grain2Scale, gw = rp.grainWeight, es = rp.edgeScale;
            const double eAmp = rp.edgeAmp / 64.0;
            const bool ztest = rp.ztest;
            const float* zrow = &fb.invz[(size_t)y * FBW];
            for (int x = xStart; x <= xEnd; x++) {
                // B-315: a pixel hidden by the depth buffer costs nothing more (the texture reads come after the test)
                if (ztest && (float)(iz * rp.zbias) <= zrow[x]) { iz += diz; sh += dsh; u += du; v += dv; w += dw; continue; }
                double z = 1.0 / iz;
                double uu = u * z, vv = v * z;
                int bank = rp.bank;
                const GrainTexture* g2 = rp.grain2;
                double s2 = sh;
                if (twoMat) {
                    double wp = w * z;
                    if (rp.edge) wp += eAmp * rp.edge->at((int)std::floor(uu * es), (int)std::floor(vv * es));
                    if (wp < 0.5) {
                        if (rp.cutout) { iz += diz; sh += dsh; u += du; v += dv; w += dw; continue; }
                        bank = rp.bank2; g2 = rp.grain2b; s2 = sh + rp.shade2;
                    }
                }
                double go = 0;
                if (rp.grain) go += z < lerpZ1 ? rp.grain->atLerp(uu * gs - 0.5, vv * gs - 0.5) : (double)rp.grain->at((int)std::floor(uu * gs), (int)std::floor(vv * gs));
                if (g2) go += (z < lerpZ2 || g2s < 1.0) ? g2->atLerp(uu * g2s - 0.5, vv * g2s - 0.5) : (double)g2->at((int)std::floor(uu * g2s), (int)std::floor(vv * g2s));
                go *= gw;
                emitPixel(fb, x, y, iz, s2, rp, go, bank);
                iz += diz; sh += dsh; u += du; v += dv; w += dw;
            }
        } else {
            for (int x = xStart; x <= xEnd; x++) {
                emitPixel(fb, x, y, iz, sh, rp, 0, rp.bank);
                iz += diz; sh += dsh;
            }
        }
    }
}

// u and v are stored pre-divided by z so that screen-space linear interpolation is perspective correct.
inline SV project(const RVert& v, const Proj& pj) {
    double iz = 1.0 / v.z;
    return SV{pj.cx + pj.f * v.x * iz, pj.cy - pj.f * v.y * iz, iz, v.shade, v.u * iz, v.v * iz, v.w * iz};
}

inline RVert clipLerp(const RVert& a, const RVert& b, double t) {
    RVert r;
    r.x = a.x + (b.x - a.x) * t; r.y = a.y + (b.y - a.y) * t; r.z = a.z + (b.z - a.z) * t;
    r.shade = a.shade + (b.shade - a.shade) * t; r.u = a.u + (b.u - a.u) * t; r.v = a.v + (b.v - a.v) * t; r.w = a.w + (b.w - a.w) * t;
    return r;
}
}

void setRasterBand(int y0, int y1) { g_bandY0 = y0; g_bandY1 = y1; }

void rasterPolygon(Framebuffer& fb, const RVert* v, int n, const RasterParams& rp, const Proj& pj) {
    // Sutherland-Hodgman against z = NEAR_Z (B-315: up to 16 vertices; a leaf cluster is a fan of 12)
    RVert out[16];
    int m = 0;
    for (int i = 0; i < n; i++) {
        const RVert& a = v[i];
        const RVert& b = v[(i + 1) % n];
        bool ain = a.z >= NEAR_Z, bin = b.z >= NEAR_Z;
        if (ain) out[m++] = a;
        if (ain != bin) {
            double t = (NEAR_Z - a.z) / (b.z - a.z);
            out[m++] = clipLerp(a, b, t);
        }
        if (m >= 15) break;
    }
    if (m < 3) return;
    SV p[16];
    double minx = 1e9, maxx = -1e9, miny = 1e9, maxy = -1e9;
    for (int i = 0; i < m; i++) {
        p[i] = project(out[i], pj);
        minx = std::min(minx, p[i].sx); maxx = std::max(maxx, p[i].sx);
        miny = std::min(miny, p[i].sy); maxy = std::max(maxy, p[i].sy);
    }
    if (maxx < 0 || minx > FBW || maxy < 0 || miny > FBH) return;
    for (int i = 1; i + 1 < m; i++) rasterProjected(fb, p[0], p[i], p[i + 1], rp, pj);
}

void rasterTriangle(Framebuffer& fb, const RVert* v, const RasterParams& rp, const Proj& pj) {
    if (v[0].z >= NEAR_Z && v[1].z >= NEAR_Z && v[2].z >= NEAR_Z) {
        SV p0 = project(v[0], pj), p1 = project(v[1], pj), p2 = project(v[2], pj);
        double minx = std::min({p0.sx, p1.sx, p2.sx}), maxx = std::max({p0.sx, p1.sx, p2.sx});
        double miny = std::min({p0.sy, p1.sy, p2.sy}), maxy = std::max({p0.sy, p1.sy, p2.sy});
        if (maxx < 0 || minx > FBW || maxy < 0 || miny > FBH) return;
        rasterProjected(fb, p0, p1, p2, rp, pj);
    } else {
        rasterPolygon(fb, v, 3, rp, pj);
    }
}

bool projectPoint(const RVert& a, const Proj& pj, double& sx, double& sy) {
    if (a.z < NEAR_Z) return false;
    double iz = 1.0 / a.z;
    sx = pj.cx + pj.f * a.x * iz;
    sy = pj.cy - pj.f * a.y * iz;
    return true;
}

bool rasterPoint3(Framebuffer& fb, const RVert& a, int bank, const Proj& pj, bool ztest, int size) {
    size *= FB_SCALE;
    double sx, sy;
    if (!projectPoint(a, pj, sx, sy)) return false;
    int x = (int)std::floor(sx), y = (int)std::floor(sy);
    float iz = (float)(1.0 / a.z);
    bool drawn = false;
    int half = size / 2;
    for (int dy = -half; dy <= half; dy++)
        for (int dx = -half; dx <= half; dx++) {
            int px = x + dx, py = y + dy;
            if (px < 0 || py < 0 || px >= FBW || py >= FBH || py < g_bandY0 || py >= g_bandY1) continue;
            int o = py * FBW + px;
            if (ztest && iz <= fb.invz[o]) continue;
            fb.idx[o] = pix(bank, a.shade);
            fb.invz[o] = iz;
            drawn = true;
        }
    return drawn;
}

void rasterLine3(Framebuffer& fb, RVert a, RVert b, int bank, const Proj& pj, bool ztest, int thickness) {
    thickness *= FB_SCALE;
    if (a.z < NEAR_Z && b.z < NEAR_Z) return;
    if (a.z < NEAR_Z) a = clipLerp(a, b, (NEAR_Z - a.z) / (b.z - a.z));
    else if (b.z < NEAR_Z) b = clipLerp(b, a, (NEAR_Z - b.z) / (a.z - b.z));
    SV p = project(a, pj), q = project(b, pj);
    double dx = q.sx - p.sx, dy = q.sy - p.sy;
    int steps = (int)std::ceil(std::max(std::fabs(dx), std::fabs(dy)));
    if (steps > 4000) return;
    if (steps < 1) steps = 1;
    // B-310: walk only the steps whose rows fall into this thread's band (and the frame); a band thread used to walk
    // every trunk and branch of a forest in full and reject per pixel, which was most of a forest frame
    int y0 = std::max(0, g_bandY0), y1 = std::min(FBH, g_bandY1);
    if (y0 >= y1) return;
    int i0 = 0, i1 = steps;
    {
        double ylo = std::min(p.sy, q.sy), yhi = std::max(p.sy, q.sy) + thickness;
        if (yhi < y0 || ylo >= y1) return;
        if (std::fabs(dy) > 1e-9) {
            double ta = (y0 - thickness - p.sy) / dy, tb = (y1 - p.sy) / dy;
            double tmin = std::min(ta, tb), tmax = std::max(ta, tb);
            i0 = std::max(0, (int)std::floor(tmin * steps) - 1);
            i1 = std::min(steps, (int)std::ceil(tmax * steps) + 1);
        }
    }
    for (int i = i0; i <= i1; i++) {
        double t = (double)i / steps;
        int x = (int)std::floor(p.sx + dx * t), y = (int)std::floor(p.sy + dy * t);
        if (y + thickness <= y0 || y >= y1 || x + thickness <= 0 || x >= FBW) continue;
        double iz = p.iz + (q.iz - p.iz) * t;
        double sh = p.shade + (q.shade - p.shade) * t;
        Pix pv = pix(bank, sh);
        float fiz = (float)iz;
        int ya = std::max(y, y0), yb = std::min(y + thickness, y1), xa = std::max(x, 0), xb = std::min(x + thickness, FBW);
        for (int py = ya; py < yb; py++) {
            Pix* row = &fb.idx[(size_t)py * FBW]; float* zrow = &fb.invz[(size_t)py * FBW];
            for (int px = xa; px < xb; px++) {
                if (ztest && fiz <= zrow[px]) continue;
                row[px] = pv; zrow[px] = fiz;
            }
        }
    }
}

void fillRect(Framebuffer& fb, int x0, int y0, int x1, int y1, Pix value) {
    x0 = std::max(0, x0); y0 = std::max(0, y0); x1 = std::min(FBW - 1, x1); y1 = std::min(FBH - 1, y1);
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) fb.idx[y * FBW + x] = value;
}

void fillDisc(Framebuffer& fb, double cx, double cy, double r, Pix value) {
    int x0 = std::max(0, (int)std::floor(cx - r)), x1 = std::min(FBW - 1, (int)std::ceil(cx + r));
    int y0 = std::max(0, (int)std::floor(cy - r)), y1 = std::min(FBH - 1, (int)std::ceil(cy + r));
    double r2 = r * r;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            double dx = x + 0.5 - cx, dy = y + 0.5 - cy;
            if (dx * dx + dy * dy <= r2) fb.idx[y * FBW + x] = value;
        }
}

void drawLine2D(Framebuffer& fb, int x0, int y0, int x1, int y1, Pix value) {
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int steps = std::max(dx, dy);
    if (steps == 0) { if (x0 >= 0 && y0 >= 0 && x0 < FBW && y0 < FBH) fb.idx[y0 * FBW + x0] = value; return; }
    for (int i = 0; i <= steps; i++) {
        int x = x0 + (x1 - x0) * i / steps, y = y0 + (y1 - y0) * i / steps;
        if (x >= 0 && y >= 0 && x < FBW && y < FBH) fb.idx[y * FBW + x] = value;
    }
}
