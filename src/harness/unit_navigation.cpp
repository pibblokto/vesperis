// E-03: the unit's flights, marks and auto-explore (groups `navigation`, `sights`, `auto`); E-06: the states' table (`states`)
#include "harness/harness.h"

// a tape of two files (and one listed but gone) with their credits and gains, in shots/tests/unit_tape
static std::string tapeTestDir() {
    const std::string dir = "shots/tests/unit_tape";
    makeDir(dir); makeDir(dir + "/empty");
    for (const char* f : {"one.mp3", "two.mp3"}) if (FILE* fp = fopen((dir + "/" + f).c_str(), "wb")) { fputs("not sound: the harness plays it\n", fp); fclose(fp); }
    remove((dir + "/gone.mp3").c_str());
    if (FILE* fp = fopen((dir + "/playlist.txt").c_str(), "w")) {
        fputs("# the harness's tape (R-420)\none.mp3 | Jarrad Wright | Forwards | -1.1\ngone.mp3 | Nobody | Not There\n\ntwo.mp3 | Black Sabbath | Planet Caravan | +10.5\n", fp);
        fclose(fp);
    }
    return dir;
}

// O3, B-410, R-407, G-05 to G-07: the belts, the approach, the jump, the chart, the flight
void unitNavigation(const Check& check) {
    // belt rocks keep their identity as the belt shears
    {
        Star bs; StarSystem bsys; bool have = false;
        for (int64_t x = 150; x < 320 && !have; x++)
            for (int64_t z = 20; z < 140 && !have; z++) {
                if (!starInSector(x, 0, z, bs)) continue;
                bsys.generate(bs);
                if (!bsys.belts.empty()) have = true;
            }
        check("a belt found near the start", have);
        if (have) {
            const Belt& bl = bsys.belts[0];
            Vec3 anchor = bsys.star.pos + Vec3(0.5 * (bl.innerKm + bl.outerKm), 0, 0);
            int n = 0; BeltRock first; bool got = false;
            bsys.forBeltRocksNear(0, anchor, 1000.0, 2, 2, 1, [&](const BeltRock& rk) { n++; if (!got) { first = rk; got = true; } });
            check("rocks within two cells of the anchor", n >= 3, std::to_string(n) + " rocks");
            BeltRock later; bool same = got && bsys.beltRockAt(0, first.ir, first.ia, first.iy, first.m, 1000.0 + 7200, later);
            check("a rock is the same rock two hours later", same && std::fabs(later.radiusKm - first.radiusKm) < 1e-12 && later.seed == first.seed);
            double rr = std::sqrt((later.pos.x - bsys.star.pos.x) * (later.pos.x - bsys.star.pos.x) + (later.pos.z - bsys.star.pos.z) * (later.pos.z - bsys.star.pos.z));
            check("the rock stays inside the belt", same && rr >= bl.innerKm && rr <= bl.outerKm);
            // late in an expedition (t = 4e6 s) the rings have sheared by hundreds of cells: the rock beside the ship must still be found
            {
                double tl = 4.0e6; BeltRock rk0; bool ok0 = false;
                bsys.forBeltRocksNear(0, anchor, tl, 1, 1, 1, [&](const BeltRock& rk) { if (!ok0) { rk0 = rk; ok0 = true; } });
                int near = 0;
                if (ok0) bsys.forBeltRocksNear(0, rk0.pos, tl, 4, 4, 1, [&](const BeltRock& rk) { if (length(rk.pos - rk0.pos) < 3 * BELT_CELL_KM) near++; });
                check("rocks are found round a rock after the rings sheared", ok0 && near >= 5, std::to_string(near) + " within three cells");
            }
        }
    }
    {   // a planet approached from a belt's parking parks at the planet. The approach kept the belt's parking, so at its end the
        // parked ship was put back beside its rock; the flight computer's next target kept the belt targeted, so its approach flew to the rock
        Star bs; StarSystem bsys; int pi = -1;
        for (int64_t x = 150; x < 320 && pi < 0; x++)
            for (int64_t z = 20; z < 140 && pi < 0; z++) {
                if (!starInSector(x, 0, z, bs)) continue;
                bsys.generate(bs);
                if (bsys.belts.empty()) continue;
                for (int i = 0; i < (int)bsys.bodies.size() && pi < 0; i++) if (bsys.bodies[i].parent < 0 && bsys.bodies[i].type != PT_COMPANION) pi = i;
            }
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFrames(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        auto parkedAt = [&](int body, std::string& why) {   // waits for the parking, then checks the ship holds it over two seconds
            for (int i = 0; i < 30 * 20 && game.testDebugInfo().find("mode=3 ") == std::string::npos; i++) run(1.0 / 30);
            run(2.0);
            const StarSystem& S = game.testSystem();
            double d = length(game.testShipPos() - S.bodyPos(body, game.testGameTime())), r = S.bodies[body].radiusKm;
            why = fmt("%s, %.1f radii from it", game.testDebugInfo().c_str(), d / r);
            return game.testParkedBody() == body && game.testParkedBelt() < 0 && d < 30 * r;
        };
        bool inBelt = false, listOk = false, computerOk = false; std::string w1, w2;
        if (pi >= 0) {
            game.testParkAtBelt(bs, 0); run(0.3);
            inBelt = game.testParkedBelt() == 0;
            press(KEY_TAB); for (int i = 0; i < (int)bsys.bodies.size() - pi; i++) press(KEY_UP);   // the list opens on the belt's row, after the bodies
            press(KEY_ENTER);
            listOk = parkedAt(pi, w1);
            game.testParkAtBelt(bs, 0); run(0.3);
            game.setState(GameState::SHIPSCREEN); game.testShipScreenRow(0, 2); press(KEY_ENTER);   // LOCAL TARGET: NEXT BODY (from none: the first)
            game.setState(GameState::SHIPSCREEN); game.testShipScreenRow(0, 4); press(KEY_ENTER);   // FINE APPROACH TO THE LOCAL TARGET
            computerOk = parkedAt(0, w2);
        }
        check("a planet approached from a belt keeps its parking (B-410)", pi >= 0 && inBelt && listOk && computerOk,
              fmt("%s %lld %lld body %d: from the list %s (%s); from the flight computer %s (%s)", bs.name.c_str(), (long long)bs.sx, (long long)bs.sz, pi, listOk ? "ok" : "PULLED BACK", w1.c_str(), computerOk ? "ok" : "PULLED BACK", w2.c_str()));
    }
    {   // a jump of any length along the aim. From home the aim (R), 4 0 0 typed and Enter: the star nearest the line's end becomes the
        // remote target, the Vimana flies at once (its length by `vimanaSeconds`) and the ship comes out at that star; a jump north out of
        // the disc comes back along the line to the first stars; the aim's markers say the distance where the star has no name, VISITED
        // where the ship has been
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        run(0.3);
        const Vec3 p0 = game.testShipPos();
        const std::string home = game.testStarGenName();
        press(KEY_R); press(KEY_4); press(KEY_0); press(KEY_0); run(0.1);
        const Vec3 dir = game.testAimDirection(), end = p0 + dir * (400.0 * SECTOR_KM);
        writePNG("shots/tests/unit_r407_aim.png", game.output(), FBW, FBH);
        const std::string typed = game.testJumpDigits();
        int64_t rx = 0, ry = 0, rz = 0;
        press(KEY_ENTER);
        const bool remote = game.testRemote(rx, ry, rz), flying = game.testDebugInfo().find("mode=1 ") != std::string::npos;
        Star dest; starInSector(rx, ry, rz, dest, false);
        const double offEnd = length(dest.pos - end) / SECTOR_KM, flyLy = length(dest.pos - p0) / SECTOR_KM, dur = game.testFlightDur();
        bool logged = false; for (const LogEntry& e : game.guide.log) if (e.kind == "JUMP" && e.text.find("400 LY") != std::string::npos) logged = true;
        run(dur + 1.0);
        const bool arrived = game.testSystem().valid && game.testSystem().star.sx == rx && game.testSystem().star.sy == ry && game.testSystem().star.sz == rz;
        check("a jump of 400 ly along the aim (R-407)", typed == "400" && remote && flying && offEnd < 8 && std::fabs(dur - vimanaSeconds(flyLy)) < 0.5 && logged && arrived,
              fmt("typed '%s', from %s to sector %lld %lld %lld, %.1f ly from the line's end, %.1f ly flown in %.1f s; logged %d, arrived %d", typed.c_str(), home.c_str(), (long long)rx, (long long)ry, (long long)rz, offEnd, flyLy, dur, logged, arrived));
        Star up; double shortLy = 0;
        const bool upOk = jumpTarget(game.testShipPos(), Vec3(0, 1, 0), 3000, up, shortLy, 0);
        const bool upShort = upOk && shortLy > 100 && up.sy < 2950 && up.sy > 1000;
        Star far; double farShort = 0;
        const bool coreOk = jumpTarget(p0, normalize(Vec3(GALAXY_CENTRE_SX * SECTOR_KM, 0, GALAXY_CENTRE_SZ * SECTOR_KM) - p0), 13000, far, farShort, 0) && galaxyRegion(far.sx, far.sy, far.sz) == REGION_CORE;
        check("a jump into the void comes back to the stars", upShort && coreOk && vimanaSeconds(100) == 27.0 && std::fabs(vimanaSeconds(100000) - 86.8) < 0.1,
              fmt("3000 ly north: a star at height %lld, %.0f ly short; 13000 ly coreward: %s; flights of 100 and 100,000 ly %.1f and %.1f s", (long long)up.sy, shortLy, REGION_NAMES[galaxyRegion(far.sx, far.sy, far.sz)], vimanaSeconds(100), vimanaSeconds(100000)));
        // the aim's labels: the nearest stars of the arrival's neighbourhood, unnamed and unvisited, then the home star (visited)
        int unnamed = 0, bare = 0; std::string sample;
        for (const Star& s : game.testNeighborhood().stars) {
            if (unnamed >= 6) break;
            if (game.testSystem().valid && s.seed == game.testSystem().star.seed) continue;
            std::string l = game.testAimLabel(s); unnamed++;
            if (l.find("UNKNOWN") == std::string::npos && l.find(" LY") != std::string::npos) bare++;
            if (sample.empty()) sample = l;
        }
        Star homeStar; Guide::parseStarKey(game.guide.home, rx, ry, rz); starInSector(rx, ry, rz, homeStar, false);
        const std::string homeLabel = game.testAimLabel(homeStar);
        check("the aim's markers name no UNKNOWN, say VISITED", unnamed == 6 && bare == 6 && homeLabel.find("VISITED") != std::string::npos,
              fmt("%d of %d bare ('%s'); home '%s'", bare, unnamed, sample.c_str(), homeLabel.c_str()));
    }
    {   // the galaxy chart. From home the star map's Tab turns to the galaxy; at every level the ship's crosshair is drawn within a
        // pixel of where the level's scale puts it from the view's centre (the galactic centre on the whole disc, the ship from level 1),
        // found in the frame by its four white arms; three arrivals are the stars visited, with no line between them (R-414: G-05's route
        // is gone, from the chart and from the guide's file); in a Vimana flight M opens the galaxy page and Tab keeps it there
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        run(0.3);
        press(KEY_M); run(0.1); const int page0 = game.testMapPage();
        press(KEY_TAB); run(0.1); const int page1 = game.testMapPage();
        const Vec3 sp = game.testShipPos() / SECTOR_KM;
        int worst = 0; std::string offs;
        for (int L = 0; L < Game::CHART_LEVELS; L++) {
            game.testChartLevel(L); game.testChartFollow(); run(0.05);
            const double sc = Game::chartScale(L), cx = L == 0 ? GALAXY_CENTRE_SX : sp.x, cz = L == 0 ? GALAXY_CENTRE_SZ : sp.z;
            const double ex = UW / 2 + (sp.x - cx) / sc, ey = Game::CHART_FACE_Y0 + Game::CHART_FACE_H / 2 + (sp.z - cz) / sc;
            const uint32_t* px = game.output(); const int S = FBW / UW;
            auto white = [&](int x, int y) { return x >= 0 && y >= 0 && x < UW && y < UH && px[(size_t)(y * S) * FBW + x * S] == HUD_WHITE; };
            int fx = -99, fy = -99;
            for (int y = (int)std::floor(ey) - 3; y <= (int)std::floor(ey) + 3 && fx < -9; y++)
                for (int x = (int)std::floor(ex) - 3; x <= (int)std::floor(ex) + 3 && fx < -9; x++)
                    if (white(x + 3, y) && white(x - 3, y) && white(x, y + 3) && white(x, y - 3) && white(x + 5, y) && white(x, y - 5)) { fx = x; fy = y; }
            const int off = std::max(std::abs(fx - (int)std::floor(ex)), std::abs(fy - (int)std::floor(ey)));
            worst = std::max(worst, off); offs += fmt(" %d", off);
        }
        check("the galaxy chart puts the ship where it is (G-05)", page0 == 0 && page1 == 1 && worst <= 1, fmt("pages %d then %d; the crosshair's offset from its place by level:%s", page0, page1, offs.c_str()));
        // three arrivals at stars of the neighbourhood (its first three in the scan's order), each a frame on the chart (the visit noted there)
        std::vector<Star> stops;
        for (const Star& st : game.testNeighborhood().stars) {
            if (stops.size() >= 3) break;
            if (game.testSystem().valid && st.seed == game.testSystem().star.seed) continue;
            Star full; starInSector(st.sx, st.sy, st.sz, full, true); stops.push_back(full);
        }
        std::string keys[3];
        for (size_t k = 0; k < stops.size() && k < 3; k++) { game.testArriveAt(stops[k]); run(0.1); keys[k] = Guide::starKey(stops[k].sx, stops[k].sy, stops[k].sz); }
        game.testChartLevel(Game::CHART_LEVELS - 1); game.testChartFollow(); run(0.05);
        // the stars visited, a dot each; no line of the way the ship came between them (G-05's route, in its green, is gone)
        bool visited = stops.size() == 3;
        for (const std::string& k : keys) visited = visited && game.guide.visited.count(k);
        int routeGreen = 0;
        {
            const uint32_t* px = game.output(); const int S = FBW / UW;
            for (int y = Game::CHART_FACE_Y0; y < Game::CHART_FACE_Y0 + Game::CHART_FACE_H; y++)
                for (int x = 0; x < UW; x++) if (px[(size_t)(y * S) * FBW + x * S] == rgb(70, 130, 95)) routeGreen++;
        }
        game.guide.save("shots/tests/test_guide_route.txt");
        std::ifstream gf("shots/tests/test_guide_route.txt"); std::string gl; int routeLines = 0;
        while (std::getline(gf, gl)) if (gl.rfind("route ", 0) == 0) routeLines++;
        remove("shots/tests/test_guide_route.txt");
        check("three arrivals visited, no route drawn between them (R-414)", visited && routeGreen == 0 && routeLines == 0,
              fmt("the three %s visited; the route's green on the chart %d pixels; %d route lines in the guide", visited ? "all" : "NOT", routeGreen, routeLines));
        // a jump flies at once; M in the flight shows the galaxy (the map left on the neighbourhood), Tab is refused there
        press(KEY_TAB); run(0.05); press(KEY_ESCAPE); run(0.05);
        const int leftOn = game.testMapPage();
        press(KEY_R); press(KEY_6); press(KEY_0); press(KEY_ENTER); run(0.2);
        const bool flying = game.testDebugInfo().find("mode=1 ") != std::string::npos;
        press(KEY_M); run(0.1); const bool open = game.state == GameState::STAR_MAP; const int inFlight = game.testMapPage();
        press(KEY_TAB); run(0.1); const int afterTab = game.testMapPage();
        writePNG("shots/tests/unit_g05_flight.png", game.output(), FBW, FBH);
        press(KEY_ESCAPE); run(0.05);
        check("the galaxy page in a Vimana flight (G-05)", leftOn == 0 && flying && open && inFlight == 1 && afterTab == 1,
              fmt("left on page %d; flying %d; M opens the map %d on page %d; after Tab page %d ('%s')", leftOn, flying, open, inFlight, afterTab, game.testStatus().c_str()));
    }
    {   // the next jump chosen on the chart. From home the cursor 5,000 ly coreward in the plane: Enter plots the star nearest the point
        // (the remote target), Esc keeps it, M opens the chart on it still plotted; Tab looks at the stars round its end, where another is
        // taken and stays the plot; Enter flies (the flight of `vimanaSeconds`, the JUMP line says the chart) and the ship comes out there, the
        // chart's crosshair on it. A plot into the halo comes back to the stars as R-407's jump does (the same star as `jumpTarget`'s). L marks
        // a place 1,500 ly over the plane; the mark goes through the guide's file and an export lends it (once); N steps through the signals,
        // home, the named stars, the marks and the systems visited in that order; L on the mark renames it, Delete twice removes it
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        run(0.3);
        press(KEY_M); if (game.testMapPage() != 1) press(KEY_TAB);
        game.testChartLevel(2); run(0.05);
        const Vec3 h = game.testShipPos() / SECTOR_KM;
        const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z)), aim = Vec3(h.x, 0, h.z) + toCore * 5000;
        game.testChartCursor(aim); run(0.05);
        press(KEY_ENTER); run(0.05);
        int64_t rx = 0, ry = 0, rz = 0; const bool remote = game.testRemote(rx, ry, rz);
        Star dest; starInSector(rx, ry, rz, dest, false);
        const double offPoint = length(dest.pos / SECTOR_KM - aim);
        const bool plotted = game.testChartPlotted() && game.state == GameState::STAR_MAP;
        press(KEY_ESCAPE); run(0.05);
        int64_t kx = 0, ky = 0, kz = 0; const bool kept = game.state == GameState::SPACE && game.testRemote(kx, ky, kz) && kx == rx && ky == ry && kz == rz;
        press(KEY_M); run(0.05);
        const bool reopened = game.state == GameState::STAR_MAP && game.testMapPage() == 1 && game.testChartPlotted();
        press(KEY_TAB); run(0.05);
        const bool look = game.testChartLook() && game.testMapPage() == 0;
        const std::vector<Star> ls = game.testLookStars().stars;
        bool endIn = false; for (const Star& st : ls) if (st.sx == rx && st.sy == ry && st.sz == rz) endIn = true;
        writePNG("shots/tests/unit_g06_look.png", game.output(), FBW, FBH);
        const int pick = game.testMapPick();
        Star other; if (pick >= 0 && pick < (int)ls.size()) other = ls[pick];
        press(KEY_ENTER); run(0.05);
        int64_t ox = 0, oy = 0, oz = 0; game.testRemote(ox, oy, oz);
        const bool taken = pick >= 0 && other.valid && ox == other.sx && oy == other.sy && oz == other.sz && !(ox == rx && oy == ry && oz == rz) && game.testMapPage() == 1 && !game.testChartLook() && game.testChartPlotted();
        Star dest2; starInSector(ox, oy, oz, dest2, false);
        const Vec3 p0 = game.testShipPos();
        press(KEY_ENTER);
        const bool flying = game.testDebugInfo().find("mode=1 ") != std::string::npos && game.state == GameState::SPACE;
        const double dur = game.testFlightDur(), flyLy = length(dest2.pos - p0) / SECTOR_KM;
        bool logged = false; for (const LogEntry& e : game.guide.log) if (e.kind == "JUMP" && e.text.find("PLOTTED ON THE CHART") != std::string::npos && e.text.find("COREWARD") != std::string::npos) logged = true;
        run(dur + 1.0);
        const bool arrived = game.testSystem().valid && game.testSystem().star.sx == ox && game.testSystem().star.sy == oy && game.testSystem().star.sz == oz;
        const Vec3 at = game.testShipPos() / SECTOR_KM;
        const double fromPoint = length(at - aim);
        press(KEY_M); run(0.05);
        const bool onShip = game.testChartOnShip() && game.testMapPage() == 1;
        const int cxOff = std::abs(game.testChartYouX() - UW / 2), cyOff = std::abs(game.testChartYouY() - (Game::CHART_FACE_Y0 + Game::CHART_FACE_H / 2));
        check("a jump plotted on the chart and flown (G-06)", remote && plotted && offPoint < 5 && kept && reopened && look && endIn && ls.size() > 100 && taken && flying &&
              std::fabs(dur - vimanaSeconds(flyLy)) < 0.5 && std::fabs(dur - vimanaSeconds(5000)) < 0.2 && logged && arrived && fromPoint < 15 && onShip && cxOff <= 1 && cyOff <= 1,
              fmt("plotted %d at %lld %lld %lld, %.1f ly from the point; kept through Esc %d, reopened plotted %d; the look %d: %zu stars, the end among them %d; another taken %d (%lld %lld %lld); flying %d, %.1f s for %.0f ly (vimanaSeconds(5000) %.1f), logged %d; arrived %d, %.1f ly from the point; the chart on the ship %d, the crosshair %d %d px off",
                  plotted, (long long)rx, (long long)ry, (long long)rz, offPoint, kept, reopened, look, ls.size(), endIn, taken, (long long)ox, (long long)oy, (long long)oz, flying, dur, flyLy, vimanaSeconds(5000), logged, arrived, fromPoint, onShip, cxOff, cyOff));
        // into the halo: 20,000 ly over the plane, the plot comes back along the line to the stars
        game.testChartCursor(at + Vec3(0, 20000, 0)); run(0.05);
        press(KEY_ENTER); run(0.05);
        int64_t hx = 0, hy = 0, hz = 0; const bool haloRemote = game.testRemote(hx, hy, hz);
        Star want; double wantShort = 0;
        const bool wantOk = jumpTarget(game.testShipPos(), Vec3(0, 1, 0), 20000, want, wantShort, game.testSystem().star.seed);
        const bool same = haloRemote && wantOk && want.sx == hx && want.sy == hy && want.sz == hz && std::fabs(game.testJumpShort() - wantShort) < 1e-6;
        check("a plot into the halo comes back to the stars (G-06)", game.testChartPlotted() && same && wantShort > 10000 && hy < 5000,
              fmt("plotted %d, the star at height %lld, %.0f ly short; jumpTarget's the same %d", game.testChartPlotted(), (long long)hy, game.testJumpShort(), same));
        press(KEY_C); run(0.05);
        // a mark 1,500 ly over the plane: through the file and an export
        const Vec3 markAt = at + Vec3(0, 1500, 0);
        game.testChartCursor(markAt); run(0.05);
        press(KEY_L); run(0.05); const bool entry = game.state == GameState::TEXT_ENTRY;
        writePNG("shots/tests/unit_g06_mark_entry.png", game.output(), FBW, FBH);
        game.testTypeText("THE HIGH VOID"); run(0.05);
        const auto& gm = game.guide.galaxyMarks;
        const bool placed = gm.size() == 1 && gm[0].name == "THE HIGH VOID" && !gm[0].lent && length(Vec3(gm[0].x, gm[0].y, gm[0].z) - markAt) < 1e-6 && game.state == GameState::STAR_MAP;
        game.guide.save("shots/tests/test_guide_g06.txt");
        Guide back; const bool loaded = back.load("shots/tests/test_guide_g06.txt");
        const bool through = loaded && back.galaxyMarks.size() == 1 && back.galaxyMarks[0].name == "THE HIGH VOID" && length(Vec3(back.galaxyMarks[0].x, back.galaxyMarks[0].y, back.galaxyMarks[0].z) - markAt) < 1e-6;
        Guide friendG; int lentN = 0, probesN = 0, marksN = 0, galaxyN = 0, galaxyN2 = 0;
        const int names = friendG.importInbox("shots/tests/test_guide_g06.txt", &lentN, &probesN, &marksN, &galaxyN);
        friendG.importInbox("shots/tests/test_guide_g06.txt", nullptr, nullptr, nullptr, &galaxyN2);
        remove("shots/tests/test_guide_g06.txt");
        const bool lent = names >= 0 && galaxyN == 1 && galaxyN2 == 0 && friendG.galaxyMarks.size() == 1 && friendG.galaxyMarks[0].lent && friendG.galaxyMarks[0].name == "THE HIGH VOID";
        check("a mark of the galaxy through the guide and an export (G-06)", entry && placed && through && lent,
              fmt("the entry %d; placed %d ('%s'); through the file %d; lent %d (%d, again %d)", entry, placed, gm.empty() ? "-" : gm[0].name.c_str(), through, lent, galaxyN, galaxyN2));
        // N: the stops in their order (a signal, home, a named star, the mark, a star visited), each where it is; Shift+N back
        std::vector<Star> near;
        for (const Star& st : game.testNeighborhood().stars) { if (near.size() >= 3) break; if (st.seed != game.testSystem().star.seed) near.push_back(st); }
        std::string order, want5 = "A SIGNAL|HOME|VEGA TEST|THE HIGH VOID|A STAR VISITED|A SIGNAL|";
        bool where = near.size() == 3;
        if (near.size() == 3) {
            game.guide.signals[Guide::starKey(near[0].sx, near[0].sy, near[0].sz) + "/1"] = 1;
            game.guide.names[Guide::starKey(near[1].sx, near[1].sy, near[1].sz)] = "VEGA TEST";
            game.guide.visited.insert(Guide::starKey(near[2].sx, near[2].sy, near[2].sz));
            Star homeStar; int64_t ax = 0, ay = 0, az = 0; Guide::parseStarKey(game.guide.home, ax, ay, az); starInSector(ax, ay, az, homeStar, false);
            const Vec3 places[5] = {near[0].pos / SECTOR_KM, homeStar.pos / SECTOR_KM, near[1].pos / SECTOR_KM, markAt, near[2].pos / SECTOR_KM};
            for (int k = 0; k < 6; k++) {
                press(KEY_N); run(0.05);
                const std::string st = game.testStatus();
                order += st.substr(0, st.find(" - ")) + "|";
                if (length(game.testChartCursorAt() - places[k % 5]) > 1e-6) where = false;
            }
        }
        gi.down[KEY_LEFT_SHIFT] = true; press(KEY_N); gi.down[KEY_LEFT_SHIFT] = false; run(0.05);
        const std::string backSt = game.testStatus();
        check("N steps through what the explorer has (G-06)", order == want5 && where && backSt.rfind("A STAR VISITED - 5 OF 5", 0) == 0,
              fmt("'%s', each where it is %d; Shift+N '%s'", order.c_str(), where, backSt.c_str()));
        // the mark renamed and removed on the chart
        game.testChartCursor(markAt + Vec3(1, 0, 1)); run(0.05);
        press(KEY_L); run(0.05);
        for (int i = 0; i < 20; i++) press(KEY_BACKSPACE);
        game.testTypeText("THE TALL VOID"); run(0.05);
        const bool renamed = gm.size() == 1 && gm[0].name == "THE TALL VOID";
        writePNG("shots/tests/unit_g06_mark.png", game.output(), FBW, FBH);
        press(KEY_DELETE); run(0.05); const bool asked = gm.size() == 1 && game.testStatus().find("DELETE AGAIN") != std::string::npos;
        press(KEY_DELETE); run(0.05);
        check("a mark of the galaxy renamed and removed (G-06)", renamed && asked && gm.empty(),
              fmt("renamed %d; asked %d; left %zu ('%s')", renamed, asked, gm.size(), game.testStatus().c_str()));
        press(KEY_ESCAPE); run(0.05);
    }
    {   // the band in a Vimana flight, 5,000 ly coreward from home, a frame every sixtieth of a second: the first frame draws the
        // band the ship was seeing (the parked map, nothing built at once), no frame moves a cell of it by a twentieth of its range, the
        // last is the band built at the end and is handed to the parked band (a frame at the arrival builds nothing); the keyframes are
        // half a second apart here and closer through the core (26,000 ly from home to the far side), where the centre turns fast
        Star hs; starInSector(HOME_SX, HOME_SY, HOME_SZ, hs, true);
        const Vec3 h = hs.pos / SECTOR_KM;
        const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z));
        SpaceRenderer sr; Framebuffer fb;
        SpaceContext pc; pc.shipPos = hs.pos; pc.cam = cameraBasis(0, 0);
        sr.setupPalette(fb, nullptr, -1, -1, 1.0); sr.render(fb, pc);   // parked at home: the band the flight starts from
        const std::vector<float> parked = sr.parkedBand();
        const long builds0 = sr.parkedBandBuilds;
        SpaceContext c; c.vimana = true; c.flightFrom = hs.pos; c.flightTo = hs.pos + toCore * (5000 * SECTOR_KM); c.flightDur = vimanaSeconds(5000);
        double firstDiff = 1, worstStep = 0, worstAt = 0;
        std::vector<float> prev;
        const double t0 = nowSec();
        for (int i = 0;; i++) {
            c.flightT = std::min(c.flightDur, i / 60.0);
            const std::vector<float>& m = sr.flightBand(c);
            if (i == 0) { firstDiff = 0; for (size_t k = 0; k < m.size(); k++) firstDiff = std::max(firstDiff, (double)std::fabs(m[k] - parked[k])); }
            else for (size_t k = 0; k < m.size(); k++) { const double d = std::fabs(m[k] - prev[k]); if (d > worstStep) { worstStep = d; worstAt = c.flightT; } }
            prev = m;
            if (c.flightT >= c.flightDur) break;
        }
        const double flightMs = (nowSec() - t0) * 1e3;
        std::vector<float> end; buildGalaxyBand(c.flightTo / SECTOR_KM, end, BAND_MAP_W, BAND_MAP_H);
        double endDiff = 0; for (size_t k = 0; k < end.size(); k++) endDiff = std::max(endDiff, (double)std::fabs(prev[k] - end[k]));
        const SpaceRenderer::FlightBand F = sr.fband;
        double minDt = 1e9, maxDt = 0; for (size_t k = 1; k < F.tau.size(); k++) { minDt = std::min(minDt, F.tau[k] - F.tau[k - 1]); maxDt = std::max(maxDt, F.tau[k] - F.tau[k - 1]); }
        pc.shipPos = c.flightTo; sr.render(fb, pc);   // the arrival
        const bool handed = sr.parkedBandBuilds == builds0 && length(sr.parkedBandPos() - c.flightTo / SECTOR_KM) < 1e-6;
        SpaceContext cc = c; cc.flightTo = hs.pos + toCore * (26000 * SECTOR_KM); cc.flightDur = vimanaSeconds(26000); cc.flightT = 0;
        sr.flightBand(cc);
        double coreMin = 1e9, coreAt = 0;
        for (size_t k = 1; k < sr.fband.tau.size(); k++) if (sr.fband.tau[k] - sr.fband.tau[k - 1] < coreMin) { coreMin = sr.fband.tau[k] - sr.fband.tau[k - 1]; coreAt = length(sr.flightBandPos(sr.fband.tau[k - 1]) - Vec3(GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ)); }
        check("the band in a Vimana flight (G-07)", firstDiff == 0 && F.forced == 0 && F.built + 1 == (long)F.tau.size() && worstStep < 0.05 && endDiff < 1e-6 && handed &&
              maxDt <= 0.5 + 1e-9 && minDt > 0.025 && coreMin < 0.1 && coreAt < 2000,
              fmt("the first frame the parked band (%.6f); %zu keyframes %.2f-%.2f s apart, %ld built, %ld at once, %.0f ms for the %.1f s; the worst step %.4f at %.1f s; the end %.7f off the band built there; handed to the parked band %d; through the core %zu keyframes, the closest %.3f s apart %.0f ly from the centre",
                  firstDiff, F.tau.size(), minDt, maxDt, F.built, F.forced, flightMs, c.flightDur, worstStep, worstAt, endDiff, handed, sr.fband.tau.size(), coreMin, coreAt));
    }
}

