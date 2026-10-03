// C-01: the ruins of a world (see ruins.h). The monoliths of the old ones keep M4-07's draw to the bit (the same cells, kinds,
// styles and sizes as before, so nothing moved on the worlds that had no civilisation); a civilisation's settlements take
// the cells of its world that the monoliths left. A settlement is laid out once from the cell's hash: its class gives the
// number of buildings and the radius, its plan places them (a cluster, a street, rings round a centre, a grid of streets
// with a plaza), and every building draws its own pieces from its seed: walls that stand to a share of their height by
// its decay, a doorway with a lintel, a window, an inner wall, the columns of a hall, a roof where the culture built stone
// ones and the house is sound, rubble at the foot of a fallen wall.
#include "ruins.h"
#include "drainage.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <mutex>
#include <unordered_map>

const char* const SETTLEMENT_CLASS_NAMES[4] = {"HAMLET", "VILLAGE", "TOWN", "MONUMENT"};
const char* const BUILDING_KIND_NAMES[BK_COUNT] = {"HOUSE", "HALL", "TOWER", "ROTUNDA", "GATE", "PLATFORM", "COLONNADE", "STELA", "RUBBLE", "WALL", "QUAY", "MOLE", "BOLLARD", "TERRACE", "CISTERN", "WIND WALL", "CAIRN"};

bool worldHasRuins(const BodyGen& g) { return g.type == PT_FELISIAN || g.type == PT_QUARTZ || g.type == PT_OCEAN || g.hasTrait(TR_CIVILISATION); }
double ruinCellLat(const BodyGen& g) { return 2000.0 / (g.R * 1000.0); }

void ruinCellOf(const BodyGen& g, double lat, double lon, int& gLat, int& gLon) {
    double dLat = ruinCellLat(g);
    gLat = (int)std::floor(lat / dLat);
    double dLon = dLat / std::max(std::cos((gLat + 0.5) * dLat), 0.05);
    gLon = (int)std::floor(lon / dLon);
}
int ruinCellsAround(const BodyGen& g, int gLat) {
    double dLat = ruinCellLat(g);
    double dLon = dLat / std::max(std::cos((gLat + 0.5) * dLat), 0.05);
    return (int)std::ceil(TAU / dLon);
}

// C-08: the base decay from the age and the wet: a village of two centuries stands to its lintels, one of two millennia to
// half its walls, one of thirty millennia is foundations; the rain takes a fifth more
double decayOf(double ageYears, double wet) { return clampd(0.08 + 0.3 * std::log10(std::max(100.0, ageYears) / 100.0) + 0.2 * wet, 0.05, 0.95); }

// C-13: the peoples of a world (see ruins.h)
int peoplesOf(const BodyGen& g) { return g.hasTrait(TR_CIVILISATION) ? (unitFromHash(mix64(g.seed ^ 0xC13ULL)) < 0.2 ? 2 : 1) : 0; }
BodyGen peopleGen(const BodyGen& g, int people) { BodyGen pg = g; if (people > 0) pg.seed = mix64(g.seed ^ (0xC13A5ULL + (uint64_t)people * 0x9E3779B97F4A7C15ULL)); return pg; }
bool peoplesEndedTogether(const BodyGen& g) { return unitFromHash(mix64(g.seed ^ 0xC13E0ULL)) < 0.5; }
namespace {
std::mutex g_divideMutex;
std::unordered_map<uint64_t, Vec3> g_divides;   // a world's divide, by its seed (two threads may find the same one: they agree)
}
// the great circle: twelve poles drawn from the seed, forty-eight points along each sampled at 2 km (the sea, the inland water
// and a dead desert's old sea count as water), the one crossing the least land kept
Vec3 peopleDivide(const BodyGen& g) {
    { std::lock_guard<std::mutex> lock(g_divideMutex); auto it = g_divides.find(g.seed); if (it != g_divides.end()) return it->second; }
    DrainageOff off;
    Rng rng(mix64(g.seed ^ 0xD1F1DEULL));
    Vec3 best(0, 0, 1); int bestLand = 1 << 30;
    for (int c = 0; c < 12; c++) {
        double z = rng.range(-1, 1), a = rng.uni() * TAU, r = std::sqrt(std::max(0.0, 1 - z * z));
        Vec3 n(r * std::cos(a), r * std::sin(a), z);
        Vec3 e = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), f = cross(n, e);
        int land = 0;
        for (int k = 0; k < 48; k++) {
            double t = k * TAU / 48;
            SurfaceSample s = sampleSurface(g, normalize(e * std::cos(t) + f * std::sin(t)), 2048.0);
            if (!(s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5)) land++;
        }
        if (land < bestLand) { bestLand = land; best = n; }
    }
    std::lock_guard<std::mutex> lock(g_divideMutex);
    if (g_divides.size() > 4096) g_divides.clear();
    g_divides[g.seed] = best;
    return best;
}
int peopleAt(const BodyGen& g, double lat, double lon) {
    if (peoplesOf(g) < 2) return 0;
    return dot(StarSystem::bodyFromLatLon(lat, lon), peopleDivide(g)) >= 0 ? 0 : 1;
}

Culture cultureOf(const BodyGen& g, int people) {
    Culture c;
    const BodyGen pg = peopleGen(g, people);   // C-13: the people's draws (the world's own for the first)
    c.people = people;
    Rng r(pg.seed ^ 0xC01DULL);
    c.style = r.chance(0.55) ? 0 : 1;
    c.family = r.chance(g.type == PT_DESERT ? 0.6 : 0.2) ? 1 : 0;
    c.tall = 0.75 + 0.55 * r.uni();
    double uDecay = r.uni();   // C-01's draw kept in the stream; the decay is the age's now
    c.stoneRoofs = r.chance(0.55);
    c.plan = r.chance(0.6) ? SP_GRID : SP_RADIAL;
    c.towers = r.chance(0.7);
    double uBuried = r.uni();
    // C-08 weathering by age: the years since the end (200 to 40,000, log-uniform) and the world's wet; the walls sink the deeper the older
    c.ageYears = std::exp(std::log(200.0) + unitFromHash(mix64(pg.seed ^ 0xA6EDULL)) * std::log(200.0));
    if (people > 0 && peoplesEndedTogether(g)) c.ageYears = cultureOf(g, 0).ageYears;   // C-13: one catastrophe for both
    c.wet = g.type == PT_DESERT ? 0.05 + 0.15 * uDecay : 0.5 + 0.5 * uDecay;
    c.decay = decayOf(c.ageYears, c.wet);
    double ageK = clampd(std::log10(c.ageYears / 100.0) / 2.6, 0.25, 1.0);
    c.buried = (g.type == PT_DESERT ? 0.6 + 1.2 * uBuried : 0.15 + 0.4 * uBuried) * ageK;
    // C-09 the roads, by hashes of their own (the stream above stays): a desert people beat tracks as often as it paves
    c.paved = unitFromHash(mix64(pg.seed ^ 0xC09AULL)) < (g.type == PT_DESERT ? 0.5 : 0.7);
    c.roadHalf = 2.5 + 1.5 * unitFromHash(mix64(pg.seed ^ 0xC09BULL));
    c.roadWear = clampd(0.1 + 0.55 * ageK * (0.35 + 0.65 * c.wet), 0.1, 0.6);
    return c;
}

