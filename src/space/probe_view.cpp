// X-02: the probe's camera inside a giant (probe_view.h).
#include "probe_view.h"
#include "core/noise.h"
#include "core/rng.h"
#include "core/parallel.h"
#include "galaxy/planetmap.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>
#include <atomic>

void ShadeRamp::set(const std::vector<std::pair<double, RGB>>& stops, float gain) {
    for (int i = 0; i < 64; i++) {
        RGB col = stops.front().second;
        if (i >= stops.back().first) col = stops.back().second;
        else
            for (size_t k = 0; k + 1 < stops.size(); k++)
                if (i >= stops[k].first && i <= stops[k + 1].first) {
                    double span = stops[k + 1].first - stops[k].first;
                    col = lerp(stops[k].second, stops[k + 1].second, (float)(span > 0 ? (i - stops[k].first) / span : 0.0));
                    break;
                }
        c[i][0] = col.r * gain; c[i][1] = col.g * gain; c[i][2] = col.b * gain;
    }
}

void ShadeRamp::material(const RGB& col, float gain) {
    MatRamp m = materialRamp(col, nullptr, 1.0);
    for (int i = 0; i < 64; i++) { RGB q = materialRampColor(m, i); c[i][0] = q.r * gain; c[i][1] = q.g * gain; c[i][2] = q.b * gain; }
}

namespace {
constexpr int LW = 320, LH = 200;   // the logical grid: a node a UI pixel
constexpr int FC = 4;               // the fog's grid: a node every fourth
constexpr int NF = 18;              // the fog's steps along a ray
constexpr double STEP = 0.022;      // a panorama's step: this share of the distance

struct Frame { double aC = 0, aLo = 0, aHi = 0, eLo = 0, eHi = 0, da = 0, de = 0; };
struct Pano {
    int deck = -1; bool under = false;
    int na = 0, ne = 0;
    double a0 = 0, da = 0, e0 = 0, de = 0;   // the first column's azimuth and the first row's elevation, on fixed grids (a turn of the camera does not resample the clouds)
    std::vector<float> sh, ds, gl;   // the shade (lit, before the haze), the distance (km; negative: nothing there), the glow from below (X-03)
    std::vector<uint8_t> dk;         // X-03: the deck a cell shows (a deck under a hole in the panorama's own)
};
struct DeckCache { int n = 0; double half = 0, step = 0; std::vector<float> top, base; };
struct FogNode { float sun, lamp, flash, heat, glow, t; };
struct OutNode { float r, g, b, t; };
ProbeViewStats g_stats;

// the clear air's optical depth from the top down to an altitude (the pressure sampled by its logarithm, resampled by the
// altitude); kept while the atmosphere stays the same
struct HazeTable { double z0 = 0, dz = 1; int n = 0; double key[7] = {0, 0, 0, 0, 0, 0, 0}; std::vector<float> K; };
const HazeTable& hazeOf(const GiantAtmosphere& a) {
    static HazeTable H;
    const double key[7] = {a.t1K, a.gravity, a.hazeBar, a.top[0], a.base[a.decks - 1], (double)a.decks, a.top[1] + a.base[0]};
    if (H.n && std::equal(key, key + 7, H.key)) return H;
    std::copy(key, key + 7, H.key);
    const int NS = 4000;
    std::vector<double> zs(NS + 1), ts(NS + 1);
    for (int i = 0; i <= NS; i++) { double P = std::exp(std::log(1e-7) + (std::log(80.0) - std::log(1e-7)) * i / NS); zs[i] = giantAltitudeKm(a, P); ts[i] = giantHazeTau(a, P); }
    const double zHi = zs[0], zLo = zs[NS];   // the altitude falls as the pressure grows
    H.dz = std::max(0.25, (zHi - zLo) / 4096); H.z0 = zLo; H.n = (int)((zHi - zLo) / H.dz) + 2;
    H.K.assign(H.n, 0.f);
    int k = NS - 1;
    for (int i = 0; i < H.n; i++) {
        double z = H.z0 + i * H.dz;
        while (k > 0 && zs[k] < z) k--;
        double t = zs[k] > zs[k + 1] ? clampd((z - zs[k + 1]) / (zs[k] - zs[k + 1]), 0, 1) : 0;
        H.K[i] = (float)(ts[k + 1] + (ts[k] - ts[k + 1]) * t);
    }
    return H;
}
inline double hazeAt(const HazeTable& H, double z) {
    double f = clampd((z - H.z0) / H.dz, 0, H.n - 1.0001);
    int i = (int)f;
    return H.K[i] + (H.K[i + 1] - H.K[i]) * (f - i);
}
// the haze's optical depth along a path between two altitudes: the mean extinction over the altitudes it spans times its length
inline double pathTau(const HazeTable& H, double z1, double z2, double dist) {
    double dz = z2 - z1;
    if (std::fabs(dz) < 0.05) { z2 = z1 + 0.05; dz = 0.05; }
    return std::fabs(hazeAt(H, z2) - hazeAt(H, z1)) / std::fabs(dz) * dist;
}
// a shade's highlights rolled off (a camera's shoulder: a bright zone beside the belt the exposure was set for keeps its shape)
inline double knee(double s) { return s < 46 ? s : 46 + 17 * (1 - std::exp(-(s - 46) / 17)); }
double msSince(std::chrono::steady_clock::time_point t0) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }

// work taken a piece at a time by whichever thread is free (the panoramas' columns differ in cost, and the efficiency cores
// would hold up an even split)
template <class F> void dealt(int n, const F& fn) {
    std::atomic<int> next(0);
    parallelFor(parallelThreads(), 1, [&](int, int) { for (int i; (i = next.fetch_add(1)) < n;) fn(i); });
}

// the frame's rays by azimuth (round the camera's heading) and elevation; every azimuth when the zenith or the nadir is in it
Frame frameOf(const ProbeView& v) {
    Frame F;
    const Mat3 camT = v.cam.transposed();
    const double S = FB_SCALE, fl = v.pj.f / S, cx = v.pj.cx / S, cy = v.pj.cy / S;
    Vec3 fwd = camT * Vec3(0, 0, 1), upv = camT * Vec3(0, 1, 0);
    Vec3 h = std::hypot(fwd.x, fwd.z) > 1e-3 ? fwd : upv;
    F.aC = std::atan2(h.x, h.z);
    F.aLo = F.eLo = 1e9; F.aHi = F.eHi = -1e9;
    for (int k = 0; k <= 32; k++) {
        double t = k / 32.0;
        const double pts[4][2] = {{t * LW, 0}, {t * LW, (double)LH}, {0, t * LH}, {(double)LW, t * LH}};
        for (const auto& p : pts) {
            Vec3 d = normalize(camT * Vec3((p[0] - cx) / fl, -(p[1] - cy) / fl, 1));
            double a = wrapAngle(std::atan2(d.x, d.z) - F.aC), e = std::asin(clampd(d.y, -1, 1));
            F.aLo = std::min(F.aLo, a); F.aHi = std::max(F.aHi, a); F.eLo = std::min(F.eLo, e); F.eHi = std::max(F.eHi, e);
        }
    }
    F.da = 2 * std::atan(LW * 0.5 / fl) / LW;
    F.de = 1 / fl;
    for (int s = -1; s <= 1; s += 2) {
        Vec3 pv = v.cam * Vec3(0, s, 0);
        if (pv.z <= 1e-6) continue;
        double sx = cx + fl * pv.x / pv.z, sy = cy - fl * pv.y / pv.z;
        if (sx >= -2 && sx <= LW + 2 && sy >= -2 && sy <= LH + 2) { F.aLo = -PI; F.aHi = PI; if (s > 0) F.eHi = PI / 2; else F.eLo = -PI / 2; }
    }
    F.aLo -= 2 * F.da; F.aHi += 2 * F.da; F.eLo -= 2 * F.de; F.eHi += 2 * F.de;
    if (F.aHi - F.aLo > TAU) { F.aLo = -PI; F.aHi = PI + 2 * F.da; }
    return F;
}

