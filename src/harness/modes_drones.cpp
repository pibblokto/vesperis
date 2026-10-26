// E-03: the drone bay (C-18): `drones`, its scripted trips and their checks
#include "harness/harness.h"
#include "harness/test_worlds.h"

// ---- C-18: the drone bay -------------------------------------------------------------------------------------------------------------

static const char* dronePlaceWord(int sclass) {
    return sclass == SC_CAPITAL ? "capital" : (sclass == SC_CITY ? "city" : (sclass == SC_TOWN ? "town" : (sclass == SC_VILLAGE ? "village" : "place")));
}

// a world's items for the drones and three hauls drawn from none held (`drones world <sx> <sz> <body>`)
static int printDroneWorld(int64_t sx, int64_t sz, int body) {
    Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    StarSystem sys; sys.generate(s);
    if (body < 0 || body >= (int)sys.bodies.size()) { printf("no body %d\n", body); return 1; }
    DroneWorld W; droneWorldOf(sys, body, W);
    int kinds[DF_COUNT] = {}, homed = 0;
    for (const DroneItem& it : W.items) { kinds[std::max(0, std::min(DF_COUNT - 1, it.kind))]++; if (it.place >= 0) homed++; }
    printf("%s: %d people%s, %d items (%d texts, %d music, %d star charts, %d instruments), %d places their homes, %d items at a home, read in %.0f ms\n", sys.bodies[body].name.c_str(), W.peoples, W.peoples == 1 ? "" : "s",
           (int)W.items.size(), kinds[DF_TEXT], kinds[DF_MUSIC], kinds[DF_CHART], kinds[DF_INSTRUMENT], (int)W.places.size(), homed, W.buildMs);
    for (int i = 0; i < 3; i++) {
        DroneHaul H; droneHaulOf(W, std::vector<bool>(W.items.size(), false), 0xD00ULL + i, H);
        printf("  haul %d: %d finds\n", i + 1, H.total);
        for (int k = 0; k < DRONE_COUNT; k++) {
            printf("    drone %d wants %d:", k + 1, H.wanted[k]);
            for (const DroneLeg& leg : H.legs[k]) {
                const DronePlace* pl = leg.place >= 0 && leg.place < (int)W.places.size() ? &W.places[leg.place] : nullptr;
                printf(" [%s %d of people %d:", pl ? dronePlaceWord(pl->sclass) : "nowhere", pl ? pl->rank : -1, pl ? pl->people : -1);
                for (int idx : leg.items) { const DroneItem& it = W.items[idx]; printf(" %s", it.kind == DF_INSTRUMENT ? ("'" + it.name + "'").c_str() : DRONE_FIND_NAMES[it.kind]); }
                printf("]");
            }
            printf("\n");
        }
    }
    return 0;
}

