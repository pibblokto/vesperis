#include "drifters.h"
#include "kinds.h"
#include "system.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

// D-01: the dark's table (`docs/ideas/IDEAS-worlds-and-wanderers.md` section 3): a drifter's kind by weight, its own heat's range (K)
const DarkKind DARK_KINDS[] = {
    {PT_GASGIANT, 26, 60, 130},          // the loudest that call (SIMP 0136's aurorae): a giant's own heat, Neptune's 59 K to Jupiter's 125
    {PT_SUBSTELLAR, 14, 400, 1200},      // a brown dwarf, glowing with what is left of its making
    {PT_ROCKY, 8, 30, 60},               // a cold rock: the heat of its decaying uranium and thorium (Earth's flow alone would hold it at 35 K)
    {PT_CRATERED, 6, 25, 50},
    {PT_ICY, 7, 20, 40},
    {PT_NITROGEN, 3, 25, 40},
    {PT_BRINE_DWARF, 2, 25, 45},
    {PT_FROST_DWARF, 2, 18, 32},
    {PT_CARBON, 2, 30, 60},
    {PT_METAL, 2, 35, 65},
    {PT_DESERT, 5, 30, 55},
    {PT_EUROPAN, 3, 35, 60},             // its ocean kept by a moon's tides (it keeps one: `StarSystem::generate`)
    {PT_MOLTEN, 2, 180, 320},            // thrown out young: still molten under its crust
    {PT_TECTONIC, 3, 90, 170},           // young enough to keep its plates moving
    {PT_FROZEN_FELISIAN, 8, 30, 55},     // a living world that lost its sun (never a living one)
};
const int DARK_KIND_COUNT = (int)(sizeof(DARK_KINDS) / sizeof(DARK_KINDS[0]));

namespace {
int64_t floorDiv(int64_t a, int64_t b) { int64_t q = a / b; return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q; }
uint64_t drifterSeed(int64_t sx, int64_t sy, int64_t sz) { return mix64(sectorSeed(sx, sy, sz) ^ DRIFT_SALT); }

// whether a candidate at a sector, with its own uniform u, is kept: as often as the stars are dense there (its acceptance d / (1 - d) under
// the field's rate, so with the sectors that hold a star left out the drifters are as dense as the stars times the rate), twice as often
// among the old populations, and never in a sector with a star
bool kept(int64_t x, int64_t y, int64_t z, double u) {
    const double d = galaxyDensity((double)x, (double)y, (double)z);
    const double a = DRIFT_RATE * d / std::max(1.0 - d, 0.03);
    if (u >= a * DRIFT_OLD) return false;
    if (u >= a) {
        const int region = galaxyRegion(x, y, z);
        if (!(region == REGION_CORE || region == REGION_BULGE || region == REGION_HALO || region == REGION_CLUSTER)) return false;
    }
    return !sectorHoldsStar(x, y, z, d);
}

void addOnce(int64_t x, int64_t y, int64_t z, int64_t* sx, int64_t* sy, int64_t* sz, int& n) {
    for (int j = 0; j < n; j++) if (sx[j] == x && sy[j] == y && sz[j] == z) return;
    sx[n] = x; sy[n] = y; sz[n] = z; n++;
}
}

int drifterCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz) {
    const uint64_t h = hash3i(cx, cy, cz, DRIFT_SALT);
    int n = 0;
    for (int k = 0; k < DRIFT_CELL_MAX; k++) {
        const uint64_t hk = mix64(h ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(k + 1)));
        const int64_t x = cx * DRIFT_CELL + (int64_t)(unitFromHash(mix64(hk + 1)) * DRIFT_CELL);
        const int64_t y = cy * DRIFT_CELL + (int64_t)(unitFromHash(mix64(hk + 2)) * DRIFT_CELL);
        const int64_t z = cz * DRIFT_CELL + (int64_t)(unitFromHash(mix64(hk + 3)) * DRIFT_CELL);
        if (kept(x, y, z, unitFromHash(hk))) addOnce(x, y, z, sx, sy, sz, n);
    }
    return n;
}

