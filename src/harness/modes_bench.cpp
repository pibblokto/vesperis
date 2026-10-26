// E-03: the costs: `bench` (the frame's budgets, `bench check`), `bench forest`, `bench noise`, `pool`, `load`, `soak`
#include "harness/harness.h"
#include "harness/test_worlds.h"

// the process as B-422 measured it: its CPU seconds (all threads), its threads alive and running now, its resident memory (MB);
// macOS and Linux (elsewhere zeros)
struct ProcSample { double cpuS = 0; int threads = 0, running = 0; double rssMB = 0; };
static ProcSample procSample() {
    ProcSample p;
#if defined(__APPLE__) || defined(__linux__)
    struct rusage ru; if (getrusage(RUSAGE_SELF, &ru) == 0) p.cpuS = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec * 1e-6 + ru.ru_stime.tv_sec + ru.ru_stime.tv_usec * 1e-6;
#endif
#ifdef __APPLE__
    thread_act_array_t list; mach_msg_type_number_t n = 0;
    if (task_threads(mach_task_self(), &list, &n) == KERN_SUCCESS) {
        p.threads = (int)n;
        for (mach_msg_type_number_t i = 0; i < n; i++) {
            thread_basic_info_data_t info; mach_msg_type_number_t cnt = THREAD_BASIC_INFO_COUNT;
            if (thread_info(list[i], THREAD_BASIC_INFO, (thread_info_t)&info, &cnt) == KERN_SUCCESS && info.run_state == TH_STATE_RUNNING) p.running++;
            mach_port_deallocate(mach_task_self(), list[i]);
        }
        vm_deallocate(mach_task_self(), (vm_address_t)list, n * sizeof(thread_act_t));
    }
    mach_task_basic_info_data_t ti; mach_msg_type_number_t tc = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&ti, &tc) == KERN_SUCCESS) p.rssMB = ti.resident_size / 1048576.0;
#elif defined(__linux__)
    if (DIR* d = opendir("/proc/self/task")) {
        while (dirent* e = readdir(d)) {
            if (e->d_name[0] == '.') continue;
            p.threads++;
            std::ifstream st(std::string("/proc/self/task/") + e->d_name + "/stat"); std::string all; std::getline(st, all);
            const size_t close = all.rfind(')'); if (close != std::string::npos && close + 2 < all.size() && all[close + 2] == 'R') p.running++;
        }
        closedir(d);
    }
    { std::ifstream sm("/proc/self/statm"); long pages = 0, res = 0; sm >> pages >> res; p.rssMB = res * (double)sysconf(_SC_PAGESIZE) / 1048576.0; }
#endif
    return p;
}

// F-01: a guide of the player's composition (the dev Mac's guide.txt of 2026-10-11, 731,810 bytes: five probes' camera tracks of 5,889 to
// 8,856 values, a log of 1,087 lines, the shards, names, marks and the rest in their counts), made from a seed: the bench times the save a
// long expedition pays at every logged event without reading anyone's file
void fillBenchGuide(Guide& G) {
    Rng r(0x6D1DEULL);
    auto key = [](int i) { return Guide::bodyKey(150 + i % 97, 0, 20 + i % 89, i % 7); };
    G.genVersion = GEN_VERSION;
    for (int i = 0; i < 46; i++) G.names[key(i)] = fmt("THE EXPLORER'S NAME %d", i);
    for (int i = 0; i < 93; i++) G.visited.insert(Guide::starKey(150 + i, 0, 30 + i % 40));
    for (int i = 0; i < 65; i++) G.landed.insert(key(i));
    for (int i = 0; i < 35; i++) { SurfaceMark m; m.body = key(i); m.lat = r.range(-1.2, 1.2); m.lon = r.range(-PI, PI); m.when = r.range(1e6, 2e7); if (i % 2) m.name = "A MARK"; G.marks.push_back(m); }
    for (int i = 0; i < 285; i++) G.shards.insert(key(i % 40) + "/S" + std::to_string(i));
    for (int i = 0; i < 268; i++) G.decoded[key(i % 40) + "/S" + std::to_string(i)] = i % 30;
    for (int i = 0; i < 24; i++) G.signals[key(i)] = i % 4;
    for (int i = 0; i < 53; i++) G.heard.insert(key(i % 30) + "/S" + std::to_string(i));
    for (int i = 0; i < 28; i++) G.graves.insert(key(i % 20) + "/G" + std::to_string(i));
    for (int i = 0; i < 9; i++) G.instruments.insert(key(i) + "/I" + std::to_string(i));
    for (int i = 0; i < 14; i++) G.classesSeen.insert(i);
    for (int i = 0; i < 13; i++) G.typesSeen.insert(i);
    const int cams[5] = {8856, 8784, 8451, 7620, 5889}, times[5] = {102, 82, 82, 82, 82}, layers[5] = {18, 18, 24, 18, 24};
    for (int k = 0; k < 5; k++) {
        ProbeRecord p; p.id = k + 1; p.body = key(k); p.gen = GEN_VERSION; p.saved = 1760000000LL + k; p.launchT = r.range(1e6, 2e7); p.aimLat = r.range(-1, 1); p.aimLon = r.range(-3, 3);
        p.seed = r.next(); p.startX = r.range(-1e5, 1e5); p.startY = r.range(-1e5, 1e5); p.startZ = r.range(-1e5, 1e5); p.headYaw = r.range(-3, 3); p.descYaw = r.range(-3, 3);
        p.endClock = r.range(300, 700); p.endBar = r.range(5, 30); p.endKm = r.range(50, 200); p.endK = r.range(200, 600); p.endCause = 1;
        for (int i = 0; i < cams[k]; i++) p.cam.push_back(i % 3 == 0 ? i * 0.0371 : r.range(-PI, PI));   // clock, yaw, pitch where they changed
        for (int i = 0; i < times[k]; i++) p.time.push_back(i % 2 == 0 ? i * 3.7 : r.range(1e6, 2e7));
        for (int i = 0; i < 42; i++) { ProbeSample q; q.clock = i * 15.7; q.bar = 0.1 * (i + 1) + r.range(0, 0.05); q.tempK = r.range(100, 400); q.windE = r.range(-150, 150); q.light = r.range(0, 1); q.layer = i % 4; q.flashes = i % 3; p.profile.push_back(q); }
        for (int i = 0; i < layers[k]; i++) p.layers.push_back(r.range(0, 30));
        G.probes.push_back(p);
    }
    for (int i = 0; i < 1087; i++) G.log.push_back({r.range(1e6, 2e7), i % 3 ? "SECTOR" : "SIGHT", fmt("ENTERED SECTOR %d:%d ON THE WORLD THE STARDRIFTER CIRCLES, THE %d ENTRY OF A LONG ROAD", i % 360, i % 180, i)});
}

// B-310: `bench forest`: the forest scene at 4x rendered 150 times, for a profiler (`sample vesperis_test 5` on macOS)
int runBenchForest() {
    setFramebufferScale(4);
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    if (!setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv, sys, nb, t, SCENE_FOREST)) return 1;
    Framebuffer fb; Input in;
    double t0 = nowSec();
    for (int i = 0; i < 150; i++) { sv.update(1.0 / 30, in, t + i * 0.033, false); sv.render(fb, t + i * 0.033, nb.stars, sr, 1.0); fb.mush(2); }
    printf("forest 4x x150: %.2f ms/frame (sections sky %.1f warm %.1f terrain %.1f objects %.1f flora %.1f rest %.1f; trees %.1f ms)\n", (nowSec() - t0) * 1000 / 150,
           sv.lastRenderMs[0], sv.lastRenderMs[1], sv.lastRenderMs[2], sv.lastRenderMs[3], sv.lastRenderMs[4], sv.lastRenderMs[5], sv.lastFloraMs[0]);
    printf("  %s\n  %s\n", sv.bandTraceLine(false).c_str(), sv.bandTraceLine(true).c_str());   // the last frame's bands
    return 0;
}

// E-03: the bench's state: the game, its input, the times the budgets read and the failures, and what every row does (press, bench,
// settle, budget); each stage is a member function, so that its code reads as it did in one function
struct Bench {
    TestGame game;
    Input in;
    double slack = 1;
    double surface2x = 0, surface4x = 0, space2x = 0, space4x = 0, descentFirst = 0, runWorst = 0;
    double telescope2x = 0, vimana2x = 0, probe2x = 0, descentPeople = 0, capital2x = 0;
    int fails = 0;
    void press(int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 60); in.newFrame(); in.down[key] = false; }
    double bench(const char* name, int frames) {
        double t0 = nowSec();
        for (int i = 0; i < frames; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
        double dt = (nowSec() - t0) / frames * 1000;
        printf("%-12s %.2f ms/frame\n", name, dt);
        return dt;
    }
    // five frames at one instant after a scale's change, before a row's timed frames: the first frames at a new scale allocate its
    // buffers and tables (on the swapping Mac one run's space 4x read 11.0 ms against 6.4-6.6), and the timeline stays the rows' before
    void settle() { for (int i = 0; i < 5; i++) { game.frame(in, 0); in.newFrame(); } }
    // performance budgets. F-11: about 1.5x the dev Mac's medians after F-10 (an M4 of four performance and six efficiency cores;
    // at least the median + 0.5 ms), so a change that gives a gain back fails; `bench check <factor>` multiplies them for another machine
    // or one thread
    void budget(const char* name, double ms, double limit) { limit *= slack; bool ok = ms <= limit; printf("budget %-14s %.1f ms (limit %g) %s\n", name, ms, limit, ok ? "ok" : "EXCEEDED"); if (!ok) fails++; }

    void space();   // the start: space at each scale, the telescope at its deepest
    void descent();   // the landing map, the drainage tiles, the descent's first frames
    void surface();   // the ground: the first frames, each scale with its sections, the sprint, the turns, the map cache
    void forest();   // a forest at 2x through the surface renderer alone
    void lamp();   // the lamp at 4x on a drifter in the dark
    void city();   // the test city at 2x
    void herd();   // a herd at 1x
    void drive();   // the driving frame at 2x
    void belt();   // inside a belt at 2x
    void vimana();   // a flight's frame at 2x
    void probe();   // the probe's camera at 2x
    void people();   // the descent onto a people's world, its capital at 2x
    void unbudgeted();   // the rows without budgets
    void budgets();   // the budgets
};

