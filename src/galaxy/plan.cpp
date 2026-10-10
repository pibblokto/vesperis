// T-01 (2026-10-09): a people's map of its world (see plan.h)
#include "plan.h"
#include "history.h"
#include "works.h"
#include "peoples.h"
#include "drainage.h"
#include "core/rng.h"
#include "core/noise.h"
#include "core/parallel.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <queue>
#include <thread>

const char* const NET_KIND_NAMES[NK_COUNT] = {"path", "road", "rail", "canal", "highway", "maglev", "sea crossing"};
const char* const NET_KIND_HUD[NK_COUNT] = {"PATH", "ROAD", "RAILWAY", "CANAL", "HIGHWAY", "MAGLEV", "CROSSING"};

namespace {
constexpr int PLAN_ROWS = 360;   // half a degree a row: about 165,000 cells, 30-50 km across on the peoples' worlds
double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
bool wetSample(const SurfaceSample& s) { return s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5; }
double arcKm(const Vec3& a, const Vec3& b, double R) { return std::acos(clampd(dot(a, b), -1, 1)) * R; }
// a point `eastM` and `northM` off a unit vector, on the tangent plane's great circles (the roads' and the site's frame)
Vec3 offsetUnit(const Vec3& u, double eastM, double northM, double Rm) {
    Vec3 pole(0, 0, 1), east = cross(pole, u);
    if (length(east) < 1e-9) east = Vec3(0, 1, 0);
    east = normalize(east);
    const Vec3 north = cross(u, east);
    const double d = std::sqrt(eastM * eastM + northM * northM);
    if (d < 1e-9) return u;
    const Vec3 dir = (east * eastM + north * northM) / d;
    return normalize(u * std::cos(d / Rm) + dir * std::sin(d / Rm));
}
Vec3 slerpUnit(const Vec3& a, const Vec3& b, double t) {
    const double c = clampd(dot(a, b), -1, 1), w = std::acos(c);
    if (w < 1e-9) return a;
    const double s = std::sin(w);
    return normalize(a * (std::sin((1 - t) * w) / s) + b * (std::sin(t * w) / s));
}
// a unit vector moved `m` metres toward another along their great circle
Vec3 toward(const Vec3& from, const Vec3& to, double m, double Rm) {
    const double w = std::acos(clampd(dot(from, to), -1, 1));
    if (w < 1e-12) return from;
    return slerpUnit(from, to, std::min(1.0, m / Rm / w));
}
}   // namespace

// ---- the grid ----------------------------------------------------------------------------------------------------------------
int PlanGrid::rowOf(double lat) const { return clampi((int)std::floor((lat + PI / 2) / dLat), 0, rows - 1); }
int PlanGrid::cellOf(double lat, double lon) const {
    const int i = rowOf(lat), n = rowCols[i];
    int j = (int)std::floor((wrapAngle(lon) + PI) / TAU * n);
    j = ((j % n) + n) % n;
    return rowStart[i] + j;
}
int PlanGrid::cellOf(const Vec3& u) const { double la, lo; StarSystem::latLonFromBody(u, la, lo); return cellOf(la, lo); }
void PlanGrid::rowCol(int cell, int& i, int& j) const {
    i = (int)(std::upper_bound(rowStart.begin(), rowStart.begin() + rows, cell) - rowStart.begin()) - 1;
    j = cell - rowStart[i];
}
void PlanGrid::latLonOf(int cell, double& lat, double& lon) const {
    int i, j; rowCol(cell, i, j);
    lat = -PI / 2 + (i + 0.5) * dLat;
    lon = -PI + (j + 0.5) * TAU / rowCols[i];
}
Vec3 PlanGrid::unitOf(int cell) const { double la, lo; latLonOf(cell, la, lo); return StarSystem::bodyFromLatLon(la, lo); }
double PlanGrid::areaKm2(int cell) const {
    int i, j; rowCol(cell, i, j);
    return R * R * dLat * (TAU / rowCols[i]) * std::cos(-PI / 2 + (i + 0.5) * dLat);
}
int PlanGrid::neighbours(int cell, int out[8]) const {
    int i, j; rowCol(cell, i, j);
    const int n = rowCols[i];
    int k = 0;
    out[k++] = rowStart[i] + (j + 1) % n;
    out[k++] = rowStart[i] + (j + n - 1) % n;
    const double lon = -PI + (j + 0.5) * TAU / n;
    for (int di = -1; di <= 1; di += 2) {
        const int ii = i + di;
        if (ii < 0 || ii >= rows) continue;
        const int m = rowCols[ii];
        int jj = (int)std::floor((lon + PI) / TAU * m); jj = ((jj % m) + m) % m;
        out[k++] = rowStart[ii] + jj;
        out[k++] = rowStart[ii] + (jj + 1) % m;
        out[k++] = rowStart[ii] + (jj + m - 1) % m;
    }
    return k;
}

namespace {
// the fertility of a biome (the good soils of the temperate lands and the grasslands, the tropics' forests, the deserts' little)
double biomeFertility(int b) {
    switch (b) {
        case BIO_TROPICAL: return 0.75; case BIO_SAVANNA: return 0.6; case BIO_DESERT: return 0.12; case BIO_TEMPERATE: return 1.0;
        case BIO_GRASSLAND: return 0.85; case BIO_TAIGA: return 0.3; case BIO_TUNDRA: return 0.08; case BIO_WETLAND: return 0.5;
        case BIO_ALPINE: return 0.1; case BIO_ICE: case BIO_OCEAN: return 0.0; default: return 0.3;
    }
}
}

