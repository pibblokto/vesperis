// C-09 (2026-10-03): roads that still go somewhere (see roads.h).
#include "roads.h"
#include "drainage.h"
#include "core/rng.h"
#include "core/noise.h"
#include "core/parallel.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace {
// a local tangent frame at a point (the site's own mapping: azimuthal equidistant, x east, z north, metres)
struct Frame {
    Vec3 up, east, north; double R;
    Frame(const Vec3& u, double radiusM) : up(u), R(radiusM) {
        Vec3 pole(0, 0, 1);
        east = normalize(cross(pole, up));
        if (length(cross(pole, up)) < 1e-6) east = Vec3(0, 1, 0);
        north = cross(up, east);
    }
    Vec3 unitAt(double x, double z) const {
        double d = std::sqrt(x * x + z * z);
        if (d < 1e-9) return up;
        double ang = d / R;
        Vec3 dir = (east * x + north * z) / d;
        return normalize(up * std::cos(ang) + dir * std::sin(ang));
    }
    void localAt(const Vec3& u, double& x, double& z) const {
        double c = clampd(dot(u, up), -1, 1), ang = std::acos(c);
        Vec3 tang = u - up * c; double tl = length(tang);
        if (tl < 1e-12 || ang < 1e-12) { x = 0; z = 0; return; }
        tang = tang / tl;
        x = R * ang * dot(tang, east); z = R * ang * dot(tang, north);
    }
};
uint64_t cellKey(int gLat, int gLon) { return ((uint64_t)(uint32_t)(gLat + 0x100000) << 32) | (uint32_t)gLon; }
bool cellBefore(const RoadNode& a, const RoadNode& b) { return a.gLat != b.gLat ? a.gLat < b.gLat : a.gLon < b.gLon; }
int wrapLon(const BodyGen& g, int gLat, int gLon) { int n = std::max(1, ruinCellsAround(g, gLat)); return ((gLon % n) + n) % n; }

}   // namespace

// the sited settlement of a cell, if any
bool roadNodeOfCell(const BodyGen& g, int gLat, int gLon, RoadNode& out) {
    RuinSpec sp;
    if (!ruinOfCell(g, gLat, gLon, sp, false) || sp.kind != RK_SETTLEMENT) return false;
    { DrainageOff off; if (!ruinSiteOk(g, sp)) return false; }
    out.gLat = gLat; out.gLon = wrapLon(g, gLat, gLon);
    out.lat = sp.lat; out.lon = sp.lon; out.size = sp.size; out.heading = sp.heading; out.sclass = sp.sclass; out.plan = sp.plan; out.seed = sp.seed;
    out.people = sp.people;   // C-13
    out.unit = StarSystem::bodyFromLatLon(sp.lat, sp.lon);
    return true;
}
int roadPeopleOf(const RoadNode& a, const RoadNode& b) {   // C-13
    if (a.people == b.people) return a.people;
    return settlementRank(b.sclass) > settlementRank(a.sclass) ? b.people : a.people;
}
uint64_t roadCellKey(const BodyGen& g, int gLat, int gLon) { return cellKey(gLat, wrapLon(g, gLat, gLon)); }