void Bench::space() {
    game.savePrefix = "shots/tests/bench_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/bench_guide.txt";
    press(KEY_ENTER);
    for (int s = 1; s <= 4; s++) { game.settings.renderScale = s; game.applySettings(); settle(); double ms = bench(("space " + std::to_string(s) + "x").c_str(), 60); if (s == 2) space2x = ms; if (s == 4) space4x = ms; }
    {   // W-06: the telescope at its deepest magnification on the parked world (the globe fills the frame), 2x
        game.settings.renderScale = 2; game.applySettings();
        game.testAimAtBody(game.testParkedBody()); press(KEY_Z); for (int i = 0; i < 9; i++) press(KEY_EQUAL);
        telescope2x = bench("telescope 2x", 60);
        printf("  %s\n  %s\n", game.testTelescopeInfo().c_str(), game.testTelescopePlate().c_str());
        writePNG("shots/tests/bench_telescope.png", game.output(), FBW, FBH);
        press(KEY_Z);
    }
}

void Bench::descent() {
    game.settings.renderScale = 1; game.applySettings();
    press(KEY_C);   // G-02: no R: it picked a random lit site seeded by the clock, whose tiles took 0.4-3.2 s by luck (KI-338); the default site is the measure
    game.testWaitMap();   // no map thread beside the descent's frames
    {   // a player looks at the map for a moment; the drainage tiles of the site are computed meanwhile (up to 3 s here)
        int waited = 0; double tw0 = nowSec();
        for (; nowSec() - tw0 < 3.0 && !game.testDrainageReady(); waited++) { game.frame(in, 1.0 / 60); in.newFrame(); }
        printf("landing map  %.2f s (%d frames) until the drainage tiles were ready (%s, %d tiles held)\n", nowSec() - tw0, waited, game.testBodyTypes().c_str(), drainageTilesHeld());
    }
    {
        // the first descent frames can hitch (KI-009): time them
        in.pressed[KEY_ENTER] = true; in.down[KEY_ENTER] = true;
        double worst = 0, first = 0;
        for (int i = 0; i < 60; i++) {
            double t0 = nowSec();
            game.frame(in, 1.0 / 60);
            double ms = (nowSec() - t0) * 1000;
            in.newFrame(); in.down[KEY_ENTER] = false;
            if (i == 0) first = ms;
            worst = std::max(worst, ms);
        }
        printf("descent      first frame %.1f ms, worst of 60: %.1f ms\n", first, worst);
        { const double* ms = game.testRenderMs(); printf("  last frame's render sections: palette+sky %.1f, warm %.1f, terrain %.1f, footprints..objects %.1f, flora %.1f, rest %.1f ms\n", ms[0], ms[1], ms[2], ms[3], ms[4], ms[5]); }
        descentFirst = first;
    }
    for (int i = 0; i < 60 * 7; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
}

void Bench::surface() {
    {   // M7-01: the first surface frames after landing (the caches were warmed during the descent)
        double first = 0, worst = 0;
        for (int i = 0; i < 30; i++) { double t0 = nowSec(); game.frame(in, 1.0 / 60); double ms = (nowSec() - t0) * 1000; in.newFrame(); if (i == 0) first = ms; worst = std::max(worst, ms); }
        printf("landing      first frame %.1f ms, worst of 30: %.1f ms\n", first, worst);
    }
    for (int s = 4; s >= 2; s--) {
        game.settings.renderScale = s; game.applySettings(); settle();
        double ms = bench(("surface " + std::to_string(s) + "x").c_str(), 40);
        if (s == 2) surface2x = ms;
        if (s == 4) surface4x = ms;
        const double* r = game.testRenderMs();
        printf("             sections: sky %.1f, warm %.1f, terrain %.1f, objects %.1f, flora %.1f, rest %.1f ms\n", r[0], r[1], r[2], r[3], r[4], r[5]);
        for (int f = 0; f < 2; f++) { const std::string bl = game.testSurface().bandTraceLine(f == 1); if (!bl.empty()) printf("             %s\n", bl.c_str()); }   // the last frame's bands
    }
    game.settings.renderScale = 1; game.applySettings(); settle();
    bench("surface", 120);
    writePNG("shots/tests/bench_surface.png", game.output(), FBW, FBH);
    printf("  state=%d %s\n", (int)game.state, game.testStatus().c_str());
    in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true;
    {   // a sprint of four seconds; the worst frame shows whether the caches stall
        double worst = 0, sum = 0;
        for (int i = 0; i < 240; i++) { double t0 = nowSec(); game.frame(in, 1.0 / 60); double ms = (nowSec() - t0) * 1000; in.newFrame(); worst = std::max(worst, ms); sum += ms; }
        printf("surface-run  %.2f ms/frame, worst %.1f ms, %d cells delivered by the prefetch worker\n", sum / 240, worst, game.testAheadCells());
        runWorst = worst;
    }
    in.down[KEY_W] = false; in.down[KEY_LEFT_SHIFT] = false;
    in.mouseDx = 300; game.frame(in, 1.0 / 60); in.newFrame();
    bench("surface2", 120);
    writePNG("shots/tests/bench_surface2.png", game.output(), FBW, FBH);
    in.mouseDx = 300; game.frame(in, 1.0 / 60); in.newFrame();
    bench("surface3", 120);
    writePNG("shots/tests/bench_surface3.png", game.output(), FBW, FBH);
    for (int i = 0; i < 3; i++) press(KEY_T);
    for (int i = 0; i < 60 * 6; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
    writePNG("shots/tests/bench_surface4.png", game.output(), FBW, FBH);
    // map cache counters
    const SpaceRenderer::MapStats& ms = game.testMapStats();
    printf("maps         %d generated (%.0f ms each on the worker), %d hits, %d evicted, %zu held (cap 24)\n", ms.generated, ms.generated ? game.testGenMicros() / 1000.0 / ms.generated : 0.0, ms.hits, ms.evicted, game.testMapsHeld());
}

void Bench::forest() {
    {   // N2-04: a forest view at 2x through the surface renderer alone
        int saved = g_testScale;
        setFramebufferScale(2);
        SpaceRenderer sr2; StarNeighborhood nb2; StarSystem sys2; SurfaceView sv2; double t2 = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv2, sys2, nb2, t2, SCENE_FOREST)) {
            Framebuffer fb2; Input in2;
            for (int i = 0; i < 5; i++) { sv2.update(1.0 / 30, in2, t2, false); sv2.render(fb2, t2, nb2.stars, sr2, 1.0); }
            double t0 = nowSec();
            for (int i = 0; i < 30; i++) { sv2.update(1.0 / 30, in2, t2 + i * 0.033, false); sv2.render(fb2, t2 + i * 0.033, nb2.stars, sr2, 1.0); fb2.mush(2); }
            double ms = (nowSec() - t0) * 1000 / 30;
            printf("forest 2x    %.2f ms/frame (%d trees: near %d mid %d far %d, %d hidden behind canopy, %d flora points; flora sections trees %.1f, cells %.1f ms)\n", ms, sv2.lastTreesDrawn,
                   sv2.lastNearTrees, sv2.lastMidTrees, sv2.lastFarTrees, sv2.lastTreesHidden, sv2.lastFloraPoints, sv2.lastFloraMs[0], sv2.lastFloraMs[1]);
            saveFB(fb2, "shots/tests/bench_forest.png");
            budget("forest 2x", ms, 8.2);   // a tropical giant forest, the densest case (12 ms on an M4 then, 5.46 after F-10; the budget 14 -> 8.2)
        }
        setFramebufferScale(4);   // the same forest at 4x (the default picture)
        { SpaceRenderer sr4; StarNeighborhood nb4; StarSystem sys4; SurfaceView sv4; double t4 = 0;
          if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv4, sys4, nb4, t4, SCENE_FOREST)) {
            Framebuffer fb4; Input in4;
            for (int i = 0; i < 5; i++) { sv4.update(1.0 / 30, in4, t4, false); sv4.render(fb4, t4, nb4.stars, sr4, 1.0); }
            double t0 = nowSec();
            for (int i = 0; i < 30; i++) { sv4.update(1.0 / 30, in4, t4 + i * 0.033, false); sv4.render(fb4, t4 + i * 0.033, nb4.stars, sr4, 1.0); fb4.mush(2); }
            double ms = (nowSec() - t0) * 1000 / 30;
            printf("forest 4x    %.2f ms/frame (flora sections trees %.1f, cells %.1f ms; %d trees listed, %d visits over the bands)\n", ms, sv4.lastFloraMs[0], sv4.lastFloraMs[1], sv4.lastTreesListed, sv4.lastTreesVisited);
            budget("forest 4x", ms, 13);   // 8.37 after F-10 (F-11: 24 -> 13)
            printf("             %s\n             %s\n", sv4.bandTraceLine(false).c_str(), sv4.bandTraceLine(true).c_str());
            {   // the same forest at the game's kernel (the game's fine mush at 4x is 3x3; the row above runs setFramebufferScale's 5x5)
                const int k0 = g_mushKernel; g_mushKernel = 3;
                const double t3 = nowSec();
                for (int i = 0; i < 30; i++) { sv4.update(1.0 / 30, in4, t4 + i * 0.033, false); sv4.render(fb4, t4 + i * 0.033, nb4.stars, sr4, 1.0); fb4.mush(2); }
                const double ms3 = (nowSec() - t3) * 1000 / 30;
                printf("forest 4x    %.2f ms/frame at the game's 3x3 mush (F-01)\n", ms3);
                budget("forest 4x 3x3", ms3, 12);   // 7.97 after F-10
                g_mushKernel = k0;
            }
        } }
        setFramebufferScale(saved);
    }
}

