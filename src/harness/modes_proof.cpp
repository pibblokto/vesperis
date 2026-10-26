// E-03: the proofs between two builds: `regress` (the frame hashes), `pngdiff`, `bothways`, `suncone`, `screens`
#include "harness/harness.h"
#include "harness/test_worlds.h"
#include "harness/scenes_table.h"
static void appendFrameHashes(std::vector<std::pair<std::string, uint64_t>>& lines, std::vector<std::string>& notes, std::vector<std::string>& differs);
int runRegress(bool bless) {
    const uint64_t FNV0 = 1469598103934665603ULL;
    std::vector<std::pair<std::string, uint64_t>> lines;
    std::vector<std::string> notes;
    int nsys = 0, nmap = 0, nsite = 0;
    for (int64_t x = 150; x < 260 && (nsys < 5 || nmap < 10 || nsite < 3); x++)
        for (int64_t z = 20; z < 120 && (nsys < 5 || nmap < 10 || nsite < 3); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            if (nsys < 5) {
                uint64_t h = FNV0;
                h = fnv(h, s.name.data(), s.name.size()); h = fnv(h, &s.cls, sizeof s.cls); h = fnv(h, &s.radiusKm, 8); h = fnv(h, &s.luminosity, 8);
                for (auto& b : sys.bodies) {
                    h = fnv(h, b.name.data(), b.name.size()); h = fnv(h, &b.type, sizeof b.type); h = fnv(h, &b.radiusKm, 8);
                    h = fnv(h, &b.orbitRadiusKm, 8); h = fnv(h, &b.orbitPeriod, 8); h = fnv(h, &b.rotPeriod, 8); h = fnv(h, &b.axialTilt, 8);
                }
                lines.push_back({"system_" + std::to_string(nsys), h});
                notes.push_back(s.name + " (" + std::to_string(sys.bodies.size()) + " bodies)");
                nsys++;
            }
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (nmap < 10) {
                    PlanetMap m; m.generate(BodyGen::make(b));
                    uint64_t h = FNV0;
                    h = fnv(h, m.height.data(), m.height.size() * sizeof(float));
                    h = fnv(h, m.material.data(), m.material.size()); h = fnv(h, m.albedo.data(), m.albedo.size());
                    h = fnv(h, m.cloud.data(), m.cloud.size()); h = fnv(h, m.veg.data(), m.veg.size());
                    lines.push_back({"map_" + std::to_string(nmap), h});
                    notes.push_back(b.name + " " + KINDS[b.type].name);
                    nmap++;
                }
                if (nsite < 3 && KINDS[b.type].landable) {
                    SurfaceSite site;
                    site.init(&sys, bi, 0.3, 0.7, 0.0);
                    uint64_t h = FNV0;
                    for (int i = 0; i <= 80; i++) {
                        TerrainVertex v = site.sampleAt(-2000 + i * 50.0, 0.0, 16);
                        h = fnv(h, &v.h, 4); h = fnv(h, &v.albedo, 4); h = fnv(h, &v.material, 1);
                        TerrainVertex w = site.sampleAt(0.0, -2000 + i * 50.0, 16);
                        h = fnv(h, &w.h, 4); h = fnv(h, &w.albedo, 4); h = fnv(h, &w.material, 1);
                    }
                    lines.push_back({"site_" + std::to_string(nsite), h});
                    notes.push_back(b.name + " profile at 0.3 rad N 0.7 rad E");
                    nsite++;
                }
            }
        }
    std::vector<std::string> differs;
    appendFrameHashes(lines, notes, differs);
    for (const std::string& d : differs) printf("  %-10s DRAWN DIFFERENTLY by one thread\n", d.c_str());
    const char* path = "tests/regress_baseline.txt";
    if (bless) {
        if (!differs.empty()) { printf("not blessed: a frame one thread draws differently is not a reference\n"); return 1; }
        std::ofstream f(path);
        f << "# vesperis determinism baseline; regenerate with ./vesperis_test regress bless\n";
        for (size_t i = 0; i < lines.size(); i++) f << lines[i].first << " " << std::hex << lines[i].second << std::dec << "  # " << notes[i] << "\n";
        printf("blessed %zu hashes into %s\n", lines.size(), path);
        return 0;
    }
    std::ifstream f(path);
    if (!f) { printf("no baseline at %s: run ./vesperis_test regress bless\n", path); return 1; }
    std::vector<std::pair<std::string, uint64_t>> base;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::string key; uint64_t v = 0;
        std::istringstream is(line);
        is >> key >> std::hex >> v;
        base.push_back({key, v});
    }
    int changed = 0, missing = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        const uint64_t* bv = nullptr;
        for (auto& b : base) if (b.first == lines[i].first) bv = &b.second;
        if (!bv) { printf("  %-10s new (no baseline)  %s\n", lines[i].first.c_str(), notes[i].c_str()); missing++; }
        else if (*bv != lines[i].second) { printf("  %-10s CHANGED %016llx -> %016llx  %s\n", lines[i].first.c_str(), (unsigned long long)*bv, (unsigned long long)lines[i].second, notes[i].c_str()); changed++; }
    }
    const bool fail = changed || missing || !differs.empty();
    printf("regress: %zu hashes, %d changed, %d without baseline, %zu drawn differently by one thread -> %s\n", lines.size(), changed, missing, differs.size(), fail ? "FAIL" : "PASS");
    return fail ? 1 : 0;
}
// M7-04 frame hashes (defined after the scene helpers it uses)
static void appendFrameHashes(std::vector<std::pair<std::string, uint64_t>>& lines, std::vector<std::string>& notes, std::vector<std::string>& differs) {
    const uint64_t FNV0 = 1469598103934665603ULL;
    // M7-04 frame hashes: three fixed surface scenes and one space frame at 1x (the framebuffer indices, before the RGB conversion); F-11:
    // the forest at 4x and the cabin at 1x, each also drawn by one thread (`differs` names a frame one thread draws differently)
    {
        int savedScale = g_testScale;
        setFramebufferScale(1);
        SpaceRenderer sr; StarNeighborhood nb;
        const char* want[3] = {"felisian_shore", "cratered_noon", "molten_night"};
        for (const char* name : want)
            for (const CmpScene& sc : CMP_SCENES) {
                if (std::string(name) != sc.name) continue;
                StarSystem sys; SurfaceView sv; double t = 0;
                if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) continue;
                for (const Body& b : sys.bodies) if (b.type != PT_COMPANION) sr.mapFor(b);   // sky bodies drawn textured, never mid-generation
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                uint64_t h = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                lines.push_back({std::string("scene_") + name, h});
                notes.push_back(std::string("frame of scene ") + name + " at 1x (" + sys.bodies[sv.site.body].name + ")");
            }
        for (int64_t x = 150; x < 300; x++) {
            bool done = false;
            for (int64_t z = 20; z < 120 && !done; z++) {
                Star s;
                if (!starInSector(x, 0, z, s) || s.cls != STAR_YELLOW) continue;
                StarSystem sys; sys.generate(s);
                double d = std::max(s.radiusKm * STAR_CLASSES[s.cls].firstOrbitMult, STAR_CLASSES[s.cls].minFirstOrbitKm);
                Vec3 shipPos = s.pos + Vec3(0, 0, -d);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = 1234.0; c.shipPos = shipPos; c.cam = cameraBasis(0, 0);
                for (const Body& b : sys.bodies) if (b.type != PT_COMPANION) sr.mapFor(b);
                sr.setupPalette(fb, &sys, -1, -1, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                lines.push_back({"space_S00", fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix))});
                notes.push_back("space frame from the first orbit of " + s.name);
                done = true;
            }
            if (done) break;
        }
        mkdir("shots", 0755); mkdir("shots/tests", 0755);
        {   // F-11: the forest at 4x (the bench's `felisian_forest`, B-423's frame: the flora's bands pinned per scale), then drawn again by
            // the workers and by one thread (B-310's rule within the run; `VESPERIS_THREADS=1 regress` checks it between runs)
            setFramebufferScale(4);
            for (const CmpScene& sc : CMP_SCENES) {
                if (std::string("felisian_forest") != sc.name) continue;
                SpaceRenderer srF; StarNeighborhood nbF; StarSystem sys; SurfaceView sv; double t = 0;
                if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nbF, t, sc.wantMat)) break;
                for (const Body& b : sys.bodies) if (b.type != PT_COMPANION) srF.mapFor(b);
                Framebuffer fb;
                auto draw = [&]() { sv.render(fb, t, nbF.stars, srF, 1.0); fb.mush(2); return fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix)); };
                const uint64_t h = draw();
                saveFB(fb, "shots/tests/regress_forest_4x.png");
                const uint64_t w = draw();
                parallelThreadsOverride() = 1; const uint64_t s = draw(); parallelThreadsOverride() = 0;
                lines.push_back({"scene_felisian_forest_4x", h});
                notes.push_back("frame of scene felisian_forest at 4x (" + sys.bodies[sv.site.body].name + ")");
                if (s != w) differs.push_back("scene_felisian_forest_4x");
                break;
            }
        }
        {   // F-11: the cabin at 1x through the game: the start's parking, the light on and the hull polarised (`screens`' cabin), every
            // map of the system made; the game's picture (the RGB with the HUD) at one instant once it holds still, then by one thread
            TestGame game; game.savePrefix = "shots/tests/regress_save"; game.settingsPath = "shots/tests/regress_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/regress_guide.txt";
            remove(game.guidePath.c_str()); remove(game.settingsPath.c_str());
            game.newGame(); game.setState(GameState::SPACE);
            game.settings.renderScale = 1; game.applySettings();
            Input gi;
            auto run = [&](int frames) { for (int i = 0; i < frames; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); } };
            auto press = [&](int key) { gi.pressed[key] = true; gi.down[key] = true; run(1); gi.down[key] = false; };
            run(9);
            const int lights = game.testCabinLights();
            if (!(lights & 1)) press(KEY_U);
            if (lights & 2) press(KEY_Y);
            run(45);
            const StarSystem& gs = game.testSystem();
            for (const Body& b : gs.bodies) if (b.type != PT_COMPANION) game.testWaitMapOf(b.index);
            auto out = [&]() { return fnv(FNV0, game.output(), (size_t)FBW * FBH * sizeof(uint32_t)); };
            uint64_t h = 0; int still = 0;
            for (int n = 0; n < 200 && still < 10; n++) { game.frame(gi, 0); const uint64_t k = out(); still = n > 0 && k == h ? still + 1 : 0; h = k; std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
            writePNG("shots/tests/regress_cabin_1x.png", game.output(), FBW, FBH);
            parallelThreadsOverride() = 1; game.frame(gi, 0); const uint64_t s = out(); parallelThreadsOverride() = 0;
            const int pb = game.testParkedBody();
            lines.push_back({"cabin_1x", h});
            notes.push_back("the cabin at 1x, the light on, parked at " + (pb >= 0 ? gs.bodies[pb].name : std::string("nothing")));
            if (s != h) differs.push_back("cabin_1x");
            if (still < 10) differs.push_back("cabin_1x (never held still)");
        }
        setFramebufferScale(savedScale);
    }
}