int drifterGlobular(int64_t gx, int64_t gy, int64_t gz, int64_t* sx, int64_t* sy, int64_t* sz) {
    Vec3 c; double R;
    if (!globularInCell(gx, gy, gz, c, R)) return 0;
    const uint64_t h = hash3i(gx, gy, gz, DRIFT_SALT ^ 0x610BULL);
    int want = 0;   // a Poisson count of mean DRIFT_GLOBULAR_MEAN by the inverse of its distribution
    {
        double u = unitFromHash(h), p = std::exp(-DRIFT_GLOBULAR_MEAN), cum = p;
        while (u >= cum && want < DRIFT_GLOBULAR_MAX) { want++; p *= DRIFT_GLOBULAR_MEAN / want; cum += p; }
    }
    int n = 0;
    for (int k = 0; k < want; k++) {   // a way out from the cluster's middle, and the first sector along it with no star past a hashed depth
        const uint64_t hk = mix64(h ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(k + 1)));
        const double zc = 2 * unitFromHash(mix64(hk + 1)) - 1, ph = TAU * unitFromHash(mix64(hk + 2)), rc = std::sqrt(std::max(0.0, 1 - zc * zc));
        const Vec3 dir(rc * std::cos(ph), zc, rc * std::sin(ph));
        for (double r = R * (0.25 + 0.5 * unitFromHash(mix64(hk + 3))); r < R; r += 1.5) {
            const Vec3 p = c + dir * r;
            const int64_t x = (int64_t)std::floor(p.x), y = (int64_t)std::floor(p.y), z = (int64_t)std::floor(p.z);
            if (!sectorHoldsStar(x, y, z, galaxyDensity((double)x, (double)y, (double)z))) { addOnce(x, y, z, sx, sy, sz, n); break; }
        }
    }
    return n;
}

namespace {
void drifterPlace(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName) {
    const uint64_t seed = drifterSeed(sx, sy, sz);
    const StarClassInfo& ci = STAR_CLASSES[STAR_ROGUE];
    Rng rng(seed ^ 0xA2C40BULL);
    out = Star();
    out.sx = sx; out.sy = sy; out.sz = sz;
    out.pos = Vec3((sx + 0.1 + 0.8 * rng.uni()) * SECTOR_KM, (sy + 0.1 + 0.8 * rng.uni()) * SECTOR_KM, (sz + 0.1 + 0.8 * rng.uni()) * SECTOR_KM);
    out.cls = STAR_ROGUE;
    out.radiusKm = ci.radiusKm; out.luminosity = ci.luminosity; out.massFactor = ci.massFactor; out.color = ci.color;
    out.seed = seed;
    if (withName) {
        const int region = galaxyRegion(sx, sy, sz);
        out.name = generateName(seed, (region == REGION_CORE || region == REGION_HALO || region == REGION_BULGE || region == REGION_CLUSTER) ? 1 : ((region == REGION_NEBULA || region == REGION_ARM || region == REGION_OPEN) ? 2 : 0));
    }
    out.valid = true;
}
}

bool drifterInSector(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName) {
    out.valid = false;
    int64_t xs[DRIFT_GLOBULAR_MAX + DRIFT_CELL_MAX], ys[DRIFT_GLOBULAR_MAX + DRIFT_CELL_MAX], zs[DRIFT_GLOBULAR_MAX + DRIFT_CELL_MAX];
    int n = drifterCell(floorDiv(sx, DRIFT_CELL), floorDiv(sy, DRIFT_CELL), floorDiv(sz, DRIFT_CELL), xs, ys, zs);
    for (int k = 0; k < n; k++) if (xs[k] == sx && ys[k] == sy && zs[k] == sz) { drifterPlace(sx, sy, sz, out, withName); return true; }
    // within a globular: its own (the cluster's cell holds the whole of it, the sector's corner may lie a little outside its radius)
    const int64_t gx = (int64_t)std::floor(((double)sx - GALAXY_CENTRE_SX) / GLOBULAR_CELL), gy = (int64_t)std::floor((double)sy / GLOBULAR_CELL);
    const int64_t gz = (int64_t)std::floor(((double)sz - GALAXY_CENTRE_SZ) / GLOBULAR_CELL);
    Vec3 c; double R;
    if (globularInCell(gx, gy, gz, c, R) && length(Vec3(sx + 0.5, sy + 0.5, sz + 0.5) - c) < R + 2) {
        n = drifterGlobular(gx, gy, gz, xs, ys, zs);
        for (int k = 0; k < n; k++) if (xs[k] == sx && ys[k] == sy && zs[k] == sz) { drifterPlace(sx, sy, sz, out, withName); return true; }
    }
    return false;
}