namespace {
// where a road meets a settlement: the end of its street's axis that faces the other settlement (a street plan, a grid's
// four), else the edge toward it; a town's gate stands at 1.04 radii
void attachPoint(const RoadNode& n, double cx, double cz, double tx, double tz, double& x, double& z) {
    double reach = n.sclass == SC_TOWN ? n.size * 1.04 : n.size;
    if (n.plan == SP_STREET || n.plan == SP_GRID) {
        int ends = n.plan == SP_GRID ? 4 : 2;
        double best = -2, bx = tx, bz = tz;
        for (int k = 0; k < ends; k++) {
            double h = n.heading + k * (TAU / ends), dx = std::sin(h), dz = std::cos(h), d = dx * tx + dz * tz;
            if (d > best) { best = d; bx = dx; bz = dz; }
        }
        x = cx + bx * reach; z = cz + bz * reach;
    } else { x = cx + tx * reach; z = cz + tz * reach; }
}

// the walk from one edge to the other: the gentlest of five headings each step
bool walk(const BodyGen& g, const Frame& F, double ax, double az, double gx, double gz, Road& out) {
    DrainageOff off;
    auto sample = [&](double x, double z) { return sampleSurface(g, F.unitAt(x, z), 32.0); };
    auto wet = [](const SurfaceSample& s) { return s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5; };
    static const double OFF[5] = {0, 22 * DEG, -22 * DEG, 45 * DEG, -45 * DEG};
    double px = ax, pz = az; SurfaceSample ps = sample(px, pz);
    double prevH = std::atan2(gx - px, gz - pz);
    std::vector<std::pair<double, double>> way; way.push_back({px, pz});
    int steps = 0;
    for (;; steps++) {
        if (steps >= ROAD_WALK_MAX) return false;
        double dx = gx - px, dz = gz - pz, dist = std::sqrt(dx * dx + dz * dz);
        if (dist <= 1.5 * ROAD_STEP_M) break;
        double theta = std::atan2(dx, dz);
        int bestK = -1; double bestCost = 1e30, bestH = 0, bx = 0, bz = 0, bestGrade = 0; SurfaceSample bestS;
        for (int k = 0; k < 5; k++) {
            double h = theta + OFF[k], qx = px + std::sin(h) * ROAD_STEP_M, qz = pz + std::cos(h) * ROAD_STEP_M;
            SurfaceSample qs = sample(qx, qz);
            if (wet(qs)) continue;
            double grade = std::fabs(qs.height - ps.height) / ROAD_STEP_M, turn = std::remainder(h - prevH, TAU) / (45 * DEG), off = OFF[k] / (45 * DEG);
            double cost = grade * grade * 60.0 + off * off + turn * turn * 0.5;
            if (cost < bestCost) { bestCost = cost; bestK = k; bestH = h; bx = qx; bz = qz; bestS = qs; bestGrade = grade; }
            if (k == 0 && grade < 0.06) break;   // the straight step over easy ground: no other heading is tried
        }
        if (bestK < 0) return false;   // every heading steps into water
        if (bestK != 0) out.bends++;
        out.maxGrade = std::max(out.maxGrade, bestGrade);
        px = bx; pz = bz; ps = bestS; prevH = bestH;
        way.push_back({px, pz});
    }
    way.push_back({gx, gz});
    // the kinks of the five headings taken out: every inner point to the mean of itself and its neighbours, the ends kept;
    // then a gentle waver across the way (a few metres over a few hundred), so a road over a plain is not a ruled line
    std::vector<std::pair<double, double>> sm = way;
    for (size_t i = 1; i + 1 < way.size(); i++) sm[i] = {(way[i - 1].first + way[i].first + way[i + 1].first) / 3.0, (way[i - 1].second + way[i].second + way[i + 1].second) / 3.0};
    for (size_t i = 1; i + 1 < way.size(); i++) {
        double dx = sm[i + 1].first - sm[i - 1].first, dz = sm[i + 1].second - sm[i - 1].second, L = std::hypot(dx, dz);
        if (L < 1e-6) continue;
        double w = ROAD_WAVER_M * gnoise2(i * ROAD_STEP_M / 420.0, 0.5, out.id) * std::min(1.0, std::min(i, way.size() - 1 - i) / 3.0);   // nothing at the ends
        sm[i].first += -dz / L * w; sm[i].second += dx / L * w;
    }
    out.pts.clear(); out.lengthM = 0;
    for (size_t i = 0; i < sm.size(); i++) {
        out.pts.push_back(F.unitAt(sm[i].first, sm[i].second));
        if (i) out.lengthM += std::hypot(sm[i].first - sm[i - 1].first, sm[i].second - sm[i - 1].second);
    }
    out.straightM = std::hypot(gx - ax, gz - az);
    return true;
}
}   // namespace

uint64_t roadIdOf(const BodyGen& g, int gLatA, int gLonA, int gLatB, int gLonB) {
    uint64_t ka = hash2i(gLatA, wrapLon(g, gLatA, gLonA), g.seed ^ 0xC09ULL), kb = hash2i(gLatB, wrapLon(g, gLatB, gLonB), g.seed ^ 0xC09ULL);
    if (ka > kb) std::swap(ka, kb);
    return mix64(ka ^ (kb * 0x9E3779B97F4A7C15ULL) ^ 0xC09DULL);
}