// ---- F-01: the proof tools' modes and the measurements the bench cannot see ----------------------------------------------------------
// `pngdiff <a.png> <b.png> [diff.png]`, or `pngdiff <dirA> <dirB>` (every PNG of A against B's of the same name, the differing ones listed):
// the exit code is the number of pictures that differ or are missing, 0 when every pair is the same picture
int runPngDiff(int argc, char** argv) {
    if (argc < 4) { printf("pngdiff <a.png> <b.png> [diff.png]  |  pngdiff <dirA> <dirB>\n"); return 1; }
    auto isDir = [](const char* p) { struct stat st; return stat(p, &st) == 0 && S_ISDIR(st.st_mode); };
    auto one = [](const std::string& pa, const std::string& pb, const char* diffOut, const std::string& label, bool quiet) -> int {
        std::vector<uint32_t> a, b; int wa = 0, ha = 0, wb = 0, hb = 0;
        if (!readPNG(pa.c_str(), a, wa, ha)) { printf("%s: cannot read %s\n", label.c_str(), pa.c_str()); return 1; }
        if (!readPNG(pb.c_str(), b, wb, hb)) { printf("%s: cannot read %s\n", label.c_str(), pb.c_str()); return 1; }
        if (wa != wb || ha != hb) { printf("%s: sizes differ (%dx%d against %dx%d)\n", label.c_str(), wa, ha, wb, hb); return 1; }
        const FrameDiff d = diffFrames(a.data(), b.data(), wa, ha);
        if (d.differ || !quiet) printf("%s: %s\n", label.c_str(), diffLine(d, wa, ha).c_str());
        if (d.differ && diffOut) { writeDiffImage(diffOut, a.data(), b.data(), wa, ha); printf("  the differing pixels in red -> %s\n", diffOut); }
        return d.differ ? 1 : 0;
    };
    if (!isDir(argv[2])) return one(argv[2], argv[3], argc > 4 ? argv[4] : nullptr, argv[2], false);
    std::vector<std::string> names;
    if (DIR* dd = opendir(argv[2])) { while (dirent* e = readdir(dd)) { const std::string n = e->d_name; if (n.size() > 4 && n.compare(n.size() - 4, 4, ".png") == 0) names.push_back(n); } closedir(dd); }
    std::sort(names.begin(), names.end());
    int same = 0, differ = 0, missing = 0;
    for (const std::string& n : names) {
        const std::string pa = std::string(argv[2]) + "/" + n, pb = std::string(argv[3]) + "/" + n;
        if (!fileExists(pb)) { missing++; printf("%s: missing in %s\n", n.c_str(), argv[3]); continue; }
        if (one(pa, pb, nullptr, n, true)) differ++; else same++;
    }
    printf("pngdiff: %zu pictures in %s: %d the same, %d differ, %d missing in %s\n", names.size(), argv[2], same, differ, missing, argv[3]);
    return differ + missing;
}

