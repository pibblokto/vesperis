// Z-02: every plan's cell (life/morph_shape_internal.h): the protocells, the small cells' eleven shapes and their arrangements, the large
// cells' eight with their variants, the colonies' five; the parts they share (a cell's look, flagella as travelling waves, cilia in
// metachronal waves, the insides)
#include "life/morph_shape_internal.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace morph {

namespace {
inline double h01(uint64_t h) { return unitFromHash(mix64(h)); }   // re-mixed: a shifted hash would span [0, 2^-n) alone
inline uint64_t hk(uint64_t seed, int64_t a, int64_t b = 0) { return hash2i(a, b, seed); }
double ease(double x) { x = clampd(x, 0, 1); return x * x * (3 - 2 * x); }
}   // namespace

void pushStrand(Shape& out, std::vector<P2>&& pts, float w0, float w1, int kind, int bank, float strength, int group) {
    if (pts.size() < 2) return;
    Strand s; s.w0 = w0; s.w1 = w1; s.kind = kind; s.bank = bank; s.strength = strength; s.group = group;
    s.x0 = s.y0 = 1e30; s.x1 = s.y1 = -1e30;
    const double e = std::max(w0, w1);
    for (const P2& q : pts) { s.x0 = std::min(s.x0, q.x - e); s.x1 = std::max(s.x1, q.x + e); s.y0 = std::min(s.y0, q.y - e); s.y1 = std::max(s.y1, q.y + e); }
    s.pts = std::move(pts);
    out.grow(s.x0, s.y0, s.x1, s.y1);
    out.strands.push_back(std::move(s));
}

Look cellLook(const Morph& m, double R) {
    Look L;
    L.bank = pigmentBank(m.pigment);
    L.density = (float)m.density;
    L.wall = (float)std::max(0.02, m.wall * 2 * R);
    L.wallDensity = (float)(0.25 + 1.5 * m.wall);
    L.phase = 1;
    L.stain = m.nucleus > 0 ? 0.08f : 0.7f;
    const bool thylakoids = m.way == LW_LIGHT && m.plastids == 0;
    L.red = thylakoids ? 0.9f : 0;
    L.lum = m.glows ? 1.0f : 0;
    L.mark = m.mark; L.marks = m.marks; L.markSeed = m.seed;
    L.mineral = m.mineral;
    return L;
}

// a flagellum from (u, v) toward `dir`: a travelling wave of its length's `waveLen`, its amplitude ramping up from the base, `phase` the beat
void flagellum(Build& B, double u, double v, double dir, double len, double width, double phase, double amp, double waveLen, int kind, float strength) {
    if (len <= 0) return;
    const int n = (int)clampd(len / std::max(1e-4, B.pixel * 2.0), 10, 72);
    const double lam = std::max(1e-3, waveLen * len), c = std::cos(dir), s = std::sin(dir);
    std::vector<P2> pts(n + 1);
    for (int i = 0; i <= n; i++) {
        const double q = len * i / n, ramp = std::min(1.0, q / (0.45 * lam));
        const double off = amp * lam * ramp * std::sin(TAU * q / lam - phase);
        pts[i] = {u + c * q - s * off, v + s * q + c * off};
    }
    B.strand(pts, (float)width, (float)(width * 0.6), kind, SB_FIELD, strength);
    if (B.m.flagKind == FS_TINSEL && kind == ST_FLAGELLUM && !smallCellPlan(B.m.plan)) {   // the tinsel's hairs (mastigonemes), seen in the electron's relief
        for (int i = 2; i < n; i += 2) {
            const P2 a = pts[i], b = pts[i + 1];
            const double tx = b.x - a.x, ty = b.y - a.y, l = std::max(1e-9, std::sqrt(tx * tx + ty * ty));
            const double hl = width * 6;
            B.strand({{a.x - ty / l * hl, a.y + tx / l * hl}, {a.x + ty / l * hl, a.y - tx / l * hl}}, (float)(width * 0.25), (float)(width * 0.2), ST_HAIR, SB_FIELD, 1);
        }
    }
}

// cilia round a radial body's edge (slide coordinates) between two angles: each beats about its normal, its phase a metachronal wave
void ciliaRound(Build& B, const Body& body, double from, double to, int n, double len, double width, double beat, double wave, float strength, double curl,
                int kind) {
    if (n <= 0 || len <= 0) return;
    for (int k = 0; k < n; k++) {
        const double phi = from + (to - from) * (k + 0.5) / n;
        P2 p, nr; edgeOf(body, phi, p, nr);
        const double ph = beat - (wave > 0 ? TAU * k / wave : 0);
        const double a = 0.75 * std::sin(ph) + curl;
        const double ca = std::cos(a), sa = std::sin(a);
        const double dx = nr.x * ca - nr.y * sa, dy = nr.x * sa + nr.y * ca;
        const double bend = 0.35 * std::cos(ph);
        std::vector<P2> pts(5);
        for (int i = 0; i < 5; i++) {
            const double q = len * i / 4.0, side = bend * q * q / len;
            pts[i] = {p.x + dx * q - dy * side, p.y + dy * q + dx * side};
        }
        pushStrand(B.out, std::move(pts), (float)width, (float)(width * 0.7), kind, SB_FIELD, strength, B.group);
    }
}

// ---- a cell's frame for its insides
P2 InFrame::point(double s, double w) const {
    if (!tube) {
        const double u = s * L;
        double sc = 1;
        if (tail > 0 && s < 0) sc = 1 - tail * 0.82 * std::pow(std::min(1.0, -s), 1.4);
        else if (head > 0 && s > 0) sc = 1 - head * 0.82 * std::pow(std::min(1.0, s), 1.4);
        const double halfAt = R * std::max(0.1, sc) * std::sqrt(std::max(0.0, 1 - s * s));
        const double lv = w * halfAt + bend * u * u / std::max(1e-9, L);
        return {cu + u * ca - lv * sa, cv + u * sa + lv * ca};
    }
    const int n = (int)line.size();
    if (n < 2) return line.empty() ? P2{} : line[0];
    const double f = (s + 1) * 0.5 * (n - 1);
    const int i = std::max(0, std::min(n - 2, (int)f));
    const double q = clampd(f - i, 0, 1);
    const P2 a = line[i], b = line[i + 1];
    const double tx = b.x - a.x, ty = b.y - a.y, l = std::max(1e-9, std::sqrt(tx * tx + ty * ty));
    const double r = rad[i] + (rad[i + 1] - rad[i]) * q;
    return {a.x + tx * q - ty / l * w * r, a.y + ty * q + tx / l * w * r};
}
double InFrame::half(double s) const {
    if (!tube) {
        double sc = 1;
        if (tail > 0 && s < 0) sc = 1 - tail * 0.82 * std::pow(std::min(1.0, -s), 1.4);
        else if (head > 0 && s > 0) sc = 1 - head * 0.82 * std::pow(std::min(1.0, s), 1.4);
        return R * std::max(0.1, sc) * std::sqrt(std::max(0.0, 1 - s * s));
    }
    const int n = (int)rad.size();
    if (n == 0) return R;
    const double f = clampd((s + 1) * 0.5 * (n - 1), 0, n - 1);
    const int i = std::min(n - 1, (int)f);
    return rad[i];
}
double InFrame::dir(double s) const {
    if (!tube) return std::atan(2 * bend * s) + std::atan2(sa, ca);
    const int n = (int)line.size();
    if (n < 2) return 0;
    const int i = std::max(0, std::min(n - 2, (int)((s + 1) * 0.5 * (n - 1))));
    return std::atan2(line[i + 1].y - line[i].y, line[i + 1].x - line[i].x);
}
InFrame frameOf(const Build& B, const Body& b, double L, double R) {
    InFrame F; F.L = L; F.R = R; F.head = b.head; F.tail = b.tail; F.bend = b.bend;
    // the body's centre and axis back in the builder's coordinates
    const double dx = b.c.x - B.x, dy = b.c.y - B.y;
    F.cu = dx * B.ca + dy * B.sa; F.cv = -dx * B.sa + dy * B.ca;
    double a = std::atan2(b.sa, b.ca) - std::atan2(B.sa, B.ca);
    if (B.mirror) { F.cv = -F.cv; a = -a; }
    F.ca = std::cos(a); F.sa = std::sin(a);
    return F;
}

