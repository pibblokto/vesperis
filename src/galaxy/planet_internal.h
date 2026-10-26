#pragma once
// E-07: the planet function's pieces its files share (planetmap.cpp, planet_features.cpp, planet_bodygen.cpp,
// planet_landforms.cpp, planet_felisian.cpp, planet_grounds.cpp, planet_grounds2.cpp), for those files alone: the sampling
// scale, the features' lattice and the point features, the types the files pass each other with the functions behind them
// (the fields and the relief defined in planet_features.cpp, `landformsAt` in planet_landforms.cpp, `applyDrainage` and
// `felisianSample` in planet_felisian.cpp), the finders' constants and helpers the grounds read, the grounds' context
// (`GroundCtx`) and the grounds (planet_grounds.cpp, planet_grounds2.cpp)
#include "planetmap.h"
#include "planet_features.h"
#include "core/noise.h"
#include <cmath>
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
double craterField(const Vec3& p, double R, double cellKm, double density, double depthScale, uint64_t seed, CraterMarks& m, double detailM);
double domeField(const Vec3& p, double R, double cellKm, double density, uint64_t seed, double radiusFrac, double heightM, double flatTop);
double volcanoField(const Vec3& p, double R, double cellKm, double density, uint64_t seed, double coneHeightM, bool& inCaldera);

// O6-05: lava tongues and pit chains from the cones of a `volcanoField` grid (the same cells, seed and density, so every
// cone has its flows): two to four tongues at hashed azimuths run from the cone's foot 3-8 radii downhill along the
// radial line, 15% of the radius wide at the foot and tapering, 4-10 m thick with rounded levees; one chain of collapse
// pits (skylights of a lava tube) runs out at another azimuth. Returns the height offset and sets `flow` (0..1, fresh
// basalt) and `pit` (0..1)
struct LavaOut { double dh = 0, flow = 0, pit = 0; };
LavaOut lavaFlows(const Vec3& p, double R, double cellKm, double density, uint64_t seed, double detailM);

// M9-02: plates as worley cells on the unit sphere. Returns the uplift of the plate interior
// (continental +, oceanic -), and the boundary terms: convergent chains, rifts, arcs.
struct Tectonics { double uplift = 0, chain = 0, rift = 0, ridge = 0, arc = 0, bdKm = 1e9; };
Tectonics tectonicsAt(const Vec3& unit, const BodyGen& g);
double duneField(const Vec3& p, const BodyGen& g, double lat, double amp, double detailM, double* lee = nullptr);

// O6-02: the relief spectrum. A self-affine sum of octaves from `g.reliefWl0` km down to the sampling scale: octave i
// has the wavelength wl0 / 2^i and the amplitude a0 (wl_i / wl0)^H. Each octave is shaped toward ridges by `ridge`
// (2 (1 - |n|)^2 - 1: a sharp crest at the zero crossing, wide valleys), weighted by the slope of the sum so far
// (steep ground stops gathering detail, so scarps stay smooth while crests and flats gather it: the erosion look, from
// the analytic gradient) and by its height (low ground is smoothed, the hybrid multifractal: valley floors read as
// sediment); the coordinate is folded twice by low-frequency noise so ridges curve. Every octave fades with `lodFade`
// and the weights depend only on the coarser octaves, so the map, the far ring and the ground add the same terms at
// their own scales. Returns metres and the gradient of the sum on the tangent plane (rise over run).
struct ReliefSum { double h = 0; double slope = 0; double hCoarse = 0; Vec3 gradT; double hM = 0, slopeM = 0; Vec3 gradM; };   // B-320: hCoarse = the octaves of 2 km and longer, with the floor; O6-04: gradT = the gradient on the tangent plane, rise over run, uphill; B-404: hM, slopeM, gradM = the material chain, the octaves of MAT_WL_KM and longer unfaded at every sampling scale
ReliefSum reliefSum(const Vec3& p, const Vec3& unit, const BodyGen& g, double a0, double ridge, uint64_t seed, double detailM);
// the amplitude of the relief sum from the mountain intensity `rel` (0..1) and the hill country field `hillsF` (0..1)
inline double reliefAmp(const BodyGen& g, double rel, double hillsF) { return g.reliefFlat + (g.reliefHill - g.reliefFlat) * hillsF * (1 - rel) + (g.reliefMtn - g.reliefFlat) * rel; }

