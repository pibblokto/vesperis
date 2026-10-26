// E-03: the unit's drifters (group `drifters`)
#include "harness/harness.h"
#include "harness/test_worlds.h"

// a history's events, persons and places as a hash (the decoder's and the ground's compared)
static uint64_t historyHash(const History& H) {
    uint64_t h = 0xD05A15ULL;
    auto mixIn = [&](int64_t v) { h = mix64(h ^ (uint64_t)v); };
    auto mixD = [&](double v) { int64_t b; std::memcpy(&b, &v, sizeof b); mixIn(b); };
    for (const HistoryEvent& e : H.events) { mixIn(e.kind); mixIn(e.year); mixIn(e.season); mixIn(e.pub); mixD(e.salience); mixIn(e.place); mixIn(e.place2); mixIn(e.a); mixIn(e.b); mixIn(e.c); mixIn(e.cause); mixIn(e.thing); mixIn(e.out); mixIn(e.other); mixIn(e.value); }
    for (const HistoryPerson& p : H.persons) { mixIn((int64_t)p.name); mixIn(p.born); mixIn(p.died); mixIn(p.fate); mixIn(p.family); mixIn(p.role); mixIn(p.trade); mixIn(p.traits); }
    for (const HistoryPlace& p : H.places) { mixIn(p.plan); mixIn(p.sclass); mixIn((int64_t)p.name); mixIn(p.founded); mixIn(p.ended); mixIn(p.fate); mixD(p.lat); mixD(p.lon); mixD(p.peak); }
    return h;
}