namespace {
// the class's size: the number of buildings and the radius, the first draws of the layout's stream (the landmark finder
// reads the radius without the layout)
void settlementSize(RuinSpec& r, Rng& rng, int& n, double& R) {
    switch (r.sclass) {
        case SC_HAMLET: n = 3 + rng.irange(4); R = 22 + 12 * rng.uni(); r.plan = rng.chance(0.6) ? SP_CLUSTER : SP_STREET; break;
        case SC_VILLAGE: n = 7 + rng.irange(8); R = 45 + 25 * rng.uni(); r.plan = rng.chance(0.5) ? SP_STREET : SP_RADIAL; break;
        default: n = 24 + rng.irange(21); R = 60 + 30 * rng.uni(); break;   // the plan is the culture's
    }
}

struct Layout {
    RuinSpec& r; const Culture& c; Rng rng; double decayS = 0.5;
    Layout(RuinSpec& rr, const Culture& cc) : r(rr), c(cc), rng(rr.seed ^ 0x5E77ULL) {}
    bool fits(double x, double z, double rad) const {
        for (const Building& b : r.buildings) {
            double dx = b.x - x, dz = b.z - z, rb = std::sqrt(b.hw * b.hw + b.hd * b.hd) + 1.2;
            if (dx * dx + dz * dz < (rad + rb) * (rad + rb)) return false;
        }
        return true;
    }
    bool add(int kind, double x, double z, double heading, double hw, double hd, double hWall, double thick, bool checkFit = true) {
        double rad = std::sqrt(hw * hw + hd * hd) + 1.2;
        if (checkFit && !fits(x, z, rad)) return false;
        Building b; b.kind = kind; b.x = x; b.z = z; b.heading = heading; b.hw = hw; b.hd = hd; b.hWall = hWall * c.tall; b.thick = thick;
        b.decay = clampd(decayS + rng.sym(0.25), 0.02, 0.98);
        b.door = kind == BK_HOUSE ? rng.irange(4) : 0;
        b.seed = rng.next();
        r.buildings.push_back(b);
        return true;
    }
    // a dwelling at a spot: mostly houses, a rotunda now and then, a heap where one fell
    bool dwelling(double x, double z, double heading) {
        double u = rng.uni();
        if (u < 0.08) return add(BK_RUBBLE, x, z, heading, 3.5, 3.5, 1.0, 0.5);
        if (u < 0.2) { double rr = 3.0 + 2.5 * rng.uni(); return add(BK_ROTUNDA, x, z, heading, rr, rr, 2.8 + 1.0 * rng.uni(), 0.5); }
        return add(BK_HOUSE, x, z, heading, 2.4 + 1.6 * rng.uni(), 3.0 + 2.0 * rng.uni(), 2.6 + 0.8 * rng.uni(), 0.45);
    }
};

// C-08: the terrain's features. A harbour where a shore is within reach (a quay along the shore, a mole out from its end into the
// water, bollards, a warehouse behind; on a dead desert world the harbour stands over the dry bed), terraces where the ground
// slopes (retaining walls along the contour between the houses, a wall's height apart), on a desert a cistern at the upslope
// edge with its channel and a crescent of wall against the wind, and cairns out along the ridge where the relief is mountain
bool clearOfBuildings(const RuinSpec& r, double x, double z, double margin) {
    for (const Building& b : r.buildings) {
        double dx = x - b.x, dz = z - b.z;
        double lx = dx * std::cos(b.heading) - dz * std::sin(b.heading), lz = dx * std::sin(b.heading) + dz * std::cos(b.heading);
        if (std::fabs(lx) < b.hw + margin && std::fabs(lz) < b.hd + margin) return false;
    }
    return true;
}
// a wall segment of half length `half` across heading `h` (its axis along x') is clear along its whole length (nine points)
bool segmentClear(const RuinSpec& r, double x, double z, double h, double half, double margin) {
    double ax = std::cos(h), az = -std::sin(h);
    for (int k = -4; k <= 4; k++) { double t = half * k / 4.0; if (!clearOfBuildings(r, x + ax * t, z + az * t, margin)) return false; }
    return true;
}
void addFeatures(Layout& L, RuinSpec& r, const Culture& c) {
    Rng& rng = L.rng;
    const SiteRead& st = r.site;
    const double R = r.size;
    bool big = r.sclass == SC_VILLAGE || r.sclass == SC_TOWN;
    // the harbour
    if (st.shoreDist < 500 + R && (big || rng.chance(0.5)) && r.sclass != SC_MONUMENT) {
        double sx = std::sin(st.shoreDir), sz = std::cos(st.shoreDir);
        double q = st.shoreDist - 3.0, along = st.shoreDir + PI / 2;
        double hw = 10 + 12 * rng.uni();
        double end = rng.chance(0.5) ? 1 : -1, len = 12 + 12 * rng.uni();
        if (segmentClear(r, sx * q, sz * q, along, hw, 2.8 + 1.5) && L.add(BK_QUAY, sx * q, sz * q, along, hw, 2.8, 1.2, 0.5, false)) {   // the quay clear of the houses along its length (its own half width and a margin)
            r.harbour = true;
            double ex = sx * q + std::sin(along) * end * (hw - 1.5), ez = sz * q + std::cos(along) * end * (hw - 1.5);
            L.add(BK_MOLE, ex + sx * (2.8 + len * 0.5), ez + sz * (2.8 + len * 0.5), st.shoreDir, 1.0, len * 0.5, 1.6, 0.9, false);
            double wq = q - 2.8 - 7.5;
            if (wq > 6) L.add(BK_HALL, sx * wq, sz * wq, st.shoreDir, 4.5 + 1.5 * rng.uni(), 5.5 + 1.5 * rng.uni(), 4 + 1.5 * rng.uni(), 0.6);
        }
    }
    // the terraces
    if (st.slope > 0.06 && r.sclass != SC_MONUMENT) {
        double step = clampd(1.4 / st.slope, 8, 30), h = 1.3 + 0.6 * rng.uni();
        double dx = std::sin(st.downhill), dz = std::cos(st.downhill), ax = std::cos(st.downhill), az = -std::sin(st.downhill);
        double d0 = -std::floor(R / step) * step + step * rng.uni();
        int segs = 0;
        for (double d = d0; d < R; d += step) {
            double w = std::sqrt(std::max(0.0, R * R - d * d)) * 0.9;
            for (double tt = -w + 3; tt < w - 3; tt += 6.2) {
                double x = dx * d + ax * tt, z = dz * d + az * tt;
                if (!segmentClear(r, x, z, st.downhill, 3.0, 1.2)) continue;   // the whole segment, not only its middle
                if (L.add(BK_TERRACE, x, z, st.downhill, 3.0, 0.4, h, 0.4, false)) segs++;
            }
        }
        r.terraced = segs > 0;
    }
    // a desert's cistern and its wind wall
    if (st.desert && r.sclass != SC_MONUMENT) {
        if (rng.chance(big ? 0.8 : 0.4)) {
            double head = st.slope > 0.02 ? st.downhill + PI : rng.uni() * TAU;
            for (int tries = 0; tries < 6 && !r.cistern; tries++) {
                double a = head + rng.sym(0.5), rr = R * (0.75 + 0.2 * rng.uni()), rad = 2.5 + 1.5 * rng.uni();
                if (L.add(BK_CISTERN, std::sin(a) * rr, std::cos(a) * rr, a, rad, rad, 2.0, 0.7)) r.cistern = true;
            }
        }
        if (rng.chance(big ? 0.7 : 0.35)) {
            double rw = R + 14, hWall = 2.2 + 1.0 * rng.uni();
            int segs = std::max(4, (int)(rw * 1.9 / 7.4)), placed = 0;
            for (int k = 0; k < segs; k++) {
                double a = st.wind + (-0.95 + 1.9 * (k + 0.5) / segs);
                if (rng.chance(0.15)) continue;   // fallen
                if (L.add(BK_WINDWALL, std::sin(a) * rw, std::cos(a) * rw, a + PI / 2, 3.5, 0.5, hWall, 0.5, false)) placed++;
            }
            r.windwall = placed > 0;
        }
    }
    // the cairns on the ridge
    if (st.relief > 0.45 && rng.chance(0.7)) {
        int n = 3 + rng.irange(4), placed = 0;
        for (int k = 0; k < n; k++) {
            double d = R + 12 + 14 * k + rng.sym(3), a = st.ridgeDir + rng.sym(0.12);
            if (L.add(BK_CAIRN, std::sin(a) * d, std::cos(a) * d, a, 1.1, 1.1, 2.1, 0.5, false)) placed++;
        }
        r.cairns = placed > 0;
    }
}

void layoutSettlement(RuinSpec& r, const Culture& c, const BodyGen& g) {
    Layout L(r, c);
    Rng& rng = L.rng;
    int n = 1; double R = 12;
    if (r.sclass == SC_MONUMENT) r.plan = SP_LONE;
    else { settlementSize(r, rng, n, R); if (r.sclass == SC_TOWN) r.plan = c.plan; }
    r.size = R;
    r.site = readSite(g, r);   // C-08: the terrain round it, read before anything is placed (no draw of the layout's stream)
    L.decayS = clampd(c.decay + rng.sym(0.15), 0.05, 0.95);
    const double h0 = r.heading;
    const double fx = std::sin(h0), fz = std::cos(h0), sx = std::cos(h0), sz = -std::sin(h0);   // along the heading, and across it
    r.walled = r.sclass == SC_TOWN && rng.chance(0.75);
    switch (r.plan) {
        case SP_LONE: {   // one monument alone: a platform with its shrine, a tower, a rotunda, or a pair of stelae
            double u = rng.uni();
            if (u < 0.4) L.add(BK_PLATFORM, 0, 0, h0, 5 + 3 * rng.uni(), 5 + 3 * rng.uni(), 3.3, 0.5, false);
            else if (u < 0.6 && c.towers) L.add(BK_TOWER, 0, 0, h0, 2.4 + 1.0 * rng.uni(), 2.4 + 1.0 * rng.uni(), 10 + 7 * rng.uni(), 0.7, false);
            else if (u < 0.8) { double rr = 4.0 + 2.0 * rng.uni(); L.add(BK_ROTUNDA, 0, 0, h0, rr, rr, 3.0 + 1.0 * rng.uni(), 0.5, false); }
            else { L.add(BK_STELA, -2.5, 0, h0, 0.45, 0.25, 3.5 + 2.0 * rng.uni(), 0.3, false); L.add(BK_STELA, 2.5, 0, h0, 0.45, 0.25, 3.5 + 2.0 * rng.uni(), 0.3, false); }
            break;
        }
        case SP_CLUSTER:
            for (int k = 0; k < n; k++)
                for (int tries = 0; tries < 20; tries++) {
                    double a = rng.uni() * TAU, rr = R * 0.85 * std::sqrt(rng.uni());
                    if (L.dwelling(std::sin(a) * rr, std::cos(a) * rr, rng.uni() * TAU)) break;
                }
            break;
        case SP_STREET: {
            // a street through the centre along the heading; the dwellings on both sides, their fronts to it
            double t = -R * 0.9;
            int placed = 0;
            while (t < R * 0.9 && placed < n) {
                for (int side = -1; side <= 1; side += 2) {
                    if (placed >= n) break;
                    if (rng.chance(0.2)) continue;   // a gap
                    double off = 7.5 + 1.5 * rng.uni();
                    if (L.dwelling(fx * t + sx * side * off, fz * t + sz * side * off, h0 - side * PI / 2)) placed++;
                }
                t += 9.0 + 4.0 * rng.uni();
            }
            if (r.sclass == SC_VILLAGE && rng.chance(0.5)) L.add(BK_STELA, fx * (R * 0.95), fz * (R * 0.95), h0, 0.45, 0.25, 3.0 + 2.0 * rng.uni(), 0.3);
            break;
        }
        case SP_RADIAL: {
            // rings round a centre: a platform or a hall in the middle, the dwellings facing in
            if (rng.uni() < 0.5) L.add(BK_PLATFORM, 0, 0, h0, 6 + 3 * rng.uni(), 6 + 3 * rng.uni(), 3.3, 0.5);
            else L.add(BK_HALL, 0, 0, h0, 5 + 2 * rng.uni(), 8 + 4 * rng.uni(), 5 + 2 * rng.uni(), 0.7);
            int rings = r.sclass == SC_TOWN ? 2 : 1;
            int placed = 0;
            for (int ring = 0; ring < rings && placed < n; ring++) {
                double rr = R * (rings == 1 ? 0.6 : (ring == 0 ? 0.45 : 0.82));
                int m = std::max(4, (int)(TAU * rr / 12.0));
                double a0 = rng.uni() * TAU;
                for (int k = 0; k < m && placed < n; k++) {
                    if (rng.chance(0.15)) continue;
                    double a = a0 + k * TAU / m + rng.sym(0.3 / m);
                    if (ring == 1 && c.towers && placed < n - 1 && rng.chance(0.08)) {
                        if (L.add(BK_TOWER, std::sin(a) * rr, std::cos(a) * rr, a + PI, 2.2 + 1.0 * rng.uni(), 2.2 + 1.0 * rng.uni(), 9 + 7 * rng.uni(), 0.7)) placed++;
                        continue;
                    }
                    if (L.dwelling(std::sin(a) * rr, std::cos(a) * rr, a + PI)) placed++;
                }
            }
            break;
        }
        case SP_GRID: {
            // streets along the heading and across it, blocks of 17 x 15 m; the plaza at the centre with a platform, a
            // colonnade and stelae; a hall beside it; a tower or two toward the edge
            double bx = 17.0 + 3.0 * rng.uni(), bz = 15.0 + 3.0 * rng.uni();
            L.add(BK_PLATFORM, 0, 0, h0, 5 + 2 * rng.uni(), 5 + 2 * rng.uni(), 3.3, 0.5, false);
            L.add(BK_COLONNADE, sx * bx * 0.75, sz * bx * 0.75, h0, 0.5, bz * 0.6, 3.5, 0.4, false);
            if (rng.chance(0.6)) L.add(BK_STELA, -sx * bx * 0.5 + fx * bz * 0.55, -sz * bx * 0.5 + fz * bz * 0.55, h0, 0.45, 0.25, 3.0 + 2.0 * rng.uni(), 0.3, false);
            if (rng.chance(0.6)) L.add(BK_STELA, -sx * bx * 0.5 - fx * bz * 0.55, -sz * bx * 0.5 - fz * bz * 0.55, h0, 0.45, 0.25, 3.0 + 2.0 * rng.uni(), 0.3, false);
            int nx = (int)std::ceil(R / bx), nz = (int)std::ceil(R / bz);
            std::vector<std::pair<double, std::pair<int, int>>> nodes;
            for (int i = -nx; i <= nx; i++)
                for (int j = -nz; j <= nz; j++) {
                    if (i == 0 && j == 0) continue;
                    double d = std::sqrt(i * bx * i * bx + j * bz * j * bz);
                    if (d > R * 0.95) continue;
                    nodes.push_back({d + rng.uni() * 4.0, {i, j}});
                }
            std::sort(nodes.begin(), nodes.end());
            bool hallDone = false; int towersDone = 0, placed = 0;
            for (const auto& nd : nodes) {
                if (placed >= n) break;
                int i = nd.second.first, j = nd.second.second;
                if (rng.chance(0.12)) continue;   // a yard, or a house gone to nothing
                double x = sx * i * bx + fx * j * bz, z = sz * i * bx + fz * j * bz;
                double head = std::abs(i) >= std::abs(j) ? (i > 0 ? h0 - PI / 2 : h0 + PI / 2) : (j > 0 ? h0 + PI : h0);   // the fronts to the centre
                if (!hallDone && std::abs(i) + std::abs(j) == 1) {
                    hallDone = L.add(BK_HALL, x, z, head, 5 + 1.5 * rng.uni(), 6.0 + 1.5 * rng.uni(), 5 + 2 * rng.uni(), 0.7);
                    if (hallDone) { placed++; continue; }
                }
                if (c.towers && towersDone < 2 && nd.first > R * 0.5 && rng.chance(0.3)) {
                    if (L.add(BK_TOWER, x, z, head, 2.2 + 1.0 * rng.uni(), 2.2 + 1.0 * rng.uni(), 9 + 7 * rng.uni(), 0.7)) { towersDone++; placed++; continue; }
                }
                if (L.dwelling(x, z, head)) placed++;
            }
            break;
        }
    }
    if (r.walled) {   // the town wall: a ring of segments, a stretch fallen here and there, a gate at each end of the street's axis
        double reach = 0;   // C-08: outside every building (a wall through a house blocked its doorway on the desert test world)
        for (const Building& b : r.buildings) reach = std::max(reach, std::sqrt(b.x * b.x + b.z * b.z) + std::sqrt(b.hw * b.hw + b.hd * b.hd));
        double rw = std::max(R * 1.04, reach + 3.0);
        int segs = 10 + rng.irange(5);
        double hWall = 4.0 + 2.0 * rng.uni();
        std::vector<double> rad(segs);
        for (int k = 0; k < segs; k++) rad[k] = rw * (0.96 + 0.08 * rng.uni());
        for (int k = 0; k < segs; k++) {
            double aA = h0 + (k + 0.5) * TAU / segs, aB = h0 + (k + 1.5) * TAU / segs;
            double xA = std::sin(aA) * rad[k], zA = std::cos(aA) * rad[k], xB = std::sin(aB) * rad[(k + 1) % segs], zB = std::cos(aB) * rad[(k + 1) % segs];
            double mx = (xA + xB) * 0.5, mz = (zA + zB) * 0.5, dx = xB - xA, dz = zB - zA, len = std::sqrt(dx * dx + dz * dz);
            double wh = std::atan2(dx, dz) + PI / 2;   // the wall runs across its heading
            double am = h0 + (k + 1.0) * TAU / segs;
            double gA = std::fabs(std::remainder(am - h0, TAU)), gB = std::fabs(std::remainder(am - h0 - PI, TAU));
            bool gate = std::min(gA, gB) < PI / segs;
            if (!gate && rng.chance(0.12)) continue;   // a fallen stretch
            if (gate) {
                double ux = dx / len, uz = dz / len, q = len * 0.25 + 1.5;
                L.add(BK_WALL, mx - ux * q, mz - uz * q, wh, len * 0.25 - 1.5, 0.45, hWall, 0.9, false);
                L.add(BK_WALL, mx + ux * q, mz + uz * q, wh, len * 0.25 - 1.5, 0.45, hWall, 0.9, false);
                L.add(BK_GATE, mx, mz, wh, 1.9, 0.6, hWall + 1.0, 0.8, false);
            } else {
                L.add(BK_WALL, mx, mz, wh, len * 0.5, 0.45, hWall, 0.9, false);
            }
        }
    }
    addFeatures(L, r, c);   // C-08: after the plan and the wall, so the earlier draws stay where C-01 left them
}
}   // namespace

