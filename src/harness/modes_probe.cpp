// E-03: the probe: `probe` (a descent's layers), `probe giants`, `probe stability`
#include "harness/harness.h"
std::vector<LayerRun> probeLayerRuns(const TestGame& game, double endC) {
    std::vector<LayerRun> runs;
    const GiantAtmosphere& a = game.testProbeAtm();
    for (double c = 0; c <= endC + 0.5; c += 0.25) {
        const int st = game.testProbeStageAt(c);
        const double bar = game.testProbeBarAt(c);
        const int dk = st == PS_DECK1 || st == PS_DECK2 ? giantDeckAt(a, bar) : (st == PS_BETWEEN ? giantDeckAbove(a, bar) : -1);
        if (runs.empty() || runs.back().stage != st || runs.back().deck != dk) {
            LayerRun r; r.stage = st; r.deck = dk; r.from = r.to = c;
            if (dk >= 0 && st != PS_BETWEEN) { r.tag = CLOUD_SPECIES[a.species[dk]].name; r.tag = r.tag.substr(r.tag.find_last_of(' ') + 1); }
            else if (dk >= 0) r.tag = std::to_string(dk + 1);
            runs.push_back(r);
        } else runs.back().to = c;
    }
    return runs;
}
static void printProbeLayers(const TestGame& game, const std::vector<LayerRun>& runs, double aimLat) {
    const GiantAtmosphere& a = game.testProbeAtm();
    for (const LayerRun& r : runs) {
        const double bar = game.testProbeBarAt(r.from);
        const char* name = r.stage >= PS_ABOVE && r.stage <= PS_DEEP && bar > 0 ? giantStageName(a, bar) : PROBE_STAGE_NAMES[r.stage];
        printf("  %-22s from %5.1f s to %5.1f s, %9.6f bar, %s %6.1f km, %5.0f K, wind %+5.0f m/s\n", name, r.from, r.to, bar, giantAltitudeKm(a, std::max(bar, 1e-9)) >= 0 ? "alt  " : "depth",
               std::fabs(bar > 0 ? giantAltitudeKm(a, bar) : 0.0), bar > 0 ? giantTemperatureK(a, bar) : 0.0, giantWindEast(a, aimLat, bar));   // at the aim's latitude
    }
}

