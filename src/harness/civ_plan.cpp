// E-03: the plan, the towns and the roads (T-01, T-02, C-09): `plan`, `towns`, `roads` and their checks
#include "harness/harness.h"
#include "harness/test_worlds.h"

// T-01: a people's map of its world. On the six age worlds (`AGE_WORLDS`): the plan the same twice and on the workers, within a second and a
// half on a thread; its places dry where they stand and round their cores, the capital on the territory's best ground (dry, gentle, its
// score in the top tenth, by the water before the futuristic age), the networks reaching every place from the capital; the cells read it
// (no settlement on land outside every territory, none in a city's core but the city); a city's core that spans cells drawn whole and walked
// into from a cell not its own; the landing zoom over a modern capital shows its core and its highways, the scanner hears it from 150 km,
// and a way's legs lie on the ground by its kind
void planChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    struct World { Star st; StarSystem sys; BodyGen g; int which = 0; bool ok = false; };
    std::vector<World> W(AGE_COUNT);
    for (int a = 0; a < AGE_COUNT; a++) {
        World& w = W[a];
        if (!starInSector(AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, w.st, true)) continue;
        w.sys.generate(w.st);
        if (AGE_WORLDS[a].body >= (int)w.sys.bodies.size()) continue;
        w.g = BodyGen::make(w.sys.bodies[AGE_WORLDS[a].body]); w.which = AGE_WORLDS[a].which; w.ok = worldHadCivilisation(w.g);
    }
    {   // deterministic, the workers' the serial reading's, the time a world on one thread
        bool same = true; double worst = 0; int worlds = 0; std::string times;
        for (int a = 0; a < AGE_COUNT; a++) {
            if (!W[a].ok) continue;
            worlds++;
            const double t0 = nowSec();
            std::shared_ptr<WorldPlan> A = buildWorldPlan(W[a].g, false);
            const double ms = (nowSec() - t0) * 1000;
            std::shared_ptr<WorldPlan> B = buildWorldPlan(W[a].g, true);
            worst = std::max(worst, ms);
            times += fmt("%s%.0f", times.empty() ? "" : "/", ms);
            if (A->grid.height != B->grid.height || A->grid.river != B->grid.river || A->countryside != B->countryside) same = false;
            for (int k = 0; k < A->peoples; k++) {
                const PeoplePlan &p = A->plans[k], &q = B->plans[k];
                if (p.places.size() != q.places.size() || p.edges.size() != q.edges.size() || p.territory != q.territory) { same = false; continue; }
                for (size_t i = 0; i < p.places.size(); i++) if (p.places[i].seed != q.places[i].seed || p.places[i].lat != q.places[i].lat || p.places[i].sclass != q.places[i].sclass || p.places[i].radiusM != q.places[i].radiusM) same = false;
                for (size_t i = 0; i < p.edges.size(); i++) if (p.edges[i].id != q.edges[i].id || p.edges[i].kind != q.edges[i].kind) same = false;
            }
        }
        check("the world plan: the same twice and on the workers, within a second and a half on one thread (T-01)", worlds == AGE_COUNT && same && worst < 1500,
              fmt("%d age worlds, %s; built on one thread in %s ms (the worst %.0f)", worlds, same ? "the same twice, the workers' the serial's" : "NOT the same", times.c_str(), worst));
    }
    {   // the places: dry where they stand and round their cores, the capital on good ground, the networks connected
        int places = 0, wet = 0, capBad = 0, unconnected = 0, ages = 0; std::string caps;
        for (int a = 0; a < AGE_COUNT; a++) {
            if (!W[a].ok) continue;
            std::shared_ptr<const WorldPlan> P = worldPlanOf(W[a].g);
            const PeoplePlan& pp = P->plans[std::min(W[a].which, P->peoples - 1)];
            if (pp.places.empty()) { capBad++; continue; }
            ages++;
            const double Rm = W[a].g.R * 1000.0;
            DrainageOff off;
            for (const PlanPlace& p : pp.places) {
                places++;
                RuinSpec probe; probe.lat = p.lat; probe.lon = p.lon; probe.size = p.radiusM;
                bool dry = ruinSiteOk(W[a].g, probe);
                for (int k = 0; k < 8 && dry; k++) {
                    const double ang = (k + 0.25) * TAU / 8, cl = std::max(std::cos(p.lat), 0.05);
                    const SurfaceSample q = sampleSurface(W[a].g, StarSystem::bodyFromLatLon(p.lat + std::cos(ang) * p.radiusM / Rm, p.lon + std::sin(ang) * p.radiusM / (Rm * cl)), 64.0);
                    if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water) || q.oldSea > 0.5) dry = false;
                }
                if (!dry) wet++;
            }
            const PlanPlace& cap = pp.places[0];
            std::vector<float> sc; for (int c = 0; c < P->grid.cells; c++) if (pp.territory[c]) sc.push_back(pp.score[c]);
            std::sort(sc.begin(), sc.end());
            const double top = sc.empty() ? 1 : sc[(size_t)(0.9 * (sc.size() - 1))];
            const bool water = a >= AGE_FUTURISTIC || cap.coast || cap.river;
            const bool good = cap.score >= top && water && (cap.sclass == SC_CAPITAL || (a == AGE_FAR_FUTURE && cap.sclass == SC_MONUMENT));
            if (!good) capBad++;
            caps += fmt("%s%s %.2f (top tenth from %.2f)%s", caps.empty() ? "" : "; ", AGE_NAMES[a], cap.score, top, cap.coast ? " on a coast" : (cap.river ? " on a river" : ""));
            if (a == AGE_FAR_FUTURE) continue;   // no ways in the far future
            const int n = (int)pp.places.size();
            std::vector<std::vector<int>> adj(n);
            for (const PlanEdge& e : pp.edges) { adj[e.a].push_back(e.b); adj[e.b].push_back(e.a); }
            std::vector<char> seen(n, 0); std::vector<int> q{0}; seen[0] = 1;
            for (size_t h = 0; h < q.size(); h++) for (int v : adj[q[h]]) if (!seen[v]) { seen[v] = 1; q.push_back(v); }
            unconnected += n - (int)q.size();
        }
        check("the world plan's places: none in the sea, the capital on the territory's best ground, the networks reaching every place (T-01)", ages == AGE_COUNT && wet == 0 && capBad == 0 && unconnected == 0 && places > 500,
              fmt("%d places of the six peoples, %d wet where they stand or round their cores; the capitals %s%s; %d places the networks miss", places, wet, caps.c_str(), capBad ? fmt(" (%d NOT on good ground)", capBad).c_str() : "", unconnected));
    }
    {   // the cells read the plan: on the medieval world, land cells far from every territory hold no settlement; a city's core holds no
        // settlement but the city
        int outsideCells = 0, outsideSettled = 0, coreCells = 0, coreOthers = 0;
        const World& w = W[AGE_MEDIEVAL];
        if (w.ok) {
            std::shared_ptr<const WorldPlan> P = worldPlanOf(w.g);
            const PlanGrid& G = P->grid;
            int tried = 0;
            for (int c = 0; c < G.cells && tried < 40; c += 997) {   // land of the grid, two cells from any territory
                if (!G.land[c]) continue;
                bool near = false; int nb[8]; const int k = G.neighbours(c, nb);
                for (int p = 0; p < P->peoples; p++) { if (P->plans[p].territory[c]) near = true; for (int i = 0; i < k; i++) if (P->plans[p].territory[nb[i]]) near = true; }
                if (near) continue;
                tried++;
                double la, lo; G.latLonOf(c, la, lo);
                int gl, gn; ruinCellOf(w.g, la, lo, gl, gn);
                for (int dl = -2; dl <= 2; dl++) for (int dn = -2; dn <= 2; dn++) { RuinSpec sp; outsideCells++; if (ruinOfCell(w.g, gl + dl, gn + dn, sp, false)) outsideSettled++; }
            }
            const PeoplePlan& pp = P->plans[0];
            for (size_t i = 0; i < pp.places.size() && i < 12; i++) {   // the capital and the first cities: the cells under their cores
                const PlanPlace& p = pp.places[i];
                const int span = (int)std::ceil(p.radiusM / (ruinCellLat(w.g) * w.g.R * 1000.0)) + 1;
                for (int dl = -span; dl <= span; dl++) for (int dn = -span; dn <= span; dn++) {
                    RuinSpec sp; if (!ruinOfCell(w.g, p.gLat + dl, p.gLon + dn, sp, false)) continue;
                    const double d = std::acos(clampd(dot(StarSystem::bodyFromLatLon(sp.lat, sp.lon), p.unit), -1, 1)) * w.g.R * 1000.0;
                    if (d > p.radiusM) continue;
                    coreCells++;
                    if (sp.seed != p.seed) coreOthers++;
                }
            }
        }
        check("the cells read the plan: no settlement in the land no people held, none in a city's core but the city (T-01)", w.ok && outsideCells >= 200 && outsideSettled == 0 && coreCells >= 3 && coreOthers == 0,
              fmt("%d cells of land beyond every territory, %d settled; %d settlements in the cores of the medieval capital and its first cities, %d not the city", outsideCells, outsideSettled, coreCells, coreOthers));
    }
    {   // a city spanning cells drawn whole: the modern capital (a core of a kilometre) from a cell of its core not its own: the view reaches its cell,
        // gathers its buildings to draw, and its colliders stand round a house of it
        const World& w = W[AGE_MODERN];
        bool reached = false, drawn = false, collides = false, spans = false; int buildings = 0; double ms = 0; std::string where = "no modern world";
        if (w.ok) {
            std::shared_ptr<const WorldPlan> P = worldPlanOf(w.g);
            const PlanPlace& cap = P->plans[std::min(w.which, P->peoples - 1)].places[0];
            const double Rm = w.g.R * 1000.0, cl = std::max(std::cos(cap.lat), 0.05);
            double la = cap.lat, lo = cap.lon; int gl = 0, gn = 0;
            for (int k = 0; k < 16; k++) {   // a point at 0.7 of the core in another cell
                const double ang = k * TAU / 16, d = 0.7 * cap.radiusM;
                la = cap.lat + std::cos(ang) * d / Rm; lo = cap.lon + std::sin(ang) * d / (Rm * cl);
                ruinCellOf(w.g, la, lo, gl, gn);
                const int nLon = std::max(1, ruinCellsAround(w.g, gl));
                if (gl != cap.gLat || ((gn % nLon) + nLon) % nLon != cap.gLon) { spans = true; break; }
            }
            where = fmt("%s's capital (a core of %.0f m) at %.2f %.2f", w.sys.bodies[AGE_WORLDS[AGE_MODERN].body].name.c_str(), cap.radiusM, la / DEG, lo / DEG);
            if (spans) {
                SurfaceView sv; sv.init(&w.sys, AGE_WORLDS[AGE_MODERN].body, la, lo, 0.0);
                const SurfaceView::RuinCell* rc = nullptr;
                sv.forNearbyRuins(0, 0, 1, [&](const Ruin& r, const SurfaceView::RuinCell& c, int) { if (c.spec.seed == cap.seed) rc = &c; });
                reached = rc != nullptr;
                if (rc) {
                    buildings = (int)rc->spec.buildings.size();
                    Framebuffer fb; SpaceRenderer sr; StarNeighborhood nb;
                    sv.render(fb, 0.0, nb.stars, sr, 1.0);
                    drawn = rc->drawn != 0 && sv.lastRuinElems > buildings / 2;
                    ms = sv.lastRuinMs;
                    for (const Building& b : rc->spec.buildings) {   // a dwelling of the core near the point: its colliders
                        const double x = rc->r.x + b.x, z = rc->r.z + b.z;
                        if (std::hypot(x, z) > 300 || b.kind == BK_WALL) continue;
                        std::vector<SurfaceView::Collider> cols; sv.collectColliders(x, z, cols);
                        for (const auto& c : cols) if (c.kind == 3 && std::hypot(c.x - x, c.z - z) < std::max(b.hw, b.hd) + 2) collides = true;
                        break;
                    }
                }
            }
        }
        check("a city's core spanning cells drawn whole: reached, gathered and walked into from a cell of its core not its own (T-01)", spans && reached && drawn && collides,
              fmt("%s: %s, %s, %d buildings, %s (the forms %.1f ms), its colliders %s", where.c_str(), spans ? "the point in another cell" : "NO other cell in the core", reached ? "reached" : "NOT reached", buildings, drawn ? "drawn" : "NOT drawn", ms, collides ? "round a house" : "NOT found"));
    }
    {   // the ship and the ground: the zoom over the modern capital shows its core and its highways; the scanner hears it from 150 km; a way's
        // legs lie on the ground by its kind
        const World& w = W[AGE_MODERN];
        int cores = 0, highways = 0, siteWays = 0; bool heard = false; std::string kinds;
        if (w.ok) {
            std::shared_ptr<const WorldPlan> P = worldPlanOf(w.g);
            const PeoplePlan& pp = P->plans[std::min(w.which, P->peoples - 1)];
            const PlanPlace& cap = pp.places[0];
            TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
            game.newGame(); game.setState(GameState::SPACE);
            Input gi; game.testParkAt(w.st, AGE_WORLDS[AGE_MODERN].body);
            for (int i = 0; i < 6; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
            game.testLandCursor(cap.lat / DEG, cap.lon / DEG); game.testBuildZoom();
            cores = game.testZoomCores(); highways = game.testZoomWays(NK_HIGHWAY);
            remove("shots/tests/test_guide.txt");
            const double Rm = w.g.R * 1000.0;
            const Vec3 far = StarSystem::bodyFromLatLon(cap.lat + 150e3 / Rm, cap.lon);
            std::vector<Landmark> echoes; ruinsNear(w.g, far, 40000, echoes);
            for (const Landmark& L : echoes) if (L.id == cap.seed && L.sub == SC_CAPITAL) heard = true;   // B-417: a ruin's echo its class
            // a way on the ground: the site at the middle of the capital's first highway or road
            for (const PlanEdge& e : pp.edges) {
                if (e.a != 0 && e.b != 0) continue;
                if (e.kind == NK_SEA) continue;
                PlanEdgeRef r; r.people = pp.people; r.e = &e; r.index = (int)(&e - &pp.edges[0]);
                const std::vector<Vec3>& way = planEdgeWay(w.g, *P, r);
                const Vec3 mid = way[way.size() / 2];
                double la, lo; StarSystem::latLonFromBody(mid, la, lo);
                SurfaceSite site; site.init(&w.sys, AGE_WORLDS[AGE_MODERN].body, la, lo, 0.0); site.clearRoads(); site.ensureRoads(0, 0);
                for (const SiteRoad& sr : site.roads) if (sr.kind == e.kind) siteWays++;
                kinds = NET_KIND_NAMES[e.kind];
                break;
            }
        }
        check("the plan on the ship and the ground: the zoom shows a modern capital's core and its highways, the scanner hears it from 150 km, a way's legs on the ground (T-01)", cores >= 1 && highways >= 1 && heard && siteWays >= 1,
              fmt("the zoom over the capital: %d cores, %d highways; the capital %s from 150 km; the capital's first %s: %d legs at the site", cores, highways, heard ? "heard" : "NOT heard", kinds.c_str(), siteWays));
    }
    {   // R-415: the landing map's peoples (V), off as the map opens: the marks of ruins hidden, the zoom the land alone; on: the marks, the places a
        // few held records were kept at (one each), and with all of a people's records held its whole map (its land, every place, its ways); the
        // zoom's rings and the capital named
        const AgeWorld& W = AGE_WORLDS[AGE_MODERN];
        Star st; starInSector(W.sx, 0, W.sz, st, true);
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
        auto press = [&](int key) { pressKey(game, in, key); };
        auto drawn = [&](int what) {   // the info's "drawn: P places, W ways, L land pixels, M marks, R rings, C capitals named"
            const std::string i = game.testLandPeoplesInfo(); const size_t p = i.find("drawn: ");
            int v[6] = {}; if (p != std::string::npos) sscanf(i.c_str() + p, "drawn: %d places, %d ways, %d land pixels, %d marks, %d rings, %d capitals", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
            return v[what];
        };
        game.testParkAt(st, W.body); run(0.1);
        StarSystem sys; sys.generate(st);
        const BodyGen g = BodyGen::make(sys.bodies[W.body]);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        game.testDronesSettle(); run(0.1);
        const PlanPlace cap = P->plans[0].places[0];
        const std::string bkey = Guide::bodyKey(W.sx, 0, W.sz, W.body);
        SurfaceMark m; m.body = bkey; m.lat = cap.lat - 0.02; m.lon = cap.lon + 0.03; m.kind = LM_RUIN; m.name = "A RUIN"; game.guide.marks.push_back(m);
        game.testLandCursor(cap.lat / DEG, cap.lon / DEG); run(0.1);
        const bool offFirst = game.testLandPeoplesInfo().find("hidden") != std::string::npos && drawn(3) == 0;
        press(KEY_V); run(0.1);
        const int marksOn = drawn(3), placesNone = drawn(0);
        const int n0 = recordCountOf(g, 0);
        std::set<int> ranks; std::shared_ptr<const RecordHomes> H = recordHomesOf(g, 0);
        for (int i = 0; i < n0; i += 9) { game.guide.shards.insert(bkey + "/S" + std::to_string(shardWorldIndex(0, i))); if (H->rank[i] >= 0) ranks.insert(H->rank[i]); }
        run(0.1);
        const int placesSome = drawn(0), waysSome = drawn(1);
        for (int i = 0; i < n0; i++) game.guide.shards.insert(bkey + "/S" + std::to_string(shardWorldIndex(0, i)));
        run(0.1);
        const int placesAll = drawn(0), waysAll = drawn(1), landAll = drawn(2), capitalAll = drawn(5);
        const bool allSaid = game.testLandPeoplesInfo().find(", ALL") != std::string::npos;
        press(KEY_Z); run(0.3);
        const int rings = drawn(4), capitalZoom = drawn(5);
        std::vector<uint32_t> on(game.output(), game.output() + (size_t)FBW * FBH);
        press(KEY_V); run(0.1);
        int differ = 0; for (size_t i = 0; i < on.size(); i++) if (on[i] != game.output()[i]) differ++;
        check("the landing map's peoples: off as it opens, the places of the records held, all of them the whole map (R-415)",
              offFirst && marksOn == 1 && placesNone == 0 && placesSome == (int)ranks.size() && waysSome == 0 && allSaid && placesAll == (int)P->plans[0].places.size() && waysAll > 0 && landAll > 0 && capitalAll == 1 && rings >= 1 && capitalZoom == 1 && differ > 0,
              fmt("off at first %s; on: the mark %d, places %d with none held; %d held: %d places drawn of %zu homes, %d ways; all %d held: %d places of %zu, %d ways, %d land pixels, the capital named %d; the zoom: %d rings, the capital %d, %d pixels the layer's",
                  offFirst ? "yes" : "NO", marksOn, placesNone, (n0 + 8) / 9, placesSome, ranks.size(), waysSome, n0, placesAll, P->plans[0].places.size(), waysAll, landAll, capitalAll, rings, capitalZoom, differ));
    }
    {   // B-422: the landing map's cursor asked for the drainage tiles of every place it stepped to (Shift held: 46 threads, every core busy, the
        // Enter after it three seconds): held, it asks for none of the places it passes; at rest, 0.4 s on, for the place it stopped at
        StarSystem fs; int fb = -1;
        const bool have = firstBodyOfType(PT_FELISIAN, fs, fb);
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
        auto quiet = [&]() { int n = drainageTilesBuilt(); for (double t0 = nowSec(); nowSec() - t0 < 20;) { std::this_thread::sleep_for(std::chrono::milliseconds(600)); const int m = drainageTilesBuilt(); if (m == n) break; n = m; } return n; };
        bool readyFirst = false, readyRest = false; int during = -1, rested = -1, built0 = 0;
        if (have) {
            game.testParkAt(fs.star, fb); run(0.1);
            game.testLandCursor(20.5, 30.5); run(0.5);
            for (double t0 = nowSec(); nowSec() - t0 < 30 && !game.testDrainageReady();) run(1.0 / 30);
            readyFirst = game.testDrainageReady();
            built0 = quiet();
            in.down[KEY_LEFT_SHIFT] = true;
            for (int i = 0; i < 10; i++) { in.down[KEY_RIGHT] = true; game.frame(in, 1.0 / 30); in.newFrame(); }   // six degrees a frame
            in.down[KEY_RIGHT] = false; in.down[KEY_LEFT_SHIFT] = false;
            run(0.2);   // at rest, short of the 0.4 s
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            during = drainageTilesBuilt() - built0;
            run(0.4);
            for (double t0 = nowSec(); nowSec() - t0 < 30 && !game.testDrainageReady();) run(1.0 / 30);
            readyRest = game.testDrainageReady();
            rested = drainageTilesBuilt() - built0;
        }
        check("the landing map's cursor: no drainage tiles for the places it passes, the place it rests at's 0.4 s on (B-422)", have && readyFirst && during == 0 && readyRest && rested > 0,
              fmt("%s: the first place's tiles %s; ten steps of six degrees and 0.2 s at rest: %d tiles built in 1.5 s; 0.4 s more at rest: the place's tiles %s (%d built)",
                  have ? fs.bodies[fb].name.c_str() : "no felisian world", readyFirst ? "in" : "NOT IN", during, readyRest ? "in" : "NOT IN", rested));
    }
}

void townChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    struct T { std::string what; RuinSpec sp; Culture cu; int rank = -1, worldAge = 1, histAge = -1, leftAge = -1; };
    std::vector<T> towns;
    const double t0 = nowSec();
    double worstMs = 0; std::string worstWhat;
    int goneN = 0, goneStanding = 0; std::string goneWhat;   // B-414: the places left before their people's last age
    for (int a = 0; a < AGE_COUNT; a++) {
        Star st; StarSystem sys; if (!systemAt(AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, st, sys, true)) continue;
        if (AGE_WORLDS[a].body >= (int)sys.bodies.size()) continue;
        const BodyGen g = BodyGen::make(sys.bodies[AGE_WORLDS[a].body]);
        if (!worldHadCivilisation(g)) continue;
        const int people = std::min(AGE_WORLDS[a].which, std::max(1, peoplesOf(g)) - 1);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g); const PeoplePlan& pp = P->plans[people];
        const Culture cu = cultureOf(g, people);
        std::shared_ptr<const History> H = historyOfWorld(g, people);
        if (H) for (const HistoryPlace& hp : H->places) {   // B-414: left before the last age began: nothing of it on the ground
            if (hp.plan < 0 || hp.plan >= (int)pp.places.size() || hp.fate != HP_EARLY || hp.ended >= ageFromOf(H->profile)) continue;
            goneN++;
            RuinSpec q; if (ruinOfCell(g, pp.places[hp.plan].gLat, pp.places[hp.plan].gLon, q, false) && q.rank == hp.plan) { goneStanding++; if (goneWhat.size() < 200) goneWhat += fmt("%s%s's rank %d still stands", goneWhat.empty() ? "" : "; ", sys.bodies[AGE_WORLDS[a].body].name.c_str(), hp.plan); }
        }
        std::vector<int> ranks;
        if (!pp.places.empty()) ranks.push_back(0);
        for (const PlanPlace& p : pp.places) if (p.sclass == SC_CITY) { ranks.push_back(p.rank); break; }
        if (H) for (int i = 1; i < (int)pp.places.size() && i < (int)H->places.size(); i++) if (H->places[i].fate == HP_EARLY && H->places[i].ended >= ageFromOf(H->profile) && townOrGreater(pp.places[i].sclass)) { ranks.push_back(i); break; }   // B-414: left in the last age (one left before it is gone)
        for (int rk : ranks) {
            const PlanPlace& p = pp.places[rk];
            T t; t.cu = cu; t.rank = rk; t.worldAge = cu.age;
            const double ts = nowSec();
            if (!ruinOfCell(g, p.gLat, p.gLon, t.sp, true)) continue;
            const double ms = (nowSec() - ts) * 1000;
            t.what = fmt("%s's %s", sys.bodies[AGE_WORLDS[a].body].name.c_str(), rk == 0 ? "capital" : (H && rk < (int)H->places.size() && H->places[rk].fate == HP_EARLY ? "place left early" : "city"));
            if (ms > worstMs) { worstMs = ms; worstWhat = t.what; }
            if (H && rk < (int)H->places.size()) {
                t.histAge = H->places[rk].age;
                if (H->places[rk].fate == HP_EARLY) { const int e = H->eraAt(H->places[rk].ended); t.leftAge = e >= 0 ? H->eras[e].age : H->places[rk].age; }
            }
            towns.push_back(t);
        }
        if (!pp.places.empty()) {   // the countryside round the capital: its first town and village
            bool gt = false, gv = false;
            for (int ring = 1; ring < 30 && !(gt && gv); ring++)
                for (int dl = -ring; dl <= ring && !(gt && gv); dl++)
                    for (int dn = -ring; dn <= ring && !(gt && gv); dn++) {
                        if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                        const int row = pp.places[0].gLat + dl, col = ruinCellLonIn(g, pp.places[0].gLat, pp.places[0].gLon, row) + dn;
                        RuinSpec q; if (!ruinOfCell(g, row, col, q, false) || q.rank >= 0 || q.people != people) continue;
                        if (!((q.sclass == SC_TOWN && !gt) || (q.sclass == SC_VILLAGE && !gv))) continue;
                        (q.sclass == SC_TOWN ? gt : gv) = true;
                        T t; t.cu = cu; t.worldAge = cu.age;
                        if (!ruinOfCell(g, row, col, t.sp, true)) continue;
                        t.what = fmt("%s's countryside %s", sys.bodies[AGE_WORLDS[a].body].name.c_str(), q.sclass == SC_TOWN ? "town" : "village");
                        towns.push_back(t);
                    }
        }
    }
    // every street pattern, in each age that lays streets, on the medieval world
    int patterns = 0;
    {
        Star st; StarSystem sys; const AgeWorld& mw = AGE_WORLDS[AGE_MEDIEVAL];
        if (starInSector(mw.sx, 0, mw.sz, st, true)) {
            sys.generate(st);
            const BodyGen g = BodyGen::make(sys.bodies[mw.body]);
            double la = 0.2, lo = 0.7; planPlacePoint(g, 0, la, lo);
            int gLat0, gLon0; ruinCellOf(g, la, lo, gLat0, gLon0);
            for (int age = AGE_MEDIEVAL; age <= AGE_MODERN; age++)
                for (int pat = 0; pat < 7; pat++) {
                    Culture c = cultureAs(g, 0, age, 300);
                    c.genome.set(GV_STREETS, pat); cultureWays(c, c.seed);
                    T t; t.cu = c; t.worldAge = age;
                    settlementOf(g, gLat0 + 3 * pat, gLon0 + 5 * (age - AGE_MEDIEVAL), c, SC_TOWN, t.sp);
                    t.what = fmt("a %s town of %s streets", AGE_NAMES[age], genomeValue(c.genome, GV_STREETS).c_str());
                    towns.push_back(t); patterns++;
                }
        }
    }
    // 1. no building across a street
    int crossing = 0, buildingsN = 0, streetsN = 0; std::string crossWhat;
    for (const T& t : towns) {
        streetsN += (int)t.sp.streets.size();
        for (const Building& b : t.sp.buildings) {
            buildingsN++;
            if (b.kind == BK_GATE || b.kind == BK_MAGLEV) continue;
            const int k = buildingStreet(t.sp, b, -0.05);
            if (k >= 0) { crossing++; if (crossWhat.size() < 300) crossWhat += fmt("%s%s %s (%s) across a %s", crossWhat.empty() ? "" : "; ", t.what.c_str(), BUILDING_KIND_NAMES[b.kind], DISTRICT_NAMES[clampi(b.district, 0, DI_COUNT - 1)], STREET_RANK_NAMES[t.sp.streets[k].rank]); }
        }
    }
    check("towns: no building across a street (T-02)", crossing == 0 && towns.size() >= 20 && streetsN > 2000,
          fmt("%zu towns (%d of every street pattern in three ages), %d buildings, %d streets: %d across one%s%s; the slowest laid out in %.1f ms (%s); %.1f s in all", towns.size(), patterns, buildingsN, streetsN, crossing, crossing ? ": " : "", crossWhat.c_str(), worstMs, worstWhat.c_str(), nowSec() - t0));
    // 2. the streets walkable: every street's line clear of the colliders where the explorer walks (but where a fallen piece lies across it), the
    // web joined
    int points = 0, blocked = 0, unjoined = 0; double worstShare = 0; std::string walkWhat;
    for (const T& t : towns) {
        if (t.sp.streets.empty()) continue;
        std::vector<Form> el; ruinElements(t.sp, t.cu, 0, el);
        std::vector<FormDisc> discs; std::vector<char> fell;
        std::vector<int> ownerB; std::vector<const Form*> ownerF;
        for (const Form& e : el) { const size_t n0 = discs.size(); formColliders(e, discs); for (size_t q = n0; q < discs.size(); q++) { fell.push_back(e.tilt > 0.3 || e.part == 3); ownerB.push_back(e.building); ownerF.push_back(&e); } }
        std::unordered_map<int64_t, std::vector<int>> grid;
        auto key = [](int i, int j) { return ((int64_t)i << 32) ^ (int64_t)(uint32_t)j; };
        for (int q = 0; q < (int)discs.size(); q++) { const FormDisc& d = discs[q]; if (d.y0 > 1.8 || d.y1 < 0.3) continue; for (int j = (int)std::floor((d.z - d.r - 0.4) / 8); j <= (int)std::floor((d.z + d.r + 0.4) / 8); j++) for (int i = (int)std::floor((d.x - d.r - 0.4) / 8); i <= (int)std::floor((d.x + d.r + 0.4) / 8); i++) grid[key(i, j)].push_back(q); }
        const double plaza = t.sp.plazas.empty() ? 0 : t.sp.plazas[0].r;
        int tp = 0, tb = 0;
        for (const Street& s : t.sp.streets) {
            const double L = std::hypot(s.bx - s.ax, s.bz - s.az);
            for (double u = 0.75; u < L; u += 1.5) {
                const double x = s.ax + (s.bx - s.ax) * u / L, z = s.az + (s.bz - s.az) * u / L;
                if (std::hypot(x, z) < plaza + 2 || std::hypot(x, z) > t.sp.size + 25) continue;
                tp++;
                auto it = grid.find(key((int)std::floor(x / 8), (int)std::floor(z / 8)));
                if (it == grid.end()) continue;
                for (int q : it->second) { const FormDisc& d = discs[q]; if (std::hypot(d.x - x, d.z - z) < d.r + 0.35) { if (!fell[q]) { tb++; if (getenv("TOWNS_VERBOSE")) { const Building* ob = ownerB[q] >= 0 && ownerB[q] < (int)t.sp.buildings.size() ? &t.sp.buildings[ownerB[q]] : nullptr; printf("    blocked: %s at %.1f %.1f on a %s (half %.1f, dist %.2f); disc r %.2f y %.1f..%.1f of a %s %d part %d mat %d; building %s at %.1f %.1f hw %.1f hd %.1f heading %.2f found %d district %d gap %.2f\n", t.what.c_str(), x, z, STREET_RANK_NAMES[s.rank], s.half, std::hypot(d.x - x, d.z - z), d.r, d.y0, d.y1, FORM_KIND_NAMES[ownerF[q]->kind], ownerF[q]->sub, ownerF[q]->part, ownerF[q]->material, ob ? BUILDING_KIND_NAMES[ob->kind] : "-", ob ? ob->x : 0, ob ? ob->z : 0, ob ? ob->hw : 0, ob ? ob->hd : 0, ob ? ob->heading : 0, ob ? ob->found : -1, ob ? ob->district : -1, ob ? streetRectGap(s, ob->x, ob->z, ob->heading, ob->hw, ob->hd) : 0); }
                    if (walkWhat.size() < 260) walkWhat += fmt("%s%s: a %s's piece on a %s at %.0f %.0f", walkWhat.empty() ? "" : "; ", t.what.c_str(), ownerB[q] >= 0 && ownerB[q] < (int)t.sp.buildings.size() ? BUILDING_KIND_NAMES[t.sp.buildings[ownerB[q]].kind] : "grave", STREET_RANK_NAMES[s.rank], x, z); } break; } }
            }
        }
        points += tp; blocked += tb;
        if (tp) worstShare = std::max(worstShare, (double)tb / tp);
        if (streetsJoined(t.sp.streets) < 0.9) { unjoined++; if (walkWhat.size() < 300) walkWhat += fmt("%s%s: the web %.0f%% joined", walkWhat.empty() ? "" : "; ", t.what.c_str(), 100 * streetsJoined(t.sp.streets)); }
    }
    check("towns: the streets walkable, the colliders off them and the web joined (T-02)", points > 20000 && blocked == 0 && unjoined == 0,
          fmt("%d points along the streets, %d under a standing piece's collider (the worst town %.2f%%), %d webs under 90%% joined%s%s", points, blocked, 100 * worstShare, unjoined, walkWhat.empty() ? "" : ": ", walkWhat.c_str()));
    // 3. B-414: one age a world: every town all of its people's age (one layer, every building but a work's piece of that age); a place left before
    // the people's last age not on the ground; one left early in it of that age and weathered longer
    int oneAge = 0, mixed = 0, left = 0, leftBad = 0; std::string ageWhat;
    for (const T& t : towns) {
        int other = 0;
        for (const TownLayer& L : t.sp.layers) if (L.age != t.worldAge) other++;
        for (const Building& b : t.sp.buildings) if (b.kind != BK_MAGLEV && b.age >= 0 && b.age != t.worldAge) other++;
        if (other) { mixed++; if (ageWhat.size() < 300) ageWhat += fmt("%s%s (%s): %d layers or buildings of another age", ageWhat.empty() ? "" : "; ", t.what.c_str(), AGE_NAMES[t.worldAge], other); }
        else oneAge++;
        if (t.leftAge >= 0) {
            left++;
            int n = 0; double dOld = 0, dOwn = 0;
            for (const Building& b : t.sp.buildings) {
                if (b.kind == BK_MAGLEV) continue;
                if (++n == 1) { Building b0 = b; b0.before = 0; dOld = buildingDecay(b, cultureAtAge(t.cu, t.worldAge, b.before), MT_MASONRY); dOwn = buildingDecay(b0, cultureAtAge(t.cu, t.worldAge, 0), MT_MASONRY); }
            }
            if (!(t.leftAge == t.worldAge && t.sp.before > 0 && n > 0 && (dOld > dOwn || dOwn >= 1))) { leftBad++; ageWhat += fmt("%s%s: left in the %s age, %.0f years before the end, its decay %.2f against %.2f", ageWhat.empty() ? "" : "; ", t.what.c_str(), AGE_NAMES[t.leftAge], t.sp.before, dOld, dOwn); }
            else if (ageWhat.size() < 400) ageWhat += fmt("%s%s: %d buildings, left %.0f years before the end", ageWhat.empty() ? "" : "; ", t.what.c_str(), n, t.sp.before);
        }
    }
    check("towns: one age a world, a place left before it gone, one left early in it weathered longer (B-414)", mixed == 0 && oneAge == (int)towns.size() && goneN >= 2 && goneStanding == 0 && left >= 1 && leftBad == 0,
          fmt("%d towns all of their people's age, %d mixed; %d places left before their people's last age, %d standing%s%s; %d left early in it: %s", oneAge, mixed, goneN, goneStanding, goneWhat.empty() ? "" : ": ", goneWhat.c_str(), left, ageWhat.c_str()));
    // 4. the districts and works by the age, the harbours of three ages, the ground's marks
    std::string dist; int distBad = 0;
    for (const T& t : towns) {
        if (t.rank != 0) continue;
        int byD[DI_COUNT] = {}, highrises = 0, mills = 0, stations = 0, gates = 0, rings = 0;
        for (const Building& b : t.sp.buildings) { byD[clampi(b.district, 0, DI_COUNT - 1)]++; highrises += b.kind == BK_HIGHRISE; mills += b.kind == BK_MILL; stations += b.kind == BK_STATION; gates += b.kind == BK_GATE; }
        for (const Street& s : t.sp.streets) rings += s.rank == SR_RING;
        bool ok = true;
        if (t.worldAge == AGE_MODERN) ok = byD[DI_TOWERS] >= 10 && highrises >= 5 && byD[DI_SUBURBS] >= 20 && rings > 0 && !t.sp.fields.empty();
        else if (t.worldAge == AGE_INDUSTRIAL) ok = byD[DI_WORKS] >= 5 && mills >= 1 && stations >= 1 && !t.sp.fields.empty();
        else if (t.worldAge == AGE_MEDIEVAL) ok = (t.sp.walled ? gates >= 2 : true) && byD[DI_HOUSES] >= 50 && !t.sp.fields.empty();
        if (!ok) distBad++;
        if (t.worldAge >= AGE_MEDIEVAL && t.worldAge <= AGE_MODERN) dist += fmt("%s%s: %d towers (%d tall), %d suburbs, %d works (%d mills, %d stations), %d houses, %d old core, %d gates, a ring road %s, %zu fields%s", dist.empty() ? "" : "; ", AGE_NAMES[t.worldAge], byD[DI_TOWERS], highrises, byD[DI_SUBURBS], byD[DI_WORKS], mills, stations, byD[DI_HOUSES], byD[DI_CORE], gates, rings ? "yes" : "no", t.sp.fields.size(), ok ? "" : " (NOT as its age)");
    }
    // the harbours: a town of each age on a coast of the plan (the first that gets one)
    int harbourKinds = 0; std::string hw;
    {
        Star st; StarSystem sys; struct { int64_t sx, sz; int bi; } TW = {151, 25, 1};
        if (starInSector(TW.sx, 0, TW.sz, st, true)) {
            sys.generate(st);
            const BodyGen g = BodyGen::make(sys.bodies[TW.bi]);
            std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
            for (int age = AGE_MEDIEVAL; age <= AGE_MODERN; age++) {
                const Culture c = cultureAs(g, 0, age, 300);
                bool got = false;
                for (const PlanPlace& p : P->plans[0].places) {
                    if (!p.coast || got) continue;
                    for (int dl = -2; dl <= 2 && !got; dl++) for (int dn = -2; dn <= 2 && !got; dn++) {
                        RuinSpec sp; settlementOf(g, p.gLat + dl, p.gLon + dn, c, SC_TOWN, sp);
                        if (!sp.harbour) continue;
                        int cranes = 0, boxes = 0; for (const Building& b : sp.buildings) { cranes += b.kind == BK_CRANE; boxes += b.kind == BK_CONTAINERS; }
                        const bool right = age == AGE_MEDIEVAL ? sp.harbourKind == 0 : (age == AGE_INDUSTRIAL ? sp.harbourKind == 1 && cranes > 0 : sp.harbourKind == 2 && cranes > 0 && boxes > 0);
                        if (right) { got = true; harbourKinds++; hw += fmt("%s%s: %s (%d cranes, %d stacks)", hw.empty() ? "" : "; ", AGE_NAMES[age], featureList(sp).c_str(), cranes, boxes); }
                    }
                }
                if (!got) hw += fmt("%s%s: NO harbour of its age", hw.empty() ? "" : "; ", AGE_NAMES[age]);
            }
        }
    }
    // the marks: a town's street and a field drawn into a site's vertices
    bool paved = false, toned = false;
    for (const T& t : towns) {
        if (t.rank != 0 || t.worldAge != AGE_MEDIEVAL || t.sp.streets.empty()) continue;
        Star st; StarSystem sys; const AgeWorld& mw = AGE_WORLDS[AGE_MEDIEVAL];
        if (!starInSector(mw.sx, 0, mw.sz, st, true)) break;
        sys.generate(st);
        SurfaceSite site; site.init(&sys, mw.body, t.sp.lat, t.sp.lon, 1000);
        DrainageOff off;
        const Street* s = nullptr; for (const Street& q : t.sp.streets) if (q.rank >= SR_STREET && std::hypot(q.bx - q.ax, q.bz - q.az) > 8) { s = &q; break; }
        if (!s) break;
        const double mx = (s->ax + s->bx) * 0.5, mz = (s->az + s->bz) * 0.5;
        const TerrainVertex before = site.sampleAt(mx, mz, 4);
        double fx = 0, fz = 0; bool haveField = !t.sp.fields.empty(); if (haveField) { fx = t.sp.fields[0].x; fz = t.sp.fields[0].z; }
        const TerrainVertex fBefore = site.sampleAt(fx, fz, 16);
        site.addTownMarks(t.sp, 0, 0, t.cu.ageYears, t.cu.wet, t.cu.roadWear);
        const TerrainVertex after = site.sampleAt(mx, mz, 4), fAfter = site.sampleAt(fx, fz, 16);
        paved = after.road > 0 && (after.material != before.material || after.albedo != before.albedo);
        toned = haveField && (fAfter.albedo != fBefore.albedo || fAfter.veg != fBefore.veg);
        break;
    }
    check("towns: the districts and works of each age (T-02)", distBad == 0 && !dist.empty(), "the capitals: " + dist);
    check("towns: a harbour of each age, the streets and fields in the ground (T-02)", harbourKinds == 3 && paved && toned,
          fmt("harbours: %s; a street's vertex %s, a field's %s", hw.c_str(), paved ? "paved" : "NOT paved", toned ? "toned" : "NOT toned"));
}
static void planLine(std::vector<uint32_t>& img, int W, int H, int x0, int y0, int x1, int y1, uint32_t c, int width, bool dotted = false) {
    const int steps = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
    for (int q = 0; q <= steps; q++) {
        if (dotted && (q / 3) % 2) continue;
        const int x = x0 + (steps ? (x1 - x0) * q / steps : 0), y = y0 + (steps ? (y1 - y0) * q / steps : 0);
        for (int dy = 0; dy < width; dy++) for (int dx = 0; dx < width; dx++) { const int xx = x + dx, yy = y + dy; if (xx >= 0 && xx < W && yy >= 0 && yy < H) img[(size_t)yy * W + xx] = c; }
    }
}
static void planProject(const Vec3& u, int W, int H, double& x, double& y) { double la, lo; StarSystem::latLonFromBody(u, la, lo); x = (lo + PI) / TAU * W; y = (0.5 - la / PI) * H; }
static void planMapImage(const BodyGen& g, const PlanGrid& G, const PlanetMap& m, const PeoplePlan* pps, int nP, int W, int H, std::vector<uint32_t>& img) {
    img.assign((size_t)W * H, 0);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            const double lon = (x + 0.5) / W * TAU - PI, lat = (0.5 - (y + 0.5) / H) * PI, dl = TAU / PlanetMap::W;
            const int i = m.texelIndex(lon, lat), mat = m.material[i];
            const double alb = m.albedo[i] / 255.0;
            const double gx = m.heightAt(lon + dl, lat) - m.heightAt(lon - dl, lat), gy = m.heightAt(lon, lat + dl) - m.heightAt(lon, lat - dl);
            const double shade = clampd(1.0 + (gy - gx) / 1600.0, 0.55, 1.45);
            const int cell = G.cellOf(lat, lon);
            RGB c;
            if (!G.land[cell]) c = RGB(0.07f, 0.14f, 0.30f) * (float)(0.75 + 0.25 * shade);
            else c = g.matColor[mat] * (float)((0.35 + 0.8 * alb) * shade);
            for (int k = 0; k < nP; k++) {
                const PeoplePlan& pp = pps[k];
                if (pp.territory.empty() || !pp.territory[cell]) continue;
                const RGB tint = k == 0 ? RGB(1.0f, 0.85f, 0.35f) : RGB(0.35f, 0.95f, 1.0f);
                c = lerp(c, tint, 0.28f);
                const int cr = G.cellOf(lat, lon + TAU / W * 1.5), cd = G.cellOf(lat - PI / H * 1.5, lon);   // the frontier's outline
                if (!pp.territory[cr] || !pp.territory[cd]) c = tint;
            }
            img[(size_t)y * W + x] = rgb(clampi((int)(c.r * 255), 0, 255), clampi((int)(c.g * 255), 0, 255), clampi((int)(c.b * 255), 0, 255));
        }
    for (int k = 0; k < nP; k++) {
        const PeoplePlan& pp = pps[k];
        for (const PlanEdge& e : pp.edges) {
            const Vec3 a = pp.places[e.a].unit, b = pp.places[e.b].unit;
            const int n = std::max(2, (int)(e.lengthKm / 25));
            double px = 0, py = 0;
            for (int s = 0; s <= n; s++) {
                const double t = (double)s / n, w = std::acos(clampd(dot(a, b), -1, 1));
                const Vec3 p = w < 1e-9 ? a : normalize(a * (std::sin((1 - t) * w) / std::sin(w)) + b * (std::sin(t * w) / std::sin(w)));
                double x, y; planProject(p, W, H, x, y);
                if (s && std::fabs(x - px) < W / 2) planLine(img, W, H, (int)px, (int)py, (int)x, (int)y, PLAN_WAY_COLOURS[e.kind], e.trunk ? 2 : 1, e.kind == NK_SEA);
                px = x; py = y;
            }
        }
        for (const PlanPlace& p : pp.places) {
            double x, y; planProject(p.unit, W, H, x, y);
            int r = 1; uint32_t c = rgb(255, 230, 90);
            switch (p.sclass) {
                case SC_CAPITAL: r = 4; c = rgb(255, 255, 255); break;
                case SC_CITY: r = 2; c = rgb(255, 70, 60); break;
                case SC_MONUMENT: r = 4; c = rgb(80, 255, 255); break;
                case SC_VILLAGE: r = 1; c = rgb(255, 170, 60); break;
                default: if (pp.age == AGE_FUTURISTIC) { r = 3; c = rgb(255, 90, 255); } break;
            }
            for (int dy = -r - 1; dy <= r + 1; dy++) for (int dx = -r - 1; dx <= r + 1; dx++) {
                const int xx = (int)x + dx, yy = (int)y + dy; if (xx < 0 || xx >= W || yy < 0 || yy >= H) continue;
                img[(size_t)yy * W + xx] = (std::abs(dx) > r || std::abs(dy) > r) ? rgb(0, 0, 0) : c;
            }
        }
    }
}
// whether every place is reached from the capital by the ways (the crossings over water among them)
static bool planConnected(const PeoplePlan& pp) {
    const int n = (int)pp.places.size();
    if (n <= 1) return true;
    std::vector<std::vector<int>> adj(n);
    for (const PlanEdge& e : pp.edges) { adj[e.a].push_back(e.b); adj[e.b].push_back(e.a); }
    std::vector<char> seen(n, 0); std::vector<int> q{0}; seen[0] = 1;
    for (size_t h = 0; h < q.size(); h++) for (int v : adj[q[h]]) if (!seen[v]) { seen[v] = 1; q.push_back(v); }
    return (int)q.size() == n;
}
static std::string planSummary(const PeoplePlan& pp) {
    int ways[NK_COUNT] = {0}; double km[NK_COUNT] = {0};
    for (const PlanEdge& e : pp.edges) { ways[e.kind]++; km[e.kind] += e.lengthKm; }
    std::string w;
    for (int k = 0; k < NK_COUNT; k++) if (ways[k]) w += fmt("%s%d %s (%.0f km)", w.empty() ? "" : ", ", ways[k], NET_KIND_NAMES[k], km[k]);
    const int cities = pp.count(SC_CITY), towns = pp.count(SC_TOWN), villages = pp.count(SC_VILLAGE), works = pp.count(SC_MONUMENT);
    double pop = 0; for (const PlanPlace& p : pp.places) pop += p.population;
    return fmt("%d places (%d capital, %d cities, %d towns, %d seats, %d works) of %s, %s; held %.0f%% of the land (%.2g km2); %s; %.0f ms",
               (int)pp.places.size(), pp.count(SC_CAPITAL), cities, towns, villages, works, peopleCount(pop).c_str(), w.empty() ? "no ways" : w.c_str(), pp.heldShare * 100, pp.areaKm2,
               pp.edges.empty() && pp.age == AGE_FAR_FUTURE ? "no ways in the far future" : (planConnected(pp) ? "connected" : "NOT CONNECTED"), pp.buildMs);
}
int runPlan(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    const int which = takePeopleArg(argc, argv);
    if (argc > 5 && std::string(argv[2]) == "view") {   // `plan view <sx> <sz> <body> [second]`: the people's capital through the telescope from above it
        Star st; StarSystem sys; if (!systemAt(atoll(argv[3]), 0, atoll(argv[4]), st, sys, true)) { printf("no star\n"); return 1; }
        const int bi = atoi(argv[5]);
        if (bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
        const Body& b = sys.bodies[bi]; const BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) { printf("%s had no people\n", b.name.c_str()); return 1; }
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        const PlanPlace& cap = P->plans[std::min(which, P->peoples - 1)].places[0];
        double t = 7000, best = -2;   // the capital under the highest sun of a turn
        for (int k = 0; k < 192; k++) {
            const double tt = 7000 + k * std::fabs(b.rotPeriod) / 192;
            const Vec3 bp = sys.bodyPos(bi, tt), cw = sys.surfacePointWorld(bi, tt, cap.lat, cap.lon, 0);
            const double up = dot(normalize(cw - bp), normalize(sys.star.pos - bp));
            if (up > best) { best = up; t = tt; }
        }
        const Vec3 bp = sys.bodyPos(bi, t), cw = sys.surfacePointWorld(bi, t, cap.lat, cap.lon, 0), up = normalize(cw - bp);
        const Vec3 side = normalize(cross(up, Vec3(0, 1, 0)));
        const Vec3 ship = bp + normalize(up + side * 0.25) * b.radiusKm * 3.5;
        SpaceRenderer sr; StarNeighborhood nb;
        printf("%s: the %s capital at %.2f %.2f (a core of %.0f m), the sun %.0f degrees up; the ship %.0f km over it\n", b.name.c_str(), AGE_NAMES[P->plans[std::min(which, P->peoples - 1)].age], cap.lat / DEG, cap.lon / DEG, cap.radiusM, std::asin(clampd(best, -1, 1)) / DEG, length(ship - cw));
        std::string name = b.name; for (char& ch : name) if (ch == ' ') ch = '_';
        for (double zoom : {64.0, 256.0, 1024.0}) {
            double rz = 0;
            const std::string fn = fmt("shots/tests/plan_view_%s_x%.0f.png", name.c_str(), zoom), fm = fmt("shots/tests/plan_view_%s_x%.0f_map.png", name.c_str(), zoom);
            telescopeFrame(sr, nb, sys, t, ship, cw, zoom, bi, -1, fm.c_str(), &rz, false);
            const double ms = telescopeFrame(sr, nb, sys, t, ship, cw, zoom, bi, -1, fn.c_str(), &rz, true);
            printf("  x%.0f: %.1f ms -> %s (%s: the map alone)\n", zoom, ms, fn.c_str(), fm.c_str());
        }
        return 0;
    }
    std::vector<AgeWorld> worlds;
    const bool ages = argc > 2 && std::string(argv[2]) == "ages";
    if (ages) for (int a = 0; a < AGE_COUNT; a++) worlds.push_back(AGE_WORLDS[a]);
    else if (argc > 4) worlds.push_back(AgeWorld{atoll(argv[2]), atoll(argv[3]), atoi(argv[4]), which});
    else { printf("plan <sx> <sz> <body> [second] | plan ages\n"); return 1; }
    const int W = 1024, H = 512;
    for (const AgeWorld& aw : worlds) {
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)aw.sx, (long long)aw.sz); continue; }
        if (aw.body >= (int)sys.bodies.size()) { printf("no body %d\n", aw.body); continue; }
        const Body& b = sys.bodies[aw.body];
        const BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) { printf("%s had no people\n", b.name.c_str()); continue; }
        PlanetMap m; m.generate(g);
        double t0 = nowSec();
        const PlanGrid G = readPlanGrid(g, true);
        const double gridMs = (nowSec() - t0) * 1000;
        const int people = std::min(aw.which, peoplesOf(g) - 1);
        const int own = peopleAgeOf(g, people).age;
        std::string name = b.name; for (char& ch : name) if (ch == ' ') ch = '_';
        printf("%s (%s, R %.0f km, %d people%s), the plan of people %d (its own age %s): the reading %.0f ms on the workers (%d cells of %.0f km, land %.0f%%)\n",
               b.name.c_str(), KINDS[b.type].name, b.radiusKm, peoplesOf(g), peoplesOf(g) > 1 ? "s" : "", people, AGE_NAMES[own], gridMs, G.cells, G.sizeKm(), 100 * G.landKm2 / (4 * PI * G.R * G.R));
        std::vector<std::vector<uint32_t>> imgs(AGE_COUNT);
        for (int a = 0; a < AGE_COUNT; a++) {
            if (ages && a != own) continue;
            PeoplePlan pps[2];
            const int nP = peoplesOf(g);
            for (int k = 0; k < nP; k++) planPeople(g, G, k, k == people ? a : peopleAgeOf(g, k).age, pps[k]);
            const PeoplePlan& pp = pps[people];
            printf("  %-11s %s\n", AGE_NAMES[a], planSummary(pp).c_str());
            if (!pp.places.empty()) {
                const PlanPlace& c = pp.places[0];
                printf("              the capital at %.2f %.2f: score %.2f, %s%s, core %.0f m, %d buildings, %s people\n", c.lat / DEG, c.lon / DEG, c.score, c.coast ? "on a coast" : "inland", c.river ? ", on a river" : "", c.radiusM, c.buildings, peopleCount(c.population).c_str());
            }
            planMapImage(g, G, m, pps, nP, W, H, imgs[a]);
            RGBCanvas cv{imgs[a].data(), W, H, 1};
            drawTextShadow(cv, 4, 4, fmt("%s: %s PEOPLE (AS IF), %s", upper(b.name).c_str(), upper(AGE_NAMES[a]).c_str(), upper(planSummary(pp).substr(0, 140)).c_str()).c_str(), rgb(240, 240, 220), rgb(0, 0, 0));
            const std::string fn = fmt("shots/tests/plan_%s_%s.png", name.c_str(), a == AGE_FAR_FUTURE ? "far_future" : AGE_NAMES[a]);
            writePNG(fn.c_str(), imgs[a].data(), W, H);
        }
        if (!ages) {   // the six on one sheet, half size
            const int SW = 3 * W / 2, SH = 2 * H / 2;
            std::vector<uint32_t> sheet((size_t)SW * SH, 0);
            for (int a = 0; a < AGE_COUNT; a++) {
                const int ox = (a % 3) * W / 2, oy = (a / 3) * H / 2;
                for (int y = 0; y < H / 2; y++) for (int x = 0; x < W / 2; x++) sheet[(size_t)(oy + y) * SW + ox + x] = imgs[a][(size_t)(2 * y) * W + 2 * x];
                RGBCanvas cv{sheet.data(), SW, SH, 1};
                drawTextShadow(cv, ox + 4, oy + 4, upper(AGE_NAMES[a]).c_str(), rgb(255, 255, 255), rgb(0, 0, 0));
            }
            writePNG(fmt("shots/tests/plan_%s_ages.png", name.c_str()).c_str(), sheet.data(), SW, SH);
        }
        // the plan the game builds and keeps: on one thread, the world's reading and every people's plan
        worldPlanClearCache();
        t0 = nowSec();
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        printf("  the game's plan: %.0f ms on one thread (the reading %.0f ms), %zu + %zu places, %zu + %zu ways\n", (nowSec() - t0) * 1000, P->gridMs, P->plans[0].places.size(), P->plans[1].places.size(), P->plans[0].edges.size(), P->plans[1].edges.size());
    }
    return 0;
}

