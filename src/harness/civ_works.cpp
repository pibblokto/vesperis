// E-03: the works (T-03): `works`, its map and its gallery, its checks
#include "harness/harness.h"
#include "harness/test_worlds.h"

// the towns' checks. The towns: each age world's capital, its first city and a place it left early, a town and a village of its
// countryside; every street pattern in each age of streets (a town laid out at one cell of the medieval world); the harbours of three ages on a
// coast. Each: no building across a street (a gate stands on its street, a maglev's pylons over everything); the streets walkable (the explorer
// on every street's line clear of every collider but where something fell across it, and the web joined); B-414: one age a world (every town
// all of its people's age, a place left before that age not on the ground, one left early in it weathered longer); the districts and works by the
// age (a modern capital's towers, ring road and suburbs, an industrial one's mills and station, a medieval one's wall and gates, a harbour's
// quay, dock and container port); the ground's marks (a street's vertex paved, a field's toned)
// the works' checks over the age worlds and the ages' scene worlds, a world at a time (its plan, histories and works in the caches while it
// is checked): a dam across its valley holding its reservoir (or its old bed), a quarry in the rock its town was built of, a battlefield between
// the two sides of its war, the scanner hearing them, the works the same twice and clear of the cities' cores, every kind somewhere, their forms
// sound
// the land's works of a people round its first `nPlaces` places (within `radiusM` of each) and round `nPoints` points of its land (drawn
// from the plan's grid), each once
void landWorksOf(const BodyGen& g, int people, int nPlaces, double radiusM, int nPoints, std::vector<std::shared_ptr<const Work>>& out) {
    out.clear();
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    if (people >= P->peoples) return;
    const PeoplePlan& pp = P->plans[people];
    std::set<uint64_t> seen;
    auto take = [&](const Vec3& u) {
        std::vector<std::shared_ptr<const Work>> near; worksAround(g, u, radiusM, near);
        for (std::shared_ptr<const Work>& w : near) if (w->land && w->people == people && seen.insert(w->id).second) out.push_back(std::move(w));
    };
    for (int i = 0; i < (int)pp.places.size() && i < nPlaces; i++) take(pp.places[i].unit);
    std::vector<int> land; for (int c = 0; c < P->grid.cells; c++) if (pp.territory[c] && P->grid.land[c]) land.push_back(c);
    Rng rng(g.seed ^ 0xB418ULL ^ (uint64_t)people);
    for (int k = 0; k < nPoints && !land.empty(); k++) take(P->grid.unitOf(land[rng.irange((int)land.size())]));
}

void worksChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    const double t0 = nowSec();
    int dams = 0, damBad = 0, damWet = 0, damBeds = 0, damBreached = 0; std::string damWhy;
    int quarries = 0, qBad = 0; std::string qWhy;
    int landN = 0, landOut = 0, landTouch = 0, landClose = 0, landAsked = 0, landSame = 0, covPts = 0, covHeard = 0, covPlaces = 0, covPlacesHeard = 0, covSum = 0;
    std::string landWhy, covWhy;
    int fields = 0, fBad = 0, fJoint = 0; std::string fWhy;
    int heard = 0, hMissed = 0, unheard = 0; std::string hWhy;
    int nWorlds = 0, same = 0, inTown = 0, nanForms = 0, pieces = 0, total = 0; bool kinds[WO_COUNT] = {}; std::string sWhy;
    double buildMs = 0, worstMs = 0; std::string worstName;
    std::set<std::string> seen;
    // the age and scene worlds, and the nearest living world whose dams hold their water (R-413: the modern test world a desert since;
    // Shioleesein I-a, 20 ly, a modern people's two)
    std::vector<AgeWorld> worlds(AGE_WORLDS, AGE_WORLDS + AGE_COUNT);
    worlds.insert(worlds.end(), AGE_SCENE_WORLDS, AGE_SCENE_WORLDS + AGE_COUNT);
    worlds.push_back(AgeWorld{196, 66, 7, 0});
    worlds.push_back(TEST_DRIFTERS[1]);   // a drifter's modern peoples (their works of the end: the shelters, the domes and their glasshouses)
    for (const AgeWorld& aw : worlds) {
            if (!seen.insert(fmt("%lld %lld %d", (long long)aw.sx, (long long)aw.sz, aw.body)).second) continue;
            Star st; if (!placeInSector(aw.sx, aw.sy, aw.sz, st, true)) continue;
            StarSystem sys; sys.generate(st);
            if (aw.body >= (int)sys.bodies.size()) continue;
            const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
            if (!worldHadCivilisation(g)) continue;
            const std::string wname = sys.bodies[aw.body].name;
            const double Rm = g.R * 1000.0;
            std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
            std::shared_ptr<const History> Hs[2] = {historyOfWorld(g, 0), peoplesOf(g) > 1 ? historyOfWorld(g, 1) : nullptr};
            const Culture cus[2] = {cultureOf(g, 0), peoplesOf(g) > 1 ? cultureOf(g, 1) : cultureOf(g, 0)};
            const double tb = nowSec();
            std::shared_ptr<const WorldWorks> W = worldWorksOf(g);
            const double ms = (nowSec() - tb) * 1000; buildMs += ms;
            if (ms > worstMs) { worstMs = ms; worstName = wname; }
            nWorlds++;
            DrainageOff off;
            // the rest of every work: its kind, out of a city's core, its forms sound
            auto every = [&](const Work& x) {
                total++; kinds[x.kind] = true;
                if (planCoreAt(*P, x.unit, -50.0)) { inTown++; if (sWhy.size() < 300) sWhy += fmt("a %s in a city's core; ", WORK_KIND_NAMES[x.kind]); }
                RuinSpec sp; workSpec(g, x, sp);
                std::vector<Form> fs; ruinElements(sp, cus[x.people], 0, fs);
                pieces += (int)sp.buildings.size();
                for (const Form& f : fs) if (!std::isfinite(f.x) || !std::isfinite(f.z) || !std::isfinite(f.y0) || !std::isfinite(f.y1) || f.y1 < f.y0 - 1e-6) { nanForms++; break; }
            };
            for (const Work& x : W->works) {
                auto at = [&](double e, double n) { double la, lo; StarSystem::latLonFromBody(x.unit, la, lo); return sampleSurface(g, StarSystem::bodyFromLatLon(la + n / Rm, lo + e / (Rm * std::max(std::cos(la), 0.05))), 64.0); };
                if (x.kind == WO_DAM) {   // its wall's ends on the valley's sides, its water under its crest, its reservoir held behind it and drawn
                    dams++;
                    const double ax = std::sin(x.axis), az = std::cos(x.axis);
                    const double endA = at(ax * x.length * 0.5, az * x.length * 0.5).height, endB = at(-ax * x.length * 0.5, -az * x.length * 0.5).height;
                    std::string e;
                    if (std::min(endA, endB) < x.crest - 2.5) e += fmt("an end at %.0f under the crest %.0f; ", std::min(endA, endB), x.crest);
                    if (x.crest - x.floor < 8) e += fmt("only %.0f m over its floor; ", x.crest - x.floor);
                    if (x.level < x.floor + 2 || x.level > x.crest - 1) e += fmt("the water at %.1f between %.0f and %.0f; ", x.level, x.floor, x.crest);
                    const GroundEdit* we = nullptr; for (const GroundEdit& ge : x.edits) if (ge.kind == GE_WATER) we = &ge;
                    if (!we) e += "no reservoir; ";
                    else {
                        int cells = 0, edge = 0, down = 0, lowest = -1; double lowH = 1e18;
                        const double hx = std::sin(x.heading), hz = std::cos(x.heading);
                        for (int j = 0; j < we->nz; j++) for (int i = 0; i < we->nx; i++) {
                            if (we->mask[(size_t)j * we->nx + i] != 1) continue;
                            cells++;
                            if (i == 0 || j == 0 || i == we->nx - 1 || j == we->nz - 1) edge++;
                            const double cx = we->gx0 + (i + 0.5) * we->cell, cz = we->gz0 + (j + 0.5) * we->cell;
                            if (cx * hx + cz * hz < -60) down++;
                            if ((i + j) % 3 == 0) { const double h = at(cx, cz).height; if (h < lowH) { lowH = h; lowest = j * we->nx + i; } }
                        }
                        if (cells < 20) e += fmt("a reservoir of %d cells; ", cells);
                        if (edge) e += fmt("%d cells at its grid's edge (not held); ", edge);
                        if (down) e += fmt("%d cells below the wall; ", down);
                        if (lowest >= 0) {   // the water as the surface draws it, or its old bed
                            SurfaceSite site; site.init(&sys, aw.body, x.lat, x.lon, 0.0);
                            site.addWorkEdits(x, 0, 0, 500, 0.5);
                            const TerrainVertex v = site.sampleAt(we->gx0 + (lowest % we->nx + 0.5) * we->cell, we->gz0 + (lowest / we->nx + 0.5) * we->cell, 16.0);
                            const bool drained = we->tone == ET_DARK;
                            if (drained) damBreached++;
                            if (g.type == PT_DESERT || drained) { if (v.water > -1e8f && v.h < v.water) e += "water on a dead or drained bed; "; else damBeds++; }
                            else if (!(v.water > -1e8f && std::fabs(v.water - x.level) < 0.01 && v.h < v.water)) e += fmt("no water drawn at its lowest (h %.1f, water %.1f); ", v.h, v.water);
                            else damWet++;
                        }
                    }
                    if (!e.empty()) { damBad++; if (damWhy.size() < 300) damWhy += wname + "'s dam: " + e; }
                }
                if (x.kind == WO_BATTLEFIELD) {   // a battle of its war in its years, between the two sides' places on the line between them
                    fields++;
                    const History* H = Hs[x.people].get();
                    std::string e;
                    if (!H || x.event < 0 || x.event >= (int)H->events.size() || H->events[x.event].kind != HE_BATTLE) e += "no battle; ";
                    else {
                        const HistoryEvent& ev = H->events[x.event]; const HistoryWar& war = H->wars[ev.thing];
                        if (ev.thing != x.war || x.year != ev.year || ev.year < war.start || ev.year > war.end) e += fmt("year %d outside its war's %d..%d; ", x.year, war.start, war.end);
                        if (ev.other >= 0) fJoint++;
                    }
                    const Vec3 A = x.sideA, B = x.sideB, X = x.unit;
                    const double sep = std::acos(clampd(dot(A, B), -1, 1));
                    const Vec3 nrm = normalize(cross(A, B));
                    const double offM = std::asin(clampd(dot(X, nrm), -1, 1)) * Rm;
                    const Vec3 Xp = normalize(X - nrm * dot(X, nrm));
                    const double ta = std::acos(clampd(dot(A, Xp), -1, 1)), tbb = std::acos(clampd(dot(B, Xp), -1, 1)), t = ta / std::max(sep, 1e-12);
                    if (ta + tbb > sep * 1.001 + 1e-9 || t < 0 || t > 1) e += fmt("not between its sides (at %.2f of the way); ", t);
                    if (std::fabs(offM) > std::max(2500.0, 0.25 * sep * Rm)) e += fmt("%.0f m off the line between them; ", offM);
                    if (!e.empty()) { fBad++; if (fWhy.size() < 300) fWhy += wname + "'s battlefield: " + e; }
                }
                every(x);
            }
            // the scanner: a seventh of the works asked for from their middles
            if (!((sightKindsOf(g) >> LM_WORK) & 1)) { hMissed++; hWhy += wname + ": no WORKS mode; "; }
            else {
                int k = 0;
                for (const Work& x : W->works) {
                    if (k++ % 7) continue;
                    std::vector<Landmark> L; worksHeardNear(g, x.unit, 40000, L);
                    bool found = false; for (const Landmark& l : L) if (l.id == x.id && l.kind == LM_WORK && l.sub == x.kind) found = true;
                    if (found != x.heard) { hMissed++; if (hWhy.size() < 200) hWhy += fmt("%s %s; ", WORK_KIND_NAMES[x.kind], x.heard ? "not heard" : "heard"); }
                    else if (found) heard++; else unheard++;
                }
            }
            // the land's works round each people's first places and at points of its land: a quarry's face in the rock of a stone-built
            // people, a pit cut under the ground; each inside its cell with its reach (no two touch), clear of the settlements, the cities' cores
            // and the world's works; the same built again; heard from their middles
            for (int p = 0; p < peoplesOf(g); p++) {
                std::vector<std::shared_ptr<const Work>> lw; landWorksOf(g, p, 6, 20000, 10, lw);
                for (size_t i = 0; i < lw.size(); i++) {
                    const Work& x = *lw[i];
                    landN++; every(x);
                    auto at = [&](double e, double n) { double la, lo; StarSystem::latLonFromBody(x.unit, la, lo); return sampleSurface(g, StarSystem::bodyFromLatLon(la + n / Rm, lo + e / (Rm * std::max(std::cos(la), 0.05))), 64.0); };
                    if (x.kind == WO_QUARRY && x.style != 0) {
                        quarries++;
                        const SurfaceSample sm = at(0, 0);
                        std::string e;
                        const GroundEdit* ce = x.edits.empty() ? nullptr : &x.edits[0];
                        if (!ce) e += "no cut; ";
                        else if (ce->kind == GE_CUT) {
                            if (matFamily(sm.material) != FAM_ROCK) e += fmt("a face in %s; ", MATERIAL_NAMES[sm.material]);
                            if (cus[x.people].family != 0) e += "a face cut by a people that built of earth; ";
                            const double back = at(std::sin(x.heading) * ce->hd * 0.85, std::cos(x.heading) * ce->hd * 0.85).height; if (back < ce->level + 2) e += fmt("its face %.1f m over its floor; ", back - ce->level);
                        }
                        else if (ce->kind == GE_PIT && ce->level > sm.height - 2) e += fmt("its pit's floor at %.0f, the ground %.0f; ", ce->level, sm.height);
                        if (!e.empty()) { qBad++; if (qWhy.size() < 300) qWhy += wname + "'s quarry: " + e; }
                    }
                    // inside its cell with its reach and 60 m
                    {
                        int gL, gN; landCellOf(g, x.lat, x.lon, gL, gN);
                        const double dLat = LAND_CELL_M / Rm, lat0 = gL * dLat; const int n = landCellsAround(g, gL);
                        const double lonMid = std::remainder((gN + 0.5) * TAU / n, TAU), m = (x.radiusM + 59) / Rm;
                        if (x.lat < lat0 + m || x.lat > lat0 + dLat - m || std::fabs(std::remainder(x.lon - lonMid, TAU)) > PI / n - m / std::max(std::cos(x.lat), 0.05)) {
                            landOut++; if (landWhy.size() < 300) landWhy += fmt("%s's %s over its cell's edge; ", wname.c_str(), WORK_KIND_NAMES[x.kind]);
                        }
                        if (i % 3 == 0) {   // built again, the same
                            landAsked++;
                            std::shared_ptr<const Work> again = buildLandWork(g, gL, gN);
                            if (again && again->id == x.id && again->kind == x.kind && length(again->unit - x.unit) < 1e-12 && again->edits.size() == x.edits.size()) landSame++;
                            else if (landWhy.size() < 300) landWhy += fmt("%s's %s not the same built again; ", wname.c_str(), WORK_KIND_NAMES[x.kind]);
                        }
                    }
                    for (size_t j = 0; j < i; j++) if (std::acos(clampd(dot(lw[j]->unit, x.unit), -1, 1)) * Rm < lw[j]->radiusM + x.radiusM) { landTouch++; if (landWhy.size() < 300) landWhy += fmt("%s's %s and %s touch; ", wname.c_str(), WORK_KIND_NAMES[x.kind], WORK_KIND_NAMES[lw[j]->kind]); }
                    // clear of the settlements of the 2 km cells (their radius and the margin), the cities' cores and the world's works
                    {
                        bool close = planCoreAt(*P, x.unit, x.radiusM + 149) != nullptr;
                        for (const Work& y : W->works) if (std::acos(clampd(dot(y.unit, x.unit), -1, 1)) * Rm < x.radiusM + y.radiusM + 39) close = true;
                        const double dLatR = ruinCellLat(g); const int reach = 1 + (int)((x.radiusM + 400) / 2000.0), row0 = (int)std::floor(x.lat / dLatR);
                        for (int dl = -reach; dl <= reach && !close; dl++) {
                            const int row = row0 + dl; const double latc = (row + 0.5) * dLatR;
                            if (std::fabs(latc) > PI / 2 - dLatR) continue;
                            const int nLon = std::max(1, ruinCellsAround(g, row)); const int col0 = (int)std::floor(x.lon / (dLatR / std::max(std::cos(latc), 0.05)));
                            for (int dd = -reach; dd <= reach && !close; dd++) {
                                RuinSpec q; if (!ruinOfCell(g, row, (((col0 + dd) % nLon) + nLon) % nLon, q, false) || q.sclass == SC_WORKS) continue;
                                if (std::acos(clampd(dot(StarSystem::bodyFromLatLon(q.lat, q.lon), x.unit), -1, 1)) * Rm < x.radiusM + q.size + 59) close = true;
                            }
                        }
                        if (close) { landClose++; if (landWhy.size() < 300) landWhy += fmt("%s's %s on a settlement, a core or a work; ", wname.c_str(), WORK_KIND_NAMES[x.kind]); }
                    }
                    if (i % 5 == 0) {   // heard from its middle
                        std::vector<Landmark> L; worksHeardNear(g, x.unit, 40000, L);
                        bool found = false; for (const Landmark& l : L) if (l.id == x.id && l.kind == LM_WORK && l.sub == x.kind) found = true;
                        if (!found) { hMissed++; if (hWhy.size() < 200) hWhy += fmt("the land's %s not heard; ", WORK_KIND_NAMES[x.kind]); } else heard++;
                    }
                }
                // the scanner from the land: a work heard within 40 km of points of a prehistoric to a modern people's land and of its places
                const PeoplePlan& pp = P->plans[p];
                if (pp.age > AGE_MODERN) continue;
                std::vector<int> land; for (int c = 0; c < P->grid.cells; c++) if (pp.territory[c] && P->grid.land[c]) land.push_back(c);
                Rng rng(g.seed ^ 0xB4180C0ULL ^ (uint64_t)p);
                int pts = 0, ph = 0;
                for (int k = 0; k < 24 && !land.empty(); k++) {
                    std::vector<Landmark> L; worksHeardNear(g, P->grid.unitOf(land[rng.irange((int)land.size())]), 40000, L);
                    covPts++; pts++; covSum += (int)L.size(); if (!L.empty()) { covHeard++; ph++; }
                }
                int pl = 0, plh = 0;
                for (int i = 0; i < (int)pp.places.size() && i < 12; i++) { std::vector<Landmark> L; worksHeardNear(g, pp.places[i].unit, 40000, L); covPlaces++; pl++; if (!L.empty()) { covPlacesHeard++; plh++; } }
                covWhy += fmt("%s%s %s: %d of %d points, %d of %d places", covWhy.empty() ? "" : "; ", wname.c_str(), AGE_NAMES[pp.age], ph, pts, plh, pl);
            }
            // built again, the same
            std::shared_ptr<WorldWorks> again = buildWorldWorks(g);
            bool eq = again->works.size() == W->works.size();
            for (size_t i = 0; eq && i < again->works.size(); i++) eq = again->works[i].id == W->works[i].id && again->works[i].kind == W->works[i].kind && length(again->works[i].unit - W->works[i].unit) < 1e-12;
            if (eq) same++; else sWhy += wname + " differs built again; ";
        }
    check("works: a dam across a valley holding a reservoir (T-03)", dams >= 3 && damBad == 0 && damWet >= 1,
          fmt("%d dams over the test worlds: %d with water drawn behind them, %d old beds (%d breached); %s", dams, damWet, damBeds, damBreached, damBad ? damWhy.c_str() : "every wall's ends on the valley's sides, its water under its crest, its reservoir held behind it"));
    check("works: a quarry's face in the rock of a stone-built people, its pit under the ground (T-03, B-418)", quarries >= 10 && qBad == 0, fmt("%d of the land's quarries: %s", quarries, qBad ? qWhy.c_str() : "each face in the exposed rock of a people that built in stone, cut into the hill, each pit's floor under the ground"));
    check("works: the land's, each inside its cell, none touching, clear of the settlements, the cores and the world's, the same built again (B-418)", landN >= 100 && landOut == 0 && landTouch == 0 && landClose == 0 && landSame == landAsked,
          fmt("%d of the land's works round the test worlds' first places and at points of their land: %d over their cell's edge, %d touching, %d on a settlement, a core or a world's work; %d of %d the same built again%s%s", landN, landOut, landTouch, landClose, landSame, landAsked, landWhy.empty() ? "" : "; ", landWhy.c_str()));
    {   // the plan, the people's extent, a cell's settlement, the world's works and the land's: the same from a generator made in winter and in
        // summer as from one made at the year's mean (the explorer's site and the approach make theirs in the season of the moment)
        int same = 0, asked = 0; std::string why;
        Star st; if (starInSector(AGE_WORLDS[AGE_INDUSTRIAL].sx, 0, AGE_WORLDS[AGE_INDUSTRIAL].sz, st, true)) {
            StarSystem sys; sys.generate(st);
            const Body& b = sys.bodies[AGE_WORLDS[AGE_INDUSTRIAL].body];
            const BodyGen g0 = BodyGen::make(b, 0.0);
            std::shared_ptr<const WorldPlan> P0 = worldPlanOf(g0); std::shared_ptr<const WorldWorks> W0 = worldWorksOf(g0);
            const PlanPlace& cap = P0->plans[0].places[0];
            int gL, gN; landCellOf(g0, cap.lat, cap.lon, gL, gN); int rL, rN; ruinCellOf(g0, cap.lat, cap.lon, rL, rN);
            for (double season : {-1.0, 1.0}) {
                const BodyGen gs = BodyGen::make(b, season);
                asked++;
                std::shared_ptr<WorldPlan> P = buildWorldPlan(gs, true);
                bool ok = P->plans[0].places.size() == P0->plans[0].places.size();
                for (size_t i = 0; ok && i < P->plans[0].places.size(); i++) ok = length(P->plans[0].places[i].unit - P0->plans[0].places[i].unit) < 1e-12;
                if (!ok) why += fmt("the plan at season %+.0f; ", season);
                if (std::fabs(peopleExtentOf(gs).landShare - peopleExtentOf(g0).landShare) > 1e-12) { ok = false; why += fmt("the extent at %+.0f; ", season); }
                std::shared_ptr<WorldWorks> W = buildWorldWorks(gs);
                bool wok = W->works.size() == W0->works.size(); for (size_t i = 0; wok && i < W->works.size(); i++) wok = W->works[i].id == W0->works[i].id && length(W->works[i].unit - W0->works[i].unit) < 1e-12;
                if (!wok) { ok = false; why += fmt("the world's works at %+.0f; ", season); }
                for (int dl = -2; dl <= 2; dl++) for (int dn = -2; dn <= 2; dn++) {
                    std::shared_ptr<const Work> a = buildLandWork(gs, gL + dl, gN + dn), c = buildLandWork(g0, gL + dl, gN + dn);
                    if ((a == nullptr) != (c == nullptr) || (a && (a->id != c->id || length(a->unit - c->unit) > 1e-12))) { ok = false; why += fmt("a land's cell at %+.0f; ", season); }
                    RuinSpec x, y; const bool hx = ruinOfCell(gs, rL + dl, rN + dn, x, true), hy = ruinOfCell(g0, rL + dl, rN + dn, y, true);
                    if (hx != hy || (hx && (x.buildings.size() != y.buildings.size() || x.lat != y.lat || x.lon != y.lon))) { ok = false; why += fmt("a settlement's cell at %+.0f; ", season); }
                }
                if (ok) same++;
            }
        }
        check("works: the plan, a people's extent, its settlements, its works and its land's the same in every season (B-419)", asked == 2 && same == asked,
              fmt("Meihimeeri III from a generator made in winter and in summer: %d of %d the same as the year's mean%s%s", same, asked, why.empty() ? "" : ": ", why.substr(0, 300).c_str()));
    }
    check("works: the scanner hears one within 40 km of nine points in ten of a people's land and of its places (B-418)", covPts > 0 && covHeard * 10 >= covPts * 9 && covPlacesHeard * 20 >= covPlaces * 19,
          fmt("%d of %d points of the prehistoric to the modern peoples' land (%.1f heard on the whole), %d of %d places: %s", covHeard, covPts, covPts ? (double)covSum / covPts : 0.0, covPlacesHeard, covPlaces, covWhy.c_str()));
    check("works: a battlefield between the two sides of its war (T-03)", fields >= 10 && fBad == 0, fmt("%d battlefields (%d of a war with the other people): %s", fields, fJoint, fBad ? fWhy.c_str() : "each a battle of its war in its war's years, on the line between the two sides' places and between them"));
    const BodyGen plain = [] { BodyGen gg; gg.type = PT_FELISIAN; gg.seed = 12345; return gg; }();
    const bool none = !((sightKindsOf(plain) >> LM_WORK) & 1);
    check("works: the scanner's WORKS mode hears them (T-03)", hMissed == 0 && heard > 20 && none, fmt("%d works of the world and the land heard from their middles as WORKS echoes of their kind, %d lesser stretches unheard as they should be; %s", heard, unheard, hMissed ? hWhy.c_str() : (none ? "no WORKS mode on a world without a people" : "a WORKS mode on a plain world")));
    int nk = 0; std::string missing; for (int k = 0; k < WO_COUNT; k++) { if (kinds[k]) nk++; else missing += std::string(missing.empty() ? "" : ", ") + WORK_KIND_NAMES[k]; }
    check("works: the same twice, out of the cores, every kind laid, their forms sound (T-03)", same == nWorlds && inTown == 0 && nanForms == 0 && nk >= WO_COUNT - 2,
          fmt("%d worlds, %d works of %d pieces, built in %.0f ms (the slowest %s, %.0f ms): %d the same built again, %d in a city's core, %d with an unsound form; %d of %d kinds%s%s %s", nWorlds, total, pieces, buildMs, worstName.c_str(), worstMs,
              same, inTown, nanForms, nk, WO_COUNT, missing.empty() ? "" : " (none of ", missing.empty() ? "" : (missing + ")").c_str(), sWhy.c_str()));
    printf("  (the works' checks %.1f s)\n", nowSec() - t0);
}

