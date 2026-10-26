#include "raster.h"
#include <cmath>
#include <algorithm>

namespace {
thread_local int g_bandY0 = 0, g_bandY1 = 1 << 30;   // B-310: the rows this thread may write
struct SV { double sx, sy, iz, shade, u, v, w; };

// F-07: a pixel whose depth test has passed (izb: its 1/z with the bias). The opaque replace, nearly every pixel of the ground, the
// forms and the leaves, inline; the other blends and the quantized shade out of line
[[gnu::noinline]] void emitPixelRare(Framebuffer& fb, int o, double izb, double shade, const RasterParams& rp, double grainOff, int bank) {
    if (rp.blend == BLEND_DARKEN) {
        Pix& q = fb.idx[o];
        const int behind = (int)(intenOf(q) * rp.darken);
        const int si = rp.sheen ? toInten(std::min(std::max(shade + grainOff, rp.shadeMin), rp.shadeMax)) : -1;   // the glass's sheen
        q = si > behind ? pixI(bank, si) : pixI(bankOf(q), behind);
        if (rp.zwrite) fb.invz[o] = (float)izb;
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
    if (rp.zwrite) fb.invz[o] = (float)izb;
}
inline void emitPassed(Framebuffer& fb, int o, double izb, double shade, const RasterParams& rp, double grainOff, int bank) {
    if (rp.blend != BLEND_REPLACE || rp.quantize > 0) { emitPixelRare(fb, o, izb, shade, rp, grainOff, bank); return; }
    double s = shade + grainOff;
    if (s < rp.shadeMin) s = rp.shadeMin;
    if (s > rp.shadeMax) s = rp.shadeMax;
    fb.idx[o] = pixI(bank, toInten(s));
    if (rp.zwrite) fb.invz[o] = (float)izb;
}
inline void emitPixel(Framebuffer& fb, int x, int y, double iz, double shade, const RasterParams& rp, double grainOff, int bank) {
    int o = y * FBW + x;
    iz *= rp.zbias;
    if (rp.ztest && iz <= fb.invz[o]) return;
    emitPassed(fb, o, iz, shade, rp, grainOff, bank);
}

// B-311: a texel of a world-mapped tile that spans more than about two and a half pixels on screen is read bilinearly, so the
// ground close to the camera is soft mottling instead of a mosaic of texel squares (the mush cannot melt 30-pixel
// squares at 2x and above); farther away, where texels are sub-pixel, the nearest texel is enough
void rasterProjected(Framebuffer& fb, const SV& a0, const SV& b0, const SV& c0, const RasterParams& rp, const Proj& pj) {
    const SV* pa = &a0; const SV* pb = &b0; const SV* pc = &c0;   // sorted by reference
    if (pa->sy > pb->sy) std::swap(pa, pb);
    if (pb->sy > pc->sy) std::swap(pb, pc);
    if (pa->sy > pb->sy) std::swap(pa, pb);
    const SV& a = *pa; const SV& b = *pb; const SV& c = *pc;
    int yStart = std::max(std::max(0, g_bandY0), (int)std::ceil(a.sy - 0.5));
    int yEnd = std::min(std::min(FBH - 1, g_bandY1 - 1), (int)std::floor(c.sy - 0.5));
    if (yStart > yEnd) return;
    double dyac = c.sy - a.sy;
    if (dyac < 1e-9) return;
    const double lerpZ1 = rp.grain ? pj.f / (2.5 * rp.grainScale) : 0, lerpZ2 = rp.grain2 || rp.grain2b ? pj.f / (2.5 * rp.grain2Scale) : 0;
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
        if (rp.grain || rp.grain2 || twoMat || rp.pattern) {
            const double gs = rp.grainScale, g2s = rp.grain2Scale, gw = rp.grainWeight, es = rp.edgeScale;
            const double eAmp = rp.edgeAmp / 64.0;
            const bool ztest = rp.ztest;
            const float* zrow = &fb.invz[(size_t)y * FBW];
            const bool flatZ = diz == 0.0;           // a span at one depth (a leaf cluster, a pad, a blob shadow): its 1/z once
            const double zFlat = flatZ ? 1.0 / iz : 0.0;
            for (int x = xStart; x <= xEnd; x++) {
                // a pixel hidden by the depth buffer costs nothing more (the texture reads come after the test)
                if (ztest && (float)(iz * rp.zbias) <= zrow[x]) { iz += diz; sh += dsh; u += du; v += dv; w += dw; continue; }
                double z = flatZ ? zFlat : 1.0 / iz;
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
                if (rp.pattern) {   // the windows
                    const FacePattern& P = *rp.pattern;
                    if (P.stripe > 0 || P.dado > 0) {   // the wall's courses and its dado, faded to their mean where a course is under two pixels
                        const double coursePx = P.stripe > 0 ? pj.f * P.stripe * iz / FB_SCALE : 1e9;
                        if (P.dado > 0 && vv < P.dado) go += P.dadoShade;
                        else if (P.stripe > 0) {
                            const double fade = coursePx <= 2.0 ? 0.0 : (coursePx >= 4.0 ? 1.0 : (coursePx - 2.0) * 0.5);
                            const bool odd = ((int64_t)std::floor(vv / P.stripe)) & 1;
                            go += P.stripeShade * (fade * (odd ? 1.0 : 0.0) + (1 - fade) * 0.5);
                        }
                    }
                    if (vv >= P.fromV) {
                        // how many pixels a cell spans here: under two (in 1x pixels) the pattern is its mean, from four whole
                        const double cellPx = pj.f * std::min(P.bay, P.storey) * iz / FB_SCALE;
                        const double fade = cellPx <= 2.0 ? 0.0 : (cellPx >= 4.0 ? 1.0 : (cellPx - 2.0) * 0.5);
                        bool in = false; double dress = 0;
                        if (fade > 0) {
                            double fu = uu / P.bay, fv = vv / P.storey;
                            const double iu = std::floor(fu), iv = std::floor(fv);
                            fu -= iu; fv -= iv;
                            if ((P.frame > 0 || P.shutter > 0) && !(fu >= P.x0 && fu < P.x1 && fv >= P.y0 && fv < P.y1)) {   // the surround, the shutters
                                const double fvh = P.frame * P.bay / P.storey;   // the surround as many metres up as across
                                bool blank = false;
                                if (P.scatter > 0) { uint32_t h = (uint32_t)((int64_t)iu * 73856093) ^ (uint32_t)((int64_t)iv * 19349663); h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15; blank = (h & 1023) < (uint32_t)(P.scatter * 1024); }
                                if (!blank) {
                                    if (P.shutter > 0 && fv >= P.y0 && fv < P.y1 && ((fu >= P.x0 - P.shutter && fu < P.x0) || (fu >= P.x1 && fu < P.x1 + P.shutter))) dress = P.shutterShade;
                                    else if (P.frame > 0 && fu >= P.x0 - P.frame && fu < P.x1 + P.frame && fv >= P.y0 - fvh && fv < P.y1 + fvh) dress = P.frameShade;
                                }
                            }
                            if (fu >= P.x0 && fu < P.x1 && fv >= P.y0 && fv < P.y1) {
                                in = true;
                                double cx = (fu - P.x0) / (P.x1 - P.x0), cy = (fv - P.y0) / (P.y1 - P.y0);
                                if (P.group > 1) {   // the lights of a pair or a group, their mullions between
                                    const double gx = cx * P.group, fr = gx - std::floor(gx);
                                    if (fr < 0.1 || fr > 0.9) in = false;
                                    cx = (fr - 0.1) / 0.8;
                                }
                                cx = cx * 2 - 1;
                                if (P.shape == 1 && cy > 0.7) { double q = (cy - 0.7) / 0.3; in = in && cx * cx + q * q < 1.0; }   // an arched head
                                else if (P.shape == 3) { double q = cy * 2 - 1; in = in && cx * cx + q * q < 1.0; }                 // round
                                else if (P.shape == 5) in = in && std::fabs(cx) < 1.0 - cy;                                         // a triangle
                                else if (P.shape == 6) { double q = (cy - 0.72) / 0.28; in = in && (cy > 0.44 ? cx * cx + q * q < 1.0 : std::fabs(cx) < 0.42 + 0.3 * cy); }   // a keyhole
                                if (in && P.scatter > 0) {   // a cell left blank by its hash
                                    uint32_t h = (uint32_t)((int64_t)iu * 73856093) ^ (uint32_t)((int64_t)iv * 19349663);
                                    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
                                    if ((h & 1023) < (uint32_t)(P.scatter * 1024)) in = false;
                                }
                            }
                        }
                        bool lit = in && fade > 0.5 && P.litBank >= 0;
                        if (P.litShare > 0) {   // a share of the window cells lit, each cell by its own hash; far off a scatter of points, as many as the lit windows' share of the face
                            uint32_t h = (uint32_t)((int64_t)std::floor(uu / P.bay) * 83492791) ^ (uint32_t)((int64_t)std::floor(vv / P.storey) * 2971215073u);
                            h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
                            lit = (lit && (h & 1023) < (uint32_t)(P.litShare * 1024))
                               || (fade <= 0.5 && (h & 1023) < (uint32_t)(P.litShare * std::min(1.0, 1.4 * P.meanShare) * 1024));
                        }
                        if (lit) { bank = P.litBank; s2 = P.litShade; go = 0; }
                        else go += -P.dark * (fade * (in ? 1.0 : 0.0) + (1 - fade) * P.meanShare) + fade * dress + (1 - fade) * P.meanDress;
                    }
                }
                // the depth test above passed (a float under the stored one: the double is too), so the pixel is written as it is
                emitPassed(fb, y * FBW + x, iz * rp.zbias, s2, rp, go, bank);
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

// F-07: no row of the shape in this thread's band: a fan triangle's rows lie within the shape's, and rasterProjected draws none outside
// [ceil(min - 0.5), floor(max - 0.5)] (an integer edge n: ceil(y) > n exactly when y > n, floor(y) < n when y < n)
inline bool outsideBand(double miny, double maxy) { return maxy - 0.5 < g_bandY0 || miny - 0.5 > g_bandY1 - 1; }

void rasterPolygon(Framebuffer& fb, const RVert* v, int n, const RasterParams& rp, const Proj& pj) {
    // Sutherland-Hodgman against z = NEAR_Z (B-315: up to 16 vertices; a leaf cluster is a fan of 12)
    RVert out[16];
    const RVert* src = out;
    int m = 0;
    bool allIn = true;
    for (int i = 0; i < n && allIn; i++) allIn = v[i].z >= NEAR_Z;
    if (allIn) { src = v; m = std::min(n, 15); }   // every vertex in front: read in place, the first 15 as the clip keeps them
    else
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
        p[i] = project(src[i], pj);
        minx = std::min(minx, p[i].sx); maxx = std::max(maxx, p[i].sx);
        miny = std::min(miny, p[i].sy); maxy = std::max(maxy, p[i].sy);
    }
    if (maxx < 0 || minx > FBW || maxy < 0 || miny > FBH) return;
    if (outsideBand(miny, maxy)) return;
    for (int i = 1; i + 1 < m; i++) rasterProjected(fb, p[0], p[i], p[i + 1], rp, pj);
}

void rasterTriangle(Framebuffer& fb, const RVert* v, const RasterParams& rp, const Proj& pj) {
    if (v[0].z >= NEAR_Z && v[1].z >= NEAR_Z && v[2].z >= NEAR_Z) {
        SV p0 = project(v[0], pj), p1 = project(v[1], pj), p2 = project(v[2], pj);
        double minx = std::min({p0.sx, p1.sx, p2.sx}), maxx = std::max({p0.sx, p1.sx, p2.sx});
        double miny = std::min({p0.sy, p1.sy, p2.sy}), maxy = std::max({p0.sy, p1.sy, p2.sy});
        if (maxx < 0 || minx > FBW || maxy < 0 || miny > FBH) return;
        if (outsideBand(miny, maxy)) return;
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
    // B-310: walk only the steps whose rows fall into this thread's band (and the frame): walking every trunk and branch of a
    // forest in full and rejecting per pixel would be most of a forest frame
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
