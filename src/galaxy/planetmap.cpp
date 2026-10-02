#include "planetmap.h"
#include "drainage.h"
#include "core/noise.h"
#include <cmath>

const char* MATERIAL_NAMES[MAT_COUNT] = {"ROCK", "SAND", "GRASS", "FOREST", "SNOW", "WATER", "LAVA", "ICE", "CLOUD",
                                         "QUARTZ", "BASALT", "DUST", "GAS", "IRON", "SULPHUR", "GRAPHITE", "SALT", "GLASS"};
const char* TRAIT_NAMES[TR_COUNT] = {"", "CANYON LANDS", "BADLANDS", "KARST TOWERS", "GREAT RIFT", "ESCARPMENTS", "GLACIATED", "INSELBERGS", "CINDER FIELDS", "CHAOS TERRAIN", "PATTERNED GROUND",
                                     "YARDANGS", "DUNE SEAS", "SALT FLATS", "TRAP TERRACES", "GREAT BASIN", "CORONAE", "SPIRE FIELDS", "GEYSER BASINS",
                                     "ARCHIPELAGO", "PANGAEA", "LAKE COUNTRY", "SNOWBALL", "EXOTIC SEAS", "STORM WORLD", "HAZE",
                                     "GIANT FLORA", "LUMINOUS FLORA", "DEAD FORESTS", "RED SOILS", "BLACK SANDS", "CHALK LANDS", "GLASSED"};
