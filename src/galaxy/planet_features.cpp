// E-07: the planet function's features: the fields on the features' lattice (craters, domes, volcanoes, lava flows), the
// plates, the dunes, the relief spectrum, the cliff bands and the plains' relief; the finders the surface, the scanner and
// the harness ask for what stands near a point (vents, springs, the great shield, the landforms' features, the tiger
// stripes, the great cliff, a giant's storm cells, the ember fields)
#include "planet_internal.h"

namespace {

// the features of the same jittered grid whose points lie in a box round `pc` (`halfKm` each way): what `visitFeatures` finds near
// any point of it, so the scanner lists the cones, the hills and the craters the landforms build (`landformFeatures`); `hit.d` is from `pc`
template <class F>
void featuresInBox(const Vec3& pc, double halfKm, double R, double cellKm, double density, uint64_t seed, F fn) {
    const double inv = 1.0 / cellKm;
    const int64_t x0 = (int64_t)std::floor((pc.x - halfKm) * inv), x1 = (int64_t)std::floor((pc.x + halfKm) * inv);
    const int64_t y0 = (int64_t)std::floor((pc.y - halfKm) * inv), y1 = (int64_t)std::floor((pc.y + halfKm) * inv);
    const int64_t z0 = (int64_t)std::floor((pc.z - halfKm) * inv), z1 = (int64_t)std::floor((pc.z + halfKm) * inv);
    for (int64_t cz = z0; cz <= z1; cz++)
        for (int64_t cy = y0; cy <= y1; cy++)
            for (int64_t cx = x0; cx <= x1; cx++) {
                const uint64_t h = hash3i(cx, cy, cz, seed);
                if (unitFromHash(h) > density) continue;
                const double u0 = unitFromHash(mix64(h + 1)), u1 = unitFromHash(mix64(h + 2)), u2 = unitFromHash(mix64(h + 3));
                Vec3 c((cx + u0) * cellKm, (cy + u1) * cellKm, (cz + u2) * cellKm);
                const double cl = length(c);
                if (std::fabs(cl - R) > cellKm * 0.5) continue;
                c = c * (R / cl);
                FeatureHit hit;
                hit.d = length(pc - c); hit.cellKm = cellKm; hit.c = c;
                for (int k = 0; k < 6; k++) hit.u[k] = unitFromHash(mix64(h + 10 + k));
                fn(hit);
            }
}
// D-04 (TR_EMBERS): the ember fields of a young interior (`emberLakes`): 45% of the cells of a 1,300 km grid (a few dozen a world), each 80-250 km
// across, its strength (how many lakes, how bright from orbit) 0.55-1
inline PointGrid emberGrid(const BodyGen& g) { return {1300.0, 0.45, g.sE + 520}; }
inline double emberFieldKm(const FeatureHit& f) { return 40.0 + 85.0 * f.u[3]; }
}   // namespace
double craterField(const Vec3& p, double R, double cellKm, double density, double depthScale, uint64_t seed, CraterMarks& m, double detailM) {
    double h = 0;
    visitFeatures(p, R, cellKm, density, seed, 0.8, [&](const FeatureHit& f) {
        double r = f.cellKm * (0.14 + 0.30 * f.u[3]);
        double x = f.d / r;
        double age = f.u[5];
        bool big = r > 8.0;
        // rays: young craters throw bright streaks out to seven radii
        if (x > 1.2 && x < 7.0 && age < 0.22 && r > 1.5) {
            Vec3 cn = normalize(f.c);
            Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
            Vec3 d = normalize(p - f.c);
            double ang = std::atan2(dot(d, e2), dot(d, e1));
            int nRays = 6 + (int)(f.u[1] * 8);
            double ray = std::pow(0.5 + 0.5 * std::cos(ang * nRays + f.u[0] * TAU), 22.0);
            m.rays += 0.45 * ray * std::exp(-(x - 1.2) / 2.2) * (1 - age / 0.22);
        }
        if (x > 1.8) { if (big && x < 3.0) m.secondary += 0.5 * (1 - (x - 1.8) / 1.2); return; }
        if (big && x > 1.2) m.secondary += 0.5;
        double dep = r * 0.20 * depthScale * (0.4 + 0.6 * f.u[4]);
        double bowl = x < 1 ? -(1 - x * x) : 0;
        bool flooded = big && age > 0.7 && r > 25.0;
        if (big && x > 0.55 && x < 1 && !flooded) {   // O6-05: the inner wall of a big crater slumps in three terraces (seen once a terrace spans a few samples)
            double tf = lodFade(r * 0.15, detailM);
            if (tf > 0) {
                double t = (x - 0.55) / 0.45, st = t * 3, k = std::floor(st), fr = st - k;
                double tq = (k + smoothstep(0.42, 0.58, fr)) / 3;   // the risers over a sixth of a terrace's run: scarps over 1:1 (the basin walls are the crater-field types' cliffs)
                double xq = 0.55 + 0.45 * tq;
                bowl += (-(1 - xq * xq) - bowl) * tf * 0.8;
            }
        }
        if (flooded && bowl < -0.35) bowl = -0.35;                     // lava filled the old basin
        double rim = 0.35 * std::exp(-((x - 1) * (x - 1)) / (0.14 * 0.14));
        double ejecta = x > 1 ? 0.10 * std::exp(-(x - 1) / 0.35) : 0;
        if (big && x > 1 && x < 1.8 && scaleVisible(r * 0.05, detailM)) ejecta *= 1 + 0.6 * gnoise3(p / (r * 0.08), seed + 0xE3);   // the ejecta blanket is rough ground
        double peak = (r > 6 && f.u[2] > 0.5 && !flooded) ? 0.30 * std::exp(-(x * x) / (0.10 * 0.10)) : 0;
        double ring = (r > 60.0 && f.u[1] > 0.55) ? 0.14 * std::exp(-((x - 1.6) * (x - 1.6)) / (0.09 * 0.09)) : 0;   // multi-ring basin
        double erosion = 0.35 + 0.65 * (1 - age);
        h += dep * (bowl + rim + ejecta + peak + ring) * erosion * 1000.0;
        if (x > 0.8 && x < 1.2) m.rimBright += 0.5 * (1 - age) * (1 - std::fabs(x - 1) / 0.2);
        if (x < 0.6 && age < 0.08 && r > 1.0) m.young = std::max(m.young, (1 - x / 0.6) * (1 - age / 0.08 * 0.5));
        if (x < 0.8) { m.floorDark += 0.25 * (1 - age); if (flooded) m.flooded += 1.0 - smoothstep(0.6, 0.8, x); }
    });
    return h;
}