PlanGrid readPlanGrid(const BodyGen& g, bool parallel) {
    PlanGrid G;
    G.rows = PLAN_ROWS; G.dLat = PI / G.rows; G.R = g.R;
    G.rowStart.resize(G.rows + 1); G.rowCols.resize(G.rows);
    int acc = 0;
    for (int i = 0; i < G.rows; i++) {
        const double latc = -PI / 2 + (i + 0.5) * G.dLat;
        const int n = std::max(4, (int)std::lround(2.0 * G.rows * std::cos(latc)));
        G.rowCols[i] = n; G.rowStart[i] = acc; acc += n;
    }
    G.rowStart[G.rows] = acc; G.cells = acc;
    const int N = G.cells;
    G.height.assign(N, 0); G.land.assign(N, 0); G.biome.assign(N, 0); G.material.assign(N, 0); G.relief.assign(N, 0); G.veg.assign(N, 0);
    G.tempC.assign(N, 0); G.fert.assign(N, 0); G.coastKm.assign(N, 0); G.river.assign(N, 0); G.down.assign(N, -1); G.res.assign(N, 0); G.side.assign(N, 0);
    const double detail = G.sizeKm() * 1000.0 * 0.5;
    const int peoples = peoplesOf(g);
    const Vec3 divide = peoples > 1 ? peopleDivide(g) : Vec3(0, 0, 1);   // found before the workers (it is cached under a lock)
    auto readRange = [&](int c0, int c1) {
        DrainageOff off;   // per thread
        for (int c = c0; c < c1; c++) {
            double la, lo; G.latLonOf(c, la, lo);
            const Vec3 u = StarSystem::bodyFromLatLon(la, lo);
            const SurfaceSample s = sampleSurface(g, u, detail);
            G.height[c] = (float)s.height; G.land[c] = wetSample(s) ? 0 : 1;
            G.biome[c] = (uint8_t)s.biome; G.material[c] = (uint8_t)s.material;
            G.relief[c] = (float)s.relief; G.veg[c] = (float)s.veg;
            G.tempC[c] = (float)climateTempC(g, std::fabs(la) / DEG, std::max(0.0, s.height), 0, g.locked ? dot(u, g.lockedDir) : 0.0);
            G.side[c] = peoples > 1 && dot(u, divide) < 0 ? 1 : 0;
            // the coal of the basins at the mountains' feet and the ore of the mountains (the materials of the peoples' worlds are rock
            // whatever lies in it: a field of the world's own)
            const double basin = smoothstep(0.05, 0.45, gnoise3(u * 3.0, g.seed ^ 0x7011C0A1ULL));
            const double foot = smoothstep(0.12, 0.35, s.relief) * (1 - smoothstep(0.6, 0.85, s.relief));
            const double ore = smoothstep(0.5, 0.8, s.relief) * (0.5 + 0.5 * gnoise3(u * 5.0, g.seed ^ 0x7011E0EULL));
            G.res[c] = (float)clampd(std::max(basin * (0.35 + 0.65 * foot), 0.7 * ore), 0, 1);
            G.fert[c] = (float)(0.7 * biomeFertility(s.biome) + 0.3 * clampd(s.veg, 0, 1));
        }
    };
    if (parallel) parallelFor(N, 2048, readRange); else readRange(0, N);
    for (int c = 0; c < N; c++) if (G.land[c]) G.landKm2 += G.areaKm2(c);
    // the coast: hops from the water, a cell's size each
    {
        std::vector<int> q; q.reserve(N);
        std::vector<int> hops(N, -1);
        for (int c = 0; c < N; c++) if (!G.land[c]) { hops[c] = 0; q.push_back(c); }
        for (size_t h = 0; h < q.size(); h++) {
            int nb[8]; const int k = G.neighbours(q[h], nb);
            for (int t = 0; t < k; t++) if (hops[nb[t]] < 0) { hops[nb[t]] = hops[q[h]] + 1; q.push_back(nb[t]); }
        }
        for (int c = 0; c < N; c++) G.coastKm[c] = hops[c] < 0 ? 1e4f : (float)(std::max(0, hops[c] - 1) * G.sizeKm() + (G.land[c] ? 0.5 * G.sizeKm() : 0));
    }
    // the rivers: the land drains toward the sea over the grid, every pit filled (a priority flood from the water), and each cell
    // gathers the rain of the cells upstream (a dead desert's people knew its seas: their rivers ran into them)
    {
        typedef std::pair<float, int> HC;
        std::priority_queue<HC, std::vector<HC>, std::greater<HC>> pq;
        std::vector<char> done(N, 0);
        std::vector<float> fill(N, 0);
        std::vector<int> order; order.reserve(N);
        for (int c = 0; c < N; c++) if (!G.land[c]) { done[c] = 1; fill[c] = G.height[c]; pq.push(HC(-1e9f, c)); }
        if (pq.empty()) {   // a world of land alone: its lowest cell the sink
            int lo = 0; for (int c = 1; c < N; c++) if (G.height[c] < G.height[lo]) lo = c;
            done[lo] = 1; fill[lo] = G.height[lo]; pq.push(HC(G.height[lo], lo));
        }
        while (!pq.empty()) {
            const HC top = pq.top(); pq.pop();
            const int c = top.second;
            if (G.land[c]) order.push_back(c);
            int nb[8]; const int k = G.neighbours(c, nb);
            for (int t = 0; t < k; t++) {
                const int n = nb[t];
                if (done[n]) continue;
                done[n] = 1;
                G.down[n] = G.land[c] ? c : -1;   // into this cell, or straight into the sea
                fill[n] = std::max(G.height[n], G.land[c] ? fill[c] : G.height[n]);
                pq.push(HC(fill[n], n));
            }
        }
        const double meanArea = G.landKm2 > 0 ? 4 * PI * G.R * G.R / N : 1;
        const bool desert = g.type == PT_DESERT;
        for (int c = 0; c < N; c++) if (G.land[c]) G.river[c] = (float)((desert ? 0.6 : 0.25 + 0.75 * clampd(G.veg[c] * 1.3, 0, 1)) * G.areaKm2(c) / meanArea);
        for (int k = (int)order.size() - 1; k >= 0; k--) { const int c = order[k]; if (G.down[c] >= 0) G.river[G.down[c]] += G.river[c]; }
    }
    return G;
}

// ---- a people's plan -----------------------------------------------------------------------------------------------------------
namespace {
// the age's places and ways: the cities' and the towns' most and their spacing (km) as it travelled, the share of the people in its
// places, the ordinary way between them
struct AgeRule { int maxCities; double dCity; int maxTowns; double dTown; double urban; int way; };
const AgeRule RULES[AGE_COUNT] = {
    {0, 0, 40, 90, 0.05, NK_PATH},       // prehistoric: its gathering place (the capital) and the seats of its bands (villages), a day's walks apart
    {40, 260, 400, 70, 0.08, NK_ROAD},   // medieval: cities a week's ride apart, towns a day's
    {80, 170, 700, 45, 0.35, NK_ROAD},   // industrial: the rail's cities, its towns
    {120, 140, 900, 45, 0.65, NK_ROAD},  // modern
    {24, 500, 60, 250, 0.9, NK_MAGLEV},  // futuristic: its great places and the lesser (the land between left wild)
    {0, 0, 2, 1500, 1.0, -1}};           // far future: one to three works on a world
// the ground's score for an age: water and good land always; the hills for defence in the medieval age; the rivers, the coal and the
// ore in the industrial; the coasts in the modern; whatever a futuristic or far-future people chose; the genome's relation to the land
double scoreOf(const PlanGrid& G, int c, int age, int landGene, double noise, bool tempMatters) {
    if (!G.land[c]) return 0;
    if (G.material[c] == MAT_ICE || G.biome[c] == BIO_ICE) return 0;
    const double T = G.tempC[c];
    const double tempOk = tempMatters ? smoothstep(-14, 2, T) * (1 - smoothstep(32, 46, T)) : 1.0;
    const double alt = 1 - smoothstep(2200, 4200, G.height[c]);
    const double rough = G.relief[c];
    const double flat = 1 - smoothstep(0.3, 0.75, rough);
    const double coast = std::exp(-G.coastKm[c] / 60.0);
    const double riv = smoothstep(3, 40, G.river[c]);
    const double water = std::max(coast, riv);
    const double hills = smoothstep(0.12, 0.35, rough) * (1 - smoothstep(0.55, 0.8, rough));
    const double f = G.fert[c], veg = clampd(G.veg[c], 0, 1), res = G.res[c];
    double s;
    switch (age) {
        case AGE_PREHISTORIC: s = 0.35 * water + 0.25 * f + 0.15 * veg + 0.10 * flat + 0.15 * hills; break;
        case AGE_MEDIEVAL: s = 0.35 * f + 0.30 * water + 0.15 * flat + 0.20 * hills; break;
        case AGE_INDUSTRIAL: s = 0.30 * std::max(riv, 0.7 * coast) + 0.25 * res + 0.20 * f + 0.25 * flat; break;
        case AGE_MODERN: s = 0.40 * coast + 0.15 * riv + 0.20 * flat + 0.15 * f + 0.10 * res; break;
        case AGE_FUTURISTIC: s = 0.25 * flat + 0.20 * coast + 0.15 * f + 0.40 * noise; break;
        default: s = 0.5 * noise + 0.5 * std::max(smoothstep(0.4, 0.8, rough), flat * 0.8); break;   // a mountain to ring or a plain to make flat
    }
    switch (landGene) {   // A-04's relation to the land (KI-375): where the people chose to stand
        case LD_RIDGES: s += 0.25 * smoothstep(0.25, 0.55, rough); break;
        case LD_VALLEYS: s += 0.25 * riv; break;
        case LD_TERRACED: s += 0.20 * hills; break;
        case LD_CLIFFS: s += 0.25 * smoothstep(0.4, 0.7, rough); break;
        case LD_STILTS: s += 0.30 * std::max(std::max(coast, riv), G.biome[c] == BIO_WETLAND ? 1.0 : 0.0); break;
        case LD_MOUNDS: s += 0.15 * flat; break;
        case LD_PITS: s += 0.15 * (1 - veg); break;
        default: break;
    }
    return clampd(s, 0, 1.25) * tempOk * alt;
}
// a spatial hash of unit vectors (chord cells), for the spacing of the places
struct UnitHash {
    double s = 0.01;
    std::unordered_map<uint64_t, std::vector<int>> b;
    uint64_t key(int x, int y, int z) const { return ((uint64_t)(uint32_t)(x + 0x80000) << 42) ^ ((uint64_t)(uint32_t)(y + 0x80000) << 21) ^ (uint64_t)(uint32_t)(z + 0x80000); }
    void add(const Vec3& u, int id) { b[key((int)std::floor(u.x / s), (int)std::floor(u.y / s), (int)std::floor(u.z / s))].push_back(id); }
    template <class F> bool any(const Vec3& u, F fn) const {
        const int x = (int)std::floor(u.x / s), y = (int)std::floor(u.y / s), z = (int)std::floor(u.z / s);
        for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) for (int dz = -1; dz <= 1; dz++) {
            auto it = b.find(key(x + dx, y + dy, z + dz));
            if (it == b.end()) continue;
            for (int id : it->second) if (fn(id)) return true;
        }
        return false;
    }
};
// the drawn core of a capital or a city by the age (metres; the futuristic capital's great place, the prehistoric gathering place)
double coreRadius(int sclass, int age, double u) {
    static const double CAP[AGE_COUNT] = {110, 470, 820, 1150, 480, 0}, CITY[AGE_COUNT] = {90, 270, 500, 700, 230, 0};
    const double base = sclass == SC_CAPITAL ? CAP[age] : CITY[age];
    return base * (0.88 + 0.24 * u);
}
// how many buildings a core of that radius holds at the age (C-01's town: 34 in 75 m, its spacing the age's)
int coreBuildings(int sclass, int age, double R, double u) {
    if (age == AGE_FUTURISTIC) return sclass == SC_CAPITAL ? 14 + (int)(u * 5) : 8;
    static const double S[AGE_COUNT] = {0.85, 1.0, 1.85, 2.4, 1.0, 1.0};
    const double dens = 34.0 / (PI * 75.0 * 75.0) / (S[age] * S[age]);
    return std::max(30, (int)std::lround(dens * PI * R * R * 0.9));
}
}