void drifterAt(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName) { drifterPlace(sx, sy, sz, out, withName); }

bool placeInSector(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName) {
    return starInSector(sx, sy, sz, out, withName) || drifterInSector(sx, sy, sz, out, withName);
}

int driftersNear(const Vec3& obsKm, double reachLy, std::vector<Star>& out) {
    out.clear();
    const Vec3 o = obsKm / SECTOR_KM;
    const double r2 = reachLy * reachLy;
    std::vector<std::pair<double, Star>> found;
    int64_t xs[DRIFT_GLOBULAR_MAX + DRIFT_CELL_MAX], ys[DRIFT_GLOBULAR_MAX + DRIFT_CELL_MAX], zs[DRIFT_GLOBULAR_MAX + DRIFT_CELL_MAX];
    auto take = [&](int n) {
        for (int k = 0; k < n; k++) {
            const double dx = xs[k] + 0.5 - o.x, dy = ys[k] + 0.5 - o.y, dz = zs[k] + 0.5 - o.z;
            if (dx * dx + dy * dy + dz * dz > r2 + 3 * reachLy + 3) continue;   // a sector's anchor is within it
            Star s; drifterPlace(xs[k], ys[k], zs[k], s, false);
            const double d2 = length2(s.pos - obsKm) / (SECTOR_KM * SECTOR_KM);
            if (d2 > r2) continue;
            bool again = false;
            for (const auto& f : found) if (f.second.sx == s.sx && f.second.sy == s.sy && f.second.sz == s.sz) again = true;   // a cluster's and a cell's alike
            if (!again) found.push_back({d2, s});
        }
    };
    int looked = 0;
    const int64_t c0[3] = {floorDiv((int64_t)std::floor(o.x - reachLy), DRIFT_CELL), floorDiv((int64_t)std::floor(o.y - reachLy), DRIFT_CELL), floorDiv((int64_t)std::floor(o.z - reachLy), DRIFT_CELL)};
    const int64_t c1[3] = {floorDiv((int64_t)std::floor(o.x + reachLy), DRIFT_CELL), floorDiv((int64_t)std::floor(o.y + reachLy), DRIFT_CELL), floorDiv((int64_t)std::floor(o.z + reachLy), DRIFT_CELL)};
    for (int64_t cx = c0[0]; cx <= c1[0]; cx++)
        for (int64_t cy = c0[1]; cy <= c1[1]; cy++)
            for (int64_t cz = c0[2]; cz <= c1[2]; cz++) {
                // the cell's nearest point first: the cells the sphere does not reach are skipped
                const double nx = clampd(o.x, (double)(cx * DRIFT_CELL), (double)((cx + 1) * DRIFT_CELL)) - o.x;
                const double ny = clampd(o.y, (double)(cy * DRIFT_CELL), (double)((cy + 1) * DRIFT_CELL)) - o.y;
                const double nz = clampd(o.z, (double)(cz * DRIFT_CELL), (double)((cz + 1) * DRIFT_CELL)) - o.z;
                if (nx * nx + ny * ny + nz * nz > r2) continue;
                looked++;
                take(drifterCell(cx, cy, cz, xs, ys, zs));
            }
    const double gx0 = std::floor((o.x - GALAXY_CENTRE_SX - reachLy - 60) / GLOBULAR_CELL), gx1 = std::floor((o.x - GALAXY_CENTRE_SX + reachLy + 60) / GLOBULAR_CELL);
    const double gy0 = std::floor((o.y - reachLy - 60) / GLOBULAR_CELL), gy1 = std::floor((o.y + reachLy + 60) / GLOBULAR_CELL);
    const double gz0 = std::floor((o.z - GALAXY_CENTRE_SZ - reachLy - 60) / GLOBULAR_CELL), gz1 = std::floor((o.z - GALAXY_CENTRE_SZ + reachLy + 60) / GLOBULAR_CELL);
    for (double gx = gx0; gx <= gx1; gx++)
        for (double gy = gy0; gy <= gy1; gy++)
            for (double gz = gz0; gz <= gz1; gz++) {
                Vec3 c; double R;
                if (!globularInCell((int64_t)gx, (int64_t)gy, (int64_t)gz, c, R) || length(c - o) > reachLy + R) continue;
                looked++;
                take(drifterGlobular((int64_t)gx, (int64_t)gy, (int64_t)gz, xs, ys, zs));
            }
    std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (auto& f : found) out.push_back(f.second);
    return looked;
}