// D-01 to D-05: the drifters, heard, their voices, their dark, their peoples
void unitDrifters(const Check& check) {
    // the drifters. Their field round home and in a globular cluster, each one's sector without a star and its key naming it; the field
    // the same however it is asked (a hash of the nearest pinned); a save at one and a target of another kept through a load; no light, no
    // season and no sun but at the nadir anywhere on their worlds; nothing undefined from the anchor the drifter sits on
    {
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
        std::vector<Star> near; driftersNear(home.pos, 1500, near);
        std::vector<Star> all = near;
        int globulars = 0;
        for (int64_t gx = -12; gx <= 12 && globulars < 4; gx++)   // the globulars of the inner halo, until four that hold drifters
            for (int64_t gz = -6; gz <= 6 && globulars < 4; gz++) {
                int64_t xs[DRIFT_GLOBULAR_MAX], ys[DRIFT_GLOBULAR_MAX], zs[DRIFT_GLOBULAR_MAX];
                const int n = drifterGlobular(gx, 0, gz, xs, ys, zs);
                for (int k = 0; k < n; k++) { Star d; if (drifterInSector(xs[k], ys[k], zs[k], d, false)) all.push_back(d); }
                if (n > 0) globulars++;
            }
        {
            int withStar = 0, misnamed = 0, empties = 0, emptyWrong = 0;
            for (const Star& d : all) {
                Star s, p;
                if (starInSector(d.sx, d.sy, d.sz, s, false)) withStar++;
                if (!placeInSector(d.sx, d.sy, d.sz, p, true) || p.cls != STAR_ROGUE || p.seed != d.seed || length(p.pos - d.pos) > 1e-3) misnamed++;
            }
            Rng r(0xD01E3);   // sectors near home with neither: no place
            for (int k = 0; k < 4000; k++) {
                const int64_t x = HOME_SX + r.irange(400) - 200, y = HOME_SY + r.irange(200) - 100, z = HOME_SZ + r.irange(400) - 200;
                Star s, p;
                if (starInSector(x, y, z, s, false) || drifterInSector(x, y, z, s, false)) continue;
                empties++;
                if (placeInSector(x, y, z, p, false)) emptyWrong++;
            }
            check("drifters: none in a sector with a star, each its key's place, an empty sector none (D-01)", !all.empty() && globulars > 0 && withStar == 0 && misnamed == 0 && emptyWrong == 0,
                  fmt("%zu within 1,500 ly of home and %zu in %d globulars: %d with a star, %d not named by their key; %d empty sectors, %d with a place", near.size(), all.size() - near.size(), globulars, withStar, misnamed, empties, emptyWrong));
        }
        {
            std::vector<Star> again; driftersNear(home.pos, 1500, again);
            std::vector<Star> inner; driftersNear(home.pos, 700, inner);
            bool same = again.size() == near.size();
            for (size_t i = 0; same && i < near.size(); i++) same = again[i].sx == near[i].sx && again[i].sy == near[i].sy && again[i].sz == near[i].sz && again[i].seed == near[i].seed;
            int innerOut = 0;   // the nearer list is the farther one's head
            for (size_t i = 0; i < inner.size(); i++) if (i >= near.size() || inner[i].seed != near[i].seed) innerOut++;
            uint64_t h = 0xD01;
            for (size_t i = 0; i < near.size() && i < 40; i++) { const DrifterBody b = drifterBody(near[i]); h = hashCombine(h, near[i].seed ^ (uint64_t)b.drawnType * 0x9E3779B97F4A7C15ULL); }
            const uint64_t PINNED = 0xb00ac446b619bf6fULL;   // the field of the first forty, as D-01 made it (D-04: the kinds as drawn; a dark ocean is a rocky drifter made)
            check("drifters: the field the same asked twice and nearer, its nearest forty as D-01 made them (D-01)", same && innerOut == 0 && h == PINNED,
                  fmt("%zu within 1,500 ly, %zu within 700 (%d out of order); the hash %016llx (pinned %016llx)", near.size(), inner.size(), innerOut, (unsigned long long)h, (unsigned long long)PINNED));
        }
        // the first dozen near home with a landable world among them
        std::vector<StarSystem> systems;
        for (size_t i = 0; i < near.size() && systems.size() < 12; i++) { Star d; drifterInSector(near[i].sx, near[i].sy, near[i].sz, d, true); systems.emplace_back(); systems.back().generate(d); }
        {
            int bodies = 0, lit = 0, seasons = 0, notNadir = 0, dusty = 0, looked = 0;
            for (const StarSystem& sys : systems)
                for (int i = 0; i < (int)sys.bodies.size(); i++) {
                    bodies++;
                    for (int k = 0; k < 3; k++) {
                        const double t = 86400.0 * 37 * k + 1234.5;
                        if (sys.lightAt(sys.bodyPos(i, t), t).factor != 0) lit++;
                        if (sys.seasonOf(i, t) != 0) seasons++;
                    }
                    if (zodiacalDust(sys, i) != 0) dusty++;
                    if (!KINDS[sys.bodies[i].type].landable) continue;
                    for (int k = 0; k < 4; k++) {
                        SurfaceSite site; site.initFrame(&sys, i, (k * 40 - 60) * DEG, (k * 97 - 150) * DEG, 5000.0 * k);
                        const SunInfo si = site.sun(5000.0 * k + 333);
                        looked++;
                        if (si.lightFactor != 0) lit++;
                        if (si.altitude != -PI / 2 || si.dirLocal.y != -1) notNadir++;
                        if (site.season != 0) seasons++;
                    }
                }
            check("drifters: no light, no season and the sun at the nadir on their worlds (D-01)", bodies > 0 && looked > 0 && lit == 0 && seasons == 0 && notNadir == 0 && dusty == 0,
                  fmt("%d bodies of %zu drifters, %d places on their ground: %d lit, %d with a season, %d with a sun off the nadir, %d with a zodiacal dust", bodies, systems.size(), looked, lit, seasons, notNadir, dusty));
        }
        {
            int bad = 0, sunEvents = 0, events = 0, places = 0;
            auto finite = [](const Vec3& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); };
            for (const StarSystem& sys : systems) {
                for (int i = 0; i < (int)sys.bodies.size(); i++)
                    for (int k = 0; k < 3; k++) {
                        const double t = 86400.0 * 11 * k + 17.0;
                        const Mat3 f = sys.bodyFrame(i, t);
                        if (!finite(sys.bodyPos(i, t)) || !finite(sys.bodyVel(i, t)) || !std::isfinite(sys.meanAnomaly(i, t)) || !finite(f * Vec3(1, 2, 3))) bad++;
                    }
                for (int i = 0; i < (int)sys.bodies.size(); i++) {
                    if (!KINDS[sys.bodies[i].type].landable) continue;
                    AlmanacPlace p; p.sys = &sys; p.t = 1000; p.body = i; p.lat = 0.3; p.lon = 1.1; p.atmosphere = KINDS[sys.bodies[i].type].atmosphere;
                    if (sys.bodies[i].rings) p.ringProf = sys.ringProfileOf(i);
                    std::vector<AlmanacEvent> ev; almanacOf(p, ev); places++;
                    double ar = 1; if (almanacSunAltitude(p, 1500, &ar) != -PI / 2 || ar != 0) bad++;
                    for (const AlmanacEvent& e : ev) {
                        events++;
                        if (!std::isfinite(e.t) || !std::isfinite(e.value)) bad++;
                        if (e.kind <= AL_DAWN || e.kind == AL_FULL || e.kind == AL_NEW || e.kind == AL_ECLIPSE || e.kind == AL_RING_SHADOW || e.kind == AL_RING_CLEAR || e.kind == AL_SHADOW || e.kind == AL_ECLIPSED || e.kind == AL_SUN_TRANSIT) sunEvents++;
                    }
                }
                AlmanacPlace ship; ship.sys = &sys; ship.t = 1000; ship.window = 30 * 86400.0; ship.ground = false; ship.body = 0; ship.parkDir = normalize(Vec3(0.3, 0.2, 1)); ship.parkDist = sys.bodies[0].radiusKm * 3.5; ship.orbiting = true;
                std::vector<AlmanacEvent> ev; almanacOf(ship, ev); places++;
                for (const AlmanacEvent& e : ev) { events++; if (!std::isfinite(e.t)) bad++; if (e.kind == AL_SHADOW || e.kind == AL_ECLIPSED || e.kind == AL_SUN_TRANSIT) sunEvents++; }
            }
            // the arrival and the parking as the game makes them, at the nearest
            TestGame game; game.savePrefix = "shots/tests/unit_rogue_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/unit_rogue_guide.txt";
            remove(game.guidePath.c_str());
            game.newGame(); game.setState(GameState::SPACE);
            Star d; drifterInSector(near[0].sx, near[0].sy, near[0].sz, d, true);
            game.testArriveAt(d);
            const double r0 = game.testSystem().bodies[0].radiusKm, arrived = length(game.testShipPos() - d.pos) / r0;
            const bool arrivalOk = finite(game.testShipPos()) && arrived >= 40 && arrived <= 80 && game.testSystem().sunless();
            game.testParkAt(d, 0, false);
            const bool parkOk = finite(game.testShipPos()) && game.testParkedBody() == 0 && length(game.testShipPos() - d.pos) > r0;
            check("drifters: nothing undefined from the anchor: positions, the almanac, the arrival, the parking (D-01)", bad == 0 && sunEvents == 0 && places > 0 && arrivalOk && parkOk,
                  fmt("%zu systems, %d almanac places, %d events (%d of the sun's), %d undefined; the arrival %.1f radii off, the parking %s", systems.size(), places, events, sunEvents, bad, arrived, parkOk ? "beside it" : "WRONG"));
            // a save at it, and the next one as the remote target, kept through a load
            Star d2; drifterInSector(near[1].sx, near[1].sy, near[1].sz, d2, true);
            game.testSetRemote(d2);
            const Vec3 at = game.testShipPos();
            const bool saved = game.saveSlot(1);
            TestGame g2; g2.savePrefix = game.savePrefix; g2.settingsPath = game.settingsPath; g2.keysPath = game.keysPath; g2.guidePath = "shots/tests/unit_rogue_guide2.txt";
            remove(g2.guidePath.c_str());
            g2.newGame();
            const bool loaded = g2.loadSlot(1);
            Star rem; const bool hasRem = g2.testRemoteStar(rem);
            const bool trip = saved && loaded && g2.testSystem().sunless() && g2.testSystem().star.seed == d.seed && length(g2.testShipPos() - at) < 1.0 && g2.testParkedBody() == 0 &&
                              hasRem && rem.cls == STAR_ROGUE && rem.seed == d2.seed && g2.testSystem().bodies.size() == game.testSystem().bodies.size();
            check("drifters: a save at one and the next as the target come back as they were (D-01)", trip,
                  fmt("saved %d, loaded %d: the place %s, the ship %.3f km from where it was, parked at %d, the target %s", saved, loaded, g2.testSystem().sunless() ? "a drifter's" : "NOT A DRIFTER'S",
                      length(g2.testShipPos() - at), g2.testParkedBody(), hasRem && rem.cls == STAR_ROGUE && rem.seed == d2.seed ? "the drifter's" : "LOST"));
        }
    }
    // heard, not seen. The drifters by their own law from home, the three C-series worlds and the six age worlds; the radar in the game
    // hearing one, locking it, the chart's plot landing at it, its place named; the black disc and the survey's maps by their numbers
    {
        struct At { int64_t x, y, z; };
        std::vector<At> places = {{HOME_SX, HOME_SY, HOME_SZ}, {151, 0, 25}, {153, 0, 70}, {152, 0, 92}};
        for (int a = 0; a < AGE_COUNT; a++) places.push_back({AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz});
        int atX1 = 0, wrong = 0, homeFirst = -1, fewest32 = 1 << 30; int homeSteps[SIGNAL_GAIN_STEPS] = {0}; std::string lines;
        std::vector<Signal> home32;
        Vec3 homeAt;
        for (size_t pi = 0; pi < places.size(); pi++) {
            const At& P = places[pi];
            Star st; Vec3 at((P.x + 0.5) * SECTOR_KM, (P.y + 0.5) * SECTOR_KM, (P.z + 0.5) * SECTOR_KM);
            if (starInSector(P.x, P.y, P.z, st, false)) at = st.pos;
            std::vector<Signal> x1; signalsNear(at, x1);
            for (const Signal& x : x1) if (x.kind == SIG_ROGUE) atX1++;
            std::vector<Signal> L; signalsNear(at, L, SIGNAL_REACH_LY, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]);
            int steps[SIGNAL_GAIN_STEPS] = {0}, first = -1;
            for (const Signal& x : L) {
                if (x.kind != SIG_ROGUE) continue;
                Star d, s2;
                const bool place = drifterInSector(x.star.sx, x.star.sy, x.star.sz, d, false) && d.seed == x.star.seed && !starInSector(x.star.sx, x.star.sy, x.star.sz, s2, false);
                const double pw = drifterPower(d), need = rogueNeedGain(x.distLy, pw);
                int k = 0; while (k < SIGNAL_GAIN_STEPS && SIGNAL_GAINS[k] < x.needGain) k++;
                const bool law = place && k > 0 && k < SIGNAL_GAIN_STEPS && std::fabs(pw - x.power) < 1e-12 && std::fabs(need - x.needGain) < 1e-9 && x.needGain > 1 &&
                                 x.distLy <= rogueReachAtGain(pw, SIGNAL_GAINS[k]) + 1e-6 && x.distLy > rogueReachAtGain(pw, SIGNAL_GAINS[k - 1]) && x.body == 0 && x.bodyType == drifterBody(d).type;
                if (!law) { wrong++; continue; }
                for (int j = k; j < SIGNAL_GAIN_STEPS; j++) steps[j]++;
                if (first < 0 || k < first) first = k;
                if (pi == 0) home32.push_back(x);
            }
            fewest32 = std::min(fewest32, steps[SIGNAL_GAIN_STEPS - 1]);
            if (pi == 0) { homeFirst = first; for (int k = 0; k < SIGNAL_GAIN_STEPS; k++) homeSteps[k] = steps[k]; homeAt = at; }
            lines += fmt("%s%d", pi ? "," : "", steps[SIGNAL_GAIN_STEPS - 1]);
        }
        SignalScan sc; sc.begin(homeAt, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]); int slices = 1; while (!sc.step(97)) slices++;
        int sliced = 0, slicedOut = 0;
        for (const Signal& x : sc.out) if (x.kind == SIG_ROGUE) { sliced++; bool in = false; for (const Signal& h : home32) if (h.star.seed == x.star.seed && h.needGain == x.needGain) in = true; if (!in) slicedOut++; }
        std::vector<Signal> again; signalsNear(homeAt, again, SIGNAL_REACH_LY, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]);
        int again32 = 0; for (const Signal& x : again) if (x.kind == SIG_ROGUE) again32++;
        std::string steps; for (int k = 0; k < SIGNAL_GAIN_STEPS; k++) steps += fmt("%s x%d %d", k ? "," : "", SIGNAL_GAINS[k], homeSteps[k]);
        check("drifters: heard by their own law, never at x1, the slices the whole scan's (D-02)",
              atX1 == 0 && wrong == 0 && homeSteps[1] == 0 && (homeFirst == 2 || homeFirst == 3) && homeSteps[SIGNAL_GAIN_STEPS - 1] >= 6 && homeSteps[SIGNAL_GAIN_STEPS - 1] <= 10 && fewest32 >= 3 &&
              sliced == (int)home32.size() && slicedOut == 0 && again32 == (int)home32.size(),
              fmt("from home%s; at x32 from the ten places %s; at x1 %d anywhere, %d off their law; the slices of 97 cells (%d) %d of %zu, %d not the whole scan's; asked twice %d",
                  steps.c_str(), lines.c_str(), atX1, wrong, slices, sliced, home32.size(), slicedOut, again32));
    }
    {   // the radar in the game. At home at x1 no drifter in the list; the dial up, the nearest heard at its own step, aimed and held,
        // locks: the status says there is no star, the log no class, the lock's key the sector's; off, the chart's first stop is it (A
        // SIGNAL at its place), Enter plots it (the remote the drifter), Enter flies, the ship comes out 40-80 radii off with it the local
        // target; the guide notes the visit and names the place
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_d02.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        auto rogues = [&]() { int n = 0; for (int i = 0; i < game.testRadarSignals(); i++) if (game.testRadarKind(i) == SIG_ROGUE && !game.testRadarLocal(i)) n++; return n; };
        run(0.2); press(KEY_B); run(0.3);
        const int x1 = rogues();
        for (int i = 0; i < SIGNAL_GAIN_STEPS - 1; i++) press(KEY_EQUAL);
        for (int i = 0; i < 60 && !game.testRadarDeepReady(); i++) run(1.0 / 30);
        run(0.1);
        const int x32 = rogues();
        int who = -1; double ly = 1e18;
        for (int i = 0; i < game.testRadarSignals(); i++) if (game.testRadarKind(i) == SIG_ROGUE && !game.testRadarLocal(i) && game.testRadarDist(i) < ly) { ly = game.testRadarDist(i); who = i; }
        int64_t wx = 0, wy = 0, wz = 0; if (who >= 0) game.testRadarSector(who, wx, wy, wz);
        const double need = who >= 0 ? game.testRadarNeedGain(who) : 1e9;
        int step = 0; while (step < SIGNAL_GAIN_STEPS - 1 && SIGNAL_GAINS[step] < need) step++;
        for (int i = SIGNAL_GAIN_STEPS - 1; i > step; i--) press(KEY_MINUS);
        run(0.2);
        for (int i = 0; i < game.testRadarSignals(); i++) { int64_t sx, sy, sz; if (game.testRadarKind(i) == SIG_ROGUE && game.testRadarSector(i, sx, sy, sz) && sx == wx && sy == wy && sz == wz) who = i; }
        if (who >= 0) game.testAimAtSignal(who);
        double lockAt = -1; for (int i = 0; i < 30 * 20 && lockAt < 0; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); if (game.testRadarLocked() >= 0) lockAt = (i + 1) / 30.0; }
        const std::string lockSt = game.testStatus(), logged = game.guide.log.empty() ? "" : game.guide.log.back().text;
        const std::string key = Guide::starKey(wx, wy, wz);
        const bool lockOk = lockAt > 0 && game.testRadarKind(game.testRadarLocked()) == SIG_ROGUE && lockSt.find("NO STAR") != std::string::npos &&
                            logged.find("NO STAR AT ITS SOURCE") != std::string::npos && logged.find("(S") == std::string::npos && game.guide.signals.count(key) && game.guide.signals[key] == SIG_ROGUE;
        press(KEY_B); run(0.1);
        press(KEY_M); if (game.testMapPage() != 1) press(KEY_TAB);
        run(0.05);
        press(KEY_N); run(0.05);
        const std::string stopSt = game.testStatus();
        Star d; drifterInSector(wx, wy, wz, d, true);
        const bool stopOk = stopSt.rfind("A SIGNAL - 1 OF", 0) == 0 && length(game.testChartCursorAt() - d.pos / SECTOR_KM) < 1e-6;
        press(KEY_ENTER); run(0.05);
        int64_t rx = 0, ry = 0, rz = 0; Star rem;
        const bool plotted = game.testChartPlotted() && game.testRemote(rx, ry, rz) && rx == wx && ry == wy && rz == wz && game.testRemoteStar(rem) && rem.cls == STAR_ROGUE;
        press(KEY_ENTER); run(0.1);
        const bool flying = game.testShipMode() == (int)ShipState::VIMANA;
        for (int i = 0; i < 30 * 90 && game.testShipMode() == (int)ShipState::VIMANA; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
        run(1.0);
        const double off = length(game.testShipPos() - d.pos) / std::max(1.0, game.testSystem().bodies.empty() ? 1.0 : game.testSystem().bodies[0].radiusKm);
        const bool arrived = game.testSystem().sunless() && game.testSystem().star.seed == d.seed && off >= 40 && off <= 80 && game.testLocalTarget() == 0;
        const bool visited = game.guide.visited.count(key) > 0;
        press(KEY_G); run(0.05); press(KEY_DOWN); run(0.05); press(KEY_ENTER); run(0.05);
        const bool entry = game.state == GameState::TEXT_ENTRY;
        game.testTypeText("THE LANTERN"); run(0.05);
        const bool named = game.guide.names.count(key) && game.guide.names[key] == "THE LANTERN" && game.testStatus() == "THE PLACE IS NOW CALLED THE LANTERN";
        check("radar: a drifter heard, locked, plotted on the chart and flown to, its place named (D-02)", x1 == 0 && x32 >= 6 && who >= 0 && lockOk && stopOk && plotted && flying && arrived && visited && entry && named,
              fmt("x1 %d drifters, x32 %d; the nearest %.0f ly (needs x%.1f) at x%d locked after %.1f s: '%s', logged '%s'; the chart's stop '%s' %s; plotted %d, flying %d; arrived %d (%.1f radii off, the local target %d), visited %d; named %d",
                  x1, x32, ly, need, SIGNAL_GAINS[step], lockAt, lockSt.c_str(), trunc(logged, 70).c_str(), stopSt.c_str(), stopOk ? "at it" : "ASTRAY", plotted, flying, arrived, off, game.testLocalTarget(), visited, named));
        remove(game.guidePath.c_str());
    }
    {   // the black disc and the survey, by their numbers. Parked at the nearest giant drifter: no sun drawn (the sun's bank empty), the
        // globe a dark disc (its pixels' shade under the band's plane on average, at most one in a thousand over 30: a lightning's flash in its
        // night, a star through its rings) with its aurorae; the band's light on its coreward side over its far side's; the landing map of the
        // nearest landable drifter lit all over in the survey (not black, no night side)
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, false);
        std::vector<Star> near; driftersNear(home.pos, 1500, near);
        int gi = -1, li = -1;
        for (size_t i = 0; i < near.size(); i++) { const DrifterBody b = drifterBody(near[i]); if (gi < 0 && b.type == PT_GASGIANT) gi = (int)i; if (li < 0 && KINDS[b.type].landable && b.radiusKm >= 1500) li = (int)i; }
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_d02.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.settings.cabin = false; game.applySettings(); game.setState(GameState::SPACE);
        Input in;
        auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
        int discPx = 0, brightIn = 0, sunPx = 0, aurPx = 0; double discMax = 0, discMean = 0; bool lightOk = false;
        if (gi >= 0) {
            Star g; drifterInSector(near[gi].sx, near[gi].sy, near[gi].sz, g, true);
            game.testParkAt(g, 0, false); game.setState(GameState::SPACE); game.testAimAtBody(0); run(0.4);
            for (const Pix p : game.fb.idx) {
                const int bk = bankOf(p);
                if (bk == 1 && intenOf(p) > 0) sunPx++;
                if (bk == 12 && intenOf(p) > 0) aurPx++;
                if (bk == 2) { discPx++; discMax = std::max(discMax, shadeOf(p)); discMean += shadeOf(p); if (shadeOf(p) > 30) brightIn++; }
            }
            discMean /= std::max(1, discPx);
            std::vector<float> band; buildGalaxyBand(g.pos / SECTOR_KM, band, BAND_MAP_W, BAND_MAP_H);
            double sh[9]; galaxyBandLight(band, sh);
            const Vec3 core = normalize(Vec3(GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ) - g.pos / SECTOR_KM);
            lightOk = galaxyBandIrradiance(sh, core) > 1.5 * galaxyBandIrradiance(sh, core * -1.0);
        }
        double mapMean = 0, mapLeft = 0, mapRight = 0;
        if (li >= 0) {
            Star l; drifterInSector(near[li].sx, near[li].sy, near[li].sz, l, true);
            game.testParkAt(l, 0); run(0.3);
            const uint32_t* px = game.output(); const int S = FB_SCALE; long nL = 0, nR = 0;
            for (int y = (12 + 2) * S; y < (12 + 126) * S; y++)
                for (int x = (32 + 2) * S; x < (32 + 254) * S; x++) {
                    const uint32_t c = px[(size_t)y * FBW + x]; const double L = 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255);
                    if (x < (32 + 128) * S) { mapLeft += L; nL++; } else { mapRight += L; nR++; }
                }
            mapMean = (mapLeft + mapRight) / std::max(1L, nL + nR); mapLeft /= std::max(1L, nL); mapRight /= std::max(1L, nR);
        }
        const bool discOk = gi >= 0 && sunPx == 0 && discPx > 2000 && discMean < 8 && brightIn * 1000 <= discPx && aurPx > 0 && lightOk;
        const bool mapOk = li >= 0 && mapMean > 40 && std::min(mapLeft, mapRight) > 0.6 * std::max(mapLeft, mapRight);
        check("drifters: a black disc where the stars stop, the survey's maps (D-02)", discOk && mapOk,
              fmt("the giant at %.0f ly: the sun's pixels %d, the disc's %d (shade %.1f on average, %.1f at most, %d over 30), the aurorae's %d, the band's light coreward over the far side %s; the landing map's light %.0f (the halves %.0f and %.0f)",
                  gi >= 0 ? length(near[gi].pos - home.pos) / SECTOR_KM : 0.0, sunPx, discPx, discMean, discMax, brightIn, aurPx, lightOk ? "yes" : "NO", mapMean, mapLeft, mapRight));
        remove(game.guidePath.c_str());
    }
    {   // a voice for every drifter, through the synth. Of the hundred drifters nearest home, the first of each kind of voice (a brown
        // dwarf, a giant, a young, a metal and a small world) over a breath, with its moon and without (a moon of four hours given one that has
        // none), through the receiver at full clarity; the brown dwarf worn at x32 and locked, the giant under a people's voice at x18, the
        // cabin's radio and the suit's: each finite, at most 20 samples clipped, no peak at 1, the rms within C-07's 0.02-0.13. A beacon in its
        // slot under the giant's voice changes it (D-05's) and keeps the bounds; the same voice twice the same. No two alike: the 24 nearest
        // home over a breath each, every pair's voice prints further apart than two magnetospheres' are (the median of 20 over a minute)
        const Vec3 home((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
        const std::vector<RogueRef> field = roguesNearest(home, 100);
        int first[RV_COUNT]; for (int c = 0; c < RV_COUNT; c++) first[c] = -1;
        for (size_t i = 0; i < field.size(); i++) { const int c = field[i].voice.cls; if (c >= 0 && c < RV_COUNT && first[c] < 0) first[c] = (int)i; }
        bool all = true; for (int c = 0; c < RV_COUNT; c++) if (first[c] < 0) all = false;
        struct Job { std::string name; RogueRef ref; RxOpts o; double secs; };
        std::vector<Job> jobs;
        for (int c = 0; c < RV_COUNT && all; c++) {
            const RogueRef& r = field[first[c]];
            RogueRef other = r; other.voice.moonS = r.voice.moonS > 0 ? 0 : 4 * 3600.0;
            const double secs = std::max(32.0, AudioSynth::rogueBreathS(r.sig.seed, r.voice) + 2);
            jobs.push_back({fmt("%s%s", ROGUE_VOICE_NAMES[c], r.voice.moonS > 0 ? " with its moon" : ""), r, RxOpts(), secs});
            jobs.push_back({fmt("%s%s", ROGUE_VOICE_NAMES[c], other.voice.moonS > 0 ? " given a moon" : " without its moon"), other, RxOpts(), secs});
        }
        if (all) {
            const RogueRef& bd = field[first[RV_BROWN_DWARF]];
            RxOpts far; far.gainT = 1; far.sky = 0.4; far.clarity = 0.6; far.wear = 0.6; jobs.push_back({"a brown dwarf worn at x32", bd, far, 30});
            RxOpts cab; cab.set = RADIO_CABIN; cab.level = RADIO_CABIN_LEVEL; cab.hum = 0.6; cab.clarity = 0.9; jobs.push_back({"the cabin's radio", bd, cab, 30});
            const RogueRef& sm = field[first[RV_SMALL]];
            RxOpts suit; suit.set = RADIO_SUIT; suit.level = RADIO_SUIT_LEVEL; suit.hum = 0.05; suit.clarity = 0.9; jobs.push_back({"the suit's radio", sm, suit, 30});
        }
        std::vector<RenderStats> rs(jobs.size());
        parallelFor((int)jobs.size(), 1, [&](int b, int e) { for (int i = b; i < e; i++) rs[i] = renderRogue(jobs[i].ref, jobs[i].secs, nullptr, jobs[i].o); }, 1);
        std::string detail; bool ok = all; double lo = 1, hi = 0;
        auto judge = [&](const std::string& what, const RenderStats& r) {
            lo = std::min(lo, r.rms); hi = std::max(hi, r.rms);
            if (r.nan || r.clip > 20 || r.peak >= 1.0 || r.rms < 0.02 || r.rms > 0.13) { ok = false; detail += fmt("%s rms %.3f peak %.2f %d clipped%s; ", what.c_str(), r.rms, r.peak, r.clip, r.nan ? " NON-FINITE" : ""); }
        };
        for (size_t i = 0; i < jobs.size(); i++) judge(jobs[i].name, rs[i]);
        bool beaconOk = false, same = false;
        if (all) {
            const RogueRef& g = field[first[RV_GIANT]];
            PeopleWorld w;
            if (firstPeopleWorld(w)) { RxOpts u; u.gainT = gainShareOf(18); u.sky = 0.4; u.age = AGE_MODERN; u.kind2 = SIG_ROGUE; u.seed2 = g.sig.seed; u.rogue2 = g.voice; u.sig2 = 0.5; judge("a giant under a voice at x18", renderProgramme(w, RP_VOICE, 12.0, nullptr, nullptr, u)); }
            else { ok = false; detail += "no people's world for the mix; "; }
            std::vector<int16_t> a, b, c; RxOpts bo; bo.voice = RP_BEACON;
            const RenderStats rb = renderRogue(g, 12, &a, bo); renderRogue(g, 12, &b); renderRogue(g, 12, &c);
            judge("a beacon under a giant", rb);
            long differ = 0; for (size_t i = 0; i < std::min(a.size(), b.size()); i++) if (a[i] != b[i]) differ++;
            beaconOk = differ > (long)a.size() / 4; same = b == c;
        }
        Distinctness d;
        if (all) { const std::vector<RogueRef> some(field.begin(), field.begin() + 24); d = rogueDistinctness(some, 20, 60.0, SIG_MAGNETOSPHERE); }
        check("radar: the drifters' voices, no two alike (D-03)", ok && beaconOk && same && all && d.under == 0,
              fmt("%d renders%s, rms %.3f-%.3f; a beacon in its slot %s; the same twice %s; the 24 nearest home: the nearest pair %.2f apart, the median %.2f; two magnetospheres' median %.2f",
                  (int)jobs.size() + 2, detail.empty() ? "" : (" - " + detail).c_str(), lo, hi, beaconOk ? "heard" : "NOT HEARD", same ? "yes" : "NO", d.nearest, d.median, d.bar));
    }
    {   // a drifter's voice in the game. At home the radar tuned to the nearest drifter at its step: the receiver plays it as a drifter by
        // what it is (its class, its turning, its moon, its power as its system has them); locked and flown to, the camera off, the cabin's radio
        // carries it; the camera on, the camera's; the decoder takes the synth (nothing); on a landable drifter's ground the suit's radio, faint;
        // at a star's system nothing at all
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_d03.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        run(0.2); press(KEY_B); run(0.3);
        for (int i = 0; i < SIGNAL_GAIN_STEPS - 1; i++) press(KEY_EQUAL);
        for (int i = 0; i < 60 && !game.testRadarDeepReady(); i++) run(1.0 / 30);
        run(0.1);
        int who = -1; double ly = 1e18;
        for (int i = 0; i < game.testRadarSignals(); i++) if (game.testRadarKind(i) == SIG_ROGUE && !game.testRadarLocal(i) && game.testRadarDist(i) < ly) { ly = game.testRadarDist(i); who = i; }
        int64_t wx = 0, wy = 0, wz = 0; if (who >= 0) game.testRadarSector(who, wx, wy, wz);
        const double need = who >= 0 ? game.testRadarNeedGain(who) : 1e9;
        int step = 0; while (step < SIGNAL_GAIN_STEPS - 1 && SIGNAL_GAINS[step] < need) step++;
        for (int i = SIGNAL_GAIN_STEPS - 1; i > step; i--) press(KEY_MINUS);
        run(0.2);
        for (int i = 0; i < game.testRadarSignals(); i++) { int64_t sx, sy, sz; if (game.testRadarKind(i) == SIG_ROGUE && game.testRadarSector(i, sx, sy, sz) && sx == wx && sy == wy && sz == wz) who = i; }
        if (who >= 0) game.testAimAtSignal(who);
        run(0.5);
        Star d; drifterInSector(wx, wy, wz, d, true);
        Signal want; { StarSystem sys; sys.generate(d); want.bodyType = sys.bodies[0].type; want.power = drifterPower(d); rogueVoiceOf(sys, want); }
        const RogueVoice wv = rogueVoiceFor(want), hv = game.audio.radarRogue;
        const bool onRadar = who >= 0 && game.audio.radarKind == SIG_ROGUE && game.audio.radarSeed == d.seed && game.audio.radarSet == RADIO_CAMERA && hv.cls == wv.cls &&
                             std::fabs(hv.turnS - wv.turnS) < 1e-6 && std::fabs(hv.moonS - wv.moonS) < 1e-6 && std::fabs(hv.power - wv.power) < 1e-9 && wv.turnS > 0;
        double lockAt = -1; for (int i = 0; i < 30 * 20 && lockAt < 0; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); if (game.testRadarLocked() >= 0) lockAt = (i + 1) / 30.0; }
        press(KEY_ENTER); run(0.1);
        for (int i = 0; i < 30 * 90 && game.testShipMode() == (int)ShipState::VIMANA; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
        run(0.5);
        const bool there = game.testSystem().sunless() && game.testSystem().star.seed == d.seed;
        const double cabinLvl = game.audio.radar;
        const bool cabin = there && !game.testRadarOn() && cabinLvl == RADIO_CABIN_LEVEL && game.audio.radarSet == RADIO_CABIN && game.audio.radarKind == SIG_ROGUE && game.audio.radarSeed == d.seed && game.audio.radarRogue.cls == wv.cls;
        press(KEY_B); run(0.3);
        const bool camera = game.testRadarOn() && game.audio.radar == 1 && game.audio.radarSet == RADIO_CAMERA;
        press(KEY_B); run(0.1);
        const bool back = game.audio.radar == RADIO_CABIN_LEVEL && game.audio.radarSet == RADIO_CABIN;
        game.testOpenShards(); run(0.1);
        const bool decoder = game.state == GameState::SHARDS && game.audio.radar == 0;
        game.setState(GameState::SPACE); run(0.1);
        // a landable drifter's ground: the suit's radio
        Star here; starInSector(HOME_SX, HOME_SY, HOME_SZ, here, false);
        std::vector<Star> near; driftersNear(here.pos, 1500, near);
        int li = -1; for (size_t i = 0; i < near.size() && li < 0; i++) { const DrifterBody b = drifterBody(near[i]); if (KINDS[b.type].landable && !kindHas(b.type, KT_GIANT) && b.radiusKm >= 1500) li = (int)i; }
        bool suit = false; double suitLvl = -1; int suitSet = -1;
        if (li >= 0) {
            Star l; drifterInSector(near[li].sx, near[li].sy, near[li].sz, l, true);
            game.testParkAt(l, 0); run(0.3);
            const bool mapQuiet = game.state == GameState::LANDING_MAP && game.audio.radar == 0;
            game.testLandSite(); run(0.1); press(KEY_ENTER);
            for (int i = 0; i < 30 * 30 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
            run(0.3);
            suitLvl = game.audio.radar; suitSet = game.audio.radarSet;
            suit = mapQuiet && game.state == GameState::SURFACE && suitLvl == RADIO_SUIT_LEVEL && suitSet == RADIO_SUIT && game.audio.radarKind == SIG_ROGUE && game.audio.radarSeed == l.seed;
        }
        // a star's system: nothing
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
        game.testArriveAt(home); game.setState(GameState::SPACE); run(0.3);
        const bool quiet = !game.testSystem().sunless() && game.audio.radar == 0;
        check("radar: a drifter's voice in the game: the radar, the cabin's radio, the suit's (D-03)", onRadar && lockAt > 0 && there && cabin && camera && back && decoder && suit && quiet,
              fmt("the nearest %.0f ly at x%d: heard as %s (turning %.0f s, its moon %.0f s, power %.2f) %s; locked after %.1f s, there %d; the cabin's radio %d (level %.2f), the camera %d, back %d, the decoder silent %d; the suit's radio %d (level %.2f, set %d); at home %s",
                  ly, SIGNAL_GAINS[step], who >= 0 ? ROGUE_VOICE_NAMES[std::max(0, std::min((int)RV_COUNT - 1, hv.cls))] : "nothing", hv.turnS, hv.moonS, hv.power, onRadar ? "as its system has it" : "NOT AS ITS SYSTEM HAS IT", lockAt, there, cabin, cabinLvl, camera, back, decoder, suit, suitLvl, suitSet, quiet ? "nothing" : "STILL HEARD"));
        remove(game.guidePath.c_str());
    }
    {   // the drifters' kinds by the dark's table: every drifter within 3,000 ly of home drawn as a kind of the table, made a dark ocean only from a
        // rocky one (its size and heat in the dark ocean's ranges, its system's moons the rocky drifter's: drawn as it), the kinds' shares the table's
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, false);
        std::vector<Star> near; driftersNear(home.pos, 3000, near);
        int drawn[PT_COUNT] = {}, oceans = 0, bad = 0; std::string why;
        for (const Star& d0 : near) {
            const DrifterBody b = drifterBody(d0);
            bool inTable = false; for (int k = 0; k < DARK_KIND_COUNT; k++) if (DARK_KINDS[k].type == b.drawnType) inTable = true;
            drawn[b.drawnType]++;
            const bool ocean = b.type == PT_DARK_OCEAN;
            if (ocean) oceans++;
            bool ok = inTable && (b.type == b.drawnType || (ocean && b.drawnType == PT_ROCKY));
            if (ocean) ok = ok && b.heatK >= DARK_OCEAN_HEAT_LO && b.heatK <= DARK_OCEAN_HEAT_HI && b.radiusKm >= KINDS[PT_DARK_OCEAN].minRadiusKm && b.radiusKm <= KINDS[PT_DARK_OCEAN].maxRadiusKm;
            if (ok && ocean) { Star d; drifterAt(d0.sx, d0.sy, d0.sz, d, false); StarSystem sys; sys.generate(d); ok = sys.bodies[0].type == PT_DARK_OCEAN && drawnKind(sys.bodies[0]) == PT_ROCKY; }
            if (!ok) { bad++; if (why.empty()) why = fmt(" (%s drawn as %s)", KINDS[b.type].name, KINDS[b.drawnType].name); }
        }
        double W = 0; for (int k = 0; k < DARK_KIND_COUNT; k++) W += DARK_KINDS[k].weight;
        int offShare = 0; std::string shares;
        for (int k = 0; k < DARK_KIND_COUNT; k++) {
            const double want = near.size() * DARK_KINDS[k].weight / W;
            if (want >= 12 && (drawn[DARK_KINDS[k].type] < 0.5 * want || drawn[DARK_KINDS[k].type] > 1.6 * want)) offShare++;
            if (want >= 12) shares += fmt(" %s %d/%.0f", KINDS[DARK_KINDS[k].type].shortName, drawn[DARK_KINDS[k].type], want);
        }
        const double oceanShare = drawn[PT_ROCKY] ? (double)oceans / drawn[PT_ROCKY] : 0;
        check("dark: the drifters' kinds by the dark's table (D-04)", !near.empty() && bad == 0 && offShare == 0 && oceanShare > 0.2 && oceanShare < 0.6,
              fmt("%zu drifters, %d off the table%s; drawn against the table:%s; dark oceans %d of the %d rocky (%.2f)", near.size(), bad, why.c_str(), shares.c_str(), oceans, drawn[PT_ROCKY], oceanShare));
        // the drifters' own traits and the ground in the dark, over the same systems' landable bodies: FROZEN AIR exactly where the air is colder
        // than its freezing, EMBERS on every lava world and on none of ice, LIGHTS IN THE WATER on dark oceans alone, LIGHTS LEFT ON on none yet (D-05's);
        // a frozen sea's world with no liquid on its ground (400 samples), a dark ocean's sea liquid; and none of the four on a world with a sun
        int bodies = 0, wrongAir = 0, wrongEmbers = 0, wrongLights = 0, leftOn = 0, frozenWet = 0, oceanDry = 0, frozenChecked = 0, embers = 0, airs = 0;
        for (const Star& d0 : near) {
            Star d; drifterAt(d0.sx, d0.sy, d0.sz, d, false);
            StarSystem sys; sys.generate(d);
            for (const Body& b : sys.bodies) {
                if (!KINDS[b.type].landable) continue;
                bodies++;
                const BodyGen g = BodyGen::make(b);
                if (g.hasTrait(TR_FROZEN_AIR) != airFrozen(b)) wrongAir++;
                if (airFrozen(b)) airs++;
                if (g.hasTrait(TR_EMBERS)) embers++;
                if ((isLavaWorld(b.type) && !g.hasTrait(TR_EMBERS)) || (g.hasTrait(TR_EMBERS) && kindHas(b.type, KT_FROZEN))) wrongEmbers++;
                if (g.hasTrait(TR_LIGHTS_IN_WATER) && b.type != PT_DARK_OCEAN) wrongLights++;
                if (g.hasTrait(TR_LIGHTS_LEFT_ON)) leftOn++;
                if ((g.frozenSeas || b.type == PT_DARK_OCEAN) && frozenChecked < 40) {
                    frozenChecked++;
                    DrainageOff off;
                    int wet = 0;
                    for (int i = 0; i < 400; i++) {
                        const double z = 2.0 * ((i * 0.6180339887) - std::floor(i * 0.6180339887)) - 1, ph = i * 2.399963;
                        const SurfaceSample ss = sampleSurface(g, Vec3(std::sqrt(1 - z * z) * std::cos(ph), std::sqrt(1 - z * z) * std::sin(ph), z), 64);
                        if (ss.material == MAT_WATER || ss.water > -1e8) wet++;
                    }
                    if (g.frozenSeas && wet > 0) frozenWet++;
                    if (b.type == PT_DARK_OCEAN && wet < 300) oceanDry++;
                }
            }
        }
        int sunlit = 0, sunlitBodies = 0;
        for (int64_t x = 150; x < 250 && sunlitBodies < 3000; x++)
            for (int64_t z = 20; z < 100 && sunlitBodies < 3000; z++) {
                Star st; StarSystem sys; if (!systemAt(x, 0, z, st, sys, false)) continue;
                for (const Body& b : sys.bodies) { sunlitBodies++; const BodyGen g = BodyGen::make(b); for (int t = TR_FROZEN_AIR; t <= TR_LIGHTS_IN_WATER; t++) if (g.hasTrait(t)) sunlit++; if (g.dark || g.frozenSeas) sunlit++; }
            }
        check("dark: the drifters' own traits, frozen seas (D-04)", wrongAir == 0 && wrongEmbers == 0 && wrongLights == 0 && leftOn == 0 && frozenWet == 0 && oceanDry == 0 && sunlit == 0 && airs > 0 && embers > 0 && frozenChecked > 0,
              fmt("%d bodies: frozen air %d (%d off its rule), embers %d (%d wrong), lights in water off a dark ocean %d, lights left on %d; %d seas read: %d frozen with liquid, %d dark oceans dry; %d worlds with a sun: %d with the dark's own",
                  bodies, airs, wrongAir, embers, wrongEmbers, wrongLights, leftOn, frozenChecked, frozenWet, oceanDry, sunlitBodies, sunlit));
    }
    {   // on a drifter's ground: the thermometer by the world's own heat, an air fallen as snow no air (no wind, a trace of pressure), a dark ocean's air
        // and sea; and the explorer's lamp, which lights the ground by night and changes nothing by day
        StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        bool rockOk = false, airOk = false, oceanOk = false; std::string rd, ad, od;
        if (setupDarkScene(0, -0.1, sv, sys, nb, t)) {
            const Body& b = sys.bodies[0];
            rockOk = std::fabs(sv.env.temperatureC - (b.tempK - 273.15)) < 0.5 && !sv.site.atmosphere;
            rd = fmt("a rocky drifter %.0f K reads %+.0f C", b.tempK, sv.env.temperatureC);
        }
        if (setupDarkScene(2, -0.1, sv, sys, nb, t)) {
            airOk = !sv.site.atmosphere && sv.env.pressureAtm < 1e-4 && sv.env.windKnots == 0 && sv.site.gen.hasTrait(TR_FROZEN_AIR);
            ad = fmt("its air fallen: atmosphere %d, %.1g atm, wind %.0f kn", (int)sv.site.atmosphere, sv.env.pressureAtm, sv.env.windKnots);
        }
        if (setupDarkScene(3, -0.1, sv, sys, nb, t)) {
            const Body& b = sys.bodies[0];
            oceanOk = sv.site.atmosphere && sv.site.hasWater && sv.env.pressureAtm > 50 && sv.env.temperatureC > 0 && std::fabs(sv.env.temperatureC + 6.5 * sv.player.y / 1000.0 - (b.tempK - 273.15)) < 0.5;
            od = fmt("a dark ocean %.0f K reads %+.0f C under %.0f atm, its sea %s", b.tempK, sv.env.temperatureC, sv.env.pressureAtm, sv.site.hasWater ? "liquid" : "NONE");
        }
        check("dark: the thermometer and the air on a drifter's ground (D-04)", rockOk && airOk && oceanOk, rd + "; " + ad + "; " + od);
        // the lamp: a dayside's frame the same lit or not (the lamps add nothing while the sun is up); a night's lower third brighter (the cratered noon's
        // site at midnight), and a drifter's (the rogue_lamp scene's)
        auto lower = [](const Framebuffer& f) { double s2 = 0; int n = 0; for (int y = FBH * 2 / 3; y < FBH; y++) for (int x = 0; x < FBW; x++) { s2 += shadeOf(f.idx[(size_t)y * FBW + x]); n++; } return s2 / n; };
        SpaceRenderer sr; int dayDiff = -1; double nightOff = 0, nightOn = 0, darkOff = 0, darkOn = 0;
        {
            StarSystem s2; SurfaceView v2; double t2 = 0; StarNeighborhood n2;
            if (setupSceneForType(PT_CRATERED, 12, 45 * DEG, 0.0, -0.1, v2, s2, n2, t2)) {   // the same state drawn twice, the lamp off and on
                Input in2; Framebuffer a, b2;
                v2.lamp = false; v2.update(0.016, in2, t2, false); v2.render(a, t2, n2.stars, sr, 1.0); v2.render(a, t2, n2.stars, sr, 1.0);
                v2.lamp = true; v2.render(b2, t2, n2.stars, sr, 1.0);
                dayDiff = 0; for (size_t i = 0; i < a.idx.size(); i++) if (a.idx[i] != b2.idx[i]) dayDiff++;
                const double tn = findTime(v2.site, -30 * DEG, t2);
                v2.lamp = false; v2.update(0.016, in2, tn, false); v2.render(a, tn, n2.stars, sr, 1.0); v2.render(a, tn, n2.stars, sr, 1.0); nightOff = lower(a);
                v2.lamp = true; v2.render(b2, tn, n2.stars, sr, 1.0); nightOn = lower(b2);
            }
        }
        if (setupDarkScene(0, -0.1, sv, sys, nb, t)) {
            Framebuffer a, b2;
            sv.lamp = false; sv.render(a, t, nb.stars, sr, 1.0); darkOff = lower(a);
            sv.lamp = true; sv.render(b2, t, nb.stars, sr, 1.0); darkOn = lower(b2);
        }
        check("surface: the explorer's lamp by night, nothing by day (D-04)", dayDiff == 0 && nightOn > nightOff + 4 && darkOn > darkOff + 4,
              fmt("a dayside frame lit and unlit: %d pixels differ; the lower third at night %.1f -> %.1f, on a drifter %.1f -> %.1f", dayDiff, nightOff, nightOn, darkOff, darkOn));
    }
    {   // the people who lost their sun. One frozen felisian or desert drifter in four kept a people (its own hash), each its former star found,
        // its peoples' cause the loss of the sun in its order (the warning, the last sunrise, the end), two peoples on one ending together
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
        std::vector<Star> near; driftersNear(home.pos, 3000, near);
        int cand = 0, withPeople = 0, unfound = 0, badCause = 0, badLine = 0, apart = 0, peoples = 0, ways[SW_COUNT] = {}; std::string why;
        for (const Star& n0 : near) {
            Star d; drifterInSector(n0.sx, n0.sy, n0.sz, d, true);
            StarSystem dsys; dsys.generate(d);
            const Body& b = dsys.bodies[0];
            if (b.type == PT_FROZEN_FELISIAN || b.type == PT_DESERT) cand++;
            if (!drifterHadPeople(b)) continue;
            withPeople++;
            if (!drifterPast(d).found) { unfound++; if (why.size() < 200) why += d.name + ": no former star; "; }
            const BodyGen g = BodyGen::make(b);
            double ended[2] = {0, 0};
            for (int k = 0; k < peoplesOf(g) && k < 2; k++) {
                const PeopleProfile P = peopleProfileOf(dsys, b, g, k);
                peoples++;
                if (P.cause != CAUSE_SUN_LOST || P.sunWay < 0 || P.sunWay >= SW_COUNT) { badCause++; continue; }
                ways[P.sunWay]++;
                if (!(P.collapseYear < P.lastSunrise && P.lastSunrise < P.lastYear)) { badLine++; if (why.size() < 200) why += fmt("%s: years %d, %d, %d; ", d.name.c_str(), P.collapseYear, P.lastSunrise, P.lastYear); }
                ended[k] = P.endedYearsAgo;
            }
            if (peoplesOf(g) > 1 && std::fabs(ended[0] - ended[1]) > 0.5) apart++;
        }
        const double share = cand ? (double)withPeople / cand : 0;
        int nWays = 0; for (int w = 0; w < SW_COUNT; w++) if (ways[w]) nWays++;
        check("drifters' peoples: one frozen felisian or desert drifter in four, each its former star found, the loss of the sun its cause in its order, two ending together (D-05)",
              withPeople > 0 && share > 0.12 && share < 0.4 && unfound == 0 && badCause == 0 && badLine == 0 && apart == 0 && nWays == SW_COUNT,
              fmt("%d of %d frozen felisian or desert drifters within 3,000 ly (%.0f%%) with %d peoples; %d without a former star, %d not lost by the sun, %d out of order, %d pairs apart; the ways %d, %d, %d, %d%s",
                  withPeople, cand, 100 * share, peoples, unfound, badCause, badLine, apart, ways[0], ways[1], ways[2], ways[3], why.empty() ? "" : (" - " + why).c_str()));
    }
    {   // a drifter's people's history the same from the decoder (its system: built anew) and from the ground (the planet function alone, as the
        // site has it: the towns, the works and the records' homes read it)
        int same = 0, n = 0; std::string why;
        for (const AgeWorld& aw : TEST_DRIFTERS) {
            Star d; if (!placeInSector(aw.sx, aw.sy, aw.sz, d, true)) continue;
            StarSystem dsys; dsys.generate(d);
            const Body& b = dsys.bodies[0]; const BodyGen g = BodyGen::make(b);
            SurfaceSite site; site.init(&dsys, 0, 0.3, 1.1, 1000.0);
            for (int k = 0; k < std::max(1, peoplesOf(g)); k++) {
                std::shared_ptr<History> a = buildHistory(dsys, b, g, k);
                std::shared_ptr<const History> c = historyOfWorld(site.gen, k);
                n++;
                if (a && c && historyHash(*a) == historyHash(*c)) same++;
                else if (why.size() < 200) why += fmt("%s people %d: %s; ", d.name.c_str(), k, a && c ? fmt("%zu and %zu events", a->events.size(), c->events.size()).c_str() : "none");
            }
        }
        check("history: a drifter's people's the same from the decoder and from the ground (D-05)", n == 3 && same == n, fmt("%d of %d the same%s", same, n, why.empty() ? "" : (" - " + why).c_str()));
    }
    {   // the works of the end on Vreikheid (two modern peoples): a shelter by each of the last places, the last holding the last recording and
        // the others the last records of their own years; a dome over warm ground by each with its glasshouses and a record or two; the graves by the last
        Star d; placeInSector(TEST_DRIFTERS[1].sx, TEST_DRIFTERS[1].sy, TEST_DRIFTERS[1].sz, d, true);
        StarSystem dsys; dsys.generate(d);
        const BodyGen g = BodyGen::make(dsys.bodies[0]);
        std::shared_ptr<const WorldWorks> W = worldWorksOf(g);
        int shelters = 0, lastShelters = 0, domes = 0, glass = 0, graves = 0, lastIn = 0, lastElse = 0, domeRecs = 0, earlyRecs = 0, late = 0;
        for (const Work& w : W->works) {
            if (w.kind == WO_SHELTER) { shelters++; if (w.before <= 0) lastShelters++; }
            if (w.kind == WO_DOME) domes++;
            if (w.kind == WO_GREENHOUSE) glass++;
            if (w.kind == WO_NECROPOLIS && w.year > 0) graves++;
            if (w.kind != WO_SHELTER && w.kind != WO_DOME) continue;
            RuinSpec sp; workSpec(g, w, sp);
            std::vector<ShardSite> sites; recordSitesOf(g, sp, cultureOf(g, w.people), sites, nullptr, nullptr);
            std::shared_ptr<const RecordCatalogue> C = recordsOf(g, w.people);
            for (const ShardSite& x : sites) {
                const int local = shardLocalOf(x.index);
                if (local == C->last) { if (w.kind == WO_SHELTER && w.before <= 0) lastIn++; else lastElse++; }
                if (w.kind == WO_DOME) domeRecs++;
                if (w.kind == WO_SHELTER && w.before > 0) { earlyRecs++; if (local >= 0 && local < C->count && C->entries[local].year > C->lastYear - (int)std::lround(w.before)) late++; }
            }
        }
        check("works: a people that lost its sun's works of the end, its last recording in the last shelter alone (D-05)",
              lastShelters == 2 && shelters > lastShelters && domes >= 2 && glass >= 2 && graves == 2 && lastIn == 2 && lastElse == 0 && domeRecs > 0 && earlyRecs > 0 && late == 0,
              fmt("%s: %d shelters (%d the last), %d domes over warm ground, %d glasshouses, %d graves of the last generations; the last recordings %d in the last shelters, %d elsewhere; %d records in the domes, %d in the other shelters (%d of years after theirs)",
                  d.name.c_str(), shelters, lastShelters, domes, glass, graves, lastIn, lastElse, domeRecs, earlyRecs, late));
    }
    {   // the beacon. Vreikheid's radio people's last broadcast (its last recording) under its voice, none on Balyaphebist (medieval); through the
        // receiver its levels C-07's; in the game the cabin's radio carries its machine under the voice, and the radar camera on the drifter speaks the
        // message and, locked, hears it (`Guide::heard`)
        Star d0, d1; placeInSector(TEST_DRIFTERS[0].sx, TEST_DRIFTERS[0].sy, TEST_DRIFTERS[0].sz, d0, true); placeInSector(TEST_DRIFTERS[1].sx, TEST_DRIFTERS[1].sy, TEST_DRIFTERS[1].sz, d1, true);
        StarSystem s0, s1; s0.generate(d0); s1.generate(d1);
        const DrifterBeacon B0 = drifterBeaconOf(s0), B1 = drifterBeaconOf(s1);
        BeaconInfo bi; RxOpts o; o.clarity = 0.85;
        const RenderStats r = renderBeacon(d1, 30, nullptr, &bi, o);
        const bool levels = bi.valid && r.nan == 0 && r.peak < 1.0 && r.clip <= 20 && r.rms >= 0.02 && r.rms <= 0.13;
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_d05.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        game.testParkAt(d1, 0, false); run(0.3);
        if (game.state == GameState::LANDING_MAP) { press(KEY_ESCAPE); run(0.1); }
        for (int i = 0; i < 30 * 120 && game.audio.radarVoice < 0; i++) run(1.0 / 30);   // its people's records written on a thread, then the beacon under the voice
        const bool cabin = game.audio.radarSet == RADIO_CABIN && game.audio.radarKind == SIG_ROGUE && B1.people >= 0 && game.audio.radarVoice == B1.machine;
        press(KEY_B); run(0.3);
        int who = -1; for (int i = 0; i < game.testRadarSignals(); i++) if (game.testRadarKind(i) == SIG_ROGUE && game.testRadarLocal(i)) who = i;
        if (who >= 0) game.testAimAtSignal(who);
        bool spoken = false; for (int i = 0; i < 30 * 120 && !spoken; i++) { run(1.0 / 30); spoken = game.audio.radio && game.audio.speech != nullptr && game.audio.radarVoice == B1.machine; }
        for (int i = 0; i < 30 * 20 && game.testRadarLocked() < 0; i++) run(1.0 / 30);
        run(0.2);
        const Lore L1 = loreQuick(s1, s1.bodies[0], BodyGen::make(s1.bodies[0]), std::max(0, B1.people));
        const std::string key = Guide::bodyKey(d1.sx, d1.sy, d1.sz, 0) + "/S" + std::to_string(shardWorldIndex(std::max(0, B1.people), L1.last));
        const bool heard = game.guide.heard.count(key) > 0;
        press(KEY_B); run(0.1);
        check("radar: a drifter's beacon, a radio people's last broadcast under its voice, heard (D-05)",
              B0.people < 0 && B1.people >= 0 && levels && bi.genre == "lastbroadcast" && cabin && spoken && heard,
              fmt("%s: %s; %s: the %s's (%s under the voice, its last recording %s, its opening %d words spoken %.0f s, rms %.3f, peak %.3f); the cabin's radio %s; the camera on it speaks it %s; locked, heard %s",
                  d0.name.c_str(), B0.people < 0 ? "none" : "ONE", d1.name.c_str(), bi.people.c_str(), RADIO_PROGRAMME_NAMES[std::max(0, B1.machine)], bi.genre.c_str(), bi.words, bi.speechS, r.rms, r.peak,
                  cabin ? "carries its machine" : "WITHOUT IT", spoken ? "yes" : "NO", heard ? "yes" : "NO"));
    }
}