// T-03: `works <sx> <sz> <body>`, `works ages`: a world's works, each with its age, its place and the numbers its rule found; D-05: `works rogue
// <k> [ly]`, a drifter's people's (the k-th nearest home)
static void printWorksStar(const Star& st, const AgeWorld& aw);
static void printWorks(const AgeWorld& aw) {
    Star st; if (!starInSector(aw.sx, 0, aw.sz, st, true)) { printf("no star at %lld 0 %lld\n", (long long)aw.sx, (long long)aw.sz); return; }
    printWorksStar(st, aw);
}
static void printWorksStar(const Star& st, const AgeWorld& aw) {
    StarSystem sys; sys.generate(st);
    if (aw.body >= (int)sys.bodies.size()) { printf("no body %d\n", aw.body); return; }
    const Body& b = sys.bodies[aw.body];
    const BodyGen g = BodyGen::make(b);
    if (!worldHadCivilisation(g)) { printf("%s had no people\n", b.name.c_str()); return; }
    worldPlanOf(g);
    for (int p = 0; p < peoplesOf(g); p++) historyOfWorld(g, p);
    const double t0 = nowSec();
    std::shared_ptr<WorldWorks> W = buildWorldWorks(g);
    const double ms = (nowSec() - t0) * 1000;
    printf("%s (%lld 0 %lld body %d): %zu works in %.0f ms\n", b.name.c_str(), (long long)aw.sx, (long long)aw.sz, aw.body, W->works.size(), ms);
    for (int p = 0; p < W->peoples; p++) {
        printf("  people %d (%s):", p, AGE_NAMES[peopleAgeOf(g, p).age]);
        for (int k = 0; k < WO_COUNT; k++) if (W->counts[p][k]) printf(" %s %d,", WORK_KIND_NAMES[k], W->counts[p][k]);
        printf("\n");
        std::shared_ptr<const History> H = historyOfWorld(g, p);
        if (H) {   // what its history finished that the works lay down, and its battles
            int built[WK_COUNT] = {}, battles = 0;
            for (const HistoryEvent& e : H->events) { if (e.kind == HE_BUILT && e.thing >= 0 && e.thing < WK_COUNT) built[e.thing]++; if (e.kind == HE_BATTLE) battles++; }
            printf("    its history: dams %d, mines %d, aqueducts %d, observatories %d, lighthouses %d, launch sites %d, elevators %d, holy %d (%s), stones %d; %d battles in %zu wars; the last place %d\n",
                   built[WK_DAM], built[WK_MINE], built[WK_AQUEDUCT], built[WK_OBSERVATORY], built[WK_LIGHTHOUSE], built[WK_LAUNCH], built[WK_ELEVATOR], built[WK_HOLY], HOLY_NAMES[H->profile.holy], built[WK_STONES], battles, H->wars.size(), H->lastPlace);
        }
    }
    for (const Work& w : W->works) {
        RuinSpec sp; workSpec(g, w, sp);
        printf("    %s; %zu pieces, %zu edits\n", workLine(w).c_str(), sp.buildings.size(), w.edits.size());
    }
    // B-418: the land's round each people's first ten places and at forty points of its land, and what the scanner hears from those points
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    for (int p = 0; p < W->peoples; p++) {
        const double tl = nowSec();
        std::vector<std::shared_ptr<const Work>> lw; landWorksOf(g, p, 10, 20000, 40, lw);
        const double lms = (nowSec() - tl) * 1000;
        int by[WO_COUNT] = {}; for (const auto& w : lw) by[w->kind]++;
        printf("  the land's works of people %d round its first ten places and at forty points of its land: %zu in %.0f ms:", p, lw.size(), lms);
        for (int k = 0; k < WO_COUNT; k++) if (by[k]) printf(" %s %d,", WORK_KIND_NAMES[k], by[k]);
        printf("\n");
        const PeoplePlan& pp = P->plans[p];
        std::vector<int> land; for (int c = 0; c < P->grid.cells; c++) if (pp.territory[c] && P->grid.land[c]) land.push_back(c);
        Rng rng(g.seed ^ 0xB4180C0ULL ^ (uint64_t)p);
        int pts = 0, heard = 0, sum = 0;
        for (int k = 0; k < 40 && !land.empty(); k++) { std::vector<Landmark> L; worksHeardNear(g, P->grid.unitOf(land[rng.irange((int)land.size())]), 40000, L); pts++; sum += (int)L.size(); if (!L.empty()) heard++; }
        printf("    the scanner from %d points of its land: a work within 40 km from %d (%.1f heard on the whole)\n", pts, heard, pts ? (double)sum / pts : 0.0);
        for (size_t i = 0; i < lw.size() && i < 12; i++) printf("    %s\n", workLine(*lw[i]).c_str());
    }
}
// T-03: a work through the renderer: the site at its middle at a morning's light, the capsule out of the way, the camera where its kind shows
// best (a dam from below it with its reservoir behind, a cut from in front of its face, lines from high over the plain), six frames to settle
static void workFrame(const StarSystem& sys, int body, const BodyGen& g, const Work& w, std::vector<uint32_t>& rgbOut, std::string* note = nullptr) {
    SpaceRenderer sr; StarNeighborhood nb; SurfaceView sv;
    sv.init(&sys, body, w.lat, w.lon, 1000.0);
    double err = 0; const double t = findTime(sv.site, 30 * DEG, 1000.0, &err);
    sv.init(&sys, body, w.lat, w.lon, t);
    sv.flocks.clear(); sv.critters.clear();
    sv.relocateCapsule(-6000, -6000);
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    const SunInfo sun = sv.site.sun(t);
    RuinSpec sp; workSpec(g, w, sp);
    // its extent: its pieces' and its ground's (a reservoir's left out: the dam is the work), the view about a frame's width over it
    double ext = 20;
    {   // the pieces' reach, the farthest few left out (a barrow or a menhir off on its own)
        std::vector<double> rs; for (const Building& b : sp.buildings) rs.push_back(std::hypot(b.x, b.z) + std::max(b.hw, b.hd));
        std::sort(rs.begin(), rs.end());
        if (!rs.empty()) ext = std::max(ext, rs[std::min(rs.size() - 1, (size_t)(rs.size() * 0.85))]);
    }
    for (const GroundEdit& e : w.edits) {
        if (e.kind == GE_WATER) continue;
        if (e.kind == GE_LINE || e.kind == GE_DITCH) { for (size_t q = 0; q + 1 < e.pts.size(); q += 2) ext = std::max(ext, std::hypot((double)e.pts[q], (double)e.pts[q + 1])); }
        else ext = std::max(ext, std::hypot(e.x, e.z) + std::max(e.r, std::max(e.hw, e.hd)));
    }
    double tx = 0, tz = 0, ty = -1e9, D = std::max(45.0, 1.7 * ext), H = D * 0.36, a = sun.azimuth + 1.9;
    const double hd = w.heading, fx = std::sin(hd), fz = std::cos(hd);
    switch (w.kind) {
        case WO_QUARRY: case WO_MINE: if ((w.kind == WO_QUARRY && w.style >= 1 && w.style <= 3) || (w.kind == WO_MINE && w.style == 0)) { a = hd + PI + 0.35; D = std::max(70.0, 1.9 * ext); H = D * 0.33; } break;
        case WO_DAM: a = hd + PI + 0.75; D = w.length * 0.5 + 60; H = w.height * 1.4 + 22; ty = w.crest - w.height * 0.5; break;   // three quarters from downstream
        case WO_AQUEDUCT: {
            double best = -1; for (const Building& b : sp.buildings) if (b.kind == BK_AQUEDUCT && b.hWall > best) { best = b.hWall; tx = b.x; tz = b.z; }
            a = hd + PI / 2 + 0.3; D = 230; H = 40; break;
        }
        case WO_SHRINE: if (w.style == HOLY_CAVE) a = hd + 0.3; break;
        case WO_NECROPOLIS: a = hd + 0.4; break;
        case WO_GEOGLYPH: D = 160; H = 760; break;
        case WO_FORT_LINE: a = hd + PI / 2 + 0.6; D = 300; H = 80; break;
        case WO_BATTLEFIELD: D = std::min(D, 380.0); H = D * 0.4; break;
        case WO_WRECK: if (w.style <= 2) a = w.axis + PI + 0.6; break;
        case WO_COLOSSUS: D = w.height * 2.4 + 25; H = w.height * 0.55; a = hd + 0.3; break;
        case WO_TERRAFORM: D = w.length * 0.9; H = w.length * 0.32; break;
        case WO_SHELTER: if (w.height > 0.12) a = hd + 0.35; break;
        case WO_STRANGE:
            if (w.style == 0) { D = w.radiusM * 1.15; H = 280; }
            else if (w.style == 1) { a = hd + PI + 0.15; tx = -fx * w.length * 0.5; tz = -fz * w.length * 0.5; D = 700; H = 110; }
            else if (w.style == 2) { D = std::min(w.radiusM * 0.85, 2600.0); H = D * 0.65; ty = w.level; }   // within the 64 m ring that draws its terraces
            else if (w.style == 3) { D = w.length * 1.25; H = D * 0.35; }
            else { D = w.height * 6; H = w.height * 1.5; }
            break;
        case WO_WINDFARM: D = std::max(D, 1600.0); H = 340; break;
        default: break;
    }
    if (getenv("WORK_D")) D = atof(getenv("WORK_D"));   // tests: the camera's distance, height and bearing's turn given
    if (getenv("WORK_H")) H = atof(getenv("WORK_H"));
    if (getenv("WORK_A")) a += atof(getenv("WORK_A"));
    sv.player.x = tx + std::sin(a) * D; sv.player.z = tz + std::cos(a) * D;
    if (ty < -1e8) ty = sv.site.surfaceHeight(tx, tz);
    formsCamera(sv, H, tx, ty, tz, t);
    Framebuffer fb; Input in;
    for (int i = 0; i < 6; i++) { sv.update(1.0 / 30, in, t, false); sv.render(fb, t, nb.stars, sr, 1.0); }
    fb.mush(2);
    rgbOut.assign((size_t)FBW * FBH, 0);
    fb.toRGB(rgbOut.data(), 1.0);
    if (note) *note = fmt("%zu pieces, %d forms drawn (%d polygons), %zu edits in the ground, the view %.0f m off and %.0f m up (the eye at %.0f m, the ground under it %.0f, at the middle %.0f)", sp.buildings.size(), sv.lastRuinElems, sv.lastFormPolys, sv.site.edits.size(), D, H, sv.freePos.y, sv.site.surfaceHeight(sv.player.x, sv.player.z), sv.site.surfaceHeight(tx, tz));
}

