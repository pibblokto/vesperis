#pragma once
// E-09: what works.cpp's parts share, for them alone (works, works_sites, works_land, works_pieces): the includes, the
// helpers on the unit sphere and in a work's own metres, the land's grid, and the builder (`WB`), whose members the parts define
#include "works.h"
#include "plan.h"
#include "history.h"
#include "peoples.h"
#include "buildings.h"
#include "galaxy/drainage.h"
#include "towns.h"
#include "galaxy/planet_features.h"
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

inline bool wetS(const SurfaceSample& s) { return s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5; }
inline double arcM(const Vec3& a, const Vec3& b, double Rm) { return std::acos(clampd(dot(a, b), -1, 1)) * Rm; }
// a point `eastM` and `northM` off a unit vector on the tangent plane's great circles (the plan's and the roads' frame)
inline Vec3 offsetUnit(const Vec3& u, double eastM, double northM, double Rm) {
    Vec3 pole(0, 0, 1), east = cross(pole, u);
    if (length(east) < 1e-9) east = Vec3(0, 1, 0);
    east = normalize(east);
    const Vec3 north = cross(u, east);
    const double d = std::sqrt(eastM * eastM + northM * northM);
    if (d < 1e-9) return u;
    const Vec3 dir = (east * eastM + north * northM) / d;
    return normalize(u * std::cos(d / Rm) + dir * std::sin(d / Rm));
}
inline Vec3 slerpUnit(const Vec3& a, const Vec3& b, double t) {
    const double c = clampd(dot(a, b), -1, 1), w = std::acos(c);
    if (w < 1e-9) return a;
    const double s = std::sin(w);
    return normalize(a * (std::sin((1 - t) * w) / s) + b * (std::sin(t * w) / s));
}
inline Vec3 toward(const Vec3& from, const Vec3& to, double m, double Rm) {
    const double w = std::acos(clampd(dot(from, to), -1, 1));
    if (w < 1e-12) return from;
    return slerpUnit(from, to, m / Rm / w);
}
// the bearing (from north, clockwise) of `to` seen from `from`
inline double bearingOf(const Vec3& from, const Vec3& to) {
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
inline void addPt(GroundEdit& e, double x, double z) { e.pts.push_back((float)x); e.pts.push_back((float)z); }

// ---- the builder -------------------------------------------------------------------------------------------------------------

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
// the share of a land's cells holding a work where the countryside's density is full, by the age: a prehistoric people's flint pits, barrows
// and stones thinner than a later people's quarries, mines, cemeteries, wrecks and wind farms; a futuristic people's land is wild but round
// its great places (`planCountryside`), the far future's none (its works are the world's)
const double LAND_RATE[AGE_COUNT] = {0.34, 0.34, 0.36, 0.36, 0.5, 0};
constexpr double LAND_REACH_MAX = 1700;   // the farthest a land's work reaches from its middle (a wind farm's rows)
inline double landCellLat(const BodyGen& g) { return LAND_CELL_M / (g.R * 1000.0); }
inline LandCell landCellAt(const BodyGen& g, int gLat, int gLon) {
    LandCell L; const double dLat = landCellLat(g); const int n = landCellsAround(g, gLat);
    L.gLat = gLat; L.gl = ((gLon % n) + n) % n;
    L.lat0 = gLat * dLat; L.lat1 = L.lat0 + dLat; L.latMid = L.lat0 + 0.5 * dLat;
    L.halfLon = 0.5 * TAU / n; L.lonMid = std::remainder((L.gl + 0.5) * TAU / n, TAU);
    L.mid = StarSystem::bodyFromLatLon(L.latMid, L.lonMid);
    L.h = hash2i(gLat, L.gl, g.seed ^ 0xB418C311ULL);
    return L;
}
