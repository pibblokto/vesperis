// E-09: B-418's works of the land, a cell's each (`WB`'s members): the settlement it serves, its summit, a quarry, a mine, a
// necropolis, a henge, a wreck, a wind farm, a shrine, a lighthouse, a fort, and `landWork`, which draws a cell's kind and sites it
#include "works_internal.h"

// ---- B-418: the works of the land ---------------------------------------------------------------------------------------------------
namespace {
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
        const double key = settlementRank(sclass) * 1e7 - arcLength(u, L.mid, Rm);
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
