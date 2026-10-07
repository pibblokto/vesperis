#include "landmarks.h"
#include "drainage.h"
#include "ruins.h"
#include "core/noise.h"
#include <cmath>
#include <algorithm>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>

const char* LANDMARK_KIND_NAMES[LM_COUNT] = {"PEAK", "MESA", "CANYON RIM", "CRATER", "GEYSER FIELD", "CRYSTAL FIELD", "LAKE", "RUINS"};
const char* LANDMARK_KIND_SYMBOLS[LM_COUNT] = {"^", "=", "V", "O", "*", "+", "~", "#"};

namespace {
constexpr double CELL_KM = 14.0;   // the sights' grid: one of each kind at most per cell of about 14 km

// the grid: cells of CELL_KM in latitude; in longitude the same length at the cell row's latitude (fewer cells toward the poles)
struct Grid { double dLat; int rows; };
Grid gridOf(const BodyGen& g) { Grid gr; gr.dLat = CELL_KM / g.R; gr.rows = std::max(4, (int)std::floor(PI / gr.dLat)); gr.dLat = PI / gr.rows; return gr; }
int colsOfRow(const BodyGen& g, const Grid& gr, int ci) {
    double lat = -PI / 2 + (ci + 0.5) * gr.dLat;
    return std::max(4, (int)std::floor(TAU * std::cos(lat) / gr.dLat));
}
Vec3 cellCentre(const Grid& gr, int ci, int cj, int cols) {
    double lat = -PI / 2 + (ci + 0.5) * gr.dLat, lon = -PI + (cj + 0.5) * TAU / cols;
    return StarSystem::bodyFromLatLon(lat, lon);
}
// a point `dx`, `dz` metres east and north of a unit vector
Vec3 offsetUnit(const BodyGen& g, const Vec3& u, double dx, double dz) {
    Vec3 pole(0, 0, 1);
    Vec3 e = cross(pole, u); double el = length(e);
    if (el < 1e-9) e = Vec3(1, 0, 0); else e = e / el;
    Vec3 n = cross(u, e);
    return normalize(u + (e * dx + n * dz) / (g.R * 1000.0));
}
}

double landmarkCellDeg(const BodyGen& g) { return gridOf(g).dLat / DEG; }

void landmarkCellOf(const BodyGen& g, const Vec3& unit, int& ci, int& cj) {
    Grid gr = gridOf(g);
    double lat, lon;
    StarSystem::latLonFromBody(unit, lat, lon);
    ci = clampi((int)std::floor((lat + PI / 2) / gr.dLat), 0, gr.rows - 1);
    int cols = colsOfRow(g, gr, ci);
    cj = ((int)std::floor((lon + PI) / (TAU / cols)) % cols + cols) % cols;
}

// the cells searched are kept (a landing, the zoom and the harness ask for the same cells again and again)
namespace {
struct CellKey { uint64_t seed; int ci, cj; bool operator<(const CellKey& o) const { return seed != o.seed ? seed < o.seed : (ci != o.ci ? ci < o.ci : cj < o.cj); } };
struct CellVal { bool any; CellSights s; };
std::mutex g_lmMutex;
std::map<CellKey, CellVal> g_lmCache;
std::atomic<int> g_lmSearched{0};
double g_lmMs[3] = {0, 0, 0};
inline double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline uint64_t cellIdOf(const BodyGen& g, int ci, int cj) { return hash3i(ci, cj, (int64_t)(g.seed & 0xffffff), g.seed ^ 0x1A4DULL); }
bool sightsOfCellUncached(const BodyGen& g, int ci, int cj, CellSights& out, bool buildTiles, bool& complete);
bool sightsWorld(const BodyGen& g) { return PLANET_TYPES[g.type].landable && g.type != PT_OCEAN && g.type != PT_COMET; }
}
int landmarkCellsSearched() { return g_lmSearched.load(); }
double landmarkScanMs(int part) { return g_lmMs[part]; }

