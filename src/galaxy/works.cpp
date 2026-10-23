// T-03 (2026-10-09): the works (see works.h)
#include "works.h"
#include "plan.h"
#include "history.h"
#include "peoples.h"
#include "buildings.h"
#include "drainage.h"
#include "towns.h"
#include "core/rng.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <numeric>
#include <queue>
#include <unordered_map>

const char* const WORK_KIND_NAMES[WO_COUNT] = {"a quarry", "a mine", "a dam", "an aqueduct", "an observatory", "a shrine", "a pilgrims' way", "a necropolis",
    "lines on a plain", "a henge", "a fortified line", "a battlefield", "a launch site", "an antenna field", "a wreck", "a colossus", "a terraformed basin",
    "the last shelter", "something of the far future", "a wind farm", "a lighthouse", "a fort", "a dome over warm ground", "glasshouses"};
const char* const WORK_HEARD[WO_COUNT] = {"A QUARRY", "A MINE", "A DAM", "AN AQUEDUCT", "AN OBSERVATORY", "A SHRINE", "A WAY", "A NECROPOLIS", "LINES ON THE GROUND",
    "STANDING STONES", "A FORTIFIED LINE", "A BATTLEFIELD", "A LAUNCH SITE", "AN ANTENNA FIELD", "A WRECK", "A COLOSSUS", "SHAPED LAND", "A SHELTER",
    "SOMETHING MADE", "A WIND FARM", "A LIGHTHOUSE", "A FORT", "A DOME", "GLASSHOUSES"};

double editLeft(double years, double wet, bool line) {
    const double y = std::max(1.0, years);
    if (line) return clampd(1 - (std::log10(std::max(100.0, y)) - 2) / (wet > 0.4 ? 2.4 : 4.5), 0.15, 1);   // lines on a dry plain keep for millennia
    return clampd(1 - (std::log10(std::max(30.0, y)) - 1.5) / (wet > 0.4 ? 2.6 : 3.6), 0.12, 1);
}

namespace {
double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline bool wetS(const SurfaceSample& s) { return s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5; }
inline double arcM(const Vec3& a, const Vec3& b, double Rm) { return std::acos(clampd(dot(a, b), -1, 1)) * Rm; }
// a point `eastM` and `northM` off a unit vector on the tangent plane's great circles (the plan's and the roads' frame)
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
Vec3 toward(const Vec3& from, const Vec3& to, double m, double Rm) {
    const double w = std::acos(clampd(dot(from, to), -1, 1));
    if (w < 1e-12) return from;
    return slerpUnit(from, to, m / Rm / w);
}
// the bearing (from north, clockwise) of `to` seen from `from`
double bearingOf(const Vec3& from, const Vec3& to) {
    Vec3 pole(0, 0, 1), east = cross(pole, from);
    if (length(east) < 1e-9) east = Vec3(0, 1, 0);
    east = normalize(east);
    const Vec3 north = cross(from, east), d = to - from;
    return std::atan2(dot(d, east), dot(d, north));
}
// a work's own metres: east and north of its middle as the settlements lay theirs out (the latitude and the longitude scaled at the middle)
struct Frame {
    double lat0, lon0, Rm, cl;
    Frame(const Vec3& u, double R) : Rm(R) { StarSystem::latLonFromBody(u, lat0, lon0); cl = std::max(std::cos(lat0), 0.05); }
    Vec3 unit(double e, double n) const { return StarSystem::bodyFromLatLon(lat0 + n / Rm, lon0 + e / (Rm * cl)); }
    void local(const Vec3& u, double& e, double& n) const { double la, lo; StarSystem::latLonFromBody(u, la, lo); e = std::remainder(lo - lon0, TAU) * Rm * cl; n = (la - lat0) * Rm; }
};
// a building's frame: local (across, along) of a heading to the work's east and north
inline void rot(double heading, double a, double b, double& x, double& z) { x = std::cos(heading) * a + std::sin(heading) * b; z = -std::sin(heading) * a + std::cos(heading) * b; }
void addPt(GroundEdit& e, double x, double z) { e.pts.push_back((float)x); e.pts.push_back((float)z); }

// ---- the builder -------------------------------------------------------------------------------------------------------------
}   // namespace

bool damBreached(const Work& w, const Culture& c) {
    if (w.kind != WO_DAM) return false;
    Building b; b.kind = BK_DAM; b.age = w.age; b.before = w.before; b.style = w.style;
    b.seed = mix64(w.seed ^ 0xB01D0000ULL);   // its wall is the first piece `workSpec` lays
    b.luck = (unitFromHash(mix64(b.seed ^ 0x1CCULL)) - 0.5) * 0.5;
    const Culture wc = w.age == c.age && w.before <= 0 ? c : cultureAtAge(c, w.age, w.before);
    return buildingDecay(b, wc, w.style == 0 ? MT_MASONRY : (w.style == 3 ? MT_COMPOSITE : MT_MASS)) > 0.5;
}

namespace {
// B-418: one cell of the land's grid (its bounds, its middle, its hash) and the settlement a work in it would serve (the greatest in it and
// round it, the nearest of those)
struct LandCell { int gLat = 0, gl = 0; double lat0 = 0, lat1 = 0, latMid = 0, lonMid = 0, halfLon = 0; Vec3 mid; uint64_t h = 0; };
struct LandSettle { bool found = false; Vec3 u; double r = 0; int rank = -1, sclass = -1; bool river = false; };

struct WB {
    const BodyGen& g; const WorldPlan& P; std::vector<Work>& out; double Rm;
    const std::vector<Work>* world = nullptr;   // B-418: the world's works a land's work keeps clear of (null: building the world's)
    struct CellS { bool has = false; Vec3 u; double r = 0; int sclass = 0, people = 0; };
    std::unordered_map<uint64_t, CellS> cells;
    // the people being built
    int p = 0, age = 0; const PeoplePlan* pp = nullptr; std::shared_ptr<const History> H; Culture C; uint64_t pseed = 0;
    int ageFrom = 1;   // B-414: the first year of the people's last age: what was made, fought or left before it is gone (`ageFromOf`)
    int made[WO_COUNT] = {};

    WB(const BodyGen& gg, const WorldPlan& PP, std::vector<Work>& o) : g(gg), P(PP), out(o), Rm(gg.R * 1000.0) {}
    SurfaceSample at(const Vec3& u) const { return sampleSurface(g, u, 64.0); }
    // the grade at a point (rise over run), the heading it falls toward and its height
    double grade(const Vec3& u, double d, double& downhill, double& h0) const {
        h0 = at(u).height;
        const double he = at(offsetUnit(u, d, 0, Rm)).height, hw = at(offsetUnit(u, -d, 0, Rm)).height;
        const double hn = at(offsetUnit(u, 0, d, Rm)).height, hs = at(offsetUnit(u, 0, -d, Rm)).height;
        const double gx = (he - hw) / (2 * d), gz = (hn - hs) / (2 * d);
        downhill = std::atan2(-gx, -gz);
        return std::sqrt(gx * gx + gz * gz);
    }
    // a ruin cell's settlement (the finders' reading: its middle and its radius), kept while the world's works are built
    const CellS& cellAt(int row, int gl) {
        const uint64_t key = ((uint64_t)(uint32_t)(row + 0x100000) << 32) | (uint32_t)gl;
        auto it = cells.find(key);
        if (it != cells.end()) return it->second;
        CellS c; RuinSpec sp;
        if (ruinOfCell(g, row, gl, sp, false) && sp.sclass != SC_WORKS) { c.has = true; c.u = StarSystem::bodyFromLatLon(sp.lat, sp.lon); c.r = sp.size; c.sclass = sp.sclass; c.people = sp.people; }
        return cells[key] = c;
    }
    // nothing else within `r`: no settlement of the cells (its radius and a margin), no city's core, no work already made (its reach; B-418: a
    // land's work the world's too)
    bool clearOf(const Vec3& u, double r, bool ofWorks = true) {
        if (ofWorks) {
            for (const Work& w : out) if (arcM(u, w.unit, Rm) < r + w.radiusM + 40) return false;
            if (world) for (const Work& w : *world) if (arcM(u, w.unit, Rm) < r + w.radiusM + 40) return false;
        }
        if (planCoreAt(P, u, r + 150)) return false;
        double lat, lon; StarSystem::latLonFromBody(u, lat, lon);
        const double dLat = ruinCellLat(g);
        const int reach = 1 + (int)((r + 400) / 2000.0), gLat = (int)std::floor(lat / dLat);
        for (int dl = -reach; dl <= reach; dl++) {
            const int row = gLat + dl; const double latc = (row + 0.5) * dLat;
            if (std::fabs(latc) > PI / 2 - dLat) continue;
            const int nLon = std::max(1, ruinCellsAround(g, row)); const double dLon = dLat / std::max(std::cos(latc), 0.05);
            const int g2 = (int)std::floor(lon / dLon);
            for (int dd = -reach; dd <= reach; dd++) {
                const CellS& c = cellAt(row, (((g2 + dd) % nLon) + nLon) % nLon);
                if (c.has && arcM(u, c.u, Rm) < r + c.r + 60) return false;
            }
        }
        return true;
    }
    // a history place's age (B-414: its people's last, whatever age founded it: a work by it stands in that age's way) and the years before the
    // end it was left (a place left early)
    void placeAge(int hp, int& a, double& before) const {
        a = age; before = 0;
        if (!H || hp < 0 || hp >= (int)H->places.size()) return;
        const HistoryPlace& q = H->places[hp];
        if (q.fate == HP_EARLY || q.fate == HP_CLEARED) before = std::max(0, H->last - q.ended);
    }
    // the place is gone, and its works with it: cleared by its people (a futuristic or far-future people's older city), or B-414 left before its
    // people's last age began
    bool gone(int hp) const {
        if (!H || hp < 0 || hp >= (int)H->places.size()) return false;
        const HistoryPlace& q = H->places[hp];
        return q.fate == HP_CLEARED || (q.fate == HP_EARLY && q.ended < ageFrom);
    }
    // a history place's middle and its core's radius (the plan's, or a cleared city's)
    void placeAt(int hp, Vec3& u, double& r) const {
        const HistoryPlace& q = H->places[hp];
        if (q.plan >= 0 && q.plan < (int)pp->places.size()) { u = pp->places[q.plan].unit; r = pp->places[q.plan].radiusM; return; }
        u = StarSystem::bodyFromLatLon(q.lat, q.lon); r = q.sclass == SC_CITY ? 600 : (q.sclass == SC_CAPITAL ? 900 : 250);
    }
    uint64_t seedOf(int kind, int k) const { return mix64(pseed ^ (0x7030000000ULL + ((uint64_t)kind << 20) + (uint64_t)k)); }
    Work& add(int kind, const Vec3& u, double reach, uint64_t seed) {
        Work w; w.kind = kind; w.people = p; w.age = age; w.unit = u; StarSystem::latLonFromBody(u, w.lat, w.lon);
        w.radiusM = reach; w.seed = seed; w.id = mix64(g.seed ^ seed ^ 0x7031D00DULL);
        w.ground = at(u).height;
        out.push_back(w);
        made[kind]++;
        return out.back();
    }
    // the events of a kind: HE_BUILT of a work (`thing`), HE_BATTLE, ...
    template <class F> void forEvents(int kind, F fn) const { if (!H) return; for (int i = 0; i < (int)H->events.size(); i++) if (H->events[i].kind == kind) fn(i, H->events[i]); }

    void mines(); void dams(); void aqueducts(); void observatories(); void shrine(); void geoglyphs(); void fortLine(); void battlefields();
    void launchSites(); void antennas(); void colossi(); void terraforms(); void shelter(); void strange(); void lighthouses();
    // D-05: a people that lost its sun: a shelter's site (the last's and the others'), the warmest open ground near a place, the works of the end
    bool shelterSite(const Vec3& u0, double r0, Rng& rng, Vec3& best, double& bestDown);
    int shelterStyle() const;
    bool warmGround(const Vec3& u0, double r0, double reach, Rng& rng, Vec3& best);
    void sunlostWorks();
    void mineAt(Work& w, int style, double h, double down, double pitR, Rng& rng);
    // B-418: a cell of the land (`landWork`: what its ground offers, the kind drawn, sited by its rule inside the cell)
    bool landWork(const LandCell& L);
    LandSettle landSettle(const LandCell& L);
    bool landSummit(const LandCell& L, double reach, Rng& rng, Vec3& top, double& hTop);
    void landClimb(const LandCell& L, double reach, Vec3& top, double& hTop);
    double fallOver(const Vec3& u, double r);
    bool landQuarry(const LandCell& L, bool anyGround, Rng& rng); bool landMine(const LandCell& L, Rng& rng); bool landNecropolis(const LandCell& L, const LandSettle& S, Rng& rng);
    bool landHenge(const LandCell& L, Rng& rng); bool landWreck(const LandCell& L, bool coast, Rng& rng); bool landWindfarm(const LandCell& L, Rng& rng);
    bool landShrine(const LandCell& L, const LandSettle& S, Rng& rng); bool landLighthouse(const LandCell& L, Rng& rng); bool landFort(const LandCell& L, const LandSettle& S, Rng& rng);
    bool damNear(const Vec3& u0, double R0, int a, uint64_t seed, Work& out);
    bool summitNear(const Vec3& u0, double r0, double r1, Rng& rng, Vec3& best, double& hBest);
    bool shoreNear(const Vec3& u0, double r0, double r1, Rng& rng, Vec3& shore, double& seaward, bool headland);
};

// the highest point between r0 and r1 of a point, refined to its summit
bool WB::summitNear(const Vec3& u0, double r0, double r1, Rng& rng, Vec3& best, double& hBest) {
    hBest = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    const int rings = 6;
    for (int ri = 0; ri < rings; ri++) {
        const double d = r0 + (r1 - r0) * (ri + 0.5) / rings;
        const int n = 12 + 4 * ri;
        for (int k = 0; k < n; k++) {
            const double a = a0 + k * TAU / n;
            const Vec3 u = offsetUnit(u0, std::sin(a) * d, std::cos(a) * d, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || s.height <= hBest) continue;
            hBest = s.height; best = u; any = true;
        }
    }
    if (!any) return false;
    for (double step : {300.0, 100.0, 35.0}) {   // up the slope to its top
        for (int it = 0; it < 6; it++) {
            Vec3 nb = best; double hb = hBest;
            for (int k = 0; k < 8; k++) {
                const double a = k * TAU / 8;
                const Vec3 u = offsetUnit(best, std::sin(a) * step, std::cos(a) * step, Rm);
                const SurfaceSample s = at(u);
                if (!wetS(s) && s.height > hb) { hb = s.height; nb = u; }
            }
            if (hb <= hBest) break;
            hBest = hb; best = nb;
        }
    }
    return true;
}

// the shoreline nearest a point within r0..r1 (a dry probe beside a wet one, bisected): its point and the heading toward the water; with
// `headland`, the shore point with the most water round it (a headland's)
bool WB::shoreNear(const Vec3& u0, double r0, double r1, Rng& rng, Vec3& shore, double& seaward, bool headland) {
    double bestScore = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (int ri = 0; ri < 6; ri++) {
        const double d = r0 + (r1 - r0) * ri / 5.0;
        for (int k = 0; k < 24; k++) {
            const double a = a0 + k * TAU / 24;
            const Vec3 dry = offsetUnit(u0, std::sin(a) * d, std::cos(a) * d, Rm);
            if (wetS(at(dry))) continue;
            for (int q = 0; q < 8; q++) {   // water within 300 m of it
                const double b = q * TAU / 8;
                const Vec3 wv = offsetUnit(dry, std::sin(b) * 300, std::cos(b) * 300, Rm);
                const SurfaceSample ws = at(wv);
                if (!wetS(ws) || ws.oldSea > 0.5) continue;
                double lo = 0, hi = 300;   // bisect the edge
                for (int it = 0; it < 7; it++) { const double mid = 0.5 * (lo + hi); if (wetS(at(offsetUnit(dry, std::sin(b) * mid, std::cos(b) * mid, Rm)))) hi = mid; else lo = mid; }
                const Vec3 pt = offsetUnit(dry, std::sin(b) * lo, std::cos(b) * lo, Rm);
                double score = -d / r1;
                if (headland) { int wn = 0; for (int m = 0; m < 12; m++) { const double c = m * TAU / 12; if (wetS(at(offsetUnit(pt, std::sin(c) * 400, std::cos(c) * 400, Rm)))) wn++; } score += wn * 0.3; }
                if (score > bestScore) { bestScore = score; shore = pt; seaward = bearingOf(pt, wv); any = true; }
                break;
            }
            if (any && !headland && ri > 0) return true;
        }
    }
    return any;
}

// ---- the mines the history sank (B-418: the land's where the plan read coal and ore, `landMine`) ---------------------------------------
void WB::mineAt(Work& w, int style, double h, double down, double pitR, Rng& rng) {   // a mine's style and its open pit
    w.style = style; w.heading = down + PI;
    if (style == 2) {
        const int nb = 5 + (int)(rng.uni() * 5);
        GroundEdit e; e.kind = GE_PIT; e.r = pitR; e.depth = 13 + 3 * rng.uni(); e.step = std::min(24.0, pitR * 0.55 / nb); e.level = h - nb * e.depth; e.hw = pitR - nb * e.step; e.fades = false;
        w.edits.push_back(e); w.floor = e.level; w.length = 2 * pitR;
    }
}
void WB::mines() {
    if (age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC) return;
    Rng rng(pseed ^ 0x70320ULL);
    struct Src { Vec3 u; double r; int a; double before; int hp, ev, year; };
    std::vector<Src> src;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_MINE || e.place < 0 || src.size() >= 6 || gone(e.place)) return;
        Src s; placeAt(e.place, s.u, s.r); s.a = age; int pa; placeAge(e.place, pa, s.before); s.hp = e.place; s.ev = i; s.year = e.year;
        src.push_back(s);
    });
    int k = 0;
    for (const Src& s : src) {
        const int a = clampi(s.a, AGE_MEDIEVAL, AGE_MODERN);
        const int style = a == AGE_MEDIEVAL ? 0 : (a == AGE_INDUSTRIAL ? 1 : 2);
        Vec3 best; double bestS = -1e18, bestDown = 0, bestH = 0; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {1500.0, 3000.0, 5000.0, 8000.0})
            for (int q = 0; q < 16; q++) {
                const double ang = a0 + q * TAU / 16, dd = s.r + d;
                const Vec3 u = offsetUnit(s.u, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                const SurfaceSample sm = at(u);
                if (wetS(sm) || matFamily(sm.material) == FAM_SNOW) continue;
                double down, h; const double sl = grade(u, 45, down, h);
                double sc = P.grid.res[P.grid.cellOf(u)] - dd / 20000;
                if (style == 0) { if (sl < 0.15) continue; sc += std::min(sl, 0.5); }
                else { if (sl > 0.14) continue; sc -= sl; }
                if (matFamily(sm.material) == FAM_ROCK) sc += 0.1;
                if (sc > bestS && clearOf(u, style == 2 ? 520 : 260)) { bestS = sc; best = u; bestDown = down; bestH = h; any = true; }
            }
        if (!any) { k++; continue; }
        const double pitR = 220 + 200 * rng.uni();
        Work& w = add(WO_MINE, best, style == 2 ? pitR + 120 : 240, seedOf(WO_MINE, k++));
        w.age = a; w.before = s.before; w.place = s.hp; w.event = s.ev; w.year = s.year;
        if (s.hp >= 0 && H->places[s.hp].plan >= 0) w.planRank = H->places[s.hp].plan;
        mineAt(w, style, bestH, bestDown, pitR, rng);
    }
}