// Round hills / domes / boulders: radius fraction of cell, height in metres.
double domeField(const Vec3& p, double R, double cellKm, double density, uint64_t seed, double radiusFrac,
                 double heightM, double flatTop) {
    double h = 0;
    visitFeatures(p, R, cellKm, density, seed, 0.6, [&](const FeatureHit& f) {
        double r = f.cellKm * radiusFrac * (0.5 + 0.5 * f.u[3]);
        double x = f.d / r;
        if (x >= 1) return;
        double prof = std::cos(x * PI * 0.5);
        prof = prof * prof;
        if (flatTop > 0 && prof > flatTop) prof = flatTop;
        h += prof * heightM * (0.5 + 0.5 * f.u[4]);
    });
    return h;
}

// Volcano cones with calderas; returns height and sets lava flag when inside caldera.
double volcanoField(const Vec3& p, double R, double cellKm, double density, uint64_t seed, double coneHeightM,
                    bool& inCaldera) {
    double h = 0;
    visitFeatures(p, R, cellKm, density, seed, 0.5, [&](const FeatureHit& f) {
        double r = f.cellKm * (0.12 + 0.18 * f.u[3]);
        double x = f.d / r;
        if (x >= 1) return;
        double cone = std::pow(1 - x, 1.4);
        double cal = 0.14 + 0.08 * f.u[4];
        double hh = cone;
        if (x < cal) { hh = std::pow(1 - cal, 1.4) - (cal - x) / cal * 0.35; if (x < cal * 0.75) inCaldera = true; }
        h += hh * coneHeightM * (0.6 + 0.4 * f.u[5]);
    });
    return h;
}
LavaOut lavaFlows(const Vec3& p, double R, double cellKm, double density, uint64_t seed, double detailM) {
    LavaOut o;
    if (!scaleVisible(cellKm, detailM)) return o;
    visitFeatures(p, R, cellKm, density, seed, 1.6, [&](const FeatureHit& f) {
        double r = f.cellKm * (0.12 + 0.18 * f.u[3]);
        double x = f.d / r;
        if (x < 0.95 || x > 9.0) return;
        Vec3 cn = normalize(f.c);
        Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
        Vec3 d = normalize(p - f.c);
        double ang = std::atan2(dot(d, e2), dot(d, e1));
        int nT = 2 + (int)(f.u[1] * 3);
        uint64_t hc = hash3i((int64_t)std::llround(f.c.x * 8), (int64_t)std::llround(f.c.y * 8), (int64_t)std::llround(f.c.z * 8), seed ^ 0x1A7A);
        for (int k = 0; k < nT; k++) {
            uint64_t hk = mix64(hc + 3 * k + 1);
            double a0 = unitFromHash(hk) * TAU, len = 3.0 + 5.0 * unitFromHash(mix64(hk + 1)), thick = 4.0 + 6.0 * unitFromHash(mix64(hk + 2));
            double da = wrapAngle(ang - a0);
            double wAng = 0.075 * (1.0 - 0.6 * (x - 1) / len);   // the tongue's half-width in radians of azimuth at this distance
            if (x > 1 + len || std::fabs(da) > wAng * 1.6) continue;
            double y = da / wAng;
            double shape = std::max(0.0, 1 - std::pow(std::fabs(y), 4.0));
            double tip = smoothstep(1 + len, 1 + len - 0.8, x);
            double widthKm = 2 * wAng * x * r;   // the tongue's width here
            // B-404: the relief by the tongue's own width (it faded by `lodFade` of the foot's: half there in the 64 m ring, gone at 512)
            o.dh += thick * shape * tip * featFade(widthKm, detailM);
            if (matVisible(widthKm, detailM)) o.flow = std::max(o.flow, shape * tip);   // the material by the unfaded tongue wherever it spans half a cell
        }
        {   // the pit chain
            uint64_t hk = mix64(hc + 77);
            double a0 = unitFromHash(hk) * TAU, len = 1.5 + 2.5 * unitFromHash(mix64(hk + 1));
            double da = wrapAngle(ang - a0);
            if (x < 1 + len && std::fabs(da) < 0.05) {
                double along = (x - 1) / len;   // 0..1 along the chain
                int n = 4 + (int)(unitFromHash(mix64(hk + 2)) * 4);
                double s = along * n, q = std::floor(s), fr = s - q;
                double pr = unitFromHash(mix64(hk + 10 + (uint64_t)q)) < 0.75 ? 1.0 : 0.0;
                double dx = (fr - 0.5) * 2, dy = da / 0.05;
                double rr = dx * dx + dy * dy;
                double pitKm = 0.1 * x * r;   // a pit's width (B-404: its own fade, the material wherever it spans half a cell)
                if (rr < 1) {
                    double bowl = (1 - rr) * (1 - rr) * pr;
                    o.dh -= (6.0 + 6.0 * unitFromHash(mix64(hk + 3))) * bowl * featFade(pitKm, detailM);
                    if (matVisible(pitKm, detailM)) o.pit = std::max(o.pit, bowl);
                }
            }
        }
    });
    return o;
}
Tectonics tectonicsAt(const Vec3& unit, const BodyGen& g) {
    Tectonics t;
    Worley3 w = worley3(unit * g.plateK + Vec3(7.1, 3.3, 9.7), g.sA + 77);
    double cellKm = g.R / g.plateK;
    t.bdKm = (w.f2 - w.f1) * cellKm;
    bool contA = unitFromHash(mix64(w.id1 + 5)) < 0.45, contB = unitFromHash(mix64(w.id2 + 5)) < 0.45;
    uint64_t lo = std::min(w.id1, w.id2), hi = std::max(w.id1, w.id2);
    double kind = unitFromHash(hash2i((int64_t)(lo & 0xffffff), (int64_t)(hi & 0xffffff), g.sA + 9));
    double interior = smoothstep(0.0, 300.0, t.bdKm);
    t.uplift = (contA ? 0.32 : -0.28) * (0.4 + 0.6 * interior);
    double band = smoothstep(320.0, 0.0, t.bdKm);
    if (kind < 0.45) {   // convergent
        if (contA && contB) t.chain = band;
        else if (contA != contB) t.chain = band * (contA ? 1.0 : 0.5);
        else t.arc = smoothstep(140.0, 0.0, std::fabs(t.bdKm - 90.0));
    } else if (kind < 0.75) {   // divergent
        if (contA && contB) t.rift = smoothstep(160.0, 0.0, t.bdKm);
        else t.ridge = smoothstep(220.0, 0.0, t.bdKm);
    } else t.chain = 0.25 * band;   // transform: low broken ridges
    return t;
}