const char* traitPhrase(int t) {
    switch (t) {
        case TR_CANYONS: return ", cut by canyons"; case TR_MESAS: return ", its dry country carved into mesas and buttes"; case TR_KARST: return ", stone towers rising from its wet lowlands";
        case TR_RIFT: return ", split by a great rift valley"; case TR_ESCARPMENTS: return ", stepped by long escarpments"; case TR_GLACIAL: return ", scoured by glaciers, its cold coasts fjords";
        case TR_INSELBERGS: return ", lone mountains standing on its plains"; case TR_CINDER_FIELD: return ", fields of cinder cones and fresh lava"; case TR_CHAOS: return ", its crust broken into tilted blocks";
        case TR_POLYGONS: return ", its cold ground patterned in polygons"; case TR_YARDANGS: return ", ridges carved by the wind"; case TR_ERG: return ", seas of giant dunes";
        case TR_SALT_FLATS: return ", white salt flats in its basins"; case TR_TRAPS: return ", stepped terraces of old lava"; case TR_GREAT_BASIN: return ", one vast impact basin";
        case TR_CORONAE: return ", ringed by coronae"; case TR_SPIRES: return ", fields of spires"; case TR_GEYSERS: return ", geyser basins steaming";
        case TR_ARCHIPELAGO: return ", an archipelago of islands"; case TR_PANGAEA: return ", one great continent"; case TR_LAKELAND: return ", lakes everywhere";
        case TR_SNOWBALL: return ", frozen down to the middle latitudes"; case TR_EXOTIC_SEAS: return ", seas of something that is not water"; case TR_STORMS: return ", storms that never end";
        case TR_HAZE: return ", under a thick haze"; case TR_GIANT_FLORA: return ", flora of giant size"; case TR_LUMINOUS_FLORA: return ", flora that glows at night";
        case TR_DEAD_FOREST: return ", its forests dead and grey"; case TR_RED_SOIL: return ", red soils"; case TR_BLACK_SAND: return ", black sands"; case TR_CHALK: return ", chalk-white ground";
        case TR_GLASSED: return ", its plains fused to glass by the blast that made its star";   // S-03
        default: return "";
    }
}
bool traitEligible(int type, int t) {
    switch (t) {
        case TR_CANYONS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_VENUSIAN || type == PT_METAL || type == PT_ICY || type == PT_DESERT || type == PT_ACIDIC || type == PT_TECTONIC;
        case TR_MESAS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_QUARTZ || type == PT_CARBON || type == PT_DESERT || type == PT_ACIDIC;
        case TR_KARST: return type == PT_FELISIAN || type == PT_ACIDIC;
        case TR_RIFT: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_VENUSIAN || type == PT_MOLTEN || type == PT_ICY || type == PT_VOLCANIC || type == PT_DESERT || type == PT_ACIDIC;
        case TR_ESCARPMENTS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_CRATERED || type == PT_VENUSIAN || type == PT_ICY || type == PT_METAL || type == PT_CARBON || type == PT_DESERT || type == PT_TECTONIC || type == PT_ACIDIC || type == PT_BOMBARDED;
        case TR_GLACIAL: return type == PT_FELISIAN || type == PT_THINATMO;
        case TR_INSELBERGS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_QUARTZ || type == PT_DESERT;
        case TR_CINDER_FIELD: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_CRATERED || type == PT_VENUSIAN || type == PT_MOLTEN || type == PT_VOLCANIC || type == PT_TECTONIC || type == PT_DESERT;
        case TR_CHAOS: return type == PT_ICY || type == PT_ROCKY || type == PT_METAL || type == PT_COMET || type == PT_EUROPAN || type == PT_BOMBARDED;
        case TR_POLYGONS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ICY || type == PT_COMET || type == PT_EUROPAN || type == PT_HYDROCARBON;
        case TR_YARDANGS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_DESERT || type == PT_HYDROCARBON;
        case TR_ERG: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_DESERT || type == PT_HYDROCARBON;
        case TR_SALT_FLATS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_DESERT || type == PT_ACIDIC;
        case TR_TRAPS: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_CRATERED || type == PT_VENUSIAN || type == PT_MOLTEN || type == PT_METAL || type == PT_VOLCANIC || type == PT_QUARTZ || type == PT_TECTONIC || type == PT_DESERT;
        case TR_GREAT_BASIN: return type == PT_THINATMO || type == PT_ROCKY || type == PT_CRATERED || type == PT_MOLTEN || type == PT_ICY || type == PT_METAL || type == PT_CARBON || type == PT_DESERT || type == PT_BOMBARDED || type == PT_HYDROCARBON;
        case TR_CORONAE: return type == PT_VENUSIAN || type == PT_MOLTEN || type == PT_VOLCANIC || type == PT_TECTONIC || type == PT_ACIDIC;
        case TR_SPIRES: return type == PT_ICY || type == PT_COMET || type == PT_QUARTZ || type == PT_CARBON || type == PT_EUROPAN || type == PT_ACIDIC;
        case TR_GEYSERS: return type == PT_FELISIAN || type == PT_ICY || type == PT_VOLCANIC || type == PT_TECTONIC || type == PT_ACIDIC;
        case TR_ARCHIPELAGO: return type == PT_FELISIAN || type == PT_OCEAN;
        case TR_PANGAEA: case TR_LAKELAND: case TR_GIANT_FLORA: case TR_LUMINOUS_FLORA: case TR_DEAD_FOREST: return type == PT_FELISIAN;
        case TR_SNOWBALL: return type == PT_FELISIAN || type == PT_OCEAN;
        case TR_EXOTIC_SEAS: return type == PT_THINATMO || type == PT_CARBON || type == PT_ICY;
        case TR_STORMS: return type == PT_FELISIAN || type == PT_OCEAN || type == PT_VENUSIAN || type == PT_THINATMO || type == PT_DESERT || type == PT_HYDROCARBON || type == PT_ACIDIC;
        case TR_HAZE: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_QUARTZ || type == PT_OCEAN || type == PT_DESERT || type == PT_ACIDIC;
        case TR_RED_SOIL: case TR_BLACK_SAND: case TR_CHALK: return type == PT_FELISIAN || type == PT_THINATMO || type == PT_ROCKY || type == PT_DESERT || type == PT_TECTONIC || type == PT_ACIDIC;
        case TR_GLASSED: return false;   // S-03: never drawn by the hash; `StarSystem::generate` marks a neutron star's outer survivors and `BodyGen::make` reads the mark
        default: return false;
    }
}
bool typeSky(const BodyGen& g, RGB& zen, RGB& hor) {
    if (!PLANET_TYPES[g.type].atmosphere || g.type == PT_GASGIANT || g.type == PT_SUBSTELLAR) return false;
    RGB white(1, 1, 1);
    zen = g.skyTint * 0.75f; hor = lerp(g.skyTint, white, 0.55f);
    switch (g.type) {
        case PT_THINATMO: zen = g.skyTint * 0.35f; hor = g.skyTint * 0.75f; break;
        case PT_VENUSIAN: zen = RGB(0.85f, 0.7f, 0.45f); hor = RGB(0.95f, 0.85f, 0.6f); break;
        case PT_QUARTZ: zen = RGB(0.9f, 0.88f, 0.92f); hor = RGB(1, 0.98f, 1); break;
        case PT_DESERT: zen = RGB(0.62f, 0.55f, 0.45f); hor = RGB(0.90f, 0.78f, 0.58f); break;        // R-307: a dusty ochre sky
        case PT_HYDROCARBON: zen = RGB(0.70f, 0.42f, 0.14f); hor = RGB(0.92f, 0.62f, 0.28f); break;   // the orange haze deck
        case PT_ACIDIC: zen = RGB(0.62f, 0.66f, 0.32f); hor = RGB(0.88f, 0.88f, 0.50f); break;        // yellow-green murk
        case PT_TECTONIC: zen = RGB(0.40f, 0.32f, 0.24f); hor = RGB(0.80f, 0.60f, 0.38f); break;      // thin, sulphurous
        default: break;
    }
    return true;
}
std::string traitList(const BodyGen& g) {
    std::string s;
    for (int k = 0; k < 3; k++) if (g.traits[k]) { if (!s.empty()) s += ", "; s += TRAIT_NAMES[g.traits[k]]; }
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
    for (int dz = -1; dz <= 1; dz++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                uint64_t h = hash3i(cx + dx, cy + dy, cz + dz, seed);
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

// Crater field (M9-06): returns height (metres); accumulates rim brightness, floor darkening,
// bright ray systems of young craters, mare flooding of old large basins and the secondary
// field around large craters (which raises the density of the small crater pass).
struct CraterMarks { double rimBright = 0, floorDark = 0, rays = 0, flooded = 0, secondary = 0; };
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

// M9-08: dune fields from the prevailing wind; returns a height offset in metres.
double duneField(const Vec3& p, const BodyGen& g, double lat, double amp, double detailM) {
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
    double v;
    if (g.duneStyle == 0) v = slip((along + bend) / TAU);                                                    // transverse
    else if (g.duneStyle == 1) v = std::pow(slip((along + bend + 0.6 * std::sin(across * 0.5)) / TAU), 1.7);   // barchan-like crescents
    else v = (std::sin(along + bend) + std::sin(along * 0.5 + across * 0.87 + bend) + std::sin(along * 0.5 - across * 0.87 - bend)) / 3.0 * 0.5 + 0.5;   // star dunes
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
        case MAT_WATER: case MAT_GLASS: return FAM_WATER;   // S-03: the glass takes the water family's bank (bank 2, the secondary) in the glass's colour; a glassed world has no sea
        case MAT_FOREST: return FAM_FOREST;
        case MAT_GRASS: return FAM_GRASS;
        case MAT_SAND: case MAT_DUST: case MAT_SULPHUR: case MAT_QUARTZ: return FAM_SAND;
        case MAT_SNOW: case MAT_ICE: case MAT_CLOUD: case MAT_SALT: return FAM_SNOW;
        default: return FAM_ROCK;   // rock, basalt, lava, gas, metal, graphite
    }
}

int familyRep(int type, int family) {
    switch (family) {
        case FAM_WATER: return MAT_WATER;
        case FAM_FOREST: return MAT_FOREST;
        case FAM_GRASS: return MAT_GRASS;
        case FAM_SAND:
            if (type == PT_FELISIAN || type == PT_DESERT || type == PT_HYDROCARBON) return MAT_SAND;
            if (type == PT_VOLCANIC || type == PT_ACIDIC) return MAT_SULPHUR;
            if (type == PT_QUARTZ || type == PT_CARBON) return MAT_QUARTZ;
            return MAT_DUST;
        case FAM_SNOW: return type == PT_FELISIAN ? MAT_SNOW : MAT_ICE;
        default:
            if (type == PT_MOLTEN || type == PT_VENUSIAN || type == PT_VOLCANIC || type == PT_CRATERED || type == PT_TECTONIC) return MAT_BASALT;
            if (type == PT_METAL) return MAT_METAL;
            if (type == PT_CARBON) return MAT_GRAPHITE;
            if (type == PT_GASGIANT || type == PT_SUBSTELLAR) return MAT_GAS;
            return MAT_ROCK;
    }
}

// N1-01 (B-202): one palette per body. The base hue is the body's colour seen from afar (typeColor); each
// material mixes its own tint with it, so a red thin-atmosphere world is red underfoot and a felisian
// world is green forest, straw grass, tan sand and white snow in every view.
static void materialPalette(BodyGen& g) {
    RGB base = g.color;
    bool atmo = PLANET_TYPES[g.type].atmosphere;
    RGB rockGrey(0.50f, 0.46f, 0.42f), basaltDark(0.24f, 0.22f, 0.21f), dustWarm(0.62f, 0.56f, 0.48f), sandTan(0.82f, 0.72f, 0.50f);
    RGB snowWhite(0.92f, 0.93f, 0.97f), iceBlue(0.72f, 0.80f, 0.92f), water(0.10f, 0.28f, 0.62f), lava(1.0f, 0.5f, 0.1f);
    RGB quartzPale(0.90f, 0.86f, 0.90f), metalGrey(0.45f, 0.43f, 0.42f), sulphurY(0.88f, 0.78f, 0.30f), graphite(0.17f, 0.17f, 0.18f), cloudW(0.95f, 0.95f, 0.97f);
    RGB* m = g.matColor;
    RGB veg = (g.vegColor.r + g.vegColor.g + g.vegColor.b > 0.05f) ? g.vegColor : RGB(0.25f, 0.5f, 0.15f);   // worlds without flora still own a colour
    m[MAT_ROCK] = lerp(rockGrey, base, 0.5f);
    m[MAT_BASALT] = lerp(basaltDark, base, 0.25f);
    m[MAT_DUST] = lerp(dustWarm, base, 0.45f);
    m[MAT_SAND] = lerp(sandTan, base, 0.3f);
    m[MAT_GRASS] = lerp(veg, RGB(0.72f, 0.66f, 0.32f), 0.4f);   // straw in the green
    m[MAT_FOREST] = veg * 0.85f;
    m[MAT_SNOW] = atmo ? lerp(snowWhite, g.skyTint, 0.1f) : snowWhite;
    m[MAT_ICE] = atmo ? lerp(iceBlue, g.skyTint, 0.15f) : iceBlue;
    m[MAT_WATER] = atmo ? lerp(water, g.skyTint * 0.55f, 0.25f) : water;
    m[MAT_LAVA] = lava;
    m[MAT_CLOUD] = cloudW;
    m[MAT_QUARTZ] = lerp(quartzPale, base, 0.3f);
    m[MAT_METAL] = lerp(metalGrey, base, 0.5f);
    m[MAT_SULPHUR] = lerp(sulphurY, base, 0.4f);
    m[MAT_GRAPHITE] = lerp(graphite, base, 0.3f);
    m[MAT_GAS] = base;
    m[MAT_SALT] = lerp(RGB(0.93f, 0.92f, 0.87f), base, 0.08f);   // R-305: playas and sinter
    m[MAT_GLASS] = RGB(0.50f, 0.66f, 0.52f);   // S-03: pale green glass (the glassed worlds set their own below)
    switch (g.type) {
        case PT_FELISIAN:   // the base hue is the blend seen from afar; the ground is rock and sand, not teal
            m[MAT_ROCK] = lerp(RGB(0.52f, 0.46f, 0.38f), base, 0.2f);
            m[MAT_SAND] = lerp(sandTan, base, 0.15f);
            break;
        case PT_THINATMO:   // exotic colours are the point of the type
            m[MAT_ROCK] = lerp(rockGrey, base, 0.65f); m[MAT_DUST] = lerp(dustWarm, base, 0.6f); m[MAT_BASALT] = lerp(basaltDark, base, 0.4f);
            m[MAT_ICE] = lerp(RGB(0.85f, 0.88f, 0.95f), g.skyTint, 0.15f);
            break;
        case PT_ICY: m[MAT_ICE] = lerp(iceBlue, base, 0.6f); m[MAT_ROCK] = RGB(0.42f, 0.42f, 0.50f); break;   // the dark rock of the cracks
        case PT_OCEAN: m[MAT_WATER] = lerp(g.color2, base, 0.3f); m[MAT_ICE] = RGB(0.85f, 0.9f, 0.98f); break;
        case PT_VENUSIAN: m[MAT_CLOUD] = base; m[MAT_BASALT] = lerp(RGB(0.42f, 0.34f, 0.24f), base, 0.2f); break;   // the ground under the deck is warm and dim
        case PT_MOLTEN: m[MAT_BASALT] = lerp(RGB(0.30f, 0.26f, 0.24f), base, 0.15f); break;
        case PT_VOLCANIC: m[MAT_SULPHUR] = lerp(sulphurY, base, 0.5f); m[MAT_BASALT] = RGB(0.30f, 0.27f, 0.25f); break;
        case PT_CARBON: m[MAT_QUARTZ] = RGB(0.9f, 0.95f, 1.0f); m[MAT_GRAPHITE] = lerp(graphite, base, 0.5f); break;   // diamond crusts
        case PT_QUARTZ: m[MAT_QUARTZ] = lerp(quartzPale, base, 0.45f); break;
        case PT_METAL: m[MAT_METAL] = lerp(metalGrey, base, 0.6f); break;
        case PT_CRATERED: m[MAT_DUST] = lerp(dustWarm, base, 0.5f); m[MAT_BASALT] = lerp(RGB(0.30f, 0.29f, 0.28f), base, 0.3f); break;
        case PT_COMET: m[MAT_ICE] = lerp(iceBlue, base, 0.5f); m[MAT_ROCK] = RGB(0.30f, 0.30f, 0.34f); break;
        // R-307
        case PT_EUROPAN: m[MAT_ICE] = lerp(RGB(0.90f, 0.88f, 0.82f), base, 0.15f); m[MAT_DUST] = RGB(0.55f, 0.32f, 0.20f); m[MAT_ROCK] = RGB(0.45f, 0.40f, 0.38f); break;   // the lineae's stain is the dust
        case PT_TECTONIC: m[MAT_BASALT] = RGB(0.24f, 0.21f, 0.19f); m[MAT_DUST] = RGB(0.50f, 0.46f, 0.42f); m[MAT_SULPHUR] = lerp(sulphurY, base, 0.25f); m[MAT_ROCK] = lerp(RGB(0.42f, 0.36f, 0.30f), base, 0.3f); break;   // ash plains pale over the dark basalt
        case PT_DESERT: m[MAT_DUST] = lerp(RGB(0.78f, 0.60f, 0.38f), base, 0.4f); m[MAT_SAND] = lerp(RGB(0.90f, 0.75f, 0.45f), base, 0.25f); m[MAT_ROCK] = lerp(RGB(0.55f, 0.42f, 0.30f), base, 0.3f); break;
        case PT_HYDROCARBON: m[MAT_ICE] = RGB(0.62f, 0.50f, 0.32f); m[MAT_SAND] = RGB(0.36f, 0.24f, 0.12f); m[MAT_WATER] = RGB(0.14f, 0.10f, 0.05f); m[MAT_CLOUD] = RGB(0.85f, 0.55f, 0.22f); m[MAT_ROCK] = RGB(0.5f, 0.42f, 0.3f); break;   // methane (near black), tholin, the haze deck
        case PT_BOMBARDED: m[MAT_DUST] = lerp(RGB(0.46f, 0.45f, 0.44f), base, 0.3f); m[MAT_ROCK] = RGB(0.44f, 0.42f, 0.40f); break;
        case PT_ACIDIC: m[MAT_ROCK] = lerp(RGB(0.80f, 0.78f, 0.60f), base, 0.25f); m[MAT_SULPHUR] = lerp(sulphurY, RGB(0.9f, 0.85f, 0.5f), 0.5f); m[MAT_WATER] = RGB(0.26f, 0.32f, 0.10f); break;   // bleached rock, dilute acid (dark, so the map and the ground agree)
        default: break;
    }
    // R-304: the colour traits tint the ground family, the exotic seas colour their liquid
    if (g.hasTrait(TR_RED_SOIL)) { RGB red(0.62f, 0.30f, 0.16f); m[MAT_ROCK] = lerp(m[MAT_ROCK], red, 0.5f); m[MAT_SAND] = lerp(m[MAT_SAND], red, 0.55f); m[MAT_DUST] = lerp(m[MAT_DUST], red, 0.6f); m[MAT_BASALT] = lerp(m[MAT_BASALT], red, 0.25f); }
    if (g.hasTrait(TR_BLACK_SAND)) { RGB blk(0.13f, 0.12f, 0.13f); m[MAT_SAND] = lerp(m[MAT_SAND], blk, 0.8f); m[MAT_DUST] = lerp(m[MAT_DUST], blk, 0.7f); m[MAT_ROCK] = lerp(m[MAT_ROCK], blk, 0.35f); }
    if (g.hasTrait(TR_CHALK)) { RGB chalk(0.9f, 0.88f, 0.8f); m[MAT_ROCK] = lerp(m[MAT_ROCK], chalk, 0.6f); m[MAT_SAND] = lerp(m[MAT_SAND], chalk, 0.5f); m[MAT_DUST] = lerp(m[MAT_DUST], chalk, 0.6f); }
    if (g.hasTrait(TR_EXOTIC_SEAS)) m[MAT_WATER] = g.type == PT_CARBON ? RGB(0.07f, 0.06f, 0.06f) : (g.type == PT_ICY ? RGB(0.32f, 0.28f, 0.42f) : RGB(0.42f, 0.28f, 0.08f));   // tar, brine, methane
    if (g.hasTrait(TR_GLASSED)) {   // S-03: the sheets of pale green glass take the water family's colour (the globe, the maps and bank 2 read it); the ground round them is scorched
        RGB glass = lerp(RGB(0.50f, 0.66f, 0.52f), base, 0.2f), scorched(0.26f, 0.23f, 0.21f);
        m[MAT_GLASS] = glass; m[MAT_WATER] = glass;
        m[MAT_ROCK] = lerp(m[MAT_ROCK], scorched, 0.45f); m[MAT_DUST] = lerp(m[MAT_DUST], scorched, 0.45f); m[MAT_SAND] = lerp(m[MAT_SAND], scorched, 0.4f);
        m[MAT_BASALT] = lerp(m[MAT_BASALT], scorched, 0.3f); m[MAT_METAL] = lerp(m[MAT_METAL], scorched, 0.3f); m[MAT_GRAPHITE] = lerp(m[MAT_GRAPHITE], scorched, 0.2f);
    }
}

BodyGen BodyGen::make(const Body& b, double season) {
    BodyGen g;
    g.type = b.type;
    g.seed = b.seed;
    g.R = b.radiusKm;
    g.season = season;
    // B-321: a living world's air keeps it 33 degrees over its equilibrium temperature (Earth's greenhouse), so an Earth-like
    // orbit gives an Earth-like climate; without it two thirds of the felisian land was ice sheet (`vesperis_test encounters`)
    g.tempBias = (b.tempK + (b.type == PT_FELISIAN ? 33.0 : 0.0) - 288.0) * 0.3;
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
    g.tectonic = b.type == PT_FELISIAN || b.type == PT_THINATMO || b.type == PT_ROCKY || b.type == PT_VENUSIAN || b.type == PT_TECTONIC || b.type == PT_DESERT || b.type == PT_ACIDIC;
    g.riverDensity = 0.4 + 0.5 * rng.uni();
    g.lakeDensity = 0.35 + 0.45 * unitFromHash(mix64(b.seed ^ 0x1A4EULL));   // B-320: hashed (the rng stream stays)
    g.floraFamily = (int)(unitFromHash(mix64(b.seed ^ 0xF10A)) * 10);    // hashed, not drawn: the rng stream stays as in GEN_VERSION 2; B-315: ten silhouettes
    g.floraFamily2 = (int)(unitFromHash(mix64(b.seed ^ 0xF10B)) * 10);
    if (g.floraFamily2 == g.floraFamily) g.floraFamily2 = (g.floraFamily + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10D)) * 9)) % 10;
    g.locked = b.locked; g.lockedDir = b.lockedDir;
    g.selfGlow = b.type == PT_SUBSTELLAR ? clampd(b.luminosity / 0.003, 0.55, 1.0) : 0.0;
    switch (b.type) {
        case PT_MOLTEN:
            g.lavaLevel = -100 - 500 * rng.uni();
            g.volcanoes = 0.2 + 0.5 * rng.uni();
            g.mountainAmp = 1200 + 1500 * rng.uni();
            g.hillAmp = 150 + 200 * rng.uni();
            g.color2 = RGB(1.0f, 0.55f, 0.1f);
            break;
        case PT_CRATERED:
            g.mountainAmp = 500 + 900 * rng.uni();
            g.hillAmp = 80 + 120 * rng.uni();
            g.craterDensity = 0.35 + 0.4 * rng.uni();
            g.color2 = RGB(0.3f, 0.3f, 0.3f);
            break;
        case PT_VENUSIAN:
            g.mountainAmp = 1000 + 1200 * rng.uni();
            g.hillAmp = 200 + 200 * rng.uni();
            g.cloudCover = 1.0;
            g.color2 = RGB(0.9f, 0.8f, 0.6f);
            g.skyTint = RGB(0.95f, 0.85f, 0.55f);
            break;
        case PT_FELISIAN:
            g.seaLevel = -0.35 + 0.55 * rng.uni();            // more water than land on average
            g.mountainAmp = 1500 + 2500 * rng.uni();   // B-321: 2500-5500 before, on a base half as high now
            g.hillAmp = 150 + 250 * rng.uni();
            g.iceCapLat = 62 + 18 * rng.uni();
            g.snowLine = 2500 + 2500 * rng.uni();
            g.moistureBias = rng.sym(0.25);
            g.cloudCover = 0.25 + 0.4 * rng.uni();
            g.color2 = RGB(0.2f, 0.45f, 0.75f);
            g.skyTint = RGB(0.45f + 0.15f * (float)rng.uni(), 0.6f + 0.15f * (float)rng.uni(), 0.9f + 0.1f * (float)rng.uni());
            {
                double k = rng.uni();
                if (k < 0.55) g.vegColor = RGB(0.2f + 0.15f * (float)rng.uni(), 0.5f + 0.2f * (float)rng.uni(), 0.12f);
                else if (k < 0.7) g.vegColor = RGB(0.55f, 0.15f, 0.2f);          // red-purple flora
                else if (k < 0.82) g.vegColor = RGB(0.3f, 0.25f, 0.65f);         // blue-violet
                else if (k < 0.92) g.vegColor = RGB(0.7f, 0.6f, 0.15f);          // yellow-ochre
                else g.vegColor = RGB(0.15f, 0.55f, 0.6f);                       // teal
            }
            break;
        case PT_ROCKY:
            g.mountainAmp = 1500 + 2000 * rng.uni();
            g.hillAmp = 150 + 200 * rng.uni();
            g.boulderDensity = 0.2 + 0.6 * rng.uni();
            g.craterDensity = 0.05 + 0.1 * rng.uni();
            g.color2 = RGB(0.35f, 0.3f, 0.28f);
            break;
        case PT_THINATMO:
            g.mountainAmp = 1500 + 2500 * rng.uni();
            g.hillAmp = 120 + 200 * rng.uni();
            g.craterDensity = 0.15 + 0.25 * rng.uni();
            g.duneAmp = 4 + 14 * rng.uni();
            g.iceCapLat = 70 + 15 * rng.uni();
            g.cloudCover = 0.08 + 0.2 * rng.uni();
            g.color2 = RGB(0.92f, 0.92f, 0.98f);
            {
                RGB c = b.color;
                g.skyTint = RGB(0.6f * c.b + 0.4f * c.r, 0.5f * c.g + 0.3f, 0.7f * c.r + 0.3f * c.b);
            }
            break;
        case PT_GASGIANT:
            g.bandCount = 5 + 9 * rng.uni();
            g.color2 = RGB(b.color.r * 0.7f, b.color.g * 0.7f, b.color.b * 0.8f);
            break;
        case PT_SUBSTELLAR:   // M5-02: fewer, broader bands
            g.bandCount = 3 + 5 * rng.uni();
            g.color2 = RGB(b.color.r * 0.5f, b.color.g * 0.4f, b.color.b * 0.4f);
            break;
        case PT_COMET:        // M5-07: a battered icy hill
            g.mountainAmp = 150 + 250 * rng.uni();
            g.hillAmp = 30 + 40 * rng.uni();
            g.fractureWidth = 0.04 + 0.05 * rng.uni();
            g.craterDensity = 0.3 + 0.3 * rng.uni();
            g.color2 = RGB(0.45f, 0.5f, 0.6f);
            break;
        case PT_COMPANION:
            break;
        case PT_ICY:
            g.mountainAmp = 200 + 400 * rng.uni();
            g.hillAmp = 40 + 60 * rng.uni();
            g.fractureWidth = 0.03 + 0.05 * rng.uni();
            g.craterDensity = 0.08 + 0.15 * rng.uni();
            g.color2 = RGB(0.45f, 0.5f, 0.6f);
            break;
        case PT_QUARTZ:
            g.mountainAmp = 700 + 700 * rng.uni();
            g.hillAmp = 250 + 250 * rng.uni();
            g.craterDensity = 0.05 + 0.08 * rng.uni();
            g.cloudCover = 0.12 + 0.2 * rng.uni();
            g.color2 = RGB(1.0f, 0.95f, 0.98f);
            g.skyTint = RGB(0.98f, 0.95f, 0.98f);
            break;
        case PT_OCEAN:
            g.iceCapLat = 40 + 25 * rng.uni();
            g.cloudCover = 0.4 + 0.4 * rng.uni();
            g.color2 = RGB(0.15f, 0.35f, 0.7f);
            g.skyTint = RGB(0.5f, 0.65f, 0.9f);
            g.hillAmp = 2 + 3 * rng.uni();
            break;
        // R-307
        case PT_EUROPAN:
            g.mountainAmp = 80 + 60 * rng.uni();
            g.hillAmp = 15 + 15 * rng.uni();
            g.craterDensity = 0.03 + 0.05 * rng.uni();
            g.color2 = RGB(0.55f, 0.32f, 0.2f);
            break;
        case PT_TECTONIC:
            g.mountainAmp = 1500 + 1500 * rng.uni();
            g.hillAmp = 150 + 150 * rng.uni();
            g.volcanoes = 0.3 + 0.5 * rng.uni();
            g.craterDensity = 0.05 + 0.05 * rng.uni();
            g.cloudCover = 0.1 + 0.2 * rng.uni();
            g.color2 = RGB(1.0f, 0.5f, 0.1f);
            g.skyTint = RGB(0.85f, 0.7f, 0.45f);
            break;
        case PT_DESERT:
            g.mountainAmp = 1200 + 1800 * rng.uni();
            g.hillAmp = 120 + 180 * rng.uni();
            g.craterDensity = 0.06 + 0.1 * rng.uni();
            g.duneAmp = 10 + 22 * rng.uni();
            g.cloudCover = 0.05 + 0.15 * rng.uni();
            g.color2 = RGB(0.95f, 0.85f, 0.6f);
            g.skyTint = RGB(0.85f, 0.72f, 0.5f);
            break;
        case PT_HYDROCARBON:
            g.mountainAmp = 400 + 400 * rng.uni();
            g.hillAmp = 60 + 60 * rng.uni();
            g.craterDensity = 0.04 + 0.05 * rng.uni();
            g.duneAmp = 15 + 25 * rng.uni();
            g.cloudCover = 0.3 + 0.3 * rng.uni();
            g.liquidLevel = 0;   // the methane sea at the reference level
            g.color2 = RGB(0.3f, 0.18f, 0.06f);
            g.skyTint = RGB(0.85f, 0.55f, 0.2f);
            break;
        case PT_BOMBARDED:
            g.mountainAmp = 600 + 900 * rng.uni();
            g.hillAmp = 80 + 120 * rng.uni();
            g.craterDensity = 0.8 + 0.2 * rng.uni();
            g.craterDepth = 1.0 + 0.4 * rng.uni();
            g.color2 = RGB(0.3f, 0.3f, 0.3f);
            break;
        case PT_ACIDIC:
            g.mountainAmp = 900 + 1200 * rng.uni();
            g.hillAmp = 150 + 200 * rng.uni();
            g.craterDensity = 0.04 + 0.05 * rng.uni();
            g.cloudCover = 0.5 + 0.35 * rng.uni();
            g.liquidLevel = 0;   // the acid sea at the reference level
            g.color2 = RGB(0.55f, 0.6f, 0.2f);
            g.skyTint = RGB(0.8f, 0.85f, 0.45f);
            break;
        case PT_METAL:
            g.mountainAmp = 600 + 900 * rng.uni();
            g.hillAmp = 80 + 120 * rng.uni();
            g.craterDensity = 0.3 + 0.4 * rng.uni();
            g.craterDepth = 1.0 + 0.5 * rng.uni();
            g.color2 = RGB(0.6f, 0.58f, 0.55f);
            break;
        case PT_VOLCANIC:
            g.volcanoes = 0.4 + 0.5 * rng.uni();
            g.lavaLevel = -400 - 400 * rng.uni();
            g.mountainAmp = 800 + 900 * rng.uni();
            g.hillAmp = 100 + 150 * rng.uni();
            g.color2 = RGB(0.95f, 0.8f, 0.3f);
            break;
        case PT_CARBON:
            g.mountainAmp = 500 + 700 * rng.uni();
            g.hillAmp = 60 + 90 * rng.uni();
            g.craterDensity = 0.15 + 0.2 * rng.uni();
            g.color2 = RGB(0.9f, 0.95f, 1.0f);
            break;
    }
    {   // R-304: the traits, hashed (the rng stream stays as in generation 7): none for a fifth of the bodies, one for two
        // fifths, two for a bit over a quarter, three for the rest, drawn from the type's list without repeats; one colour
        // trait at most, pangaea and archipelago never together
        auto tu = [&](int k) { return unitFromHash(mix64(b.seed ^ (0x7A17ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
        int eligL[TR_COUNT], eligW[TR_COUNT]; int nl = 0, nw = 0;   // the landform traits (1..18) and the world traits (19..)
        for (int t = 1; t < TR_COUNT; t++) if (traitEligible(b.type, t)) { if (t <= TR_GEYSERS) eligL[nl++] = t; else eligW[nw++] = t; }
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
                bool colT = t == TR_RED_SOIL || t == TR_BLACK_SAND || t == TR_CHALK, colO = o == TR_RED_SOIL || o == TR_BLACK_SAND || o == TR_CHALK;
                if (colT && colO) ok = false;
                if ((t == TR_PANGAEA && o == TR_ARCHIPELAGO) || (t == TR_ARCHIPELAGO && o == TR_PANGAEA)) ok = false;
                if ((t == TR_GIANT_FLORA && o == TR_DEAD_FOREST) || (t == TR_DEAD_FOREST && o == TR_GIANT_FLORA)) ok = false;
            }
            if (ok) g.traits[got++] = t;
        }
        if (b.glassed) {   // S-03: a survivor of its neutron star's supernova: the glass comes first and the exotic seas boiled off (the glass owns bank 2)
            int keep[3] = {0, 0, 0}, nk = 0;
            for (int k = 0; k < 3 && nk < 2; k++) if (g.traits[k] && g.traits[k] != TR_EXOTIC_SEAS) keep[nk++] = g.traits[k];
            g.traits[0] = TR_GLASSED; g.traits[1] = keep[0]; g.traits[2] = keep[1];
        }
        for (int k = 0; k < 3; k++) switch (g.traits[k]) {
            case TR_ARCHIPELAGO: g.seaLevel += 0.3; g.islandDensity = 0.5; break;
            case TR_PANGAEA: g.contScale = 1.8; g.seaLevel -= 0.05; break;
            case TR_LAKELAND: g.lakeDensity = 0.85; g.riverDensity = 0.9; break;
            case TR_SNOWBALL: g.iceCapLat = 28 + 12 * tu(20); g.snowLine *= 0.55; g.tempBias -= 6; break;
            case TR_EXOTIC_SEAS: g.liquidLevel = b.type == PT_CARBON ? -150 - 300 * tu(21) : (b.type == PT_ICY ? -60 - 80 * tu(21) : -250 - 500 * tu(21)); break;
            case TR_STORMS: g.cloudCover = std::min(1.0, g.cloudCover + 0.35); break;
            case TR_DEAD_FOREST: g.vegColor = lerp(g.vegColor, RGB(0.45f, 0.42f, 0.35f), 0.65f); break;
            default: break;
        }
    }
    {   // N2-05: the second family's colour, a related hue from the family palettes (hashed, the rng stream untouched)
        static const RGB pal[5] = {RGB(0.28f, 0.6f, 0.12f), RGB(0.55f, 0.15f, 0.2f), RGB(0.3f, 0.25f, 0.65f), RGB(0.7f, 0.6f, 0.15f), RGB(0.15f, 0.55f, 0.6f)};
        RGB base = (g.vegColor.r + g.vegColor.g + g.vegColor.b > 0.05f) ? g.vegColor : RGB(0.25f, 0.5f, 0.15f);
        int nearest = 0; float bestD = 1e9f;
        for (int k = 0; k < 5; k++) { float d = (pal[k].r - base.r) * (pal[k].r - base.r) + (pal[k].g - base.g) * (pal[k].g - base.g) + (pal[k].b - base.b) * (pal[k].b - base.b); if (d < bestD) { bestD = d; nearest = k; } }
        RGB other = pal[(nearest + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10CULL)) * 4)) % 5];   // a different family palette
        g.vegColor2 = lerp(base, other, 0.6f);
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
        double k = 1000.0 * g.reliefWl0 / 16.0, flat = 0.10, hill = 0.5, mtn = 2.6;   // metres per 16 km of base wavelength, before the ridge normalisation
        switch (b.type) {
            case PT_FELISIAN: mtn = 4.0; hill = 0.42; break;   // its ridge blend is the softest (0.4 + 0.6 rel), so it needs the most amplitude per grade
            case PT_MOLTEN: g.reliefRidge += 0.15; mtn = 3.7; hill = 0.42; break;   // O6-07: 3.0 read a 0.23 median and 1309 m over 16 km with the cliff bands' benches
            case PT_CRATERED: g.reliefRidge *= 0.6; mtn = 1.5; hill = 0.4; break;          // lunar highlands roll
            case PT_VENUSIAN: mtn = 1.9; hill = 0.55; break;
            case PT_ROCKY: g.reliefRidge += 0.2; mtn = 2.7; break;   // O6-07: 2.4 read a 0.24 median with the benches
            case PT_THINATMO: g.reliefRidge += 0.15; mtn = 2.3; break;
            case PT_ICY: g.reliefRidge *= 0.5; flat = 0.14; mtn = 1.8; hill = 0.6; break;   // chaos plains between the cracks
            case PT_QUARTZ: g.reliefRidge = 0.05; g.reliefHybrid = 0.8; flat = 0.11; mtn = 1.2; hill = 0.34; break;   // milky mounds
            case PT_OCEAN: flat = hill = mtn = 0; break;
            case PT_METAL: g.reliefRidge += 0.3; mtn = 2.3; break;
            case PT_VOLCANIC: mtn = 1.9; hill = 0.85; break;
            case PT_CARBON: mtn = 1.4; hill = 0.32; break;
            // R-307
            case PT_EUROPAN: g.reliefRidge *= 0.3; flat = 0.05; hill = 0.3; mtn = 0.5; break;        // a shell a few tens of metres in relief
            case PT_TECTONIC: g.reliefRidge += 0.3; mtn = 2.6; hill = 0.7; break;                    // fault-block ranges
            case PT_DESERT: mtn = 2.0; hill = 0.65; break;
            case PT_HYDROCARBON: g.reliefRidge *= 0.4; g.reliefHybrid = 0.8; flat = 0.08; hill = 0.35; mtn = 1.2; break;   // rounded hills, flat lowlands
            case PT_BOMBARDED: g.reliefRidge *= 0.6; mtn = 1.5; hill = 0.4; break;
            case PT_ACIDIC: mtn = 1.8; hill = 0.65; break;
            default: break;
        }
        g.reliefRidge = clampd(g.reliefRidge, 0, 1);
        g.reliefFlat = flat * k; g.reliefHill = hill * k; g.reliefMtn = mtn * k;
        // O6-04: the cliff bands (a shell of ice and a soft world have fewer and lower ones)
        g.cliffStep = 60 + 70 * hu(7);
        g.cliffAmount = 0.45 + 0.4 * hu(8);
        if (b.type == PT_EUROPAN || b.type == PT_HYDROCARBON || b.type == PT_QUARTZ || b.type == PT_ICY) { g.cliffAmount *= 0.5; g.cliffStep *= 0.6; }
        if (b.type == PT_COMET) g.cliffAmount = 0;
    }
    materialPalette(g);   // N1-01
    return g;
}