void panoInit(Pano& P, const Frame& F, int deck, bool under) {
    P.deck = deck; P.under = under;
    P.da = F.da; P.de = F.de;
    P.a0 = std::floor((F.aC + F.aLo) / F.da) * F.da; P.e0 = std::floor(F.eLo / F.de) * F.de;
    P.na = std::min(2400, (int)std::ceil((F.aHi - F.aLo) / F.da) + 3);
    P.ne = std::min(1400, (int)std::ceil((F.eHi - F.eLo) / F.de) + 3);
    P.sh.assign((size_t)P.na * P.ne, 0.f);
    P.ds.assign((size_t)P.na * P.ne, -1.f);
    P.gl.assign((size_t)P.na * P.ne, 0.f);
    P.dk.assign((size_t)P.na * P.ne, (uint8_t)deck);
}

// a panorama's cells round a direction: the share of them the surface fills, and the filled ones' shade, distance and glow, the
// deck of the nearest filled one
inline void panoAt(const Pano& P, double a, double e, double& cov, double& sh, double& ds, double& gl, int& dk) {
    cov = 0; sh = 0; ds = -1; gl = 0; dk = P.deck;
    if (P.deck < 0) return;
    double fi = wrap2pi(a - P.a0) / P.da, fj = (e - P.e0) / P.de;
    int i = (int)std::floor(fi), j = (int)std::floor(fj);
    double u = fi - i, w = fj - j;
    if (i < 0) { i = 0; u = 0; } else if (i > P.na - 2) { i = P.na - 2; u = 1; }
    if (j < 0) { j = 0; w = 0; } else if (j > P.ne - 2) { j = P.ne - 2; w = 1; }
    const size_t o = (size_t)i * P.ne + j;
    const size_t cells[4] = {o, o + P.ne, o + 1, o + P.ne + 1};
    const double wt[4] = {(1 - u) * (1 - w), u * (1 - w), (1 - u) * w, u * w};
    double sw = 0, ss = 0, sd = 0, sg = 0, best = -1;
    for (int k = 0; k < 4; k++) {
        float d = P.ds[cells[k]];
        if (d < 0) continue;
        sw += wt[k]; ss += wt[k] * P.sh[cells[k]]; sd += wt[k] * d; sg += wt[k] * P.gl[cells[k]];
        if (wt[k] > best) { best = wt[k]; dk = P.dk[cells[k]]; }
    }
    if (sw <= 1e-6) return;
    cov = sw; sh = ss / sw; ds = sd / sw; gl = sg / sw;
}

inline void cacheAt(const DeckCache& C, double x, double z, double& top, double& base) {
    double fx = clampd((x + C.half) / C.step, 0, C.n - 1.0001), fz = clampd((z + C.half) / C.step, 0, C.n - 1.0001);
    int i = (int)fx, j = (int)fz; double u = fx - i, w = fz - j;
    size_t o = (size_t)j * C.n + i;
    top = (C.top[o] * (1 - u) + C.top[o + 1] * u) * (1 - w) + (C.top[o + C.n] * (1 - u) + C.top[o + C.n + 1] * u) * w;
    base = (C.base[o] * (1 - u) + C.base[o + 1] * u) * (1 - w) + (C.base[o + C.n] * (1 - u) + C.base[o + C.n + 1] * u) * w;
}

// the rings' shadow on a point (local, km from the probe): the sun's way to the ring plane, the ring's density there
double ringShadowAt(const ProbeView& v, const Vec3& q) {
    double dn = dot(v.sun, v.ringN);
    if (std::fabs(dn) < 1e-6) return 0;
    double s = dot(v.ringC - q, v.ringN) / dn;
    if (s <= 0) return 0;
    double rr = length(q + v.sun * s - v.ringC);
    if (rr < v.ringR0 || rr > v.ringR1) return 0;
    return 0.75 * (*v.ringProf)[clampi((int)((rr - v.ringR0) / (v.ringR1 - v.ringR0) * 255), 0, 255)];
}

// the sun over a place of the decks (km from the probe, at a height): the giant's curve raises it toward the sun and lowers
// it away; a crest sees past the dip. 0 night .. 1 day
inline double dayAt(const ProbeView& v, double xr, double zr, double hOver) {
    double hl = std::hypot(v.sun.x, v.sun.z);
    double el = std::asin(clampd(v.sun.y, -1, 1)) + (hl > 1e-9 ? (xr * v.sun.x + zr * v.sun.z) / hl : 0.0) / v.radiusKm + std::sqrt(2 * std::max(0.0, hOver) / v.radiusKm);
    return smoothstep(-0.012, 0.012, el);
}

// a top's shade: the band's albedo (the globe's: zones bright, belts dark) under the sun (wrapped round the billows, the
// towers' and the rings' shadows) and the sky's light (darker in the folds), as the globe exposes it. X-03: `kl` the deck whose
// light falls on it (the deck over a hole's floor), times `lightF`; `glow` the deep's light come up through it, brighter in
// the troughs than through a tower's thickness
double shadeTop(const ProbeView& v, int k, int kl, double lightF, double x, double z, double xr, double zr, double h, double low, double sr, double lod, double& glow, const double* ssGiven = nullptr) {
    const GiantDecks& D = *v.decks;
    const double hl = std::hypot(v.sun.x, v.sun.z);
    glow = v.glowTop[k] > 1e-4 ? v.glowTop[k] * clampd(std::exp(-0.4 * (h - low) / D.sh), 0.15, 2.0) : 0.0;
    double day = dayAt(v, xr, zr, h - D.lvTop[k]);
    double direct = 0, nn = 1;
    if (v.direct[kl] > 0.004 && day > 0 && hl > 1e-6) {
        const double sx = v.sun.x / hl, sz = v.sun.z / hl, eps = std::max(lod, 0.05 * D.sh);
        // the rise toward the sun and along the march, held to a soft slope: a cloud's face scatters the light it gets, so a
        // steep little head reads as a softer shade rather than a hole
        double ss = clampd(ssGiven ? *ssGiven : (D.top(k, x + sx * eps, z + sz * eps, lod) - h) / eps, -1.2, 1.2);
        sr = clampd(sr, -1.2, 1.2);
        nn = 1 / std::sqrt(1 + 0.6 * (ss * ss + sr * sr));
        double dif = std::max(0.0, ((v.sun.y - hl * ss) * nn + 0.5) / 1.5);   // wrapped: a cloud carries light round into its shaded side
        if (dif > 0) {
            double lit = 1 - D.towerShadow(k, x, h, z, v.sun, 160 * D.sh);
            if (v.rings && lit > 0) lit *= 1 - ringShadowAt(v, Vec3(xr, h - v.zKm, zr));
            direct = v.direct[kl] * lightF * dif * lit;
        }
    } else { sr = clampd(sr, -1.2, 1.2); nn = 1 / std::sqrt(1 + 0.6 * sr * sr); }
    double ao = 0.5 + 0.5 * smoothstep(-1.2 * D.sh, 1.0 * D.sh, h - low);
    double amb = v.diffuse[kl] * lightF * ao * (0.55 + 0.45 * nn);
    double alb = 0.85, st = 0;
    if (k == 0 && D.bands) D.bands->at(x, z, alb, st);
    alb *= 0.78 + 0.32 * smoothstep(-2.5 * D.sh, 2.5 * D.sh, h - D.lvTop[k] - D.lift(k, x, z));   // the high tops fresh and bright, the troughs showing the deeper, darker cloud
    return knee(63.0 * std::pow(std::max(alb, 0.05), 0.7) * (amb + direct) * (0.05 + 0.95 * day) * 1.05);
}

// an underside's shade: the light come through the deck, the pouches' bottoms catching more of the clear band's light than
// the creases between them
double shadeUnder(const ProbeView& v, int k, double h, double swell, double xr, double zr, double& glow) {
    double pouch = clampd((swell - h) / (1.3 * v.decks->sh), 0, 1);
    glow = v.glowUnder[k] * (0.55 + 0.7 * pouch);   // X-03: the deep's glow on it, the pouches' bottoms nearest it
    return knee(63.0 * 1.05 * v.under[k] * (0.58 + 0.6 * pouch) * (0.05 + 0.95 * dayAt(v, xr, zr, 0)));
}