// ---- the dams: at a valley's narrows near the place the history built one, holding the reservoir behind ----------------------------
// the valleys round the place (a grid of 300 m over 18 km, the flow down its steepest neighbours and gathered); the narrowest cross-section
// at the age's height among the larger valleys; then the reservoir: the water at a level filled from behind the wall over a grid of 50 m,
// as high as it stays behind the wall and inside the grid, the crest a few metres over it
bool WB::damNear(const Vec3& u0, double R0, int a, uint64_t seed, Work& out) {
    Rng rng(seed);
    const double Ht = a == AGE_INDUSTRIAL ? 18 + 16 * rng.uni() : (a == AGE_MODERN ? 45 + 60 * rng.uni() : 90 + 80 * rng.uni());
    const double Hmin = a == AGE_INDUSTRIAL ? 8 : (a == AGE_MODERN ? 18 : 35), maxHalf = a == AGE_INDUSTRIAL ? 700 : (a == AGE_MODERN ? 1100 : 1500);
    const int N = 61; const double cs = 300;
    const Frame F(u0, Rm);
    std::vector<float> h(N * N); std::vector<uint8_t> wt(N * N);
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) { const SurfaceSample s = at(F.unit((i - N / 2) * cs, (j - N / 2) * cs)); h[j * N + i] = (float)s.height; wt[j * N + i] = wetS(s); }
    std::vector<int> down(N * N, -1);
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        const int c = j * N + i; if (wt[c]) continue;
        double best = 0;
        for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
            if (!di && !dj) continue;
            const int ii = i + di, jj = j + dj; if (ii < 0 || jj < 0 || ii >= N || jj >= N) continue;
            const double drop = (h[c] - h[jj * N + ii]) / (cs * std::sqrt((double)(di * di + dj * dj)));
            if (drop > best) { best = drop; down[c] = jj * N + ii; }
        }
    }
    std::vector<int> order(N * N); std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int x, int y) { return h[x] > h[y]; });
    std::vector<float> acc(N * N, 1.0f);
    for (int c : order) if (down[c] >= 0) acc[down[c]] += acc[c];
    // the closed basins filled to their spill (a priority flood from the grid's edge and the water): the drainage the surface draws fills them
    // with a lake or a playa's flat, over a dam's floor (galaxy/drainage.cpp)
    std::vector<float> filled(N * N, 1e30f);
    {
        using QE = std::pair<float, int>;
        std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
        for (int c = 0; c < N * N; c++) { const int i = c % N, j = c / N; if (wt[c] || i == 0 || j == 0 || i == N - 1 || j == N - 1) { filled[c] = h[c]; pq.push({h[c], c}); } }
        while (!pq.empty()) {
            const QE q = pq.top(); pq.pop();
            if (q.first > filled[q.second]) continue;
            const int i = q.second % N, j = q.second / N;
            for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
                const int ii = i + di, jj = j + dj; if (ii < 0 || jj < 0 || ii >= N || jj >= N) continue;
                const int nb = jj * N + ii; const float f = std::max(h[nb], q.first);
                if (f < filled[nb]) { filled[nb] = f; pq.push({f, nb}); }
            }
        }
    }
    struct Cand { int c; double score, floor, tL, tR, ex, ez, fx, fz, crest; };
    std::vector<int> cand;   // every valley cell in reach
    for (int c = 0; c < N * N; c++) {
        if (wt[c] || down[c] < 0 || wt[down[c]] || acc[c] < 12 || acc[c] > 3000) continue;
        if (filled[c] > h[c] + 2 || filled[down[c]] > h[down[c]] + 2) continue;   // in a basin the drainage fills
        const double ex = (c % N - N / 2) * cs, ez = (c / N - N / 2) * cs, d = std::hypot(ex, ez);
        if (d < R0 + 1200 || d > 8500) continue;
        cand.push_back(c);
    }
    // each one's cross-section on the coarse grid first (bilinear, free), then the best few's with samples every 25 m
    auto hc = [&](double x, double z) {
        const double fi = clampd(x / cs + N / 2, 0, N - 1.001), fj = clampd(z / cs + N / 2, 0, N - 1.001);
        const int i0 = (int)fi, j0 = (int)fj; const double tx = fi - i0, tz = fj - j0;
        return (h[j0 * N + i0] * (1 - tx) + h[j0 * N + i0 + 1] * tx) * (1 - tz) + (h[(j0 + 1) * N + i0] * (1 - tx) + h[(j0 + 1) * N + i0 + 1] * tx) * tz;
    };
    auto section = [&](double ex, double ez, double axx, double axz, double step, const std::function<double(double, double)>& height, Cand& cd) -> bool {
        const int half = (int)(maxHalf / step), M = 2 * half + 1;
        std::vector<double> prof(M);
        for (int q = 0; q < M; q++) { const double t = (q - half) * step; prof[q] = height(ex + axx * t, ez + axz * t); }
        const int w0 = std::max(1, (int)std::lround(150 / step));
        int q0 = half; for (int q = half - w0; q <= half + w0; q++) if (prof[q] < prof[q0]) q0 = q;
        double hL = -1e18, hR = -1e18;   // the crest: the lower of the two shoulders within reach, no higher than the age built
        for (int q = 0; q <= q0; q++) hL = std::max(hL, prof[q]);
        for (int q = q0; q < M; q++) hR = std::max(hR, prof[q]);
        const double floor = prof[q0], crest = std::min(std::min(hL, hR) - 1, floor + Ht);
        if (crest - floor < Hmin) return false;
        int qa = q0, qb = q0;
        while (qa > 0 && prof[qa] < crest) qa--;
        while (qb < M - 1 && prof[qb] < crest) qb++;
        if (prof[qa] < crest || prof[qb] < crest) return false;
        cd.floor = floor; cd.crest = crest; cd.tL = (qa - q0) * step; cd.tR = (qb - q0) * step;
        cd.ex = ex + axx * (q0 - half) * step; cd.ez = ez + axz * (q0 - half) * step;
        cd.score = (crest - floor) / (cd.tR - cd.tL) + 0.004 * (crest - floor);
        return true;
    };
    std::vector<Cand> coarse;
    for (int c : cand) {
        const double ex = (c % N - N / 2) * cs, ez = (c / N - N / 2) * cs;
        const double dx = (down[c] % N - c % N) * cs, dz = (down[c] / N - c / N) * cs, dl = std::hypot(dx, dz);
        Cand cd; cd.c = c; cd.fx = dx / dl; cd.fz = dz / dl;
        if (!section(ex, ez, -cd.fz, cd.fx, 100, hc, cd)) continue;
        cd.score += 0.0002 * std::min(acc[c], 600.0f);
        coarse.push_back(cd);
    }
    std::sort(coarse.begin(), coarse.end(), [](const Cand& x, const Cand& y) { return x.score > y.score; });
    std::vector<Cand> good;
    for (int k = 0; k < (int)coarse.size() && k < 6; k++) {
        Cand cd = coarse[k];
        const double ex = (cd.c % N - N / 2) * cs, ez = (cd.c / N - N / 2) * cs;
        if (section(ex, ez, -cd.fz, cd.fx, 25, [&](double x, double z) { return at(F.unit(x, z)).height; }, cd)) good.push_back(cd);
    }
    std::sort(good.begin(), good.end(), [](const Cand& x, const Cand& y) { return x.score > y.score; });
    static const bool dbg = std::getenv("WORKS_DEBUG") != nullptr;
    if (dbg) { int big = 0; double hmin = 1e18, hmax = -1e18; for (int c = 0; c < N * N; c++) { if (acc[c] >= 25) big++; if (!wt[c]) { hmin = std::min(hmin, (double)h[c]); hmax = std::max(hmax, (double)h[c]); } } fprintf(stderr, "dam near: Ht %.0f, %d cells of acc >= 25, %zu candidates, %zu with walls; the grid's heights %.0f..%.0f\n", Ht, big, cand.size(), good.size(), hmin, hmax); }
    for (int gi = 0; gi < (int)good.size() && gi < 3; gi++) {
        const Cand& cd = good[gi];
        const Vec3 du = F.unit(cd.ex, cd.ez);   // the floor's point under the dam
        const Frame D(du, Rm);
        const double axx = -cd.fz, axz = cd.fx;
        // the reservoir's grid: 100 cells of 50 m, most of it upstream
        const int M = 100; const double fc = 50;
        const double gx0 = -cd.fx * 2000 - M * fc * 0.5, gz0 = -cd.fz * 2000 - M * fc * 0.5;
        std::vector<float> hf(M * M);
        for (int j = 0; j < M; j++) for (int i = 0; i < M; i++) hf[j * M + i] = (float)at(D.unit(gx0 + (i + 0.5) * fc, gz0 + (j + 0.5) * fc)).height;
        // the wall's line (beyond its abutments a little) as a barrier of cells
        std::vector<uint8_t> bar(M * M, 0);
        const double ax0 = axx * (cd.tL - 120), az0 = axz * (cd.tL - 120), ax1 = axx * (cd.tR + 120), az1 = axz * (cd.tR + 120);
        for (int j = 0; j < M; j++) for (int i = 0; i < M; i++) {
            const double x = gx0 + (i + 0.5) * fc, z = gz0 + (j + 0.5) * fc;
            const double vx = ax1 - ax0, vz = az1 - az0, L2 = vx * vx + vz * vz, t = clampd(((x - ax0) * vx + (z - az0) * vz) / L2, 0, 1);
            if (std::hypot(x - (ax0 + vx * t), z - (az0 + vz * t)) < fc * 0.9) bar[j * M + i] = 1;
        }
        const int si = (int)std::floor((-cd.fx * 90 - gx0) / fc), sj = (int)std::floor((-cd.fz * 90 - gz0) / fc);
        if (si < 1 || sj < 1 || si >= M - 1 || sj >= M - 1 || bar[sj * M + si]) continue;
        std::vector<uint8_t> fill(M * M);
        std::vector<int> stack;
        auto flood = [&](double L) -> int {   // the cells filled, -1 a leak (the grid's edge, or round the wall to its downstream side)
            std::fill(fill.begin(), fill.end(), 0);
            if (hf[sj * M + si] >= L) return 0;
            stack.clear(); stack.push_back(sj * M + si); fill[sj * M + si] = 1; int n = 0;
            while (!stack.empty()) {
                const int c = stack.back(); stack.pop_back(); n++;
                const int i = c % M, j = c / M;
                if (i == 0 || j == 0 || i == M - 1 || j == M - 1) return -1;
                const double x = gx0 + (i + 0.5) * fc, z = gz0 + (j + 0.5) * fc;
                if (x * cd.fx + z * cd.fz > 40) return -1;
                const int nb[4] = {c - 1, c + 1, c - M, c + M};
                for (int q = 0; q < 4; q++) { const int d = nb[q]; if (!fill[d] && !bar[d] && hf[d] < L) { fill[d] = 1; stack.push_back(d); } }
            }
            return n;
        };
        const double fb = a == AGE_INDUSTRIAL ? 2 : 4;
        double lo = cd.floor + 2, hi = cd.crest - fb, L = -1e9;
        if (flood(lo) < 0) { if (dbg) fprintf(stderr, "  cand %d: leaks at the floor + 2\n", gi); continue; }
        for (int it = 0; it < 14; it++) { const double mid = 0.5 * (lo + hi); if (flood(mid) >= 0) { L = mid; lo = mid; } else hi = mid; }
        if (L < -1e8) L = cd.floor + 2;
        const int cellsN = flood(L);
        if (dbg) fprintf(stderr, "  cand %d: floor %.0f, level %.1f (%.0f over), %d cells\n", gi, cd.floor, L, L - cd.floor, cellsN);
        if (cellsN < 30 || L - cd.floor + fb < Hmin) continue;
        const double crest = L + fb;
        // the abutments at the crest found: the cross-section again
        double tL = 0, tR = 0;
        for (int s = -1; s <= 1; s += 2) {
            double t = 0; bool hit = false;
            for (int q = 1; q <= (int)(maxHalf * 1.4 / 10); q++) { t = s * q * 10.0; if (at(D.unit(axx * t, axz * t)).height >= crest) { hit = true; break; } }
            if (!hit) { tL = tR = 0; break; }
            (s < 0 ? tL : tR) = t;
        }
        if (tR - tL < 20) continue;
        const double mx = axx * 0.5 * (tL + tR), mz = axz * 0.5 * (tL + tR);
        const Vec3 wu = D.unit(mx, mz);
        out.unit = wu; StarSystem::latLonFromBody(wu, out.lat, out.lon);
        out.floor = cd.floor; out.crest = crest; out.level = L; out.length = tR - tL; out.height = crest - cd.floor;
        out.heading = std::atan2(-cd.fx, -cd.fz);   // toward the reservoir
        out.axis = std::atan2(axx, axz);
        // the water's cells (filled at L, a cell more round them for the shore's cut), in the work's metres
        GroundEdit e; e.kind = GE_WATER; e.level = L; e.cell = fc; e.nx = M; e.nz = M; e.gx0 = gx0 - mx; e.gz0 = gz0 - mz; e.fades = false;
        e.mask.assign(M * M, 0);
        int area = 0;
        for (int j = 1; j < M - 1; j++) for (int i = 1; i < M - 1; i++) {
            const int c = j * M + i;
            if (fill[c]) { e.mask[c] = 1; area++; continue; }
            if (bar[c]) continue;
            const double x = gx0 + (i + 0.5) * fc, z = gz0 + (j + 0.5) * fc;
            if (x * cd.fx + z * cd.fz > -20) continue;
            if (fill[c - 1] || fill[c + 1] || fill[c - M] || fill[c + M]) e.mask[c] = 2;
        }
        out.width = area * fc * fc;   // the reservoir's area (m2)
        out.radiusM = std::max(out.length * 0.5 + 60, 0.0);
        double far = 0;
        for (int j = 0; j < M; j++) for (int i = 0; i < M; i++) if (fill[j * M + i]) far = std::max(far, std::hypot(gx0 + (i + 0.5) * fc - mx, gz0 + (j + 0.5) * fc - mz));
        out.radiusM = std::max(out.radiusM, far + 60);
        out.edits.push_back(e);
        return true;
    }
    return false;
}

void WB::dams() {
    if (!H || age < AGE_INDUSTRIAL || age == AGE_FAR_FUTURE) return;
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_DAM || e.place < 0 || made[WO_DAM] >= 5 || gone(e.place)) return;
        const int a = clampi(age, AGE_INDUSTRIAL, AGE_FUTURISTIC);
        Vec3 u; double r; placeAt(e.place, u, r);
        Work w; w.kind = WO_DAM;
        const uint64_t sd = seedOf(WO_DAM, k++);
        // by its town, else up its river into the hills (the plan's coarse rivers: the upstream cell that brings the most water, three times)
        bool found = damNear(u, r, a, sd, w);
        const PlanGrid& G = P.grid;
        int c = G.cellOf(u);
        const double h0 = G.height[c];
        for (int step = 0; step < 4 && !found; step++) {
            int nb[8]; const int nn = G.neighbours(c, nb); int up = -1; float bf = 0;
            for (int q = 0; q < nn; q++) if (G.down[nb[q]] == c && G.land[nb[q]] && G.river[nb[q]] > bf) { bf = G.river[nb[q]]; up = nb[q]; }
            if (up < 0) break;
            c = up;
            if (G.relief[c] < 0.12f && G.height[c] < h0 + 120) continue;   // still the plain
            found = damNear(G.unitOf(c), 0, a, sd ^ (uint64_t)(step + 1), w);
        }
        if (!found) return;
        if (!clearOf(w.unit, std::min(w.radiusM, w.length * 0.5 + 200))) return;
        Work& x = add(WO_DAM, w.unit, w.radiusM, sd);
        const Vec3 keep = x.unit; const uint64_t id = x.id; const double ground = x.ground;
        x = w; x.unit = keep; x.id = id; x.seed = sd; x.people = p; x.ground = ground;
        x.age = a; x.style = a == AGE_INDUSTRIAL ? 0 : (a == AGE_FUTURISTIC ? 3 : (x.length / std::max(1.0, x.height) < 4.5 ? 2 : 1));
        x.place = e.place; x.event = i; x.year = e.year; int pa; placeAge(e.place, pa, x.before);
        if (H->places[e.place].plan >= 0) x.planRank = H->places[e.place].plan;
        if (damBreached(x, C)) for (GroundEdit& ge : x.edits) if (ge.kind == GE_WATER) ge.tone = ET_DARK;   // breached: the reservoir drained, its bed left
    });
}

// ---- the aqueducts: from a spring on higher ground to the town, over the low ground on arches -----------------------------------------
void WB::aqueducts() {
    if (!H || age < AGE_MEDIEVAL || age > AGE_INDUSTRIAL) return;   // B-414: a medieval or an industrial people's (its arches in its age's way)
    Rng rng(pseed ^ 0x70340ULL);
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_AQUEDUCT || e.place < 0 || made[WO_AQUEDUCT] >= 3 || gone(e.place)) return;
        Vec3 u0; double r0; placeAt(e.place, u0, r0);
        const double h0 = at(u0).height;
        Vec3 src; double hs = -1e18, ds = 0; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {3000.0, 5000.0, 7500.0, 10000.0, 13000.0}) {
            for (int q = 0; q < 24; q++) {
                const double ang = a0 + q * TAU / 24;
                const Vec3 u = offsetUnit(u0, std::sin(ang) * d, std::cos(ang) * d, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s) || s.height < h0 + 25 + 0.002 * d) continue;
                if (!any || s.height > hs) { hs = s.height; src = u; ds = d; any = true; }
            }
            if (any) break;
        }
        if (!any) return;
        const Vec3 end = toward(u0, src, r0 + 60, Rm);
        const double len = arcM(src, end, Rm);
        if (len < 1500) return;
        const Vec3 mid = slerpUnit(src, end, 0.5);
        if (!clearOf(mid, 120, true)) return;
        const Frame F(mid, Rm);
        const double hEnd = at(end).height, c0 = hs - 3, c1 = std::max(hEnd + 5, c0 - 0.004 * len);
        Work& w = add(WO_AQUEDUCT, mid, len * 0.5 + 80, seedOf(WO_AQUEDUCT, k++));
        w.age = age; w.place = e.place; w.event = i; w.year = e.year; int pa; placeAge(e.place, pa, w.before);
        if (H->places[e.place].plan >= 0) w.planRank = H->places[e.place].plan;
        w.length = len; w.heading = bearingOf(src, end); w.level = c0; w.crest = c1;
        double sx, sz, ex, ez; F.local(src, sx, sz); F.local(end, ex, ez);
        w.sideA = src; w.sideB = end;
        // the conduit's line on the ground (its stretches the arches do not carry)
        GroundEdit line; line.kind = GE_LINE; line.width = 2.5; line.tone = ET_DARK;
        addPt(line, sx, sz); addPt(line, ex, ez);
        w.edits.push_back(line);
        (void)ds;
    });
}

// ---- the observatories: on the highest summit near their town -------------------------------------------------------------------
void WB::observatories() {
    if (age < AGE_MEDIEVAL || age == AGE_FAR_FUTURE) return;   // B-414: the far future's own sky is read by nothing a futuristic people built
    Rng rng(pseed ^ 0x70350ULL);
    std::vector<std::pair<int, int>> src;   // (history place, event)
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) { if (e.thing == WK_OBSERVATORY && e.place >= 0 && src.size() < 3 && !gone(e.place)) src.push_back({e.place, i}); });
    if (src.empty() && age >= AGE_FUTURISTIC && !pp->places.empty()) src.push_back({0, -1});   // a futuristic people's array by its first great place
    int k = 0;
    for (auto [hp, ev] : src) {
        Vec3 u0; double r0;
        if (H && hp < (int)H->places.size()) placeAt(hp, u0, r0); else { u0 = pp->places[0].unit; r0 = pp->places[0].radiusM; }
        Vec3 top; double hTop;
        if (!summitNear(u0, r0 + 1500, r0 + 24000, rng, top, hTop)) continue;
        if (!clearOf(top, 180)) continue;
        Work& w = add(WO_OBSERVATORY, top, 180, seedOf(WO_OBSERVATORY, k++));
        w.age = clampi(age, AGE_MEDIEVAL, AGE_FUTURISTIC);
        if (ev >= 0) { w.event = ev; w.year = H->events[ev].year; }
        w.place = hp; int pa; placeAge(hp, pa, w.before);
        if (H && hp < (int)H->places.size() && H->places[hp].plan >= 0) w.planRank = H->places[hp].plan;
        w.style = w.age - AGE_MEDIEVAL; w.height = hTop; w.heading = bearingOf(top, u0);
    }
}

// ---- the shrine at the holy place, and the pilgrims' way to it ---------------------------------------------------------------------
void WB::shrine() {
    if (!H || age >= AGE_FAR_FUTURE) return;
    const int holy = H->profile.holy;
    if (holy != HOLY_HIGH_PLACE && holy != HOLY_SPRING && holy != HOLY_CAVE && holy != HOLY_GROVE && holy != HOLY_OPEN_SKY) return;
    Rng rng(pseed ^ 0x70360ULL);
    int hp = 0, ev = -1;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) { if (ev < 0 && e.thing == WK_HOLY && e.place >= 0) { hp = e.place; ev = i; } });
    if (hp >= (int)H->places.size() || gone(hp)) return;
    Vec3 u0; double r0; placeAt(hp, u0, r0);
    Vec3 site; bool any = false; double heading = 0;
    if (holy == HOLY_HIGH_PLACE) { double h; any = summitNear(u0, r0 + 4000, r0 + 38000, rng, site, h); }
    else {
        double bestS = -1e18;
        const double a0 = rng.uni() * TAU;
        for (double d : {3000.0, 6000.0, 10000.0, 15000.0, 21000.0})
            for (int q = 0; q < 20; q++) {
                const double ang = a0 + q * TAU / 20;
                const Vec3 u = offsetUnit(u0, std::sin(ang) * (r0 + d), std::cos(ang) * (r0 + d), Rm);
                const SurfaceSample s = at(u);
                if (wetS(s)) continue;
                double down, h; const double sl = grade(u, 45, down, h);
                double sc = -d / 30000;
                switch (holy) {
                    case HOLY_CAVE: if (sl < 0.3) continue; sc += sl + (matFamily(s.material) == FAM_ROCK ? 0.4 : 0); break;
                    case HOLY_GROVE: if (s.material != MAT_FOREST && s.veg < 0.55) continue; sc += s.veg; break;
                    case HOLY_SPRING: { double lowest = 1e18; for (int m = 0; m < 6; m++) { const double b = m * TAU / 6; lowest = std::min(lowest, at(offsetUnit(u, std::sin(b) * 250, std::cos(b) * 250, Rm)).height); } if (lowest < h - 2) continue; sc += 0.5 - sl; break; }
                    default: if (sl > 0.05 || s.material == MAT_FOREST) continue; sc += 0.5 - sl * 5; break;
                }
                if (sc > bestS) { bestS = sc; site = u; heading = down; any = true; }
            }
    }
    if (!any || !clearOf(site, 60)) return;
    Work& w = add(WO_SHRINE, site, 60, seedOf(WO_SHRINE, 0));
    w.age = age;
    if (ev >= 0) { w.event = ev; w.year = H->events[ev].year; }
    w.place = hp; w.planRank = H->places[hp].plan; w.style = holy; w.heading = holy == HOLY_CAVE ? heading : bearingOf(site, u0);
    if (holy == HOLY_GROVE) { GroundEdit e; e.kind = GE_FLAT; e.r = 20; e.width = 8; e.level = at(site).height; e.tone = ET_CLEARED; e.fades = false; w.edits.push_back(e); }   // its clearing
    const int sAge = w.age; const double sBefore = w.before; const Vec3 sU = site;
    // the way: from the place's edge to the shrine, a beaten track with a cairn or a stela every 600 m, in stretches of three kilometres
    const Vec3 a = toward(u0, sU, r0 + 80, Rm);
    const double L = arcM(a, sU, Rm);
    if (L < 1200) return;
    const double ph1 = rng.uni() * TAU, ph2 = rng.uni() * TAU, amp = std::min(400.0, L * 0.025);
    const int nPts = std::max(2, (int)(L / 100));
    std::vector<Vec3> pts;
    for (int q = 0; q <= nPts; q++) {
        const double t = (double)q / nPts;
        Vec3 c = slerpUnit(a, sU, t);
        const double side = amp * std::sin(PI * t) * (std::sin(ph1 + t * 7.0) * 0.7 + std::sin(ph2 + t * 17.0) * 0.3);
        const double brg = bearingOf(a, sU) + PI / 2;
        c = offsetUnit(c, std::sin(brg) * side, std::cos(brg) * side, Rm);
        pts.push_back(c);
    }
    const uint32_t group = (uint32_t)mix64(pseed ^ 0x7036AAULL);
    int seg = 0;
    for (size_t q0 = 0; q0 + 1 < pts.size(); q0 += 30) {
        const size_t q1 = std::min(pts.size() - 1, q0 + 30);
        const Vec3 mid = pts[(q0 + q1) / 2];
        Work& s = add(WO_WAY, mid, arcM(pts[q0], pts[q1], Rm) * 0.5 + 40, seedOf(WO_WAY, seg));
        s.age = sAge; s.before = sBefore; s.place = hp; s.planRank = H->places[hp].plan; s.heard = false; s.group = group; s.style = seg++;
        const Frame F(mid, Rm);
        GroundEdit e; e.kind = GE_LINE; e.width = 2.4; e.tone = ET_DARK;
        for (size_t q = q0; q <= q1; q++) {
            if (wetS(at(pts[q]))) { if (e.pts.size() >= 4) s.edits.push_back(e); e.pts.clear(); continue; }
            double x, z; F.local(pts[q], x, z); addPt(e, x, z);
        }
        if (e.pts.size() >= 4) s.edits.push_back(e);
    }
}

