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
    "the last shelter", "something of the far future", "a wind farm", "a lighthouse"};
const char* const WORK_HEARD[WO_COUNT] = {"A QUARRY", "A MINE", "A DAM", "AN AQUEDUCT", "AN OBSERVATORY", "A SHRINE", "A WAY", "A NECROPOLIS", "LINES ON THE GROUND",
    "STANDING STONES", "A FORTIFIED LINE", "A BATTLEFIELD", "A LAUNCH SITE", "AN ANTENNA FIELD", "A WRECK", "A COLOSSUS", "SHAPED LAND", "A SHELTER",
    "SOMETHING MADE", "A WIND FARM", "A LIGHTHOUSE"};

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
struct WB {
    const BodyGen& g; const WorldPlan& P; WorldWorks& W; double Rm;
    struct CellS { bool has = false; Vec3 u; double r = 0; };
    std::unordered_map<uint64_t, CellS> cells;
    // the people being built
    int p = 0, age = 0; const PeoplePlan* pp = nullptr; std::shared_ptr<const History> H; Culture C; uint64_t pseed = 0;
    int made[WO_COUNT] = {};

    WB(const BodyGen& gg, const WorldPlan& PP, WorldWorks& WW) : g(gg), P(PP), W(WW), Rm(gg.R * 1000.0) {}
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
        if (ruinOfCell(g, row, gl, sp, false) && sp.kind == RK_SETTLEMENT && sp.sclass != SC_WORKS) { c.has = true; c.u = StarSystem::bodyFromLatLon(sp.lat, sp.lon); c.r = sp.size; }
        return cells[key] = c;
    }
    // nothing else within `r`: no settlement of the cells (its radius and a margin), no city's core, no work already made (its reach)
    bool clearOf(const Vec3& u, double r, bool ofWorks = true) {
        if (ofWorks) for (const Work& w : W.works) if (arcM(u, w.unit, Rm) < r + w.radiusM + 40) return false;
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
    int eraAge(int year) const { if (!H || H->eras.empty()) return age; return H->eras[clampi(H->eraAt(year), 0, (int)H->eras.size() - 1)].age; }
    // a history place's last age and the years before the end it was left (a place left early: the era it was left in)
    void placeAge(int hp, int& a, double& before) const {
        a = age; before = 0;
        if (!H || hp < 0 || hp >= (int)H->places.size()) return;
        const HistoryPlace& q = H->places[hp];
        if (q.fate == HP_EARLY || q.fate == HP_CLEARED) { a = eraAge(q.ended); before = std::max(0, H->last - q.ended); }
    }
    // the place was cleared by its people (a futuristic or far-future people's older city): its works went with it
    bool cleared(int hp) const { return H && hp >= 0 && hp < (int)H->places.size() && H->places[hp].fate == HP_CLEARED; }
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
        W.works.push_back(w);
        made[kind]++;
        return W.works.back();
    }
    // the events of a kind: HE_BUILT of a work (`thing`), HE_BATTLE, ...
    template <class F> void forEvents(int kind, F fn) const { if (!H) return; for (int i = 0; i < (int)H->events.size(); i++) if (H->events[i].kind == kind) fn(i, H->events[i]); }

    void quarries(); void mines(); void dams(); void aqueducts(); void observatories(); void shrine(); void necropolises(); void geoglyphs();
    void henges(); void fortLine(); void battlefields(); void launchSites(); void antennas(); void wrecks(); void colossi(); void terraforms();
    void shelter(); void strange(); void windfarms(); void lighthouses();
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