// R-408: the sights by kind and the explorer's marks
void unitSights(const Check& check) {
    {   // the sights by kind and the explorer's marks. On Aieliaalas II (a people's world): the cells round a mountain place give lakes
        // that are water where they are heard (a playa or a frozen basin is no lake) and peaks over the ground round them; a sight named under O6-06 (`<world>/L<id>`) becomes a mark where it
        // stands at the arrival; on the ground the scanner (R) hears the ruins, the nearest heard within 8% of its distance; a mark placed on
        // it from the map's cursor leaves it out (the next ruin is the nearest), Shift+R takes it in again; the map's Enter renames, Delete
        // twice removes, M puts the waypoint at the cursor; L marks the rangefinder's spot; the marks go through the guide and lend
        Star ws; StarSystem wsys; const bool haveWorld = starInSector(151, 0, 25, ws, true);
        if (haveWorld) wsys.generate(ws);
        const BodyGen g = haveWorld && wsys.bodies.size() > 1 ? BodyGen::make(wsys.bodies[1]) : BodyGen();
        const Vec3 hill = StarSystem::bodyFromLatLon(-30 * DEG, -100 * DEG);
        std::vector<Landmark> sights;
        if (haveWorld) sightsNear(g, hill, 40000.0, sightKindsOf(g) & ~(1u << LM_RUIN), sights);
        int lakes = 0, wet = 0, peaks = 0, tops = 0;
        for (const Landmark& L : sights) {
            if (L.kind == LM_LAKE) { lakes++; SurfaceSample sm = sampleSurface(g, L.unit, 64.0); if (sm.material == MAT_WATER || (sm.water > -1e8 && sm.height < sm.water + 0.5)) wet++; }
            if (L.kind == LM_PEAK) {
                peaks++; bool top = true;
                Vec3 e = normalize(cross(Vec3(0, 0, 1), L.unit)), n = cross(L.unit, e);
                for (int k = 0; k < 8 && top; k++) { double a = k * PI / 4; if (sampleSurface(g, normalize(L.unit + (e * std::cos(a) + n * std::sin(a)) * (400.0 / (g.R * 1000.0))), 64.0).height > L.heightM + 5) top = false; }
                if (top) tops++;
            }
        }
        check("the sights by kind: lakes are water, peaks the tops (R-408)", lakes >= 3 && wet * 5 >= lakes * 4 && peaks >= 3 && tops * 4 >= peaks * 3,   // a summit of the 500 m scan on a ridge can have higher ground 400 m along it
              fmt("within 40 km of 30 S 100 W: %d lakes (%d of them water), %d peaks (%d over the ground 400 m round)", lakes, wet, peaks, tops));
        // a legacy name: the O6-06 landmark of the place's cell (else of the nearest cell holding one), named as the explorer would have named it
        Landmark legacy; std::string legacyKey;
        if (haveWorld) {
            int ci0, cj0; landmarkCellOf(g, hill, ci0, cj0);
            for (int d = 0; d < 4 && legacyKey.empty(); d++)
                for (int di = -d; di <= d && legacyKey.empty(); di++)
                    for (int dj = -d; dj <= d && legacyKey.empty(); dj++) {
                        if (std::max(std::abs(di), std::abs(dj)) != d) continue;
                        CellSights S; Landmark L;
                        if (sightsOfCell(g, ci0 + di, cj0 + dj, S) && legacyLandmarkOf(S, L)) { legacy = L; legacyKey = Guide::bodyKey(ws.sx, ws.sy, ws.sz, 1) + "/L" + std::to_string((unsigned long long)(L.id & 0xffffffffULL)); }
                    }
        }
        remove("shots/tests/test_r408_guide.txt");
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_r408_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        auto shiftPress = [&](int key) { gi.down[KEY_LEFT_SHIFT] = true; press(key); gi.down[KEY_LEFT_SHIFT] = false; };
        run(0.2);
        if (!legacyKey.empty()) game.guide.names[legacyKey] = "City of tests";
        game.testParkAt(ws, 1); run(0.2);
        for (int i = 0; i < 30 * 15 && !legacyKey.empty() && game.guide.names.count(legacyKey); i++) run(1.0 / 30);   // the cells are found on a thread
        bool migrated = false; double mLat, mLon; StarSystem::latLonFromBody(legacy.unit, mLat, mLon);
        for (const SurfaceMark& m : game.guide.marks) if (m.name == "City of tests" && m.kind == legacy.kind && std::fabs(m.lat - mLat) < 1e-9 && std::fabs(m.lon - mLon) < 1e-9 && !m.lent) migrated = true;
        check("a sight named before is a mark where it stands", !legacyKey.empty() && migrated && !game.guide.names.count(legacyKey),
              fmt("%s (%s): %s; the old name %s", legacyKey.c_str(), LANDMARK_KIND_NAMES[legacy.kind], migrated ? "a mark where it stands" : "NO MARK", game.guide.names.count(legacyKey) ? "STILL THERE" : "gone"));
        // the scanner on the ground (B-417: in the people's land, 5 km off a town of its plan: the old ones' monoliths stood anywhere)
        game.testLandSite();
        { double la = 0, lo = 0; if (haveWorld && planPlacePoint(g, 0, la, lo)) game.testLandCursor(la / DEG, (lo + 5000.0 / (g.R * 1000.0 * std::max(std::cos(la), 0.05))) / DEG); }
        run(0.2); press(KEY_ENTER); run(8.5);
        const bool landed = game.state == GameState::SURFACE;
        press(KEY_R);
        for (int i = 0; i < 30 * 30 && !game.testScanned(); i++) run(1.0 / 30);
        run(0.1);
        const int ruins = game.testEchoes(LM_RUIN), mode = game.testScanMode();
        double ex = 0, ez = 0, er = 0; const bool heard = game.testEchoTrue(ex, ez, er);
        const double px = game.testPlayerX(), pz = game.testPlayerZ(), dist = std::hypot(ex - px, ez - pz);
        const std::string info1 = game.testScanInfo();
        double hx = 0, hz = 0; sscanf(info1.c_str() + std::max<size_t>(0, info1.find("heard at ") == std::string::npos ? 0 : info1.find("heard at ") + 9), "%lf %lf", &hx, &hz);
        const bool close = heard && std::hypot(hx - ex, hz - ez) <= 0.08 * dist + 1;
        writePNG("shots/tests/unit_r408_scan.png", game.output(), FBW, FBH);
        // a mark on it from the map's cursor: the next ruin is the nearest; Shift+R takes the marked in again
        press(KEY_N); run(0.1); game.testMapCursor(ex, ez); run(0.1);
        press(KEY_ENTER); game.testTypeText("FIRST RUIN"); run(0.1);
        writePNG("shots/tests/unit_r408_map.png", game.output(), FBW, FBH);
        double ex2 = 0, ez2 = 0, er2 = 0; const bool next = game.testEchoTrue(ex2, ez2, er2) && std::hypot(ex2 - ex, ez2 - ez) > 300;
        shiftPress(KEY_R); run(0.1);
        double ex3 = 0, ez3 = 0, er3 = 0; const bool back = game.testEchoTrue(ex3, ez3, er3) && std::hypot(ex3 - ex, ez3 - ez) < 1;
        shiftPress(KEY_R); run(0.1);
        int firstMark = -1; for (int i = 0; i < (int)game.guide.marks.size(); i++) if (game.guide.marks[i].name == "FIRST RUIN") firstMark = i;
        const bool placed = firstMark >= 0 && game.guide.marks[firstMark].kind == LM_RUIN;
        // the map: Enter on the mark renames it, Delete twice removes it, M puts the waypoint at the cursor
        press(KEY_ENTER); for (int i = 0; i < 12; i++) press(KEY_BACKSPACE); game.testTypeText("SECOND NAME"); run(0.1);   // the entry holds the old name
        bool renamed = false; for (const SurfaceMark& m : game.guide.marks) if (m.name == "SECOND NAME") renamed = true;
        const size_t before = game.guide.marks.size();
        press(KEY_DELETE); run(0.1); const bool armed = game.guide.marks.size() == before;
        press(KEY_DELETE); run(0.1); const bool removed = game.guide.marks.size() == before - 1;
        press(KEY_M); run(0.1); const bool waypoint = game.testMarksInfo().find("waypoint=1") != std::string::npos;
        press(KEY_ESCAPE); run(0.1);
        // L: a mark at the rangefinder's spot
        game.testSetPitch(-0.25); run(0.3);
        const double rx = game.testRangeX(), rz = game.testRangeZ(), range = game.testRange();
        press(KEY_L); game.testTypeText("LOOKOUT"); run(0.1);
        bool looked = false;
        for (const SurfaceMark& m : game.guide.marks) if (m.name == "LOOKOUT") { double la, lo; game.testLatLonAt(rx, rz, la, lo); looked = std::fabs(la - m.lat) < 1e-7 && std::fabs(lo - m.lon) < 1e-7; }
        check("the scanner hears the ruins, the marked left out (R-408)", landed && mode == LM_RUIN && ruins > 10 && heard && dist < 40000 && close && placed && next && back,
              fmt("%d ruins heard; the nearest %.0f m off, heard %.0f m from it; marked from the map: %s, the next nearest %s, taken in again %s", ruins, dist, std::hypot(hx - ex, hz - ez), placed ? "yes" : "NO", next ? fmt("%.0f m off", std::hypot(ex2 - px, ez2 - pz)).c_str() : "THE SAME", back ? "yes" : "NO"));
        check("the map's cursor renames, removes, waypoints; L marks", renamed && armed && removed && waypoint && range > 0 && looked,
              fmt("renamed %d, armed %d, removed %d, waypoint %d; L at %.0f m: %s", renamed, armed, removed, waypoint, range, looked ? "marked there" : "NOT there"));
        {   // R again: the works, heard in the same scan; a work's mark and a place's kept in the guide's file by R-408's numbers
            press(KEY_R); run(0.1);
            const int wmode = game.testScanMode(), works0 = game.testEchoes(LM_WORK);   // heard at the landing in a people's land
            const bool walked = game.testWalkToWork(1e9);   // to the nearest (the frame re-anchored there if it is far), asked again
            if (walked) for (int i = 0; i < 30 * 40 && game.testEchoes(LM_WORK) == 0; i++) run(1.0 / 30);
            run(0.2);
            const int works = game.testEchoes(LM_WORK);
            double wx = 0, wz = 0, wr = 0; const bool wheard = game.testEchoTrue(wx, wz, wr);
            const double wd = std::hypot(wx - game.testPlayerX(), wz - game.testPlayerZ());
            const std::string winfo = game.testScanInfo();
            writePNG("shots/tests/unit_t03_scan.png", game.output(), FBW, FBH);
            Guide gk; SurfaceMark mw; mw.body = "x"; mw.kind = LM_WORK; mw.name = "A WORK"; gk.marks.push_back(mw); SurfaceMark mp = mw; mp.kind = MARK_PLACE; mp.name = "A PLACE"; gk.marks.push_back(mp);
            gk.save("shots/tests/test_t03_marks.txt");
            std::ifstream f("shots/tests/test_t03_marks.txt"); std::string line, kinds;
            while (std::getline(f, line)) if (line.rfind("mark ", 0) == 0) { std::istringstream is(line); std::string k, b; double la, lo; int kd; is >> k >> b >> la >> lo >> kd; kinds += std::to_string(kd) + " "; }
            Guide gb; gb.load("shots/tests/test_t03_marks.txt");
            const bool kept = gb.marks.size() == 2 && gb.marks[0].kind == LM_WORK && gb.marks[1].kind == MARK_PLACE && kinds == "9 8 ";
            for (int i = 0; i < 12 && game.testScanMode() != LM_RUIN; i++) press(KEY_R);   // round to RUINS again, as the checks after it found it
            check("the scanner's WORKS mode in the game, a work's mark kept (T-03, B-418)", wmode == LM_WORK && works0 > 0 && works > 0 && walked && wheard && wd < wr + 500 && kept,
                  fmt("R after RUINS: mode %d (%s), %d works heard at the landing (B-418: some within 40 km), %d after walking to the nearest%s, its middle %.0f m off (its reach %.0f); %s; the marks written as kinds %s(R-408's place 8, a work 9) and read back %s", wmode, wmode == LM_WORK ? "WORKS" : "not WORKS", works0, works, walked ? "" : " (NOT WALKED)", wd, wr, winfo.c_str(), kinds.c_str(), kept ? "as they were" : "WRONG"));
        }
        {   // the user saw two of their marks' names run into each other over the view (one behind the other on a line of sight):
            // drew every mark within 25 km. Now none is drawn until the explorer shows one from the map (Shift+Enter or
            // Shift+click on it), one at a time, again to hide it; the choice kept through a removal and in the guide. Two marks in a
            // line ahead, 350 m and 1.8 km; the HUD's white counted over the frame (the sun's line left out), against the marks taken away
            while (game.testScanMode() >= 0) press(KEY_R);   // the scanner off (its lines, its mark on the compass)
            const double yaw = 0.7, px0 = game.testPlayerX(), pz0 = game.testPlayerZ();
            const double nx = px0 + std::sin(yaw) * 350, nz = pz0 + std::cos(yaw) * 350, fx = px0 + std::sin(yaw) * 1800, fz = pz0 + std::cos(yaw) * 1800;
            const double bx = px0 - std::cos(yaw) * 600, bz = pz0 + std::sin(yaw) * 600;   // a third, 600 m to the left, placed first
            game.testSetYaw(yaw); game.testSetPitch(-0.05);
            press(KEY_N); run(0.1);   // 8 km wide (the level the map opens on): all three in the frame
            game.testMapCursor(bx, bz); press(KEY_ENTER); game.testTypeText("BEFORE ONE"); run(0.1);
            game.testMapCursor(nx, nz); press(KEY_ENTER); game.testTypeText("NEAR ONE"); run(0.1);
            game.testMapCursor(fx, fz); press(KEY_ENTER); game.testTypeText("FAR ONE"); run(0.1);
            int nearM = -1, farM = -1;
            for (int i = 0; i < (int)game.guide.marks.size(); i++) { if (game.guide.marks[i].name == "NEAR ONE") nearM = i; if (game.guide.marks[i].name == "FAR ONE") farM = i; }
            const bool noneAtFirst = game.testShownMark() == -1;
            press(KEY_ESCAPE); run(0.1);
            double pitch = -0.05; int mlx = -1, mly = -1;   // the near one's diamond to the middle of the view, whatever the ground's lie
            for (int k = 0; k < 60; k++) {
                if (!game.testMarkOnScreen(nearM, mlx, mly)) break;
                if (mly >= 80 && mly <= 120) break;
                pitch += mly < 80 ? 0.01 : -0.01; game.testSetPitch(pitch); run(1.0 / 30);
            }
            const int S = FB_SCALE;
            auto whiteNow = [&]() {   // the HUD's white in the frame, in logical pixels (the sun's line at UH-40 left out)
                const uint32_t* px = game.output(); int n = 0;
                for (int y = 0; y < FBH; y++) { if (y >= (UH - 41) * S && y < (UH - 32) * S) continue; for (int x = 0; x < FBW; x++) if (px[(size_t)y * FBW + x] == HUD_WHITE) n++; }
                return n / (S * S);
            };
            run(0.2); const int none = whiteNow();
            writePNG("shots/tests/unit_b412_none.png", game.output(), FBW, FBH);
            std::vector<SurfaceMark> away; away.swap(game.guide.marks); game.marksVersion++; run(0.2); const int base = whiteNow();
            game.guide.marks.swap(away); game.marksVersion++;
            press(KEY_N); run(0.1);
            game.testMapCursor(nx, nz); shiftPress(KEY_ENTER); run(0.1);
            const bool showNear = game.testShownMark() == nearM && game.state == GameState::SECTOR_MAP && game.testStatus().find("SHOWN OVER THE VIEW: NEAR ONE") != std::string::npos;
            writePNG("shots/tests/unit_b412_map.png", game.output(), FBW, FBH);
            press(KEY_ESCAPE); run(0.2); const int shown = whiteNow();
            writePNG("shots/tests/unit_b412_shown.png", game.output(), FBW, FBH);
            press(KEY_N); run(0.1);   // Shift+click on the far one: it alone; Shift+Enter on it again hides it; on no mark nothing
            game.testMapCursor(fx, fz); gi.down[KEY_LEFT_SHIFT] = true; gi.mousePressed[0] = true; gi.mouseDown[0] = true; game.frame(gi, 1.0 / 30); gi.newFrame(); gi.down[KEY_LEFT_SHIFT] = false; gi.mouseDown[0] = false;
            const bool clickFar = game.testShownMark() == farM && game.state == GameState::SECTOR_MAP;
            game.testMapCursor(fx, fz); shiftPress(KEY_ENTER); run(0.1);
            const bool hideFar = game.testShownMark() == -1 && game.state == GameState::SECTOR_MAP;
            const size_t count = game.guide.marks.size();
            game.testMapCursor(px0 - std::sin(yaw) * 900, pz0 - std::cos(yaw) * 900); shiftPress(KEY_ENTER); run(0.1);
            const bool onNothing = game.testShownMark() == -1 && game.state == GameState::SECTOR_MAP && game.guide.marks.size() == count;
            press(KEY_ESCAPE); run(0.2); const int hidden = whiteNow();
            // the near one shown again; the mark before it in the guide removed: still the near one; through the guide's file; removed
            // itself: none
            press(KEY_N); run(0.1); game.testMapCursor(nx, nz); shiftPress(KEY_ENTER); run(0.1);
            int before = -1; for (int i = 0; i < (int)game.guide.marks.size(); i++) if (game.guide.marks[i].name == "BEFORE ONE") before = i;
            bool keptShown = false, keptFile = false;
            if (before >= 0 && before < game.testShownMark()) {
                game.testMapCursor(bx, bz); press(KEY_DELETE); press(KEY_DELETE); run(0.1);
                keptShown = game.testShownMark() >= 0 && game.guide.marks[game.testShownMark()].name == "NEAR ONE";
                game.guide.save("shots/tests/test_b412_guide.txt"); Guide gb; gb.load("shots/tests/test_b412_guide.txt");
                keptFile = gb.shownMark >= 0 && gb.shownMark < (int)gb.marks.size() && gb.marks[gb.shownMark].name == "NEAR ONE";
                remove("shots/tests/test_b412_guide.txt");
            }
            game.testMapCursor(nx, nz); press(KEY_DELETE); press(KEY_DELETE); run(0.1);
            const bool goneWith = game.testShownMark() == -1;
            game.testMapCursor(fx, fz); press(KEY_DELETE); press(KEY_DELETE); run(0.1);   // the far one away too: the guide's marks as they were
            press(KEY_ESCAPE); run(0.1);
            check("marks: none over the view until one is shown from the map, one at a time (B-412)",
                  nearM >= 0 && farM >= 0 && noneAtFirst && mly >= 80 && mly <= 120 && none <= base + 4 && shown >= base + 60 && showNear && clickFar,
                  fmt("two marks in a line ahead (the near one's diamond at %d,%d): the HUD's white %d px without the marks, %d with them and none shown, %d with the near one shown; Shift+Enter on it %s, Shift+click on the far one %s",
                      mlx, mly, base, none, shown, showNear ? "shows it" : "DOES NOT", clickFar ? "shows it alone" : "DOES NOT"));
            check("marks: the shown one hidden again, kept through a removal and the guide (B-412)",
                  hideFar && onNothing && hidden <= base + 4 && keptShown && keptFile && goneWith,
                  fmt("Shift+Enter on the shown one %s (white %d px), on no mark %s; a mark before it removed: %s, read back from the guide %s; itself removed: %s",
                      hideFar ? "hides it" : "DOES NOT", hidden, onNothing ? "nothing" : "SOMETHING", keptShown ? "still shown" : "LOST", keptFile ? "shown" : "NOT", goneWith ? "none shown" : "STILL SET"));
        }
        {   // the user's landings froze for 5-7 s: the readings (Tab), the scanner (R) or a shown mark asked for the world's marks,
            // whose ground was read with the rivers, and every mark far off built a drainage tile of a degree on the main thread (their
            // nineteen on BUBAS: 8 s). Eight marks far across this world, none of their tiles built: read at once, without the rivers,
            // no tile built for them; one's tiles built, the look two seconds on reads it with them
            const BodyGen& sg = game.testSiteGen();
            std::string wk; for (const SurfaceMark& m : game.guide.marks) if (m.name == "LOOKOUT") wk = m.body;
            const size_t kept = game.guide.marks.size();
            double la0 = 0, lo0 = 0; game.testLatLonAt(game.testPlayerX(), game.testPlayerZ(), la0, lo0);
            std::vector<Vec3> far; int unbuilt0 = 0;
            for (int k = 0; k < 8 && !wk.empty(); k++) {
                SurfaceMark m; m.body = wk; m.name = fmt("FAR %d", k); m.lat = clampd(la0 + (k % 2 ? -1 : 1) * (12 + 5 * k) * DEG, -60 * DEG, 60 * DEG); m.lon = wrapAngle(lo0 + (40 + 35 * k) * DEG);
                game.guide.marks.push_back(m); far.push_back(StarSystem::bodyFromLatLon(m.lat, m.lon));
                if (!drainageReady(sg, far.back(), 0.0)) unbuilt0++;
            }
            const double t0 = nowSec(); int dry = 0; const int count = game.testMarksHere(dry); const double ms = (nowSec() - t0) * 1000;
            int unbuilt = 0; for (const Vec3& u : far) if (!drainageReady(sg, u, 0.0)) unbuilt++;
            bool wetLater = false; double y = 0, wet = 0; int dry2 = -1;
            if (!far.empty()) {
                drainagePrefetch(sg, far[0], 0.0, false);   // the first one's tiles, as the explorer's way would build them
                run(2.2);
                game.testMarksHere(dry2);
                wetLater = game.testMarkGround((int)kept, y, wet) && std::fabs(y - wet) < 1e-6 && dry2 >= 0 && dry2 < dry;
            }
            game.guide.marks.resize(kept); game.marksVersion++;
            check("marks: the far ones read without building a drainage tile, with their rivers once built (B-422)",
                  typeHasDrainage(sg.type) && !wk.empty() && unbuilt0 == 8 && unbuilt == 8 && dry >= 8 && count >= 8 && ms < 250 && wetLater,
                  fmt("%s world; eight marks far across it, their tiles unbuilt %d; the marks read in %.1f ms (%d, %d without their rivers), the tiles still unbuilt %d; the first's tiles built, two seconds on: %s (ground %.2f, with the rivers %.2f, %d dry)",
                      KINDS[sg.type].name, unbuilt0, ms, count, dry, unbuilt, wetLater ? "read with them" : "NOT", y, wet, dry2));
        }
        // the guide: the marks saved and read back; lent to a friend, not lent on
        game.guide.save("shots/tests/test_r408_export.txt");
        Guide back2; back2.load("shots/tests/test_r408_export.txt");
        bool same = back2.marks.size() == game.guide.marks.size();
        for (size_t i = 0; same && i < back2.marks.size(); i++) { const SurfaceMark &a = back2.marks[i], &b = game.guide.marks[i]; same = a.body == b.body && a.name == b.name && a.kind == b.kind && std::fabs(a.lat - b.lat) < 1e-9 && std::fabs(a.lon - b.lon) < 1e-9 && a.lent == b.lent; }
        Guide friendG; int got = 0; friendG.importInbox("shots/tests/test_r408_export.txt", nullptr, nullptr, &got);
        int again = 0; friendG.importInbox("shots/tests/test_r408_export.txt", nullptr, nullptr, &again);
        bool allLent = true; for (const SurfaceMark& m : friendG.marks) if (!m.lent) allLent = false;
        friendG.save("shots/tests/test_r408_friend.txt");
        Guide third; int on = 0; third.importInbox("shots/tests/test_r408_friend.txt", nullptr, nullptr, &on);
        check("the marks go through the guide and lend", same && got == (int)game.guide.marks.size() && again == 0 && allLent && on == 0,
              fmt("%zu marks read back %s; lent %d (again %d), all cyan %d; lent on %d", back2.marks.size(), same ? "the same" : "DIFFERENT", got, again, allLent, on));
        remove("shots/tests/test_r408_guide.txt"); remove("shots/tests/test_r408_export.txt"); remove("shots/tests/test_r408_friend.txt");
    }
}