// the insides: a nucleus and its nucleolus, the vacuoles (contractile, pulsing; food, with their prey), the granules streaming, the
// plastids (discs, a cup, a band, a star, a net) and their pyrenoid, an eyespot, gas vesicles, a magnetosomes' chain, droplets, a forespore
void insides(Build& B, const InFrame& F, const CellCtx& C, int cellIndex) {
    const Morph& m = B.m;
    if (C.ghost > 0.6 || C.form == CF_FOSSIL) return;
    const uint64_t seed = mix64(m.seed ^ ((uint64_t)cellIndex * 0x9E3779B97F4A7C15ULL) ^ 0x1D51DE5ULL);
    const double R = F.R, width = 2 * R, t = B.t;
    auto put = [&](double s, double w, double rx, double ry, double ang, int kind, int bank, float st) {
        const P2 p = F.point(s, w);
        B.dot(p.x, p.y, rx, ry, ang + F.dir(s), kind, bank, st);
    };
    const int pbank = pigmentBank(m.pigment);
    // the nucleus
    if (m.nucleus > 0) {
        const double rn = m.nucleus * width;
        if (m.plan == MP_CILIATE && m.variant == 1) {   // a trumpet's beads
            for (int k = 0; k < 8; k++) put(-0.6 + 1.1 * k / 7.0, 0.15 * std::sin(k * 1.3), rn * 0.35, rn * 0.3, 0, DOT_NUCLEUS, SB_FIELD, 1);
        } else if (m.plan == MP_CILIATE && (m.variant == 2 || m.variant == 3)) {   // a C
            for (int k = 0; k < 7; k++) { const double a = -1.2 + 2.4 * k / 6.0; put(0.45 * std::cos(a) - 0.1, 0.55 * std::sin(a), rn * 0.32, rn * 0.28, a, DOT_NUCLEUS, SB_FIELD, 1); }
        } else {
            const double s0 = m.plan == MP_FLAGELLATE ? 0.1 : 0.0;
            put(s0, 0.05, m.plan == MP_CILIATE ? rn * 1.5 : rn, rn, 0.2, DOT_NUCLEUS, SB_FIELD, 1);
            put(s0 + 0.02, 0.08, rn * 0.35, rn * 0.35, 0, DOT_NUCLEOLUS, SB_FIELD, 1);
            if (m.plan == MP_CILIATE) put(0.15, 0.45, rn * 0.25, rn * 0.25, 0, DOT_NUCLEUS, SB_FIELD, 1);   // the micronucleus
        }
    }
    // the vacuoles
    for (int k = 0; k < m.vacuoles; k++) {
        const uint64_t q = hk(seed, 1, k);
        if (k < (m.vacPulse > 0 ? (m.plan == MP_CILIATE ? 2 : 1) : 0)) {   // contractile: fills, then empties at once
            const double ph = t / m.vacPulse + h01(q), f = ph - std::floor(ph);
            const double r = width * (m.plan == MP_CILIATE ? 0.07 : 0.09) * (f < 0.9 ? std::cbrt(f / 0.9) : (1 - f) / 0.1);
            const double s = m.plan == MP_CILIATE ? (k == 0 ? 0.55 : -0.55) : (m.plan == MP_FLAGELLATE ? 0.65 : -0.4);
            const double w = m.plan == MP_CILIATE ? 0.4 : 0.2;
            put(s, w, r, r, 0, DOT_VACUOLE, SB_FIELD, 1);
            if (m.plan == MP_CILIATE) for (int c = 0; c < 7; c++) {   // its canals, full while it fills
                const double a = c * TAU / 7, cr = width * 0.11 * (f < 0.9 ? 0.6 + 0.4 * f : 0.6);
                const P2 o = F.point(s, w);
                B.dot(o.x + std::cos(a) * cr, o.y + std::sin(a) * cr, cr * 0.55, width * 0.012, a, DOT_CANAL, SB_FIELD, (float)(f < 0.9 ? 0.4 + 0.6 * (1 - f) : 0.3));
            }
            continue;
        }
        // a food vacuole on its round (cyclosis: Paramecium's)
        const double a = h01(q >> 3) * TAU + t * 0.05 * (0.5 + h01(q >> 7));
        const double s = 0.55 * std::cos(a), w = 0.5 * std::sin(a);
        const double r = width * (0.06 + 0.07 * h01(q >> 11));
        put(s, w, r, r, 0, DOT_FOOD, SB_FIELD, 1);
        put(s, w, r * 0.45, r * 0.35, a, DOT_GRANULE, SB_PIG + PG_BROWN - 1, 0.8f);
    }
    // the plastids
    const int plastBank = pbank;
    if (m.plastidShape == PX_DISCS) {
        const int n = std::min(48, std::max(1, m.plastids));
        const double r = R * std::min(0.32, 1.1 / std::sqrt((double)n + 2));
        for (int k = 0; k < n; k++) {
            const uint64_t q = hk(seed, 2, k);
            const double s = (h01(q) * 2 - 1) * 0.8, w = (h01(q >> 9) * 2 - 1) * 0.75;
            put(s, w, r * (0.8 + 0.5 * h01(q >> 17)), r * (0.55 + 0.3 * h01(q >> 23)), h01(q >> 29) * TAU, DOT_PLASTID, plastBank, 1);
        }
    } else if (m.plastidShape == PX_CUP) {
        for (int k = 0; k < 9; k++) {
            const double a = PI * 0.25 + PI * 1.5 * k / 8.0;
            put(0.72 * std::cos(a) - 0.15, 0.72 * std::sin(a), R * 0.42, R * 0.3, a + PI / 2, DOT_PLASTID, plastBank, 1);
        }
        put(-0.45, 0, R * 0.2, R * 0.2, 0, DOT_PYRENOID, SB_FIELD, 1);
    } else if (m.plastidShape == PX_BAND) {
        const int bands = std::max(1, std::min(3, m.plastids));
        for (int bnd = 0; bnd < bands; bnd++)
            for (int k = 0; k < 28; k++) {
                const double s = -0.9 + 1.8 * k / 27.0, w = 0.7 * std::sin(s * PI * (1.5 + bnd) + bnd * 2.1);
                put(s, w, R * 0.18, R * 0.12, 0.8, DOT_PLASTID, plastBank, 1);
                if (k % 7 == 3) put(s, w, R * 0.07, R * 0.07, 0, DOT_PYRENOID, SB_FIELD, 1);
            }
    } else if (m.plastidShape == PX_STAR) {
        const int stars = std::max(1, std::min(2, m.plastids));
        for (int st = 0; st < stars; st++) {
            const double s0 = stars == 1 ? 0 : (st == 0 ? -0.45 : 0.45);
            put(s0, 0, R * 0.22, R * 0.22, 0, DOT_PLASTID, plastBank, 1);
            put(s0, 0, R * 0.09, R * 0.09, 0, DOT_PYRENOID, SB_FIELD, 1);
            for (int a = 0; a < 7; a++) {
                const double ang = a * TAU / 7;
                for (int k = 1; k <= 3; k++) put(s0 + std::cos(ang) * 0.12 * k, std::sin(ang) * 0.2 * k, R * 0.08, R * 0.06, ang, DOT_PLASTID, plastBank, 1);
            }
        }
    } else if (m.plastidShape == PX_NET) {
        for (int k = 0; k < 60; k++) {
            const uint64_t q = hk(seed, 3, k);
            put((h01(q) * 2 - 1) * 0.85, (h01(q >> 9) * 2 - 1) * 0.8, R * 0.16, R * 0.05, h01(q >> 19) * TAU, DOT_PLASTID, plastBank, 1);
        }
    }
    if (m.eyespot) put(0.72, 0.35, width * 0.06, width * 0.045, 0.5, DOT_EYESPOT, SB_EYE, 1);
    // the granules, streaming toward the front where the cell crawls
    const double gsize = std::max(width * 0.025, std::max(0.18, B.pixel * 0.9));
    const double area = F.L * R * 3.0;
    const int ng = (int)std::min(700.0, m.granules * area / (gsize * gsize * 9.0));
    const double stream = m.motion == MO_CRAWLING ? 0.04 : 0.008;
    for (int k = 0; k < ng; k++) {
        const uint64_t q = hk(seed, 4, k);
        double s = h01(q) * 2 - 1 + t * stream * (0.5 + h01(q >> 7));
        s = s - 2 * std::floor((s + 1) / 2);
        const double w = (h01(q >> 13) * 2 - 1) * 0.78;
        put(s * 0.9, w, gsize * (0.6 + 0.8 * h01(q >> 19)), gsize * (0.6 + 0.6 * h01(q >> 23)), 0, DOT_GRANULE, SB_FIELD, (float)(0.6 + 0.4 * h01(q >> 29)));
    }
    // a small cell's marks: the nucleoid's clear zone, refractile granules at a pole or both
    if (m.nucleoid > 0 && m.nucleus <= 0) put(0, 0, F.tube ? F.L * 0.55 : R * 0.55, R * 0.42, 0, DOT_NUCLEOID, SB_FIELD, (float)m.nucleoid);
    for (int k = 0; k < m.polar; k++) put(k == 0 ? 0.74 : -0.74, 0, width * 0.17, width * 0.17, 0, DOT_DROPLET, SB_FIELD, 0.9f);
    // gas vesicles, magnetosomes, droplets
    if (m.gas > 0) {
        const int n = (int)(m.gas * 24);
        for (int k = 0; k < n; k++) {
            const uint64_t q = hk(seed, 5, k);
            put((h01(q) * 2 - 1) * 0.8, (h01(q >> 9) * 2 - 1) * 0.7, width * 0.07, width * 0.05, h01(q >> 17) * TAU, DOT_GAS, SB_FIELD, 1);
        }
    }
    for (int k = 0; k < m.magnetosomes; k++) {
        const double s = -0.6 + 1.2 * (k + 0.5) / m.magnetosomes;
        put(s, 0, width * 0.045, width * 0.045, 0, DOT_MAGNET, SB_FIELD, 1);
    }
    const bool sulphur = m.way == LW_CHEMICAL && m.droplets > 0 && smallCellPlan(m.plan);
    for (int k = 0; k < m.droplets; k++) {
        const uint64_t q = hk(seed, 6, k);
        const double r = width * (sulphur ? 0.16 : 0.08 + 0.08 * h01(q >> 17));
        put((h01(q) * 2 - 1) * 0.7, (h01(q >> 9) * 2 - 1) * 0.5, r, r, 0, DOT_DROPLET, sulphur ? SB_SULPHUR : SB_OIL, 1);
    }
    // a forespore: dark, then bright as its coat forms
    if (C.spore >= 0) {
        const bool rod = F.tube;   // a rod's spore at its place along it; a round cell's at its centre, smaller
        const double s = !rod || m.sporeAt == 0 ? 0 : (m.sporeAt == 1 ? 0.55 : 0.82);
        const double g = ease(C.spore / 0.3);
        const double ry = R * std::min(rod ? 0.95 : 0.6, m.sporeSize * (rod ? 1.0 : 0.75)) * (0.5 + 0.5 * g), rx = ry * (rod ? 1.45 : 1.08);
        put(s, 0, rx, ry, 0, DOT_SPORE, SB_FIELD, (float)clampd((C.spore - 0.3) / 0.4, 0, 1));
    }
}