// ---- lines on a plain: a figure, rays and cleared trapezoids where the ground is flat and dry ---------------------------------------
namespace {
// a figure's lines in a unit box (x across, z along its length): a creature drawn in one line as the plain's figures are
void figureLines(Rng& r, std::vector<std::vector<std::pair<double, double>>>& out) {
    const int kind = r.irange(5);
    auto ellipse = [&](double cx, double cz, double ax, double az, int n, double a0 = 0, double a1 = TAU) {
        std::vector<std::pair<double, double>> l;
        for (int k = 0; k <= n; k++) { const double t = a0 + (a1 - a0) * k / n; l.push_back({cx + std::sin(t) * ax, cz + std::cos(t) * az}); }
        out.push_back(l);
    };
    auto limb = [&](double x0, double z0, double ang, double len, int fingers) {
        std::vector<std::pair<double, double>> l = {{x0, z0}};
        double x = x0, z = z0, a = ang;
        for (int s = 0; s < 2; s++) { a += r.sym(0.6); x += std::sin(a) * len * 0.5; z += std::cos(a) * len * 0.5; l.push_back({x, z}); }
        out.push_back(l);
        for (int f = 0; f < fingers; f++) { const double fa = a + (f - (fingers - 1) * 0.5) * 0.45; out.push_back({{x, z}, {x + std::sin(fa) * len * 0.22, z + std::cos(fa) * len * 0.22}}); }
    };
    switch (kind) {
        case 0: {   // a bird: its body, the wings out wide, the tail's fan, a long beak
            ellipse(0, 0, 0.09, 0.32, 16);
            for (int s = -1; s <= 1; s += 2) {
                std::vector<std::pair<double, double>> w = {{s * 0.08, 0.05}};
                double x = s * 0.08, z = 0.05;
                const int steps = 5 + r.irange(3);
                for (int k = 0; k < steps; k++) { x += s * 0.16; z += (k % 2 ? 0.08 : -0.05) + r.sym(0.02); w.push_back({x, z}); }
                out.push_back(w);
            }
            for (int f = -2; f <= 2; f++) out.push_back({{0, -0.3}, {f * 0.08, -0.55}});
            out.push_back({{0, 0.32}, {0, 0.75}});
            break;
        }
        case 1: {   // a spiral's creature: a body and its tail wound three times
            ellipse(0, 0.25, 0.2, 0.25, 18);
            std::vector<std::pair<double, double>> sp;
            for (int k = 0; k <= 90; k++) { const double t = k / 90.0, a = t * TAU * 3, rr = 0.36 * (1 - t) + 0.02; sp.push_back({0.2 + std::cos(a) * rr * 0.9, -0.45 + std::sin(a) * rr * 0.9}); }
            out.push_back(sp);
            for (int s = -1; s <= 1; s += 2) limb(s * 0.18, 0.35, s * 1.1, 0.4, 4);
            ellipse(0, 0.58, 0.08, 0.08, 10);
            break;
        }
        case 2: {   // a spider: its two bodies and eight legs
            ellipse(0, 0.18, 0.12, 0.16, 14); ellipse(0, -0.12, 0.17, 0.2, 16);
            for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 4; k++) limb(s * 0.12, 0.25 - k * 0.12, s * (0.7 + k * 0.45), 0.6, 0);
            break;
        }
        case 3: {   // a fish or a whale: its outline, the fins, the forked tail
            ellipse(0, 0.05, 0.22, 0.55, 22, 0.3, TAU - 0.3);
            out.push_back({{0.065, -0.47}, {0.25, -0.8}, {0, -0.62}, {-0.25, -0.8}, {-0.065, -0.47}});
            for (int s = -1; s <= 1; s += 2) out.push_back({{s * 0.2, 0.1}, {s * 0.42, -0.05}, {s * 0.2, -0.12}});
            ellipse(0.06, 0.38, 0.03, 0.03, 8);
            break;
        }
        default: {   // a lizard: its body, four legs, the long tail
            ellipse(0, 0.12, 0.11, 0.28, 16);
            ellipse(0, 0.48, 0.07, 0.09, 10);
            for (int s = -1; s <= 1; s += 2) { limb(s * 0.1, 0.25, s * 1.0, 0.36, 3); limb(s * 0.1, -0.05, s * 2.1, 0.36, 3); }
            std::vector<std::pair<double, double>> t = {{0, -0.16}};
            for (int k = 1; k <= 8; k++) t.push_back({std::sin(k * 0.8) * 0.06 * k / 8.0 + 0.02 * k, -0.16 - k * 0.08});
            out.push_back(t);
            break;
        }
    }
}
}

void WB::geoglyphs() {
    if (age > AGE_MEDIEVAL) return;
    Rng rng(pseed ^ 0x70380ULL);
    std::vector<std::pair<float, int>> plains;
    for (int c = 0; c < P.grid.cells; c++)
        if (pp->territory[c] && P.grid.land[c] && P.grid.relief[c] < 0.14f && P.grid.veg[c] < 0.45f && P.grid.tempC[c] > -3) plains.push_back({P.grid.relief[c] + 0.3f * (float)unitFromHash(mix64(pseed ^ (uint64_t)c)), c});
    std::sort(plains.begin(), plains.end());
    const int want = 1 + (pp->areaKm2 > 400000 ? 1 : 0) + (pp->areaKm2 > 1500000 ? 1 : 0);
    int k = 0;
    for (const auto& pc : plains) {
        if (made[WO_GEOGLYPH] >= want || k > 12) break;
        k++;
        const Vec3 c0 = offsetUnit(P.grid.unitOf(pc.second), rng.sym(6000), rng.sym(6000), Rm);
        Vec3 best; double bestRange = 1e18; bool any = false;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) {
            const Vec3 u = offsetUnit(c0, i * 1500.0, j * 1500.0, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || s.material == MAT_FOREST || matFamily(s.material) == FAM_SNOW) continue;
            double lo = s.height, hi = s.height;
            for (int m = 0; m < 6; m++) { const double b = m * TAU / 6; const SurfaceSample q = at(offsetUnit(u, std::sin(b) * 450, std::cos(b) * 450, Rm)); if (wetS(q)) { lo = -1e18; break; } lo = std::min(lo, q.height); hi = std::max(hi, q.height); }
            if (lo < -1e17) continue;
            if (hi - lo < bestRange) { bestRange = hi - lo; best = u; any = true; }
        }
        if (!any || bestRange > 14 || !clearOf(best, 900)) continue;
        Work& w = add(WO_GEOGLYPH, best, 900, seedOf(WO_GEOGLYPH, k));
        w.age = age; w.style = 0;
        Rng r2(w.seed);
        const double S = 160 + 140 * r2.uni(), hd = r2.uni() * TAU;   // the figure's half size: its lines cleared bands wide enough to read from the air
        std::vector<std::vector<std::pair<double, double>>> lines;
        figureLines(r2, lines);
        const double lw = 8 + 4 * r2.uni();
        for (const auto& l : lines) {
            GroundEdit e; e.kind = GE_LINE; e.width = lw; e.tone = ET_CLEARED;
            for (const auto& q : l) { double x, z; rot(hd, q.first * S * 2, q.second * S * 2, x, z); addPt(e, x, z); }
            w.edits.push_back(e);
        }
        // the rays from a centre beside it, and the cleared trapezoids
        const double ca = r2.uni() * TAU, cd = S * 1.6 + 120;
        const double rx = std::sin(ca) * cd, rz = std::cos(ca) * cd;
        const int nr = 3 + r2.irange(6);
        for (int q = 0; q < nr; q++) {
            const double a = r2.uni() * TAU, len = 900 + 1500 * r2.uni();
            GroundEdit e; e.kind = GE_LINE; e.width = 6; e.tone = ET_CLEARED;
            addPt(e, rx, rz); addPt(e, rx + std::sin(a) * len, rz + std::cos(a) * len);
            w.edits.push_back(e);
        }
        const int nt = r2.irange(3);
        for (int q = 0; q < nt; q++) {
            const double a = r2.uni() * TAU, len = 500 + 900 * r2.uni(), off = r2.sym(250);
            GroundEdit e; e.kind = GE_LINE; e.width = 55 + 40 * r2.uni(); e.width2 = 14 + 10 * r2.uni(); e.tone = ET_CLEARED;
            const double sx = rx + std::cos(a) * off, sz = rz - std::sin(a) * off;
            addPt(e, sx, sz); addPt(e, sx + std::sin(a) * len, sz + std::cos(a) * len);
            w.edits.push_back(e);
        }
        w.sideA = Vec3(rx, 0, rz);   // the rays' centre, in the work's metres (its cairns)
        w.length = S * 2;
    }
}

// ---- the fortified line: along the frontier with the other people, or across the way into the land the people held -----------------
void WB::fortLine() {
    if (age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC || pp->places.empty()) return;
    Rng rng(pseed ^ 0x703A0ULL);
    const PlanGrid& G = P.grid;
    Vec3 mid, dir; bool any = false;
    if (P.peoples > 1) {   // the frontier: our cell nearest the divide where the other's territory comes near
        const Vec3 pole = peopleDivide(g);
        const double sideSign = p == 0 ? 1 : -1;
        double best = 1e18;
        for (int c = 0; c < G.cells; c++) {
            if (!pp->territory[c] || !G.land[c]) continue;
            const Vec3 u = G.unitOf(c);
            const double dd = std::fabs(dot(u, pole));
            if (dd > best) continue;
            int nb[8]; const int nn = G.neighbours(c, nb);
            bool near = false; for (int q = 0; q < nn; q++) if (P.plans[1 - p].territory[nb[q]]) near = true;
            for (int q = 0; q < nn && !near; q++) { int nb2[8]; const int n2 = G.neighbours(nb[q], nb2); for (int m = 0; m < n2; m++) if (P.plans[1 - p].territory[nb2[m]]) near = true; }
            if (!near) continue;
            best = dd; mid = u; any = true;
        }
        if (any) {
            mid = normalize(mid + pole * (sideSign * 0.25 * G.sizeKm() / G.R));   // a few kilometres in on our side
            dir = normalize(cross(pole, mid));
        }
    }
    if (!any) {   // the border nearest the capital that faces land the people did not hold
        const Vec3 cap = pp->places[0].unit;
        double best = 1e18; int bc = -1, outC = -1;
        for (int c = 0; c < G.cells; c++) {
            if (!pp->territory[c] || !G.land[c]) continue;
            int nb[8]; const int nn = G.neighbours(c, nb);
            for (int q = 0; q < nn; q++) if (!pp->territory[nb[q]] && G.land[nb[q]]) {
                const double d = arcM(G.unitOf(c), cap, Rm);
                if (d < best && d > 20000) { best = d; bc = c; outC = nb[q]; }
                break;
            }
        }
        if (bc < 0) return;
        mid = G.unitOf(bc);
        const Vec3 out = G.unitOf(outC), fwd = normalize(out - mid * dot(out, mid));
        dir = normalize(cross(mid, fwd));
        any = true;
    }
    if (!any) return;
    const double total = 6000 + 9000 * rng.uni(), segL = 2500;
    const int nSeg = std::max(1, (int)std::lround(total / segL));
    const uint32_t group = (uint32_t)mix64(pseed ^ 0x703AAAULL);
    for (int s = 0; s < nSeg; s++) {
        const double t0 = (s - nSeg * 0.5) * segL, t1 = t0 + segL;
        const Vec3 a = normalize(mid * std::cos(t0 / Rm) + dir * std::sin(t0 / Rm)), b = normalize(mid * std::cos(t1 / Rm) + dir * std::sin(t1 / Rm));
        const Vec3 m = slerpUnit(a, b, 0.5);
        if (wetS(at(m)) || planCoreAt(P, m, 300)) continue;
        Work& w = add(WO_FORT_LINE, m, segL * 0.5 + 60, seedOf(WO_FORT_LINE, s));
        w.age = age; w.style = age - AGE_MEDIEVAL; w.group = group; w.heard = s == nSeg / 2; w.heading = bearingOf(a, b); w.length = segL;
        const Frame F(m, Rm);
        double ax, az, bx, bz; F.local(a, ax, az); F.local(b, bx, bz);
        w.sideA = Vec3(ax, 0, az); w.sideB = Vec3(bx, 0, bz);
        if (w.style >= 1) {   // the ditch before the line (toward the far side)
            GroundEdit e; e.kind = GE_DITCH; e.width = w.style == 1 ? 6 : 8; e.depth = w.style == 1 ? 2.4 : 3.0; e.bank = w.style == 1 ? 1.6 : 1.0; e.tone = ET_DARK;
            const double nx = std::cos(w.heading) * 9, nz = -std::sin(w.heading) * 9;
            const int steps = 25;
            for (int q = 0; q <= steps; q++) {
                const double t = (double)q / steps;
                const Vec3 u = slerpUnit(a, b, t);
                if (wetS(at(u))) { if (e.pts.size() >= 4) w.edits.push_back(e); e.pts.clear(); continue; }
                double x, z; F.local(u, x, z); addPt(e, x + nx, z + nz);
            }
            if (e.pts.size() >= 4) w.edits.push_back(e);
        }
    }
}

// ---- the battlefields: where the history fought its battles, between the two sides' places -----------------------------------------------
void WB::battlefields() {
    if (!H) return;
    Rng rng(pseed ^ 0x703B0ULL);
    // each war's greatest battle (and the next of a great war), the greatest wars first
    std::vector<std::pair<int64_t, int>> evs;
    std::map<int, int> perWar;
    forEvents(HE_BATTLE, [&](int i, const HistoryEvent& e) { if (e.place >= 0 && e.thing >= 0 && e.thing < (int)H->wars.size()) evs.push_back({-e.value, i}); });
    std::sort(evs.begin(), evs.end());
    int k = 0;
    for (const auto& pe : evs) {
        if (made[WO_BATTLEFIELD] >= 8) break;
        const HistoryEvent& e = H->events[pe.second];
        if (e.year < ageFrom) continue;   // B-414: a battle of an earlier age left nothing on the ground
        const HistoryWar& war = H->wars[e.thing];
        int& nw = perWar[e.thing];
        if (nw >= (war.dead > 20000 ? 2 : 1)) continue;
        // the two sides: the battle's place, and the place the other side came from (the other people's place, the side of the war's
        // head, or the seat)
        Vec3 ub; double rb; placeAt(e.place, ub, rb);
        Vec3 ua; double ra = 0; bool joint = false;
        if (e.other >= 0 && P.peoples > 1 && e.other < (int)P.plans[1 - p].places.size()) {
            if (p != 0) continue;   // a battle with the other people: the first people's history lays it down (both hold it)
            ua = P.plans[1 - p].places[e.other].unit; ra = P.plans[1 - p].places[e.other].radiusM; joint = true;
        } else {
            int other = war.sideA >= 0 ? war.sideA : historySeat(*H, e.year);
            if (other == e.place && war.sideB >= 0) other = war.sideB;
            if (other < 0 || other == e.place) {   // a war at its own seat: they came from the nearest other place
                double bd = 1e18; for (int q = 0; q < (int)H->places.size(); q++) if (q != e.place && H->placeStands(q, e.year)) { Vec3 uq; double rq; placeAt(q, uq, rq); const double d = arcM(uq, ub, Rm); if (d < bd) { bd = d; other = q; } }
            }
            if (other < 0) continue;
            placeAt(other, ua, ra);
        }
        const double sep = arcM(ua, ub, Rm);
        if (sep < rb + ra + 2500) continue;
        const double d = clampd(0.3 * sep, rb + 1500, std::min(rb + 7000, sep - ra - 1500));
        const Vec3 c0 = toward(ub, ua, d, Rm);
        Vec3 best; double bestS = -1e18; bool any = false;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) {
            const Vec3 u = offsetUnit(c0, i * 350.0, j * 350.0, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || matFamily(s.material) == FAM_SNOW) continue;
            double down, h; const double sl = grade(u, 60, down, h);
            const double sc = -sl * 3 - (s.material == MAT_FOREST ? 0.3 : 0) - std::hypot(i, j) * 0.05;
            if (sc > bestS && clearOf(u, 420)) { bestS = sc; best = u; any = true; }
        }
        if (!any) continue;
        nw++;
        Work& w = add(WO_BATTLEFIELD, best, 420, seedOf(WO_BATTLEFIELD, k++));
        w.age = age; w.before = std::max(0, H->last - e.year); w.event = pe.second; w.year = e.year; w.war = e.thing; w.place = e.place;
        if (H->places[e.place].plan >= 0) w.planRank = H->places[e.place].plan;
        w.sideA = ua; w.sideB = ub; w.heading = bearingOf(ub, ua); w.style = joint ? 1 : 0;
        w.length = sep;
        // the ground: trenches facing each other and the shells' craters between (the machine ages), the glassed craters of the futuristic
        Rng r2(w.seed ^ 0xBA77ULL);
        const int a = w.age;
        const double hx = std::cos(w.heading), hz = -std::sin(w.heading), fx = std::sin(w.heading), fz = std::cos(w.heading);   // across, toward A
        if (a == AGE_INDUSTRIAL || a == AGE_MODERN) {
            for (int side = -1; side <= 1; side += 2) {
                const int lines = a == AGE_INDUSTRIAL ? 2 : 1;
                for (int l = 0; l < lines; l++) {
                    GroundEdit e2; e2.kind = GE_DITCH; e2.width = 2.4; e2.depth = 2.0; e2.bank = 0.8 * side; e2.tone = ET_DARK;
                    const double off = side * (90 + l * 35 + r2.uni() * 20), half = 260 + 120 * r2.uni();
                    for (int q = 0; q <= 26; q++) {
                        const double t = -half + 2 * half * q / 26, zig = (q % 2 ? 7.0 : -7.0) + r2.sym(2);
                        addPt(e2, hx * t + fx * (off + zig), hz * t + fz * (off + zig));
                    }
                    w.edits.push_back(e2);
                }
            }
            const int nc = a == AGE_INDUSTRIAL ? 60 + r2.irange(60) : 40 + r2.irange(50);
            for (int q = 0; q < nc; q++) {
                GroundEdit c; c.kind = GE_BOWL; c.r = a == AGE_INDUSTRIAL ? 2 + 3 * r2.uni() : 3 + 7 * r2.uni() * r2.uni(); c.depth = c.r * 0.45; c.tone = ET_DARK;
                const double t = r2.sym(300), s = r2.sym(120);
                c.x = hx * t + fx * s; c.z = hz * t + fz * s;
                w.edits.push_back(c);
            }
        } else if (a == AGE_FUTURISTIC) {
            const int nc = 5 + r2.irange(10);
            for (int q = 0; q < nc; q++) {
                GroundEdit c; c.kind = GE_BOWL; c.r = 10 + 28 * r2.uni(); c.depth = c.r * 0.3; c.tone = ET_GLASS;
                const double t = r2.sym(320), s = r2.sym(160);
                c.x = hx * t + fx * s; c.z = hz * t + fz * s;
                w.edits.push_back(c);
            }
        }
    }
}

// ---- the launch sites: near the equator, on flat ground, the sea to the east where it can be ------------------------------------------
void WB::launchSites() {
    if (!H || age < AGE_MODERN || age == AGE_FAR_FUTURE) return;
    std::vector<std::pair<int, int>> src;   // (event, the work: WK_LAUNCH or WK_ELEVATOR)
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) { if ((e.thing == WK_LAUNCH || e.thing == WK_ELEVATOR) && src.size() < 2) src.push_back({i, e.thing}); });
    if (src.empty()) return;
    Rng rng(pseed ^ 0x703C0ULL);
    const PlanGrid& G = P.grid;
    std::vector<std::pair<double, int>> low;
    for (int c = 0; c < G.cells; c++) {
        if (!pp->territory[c] || !G.land[c] || G.relief[c] > 0.35f) continue;
        double la, lo; G.latLonOf(c, la, lo);
        double sc = std::fabs(la);
        int nb[8]; const int nn = G.neighbours(c, nb);
        for (int q = 0; q < nn; q++) if (!G.land[nb[q]]) { double la2, lo2; G.latLonOf(nb[q], la2, lo2); if (std::remainder(lo2 - lo, TAU) > 0) sc -= 0.03; }   // the sea to the east
        low.push_back({sc, c});
    }
    std::sort(low.begin(), low.end());
    int k = 0;
    for (const auto& se : src) {
        bool done = false;
        for (int li = 0; li < (int)low.size() && li < 12 && !done; li++) {
            const Vec3 c0 = offsetUnit(G.unitOf(low[li].second), rng.sym(8000), rng.sym(8000), Rm);
            Vec3 best; double bestS = -1e18; bool any = false;
            for (int j = -3; j <= 3; j++) for (int i = -3; i <= 3; i++) {
                const Vec3 u = offsetUnit(c0, i * 700.0, j * 700.0, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s) || s.material == MAT_FOREST) continue;
                double down, h; const double sl = grade(u, 120, down, h);
                if (sl > 0.03) continue;
                const double sc = -sl * 20 - std::hypot(i, j) * 0.02;
                if (sc > bestS && clearOf(u, 1300)) { bestS = sc; best = u; any = true; }
            }
            if (!any) continue;
            const HistoryEvent& e = H->events[se.first];
            Work& w = add(WO_LAUNCH, best, 1300, seedOf(WO_LAUNCH, k++));
            w.age = clampi(age, AGE_MODERN, AGE_FUTURISTIC); w.event = se.first; w.year = e.year;   // stood where the latitude put it, not by its town
            w.style = se.second == WK_ELEVATOR ? 2 : (w.age == AGE_FUTURISTIC ? 1 : 0);
            w.heading = PI / 2 + rng.sym(0.3);   // the pads toward the east
            Rng r2(w.seed);
            if (w.style == 0) {   // the pads' aprons made flat, the crawlerways from the hall to each
                const int pads = 1 + r2.irange(3);
                w.height = pads;
                for (int q = 0; q < pads; q++) {
                    const double t = (q - (pads - 1) * 0.5) * 700, along = 650;
                    double x, z; rot(w.heading, t, along, x, z);
                    GroundEdit f; f.kind = GE_FLAT; f.x = x; f.z = z; f.r = 95; f.width = 25; f.level = at(Frame(best, Rm).unit(x, z)).height; f.tone = ET_PAVED; f.fades = false;
                    w.edits.push_back(f);
                    GroundEdit l; l.kind = GE_LINE; l.width = 28; l.tone = ET_PAVED; l.fades = false;
                    double hx, hz; rot(w.heading, 0, -380, hx, hz);
                    addPt(l, hx, hz); addPt(l, x, z);
                    w.edits.push_back(l);
                }
            } else if (w.style == 1) {   // a mass driver's track rising east
                GroundEdit l; l.kind = GE_LINE; l.width = 14; l.tone = ET_PAVED; l.fades = false;
                double x0, z0, x1, z1; rot(w.heading, 0, -1100, x0, z0); rot(w.heading, 0, 1200, x1, z1);
                addPt(l, x0, z0); addPt(l, x1, z1);
                w.edits.push_back(l);
            } else {
                GroundEdit f; f.kind = GE_FLAT; f.r = 260; f.width = 40; f.level = at(best).height; f.tone = ET_PAVED; f.fades = false;
                w.edits.push_back(f);
            }
            done = true;
        }
    }
}

// ---- the antenna field: on a high plain, away from the towns ------------------------------------------------------------------------
void WB::antennas() {
    if (!H || !H->profile.radio || age < AGE_INDUSTRIAL || age == AGE_FAR_FUTURE || pp->places.empty()) return;
    Rng rng(pseed ^ 0x703D0ULL);
    const PlanGrid& G = P.grid;
    const Vec3 cap = pp->places[0].unit;
    double mean = 0; int n = 0;
    for (int c = 0; c < G.cells; c++) if (pp->territory[c] && G.land[c]) { mean += G.height[c]; n++; }
    if (!n) return;
    mean /= n;
    std::vector<std::pair<double, int>> high;
    for (int c = 0; c < G.cells; c++) {
        if (!pp->territory[c] || !G.land[c] || G.relief[c] > 0.3f) continue;
        const double d = arcM(G.unitOf(c), cap, Rm);
        if (d < 20000 || d > 300000) continue;
        high.push_back({-(G.height[c] - mean) / 400.0 + G.relief[c] * 2 + rng.uni() * 0.3, c});
    }
    std::sort(high.begin(), high.end());
    for (int li = 0; li < (int)high.size() && li < 10; li++) {
        const Vec3 c0 = offsetUnit(G.unitOf(high[li].second), rng.sym(6000), rng.sym(6000), Rm);
        Vec3 best; double bestS = -1e18; bool any = false;
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) {
            const Vec3 u = offsetUnit(c0, i * 800.0, j * 800.0, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s) || s.material == MAT_FOREST || matFamily(s.material) == FAM_SNOW) continue;
            double down, h; const double sl = grade(u, 100, down, h);
            if (sl > 0.05) continue;
            const double sc = -sl * 10 + h * 0.001;
            if (sc > bestS && clearOf(u, 700)) { bestS = sc; best = u; any = true; }
        }
        if (!any) continue;
        Work& w = add(WO_ANTENNA, best, 700, seedOf(WO_ANTENNA, 0));
        w.age = age; w.year = H->profile.radioYear; w.heading = rng.uni() * PI;
        w.style = age == AGE_INDUSTRIAL ? 0 : (age == AGE_FUTURISTIC ? 3 : (rng.uni() < 0.5 ? 1 : 2));
        return;
    }
}