void Bench::lamp() {
    {   // D-04: the explorer's lamp at 4x on a rocky drifter's ground in the dark (the `rogue_lamp` scene): the lamp's term in every vertex and
        // every object's light, against the same frames unlit
        int saved = g_testScale;
        setFramebufferScale(4);
        SpaceRenderer srL; StarNeighborhood nbL; StarSystem sysL; SurfaceView svL; double tL = 0;
        if (setupSceneForType(-1, 20, 0, 0.0, -0.10, svL, sysL, nbL, tL, SCENE_ROGUE_LAMP)) {
            Framebuffer fbL; Input inL;
            auto time30 = [&]() {
                for (int i = 0; i < 5; i++) { svL.update(1.0 / 30, inL, tL, false); svL.render(fbL, tL, nbL.stars, srL, 1.0); }
                const double t0 = nowSec();
                for (int i = 0; i < 30; i++) { svL.update(1.0 / 30, inL, tL + i * 0.033, false); svL.render(fbL, tL + i * 0.033, nbL.stars, srL, 1.0); fbL.mush(2); }
                return (nowSec() - t0) * 1000 / 30;
            };
            svL.lamp = false; const double off = time30();
            svL.lamp = true; const double on = time30();
            printf("lamp 4x      %.2f ms/frame lit, %.2f unlit (a rocky drifter's ground in the dark)\n", on, off);
            saveFB(fbL, "shots/tests/bench_lamp.png");
            budget("lamp 4x", on, 7.0);   // 4.7 lit, 4.7 unlit (the first check's)
        }
        setFramebufferScale(saved);
    }
}

void Bench::city() {
    {   // A-02: the test city at 2x (1,348 buildings, the forms built on the workers and drawn in bands): from 300 m outside its edge and
        // from its plaza looking along a street; the worse of the two
        int saved = g_testScale;
        setFramebufferScale(2);
        SpaceRenderer srC; StarNeighborhood nbC; StarSystem sysC; SurfaceView svC; double tC = 0;
        if (setupSceneForType(PT_DESERT, 15, 40 * DEG, 1.3, 0.0, svC, sysC, nbC, tC, SCENE_TOWN)) {
            svC.palettes[0] = testPalette(); svC.flocks.clear(); svC.critters.clear();
            const double yaw = svC.player.yaw, px0 = svC.player.x, pz0 = svC.player.z, cx = px0 + std::sin(yaw) * 1250, cz = pz0 + std::cos(yaw) * 1250;
            placeCity(svC, cx, cz, 0xC17E5EEDULL);
            Framebuffer fbC; double worst = 0;
            for (int view = 0; view < 2; view++) {
                if (view == 1) { svC.player.x = cx + std::sin(yaw) * 20; svC.player.z = cz + std::cos(yaw) * 20; }   // in the plaza, by the ring
                formsCamera(svC, view == 0 ? 12.0 : 1.7, view == 0 ? cx : cx + std::sin(yaw + 0.25) * 600, svC.site.surfaceHeight(cx, cz) + 30, view == 0 ? cz : cz + std::cos(yaw + 0.25) * 600, tC);
                Input inC;
                for (int i = 0; i < 5; i++) { svC.update(1.0 / 30, inC, tC, false); svC.render(fbC, tC, nbC.stars, srC, 1.0); fbC.mush(2); }
                double t0 = nowSec();
                for (int i = 0; i < 30; i++) { svC.update(1.0 / 30, inC, tC + i * 0.033, false); svC.render(fbC, tC + i * 0.033, nbC.stars, srC, 1.0); fbC.mush(2); }
                double ms = (nowSec() - t0) * 1000 / 30;
                printf("city 2x      %.2f ms/frame %s (the forms %.2f ms: %d items, %d polygons, %d lines, %d silhouettes)\n", ms, view == 0 ? "from its edge" : "in its plaza", svC.lastRuinMs, svC.lastRuinElems, svC.lastFormPolys, svC.lastFormLines, svC.lastSilhouettes);
                saveFB(fbC, view == 0 ? "shots/tests/bench_city.png" : "shots/tests/bench_city_plaza.png");
                worst = std::max(worst, ms);
            }
            budget("city 2x", worst, 5.7);   // 3.78 after F-10 (F-11: 16 -> 5.7)
        }
        setFramebufferScale(saved);
    }
}

void Bench::herd() {
    {   // N3: a herd at 18 m at 1x through the surface renderer: the cost of updating and drawing life
        SpaceRenderer sr3; StarNeighborhood nb3; StarSystem sys3; SurfaceView sv3; double t3 = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, sv3, sys3, nb3, t3, SCENE_HERD)) {
            Framebuffer fb3; Input in3;
            double upd = 0, drw = 0; int n = 0; for (const Critter& c : sv3.critters) if (c.herd == 0) n++;
            for (int i = 0; i < 20; i++) { sv3.update(1.0 / 30, in3, t3 + i * 0.033, false); sv3.render(fb3, t3 + i * 0.033, nb3.stars, sr3, 1.0); if (i >= 5) { upd += sv3.lastLifeMs[0]; drw += sv3.lastLifeMs[1]; } }
            printf("herd 1x      update %.2f ms, draw %.2f ms per frame (%d in the herd, %zu creatures at the site)\n", upd / 15, drw / 15, n, sv3.critters.size());
            budget("herd draw 1x", drw / 15, 0.6);   // 0.07 after F-10 (F-11: 1 -> 0.6, the median + 0.5)
        }
    }
}

void Bench::drive() {
    {   // N4-02: the driving frame at 2x (the game, seat view, 60 frames at full throttle)
        game.settings.renderScale = 2; game.applySettings();
        if (game.testDriveSetup()) {
            Input in4; in4.down[KEY_W] = true;
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in4, 1.0 / 30); in4.newFrame(); in4.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("buggy 2x     %.2f ms/frame, worst %.1f ms (%s)\n", sum / 60, worst, game.testBuggyInfo().c_str());
            writePNG("shots/tests/bench_buggy.png", game.output(), FBW, FBH);
            budget("buggy 2x", sum / 60, 6.1);   // 4.02 after F-10 (F-11: 14 -> 6.1)
        }
        if (game.testFlySetup(200)) {   // the drone 200 m up at full thrust, the camera tilted down 30 deg: the ground from the air
            Input in5; in5.down[KEY_W] = true; game.testSetPitch(-30 * DEG);
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in5, 1.0 / 30); in5.newFrame(); in5.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("drone 2x     %.2f ms/frame, worst %.1f ms (%s)\n", sum / 60, worst, game.testDroneInfo().c_str());
            writePNG("shots/tests/bench_drone.png", game.output(), FBW, FBH);
            budget("drone 2x", sum / 60, 3.3);   // 2.20 after F-10 (F-11: 16 -> 3.3)
        }
        game.settings.renderScale = 4; game.applySettings();   // and at 4x, the default picture
        if (game.testDriveSetup()) {
            Input in4; in4.down[KEY_W] = true;
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in4, 1.0 / 30); in4.newFrame(); in4.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("buggy 4x     %.2f ms/frame, worst %.1f ms\n", sum / 60, worst);
            budget("buggy 4x", sum / 60, 6.1);   // 4.04 after F-10 (F-11: 24 -> 6.1)
        }
        game.settings.renderScale = 1; game.applySettings();
    }
}

void Bench::belt() {
    {   // O3: a frame inside a belt at 2x, parked beside a rock (the rock field is the cost)
        bool found = false;
        for (auto& [x, z, s, sy] : scanSystems(150, 320, 20, 140, [&] { return !found; })) {
            if (sy.belts.empty()) continue;
            game.settings.renderScale = 2; game.applySettings();
            game.testParkAtBelt(s, 0);
            Input in5;
            for (int i = 0; i < 10; i++) { game.frame(in5, 1.0 / 60); in5.newFrame(); }
            in5.pressed[KEY_X] = true; in5.down[KEY_X] = true; game.frame(in5, 1.0 / 60); in5.newFrame(); in5.down[KEY_X] = false;
            double sum = 0;
            for (int i = 0; i < 60; i++) { double t0 = nowSec(); game.frame(in5, 1.0 / 60); in5.newFrame(); sum += (nowSec() - t0) * 1000; }
            printf("belt 2x      %.2f ms/frame (%s)\n", sum / 60, game.testDebugInfo().c_str());
            writePNG("shots/tests/bench_belt.png", game.output(), FBW, FBH);
            budget("belt 2x", sum / 60, 1.1);   // 0.60 after F-10 (F-11: 12 -> 1.1, the median + 0.5)
            game.settings.renderScale = 1; game.applySettings();
            found = true;
        }
    }
}