// M9-08: dune fields from the prevailing wind; returns a height offset in metres. Q-03: `lee`, when asked, how much the point is a dune's slip
// face (the steep side downwind of its crest, faded as the height is)
double duneField(const Vec3& p, const BodyGen& g, double lat, double amp, double detailM, double* lee) {
    if (lee) *lee = 0;
    if (amp <= 0) return 0;
    double fade = lodFade(g.duneWl, detailM);   // faded like every other feature (a hard cut at 150 m aliased the 64 m maps into stripes)
    if (fade <= 0) return 0;
    // wind: the seed angle, flipped per hemisphere and per 30-degree band (trade winds)
    int bandIdx = (int)std::floor(std::fabs(lat) / (30 * DEG));
    double a = g.windAngle + (lat < 0 ? PI : 0) + bandIdx * 0.9;
    Vec3 dd = normalize(Vec3(std::cos(a), std::sin(a), 0.15 * std::sin(a * 2)));
    Vec3 dp = normalize(cross(dd, Vec3(0, 0, 1)));
    double along = dot(p, dd) / g.duneWl * TAU, across = dot(p, dp) / g.duneWl * TAU;
    double bend = fbm3(p / 3.0, g.sE + 11, 2) * 4.0;
    // the slip face: a dune climbs gently over three quarters of its wavelength on the windward side and drops over
    // the last quarter (the profile of the phase, C1), for the transverse and the barchan styles
    auto slip = [](double ph) { ph -= std::floor(ph); return ph < 0.72 ? smoothstep(0.0, 0.72, ph) : 1.0 - smoothstep(0.72, 1.0, ph); };
    auto face = [](double ph) { ph -= std::floor(ph); return smoothstep(0.72, 0.76, ph) * (1.0 - smoothstep(0.96, 1.0, ph)); };   // the drop's span
    double v;
    if (g.duneStyle == 0) { v = slip((along + bend) / TAU); if (lee) *lee = face((along + bend) / TAU); }                     // transverse
    else if (g.duneStyle == 1) {                                                                                              // barchan-like crescents
        const double ph = (along + bend + 0.6 * std::sin(across * 0.5)) / TAU;
        v = std::pow(slip(ph), 1.7); if (lee) *lee = face(ph);
    } else {   // star dunes (Q-03: their faces where the ground falls downwind)
        v = (std::sin(along + bend) + std::sin(along * 0.5 + across * 0.87 + bend) + std::sin(along * 0.5 - across * 0.87 - bend)) / 3.0 * 0.5 + 0.5;
        if (lee) *lee = smoothstep(-0.2, -0.8, std::cos(along + bend) + 0.5 * std::cos(along * 0.5 + across * 0.87 + bend) + 0.5 * std::cos(along * 0.5 - across * 0.87 - bend));
    }
    if (lee) *lee *= fade;
    return amp * v * fade;
}
// B-404: the material chain's finest octave: 48 m, the twelve samples of the 4 m ring, so the ground underfoot keeps its look and every
// coarser ring decides its materials by the same slope
constexpr double MAT_WL_KM = 0.048;
ReliefSum reliefSum(const Vec3& p, const Vec3& unit, const BodyGen& g, double a0, double ridge, uint64_t seed, double detailM) {
    ReliefSum r;
    if (a0 <= 0) return r;
    const double wl0 = g.reliefWl0;
    Vec3 q = p;
    if (g.reliefWarp > 0) {
        double f1 = lodFade(wl0 * 1.7, detailM);
        if (f1 > 0) {
            double wl = wl0 * 1.7, sc = wl0 * g.reliefWarp * f1;
            q = p + Vec3(gnoise3(p / wl, seed + 91), gnoise3(p / wl, seed + 92), gnoise3(p / wl, seed + 93)) * sc;
        }
        double f2 = lodFade(wl0 * 0.45, detailM);
        if (f2 > 0) {
            double wl = wl0 * 0.45, sc = wl0 * g.reliefWarp * 0.35 * f2;
            q = q + Vec3(gnoise3(q / wl, seed + 94), gnoise3(q / wl, seed + 95), gnoise3(q / wl, seed + 96)) * sc;
        }
    }
    double wl = wl0, amp = a0 / (1 + 1.2 * ridge);   // ridged octaves carry about twice the slope of rolling ones: the grade stays the type's
    const double gain = std::pow(2.0, -g.reliefH);
    Vec3 grad(0, 0, 0), gradM(0, 0, 0);   // of the sum so far, metres per km; B-404: of the material chain
    for (int i = 0; i < 12; i++) {
        double fade = lodFade(wl, detailM);
        bool matOct = wl >= MAT_WL_KM;   // the material chain takes every octave down to 48 m, whatever the sampling scale
        if (fade <= 0 && !matOct) break;
        Vec3 gn;
        double n = gnoise3d(q.x / wl + i * 17.3, q.y / wl + i * 9.1, q.z / wl + i * 3.7, seed + i * 7919, gn);
        double a = std::fabs(n), sg = n < 0 ? -1.0 : 1.0;
        double rw = ridge * clampd(1.0 - i * 0.22, 0.0, 1.0);   // the ridges are the first octaves' (the range and its spurs); the flanks roll
        double v = n + (2.0 * (1 - a) * (1 - a) - 1.0 - n) * rw;
        Vec3 gv = gn + (gn * (-4.0 * (1 - a) * sg) - gn) * rw;
        if (fade > 0) {
            Vec3 gt = grad - unit * dot(grad, unit);
            double w = fade / (1 + g.reliefErosion * length2(gt) * 1e-6);
            if (i > 0) w *= 1 + (clampd(0.4 + r.h / (0.9 * a0), 0.08, 1.0) - 1) * g.reliefHybrid;
            r.h += amp * w * v;
            if (wl >= 1.9) r.hCoarse = r.h;
            grad += gv * (amp * w / wl);
        }
        if (matOct) {   // the same recursion unfaded: identical in every ring, since the weights read only the chain's own coarser octaves
            Vec3 gt = gradM - unit * dot(gradM, unit);
            double w = 1.0 / (1 + g.reliefErosion * length2(gt) * 1e-6);
            if (i > 0) w *= 1 + (clampd(0.4 + r.hM / (0.9 * a0), 0.08, 1.0) - 1) * g.reliefHybrid;
            r.hM += amp * w * v;
            gradM += gv * (amp * w / wl);
        }
        wl *= 0.5; amp *= gain;
    }
    {   // a soft sediment floor: below -0.05 a0 the ground keeps a tenth of its depth (filled valleys and basins); C1 at the floor
        const double f = -0.05 * a0, k = 0.08 * a0;
        if (r.h < f) { double d = f - r.h, e = std::exp(-d / k); r.h = f - (0.1 * d + 0.9 * k * (1 - e)); grad *= 0.1 + 0.9 * e; }
        if (r.hCoarse < f) { double d = f - r.hCoarse, e = std::exp(-d / k); r.hCoarse = f - (0.1 * d + 0.9 * k * (1 - e)); }
        if (r.hM < f) { double d = f - r.hM, e = std::exp(-d / k); r.hM = f - (0.1 * d + 0.9 * k * (1 - e)); gradM *= 0.1 + 0.9 * e; }
    }
    Vec3 gt = grad - unit * dot(grad, unit);
    r.slope = length(gt) * 1e-3;
    r.gradT = gt * 1e-3;
    Vec3 gtM = gradM - unit * dot(gradM, unit);
    r.slopeM = length(gtM) * 1e-3;
    r.gradM = gtM * 1e-3;
    return r;
}
CliffOut cliffBands(const Vec3& p, const BodyGen& g, double h, double slope, double zone, double detailM) {
    CliffOut o;
    if (zone < 0.01 || slope < 0.2) return o;
    double step = g.cliffStep;
    double run = step / std::max(slope, 0.05);
    double on = zone * smoothstep(0.22, 0.32, slope);   // B-404: the band's strength, free of the sampling scale (`slope` is the material chain's)
    if (on <= 0.001) return o;
    double n = 0.5 + 0.5 * gnoise3(p / 1.2 + Vec3(2.7, 9.1, 4.3), g.sC + 0xC1F);
    double amount = smoothstep(1.0 - g.cliffAmount - 0.08, 1.0 - g.cliffAmount + 0.08, n);
    on *= amount;
    if (on <= 0.001) return o;
    double fade = on * smoothstep(1.5, 4.0, run / detailM);   // the relief: a band's run must span a few samples
    // the riser (a tenth of the run) decides the rock and the scree wherever it spans half a cell: the same cells in every ring that can
    // draw it (following `fade`, the rock would come and go with the ring)
    bool mat = matVisible(0.1 * run / 1000.0, detailM);
    if (fade <= 0.001 && !mat) return o;
    double q = std::floor(h / step), f = h / step - q;
    // the band's rise: a bench (35% of it at 0.29 x the slope), the talus (20% at 1.4 x), the riser (20% at 2.75 x: over
    // 1:1 from a 1:2.7 hillside on) and the rounded top (25% at 0.28 x)
    double pr = 0.10 * smoothstep(0.0, 0.35, f) + 0.28 * smoothstep(0.35, 0.55, f) + 0.55 * smoothstep(0.55, 0.75, f) + 0.07 * smoothstep(0.75, 1.0, f);
    o.dh = ((q + pr) * step - h) * fade;
    if (mat) {
        o.cliff = on * smoothstep(0.55, 0.6, f) * smoothstep(0.75, 0.7, f);
        o.talus = on * smoothstep(0.3, 0.38, f) * smoothstep(0.6, 0.52, f);
    }
    return o;
}
// O6-04: the plains broken: hummocks a metre and a half high at 200 m and half a metre at 80 m, low outcrops (knobs of
// 2-4.5 m on one cell in seven of a 60 m grid) and, where `gullies` says it rains, shallow gullies on 200 m cells; all
// fading with the sampling scale; `mask` 0..1 selects the flat country
double plainRelief(const Vec3& p, const BodyGen& g, double mask, double gullies, double detailM) {
    if (mask < 0.01) return 0;
    double f1 = lodFade(0.22, detailM), f2 = lodFade(0.12, detailM), f3 = lodFade(0.05, detailM);
    if (f1 <= 0) return 0;   // nothing of the plains' relief spans this sampling scale
    double h = 3.0 * gnoise3(p / 0.22 + Vec3(1.1, 2.3, 3.7), g.sB + 95) * f1;
    if (f2 > 0) h += 2.0 * gnoise3(p / 0.12 + Vec3(5.2, 1.7, 2.9), g.sB + 96) * f2;
    if (f3 > 0) h += 0.5 * gnoise3(p / 0.05 + Vec3(2.2, 4.7, 0.9), g.sB + 98) * f3;
    // the shaped features fade by their own size against the sampling scale (a knob of 40 m is three cells at 16 m and
    // drawn, where the octave rule would hide it): full when they span four samples (`featFade`, shared since B-404)
    if (scaleVisible(0.1, detailM)) {   // the outcrops: knobs 3-7 m high and 35-70 m across on two fifths of the 100 m cells
        Worley3 w = worley3(p / 0.1, g.sB + 97);
        if (unitFromHash(mix64(w.id1 + 3)) < 0.4) {
            double r = 0.35 * (0.5 + 0.5 * unitFromHash(mix64(w.id1 + 4)));
            double x = w.f1 / r;
            if (x < 1) h += (3.0 + 4.0 * unitFromHash(mix64(w.id1 + 5))) * std::pow(1 - x, 1.4) * featFade(2 * r * 0.1, detailM);
        }
    }
    if (gullies > 0 && scaleVisible(0.2, detailM)) {   // gullies and cracks 50 m wide and 3 m deep along the boundaries of 200 m cells
        Worley3 w = worley3(p / 0.2, g.sB + 92);
        double d = (w.f2 - w.f1) * 100.0;
        h -= 3.0 * gullies * (1 - smoothstep(0.0, 25.0, d)) * featFade(0.05, detailM);
    }
    return h * mask;
}