// X-01: `probe [<sx> <sy> <sz> <body>] [wav] [chute]`: a giant's air and a probe's fall into it. The giant (the first gas giant of the
// scan when none is given) and its numbers, the stages with their clocks, pressures, heights, temperatures and winds; a `Game`
// parks at it, sends a probe to the ground under the ship and renders a frame in each stage (`shots/tests/probe_<n>_<stage>.png`,
// timed, with the picture's layers and costs under it) and the last frame held; `chute` opens the chute in the upper haze first;
// `night` carries the ship round to the night side first (Shift+Right, R-405); `wav` writes the relay's sound in each stage.
// X-02: `probe stages [night]` does it for the two test giants (X-02's "each stage on two giants": Thilawiane II, pale and
// unringed, and Wailie VI, orange-brown and ringed), with a look up at the rings and the cirrus and a lightning channel, into
// `probe_stages_[night_]<A|B>_<n>_<stage>.png`
int runProbe(int argc, char** argv) {
    bool wav = false, chute = false, night = false, stagesMode = false; std::vector<long long> nums;
    for (int i = 2; i < argc; i++) { std::string a = argv[i]; if (a == "wav") wav = true; else if (a == "chute") chute = true; else if (a == "night") night = true; else if (a == "stages") stagesMode = true; else nums.push_back(atoll(argv[i])); }
    struct Target { Star st; StarSystem sys; int bi = -1; std::string tag; };
    std::vector<Target> targets;
    if (stagesMode) {
        const long long pins[2][4] = {{150, 0, 66, 2}, {150, 0, 106, 5}};
        for (int k = 0; k < 2; k++) { Target t; if (starInSector(pins[k][0], pins[k][1], pins[k][2], t.st, true)) { t.sys.generate(t.st); t.bi = (int)pins[k][3]; t.tag = k ? "B" : "A"; targets.push_back(t); } }
    } else {
        Target t;
        if (nums.size() >= 4) { if (starInSector(nums[0], nums[1], nums[2], t.st, true)) { t.sys.generate(t.st); t.bi = (int)nums[3]; } }
        else {
            for (int64_t x = 150; x < 320 && t.bi < 0; x++) for (int64_t z = 20; z < 140 && t.bi < 0; z++) {
                Star s; if (!starInSector(x, 0, z, s, true)) continue;
                StarSystem ss; ss.generate(s);
                for (int i = 0; i < (int)ss.bodies.size(); i++) if (ss.bodies[i].type == PT_GASGIANT) { t.st = s; t.sys = ss; t.bi = i; break; }
            }
        }
        targets.push_back(t);
    }
    TestGame game;
    game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/test_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
    applyTestScale(game);
    Input in;
    auto run = [&](double seconds) { runFramesAtLeastOne(game, in, seconds); };
    auto press = [&](int key) { pressKey(game, in, key); };
    AudioSynth synth;
    for (Target& tg : targets) {
    Star& st = tg.st; StarSystem& sys = tg.sys; int bi = tg.bi;
    if (bi < 0 || bi >= (int)sys.bodies.size() || !isProbeGiant(sys.bodies[bi].type)) { printf("probe: no giant there\n"); return 1; }
    const Body& b = sys.bodies[bi];
    GiantAtmosphere a = giantAtmosphereOf(sys, bi);
    { RGB mc = BodyGen::make(b).matColor[familyRep(b.type, FAM_ROCK)];
    printf("probe: %s body %d (%lld %lld %lld), %s, R %.0f km, g %.1f m/s2, T(1 bar) %.0f K, wind %.0f m/s at the strongest band, %.1f bands%s, colour %.2f %.2f %.2f\n", sys.star.name.c_str(), bi, (long long)st.sx, (long long)st.sy, (long long)st.sz,
           KINDS[b.type].name, b.radiusKm, a.gravity, a.t1K, a.windPeak, a.bandCount, b.rings ? ", rings" : "", mc.r, mc.g, mc.b); }
    game.testParkAt(st, bi); run(0.3);
    if (night) { in.down[KEY_LEFT_SHIFT] = true; in.down[KEY_RIGHT] = true; run(8.0); in.down[KEY_RIGHT] = false; in.down[KEY_LEFT_SHIFT] = false; run(0.1); printf("  carried round: '%s'\n", game.testStatus().c_str()); }
    press(KEY_C); run(0.1);
    if (!game.testProbeActive()) { printf("probe: C launched nothing: '%s'\n", game.testStatus().c_str()); return 1; }
    printf("  launched: '%s'; %s\n", game.testStatus().c_str(), game.testProbeInfo().c_str());
    // the layers' clocks (X-03: each deck by its cloud, the clear bands between them)
    double endC = game.testProbeEndClock();
    std::vector<LayerRun> runs = probeLayerRuns(game, endC);
    std::vector<double> from(PS_COUNT, -1), to(PS_COUNT, -1);
    for (const LayerRun& r : runs) { if (from[r.stage] < 0) from[r.stage] = r.from; to[r.stage] = r.to; }
    double aimLat = 0, aimLon = 0; game.testProbeAimed(aimLat, aimLon);
    printProbeLayers(game, runs, aimLat);
    struct Shot { double c; std::string name; };
    std::vector<Shot> shots = {{3, "fall_start"}, {24, "fall"}, {PROBE_FALL_S + 5.5, "entry"}, {from[PS_ABOVE] + 6, "above_high"}};
    if (stagesMode) shots = {{PROBE_FALL_S + 10.5, "entry_late"}, {from[PS_ABOVE] + 6, "above_high"}};
    auto mid = [&](int s2, double u) { return from[s2] + (to[s2] - from[s2]) * u; };
    // the crossfade's end and the stages' middles
    for (double c = from[PS_ABOVE]; c < to[PS_ABOVE]; c += 0.25) if (game.testProbeBarAt(c) > 0.06) { shots.push_back({c, "above"}); break; }
    if (stagesMode) shots.push_back({mid(PS_ABOVE, 0.75), "above_sky"});   // looking up: the rings' arc, the moons, the stars over the limb
    if (stagesMode) for (double c = from[PS_HAZE]; c < to[PS_HAZE]; c += 0.25) if (game.testProbeBarAt(c) > std::min(0.34, game.testProbeAtm().top[0] * 0.8)) { shots.push_back({c, "cirrus"}); break; }   // just under the cirrus, looking up at it
    if (from[PS_HAZE] >= 0) { shots.push_back({mid(PS_HAZE, 0.5), "haze"}); if (stagesMode) shots.push_back({mid(PS_HAZE, 0.92), "haze_low"}); }
    bool lit = false;
    for (const LayerRun& r : runs) {
        const double rm = r.from + (r.to - r.from) * 0.5;
        if (r.stage == PS_DECK1 || r.stage == PS_DECK2) {
            shots.push_back({r.from + (r.to - r.from) * (r.stage == PS_DECK1 ? 0.5 : 0.3), "deck_" + r.tag});
            if (r.stage == PS_DECK1) shots.push_back({r.from + (r.to - r.from) * 0.95, "deck_" + r.tag + "_base"});
        } else if (r.stage == PS_BETWEEN) {
            shots.push_back({rm, "between_" + r.tag});
            if (!lit) {   // a flash in the floor, and one with a channel
                for (double c = rm + 0.5; c < r.to; c += 1.0 / 30) if (game.testProbeFlash(c) > 0.3) { shots.push_back({c, "lightning"}); lit = true; break; }
                if (stagesMode) for (double c = r.from + (r.to - r.from) * 0.3; c < r.to; c += 1.0 / 30) if (game.testProbeFlash(c) > 0.3 && game.testProbeBolt(c)) { shots.push_back({c, "bolt"}); break; }
            }
        } else if (r.stage == PS_DEEP) shots.push_back({rm, "deep"});
    }
    std::sort(shots.begin(), shots.end(), [](const Shot& x, const Shot& y) { return x.c < y.c; });
    int k = 0;
    for (const Shot& sh : shots) {
        if (sh.c <= game.testProbeClock()) continue;
        if (chute && sh.c >= mid(PS_HAZE, 0.3) && game.testProbeClock() < mid(PS_HAZE, 0.3)) { game.testProbeSkip(mid(PS_HAZE, 0.3)); game.testProbeChute(); printf("  the chute opened: %s\n", game.testProbeInfo().c_str()); }
        game.testProbeSkip(sh.c - 1.0 / 30);
        bool up = sh.name == "above_sky" || sh.name == "cirrus";
        if (up) game.testProbeLook(0, sh.name == "cirrus" ? 34 * DEG : 48 * DEG);
        auto t0 = std::chrono::steady_clock::now();
        game.frame(in, 1.0 / 30); in.newFrame();
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        if (up) game.testProbeLook(0, 0);
        std::string fn = stagesMode ? fmt("shots/tests/probe_stages_%s%s_%02d_%s.png", night ? "night_" : "", tg.tag.c_str(), k++, sh.name.c_str()) : fmt("shots/tests/probe_%s%02d_%s.png", night ? "night_" : "", k++, sh.name.c_str());
        writePNG(fn.c_str(), game.output(), FBW, FBH);
        printf("  %-12s %.1f ms  %s -> %s\n               %s\n", sh.name.c_str(), ms, game.testProbeInfo().c_str(), fn.c_str(), game.testProbeViewInfo().c_str());
        if (wav) {
            std::vector<int16_t> pcm; AudioState as = game.audio; as.master = 1.0;
            RenderStats r = renderReceiver(synth, as, 4.0, &pcm, [&](double) { as.probeThunder = 0; });
            std::string wf = fmt("shots/tests/probe_%s.wav", sh.name.c_str()); writeWav16(wf, pcm, 22050);
            printf("    sound: rms %.3f peak %.3f clip %d nan %d -> %s\n", r.rms, r.peak, r.clip, r.nan, wf.c_str());
        }
    }
    game.testProbeSkip(game.testProbeEndClock() + 1); run(0.5);
    if (!stagesMode) writePNG(fmt("shots/tests/probe_%s%02d_lost.png", night ? "night_" : "", k).c_str(), game.output(), FBW, FBH);
    printf("  the end: %s; state %d; '%s'\n", game.testProbeInfo().c_str(), (int)game.state, game.testStatus().c_str());
    if (wav) { std::vector<int16_t> pcm; AudioState as = game.audio; as.master = 1.0; RenderStats r = renderReceiver(synth, as, 4.0, &pcm); writeWav16("shots/tests/probe_lost.wav", pcm, 22050); printf("    the hiss: rms %.3f peak %.3f -> shots/tests/probe_lost.wav\n", r.rms, r.peak); }
    press(KEY_ESCAPE); run(0.2);
    printf("  Esc: state %d, probe %s; '%s'\n", (int)game.state, game.testProbeActive() ? "STILL ACTIVE" : "let go", game.testStatus().c_str());
    }
    return 0;
}