// ---- the quarries: in the rock their town was built of -----------------------------------------------------------------------
void WB::quarries() {
    if (age >= AGE_FUTURISTIC) return;
    const bool flint = age == AGE_PREHISTORIC;
    if (!flint && C.family != 0) return;   // an adobe people dug its earth where it built: no quarry
    Rng rng(pseed ^ 0x70310ULL);
    int n = 0;
    for (const PlanPlace& pl : pp->places) {
        if (n >= (flint ? 4 : 24)) break;
        if (flint ? pl.rank > 12 : !townOrGreater(pl.sclass)) continue;
        int a; double before; placeAge(pl.rank, a, before);
        if (a >= AGE_FUTURISTIC) a = AGE_MODERN;
        const int style = flint ? 0 : (a <= AGE_MEDIEVAL ? 1 : (a == AGE_INDUSTRIAL ? 2 : 3));
        struct Cand { Vec3 u; double score, slope, down, h; };
        std::vector<Cand> cs;
        const double a0 = rng.uni() * TAU;
        static const double RINGS[5] = {700, 1500, 2500, 3700, 5200};
        for (int ri = 0; ri < 5; ri++)
            for (int k = 0; k < 16; k++) {
                const double ang = a0 + k * TAU / 16 + (ri & 1) * TAU / 32, d = pl.radiusM + RINGS[ri];
                const Vec3 u = offsetUnit(pl.unit, std::sin(ang) * d, std::cos(ang) * d, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s)) continue;
                const int fam = matFamily(s.material);
                if (flint) { if (fam == FAM_FOREST || fam == FAM_SNOW || fam == FAM_WATER) continue; }
                else if (fam != FAM_ROCK) continue;
                double down, h; const double sl = grade(u, 45, down, h);
                double sc;
                if (flint) { if (sl > 0.15) continue; sc = 1 - sl * 3 - d / 12000; }
                else if (sl > 1.4) continue;
                else sc = (sl >= 0.18 ? 0.3 + std::min(sl, 0.6) : 0.2 - sl) - d / 12000;   // a face where the hill stands steep, a pit in flat rock
                cs.push_back({u, sc, sl, down, h});
            }
        std::sort(cs.begin(), cs.end(), [](const Cand& x, const Cand& y) { return x.score > y.score; });
        for (const Cand& c : cs) {
            const bool pit = !flint && c.slope < 0.18;
            const double hw = style == 1 ? 18 + 14 * rng.uni() : (style == 2 ? 32 + 22 * rng.uni() : 55 + 40 * rng.uni());
            const double hd = style == 1 ? 15 + 12 * rng.uni() : (style == 2 ? 28 + 18 * rng.uni() : 45 + 30 * rng.uni());
            const double pitR = style == 1 ? 28 + 22 * rng.uni() : (style == 2 ? 60 + 50 * rng.uni() : 140 + 130 * rng.uni());
            const double reach = flint ? 70 : (pit ? pitR + 70 : std::hypot(hw, hd) + 45);
            if (!clearOf(c.u, reach)) continue;
            Work& w = add(WO_QUARRY, c.u, reach, seedOf(WO_QUARRY, pl.rank));
            w.age = flint ? AGE_PREHISTORIC : a; w.style = pit ? (style == 3 ? 4 : 4 + style) : style; w.place = pl.rank; w.planRank = pl.rank; w.before = before;
            w.heading = c.down + PI;   // into the hill
            if (flint) {   // the pits and the spoil round each
                Rng r2(w.seed ^ 0xF117ULL);
                const int np = 14 + r2.irange(16);
                for (int k = 0; k < np; k++) {
                    const double rr = 55 * std::sqrt(r2.uni()), aa = r2.uni() * TAU, x = std::sin(aa) * rr, z = std::cos(aa) * rr;
                    GroundEdit e; e.kind = GE_PIT; e.x = x; e.z = z; e.r = 2.2 + 1.6 * r2.uni();
                    e.level = at(Frame(c.u, Rm).unit(x, z)).height - 1.8 - 0.8 * r2.uni(); e.fades = true;
                    w.edits.push_back(e);
                }
            } else if (pit) {   // a stone pit in benches: a medieval people's shallow one of three-metre steps, an industrial one's, a modern open pit
                const int nb = style == 1 ? 2 + (int)(rng.uni() * 3) : (style == 2 ? 3 + (int)(rng.uni() * 3) : 4 + (int)(rng.uni() * 4));
                GroundEdit e; e.kind = GE_PIT; e.r = pitR; e.depth = style == 1 ? 3 + rng.uni() : (style == 2 ? 6 + 2 * rng.uni() : 11 + 3 * rng.uni());
                e.step = std::min(style == 1 ? 6.0 : 22.0, pitR * 0.55 / nb); e.level = c.h - nb * e.depth; e.hw = pitR - nb * e.step; e.fades = false;
                w.edits.push_back(e); w.floor = e.level; w.length = 2 * pitR;
            } else {
                // the floor level with the ground at the cut's front edge, the benches stepping up into the hill
                const double fx = -std::sin(w.heading) * hd, fz = -std::cos(w.heading) * hd;
                GroundEdit e; e.kind = GE_CUT; e.heading = w.heading; e.hw = hw; e.hd = hd; e.fades = false;
                e.level = std::min(at(Frame(c.u, Rm).unit(fx, fz)).height, c.h) - 0.8;   // no higher than its middle's ground (the slope is not a plane)
                const int nb = style == 1 ? 1 : (style == 2 ? 3 : 4);
                e.step = 2 * hd / nb; e.depth = nb > 1 ? (style == 2 ? 6 : 10) : 0;
                w.edits.push_back(e); w.floor = e.level; w.length = 2 * hw; w.width = 2 * hd;
            }
            n++;
            break;
        }
    }
}

