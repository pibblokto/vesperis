#include "drainage.h"
#include "core/noise.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <climits>

namespace {
constexpr double MAX_LAT_DEG = 78.0;    // no drainage under the polar caps (the lattice's cells thin out there)
constexpr double CELL_M = 400.0;        // the lattice spacing aimed at
constexpr double MARGIN_DEG = 0.3;      // a tile is routed over its core plus this margin
constexpr double BLEND_DEG = 0.2;       // two tiles' estimates blend over this distance from a tile's edge
constexpr double DETAIL_MAX = 2100.0;   // no drainage above this sampling scale (the world map has no rivers)
constexpr double COARSE_DETAIL = 1000.0; // from this scale up (the 2048 m ring) the neighbourhood is the own tile's raw cells with straight reaches
constexpr int CACHE_CAP = 28;
constexpr double LAKE_DEPTH = 8.0;      // a depression at least this deep at its pit stays a lake; a shallower one is breached (its spill carved down)
constexpr double LAKE_AREA_MAX = 30.0;  // km^2: a bigger depression is breached part way (below), so mountain basins are not seas
constexpr double LAKE_KEEP = 0.4;       // the share of the small deep depressions that stay full lakes (by the pit's hash); the rest are breached to a pond: the ridged relief pockets every valley, and every pocket a lake made a highland half water
constexpr double CARVE_MAX = 400.0;     // metres: no reach runs deeper than this under the ground it crosses (a canyon, never a slot)
// a big basin's outlet is cut this far below its spill (a gorge, deeper for a bigger basin: more water cut it), the rest stays a lake
inline double breachDepth(double areaKm2) { return 60.0 + 12.0 * std::sqrt(areaKm2); }

std::atomic<bool> g_enabled{std::getenv("VESPERIS_NODRAIN") == nullptr};   // VESPERIS_NODRAIN=1: the harness measures the ground without the drainage
thread_local int t_noDrain = 0;   // > 0 while a tile is being sampled: the planet function is read without the drainage

// the eight neighbours, k = 0..7; 7 - k is the opposite direction
const int OFF_I[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
const int OFF_J[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
enum { FL_SEA = 1, FL_LAKE = 2, FL_VALID = 4 };

struct Tile {
    int N0 = 0, M = 0, Nd = 0, i0 = 0, j0 = 0;   // N0 cells per degree; the domain of Nd x Nd cells from lattice (i0, j0)
    bool empty = true;
    std::vector<float> lvl, acc;                 // the filled height (m) and the accumulation (km^2) per domain cell
    std::vector<uint8_t> dir, up, flag;          // downstream neighbour 0-7 (8 none), main upstream neighbour (8 none), flags
    double buildMs = 0;
    int lakesKept = 0, lakesBreached = 0;
    // -1 when (I, J) lies outside the domain
    int index(int I, int J) const {
        int i = I - i0;
        if (i < 0 || i >= Nd) return -1;
        int W = 360 * N0;
        int j = ((J - j0) % W + W) % W;
        if (j >= Nd) return -1;
        return i * Nd + j;
    }
    // the cell's distance to the domain's edge, in cells (for the blend)
    int edgeDist(int I, int J) const {
        int i = I - i0;
        int W = 360 * N0;
        int j = ((J - j0) % W + W) % W;
        return std::min(std::min(i, Nd - 1 - i), std::min(j, Nd - 1 - j));
    }
};

struct Key { uint64_t gen; int tLat, tLon; bool operator<(const Key& o) const { return gen != o.gen ? gen < o.gen : (tLat != o.tLat ? tLat < o.tLat : tLon < o.tLon); } };
struct Entry { std::shared_ptr<Tile> tile; bool computing = false; uint64_t use = 0; };
std::mutex g_mutex;
std::condition_variable g_cv;
std::map<Key, Entry> g_tiles;
uint64_t g_use = 0;
std::vector<std::thread> g_workers;   // async prefetches, joined when finished (and at exit, by the reaper below)
std::vector<std::shared_ptr<std::atomic<bool>>> g_workerDone;

inline uint64_t hashD(uint64_t h, double v) { uint64_t b; std::memcpy(&b, &v, 8); return mix64(h ^ (b + 0x9E3779B97F4A7C15ULL)); }
// everything of a BodyGen that shapes the ground (a test's copy of a body with its traits removed must get its own tiles)
uint64_t genKey(const BodyGen& g) {
    uint64_t h = mix64(g.seed ^ 0xD8A1A6EULL) ^ (uint64_t)g.type * 0x9E37ULL;
    for (int k = 0; k < 3; k++) h = mix64(h ^ (uint64_t)(g.traits[k] + 1) * 0xC2B2AE3D27D4EB4FULL);
    h = hashD(h, g.R); h = hashD(h, g.seaLevel); h = hashD(h, g.mountainAmp); h = hashD(h, g.hillAmp); h = hashD(h, g.craterDensity); h = hashD(h, g.craterDepth);
    h = hashD(h, g.moistureBias); h = hashD(h, g.lavaLevel); h = hashD(h, g.duneAmp); h = hashD(h, g.duneWl); h = hashD(h, g.windAngle); h = hashD(h, g.plateK); h = hashD(h, g.chainAmp);
    h = hashD(h, g.tempBias); h = hashD(h, g.riverDensity); h = hashD(h, g.lakeDensity); h = hashD(h, g.liquidLevel); h = hashD(h, g.contScale); h = hashD(h, g.islandDensity);
    h = hashD(h, g.reliefWl0); h = hashD(h, g.reliefH); h = hashD(h, g.reliefRidge); h = hashD(h, g.reliefWarp); h = hashD(h, g.reliefErosion); h = hashD(h, g.reliefHybrid);
    h = hashD(h, g.reliefFlat); h = hashD(h, g.reliefHill); h = hashD(h, g.reliefMtn); h = hashD(h, g.snowLine); h = hashD(h, g.volcanoes); h = hashD(h, g.boulderDensity);
    h = mix64(h ^ g.sA) ^ mix64(g.sB + 1) ^ mix64(g.sC + 2) ^ mix64(g.sD + 3) ^ mix64(g.sE + 4) ^ mix64(g.sF + 5) ^ mix64(g.sG + 6);
    h = mix64(h ^ (uint64_t)(g.tectonic ? 1 : 0) ^ (uint64_t)(g.locked ? 2 : 0) ^ (uint64_t)g.duneStyle * 16);
    if (g.locked) { h = hashD(h, g.lockedDir.x); h = hashD(h, g.lockedDir.y); h = hashD(h, g.lockedDir.z); }
    return h;
}

inline bool isSeaSample(const BodyGen& g, const SurfaceSample& s) {
    if (g.type == PT_FELISIAN) return s.biome == BIO_OCEAN;
    return g.liquidLevel > -1e8 && s.material == MAT_WATER && s.water > -1e8 && std::fabs(s.water - g.liquidLevel) < 0.01;
}

void buildTile(const BodyGen& g, int tLat, int tLon, Tile& T) {
    auto t0 = std::chrono::steady_clock::now();
    const int N0 = drainageLatticeN0(g.R);
    T.N0 = N0; T.M = std::max(2, (int)std::lround(MARGIN_DEG * N0)); T.Nd = N0 + 2 * T.M;
    T.i0 = tLat * N0 - T.M; T.j0 = tLon * N0 - T.M;
    double latC = -90.0 + tLat + 0.5;
    if (std::fabs(latC) > MAX_LAT_DEG) { T.empty = true; return; }
    T.empty = false;
    const int Nd = T.Nd, n = Nd * Nd;
    const double dDeg = 1.0 / N0, dRad = dDeg * DEG;
    const double cellLatM = dRad * g.R * 1000.0;
    const double detail = std::max(30.0, 0.6 * cellLatM);
    std::vector<float> h(n, 0.f);
    std::vector<uint8_t> flag(n, 0);
    {   // sample the ground of the domain (in threads of this function's own: it may run inside a worker of the pool)
        int threads = std::min(8, std::max(1, (int)std::thread::hardware_concurrency()));
        std::vector<std::thread> ts;
        auto job = [&](int r0, int r1) {
            t_noDrain++;
            for (int i = r0; i < r1; i++) {
                int I = T.i0 + i;
                if (I < 0 || I >= 180 * N0) continue;
                double lat = (-90.0 + (I + 0.5) * dDeg) * DEG;
                for (int j = 0; j < Nd; j++) {
                    int J = T.j0 + j;
                    double lon = ((J + 0.5) * dDeg - 180.0) * DEG;
                    SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), detail);
                    int c = i * Nd + j;
                    h[c] = (float)s.height;
                    flag[c] = FL_VALID | (isSeaSample(g, s) ? FL_SEA : 0);
                }
            }
            t_noDrain--;
        };
        int band = (Nd + threads - 1) / threads;
        for (int k = 1; k < threads; k++) { int r0 = k * band, r1 = std::min(Nd, r0 + band); if (r0 < r1) ts.emplace_back(job, r0, r1); }
        job(0, std::min(Nd, band));
        for (auto& t : ts) t.join();
    }
    // the routing surface: the ground plus a metre of slow noise and a few centimetres of hash, so the flood's paths over
    // the flats (the sediment floors, the breached basins) follow a lie of the land instead of the scan order, which made
    // combs of parallel streams and dead-straight reaches
    std::vector<float> hr(n);
    for (int i = 0; i < Nd; i++) {
        double lat = (-90.0 + (T.i0 + i + 0.5) * dDeg) * DEG;
        for (int j = 0; j < Nd; j++) {
            int c = i * Nd + j;
            int I = T.i0 + i, J = T.j0 + j;
            double lon = ((J + 0.5) * dDeg - 180.0) * DEG;
            Vec3 pk = StarSystem::bodyFromLatLon(lat, lon) * g.R;
            hr[c] = h[c] + (float)(1.0 * gnoise3(pk / (2.5 * cellLatM * 0.001), g.sF ^ 0x0F10ULL) + 0.15 * (unitFromHash(hash2i(I, J, g.sF ^ 0x0F11ULL)) - 0.5));
        }
    }
    // priority flood: outlets are the domain's edge and every sea cell; the cells are popped lowest first and each newly
    // reached cell takes the popped cell as its downstream neighbour, so every path to an outlet never rises
    T.lvl.assign(n, 0.f); T.dir.assign(n, 8); T.up.assign(n, 8); T.acc.assign(n, 0.f); T.flag = flag;
    std::vector<uint8_t> seen(n, 0);
    using QE = std::pair<float, int>;
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
    std::vector<int> order; order.reserve(n);
    for (int i = 0; i < Nd; i++)
        for (int j = 0; j < Nd; j++) {
            int c = i * Nd + j;
            bool edge = i == 0 || j == 0 || i == Nd - 1 || j == Nd - 1;
            if (edge || !(flag[c] & FL_VALID) || (flag[c] & FL_SEA)) { seen[c] = 1; T.lvl[c] = hr[c]; pq.push({hr[c], c}); }
        }
    while (!pq.empty()) {
        QE e = pq.top(); pq.pop();
        int c = e.second; float f = e.first;
        order.push_back(c);
        int i = c / Nd, j = c % Nd;
        for (int k = 0; k < 8; k++) {
            int ni = i + OFF_I[k], nj = j + OFF_J[k];
            if (ni < 0 || nj < 0 || ni >= Nd || nj >= Nd) continue;
            int nc = ni * Nd + nj;
            if (seen[nc]) continue;
            seen[nc] = 1;
            T.lvl[nc] = std::max(hr[nc], f);
            T.dir[nc] = (uint8_t)(7 - k);   // from nc back to c
            pq.push({T.lvl[nc], nc});
        }
    }
    // the depressions: the flooded cells (filled over the ground) grouped by their spill level; one at least LAKE_DEPTH
    // deep at its pit and at most LAKE_AREA_MAX across stays a lake, the others are breached: a river cuts out through the rim
    {
        std::vector<int> comp(n, -1);
        std::vector<int> stack, members;
        for (int c0 = 0; c0 < n; c0++) {
            if (comp[c0] >= 0 || !(flag[c0] & FL_VALID) || (flag[c0] & FL_SEA) || T.lvl[c0] <= hr[c0] + 0.3f) continue;
            float spill = T.lvl[c0], minH = hr[c0]; int pit = c0; bool cut = false;
            stack.clear(); members.clear(); stack.push_back(c0); comp[c0] = c0;
            while (!stack.empty()) {
                int c = stack.back(); stack.pop_back();
                members.push_back(c);
                if (hr[c] < hr[pit]) pit = c;
                minH = std::min(minH, hr[c]);
                int i = c / Nd, j = c % Nd;
                if (i <= 1 || j <= 1 || i >= Nd - 2 || j >= Nd - 2) cut = true;   // the domain's edge cut this depression: its spill is not the ground's
                for (int k = 0; k < 8; k++) {
                    int ni = i + OFF_I[k], nj = j + OFF_J[k];
                    if (ni < 0 || nj < 0 || ni >= Nd || nj >= Nd) continue;
                    int nc = ni * Nd + nj;
                    if (comp[nc] >= 0 || !(flag[nc] & FL_VALID) || (flag[nc] & FL_SEA) || T.lvl[nc] <= hr[nc] + 0.3f || std::fabs(T.lvl[nc] - spill) > 0.01f) continue;
                    comp[nc] = c0; stack.push_back(nc);
                }
            }
            double lat0 = (-90.0 + (T.i0 + c0 / Nd + 0.5) * dDeg) * DEG;
            double areaKm2 = members.size() * dRad * g.R * dRad * g.R * std::max(0.05, std::cos(lat0));
            double depth = spill - minH;
            // a small depression fills to its spill; a big one (or one the domain's edge cut, whose spill is not the ground's)
            // is breached part way: its outlet gorge is cut at most BREACH_MAX below the spill, the water stands at the gorge's
            // floor. A full breach dug trenches 1.6 km deep with vertical walls out of the highland basins
            if (depth >= LAKE_DEPTH) {
                bool keep = !cut && areaKm2 <= LAKE_AREA_MAX && unitFromHash(hash2i(T.i0 + pit / Nd, T.j0 + pit % Nd, g.seed ^ 0x1AC3ULL)) < LAKE_KEEP;   // the pit's global cell: the same answer in every tile that sees the basin
                float L = keep ? spill : (float)(spill - std::min(depth - 4.0, std::min(breachDepth(areaKm2), CARVE_MAX - 20.0)));
                if (L > minH + 0.5f) {
                    for (int c : members) if (hr[c] < L - 0.3f) { T.flag[c] |= FL_LAKE; T.lvl[c] = L; }
                    T.lakesKept++;
                } else T.lakesBreached++;
            } else T.lakesBreached++;

        }
    }
    // the routing surface carved: a lake lies flat at its spill; every other pit is breached along the least-cost path
    // out of it (the cost the ground over the pit's level: the path follows the low ground, so the trench curves like a
    // valley), the path cut into a ramp that descends from the pit to ground lower than it. The flow directions are then
    // the steepest descent of that surface: the flood's own directions on the flats were the scan order, dead-straight
    // reaches with combs of parallel streams across every flat floor. A lake's cells keep the flood's directions
    std::vector<float> hc = hr;
    for (int c = 0; c < n; c++) if (T.flag[c] & FL_LAKE) hc[c] = T.lvl[c];
    auto neighbourOf = [&](int c, int k) { int i = c / Nd + OFF_I[k], j = c % Nd + OFF_J[k]; return (i < 0 || j < 0 || i >= Nd || j >= Nd) ? -1 : i * Nd + j; };
    {
        std::vector<int> stamp(n, 0), par(n, -1);
        std::vector<float> cost(n, 0.f);
        int cur = 0;
        using CE = std::pair<float, int>;
        std::priority_queue<CE, std::vector<CE>, std::greater<CE>> q;
        std::vector<int> path;
        auto breach = [&](int m) {
            float base = hc[m];
            cur++;
            while (!q.empty()) q.pop();
            stamp[m] = cur; par[m] = -1; cost[m] = 0; q.push({0.f, m});
            int found = -1;
            while (!q.empty()) {
                CE e = q.top(); q.pop();
                int c = e.second;
                if (e.first > cost[c]) continue;
                if (hc[c] < base) { found = c; break; }
                for (int k = 0; k < 8; k++) {
                    int nc = neighbourOf(c, k);
                    if (nc < 0 || !(flag[nc] & FL_VALID) || (T.flag[nc] & FL_LAKE)) continue;
                    float step = 0.3f * std::max(0.f, hc[nc] - base) + 0.002f + 0.05f * (hr[nc] - h[nc] + 1.2f);   // over the pit's level, a step, and the lie of the land
                    if (stamp[nc] != cur || cost[c] + step < cost[nc]) { stamp[nc] = cur; cost[nc] = cost[c] + step; par[nc] = c; q.push({cost[nc], nc}); }
                }
            }
            if (found < 0) return;
            path.clear();
            for (int c = found; c != m; c = par[c]) path.push_back(c);
            // the ramp from the pit down along the path (the last cell is `found`, already lower); a cell already lower stays
            float v = base;
            for (int k = (int)path.size() - 1; k >= 0; k--) {
                int c = path[k];
                v = std::min(hc[c], v - 0.005f);
                hc[c] = v;
            }
            // the ramp may have run under `found`: carry on down its steepest descent until the ground is lower than the ramp
            int c = found;
            for (int guard = 0; guard < n; guard++) {
                int best = -1; float bh = 1e30f;
                for (int k = 0; k < 8; k++) { int nc = neighbourOf(c, k); if (nc >= 0 && (flag[nc] & FL_VALID) && hc[nc] < bh) { bh = hc[nc]; best = nc; } }
                if (best < 0 || bh < hc[c]) break;
                v = hc[c] - 0.005f;
                if (T.flag[best] & FL_LAKE) break;
                hc[best] = v; c = best;
            }
        };
        for (int c = 0; c < n; c++) {
            if (T.dir[c] == 8 || (T.flag[c] & FL_LAKE) || !(flag[c] & FL_VALID)) continue;
            bool lower = false;
            for (int k = 0; k < 8 && !lower; k++) { int nc = neighbourOf(c, k); if (nc >= 0 && hc[nc] < hc[c]) lower = true; }
            if (!lower) breach(c);
        }
    }
    std::vector<int> popIndex(n, 0);
    for (int q = 0; q < (int)order.size(); q++) popIndex[order[q]] = q;
    for (int c = 0; c < n; c++) {
        if (T.dir[c] == 8 || (T.flag[c] & FL_LAKE) || !(flag[c] & FL_VALID)) continue;
        int best = -1; float bh = hc[c];
        for (int k = 0; k < 8; k++) { int nc = neighbourOf(c, k); if (nc >= 0 && hc[nc] < bh) { bh = hc[nc]; best = k; } }
        if (best >= 0) T.dir[c] = (uint8_t)best;
    }
    // a topological order of the new directions: the higher cells first; a flat lake by the flood's order (parents first)
    std::vector<int> topo(n);
    for (int c = 0; c < n; c++) topo[c] = c;
    std::sort(topo.begin(), topo.end(), [&](int a, int b) { if (hc[a] != hc[b]) return hc[a] > hc[b]; return popIndex[a] > popIndex[b]; });
    // accumulation (km^2) down the new directions
    for (int i = 0; i < Nd; i++) {
        double lat = (-90.0 + (T.i0 + i + 0.5) * dDeg) * DEG;
        float area = (float)(dRad * g.R * dRad * g.R * std::max(0.05, std::cos(lat)));
        for (int j = 0; j < Nd; j++) T.acc[i * Nd + j] = area;
    }
    for (int c : topo) {
        if (T.dir[c] == 8) continue;
        int pc = neighbourOf(c, T.dir[c]);
        if (pc >= 0) T.acc[pc] += T.acc[c];
    }
    {   // the levels: a kept lake's cells sit at the spill, every other cell at its own ground, and no cell higher than
        // any cell upstream of it (the upstream cells come first in the order)
        std::vector<float> lv(n);
        for (int c = 0; c < n; c++) lv[c] = (T.flag[c] & FL_LAKE) ? T.lvl[c] : h[c];
        for (int c : topo) {
            if (T.dir[c] == 8) continue;
            int pc = neighbourOf(c, T.dir[c]);
            if (pc >= 0 && lv[c] < lv[pc]) lv[pc] = std::max(lv[c], (T.flag[pc] & FL_LAKE) ? lv[c] : h[pc] - (float)CARVE_MAX);   // never deeper than CARVE_MAX under the ground
        }
        T.lvl = lv;
    }
    // the main upstream neighbour (the biggest of those flowing in)
    for (int c = 0; c < n; c++) {
        float best = 0; int bk = 8;
        for (int k = 0; k < 8; k++) {
            int nc = neighbourOf(c, k);
            if (nc >= 0 && T.dir[nc] == 7 - k && T.acc[nc] > best) { best = T.acc[nc]; bk = k; }
        }
        T.up[c] = (uint8_t)bk;
    }
    T.buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

void reapWorkers() {   // g_mutex held
    for (size_t k = 0; k < g_workers.size();) {
        if (g_workerDone[k]->load()) { if (g_workers[k].joinable()) g_workers[k].join(); g_workers.erase(g_workers.begin() + k); g_workerDone.erase(g_workerDone.begin() + k); }
        else k++;
    }
}

static double traceMs() { static const auto t0 = std::chrono::steady_clock::now(); return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }

std::shared_ptr<Tile> getTile(const BodyGen& g, uint64_t gkey, int tLat, int tLon) {
    Key key{gkey, tLat, tLon};
    std::unique_lock<std::mutex> lock(g_mutex);
    for (;;) {
        auto it = g_tiles.find(key);
        if (it == g_tiles.end()) { Entry e; e.computing = true; g_tiles[key] = e; break; }
        if (it->second.tile) { it->second.use = ++g_use; return it->second.tile; }
        g_cv.wait(lock);   // another thread is computing it
    }
    lock.unlock();
    auto t = std::make_shared<Tile>();
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
    auto tb0 = std::chrono::steady_clock::now();
    buildTile(g, tLat, tLon, *t);
    if (trace) fprintf(stderr, "    [%.0f] drainage tile %d %d built in %.1f ms (thread %zu)\n", traceMs(), tLat, tLon, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tb0).count(), std::hash<std::thread::id>()(std::this_thread::get_id()) % 1000);
    lock.lock();
    Entry& e = g_tiles[key];
    e.tile = t; e.computing = false; e.use = ++g_use;
    if ((int)g_tiles.size() > CACHE_CAP) {   // the least recently used finished tile goes (holders keep their shared pointers)
        auto victim = g_tiles.end();
        for (auto jt = g_tiles.begin(); jt != g_tiles.end(); ++jt) if (jt->second.tile && !jt->second.computing && (victim == g_tiles.end() || jt->second.use < victim->second.use)) victim = jt;
        if (victim != g_tiles.end() && victim->first.gen != key.gen) g_tiles.erase(victim);
        else if (victim != g_tiles.end() && (int)g_tiles.size() > CACHE_CAP + 8) g_tiles.erase(victim);
    }
    reapWorkers();
    g_cv.notify_all();
    return t;
}

// joins the prefetch threads before the cache above is destroyed (defined after it: destroyed before it)
struct Reaper { ~Reaper() { std::vector<std::thread> ws; { std::lock_guard<std::mutex> lock(g_mutex); ws.swap(g_workers); g_workerDone.clear(); } for (auto& t : ws) if (t.joinable()) t.join(); } } g_reaper;

// per thread: the tiles round the last query (a shared pointer each, so an evicted tile stays valid while in use)
struct TileSlot { int tLat = INT_MIN, tLon = INT_MIN; std::shared_ptr<Tile> t; };
struct ThreadTiles { uint64_t gkey = 0; TileSlot slots[12]; int next = 0; };
thread_local ThreadTiles t_tiles;

const Tile* tileFor(const BodyGen& g, uint64_t gkey, int tLat, int tLon) {
    if (t_tiles.gkey != gkey) { for (auto& s : t_tiles.slots) { s.t.reset(); s.tLat = INT_MIN; } t_tiles.gkey = gkey; }
    for (auto& s : t_tiles.slots) if (s.tLat == tLat && s.tLon == tLon && s.t) return s.t.get();
    TileSlot& s = t_tiles.slots[t_tiles.next]; t_tiles.next = (t_tiles.next + 1) % 12;
    s.tLat = tLat; s.tLon = tLon; s.t = getTile(g, gkey, tLat, tLon);
    return s.t.get();
}

struct CellVal { double lvl = 0, acc = 0, lake = 0, lakeLvl = 0; int dir = 8, up = 8; bool valid = false, sea = false; };   // lake/lakeLvl: the own tile's (a tile blends nothing of another's lakes: a basin cut by the other's domain edge looks like another lake there)
struct Ctx { const BodyGen* g; uint64_t gkey; int N0; };

// the blended values of a lattice cell: its own tile's, and, near a tile's edge, the neighbouring tiles' estimates for
// the same cell weighted by the cell's distance to their domain's edge, so a river keeps its width across a border
bool readCell(const Ctx& c, int I, int J, CellVal& out) {
    const int N0 = c.N0;
    if (I < 0 || I >= 180 * N0) return false;
    int W = 360 * N0;
    J = ((J % W) + W) % W;
    int tLat = I / N0, tLon = J / N0;
    const Tile* T0 = tileFor(*c.g, c.gkey, tLat, tLon);
    if (!T0 || T0->empty) return false;
    int k0 = T0->index(I, J);
    if (k0 < 0 || !(T0->flag[k0] & FL_VALID)) return false;
    out.valid = true; out.sea = (T0->flag[k0] & FL_SEA) != 0; out.dir = T0->dir[k0]; out.up = T0->up[k0];
    out.lake = (T0->flag[k0] & FL_LAKE) ? 1.0 : 0.0; out.lakeLvl = T0->lvl[k0];
    double wsum = 1, lvl = T0->lvl[k0], acc = T0->acc[k0];
    int mi = I % N0, mj = J % N0;
    int dI = mi < T0->M ? -1 : (mi >= N0 - T0->M ? 1 : 0), dJ = mj < T0->M ? -1 : (mj >= N0 - T0->M ? 1 : 0);
    const double blendCells = BLEND_DEG * N0;
    auto addFrom = [&](int tl, int tn) {
        if (tl < 0 || tl >= 180) return;
        tn = ((tn % 360) + 360) % 360;
        const Tile* T = tileFor(*c.g, c.gkey, tl, tn);
        if (!T || T->empty) return;
        int k = T->index(I, J);
        if (k < 0 || !(T->flag[k] & FL_VALID)) return;
        double w = smoothstep(0.0, blendCells, (double)T->edgeDist(I, J));
        if (w <= 0) return;
        wsum += w; lvl += w * T->lvl[k]; acc += w * T->acc[k];
    };
    if (dI) addFrom(tLat + dI, tLon);
    if (dJ) addFrom(tLat, tLon + dJ);
    if (dI && dJ) addFrom(tLat + dI, tLon + dJ);
    out.lvl = lvl / wsum; out.acc = acc / wsum;
    return true;
}

// the channel's node point of a cell: its centre jittered by up to 0.3 cells (lattice units)
inline void nodeUV(uint64_t seed, int I, int J, double& u, double& v) {
    uint64_t h = hash2i(I, J, seed ^ 0xD8A1ULL);
    u = I + 0.5 + 0.5 * (unitFromHash(h) - 0.5) + 0.36 * gnoise2(I / 4.7 + 0.3, J / 4.7, seed ^ 0xD8A2ULL);
    v = J + 0.5 + 0.5 * (unitFromHash(mix64(h + 1)) - 0.5) + 0.36 * gnoise2(I / 4.7, J / 4.7 + 0.7, seed ^ 0xD8A3ULL);
}
inline double widthOf(double accKm2) { return 4.0 + 1.2 * std::sqrt(std::max(0.0, accKm2)); }   // metres, the full width
inline double depthOf(double accKm2) { return 1.5 + 0.35 * std::pow(std::max(0.0, accKm2), 0.35); }
// the accumulation a channel needs, km^2: the body's river density (lake country 0.9: 3 km^2; a dry world 0.4: 12)
inline double channelA0(const BodyGen& g) {
    double d = clampd((g.riverDensity - 0.4) / 0.5, 0, 1);
    double a0 = 3.0 + 9.0 * (1 - d);
    if (g.type == PT_DESERT || g.type == PT_THINATMO) a0 *= 2.2;
    else if (g.type != PT_FELISIAN) a0 *= 1.5;
    return a0;
}

// one reach: the curve from a channel cell's node to its downstream node, five points in lattice units, its ends' levels
// and accumulations (interpolated along it), the flatness of its two ends (the meander swing)
struct Seg { double u[5], v[5]; double lvl0, lvl1, acc0, acc1; double lake0; };
struct Neigh {
    uint64_t gkey = 0; int N0 = -1, I = INT_MIN, J = INT_MIN; bool coarse = false, brooks = false;
    std::vector<Seg> segs;
    double lake[5][5], lvl[5][5], lakeLvl[5][5]; bool have[5][5];
};
thread_local Neigh t_neigh;

inline Vec3 unitOfUV(const Ctx& c, double u, double v) {
    double dDeg = 1.0 / c.N0;
    return StarSystem::bodyFromLatLon((-90.0 + u * dDeg) * DEG, (v * dDeg - 180.0) * DEG);
}

void buildNeigh(const Ctx& c, int I, int J, double cellLatM, double cellLonM, bool coarse, double detailM) {
    Neigh& N = t_neigh;
    N.gkey = c.gkey; N.N0 = c.N0; N.I = I; N.J = J; N.coarse = coarse;
    N.brooks = detailM < 300.0;   // the brooks' valleys (Wv 200 m) show at 64 m and finer only; the 512 m ring skips them (a third of its fill)
    N.segs.clear();
    const double a0 = channelA0(*c.g);
    const double accMin = a0 * (N.brooks ? 0.1 : 0.4);
    if (coarse) {
        // the 2048 m ring samples a new lattice cell every time, and rebuilt the curved, blended neighbourhood at each
        // sample (9 us, half the ring's fill). At that scale a cell is a quarter of a sample: the own tile's raw cells
        // serve, the reaches run straight from a cell's centre to its downstream cell's, and only the channels whose
        // valleys can show at the scale are listed (a valley shows from Wv = 25 w > 0.4 x detail)
        int W = 360 * c.N0, Jw = ((J % W) + W) % W;
        const Tile* T0 = (I >= 0 && I < 180 * c.N0) ? tileFor(*c.g, c.gkey, I / c.N0, Jw / c.N0) : nullptr;
        if (T0 && T0->empty) T0 = nullptr;
        for (int di = -2; di <= 2; di++)
            for (int dj = -2; dj <= 2; dj++) {
                int kk = T0 ? T0->index(I + di, Jw + dj) : -1;
                bool ok = kk >= 0 && (T0->flag[kk] & FL_VALID);
                N.have[di + 2][dj + 2] = ok;
                N.lake[di + 2][dj + 2] = ok && (T0->flag[kk] & FL_LAKE) ? 1.0 : 0.0;
                N.lvl[di + 2][dj + 2] = ok ? T0->lvl[kk] : 0;
                N.lakeLvl[di + 2][dj + 2] = ok ? T0->lvl[kk] : 0;
            }
        if (!T0) return;
        double wMin = 0.016 * detailM, accMin = std::max(0.4 * a0, std::pow(std::max(0.0, wMin - 4.0) / 1.2, 2.0));
        for (int di = -6; di <= 6; di++)
            for (int dj = -6; dj <= 6; dj++) {
                int Ic = I + di, Jc = Jw + dj;
                int kk = T0->index(Ic, Jc);
                if (kk < 0 || !(T0->flag[kk] & FL_VALID) || (T0->flag[kk] & FL_SEA) || T0->dir[kk] == 8 || T0->acc[kk] < accMin) continue;
                int Id = Ic + OFF_I[T0->dir[kk]], Jd = Jc + OFF_J[T0->dir[kk]];
                int kd = T0->index(Id, Jd);
                if (kd < 0 || !(T0->flag[kd] & FL_VALID)) continue;
                Seg s;
                s.lvl0 = T0->lvl[kk] - 1.0; s.lvl1 = T0->lvl[kd] - 1.0; s.acc0 = T0->acc[kk]; s.acc1 = std::max(T0->acc[kk], T0->acc[kd]); s.lake0 = (T0->flag[kk] & FL_LAKE) ? 1.0 : 0.0;
                for (int k = 0; k < 5; k++) { double t = k * 0.25; s.u[k] = Ic + 0.5 + (Id - Ic) * t; s.v[k] = J + dj + 0.5 + (Jd - Jc) * t; }
                N.segs.push_back(s);
            }
        return;
    }
    // one reach: the curve from a channel cell's node to its downstream node
    auto addSeg = [&](int Ic, int Jc, const CellVal& cv) {
        int Id = Ic + OFF_I[cv.dir], Jd = Jc + OFF_J[cv.dir];
        CellVal dv;
        if (!readCell(c, Id, Jd, dv)) return;
        double uc, vc, ud, vd, uu, vu, ue, ve;
        nodeUV(c.g->seed, Ic, Jc, uc, vc);
        nodeUV(c.g->seed, Id, Jd, ud, vd);
        if (cv.up != 8) nodeUV(c.g->seed, Ic + OFF_I[cv.up], Jc + OFF_J[cv.up], uu, vu); else { uu = 2 * uc - ud; vu = 2 * vc - vd; }
        if (!dv.sea && dv.dir != 8) nodeUV(c.g->seed, Id + OFF_I[dv.dir], Jd + OFF_J[dv.dir], ue, ve); else { ue = 2 * ud - uc; ve = 2 * vd - vc; }
        Seg s;
        s.lvl0 = cv.lvl - 1.0; s.lvl1 = dv.lvl - 1.0; s.acc0 = cv.acc; s.acc1 = std::max(cv.acc, dv.acc); s.lake0 = cv.lake;
        // the meander swing at each end: the grade there (a metre per kilometre and under is a flat reach)
        double len = std::sqrt((ud - uc) * (ud - uc) * cellLatM * cellLatM + (vd - vc) * (vd - vc) * cellLonM * cellLonM);
        double grade0 = (cv.lvl - dv.lvl) / std::max(50.0, len);
        double grade1 = grade0;
        if (!dv.sea && dv.dir != 8) { CellVal ev; if (readCell(c, Id + OFF_I[dv.dir], Jd + OFF_J[dv.dir], ev)) grade1 = (dv.lvl - ev.lvl) / std::max(50.0, len); }
        double flat0 = smoothstep(0.004, 0.0005, std::fabs(grade0)), flat1 = smoothstep(0.004, 0.0005, std::fabs(grade1));
        for (int k = 0; k < 5; k++) {
            double t = k * 0.25, t2 = t * t, t3 = t2 * t;
            // Catmull-Rom through the node points
            double u = 0.5 * ((2 * uc) + (-uu + ud) * t + (2 * uu - 5 * uc + 4 * ud - ue) * t2 + (-uu + 3 * uc - 3 * ud + ue) * t3);
            double v = 0.5 * ((2 * vc) + (-vu + vd) * t + (2 * vu - 5 * vc + 4 * vd - ve) * t2 + (-vu + 3 * vc - 3 * vd + ve) * t3);
            double tu = 0.5 * ((-uu + ud) + 2 * (2 * uu - 5 * uc + 4 * ud - ue) * t + 3 * (-uu + 3 * uc - 3 * ud + ue) * t2);
            double tv = 0.5 * ((-vu + vd) + 2 * (2 * vu - 5 * vc + 4 * vd - ve) * t + 3 * (-vu + 3 * vc - 3 * vd + ve) * t2);
            // the meander: a lateral swing by a noise of the place, up to 2.2 widths on a flat reach
            double acc = s.acc0 + (s.acc1 - s.acc0) * t, w = widthOf(acc), flat = flat0 + (flat1 - flat0) * t;
            double amp = std::min(std::max(2.2 * w, 0.3 * cellLatM) * flat, 0.4 * cellLatM), wl = std::max(10.0 * w, 2.5 * cellLatM);
            if (amp > 0.5) {
                double txm = tv * cellLonM, tzm = tu * cellLatM, tl = std::sqrt(txm * txm + tzm * tzm);
                if (tl > 1e-9) {
                    Vec3 q = unitOfUV(c, u, v) * (c.g->R / (wl * 0.001));
                    double n = gnoise3(q, c.g->sF ^ 0x3EA5ULL);
                    double ox = -tzm / tl * amp * n, oz = txm / tl * amp * n;   // metres, across the flow
                    v += ox / cellLonM; u += oz / cellLatM;
                }
            }
            s.u[k] = u; s.v[k] = v;
        }
        N.segs.push_back(s);
    };
    for (int di = -2; di <= 2; di++)
        for (int dj = -2; dj <= 2; dj++) {
            CellVal cv;
            bool ok = readCell(c, I + di, J + dj, cv);
            N.have[di + 2][dj + 2] = ok;
            N.lake[di + 2][dj + 2] = ok ? cv.lake : 0;
            N.lvl[di + 2][dj + 2] = ok ? cv.lvl : 0;
            N.lakeLvl[di + 2][dj + 2] = ok ? cv.lakeLvl : 0;
            if (!ok || cv.sea || cv.dir == 8 || cv.acc < accMin) continue;   // the brooks too at fine detail: their valleys and floors (a channel needs a0)
            addSeg(I + di, J + dj, cv);
        }
    for (int di = -6; di <= 6; di++)   // the big rivers of a second ring: their valleys reach 2.4 km (a cheap look at the own tile's accumulation first)
        for (int dj = -6; dj <= 6; dj++) {
            if (std::abs(di) <= 2 && std::abs(dj) <= 2) continue;
            int Ic = I + di, Jc = J + dj;
            if (Ic < 0 || Ic >= 180 * c.N0) continue;
            int W = 360 * c.N0; Jc = ((Jc % W) + W) % W;
            const Tile* T0 = tileFor(*c.g, c.gkey, Ic / c.N0, Jc / c.N0);
            if (!T0 || T0->empty) continue;
            int k0 = T0->index(Ic, Jc);
            if (k0 < 0 || T0->acc[k0] < 120.0f) continue;
            CellVal cv;
            if (!readCell(c, Ic, Jc, cv) || cv.sea || cv.dir == 8 || cv.acc < 150.0) continue;
            addSeg(Ic, Jc, cv);
        }
}
}