// T-03: `works map`: on each age's scene world, an inset for each kind of work its people made (the gallery's), framing it with the place its rule
// tied it to: the ground shaded by its slope, the water, the places' cores, the ways, the work's reach, a battlefield's two sides; the rule said
// under it (`shots/tests/works_map_<age>.png`)
static const char WORK_GLYPHS[WO_COUNT + 1] = "QMDAOSWNGHFBLRVCTX?EIKUP";   // a dome, glasshouses
// one work of each kind a people made (a strange work of each style): the world's, else the land's round its first places and at points of its land
static std::vector<std::shared_ptr<const Work>> galleryPicks(const BodyGen& g, const std::shared_ptr<const WorldWorks>& W, int people) {
    std::vector<std::shared_ptr<const Work>> all;
    for (const Work& w : W->works) if (w.people == people) all.push_back(std::shared_ptr<const Work>(W, &w));
    std::vector<std::shared_ptr<const Work>> lw; landWorksOf(g, people, 12, 20000, 40, lw);
    all.insert(all.end(), lw.begin(), lw.end());
    std::vector<std::shared_ptr<const Work>> pick;
    for (int k = 0; k < WO_COUNT; k++) {
        if (k == WO_WAY) continue;
        if (k == WO_STRANGE) { bool seen[8] = {}; for (const auto& w : all) if (w->kind == k && w->heard && !seen[clampi(w->style, 0, 7)]) { seen[clampi(w->style, 0, 7)] = true; pick.push_back(w); } continue; }
        for (const auto& w : all) if (w->kind == k) { pick.push_back(w); break; }
    }
    return pick;
}
static void worksMap(const AgeWorld& aw, const char* tag) {
    Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) return;
    if (aw.body >= (int)sys.bodies.size()) return;
    const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
    if (!worldHadCivilisation(g)) return;
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    std::shared_ptr<const WorldWorks> Wk = worldWorksOf(g);
    const int people = std::min(aw.which, P->peoples - 1);
    const PeoplePlan& pp = P->plans[people];
    std::shared_ptr<const History> H = historyOfWorld(g, people);
    const std::vector<std::shared_ptr<const Work>> pick = galleryPicks(g, Wk, people);
    const int T = 300, cols = 4, rows = std::max(1, ((int)pick.size() + cols - 1) / cols), SW = cols * T, SH = 16 + rows * (T + 24);
    std::vector<uint32_t> img((size_t)SW * SH, rgb(16, 16, 20));
    RGBCanvas cv{img.data(), SW, SH, 1};
    drawText(cv, 4, 4, fmt("WHERE THE RULES PUT THE WORKS OF A %s PEOPLE ON %s", pp.age == AGE_FAR_FUTURE ? "FAR FUTURE" : upper(AGE_NAMES[pp.age]).c_str(), upper(sys.bodies[aw.body].name).c_str()).c_str(), rgb(235, 225, 170));
    const double Rm = g.R * 1000.0;
    DrainageOff off;
    for (size_t n = 0; n < pick.size(); n++) {
        const Work& w = *pick[n];
        // the place it serves (the plan's, or the history's), and a battlefield's two sides
        std::vector<Vec3> pts = {w.unit};
        Vec3 anchor; bool hasA = false;
        if (w.planRank >= 0 && w.planRank < (int)pp.places.size()) { anchor = pp.places[w.planRank].unit; hasA = true; }
        else if (H && w.place >= 0 && w.place < (int)H->places.size()) { anchor = StarSystem::bodyFromLatLon(H->places[w.place].lat, H->places[w.place].lon); hasA = true; }
        if (hasA) pts.push_back(anchor);
        if (w.kind == WO_BATTLEFIELD) pts.push_back(std::acos(clampd(dot(w.unit, w.sideA), -1, 1)) < std::acos(clampd(dot(w.unit, w.sideB), -1, 1)) ? w.sideA : w.sideB);
        double lat0, lon0; StarSystem::latLonFromBody(w.unit, lat0, lon0);
        const double cl = std::max(std::cos(lat0), 0.05);
        double ext = std::max(4000.0, w.radiusM * 2.5);
        for (const Vec3& q : pts) { double la, lo; StarSystem::latLonFromBody(q, la, lo); ext = std::max(ext, 1.25 * std::max(std::fabs(std::remainder(lo - lon0, TAU)) * Rm * cl, std::fabs(la - lat0) * Rm)); }
        const double mpp = 2 * ext / T;
        const int ox = (int)(n % cols) * T, oy = 16 + (int)(n / cols) * (T + 24);
        auto toPx = [&](const Vec3& u, double& x, double& y) { double la, lo; StarSystem::latLonFromBody(u, la, lo); x = ox + T / 2 + std::remainder(lo - lon0, TAU) * Rm * cl / mpp; y = oy + T / 2 - (la - lat0) * Rm / mpp; };
        const int G = 101; std::vector<double> hh(G * G); std::vector<uint32_t> col(G * G);
        for (int j = 0; j < G; j++) for (int i = 0; i < G; i++) {
            const double e = (i - G / 2) * 2 * ext / (G - 1), nn = (G / 2 - j) * 2 * ext / (G - 1);
            const SurfaceSample sm = sampleSurface(g, StarSystem::bodyFromLatLon(lat0 + nn / Rm, lon0 + e / (Rm * cl)), std::max(64.0, 2 * ext / (G - 1)));
            hh[j * G + i] = sm.height;
            const bool wet = sm.material == MAT_WATER || (sm.water > -1e8 && sm.height < sm.water) || sm.oldSea > 0.5;
            const RGB c = wet ? RGB(0.08f, 0.16f, 0.32f) : g.matColor[sm.material];
            col[j * G + i] = rgb(clampi((int)(c.r * 255), 0, 255), clampi((int)(c.g * 255), 0, 255), clampi((int)(c.b * 255), 0, 255));
        }
        const double relief = std::max(20.0, ext / 40);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            const int i = std::min(G - 2, x * (G - 1) / T), j = std::min(G - 2, y * (G - 1) / T), c = j * G + i;
            const double sh = clampd(1.0 + ((hh[c + G] - hh[c]) - (hh[c + 1] - hh[c])) / relief, 0.45, 1.5);
            const uint32_t cc = col[c];
            img[(size_t)(oy + y) * SW + ox + x] = rgb(clampi((int)((cc & 255) * 0.8 * sh), 0, 255), clampi((int)(((cc >> 8) & 255) * 0.8 * sh), 0, 255), clampi((int)(((cc >> 16) & 255) * 0.8 * sh), 0, 255));   // rgb() packs red low
        }
        auto px = [&](int x, int y, uint32_t c) { if (x >= ox && y >= oy && x < ox + T && y < oy + T) img[(size_t)y * SW + x] = c; };
        auto line = [&](const Vec3& a, const Vec3& b, uint32_t c, bool dotted) { double x0, y0, x1, y1; toPx(a, x0, y0); toPx(b, x1, y1); const int m = (int)std::max(std::fabs(x1 - x0), std::fabs(y1 - y0)) + 1; for (int k = 0; k <= m; k++) { if (dotted && (k / 3) % 2) continue; px((int)(x0 + (x1 - x0) * k / m), (int)(y0 + (y1 - y0) * k / m), c); } };
        for (const PlanEdge& e : pp.edges) {   // the ways near
            PlanEdgeRef ref; ref.people = people; ref.index = (int)(&e - pp.edges.data()); ref.e = &e;
            const double da = std::acos(clampd(dot(pp.places[e.a].unit, w.unit), -1, 1)) * Rm, db = std::acos(clampd(dot(pp.places[e.b].unit, w.unit), -1, 1)) * Rm;
            if (std::min(da, db) > 6 * ext + e.lengthKm * 1000) continue;
            const std::vector<Vec3>& way = planEdgeWay(g, *P, ref);
            for (size_t k = 1; k < way.size(); k++) line(way[k - 1], way[k], PLAN_WAY_COLOURS[e.kind], e.kind == NK_SEA);
        }
        for (const PlanPlace& pl : pp.places) {   // the places' cores
            double x, y; toPx(pl.unit, x, y);
            const int r = std::max(2, (int)(pl.radiusM / mpp));
            if (x < ox - r || y < oy - r || x > ox + T + r || y > oy + T + r) continue;
            for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) if (dx * dx + dy * dy <= r * r) px((int)x + dx, (int)y + dy, pl.rank == 0 ? rgb(255, 255, 255) : (pl.sclass == SC_CITY ? rgb(255, 90, 80) : rgb(255, 210, 90)));
        }
        if (w.kind == WO_BATTLEFIELD) line(w.sideA, w.sideB, rgb(255, 70, 70), true);
        {   // the work: its reach, its glyph
            double x, y; toPx(w.unit, x, y);
            const int r = std::max(3, (int)(w.radiusM / mpp));
            for (int k = 0; k < 96; k++) { const double a = k * TAU / 96; px((int)(x + std::sin(a) * r), (int)(y + std::cos(a) * r), rgb(120, 255, 255)); }
            const char gl[2] = {WORK_GLYPHS[w.kind], 0};
            fillRectRGB(cv, (int)x - 3, (int)y - 4, (int)x + 4, (int)y + 5, rgb(0, 0, 0));
            drawText(cv, (int)x - 2, (int)y - 3, gl, rgb(120, 255, 255));
        }
        // the rule, in a line
        std::string rule;
        const double dA = hasA ? std::acos(clampd(dot(anchor, w.unit), -1, 1)) * Rm / 1000.0 : 0;
        switch (w.kind) {
            case WO_DAM: rule = fmt("%.0f M HIGH, ITS WATER AT %.0f M; %.0f KM FROM ITS TOWN", w.height, w.level, dA); break;
            case WO_BATTLEFIELD: rule = fmt("BETWEEN ITS SIDES %.0f KM APART, YEAR %d", w.length / 1000.0, w.year); break;
            case WO_OBSERVATORY: rule = fmt("ON A SUMMIT OF %.0f M, %.0f KM FROM ITS TOWN", w.height, dA); break;
            default: rule = hasA ? fmt("%.1f KM FROM ITS PLACE", dA) : std::string("ON THE PEOPLE'S LAND"); break;
        }
        std::string lab = WORK_KIND_NAMES[w.kind]; for (char& ch : lab) ch = (char)std::toupper((unsigned char)ch);
        drawText(cv, ox + 3, oy + T + 2, fmt("%s, %s (%.0f KM ACROSS)", lab.c_str(), w.age == AGE_FAR_FUTURE ? "FAR FUTURE" : upper(AGE_NAMES[w.age]).c_str(), 2 * ext / 1000).c_str(), rgb(200, 205, 190));
        drawText(cv, ox + 3, oy + T + 12, rule.substr(0, 48).c_str(), rgb(150, 210, 210));
    }
    const std::string fn = fmt("shots/tests/works_map_%s.png", tag);
    writePNG(fn.c_str(), img.data(), SW, SH);
    printf("%s (%s): %zu kinds -> %s\n", sys.bodies[aw.body].name.c_str(), AGE_NAMES[pp.age], pick.size(), fn.c_str());
}