// R-419, R-420: auto-explore and its tape
void unitAuto(const Check& check) {
    {   // auto-explore from the home parking: the menu's second row opens the chooser on the profile chosen last, Enter begins it; it walks
        // to the front window and looks at the world, the eye never turning faster than a hand's limits (the head's spring 43 degrees a second,
        // the ship's nose 26: under 60 together); a key gives the controls back and is not acted on (G opens no guide)
        TestGame game;
        game.savePrefix = "shots/tests/unit_auto_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_auto_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.settings.autoProfile = AP_PLANETFALL; game.settings.renderScale = 1; game.applySettings();
        game.setState(GameState::SPACE);
        Input in;
        auto frame = [&]() { game.frame(in, 1.0 / 30); in.newFrame(); };
        auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; frame(); in.down[key] = false; };
        press(KEY_ESCAPE); press(KEY_DOWN); press(KEY_ENTER);
        const bool chooser = game.state == GameState::AUTO && game.testAutoSel() == AP_PLANETFALL;
        press(KEY_ENTER);
        const bool began = game.autoOn() && game.state == GameState::SPACE;
        double maxTurn = 0; Vec3 prev = game.testViewDir(); std::set<std::string> steps; int frames = 0;
        for (int i = 0; i < 20 * 30 && game.autoOn(); i++) {
            frame(); frames++;
            const Vec3 v = game.testViewDir();
            maxTurn = std::max(maxTurn, std::acos(clampd(dot(v, prev), -1, 1)) / DEG * 30);
            prev = v;
            steps.insert(game.testAutoDoing());
        }
        const bool window = steps.count("to the window") && steps.count("the world");
        const bool on = game.autoOn();
        press(KEY_G);
        const bool back = !game.autoOn() && game.state == GameState::SPACE;
        check("auto-explore: begun from the menu, eased, given back (R-419)", chooser && began && on && window && maxTurn < 60 && back,
              fmt("chooser %d on %d, %d frames, steps %zu (window %d), the fastest turn %.0f deg/s; G: auto %d, state %d", (int)chooser, (int)began, frames, steps.size(), (int)window, maxTurn, (int)game.autoOn(), (int)game.state));
    }
    {   // auto-explore's tape. M begins it: the deck puts a cassette in and plays it, the track starting as the sequence moves the tape,
        // its author and title fading in at the top left and gone after a few seconds, the game's own sounds ducked; at a track's end the next
        // cassette, round to the first; M again the stop key (the place kept), once more the play key from there; the controller's SELECT the
        // same; a key the end, the music fading with auto-explore, taken up again as it begins. A file listed but gone is left out, one that
        // will not play passed over; an empty folder is said
        const std::string dir = tapeTestDir();
        TestGame game;
        game.savePrefix = "shots/tests/unit_tape_save"; game.settingsPath = "shots/tests/unit_tape_settings.txt"; game.guidePath = "shots/tests/unit_tape_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.settings.renderScale = 1; game.settings.autoMusic = false; game.settings.autoTrack = 0; game.settings.autoTrackAt = -1; game.applySettings();
        game.musicDir = dir;
        game.setState(GameState::SPACE);
        game.autoStart(AP_PLANETFALL, 7);
        TapePump pump;
        Input in; const double dt = 1.0 / 30; double clock = 0;
        auto frame = [&]() { game.frame(in, dt); in.newFrame(); pump.step(game.audio, dt); clock += dt; };
        auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; frame(); in.down[key] = false; };
        auto until = [&](auto done, double secs) { for (double s = 0; s < secs && !done(); s += dt) frame(); return (bool)done(); };
        auto wait = [&](double secs) { for (double s = 0; s < secs; s += dt) frame(); };
        const size_t listed = game.testTapeTracks();
        const bool hint = !game.tapeOn() && listed == 2;
        std::string au, ti;
        press(KEY_M);   // a cassette in
        const bool inserted = pump.decks[DECK_INSERT] == 1 && game.tapeOn() && game.autoOn();
        const double atInsert = deckTapeAt(DECK_INSERT, pump.lastSeed), m0 = clock;
        const bool started1 = until([&]() { return pump.playing; }, 5);
        const double lead1 = clock - m0;
        const bool first = pump.file == dir + "/one.mp3" && std::fabs(game.audio.musicGain - 0.8 * std::pow(10.0, -1.1 / 20)) < 1e-3;
        frame(); const double capIn = game.tapeCaption(au, ti);
        wait(1.0);
        const double capWhole = game.tapeCaption(au, ti), duck = game.audio.duck;
        const bool credit1 = au == "Jarrad Wright" && ti == "Forwards";
        wait(6.5);
        const double capGone = game.tapeCaption(au, ti);
        const bool changed = until([&]() { return pump.decks[DECK_CHANGE] == 1; }, 5);   // its end: the next cassette
        const double atChange = deckTapeAt(DECK_CHANGE, pump.lastSeed), c0 = clock;
        const bool started2 = until([&]() { return pump.playing; }, 6);
        const double lead2 = clock - c0;
        frame(); game.tapeCaption(au, ti);
        const bool second = pump.file == dir + "/two.mp3" && au == "Black Sabbath" && ti == "Planet Caravan" && std::fabs(game.audio.musicGain - 0.8 * std::pow(10.0, 10.5 / 20)) < 1e-3;
        const bool round = until([&]() { return pump.decks[DECK_CHANGE] == 2; }, 12) && until([&]() { return pump.playing; }, 6) && pump.file == dir + "/one.mp3";
        wait(4.0);   // the stop key four seconds in: the music cut, the place kept
        const double placeAt = game.audio.musicT;
        press(KEY_M);
        const bool stopKey = pump.decks[DECK_STOP] == 1 && game.autoOn();
        const bool cut = until([&]() { return !pump.playing; }, 0.5) && !game.settings.autoMusic && std::fabs(game.settings.autoTrackAt - placeAt) < 0.1;
        const double keptAt = game.settings.autoTrackAt;
        wait(2.0);
        const double duckOff = game.audio.duck;
        press(KEY_M);   // the play key: from there
        const bool resumed = pump.decks[DECK_PLAY] == 1 && until([&]() { return pump.playing; }, 2) && std::fabs(pump.from - placeAt) < 0.1 && game.settings.autoMusic;
        wait(1.0);
        in.pad.present = in.pad.mapped = true; frame();   // a controller arrives (what it holds presses nothing), its SELECT pressed
        in.pad.buttons[PB_BACK] = true; frame(); in.pad.buttons[PB_BACK] = false; frame();
        const bool select = pump.decks[DECK_STOP] == 2 && game.autoOn();
        in.pad = PadState(); frame();
        press(KEY_M); until([&]() { return pump.playing; }, 2);
        wait(1.0);
        press(KEY_G);   // the controls back: the music fades, the choice kept
        const bool ended = !game.autoOn() && game.settings.autoMusic;
        const double g1 = game.audio.musicGain; wait(0.5); const double g2 = game.audio.musicGain;
        const bool faded = g1 > 0 && g2 < g1 && until([&]() { return !pump.playing; }, 2) && game.audio.duck == 1;
        const double leftAt = game.settings.autoTrackAt;
        game.autoStart(AP_PLANETFALL, 8); frame();   // auto-explore again: the play key, from where it was left
        const bool again = pump.decks[DECK_PLAY] == 3 && until([&]() { return pump.playing; }, 2) && std::fabs(pump.from - leftAt) < 0.1;
        remove((dir + "/two.mp3").c_str());   // the next file gone: passed over, round to the first
        const int changes = pump.decks[DECK_CHANGE];
        const bool passed = until([&]() { return pump.decks[DECK_CHANGE] == changes + 2; }, 20) && until([&]() { return pump.playing; }, 6) && pump.file == dir + "/one.mp3";
        press(KEY_M); wait(0.5);
        game.musicDir = dir + "/empty"; press(KEY_M);   // nothing to play: said
        const double capEmpty = game.tapeCaption(au, ti) + (frame(), game.tapeCaption(au, ti));
        const bool empty = !game.tapeOn() && game.autoOn() && capEmpty > 0 && au.find("NO MUSIC") == 0;
        check("auto-explore's tape: M, the deck, credits, ducked, kept (R-420)",
              hint && inserted && started1 && std::fabs(lead1 - atInsert) < 0.1 && first && capIn > 0 && capIn < 0.5 && capWhole == 1 && credit1 && std::fabs(duck - 0.35) < 1e-9 && capGone == 0 &&
                  changed && started2 && std::fabs(lead2 - atChange) < 0.1 && second && round && stopKey && cut && duckOff == 1 && resumed && select && ended && faded && again && passed && empty,
              fmt("%zu listed; M: a cassette in %d, the track after %.2f s (the deck's %.2f) %d, its level %d, caption %.2f / %.2f / %.2f %d, duck %.2f; next %d after %.2f s (%.2f) %d, round %d; "
                  "stop key %d cut %d (place %.1f s, kept %.1f), duck after %.2f; play key %d; SELECT %d; G: off %d, faded %d (%.3f to %.3f), back %d; a file gone passed %d; empty folder %d",
                  listed, (int)inserted, lead1, atInsert, (int)started1, (int)first, capIn, capWhole, capGone, (int)credit1, duck, (int)changed, lead2, atChange, (int)second, (int)round,
                  (int)stopKey, (int)cut, placeAt, keptAt, duckOff, (int)resumed, (int)select, (int)ended, (int)faded, g1, g2, (int)again, (int)passed, (int)empty));
    }
    {   // the deck's sequences: each kind's parts at their times (onsets counted), the tape moving where it should (the play key's ka-chunk
        // just before it, the hiss after it), never near clipping, alike for a seed and not for two; through the synth over the ducked rest,
        // the deck itself not ducked; -1 hushes it
        const int sr = 22050;
        static const char* const KN[DECK_KINDS] = {"", "insert", "change", "play", "stop"};
        const double atLo[DECK_KINDS] = {0, 1.5, 2.6, 0.4, 0.03}, atHi[DECK_KINDS] = {0, 2.8, 4.2, 0.7, 0.03}, onsetsMin[DECK_KINDS] = {0, 6, 9, 2, 2};
        bool ok = true; std::string det;
        for (int kind = DECK_INSERT; kind < DECK_KINDS; kind++) {
            const std::vector<float> a = deckRender(kind, 11, sr), b = deckRender(kind, 11, sr), c = deckRender(kind, 12, sr);
            double peak = 0; bool finite = true;
            for (float v : a) { finite = finite && std::isfinite(v); peak = std::max(peak, (double)std::fabs(v)); }
            const double at = deckTapeAt(kind, 11), len = deckLength(kind, 11);
            auto rms = [&](double t0, double t1) { const long i0 = std::max(0L, (long)(t0 * sr)), i1 = std::min((long)a.size(), (long)(t1 * sr)); double s = 0; for (long i = i0; i < i1; i++) s += (double)a[i] * a[i]; return i1 > i0 ? std::sqrt(s / (i1 - i0)) : 0.0; };
            int onsets = 0; double prevE = 0;   // 5 ms frames whose energy jumps fourfold over a floor
            for (size_t i = 0; i + 110 <= a.size(); i += 110) { double e = 0; for (size_t j = i; j < i + 110; j++) e += (double)a[j] * a[j]; e /= 110; if (e > 4 * prevE + 1e-5 && e > 2e-4) onsets++; prevE = e; }
            const bool timeOk = at >= atLo[kind] - 1e-9 && at <= atHi[kind] + 1e-9 && a.size() == (size_t)std::ceil(len * sr) + 1;
            const double knock = kind == DECK_STOP ? rms(0, 0.08) : rms(at - 0.5, at - 0.3), after = kind == DECK_STOP ? 0 : rms(at + 0.05, at + 0.4);
            const bool soundOk = kind == DECK_STOP ? knock > 0.02 : knock > 0.02 && after > 0.004 && after < knock;
            const bool kOk = finite && peak < 0.9 && a == b && a != c && timeOk && soundOk && onsets >= onsetsMin[kind];
            ok = ok && kOk;
            det += fmt("%s%s %.2f s of %.2f, peak %.2f, %d onsets, knock %.3f, hiss %.4f%s", det.empty() ? "" : "; ", KN[kind], at, len, peak, onsets, knock, after, kOk ? "" : " (wrong)");
        }
        std::vector<float> buf(2048), buf2(2048);   // the same hum through two synths, one ducked, block for block
        auto bufRms = [](const std::vector<float>& v) { double s = 0; for (float x : v) s += (double)x * x; return std::sqrt(s / v.size()); };
        AudioSynth plainS, duckS; AudioState plainSt, duckSt; plainSt.hum = duckSt.hum = 0.6; duckSt.duck = 0.35;
        for (int i = 0; i < 40; i++) { plainS.render(buf.data(), (int)buf.size(), sr, plainSt); duckS.render(buf2.data(), (int)buf2.size(), sr, duckSt); }
        const double plain = bufRms(buf), ducked = bufRms(buf2);
        AudioSynth quiet; AudioState q; q.duck = 0.35; q.deck = DECK_STOP; q.deckSeed = 5;
        quiet.render(buf.data(), (int)buf.size(), sr, q);
        const std::vector<float> ref = deckRender(DECK_STOP, 5, sr);
        double diff = 0; for (size_t i = 0; i < buf.size(); i++) diff = std::max(diff, std::fabs(buf[i] - ref[i] * q.master));
        q.deck = -1; quiet.render(buf.data(), (int)buf.size(), sr, q);
        const double hushed = bufRms(buf);
        const bool mixOk = std::fabs(ducked / plain - 0.35) < 0.005 && diff < 1e-5 && hushed == 0;
        check("auto-explore's tape: the cassette deck (R-420)", ok && mixOk, det + fmt("; the hum ducked to %.3f of itself, the deck through the synth within %.1e, hushed %.4f", ducked / plain, diff, hushed));
    }
    {   // where the tape is read: the explorer's `music` folder when it holds a track, else the game's own beside the executable (in a
        // macOS app's Resources from its executable, the app's from a bare binary beside it); a folder's playlist first, then the sound
        // files it does not list, by name
        const std::string own = tapeTestDir(), app = "shots/tests/unit_tape_app", res = app + "/Vesperis.app/Contents/Resources/music";
        for (const std::string& d : {app, app + "/empty", app + "/Vesperis.app", app + "/Vesperis.app/Contents", app + "/Vesperis.app/Contents/MacOS", app + "/Vesperis.app/Contents/Resources", res}) makeDir(d);
        for (const char* f : {"a.mp3", "Someone - Extra.mp3", "notes.txt"}) if (FILE* fp = fopen((res + "/" + f).c_str(), "wb")) { fputs("not sound: the harness lists it\n", fp); fclose(fp); }
        if (FILE* fp = fopen((res + "/playlist.txt").c_str(), "w")) { fputs("a.mp3 | Jarrad Wright | Forwards | -1.1\n", fp); fclose(fp); }
        TestGame game;
        game.settingsPath = "shots/tests/unit_tape_settings.txt"; game.guidePath = "shots/tests/unit_tape_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
        game.musicDir = app + "/empty";
        game.tapeGameDirs(app + "/Vesperis.app/Contents/MacOS/");   // the app's own executable
        const size_t inApp = game.testTapeTracks(); const std::string appDir = game.testTapeDir(), a0 = game.testTapeTrack(0), a1 = game.testTapeTrack(1);
        game.tapeGameDirs(app + "/");                              // a bare binary beside the app
        const size_t beside = game.testTapeTracks(); const std::string besideDir = game.testTapeDir();
        game.musicDir = own;                                       // the explorer's folder holds tracks: its tape plays
        const size_t mine = game.testTapeTracks(); const std::string mineDir = game.testTapeDir();
        game.musicDir = app + "/empty"; game.tapeGameDirs("");
        const size_t none = game.testTapeTracks();
        const bool ok = inApp == 2 && appDir == app + "/Vesperis.app/Contents/MacOS/../Resources/music" && a0 == "a.mp3 | Jarrad Wright | Forwards | -1.1 dB" &&
                        a1 == "Someone - Extra.mp3 | Someone | Extra | +0.0 dB" && beside == 2 && besideDir == res && mine == 2 && mineDir == own && none == 0;
        check("auto-explore's tape: the explorer's folder, else the game's own (R-420)", ok,
              fmt("the app's executable: %zu tracks from %s (%s; %s); a bare binary beside it: %zu from %s; the explorer's folder with tracks: %zu from %s; none anywhere: %zu",
                  inApp, appDir.c_str(), a0.c_str(), a1.c_str(), beside, besideDir.c_str(), mine, mineDir.c_str(), none));
    }
}