bool drainageLakeCell(const BodyGen& g, const Vec3& unit, double& level, bool buildTiles, bool& known) {
    known = true;
    if (!typeHasDrainage(g.type) || !g_enabled.load(std::memory_order_relaxed)) return false;
    double lat, lon;
    StarSystem::latLonFromBody(unit, lat, lon);
    if (std::fabs(lat) > MAX_LAT_DEG * DEG) return false;
    uint64_t gkey = genKey(g);
    int N0 = drainageLatticeN0(g.R);
    double dDeg = 1.0 / N0;
    int I = (int)std::floor((lat / DEG + 90.0) / dDeg), J = (int)std::floor(wrap2pi(lon + PI) / DEG / dDeg);
    if (I < 0 || I >= 180 * N0) return false;
    int W = 360 * N0; J = ((J % W) + W) % W;
    // the own tile alone: the lake flags are never blended, so no neighbour tile is built for this (the landmark
    // search's lake pass built the neighbours of every tile it looked at, for nothing, while the landing's first
    // frames ran)
    const Tile* T = nullptr; std::shared_ptr<Tile> hold;
    if (buildTiles) T = tileFor(g, gkey, I / N0, J / N0);
    else {   // only a tile already in the cache (the landing map's zoom must not compute tiles)
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_tiles.find(Key{gkey, I / N0, J / N0});
        if (it == g_tiles.end() || !it->second.tile) { known = false; return false; }
        hold = it->second.tile; T = hold.get();
    }
    if (!T || T->empty) return false;
    int k = T->index(I, J);
    if (k < 0 || !(T->flag[k] & FL_VALID) || !(T->flag[k] & FL_LAKE)) return false;
    level = T->lvl[k] - 0.5;
    return true;
}
DrainageOff::DrainageOff(bool off_) : off(off_) { if (off) t_noDrain++; }
DrainageOff::~DrainageOff() { if (off) t_noDrain--; }
bool typeHasDrainage(int type) { return type == PT_FELISIAN || type == PT_DESERT || type == PT_THINATMO || type == PT_HYDROCARBON || type == PT_ACIDIC; }
void setDrainageEnabled(bool on) { g_enabled.store(on); }
bool drainageEnabled() { return g_enabled.load(); }
int drainageLatticeN0(double radiusKm) { return clampi((int)std::lround(radiusKm * 1000.0 * DEG / CELL_M), 16, 1024); }
int drainageTilesHeld() { std::lock_guard<std::mutex> lock(g_mutex); return (int)g_tiles.size(); }