// X-03: `probe giants`: the character of the first twenty giants of the scan (the probe mode's, sectors 150-319 by 20-139), then
// a hot young giant, an ice giant and a brown dwarf of the scan rendered at the same stages (over the clouds, the first clear band,
// the deep; the brown dwarf on its night side too: its glow alone), into `probe_giants_<hot|ice|brown>_<stage>.png`, and the
// rare sights where the scan has them (`probe_giants_<storm|aurora|hole|hail|sight>_*.png`): a great storm's wall, the aurora's
// curtains on a night side, a descent down a clear-air hole, diamond hail in the lamp, and the rare giant's sight
int runProbeGiants() {
    struct G { int64_t sx, sz; int bi; GiantAtmosphere a; std::string name; int64_t sy = 0; };
    std::vector<G> all;
    for (int64_t x = 150; x < 320 && all.size() < 600; x++)
        for (int64_t z = 20; z < 140 && all.size() < 600; z++) {
            Star s; if (!starInSector(x, 0, z, s, true)) continue;
            StarSystem ss; ss.generate(s);
            for (int i = 0; i < (int)ss.bodies.size(); i++) if (isProbeGiant(ss.bodies[i].type)) all.push_back({x, z, i, giantAtmosphereOf(ss, i), ss.bodies[i].name});
        }
    printf("probe giants: the first twenty of the scan\n");
    for (int i = 0; i < 20 && i < (int)all.size(); i++) printf("%2d %s (sector %lld 0 %lld, body %d): %s\n", i + 1, all[i].name.c_str(), (long long)all[i].sx, (long long)all[i].sz, all[i].bi, giantCharacterText(all[i].a).c_str());
    int counts[GK_COUNT] = {}, decks[4] = {0, 0, 0, 0}, young = 0, storms = 0, holes = 0, hail = 0, sight = 0;
    for (const G& g : all) { counts[g.a.ch.kind]++; decks[g.a.decks]++; young += g.a.ch.young; storms += g.a.ch.greatStorm; holes += g.a.ch.holes > 0; hail += g.a.ch.hail; sight += g.a.ch.sight; }
    printf("of %zu giants: %d gas giants (%d young), %d ice giants, %d brown dwarfs; 1/2/3 decks %d/%d/%d; a great storm on %d, holes on %d, hail on %d, the sight on %d\n", all.size(), counts[0], young, counts[1], counts[2],
           decks[1], decks[2], decks[3], storms, holes, hail, sight);
    {   // P-07: and the made giants among them, by class
        std::string made;
        for (int k = GK_WATER; k < GK_COUNT; k++) made += fmt("%s%d %s", made.empty() ? "" : ", ", counts[k], GIANT_KIND_NAMES[k]);
        printf("  made: %s\n", made.c_str());
    }
    const G* hot = nullptr; const G* ice = nullptr; const G* bd = nullptr;
    const G* gs = nullptr; const G* au = nullptr; const G* ho = nullptr; const G* ha = nullptr; const G* si = nullptr;
    for (const G& g : all) {
        const GiantCharacter& c = g.a.ch;
        if (c.kind == GK_GAS && c.young && g.a.decks >= 2 && (!hot || c.heat > hot->a.ch.heat)) hot = &g;
        if (c.kind == GK_ICE && g.a.decks >= 2 && !ice) ice = &g;
        if (c.kind == GK_BROWN && g.a.decks >= 2 && !bd) bd = &g;
        if (c.greatStorm && c.kind == GK_GAS && !gs) gs = &g;
        if (c.kind == GK_GAS && (!au || c.aurora > au->a.ch.aurora)) au = &g;
        if (c.holes > 0 && c.kind == GK_GAS && g.a.decks >= 2 && !ho) { GiantDecks D; D.init(g.a, nullptr, 1); if (D.holeable(0)) ho = &g; }
        if (c.hail && !ha) ha = &g;
        if (c.sight && !si) si = &g;
    }
    TestGame game;
    game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/test_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
    applyTestScale(game);
    Input in;
    auto run = [&](double seconds) { runFramesAtLeastOne(game, in, seconds); };
    auto press = [&](int key) { pressKey(game, in, key); };
    // the sub-solar point of a body in its own frame now (its latitude and longitude)
    auto subSolar = [&](int bi, double& lat, double& lon) {
        const StarSystem& sys = game.testSystem(); const double t = game.testGameTime();
        StarSystem::latLonFromBody(normalize(sys.bodyFrame(bi, t) * normalize(sys.star.pos - sys.bodyPos(bi, t))), lat, lon);
    };
    // park at a giant and send a probe (to an aim when one is given)
    auto launch = [&](const G& g, bool aimed, double lat, double lon) {
        Star st; starInSector(g.sx, g.sy, g.sz, st, true);
        if (game.testProbeActive()) game.testProbeRelease();
        game.testParkAt(st, g.bi); run(0.3);
        game.harness.aimOverride = aimed; game.harness.aimLat = lat; game.harness.aimLon = lon;
        press(KEY_C); run(0.1);
        game.harness.aimOverride = false;
        if (!game.testProbeActive()) { printf("  no probe into %s: '%s'\n", g.name.c_str(), game.testStatus().c_str()); return false; }
        printf("%s (sector %lld 0 %lld, body %d): %s\n  %s\n", g.name.c_str(), (long long)g.sx, (long long)g.sz, g.bi, game.testProbeInfo().c_str(), giantCharacterText(game.testProbeAtm()).c_str());
        return true;
    };
    auto shoot = [&](const std::string& tag, double c, double look) {
        if (c < 0) { printf("  %-22s none on this giant\n", tag.c_str()); return; }
        game.testProbeSkip(c - 1.0 / 30);
        if (look != 0) game.testProbeLook(0, look);
        auto t0 = std::chrono::steady_clock::now();
        game.frame(in, 1.0 / 30); in.newFrame();
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        if (look != 0) game.testProbeLook(0, 0);
        std::string fn = "shots/tests/probe_giants_" + tag + ".png";
        writePNG(fn.c_str(), game.output(), FBW, FBH);
        printf("  %-22s %.1f ms %s -> %s\n                         %s\n", tag.c_str(), ms, game.testProbeInfo().c_str(), fn.c_str(), game.testProbeViewInfo().c_str());
    };
    auto runsAt = [&](int stage, double u) { std::vector<LayerRun> runs = probeLayerRuns(game, game.testProbeEndClock()); for (const LayerRun& r : runs) if (r.stage == stage) return r.from + (r.to - r.from) * u; return -1.0; };
    // the same stages on the three kinds
    const G* three[3] = {hot, ice, bd};
    const char* names[3] = {"hot", "ice", "brown"};
    for (int q = 0; q < 3; q++) {
        if (!three[q]) { printf("no %s giant in the scan\n", names[q]); continue; }
        if (!launch(*three[q], false, 0, 0)) continue;
        printProbeLayers(game, probeLayerRuns(game, game.testProbeEndClock()), 0);
        shoot(std::string(names[q]) + "_above", runsAt(PS_ABOVE, 0.75), 0);
        shoot(std::string(names[q]) + "_deck", runsAt(PS_DECK1, 0.5), 0);
        shoot(std::string(names[q]) + "_between", runsAt(PS_BETWEEN, 0.5), 0);
        shoot(std::string(names[q]) + "_deep", runsAt(PS_DEEP, 0.4), 0);
    }
    if (bd) {   // the brown dwarf's night side: its glow alone
        double la, lo; Star st; starInSector(bd->sx, 0, bd->sz, st, true); game.testParkAt(st, bd->bi); run(0.2); subSolar(bd->bi, la, lo);
        if (launch(*bd, true, -0.3 * la + 10 * DEG, lo + PI)) { shoot("brown_night_above", runsAt(PS_ABOVE, 0.75), 0); shoot("brown_night_between", runsAt(PS_BETWEEN, 0.5), 0); }
    }
    if (gs) {   // a great storm's wall: aimed just inside its northern edge, the camera turned to the wall
        Star st; starInSector(gs->sx, 0, gs->sz, st, true);
        StarSystem ss; ss.generate(st);
        const Body& b = ss.bodies[gs->bi]; BodyGen g = BodyGen::make(b);
        game.testParkAt(st, gs->bi); run(0.2);
        for (int q = 0; q < 48; q++) {   // the time the storm's side faces the sun (the giant turned under it by the warp)
            const double tt = game.testGameTime();
            const Vec3 us = StarSystem::bodyFromLatLon(g.gsLat, g.gsLon - giantJetTurn(b, g.bandCount, g.gsLat, tt));
            if (dot(us, normalize(ss.bodyFrame(gs->bi, tt) * normalize(ss.star.pos - ss.bodyPos(gs->bi, tt)))) > 0.6) break;
            game.testAdvanceTime(std::fabs(b.rotPeriod) / 48); run(0.05);
        }
        const double t = game.testGameTime(), lon = wrapAngle(g.gsLon - giantJetTurn(b, g.bandCount, g.gsLat, t));
        if (launch(*gs, true, g.gsLat + (g.gsLat >= 0 ? 1 : -1) * 0.975 * g.gsB, lon)) {
            shoot("storm_above_high", runsAt(PS_ABOVE, 0.3), 0); shoot("storm_above", runsAt(PS_ABOVE, 0.8), 0); shoot("storm_haze", runsAt(PS_HAZE, 0.6), 0);
        }
        if (launch(*gs, true, g.gsLat, lon)) shoot("storm_inside", runsAt(PS_ABOVE, 0.8), 0);
        // the globe from its parking and through the telescope: the oval the probe fell into (GEN 12: the planet function's)
        game.testProbeRelease(); game.testParkAt(st, gs->bi); run(0.3);
        game.testAimAtBody(gs->bi); run(0.1);
        writePNG("shots/tests/probe_giants_globe_storm.png", game.output(), FBW, FBH);
        press(KEY_Z); run(0.3); for (int i = 0; i < 2; i++) { press(KEY_EQUAL); run(0.05); }
        run(0.5); writePNG("shots/tests/probe_giants_globe_storm_tele.png", game.output(), FBW, FBH);
        press(KEY_Z); run(0.1);
        printf("  the globe from its parking and at x8: probe_giants_globe_storm[_tele].png\n");
    }
    if (ice) {   // an ice giant's globe: its faint bands, its few storms
        Star st; starInSector(ice->sx, 0, ice->sz, st, true);
        game.testProbeRelease(); game.testParkAt(st, ice->bi); run(0.3); game.testAimAtBody(ice->bi); run(0.1);
        press(KEY_Z); run(0.3); run(0.5); writePNG("shots/tests/probe_giants_globe_ice_tele.png", game.output(), FBW, FBH); press(KEY_Z); run(0.1);
    }
    if (au) {   // the aurora: on the night side, a little equatorward of the oval round the nearer magnetic pole, looking up toward it
        Star st; starInSector(au->sx, 0, au->sz, st, true); game.testParkAt(st, au->bi); run(0.2);
        double sla, slo; subSolar(au->bi, sla, slo);
        const GiantCharacter& c = au->a.ch;
        Vec3 pole = StarSystem::bodyFromLatLon(PI / 2 - c.magColat, c.magLon), anti = StarSystem::bodyFromLatLon(-sla, slo + PI);
        if (dot(pole, anti) < 0) pole = pole * -1.0;
        const double want = c.ovalRad + 1200 / game.testSystem().bodies[au->bi].radiusKm;   // 1,200 km equatorward of the oval, toward the night
        const Vec3 toward = normalize(anti - pole * dot(anti, pole));
        double la, lo;
        StarSystem::latLonFromBody(normalize(pole * std::cos(want) + toward * std::sin(want)), la, lo);
        if (launch(*au, true, la, lo)) { shoot("aurora_above", runsAt(PS_ABOVE, 0.75), 0); shoot("aurora_up", runsAt(PS_ABOVE, 0.8), 25 * DEG); }
    }
    if (ho) {   // a descent down a clear-air hole: relaunched until one falls through one
        bool fell = false;
        for (int tries = 0; tries < 12 && !fell; tries++) { if (!launch(*ho, false, 0, 0)) break; fell = game.testProbeHolePath(); if (!fell) run(0.5); }
        if (fell) {
            shoot("hole_above", runsAt(PS_ABOVE, 0.85), 0); shoot("hole_haze", runsAt(PS_HAZE, 0.7), 0);
            const GiantAtmosphere& a = game.testProbeAtm();
            double c1 = -1; for (double c = probeDescentStart(); c < game.testProbeEndClock(); c += 0.25) if (game.testProbeBarAt(c) > std::sqrt(a.top[0] * a.base[0])) { c1 = c; break; }
            shoot("hole_shaft", c1, 0); shoot("hole_shaft_up", c1, 60 * DEG);
        } else printf("  no descent of %s fell through a hole\n", ho->name.c_str());
    }
    if (ha && launch(*ha, false, 0, 0)) shoot("hail_deep", runsAt(PS_DEEP, 0.15), 0);
    if (si && launch(*si, false, 0, 0)) { const double sc = game.testProbeSightClock(); shoot("sight", sc, 0); shoot("sight_after", sc > 0 ? sc + 3 : -1, 0); }
    // P-07: a descent into each made class, the first of the scan region (or of the far strips the stream proofs read), on the day side under
    // the sun (a hot giant's locked face), the hot ones on their night side too: `probe_giants_<class>_<stage>.png`
    static const char* const CLASS_TAGS[GK_COUNT] = {"", "", "", "water", "azure", "alkali", "silicate", "dwarf", "puff"};
    for (int k = GK_WATER; k < GK_COUNT; k++) {
        G found; bool got = false;
        auto look = [&](int64_t x, int64_t y, int64_t z) {
            Star s; if (got || !starInSector(x, y, z, s, true)) return;
            StarSystem ss; ss.generate(s);
            for (int i = 0; i < (int)ss.bodies.size() && !got; i++)
                if (isProbeGiant(ss.bodies[i].type) && KINDS[ss.bodies[i].type].giantClass == k - GK_WATER + GC_WATER) { found = {x, z, i, giantAtmosphereOf(ss, i), ss.bodies[i].name, y}; got = true; }
        };
        for (int64_t x = 150; x < 340 && !got; x++) for (int64_t z = 20; z < 160 && !got; z++) look(x, 0, z);
        for (int64_t x = -2000; x < 12000 && !got; x += 37) for (int64_t z = -5500; z < 400 && !got; z += 211) look(x, (x / 37) % 7 - 3, z);
        if (!got) { printf("no %s in the scan\n", GIANT_KIND_NAMES[k]); continue; }
        Star st; starInSector(found.sx, found.sy, found.sz, st, true); game.testParkAt(st, found.bi); run(0.2);
        double la, lo; subSolar(found.bi, la, lo);
        const std::string tag = CLASS_TAGS[k];
        if (!launch(found, true, 0.6 * la, lo + 0.25)) continue;
        printProbeLayers(game, probeLayerRuns(game, game.testProbeEndClock()), 0);
        shoot(tag + "_above", runsAt(PS_ABOVE, 0.75), 0);
        shoot(tag + "_haze", runsAt(PS_HAZE, 0.5), 0);
        shoot(tag + "_deck", runsAt(PS_DECK1, 0.5), 0);
        shoot(tag + "_between", runsAt(PS_BETWEEN, 0.5), 0);
        shoot(tag + "_deep", runsAt(PS_DEEP, 0.4), 0);
        const double endC = game.testProbeEndClock();
        shoot(tag + "_last", endC - 2, 0);
        if (k == GK_ALKALI || k == GK_SILICATE) {   // the night side: the glow alone
            if (launch(found, true, -0.6 * la, lo + PI)) { shoot(tag + "_night_above", runsAt(PS_ABOVE, 0.75), 0); shoot(tag + "_night_last", game.testProbeEndClock() - 2, 0); }
        }
        // the globe from its parking
        game.testProbeRelease(); game.testParkAt(st, found.bi); run(0.3); game.testAimAtBody(found.bi); run(0.1);
        writePNG(("shots/tests/probe_giants_globe_" + tag + ".png").c_str(), game.output(), FBW, FBH);
    }
    return 0;
}