// the bay in a game on the modern age world (two peoples): the rack, the screen, a trip from the launch to the docks, the cooldown, a recall,
// a save and a load in a trip, and the case's worlds (a folder each people of the world); frames `drones_*.png` when `frames`. `ck` reports
// each check
static void dronesGame(bool frames, const std::function<void(const char*, bool, const std::string&)>& ck) {
    mkdir("shots/tests", 0755);
    const AgeWorld& W = AGE_WORLDS[AGE_MODERN];
    Star st; if (!starInSector(W.sx, 0, W.sz, st, true)) { ck("the drone bay: a trip (C-18)", false, "no modern age world"); return; }
    StarSystem sys; sys.generate(st);
    const std::string gp = "shots/tests/test_drones_guide.txt";
    remove(gp.c_str());
    TestGame game; game.savePrefix = "shots/tests/test_drones_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = gp;
    game.newGame(); game.setState(GameState::SPACE); applyTestScale(game);
    Input gi;
    auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
    auto press = [&](int key) { pressKey(game, gi, key); };
    auto shot = [&](const std::string& what) { if (!frames) return; const std::string fn = fmt("shots/tests/drones_%s.png", what.c_str()); writePNG(fn.c_str(), game.output(), FBW, FBH); printf("    %s\n", fn.c_str()); };
    auto held = [&]() { std::set<std::string> h(game.guide.shards.begin(), game.guide.shards.end()); h.insert(game.guide.instruments.begin(), game.guide.instruments.end()); return h; };
    auto atRack = [&]() { game.setState(GameState::SPACE); game.testCabinGoto(-0.3, 1.435, -PI / 2, -0.38); run(0.1); };
    run(0.2); game.testParkAt(st, W.body); run(0.2); game.setState(GameState::SPACE); run(0.2);
    // ---- the rack with the drones aboard, E, the screen
    atRack(); run(0.2);
    const int facing = game.testCabinFacing();
    shot("rack");
    press(KEY_E); run(0.1);
    const bool open = game.state == GameState::DRONES;
    game.testDronesSettle(); run(0.2);
    const int items = game.testDronesWorldItems(-1, false), texts = game.testDronesWorldItems(DF_TEXT), music = game.testDronesWorldItems(DF_MUSIC), charts = game.testDronesWorldItems(DF_CHART), instr = game.testDronesWorldItems(DF_INSTRUMENT);
    shot("screen_ready");
    ck("the drone bay: the rack and its screen (C-18)", facing == 11 && open && items > 0 && instr > 0 && charts > 0,
       fmt("facing %d (11 the bay), the screen %s; %s holds %d for them: %d texts, %d music, %d star charts, %d instruments", facing, open ? "open" : "NOT OPEN", sys.bodies[W.body].name.c_str(), items, texts, music, charts, instr));
    // ---- the launch: Enter, the screen gone to the rack, the drones out of their ports one after another
    const std::set<std::string> before = held();
    press(KEY_ENTER);
    const bool sent = game.testDronesOut() && game.state == GameState::SPACE;
    game.testDronesSettle();
    atRack(); run(1.1);
    const double x1 = game.testDroneRackX(0), x3 = game.testDroneRackX(2), y1 = game.testDroneRackY(0);
    shot("launch");
    game.testDronesSkip(3.6);
    const double gone = game.testDroneRackX(0);
    // the lights: the ship's nose on the world, the explorer at the front window
    game.testAimAtBody(W.body); game.testCabinGoto(-0.6, 0.6, -0.25, -0.05); run(0.05);
    game.testDronesSkip(4.0); run(0.02);
    shot("lights");
    game.testDronesSkip(6.5); run(0.02);
    shot("lights_far");
    game.testDronesSkip(9.0); game.testOpenDroneBay(); run(0.05);
    const std::string descent = game.testDroneState(0);
    shot("screen_descent");
    ck("the drone bay: the launch (C-18)", sent && x1 < -2.3 && y1 > 1.0 && std::fabs(x3 - (-2.19)) < 0.01 && gone < -90 && game.testDronesPlanned() && descent == "DESCENDING",
       fmt("sent %s; at 1.2 s drone 1 at x %.2f (backing into the hull from -2.19) %.2f m up, drone 3 at x %.2f (waiting its turn); at 3.6 s drone 1 %s; the haul %s; at 9 s drone 1 %s",
           sent ? "yes" : "NO", x1, y1, x3, gone < -90 ? "gone through its port" : "STILL IN THE CABIN", game.testDronesPlanned() ? "drawn" : "NOT DRAWN", descent.c_str()));
    // ---- the search: a hand closing on a find, the ways on the map; each drone two minutes from its first place to its last (the user's)
    std::string info = game.testDronesInfo();
    double arrive = 0;
    { const size_t p = info.find(" down "); if (p != std::string::npos) arrive = atof(info.c_str() + p + 6); }
    int searchOk = 0; std::string searches;
    for (size_t p = info.find(" down "); p != std::string::npos; p = info.find(" down ", p + 6)) {
        const double d = atof(info.c_str() + p + 6); const size_t u = info.find(" up ", p);
        const double up = u != std::string::npos ? atof(info.c_str() + u + 4) : 0;
        if (std::fabs(up - d - DRONE_SEARCH_S) < 0.05) searchOk++;
        searches += fmt("%s%.1f s", searches.empty() ? "" : ", ", up - d);
    }
    game.testDronesSkip(arrive + 3.0); run(0.05);
    const std::string searching = game.testDroneState(0);
    shot("screen_search");
    atRack(); shot("rack_away");
    // ---- the docks: everything found aboard, in the guide; nothing found twice, nothing held before
    double dock = 0;
    for (size_t p = info.find(" dock "); p != std::string::npos; p = info.find(" dock ", p + 6)) dock = std::max(dock, atof(info.c_str() + p + 6));
    game.testDronesSkip(dock + 0.5); run(0.05);
    const int total = game.testDronesFinds(), aboard = game.testDronesAboard();
    int perOk = 0, wantedSum = 0; std::set<std::string> keys; int dup = 0, old = 0, kinds[DF_COUNT] = {};
    for (int k = 0; k < DRONE_COUNT; k++) {
        const int n = game.testDronesOfDrone(k), w = game.testDronesWanted(k);
        wantedSum += w;
        if (n == w && n >= DRONE_MIN_FINDS && n <= DRONE_MAX_FINDS) perOk++;
        for (int i = 0; i < n; i++) { const std::string key = game.testDronesFindKey(k, i); if (!keys.insert(key).second) dup++; if (before.count(key)) old++; }
    }
    for (int kd = 0; kd < DF_COUNT; kd++) kinds[kd] = game.testDronesFinds(kd);
    const std::set<std::string> after = held();
    const int grew = (int)after.size() - (int)before.size();
    game.testOpenDroneBay(); run(0.05); shot("screen_back");
    // R-414: the list goes once the rack has shown it: closed, the finds and the places searched are gone from it, the guide keeps them
    const int listed = game.testDronesFinds();
    press(KEY_ESCAPE); run(0.05);
    const int afterSeen = game.testDronesFinds(); const bool keptInGuide = held() == after;
    game.testOpenDroneBay(); run(0.05); shot("screen_seen"); press(KEY_ESCAPE); run(0.05);
    ck("the drone bay: the finds go from the rack once seen (R-414)", listed == total && afterSeen == 0 && keptInGuide && game.state == GameState::SPACE,
       fmt("%d listed on the screen; after it closed %d on the rack, the guide's %s", listed, afterSeen, keptInGuide ? "as they were" : "CHANGED"));
    ck("the drone bay: a trip's finds aboard (C-18)", searching.rfind("IN ", 0) == 0 && !game.testDronesOut() && perOk == DRONE_COUNT && total >= 6 && total <= 24 && aboard == total && grew == total && dup == 0 && old == 0 && searchOk == DRONE_COUNT,
       fmt("drone 1 at a stop %s; searching %s (two minutes each %d of 3); %d finds (%d texts, %d music, %d charts, %d instruments) of %d wanted, each drone 2 to 8 and what it wanted %d of 3, %d aboard, the guide %d more; %d found twice, %d held before; %s",
           searching.c_str(), searches.c_str(), searchOk, total, kinds[DF_TEXT], kinds[DF_MUSIC], kinds[DF_CHART], kinds[DF_INSTRUMENT], wantedSum, perOk, aboard, grew, dup, old, trunc(game.testStatus(), 40).c_str()));
    // ---- the charge: refused until five minutes from the return (not from the launch), then a second trip that brings none of the first's,
    // watched on the decoder: the world's records listed open there grow as the drones dock (the user's: "when drones arrive everything
    // immediately appears on shard decoder")
    const bool refused = !game.testDronesLaunch();
    const std::string why = game.testStatus();
    const double back1 = game.testDronesBack();
    game.testDronesSkip(DRONE_COOLDOWN_S + 0.1); const bool refusedAtOld = !game.testDronesLaunch();   // five minutes from the launch: still charging
    game.testDronesSkip(back1 + DRONE_COOLDOWN_S - 2); const bool stillRefused = !game.testDronesLaunch();
    game.testDronesSkip(back1 + DRONE_COOLDOWN_S + 0.1);
    const std::set<std::string> before2 = held();
    const bool second = game.testDronesLaunch();
    game.testDronesSettle();
    game.setState(GameState::SPACE); game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.1);
    press(KEY_E); run(0.1);
    const bool decoderOpen = game.state == GameState::SHARDS && game.testShardsLevel() == 0;
    press(KEY_ENTER); run(0.1);
    const int listedBefore = game.testShardsListed(), levelBefore = game.testShardsLevel();
    info = game.testDronesInfo(); dock = 0;
    for (size_t p = info.find(" dock "); p != std::string::npos; p = info.find(" dock ", p + 6)) dock = std::max(dock, atof(info.c_str() + p + 6));
    game.testDronesSkip(dock + 0.5); run(0.1);
    int heldNow = 0, which = -1;
    {   // the open world's people's shards in the guide now
        const std::string si = game.testShardsInfo(); const size_t pp = si.find("(people ");
        which = pp != std::string::npos ? atoi(si.c_str() + pp + 8) - 1 : -1;
        const std::string prefix = Guide::bodyKey(W.sx, 0, W.sz, W.body) + "/S";
        for (const std::string& k : game.guide.shards) if (k.compare(0, prefix.size(), prefix) == 0 && shardPeopleOf(atoi(k.c_str() + prefix.size())) == which) heldNow++;
    }
    const int listedAfter = game.testShardsListed(), levelAfter = game.testShardsLevel();
    shot("decoder_after");
    press(KEY_ESCAPE); run(0.05); press(KEY_ESCAPE); run(0.05);
    int again = 0, n2 = 0;
    for (int k = 0; k < DRONE_COUNT; k++) for (int i = 0; i < game.testDronesOfDrone(k); i++) { n2++; if (before2.count(game.testDronesFindKey(k, i))) again++; }
    ck("the drone bay: five minutes charging from the return (C-18)", refused && refusedAtOld && stillRefused && second && !game.testDronesOut() && n2 >= 6 && again == 0,
       fmt("refused after the trip (%s), at %.0f s from the launch still, and at %.0f s (back at %.1f), sent at %.1f; the second brought %d, %d of them held before", trunc(why, 40).c_str(), DRONE_COOLDOWN_S + 0.1, back1 + DRONE_COOLDOWN_S - 2, back1, back1 + DRONE_COOLDOWN_S + 0.1, n2, again));
    ck("the decoder open as the drones dock (C-18)", decoderOpen && levelBefore == 1 && levelAfter == 1 && which >= 0 && listedAfter == heldNow && listedAfter >= listedBefore,
       fmt("the decoder %s; people %d's records listed %d before the docks, %d after (the guide %d), at the world's records %s", decoderOpen ? "open" : "NOT OPEN", which + 1, listedBefore, listedAfter, heldNow, levelBefore == 1 && levelAfter == 1 ? "throughout" : "NOT THROUGHOUT"));
    // ---- a recall: the ship leaves its parking mid-trip; what was found comes aboard, the rest stays down there
    game.testDronesSkip(game.testDronesBack() + DRONE_COOLDOWN_S + 0.1);
    const std::set<std::string> before3 = held();
    const bool third = game.testDronesLaunch();
    game.testDronesSettle();
    info = game.testDronesInfo();
    { const size_t p = info.find(" down "); arrive = p != std::string::npos ? atof(info.c_str() + p + 6) : 20; }
    game.testDronesSkip(arrive + 40);
    const int wantedAll = game.testDronesFinds();
    game.testPlaceShip(game.testShipPos()); run(0.05);   // the parking left
    const int brought = (int)held().size() - (int)before3.size();
    ck("the drone bay: called back as the ship leaves (C-18)", third && !game.testDronesOut() && brought == game.testDronesAboard() && brought > 0 && brought < wantedAll && game.testStatus().find("AS THE SHIP LEFT") != std::string::npos,
       fmt("sent %s; %d of the %d planned aboard when the ship left at %.0f s; %s", third ? "yes" : "NO", brought, wantedAll, arrive + 40, trunc(game.testStatus(), 44).c_str()));
    // ---- saved in a trip and loaded: it goes on and docks
    game.setState(GameState::SPACE); game.testParkAt(st, W.body); run(0.1); game.setState(GameState::SPACE);
    game.testDronesSkip(game.testDronesClock() + DRONE_COOLDOWN_S + 1);
    const bool fourth = game.testDronesLaunch();
    game.testDronesSettle();
    game.testDronesSkip(25);
    const std::string mid = game.testDronesInfo();
    game.saveSlot(1);
    const std::set<std::string> before4 = held();
    const bool loaded = game.loadSlot(1);
    const std::string mid2 = game.testDronesInfo();
    info = mid2; dock = 0;
    for (size_t p = info.find(" dock "); p != std::string::npos; p = info.find(" dock ", p + 6)) dock = std::max(dock, atof(info.c_str() + p + 6));
    game.testDronesSkip(dock + 0.5);
    const int brought4 = (int)held().size() - (int)before4.size();
    const std::string runs = mid.substr(std::min(mid.size(), mid.find("drone 1:"))), runs2 = mid2.substr(std::min(mid2.size(), mid2.find("drone 1:")));
    ck("the drone bay: a trip saved and loaded (C-18)", fourth && loaded && !runs.empty() && runs == runs2 && !game.testDronesOut() && brought4 == game.testDronesAboard() && brought4 >= 6,
       fmt("sent %s, loaded %s; the drones %s; %d aboard at the end", fourth ? "yes" : "NO", loaded ? "yes" : "NO", runs == runs2 ? "as they were" : ("NOT AS THEY WERE: " + trunc(runs, 60) + " | " + trunc(runs2, 60)).c_str(), brought4));
    remove("shots/tests/test_drones_save_1.txt");
    // ---- the case: its worlds as folders (the drones' world and another's instrument), a world opened, closed; the drones' world, one of
    // two peoples, a folder each people (an instrument of each put in the case), each opening its own people's alone
    auto caseAdd = [&](int64_t sx, int64_t sz, int body, int which) {   // the first instrument of a world's people, into the case
        Star is; StarSystem isys;
        if (!starInSector(sx, 0, sz, is, true)) return;
        isys.generate(is);
        if (body >= (int)isys.bodies.size()) return;
        const BodyGen ig = BodyGen::make(isys.bodies[body]);
        if (which >= std::max(1, peoplesOf(ig))) return;
        const Lore L = loreQuick(isys, isys.bodies[body], ig, which); const Tradition T = traditionOf(ig, L);
        PeopleInstruments P; instrumentsOf(T, L.which, P);
        for (int a = 0; a < AGE_COUNT; a++) if (!P.byAge[a].empty()) { game.guide.instruments.insert(Guide::bodyKey(sx, 0, sz, body) + "/I" + std::to_string(P.byAge[a][0].id)); break; }
    };
    caseAdd(AGE_WORLDS[AGE_INDUSTRIAL].sx, AGE_WORLDS[AGE_INDUSTRIAL].sz, AGE_WORLDS[AGE_INDUSTRIAL].body, AGE_WORLDS[AGE_INDUSTRIAL].which);
    const std::string twoKey = Guide::bodyKey(W.sx, 0, W.sz, W.body);
    const bool two = peoplesOf(BodyGen::make(sys.bodies[W.body])) > 1;
    for (int p = 0; p < 2 && two; p++) caseAdd(W.sx, W.sz, W.body, p);
    auto keyFits = [&](const std::string& k, const std::string& world, int which) { return k.rfind(world + "/I", 0) == 0 && atoi(k.c_str() + world.size() + 2) / 1000 == which; };
    game.setState(GameState::SPACE); game.testCabinGoto(-1.9, -1.38, -PI / 2, -0.25); run(0.1);
    press(KEY_E); run(0.1);
    const bool caseOpen = game.state == GameState::INSTRUMENTS && game.testCaseLevel() == 0;
    int counted = 0; for (int w = 0; w < game.testCaseWorlds(); w++) counted += game.testCaseWorldItems(w);
    shot("case_worlds");
    press(KEY_ENTER); run(0.1);
    const int lvl = game.testCaseLevel(), nk = game.testCaseKeys();
    const int wsel = game.testCaseWorldSel();
    int ofWorld = 0; for (int i = 0; i < nk; i++) if (keyFits(game.testCaseKey(i), game.testCaseWorldKey(wsel), game.testCaseWorldWhich(wsel))) ofWorld++;
    shot("case_world");
    press(KEY_ESCAPE); run(0.05);
    const int lvlBack = game.testCaseLevel(); const bool stillOpen = game.state == GameState::INSTRUMENTS;
    int twoFolders = 0, twoRight = 0, whichSeen = 0; std::string twoLines;
    for (int w = 0; w < game.testCaseWorlds(); w++) {   // the world of two's folders, opened in turn
        if (game.testCaseWorldKey(w) != twoKey) continue;
        const int which = game.testCaseWorldWhich(w);
        twoFolders++; whichSeen |= 1 << std::max(0, std::min(1, which));
        for (int k = 0; k < game.testCaseWorlds() && game.testCaseWorldSel() != w; k++) press(KEY_DOWN);
        press(KEY_ENTER); run(0.1);
        const int n = game.testCaseKeys();
        bool right = game.testCaseLevel() == 1 && n > 0 && n == game.testCaseWorldItems(w);
        for (int i = 0; i < n; i++) if (!keyFits(game.testCaseKey(i), twoKey, which)) right = false;
        if (right) twoRight++;
        twoLines += fmt("%speople %d: %d of its people's%s", twoLines.empty() ? "" : "; ", which + 1, n, right ? "" : " (NOT ITS OWN ALONE)");
        if (which == 1) shot("case_world_second");
        press(KEY_ESCAPE); run(0.05);
    }
    press(KEY_ESCAPE); run(0.05);
    ck("the instruments' case by its worlds (C-18)", caseOpen && game.testCaseWorlds() >= 2 && counted == (int)game.guide.instruments.size() && lvl == 1 && nk == game.testCaseWorldItems(wsel) && ofWorld == nk && lvlBack == 0 && stillOpen && game.state == GameState::SPACE,
       fmt("%d folders holding %d of the %d in the case; the first opened: %d instruments, %d of its world and people; Esc back to the worlds %s, again to the cabin %s", game.testCaseWorlds(), counted, (int)game.guide.instruments.size(), nk, ofWorld,
           lvlBack == 0 && stillOpen ? "yes" : "NO", game.state == GameState::SPACE ? "yes" : "NO"));
    ck("the instruments' case: a world of two peoples a folder each people (C-18)", two && twoFolders == 2 && whichSeen == 3 && twoRight == 2,
       fmt("%s: %s; %d folders%s%s", sys.bodies[W.body].name.c_str(), two ? "two peoples" : "NOT TWO PEOPLES", twoFolders, twoLines.empty() ? "" : ", ", twoLines.c_str()));
    remove(gp.c_str());
}