// O6-04: cliff bands. On steep ground the height is remapped within bands of `cliffStep` metres: a bench (the first 35%
// of the band's rise, at a seventh of the slope), a talus ramp (the next 20%, at 1.4 times the slope: 35 degrees on a
// 1:2 hillside), the riser (30% of the rise over a tenth of the run: over 1:1, the cliff) and a rounded top; the remap is
// a monotone C1 bijection of the band, so the ground is continuous and no band is higher than the next. Half of the
// steep ground is cliffed (`cliffAmount`, by a noise of 1.2 km), and a band's run must span a few samples (1.5-4 x the
// sampling scale), so the 16 m ring and the near ring carry the cliffs and the 64 m ring shows the slope they stand on
struct CliffOut { double dh = 0, cliff = 0, talus = 0; };
CliffOut cliffBands(const Vec3& p, const BodyGen& g, double h, double slope, double zone, double detailM);
double plainRelief(const Vec3& p, const BodyGen& g, double mask, double gullies, double detailM);
// R-305: the landform library. `landformsAt` adds the traits' landforms at a point: `dhSmooth` moves the smooth ground
// and the ground alike (canyons, mesas, rifts, scarps, basins...: rivers and lakes are cut into the result), `dhFine`
// the ground alone (spires, polygons, terraces, crevasses), `flatten` melts the ground into the smooth ground by that
// fraction, `flatOffset` metres off it (playas, glaciers); `mat`/`albedo` override the material where set. Every term
// fades with the sampling scale (`lodFade`, `scaleVisible`) so the map, the far rings and the ground agree
struct LandCtx { double landMask = 1, moist = 0.15, relief0 = 0, h = 0, hS = 0, T = 15, hM = 0, slope = 0; };   // B-404: hM = the ground on the material chain's relief, slope = that chain's slope (the same in every ring)
struct LandOut { double dhSmooth = 0, dhFine = 0, flatten = 0, flatOffset = 0; int mat = -1; double albedo = -1; double glacier = 0, riftFloor = 0; double mark = 0; int markTrait = 0; double tint = 0, life = 0, slip = 0; };   // Q-02: tint, the ground trait's field (`SurfaceSample::groundTint`); life, the life trait's on the land (a petrified forest's)   // mark: 1 where the landform is at its most distinctive (the galleries); Q-01: markTrait, the trait that set it
void landformsAt(const BodyGen& g, const Vec3& p, const Vec3& unit, double detailM, const LandCtx& c, LandOut& o);

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
void applyDrainage(const BodyGen& g, const Vec3& unit, double detailM, double& h, double* hS, double slope, bool wet, bool lakesWet, double moist, DrainApply& A, double uShape = 0);
void felisianSample(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s);

static constexpr double SPRING_CELL = 0.5, SPRING_DENS = 0.55;   // P-03: the chemotroph's springs' grid (its ground and `springVents`)