// ---- the colossi: over the capital, or at its harbour's mouth -----------------------------------------------------------------------
void WB::colossi() {
    if (age < AGE_MEDIEVAL || age == AGE_FAR_FUTURE || pp->places.empty()) return;
    const bool want = C.genome.of(GV_FOCUS) == FO_COLOSSUS || C.genome.of(GV_MONUMENTS) == MN_COLOSSI;
    if (!want) return;
    Rng rng(pseed ^ 0x703F0ULL);
    const PlanPlace& cap = pp->places[0];
    Vec3 site; double facing = 0; bool any = false;
    if (cap.coast) { double seaward; if (shoreNear(cap.unit, cap.radiusM + 200, cap.radiusM + 4000, rng, site, seaward, true)) { facing = seaward; any = true; } }
    if (!any) { double h; if (summitNear(cap.unit, cap.radiusM + 600, cap.radiusM + 5000, rng, site, h)) { facing = bearingOf(site, cap.unit); any = true; } }
    if (!any || !clearOf(site, 80)) return;
    Work& w = add(WO_COLOSSUS, site, 80, seedOf(WO_COLOSSUS, 0));
    w.age = age; w.place = 0; w.planRank = 0; w.heading = facing; w.style = (int)(rng.uni() * 2);
    w.height = age == AGE_MEDIEVAL ? 24 + 12 * rng.uni() : (age == AGE_INDUSTRIAL ? 32 + 14 * rng.uni() : (age == AGE_MODERN ? 40 + 20 * rng.uni() : 70 + 40 * rng.uni()));
}

// ---- the terraformed basins: a futuristic people's land made level and laid out in channels -----------------------------------------
void WB::terraforms() {
    if (age != AGE_FUTURISTIC || pp->places.empty()) return;
    Rng rng(pseed ^ 0x70400ULL);
    const PlanGrid& G = P.grid;
    std::vector<std::pair<double, int>> low;
    for (int c = 0; c < G.cells; c++) if (pp->territory[c] && G.land[c] && G.relief[c] < 0.25f) low.push_back({G.height[c] + rng.uni() * 300, c});
    std::sort(low.begin(), low.end());
    int k = 0;
    for (int li = 0; li < (int)low.size() && li < 10 && k < 2; li++) {
        const Vec3 u = offsetUnit(G.unitOf(low[li].second), rng.sym(5000), rng.sym(5000), Rm);
        const SurfaceSample s = at(u);
        if (wetS(s)) continue;
        const double R = 1500 + 2000 * rng.uni();
        if (!clearOf(u, R + 200)) continue;
        double mean = s.height; int nm = 1;
        for (int m = 0; m < 12; m++) { const double b = m * TAU / 12; const SurfaceSample q = at(offsetUnit(u, std::sin(b) * R * 0.7, std::cos(b) * R * 0.7, Rm)); if (!wetS(q)) { mean += q.height; nm++; } }
        mean /= nm;
        Work& w = add(WO_TERRAFORM, u, R + 200, seedOf(WO_TERRAFORM, k++));
        w.age = AGE_FUTURISTIC; w.heading = rng.uni() * PI; w.level = mean; w.length = 2 * R;
        GroundEdit f; f.kind = GE_FLAT; f.r = R; f.width = 160; f.level = mean; f.tone = ET_CLEARED; f.fades = false;
        w.edits.push_back(f);
        const double step = 180;
        for (int q = -(int)(R / step); q <= (int)(R / step); q++) {   // the channels across it
            const double off = q * step, half = std::sqrt(std::max(0.0, (R - 120) * (R - 120) - off * off));
            if (half < 50) continue;
            GroundEdit l; l.kind = GE_LINE; l.width = 6; l.tone = ET_DARK; l.fades = false;
            double x0, z0, x1, z1; rot(w.heading, off, -half, x0, z0); rot(w.heading, off, half, x1, z1);
            addPt(l, x0, z0); addPt(l, x1, z1);
            w.edits.push_back(l);
        }
    }
}

// ---- the last shelter: by the place the collapse ended in, in the nearest steep rock ---------------------------------------------------
// a shelter's site near a place: steep rock, near (a medieval people's citadel on the height); D-05: a people that lost its sun's a hall into the
// rock by the warmest ground
bool WB::shelterSite(const Vec3& u0, double r0, Rng& rng, Vec3& best, double& bestDown) {
    const bool sunlost = H->cause == CAUSE_SUN_LOST, embers = sunlost && g.hasTrait(TR_EMBERS);
    double bestS = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (double d : {600.0, 1400.0, 2600.0, 4200.0, 6500.0})
        for (int q = 0; q < 20; q++) {
            const double ang = a0 + q * TAU / 20, dd = r0 + d;
            const Vec3 u = offsetUnit(u0, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, 40, down, h);
            double sc = std::min(sl, 0.8) - d / 15000 + (matFamily(s.material) == FAM_ROCK ? 0.2 : 0);
            if (age == AGE_MEDIEVAL && !sunlost) sc = h * 0.004 - d / 15000;   // a citadel on the height
            if (embers) sc += 1.5 * emberFieldAt(g, u * g.R);
            if (sc > bestS && clearOf(u, 140)) { bestS = sc; best = u; bestDown = down; any = true; }
        }
    return any;
}
// its way by the age (D-05: a medieval people that lost its sun's the halls under the mountain, a far-future one's a habitat's)
int WB::shelterStyle() const {
    if (age == AGE_MEDIEVAL && H && H->cause == CAUSE_SUN_LOST) return 5;
    return age == AGE_PREHISTORIC ? 0 : (age == AGE_MEDIEVAL ? 1 : (age == AGE_INDUSTRIAL ? 2 : (age == AGE_MODERN ? 3 : 4)));
}
void WB::shelter() {
    if (!H || H->lastPlace < 0 || (age == AGE_FAR_FUTURE && H->cause != CAUSE_SUN_LOST)) return;
    Rng rng(pseed ^ 0x70410ULL);
    Vec3 u0; double r0; placeAt(H->lastPlace, u0, r0);
    Vec3 best; double bestDown = 0;
    if (!shelterSite(u0, r0, rng, best, bestDown)) return;
    Work& w = add(WO_SHELTER, best, 140, seedOf(WO_SHELTER, 0));
    w.age = age; w.place = H->lastPlace; w.planRank = H->places[H->lastPlace].plan; w.year = H->last;
    forEvents(HE_END, [&](int i, const HistoryEvent&) { w.event = i; });
    double down2, h2; const double sl = grade(best, 40, down2, h2);
    w.style = shelterStyle();
    w.heading = sl > 0.12 ? bestDown : bearingOf(best, u0);   // its door out of the hill, else toward the place
    w.height = sl;
}

// ---- D-05: the works of the end of a people that lost its sun ---------------------------------------------------------------------------
// the warmest open ground near a place for a work of `reach`: a field of embers where the world has them, else the lowest dry ground (the
// deep), flat enough
bool WB::warmGround(const Vec3& u0, double r0, double reach, Rng& rng, Vec3& best) {
    const bool embers = g.hasTrait(TR_EMBERS);
    double bestS = -1e18; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (double d : {250.0, 600.0, 1100.0, 1800.0, 2800.0, 4000.0})
        for (int q = 0; q < 14; q++) {
            const double ang = a0 + q * TAU / 14, dd = r0 + reach + d;
            const Vec3 u = offsetUnit(u0, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, std::max(30.0, reach * 0.6), down, h);
            if (sl > 0.14) continue;
            double sc = -h / 400.0 - d / 6000.0 - 3 * sl;
            if (embers) sc += 3.0 * emberFieldAt(g, u * g.R);
            if (sc > bestS && clearOf(u, reach)) { bestS = sc; best = u; any = true; }
        }
    return any;
}
// the places that lived below the longest (the last, then the two left last after the last sunrise): a shelter by each of the others (the last's
// is `shelter`'s; their last records with them, `before` the years before the end it was left), a dome over the warm ground by each (a medieval
// people's vaulted hall over its spring, none for a prehistoric people's: the warm cave was its shelter), its glasshouses beside it (an
// industrial people's and later); the graves of the last generations on a slope facing the last place. Every draw its own stream
void WB::sunlostWorks() {
    if (!H || H->cause != CAUSE_SUN_LOST || H->lastPlace < 0) return;
    std::vector<int> ps;
    for (int i = 0; i < (int)H->places.size(); i++) {
        const HistoryPlace& q = H->places[i];
        if (i == H->lastPlace || q.fate != HP_COLLAPSE || gone(i) || q.ended < H->profile.lastSunrise || q.ended >= H->last) continue;
        ps.push_back(i);
    }
    std::sort(ps.begin(), ps.end(), [&](int a, int b) { return H->places[a].ended != H->places[b].ended ? H->places[a].ended > H->places[b].ended : a < b; });
    if (ps.size() > 2) ps.resize(2);
    for (size_t k = 0; k < ps.size(); k++) {   // their shelters
        const HistoryPlace& q = H->places[ps[k]];
        Rng rng(pseed ^ (0xD05510ULL + k));
        Vec3 u0; double r0; placeAt(ps[k], u0, r0);
        Vec3 best; double bestDown = 0;
        if (!shelterSite(u0, r0, rng, best, bestDown)) continue;
        double down2, h2; const double sl = grade(best, 40, down2, h2);
        Work& w = add(WO_SHELTER, best, 140, seedOf(WO_SHELTER, 1 + (int)k));
        w.place = ps[k]; w.planRank = q.plan; w.year = q.ended; w.event = q.endEvent; w.before = H->last - q.ended;
        w.style = shelterStyle(); w.heading = sl > 0.12 ? bestDown : bearingOf(best, u0); w.height = sl;
    }
    if (age >= AGE_MEDIEVAL) {   // the domes and their glasshouses
        std::vector<int> by = {H->lastPlace}; by.insert(by.end(), ps.begin(), ps.end());
        const int style = age == AGE_MEDIEVAL ? 0 : (age == AGE_INDUSTRIAL ? 1 : (age == AGE_MODERN ? 2 : (age == AGE_FUTURISTIC ? 3 : 4)));
        for (size_t k = 0; k < by.size(); k++) {
            const HistoryPlace& q = H->places[by[k]];
            Rng rng(pseed ^ (0xD05520ULL + k));
            Vec3 u0; double r0; placeAt(by[k], u0, r0);
            const double rad = style == 0 ? 11 + 5 * rng.uni() : 24 + 16 * rng.uni(), reach = rad + 25;
            Vec3 u; if (!warmGround(u0, r0, reach, rng, u)) continue;
            {
                Work& w = add(WO_DOME, u, reach, seedOf(WO_DOME, (int)k));
                w.style = style; w.place = by[k]; w.planRank = q.plan; w.year = q.ended; w.width = rad; w.heading = bearingOf(u, u0);
                w.before = by[k] == H->lastPlace ? 0 : H->last - q.ended;
            }
            if (age < AGE_INDUSTRIAL) continue;
            // its glasshouses: rows of them on flat dry ground beside it, their long sides toward it
            const int gs = age >= AGE_FUTURISTIC ? 2 : (age == AGE_MODERN ? 1 : 0);
            const double hw = gs == 0 ? 4.0 : (gs == 1 ? 4.5 : 6.0), hd = gs == 0 ? 12.0 : (gs == 1 ? 18.0 : 20.0);
            const int rows = 3 + rng.irange(4), per = 1 + rng.irange(2);
            const double across = 0.5 * (rows * (2 * hw + 3) - 3), along = 0.5 * (per * 2 * hd + (per - 1) * 4), greach = std::hypot(across, along) + 6;
            Vec3 best; double bestS = -1e18; bool any = false;
            const double a0 = rng.uni() * TAU;
            for (double d : {20.0, 60.0, 120.0, 220.0, 360.0})
                for (int qq = 0; qq < 12; qq++) {
                    const double ang = a0 + qq * TAU / 12, dd = reach + greach + d;
                    const Vec3 v = offsetUnit(u, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                    if (wetS(at(v))) continue;
                    double down, h; const double sl = grade(v, std::max(25.0, greach * 0.6), down, h);
                    if (sl > 0.08) continue;
                    const double sc = -sl * 10 - d / 300.0;
                    if (sc > bestS && clearOf(v, greach)) { bestS = sc; best = v; any = true; }
                }
            if (!any) continue;
            Work& w = add(WO_GREENHOUSE, best, greach, seedOf(WO_GREENHOUSE, (int)k));
            w.style = gs; w.place = by[k]; w.planRank = q.plan; w.year = q.ended; w.width = across; w.length = along; w.heading = bearingOf(best, u);
            w.before = by[k] == H->lastPlace ? 0 : H->last - q.ended;
        }
    }
    {   // the graves of the last generations: on a slope facing the last place, in its people's way of the dead
        Rng rng(pseed ^ 0xD05530ULL);
        Vec3 u0; double r0; placeAt(H->lastPlace, u0, r0);
        const int dead = C.genome.of(GV_DEAD), a = age;
        int style;
        if (a == AGE_PREHISTORIC || dead == DE_BARROWS) style = 0;
        else if (dead == DE_TOWERS) style = 5;
        else if (a >= AGE_FUTURISTIC) style = 4;
        else if (dead == DE_TOMBS) style = 1;
        else style = a == AGE_MODERN ? 3 : 2;
        const double reach = style == 2 || style == 3 ? 170 : 120, ph = at(u0).height;
        Vec3 best; double bestS = -1e18, bestDown = 0; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {250.0, 500.0, 900.0, 1600.0, 2500.0})
            for (int q = 0; q < 16; q++) {
                const double ang = a0 + q * TAU / 16, dd = r0 + reach + d;
                const Vec3 u = offsetUnit(u0, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s) || s.material == MAT_FOREST) continue;
                double down, h; const double sl = grade(u, 50, down, h);
                if (sl > 0.25) continue;
                const double sc = std::cos(down - (ang + PI)) * std::min(sl, 0.15) * 4 - d / 8000 - std::max(0.0, h - ph) / 2000;   // a slope that looks at the place
                if (sc > bestS && clearOf(u, reach)) { bestS = sc; best = u; bestDown = down; any = true; }
            }
        if (any) {
            Work& w = add(WO_NECROPOLIS, best, reach, seedOf(WO_NECROPOLIS, 0));
            w.style = style; w.place = H->lastPlace; w.planRank = H->places[H->lastPlace].plan; w.year = H->last; w.heading = bestDown;
        }
    }
}

// ---- the far future's inexplicable: a plain made flat, pillars to the horizon, a mountain cut into terraces, a lattice over a valley, a
// sphere on its point -------------------------------------------------------------------------------------------------------------------
void WB::strange() {
    if (age != AGE_FAR_FUTURE || pp->places.empty()) return;
    Rng rng(pseed ^ 0x70420ULL);
    const PlanGrid& G = P.grid;
    const int want = 2 + rng.irange(3);
    std::vector<int> kinds = {0, 1, 2, 3, 4};
    for (int q = 4; q > 0; q--) std::swap(kinds[q], kinds[rng.irange(q + 1)]);
    int k = 0;
    for (int kk = 0; kk < 5 && k < want; kk++) {
        const int st = kinds[kk];
        if (st == 2) {   // the territory's highest mountain cut into terraces
            int bc = -1; double bh = -1e18;
            for (int c = 0; c < G.cells; c++) if (pp->territory[c] && G.land[c] && G.height[c] > bh) { bh = G.height[c]; bc = c; }
            if (bc < 0) continue;
            Vec3 top; double hTop;
            if (!summitNear(G.unitOf(bc), 500, 15000, rng, top, hTop)) continue;
            const double R = 1800 + 1200 * rng.uni();
            if (!clearOf(top, R)) continue;
            Work& w = add(WO_STRANGE, top, R + 100, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 2; w.height = hTop;
            // each ring band cut flat at the lowest ground of its ring, rising inward: treads cut into the mountain, a riser at each band's
            // inner edge (the levels in `pts`, the outermost first)
            const int nb = clampi((int)(R / 160), 6, 14);
            GroundEdit e; e.kind = GE_TERRACES; e.r = R; e.step = R / nb; e.fades = false; e.tone = ET_GLASS;
            double prev = -1e18;
            for (int b = 0; b < nb; b++) {
                const double rr = R - (b + 0.5) * e.step;
                double lo = 1e18; for (int m = 0; m < 16; m++) { const double a = m * TAU / 16; lo = std::min(lo, at(offsetUnit(top, std::sin(a) * rr, std::cos(a) * rr, Rm)).height); }
                lo = std::max(lo - 4, prev + 6);
                e.pts.push_back((float)lo); prev = lo;
            }
            e.level = e.pts[0]; e.depth = (e.pts.back() - e.pts[0]) / std::max(1, nb - 1);
            w.edits.push_back(e); w.level = e.level;
            continue;
        }
        // a site on the people's land: flat for the plain, the pillars, the sphere; a valley for the lattice
        Vec3 best; bool any = false; double bestS = -1e18;
        for (int tries = 0; tries < 24; tries++) {
            const int c = (int)(rng.uni() * G.cells);
            if (!pp->territory[c] || !G.land[c]) continue;
            const Vec3 u = offsetUnit(G.unitOf(c), rng.sym(8000), rng.sym(8000), Rm);
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, 150, down, h);
            const double sc = st == 3 ? -std::fabs(sl - 0.12) : -sl;
            if (sc > bestS) { bestS = sc; best = u; any = true; }
        }
        if (!any) continue;
        if (st == 0) {   // a plain made flat
            const double R = 3000 + 3000 * rng.uni();
            if (!clearOf(best, R)) continue;
            Work& w = add(WO_STRANGE, best, R + 100, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 0;
            double mean = at(best).height; for (int m = 0; m < 12; m++) { const double b = m * TAU / 12; mean += at(offsetUnit(best, std::sin(b) * R * 0.6, std::cos(b) * R * 0.6, Rm)).height; } mean /= 13;
            GroundEdit f; f.kind = GE_FLAT; f.r = R; f.width = 30; f.level = mean; f.tone = ET_GLASS; f.fades = false;
            w.edits.push_back(f); w.level = mean;
        } else if (st == 1) {   // pillars to the horizon: one every 400 m along a great circle, in stretches of three kilometres
            const double hd = rng.uni() * TAU, total = 24000 + 16000 * rng.uni(), segL = 3000;
            const Vec3 dir = normalize(offsetUnit(best, std::sin(hd) * 1000, std::cos(hd) * 1000, Rm) - best);
            const uint32_t group = (uint32_t)mix64(pseed ^ 0x7042AAULL);
            const int nSeg = (int)(total / segL);
            int placed = 0;
            for (int s = 0; s < nSeg; s++) {
                const double t0 = (s - nSeg * 0.5) * segL, t1 = t0 + segL;
                const Vec3 a = normalize(best * std::cos(t0 / Rm) + dir * std::sin(t0 / Rm)), b = normalize(best * std::cos(t1 / Rm) + dir * std::sin(t1 / Rm));
                const Vec3 m = slerpUnit(a, b, 0.5);
                if (planCoreAt(P, m, 400)) continue;
                Work& w = add(WO_STRANGE, m, segL * 0.5 + 60, seedOf(WO_STRANGE, 50 + s));
                w.age = AGE_FAR_FUTURE; w.style = 1; w.group = group; w.heard = s == nSeg / 2; w.heading = bearingOf(a, b); w.length = segL;
                placed++;
            }
            if (placed) k++;
        } else if (st == 3) {   // a lattice over the valley: its girders at the height of the valley's shoulders
            double down, h; grade(best, 150, down, h);
            const double hw = 300 + 300 * rng.uni(), hd = 200 + 200 * rng.uni();
            if (!clearOf(best, std::hypot(hw, hd))) continue;
            double top = -1e18; for (int m = 0; m < 8; m++) { const double b = m * TAU / 8; top = std::max(top, at(offsetUnit(best, std::sin(b) * hw, std::cos(b) * hw, Rm)).height); }
            Work& w = add(WO_STRANGE, best, std::hypot(hw, hd) + 60, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 3; w.heading = down + PI / 2; w.length = 2 * hw; w.width = 2 * hd; w.floor = h; w.crest = std::max(top, h + 60) + 20;
        } else {   // a sphere resting on its point
            const double R = 60 + 80 * rng.uni();
            if (!clearOf(best, R * 2)) continue;
            Work& w = add(WO_STRANGE, best, R * 2, seedOf(WO_STRANGE, k++));
            w.age = AGE_FAR_FUTURE; w.style = 4; w.height = R;
        }
    }
}

// ---- the lighthouses: on the headland nearest the harbour the history lit ----------------------------------------------------------------
void WB::lighthouses() {
    if (!H || age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC) return;
    Rng rng(pseed ^ 0x70440ULL);
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_LIGHTHOUSE || e.place < 0 || made[WO_LIGHTHOUSE] >= 3 || gone(e.place)) return;
        Vec3 u0; double r0; placeAt(e.place, u0, r0);
        Vec3 shore; double seaward;
        if (!shoreNear(u0, r0 + 400, r0 + 9000, rng, shore, seaward, true)) return;
        const Vec3 site = offsetUnit(shore, -std::sin(seaward) * 25, -std::cos(seaward) * 25, Rm);   // a little back from the edge
        if (!clearOf(site, 50)) return;
        Work& w = add(WO_LIGHTHOUSE, site, 50, seedOf(WO_LIGHTHOUSE, k++));
        w.age = clampi(age, AGE_MEDIEVAL, AGE_MODERN); w.event = i; w.year = e.year; w.place = e.place; int pa; placeAge(e.place, pa, w.before);
        if (H->places[e.place].plan >= 0) w.planRank = H->places[e.place].plan;
        w.heading = seaward; w.height = w.age == AGE_MEDIEVAL ? 22 + 10 * rng.uni() : 30 + 25 * rng.uni();
    });
}
// ---- B-418: the works of the land ---------------------------------------------------------------------------------------------------
namespace {
// the share of a land's cells holding a work where the countryside's density is full, by the age: a prehistoric people's flint pits, barrows
// and stones thinner than a later people's quarries, mines, cemeteries, wrecks and wind farms; a futuristic people's land is wild but round
// its great places (`planCountryside`), the far future's none (its works are the world's)
const double LAND_RATE[AGE_COUNT] = {0.34, 0.34, 0.36, 0.36, 0.5, 0};
constexpr double LAND_REACH_MAX = 1700;   // the farthest a land's work reaches from its middle (a wind farm's rows)
double landCellLat(const BodyGen& g) { return LAND_CELL_M / (g.R * 1000.0); }
LandCell landCellAt(const BodyGen& g, int gLat, int gLon) {
    LandCell L; const double dLat = landCellLat(g); const int n = landCellsAround(g, gLat);
    L.gLat = gLat; L.gl = ((gLon % n) + n) % n;
    L.lat0 = gLat * dLat; L.lat1 = L.lat0 + dLat; L.latMid = L.lat0 + 0.5 * dLat;
    L.halfLon = 0.5 * TAU / n; L.lonMid = std::remainder((L.gl + 0.5) * TAU / n, TAU);
    L.mid = StarSystem::bodyFromLatLon(L.latMid, L.lonMid);
    L.h = hash2i(gLat, L.gl, g.seed ^ 0xB418C311ULL);
    return L;
}
// a point's reach `r` lies inside the cell with 60 m to spare (no two of the land's works touch)
bool landInside(const LandCell& L, const Vec3& u, double r, double Rm) {
    double lat, lon; StarSystem::latLonFromBody(u, lat, lon);
    const double m = (r + 60) / Rm;
    if (lat < L.lat0 + m || lat > L.lat1 - m) return false;
    return std::fabs(std::remainder(lon - L.lonMid, TAU)) <= L.halfLon - m / std::max(std::cos(lat), 0.05);
}
// candidates for a work of reach `r`: an n x n jittered grid over the part of the cell its reach stays inside
void landPoints(const LandCell& L, double r, int n, Rng& rng, double Rm, std::vector<Vec3>& out) {
    out.clear();
    const double m = (r + 60) / Rm, la0 = L.lat0 + m, la1 = L.lat1 - m;
    if (la1 <= la0) return;
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            const double la = la0 + (la1 - la0) * (j + 0.15 + 0.7 * rng.uni()) / n;
            const double hl = L.halfLon - m / std::max(std::cos(la), 0.05);
            if (hl <= 0) continue;
            out.push_back(StarSystem::bodyFromLatLon(la, L.lonMid - hl + 2 * hl * (i + 0.15 + 0.7 * rng.uni()) / n));
        }
}
uint64_t landSeed(const LandCell& L, int kind) { return mix64(L.h ^ (0xB4180000ULL + (uint64_t)kind)); }
}   // namespace