DrainInfo drainageAt(const BodyGen& g, const Vec3& unit, double detailM, double moist) {
    DrainInfo D;
    if (t_noDrain > 0 || !g_enabled.load(std::memory_order_relaxed) || !typeHasDrainage(g.type) || detailM > DETAIL_MAX) return D;
    {   // VESPERIS_TRACE=1: which thread first asks for drainage, and at what scale (a caller sampling finer than DETAIL_MAX builds tiles)
        static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
        thread_local bool first = true;
        if (trace && first) { first = false; fprintf(stderr, "    [%.0f] drainageAt: first call on thread %zu, detail %.0f m\n", traceMs(), std::hash<std::thread::id>()(std::this_thread::get_id()) % 1000, detailM); }
    }
    double lat, lon;
    StarSystem::latLonFromBody(unit, lat, lon);
    if (std::fabs(lat) > MAX_LAT_DEG * DEG) return D;
    Ctx c; c.g = &g; c.gkey = genKey(g); c.N0 = drainageLatticeN0(g.R);
    const double dDeg = 1.0 / c.N0;
    const double cellLatM = dDeg * DEG * g.R * 1000.0, cellLonM = cellLatM * std::max(0.05, std::cos(lat));
    double u = (lat / DEG + 90.0) / dDeg, v = wrap2pi(lon + PI) / DEG / dDeg;
    int I = (int)std::floor(u), J = (int)std::floor(v);
    Neigh& N = t_neigh;
    const bool coarse = detailM >= COARSE_DETAIL;
    if (N.gkey != c.gkey || N.N0 != c.N0 || N.I != I || N.J != J || N.coarse != coarse || N.brooks != (detailM < 300.0)) buildNeigh(c, I, J, cellLatM, cellLonM, coarse, detailM);
    // every reach of the neighbourhood: its distance, the level and accumulation at its nearest point, its floodplain's
    // strength here and its valley's; the channel that shapes the point is the one whose floodplain reaches it most (a
    // brook beside a big river does not hide the river's plain), the water surface is the reaches' levels blended by
    // nearness (the level of the nearest point alone jumped by metres across a meander's neck on a steep river), the
    // valley is the strongest of the valleys
    const double a0 = channelA0(g) * (0.5 + 2.5 * (1 - moist) * (1 - moist));   // a dry country's streams need a bigger catchment
    const double floorK = 0.6 + 0.4 * clampd(0.5 + 0.5 * gnoise3(unit * (g.R / 1.8), g.sF ^ 0x5F10ULL), 0, 1);   // the floor's width here, 0.6-1 (a 1.8 km noise)
    struct Hit { double d, s, lvl, acc, w, wh, pres, presA, zone, vz; int seg; };
    Hit hits[64]; int nh = 0;
    double lvlSum = 0, lvlW = 0;
    for (size_t k = 0; k < N.segs.size() && nh < 64; k++) {
        const Seg& s = N.segs[k];
        double best = 1e18, bs = 0;
        for (int q = 0; q < 4; q++) {
            double ax = (s.v[q] - v) * cellLonM, az = (s.u[q] - u) * cellLatM, bx = (s.v[q + 1] - v) * cellLonM, bz = (s.u[q + 1] - u) * cellLatM;
            double ex = bx - ax, ez = bz - az, l2 = ex * ex + ez * ez;
            double t = l2 > 1e-9 ? clampd(-(ax * ex + az * ez) / l2, 0, 1) : 0;
            double px = ax + ex * t, pz = az + ez * t, d2 = px * px + pz * pz;
            if (d2 < best) { best = d2; bs = (q + t) * 0.25; }
        }
        Hit& H = hits[nh++];
        H.seg = (int)k;
        H.d = std::sqrt(best); H.s = bs;
        H.acc = s.acc0 + (s.acc1 - s.acc0) * bs;
        H.lvl = s.lvl0 + (s.lvl1 - s.lvl0) * bs;
        H.w = widthOf(H.acc);
        H.wh = std::max(0.5 * H.w, 0.8 * detailM);   // a sheet drawn on the corners of cells needs a band 1.6 cells wide, or a diagonal channel breaks into blobs
        H.presA = smoothstep(0.7, 1.3, H.acc / a0);
        H.pres = H.presA * smoothstep(0.05 * detailM, 0.15 * detailM, H.w);
        H.zone = smoothstep(H.wh * 2.2, H.wh * 0.9, H.d) * H.pres;
        double Wv = clampd(25.0 * H.w, 200.0, 6.0 * cellLatM);
        // the valley of a stream too small to count as a channel here is still a valley (a mountain window's brooks)
        double gate = smoothstep(0.08, 0.3, H.acc / a0) * smoothstep(0.4 * detailM, 1.2 * detailM, Wv);
        double x = H.d / Wv;
        // the valley's floor: flat at the channel's level for ten widths either side (30-350 m half-width, narrower
        // by a slow noise of the place: fills in some valleys, gorges in others), then the V up to Wv. A V to the
        // channel alone left mountain valleys without a floor a buggy could follow (floors 2% against 15%)
        double xf = std::min(0.6, clampd(10.0 * H.w, 30.0, 350.0) * floorK / Wv);
        H.vz = (x < 1 && gate > 0) ? std::pow(smoothstep(1.0, xf, x), 1.5) * gate : 0.0;
        double wgt = 1.0 / ((H.d + H.wh) * (H.d + H.wh));
        lvlSum += wgt * H.lvl; lvlW += wgt;
    }
    if (nh) {
        int ci = 0;   // the channel that shapes the point
        for (int k = 1; k < nh; k++) if (hits[k].zone > hits[ci].zone || (hits[k].zone == hits[ci].zone && hits[k].d < hits[ci].d)) ci = k;
        const Hit& H = hits[ci];
        D.near = true;
        D.dist = H.d; D.acc = H.acc; D.halfWidth = H.wh; D.depth = depthOf(H.acc); D.pres = H.pres; D.any = H.pres > 0.001;
        D.level = lvlW > 0 ? lvlSum / lvlW : H.lvl;
        int vi = 0;
        for (int k = 1; k < nh; k++) if (hits[k].vz > hits[vi].vz) vi = k;
        D.valley = hits[vi].vz; D.valleyLevel = hits[vi].lvl;
        D.wetness = std::max(D.valley, H.pres * smoothstep(3.5 * H.wh, 1.2 * H.wh, H.d));
        {   // the grade of the chosen reach
            const Seg& s = N.segs[H.seg];
            double segLen = 0;
            for (int q = 0; q < 4; q++) { double ex = (s.v[q + 1] - s.v[q]) * cellLonM, ez = (s.u[q + 1] - s.u[q]) * cellLatM; segLen += std::sqrt(ex * ex + ez * ez); }
            D.slope = (s.lvl0 - s.lvl1) / std::max(50.0, segLen);
        }
    }
    // the lakes: the flood's filled cells, bilinear over the four nearest cell centres; the level is the nearest lake
    // cell's, and only cells of that lake (the same spill, within half a metre) count toward the mask: two lakes side by
    // side at different levels blended into one sloping sheet otherwise
    {
        double fu = u - I - 0.5, fv = v - J - 0.5;
        int oi = fu < 0 ? -1 : 0, oj = fv < 0 ? -1 : 0;
        double tu = fu - oi, tv = fv - oj;
        double best = 0, lv0 = 0;
        // the cell the point lies in decides its lake; a neighbouring lake's fade reaches into it only where the cell's own
        // ground stands within a few metres of that lake's surface (a lake on a terrace must not flood the basin below it)
        if (N.have[2][2] && N.lake[2][2] > 0.5) { best = 1; lv0 = N.lakeLvl[2][2]; }
        else if (N.have[2][2])
            for (int a = 0; a < 2; a++)
                for (int b = 0; b < 2; b++) {
                    int ci = 2 + oi + a, cj = 2 + oj + b;
                    if (!N.have[ci][cj] || N.lake[ci][cj] <= 0 || N.lakeLvl[2][2] < N.lakeLvl[ci][cj] - 3.0) continue;
                    double wgt = (a ? tu : 1 - tu) * (b ? tv : 1 - tv) * N.lake[ci][cj];
                    if (wgt > best) { best = wgt; lv0 = N.lakeLvl[ci][cj]; }
                }
        if (best > 0) {
            double m = 0;
            for (int a = 0; a < 2; a++)
                for (int b = 0; b < 2; b++) {
                    int ci = 2 + oi + a, cj = 2 + oj + b;
                    if (!N.have[ci][cj] || N.lake[ci][cj] <= 0 || std::fabs(N.lakeLvl[ci][cj] - lv0) > 0.5) continue;
                    m += (a ? tu : 1 - tu) * (b ? tv : 1 - tv) * N.lake[ci][cj];
                }
            if (m > 0.02) { D.lake = clampd(m, 0, 1); D.lakeLevel = lv0 - 0.5; }
        }
    }
    return D;
}