// T-03: `works gallery`: on each age's scene world, a frame of each kind of work its people made, where its rule put it, and a sheet of them
// (`shots/tests/works_<age>_<kind>.png`, `shots/tests/works_<age>_sheet.png`)
static void worksGallery() {
    const int scale = std::max(1, g_testScale);
    setFramebufferScale(scale);
    for (int age = 0; age < AGE_COUNT; age++) {
        const AgeWorld& aw = AGE_SCENE_WORLDS[age];
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) continue;
        if (aw.body >= (int)sys.bodies.size()) continue;
        const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
        if (!worldHadCivilisation(g)) continue;
        std::shared_ptr<const WorldWorks> W = worldWorksOf(g);
        const int people = std::min(aw.which, std::max(0, W->peoples - 1));
        const std::vector<std::shared_ptr<const Work>> pick = galleryPicks(g, W, people);
        const int cols = 3, rows = std::max(1, ((int)pick.size() + cols - 1) / cols), textH = 12 * scale, tileH = FBH + textH;
        const int SW = cols * FBW, SH = 16 * scale + rows * tileH;
        std::vector<uint32_t> sheet((size_t)SW * SH, rgb(16, 16, 20));
        RGBCanvas cv{sheet.data(), SW, SH, scale};
        drawText(cv, 4, 4, fmt("THE WORKS OF A %s PEOPLE ON %s", age == AGE_FAR_FUTURE ? "FAR FUTURE" : AGE_NAMES[age], sys.bodies[aw.body].name.c_str()).c_str(), rgb(235, 225, 170));
        printf("%s, %s: %zu kinds\n", AGE_NAMES[age], sys.bodies[aw.body].name.c_str(), pick.size());
        for (size_t i = 0; i < pick.size(); i++) {
            const Work& w = *pick[i];
            std::vector<uint32_t> rgbBuf; std::string note;
            workFrame(sys, aw.body, g, w, rgbBuf, &note);
            std::string kname = WORK_KIND_NAMES[w.kind]; for (char& ch : kname) if (ch == ' ' || ch == '\'') ch = '_';
            if (w.kind == WO_STRANGE) kname += fmt("_%d", w.style);
            const std::string fn = fmt("shots/tests/works_%s_%s.png", age == AGE_FAR_FUTURE ? "far_future" : AGE_NAMES[age], kname.c_str());
            writePNG(fn.c_str(), rgbBuf.data(), FBW, FBH);
            const int cx = (int)(i % cols), cy = (int)(i / cols), dx0 = cx * FBW, dy0 = 16 * scale + cy * tileH;
            for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(dy0 + y) * SW + dx0 + x] = rgbBuf[(size_t)y * FBW + x];
            std::string lab = WORK_KIND_NAMES[w.kind]; for (char& ch : lab) ch = (char)std::toupper((unsigned char)ch);
            drawText(cv, dx0 / scale + 3, (dy0 + FBH) / scale + 2, fmt("%s, %s", lab.c_str(), w.age == AGE_FAR_FUTURE ? "FAR FUTURE" : AGE_NAMES[w.age]).c_str(), rgb(200, 205, 190));
            printf("  %s: %s\n    %s -> %s\n", WORK_KIND_NAMES[w.kind], workLine(w).c_str(), note.c_str(), fn.c_str());
            fflush(stdout);
        }
        const std::string sfn = fmt("shots/tests/works_%s_sheet.png", age == AGE_FAR_FUTURE ? "far_future" : AGE_NAMES[age]);
        writePNG(sfn.c_str(), sheet.data(), SW, SH);
        printf("  -> %s\n", sfn.c_str());
    }
}