// the settlement a work in the cell would serve: the greatest of the plan's places reaching into it and of the countryside's in its 2 km cells,
// the nearest of the greatest
LandSettle WB::landSettle(const LandCell& L) {
    LandSettle S; double bestKey = -1e18;
    auto take = [&](const Vec3& u, double r, int sclass, int rank, bool river) {
        const double key = settlementRank(sclass) * 1e7 - arcM(u, L.mid, Rm);
        if (key > bestKey) { bestKey = key; S.found = true; S.u = u; S.r = r; S.sclass = sclass; S.rank = rank; S.river = river; }
    };
    std::vector<const PlanPlace*> near; planPlacesNear(P, L.mid, 0.75 * LAND_CELL_M, near);
    for (const PlanPlace* q : near) if (q->people == p) take(q->unit, q->radiusM, q->sclass, q->rank, q->river);
    const double dLatR = ruinCellLat(g);
    const int r0 = (int)std::floor(L.lat0 / dLatR), r1 = (int)std::floor(L.lat1 / dLatR);
    for (int row = r0; row <= r1; row++) {
        const double latc = (row + 0.5) * dLatR;
        if (std::fabs(latc) > PI / 2 - dLatR) continue;
        const int nLon = std::max(1, ruinCellsAround(g, row));
        const double dLonR = dLatR / std::max(std::cos(latc), 0.05);
        int c0 = (int)std::floor((L.lonMid - L.halfLon) / dLonR), c1 = (int)std::floor((L.lonMid + L.halfLon) / dLonR);
        if (c1 - c0 + 1 >= nLon) { c0 = 0; c1 = nLon - 1; }
        for (int c = c0; c <= c1; c++) {
            const CellS& cs = cellAt(row, ((c % nLon) + nLon) % nLon);
            if (cs.has && cs.people == p && cs.sclass != SC_MONUMENT) take(cs.u, cs.r, cs.sclass, -1, false);
        }
    }
    return S;
}

// the highest ground of the cell where a work of reach `reach` stays inside it, walked up to its top
bool WB::landSummit(const LandCell& L, double reach, Rng& rng, Vec3& top, double& hTop) {
    std::vector<Vec3> cand; landPoints(L, reach, 6, rng, Rm, cand);
    hTop = -1e18; bool any = false;
    for (const Vec3& u : cand) { const SurfaceSample s = at(u); if (wetS(s) || s.height <= hTop) continue; hTop = s.height; top = u; any = true; }
    if (!any) return false;
    landClimb(L, reach, top, hTop);
    return true;
}
// how far the ground falls across a footprint of radius `r` (its middle, eight points at half of it and eight at it): a work of one form (a
// ring bank) stands on it level, and a crest's rim over a canyon is no crest
double WB::fallOver(const Vec3& u, double r) {
    double lo = at(u).height, hi = lo;
    for (int k = 0; k < 16; k++) {
        const double a = k * TAU / 8 + (k >= 8 ? PI / 8 : 0), d = k >= 8 ? r : 0.5 * r;
        const SurfaceSample s = at(offsetUnit(u, std::sin(a) * d, std::cos(a) * d, Rm));
        if (wetS(s)) return 1e9;
        lo = std::min(lo, s.height); hi = std::max(hi, s.height);
    }
    return hi - lo;
}
// up the slope from `top` to its top, inside the cell
void WB::landClimb(const LandCell& L, double reach, Vec3& top, double& hTop) {
    for (double step : {300.0, 100.0, 35.0})
        for (int it = 0; it < 6; it++) {
            Vec3 nb = top; double hb = hTop;
            for (int k = 0; k < 8; k++) {
                const double a = k * TAU / 8;
                const Vec3 u = offsetUnit(top, std::sin(a) * step, std::cos(a) * step, Rm);
                if (!landInside(L, u, reach, Rm)) continue;
                const SurfaceSample s = at(u);
                if (!wetS(s) && s.height > hb) { hb = s.height; nb = u; }
            }
            if (hb <= hTop) break;
            hTop = hb; top = nb;
        }
}

// a quarry: a prehistoric people's flint pits on open ground; a stone-built people's face cut into the hillside where its rock shows (a pit in
// benches where the rock lies flat), in its age's way (a medieval face, an industrial face in benches, a modern face in great benches); with
// `anyGround` (a medieval people's and later) a pit on any dry ground too, the forest grown over it since (clay, marl, gravel, sand)
bool WB::landQuarry(const LandCell& L, bool anyGround, Rng& rng) {
    const bool flint = age == AGE_PREHISTORIC;
    const int a = std::min(age, (int)AGE_MODERN), style = flint ? 0 : (a <= AGE_MEDIEVAL ? 1 : (a == AGE_INDUSTRIAL ? 2 : 3));
    const double hw = style == 1 ? 18 + 14 * rng.uni() : (style == 2 ? 32 + 22 * rng.uni() : 55 + 40 * rng.uni());
    const double hd = style == 1 ? 15 + 12 * rng.uni() : (style == 2 ? 28 + 18 * rng.uni() : 45 + 30 * rng.uni());
    const double pitR = style == 1 ? 28 + 22 * rng.uni() : (style == 2 ? 60 + 50 * rng.uni() : 140 + 130 * rng.uni());
    const double faceReach = std::hypot(hw, hd) + 45, pitReach = pitR + 70;
    std::vector<Vec3> cand; landPoints(L, flint ? 70 : std::max(faceReach, pitReach), 7, rng, Rm, cand);
    struct Cand { Vec3 u; double score, slope, down, h; };
    std::vector<Cand> cs;
    for (const Vec3& u : cand) {
        const SurfaceSample s = at(u);
        if (wetS(s)) continue;
        const int fam = matFamily(s.material);
        if (flint) { if (fam == FAM_SNOW || fam == FAM_WATER) continue; }   // the forest grown over them since
        else if (fam == FAM_ROCK) { if (C.family != 0) continue; }   // an adobe people cut no stone
        else if (!anyGround || fam == FAM_SNOW || fam == FAM_WATER) continue;
        double down, h; const double sl = grade(u, 45, down, h);
        double sc;
        if (flint) { if (sl > 0.15) continue; sc = 1 - sl * 3; }
        else if (fam != FAM_ROCK) { if (sl >= 0.18) continue; sc = 0.1 - sl; }   // a pit on the open ground
        else if (sl > 1.4) continue;
        else sc = sl >= 0.18 ? 0.3 + std::min(sl, 0.6) : 0.2 - sl;   // a face where the hill stands steep, a pit in flat rock
        cs.push_back({u, sc, sl, down, h});
    }
    std::sort(cs.begin(), cs.end(), [](const Cand& x, const Cand& y) { return x.score > y.score; });
    for (const Cand& c : cs) {
        const bool pit = !flint && c.slope < 0.18;
        const double reach = flint ? 70 : (pit ? pitReach : faceReach);
        if (!clearOf(c.u, reach)) continue;
        Work& w = add(WO_QUARRY, c.u, reach, landSeed(L, WO_QUARRY));
        w.age = flint ? AGE_PREHISTORIC : a; w.style = pit ? (style == 3 ? 4 : 4 + style) : style;
        w.heading = c.down + PI;   // into the hill
        const Frame F(c.u, Rm);
        if (flint) {   // the pits and the spoil round each
            Rng r2(w.seed ^ 0xF117ULL);
            const int np = 14 + r2.irange(16);
            for (int k = 0; k < np; k++) {
                const double rr = 55 * std::sqrt(r2.uni()), aa = r2.uni() * TAU, x = std::sin(aa) * rr, z = std::cos(aa) * rr;
                GroundEdit e; e.kind = GE_PIT; e.x = x; e.z = z; e.r = 2.2 + 1.6 * r2.uni();
                e.level = at(F.unit(x, z)).height - 1.8 - 0.8 * r2.uni(); e.fades = true;
                w.edits.push_back(e);
            }
        } else if (pit) {   // a stone pit in benches: a medieval people's shallow one of three-metre steps, an industrial one's, a modern open pit
            const int nb = style == 1 ? 2 + (int)(rng.uni() * 3) : (style == 2 ? 3 + (int)(rng.uni() * 3) : 4 + (int)(rng.uni() * 4));
            GroundEdit e; e.kind = GE_PIT; e.r = pitR; e.depth = style == 1 ? 3 + rng.uni() : (style == 2 ? 6 + 2 * rng.uni() : 11 + 3 * rng.uni());
            e.step = std::min(style == 1 ? 6.0 : 22.0, pitR * 0.55 / nb); e.level = c.h - nb * e.depth; e.hw = pitR - nb * e.step; e.fades = false;
            w.edits.push_back(e); w.floor = e.level; w.length = 2 * pitR;
        } else {   // the floor level with the ground at the cut's front edge, the benches stepping up into the hill
            const double fx = -std::sin(w.heading) * hd, fz = -std::cos(w.heading) * hd;
            GroundEdit e; e.kind = GE_CUT; e.heading = w.heading; e.hw = hw; e.hd = hd; e.fades = false;
            e.level = std::min(at(F.unit(fx, fz)).height, c.h) - 0.8;   // no higher than its middle's ground (the slope is not a plane)
            const int nb = style == 1 ? 1 : (style == 2 ? 3 : 4);
            e.step = 2 * hd / nb; e.depth = nb > 1 ? (style == 2 ? 6 : 10) : 0;
            w.edits.push_back(e); w.floor = e.level; w.length = 2 * hw; w.width = 2 * hd;
        }
        return true;
    }
    return false;
}

// a mine where the plan read coal and ore: a medieval people's adits along a hillside, an industrial people's headframes on the flat, a modern
// open pit
bool WB::landMine(const LandCell& L, Rng& rng) {
    const int a = clampi(age, AGE_MEDIEVAL, AGE_MODERN), style = a == AGE_MEDIEVAL ? 0 : (a == AGE_INDUSTRIAL ? 1 : 2);
    const double pitR = 220 + 200 * rng.uni(), reach = style == 2 ? pitR + 120 : 240;
    std::vector<Vec3> cand; landPoints(L, reach, 7, rng, Rm, cand);
    Vec3 best; double bestS = -1e18, bestDown = 0, bestH = 0; bool any = false;
    for (const Vec3& u : cand) {
        const SurfaceSample sm = at(u);
        if (wetS(sm) || matFamily(sm.material) == FAM_SNOW) continue;
        double down, h; const double sl = grade(u, 45, down, h);
        double sc = 0;
        if (style == 0) { if (sl < 0.15) continue; sc += std::min(sl, 0.5); }
        else { if (sl > 0.14) continue; sc -= sl; }
        if (matFamily(sm.material) == FAM_ROCK) sc += 0.1;
        if (sc > bestS && clearOf(u, reach)) { bestS = sc; best = u; bestDown = down; bestH = h; any = true; }
    }
    if (!any) return false;
    Work& w = add(WO_MINE, best, reach, landSeed(L, WO_MINE));
    w.age = a;
    mineAt(w, style, bestH, bestDown, pitR, rng);
    return true;
}

// a necropolis on the slope that looks at the settlement it served (across the low ground where a river runs by it), in the people's way
// with its dead (barrows, tombs cut into the rock, a walled garden of graves, a modern cemetery, memorial pillars, towers of silence)
bool WB::landNecropolis(const LandCell& L, const LandSettle& S, Rng& rng) {
    const int dead = C.genome.of(GV_DEAD), a = age;
    int style;
    if (a == AGE_PREHISTORIC || dead == DE_BARROWS) style = 0;
    else if (dead == DE_TOWERS) style = 5;
    else if (a >= AGE_FUTURISTIC) style = 4;
    else if (dead == DE_TOMBS) style = 1;
    else style = a == AGE_MODERN ? 3 : 2;
    const double reach = style == 2 || style == 3 ? 170 : 120, ph = at(S.u).height;
    Vec3 best; double bestS = -1e18, bestDown = 0; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (double d : {250.0, 500.0, 900.0, 1600.0, 2500.0, 3600.0})
        for (int q = 0; q < 16; q++) {
            const double ang = a0 + q * TAU / 16, dd = S.r + reach + d;
            const Vec3 u = offsetUnit(S.u, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
            if (!landInside(L, u, reach, Rm)) continue;
            const SurfaceSample s = at(u);
            if (wetS(s) || s.material == MAT_FOREST || matFamily(s.material) == FAM_SNOW) continue;
            double down, h; const double sl = grade(u, 50, down, h);
            if (sl > 0.25) continue;
            double sc = std::cos(down - (ang + PI)) * std::min(sl, 0.15) * 4 - d / 8000;   // a slope that looks at the settlement
            const SurfaceSample midS = at(slerpUnit(S.u, u, 0.55));
            if (S.river && midS.height < std::min(h, ph) - 4) sc += 0.6;   // across the low ground (the river's) between
            if (wetS(midS)) sc += S.river ? 0.5 : 0.2;
            if (sc > bestS && clearOf(u, reach)) { bestS = sc; best = u; bestDown = down; any = true; }
        }
    if (!any) return false;
    Work& w = add(WO_NECROPOLIS, best, reach, landSeed(L, WO_NECROPOLIS));
    w.style = style; w.planRank = S.rank; w.heading = bestDown;   // down the slope, toward the settlement
    return true;
}

// a henge or rows of standing stones on open ground, on a slight rise
bool WB::landHenge(const LandCell& L, Rng& rng) {
    std::vector<Vec3> cand; landPoints(L, 260, 6, rng, Rm, cand);
    Vec3 best; double bestS = -1e18; bool any = false;
    for (const Vec3& u : cand) {
        const SurfaceSample sm = at(u);
        if (wetS(sm) || sm.material == MAT_FOREST || matFamily(sm.material) == FAM_SNOW) continue;
        double down, h; const double sl = grade(u, 60, down, h);
        if (sl > 0.08) continue;
        double lowest = 1e18; for (int m = 0; m < 6; m++) { const double b = m * TAU / 6; lowest = std::min(lowest, at(offsetUnit(u, std::sin(b) * 400, std::cos(b) * 400, Rm)).height); }
        const double sc = (h - lowest) * 0.05 - sl * 4;
        if (sc > bestS && fallOver(u, 70) <= 4.0 && clearOf(u, 260)) { bestS = sc; best = u; any = true; }   // its ring bank and circle on the level
    }
    if (!any) return false;
    Work& w = add(WO_HENGE, best, 260, landSeed(L, WO_HENGE));
    w.age = AGE_PREHISTORIC;
    Rng r2(w.seed);
    w.style = r2.irange(4);
    w.heading = g.windAngle + r2.sym(0.4);   // the avenue toward a rising of its own
    if (w.style == 0) {   // the ditch round the circle
        const double R = 38 + 30 * r2.uni();
        GroundEdit e; e.kind = GE_DITCH; e.width = 7 + 3 * r2.uni(); e.depth = 2.2; e.bank = 0; e.tone = ET_DARK;
        for (int m = 0; m <= 48; m++) { const double t = m * TAU / 48; if (m > 0 && m < 48 && std::fabs(std::remainder(t - w.heading, TAU)) < 0.12) { if (e.pts.size() >= 4) w.edits.push_back(e); e.pts.clear(); continue; } addPt(e, std::sin(t) * R, std::cos(t) * R); }
        if (e.pts.size() >= 4) w.edits.push_back(e);
        w.width = R;
    }
    return true;
}

// a wreck: a ship cast up on the shore (a medieval people's, an industrial people's steamer, a modern hull), else a modern people's aircraft on
// open flat ground, a futuristic people's wreckage fallen from orbit in its crater
bool WB::landWreck(const LandCell& L, bool coast, Rng& rng) {
    if (coast && age >= AGE_MEDIEVAL && age <= AGE_MODERN) {
        std::vector<Vec3> cand; landPoints(L, 200, 7, rng, Rm, cand);
        for (const Vec3& u : cand) {   // a dry point with the water within 300 m: the shoreline bisected toward it
            if (wetS(at(u))) continue;
            for (int q = 0; q < 8; q++) {
                const double b = q * TAU / 8;
                const SurfaceSample ws = at(offsetUnit(u, std::sin(b) * 300, std::cos(b) * 300, Rm));
                if (!wetS(ws) || ws.oldSea > 0.5) continue;
                double lo = 0, hi = 300;
                for (int it = 0; it < 7; it++) { const double mid = 0.5 * (lo + hi); if (wetS(at(offsetUnit(u, std::sin(b) * mid, std::cos(b) * mid, Rm)))) hi = mid; else lo = mid; }
                const Vec3 pt = offsetUnit(u, std::sin(b) * lo, std::cos(b) * lo, Rm);
                if (!landInside(L, pt, 140, Rm) || !clearOf(pt, 140)) break;
                Work& w = add(WO_WRECK, pt, 140, landSeed(L, WO_WRECK));
                w.style = age - AGE_MEDIEVAL; w.heading = b + PI / 2 + rng.sym(0.5); w.axis = b;   // along the shore, the sea at `axis`
                return true;
            }
        }
    }
    if (age != AGE_MODERN && age != AGE_FUTURISTIC) return false;
    const bool orbit = age == AGE_FUTURISTIC;
    const double reach = orbit ? 400 : 120;
    std::vector<Vec3> cand; landPoints(L, reach, 6, rng, Rm, cand);
    for (const Vec3& u : cand) {
        const SurfaceSample s = at(u);
        if (wetS(s) || s.material == MAT_FOREST) continue;
        double down, h; if (grade(u, 60, down, h) > 0.08) continue;
        if (!clearOf(u, reach)) continue;
        Work& w = add(WO_WRECK, u, reach, landSeed(L, WO_WRECK));
        w.style = orbit ? 4 : 3; w.heading = rng.uni() * TAU;
        if (orbit) { GroundEdit c2; c2.kind = GE_BOWL; c2.r = 60 + 50 * rng.uni(); c2.depth = c2.r * 0.25; c2.tone = ET_DARK; w.edits.push_back(c2); }
        return true;
    }
    return false;
}

// a wind farm: rows of turbines along the cell's highest ridge
bool WB::landWindfarm(const LandCell& L, Rng& rng) {
    Vec3 top; double hTop;
    if (!landSummit(L, 1600, rng, top, hTop) || !clearOf(top, 1600)) return false;
    double down, h; grade(top, 200, down, h);
    Work& w = add(WO_WINDFARM, top, 1600, landSeed(L, WO_WINDFARM));
    w.age = AGE_MODERN; w.heading = down + PI / 2;   // the rows along the ridge
    w.height = 80 + 50 * rng.uni();
    return true;
}

// a shrine where the people's holy place would be (a summit, a spring, a cave, a grove, the open sky), as its holy place's is built
bool WB::landShrine(const LandCell& L, const LandSettle& S, Rng& rng) {
    const int holy = H->profile.holy;
    Vec3 site; double heading = 0; bool any = false;
    if (holy == HOLY_HIGH_PLACE) { double hTop; any = landSummit(L, 60, rng, site, hTop) && clearOf(site, 60); }
    else {
        std::vector<Vec3> cand; landPoints(L, 60, 7, rng, Rm, cand);
        double bestS = -1e18;
        for (const Vec3& u : cand) {
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, 45, down, h);
            double sc = 0;
            switch (holy) {
                case HOLY_CAVE: if (sl < 0.3) continue; sc = sl + (matFamily(s.material) == FAM_ROCK ? 0.4 : 0); break;
                case HOLY_GROVE: if (s.material != MAT_FOREST && s.veg < 0.55) continue; sc = s.veg; break;
                case HOLY_SPRING: { double lowest = 1e18; for (int m = 0; m < 6; m++) { const double b = m * TAU / 6; lowest = std::min(lowest, at(offsetUnit(u, std::sin(b) * 250, std::cos(b) * 250, Rm)).height); } if (lowest < h - 2) continue; sc = 0.5 - sl; break; }
                default: if (sl > 0.05 || s.material == MAT_FOREST) continue; sc = 0.5 - sl * 5; break;
            }
            if (sc > bestS && clearOf(u, 60)) { bestS = sc; site = u; heading = down; any = true; }
        }
    }
    if (!any) return false;
    Work& w = add(WO_SHRINE, site, 60, landSeed(L, WO_SHRINE));
    w.style = holy; w.heading = holy == HOLY_CAVE ? heading : (S.found ? bearingOf(site, S.u) : rng.uni() * TAU);
    if (holy == HOLY_GROVE) { GroundEdit e; e.kind = GE_FLAT; e.r = 20; e.width = 8; e.level = at(site).height; e.tone = ET_CLEARED; e.fades = false; w.edits.push_back(e); }   // its clearing
    return true;
}

// a lighthouse on a headland (the water round a third of it at 400 m), a little back from its edge
bool WB::landLighthouse(const LandCell& L, Rng& rng) {
    std::vector<Vec3> cand; landPoints(L, 450, 7, rng, Rm, cand);
    Vec3 best; double bestS = -1e18, bestSea = 0; bool any = false;
    for (const Vec3& u : cand) {
        if (wetS(at(u))) continue;
        int wn = 0; double sea = 0;
        for (int m = 0; m < 12; m++) {
            const double c = m * TAU / 12; const SurfaceSample q = at(offsetUnit(u, std::sin(c) * 400, std::cos(c) * 400, Rm));
            if (wetS(q) && q.oldSea <= 0.5) { if (!wn) sea = c; wn++; }
        }
        if (wn < 4) continue;
        double lo = 0, hi = 400;   // the edge toward the water, bisected
        for (int it = 0; it < 7; it++) { const double mid = 0.5 * (lo + hi); if (wetS(at(offsetUnit(u, std::sin(sea) * mid, std::cos(sea) * mid, Rm)))) hi = mid; else lo = mid; }
        const double back = std::max(0.0, lo - 25);
        const Vec3 site = offsetUnit(u, std::sin(sea) * back, std::cos(sea) * back, Rm);
        const double sc = wn + 0.5 * rng.uni();
        if (sc > bestS && landInside(L, site, 50, Rm) && clearOf(site, 50)) { bestS = sc; best = site; bestSea = sea; any = true; }
    }
    if (!any) return false;
    Work& w = add(WO_LIGHTHOUSE, best, 50, landSeed(L, WO_LIGHTHOUSE));
    w.age = clampi(age, AGE_MEDIEVAL, AGE_MODERN); w.heading = bestSea; w.height = w.age == AGE_MEDIEVAL ? 22 + 10 * rng.uni() : 30 + 25 * rng.uni();
    return true;
}