void Bench::vimana() {
    {   // G-07: a Vimana flight's frame at 2x, 10,000 ly coreward from home at the middle of the flight: the streaks, the band blended from
        // its keyframes and the next keyframe's share built (the frame before the probe's: the probe's parking takes the ship from the flight)
        Star hs;
        if (starInSector(HOME_SX, HOME_SY, HOME_SZ, hs, true)) {
            game.settings.renderScale = 2; game.applySettings();
            game.testArriveAt(hs); game.setState(GameState::SPACE);
            Input in7;
            for (int i = 0; i < 10; i++) { game.frame(in7, 1.0 / 60); in7.newFrame(); }
            const Vec3 h = game.testShipPos() / SECTOR_KM;
            game.testStartJump(10000, normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z)));
            if (game.testFlying()) {
                game.testFlightSkip(0.5 * game.testFlightDur() - 1.0);
                for (int i = 0; i < 60; i++) { game.frame(in7, 1.0 / 60); in7.newFrame(); }
                double sum = 0, worst = 0;
                for (int i = 0; i < 60; i++) { const double t0 = nowSec(); game.frame(in7, 1.0 / 60); in7.newFrame(); const double ms = (nowSec() - t0) * 1000; sum += ms; worst = std::max(worst, ms); }
                vimana2x = sum / 60;
                printf("vimana 2x    %.2f ms/frame, worst %.1f ms (%zu keyframes, %d cells built the last frame)\n", vimana2x, worst, game.testFlightBand().tau.size(), game.testFlightBand().cellsFrame);
                writePNG("shots/tests/bench_vimana.png", game.output(), FBW, FBH);
            } else printf("vimana 2x    no flight ('%s')\n", game.testStatus().c_str());
            game.settings.renderScale = 1; game.applySettings();
        }
    }
}

void Bench::probe() {
    {   // X-02: the probe's camera at 2x on the ringed test giant: over the ammonia deck (its tops rushing up, the panorama's
        // longest marches) and between the decks (two panoramas and the lamp); the worse of the two
        Star ps;
        if (starInSector(150, 0, 106, ps, true)) {
            game.settings.renderScale = 2; game.applySettings();
            game.testParkAt(ps, 5);
            Input in6;
            for (int i = 0; i < 10; i++) { game.frame(in6, 1.0 / 60); in6.newFrame(); }
            in6.pressed[KEY_C] = true; in6.down[KEY_C] = true; game.frame(in6, 1.0 / 60); in6.newFrame(); in6.down[KEY_C] = false;
            if (game.testProbeActive()) {
                double endC = game.testProbeEndClock(), fromHaze = -1, fromBetween = -1, toBetween = -1, toHaze = -1;
                bool betweenDone = false;
                for (double c = 0; c <= endC; c += 0.25) {
                    int st2 = game.testProbeStageAt(c);
                    if (st2 == PS_HAZE) { if (fromHaze < 0) fromHaze = c; toHaze = c; }
                    if (st2 == PS_BETWEEN && !betweenDone) { if (fromBetween < 0) fromBetween = c; toBetween = c; }   // the first clear band
                    else if (fromBetween >= 0) betweenDone = true;
                }
                const double at[2] = {fromHaze + (toHaze - fromHaze) * 0.8, fromBetween + (toBetween - fromBetween) * 0.5};
                const char* const what[2] = {"over the deck", "between the decks"};
                for (int k = 0; k < 2; k++) {
                    game.testProbeSkip(at[k]);
                    for (int i = 0; i < 5; i++) { game.frame(in6, 1.0 / 60); in6.newFrame(); }
                    double sum = 0, worst = 0;
                    for (int i = 0; i < 60; i++) { double t0 = nowSec(); game.frame(in6, 1.0 / 60); in6.newFrame(); double ms = (nowSec() - t0) * 1000; sum += ms; worst = std::max(worst, ms); }
                    printf("probe 2x     %.2f ms/frame, worst %.1f ms (%s: %s)\n", sum / 60, worst, what[k], game.testProbeViewInfo().c_str());
                    if (k == 0) writePNG("shots/tests/bench_probe.png", game.output(), FBW, FBH);
                    probe2x = std::max(probe2x, sum / 60);
                }
            } else printf("probe 2x     no probe launched ('%s')\n", game.testStatus().c_str());
            game.settings.renderScale = 1; game.applySettings();
        }
    }
}

void Bench::people() {
    {   // T-01: the descent's first frame onto a people's world beside its capital (the modern test world: its plan built at the parking, on a thread
        // of its own, waited for as a player looks at the map), then the capital from its edge at 2x (its 1,200 buildings, the skyline beyond)
        Star ms; const AgeWorld& mw = AGE_WORLDS[AGE_MODERN];
        if (starInSector(mw.sx, 0, mw.sz, ms, true)) {
            game.settings.renderScale = 1; game.applySettings();
            Input in8;
            StarSystem msys; msys.generate(ms);
            const BodyGen mg = BodyGen::make(msys.bodies[mw.body]);
            worldPlanClearCache();
            game.testParkAt(ms, mw.body);
            double tw0 = nowSec(); int waited = 0;
            for (; nowSec() - tw0 < 5.0 && !worldPlanBuilt(mg.seed); waited++) { game.frame(in8, 1.0 / 60); in8.newFrame(); }
            const double planS = nowSec() - tw0;
            std::shared_ptr<const WorldPlan> P = worldPlanOf(mg);
            const PlanPlace& cap = P->plans[std::min(mw.which, P->peoples - 1)].places[0];
            const double Rm = mg.R * 1000.0, off = cap.radiusM + 400;
            game.testLandCursor((cap.lat + off / Rm) / DEG, cap.lon / DEG);
            tw0 = nowSec();
            for (; nowSec() - tw0 < 3.0 && !game.testDrainageReady(); waited++) { game.frame(in8, 1.0 / 60); in8.newFrame(); }
            printf("people's world: the plan ready %.2f s after the parking (%d frames meanwhile), the drainage %.2f s later\n", planS, waited, nowSec() - tw0);
            in8.pressed[KEY_ENTER] = true; in8.down[KEY_ENTER] = true;
            double worst = 0;
            for (int i = 0; i < 60; i++) {
                const double t0 = nowSec(); game.frame(in8, 1.0 / 60); const double msF = (nowSec() - t0) * 1000;
                in8.newFrame(); in8.down[KEY_ENTER] = false;
                if (i == 0) descentPeople = msF;
                worst = std::max(worst, msF);
            }
            printf("descent      onto a people's world beside its capital: first frame %.1f ms, worst of 60: %.1f ms\n", descentPeople, worst);
            for (int i = 0; i < 60 * 8; i++) { game.frame(in8, 1.0 / 60); in8.newFrame(); }
            game.settings.renderScale = 2; game.applySettings();
            game.testSurfaceFace(PI, 0.02);   // the capital to the south
            for (int i = 0; i < 20; i++) { game.frame(in8, 1.0 / 60); in8.newFrame(); }
            double sum = 0; worst = 0;
            for (int i = 0; i < 60; i++) { const double t0 = nowSec(); game.frame(in8, 1.0 / 60); in8.newFrame(); const double msF = (nowSec() - t0) * 1000; sum += msF; worst = std::max(worst, msF); }
            capital2x = sum / 60;
            printf("capital 2x   %.2f ms/frame, worst %.1f ms (%s; the forms %.2f ms: %d items, %d polygons, %d lines)\n", capital2x, worst, game.testStatus().c_str(), game.testSurface().lastRuinMs, game.testSurface().lastRuinElems, game.testSurface().lastFormPolys, game.testSurface().lastFormLines);
            writePNG("shots/tests/bench_capital.png", game.output(), FBW, FBH);
            {   // the capital at 4x (no budget)
                game.settings.renderScale = 4; game.applySettings();
                for (int i = 0; i < 10; i++) { game.frame(in8, 1.0 / 60); in8.newFrame(); }
                sum = 0; worst = 0;
                for (int i = 0; i < 60; i++) { const double t0 = nowSec(); game.frame(in8, 1.0 / 60); in8.newFrame(); const double msF = (nowSec() - t0) * 1000; sum += msF; worst = std::max(worst, msF); }
                printf("capital 4x   %.2f ms/frame, worst %.1f ms (the forms %.2f ms; F-01)\n", sum / 60, worst, game.testSurface().lastRuinMs);
                budget("capital 4x", sum / 60, 8.3);   // 5.48 after F-10
            }
            game.settings.renderScale = 1; game.applySettings();
        }
    }
}