// T-02: a town's plan from above: its fields, its layers' rings, its streets by rank, its squares, its buildings by district (the walls red),
// a scale bar; `title` printed at the top
static void drawTownMap(const RuinSpec& sp, const std::string& path, const std::vector<std::string>& title, bool wide = false) {
    const int S = 1000;
    double ext = sp.size + 40;
    for (const Building& b : sp.buildings) ext = std::max(ext, std::hypot(b.x, b.z) + std::hypot(b.hw, b.hd) + 20);
    if (wide) for (const FieldPatch& f : sp.fields) ext = std::max(ext, std::hypot(f.x, f.z) + std::max(f.hw, f.hd) + 10);
    const double m = 2 * ext / S;   // metres a pixel
    std::vector<uint32_t> img((size_t)S * S, rgb(26, 30, 24));
    auto put = [&](int i, int j, uint32_t c, int a) {
        if (i < 0 || j < 0 || i >= S || j >= S) return;
        uint32_t& d = img[(size_t)j * S + i];
        auto ch = [&](int sh) { const int x = (d >> sh) & 255, y = (c >> sh) & 255; return (uint32_t)((x * (255 - a) + y * a) / 255); };
        d = 0xFF000000u | (ch(16) << 16) | (ch(8) << 8) | ch(0);
    };
    auto area = [&](double x, double z, double r, const std::function<bool(double, double)>& in, uint32_t c, int a) {
        const int i0 = (int)std::floor((x - r + ext) / m), i1 = (int)std::floor((x + r + ext) / m), j0 = (int)std::floor((ext - z - r) / m), j1 = (int)std::floor((ext - z + r) / m);
        for (int j = std::max(0, j0); j <= std::min(S - 1, j1); j++)
            for (int i = std::max(0, i0); i <= std::min(S - 1, i1); i++) { const double wx = -ext + (i + 0.5) * m, wz = ext - (j + 0.5) * m; if (in(wx - x, wz - z)) put(i, j, c, a); }
    };
    auto rect = [&](double x, double z, double h, double hw, double hd, uint32_t c, int a) {
        const double ch = std::cos(h), sh = std::sin(h), e = m * 0.5;
        area(x, z, std::hypot(hw, hd) + m, [&](double ex, double ez) { const double lx = ex * ch - ez * sh, lz = ex * sh + ez * ch; return std::fabs(lx) <= hw + e && std::fabs(lz) <= hd + e; }, c, a);
    };
    auto disc = [&](double x, double z, double r, uint32_t c, int a, double ring = -1) {
        area(x, z, r + m, [&](double ex, double ez) { const double d = std::hypot(ex, ez); return ring < 0 ? d <= r + m * 0.5 : std::fabs(d - r) <= ring * 0.5 + m * 0.5; }, c, a);
    };
    static const uint32_t FIELD[FI_COUNT] = {rgb(92, 84, 52), rgb(104, 98, 56), rgb(96, 90, 64), rgb(78, 96, 56), rgb(70, 104, 62), rgb(110, 92, 60)};
    for (const FieldPatch& f : sp.fields) {
        if (f.kind == FI_CIRCLE) { disc(f.x, f.z, f.hw, FIELD[f.kind], 150); disc(f.x, f.z, f.hw, rgb(150, 160, 110), 160, 1.5); continue; }
        rect(f.x, f.z, f.heading, f.hw, f.hd, FIELD[clampi(f.kind, 0, FI_COUNT - 1)], 120);
        if (f.kind == FI_STRIP) for (double u = -f.hw + 7; u < f.hw; u += 14) rect(f.x + std::cos(f.heading) * u, f.z - std::sin(f.heading) * u, f.heading, 1.2, f.hd, rgb(60, 56, 36), 120);
    }
    static const uint32_t AGEC[AGE_COUNT] = {rgb(160, 130, 90), rgb(200, 150, 90), rgb(190, 100, 80), rgb(130, 160, 200), rgb(160, 120, 220), rgb(220, 220, 220)};
    for (const TownLayer& L : sp.layers) disc(0, 0, L.r1, AGEC[clampi(L.age, 0, AGE_COUNT - 1)], 160, std::max(1.0, 2 * m));
    for (const Plaza& p : sp.plazas) disc(p.x, p.z, p.r, rgb(120, 114, 100), 200);
    static const uint32_t RANK[SR_COUNT] = {rgb(115, 110, 100), rgb(165, 158, 140), rgb(205, 195, 160), rgb(235, 220, 170), rgb(250, 205, 120)};
    for (const Street& s : sp.streets) {
        const double L = std::hypot(s.bx - s.ax, s.bz - s.az);
        rect((s.ax + s.bx) * 0.5, (s.az + s.bz) * 0.5, std::atan2(s.bx - s.ax, s.bz - s.az), std::max(s.half, m * 0.5), L * 0.5, s.paved ? RANK[clampi(s.rank, 0, SR_COUNT - 1)] : rgb(130, 100, 70), 255);
    }
    static const uint32_t DIST[DI_COUNT] = {rgb(185, 185, 185), rgb(245, 225, 120), rgb(215, 125, 80), rgb(150, 150, 165), rgb(215, 95, 60), rgb(90, 150, 215), rgb(235, 235, 250), rgb(125, 175, 115)};
    for (const Building& b : sp.buildings) {
        const bool wall = b.kind == BK_WALL || b.kind == BK_GATE || b.kind == BK_WALL_TOWER || b.kind == BK_PALISADE;
        uint32_t c = wall ? rgb(235, 80, 70) : DIST[clampi(b.district, 0, DI_COUNT - 1)];
        if (b.kind == BK_WALL_TOWER || b.kind == BK_HIGHRISE || b.kind == BK_STADIUM || b.kind == BK_GASOMETER || b.kind == BK_WATER_TOWER || b.kind == BK_RESERVOIR || b.kind == BK_HABITAT || b.kind == BK_SPIRE || b.kind == BK_ROTUNDA || b.kind == BK_HUT || b.kind == BK_STONE_CIRCLE)
            disc(b.x, b.z, std::max(b.hw, 1.5), c, 255, b.kind == BK_STONE_CIRCLE || b.kind == BK_STADIUM ? std::max(2.0, 2 * m) : -1);
        else rect(b.x, b.z, b.heading, std::max(b.hw, m * 0.6), std::max(b.hd, m * 0.6), c, 255);
    }
    // the scale bar: 100 m (or 500)
    const double bar = ext > 900 ? 500 : 100;
    for (int k = 0; k < (int)(bar / m); k++) for (int w = 0; w < 4; w++) put(20 + k, S - 24 + w, rgb(240, 240, 240), 255);
    RGBCanvas cv{img.data(), S, S, 1};
    drawText(cv, 20, S - 40, fmt("%.0f M", bar).c_str(), rgb(240, 240, 240), 2);
    for (size_t k = 0; k < title.size(); k++) drawText(cv, 12, 10 + (int)k * 22, title[k].c_str(), rgb(240, 240, 240), 2);
    writePNG(path.c_str(), img.data(), S, S);
}