int PeoplePlan::count(int sclass) const { int n = 0; for (const PlanPlace& p : places) if (p.sclass == sclass) n++; return n; }

// the score: the climate counts where the side has a habitable band (a world too hot or cold all over was the people's own)
void planScores(const BodyGen& g, const PlanGrid& G, int people, int age, std::vector<float>& out, const std::vector<uint8_t>* only) {
    age = clampi(age, 0, AGE_COUNT - 1);
    const int N = G.cells, peoples = peoplesOf(g);
    const uint64_t ps = peopleGen(g, people).seed;
    const int landGene = genomeOf(ps).of(GV_LAND);
    auto onSide = [&](int c) { return peoples < 2 || G.side[c] == people; };
    int landN = 0, tempN = 0;
    for (int c = 0; c < N; c++) if (G.land[c] && onSide(c)) { landN++; if (G.tempC[c] > -14 && G.tempC[c] < 40) tempN++; }
    const bool tempMatters = tempN > 0.05 * std::max(1, landN);
    out.assign(N, 0);
    double sMax = 1e-9;
    for (int c = 0; c < N; c++) {
        if (!G.land[c] || !onSide(c) || (only && !(*only)[c])) continue;
        double noise = 0;
        if (age >= AGE_FUTURISTIC) { const Vec3 u = G.unitOf(c); noise = 0.5 + 0.5 * gnoise3(u * 3.0, ps ^ 0x7011F0ULL); }
        out[c] = (float)scoreOf(G, c, age, landGene, noise, tempMatters);
        sMax = std::max(sMax, (double)out[c]);
    }
    for (int c = 0; c < N; c++) out[c] = (float)(out[c] / sMax);
}