int runWorks(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    const int which = takePeopleArg(argc, argv);
    if (argc > 2 && std::string(argv[2]) == "ages") {
        for (int a = 0; a < AGE_COUNT; a++) { printf("== %s (the age test world)\n", AGE_NAMES[a]); printWorks(AGE_WORLDS[a]); }
        for (int a = 0; a < AGE_COUNT; a++) { printf("== %s (the age scene world)\n", AGE_NAMES[a]); printWorks(AGE_SCENE_WORLDS[a]); }
        return 0;
    }
    if (argc > 2 && std::string(argv[2]) == "check") {   // the unit's T-03 checks alone
        int failures = 0;
        worksChecks(modeCheck(failures));
        printf("works check: %d failure%s\n", failures, failures == 1 ? "" : "s");
        return failures ? 1 : 0;
    }
    if (argc > 2 && std::string(argv[2]) == "gallery") { worksGallery(); return 0; }
    if (argc > 3 && std::string(argv[2]) == "rogue") {   // the k-th drifter's people nearest home (`peoples rogues`' order)
        const std::vector<Star> ds = drifterPeoplesNearHome(argc > 4 ? atof(argv[4]) : 6000);
        const int k = atoi(argv[3]);
        if (k < 0 || k >= (int)ds.size()) { printf("no drifter's people %d (of %zu)\n", k, ds.size()); return 1; }
        printWorksStar(ds[k], AgeWorld{ds[k].sx, ds[k].sz, 0, which});
        return 0;
    }
    if (argc > 2 && std::string(argv[2]) == "map") { for (int a = 0; a < AGE_COUNT; a++) worksMap(AGE_SCENE_WORLDS[a], a == AGE_FAR_FUTURE ? "far_future" : AGE_NAMES[a]); return 0; }
    if (argc > 6 && std::string(argv[2]) == "view") {   // `works view <sx> <sz> <body> <a word of the kind's name, or its whole name> [index]`: one work through the renderer
        const AgeWorld aw{atoll(argv[3]), atoll(argv[4]), atoi(argv[5]), which};
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) return 1;
        const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
        std::shared_ptr<const WorldWorks> W = worldWorksOf(g);
        const std::string want = argv[6]; const int idx = argc > 7 ? atoi(argv[7]) : 0;
        setFramebufferScale(std::max(1, g_testScale));
        std::vector<std::shared_ptr<const Work>> all;   // the world's, then the land's (B-418) round the people's first places and at points of its land
        for (const Work& w : W->works) all.push_back(std::shared_ptr<const Work>(W, &w));
        { std::vector<std::shared_ptr<const Work>> lw; landWorksOf(g, std::min(aw.which, std::max(0, W->peoples - 1)), 12, 20000, 40, lw); all.insert(all.end(), lw.begin(), lw.end()); }
        int n = 0;
        bool exact = false; for (int k = 0; k < WO_COUNT; k++) if (want == WORK_KIND_NAMES[k]) exact = true;   // "a fort", not "a fortified line"
        for (const std::shared_ptr<const Work>& wp : all) {
            const Work& w = *wp;
            std::string k = WORK_KIND_NAMES[w.kind];
            if ((exact ? k != want : k.find(want) == std::string::npos) || n++ != idx) continue;
            std::vector<uint32_t> rgbBuf; std::string note;
            workFrame(sys, aw.body, g, w, rgbBuf, &note);
            const std::string fn = fmt("shots/tests/works_view_%s_%d.png", want.c_str(), idx);
            writePNG(fn.c_str(), rgbBuf.data(), FBW, FBH);
            printf("%s\n  %s -> %s\n", workLine(w).c_str(), note.c_str(), fn.c_str());
            RuinSpec sp; workSpec(g, w, sp);
            for (const Building& b : sp.buildings) printf("    %s at %.0f %.0f, %.1f x %.1f, %.1f m, style %d, age %d%s\n", BUILDING_KIND_NAMES[b.kind], b.x, b.z, b.hw, b.hd, b.hWall, b.style, b.age, b.base > -1e8 ? fmt(", from %.0f m", b.base).c_str() : "");
            return 0;
        }
        printf("no %s %d\n", want.c_str(), idx);
        return 1;
    }
    if (argc > 4) { printWorks(AgeWorld{atoll(argv[2]), atoll(argv[3]), atoi(argv[4]), which}); return 0; }
    printf("works <sx> <sz> <body> | works ages | works gallery [scale=N] | works rogue <k> [ly]\n");
    return 1;
}