void Bench::unbudgeted() {
    {   // F-01: rows without budgets for what the bench did not see, printed so that a later change that gives a gain back is seen (the
        // targets of PLAN-performance's section 2): the sector map, the landing map and its zoom with the cursor moving, the radar camera, a
        // recorded frame and a photo's PNG, the telescope and the decoder, the sun in the view, the city and the probe at 4x, an arrival while
        // its maps generate, the guide's save and load at the player's size
        Input inF;
        auto stepF = [&]() { game.frame(inF, 1.0 / 60); inF.newFrame(); };
        auto pressF = [&](int key) { inF.pressed[key] = true; inF.down[key] = true; stepF(); inF.down[key] = false; };
        auto rowF = [&](const char* name, int frames, const std::function<void()>& each, double limit) {   // a limit of 0, no budget
            double sum = 0, worst = 0;
            for (int i = 0; i < frames; i++) { const double t0 = nowSec(); each(); const double ms = (nowSec() - t0) * 1000; sum += ms; worst = std::max(worst, ms); }
            printf("%-12s %.2f ms/frame, worst %.1f ms (F-01%s)\n", name, sum / frames, worst, limit > 0 ? "" : ", no budget");
            if (limit > 0) budget(name, sum / frames, limit);
            return sum / frames;
        };
        game.settings.renderScale = 4; game.applySettings();
        if (game.state == GameState::SURFACE) {   // the people's world of the capital's rows: its sector map (N)
            pressF(KEY_N); for (int i = 0; i < 10; i++) stepF();
            rowF("sectormap 4x", 60, stepF, 8.2);   // 5.42 after F-10
            writePNG("shots/tests/bench_sectormap.png", game.output(), FBW, FBH);
            pressF(KEY_ESCAPE);
        }
        Star hs; StarSystem hsys;
        if (starInSector(HOME_SX, HOME_SY, HOME_SZ, hs, true)) {
            hsys.generate(hs);
            int hb = -1; for (const Body& b : hsys.bodies) if (KINDS[b.type].landable && b.parent < 0 && b.type != PT_COMET) { hb = b.index; break; }
            const int blocked0 = game.testMapStats().blocked; const double blockedMs0 = game.testMapStats().blockedMs;
            game.testParkAt(hs, hb);   // the landing map opens on the home world
            for (int i = 0; i < 20; i++) stepF();
            printf("landmap      opened on %s: %d blocking map calls (%.0f ms)\n", hsys.bodies[hb].name.c_str(), game.testMapStats().blocked - blocked0, game.testMapStats().blockedMs - blockedMs0);
            auto shiftRight = [&]() { inF.down[KEY_LEFT_SHIFT] = true; inF.down[KEY_RIGHT] = true; stepF(); };
            rowF("landmap 4x", 60, shiftRight, 3.5);   // Shift+Right held: six degrees a frame; F-11: 2.29 after F-10
            inF.down[KEY_LEFT_SHIFT] = false; inF.down[KEY_RIGHT] = false;
            writePNG("shots/tests/bench_landmap.png", game.output(), FBW, FBH);
            const int zb0 = game.testZoomBuilds();
            const double tz0 = nowSec(); pressF(KEY_Z); const double zMs = (nowSec() - tz0) * 1000;
            const double* zp = game.testZoomBuildMs();
            printf("landzoom     Z: a frame of %.1f ms (the build: samples %.1f, colours %.1f, roads %.1f, plan ways %.1f, places %.1f ms)\n", zMs, zp[0], zp[1], zp[2], zp[3], zp[4]);
            budget("landzoom Z", zMs, 22);   // 14.45 after F-10
            rowF("landzoom 4x", 120, shiftRight, 3.9);   // Shift+Right held: 0.05 degrees a frame, the window built again as the cursor leaves three quarters of it; F-11: 2.56 after F-10
            inF.down[KEY_LEFT_SHIFT] = false; inF.down[KEY_RIGHT] = false;
            printf("             %d builds in the 120 frames, the last: samples %.1f, colours %.1f, roads %.1f, plan ways %.1f, places %.1f ms\n", game.testZoomBuilds() - zb0 - 1, zp[0], zp[1], zp[2], zp[3], zp[4]);
            writePNG("shots/tests/bench_landzoom.png", game.output(), FBW, FBH);
            pressF(KEY_Z); pressF(KEY_ESCAPE); for (int i = 0; i < 10; i++) stepF();   // to space, parked
            if (game.state == GameState::SPACE) {
                pressF(KEY_B); for (int i = 0; i < 10; i++) stepF();   // the radar camera
                rowF("radar 4x", 60, stepF, 3.8);   // 2.52 after F-10
                writePNG("shots/tests/bench_radar.png", game.output(), FBW, FBH);
                pressF(KEY_B); for (int i = 0; i < 5; i++) stepF();
                // a recorded frame (one PNG a frame) and a photo's PNG alone
                game.moviesDir = "shots/tests/test_movies"; game.recFixedTake = true;
                game.toggleRecording();
                double recMs = 0;
                rowF("recording 4x", 30, [&]() { stepF(); const double t0 = nowSec(); game.recordFrame(); recMs += (nowSec() - t0) * 1000; }, 0);   // the writer's pace: no budget
                game.toggleRecording();
                const double tq = nowSec(); pngFlush(); const double queueMs = (nowSec() - tq) * 1000;   // the writer's queue out before the rows below
                double pngMs = 1e9; for (int i = 0; i < 5; i++) { const double t0 = nowSec(); writePNG("shots/tests/bench_photo.png", game.output(), FBW, FBH); pngMs = std::min(pngMs, (nowSec() - t0) * 1000); }
                printf("             the recorder's PNG %.1f ms a frame on the main thread (F-02: the copy for the writer's thread; its queue out %.1f ms after); a photo's PNG at %dx%d %.1f ms (the best of five)\n", recMs / 30, queueMs, FBW, FBH, pngMs);
                budget("photo PNG", pngMs, 11);   // 7.2 after F-10
                // the telescope at its deepest on the home world (the 2x row's view at 4x)
                game.testAimAtBody(game.testParkedBody()); pressF(KEY_Z); for (int i = 0; i < 9; i++) pressF(KEY_EQUAL);
                rowF("telescope 4x", 60, stepF, 28);   // 18.33 after F-10
                pressF(KEY_Z); for (int i = 0; i < 5; i++) stepF();
                // the decoder with a world's records listed (the flow's world)
                for (int i = 0; i < 12; i++) game.guide.shards.insert("151,0,25/1/S" + std::to_string(i));
                game.testCabinGoto(0.0, -0.8, PI, -0.65); for (int i = 0; i < 5; i++) stepF();
                pressF(KEY_E); for (int i = 0; i < 10; i++) stepF();
                if (game.state == GameState::SHARDS) { rowF("decoder 4x", 60, stepF, 6.4); writePNG("shots/tests/bench_decoder.png", game.output(), FBW, FBH); }   // 4.22 after F-10
                else printf("decoder 4x   not opened (state %d)\n", (int)game.state);
                pressF(KEY_ESCAPE); pressF(KEY_ESCAPE); for (int i = 0; i < 5; i++) stepF();
            } else printf("radar 4x     not in space (state %d)\n", (int)game.state);
        }
        {   // the sun in the view at 4x (the veil's disc): the shore scene, facing the sun ten degrees under it, and facing away
            const int saved = g_testScale; setFramebufferScale(4);
            SpaceRenderer srS; StarNeighborhood nbS; StarSystem sysS; SurfaceView svS; double tS = 0;
            if (setupSceneForType(PT_FELISIAN, 12, 30 * DEG, 0.0, 0.0, svS, sysS, nbS, tS, MAT_SAND)) {
                const SunInfo si = svS.site.sun(tS);
                Framebuffer fbS; Input inS;
                for (int k = 0; k < 2; k++) {
                    svS.player.yaw = k == 0 ? si.azimuth : wrap2pi(si.azimuth + PI); svS.player.pitch = k == 0 ? std::max(0.0, si.altitude - 10 * DEG) : 0.0;
                    for (int i = 0; i < 3; i++) { svS.update(1.0 / 30, inS, tS, false); svS.render(fbS, tS, nbS.stars, srS, 1.0); fbS.mush(2); }
                    const double t0 = nowSec();
                    for (int i = 0; i < 30; i++) { svS.update(1.0 / 30, inS, tS + i * 0.033, false); svS.render(fbS, tS + i * 0.033, nbS.stars, srS, 1.0); fbS.mush(2); }
                    const double msS = (nowSec() - t0) * 1000 / 30;
                    printf("%-12s %.2f ms/frame (the sun %.0f deg up; sky %.1f, terrain %.1f, the rest with the veil %.1f ms; F-01)\n", k == 0 ? "sunward 4x" : "sunaway 4x", msS,
                           si.altitude / DEG, svS.lastRenderMs[0], svS.lastRenderMs[2], svS.lastRenderMs[5]);
                    budget(k == 0 ? "sunward 4x" : "sunaway 4x", msS, k == 0 ? 8.5 : 6.2);   // 5.66 and 4.10 after F-10
                    if (k == 0) saveFB(fbS, "shots/tests/bench_sunward.png");
                }
                {   // a sprint at 4x facing away from the sun: Shift and W held for a second, then let go (the FOV kick rebuilds the direction table)
                    double sum = 0, worst = 0, base = 0; int over = 0;
                    std::vector<double> fr;
                    for (int i = 0; i < 120; i++) {
                        Input inR; inR.down[KEY_W] = i < 60; inR.down[KEY_LEFT_SHIFT] = i < 60;
                        const double t0 = nowSec(); svS.update(1.0 / 60, inR, tS + 1 + i / 60.0, true); svS.render(fbS, tS + 1 + i / 60.0, nbS.stars, srS, 1.0); fbS.mush(2);
                        fr.push_back((nowSec() - t0) * 1000);
                    }
                    std::vector<double> sorted = fr; std::sort(sorted.begin(), sorted.end()); base = sorted[sorted.size() / 2];
                    for (double v : fr) { sum += v; worst = std::max(worst, v); if (v > base + 1.5) over++; }
                    printf("sprint 4x    %.2f ms/frame over 120 (a second's sprint and its stop), median %.2f, worst %.1f ms, %d frames over the median + 1.5 ms (F-01)\n", sum / 120, base, worst, over);
                    budget("sprint 4x", sum / 120, 5.7);   // 3.77 after F-10
                }
            }
            setFramebufferScale(saved);
        }
        {   // the test city at 4x from its edge (the 2x row's first view)
            const int saved = g_testScale; setFramebufferScale(4);
            SpaceRenderer srC; StarNeighborhood nbC; StarSystem sysC; SurfaceView svC; double tC = 0;
            if (setupSceneForType(PT_DESERT, 15, 40 * DEG, 1.3, 0.0, svC, sysC, nbC, tC, SCENE_TOWN)) {
                svC.palettes[0] = testPalette(); svC.flocks.clear(); svC.critters.clear();
                const double yaw = svC.player.yaw, cx = svC.player.x + std::sin(yaw) * 1250, cz = svC.player.z + std::cos(yaw) * 1250;
                placeCity(svC, cx, cz, 0xC17E5EEDULL);
                formsCamera(svC, 12.0, cx, svC.site.surfaceHeight(cx, cz) + 30, cz, tC);
                Framebuffer fbC; Input inC;
                for (int i = 0; i < 5; i++) { svC.update(1.0 / 30, inC, tC, false); svC.render(fbC, tC, nbC.stars, srC, 1.0); fbC.mush(2); }
                const double t0 = nowSec();
                for (int i = 0; i < 30; i++) { svC.update(1.0 / 30, inC, tC + i * 0.033, false); svC.render(fbC, tC + i * 0.033, nbC.stars, srC, 1.0); fbC.mush(2); }
                const double msC = (nowSec() - t0) * 1000 / 30;
                printf("city 4x      %.2f ms/frame from its edge (the forms %.2f ms; F-01)\n", msC, svC.lastRuinMs);
                budget("city 4x", msC, 8.3);   // 5.52 after F-10
            }
            setFramebufferScale(saved);
        }
        {   // the probe at 4x over the deck (the 2x row's first view)
            Star ps;
            if (starInSector(150, 0, 106, ps, true)) {
                game.settings.renderScale = 4; game.applySettings();
                game.testParkAt(ps, 5);
                for (int i = 0; i < 10; i++) stepF();
                pressF(KEY_C);
                if (game.testProbeActive()) {
                    const double endC = game.testProbeEndClock(); double fromHaze = -1, toHaze = -1;
                    for (double c = 0; c <= endC; c += 0.25) if (game.testProbeStageAt(c) == PS_HAZE) { if (fromHaze < 0) fromHaze = c; toHaze = c; }
                    game.testProbeSkip(fromHaze + (toHaze - fromHaze) * 0.8);
                    for (int i = 0; i < 5; i++) stepF();
                    rowF("probe 4x", 60, stepF, 13);   // 8.66 after F-10
                } else printf("probe 4x     no probe launched ('%s')\n", game.testStatus().c_str());
            }
        }
        {   // an arrival at a star never seen, at 4x: its maps generate on their threads while the frames go on
            Star as; bool found = false;
            for (int64_t x = HOME_SX + 40; x < HOME_SX + 80 && !found; x++) found = starInSector(x, HOME_SY, HOME_SZ + 7, as);
            if (found) {
                game.settings.renderScale = 4; game.applySettings();
                const SpaceRenderer::MapStats m0 = game.testMapStats();
                game.testArriveAt(as); game.setState(GameState::SPACE);
                double first = 0, sum = 0, worst = 0;
                for (int i = 0; i < 120; i++) { const double t0 = nowSec(); stepF(); const double ms = (nowSec() - t0) * 1000; if (i == 0) first = ms; sum += ms; worst = std::max(worst, ms); }
                const SpaceRenderer::MapStats& m1 = game.testMapStats();
                printf("arrival 4x   first frame %.1f ms, %.2f ms/frame over 120, worst %.1f ms; %d maps generated, %d blocking map calls (%.0f ms) (F-01)\n", first, sum / 120, worst,
                       m1.generated - m0.generated, m1.blocked - m0.blocked, m1.blockedMs - m0.blockedMs);
                budget("arrival 4x", sum / 120, 6.6);   // 4.38 after F-10 (the first frame without one: 11.8-20.3 by its maps' threads)
            }
        }
        {   // the guide's save and load at the player's size (every logged event saves it: a sector crossed, a sighting, a mark, a photo, the autosave)
            Guide G; fillBenchGuide(G);
            const std::string gp = "shots/tests/bench_guide_big.txt";
            double save = 1e9, load = 1e9;
            for (int i = 0; i < 5; i++) { const double t0 = nowSec(); G.save(gp); save = std::min(save, (nowSec() - t0) * 1000); }
            for (int i = 0; i < 3; i++) { Guide L; const double t0 = nowSec(); L.load(gp); load = std::min(load, (nowSec() - t0) * 1000); }
            struct stat st; stat(gp.c_str(), &st);
            printf("guide        a save %.1f ms, a load %.1f ms (the best of five and of three; %lld bytes of the player's composition; F-01)\n", save, load, (long long)st.st_size);
            budget("guide save", save, 1.5);   // 0.95 after F-10 (every logged event saves it)
            budget("guide load", load, 23);    // 14.85
            remove(gp.c_str());
        }
        game.settings.renderScale = 1; game.applySettings();
    }
}