// a fort on the cell's height (a crest it can stand on): a prehistoric people's hill fort of one to three rings of bank, a medieval people's
// castle of five to eight sides, an industrial people's redoubt, a modern people's pillboxes round a blockhouse; its gate toward the settlement
// it guarded, else down the slope
bool WB::landFort(const LandCell& L, const LandSettle& S, Rng& rng) {
    const int style = std::min(age, (int)AGE_MODERN);
    double width = 0, height = 0, reach = 0;
    switch (style) {
        case 0: height = 1 + rng.irange(3); width = 40 + 14 * height + 20 * rng.uni(); reach = width + 16; break;
        case 1: height = 5 + rng.irange(4); width = 24 + 14 * rng.uni(); reach = width + 14; break;
        case 2: width = 26 + 10 * rng.uni(); reach = width + 14; break;
        default: width = 60; reach = 90; break;
    }
    // the cell's heights, the highest first, each walked up to its top (a redoubt's level ground as it is): the first whose footprint the ground
    // falls across within the style's bound (a redoubt's one ring bank on the level, a hill fort's rings on a hilltop's crown, a castle's and
    // a strongpoint's pieces each on its own ground) and clear
    const double maxFall = style == 2 ? 3.0 : (style == 0 ? 0.25 * width : (style == 1 ? 10.0 : 14.0));
    std::vector<Vec3> cand; landPoints(L, reach, 6, rng, Rm, cand);
    std::vector<std::pair<double, Vec3>> high;
    for (const Vec3& u : cand) { const SurfaceSample s = at(u); if (!wetS(s)) high.push_back({s.height, u}); }
    std::sort(high.begin(), high.end(), [](const std::pair<double, Vec3>& x, const std::pair<double, Vec3>& y) { return x.first > y.first; });
    Vec3 top; double sl = 1e9, down = 0; bool any = false;
    for (int k = 0; k < (int)high.size() && k < 8 && !any; k++) {
        top = high[k].second; double hTop = high[k].first;
        if (style != 2) landClimb(L, reach, top, hTop);
        double h; sl = grade(top, std::max(30.0, 0.6 * width), down, h);
        if (sl > 0.3 || fallOver(top, std::max(30.0, width)) > maxFall || !clearOf(top, reach)) continue;
        any = true;
    }
    if (!any) return false;
    Work& w = add(WO_FORT, top, reach, landSeed(L, WO_FORT));
    w.style = style; w.width = width; w.height = height; w.heading = S.found ? bearingOf(top, S.u) : down;
    const bool moat = style == 1 && sl < 0.06;   // a castle on the flat behind its moat
    if (style == 0 || style == 2 || moat) {   // the ditches outside the banks (the walls), a gap at the gate
        const int rings = style == 0 ? (int)height : 1;
        for (int k = 0; k < rings; k++) {
            const double R = width - (style == 0 ? k * 18 : 0) + (moat ? 9 : 6);
            GroundEdit e; e.kind = GE_DITCH; e.width = 6; e.depth = 2.4; e.bank = 0; e.tone = ET_DARK;
            for (int m = 0; m <= 48; m++) { const double t = m * TAU / 48; if (m > 0 && m < 48 && std::fabs(std::remainder(t - w.heading, TAU)) < 0.1) { if (e.pts.size() >= 4) w.edits.push_back(e); e.pts.clear(); continue; } addPt(e, std::sin(t) * R, std::cos(t) * R); }
            if (e.pts.size() >= 4) w.edits.push_back(e);
        }
    }
    return true;
}

// one cell: what its ground offers its people's age (a 5 x 5 reading of it and the plan's coarse grid at its middle: rock, open ground, forest,
// a shore, coal and ore, a ridge; the settlement it would serve; the people's way with its dead and its holy place), a kind drawn among those by
// their weights, sited by its rule (one that finds no site gives way to the next)
bool WB::landWork(const LandCell& L) {
    Rng rr(L.h ^ 0x7E4D1ULL);
    std::vector<Vec3> pts; landPoints(L, 0, 5, rr, Rm, pts);
    int nRock = 0, nOpen = 0, nForest = 0, nWet = 0, nDry = 0, nSnow = 0;
    for (const Vec3& u : pts) {
        const SurfaceSample s = at(u);
        if (wetS(s)) { if (s.oldSea <= 0.5) nWet++; continue; }
        nDry++;
        const int fam = matFamily(s.material);
        if (fam == FAM_ROCK) nRock++;
        if (fam == FAM_SNOW) nSnow++;
        else if (s.material == MAT_FOREST || s.veg > 0.55) nForest++;
        else nOpen++;
    }
    if (nDry < 3) return false;
    const int nLand = nDry - nSnow;
    const int gc = P.grid.cellOf(L.mid);
    const double res = P.grid.res[gc], relief = P.grid.relief[gc];
    const bool coast = nWet >= 2, open = nOpen >= 8;
    const LandSettle S = landSettle(L);
    struct Opt { int kind; double w; };
    std::vector<Opt> opts;
    const bool hill = relief > 0.1, served = S.found && (age == AGE_PREHISTORIC || settlementRank(S.sclass) >= settlementRank(SC_VILLAGE));
    if (age == AGE_PREHISTORIC) {
        if (nLand >= 5) opts.push_back({WO_QUARRY, 0.7});   // flint pits
        if (open) opts.push_back({WO_HENGE, 0.9});
        if (hill) opts.push_back({WO_FORT, 0.5});   // a hill fort
    } else if (age <= AGE_MODERN) {
        const bool stone = C.family == 0 && nRock >= 1, pits = nLand >= 5;   // an adobe people cut no stone: it dug its earth where it built
        if (stone || pits) opts.push_back({WO_QUARRY, stone ? 0.55 + 0.15 * std::min(nRock, 4) : 0.45});
        if (res > 0.3) opts.push_back({WO_MINE, 0.4 + 1.4 * res});
        if (coast) opts.push_back({WO_WRECK, 0.6});
        else if (age == AGE_MODERN && open && relief < 0.3) opts.push_back({WO_WRECK, 0.25});   // an aircraft
        if (coast && served) opts.push_back({WO_LIGHTHOUSE, 0.3});
        if (age == AGE_MODERN && relief > 0.12 && relief < 0.6) opts.push_back({WO_WINDFARM, 0.9});
        opts.push_back({WO_FORT, (age == AGE_MEDIEVAL ? 0.7 : 0.35) * (hill ? 1 : 0.45)});   // a castle (behind its moat on the flat), a redoubt, pillboxes
    } else if (open) opts.push_back({WO_WRECK, 1.0});   // fallen from orbit
    if (C.genome.of(GV_DEAD) != DE_NOTHING && served) opts.push_back({WO_NECROPOLIS, age == AGE_PREHISTORIC ? 0.8 : 0.5});
    bool holy = false;
    switch (H ? H->profile.holy : -1) {
        case HOLY_HIGH_PLACE: holy = relief > 0.12; break;
        case HOLY_SPRING: holy = relief > 0.08; break;   // at the foot of the hills
        case HOLY_CAVE: holy = nRock >= 2 && relief > 0.15; break;
        case HOLY_GROVE: holy = nForest >= 5; break;
        case HOLY_OPEN_SKY: holy = nOpen >= 12 && relief < 0.1; break;
        default: break;
    }
    if (holy) opts.push_back({WO_SHRINE, 0.25});
    Rng rng(L.h ^ 0xB418A11ULL);
    while (!opts.empty()) {
        double sum = 0; for (const Opt& o : opts) sum += o.w;
        double x = rng.uni() * sum; size_t k = 0;
        for (; k + 1 < opts.size(); k++) { if (x < opts[k].w) break; x -= opts[k].w; }
        Rng kr(landSeed(L, opts[k].kind) ^ 0x5173ULL);
        bool ok = false;
        switch (opts[k].kind) {
            case WO_QUARRY: ok = landQuarry(L, age >= AGE_MEDIEVAL, kr); break;
            case WO_MINE: ok = landMine(L, kr); break;
            case WO_NECROPOLIS: ok = landNecropolis(L, S, kr); break;
            case WO_HENGE: ok = landHenge(L, kr); break;
            case WO_WRECK: ok = landWreck(L, coast, kr); break;
            case WO_WINDFARM: ok = landWindfarm(L, kr); break;
            case WO_SHRINE: ok = landShrine(L, S, kr); break;
            case WO_LIGHTHOUSE: ok = landLighthouse(L, kr); break;
            case WO_FORT: ok = landFort(L, S, kr); break;
            default: break;
        }
        if (ok) return true;
        opts.erase(opts.begin() + k);
    }
    return false;
}

}   // namespace

// ---- the world's works ---------------------------------------------------------------------------------------------------------------
std::shared_ptr<WorldWorks> buildWorldWorks(const BodyGen& gSeason) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    auto W = std::make_shared<WorldWorks>();
    W->seed = g.seed; W->Rm = g.R * 1000.0;
    if (!worldHadCivilisation(g)) return W;
    const double t0 = nowMs();
    const std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    W->peoples = P->peoples;
    DrainageOff off;
    WB b(g, *P, W->works);
    for (int p = 0; p < P->peoples; p++) {
        b.p = p; b.pp = &P->plans[p]; b.age = b.pp->age; b.H = historyOfWorld(g, p); b.C = cultureOf(g, p); b.pseed = peopleGen(g, p).seed ^ 0x7030ULL;
        b.ageFrom = b.H ? ageFromOf(b.H->profile) : 1;
        std::fill(b.made, b.made + WO_COUNT, 0);
        if (b.pp->places.empty()) continue;
        // the history's first (its battles, the last shelter, the works it finished where it finished them), then the world's great ones by
        // the terrain (B-418: the everyday works are the land's, `landWorkOf`)
        b.battlefields(); b.shelter(); b.sunlostWorks(); b.dams(); b.aqueducts(); b.observatories(); b.lighthouses(); b.launchSites(); b.shrine();
        b.colossi(); b.mines(); b.fortLine(); b.antennas(); b.terraforms(); b.strange(); b.geoglyphs();
        for (int k = 0; k < WO_COUNT; k++) W->counts[p][k] = b.made[k];
    }
    W->buildMs = nowMs() - t0;
    return W;
}