bool settlementClear(const RuinSpec& r, double x, double z, double margin) { return clearOfBuildings(r, x, z, margin); }   // C-12

// C-08: the terrain round a settlement. The planet function at 64 m (no tile: the drainage is off here) at the centre and in
// sixteen directions at the radius (the slope and its fall, the highest ground), then out to 240 m past the edge for the
// nearest water or old sea out to 500 m past the edge (the shore bisected between the last dry sample and the first wet); the wind by the dune fields'
// rule (the world's angle, flipped per hemisphere, turned per 30-degree band) as a heading
SiteRead readSite(const BodyGen& g, const RuinSpec& r) {
    DrainageOff off;
    SiteRead st;
    st.desert = g.type == PT_DESERT;
    const double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(r.lat), 0.05);
    auto at = [&](double east, double north) { return sampleSurface(g, StarSystem::bodyFromLatLon(r.lat + north / mPerRad, r.lon + east / (mPerRad * cosLat)), 64.0); };
    auto wet = [](const SurfaceSample& q) { return q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water) || q.oldSea > 0.5; };
    SurfaceSample c0 = at(0, 0);
    st.relief = c0.relief;
    double R = std::max(r.size, 12.0), sumX = 0, sumZ = 0, hBest = -1e18;
    for (int k = 0; k < 16; k++) {
        double a = k * TAU / 16, ex = std::sin(a) * R, nz = std::cos(a) * R;
        SurfaceSample q = at(ex, nz);
        double dh = q.height - c0.height;
        sumX += dh * std::sin(a); sumZ += dh * std::cos(a);   // the gradient's direction (the rise)
        if (q.height > hBest) { hBest = q.height; st.ridgeDir = a; }
    }
    double gx = sumX / 8.0 / R, gz = sumZ / 8.0 / R;   // rise over run toward the high side
    st.slope = std::sqrt(gx * gx + gz * gz);
    st.downhill = std::atan2(-gx, -gz);
    // the shore: rings out from the edge
    static const double RINGS[7] = {20, 60, 110, 170, 240, 340, 500};   // out to half a kilometre: a village's port is a walk away
    double bestD = 1e9, bestA = 0; bool dry = false;
    for (int ring = 0; ring < 7 && bestD > 1e8; ring++)
        for (int k = 0; k < 16; k++) {
            double a = k * TAU / 16, d = R + RINGS[ring];
            SurfaceSample q = at(std::sin(a) * d, std::cos(a) * d);
            if (!wet(q)) continue;
            double lo = ring == 0 ? R : R + RINGS[ring - 1], hi = d;   // the last dry ring and this one: bisect six times
            for (int it = 0; it < 6; it++) { double mid = 0.5 * (lo + hi); if (wet(at(std::sin(a) * mid, std::cos(a) * mid))) hi = mid; else lo = mid; }
            if (hi < bestD) { bestD = hi; bestA = a; dry = q.oldSea > 0.5 && q.material != MAT_WATER; }
        }
    if (bestD < 1e8) { st.shoreDist = bestD; st.shoreDir = bestA; st.shoreDry = dry; }
    int band = (int)std::floor(std::fabs(r.lat) / (30 * DEG));
    st.wind = g.windAngle + (r.lat < 0 ? PI : 0) + band * 0.9;
    return st;
}

