#include "planetmap.h"
#include "drainage.h"
#include "peoples.h"
#include "core/noise.h"
#include <cmath>

const char* const MATERIAL_NAMES[] = {"ROCK", "SAND", "GRASS", "FOREST", "SNOW", "WATER", "LAVA", "ICE", "CLOUD",
                                      "QUARTZ", "BASALT", "DUST", "GAS", "IRON", "SULPHUR", "GRAPHITE", "SALT", "GLASS", "MUD", "POTASH", "BRINE",
                                      "MATS", "STROMATOLITE", "CRUST", "MOSS"};
static_assert(sizeof(MATERIAL_NAMES) / sizeof(MATERIAL_NAMES[0]) == MAT_COUNT, "a name for every material");
// R-307: one sky pair per kind with air (P-00: the row's: from its tint, a thin air's, or fixed), shared by the ground, the globe and the map
bool typeSky(const BodyGen& g, RGB& zen, RGB& hor) {
    const KindDef& K = KINDS[g.type];
    if (!K.atmosphere || (K.tags & KT_GIANT)) return false;
    RGB white(1, 1, 1);
    switch (K.sky) {
        case SKY_THIN: zen = g.skyTint * 0.35f; hor = g.skyTint * 0.75f; break;
        case SKY_FIXED: zen = K.zenith; hor = K.horizon; break;
        default: zen = g.skyTint * 0.75f; hor = lerp(g.skyTint, white, 0.55f); break;
    }
    return true;
}
std::string traitList(const BodyGen& g) {
    std::string s;
    for (int k = 0; k < TRAIT_SLOTS; k++) if (g.traits[k]) { if (!s.empty()) s += ", "; s += TRAITS[g.traits[k]].name; }
    return s;
}
const char* BIOME_NAMES[BIO_COUNT] = {"", "TROPICAL FOREST", "SAVANNA", "DESERT", "TEMPERATE FOREST", "GRASSLAND", "TAIGA", "TUNDRA", "WETLAND", "ALPINE", "ICE SHEET", "OCEAN"};

namespace {
// Number of fbm octaves whose finest wavelength still spans several samples.
inline int octs(double wavelengthKm, double detailM) {
    double n = std::log2(wavelengthKm * 1000.0 / (detailM * 4.0));
    return clampi((int)std::floor(n) + 1, 1, 14);
}
inline bool scaleVisible(double cellKm, double detailM) { return cellKm * 1000.0 >= detailM * 3.0; }
// Amplitude multiplier that fades out features smaller than the sampling scale.
inline double lodFade(double wavelengthKm, double detailM) { return smoothstep(4.0, 12.0, wavelengthKm * 1000.0 / detailM); }
// B-404: a shaped feature of a known width (a mesa, a tongue, a tower, a trench: one thing, not a periodic pattern) is
// drawn when it spans a few samples: full at four, gone under one and a half (O6-04's rule for the plains' knobs).
// `lodFade`'s twelve samples per wavelength guard noise against aliasing; on the landforms it hid every shaped feature
// from the ring that could have drawn it
inline double featFade(double widthKm, double detailM) { return smoothstep(1.5, 4.0, widthKm * 1000.0 / detailM); }
// B-404: a material is decided by the unfaded feature wherever its patch covers at least a quarter of a cell of the
// sampling scale, so every ring that can draw a patch at all draws the same one (a line narrower than a cell comes out
// as a chain of cells with gaps, which is what a line looks like at that scale; a half-cell rule ended the icy worlds'
// 200 m crack lines and the chaos cracks at the 512 m ring, where they used to reach 13 km); a narrower patch is the
// coarser ring's surroundings, and the world map (texels of 11-44 km) takes nothing under a few kilometres. A material
// must never follow a faded value: the fade differs between the rings, and a threshold on it put a patch in one ring
// and not in the next, its edge sweeping along with the explorer (the europan stain of B-404, the lava tongues, the
// cliffs' rock and scree, the ergs' sand, the landforms' walls)
// The world map's texels (11-44 km; the drainage stops at 2100 m, the far ring is 2048) take only a patch that spans
// half of one: a texel stands for a world-sized area, and the trap terraces' risers (2 km) on a 7 km texel came out as
// scattered dots; the icy worlds' great crack lines (7-20 km) stay the dotted net the globe always showed
inline bool matVisible(double widthKm, double detailM) { return widthKm * 1000.0 >= (detailM > 2100.0 ? 0.5 : 0.25) * detailM; }

struct FeatureHit {
    double d;        // chord distance from centre, km
    double u[6];     // random numbers of the feature
    double cellKm;
    Vec3 c;          // centre on the sphere, km
};

// Visit the features of a jittered cubic grid whose random points lie on a thin
// shell around the sphere. `fn(hit)` is called for each feature in range.
template <class F>
void visitFeatures(const Vec3& p, double R, double cellKm, double density, uint64_t seed, double reachCells, F fn) {
    double inv = 1.0 / cellKm;
    int64_t cx = (int64_t)std::floor(p.x * inv), cy = (int64_t)std::floor(p.y * inv), cz = (int64_t)std::floor(p.z * inv);
    uint64_t hxy[3][3];   // F-07: hash3i's (x, y) steps once for the 27 cells
    for (int dx = -1; dx <= 1; dx++) { const uint64_t hx = hashX(cx + dx, seed); for (int dy = -1; dy <= 1; dy++) hxy[dy + 1][dx + 1] = hashXY(hx, cy + dy); }
    for (int dz = -1; dz <= 1; dz++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                uint64_t h = hashXYZ(hxy[dy + 1][dx + 1], cz + dz);
                if (unitFromHash(h) > density) continue;
                double u0 = unitFromHash(mix64(h + 1)), u1 = unitFromHash(mix64(h + 2)), u2 = unitFromHash(mix64(h + 3));
                Vec3 c((cx + dx + u0) * cellKm, (cy + dy + u1) * cellKm, (cz + dz + u2) * cellKm);
                double cl = length(c);
                if (std::fabs(cl - R) > cellKm * 0.5) continue;
                c = c * (R / cl);
                double d = length(p - c);
                if (d > cellKm * reachCells) continue;
                FeatureHit hit;
                hit.d = d; hit.cellKm = cellKm; hit.c = c;
                for (int k = 0; k < 6; k++) hit.u[k] = unitFromHash(mix64(h + 10 + k));
                fn(hit);
            }
}

// Q-01: the features of the same jittered grid whose points lie in a box round `pc` (`halfKm` each way): what `visitFeatures` finds near
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

// Q-01: the new landforms' point features, the grids they stand on and each one's size from its hashes (the landform and the scanner read the
// same): mud volcanoes 6-50 m high and 100-700 m across on a third of the cells of a 1.5 km grid; pingos 10-60 m high and 80-400 m across on
// a quarter of a 1.2 km grid's; maars 0.4-2 km across and 50-200 m deep on 45% of a 4 km grid's; the crater chains on 15% of the cells of a
// grid of a fifth of the radius (150-450 km), 60-280 km long, six to seventeen craters of one size
struct PointGrid { double cellKm, density; uint64_t seed; };
inline PointGrid mudGrid(const BodyGen& g) { return {1.5, 0.35, g.sE + 450}; }
inline PointGrid pingoGrid(const BodyGen& g) { return {1.2, 0.25, g.sE + 460}; }
inline PointGrid maarGrid(const BodyGen& g) { return {4.0, 0.45, g.sE + 470}; }
inline PointGrid chainGrid(const BodyGen& g) { return {clampd(g.R * 0.18, 150.0, 450.0), 0.15, g.sE + 480}; }
// Q-02: the hot springs' clusters (a third of the cells of a 3 km grid), three to six springs within 0.7 km of a cluster's middle, each a
// terraced mound 80-300 m across and 3-17 m high with four to eight terraces
inline PointGrid springGrid(const BodyGen& g) { return {3.0, 0.33, g.sE + 490}; }
inline int springCount(const FeatureHit& f) { return 3 + (int)(f.u[3] * 4); }
// Q-02: the meteorites' strewn fields: on 22% of the cells of a 70 km grid an ellipse 6-25 km long and 1.5-4 km wide along the way the body
// came in and broke up, its middle `c`, its way `dir` (downrange, where the heaviest masses went furthest) and across it `perp`
inline PointGrid strewnGrid(const BodyGen& g) { return {70.0, 0.22, g.sE + 500}; }
// D-04 (TR_EMBERS): the ember fields of a young interior (`emberLakes`): 45% of the cells of a 1,300 km grid (a few dozen a world), each 80-250 km
// across, its strength (how many lakes, how bright from orbit) 0.55-1
inline PointGrid emberGrid(const BodyGen& g) { return {1300.0, 0.45, g.sE + 520}; }
inline double emberFieldKm(const FeatureHit& f) { return 40.0 + 85.0 * f.u[3]; }
struct StrewnField { Vec3 c, dir, perp; double lenKm, widthKm; };
inline StrewnField strewnOf(const FeatureHit& f) {
    StrewnField S; S.c = f.c;
    const Vec3 cn = normalize(f.c);
    const Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
    const double a = TAU * f.u[0];
    S.dir = e1 * std::cos(a) + e2 * std::sin(a); S.perp = cross(cn, S.dir);
    S.lenKm = 6.0 + 19.0 * f.u[3]; S.widthKm = 1.5 + 2.5 * f.u[4];
    return S;
}
struct HotSpring { Vec3 c; double rKm, heightM; int terraces; uint64_t id; };
inline HotSpring springOf(const FeatureHit& f, int k, double R) {
    const Vec3 cn = normalize(f.c);
    const Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
    auto fr = [](double v) { return v - std::floor(v); };
    const double a = TAU * fr(f.u[0] + 0.618034 * k), d = k == 0 ? 0.0 : 0.7 * std::sqrt(fr(f.u[1] + 0.414214 * k));
    HotSpring S;
    S.c = normalize(f.c + (e1 * std::cos(a) + e2 * std::sin(a)) * d) * R;
    S.rKm = 0.04 + 0.11 * fr(f.u[2] + 0.31 * k);
    S.heightM = (3.0 + 14.0 * fr(f.u[4] + 0.71 * k)) * std::sqrt(S.rKm / 0.15);
    S.terraces = 4 + (int)(fr(f.u[5] + 0.23 * k) * 5);
    S.id = mix64((uint64_t)(f.c.x * 4096) ^ ((uint64_t)(f.c.z * 4096) << 20) ^ (uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL);
    return S;
}
struct PointSize { double rKm, heightM, rimM; };
inline PointSize mudConeOf(const FeatureHit& f) { const double r = 0.05 + 0.3 * f.u[3] * f.u[3]; return {r, (8.0 + 40.0 * f.u[4]) * std::sqrt(r / 0.35), 0}; }
inline PointSize pingoOf(const FeatureHit& f) { const double r = 0.04 + 0.16 * f.u[3]; return {r, (10.0 + 50.0 * f.u[4] * std::sqrt(f.u[4])) * (0.5 + 2.5 * r), 0}; }
inline PointSize maarOf(const FeatureHit& f) { const double r = 0.2 + 0.8 * f.u[3] * std::sqrt(f.u[3]); return {r, (50.0 + 150.0 * f.u[4]) * std::min(1.0, r / 0.6), 8.0 + 20.0 * f.u[5]}; }
// a chain: its middle, its direction along the ground (unit, tangent), its length and its craters (their places along it, km, and radii)
struct CraterChain { Vec3 c, dir, perp; double lenKm = 0; int n = 0; double at[17], rKm[17], depthM[17]; };
inline CraterChain chainOf(const FeatureHit& f) {
    CraterChain C;
    C.c = f.c;
    const Vec3 cn = normalize(f.c);
    const Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
    const double a = TAU * f.u[0];
    C.dir = e1 * std::cos(a) + e2 * std::sin(a); C.perp = cross(cn, C.dir);
    C.lenKm = f.cellKm * (0.2 + 0.42 * f.u[1]);
    C.n = 6 + (int)(f.u[2] * 12); if (C.n > 17) C.n = 17;
    const double sp = C.lenKm / (C.n - 1), rb = sp * (0.42 + 0.12 * f.u[3]);
    const uint64_t hc = hash3i((int64_t)std::llround(f.c.x * 8), (int64_t)std::llround(f.c.y * 8), (int64_t)std::llround(f.c.z * 8), 0xCA7E4AULL);
    for (int k = 0; k < C.n; k++) {
        const uint64_t hk = mix64(hc + 7 * (uint64_t)k + 1);
        C.at[k] = -0.5 * C.lenKm + k * sp + 0.12 * sp * (unitFromHash(hk) - 0.5);
        C.rKm[k] = rb * (0.85 + 0.3 * unitFromHash(mix64(hk + 1)));
        C.depthM[k] = C.rKm[k] * 1000.0 * 0.22 * (0.75 + 0.25 * f.u[4]);
    }
    return C;
}

// Crater field (M9-06): returns height (metres); accumulates rim brightness, floor darkening,
// bright ray systems of young craters, mare flooding of old large basins and the secondary
// field around large craters (which raises the density of the small crater pass).
struct CraterMarks { double rimBright = 0, floorDark = 0, rays = 0, flooded = 0, secondary = 0, young = 0; };   // P-05: young, 1 at the middle of the floor of a young crater 2 km wide or more (the brine dwarf's faculae)
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
                double tq = (k + smoothstep(0.42, 0.58, fr)) / 3;   // O6-07: the risers over a sixth of a terrace's run: scarps over 1:1 (the basin walls are the crater-field types' cliffs)
                double xq = 0.55 + 0.45 * tq;
                bowl += (-(1 - xq * xq) - bowl) * tf * 0.8;
            }
        }
        if (flooded && bowl < -0.35) bowl = -0.35;                     // lava filled the old basin
        double rim = 0.35 * std::exp(-((x - 1) * (x - 1)) / (0.14 * 0.14));
        double ejecta = x > 1 ? 0.10 * std::exp(-(x - 1) / 0.35) : 0;
        if (big && x > 1 && x < 1.8 && scaleVisible(r * 0.05, detailM)) ejecta *= 1 + 0.6 * gnoise3(p / (r * 0.08), seed + 0xE3);   // O6-05: the ejecta blanket is rough ground
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

// O6-05: lava tongues and pit chains from the cones of a `volcanoField` grid (the same cells, seed and density, so every
// cone has its flows): two to four tongues at hashed azimuths run from the cone's foot 3-8 radii downhill along the
// radial line, 15% of the radius wide at the foot and tapering, 4-10 m thick with rounded levees; one chain of collapse
// pits (skylights of a lava tube) runs out at another azimuth. Returns the height offset and sets `flow` (0..1, fresh
// basalt) and `pit` (0..1)
struct LavaOut { double dh = 0, flow = 0, pit = 0; };
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
            o.dh += thick * shape * tip * featFade(widthKm, detailM);                   // B-404: the relief by the tongue's own width (it faded by `lodFade` of the foot's: half there in the 64 m ring, gone at 512)
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
                if (rr < 1) { double bowl = (1 - rr) * (1 - rr) * pr; o.dh -= (6.0 + 6.0 * unitFromHash(mix64(hk + 3))) * bowl * featFade(pitKm, detailM); if (matVisible(pitKm, detailM)) o.pit = std::max(o.pit, bowl); }
            }
        }
    });
    return o;
}

// M9-02: plates as worley cells on the unit sphere. Returns the uplift of the plate interior
// (continental +, oceanic -), and the boundary terms: convergent chains, rifts, arcs.
struct Tectonics { double uplift = 0, chain = 0, rift = 0, ridge = 0, arc = 0, bdKm = 1e9; };
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
double duneField(const Vec3& p, const BodyGen& g, double lat, double amp, double detailM, double* lee = nullptr) {
    if (lee) *lee = 0;
    if (amp <= 0) return 0;
    double fade = lodFade(g.duneWl, detailM);   // O6-05 (KI-311): faded like every other feature (a hard cut at 150 m aliased the 64 m maps into stripes)
    if (fade <= 0) return 0;
    // wind: the seed angle, flipped per hemisphere and per 30-degree band (trade winds)
    int bandIdx = (int)std::floor(std::fabs(lat) / (30 * DEG));
    double a = g.windAngle + (lat < 0 ? PI : 0) + bandIdx * 0.9;
    Vec3 dd = normalize(Vec3(std::cos(a), std::sin(a), 0.15 * std::sin(a * 2)));
    Vec3 dp = normalize(cross(dd, Vec3(0, 0, 1)));
    double along = dot(p, dd) / g.duneWl * TAU, across = dot(p, dp) / g.duneWl * TAU;
    double bend = fbm3(p / 3.0, g.sE + 11, 2) * 4.0;
    // O6-05: the slip face: a dune climbs gently over three quarters of its wavelength on the windward side and drops over
    // the last quarter (the profile of the phase, C1), for the transverse and the barchan styles
    auto slip = [](double ph) { ph -= std::floor(ph); return ph < 0.72 ? smoothstep(0.0, 0.72, ph) : 1.0 - smoothstep(0.72, 1.0, ph); };
    auto face = [](double ph) { ph -= std::floor(ph); return smoothstep(0.72, 0.76, ph) * (1.0 - smoothstep(0.96, 1.0, ph)); };   // Q-03: the drop's span
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

inline double warpedFbm(const Vec3& p, double wl, uint64_t seed, int oct, double warpAmt) {
    Vec3 q = p;
    if (warpAmt > 0) {
        double wx = gnoise3(p / (wl * 0.7), seed + 91), wy = gnoise3(p / (wl * 0.7), seed + 92),
               wz = gnoise3(p / (wl * 0.7), seed + 93);
        q = p + Vec3(wx, wy, wz) * (wl * warpAmt);
    }
    return fbm3(q / wl, seed, oct);
}

// O6-02: the relief spectrum. A self-affine sum of octaves from `g.reliefWl0` km down to the sampling scale: octave i
// has the wavelength wl0 / 2^i and the amplitude a0 (wl_i / wl0)^H. Each octave is shaped toward ridges by `ridge`
// (2 (1 - |n|)^2 - 1: a sharp crest at the zero crossing, wide valleys), weighted by the slope of the sum so far
// (steep ground stops gathering detail, so scarps stay smooth while crests and flats gather it: the erosion look, from
// the analytic gradient) and by its height (low ground is smoothed, the hybrid multifractal: valley floors read as
// sediment); the coordinate is folded twice by low-frequency noise so ridges curve. Every octave fades with `lodFade`
// and the weights depend only on the coarser octaves, so the map, the far ring and the ground add the same terms at
// their own scales. Returns metres and the gradient of the sum on the tangent plane (rise over run).
struct ReliefSum { double h = 0; double slope = 0; double hCoarse = 0; Vec3 gradT; double hM = 0, slopeM = 0; Vec3 gradM; };   // B-320: hCoarse = the octaves of 2 km and longer, with the floor; O6-04: gradT = the gradient on the tangent plane, rise over run, uphill; B-404: hM, slopeM, gradM = the material chain, the octaves of MAT_WL_KM and longer unfaded at every sampling scale
constexpr double MAT_WL_KM = 0.048;   // B-404: the material chain's finest octave: 48 m, the twelve samples of the 4 m ring, so the ground underfoot keeps its look and every coarser ring decides its materials by the same slope
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
        bool matOct = wl >= MAT_WL_KM;   // B-404: the material chain takes every octave down to 48 m, whatever the sampling scale
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
// the amplitude of the relief sum from the mountain intensity `rel` (0..1) and the hill country field `hillsF` (0..1)
inline double reliefAmp(const BodyGen& g, double rel, double hillsF) { return g.reliefFlat + (g.reliefHill - g.reliefFlat) * hillsF * (1 - rel) + (g.reliefMtn - g.reliefFlat) * rel; }

// O6-04: cliff bands. On steep ground the height is remapped within bands of `cliffStep` metres: a bench (the first 35%
// of the band's rise, at a seventh of the slope), a talus ramp (the next 20%, at 1.4 times the slope: 35 degrees on a
// 1:2 hillside), the riser (30% of the rise over a tenth of the run: over 1:1, the cliff) and a rounded top; the remap is
// a monotone C1 bijection of the band, so the ground is continuous and no band is higher than the next. Half of the
// steep ground is cliffed (`cliffAmount`, by a noise of 1.2 km), and a band's run must span a few samples (1.5-4 x the
// sampling scale), so the 16 m ring and the near ring carry the cliffs and the 64 m ring shows the slope they stand on
struct CliffOut { double dh = 0, cliff = 0, talus = 0; };
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
    bool mat = matVisible(0.1 * run / 1000.0, detailM);      // B-404: the riser (a tenth of the run) decides the rock and the scree wherever it spans half a cell: the same cells in every ring that can draw it (it used to follow `fade`, and the rock came and went with the ring)
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
}

Relief reliefAt(const BodyGen& g, const Vec3& unit, double a0, double ridge, double detailM) {
    ReliefSum r = reliefSum(unit * g.R, unit, g, a0, ridge, g.sB + 7, detailM);
    return {r.h, r.slope};
}

constexpr double MatRamp::pos[6];

MatRamp materialRamp(const RGB& c, const RGB* skyLight, double day) {
    RGB white(1, 1, 1);
    MatRamp r;
    RGB dark = c * 0.25f;
    if (skyLight) dark = lerp(dark, *skyLight * 0.55f + c * 0.12f, (float)(0.6 * day));
    r.stop[0] = RGB(0, 0, 0); r.stop[1] = dark; r.stop[2] = lerp(dark, c, 0.55f); r.stop[3] = c; r.stop[4] = lerp(c, white, 0.5f); r.stop[5] = white;
    return r;
}

RGB materialRampColor(const MatRamp& r, double stop) {
    if (stop <= 0) return r.stop[0];
    if (stop >= 63) return r.stop[5];
    int k = 0;
    while (k < 4 && stop > MatRamp::pos[k + 1]) k++;
    return lerp(r.stop[k], r.stop[k + 1], (float)((stop - MatRamp::pos[k]) / (MatRamp::pos[k + 1] - MatRamp::pos[k])));
}

const char* reliefClass(double slope) { return slope < 0.05 ? "FLAT" : (slope < 0.2 ? "HILLS" : "MOUNTAINS"); }   // O6-02: 0.04 / 0.15 before the spectrum

int matFamily(int material) {
    switch (material) {
        case MAT_WATER: case MAT_GLASS: case MAT_BRINE: return FAM_WATER;   // S-03: the glass takes the water family's bank (bank 2, the secondary) in the glass's colour; a glassed world has no sea
        case MAT_FOREST: return FAM_FOREST;
        case MAT_GRASS: case MAT_MATS: case MAT_MOSS: return FAM_GRASS;   // P-02: where they are no kind's accent, the mats and the moss are the grass's
        case MAT_SAND: case MAT_DUST: case MAT_SULPHUR: case MAT_QUARTZ: case MAT_MUD: return FAM_SAND;
        case MAT_SNOW: case MAT_ICE: case MAT_CLOUD: case MAT_SALT: case MAT_POTASH: return FAM_SNOW;
        default: return FAM_ROCK;   // rock, basalt, lava, gas, metal, graphite; P-02: the stromatolites and the crusts
    }
}

// P-00: a kind's accents are drawn in the grass and forest families (their banks: 10 and 3 on the ground, the globe's own)
int matFamilyOn(int type, int material) {
    const KindDef& K = KINDS[type];
    if (material == K.accent[0]) return FAM_GRASS;
    if (material == K.accent[1]) return FAM_FOREST;
    return matFamily(material);
}

int familyRep(int type, int family) {
    const KindDef& K = KINDS[type];
    if (family == FAM_GRASS && K.accent[0] >= 0) return K.accent[0];
    if (family == FAM_FOREST && K.accent[1] >= 0) return K.accent[1];
    return K.famRep[family >= 0 && family < FAM_COUNT ? family : FAM_ROCK];
}

// N1-01 (B-202): one palette per body. The base hue is the body's colour seen from afar (typeColor); each
// material mixes its own tint with it, so a red thin-atmosphere world is red underfoot and a felisian
// world is green forest, straw grass, tan sand and white snow in every view. P-00: a kind's own mixes are its row's
static void materialPalette(BodyGen& g) {
    using namespace tint;
    RGB base = g.color;
    const KindDef& K = KINDS[g.type];
    bool atmo = K.atmosphere;
    RGB* m = g.matColor;
    RGB veg = (g.vegColor.r + g.vegColor.g + g.vegColor.b > 0.05f) ? g.vegColor : RGB(0.25f, 0.5f, 0.15f);   // worlds without flora still own a colour
    m[MAT_ROCK] = lerp(ROCK_GREY, base, 0.5f);
    m[MAT_BASALT] = lerp(BASALT_DARK, base, 0.25f);
    m[MAT_DUST] = lerp(DUST_WARM, base, 0.45f);
    m[MAT_SAND] = lerp(SAND_TAN, base, 0.3f);
    m[MAT_GRASS] = lerp(veg, RGB(0.72f, 0.66f, 0.32f), 0.4f);   // straw in the green
    m[MAT_FOREST] = veg * 0.85f;
    m[MAT_SNOW] = atmo ? lerp(SNOW_WHITE, g.skyTint, 0.1f) : SNOW_WHITE;
    m[MAT_ICE] = atmo ? lerp(ICE_BLUE, g.skyTint, 0.15f) : ICE_BLUE;
    m[MAT_WATER] = atmo ? lerp(WATER, g.skyTint * 0.55f, 0.25f) : WATER;
    m[MAT_LAVA] = LAVA;
    m[MAT_CLOUD] = CLOUD_WHITE;
    m[MAT_QUARTZ] = lerp(QUARTZ_PALE, base, 0.3f);
    m[MAT_METAL] = lerp(METAL_GREY, base, 0.5f);
    m[MAT_SULPHUR] = lerp(SULPHUR_YELLOW, base, 0.4f);
    m[MAT_GRAPHITE] = lerp(GRAPHITE, base, 0.3f);
    m[MAT_GAS] = base;
    m[MAT_SALT] = lerp(RGB(0.93f, 0.92f, 0.87f), base, 0.08f);   // R-305: playas and sinter
    m[MAT_GLASS] = RGB(0.50f, 0.66f, 0.52f);   // S-03: pale green glass (the glassed worlds set their own below)
    m[MAT_MUD] = lerp(RGB(0.42f, 0.36f, 0.30f), base, 0.2f);   // P-01: the kinds that use them set their own
    m[MAT_POTASH] = RGB(0.76f, 0.46f, 0.42f);
    m[MAT_BRINE] = lerp(RGB(0.46f, 0.60f, 0.30f), base, 0.15f);
    m[MAT_MATS] = RGB(0.46f, 0.20f, 0.34f);   // P-02: the kinds that use them set their own
    m[MAT_STROMATOLITE] = lerp(RGB(0.58f, 0.54f, 0.40f), base, 0.2f);
    m[MAT_CRUST] = RGB(0.78f, 0.45f, 0.14f);
    m[MAT_MOSS] = RGB(0.30f, 0.40f, 0.15f);
    for (int i = 0; i < K.nPalette; i++) {
        const MatMix& x = K.palette[i];
        switch (x.mode) {
            case MM_BASE_MIX: m[x.mat] = lerp(x.c, base, x.t); break;
            case MM_SKY_MIX: m[x.mat] = lerp(x.c, g.skyTint, x.t); break;
            case MM_C2_MIX: m[x.mat] = lerp(g.color2, base, x.t); break;
            case MM_MIX: m[x.mat] = lerp(x.c, x.d, x.t); break;
            case MM_BASE: m[x.mat] = base; break;
            default: m[x.mat] = x.c; break;
        }
    }
    // P-02: the microbes' hue is the world's (the purple of the retinal mats, the orange of the carotenoids, the green-bronze of the
    // first photosynthesis: DasSarma and Schwieterman 2018), as is the lichen's (orange, the map lichen's yellow-green, a rust red)
    if (K.tags & KT_MICROBIAL) {
        static const RGB mats[3] = {RGB(0.46f, 0.20f, 0.34f), RGB(0.62f, 0.34f, 0.14f), RGB(0.44f, 0.46f, 0.18f)};
        m[MAT_MATS] = mats[(int)(unitFromHash(mix64(g.seed ^ 0x3A75ULL)) * 3) % 3];
    }
    if (K.tags & KT_VENT_LIFE) {   // P-03: the vents' mats pale (the white of the sulphur oxidisers, a cream, the pink of the thermophiles)
        static const RGB vent[3] = {RGB(0.86f, 0.85f, 0.80f), RGB(0.86f, 0.76f, 0.56f), RGB(0.84f, 0.60f, 0.62f)};
        m[MAT_MATS] = vent[(int)(unitFromHash(mix64(g.seed ^ 0xC4E3ULL)) * 3) % 3];
    }
    if (K.accent[0] == MAT_CRUST) {
        static const RGB crusts[3] = {RGB(0.78f, 0.45f, 0.14f), RGB(0.60f, 0.64f, 0.24f), RGB(0.62f, 0.28f, 0.16f)};
        m[MAT_CRUST] = crusts[(int)(unitFromHash(mix64(g.seed ^ 0xC2057ULL)) * 3) % 3];
    }
    // R-304: the colour traits tint the ground family, the exotic seas colour their liquid
    if (g.hasTrait(TR_RED_SOIL)) { RGB red(0.62f, 0.30f, 0.16f); m[MAT_ROCK] = lerp(m[MAT_ROCK], red, 0.5f); m[MAT_SAND] = lerp(m[MAT_SAND], red, 0.55f); m[MAT_DUST] = lerp(m[MAT_DUST], red, 0.6f); m[MAT_BASALT] = lerp(m[MAT_BASALT], red, 0.25f); }
    if (g.hasTrait(TR_BLACK_SAND)) { RGB blk(0.13f, 0.12f, 0.13f); m[MAT_SAND] = lerp(m[MAT_SAND], blk, 0.8f); m[MAT_DUST] = lerp(m[MAT_DUST], blk, 0.7f); m[MAT_ROCK] = lerp(m[MAT_ROCK], blk, 0.35f); }
    if (g.hasTrait(TR_CHALK)) { RGB chalk(0.9f, 0.88f, 0.8f); m[MAT_ROCK] = lerp(m[MAT_ROCK], chalk, 0.6f); m[MAT_SAND] = lerp(m[MAT_SAND], chalk, 0.5f); m[MAT_DUST] = lerp(m[MAT_DUST], chalk, 0.6f); }
    if (g.hasTrait(TR_EXOTIC_SEAS)) m[MAT_WATER] = K.exoticColour;   // tar, brine, methane
    // Q-02: the fungal forests' caps pale (cream, ochre, white, a lilac grey: the forest family's colour, so the globe and the maps see it; the
    // second family's is `make`'s), the hot springs' mats in the thermophiles' orange (the rings round the vents, `springOf`)
    if (g.hasTrait(TR_FUNGAL)) {
        static const RGB F[4] = {RGB(0.86f, 0.80f, 0.64f), RGB(0.80f, 0.60f, 0.30f), RGB(0.90f, 0.88f, 0.84f), RGB(0.74f, 0.64f, 0.70f)};
        m[MAT_FOREST] = F[(int)(unitFromHash(mix64(g.seed ^ 0xF0A6ULL)) * 4) % 4];
    }
    if (g.hasTrait(TR_HOT_SPRINGS)) m[MAT_MATS] = RGB(0.82f, 0.46f, 0.12f);
    if (g.hasTrait(TR_GLASSED)) {   // S-03: the sheets of pale green glass take the water family's colour (the globe, the maps and bank 2 read it); the ground round them is scorched
        RGB glass = lerp(RGB(0.50f, 0.66f, 0.52f), base, 0.2f), scorched(0.26f, 0.23f, 0.21f);
        m[MAT_GLASS] = glass; m[MAT_WATER] = glass;
        m[MAT_ROCK] = lerp(m[MAT_ROCK], scorched, 0.45f); m[MAT_DUST] = lerp(m[MAT_DUST], scorched, 0.45f); m[MAT_SAND] = lerp(m[MAT_SAND], scorched, 0.4f);
        m[MAT_BASALT] = lerp(m[MAT_BASALT], scorched, 0.3f); m[MAT_METAL] = lerp(m[MAT_METAL], scorched, 0.3f); m[MAT_GRAPHITE] = lerp(m[MAT_GRAPHITE], scorched, 0.2f);
    }
    if (g.hasTrait(TR_FROZEN_AIR)) {   // D-04: its air's frost on every stone (pale, a little blue), its snow the nitrogen's and the oxygen's white
        const RGB frost(0.80f, 0.84f, 0.90f), snow(0.90f, 0.92f, 0.96f);
        for (int mat : {MAT_ROCK, MAT_BASALT, MAT_DUST, MAT_SAND, MAT_SULPHUR, MAT_SALT, MAT_METAL, MAT_GRAPHITE, MAT_MUD, MAT_CRUST}) m[mat] = lerp(m[mat], frost, 0.35f);
        m[MAT_SNOW] = snow; m[MAT_ICE] = lerp(m[MAT_ICE], snow, 0.3f);
    }
}

// X-03 (GEN 12): a giant's character on its globe, hashed from its seed (the rng stream stays): its share of storms (most
// between a twentieth and a quarter of the storm cells, an ice giant's a few hundredths, a brown dwarf's a tenth or so), an
// ice giant's bands faint under its methane haze, and one giant in four a great storm (an ice giant's dark like a Neptune's,
// a brown dwarf's one in six): an oval of 10,000-24,000 km at 12-40 degrees of latitude, half to three quarters as tall as long
// P-07: a made giant's by its class: a water-cloud giant's storms many and towering (water's convection) under soft bands, an azure giant's
// faint and nearly stormless (no cloud to make them), a hot giant's few broad bands, a silicate giant's patchy clouds of rock, a gas dwarf's
// haze hiding all but a trace of them, a super-puff's faint; only water's convection builds a great storm
static void giantGlobe(const Body& b, BodyGen& g) {
    const int cls = KINDS[b.type].giantClass;
    const bool ice = isIceGiant(b), bd = kindHas(b.type, KT_GLOWS);
    const double u = unitFromHash(mix64(b.seed ^ 0x57A2ULL));
    g.stormShare = ice ? 0.02 + 0.04 * u : (bd ? 0.07 + 0.12 * u : 0.04 + 0.24 * std::pow(u, 1.3));
    g.bandContrast = ice ? 0.45 + 0.2 * unitFromHash(mix64(b.seed ^ 0x57A3ULL)) : 1.0;
    if (cls != GC_NONE) {
        static const double STORMS[GC_COUNT][2] = {{0, 0}, {0.12, 0.2}, {0.01, 0.02}, {0, 0}, {0.06, 0.1}, {0, 0}, {0, 0}};
        static const double BANDS[GC_COUNT][2] = {{1, 0}, {0.45, 0.2}, {0.25, 0.15}, {0.35, 0.15}, {0.5, 0.2}, {0.06, 0.06}, {0.12, 0.1}};
        g.stormShare = STORMS[cls][0] + STORMS[cls][1] * u;
        g.bandContrast = BANDS[cls][0] + BANDS[cls][1] * unitFromHash(mix64(b.seed ^ 0x57A3ULL));
    }
    const uint64_t h = mix64(b.seed ^ 0x6A57ULL);
    g.gsOn = unitFromHash(h) < (bd ? 0.17 : (cls == GC_NONE || cls == GC_WATER ? 0.25 : 0.0));
    if (!g.gsOn) return;
    const double aKm = std::min(5000 + 7000 * unitFromHash(mix64(h ^ 1)), 0.2 * b.radiusKm);
    g.gsA = aKm / b.radiusKm;
    g.gsB = g.gsA * (0.5 + 0.22 * unitFromHash(mix64(h ^ 2)));
    g.gsLat = (12 + 28 * unitFromHash(mix64(h ^ 3))) * DEG * (unitFromHash(mix64(h ^ 4)) < 0.5 ? -1 : 1);
    g.gsLon = (unitFromHash(mix64(h ^ 5)) - 0.5) * TAU;
    g.gsTone = ice || bd ? -1 : (unitFromHash(mix64(h ^ 6)) < 0.6 ? 1 : -1);
}

// P-00: the living worlds' flora colour, drawn after the kind's own draws
static RGB floraColour(Rng& rng) {
    double k = rng.uni();
    if (k < 0.55) return RGB(0.2f + 0.15f * (float)rng.uni(), 0.5f + 0.2f * (float)rng.uni(), 0.12f);
    else if (k < 0.7) return RGB(0.55f, 0.15f, 0.2f);          // red-purple flora
    else if (k < 0.82) return RGB(0.3f, 0.25f, 0.65f);         // blue-violet
    else if (k < 0.92) return RGB(0.7f, 0.6f, 0.15f);          // yellow-ochre
    else return RGB(0.15f, 0.55f, 0.6f);                       // teal
}

// Q-02: a new slot's trait that needs a fact of the world besides a process of its kind: the great tides a forcing (`Body::tideForcingM`), the
// great aurorae a strong field (R-402's class) and an air, the barrier reefs a warm sea (22 C at the equator's shore). Q-03: the singing dunes
// and the singing ice an air to carry them
// D-04: what a body in the dark lacks for the traits that need it: a sun (to sublime its ice into blades or its rock's volatiles, to shine through
// ice crystals or on a cold upper air's clouds, to warm and cool ice into song and valleys into fog) and the life a sun feeds (reefs, fungi,
// blooms, the salt lakes' and the glowing seas' microbes); where its air froze, what the air and a liquid open to it make (storms, haze,
// airglow, aurorae, singing sand, springs, mud, geysers); where its seas froze, the tides
constexpr uint64_t DARK_LACKS = KS_SUBLIMATION | KS_VOLATILES | KS_ICE_CRYSTALS | KS_MESOSPHERE | KS_ICE_SONG | KS_VALLEY_FOG | KS_CORALS | KS_FUNGI | KS_BLOOMS |
                                KS_HALOPHILES | KS_GLOWING_SEA;
constexpr uint64_t FROZEN_AIR_LACKS = KS_STORMS | KS_HAZE | KS_OXYGEN_AIR | KS_AURORAE | KS_SINGING_SAND | KS_SPRINGS | KS_MUD | KS_HYDROTHERMAL;
static bool frozenSeasOf(const Body& b) { return b.drifter && KINDS[b.type].seaFreezeK > 0 && b.tempK < KINDS[b.type].seaFreezeK; }
static uint64_t shapesOf(const Body& b) {
    uint64_t s = KINDS[b.type].shapes;
    if (b.drifter) s &= ~DARK_LACKS;
    if (airFrozen(b)) s &= ~FROZEN_AIR_LACKS;
    if (frozenSeasOf(b)) s &= ~KS_TIDES;
    return s;
}

int darkTraitsOf(const Body& b, int* out) {
    int n = 0;
    const int t = b.type;
    if (!b.drifter || !KINDS[t].landable) return 0;
    if (airFrozen(b)) out[n++] = TR_FROZEN_AIR;
    const bool rock = t == PT_ROCKY || t == PT_CRATERED || t == PT_METAL || t == PT_CARBON || t == PT_DESERT || t == PT_FROZEN_FELISIAN;   // a rock drifter (not ice)
    if (kindHas(t, KT_LAVA) || (b.parent < 0 && rock && unitFromHash(mix64(b.seed ^ 0xE3BE25ULL)) < EMBERS_SHARE)) out[n++] = TR_EMBERS;
    if (t == PT_DARK_OCEAN && unitFromHash(mix64(b.seed ^ 0x119A7E25ULL)) < LIGHTS_IN_WATER_SHARE) out[n++] = TR_LIGHTS_IN_WATER;
    if (drifterHadPeople(b) && peopleFirstAgeOf(b.seed) >= AGE_FUTURISTIC) out[n++] = TR_LIGHTS_LEFT_ON;   // D-05: a late people's works kept themselves lit
    return n;
}

static bool traitFits(const Body& b, const BodyGen& g, int t) {
    switch (t) {
        case TR_GREAT_TIDES: return b.tideForcingM >= TIDE_FORCING_MIN;
        case TR_GREAT_AURORAE: return magneticClass(magneticField(b)) == 3 && KINDS[b.type].pressureLo + KINDS[b.type].pressureSpan > 0.001;
        case TR_REEFS: return climateTempC(g, 0, 0, 0, 0) - (g.hasTrait(TR_SNOWBALL) ? 6.0 : 0.0) >= 22;
        case TR_SINGING_DUNES: case TR_SINGING_ICE: return KINDS[b.type].pressureLo + KINDS[b.type].pressureSpan > 0.001;
        default: return true;
    }
}

BodyGen BodyGen::make(const Body& b, double season) {
    BodyGen g;
    const KindDef& K = KINDS[b.type];
    g.type = b.type;
    g.seed = b.seed; g.worldSeed = b.seed;   // R-413
    g.R = b.radiusKm;
    g.season = K.cover != DC_NONE ? 0.0 : season;   // P-04: a dead world's ground keeps no season (its climate is its then-world's)
    g.starSx = b.starSx; g.starSy = b.starSy; g.starSz = b.starSz; g.starKnown = b.starKnown; g.bodyIndex = b.index;   // T-02
    // B-321: a living world's air keeps it 33 degrees over its equilibrium temperature (Earth's greenhouse), so an Earth-like
    // orbit gives an Earth-like climate; without it two thirds of the felisian land was ice sheet (`vesperis_test encounters`).
    // P-04: a dead world's the climate it had in its people's time (`Body::thenTempK`), its present's mean apart
    g.tempBias = ((b.thenTempK > 0 ? b.thenTempK : b.tempK) + K.greenhouseK - 288.0) * 0.3;
    if (K.cover != DC_NONE) g.nowTempC = (b.tempK - 273.15) * 0.75 + K.tempOffset;
    Rng rng(b.seed ^ 0xB0D7ULL);
    g.sA = rng.next(); g.sB = rng.next(); g.sC = rng.next(); g.sD = rng.next(); g.sE = rng.next(); g.sF = rng.next();
    g.sG = rng.next();
    g.color = b.color;
    g.roughness = 0.3 + 0.7 * rng.uni();
    g.craterDensity = 0.25 + 0.35 * rng.uni();
    g.craterDepth = 0.7 + 0.6 * rng.uni();
    g.cloudScale = 2.5 + 3.0 * rng.uni();
    g.plateK = 0.65 + 0.4 * rng.uni();   // 5-12 plates
    g.chainAmp = 1800 + 1800 * rng.uni();
    g.windAngle = rng.range(0, TAU);
    g.duneWl = 0.12 + 0.25 * rng.uni();
    g.duneStyle = rng.irange(3);
    g.tectonic = (K.tags & KT_PLATES) != 0;
    g.riverDensity = 0.4 + 0.5 * rng.uni();
    g.lakeDensity = 0.35 + 0.45 * unitFromHash(mix64(b.seed ^ 0x1A4EULL));   // B-320: hashed (the rng stream stays)
    g.floraFamily = (int)(unitFromHash(mix64(b.seed ^ 0xF10A)) * 10);    // hashed, not drawn: the rng stream stays as in GEN_VERSION 2; B-315: ten silhouettes
    g.floraFamily2 = (int)(unitFromHash(mix64(b.seed ^ 0xF10B)) * 10);
    if (g.floraFamily2 == g.floraFamily) g.floraFamily2 = (g.floraFamily + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10D)) * 9)) % 10;
    if (K.floraFamily >= 0) {   // P-02: the kind's own first silhouette (the tundra's conifers), the second kept apart from it
        g.floraFamily = K.floraFamily;
        if (g.floraFamily2 == g.floraFamily) g.floraFamily2 = (g.floraFamily + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10DULL)) * 9)) % 10;
    }
    g.latGradient = K.latGradient; g.seaDepth = K.seaDepth; g.grassBand = K.grassBand;   // P-02
    g.subContrast = K.subContrast;   // P-03
    g.locked = b.locked; g.lockedDir = b.lockedDir; g.apexDir = b.apexDir;   // P-06
    g.selfGlow = (K.tags & KT_GLOWS) ? clampd(b.luminosity / 0.003, 0.55, 1.0) : 0.0;
    // P-00: the kind's draws in its order (the stream is the type switch's of generation 15, field by field), then its colours: the
    // second tint, the sky's (three draws on a living world), the flora's (a living world's), a giant's globe
    for (int i = 0; i < K.nDraws; i++) {
        const GenDraw& d = K.draws[i];
        double v = d.kind == DK_CONST ? d.lo : (d.kind == DK_SYM ? rng.sym(d.span) : d.lo + d.span * rng.uni());
        switch (d.field) {
            case GF_SEA_LEVEL: g.seaLevel = v; break;
            case GF_MOUNTAIN: g.mountainAmp = v; break;
            case GF_HILL: g.hillAmp = v; break;
            case GF_ICE_CAP: g.iceCapLat = v; break;
            case GF_SNOW_LINE: g.snowLine = v; break;
            case GF_MOISTURE: g.moistureBias = v; break;
            case GF_CLOUD_COVER: g.cloudCover = v; break;
            case GF_LAVA_LEVEL: g.lavaLevel = v; break;
            case GF_VOLCANOES: g.volcanoes = v; break;
            case GF_CRATER_DENSITY: g.craterDensity = v; break;
            case GF_CRATER_DEPTH: g.craterDepth = v; break;
            case GF_BOULDERS: g.boulderDensity = v; break;
            case GF_DUNE_AMP: g.duneAmp = v; break;
            case GF_FRACTURE: g.fractureWidth = v; break;
            case GF_BAND_COUNT: g.bandCount = v; break;
            case GF_LIQUID_LEVEL: g.liquidLevel = v; break;
            case GF_SEA_REACH: g.seaReachDeg = v; break;
            case GF_ISLANDS: g.islandDensity = v; break;
            case GF_CONT_SCALE: g.contScale = v; break;
            default: break;
        }
    }
    if (K.color2.mode == TINT_CONST) g.color2 = K.color2.c;
    else if (K.color2.mode == TINT_BODY_SCALED) g.color2 = RGB(b.color.r * K.color2.c.r, b.color.g * K.color2.c.g, b.color.b * K.color2.c.b);
    switch (K.skyTint.mode) {
        case TINT_CONST: g.skyTint = K.skyTint.c; break;
        case TINT_THIN_AIR: { RGB c = b.color; g.skyTint = RGB(0.6f * c.b + 0.4f * c.r, 0.5f * c.g + 0.3f, 0.7f * c.r + 0.3f * c.b); break; }
        case TINT_DRAWN: g.skyTint = RGB(K.skyTint.c.r + K.skyTint.var.r * (float)rng.uni(), K.skyTint.c.g + K.skyTint.var.g * (float)rng.uni(), K.skyTint.c.b + K.skyTint.var.b * (float)rng.uni()); break;
        default: break;
    }
    if (K.tags & KT_FLORA) g.vegColor = floraColour(rng);
    if (K.tags & KT_GIANT) giantGlobe(b, g);
    {   // R-304: the traits, hashed (the rng stream stays as in generation 7): none for a fifth of the bodies, one for two
        // fifths, two for a bit over a quarter, three for the rest, drawn from the kind's list without repeats; one colour
        // trait at most, pangaea and archipelago never together (P-00: the traits' rows say which are landforms and which exclude which)
        auto tu = [&](int k) { return unitFromHash(mix64(b.seed ^ (0x7A17ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
        // D-04: a body in the dark by the processes it has there (`shapesOf`: no sun, perhaps no air); every other by its kind's
        const uint64_t shapes = shapesOf(b);
        auto eligible = [&](int t) { return TRAITS[t].cls != TC_GIVEN && (shapes & TRAITS[t].needs) != 0; };
        int eligL[TR_COUNT], eligW[TR_COUNT]; int nl = 0, nw = 0;   // the landform traits and the world traits, each in id order
        for (int t = 1; t < TR_COUNT; t++) if (eligible(t) && TRAITS[t].kind == TK_FIRST) { if (TRAITS[t].cls == TC_LANDFORM) eligL[nl++] = t; else eligW[nw++] = t; }   // Q-01: R-304's alone
        double u0 = tu(0);
        int n = u0 < 0.2 ? 0 : (u0 < 0.6 ? 1 : (u0 < 0.88 ? 2 : 3));
        int got = 0;
        for (int k = 0; k < 12 && got < n && nl + nw > 0; k++) {
            bool landform = nw == 0 || (nl > 0 && tu(30 + k) < 0.6);
            int t = landform ? eligL[(int)(tu(1 + k) * nl) % nl] : eligW[(int)(tu(1 + k) * nw) % nw];
            bool ok = true;
            for (int j = 0; j < got; j++) {
                int o = g.traits[j];
                if (o == t) ok = false;
                if (TRAITS[t].colour && TRAITS[o].colour) ok = false;
                if (TRAITS[t].excludes == o) ok = false;   // pangaea and archipelago, giant flora and dead forests
            }
            if (ok) g.traits[got++] = t;
        }
        if (K.favourTrait != TR_NONE && got < 3 && eligible(K.favourTrait) && unitFromHash(mix64(b.seed ^ 0xFA70ULL)) < K.favourChance) {   // P-02: a trait
            // the kind takes more often (the hothouse's giant flora), in a free slot where the drawn ones allow it
            const int t = K.favourTrait; bool ok = true;
            for (int j = 0; j < got; j++) { const int o = g.traits[j]; if (o == t || TRAITS[t].excludes == o || (TRAITS[t].colour && TRAITS[o].colour)) ok = false; }
            if (ok) g.traits[got++] = t;
        }
        if (b.glassed) {   // S-03: a survivor of its neutron star's supernova: the glass comes first and the exotic seas boiled off (the glass owns bank 2)
            int keep[3] = {0, 0, 0}, nk = 0;
            for (int k = 0; k < 3 && nk < 2; k++) if (g.traits[k] && g.traits[k] != TR_EXOTIC_SEAS) keep[nk++] = g.traits[k];
            g.traits[0] = TR_GLASSED; g.traits[1] = keep[0]; g.traits[2] = keep[1];
        }
        if (!b.wanderer && (b.drifter ? drifterHadPeople(b) : peoplesHash(b.type, b.seed))) {   // C-01: a people lived here (one world in eight of the kinds
            // a people may live on, by a hash of its own, so the other worlds keep their traits; P-04: a dead world's by its own; P-05: never on a
            // wanderer; D-05: a drifter's by its own, one frozen felisian or desert drifter in four): the trait goes first (after the glass), the third
            // trait drops
            int keep[3] = {0, 0, 0}, nk = 0;
            for (int k = 0; k < 3 && nk < 2; k++) if (g.traits[k] && g.traits[k] != TR_CIVILISATION) keep[nk++] = g.traits[k];
            if (keep[0] == TR_GLASSED) { g.traits[0] = TR_GLASSED; g.traits[1] = TR_CIVILISATION; g.traits[2] = keep[1]; }
            else { g.traits[0] = TR_CIVILISATION; g.traits[1] = keep[0]; g.traits[2] = keep[1]; }
            if (K.tags & KT_DEAD_SEAS) {   // the post-apocalyptic desert: its seas stood at a level of their own and its rivers ran to them
                g.oldSeaM = -120.0 + 220.0 * tu(22);
                g.riverDensity = std::max(g.riverDensity, 0.8);
            }
        }
        {   // Q-01: the three new slots, drawn by hashes of their own after the first three (which stay as R-304 drew them): how many as R-304's
            // (none for a fifth of the worlds, one for two fifths, two for a bit over a quarter, three for the rest), each from a kind of trait of its
            // own that fits the world (the kinds in an order hashed for it, the first that hold a trait the slots before allow: one colour trait at
            // most, no trait with one it excludes), the trait from its kind by a hash; a world with a people takes none that moves its ground (its
            // towns stand where 1.3.2 put them), nor the home world (where every expedition starts: its ground as the explorer first knew it)
            auto nu = [&](int k) { return unitFromHash(mix64(b.seed ^ (0x51075ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
            const double u = nu(0);
            // D-04: a body in the dark takes its own traits first (`darkTraitsOf`), the draw the slots they leave
            int own[3]; const int nOwn = darkTraitsOf(b, own);
            for (int k = 0; k < nOwn; k++) g.traits[3 + k] = own[k];
            const int want = std::min(u < 0.2 ? 0 : (u < 0.6 ? 1 : (u < 0.88 ? 2 : 3)), 3 - nOwn);
            const bool home = b.starKnown && b.starSx == HOME_SX && b.starSy == HOME_SY && b.starSz == HOME_SZ && b.parent < 0 && b.type == PT_FELISIAN;   // the game's start (`Game::newGame`)
            const bool kept = home || (!b.wanderer && (b.drifter ? drifterHadPeople(b) : peoplesHash(b.type, b.seed)));   // its ground stays (D-05: a drifter's people's too)
            int order[TK_COUNT]; double key[TK_COUNT]; int nk = 0;
            for (int k = TK_LANDFORM; k < TK_DARK; k++) {   // the kinds in the world's order (an insertion by their hashes; D-04: the dark's own are given)
                const double v = nu(1 + k); int i = nk++;
                while (i > 0 && key[i - 1] > v) { order[i] = order[i - 1]; key[i] = key[i - 1]; i--; }
                order[i] = k; key[i] = v;
            }
            for (int i = 0, got = 0; i < nk && got < want; i++) {
                int elig[TR_COUNT], n = 0;
                for (int t = TR_CIVILISATION + 1; t < TR_COUNT; t++) {
                    if (TRAITS[t].kind != order[i] || (kept && TRAITS[t].moves) || !eligible(t) || !traitFits(b, g, t)) continue;
                    bool ok = true;
                    for (int j = 0; j < TRAIT_SLOTS && ok; j++) { const int o = g.traits[j]; if (o && (o == t || TRAITS[t].excludes == o || TRAITS[o].excludes == t || (TRAITS[t].colour && TRAITS[o].colour))) ok = false; }
                    if (ok) elig[n++] = t;
                }
                if (n > 0) g.traits[3 + nOwn + got++] = elig[(int)(nu(10 + order[i]) * n) % n];
            }
        }
        for (int k = 0; k < TRAIT_SLOTS; k++) switch (g.traits[k]) {
            case TR_ARCHIPELAGO: g.seaLevel += 0.3; g.islandDensity = std::max(g.islandDensity, 0.5); break;   // P-02: a kind's own islands may be more
            case TR_PANGAEA: g.contScale = 1.8; g.seaLevel -= 0.05; break;
            case TR_LAKELAND: g.lakeDensity = 0.85; g.riverDensity = 0.9; break;
            case TR_SNOWBALL: g.iceCapLat = 28 + 12 * tu(20); g.snowLine *= 0.55; g.tempBias -= 6; break;
            case TR_EXOTIC_SEAS: g.liquidLevel = K.exoticLo + K.exoticSpan * tu(21); break;   // tar, brine, methane
            case TR_STORMS: g.cloudCover = std::min(1.0, g.cloudCover + 0.35); break;
            case TR_DEAD_FOREST: g.vegColor = lerp(g.vegColor, RGB(0.45f, 0.42f, 0.35f), 0.65f); break;
            case TR_GREAT_TIDES: g.tideRangeM = tideRangeOf(b.tideForcingM); break;   // Q-02
            case TR_PINK_LAKES: { static const RGB P[3] = {RGB(0.92f, 0.52f, 0.70f), RGB(0.86f, 0.38f, 0.48f), RGB(0.80f, 0.30f, 0.34f)}; g.waterTintColour = P[(int)(tu(23) * 3) % 3]; break; }   // Hillier's, Retba's, Natron's
            case TR_MILKY_SEAS: g.waterTintColour = RGB(0.66f, 0.80f, 0.90f); break;
            case TR_BLOOMS: { static const RGB B[3] = {RGB(0.58f, 0.20f, 0.13f), RGB(0.26f, 0.52f, 0.20f), RGB(0.62f, 0.38f, 0.14f)}; g.lifeTintColour = B[(int)(tu(24) * 3) % 3]; break; }   // a red tide, a green, a brown
            case TR_REEFS: g.lifeTintColour = RGB(0.22f, 0.70f, 0.72f); break;   // the lagoons' turquoise
            case TR_LIGHTS_IN_WATER: { static const RGB L[3] = {RGB(0.30f, 0.86f, 0.80f), RGB(0.36f, 0.62f, 0.95f), RGB(0.52f, 0.92f, 0.46f)}; g.waterTintColour = L[(int)(tu(25) * 3) % 3]; break; }   // D-04: a blue-green, a blue, a green
            default: break;
        }
    }
    if (b.drifter) {   // D-04: in the dark; its liquid frozen at its level where it is colder than its kind's (a dark ocean's never: its heat is its sea's)
        g.dark = true; g.drifter = true;   // D-05
        g.frozenSeas = frozenSeasOf(b) && (kindHas(b.type, KT_SEA) || kindHas(b.type, KT_WET) || g.liquidLevel > -1e8);
    }
    {   // N2-05: the second family's colour, a related hue from the family palettes (hashed, the rng stream untouched)
        static const RGB pal[5] = {RGB(0.28f, 0.6f, 0.12f), RGB(0.55f, 0.15f, 0.2f), RGB(0.3f, 0.25f, 0.65f), RGB(0.7f, 0.6f, 0.15f), RGB(0.15f, 0.55f, 0.6f)};
        RGB base = (g.vegColor.r + g.vegColor.g + g.vegColor.b > 0.05f) ? g.vegColor : RGB(0.25f, 0.5f, 0.15f);
        int nearest = 0; float bestD = 1e9f;
        for (int k = 0; k < 5; k++) { float d = (pal[k].r - base.r) * (pal[k].r - base.r) + (pal[k].g - base.g) * (pal[k].g - base.g) + (pal[k].b - base.b) * (pal[k].b - base.b); if (d < bestD) { bestD = d; nearest = k; } }
        RGB other = pal[(nearest + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10CULL)) * 4)) % 5];   // a different family palette
        g.vegColor2 = lerp(base, other, 0.6f);
        if (g.hasTrait(TR_FUNGAL)) {   // Q-02: the second family's caps another pale (by its own hash)
            static const RGB F2[4] = {RGB(0.92f, 0.84f, 0.70f), RGB(0.70f, 0.50f, 0.32f), RGB(0.84f, 0.82f, 0.86f), RGB(0.86f, 0.62f, 0.56f)};
            g.vegColor2 = F2[(int)(unitFromHash(mix64(b.seed ^ 0xF0A7ULL)) * 4) % 4];
        }
    }
    {   // O6-02: the relief spectrum's parameters, hashed (the rng stream stays as in generation 5); the amplitudes scale
        // with the base wavelength, which a small body caps, so a comet's relief is a small world's
        auto hu = [&](int k) { return unitFromHash(mix64(b.seed ^ (0x0616ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
        g.reliefWl0 = std::min(12.0 + 8.0 * hu(1), b.radiusKm * 0.25);
        g.reliefH = 1.04 + 0.1 * hu(2);
        g.reliefRidge = 0.35 + 0.4 * hu(3);
        g.reliefWarp = 0.2 + 0.25 * hu(4);
        g.reliefErosion = 2.5 + 3.0 * hu(5);
        g.reliefHybrid = 0.4 + 0.4 * hu(6);
        // P-00: the kind's grades (KindRelief): its ridge changed, its low ground's smoothing, metres per 16 km of base wavelength on the
        // plains, the hills and the mountains (before the ridge normalisation), its cliff bands scaled (a shell of ice and a soft world
        // have fewer and lower ones; a nucleus none)
        const KindRelief& KR = K.relief;
        double k = 1000.0 * g.reliefWl0 / 16.0, flat = KR.flat, hill = KR.hill, mtn = KR.mtn;
        switch (KR.ridgeOp) {
            case RO_ADD: g.reliefRidge += KR.ridge; break;
            case RO_MUL: g.reliefRidge *= KR.ridge; break;
            case RO_SET: g.reliefRidge = KR.ridge; break;
            default: break;
        }
        if (KR.hybrid >= 0) g.reliefHybrid = KR.hybrid;
        g.reliefRidge = clampd(g.reliefRidge, 0, 1);
        g.reliefFlat = flat * k; g.reliefHill = hill * k; g.reliefMtn = mtn * k;
        // O6-04: the cliff bands
        g.cliffStep = 60 + 70 * hu(7);
        g.cliffAmount = 0.45 + 0.4 * hu(8);
        if (KR.cliffAmount != 1) g.cliffAmount *= KR.cliffAmount;
        if (KR.cliffStep != 1) g.cliffStep *= KR.cliffStep;
    }
    materialPalette(g);   // N1-01
    return g;
}

double iceSheetEdgeDeg(const BodyGen& g, const Vec3& unit) { return g.iceCapLat + 6.0 * gnoise3(unit * g.R / (g.R * 0.15), g.sF + 7); }   // `felisianSample`'s cap edge without its season

double iceSheetIn(const BodyGen& g, const Vec3& unit) {
    const double absLatDeg = std::fabs(std::asin(clampd(unit.z, -1, 1))) / DEG;
    if (KINDS[g.type].sheetByCold && g.locked) return (-4.0 - climateTempC(g, absLatDeg, 0, 0, dot(unit, g.lockedDir)) + 7.0 * gnoise3(unit * g.R / (g.R * 0.15), g.sF + 7)) / 25.0;   // its edge wandering as the tundra's
    return (absLatDeg - iceSheetEdgeDeg(g, unit)) / 14.0;
}

double climateTempC(const BodyGen& g, double absLatDeg, double heightM, double winter, double sub) {
    // B-321: Earth's mean temperature against latitude runs 27 C at the equator, 15 at 35 degrees, 5 at 50, -2 at 60,
    // -9 at 70: `-18 + 46 cos(lat)^1.5`; the old line `28 - 0.55 lat` was five degrees too cold in the middle latitudes
    double c = std::cos(absLatDeg * DEG);
    double L = -18.0 + 46.0 * c * std::sqrt(c);
    if (g.latGradient != 1) L = 15.0 + (L - 15.0) * g.latGradient;   // P-02: a kind's contrast round Earth's mean (a hothouse's mild poles)
    double T = L - 6.5 * std::max(0.0, heightM) / 1000.0 + g.tempBias - 6.0 * winter;
    if (g.locked) T += g.subContrast * sub - 6.0 + 0.35 * absLatDeg;   // M5-08: a permanent day side and night side; the twilight belt between keeps the mild climate (P-03: the kind's contrast, 45 degrees but a terminator world's)
    return T;
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
                    LandFeature F; F.unit = at; F.radiusKm = C.rKm[k]; F.heightM = -C.depthM[k]; F.id = mix64(hash3i((int64_t)std::llround(C.c.x * 8), (int64_t)std::llround(C.c.y * 8), (int64_t)std::llround(C.c.z * 8), G.seed) + (uint64_t)k);
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

namespace {
// R-305: the landform library. `landformsAt` adds the traits' landforms at a point: `dhSmooth` moves the smooth ground
// and the ground alike (canyons, mesas, rifts, scarps, basins...: rivers and lakes are cut into the result), `dhFine`
// the ground alone (spires, polygons, terraces, crevasses), `flatten` melts the ground into the smooth ground by that
// fraction, `flatOffset` metres off it (playas, glaciers); `mat`/`albedo` override the material where set. Every term
// fades with the sampling scale (`lodFade`, `scaleVisible`) so the map, the far rings and the ground agree
struct LandCtx { double landMask = 1, moist = 0.15, relief0 = 0, h = 0, hS = 0, T = 15, hM = 0, slope = 0; };   // B-404: hM = the ground on the material chain's relief, slope = that chain's slope (the same in every ring)
struct LandOut { double dhSmooth = 0, dhFine = 0, flatten = 0, flatOffset = 0; int mat = -1; double albedo = -1; double glacier = 0, riftFloor = 0; double mark = 0; int markTrait = 0; double tint = 0, life = 0, slip = 0; };   // Q-02: tint, the ground trait's field (`SurfaceSample::groundTint`); life, the life trait's on the land (a petrified forest's)   // mark: 1 where the landform is at its most distinctive (the galleries); Q-01: markTrait, the trait that set it
void landformsAt(const BodyGen& g, const Vec3& p, const Vec3& unit, double detailM, const LandCtx& c, LandOut& o) {
    if (!g.anyTrait()) return;
    const double R = g.R;
    // a province: about a third of the world, its own for every landform
    auto prov = [&](int k, double lo, double hi) { return smoothstep(lo, hi, fbm3(p / (R * 0.22) + Vec3(31.7 * k, 11.3 * k, 7.9 * k), g.sE + 200 + k, 2) * lodFade(R * 0.22, detailM)); };
    auto tu = [&](int k) { return unitFromHash(mix64(g.seed ^ (0x1A4DULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
    auto setMat = [&](int m, double a) { o.mat = m; o.albedo = a; };
    double land = c.landMask;
    for (int ti = 0; ti < TRAIT_SLOTS; ti++) {
        int t = g.traits[ti];
        if (!t) { if (ti < 3) ti = 2; continue; }   // Q-01: R-304's slots end at their first empty one; the new slots' own after them
        const double mark0 = o.mark; o.mark = -1;   // Q-01: which trait set the mark (the last to, as before)
        switch (t) {
        case TR_CANYONS: {
            // a province of dry, flat, raised ground incised by a network of canyons (worley boundaries at 20 km, 0.6-1.3 km
            // wide, 150-600 m deep) and their side canyons (5 km, a third as wide and deep); the walls step down in three terraces
            double mask = land * prov(1, 0.1, 0.4) * smoothstep(0.45, 0.25, c.moist) * smoothstep(0.4, 0.2, c.relief0) * smoothstep(30, 150, c.hS);
            double w1 = 0.3 + 0.35 * tu(2), w2 = 0.1 + 0.12 * tu(3);   // the half-widths, km
            if (mask < 0.01 || (featFade(w1 * 2, detailM) <= 0 && !matVisible(w1, detailM))) break;   // B-404: by the canyons' own width (the 512 m ring draws them, coarsely; it was `scaleVisible(0.3)`: nothing beyond the 64 m ring)
            double depth = (150 + 450 * tu(1)) * mask;
            double wallness = 0;
            auto cut = [&](double cellKm, uint64_t seed, double width, double dep) {
                double f = featFade(width * 2, detailM);
                bool mv = matVisible(width, detailM);   // the walls' rock wherever a side spans half a cell
                if (f <= 0 && !mv) return 0.0;
                Worley3 w = worley3(p / cellKm, seed);
                double d = (w.f2 - w.f1) * cellKm * 0.5;   // km from the boundary
                double tt = clampd(1 - d / width, 0, 1);    // 1 at the centre line
                double s = tt * 3, k = std::floor(s), fr = s - k;
                double prof = tt >= 1 ? 1.0 : (k + smoothstep(0.25, 0.75, fr)) / 3;
                if (mv && prof > 0.03 && prof < 0.97) wallness = std::max(wallness, 1.0);
                return -dep * prof * f;
            };
            o.dhSmooth += cut(20.0, g.sE + 210, w1, depth) + cut(5.0, g.sE + 211, w2, depth * 0.4);
            if (wallness > 0 && mask > 0.5) { setMat(MAT_ROCK, 0.42); o.mark = 1; }
            break;
        }
        case TR_MESAS: {
            // badlands: flat-topped mesas 0.8-1.6 km across and 40-160 m high on half the cells of a 1.4 km grid, buttes and
            // hoodoos on a sixth, steep rims with a talus foot, in a dry flat province
            double mask = land * prov(2, 0.1, 0.35) * smoothstep(0.5, 0.3, c.moist) * smoothstep(0.4, 0.15, c.relief0);
            if (mask < 0.01 || (featFade(1.1, detailM) <= 0 && !matVisible(0.11, detailM))) break;   // B-404: by the biggest cap's width and its rim
            Worley3 w = worley3(p / 1.4, g.sE + 220);
            double u = unitFromHash(mix64(w.id1 + 7));
            double dh = 0; bool rim = false;
            if (u < 0.5) {
                double capH = 60 + 160 * unitFromHash(mix64(w.id1 + 8)), rad = 0.3 + 0.25 * unitFromHash(mix64(w.id1 + 9));
                double x = w.f1 / rad;
                double prof = x < 0.8 ? 1.0 : (x < 1.0 ? 1 - 0.85 * smoothstep(0.8, 1.0, x) : 0.15 * std::max(0.0, 1 - (x - 1) / 0.5));
                dh = capH * prof * featFade(2 * rad, detailM);                              // a cap's own fade
                rim = prof > 0.05 && prof < 0.95 && matVisible(0.2 * rad, detailM);   // the steep rim, a fifth of the radius wide
            } else if (u < 0.66) {
                double capH = 30 + 90 * unitFromHash(mix64(w.id1 + 8)), rad = 0.06 + 0.06 * unitFromHash(mix64(w.id1 + 9));
                double x = w.f1 / rad;
                double prof = x < 0.7 ? 1.0 : (x < 1 ? 1 - 0.9 * smoothstep(0.7, 1.0, x) : 0.1 * std::max(0.0, 1 - (x - 1) / 0.6));
                dh = capH * prof * featFade(2 * rad, detailM);
                rim = prof > 0.05 && prof < 0.95 && matVisible(0.3 * rad, detailM);
            }
            o.dhSmooth += dh * mask;
            if (rim && mask > 0.5) { setMat(MAT_ROCK, 0.4); o.mark = 1; }
            break;
        }
        case TR_KARST: {
            // tower karst: steep towers 50-210 m tall and 70-250 m across, half the cells of a 500 m grid, in warm wet
            // lowlands; sinkholes 60-160 m across between them
            double mask = land * prov(3, 0.1, 0.4) * smoothstep(0.45, 0.6, c.moist) * smoothstep(8, 16, c.T) * smoothstep(0.35, 0.15, c.relief0) * smoothstep(5, 30, c.hS) * smoothstep(900, 500, c.hS);
            if (mask < 0.01 || (featFade(0.25, detailM) <= 0 && !matVisible(0.07, detailM))) break;   // B-404: by the biggest tower and its walls
            double dh = 0; bool wall = false;
            visitFeatures(p, R, 0.5, 0.55, g.sE + 230, 0.5, [&](const FeatureHit& f) {
                double r = 0.035 + 0.09 * f.u[3], x = f.d / r;
                if (x >= 1.15) return;
                double hgt = 50 + 160 * f.u[4];
                double prof = x < 1 ? (1 - smoothstep(0.62, 1.0, x)) * (0.75 + 0.25 * std::cos(x * PI * 0.5)) : 0.0;   // a rounded top, walls over the outer 38%
                prof += 0.06 * clampd((1.15 - x) / 0.15, 0, 1) * (x > 1 ? 1 : 0) + (x < 1 ? 0.06 : 0);
                dh += hgt * prof * featFade(2 * r, detailM);   // a tower's own fade
                if (x > 0.5 && x < 1.05 && matVisible(0.55 * r, detailM)) wall = true;   // the walls, the outer 55% of the radius
            });
            visitFeatures(p, R, 0.45, 0.3, g.sE + 231, 0.4, [&](const FeatureHit& f) {
                double r = 0.03 + 0.05 * f.u[3], x = f.d / r;
                if (x >= 1) return;
                dh -= (8 + 25 * f.u[4]) * (1 - x * x) * (1 - x * x) * featFade(2 * r, detailM);
            });
            o.dhSmooth += dh * mask;
            if (wall && mask > 0.5) { setMat(MAT_ROCK, 0.45); o.mark = 1; }
            break;
        }
        case TR_RIFT: {
            // one great rift: the zero line of a broad noise; a flat floor 600-1500 m down between stepped walls, with
            // raised shoulders; the floor (`riftFloor`) takes lakes on living worlds
            double f = lodFade(R * 0.1, detailM);
            double n = gnoise3(p / (R * 0.12) + Vec3(3.1, 7.7, 1.3), g.sE + 240);
            double W = 0.02 + 0.015 * tu(4);
            double a = std::fabs(n);
            double depth = (600 + 900 * tu(5)) * land;
            double floor = smoothstep(W, W * 0.5, a);
            double s = floor * 3, k = std::floor(s), fr = s - k;
            double stepped = floor >= 1 ? 1.0 : (k + smoothstep(0.2, 0.8, fr)) / 3;   // three fault steps down each wall
            double shoulder = smoothstep(W * 2.2, W * 1.15, a) * smoothstep(W * 0.9, W * 1.15, a);
            o.dhSmooth += (-depth * stepped + 250 * land * shoulder) * f;
            double fl = smoothstep(W * 0.5, W * 0.3, a) * land * f;
            o.riftFloor = std::max(o.riftFloor, fl);
            if (fl > 0.5) { o.flatten = std::max(o.flatten, fl); o.flatOffset = -4; }
            if (stepped > 0.05 && stepped < 0.95) { setMat(MAT_ROCK, 0.4); o.mark = land; }
            break;
        }
        case TR_ESCARPMENTS: {
            // one or two cliff lines thousands of kilometres long: a step of 150-600 m across the zero line of a broad noise,
            // a talus foot then the cliff over 300 m (or the sampling scale), the height wandering along the line
            int nS = 1 + (tu(6) < 0.5 ? 1 : 0);
            bool cliff = false;
            for (int k = 0; k < nS; k++) {
                double wl = R * 0.2;
                double n = gnoise3(p / wl + Vec3(11.3 * k + 2.2, 5.1, 9.7 * k), g.sE + 250 + k);
                double stepH = (150 + 450 * tu(7 + k)) * land;
                double w = std::max(0.0003, 1.5 * detailM / (wl * 1000.0));
                double tt = clampd((n + w) / (2 * w), 0, 1);
                double prof = tt < 0.35 ? 0.15 * (tt / 0.35) * (tt / 0.35) : 0.15 + 0.85 * (tt - 0.35) / 0.65;
                double along = 0.7 + 0.3 * gnoise3(p / (wl * 0.15), g.sE + 260 + k);
                o.dhSmooth += stepH * prof * along;
                if (tt > 0.05 && tt < 0.95) cliff = true;
            }
            if (cliff) { setMat(MAT_ROCK, 0.42); o.mark = land; }
            break;
        }
        case TR_GLACIAL: {
            // ice fills the valleys of the cold high country: a tongue six metres under the smooth ground where the ground
            // lies below it, crevassed (`glacier` sets the ice); fjords come from the river layer on living worlds
            double cold = smoothstep(2, -5, c.T) * smoothstep(0.15, 0.4, c.relief0) * land;
            if (cold < 0.01) break;
            double glac = cold * smoothstep(-8, 25, c.hS - c.hM);   // B-404: the valley's depth on the material chain, the same in every ring
            if (glac < 0.02 || !matVisible(0.5, detailM)) break;     // a tongue is half a kilometre wide: the rings to 512 m carry it, the far ring and the map read the smooth ground alone
            if (scaleVisible(0.04, detailM)) { Worley3 w = worley3(p / 0.05, g.sE + 270); o.dhFine -= 3.0 * (1 - smoothstep(0.02, 0.08, w.f2 - w.f1)) * glac; }
            o.flatten = std::max(o.flatten, glac); o.flatOffset = -6;
            o.glacier = std::max(o.glacier, glac);
            if (glac > 0.5) o.mark = glac;
            break;
        }
        case TR_INSELBERGS: {
            // lone steep mountains 150-500 m tall and 0.8-3 km across on the plains, one per 25 km cell in three
            double mask = land * smoothstep(0.35, 0.15, c.relief0);
            if (mask < 0.01 || (featFade(3.2, detailM) <= 0 && !matVisible(1.1, detailM))) break;   // B-404: by the biggest mountain (the 512 m ring draws the big ones; `lodFade(2.0)` drew none beyond the 64 m ring)
            double dh = 0; bool rock = false;
            visitFeatures(p, R, 25.0, 0.3, g.sE + 280, 0.12, [&](const FeatureHit& f) {
                double r = 0.4 + 1.2 * f.u[3], x = f.d / r;
                if (x >= 1.2) return;
                double hgt = 150 + 350 * f.u[4];
                double prof = x < 1 ? smoothstep(1.0, 0.5, x) * (0.7 + 0.3 * std::cos(x * PI * 0.5)) + 0.06 : 0.06 * (1.2 - x) / 0.2;
                dh += hgt * prof * featFade(2 * r, detailM);   // a mountain's own fade
                if (x > 0.35 && x < 1.05 && matVisible(0.7 * r, detailM)) rock = true;
            });
            o.dhSmooth += dh * mask;
            if (rock && mask > 0.5) { setMat(MAT_ROCK, 0.42); o.mark = 1; }
            break;
        }
        case TR_CINDER_FIELD: {
            // a volcanic province: cinder cones 140 m tall on a 2.5 km grid, fresh lava sheets on a third of the 5 km cells
            // (dark, ropy), strings of collapse pits along fissures
            double mask = land * prov(4, 0.25, 0.5);
            if (mask < 0.01) break;
            double dh = 0; bool dark = false;
            if (featFade(1.0, detailM) > 0 || matVisible(1.0, detailM)) {   // B-404: the cones by their width (0.6-1.5 km), the dark cinder wherever one spans half a cell
                bool cal = false;
                double cones = volcanoField(p, R, 2.5, 0.45, g.sE + 290, 140.0, cal);
                dh += cones * featFade(1.0, detailM);
                if (cones > 15 && matVisible(1.0, detailM)) dark = true;
            }
            if (featFade(5.0, detailM) > 0 || matVisible(5.0, detailM)) {   // the sheets: cells of 5 km
                Worley3 w = worley3(p / 5.0, g.sE + 291);
                if (unitFromHash(mix64(w.id1 + 3)) < 0.35) {
                    double edge = smoothstep(0.0, 0.12, w.f2 - w.f1);
                    double ropy = scaleVisible(0.02, detailM) ? 1.5 * ridged3(p / 0.03, g.sE + 292, 2) : 0.0;
                    dh += (6.0 + ropy) * edge * featFade(5.0, detailM);
                    if (edge > 0.5 && matVisible(5.0, detailM)) dark = true;
                }
            }
            if (scaleVisible(0.9, detailM)) {   // the pit chains, 144 m across
                Worley3 w = worley3(p / 0.9, g.sE + 293);
                if (unitFromHash(mix64(w.id1 + 5)) < 0.15 && w.f1 < 0.08) dh -= 12.0 * (1 - w.f1 / 0.08) * (1 - w.f1 / 0.08) * featFade(0.144, detailM);
            }
            o.dhSmooth += dh * mask;
            if (dark && mask > 0.5) { setMat(MAT_BASALT, 0.15); o.mark = 1; }
            break;
        }
        case TR_CHAOS: {
            // the crust broken into tilted blocks 2-6 km across, lifted or dropped by up to 80 m, cracks 90 m deep between
            double f = featFade(4.0, detailM), fc = featFade(0.24, detailM);   // B-404: the blocks by their size, the cracks (240 m wide, 90 m deep) by theirs
            bool mv = matVisible(0.24, detailM);
            if (f <= 0 && !mv) break;
            Worley3 w = worley3(p / 4.0, g.sE + 300);
            uint64_t id = w.id1;
            double lift = (unitFromHash(mix64(id + 1)) - 0.5) * 160;
            Vec3 tilt = normalize(Vec3(unitFromHash(mix64(id + 2)) - 0.5, unitFromHash(mix64(id + 3)) - 0.5, unitFromHash(mix64(id + 4)) - 0.5));
            double slope = 0.02 + 0.05 * unitFromHash(mix64(id + 5));
            double dh = lift + dot(p - w.c1 * 4.0, tilt) * slope * 1000.0;
            double crack = 1 - smoothstep(0.03, 0.09, w.f2 - w.f1);
            o.dhSmooth += (dh * f - 90 * crack * fc) * land;
            if (crack > 0.5 && mv) { setMat(MAT_ROCK, 0.3); o.mark = 1; }
            break;
        }
        case TR_POLYGONS: {
            // patterned ground: a honeycomb of low ridges 40-120 m across on cold flat ground
            double cold = kindHas(g.type, KT_FROZEN) ? 1.0 : smoothstep(4, -2, c.T);
            double mask = cold * land * smoothstep(0.25, 0.1, c.relief0);
            double cell = 0.04 + 0.08 * tu(8);
            if (mask < 0.01 || !scaleVisible(cell, detailM)) break;
            Worley3 w = worley3(p / cell, g.sE + 310);
            double ridge = 1 - smoothstep(0.0, 0.006, (w.f2 - w.f1) * cell);
            o.dhFine += (1.5 * ridge * ridge - 0.4 * (1 - ridge)) * mask * lodFade(cell, detailM);
            if (mask > 0.5) o.mark = mask;
            break;
        }
        case TR_YARDANGS: {
            // wind-carved ridges along the wind, 80-200 m apart and 5-20 m high, broken along their length, in the dry flats
            double mask = land * prov(5, 0.1, 0.4) * smoothstep(0.3, 0.16, c.moist) * smoothstep(0.3, 0.1, c.relief0);
            if (mask < 0.01) break;
            double wl = 0.08 + 0.12 * tu(9);
            double f = lodFade(wl, detailM);
            if (f <= 0) break;
            Vec3 dd = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.1));
            Vec3 dp = normalize(cross(dd, unit));
            double across = dot(p, dp) / wl * TAU + 2.0 * gnoise3(p / 1.5, g.sE + 320);
            double alongN = smoothstep(0.25, 0.75, 0.5 + 0.5 * gnoise3(p / 0.35 + Vec3(0, 0, 9), g.sE + 321));   // the ridges break into segments a few hundred metres long
            o.dhFine += (5 + 15 * tu(10)) * std::pow(0.5 + 0.5 * std::cos(across), 3.0) * alongN * mask * f;
            if (mask > 0.5) o.mark = mask;
            break;
        }
        case TR_ERG: {
            // a sea of giant dunes: draa 1-2.5 km apart and 40-120 m high across the wind in the dry lowlands, sand
            double mask = land * smoothstep(0.34, 0.2, c.moist) * smoothstep(0.35, 0.15, c.relief0) * smoothstep(900, 400, c.hS);
            if (mask < 0.01) break;
            if (mask > 0.5) { setMat(MAT_SAND, livingGround(g.type) ? 0.6 : -1.0); o.mark = mask; }   // B-404: the sand sea is kilometres across: sand in every ring and on the maps, whether or not its draa can show (the sand used to go with the draa: dust beyond the 64 m ring); an airless or dry type keeps its own albedo under it, so its globe keeps its mottling (a flat 0.6 made a dune-sea world one yellow ball)
            double wl = 1.0 + 1.5 * tu(11), amp = 40 + 80 * tu(12);
            double f = lodFade(wl, detailM);   // the draa are a periodic pattern: the octave rule (KI-311)
            if (f <= 0) break;
            Vec3 dd = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.15 * std::sin(g.windAngle * 2)));
            Vec3 dp = normalize(cross(dd, unit));
            double along = dot(p, dd) / wl * TAU, across = dot(p, dp) / wl * TAU;
            double bend = fbm3(p / 8.0, g.sE + 330, 2) * 3.0;
            double v = std::pow(0.5 + 0.5 * std::sin(along + bend + 0.5 * std::sin(across * 0.7)), 1.6);
            o.dhSmooth += amp * v * mask * f;
            o.slip = std::max(o.slip, mask * f * smoothstep(-0.1, -0.5, std::cos(along + bend + 0.5 * std::sin(across * 0.7))));   // Q-03: the draa's faces downwind
            break;
        }
        case TR_SALT_FLATS: {
            // white playas in the closed basins of dry country: dead flat three metres under the smooth ground, cracked into
            // polygons a dozen metres across
            if (!matVisible(2.5, detailM)) break;   // B-404: the playas (a few kilometres across, the lows of the 5 km basin noise) unfaded wherever one spans half a cell: every ring carries them, the world map does not (the noise would alias on its texels)
            double basin = smoothstep(-0.2, -0.45, fbm3(p / 5.0, g.sB, 3));
            double mask = land * basin * smoothstep(0.34, 0.2, c.moist) * smoothstep(0.25, 0.1, c.relief0) * smoothstep(700, 300, c.hS);
            if (mask < 0.01) break;
            o.flatten = std::max(o.flatten, mask); o.flatOffset = -3;
            if (scaleVisible(0.012, detailM)) { Worley3 w = worley3(p / 0.012, g.sE + 340); o.dhFine += 0.15 * (1 - smoothstep(0.0, 0.5, (w.f2 - w.f1) * 12)) * mask; }   // the cracks: polygons of 12 m, the 4 m ring's (B-404: gated at 5 m before, under every ring)
            if (mask > 0.6) { setMat(MAT_SALT, 0.9); o.mark = mask; }
            break;
        }
        case TR_TRAPS: {
            // stepped terraces of old lava sheets: the ground quantised into steps of 40-90 m with steep risers, in a province
            double mask = land * prov(6, 0.2, 0.45);
            if (mask < 0.01) break;
            double step = 40 + 50 * tu(13);
            double run = step / std::max(c.slope, 0.02);        // B-404: a terrace's run in metres on the material chain's slope
            double f = featFade(run / 1000.0, detailM);          // the relief where a terrace spans a few samples (it was `lodFade(1.5)`: none beyond the 64 m ring)
            bool mv = matVisible(0.4 * run / 1000.0, detailM);   // the riser, 40% of the run, decides the basalt wherever it spans half a cell
            if (f <= 0 && !mv) break;
            double q = std::floor(c.h / step) * step, fr = (c.h - q) / step;
            double terr = q + step * smoothstep(0.3, 0.7, fr);
            o.dhFine += (terr - c.h) * mask * f;
            if (mv && fr > 0.3 && fr < 0.7 && mask > 0.5) { setMat(MAT_BASALT, 0.22); o.mark = 1; }
            break;
        }
        case TR_GREAT_BASIN: {
            // one vast impact basin, a fifth to two fifths of the radius across: a bowl 1.5-3 km deep flooded flat, a rim,
            // a second ring, ejecta
            Vec3 cdir = normalize(Vec3(tu(14) - 0.5, tu(15) - 0.5, tu(16) - 0.5));
            double rad = R * (0.22 + 0.18 * tu(17));
            double x = std::acos(clampd(dot(unit, cdir), -1, 1)) * R / rad;
            if (x > 2.2) break;
            double depth = 1500 + 1500 * tu(18);
            double bowl = x < 1 ? -(1 - x * x) : 0;
            if (bowl < -0.55) bowl = -0.55;
            double rim = 0.45 * std::exp(-((x - 1) * (x - 1)) / (0.07 * 0.07));
            double ring2 = 0.18 * std::exp(-((x - 1.55) * (x - 1.55)) / (0.05 * 0.05));
            double ejecta = x > 1 ? 0.12 * std::exp(-(x - 1) / 0.35) : 0;
            o.dhSmooth += depth * (bowl + rim + ring2 + ejecta);
            if (bowl <= -0.55) setMat(MAT_BASALT, 0.18);
            else if (x > 0.93 && x < 1.07) { setMat(MAT_ROCK, 0.4); o.mark = 1; }
            break;
        }
        case TR_CORONAE: {
            // ring-shaped volcanic ridges 100-250 km across with a moat and a sunken centre, half the cells of a grid of 0.3 R
            if (!scaleVisible(20.0, detailM)) break;
            double dh = 0;
            visitFeatures(p, R, R * 0.3, 0.5, g.sE + 350, 0.5, [&](const FeatureHit& f) {
                double r = 50 + 75 * f.u[3], x = f.d / r;
                if (x > 1.5) return;
                double ridge = 0.6 * std::exp(-((x - 1) * (x - 1)) / (0.1 * 0.1));
                double moat = -0.25 * std::exp(-((x - 1.2) * (x - 1.2)) / (0.08 * 0.08));
                double centre = x < 0.9 ? -0.15 * (1 - x / 0.9) : 0;
                double fine = scaleVisible(2.0, detailM) ? 0.08 * ridged3(p / 3.0, g.sE + 351, 2) * smoothstep(1.5, 0.8, x) : 0.0;
                dh += (600 + 400 * f.u[4]) * (ridge + moat + centre + fine);
            });
            o.dhSmooth += dh * lodFade(R * 0.05, detailM);
            if (std::fabs(dh) > 200) o.mark = 1;
            break;
        }
        case TR_SPIRES: {
            if (kindHas(g.type, KT_CRYSTAL)) {   // crystal spires 10-35 m tall in fields
                double mask = prov(7, 0.2, 0.45);
                if (mask < 0.01 || detailM > 5) break;   // the 4 m ring's, like the ice spires (B-404: `scaleVisible(0.01)` wanted a 3 m sample, so no ring ever drew them; the crystal landmarks stood on bare ground)
                Worley3 w = worley3(p / 0.05, g.sE + 360);
                if (unitFromHash(mix64(w.id1 + 2)) < 0.4) {
                    double r = 0.006 + 0.006 * unitFromHash(mix64(w.id1 + 3));
                    double x = w.f1 * 0.05 / r;
                    if (x < 1) { o.dhFine += (10 + 25 * unitFromHash(mix64(w.id1 + 4))) * std::pow(1 - x, 1.4) * mask; if (x < 0.9 && mask > 0.5) { setMat(MAT_QUARTZ, 0.88); o.mark = 1; } }
                }
            } else {   // ice spires: penitentes 3-8 m tall on a 12 m grid on the flats (the 4 m ring's, so within 120 m)
                double mask = prov(7, 0.15, 0.4) * smoothstep(0.25, 0.1, c.relief0);
                if (mask < 0.01 || detailM > 5) break;
                Worley3 w = worley3(p / 0.012, g.sE + 361);
                double x = w.f1 * 0.012 / 0.005;
                if (x < 1) { o.dhFine += (3.0 + 5.0 * unitFromHash(mix64(w.id1 + 4))) * (1 - x) * (1 - x) * mask; if (mask > 0.5) o.mark = mask; }
            }
            break;
        }
        case TR_GEYSERS: {
            // geyser basins: sinter mounds 12 m high and 0.3-0.9 km across with 1.6 m terraces and a pool at the vent, pale crusts
            double mask = land * smoothstep(0.3, 0.1, c.relief0);
            if (mask < 0.01 || (featFade(1.8, detailM) <= 0 && !matVisible(1.8, detailM))) break;   // B-404: by the biggest basin
            double dh = 0; bool sinter = false;
            visitFeatures(p, R, 20.0, 0.35, g.sE + 370, 0.1, [&](const FeatureHit& f) {
                double r = 0.3 + 0.6 * f.u[3], x = f.d / r;
                if (x >= 1.0) return;
                double mound = 12.0 * (1 - x * x);
                double terr = std::floor(mound / 1.6) * 1.6 + 1.6 * smoothstep(0.3, 0.7, std::fmod(mound, 1.6) / 1.6);
                dh += (terr * (0.6 + 0.4 * f.u[4]) + (x < 0.12 ? -3.0 * (1 - x / 0.12) : 0.0)) * featFade(2 * r, detailM);   // a mound's own fade
                if (matVisible(2 * r, detailM)) sinter = true;   // the crust wherever the mound spans half a cell
            });
            o.dhFine += dh * mask;
            if (sinter && mask > 0.5) { setMat(MAT_SALT, 0.85); o.mark = 1; }
            break;
        }
        case TR_GREAT_SHIELD: {
            // Olympus Mons: one shield volcano (`greatShieldOf`) 300-600 km across on a world of Mars' size (a tenth to a sixth of a smaller
            // world's radius), 22 km high there and lower as the radius (the gravity) grows: a convex dome of lava lobes running down from the
            // summit, the caldera's nested pits at its top, a cliff ring a fifth of its height at its foot (Olympus' basal scarp), the aureole's
            // ridged lobes spread over the plain beyond; out of a sea it stands as an island (it is hundreds of kilometres: every ring and the
            // map carry it)
            const GreatShield S = greatShieldOf(g);
            const double x = std::acos(clampd(dot(unit, S.c), -1, 1)) * R / S.radKm;
            if (x > 2.3) break;
            const double foot = 0.21;   // the cliff ring, of the height
            const Vec3 e1 = normalize(cross(S.c, std::fabs(S.c.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(S.c, e1);
            const double ang = std::atan2(dot(unit, e2), dot(unit, e1));
            double dh = 0; bool scarp = false, pit = false, lava = false;
            if (x < 1.0) {
                dh = S.heightM * (foot + (1 - foot) * std::pow(1 - std::pow(x, 1.6), 1.15));
                // the lobes: streaks running down from the summit (a noise wrapped round it, 60 a turn, 25 km along), a radius' sixtieth wide
                const double lobe = fbm3(Vec3(std::cos(ang) * 60.0, std::sin(ang) * 60.0, x * S.radKm / 25.0), g.sE + 400, 3);
                const double wKm = std::max(0.5, x * S.radKm / 60.0);
                dh += 80.0 * lobe * lodFade(wKm * 2, detailM);
                lava = lobe > 0.12 && x < 0.9 && matVisible(wKm, detailM);
                double cal = 0;   // the caldera: the deepest of its pits here
                for (int k = 0; k < S.nPits; k++) {
                    const double xp = std::acos(clampd(dot(unit, S.pit[k]), -1, 1)) * R / S.pitKm[k];
                    if (xp >= 1.0) continue;
                    cal = std::min(cal, -S.pitDepthM[k] * (xp < 0.8 ? 1.0 : 1.0 - smoothstep(0.8, 1.0, xp)));
                    if (matVisible(0.2 * S.pitKm[k], detailM)) pit = true;
                }
                dh += cal;
            } else {
                const double sw = std::max(0.025, 1.5 * detailM / (S.radKm * 1000.0));   // the scarp's run: a few samples at least
                dh = S.heightM * foot * smoothstep(1.0 + sw, 1.0, x);
                scarp = x < 1.0 + sw;
                const double lobes = smoothstep(0.15, 0.55, 0.5 + 0.5 * std::sin(5.0 * ang + 3.0 * fbm3(unit * 4.0, g.sE + 401, 2) + TAU * tu(61)));
                dh += 450.0 * lobes * smoothstep(2.3, 1.15, x) * ridged3(p / 18.0, g.sE + 402, 3) * lodFade(18.0, detailM);   // the aureole
            }
            o.dhSmooth += dh;
            if (scarp) { setMat(MAT_ROCK, 0.4); o.mark = 1; }
            else if (pit) { setMat(MAT_BASALT, 0.16); o.mark = 1; }
            else if (lava) setMat(MAT_BASALT, 0.2);
            break;
        }
        case TR_TESSERAE: {
            // Venus' tesserae (Alpha and Ovda Regio): plateaus 1-2.5 km over the plains with steep margins (a province), their crust folded into
            // ridges 8-16 km apart and broken across by graben 3-6 km apart, a third of that wide and 200-400 m deep: two directions crossing at
            // a wide angle (planes through the world: over a province they cross at about the angle between them), a woven ground
            const double plat = land * prov(20, 0.08, 0.2);
            if (plat < 0.01) break;
            const Vec3 dA = normalize(Vec3(tu(62) - 0.5, tu(63) - 0.5, tu(64) - 0.5));
            const Vec3 dB = normalize(cross(dA, normalize(Vec3(tu(65) - 0.5, tu(66) - 0.5, tu(67) - 0.5))));
            const double warp = 5.0 * fbm3(p / 45.0, g.sE + 410, 2);
            const double lamA = 8.0 + 8.0 * tu(68), lamB = 3.0 + 3.0 * tu(69);
            const double sA = (dot(p, dA) + warp) / lamA, sB = (dot(p, dB) + 0.6 * warp) / lamB;
            const double ridge = std::pow(0.5 + 0.5 * std::cos(TAU * sA), 1.5);       // the folds' rounded crests
            const double gd = std::fabs(sB - std::floor(sB) - 0.5) / 0.17;              // across a graben (its half-width 17% of the spacing)
            const double graben = gd < 1 ? 1 - smoothstep(0.55, 1.0, gd) : 0.0;       // a flat floor between steep walls
            const double gW = lamB * 0.34;   // the graben's width, km
            o.dhSmooth += (1000.0 + 1500.0 * tu(70)) * plat + plat * ((300.0 + 300.0 * tu(71)) * (ridge - 0.5) * lodFade(lamA, detailM) - (200.0 + 200.0 * tu(72)) * graben * featFade(gW, detailM));
            if (plat > 0.6 && gd > 0.5 && gd < 1.0 && matVisible(gW * 0.25, detailM)) { setMat(MAT_ROCK, 0.46); o.mark = 1; }   // the graben's walls
            else if (plat > 0.6) o.mark = 0.6;
            break;
        }
        case TR_LABYRINTH: {
            // Noctis Labyrinthus: a rise 0.8-2 km high (a province) cut by a maze of intersecting fault troughs, the boundaries of 30 km cells and
            // of 12 km cells, 6-10 and 2.5-4 km wide, their flat floors 2-5 and 0.9-2.4 km down between walls stepped by three faults; the blocks
            // between stand as plateaus and mesas
            const double mask = land * prov(21, 0.12, 0.3);
            if (mask < 0.01) break;
            bool wall = false, floor = false;
            auto trough = [&](double cellKm, uint64_t seed, double halfW, double dep) {
                const double f = featFade(halfW * 2, detailM);
                const bool mv = matVisible(halfW * 0.15, detailM);   // a fault step's run
                if (f <= 0 && !mv) return 0.0;
                const Worley3 w = worley3(p / cellKm, seed);
                const double d = (w.f2 - w.f1) * cellKm * 0.5;   // km from the boundary
                const double tt = clampd(1 - d / halfW, 0, 1);    // 1 at the middle line
                const double s3 = clampd(tt / 0.5, 0, 1) * 3, k = std::floor(s3), fr = s3 - k;
                const double prof = tt >= 0.5 ? 1.0 : (k + smoothstep(0.25, 0.75, fr)) / 3;   // the floor over the middle half, three faults down each wall
                if (mv && prof > 0.03 && prof < 0.97) wall = true;
                if (mv && prof >= 0.97) floor = true;
                return -dep * prof * f;
            };
            const double a = trough(30.0, g.sE + 420, 3.0 + 2.0 * tu(73), (2000.0 + 3000.0 * tu(74)) * mask);
            const double b = trough(12.0, g.sE + 421, 1.25 + 0.75 * tu(75), (900.0 + 1500.0 * tu(76)) * mask);
            o.dhSmooth += (800.0 + 1200.0 * tu(77)) * mask + std::min(a, b);
            if (mask > 0.5 && (wall || floor)) { if (wall) setMat(MAT_ROCK, 0.42); o.mark = 1; }
            break;
        }
        case TR_HOLLOWS: {
            // Mercury's hollows: clusters a few kilometres across (where a 6 km noise is high, in a province) of shallow flat-floored pits 60-500 m
            // across and 15-40 m deep, their outlines ragged, bright and bluish with a halo where the rock breathed out its volatile (Blewett et al.
            // 2011); the pits stand on the half-kilometre cells of a jittered grid (the 64 and 16 m rings carry them, the 512 m one their halos)
            const double mask = land * prov(22, 0.0, 0.3);
            if (mask < 0.01) break;
            const double cl = smoothstep(0.1, 0.4, fbm3(p / 6.0, g.sE + 431, 2));   // the cluster here
            if (cl <= 0) break;
            if (mask * cl > 0.5) o.mark = 0.6;   // the cluster's field at every scale (the scanner's 500 m scan reads it, the map takes no pit)
            if (!matVisible(0.12, detailM)) break;
            double dh = 0, bright = 0;
            visitFeatures(p, R, 0.5, 0.6, g.sE + 430, 0.9, [&](const FeatureHit& f) {
                const double r = (0.03 + 0.22 * f.u[3] * f.u[3]) * (0.6 + 0.4 * cl);
                const double x = f.d / r;
                if (x > 1.7) return;
                const double rag = 1 + 0.22 * gnoise3((p - f.c) * (1.5 / r) + Vec3(f.u[0] * 50, f.u[1] * 50, f.u[2] * 50), g.sE + 432);   // the ragged outline
                const double xr = x / rag;
                if (xr < 1) dh -= (15.0 + 25.0 * f.u[4]) * (xr < 0.7 ? 1.0 : 1 - smoothstep(0.7, 1.0, xr)) * featFade(2 * r, detailM);
                if (matVisible(2 * r, detailM)) bright = std::max(bright, smoothstep(1.7, 1.0, xr));
            });
            o.dhFine += dh * mask * cl;
            if (bright * mask * cl > 0.5) { setMat(MAT_SALT, 0.78); o.mark = 1; }
            break;
        }
        case TR_CRATER_CHAINS: {
            // catenae: lines of six to seventeen craters of one size, 30-280 km long, where a comet torn apart by its planet's tides struck in a row
            // (Ganymede's Enki Catena, Callisto's Gipul, Shoemaker-Levy 9 on Jupiter), on 15% of the cells of a grid of a fifth of the radius
            // (`chainOf`): bowls a fifth of their width deep, their rims touching
            const PointGrid G = chainGrid(g);
            double dh = 0; bool on = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.4, [&](const FeatureHit& f) {
                const double len = f.cellKm * (0.2 + 0.42 * f.u[1]), sp = len / (std::min(17, 6 + (int)(f.u[2] * 12)) - 1);
                if (f.d > 0.5 * len + 1.6 * sp) return;   // beyond the chain's craters
                const CraterChain C = chainOf(f);
                const Vec3 rel = p - C.c;
                const double along = dot(rel, C.dir), across = dot(rel, C.perp);
                for (int k = 0; k < C.n; k++) {
                    const double dx = along - C.at[k];
                    if (std::fabs(dx) > 2.0 * C.rKm[k] || std::fabs(across) > 2.0 * C.rKm[k]) continue;
                    const double x = std::sqrt(dx * dx + across * across) / C.rKm[k];
                    if (x > 2.0) continue;
                    const double bowl = x < 1 ? -(1 - x * x) : 0.0;
                    const double rimv = 0.3 * std::exp(-((x - 1) * (x - 1)) / (0.15 * 0.15));
                    const double ej = x > 1 ? 0.08 * std::exp(-(x - 1) / 0.35) : 0.0;
                    dh += C.depthM[k] * (bowl + rimv + ej) * featFade(2 * C.rKm[k], detailM);
                    if (x < 1.15 && matVisible(0.3 * C.rKm[k], detailM)) on = true;
                }
            });
            o.dhSmooth += dh;
            if (on) o.mark = 1;
            break;
        }
        case TR_PENITENTES: {
            // blades of ice in rows across the sun's path (each a pointed blade longer east and west than across, its rows north and south of
            // one another: they lean to the noon sun), in fields within 60 degrees of the equator: on a living world the snow of its cold dry
            // heights, blades 2-5 m tall in rows 10-14 m apart (the high Andes'; the 4 m ring's); on the ices of an airless or a thin-aired world
            // 4-10 m in rows 14-22 m apart, as Europa's might stand (Hobley et al. 2018); on a nitrogen world Pluto's bladed terrain, a hundred
            // times the size: ridges 150-500 m high on rows 3-6 km apart running east and west, broken into blades (Moore et al. 2018), seen from far
            const double absLat = std::fabs(std::asin(clampd(unit.z, -1, 1)));
            if (absLat > 62 * DEG) break;
            const bool living = KINDS[g.type].ground == GR_FELISIAN, giant = g.type == PT_NITROGEN;
            const double field = living ? land * smoothstep(-2.0, -8.0, c.T) * smoothstep(0.5, 0.3, c.moist) * smoothstep(1200.0, 2500.0, c.hS)
                                        : prov(23, -0.05, 0.25) * smoothstep(0.4, 0.15, c.relief0);
            const double mask = field * smoothstep(62 * DEG, 50 * DEG, absLat);
            if (mask < 0.01) break;
            const double rowM = giant ? 3000.0 + 3000.0 * tu(86) : (living ? 10.0 + 4.0 * tu(86) : 14.0 + 8.0 * tu(86));
            const double bladeM = giant ? 150.0 + 350.0 * tu(87) : (living ? 2.0 + 3.0 * tu(87) : 4.0 + 6.0 * tu(87));
            const double fade = giant ? lodFade(rowM / 1000.0, detailM) : (detailM <= 5 ? 1.0 : 0.0);   // the small blades the 4 m ring's alone, as the ice spires
            if (fade > 0) {
                const double lat = std::asin(clampd(unit.z, -1, 1)), north = lat * R * 1000.0;   // metres
                const double east = std::atan2(unit.y, unit.x) * R * 1000.0 * std::max(0.2, std::cos(lat));   // metres (a seam at the antimeridian, a row's one blade)
                const double alongM = rowM * 1.1;   // a blade's place along its row
                const int64_t i0 = (int64_t)std::floor(north / rowM);
                double best = 0;
                for (int64_t i = i0 - 1; i <= i0 + 1; i++) {   // the blades of the three nearest rows, each pointed: a diamond longer along its row
                    const double off = alongM * unitFromHash(hash2i(i, 0, g.sE + 440));   // the row's own phase
                    const int64_t j0 = (int64_t)std::floor((east - off) / alongM);
                    for (int64_t j = j0 - 1; j <= j0 + 1; j++) {
                        const uint64_t hb = hash2i(i, j, g.sE + 441);
                        const double cn = (i + 0.5 + 0.25 * (unitFromHash(hb) - 0.5)) * rowM, ce = off + (j + 0.5 + 0.3 * (unitFromHash(mix64(hb + 1)) - 0.5)) * alongM;
                        const double d = std::fabs(east - ce) / (0.48 * alongM) + std::fabs(north - cn) / (0.3 * rowM);
                        if (d < 1) best = std::max(best, std::pow(1 - d, 0.7) * (0.6 + 0.4 * unitFromHash(mix64(hb + 2))));
                    }
                }
                const double dh = bladeM * best * mask * fade;
                if (giant) o.dhSmooth += dh; else o.dhFine += dh;
            }
            if (mask > 0.5) { setMat(living ? MAT_SNOW : MAT_ICE, living ? 0.88 : 0.8); o.mark = mask; }
            break;
        }
        case TR_COLUMNS: {
            // columnar basalt: a province of thick lava sheets, lobes on the cells of a 2-5 km grid (two in three raised by their thickness, 15-40
            // m), each lobe's edge a cliff of columns (a colonnade: the Columbia River's coulees, the Giant's Causeway); within the 4 m ring the
            // edge runs along the columns' polygons (cells of 5-8 m: a column stands on whichever lobe its middle does) and the tops by the edge are
            // a pavement of the columns' heads stepped by up to a metre
            const double mask = land * prov(24, 0.12, 0.35) * smoothstep(0.4, 0.15, c.relief0);
            if (mask < 0.01) break;
            const double cellK = 2.0 + 3.0 * tu(93), colKm = (5.0 + 3.0 * tu(94)) / 1000.0;
            const Worley3 w = worley3(p / cellK, g.sE + 445);
            auto lobeH = [&](uint64_t id) { return unitFromHash(mix64(id + 11)) < 0.66 ? 15.0 + 25.0 * unitFromHash(mix64(id + 12)) : 0.0; };
            const double h1 = lobeH(w.id1), h2 = lobeH(w.id2);
            const double eM = (w.f2 - w.f1) * cellK * 0.5 * 1000.0;   // metres to the lobe's edge
            double hh, dFine = 0; bool cliff = false;
            if (detailM <= 5 && eM < 2.5 * colKm * 1000.0 && h1 != h2) {   // the edge along the columns
                const Worley3 cw = worley3(p / colKm, g.sE + 446);
                const Worley3 wc = worley3(cw.c1 * colKm / cellK, g.sE + 445);   // the lobe the column's middle stands on
                hh = lobeH(wc.id1);
                cliff = true;
            } else {
                const double wEdge = std::max(3.0, 0.75 * detailM);   // the cliff's run (a few samples at least)
                hh = h2 + (h1 - h2) * (0.5 + 0.5 * smoothstep(0.0, wEdge, eM));
                cliff = h1 != h2 && eM < 1.5 * wEdge;   // the mark at every scale, the basalt where the cliff spans a sample
            }
            if (detailM <= 5 && hh > 0 && eM < 40.0) {   // the pavement: the columns' heads by the edge, their joints
                const Worley3 cw = worley3(p / colKm, g.sE + 446);
                dFine = 0.9 * (unitFromHash(mix64(cw.id1 + 3)) - 0.5) - 0.3 * (1 - smoothstep(0.0, 0.6, (cw.f2 - cw.f1) * colKm * 1000.0));
            }
            o.dhSmooth += hh * mask;
            o.dhFine += dFine * mask;
            if (mask > 0.5 && cliff) o.mark = 1;
            if (mask > 0.5 && h1 != h2 && eM < 30.0 && matVisible(0.03, detailM)) setMat(MAT_BASALT, 0.2);   // B-404: the colonnade, its pavement and its foot: a band of
            break;                                                                                              // 30 m by the unfaded edge, in every ring that can hold it
        }
        case TR_MUD_VOLCANOES: {
            // fields of mud volcanoes on the low plains of a province (Azerbaijan's Gobustan; the mounds on Mars' plains): cones 6-50 m high and
            // 100-700 m across with a crater pool at the top, two to four tongues of grey mud running out one to three radii from the foot, the
            // mud cracked grey (`mudConeOf`)
            const double mask = land * prov(25, 0.1, 0.4) * smoothstep(0.4, 0.15, c.relief0) * (KINDS[g.type].ground == GR_FELISIAN ? smoothstep(1500.0, 400.0, c.hS) : 1.0);
            if (mask < 0.01) break;
            if (featFade(0.2, detailM) <= 0 && !matVisible(0.2, detailM)) break;
            const PointGrid G = mudGrid(g);
            double dh = 0, mud = 0;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.75, [&](const FeatureHit& f) {
                const PointSize S = mudConeOf(f);
                const double x = f.d / S.rKm;
                if (x > 4.0) return;
                if (x < 1) {
                    double cone = std::pow(1 - x, 1.3);
                    if (x < 0.12) cone -= 0.18 * (1 - x / 0.12);   // the crater pool
                    dh += S.heightM * cone * featFade(2 * S.rKm, detailM);
                    if (matVisible(S.rKm, detailM)) mud = 1;
                    return;
                }
                const Vec3 cn = normalize(f.c);
                const Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
                const Vec3 d = p - f.c;
                const double ang = std::atan2(dot(d, e2), dot(d, e1));
                const int nT = 2 + (int)(f.u[5] * 3);
                for (int k = 0; k < nT; k++) {   // the tongues
                    const double a0 = TAU * std::fmod(f.u[0] + 0.618034 * k, 1.0), len = 1.0 + 2.0 * std::fmod(f.u[1] + 0.414214 * k, 1.0);
                    const double wAng = 0.35 * (1.0 - 0.6 * (x - 1) / len), da = wrapAngle(ang - a0);
                    if (x > 1 + len || std::fabs(da) > wAng) continue;
                    const double shape = (1 - (da / wAng) * (da / wAng)) * smoothstep(1 + len, 1 + len - 0.4, x);
                    const double wKm = 2 * wAng * x * S.rKm;
                    dh += 1.5 * shape * featFade(wKm, detailM);
                    if (matVisible(wKm, detailM)) mud = std::max(mud, shape);
                }
            });
            o.dhFine += dh * mask;
            if (mud * mask > 0.5) { setMat(MAT_MUD, 0.34); o.mark = 1; }
            break;
        }
        case TR_INVERTED_RIVERS: {
            // Mars' Aeolis Dorsa: the beds of old rivers standing as sinuous flat-topped ridges 8-40 m high and 150-500 m wide where the plain
            // round them wore away, branching where two nets meet (the zero lines of a warped noise of 30 km and one of 10), broken where the wind
            // cut them through, on the dry plains of a province; their caps the cemented gravels of the beds
            const double mask = land * prov(26, 0.08, 0.32) * smoothstep(0.4, 0.2, c.moist) * smoothstep(0.35, 0.12, c.relief0);
            if (mask < 0.01) break;
            const double hw = 0.075 + 0.175 * tu(100);   // the main ridges' half-width, km
            const Vec3 q = p / 30.0;
            const Vec3 wq = q + Vec3(gnoise3(q * 0.6 + Vec3(3.1, 0, 0), g.sE + 451), gnoise3(q * 0.6 + Vec3(0, 7.7, 0), g.sE + 452), gnoise3(q * 0.6 + Vec3(0, 0, 5.3), g.sE + 453)) * 0.8;
            auto bed = [&](const Vec3& at, double wl, uint64_t seed, double halfW) {
                const double d = std::fabs(gnoise3(at, seed)) * wl / 1.6;   // km from the line (the noise's slope about 1.6 a wavelength)
                return 1 - smoothstep(0.6 * halfW, 1.15 * halfW, d);        // the flat top over the middle 60%, steep flanks
            };
            const double main = bed(wq, 30.0, g.sE + 454, hw), trib = 0.6 * bed(wq * 3.0, 10.0, g.sE + 455, 0.6 * hw);
            const double prof = std::max(main, trib) * smoothstep(-0.35, 0.05, gnoise3(p / 2.5, g.sE + 456));   // broken where the wind cut through
            o.dhSmooth += (8.0 + 32.0 * tu(101)) * prof * mask * featFade(hw, detailM);
            if (prof * mask > 0.5) { o.mark = 1; if (matVisible(hw * 0.6, detailM)) setMat(MAT_ROCK, 0.36); }   // the mark at every scale (the scanner's)
            break;
        }
        case TR_PINGOS: {
            // pingos on the cold flat lowlands (the Mackenzie delta's: Ibyuk, 49 m): hills of ice under a skin of ground 10-60 m high and 80-400 m
            // across on a quarter of the cells of a 1.2 km grid, steep-sided, the big ones split open at the top where the ice core shows (`pingoOf`)
            const double cold = kindHas(g.type, KT_FROZEN) ? 1.0 : smoothstep(-1.0, -6.0, c.T);
            const double mask = cold * land * smoothstep(0.3, 0.1, c.relief0) * prov(27, -0.15, 0.2) * (KINDS[g.type].ground == GR_FELISIAN ? smoothstep(900.0, 250.0, c.hS) : 1.0);
            if (mask < 0.01) break;
            if (featFade(0.16, detailM) <= 0 && !matVisible(0.1, detailM)) break;
            const PointGrid G = pingoGrid(g);
            double dh = 0; bool ice = false, on = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.4, [&](const FeatureHit& f) {
                const PointSize S = pingoOf(f);
                const double x = f.d / S.rKm;
                if (x >= 1) return;
                double prof = std::pow(1 - x * x, 1.6);
                if (S.heightM > 30 && x < 0.3) { prof -= 0.35 * (1 - x / 0.3) * (1 - x / 0.3); if (matVisible(0.6 * S.rKm, detailM)) ice = true; }   // split open
                dh += S.heightM * prof * featFade(2 * S.rKm, detailM);
                if (x < 0.8 && matVisible(1.6 * S.rKm, detailM)) on = true;
            });
            o.dhFine += dh * mask;
            if (mask > 0.5 && on) { if (ice) setMat(MAT_ICE, 0.7); o.mark = 1; }
            break;
        }
        case TR_MAARS: {
            // a volcanic field of maars (the Eifel's): round craters 0.4-2 km across blown by steam where magma met the groundwater, their floors
            // 50-200 m under the plain within steep inner walls, a low ring of tephra 8-28 m high round them and its apron over two radii, on 45%
            // of the cells of a 4 km grid in a province (`maarOf`); on a world it rains on the drainage fills them, crater lakes
            const double mask = land * prov(28, 0.12, 0.38) * smoothstep(0.45, 0.2, c.relief0);
            if (mask < 0.01) break;
            if (featFade(0.4, detailM) <= 0 && !matVisible(0.3, detailM)) break;
            const PointGrid G = maarGrid(g);
            double dh = 0; bool rim = false, in = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.75, [&](const FeatureHit& f) {
                const PointSize S = maarOf(f);
                const double x = f.d / S.rKm;
                if (x > 3.0) return;
                const double v = x < 1 ? -S.heightM + (S.heightM + S.rimM) * smoothstep(0.6, 1.0, x) : S.rimM * std::exp(-(x - 1) / 0.35);
                dh += v * featFade(2 * S.rKm, detailM);
                if (x < 1.25 && matVisible(0.4 * S.rKm, detailM)) { in = true; if (x > 0.6) rim = true; }
            });
            o.dhSmooth += dh * mask;
            if (mask > 0.5 && in) { if (rim) setMat(MAT_BASALT, 0.24); o.mark = 1; }
            break;
        }
        case TR_HOT_SPRINGS: {
            // Q-02: terraced hot springs (Pamukkale's travertine, the Grand Prismatic's rings): mounds of white sinter stepping down in rimmed
            // terraces from the vent's blue pool at the top, the pools between the rims coloured by the mats of the life that lives in their heat
            // (bright near the vent, darker as the water cools), the runoff's mats streaking the ground round them; in clusters in a geothermal
            // province (`springGrid`, `springOf`; their steam: `hotSpringsNear`)
            const double mask = land * prov(41, 0.15, 0.42) * smoothstep(0.45, 0.2, c.relief0);
            if (mask < 0.01) break;
            if (featFade(0.03, detailM) <= 0 && !matVisible(0.03, detailM)) break;
            const PointGrid G = springGrid(g);
            double dh = 0; int mat = -1; double alb = 0;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.7, [&](const FeatureHit& f) {
                if (f.d > 1.3) return;
                const int n = springCount(f);
                for (int k = 0; k < n; k++) {
                    const HotSpring S = springOf(f, k, R);
                    const double x = length(p - S.c) / S.rKm;
                    if (x > 1.8) continue;
                    double h = 0; int m = -1; double a = 0;
                    if (x < 1) {
                        const double dome = S.heightM * (1 - std::pow(x, 1.5));
                        const double u = (1 - x) * S.terraces; const int tk = (int)u; const double frac = u - tk;   // the terrace (0 the outermost) and the place across it
                        double step = S.heightM * (tk + 0.4) / S.terraces + (frac < 0.16 ? 0.45 : -0.1);   // its pool's floor, its rim's crest at its outer edge
                        if (x < 0.14) step = S.heightM - 1.3;   // the vent's pool
                        else if (x < 0.24) step = S.heightM;    // the sinter round it
                        h = dome + (step - dome) * featFade(S.rKm / S.terraces, detailM);
                        if (matVisible(0.3 * S.rKm, detailM)) {   // white sinter terraces holding coloured pools: where the eye sees a terrace's rim, its rim
                            // white and its pool coloured; where it sees the terraces, every other one white; from further, the inner half white
                            const double tw = S.rKm / S.terraces;
                            const bool white = matVisible(0.004, detailM) ? frac < 0.16 : (matVisible(tw, detailM) ? (tk & 1) == 0 : x < 0.55);
                            if (x < 0.14) { m = MAT_BRINE; a = 0.16; }
                            else if (x < 0.24 || white) { m = MAT_SALT; a = 0.86; }
                            else { m = MAT_MATS; a = 0.68 - 0.42 * x; }
                        }
                    } else if (matVisible(0.02, detailM)) {   // the runoff's streaks of mats down the ground round it
                        const Vec3 rel = p - S.c;
                        const double streak = std::sin(std::atan2(dot(rel, cross(normalize(S.c), Vec3(0, 0, 1))), dot(rel, Vec3(0, 0, 1))) * 9.0 + 3.0 * fbm3(p / 0.05, g.sE + 493, 2));
                        if (streak > 0.45 && x < 1.1 + 0.3 * streak) { m = MAT_MATS; a = 0.2; }
                    }
                    if (h > dh) dh = h;
                    if (m >= 0 && (mat < 0 || x < 1)) { mat = m; alb = a; }
                }
            });
            o.dhFine += dh * mask;
            if (mask > 0.5 && mat >= 0) { setMat(mat, alb); o.mark = 1; }
            break;
        }
        case TR_METEORITES: {
            // Q-02: the strewn fields of iron meteorites (Campo del Cielo; the blue ice of Antarctica's, where the ice brings them up and the wind
            // keeps them bare): an ellipse along the way the body broke up (`strewnOf`), its stones thicker downrange where the heaviest went,
            // where a few fresh pits 6-60 m across mark the biggest's falls; on the open ground, marked in every ring (the scanner finds them), the
            // stones themselves the surface's objects (`SurfaceView::forMeteorites`, by the field's density here)
            const double mask = land * smoothstep(0.5, 0.25, c.relief0);
            if (mask < 0.01) break;
            const PointGrid G = strewnGrid(g);
            double dens = 0, dh = 0; bool pitFloor = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.25, [&](const FeatureHit& f) {
                if (f.d > 14.0) return;
                const StrewnField S = strewnOf(f);
                const Vec3 rel = p - S.c;
                const double u = dot(rel, S.dir) / (0.5 * S.lenKm), v = dot(rel, S.perp) / (0.5 * S.widthKm), in = 1 - (u * u + v * v);
                if (in <= 0) return;
                dens = std::max(dens, smoothstep(0.0, 0.3, in) * (0.3 + 0.7 * smoothstep(-1.0, 1.0, u)));
                const int nPits = 3 + (int)(f.u[5] * 6);
                for (int k = 0; k < nPits; k++) {   // the pits downrange: their places along the way, a little either side, bigger furthest
                    const double fu = 0.25 + 0.7 * std::fmod(f.u[1] + 0.618034 * k, 1.0), fv = 0.6 * (std::fmod(f.u[2] + 0.414214 * k, 1.0) - 0.5);
                    const Vec3 pc = S.c + S.dir * (fu * 0.5 * S.lenKm) + S.perp * (fv * 0.5 * S.widthKm);
                    const double rKm = (0.003 + 0.027 * std::fmod(f.u[4] + 0.31 * k, 1.0)) * (0.5 + fu), d = length(p - pc) / rKm;
                    if (d > 2.0) continue;
                    const double depthM = 0.22 * rKm * 1000.0;
                    dh += (d < 1 ? -depthM * (1 - d * d) + 0.12 * depthM : 0.12 * depthM * std::exp(-(d - 1) / 0.3)) * featFade(2 * rKm, detailM);
                    if (d < 0.35 && matVisible(rKm, detailM)) pitFloor = true;
                }
            });
            if (dens > 0) { o.tint = std::max(o.tint, dens * mask); if (dens * mask > 0.1) o.mark = 1; }
            o.dhFine += dh * mask;
            if (pitFloor && mask > 0.5) setMat(MAT_METAL, 0.16);   // the iron at a pit's floor
            break;
        }
        case TR_STRATA: {
            // Q-02: rainbow strata (Zhangye Danxia, the Painted Hills): the sediments a wet age laid, oxidised layer by layer, bared on the hills'
            // slopes of a province: bands 4-12 m thick, each the rock's own colour or the accent's (a red, an orange or a green by the world) at a
            // brightness of its own (the two hues and their shades the banks hold), the layers' edges wavy; where a ring cannot hold the bands,
            // bands six times as thick (the far hills striped broadly). The material chain's height and slope, so every ring agrees (B-404)
            const double mask = land * prov(46, 0.18, 0.42) * smoothstep(0.1, 0.22, c.slope) * smoothstep(0.6, 0.3, c.moist);
            if (mask < 0.5) break;
            const double thick = 4.0 + 8.0 * tu(62);
            const double fine = matVisible(0.0006 * thick, detailM) ? 1.0 : 6.0;
            const double hh = c.hM + 3.0 * fbm3(p / 1.5, g.sE + 498, 2);   // the layers dip and wave a little
            const int64_t layer = (int64_t)std::floor(hh / (thick * fine));
            const uint64_t lh = mix64((uint64_t)layer * 0x9E3779B97F4A7C15ULL ^ g.seed ^ 0x57A7AULL);
            const double alb = 0.3 + 0.5 * unitFromHash(mix64(lh + 1));
            if (unitFromHash(lh) < 0.5) { o.tint = 1; setMat(familyRep(g.type, FAM_ROCK), alb); } else setMat(familyRep(g.type, FAM_ROCK), alb);
            o.mark = 1;
            break;
        }
        case TR_PETRIFIED: {
            // Q-02: a petrified forest (Arizona's Chinle beds): where a forest stood when the world was wet, its trunks buried, turned to stone
            // and bared again by the wind, lying in the open dry ground of a province; their density here for the surface's stone logs
            // (`SurfaceView::forLogs`), the ground pale and banded grey and lilac where they lie thickest
            const double mask = land * prov(45, 0.2, 0.45) * smoothstep(0.5, 0.25, c.relief0) * smoothstep(0.55, 0.3, c.moist);
            if (mask < 0.01) break;
            o.life = std::max(o.life, mask * (0.55 + 0.45 * fbm3(p / 2.5, g.sE + 497, 2)));
            if (mask > 0.5) o.mark = mask;
            break;
        }
        case TR_GYPSUM: {
            // Q-02: a field of white gypsum dunes (White Sands: the gypsum of a dried lake's beds blown downwind): transverse crests across the wind
            // 100-250 m apart and 8-18 m high, gentle on the windward side and steep on the lee, their tops bending into barchans, the flats between
            // them white too; in a province of the dry low country, white in every ring and on the maps (the field is kilometres across, B-404)
            const double mask = land * prov(43, 0.18, 0.4) * smoothstep(0.46, 0.26, c.moist) * smoothstep(0.4, 0.18, c.relief0) * smoothstep(1400.0, 600.0, c.hS);
            if (mask < 0.01) break;
            if (mask > 0.5) { setMat(MAT_SALT, 0.84); o.mark = mask; }   // gypsum is an evaporite: the salts' white in every view (the sand family's one colour is the world's dust on some)
            const double wl = 0.1 + 0.15 * tu(60), amp = 8.0 + 10.0 * tu(61);
            const double f = lodFade(wl, detailM);
            if (f <= 0) break;
            const Vec3 dd = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.15 * std::sin(g.windAngle * 2)));
            const Vec3 dp = normalize(cross(dd, unit));
            const double along = dot(p, dd) / wl, across = dot(p, dp) / wl;
            const double bend = 1.2 * fbm3(p / 0.6, g.sE + 494, 2) + 0.35 * std::sin(across * 0.9);   // the crests' bends: barchans' horns downwind
            double ph = along + bend; ph -= std::floor(ph);
            const double prof = ph < 0.8 ? smoothstep(0.0, 0.8, ph) : 1.0 - smoothstep(0.8, 1.0, ph);   // the windward slope, the slip face
            o.dhFine += amp * prof * mask * f * (0.75 + 0.25 * fbm3(p / 0.3, g.sE + 495, 2));
            break;
        }
        case TR_GLASSED: {
            // S-03: the blast that made the neutron star melted the surface; the melt pooled on the flats and lows and froze into
            // sheets of dark glass (the fine relief gone, ripples of a metre frozen in), the slopes and highs are scorched rock.
            // The mask reads the land, a province of about half the world and the material chain's slope and relief, all the same
            // in every ring and on the map (B-404), so a sheet is a sheet at every distance; the ripples alone fade by the ring
            double mask = land * prov(19, -0.45, -0.1) * smoothstep(0.25, 0.08, c.slope) * smoothstep(0.5, 0.25, c.relief0);
            if (mask < 0.01) break;
            o.flatten = std::max(o.flatten, 0.85 * mask);   // the fine relief melted into the smooth ground
            o.dhFine += mask * (0.5 * fbm3(p / 0.05, g.sE + 291, 2) * lodFade(0.05, detailM) + 1.5 * fbm3(p / 0.25, g.sE + 292, 2) * lodFade(0.25, detailM));   // frozen ripples and swells
            if (mask > 0.5) { setMat(MAT_GLASS, 0.42); o.mark = mask > 0.8 ? 1 : 0; }   // pale green glass (trinitite, not obsidian: a dark sheet under a dim star was a black screen)
            break;
        }
        default: break;
        }
        if (o.mark < 0) o.mark = mark0; else o.markTrait = t;
    }
}

// B-320: the felisian ground in one place (the sample, the far rings, the map and the lake levels read it): the
// continents, the plates, the mountain regions, the relief spectrum, the mesas, the islands, the shelf and the dunes.
// `hS` is the smooth ground, the relief's octaves of 2 km and longer over the base: the surface rivers, lakes and pools
// are cut into, so their water lies flat where the fine relief would have made it a bumpy sheet
double lakeBowlAt(const BodyGen& g, const Vec3& p, double detailM);   // O6-03: the lake bowls the drainage floods
struct FelGround {
    double h = 0, hS = 0, moist = 0.5, landMask = 0, relief0 = 0, hills = 0, slope = 0;
    double cliff = 0, talus = 0;   // O6-04: on a cliff band's riser, on its talus
    Vec3 gradT;                    // O6-04: the relief's gradient on the tangent plane (rise over run, uphill)
    double slopeM = 0; Vec3 gradM; // B-404: the material chain's slope and gradient (the same in every ring): the materials read these
    bool caldera = false;
    double iceSheet = 0;   // P-02: the ice age's sheet over the land here (metres thick: the tundra's, KT_ICE_SHEETS)
    bool sheetWall = false;   // P-03: on the sheet's terminus wall (its bare blue ice: the bedrock's slope under it said nothing of the ice's)
    double slip = 0;   // Q-03: the dry lowlands' dunes' slip face here (`SurfaceSample::slipFace`)
    LandOut land;   // R-305: the traits' landforms here
};
void felisianGround(const BodyGen& g, const Vec3& unit, double detailM, FelGround& o) {
    const double R = g.R;
    const KindDef& K = KINDS[g.type];
    Vec3 p = unit * R;
    double lat = std::asin(clampd(unit.z, -1, 1));
    auto fb = [&](double wlKm, uint64_t seed, int maxOct) -> double {
        double fade = lodFade(wlKm, detailM);
        if (fade <= 0) return 0.0;
        return fbm3(p / wlKm, seed, std::min(maxOct, octs(wlKm, detailM))) * fade;
    };
    auto rg = [&](double wlKm, uint64_t seed, int maxOct) -> double {
        double fade = lodFade(wlKm, detailM);
        if (fade <= 0) return 0.0;
        return ridged3(p / wlKm, seed, std::min(maxOct, octs(wlKm, detailM))) * fade;
    };
    Tectonics tec = tectonicsAt(unit, g);
    double massif = smoothstep(0.2, 0.6, rg(std::min(110.0, R * 0.3), g.sC + 6, 3));
    double hillsF = smoothstep(-0.15, 0.45, fb(std::min(R * 0.06, 400.0), g.sB + 6, 3));
    double contWl = R * 0.6 * g.contScale;
    double cont = warpedFbm(p, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22) * lodFade(contWl, detailM);
    cont += tec.uplift * 0.75;                     // M9-02: plate interiors decide most of the land
    double land = cont - g.seaLevel;               // >0 land
    // O6-05: the coast's exposure to the prevailing wind (the continent field's gradient against the wind: positive where
    // the wind blows onto the land), evaluated in the coastal band alone
    double exposure = 0;
    if (std::fabs(land) < 0.06 && scaleVisible(0.3, detailM)) {
        Vec3 pole(0, 0, 1);
        Vec3 e = normalize(cross(pole, unit)), n = cross(unit, e);
        if (length2(cross(pole, unit)) > 1e-8) {
            double dk = 2.0;   // km
            double cE = warpedFbm(p + e * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22) - warpedFbm(p - e * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22);
            double cN = warpedFbm(p + n * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22) - warpedFbm(p - n * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22);
            double gl = std::sqrt(cE * cE + cN * cN);
            if (gl > 1e-9) {
                int bandIdx = (int)std::floor(std::fabs(lat) / (30 * DEG));
                double a = g.windAngle + (lat < 0 ? PI : 0) + bandIdx * 0.9;   // the wind as the dunes read it
                double wE = std::cos(a), wN = std::sin(a);
                exposure = (cE * wE + cN * wN) / gl;   // +1: the wind blows straight onto this shore
            }
        }
    }
    double landMask = smoothstep(-0.02, 0.25, land);
    double mtnL = rg(R * 0.14, g.sC, 2);           // mountain regions (visible from orbit)
    double hills = fb(5.0, g.sB, 6);               // the hill pattern: beaches and wetland pools read it (O6-02: it no longer shapes the ground)
    double h = land * 3200.0;
    // B-321: the continental base is 1500 land^0.7 (an interior at 500-900 m, a shield at 1500 m; it was 3200 land^0.8,
    // whose plateaus at 2-3 km put half the land over the snow line); the sea keeps its depth from `land x 3200`
    if (h > 0) h = 1500.0 * std::pow(land, 0.7);
    const double hCont = h;   // P-03: the continents' base (a snowball's sheet lies smooth over it)
    double mtnMask = smoothstep(0.35, 0.75, mtnL) * landMask;
    // mountain chains along convergent boundaries, rifts and ridges along divergent ones
    double chainR = rg(45.0, g.sC + 2, 7);
    mtnMask = clampd(mtnMask + tec.chain * landMask * 0.9, 0, 1);
    h += tec.chain * landMask * (chainR * chainR * g.chainAmp + std::fabs(chainR) * 600.0);
    h -= tec.rift * landMask * 700.0 * (0.5 + 0.5 * std::fabs(fb(60.0, g.sC + 3, 3)));
    h += tec.ridge * (1 - landMask) * 900.0 * std::fabs(rg(40.0, g.sC + 4, 5));
    h += tec.arc * (1 - landMask) * 3200.0 * std::max(0.0, fb(28.0, g.sC + 5, 5));   // island arcs
    h += mtnL * mtnL * g.mountainAmp * mtnMask;
    // O6-02: the relief spectrum in the massifs, in hill country and (faintly) on the plains
    double rel = mtnMask * massif, hf = std::max(hillsF, 0.6 * mtnMask);   // massifs are mountains, the rest of the region hill country
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    double moist = 0.5 + 0.5 * fbm3(p / (R * 0.22), g.sF, 3) + g.moistureBias;
    {
        // rain shadow (M9-05): mountains upwind dry the land behind them
        Vec3 wdir = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.1));
        Vec3 pu = p + wdir * 90.0;
        double mtnUp = ridged3(pu / (R * 0.14), g.sC, 2) * lodFade(R * 0.14, detailM);
        moist -= 0.28 * smoothstep(0.4, 0.8, mtnUp) * landMask;
    }
    if ((K.tags & KT_FIXED_SUN) && g.locked) {   // P-03: a locked world's water by its climate at the sea's level (the same in every ring): the ring round
        // 8 C wet with the night's meltwater and the rain the day's air carries over it, the day side dried from 15 C (M5-08's rule is the others')
        const double Tsl = climateTempC(g, std::fabs(lat) / DEG, 0, 0, dot(unit, g.lockedDir)), x = (Tsl - 8.0) / 12.0;
        moist += K.ringWet * std::exp(-x * x) - K.dayDry * smoothstep(15.0, 45.0, Tsl);
    }
    ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf) * (0.05 + 0.95 * landMask), g.reliefRidge * (0.4 + 0.6 * rel), g.sB + 7, detailM);
    double hS = h + rf.hCoarse;
    double hRel = h; h += rf.h;
    {   // O6-04: cliff bands on the steep ground of the massifs and the chains
        CliffOut co = cliffBands(p, g, h, rf.slopeM, smoothstep(0.12, 0.4, relief0) * landMask, detailM);   // B-404: on the material chain's slope
        h += co.dh; o.cliff = co.cliff; o.talus = co.talus;
    }
    // R-306 / O6-04: the plains' own small relief, so flat country is not a billiard table underfoot (the shared term:
    // hummocks, outcrops, and gullies where it rains)
    h += plainRelief(p, g, 1.6 * smoothstep(0.3, 0.1, relief0) * landMask, smoothstep(0.28, 0.42, moist) * (0.7 + 0.8 * moist), detailM);   // O6-07: x1.6: the floodplains of O6-03 are flat, and the flat class keeps a 1% median grade
    const double T0 = climateTempC(g, std::fabs(lat) / DEG, hS, 0, (K.tags & KT_FIXED_SUN) && g.locked ? dot(unit, g.lockedDir) : 0.0);   // the mean climate on the smooth ground (the landforms', the permafrost's; P-03: a fixed sun's by its side)
    {   // R-305: the traits' landforms, cut into the smooth ground too (rivers and lakes follow them)
        LandCtx lc; lc.landMask = landMask; lc.moist = moist; lc.relief0 = relief0; lc.h = h; lc.hS = hS; lc.T = T0; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
        landformsAt(g, p, unit, detailM, lc, o.land);
        h += (hS - h) * o.land.flatten;
        double dS = o.land.dhSmooth + o.land.flatOffset * o.land.flatten;
        h += dS + o.land.dhFine; hS += dS;
    }
    if ((K.tags & KT_ICE_SHEETS) && !g.hasTrait(TR_POLYGONS)) {   // P-02: an ice age's permafrost patterned in polygons on the cold flats
        // (the patterned ground of R-305 everywhere it is cold, not as a trait: a honeycomb of low ridges 40-100 m across)
        double mask = smoothstep(2, -4, T0) * landMask * smoothstep(0.25, 0.1, relief0);
        double cell = 0.04 + 0.06 * unitFromHash(mix64(g.seed ^ 0x9071ULL));
        if (mask > 0.01 && scaleVisible(cell, detailM)) {
            Worley3 w = worley3(p / cell, g.sE + 310);
            double ridge = 1 - smoothstep(0.0, 0.006, (w.f2 - w.f1) * cell);
            h += (1.5 * ridge * ridge - 0.4 * (1 - ridge)) * mask * lodFade(cell, detailM);
        }
    }
    if (landMask > 0.5 && hS > 10) {   // O6-03: the lake bowls, dug into the ground and the smooth ground alike; the drainage floods them
        double bowl = lakeBowlAt(g, p, detailM);
        h -= bowl; hS -= bowl;
    }
    if (moist < 0.32 && relief0 < 0.4 && h > 30 && !g.hasTrait(TR_MESAS)) {   // M9-03: dry flats terrace into mesas (the badlands trait has its own)
        double step = 60.0, q = std::floor(h / step) * step;
        double f = (h - q) / step;
        double terr = q + step * smoothstep(0.35, 0.65, f);
        h = h + (terr - h) * (1 - smoothstep(0.2, 0.32, moist)) * (1 - smoothstep(0.25, 0.4, relief0));
    }
    // volcanic islands (M9-07) rising from the sea with a caldera lake
    bool caldera = false;
    if (scaleVisible(R * 0.25, detailM) && landMask < 0.5) { double vf = volcanoField(p, R, R * 0.25, g.islandDensity, g.sD + 9, 2600.0, caldera) * (1 - landMask) * lodFade(R * 0.25, detailM); h += vf; hS += vf; }
    if (h < 0) { h = -std::pow(-h, 1.15); if (g.seaDepth != 1) h *= g.seaDepth; }   // shallow coastal shelf (P-02: a kind's seas shallower)
    if (hS < 0) { hS = -std::pow(-hS, 1.15); if (g.seaDepth != 1) hS *= g.seaDepth; }
    // O6-05: the coast by its exposure. Windward: the land within a few metres of the sea stands up as a cliff 8-40 m
    // high with a wave-cut platform a metre or two under the water in front of it; leeward: a beach half as steep with
    // a foredune behind; in the flat shallows a barrier island rises along the shore 300-600 m out
    if (exposure != 0 && std::fabs(h) < 60) {
        double f = lodFade(0.3, detailM);
        double wind = smoothstep(0.25, 0.7, exposure), lee = smoothstep(-0.25, -0.7, exposure);
        if (wind > 0 && h > 0) {
            double cliffH = (8 + 32 * unitFromHash(mix64(g.seed ^ 0xC0A5))) * (0.6 + 0.4 * gnoise3(p / 6.0, g.sF + 44));
            double up = smoothstep(0.0, 2.5, h) * smoothstep(cliffH + 30, cliffH, h);
            h += (std::max(h, cliffH) - h) * up * wind * f;
            hS += (std::max(hS, cliffH) - hS) * up * wind * f * 0.5;
        }
        if (wind > 0 && h < 0 && h > -8) h += (-1.5 - 0.5 * (-h) - h) * smoothstep(-8, -3, h) * wind * f;   // the platform
        if (lee > 0 && h > 0 && h < 12) {
            h *= 1 - 0.5 * lee * f * smoothstep(12, 2, h);                                   // the beach
            double dune = 4.0 * smoothstep(1.5, 4.0, h) * smoothstep(9.0, 5.0, h) * (0.5 + 0.5 * gnoise3(p / 0.25, g.sF + 45));
            h += dune * lee * f * lodFade(0.25, detailM);                                    // the foredune
        }
        if (lee > 0 && h < 0 && h > -6) {   // the barrier island: a ridge 300-600 m off the shore where the shallows are flat
            double dOff = -h / 0.008;   // metres offshore for a 0.8% shelf
            double band = std::exp(-((dOff - 450) * (dOff - 450)) / (130.0 * 130.0));
            double along = gnoise3(p / 1.2 + Vec3(7, 3, 1), g.sF + 46);
            h += (2.5 - h) * band * smoothstep(0.0, 0.35, along) * lee * f;
        }
    }
    // O6-05: cirques on the high ground of the cold country: hemispherical bites 600-1200 m across and 60-150 m deep
    // with a flat floor (the drainage fills a tarn where the floor is a bowl)
    if (relief0 > 0.3 && scaleVisible(3.0, detailM)) {
        double lf0 = std::fabs(lat) / DEG / 85.0 * g.latGradient;
        double snow0 = g.snowLine * std::max(0.0, 1.0 - lf0 * lf0);
        double high = smoothstep(snow0 - 700, snow0 - 250, h) * landMask;
        if (high > 0.01) {
            double bite = 0;
            visitFeatures(p, R, 3.0, 0.35, g.sE + 380, 0.25, [&](const FeatureHit& f) {
                double r = 0.3 + 0.3 * f.u[3], x = f.d / r;
                if (x >= 1) return;
                bite += (60 + 90 * f.u[4]) * std::max(0.0, 1 - x * x) * (1 - 0.5 * smoothstep(0.7, 1.0, x)) * featFade(2 * r, detailM);   // B-404: a cirque's own fade
            });
            h -= bite * high;
            hS -= bite * high * 0.7;
        }
    }
    // dune seas (M9-08) in dry lowlands
    if (moist < 0.28 && h > 5 && h < 700 && relief0 < 0.3) {
        const double share = 1 - smoothstep(0.24, 0.28, moist); double lee = 0;
        h += duneField(p, g, lat, 6.0 + 16.0 * (0.28 - moist) / 0.28, detailM, &lee) * share;
        o.slip = share * lee;   // Q-03
    }
    if (K.tags & KT_ICE_SHEETS) {   // P-02: the ice age's sheets stand over the land poleward of the cap's mean edge (no season: the ground
        // is the world's), a dome up to 2.2 km thick at fourteen degrees in, its margin steep (a parabolic profile, the ice's), the land's fine
        // relief buried; at the coast the sheet ends in a wall of ice. P-03: a terminator world's over its night side (`iceSheetIn`); a
        // snowball's surface smooth over the continents' base, its ranges standing out of it as nunataks
        double in = iceSheetIn(g, unit);
        if (in > 0 && landMask > 0.02) {
            double cover = smoothstep(0.02, 0.3, landMask);
            const double wallIn = K.sheetByCold ? 80.0 * g.subContrast / (25.0 * R * 1000.0) : 30.0 / (14.0 * DEG * R * 1000.0);   // the terminus: an ice cliff 20-55 m high over its first 30 m (Greenland's on land; P-03: a terminator world's over 80 m, its edge oblique to the grid)
            double thick = (K.sheetM * std::sqrt(std::min(1.0, in)) + (20 + 35 * (0.5 + 0.5 * gnoise3(p / 0.8, g.sF + 71))) * smoothstep(0.0, wallIn, in)) * cover;
            o.sheetWall = in < wallIn && cover > 0.5;
            if (K.nunataks) {
                const double surf = std::max(hCont, 0.0) + thick + 0.8 * fbm3(p / 0.12, g.sF + 72, 2) * lodFade(0.12, detailM);   // the sheet's own surface, its sastrugi
                if (h < surf) { o.iceSheet = surf - h; h = surf; }
                hS = std::max(hS, surf);
            } else {
                h = hS + (h - hS) * (1 - smoothstep(0.0, 0.03, in) * cover) + thick;
                hS += thick;
                o.iceSheet = thick;
            }
        }
    }
    o.h = h; o.hS = hS; o.moist = moist; o.landMask = landMask; o.relief0 = relief0; o.hills = hills; o.slope = rf.slope; o.gradT = rf.gradT; o.slopeM = rf.slopeM; o.gradM = rf.gradM; o.caldera = caldera;
}

// O6-03: lakes are no longer features with a level of their own. The 7 km lake cells of B-320 dig bowls into the ground
// and the smooth ground (6-14 m deep, 0.4-1.3 km across, an outline that wobbles round the centre) where the country is
// flat, wet and low, and the drainage's flood fills them to their spill: a lake's level and its shore come from the same
// flood as the rivers' (`drainageAt`). `lakeSiteOk` asks the ground at the bowl's centre (one entry per thread; never
// re-entered: the centre's ground is read without bowls)
static thread_local int t_noBowls = 0;
bool lakeSiteOk(const BodyGen& g, const Vec3& centreUnit, uint64_t id) {
    static thread_local uint64_t lastId = 0; static thread_local uint64_t lastSeed = 0; static thread_local bool lastOk = false;
    if (id == lastId && lastSeed == g.seed) return lastOk;
    if (t_noBowls) return false;
    t_noBowls++;
    FelGround o;
    felisianGround(g, centreUnit, 100.0, o);
    t_noBowls--;
    bool ok = o.landMask > 0.6 && o.hS > 15 && o.relief0 < 0.3 && o.moist > 0.42;   // flat country, wetter than the badlands (0.42)
    lastId = id; lastSeed = g.seed; lastOk = ok;
    return ok;
}
double lakeBowlAt(const BodyGen& g, const Vec3& p, double detailM) {
    if (t_noBowls || !scaleVisible(2.0, detailM)) return 0;
    double bowl = 0;
    visitFeatures(p, g.R, 7.0, g.lakeDensity, g.sF + 50, 0.32, [&](const FeatureHit& f) {
        double r = f.cellKm * (0.06 + 0.13 * f.u[3]);
        if (f.d > r * 1.5) return;
        Vec3 cn = normalize(f.c);
        Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
        Vec3 d = p - f.c;
        double ang = std::atan2(dot(d, e2), dot(d, e1));
        double rr = r * (0.8 + 0.2 * std::sin(3 * ang + f.u[0] * TAU) + 0.12 * std::sin(5 * ang + f.u[1] * TAU) + 0.08 * std::sin(7 * ang + f.u[2] * TAU));
        double x = f.d / rr;
        if (x > 1.35) return;
        uint64_t id = hash3i((int64_t)std::llround(f.c.x * 64), (int64_t)std::llround(f.c.y * 64), (int64_t)std::llround(f.c.z * 64), 0x1A4EULL);
        if (!lakeSiteOk(g, cn, id)) return;
        double depth = 9.0 + 10.0 * f.u[4];
        bowl = std::max(bowl, depth * smoothstep(1.35, 0.55, x));
    });
    return bowl;
}

// O6-03: the drainage cut into the ground at a point (and, given `hS`, into the smooth ground). The valley melts the
// ground toward the bank height (the reach's water plus a metre) within 25 widths of the channel, the floodplain within
// 2.2 half-widths, the channel is carved `depth` under the banks within 1.15, and the water plane lies at `level` over
// the whole plain (B-322's `shore`, `bankFloor` and `shoreOut` as before, the edge where the channel is a metre deep); a
// filled depression is a lake on a wet world (the water half a metre under the spill, the shore the fine ground's
// contour, the dry ground in the mask lifted over the plane) and a playa flattened at that level on a dry one
struct DrainApply {
    double level = -1e9, zone = 0, shore = 1e9, bankFloor = -1e9, shoreOut = 1e9;
    double lake = 0, bed = 0, bank = 0, wetness = 0, playa = 0;   // the lake mask, the channel bed (0..1), the banks' reach (moisture), the valley floor, a dry basin's flat (0..1)
    int kind = 0;   // 1 a river, 2 a lake
};
void applyDrainage(const BodyGen& g, const Vec3& unit, double detailM, double& h, double* hS, double slope, bool wet, bool lakesWet, double moist, DrainApply& A, double uShape = 0) {
    (void)slope;
    DrainInfo D = drainageAt(g, unit, detailM, moist);
    double lakeM = D.lake > 0.3 ? smoothstep(0.3, 0.6, D.lake) : 0.0;
    A.wetness = D.wetness;
    if (D.valley > 0) {   // the valley: the ground melts toward its channel's bank height. O6-05: a glaciated valley has the U's flat floor
        double vz = D.valley * (1 - lakeM), bankV = D.valleyLevel + 1.0;
        if (uShape > 0) vz = std::max(vz, std::min(1.0, D.valley * 1.6) * uShape * (1 - lakeM));
        h += (bankV - h) * vz;
        if (hS) *hS += (bankV - *hS) * vz;
    }
    if (D.any) {
        double wh = D.halfWidth, bankH = D.level + 1.0;
        double zone = smoothstep(wh * 2.2, wh * 0.9, D.dist) * D.pres * (1 - lakeM);
        h += (bankH - h) * zone;
        if (lakeM < 0.5) {
            double depth = D.depth * D.pres;
            double v = clampd(1.0 / std::max(depth, 1e-3), 0.0, 1.0), tt = 0.5 - std::sin(std::asin(1.0 - 2.0 * v) / 3.0);
            double shoreM = D.dist - wh * (1.15 - 0.65 * tt);
            // B-322: every point that knows its channel measures the edge in the same metres (a corner beyond the plane
            // without an outline used to fall back on its height over a neighbour's plane, and a dip beside a steep river
            // read as wet: a dark triangle on the bank); the bank stands over the plane out to 3.5 half-widths
            if (shoreM > 0) { A.shoreOut = std::min(A.shoreOut, shoreM); if (D.dist < wh * 3.5) A.bankFloor = std::max(A.bankFloor, D.level + 0.05 + 0.3 * clampd(shoreM / 10.0, 0, 1)); }
            if (D.dist < wh * 3.5) {
                double chan = smoothstep(wh * 1.15, wh * 0.5, D.dist) * D.pres;
                h -= chan * depth;
                A.bed = chan;
                if (zone > 0) { A.level = D.level; A.zone = zone; A.kind = 1; A.shore = shoreM; }
                A.bank = smoothstep(wh * 3.5, wh * 1.2, D.dist) * D.pres;
            }
        }
    }
    if (D.lake > 0.02) {
        // the lake reaches over the whole fade of the flood's cell mask (a cell's centre is flagged when it lies under the
        // fill; the true shore lies between the last flagged centre and the first unflagged one), and within it the water
        // is wherever the fine ground lies under the level: the mask's own 0.5 contour cut a lake 13 m deep off along a wall
        A.lake = lakeM;
        double lv = D.lakeLevel;
        double reach = smoothstep(0.02, 0.12, D.lake);
        if (wet && lakesWet) {
            double sh = std::max((h - lv) * 20.0, (0.06 - D.lake) * 400.0);   // the height over the level in a lake's own metres (one scale for the whole lake, so the contour is the height's contour), or the fade's outer edge
            A.level = lv; A.zone = std::max(A.zone, reach); A.kind = 2; A.shore = sh;
            if (sh > 0) A.bankFloor = std::max(A.bankFloor, lv + 0.05 + 0.3 * clampd(sh / 10.0, 0, 1));
        } else {
            double target = lv - 0.5;   // a playa: dead flat at the fill level, the ground under a metre over it melted down
            A.playa = reach * smoothstep(lv + 1.0, lv - 0.5, h);
            if (h < lv + 1.0) h += (target - h) * A.playa;
            if (hS && *hS < lv + 1.0) *hS += (target - *hS) * reach * smoothstep(lv + 1.0, lv - 0.5, *hS);
        }
    }
    if (wet) {   // B-322: the bank stands over its plane, the channel lies under it
        if (A.level > -1e8 && A.shore < 1e8 && A.shore <= 0) h = std::min(h, A.level - 0.3);
        else if (A.bankFloor > -1e8) h = std::max(h, A.bankFloor);
    }
}

void felisianSample(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s) {
    const double R = g.R;
    const KindDef& K = KINDS[g.type];
    Vec3 p = unit * R;
    double lat = std::asin(clampd(unit.z, -1, 1));
    double absLatDeg = std::fabs(lat) / DEG;
    FelGround o;
    felisianGround(g, unit, detailM, o);
    double h = o.h, hS = o.hS, moist = o.moist, landMask = o.landMask, relief0 = o.relief0, hills = o.hills;
    double sub = g.locked ? dot(unit, g.lockedDir) : 0.0;   // M5-08: the sub-stellar point's side (the glacial valleys below read the climate too)
    // M9-09 seasons: the winter hemisphere's cap grows and its snow line drops
    double winter = clampd((lat >= 0 ? -g.season : g.season), 0, 1);   // 0..1 how deep in winter this hemisphere is
    double capEdge = g.iceCapLat + 6.0 * gnoise3(p / (R * 0.15), g.sF + 7) - 11.0 * winter + 4.0 * clampd((lat >= 0 ? g.season : -g.season), 0, 1);
    // B-321: the snow line falls with the square of the latitude (Earth: 5000 m at the equator, 3000 at 45, 1000 at 65,
    // nothing at 85); the old `40 m per degree` put every hill above 500 m under snow from 50 degrees on, and two
    // thirds of the felisian land came out as ice sheet
    double lf = absLatDeg / 85.0 * g.latGradient;   // P-02: a kind's climate contrast moves its snow line's fall with latitude too
    double snowLine = g.snowLine * std::max(0.0, 1.0 - lf * lf) - 900.0 * winter;
    // O6-04: the slope's aspect: a flank facing the equator keeps its snow line up to 8% higher, a shaded one lower
    double steep = clampd(o.slopeM * (1 + 1.6 * o.cliff), 0, 4);   // B-404: the material chain's slope: the rock and the scree sit on the same cells in every ring
    {
        Vec3 pole(0, 0, 1);
        Vec3 north = pole - unit * unit.z;
        double nl = length(north);
        if (nl > 1e-6 && o.slopeM > 0.02) {
            Vec3 eq = north / nl * (lat >= 0 ? -1.0 : 1.0);   // toward the equator
            double facing = dot(normalize(o.gradM * -1.0), eq) * clampd(o.slopeM / 0.3, 0, 1);   // downhill toward the equator: sun-facing (B-404: the material chain's)
            snowLine *= 1.0 + 0.08 * facing;
        }
    }
    moist -= 0.12 * smoothstep(0.25, 0.5, o.slopeM) * smoothstep(0.3, 0.6, relief0);   // O6-04: the ridges are the dry ground (the valley floors gained above); B-404: the material chain's slope
    // B-320: inland water. A river is a ribbon cut into the smooth ground along a hashed network (worley cell boundaries
    // at 80 km): across its floodplain the fine relief melts into `hS`, the channel is carved 2.5-11.5 m into it and the
    // water lies a metre under the smooth ground, flat across the ribbon and following the valley along it; the plane
    // extends under the banks (`s.water` is set over the whole zone) and the ground above it wins by depth. It used to
    // be a channel a few metres deep in the bumpy ground with the water a few metres over each vertex: a scatter of
    // squares. Lakes and wetland pools are cut into the same smooth ground with one flat level each
    double level = -1e9, waterZone = 0, riverBank = 0, shore = 1e9, bankFloor = -1e9, shoreOut = 1e9, wetness = 0; int waterKind = 0; bool dryLake = false;   // shoreOut: the edge's distance beyond the plane's zone (B-322)
    if (landMask > 0.2 && hS > -60) {   // O6-03: the drainage's rivers and lakes (the worley nets and the lake features of B-320 are gone)
        DrainApply A;
        // O6-05: where the climate is cold the valleys are glacial: U-shaped, and, reaching the sea, fjords (the valley's floor
        // keeps going under the water)
        double uShape = smoothstep(2.0, -6.0, climateTempC(g, absLatDeg, std::max(0.0, hS), 0, sub)) * smoothstep(0.15, 0.4, relief0);
        double hBefore = h;
        applyDrainage(g, unit, detailM, h, &hS, o.slope, true, moist > 0.3, moist, A, uShape);   // a dry basin is a playa, not a lake
        if (uShape > 0.3 && hBefore < 5 && A.wetness > 0.2) { double fj = 45.0 * A.wetness * uShape * smoothstep(5.0, -10.0, hBefore); h -= fj; hS -= fj; }   // the fjord
        if (A.wetness > 0.2 && A.lake < 0.5) {   // O6-07: the floodplain's own relief, bars and levee scars of a metre on 90 and 30 m (a dead-flat floor read a 0.007 median grade over the plains; the banks within 3.5 half-widths stay clean)
            double fm = smoothstep(0.2, 0.6, A.wetness) * (1 - A.lake) * (1 - A.bank) * landMask;
            auto ff = [&](double wlKm) { return smoothstep(1.5, 4.0, wlKm * 1000.0 / detailM); };   // full when the feature spans four samples
            double mr = 1.2 * gnoise3(p / 0.09 + Vec3(3.1, 1.4, 7.7), g.sB + 101) * ff(0.09) + 0.4 * gnoise3(p / 0.03 + Vec3(0.4, 6.2, 2.5), g.sB + 102) * ff(0.03);
            h += mr * fm;
        }
        dryLake = A.playa > 0.3;
        if (landMask > 0.35) { level = A.level; waterZone = A.zone; waterKind = A.kind; shore = A.shore; bankFloor = A.bankFloor; shoreOut = A.shoreOut; riverBank = A.bank; wetness = A.wetness; }
    }
    if (o.land.riftFloor > 0.3 && level < -1e8 && moist > 0.3 && hS > 2) {   // R-305: the rift floor's lakes, three metres deep
        level = hS - 1.0; waterZone = std::max(waterZone, o.land.riftFloor); waterKind = 4;
    }
    if (landMask > 0.5 && level < -1e8 && moist > 0.7 && hS < 120 && hS > 1 && relief0 < 0.2 && hills < -0.15) {   // wetland pools
        double pool = smoothstep(-0.15, -0.4, hills);
        h += (hS - 0.5 - 1.5 * pool - h) * pool;
        level = hS + 0.2; waterZone = std::max(waterZone, pool); waterKind = 3;
    }
    moist += 0.25 * riverBank + 0.15 * wetness;   // O6-03: the valley floors are the wetter ground
    // B-322: a river's or a lake's bank stands over its plane (out to 3.5 half-widths or 1.6 radii, `bankFloor`) and the
    // channel lies under it, so the water's edge is the outline (`shore`) and nothing else: the fine relief of the outer
    // floodplain used to dip under the plane, and the plane showed through it in flat patches a hundred metres wide with
    // edges snapped to the mesh, and again as a moat of cells along the plain's edge
    if (level > -1e8 && shore < 1e8 && shore <= 0) h = std::min(h, level - 0.3);
    else if (bankFloor > -1e8) h = std::max(h, bankFloor);
    double reefTint = 0;
    if (h < 0 && g.hasTrait(TR_REEFS) && absLatDeg < capEdge) {
        // Q-02: a barrier reef in the warm seas (the Great Barrier Reef's): along the shelf's edge where it falls past 18-40 m (a contour, so every
        // ring and the globe agree, B-404) its crest stands awash, the fore-reef dropping seaward under it, a few passes cut through it; behind it
        // the lagoon's floor of white sand lifted to a few metres, its water turquoise (`lifeTint`); in a province of the coasts where the sea is
        // warm enough (22 C at its level)
        const double warm = smoothstep(20.0, 23.0, climateTempC(g, absLatDeg, 0, 0, sub)) * smoothstep(-0.2, 0.15, fbm3(p / (R * 0.22) + Vec3(13.1, 3.7, 9.2), g.sG + 320, 2));
        if (warm > 0.5) {
            const double D = 18.0 + 22.0 * unitFromHash(mix64(g.seed ^ 0x2EEFULL));
            if (h > -D) { h = -(0.8 + 5.0 * (-h / D)); reefTint = 1; }
            else if (h > -D - 30.0 && fbm3(p / 1.8, g.sG + 321, 2) > -0.3) { const double k = smoothstep(-D - 30.0, -D - 8.0, h); h += (-0.35 - h) * k; reefTint = h > -4.0 ? 1.0 : 0.0; }   // the crest's shallows turquoise, the fore-reef's drop the deep's
        }
    }
    s.height = h;
    s.relief = clampd(relief0 + 0.5 * o.slope, 0, 1);
    // M9-05 biomes from temperature (latitude, altitude, the body's climate) and moisture
    double T = climateTempC(g, absLatDeg, h, winter, sub);
    if (g.locked && !(K.tags & KT_FIXED_SUN)) moist -= 0.35 * std::max(0.0, sub - 0.15);   // M5-08: deserts under a permanent sun (P-03: a fixed sun's kind has its own, `felisianGround`)
    int biome = BIO_GRASSLAND;
    if (T < -8) biome = BIO_ICE;
    else if (T < 2) biome = BIO_TUNDRA;
    else if (T < 10) biome = moist > 0.45 + g.grassBand ? BIO_TAIGA : BIO_TUNDRA;   // P-02: a kind's grass band wider (forest wetter, desert drier)
    else if (T < 20) biome = moist > 0.55 + g.grassBand ? BIO_TEMPERATE : (moist > 0.28 - g.grassBand ? BIO_GRASSLAND : BIO_DESERT);
    else biome = moist > 0.55 + g.grassBand ? BIO_TROPICAL : (moist > 0.35 - g.grassBand ? BIO_SAVANNA : BIO_DESERT);
    if (moist > 0.72 && h < 120 && relief0 < 0.2 && biome != BIO_ICE) biome = BIO_WETLAND;
    if (relief0 > 0.45 && h > std::max(700.0, 0.5 * snowLine)) biome = BIO_ALPINE;   // B-321: the zone between the treeline and the snow (it began at 700 m everywhere: 13% of the land was bare rock)
    bool inland = level > -1e8 && h < level;   // under a river, a lake or a pool
    if (h < 0) {
        s.material = MAT_WATER; s.water = 0; s.biome = BIO_OCEAN;
        s.albedo = h > -250 ? 0.36 : 0.22;
        s.lifeTint = reefTint;   // Q-02: a lagoon's or a reef's
        if (g.seaDepth < 1 && h > -40 && h < -1.5) s.albedo += 0.12 * smoothstep(-40, -12, h) * smoothstep(-1.5, -4, h);   // P-02: the reefs fringing a shelf sea's coasts, pale under the water
        if (absLatDeg > capEdge + 4 || (g.locked && T < -10)) { s.material = MAT_ICE; s.albedo = 0.85; s.height = 0; s.biome = BIO_ICE; }
        else if (K.daySeasDry && g.locked) {   // P-03: the seas boiled away where the day is too hot to hold them (the climate at the sea's level),
            // their floors dry: salt in the deeps, sand on the old shelves
            const double dry = smoothstep(30.0, 36.0, climateTempC(g, absLatDeg, 0, 0, sub) + 3.0 * hills);
            if (dry > 0.5) { s.material = h < -120.0 ? MAT_SALT : MAT_SAND; s.albedo = s.material == MAT_SALT ? 0.80 : 0.60; s.water = -1e9; s.biome = BIO_DESERT; }
        }
        if (K.seaGlaciers) {   // P-03: a snowball's frozen seas: glacier ice standing 6-14 m over the water at its edge and rising poleward, its seams
            // ridged, wind-scoured to blue ice in patches; before its edge a band of floes three degrees wide on the slushy sea
            const double over = absLatDeg - (capEdge + 4);
            if (over > 0) {
                const double wallDeg = 30.0 / (DEG * R * 1000.0), cliff = 6.0 + 8.0 * (0.5 + 0.5 * gnoise3(p / 2.0, g.sF + 73));
                double hg = (cliff + 0.6 * std::min(over, 40.0)) * smoothstep(0.0, wallDeg, over);
                if (scaleVisible(1.5, detailM)) { const Worley3 fl = worley3(p / 1.5, g.sF + 3); hg += 3.0 * smoothstep(0.03, 0.0, fl.f2 - fl.f1) * lodFade(1.5, detailM); }   // pressure ridges
                hg += 0.6 * fbm3(p / 0.12, g.sF + 72, 2) * lodFade(0.12, detailM);   // sastrugi
                s.height = hg; s.biome = BIO_ICE;
                const bool blue = fbm3(p / 0.9, g.sF + 74, 2) > 0.38;   // unfaded: the same patches in every ring
                s.material = blue ? MAT_ICE : MAT_SNOW; s.albedo = blue ? 0.62 : 0.88;
            } else if (over > -3.0) {   // the floes: the ocean's plates on a shelf under the slush (the water's edge the plane's contour on the plates)
                const double cover = smoothstep(-3.0, 0.0, over), shelf = smoothstep(0.0, 0.04, cover);
                const double floor = h + (-6.5 - h) * shelf;
                s.height = floor;
                const Worley3 fl = worley3(p / 1.5, g.sF + 3);
                if (unitFromHash(fl.id1) < cover) {
                    const double d = fl.f2 - fl.f1, rim = smoothstep(0.0, 0.13, d), top = 0.9 + 0.35 * smoothstep(0.13, 0.3, d);
                    const double hf = floor + (top - floor) * rim;
                    s.height = hf;
                    if (hf > -0.3) { s.material = MAT_ICE; s.albedo = 0.86; s.biome = BIO_ICE; }
                }
            }
        }
    } else if (inland) {
        s.material = T < -2 ? MAT_ICE : MAT_WATER; s.water = level; s.albedo = 0.3; s.biome = (biome == BIO_ICE || biome == BIO_TUNDRA) ? biome : BIO_WETLAND;
        if (T < -2) { s.height = level; s.albedo = 0.8; }
    } else if (absLatDeg > capEdge || h > snowLine || biome == BIO_ICE || o.iceSheet > 1) {   // P-02: an ice sheet's ground is its ice whatever the season
        s.material = MAT_SNOW; s.albedo = 0.92; s.biome = BIO_ICE;
        if (o.iceSheet > 1 && o.sheetWall) { s.material = MAT_ICE; s.albedo = 0.7; }   // the sheet's wall bare blue ice (P-03: its own wall, not the bedrock's slope under it)
        if (waterZone > 0) s.water = level;
    } else if (o.caldera && h > 0) {
        s.material = MAT_WATER; s.albedo = 0.3; s.water = h + 2.0; s.biome = BIO_WETLAND;   // crater lake
    } else if (biome == BIO_ALPINE) {
        s.material = MAT_ROCK; s.albedo = 0.48; s.veg = 0.05; s.biome = BIO_ALPINE;
    } else if (waterZone > 0.3 && h < level + 1.2 && T > -2) {   // the bars and beaches of inland water
        s.material = MAT_SAND; s.albedo = 0.58; s.veg = 0.05; s.biome = biome; s.water = level;
    } else if (h < 12 + 10 * hills) {
        s.material = MAT_SAND; s.albedo = 0.62; s.veg = 0.08; s.biome = biome;
    } else switch (biome) {
        case BIO_DESERT: s.material = MAT_SAND; s.albedo = 0.66; s.veg = 0.02; break;
        case BIO_TROPICAL: s.material = MAT_FOREST; s.albedo = 0.28; s.veg = 0.85 + 0.15 * clampd((moist - 0.55) / 0.3, 0, 1); break;
        case BIO_TEMPERATE: s.material = MAT_FOREST; s.albedo = 0.31; s.veg = 0.6 + 0.3 * clampd((moist - 0.55) / 0.3, 0, 1); break;
        case BIO_TAIGA: s.material = MAT_FOREST; s.albedo = 0.34; s.veg = 0.45 + 0.2 * clampd((moist - 0.45) / 0.3, 0, 1); break;
        case BIO_SAVANNA: s.material = MAT_GRASS; s.albedo = 0.46; s.veg = 0.3; break;
        case BIO_WETLAND: s.material = MAT_GRASS; s.albedo = 0.38; s.veg = 0.5; break;
        case BIO_TUNDRA: s.material = (0.5 + 0.5 * hills) * 0.6 + 0.4 * winter > 0.62 ? MAT_SNOW : MAT_GRASS; s.albedo = s.material == MAT_SNOW ? 0.9 : 0.4; s.veg = 0.1; break;
        default: s.material = MAT_GRASS; s.albedo = 0.42; s.veg = 0.25 + 0.3 * clampd((moist - 0.3) / 0.3, 0, 1); break;
    }
    if (s.material != MAT_WATER && s.material != MAT_ICE && waterZone > 0 && s.water < -1e8) s.water = level;   // the plane under the banks
    if (s.biome == BIO_NONE) s.biome = biome;
    // R-305: the landforms' materials: glacier ice, and rock, basalt, sand or salt where a landform says so (never over water)
    if (dryLake && s.material != MAT_WATER && s.material != MAT_SNOW) { s.material = MAT_SALT; s.albedo = 0.86; s.veg = 0; }   // O6-03: a dry basin's playa
    if (o.land.glacier > 0.5 && s.material != MAT_WATER) { s.material = MAT_ICE; s.albedo = 0.8; s.veg = 0; }
    else if (o.land.mat >= 0 && s.material != MAT_WATER && s.material != MAT_ICE && s.material != MAT_SNOW) { s.material = o.land.mat; if (o.land.albedo >= 0) s.albedo = o.land.albedo; s.veg *= (o.land.mat == MAT_SAND ? 0.3 : 0.0); }
    // O6-04: bare rock where the ground is too steep to hold soil (over 1:1.6, and every riser of the cliff bands), scree on
    // the flanks under it and on the talus: boulder fields, next to no flora
    if (s.material != MAT_WATER && s.material != MAT_ICE && s.material != MAT_SNOW && s.material != MAT_SALT && h > 12 && !inland) {
        if (steep > 0.62 || o.cliff > 0.4) { s.material = MAT_ROCK; s.albedo = 0.46 - 0.1 * o.cliff; s.veg = 0.03; }
        s.scree = clampd(std::max(o.talus, smoothstep(0.42, 0.62, steep) * 0.7), 0, 1);
        if (s.scree > 0.3) s.veg *= 1 - 0.7 * s.scree;
    }
    if (!(K.tags & (KT_FLORA | KT_MICROBIAL)) && K.cover == DC_NONE && (s.material == MAT_FOREST || s.material == MAT_GRASS)) {   // P-03: a living ground with nothing
        // growing on it (the snowball's): where the plants would stand, frost-shattered gravel where it is wet enough, bare rock where not
        s.material = moist > 0.4 ? MAT_SAND : MAT_ROCK; s.albedo = s.material == MAT_SAND ? 0.5 : 0.44; s.veg = 0;
    }
    if (K.nunataks && o.iceSheet <= 1 && s.material == MAT_SNOW && steep > 0.45 && h > 0) { s.material = MAT_ROCK; s.albedo = 0.34; }   // P-03: the nunataks' flanks scoured bare
    if (K.tags & KT_MICROBIAL) {   // P-02: life without plants (the archean). Where a felisian world grows its plants the wetter
        // ground is mats (the land's colour from afar), the drier bare: regolith where it is wet enough to weather, rock where not; the flats
        // along the shore a metre or two over the sea and the bars of the rivers and lakes are mats, and along the shore a reef of
        // stromatolites in patches a few hundred metres long
        const bool plant = s.material == MAT_FOREST || s.material == MAT_GRASS;
        const bool flat = s.material == MAT_SAND && h >= 0 && (h < 3.0 + 1.5 * hills || (waterZone > 0.3 && s.water > -1e8 && h < s.water + 1.2));
        if (plant && (s.biome == BIO_WETLAND || riverBank > 0.35 || (moist > 0.5 && h < 900 && relief0 < 0.45))) { s.material = MAT_MATS; s.albedo = 0.22 + 0.04 * hills; }   // the wet land, its banks
        else if (plant) { s.material = moist > 0.4 && s.biome != BIO_ALPINE ? MAT_DUST : MAT_ROCK; s.albedo = s.material == MAT_DUST ? 0.38 : 0.42; }
        else if (flat && T > 0) {
            const double reef = waterZone > 0.3 ? -1.0 : fbm3(p / 0.22, g.sG + 77, 2);   // the reefs on the sea's flats, by an unfaded field (the same in every ring)
            if (reef > 0.08) { s.material = MAT_STROMATOLITE; s.albedo = 0.40; } else { s.material = MAT_MATS; s.albedo = 0.24; }
        }
        s.veg = 0;
    }
    if (s.material != MAT_WATER) s.albedo += 0.08 * clampd(h / 4000.0, 0, 1);
    s.landform = 1000 * o.land.mark + std::fabs(o.land.dhSmooth) + std::fabs(o.land.dhFine) + 40 * o.land.flatten;
    s.landformTrait = o.land.mark >= 0.5 ? o.land.markTrait : 0;   // Q-01
    s.slipFace = std::max(o.slip, o.land.slip);   // Q-03
    s.groundTint = o.land.tint;   // Q-02
    if (o.land.life > 0 && s.material != MAT_WATER) s.lifeTint = o.land.life;
    if (s.water > 0.5 && waterZone > 0) s.waterKind = waterKind;
    if (waterZone > 0 && s.water > -1e8) s.shore = shore;   // B-322 (a snowfield or a glacier over the zone keeps the plane, so it keeps the edge too)
    else if (s.water < -1e8 && shoreOut < 1e8) s.shore = shoreOut;   // the bank beyond the plane's reach still knows how far the water is
    if (K.cover != DC_NONE) {   // P-04: a dead world, the present's cover over the living ground it was (above was its then-world's: the land's heights are
        // the same in both times, so its people's towns stand where they stood). No liquid is left; the old forests' density stays in `veg` (their dead trees)
        const double oldForest = s.material == MAT_FOREST ? s.veg : 0.0;
        const int was = s.material;
        s.water = -1e9; s.waterKind = 0; s.shore = 1e9; s.veg = 0;
        if (K.cover == DC_SCORCHED) {   // baked by its swelling sun: the seas boiled off to their salt, the soils cracked clay and dust, the forests stone; a
            // cold one's (a white dwarf's, a giant's far world) frost where its present's mean falls under freezing
            const double tNow = g.nowTempC - 0.4 * absLatDeg - 6.5 * std::max(0.0, h) / 1000.0 + (g.locked ? 25.0 * sub : 0.0);   // a locked one's night side the colder
            s.biome = BIO_DESERT;
            if (h < 0) {   // the old sea's floor, dry at its depth far under the shores: the deep basins' salt, the slopes' silt, reef limestone on the warm shelves
                s.height = h; s.oldSea = 1;
                const double depth = -h;
                if (depth > 1500.0 + 600.0 * fbm3(p / 60.0, g.sG + 82, 2)) { s.material = MAT_SALT; s.albedo = 0.84; }
                else if (depth < 110.0 && climateTempC(g, absLatDeg, 0, 0, sub) > 17.0 && fbm3(p / 120.0, g.sG + 84, 2) + (matVisible(5.0, detailM) ? 0.5 * fbm3(p / 9.0, g.sG + 81, 2) : 0.0) + (matVisible(0.35, detailM) ? 0.3 * fbm3(p / 0.7, g.sG + 83, 2) : 0.0) > -0.15) {
                    s.material = MAT_STROMATOLITE; s.albedo = 0.66;   // the reefs' fields, their edges where the eye sees them
                }
                else { s.material = MAT_DUST; s.albedo = 0.42 + 0.08 * smoothstep(1500.0, 200.0, depth); }
            } else if (inland) {   // the rivers' beds sand and gravel, the lakes' playas salt, the pools' clay
                s.height = h;
                if (waterKind == 2 || waterKind == 4) { s.material = MAT_SALT; s.albedo = 0.8; }
                else if (waterKind == 3) { s.material = MAT_MUD; s.albedo = 0.46; }
                else { s.material = MAT_SAND; s.albedo = 0.56; }
            } else switch (was) {
                case MAT_FOREST: s.material = MAT_DUST; s.albedo = 0.46; s.veg = oldForest; break;   // the stone forest's floor (its trunks the trees')
                case MAT_GRASS: s.material = moist > 0.38 ? MAT_MUD : MAT_DUST; s.albedo = s.material == MAT_MUD ? 0.46 : 0.5; break;   // the soils cracked, the wet ones clay
                case MAT_SNOW: case MAT_ICE: s.material = steep > 0.4 ? MAT_ROCK : MAT_DUST; s.albedo = s.material == MAT_ROCK ? 0.4 : 0.48; break;   // the old ice's ground, moraine
                case MAT_WATER: s.material = MAT_SALT; s.albedo = 0.8; break;   // a crater lake's
                default: break;
            }
            if (tNow < -2.0 + 4.0 * hills && (s.material == MAT_SALT || s.material == MAT_DUST || s.material == MAT_MUD || s.material == MAT_SAND)) { s.material = MAT_SNOW; s.albedo = 0.86; }
        } else {   // lost its sun: the seas ice at their level (pressure ridges along the floes' seams, snow drifted over them), the rivers and lakes ice,
            // the land under the frost of its fallen air (thicker toward the poles), bare dark rock on the steep, the dead soil where the frost lies thin
            const double polar = smoothstep(20.0, 70.0, absLatDeg);
            s.biome = BIO_ICE;
            if (h < 0) {
                double hi = 0.3 * fbm3(p / 0.15, g.sF + 72, 2) * lodFade(0.15, detailM);
                if (scaleVisible(1.5, detailM)) { const Worley3 fl = worley3(p / 1.5, g.sF + 3); hi += 2.5 * smoothstep(0.03, 0.0, fl.f2 - fl.f1) * lodFade(1.5, detailM); }
                s.height = hi; s.oldSea = 1;
                const double drifts = fbm3(p / 250.0, g.sF + 79, 2) + (matVisible(12.0, detailM) ? 0.6 * fbm3(p / 25.0, g.sF + 75, 2) : 0.0) + (matVisible(0.4, detailM) ? 0.35 * fbm3(p / 0.8, g.sF + 77, 2) : 0.0);
                const bool drift = drifts + 0.5 * polar > 0.32;   // the drifts' fields of hundreds of kilometres, their edges where a ring can draw them (the map's texels are wider)
                s.material = drift ? MAT_SNOW : MAT_ICE; s.albedo = drift ? 0.86 : 0.6;
            } else if (inland) { s.height = level; s.material = MAT_ICE; s.albedo = 0.6; }
            else if (steep > 0.62 || o.cliff > 0.4) { s.material = MAT_ROCK; s.albedo = 0.3; }
            else if (fbm3(p / 400.0, g.sF + 80, 2) + (matVisible(20.0, detailM) ? 0.5 * fbm3(p / 40.0, g.sF + 76, 2) : 0.0) + (matVisible(1.2, detailM) ? 0.3 * fbm3(p / 2.5, g.sF + 78, 2) : 0.0)
                     - 0.7 * polar - 0.25 * smoothstep(300.0, 1500.0, h) > 0.32) {   // thin frost (its fields as the drifts')
                s.material = MAT_DUST; s.albedo = 0.3; s.veg = oldForest;
            }
            else { s.material = MAT_SNOW; s.albedo = 0.88; s.veg = oldForest; }
        }
    }
}
}

double sampleCloudPattern(const BodyGen& g, double lon, double lat) {
    if (g.cloudCover <= 0) return 0;
    // sample on the sphere so the pattern wraps seamlessly
    Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
    double n = fbm3(u * g.cloudScale, g.sG, 4, 2.1, 0.55);
    double bands = 0.15 * std::sin(lat * 6.0 + n * 3.0);
    double v = n * 0.5 + 0.5 + bands;
    if (kindHas(g.type, KT_CLOUD_SWIRL)) {
        double swirl = fbm3(u * (g.cloudScale * 2.0) + Vec3(0, 0, lat * 3.0), g.sG + 5, 3);
        return clampd(0.55 + 0.45 * (0.5 * n + 0.5 * swirl) + bands, 0.15, 1.0);
    }
    double thr = 1.0 - g.cloudCover;
    return clampd((v - thr) / std::max(0.05, 1.0 - thr) * 1.6, 0.0, 1.0);
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

static constexpr double SPRING_CELL = 0.5, SPRING_DENS = 0.55;   // P-03: the chemotroph's springs' grid (its ground and `springVents`)
void springVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out) {
    out.clear();
    Vec3 p = unit * g.R;
    visitFeatures(p, g.R, SPRING_CELL, SPRING_DENS, g.sF + 33, 4.0, [&](const FeatureHit& f) {
        GeyserVent v; v.unit = normalize(f.c); v.radiusKm = 0.012 + 0.018 * f.u[3];
        v.id = hash3i((int64_t)std::llround(f.c.x * 256), (int64_t)std::llround(f.c.y * 256), (int64_t)std::llround(f.c.z * 256), 0x5B12ULL);
        out.push_back(v);
    });
}

// P-05: the cryovolcanic world's plume vents (`groundCryovolcanic`'s streaks start at them, `plumeVents` finds them for the surface) on a grid
// of 80 km cells over its cap, and the dry ice world's spiders (`groundDryIce`, `springJets`) on cells of 0.8 km in the band round its cap
static constexpr double PLUME_CELL = 120.0, SPIDER_CELL = 0.8, SPIDER_DENS = 0.45;
static double plumeDensity(const BodyGen& g) { return 0.35 * g.volcanoes; }
static double plumeStreakKm(double u) { return 45.0 + 65.0 * u; }   // a streak's length, within a cell (`visitFeatures` reaches no further)
Vec3 nitrogenBasinOf(const BodyGen& g) {   // the nitrogen world's great plain (its middle, a unit vector: within 40 degrees of the equator)
    const double la = std::asin(clampd(0.64 * (2 * unitFromHash(mix64(g.seed ^ 0x51A1ULL)) - 1), -1, 1)), lo = unitFromHash(mix64(g.seed ^ 0x51A2ULL)) * TAU - PI;
    return StarSystem::bodyFromLatLon(la, lo);
}
// the edge of a cryovolcanic cap (degrees of latitude, south of it the cap) and the dry ice cap's (poleward of it) at a point
static double cryoCapEdge(const BodyGen& g, const Vec3& unit) { return g.iceCapLat + 4.0 * gnoise3(unit * 6.0, g.sF + 51); }
static double dryCapEdge(const BodyGen& g, const Vec3& unit) { return g.iceCapLat + 5.0 * gnoise3(unit * g.R / (g.R * 0.15), g.sF + 7); }   // `groundThinAir`'s
static double dryFrostDeg(const BodyGen& g, bool north) { return 12.0 * smoothstep(0.6, -0.4, north ? g.season : -g.season); }   // the seasonal frost's reach past the cap (degrees): 12 in winter, none by midsummer
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
// P-06: the patchwork's coronae: up to three ovals 0.26-0.38 of the radius across their long axis, squared off at the corners (a superellipse
// of exponent 2.5-4), each clear of the others; in the gnomonic plane of its middle (km along `e1` and `e2`)
struct Corona { Vec3 c, e1, e2; double a = 0, b = 0, n = 2, sp = 8, amp = 400, band = 0; };
static int coronaeOf(const BodyGen& g, Corona out[3]) {
    auto hu = [&](int k) { return unitFromHash(mix64(g.seed ^ (0x9A7C0ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
    int n = 0;
    for (int t = 0; t < 24 && n < 3; t++) {
        const double z = 2 * hu(10 * t) - 1, az = hu(10 * t + 1) * TAU, rr = std::sqrt(std::max(0.0, 1 - z * z));
        Corona C; C.c = Vec3(rr * std::cos(az), rr * std::sin(az), z);
        C.a = g.R * (0.26 + 0.12 * hu(10 * t + 2)); C.b = C.a * (0.6 + 0.3 * hu(10 * t + 3));
        bool clear = true;
        for (int j = 0; j < n; j++) if (std::acos(clampd(dot(C.c, out[j].c), -1, 1)) * g.R < 1.15 * (C.a + out[j].a)) clear = false;
        if (!clear) continue;
        const Vec3 t1 = normalize(cross(C.c, std::fabs(C.c.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), t2 = cross(C.c, t1);
        const double rot = hu(10 * t + 4) * PI;
        C.e1 = t1 * std::cos(rot) + t2 * std::sin(rot); C.e2 = cross(C.c, C.e1);
        C.n = 2.5 + 1.5 * hu(10 * t + 5); C.sp = 5.0 + 5.0 * hu(10 * t + 6); C.amp = 300.0 + 300.0 * hu(10 * t + 7); C.band = hu(10 * t + 8) * TAU;
        out[n++] = C;
    }
    return n;
}
// a point's place in a corona: its superellipse radius (1 on the edge), the angle round it, the km inside its edge (negative outside)
static bool coronaAt(const Corona& C, double R, const Vec3& unit, double& rho, double& th, double& inKm) {
    const double dc = dot(unit, C.c);
    if (dc < 0.3) return false;
    const double x = R * dot(unit, C.e1) / dc, y = R * dot(unit, C.e2) / dc, xa = x / C.a, yb = y / C.b;
    rho = std::pow(std::pow(std::fabs(xa), C.n) + std::pow(std::fabs(yb), C.n), 1.0 / C.n);
    th = std::atan2(yb, xa);
    inKm = (1 - rho) * std::sqrt(C.a * C.b);
    return true;
}
// the great cliff along part of the first corona's edge: its middle's angle, its half-span (radians of the angle round the corona), its height (m)
static void cliffOf(const BodyGen& g, double& th0, double& span, double& H) {
    th0 = unitFromHash(mix64(g.seed ^ 0x9E40A1ULL)) * TAU - PI;
    span = 0.6 + 0.3 * unitFromHash(mix64(g.seed ^ 0x9E40A2ULL));
    H = 15000.0 + 5000.0 * unitFromHash(mix64(g.seed ^ 0x9E40A3ULL));
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

namespace {
// crack lines on the boundaries of worley cells: `h` is the faded value for the relief, `m` the unfaded one for the
// material wherever a line (`width` cells wide) spans half a sample (B-404: the icy worlds' rock cracks and the
// molten worlds' lava lines used to follow the fade and end at a ring's edge)
struct Crack { double h = 0, m = 0; };

// P-00: what a ground function works with at a point: the place and the scale, the sample it fills, the crater marks, the plates,
// the zones of the relief, the traits' landforms, the cliffs and the drainage. Each solid ground is its base, then the skeleton
// (`relief`: the relief spectrum, the cliffs and plains, the landforms), then its own features and materials
struct GroundCtx {
    const BodyGen& g;
    const Vec3& unit;
    const double detailM;
    SurfaceSample& s;
    const KindDef& K;
    const double R;
    const Vec3 p;                 // km
    const double lat, absLatDeg;
    CraterMarks cm;
    Tectonics tec;
    // O6-02: where the relief spectrum is strong: massifs (a ridged field of about 110 km) inside a kind's mountain regions, and rolling
    // hill country (a broad fbm); the amplitude follows `reliefAmp` (B-320: the felisian layer computes its own in `felisianGround`)
    double massif = 0, hillsF = 0;
    LandOut lo;                   // R-305: the traits' landforms of this point (the felisian layer keeps its own)
    double cliffV = 0, talusV = 0;   // O6-04: the cliff bands' riser and talus here
    DrainApply DA;                // O6-03: the drainage's channels of the desert, thin-air, hydrocarbon and acidic layers
    double hSmooth = 0, slopeM = 0;   // P-05: the ground of the last `relief` with the coarse part of its spectrum alone (its fine hollows filled: the wanderer's snow), its material chain's slope
    bool reliefDone = false;          // D-04: `relief` ran (hSmooth and slopeM are the ground's; the felisian layer and the seas have none)
    GroundCtx(const BodyGen& g_, const Vec3& unit_, double detailM_, SurfaceSample& s_)
        : g(g_), unit(unit_), detailM(detailM_), s(s_), K(KINDS[g_.type]), R(g_.R), p(unit_ * g_.R), lat(std::asin(clampd(unit_.z, -1, 1))), absLatDeg(std::fabs(lat) / DEG) {}
    // Layer helpers: each term fades out when its wavelength is too small for the sampling scale.
    double fb(double wlKm, uint64_t seed, int maxOct) const {
        double fade = lodFade(wlKm, detailM);
        if (fade <= 0) return 0.0;
        return fbm3(p / wlKm, seed, std::min(maxOct, octs(wlKm, detailM))) * fade;
    }
    double rg(double wlKm, uint64_t seed, int maxOct) const {
        double fade = lodFade(wlKm, detailM);
        if (fade <= 0) return 0.0;
        return ridged3(p / wlKm, seed, std::min(maxOct, octs(wlKm, detailM))) * fade;
    }
    double crater(double cellKm, double dens, double depth) {
        if (!scaleVisible(cellKm, detailM)) return 0.0;
        return craterField(p, R, cellKm, dens, depth, g.sD + (uint64_t)(cellKm * 1000), cm, detailM);
    }
    Crack crackAt(double cellKm, uint64_t seed, double width, double strength) const {
        Crack c;
        bool hv = scaleVisible(cellKm, detailM), mv = matVisible(width * cellKm, detailM);
        if (!hv && !mv) return c;
        Worley3 w = worley3(p / cellKm, seed);
        double v = strength * (1 - smoothstep(width * 0.5, width * 1.5, w.f2 - w.f1));
        if (hv) c.h = v * lodFade(cellKm, detailM);
        if (mv) c.m = v;
        return c;
    }
    // O6-04: the cliff bands and the plains' small relief of every solid kind (the felisian layer has its own); a shell of ice and a
    // soft cold world take less, worlds where it rains get gullies (the row's `plainK` and `gullyK`)
    void shape(double& h, const ReliefSum& rf, double rel, double hf) {
        double relief0 = clampd(rel + 0.25 * hf, 0, 1);
        // the cliffs' country: the massifs, and steep ground anywhere at seven tenths (a thin-atmosphere world's
        // mountains are hill-zone spectrum on a big base term, `rel` near 0: they had 0.1% of cliffs against 3-8)
        double cliffZone = std::max(smoothstep(0.12, 0.4, relief0), 0.7 * smoothstep(0.3, 0.5, rf.slopeM));   // B-404: on the material chain's slope
        CliffOut co = cliffBands(p, g, h, rf.slopeM, cliffZone, detailM);
        h += co.dh; cliffV = std::max(cliffV, co.cliff); talusV = std::max(talusV, co.talus);
        h += plainRelief(p, g, K.plainK * smoothstep(0.3, 0.1, relief0), K.gullyK, detailM);
    }
    // P-00: the skeleton every solid ground shares after its base `h`: the relief spectrum of amplitude a0 over the zones `rel` and
    // `hf` (`hRel`: the ground before it), the cliff bands and the plains (O6-04; `doShape` false for a ground that terraces its own),
    // and the traits' landforms (R-305; `hS0`: the smooth ground they read)
    ReliefSum relief(double& h, double rel, double hf, double a0, double* hRel = nullptr, double* hS0 = nullptr, bool doShape = true) {
        ReliefSum rf = reliefSum(p, unit, g, a0, g.reliefRidge, g.sB + 7, detailM);
        const double h0 = h; h += rf.h;
        if (hRel) *hRel = h0;
        if (doShape) shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = h0 + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            // Q-01: the world's own moisture and warmth. A world it rains on reads its moisture as the living ground's field round its kind's mean
            // (`landMoist`: a dry world's stays 0.15), and every world its mean temperature as the explorer's thermometer reads it (the surface's
            // model without the day: its air's greenhouse, its latitude, its height), where they read the equilibrium alone: an acidic world's
            // karst towers stood on 0.15 under their 0.45 and on ground at 1 C under its sky of 56 (the name and the sentence and no towers)
            if (K.landMoist >= 0) lc.moist = K.landMoist + 0.5 * fbm3(p / (R * 0.22), g.sF, 3) + g.moistureBias;
            {
                const double tK = g.tempBias / 0.3 + 288.0 - K.greenhouseK, latDeg = std::fabs(std::asin(clampd(unit.z, -1, 1))) / DEG;   // the equilibrium (`make`'s bias undone)
                lc.T = (K.atmosphere ? (tK - 273.15) * 0.75 + K.tempOffset - 0.4 * latDeg : (tK - 273.15) * 1.3 - 60.0 - 0.5 * latDeg) - 6.5 * std::max(0.0, lc.hS) / 1000.0 + K.tempAdd;
            }
            if (hS0) *hS0 = lc.hS;
            hSmooth = lc.hS; slopeM = rf.slopeM; reliefDone = true;
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        return rf;
    }
    // P-00: the kind's crater stack summed into `acc` in order (the first grid assigned when `assignFirst`)
    void craterStack(double& acc, bool assignFirst = false) {
        for (int i = 0; i < K.nCraters; i++) {
            const CraterGrid& e = K.craters[i];
            const double cell = e.cellRel ? R * e.cell : e.cell;
            const double dens = e.densMode == CD_REL ? g.craterDensity * e.dens : (e.densMode == CD_SECONDARY ? std::min(1.0, 0.7 + 0.3 * clampd(cm.secondary, 0, 1)) : e.dens);
            const double depth = e.depthRel ? g.craterDepth * e.depth : e.depth;
            const double v = crater(cell, dens, depth);
            if (i == 0 && assignFirst) acc = v; else acc += v;
        }
    }
};

// P-00: the cratered airless ground: maria between rolling highlands, the crater stack (cratered worlds)
static void groundCratered(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double mare = smoothstep(0.15, 0.4, base);
    double h = base * g.mountainAmp * (1 - 0.8 * mare);
    // O6-02: rolling highlands between the maria (the 20 km hills and the 400 m detail of generation 5 are gone)
    double rel = (1 - mare) * massif, hf = std::max(hillsF, 0.6 * (1 - mare));
    c.relief(h, rel, hf, reliefAmp(g, rel, hf) * (1 - 0.7 * mare));   // P-00: the skeleton (the maria smooth the relief)
    double cr = 0;
    c.craterStack(cr);   // P-00: the kind's eight grids (M9-06: the 0.5 km pass denser in the secondary fields near large craters)
    h += cr * (1 - 0.5 * mare);
    s.height = h;
    double flooded = clampd(cm.flooded, 0, 1);
    s.material = (mare > 0.5 || flooded > 0.5) ? MAT_BASALT : MAT_DUST;
    s.albedo = (0.52 + 0.12 * base) * (1 - 0.45 * std::max(mare, flooded)) + 0.12 * clampd(rimBright, 0, 1) - 0.08 * clampd(cm.floorDark, 0, 1) + 0.3 * clampd(cm.rays, 0, 1);
    s.relief = clampd(std::fabs(cr) / 400.0 + rel, 0, 1);
}

// P-00: the molten ground: lava lakes and glowing cracks, cones and their flows (molten worlds)
static void groundMolten(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 5);
    double roughL = c.rg(R * 0.15, g.sC, 2);
    bool caldera = false;
    double h = base * 1600.0 + roughL * g.mountainAmp * 0.5;
    double zone = smoothstep(0.45, 0.85, roughL), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: the rough belts carry the relief spectrum
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    if (scaleVisible(R * 0.25, detailM)) h += volcanoField(p, R, R * 0.25, g.volcanoes * 0.6, g.sD, 2800.0, caldera);
    if (scaleVisible(30.0, detailM)) h += volcanoField(p, R, 30.0, g.volcanoes * 0.8, g.sD + 3, 700.0, caldera);
    LavaOut lf = lavaFlows(p, R, 30.0, g.volcanoes * 0.8, g.sD + 3, detailM);   // O6-05: tongues and pit chains from the cones
    h += lf.dh;
    s.height = h;
    s.relief = clampd(roughL + 0.5 * rel + 0.5 * rf.slope, 0, 1);
    Crack crack = c.crackAt(12.0, g.sF, 0.05, 1.0);
    if (h < g.lavaLevel || caldera) {
        s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0;
        if (h < g.lavaLevel) s.height = g.lavaLevel;
    } else if (crack.m > 0.5 && h < g.lavaLevel + 900) {
        s.material = MAT_LAVA; s.glow = 0.75; s.albedo = 0.9; s.height = h - 6.0 * crack.h;
    } else if (lf.flow > 0.4 || lf.pit > 0.3) {
        s.material = MAT_BASALT; s.albedo = 0.09 + 0.03 * (1 - lf.flow); s.glow = lf.pit > 0.6 ? 0.25 : 0.0;   // fresh flows, the tubes' skylights glowing
    } else {
        s.material = MAT_BASALT; s.albedo = 0.13 + 0.10 * (0.5 + 0.5 * base) + 0.05 * roughL;
    }
}

// P-00: under the deck: the plates, tesserae on the highlands, pancake domes (venusian worlds)
static void groundVenusian(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp + tec.uplift * 800.0;
    { double chainR = c.rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.7) - tec.rift * 800.0 + tec.ridge * 400.0; }
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: tesserae on the highlands and along the chains
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    if (scaleVisible(R * 0.08, detailM)) h += domeField(p, R, R * 0.08, 0.3, g.sD, 0.35, 700.0, 0.85) * lodFade(R * 0.08, detailM);
    if (scaleVisible(3.0, detailM)) h += domeField(p, R, 3.0, 0.3, g.sD + 1, 0.4, 40.0, 0.9);
    s.height = h;
    s.material = MAT_BASALT;
    s.albedo = 0.30 + 0.12 * base;
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: creased rock: the plates' chains and rifts, old shields, boulder fields (rocky worlds)
static void groundRocky(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double creaseL = c.rg(R * 0.15, g.sA, 2);
    double base = c.fb(R * 0.45, g.sB, 4);
    double h = creaseL * g.mountainAmp + base * 600.0;
    {   // M9-02: chains and rifts across the crust
        double chainR = c.rg(40.0, g.sA + 2, 7);
        h += tec.chain * (chainR * chainR * g.chainAmp * 0.8 + std::fabs(chainR) * 500.0) + tec.uplift * 600.0;
        h -= tec.rift * 900.0 * (0.5 + 0.5 * std::fabs(c.fb(50.0, g.sA + 3, 3)));
        h += tec.ridge * 500.0 * std::fabs(c.rg(35.0, g.sA + 4, 5));
    }
    double zone = clampd(smoothstep(0.3, 0.7, creaseL) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: creased ridges (the 20 km crease, the 4 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    if (scaleVisible(R * 0.3, detailM)) h += domeField(p, R, R * 0.3, 0.2, g.sD + 7, 0.4, 900.0, 0.6) * lodFade(R * 0.3, detailM);   // old shield volcanoes
    if (scaleVisible(0.25, detailM)) h += domeField(p, R, 0.25, g.boulderDensity, g.sD, 0.16, 14.0, 0.0);
    if (scaleVisible(0.06, detailM)) h += domeField(p, R, 0.06, g.boulderDensity * 0.7, g.sD + 2, 0.2, 3.0, 0.0);
    c.craterStack(h);   // P-00: the kind's crater stack
    s.height = h;
    s.material = MAT_ROCK;
    s.albedo = 0.28 + 0.18 * creaseL + 0.05 * base + 0.06 * clampd(rf.slope * 2.0, 0, 1);
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: thin air: canyons, shield volcanoes and their dark plains, dune seas, dry valleys, ice caps (thin-atmosphere worlds)
static void groundThinAir(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double mtnL = c.rg(R * 0.14, g.sC, 2);
    double canyonN = c.rg(R * 0.3, g.sF, 2);
    double canyon = std::pow(clampd(canyonN, 0, 1), 9.0);
    double mtnMask = smoothstep(0.3, 0.7, mtnL) * smoothstep(-0.2, 0.4, base);
    double h = base * 1800.0 + mtnL * mtnL * g.mountainAmp * mtnMask - canyon * 2200.0;
    {   // M9-02 chains and rifts, M9-07 shield volcanoes with dark lava plains around them
        double chainR = c.rg(40.0, g.sC + 2, 7);
        h += tec.chain * (chainR * chainR * g.chainAmp * 0.9 + std::fabs(chainR) * 500.0) + tec.uplift * 900.0;
        h -= tec.rift * 1500.0 * (0.5 + 0.5 * std::fabs(c.fb(50.0, g.sC + 3, 3)));
        h += tec.ridge * 600.0 * std::fabs(c.rg(35.0, g.sC + 4, 5));
    }
    // O6-02: the relief spectrum (the 30 km ridges, the 8 km hills and the 400 m detail of generation 5 are gone)
    double zone = clampd(mtnMask + 0.8 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    bool shieldCal = false;
    double shield = 0;
    if (scaleVisible(R * 0.35, detailM)) { shield = volcanoField(p, R, R * 0.35, 0.28, g.sD + 11, 1400.0, shieldCal) * lodFade(R * 0.35, detailM); h += shield; }
    c.craterStack(h);   // P-00: the kind's crater stack
    if (h < 200) { const double share = smoothstep(200, -300, h); double lee = 0; h += duneField(p, g, lat, g.duneAmp, detailM, &lee) * share; s.slipFace = share * lee; }   // dune seas in the lowlands (M9-08)
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, false, false, 0.5, DA);   // O6-03: the old valley networks, dry, with dust flats in the basins
    s.height = h;
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
    double capEdge = g.iceCapLat + 5.0 * gnoise3(p / (R * 0.15), g.sF + 7);
    if (absLatDeg > capEdge) { s.material = MAT_ICE; s.albedo = 0.85; }
    else if (DA.playa > 0.3) { s.material = MAT_DUST; s.albedo = 0.56 + 0.06 * base; }   // O6-03: a dust flat in a closed basin
    else if (relief0 + 0.5 * rf.slopeM > 0.6 && h > 1500) { s.material = MAT_ROCK; s.albedo = 0.35; }   // B-404: the material chain's slope, not `s.relief` (whose slope is the ring's own)
    else if (shield > 400 || shieldCal) { s.material = MAT_BASALT; s.albedo = 0.22 + 0.06 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.42 + 0.12 * base + 0.10 * clampd(rimBright, 0, 1) - 0.12 * canyon + 0.25 * clampd(cm.rays, 0, 1) - 0.1 * clampd(cm.flooded, 0, 1); }
}

// P-00: cracked ice: fracture networks, craters, cryovolcanic domes and geyser spots (icy worlds, comets)
static void groundIce(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: ridged uplands on the high ground (the 6 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    Crack c1 = c.crackAt(R * 0.15, g.sF, g.fractureWidth, 1.0), c2 = c.crackAt(12.0, g.sF + 3, g.fractureWidth, 0.8), c3 = c.crackAt(1.2, g.sF + 5, g.fractureWidth * 1.3, 0.6);
    double crack = std::max(c1.h, std::max(c2.h, c3.h)), crackM = std::max(c1.m, std::max(c2.m, c3.m));   // B-404: the relief by the faded lines, the rock by the unfaded ones
    h -= crack * 60.0;
    c.craterStack(h);   // P-00: the kind's crater stack
    // M9-07 cryovolcanoes (bright fresh domes) and geyser fields (bright spots on the ground)
    double fresh = 0;
    if (scaleVisible(25.0, detailM)) { double cv = domeField(p, R, 25.0, 0.12, g.sD + 21, 0.25, 140.0, 0.0); h += cv; fresh += cv / 140.0; }
    if (scaleVisible(2.0, detailM)) fresh += domeField(p, R, 2.0, 0.08, g.sD + 22, 0.15, 1.0, 0.0);
    s.height = h;
    s.material = crackM > 0.55 ? MAT_ROCK : MAT_ICE;
    s.albedo = (0.72 + 0.12 * base) * (1 - 0.5 * crackM) + 0.1 * clampd(rimBright, 0, 1) + 0.15 * clampd(fresh, 0, 1) + 0.2 * clampd(cm.rays, 0, 1);
    s.relief = clampd(crack + 0.5 * rel + 0.5 * rf.slope, 0, 1);
}

// P-00: milky quartz mounds (quartz worlds)
static void groundQuartz(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.35, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: rounded mounds (ridge near 0, low ground smoothed; the 3.5 km rounded hills are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    if (scaleVisible(R * 0.06, detailM)) h += domeField(p, R, R * 0.06, 0.4, g.sD, 0.3, 500.0, 0.0) * lodFade(R * 0.06, detailM);
    c.craterStack(h);   // P-00: the kind's crater stack
    s.height = h;
    s.material = MAT_QUARTZ;
    s.albedo = 0.78 + 0.12 * base;
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1) * 0.6;
}

// P-00: one global sea and its floes (ocean worlds)
static void groundOcean(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, lat = c.lat, absLatDeg = c.absLatDeg;
    // one global sea; ice floes drift toward the poles (plates with water gaps). B-322: the floe zone lies over a shallow
    // shelf sea (the floor at 5-8 m; the open ocean stays at 150 m) and every plate rises out of it through a submerged
    // ice foot, so the shore is the water plane's contour on a continuous surface at every ring: it used to be a jump
    // from +1 m to -150 m between two vertices, a stair of cells from the air and a cliff face from the water
    double swell = c.fb(2.0, g.sB, 4) * g.hillAmp;
    double capEdge = g.iceCapLat + 8.0 * gnoise3(p / (R * 0.2), g.sF + 7) - 8.0 * clampd(lat >= 0 ? -g.season : g.season, 0, 1);
    double nightIce = g.locked && !c.K.evenClimate ? clampd((-dot(unit, g.lockedDir) - 0.05) / 0.3, 0, 1) : 0.0;   // M5-08: the night side freezes over (P-03: not under a hycean's air)
    double cover = std::max(smoothstep(capEdge - 12, capEdge + 6, absLatDeg), nightIce);       // the share of the plates that are ice
    double shelf = smoothstep(0.0, 0.04, cover);   // the whole floe zone (the plates appear at any cover) lies on the shelf
    double floor = (-150.0 + swell) + ((-6.5 + 0.3 * swell) - (-150.0 + swell)) * shelf;
    s.height = floor;
    s.material = MAT_WATER; s.water = 0; s.albedo = 0.26; s.biome = BIO_OCEAN;
    if (cover > 0) {
        Worley3 fl = worley3(p / 1.5, g.sF + 3);
        double d = fl.f2 - fl.f1;   // 0 on the plate boundary, growing inward (units of 1.5 km)
        if (unitFromHash(fl.id1) < cover) {
            // an ice plate: the foot climbs from the floor at the boundary to the water line 150 m in, the freeboard
            // 0.9-1.25 m rises over the next 45 m and stays flat; the gap between two ice plates is 300 m of shallow water
            double rim = smoothstep(0.0, 0.13, d);
            double top = 0.9 + 0.35 * smoothstep(0.13, 0.3, d);
            double h = floor + (top - floor) * rim;
            s.height = h;
            if (h > -0.3) { s.material = MAT_ICE; s.albedo = 0.86; s.biome = BIO_ICE; }
            else s.albedo = 0.26 + 0.2 * clampd((h + 6.5) / 6.5, 0, 1);   // the foot shows pale through the shallow water on the map
        }
    }
}

// P-00: an ice shell over a hidden ocean: lineae, chaos rafts (europan worlds)
static void groundEuropan(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    // R-307: an ice shell over a hidden ocean: a plain a few tens of metres in relief, crossed by lineae (double ridges
    // with a trough between, stained red-brown by what wells up through them), chaos terrain where the shell broke
    // into rafts, a few craters. The water geysers stand on the lineae (`drawGeysers` finds the stained cells)
    double base = c.fb(R * 0.5, g.sA, 4);
    double h = base * 60.0;
    double zone = smoothstep(0.3, 0.7, base), rel = zone * massif * 0.3, hf = hillsF * 0.3;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    double stain = 0, band = 0;
    auto lineae = [&](double cellKm, double ridgeM, double troughM, double spanM, uint64_t seed, double fadeKm) {
        // B-404: the ridges fade with the sampling scale as before; the stain (three spans wide: the dust) is unfaded
        // wherever it spans half a cell, so the three sets' brown bands lie on the same ground in every ring (they
        // used to follow the fade: the 6 km set's bands ended at the near ring's edge, the 40 km set's at the 16 m ring's)
        double f = featFade(spanM * 4.0 / 1000.0, detailM);   // the double ridge and its trough, four spans wide (it was `lodFade(fadeKm)`: the 12 m ridges stood at 41% in the 16 m ring)
        bool mv = matVisible(spanM * 3.0 / 1000.0, detailM);
        (void)fadeKm;
        if (f <= 0 && !mv) return;
        Worley3 w = worley3(p / cellKm, seed);
        double dm = (w.f2 - w.f1) * cellKm * 500.0;   // metres from the line (half the difference of the two distances)
        if (f > 0) {
            double rim = std::exp(-std::pow((dm - spanM) / (spanM * 0.45), 2.0));
            double trough = std::exp(-std::pow(dm / (spanM * 0.6), 2.0));
            h += (ridgeM * rim - troughM * trough) * f;
        }
        if (mv) stain = std::max(stain, smoothstep(spanM * 2.2, spanM * 0.8, dm));
    };
    {   // the great lineae stain a band 6-16 km wide (what a globe shows of them), the ridges themselves are the first set below
        Worley3 w = worley3(p / (R * 0.25), g.sF + 1);
        double dm = (w.f2 - w.f1) * R * 0.25 * 500.0;
        band = smoothstep(8000.0, 2500.0, dm) * (0.6 + 0.4 * gnoise3(p / 60.0, g.sF + 17));
        stain = std::max(stain, band);
    }
    lineae(R * 0.25, 120.0, 40.0, 300.0, g.sF + 1, 1.5);
    lineae(40.0, 45.0, 15.0, 110.0, g.sF + 2, 0.5);
    lineae(6.0, 12.0, 4.0, 30.0, g.sF + 3, 0.12);
    double chaosMask = smoothstep(0.35, 0.6, c.fb(R * 0.15, g.sC + 3, 3));   // rafts of the shell, jumbled where the ocean broke through
    if (chaosMask > 0 && scaleVisible(3.0, detailM)) {
        Worley3 cw = worley3(p / 3.0, g.sF + 9);
        double edge = smoothstep(0.02, 0.07, cw.f2 - cw.f1);
        h += chaosMask * edge * (60.0 * unitFromHash(cw.id1) - 30.0) * featFade(3.0, detailM);   // B-404: the rafts by their own size (the 512 m ring draws them)
    }
    c.craterStack(h);   // P-00: the kind's crater stack
    s.height = h;
    s.material = stain > 0.5 ? MAT_DUST : MAT_ICE;
    s.albedo = stain > 0.5 ? 0.40 - 0.1 * stain : (0.72 + 0.08 * base + 0.1 * clampd(rimBright, 0, 1) - 0.15 * chaosMask) * (1 - 0.35 * stain);
    s.relief = clampd(chaosMask * 0.5 + 0.5 * rf.slope, 0, 1);
}

// P-00: a seismic crust: rifts and their fissures, cones, ash plains (tectonic worlds)
static void groundTectonic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    // R-307: a seismic world: rift valleys where the crust pulls apart (their floors flat, 800 m down), fissures on the
    // floors running with lava (`drawFountains` stands its fountains on them), fault-block ranges, ash plains, sulphur
    // crusts round the vents, a few volcanoes
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp * 0.6 + tec.uplift * 700.0;
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    double hS = h - (rf.h - rf.hCoarse);
    Worley3 rw = worley3(p / (R * 0.3), g.sF + 5);
    double dRift = (rw.f2 - rw.f1) * R * 0.3 * 500.0;   // metres from the rift axis
    double rift = smoothstep(11000.0, 7000.0, dRift);
    h += (hS - 800.0 - h) * rift;
    double lava = 0, fresh = 0;
    if (rift > 0.5 && (scaleVisible(5.0, detailM) || matVisible(1.0, detailM))) {   // the fissures: trenches 30 m deep and 400 m wide on 5 km cells, lava along their axes (100 m), fresh basalt and sulphur crusts round them (1-1.3 km); B-404: each by its own width
        Worley3 fw = worley3(p / 5.0, g.sF + 6);
        double dm = (fw.f2 - fw.f1) * 2500.0;
        h -= 30.0 * smoothstep(220.0, 40.0, dm) * featFade(0.44, detailM);
        if (matVisible(0.14, detailM)) lava = smoothstep(90.0, 50.0, dm) * rift;
        if (matVisible(1.0, detailM)) fresh = smoothstep(700.0, 150.0, dm) * rift;
    }
    bool caldera = false;
    if (scaleVisible(R * 0.2, detailM)) h += volcanoField(p, R, R * 0.2, g.volcanoes * 0.5, g.sD, 2200.0, caldera) * lodFade(R * 0.2, detailM);
    if (scaleVisible(20.0, detailM)) h += volcanoField(p, R, 20.0, g.volcanoes * 0.7, g.sD + 3, 500.0, caldera);
    LavaOut lf = lavaFlows(p, R, 20.0, g.volcanoes * 0.7, g.sD + 3, detailM);   // O6-05
    h += lf.dh;
    c.craterStack(h);   // P-00: the kind's crater stack
    s.height = h;
    double ash = c.fb(R * 0.1, g.sC + 8, 3);
    if (lava > 0.5 || caldera) { s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0; }
    else if (fresh > 0.4 || lf.flow > 0.4 || lf.pit > 0.3) { s.material = MAT_BASALT; s.albedo = 0.12 + 0.06 * (1 - std::max(fresh, lf.flow)); if (lf.pit > 0.6) s.glow = 0.25; }
    else if (fresh > 0.12) { s.material = MAT_SULPHUR; s.albedo = 0.58 + 0.2 * fresh; }
    else if (rel < 0.45 && ash > 0.05 && rift < 0.5) { s.material = MAT_DUST; s.albedo = 0.34 + 0.1 * ash; }
    else { s.material = MAT_BASALT; s.albedo = 0.30 + 0.08 * (0.5 + 0.5 * base); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + rift * 0.3, 0, 1);
}

// P-00: a dry world: hamada, canyons, mesas, ergs, salt pans and wadis; a dead world's dry seas (desert worlds)
static void groundDesert(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, massif = c.massif, hillsF = c.hillsF;
    // R-307: a dry world under a dusty sky: hamada plains, mesas where the flats terrace, canyons, ergs in the basins
    // with salt pans at their floors (the dust devils and storms are the surface's, `computeEnvironment`)
    double base = c.fb(R * 0.5, g.sA, 5);
    double h = base * 1400.0 + tec.uplift * 600.0;
    { double chainR = c.rg(40.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 700.0 + tec.ridge * 300.0; }
    double zone = clampd(smoothstep(0.2, 0.7, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS, false);   // P-00: the skeleton without the cliff bands (the mesas terrace below); hS the smooth ground before the landforms
    double canyonN = c.rg(R * 0.3, g.sF, 2);
    double canyon = std::pow(clampd(canyonN, 0, 1), 8.0);
    h -= canyon * 900.0;
    double oldSea = 0, seaDepth = 0, bedH = 0;   // C-01: a dead world's seas, dry: the bed a sediment floor keeping a tenth of the relief, deeper offshore
    if (g.oldSeaM > -1e8) {            // (a canyon under the level was a sound of the sea), the old shore a bench of beach ridges a few metres
        double hC = hRel - canyon * 900.0;   // over the level (the coastal flats the people lived on). The level cuts the coarse ground with its
        oldSea = smoothstep(g.oldSeaM + 1.5, g.oldSeaM - 1.5, hC);   // canyons in a band of 3 m: the shore is a low scarp, not a ramp
        seaDepth = std::max(0.0, g.oldSeaM - hC);
        if (oldSea > 0) { bedH = g.oldSeaM - 7.0 - seaDepth * 0.35 + rf.h * 0.1; h += (bedH - h) * oldSea; hS += (bedH - hS) * oldSea; }
        double over = hC - g.oldSeaM;
        if (over > 0.0 && over < 40.0) { double bench = g.oldSeaM + 3.0 + 0.15 * rf.h; h += (bench - h) * 0.7 * smoothstep(40.0, 15.0, over); }
        canyon *= 1 - oldSea;   // the material's reading: no canyon walls on the bed
    }
    if (relief0 < 0.4 && h > 100 && oldSea < 0.5) {   // mesas: the dry flats terrace (M9-03's rule)
        double step = 45.0, q = std::floor(h / step) * step, f = (h - q) / step;
        double terr = q + step * smoothstep(0.35, 0.65, f);
        h += (terr - h) * (1 - smoothstep(0.25, 0.4, relief0)) * smoothstep(-0.1, 0.3, c.fb(R * 0.12, g.sC + 4, 3));
    }
    double basin = smoothstep(0.1, 0.5, -c.fb(R * 0.1, g.sG + 2, 3)) * (1 - smoothstep(300.0, 700.0, h)) * (1 - relief0) * (1 - 0.7 * oldSea);
    double salt = 0;
    if (basin > 0.7 && h < 80) { double pan = smoothstep(0.7, 0.9, basin); h += (hS - 4.0 - h) * pan; salt = pan; }
    if (basin > 0.2 && salt < 0.5) { const double share = smoothstep(0.2, 0.5, basin); double lee = 0; h += duneField(p, g, lat, g.duneAmp * (0.6 + 0.4 * basin), detailM, &lee) * share; s.slipFace = share * lee; }
    c.craterStack(h);   // P-00: the kind's crater stack
    applyDrainage(g, unit, detailM, h, &hS, rf.slope, false, false, g.oldSeaM > -1e8 ? 0.75 : 0.5, DA);   // O6-03: wadis, and playas in the closed basins (C-01: a dead world's rivers ran wetter: a denser net)
    if (oldSea > 0) h += (bedH - h) * oldSea;   // C-01: the bed again: a wadi's valley reaching down over the shore lifted the bed toward its banks, and the craters and dunes are the sea's silt now
    s.height = h;
    if (salt > 0.5 || DA.playa > 0.3) { s.material = MAT_SALT; s.albedo = 0.85; }
    else if (DA.bed > 0.4) { s.material = MAT_SAND; s.albedo = 0.6 + 0.1 * base; }   // the wadi's bed
    else if (basin > 0.5) { s.material = MAT_SAND; s.albedo = 0.6 + 0.1 * base; }
    else if (relief0 > 0.5 || canyon > 0.3) { s.material = MAT_ROCK; s.albedo = 0.4 + 0.1 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.46 + 0.12 * base + 0.1 * clampd(rimBright, 0, 1); }
    if (oldSea > 0.5 && DA.bed < 0.4) {   // C-01: the seabed's silt, the last brine's salt in the deeps; the old beaches along the shore
        if (seaDepth > 420.0 + 150.0 * c.fb(R * 0.05, g.sG + 9, 2)) { s.material = MAT_SALT; s.albedo = 0.82; }
        else { s.material = MAT_ROCK; s.albedo = 0.36 + 0.05 * base; }   // the silt crust of the floor: the rock's brown, darker than the hamada
    } else if (g.oldSeaM > -1e8 && oldSea < 0.5 && hRel - canyon * 900.0 - g.oldSeaM < 6.0 && DA.bed < 0.4 && salt < 0.5) { s.material = MAT_SAND; s.albedo = 0.62; }
    s.oldSea = oldSea;
    s.relief = clampd(relief0 + 0.5 * rf.slope + canyon, 0, 1);
}

// P-00: under the haze: ice-rock hills, tholin dunes, methane seas and channels (hydrocarbon worlds)
static void groundHydrocarbon(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    // R-307: under the orange haze: rounded hills of ice-rock, belts of dark tholin dunes in the low latitudes, seas
    // and lakes of methane in the lowlands and toward the poles (the liquid at the reference level, the post-step
    // below), dry channels cut down to them, cryovolcanic domes
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * 700.0 + 130.0 - 170.0 * smoothstep(50.0, 75.0, absLatDeg);
    double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    double chan = c.crackAt(30.0, g.sF + 4, 0.06, 1.0).h;   // channels: shallow valleys on 30 km cells, deeper toward the sea
    h -= chan * 25.0 * smoothstep(400.0, 50.0, h);
    double dunes = smoothstep(35.0, 20.0, absLatDeg) * smoothstep(0.0, 0.3, c.fb(R * 0.12, g.sG + 5, 3)) * smoothstep(300.0, 60.0, h) * smoothstep(0.0, 20.0, h);
    if (dunes > 0.05) { double lee = 0; h += duneField(p, g, lat, g.duneAmp, detailM, &lee) * dunes; s.slipFace = dunes * lee; }
    if (scaleVisible(25.0, detailM)) h += domeField(p, R, 25.0, 0.08, g.sD + 21, 0.25, 180.0, 0.0);
    c.craterStack(h);   // P-00: the kind's crater stack
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // O6-03: rivers of methane to the seas, lakes in the depressions
    s.height = h;
    s.material = dunes > 0.4 ? MAT_SAND : MAT_ICE;
    if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.2; }
    s.albedo = dunes > 0.4 ? 0.18 : 0.32 + 0.08 * base + 0.06 * clampd(rimBright, 0, 1);
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: a young airless surface: craters at every scale, rays (bombarded worlds)
static void groundBombarded(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    // R-307: a young surface under a rain of rock: craters at every scale with sharp rims and bright rays over a dark
    // glassy regolith; the strikes themselves are the surface's (`drawImpacts`)
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp * 0.6;
    double zone = smoothstep(0.2, 0.6, base), rel = zone * massif * 0.7, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    double cr = 0;
    c.craterStack(cr, true);   // P-00: the kind's crater stack
    h += cr;
    s.height = h;
    s.material = MAT_DUST;
    s.albedo = 0.28 + 0.06 * base + 0.14 * clampd(rimBright, 0, 1) + 0.5 * clampd(cm.rays, 0, 1);
    s.relief = clampd(std::fabs(cr) / 200.0 + rel + 0.5 * rf.slope, 0, 1);
}

// P-00: a corrosive world: karst, sulphur crusts, acid seas and rivers (acidic worlds)
static void groundAcidic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    // R-307: a corrosive world: the rock bleached and eaten into karst (sinkholes at 300 m, towers in the wettest
    // basins), sulphur crusts by the shores, seas of dilute acid at the reference level (the post-step below)
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp * 0.8 + 150.0 + tec.uplift * 600.0;
    { double chainR = c.rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 600.0; }
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    double low = smoothstep(600.0, 150.0, h) * (1 - smoothstep(0.3, 0.5, relief0));
    if (low > 0.1 && scaleVisible(0.3, detailM)) {
        Worley3 kw = worley3(p / 0.3, g.sF + 12);
        h -= 9.0 * low * smoothstep(0.55, 0.15, kw.f1) * lodFade(0.3, detailM);
    }
    if (low > 0.5 && scaleVisible(1.5, detailM)) h += domeField(p, R, 1.5, 0.25, g.sD + 13, 0.12, 70.0, 0.0) * low;
    c.craterStack(h);   // P-00: the kind's crater stack
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // O6-03: rivers of dilute acid, lakes in the karst basins
    s.height = h;
    double crust = smoothstep(40.0, 5.0, h) * smoothstep(0.35, 0.15, relief0);
    if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.22; }
    else if (crust > 0.5) { s.material = MAT_SULPHUR; s.albedo = 0.55 + 0.15 * crust; }
    else { s.material = MAT_ROCK; s.albedo = 0.55 + 0.1 * base - 0.1 * low; }
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
}

// P-00: dark iron: sharp ridges and craters (metal worlds)
static void groundMetal(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(-0.2, 0.5, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: sharp iron ridges (the 30 km crease and the 4 km hills are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    double cr = 0;
    c.craterStack(cr, true);   // P-00: the kind's crater stack
    h += cr;
    s.height = h;
    s.material = MAT_METAL;
    s.albedo = 0.12 + 0.08 * base + 0.1 * clampd(rimBright, 0, 1) + 0.25 * clampd(cm.rays, 0, 1);
    s.relief = clampd(std::fabs(cr) / 300.0 + rel + 0.5 * rf.slope, 0, 1);
}

// P-00: a tidally heated moon: cones, lava, sulphur plains (volcanic worlds)
static void groundVolcanic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 4);
    bool caldera = false;
    double h = base * g.mountainAmp * 0.5;
    double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02 (the 5 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    if (scaleVisible(R * 0.2, detailM)) h += volcanoField(p, R, R * 0.2, g.volcanoes, g.sD, 3200.0, caldera) * lodFade(R * 0.2, detailM);
    if (scaleVisible(25.0, detailM)) h += volcanoField(p, R, 25.0, g.volcanoes * 0.9, g.sD + 3, 900.0, caldera);
    LavaOut lf = lavaFlows(p, R, 25.0, g.volcanoes * 0.9, g.sD + 3, detailM);   // O6-05
    h += lf.dh;
    if (scaleVisible(4.0, detailM)) h += domeField(p, R, 4.0, 0.25, g.sD + 5, 0.3, 40.0, 0.0);
    s.height = h;
    double plains = fbm3(p / (R * 0.12), g.sF, 3);
    if (h < g.lavaLevel || caldera) {
        s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0;
        if (h < g.lavaLevel) s.height = g.lavaLevel;
    } else if (lf.flow > 0.4 || lf.pit > 0.3) {
        s.material = MAT_BASALT; s.albedo = 0.1 + 0.05 * (1 - lf.flow); if (lf.pit > 0.6) s.glow = 0.25;
    } else if (plains > 0.15) {
        s.material = MAT_SULPHUR; s.albedo = 0.55 + 0.25 * plains;
    } else {
        s.material = MAT_BASALT; s.albedo = 0.2 + 0.1 * (0.5 + 0.5 * base);
    }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: graphite plains with diamond crusts (carbon worlds)
static void groundCarbon(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02 (the 6 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // P-00: the relief spectrum, the cliffs and plains (O6-04), the traits' landforms (R-305)
    c.craterStack(h);   // P-00: the kind's crater stack
    s.height = h;
    Worley3 dw = worley3(p / (R * 0.1), g.sF + 2);
    bool diamond = unitFromHash(dw.id1) < 0.18 && dw.f1 < 0.45;
    s.material = diamond ? MAT_QUARTZ : MAT_GRAPHITE;
    s.albedo = diamond ? 0.85 + 0.1 * base : 0.05 + 0.04 * (0.5 + 0.5 * base) + 0.08 * clampd(rimBright, 0, 1);
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-01: a magma ocean: a molten world's crust, and round the substellar point a sea of magma at the reference level, its shore a belt of
// crust rafts breaking up in the lava and cracks running with it; past the terminator black glass on the flats and basalt on the relief,
// a frost of condensed rock over the far night (magma oceans)
static void groundMagma(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 5);
    double roughL = c.rg(R * 0.15, g.sC, 2);
    double h = base * 1100.0 + roughL * g.mountainAmp * 0.45;
    double zone = smoothstep(0.45, 0.85, roughL), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton: the relief spectrum, the cliffs and plains, the traits' landforms
    bool caldera = false;
    if (scaleVisible(30.0, detailM)) h += volcanoField(p, R, 30.0, g.volcanoes, g.sD + 3, 600.0, caldera);
    c.craterStack(h);
    // the sea: within its reach of the substellar point (the shore wandering by a few degrees) the crust melts down under the level
    const Vec3 sub = g.locked ? g.lockedDir : Vec3(1, 0, 0);
    const double ang = std::acos(clampd(dot(unit, sub), -1, 1)) / DEG;   // degrees from the substellar point
    const double reach = g.seaReachDeg + 5.0 * c.fb(R * 0.25, g.sF + 7, 3);
    const double melt = smoothstep(reach + 1.5, reach - 4.0, ang);
    if (melt > 0) h += (-80.0 - h) * melt;
    double raft = 0;   // the rafts of crust breaking up along the shore, fewer further out (the ocean world's floes, of rock in lava)
    if (melt > 0.2 && (scaleVisible(0.7, detailM) || matVisible(0.1, detailM))) {
        Worley3 fl = worley3(p / 0.7, g.sF + 11);
        if (unitFromHash(fl.id1) < 0.45 * smoothstep(reach - 10.0, reach - 1.0, ang)) {
            const double rim = smoothstep(0.0, 0.16, fl.f2 - fl.f1), top = 1.5 + 4.0 * unitFromHash(mix64(fl.id1 + 5));
            const double hr = -3.0 + (top + 3.0) * rim * featFade(0.7, detailM);
            if (hr > h) { h = hr; raft = rim; }
        }
    }
    s.height = h;
    s.relief = clampd(roughL + 0.5 * rel + 0.5 * rf.slope, 0, 1);
    const double night = smoothstep(86.0, 100.0, ang), far = smoothstep(105.0, 150.0, ang);
    Crack crack = c.crackAt(12.0, g.sF, 0.05, 1.0);
    if ((h < 0 && melt > 0) || (caldera && night < 0.5)) {
        s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0;
        if (h < 0) s.height = 0;
    } else if (raft > 0) {
        s.material = MAT_BASALT; s.albedo = 0.09 + 0.05 * raft; s.glow = 0.35 * (1 - raft);   // the raft's edge still glows
    } else if (crack.m > 0.5 && ang < reach + 14.0 && h < 900) {
        s.material = MAT_LAVA; s.glow = 0.75; s.albedo = 0.9; s.height = h - 6.0 * crack.h;
    } else if (night > 0.5) {
        const double frost = far > 0 ? smoothstep(0.3 - 0.6 * far, 0.42 - 0.6 * far, c.fb(R * 0.12, g.sG + 5, 4)) : 0.0;   // patches the map sees, joining toward the antistellar point
        if (frost > 0.5) { s.material = MAT_SALT; s.albedo = 0.55 + 0.2 * far; }
        else if (rel < 0.4 && rf.slopeM < 0.15) { s.material = MAT_GLASS; s.albedo = 0.07 + 0.03 * (0.5 + 0.5 * base); }
        else { s.material = MAT_BASALT; s.albedo = 0.11 + 0.05 * (0.5 + 0.5 * base); }
    } else {
        s.material = MAT_BASALT; s.albedo = 0.12 + 0.08 * (0.5 + 0.5 * base) + 0.04 * roughL;
    }
}

// P-01: the bare core of a giant: smooth heavy ground, broad shallow basins floored with pale dust between low ridges of dark iron basalt,
// the iron itself on their crests (chthonian worlds)
static void groundChthonian(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double basin = smoothstep(0.05, 0.4, -c.fb(R * 0.15, g.sG + 2, 3));
    const double patch = c.fb(6.0, g.sG + 6, 3);   // the regolith's pale and dark patches on the plains
    double h = base * g.mountainAmp - 350.0 * basin;
    double zone = smoothstep(0.1, 0.6, base) * (1 - basin), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone) * 0.7;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    h += 70.0 * c.crackAt(40.0, g.sF + 4, 0.08, 1.0).h * (0.5 + 0.5 * basin);   // wrinkle ridges, long and low: the crust shrank on the core (Mercury's)
    c.craterStack(h);
    s.height = h;
    double crest = c.rg(30.0, g.sC + 3, 3);
    if (basin > 0.5 || cm.flooded > 0.5 || (rel < 0.3 && patch > 0.25)) { s.material = MAT_DUST; s.albedo = 0.40 + 0.06 * base + 0.08 * patch; }
    else if (crest > 0.55 && rel + rf.slopeM > 0.2) { s.material = MAT_METAL; s.albedo = 0.26 + 0.1 * clampd(rimBright, 0, 1); }
    else { s.material = MAT_BASALT; s.albedo = 0.17 + 0.05 * (0.5 + 0.5 * base) + 0.04 * patch + 0.08 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-01: a scorched airless world: the cratered ground with its maria sheets of black glass (cooling cracks, conchoidal ripples), glass
// flows tonguing from obsidian domes, fields of glass beads round the sheets, the highlands' regolith (glass worlds)
static void groundGlass(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double mare = smoothstep(0.02, 0.22, base);   // the sheets: a third of the world
    double h = base * g.mountainAmp * (1 - 0.8 * mare);
    double rel = (1 - mare) * massif, hf = std::max(hillsF, 0.6 * (1 - mare));
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf) * (1 - 0.8 * mare));   // the skeleton (the sheets smooth the relief)
    double cr = 0;
    c.craterStack(cr);
    h += cr * (1 - 0.6 * mare);
    double dome = 0;
    if (scaleVisible(18.0, detailM)) dome = domeField(p, R, 18.0, g.volcanoes, g.sD + 3, 0.25, 260.0, 0.75);   // obsidian domes, flat-topped
    LavaOut lf = lavaFlows(p, R, 18.0, g.volcanoes, g.sD + 3, detailM);   // and their flows (the domes' cells)
    h += dome + 2.0 * lf.dh;
    const double sheet = std::max(mare, clampd(cm.flooded, 0, 1));
    if (sheet > 0.4) {   // the sheets' cooling cracks and conchoidal ripples
        Crack ck = c.crackAt(0.35, g.sF + 5, 0.05, 1.0);
        h += (2.0 * c.rg(0.12, g.sF + 6, 2) - 1.5 * ck.h) * sheet;
    }
    s.height = h;
    const double beads = smoothstep(0.0, 0.15, c.fb(R * 0.08, g.sG + 4, 3)) * smoothstep(-0.14, -0.06, base) * (1 - smoothstep(0.06, 0.1, base));   // fields of them in a band round the sheets' edges
    if (sheet > 0.5 || lf.flow > 0.4 || dome > 30) { s.material = MAT_GLASS; s.albedo = 0.06 + 0.04 * clampd(rimBright, 0, 1) + 0.25 * clampd(cm.rays, 0, 1); }
    else if (beads > 0.5) { s.material = MAT_SAND; s.albedo = 0.22 + 0.05 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.34 + 0.08 * base + 0.12 * clampd(rimBright, 0, 1) - 0.06 * clampd(cm.floorDark, 0, 1) + 0.3 * clampd(cm.rays, 0, 1); }
    s.relief = clampd(std::fabs(cr) / 400.0 + rel + 0.5 * rf.slope, 0, 1);
}

// P-01: under the steam: the plates' highlands cut by scalding rivers, boiling lakes in their hollows, mud in the valleys, sinter round
// the vents; the lowlands sinking into the supercritical fog (the row's `lowFogM`), seas boiling toward the poles (steam worlds)
static void groundSteam(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp + 400.0 + tec.uplift * 900.0;
    { double chainR = c.rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 900.0 + tec.ridge * 400.0; }
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    c.craterStack(h);
    double sinter = 0;   // fields of vents on the high ground, a low mound of sinter round each (`steamVents`: the bursts stand on their centres)
    if (h > 300 && matVisible(0.12, detailM)) {
        const double field = smoothstep(0.2, 0.4, c.fb(R * 0.05, g.sG + 3, 3));
        if (field > 0) visitFeatures(p, R, 2.0, 0.5, g.sF + 8, 0.1, [&](const FeatureHit& f) {
            const double r = 0.06 + 0.12 * f.u[3], x = f.d / r;
            if (x >= 1.0) return;
            sinter = std::max(sinter, field * smoothstep(1.0, 0.7, x));
            h += 4.0 * (1 - x * x) * field * featFade(2 * r, detailM);
        });
    }
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.6, DA);   // scalding rivers, boiling lakes
    s.height = h;
    const double capEdge = g.iceCapLat + 4.0 * gnoise3(p / (R * 0.2), g.sF + 7);   // the polar seas from here poleward
    if (h < 0 && absLatDeg > capEdge) { s.material = MAT_WATER; s.water = 0; s.albedo = 0.24; }
    else if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.24; }
    else if (sinter > 0.5) { s.material = MAT_SALT; s.albedo = 0.75; }
    else if ((h < 250 && relief0 < 0.4) || DA.bed > 0.3 || DA.bank > 0.4) { s.material = MAT_MUD; s.albedo = 0.30 + 0.06 * base; }   // the lowlands and the rivers' banks
    else { s.material = MAT_ROCK; s.albedo = 0.26 + 0.08 * base + 0.04 * clampd(rf.slope * 2.0, 0, 1); }
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
}

// P-01: a dried ocean: under the old sea's level (the row's sea level, metres) the floor is evaporites in bands by depth (gypsum along the
// old shores, halite, potash in the deeps where the last brine dried), brine pools in the deepest hollows, salt domes rising through the
// floor and the glaciers of salt flowing from them, pillars by the shores; gypsum dunes over the shores, the uplands dry (salt worlds)
static void groundSalt(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double h = base * 1400.0 + tec.uplift * 600.0;
    { double chainR = c.rg(40.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 700.0 + tec.ridge * 300.0; }
    double zone = clampd(smoothstep(0.2, 0.7, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);   // the skeleton; hRel the ground before its relief
    const double L = g.seaLevel, depth = std::max(0.0, L - hRel);
    const double oldSea = smoothstep(L + 2.0, L - 2.0, hRel);
    const double bed = L - 3.0 - depth * 0.3 + rf.h * 0.08;   // the floor keeps a tenth of the relief
    if (oldSea > 0) { h += (bed - h) * oldSea; hS += (bed - hS) * oldSea; }
    double dome = 0, pillar = 0, brine = 0;
    LavaOut glacier;
    if (oldSea > 0.3) {   // the domes and their glaciers (the domes' cells)
        if (scaleVisible(14.0, detailM)) dome = domeField(p, R, 14.0, 0.3, g.sD + 7, 0.25, 160.0, 0.8) * oldSea;
        glacier = lavaFlows(p, R, 14.0, 0.3, g.sD + 7, detailM);
        h += dome + 1.5 * glacier.dh * oldSea;
    }
    const double potashDepth = 650.0 + 250.0 * c.fb(R * 0.08, g.sG + 9, 2);   // only the deepest floor held the last brine
    if (oldSea > 0.5 && depth < 60.0 && scaleVisible(0.4, detailM)) { pillar = domeField(p, R, 0.4, 0.25, g.sD + 9, 0.06, 14.0, 0.0); h += pillar; }
    if (oldSea > 0.5 && depth > 120.0 && (scaleVisible(0.25, detailM) || matVisible(0.06, detailM))) {   // the pools on the salt, tens of metres across, flat a metre under the floor
        Worley3 bw = worley3(p / 0.25, g.sF + 10);
        if (unitFromHash(bw.id1) < 0.15) { brine = smoothstep(0.3, 0.18, bw.f1); h += (bed - 1.0 - h) * smoothstep(0.3, 0.6, brine); }
    }
    if (oldSea < 0.5 && hRel > L && hRel - L < 80.0) h += duneField(p, g, lat, g.duneAmp, detailM) * smoothstep(80.0, 20.0, hRel - L);   // gypsum dunes over the shores
    c.craterStack(h);
    applyDrainage(g, unit, detailM, h, &hS, rf.slope, false, false, 0.5, DA);   // wadis down to the floor, playas
    s.height = h;
    if (oldSea > 0.5) {
        if (brine > 0.5) { s.material = MAT_BRINE; s.albedo = 0.34; }
        else if (dome > 40 || glacier.flow > 0.4 || pillar > 4) { s.material = MAT_SALT; s.albedo = 0.80; }
        else if (depth < 40.0 + 20.0 * c.fb(R * 0.05, g.sG + 11, 2)) { s.material = MAT_DUST; s.albedo = 0.68 + 0.05 * base; }   // gypsum, the first to settle
        else if (depth < potashDepth) { s.material = MAT_SALT; s.albedo = 0.86 + 0.04 * base; }
        else { s.material = MAT_POTASH; s.albedo = 0.55 + 0.05 * base; }
    } else if (DA.playa > 0.3) { s.material = MAT_SALT; s.albedo = 0.85; }
    else if (relief0 > 0.5 || DA.bed > 0.4) { s.material = MAT_ROCK; s.albedo = 0.45 + 0.1 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.56 + 0.1 * base + 0.08 * clampd(rimBright, 0, 1); }
    s.oldSea = oldSea;
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
}

// P-02: the lichen world: the thin air's ground, its life in crusts and mosses. Moss in the wet hollows (the basins' floors, the dry valleys'
// beds and banks, the meltwater under the ice caps' edges), crusts in patches on the stable ground round them (rock, gravel, the dark lava)
// and more of them toward the water; by unfaded fields, so every ring and the map agree
static void groundLichen(GroundCtx& c) {
    groundThinAir(c);
    const BodyGen& g = c.g; SurfaceSample& s = c.s; const DrainApply& DA = c.DA;
    if (s.material == MAT_ICE) return;
    const double capEdge = g.iceCapLat + 5.0 * gnoise3(c.p / (c.R * 0.15), g.sF + 7);   // `groundThinAir`'s
    const double melt = smoothstep(capEdge - 3.5, capEdge - 0.3, c.absLatDeg);
    const double wet = std::max(std::max(DA.playa, DA.bed), std::max(0.7 * DA.bank, std::max(0.8 * DA.wetness, melt)));
    const double patch = fbm3(c.p / 0.09, g.sG + 41, 2), hollow = fbm3(c.p / 0.6, g.sG + 42, 2);
    if (wet > 0.35 || (hollow < -0.35 && patch > -0.1)) { s.material = MAT_MOSS; s.albedo = 0.16 + 0.05 * patch; }
    else if (patch > 0.12 - 0.45 * wet && (s.material == MAT_ROCK || s.material == MAT_DUST || s.material == MAT_BASALT)) { s.material = MAT_CRUST; s.albedo = 0.26 + 0.06 * patch; }
}

// P-03: an eyeball: the ocean's one sea locked to a red dwarf and frozen over but within its reach of the substellar point (the pupil, its
// shore wandering by a few degrees), floes scattered over the last three degrees of the open water (the iris, a pale ring from orbit), a band
// of brash a few kilometres wide along the ice front, and past the front the sea glacier: unbroken ice standing 8-20 m over the water at its
// cliff and rising slowly away from the sun under its snow, pressure ridges along the old floes' seams, blue ice where the wind scours it
static void groundEyeball(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM;
    const Vec3 sub = g.locked ? g.lockedDir : Vec3(1, 0, 0);
    const double ang = std::acos(clampd(dot(unit, sub), -1, 1)) / DEG;   // degrees from the substellar point
    const double edge = g.seaReachDeg + 4.0 * gnoise3(p / (R * 0.2), g.sF + 7), front = edge + 0.05;   // the front 5-7 km past the pupil's edge
    const double swell = c.fb(2.0, g.sB, 4) * g.hillAmp;
    const double cover = 0.3 * smoothstep(edge - 3.0, edge, ang) + 0.6 * smoothstep(edge - 0.02, front, ang);   // the share of the plates that are ice
    const double shelf = smoothstep(0.0, 0.04, cover);         // the iris lies on a shelf (the ocean's B-322: the plates rise through an ice foot)
    const double floor = (-150.0 + swell) + ((-6.5 + 0.3 * swell) - (-150.0 + swell)) * shelf;
    s.height = floor; s.material = MAT_WATER; s.water = 0; s.albedo = 0.26; s.biome = BIO_OCEAN;
    if (ang > front) {
        const double over = ang - front, wallDeg = 30.0 / (DEG * R * 1000.0);
        const double cliff = 8.0 + 12.0 * (0.5 + 0.5 * gnoise3(p / 3.0, g.sF + 9));
        double h = (cliff + 0.35 * over) * smoothstep(0.0, wallDeg, over);   // the front's cliff over its first 30 m, then a few tens of metres toward the night
        if (scaleVisible(1.5, detailM)) { const Worley3 fl = worley3(p / 1.5, g.sF + 3); h += 3.5 * smoothstep(0.035, 0.0, fl.f2 - fl.f1) * lodFade(1.5, detailM); }   // the old floes' seams
        h += 0.7 * fbm3(p / 0.12, g.sF + 72, 2) * lodFade(0.12, detailM);   // sastrugi
        s.height = h; s.biome = BIO_ICE;
        const bool blue = fbm3(p / 0.9, g.sF + 74, 2) > 0.36 + 0.2 * smoothstep(90.0, 130.0, ang);   // unfaded; fewer on the night side, under its snow
        s.material = blue ? MAT_ICE : MAT_SNOW; s.albedo = blue ? 0.60 : 0.88;
        return;
    }
    if (cover > 0) {   // the floes: plates of 0.6 km with leads of water between them, the brash's thicker
        const Worley3 fl = worley3(p / 0.6, g.sF + 3);
        if (unitFromHash(fl.id1) < cover) {
            const double d = fl.f2 - fl.f1, rim = smoothstep(0.0, 0.2, d), top = 0.7 + 0.3 * smoothstep(0.2, 0.4, d) + 0.6 * smoothstep(edge - 0.02, front, ang);
            const double h = floor + (top - floor) * rim;
            s.height = h;
            if (h > -0.3) { s.material = MAT_ICE; s.albedo = 0.84; s.biome = BIO_ICE; }
            else s.albedo = 0.26 + 0.2 * clampd((h + 6.5) / 6.5, 0, 1);
        }
    }
}

// P-03: ammonia seas: the methane world's rounded hills of ice and rock (no dunes: no tholin rains here), its channels and domes, a sea of
// ammonia water at the reference level that never freezes, slabs of ice afloat in its shallows (the ocean's floes, few and apart, on a
// shelf near the shore), dark silt along the shores and ammonia frost over the hills' frost line, lower toward the poles
static void groundAmmonia(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * 700.0 + 60.0 - 160.0 * smoothstep(45.0, 75.0, absLatDeg);   // the seas reach furthest toward the poles
    double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    const double chan = c.crackAt(30.0, g.sF + 4, 0.06, 1.0).h;   // channels deepening toward the sea
    h -= chan * 25.0 * smoothstep(400.0, 50.0, h);
    if (scaleVisible(25.0, detailM)) h += domeField(p, R, 25.0, 0.08, g.sD + 21, 0.25, 180.0, 0.0);   // cryovolcanic domes
    c.craterStack(h);
    bool slab = false;
    if (h < 0 && h > -14.0) {   // the shallows: slabs on a shelf (the floor at 6 m under the plates' zone), 30-60 m in from their edges at 0.6 m freeboard
        const Worley3 fl = worley3(p / 0.5, g.sF + 13);
        const double near = smoothstep(-14.0, -6.0, h);
        if (unitFromHash(fl.id1) < 0.32 * near) {
            const double floorS = h + (-6.0 - h) * near, d = fl.f2 - fl.f1, rim = smoothstep(0.0, 0.2, d);
            const double hs = floorS + (0.6 + 0.2 * smoothstep(0.2, 0.4, d) - floorS) * rim;
            if (hs > h) { h = hs; slab = h > -0.3; }
        }
    }
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // rivers of ammonia water to the sea, lakes in the hollows
    s.height = h;
    const double frostLine = g.snowLine * (1.0 - 0.7 * smoothstep(20.0, 80.0, absLatDeg)) + 90.0 * gnoise3(p / 6.0, g.sF + 15);
    if (slab) { s.material = MAT_ICE; s.albedo = 0.80; }
    else if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.24; }
    else if (h > frostLine) { s.material = MAT_SNOW; s.albedo = 0.82 + 0.06 * base; }
    else if (h < 6.0 && h > -30.0 && rf.slopeM < 0.12) { s.material = MAT_SAND; s.albedo = 0.26 + 0.04 * base; }   // the shores' silt
    else { s.material = MAT_ROCK; s.albedo = 0.40 + 0.08 * base + 0.06 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-03: the chemotroph's spring fields: the rifts' floors and scattered fields on the plains (tens of kilometres, so a pool is never cut by
// their edge), a spring a 0.5 km cell at most; `springVents` visits the same features
inline double springField(const GroundCtx& c, double rift) { return std::max(smoothstep(0.5, 0.8, rift), smoothstep(0.3, 0.5, fbm3(c.p / (c.R * 0.06), c.g.sG + 31, 3))); }   // unfaded: the map's and the ground's alike

// P-03: life on the ground's heat: the tectonic crust (rifts with their floors, fissures, cones, ash plains) cold under a dim star and
// frosted, its fissures steaming instead of running with lava; along the rifts' floors and in fields on the plains, hot springs: a
// pool of blue water 12-30 m across on a low mound of sinter, the sinter's terraces round it, pale mats spreading three to six times as
// far as the pool with a lobed edge, the frost kept off the warm ground round them (`springVents`: the surface's steam and spores)
static void groundChemotroph(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF, absLatDeg = c.absLatDeg;
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp * 0.6 + tec.uplift * 700.0;
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    const double hS = h - (rf.h - rf.hCoarse);
    const Worley3 rw = worley3(p / (R * 0.3), g.sF + 5);
    const double dRift = (rw.f2 - rw.f1) * R * 0.3 * 500.0, rift = smoothstep(11000.0, 7000.0, dRift);
    h += (hS - 800.0 - h) * rift;
    double fissure = 0;
    if (rift > 0.5 && (scaleVisible(5.0, detailM) || matVisible(0.3, detailM))) {   // the fissures: trenches 30 m deep and 400 m wide on 5 km cells, their rims mats
        const Worley3 fw = worley3(p / 5.0, g.sF + 6);
        const double dm = (fw.f2 - fw.f1) * 2500.0;
        h -= 30.0 * smoothstep(220.0, 40.0, dm) * featFade(0.44, detailM);
        if (matVisible(0.3, detailM)) fissure = smoothstep(320.0, 120.0, dm) * rift;
    }
    bool caldera = false;
    if (scaleVisible(R * 0.2, detailM)) h += volcanoField(p, R, R * 0.2, g.volcanoes * 0.5, g.sD, 2200.0, caldera) * lodFade(R * 0.2, detailM);
    if (scaleVisible(20.0, detailM)) h += volcanoField(p, R, 20.0, g.volcanoes * 0.7, g.sD + 3, 500.0, caldera);
    c.craterStack(h);
    double pool = 0, sinter = 0, mats = 0;   // the springs, by their own sizes (unfaded: every ring that can draw one draws the same)
    const double field = springField(c, rift);
    if (field > 0.05 && matVisible(0.012, detailM)) {
        visitFeatures(p, R, SPRING_CELL, SPRING_DENS, g.sF + 33, 0.45, [&](const FeatureHit& f) {
            const double rp = 0.012 + 0.018 * f.u[3], x = f.d / rp;   // the pool's radius, km
            const double reach = 3.0 + 3.0 * f.u[4];   // the mats' reach in pool radii, lobed
            const double lobe = 1.0 + 0.3 * gnoise3(p / 0.03, g.sG + 34 + (uint64_t)(f.u[5] * 8));
            if (x >= reach * lobe * 1.3) return;
            pool = std::max(pool, field * smoothstep(1.05, 0.9, x));
            sinter = std::max(sinter, field * smoothstep(2.4, 2.0, x));
            mats = std::max(mats, field * smoothstep(reach * lobe, reach * lobe * 0.8, x));
            h += (2.2 * smoothstep(2.6, 1.0, x) - 1.0 * smoothstep(1.0, 0.8, x)) * field * featFade(2 * rp, detailM);   // the mound, the pool a metre under its rim
        });
    }
    s.height = h;
    const double ash = c.fb(R * 0.1, g.sC + 8, 3);
    // the fields' cores are geyser basins, sinter and mats between the springs in a patchwork of a kilometre (what the globe shows of them);
    // frost in fields of a hundred kilometres on the cold ground, ragged at their edges, more on the heights and toward the poles, none in a
    // basin (unfaded fields)
    const double basin = smoothstep(0.7, 0.9, field), patch = fbm3(p / 1.2, g.sG + 36, 3);
    const double frost = fbm3(p / 120.0, g.sG + 35, 2) + (matVisible(3.0, detailM) ? 0.15 * fbm3(p / 3.0, g.sG + 37, 2) : 0.0) + 0.4 * smoothstep(400.0, 2000.0, hS) + 0.3 * smoothstep(30.0, 70.0, absLatDeg) - basin;   // the edges' raggedness where a ring draws it (the map's texels are wider)
    if (pool > 0.5) { s.material = MAT_BRINE; s.albedo = 0.30; }
    else if (sinter > 0.5 || (basin > 0.5 && patch > 0.12)) { s.material = MAT_SALT; s.albedo = 0.66; }
    else if (mats > 0.5 || fissure > 0.5 || (basin > 0.5 && patch > -0.2)) { s.material = MAT_MATS; s.albedo = 0.52; }
    else if (frost > 0.45) { s.material = MAT_SNOW; s.albedo = 0.70; }
    else if (rel < 0.45 && ash > 0.05 && rift < 0.5) { s.material = MAT_DUST; s.albedo = 0.30 + 0.08 * ash; }
    else { s.material = MAT_BASALT; s.albedo = 0.16 + 0.06 * (0.5 + 0.5 * base); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + rift * 0.3, 0, 1);
}

// P-05: Pluto's ground (New Horizons, 2015). A crust of water ice (its rock: cratered, the high ground frosted with methane, dark red lands of
// tholin along the low latitudes, blades of methane ice hundreds of metres tall on the high ground there); in the basins under the plains' level
// (`seaLevel`, metres) a plain of nitrogen ice churning in cells 15-40 km across, domed 50 m at their middles between dark troughs 70 m deep, pits
// in fields where it sublimes, blocks of water ice afloat in it (hills a kilometre and more high along its shores, smaller ones further out);
// nitrogen glaciers down the valleys of the shores into it
static void groundNitrogen(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp;
    {   // the great plain's basin (Sputnik Planitia's: an old impact basin 15-25 degrees round at a hashed place, its rim ragged), 1.8 km deep
        const Vec3 bc = nitrogenBasinOf(g);
        const double ang = std::acos(clampd(dot(c.unit, bc), -1, 1)) / DEG, rad = (15.0 + 10.0 * unitFromHash(mix64(g.seed ^ 0x51A0ULL))) * (1 + 0.15 * gnoise3(c.unit * 4.0, g.sF + 40));
        h -= 1800.0 * smoothstep(rad, rad * 0.72, ang);
    }
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);
    const double L = g.seaLevel, depth = L - hRel;
    const double plain = smoothstep(-25.0, 25.0, depth);   // under the level: the nitrogen
    double cr = 0; c.craterStack(cr);
    h += cr * (1 - plain);   // the plain is young: no crater
    double dome = 0, trough = 0, block = 0, pit = 0;
    if (plain > 0) {
        double hp = L;
        const bool cv = scaleVisible(26.0, detailM), tm = matVisible(1.0, detailM);
        if (cv || tm) {   // the cells: 26 km on average, their bounds the troughs
            const Worley3 w = worley3(p / 26.0, g.sF + 41);
            const double dKm = (w.f2 - w.f1) * 13.0;   // km from the bound
            trough = smoothstep(0.9, 0.12, dKm);
            dome = smoothstep(0.5, 9.0, dKm);
            if (cv) hp += 50.0 * dome * lodFade(26.0, detailM) - 70.0 * trough * featFade(1.8, detailM);
        }
        const double shore = smoothstep(450.0, 60.0, depth);   // the blocks crowd its shores
        if (shore > 0 && scaleVisible(7.0, detailM)) block += domeField(p, R, 7.0, 0.55, g.sD + 41, 0.38, 1300.0, 0.7) * shore * lodFade(7.0, detailM);
        if (scaleVisible(18.0, detailM)) block += domeField(p, R, 18.0, 0.10, g.sD + 42, 0.14, 260.0, 0.6) * lodFade(18.0, detailM);   // a few drifting further out
        const double pitField = smoothstep(0.15, 0.35, fbm3(p / (R * 0.12), g.sG + 44, 2));
        if (pitField > 0 && (scaleVisible(0.6, detailM) || matVisible(0.15, detailM))) {   // sublimation pits, 150-350 m across
            const Worley3 pw = worley3(p / 0.6, g.sF + 43);
            if (unitFromHash(pw.id1) < 0.6) pit = pitField * smoothstep(0.42, 0.22, pw.f1);
            hp -= 22.0 * pit * featFade(0.3, detailM);
        }
        hp += block;
        h += (hp - h) * plain;
        hS += (L + block - hS) * plain;
        if (plain > 0.5) c.lo.mat = -1;   // the plain buries the traits' walls and sheets under it
    }
    // the glaciers: the valleys of the shores (under their smooth ground) filled flat with nitrogen ice a few hundred metres up from the plain
    const double shoreUp = (1 - plain) * smoothstep(500.0, 80.0, hRel - L), fine = h - hS;
    const double glacier = shoreUp * smoothstep(-6.0, -25.0, fine);
    if (glacier > 0) h += (hS - 8.0 - h) * glacier;
    // the blades: ridges 3-5 km apart drawn out north to south on the high ground of the low latitudes
    double bladeCrest = 0;
    const double bladeZone = (1 - plain) * smoothstep(L + 500.0, L + 1000.0, hRel) * smoothstep(38.0, 22.0, absLatDeg) * smoothstep(-0.1, 0.25, fbm3(p / (R * 0.15), g.sG + 45, 2));
    if (bladeZone > 0 && (scaleVisible(4.0, detailM) || matVisible(0.4, detailM))) {
        const double r = ridged3(Vec3(p.x, p.y, p.z * 0.2) / 4.0, g.sF + 44, 3);   // stretched along the polar axis: blades north to south
        const double crest = smoothstep(0.55, 0.85, r);
        h += 380.0 * bladeZone * crest * featFade(1.2, detailM);
        if (matVisible(0.4, detailM)) bladeCrest = bladeZone * crest;
    }
    s.height = h;
    // the uplands' colours: tholin by an unfaded field of the low latitudes, methane frost on the heights and toward the poles
    const double tholin = fbm3(p / (R * 0.22), g.sG + 46, 3) + (matVisible(12.0, detailM) ? 0.3 * fbm3(p / 25.0, g.sG + 47, 2) : 0.0) + 0.75 * smoothstep(32.0, 8.0, absLatDeg) - 0.45;
    const double frost = (hRel - L - 700.0) / 500.0 + smoothstep(40.0, 70.0, absLatDeg) + 0.4 * fbm3(p / (R * 0.06), g.sG + 48, 2) - (tholin > 0 ? 0.6 : 0.0);
    if (plain > 0.5) {
        if (block > 120.0) { s.material = MAT_ROCK; s.albedo = 0.62; }   // a block of water ice afloat
        else { s.material = MAT_ICE; s.albedo = 0.80 + 0.05 * dome - 0.28 * trough - 0.1 * pit; }   // the troughs dark with the tholin they gather
    } else if (glacier > 0.5) { s.material = MAT_ICE; s.albedo = 0.78; }
    else if (bladeCrest > 0.4 || frost > 0.5) { s.material = MAT_SNOW; s.albedo = 0.86 + 0.04 * base; }
    else if (tholin > 0) { s.material = MAT_DUST; s.albedo = 0.14 + 0.04 * base + 0.04 * clampd(rimBright, 0, 1); }
    else { s.material = MAT_ROCK; s.albedo = 0.50 + 0.10 * base + 0.08 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1) * (1 - 0.7 * plain);
}

// P-05: Triton's ground (Voyager 2, 1989): a young crust of ice with few craters. Over most of it the cantaloupe terrain (dimples 25-35 km
// across between low ridges, smaller ones in them, long double ridges crossing it); smooth plains where its ice flooded and froze, set lower
// behind their scarps; south of the cap's edge (`iceCapLat` and a wander) a cap of pink nitrogen frost, the dark streaks of its plumes laid
// downwind of their vents (`plumeVents`: 45-110 km long, widening to 19 km)
static void groundCryovolcanic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.5, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.3, hf = hillsF * 0.4;
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);
    const double cant = smoothstep(-0.3, -0.1, fbm3(p / (R * 0.35), g.sC + 3, 3));   // the cantaloupe terrain (an unfaded field: the plains' edge on every ring)
    if (cant > 0) {
        auto dimples = [&](double cellKm, double deepM, double ridgeM, uint64_t seed) {
            if (!scaleVisible(cellKm, detailM)) return;
            const Vec3 q = p / cellKm, warp(gnoise3(q * 1.3, seed + 1), gnoise3(q * 1.3, seed + 2), gnoise3(q * 1.3, seed + 3));   // rounded, not straight-edged
            const Worley3 w = worley3(q + warp * 0.3, seed);
            h += cant * (-deepM * smoothstep(0.55, 0.0, w.f1) + ridgeM * smoothstep(0.1, 0.0, w.f2 - w.f1)) * lodFade(cellKm, detailM);
        };
        dimples(30.0, 140.0, 50.0, g.sF + 50); dimples(7.0, 35.0, 12.0, g.sF + 53);
        const double rf2 = featFade(1.2, detailM);   // the double ridges: two crests 600 m from the line, a trough between
        if (rf2 > 0) {
            const Worley3 w = worley3(p / (R * 0.3), g.sF + 54);
            const double dm = (w.f2 - w.f1) * R * 0.3 * 500.0;
            h += cant * rf2 * (90.0 * std::exp(-std::pow((dm - 600.0) / 260.0, 2.0)) - 40.0 * std::exp(-std::pow(dm / 300.0, 2.0)));
        }
    }
    h += (hS - 160.0 - h) * 0.85 * (1 - cant);   // the plains: flooded flat and frozen, set lower behind their scarps
    c.craterStack(h);
    s.height = h;
    // the cap and its streaks
    const double latDeg = c.lat / DEG, edge = cryoCapEdge(g, unit);
    const bool cap = latDeg < -edge;
    double streak = 0;
    if (cap || latDeg < -edge + 2.0) {
        visitFeatures(p, R, PLUME_CELL, plumeDensity(g), g.sF + 52, 1.0, [&](const FeatureHit& f) {
            const double len = plumeStreakKm(f.u[3]);
            if (f.d > len + 20) return;
            const Vec3 n = normalize(f.c), w = prevailingWind(g, n), across = cross(n, w), d = p - f.c;
            const double along = dot(d, w), off = std::fabs(dot(d, across));
            if (along < 0 || along > len) return;
            const double half = 0.5 + 0.08 * along;   // km: narrow at the vent, 19 km wide at the end of the longest
            if (off > half || !matVisible(2 * half, detailM)) return;
            if (std::asin(clampd(n.z, -1, 1)) / DEG > -cryoCapEdge(g, n)) return;   // a vent on the cap (the dearest test last)
            streak = std::max(streak, std::pow(1 - along / len, 0.6) * (1 - off / half));
        });
    }
    if (streak > 0.3) { s.material = MAT_DUST; s.albedo = 0.20 + 0.15 * (1 - streak); }
    else if (cap) { s.material = MAT_SNOW; s.albedo = 0.86 + 0.04 * base; }
    else if (cant < 0.5) { s.material = MAT_ROCK; s.albedo = 0.62 + 0.06 * base; }
    else { s.material = MAT_ICE; s.albedo = 0.56 + 0.08 * base + 0.08 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-05: Mars' south polar cap on a colder world: the thin air's ground (canyons, shields and their dark plains, dunes, the old dry valleys)
// under caps of dry ice to `iceCapLat` (carbon dioxide over water ice, pitted like Swiss cheese: flat-floored pits 150-450 m across and 8 m
// deep); round each cap a band 9 degrees wide of layered dust where the gas under the seasonal ice carved the spiders (radial channels a
// metre or two deep converging on a centre, 150-400 m across: `springJets`); over that band in the cold half of the year the seasonal frost
// (it reaches 12 degrees past the cap's edge in winter and is gone by midsummer), the dark fans the spring's jets laid on it downwind
static void groundDryIce(GroundCtx& c) {
    groundThinAir(c);
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg;
    const double edge = dryCapEdge(g, unit);
    if (absLatDeg > edge) {   // the residual cap (`groundThinAir` made it ice): the pits
        if (scaleVisible(0.6, detailM) || matVisible(0.15, detailM)) {
            const Worley3 w = worley3(p / 0.6, g.sF + 60);
            if (unitFromHash(w.id1) < 0.55) {
                const double pit = smoothstep(0.40, 0.30, w.f1);   // a flat floor inside steep walls
                s.height -= 8.0 * pit * featFade(0.3, detailM);
                if (matVisible(0.15, detailM) && pit > 0.5) s.albedo = 0.62;
            }
        }
        return;
    }
    if (absLatDeg < edge - 12.0) return;
    const bool frost = absLatDeg > edge - dryFrostDeg(g, c.lat >= 0);
    const bool band = absLatDeg > edge - 9.0;
    double channel = 0, fan = 0;
    if (band && (scaleVisible(0.05, detailM) || matVisible(0.02, detailM))) {
        visitFeatures(p, R, SPIDER_CELL, SPIDER_DENS, g.sF + 61, 1.0, [&](const FeatureHit& f) {
            const double reach = 0.15 + 0.25 * f.u[3], x = f.d / reach;
            if (x >= 1 && (!frost || f.d > 0.25)) return;   // neither its channels nor its fan reach here
            const Vec3 n = normalize(f.c);
            const double la = std::fabs(std::asin(clampd(n.z, -1, 1))) / DEG, e2 = dryCapEdge(g, n);
            if (la > e2 || la < e2 - 9.0) return;
            if (x < 1) {   // the spider: 6-11 channels with a branch each, deepest at the middle
                const Vec3 e1 = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2v = cross(n, e1);
                const Vec3 d = normalize(p - f.c);
                const double ang = std::atan2(dot(d, e2v), dot(d, e1));
                const int arms = 6 + (int)(f.u[1] * 6);
                const double wob = 0.5 * gnoise3(p / 0.05, g.sG + 62);
                const double arm = std::pow(0.5 + 0.5 * std::cos(arms * ang + f.u[0] * TAU + wob), 26.0), twig = std::pow(0.5 + 0.5 * std::cos(2 * arms * ang + f.u[2] * TAU + wob), 40.0) * smoothstep(0.3, 0.6, x);
                const double ch = std::max(arm, twig) * (1 - x) + smoothstep(0.12, 0.0, x);
                channel = std::max(channel, ch);
            }
            if (frost) {   // its fan: dark dust laid downwind on the frost, 60-240 m long
                const Vec3 w = prevailingWind(g, n), across = cross(n, w), dd = p - f.c;
                const double len = (0.06 + 0.18 * f.u[4]), along = dot(dd, w), off = std::fabs(dot(dd, across));
                const double half = 0.012 + 0.25 * along;
                if (along > -0.01 && along < len && off < half) fan = std::max(fan, (1 - along / len) * (1 - off / half));
            }
        });
        s.height -= 1.6 * clampd(channel, 0, 1) * featFade(0.03, detailM);
        if (!matVisible(0.02, detailM)) { channel = 0; fan = 0; }
    }
    if (frost) {
        if (fan > 0.3) { s.material = MAT_BASALT; s.albedo = 0.18 + 0.2 * (1 - fan); }
        else { s.material = MAT_SNOW; s.albedo = 0.72 - 0.08 * clampd(channel, 0, 1); }
    } else if (band) {
        if (channel > 0.45) { s.material = MAT_BASALT; s.albedo = 0.26; }
        else if (s.material != MAT_ROCK) { s.material = MAT_DUST; s.albedo = 0.52 + 0.06 * fbm3(p / 2.0, g.sG + 63, 2); }   // the layered deposits
    }
}

// P-05: a rogue world a star caught: the thin air's ground of the world it was (its mountains, craters and shields, its old river courses
// dry: the drainage), under its air fallen as snow: nitrogen and carbon monoxide snow `snowLine` metres deep on the open ground, the hollows
// of the fine relief filled near flat (the craters, the valleys' floors), drifted into low dunes, bare rock on the steep ground that sheds it;
// domes of new ice where the warmth of its interior pushed up through (`volcanoes`)
static void groundWanderer(GroundCtx& c) {
    groundThinAir(c);
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM;
    double h = s.height;
    const double fill = clampd(c.hSmooth - h, 0, 600);
    const double slope = c.slopeM;
    const double shed = smoothstep(0.32, 0.55, slope);
    // the fallen air lies on the low ground (it froze out last there, and the drifts slid down to it): the plains under the old world's middle
    // height deep in it, thinning to nothing 900 m over it (an unfaded field, its edge ragged), the highlands' old rock bare
    const double lowness = smoothstep(900.0, -150.0, c.hSmooth + 350.0 * fbm3(p / (R * 0.08), g.sG + 90, 3) + (matVisible(4.0, detailM) ? 80.0 * fbm3(p / 6.0, g.sG + 91, 2) : 0.0));
    const double depthM = (g.snowLine + 0.85 * fill) * lowness * (1 - shed);
    h += depthM;
    if (depthM > 1.0) h += duneField(p, g, c.lat, 3.0, detailM) * smoothstep(1.0, 8.0, depthM) * smoothstep(0.2, 0.05, slope);   // drifts of snow
    double dome = 0;
    if (scaleVisible(R * 0.08, detailM)) dome += domeField(p, R, R * 0.08, g.volcanoes, g.sD + 31, 0.3, 450.0, 0.85) * lodFade(R * 0.08, detailM);
    if (scaleVisible(6.0, detailM)) dome += domeField(p, R, 6.0, 0.5 * g.volcanoes, g.sD + 32, 0.3, 90.0, 0.8);
    h += dome;
    s.height = h;
    if (dome > 40.0) { s.material = MAT_ICE; s.albedo = 0.78; }
    else if (depthM > 1.5) { s.material = MAT_SNOW; s.albedo = 0.84 + 0.04 * clampd(fill / 50.0, 0, 1); }
    else if (shed > 0.5 || c.slopeM > 0.25) { s.material = MAT_ROCK; s.albedo = 0.24; }
    else { s.material = MAT_DUST; s.albedo = 0.18 + 0.2 * lowness; }   // frosted where the snow begins
}

// P-05: Ceres' ground (Dawn, 2015-18): a dwarf's cratered ground, dark with carbon; on the floors of its young craters the faculae, salt
// dried out of brine that welled up from below (a bright dome at the middle, spots scattered over the rest of the floor); and one lone
// mountain of ice (Ahuna Mons: 4 km high, a top 10 km across on flanks steep to its foot at 12 km, the flanks streaked with salt down
// their fall lines) at a place hashed from the world's seed
static void groundBrine(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.5, g.sA, 5);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    double cr = 0; c.craterStack(cr);
    h += cr;
    // the lone mountain
    const Vec3 mc = loneMountainOf(g) * R;
    const double dKm = length(p - mc);
    double mtn = 0, streakM = 0;
    if (dKm < 14.0) {
        mtn = smoothstep(12.0, 5.0, dKm);
        const double top = 3600.0 + 400.0 * unitFromHash(mix64(g.seed ^ 0xA4A2ULL));
        h += top * (mtn * mtn * (3 - 2 * mtn)) + 25.0 * c.fb(1.5, g.sG + 70, 3) * smoothstep(6.0, 4.0, dKm);   // a cracked top
        if (dKm > 4.5 && dKm < 12.5 && matVisible(0.3, detailM)) {   // the bright streaks down the flanks
            const Vec3 n = normalize(mc), e1 = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(n, e1);
            const Vec3 d = normalize(p - mc);
            const double ang = std::atan2(dot(d, e2), dot(d, e1));
            streakM = std::pow(0.5 + 0.5 * std::cos(ang * 23.0 + 2.0 * std::sin(ang * 5.0)), 6.0) * smoothstep(12.5, 9.0, dKm);
        }
    }
    s.height = h;
    const double young = clampd(cm.young, 0, 1);
    const double spots = young > 0.05 && matVisible(0.4, detailM) ? fbm3(p / 1.2, g.sG + 71, 3) : -1.0;
    if (young > 0.45 || (young > 0.05 && spots > 0.3 - 0.3 * young)) { s.material = MAT_SALT; s.albedo = 0.86 + 0.1 * young; }   // the faculae
    else if (mtn > 0.05) {
        if (streakM > 0.45 || dKm < 5.0) { s.material = MAT_SALT; s.albedo = 0.70; }
        else { s.material = MAT_ICE; s.albedo = 0.36; }
    } else {
        s.material = rf.slopeM > 0.35 ? MAT_ROCK : MAT_DUST;
        s.albedo = 0.14 + 0.04 * base + 0.10 * clampd(cm.rimBright, 0, 1) - 0.03 * clampd(cm.floorDark, 0, 1) + 0.25 * clampd(cm.rays, 0, 1);
    }
    s.relief = clampd(std::fabs(cr) / 400.0 + rel + 0.5 * mtn, 0, 1);
}

// P-05: Eris's and Makemake's ground: smooth plains under a fresh coat of methane frost (the brightest ground there is), low rounded relief,
// few craters, fields of sublimation pits (round pits 100-250 m across and 18 m deep), a glaze of nitrogen ice in the hollows, and red
// streaks of tholin drawn out along the latitudes where the frost lies thin
static void groundFrost(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.5, hf = hillsF * 0.6;
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);
    c.craterStack(h);
    double pit = 0;
    const double pitField = smoothstep(0.05, 0.3, fbm3(p / (R * 0.1), g.sG + 80, 2));
    if (pitField > 0 && (scaleVisible(0.4, detailM) || matVisible(0.1, detailM))) {
        const Worley3 w = worley3(p / 0.4, g.sF + 80);
        if (unitFromHash(w.id1) < 0.55) pit = pitField * smoothstep(0.38, 0.2, w.f1);
        h -= 18.0 * pit * featFade(0.2, detailM);
    }
    s.height = h;
    const Vec3 sq(p.x, p.y, p.z * 10.0);   // drawn out east and west
    const double streak = fbm3(sq / (R * 0.15), g.sG + 81, 3) + (matVisible(5.0, detailM) ? 0.25 * fbm3(sq / (R * 0.03), g.sG + 82, 2) : 0.0);
    if (streak > 0.2) { s.material = MAT_DUST; s.albedo = 0.52 + 0.2 * (0.5 - streak); }   // tholin
    else if (pit > 0.5 && matVisible(0.1, detailM)) { s.material = MAT_ROCK; s.albedo = 0.55; }
    else if (h - hS < -12.0) { s.material = MAT_ICE; s.albedo = 0.86; }   // the glaze in the hollows
    else { s.material = MAT_SNOW; s.albedo = 0.94 + 0.03 * base; }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-06: Ganymede's ground (Galileo, 1996-2000): its oldest crust in great dark polygons (the regiones: dark with dust, heavily cratered, the
// rims of their craters bright with ice, the furrows of ancient impacts in arcs across them), and between them lanes of bright ice (the
// sulci) where the crust pulled apart: each lane a few hundred metres lower than the polygons it parts, filled with tilted blocks whose
// ridges run along it (3-8 km apart, 300-700 m high), a finer set over them; within a lane swaths of grooves at angles of their own cut
// across the rest; fewer craters on the lanes; frost poleward of `iceCapLat` over both
static void groundGrooved(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    // the polygons and the lanes along their boundaries (each boundary its own width, drop, groove spacing and height: both sides alike)
    const double cell = R * 0.55;
    const Vec3 q = p / cell, warp(gnoise3(q * 2.3, g.sF + 81), gnoise3(q * 2.3, g.sF + 82), gnoise3(q * 2.3, g.sF + 83));
    const Worley3 w = worley3(q + warp * 0.06, g.sF + 80);
    const double dKm = (w.f2 - w.f1) * cell * 0.5;   // km from the polygons' boundary
    const uint64_t e = mix64(w.id1 ^ w.id2);
    const double half = cell * (0.07 + 0.11 * unitFromHash(e));
    const double lane = smoothstep(half, half - 6.0, dKm);   // 1 in the lane, its edge a scarp a few kilometres wide
    if (lane > 0) {
        h -= lane * (200.0 + 300.0 * unitFromHash(mix64(e + 1)));
        const double sp = 3.0 + 5.0 * unitFromHash(mix64(e + 2)), amp = 300.0 + 400.0 * unitFromHash(mix64(e + 3));
        const double fg = featFade(sp * 0.5, detailM);
        if (fg > 0) {
            double coord = dKm, spU = sp, ampU = amp;
            {   // a swath of grooves at its own angle (a third of the cells of a grid of 0.12 of the polygons)
                const Worley3 w2 = worley3(p / (cell * 0.12) + warp * 0.3, g.sF + 85);
                if (unitFromHash(mix64(w2.id1 + 7)) < 0.35) {
                    const Vec3 d3 = normalize(Vec3(unitFromHash(mix64(w2.id1 + 8)) - 0.5, unitFromHash(mix64(w2.id1 + 9)) - 0.5, unitFromHash(mix64(w2.id1 + 10)) - 0.5));
                    coord = dot(p, d3); spU = sp * (0.6 + 0.5 * unitFromHash(mix64(w2.id1 + 11))); ampU = amp * 0.7;
                }
            }
            const double rr = coord / spU + unitFromHash(mix64(e + 4)), fr = rr - std::floor(rr);
            const double block = fr < 0.2 ? fr / 0.2 : (1 - fr) / 0.8;   // a tilted block: its scarp, then its long back
            const double vary = 0.6 + 0.4 * gnoise3(p / (sp * 6.0), g.sF + 84);
            h += lane * fg * ampU * vary * (block - 0.5);
            const double ff = featFade(0.4, detailM);   // the finer set: ridges 0.6-1.1 km apart, 40-80 m high
            if (ff > 0) h += lane * ff * (20.0 + 20.0 * unitFromHash(mix64(e + 6))) * std::sin(TAU * coord / (0.6 + 0.5 * unitFromHash(mix64(e + 5))));
        }
    }
    double cr = 0; c.craterStack(cr);
    h += cr * (1 - 0.6 * lane);
    if (lane < 1 && scaleVisible(4.0, detailM)) {   // the furrows of the polygons' ancient impacts: arcs of troughs 12-22 km apart
        double furrow = 0;
        visitFeatures(p, R, R * 0.9, 0.6, g.sF + 86, 0.6, [&](const FeatureHit& f) {
            const double reach = R * (0.3 + 0.25 * f.u[3]);
            if (f.d > reach || f.d < reach * 0.25) return;
            const double fsp = 12.0 + 10.0 * f.u[4], x = f.d / fsp - std::floor(f.d / fsp);
            furrow = std::max(furrow, smoothstep(0.12, 0.0, std::fabs(x - 0.5)) * smoothstep(reach, reach * 0.7, f.d));
        });
        h -= 150.0 * furrow * (1 - lane) * featFade(1.5, detailM);
    }
    s.height = h;
    const double polar = c.absLatDeg + 4.0 * gnoise3(unit * 7.0, g.sF + 87), frost = smoothstep(g.iceCapLat - 4, g.iceCapLat + 6, polar);   // a thin frost brightening
    if (polar > g.iceCapLat + 14) { s.material = MAT_SNOW; s.albedo = 0.72 + 0.08 * base + 0.08 * lane; }   // what lies under it; a cap of it nearer the pole
    else if (lane > 0.5) { s.material = MAT_ICE; s.albedo = 0.55 + 0.08 * base + 0.06 * clampd(cm.rimBright, 0, 1) + 0.2 * clampd(cm.rays, 0, 1) + 0.12 * frost; }
    else { s.material = MAT_DUST; s.albedo = 0.24 + 0.05 * base + 0.18 * clampd(cm.rimBright, 0, 1) + 0.25 * clampd(cm.rays, 0, 1) + 0.2 * frost; }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + 0.3 * lane, 0, 1);
}

// P-06: Iapetus's ground (Cassini, 2004-07): a moon that faces its giant sweeps dark dust onto the face it leads with (the giant's outer moons'
// debris); the darkened ice warms, sublimes and freezes again on the bright face, a runaway (Spencer and Denk 2010): the leading face coal-black
// round the apex (`twoFacedApex`) to fifty degrees of latitude and a quarter turn and more round the equator either way, the rest bright ice,
// the boundary ragged at every scale; along it the slopes that face the equator dark and those that face the poles bright, the craters' floors
// dark; a ridge along the equator 8-15 km high and 22-32 km across over the dark face and on past its edge, broken into lone peaks beyond; craters at
// every scale, basins half the radius across
static void groundTwoFaced(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF, lat = c.lat;
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    double cr = 0; c.craterStack(cr);
    h += cr;
    const Vec3 a = twoFacedApex(g);
    const double lon = std::atan2(unit.y, unit.x), off = wrapAngle(lon - std::atan2(a.y, a.x));
    {   // the equatorial ridge
        const double ridgeH = 8000.0 + 7000.0 * unitFromHash(mix64(g.seed ^ 0x7F0A1ULL)), ridgeW = 7.0 + 3.0 * unitFromHash(mix64(g.seed ^ 0x7F0A2ULL));   // m; km (its flanks 30-40 degrees)
        const Vec3 ring(std::cos(lon), std::sin(lon), 0);   // the equator's point of this meridian (the ridge's noises run along it)
        const double xKm = (lat - 0.006 * gnoise3(ring * 4.0, g.sF + 92)) * R;   // the crest wanders off the equator
        if (std::fabs(xKm) < 1.6 * ridgeW) {
            const double cont = smoothstep(2.5, 2.0, std::fabs(off)), peaks = smoothstep(0.15, 0.5, gnoise3(ring * 9.0, g.sF + 93));   // on past the dark face's edge
            const double along = (0.75 + 0.25 * gnoise3(ring * 14.0, g.sF + 94)) * (cont + (1 - cont) * peaks);
            const double t = clampd(1 - std::fabs(xKm) / (1.6 * ridgeW), 0, 1);
            h += ridgeH * along * t * t * (3 - 2 * t) + 500.0 * c.rg(6.0, g.sF + 95, 4) * t * along;
        }
    }
    s.height = h;
    // the two faces: dark round the apex and along the equator, the boundary ragged (unfaded fields: the same on every ring)
    const double dd = dot(unit, a), cl = std::cos(lat);
    double dark = 0.45 * dd + 0.7 * cl * cl - 0.6 + 0.3 * fbm3(unit * 1.8, g.sF + 96, 4) + 0.06 * fbm3(p / (R * 0.04), g.sF + 97, 3);
    const double band = smoothstep(0.03, 0.0, std::fabs(dark));   // a band some tens of kilometres wide
    if (band > 0) {   // the runaway's own pattern where the faces meet: dark on the slopes that face the equator and in the craters' floors
        const Vec3 north = normalize(Vec3(0, 0, 1) - unit * unit.z);
        const double eq = dot(rf.gradM, north) * (lat >= 0 ? 1 : -1);   // + where the ground rises toward the pole (it faces the equator)
        dark += band * (0.18 * clampd(eq * 5.0, -1, 1) + 0.12 * clampd(cm.floorDark, 0, 1) - 0.08 * clampd(cm.rimBright, 0, 1));
    }
    if (dark > 0) { s.material = MAT_DUST; s.albedo = 0.04 + 0.02 * clampd(base + 0.5, 0, 1); }
    else { s.material = MAT_ICE; s.albedo = 0.58 + 0.08 * base + 0.08 * clampd(cm.rimBright, 0, 1) + 0.15 * clampd(cm.rays, 0, 1) - 0.12 * smoothstep(-0.25, 0.0, dark); }   // greyer near the dark
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-06: Enceladus's ground (Cassini, 2005-15): the whitest ground there is (the fallout of its own jets), old cratered plains to the north and
// in patches, young plains ridged along a net of 60 km; south of `iceCapLat` the south polar terrain, lower, behind a belt of folds along its
// edge: the four tiger stripes (`tigerStripesOf`), each a rift 500 m deep and 2 km wide between ridges 160 m high, its ice coarse and blue to
// 4-6 km out, the ground between grooved along them; the curtains rise from them (`curtainVents`)
static void groundTiger(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF, latDeg = c.lat / DEG;
    const double base = c.fb(R * 0.5, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.3, hf = hillsF * 0.4;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    const double old = smoothstep(-0.05, 0.2, fbm3(unit * 2.2, g.sC + 60, 3) + 0.4 * std::sin(c.lat));
    double cr = 0; c.craterStack(cr);
    h += cr * (0.2 + 0.8 * old);
    if (old < 1) {
        const double fr = featFade(1.0, detailM);
        if (fr > 0) {
            const Worley3 w = worley3(p / 60.0 + Vec3(gnoise3(p / 90.0, g.sF + 62), gnoise3(p / 90.0, g.sF + 63), 0) * 0.3, g.sF + 64);
            const double dm = (w.f2 - w.f1) * 60.0 * 500.0;   // metres from the line
            h += (1 - old) * fr * 140.0 * std::exp(-(dm / 500.0) * (dm / 500.0));
        }
    }
    const double edge = g.iceCapLat + 3.0 * gnoise3(unit * 5.0, g.sF + 61);
    const double inKm = (-edge - latDeg) * DEG * R;   // km inside the south polar terrain's edge (negative outside)
    double sd = 1e9;
    if (inKm > -40) {
        const double belt = smoothstep(-30, -5, inKm) * smoothstep(40, 10, inKm), fb = featFade(4.5, detailM);
        if (belt > 0 && fb > 0) h += belt * fb * 220.0 * std::sin(TAU * inKm / 9.0);   // the folds along its edge
        h -= 350.0 * smoothstep(-5, 25, inKm);
        if (inKm > 0) {
            const TigerStripes T = tigerStripesOf(g);
            sd = stripeDistanceKm(T, R, unit);
            const double ft = featFade(1.0, detailM);
            if (ft > 0) h += ft * (-500.0 * std::exp(-(sd / 0.8) * (sd / 0.8)) + 160.0 * std::exp(-((sd - 2.0) / 0.7) * ((sd - 2.0) / 0.7)));
            const double fg = featFade(0.5, detailM);
            if (fg > 0) h += fg * 22.0 * std::sin(TAU * dot(unit, T.e2) * R / 1.1) * smoothstep(0, 20, inKm);
        }
    }
    s.height = h;
    const double blue = sd < 1e8 ? smoothstep(6.0, 3.0, sd + 2.0 * fbm3(p / 6.0, g.sG + 65, 2)) : 0.0;
    if (blue > 0.5) { s.material = MAT_ICE; s.albedo = 0.72 + 0.1 * (1 - blue); }
    else { s.material = MAT_SNOW; s.albedo = 0.94 + 0.04 * base + 0.03 * clampd(cm.rimBright, 0, 1) - 0.06 * old; }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-06: Miranda's ground (Voyager 2, 1986): a moon remade in part by the tides of a past resonance: blocks of unlike old ground fitted together
// (rolling cratered country, each block its own shade, roughness and craters, a low scarp where two meet), up to three coronae (`coronaeOf`: a
// belt of parallel ridges and troughs following each edge in bright and dark bands, a rougher core, the floor lower), two chasms where the crust
// pulled apart, and along part of the first corona's edge the great cliff (Verona Rupes): its floor dropped and the old ground lifted, a wall of
// 15-20 km a few kilometres across at its foot
static void groundPatchwork(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const Worley3 bw = worley3(p / (R * 0.4), g.sF + 101);   // the old ground's blocks
    const double bEdge = smoothstep(0.0, 0.08, bw.f2 - bw.f1), bShade = (unitFromHash(mix64(bw.id1 + 2)) - 0.5) * 0.16;
    const double bCraters = 0.5 + unitFromHash(mix64(bw.id1 + 3)), bRough = 0.6 + 0.8 * unitFromHash(mix64(bw.id1 + 4));
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp + (unitFromHash(mix64(bw.id1 + 1)) - 0.5) * 500.0 * bEdge;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif * bRough, hf = std::max(hillsF, 0.5 * zone) * bRough;
    ReliefSum rf = c.relief(h, clampd(rel, 0, 1), clampd(hf, 0, 1), reliefAmp(g, clampd(rel, 0, 1), clampd(hf, 0, 1)));
    double cr = 0; c.craterStack(cr);
    Corona C[3]; const int nc = coronaeOf(g, C);
    double inCorona = 0, bandV = 0, coreV = 0, faceV = 0;
    for (int k = 0; k < nc; k++) {
        double rho, th, inKm;
        if (!coronaAt(C[k], R, unit, rho, th, inKm) || rho > 1.3) continue;
        const double inside = smoothstep(1.03, 0.97, rho);
        inCorona = std::max(inCorona, inside);
        h -= 600.0 * inside;
        if (inside > 0) {
            const double belt = smoothstep(0.4, 0.5, rho) * inside, fs = featFade(C[k].sp * 0.5, detailM);
            if (fs > 0) { const double x = inKm / C[k].sp - std::floor(inKm / C[k].sp); h += belt * fs * C[k].amp * (std::pow(0.5 - 0.5 * std::cos(TAU * x), 1.5) - 0.4); }
            if (belt > 0.5) bandV = std::sin(TAU * inKm / (0.35 * std::sqrt(C[k].a * C[k].b)) + C[k].band + 0.8 * gnoise3(p / 40.0, g.sF + 102));
            const double core = smoothstep(0.5, 0.4, rho) * inside;
            if (core > 0) { h += core * 400.0 * c.rg(25.0, g.sF + 103, 4); coreV = std::max(coreV, core); }
        }
        if (k == 0) {   // the great cliff
            double th0, span, H; cliffOf(g, th0, span, H);
            const double seg = smoothstep(span, span * 0.6, std::fabs(wrapAngle(th - th0)));
            if (seg > 0) {
                const double sKm = -inKm, face = 3.0, st = smoothstep(-face, face, sKm), fall = std::exp(-std::fabs(sKm) / 70.0);
                h += H * seg * (st - 0.5) * fall;
                faceV = std::max(faceV, seg * (1 - std::fabs(2 * st - 1)));
            }
        }
    }
    h += cr * bCraters * (1 - 0.7 * inCorona);
    for (int k = 0; k < 2; k++) {   // the chasms: graben along arcs of great circles
        auto hu = [&](int j) { return unitFromHash(mix64(g.seed ^ (0x9A7D0ULL + (uint64_t)(k * 16 + j) * 0x9E3779B97F4A7C15ULL))); };
        const Vec3 nrm = normalize(Vec3(hu(0) - 0.5, hu(1) - 0.5, hu(2) - 0.5));
        const double offKm = dot(unit, nrm) * R, wHalf = 6.0 + 6.0 * hu(3);
        if (std::fabs(offKm) > wHalf + 12.0) continue;
        const Vec3 mid = normalize(cross(nrm, normalize(Vec3(hu(4) - 0.5, hu(5) - 0.5, hu(6) - 0.5))));
        const Vec3 inPlane = normalize(unit - nrm * dot(unit, nrm));
        const double ang = std::acos(clampd(dot(inPlane, mid), -1, 1)), halfArc = 0.5 + 0.5 * hu(7);
        const double taper = smoothstep(halfArc, halfArc * 0.8, ang);
        if (taper <= 0) continue;
        const double depth = 2500.0 + 2500.0 * hu(8), x = std::fabs(offKm);
        const double floorP = smoothstep(wHalf + 3.0, wHalf, x), shoulder = smoothstep(wHalf + 12.0, wHalf + 5.0, x) * smoothstep(wHalf + 2.0, wHalf + 5.0, x);
        h += taper * (-depth * floorP + 300.0 * shoulder);
        if (floorP > 0.05 && floorP < 0.95) faceV = std::max(faceV, taper);
    }
    s.height = h;
    if (faceV > 0.3) { s.material = MAT_ROCK; s.albedo = 0.36 + 0.06 * base; }
    else if (inCorona > 0.5 && coreV < 0.5 && bandV < -0.2) { s.material = MAT_DUST; s.albedo = 0.34 + 0.04 * base; }   // a corona's dark bands
    else if (inCorona > 0.5) { s.material = MAT_ICE; s.albedo = (coreV > 0.5 ? 0.42 : 0.58) + 0.06 * base + 0.08 * clampd(cm.rimBright, 0, 1); }
    else { s.material = MAT_ICE; s.albedo = 0.34 + bShade + 0.05 * base + 0.12 * clampd(cm.rimBright, 0, 1) + 0.2 * clampd(cm.rays, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + faceV, 0, 1);
}

// P-06: the nearest and second nearest of a grid's features projected onto the sphere (km from a point), and the nearest's own number: cells packed
// edge to edge on the surface (`worley3` cuts its cells through the sphere, so its nearest feature is seldom under a point: its cells' middles
// are rare on the ground)
static void surfaceCells(const Vec3& p, double R, double cellKm, uint64_t seed, double& f1, double& f2, double& u1) {
    f1 = f2 = 1e18; u1 = 0.5;
    visitFeatures(p, R, cellKm, 1.0, seed, 3.0, [&](const FeatureHit& f) {
        if (f.d < f1) { f2 = f1; f1 = f.d; u1 = f.u[0]; } else if (f.d < f2) f2 = f.d;
    });
}
// P-06: Hyperion's ground (Cassini, 2005): a porous moon, four tenths of it void, whose craters crushed inward rather than throwing anything out:
// pits packed edge to edge at five scales (a hundred kilometres to half of one), each deep for its width (a twentieth to a fifth of it), its floor
// flat, its walls steepening to a knife-edged rim shared with its neighbours; dark red-brown dust pooled in the floors, the walls bright ice
static void groundSponge(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.4, g.sA, 3);
    double h = base * g.mountainAmp;
    const double rel = 0.3 * massif, hf = 0.4 * hillsF;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    static const double CELL[5] = {0.18, 0.045, 0.011, 0.0028, 0.0007}, DEPTH[5] = {0.05, 0.11, 0.16, 0.2, 0.22};   // a fraction of the radius; of the cell
    double dark = 0, rim = 0;
    for (int i = 0; i < 5; i++) {
        const double cellKm = R * CELL[i], f = featFade(cellKm * 0.5, detailM);
        const bool mv = matVisible(cellKm * 0.36, detailM);   // a floor is a third of its pit across
        if (f <= 0 && !mv) continue;
        const Vec3 q = p / cellKm, wp = p + Vec3(gnoise3(q * 1.7, g.sF + 110 + i), gnoise3(q * 1.7, g.sF + 120 + i), gnoise3(q * 1.7, g.sF + 130 + i)) * (0.18 * cellKm);
        double f1, f2, u1; surfaceCells(wp, R, cellKm, g.sF + 140 + i, f1, f2, u1);
        const double r = f2 < 1e17 ? 2 * f1 / std::max(1e-9, f1 + f2) : 1.0;   // 0 at the pit's middle, 1 on its rim
        const double dep = DEPTH[i] * cellKm * 1000.0 * (0.65 + 0.7 * u1);
        h -= dep * (1 - r * r * r) * f;
        if (mv && i < 4) { dark = std::max(dark, smoothstep(0.42, 0.3, r)); rim = std::max(rim, smoothstep(0.88, 0.97, r)); }
    }
    s.height = h;
    if (dark > 0.5) { s.material = MAT_DUST; s.albedo = 0.08 + 0.04 * clampd(base + 0.5, 0, 1); }
    else { s.material = MAT_ICE; s.albedo = 0.38 + 0.06 * base + 0.1 * rim; }
    s.relief = clampd(0.5 + 0.5 * rf.slope, 0, 1);
}

// P-00: a giant's clouds: bands, storms, a great storm (gas giants, brown dwarfs)
// D-04: the dark ocean (Stevenson 1999): one sea over the whole world, its floor an abyssal plain a kilometre and a half down, kept liquid by
// its rock's own heat under hundreds of bars of hydrogen. Where its shield volcanoes break the surface, islands of black basalt (lava that met
// the sea: Hawaii's and Iceland's): shields 8-21 km round on a 70 km grid, thickest and tallest over the hot spots' swells (the archipelagos: a
// few hundredths of the world above the sea), their flanks streaked
// with young flows, the waves' benches of black sand round their shores, the summits' fumarole fields crusted pale (`EV_FUMAROLES` steams on
// them); cinder and spatter cones on their flanks
static void groundDarkOcean(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.4, g.sA, 4);
    const double swell = smoothstep(0.05, 0.45, fbm3(p / (R * 0.12), g.sB + 41, 3));   // the hot spots' swells the archipelagos stand on (unfaded: the map sees them)
    double h = -1600.0 + 500.0 * base + 1150.0 * swell;   // the abyssal plain, half a kilometre down on a swell
    bool caldera = false;
    if (scaleVisible(70.0, detailM)) h += volcanoField(p, R, 70.0, std::min(1.0, g.volcanoes * (0.5 + swell)), g.sD, 1500.0 + g.mountainAmp, caldera);
    const double isle = smoothstep(-300.0, 150.0, h);   // the islands and their shallows carry the relief
    const double rel = isle * massif * 0.7, hf = std::max(hillsF, 0.6 * isle) * isle;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf) * isle);   // the skeleton: the flanks' relief, the traits' landforms (cinders, geysers, columns, maars)
    if (scaleVisible(8.0, detailM)) h += volcanoField(p, R, 8.0, 0.35 * isle, g.sD + 5, 160.0, caldera);   // cinder and spatter cones on the flanks
    const LavaOut lf = lavaFlows(p, R, 70.0, std::min(1.0, g.volcanoes * (0.5 + swell)), g.sD, detailM);   // the shields' young flows (O6-05's tongues)
    h += lf.dh * isle;
    const double bench = smoothstep(-12.0, -2.0, h) * smoothstep(14.0, 3.0, h);   // the waves cut the shores to benches a few metres over the sea
    h += (0.6 + 0.25 * h - h) * bench * 0.75;
    s.height = h;
    if (h < 0) { s.material = MAT_WATER; s.water = 0; s.albedo = 0.05; s.biome = BIO_OCEAN; s.relief = 0; return; }
    // the summits' fumarole fields: pale crusts in patches round the calderas and the highest ground (unfaded fields)
    const double crustF = fbm3(p / 0.8, g.sG + 41, 3) + (caldera ? 0.5 : 0.0) + 0.6 * smoothstep(600.0, 1600.0, h) - 0.55;
    if (crustF > 0.12) { s.material = MAT_SULPHUR; s.albedo = 0.46 + 0.1 * crustF; }
    else if (h < 4.5 && rf.slopeM < 0.3) { s.material = MAT_SAND; s.albedo = 0.07; }   // black sand on the benches
    else if (lf.flow > 0.4 || lf.pit > 0.3) { s.material = MAT_BASALT; s.albedo = 0.05 + 0.02 * (1 - lf.flow); if (lf.pit > 0.6) s.glow = 0.25; }   // young flows, a tube's skylight
    else if (rf.slopeM > 0.45) { s.material = MAT_ROCK; s.albedo = 0.12; }   // the old flows' cliffs
    else { s.material = MAT_BASALT; s.albedo = 0.08 + 0.04 * (0.5 + 0.5 * base); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// D-04 (TR_EMBERS on a world with no lava of its own): a young interior still erupting in the dark. Fields of it 80-250 km across on 45% of the
// cells of a 1,300 km grid (`emberGrid`: the globe's spots of light), and in a field lava lakes 160-600 m across on half the cells of a 3 km grid
// (fewer toward its edge): the lake's crust-veined lava a few metres under the plain inside a spatter rampart 4-12 m high (seen across the plain),
// fresh black basalt round it to three and a half of its radii, the snow kept off the warm ground (`warm`)
static double emberLakes(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double detailM = c.detailM;
    const double field = emberFieldAt(g, p);
    if (field < 0.02) return 0;
    double warm = 0;
    if (field > 0.3 && s.material != MAT_WATER && s.material != MAT_ICE) { s.material = MAT_BASALT; s.albedo = std::min(s.albedo, 0.10); }   // the field's young lava plains (the globe sees them)
    if (!matVisible(0.08, detailM)) return 0;
    const double ground = c.reliefDone ? c.hSmooth : s.height;   // a lake's level under the smooth ground (its lava flat)
    double dh = 0; bool lava = false, rimRock = false, fresh = false, crack = false;
    visitFeatures(p, g.R, 3.0, 0.5, g.sE + 521, 0.7, [&](const FeatureHit& f) {
        if (f.u[5] > field) return;   // fewer toward the field's edge
        const double rL = 0.08 + 0.22 * f.u[3], x = f.d / rL, depth = 1.0 + 3.0 * f.u[4], rim = 4.0 + 8.0 * f.u[0];
        if (x >= 3.5) return;
        warm = std::max(warm, smoothstep(3.5, 1.2, x));
        const double fd = featFade(2 * rL, detailM);
        if (x < 1.0) { dh = std::min(dh, (ground - depth - s.height) * fd); lava = true; }
        else if (x < 1.25) dh = std::min(dh, (ground - depth - s.height + (depth + rim) * smoothstep(1.0, 1.25, x)) * fd);   // the inner wall
        else if (dh >= 0) dh = std::max(dh, rim * smoothstep(2.4, 1.25, x) * fd);   // the rim's spatter
        if (x >= 1.0 && x < 1.6) rimRock = true;
        if (x >= 1.0 && x < 3.5) fresh = true;
        if (x >= 1.3 && x < 2.6 && fbm3(p / 0.04, g.sG + 522 + (uint64_t)(f.u[1] * 64), 2) > 0.38) crack = true;   // glowing cracks in the warm crust
    });
    s.height += dh;
    if (lava) { s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0; }
    else if (crack) { s.material = MAT_LAVA; s.glow = 0.55; s.albedo = 0.9; }
    else if (rimRock) { s.material = MAT_BASALT; s.albedo = 0.06; }
    else if (fresh) { s.material = MAT_BASALT; s.albedo = 0.05; }
    return warm;
}

// D-04: the ground in the dark (a body of a drifter's system, after its kind's own): its liquid frozen at its level where it is too cold for it
// (`frozenSeas`); a young interior's lava lakes (TR_EMBERS on a world with no lava of its own); its air fallen as snow (TR_FROZEN_AIR: as deep
// on the open ground as its air was heavy, a hundred metres of snow for every five bars, the hollows filled as far as it reaches, shed from the
// steep slopes, frost on the rest; never on lava or the ground warm round it; the felisian layer's frozen cover is its air's snow already)
static void darkGround(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s;
    if (g.frozenSeas && (s.material == MAT_WATER || s.water > -1e8)) {
        if (s.material == MAT_WATER) { s.height = std::max(s.height, s.water); s.material = MAT_ICE; s.albedo = 0.62; s.biome = BIO_ICE; s.glow = 0; s.waterTint = 0; s.lifeTint = 0; }
        s.water = -1e9; s.waterKind = 0; s.shore = 1e9;
    }
    const double warm = g.hasTrait(TR_EMBERS) && !isLavaWorld(g.type) ? emberLakes(c) : 0.0;
    if (g.hasTrait(TR_FROZEN_AIR) && c.K.ground != GR_FELISIAN && s.material != MAT_LAVA && s.material != MAT_WATER) {
        const KindDef& K = c.K;
        const double airAtm = K.pressureLo + K.pressureSpan * unitFromHash(g.seed);   // what the surface would read had it kept its air
        const double open = std::min(30.0, 20.0 * airAtm), fill = clampd(c.hSmooth - s.height, 0, 600) * clampd(open / 4.0, 0, 1);
        const double shed = smoothstep(0.32, 0.55, c.slopeM);
        const double depthM = (open + 0.85 * fill) * (1 - shed) * (1 - warm);
        s.height += depthM;
        if (depthM > 0.3) { s.material = MAT_SNOW; s.albedo = 0.80 + 0.06 * clampd(depthM / 10.0, 0, 1); }
        else if (s.material != MAT_ICE && s.material != MAT_SNOW && warm < 0.5) s.albedo = std::max(s.albedo, 0.32 + 0.3 * (1 - shed));   // its frost
    }
}

static void groundGiant(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, lat = c.lat;
    double turb = fbm3(p / (R * 0.18), g.sA, 4) * 0.35;
    double turb2 = fbm3(p / (R * 0.06), g.sB, 2) * 0.05;
    double l = lat + turb + turb2;
    double bands = 0.5 + g.bandContrast * (0.28 * std::sin(l * g.bandCount) + 0.15 * std::sin(l * g.bandCount * 2.3 + 1.0) +
                   0.08 * std::sin(l * g.bandCount * 5.1 + 2.0));   // X-03: an ice giant's faint
    if (c.K.giantClass == GC_WATER) bands += 0.16;   // P-07: water's clouds the brightest (a Bond albedo of 0.8)
    double storm = giantStormCell(g, unit);   // X-03: the giant's own share
    s.height = 0;
    s.material = MAT_GAS;
    s.albedo = clampd(bands + 0.35 * storm, 0.22, 1.0);
    s.veg = storm;   // N5-03: the storm term kept apart (the map's veg channel) so the globe can let storms grow and fade with time
    if (g.gsOn) {   // X-03: the great storm (in the albedo, not the storm term: it does not fade), its bands swirled round its centre
        const double lon = std::atan2(unit.y, unit.x);
        const double ex = wrapAngle(lon - g.gsLon) * std::cos(g.gsLat) / g.gsA, ey = (lat - g.gsLat) / g.gsB, d = std::sqrt(ex * ex + ey * ey);
        if (d < 1.6) {
            const double th = std::atan2(ey, ex) + 2.8 * std::pow(std::max(0.0, 1 - d), 1.5);   // the inner parts turned further round
            const double sw = fbm3(Vec3(3.2 * d * std::cos(th), 3.2 * d * std::sin(th), 7.7), g.sE, 3);
            const double core = 1 - smoothstep(0.82, 1.0, d), collar = std::exp(-((d - 1.0) / 0.075) * ((d - 1.0) / 0.075));
            const double inner = clampd(0.5 + g.gsTone * (0.24 + 0.08 * (1 - d)) + 0.16 * sw, 0.22, 1.0);
            s.albedo = clampd(s.albedo + (inner - s.albedo) * core - 0.16 * g.gsTone * collar * (1 - core), 0.22, 1.0);
            s.veg *= smoothstep(1.0, 1.4, d);   // no small storm inside the great one
        }
    }
}
}   // namespace

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

// P-00: the kind's ground function, then the solid grounds' common end (the felisian layer keeps its own)
// Q-02: the water's trait colours at a sample (B-404's rule: unfaded fields, so every ring, the map and the globe agree). A pink lake's whole water
// and a white crust of salt on its shore; a milky sea's patches of glowing bacteria, tens of thousands of square kilometres each (Miller et
// al. 2005); a bloom's streaks drawn out by the currents (a warped field's ridges) where the sea is rich, more on the shallows; on the seas
// and, but where they are pink, the lakes
static void qualityWater(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s) {
    const bool pink = g.hasTrait(TR_PINK_LAKES), milky = g.hasTrait(TR_MILKY_SEAS), blooms = g.hasTrait(TR_BLOOMS), lights = g.hasTrait(TR_LIGHTS_IN_WATER);
    if (!pink && !milky && !blooms && !lights) return;
    const bool wet = s.water > -1e8 && (s.material == MAT_WATER || s.height < s.water);
    const bool lake = s.waterKind == 2 || s.waterKind == 3 || s.waterKind == 4, sea = s.waterKind == 0;
    const Vec3 p = unit * g.R;
    if (pink && !wet && lake && s.height < s.water + 0.5 + 0.4 * fbm3(p / 0.05, g.sG + 301, 2) && s.material != MAT_SNOW && s.material != MAT_ICE) { s.material = MAT_SALT; s.albedo = 0.78; }   // the crust on its shore
    if (!wet) return;
    if (pink && lake) s.waterTint = 1;
    if (milky && sea) s.waterTint = smoothstep(0.22, 0.42, fbm3(p / (g.R * 0.16), g.sG + 303, 2));   // a few patches of a few hundred kilometres
    if (lights && sea) {   // D-04: a dark ocean's life, round the vents its volcanoes' flanks hold (the islands' shallows, under 500 m) and in shoals some hundreds of
        // kilometres across over the deep (what the globe shows), lit in patches of a hundred metres or two
        const double shoal = smoothstep(-0.05, 0.25, fbm3(p / (g.R * 0.05), g.sG + 313, 2)), region = std::max(shoal, smoothstep(-500.0, -60.0, s.height));
        if (region > 0.01) s.waterTint = matVisible(0.1, detailM) ? region * smoothstep(0.16, 0.42, fbm3(p / 0.3, g.sG + 314, 3)) : 0.2 * smoothstep(0.4, 0.9, shoal);   // a quarter of its water lit; from afar the shoals' cores' faint sheen
    }
    if (blooms && (sea || (lake && !pink))) {
        // swirls a few hundred kilometres across (the warped field's bands, wide enough for the globe to show), their edges frayed into the
        // currents' filaments where the eye is near enough to see them
        // two scales: the bloom's regions, swirls hundreds of kilometres across (a warped field, what the globe shows: their streaks blur into
        // a tint there), and inside them the streaks drawn out by the currents, filaments some 12 km apart where the eye resolves them
        const Vec3 q = p / (g.R * 0.05);
        const Vec3 w(fbm3(q * 0.5, g.sG + 305, 2), fbm3(q * 0.5 + Vec3(5.2, 1.3, 7.7), g.sG + 306, 2), fbm3(q * 0.5 + Vec3(2.8, 9.1, 3.4), g.sG + 307, 2));
        const double region = smoothstep(0.0, 0.25, fbm3(q + w * 2.0, g.sG + 308, 3)) * (sea ? 0.65 + 0.35 * smoothstep(-400.0, -60.0, s.height) : 1.0);
        if (region > 0.01) {
            constexpr double STREAK_KM = 12.0;
            if (matVisible(STREAK_KM * 0.3, detailM)) {
                const double n2 = fbm3(p / STREAK_KM + w * 6.0, g.sG + 310, 3) + (matVisible(0.6, detailM) ? 0.25 * fbm3(p / 1.4 + w * 9.0, g.sG + 311, 2) : 0.0);
                s.lifeTint = region * (0.2 + 0.95 * smoothstep(0.7, 0.9, 1.0 - std::fabs(n2)));   // the filaments along the field's zero lines
            } else s.lifeTint = 0.8 * region;
        }
    }
}

// Q-02 (TR_GREAT_TIDES): the tidal flats. The local spring range (the trait's at the equator, the bulges' cos^2 of the latitude) decides a band
// round the sea's level: the ground within six ranges of it is pressed toward the level (h' = S + H x (0.2 + 0.8 x^4), x the height over
// the level in units of H: the intertidal zone takes the ground of two ranges either side, a flat kilometres wide on a gentle coast, the
// coast steeper above it). Between low and high water the flat is wet sand under the sea's plane (its `water` the mean level, so the surface's
// tide moves the sheet over it), cut by creeks; the sea's own samples under low water stay sea. Rivers and lakes keep their own water
double tideRangeAt(const BodyGen& g, const Vec3& unit) { return g.tideRangeM * (1 - unit.z * unit.z); }
static void tidalFlat(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s) {
    const double Rl = tideRangeAt(g, unit);
    if (Rl < 0.5) return;
    const double S = g.liquidLevel > -1e8 ? g.liquidLevel : 0.0, H = 6.0 * Rl;
    const bool inland = s.waterKind != 0 && s.water > S + 0.01;
    if (inland || s.material == MAT_LAVA || s.material == MAT_ICE || (s.water > -1e8 && std::fabs(s.water - S) > 0.01 && s.water < S)) return;
    const double x = (s.height - S) / H;
    if (std::fabs(x) >= 1) return;
    const double x4 = x * x * x * x;
    double h = S + H * x * (0.2 + 0.8 * x4);
    if (std::fabs(h - S) < 0.52 * Rl) {
        const Vec3 p = unit * g.R;
        if (matVisible(0.01, detailM)) {   // the creeks: a dendrite of channels a metre deep draining the flat toward the sea
            const double n = 1.0 - std::fabs(fbm3(p / 0.35 + Vec3(fbm3(p / 0.9, g.sG + 331, 2), 0, 0), g.sG + 330, 3));
            h -= 1.2 * std::pow(clampd(n, 0, 1), 8.0) * smoothstep(0.52 * Rl, 0.2 * Rl, std::fabs(h - S)) * lodFade(0.02, detailM);
        }
        s.material = MAT_SAND; s.albedo = 0.3 + 0.06 * fbm3(p / 0.2, g.sG + 332, 2); s.water = S; s.waterKind = 0; s.shore = 1e9; s.veg = 0;
        if (s.biome != BIO_OCEAN && h < S) s.biome = BIO_OCEAN;   // the flat under the mean level is the sea's (the rivers end at it)
    } else if (h < S && s.material != MAT_WATER) { s.material = MAT_WATER; s.water = S; s.biome = BIO_OCEAN; s.albedo = 0.36; }
    s.height = h;
}

SurfaceSample sampleSurface(const BodyGen& g, const Vec3& unit, double detailM) {
    SurfaceSample s;
    GroundCtx c(g, unit, detailM, s);
    const int ground = c.K.ground;
    if (g.tectonic && ground != GR_FELISIAN) c.tec = tectonicsAt(unit, g);
    if (ground != GR_GIANT && ground != GR_OCEAN && ground != GR_EYEBALL && ground != GR_NONE && ground != GR_FELISIAN) {
        c.massif = smoothstep(0.2, 0.6, c.rg(std::min(110.0, c.R * 0.3), g.sC + 6, 3));
        c.hillsF = smoothstep(-0.15, 0.45, c.fb(std::min(c.R * 0.06, 400.0), g.sB + 6, 3));
    }
    switch (ground) {
    case GR_FELISIAN: felisianSample(g, unit, detailM, s); break;
    case GR_CRATERED: groundCratered(c); break;
    case GR_MOLTEN: groundMolten(c); break;
    case GR_VENUSIAN: groundVenusian(c); break;
    case GR_ROCKY: groundRocky(c); break;
    case GR_THIN_AIR: groundThinAir(c); break;
    case GR_ICE: groundIce(c); break;
    case GR_QUARTZ: groundQuartz(c); break;
    case GR_OCEAN: groundOcean(c); break;
    case GR_EUROPAN: groundEuropan(c); break;
    case GR_TECTONIC: groundTectonic(c); break;
    case GR_DESERT: groundDesert(c); break;
    case GR_HYDROCARBON: groundHydrocarbon(c); break;
    case GR_BOMBARDED: groundBombarded(c); break;
    case GR_ACIDIC: groundAcidic(c); break;
    case GR_METAL: groundMetal(c); break;
    case GR_VOLCANIC: groundVolcanic(c); break;
    case GR_CARBON: groundCarbon(c); break;
    case GR_GIANT: groundGiant(c); break;
    case GR_MAGMA: groundMagma(c); break;
    case GR_CHTHONIAN: groundChthonian(c); break;
    case GR_GLASS: groundGlass(c); break;
    case GR_STEAM: groundSteam(c); break;
    case GR_SALT: groundSalt(c); break;
    case GR_LICHEN: groundLichen(c); break;
    case GR_EYEBALL: groundEyeball(c); break;
    case GR_AMMONIA: groundAmmonia(c); break;
    case GR_CHEMOTROPH: groundChemotroph(c); break;
    case GR_NITROGEN: groundNitrogen(c); break;
    case GR_CRYOVOLCANIC: groundCryovolcanic(c); break;
    case GR_DRY_ICE: groundDryIce(c); break;
    case GR_WANDERER: groundWanderer(c); break;
    case GR_BRINE: groundBrine(c); break;
    case GR_FROST: groundFrost(c); break;
    case GR_GROOVED: groundGrooved(c); break;
    case GR_TWO_FACED: groundTwoFaced(c); break;
    case GR_TIGER: groundTiger(c); break;
    case GR_PATCHWORK: groundPatchwork(c); break;
    case GR_SPONGE: groundSponge(c); break;
    case GR_DARK_OCEAN: groundDarkOcean(c); break;
    default: break;
    }
    if (ground != GR_FELISIAN) {
        const bool wetType = kindHas(g.type, KT_WET);
        const DrainApply& DA = c.DA;
        const LandOut& lo = c.lo;
        const double cliffV = c.cliffV;
        s.scree = clampd(c.talusV, 0, 1);   // O6-04
        if (cliffV > 0.4 && s.material != MAT_WATER && s.material != MAT_LAVA && s.material != MAT_ICE && s.material != MAT_SALT) {   // a riser is bare rock, whatever lies on the plain around it
            if (s.material == MAT_DUST || s.material == MAT_SAND || s.material == MAT_SULPHUR) s.material = familyRep(g.type, FAM_ROCK);
            s.albedo *= 1 - 0.15 * cliffV;
        }
        s.relief = clampd(s.relief + 0.5 * cliffV, 0, 1);
        if (DA.kind && wetType) {   // O6-03: the liquid's plane under the banks, the edge (B-322) for the mesh's shore cut
            if (s.material != MAT_WATER && s.material != MAT_LAVA && DA.zone > 0 && s.water < -1e8) s.water = DA.level;
            if (s.water > -1e8 && DA.zone > 0) { s.waterKind = DA.kind; s.shore = DA.shore; }
        }
        if (wetType && s.water < -1e8 && DA.shoreOut < 1e8) s.shore = DA.shoreOut;
        s.landform = 1000 * lo.mark + std::fabs(lo.dhSmooth) + std::fabs(lo.dhFine) + 40 * lo.flatten;
        s.landformTrait = lo.mark >= 0.5 ? lo.markTrait : 0;   // Q-01
        s.groundTint = lo.tint;   // Q-02
        s.slipFace = std::max(s.slipFace, lo.slip);   // Q-03: a dune sea's
        if (lo.life > 0 && s.material != MAT_WATER) s.lifeTint = lo.life;
        // R-305: a landform's material (rock walls, basalt sheets, salt, crystal) over the kind's own, never over lava
        if (lo.mat >= 0 && s.material != MAT_LAVA && s.material != MAT_WATER) { s.material = lo.mat; if (lo.albedo >= 0) s.albedo = lo.albedo; }
        // R-304 (TR_EXOTIC_SEAS): the ground under the liquid's level is sea
        if (g.liquidLevel > -1e8 && s.height < g.liquidLevel && s.material != MAT_LAVA) { s.material = MAT_WATER; s.water = g.liquidLevel; s.albedo = 0.22; s.glow = 0; }
    }
    if (g.tideRangeM > 0) tidalFlat(g, unit, detailM, s);   // Q-02
    qualityWater(g, unit, detailM, s);
    if (g.dark) darkGround(c);   // D-04
    if (s.material != MAT_LAVA) s.albedo = clampd(s.albedo, 0.03, 1.0);
    return s;
}

void PlanetMap::generate(const BodyGen& g) {
    valid = false;
    height.assign(W * H, 0.f);
    material.assign(W * H, 0);
    albedo.assign(W * H, 0);
    cloud.assign(W * H, 0);
    veg.assign(W * H, 0);
    const bool tints = g.hasTrait(TR_PINK_LAKES) || g.hasTrait(TR_MILKY_SEAS) || g.hasTrait(TR_BLOOMS) || g.hasTrait(TR_REEFS) || g.hasTrait(TR_LIGHTS_IN_WATER);   // Q-02; D-04
    wtint.assign(tints ? W * H : 0, 0); ltint.assign(tints ? W * H : 0, 0);
    double detail = TAU * g.R * 1000.0 / W * 0.6;
    for (int y = 0; y < H; y++) {
        double lat = (0.5 - (y + 0.5) / H) * PI;
        for (int x = 0; x < W; x++) {
            double lon = ((x + 0.5) / W) * TAU - PI;
            Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
            SurfaceSample s = sampleSurface(g, u, detail);
            int i = y * W + x;
            height[i] = (float)s.height;
            material[i] = (uint8_t)s.material;
            albedo[i] = (uint8_t)clampi((int)(s.albedo * 255 + 0.5), 0, 255);
            veg[i] = (uint8_t)clampi((int)(s.veg * 255 + 0.5), 0, 255);
            if (tints) { wtint[i] = (uint8_t)clampi((int)(s.waterTint * 255 + 0.5), 0, 255); ltint[i] = (uint8_t)clampi((int)(s.lifeTint * 255 + 0.5), 0, 255); }
            cloud[i] = (uint8_t)clampi((int)(sampleCloudPattern(g, lon, lat) * 255 + 0.5), 0, 255);
            // M9-10: a polar vortex pattern on gas giants
            if (kindHas(g.type, KT_GAS_GIANT) && std::fabs(lat) > 76 * DEG) albedo[i] = (uint8_t)clampi((int)(albedo[i] * (1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat)))), 0, 255);
        }
    }
    seed = g.seed;
    valid = true;
}

double benchFeatureField(const Vec3& p, double R, uint64_t seed) { return domeField(p, R, 2.0, 0.6, seed, 0.4, 100.0, 0.0); }   // F-01