void Bench::budgets() {
    budget("space 2x", space2x, 3.1);           // 2.02 after F-10 (F-11: 12 -> 3.1)
    budget("space 4x", space4x, 9.5);           // 6.33 after F-10 (a still camera: the backdrop kept)
    budget("vimana 2x", vimana2x, 2.0);         // a flight's frame, the band from its keyframes; 1.32 after F-10 (F-11: 12 -> 2)
    budget("telescope 2x", telescope2x, 13);    // the deepest magnification, the globe over the whole frame; 8.64 after F-10 (F-11: 16 -> 13)
    budget("probe 2x", probe2x, 8.9);           // the descent's camera over the deck and between the decks; 5.89 after F-10 (F-11: 16 -> 8.9)
    budget("surface 2x", surface2x, 2.8);       // 1.82 after F-10 (F-11: 16 -> 2.8)
    budget("surface 4x", surface4x, 4.5);       // 2.97 after F-10
    budget("descent first", descentFirst, 26);   // the four rings' first fill reads the drainage (12000 samples, +15 ms); the frame is under the fade-in. F-03: 40 -> 22 ms (the fill, the textures and the meso tiles on the pool), the budget 80 -> 35; F-11: 16.95 after F-10, 35 -> 26
    budget("descent people", descentPeople, 62);   // onto a people's world beside its capital, its plan built at the parking. F-03: 79 -> 45 ms, the budget 80 -> 70; F-11: 41.1 after F-10 (51.7 in one run of four), 70 -> 62
    budget("capital 2x", capital2x, 5.4);       // the modern test world's capital from its edge; 3.58 after F-10 (F-11: 16 -> 5.4)
    budget("sprint worst", runWorst, 10);       // the sprint's worst frame shows a cache's stall; 1.7 after F-10, 6.6 in one run of eight by the machine alone (F-11: 40 -> 10)
}

int runBench(bool check, double slack) {
    Bench b;
    b.slack = slack;
    b.space();
    b.descent();
    b.surface();
    if (!check) return 0;
    b.forest();
    b.lamp();
    b.city();
    b.herd();
    b.drive();
    b.belt();
    b.vimana();
    b.probe();
    b.people();
    b.unbudgeted();
    b.budgets();
    return b.fails;
}

// F-06: `pool`: the pool's dispatch timed at its thread count (an empty one after a rest and back to back; 800 rows of uneven work
// against one thread), then its dealing checked at 2 to 8 threads: thousands of dispatches of random sizes and rests visit every index
// once (`parallelFor` with its own chunk and a set one, `parallelBands` in exactly its bands), a dispatch inside a dispatch runs alone in
// one call, and a second thread dispatching beside the main thread loses nothing. Run it in a `-fsanitize=thread` build too
int runPool() {
    int fails = 0;
    auto check = [&](bool ok, const std::string& what, const std::string& detail) { printf("%s %s: %s\n", ok ? "ok  " : "FAIL", what.c_str(), detail.c_str()); if (!ok) fails++; };
    {
        auto empty = [](int, int) {};
        for (int i = 0; i < 200; i++) parallelFor(8, 1, empty);
        double rested = 0, burst = 0; const int R = 200;
        for (int r = 0; r < R; r++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            double t0 = nowSec(); parallelFor(8, 1, empty); rested += nowSec() - t0;
            t0 = nowSec(); for (int k = 0; k < 15; k++) parallelFor(8, 1, empty); burst += nowSec() - t0;
        }
        printf("pool: %d threads; an empty dispatch after a rest of 2 ms %.1f us, fifteen back to back %.1f us each\n", parallelThreads(), rested / R * 1e6, burst / R / 15 * 1e6);
        // rows of uneven work (a quarter of them four times the rest, in one block, as a globe's middle rows): the pool against one thread
        auto rows = [](int b, int e) { for (int y = b; y < e; y++) { const double until = nowSec() + (y >= 300 && y < 500 ? 16e-6 : 4e-6); while (nowSec() < until) {} } };
        double ws = 1e9, one = 1e9;
        for (int r = 0; r < 20; r++) {
            double t0 = nowSec(); parallelFor(800, 32, rows); ws = std::min(ws, nowSec() - t0);
            t0 = nowSec(); rows(0, 800); one = std::min(one, nowSec() - t0);
        }
        printf("pool: 800 rows of 4 and 16 us: %.2f ms (one thread %.2f ms, %.2f of %d threads' ideal)\n", ws * 1e3, one * 1e3, one / parallelThreads() / ws, parallelThreads());
    }
    const int saved = parallelThreadsOverride();
    uint64_t rs = 0x9E3779B97F4A7C15ULL;
    auto rnd = [&](int n) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return n > 0 ? (int)(rs % (uint64_t)n) : 0; };
    const int NMAX = 3000;
    std::vector<std::atomic<int>> hits(NMAX);
    for (int t : {2, 3, 4, 6, 8}) {
        parallelThreadsOverride() = t;
        long dispatches = 0, bad = 0;
        std::atomic<long> badLayout{0};
        std::string first;
        for (int it = 0; it < 20000; it++) {
            const int n = rnd(NMAX), mpt = 1 + rnd(64), kind = rnd(3), chunk = kind == 1 ? 1 + rnd(100) : 0, bands = 1 + rnd(40);
            for (int i = 0; i < n; i++) hits[i].store(0, std::memory_order_relaxed);
            if (kind < 2) parallelFor(n, mpt, [&](int b, int e) { for (int i = b; i < e; i++) hits[i].fetch_add(1, std::memory_order_relaxed); }, chunk);
            else {
                const int k = std::max(1, std::min(bands, n)), band = (n + k - 1) / k;
                parallelBands(n, bands, [&](int b, int e) {
                    if (band <= 0 || b % band != 0 || e != std::min(n, b + band)) badLayout++;
                    for (int i = b; i < e; i++) hits[i].fetch_add(1, std::memory_order_relaxed);
                });
            }
            for (int i = 0; i < n; i++)
                if (hits[i].load(std::memory_order_relaxed) != 1) {
                    if (first.empty()) first = fmt("%s over %d (min %d, chunk %d, bands %d): index %d visited %d times", kind < 2 ? "parallelFor" : "parallelBands", n, mpt, chunk, bands, i, hits[i].load());
                    bad++; break;
                }
            dispatches++;
            if (rnd(40) == 0) std::this_thread::sleep_for(std::chrono::microseconds(rnd(400)));   // now and then a rest: the workers fall asleep
        }
        check(bad == 0 && badLayout == 0, fmt("dealing at %d threads", t), fmt("%ld dispatches, %ld with an index not visited once, %ld bands off the layout%s%s", dispatches, bad, badLayout.load(), first.empty() ? "" : "; ", first.c_str()));
    }
    parallelThreadsOverride() = 8;
    {   // a dispatch inside a dispatch: alone, in one call over the whole range
        const int O = 64, I = 512;
        std::vector<std::atomic<int>> h(O * I);
        std::atomic<int> innerCalls{0}, innerWhole{0};
        parallelFor(O, 1, [&](int b, int e) {
            for (int i = b; i < e; i++)
                parallelFor(I, 1, [&](int ib, int ie) { innerCalls++; if (ib == 0 && ie == I) innerWhole++; for (int j = ib; j < ie; j++) h[(size_t)i * I + j]++; });
        });
        int once = 0;
        for (auto& x : h) once += x.load() == 1;
        check(once == O * I && innerCalls == O && innerWhole == O, "a dispatch inside a dispatch", fmt("%d of %d indices once, %d inner calls (%d whole) for %d inner dispatches", once, O * I, innerCalls.load(), innerWhole.load(), O));
    }
    {   // two threads dispatching at once: one holds the pool, the other runs alone
        const long c0 = pool::contended();
        std::atomic<long> badB{0};
        auto loop = [&](int N) {
            std::vector<std::atomic<int>> hb(N);
            for (int it = 0; it < 3000; it++) {
                for (auto& x : hb) x.store(0, std::memory_order_relaxed);
                parallelFor(N, 16, [&](int b, int e) { for (int i = b; i < e; i++) hb[i].fetch_add(1, std::memory_order_relaxed); });
                for (auto& x : hb) if (x.load(std::memory_order_relaxed) != 1) { badB++; break; }
            }
        };
        std::thread other([&] { loop(1500); });
        loop(2000);
        other.join();
        check(badB == 0, "two threads dispatching", fmt("6000 dispatches, %ld with an index not visited once; %ld ran alone beside the other's", badB.load(), pool::contended() - c0));
    }
    parallelThreadsOverride() = saved;
    printf("pool: %d failures\n", fails);
    return fails;
}