std::string featureList(const RuinSpec& r) {
    std::string s;
    auto add = [&](const char* w) { if (!s.empty()) s += ", "; s += w; };
    if (r.harbour) add(r.site.shoreDry ? "a harbour on the dry shore" : "a harbour");
    if (r.terraced) add("terraces");
    if (r.cistern) add("a cistern");
    if (r.windwall) add("a wind wall");
    if (r.cairns) add("cairns");
    return s;
}

bool ruinOfCell(const BodyGen& g, int gLat, int gLon, RuinSpec& out, bool withBuildings) {
    if (!worldHasRuins(g)) return false;
    double dLat = ruinCellLat(g);
    double latc = (gLat + 0.5) * dLat;
    if (std::fabs(latc) > PI / 2 - dLat) return false;
    double dLon = dLat / std::max(std::cos(latc), 0.05);
    int nLon = (int)std::ceil(TAU / dLon);
    int gl = ((gLon % nLon) + nLon) % nLon;
    uint64_t h = hash2i(gLat, gl, g.seed ^ 0x2711);
    double u = unitFromHash(h);
    bool civ = g.hasTrait(TR_CIVILISATION);
    bool oldOnes = g.type == PT_FELISIAN || g.type == PT_QUARTZ || g.type == PT_OCEAN;
    if (u > 0.05 && (!civ || u > 0.35)) return false;
    out.buildings.clear();
    out.lat = (gLat + 0.2 + 0.6 * unitFromHash(mix64(h + 1))) * dLat;
    out.lon = (gl + 0.2 + 0.6 * unitFromHash(mix64(h + 2))) * dLon;
    out.heading = unitFromHash(mix64(h + 3)) * TAU;
    out.seed = h;
    out.sclass = 0; out.plan = 0; out.walled = false;
    if (u <= 0.05 && oldOnes) {   // the monoliths of the old ones (M4-07's draw): a felisian world that had a people of its own keeps them
        out.kind = unitFromHash(mix64(h + 4)) < 0.033 ? RK_GIANT_CUBE : (int)(unitFromHash(mix64(h + 5)) * 4);
        out.style = (int)(unitFromHash(mix64(h + 6)) * 3);
        out.size = out.kind == RK_GIANT_CUBE ? 40.0 : 6.0 + 8.0 * unitFromHash(mix64(h + 7));
        return true;
    }
    if (!civ) return false;
    out.kind = RK_SETTLEMENT;
    // a lone monument in the cells the old ones' monoliths take on a felisian world (5%), hamlets in 16% of the cells, villages in 9%, towns in 5%
    out.sclass = u <= 0.05 ? SC_MONUMENT : (u < 0.21 ? SC_HAMLET : (u < 0.30 ? SC_VILLAGE : SC_TOWN));
    out.people = peopleAt(g, out.lat, out.lon);   // C-13: whose side of the divide the settlement stands on
    Culture c = cultureOf(g, out.people);
    out.style = c.style;
    if (withBuildings) layoutSettlement(out, c, g);
    else if (out.sclass == SC_MONUMENT) { out.plan = SP_LONE; out.size = 12; }
    else { Rng rng(out.seed ^ 0x5E77ULL); int n; settlementSize(out, rng, n, out.size); if (out.sclass == SC_TOWN) out.plan = c.plan; }
    return true;
}