// the unit's: a world's items and the hauls drawn from them (peoples/drones.h), then the bay in the game
void droneChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    const AgeWorld& W = AGE_WORLDS[AGE_MODERN];
    Star s; StarSystem sys; bool ok = starInSector(W.sx, 0, W.sz, s, true);
    if (ok) { sys.generate(s); ok = W.body < (int)sys.bodies.size(); }
    if (!ok) { check("the drones: a world's items (C-18)", false, "no modern age world"); return; }
    const BodyGen g = BodyGen::make(sys.bodies[W.body]);
    DroneWorld D; droneWorldOf(sys, W.body, D);
    int kinds[DF_COUNT] = {}, homed = 0, records = 0;
    for (int p = 0; p < std::max(1, peoplesOf(g)); p++) records += recordCountOf(g, p);
    for (const DroneItem& it : D.items) { kinds[std::max(0, std::min(DF_COUNT - 1, it.kind))]++; if (it.place >= 0) homed++; }
    check("the drones: a world's items (C-18)", D.peoples == peoplesOf(g) && kinds[DF_TEXT] + kinds[DF_MUSIC] + kinds[DF_CHART] == records && kinds[DF_INSTRUMENT] > 0 && kinds[DF_CHART] > 0 && D.homes && homed == (int)D.items.size(),
          fmt("%s: %d peoples, %d records of %d, %d texts %d music %d charts, %d instruments, %d of %d at a home among %d places, %.0f ms", sys.bodies[W.body].name.c_str(), D.peoples, kinds[0] + kinds[1] + kinds[2], records,
              kinds[DF_TEXT], kinds[DF_MUSIC], kinds[DF_CHART], kinds[DF_INSTRUMENT], homed, (int)D.items.size(), (int)D.places.size(), D.buildMs));
    // two hundred hauls from nothing held: each drone two to eight and what it wanted, nothing twice, three stops at most, each find at its stop
    int bad = 0, totalMin = 99, totalMax = 0, seen[DF_COUNT] = {}, finds = 0, ownFinds = 0, ofPeople[2] = {0, 0}, bothPeoples = 0;
    for (int i = 0; i < 200; i++) {
        DroneHaul H; droneHaulOf(D, std::vector<bool>(D.items.size(), false), 0x5EED0ULL + i, H);
        std::set<int> got; int sum = 0, peoples = 0;
        for (int k = 0; k < DRONE_COUNT; k++) {
            int n = 0;
            if (H.wanted[k] < DRONE_MIN_FINDS || H.wanted[k] > DRONE_MAX_FINDS || (int)H.legs[k].size() > DRONE_MAX_STOPS) bad++;
            for (const DroneLeg& leg : H.legs[k]) for (int idx : leg.items) {   // each the place's own or a copy of its people's
                n++; seen[D.items[idx].kind]++; finds++;
                const int p = std::max(0, std::min(1, D.items[idx].people)); ofPeople[p]++; peoples |= 1 << p;
                const bool own = D.items[idx].place == leg.place, copy = leg.place >= 0 && D.items[idx].people == D.places[leg.place].people;
                if (!got.insert(idx).second || !(own || copy)) bad++; else if (own) ownFinds++;
            }
            if (n != H.wanted[k]) bad++;
            sum += n;
        }
        if (sum != H.total) bad++;
        if (peoples == 3) bothPeoples++;
        totalMin = std::min(totalMin, sum); totalMax = std::max(totalMax, sum);
    }
    // all but five held: those five; all held: nothing
    std::vector<bool> heldAll(D.items.size(), true);
    for (int i = 0; i < 5 && i < (int)heldAll.size(); i++) heldAll[i * 7 % heldAll.size()] = false;
    DroneHaul H5; droneHaulOf(D, heldAll, 0x5EEDULL, H5);
    std::vector<bool> none(D.items.size(), true);
    DroneHaul H0; droneHaulOf(D, none, 0x5EEDULL, H0);
    const double shareI = finds ? (double)seen[DF_INSTRUMENT] / finds : 0, shareC = finds ? (double)seen[DF_CHART] / finds : 0;
    const double itemI = (double)kinds[DF_INSTRUMENT] / std::max<size_t>(1, D.items.size()), itemC = (double)kinds[DF_CHART] / std::max<size_t>(1, D.items.size());
    check("the drones: the hauls (C-18)", bad == 0 && totalMin >= 6 && totalMax <= 24 && seen[DF_TEXT] && seen[DF_MUSIC] && seen[DF_CHART] && seen[DF_INSTRUMENT] && H5.total == 5 && H0.total == 0 && shareI > itemI && shareC > itemC,
          fmt("200 hauls: %d wrong, %d to %d finds, %d%% a place's own; found %d texts %d music %d charts %d instruments (instruments %.1f%% of the finds, %.1f%% of the items; charts %.1f%%, %.1f%%); all but five held: %d; all held: %d",
              bad, totalMin, totalMax, finds ? 100 * ownFinds / finds : 0, seen[0], seen[1], seen[2], seen[3], 100 * shareI, 100 * itemI, 100 * shareC, 100 * itemC, H5.total, H0.total));
    // a world of two peoples (the user asked whether one trip brings both): both peoples' found, and a trip that brought of both
    check("the drones: a world of two peoples, both in a trip (C-18)", D.peoples == 2 && ofPeople[0] > 0 && ofPeople[1] > 0 && bothPeoples > 0,
          fmt("%s: %d peoples; of the 200 hauls' %d finds %d the first people's and %d the second's; %d hauls brought of both", sys.bodies[W.body].name.c_str(), D.peoples, finds, ofPeople[0], ofPeople[1], bothPeoples));
    dronesGame(false, check);
}

int runDrones(int argc, char** argv) {
    const std::string what = argc > 2 ? argv[2] : "frames";
    if (what == "world" && argc > 5) return printDroneWorld(atoll(argv[3]), atoll(argv[4]), atoi(argv[5]));
    int failures = 0;
    const Check ck = modeCheck(failures);
    if (what == "check") droneChecks(ck);
    else if (what == "frames") dronesGame(true, ck);
    else { printf("drones [frames] | drones check | drones world <sx> <sz> <body>\n"); return 1; }
    printf("drones %s: %d failure%s\n", what.c_str(), failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