// `bothways <scene> [scale]`: a scene of `scene` drawn by the workers and by one thread in one run (`parallelThreadsOverride`), B-310's
// contract (the same picture whatever the threads): the workers' frame twice (one that changes by itself says so), then one thread's,
// then the workers' again; the frames go to shots/tests/bothways_<scene>_<scale>x_{workers,serial}.png and the differing pixels to _diff.png
int runBothWays(int argc, char** argv) {
    const std::string want = argc > 2 ? argv[2] : "felisian_shore";
    if (argc > 3) setFramebufferScale(atoi(argv[3]));
    if (const CmpScene* found = findScene(want)) {
        const CmpScene& sc = *found;
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no body\n"); return 1; }
        const size_t n = (size_t)FBW * FBH;
        std::vector<uint32_t> w1(n), w2(n), s1(n), w3(n);
        double ms[4] = {0, 0, 0, 0};
        auto draw = [&](std::vector<uint32_t>& out, double& msOut) {
            const double t0 = nowSec();
            Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
            if (sv.inBuggy && !sv.chaseCam) sv.cameraFeed(fb, t);
            fb.toRGB(out.data(), 1.0);
            msOut = (nowSec() - t0) * 1000;
        };
        draw(w1, ms[0]); draw(w2, ms[1]);
        const std::string floraW = sv.bandTraceLine(true);
        parallelThreadsOverride() = 1; draw(s1, ms[2]); parallelThreadsOverride() = 0;
        draw(w3, ms[3]);
        const FrameDiff again = diffFrames(w1.data(), w2.data(), FBW, FBH), both = diffFrames(w2.data(), s1.data(), FBW, FBH), back = diffFrames(w2.data(), w3.data(), FBW, FBH);
        const std::string base = fmt("shots/tests/bothways_%s_%dx", sc.name, FB_SCALE);
        writePNG((base + "_workers.png").c_str(), w2.data(), FBW, FBH);
        writePNG((base + "_serial.png").c_str(), s1.data(), FBW, FBH);
        if (both.differ) writeDiffImage((base + "_diff.png").c_str(), w2.data(), s1.data(), FBW, FBH);
        else remove((base + "_diff.png").c_str());
        printf("bothways %s at %dx (%d threads, %.1f / %.1f ms; one thread %.1f ms)\n", sc.name, FB_SCALE, parallelThreads(), ms[1], ms[3], ms[2]);
        printf("  the workers' frame twice: %s; again after one thread's: %s\n", diffLine(again, FBW, FBH).c_str(), diffLine(back, FBW, FBH).c_str());
        printf("  the workers against one thread: %s%s\n", diffLine(both, FBW, FBH).c_str(), both.differ ? fmt(" -> %s_diff.png", base.c_str()).c_str() : "");
        printf("  %s\n", floraW.c_str());
        return again.differ || back.differ || both.differ ? 1 : 0;
    }
    printf("bothways: no scene %s\n", want.c_str());
    return 1;
}