// F-01: `bench noise`: the noise's cost a call (the best of three passes over a million points at a planet's coordinates) and the bits of
// its results hashed, so F-07's gradient table and prefix hashing are judged by their time and by the same hashes (any other hash is a bug)
int runBenchNoise() {
    const int N = 1000000;
    std::vector<Vec3> pts(N), shell(N);
    Rng r(0xB0B5EEDULL);
    for (int i = 0; i < N; i++) {
        pts[i] = Vec3(r.range(-3000, 3000), r.range(-3000, 3000), r.range(-3000, 3000));
        Vec3 u(r.range(-1, 1), r.range(-1, 1), r.range(-1, 1));
        if (length(u) < 1e-6) u = Vec3(1, 0, 0);
        shell[i] = normalize(u) * 6000.0;   // km on a world of 6,000 km
    }
    const uint64_t seed = 0x5EED5EEDULL;
    std::vector<double> out;
    auto hashOf = [&]() { return fnv(1469598103934665603ULL, out.data(), out.size() * sizeof(double)); };
    auto row = [&](const char* name, int per, const std::function<void()>& pass) {
        double best = 1e9;
        for (int k = 0; k < 3; k++) { out.assign((size_t)N * per, 0.0); const double t0 = nowSec(); pass(); best = std::min(best, nowSec() - t0); }
        printf("%-14s %6.1f ns a call (best of three passes over %d points), results' bits %016llx\n", name, best / N * 1e9, N, (unsigned long long)hashOf());
    };
    row("gnoise3", 1, [&]() { for (int i = 0; i < N; i++) out[i] = gnoise3(pts[i].x, pts[i].y, pts[i].z, seed); });
    row("gnoise3d", 4, [&]() { for (int i = 0; i < N; i++) { Vec3 g; out[4 * (size_t)i] = gnoise3d(pts[i].x, pts[i].y, pts[i].z, seed, g); out[4 * (size_t)i + 1] = g.x; out[4 * (size_t)i + 2] = g.y; out[4 * (size_t)i + 3] = g.z; } });
    row("worley3", 7, [&]() {
        for (int i = 0; i < N; i++) {
            const Worley3 w = worley3(pts[i] * 0.1, seed);
            double* o = &out[7 * (size_t)i];
            o[0] = w.f1; o[1] = w.f2; std::memcpy(&o[2], &w.id1, 8); std::memcpy(&o[3], &w.id2, 8); o[4] = w.c1.x; o[5] = w.c1.y; o[6] = w.c1.z;
        }
    });
    row("features", 1, [&]() { for (int i = 0; i < N; i++) out[i] = benchFeatureField(shell[i], 6000.0, seed); });   // a dome field: `visitFeatures`' 27 cells
    return 0;
}