// X-03: a top with its holes (local km from the probe): the deck's own where it is whole, the next whole deck's top on a hole's
// floor, the wall between them over the hole's rim; `kdOut` the deck the place shows (the wall is this deck's)
double holedTop(const ProbeView& v, int k, double xr, double zr, double lod, double* low, int* kdOut) {
    const GiantDecks& D = *v.decks;
    const double x = xr + v.px[k], z = zr + v.pz[k];
    double lw, h = D.top(k, x, z, lod, &lw);
    const double m = D.holeAt(k, x, z);
    int kd = k;
    if (m > 0) {
        int kk = k + 1; double xk = xr + v.px[kk], zk = zr + v.pz[kk];
        while (kk < D.n - 1 && D.holeAt(kk, xk, zk) >= 0.5) { kk++; xk = xr + v.px[kk]; zk = zr + v.pz[kk]; }
        double lowK, hk = D.top(kk, xk, zk, lod, &lowK), w = smoothstep(0.25, 0.75, m);
        h += (hk - h) * w; lw += (lowK - lw) * w;
        if (m >= 0.75) kd = kk;
    }
    if (low) *low = lw;
    if (kdOut) *kdOut = kd;
    return h;
}

// one azimuth of a panorama, outward from r0: each sample of the surface fills the rows it rises over (a top seen from above)
// or hangs under (an underside from below), the rows between two samples seen one after the other interpolated. X-03: a hole in
// a top shows the top of the deck under it (the next whole one), its wall the slope between; a hole in an underside is open
void marchColumn(Pano& P, const ProbeView& v, int i, double r0, double rMax) {
    const GiantDecks& D = *v.decks;
    const int k = P.deck;
    const double a = P.a0 + i * P.da, sa = std::sin(a), ca = std::cos(a);
    float* S = &P.sh[(size_t)i * P.ne];
    float* Dd = &P.ds[(size_t)i * P.ne];
    float* G = &P.gl[(size_t)i * P.ne];
    uint8_t* K = &P.dk[(size_t)i * P.ne];
    const double R2 = 2 * v.radiusKm, zp = v.zKm, minStep = 0.02 * D.sh;
    const bool holed = D.holeable(k) && (D.holes > 0 || D.path[k].on);
    int edge = P.under ? P.ne : -1;
    bool prevVis = false, havePrev = false;
    // the steps on a fixed grid of distances (even to minStep / STEP, then each STEP longer): as the probe falls the samples
    // stay on the same ground and the clouds do not crawl
    const double rLin = minStep / STEP;
    auto rAt = [&](int k) { return k * minStep <= rLin ? k * minStep : rLin * std::pow(1 + STEP, k - rLin / minStep); };
    int k0 = r0 <= rLin ? (int)std::ceil(r0 / minStep) : (int)std::ceil(rLin / minStep + std::log(r0 / rLin) / std::log(1 + STEP));
    double prevE = 0, prevS = 0, prevD = 0, prevG = 0, hPrev = 0, rPrev = 0, r = rAt(k0);
    int prevK = k;
    for (int n = 0; n < 3000 && r < rMax; n++) {
        const double lod = r * std::max(STEP, P.da), xr = r * sa, zr = r * ca, x = xr + v.px[k], z = zr + v.pz[k];
        double low, h, xs = x, zs = z;
        int kd = k;
        const double m = holed ? D.holeAt(k, x, z) : 0.0;
        double ssHole = 0; bool haveSs = false;
        if (P.under) {
            if (m >= 0.5) { prevVis = false; havePrev = false; r = rAt(k0 + n + 1); continue; }   // open through the hole
            h = D.base(k, x, z, lod, &low);
        } else if (m > 0) {   // a hole: its floor the next whole deck's top (in its own frame), its wall this deck's own cloud
            h = holedTop(v, k, xr, zr, lod, &low, &kd);
            if (kd != k) { xs = xr + v.px[kd]; zs = zr + v.pz[kd]; }
            const double hl = std::hypot(v.sun.x, v.sun.z);
            if (hl > 1e-6) { const double eps = std::max(lod, 0.05 * D.sh); ssHole = (holedTop(v, k, xr + v.sun.x / hl * eps, zr + v.sun.z / hl * eps, lod, nullptr, nullptr) - h) / eps; haveSs = true; }
        } else h = D.top(k, x, z, lod, &low);
        const double e = std::atan2(h - r * r / R2 - zp, r);
        const double fj = (e - P.e0) / P.de;
        const int j = P.under ? (int)std::ceil(fj) : (int)std::floor(fj);
        const bool vis = P.under ? j < edge : j > edge;
        if (vis) {
            double g = 0;
            const double s = P.under ? shadeUnder(v, k, h, low, xr, zr, g)
                                     : shadeTop(v, kd, k, kd == k ? 1.0 : 0.6, xs, zs, xr, zr, h, low, havePrev ? (h - hPrev) / (r - rPrev) : 0.0, lod, g, haveSs ? &ssHole : nullptr);
            const double dist = std::sqrt(r * r + (h - zp) * (h - zp));
            int ja = P.under ? std::max(j, 0) : std::max(edge + 1, 0), jb = P.under ? std::min(edge - 1, P.ne - 1) : std::min(j, P.ne - 1);
            for (int jj = ja; jj <= jb; jj++) {
                double t = 1;
                if (prevVis && std::fabs(e - prevE) > 1e-12) t = clampd((P.e0 + jj * P.de - prevE) / (e - prevE), 0, 1);
                S[jj] = (float)(prevVis ? prevS + (s - prevS) * t : s);
                Dd[jj] = (float)(prevVis ? prevD + (dist - prevD) * t : dist);
                G[jj] = (float)(prevVis ? prevG + (g - prevG) * t : g);
                K[jj] = (uint8_t)(prevVis && prevK != kd ? std::min(prevK, kd) : kd);   // the rows between a hole's floor and its wall are the wall's
            }
            edge = j;
            prevVis = true; prevE = e; prevS = s; prevD = dist; prevG = g; prevK = kd;
        } else prevVis = false;
        hPrev = h; rPrev = r; havePrev = true;
        if (P.under ? edge <= 0 : edge >= P.ne - 1) break;
        r = rAt(k0 + n + 1);
    }
}

// the air's in-scatter toward a direction: as much of the horizon's light as the ray's optical depth (`tau`) gathers, over the
// zenith's (so high over the clouds the sky is dark down to the thin bright limb, in the haze it is bright all over), the
// sun's aureole, the giant's shadow rising in the air toward the night side at dusk
void airAt(const ProbeView& v, const Vec3& d, double tau, float* o) {
    double s = v.zenShade + (v.horShade - v.zenShade) * (1 - std::exp(-3 * tau));
    double cs = dot(d, v.sun);
    s += v.sunVis * (7 * std::exp((cs - 1) / 0.06) + 4 * std::exp((cs - 1) / 0.5));
    double hl = std::hypot(v.sun.x, v.sun.z), dl = std::hypot(d.x, d.z);
    double along = hl > 1e-9 && dl > 1e-9 ? (d.x * v.sun.x + d.z * v.sun.z) / (hl * dl) : 0.0;
    double el = std::asin(clampd(v.sun.y, -1, 1)) + along * 360.0 / v.radiusKm;   // the sun over the air a few hundred km off
    s *= 0.2 + 0.8 * smoothstep(-0.03, 0.02, el);
    v.air.at(s, o);
}
// the rings across a ray: their density and shade where the ray meets their plane (the giant's shadow on them dark)
bool ringAt(const ProbeView& v, const Vec3& d, double& alpha, double& shade) {
    double dn = dot(d, v.ringN);
    if (std::fabs(dn) < 1e-7) return false;
    double s = dot(v.ringC, v.ringN) / dn;
    if (s <= 0) return false;
    Vec3 q = d * s - v.ringC;
    double rr = length(q);
    if (rr < v.ringR0 || rr > v.ringR1) return false;
    double dens = (*v.ringProf)[clampi((int)((rr - v.ringR0) / (v.ringR1 - v.ringR0) * 255), 0, 255)];
    if (dens <= 0.02) return false;
    double tcs = -dot(q, v.sun);
    bool shadowed = tcs > 0 && length2(q + v.sun * tcs) < v.radiusKm * v.radiusKm;
    shade = 63.0 * dens * v.ringLit * (shadowed ? 0.12 : 1.0) * 0.9;
    alpha = clampd(dens * 1.6, 0.25, 1.0);
    return true;
}