void planPeople(const BodyGen& g, const PlanGrid& G, int people, int age, PeoplePlan& out) {
    const double t0 = nowMs();
    out = PeoplePlan();
    out.people = people; out.age = age = clampi(age, 0, AGE_COUNT - 1);
    const int N = G.cells, peoples = peoplesOf(g);
    const BodyGen pg = peopleGen(g, people);
    const uint64_t ps = pg.seed;
    const PeopleExtent ex = peopleExtentOf(g, people, age);
    const Culture cul = cultureAs(g, people, age, peopleAgeOf(g, people).endedYearsAgo);   // the culture at the age (the towns' radii)
    const AgeRule& rule = RULES[age];
    const double Rm = g.R * 1000.0;
    auto onSide = [&](int c) { return peoples < 2 || G.side[c] == people; };
    auto hashU = [&](int c, uint64_t salt) { return unitFromHash(hash2i(c, (int64_t)salt, ps ^ 0x7011A5ULL)); };
    planScores(g, G, people, age, out.score);
    // the capital: on the side's best ground, with a little of the people's own choice
    int cap = -1; double capKey = -1;
    for (int c = 0; c < N; c++) {
        if (!G.land[c] || !onSide(c) || out.score[c] <= 0) continue;
        const double k = out.score[c] * (0.8 + 0.3 * hashU(c, 1));
        if (k > capKey) { capKey = k; cap = c; }
    }
    if (cap < 0) { out.buildMs = nowMs() - t0; return; }   // no ground on its side
    // the territory: grown from the capital over the land it could reach, the dearer the poorer, the rougher or across the water (the
    // age's boats), until it holds its share of the world's land
    static const double WATER_K[AGE_COUNT] = {30, 6, 4, 2.5, 1.5, 1};
    const double target = ex.held * G.landKm2;
    std::vector<double> cost(N, 1e300);
    out.territory.assign(N, 0);
    out.remote.assign(N, -1);
    {
        typedef std::pair<double, int> DC;
        std::priority_queue<DC, std::vector<DC>, std::greater<DC>> pq;
        cost[cap] = 0; pq.push(DC(0, cap));
        double area = 0;
        std::vector<char> popped(N, 0);
        while (!pq.empty() && area < target) {
            const DC top = pq.top(); pq.pop();
            const int c = top.second;
            if (popped[c] || top.first > cost[c]) continue;
            popped[c] = 1;
            if (G.land[c]) { out.territory[c] = 1; area += G.areaKm2(c); }
            int nb[8]; const int k = G.neighbours(c, nb);
            for (int t = 0; t < k; t++) {
                const int n = nb[t];
                if (!onSide(n) || popped[n]) continue;
                const double step = (t < 2 || t == 2 || t == 5) ? 1.0 : 1.41;   // the row's and the straight neighbours, the diagonals
                double w;
                if (G.land[n]) w = (1 + 2.5 * (1 - out.score[n])) * (1 + 1.5 * smoothstep(0.5, 0.85, G.relief[n]));
                else w = WATER_K[age] * 2.0;
                const double nd = top.first + step * G.sizeKm() * w;
                if (nd < cost[n]) { cost[n] = nd; pq.push(DC(nd, n)); }
            }
        }
        out.areaKm2 = area;
        out.heldShare = G.landKm2 > 0 ? area / G.landKm2 : 0;
        double cMax = 1e-9;
        for (int c = 0; c < N; c++) if (out.territory[c]) cMax = std::max(cMax, cost[c]);
        for (int c = 0; c < N; c++) if (out.territory[c]) out.remote[c] = (float)(cost[c] / cMax);
    }
    // the places: the capital, then the cities and the towns on the territory's best ground (with the people's choice), spaced as the
    // age travelled; their counts by the territory's size, capped (a great territory's places stand farther apart)
    struct Want { int cell; int sclass; };
    std::vector<Want> wants;
    wants.push_back({cap, age == AGE_FAR_FUTURE ? SC_MONUMENT : SC_CAPITAL});
    std::vector<int> order;
    for (int c = 0; c < N; c++) if (out.territory[c] && out.score[c] > 0.05) order.push_back(c);
    std::vector<double> key(N, 0);
    for (int c : order) key[c] = out.score[c] * (0.7 + 0.6 * hashU(c, 2));
    std::sort(order.begin(), order.end(), [&](int a, int b) { return key[a] != key[b] ? key[a] > key[b] : a < b; });
    const double A = std::max(1.0, out.areaKm2);
    auto countFor = [&](int mx, double d) { return d <= 0 ? 0 : std::min(mx, (int)std::lround(A / (0.866 * d * d))); };
    int nCity = countFor(rule.maxCities, rule.dCity), nTown = countFor(rule.maxTowns, rule.dTown);
    if (age >= AGE_MEDIEVAL && age <= AGE_MODERN) nCity = std::max(nCity, std::min(rule.maxCities, 2));
    if (age == AGE_FAR_FUTURE) { const double u = unitFromHash(mix64(ps ^ 0x7011FAULL)); nTown = (u < 0.5 ? 1 : 0) + (u < 0.2 ? 1 : 0); }
    // the cities on the age's best ground (a modern people's on its coasts), the towns on any but the poorest
    double cityMin = 0, townMin = 0;
    {
        std::vector<float> sc; sc.reserve(order.size());
        for (int c : order) sc.push_back(out.score[c]);
        std::sort(sc.begin(), sc.end());
        static const double CITY_PCT[AGE_COUNT] = {0.3, 0.5, 0.5, 0.65, 0.5, 0.0};
        if (!sc.empty()) { cityMin = sc[(size_t)(CITY_PCT[age] * (sc.size() - 1))]; townMin = sc[(size_t)(0.12 * (sc.size() - 1))]; }
    }
    const double dCity = rule.maxCities ? std::max(rule.dCity, std::sqrt(A / (0.866 * rule.maxCities)) * 0.85) : 0;
    const double dTown = rule.maxTowns ? std::max(rule.dTown, std::sqrt(A / (0.866 * rule.maxTowns)) * 0.85) : 0;
    std::vector<Vec3> units(N);
    for (int c : order) units[c] = G.unitOf(c);
    units[cap] = G.unitOf(cap);
    UnitHash hashC, hashT;
    hashC.s = std::max(1e-4, dCity * 1000 / Rm); hashT.s = std::max(1e-4, dTown * 1000 / Rm);
    hashC.add(units[cap], cap); hashT.add(units[cap], cap);
    const int citySC = age == AGE_FUTURISTIC ? SC_TOWN : SC_CITY, townSC = age == AGE_PREHISTORIC || age == AGE_FUTURISTIC ? SC_VILLAGE : (age == AGE_FAR_FUTURE ? SC_MONUMENT : SC_TOWN);
    int made = 0;
    for (size_t k = 0; k < order.size() && made < nCity; k++) {
        const int c = order[k];
        if (c == cap || out.score[c] < cityMin) continue;
        const Vec3& u = units[c];
        if (hashC.any(u, [&](int o) { return arcKm(units[o], u, G.R) < dCity; })) continue;
        hashC.add(u, c); hashT.add(u, c); wants.push_back({c, citySC}); made++;
    }
    made = 0;
    for (size_t k = 0; k < order.size() && made < nTown; k++) {
        const int c = order[k];
        if (c == cap || out.score[c] < townMin) continue;
        const Vec3& u = units[c];
        if (hashT.any(u, [&](int o) { return arcKm(units[o], u, G.R) < dTown; })) continue;
        if (dCity > 0 && hashC.any(u, [&](int o) { return arcKm(units[o], u, G.R) < 0.5 * dCity; })) continue;
        hashT.add(u, c); wants.push_back({c, townSC}); made++;
    }
    // each place on the fine ground of its cell: dry where it stands and round its core (C-01's site test, and for a capital or a city
    // two rings more), the gentlest of a lattice of spots, by the water where the age wanted it (a harbour: water within a walk)
    const double cellM = G.sizeKm() * 1000.0;
    const bool wantsWater = age >= AGE_MEDIEVAL && age <= AGE_MODERN;
    std::vector<PlanPlace> placed;
    {
        DrainageOff off;
        for (size_t w = 0; w < wants.size(); w++) {
            const int c = wants[w].cell, sc = wants[w].sclass;
            const bool big = sc == SC_CAPITAL || sc == SC_CITY;
            const double u0 = hashU(c, 3);
            const double bigR = big ? coreRadius(sc, age, u0) : 0;
            const int m = big ? 5 : 3;
            const Vec3 cellU = G.unitOf(c);
            double bestQ = -1e9; PlanPlace best; bool found = false;
            for (int a = 0; a < m; a++)
                for (int b = 0; b < m; b++) {
                    const double e = ((a + 0.5) / m - 0.5 + (hashU(c, 10 + a * 7 + b) - 0.5) * 0.12) * 0.9 * cellM;
                    const double n = ((b + 0.5) / m - 0.5 + (hashU(c, 60 + a * 7 + b) - 0.5) * 0.12) * 0.9 * cellM;
                    const Vec3 u = offsetUnit(cellU, e, n, Rm);
                    double la, lo; StarSystem::latLonFromBody(u, la, lo);
                    if (std::fabs(la) > PI / 2 - 2 * ruinCellLat(g)) continue;
                    int gLat, gLon; ruinCellOf(g, la, lo, gLat, gLon);
                    const int nLon = std::max(1, ruinCellsAround(g, gLat)), gl = ((gLon % nLon) + nLon) % nLon;
                    const uint64_t seed = hash2i(gLat, gl, g.seed ^ 0x2711);
                    const double R = big ? bigR : settlementRadiusOf(cul, sc, seed);
                    RuinSpec probe; probe.kind = RK_SETTLEMENT; probe.lat = la; probe.lon = lo; probe.size = R;
                    if (!ruinSiteOk(g, probe)) continue;
                    double hmin = 1e18, hmax = -1e18; bool wet = false;
                    const int ringN = big ? 8 : 4;
                    for (int ring = 0; ring < (big ? 2 : 1) && !wet; ring++)
                        for (int k = 0; k < ringN; k++) {
                            const double ang = (k + 0.5 * ring + 0.25) * TAU / ringN, rr = ring == 0 ? R : 0.5 * R;
                            const SurfaceSample q = sampleSurface(g, offsetUnit(u, std::sin(ang) * rr, std::cos(ang) * rr, Rm), 64.0);
                            if (wetSample(q)) { wet = true; break; }
                            hmin = std::min(hmin, q.height); hmax = std::max(hmax, q.height);
                        }
                    if (wet) continue;
                    const double slope = (hmax - hmin) / (2 * std::max(R, 30.0));
                    double q = -3.0 * slope + 0.15 * hashU(c, 200 + a * 7 + b);
                    if (wantsWater && (big || sc == SC_TOWN)) {   // a harbour within a walk of the core's edge
                        bool shore = false;
                        for (int k = 0; k < 8 && !shore; k++) {
                            const double ang = k * TAU / 8, rr = R + 1500;
                            shore = wetSample(sampleSurface(g, offsetUnit(u, std::sin(ang) * rr, std::cos(ang) * rr, Rm), 64.0));
                        }
                        if (shore) q += age == AGE_MODERN ? 0.45 : 0.25;
                    }
                    if (q > bestQ) {
                        bestQ = q; found = true;
                        best = PlanPlace(); best.lat = la; best.lon = lo; best.unit = u; best.radiusM = R; best.gLat = gLat; best.gLon = gl; best.seed = seed;
                    }
                }
            if (!found) continue;
            PlanPlace p = best;
            p.sclass = sc; p.people = people; p.cell = c; p.score = out.score[c]; p.remote = std::max(0.0f, out.remote[c]);
            p.coast = G.coastKm[c] < 1.5 * G.sizeKm(); p.river = G.river[c] > 15;
            if (age == AGE_FUTURISTIC) p.plan = SP_GREAT;
            if (sc == SC_MONUMENT) p.plan = SP_LONE;
            if (big) p.buildings = coreBuildings(sc, age, p.radiusM, hashU(c, 4));
            placed.push_back(p);
        }
    }
    // the founding's order: the capital first, then the near and the good ground before the far and the poor, a city before a town
    if (!placed.empty()) {
        std::vector<int> idx(placed.size());
        for (size_t i = 0; i < idx.size(); i++) idx[i] = (int)i;
        auto fk = [&](const PlanPlace& p) { return 0.55 * p.remote + 0.45 * (1 - p.score) - (p.sclass == SC_CITY || (age == AGE_FUTURISTIC && p.sclass == SC_TOWN) ? 0.15 : 0.0); };
        std::sort(idx.begin() + 1, idx.end(), [&](int a, int b) { const double ka = fk(placed[a]), kb = fk(placed[b]); return ka != kb ? ka < kb : placed[a].cell < placed[b].cell; });
        for (size_t i = 0; i < idx.size(); i++) { out.places.push_back(placed[idx[i]]); out.places.back().rank = (int)i; }
    }
    // how many lived in them at the height: the age's share of the people in its places, by a weight a place (the capital the most)
    {
        double sumW = 0; std::vector<double> wts(out.places.size());
        for (size_t i = 0; i < out.places.size(); i++) {
            const PlanPlace& p = out.places[i];
            double w;
            switch (p.sclass) {
                case SC_CAPITAL: w = age == AGE_PREHISTORIC ? 1.0 : 3.0; break;
                case SC_CITY: w = 0.5 + p.score; break;
                case SC_TOWN: w = age == AGE_FUTURISTIC ? 0.6 + p.score : 0.06 + 0.12 * p.score; break;
                case SC_MONUMENT: w = 1.0; break;
                default: w = age == AGE_FUTURISTIC ? 0.2 : 0.3 + 0.2 * p.score; break;
            }
            wts[i] = w; sumW += w;
        }
        for (size_t i = 0; i < out.places.size(); i++) out.places[i].population = rule.urban * ex.peak * wts[i] / std::max(1e-9, sumW);
    }
    // the networks: a greedy spanner over each place's nearest neighbours (the cheapest links first, each kept only where the ways
    // already laid would go round by more than the age's detour: the near ways of a dense web, the long ones only where nothing else
    // joins), dearer over the water so a crossing is laid only where the land does not join; the components the neighbours leave apart
    // joined by their cheapest pair; the trunk the network's ways between the capital and the cities along the spanning tree of their
    // routes; the way's kind by the age (an industrial people's rail and canals, a modern people's highways, a crossing over the water)
    const int n = (int)out.places.size();
    if (n >= 2 && rule.way >= 0) {
        auto dKm = [&](int a, int b) { return arcKm(out.places[a].unit, out.places[b].unit, G.R); };
        auto waterRun = [&](int a, int b, int& longest) {   // the share of a straight way over water (the coarse grid, every third of a cell)
            const double L = dKm(a, b); const int steps = std::max(2, (int)std::ceil(L / (G.sizeKm() / 3)));
            int wetN = 0, run = 0; longest = 0;
            for (int k = 1; k < steps; k++) {
                const int cc = G.cellOf(slerpUnit(out.places[a].unit, out.places[b].unit, (double)k / steps));
                if (!G.land[cc]) { wetN++; run++; longest = std::max(longest, run); } else run = 0;
            }
            return (double)wetN / steps;
        };
        static const double DETOUR[AGE_COUNT] = {3.0, 1.7, 1.6, 1.5, 2.2, 2.0};
        const double detour = DETOUR[age];
        const int K = std::min(n - 1, 8);
        struct Cand { double cost; int a, b; };
        std::vector<Cand> cands;
        {
            std::map<std::pair<int, int>, bool> seen;
            for (int a = 0; a < n; a++) {
                std::vector<std::pair<double, int>> ds; ds.reserve(n - 1);
                for (int b = 0; b < n; b++) if (b != a) ds.push_back({dKm(a, b), b});
                std::partial_sort(ds.begin(), ds.begin() + K, ds.end());
                for (int k = 0; k < K; k++) {
                    const int lo = std::min(a, ds[k].second), hi = std::max(a, ds[k].second);
                    if (seen.count({lo, hi})) continue;
                    seen[{lo, hi}] = true;
                    int longest; const double ws = waterRun(lo, hi, longest);
                    cands.push_back({dKm(lo, hi) * (1 + 8 * ws + (longest >= 2 ? 3.0 : 0.0)), lo, hi});
                }
            }
        }
        std::sort(cands.begin(), cands.end(), [](const Cand& x, const Cand& y) { return x.cost != y.cost ? x.cost < y.cost : (x.a != y.a ? x.a < y.a : x.b < y.b); });
        std::vector<std::vector<std::pair<int, double>>> adj(n);
        std::map<std::pair<int, int>, double> chosen;   // the ways laid, by their ends: their cost
        std::vector<double> dist(n, 1e300);
        std::vector<int> touched;
        auto pathCost = [&](int from, int to, double cutoff) {   // the network's cheapest route so far, given up past the cutoff
            typedef std::pair<double, int> DC;
            std::priority_queue<DC, std::vector<DC>, std::greater<DC>> pq;
            for (int v : touched) dist[v] = 1e300;
            touched.clear();
            dist[from] = 0; touched.push_back(from); pq.push(DC(0, from));
            while (!pq.empty()) {
                const DC t = pq.top(); pq.pop();
                if (t.first > dist[t.second]) continue;
                if (t.second == to) return t.first;
                if (t.first > cutoff) break;
                for (const auto& nb : adj[t.second]) {
                    const double nd = t.first + nb.second;
                    if (nd < dist[nb.first]) { if (dist[nb.first] >= 1e299) touched.push_back(nb.first); dist[nb.first] = nd; pq.push(DC(nd, nb.first)); }
                }
            }
            return 1e300;
        };
        auto lay = [&](int a, int b, double c) { adj[a].push_back({b, c}); adj[b].push_back({a, c}); chosen[{std::min(a, b), std::max(a, b)}] = c; };
        for (const Cand& c : cands) if (pathCost(c.a, c.b, detour * c.cost) > detour * c.cost) lay(c.a, c.b, c.cost);
        for (;;) {   // the components the neighbours left apart: the cheapest pair between the capital's and another
            std::vector<char> inCap(n, 0); std::vector<int> q{0}; inCap[0] = 1;
            for (size_t h = 0; h < q.size(); h++) for (const auto& nb : adj[q[h]]) if (!inCap[nb.first]) { inCap[nb.first] = 1; q.push_back(nb.first); }
            if ((int)q.size() == n) break;
            double bc = 1e300; int ba = -1, bb = -1;
            for (int a = 0; a < n; a++) if (inCap[a]) for (int b = 0; b < n; b++) if (!inCap[b]) { const double d = dKm(a, b); if (d < bc) { bc = d; ba = a; bb = b; } }
            int longest; const double ws = waterRun(ba, bb, longest);
            lay(ba, bb, bc * (1 + 8 * ws + (longest >= 2 ? 3.0 : 0.0)));
        }
        // the trunk: the routes between the capital and the cities over the network, the spanning tree of the routes' costs; each route's
        // ways the trunk's
        std::map<std::pair<int, int>, bool> trunk;
        {
            std::vector<int> great;
            for (int i = 0; i < n; i++) if (i == 0 || out.places[i].sclass == SC_CITY || (age == AGE_FUTURISTIC && out.places[i].sclass == SC_TOWN)) great.push_back(i);
            const int m = (int)great.size();
            std::vector<std::vector<double>> rc(m, std::vector<double>(m, 1e300));
            std::vector<std::vector<int>> prevFrom(m);
            for (int i = 0; i < m; i++) {   // a full route search from each great place
                std::vector<double> d(n, 1e300); std::vector<int> prev(n, -1);
                typedef std::pair<double, int> DC;
                std::priority_queue<DC, std::vector<DC>, std::greater<DC>> pq;
                d[great[i]] = 0; pq.push(DC(0, great[i]));
                while (!pq.empty()) {
                    const DC t = pq.top(); pq.pop();
                    if (t.first > d[t.second]) continue;
                    for (const auto& nb : adj[t.second]) { const double nd = t.first + nb.second; if (nd < d[nb.first]) { d[nb.first] = nd; prev[nb.first] = t.second; pq.push(DC(nd, nb.first)); } }
                }
                for (int j = 0; j < m; j++) rc[i][j] = d[great[j]];
                prevFrom[i] = prev;
            }
            std::vector<double> best(m, 1e300); std::vector<int> from(m, -1); std::vector<char> in(m, 0);
            if (m > 0) best[0] = 0;
            for (int it = 0; it < m; it++) {
                int u = -1; for (int i = 0; i < m; i++) if (!in[i] && (u < 0 || best[i] < best[u])) u = i;
                in[u] = 1;
                if (from[u] >= 0) {   // the route from[u] -> u, its ways
                    const std::vector<int>& prev = prevFrom[from[u]];
                    for (int v = great[u]; prev[v] >= 0; v = prev[v]) trunk[{std::min(v, prev[v]), std::max(v, prev[v])}] = true;
                }
                for (int v = 0; v < m; v++) if (!in[v] && rc[u][v] < best[v]) { best[v] = rc[u][v]; from[v] = u; }
            }
        }
        for (const auto& kv : chosen) {
            const int a = kv.first.first, b = kv.first.second;
            PlanEdge e; e.a = a; e.b = b; e.lengthKm = dKm(a, b); e.trunk = trunk.count(kv.first) > 0;
            const PlanPlace& A = out.places[a]; const PlanPlace& B = out.places[b];
            int longest; const double ws = waterRun(a, b, longest);
            e.kind = rule.way;
            if (age == AGE_INDUSTRIAL) {
                if (e.trunk) e.kind = NK_RAIL;
                else if (A.river && B.river && unitFromHash(mix64(A.seed ^ B.seed ^ 0x7011CAULL)) < 0.4) {
                    double rough = 0; for (int k = 1; k < 8; k++) rough = std::max(rough, (double)G.relief[G.cellOf(slerpUnit(A.unit, B.unit, k / 8.0))]);
                    if (rough < 0.25) e.kind = NK_CANAL;
                }
            }
            if (age == AGE_MODERN && e.trunk) e.kind = NK_HIGHWAY;
            if (longest >= 3 || ws > 0.5) e.kind = NK_SEA;   // the way was over water: a crossing, not drawn
            e.id = mix64(A.seed ^ (B.seed * 0x9E3779B97F4A7C15ULL) ^ 0x7011ED6EULL);
            out.edges.push_back(e);
        }
    }
    out.buildMs = nowMs() - t0;
}