bool sightsOfCell(const BodyGen& g, int ci, int cj, CellSights& out, bool buildTiles) {
    out = CellSights();
    if (!sightsWorld(g)) return false;
    CellKey key{g.seed, ci, cj};
    {
        std::lock_guard<std::mutex> lock(g_lmMutex);
        auto it = g_lmCache.find(key);
        if (it != g_lmCache.end()) { out = it->second.s; return it->second.any; }
    }
    CellSights S;
    bool complete = true;
    bool any = sightsOfCellUncached(g, ci, cj, S, buildTiles, complete);
    g_lmSearched++;
    if (complete) {   // a verdict read without every drainage tile is not kept (the lakes may be missing from it)
        std::lock_guard<std::mutex> lock(g_lmMutex);
        if (g_lmCache.size() > 20000) g_lmCache.clear();
        g_lmCache[key] = CellVal{any, S};
    }
    out = S;
    return any;
}

bool legacyLandmarkOf(const CellSights& s, Landmark& out) {
    static const int ORDER[] = {LM_CRATER, LM_CANYON, LM_MESA, LM_GEYSERS, LM_CRYSTALS, LM_LAKE, LM_RUIN, LM_PEAK};
    for (int k : ORDER) {
        if (k == LM_LAKE) {   // O6-06's lake (a playa's too), unless a peak stood 200 m over the cell
            if (!s.hasLake0 || (s.hiH - s.mean >= 200 && s.hiH > 0)) continue;
            out = s.lake0;
            return true;
        }
        if (!s.has[k]) continue;
        if (k == LM_RUIN && s.hiH - s.mean >= 150 && s.hiH > 0) continue;   // O6-06: a peak of 150 m over the cell came before the ruins
        out = s.lm[k];
        return true;
    }
    return false;
}

void cellsOfLegacyIds(const BodyGen& g, const std::set<uint32_t>& ids, std::map<uint32_t, std::pair<int, int>>& out) {
    out.clear();
    if (ids.empty()) return;
    Grid gr = gridOf(g);
    for (int ci = 0; ci < gr.rows && out.size() < ids.size(); ci++) {
        int cols = colsOfRow(g, gr, ci);
        for (int cj = 0; cj < cols; cj++) {
            uint32_t id32 = (uint32_t)(cellIdOf(g, ci, cj) & 0xffffffffULL);
            if (ids.count(id32) && !out.count(id32)) out[id32] = {ci, cj};
        }
    }
}

unsigned sightKindsOf(const BodyGen& g) {
    if (!sightsWorld(g)) return 0;
    unsigned m = 1u << LM_PEAK;
    switch (g.type) {
        case PT_CRATERED: case PT_THINATMO: case PT_ICY: case PT_METAL: case PT_BOMBARDED: case PT_ROCKY: case PT_CARBON: case PT_QUARTZ: case PT_TECTONIC: case PT_DESERT: m |= 1u << LM_CRATER; break;
        default: break;
    }
    if (g.hasTrait(TR_CANYONS)) m |= 1u << LM_CANYON;
    if (g.hasTrait(TR_MESAS)) m |= 1u << LM_MESA;
    if (g.hasTrait(TR_GEYSERS)) m |= 1u << LM_GEYSERS;
    if ((g.type == PT_QUARTZ || g.type == PT_CARBON) && g.hasTrait(TR_SPIRES)) m |= 1u << LM_CRYSTALS;
    if (typeHasDrainage(g.type) && (g.type == PT_FELISIAN || g.type == PT_HYDROCARBON || g.type == PT_ACIDIC)) m |= 1u << LM_LAKE;
    if (worldHasRuins(g)) m |= 1u << LM_RUIN;
    return m;
}

