// E-03: the unit's proofs that the frames are the same by any thread count (group `perf`)
#include "harness/harness.h"
#include "harness/test_worlds.h"
#include "harness/scenes_table.h"

// the frames by any thread count, bit for bit
void unitPerf(const Check& check) {
    {   // the frames the regress never hashes, drawn by the workers and by one thread in one run (B-310's contract, A-02's template),
        // at 4x: the shore through the surface renderer, and through the game the cabin (the light on and off, the hull glass, the roof, the
        // drones out at the rack and outside the window), the radar camera, the landing map whole and zoomed, the ground, the nose camera and
        // the sector map. A game's frame is drawn at one instant (`frame(in, 0)`): the workers' twice (a frame that changes by itself says so),
        // then one thread's. B-423: the forest at 1x, 2x and 4x by the workers, by one thread and by three (fewer threads than its bands)
        const int saved = g_testScale; setFramebufferScale(4);
        int frames = 0, same = 0; std::string bad;
        auto note = [&](const char* what, uint64_t a, uint64_t a2, uint64_t s) {
            frames++;
            if (a == a2 && a == s) same++;
            else bad += fmt("%s%s %s", bad.empty() ? "" : "; ", what, a != a2 ? "not the same twice" : "DIFFERS with one thread");
        };
        for (int sc : {1, 2, 4}) {   // the forest (the `felisian_forest` scene)
            setFramebufferScale(sc);
            SpaceRenderer srF; StarNeighborhood nbF; StarSystem sysF; SurfaceView svF; double tF = 0;
            if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, svF, sysF, nbF, tF, SCENE_FOREST)) {
                Framebuffer fbF;
                auto draw = [&]() { svF.render(fbF, tF, nbF.stars, srF, 1.0); fbF.mush(2); return fnv(1469598103934665603ULL, fbF.idx.data(), fbF.idx.size() * sizeof(Pix)); };
                draw(); const uint64_t a = draw(), a2 = draw();
                parallelThreadsOverride() = 1; const uint64_t s1 = draw();
                parallelThreadsOverride() = 3; const uint64_t s3 = draw(); parallelThreadsOverride() = 0;
                note(fmt("the forest at %dx", sc).c_str(), a, a2, s1);
                if (s3 != a) { frames++; bad += fmt("%sthe forest at %dx DIFFERS with three threads", bad.empty() ? "" : "; ", sc); } else { frames++; same++; }
            } else bad += fmt("%sno forest at %dx", bad.empty() ? "" : "; ", sc);
        }
        setFramebufferScale(4);
        {
            SpaceRenderer srS; StarNeighborhood nbS; StarSystem sysS; SurfaceView svS; double tS = 0;
            if (setupSceneForType(PT_FELISIAN, 12, 30 * DEG, 0.0, 0.0, svS, sysS, nbS, tS, MAT_SAND)) {   // the felisian_shore scene
                Framebuffer fbS;
                auto draw = [&]() { svS.render(fbS, tS, nbS.stars, srS, 1.0); fbS.mush(2); return fnv(1469598103934665603ULL, fbS.idx.data(), fbS.idx.size() * sizeof(Pix)); };
                draw(); const uint64_t a = draw(), a2 = draw();
                parallelThreadsOverride() = 1; const uint64_t s = draw(); parallelThreadsOverride() = 0;
                note("the shore", a, a2, s);
            }
        }
        {
            TestGame game;
            game.savePrefix = "shots/tests/test_save";
            game.settingsPath = "shots/tests/test_settings.txt";
            game.keysPath = "shots/tests/test_keys.txt";
            game.guidePath = "shots/tests/test_guide_bothways.txt";
            remove(game.guidePath.c_str());
            game.newGame(); game.setState(GameState::SPACE);
            game.settings.renderScale = 4; game.applySettings();
            Input gi;
            auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
            auto press = [&](int key) { pressKey(game, gi, key); };
            auto out = [&]() { return fnv(1469598103934665603ULL, game.output(), (size_t)FBW * FBH * sizeof(uint32_t)); };
            auto both = [&](const char* what) {
                game.frame(gi, 0); const uint64_t a = out();
                game.frame(gi, 0); const uint64_t a2 = out();
                parallelThreadsOverride() = 1; game.frame(gi, 0); const uint64_t s = out(); parallelThreadsOverride() = 0;
                note(what, a, a2, s);
            };
            const AgeWorld& W = AGE_WORLDS[AGE_MODERN];
            Star st; StarSystem wsys;
            if (starInSector(W.sx, 0, W.sz, st, true)) {
                wsys.generate(st);
                run(0.2); game.testParkAt(st, W.body); run(0.5);
                both("the landing map");
                if (game.testLandableBody() >= 0) { press(KEY_Z); for (int i = 0; i < 90 && !game.testDrainageReady(); i++) run(0.1); run(0.3); both("the landing map's zoom"); press(KEY_Z); }
                game.setState(GameState::SPACE); run(0.3);
                const int lights = game.testCabinLights();
                if (!(lights & 1)) press(KEY_U);   // the light on, the hull polarised
                if (lights & 2) press(KEY_Y);
                run(1.5); both("the cabin, the light on");
                press(KEY_U); run(1.5); both("the cabin, the light off");
                press(KEY_Y); run(1.5); both("the cabin, the hull glass, the light off");
                press(KEY_U); run(1.5); both("the cabin, the hull glass, the light on");
                press(KEY_Y); run(0.5);
                press(KEY_PAGE_UP); run(0.5); both("the roof"); press(KEY_PAGE_DOWN); run(0.3);
                press(KEY_B); run(0.5); both("the radar camera"); press(KEY_B); run(0.2);
                game.testCabinGoto(-0.3, 1.435, -PI / 2, -0.38); run(0.2);   // the drones' rack (C-18's harness)
                press(KEY_E); run(0.1);
                if (game.state == GameState::DRONES) {
                    game.testDronesSettle(); run(0.2);
                    press(KEY_ENTER); game.testDronesSettle();
                    game.setState(GameState::SPACE); game.testCabinGoto(-0.3, 1.435, -PI / 2, -0.38); run(1.1);
                    both("the rack, the drones out");
                    game.testAimAtBody(W.body); game.testCabinGoto(-0.6, 0.6, -0.25, -0.05); run(0.05); game.testDronesSkip(4.0); run(0.05);
                    both("the drones outside the window");
                } else bad += fmt("%sthe drone bay not opened (state %d)", bad.empty() ? "" : "; ", (int)game.state);
            }
            {   // the ground of the home world: the landing (C, Enter), the explorer, the nose camera, the sector map
                Star hs;
                if (starInSector(HOME_SX, HOME_SY, HOME_SZ, hs, true)) {
                    StarSystem hsys; hsys.generate(hs);
                    int hb = -1; for (const Body& b : hsys.bodies) if (KINDS[b.type].landable && b.parent < 0 && b.type != PT_COMET) { hb = b.index; break; }
                    game.testParkAt(hs, hb); run(0.3);
                    for (int i = 0; i < 90 && !game.testDrainageReady(); i++) run(0.1);
                    press(KEY_ENTER); run(9.0);
                    if (game.state == GameState::SURFACE) {
                        both("the ground");
                        if (game.testDriveSetup()) { run(0.5); both("the nose camera"); }
                        press(KEY_N); run(0.3); both("the sector map"); press(KEY_ESCAPE); run(0.1);
                    } else bad += fmt("%snot on the ground (state %d)", bad.empty() ? "" : "; ", (int)game.state);
                }
            }
        }
        setFramebufferScale(saved);
        check("threads: the frames the regress skips (F-01, B-423)", frames >= 20 && same == frames,
                fmt("%d frames (the forest at 1x, 2x and 4x also with three threads; the rest at 4x) the same with %d threads and with one: %d%s%s", frames, parallelThreads(), same,
                        bad.empty() ? "" : "; ", bad.c_str()));
    }
    {   // the picture is the thread count's no more: six scenes at 4x (the forest's flora and terrain, the shore's water and reflections,
        // a town's forms, the aurora's grid, the protostar's strip, the black hole's lens) drawn with one thread and with two, four, six and
        // eight, the rows dealt to whichever thread is free
        const int saved = g_testScale; setFramebufferScale(4);
        int scenes = 0, same = 0; std::string bad;
        for (const char* name : {"felisian_forest", "felisian_shore", "civilisation_modern", "thinatmo_aurora", "protostar_night", "black_hole_sky"}) {
            for (const CmpScene& sc : CMP_SCENES) {
                if (std::string(name) != sc.name) continue;
                SpaceRenderer srT; StarNeighborhood nbT; StarSystem sysT; SurfaceView svT; double tT = 0;
                if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, svT, sysT, nbT, tT, sc.wantMat)) { bad += fmt("%s%s: no body", bad.empty() ? "" : "; ", name); break; }
                Framebuffer fbT;
                auto draw = [&](int threads) {
                    parallelThreadsOverride() = threads;
                    svT.render(fbT, tT, nbT.stars, srT, 1.0);
                    fbT.mush(2);
                    parallelThreadsOverride() = 0;
                    return fnv(1469598103934665603ULL, fbT.idx.data(), fbT.idx.size() * sizeof(Pix));
                };
                draw(8);
                const uint64_t one = draw(1);
                std::string off;
                for (int threads : {2, 4, 6, 8}) if (draw(threads) != one) off += fmt("%s%d", off.empty() ? "" : ", ", threads);
                scenes++;
                if (off.empty()) same++; else bad += fmt("%s%s DIFFERS with %s threads", bad.empty() ? "" : "; ", name, off.c_str());
                break;
            }
        }
        setFramebufferScale(saved);
        const long contended = pool::contended();   // a dispatch from a thread beside the main one's (it runs alone: none wanted)
        check("threads: the picture whatever the count (F-06)", scenes == 6 && same == scenes && contended == 0,
                fmt("%d scenes at 4x the same with one thread and with two, four, six and eight: %d%s%s; %ld dispatches from another thread in this run", scenes, same, bad.empty() ? "" : "; ",
                        bad.c_str(), contended));
    }
    {   // the probes' lines in the guide made once: a guide saved, loaded and saved again is the same file byte for byte (the second
        // save writes the load's lines), so is a third, and a friend's import carries the lines of the file it read
        Guide G; fillBenchGuide(G);
        const std::string a = "shots/tests/f02_guide_a.txt", b = "shots/tests/f02_guide_b.txt", c = "shots/tests/f02_guide_c.txt";
        G.save(a);
        Guide L; const bool loaded = L.load(a); L.save(b);
        Guide L2; L2.load(b); L2.save(c);
        auto bytes = [](const std::string& path) { std::ifstream f(path, std::ios::binary); return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); };
        const std::string A = bytes(a), B = bytes(b), C = bytes(c);
        bool cached = !L.probes.empty();
        for (const ProbeRecord& r : L.probes) cached = cached && !r.lines.empty() && r.lines == probeLines(r);
        Guide F; int lentN = 0, probesN = 0; F.importInbox(a, &lentN, &probesN);
        bool imported = probesN == (int)G.probes.size();
        for (const ProbeRecord& r : F.probes) imported = imported && r.lent && r.lines == probeLines(r);
        double saveMs = 1e9; for (int i = 0; i < 5; i++) { const double t0 = nowSec(); L.save(b); saveMs = std::min(saveMs, (nowSec() - t0) * 1000); }
        check("guide: the probes' lines made once (F-02)", loaded && A.size() > 600000 && A == B && B == C && cached && imported,
              fmt("%zu bytes: saved, loaded and saved again %s, once more %s; the loaded lines the records' %s; an import's %s; a save %.1f ms", A.size(), A == B ? "the same" : "DIFFERENT",
                  B == C ? "the same" : "DIFFERENT", cached ? "yes" : "NO", imported ? "carried" : "NOT CARRIED", saveMs));
        remove(a.c_str()); remove(b.c_str()); remove(c.c_str());
    }
    {   // the PNG writer's thread writes the call's bytes; a picture of noise (every byte in every place, 47 stored blocks at 4x's size)
        // read back as drawn; the IEND chunk's CRC the one every PNG has (0xAE426082); a run of six behind a full queue keeps its order
        bool ok = true; std::string det;
        const int sizes[3][2] = {{1280, 800}, {320, 200}, {7, 3}};
        for (const auto& wh : sizes) {
            const int w = wh[0], h = wh[1];
            std::vector<uint32_t> px((size_t)w * h);
            for (size_t i = 0; i < px.size(); i++) px[i] = 0xFF000000u | (uint32_t)(mix64(i * 7919 + w) & 0xFFFFFF);
            const std::string sp = fmt("shots/tests/f02_png_%dx%d_call.png", w, h), ap = fmt("shots/tests/f02_png_%dx%d_thread.png", w, h);
            writePNG(sp.c_str(), px.data(), w, h);
            writePNGAsync(ap.c_str(), px.data(), w, h);
            pngFlush();
            std::ifstream fs(sp, std::ios::binary), fa(ap, std::ios::binary);
            const std::string S((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>()), Aa((std::istreambuf_iterator<char>(fa)), std::istreambuf_iterator<char>());
            std::vector<uint32_t> back; int bw = 0, bh = 0; const bool read = readPNG(ap.c_str(), back, bw, bh);
            const bool iend = S.size() > 12 && S.compare(S.size() - 8, 4, "IEND") == 0 && (uint8_t)S[S.size() - 4] == 0xAE && (uint8_t)S[S.size() - 3] == 0x42 && (uint8_t)S[S.size() - 2] == 0x60 &&
                    (uint8_t)S[S.size() - 1] == 0x82;
            const bool same = S == Aa && read && bw == w && bh == h && back == px && iend;
            ok = ok && same;
            det += fmt("%s%dx%d %zu bytes %s", det.empty() ? "" : ", ", w, h, S.size(), same ? "the same" : (S != Aa ? "THE THREAD'S DIFFER" : (!iend ? "IEND'S CRC WRONG" : "NOT READ BACK")));
            remove(sp.c_str()); remove(ap.c_str());
        }
        {   // six pictures queued behind each other: each file the picture asked for under its name
            std::vector<uint32_t> px(320 * 200);
            for (int k = 0; k < 6; k++) {
                for (size_t i = 0; i < px.size(); i++) px[i] = 0xFF000000u | (uint32_t)((k * 40) << 8 | (i & 255));
                writePNGAsync(fmt("shots/tests/f02_png_run_%d.png", k).c_str(), px.data(), 320, 200);
            }
            bool order = true;
            for (int k = 0; k < 6; k++) {
                std::vector<uint32_t> back; int bw = 0, bh = 0;
                order = order && readPNG(fmt("shots/tests/f02_png_run_%d.png", k).c_str(), back, bw, bh) && bw == 320 && ((back[5] >> 8) & 255) == (uint32_t)(k * 40);
                remove(fmt("shots/tests/f02_png_run_%d.png", k).c_str());
            }
            ok = ok && order; det += order ? ", a run of six in order" : ", A RUN OF SIX OUT OF ORDER";
        }
        check("png: the writer's thread writes the call's bytes (F-02)", ok, det);
    }
    {   // the star neighbourhood kept across moves: the stars of the sectors the new cube shares with the last taken from it, in the order a
        // fresh walk finds them; fifty steps of a flight coreward (a fraction of a sector to a few, up and down), a jump far off and back
        StarNeighborhood inc; int steps = 0, same = 0; size_t most = 0; double incMs = 0, freshMs = 0;
        const Vec3 home((HOME_SX + 0.5) * SECTOR_KM, 0.5 * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
        const Vec3 core = normalize(Vec3(GALAXY_CENTRE_SX - HOME_SX, 0.07 * (GALAXY_CENTRE_SX - HOME_SX), GALAXY_CENTRE_SZ - HOME_SZ));
        auto sameStar = [](const Star& x, const Star& y) {
            return x.sx == y.sx && x.sy == y.sy && x.sz == y.sz && x.seed == y.seed && x.cls == y.cls && x.pos.x == y.pos.x && x.pos.y == y.pos.y && x.pos.z == y.pos.z &&
                   x.radiusKm == y.radiusKm && x.luminosity == y.luminosity && x.massFactor == y.massFactor && x.color.r == y.color.r && x.color.g == y.color.g && x.color.b == y.color.b &&
                   x.name == y.name && x.pulseHz == y.pulseHz && x.valid == y.valid;
        };
        auto at = [&](const Vec3& q) {
            double t0 = nowSec(); inc.update(q); incMs += (nowSec() - t0) * 1000;
            StarNeighborhood fresh; t0 = nowSec(); fresh.update(q); freshMs += (nowSec() - t0) * 1000;
            bool eq = inc.stars.size() == fresh.stars.size() && inc.cx == fresh.cx && inc.cy == fresh.cy && inc.cz == fresh.cz;
            for (size_t i = 0; eq && i < fresh.stars.size(); i++) eq = sameStar(inc.stars[i], fresh.stars[i]);
            steps++; same += eq; most = std::max(most, fresh.stars.size());
        };
        double along = 0;
        for (int k = 0; k < 50; k++) { along += (0.3 + (k % 7) * 0.55) * SECTOR_KM; at(home + core * along + Vec3(0, (k % 5 - 2) * 0.8 * SECTOR_KM, 0)); }
        at(home + Vec3(4000 * SECTOR_KM, 0, -3000 * SECTOR_KM));
        at(home + Vec3(4000 * SECTOR_KM + 2.6 * SECTOR_KM, -1.3 * SECTOR_KM, -3000 * SECTOR_KM + 0.4 * SECTOR_KM));
        at(home);
        check("stars: the neighbourhood kept across moves (F-02)", steps == 53 && same == steps && most > 50,
              fmt("%d of %d positions the fresh walk's stars in its order (up to %zu stars); %.2f ms kept against %.2f fresh", same, steps, most, incMs, freshMs));
    }
    {   // the landing map and the drone bay open at once on a world not yet mapped (the landing map READING THE WORLD, the bay THE MAP
        // IS BEING DRAWN; a world not mapped held them 150-200 ms a map before), the map drawn once its thread has made it; Enter still lands
        TestGame game;
        game.savePrefix = "shots/tests/test_save";
        game.settingsPath = "shots/tests/test_settings.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        game.guidePath = "shots/tests/test_guide_f02.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto frame = [&]() { game.frame(gi, 1.0 / 30); gi.newFrame(); };
        Star st; bool found = false; int bi = -1;
        for (int64_t x = HOME_SX + 40; x < HOME_SX + 80 && !found; x++) {
            if (!starInSector(x, HOME_SY, HOME_SZ + 7, st)) continue;
            StarSystem ss; ss.generate(st);
            for (const Body& b : ss.bodies) if (KINDS[b.type].landable && b.type != PT_COMET && b.parent < 0) { bi = b.index; found = true; break; }
        }
        bool opened = false, reading = false, drawn = false, landed = false, bay = false, bayLater = false; int blocked = -1; double openMs = 0;
        if (found) {
            const int b0 = game.testMapStats().blocked;
            const double t0 = nowSec();
            game.testParkAt(st, bi, false); frame();
            openMs = (nowSec() - t0) * 1000;
            opened = game.state == GameState::LANDING_MAP;
            const uint64_t h0 = fnv(1469598103934665603ULL, game.output(), (size_t)FBW * FBH * sizeof(uint32_t));
            reading = opened && !game.testMapReadyNow();
            game.testWaitMap(); frame();
            drawn = game.testMapReadyNow() && fnv(1469598103934665603ULL, game.output(), (size_t)FBW * FBH * sizeof(uint32_t)) != h0;
            blocked = game.testMapStats().blocked - b0;
            Star st2; bool found2 = false; int bi2 = -1;   // the bay: another world never seen, the ship parked there, the bay opened
            for (int64_t x = HOME_SX + 81; x < HOME_SX + 140 && !found2; x++) {
                if (!starInSector(x, HOME_SY, HOME_SZ + 7, st2)) continue;
                StarSystem ss; ss.generate(st2);
                for (const Body& b : ss.bodies) if (KINDS[b.type].landable && b.type != PT_COMET && b.parent < 0) { bi2 = b.index; found2 = true; break; }
            }
            if (found2) {
                game.testParkAt(st2, bi2, false); game.setState(GameState::SPACE);
                game.testOpenDroneBay(); frame();
                bay = game.state == GameState::DRONES && !game.testDroneBayMapped();
                for (int i = 0; i < 300 && !game.testDroneBayMapped(); i++) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); frame(); }
                bayLater = game.testDroneBayMapped() && game.testMapStats().blocked - b0 == 0;
                gi.pressed[KEY_ESCAPE] = true; frame();
            }
            game.testParkAt(st, bi, false); frame();   // Enter while the world is read (the map is held: Enter at once on a fresh opening)
            gi.pressed[KEY_ENTER] = true; gi.down[KEY_ENTER] = true; frame(); gi.down[KEY_ENTER] = false;
            landed = game.state == GameState::DESCENT;
        }
        check("landing map and bay: no wait for the world's map (F-02)", found && opened && reading && drawn && blocked == 0 && bay && bayLater && landed,
                fmt("the opening %.0f ms, %s, then the map %s; %d blocking map calls; the bay %s, its map %s; Enter %s", openMs, reading ? "READING THE WORLD" : "NOT READING",
                        drawn ? "drawn" : "NOT DRAWN", blocked, bay ? "open with its map being drawn" : "NOT OPEN AS EXPECTED", bayLater ? "drawn later" : "NOT DRAWN",
                        landed ? "lands" : "DOES NOT LAND"));
        remove(game.guidePath.c_str());
    }
    {   // a fresh site's first frame (every cell of its rings a miss: the warm's fill on the pool) drawn by the workers and by one
        // thread: the same picture and the same five caches, every field of every vertex. The home world's default site from the descent's
        // height and standing, the modern test world beside its capital standing (its roads, streets and works in the samples) and a comet
        // standing; each one's rings fit their tori (2r + 3 cells a side within 1 << logN, F-04: 2r + 5, so the fill's cells have distinct slots)
        const int saved = g_testScale; setFramebufferScale(2);
        auto cachesHash = [](const SurfaceSite& st) {
            uint64_t h = 1469598103934665603ULL;
            for (const TerrainCache* c : {&st.lodN, &st.lod0, &st.lod1, &st.lod2, &st.lod3})
                for (const TerrainVertex& tv : c->v) {
                    const float f[5] = {tv.h, tv.albedo, tv.veg, tv.water, tv.shore};
                    const int32_t i[2] = {tv.cx, tv.cz};
                    const uint8_t b[5] = {tv.material, tv.glow, tv.biome, tv.scree, tv.road};
                    h = fnv(h, f, sizeof f); h = fnv(h, i, sizeof i); h = fnv(h, b, sizeof b);
                }
            return h;
        };
        int sites = 0, same = 0, fit = 0, misses = 0; std::string bad, seen;
        auto both = [&](const char* what, const StarSystem& sys, int body, double lat, double lon, double t, double alt) {
            uint64_t fh[2] = {0, 0}, ch[2] = {0, 0}; int m = 0;
            for (int pass = 0; pass < 2; pass++) {
                parallelThreadsOverride() = pass;   // 0: the pool's threads; 1: one
                SurfaceView sv; sv.init(&sys, body, lat, lon, t);
                if (alt > 0) { sv.cameraOverrideAlt = alt; sv.cameraOverridePitch = -60 * DEG; }   // the descent's first frame: Enter, init, render
                StarNeighborhood nb; nb.update(sv.site.worldPos(t, 0, 0, 0)); SpaceRenderer sr; Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
                fh[pass] = fnv(1469598103934665603ULL, fb.idx.data(), fb.idx.size() * sizeof(Pix)); ch[pass] = cachesHash(sv.site);
                if (pass == 0) {
                    m = sv.lastWarmMisses;
                    int r[5]; sv.testRings(r);
                    const TerrainCache* cs[5] = {&sv.site.lod0, &sv.site.lod1, &sv.site.lod2, &sv.site.lod3, &sv.site.lodN};
                    bool ok = true;
                    for (int k = 0; k < 5; k++) if (r[k] > 0 && 2 * r[k] + 5 > (1 << cs[k]->logN)) ok = false;   // 2r + 5 (the disc's reach on a lava world)
                    if (ok) fit++; else bad += fmt("%s%s: a ring's disc wider than its torus", bad.empty() ? "" : "; ", what);
                }
            }
            parallelThreadsOverride() = 0;
            sites++; misses += m;
            seen += fmt("%s%s %d", seen.empty() ? "" : ", ", what, m);
            if (fh[0] == fh[1] && ch[0] == ch[1]) same++;
            else bad += fmt("%s%s: %s", bad.empty() ? "" : "; ", what, fh[0] != fh[1] ? "the frame DIFFERS with one thread" : "the caches DIFFER with one thread");
        };
        Star hs;
        if (starInSector(HOME_SX, HOME_SY, HOME_SZ, hs, true)) {
            StarSystem hsys; hsys.generate(hs);
            int hb = -1; for (const Body& b : hsys.bodies) if (KINDS[b.type].landable && b.parent < 0 && b.type != PT_COMET) { hb = b.index; break; }
            if (hb >= 0) { both("home, descending", hsys, hb, 15.13 * DEG, -8.01 * DEG, 1000.0, 1800); both("home, standing", hsys, hb, 15.13 * DEG, -8.01 * DEG, 1000.0, -1); }
        }
        {
            const AgeWorld& mw = AGE_WORLDS[AGE_MODERN]; Star ms;
            if (starInSector(mw.sx, 0, mw.sz, ms, true)) {
                StarSystem msys; msys.generate(ms);
                const BodyGen mg = BodyGen::make(msys.bodies[mw.body]);
                std::shared_ptr<const WorldPlan> P = worldPlanOf(mg);
                const PlanPlace& cap = P->plans[std::min(mw.which, P->peoples - 1)].places[0];
                both("the modern capital", msys, mw.body, cap.lat + (cap.radiusM + 400) / (mg.R * 1000.0), cap.lon, 1000.0, -1);
            }
        }
        {
            bool done = false;
            for (auto& [x, z, cs, csys] : scanSystems(150, 340, 20, 160, [&] { return !done; })) {
                for (const Body& b : csys.bodies) if (b.type == PT_COMET) { both("a comet", csys, b.index, 20 * DEG, 30 * DEG, 1000.0, -1); done = true; break; }
            }
        }
        setFramebufferScale(saved);
        check("warm: a fresh site's rings filled on the pool (F-03)", sites == 4 && same == sites && fit == sites && misses > 4 * 4 * SurfaceView::FILL_SERIAL_MAX,
                fmt("%d first frames the same with %d threads and with one, frame and caches: %d; the rings within their tori: %d; cells sampled (%s)%s%s", sites, parallelThreads(), same, fit,
                        seen.c_str(), bad.empty() ? "" : "; ", bad.c_str()));
    }
    {   // the routing's descent sinking a pit of two cells a binade at a time (`drainageSink`) is the float loop it stands for, bit for
        // bit: powers of two and their neighbours either side, zero, the binade where the step rounds by parity, and random starts and counts
        Rng rng(0xF03ULL);
        int cases = 0, differ = 0; long steps = 0; std::string first;
        auto one = [&](float v, long m) {
            float a = v; for (long i = 0; i < m; i++) a = a - 0.005f;
            const float b = drainageSink(v, m);
            cases++; steps += m;
            if (std::memcmp(&a, &b, 4) != 0) { if (first.empty()) first = fmt("; %.9g after %ld: the loop %.9g, the sink %.9g", v, m, a, b); differ++; }
        };
        for (int e = -8; e <= 15; e++)
            for (int sg = -1; sg <= 1; sg += 2)
                for (int d = -2; d <= 2; d++) {
                    float v = sg * std::ldexp(1.0f, e);
                    for (int k = 0; k < std::abs(d); k++) v = std::nextafter(v, d < 0 ? -1e9f : 1e9f);
                    for (long m : {0L, 1L, 2L, 3L, 40L, 3000L, 60000L}) one(v, m);
                }
        for (float v : {0.0f, -0.0f, 0.005f, 0.02f, -0.02f, 0.999f, 1.0f, -1.0f}) for (long m : {1L, 2L, 5L, 450L, 9000L}) one(v, m);
        for (int i = 0; i < 2000; i++) {
            const float v = i % 4 == 3 ? (float)((i & 4 ? -1 : 1) * std::pow(10.0, rng.range(-6, 4))) : (float)rng.range(-3000, 30000);
            one(v, rng.irange(60000));
        }
        check("drainage: the two-cell pit sunk a binade at a time (F-03)", differ == 0 && cases > 2000,
              fmt("%d cases, %ld steps of the loop, %d differ%s", cases, steps, differ, first.c_str()));
    }
    {   // the terrain's cells binned once a frame by the rows their pieces reach: the frame the same with the pool's threads and with
        // one, three and five (other band edges: a cell missing from a band its pieces reach would leave a hole there), and no corner the
        // bin found unwarmed; scenes with a sea and its shore cuts, a lake, a comet's curved disc and the nose camera, at 1x, 2x and 4x
        const int saved = g_testScale;
        int frames = 0, same = 0, stale = 0, cells = 0, every = 0; std::string bad;
        for (const char* name : {"felisian_shore", "felisian_lake", "comet_day", "buggy_seat"})
            if (const CmpScene* found = findScene(name)) {
                const CmpScene& sc = *found;
                for (int scale : {1, 2, 4}) {
                    setFramebufferScale(scale);
                    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
                    if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { bad += fmt("%sno %s", bad.empty() ? "" : "; ", name); continue; }
                    Framebuffer fb;
                    auto draw = [&]() { sv.render(fb, t, nb.stars, sr, 1.0); stale += sv.lastStaleCorners; return fnv(1469598103934665603ULL, fb.idx.data(), fb.idx.size() * sizeof(Pix)); };
                    const uint64_t a = draw();
                    cells += sv.lastTerrainCells; every += sv.lastTerrainAllRows;
                    for (int threads : {1, 3, 5}) {
                        parallelThreadsOverride() = threads; const uint64_t b = draw(); parallelThreadsOverride() = 0;
                        frames++;
                        if (b == a) same++; else bad += fmt("%s%s at %dx DIFFERS with %d threads", bad.empty() ? "" : "; ", name, scale, threads);
                    }
                }
            }
        setFramebufferScale(saved);
        check("terrain: the cells binned by their rows (F-04)", frames == 36 && same == frames && stale == 0 && cells > 0,
                fmt("%d frames the same with %d threads and with one, three and five: %d; %d cells binned (%d in every band), %d corners unwarmed%s%s", frames, parallelThreads(), same, cells, every,
                        stale, bad.empty() ? "" : "; ", bad.c_str()));
    }
    {   // the camera feeds' noise by rows: the state at every row's start from the row's jump (a 32x32 bit matrix) is the state the
        // serial walk reaches there, a row of FBW steps at a time, at every scale's width, for seeds of both feeds and the extremes
        int rows = 0, same = 0; std::string bad;
        for (int sc = 1; sc <= 4; sc++) {
            const int w = UW * sc, h = UH * sc;
            for (uint32_t seed : {0x9E37u, 0x51A7u, 123u * 2654435761u + 0x9E37u, 0xFFFFFFFFu, 1u}) {
                std::vector<uint32_t> starts;
                xorshiftRowStarts(seed, w, h, starts);
                uint32_t rs = seed;
                for (int y = 0; y < h; y++) {
                    rows++;
                    if (starts[y] == rs) same++;
                    else if (bad.empty()) bad = fmt("width %d, seed %08x: row %d starts at %08x, the walk at %08x", w, seed, y, starts[y], rs);
                    for (int x = 0; x < w; x++) rs = xorshift32(rs);
                }
            }
        }
        check("feeds: the noise's rows from the jump (F-05)", rows > 0 && same == rows,
                fmt("%d rows at four widths and five seeds, %d start as the serial walk%s%s", rows, same, bad.empty() ? "" : ": ", bad.c_str()));
    }
    {   // wrap2pi's turns without fmod give fmod's bits: a sweep over three turns either way, the values next to every turn's edge
        // and the specials, against the formula it had
        auto old = [](double a) { a = std::fmod(a, TAU); if (a < 0) a += TAU; return a; };
        std::vector<double> xs = {0.0, -0.0, TAU, -TAU, 2 * TAU, -2 * TAU, 3 * TAU, PI, -PI, 1e-300, -1e-300, 4.9e-324, -4.9e-324, 1e300, -1e300,
                                  std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
        for (int k = -4; k <= 4; k++) {
            double e = k * TAU;
            for (int s = 0; s < 64; s++) { xs.push_back(e); xs.push_back(-e); e = std::nextafter(e, 1e300); }
            e = k * TAU;
            for (int s = 0; s < 64; s++) { e = std::nextafter(e, -1e300); xs.push_back(e); }
        }
        Rng rng(0xF07);
        for (int i = 0; i < 2000000; i++) xs.push_back(rng.range(-3 * TAU, 3 * TAU));
        for (int i = 0; i < 200000; i++) xs.push_back(rng.range(-1e4, 1e4));
        size_t same = 0; std::string bad;
        for (double a : xs) {
            const double n = wrap2pi(a), o = old(a);
            if (memcmp(&n, &o, sizeof n) == 0) same++;
            else if (bad.empty()) bad = fmt(": %.17g gives %.17g, fmod %.17g", a, n, o);
        }
        check("wrap2pi: a turn either side without fmod, the bits it had (F-07)", same == xs.size(), fmt("%zu values, %zu the same%s", xs.size(), same, bad.c_str()));
    }
    {   // the floors to an integer against std::floor: the truncation's form (baseline x86-64's) and the one this build uses, over the
        // integers and their neighbours to 2^40 either way, the halves, -0.0, and two million values at the scales the noise and maps see
        std::vector<double> xs = {0.0, -0.0, 0.5, -0.5, 1e-300, -1e-300, 4.9e-324, -4.9e-324, 2147483647.0, -2147483648.0, 9.0e15, -9.0e15};
        for (double e = 1; e < 1.1e12; e *= 2)
            for (double b : {e, -e, e + 0.5, -e - 0.5}) { xs.push_back(b); xs.push_back(std::nextafter(b, 1e300)); xs.push_back(std::nextafter(b, -1e300)); }
        Rng rng(0xF10);
        for (int i = 0; i < 1000000; i++) xs.push_back(rng.range(-600, 600));
        for (int i = 0; i < 1000000; i++) xs.push_back(rng.range(-1e9, 1e9));
        size_t same = 0; std::string bad;
        for (double v : xs) {
            const int64_t f = (int64_t)std::floor(v);
            const bool ok = ifloorTrunc(v) == f && ifloor64(v) == f && (std::fabs(v) >= 2e9 || ifloor(v) == (int)f);
            if (ok) same++; else if (bad.empty()) bad = fmt(": %.17g gives %lld / %lld, floor %lld", v, (long long)ifloorTrunc(v), (long long)ifloor64(v), (long long)f);
        }
        check("ifloor: the truncation's floor and this build's, std::floor's integer (F-07)", same == xs.size(), fmt("%zu values, %zu the same%s", xs.size(), same, bad.c_str()));
    }
    {   // the mush's quotient as a multiply and a shift (`(sum x m) >> 32`, m = 2^32 / d + 1, d = rows x k): every sum of a box of
        // intensities (to 2047 d) for the kernels to 8, and for every kernel to 38 the sums either side of each multiple of d (where a
        // short reciprocal would first be wrong)
        long tested = 0, wrong = 0; std::string bad;
        auto test = [&](uint32_t sum, uint32_t d) {
            const uint64_t m = ((uint64_t)1 << 32) / d + 1;
            tested++;
            if ((uint32_t)((sum * m) >> 32) != sum / d) { wrong++; if (bad.empty()) bad = fmt(": %u / %u", sum, d); }
        };
        for (uint32_t k = 1; k <= 38; k++)
            for (uint32_t rows = 1; rows <= k; rows++) {
                const uint32_t d = rows * k;
                if (k <= 8) for (uint32_t s = 0; s <= 2047 * d; s++) test(s, d);
                else for (uint32_t q = 0; q <= 2047; q++) { test(q * d, d); if (q) test(q * d - 1, d); }
            }
        check("mush: the box's quotient by a multiply and a shift (F-07)", wrong == 0, fmt("%ld quotients, %ld wrong%s", tested, wrong, bad.c_str()));
    }
    {   // the pool's dispatch, printed and not checked (it decides F-06's spin): an empty parallelFor over eight bands after a rest of
        // 2 ms (the workers parked, as at a frame's first pass) and fifteen back to back (a surface frame dispatches about fifteen)
        auto empty = [](int, int) {};
        for (int i = 0; i < 200; i++) parallelFor(8, 1, empty);
        double rested = 0, burst = 0; const int R = 200;
        for (int r = 0; r < R; r++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            double t0 = nowSec(); parallelFor(8, 1, empty); rested += nowSec() - t0;
            t0 = nowSec(); for (int k = 0; k < 15; k++) parallelFor(8, 1, empty); burst += nowSec() - t0;
        }
        printf("  pool (F-01, printed): %d threads; an empty dispatch over eight bands after a rest %.1f us, fifteen back to back %.1f us each (%.0f us the fifteen)\n", parallelThreads(),
                rested / R * 1e6, burst / R / 15 * 1e6, burst / R * 1e6);
    }
}