DrifterBody drifterBody(const Star& d) {
    DrifterBody b;
    Rng r(d.seed ^ 0xDA2CB0D1ULL);
    double w[64];
    for (int k = 0; k < DARK_KIND_COUNT; k++) w[k] = DARK_KINDS[k].weight;
    const DarkKind& D = DARK_KINDS[r.pick(w, DARK_KIND_COUNT)];
    const KindDef& K = KINDS[D.type];
    const double uR = r.uni(), uH = r.uni();
    b.type = b.drawnType = D.type;
    b.radiusKm = b.drawnRadiusKm = K.minRadiusKm + (K.maxRadiusKm - K.minRadiusKm) * uR;
    b.heatK = D.heatLo + (D.heatHi - D.heatLo) * uH;
    if (D.type == PT_ROCKY && unitFromHash(mix64(d.seed ^ KINDS[PT_DARK_OCEAN].salt)) < DARK_OCEAN_SHARE) {   // D-04: it kept its hydrogen
        const KindDef& O = KINDS[PT_DARK_OCEAN];
        b.type = PT_DARK_OCEAN;
        b.radiusKm = O.minRadiusKm + (O.maxRadiusKm - O.minRadiusKm) * uR;
        b.heatK = DARK_OCEAN_HEAT_LO + (DARK_OCEAN_HEAT_HI - DARK_OCEAN_HEAT_LO) * uH;
    }
    return b;
}

double drifterStandOffKm(const Star& d) { return drifterBody(d).radiusKm * (40.0 + 40.0 * unitFromHash(mix64(d.seed ^ 0x57A0FFULL))); }

// D-02: the base power by kind (`build/d02/reach.cpp` tried the law from home and the three C-series worlds: none at x1 or x5, the first a
// brown dwarf at 437 ly at x11, two at x18, four at x25, six or seven at x32, the farthest about 1,200 ly)
namespace {
double basePower(int type) {
    switch (type) {
        case PT_SUBSTELLAR: return 1.0;
        case PT_GASGIANT: return 0.6;
        case PT_TECTONIC: case PT_MOLTEN: return 0.12;
        case PT_METAL: return 0.10;
        case PT_ROCKY: case PT_DESERT: case PT_EUROPAN: case PT_FROZEN_FELISIAN: case PT_CARBON: return 0.05;
        default: return 0.02;
    }
}
constexpr double POWER_SPREAD_LO = 0.5, POWER_SPREAD_HI = 2.0;
}

double drifterPower(const Star& d) {
    const DrifterBody b = drifterBody(d);
    const KindDef& K = KINDS[b.drawnType];   // D-04: the kind it was drawn as (a dark ocean calls as the rocky drifter it was)
    const double spread = POWER_SPREAD_LO * std::pow(POWER_SPREAD_HI / POWER_SPREAD_LO, unitFromHash(mix64(d.seed ^ 0x9A0E5ULL)));
    return basePower(b.drawnType) * b.drawnRadiusKm / (0.5 * (K.minRadiusKm + K.maxRadiusKm)) * spread;
}

double drifterPowerMax() {
    static const double most = [] {
        double m = 0;
        for (int k = 0; k < DARK_KIND_COUNT; k++) {
            const KindDef& K = KINDS[DARK_KINDS[k].type];
            m = std::max(m, basePower(DARK_KINDS[k].type) * K.maxRadiusKm / (0.5 * (K.minRadiusKm + K.maxRadiusKm)) * POWER_SPREAD_HI);
        }
        return m;
    }();
    return most;
}