// F-05: `suncone`: the sun drawn alone (`SpaceRenderer::drawSun` on a cleared frame), its direction swept toward the frame's corner
// across the angle where its glow's cone leaves the frame (the farthest pixel centre's angle plus the cone's half-angle, as drawSun
// finds them): 61 steps of 1e-4 and 61 of 1e-6 radians round it, and from half the cone inside the edge to half outside in 101 steps
// (where the glow still shows: near the edge it adds nothing to a pixel), for seven classes, three sizes (the larger put the edge beside or
// behind the camera, where the cone test works) with an atmosphere and without, at 1x and 4x: one hash of every frame and the frames
// that drew a pixel, for two binaries to agree on (the cone test skips a pass that would draw nothing)
int runSunCone() {
    const int saved = FB_SCALE;
    struct Kind { int cls; double glowMul; };
    const Kind kinds[] = {{STAR_YELLOW, 1.0}, {STAR_WHITE_DWARF, 1.1}, {STAR_BLUE_GIANT, 1.8}, {STAR_CARBON, 1.4}, {STAR_WOLF_RAYET, 1.6}, {STAR_PROTOSTAR, 2.0}, {STAR_BLACK_HOLE, 1.0}};
    uint64_t all = 1469598103934665603ULL;
    for (int sc : {1, 4}) {
        setFramebufferScale(sc);
        const Proj pj = Proj::fromHFov(90);
        const double S = FB_SCALE, invf = 1.0 / pj.f;
        const double mx = std::max(std::fabs(0.5 - pj.cx), std::fabs(FBW - 0.5 - pj.cx)), my = std::max(std::fabs(0.5 - pj.cy), std::fabs(FBH - 0.5 - pj.cy));
        const double cornerA = std::atan(std::sqrt(mx * mx + my * my) / pj.f), phi = std::atan2(-(FBH - 0.5 - pj.cy), FBW - 0.5 - pj.cx);
        for (const Kind& k : kinds)
            for (double angR : {0.004, 0.06, 0.2})
            for (int atmo = 0; atmo < 2; atmo++) {
                Star st; st.cls = k.cls; st.seed = 0x5C0E00ULL + k.cls; st.radiusKm = 7e5; st.valid = true;
                const double haze = atmo ? 0.3 : 0.0;
                double edge;   // the half-angle drawSun's passes reach
                if (k.cls == STAR_BLACK_HOLE) edge = std::min(2.6 * std::sqrt(std::max(angR, 0.8 * S * invf)), 80 * DEG);
                else {
                    const double rA = std::max(angR, 0.6 * S * invf), coronaA = std::max(rA * (atmo ? 3.2 : 2.4), 5.0 * S * invf) * k.glowMul;
                    edge = std::min(coronaA * (atmo ? 3.0 : 1.6) * (1.0 + haze), PI - 0.01);
                    if (k.cls == STAR_WOLF_RAYET) { const double ringA = std::max(rA * 5.0, 4.0 * S * invf); edge = std::min(std::max(edge, ringA + 3 * ringA / 7.0), PI - 0.01); }
                    if (k.cls == STAR_PROTOSTAR) edge = std::max(edge, 0.6);   // its dust strip's cone
                }
                uint64_t h = 1469598103934665603ULL; int drew = 0, frames = 0;
                const Vec3 up(0, 1, 0);
                for (int i = -61; i <= 161; i++) {
                    const double th = cornerA + edge + (i < 0 ? (i + 31) * 1e-4 : (i <= 60 ? (i - 30) * 1e-6 : (i - 111) * 0.01 * edge));
                    const Vec3 dir(std::sin(th) * std::cos(phi), std::sin(th) * std::sin(phi), std::cos(th));
                    Framebuffer fb; fb.clear(0); fb.clearDepth();
                    SpaceRenderer::drawSun(fb, st, dir, angR, 100.0, 1.0, 1, atmo != 0, haze, true, pj, &up);
                    frames++;
                    bool any = false; for (Pix p : fb.idx) if (p) { any = true; break; }
                    drew += any ? 1 : 0;
                    h = fnv(h, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                }
                all = fnv(all, &h, sizeof(h));
                printf("suncone %dx class %2d, %.3f rad, %s: %d frames over the edge at %.5f rad (the corner %.5f, the cone %.5f), %d drew a pixel, hash %016llx\n", sc, k.cls, angR,
                       atmo ? "with an atmosphere" : "airless           ", frames, cornerA + edge, cornerA, edge, drew, (unsigned long long)h);
            }
    }
    setFramebufferScale(saved);
    printf("suncone: every frame %016llx\n", (unsigned long long)all);
    return 0;
}

// F-05: `screens [scale ...]`: the game's frames the regress never hashes, as pictures at each scale (1 2 3 4 by default): the cabin (the
// light on and off, the hull glass, the roof, the decoder's desk, the case, the drones' rack), the console, the ship's screen, the help, the
// guide, the radar camera, the landing map whole and zoomed, and on the home world the descent, the ground, the menu, the sector map near
// and wide and the nose camera. Each is drawn at one instant (`frame(in, 0)`) once it has held still for half a second (a world's map or a
// tile arriving changes it), by the workers and by one thread: the workers' frames go to shots/tests/screens/<name>_<scale>x.png, for
// `pngdiff` between two binaries' directories, and a frame one thread draws differently is named. The waits for a thread's work run at one
// instant too, so the clocks reach every picture by the same frames. The times are single frames: a hint, not a bench
int runScreens(int argc, char** argv) {
    std::vector<int> scales;
    for (int i = 2; i < argc; i++) scales.push_back(clampi(atoi(argv[i]), 1, 4));
    if (scales.empty()) scales = {1, 2, 3, 4};
    mkdir("shots/tests", 0755); mkdir("shots/tests/screens", 0755);
    TestGame game; game.savePrefix = "shots/tests/screens_save"; game.settingsPath = "shots/tests/screens_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/screens_guide.txt";
    remove(game.guidePath.c_str()); remove(game.settingsPath.c_str());
    game.newGame(); game.setState(GameState::SPACE);
    const int stepScale = 2;   // the scale the steps between the pictures run at
    game.settings.renderScale = stepScale; game.applySettings();
    Input gi;
    auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
    auto press = [&](int key) { pressKey(game, gi, key); };
    auto out = [&]() { return fnv(1469598103934665603ULL, game.output(), (size_t)FBW * FBH * sizeof(uint32_t)); };
    auto waitDrainage = [&]() { for (int i = 0; i < 900 && !game.testDrainageReady(); i++) { game.frame(gi, 0); std::this_thread::sleep_for(std::chrono::milliseconds(10)); } };
    int shots = 0, differ = 0, unsettled = 0;
    auto capture = [&](const char* name) {
        for (int s : scales) {
            game.settings.renderScale = s; game.applySettings();
            uint64_t last = 0; int still = 0;
            for (int n = 0; n < 200 && still < 10; n++) { game.frame(gi, 0); const uint64_t h = out(); still = n > 0 && h == last ? still + 1 : 0; last = h; std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
            if (still < 10) unsettled++;
            const size_t np = (size_t)FBW * FBH;
            double t0 = nowSec(); game.frame(gi, 0); const double msW = (nowSec() - t0) * 1000;
            const std::vector<uint32_t> w(game.output(), game.output() + np);
            parallelThreadsOverride() = 1; t0 = nowSec(); game.frame(gi, 0); const double msS = (nowSec() - t0) * 1000; parallelThreadsOverride() = 0;
            const FrameDiff d = diffFrames(w.data(), game.output(), FBW, FBH);
            writePNG(fmt("shots/tests/screens/%s_%dx.png", name, s).c_str(), w.data(), FBW, FBH);
            shots++; differ += d.differ ? 1 : 0;
            printf("  %-18s %dx %7.2f ms, one thread %7.2f ms: %s%s%s\n", name, s, msW, msS, d.differ ? "ONE THREAD DIFFERS, " : "the same", d.differ ? diffLine(d, FBW, FBH).c_str() : "", still < 10 ? " (never held still)" : "");
        }
        game.settings.renderScale = stepScale; game.applySettings();
    };
    printf("screens at %s, %d threads in the pool:\n", [&]() { std::string s; for (int k : scales) s += fmt("%s%dx", s.empty() ? "" : " ", k); return s; }().c_str(), parallelThreads());
    run(0.3);
    const int lights = game.testCabinLights();   // the light on, the hull polarised
    if (!(lights & 1)) press(KEY_U);
    if (lights & 2) press(KEY_Y);
    run(1.5); capture("cabin");
    press(KEY_U); run(1.5); capture("cabin_dark");
    press(KEY_Y); run(1.5); capture("cabin_glass_dark");
    press(KEY_U); run(1.5); capture("cabin_glass");
    press(KEY_Y); run(0.5);
    press(KEY_PAGE_UP); run(0.5); capture("roof"); press(KEY_PAGE_DOWN); run(0.3);
    game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); capture("cabin_decoder");   // the flow's: looking down at the decoder's desk
    game.testCabinGoto(-1.1, -1.38, -PI / 2, -0.55); run(0.2); capture("cabin_case");
    game.testCabinGoto(-0.3, 1.435, -PI / 2, -0.38); run(0.2); capture("cabin_rack");
    game.testCabinGoto(-1.2, 0.0, -PI / 2); run(0.2);   // at the GOES console
    press(KEY_E); run(0.1); game.testTypeText("SL 4"); run(0.1); capture("console"); press(KEY_ESCAPE); run(0.1);
    game.testCabinGoto(0.0, 1.3, 0.0); run(0.2); press(KEY_E); run(0.1); capture("shipscreen"); press(KEY_ESCAPE); run(0.1);
    game.testCabinGoto(0.0, 0.0, 0.0); run(0.2);
    press(KEY_H); run(0.1); capture("help"); press(KEY_ESCAPE); run(0.1);
    press(KEY_G); run(0.1); capture("guide"); press(KEY_ESCAPE); run(0.1);
    press(KEY_B); run(0.5); capture("radar"); press(KEY_B); run(0.2);
    const AgeWorld& W = AGE_WORLDS[AGE_MODERN];
    Star st;
    if (starInSector(W.sx, 0, W.sz, st, true)) {   // the modern test world's landing map, whole and zoomed (the zoom's drainage waited for)
        game.testParkAt(st, W.body); run(0.5); capture("landmap");
        if (game.testLandableBody() >= 0) { press(KEY_Z); waitDrainage(); run(0.3); capture("landzoom"); press(KEY_Z); }
        game.setState(GameState::SPACE); run(0.3);
        game.testCabinGoto(-0.3, 1.435, -PI / 2, -0.38); run(0.2);   // the drone bay (C-18's harness): its screen, the drones leaving the rack, outside the window
        press(KEY_E); run(0.1);
        if (game.state == GameState::DRONES) {
            game.testDronesSettle(); run(0.2); capture("bay");
            press(KEY_ENTER); game.testDronesSettle();
            game.setState(GameState::SPACE); game.testCabinGoto(-0.3, 1.435, -PI / 2, -0.38); run(1.1); capture("rack_out");
            game.testAimAtBody(W.body); game.testCabinGoto(-0.6, 0.6, -0.25, -0.05); run(0.05); game.testDronesSkip(4.0); run(0.05); capture("drones_window");
        } else printf("screens: the drone bay not opened (state %d)\n", (int)game.state);
    }
    Star hs;
    if (starInSector(HOME_SX, HOME_SY, HOME_SZ, hs, true)) {   // the home world: the descent, the ground, the menu, the sector map, the nose camera
        StarSystem hsys; hsys.generate(hs);
        int hb = -1; for (const Body& b : hsys.bodies) if (KINDS[b.type].landable && b.parent < 0 && b.type != PT_COMET) { hb = b.index; break; }
        game.testParkAt(hs, hb); run(0.3);
        waitDrainage();
        press(KEY_ENTER); run(2.5); capture("descent"); run(6.5);
        if (game.state == GameState::SURFACE) {
            capture("ground");
            press(KEY_ESCAPE); run(0.1); capture("menu"); press(KEY_ESCAPE); run(0.1);
            press(KEY_N); run(0.3); capture("sectormap");
            for (int i = 0; i < 3; i++) { press(KEY_MINUS); run(0.3); }
            capture("sectormap_wide");
            for (int i = 0; i < 3; i++) press(KEY_EQUAL);
            press(KEY_ESCAPE); run(0.1);
            if (game.testDriveSetup()) { run(0.5); capture("nose"); }
        } else printf("screens: not on the ground (state %d)\n", (int)game.state);
    }
    printf("screens: %d pictures in shots/tests/screens, %d drawn differently by one thread, %d never held still\n", shots, differ, unsettled);
    return differ ? 1 : 0;
}