// T-02: what a town holds, as lines (the harness's `towns` mode and the unit's details)
static std::vector<std::string> townLines(const RuinSpec& sp, double ms) {
    std::vector<std::string> L;
    std::string lay;
    for (const TownLayer& t : sp.layers) lay += fmt("%s%s to %.0f m", lay.empty() ? "" : ", ", AGE_NAMES[t.age], t.r1);
    L.push_back(fmt("%s, radius %.0f m, %s%s; founded %s%s", SETTLEMENT_CLASS_NAMES[sp.sclass], sp.size, lay.c_str(), sp.buildAge >= 0 ? fmt(" (built %s)", AGE_NAMES[sp.buildAge]).c_str() : "",
                    sp.founded ? fmt("year %d", sp.founded).c_str() : "unknown", sp.before > 0 ? fmt(", left %.0f years before the end", sp.before).c_str() : ""));
    int byD[DI_COUNT] = {}, byA[AGE_COUNT] = {}, walls = 0, gates = 0;
    for (const Building& b : sp.buildings) { byD[clampi(b.district, 0, DI_COUNT - 1)]++; byA[clampi(b.age, 0, AGE_COUNT - 1)]++; if (b.kind == BK_WALL || b.kind == BK_PALISADE) walls++; if (b.kind == BK_GATE) gates++; }
    std::string d, a;
    for (int k = 0; k < DI_COUNT; k++) if (byD[k]) d += fmt("%s%d %s", d.empty() ? "" : ", ", byD[k], k ? DISTRICT_NAMES[k] + 4 : "other");
    for (int k = 0; k < AGE_COUNT; k++) if (byA[k]) a += fmt("%s%d %s", a.empty() ? "" : ", ", byA[k], AGE_NAMES[k]);
    L.push_back(fmt("%zu buildings: %s; by age %s", sp.buildings.size(), d.c_str(), a.c_str()));
    double len[SR_COUNT] = {}; for (const Street& s : sp.streets) len[clampi(s.rank, 0, SR_COUNT - 1)] += std::hypot(s.bx - s.ax, s.bz - s.az);
    std::string sl; for (int k = 0; k < SR_COUNT; k++) if (len[k] > 0) sl += fmt("%s%s %.1f km", sl.empty() ? "" : ", ", STREET_RANK_NAMES[k], len[k] / 1000);
    int fk[FI_COUNT] = {}; for (const FieldPatch& f : sp.fields) fk[clampi(f.kind, 0, FI_COUNT - 1)]++;
    std::string fl; for (int k = 0; k < FI_COUNT; k++) if (fk[k]) fl += fmt("%s%d %s", fl.empty() ? "" : ", ", fk[k], FIELD_KIND_NAMES[k]);
    L.push_back(fmt("%zu streets (%s; joined %.0f%%), %zu squares; fields: %s", sp.streets.size(), sl.c_str(), 100 * streetsJoined(sp.streets), sp.plazas.size(), fl.empty() ? "none" : fl.c_str()));
    const std::string ft = featureList(sp);
    L.push_back(fmt("%s%s; laid out in %.1f ms", ft.empty() ? "no works of the water" : ft.c_str(), sp.walled ? fmt(", walled (%d stretches, %d gates)", walls, gates).c_str() : "", ms));
    return L;
}