namespace {
// every kind of the cell on its own (O6-06 returned the first of its order; R-408's scanner asks for each)
bool sightsOfCellUncached(const BodyGen& g, int ci, int cj, CellSights& S, bool buildTiles, bool& complete) {
    Grid gr = gridOf(g);
    if (ci < 0 || ci >= gr.rows) return false;
    int cols = colsOfRow(g, gr, ci);
    cj = (cj % cols + cols) % cols;
    Vec3 c = cellCentre(gr, ci, cj, cols);
    double lat0 = -PI / 2 + (ci + 0.5) * gr.dLat;
    if (std::fabs(lat0) > 80 * DEG) return false;
    uint64_t id = cellIdOf(g, ci, cj);
    double half = 0.5 * gr.dLat * g.R * 1000.0;   // the cell's half-size in metres
    // the cell is scanned at 500 m on a 24 x 24 grid (without the drainage: a tile's cell per sample would cost more than
    // the scan): the ground, its highest point, the traits' marks; the lakes are asked of the drainage at the cell's grid
    const int N = 24;
    double hs[N * N]; double marks[N * N]; int mats[N * N]; int hiIdx = 0; double hiH = -1e9, loH = 1e9, sum = 0;
    int wallN = 0;
    double tScan0 = nowMs();
    {
        DrainageOff off;
        for (int j = 0; j < N; j++)
            for (int i = 0; i < N; i++) {
                double dx = ((i + 0.5) / N - 0.5) * 2 * half, dz = ((j + 0.5) / N - 0.5) * 2 * half;
                SurfaceSample s = sampleSurface(g, offsetUnit(g, c, dx, dz), 500.0);
                int k = j * N + i;
                hs[k] = s.height; marks[k] = s.landform; mats[k] = s.material;
                if (s.material == MAT_WATER && s.height < 0) hs[k] = 0;
                sum += hs[k];
                if (hs[k] > hiH) { hiH = hs[k]; hiIdx = k; }
                loH = std::min(loH, hs[k]);
                if (s.landform >= 1000) { if (g.hasTrait(TR_CANYONS) && s.material == MAT_ROCK) wallN++; }
            }
    }
    (void)mats;
    double tScan1 = nowMs(); g_lmMs[0] += tScan1 - tScan0;
    double mean = sum / (N * N);
    S.hiH = hiH; S.mean = mean;
    auto sight = [&](int kind) -> Landmark& { S.has[kind] = true; Landmark& L = S.lm[kind]; L = Landmark(); L.kind = kind; L.id = id; return L; };
    // 1. a crater: the biggest crater of the world's grids with its centre in the cell and a radius over 1.2 km
    {
        double bestR = 0; Vec3 bestC; double bestDep = 0;
        struct Grd { double cellKm, density; } grids[5]; int ng = 0;
        double cd = g.craterDensity, dp = g.craterDepth;
        switch (g.type) {
            case PT_CRATERED: grids[ng++] = {g.R * 0.06, cd}; grids[ng++] = {12.0, cd * 1.2}; grids[ng++] = {2.5, cd * 1.5}; break;
            case PT_THINATMO: grids[ng++] = {g.R * 0.05, cd}; grids[ng++] = {8.0, cd * 0.9}; break;
            case PT_ICY: grids[ng++] = {6.0, cd}; break;
            case PT_METAL: grids[ng++] = {g.R * 0.08, cd}; grids[ng++] = {8.0, cd * 1.2}; break;
            case PT_BOMBARDED: grids[ng++] = {g.R * 0.08, 0.7}; grids[ng++] = {8.0, 0.9}; break;
            case PT_ROCKY: grids[ng++] = {20.0, cd}; break;
            case PT_CARBON: grids[ng++] = {5.0, cd}; break;
            case PT_QUARTZ: grids[ng++] = {15.0, cd}; break;
            case PT_TECTONIC: grids[ng++] = {30.0, 0.08}; break;
            case PT_DESERT: grids[ng++] = {3.0, cd * 0.6}; break;
            default: break;
        }
        (void)dp;
        Vec3 pc = c * g.R;
        for (int k = 0; k < ng; k++) {
            double cellKm = grids[k].cellKm;
            uint64_t seed = g.sD + (uint64_t)(cellKm * 1000);
            double inv = 1.0 / cellKm;
            int64_t cx = (int64_t)std::floor(pc.x * inv), cy = (int64_t)std::floor(pc.y * inv), cz = (int64_t)std::floor(pc.z * inv);
            for (int dz = -1; dz <= 1; dz++)
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        uint64_t h = hash3i(cx + dx, cy + dy, cz + dz, seed);
                        if (unitFromHash(h) > grids[k].density) continue;
                        double u0 = unitFromHash(mix64(h + 1)), u1 = unitFromHash(mix64(h + 2)), u2 = unitFromHash(mix64(h + 3));
                        Vec3 cc((cx + dx + u0) * cellKm, (cy + dy + u1) * cellKm, (cz + dz + u2) * cellKm);
                        double cl = length(cc);
                        if (std::fabs(cl - g.R) > cellKm * 0.5) continue;
                        cc = cc * (g.R / cl);
                        double r = cellKm * (0.14 + 0.30 * unitFromHash(mix64(h + 13)));
                        if (r < 1.2 || r < bestR) continue;
                        // its centre in this cell?
                        int ci2, cj2; landmarkCellOf(g, normalize(cc), ci2, cj2);
                        if (ci2 != ci || cj2 != cj) continue;
                        bestR = r; bestC = normalize(cc); bestDep = r * 0.2 * g.craterDepth * (0.4 + 0.6 * unitFromHash(mix64(h + 14))) * 1000.0;
                    }
        }
        if (bestR > 0) {
            Landmark& L = sight(LM_CRATER);
            L.unit = bestC; L.radiusM = bestR * 1000.0; L.prominenceM = bestDep;
            L.heightM = sampleSurface(g, bestC, 64.0).height;
        }
    }
    // 2. a canyon rim: the world has canyons and the cell holds their walls; the rim is the highest wall sample's
    // neighbour outside the wall, refined at 64 m
    if (g.hasTrait(TR_CANYONS) && wallN >= 6) {
        int best = -1; double bestH = -1e9;
        for (int k = 0; k < N * N; k++) if (marks[k] >= 1000 && hs[k] > bestH) { bestH = hs[k]; best = k; }
        if (best >= 0) {
            int i = best % N, j = best / N;
            double dx = ((i + 0.5) / N - 0.5) * 2 * half, dz = ((j + 0.5) / N - 0.5) * 2 * half;
            // step outward in eight directions until the mark ends: the rim
            Vec3 rim = offsetUnit(g, c, dx, dz); double rimH = bestH; bool found = false;
            for (int a = 0; a < 8 && !found; a++) {
                double ax = std::cos(a * TAU / 8), az = std::sin(a * TAU / 8);
                for (double d = 60; d <= 900; d += 60) {
                    Vec3 u = offsetUnit(g, c, dx + ax * d, dz + az * d);
                    SurfaceSample s = sampleSurface(g, u, 64.0);
                    if (s.landform < 1000 && s.height > bestH - 30) { rim = offsetUnit(g, c, dx + ax * (d - 30), dz + az * (d - 30)); rimH = s.height; found = true; break; }
                }
            }
            Landmark& L = sight(LM_CANYON);
            L.unit = rim; L.radiusM = 600; L.heightM = rimH; L.prominenceM = rimH - loH;
        }
    }
    // 3. a mesa: the badlands trait's caps (its 1.4 km cells, the same hash): the biggest cap centred in the cell
    if (g.hasTrait(TR_MESAS)) {
        double bestCap = 0; Vec3 bestC; double bestRad = 0;
        Vec3 pc = c * g.R;
        double cellKm = 1.4;
        int64_t cx = (int64_t)std::floor(pc.x / cellKm), cy = (int64_t)std::floor(pc.y / cellKm), cz = (int64_t)std::floor(pc.z / cellKm);
        int reach = (int)std::ceil(half / 1000.0 / cellKm) + 1;
        for (int dz = -reach; dz <= reach; dz++)
            for (int dy = -reach; dy <= reach; dy++)
                for (int dx = -reach; dx <= reach; dx++) {
                    // the worley cell's feature point (worley3 hashes the cell's integer coordinates)
                    Worley3 w = worley3(Vec3((cx + dx + 0.5) * cellKm, (cy + dy + 0.5) * cellKm, (cz + dz + 0.5) * cellKm) / cellKm, g.sE + 220);
                    if (unitFromHash(mix64(w.id1 + 7)) >= 0.5) continue;   // no cap in this cell
                    Vec3 cc = w.c1 * cellKm;
                    double cl = length(cc);
                    if (std::fabs(cl - g.R) > cellKm) continue;
                    Vec3 cu = normalize(cc);
                    int ci2, cj2; landmarkCellOf(g, cu, ci2, cj2);
                    if (ci2 != ci || cj2 != cj) continue;
                    double capH = 60 + 160 * unitFromHash(mix64(w.id1 + 8)), rad = 0.3 + 0.25 * unitFromHash(mix64(w.id1 + 9));
                    if (capH <= bestCap) continue;
                    SurfaceSample sr = sampleSurface(g, offsetUnit(g, cu, rad * 900.0, 0), 64.0);   // the wall stands where the mask does
                    if (sr.landform < 1000) continue;
                    bestCap = capH; bestC = cu; bestRad = rad;
                }
        if (bestCap > 0) {
            Landmark& L = sight(LM_MESA);
            L.unit = bestC; L.radiusM = bestRad * 1000.0; L.prominenceM = bestCap;
            L.heightM = sampleSurface(g, bestC, 64.0).height;
        }
    }
    // 4. a geyser field: the basin's vents (`geyserVents`) centred in the cell, the biggest mound
    if (g.hasTrait(TR_GEYSERS)) {
        std::vector<GeyserVent> vents;
        geyserVents(g, c, vents);
        double bestR = 0; const GeyserVent* best = nullptr;
        for (const GeyserVent& v : vents) {
            int ci2, cj2; landmarkCellOf(g, v.unit, ci2, cj2);
            if (ci2 != ci || cj2 != cj || v.radiusKm <= bestR) continue;
            if (sampleSurface(g, v.unit, 64.0).material != MAT_SALT) continue;   // the mound stands where the basin's mask does
            bestR = v.radiusKm; best = &v;
        }
        if (best) {
            Landmark& L = sight(LM_GEYSERS);
            L.unit = best->unit; L.radiusM = best->radiusKm * 1000.0; L.prominenceM = 12;
            L.heightM = sampleSurface(g, best->unit, 64.0).height;
        }
    }
    // 5. a crystal field: a quartz or carbon world's spire province at its densest (the spires live in the 4 m ring: the
    // province is read where the mark stands at 2 m)
    if ((g.type == PT_QUARTZ || g.type == PT_CARBON) && g.hasTrait(TR_SPIRES)) {
        int hits = 0; double bx = 0, bz = 0;
        for (int j = 0; j < 6; j++)
            for (int i = 0; i < 6; i++) {
                double dx = ((i + 0.5) / 6 - 0.5) * 2 * half, dz = ((j + 0.5) / 6 - 0.5) * 2 * half;
                int n = 0;
                for (int k = 0; k < 12; k++) { SurfaceSample s = sampleSurface(g, offsetUnit(g, c, dx + (k % 4) * 11.0, dz + (k / 4) * 13.0), 2.0); if (s.landform >= 1000) n++; }
                if (n > hits) { hits = n; bx = dx; bz = dz; }
            }
        if (hits >= 3) {
            Landmark& L = sight(LM_CRYSTALS);
            L.unit = offsetUnit(g, c, bx, bz); L.radiusM = 400; L.prominenceM = 25;
            L.heightM = sampleSurface(g, L.unit, 64.0).height;
        }
    }
    // 6. a lake of the drainage (the wet worlds), asked of the drainage's own cells on a 20 x 20 grid of the cell (a lake cell).
    // O6-06's lake: the centroid of every lake sample of the cell, 0.6 km^2 at least (a playa's too: a dry basin's fill is a lake
    // cell) and its peak of 200 m that came first, kept for the legacy pick. R-408's: the lakes of the cell (the samples joined to
    // their eight neighbours), the biggest that holds water at 64 m, at its sample under the water nearest its centroid (a lattice
    // cell of the lake holds shore and islands too; 48 asked at most a lake): the scanner points at water
    if (typeHasDrainage(g.type) && (g.type == PT_FELISIAN || g.type == PT_HYDROCARBON || g.type == PT_ACIDIC)) {
        const int M = 20;
        bool wetAt[M * M] = {}; double levelAt[M * M] = {};
        double tl0 = nowMs();
        double sx = 0, sz = 0, sl = 0; int n = 0;
        auto offOf = [&](int k, double& dx, double& dz) { dx = ((k % M + 0.5) / M - 0.5) * 2 * half; dz = ((k / M + 0.5) / M - 0.5) * 2 * half; };
        for (int k = 0; k < M * M; k++) {
            double dx, dz; offOf(k, dx, dz);
            double lv; bool known;
            bool lake = drainageLakeCell(g, offsetUnit(g, c, dx, dz), lv, buildTiles, known);
            if (!known) complete = false;
            wetAt[k] = lake; levelAt[k] = lv;
            if (lake) { n++; sx += dx; sz += dz; sl += lv; }
        }
        const double cellAreaKm2 = (2 * half / M) * (2 * half / M) * 1e-6;
        if (n >= 3 && n * cellAreaKm2 >= 0.6) {
            S.hasLake0 = true; Landmark& L = S.lake0; L = Landmark(); L.kind = LM_LAKE; L.id = id;
            L.unit = offsetUnit(g, c, sx / n, sz / n); L.radiusM = std::sqrt(n * cellAreaKm2 * 1e6 / PI); L.heightM = sl / n;
        }
        std::vector<std::vector<int>> lakes;
        int comp[M * M]; for (int k = 0; k < M * M; k++) comp[k] = -1;
        for (int k0 = 0; k0 < M * M; k0++) {
            if (!wetAt[k0] || comp[k0] >= 0) continue;
            std::vector<int> members, stack(1, k0); comp[k0] = k0;
            while (!stack.empty()) {
                int k = stack.back(); stack.pop_back(); members.push_back(k);
                for (int dj = -1; dj <= 1; dj++)
                    for (int di = -1; di <= 1; di++) {
                        int ii = k % M + di, jj = k / M + dj;
                        if (ii < 0 || jj < 0 || ii >= M || jj >= M) continue;
                        int kk = jj * M + ii;
                        if (wetAt[kk] && comp[kk] < 0) { comp[kk] = k0; stack.push_back(kk); }
                    }
            }
            if ((int)members.size() >= 3 && members.size() * cellAreaKm2 >= 0.6) lakes.push_back(members);
        }
        std::sort(lakes.begin(), lakes.end(), [](const std::vector<int>& a, const std::vector<int>& b) { return a.size() > b.size(); });
        for (size_t li = 0; li < lakes.size() && li < 3 && !S.has[LM_LAKE]; li++) {
            const std::vector<int>& mem = lakes[li];
            double ci_ = 0, cj_ = 0, lsum = 0;
            for (int k : mem) { ci_ += k % M; cj_ += k / M; lsum += levelAt[k]; }
            ci_ /= mem.size(); cj_ /= mem.size();
            std::vector<std::pair<double, int>> order;
            for (int k : mem) order.push_back({(k % M - ci_) * (k % M - ci_) + (k / M - cj_) * (k / M - cj_), k});
            std::sort(order.begin(), order.end());
            const size_t stride = std::max<size_t>(1, order.size() / 48);
            for (size_t q = 0; q < order.size(); q += (q == 0 ? 1 : stride)) {
                double dx, dz; offOf(order[q].second, dx, dz);
                const Vec3 u = offsetUnit(g, c, dx, dz);
                const SurfaceSample sm = sampleSurface(g, u, 64.0);
                if (!(sm.water > -1e8 && sm.height < sm.water)) continue;
                Landmark& L = sight(LM_LAKE);
                L.unit = u; L.radiusM = std::sqrt(mem.size() * cellAreaKm2 * 1e6 / PI); L.heightM = sm.water; L.prominenceM = 0;
                break;
            }
            (void)lsum;
        }
        g_lmMs[1] += nowMs() - tl0;
    }
    // 7. the ruins of the old ones (M4-07's grid of 2 km cells, one in twenty, on felisian, quartz and ocean worlds) and,
    // on a world that had a civilisation (C-01, `galaxy/ruins.*`), its settlements: the biggest of the cell (a town before a
    // village before a hamlet before a monolith); the scanner lists every one of them apart (`ruinsNear`)
    if (g.type == PT_FELISIAN || g.type == PT_QUARTZ || worldHadCivilisation(g)) {
        double dLat = ruinCellLat(g);   // the ruins' grid (M4-07): 2 km cells of latitude and longitude
        int gLat0 = (int)std::floor((lat0 - half / (g.R * 1000.0)) / dLat), gLat1 = (int)std::floor((lat0 + half / (g.R * 1000.0)) / dLat);
        double bestSize = 0; Vec3 bestU; RuinSpec bestSpec;
        for (int gLat = gLat0; gLat <= gLat1; gLat++) {
            double latc = (gLat + 0.5) * dLat;
            if (std::fabs(latc) > PI / 2 - dLat) continue;
            double dLon = dLat / std::max(std::cos(latc), 0.05);
            double lonC; { double la; StarSystem::latLonFromBody(c, la, lonC); }
            double lonHalf = half / (g.R * 1000.0) / std::max(std::cos(lat0), 0.05);
            int gLon0 = (int)std::floor((lonC - lonHalf) / dLon), gLon1 = (int)std::floor((lonC + lonHalf) / dLon);
            for (int gLon = gLon0; gLon <= gLon1; gLon++) {
                RuinSpec spec;
                if (!ruinOfCell(g, gLat, gLon, spec, false)) continue;
                Vec3 u = StarSystem::bodyFromLatLon(spec.lat, spec.lon);
                int ci2, cj2; landmarkCellOf(g, u, ci2, cj2);
                if (ci2 != ci || cj2 != cj) continue;
                double size = spec.kind == RK_SETTLEMENT ? 1000.0 * settlementRank(spec.sclass) + 500.0 + spec.size : spec.size;
                if (size <= bestSize || !ruinSiteOk(g, spec)) continue;
                bestSize = size; bestU = u; bestSpec = spec;
            }
        }
        if (bestSize > 0) {
            bool settlement = bestSpec.kind == RK_SETTLEMENT;
            Landmark& L = sight(LM_RUIN);
            L.unit = bestU; L.radiusM = settlement ? bestSpec.size : 60; L.prominenceM = settlement ? 2 * bestSpec.size : bestSpec.size;
            L.sub = settlement ? bestSpec.sclass + 1 : 0; L.heightM = sampleSurface(g, bestU, 16.0).height;
        }
    }
    // 8. a peak: the highest point of the cell, standing 90 m over the cell's mean (50 m on the flat types: an inselberg
    // of 50 m is the sight of a desert, whose sites had a landmark within 10 km one time in three), refined at 64 m in a 9 x 9 round it
    const bool flatType = g.type == PT_DESERT || g.type == PT_HYDROCARBON || g.type == PT_EUROPAN || g.type == PT_VENUSIAN || g.type == PT_QUARTZ || g.type == PT_ACIDIC || g.type == PT_VOLCANIC || g.type == PT_CARBON;
    if (hiH - mean >= (flatType ? 50 : 90) && hiH > 0) {
        int i = hiIdx % N, j = hiIdx / N;
        double dx = ((i + 0.5) / N - 0.5) * 2 * half, dz = ((j + 0.5) / N - 0.5) * 2 * half;
        double step = 2 * half / N;
        double bestH = -1e9, bx = dx, bz = dz;
        for (int jj = -4; jj <= 4; jj++)
            for (int ii = -4; ii <= 4; ii++) {
                double x = dx + ii * step / 4.5, z = dz + jj * step / 4.5;
                double hh = sampleSurface(g, offsetUnit(g, c, x, z), 64.0).height;
                if (hh > bestH) { bestH = hh; bx = x; bz = z; }
            }
        for (int jj = -3; jj <= 3; jj++)   // and once more at 16 m
            for (int ii = -3; ii <= 3; ii++) {
                double x = bx + ii * 24.0, z = bz + jj * 24.0;
                double hh = sampleSurface(g, offsetUnit(g, c, x, z), 16.0).height;
                if (hh > bestH) { bestH = hh; dx = x; dz = z; }
            }
        Landmark& L = sight(LM_PEAK);
        L.unit = offsetUnit(g, c, dx, dz); L.heightM = bestH; L.prominenceM = bestH - mean; L.radiusM = 900;
    }
    for (int k = 0; k < LM_COUNT; k++) if (S.has[k]) return true;
    return S.hasLake0;
}
}