namespace {
using WorksFuture = std::shared_future<std::shared_ptr<const WorldWorks>>;
struct WorksSlot { WorksFuture fut; uint64_t used = 0; };
std::mutex g_worksMutex;
std::map<uint64_t, WorksSlot> g_works;
uint64_t g_worksClock = 0;
bool worksReady(const WorksFuture& f) { return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
}

std::shared_ptr<const WorldWorks> worldWorksOf(const BodyGen& g) {
    std::shared_ptr<std::promise<std::shared_ptr<const WorldWorks>>> pr;
    WorksFuture fut;
    {
        std::lock_guard<std::mutex> lock(g_worksMutex);
        auto it = g_works.find(g.seed);
        if (it != g_works.end()) { it->second.used = ++g_worksClock; fut = it->second.fut; }
        else {
            while (g_works.size() >= 4) {   // the least used of those built
                auto old = g_works.end();
                for (auto j = g_works.begin(); j != g_works.end(); ++j) if (worksReady(j->second.fut) && (old == g_works.end() || j->second.used < old->second.used)) old = j;
                if (old == g_works.end()) break;
                g_works.erase(old);
            }
            pr = std::make_shared<std::promise<std::shared_ptr<const WorldWorks>>>();
            fut = pr->get_future().share();
            g_works[g.seed] = WorksSlot{fut, ++g_worksClock};
        }
    }
    if (pr) pr->set_value(buildWorldWorks(g));
    return fut.get();
}

std::shared_ptr<const WorldWorks> worldWorksIfReady(const BodyGen& g) {
    std::lock_guard<std::mutex> lock(g_worksMutex);
    auto it = g_works.find(g.seed);
    if (it == g_works.end() || !worksReady(it->second.fut)) return nullptr;
    return it->second.fut.get();
}

// ---- B-418: the land's works, kept ---------------------------------------------------------------------------------------------------
int landCellsAround(const BodyGen& g, int gLat) {
    const double dLat = landCellLat(g);
    return std::max(1, (int)std::floor(TAU * std::cos(clampd((gLat + 0.5) * dLat, -PI / 2, PI / 2)) / dLat));
}

void landCellOf(const BodyGen& g, double lat, double lon, int& gLat, int& gLon) {
    gLat = (int)std::floor(lat / landCellLat(g));
    const int n = landCellsAround(g, gLat);
    double l = std::fmod(lon, TAU); if (l < 0) l += TAU;
    gLon = std::min(n - 1, (int)std::floor(l / (TAU / n)));
}

namespace {
// what every cell of a world's land reads: its plan, its works, its peoples' histories and cultures
struct LandCtx {
    BodyGen g; std::shared_ptr<const WorldPlan> P; std::shared_ptr<const WorldWorks> W;
    std::shared_ptr<const History> H[2]; Culture C[2]; uint64_t pseed[2] = {0, 0}; int ageFrom[2] = {1, 1};
};
std::shared_ptr<LandCtx> makeLandCtx(const BodyGen& g, std::shared_ptr<const WorldWorks> W) {
    auto X = std::make_shared<LandCtx>();
    { BodyGen gTmp; X->g = thenWorld(g, gTmp); }   // B-419: the world's, not its season's (P-04: in its people's time)
    X->P = worldPlanOf(g); X->W = std::move(W);
    for (int p = 0; p < std::min(2, X->P->peoples); p++) {
        X->H[p] = historyOfWorld(g, p); X->C[p] = cultureOf(g, p); X->pseed[p] = peopleGen(g, p).seed ^ 0x7030ULL;
        X->ageFrom[p] = X->H[p] ? ageFromOf(X->H[p]->profile) : 1;
    }
    return X;
}
// a cell's work: none where the draw by the countryside's density and the age's appetite says so, none where the cell's ground offers nothing
std::shared_ptr<const Work> landWorkIn(const LandCtx& X, int gLat, int gLon) {
    const BodyGen& g = X.g; const WorldPlan& P = *X.P;
    const double dLat = landCellLat(g);
    if (P.peoples == 0 || std::fabs((gLat + 0.5) * dLat) > PI / 2 - dLat) return nullptr;
    const LandCell L = landCellAt(g, gLat, gLon);
    const int p = clampi(peopleAt(g, L.latMid, L.lonMid), 0, P.peoples - 1);
    const PeoplePlan& pp = P.plans[p];
    if (pp.places.empty() || unitFromHash(L.h) >= LAND_RATE[clampi(pp.age, 0, AGE_COUNT - 1)] * planCountryside(P, L.latMid, L.lonMid, p)) return nullptr;
    std::vector<Work> out;
    DrainageOff off;
    WB b(g, P, out); b.world = &X.W->works;
    b.p = p; b.pp = &pp; b.age = pp.age; b.H = X.H[p]; b.C = X.C[p]; b.pseed = X.pseed[p]; b.ageFrom = X.ageFrom[p];
    if (!b.landWork(L) || out.empty()) return nullptr;
    out.back().land = true;
    return std::make_shared<const Work>(std::move(out.back()));
}
// the last two worlds' cells (a world's slot: its reading and the cells asked for; cleared when it holds 40,000)
struct LandSlot { uint64_t seed = 0, used = 0; std::shared_ptr<LandCtx> ctx; std::unordered_map<uint64_t, std::shared_ptr<const Work>> cells; };
std::mutex g_landMutex;
LandSlot g_land[2];
uint64_t g_landClock = 0;
inline uint64_t landKey(int gLat, int gl) { return ((uint64_t)(uint32_t)(gLat + 0x100000) << 32) | (uint32_t)gl; }
}

std::shared_ptr<const Work> landWorkOf(const BodyGen& g, int gLat, int gLon) {
    if (!worldHadCivilisation(g)) return nullptr;
    const int n = landCellsAround(g, gLat), gl = ((gLon % n) + n) % n;
    const uint64_t key = landKey(gLat, gl);
    std::shared_ptr<LandCtx> ctx;
    {
        std::lock_guard<std::mutex> lock(g_landMutex);
        for (LandSlot& s : g_land) if (s.ctx && s.seed == g.seed) {
            s.used = ++g_landClock;
            auto it = s.cells.find(key);
            if (it != s.cells.end()) return it->second;
            ctx = s.ctx;
        }
    }
    if (!ctx) {
        std::shared_ptr<LandCtx> X = makeLandCtx(g, worldWorksOf(g));   // the world's works waited for: the land's keep clear of them
        std::lock_guard<std::mutex> lock(g_landMutex);
        LandSlot* s = nullptr;
        for (LandSlot& q : g_land) if (q.ctx && q.seed == g.seed) s = &q;
        if (!s) {
            s = &g_land[0];
            for (LandSlot& q : g_land) if (!q.ctx || q.used < s->used) s = &q;
            *s = LandSlot(); s->seed = g.seed; s->ctx = X;
        }
        s->used = ++g_landClock; ctx = s->ctx;
    }
    std::shared_ptr<const Work> w = landWorkIn(*ctx, gLat, gl);
    std::lock_guard<std::mutex> lock(g_landMutex);
    for (LandSlot& s : g_land) if (s.ctx && s.seed == g.seed) {
        if (s.cells.size() >= 40000) s.cells.clear();
        return s.cells.emplace(key, w).first->second;
    }
    return w;   // its world's slot was taken meanwhile
}

std::shared_ptr<const Work> buildLandWork(const BodyGen& g, int gLat, int gLon) {
    if (!worldHadCivilisation(g)) return nullptr;
    const int n = landCellsAround(g, gLat);
    return landWorkIn(*makeLandCtx(g, worldWorksOf(g)), gLat, ((gLon % n) + n) % n);
}

void worldWorksClearCache() {
    {
        std::lock_guard<std::mutex> lock(g_worksMutex);
        for (auto it = g_works.begin(); it != g_works.end();) { if (worksReady(it->second.fut)) it = g_works.erase(it); else ++it; }
    }
    std::lock_guard<std::mutex> lock(g_landMutex);
    for (LandSlot& s : g_land) s = LandSlot();
}

namespace {
// the world's works whose reach comes within `radiusM`, then the land's cells within `landR` and the farthest a land's work reaches
void worksAroundOf(const BodyGen& g, const std::shared_ptr<const WorldWorks>& W, const Vec3& unit, double radiusM, double landR, std::vector<std::shared_ptr<const Work>>& out) {
    out.clear();
    for (const Work& w : W->works) if (arcM(unit, w.unit, W->Rm) < radiusM + w.radiusM) out.push_back(std::shared_ptr<const Work>(W, &w));
    radiusM = landR;
    const double Rm = g.R * 1000.0, dLat = landCellLat(g), reach = radiusM + LAND_REACH_MAX;
    double lat, lon; StarSystem::latLonFromBody(unit, lat, lon);
    const int r0 = (int)std::floor((lat - reach / Rm) / dLat), r1 = (int)std::floor((lat + reach / Rm) / dLat);
    double lon01 = std::fmod(lon, TAU); if (lon01 < 0) lon01 += TAU;
    for (int row = r0; row <= r1; row++) {
        const double latc = (row + 0.5) * dLat;
        if (std::fabs(latc) > PI / 2 - dLat) continue;
        const int n = landCellsAround(g, row);
        const double w = TAU / n, edge = std::min(PI / 2, std::fabs(latc) + 0.5 * dLat);
        const double lonR = reach / (Rm * std::max(std::cos(edge), 1e-3));
        int c0 = (int)std::floor((lon01 - lonR) / w), c1 = (int)std::floor((lon01 + lonR) / w);
        if (c1 - c0 + 1 >= n) { c0 = 0; c1 = n - 1; }
        for (int c = c0; c <= c1; c++) {
            const int gl = ((c % n) + n) % n;
            const Vec3 mid = StarSystem::bodyFromLatLon(latc, (gl + 0.5) * w);
            if (arcM(unit, mid, Rm) > reach + 0.75 * (w * Rm * std::cos(latc) + dLat * Rm)) continue;   // beyond the cell's corners
            std::shared_ptr<const Work> lw = landWorkOf(g, row, gl);
            if (lw && arcM(unit, lw->unit, Rm) < radiusM + lw->radiusM) out.push_back(std::move(lw));
        }
    }
}
}

void worksAround(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<std::shared_ptr<const Work>>& out) {
    out.clear();
    if (!worldHadCivilisation(g)) return;
    worksAroundOf(g, worldWorksOf(g), unit, radiusM, radiusM, out);
}

bool worksAroundIfReady(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<std::shared_ptr<const Work>>& out, double landRadiusM) {
    out.clear();
    if (!worldHadCivilisation(g)) return true;
    const std::shared_ptr<const WorldWorks> W = worldWorksIfReady(g);
    if (!W) return false;
    worksAroundOf(g, W, unit, radiusM, landRadiusM < 0 ? radiusM : landRadiusM, out);
    return true;
}

// ---- a work's pieces --------------------------------------------------------------------------------------------------------------
namespace {
struct Lay {
    RuinSpec& r; const Work& w; Rng rng; int n = 0;
    Lay(RuinSpec& rr, const Work& ww) : r(rr), w(ww), rng(ww.seed ^ 0x5BEC0ULL) {}
    // B-421: a building's doorway in its front (its heading's side): a keep, a house, a hall, a tower of the works stood with none, and the
    // records the rules laid in their rooms were shut in (C-01's settlements draw their own)
    Building& add(int kind, double x, double z, double heading, double hw, double hd, double h, int style = 0, int age = -2) {
        Building b; b.kind = kind; b.x = x; b.z = z; b.heading = heading; b.hw = hw; b.hd = hd; b.hWall = h; b.thick = 0.5; b.door = 0;
        b.age = age == -2 ? w.age : age; b.style = style; b.seed = mix64(w.seed ^ (0xB01D0000ULL + (uint64_t)(n++) * 0x9E3779B97F4A7C15ULL));
        b.luck = (unitFromHash(mix64(b.seed ^ 0x1CCULL)) - 0.5) * 0.5; b.before = w.before;
        r.buildings.push_back(b);
        return r.buildings.back();
    }
    // the work's metres of a point (across, along) in a frame of heading `h` about (cx, cz)
    void at(double h, double cx, double cz, double a, double b, double& x, double& z) const { double dx, dz; rot(h, a, b, dx, dz); x = cx + dx; z = cz + dz; }
    void plot(double x, double z, double heading, double hw, double hd) { FieldPatch f; f.x = x; f.z = z; f.heading = heading; f.hw = hw; f.hd = hd; f.kind = FI_PLOT; f.age = w.age; r.fields.push_back(f); }
};
}

void workSpec(const BodyGen& gSeason, const Work& w, RuinSpec& r) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    r = RuinSpec();
    r.sclass = SC_WORKS; r.plan = SP_LONE; r.lat = w.lat; r.lon = w.lon; r.heading = w.heading; r.size = w.radiusM;
    r.seed = w.seed; r.people = w.people; r.work = w.kind; r.buildAge = w.age; r.before = w.before;
    r.site.desert = kindHas(g.type, KT_DRY);
    { const int now = nowKind(g); r.site.drift = kindHas(now, KT_DRY) ? 1 : (KINDS[now].cover == DC_FROZEN ? 2 : 0); }   // P-04: the present's drifts
    r.site.wind = g.windAngle + (w.lat < 0 ? PI : 0) + (int)std::floor(std::fabs(w.lat) / (30 * DEG)) * 0.9;
    const double Rm = g.R * 1000.0;
    const Frame F(w.unit, Rm);
    DrainageOff off;
    auto ground = [&](double x, double z) { return sampleSurface(g, F.unit(x, z), 64.0).height; };
    auto wet = [&](double x, double z) { return wetS(sampleSurface(g, F.unit(x, z), 64.0)); };
    Lay L(r, w);
    Rng& rng = L.rng;
    const double H = w.heading;
    switch (w.kind) {
        case WO_QUARRY: {
            if (w.style == 0) {   // the flint pits' spoil, a ring round each
                for (const GroundEdit& e : w.edits) L.add(BK_MOUND, e.x, e.z, 0, e.r + 2.4, 1.5, 0.9, 4);
                break;
            }
            if (w.style >= 4) {   // a pit: a modern open pit's mill at its rim, its spoil and the trucks on its floor; a medieval stone pit's blocks
                                  // on its floor and its derrick at the rim; an industrial pit's, its shed and its heap
                const GroundEdit& e = w.edits[0];
                double x, z;
                const double r0 = std::max(6.0, e.hw);
                if (w.style == 4) {
                    L.at(H, 0, 0, 0, -(e.r + 60), x, z); L.add(BK_HANGAR, x, z, H, 16, 26, 18, 1);
                    L.at(H, 0, 0, e.r + 110, 30, x, z); L.add(BK_MOUND, x, z, H, 70, 45, 11, 5);
                    for (int k = 0; k < 3; k++) { const double a = rng.uni() * TAU, d = r0 * 0.6 * rng.uni(); L.add(BK_VEHICLE, std::sin(a) * d, std::cos(a) * d, rng.uni() * TAU, 3.4, 5.6, 5.5, 1, AGE_MODERN); }
                } else {
                    const bool iron = w.style == 6;
                    for (int k = 0; k < (iron ? 3 : 2); k++) { const double a = rng.uni() * TAU, d = r0 * 0.5 * rng.uni(); L.add(BK_BLOCKS, std::sin(a) * d, std::cos(a) * d, rng.uni() * PI, 4.5, 3.2, 1.5); }
                    L.at(H, 0, 0, 0, -(e.r + 3), x, z); L.add(BK_DERRICK, x, z, H + PI, 1, 1, iron ? 16 : 9, iron ? 1 : 0);
                    L.at(H, 0, 0, e.r + 10, -(e.r * 0.4), x, z);
                    if (iron) { L.add(BK_HALL, x, z, H + PI, 6, 10, 5.5, 0, AGE_INDUSTRIAL); L.at(H, 0, 0, -(e.r + 22), 0, x, z); L.add(BK_MOUND, x, z, 0, 15, 15, 10, 2); }
                    else L.add(BK_HOUSE, x, z, H + PI, 2.6, 3.2, 2.8, 0, AGE_MEDIEVAL);
                }
                break;
            }
            const GroundEdit& e = w.edits[0];
            const double step = e.step > 0 ? e.step : 2 * e.hd;
            double x, z;
            const int nb = w.style == 1 ? 2 : 3;
            for (int k = 0; k < nb; k++) { L.at(H, 0, 0, (k - (nb - 1) * 0.5) * e.hw * 0.55, -e.hd + step * 0.35, x, z); L.add(BK_BLOCKS, x, z, H + rng.sym(0.3), 4.5, 3.2, 1.5); }
            L.at(H, 0, 0, e.hw * 0.2, -e.hd + step * 0.7, x, z); L.add(BK_DERRICK, x, z, H + rng.sym(0.6), 1, 1, w.style == 1 ? 9 : 16, w.style == 1 ? 0 : 1);
            if (w.style == 1) { L.at(H, 0, 0, e.hw + 9, -e.hd - 9, x, z); L.add(BK_HOUSE, x, z, H + PI, 2.6, 3.2, 2.8, 0, AGE_MEDIEVAL); }
            else if (w.style == 2) {
                L.at(H, 0, 0, e.hw + 14, -e.hd - 16, x, z); L.add(BK_HALL, x, z, H + PI, 6, 10, 5.5, 0, AGE_INDUSTRIAL);
                L.at(H, 0, 0, -e.hw - 22, -e.hd - 14, x, z); L.add(BK_MOUND, x, z, 0, 15, 15, 10, 2);
            } else {
                L.at(H, 0, 0, e.hw + 36, -e.hd - 34, x, z); L.add(BK_HANGAR, x, z, H + PI, 14, 22, 16, 1);
                L.at(H, 0, 0, -e.hw - 60, -e.hd - 25, x, z); L.add(BK_MOUND, x, z, H, 45, 32, 10, 5);
                for (int k = 0; k < 2; k++) { L.at(H, 0, 0, rng.sym(e.hw * 0.6), -e.hd + step * (0.3 + 0.4 * rng.uni()), x, z); L.add(BK_VEHICLE, x, z, rng.uni() * TAU, 3.4, 5.6, 5.5, 1, AGE_MODERN); }
            }
            break;
        }
        case WO_MINE: {
            double x, z;
            if (w.style == 0) {   // adits along the hillside, the spoil tipped below each, a hut
                const int n = 2 + rng.irange(3);
                for (int k = 0; k < n; k++) {
                    const double a = (k - (n - 1) * 0.5) * (28 + 14 * rng.uni());
                    L.at(H, 0, 0, a, 0, x, z); L.add(BK_PORTAL, x, z, H + PI, 1.6, 1, 3.2);
                    L.at(H, 0, 0, a + rng.sym(4), -14, x, z); L.add(BK_MOUND, x, z, H, 5 + 3 * rng.uni(), 8 + 4 * rng.uni(), 1.6 + 1.6 * rng.uni(), 6);
                }
                L.at(H, 0, 0, 0, -40, x, z); L.add(BK_HOUSE, x, z, H + PI, 2.8, 3.5, 3, 0, AGE_MEDIEVAL);
            } else if (w.style == 1) {   // headframes over their shafts, the great spoil heap, the miners' houses
                const int n = 1 + rng.irange(3);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 75, 0, x, z); L.add(BK_HEADFRAME, x, z, H + rng.sym(0.3), 13, 16, 24 + 8 * rng.uni()); }
                L.at(H, 0, 0, 110 + 40 * rng.uni(), -60, x, z); L.add(BK_MOUND, x, z, 0, 28 + 18 * rng.uni(), 28, 20 + 12 * rng.uni(), 2);
                for (int k = 0; k < 4; k++) { L.at(H, 0, 0, -90 + k * 14, -95, x, z); L.add(BK_HOUSE, x, z, H + PI, 3, 4, 5, 0, AGE_INDUSTRIAL); }
            } else {   // the open pit, a headframe and the mill at its rim, the tailings, trucks on its floor
                const GroundEdit& e = w.edits[0];
                L.at(H, 0, 0, -e.r * 0.4, -(e.r + 50), x, z); L.add(BK_HEADFRAME, x, z, H, 13, 16, 34);
                L.at(H, 0, 0, e.r * 0.35, -(e.r + 70), x, z); L.add(BK_HANGAR, x, z, H + PI, 18, 30, 20, 1);
                L.at(H, 0, 0, e.r + 160, 0, x, z); L.add(BK_MOUND, x, z, H, 110, 80, 12, 5);
                for (int k = 0; k < 3; k++) { const double a = rng.uni() * TAU, d = e.r * 0.25 * rng.uni(); L.add(BK_VEHICLE, std::sin(a) * d, std::cos(a) * d, rng.uni() * TAU, 3.4, 5.6, 5.5, 1, AGE_MODERN); }
            }
            break;
        }
        case WO_DAM: {
            const double foot = w.style == 2 ? 0.13 * w.height : (w.style == 0 ? 0.38 * w.height : (w.style == 3 ? 0.3 * w.height : 0.42 * w.height));
            Building& d = L.add(BK_DAM, 0, 0, H, w.length * 0.5 + 6, std::max(4.0, foot), w.crest - (w.floor - 6), w.style);
            d.base = w.floor - 6;
            if (w.style >= 1) {   // the power house at its foot downstream
                double x, z; L.at(H, 0, 0, 0, -(foot + 28), x, z);
                L.add(BK_HALL, x, z, H + PI, 12, 18, 12, 0);
            }
            break;
        }
        case WO_AQUEDUCT: {
            double sx, sz, ex, ez; F.local(w.sideA, sx, sz); F.local(w.sideB, ex, ez);
            const double len = std::hypot(ex - sx, ez - sz), hd = std::atan2(ex - sx, ez - sz);
            const int n = std::max(2, (int)(len / 40));
            std::vector<double> gr(n + 1);
            for (int k = 0; k <= n; k++) { const double t = (double)k / n; gr[k] = ground(sx + (ex - sx) * t, sz + (ez - sz) * t); }
            auto chan = [&](double t) { return w.level + (w.crest - w.level) * t; };
            int k = 0;
            while (k < n) {   // runs of the line where the channel stands over the ground: arcades of up to sixty metres
                if (chan((k + 0.5) / n) - std::min(gr[k], gr[k + 1]) < 3.5) { k++; continue; }
                int k1 = k + 1;
                while (k1 < n && k1 - k < 2 && chan((k1 + 0.5) / n) - std::min(gr[k1], gr[k1 + 1]) >= 3.5) k1++;
                const double t0 = (double)k / n, t1 = (double)k1 / n, tm = 0.5 * (t0 + t1);
                double lo = 1e18; for (int q = k; q <= k1; q++) lo = std::min(lo, gr[q]);
                const double top = chan(tm);
                Building& b = L.add(BK_AQUEDUCT, sx + (ex - sx) * tm, sz + (ez - sz) * tm, hd, 1.3, 0.5 * len * (t1 - t0), top - (lo - 1.5), 0);
                b.base = lo - 1.5;
                k = k1;
            }
            L.add(BK_CISTERN, sx, sz, hd + PI, 3.5, 3.5, 3, 0);   // the spring's house
            L.add(BK_CISTERN, ex, ez, hd, 4.5, 4.5, 4, 0);        // the castellum at the town's edge
            break;
        }
        case WO_OBSERVATORY: {
            double x, z;
            switch (w.style) {
                case 0:
                    L.add(BK_PLATFORM, 0, 0, H, 9, 9, 3);
                    L.at(H, 0, 0, 0, -18, x, z); L.add(BK_TOWER, x, z, H, 3.6, 3.6, 13);
                    for (int k = 0; k < 8; k++) { const double a = k * TAU / 8; L.add(BK_STELA, std::sin(a) * 26, std::cos(a) * 26, a + PI, 1, 1, 2.6, MN_STELAE); }
                    break;
                case 1: {
                    const int n = 1 + rng.irange(3);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 22, 0, x, z); L.add(BK_OBSERVATORY, x, z, H, 4 + 3 * rng.uni(), 4, 7 + 3 * rng.uni()); }
                    L.at(H, 0, 0, 0, -30, x, z); L.add(BK_VILLA, x, z, H, 6, 8, 7, 0, AGE_INDUSTRIAL);
                    break;
                }
                case 2: {
                    const int n = 1 + rng.irange(4);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 80, 40 + 30 * (k % 2), x, z); L.add(BK_DISH, x, z, H + rng.sym(1), 8 + 22 * rng.uni(), 1, 1); }
                    L.at(H, 0, 0, -30, -25, x, z); L.add(BK_OBSERVATORY, x, z, H, 9, 9, 13, 0, AGE_MODERN);
                    L.at(H, 0, 0, 35, -30, x, z); L.add(BK_HALL, x, z, H, 8, 14, 8, 0, AGE_MODERN);
                    break;
                }
                default: {
                    const int n = 8 + rng.irange(5);
                    for (int k = 0; k < n; k++) { const double a = k * TAU / n; L.add(BK_DISH, std::sin(a) * 85, std::cos(a) * 85, a, 7 + 4 * rng.uni(), 1, 1, 1); }
                    L.add(BK_POD, 0, 0, H, 14, 14, 9);
                    break;
                }
            }
            break;
        }
        case WO_SHRINE: {
            const int a = w.age; double x, z;
            switch (w.style) {
                case HOLY_CAVE:
                    L.add(BK_PORTAL, 0, 0, H, 3.2, 1, 4.2);
                    for (int s = -1; s <= 1; s += 2) { L.at(H, 0, 0, s * 6, 3, x, z); L.add(BK_STELA, x, z, H, 1, 1, 2.4, a == AGE_PREHISTORIC ? MN_STELAE : MN_OBELISKS); }
                    break;
                case HOLY_SPRING:
                    L.add(BK_CISTERN, 0, 0, H, 4, 4, 1.8);
                    L.at(H, 0, 0, 0, -17, x, z);   // B-421: its doorway facing the spring, a way between (at 12 m the cistern stood a metre before it)
                    if (a == AGE_PREHISTORIC) L.add(BK_STONE_CIRCLE, x, z, H, 6, 6, 1.8);
                    else L.add(BK_TEMPLE, x, z, H, 5, 7, 6);
                    break;
                case HOLY_GROVE:
                    for (int k = 0; k < 8; k++) { const double t = k * TAU / 8; L.add(BK_STELA, std::sin(t) * 13, std::cos(t) * 13, t + PI, 1, 1, 2.2, MN_STELAE); }
                    L.add(BK_PLATFORM, 0, 0, H, 2.2, 2.2, 1.1);
                    break;
                default:   // a summit or the open sky
                    if (a == AGE_PREHISTORIC) { L.add(BK_STONE_CIRCLE, 0, 0, H, w.style == HOLY_OPEN_SKY ? 15 : 8, 8, 2.6); L.at(H, 0, 0, 0, -14, x, z); L.add(BK_CAIRN, x, z, 0, 1.6, 1.6, 2); }
                    else if (a == AGE_MEDIEVAL) { L.add(BK_TEMPLE, 0, 0, H, 6, 9, 7); for (int k = 0; k < 3; k++) { L.at(H, 0, 0, (k - 1) * 7, 16, x, z); L.add(BK_STELA, x, z, H + PI, 1, 1, 2.4, MN_STELAE); } }
                    else if (a == AGE_INDUSTRIAL) { L.add(BK_CHURCH, 0, 0, H, 5, 10, 8); L.at(H, 0, 0, 0, 18, x, z); L.add(BK_STELA, x, z, H, 1, 1, 6, MN_OBELISKS); }
                    else { L.add(BK_PLATFORM, 0, 0, H, 10, 10, 1.6); L.add(BK_STELA, 0, 0, H, 1, 1, 12, MN_OBELISKS); }
                    break;
            }
            break;
        }
        case WO_WAY: {   // a cairn or a stela every 600 m along the track, to one side and then the other
            double run = 0, next = 300; int side = 1;
            for (const GroundEdit& e : w.edits) {
                for (size_t q = 2; q + 1 < e.pts.size(); q += 2) {
                    const double x0 = e.pts[q - 2], z0 = e.pts[q - 1], x1 = e.pts[q], z1 = e.pts[q + 1], seg = std::hypot(x1 - x0, z1 - z0);
                    while (run + seg >= next) {
                        const double t = (next - run) / std::max(seg, 1e-6), x = x0 + (x1 - x0) * t, z = z0 + (z1 - z0) * t, hd = std::atan2(x1 - x0, z1 - z0);
                        const double ox = std::cos(hd) * 4 * side, oz = -std::sin(hd) * 4 * side;
                        if (w.age == AGE_PREHISTORIC) L.add(BK_CAIRN, x + ox, z + oz, 0, 1.0, 1.0, 1.4); else L.add(BK_STELA, x + ox, z + oz, hd + PI / 2 * side, 1, 1, 1.8, MN_STELAE);
                        next += 600; side = -side;
                    }
                    run += seg;
                }
            }
            break;
        }
        case WO_NECROPOLIS: {
            double x, z;
            switch (w.style) {
                case 0: {   // barrows along the slope, a long barrow, standing stones
                    const int n = 6 + rng.irange(10);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 22 + rng.sym(6), rng.sym(20), x, z); const double rr = 4 + 8 * rng.uni(); L.add(BK_MOUND, x, z, 0, rr, rr, 1.4 + rr * 0.25, 0); }
                    L.at(H, 0, 0, rng.sym(40), 50, x, z); L.add(BK_MOUND, x, z, H + PI / 2 + rng.sym(0.3), 6 + 3 * rng.uni(), 20 + 12 * rng.uni(), 2.5, 1);
                    for (int k = 0; k < 1 + rng.irange(3); k++) {   // standing stones; B-414: a later people's own stelae
                        L.at(H, 0, 0, rng.sym(80), -30 + rng.sym(10), x, z);
                        const double turn = rng.uni() * TAU;
                        if (w.age == AGE_PREHISTORIC) L.add(BK_MENHIR, x, z, turn, 0.8, 0, 3.6, 0); else L.add(BK_STELA, x, z, turn, 1, 1, 3.6, MN_STELAE);
                    }
                    break;
                }
                case 1: {   // tombs cut into the slope above, the houses of the dead below, the graves between
                    const int nt = 3 + rng.irange(6);
                    for (int k = 0; k < nt; k++) { L.at(H, 0, 0, (k - (nt - 1) * 0.5) * 16, 45 + rng.sym(5), x, z); L.add(BK_TOMB, x, z, H, 2.4 + 1.4 * rng.uni(), 1, 4 + 3 * rng.uni(), 2); }
                    const int nm = 2 + rng.irange(5);
                    for (int k = 0; k < nm; k++) { L.at(H, 0, 0, (k - (nm - 1) * 0.5) * 18, -10 + rng.sym(4), x, z); L.add(BK_TOMB, x, z, H, 2.2 + 0.8 * rng.uni(), 3 + rng.uni(), 3.4 + 1.6 * rng.uni(), rng.irange(2)); }
                    const int np = 2 + rng.irange(3);
                    for (int k = 0; k < np; k++) { L.at(H, 0, 0, (k - (np - 1) * 0.5) * 32, -45, x, z); L.plot(x, z, H, 12, 14); }
                    break;
                }
                case 2: {   // a walled garden of graves: its gate toward the city, the tombs along its middle walk, obelisks, a chapel at its head
                    const double hw = 70 + 40 * rng.uni(), hd = 50 + 30 * rng.uni();
                    const int perSide = std::max(3, (int)(hw / 20));
                    for (int s = 0; s < 4; s++) {
                        const bool across = s % 2 == 0;
                        const double half = across ? hw : hd, off = across ? hd : hw;
                        const int m = across ? perSide : std::max(2, (int)(hd / 20));
                        for (int q = 0; q < m; q++) {
                            const double t = -half + (q + 0.5) * 2 * half / m;
                            if (s == 0 && std::fabs(t) < 8) continue;   // the gate's gap
                            const double lx = across ? t : (s == 1 ? off : -off), lz = across ? (s == 0 ? -off : off) : t;
                            L.at(H, 0, 0, lx, lz, x, z);
                            L.add(BK_WALL, x, z, across ? H : H + PI / 2, half / m + 0.4, 0.35, 2.4);
                        }
                    }
                    L.at(H, 0, 0, 0, -hd, x, z); L.add(BK_GATE, x, z, H, 4, 1.2, 5);
                    const int nm = 6 + rng.irange(9);
                    for (int k = 0; k < nm; k++) { const int side = k % 2 ? 1 : -1; L.at(H, 0, 0, side * 10, -hd + 18 + (k / 2) * (2 * hd - 36) / std::max(1, nm / 2), x, z); L.add(BK_TOMB, x, z, H + (side > 0 ? -PI / 2 : PI / 2), 2.2, 2.8, 3.6 + 1.5 * rng.uni(), rng.irange(2)); }
                    for (int k = 0; k < 2 + rng.irange(4); k++) { L.at(H, 0, 0, rng.sym(hw * 0.7), rng.sym(hd * 0.6), x, z); L.add(BK_STELA, x, z, H, 1, 1, 3 + 3 * rng.uni(), MN_OBELISKS); }
                    L.at(H, 0, 0, 0, hd - 16, x, z); L.add(BK_CHURCH, x, z, H + PI, 5, 9, 8);
                    for (int s = -1; s <= 1; s += 2) for (int q = 0; q < 2; q++) { L.at(H, 0, 0, s * (hw * 0.55), -hd * 0.45 + q * hd * 0.8, x, z); L.plot(x, z, H, hw * 0.35, hd * 0.32); }
                    break;
                }
                case 3: {   // the modern cemetery: a grid of plots, a columbarium, a memorial
                    for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) { L.at(H, 0, 0, (i - 1.5) * 36, (j - 1.5) * 26, x, z); L.plot(x, z, H, 15, 10); }
                    L.at(H, 0, 0, 0, 72, x, z); L.add(BK_TOMB, x, z, H + PI, 34, 5, 4, 3);
                    L.at(H, 0, 0, 0, -68, x, z); L.add(BK_STELA, x, z, H, 1, 1, 12, MN_OBELISKS);
                    break;
                }
                case 4: {   // a futuristic people's field of memorial pillars
                    for (int j = 0; j < 6; j++) for (int i = 0; i < 6; i++) { L.at(H, 0, 0, (i - 2.5) * 14, (j - 2.5) * 14, x, z); L.add(BK_STELA, x, z, H, 1, 1, 2.2 + rng.uni(), MN_PILLARS); }
                    break;
                }
                default: {   // towers of silence on the slope
                    const int n = 1 + rng.irange(3);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, (k - (n - 1) * 0.5) * 45, rng.sym(12), x, z); L.add(BK_TOMB, x, z, H, 8 + 3 * rng.uni(), 8, 7 + 4 * rng.uni(), 4); }
                    break;
                }
            }
            break;
        }
        case WO_GEOGLYPH: {   // cairns at the rays' centre
            for (int k = 0; k < 2; k++) L.add(BK_CAIRN, w.sideA.x + (k ? 6 : -4), w.sideA.z + (k ? -3 : 5), 0, 1.4, 1.4, 1.6, 0);
            break;
        }
        case WO_HENGE: {
            double x, z;
            switch (w.style) {
                case 0: {   // the circle within its ditch and bank, its avenue out of the entrance, barrows round it
                    const double R = w.width > 0 ? w.width : 50;
                    L.add(BK_MOUND, 0, 0, 0, R + 9, 3.6, 1.6, 4);
                    L.add(BK_STONE_CIRCLE, 0, 0, H, R * 0.5, R * 0.5, 3.4);
                    L.at(H, 0, 0, 0, R + 75, x, z); L.add(BK_MENHIR, x, z, H, 1, 60, 2.6, 1);
                    for (int k = 0; k < 2 + rng.irange(3); k++) { const double a = rng.uni() * TAU, d = R + 60 + 90 * rng.uni(); const double rr = 5 + 6 * rng.uni(); L.add(BK_MOUND, std::sin(a) * d, std::cos(a) * d, 0, rr, rr, 1.6 + rr * 0.2, 0); }
                    break;
                }
                case 1: {   // rows of standing stones across the land
                    const int rows = 3 + rng.irange(6);
                    const double half = 120 + 150 * rng.uni();
                    for (int k = 0; k < rows; k++) { L.at(H, 0, 0, (k - (rows - 1) * 0.5) * 11, rng.sym(15), x, z); L.add(BK_MENHIR, x, z, H, 1, half * (0.75 + 0.25 * rng.uni()), 2.2 + rng.uni(), 0); }
                    L.at(H, 0, 0, 0, half + 30, x, z); L.add(BK_STONE_CIRCLE, x, z, H, 14, 14, 2.4);
                    break;
                }
                case 2: {   // the great circle: its lintelled ring, the horseshoe of trilithons, the heel stone
                    L.add(BK_STONE_CIRCLE, 0, 0, H, 16, 16, 4.4);
                    for (int k = 0; k < 5; k++) { const double a = H + (k - 2) * 0.55 + PI; L.add(BK_TRILITHON, std::sin(a) * 8.5, std::cos(a) * 8.5, a, 2.6, 0.7, 5.5 + (k == 2 ? 1.5 : 0)); }
                    L.at(H, 0, 0, 0, 42, x, z); L.add(BK_MENHIR, x, z, H, 1.1, 0, 4.6, 0);
                    break;
                }
                default: {   // a field of dolmens and barrows
                    for (int k = 0; k < 4 + rng.irange(7); k++) { const double a = rng.uni() * TAU, d = 30 + 170 * rng.uni(); L.add(BK_DOLMEN, std::sin(a) * d, std::cos(a) * d, rng.uni() * TAU, 2.4, 3.4, 2); }
                    for (int k = 0; k < 3; k++) { const double a = rng.uni() * TAU, d = 40 + 160 * rng.uni(); const double rr = 6 + 5 * rng.uni(); L.add(BK_MOUND, std::sin(a) * d, std::cos(a) * d, 0, rr, rr, 2, 0); }
                    break;
                }
            }
            break;
        }
        case WO_FORT_LINE: {
            const double ax = w.sideA.x, az = w.sideA.z, bx = w.sideB.x, bz = w.sideB.z, len = std::hypot(bx - ax, bz - az), brg = std::atan2(bx - ax, bz - az);
            auto pt = [&](double t, double off, double& x, double& z) { x = ax + (bx - ax) * t + std::cos(brg) * off; z = az + (bz - az) * t - std::sin(brg) * off; };
            double x, z;
            if (w.style == 0) {   // a wall in stretches with its towers, a fort at the middle of every other stretch
                const int n = std::max(2, (int)(len / 40));
                for (int k = 0; k < n; k++) {
                    const double t = (k + 0.5) / n; pt(t, 0, x, z);
                    if (wet(x, z)) continue;
                    if (k % 8 == 4) L.add(BK_WALL_TOWER, x, z, brg, 4, 4, 10);
                    else L.add(BK_WALL, x, z, brg - PI / 2, len / n * 0.5 + 0.3, 1.2, 6.5);
                }
                if (w.group % 2 == (uint32_t)(w.seed % 2)) { pt(0.5, -45, x, z); if (!wet(x, z)) { L.add(BK_TOWER, x, z, brg, 6, 6, 14); for (int q = 0; q < 4; q++) { const double a = q * TAU / 4 + brg; L.add(BK_WALL, x + std::sin(a) * 22, z + std::cos(a) * 22, a, 22, 0.9, 5); } } }   // a wall's length runs across its heading: a ring's stretch faces out
            } else if (w.style == 1) {   // the rampart behind the ditch, a redoubt
                pt(0.5, -2, x, z); L.add(BK_MOUND, x, z, brg, 5, len * 0.5, 3.2, 3);
                pt(0.5, -70, x, z); if (!wet(x, z)) L.add(BK_FORT, x, z, brg, 32, 32, 3, 0);
            } else {   // dragon's teeth behind the ditch, pillboxes, a blockhouse
                const int n = std::max(2, (int)(len / 60));
                for (int k = 0; k < n; k++) { pt((k + 0.5) / n, -4, x, z); if (!wet(x, z)) L.add(BK_TEETH, x, z, brg, 4, len / n * 0.5, 1.2); }
                for (int k = 0; k < (int)(len / 420); k++) { pt((k + 0.5) / (len / 420), -30, x, z); if (!wet(x, z)) L.add(BK_BUNKER, x, z, brg + PI / 2, 3.4, 3.4, 2.4, 0); }
                pt(0.5, -80, x, z); if (!wet(x, z)) L.add(BK_BUNKER, x, z, brg, 9, 7, 4, 1);
            }
            break;
        }
        case WO_BATTLEFIELD: {
            double x, z;
            const int a = w.age;
            r.founded = w.year;   // its war graves' dead all of the battle's year
            if (a == AGE_PREHISTORIC) {
                const int n = 8 + rng.irange(18);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(160), rng.sym(90), x, z); L.add(BK_CAIRN, x, z, rng.uni() * TAU, 1.0 + 0.8 * rng.uni(), 1.0 + 0.6 * rng.uni(), 1.5); }
                L.add(BK_MENHIR, 0, 0, H, 0.9, 0, 3.4, 0);
            } else if (a == AGE_MEDIEVAL) {
                const int n = 2 + rng.irange(5);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(140), rng.sym(70), x, z); const double rr = 6 + 8 * rng.uni(); L.add(BK_MOUND, x, z, 0, rr, rr, 2 + rr * 0.18, 0); }
                L.at(H, 0, 0, 0, -20, x, z); L.add(BK_STELA, x, z, H, 1, 1, 3.2, MN_STELAE);
            } else if (a == AGE_INDUSTRIAL || a == AGE_MODERN) {
                L.at(H, 0, 0, 220, -170, x, z); L.add(BK_STELA, x, z, H, 1, 1, a == AGE_INDUSTRIAL ? 8 : 11, MN_OBELISKS);
                for (int q = 0; q < 2; q++) { L.at(H, 0, 0, 220 + (q ? 26 : -26), -150, x, z); L.plot(x, z, H, 9, 13); }
                if (a == AGE_MODERN) {
                    const int n = 4 + rng.irange(9);
                    for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(260), rng.sym(70), x, z); L.add(BK_VEHICLE, x, z, H + rng.sym(1.2) + (rng.uni() < 0.4 ? PI : 0), 1.8, 3.6, 2.4, 0); }
                    if (rng.uni() < 0.6) { L.at(H, 0, 0, rng.sym(200), rng.sym(150), x, z); L.add(BK_AIRCRAFT, x, z, rng.uni() * TAU, 7, 6.5, 2, 0); }
                    for (int k = 0; k < 1 + rng.irange(3); k++) { L.at(H, 0, 0, rng.sym(200), 95, x, z); L.add(BK_BUNKER, x, z, H + PI, 3.4, 3.4, 2.4, 0); }
                }
            } else {
                const int n = 2 + rng.irange(5);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, rng.sym(250), rng.sym(120), x, z); L.add(BK_WRECK, x, z, rng.uni() * TAU, 3 + 3 * rng.uni(), 8 + 6 * rng.uni(), 3, 0); }
                L.at(H, 0, 0, 0, -200, x, z); L.add(BK_STELA, x, z, H, 1, 1, 6, MN_PILLARS);
            }
            break;
        }
        case WO_LAUNCH: {
            double x, z;
            if (w.style == 0) {
                for (const GroundEdit& e : w.edits) if (e.kind == GE_FLAT) {
                    L.add(BK_LAUNCHPAD, e.x, e.z, H, 55, 55, 70 + 40 * rng.uni());
                    L.at(H, e.x, e.z, 70, -45, x, z); L.add(BK_TANKS, x, z, H, 22, 7, 12);
                }
                L.at(H, 0, 0, 0, -380, x, z); L.add(BK_HANGAR, x, z, H, 45, 65, 95 + 30 * rng.uni(), 0);
                L.at(H, 0, 0, 160, 300, x, z); L.add(BK_BUNKER, x, z, H, 10, 7, 4, 1);
            } else if (w.style == 1) {   // a mass driver: its spans rising east from the hall
                const int n = 22;
                for (int k = 0; k < n; k++) {
                    const double t = (k + 0.5) / n, along = -1100 + 2300 * t, h = 8 + 520 * t * t;
                    L.at(H, 0, 0, 0, along, x, z); L.add(BK_MAGLEV, x, z, H, 3, 2300.0 / n * 0.5, h, 0);
                }
                L.at(H, 0, 0, 0, -1180, x, z); L.add(BK_HANGAR, x, z, H, 40, 60, 60, 0);
            } else {
                L.add(BK_TETHER, 0, 0, H, 70, 70, 380, 0, AGE_FUTURISTIC);
                for (int k = 0; k < 4; k++) { const double a = H + k * TAU / 4 + PI / 4; L.add(BK_HANGAR, std::sin(a) * 200, std::cos(a) * 200, a, 30, 45, 50, 0); }
            }
            break;
        }
        case WO_ANTENNA: {
            double x, z;
            if (w.style == 0) {
                const int n = 4 + rng.irange(5);
                for (int k = 0; k < n; k++) { L.at(H, 0, 0, 0, (k - (n - 1) * 0.5) * 200, x, z); L.add(BK_MAST, x, z, H, 2, 2, 120 + 80 * rng.uni()); }
                L.at(H, 0, 0, 90, 0, x, z); L.add(BK_HALL, x, z, H, 8, 14, 8, 0, AGE_INDUSTRIAL);
            } else if (w.style == 1) {
                const int m = 3 + rng.irange(3); const double rr = 6 + 8 * rng.uni();
                for (int j = 0; j < m; j++) for (int i = 0; i < m; i++) { L.at(H, 0, 0, (i - (m - 1) * 0.5) * rr * 4.5, (j - (m - 1) * 0.5) * rr * 4.5, x, z); L.add(BK_DISH, x, z, H, rr, 1, 1); }
                L.at(H, 0, 0, 0, -(m * rr * 2.6 + 30), x, z); L.add(BK_HALL, x, z, H, 8, 14, 7, 0);
            } else if (w.style == 2) {
                const int m = 4 + rng.irange(4); const double hh = 30 + 25 * rng.uni();
                for (int j = 0; j < m; j++) for (int i = 0; i < m; i++) { L.at(H, 0, 0, (i - (m - 1) * 0.5) * 45, (j - (m - 1) * 0.5) * 45, x, z); L.add(BK_MAST, x, z, H, 1.5, 1.5, hh); }
                L.at(H, 0, 0, 0, -(m * 23 + 40), x, z); L.add(BK_HALL, x, z, H, 8, 14, 7, 0);
            } else {
                for (int k = 0; k < 12; k++) { const double a = k * TAU / 12; L.add(BK_DISH, std::sin(a) * 200, std::cos(a) * 200, a, 9, 1, 1); }
                L.add(BK_POD, 0, 0, H, 18, 18, 10);
            }
            break;
        }
        case WO_WRECK: {
            switch (w.style) {
                case 0: L.add(BK_SHIP, 0, 0, H, 3.4, 13 + 5 * rng.uni(), 3.6, 0); break;
                case 1: L.add(BK_SHIP, 0, 0, H, 6, 30 + 14 * rng.uni(), 7.5, 1); break;
                case 2: L.add(BK_SHIP, 0, 0, H, 12, 70 + 40 * rng.uni(), 13, 2); break;
                case 3: L.add(BK_AIRCRAFT, 0, 0, H, 16 + 10 * rng.uni(), 14 + 8 * rng.uni(), 4, 0); break;
                default: L.add(BK_WRECK, 0, 0, H, 14 + 12 * rng.uni(), 50 + 60 * rng.uni(), 8, 0); break;
            }
            break;
        }
        case WO_COLOSSUS: L.add(BK_COLOSSUS, 0, 0, H, std::max(5.0, w.height * 0.2), std::max(5.0, w.height * 0.2), w.height, w.style); break;
        case WO_TERRAFORM: {
            const double R = w.length * 0.5;
            L.add(BK_MOUND, 0, 0, 0, R, 10, 4, 4);
            for (int k = 0; k < 4; k++) { const double a = H + k * TAU / 4; L.add(BK_POD, std::sin(a) * (R - 60), std::cos(a) * (R - 60), a, 12, 12, 8); }
            break;
        }
        case WO_SHELTER: {
            double x, z;
            const bool hill = w.height > 0.12;
            switch (w.style) {
                case 0: L.add(BK_PORTAL, 0, 0, H, 3.6, 1, 3); L.at(H, 0, 0, 0, 7, x, z); L.add(BK_TENT_RING, x, z, H, 2.4, 2.4, 0.4); break;
                case 1:
                    L.add(BK_TOWER, 0, 0, H, 6, 6, 16);
                    for (int q = 0; q < 6; q++) { const double a = q * TAU / 6 + H; if (q == 0) { L.add(BK_GATE, std::sin(a) * 26, std::cos(a) * 26, a, 3.5, 1.4, 6); continue; } L.add(BK_WALL, std::sin(a) * 26, std::cos(a) * 26, a, 13.5, 1.0, 6); }
                    break;
                case 2: L.add(BK_PORTAL, 0, 0, H, 4, 1, 5); L.at(H, 0, 0, 0, 30, x, z); if (!hill) L.add(BK_FORT, x, z, H, 14, 14, 4, 1); break;
                case 3:
                    if (hill) L.add(BK_PORTAL, 0, 0, H, 3, 1, 4.5); else L.add(BK_BUNKER, 0, 0, H, 9, 7, 4, 1);
                    L.at(H, 0, 0, 6, hill ? -30 : -24, x, z); L.add(BK_BUNKER, x, z, H, 2, 2, 2, 2);
                    for (int s = -1; s <= 1; s += 2) { L.at(H, 0, 0, s * 18, hill ? -40 : -30, x, z); L.add(BK_MAST, x, z, H, 1, 1, 22 + 6 * rng.uni()); }
                    L.at(H, 0, 0, 14, 16, x, z); L.add(BK_BUNKER, x, z, H, 4, 3, 3, 1);
                    break;
                case 5:   // D-05: a medieval people that lost its sun's halls under the mountain: their mouth in the rock, a gatehouse before it, walls either side
                    L.add(BK_PORTAL, 0, 0, H, 4.5, 1, 6);
                    L.at(H, 0, 0, 0, 16, x, z); L.add(BK_GATE, x, z, H, 3.5, 1.4, 6);
                    for (int s = -1; s <= 1; s += 2) { L.at(H, 0, 0, s * 11.5, 16, x, z); L.add(BK_WALL, x, z, H, 7, 1.0, 5); }
                    break;
                default:
                    L.add(BK_HABITAT, 0, 0, H, 22, 22, 8);
                    if (hill) { L.at(H, 0, 0, 0, 30, x, z); L.add(BK_PORTAL, x, z, H, 4, 1, 6); }
                    break;
            }
            break;
        }
        case WO_DOME: {   // D-05: the dome over the warm ground (`width` its radius), its door toward its place
            const double rad = w.width;
            if (w.style == 0) L.add(BK_ROTUNDA, 0, 0, H, rad, rad, 6 + rad * 0.25);   // a medieval people's vaulted hall over its spring
            else if (w.style == 3) L.add(BK_HABITAT, 0, 0, H, rad, rad, 8);
            else L.add(BK_DOME, 0, 0, H, rad, rad, 2.6, w.style == 1 ? 0 : (w.style == 2 ? 1 : 2));
            break;
        }
        case WO_GREENHOUSE: {   // D-05: the glasshouses in rows (`width` and `length` the block's half extents), their long sides along `heading`
            const int st = clampi(w.style, 0, 2);
            const double hw = st == 0 ? 4.0 : (st == 1 ? 4.5 : 6.0), hd = st == 0 ? 12.0 : (st == 1 ? 18.0 : 20.0), h = st == 2 ? hw * 0.9 : hw;
            const int rows = std::max(1, (int)std::lround((2 * w.width + 3) / (2 * hw + 3))), per = std::max(1, (int)std::lround((2 * w.length + 4) / (2 * hd + 4)));
            for (int i = 0; i < rows; i++)
                for (int j = 0; j < per; j++) {
                    double x, z; rot(H, (i - (rows - 1) * 0.5) * (2 * hw + 3), (j - (per - 1) * 0.5) * (2 * hd + 4), x, z);
                    if (wet(x, z)) continue;
                    L.add(BK_GREENHOUSE, x, z, H, hw, hd, h, st);
                }
            break;
        }
        case WO_STRANGE: {
            switch (w.style) {
                case 1: {   // the pillars of this stretch, one every 400 m along the line
                    const double hd = w.length * 0.5;
                    for (double t = -hd + 200; t <= hd - 199; t += 400) { double x, z; rot(H, 0, t, x, z); L.add(BK_SPHERE, x, z, H, 4, 4, 160, 1); }
                    break;
                }
                case 3: { Building& b = L.add(BK_LATTICE, 0, 0, H, w.length * 0.5, w.width * 0.5, w.crest - w.floor, 0); b.base = w.floor; break; }
                case 4: L.add(BK_SPHERE, 0, 0, H, w.height, w.height, w.height * 2, 0); break;
                default: break;   // a plain made flat, a mountain cut into terraces: the ground alone
            }
            break;
        }
        case WO_WINDFARM: {
            const double top = w.ground;
            for (int row = -1; row <= 1; row++)
                for (int k = -4; k <= 4; k++) {
                    double x, z; rot(H, row * 380 + rng.sym(30), k * 330 + rng.sym(40), x, z);
                    if (wet(x, z) || ground(x, z) < top - 70) continue;
                    L.add(BK_TURBINE, x, z, r.site.wind + PI, 42 + 12 * rng.uni(), 1, w.height, 0, AGE_MODERN);
                }
            break;
        }
        case WO_LIGHTHOUSE: {
            L.add(BK_LIGHTHOUSE, 0, 0, H, 3.4 + 1.2 * rng.uni(), 3.4, w.height, 0);
            break;
        }
        case WO_FORT: {   // B-418: on its height, its gate toward the settlement it guarded (`heading`)
            double x, z;
            switch (w.style) {
                case 0: {   // a hill fort: its rings of bank inside their ditches (the edits) in stretches each on its own ground (the rampart follows
                            // the hill), a gap at the gate; the huts of its last people inside
                    const int rings = std::max(1, (int)w.height);
                    for (int k = 0; k < rings; k++) {
                        const double R = w.width - k * 18;
                        const int n = std::max(12, (int)std::ceil(TAU * R / 16));
                        for (int q = 0; q < n; q++) {
                            const double a = (q + 0.5) * TAU / n;
                            if (std::fabs(std::remainder(a - H, TAU)) < std::max(0.1, PI / n)) continue;   // the gate (R-412: the stretch nearest it whatever the ring's: a quarter of the forts had none)
                            L.add(BK_MOUND, std::sin(a) * R, std::cos(a) * R, a + PI / 2, 2.6, PI * R / n + 0.6, 2.4 - 0.3 * k, 3);
                        }
                    }
                    const double inner = std::max(10.0, w.width - rings * 18);
                    const int nh = 3 + rng.irange(5);
                    std::vector<std::pair<double, double>> huts;   // B-421: each clear of the others (two stood through each other, one's doorway in the other's wall)
                    for (int k = 0; k < nh; k++) {
                        const double a = rng.uni() * TAU, d = inner * 0.6 * std::sqrt(rng.uni()), hh = rng.uni() * TAU, hw = 2.6 + rng.uni(), hd = 2.6 + rng.uni();
                        const double hx = std::sin(a) * d, hz = std::cos(a) * d;
                        bool clear = true; for (const auto& q : huts) clear = clear && std::hypot(q.first - hx, q.second - hz) > 2 * 3.6 + 2.5;
                        if (!clear) continue;
                        huts.push_back({hx, hz});
                        L.add(BK_HUT, hx, hz, hh, hw, hd, 2.2);
                    }
                    break;
                }
                case 1: {   // a castle: its curtain wall of `height` sides with a tower at each corner, the gatehouse in the side toward its settlement, the keep
                    const int n = clampi((int)w.height, 4, 8);
                    const double R = w.width, half = R * std::sin(PI / n), apo = R * std::cos(PI / n);
                    for (int q = 0; q < n; q++) {
                        const double a = H + q * TAU / n;   // the middle of side q faces out at `a`
                        if (q == 0) {
                            L.add(BK_GATE, std::sin(a) * apo, std::cos(a) * apo, a, 3.6, 1.6, 7.5);
                            for (int sd = -1; sd <= 1; sd += 2) { L.at(a, 0, 0, sd * (half + 3.6) * 0.5, apo, x, z); L.add(BK_WALL, x, z, a, (half - 3.6) * 0.5 + 0.3, 1.2, 7); }
                        } else L.add(BK_WALL, std::sin(a) * apo, std::cos(a) * apo, a, half + 0.3, 1.2, 7);
                        const double c = a + PI / n;   // the corner after it
                        L.add(BK_WALL_TOWER, std::sin(c) * R, std::cos(c) * R, c, 3.6, 3.6, 11);
                    }
                    L.at(H, 0, 0, 0, -R * 0.35, x, z); L.add(BK_TOWER, x, z, H, 6.5, 6.5, 20);   // the keep, back from the gate
                    break;
                }
                case 2: L.add(BK_FORT, 0, 0, H, w.width, w.width, 3, 0); break;   // a redoubt: its ring bank and casemate (its ditch in the edits)
                default: {   // a strongpoint: a blockhouse, pillboxes round it, dragon's teeth across the way up
                    L.add(BK_BUNKER, 0, 0, H, 9, 7, 4, 1);
                    const int np = 3 + rng.irange(3);
                    for (int k = 0; k < np; k++) { const double a = H + k * TAU / np + rng.sym(0.3), d = 40 + 18 * rng.uni(); L.add(BK_BUNKER, std::sin(a) * d, std::cos(a) * d, a, 3.4, 3.4, 2.4, 0); }
                    L.at(H, 0, 0, 0, 70, x, z); L.add(BK_TEETH, x, z, H + PI / 2, 4, 28, 1.2);
                    break;
                }
            }
            break;
        }
        default: break;
    }
}