// a lightning channel: a walk of midpoints displaced sideways, a branch or two
void boltPoints(const Vec3& a, const Vec3& b, uint64_t seed, std::vector<std::pair<Vec3, Vec3>>& segs, int depth, double rough) {
    if (depth == 0) { segs.push_back({a, b}); return; }
    Vec3 m = (a + b) * 0.5;
    double len = length(b - a);
    uint64_t h = mix64(seed);
    m = m + Vec3(unitFromHash(h) - 0.5, (unitFromHash(mix64(h ^ 1)) - 0.5) * 0.6, unitFromHash(mix64(h ^ 2)) - 0.5) * (len * rough);
    boltPoints(a, m, mix64(seed ^ 0xA1), segs, depth - 1, rough);
    boltPoints(m, b, mix64(seed ^ 0xB2), segs, depth - 1, rough);
    if (depth >= 4 && unitFromHash(mix64(h ^ 3)) < 0.35) {   // a branch off the midpoint
        Vec3 dir = normalize(b - a) + Vec3(unitFromHash(mix64(h ^ 4)) - 0.5, -0.4, unitFromHash(mix64(h ^ 5)) - 0.5);
        boltPoints(m, m + normalize(dir) * (len * 0.35), mix64(seed ^ 0xC3), segs, depth - 2, rough);
    }
}

inline uint32_t packRGB(float r, float g, float b) {
    int R = (int)(std::min(1.f, std::max(0.f, r)) * 255 + 0.5f), G = (int)(std::min(1.f, std::max(0.f, g)) * 255 + 0.5f), B = (int)(std::min(1.f, std::max(0.f, b)) * 255 + 0.5f);
    return 0xFF000000u | ((uint32_t)B << 16) | ((uint32_t)G << 8) | (uint32_t)R;
}
inline void addRGB(uint32_t& p, float r, float g, float b) {
    p = packRGB((p & 255) / 255.f + r, ((p >> 8) & 255) / 255.f + g, ((p >> 16) & 255) / 255.f + b);
}

// which layers the probe is among (galaxy/probe.h's `GiantDecks::layersAt`: the floor, the ceiling, the fog, a hole's shaft; the
// profile of X-04 names the layers by the same)
using Layers = GiantDecks::Layers;
Layers layersOf(const ProbeView& v) { return v.decks->layersAt(v.zKm, v.px, v.pz); }

// X-03: the aurora along a ray, B-403's curtains at the giant's scale: three sheets across the way to the magnetic pole, the
// first at the oval's distance, folded and rayed along their length, their light a gamma profile in height over the lower
// border (the profile's integral between the heights where the ray enters and leaves each slab, over the ray's climb); the
// lower curtain's light and the tops'
void auroraAt(const ProbeView& v, const Vec3& d, double& low, double& top) {
    low = top = 0;
    const double el = d.y;
    if (el < -0.03) return;
    const double h0 = v.auroraH0 - v.zKm, H = v.auroraH, H2 = 3 * v.auroraH, hPeak = h0 + H;
    if (hPeak <= 1) return;
    const double hl = std::sqrt(std::max(1e-9, 1 - el * el)), a = hl * hl / (2 * v.radiusKm);
    const double sPeak = 2 * hPeak / (el + std::sqrt(el * el + 4 * a * hPeak)), climb = el + 2 * a * sPeak;
    double dz = d.x * v.auroraDir.x + d.z * v.auroraDir.z;
    if (std::fabs(dz) < 1e-4) dz = dz < 0 ? -1e-4 : 1e-4;
    const double dxs = d.x * v.auroraDir.z - d.z * v.auroraDir.x;   // along the sheets
    auto G = [](double u, double Hk) { return u <= 0 ? 0.0 : 1.0 - std::exp(-u / Hk) * (1 + u / Hk); };
    struct Sheet { double off, w, gain, ph; };
    static const Sheet sheets[3] = {{0, 40, 1.0, 0.0}, {1200, 32, 0.45, 2.1}, {-900, 28, 0.4, 4.2}};
    const double E = sPeak * dxs, t = v.time;
    for (const Sheet& sh : sheets) {
        const double Pf = v.auroraP0 + sh.off + 110 * std::sin(E / 360 + t * 0.05 + sh.ph) + 48 * std::sin(E / 130 - t * 0.08 + 2 * sh.ph);   // the folds
        double lo = 0, tp = 0;
        auto slab = [&](double w, double wgt) {   // a dense core in thinner envelopes: soft edges
            double sA = (Pf - w) / dz, sB = (Pf + w) / dz;
            if (sA > sB) std::swap(sA, sB);
            if (sB <= 0) return;
            if (sA < 0) sA = 0;
            const double uA = sA * el + sA * sA * a - h0, uB = sB * el + sB * sB * a - h0;
            lo += wgt * (G(uB, H) - G(uA, H)) / climb; tp += wgt * (G(uB, H2) - G(uA, H2)) / climb;
        };
        slab(sh.w, 0.5); slab(1.7 * sh.w, 0.3); slab(2.6 * sh.w, 0.2);
        if (lo + tp < 1e-5) continue;
        const double Er = clampd(std::fabs(Pf / dz), 0.6 * sPeak, 2.5 * sPeak) * dxs;
        const double n = 0.5 + 0.67 * (cloudNoise2(Er / 96, t / 40 + sh.ph, (uint32_t)v.seed ^ 0xA0A1u) + 0.5 * cloudNoise2(Er / 36, t / 25 + sh.ph, (uint32_t)v.seed ^ 0xA0A2u));
        const double rays = 1 - 0.65 * (1 - clampd(n, 0, 1));
        const double patch = 0.25 + 0.75 * std::pow(clampd(0.5 + 0.67 * cloudNoise2(E / 2000, t / 120 + sh.ph, (uint32_t)v.seed ^ 0xA0A3u), 0, 1), 1.4);
        const double m = sh.gain * rays * patch * (0.75 + 0.25 * std::sin(t / 17 + sh.ph));
        low += m * lo; top += m * tp;
    }
}
}

double probeSpaceShows(const ProbeView& v) {
    if (!v.decks) return 1;
    Layers L = layersOf(v);
    if (!L.skyOpen) return 0;
    Frame F = frameOf(v);
    if (F.eHi <= 0) return 0;
    return std::exp(-hazeAt(hazeOf(*v.atm), v.zKm) / std::max(std::sin(std::min(F.eHi, PI / 2)), 0.03));
}

const ProbeViewStats& probeViewStats() { return g_stats; }