// ---- the mines: where the plan read coal and ore, and where the history sank one -----------------------------------------------
void WB::mines() {
    if (age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC) return;
    Rng rng(pseed ^ 0x70320ULL);
    struct Src { Vec3 u; double r; int a; double before; int hp, ev, year; };
    std::vector<Src> src;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_MINE || e.place < 0 || src.size() >= 6 || cleared(e.place)) return;
        Src s; placeAt(e.place, s.u, s.r); s.a = eraAge(e.year); int pa; placeAge(e.place, pa, s.before); s.hp = e.place; s.ev = i; s.year = e.year;
        src.push_back(s);
    });
    {   // the territory's richest ground
        std::vector<std::pair<float, int>> rich;
        for (int c = 0; c < P.grid.cells; c++) if (pp->territory[c] && P.grid.land[c] && P.grid.res[c] > 0.35f) rich.push_back({-P.grid.res[c], c});
        std::sort(rich.begin(), rich.end());
        const int want = age == AGE_MEDIEVAL ? 2 : 3;
        for (int k = 0; k < (int)rich.size() && k < want; k++) { Src s; s.u = P.grid.unitOf(rich[k].second); s.r = 0; s.a = age; s.before = 0; s.hp = -1; s.ev = -1; s.year = 0; src.push_back(s); }
    }
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
        w.age = a; w.style = style; w.before = s.before; w.place = s.hp; w.event = s.ev; w.year = s.year; w.heading = bestDown + PI;
        if (s.hp >= 0 && H->places[s.hp].plan >= 0) w.planRank = H->places[s.hp].plan;
        if (style == 2) {
            const int nb = 5 + (int)(rng.uni() * 5);
            GroundEdit e; e.kind = GE_PIT; e.r = pitR; e.depth = 13 + 3 * rng.uni(); e.step = std::min(24.0, pitR * 0.55 / nb); e.level = bestH - nb * e.depth; e.hw = pitR - nb * e.step; e.fades = false;
            w.edits.push_back(e); w.floor = e.level; w.length = 2 * pitR;
        }
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
        if (e.thing != WK_DAM || e.place < 0 || made[WO_DAM] >= 5 || cleared(e.place)) return;
        const int a = clampi(eraAge(e.year), AGE_INDUSTRIAL, AGE_FUTURISTIC);
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
    if (!H || age < AGE_MEDIEVAL) return;
    Rng rng(pseed ^ 0x70340ULL);
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_AQUEDUCT || e.place < 0 || made[WO_AQUEDUCT] >= 3 || cleared(e.place)) return;
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
        w.age = AGE_MEDIEVAL; w.place = e.place; w.event = i; w.year = e.year; int pa; placeAge(e.place, pa, w.before);
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
    if (age < AGE_MEDIEVAL) return;
    Rng rng(pseed ^ 0x70350ULL);
    std::vector<std::pair<int, int>> src;   // (history place, event)
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) { if (e.thing == WK_OBSERVATORY && e.place >= 0 && src.size() < 3 && !cleared(e.place)) src.push_back({e.place, i}); });
    if (src.empty() && age >= AGE_FUTURISTIC && !pp->places.empty()) src.push_back({0, -1});   // a futuristic people's array by its first great place
    int k = 0;
    for (auto [hp, ev] : src) {
        Vec3 u0; double r0;
        if (H && hp < (int)H->places.size()) placeAt(hp, u0, r0); else { u0 = pp->places[0].unit; r0 = pp->places[0].radiusM; }
        Vec3 top; double hTop;
        if (!summitNear(u0, r0 + 1500, r0 + 24000, rng, top, hTop)) continue;
        if (!clearOf(top, 180)) continue;
        Work& w = add(WO_OBSERVATORY, top, 180, seedOf(WO_OBSERVATORY, k++));
        w.age = ev >= 0 ? clampi(eraAge(H->events[ev].year), AGE_MEDIEVAL, AGE_FUTURISTIC) : age;
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
    if (hp >= (int)H->places.size()) return;
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
    w.age = ev >= 0 ? eraAge(H->events[ev].year) : age;
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

// ---- the necropolises: on the slope that faces the city across the low ground --------------------------------------------------------
void WB::necropolises() {
    if (age >= AGE_FAR_FUTURE) return;
    const int dead = C.genome.of(GV_DEAD);
    if (dead == DE_NOTHING) return;
    Rng rng(pseed ^ 0x70370ULL);
    int n = 0;
    for (const PlanPlace& pl : pp->places) {
        if (n >= 4) break;
        if (pl.rank != 0 && pl.sclass != SC_CITY && !(age == AGE_PREHISTORIC && pl.rank < 4)) continue;
        int a; double before; placeAge(pl.rank, a, before);
        const double ph = at(pl.unit).height;
        Vec3 best; double bestS = -1e18, bestDown = 0; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {900.0, 1600.0, 2500.0, 3600.0})
            for (int q = 0; q < 16; q++) {
                const double ang = a0 + q * TAU / 16, dd = pl.radiusM + d;
                const Vec3 u = offsetUnit(pl.unit, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                const SurfaceSample s = at(u);
                if (wetS(s) || s.material == MAT_FOREST || matFamily(s.material) == FAM_SNOW) continue;
                double down, h; const double sl = grade(u, 50, down, h);
                if (sl > 0.25) continue;
                const double toCity = ang + PI;   // the bearing back to the city
                double sc = std::cos(down - toCity) * std::min(sl, 0.15) * 4 - d / 8000;   // a slope that looks at the city
                const SurfaceSample midS = at(slerpUnit(pl.unit, u, 0.55));
                if (pl.river && midS.height < std::min(h, ph) - 4) sc += 0.6;   // across the low ground (the river's) between
                if (wetS(midS)) sc += pl.river ? 0.5 : 0.2;
                if (sc > bestS && clearOf(u, 160)) { bestS = sc; best = u; bestDown = down; any = true; }
            }
        if (!any) continue;
        int style;
        if (a == AGE_PREHISTORIC || dead == DE_BARROWS) style = 0;
        else if (dead == DE_TOWERS) style = 5;
        else if (a >= AGE_FUTURISTIC) style = 4;
        else if (dead == DE_TOMBS) style = 1;
        else style = a == AGE_MODERN ? 3 : 2;
        const double reach = style == 2 || style == 3 ? 170 : 120;
        Work& w = add(WO_NECROPOLIS, best, reach, seedOf(WO_NECROPOLIS, pl.rank));
        w.age = a; w.before = before; w.place = pl.rank; w.planRank = pl.rank; w.style = style;
        w.heading = bestDown;   // down the slope, toward the city
        n++;
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

// ---- the henges and the rows of standing stones: on open ground near the old people's seats --------------------------------------------
void WB::henges() {
    Rng rng(pseed ^ 0x70390ULL);
    struct Src { Vec3 u; double r, before; int hp, ev, year; };
    std::vector<Src> src;
    if (age == AGE_PREHISTORIC) { for (const PlanPlace& pl : pp->places) if (src.size() < 3 && (pl.rank == 0 || pl.rank % 5 == 2)) src.push_back({pl.unit, pl.radiusM, 0, pl.rank, -1, 0}); }
    else if (H && !H->eras.empty() && H->eras.front().age == AGE_PREHISTORIC && age < AGE_FUTURISTIC) {
        forEvents(HE_STONES, [&](int i, const HistoryEvent& e) { if (src.size() < 1) src.push_back({pp->places[0].unit, pp->places[0].radiusM, (double)std::max(0, H->last - e.year), 0, i, e.year}); });
    }
    int k = 0;
    for (const Src& s : src) {
        Vec3 best; double bestS = -1e18; bool any = false;
        const double a0 = rng.uni() * TAU;
        for (double d : {1500.0, 2600.0, 4000.0, 6000.0})
            for (int q = 0; q < 14; q++) {
                const double ang = a0 + q * TAU / 14, dd = s.r + d;
                const Vec3 u = offsetUnit(s.u, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
                const SurfaceSample sm = at(u);
                if (wetS(sm) || sm.material == MAT_FOREST || matFamily(sm.material) == FAM_SNOW) continue;
                double down, h; const double sl = grade(u, 60, down, h);
                if (sl > 0.08) continue;
                double lowest = 1e18; for (int m = 0; m < 6; m++) { const double b = m * TAU / 6; lowest = std::min(lowest, at(offsetUnit(u, std::sin(b) * 400, std::cos(b) * 400, Rm)).height); }
                const double sc = (h - lowest) * 0.05 - sl * 4 - d / 10000;   // a slight rise
                if (sc > bestS && clearOf(u, 260)) { bestS = sc; best = u; any = true; }
            }
        if (!any) continue;
        Work& w = add(WO_HENGE, best, 260, seedOf(WO_HENGE, k++));
        w.age = AGE_PREHISTORIC; w.before = s.before; w.place = s.hp; w.planRank = s.hp >= 0 && s.hp < (int)pp->places.size() ? s.hp : -1; w.event = s.ev; w.year = s.year;
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
        w.age = eraAge(e.year); w.before = std::max(0, H->last - e.year); w.event = pe.second; w.year = e.year; w.war = e.thing; w.place = e.place;
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
            w.age = clampi(eraAge(e.year), AGE_MODERN, AGE_FUTURISTIC); w.event = se.first; w.year = e.year;   // stood where the latitude put it, not by its town
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

// ---- the wrecks: ships on the shore near the harbour towns, an aircraft on open ground, wreckage fallen from orbit ----------------------
void WB::wrecks() {
    if (age < AGE_MEDIEVAL || age == AGE_FAR_FUTURE) return;
    Rng rng(pseed ^ 0x703E0ULL);
    int k = 0;
    if (age <= AGE_MODERN) {
        int ships = 0;
        for (const PlanPlace& pl : pp->places) {
            if (ships >= 4) break;
            if (!pl.coast || rng.uni() < 0.4) continue;
            Vec3 shore; double seaward;
            if (!shoreNear(pl.unit, pl.radiusM + 1500, pl.radiusM + 12000, rng, shore, seaward, false)) continue;
            if (!clearOf(shore, 140)) continue;
            int a; double before; placeAge(pl.rank, a, before);
            Work& w = add(WO_WRECK, shore, 140, seedOf(WO_WRECK, k++));
            w.age = clampi(a, AGE_MEDIEVAL, AGE_MODERN); w.before = before * rng.uni(); w.style = w.age - AGE_MEDIEVAL; w.place = pl.rank; w.planRank = pl.rank;
            w.heading = seaward + PI / 2 + rng.sym(0.5); w.axis = seaward;
            ships++;
        }
    }
    const PlanGrid& G = P.grid;
    const int others = age == AGE_MODERN ? 2 : (age == AGE_FUTURISTIC ? 3 : 0);
    for (int q = 0, tries = 0; q < others && tries < 40; tries++) {
        const int c = (int)(rng.uni() * G.cells);
        if (!pp->territory[c] || !G.land[c] || G.relief[c] > 0.4f) continue;
        const Vec3 u = offsetUnit(G.unitOf(c), rng.sym(8000), rng.sym(8000), Rm);
        const SurfaceSample s = at(u);
        if (wetS(s) || s.material == MAT_FOREST) continue;
        double down, h; if (grade(u, 60, down, h) > 0.08) continue;
        const bool orbit = age == AGE_FUTURISTIC;
        if (!clearOf(u, orbit ? 400 : 120)) continue;
        Work& w = add(WO_WRECK, u, orbit ? 400 : 120, seedOf(WO_WRECK, 100 + q));
        w.age = age; w.style = orbit ? 4 : 3; w.heading = rng.uni() * TAU;
        w.before = rng.uni() * (H ? std::max(0, H->last - (H->eras.empty() ? H->last : H->eras.back().start)) : 0) * 0.5;
        if (orbit) { GroundEdit c2; c2.kind = GE_BOWL; c2.r = 60 + 50 * rng.uni(); c2.depth = c2.r * 0.25; c2.tone = ET_DARK; w.edits.push_back(c2); }
        q++;
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
void WB::shelter() {
    if (!H || H->lastPlace < 0 || age == AGE_FAR_FUTURE) return;
    Rng rng(pseed ^ 0x70410ULL);
    Vec3 u0; double r0; placeAt(H->lastPlace, u0, r0);
    Vec3 best; double bestS = -1e18, bestDown = 0; bool any = false;
    const double a0 = rng.uni() * TAU;
    for (double d : {600.0, 1400.0, 2600.0, 4200.0, 6500.0})
        for (int q = 0; q < 20; q++) {
            const double ang = a0 + q * TAU / 20, dd = r0 + d;
            const Vec3 u = offsetUnit(u0, std::sin(ang) * dd, std::cos(ang) * dd, Rm);
            const SurfaceSample s = at(u);
            if (wetS(s)) continue;
            double down, h; const double sl = grade(u, 40, down, h);
            double sc = std::min(sl, 0.8) - d / 15000 + (matFamily(s.material) == FAM_ROCK ? 0.2 : 0);
            if (age == AGE_MEDIEVAL) sc = h * 0.004 - d / 15000;   // a citadel on the height
            if (sc > bestS && clearOf(u, 140)) { bestS = sc; best = u; bestDown = down; any = true; }
        }
    if (!any) return;
    Work& w = add(WO_SHELTER, best, 140, seedOf(WO_SHELTER, 0));
    w.age = age; w.place = H->lastPlace; w.planRank = H->places[H->lastPlace].plan; w.year = H->last;
    forEvents(HE_END, [&](int i, const HistoryEvent&) { w.event = i; });
    double down2, h2; const double sl = grade(best, 40, down2, h2);
    w.style = age == AGE_PREHISTORIC ? 0 : (age == AGE_MEDIEVAL ? 1 : (age == AGE_INDUSTRIAL ? 2 : (age == AGE_MODERN ? 3 : 4)));
    w.heading = sl > 0.12 ? bestDown : bearingOf(best, u0);   // its door out of the hill, else toward the place
    w.height = sl;
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

// ---- the wind farms: rows of turbines along the high ground ---------------------------------------------------------------------------
void WB::windfarms() {
    if (age != AGE_MODERN) return;
    Rng rng(pseed ^ 0x70430ULL);
    const PlanGrid& G = P.grid;
    std::vector<std::pair<double, int>> windy;
    for (int c = 0; c < G.cells; c++) if (pp->territory[c] && G.land[c] && G.relief[c] > 0.15f && G.relief[c] < 0.6f) windy.push_back({-(G.relief[c] + (G.coastKm[c] < 30 ? 0.2 : 0)) + rng.uni() * 0.25, c});
    std::sort(windy.begin(), windy.end());
    const int want = 1 + rng.irange(3);
    int k = 0;
    for (int li = 0; li < (int)windy.size() && li < 16 && k < want; li++) {
        Vec3 top; double hTop;
        if (!summitNear(offsetUnit(G.unitOf(windy[li].second), rng.sym(6000), rng.sym(6000), Rm), 300, 4000, rng, top, hTop)) continue;
        if (!clearOf(top, 1600)) continue;
        double down, h; grade(top, 200, down, h);
        Work& w = add(WO_WINDFARM, top, 1600, seedOf(WO_WINDFARM, k++));
        w.age = AGE_MODERN; w.heading = down + PI / 2;   // the rows along the ridge
        w.height = 80 + 50 * rng.uni();
    }
}

// ---- the lighthouses: on the headland nearest the harbour the history lit ----------------------------------------------------------------
void WB::lighthouses() {
    if (!H || age < AGE_MEDIEVAL || age >= AGE_FUTURISTIC) return;
    Rng rng(pseed ^ 0x70440ULL);
    int k = 0;
    forEvents(HE_BUILT, [&](int i, const HistoryEvent& e) {
        if (e.thing != WK_LIGHTHOUSE || e.place < 0 || made[WO_LIGHTHOUSE] >= 3 || cleared(e.place)) return;
        Vec3 u0; double r0; placeAt(e.place, u0, r0);
        Vec3 shore; double seaward;
        if (!shoreNear(u0, r0 + 400, r0 + 9000, rng, shore, seaward, true)) return;
        const Vec3 site = offsetUnit(shore, -std::sin(seaward) * 25, -std::cos(seaward) * 25, Rm);   // a little back from the edge
        if (!clearOf(site, 50)) return;
        Work& w = add(WO_LIGHTHOUSE, site, 50, seedOf(WO_LIGHTHOUSE, k++));
        w.age = clampi(eraAge(e.year), AGE_MEDIEVAL, AGE_MODERN); w.event = i; w.year = e.year; w.place = e.place; int pa; placeAge(e.place, pa, w.before);
        if (H->places[e.place].plan >= 0) w.planRank = H->places[e.place].plan;
        w.heading = seaward; w.height = w.age == AGE_MEDIEVAL ? 22 + 10 * rng.uni() : 30 + 25 * rng.uni();
    });
}
}   // namespace

// ---- the world's works ---------------------------------------------------------------------------------------------------------------
std::shared_ptr<WorldWorks> buildWorldWorks(const BodyGen& g) {
    auto W = std::make_shared<WorldWorks>();
    W->seed = g.seed; W->Rm = g.R * 1000.0;
    if (!worldHadCivilisation(g)) return W;
    const double t0 = nowMs();
    const std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    W->peoples = P->peoples;
    DrainageOff off;
    WB b(g, *P, *W);
    for (int p = 0; p < P->peoples; p++) {
        b.p = p; b.pp = &P->plans[p]; b.age = b.pp->age; b.H = historyOfWorld(g, p); b.C = cultureOf(g, p); b.pseed = peopleGen(g, p).seed ^ 0x7030ULL;
        std::fill(b.made, b.made + WO_COUNT, 0);
        if (b.pp->places.empty()) continue;
        // the history's first (its battles, the last shelter, the works it finished where it finished them), then the terrain's own
        b.battlefields(); b.shelter(); b.dams(); b.aqueducts(); b.observatories(); b.lighthouses(); b.launchSites(); b.shrine();
        b.necropolises(); b.henges(); b.colossi(); b.mines(); b.quarries(); b.fortLine(); b.antennas(); b.wrecks(); b.terraforms();
        b.strange(); b.windfarms(); b.geoglyphs();
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

void worldWorksClearCache() {
    std::lock_guard<std::mutex> lock(g_worksMutex);
    for (auto it = g_works.begin(); it != g_works.end();) { if (worksReady(it->second.fut)) it = g_works.erase(it); else ++it; }
}

void worksNear(const WorldWorks& W, const Vec3& unit, double radiusM, std::vector<const Work*>& out) {
    out.clear();
    for (const Work& w : W.works) if (arcM(unit, w.unit, W.Rm) < radiusM + w.radiusM) out.push_back(&w);
}

// ---- a work's pieces --------------------------------------------------------------------------------------------------------------
namespace {
struct Lay {
    RuinSpec& r; const Work& w; Rng rng; int n = 0;
    Lay(RuinSpec& rr, const Work& ww) : r(rr), w(ww), rng(ww.seed ^ 0x5BEC0ULL) {}
    Building& add(int kind, double x, double z, double heading, double hw, double hd, double h, int style = 0, int age = -2) {
        Building b; b.kind = kind; b.x = x; b.z = z; b.heading = heading; b.hw = hw; b.hd = hd; b.hWall = h; b.thick = 0.5; b.door = -1;
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

void workSpec(const BodyGen& g, const Work& w, RuinSpec& r) {
    r = RuinSpec();
    r.kind = RK_SETTLEMENT; r.sclass = SC_WORKS; r.plan = SP_LONE; r.lat = w.lat; r.lon = w.lon; r.heading = w.heading; r.size = w.radiusM;
    r.seed = w.seed; r.people = w.people; r.work = w.kind; r.buildAge = w.age; r.before = w.before;
    r.site.desert = g.type == PT_DESERT;
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
                Building& b = L.add(BK_AQUEDUCT, sx + (ex - sx) * tm, sz + (ez - sz) * tm, hd, 1.3, 0.5 * len * (t1 - t0), top - (lo - 1.5), 0, AGE_MEDIEVAL);
                b.base = lo - 1.5;
                k = k1;
            }
            L.add(BK_CISTERN, sx, sz, hd + PI, 3.5, 3.5, 3, 0, AGE_MEDIEVAL);   // the spring's house
            L.add(BK_CISTERN, ex, ez, hd, 4.5, 4.5, 4, 0, AGE_MEDIEVAL);        // the castellum at the town's edge
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
                    L.at(H, 0, 0, 0, -12, x, z);
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
                    for (int k = 0; k < 1 + rng.irange(3); k++) { L.at(H, 0, 0, rng.sym(80), -30 + rng.sym(10), x, z); L.add(BK_MENHIR, x, z, rng.uni() * TAU, 0.8, 0, 3.6, 0, AGE_PREHISTORIC); }
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
            for (int k = 0; k < 2; k++) L.add(BK_CAIRN, w.sideA.x + (k ? 6 : -4), w.sideA.z + (k ? -3 : 5), 0, 1.4, 1.4, 1.6, 0, AGE_PREHISTORIC);
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
                default:
                    L.add(BK_HABITAT, 0, 0, H, 22, 22, 8);
                    if (hill) { L.at(H, 0, 0, 0, 30, x, z); L.add(BK_PORTAL, x, z, H, 4, 1, 6); }
                    break;
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
    snprintf(buf, sizeof buf, "%s (%s%s) at %.3f %.3f, %.0f m across%s%s; %s", WORK_KIND_NAMES[w.kind], AGE_NAMES[clampi(w.age, 0, AGE_COUNT - 1)], w.before > 0 ? ", left early" : "",
             w.lat / DEG, w.lon / DEG, 2 * w.radiusM, w.year ? (std::string(", year ") + std::to_string(w.year)).c_str() : "", w.heard ? "" : ", unheard", what.c_str());
    return buf;
}