// ---- the world's plan --------------------------------------------------------------------------------------------------------
namespace {
uint64_t planCellKey(int gLat, int gLon) { return ((uint64_t)(uint32_t)(gLat + 0x100000) << 32) | (uint32_t)gLon; }
// the grid's cells within `radiusKm` of a point (each once)
template <class F> void forCellsNear(const PlanGrid& G, const Vec3& unit, double radiusKm, F fn) {
    double lat, lon; StarSystem::latLonFromBody(unit, lat, lon);
    const double dr = radiusKm / G.R;
    const int i0 = G.rowOf(lat - dr - 0.5 * G.dLat), i1 = G.rowOf(lat + dr + 0.5 * G.dLat);
    for (int i = i0; i <= i1; i++) {
        const int n = G.rowCols[i];
        const double latA = -PI / 2 + i * G.dLat, latB = latA + G.dLat;
        const double cosL = std::max(std::min(std::cos(latA), std::cos(latB)), 1e-3);
        const double lonR = dr / cosL + TAU / n;
        int j0 = (int)std::floor((lon - lonR + PI) / TAU * n), j1 = (int)std::floor((lon + lonR + PI) / TAU * n);
        if (lonR >= PI || j1 - j0 + 1 >= n) { j0 = 0; j1 = n - 1; }
        for (int j = j0; j <= j1; j++) fn(G.rowStart[i] + ((j % n) + n) % n);
    }
}
double gridBilinear(const PlanGrid& G, const std::vector<uint8_t>& v, double lat, double lon) {
    const double fy = (lat + PI / 2) / G.dLat - 0.5;
    const int i0 = (int)std::floor(fy); const double t = fy - i0;
    auto rowVal = [&](int i) {
        i = clampi(i, 0, G.rows - 1);
        const int n = G.rowCols[i];
        const double fx = (wrapAngle(lon) + PI) / TAU * n - 0.5;
        const int j0 = (int)std::floor(fx); const double s = fx - j0;
        const double a = v[G.rowStart[i] + ((j0 % n) + n) % n], b = v[G.rowStart[i] + (((j0 + 1) % n) + n) % n];
        return a + (b - a) * s;
    };
    return rowVal(i0) + (rowVal(i0 + 1) - rowVal(i0)) * t;
}
void indexWorldPlan(WorldPlan& P) {
    const PlanGrid& G = P.grid;
    P.placesIn.assign(G.cells, {});
    P.edgesIn.assign(G.cells, {});
    P.countryside.assign(G.cells, 0);
    for (int k = 0; k < P.peoples; k++) {
        const PeoplePlan& pp = P.plans[k];
        for (const PlanPlace& p : pp.places) {
            const uint32_t ref = ((uint32_t)k << 24) | (uint32_t)p.rank;
            P.placeAt[planCellKey(p.gLat, p.gLon)] = ref;
            P.placesIn[G.cellOf(p.unit)].push_back(ref);
        }
        for (size_t ei = 0; ei < pp.edges.size(); ei++) {
            const PlanEdge& e = pp.edges[ei];
            if (e.kind == NK_SEA) continue;
            const Vec3 a = pp.places[e.a].unit, b = pp.places[e.b].unit;
            const int steps = std::max(2, (int)std::ceil(e.lengthKm / (G.sizeKm() / 3)));
            const uint32_t ref = ((uint32_t)k << 24) | (uint32_t)ei;
            for (int s = 0; s <= steps; s++) {
                const int c = G.cellOf(slerpUnit(a, b, (double)s / steps));
                P.edgesIn[c].push_back(ref);
                int nb[8]; const int m = G.neighbours(c, nb);
                for (int t = 0; t < m; t++) P.edgesIn[nb[t]].push_back(ref);
            }
        }
        // the countryside: the territory's ground, thinning toward the frontier; none for the futuristic and the far-future peoples
        // (the land between their places left wild)
        if (pp.age >= AGE_FUTURISTIC || pp.territory.empty()) continue;
        std::vector<float> d(G.cells, 0);
        for (int c = 0; c < G.cells; c++) if (pp.territory[c]) d[c] = (float)(0.35 + 0.65 * std::pow(std::max(0.0f, pp.score[c]), 0.6) * (1 - 0.3 * std::max(0.0f, pp.remote[c])));
        for (int c = 0; c < G.cells; c++) {
            const bool sideLand = G.land[c] && (P.peoples < 2 || G.side[c] == k);
            if (!pp.territory[c] && !sideLand && G.land[c]) continue;   // the other people's land
            int nb[8]; const int m = G.neighbours(c, nb);
            double sum = pp.territory[c] ? 2 * d[c] : 0; int cnt = pp.territory[c] ? 2 : 0;
            for (int t = 0; t < m; t++) if (pp.territory[nb[t]] || pp.territory[c]) { sum += d[nb[t]]; cnt++; }
            if (!cnt) continue;
            // the territory's own; the sea's next to it its shores' (a coast is not thinned by the water beside it); the land beyond
            // the frontier half, a soft edge half a cell out
            const double v = pp.territory[c] || !G.land[c] ? sum / cnt : 0.5 * sum / cnt;
            P.countryside[c] = (uint8_t)std::max<int>(P.countryside[c], clampi((int)std::lround(v * 255), 0, 255));
        }
    }
    for (auto& v : P.edgesIn) { std::sort(v.begin(), v.end()); v.erase(std::unique(v.begin(), v.end()), v.end()); }
    for (int k = 0; k < P.peoples; k++) for (const PlanEdge& e : P.plans[k].edges) if (e.kind == NK_MAGLEV) P.works = true;
}
const PlanPlace& placeOf(const WorldPlan& P, uint32_t ref) { return P.plans[ref >> 24].places[ref & 0xFFFFFF]; }
}