bool ruinSiteOk(const BodyGen& g, const RuinSpec& r) {
    SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(r.lat, r.lon), 64.0);
    if (s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water)) return false;
    if (r.kind != RK_SETTLEMENT) return true;
    if (s.oldSea > 0.5) return false;   // nobody lived on the sea's floor while it was a sea
    double dAng = r.size / (g.R * 1000.0), hmin = s.height, hmax = s.height;
    for (int k = 0; k < 4; k++) {
        double a = k * PI / 2;
        double lat = r.lat + std::cos(a) * dAng, lon = r.lon + std::sin(a) * dAng / std::max(std::cos(r.lat), 0.05);
        SurfaceSample q = sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), 64.0);
        if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water) || q.oldSea > 0.5) return false;
        hmin = std::min(hmin, q.height); hmax = std::max(hmax, q.height);
    }
    return hmax - hmin < 0.5 * r.size;   // a grade of a quarter across the settlement at most
}

namespace {
inline double u01(uint64_t seed, int k) { return unitFromHash(mix64(seed ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(k + 1)))); }
struct Emit {
    std::vector<RuinElem>& out; int bi;
    void box(double x, double z, double heading, double hx, double hz, double y0, double y1, int part, bool glyphs = false) {
        RuinElem e; e.shape = 0; e.x = x; e.z = z; e.heading = heading; e.hx = hx; e.hz = hz; e.y0 = y0; e.y1 = y1; e.part = part; e.glyphs = glyphs; e.building = bi;
        out.push_back(e);
    }
};
// a building's frame: x' across the heading (s), z' along it (f), in metres of the settlement
struct Frame {
    double x, z, sx, sz, fx, fz;
    explicit Frame(const Building& b) : x(b.x), z(b.z), sx(std::cos(b.heading)), sz(-std::sin(b.heading)), fx(std::sin(b.heading)), fz(std::cos(b.heading)) {}
    void to(double lx, double lz, double& ox, double& oz) const { ox = x + sx * lx + fx * lz; oz = z + sz * lx + fz * lz; }
};