// the tiles a query within radiusM of a point may read: its own, and the neighbours the blend reads within BLEND_DEG
// of a tile's edge (a landing's first frame built those two on the main thread, 300 ms, until the range included them)
static void tileRange(const BodyGen& g, const Vec3& unit, double radiusM, double& lat, double& lon, int& la0, int& la1, int& lo0, int& lo1) {
    StarSystem::latLonFromBody(unit, lat, lon);
    double rDeg = radiusM / (g.R * 1000.0) / DEG + BLEND_DEG + 10.0 / drainageLatticeN0(g.R);
    la0 = (int)std::floor(lat / DEG + 90.0 - rDeg); la1 = (int)std::floor(lat / DEG + 90.0 + rDeg);
    double lonR = rDeg / std::max(0.1, std::cos(lat));
    lo0 = (int)std::floor(lon / DEG + 180.0 - lonR); lo1 = (int)std::floor(lon / DEG + 180.0 + lonR);
}

void drainagePrefetch(const BodyGen& g, const Vec3& unit, double radiusM, bool async) {
    if (!typeHasDrainage(g.type) || !g_enabled.load()) return;
    double lat, lon; int la0, la1, lo0, lo1;
    tileRange(g, unit, radiusM, lat, lon, la0, la1, lo0, lo1);
    std::vector<std::pair<int, int>> want;
    for (int a = la0; a <= la1; a++) {
        if (a < 0 || a >= 180 || std::fabs(-90.0 + a + 0.5) > MAX_LAT_DEG) continue;
        for (int b = lo0; b <= lo1 && b < lo0 + 360; b++) want.push_back({a, ((b % 360) + 360) % 360});
    }
    if (want.empty()) return;
    uint64_t gkey = genKey(g);
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
    if (trace) fprintf(stderr, "    [%.0f] drainage prefetch %s: type %d lat %.2f lon %.2f radius %.0f m: tiles %d..%d x %d..%d (%zu)\n", traceMs(), async ? "async" : "sync", g.type, lat / DEG, lon / DEG, radiusM, la0, la1, lo0, lo1, want.size());
    BodyGen gc = g;
    // a later prefetch supersedes an earlier one's remaining tiles: the landing map's cursor moved on, and the old site's
    // worker went on computing nine tiles into the landing's first frames, which it slowed by half
    static std::atomic<uint64_t> gen{0};
    uint64_t myGen = ++gen;
    auto work = [gc, gkey, want, myGen]() { for (auto& w : want) { if (gen.load() != myGen) break; getTile(gc, gkey, w.first, w.second); } };
    if (!async) { for (auto& w : want) getTile(gc, gkey, w.first, w.second); return; }
    std::lock_guard<std::mutex> lock(g_mutex);
    reapWorkers();
    auto done = std::make_shared<std::atomic<bool>>(false);
    g_workerDone.push_back(done);
    g_workers.emplace_back([work, done]() { work(); done->store(true); });
}