double roadDistanceM(const BodyGen& g, const Vec3& a, const Vec3& b) { return std::acos(clampd(dot(a, b), -1, 1)) * g.R * 1000.0; }

bool roadJoined(const BodyGen& g, const RoadNode& a, const RoadNode& b, const std::vector<RoadNode>& others) {
    if (a.gLat == b.gLat && a.gLon == b.gLon) return false;
    if (std::abs(a.gLat - b.gLat) > ROAD_REACH) return false;
    double d = roadDistanceM(g, a.unit, b.unit);
    if (d > ROAD_MAX_M || d < a.size + b.size + 20) return false;
    for (const RoadNode& c : others) {
        if ((c.gLat == a.gLat && c.gLon == a.gLon) || (c.gLat == b.gLat && c.gLon == b.gLon)) continue;
        if (roadDistanceM(g, c.unit, a.unit) < d && roadDistanceM(g, c.unit, b.unit) < d) return false;   // a third closer to both: the way goes through it
    }
    return true;
}

bool roadWay(const BodyGen& g, const Culture& c, const RoadNode& a0, const RoadNode& b0, Road& out) {
    out = Road();
    const RoadNode& a = cellBefore(a0, b0) ? a0 : b0;
    const RoadNode& b = cellBefore(a0, b0) ? b0 : a0;
    out.a = a; out.b = b; out.id = roadIdOf(g, a.gLat, a.gLon, b.gLat, b.gLon);
    out.people = roadPeopleOf(a, b);   // C-13
    out.halfWidth = c.roadHalf + 0.5 * (std::max(settlementRank(a.sclass), settlementRank(b.sclass)) - 1);
    Frame F(a.unit, g.R * 1000.0);
    double bx, bz; F.localAt(b.unit, bx, bz);
    double d = std::sqrt(bx * bx + bz * bz);
    if (d < 1e-6) return false;
    double tx = bx / d, tz = bz / d;
    double ax, az, gx, gz;
    attachPoint(a, 0, 0, tx, tz, ax, az);
    attachPoint(b, bx, bz, -tx, -tz, gx, gz);
    return walk(g, F, ax, az, gx, gz, out);
}

bool RoadNodeCache::node(int gLat, int gLon, RoadNode& out) {
    uint64_t key = cellKey(gLat, wrapLon(*g, gLat, gLon));
    auto it = cells.find(key);
    if (it == cells.end()) { RoadNode n; bool has = roadNodeOfCell(*g, gLat, gLon, n); it = cells.emplace(key, std::make_pair(has, n)).first; }
    if (it->second.first) out = it->second.second;
    return it->second.first;
}
void RoadNodeCache::block(int gLat, int gLon, std::vector<RoadNode>& out) {
    for (int dl = -ROAD_REACH; dl <= ROAD_REACH; dl++)
        for (int dn = -ROAD_REACH; dn <= ROAD_REACH; dn++) { RoadNode n; if (node(gLat + dl, gLon + dn, n)) out.push_back(n); }
}
void RoadNodeCache::joinedTo(const RoadNode& a, std::vector<RoadNode>& out) {
    std::vector<RoadNode> blockA; block(a.gLat, a.gLon, blockA);
    for (const RoadNode& b : blockA) {
        if (b.gLat == a.gLat && b.gLon == a.gLon) continue;
        std::vector<RoadNode> others = blockA; block(b.gLat, b.gLon, others);
        if (roadJoined(*g, a, b, others)) out.push_back(b);
    }
}
void RoadNodeCache::pairsOf(int gLat, int gLon, std::vector<RoadPair>& out) {
    RoadNode a; if (!node(gLat, gLon, a)) return;
    std::vector<RoadNode> joined; joinedTo(a, joined);
    for (const RoadNode& b : joined) {
        RoadPair p; p.a = cellBefore(a, b) ? a : b; p.b = cellBefore(a, b) ? b : a; p.id = roadIdOf(*g, p.a.gLat, p.a.gLon, p.b.gLat, p.b.gLon);
        out.push_back(p);
    }
}