void elemsBuilding(const Building& b, const Culture& c, const SiteRead& st, int lod, std::vector<RuinElem>& out, int bi) {
    Emit E{out, bi};
    Frame F(b);
    auto U = [&](int k) { return u01(b.seed, k); };
    const double sink = -c.buried * b.decay;   // the walls sank into the ground by this much
    auto standing = [&](int side) { return 1 - b.decay * (0.15 + 0.85 * U(10 + side)); };   // the share of a wall's height that stands
    // C-08: on a desert the sand drifts against the walls the wind meets (the sides whose outward normal faces within 60 degrees
    // of where the wind comes from), a low wedge the age has built up; part 7, drawn in the sand's bank
    auto drifts = [&](double hw, double hd) {
        if (!st.desert || lod != 0 || c.buried < 0.15) return;
        double hDrift = 0.25 + 0.6 * std::min(1.0, c.buried / 1.2) * (0.5 + 0.5 * b.decay);
        for (int side = 0; side < 4; side++) {
            double nAng = b.heading + side * PI / 2;   // the side's outward normal: 0 front (+z'), 1 right (+x'), 2 back, 3 left
            if (std::fabs(std::remainder(nAng - st.wind, TAU)) > PI / 3) continue;
            bool along = side == 0 || side == 2; double L = along ? hw : hd;
            double out = b.thick * 0.5 + 0.95;   // from the wall's outer face outward (clear of the wall's centre line, where the doorway is read)
            double lx = side == 1 ? hw + out : (side == 3 ? -hw - out : 0), lz = side == 0 ? hd + out : (side == 2 ? -hd - out : 0);
            double cx, cz; F.to(lx, lz, cx, cz);
            E.box(cx, cz, along ? b.heading : b.heading + PI / 2, L, 0.9, 0, hDrift, 7);
        }
    };
    switch (b.kind) {
        case BK_HOUSE: case BK_HALL: case BK_TOWER: {
            if (lod == 2) {
                double hm = 0; for (int s = 0; s < 4; s++) hm += standing(s);
                E.box(b.x, b.z, b.heading, b.hw, b.hd, sink, sink + std::max(0.4, b.hWall * hm * 0.25), 0);
                break;
            }
            double hs[4];
            for (int s = 0; s < 4; s++) hs[s] = std::max(0.35, b.hWall * standing(s));
            const double doorL = (b.door == 0 || b.door == 2) ? b.hw : b.hd, doorPos = (doorL - 1.6) * (U(20) - 0.5) * 1.2;   // the doorway's place along its wall
            double doorX = 0, doorZ = 0;   // the doorway's midpoint in the building's frame (no rubble lands in it)
            // the walls' axes run +x' on the front and the back and -z' on the right and the left, so the gap at `doorPos` along the axis
            // sits at x' = doorPos on sides 0 and 2 and at z' = -doorPos on sides 1 and 3 (C-08: sides 2 and 3 were mirrored since C-01, so
            // the rubble kept out of the wrong spot and could lie in the doorway)
            switch (b.door) { case 0: doorX = doorPos; doorZ = b.hd; break; case 2: doorX = doorPos; doorZ = -b.hd; break; case 1: doorX = b.hw; doorZ = -doorPos; break; default: doorX = -b.hw; doorZ = -doorPos; break; }
            for (int side = 0; side < 4; side++) {
                // side 0 the front (+z'), 1 the right (+x'), 2 the back (-z'), 3 the left (-x')
                bool along = side == 0 || side == 2;
                double L = along ? b.hw : b.hd;
                double lx = side == 1 ? b.hw : (side == 3 ? -b.hw : 0), lz = side == 0 ? b.hd : (side == 2 ? -b.hd : 0);
                double wh = along ? b.heading : b.heading + PI / 2, ht = b.thick * 0.5, h = hs[side];
                double cx, cz; F.to(lx, lz, cx, cz);
                double ax = along ? F.sx : -F.fx, az = along ? F.sz : -F.fz;   // the wall's own axis
                bool window = side == (b.door + 2) % 4 && b.kind != BK_TOWER && b.decay < 0.55 && h > 2.4;
                if (lod == 1 || (side != b.door && !window)) {
                    E.box(cx, cz, wh, L, ht, sink, sink + h, 0, lod == 0 && side == 0 && h > 1.5);
                    continue;
                }
                if (side == b.door) {   // the doorway: a gap of 1.8 m off the middle, a lintel over it where the jambs stand over 2.4 m
                    double gap = 0.9, pos = doorPos;
                    double l1 = pos - gap, l2 = pos + gap;
                    E.box(cx + ax * (-L + l1) * 0.5, cz + az * (-L + l1) * 0.5, wh, (l1 + L) * 0.5, ht, sink, sink + h, 0, lod == 0 && side == 0 && h > 1.5);
                    E.box(cx + ax * (l2 + L) * 0.5, cz + az * (l2 + L) * 0.5, wh, (L - l2) * 0.5, ht, sink, sink + h, 0);
                    if (h > 2.4) E.box(cx + ax * pos, cz + az * pos, wh, gap + 0.3, ht, sink + 2.1, sink + std::min(h, 2.6), 4);
                } else {   // the window: a sill under it and the wall over it
                    double gap = 0.55, pos = (L - 1.2) * (U(21) - 0.5);
                    double l1 = pos - gap, l2 = pos + gap;
                    E.box(cx + ax * (-L + l1) * 0.5, cz + az * (-L + l1) * 0.5, wh, (l1 + L) * 0.5, ht, sink, sink + h, 0);
                    E.box(cx + ax * (l2 + L) * 0.5, cz + az * (l2 + L) * 0.5, wh, (L - l2) * 0.5, ht, sink, sink + h, 0);
                    E.box(cx + ax * pos, cz + az * pos, wh, gap, ht, sink, sink + 1.0, 0);
                    E.box(cx + ax * pos, cz + az * pos, wh, gap, ht, sink + 1.9, sink + h, 0);
                }
            }
            double hmin = std::min(std::min(hs[0], hs[1]), std::min(hs[2], hs[3]));
            if (b.kind == BK_HOUSE && lod == 0 && U(30) < 0.45) {   // an inner wall from the wall opposite the door, three fifths of the way in, beside the
                double cx, cz, half, wh;                                   // centre line (the room's middle stays clear, the doorway too)
                if (b.door == 0 || b.door == 2) { double zs = b.door == 0 ? -1 : 1; F.to(0.5 * b.hw, zs * 0.7 * b.hd, cx, cz); half = 0.3 * b.hd; wh = b.heading + PI / 2; }
                else { double xs = b.door == 1 ? -1 : 1; F.to(xs * 0.7 * b.hw, 0.5 * b.hd, cx, cz); half = 0.3 * b.hw; wh = b.heading; }
                E.box(cx, cz, wh, half, b.thick * 0.4, sink, sink + std::max(0.35, hmin * 0.9), 0);
            }
            if (b.kind == BK_HALL && lod == 0) {   // two rows of columns inside, some fallen
                int k = 0;
                for (int rowS = -1; rowS <= 1; rowS += 2)
                    for (double tt = -b.hd + 2.0; tt < b.hd - 1.5; tt += 3.0, k++) {
                        double f = 1 - b.decay * (0.2 + 0.8 * U(40 + k));
                        if (f < 0.2) continue;
                        double cx, cz; F.to(rowS * b.hw * 0.5, tt, cx, cz);
                        E.box(cx, cz, b.heading, 0.35, 0.35, sink, sink + b.hWall * 0.9 * f, 1);
                    }
            }
            if (c.stoneRoofs && b.kind != BK_TOWER && hmin > b.hWall * 0.6) {   // the roof: whole on a sound house, half on a worn one
                if (b.decay < 0.35) E.box(b.x, b.z, b.heading, b.hw + 0.2, b.hd + 0.2, sink + hmin - 0.05, sink + hmin + 0.3, 2);
                else if (b.decay < 0.6) { double cx, cz; F.to(0, -b.hd * 0.5, cx, cz); E.box(cx, cz, b.heading, b.hw + 0.2, b.hd * 0.5 + 0.2, sink + hmin - 0.05, sink + hmin + 0.3, 2); }
            }
            if (lod == 0)   // rubble at the foot of the fallen walls
                for (int side = 0; side < 4; side++) {
                    if (standing(side) > 0.5) continue;
                    bool along = side == 0 || side == 2; double L = along ? b.hw : b.hd;
                    double lx = side == 1 ? b.hw : (side == 3 ? -b.hw : 0), lz = side == 0 ? b.hd : (side == 2 ? -b.hd : 0);
                    for (int k = 0; k < 3; k++) {
                        double u1 = U(60 + side * 3 + k), u2 = U(80 + side * 3 + k), sz = 0.3 + 0.5 * U(100 + side * 3 + k);
                        double tt = (u1 - 0.5) * 2 * L, off = 0.8 + 1.5 * u2;
                        double px = along ? tt : lx + (side == 1 ? off : -off), pz = along ? lz + (side == 0 ? off : -off) : tt;
                        if ((px - doorX) * (px - doorX) + (pz - doorZ) * (pz - doorZ) < (1.9 + sz) * (1.9 + sz)) continue;   // not in the doorway
                        double cx, cz; F.to(px, pz, cx, cz);
                        E.box(cx, cz, b.heading + u1 * 1.2, sz, sz * 0.8, 0, sz * 1.1, 3);
                    }
                }
            drifts(b.hw, b.hd);
            break;
        }
        case BK_ROTUNDA: {
            double rr = b.hw;
            if (lod == 2) { E.box(b.x, b.z, b.heading, rr, rr, sink, sink + b.hWall * (1 - 0.5 * b.decay), 0); break; }
            const int N = 12;
            double hmin = 1e9, ht = b.thick * 0.5, half = rr * std::sin(PI / N) + ht * 0.6;
            int doorSeg = (int)(U(5) * N);
            for (int k = 0; k < N; k++) {
                if (k == doorSeg && lod == 0) continue;
                double a = b.heading + (k + 0.5) * TAU / N;
                double h = std::max(0.35, b.hWall * (1 - b.decay * (0.15 + 0.85 * U(10 + k))));
                hmin = std::min(hmin, h);
                E.box(b.x + std::sin(a) * rr, b.z + std::cos(a) * rr, a + PI / 2, half, ht, sink, sink + h, 0);
            }
            if (c.stoneRoofs && b.decay < 0.55 && hmin > b.hWall * 0.7) {   // the dome, its top fallen in when worn
                RuinElem e; e.shape = 1; e.x = b.x; e.z = b.z; e.heading = b.heading; e.hx = rr + ht; e.hz = rr + ht; e.y0 = sink + hmin - 0.1; e.y1 = e.y0 + rr;
                e.part = 2; e.broken = b.decay < 0.3 ? 0 : 0.35; e.building = bi;
                out.push_back(e);
            }
            break;
        }
        case BK_GATE: {
            double h = b.hWall * (1 - 0.5 * b.decay), f0 = 1 - b.decay * U(10), f1 = 1 - b.decay * U(11), cx, cz;
            F.to(-b.hw, 0, cx, cz); E.box(cx, cz, b.heading, 0.5, 0.5, sink, sink + std::max(0.5, h * f0), 1);
            F.to(b.hw, 0, cx, cz); E.box(cx, cz, b.heading, 0.5, 0.5, sink, sink + std::max(0.5, h * f1), 1);
            if (f0 > 0.8 && f1 > 0.8 && lod < 2) E.box(b.x, b.z, b.heading, b.hw + 0.5, 0.5, sink + h - 0.7, sink + h, 4);
            break;
        }
        case BK_PLATFORM: {
            if (lod == 2) { E.box(b.x, b.z, b.heading, b.hw, b.hd, sink, sink + 3.3, 2); break; }
            for (int i = 0; i < 3; i++) E.box(b.x, b.z, b.heading, b.hw * (1 - 0.28 * i), b.hd * (1 - 0.28 * i), sink + 1.1 * i, sink + 1.1 * (i + 1), 2);
            if (lod == 0)   // the steps up the front
                for (int k = 0; k < 4; k++) { double cx, cz; F.to(0, b.hd + 0.35 + 0.7 * (3 - k), cx, cz); E.box(cx, cz, b.heading, 1.6, 0.35, 0, 0.28 * (k + 1), 2); }
            if (U(7) < 0.5 && b.decay < 0.6) E.box(b.x, b.z, b.heading, 1.3, 1.3, sink + 3.3, sink + 3.3 + 2.4 * (1 - 0.5 * b.decay), 0, lod == 0);   // a shrine on top
            break;
        }
        case BK_COLONNADE: {
            if (lod == 2) break;
            int k = 0;
            for (double tt = -b.hd + 1.0; tt <= b.hd - 0.9; tt += 2.5, k++) {
                double f = 1 - b.decay * (0.2 + 0.8 * U(10 + k));
                if (f < 0.25) continue;
                double cx, cz; F.to(0, tt, cx, cz);
                E.box(cx, cz, b.heading, 0.4, 0.4, sink, sink + b.hWall * f, 1);
            }
            break;
        }
        case BK_STELA: {
            E.box(b.x, b.z, b.heading, 0.45, 0.25, sink, sink + b.hWall * (1 - b.decay * 0.6 * U(10)), 5, lod == 0);
            break;
        }
        case BK_RUBBLE: {
            if (lod == 2) { E.box(b.x, b.z, b.heading, b.hw * 0.8, b.hd * 0.8, 0, 1.0, 3); break; }
            int nb = 5 + (int)(U(1) * 4);
            for (int k = 0; k < nb; k++) {
                double u1 = U(10 + k), u2 = U(30 + k), sz = 0.4 + 0.7 * U(50 + k), cx, cz;
                F.to((u1 - 0.5) * 1.6 * b.hw, (u2 - 0.5) * 1.6 * b.hd, cx, cz);
                E.box(cx, cz, b.heading + u1 * 2.0, sz, sz * 0.7, 0, sz * 1.2, 3);
            }
            break;
        }
        case BK_WALL: {
            E.box(b.x, b.z, b.heading, b.hw, b.hd, sink, sink + std::max(0.4, b.hWall * (1 - b.decay * (0.15 + 0.85 * U(10)))), 0);
            break;
        }
        // C-08: the terrain's features
        case BK_QUAY: {   // two tiers along the shore, the top one worn away toward one end, bollards on the seaward edge
            double keep = 1 - 0.5 * b.decay;
            E.box(b.x, b.z, b.heading, b.hw, b.hd, sink - 0.6, sink + 0.6, 6);
            double cx, cz; F.to(-b.hw * (1 - keep), 0, cx, cz);
            E.box(cx, cz, b.heading, b.hw * keep, b.hd - 0.2, sink + 0.6, sink + 1.2, 6);
            if (lod == 0)
                for (int k = 0; k < 3; k++) {
                    double tt = -b.hw * 0.7 + b.hw * 0.7 * k;
                    if (U(20 + k) < b.decay * 0.6) continue;
                    F.to(tt, b.hd - 0.5, cx, cz); E.box(cx, cz, b.heading, 0.22, 0.22, sink + 1.2, sink + 1.9, 1);
                }
            break;
        }
        case BK_MOLE: {   // a wall out into the water, its far end fallen to the waterline
            if (lod == 2) { E.box(b.x, b.z, b.heading, b.hw, b.hd, sink - 0.6, sink + b.hWall * (1 - 0.5 * b.decay), 6); break; }
            int segs = std::max(2, (int)(b.hd / 3.0));
            for (int k = 0; k < segs; k++) {
                double u = (k + 0.5) / segs, f = 1 - b.decay * (0.1 + 0.9 * u) * (0.6 + 0.4 * U(10 + k));
                if (f < 0.12) continue;
                double cx, cz; F.to(0, -b.hd + b.hd * 2 * u, cx, cz);
                E.box(cx, cz, b.heading, b.hw, b.hd / segs, sink - 0.6, sink + std::max(0.1, b.hWall * f), 6);
            }
            break;
        }
        case BK_BOLLARD: { E.box(b.x, b.z, b.heading, 0.22, 0.22, sink, sink + 0.8, 1); break; }
        case BK_TERRACE: {   // a retaining wall across the slope, standing to its share
            double f = 1 - b.decay * (0.2 + 0.8 * U(10));
            E.box(b.x, b.z, b.heading, b.hw, b.hd, sink, sink + std::max(0.3, b.hWall * f), 0);
            break;
        }
        case BK_CISTERN: {   // a round tank of thick wall, its lid fallen in, the channel that fed it from upslope
            double rr = b.hw;
            if (lod == 2) { E.box(b.x, b.z, b.heading, rr, rr, sink, sink + b.hWall * (1 - 0.4 * b.decay), 0); break; }
            const int N = 10; double ht = b.thick * 0.5, half = rr * std::sin(PI / N) + ht * 0.6, hmin = 1e9;
            for (int k = 0; k < N; k++) {
                double a = b.heading + (k + 0.5) * TAU / N, h = std::max(0.4, b.hWall * (1 - b.decay * (0.1 + 0.6 * U(10 + k))));
                hmin = std::min(hmin, h);
                E.box(b.x + std::sin(a) * rr, b.z + std::cos(a) * rr, a + PI / 2, half, ht, sink, sink + h, 0);
            }
            if (b.decay < 0.5) E.box(b.x, b.z, b.heading, rr * 0.5, rr * 0.5, sink + hmin - 0.1, sink + hmin + 0.25, 2);   // what is left of the lid
            if (lod == 0) {   // the channel: two low walls a metre apart, 6-14 m out along the heading (upslope)
                double len = 6 + 8 * U(30), cx, cz;
                for (int side = -1; side <= 1; side += 2) { F.to(side * 0.6, rr + 0.7 + len * 0.5, cx, cz); E.box(cx, cz, b.heading, 0.15, len * 0.5, sink, sink + 0.45, 0); }
            }
            break;
        }
        case BK_WINDWALL: {   // a segment of the crescent, the sand against its windward face
            double f = 1 - b.decay * (0.15 + 0.85 * U(10));
            E.box(b.x, b.z, b.heading, b.hw, b.hd, sink, sink + std::max(0.4, b.hWall * f), 0);
            if (lod == 0 && c.buried >= 0.15) { double cx, cz; F.to(0, b.hd + 0.9, cx, cz); E.box(cx, cz, b.heading, b.hw, 0.9, 0, 0.25 + 0.6 * std::min(1.0, c.buried / 1.2), 7); }
            break;
        }
        case BK_CAIRN: {   // three tiers of stones, the top ones tumbled by the age
            int tiers = b.decay < 0.35 ? 3 : (b.decay < 0.7 ? 2 : 1);
            for (int i = 0; i < tiers; i++) E.box(b.x, b.z, b.heading + i * 0.3, b.hw * (1 - 0.3 * i), b.hd * (1 - 0.3 * i), 0.7 * i, 0.7 * (i + 1), 3);
            if (lod == 0 && tiers < 3) { double cx, cz; F.to(1.4, 0.6, cx, cz); E.box(cx, cz, b.heading + 0.8, 0.4, 0.3, 0, 0.4, 3); }
            break;
        }
        default: break;
    }
}
}   // namespace