bool drainageReady(const BodyGen& g, const Vec3& unit, double radiusM) {
    if (!typeHasDrainage(g.type) || !g_enabled.load()) return true;
    double lat, lon; int la0, la1, lo0, lo1;
    tileRange(g, unit, radiusM, lat, lon, la0, la1, lo0, lo1);
    uint64_t gkey = genKey(g);
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
    if (trace) fprintf(stderr, "    [%.0f] drainage ready? type %d lat %.2f lon %.2f: tiles %d..%d x %d..%d\n", traceMs(), g.type, lat / DEG, lon / DEG, la0, la1, lo0, lo1);
    std::lock_guard<std::mutex> lock(g_mutex);
    for (int a = la0; a <= la1; a++) {
        if (a < 0 || a >= 180 || std::fabs(-90.0 + a + 0.5) > MAX_LAT_DEG) continue;
        for (int b = lo0; b <= lo1 && b < lo0 + 360; b++) {
            auto it = g_tiles.find(Key{gkey, a, ((b % 360) + 360) % 360});
            if (it == g_tiles.end() || !it->second.tile) return false;
        }
    }
    return true;
}

DrainStats drainageStats(const BodyGen& g, const Vec3& unit, int halfCells) {
    DrainStats st;
    if (!typeHasDrainage(g.type)) return st;
    double lat, lon;
    StarSystem::latLonFromBody(unit, lat, lon);
    Ctx c; c.g = &g; c.gkey = genKey(g); c.N0 = drainageLatticeN0(g.R);
    double dDeg = 1.0 / c.N0;
    int I = (int)std::floor((lat / DEG + 90.0) / dDeg), J = (int)std::floor(wrap2pi(lon + PI) / DEG / dDeg);
    const Tile* T = tileFor(g, c.gkey, I / c.N0, ((J / c.N0) % 360 + 360) % 360);
    if (T) { st.msBuild = T->buildMs; st.lakesKept = T->lakesKept; st.lakesBreached = T->lakesBreached; }
    double a0 = channelA0(g);
    for (int di = -halfCells; di <= halfCells; di++)
        for (int dj = -halfCells; dj <= halfCells; dj++) {
            CellVal v;
            if (!readCell(c, I + di, J + dj, v)) continue;
            st.cells++;
            if (v.sea) { st.sea++; continue; }
            if (v.lake > 0.5) st.lakes++;
            st.maxAcc = std::max(st.maxAcc, v.acc);
            if (v.acc >= a0 && v.dir != 8) {
                st.channels++;
                CellVal d;
                if (readCell(c, I + di + OFF_I[v.dir], J + dj + OFF_J[v.dir], d)) { st.tested++; if (d.lvl <= v.lvl + 1e-3) st.monotone++; }
            }
        }
    return st;
}