void renderProbeView(const ProbeView& v, const uint32_t* space, uint32_t* out) {
    const GiantDecks& D = *v.decks;
    const Layers L = layersOf(v);
    const Frame F = frameOf(v);
    const Mat3 camT = v.cam.transposed();
    const double S = FB_SCALE, fl = v.pj.f / S, cx = v.pj.cx / S, cy = v.pj.cy / S;
    const double fogReach = 7.0 * D.sh;
    const HazeTable& HZ = hazeOf(*v.atm);
    ProbeViewStats st;
    auto t0 = std::chrono::steady_clock::now();
    // the panoramas, out to where the haze leaves nothing of them (the extinction along the probe's level, or the horizon)
    static Pano floorP, ceilP;
    floorP.deck = ceilP.deck = -1;
    double kHere = pathTau(HZ, v.zKm, v.zKm, 1.0);
    const double reach = std::min(9000.0, std::max(200.0, 5.0 / std::max(kHere, 1e-6)));
    if (L.floor >= 0) {
        panoInit(floorP, F, L.floor, false);
        double drop = std::max(0.3, v.zKm - D.lvTop[L.floor]);
        double rMax = std::min(std::max(reach, 5.0 / std::max(pathTau(HZ, v.zKm, D.lvTop[L.floor], 1.0), 1e-6)), std::sqrt(2 * v.radiusKm * drop) + std::sqrt(2 * v.radiusKm * 20 * D.sh));
        double r0 = L.floorFar ? fogReach : std::max(0.02 * D.sh, drop * 0.05);
        if (F.eLo > -PI / 2 + 0.05 && !L.floorFar) r0 = std::max(r0, 0.6 * drop / std::tan(std::min(PI / 2 - 0.01, -std::min(-0.01, F.eLo))));
        if (L.inHole) r0 = 0.02 * D.sh;   // the shaft's walls round the probe
        dealt(floorP.na, [&](int i) { marchColumn(floorP, v, i, r0, rMax); });
    }
    if (L.ceil >= 0) {
        panoInit(ceilP, F, L.ceil, true);
        double rise = std::max(0.3, D.lvBase[L.ceil] - v.zKm);
        double r0 = std::max(0.02 * D.sh, rise * 0.05);
        if (F.eHi < PI / 2 - 0.05) r0 = std::max(r0, 0.6 * rise / std::tan(std::min(PI / 2 - 0.01, std::max(0.01, F.eHi))));
        double rMax = std::min(9000.0, std::max(reach, 5.0 / std::max(pathTau(HZ, v.zKm, D.lvBase[L.ceil], 1.0), 1e-6)));
        dealt(ceilP.na, [&](int i) { marchColumn(ceilP, v, i, r0, rMax); });
    }
    st.msPano = msSince(t0); t0 = std::chrono::steady_clock::now();
    // the fog: the deck the probe is in, the deep's haze, the lamp's beam
    const bool deep = v.zKm < D.lvBase[D.n - 1];
    const double haze = deep ? 0.14 : 0.0;   // the deep's dark air (the clear band's thin haze is the panoramas')
    const bool fogOn = L.fog >= 0 || haze > 0 || v.lamp > 0.01 || v.sight > 0.01;
    static std::vector<FogNode> fog;
    const int FW = LW / FC + 1, FH = LH / FC + 1;
    if (fogOn) {
        static DeckCache C;
        const int k = L.fog;
        if (k >= 0) {
            C.step = 0.25 * D.sh; C.n = 2 * (int)std::ceil(fogReach * 1.05 / C.step) + 1; C.half = (C.n - 1) / 2 * C.step;
            C.top.resize((size_t)C.n * C.n); C.base.resize((size_t)C.n * C.n);
            const bool holed = D.holeable(k) && (D.holes > 0 || D.path[k].on);
            dealt(C.n, [&](int j) {
                for (int i = 0; i < C.n; i++) {
                    double x = -C.half + i * C.step, z = -C.half + j * C.step;
                    C.top[(size_t)j * C.n + i] = (float)D.top(k, v.px[k] + x, v.pz[k] + z, C.step);
                    C.base[(size_t)j * C.n + i] = (float)D.base(k, v.px[k] + x, v.pz[k] + z, C.step);
                    if (holed && D.holeAt(k, v.px[k] + x, v.pz[k] + z) >= 0.5) C.top[(size_t)j * C.n + i] = C.base[(size_t)j * C.n + i] - 0.01f;   // X-03: a hole: nothing there
                }
            });
        }
        double sk[NF];
        const double s0 = 0.04 * D.sh, sEnd = deep ? 6.0 : fogReach;
        for (int n = 0; n < NF; n++) sk[n] = s0 * std::pow(sEnd / s0, n / (double)(NF - 1));
        const Vec3 beam = normalize(camT * Vec3(0, -0.21, 1));   // the lamp under the lens, tipped down a little
        const double dayHere = 0.05 + 0.95 * dayAt(v, 0, 0, 0);
        const double overMean = k >= 0 ? 2.4 * (D.convective(k) ? 1.6 : 1.25) / D.sh * 0.42 : 0;   // the taps' sum through the mean structure
        fog.resize((size_t)FW * FH);
        dealt(FH, [&](int j) {
            for (int i = 0; i < FW; i++) {
                const Vec3 d = normalize(camT * Vec3((i * FC - cx) / fl, -(j * FC - cy) / fl, 1));
                const double phase = 1 + 0.8 * std::pow(std::max(0.0, dot(d, v.sun)), 3);
                double T = 1, sun = 0, lamp = 0, fla = 0, heat = 0, glo = 0, sPrev = 0;
                for (int n = 0; n < NF; n++) {
                    const double s = sk[n], ds = s - sPrev;
                    sPrev = s;
                    const Vec3 p = d * s;
                    double rho = haze, light = 0, gl = 0;
                    if (k >= 0) {
                        double tp, bs; cacheAt(C, p.x, p.z, tp, bs);
                        const double y = v.zKm + p.y;
                        const double dk = D.density(k, v.px[k] + p.x, y, v.pz[k] + p.z, tp, bs);
                        if (dk > 0) {   // the light coming down through the deck: its top's to its base's by the share of the deck over the point, the sun's own under the top
                            const double below = std::max(0.0, tp - y), u = clampd(below / std::max(0.5, tp - bs), 0, 1);
                            light = (v.inside[k] * std::pow(std::max(v.under[k], 1e-4) / std::max(v.inside[k], 1e-4), u) + 0.5 * v.direct[k] * std::exp(-below * 1.4 / D.sh) * phase) * dayHere;
                            // the cloud over the point shades it (two taps up the light's way): a cavern's walls lit from its open roof, shafts down through the gaps
                            if (T > 0.05) {
                                double over = D.density(k, v.px[k] + p.x, y + 0.5 * D.sh, v.pz[k] + p.z, tp, bs) + 1.4 * D.density(k, v.px[k] + p.x, y + 2.0 * D.sh, v.pz[k] + p.z, tp, bs);
                                light *= std::min(2.5, std::exp(-1.1 * D.sh * (over - overMean)));   // against the deck's mean, so the fog keeps its brightness and gains its structure
                            }
                            light = light * dk / (dk + rho);
                            // X-03: the deep's glow, from the deck's base up; B-409: a little over half of it back to the lens (all of it
                            // was a flat orange that clipped at a brown dwarf's deck base), darkened where the cloud under the point
                            // is thick (two taps down the glow's way): its pockets dark against the glow
                            gl = 0.6 * v.glowUnder[k] * std::exp(-0.5 * std::max(0.0, y - bs) / D.sh);
                            if (gl > 0.02 && T > 0.05) {
                                double under = D.density(k, v.px[k] + p.x, y - 0.5 * D.sh, v.pz[k] + p.z, tp, bs) + 1.4 * D.density(k, v.px[k] + p.x, y - 2.0 * D.sh, v.pz[k] + p.z, tp, bs);
                                gl *= std::min(1.0, std::exp(-1.1 * D.sh * (under - overMean)));
                            }
                            gl *= dk;
                            rho += dk;
                        }
                    }
                    if (rho <= 0) continue;
                    const double a = 1 - std::exp(-rho * ds), w = T * a;
                    sun += w * light;
                    glo += w * (gl + haze * v.glowAir) / rho;
                    if (v.flash > 0) fla += w * std::exp(-length(p - v.flashPos) / (2.5 * D.sh));
                    heat += w;
                    T *= 1 - a;
                    if (T < 0.003) { T = 0; break; }
                }
                if (v.lamp > 0.01) {   // the lamp a little under the lens, its beam ahead and down: what lights the medium near the probe, marched on its own short steps
                    const Vec3 lampAt(0, -0.012, 0);
                    double Tl = 1, lp = 0;
                    for (int n = 0; n < 12; n++) {
                        const double s1 = 0.006 * std::pow(120.0, n / 11.0), s0p = n ? 0.006 * std::pow(120.0, (n - 1) / 11.0) : 0.0, ds = s1 - s0p;
                        const Vec3 p = d * s1, q = p - lampAt;
                        double rho = haze + (deep ? 0.55 : 0.05);   // the motes and droplets the beam shows (the deep's thick with them)
                        if (k >= 0) { double tp, bs; cacheAt(C, p.x, p.z, tp, bs); rho += D.density(k, v.px[k] + p.x, v.zKm + p.y, v.pz[k] + p.z, tp, bs); }
                        if (v.sight > 0.01) {   // X-03: what passes through the beam, once, on one giant in many
                            const Vec3 o = p - v.sightPos;
                            const double al = dot(o, v.sightAxis), e2 = (al / 0.35) * (al / 0.35) + length2(o - v.sightAxis * al) / (0.075 * 0.075);
                            if (e2 < 1.3) rho += 70 * v.sight * smoothstep(1.3, 0.85, e2) * (0.82 + 0.18 * std::sin(al * 38 - v.time * 1.1));
                        }
                        const double ql = length(q);
                        const double cone = std::exp((dot(q, beam) / std::max(ql, 1e-6) - 1) / 0.1);
                        const double a = 1 - std::exp(-rho * ds);
                        lp += Tl * a * Tl * cone / (1 + (ql / 0.08) * (ql / 0.08));
                        Tl *= 1 - a;
                    }
                    lamp = lp;
                }
                fog[(size_t)j * FW + i] = {(float)sun, (float)(lamp * v.lamp), (float)(fla * v.flash), (float)(heat * v.heat), (float)glo, (float)T};
            }
        });
        double ts = 0; for (const FogNode& f : fog) ts += f.t;
        st.fogT = ts / fog.size();
    }
    st.msFog = msSince(t0); t0 = std::chrono::steady_clock::now();
    // the layers composited at the logical nodes: the surfaces under their haze or the air (and the rings and the space
    // beyond it), the cirrus, the fog in front; each node keeps the light it adds and how much of the space shows through
    static std::vector<OutNode> on;
    on.resize((size_t)LW * LH);
    const double windE = std::sin(v.windAz), windN = std::cos(v.windAz);
    const ShadeRamp& fogRamp = v.deck[L.fog >= 0 ? L.fog : D.n - 1];
    const double tauUp = hazeAt(HZ, v.zKm);
    const Vec3 flashDir = length(v.flashPos) > 1e-6 ? normalize(v.flashPos) : Vec3(0, -1, 0);
    const double limbF = std::sqrt(TAU * v.radiusKm / (25 * D.sh));   // a grazing ray's path through the layers against a vertical one's
    static float hzTab[721][3];   // the haze's colour toward the horizon by azimuth
    for (int k = 0; k <= 720; k++) { double a = k * TAU / 720 - PI; airAt(v, Vec3(std::sin(a), 0, std::cos(a)), 9.0, hzTab[k]); }   // (the haze between the probe and a deck is the dense air's, the horizon's colour)
    // X-03: the aurora's curtains on the fog's grid (they are smooth but for their rays), where the sky is open over the probe
    static std::vector<float> aur;
    const bool aurOn = v.aurora > 0.005 && L.skyOpen;
    if (aurOn) {
        aur.assign((size_t)FW * FH * 2, 0.f);
        dealt(FH, [&](int j) {
            for (int i = 0; i < FW; i++) {
                double lo, tp;
                auroraAt(v, normalize(camT * Vec3((i * FC - cx) / fl, -(j * FC - cy) / fl, 1)), lo, tp);
                aur[((size_t)j * FW + i) * 2] = (float)lo; aur[((size_t)j * FW + i) * 2 + 1] = (float)tp;
            }
        });
    }
    const RGB glowAirCol = v.glowCol * (float)v.glowAir;
    double sumCF[2] = {0, 0}, sumSF[2] = {0, 0}, sumAur = 0;
    std::vector<double> rowCF(LH * 2, 0.0), rowSF(LH * 2, 0.0), rowAur(LH, 0.0);
    dealt(LH, [&](int j) {
        float c[3], h[3];
        for (int i = 0; i < LW; i++) {
            const Vec3 d = normalize(camT * Vec3((i + 0.5 - cx) / fl, -(j + 0.5 - cy) / fl, 1));
            const double a = std::atan2(d.x, d.z), e = std::asin(clampd(d.y, -1, 1));
            double covF, shF, dsF, glF, covC, shC, dsC, glC;
            int dkF, dkC;
            panoAt(floorP, a, e, covF, shF, dsF, glF, dkF);
            panoAt(ceilP, a, e, covC, shC, dsC, glC, dkC);
            if (covF > 0 && covC > 0) { if (dsF <= dsC) covC *= 1 - covF; else covF *= 1 - covC; }
            rowCF[j * 2] += covF; rowCF[j * 2 + 1] += covC; rowSF[j * 2] += covF * shF; rowSF[j * 2 + 1] += covC * shC;
            float bg[3];
            double tsp = 0;
            if (!L.skyOpen) airAt(v, d, 9.0, bg);   // under the clouds: the air's own light, no space beyond it
            else if (d.y >= 0) { double tau = tauUp * std::min(1 / std::max(d.y, 1e-4), limbF); airAt(v, d, tau, bg); tsp = std::exp(-tau); }
            else {   // under the horizontal, over the deck's horizon: the limb, the air grazed at the ray's lowest point
                double zt = (v.radiusKm + v.zKm) * std::cos(e) - v.radiusKm, tau = limbF * hazeAt(HZ, zt);
                airAt(v, d, tau, bg); tsp = std::exp(-tau);
            }
            if (v.glowAir > 1e-4) {   // X-03: the clear air lit from below by the deep's glow, most toward the depth
                const float gw = (float)(0.35 + 0.65 * smoothstep(0.2, -0.7, d.y));
                bg[0] += gw * glowAirCol.r; bg[1] += gw * glowAirCol.g; bg[2] += gw * glowAirCol.b;
            }
            if (aurOn && tsp > 0.002) {   // the curtains, over the air and in front of the rings and the stars
                double fx = (i + 0.5) / FC, fy = (j + 0.5) / FC;
                int fi = std::min((int)fx, FW - 2), fj = std::min((int)fy, FH - 2);
                double u = fx - fi, w = fy - fj;
                const float* a00 = &aur[((size_t)fj * FW + fi) * 2]; const float* a10 = a00 + 2; const float* a01 = a00 + FW * 2; const float* a11 = a01 + 2;
                double lo = (a00[0] * (1 - u) + a10[0] * u) * (1 - w) + (a01[0] * (1 - u) + a11[0] * u) * w;
                double tp = (a00[1] * (1 - u) + a10[1] * u) * (1 - w) + (a01[1] * (1 - u) + a11[1] * u) * w;
                if (lo + tp > 1e-5) {
                    const float A = (float)(0.75 * (1 - std::exp(-4 * v.aurora * (lo + tp))) * tsp), fT = (float)(tp / (lo + tp));
                    const RGB col = lerp(v.auroraLow, v.auroraHigh, fT);
                    bg[0] += A * col.r; bg[1] += A * col.g; bg[2] += A * col.b;
                    rowAur[j] += A;
                }
            }
            const double open = 1 - covF - covC;
            bg[0] *= (float)open; bg[1] *= (float)open; bg[2] *= (float)open;
            tsp *= open;
            if (covF > 0 || covC > 0) {
                const float* hz = hzTab[clampi((int)((a + PI) / TAU * 720 + 0.5), 0, 720)];
                h[0] = hz[0] + 0.5f * glowAirCol.r; h[1] = hz[1] + 0.5f * glowAirCol.g; h[2] = hz[2] + 0.5f * glowAirCol.b;
                for (int q = 0; q < 2; q++) {
                    double cov = q == 0 ? covF : covC;
                    if (cov <= 0) continue;
                    const double dist = q == 0 ? dsF : dsC, gl = q == 0 ? glF : glC;
                    if (q == 0) v.deck[dkF].at(shF, c); else v.below[dkC].at(shC, c);
                    if (gl > 1e-4) { c[0] += (float)(gl * v.glowCol.r); c[1] += (float)(gl * v.glowCol.g); c[2] += (float)(gl * v.glowCol.b); }
                    if (v.flash > 0) {   // the flash lights the tower it is in from within, and the ceiling over it
                        Vec3 hp = d * dist;
                        double g = q == 0 ? 1.35 * std::exp(-std::hypot(hp.x - v.flashPos.x, hp.z - v.flashPos.z) / (11 * D.sh)) : 0.9 * std::exp(-length(hp - v.flashPos) / (30 * D.sh));
                        g *= v.flash;
                        c[0] += (float)(g * v.flashCol.r); c[1] += (float)(g * v.flashCol.g); c[2] += (float)(g * v.flashCol.b);
                    }
                    const double re = dist * std::cos(e), zh = v.zKm + dist * std::sin(e) + re * re / (2 * v.radiusKm);   // the hit's altitude (the deck curves away under the straight ray)
                    const float th = (float)std::exp(-pathTau(HZ, v.zKm, zh, dist));
                    for (int k = 0; k < 3; k++) bg[k] += (float)cov * (c[k] * th + h[k] * (1 - th));
                }
            }
            if (tsp > 0.002 && v.rings) {
                double ra, rs;
                if (ringAt(v, d, ra, rs)) {
                    v.ring.at(rs, c);
                    for (int k = 0; k < 3; k++) bg[k] += (float)(tsp * ra) * c[k];
                    tsp *= 1 - ra;
                }
            }
            if (v.cirrus > 0.01 && std::fabs(d.y) > 1e-4) {   // the streaks of the cirrus layer, along the wind, racing on it
                double s = (v.cirrusKm - v.zKm) / d.y;
                double nearest = std::min(covF > 0.5 ? dsF : 1e9, covC > 0.5 ? dsC : 1e9);
                if (s > 0 && s < std::min(nearest, 400.0)) {
                    double x = d.x * s + v.px[0], z = d.z * s + v.pz[0];
                    double along = x * windE + z * windN - v.cirrusRun, across = x * windN - z * windE;
                    double n = cloudNoise2(across / 0.8, along / 9.0, (uint32_t)v.seed ^ 0xC1A5u) + 0.5 * cloudNoise2(across / 0.3, along / 3.5, (uint32_t)v.seed ^ 0xC1A6u);
                    double al = 0.6 * clampd((n - 0.15) * 2.4, 0, 1) * v.cirrus * std::exp(-s / 160);
                    if (al > 0.003) {   // sunlit ice in the band's own colour (the globe's ramp), paler where it is thicker
                        float hi[3]; v.deck[0].at((50 + 12 * al) * (0.15 + 0.85 * dayAt(v, d.x * s, d.z * s, 0)), hi);
                        for (int k = 0; k < 3; k++) bg[k] = (float)(hi[k] * al + bg[k] * (1 - al));
                        tsp *= 1 - al;
                    }
                }
            }
            if (v.flash > 0 && L.ceil >= 0) {   // the flash's light in the clear band: the air round it glowing, and a little everywhere
                float g = (float)(v.flash * (0.07 + 0.55 * std::exp((dot(d, flashDir) - 1) / 0.012)));
                bg[0] += g * v.flashCol.r; bg[1] += g * v.flashCol.g; bg[2] += g * v.flashCol.b;
            }
            if (fogOn) {
                double fx = (i + 0.5) / FC, fy = (j + 0.5) / FC;
                int fi = std::min((int)fx, FW - 2), fj = std::min((int)fy, FH - 2);
                double u = fx - fi, w = fy - fj;
                const FogNode& f00 = fog[(size_t)fj * FW + fi]; const FogNode& f10 = fog[(size_t)fj * FW + fi + 1];
                const FogNode& f01 = fog[(size_t)(fj + 1) * FW + fi]; const FogNode& f11 = fog[(size_t)(fj + 1) * FW + fi + 1];
                const double w00 = (1 - u) * (1 - w), w10 = u * (1 - w), w01 = (1 - u) * w, w11 = u * w;
                const double T = f00.t * w00 + f10.t * w10 + f01.t * w01 + f11.t * w11;
                const double sun = f00.sun * w00 + f10.sun * w10 + f01.sun * w01 + f11.sun * w11;
                const double la = f00.lamp * w00 + f10.lamp * w10 + f01.lamp * w01 + f11.lamp * w11;
                const double fa = f00.flash * w00 + f10.flash * w10 + f01.flash * w01 + f11.flash * w11;
                const double he = f00.heat * w00 + f10.heat * w10 + f01.heat * w01 + f11.heat * w11;
                const double go = f00.glow * w00 + f10.glow * w10 + f01.glow * w01 + f11.glow * w11;
                float fc[3] = {0, 0, 0};
                if (T < 0.999) fogRamp.at(63 * 0.9 * sun / (1 - T), fc);
                for (int k = 0; k < 3; k++) bg[k] = (float)(fc[k] * (1 - T) + bg[k] * T);
                const float lg = (float)(4.5 * la), fg = (float)(1.6 * fa), hg = (float)he, gg = (float)go;
                bg[0] += lg * v.lampCol.r + fg * v.flashCol.r + hg * v.heatCol.r + gg * v.glowCol.r;
                bg[1] += lg * v.lampCol.g + fg * v.flashCol.g + hg * v.heatCol.g + gg * v.glowCol.g;
                bg[2] += lg * v.lampCol.b + fg * v.flashCol.b + hg * v.heatCol.b + gg * v.glowCol.b;
                tsp *= T;
            }
            on[(size_t)j * LW + i] = {bg[0], bg[1], bg[2], (float)tsp};
        }
    });
    for (int j = 0; j < LH; j++) { for (int q = 0; q < 2; q++) { sumCF[q] += rowCF[j * 2 + q]; sumSF[q] += rowSF[j * 2 + q]; } sumAur += rowAur[j]; }
    st.floor = L.floor; st.ceil = L.ceil; st.fog = L.fog; st.inHole = L.inHole; st.aurora = sumAur / (LW * LH);
    st.flash = v.flash; st.flashPos = v.flashPos;
    if (v.flash > 0) { Vec3 fv = v.cam * v.flashPos; if (fv.z > 0) { st.flashSx = (v.pj.cx + v.pj.f * fv.x / fv.z) / S; st.flashSy = (v.pj.cy - v.pj.f * fv.y / fv.z) / S; } }
    st.sunElDeg = std::asin(clampd(v.sun.y, -1, 1)) / DEG; st.direct0 = v.direct[0]; st.diffuse0 = v.diffuse[0]; st.inside0 = v.inside[0];
    { double al, sto; if (D.bands) D.bands->at(v.px[0], v.pz[0], al, sto); else al = 0; st.alb0 = al; }
    st.covFloor = sumCF[0] / (LW * LH); st.covCeil = sumCF[1] / (LW * LH);
    st.shadeFloor = sumCF[0] > 0 ? sumSF[0] / sumCF[0] : 0; st.shadeCeil = sumCF[1] > 0 ? sumSF[1] / sumCF[1] : 0;
    st.columns = floorP.deck >= 0 ? floorP.na : ceilP.na; st.rows = floorP.deck >= 0 ? floorP.ne : ceilP.ne;
    st.msNodes = msSince(t0); t0 = std::chrono::steady_clock::now();
    // spread over the frame: the nodes read bilinearly, the space beyond added where the air lets it through
    static std::vector<int> xi;
    static std::vector<float> xf;
    xi.resize(FBW); xf.resize(FBW);
    for (int x = 0; x < FBW; x++) { double u = (x + 0.5) / S - 0.5; int i0 = clampi((int)std::floor(u), 0, LW - 2); xi[x] = i0; xf[x] = (float)clampd(u - i0, 0, 1); }
    parallelFor(FBH, 16, [&](int y0, int y1) {
        for (int y = y0; y < y1; y++) {
            double vv = (y + 0.5) / S - 0.5;
            int j0 = clampi((int)std::floor(vv), 0, LH - 2);
            float fy = (float)clampd(vv - j0, 0, 1);
            const OutNode* r0 = &on[(size_t)j0 * LW];
            const OutNode* r1 = r0 + LW;
            uint32_t* o = out + (size_t)y * FBW;
            const uint32_t* sp = space ? space + (size_t)y * FBW : nullptr;
            for (int x = 0; x < FBW; x++) {
                const int i0 = xi[x];
                const float fx = xf[x];
                const OutNode &a = r0[i0], &b = r0[i0 + 1], &c = r1[i0], &e = r1[i0 + 1];
                const float w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
                float R = a.r * w00 + b.r * w10 + c.r * w01 + e.r * w11;
                float G = a.g * w00 + b.g * w10 + c.g * w01 + e.g * w11;
                float B = a.b * w00 + b.b * w10 + c.b * w01 + e.b * w11;
                if (sp) {
                    const float T = a.t * w00 + b.t * w10 + c.t * w01 + e.t * w11;
                    if (T > 0.001f) { const uint32_t q = sp[x]; R += T * (q & 255) / 255.f; G += T * ((q >> 8) & 255) / 255.f; B += T * ((q >> 16) & 255) / 255.f; }
                }
                o[x] = packRGB(R, G, B);
            }
        }
    });
    st.msSpread = msSince(t0);
    g_stats = st;
    // the channel through the clear air: a jagged line glowing, behind the haze by its distance
    if (v.bolt && v.flash > 0.05) {
        std::vector<std::pair<Vec3, Vec3>> segs;
        boltPoints(v.flashPos, v.boltTo, v.boltSeed, segs, 6, 0.32);
        const double gr = 1.6 * S;
        for (const auto& sg : segs) {
            Vec3 pa = v.cam * sg.first, pb = v.cam * sg.second;
            if (pa.z < 0.5 || pb.z < 0.5) continue;
            double ax = v.pj.cx + v.pj.f * pa.x / pa.z, ay = v.pj.cy - v.pj.f * pa.y / pa.z, bx = v.pj.cx + v.pj.f * pb.x / pb.z, by = v.pj.cy - v.pj.f * pb.y / pb.z;
            double len = std::hypot(bx - ax, by - ay);
            if (len > 4 * FBW) continue;
            float I = (float)(v.flash * std::exp(-pathTau(HZ, v.zKm, v.zKm + sg.first.y, length(sg.first))));
            int nstep = std::max(1, (int)(len / (0.5 * S)));
            for (int s = 0; s <= nstep; s++) {
                double t = s / (double)nstep, px = ax + (bx - ax) * t, py = ay + (by - ay) * t;
                int x0 = (int)(px - gr), x1 = (int)(px + gr), y0 = (int)(py - gr), y1 = (int)(py + gr);
                for (int y = std::max(0, y0); y <= std::min(FBH - 1, y1); y++)
                    for (int x = std::max(0, x0); x <= std::min(FBW - 1, x1); x++) {
                        double dd = std::hypot(x + 0.5 - px, y + 0.5 - py) / gr;
                        if (dd >= 1) continue;
                        float g = I * (float)(0.16 * (1 - dd) * (1 - dd) + (dd < 0.35 ? 0.3 : 0.0));
                        addRGB(out[(size_t)y * FBW + x], g * 0.85f, g * 0.9f, g);
                    }
            }
        }
    }
    // the rain rising past the lens (the probe falls faster than the drops) and the motes in the deep, lit by the lamp (its
    // beam ahead and under the lens) and the lightning
    for (int pass = 0; pass < 2; pass++) {
        const double amount = pass == 0 ? v.rain : v.motes;
        if (amount < 0.01) continue;
        const int n = (int)(amount * (pass == 0 ? 220 : 160));
        const double lit = (pass == 0 ? 0.08 : 0.0) + 0.5 * v.lamp + 0.4 * v.flash;
        for (int i = 0; i < n; i++) {
            uint64_t hh = mix64(v.seed ^ (0x5A1Eull + (uint64_t)(i + pass * 1000) * 0x9E3779B97F4A7C15ULL));
            double x0 = unitFromHash(hh) * FBW, y0 = unitFromHash(mix64(hh ^ 1)) * FBH;
            double sp = (pass == 0 ? 0.6 + 0.9 * unitFromHash(mix64(hh ^ 2)) : 0.12 + 0.2 * unitFromHash(mix64(hh ^ 2))) * FBH;
            double len = (pass == 0 ? 5 + 9 * unitFromHash(mix64(hh ^ 3)) : 1.2 + unitFromHash(mix64(hh ^ 3))) * S;
            double y = std::fmod(y0 - v.time * sp, (double)FBH); if (y < 0) y += FBH;
            double xs = x0 + (pass == 0 ? 0.0 : 6 * S * std::sin(v.time * 0.7 + i));
            double bx = (xs - v.pj.cx) / (0.45 * FBW), by = (y - v.pj.cy - 0.21 * v.pj.f) / (0.45 * FBH);
            double inBeam = std::exp(-(bx * bx + by * by) * 1.6);
            float b = (float)(lit * (0.35 + 0.65 * inBeam) * (0.5 + 0.5 * unitFromHash(mix64(hh ^ 4))) * (pass == 0 ? 0.45 : 0.35));
            for (int k = 0; k < (int)len; k++) {
                int yy = (int)y + k, xx = (int)(xs + 0.15 * k);
                if (yy < 0 || yy >= FBH || xx < 0 || xx >= FBW) continue;
                float f = b * (float)(1 - (double)k / len * 0.6);
                addRGB(out[(size_t)yy * FBW + xx], f * v.lampCol.r, f * v.lampCol.g, f * v.lampCol.b);
            }
        }
    }
    // X-03: diamond hail: specks falling past the lens faster than the probe, glinting as a facet turns into the lamp's beam
    if (v.hail > 0.01) {
        const int n = (int)(v.hail * 240);
        for (int i = 0; i < n; i++) {
            const uint64_t hh = mix64(v.seed ^ (0xD1A30ull + (uint64_t)i * 0x9E3779B97F4A7C15ULL));
            const double sp = (0.25 + 0.5 * unitFromHash(mix64(hh ^ 2))) * FBH;   // a frame's height in two to four seconds
            double y = std::fmod(unitFromHash(mix64(hh ^ 1)) * FBH + v.time * sp, (double)FBH);
            if (y < 0) y += FBH;
            const double xs = unitFromHash(hh) * FBW + 3 * S * std::sin(v.time * 0.9 + i * 1.7);
            const double bx = (xs - v.pj.cx) / (0.45 * FBW), by = (y - v.pj.cy - 0.21 * v.pj.f) / (0.45 * FBH);
            const double inBeam = std::exp(-(bx * bx + by * by) * 1.6);
            const bool glint = unitFromHash(mix64(hh ^ (uint64_t)(v.time * 14) ^ 0x6117ULL)) > 0.7;
            const float b = (float)(((0.15 + 0.85 * inBeam) * v.lamp * (glint ? 2.2 : 0.3) + 0.3 * v.flash) * (0.45 + 0.55 * unitFromHash(mix64(hh ^ 4))) * v.hail);
            if (b < 0.02) continue;
            const int px = (int)xs, py = (int)y, arm = glint ? (int)(2.5 * S) : 0;
            for (int dy = -arm; dy <= arm; dy++)
                for (int dx = -arm; dx <= arm; dx++) {
                    if (dx && dy) continue;   // the speck and, when it glints, a cross of light
                    const int xx = px + dx, yy = py + dy;
                    if (xx < 0 || xx >= FBW || yy < 0 || yy >= FBH) continue;
                    const float f = b * (dx || dy ? 0.45f * (1 - (float)(std::abs(dx) + std::abs(dy)) / (arm + 1)) : 1.0f);
                    addRGB(out[(size_t)yy * FBW + xx], f * 0.88f, f * 0.94f, f);
                }
        }
    }
}