// T-02: a people's capital through the surface's renderer: from the air over its edge, the light from the side (the plan: the old core, the
// streets, the districts, the fields round it), and from a street of its middle at the eye's height, looking along it
static void townViews(const AgeWorld& aw, int scale) {
    Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) return;
    if (aw.body >= (int)sys.bodies.size()) return;
    const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
    if (!worldHadCivilisation(g)) return;
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    const int people = std::min(aw.which, P->peoples - 1);
    const PeoplePlan& pp = P->plans[people];
    if (pp.places.empty()) return;
    const PlanPlace& cap = pp.places[0];
    std::string name = sys.bodies[aw.body].name; for (char& ch : name) if (ch == ' ') ch = '_';
    if (people) name += "_2";
    setFramebufferScale(scale);
    SpaceRenderer sr; StarNeighborhood nb; SurfaceView sv;
    sv.init(&sys, aw.body, cap.lat, cap.lon, 1000.0);
    double err = 0; const double t = findTime(sv.site, 32 * DEG, 1000.0, &err);
    sv.init(&sys, aw.body, cap.lat, cap.lon, t);
    sv.flocks.clear(); sv.critters.clear();
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    int gLat, gLon; sv.ruinCellAt(0, 0, gLat, gLon);
    const SurfaceView::RuinCell* cell = nullptr;
    for (int dl = -1; dl <= 1 && !cell; dl++) for (int dn = -1; dn <= 1 && !cell; dn++) { const SurfaceView::RuinCell* c = sv.ruinCell(gLat + dl, gLon + dn); if (c->has && c->spec.rank == 0 && c->spec.people == people) cell = c; }
    if (!cell) { printf("  %s: the capital's cell not found\n", name.c_str()); return; }
    const RuinSpec& sp = cell->spec;
    const double R = sp.size, cx = cell->r.x, cz = cell->r.z;
    const SunInfo sun = sv.site.sun(t);
    Framebuffer fb; Input in;
    auto shoot = [&](const std::string& what) {
        for (int i = 0; i < 6; i++) { sv.update(1.0 / 30, in, t, false); sv.render(fb, t, nb.stars, sr, 1.0); }
        fb.mush(2);
        const std::string fn = fmt("shots/tests/towns_view_%s_%s.png", name.c_str(), what.c_str());
        saveFB(fb, fn.c_str());
        printf("  %s: %s (the forms %.1f ms, %d items, %d polygons; %zu marks in the ground)\n", what.c_str(), fn.c_str(), sv.lastRuinMs, sv.lastRuinElems, sv.lastFormPolys, sv.site.marks.size());
        if (getenv("TOWNS_VERBOSE")) {   // the drawn vertices on the nearest street: their material, albedo and the street's share
            const Street* ns = nullptr; double nd = 1e18;
            for (const Street& s2 : sp.streets) { const double d = streetPointDist(s2, sv.player.x - cx, sv.player.z - cz); if (d < nd) { nd = d; ns = &s2; } }
            if (ns) {
                const double mx = cx + (ns->ax + ns->bx) * 0.5, mz = cz + (ns->az + ns->bz) * 0.5;
                for (int k = -3; k <= 3; k++) {
                    const double L = std::hypot(ns->bx - ns->ax, ns->bz - ns->az), qx = mx - (ns->bz - ns->az) / L * k * 2.0, qz = mz + (ns->bx - ns->ax) / L * k * 2.0;
                    const int ix = (int)std::lround(qx / 4), iz = (int)std::lround(qz / 4);
                    const TerrainVertex& v = sv.site.lodN.at(ix, iz);
                    printf("    across the %s (half %.1f, paved %d) %+d m: lodN mat %d albedo %.3f road %d veg %.2f; markCover %.2f\n", STREET_RANK_NAMES[ns->rank], ns->half, (int)ns->paved, k * 2, v.material, v.albedo, v.road, v.veg, sv.site.markCover(qx, qz));
                }
            }
        }
    };
    // from the air: over the edge, the sun to the side
    const double a = sun.azimuth + 1.9, D = std::max(R * 1.05, 120.0), H = clampd(0.42 * R, 70, 480);
    sv.player.x = cx + std::sin(a) * D; sv.player.z = cz + std::cos(a) * D;
    formsCamera(sv, H, cx, sv.site.surfaceHeight(cx, cz), cz, t);
    shoot("air");
    // closer over the old core (or the middle): a third of the way out, lower
    const double coreR = sp.layers.size() > 1 ? sp.layers[0].r1 : R * 0.45;
    sv.player.x = cx + std::sin(a) * coreR * 1.1; sv.player.z = cz + std::cos(a) * coreR * 1.1;
    formsCamera(sv, clampd(0.35 * coreR, 40, 160), cx, sv.site.surfaceHeight(cx, cz), cz, t);
    shoot("core");
    // on a street near the middle, at the eye's height, along it
    // the street: of those of a street's rank at a fifth to a half of the old core's radius, the one with the most buildings along it
    const Street* best = nullptr; double bestN = -1;
    for (const Street& s2 : sp.streets) {
        if (s2.rank < SR_STREET) continue;
        const double L = std::hypot(s2.bx - s2.ax, s2.bz - s2.az), mr = std::hypot((s2.ax + s2.bx) * 0.5, (s2.az + s2.bz) * 0.5);
        if (L < 8 || mr < std::max(30.0, coreR * 0.2) || mr > std::max(80.0, coreR * 0.6)) continue;
        int nb = 0; for (const Building& b : sp.buildings) if (streetPointDist(s2, b.x, b.z) < s2.half + 25) nb++;
        if (nb > bestN) { bestN = nb; best = &s2; }
    }
    if (best) {
        sv.player.x = cx + best->ax; sv.player.z = cz + best->az;
        sv.enterFreeCam();
        sv.freePos = Vec3(sv.player.x, sv.site.surfaceHeight(sv.player.x, sv.player.z) + 1.7, sv.player.z);
        sv.freeYaw = std::atan2(best->bx - best->ax, best->bz - best->az); sv.freePitch = 0.02;
        sv.update(1.0 / 30, in, t, false);
        shoot("street");
    }
}