namespace {
// ---- the protocells: a vesicle (vesicles in it, a clay grain), a layered vesicle, a coacervate droplet (vacuoles in it)
void protocell(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const double R = m.widthUm * 0.5 * C.scale;
    Look L = cellLook(m, R);
    L.stain = 0; L.red = 0;
    if (m.plan == MP_DROPLET) { L.density = 0.35f; L.wall = 0; L.refract = 0.55f; L.phase = 1.3f; }
    else { L.density = 0.03f; L.phase = 0.35f; L.wall = (float)std::max(0.03, m.wall * 2 * R); L.wallDensity = 0.5f; L.layers = m.plan == MP_LAMELLAR ? m.marks : 0; }
    L.ghost = (float)C.ghost;
    Body& b = B.radial(0, 0, 0, R, 1, L);
    fillOutline(b, 0, 2, 2, 2, m.wander, m.seed, B.t, 0.4);
    B.finish(b);
    const InFrame F = frameOf(B, b, R, R);
    if (m.plan == MP_VESICLE && m.variant == 1)
        for (int k = 0; k < m.vacuoles; k++) {
            const uint64_t q = hk(m.seed, 9, k);
            const double r = R * (0.15 + 0.25 * h01(q));
            const double a = h01(q >> 9) * TAU, d = (R - r) * 0.8 * h01(q >> 19);
            Look l2 = L; l2.wall = L.wall * 0.8f;
            Body& in = B.radial(std::cos(a) * d, std::sin(a) * d, 0, r, 1, l2);
            fillOutline(in, 0, 2, 2, 2, m.wander * 0.5, q, B.t, 0.6);
            B.finish(in);
        }
    if (m.plan == MP_DROPLET && m.variant == 1)
        for (int k = 0; k < m.vacuoles; k++) {
            const uint64_t q = hk(m.seed, 9, k);
            const P2 p = F.point(h01(q) * 1.6 - 0.8, h01(q >> 9) * 1.6 - 0.8);
            const double r = R * (0.08 + 0.18 * h01(q >> 19));
            B.dot(p.x, p.y, r, r, 0, DOT_VACUOLE, SB_FIELD, 1);
        }
    for (int k = 0; k < m.clay; k++) {   // clay grains at its membrane (Hanczyc et al. 2003)
        const double a = h01(m.seed + 50 + (uint64_t)k) * TAU;
        B.dot(std::cos(a) * R * 0.92, std::sin(a) * R * 0.92, R * 0.13, R * 0.09, a, DOT_GRAIN, SB_GRAIN + (int)(k % 3), 1);
    }
    const int ng = (int)(m.granules * 30);
    for (int k = 0; k < ng; k++) {
        const uint64_t q = hk(m.seed, 10, k);
        const P2 p = F.point(h01(q) * 1.6 - 0.8, h01(q >> 9) * 1.6 - 0.8);
        B.dot(p.x, p.y, R * 0.04, R * 0.04, 0, DOT_GRANULE, SB_FIELD, 0.5f);
    }
}

// a small cell's centreline: straight, bent into an arc, a helix (its height the electron's), a corkscrew with hooked ends
void smallLine(const Morph& m, double L, double R, const CellCtx& C, double phase, std::vector<P2>& pts, std::vector<float>& rad, std::vector<float>& z) {
    const int p = m.plan;
    const bool helix = p == MP_SPIRAL || p == MP_CORKSCREW;
    const int n = helix ? (int)clampd(m.turns * 10, 16, 260) : 14;
    pts.resize(n + 1); rad.resize(n + 1); z.assign(n + 1, 0);
    const double ends = std::max(0.0, L * 0.5 - R);
    for (int i = 0; i <= n; i++) {
        const double s = -1 + 2.0 * i / n;   // -1 rear .. 1 front
        double u = s * ends, v = 0;
        if (m.bend > 0) { const double a = m.bend * s * 0.5; const double rr = ends / std::max(1e-6, m.bend * 0.5); u = std::sin(a) * rr; v = (1 - std::cos(a)) * rr; }
        if (m.wave > 0) v += m.wave * R * std::sin(PI * m.waves * (s + 1));
        if (helix) {
            const double amp = m.amp * R, ph = m.turns * PI * (s + 1) - phase;
            v += amp * std::sin(ph); z[i] = (float)(amp * std::cos(ph));
            if (p == MP_CORKSCREW && m.variant == 1 && std::fabs(s) > 0.88) {   // the hooked ends (Leptospira's)
                const double k = (std::fabs(s) - 0.88) / 0.12;
                v += (s > 0 ? 1 : -1) * k * k * ends * 0.12; u -= (s > 0 ? 1 : -1) * k * k * ends * 0.05;
            }
        }
        double r = R;
        if (m.taper > 0) r *= 1 - m.taper * 0.75 * std::pow(std::fabs(s), 2.0);
        if (m.taper < 0 && std::fabs(s) > 0.92) r *= 1 + m.taper * 0.4;
        if (p == MP_ROD && m.variant == 3 && s > 0.3) r *= 1 + 0.35 * (s - 0.3) / 0.7;   // the club
        if (p == MP_CORKSCREW) r *= 1 - 0.6 * std::pow(std::fabs(s), 6);
        if (C.pinch > 0) r *= 1 - C.pinch * std::exp(-s * s * 60) * 0.97;
        pts[i] = {u, v}; rad[i] = (float)std::max(R * 0.04, r);
    }
}

// the flagella of a small cell (its frame: centred, heading +u): one at the rear pole, both poles, a tuft, all round and bundled behind it
// while it runs, splayed while it tumbles
void smallFlagella(Build& B, const Pose& P, double L, double R, const CellCtx& C) {
    const Morph& m = B.m;
    if (m.flagella <= 0 || C.limbs <= 0.01) return;
    const double len = m.flagLen * std::max(L, 2 * R) * C.limbs, w = std::max(0.012, R * 0.05), amp = 0.18;
    const double ph = P.beat;
    switch (m.flagPlace) {
        case FV_POLAR: flagellum(B, -L * 0.5, 0, PI, len, w, ph, amp, m.waveLen, ST_FLAGELLUM, 1); break;
        case FV_BIPOLAR: case FV_TUFT: {
            const int ends = m.flagPlace == FV_BIPOLAR ? 2 : 1, per = std::max(1, m.flagella / ends);
            for (int e = 0; e < ends; e++)
                for (int k = 0; k < per; k++) {
                    const double spread = (per > 1 ? (k - (per - 1) * 0.5) / (per - 1) : 0) * 0.5;
                    const double base = e == 0 ? PI : 0;
                    flagellum(B, (e == 0 ? -1 : 1) * L * 0.5, spread * R, base + spread, len, w, ph + k * 0.7, amp, m.waveLen, ST_FLAGELLUM, 1);
                }
            break;
        }
        case FV_AROUND: {
            for (int k = 0; k < m.flagella; k++) {
                const uint64_t q = hk(m.seed ^ B.sp.seed, 21, k);
                const double s = h01(q) * 1.6 - 0.8, side = (k & 1) ? 1 : -1;
                const double dir = P.tumbling ? h01(q >> 9) * TAU : PI + side * (0.25 + 0.2 * h01(q >> 9));
                flagellum(B, s * L * 0.45, side * R * 0.95, dir, len, w, ph + h01(q >> 17) * 2, amp * 0.8, m.waveLen, ST_FLAGELLUM, 1);
            }
            break;
        }
        default: break;
    }
}

void smallPili(Build& B, double L, double R, const CellCtx& C) {
    const Morph& m = B.m;
    if (m.pili <= 0 || C.limbs < 0.5) return;
    for (int k = 0; k < m.pili; k++) {
        const uint64_t q = hk(m.seed ^ B.sp.seed, 22, k);
        const bool twitch = m.motion == MO_TWITCHING;
        const double a = twitch ? (h01(q) - 0.5) * 1.2 : h01(q) * TAU;
        const double reach = twitch ? R * (2 + 4 * (0.5 + 0.5 * std::sin(B.t * 2.1 + h01(q >> 9) * TAU))) : R * (0.6 + 0.9 * h01(q >> 9));
        const double bx = std::cos(a) * L * 0.5, by = std::sin(a) * R;
        B.strand({{bx, by}, {bx + std::cos(a) * reach, by + std::sin(a) * reach}}, (float)(R * 0.02), (float)(R * 0.015), ST_PILUS, SB_FIELD, 1);
    }
}

// one small cell in the builder's frame (centred, heading +u)
void smallCell(Build& B, const Pose& P, const CellCtx& C, int idx) {
    const Morph& m = B.m;
    const int p = m.plan;
    const double R = m.widthUm * 0.5 * C.scale * (C.round > 0 ? 1 + 0.25 * C.round : 1);
    double L = m.lengthUm * C.scale * (1 + C.grow);
    if (C.round > 0) L = L + (2 * R - L) * C.round;
    Look look = cellLook(m, R);
    look.ghost = (float)C.ghost;
    look.halo = (float)(m.capsule * 2 * R);
    if (C.glow >= 0) look.lum = (float)C.glow;
    if (C.form == CF_FOSSIL) look.fossil = true;
    const bool radialPlan = p == MP_SPHERE || p == MP_SQUARE || p == MP_STAR || p == MP_CHANGING || (p == MP_BUDDING && m.variant == 1) || C.round > 0.6;
    if (radialPlan) {
        // a sphere dividing: two halves parting along the axis
        const double halves = p == MP_SPHERE && C.pinch > 0 ? 1 : 0;
        for (int hv = 0; hv <= halves; hv++) {
            const double off = halves ? (hv ? 1 : -1) * R * (0.5 * C.pinch + C.part) : 0;
            const double rr = halves ? R * (1 - 0.18 * C.pinch) : R;
            Look l = look;
            if (p == MP_SQUARE || p == MP_STAR) { l.zScale = p == MP_SQUARE ? 0.1f : 0.25f; l.density *= 0.8f; }
            Body& b = B.radial(off, 0, 0, rr, halves ? 1.0 - 0.25 * C.pinch : (p == MP_BUDDING ? 1.25 : m.aspect), l);
            if (p == MP_BUDDING) b.tail = 0.5f;
            const double wob = p == MP_CHANGING ? 0.35 : 0.2;
            fillOutline(b, p == MP_SQUARE || p == MP_STAR ? m.sym : 0, m.n1, m.n2, m.n3, m.wander, m.seed ^ (uint64_t)idx, B.t, wob);
            B.finish(b);
            insides(B, frameOf(B, b, rr * b.aspect, rr), C, idx * 2 + hv);
        }
        if (m.spines > 0 && C.limbs > 0.3 && p == MP_SPHERE) {   // the hami: fine spines curled into a crook at their tips
            const Body& b0 = B.out.bodies.back();
            for (int k = 0; k < m.spines; k++) {
                const double phi = (k + 0.5) * TAU / m.spines + 0.2 * std::sin(k * 2.1);
                P2 e, nr; edgeOf(b0, phi, e, nr);
                const double len = R * 2 * m.spineLen * (0.8 + 0.4 * h01(m.seed + (uint64_t)k)), hr = len * 0.16, turn = (k & 1) ? 1.0 : -1.0;
                std::vector<P2> pts;
                for (int i = 0; i <= 6; i++) pts.push_back({e.x + nr.x * len * i / 6.0, e.y + nr.y * len * i / 6.0});
                const double a0 = std::atan2(nr.y, nr.x) - turn * PI * 0.5;
                const P2 c = {e.x + nr.x * len + std::cos(a0 + PI) * hr, e.y + nr.y * len + std::sin(a0 + PI) * hr};
                for (int i = 1; i <= 7; i++) { const double a = a0 + turn * PI * 1.15 * i / 7.0; pts.push_back({c.x + std::cos(a) * hr, c.y + std::sin(a) * hr}); }
                pushStrand(B.out, std::move(pts), (float)(R * 0.05), (float)(R * 0.035), ST_SPINE, SB_FIELD, 0.8f, B.group);
            }
        }
        if (p == MP_SQUARE && m.variant == 1 && C.pinch == 0) {   // a second sheet under it
            Look l = look; l.zScale = 0.1f; l.density *= 0.7f;
            Body& b = B.radial(R * 0.35, R * 0.3, 0.2, R * 0.92, m.aspect, l);
            fillOutline(b, m.sym, m.n1, m.n2, m.n3, m.wander, m.seed ^ 0x51ULL, B.t, 0.2);
            B.finish(b);
        }
        if (p == MP_STAR && m.variant == 1) {   // the prosthecae
            const int arms = std::max(3, std::min(8, m.sym));
            for (int a = 0; a < arms; a++) {
                const double ang = a * TAU / arms + 0.3;
                B.tube({{std::cos(ang) * R * 0.6, std::sin(ang) * R * 0.6}, {std::cos(ang) * R * (1.2 + m.stalk * 2), std::sin(ang) * R * (1.2 + m.stalk * 2)}},
                       {(float)(R * 0.22), (float)(R * 0.16)}, look);
            }
        }
        if (p == MP_BUDDING && m.variant == 1) {   // a bud straight off its narrow pole
            const double br = R * (0.25 + 0.55 * C.bud);
            if (C.bud > 0 || (m.seed & 3) == 0) {
                Body& bud = B.radial(-R * 1.25 - br * 0.9 - C.part * R, 0, 0, br, 1.15, look);
                fillOutline(bud, 0, 2, 2, 2, m.wander, m.seed ^ 0xB0DULL, B.t, 0.2);
                B.finish(bud);
            }
            B.strand({{R * 1.2, 0}, {R * 1.2 + m.lengthUm * 0.8, R * 0.2}}, (float)(R * 0.1), (float)(R * 0.08), ST_STALK, SB_FIELD, 1);   // the fibrous stalk
        }
    } else {
        std::vector<P2> pts; std::vector<float> rad, z;
        const double phase = p == MP_CORKSCREW ? B.t * TAU * 1.5 : 0;
        smallLine(m, L, R, C, phase, pts, rad, z);
        if (C.pinch >= 0.97) {   // two daughters
            const int n = (int)pts.size(), mid = n / 2;
            for (int hv = 0; hv < 2; hv++) {
                std::vector<P2> pp; std::vector<float> rr, zz;
                const double shift = (hv ? 1 : -1) * (C.part * R * 2);
                for (int i = hv ? mid : 0; i <= (hv ? n - 1 : mid); i++) { pp.push_back({pts[i].x + shift, pts[i].y}); rr.push_back(std::max(rad[i], (float)(R * 0.5))); zz.push_back(z[i]); }
                B.tube(pp, rr, look, &zz);
            }
        } else {
            B.tube(pts, rad, look, &z);
        }
        InFrame F; F.tube = true; F.line = pts; F.rad = rad; F.R = R; F.L = L * 0.5;
        insides(B, F, C, idx);
        if (p == MP_STALKED || (p == MP_BUDDING && m.variant == 0)) {
            const double sl = m.stalk * L;
            const bool ribbon = p == MP_STALKED && m.variant == 1;
            const P2 base = pts.front();
            const double dir = p == MP_STALKED && m.variant == 2 ? PI * 0.65 : PI;
            std::vector<P2> sp; std::vector<P2> sp2;
            for (int i = 0; i <= 24; i++) {
                const double q = sl * i / 24.0;
                const double tw = ribbon ? R * 0.35 * std::sin(q / R * 1.7) : R * 0.15 * std::sin(q / sl * PI);
                sp.push_back({base.x + std::cos(dir) * q - std::sin(dir) * tw, base.y + std::sin(dir) * q + std::cos(dir) * tw});
                if (ribbon) sp2.push_back({base.x + std::cos(dir) * q + std::sin(dir) * tw, base.y + std::sin(dir) * q - std::cos(dir) * tw});
            }
            B.strand(sp, (float)(R * (ribbon ? 0.18 : 0.12)), (float)(R * (ribbon ? 0.18 : 0.1)), ST_STALK, ribbon ? SB_MINERAL + MI_IRON : SB_FIELD, 1);
            if (ribbon) B.strand(sp2, (float)(R * 0.18), (float)(R * 0.18), ST_STALK, SB_MINERAL + MI_IRON, 1);
            if (m.holdfast) B.dot(sp.back().x, sp.back().y, R * 0.3, R * 0.3, 0, DOT_GRAIN, SB_FIELD, 1);
            if (p == MP_BUDDING) {   // the bud at the hypha's tip
                const double br = R * (0.3 + 0.6 * std::max(C.bud, h01(m.seed >> 3) * 0.7));
                Body& bud = B.radial(sp.back().x + std::cos(dir) * br * (1 + C.part * 2), sp.back().y + std::sin(dir) * br * (1 + C.part * 2), dir, br, 1.2, look);
                fillOutline(bud, 0, 2, 2, 2, m.wander, m.seed ^ 0xB0D, B.t, 0.2);
                B.finish(bud);
            }
        }
        smallPili(B, L, R, C);
    }
    smallFlagella(B, P, L, R, C);
}

// where the cells of a small cell's arrangement lie (centres and headings, in the specimen's frame)
struct Placed { double u, v, a; };
void arrangement(const Morph& m, uint64_t seed, std::vector<Placed>& out) {
    const double w = m.widthUm, L = std::max(m.lengthUm, w);
    out.clear();
    switch (m.arrangement) {
        case AR_PAIRS: out = {{-L * 0.52, 0, 0}, {L * 0.52, 0, 0.05}}; break;
        case AR_CHAINS: {
            double u = 0, v = 0, a = 0;
            const double bend = (h01(seed) - 0.5) * 0.5;
            for (int k = 0; k < m.groupN; k++) { out.push_back({u, v, a}); a += bend + (h01(seed >> (k % 40)) - 0.5) * 0.2; u += std::cos(a) * L * 1.04; v += std::sin(a) * L * 1.04; }
            const double cu = u * 0.5, cv = v * 0.5;
            for (Placed& p : out) { p.u -= cu; p.v -= cv; }
            break;
        }
        case AR_CLUSTERS:
            for (int k = 0; k < m.groupN; k++) {
                const double r = std::sqrt(k + 0.5) * w * 0.55, a = k * 2.39996 + h01(hk(seed, 31, k)) * 0.5;
                out.push_back({r * std::cos(a), r * std::sin(a), h01(hk(seed, 32, k)) * TAU});
            }
            break;
        case AR_FOURS: out = {{-w * 0.5, -w * 0.5, 0}, {w * 0.5, -w * 0.5, 0}, {-w * 0.5, w * 0.5, 0}, {w * 0.5, w * 0.5, 0}}; break;
        case AR_PACKETS:
            for (int k = 0; k < 8; k++) { const double back = k < 4 ? 0.3 : 0; out.push_back({((k & 1) - 0.5) * w * 1.0 + back * w, (((k >> 1) & 1) - 0.5) * w * 1.0 + back * w, 0}); }
            break;
        case AR_PALISADES:
            for (int k = 0; k < m.groupN; k++) out.push_back({(h01(hk(seed, 33, k)) - 0.5) * L * 0.3, (k - (m.groupN - 1) * 0.5) * w * 1.08, PI * 0.5 + (h01(hk(seed, 34, k)) - 0.5) * 0.35});
            break;
        case AR_ROSETTES:
            for (int k = 0; k < m.groupN; k++) { const double a = k * TAU / m.groupN + (h01(hk(seed, 35, k)) - 0.5) * 0.3; const double r = L * 0.5 + m.stalk * L; out.push_back({std::cos(a) * r, std::sin(a) * r, a}); }
            break;
        default: out = {{0, 0, 0}}; break;
    }
}

// ---- the filament: a chain of short cells along a sweeping line (a sheath round them, beads with heterocysts, a helix)
void filament(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const double R = m.widthUm * 0.5 * C.scale, cl = std::max(m.widthUm * m.aspect, m.widthUm * 0.4) * C.scale;
    const int n = std::max(2, m.groupN);
    const double total = cl * n;
    const bool beads = m.variant == 1, helixF = m.variant == 3;
    Look look = cellLook(m, R);
    look.ghost = (float)C.ghost;
    if (C.form == CF_FOSSIL) look.fossil = true;
    // the line
    const uint64_t s = m.seed;
    const int pts = n * 2 + 1;
    std::vector<P2> line(pts);
    double a = (h01(s) - 0.5) * 0.6, u = -total * 0.5, v = 0;
    const double sweep = m.bend / std::max(1.0, total / (R * 40));
    for (int i = 0; i < pts; i++) {
        line[i] = {u, v};
        const double ph = B.t * (m.motion == MO_GLIDING ? 0.6 : 0.1);
        a += (sweep + 0.25 * std::sin(i * 0.13 + h01(s >> 7) * 6 + ph * (i > pts * 0.7 ? 1 : 0))) * (total / pts) / (R * 20);
        u += std::cos(a) * total / (pts - 1); v += std::sin(a) * total / (pts - 1);
    }
    if (helixF) for (int i = 0; i < pts; i++) line[i].y += m.amp * R * std::sin(i * TAU * m.turns / pts - B.t * 0.8);
    {   // its middle at the specimen's place (a first look at its cells finds them there)
        const P2 mid = line[pts / 2];
        for (P2& q : line) { q.x -= mid.x; q.y -= mid.y; }
    }
    // the sheath round it all
    if (m.sheath > 0) {
        Look sh = look; sh.density = 0.06f; sh.bank = SB_FIELD; sh.wall = (float)(R * 0.12); sh.wallDensity = 0.12f; sh.stain = 0; sh.red = 0; sh.phase = 0.3f; sh.mark = MK_SMOOTH;
        std::vector<float> rr(pts, (float)(R * (1 + m.sheath * 2)));
        B.tube(line, rr, sh);
    }
    // the break (a fragmenting filament's dead cell, then its two parts apart)
    const int brk = C.pinch > 0 ? n / 2 : -1;
    for (int k = 0; k < n; k++) {
        const int i0 = k * 2, i1 = k * 2 + 2;
        const bool hetero = beads && m.vacuoles > 0 && k % 9 == 4;
        double shift = 0;
        if (brk >= 0 && C.part > 0) shift = (k < brk ? -1 : 1) * C.part * R * 3;
        const P2 a0 = line[i0], a1 = line[i1];
        const double dx = a1.x - a0.x, dy = a1.y - a0.y, l = std::max(1e-9, std::sqrt(dx * dx + dy * dy));
        const double sx = dx / l * shift, sy = dy / l * shift;
        Look lk = look;
        if (hetero) { lk.density *= 0.35f; lk.wall *= 2.2f; lk.red = 0; }
        if (k == brk && C.pinch > 0) { lk.ghost = (float)std::max(C.ghost, C.pinch); }
        if (beads) {
            const P2 c = {(a0.x + a1.x) * 0.5 + sx, (a0.y + a1.y) * 0.5 + sy};
            const double rr = hetero ? R * 1.15 : R * (0.92 + 0.1 * h01(hk(s, 41, k)));
            Body& b = B.radial(c.x, c.y, std::atan2(dy, dx), rr, hetero ? 1.1 : 0.95 + 0.1 * h01(hk(s, 42, k)), lk);
            fillOutline(b, 0, 2, 2, 2, 0.02, s ^ (uint64_t)k, B.t, 0.1);
            B.finish(b);
            if (hetero) { B.dot(c.x + dx / l * rr * 0.85, c.y + dy / l * rr * 0.85, rr * 0.18, rr * 0.18, 0, DOT_DROPLET, SB_OIL, 1); }
            else insides(B, frameOf(B, b, rr, rr), C, k);
        } else {
            const double gap = R * 0.06;
            std::vector<P2> seg = {{a0.x + dx / l * gap + sx, a0.y + dy / l * gap + sy}, {line[i0 + 1].x + sx, line[i0 + 1].y + sy}, {a1.x - dx / l * gap + sx, a1.y - dy / l * gap + sy}};
            const bool end = k == 0 || k == n - 1;
            std::vector<float> rr = {(float)(R * (k == 0 ? 0.7 : 0.98)), (float)R, (float)(R * (k == n - 1 ? 0.7 : 0.98))};
            if (end && m.taper > 0) { if (k == 0) rr[0] *= 0.6f; else rr[2] *= 0.6f; }
            Body& b = B.tube(seg, rr, lk);
            (void)b;
            InFrame F; F.tube = true; F.line = seg; F.rad = rr; F.R = R; F.L = cl * 0.5;
            insides(B, F, C, k);
        }
    }
}

// ---- the large cells
void largeCell(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const int p = m.plan, v = m.variant;
    const uint64_t s = m.seed;
    const double R = m.widthUm * 0.5 * C.scale * (C.round > 0 ? 1 - 0.15 * C.round : 1);
    double aspect = m.aspect;
    if (C.round > 0) aspect = aspect + (1 - aspect) * C.round;
    Look look = cellLook(m, R);
    look.ghost = (float)C.ghost;
    if (C.glow >= 0) look.lum = (float)C.glow;
    if (C.form == CF_FOSSIL) look.fossil = true;
    std::vector<Lobe> lobes;
    double wobble = 0.15, wander = m.wander;
    int sym = 0; double n1 = 2, n2 = 2, n3 = 2;
    float head = 0, tail = 0, bend = 0;
    switch (p) {
        case MP_AMOEBA: case MP_GATHERING: {
            const int nl = v == 3 ? 1 : std::max(1, m.podCount);
            if (m.pods == PD_LOBES || p == MP_GATHERING)
                for (int k = 0; k < nl; k++) {
                    const uint64_t q = hk(s, 51, k);
                    const double ext = 0.5 + 0.5 * std::sin(B.t * (0.08 + 0.12 * h01(q)) + h01(q >> 9) * TAU);
                    lobes.push_back({(h01(q >> 17) - 0.5) * (v == 3 ? 0.4 : 2.4), (v == 3 ? 0.35 : 0.18 + 0.28 * ext) * C.limbs, v == 3 ? 0.9 : 0.25 + 0.2 * h01(q >> 23)});
                }
            if (C.toward > 0) {   // wrapping a prey: two arms either side of it
                lobes.push_back({C.towardAngle - 0.45, 0.7 * C.toward, 0.3});
                lobes.push_back({C.towardAngle + 0.45, 0.7 * C.toward, 0.3});
            }
            if (v == 2) wander = 0.03;
            wobble = 0.12;
            if (v == 3) tail = 0.3f;
            break;
        }
        case MP_FLAGELLATE:
            if (v == 0) { tail = (float)m.taper; head = 0.25f; aspect *= 1 + 0.12 * std::sin(B.t * 0.7) * (1 - C.round); }   // metaboly
            else if (v == 2) tail = (float)m.taper;
            if (v == 1) lobes.push_back({0, 0.08, 0.15});   // the papilla
            break;
        case MP_CILIATE:
            if (v == 0) { lobes.push_back({1.75, -0.14, 0.45}); tail = (float)(m.taper * 0.6); head = 0.12f; bend = 0.04f; }
            else if (v == 1 || v == 2) { tail = (float)m.taper; head = -0.0f; }
            break;
        case MP_SHELLED:
            if (v == 1 || v == 2) { head = tail = (float)m.taper; if (v == 2) bend = (float)(m.bend * 0.15); }
            else if (v == 3) { sym = m.sym; n1 = m.n1; n2 = m.n2; n3 = m.n3; }
            wander = 0.005;
            break;
        case MP_TEST: if (v == 0 || v == 2) head = (float)m.taper; wander = 0.02; break;
        case MP_LATTICE: if (v == 2) head = 0.7f; wander = 0.005; break;
        case MP_ARMOURED: sym = v == 0 ? 4 : 3; n1 = m.n1; n2 = m.n2; n3 = m.n3; wander = 0.01; if (v != 0) { sym = 0; } break;
        default: break;
    }
    if (C.round > 0) { for (Lobe& l : lobes) l.amp *= 1 - C.round; head *= (float)(1 - C.round); tail *= (float)(1 - C.round); bend *= (float)(1 - C.round); }
    // the shells' looks: a box of its mineral, a test of grains, a lattice, the plates
    Look body = look;
    if (p == MP_SHELLED) { body.layout = v == 0 ? 1 : (v == 3 ? 0 : 2); body.zScale = 0.35f; body.refract = 0.25f; body.wallDensity = 0.5f; }
    if (p == MP_TEST) { body.density = 0.3f; body.wallDensity = 0.9f; body.zScale = v == 1 ? 0.5f : 1; body.gapPhi = 0; body.gapW = v == 1 ? 0 : 0.35f; body.stain = 0.02f;
                        if (v == 1) { body.bank = SB_PIG + PG_BROWN - 1; body.density = 0.55f; } }
    if (p == MP_LATTICE) { body.density = 0.05f; body.wallDensity = 0.8f; body.refract = 0.3f; body.stain = 0; }
    if (p == MP_ARMOURED) { body.wallDensity = 0.55f; }
    if (p == MP_CILIATE && v == 1) { body.mark = MK_RIBS; body.layout = 3; body.marks = 24; }
    if (p == MP_CILIATE && v == 0) { body.layout = 3; }
    if (p == MP_FLAGELLATE && m.mark == MK_RIBS) body.layout = 4;
    const double t = B.t;
    Body& b = B.radial(0, 0, 0, R, aspect, body);
    b.head = head; b.tail = tail; b.bend = bend;
    fillOutline(b, sym, n1, n2, n3, wander, s, t, wobble, lobes.data(), (int)lobes.size());
    B.finish(b);
    const double L = R * aspect;
    const Body bodyCopy = b;   // the outline the appendages read (b moves when more bodies are added)
    // the lattice's inner shells and central capsule, the test's aperture and pods, the foram's... (inside the outline)
    if (p == MP_LATTICE) {
        for (int k = 1; k <= (v == 1 ? 0 : 1 + (int)(h01(s >> 3) * 1.99)); k++) {
            Look in = body; in.marks = m.marks + 4 * k; in.wallDensity = 0.5f;
            Body& ib = B.radial(0, 0, 0, R * (0.62 - 0.22 * (k - 1)), 1, in);
            fillOutline(ib, 0, 2, 2, 2, 0.003, s ^ (uint64_t)k, t, 0);
            B.finish(ib);
        }
        Look cap = look; cap.mark = MK_SMOOTH; cap.bank = SB_PIG + (m.pigment > PG_NONE ? m.pigment : PG_RED) - 1; cap.density = 0.5f; cap.wall = (float)(R * 0.03);
        Body& cb = B.radial(0, 0, 0, R * (v == 1 ? 0.3 : 0.28), 1, cap);
        fillOutline(cb, 0, 2, 2, 2, 0.02, s ^ 0xCA9, t, 0.05);
        B.finish(cb);
    }
    if (C.form != CF_FOSSIL && C.ghost < 0.6) {
        if (p == MP_TEST) {   // the cytoplasm inside, faint through the wall
            InFrame F = frameOf(B, bodyCopy, L, R); F.R = R * 0.7;
            insides(B, F, C, 0);
        } else if (p != MP_LATTICE) {
            InFrame F = frameOf(B, bodyCopy, L, R * 0.9);
            insides(B, F, C, 0);
        } else {
            // the symbionts and granules in the frothy outer cytoplasm
            for (int k = 0; k < (m.plastids > 0 ? m.plastids : 0); k++) {
                const uint64_t q = hk(s, 57, k);
                const double a = h01(q) * TAU, r = R * (0.35 + 0.5 * h01(q >> 9));
                B.dot(std::cos(a) * r, std::sin(a) * r, R * 0.05, R * 0.05, 0, DOT_PLASTID, SB_PIG + PG_ORANGE - 1, 1);
            }
        }
    }
    // the appendages
    const double width = 2 * R;
    const double limbs = C.limbs * (C.form == CF_FOSSIL ? 0 : 1) * (1 - C.ghost);
    switch (p) {
        case MP_AMOEBA:
            if ((v == 1 || v == 2) && limbs > 0) {   // fine threads, or the heliozoan's stiff axopods with their beads
                const int n = v == 2 ? m.podCount : std::max(4, m.podCount * 2);
                for (int k = 0; k < n; k++) {
                    const uint64_t q = hk(s, 52, k);
                    const double phi = v == 2 ? k * TAU / n + h01(q) * 0.1 : (h01(q) - 0.5) * 2.6;
                    P2 e, nr; edgeOf(bodyCopy, phi, e, nr);
                    const double len = width * (v == 2 ? m.flagLen : 0.4 + 0.8 * h01(q >> 9)) * limbs;
                    const double wav = v == 2 ? 0 : R * 0.08 * std::sin(t * 0.5 + h01(q >> 17) * TAU);
                    std::vector<P2> pts;
                    for (int i = 0; i <= 8; i++) { const double qq = len * i / 8.0; pts.push_back({e.x + nr.x * qq - nr.y * wav * std::sin(i * 0.5), e.y + nr.y * qq + nr.x * wav * std::sin(i * 0.5)}); }
                    pushStrand(B.out, std::move(pts), (float)(width * (v == 2 ? 0.012 : 0.02)), (float)(width * 0.004), ST_THREAD, SB_FIELD, 1, B.group);
                    if (v == 2) for (int g = 0; g < 3; g++) {
                        const double f = std::fmod(h01(q >> (20 + g)) + t * 0.02 * (g + 1), 1.0) * len;
                        B.out.dots.push_back(Dot{}); Dot& d = B.out.dots.back();
                        d.c = {e.x + nr.x * f, e.y + nr.y * f}; d.rx = d.ry = width * 0.012; d.kind = DOT_GRANULE; d.group = B.group;
                    }
                }
            }
            break;
        case MP_FLAGELLATE: {
            if (limbs <= 0) break;
            const double fl = m.flagLen * 2 * L * limbs, fw = width * 0.025;
            if (v == 1) {   // two, breaststroke
                for (int k = 0; k < 2; k++) {
                    const double sd = k ? 1 : -1;
                    const double sweep = 0.35 + 0.9 * (0.5 + 0.5 * std::sin(P.beat));
                    flagellum(B, L * 0.95, sd * R * 0.1, sd * sweep, fl, fw, P.beat * 0.5, 0.12, 0.9, ST_FLAGELLUM, 1);
                }
            } else if (v == 3) {   // one in a collar of microvilli
                flagellum(B, L * 0.95, 0, 0, fl, fw, P.beat, 0.2, m.waveLen, ST_FLAGELLUM, 1);
                const int n = 18;
                for (int k = 0; k < n; k++) {
                    const double sd = (k - (n - 1) * 0.5) / ((n - 1) * 0.5);
                    const double a = sd * 0.35, cl = width * (0.9 + 0.6 * m.ciliaLen) * limbs;
                    B.strand({{L * 0.9, sd * R * 0.35}, {L * 0.9 + std::cos(a) * cl, sd * R * 0.35 + std::sin(a) * cl}}, (float)(width * 0.008), (float)(width * 0.006), ST_COLLAR, SB_FIELD, 0.8f);
                }
            } else {
                const int n = std::max(1, m.flagella);
                for (int k = 0; k < n; k++) {
                    const double sd = n > 1 ? (k - (n - 1) * 0.5) / ((n - 1) * 0.5) : 0;
                    const bool trailing = v == 2 && k == n - 1;
                    const double dir = trailing ? PI - 0.15 : sd * 0.5, len = trailing ? fl * 0.8 : fl * (k == 1 && v == 0 ? 0.25 : 1);
                    flagellum(B, L * 0.9, sd * R * 0.2, dir, len, fw, P.beat + k * 0.9, 0.16, m.waveLen, ST_FLAGELLUM, 1);
                }
            }
            break;
        }
        case MP_CILIATE: {
            if (limbs <= 0) break;
            const double pix = std::max(B.pixel * 2.2, width * 0.012);
            const double per = PI * (L + R) * 2;
            const double cl = m.ciliaLen * width * limbs, cw = std::max(width * 0.004, 0.1);
            if (v == 0) ciliaRound(B, bodyCopy, 0, TAU, (int)std::min(420.0, per / pix), cl, cw, P.beat, m.ciliaWave, 0.75f);
            else if (v == 1) {
                ciliaRound(B, bodyCopy, -0.75, 0.75, (int)std::min(80.0, per * 0.25 / pix), cl * 2.2, cw * 1.6, P.beat, m.ciliaWave, 1, 0.5);
                ciliaRound(B, bodyCopy, 0.8, TAU - 0.8, (int)std::min(160.0, per * 0.6 / pix * 0.5), cl * 0.6, cw, P.beat * 0.8, m.ciliaWave, 0.6f);
            } else if (v == 2) {
                ciliaRound(B, bodyCopy, -0.9, 0.9, (int)std::min(80.0, per * 0.3 / pix), cl * 1.8, cw * 1.4, P.beat, m.ciliaWave, 1, 0.9);
            } else {
                ciliaRound(B, bodyCopy, 0.15, 1.9, (int)std::min(60.0, per * 0.25 / pix), cl * 1.3, cw * 1.4, P.beat, m.ciliaWave, 1, 0.3);
                for (int k = 0; k < 12; k++) {   // the cirri: tufts that step
                    const uint64_t q = hk(s, 53, k);
                    const double phi = (k < 4 ? 0.0 : (k < 8 ? PI : PI * 0.5)) + (h01(q) - 0.5) * 1.2;
                    P2 e, nr; edgeOf(bodyCopy, phi, e, nr);
                    const double a = 0.5 * std::sin(P.beat * 0.4 + (k & 3) * 1.3);
                    const double ca = std::cos(a), sa = std::sin(a), len = width * (0.2 + 0.12 * h01(q >> 7)) * limbs;
                    pushStrand(B.out, {{e.x, e.y}, {e.x + (nr.x * ca - nr.y * sa) * len, e.y + (nr.x * sa + nr.y * ca) * len}}, (float)(width * 0.035), (float)(width * 0.012), ST_CILIUM, SB_FIELD, 1, B.group);
                }
            }
            if (v == 2 && m.stalk > 0) {   // the bell's stalk: coils at once, stretches out slowly
                const double period = 7 + 9 * h01(s >> 11), ph = std::fmod(t / period + h01(s >> 13), 1.0);
                const double coil = ph < 0.015 ? ph / 0.015 : (ph < 0.4 ? 1 - (ph - 0.015) / 0.385 : 0);
                const double sl = m.stalk * 2 * L * (1 - 0.65 * coil);
                std::vector<P2> pts;
                for (int i = 0; i <= 60; i++) {
                    const double q = sl * i / 60.0;
                    pts.push_back({-L * 0.95 - q, coil * R * 0.35 * std::sin(i * 1.1)});
                }
                B.strand(pts, (float)(width * 0.05), (float)(width * 0.045), ST_STALK, SB_FIELD, 1);
                B.dot(pts.back().x, pts.back().y, width * 0.07, width * 0.07, 0, DOT_GRAIN, SB_FIELD, 1);
            }
            break;
        }
        case MP_SHELLED:
            if (m.spines > 0) for (int k = 0; k < m.spines; k++) {
                const double phi = k * TAU / m.spines;
                P2 e, nr; edgeOf(bodyCopy, phi, e, nr);
                const double len = m.spineLen * width;
                pushStrand(B.out, {{e.x, e.y}, {e.x + nr.x * len, e.y + nr.y * len}}, (float)(width * 0.012), (float)(width * 0.005), ST_SPINE, SB_MINERAL + m.mineral, 1, B.group);
            }
            if (v == 1 || v == 2) {   // the raphe in two halves either side of its central nodule
                for (int half = 0; half < 2; half++) {
                    std::vector<P2> pts;
                    for (int i = 0; i <= 8; i++) {
                        const double q = (half ? 1 : -1) * (0.06 + 0.84 * i / 8.0);
                        pts.push_back({q * L, bend * q * q * L});
                    }
                    B.strand(pts, (float)(width * 0.02), (float)(width * 0.02), ST_SPINE, SB_MINERAL + m.mineral, 0.8f);
                }
                B.dot(0, 0, width * 0.07, width * 0.11, 0, DOT_GRAIN, SB_MINERAL + m.mineral, 0.6f);
            }
            if (m.stalk > 0 && limbs > 0) B.strand({{-L, 0}, {-L - m.stalk * 2 * L, R * 0.2}}, (float)(width * 0.06), (float)(width * 0.05), ST_STALK, SB_FIELD, 0.7f);
            break;
        case MP_TEST:
            if (limbs > 0) {   // pods out of its aperture
                for (int k = 0; k < std::max(1, m.podCount); k++) {
                    const uint64_t q = hk(s, 54, k);
                    const double reach = width * (0.25 + 0.5 * (0.5 + 0.5 * std::sin(t * 0.15 + h01(q) * TAU))) * limbs;
                    const double a = (h01(q >> 9) - 0.5) * (v == 1 ? TAU : 1.0);
                    if (v == 1) {   // from under the dome, lobes out past its rim
                        Look pl = look; pl.mark = MK_SMOOTH; pl.density = 0.1f; pl.bank = SB_FIELD; pl.stain = 0.05f;
                        const double lr = R * (0.16 + 0.05 * h01(q + 3)), lc = R + reach * 0.5 - lr;
                        Body& lb = B.radial(std::cos(a) * lc, std::sin(a) * lc, a, lr, std::max(1.0, (reach * 0.5 + lr) / lr * 0.9), pl);
                        lb.head = 0.15f;
                        fillOutline(lb, 0, 2, 2, 2, 0.06, q, t, 0.3);
                        B.finish(lb);
                    } else if (m.pods == PD_THREADS) {
                        std::vector<P2> pts;
                        for (int i = 0; i <= 8; i++) pts.push_back({L * (1 - m.taper * 0.1) + std::cos(a) * reach * 1.6 * i / 8.0, std::sin(a) * reach * 1.6 * i / 8.0});
                        B.strand(pts, (float)(width * 0.015), (float)(width * 0.004), ST_THREAD, SB_FIELD, 1);
                    } else {
                        Look pl = look; pl.mark = MK_SMOOTH; pl.density = 0.12f; pl.bank = SB_FIELD;
                        B.tube({{L * 0.85, 0}, {L * 0.9 + std::cos(a) * reach, std::sin(a) * reach}}, {(float)(R * 0.18), (float)(R * (0.12 + 0.06 * h01(q >> 17)))}, pl);
                    }
                }
            }
            if (v == 1) B.dot(0, 0, R * 0.28, R * 0.28, 0, DOT_VACUOLE, SB_FIELD, 0.9f);   // the round aperture under the dome
            if (m.spines > 0) for (int k = 0; k < m.spines; k++) {
                const double phi = PI + (k - (m.spines - 1) * 0.5) * 0.5;
                P2 e, nr; edgeOf(bodyCopy, phi, e, nr);
                pushStrand(B.out, {{e.x, e.y}, {e.x + nr.x * m.spineLen * width, e.y + nr.y * m.spineLen * width}}, (float)(width * 0.015), (float)(width * 0.004), ST_SPINE, SB_MINERAL + m.mineral, 1, B.group);
            }
            break;
        case MP_LATTICE: {
            const int n = m.spines;
            for (int k = 0; k < n; k++) {
                const uint64_t q = hk(s, 55, k);
                double phi, reach;
                if (v == 1) {   // twenty from the centre, foreshortened by their tilt
                    const double z = -0.9 + 1.8 * ((k % 5) + 0.5) / 5.0, az = (k / 5) * PI * 0.5 + (k % 2) * PI * 0.25;
                    phi = az; reach = std::sqrt(std::max(0.05, 1 - z * z));
                } else { phi = v == 2 ? PI + (k - (n - 1) * 0.5) * 0.7 : k * TAU / n + h01(q) * 0.2; reach = 1; }
                if (v == 2 && k == n - 1) phi = 0;   // the apical horn
                P2 e, nr;
                if (v == 1) { e = B.at(0, 0); const double a = B.angle(phi); nr = {std::cos(a), std::sin(a)}; }
                else edgeOf(bodyCopy, phi, e, nr);
                const double len = m.spineLen * width * reach + (v == 1 ? R * reach : 0);
                const P2 tip = {e.x + nr.x * len, e.y + nr.y * len};
                pushStrand(B.out, {{e.x, e.y}, tip}, (float)(width * (v == 1 ? 0.03 : 0.022)), (float)(width * 0.006), ST_SPINE, SB_MINERAL + m.mineral, 1, B.group);
                for (int f = 0; f < m.spineBranch * 2; f++) {   // a forked tip
                    const double a = (f & 1 ? 1 : -1) * (0.4 + 0.2 * (f >> 1));
                    const double bx = nr.x * std::cos(a) - nr.y * std::sin(a), by = nr.x * std::sin(a) + nr.y * std::cos(a);
                    const double bl = len * 0.2;
                    pushStrand(B.out, {tip, {tip.x + bx * bl, tip.y + by * bl}}, (float)(width * 0.008), (float)(width * 0.004), ST_SPINE, SB_MINERAL + m.mineral, 1, B.group);
                }
            }
            if (limbs > 0)
                for (int k = 0; k < m.podCount; k++) {   // the axopods
                    const uint64_t q = hk(s, 56, k);
                    const double phi = h01(q) * TAU;
                    P2 e, nr; edgeOf(bodyCopy, phi, e, nr);
                    const double len = m.flagLen * width * (0.6 + 0.4 * h01(q >> 9)) * limbs;
                    pushStrand(B.out, {{e.x, e.y}, {e.x + nr.x * len, e.y + nr.y * len}}, (float)(width * 0.005), (float)(width * 0.003), ST_THREAD, SB_FIELD, 0.8f, B.group);
                }
            break;
        }
        case MP_ARMOURED: {
            // the girdle across it (a groove, a little helical), the sulcus behind; its flagellum round the girdle, the other trailing
            std::vector<P2> g1, g2;
            for (int i = 0; i <= 20; i++) {
                const double q = -1 + 2.0 * i / 20.0;
                g1.push_back(B.at(-R * 0.08 + q * 0.06 * R - R * 0.07, q * R * 0.98));
                g2.push_back(B.at(-R * 0.08 + q * 0.06 * R + R * 0.07, q * R * 0.98));
            }
            pushStrand(B.out, std::move(g1), (float)(width * 0.02), (float)(width * 0.02), ST_SPINE, SB_FIELD, 0.7f, B.group);
            pushStrand(B.out, std::move(g2), (float)(width * 0.02), (float)(width * 0.02), ST_SPINE, SB_FIELD, 0.7f, B.group);
            B.strand({{-R * 0.1, -R * 0.05}, {-L * 0.75, -R * 0.12}}, (float)(width * 0.02), (float)(width * 0.015), ST_SPINE, SB_FIELD, 0.5f);
            if (limbs > 0) {
                std::vector<P2> wv;
                for (int i = 0; i <= 40; i++) { const double q = -1 + 2.0 * i / 40.0; wv.push_back({-R * 0.08 + q * 0.06 * R + R * 0.04 * std::sin(q * 18 - P.beat * 2), q * R * 1.02}); }
                B.strand(wv, (float)(width * 0.012), (float)(width * 0.012), ST_FLAGELLUM, SB_FIELD, 1);
                flagellum(B, -R * 0.4, -R * 0.08, PI, m.flagLen * 2 * L * limbs, width * 0.014, P.beat, 0.14, m.waveLen, ST_FLAGELLUM, 1);
            }
            // horns: Ceratium's hollow ones, the others' small spines
            if (v == 1) {
                Look hl = body; hl.layout = 0;
                const int n = std::max(2, m.spines);
                for (int k = 0; k < n; k++) {
                    const double dir = k == 0 ? 0 : PI + (k - 1 - (n - 2) * 0.5) * 0.55;
                    const double len = m.spineLen * width * (k == 0 ? 1.3 : (k == n - 1 ? 0.6 : 1.0));
                    std::vector<P2> pts; std::vector<float> rr;
                    for (int i = 0; i <= 8; i++) {
                        const double q = i / 8.0, curve = k == 0 ? 0 : (dir > PI ? 1 : -1) * q * q * 0.25;
                        const double a = dir + curve;
                        pts.push_back({std::cos(dir) * R * 0.7 + std::cos(a) * len * q, std::sin(dir) * R * 0.7 + std::sin(a) * len * q});
                        rr.push_back((float)(R * (0.32 - 0.26 * q)));
                    }
                    B.tube(pts, rr, hl);
                }
            } else if (m.spines > 0) {
                for (int k = 0; k < m.spines; k++) {
                    const double phi = k == 0 ? 0 : PI + (k - 1) * 0.6 - 0.3;
                    P2 e, nr; edgeOf(bodyCopy, phi, e, nr);
                    pushStrand(B.out, {{e.x, e.y}, {e.x + nr.x * m.spineLen * width, e.y + nr.y * m.spineLen * width}}, (float)(width * 0.03), (float)(width * 0.005), ST_SPINE, SB_FIELD, 1, B.group);
                }
            }
            if (v == 2) {   // the lists: a collar over the girdle, a sail down one side
                Look wl = body; wl.density = 0.03f; wl.wallDensity = 0.25f; wl.mark = MK_RIBS; wl.marks = 14; wl.layout = 1; wl.bank = SB_FIELD; wl.zScale = 0.05f;
                Body& c1 = B.radial(R * 0.35, 0, 0, R * 0.75, 0.45, wl);
                fillOutline(c1, 0, 2, 2, 2, 0.01, s ^ 0xC01, t, 0);
                B.finish(c1);
                Body& c2 = B.radial(-R * 0.2, R * 0.75, -0.3, R * 0.85, 1.3, wl);
                fillOutline(c2, 0, 2, 2, 2, 0.02, s ^ 0xC02, t, 0);
                B.finish(c2);
            }
            break;
        }
        default: break;
    }
}

// ---- the chambered: chambers on a logarithmic spiral (in a line, or a trochospire), sutures, pores, the reticulopodia from its aperture
void chambered(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const int v = m.variant;
    const uint64_t s = m.seed;
    const double Rt = m.widthUm * 0.5 * C.scale;
    Look look = cellLook(m, Rt);
    look.ghost = (float)C.ghost;
    look.bank = m.mineral == MI_IRON ? SB_MINERAL + MI_IRON : SB_MINERAL + (m.mineral == MI_ORGANIC ? MI_CALCITE : m.mineral);
    look.density = 0.35f; look.wallDensity = 0.6f; look.refract = 0.15f; look.stain = 0.02f;
    if (C.form == CF_FOSSIL) look.fossil = true;
    const int perWhorl = v == 2 ? 4 : 7 + (int)(h01(s >> 5) * 4);
    const int n = v == 1 ? 5 + (int)(h01(s >> 7) * 6) : std::max(5, (int)(m.turns * perWhorl));
    const double g = v == 2 ? 1.45 : std::pow(1.0 + (m.amp - 1.0) * 6.0, 1.0 / perWhorl);
    // the spiral's scale: its last chamber reaches the form's radius
    std::vector<double> rad(n), ang(n), dist(n);
    double total = 0;
    for (int k = 0; k < n; k++) {
        rad[k] = std::pow(g, k);
        ang[k] = v == 1 ? 0 : k * TAU / perWhorl;
        dist[k] = v == 1 ? 0 : rad[k] * (v == 2 ? 1.25 : 1.6);
    }
    if (v == 1) { double u = 0; for (int k = 0; k < n; k++) { u += k ? (rad[k - 1] + rad[k]) * 0.85 : 0; dist[k] = u; } total = dist[n - 1] + rad[n - 1]; }
    else total = dist[n - 1] + rad[n - 1];
    const double scale = (v == 1 ? 2 * Rt : Rt) / std::max(1e-9, total);
    double lastX = 0, lastY = 0, lastA = 0, lastR = 0;
    const double off = v == 1 ? -Rt : 0;
    if (v == 0) {   // the planispiral: one coiled shell, its periphery lobed by its chambers, the sutures curving back, its inner whorl's line
        const double G = std::pow(g, perWhorl), b = std::log(G) / TAU, hand = m.mirrored ? -1 : 1;
        const double th0 = (n - 1) * TAU / perWhorl;   // the last chamber's angle
        Look sh = look;
        sh.mark = look.mark == MK_SMOOTH ? MK_SMOOTH : MK_PORES; sh.marks = std::max(30, look.marks); sh.zScale = 0.45f; sh.wall = (float)(Rt * 0.035);
        Body& bd = B.radial(0, 0, 0, Rt, 1, sh);
        for (int i = 0; i < RTAB; i++) {
            const double phi = i * TAU / RTAB;
            double back = std::fmod(hand * (th0 - phi) + 8 * TAU, TAU);             // how far behind the last chamber's face
            const double rho = std::exp(-b * back);
            const double ch = std::fmod(back * perWhorl / TAU + 0.5, 1.0);          // where within its chamber
            bd.r[i] = (float)(rho * (0.93 + 0.07 * std::sin(PI * ch)));
        }
        B.finish(bd);
        const double inner = std::exp(-b * TAU);
        if (C.ghost < 0.9) {
            for (int k = 0; k < perWhorl; k++) {   // the sutures between the last whorl's chambers
                const double back = (k + 0.5) * TAU / perWhorl;
                std::vector<P2> pts;
                for (int i = 0; i <= 8; i++) {
                    const double f = i / 8.0, bb = back - 0.25 * f * f * TAU / perWhorl;
                    const double rr = Rt * std::exp(-b * bb) * (inner + (0.93 - inner) * f), a = hand * (th0 - bb);
                    pts.push_back({std::cos(a) * rr, std::sin(a) * rr});
                }
                B.strand(pts, (float)(Rt * 0.02), (float)(Rt * 0.012), ST_SPINE, look.bank, 0.6f);
            }
            std::vector<P2> sp;   // the inner whorl's line, and the whorls inside it where it is evolute
            const double turns = 1 + (h01(s + 11) < 0.5 ? 1.5 : 0.2);
            for (int i = 0; i <= 80; i++) {
                const double bb = TAU * turns * i / 80.0, rr = Rt * inner * std::exp(-b * bb), a = hand * (th0 - bb);
                sp.push_back({std::cos(a) * rr, std::sin(a) * rr});
            }
            B.strand(sp, (float)(Rt * 0.018), (float)(Rt * 0.01), ST_SPINE, look.bank, 0.6f);
            B.dot(0, 0, Rt * 0.07, Rt * 0.07, 0, DOT_GRAIN, look.bank, 0.5f);   // the umbilical boss
        }
        const double a = hand * th0;
        lastX = std::cos(a) * Rt * 0.7; lastY = std::sin(a) * Rt * 0.7; lastA = a + hand * PI * 0.5; lastR = Rt * 0.3;
    } else
    for (int k = 0; k < n; k++) {
        const double a = ang[k] * (m.mirrored ? -1 : 1);
        const double cx = v == 1 ? off + dist[k] * scale : std::cos(a) * dist[k] * scale, cy = v == 1 ? 0.04 * std::sin(k * 0.7) * Rt : std::sin(a) * dist[k] * scale;
        const double r = rad[k] * scale;
        Look lk = look; lk.density = (float)(look.density * (0.6 + 0.6 * (1 - (double)k / n)));
        lk.wall = (float)std::max(0.02, r * 0.1);
        Body& b = B.radial(cx, cy, v == 1 ? 0 : a + PI * 0.5, r, v == 1 ? 1.2 : (v == 2 ? 1.0 : 1.25), lk);
        fillOutline(b, 0, 2, 2, 2, 0.01, s ^ (uint64_t)k, B.t, 0);
        B.finish(b);
        lastX = cx; lastY = cy; lastA = v == 1 ? 0 : a + PI * 0.5; lastR = r;
    }
    if (C.form != CF_FOSSIL && C.ghost < 0.6) {
        // the spines of the spinose
        for (int k = 0; k < m.spines * 3; k++) {
            const uint64_t q = hk(s, 61, k);
            const double a = h01(q) * TAU, r0 = Rt * (0.4 + 0.5 * h01(q >> 9));
            const double len = m.spineLen * 2 * Rt * (0.6 + 0.4 * h01(q >> 17));
            B.strand({{std::cos(a) * r0, std::sin(a) * r0}, {std::cos(a) * (r0 + len), std::sin(a) * (r0 + len)}}, (float)(Rt * 0.012), (float)(Rt * 0.004), ST_SPINE, SB_MINERAL + MI_CALCITE, 0.9f);
        }
        // the reticulopodia: a net from its aperture, granules streaming on it
        const double reach = m.flagLen * 2 * Rt * C.limbs;
        if (reach > 0) {
            const double ax = lastX + std::cos(lastA) * lastR, ay = lastY + std::sin(lastA) * lastR;
            for (int k = 0; k < m.podCount; k++) {
                const uint64_t q = hk(s, 62, k);
                const double a = lastA + (h01(q) - 0.5) * 2.4;
                std::vector<P2> pts;
                double x = ax, y = ay, d = a;
                for (int i = 0; i < 12; i++) { pts.push_back({x, y}); d += (h01(q >> (i + 3)) - 0.5) * 0.5; x += std::cos(d) * reach / 12; y += std::sin(d) * reach / 12; }
                B.strand(pts, (float)(Rt * 0.008), (float)(Rt * 0.004), ST_NET, SB_FIELD, 0.8f);
                for (int gr = 0; gr < 3; gr++) {
                    const double f = std::fmod(h01(q >> (40 + gr)) + B.t * 0.015, 1.0) * 11;
                    const int i = (int)f;
                    const P2 pp = pts[std::min(11, i)];
                    B.dot(pp.x, pp.y, Rt * 0.012, Rt * 0.012, 0, DOT_GRANULE, SB_FIELD, 0.9f);
                }
            }
        }
    }
}

// ---- the colonies
void roller(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const int v = m.variant;
    const uint64_t s = m.seed;
    const double R = m.widthUm * 0.5 * C.scale;
    Look matrix = cellLook(m, R);
    matrix.bank = SB_FIELD; matrix.density = 0.02f; matrix.phase = 0.12f; matrix.wall = (float)(R * 0.01); matrix.wallDensity = 0.15f; matrix.stain = 0; matrix.red = 0;
    matrix.ghost = (float)C.ghost; matrix.mark = MK_SMOOTH; matrix.gapPhi = 0; matrix.gapW = (float)(C.gape * 0.6); matrix.gel = true;
    if (C.form == CF_FOSSIL) matrix.fossil = true;
    Body& b = B.radial(0, 0, 0, R, m.aspect, matrix);
    fillOutline(b, 0, 2, 2, 2, 0.005, s, B.t, 0);
    B.finish(b);
    // its rolling: the cells' sphere turned about an axis tilted to the eye
    const double tiltA = 0.5 + 0.4 * h01(s >> 3), roll = P.roll;
    const double ct = std::cos(tiltA), st = std::sin(tiltA), cr = std::cos(roll), sr = std::sin(roll);
    auto turn = [&](double x, double y, double z, double& X, double& Y, double& Z) {
        const double x1 = x * cr - y * sr, y1 = x * sr + y * cr;   // about the colony's axis
        X = x1; Y = y1 * ct - z * st; Z = y1 * st + z * ct;       // the axis tilted
    };
    const int n = v == 0 ? m.groupN : m.groupN;
    const double cellR = v == 0 ? R * std::max(0.012, 1.4 / std::sqrt((double)n)) : R * std::min(0.42, 1.5 / std::sqrt((double)n));
    const int pbank = pigmentBank(m.pigment);
    if (C.ghost < 0.8) {
        std::vector<std::pair<double, int>> order;
        for (int k = 0; k < n; k++) {
            const double zz = 1 - 2 * (k + 0.5) / n, rr = std::sqrt(std::max(0.0, 1 - zz * zz)), a = k * 2.39996323;
            double X, Y, Z; turn(rr * std::cos(a), rr * std::sin(a), zz, X, Y, Z);
            order.push_back({Z, k});
        }
        std::sort(order.begin(), order.end());
        for (const auto& o : order) {
            const int k = o.second;
            const double zz = 1 - 2 * (k + 0.5) / n, rr = std::sqrt(std::max(0.0, 1 - zz * zz)), a = k * 2.39996323;
            double X, Y, Z; turn(rr * std::cos(a), rr * std::sin(a), zz, X, Y, Z);
            const double ux = X * R * 0.97, vy = Y * R * 0.97;
            if (v == 0) {
                B.dot(ux, vy, cellR, cellR, 0, DOT_PLASTID, pbank, (float)(Z > 0 ? 1.0 : 0.45));
                B.out.dots.back().lift = (float)(R + Z * R * 0.97);
                if (std::fabs(Z) < 0.18 && C.limbs > 0) {   // the two flagella of a cell at its rim
                    const double na = std::atan2(vy, ux);
                    for (int f = 0; f < 2; f++) flagellum(B, ux, vy, na + (f ? 0.35 : -0.35), cellR * 5, cellR * 0.25, P.beat + k, 0.15, 0.8, ST_FLAGELLUM, 0.8f);
                }
            } else {
                Look cl = cellLook(m, cellR);
                cl.ghost = (float)C.ghost;
                const double sq = 1 - 0.3 * (1 - std::fabs(Z));
                Body& cb = B.radial(ux, vy, std::atan2(vy, ux), cellR, 1.0 / std::max(0.4, sq), cl);
                fillOutline(cb, 0, 2, 2, 2, 0.04, s ^ (uint64_t)k, B.t, 0.1);
                cb.lift = (float)(R + Z * R * 0.97 - cellR);
                B.finish(cb);
                if (Z > -0.2) insides(B, frameOf(B, cb, cellR, cellR), C, k);
                if (std::fabs(Z) < 0.35 && C.limbs > 0) {
                    const double na = std::atan2(vy, ux);
                    for (int f = 0; f < 2; f++) flagellum(B, ux + std::cos(na) * cellR, vy + std::sin(na) * cellR, na + (f ? 0.3 : -0.3), cellR * 3.2, cellR * 0.08, P.beat + k, 0.14, 0.8, ST_FLAGELLUM, 1);
                }
            }
        }
    }
    // the daughters inside, growing; let out through the gap
    if (v == 0) {
        const int nd = 4 + (int)(h01(s >> 9) * 8);
        for (int k = 0; k < nd; k++) {
            const uint64_t q = hk(s, 71, k);
            const double a = h01(q) * TAU, d = R * 0.5 * std::sqrt(h01(q >> 9));
            double X, Y, Z; turn(std::cos(a) * d / R, std::sin(a) * d / R, (h01(q >> 17) - 0.5) * 0.8, X, Y, Z);
            const double out = C.gape > 0 ? C.gape * R * 1.6 * (0.6 + 0.4 * h01(q >> 23)) : 0;
            const double rd = R * (0.12 + 0.06 * h01(q >> 27)) * (0.6 + 0.6 * std::max(C.grow, 0.5));
            Look dl = cellLook(m, rd); dl.density = 0.45f; dl.mark = MK_NET; dl.marks = 16; dl.layout = 0; dl.markSeed = q;
            Body& db = B.radial(X * R + out, Y * R, roll, rd, 1, dl);
            fillOutline(db, 0, 2, 2, 2, 0.02, q, B.t, 0);
            B.finish(db);
        }
    }
}

void plate(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const int v = m.variant, n = std::max(4, m.groupN);
    const uint64_t s = m.seed;
    const double R = m.widthUm * 0.5 * C.scale;
    // the cells' places: Gonium's square of four and twelve, Pediastrum's rings, Merismopedia's fours in rows
    std::vector<Placed> at;
    double cellR = R * 0.2;
    if (v == 0) {
        cellR = R * (n <= 4 ? 0.36 : (n <= 8 ? 0.27 : 0.2));
        const int inner = std::min(n, 4);
        for (int k = 0; k < inner; k++) at.push_back({((k & 1) - 0.5) * cellR * 2.3, (((k >> 1) & 1) - 0.5) * cellR * 2.3, 0});
        for (int k = 0; k < n - inner; k++) { const double a = (k + 0.5) * TAU / (n - inner); at.push_back({std::cos(a) * cellR * 3.4, std::sin(a) * cellR * 3.4, a}); }
    } else if (v == 1) {
        int rings[6] = {1, 0, 0, 0, 0, 0}, left = n - 1, nr = 1;
        for (int r = 1; r < 6 && left > 0; r++) { const int c = std::min(left, (int)std::round(6.2 * r)); rings[r] = c; left -= c; nr = r + 1; }
        cellR = R / (nr - 0.4) * 0.52;
        for (int r = 0; r < nr; r++) for (int k = 0; k < rings[r]; k++) { const double a = (k + 0.5 * (r & 1)) * TAU / std::max(1, rings[r]); at.push_back({std::cos(a) * r * cellR * 1.95, std::sin(a) * r * cellR * 1.95, a + (r == nr - 1 ? 0 : 0.0)}); }
    } else {
        const int side = std::max(2, (int)std::round(std::sqrt((double)n)));
        cellR = R / (side * 1.25);
        for (int k = 0; k < side * side && (int)at.size() < n; k++) {
            const int i = k % side, j = k / side;
            const double gx = (i / 2) * 0.5, gy = (j / 2) * 0.5;
            at.push_back({(i - (side - 1) * 0.5 + gx - 0.25 * (side / 2 - 1)) * cellR * 2.15, (j - (side - 1) * 0.5 + gy - 0.25 * (side / 2 - 1)) * cellR * 2.15, 0});
        }
    }
    if (v != 1) {   // the jelly round them
        Look jelly = cellLook(m, R); jelly.bank = SB_FIELD; jelly.density = 0.02f; jelly.phase = 0.1f; jelly.wall = (float)(R * 0.01); jelly.wallDensity = 0.1f;
        jelly.stain = 0; jelly.red = 0; jelly.mark = MK_SMOOTH; jelly.ghost = (float)C.ghost; jelly.gel = true;
        Body& j = B.radial(0, 0, 0, R * 1.05, 1, jelly);
        fillOutline(j, 4, 6, 6, 6, 0.01, s, B.t, 0);
        B.finish(j);
    }
    const int rimFrom = v == 1 ? n - (int)std::round(6.2 * std::max(1, (int)std::floor(std::sqrt(n / 3.1)))) : n;
    for (int k = 0; k < (int)at.size(); k++) {
        Look cl = cellLook(m, cellR);
        cl.ghost = (float)C.ghost; if (C.form == CF_FOSSIL) cl.fossil = true;
        const bool rim = v == 1 && k >= std::max(1, rimFrom);
        std::vector<Lobe> lb;
        if (rim) { lb.push_back({0.5, 0.35, 0.25}); lb.push_back({-0.5, 0.35, 0.25}); lb.push_back({0, -0.25, 0.25}); }
        Body& cb = B.radial(at[k].u, at[k].v, at[k].a, cellR, v == 0 ? 1.15 : 1.0, cl);
        fillOutline(cb, v == 1 && !rim ? 6 : 0, 3, 8, 8, 0.03, s ^ (uint64_t)k, B.t, 0.05, lb.data(), (int)lb.size());
        B.finish(cb);
        const Body copy = cb;
        insides(B, frameOf(B, copy, cellR, cellR * 0.85), C, k);
        if (rim && m.spines > 0)
            for (int h = 0; h < 2; h++) {
                P2 e, nr; edgeOf(copy, h ? 0.5 : -0.5, e, nr);
                pushStrand(B.out, {{e.x, e.y}, {e.x + nr.x * m.spineLen * cellR * 4, e.y + nr.y * m.spineLen * cellR * 4}}, (float)(cellR * 0.12), (float)(cellR * 0.04), ST_SPINE, SB_FIELD, 1, B.group);
            }
        if (v == 0 && m.flagella > 0 && C.limbs > 0) {
            const double na = std::atan2(at[k].v, at[k].u + 1e-9);
            for (int f = 0; f < 2; f++) flagellum(B, at[k].u + std::cos(na) * cellR, at[k].v + std::sin(na) * cellR, na + (f ? 0.25 : -0.25), cellR * 4, cellR * 0.07, P.beat + k * 0.3, 0.14, 0.8, ST_FLAGELLUM, 1);
        }
    }
}

void chain(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const int v = m.variant, n = std::max(2, m.groupN);
    const uint64_t s = m.seed;
    const double R = m.widthUm * 0.5 * C.scale, L = R * m.aspect;
    if (v == 3) {   // beads coiled in a jelly
        Look jelly = cellLook(m, R); jelly.bank = SB_FIELD; jelly.density = 0.02f; jelly.phase = 0.1f; jelly.wallDensity = 0.08f; jelly.stain = 0; jelly.red = 0; jelly.mark = MK_SMOOTH;
        jelly.gel = true;
        Body& j = B.radial(0, 0, 0, R * (2.2 + n * 0.25), 1.2, jelly);
        fillOutline(j, 0, 2, 2, 2, 0.12, s, B.t, 0.02);
        B.finish(j);
    }
    double u = 0, vv = 0, a = 0;
    std::vector<Placed> at;
    for (int k = 0; k < n; k++) {
        if (v == 0) at.push_back({0, (k - (n - 1) * 0.5) * R * 2.05, 0});
        else if (v == 1) at.push_back({(k - (n - 1) * 0.5) * R * (m.aspect * 0.0 + 1.0) * 1.15 * 1.6, 0, 0});
        else if (v == 2) { at.push_back({u, vv, (k & 1) ? 1.0 : -1.0}); u += L * 1.6 * std::cos((k & 1) ? 0.55 : -0.55); vv += L * 1.6 * std::sin((k & 1) ? 0.55 : -0.55) * 0.2; }
        else { at.push_back({u, vv, a}); a += 0.45 + 0.2 * std::sin(k * 0.9); u += std::cos(a) * R * 2.0; vv += std::sin(a) * R * 2.0; }
    }
    if (v >= 2) { double cu = 0, cv = 0; for (const Placed& p : at) { cu += p.u; cv += p.v; } cu /= n; cv /= n; for (Placed& p : at) { p.u -= cu; p.v -= cv; } }
    for (int k = 0; k < n; k++) {
        Look cl = cellLook(m, R);
        cl.ghost = (float)C.ghost; if (C.form == CF_FOSSIL) cl.fossil = true;
        double asp = v == 0 ? m.aspect : (v == 1 ? 0.62 : (v == 2 ? m.aspect : 1.0));
        int sym = v == 1 || v == 2 ? 4 : 0;
        const bool hetero = v == 3 && k % 8 == 3;
        if (hetero) { cl.density *= 0.35f; cl.wall *= 2; cl.red = 0; }
        Body& cb = B.radial(at[k].u, at[k].v, at[k].a + (v == 1 ? PI * 0.5 : 0), R * (hetero ? 1.15 : 1), asp, cl);
        if (v == 0) { cb.head = cb.tail = (float)m.taper; }
        fillOutline(cb, sym, 6, 8, 8, 0.02, s ^ (uint64_t)k, B.t, 0.05);
        B.finish(cb);
        const Body copy = cb;
        insides(B, frameOf(B, copy, R * asp, R * 0.85), C, k);
        if (v == 0 && (k == 0 || k == n - 1) && m.spines > 0) {   // the end cells' spines
            for (int e = 0; e < std::min(4, m.spines); e++) {
                const double phi = (e & 1) ? 0.15 : PI - 0.15;
                P2 ep, nr; edgeOf(copy, phi, ep, nr);
                const double side = k == 0 ? -1 : 1, len = m.spineLen * L;
                std::vector<P2> pts;
                for (int i = 0; i <= 6; i++) { const double q = i / 6.0; pts.push_back({ep.x + (nr.x * 0.4 + 0 * side) * len * q + std::cos(B.angle(PI * 0.5)) * side * len * q * q, ep.y + nr.y * 0.4 * len * q + std::sin(B.angle(PI * 0.5)) * side * len * q * q}); }
                pushStrand(B.out, std::move(pts), (float)(R * 0.08), (float)(R * 0.03), ST_SPINE, SB_FIELD, 1, B.group);
            }
        }
        if (v == 1) {   // the setae from its corners
            for (int e = 0; e < 4; e++) {
                const double cx = at[k].u + ((e & 1) ? 1 : -1) * R * 0.55, cy = ((e & 2) ? 1 : -1) * R * 0.95;
                const double dir = ((e & 2) ? 1 : -1) * (PI * 0.5 - 0.35) + ((e & 1) ? 0.25 : -0.25) * 0.5;
                std::vector<P2> pts;
                const double len = m.spineLen * R * 2;
                for (int i = 0; i <= 10; i++) { const double q = len * i / 10.0; pts.push_back({cx + std::cos(dir) * q + ((e & 1) ? 1 : -1) * q * q / len * 0.3 * R, cy + std::sin(dir) * q}); }
                B.strand(pts, (float)(R * 0.035), (float)(R * 0.015), ST_SPINE, SB_MINERAL + m.mineral, 1);
            }
        }
    }
}

void branching(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const int v = m.variant;
    const uint64_t s = m.seed;
    const double R = m.widthUm * 0.5 * C.scale;
    Look look = cellLook(m, R);
    look.ghost = (float)C.ghost; if (C.form == CF_FOSSIL) look.fossil = true;
    const double grown = C.grow > 0 ? C.grow : 1.0;   // the act's growth (its tips extend)
    const double seg = v == 1 ? R * 2 * (3 + 2 * h01(s >> 5)) : R * 2 * 6;
    const double reachMax = std::max(seg, m.bodyUm() * C.scale - R * 1.2);   // its tips stop at the reach its form declares
    struct Tip { double u, v, a; int depth; uint64_t h; };
    std::vector<Tip> stack;
    for (int k = 0; k < m.groupN; k++) stack.push_back({0, 0, k * TAU / m.groupN + h01(hk(s, 81, k)) * 0.6, 0, hk(s, 82, k)});
    int made = 0;
    while (!stack.empty() && made < 90) {
        Tip tp = stack.back(); stack.pop_back();
        const int segs = v == 1 ? 3 + (int)(h01(tp.h) * 3) : 4 + (int)(h01(tp.h) * 5);
        const int shown = std::max(1, (int)std::ceil(segs * std::min(1.0, grown * (1.4 - 0.25 * tp.depth))));
        std::vector<P2> pts = {{tp.u, tp.v}};
        double u = tp.u, vv = tp.v, a = tp.a;
        for (int i = 0; i < shown; i++) {
            a += (h01(tp.h >> (i % 50)) - 0.5) * m.bend;
            if (std::sqrt((u + std::cos(a) * seg) * (u + std::cos(a) * seg) + (vv + std::sin(a) * seg) * (vv + std::sin(a) * seg)) > reachMax) break;
            u += std::cos(a) * seg; vv += std::sin(a) * seg;
            pts.push_back({u, vv});
            if (v == 1) {   // a cell a segment, its cross walls
                std::vector<float> rr = {(float)R, (float)R};
                Look cl = look;
                B.tube({pts[pts.size() - 2], pts.back()}, rr, cl);
                InFrame F; F.tube = true; F.line = {pts[pts.size() - 2], pts.back()}; F.rad = rr; F.R = R; F.L = seg * 0.5;
                insides(B, F, C, made * 10 + i);
            }
            if (tp.depth < 3 && i % 2 == 1 && h01(tp.h >> (i + 7)) < (v == 1 ? 0.45 : 0.6))
                stack.push_back({u, vv, a + ((i & 2) ? 1 : -1) * (0.6 + 0.4 * h01(tp.h >> (i + 11))), tp.depth + 1, mix64(tp.h + (uint64_t)i)});
        }
        if (v != 1) {
            std::vector<float> rr(pts.size(), (float)(R * (1 - 0.15 * tp.depth)));
            B.tube(pts, rr, look);
            if ((m.cycle & LC_FRUITING) && tp.depth >= 2 && h01(tp.h >> 3) < 0.6 && std::sqrt(u * u + vv * vv) < reachMax - R * 20) {   // a chain of spores at the tip, coiled
                for (int k = 0; k < 8; k++) {
                    const double ca = a + k * 0.55 * (m.mirrored ? -1 : 1);
                    const double su = u + std::cos(a) * R * 2 * (k + 1) * 0.9 + std::cos(ca) * R * 1.5, sv = vv + std::sin(a) * R * 2 * (k + 1) * 0.9 + std::sin(ca) * R * 1.5;
                    Look sl = look; sl.refract = 0.8f; sl.density = 0.4f;
                    Body& sb = B.radial(su, sv, a, R * 1.1, 1.2, sl);
                    fillOutline(sb, 0, 2, 2, 2, 0.02, tp.h ^ (uint64_t)k, B.t, 0);
                    B.finish(sb);
                }
            }
        }
        made++;
    }
}

// a diatoms' chain seen from the girdle: Melosira's boxes end to end (a centric's), Fragilaria's ribbon side by side (a pennate's); each box its
// two valves' rims and the girdle's bands, its plastids
void diatomChain(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const bool centric = m.variant == 0 || m.variant == 3;
    const int n = std::max(2, m.groupN);
    const double D = m.widthUm * C.scale;                                  // a valve's diameter (centric) or a box's height (pennate)
    const double len = centric ? D : m.lengthUm * C.scale;                 // across the chain
    const double h = centric ? D * (0.45 + 0.35 * h01(m.seed + 7)) : D * 0.55;   // a box's depth along the chain
    for (int k = 0; k < n; k++) {
        const double along = (k - (n - 1) * 0.5) * h * 1.04;
        Look lk = cellLook(m, h * 0.5);
        lk.ghost = (float)C.ghost; if (C.form == CF_FOSSIL) lk.fossil = true;
        lk.zScale = 0.6f; lk.refract = 0.2f; lk.wallDensity = 0.55f; lk.mark = MK_RIBS; lk.layout = 3; lk.marks = 6 + (int)(h01(m.seed + 9) * 6);
        Body& b = centric ? B.radial(along, 0, PI * 0.5, h * 0.5, len / h, lk) : B.radial(0, along, 0, h * 0.5, len / h, lk);
        fillOutline(b, 4, 6, 9, 9, 0.004, m.seed ^ (uint64_t)k, B.t, 0);
        if (!centric) { b.head = b.tail = (float)(m.taper * 0.5); }
        B.finish(b);
        const Body copy = b;
        insides(B, frameOf(B, copy, len * 0.5, h * 0.45), C, k);
    }
}

void gathering(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const uint64_t s = m.seed;
    const double R = m.widthUm * 0.5 * C.scale;
    if (C.form == CF_SLUG) {   // the slug: many cells in one sheath, its tip paler, the slime it leaves
        const double L = R * 60, W = R * 6;
        Look sl = cellLook(m, W); sl.density = 0.45f; sl.mark = MK_GRAINS; sl.marks = 60; sl.layout = 0; sl.stain = 0.4f;
        Look trail = sl; trail.density = 0.03f; trail.mark = MK_SMOOTH; trail.phase = 0.15f; trail.stain = 0; trail.gel = true;
        std::vector<P2> pts; std::vector<float> rr, tr;
        for (int i = 0; i <= 12; i++) { const double q = i / 12.0; pts.push_back({-L * 0.5 + L * q, W * 0.6 * std::sin(q * 3 + B.t * 0.05)}); rr.push_back((float)(W * (0.55 + 0.45 * std::sin(PI * std::min(1.0, q * 1.1 + 0.05))))); tr.push_back((float)(W * 0.7)); }
        std::vector<P2> back; std::vector<float> br;
        for (int i = 0; i <= 6; i++) { back.push_back({-L * 0.5 - L * 0.6 * i / 6.0, W * 0.6 * std::sin(-i * 0.3)}); br.push_back((float)(W * 0.7)); }
        B.tube(back, br, trail);
        B.tube(pts, rr, sl);
        Look tip = sl; tip.density = 0.25f;
        B.tube({pts[10], pts[12]}, {rr[10] * 0.9f, rr[12] * 0.8f}, tip);
        return;
    }
    if (C.form == CF_STALK) {   // the fruiting body from the side: its stalk, its basal disc, its head of spores (whorls of small ones)
        const double H = R * 120, W = R * 3;
        Look st = cellLook(m, W); st.bank = SB_FIELD; st.density = 0.12f; st.mark = MK_NET; st.marks = 30; st.layout = 3; st.stain = 0.05f; st.refract = 0.1f;
        std::vector<P2> pts; std::vector<float> rr;
        for (int i = 0; i <= 10; i++) { const double q = i / 10.0; pts.push_back({0, H * 0.5 - H * q}); rr.push_back((float)(W * (1.0 - 0.6 * q))); }
        B.tube(pts, rr, st);
        Look disc = st; disc.density = 0.2f;
        Body& d = B.radial(0, H * 0.5, 0, W * 3, 0.35, disc);
        fillOutline(d, 0, 2, 2, 2, 0.05, s, B.t, 0);
        B.finish(d);
        Look sor = cellLook(m, W * 6); sor.density = 0.6f; sor.refract = 0.4f; sor.mark = MK_GRAINS; sor.marks = 40;
        Body& so = B.radial(0, -H * 0.5 - W * 4, 0, W * 6, 1, sor);
        fillOutline(so, 0, 2, 2, 2, 0.04, s ^ 0x50, B.t, 0);
        B.finish(so);
        if (m.variant == 1)
            for (int w = 0; w < 4; w++) for (int k = 0; k < 2; k++) {
                const double y = -H * 0.35 + w * H * 0.22, dir = k ? 1 : -1;
                B.strand({{0, y}, {dir * W * 8, y - W * 4}}, (float)(W * 0.25), (float)(W * 0.15), ST_STALK, SB_FIELD, 1);
                Body& sm = B.radial(dir * W * 9, y - W * 5, 0, W * 2, 1, sor);
                fillOutline(sm, 0, 2, 2, 2, 0.04, s ^ (uint64_t)(w * 2 + k), B.t, 0);
                B.finish(sm);
            }
        return;
    }
    largeCell(B, P, C);
}
}   // namespace