// E-06: the states' table (game_types.h) holds, for its first 29 states, what the hand-kept lists it replaced held: `frame`'s
// `simulating` and its stowing of the radar camera and the telescope, the render switch's overlays, the dark radio's `taken`, the
// save's `inPlay` and its ground, `mouseCaptureWanted`, the photo tools' scenes, the typing states (the global keys, the controller's
// keyboard), `padContext`'s contexts, and the names the harness printed (modes_game.cpp's `auto`)
void unitStates(const Check& check) {
    using S = GameState;
    auto in = [](S s, std::initializer_list<S> l) { return std::find(l.begin(), l.end(), s) != l.end(); };
    static const char* const NAMES[29] = {"TITLE", "SPACE", "LANDING_MAP", "DESCENT", "SURFACE", "ASCENT", "HELP", "MENU", "SYSTEM_LIST", "DATA",
                                    "SETTINGS", "SLOTS", "GUIDE", "TEXT_ENTRY", "STAR_MAP", "LOG", "STATS", "GALLERY", "SHIPSCREEN", "CONSOLE",
                                    "SECTOR_MAP", "KEYS", "SHARDS", "ALMANAC", "PROBE", "RECORDING", "INSTRUMENTS", "DRONES", "AUTO"};
    std::string bad;
    for (int i = 0; i < 29; i++) {
        const S s = (S)i;
        const struct { unsigned trait; const char* what; bool was; } T[] = {
            {ST_SIMULATES, "simulating", in(s, {S::SPACE, S::SURFACE, S::DESCENT, S::ASCENT, S::LANDING_MAP, S::SHIPSCREEN, S::CONSOLE, S::SHARDS,
                                                S::PROBE, S::INSTRUMENTS, S::DRONES})},
            {ST_OVERLAY, "an overlay", in(s, {S::HELP, S::MENU, S::SETTINGS, S::SLOTS, S::GUIDE, S::TEXT_ENTRY, S::STAR_MAP, S::LOG, S::STATS, S::GALLERY,
                                              S::SHIPSCREEN, S::CONSOLE, S::SECTOR_MAP, S::SYSTEM_LIST, S::DATA, S::KEYS, S::SHARDS, S::INSTRUMENTS,
                                              S::DRONES, S::AUTO})},
            {ST_STOWS, "stowing", in(s, {S::SURFACE, S::DESCENT, S::ASCENT, S::LANDING_MAP, S::TITLE, S::SHARDS, S::SECTOR_MAP, S::PROBE, S::INSTRUMENTS,
                                         S::DRONES})},
            {ST_TAKES_SYNTH, "taking the synth", in(s, {S::TITLE, S::LANDING_MAP, S::DESCENT, S::ASCENT, S::SHARDS, S::INSTRUMENTS, S::PROBE, S::RECORDING,
                                                        S::DRONES})},
            {ST_IN_PLAY, "in play", in(s, {S::SPACE, S::LANDING_MAP, S::SURFACE, S::DESCENT, S::ASCENT, S::PROBE})},
            {ST_MOUSE, "the mouse's", in(s, {S::SPACE, S::SURFACE, S::DESCENT, S::ASCENT, S::STAR_MAP, S::SECTOR_MAP})},
            {ST_PHOTO, "a photo's", in(s, {S::SPACE, S::SURFACE, S::PROBE, S::RECORDING})},
            {ST_TYPING, "typing", in(s, {S::TEXT_ENTRY, S::CONSOLE})},
            {ST_GROUND, "the ground's", in(s, {S::SURFACE, S::DESCENT, S::ASCENT})},
        };
        for (const auto& t : T)
            if (stateHas(s, t.trait) != t.was && bad.empty()) bad = fmt("%s %s %s", NAMES[i], t.was ? "is no longer" : "is now", t.what);
        const int pad = s == S::TITLE ? PCX_TITLE : s == S::SPACE ? PCX_SHIP : s == S::SURFACE ? PCX_FOOT : s == S::PROBE ? PCX_PROBE :
                        s == S::RECORDING ? PCX_RECORDING : s == S::INSTRUMENTS ? PCX_INSTRUMENT : PCX_SCREEN;
        if (statePad(s) != pad && bad.empty()) bad = fmt("%s's controller context", NAMES[i]);
        if (strcmp(stateName(s), NAMES[i]) != 0 && bad.empty()) bad = fmt("%s's name", NAMES[i]);
    }
    check("the states' table, the old lists", bad.empty(), bad.empty() ? "29 states: 9 traits, the controller's contexts, the names" : bad);
}