double climateTempC(const BodyGen& g, double absLatDeg, double heightM, double winter, double sub) {
    // B-321: Earth's mean temperature against latitude runs 27 C at the equator, 15 at 35 degrees, 5 at 50, -2 at 60,
    // -9 at 70: `-18 + 46 cos(lat)^1.5`; the old line `28 - 0.55 lat` was five degrees too cold in the middle latitudes
    double c = std::cos(absLatDeg * DEG);
    double T = -18.0 + 46.0 * c * std::sqrt(c) - 6.5 * std::max(0.0, heightM) / 1000.0 + g.tempBias - 6.0 * winter;
    if (g.locked) T += 45.0 * sub - 6.0 + 0.35 * absLatDeg;   // M5-08: a permanent day side and night side; the twilight belt between keeps the mild climate
    return T;
}

namespace {
// R-305: the landform library. `landformsAt` adds the traits' landforms at a point: `dhSmooth` moves the smooth ground
// and the ground alike (canyons, mesas, rifts, scarps, basins...: rivers and lakes are cut into the result), `dhFine`
// the ground alone (spires, polygons, terraces, crevasses), `flatten` melts the ground into the smooth ground by that
// fraction, `flatOffset` metres off it (playas, glaciers); `mat`/`albedo` override the material where set. Every term
// fades with the sampling scale (`lodFade`, `scaleVisible`) so the map, the far rings and the ground agree
struct LandCtx { double landMask = 1, moist = 0.15, relief0 = 0, h = 0, hS = 0, T = 15, hM = 0, slope = 0; };   // B-404: hM = the ground on the material chain's relief, slope = that chain's slope (the same in every ring)
struct LandOut { double dhSmooth = 0, dhFine = 0, flatten = 0, flatOffset = 0; int mat = -1; double albedo = -1; double glacier = 0, riftFloor = 0; double mark = 0; };   // mark: 1 where the landform is at its most distinctive (the galleries)
void landformsAt(const BodyGen& g, const Vec3& p, const Vec3& unit, double detailM, const LandCtx& c, LandOut& o) {
    if (!g.traits[0]) return;
    const double R = g.R;
    // a province: about a third of the world, its own for every landform
    auto prov = [&](int k, double lo, double hi) { return smoothstep(lo, hi, fbm3(p / (R * 0.22) + Vec3(31.7 * k, 11.3 * k, 7.9 * k), g.sE + 200 + k, 2) * lodFade(R * 0.22, detailM)); };
    auto tu = [&](int k) { return unitFromHash(mix64(g.seed ^ (0x1A4DULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
    auto setMat = [&](int m, double a) { o.mat = m; o.albedo = a; };
    double land = c.landMask;
    for (int ti = 0; ti < 3; ti++) {
        int t = g.traits[ti];
        if (!t) break;
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
            double cold = (g.type == PT_ICY || g.type == PT_COMET) ? 1.0 : smoothstep(4, -2, c.T);
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
            if (mask > 0.5) { setMat(MAT_SAND, g.type == PT_FELISIAN ? 0.6 : -1.0); o.mark = mask; }   // B-404: the sand sea is kilometres across: sand in every ring and on the maps, whether or not its draa can show (the sand used to go with the draa: dust beyond the 64 m ring); an airless or dry type keeps its own albedo under it, so its globe keeps its mottling (a flat 0.6 made a dune-sea world one yellow ball)
            double wl = 1.0 + 1.5 * tu(11), amp = 40 + 80 * tu(12);
            double f = lodFade(wl, detailM);   // the draa are a periodic pattern: the octave rule (KI-311)
            if (f <= 0) break;
            Vec3 dd = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.15 * std::sin(g.windAngle * 2)));
            Vec3 dp = normalize(cross(dd, unit));
            double along = dot(p, dd) / wl * TAU, across = dot(p, dp) / wl * TAU;
            double bend = fbm3(p / 8.0, g.sE + 330, 2) * 3.0;
            double v = std::pow(0.5 + 0.5 * std::sin(along + bend + 0.5 * std::sin(across * 0.7)), 1.6);
            o.dhSmooth += amp * v * mask * f;
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
            if (g.type == PT_QUARTZ || g.type == PT_CARBON) {   // crystal spires 10-35 m tall in fields
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
    LandOut land;   // R-305: the traits' landforms here
};
void felisianGround(const BodyGen& g, const Vec3& unit, double detailM, FelGround& o) {
    const double R = g.R;
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
    {   // R-305: the traits' landforms, cut into the smooth ground too (rivers and lakes follow them)
        LandCtx lc; lc.landMask = landMask; lc.moist = moist; lc.relief0 = relief0; lc.h = h; lc.hS = hS; lc.T = climateTempC(g, std::fabs(lat) / DEG, hS, 0, 0); lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
        landformsAt(g, p, unit, detailM, lc, o.land);
        h += (hS - h) * o.land.flatten;
        double dS = o.land.dhSmooth + o.land.flatOffset * o.land.flatten;
        h += dS + o.land.dhFine; hS += dS;
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
    if (h < 0) h = -std::pow(-h, 1.15);            // shallow coastal shelf
    if (hS < 0) hS = -std::pow(-hS, 1.15);
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
        double lf0 = std::fabs(lat) / DEG / 85.0;
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
    if (moist < 0.28 && h > 5 && h < 700 && relief0 < 0.3) h += duneField(p, g, lat, 6.0 + 16.0 * (0.28 - moist) / 0.28, detailM) * (1 - smoothstep(0.24, 0.28, moist));
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
    double lf = absLatDeg / 85.0;
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
    s.height = h;
    s.relief = clampd(relief0 + 0.5 * o.slope, 0, 1);
    // M9-05 biomes from temperature (latitude, altitude, the body's climate) and moisture
    double T = climateTempC(g, absLatDeg, h, winter, sub);
    if (g.locked) moist -= 0.35 * std::max(0.0, sub - 0.15);   // M5-08: deserts under a permanent sun
    int biome = BIO_GRASSLAND;
    if (T < -8) biome = BIO_ICE;
    else if (T < 2) biome = BIO_TUNDRA;
    else if (T < 10) biome = moist > 0.45 ? BIO_TAIGA : BIO_TUNDRA;
    else if (T < 20) biome = moist > 0.55 ? BIO_TEMPERATE : (moist > 0.28 ? BIO_GRASSLAND : BIO_DESERT);
    else biome = moist > 0.55 ? BIO_TROPICAL : (moist > 0.35 ? BIO_SAVANNA : BIO_DESERT);
    if (moist > 0.72 && h < 120 && relief0 < 0.2 && biome != BIO_ICE) biome = BIO_WETLAND;
    if (relief0 > 0.45 && h > std::max(700.0, 0.5 * snowLine)) biome = BIO_ALPINE;   // B-321: the zone between the treeline and the snow (it began at 700 m everywhere: 13% of the land was bare rock)
    bool inland = level > -1e8 && h < level;   // under a river, a lake or a pool
    if (h < 0) {
        s.material = MAT_WATER; s.water = 0; s.biome = BIO_OCEAN;
        s.albedo = h > -250 ? 0.36 : 0.22;
        if (absLatDeg > capEdge + 4 || (g.locked && T < -10)) { s.material = MAT_ICE; s.albedo = 0.85; s.height = 0; s.biome = BIO_ICE; }
    } else if (inland) {
        s.material = T < -2 ? MAT_ICE : MAT_WATER; s.water = level; s.albedo = 0.3; s.biome = (biome == BIO_ICE || biome == BIO_TUNDRA) ? biome : BIO_WETLAND;
        if (T < -2) { s.height = level; s.albedo = 0.8; }
    } else if (absLatDeg > capEdge || h > snowLine || biome == BIO_ICE) {
        s.material = MAT_SNOW; s.albedo = 0.92; s.biome = BIO_ICE;
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
    if (s.material != MAT_WATER) s.albedo += 0.08 * clampd(h / 4000.0, 0, 1);
    s.landform = 1000 * o.land.mark + std::fabs(o.land.dhSmooth) + std::fabs(o.land.dhFine) + 40 * o.land.flatten;
    if (s.water > 0.5 && waterZone > 0) s.waterKind = waterKind;
    if (waterZone > 0 && s.water > -1e8) s.shore = shore;   // B-322 (a snowfield or a glacier over the zone keeps the plane, so it keeps the edge too)
    else if (s.water < -1e8 && shoreOut < 1e8) s.shore = shoreOut;   // the bank beyond the plane's reach still knows how far the water is
}
}

double sampleCloudPattern(const BodyGen& g, double lon, double lat) {
    if (g.cloudCover <= 0) return 0;
    // sample on the sphere so the pattern wraps seamlessly
    Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
    double n = fbm3(u * g.cloudScale, g.sG, 4, 2.1, 0.55);
    double bands = 0.15 * std::sin(lat * 6.0 + n * 3.0);
    double v = n * 0.5 + 0.5 + bands;
    if (g.type == PT_VENUSIAN) {
        double swirl = fbm3(u * (g.cloudScale * 2.0) + Vec3(0, 0, lat * 3.0), g.sG + 5, 3);
        return clampd(0.55 + 0.45 * (0.5 * n + 0.5 * swirl) + bands, 0.15, 1.0);
    }
    double thr = 1.0 - g.cloudCover;
    return clampd((v - thr) / std::max(0.05, 1.0 - thr) * 1.6, 0.0, 1.0);
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
SurfaceSample sampleSurface(const BodyGen& g, const Vec3& unit, double detailM) {
    SurfaceSample s;
    const double R = g.R;
    Vec3 p = unit * R;                    // km
    double lat = std::asin(clampd(unit.z, -1, 1));
    double absLatDeg = std::fabs(lat) / DEG;
    CraterMarks cm;
    double& rimBright = cm.rimBright;
    // Layer helpers: each term fades out when its wavelength is too small for the sampling scale.
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
    auto crater = [&](double cellKm, double dens, double depth) {
        if (!scaleVisible(cellKm, detailM)) return 0.0;
        return craterField(p, R, cellKm, dens, depth, g.sD + (uint64_t)(cellKm * 1000), cm, detailM);
    };
    Tectonics tec;
    if (g.tectonic && g.type != PT_FELISIAN) tec = tectonicsAt(unit, g);
    // O6-02: where the relief spectrum is strong: massifs (a ridged field of about 110 km) inside a type's mountain
    // regions, and rolling hill country (a broad fbm); the amplitude follows `reliefAmp` (B-320: the felisian layer
    // computes its own in `felisianGround`)
    double massif = 0, hillsF = 0;
    if (g.type != PT_GASGIANT && g.type != PT_SUBSTELLAR && g.type != PT_OCEAN && g.type != PT_COMPANION && g.type != PT_FELISIAN) {
        massif = smoothstep(0.2, 0.6, rg(std::min(110.0, R * 0.3), g.sC + 6, 3));
        hillsF = smoothstep(-0.15, 0.45, fb(std::min(R * 0.06, 400.0), g.sB + 6, 3));
    }
    // crack lines on the boundaries of worley cells: `h` is the faded value for the relief, `m` the unfaded one for the
    // material wherever a line (`width` cells wide) spans half a sample (B-404: the icy worlds' rock cracks and the
    // molten worlds' lava lines used to follow the fade and end at a ring's edge)
    struct Crack { double h = 0, m = 0; };
    auto crackAt = [&](double cellKm, uint64_t seed, double width, double strength) -> Crack {
        Crack c;
        bool hv = scaleVisible(cellKm, detailM), mv = matVisible(width * cellKm, detailM);
        if (!hv && !mv) return c;
        Worley3 w = worley3(p / cellKm, seed);
        double v = strength * (1 - smoothstep(width * 0.5, width * 1.5, w.f2 - w.f1));
        if (hv) c.h = v * lodFade(cellKm, detailM);
        if (mv) c.m = v;
        return c;
    };

    LandOut lo;   // R-305: the traits' landforms of this point (the felisian layer keeps its own)
    // O6-04: the cliff bands and the plains' small relief of every solid type (the felisian layer has its own); a shell of
    // ice and a soft cold world take less, worlds where it rains get gullies
    double cliffV = 0, talusV = 0;
    double plainK = 1.0, gullyK = 0.6;   // the gullies are contraction cracks and collapse seams on the worlds where nothing rains
    switch (g.type) {
        case PT_EUROPAN: plainK = 0.3; gullyK = 0.3; break;
        case PT_HYDROCARBON: plainK = 0.7; gullyK = 0.8; break;
        case PT_ACIDIC: plainK = 0.6; gullyK = 1.0; break;   // O6-07: its rained-on plains read a 0.048 median grade against the flat class's 0.04
        case PT_THINATMO: plainK = 0.6; gullyK = 0.6; break;
        case PT_ICY: plainK = 0.5; gullyK = 0.4; break;
        case PT_CRATERED: case PT_BOMBARDED: case PT_METAL: plainK = 0.5; gullyK = 0.3; break;   // their plains are crater fields
        default: break;
    }
    auto shape = [&](double& h, const ReliefSum& rf, double rel, double hf) {
        double relief0 = clampd(rel + 0.25 * hf, 0, 1);
        // the cliffs' country: the massifs, and steep ground anywhere at seven tenths (a thin-atmosphere world's
        // mountains are hill-zone spectrum on a big base term, `rel` near 0: they had 0.1% of cliffs against 3-8)
        double cliffZone = std::max(smoothstep(0.12, 0.4, relief0), 0.7 * smoothstep(0.3, 0.5, rf.slopeM));   // B-404: on the material chain's slope
        CliffOut co = cliffBands(p, g, h, rf.slopeM, cliffZone, detailM);
        h += co.dh; cliffV = std::max(cliffV, co.cliff); talusV = std::max(talusV, co.talus);
        h += plainRelief(p, g, plainK * smoothstep(0.3, 0.1, relief0), gullyK, detailM);
    };
    DrainApply DA;   // O6-03: the drainage's channels of the desert, thin-air, hydrocarbon and acidic layers
    bool wetType = g.type == PT_HYDROCARBON || g.type == PT_ACIDIC;
    switch (g.type) {
    case PT_FELISIAN: felisianSample(g, unit, detailM, s); break;
    case PT_CRATERED: {
        double base = fb(R * 0.5, g.sA, 5);
        double mare = smoothstep(0.15, 0.4, base);
        double h = base * g.mountainAmp * (1 - 0.8 * mare);
        // O6-02: rolling highlands between the maria (the 20 km hills and the 400 m detail of generation 5 are gone)
        double rel = (1 - mare) * massif, hf = std::max(hillsF, 0.6 * (1 - mare));
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf) * (1 - 0.7 * mare), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        double cr = 0;
        cr += crater(R * 0.55, g.craterDensity * 0.6, g.craterDepth);
        cr += crater(R * 0.18, g.craterDensity, g.craterDepth);
        cr += crater(R * 0.06, g.craterDensity, g.craterDepth);
        cr += crater(12.0, g.craterDensity * 1.2, g.craterDepth);
        cr += crater(2.5, g.craterDensity * 1.5, g.craterDepth);
        double sec = clampd(cm.secondary, 0, 1);                        // M9-06 secondary fields near large craters
        cr += crater(0.5, std::min(1.0, 0.7 + 0.3 * sec), g.craterDepth * 0.8);
        cr += crater(0.1, 0.9, g.craterDepth * 0.6);
        cr += crater(0.025, 0.8, g.craterDepth * 0.5);
        h += cr * (1 - 0.5 * mare);
        s.height = h;
        double flooded = clampd(cm.flooded, 0, 1);
        s.material = (mare > 0.5 || flooded > 0.5) ? MAT_BASALT : MAT_DUST;
        s.albedo = (0.52 + 0.12 * base) * (1 - 0.45 * std::max(mare, flooded)) + 0.12 * clampd(rimBright, 0, 1) - 0.08 * clampd(cm.floorDark, 0, 1) + 0.3 * clampd(cm.rays, 0, 1);
        s.relief = clampd(std::fabs(cr) / 400.0 + rel, 0, 1);
        break;
    }
    case PT_MOLTEN: {
        double base = fb(R * 0.4, g.sA, 5);
        double roughL = rg(R * 0.15, g.sC, 2);
        bool caldera = false;
        double h = base * 1600.0 + roughL * g.mountainAmp * 0.5;
        double zone = smoothstep(0.45, 0.85, roughL), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: the rough belts carry the relief spectrum
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        if (scaleVisible(R * 0.25, detailM)) h += volcanoField(p, R, R * 0.25, g.volcanoes * 0.6, g.sD, 2800.0, caldera);
        if (scaleVisible(30.0, detailM)) h += volcanoField(p, R, 30.0, g.volcanoes * 0.8, g.sD + 3, 700.0, caldera);
        LavaOut lf = lavaFlows(p, R, 30.0, g.volcanoes * 0.8, g.sD + 3, detailM);   // O6-05: tongues and pit chains from the cones
        h += lf.dh;
        s.height = h;
        s.relief = clampd(roughL + 0.5 * rel + 0.5 * rf.slope, 0, 1);
        Crack crack = crackAt(12.0, g.sF, 0.05, 1.0);
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
        break;
    }
    case PT_VENUSIAN: {
        double base = fb(R * 0.45, g.sA, 5);
        double h = base * g.mountainAmp + tec.uplift * 800.0;
        { double chainR = rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.7) - tec.rift * 800.0 + tec.ridge * 400.0; }
        double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: tesserae on the highlands and along the chains
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        if (scaleVisible(R * 0.08, detailM)) h += domeField(p, R, R * 0.08, 0.3, g.sD, 0.35, 700.0, 0.85) * lodFade(R * 0.08, detailM);
        if (scaleVisible(3.0, detailM)) h += domeField(p, R, 3.0, 0.3, g.sD + 1, 0.4, 40.0, 0.9);
        s.height = h;
        s.material = MAT_BASALT;
        s.albedo = 0.30 + 0.12 * base;
        s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_ROCKY: {
        double creaseL = rg(R * 0.15, g.sA, 2);
        double base = fb(R * 0.45, g.sB, 4);
        double h = creaseL * g.mountainAmp + base * 600.0;
        {   // M9-02: chains and rifts across the crust
            double chainR = rg(40.0, g.sA + 2, 7);
            h += tec.chain * (chainR * chainR * g.chainAmp * 0.8 + std::fabs(chainR) * 500.0) + tec.uplift * 600.0;
            h -= tec.rift * 900.0 * (0.5 + 0.5 * std::fabs(fb(50.0, g.sA + 3, 3)));
            h += tec.ridge * 500.0 * std::fabs(rg(35.0, g.sA + 4, 5));
        }
        double zone = clampd(smoothstep(0.3, 0.7, creaseL) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: creased ridges (the 20 km crease, the 4 km hills and the detail are gone)
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        if (scaleVisible(R * 0.3, detailM)) h += domeField(p, R, R * 0.3, 0.2, g.sD + 7, 0.4, 900.0, 0.6) * lodFade(R * 0.3, detailM);   // old shield volcanoes
        if (scaleVisible(0.25, detailM)) h += domeField(p, R, 0.25, g.boulderDensity, g.sD, 0.16, 14.0, 0.0);
        if (scaleVisible(0.06, detailM)) h += domeField(p, R, 0.06, g.boulderDensity * 0.7, g.sD + 2, 0.2, 3.0, 0.0);
        h += crater(20.0, g.craterDensity, 0.6);
        s.height = h;
        s.material = MAT_ROCK;
        s.albedo = 0.28 + 0.18 * creaseL + 0.05 * base + 0.06 * clampd(rf.slope * 2.0, 0, 1);
        s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_THINATMO: {
        double base = fb(R * 0.5, g.sA, 5);
        double mtnL = rg(R * 0.14, g.sC, 2);
        double canyonN = rg(R * 0.3, g.sF, 2);
        double canyon = std::pow(clampd(canyonN, 0, 1), 9.0);
        double mtnMask = smoothstep(0.3, 0.7, mtnL) * smoothstep(-0.2, 0.4, base);
        double h = base * 1800.0 + mtnL * mtnL * g.mountainAmp * mtnMask - canyon * 2200.0;
        {   // M9-02 chains and rifts, M9-07 shield volcanoes with dark lava plains around them
            double chainR = rg(40.0, g.sC + 2, 7);
            h += tec.chain * (chainR * chainR * g.chainAmp * 0.9 + std::fabs(chainR) * 500.0) + tec.uplift * 900.0;
            h -= tec.rift * 1500.0 * (0.5 + 0.5 * std::fabs(fb(50.0, g.sC + 3, 3)));
            h += tec.ridge * 600.0 * std::fabs(rg(35.0, g.sC + 4, 5));
        }
        // O6-02: the relief spectrum (the 30 km ridges, the 8 km hills and the 400 m detail of generation 5 are gone)
        double zone = clampd(mtnMask + 0.8 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
        double relief0 = clampd(rel + 0.25 * hf, 0, 1);
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        bool shieldCal = false;
        double shield = 0;
        if (scaleVisible(R * 0.35, detailM)) { shield = volcanoField(p, R, R * 0.35, 0.28, g.sD + 11, 1400.0, shieldCal) * lodFade(R * 0.35, detailM); h += shield; }
        h += crater(R * 0.2, g.craterDensity * 0.7, g.craterDepth * 0.7);
        h += crater(R * 0.05, g.craterDensity, g.craterDepth * 0.6);
        h += crater(8.0, g.craterDensity * 0.9, g.craterDepth * 0.5);
        h += crater(1.5, g.craterDensity * 0.9, 0.4);
        h += crater(0.3, 0.5, 0.3);
        h += crater(0.06, 0.4, 0.25);
        if (h < 200) h += duneField(p, g, lat, g.duneAmp, detailM) * smoothstep(200, -300, h);   // dune seas in the lowlands (M9-08)
        applyDrainage(g, unit, detailM, h, nullptr, rf.slope, false, false, 0.5, DA);   // O6-03: the old valley networks, dry, with dust flats in the basins
        s.height = h;
        s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
        double capEdge = g.iceCapLat + 5.0 * gnoise3(p / (R * 0.15), g.sF + 7);
        if (absLatDeg > capEdge) { s.material = MAT_ICE; s.albedo = 0.85; }
        else if (DA.playa > 0.3) { s.material = MAT_DUST; s.albedo = 0.56 + 0.06 * base; }   // O6-03: a dust flat in a closed basin
        else if (relief0 + 0.5 * rf.slopeM > 0.6 && h > 1500) { s.material = MAT_ROCK; s.albedo = 0.35; }   // B-404: the material chain's slope, not `s.relief` (whose slope is the ring's own)
        else if (shield > 400 || shieldCal) { s.material = MAT_BASALT; s.albedo = 0.22 + 0.06 * base; }
        else { s.material = MAT_DUST; s.albedo = 0.42 + 0.12 * base + 0.10 * clampd(rimBright, 0, 1) - 0.12 * canyon + 0.25 * clampd(cm.rays, 0, 1) - 0.1 * clampd(cm.flooded, 0, 1); }
        break;
    }
    case PT_COMET:
    case PT_ICY: {
        double base = fb(R * 0.45, g.sA, 5);
        double h = base * g.mountainAmp;
        double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: ridged uplands on the high ground (the 6 km hills and the detail are gone)
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        Crack c1 = crackAt(R * 0.15, g.sF, g.fractureWidth, 1.0), c2 = crackAt(12.0, g.sF + 3, g.fractureWidth, 0.8), c3 = crackAt(1.2, g.sF + 5, g.fractureWidth * 1.3, 0.6);
        double crack = std::max(c1.h, std::max(c2.h, c3.h)), crackM = std::max(c1.m, std::max(c2.m, c3.m));   // B-404: the relief by the faded lines, the rock by the unfaded ones
        h -= crack * 60.0;
        h += crater(R * 0.15, g.craterDensity, 0.5);
        h += crater(6.0, g.craterDensity, 0.4);
        h += crater(0.8, g.craterDensity * 1.2, 0.3);
        h += crater(0.15, 0.35, 0.25);
        // M9-07 cryovolcanoes (bright fresh domes) and geyser fields (bright spots on the ground)
        double fresh = 0;
        if (scaleVisible(25.0, detailM)) { double cv = domeField(p, R, 25.0, 0.12, g.sD + 21, 0.25, 140.0, 0.0); h += cv; fresh += cv / 140.0; }
        if (scaleVisible(2.0, detailM)) fresh += domeField(p, R, 2.0, 0.08, g.sD + 22, 0.15, 1.0, 0.0);
        s.height = h;
        s.material = crackM > 0.55 ? MAT_ROCK : MAT_ICE;
        s.albedo = (0.72 + 0.12 * base) * (1 - 0.5 * crackM) + 0.1 * clampd(rimBright, 0, 1) + 0.15 * clampd(fresh, 0, 1) + 0.2 * clampd(cm.rays, 0, 1);
        s.relief = clampd(crack + 0.5 * rel + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_QUARTZ: {
        double base = fb(R * 0.35, g.sA, 5);
        double h = base * g.mountainAmp;
        double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: rounded mounds (ridge near 0, low ground smoothed; the 3.5 km rounded hills are gone)
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        if (scaleVisible(R * 0.06, detailM)) h += domeField(p, R, R * 0.06, 0.4, g.sD, 0.3, 500.0, 0.0) * lodFade(R * 0.06, detailM);
        h += crater(15.0, g.craterDensity, 0.5);
        s.height = h;
        s.material = MAT_QUARTZ;
        s.albedo = 0.78 + 0.12 * base;
        s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1) * 0.6;
        break;
    }
    case PT_OCEAN: {
        // one global sea; ice floes drift toward the poles (plates with water gaps). B-322: the floe zone lies over a shallow
        // shelf sea (the floor at 5-8 m; the open ocean stays at 150 m) and every plate rises out of it through a submerged
        // ice foot, so the shore is the water plane's contour on a continuous surface at every ring: it used to be a jump
        // from +1 m to -150 m between two vertices, a stair of cells from the air and a cliff face from the water
        double swell = fb(2.0, g.sB, 4) * g.hillAmp;
        double capEdge = g.iceCapLat + 8.0 * gnoise3(p / (R * 0.2), g.sF + 7) - 8.0 * clampd(lat >= 0 ? -g.season : g.season, 0, 1);
        double nightIce = g.locked ? clampd((-dot(unit, g.lockedDir) - 0.05) / 0.3, 0, 1) : 0.0;   // M5-08: the night side freezes over
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
        break;
    }
    case PT_EUROPAN: {
        // R-307: an ice shell over a hidden ocean: a plain a few tens of metres in relief, crossed by lineae (double ridges
        // with a trough between, stained red-brown by what wells up through them), chaos terrain where the shell broke
        // into rafts, a few craters. The water geysers stand on the lineae (`drawGeysers` finds the stained cells)
        double base = fb(R * 0.5, g.sA, 4);
        double h = base * 60.0;
        double zone = smoothstep(0.3, 0.7, base), rel = zone * massif * 0.3, hf = hillsF * 0.3;
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
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
        double chaosMask = smoothstep(0.35, 0.6, fb(R * 0.15, g.sC + 3, 3));   // rafts of the shell, jumbled where the ocean broke through
        if (chaosMask > 0 && scaleVisible(3.0, detailM)) {
            Worley3 cw = worley3(p / 3.0, g.sF + 9);
            double edge = smoothstep(0.02, 0.07, cw.f2 - cw.f1);
            h += chaosMask * edge * (60.0 * unitFromHash(cw.id1) - 30.0) * featFade(3.0, detailM);   // B-404: the rafts by their own size (the 512 m ring draws them)
        }
        h += crater(R * 0.1, 0.05, 0.4);
        h += crater(2.0, 0.08, 0.3);
        s.height = h;
        s.material = stain > 0.5 ? MAT_DUST : MAT_ICE;
        s.albedo = stain > 0.5 ? 0.40 - 0.1 * stain : (0.72 + 0.08 * base + 0.1 * clampd(rimBright, 0, 1) - 0.15 * chaosMask) * (1 - 0.35 * stain);
        s.relief = clampd(chaosMask * 0.5 + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_TECTONIC: {
        // R-307: a seismic world: rift valleys where the crust pulls apart (their floors flat, 800 m down), fissures on the
        // floors running with lava (`drawFountains` stands its fountains on them), fault-block ranges, ash plains, sulphur
        // crusts round the vents, a few volcanoes
        double base = fb(R * 0.4, g.sA, 5);
        double h = base * g.mountainAmp * 0.6 + tec.uplift * 700.0;
        double zone = clampd(smoothstep(0.1, 0.6, base) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
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
        h += crater(30.0, 0.08, 0.5);
        s.height = h;
        double ash = fb(R * 0.1, g.sC + 8, 3);
        if (lava > 0.5 || caldera) { s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0; }
        else if (fresh > 0.4 || lf.flow > 0.4 || lf.pit > 0.3) { s.material = MAT_BASALT; s.albedo = 0.12 + 0.06 * (1 - std::max(fresh, lf.flow)); if (lf.pit > 0.6) s.glow = 0.25; }
        else if (fresh > 0.12) { s.material = MAT_SULPHUR; s.albedo = 0.58 + 0.2 * fresh; }
        else if (rel < 0.45 && ash > 0.05 && rift < 0.5) { s.material = MAT_DUST; s.albedo = 0.34 + 0.1 * ash; }
        else { s.material = MAT_BASALT; s.albedo = 0.30 + 0.08 * (0.5 + 0.5 * base); }
        s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + rift * 0.3, 0, 1);
        break;
    }
    case PT_DESERT: {
        // R-307: a dry world under a dusty sky: hamada plains, mesas where the flats terrace, canyons, ergs in the basins
        // with salt pans at their floors (the dust devils and storms are the surface's, `computeEnvironment`)
        double base = fb(R * 0.5, g.sA, 5);
        double h = base * 1400.0 + tec.uplift * 600.0;
        { double chainR = rg(40.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 700.0 + tec.ridge * 300.0; }
        double zone = clampd(smoothstep(0.2, 0.7, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
        double relief0 = clampd(rel + 0.25 * hf, 0, 1);
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        double hS = h - (rf.h - rf.hCoarse);
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        double canyonN = rg(R * 0.3, g.sF, 2);
        double canyon = std::pow(clampd(canyonN, 0, 1), 8.0);
        h -= canyon * 900.0;
        if (relief0 < 0.4 && h > 100) {   // mesas: the dry flats terrace (M9-03's rule)
            double step = 45.0, q = std::floor(h / step) * step, f = (h - q) / step;
            double terr = q + step * smoothstep(0.35, 0.65, f);
            h += (terr - h) * (1 - smoothstep(0.25, 0.4, relief0)) * smoothstep(-0.1, 0.3, fb(R * 0.12, g.sC + 4, 3));
        }
        double basin = smoothstep(0.1, 0.5, -fb(R * 0.1, g.sG + 2, 3)) * (1 - smoothstep(300.0, 700.0, h)) * (1 - relief0);
        double salt = 0;
        if (basin > 0.7 && h < 80) { double pan = smoothstep(0.7, 0.9, basin); h += (hS - 4.0 - h) * pan; salt = pan; }
        if (basin > 0.2 && salt < 0.5) h += duneField(p, g, lat, g.duneAmp * (0.6 + 0.4 * basin), detailM) * smoothstep(0.2, 0.5, basin);
        h += crater(R * 0.1, g.craterDensity * 0.35, 0.35);
        h += crater(3.0, g.craterDensity * 0.6, 0.3);
        applyDrainage(g, unit, detailM, h, &hS, rf.slope, false, false, 0.5, DA);   // O6-03: wadis, and playas in the closed basins
        s.height = h;
        if (salt > 0.5 || DA.playa > 0.3) { s.material = MAT_SALT; s.albedo = 0.85; }
        else if (DA.bed > 0.4) { s.material = MAT_SAND; s.albedo = 0.6 + 0.1 * base; }   // the wadi's bed
        else if (basin > 0.5) { s.material = MAT_SAND; s.albedo = 0.6 + 0.1 * base; }
        else if (relief0 > 0.5 || canyon > 0.3) { s.material = MAT_ROCK; s.albedo = 0.4 + 0.1 * base; }
        else { s.material = MAT_DUST; s.albedo = 0.46 + 0.12 * base + 0.1 * clampd(rimBright, 0, 1); }
        s.relief = clampd(relief0 + 0.5 * rf.slope + canyon, 0, 1);
        break;
    }
    case PT_HYDROCARBON: {
        // R-307: under the orange haze: rounded hills of ice-rock, belts of dark tholin dunes in the low latitudes, seas
        // and lakes of methane in the lowlands and toward the poles (the liquid at the reference level, the post-step
        // below), dry channels cut down to them, cryovolcanic domes
        double base = fb(R * 0.45, g.sA, 5);
        double h = base * 700.0 + 130.0 - 170.0 * smoothstep(50.0, 75.0, absLatDeg);
        double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone);
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        double chan = crackAt(30.0, g.sF + 4, 0.06, 1.0).h;   // channels: shallow valleys on 30 km cells, deeper toward the sea
        h -= chan * 25.0 * smoothstep(400.0, 50.0, h);
        double dunes = smoothstep(35.0, 20.0, absLatDeg) * smoothstep(0.0, 0.3, fb(R * 0.12, g.sG + 5, 3)) * smoothstep(300.0, 60.0, h) * smoothstep(0.0, 20.0, h);
        if (dunes > 0.05) h += duneField(p, g, lat, g.duneAmp, detailM) * dunes;
        if (scaleVisible(25.0, detailM)) h += domeField(p, R, 25.0, 0.08, g.sD + 21, 0.25, 180.0, 0.0);
        h += crater(40.0, 0.06, 0.4);
        applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // O6-03: rivers of methane to the seas, lakes in the depressions
        s.height = h;
        s.material = dunes > 0.4 ? MAT_SAND : MAT_ICE;
        if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.2; }
        s.albedo = dunes > 0.4 ? 0.18 : 0.32 + 0.08 * base + 0.06 * clampd(rimBright, 0, 1);
        s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_BOMBARDED: {
        // R-307: a young surface under a rain of rock: craters at every scale with sharp rims and bright rays over a dark
        // glassy regolith; the strikes themselves are the surface's (`drawImpacts`)
        double base = fb(R * 0.4, g.sA, 5);
        double h = base * g.mountainAmp * 0.6;
        double zone = smoothstep(0.2, 0.6, base), rel = zone * massif * 0.7, hf = std::max(hillsF, 0.5 * zone);
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        double cr = crater(R * 0.3, 0.5, 1.0);
        cr += crater(R * 0.08, 0.7, 1.0);
        cr += crater(8.0, 0.9, 1.0);
        cr += crater(1.2, 1.0, 0.9);
        cr += crater(0.2, 0.9, 0.8);
        cr += crater(0.04, 0.8, 0.6);
        h += cr;
        s.height = h;
        s.material = MAT_DUST;
        s.albedo = 0.28 + 0.06 * base + 0.14 * clampd(rimBright, 0, 1) + 0.5 * clampd(cm.rays, 0, 1);
        s.relief = clampd(std::fabs(cr) / 200.0 + rel + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_ACIDIC: {
        // R-307: a corrosive world: the rock bleached and eaten into karst (sinkholes at 300 m, towers in the wettest
        // basins), sulphur crusts by the shores, seas of dilute acid at the reference level (the post-step below)
        double base = fb(R * 0.45, g.sA, 5);
        double h = base * g.mountainAmp * 0.8 + 150.0 + tec.uplift * 600.0;
        { double chainR = rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 600.0; }
        double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
        double relief0 = clampd(rel + 0.25 * hf, 0, 1);
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        double low = smoothstep(600.0, 150.0, h) * (1 - smoothstep(0.3, 0.5, relief0));
        if (low > 0.1 && scaleVisible(0.3, detailM)) {
            Worley3 kw = worley3(p / 0.3, g.sF + 12);
            h -= 9.0 * low * smoothstep(0.55, 0.15, kw.f1) * lodFade(0.3, detailM);
        }
        if (low > 0.5 && scaleVisible(1.5, detailM)) h += domeField(p, R, 1.5, 0.25, g.sD + 13, 0.12, 70.0, 0.0) * low;
        h += crater(20.0, 0.05, 0.4);
        applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // O6-03: rivers of dilute acid, lakes in the karst basins
        s.height = h;
        double crust = smoothstep(40.0, 5.0, h) * smoothstep(0.35, 0.15, relief0);
        if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.22; }
        else if (crust > 0.5) { s.material = MAT_SULPHUR; s.albedo = 0.55 + 0.15 * crust; }
        else { s.material = MAT_ROCK; s.albedo = 0.55 + 0.1 * base - 0.1 * low; }
        s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_METAL: {
        double base = fb(R * 0.4, g.sA, 5);
        double h = base * g.mountainAmp;
        double zone = smoothstep(-0.2, 0.5, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02: sharp iron ridges (the 30 km crease and the 4 km hills are gone)
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        double cr = crater(R * 0.3, g.craterDensity * 0.7, g.craterDepth);
        cr += crater(R * 0.08, g.craterDensity, g.craterDepth);
        cr += crater(8.0, g.craterDensity * 1.2, g.craterDepth);
        cr += crater(1.2, g.craterDensity * 1.4, g.craterDepth * 0.9);
        cr += crater(0.2, 0.8, g.craterDepth * 0.7);
        cr += crater(0.04, 0.8, g.craterDepth * 0.5);
        h += cr;
        s.height = h;
        s.material = MAT_METAL;
        s.albedo = 0.12 + 0.08 * base + 0.1 * clampd(rimBright, 0, 1) + 0.25 * clampd(cm.rays, 0, 1);
        s.relief = clampd(std::fabs(cr) / 300.0 + rel + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_VOLCANIC: {
        double base = fb(R * 0.4, g.sA, 4);
        bool caldera = false;
        double h = base * g.mountainAmp * 0.5;
        double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02 (the 5 km hills and the detail are gone)
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
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
        break;
    }
    case PT_CARBON: {
        double base = fb(R * 0.45, g.sA, 5);
        double h = base * g.mountainAmp;
        double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // O6-02 (the 6 km hills and the detail are gone)
        ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf), g.reliefRidge, g.sB + 7, detailM);
        double hRel = h; h += rf.h;
        shape(h, rf, rel, hf);   // O6-04
        {   // R-305: the traits' landforms
            LandCtx lc; lc.relief0 = clampd(rel + 0.25 * hf, 0, 1); lc.h = h; lc.hS = h - (rf.h - rf.hCoarse); lc.T = g.tempBias / 0.3 + 15; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;   // B-404: the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
            landformsAt(g, p, unit, detailM, lc, lo);
            h += (lc.hS - h) * lo.flatten + lo.dhSmooth + lo.dhFine + lo.flatOffset * lo.flatten;
        }
        h += crater(R * 0.15, g.craterDensity, 0.6);
        h += crater(5.0, g.craterDensity, 0.5);
        h += crater(0.4, 0.6, 0.4);
        s.height = h;
        Worley3 dw = worley3(p / (R * 0.1), g.sF + 2);
        bool diamond = unitFromHash(dw.id1) < 0.18 && dw.f1 < 0.45;
        s.material = diamond ? MAT_QUARTZ : MAT_GRAPHITE;
        s.albedo = diamond ? 0.85 + 0.1 * base : 0.05 + 0.04 * (0.5 + 0.5 * base) + 0.08 * clampd(rimBright, 0, 1);
        s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
        break;
    }
    case PT_SUBSTELLAR:
    case PT_GASGIANT: {
        double turb = fbm3(p / (R * 0.18), g.sA, 4) * 0.35;
        double turb2 = fbm3(p / (R * 0.06), g.sB, 2) * 0.05;
        double l = lat + turb + turb2;
        double bands = 0.5 + 0.28 * std::sin(l * g.bandCount) + 0.15 * std::sin(l * g.bandCount * 2.3 + 1.0) +
                       0.08 * std::sin(l * g.bandCount * 5.1 + 2.0);
        Vec3 q = p / (R * 0.12);
        q.z *= 2.5;
        Worley3 w = worley3(q, g.sC);
        double storm = (unitFromHash(w.id1) > 0.85) ? (1 - smoothstep(0.15, 0.4, w.f1)) : 0;
        s.height = 0;
        s.material = MAT_GAS;
        s.albedo = clampd(bands + 0.35 * storm, 0.22, 1.0);
        s.veg = storm;   // N5-03: the storm term kept apart (the map's veg channel) so the globe can let storms grow and fade with time
        break;
    }
    }
    if (g.type != PT_FELISIAN) {
        s.scree = clampd(talusV, 0, 1);   // O6-04
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
        // R-305: a landform's material (rock walls, basalt sheets, salt, crystal) over the type's own, never over lava
        if (lo.mat >= 0 && s.material != MAT_LAVA && s.material != MAT_WATER) { s.material = lo.mat; if (lo.albedo >= 0) s.albedo = lo.albedo; }
        // R-304 (TR_EXOTIC_SEAS): the ground under the liquid's level is sea
        if (g.liquidLevel > -1e8 && s.height < g.liquidLevel && s.material != MAT_LAVA) { s.material = MAT_WATER; s.water = g.liquidLevel; s.albedo = 0.22; s.glow = 0; }
    }
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
            cloud[i] = (uint8_t)clampi((int)(sampleCloudPattern(g, lon, lat) * 255 + 0.5), 0, 255);
            // M9-10: a polar vortex pattern on gas giants
            if (g.type == PT_GASGIANT && std::fabs(lat) > 76 * DEG) albedo[i] = (uint8_t)clampi((int)(albedo[i] * (1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat)))), 0, 255);
        }
    }
    seed = g.seed;
    valid = true;
}

int PlanetMap::texelIndex(double lon, double lat) const {
    double fx = (wrap2pi(lon + PI) / TAU) * W;
    double fy = (0.5 - lat / PI) * H;
    int x = clampi((int)fx, 0, W - 1), y = clampi((int)fy, 0, H - 1);
    return y * W + x;
}

namespace {
template <class T>
double bilinear(const std::vector<T>& v, double lon, double lat) {
    double fx = (wrap2pi(lon + PI) / TAU) * PlanetMap::W - 0.5;
    double fy = (0.5 - lat / PI) * PlanetMap::H - 0.5;
    int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    double tx = fx - x0, ty = fy - y0;
    int x1 = (x0 + 1) & (PlanetMap::W - 1);
    x0 &= (PlanetMap::W - 1);
    int y1 = clampi(y0 + 1, 0, PlanetMap::H - 1);
    y0 = clampi(y0, 0, PlanetMap::H - 1);
    double a = v[y0 * PlanetMap::W + x0], b = v[y0 * PlanetMap::W + x1];
    double c = v[y1 * PlanetMap::W + x0], d = v[y1 * PlanetMap::W + x1];
    return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
}
}

double PlanetMap::albedoAt(double lon, double lat) const { return bilinear(albedo, lon, lat) / 255.0; }
double PlanetMap::cloudAt(double lon, double lat) const { return bilinear(cloud, lon, lat) / 255.0; }
double PlanetMap::heightAt(double lon, double lat) const { return bilinear(height, lon, lat); }
int PlanetMap::materialAt(double lon, double lat) const { return material[texelIndex(lon, lat)]; }