void roadsOfCell(const BodyGen& g, int gLat, int gLon, std::vector<Road>& out) {
    out.clear();
    if (!worldHadCivilisation(g)) return;
    RoadNodeCache nc; nc.reset(&g);
    std::vector<RoadPair> pairs; nc.pairsOf(gLat, gLon, pairs);
    Culture cs[2] = {cultureOf(g, 0), peoplesOf(g) > 1 ? cultureOf(g, 1) : Culture()};   // C-13: each road as its people built
    for (const RoadPair& p : pairs) { Road r; if (roadWay(g, cs[roadPeopleOf(p.a, p.b)], p.a, p.b, r)) out.push_back(r); }
    std::sort(out.begin(), out.end(), [](const Road& x, const Road& y) { return x.id < y.id; });
}

void roadsNear(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<Road>& out, bool parallel) {
    out.clear();
    if (!worldHadCivilisation(g)) return;
    double lat, lon; StarSystem::latLonFromBody(unit, lat, lon);
    int gLat0, gLon0; ruinCellOf(g, lat, lon, gLat0, gLon0);
    double cellM = ruinCellLat(g) * g.R * 1000.0;
    int cellsR = (int)std::ceil((radiusM + ROAD_MAX_M) / cellM) + 1;
    RoadNodeCache nc; nc.reset(&g);
    Culture cs[2] = {cultureOf(g, 0), peoplesOf(g) > 1 ? cultureOf(g, 1) : Culture()};   // C-13: each road as its people built
    // the nodes whose roads can reach the disc, and their joined pairs, each once
    std::vector<RoadNode> nodes;
    for (int dl = -cellsR; dl <= cellsR; dl++) {
        int nLon = std::max(1, ruinCellsAround(g, gLat0 + dl));
        int cellsLon = std::min(nLon / 2, cellsR);   // the grid's lon cells are the same metres across as its lat cells
        for (int dn = -cellsLon; dn <= cellsLon; dn++) { RoadNode n; if (nc.node(gLat0 + dl, gLon0 + dn, n)) nodes.push_back(n); }
    }
    struct Pair { RoadNode a, b; };
    std::vector<Pair> pairs;
    for (const RoadNode& a : nodes) {
        if (roadDistanceM(g, a.unit, unit) > radiusM + ROAD_MAX_M) continue;
        std::vector<RoadNode> joined; nc.joinedTo(a, joined);
        for (const RoadNode& b : joined) {
            if (!cellBefore(a, b)) continue;   // listed from its first cell
            // the straight line's nearest approach to the centre, with room for the bends
            double da = roadDistanceM(g, a.unit, unit), db = roadDistanceM(g, b.unit, unit), ab = roadDistanceM(g, a.unit, b.unit);
            double near = std::min(da, db);
            if (da < ab && db < ab) { double s = (da + db + ab) * 0.5; double area = std::sqrt(std::max(0.0, s * (s - da) * (s - db) * (s - ab))); near = std::min(near, 2 * area / ab); }
            if (near > radiusM + 0.35 * ab + 100) continue;
            pairs.push_back({a, b});
        }
    }
    std::vector<Road> roads(pairs.size());
    std::vector<char> ok(pairs.size(), 0);
    auto walkRange = [&](int i0, int i1) { for (int i = i0; i < i1; i++) ok[i] = roadWay(g, cs[roadPeopleOf(pairs[i].a, pairs[i].b)], pairs[i].a, pairs[i].b, roads[i]) ? 1 : 0; };
    if (parallel && pairs.size() > 8) parallelFor((int)pairs.size(), 4, walkRange); else walkRange(0, (int)pairs.size());
    for (size_t i = 0; i < pairs.size(); i++) {
        if (!ok[i]) continue;
        bool in = false;
        for (const Vec3& p : roads[i].pts) if (roadDistanceM(g, p, unit) <= radiusM + roads[i].halfWidth + 50) { in = true; break; }
        if (in) out.push_back(roads[i]);
    }
    std::sort(out.begin(), out.end(), [](const Road& x, const Road& y) { return x.id < y.id; });
}

double roadLeft(uint64_t id, double alongM, double wear) {
    double n = 0.5 + 0.7 * gnoise2(alongM / 70.0, 0.37, id) + 0.45 * gnoise2(alongM / 23.0, 0.61, id ^ 0x55ULL);
    return smoothstep(wear - 0.08, wear + 0.08, n);
}