int drifterSunWayOf(uint64_t worldSeed) {
    static const double W[4] = {5, 2, 1, 2};   // the wanderer, the intruder, the dark, the dance (`SunWay`)
    Rng r(worldSeed ^ 0xD05A7ULL);
    return r.pick(W, 4);
}

namespace {
// D-05: the star a people's world circled, near p: the living suns of the cube of 21 sectors a side round it (else of 41), the nearest first;
// of the sixteen nearest the first whose system the way needs (`way` 0 a giant, 3 a companion), else the nearest (false: none)
bool formerSunNear(const Vec3& p, int way, Star& out) {
    const int64_t cx = (int64_t)std::floor(p.x / SECTOR_KM), cy = (int64_t)std::floor(p.y / SECTOR_KM), cz = (int64_t)std::floor(p.z / SECTOR_KM);
    for (int radius : {10, 20}) {
        std::vector<std::pair<double, Star>> found;
        for (int64_t x = cx - radius; x <= cx + radius; x++)
            for (int64_t y = cy - radius; y <= cy + radius; y++)
                for (int64_t z = cz - radius; z <= cz + radius; z++) {
                    Star s;
                    if (!starInSector(x, y, z, s, false) || !livingSun(s.cls)) continue;
                    found.push_back({length2(s.pos - p), s});
                }
        if (found.empty()) continue;
        std::sort(found.begin(), found.end(), [](const std::pair<double, Star>& a, const std::pair<double, Star>& b) { return a.first < b.first; });
        out = found[0].second;
        if (way != 0 && way != 3) return true;
        for (size_t k = 0; k < found.size() && k < 16; k++) {
            Star full; if (!starInSector(found[k].second.sx, found[k].second.sy, found[k].second.sz, full, true)) continue;
            StarSystem sys; sys.generate(full);
            bool fits = false;
            if (way == 3) fits = sys.companion >= 0;
            else for (const Body& b : sys.bodies) if (b.parent < 0 && (KINDS[b.type].tags & KT_GIANT)) fits = true;
            if (fits) { out = found[k].second; return true; }
        }
        return true;
    }
    return false;
}
}

DrifterPast drifterPast(const Star& d) {
    DrifterPast p;
    Rng r(d.seed ^ 0xDA2C9A57ULL);
    const double zc = 0.35 * (2 * r.uni() - 1), ph = TAU * r.uni();   // near the disc's plane, as the stars move
    p.dir = normalize(Vec3(std::cos(ph), zc, std::sin(ph)));
    p.years = std::exp(std::log(1500.0) + r.uni() * std::log(20000.0 / 1500.0));
    p.speedKms = 2 + 6 * r.uni();
    p.travelLy = p.speedKms * p.years * 3.15576e7 / SECTOR_KM;
    double shortLy = 0;   // the star nearest the way's end; where the end lies in the void, the first stars back along the way (`jumpTarget`)
    Star s;
    if (jumpTarget(d.pos, p.dir, p.travelLy, s, shortLy)) {
        // D-05: a drifter with a people left a sun its world could have circled, in a system its way of losing it needs (near the star the way reached)
        const DrifterBody db = drifterBody(d);
        const uint64_t ws = hashCombine(d.seed, 1000);   // the drifter's world's seed (`StarSystem::generate`'s)
        Star sun;
        if (drifterPeoplesHash(db.type, ws) && formerSunNear(s.pos, drifterSunWayOf(ws), sun)) s = sun;
        starInSector(s.sx, s.sy, s.sz, p.star, true); p.found = true;
    }
    return p;
}

double drifterSpeedKms(const Star& d) {
    Rng r(d.seed ^ 0xDA2C9A57ULL);
    (void)r.uni(); (void)r.uni(); (void)r.uni();
    return 2 + 6 * r.uni();
}

double drifterLeftYears(const Star& d) {
    Rng r(d.seed ^ 0xDA2C9A57ULL);
    (void)r.uni(); (void)r.uni();
    return std::exp(std::log(1500.0) + r.uni() * std::log(20000.0 / 1500.0));
}