std::shared_ptr<WorldPlan> buildWorldPlan(const BodyGen& g, bool parallel) {
    const double t0 = nowMs();
    auto P = std::make_shared<WorldPlan>();
    P->seed = g.seed; P->peoples = peoplesOf(g);
    if (P->peoples == 0) return P;
    P->grid = readPlanGrid(g, parallel);
    P->gridMs = nowMs() - t0;
    for (int k = 0; k < P->peoples; k++) planPeople(g, P->grid, k, peopleAgeOf(g, k).age, P->plans[k]);
    indexWorldPlan(*P);
    P->buildMs = nowMs() - t0;
    return P;
}

// ---- the cache ------------------------------------------------------------------------------------------------------------------
namespace {
struct PlanSlot { std::shared_future<std::shared_ptr<const WorldPlan>> fut; uint64_t used = 0; };
std::mutex g_planMutex;
std::map<uint64_t, PlanSlot> g_planSlots;
uint64_t g_planClock = 0;
struct PlanThread { std::thread t; std::shared_ptr<std::atomic<bool>> done; };
std::list<PlanThread> g_planThreads;
struct PlanThreadsJoin { ~PlanThreadsJoin() { for (PlanThread& p : g_planThreads) if (p.t.joinable()) p.t.join(); } } g_planJoin;
bool ready(const std::shared_future<std::shared_ptr<const WorldPlan>>& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
void evictLocked(uint64_t keep) {   // the least recently used of five, never one on its way
    while (g_planSlots.size() >= 4) {
        auto victim = g_planSlots.end();
        for (auto it = g_planSlots.begin(); it != g_planSlots.end(); ++it)
            if (it->first != keep && ready(it->second.fut) && (victim == g_planSlots.end() || it->second.used < victim->second.used)) victim = it;
        if (victim == g_planSlots.end()) break;
        g_planSlots.erase(victim);
    }
    for (auto it = g_planThreads.begin(); it != g_planThreads.end();) {   // the finished threads joined
        if (it->done->load()) { if (it->t.joinable()) it->t.join(); it = g_planThreads.erase(it); } else ++it;
    }
}
}

std::shared_ptr<const WorldPlan> worldPlanOf(const BodyGen& g) {
    std::shared_future<std::shared_ptr<const WorldPlan>> f;
    std::promise<std::shared_ptr<const WorldPlan>> pr;
    bool mine = false;
    {
        std::lock_guard<std::mutex> lock(g_planMutex);
        auto it = g_planSlots.find(g.seed);
        if (it != g_planSlots.end()) { it->second.used = ++g_planClock; f = it->second.fut; }
        else { evictLocked(g.seed); f = pr.get_future().share(); g_planSlots[g.seed] = PlanSlot{f, ++g_planClock}; mine = true; }
    }
    if (mine) pr.set_value(buildWorldPlan(g, false));
    return f.get();
}

std::shared_ptr<const WorldPlan> worldPlanIfReady(const BodyGen& g) {
    std::lock_guard<std::mutex> lock(g_planMutex);
    auto it = g_planSlots.find(g.seed);
    if (it == g_planSlots.end() || !ready(it->second.fut)) return nullptr;
    it->second.used = ++g_planClock;
    return it->second.fut.get();
}

void worldPlanPrefetch(const BodyGen& g) {
    if (!worldHadCivilisation(g)) return;
    std::lock_guard<std::mutex> lock(g_planMutex);
    if (g_planSlots.count(g.seed)) { g_planSlots[g.seed].used = ++g_planClock; return; }
    evictLocked(g.seed);
    auto pr = std::make_shared<std::promise<std::shared_ptr<const WorldPlan>>>();
    g_planSlots[g.seed] = PlanSlot{pr->get_future().share(), ++g_planClock};
    PlanThread th; th.done = std::make_shared<std::atomic<bool>>(false);
    auto done = th.done;
    const BodyGen gc = g;
    th.t = std::thread([pr, done, gc]() {
        pr->set_value(buildWorldPlan(gc, false));
        for (int p = 0; p < std::max(1, peoplesOf(gc)); p++) historyOfWorld(gc, p);   // T-02: the peoples' histories too (the towns read them)
        worldWorksOf(gc);   // T-03: and the works (their battlefields, dams and shelters read the histories)
        done->store(true);
    });
    g_planThreads.push_back(std::move(th));
}

bool worldPlanBuilt(uint64_t seed) {
    std::lock_guard<std::mutex> lock(g_planMutex);
    auto it = g_planSlots.find(seed);
    return it != g_planSlots.end() && ready(it->second.fut);
}

void worldPlanClearCache() {
    std::lock_guard<std::mutex> lock(g_planMutex);
    for (auto it = g_planSlots.begin(); it != g_planSlots.end();) { if (ready(it->second.fut)) it = g_planSlots.erase(it); else ++it; }
}

// ---- the cells' reading ---------------------------------------------------------------------------------------------------------
const PlanPlace* planPlaceIn(const WorldPlan& P, int gLat, int gLonWrapped) {
    auto it = P.placeAt.find(planCellKey(gLat, gLonWrapped));
    return it == P.placeAt.end() ? nullptr : &placeOf(P, it->second);
}

double planCountryside(const WorldPlan& P, double lat, double lon, int people) {
    if (P.peoples == 0 || P.grid.cells == 0) return 0;
    const PeoplePlan& pp = P.plans[clampi(people, 0, P.peoples - 1)];
    if (pp.age == AGE_FAR_FUTURE) return 0;
    const Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
    double dMin = 1e9;   // the nearest of the people's places (km)
    forCellsNear(P.grid, u, 60.0, [&](int c) { for (uint32_t ref : P.placesIn[c]) { const PlanPlace& p = placeOf(P, ref); if (p.people == pp.people) dMin = std::min(dMin, arcKm(u, p.unit, P.grid.R)); } });
    if (pp.age == AGE_FUTURISTIC) return 0.7 * std::exp(-dMin / 12.0);   // a futuristic people's pods round its great places alone
    const double base = gridBilinear(P.grid, P.countryside, lat, lon) / 255.0;
    if (base <= 0) return 0;
    return std::min(1.0, base * (0.7 + 0.3 * std::exp(-dMin / 30.0)));
}

const PlanPlace* planCoreAt(const WorldPlan& P, const Vec3& unit, double marginM) {
    if (P.grid.cells == 0) return nullptr;
    const PlanPlace* hit = nullptr;
    forCellsNear(P.grid, unit, 2.0 + marginM / 1000.0, [&](int c) {
        for (uint32_t ref : P.placesIn[c]) { const PlanPlace& p = placeOf(P, ref); if (!hit && arcKm(unit, p.unit, P.grid.R) * 1000.0 < p.radiusM + marginM) hit = &p; }
    });
    return hit;
}

void planPlacesNear(const WorldPlan& P, const Vec3& unit, double radiusM, std::vector<const PlanPlace*>& out) {
    out.clear();
    if (P.grid.cells == 0) return;
    forCellsNear(P.grid, unit, radiusM / 1000.0 + 2.0, [&](int c) {
        for (uint32_t ref : P.placesIn[c]) { const PlanPlace& p = placeOf(P, ref); if (arcKm(unit, p.unit, P.grid.R) * 1000.0 - p.radiusM < radiusM) out.push_back(&p); }
    });
    std::sort(out.begin(), out.end(), [](const PlanPlace* a, const PlanPlace* b) { return a->people != b->people ? a->people < b->people : a->rank < b->rank; });
}

void planEdgesNear(const WorldPlan& P, const Vec3& unit, double radiusM, std::vector<PlanEdgeRef>& out) {
    out.clear();
    if (P.grid.cells == 0) return;
    std::vector<uint32_t> refs;
    forCellsNear(P.grid, unit, radiusM / 1000.0, [&](int c) { refs.insert(refs.end(), P.edgesIn[c].begin(), P.edgesIn[c].end()); });
    std::sort(refs.begin(), refs.end()); refs.erase(std::unique(refs.begin(), refs.end()), refs.end());
    for (uint32_t ref : refs) { PlanEdgeRef r; r.people = (int)(ref >> 24); r.index = (int)(ref & 0xFFFFFF); r.e = &P.plans[r.people].edges[r.index]; out.push_back(r); }
}

const std::vector<Vec3>& planEdgeWay(const BodyGen& g, const WorldPlan& P, const PlanEdgeRef& r) {
    const PlanEdge& e = *r.e;
    {
        std::lock_guard<std::mutex> lock(P.wayMutex);
        auto it = P.ways.find(e.id);
        if (it != P.ways.end()) return it->second;
    }
    const PeoplePlan& pp = P.plans[r.people];
    const PlanPlace& A = pp.places[e.a]; const PlanPlace& B = pp.places[e.b];
    const double Rm = g.R * 1000.0;
    const Vec3 start = toward(A.unit, B.unit, A.radiusM * 1.06 + 25, Rm), end = toward(B.unit, A.unit, B.radiusM * 1.06 + 25, Rm);
    const double L = arcKm(start, end, g.R);
    const int N = std::max(1, (int)std::ceil(L / PLAN_LEG_KM));
    std::vector<Vec3> way; way.reserve(N + 1);
    way.push_back(start);
    Vec3 nrm = cross(start, end);
    nrm = length(nrm) > 1e-12 ? normalize(nrm) : Vec3(0, 0, 1);
    {
        DrainageOff off;
        static const double OFF[7] = {0, 600, -600, 1200, -1200, 2400, -2400};
        for (int k = 1; k < N; k++) {
            const Vec3 p = slerpUnit(start, end, (double)k / N);
            if (e.kind == NK_MAGLEV || e.kind == NK_SEA) { way.push_back(p); continue; }   // a maglev runs straight on its pylons
            Vec3 pick = p;
            for (int o = 0; o < 7; o++) {   // on dry ground: the line's point, else the nearest dry one across it
                const Vec3 q = normalize(p + nrm * (OFF[o] / Rm));
                if (!wetSample(sampleSurface(g, q, 64.0))) { pick = q; break; }
            }
            way.push_back(pick);
        }
    }
    way.push_back(end);
    std::lock_guard<std::mutex> lock(P.wayMutex);
    return P.ways.emplace(e.id, std::move(way)).first->second;
}

void planSpansIn(const BodyGen& g, const WorldPlan& P, int gLat, int gLon, std::vector<PlanSpan>& out) {   // T-01
    out.clear();
    if (!P.works) return;
    const double dLat = ruinCellLat(g), latc = (gLat + 0.5) * dLat, dLon = dLat / std::max(std::cos(latc), 0.05);
    const double lat0 = gLat * dLat, lat1 = lat0 + dLat, lon0 = gLon * dLon, lon1 = lon0 + dLon;
    const Vec3 centre = StarSystem::bodyFromLatLon(latc, (gLon + 0.5) * dLon);
    std::vector<PlanEdgeRef> es; planEdgesNear(P, centre, 4000, es);
    const double Rm = g.R * 1000.0, cellM = dLat * Rm;
    for (const PlanEdgeRef& r : es) {
        if (r.e->kind != NK_MAGLEV) continue;
        const std::vector<Vec3>& way = planEdgeWay(g, P, r);
        const double h = 18 + 8 * unitFromHash(mix64(r.e->id ^ 0x7011A6ULL));
        for (size_t k = 0; k + 1 < way.size(); k++) {
            const Vec3 &a = way[k], &b = way[k + 1];
            const double L = std::acos(clampd(dot(a, b), -1, 1)) * Rm;
            // the leg near the cell at all (its nearest end within the leg's length and the cell's size of the centre)
            if (std::min(std::acos(clampd(dot(a, centre), -1, 1)), std::acos(clampd(dot(b, centre), -1, 1))) * Rm > L + cellM) continue;
            const int n = std::max(1, (int)std::ceil(L / 150.0));
            for (int q = 0; q < n; q++) {
                const Vec3 p0 = slerpUnit(a, b, (double)q / n), p1 = slerpUnit(a, b, (double)(q + 1) / n), m = slerpUnit(a, b, (q + 0.5) / n);
                double la, lo; StarSystem::latLonFromBody(m, la, lo);
                lo = wrap2pi(lo);
                if (la < lat0 || la >= lat1 || lo < lon0 || lo >= lon1) continue;
                PlanSpan sp; sp.a = p0; sp.b = p1; sp.h = h; sp.people = r.people;
                sp.seed = mix64(r.e->id ^ ((uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL) ^ ((uint64_t)(q + 1) * 0xD1B54A32D192ED03ULL));
                out.push_back(sp);
            }
        }
    }
}