int runLoad(int argc, char** argv) {
    const std::string what = argc > 2 ? argv[2] : "landing";
    const double secs = argc > 3 ? atof(argv[3]) : 60.0;
    const int scale = argc > 4 ? atoi(argv[4]) : 4;
    TestGame game;
    game.savePrefix = "shots/tests/load_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/load_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
    remove(game.guidePath.c_str());
    Input in;
    const double tick = 1.0 / 60;
    double next = nowSec();
    auto paced = [&]() { next += tick; const double wait = next - nowSec(); if (wait > 0) std::this_thread::sleep_for(std::chrono::duration<double>(wait)); else next = nowSec(); };
    auto frame = [&]() { const double t0 = nowSec(); game.frame(in, tick); in.newFrame(); const double ms = (nowSec() - t0) * 1000; paced(); return ms; };
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; const double ms = frame(); in.down[key] = false; return ms; };
    press(KEY_ENTER);
    game.settings.renderScale = scale; game.applySettings();
    for (int i = 0; i < 30; i++) frame();
    const int mapsBefore = game.testMapStats().generated, blockedBefore = game.testMapStats().blocked;
    const double blockedMsBefore = game.testMapStats().blockedMs;
    bool drive = false;
    if (what == "landing") {
        press(KEY_C);
        const double tw0 = nowSec();
        while (nowSec() - tw0 < 3.0 && !game.testDrainageReady()) frame();
        printf("load landing: the drainage ready %.2f s after the landing map opened (%s)\n", nowSec() - tw0, game.testBodyTypes().c_str());
        in.pressed[KEY_ENTER] = true; in.down[KEY_ENTER] = true;
    } else if (what == "map") {
        press(KEY_C);
    } else if (what == "arrival") {
        Star s; bool found = false;
        for (int64_t x = HOME_SX + 40; x < HOME_SX + 80 && !found; x++) found = starInSector(x, HOME_SY, HOME_SZ + 7, s);
        if (!found) { printf("load arrival: no star found\n"); return 1; }
        game.testArriveAt(s);
    } else if (what == "roads") {   // the modern test world's roads grown on the main thread: the explorer moved two ruin cells (4 km) at a time, ten times
        Star ms; const AgeWorld& mw = AGE_WORLDS[AGE_MODERN];
        if (!starInSector(mw.sx, 0, mw.sz, ms, true)) { printf("load roads: no world\n"); return 1; }
        StarSystem msys; msys.generate(ms);
        const BodyGen mg = BodyGen::make(msys.bodies[mw.body]);
        game.testParkAt(ms, mw.body);
        for (double tw0 = nowSec(); nowSec() - tw0 < 5.0 && !worldPlanBuilt(mg.seed);) frame();
        std::shared_ptr<const WorldPlan> P = worldPlanOf(mg);
        const PlanPlace& cap = P->plans[std::min(mw.which, P->peoples - 1)].places[0];
        game.testLandCursor((cap.lat + (cap.radiusM + 400) / (mg.R * 1000.0)) / DEG, cap.lon / DEG);
        for (double tw0 = nowSec(); nowSec() - tw0 < 3.0 && !game.testDrainageReady();) frame();
        press(KEY_ENTER);
        for (int i = 0; i < 60 * 9 && game.state != GameState::SURFACE; i++) frame();
        for (int i = 0; i < 120; i++) frame();
        printf("load roads: on %s's ground (state %d), %zu road cells\n", msys.bodies[mw.body].name.c_str(), (int)game.state, game.testSurface().site.roadCells.size());
        const double step = 2.05 * 2000.0;   // two ruin cells and a little (`ruinCellLat` is 2 km)
        for (int k = 1; k <= 10; k++) {
            const int rb = game.testSurface().roadsBuilds;
            game.testWalkTo(game.testPlayerX() + step * 0.6, game.testPlayerZ() + step * 0.8);
            const double ms = frame(); double after = 0;
            for (int i = 0; i < 30; i++) after = std::max(after, frame());
            printf("  hop %2d: the roads grown %d time(s), %.1f ms (%zu roads held); that frame %.1f ms, the worst of the next 30 %.1f ms\n", k, game.testSurface().roadsBuilds - rb,
                   game.testSurface().lastRoadsMs, game.testSurface().site.roads.size(), ms, after);
            for (int i = 0; i < 90; i++) frame();
        }
        return 0;
    } else if (what == "drive" || what == "capital") {   // a people's world, 400 m outside its capital's core: `drive` the bench's (modern, desert) at full
        // throttle (the roads grow every two ruin cells); `capital` the industrial test world's (felisian: the trees listed against the buildings), facing the capital
        const int age = argc > 5 ? std::max(0, std::min(AGE_COUNT - 1, atoi(argv[5]))) : (what == "drive" ? AGE_MODERN : AGE_INDUSTRIAL);   // `load capital <s> <scale> <age>`
        Star ms; const AgeWorld& mw = AGE_WORLDS[age];
        if (!starInSector(mw.sx, 0, mw.sz, ms, true)) { printf("load drive: no world\n"); return 1; }
        StarSystem msys; msys.generate(ms);
        const BodyGen mg = BodyGen::make(msys.bodies[mw.body]);
        game.testParkAt(ms, mw.body);
        for (double tw0 = nowSec(); nowSec() - tw0 < 5.0 && !worldPlanBuilt(mg.seed);) frame();
        std::shared_ptr<const WorldPlan> P = worldPlanOf(mg);
        const PlanPlace& cap = P->plans[std::min(mw.which, P->peoples - 1)].places[0];
        game.testLandCursor((cap.lat + (cap.radiusM + 400) / (mg.R * 1000.0)) / DEG, cap.lon / DEG);
        for (double tw0 = nowSec(); nowSec() - tw0 < 3.0 && !game.testDrainageReady();) frame();
        press(KEY_ENTER);
        for (int i = 0; i < 60 * 9 && game.state != GameState::SURFACE; i++) frame();
        for (int i = 0; i < 60; i++) frame();
        if (what == "capital") game.testSurfaceFace(PI, 0.02);   // the capital to the south (the bench's view)
        else if (!game.testDriveSetup()) { printf("load drive: no buggy (state %d)\n", (int)game.state); return 1; }
        else drive = true;
        printf("load %s: on %s's ground (state %d), %s\n", what.c_str(), msys.bodies[mw.body].name.c_str(), (int)game.state, game.testStatus().c_str());
    } else if (what == "probe") {   // the probe's launch from the ringed test giant: the handover (KI-361) and the first stages
        Star ps; if (!starInSector(150, 0, 106, ps, true)) { printf("load probe: no giant\n"); return 1; }
        game.testParkAt(ps, 5);
        for (int i = 0; i < 30; i++) frame();
        in.pressed[KEY_C] = true; in.down[KEY_C] = true;
    } else { printf("load landing|map|arrival|drive|capital|probe|roads [seconds] [scale]\n"); return 1; }
    printf("load %s at %dx, frames paced at 60 a second, %d threads in the pool:\n", what.c_str(), scale, parallelThreads());
    ProcSample p0 = procSample();
    double w0 = nowSec(), first = -1, tapT = 0;
    std::vector<double> all;
    for (int sec = 1; nowSec() - w0 < secs + 1e-9 || sec == 1; sec++) {
        double sum = 0, worst = 0; int frames = 0, threads = 0, running = 0, samples = 0;
        const double end = w0 + sec;
        const int roads0 = game.testSurface().roadsBuilds; double roadsWorst = 0, listWorst = 0;
        while (nowSec() < end) {
            if (what == "map" && nowSec() - tapT >= 0.15) { in.pressed[KEY_RIGHT] = true; in.down[KEY_RIGHT] = true; tapT = nowSec(); }
            if (drive) in.down[KEY_W] = true;
            const int rb = game.testSurface().roadsBuilds;
            const double ms = frame();
            if (what == "capital" && frames == 0 && sec == 1) printf("  the first frame on the ground: %d trees listed (%.2f ms), %d drawn; the forms %.2f ms\n", game.testSurface().lastTreesListed, game.testSurface().lastFloraListMs, game.testSurface().lastTreesDrawn, game.testSurface().lastRuinMs);
            listWorst = std::max(listWorst, game.testSurface().lastFloraListMs);
            if (game.testSurface().roadsBuilds != rb) roadsWorst = std::max(roadsWorst, game.testSurface().lastRoadsMs);
            in.down[KEY_ENTER] = false; in.down[KEY_RIGHT] = false; in.down[KEY_C] = false;
            if (first < 0) first = ms;
            sum += ms; worst = std::max(worst, ms); frames++; all.push_back(ms);
            if (frames % 10 == 1) { const ProcSample q = procSample(); threads += q.threads; running += q.running; samples++; }
        }
        const ProcSample p1 = procSample(); const double w1 = nowSec();
        printf("  %3d s: %.2f cores, %.1f threads (%.1f running), %d frames: mean %.1f ms, worst %.1f ms, state %d, %.0f MB%s\n", sec, (p1.cpuS - p0.cpuS) / (w1 - (w0 + sec - 1)), samples ? (double)threads / samples : 0.0,
               samples ? (double)running / samples : 0.0, frames, frames ? sum / frames : 0.0, worst, (int)game.state, p1.rssMB,
               game.testSurface().roadsBuilds != roads0 ? fmt("; the roads grown %d times, the worst %.1f ms (%.0f km/h)", game.testSurface().roadsBuilds - roads0, roadsWorst, game.testBuggySpeed() * 3.6).c_str() :
               (what == "capital" ? fmt("; the flora's listing %.2f ms at worst, %d trees listed", listWorst, game.testSurface().lastTreesListed).c_str() :
               (drive ? fmt("; %.0f km/h, %.2f km from the start, %zu road cells", game.testBuggySpeed() * 3.6, std::hypot(game.testPlayerX(), game.testPlayerZ()) / 1000, game.testSurface().site.roadCells.size()).c_str() : "")));
        p0 = p1;
        if (nowSec() - w0 >= secs) break;
    }
    std::sort(all.begin(), all.end());
    auto at = [&](double q) { return all.empty() ? 0.0 : all[std::min(all.size() - 1, (size_t)(q * all.size()))]; };
    printf("load %s: the first frame %.1f ms; %zu frames: p50 %.1f, p99 %.1f, max %.1f ms; maps generated %d, blocking map calls %d (%.0f ms)\n", what.c_str(), first, all.size(), at(0.5), at(0.99), all.empty() ? 0.0 : all.back(),
           game.testMapStats().generated - mapsBefore, game.testMapStats().blocked - blockedBefore, game.testMapStats().blockedMs - blockedMsBefore);
    return 0;
}

// F-01: `soak [minutes] [scale]`: a long expedition at 4x, the frames paced at 60 a second, round the stars of the harness's plane: at each
// a parking (the landing map, then the cabin turning for 10 s), a landing at the map's default site and 40 s on the ground walking and
// turning; the resident memory, its peak, the maps and drainage tiles held after each stop. The guide's load at the start is timed too
int runSoak(int argc, char** argv) {
    const double minutes = argc > 2 ? atof(argv[2]) : 30.0;
    const int scale = argc > 3 ? atoi(argv[3]) : 4;
    {
        Guide G; fillBenchGuide(G); G.save("shots/tests/soak_guide_big.txt");
        Guide L; const double t0 = nowSec(); L.load("shots/tests/soak_guide_big.txt"); const double ms = (nowSec() - t0) * 1000;
        struct stat st; stat("shots/tests/soak_guide_big.txt", &st);
        printf("soak: a guide of %lld bytes (the player's composition) loads in %.1f ms\n", (long long)st.st_size, ms);
        remove("shots/tests/soak_guide_big.txt");
    }
    TestGame game;
    game.savePrefix = "shots/tests/soak_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/soak_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
    remove(game.guidePath.c_str());
    Input in;
    const double tick = 1.0 / 60;
    double next = nowSec();
    long frames = 0;
    auto frame = [&]() { game.frame(in, tick); in.newFrame(); frames++; next += tick; const double wait = next - nowSec(); if (wait > 0) std::this_thread::sleep_for(std::chrono::duration<double>(wait)); else next = nowSec(); };
    auto run = [&](double s) { for (int i = 0; i < (int)(s * 60); i++) frame(); };
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; frame(); in.down[key] = false; };
    press(KEY_ENTER);
    game.settings.renderScale = scale; game.applySettings();
    const double w0 = nowSec();
    printf("soak: %.0f minutes at %dx\n", minutes, scale);
    int stop = 0;
    for (int64_t x = HOME_SX + 1; nowSec() - w0 < minutes * 60; x++) {
        Star s; StarSystem sys; if (!systemAt(x, HOME_SY, HOME_SZ + 3, s, sys)) continue;
        int body = -1; for (const Body& b : sys.bodies) if (KINDS[b.type].landable && b.type != PT_COMET && b.radiusKm > 1000) { body = b.index; break; }
        if (body < 0) continue;
        game.testParkAt(s, body); run(2.0);
        press(KEY_ESCAPE); for (int i = 0; i < 600; i++) { in.mouseDx = 3; frame(); }   // the cabin turning
        press(KEY_C); run(1.0); press(KEY_ENTER); run(9.0);   // the landing map, the descent
        for (int i = 0; i < 2400; i++) { in.down[KEY_W] = true; in.mouseDx = (i / 240) % 2 ? 2 : -2; frame(); }   // walking and turning
        in.down[KEY_W] = false;
        press(KEY_Q); run(9.0);   // the ascent
        stop++;
        const ProcSample p = procSample();
        double peakMB = 0;
#if defined(__APPLE__) || defined(__linux__)
        struct rusage ru; getrusage(RUSAGE_SELF, &ru);
#ifdef __APPLE__
        peakMB = ru.ru_maxrss / 1048576.0;   // bytes on macOS
#else
        peakMB = ru.ru_maxrss / 1024.0;      // kilobytes on Linux
#endif
#endif
        printf("  %5.1f min, stop %d (%s): %.0f MB resident, peak %.0f MB, %d threads; %zu maps held, %d drainage tiles; %ld frames\n", (nowSec() - w0) / 60, stop, game.testStarName().c_str(), p.rssMB, peakMB, p.threads,
               game.testMapsHeld(), drainageTilesHeld(), frames);
        fflush(stdout);
    }
    return 0;
}