// T-02: C-08's reading of the site by age, on the ages' scene worlds: of a world's towns and cities, the first with a harbour (a quay, a dock and
// its cranes, a container port), the first with its reservoir or cistern, the first with irrigation circles or strips in its fields, each seen
// from the air with the light from the side (`shots/tests/towns_feature_<age>_<what>.png`)
static void featureViews(int scale) {
    for (int age = AGE_MEDIEVAL; age <= AGE_MODERN; age++) {
        const AgeWorld& aw = AGE_SCENE_WORLDS[age];
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) continue;
        if (aw.body >= (int)sys.bodies.size()) continue;
        const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        const PeoplePlan& pp = P->plans[std::min(aw.which, P->peoples - 1)];
        const char* const WHAT[3] = {"harbour", "water", "fields"};
        for (int w = 0; w < 3; w++) {
            const PlanPlace* found = nullptr; RuinSpec fsp;
            for (const PlanPlace& p : pp.places) {
                if (p.sclass != SC_TOWN && p.sclass != SC_CITY) continue;
                if (w == 0 && !p.coast) continue;
                RuinSpec sp; if (!ruinOfCell(g, p.gLat, p.gLon, sp, true) || sp.buildAge >= 0) continue;
                bool ok = false;
                if (w == 0) ok = sp.harbour;
                else if (w == 1) ok = sp.reservoir || sp.cistern;
                else { for (const FieldPatch& f : sp.fields) if (f.kind == FI_CIRCLE || f.kind == FI_STRIP || f.kind == FI_ENCLOSED || f.kind == FI_WALLED) ok = true; }
                if (ok) { found = &p; fsp = sp; break; }
            }
            if (!found) { printf("  %s: no town with its %s\n", AGE_NAMES[age], WHAT[w]); continue; }
            setFramebufferScale(scale);
            SpaceRenderer sr; StarNeighborhood nb; SurfaceView sv;
            sv.init(&sys, aw.body, found->lat, found->lon, 1000.0);
            double err = 0; const double t = findTime(sv.site, 30 * DEG, 1000.0, &err);
            sv.init(&sys, aw.body, found->lat, found->lon, t);
            sv.flocks.clear(); sv.critters.clear();
            nb.update(sv.site.worldPos(t, 0, 0, 0));
            double cx = 0, cz = 0; sv.site.localAt(StarSystem::bodyFromLatLon(fsp.lat, fsp.lon), cx, cz);
            const SunInfo sun = sv.site.sun(t);
            double tx = cx, tz = cz, eyeD = 0, eyeH = 0, eyeA = sun.azimuth + 1.8;
            if (w == 0) {   // the harbour's works: from the land, over the town, toward the water
                double hx = 0, hz = 0; int n = 0;
                for (const Building& b : fsp.buildings) if (b.kind == BK_QUAY || b.kind == BK_CRANE || b.kind == BK_CONTAINERS || b.kind == BK_MOLE) { hx += b.x; hz += b.z; n++; }
                if (n) { tx = cx + hx / n; tz = cz + hz / n; }
                eyeA = fsp.site.shoreDir + PI + 0.5; eyeD = 140; eyeH = 55;
            } else if (w == 1) {
                for (const Building& b : fsp.buildings) if (b.kind == BK_RESERVOIR || b.kind == BK_CISTERN) { tx = cx + b.x; tz = cz + b.z; break; }
                eyeD = 70; eyeH = 35;
            } else {   // a field of its own kind (a circle the modern age's), from over the town's edge
                const FieldPatch* fp = nullptr;
                for (const FieldPatch& f : fsp.fields) if (!fp || (f.kind == FI_CIRCLE && fp->kind != FI_CIRCLE)) fp = &f;
                if (fp) { tx = cx + fp->x; tz = cz + fp->z; }
                eyeA = std::atan2(cx - tx, cz - tz) + 0.3; eyeD = fp ? std::max(260.0, std::max(fp->hw, fp->hd) * 2.2) : 600; eyeH = eyeD * 0.6;
            }
            sv.player.x = tx + std::sin(eyeA) * eyeD; sv.player.z = tz + std::cos(eyeA) * eyeD;
            formsCamera(sv, eyeH, tx, sv.site.surfaceHeight(tx, tz), tz, t);
            Framebuffer fb; Input in;
            for (int i = 0; i < 6; i++) { sv.update(1.0 / 30, in, t, false); sv.render(fb, t, nb.stars, sr, 1.0); }
            fb.mush(2);
            const std::string fn = fmt("shots/tests/towns_feature_%s_%s.png", AGE_NAMES[age], WHAT[w]);
            saveFB(fb, fn.c_str());
            int fk[FI_COUNT] = {}; for (const FieldPatch& f : fsp.fields) fk[clampi(f.kind, 0, FI_COUNT - 1)]++;
            printf("  %s %s: a %s of %zu buildings on %s (%s; fields %d strips, %d walled, %d enclosed, %d circles, %d terraced) -> %s\n", AGE_NAMES[age], WHAT[w], SETTLEMENT_CLASS_NAMES[fsp.sclass], fsp.buildings.size(), sys.bodies[aw.body].name.c_str(), featureList(fsp).c_str(), fk[FI_STRIP], fk[FI_WALLED], fk[FI_ENCLOSED], fk[FI_CIRCLE], fk[FI_TERRACE], fn.c_str());
        }
    }
}