void ruinElements(const RuinSpec& r, const Culture& c, int lod, std::vector<RuinElem>& out) {
    out.clear();
    for (size_t i = 0; i < r.buildings.size(); i++) elemsBuilding(r.buildings[i], c, r.site, lod, out, (int)i);
}

bool buildingDoor(const Building& b, double& dx, double& dz) {   // C-03
    auto U = [&](int k) { return u01(b.seed, k); };
    Frame F(b);
    switch (b.kind) {
        case BK_HOUSE: case BK_HALL: case BK_TOWER: {
            const double doorL = (b.door == 0 || b.door == 2) ? b.hw : b.hd, doorPos = (doorL - 1.6) * (U(20) - 0.5) * 1.2;   // as `elemsBuilding` draws it
            double lx = 0, lz = 0;
            switch (b.door) { case 0: lx = doorPos; lz = b.hd; break; case 2: lx = doorPos; lz = -b.hd; break; case 1: lx = b.hw; lz = -doorPos; break; default: lx = -b.hw; lz = -doorPos; break; }   // C-08: sides 2 and 3 as the walls are drawn
            F.to(lx, lz, dx, dz);
            return true;
        }
        case BK_ROTUNDA: {
            int doorSeg = (int)(U(5) * 12);
            double a = b.heading + (doorSeg + 0.5) * TAU / 12;
            dx = b.x + std::sin(a) * b.hw; dz = b.z + std::cos(a) * b.hw;
            return true;
        }
        default: return false;
    }
}

bool settlementClearance(const BodyGen& g, double& lat, double& lon, double marginM) {
    if (!worldHadCivilisation(g)) return false;
    int gLat, gLon; ruinCellOf(g, lat, lon, gLat, gLon);
    bool moved = false;
    for (int dl = -1; dl <= 1; dl++)
        for (int dn = -1; dn <= 1; dn++) {
            RuinSpec sp;
            if (!ruinOfCell(g, gLat + dl, gLon + dn, sp, false) || sp.kind != RK_SETTLEMENT) continue;
            double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(sp.lat), 0.05);
            double north = (lat - sp.lat) * mPerRad, east = std::remainder(lon - sp.lon, TAU) * mPerRad * cosLat, d = std::sqrt(north * north + east * east);
            double want = sp.size + marginM;
            if (d >= want) continue;
            if (d < 1) { east = 1; north = 0; d = 1; }
            lat = sp.lat + north / d * want / mPerRad; lon = sp.lon + east / d * want / (mPerRad * cosLat);
            moved = true;
        }
    return moved;
}