void sightsNear(const BodyGen& g, const Vec3& unit, double radiusM, unsigned kindMask, std::vector<Landmark>& out, bool buildTiles) {
    out.clear();
    if (!sightsWorld(g) || !kindMask) return;
    Grid gr = gridOf(g);
    double lat, lon;
    StarSystem::latLonFromBody(unit, lat, lon);
    const double R = g.R * 1000.0, rRad = radiusM / R;
    int ci0 = (int)std::floor((lat - rRad + PI / 2) / gr.dLat), ci1 = (int)std::floor((lat + rRad + PI / 2) / gr.dLat);
    for (int ci = std::max(0, ci0); ci <= std::min(gr.rows - 1, ci1); ci++) {
        int cols = colsOfRow(g, gr, ci);
        double latc = -PI / 2 + (ci + 0.5) * gr.dLat;
        double lonR = rRad / std::max(0.1, std::cos(latc));
        int cj0 = (int)std::floor((lon - lonR + PI) / (TAU / cols)), cj1 = (int)std::floor((lon + lonR + PI) / (TAU / cols));
        if (cj1 - cj0 >= cols) { cj0 = 0; cj1 = cols - 1; }
        for (int cj = cj0; cj <= cj1; cj++) {
            CellSights S;
            if (!sightsOfCell(g, ci, ((cj % cols) + cols) % cols, S, buildTiles)) continue;
            for (int k = 0; k < LM_COUNT; k++)
                if (S.has[k] && ((kindMask >> k) & 1) && std::acos(clampd(dot(S.lm[k].unit, unit), -1, 1)) * R <= radiusM) out.push_back(S.lm[k]);
        }
    }
}