// T-02: `towns <sx> <sz> <body> [second]`, `towns ages`: a people's towns: its capital, its first city, its oldest town, a place it left early,
// a town and a village of its countryside, each printed and drawn from above (`shots/tests/towns_<world>_<what>.png`)
int runTowns(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    const int which = takePeopleArg(argc, argv);
    std::vector<AgeWorld> worlds;
    if (argc > 2 && std::string(argv[2]) == "check") {   // the unit's T-02 checks alone
        int failures = 0;
        townChecks(modeCheck(failures));
        printf("towns check: %d failure%s\n", failures, failures == 1 ? "" : "s");
        return failures ? 1 : 0;
    }
    if (argc > 2 && std::string(argv[2]) == "view") {   // `towns view [<sx> <sz> <body> [second]] [scale=N]`: the capitals through the renderer
        std::vector<AgeWorld> vw;
        if (argc > 5) vw.push_back(AgeWorld{atoll(argv[3]), atoll(argv[4]), atoi(argv[5]), which});
        else for (int a = AGE_MEDIEVAL; a <= AGE_MODERN; a++) vw.push_back(AGE_SCENE_WORLDS[a]);
        for (const AgeWorld& aw : vw) { printf("%lld %lld %d:\n", (long long)aw.sx, (long long)aw.sz, aw.body); townViews(aw, std::max(1, g_testScale)); }
        if (argc <= 5) { printf("the site's reading by age:\n"); featureViews(std::max(1, g_testScale)); }
        return 0;
    }
    const bool ages = argc > 2 && std::string(argv[2]) == "ages";
    if (ages) for (int a = 0; a < AGE_COUNT; a++) worlds.push_back(AGE_WORLDS[a]);
    else if (argc > 4) worlds.push_back(AgeWorld{atoll(argv[2]), atoll(argv[3]), atoi(argv[4]), which});
    else { printf("towns <sx> <sz> <body> [second] | towns ages | towns view [<sx> <sz> <body> [second]]\n"); return 1; }
    for (const AgeWorld& aw : worlds) {
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)aw.sx, (long long)aw.sz); continue; }
        if (aw.body >= (int)sys.bodies.size()) { printf("no body %d\n", aw.body); continue; }
        const Body& b = sys.bodies[aw.body];
        const BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) { printf("%s had no people\n", b.name.c_str()); continue; }
        const int people = std::min(aw.which, peoplesOf(g) - 1);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        const PeoplePlan& pp = P->plans[people];
        double t0 = nowSec();
        std::shared_ptr<const History> H = historyOfWorld(g, people);
        const double hMs = (nowSec() - t0) * 1000;
        const Culture cu = cultureOf(g, people);
        std::string name = b.name; for (char& ch : name) if (ch == ' ') ch = '_';
        if (people) name += "_2";
        printf("%s (sector %lld 0 %lld body %d), the %s people: %zu places; the history %s (%.1f ms with the system found again)\n", b.name.c_str(), (long long)aw.sx, (long long)aw.sz, aw.body, AGE_NAMES[cu.age], pp.places.size(), H ? "read" : "NOT FOUND", hMs);
        std::vector<std::pair<std::string, int>> picks;   // what, the plan's rank
        if (!pp.places.empty()) picks.push_back({"capital", 0});
        for (const PlanPlace& p : pp.places) if (p.sclass == SC_CITY) { picks.push_back({"city", p.rank}); break; }
        if (H) {
            int oldest = -1;
            for (int i = 1; i < (int)pp.places.size() && i < (int)H->places.size(); i++) if (pp.places[i].sclass == SC_TOWN && (oldest < 0 || H->places[i].age < H->places[oldest].age || (H->places[i].age == H->places[oldest].age && H->places[i].founded < H->places[oldest].founded))) oldest = i;
            if (oldest > 0) picks.push_back({"town", oldest});
            for (int i = 1; i < (int)pp.places.size() && i < (int)H->places.size(); i++) if (H->places[i].fate == HP_EARLY && (pp.places[i].sclass == SC_TOWN || pp.places[i].sclass == SC_CITY)) { picks.push_back({"left", i}); break; }
        }
        for (const auto& pk : picks) {
            const PlanPlace& p = pp.places[pk.second];
            RuinSpec sp; const double ts = nowSec();
            if (!ruinOfCell(g, p.gLat, p.gLon, sp, true)) { printf("  %s: no settlement in its cell\n", pk.first.c_str()); continue; }
            const double ms = (nowSec() - ts) * 1000;
            const std::vector<std::string> L = townLines(sp, ms);
            printf("  the %s (rank %d at %.2f %.2f):\n", pk.first.c_str(), pk.second, p.lat / DEG, p.lon / DEG);
            for (const std::string& l : L) printf("    %s\n", l.c_str());
            std::vector<std::string> title = {b.name + ": THE " + pk.first + " (" + AGE_NAMES[cu.age] + " PEOPLE)"};
            for (const std::string& l : L) title.push_back(l.substr(0, 150));
            for (auto& tl : title) for (char& ch : tl) ch = (char)std::toupper((unsigned char)ch);
            drawTownMap(sp, fmt("shots/tests/towns_%s_%s.png", name.c_str(), pk.first.c_str()), title);
        }
        // the countryside round the capital: the nearest town and village of the people's own (not the plan's)
        if (!pp.places.empty()) {
            const PlanPlace& cap = pp.places[0];
            bool gotTown = false, gotVillage = false;
            for (int ring = 1; ring < 40 && !(gotTown && gotVillage); ring++)
                for (int dl = -ring; dl <= ring && !(gotTown && gotVillage); dl++)
                    for (int dn = -ring; dn <= ring && !(gotTown && gotVillage); dn++) {
                        if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                        RuinSpec sp;
                        const int row = cap.gLat + dl, col = ruinCellLonIn(g, cap.gLat, cap.gLon, row) + dn;
                        if (!ruinOfCell(g, row, col, sp, false) || sp.rank >= 0 || sp.people != people) continue;
                        const bool town = sp.sclass == SC_TOWN, vil = sp.sclass == SC_VILLAGE;
                        if ((town && gotTown) || (vil && gotVillage) || (!town && !vil)) continue;
                        const double ts = nowSec();
                        ruinOfCell(g, row, col, sp, true);
                        const double ms = (nowSec() - ts) * 1000;
                        const std::string what = town ? "country_town" : "country_village";
                        (town ? gotTown : gotVillage) = true;
                        const std::vector<std::string> L = townLines(sp, ms);
                        printf("  a %s of the countryside (%d cells from the capital):\n", town ? "town" : "village", ring);
                        for (const std::string& l : L) printf("    %s\n", l.c_str());
                        std::vector<std::string> title = {b.name + ": A " + (town ? "TOWN" : "VILLAGE") + " OF THE COUNTRYSIDE"};
                        for (const std::string& l : L) title.push_back(l.substr(0, 150));
                        for (auto& tl : title) for (char& ch : tl) ch = (char)std::toupper((unsigned char)ch);
                        drawTownMap(sp, fmt("shots/tests/towns_%s_%s.png", name.c_str(), what.c_str()), title);
                    }
        }
        printf("\n");
    }
    return 0;
}