// P-05: the cryovolcanic world's plume vents (`groundCryovolcanic`'s streaks start at them, `plumeVents` finds them for the surface) on a grid
// of 80 km cells over its cap, and the dry ice world's spiders (`groundDryIce`, `springJets`) on cells of 0.8 km in the band round its cap
static constexpr double PLUME_CELL = 120.0, SPIDER_CELL = 0.8, SPIDER_DENS = 0.45;
inline double plumeDensity(const BodyGen& g) { return 0.35 * g.volcanoes; }
inline double plumeStreakKm(double u) { return 45.0 + 65.0 * u; }   // a streak's length, within a cell (`visitFeatures` reaches no further)
// the edge of a cryovolcanic cap (degrees of latitude, south of it the cap) and the dry ice cap's (poleward of it) at a point
inline double cryoCapEdge(const BodyGen& g, const Vec3& unit) { return g.iceCapLat + 4.0 * gnoise3(unit * 6.0, g.sF + 51); }
inline double dryCapEdge(const BodyGen& g, const Vec3& unit) { return g.iceCapLat + 5.0 * gnoise3(unit * g.R / (g.R * 0.15), g.sF + 7); }   // `groundThinAir`'s
inline double dryFrostDeg(const BodyGen& g, bool north) { return 12.0 * smoothstep(0.6, -0.4, north ? g.season : -g.season); }   // the seasonal frost's reach past the cap (degrees): 12 in winter, none by midsummer
// P-06: the patchwork's coronae: up to three ovals 0.26-0.38 of the radius across their long axis, squared off at the corners (a superellipse
// of exponent 2.5-4), each clear of the others; in the gnomonic plane of its middle (km along `e1` and `e2`)
struct Corona { Vec3 c, e1, e2; double a = 0, b = 0, n = 2, sp = 8, amp = 400, band = 0; };
inline int coronaeOf(const BodyGen& g, Corona out[3]) {
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
inline bool coronaAt(const Corona& C, double R, const Vec3& unit, double& rho, double& th, double& inKm) {
    const double dc = dot(unit, C.c);
    if (dc < 0.3) return false;
    const double x = R * dot(unit, C.e1) / dc, y = R * dot(unit, C.e2) / dc, xa = x / C.a, yb = y / C.b;
    rho = std::pow(std::pow(std::fabs(xa), C.n) + std::pow(std::fabs(yb), C.n), 1.0 / C.n);
    th = std::atan2(yb, xa);
    inKm = (1 - rho) * std::sqrt(C.a * C.b);
    return true;
}
// the great cliff along part of the first corona's edge: its middle's angle, its half-span (radians of the angle round the corona), its height (m)
inline void cliffOf(const BodyGen& g, double& th0, double& span, double& H) {
    th0 = unitFromHash(mix64(g.seed ^ 0x9E40A1ULL)) * TAU - PI;
    span = 0.6 + 0.3 * unitFromHash(mix64(g.seed ^ 0x9E40A2ULL));
    H = 15000.0 + 5000.0 * unitFromHash(mix64(g.seed ^ 0x9E40A3ULL));
}
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

// the kinds' grounds (planet_grounds.cpp, planet_grounds2.cpp), `sampleSurface` calling its kind's; the drifters' dark ground over any
void groundCratered(GroundCtx& c);
void groundMolten(GroundCtx& c);
void groundVenusian(GroundCtx& c);
void groundRocky(GroundCtx& c);
void groundThinAir(GroundCtx& c);
void groundIce(GroundCtx& c);
void groundQuartz(GroundCtx& c);
void groundOcean(GroundCtx& c);
void groundEuropan(GroundCtx& c);
void groundTectonic(GroundCtx& c);
void groundDesert(GroundCtx& c);
void groundHydrocarbon(GroundCtx& c);
void groundBombarded(GroundCtx& c);
void groundAcidic(GroundCtx& c);
void groundMetal(GroundCtx& c);
void groundVolcanic(GroundCtx& c);
void groundCarbon(GroundCtx& c);
void groundMagma(GroundCtx& c);
void groundChthonian(GroundCtx& c);
void groundGlass(GroundCtx& c);
void groundSteam(GroundCtx& c);
void groundSalt(GroundCtx& c);
void groundLichen(GroundCtx& c);
void groundEyeball(GroundCtx& c);
void groundAmmonia(GroundCtx& c);
void groundChemotroph(GroundCtx& c);
void groundNitrogen(GroundCtx& c);
void groundCryovolcanic(GroundCtx& c);
void groundDryIce(GroundCtx& c);
void groundWanderer(GroundCtx& c);
void groundBrine(GroundCtx& c);
void groundFrost(GroundCtx& c);
void groundGrooved(GroundCtx& c);
void groundTwoFaced(GroundCtx& c);
void groundTiger(GroundCtx& c);
void groundPatchwork(GroundCtx& c);
void groundSponge(GroundCtx& c);
void groundDarkOcean(GroundCtx& c);
void darkGround(GroundCtx& c);
void groundGiant(GroundCtx& c);