void ruinsNear(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<Landmark>& out) {
    out.clear();
    if (!worldHasRuins(g) || !sightsWorld(g)) return;
    double lat, lon;
    StarSystem::latLonFromBody(unit, lat, lon);
    const double R = g.R * 1000.0, rRad = radiusM / R, dLat = ruinCellLat(g);
    int gLat0 = (int)std::floor((lat - rRad) / dLat), gLat1 = (int)std::floor((lat + rRad) / dLat);
    for (int gLat = gLat0; gLat <= gLat1; gLat++) {
        double latc = (gLat + 0.5) * dLat;
        if (std::fabs(latc) > PI / 2 - dLat) continue;
        int nLon = std::max(1, ruinCellsAround(g, gLat));
        double dLon = dLat / std::max(std::cos(latc), 0.05), lonR = rRad / std::max(std::cos(latc), 0.05);   // the grid's own (`ruinCellOf`)
        int gLon0 = (int)std::floor((lon - lonR) / dLon), gLon1 = (int)std::floor((lon + lonR) / dLon);
        if (gLon1 - gLon0 >= nLon) { gLon0 = 0; gLon1 = nLon - 1; }
        for (int gLon = gLon0; gLon <= gLon1; gLon++) {
            RuinSpec spec;
            if (!ruinOfCell(g, gLat, gLon, spec, false)) continue;
            Vec3 u = StarSystem::bodyFromLatLon(spec.lat, spec.lon);
            if (std::acos(clampd(dot(u, unit), -1, 1)) * R > radiusM) continue;
            const bool settlement = spec.kind == RK_SETTLEMENT;
            double h = 0;
            if (settlement) { DrainageOff off; if (!ruinSiteOk(g, spec)) continue; h = sampleSurface(g, u, 64.0).height; }   // as the surface view keeps them
            else { SurfaceSample sm = sampleSurface(g, u, 16.0); if (sm.water > -1e8 && sm.height < sm.water) continue; h = sm.height; }   // as it draws the monoliths
            Landmark L; L.kind = LM_RUIN; L.unit = u; L.id = spec.seed; L.heightM = h;
            L.radiusM = settlement ? spec.size : std::max(20.0, spec.size); L.prominenceM = settlement ? 2 * spec.size : spec.size;
            L.sub = settlement ? spec.sclass + 1 : 0;
            out.push_back(L);
        }
    }
}