// C-09: `roads <sx> <sz> <body index> [latDeg lonDeg]`: a world's roads (`peoples/roads.h`): the culture's road style, then the network of the
// 13 x 13 ruin cells round a point: every sited settlement (the nodes) and every road between them (the ends' classes, the
// length along the way against the straight line, the bends, the steepest step, the share of the length left), the cost, and a
// sketch of the region (61 x 31: a hamlet `h`, a village `v`, a town `T`, a monument `m`, a road's points `.`)
int runRoads(int argc, char** argv) {
    if (argc < 5) { printf("roads <sx> <sz> <body index> [latDeg lonDeg]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    double lat = argc > 6 ? atof(argv[5]) * DEG : 0.2, lon = argc > 6 ? atof(argv[6]) * DEG : 0.7;
    Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) { printf("%s: no civilisation, no roads\n", b.name.c_str()); return 0; }
    int np = std::max(1, peoplesOf(g)); Culture cs[2] = {cultureOf(g, 0), np > 1 ? cultureOf(g, 1) : Culture()};   // C-13: a road is its people's build
    for (int k = 0; k < np; k++) printf("%s, %s, R %.0f km: %sthe roads %s, %.1f m wide (half %.2f), wear %.2f (the end %.0f years ago, wet %.2f)\n", b.name.c_str(), KINDS[b.type].name, b.radiusKm, np > 1 ? fmt("people %d: ", k + 1).c_str() : "",
           cs[k].paved ? "paved with stone" : "beaten tracks", 2 * cs[k].roadHalf, cs[k].roadHalf, cs[k].roadWear, cs[k].ageYears, cs[k].wet);
    int gLat0, gLon0; ruinCellOf(g, lat, lon, gLat0, gLon0);
    const int H = 6;
    double t0 = nowSec();
    RoadNodeCache nc; nc.reset(&g);
    std::vector<RoadNode> nodes;
    for (int dl = -H; dl <= H; dl++) for (int dn = -H; dn <= H; dn++) { RoadNode n; if (nc.node(gLat0 + dl, gLon0 + dn, n)) nodes.push_back(n); }
    double tNodes = nowSec() - t0;
    std::vector<RoadPair> pairs;
    for (int dl = -H; dl <= H; dl++) for (int dn = -H; dn <= H; dn++) nc.pairsOf(gLat0 + dl, gLon0 + dn, pairs);
    std::sort(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id < y.id; });
    pairs.erase(std::unique(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id == y.id; }), pairs.end());
    double tPairs = nowSec() - t0 - tNodes;
    std::vector<Road> roads; int failed = 0;
    for (const RoadPair& p : pairs) { Road r; if (roadWay(g, cs[roadPeopleOf(p.a, p.b)], p.a, p.b, r)) roads.push_back(r); else failed++; }
    double tWalk = nowSec() - t0 - tNodes - tPairs;
    int degree[64] = {0}; std::unordered_map<uint64_t, int> deg;
    for (const Road& r : roads) { deg[roadCellKey(g, r.a.gLat, r.a.gLon)]++; deg[roadCellKey(g, r.b.gLat, r.b.gLon)]++; }
    int lonely = 0; for (const RoadNode& n : nodes) { int d = deg.count(roadCellKey(g, n.gLat, n.gLon)) ? deg[roadCellKey(g, n.gLat, n.gLon)] : 0; degree[std::min(d, 63)]++; if (!d) lonely++; }
    double lenSum = 0, straightSum = 0, leftSum = 0, gradeMax = 0; int bendsSum = 0, samples = 0;
    for (const Road& r : roads) {
        lenSum += r.lengthM; straightSum += r.straightM; bendsSum += r.bends; gradeMax = std::max(gradeMax, r.maxGrade);
        for (double a = 0; a < r.lengthM; a += 10) { leftSum += roadLeft(r.id, a, cs[r.people].roadWear); samples++; }
    }
    printf("13 x 13 cells round %.2f %.2f: %zu sited settlements, %zu joined pairs, %zu roads walked (%d found no dry way); %d settlements without a road; degrees 1..5: %d %d %d %d %d\n",
           lat / DEG, lon / DEG, nodes.size(), pairs.size(), roads.size(), failed, lonely, degree[1], degree[2], degree[3], degree[4], degree[5]);
    if (!roads.empty()) printf("the roads: %.2f km together, %.2f along the way per km straight, %.1f bends each, the steepest step %.3f; %.0f%% of the length left after the wear\n",
                               lenSum / 1000, lenSum / std::max(1.0, straightSum), (double)bendsSum / roads.size(), gradeMax, 100.0 * leftSum / std::max(1, samples));
    printf("the cost: the nodes %.1f ms, the pairs %.1f ms, the walks %.1f ms (%.2f ms a road)\n", tNodes * 1000, tPairs * 1000, tWalk * 1000, roads.empty() ? 0.0 : tWalk * 1000 / roads.size());
    int printed = 0;
    for (const Road& r : roads) {
        if (printed++ >= 16) break;
        printf("  %s%-8s %3d %4d -> %-8s %3d %4d: %5.0f m along, %5.0f straight, %2d bends, steepest %.3f, %.1f m wide, %zu points%s\n", np > 1 ? fmt("p%d ", r.a.people + 1).c_str() : "", SETTLEMENT_CLASS_NAMES[r.a.sclass], r.a.gLat, r.a.gLon,
               SETTLEMENT_CLASS_NAMES[r.b.sclass], r.b.gLat, r.b.gLon, r.lengthM, r.straightM, r.bends, r.maxGrade, 2 * r.halfWidth, r.pts.size(), np > 1 ? fmt(" (to people %d, built as people %d)", r.b.people + 1, r.people + 1).c_str() : "");
    }
    // the sketch: the region's cells spread over 61 x 31 characters
    const int W = 61, Hh = 31;
    std::vector<std::string> sk(Hh, std::string(W, ' '));
    double dLat = ruinCellLat(g), dLon = dLat / std::max(std::cos((gLat0 + 0.5) * dLat), 0.05);
    double lat0 = (gLat0 - H) * dLat, lat1 = (gLat0 + H + 1) * dLat, lon0 = (gLon0 - H) * dLon, lon1 = (gLon0 + H + 1) * dLon;
    auto put = [&](double la, double lo, char ch) { int x = (int)((std::remainder(lo - lon0, TAU) / (lon1 - lon0)) * W), y = (int)((1 - (la - lat0) / (lat1 - lat0)) * Hh); if (x >= 0 && x < W && y >= 0 && y < Hh && (sk[y][x] == ' ' || sk[y][x] == '.')) sk[y][x] = ch; };
    for (const Road& r : roads) for (const Vec3& p : r.pts) { double la, lo; StarSystem::latLonFromBody(p, la, lo); put(la, lo, '.'); }
    for (const RoadNode& n : nodes) put(n.lat, n.lon, n.sclass == SC_HAMLET ? 'h' : (n.sclass == SC_VILLAGE ? 'v' : (n.sclass == SC_TOWN ? 'T' : 'm')));
    printf("the region, north up (%.0f x %.0f km):\n", (lon1 - lon0) * std::cos((gLat0 + 0.5) * dLat) * g.R, (lat1 - lat0) * g.R);
    for (const std::string& row : sk) printf("  |%s|\n", row.c_str());
    return 0;
}