// X-02: `probe stability [scale]`: 16 frames a thirtieth of a second apart at three places of the ammonia deck stage on the
// ringed test giant (its tops rushing up, just inside, the clear band under it), the camera's shake and the link's static
// off: the mean change of a pixel's light per frame and the share that jumps by over 40 (the clouds' own flicker).
// B-409: and a brown dwarf's (Weesies I's) first deck at its base, the fog lit from under it by the deep's glow
int runProbeStability() {
    TestGame game;
    game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/test_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
    applyTestScale(game);
    Input in;
    auto press = [&](int key) { pressKey(game, in, key); };
    auto lum = [](uint32_t c) { return 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255); };
    int bad = 0;
    const long long pins[2][4] = {{150, 0, 106, 5}, {150, 0, 73, 0}};
    for (int g = 0; g < 2; g++) {
    Star st; if (!starInSector(pins[g][0], pins[g][1], pins[g][2], st, true)) { printf("no star\n"); return 1; }
    if (g) game.testProbeRelease();
    game.testParkAt(st, (int)pins[g][3]);
    for (int i = 0; i < 9; i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
    press(KEY_C);
    if (!game.testProbeActive()) { printf("probe stability: no probe\n"); return 1; }
    game.harness.probeQuiet = true;
    double endC = game.testProbeEndClock();
    std::vector<LayerRun> runs = probeLayerRuns(game, endC);
    auto at = [&](int stage, double u) { for (const LayerRun& r : runs) if (r.stage == stage) return r.from + (r.to - r.from) * u; return -1.0; };   // the stage's first run
    struct Spot { double c; const char* name; };
    std::vector<Spot> spots = {{at(PS_HAZE, 0.85), "tops"}, {at(PS_DECK1, 0.12), "inside"}, {at(PS_BETWEEN, 0.4), "between"}};
    if (g) spots = {{at(PS_DECK1, 0.95), "glow"}};
    for (const Spot& sp : spots) {
        if (sp.c < 0) { printf("probe stability %-8s: no such stage on this giant\n", sp.name); continue; }
        game.testProbeSkip(sp.c);
        const int N = 16;
        std::vector<uint32_t> prev, cur((size_t)FBW * FBH);
        std::vector<double> heat((size_t)FBW * FBH, 0);
        double sum = 0; long pops = 0;
        for (int k = 0; k < N; k++) {
            game.frame(in, 1.0 / 30); in.newFrame();
            std::memcpy(cur.data(), game.output(), cur.size() * 4);
            if (!prev.empty()) for (size_t i = 0; i < cur.size(); i++) { double d = std::fabs(lum(cur[i]) - lum(prev[i])); heat[i] += d; sum += d; if (d > 40) pops++; }
            prev = cur;
        }
        double mean = sum / ((double)cur.size() * (N - 1)), popShare = 100.0 * pops / ((double)cur.size() * (N - 1));
        std::vector<uint32_t> sheet((size_t)FBW * 2 * FBH);
        for (int y = 0; y < FBH; y++)
            for (int x = 0; x < FBW; x++) {
                sheet[(size_t)y * FBW * 2 + x] = prev[(size_t)y * FBW + x];
                int v = (int)std::min(255.0, heat[(size_t)y * FBW + x] / (N - 1) * 8.0);
                sheet[(size_t)y * FBW * 2 + FBW + x] = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
            }
        std::string fn = fmt("shots/tests/probe_stability_%s.png", sp.name);
        writePNG(fn.c_str(), sheet.data(), FBW * 2, FBH);
        bool ok = popShare < 0.5;
        if (!ok) bad++;
        printf("probe stability %-8s %s: mean |dL| %.2f a frame, %.3f%% of the pixels jump over 40 -> %s (%s)\n", sp.name, game.testProbeInfo().c_str(), mean, popShare, fn.c_str(), ok ? "steady" : "FLICKERS");
    }
    }
    return bad ? 1 : 0;
}