Relief reliefAt(const BodyGen& g, const Vec3& unit, double a0, double ridge, double detailM) {
    ReliefSum r = reliefSum(unit * g.R, unit, g, a0, ridge, g.sB + 7, detailM);
    return {r.h, r.slope};
}

// Q-01: the landforms' hashed constants (`landformsAt`'s `tu`), for the features read outside it
static inline double landTu(const BodyGen& g, int k) { return unitFromHash(mix64(g.seed ^ (0x1A4DULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); }

GreatShield greatShieldOf(const BodyGen& g) {
    GreatShield S;
    const double lat = std::asin(0.7 * (2 * landTu(g, 40) - 1)), lon = TAU * landTu(g, 41) - PI;   // within 44 degrees of the equator (Olympus at 18 N)
    S.c = StarSystem::bodyFromLatLon(lat, lon);
    S.radKm = std::min(300.0, g.R * (0.045 + 0.04 * landTu(g, 42)));
    S.heightM = clampd(22000.0 * 3390.0 / g.R, 9000.0, 25000.0) * (0.85 + 0.25 * landTu(g, 43));   // Olympus' 22 km at Mars' radius, lower as the gravity grows
    const Vec3 e1 = normalize(cross(S.c, std::fabs(S.c.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(S.c, e1);
    S.nPits = 2 + (int)(landTu(g, 44) * 3); if (S.nPits > 4) S.nPits = 4;
    for (int k = 0; k < S.nPits; k++) {   // the caldera: nested pits within a thirtieth of the radius of the summit, 1-3 km deep
        const double a = TAU * landTu(g, 45 + 4 * k), off = (k == 0 ? 0.0 : 0.035 * landTu(g, 46 + 4 * k)) * S.radKm;
        S.pit[k] = normalize(S.c * g.R + (e1 * std::cos(a) + e2 * std::sin(a)) * off);
        S.pitKm[k] = S.radKm * (0.03 + 0.035 * landTu(g, 47 + 4 * k));
        S.pitDepthM[k] = 1000.0 + 2000.0 * landTu(g, 48 + 4 * k);
    }
    return S;
}

// Q-02: the hot springs' vents within 2.5 km of a point (the surface's steam over each)
void hotSpringsNear(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    if (!g.hasTrait(TR_HOT_SPRINGS)) return;
    const PointGrid G = springGrid(g);
    featuresInBox(unit * g.R, 2.5, g.R, G.cellKm, G.density, G.seed, [&](const FeatureHit& f) {
        for (int k = 0; k < springCount(f); k++) { const HotSpring S = springOf(f, k, g.R); out.push_back({normalize(S.c), S.rKm * 0.14, S.id}); }
    });
}

void landformFeatures(const BodyGen& g, int trait, const Vec3& unit, double halfKm, std::vector<LandFeature>& out) {
    out.clear();
    if (!g.hasTrait(trait)) return;
    const Vec3 pc = unit * g.R;
    auto point = [&](const PointGrid& G, PointSize (*size)(const FeatureHit&), bool depth) {
        featuresInBox(pc, halfKm, g.R, G.cellKm, G.density, G.seed, [&](const FeatureHit& f) {
            const PointSize S = size(f);
            LandFeature F; F.unit = normalize(f.c); F.radiusKm = S.rKm; F.heightM = depth ? -S.heightM : S.heightM;
            F.id = hash3i((int64_t)std::llround(f.c.x * 64), (int64_t)std::llround(f.c.y * 64), (int64_t)std::llround(f.c.z * 64), G.seed);
            out.push_back(F);
        });
    };
    switch (trait) {
        case TR_GREAT_SHIELD: {
            const GreatShield S = greatShieldOf(g);
            for (int k = 0; k < S.nPits; k++) {
                if (std::acos(clampd(dot(S.pit[k], unit), -1, 1)) * g.R > halfKm * 1.5) continue;
                LandFeature F; F.unit = S.pit[k]; F.radiusKm = S.pitKm[k]; F.heightM = -S.pitDepthM[k]; F.id = mix64(g.seed ^ (0xCA1DE7AULL + (uint64_t)k)); out.push_back(F);
            }
            break;
        }
        case TR_CRATER_CHAINS: {
            const PointGrid G = chainGrid(g);
            featuresInBox(pc, halfKm + 0.35 * G.cellKm, g.R, G.cellKm, G.density, G.seed, [&](const FeatureHit& f) {
                const CraterChain C = chainOf(f);
                for (int k = 0; k < C.n; k++) {
                    const Vec3 at = normalize(C.c + C.dir * C.at[k]);
                    if (std::acos(clampd(dot(at, unit), -1, 1)) * g.R > halfKm * 1.5) continue;
                    LandFeature F;
                    F.unit = at;
                    F.radiusKm = C.rKm[k];
                    F.heightM = -C.depthM[k];
                    F.id = mix64(hash3i((int64_t)std::llround(C.c.x * 8), (int64_t)std::llround(C.c.y * 8), (int64_t)std::llround(C.c.z * 8), G.seed) + (uint64_t)k);
                    out.push_back(F);
                }
            });
            break;
        }
        case TR_MUD_VOLCANOES: point(mudGrid(g), mudConeOf, false); break;
        case TR_PINGOS: point(pingoGrid(g), pingoOf, false); break;
        case TR_MAARS: point(maarGrid(g), maarOf, true); break;
        default: break;
    }
}

void steamVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    Vec3 p = unit * g.R;
    visitFeatures(p, g.R, 2.0, 0.5, g.sF + 8, 1.5, [&](const FeatureHit& f) {
        GeyserVent v; v.unit = normalize(f.c); v.radiusKm = 0.06 + 0.12 * f.u[3];
        v.id = hash3i((int64_t)std::llround(f.c.x * 64), (int64_t)std::llround(f.c.y * 64), (int64_t)std::llround(f.c.z * 64), 0x57EAULL);
        out.push_back(v);
    });
}
void springVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    Vec3 p = unit * g.R;
    visitFeatures(p, g.R, SPRING_CELL, SPRING_DENS, g.sF + 33, 4.0, [&](const FeatureHit& f) {
        GeyserVent v; v.unit = normalize(f.c); v.radiusKm = 0.012 + 0.018 * f.u[3];
        v.id = hash3i((int64_t)std::llround(f.c.x * 256), (int64_t)std::llround(f.c.y * 256), (int64_t)std::llround(f.c.z * 256), 0x5B12ULL);
        out.push_back(v);
    });
}
Vec3 nitrogenBasinOf(const BodyGen& g) {   // the nitrogen world's great plain (its middle, a unit vector: within 40 degrees of the equator)
    const double la = std::asin(clampd(0.64 * (2 * unitFromHash(mix64(g.seed ^ 0x51A1ULL)) - 1), -1, 1)), lo = unitFromHash(mix64(g.seed ^ 0x51A2ULL)) * TAU - PI;
    return StarSystem::bodyFromLatLon(la, lo);
}
Vec3 prevailingWind(const BodyGen& g, const Vec3& unit) {
    const Vec3 Z(0, 0, 1);
    Vec3 east = cross(Z, unit);
    const double el = length(east);
    east = el > 1e-6 ? east / el : Vec3(0, 1, 0);
    const Vec3 north = cross(unit, east);
    return north * std::cos(g.windAngle) + east * std::sin(g.windAngle);
}
void plumeVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    if (!(KINDS[g.type].events & EV_PLUMES)) return;
    const Vec3 p = unit * g.R;
    visitFeatures(p, g.R, PLUME_CELL, plumeDensity(g), g.sF + 52, 2.0, [&](const FeatureHit& f) {
        const Vec3 n = normalize(f.c);
        if (std::asin(clampd(n.z, -1, 1)) / DEG > -cryoCapEdge(g, n)) return;   // on the cap only
        GeyserVent v; v.unit = n; v.radiusKm = plumeStreakKm(f.u[3]);   // the streak's length (km)
        v.id = hash3i((int64_t)std::llround(f.c.x * 8), (int64_t)std::llround(f.c.y * 8), (int64_t)std::llround(f.c.z * 8), 0x9105EULL);
        out.push_back(v);
    });
}
Vec3 loneMountainOf(const BodyGen& g) {
    const double la = std::asin(clampd(0.7 * (2 * unitFromHash(mix64(g.seed ^ 0xA4A0ULL)) - 1), -1, 1)), lo = unitFromHash(mix64(g.seed ^ 0xA4A1ULL)) * TAU - PI;
    return StarSystem::bodyFromLatLon(la, lo);
}
void springJets(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    if (!(KINDS[g.type].events & EV_SPRING_JETS)) return;
    const Vec3 p = unit * g.R;
    visitFeatures(p, g.R, SPIDER_CELL, SPIDER_DENS, g.sF + 61, 4.0, [&](const FeatureHit& f) {
        const Vec3 n = normalize(f.c);
        const double absLat = std::fabs(std::asin(clampd(n.z, -1, 1))) / DEG, edge = dryCapEdge(g, n);
        if (absLat > edge || absLat < edge - 9.0) return;   // the spiders' band round the cap
        if (absLat < edge - dryFrostDeg(g, n.z >= 0)) return;   // under the seasonal frost (`groundDryIce`'s)
        GeyserVent v; v.unit = n; v.radiusKm = 0.15 + 0.25 * f.u[3];   // the spider's reach (km)
        v.id = hash3i((int64_t)std::llround(f.c.x * 512), (int64_t)std::llround(f.c.y * 512), (int64_t)std::llround(f.c.z * 512), 0x5D1DEULL);
        out.push_back(v);
    });
}

// P-06: the two-faced moon's apex (planetmap.h)
Vec3 twoFacedApex(const BodyGen& g) {
    const double l = length(g.apexDir);
    if (l > 0.5) return g.apexDir / l;
    return StarSystem::bodyFromLatLon(0, unitFromHash(mix64(g.seed ^ 0xA9E0ULL)) * TAU - PI);   // a body the harness staged
}
// P-06: the tiger stripes, in the south pole's tangent plane (x along `e1`, y along `e2`, km): four rifts 0.13-0.16 of the radius apart and
// 0.45-0.6 of it long (Enceladus's: 35 km apart and 130 long on a moon of 252 km), bowed a little the same way, wandering a little
TigerStripes tigerStripesOf(const BodyGen& g) {
    TigerStripes T;
    auto hu = [&](int k) { return unitFromHash(mix64(g.seed ^ (0x716E0ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
    const double az = hu(0) * TAU, sp = g.R * (0.13 + 0.03 * hu(1));
    T.e1 = Vec3(std::cos(az), std::sin(az), 0); T.e2 = Vec3(std::sin(az), -std::cos(az), 0);
    for (int k = 0; k < 4; k++) {
        T.off[k] = (k - 1.5) * sp + 0.02 * g.R * (hu(2 + k) - 0.5);
        T.bow[k] = g.R * (0.02 + 0.02 * hu(6 + k));
        T.len[k] = g.R * (0.45 + 0.15 * hu(10 + k));
        T.mid[k] = 0.05 * g.R * (hu(14 + k) - 0.5);
        T.ph[k] = hu(18 + k) * TAU;
    }
    return T;
}
double stripeYKm(const TigerStripes& T, double R, int k, double along) {
    const double half = 0.5 * T.len[k], u = clampd(along, -half, half) / half;
    return T.off[k] + T.bow[k] * u * u + 0.012 * R * std::sin(along / (0.06 * R) + T.ph[k]);
}
Vec3 stripePoint(const TigerStripes& T, double R, int k, double along) {
    const double x = (T.mid[k] + along) / R, y = stripeYKm(T, R, k, along) / R;
    return normalize(T.e1 * x + T.e2 * y + Vec3(0, 0, -std::sqrt(std::max(0.0, 1 - x * x - y * y))));
}
double stripeDistanceKm(const TigerStripes& T, double R, const Vec3& unit, int* which, double* along) {
    if (unit.z > -0.5) return 1e9;   // the south polar terrain lies within 50 degrees of the pole at most
    const double x = dot(unit, T.e1) * R, y = dot(unit, T.e2) * R;
    double best = 1e9;
    for (int k = 0; k < 4; k++) {
        const double half = 0.5 * T.len[k], u = x - T.mid[k], uc = clampd(u, -half, half);
        const double dy = y - stripeYKm(T, R, k, uc), d = std::sqrt((u - uc) * (u - uc) + dy * dy);
        if (d < best) { best = d; if (which) *which = k; if (along) *along = u; }
    }
    return best;
}
static constexpr double CURTAIN_STEP = 1.0;   // km between the curtains' vents along a stripe
void curtainVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    if (!(KINDS[g.type].events & EV_CURTAINS) || unit.z > -0.3) return;
    const TigerStripes T = tigerStripesOf(g);
    for (int k = 0; k < 4; k++) {
        const double half = 0.5 * T.len[k];
        const int n = (int)(T.len[k] / CURTAIN_STEP);
        for (int i = 0; i <= n; i++) {
            const uint64_t id = mix64(g.seed ^ (0xC0E7A1ULL + (uint64_t)(k * 4096 + i)));
            const double a = -half + i * CURTAIN_STEP + 0.6 * CURTAIN_STEP * (unitFromHash(id) - 0.5);
            const Vec3 v = stripePoint(T, g.R, k, clampd(a, -half, half));
            if (length(v - unit) * g.R > 400.0) continue;
            GeyserVent gv; gv.unit = v; gv.radiusKm = 0.3 + 0.7 * unitFromHash(mix64(id + 1)); gv.id = id;
            out.push_back(gv);
        }
    }
}
bool greatCliffOf(const BodyGen& g, Vec3& foot, Vec3& up) {
    Corona C[3];
    if (coronaeOf(g, C) < 1) return false;
    double th0, span, H; cliffOf(g, th0, span, H);
    const double ct = std::cos(th0), st = std::sin(th0), r = std::pow(std::pow(std::fabs(ct), C[0].n) + std::pow(std::fabs(st), C[0].n), -1.0 / C[0].n);
    const double rf = 1 - 5.0 / std::sqrt(C[0].a * C[0].b);   // the foot 5 km in from the edge
    const double x = C[0].a * r * ct * rf, y = C[0].b * r * st * rf;
    foot = normalize(C[0].c + C[0].e1 * (x / g.R) + C[0].e2 * (y / g.R));
    up = normalize(C[0].e1 * (ct / C[0].a) + C[0].e2 * (st / C[0].b));   // outward, roughly across the edge
    up = normalize(up - foot * dot(up, foot));
    return true;
}

void geyserVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    Vec3 p = unit * g.R;
    visitFeatures(p, g.R, 20.0, 0.35, g.sE + 370, 0.1, [&](const FeatureHit& f) {
        double r = 0.3 + 0.6 * f.u[3];
        GeyserVent v; v.unit = normalize(f.c); v.radiusKm = r;
        v.id = hash3i((int64_t)std::llround(f.c.x * 64), (int64_t)std::llround(f.c.y * 64), (int64_t)std::llround(f.c.z * 64), 0x6E75ULL);
        out.push_back(v);
    });
}
double giantStormCell(const BodyGen& g, const Vec3& unit, uint64_t* id) {
    Vec3 q = (unit * g.R) / (g.R * 0.12);   // the branch's arithmetic, to the bit
    q.z *= 2.5;
    Worley3 w = worley3(q, g.sC);
    const bool on = unitFromHash(w.id1) > 1 - g.stormShare && w.f1 < 0.4;
    if (id) *id = on ? w.id1 : 0;
    return on ? 1 - smoothstep(0.15, 0.4, w.f1) : 0;
}

// D-04 (TR_EMBERS): the ember fields (`emberLakes`), the globe's and the ground's
void emberFieldsOf(const BodyGen& g, std::vector<EmberField>& out) {
    out.clear();
    if (!g.dark || !g.hasTrait(TR_EMBERS) || isLavaWorld(g.type)) return;
    const PointGrid G = emberGrid(g);
    featuresInBox(Vec3(0, 0, 0), g.R + G.cellKm, g.R, G.cellKm, G.density, G.seed, [&](const FeatureHit& h) { out.push_back({normalize(h.c), emberFieldKm(h), 0.55 + 0.45 * h.u[4]}); });
}
double emberFieldAt(const BodyGen& g, const Vec3& p) {
    const PointGrid G = emberGrid(g);
    double f = 0;
    visitFeatures(p, g.R, G.cellKm, G.density, G.seed, 0.1, [&](const FeatureHit& h) { const double rF = emberFieldKm(h); f = std::max(f, (0.55 + 0.45 * h.u[4]) * smoothstep(rF, 0.55 * rF, h.d)); });
    return f;
}

double benchFeatureField(const Vec3& p, double R, uint64_t seed) { return domeField(p, R, 2.0, 0.6, seed, 0.4, 100.0, 0.0); }