void buildCell(Build& B, const Pose& P, const CellCtx& C) {
    const Morph& m = B.m;
    const int p = m.plan;
    if (p == MP_FILAMENT) { filament(B, P, C); return; }
    if (smallCellPlan(p) || protocellPlan(p)) {
        std::vector<Placed> at;
        if (C.single || C.pinch > 0 || C.form == CF_DIVIDING || C.round > 0.5) at = {{0, 0, 0}};
        else arrangement(m, B.sp.seed ^ m.seed, at);
        const double X = B.x, Y = B.y, ca = B.ca, sa = B.sa;
        const bool mir = B.mirror;
        for (int k = 0; k < (int)at.size(); k++) {
            const double u = at[k].u * C.scale, v = at[k].v * C.scale;
            const P2 c = B.at(u, v);
            B.place(c.x, c.y, std::atan2(sa, ca) + (mir ? -at[k].a : at[k].a), mir);
            B.group = k;
            if (protocellPlan(p)) protocell(B, P, C); else smallCell(B, P, C, k);
            B.x = X; B.y = Y; B.ca = ca; B.sa = sa;
        }
        B.group = 0;
        return;
    }
    if (p == MP_SHELLED && m.arrangement == AR_CHAINS && C.form != CF_DIVIDING && C.pinch == 0) { diatomChain(B, P, C); return; }
    switch (p) {
        case MP_CHAMBERED: chambered(B, P, C); break;
        case MP_ROLLER: roller(B, P, C); break;
        case MP_PLATE: plate(B, P, C); break;
        case MP_CHAIN: chain(B, P, C); break;
        case MP_BRANCHING: branching(B, P, C); break;
        case MP_GATHERING: gathering(B, P, C); break;
        default: largeCell(B, P, C); break;
    }
}

}   // namespace morph