std::string workLine(const Work& w) {
    char buf[512];
    std::string what;
    switch (w.kind) {
        case WO_DAM: { bool drained = false; for (const GroundEdit& e : w.edits) if (e.kind == GE_WATER && e.tone == ET_DARK) drained = true;
            char b2[220]; snprintf(b2, sizeof b2, "%.0f m over its floor, %.0f m long; its reservoir at %.0f m, %.2f km2%s", w.height, w.length, w.level, w.width / 1e6, drained ? ", breached and drained" : ""); what = b2; break; }
        case WO_QUARRY: case WO_MINE: { static const char* const QS[7] = {"flint pits", "a face", "a face in benches", "a face in great benches", "an open pit", "a stone pit", "a pit in benches"}; static const char* const MS[3] = {"adits", "headframes", "an open pit"};
            what = w.kind == WO_QUARRY ? QS[clampi(w.style, 0, 6)] : MS[clampi(w.style, 0, 2)]; break; }
        case WO_BATTLEFIELD: { char b2[160]; snprintf(b2, sizeof b2, "the war %d's battle of year %d, %.0f km between its sides", w.war, w.year, w.length / 1000.0); what = b2; break; }
        case WO_AQUEDUCT: { char b2[120]; snprintf(b2, sizeof b2, "%.1f km from its spring at %.0f m", w.length / 1000.0, w.level); what = b2; break; }
        case WO_OBSERVATORY: { char b2[80]; snprintf(b2, sizeof b2, "on a summit of %.0f m", w.height); what = b2; break; }
        default: { char b2[60]; snprintf(b2, sizeof b2, "style %d", w.style); what = b2; break; }
    }
    snprintf(buf, sizeof buf, "%s (%s%s) at %.3f %.3f, %.0f m across%s%s; %s", w.kind == WO_SHELTER && w.before > 0 ? "a shelter" : WORK_KIND_NAMES[w.kind],   // D-05: a shelter not the last
             AGE_NAMES[clampi(w.age, 0, AGE_COUNT - 1)], w.before > 0 ? ", left early" : "",
             w.lat / DEG, w.lon / DEG, 2 * w.radiusM, w.year ? (std::string(", year ") + std::to_string(w.year)).c_str() : "", w.heard ? "" : ", unheard", what.c_str());
    return buf;
}
