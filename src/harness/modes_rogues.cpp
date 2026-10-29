// E-03: the drifters: `rogues` (`heard`, `voices`, `dark`)
#include "harness/harness.h"
#include "harness/test_worlds.h"

// `rogues heard`: what the radar hears of the drifters from home, the three C-series worlds and the six age worlds at each step of the
// gain (none at x1; the gain each needs, its distance, what it is), then the radar in the game: at home the dial up, the nearest drifter heard
// locked at its own step (shots/tests/rogue_radar_lock.png, the readout's status and the log), Enter flies there (rogue_heard_arrival.png:
// the HUD's brackets on it), the black disc from 60 radii (rogue_disc_60.png) and from orbit (rogue_disc_orbit.png; a brown dwarf's largest
// moon lit by its glow, rogue_bd_moon.png), the nearest giant heard likewise (rogue_giant_60.png, rogue_giant_orbit.png: its aurorae), the
// middle row's bank and shade across each disc, and the nearest landable drifter's maps in the survey look (rogue_landing_map.png,
// rogue_landing_zoom.png) and, landed, its sector map (rogue_sector_map.png)
static int runRoguesHeard() {
    struct Place { std::string name; int64_t x, y, z; };
    std::vector<Place> places = {{"home", HOME_SX, HOME_SY, HOME_SZ}, {"Aieliaalas II", 151, 0, 25}, {"Leileashphail III", 153, 0, 70}, {"Eleinewai I", 152, 0, 92}};
    for (int a = 0; a < AGE_COUNT; a++) places.push_back({fmt("the %s world", AGE_NAMES[a]), AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz});
    printf("the drifters the radar hears (a drifter of power p heard to %.0f sqrt((G - 1) p) ly at gain G: never at x1):\n", SIGNAL_ROGUE_REACH_LY);
    for (size_t pi = 0; pi < places.size(); pi++) {
        const Place& P = places[pi];
        Star st; Vec3 at((P.x + 0.5) * SECTOR_KM, (P.y + 0.5) * SECTOR_KM, (P.z + 0.5) * SECTOR_KM);
        if (starInSector(P.x, P.y, P.z, st, false)) at = st.pos;
        std::vector<Signal> L; signalsNear(at, L, SIGNAL_REACH_LY, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]);
        std::vector<Signal> d; for (const Signal& x : L) if (x.kind == SIG_ROGUE) d.push_back(x);
        std::sort(d.begin(), d.end(), [](const Signal& a, const Signal& b) { return a.needGain < b.needGain; });
        std::vector<Signal> x1; signalsNear(at, x1);
        int atX1 = 0; for (const Signal& x : x1) if (x.kind == SIG_ROGUE) atX1++;
        std::string steps; double farLy = 0;
        for (int k = 0; k < SIGNAL_GAIN_STEPS; k++) {
            int n = 0;
            for (const Signal& x : d)
                if (x.needGain <= SIGNAL_GAINS[k]) n++;
            steps += fmt("%s x%d %d", k ? "," : "", SIGNAL_GAINS[k], k ? n : atX1);
        }
        for (const Signal& x : d) farLy = std::max(farLy, x.distLy);
        printf("  %-22s%s; the farthest %s ly%s\n", P.name.c_str(), steps.c_str(), thousands(farLy).c_str(),
                d.empty() ? "" : fmt("; the first at %.0f ly, needing x%.1f", d[0].distLy, d[0].needGain).c_str());
        if (pi == 0)
            for (const Signal& x : d) {
                int step = 0; while (step < SIGNAL_GAIN_STEPS - 1 && SIGNAL_GAINS[step] < x.needGain) step++;
                const DrifterBody b = drifterBody(x.star);
                printf("      %7.1f ly  needs x%5.1f (heard from x%d)  power %.3f  strength at x32 %.2f  %-12s %6.0f km, own heat %4.0f K  sector %lld %lld %lld\n", x.distLy, x.needGain,
                        SIGNAL_GAINS[step], x.power, signalStrengthAtGain(x, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]), KINDS[b.type].name, b.radiusKm, b.heatK, (long long)x.star.sx, (long long)x.star.sy,
                        (long long)x.star.sz);
            }
    }
    TestGame game;
    game.savePrefix = "shots/tests/rogues_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.guidePath = "shots/tests/rogues_guide.txt";
    remove(game.guidePath.c_str());
    game.newGame(); game.settings.cabin = false; game.applySettings(); applyTestScale(game); game.setState(GameState::SPACE);
    for (const Body& ob : game.testSystem().bodies) if (ob.type != PT_COMPANION) game.testWaitMapOf(ob.index);   // every map first
    Input in;
    auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
    auto press = [&](int key) { pressKey(game, in, key); };
    auto shot = [&](const char* name) { writePNG(fmt("shots/tests/%s.png", name).c_str(), game.output(), FBW, FBH); };
    run(0.2); press(KEY_B); run(0.2);
    for (int i = 0; i < SIGNAL_GAIN_STEPS - 1; i++) press(KEY_EQUAL);
    for (int i = 0; i < 60 && !game.testRadarDeepReady(); i++) run(1.0 / 30);
    run(0.1);
    int who = -1, giant = -1; double whoLy = 1e18, giantLy = 1e18;
    for (int i = 0; i < game.testRadarSignals(); i++) {
        if (game.testRadarKind(i) != SIG_ROGUE || game.testRadarLocal(i)) continue;
        int64_t sx, sy, sz; game.testRadarSector(i, sx, sy, sz);
        Star d; drifterInSector(sx, sy, sz, d, false);
        const double ly = game.testRadarDist(i);
        if (ly < whoLy) { whoLy = ly; who = i; }
        if (drifterBody(d).type == PT_GASGIANT && ly < giantLy) { giantLy = ly; giant = i; }
    }
    if (who < 0) { printf("the radar at x32 hears no drifter from home\n"); return 1; }
    int64_t wx, wy, wz, gx = 0, gy = 0, gz = 0; game.testRadarSector(who, wx, wy, wz);
    if (giant >= 0) game.testRadarSector(giant, gx, gy, gz);
    const double need = game.testRadarNeedGain(who);
    int step = 0; while (step < SIGNAL_GAIN_STEPS - 1 && SIGNAL_GAINS[step] < need) step++;
    for (int i = SIGNAL_GAIN_STEPS - 1; i > step; i--) press(KEY_MINUS);
    run(0.2);
    for (int i = 0; i < game.testRadarSignals(); i++) {
        int64_t sx, sy, sz;
        if (game.testRadarKind(i) == SIG_ROGUE && game.testRadarSector(i, sx, sy, sz) && sx == wx && sy == wy && sz == wz) who = i;
    }
    game.testAimAtSignal(who);
    double lockAt = -1; for (int i = 0; i < 30 * 20 && lockAt < 0; i++) { game.frame(in, 1.0 / 30); in.newFrame(); if (game.testRadarLocked() >= 0) lockAt = (i + 1) / 30.0; }
    run(0.3);
    shot("rogue_radar_lock");
    std::string logLine = game.guide.log.empty() ? "" : game.guide.log.back().text;
    printf("the radar at home: the nearest drifter heard, %.0f ly off, needs x%.1f: at x%d locked after %.1f s: '%s'; the log: '%s' -> shots/tests/rogue_radar_lock.png\n", whoLy, need,
            SIGNAL_GAINS[step], lockAt, game.testStatus().c_str(), logLine.c_str());
    if (lockAt < 0) return 1;
    press(KEY_ENTER); run(0.2);
    printf("Enter: '%s'\n", game.testStatus().c_str());
    for (int i = 0; i < 30 * 120 && game.testShipMode() == (int)ShipState::VIMANA; i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
    run(0.6);
    Star d; drifterInSector(wx, wy, wz, d, true);
    {
        const StarSystem& sys = game.testSystem(); const Body& b = sys.bodies[0];
        const double dist = length(game.testShipPos() - sys.star.pos);
        printf("the arrival: %s, %s, the ship %.1f radii off, the local target %d: '%s' -> shots/tests/rogue_heard_arrival.png\n", sys.star.seed == d.seed ? "the drifter locked" : "NOT THE DRIFTER",
                KINDS[b.type].name, dist / b.radiusKm, game.testLocalTarget(), game.testStatus().c_str());
        shot("rogue_heard_arrival");
    }
    auto disc = [&](const Star& at, const char* far, const char* orbit) {
        game.testArriveAt(at); run(0.1);
        const StarSystem& sys = game.testSystem(); const double R = sys.bodies[0].radiusKm;
        const Vec3 dir = normalize(game.testShipPos() - sys.star.pos);
        game.testPlaceShip(sys.star.pos + dir * 60 * R); game.testAimAtBody(0); run(0.4);
        shot(far);
        game.testParkAt(at, 0, false); run(0.2);
        if (game.state == GameState::LANDING_MAP) press(KEY_ESCAPE);
        game.testAimAtBody(0); run(0.4);
        shot(orbit);
        {   // the disc's shades along the frame's middle row, across it (bank:shade; the dark frames are read by their numbers)
            std::string row; const int y = FBH / 2;
            for (int x = 0; x < FBW; x += FBW / 24) { const Pix p = game.fb.idx[(size_t)y * FBW + x]; row += fmt(" %d:%.0f", bankOf(p), shadeOf(p)); }
            int aur = 0; for (const Pix p : game.fb.idx) if (bankOf(p) == 12 && intenOf(p) > 0) aur++;
            printf("    the middle row (bank:shade):%s; the aurorae's pixels %d\n", row.c_str(), aur);
        }
        printf("  %s: from 60 radii -> shots/tests/%s.png, parked %.1f radii off -> shots/tests/%s.png\n", KINDS[sys.bodies[0].type].name, far, length(game.testShipPos() - sys.star.pos) / R, orbit);
        if (kindHas(sys.bodies[0].type, KT_GLOWS) && sys.bodies.size() > 1) {   // a brown dwarf's moon lit by its glow on the side facing it
            int mi = 1; for (int i = 1; i < (int)sys.bodies.size(); i++) if (sys.bodies[i].radiusKm > sys.bodies[mi].radiusKm) mi = i;
            game.testParkAt(at, mi, false); run(0.2);
            if (game.state == GameState::LANDING_MAP) press(KEY_ESCAPE);
            game.testAimAtBody(mi); run(0.4);
            shot("rogue_bd_moon");
            printf("  its largest moon (%s, %.0f km, %.0f K) parked at, lit by the brown dwarf's glow -> shots/tests/rogue_bd_moon.png\n", KINDS[game.testSystem().bodies[mi].type].name,
                    game.testSystem().bodies[mi].radiusKm, game.testSystem().bodies[mi].tempK);
        }
    };
    printf("the black disc:\n");
    disc(d, "rogue_disc_60", "rogue_disc_orbit");
    if (giant >= 0) { Star g; drifterInSector(gx, gy, gz, g, true); disc(g, "rogue_giant_60", "rogue_giant_orbit"); }
    {   // the maps in the survey look: the nearest landable drifter
        std::vector<Star> near; Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, false); driftersNear(home.pos, 1500, near);
        int li = -1; for (size_t i = 0; i < near.size() && li < 0; i++) { const DrifterBody b = drifterBody(near[i]); if (KINDS[b.type].landable && b.radiusKm >= 1500) li = (int)i; }
        if (li < 0) { printf("no landable drifter within 1,500 ly\n"); return 0; }
        Star l; drifterInSector(near[li].sx, near[li].sy, near[li].sz, l, true);
        game.testParkAt(l, 0); run(0.3);
        shot("rogue_landing_map");
        game.testLandCursor(20, 40); press(KEY_Z); run(0.2);
        for (int i = 0; i < 60; i++) run(1.0 / 30);
        shot("rogue_landing_zoom");
        printf("the survey of %s (%s): the landing map -> shots/tests/rogue_landing_map.png, its zoom -> shots/tests/rogue_landing_zoom.png\n", KINDS[drifterBody(l).type].name, l.name.c_str());
        press(KEY_ENTER);
        for (int i = 0; i < 30 * 20 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
        if (game.state != GameState::SURFACE) { printf("not landed\n"); return 1; }
        run(1.0); press(KEY_N); run(0.5);
        shot("rogue_sector_map");
        printf("landed: the sector map -> shots/tests/rogue_sector_map.png\n");
    }
    remove(game.guidePath.c_str());
    return 0;
}

// D-01: `rogues [ly]`: the drifters within ly of home (1,000 by default), nearest first: where, how far, what (the kind by the dark's table, its
// size, its own heat, its moons), the star it came from and how long ago; their density against the stars' there (the stars counted from the
// field's density, sampled); the kinds' shares within three times as far against the dark's table; then the nearest landable one visited as
// the game reaches it (the console's way until the radar finds them, D-02): the arrival (shots/tests/rogue_arrival.png), the parked view
// (rogue_parked.png) and a landing (rogue_landing.png: the night's light alone)
// `rogues voices`: a voice for every drifter. The drifters the radar hears at x32 from home, the three C-series worlds and the six age
// worlds, each's voice in words and numbers; twelve WAVs (shots/tests/rogue_voice_*.wav): eight voices whole as at the drifter (the first two
// brown dwarfs and the first two giants heard, the nearest young, metal, rocky and icy worlds of the field), the first heard from home as the
// radar hears it at its step (worn, locked), a drifter under a people's voice at x18, the cabin's radio at a drifter (the ship's hum beside it)
// and the suit's radio on its ground; their levels (no peak over 1, the rms within C-07's 0.02-0.13, at most 20 samples clipped); a beacon in
// its slot under a giant's voice (D-05's: shots/tests/rogue_beacon_slot.wav); the distinctness over the hundred drifters nearest home (a breath
// each: their parameters' nearest pair, their voice prints pair by pair against two signals of one kind)
static int runRoguesVoices() {
    makeDir("shots"); makeDir("shots/tests");
    struct Place { std::string name; int64_t x, y, z; };
    std::vector<Place> places = {{"home", HOME_SX, HOME_SY, HOME_SZ}, {"Aieliaalas II", 151, 0, 25}, {"Leileashphail III", 153, 0, 70}, {"Eleinewai I", 152, 0, 92}};
    for (int a = 0; a < AGE_COUNT; a++) places.push_back({fmt("the %s world", AGE_NAMES[a]), AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz});
    std::vector<Signal> heard; Signal firstHome; bool haveFirst = false;
    printf("the drifters heard at x32 from the ten places, each's voice:\n");
    for (size_t pi = 0; pi < places.size(); pi++) {
        const Place& P = places[pi];
        Star st; Vec3 at((P.x + 0.5) * SECTOR_KM, (P.y + 0.5) * SECTOR_KM, (P.z + 0.5) * SECTOR_KM);
        if (starInSector(P.x, P.y, P.z, st, false)) at = st.pos;
        std::vector<Signal> L; signalsNear(at, L, SIGNAL_REACH_LY, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]);
        std::vector<Signal> d; for (const Signal& x : L) if (x.kind == SIG_ROGUE) d.push_back(x);
        std::sort(d.begin(), d.end(), [](const Signal& a, const Signal& b) { return a.needGain < b.needGain; });
        if (pi == 0 && !d.empty()) { firstHome = d[0]; haveFirst = true; }
        for (const Signal& x : d) {
            bool seen = false; for (const Signal& h : heard) if (h.seed == x.seed) seen = true;
            if (seen) continue;
            heard.push_back(x);
            printf("  from %-20s %6.0f ly, x%4.1f: %s\n", P.name.c_str(), x.distLy, x.needGain, AudioSynth::rogueLine(x.seed, rogueVoiceFor(x)).c_str());
        }
    }
    const Vec3 home((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
    std::vector<RogueRef> field = roguesNearest(home, 100);
    // the twelve: the first two brown dwarfs and giants heard (home's first among them), the field's nearest young, metal, rocky and icy worlds
    struct Pick { std::string name; RogueRef ref; double distLy = 0; };
    std::vector<Pick> whole;
    auto pickHeard = [&](int cls, int count, const char* what) {
        int got = 0;
        if (haveFirst && rogueVoiceClass(firstHome.bodyType) == cls) {
            RogueRef r;
            Star full = firstHome.star;
            placeInSector(full.sx, full.sy, full.sz, full, false);
            r = rogueRefOf(full);
            whole.push_back({fmt("%s_%d", what, ++got), r, firstHome.distLy});
        }
        for (const Signal& h : heard) {
            if (got >= count) break;
            if (rogueVoiceClass(h.bodyType) != cls || (haveFirst && h.seed == firstHome.seed)) continue;
            Star full = h.star; placeInSector(full.sx, full.sy, full.sz, full, false);
            whole.push_back({fmt("%s_%d", what, ++got), rogueRefOf(full), h.distLy});
        }
    };
    pickHeard(RV_BROWN_DWARF, 2, "brown_dwarf"); pickHeard(RV_GIANT, 2, "giant");
    auto pickField = [&](const char* what, const std::function<bool(int)>& want) {
        for (const RogueRef& r : field) if (want(r.sig.bodyType)) { whole.push_back({what, r, length(r.star.pos - home) / SECTOR_KM}); return; }
        printf("  (no %s among the hundred nearest home)\n", what);
    };
    pickField("young", [](int t) { return rogueVoiceClass(t) == RV_YOUNG; });
    pickField("metal", [](int t) { return rogueVoiceClass(t) == RV_METAL; });
    pickField("rocky", [](int t) { return t == PT_ROCKY || t == PT_CRATERED || t == PT_DESERT || t == PT_EUROPAN || t == PT_FROZEN_FELISIAN || t == PT_CARBON; });
    pickField("icy", [](int t) { return t == PT_ICY || t == PT_NITROGEN || t == PT_BRINE_DWARF || t == PT_FROST_DWARF; });
    printf("the twelve voices (22,050 Hz; the levels: no peak over 1, the rms within C-07's 0.02-0.13, at most 20 samples clipped):\n");
    int n = 0, bad = 0;
    auto judge = [&](const std::string& name, const RenderStats& r, const std::vector<int16_t>& pcm, const std::string& what) {
        const std::string fn = fmt("shots/tests/rogue_voice_%02d_%s.wav", ++n, name.c_str()); writeWav16(fn, pcm, 22050);
        const bool ok = !r.nan && r.peak < 1.0 && r.clip <= 20 && r.rms >= 0.02 && r.rms <= 0.13; if (!ok) bad++;
        printf("  %-34s %4.0f s, rms %.3f, peak %.3f, %d clipped%s: %s\n", fn.c_str() + 12, pcm.size() / 22050.0, r.rms, r.peak, r.clip, ok ? "" : " OUT OF BOUNDS", what.c_str());
    };
    for (const Pick& p : whole) {
        const double secs = std::min(96.0, std::max(36.0, AudioSynth::rogueBreathS(p.ref.sig.seed, p.ref.voice) + 10));
        std::vector<int16_t> pcm; RenderStats r = renderRogue(p.ref, secs, &pcm);
        judge(p.name, r, pcm, fmt("%s, %.0f ly from home, whole: %s", KINDS[p.ref.sig.bodyType].name, p.distLy, AudioSynth::rogueLine(p.ref.sig.seed, p.ref.voice).c_str()));
    }
    if (haveFirst) {
        Star full = firstHome.star; placeInSector(full.sx, full.sy, full.sz, full, false); const RogueRef first = rogueRefOf(full);
        int step = 0; while (step < SIGNAL_GAIN_STEPS - 1 && SIGNAL_GAINS[step] < firstHome.needGain) step++;
        RxOpts o; o.gainT = gainShareOf(SIGNAL_GAINS[step]); o.sky = 0.4; o.clarity = 0.6; o.wear = clampd(std::log(firstHome.distLy / SIGNAL_REACH_LY) / std::log(30.0), 0, 1);
        {
            std::vector<int16_t> pcm;
            RenderStats r = renderRogue(first, 40, &pcm, o);
            judge(fmt("far_x%d", SIGNAL_GAINS[step]), r, pcm, fmt("the first heard from home, %.0f ly, as the radar hears it locked at x%d (worn %.2f)", firstHome.distLy, SIGNAL_GAINS[step], o.wear));
        }
        PeopleWorld w;
        const RogueRef& g = whole.size() > 2 ? whole[2].ref : first;
        if (firstPeopleWorld(w)) {
            RxOpts u; u.gainT = gainShareOf(18); u.sky = 0.4; u.age = AGE_MODERN; u.kind2 = SIG_ROGUE; u.seed2 = g.sig.seed; u.rogue2 = g.voice; u.sig2 = 0.5;
            std::vector<int16_t> pcm; RenderStats r = renderProgramme(w, RP_VOICE, 40, &pcm, nullptr, u);
            judge("under_a_voice_x18", r, pcm, fmt("%s's voice under a people's broadcast (%s) at x18", KINDS[g.sig.bodyType].name, w.name.c_str()));
        }
        {
            RxOpts c;
            c.set = RADIO_CABIN;
            c.level = RADIO_CABIN_LEVEL;
            c.hum = 0.6;
            c.clarity = 0.9;
            std::vector<int16_t> pcm;
            RenderStats r = renderRogue(first, 40, &pcm, c);
            judge("cabin_radio", r, pcm, "the cabin's radio at the first heard from home, the ship's hum beside it");
        }
        const RogueRef* land = nullptr; for (const Pick& p : whole) if (KINDS[p.ref.sig.bodyType].landable && !kindHas(p.ref.sig.bodyType, KT_GIANT)) { land = &p.ref; break; }
        if (land) {
            RxOpts c;
            c.set = RADIO_SUIT;
            c.level = RADIO_SUIT_LEVEL;
            c.hum = 0.05;
            c.clarity = 0.9;
            std::vector<int16_t> pcm;
            RenderStats r = renderRogue(*land, 40, &pcm, c);
            judge("suit_radio", r, pcm, fmt("the suit's radio on the ground of %s", KINDS[land->sig.bodyType].name));
        }
        {   // D-05's slot: a beacon under a giant's voice
            RxOpts b; b.voice = RP_BEACON; std::vector<int16_t> pcm; RenderStats r = renderRogue(g, 40, &pcm, b); writeWav16("shots/tests/rogue_beacon_slot.wav", pcm, 22050);
            printf("  %-34s %4.0f s, rms %.3f, peak %.3f, %d clipped: a beacon under %s's voice, slowed and darkened (D-05's slot)\n", "rogue_beacon_slot.wav", 40.0, r.rms, r.peak, r.clip,
                    KINDS[g.sig.bodyType].name);
        }
    }
    // the test drifters' beacons as the radar plays them locked (a radio people's last broadcast under its drifter's voice)
    for (const AgeWorld& aw : TEST_DRIFTERS) {
        Star d; if (!placeInSector(aw.sx, aw.sy, aw.sz, d, true)) continue;
        BeaconInfo bi; std::vector<int16_t> pcm; RxOpts o; o.clarity = 0.85;
        const RenderStats r = renderBeacon(d, 80, &pcm, &bi, o);
        if (!bi.valid) { printf("  %s: no beacon (no radio people, or its radio came after its last sunrise)\n", d.name.c_str()); continue; }
        std::string nm = d.name; for (char& c : nm) c = (char)std::tolower((unsigned char)c);
        judge(fmt("beacon_%s", nm.c_str()), r, pcm, fmt("%s's beacon: %s under its voice; the %s's last recording (%s, its opening %d words) spoken slowed, %.0f s, three times %.0f s apart",
                                                       d.name.c_str(), RADIO_PROGRAMME_NAMES[std::max(0, bi.B.machine)], bi.people.c_str(), bi.genre.c_str(), bi.words, bi.speechS, bi.gap));
        printf("      read:  %s\n      heard: %s\n", bi.opening.c_str(), bi.theirs.c_str());
    }
    printf("  %d written, %d out of bounds\n", n, bad);
    // the distinctness
    const double t0 = nowSec();
    const Distinctness d = rogueDistinctness(field, 20, 60.0);
    int byClass[RV_COUNT] = {0}; for (const RogueRef& r : field) byClass[std::max(0, std::min((int)RV_COUNT - 1, r.voice.cls))]++;
    std::string classes; for (int c = 0; c < RV_COUNT; c++) classes += fmt("%s%s %d", c ? ", " : "", ROGUE_VOICE_NAMES[c], byClass[c]);
    printf("the distinctness over the %d drifters nearest home (%s), a breath each, the kinds 60 s (%.0f s):\n", d.n, classes.c_str(), nowSec() - t0);
    if (d.traitA >= 0)
        printf(
                "  their parameters: the nearest two %.3f apart (of %zu, each 0..1): %s / %s\n", d.traitNearest,
                [&] {
                    std::vector<double> t;
                    AudioSynth::rogueTraits(0, RogueVoice(), t);
                    return t.size();
                }(),
                AudioSynth::rogueLine(field[d.traitA].sig.seed, field[d.traitA].voice).c_str(), AudioSynth::rogueLine(field[d.traitB].sig.seed, field[d.traitB].voice).c_str());
    printf("  their voice prints (twelfths of an octave, dB), %d pairs: the nearest %.2f, the median %.2f\n", d.pairs, d.nearest, d.median);
    if (d.nearA >= 0)
        printf("    the nearest pair: %s / %s\n", AudioSynth::rogueLine(field[d.nearA].sig.seed, field[d.nearA].voice).c_str(),
                AudioSynth::rogueLine(field[d.nearB].sig.seed, field[d.nearB].voice).c_str());
    static const char* const KN[3] = {"pulsars", "comets", "magnetospheres"};
    for (int k = 0; k < 3; k++) printf("  two %s (20 by seeds of their own, 60 s, 190 pairs): the median %.2f, the nearest %.2f\n", KN[k], d.kindMedian[k], d.kindNearest[k]);
    printf("  every pair of drifters apart by more than two signals of one kind are (the largest median, %.2f): %s\n", d.bar, d.under == 0 ? "yes" : fmt("NO, %d pairs under", d.under).c_str());
    if (d.under > 0) {   // the drifters in the pairs under the bar, the most often first
        std::vector<int> idx; for (int i = 0; i < d.n; i++) if (d.inUnder[i]) idx.push_back(i);
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return d.inUnder[a] > d.inUnder[b]; });
        for (size_t k = 0; k < idx.size() && k < 8; k++) printf("    in %d of them: %s\n", d.inUnder[idx[k]], AudioSynth::rogueLine(field[idx[k]].sig.seed, field[idx[k]].voice).c_str());
    }
    return bad == 0 && d.under == 0 ? 0 : 1;
}

// D-04: `rogues dark`: the ground in the dark. Over the drifters within 3,000 ly of home: the kinds by the dark's table (a dark ocean made from a
// rocky drifter), their own traits by kind (FROZEN AIR, EMBERS, LIGHTS IN THE WATER; LIGHTS LEFT ON is D-05's), the bodies of their systems whose
// seas froze, the airs that fell; then from orbit a young interior's ember fields (rogue_embers_orbit.png), a dark ocean's lights in its water
// (rogue_dark_ocean_orbit.png), an air fallen as snow (rogue_frozen_air_orbit.png); the capsule's floodlights coming down 120 m over a drifter
// (rogue_floodlights.png); and the lights left on, staged on a modern people's town at night (the trait set on the world for the frame:
// rogue_lights_left_on.png, its lit windows and its street lamps; and from orbit over its night side, rogue_lights_left_on_orbit.png)
static int runRoguesDark() {
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    std::vector<Star> near; driftersNear(home.pos, 3000, near);
    int kinds[PT_COUNT] = {}, own[TR_COUNT] = {}, bodies = 0, aired = 0, airFell = 0, seas = 0, seasFrozen = 0, withOwn = 0;
    int ownByKind[PT_COUNT][4] = {};
    Star embersAt, oceanAt, frozenAt; bool haveEmbers = false, haveOcean = false, haveFrozen = false;
    for (const Star& d0 : near) {
        Star d; drifterInSector(d0.sx, d0.sy, d0.sz, d, true);
        StarSystem sys; sys.generate(d);
        kinds[sys.bodies[0].type]++;
        for (const Body& b : sys.bodies) {
            if (!KINDS[b.type].landable) continue;
            bodies++;
            const BodyGen g = BodyGen::make(b);
            if (KINDS[b.type].atmosphere) { aired++; if (airFrozen(b)) airFell++; }
            if (kindHas(b.type, KT_SEA) || kindHas(b.type, KT_WET) || g.liquidLevel > -1e8) { seas++; if (g.frozenSeas) seasFrozen++; }
            bool any = false;
            for (int t = TR_FROZEN_AIR; t <= TR_LIGHTS_IN_WATER; t++) if (g.hasTrait(t)) { own[t]++; any = true; if (b.parent < 0) ownByKind[b.type][t - TR_FROZEN_AIR]++; }
            if (any) withOwn++;
            if (b.parent < 0 && !haveEmbers && g.hasTrait(TR_EMBERS) && !isLavaWorld(b.type)) { embersAt = d; haveEmbers = true; }
            if (b.parent < 0 && !haveOcean && g.hasTrait(TR_LIGHTS_IN_WATER)) { oceanAt = d; haveOcean = true; }
            if (b.parent < 0 && !haveFrozen && g.hasTrait(TR_FROZEN_AIR) && b.type == PT_DESERT) { frozenAt = d; haveFrozen = true; }
        }
    }
    printf("the drifters within 3,000 ly of home: %zu; their kinds:", near.size());
    for (int k = 0; k < PT_COUNT; k++) if (kinds[k]) printf(" %s %d", KINDS[k].shortName, kinds[k]);
    printf("\nthe landable bodies of their systems: %d; with a drifter's own trait %d: FROZEN AIR %d, EMBERS %d, LIGHTS LEFT ON %d, LIGHTS IN THE WATER %d\n", bodies, withOwn,
           own[TR_FROZEN_AIR], own[TR_EMBERS], own[TR_LIGHTS_LEFT_ON], own[TR_LIGHTS_IN_WATER]);
    printf("  airs %d, fallen as snow %d; seas and lakes %d, frozen at their level %d\n", aired, airFell, seas, seasFrozen);
    printf("  the drifters' own by kind (frozen air, embers, lights left on, lights in the water):");
    for (int k = 0; k < PT_COUNT; k++)
        if (kinds[k] && KINDS[k].landable) printf(" %s %d/%d/%d/%d of %d;", KINDS[k].shortName, ownByKind[k][0], ownByKind[k][1], ownByKind[k][2], ownByKind[k][3], kinds[k]);
    printf("\n");
    TestGame game;
    game.savePrefix = "shots/tests/rogues_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.guidePath = "shots/tests/rogues_guide.txt";
    remove(game.guidePath.c_str());
    game.newGame(); game.settings.cabin = false; game.applySettings(); applyTestScale(game);
    Input in;
    auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
    auto press = [&](int key) { pressKey(game, in, key); };
    auto shot = [&](const char* name) { writePNG((std::string("shots/tests/") + name + ".png").c_str(), game.output(), FBW, FBH); };
    auto orbit = [&](const Star& at, const char* name, const char* what) {
        game.testParkAt(at, 0, true); run(0.2);
        if (game.state == GameState::LANDING_MAP) press(KEY_ESCAPE);
        game.testAimAtBody(0); run(0.6);
        shot(name);
        int lit[32] = {}; for (const Pix p : game.fb.idx) if (shadeOf(p) >= 40) lit[bankOf(p)]++;   // the pixels over shade 40 by bank (the globe's own lights)
        std::string banks; for (int k = 0; k < 32; k++) if (lit[k]) banks += fmt(" %d:%d", k, lit[k]);
        printf("%s: %s (%s) parked %.1f radii off -> shots/tests/%s.png (bank:pixels over shade 40:%s)\n", what, at.name.c_str(), KINDS[game.testSystem().bodies[0].type].name,
                length(game.testShipPos() - game.testSystem().star.pos) / game.testSystem().bodies[0].radiusKm, name, banks.c_str());
    };
    run(0.2);
    if (haveEmbers) orbit(embersAt, "rogue_embers_orbit", "a young interior's ember fields");
    if (haveOcean) orbit(oceanAt, "rogue_dark_ocean_orbit", "a dark ocean's lights in its water");
    if (haveFrozen) orbit(frozenAt, "rogue_frozen_air_orbit", "an air fallen as snow");
    if (haveFrozen) {   // the capsule's floodlights on the way down, 120 m over the frozen air's snow
        game.testParkAt(frozenAt, 0); run(0.3);
        game.testLandCursor(20, 40); run(0.1); press(KEY_ENTER);
        for (int i = 0; i < 30 * 20 && !(game.state == GameState::DESCENT && game.testSurface().cameraOverrideAlt < 120); i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
        shot("rogue_floodlights");
        printf("the capsule's floodlights coming down, %.0f m over the ground -> shots/tests/rogue_floodlights.png\n", game.testSurface().cameraOverrideAlt);
        for (int i = 0; i < 30 * 20 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
        run(0.5);
        printf("  landed: '%s'\n", game.testStatus().c_str());
    }
    {   // the lights left on, staged: a modern people's town at night (D-05 gives the trait to the drifters' late peoples)
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        if (setupSceneForType(-1, 0, -25 * DEG, 0.0, 0.02, sv, sys, nb, t, SCENE_AGE_MODERN)) {
            Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0); fb0.mush(2); saveFB(fb0, "shots/tests/rogue_lights_left_off.png");
            sv.site.gen.traits[TRAIT_SLOTS - 1] = TR_LIGHTS_LEFT_ON;
            Framebuffer fb; Input in2; sv.update(0.016, in2, t, false); sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
            saveFB(fb, "shots/tests/rogue_lights_left_on.png");
            int lamps = 0; for (const GroundMark& m : sv.site.marks) if (m.kind == 0) lamps++;
            printf("the lights left on, staged on %s (%s) at night (the sun %.0f degrees down): %d streets near; before -> shots/tests/rogue_lights_left_off.png, after -> "
                   "shots/tests/rogue_lights_left_on.png\n",
                    sys.bodies[sv.site.body].name.c_str(), KINDS[sys.bodies[sv.site.body].type].name, -sv.env.sun.altitude / DEG, lamps);
        } else printf("the lights left on: no modern town to stage them on\n");
    }
    return 0;
}

int runRogues(int argc, char** argv) {
    if (argc > 2 && std::string(argv[2]) == "heard") return runRoguesHeard();
    if (argc > 2 && std::string(argv[2]) == "dark") return runRoguesDark();
    if (argc > 2 && std::string(argv[2]) == "voices") return runRoguesVoices();
    const double R = argc > 2 ? std::max(10.0, atof(argv[2])) : 1000.0;
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    std::vector<Star> near;
    auto t0 = std::chrono::steady_clock::now();
    const int looked = driftersNear(home.pos, R, near);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    printf("the drifters within %.0f ly of home (%s, sector %lld %lld %lld): %zu (%d cells and clusters looked at, %.1f ms)\n", R, home.name.c_str(), (long long)HOME_SX, (long long)HOME_SY,
            (long long)HOME_SZ, near.size(), looked, ms);
    int landable = -1;
    for (size_t i = 0; i < near.size(); i++) {
        Star d; drifterInSector(near[i].sx, near[i].sy, near[i].sz, d, true);
        StarSystem sys; sys.generate(d);
        const Body& b = sys.bodies[0];
        std::string moons;
        for (const Body& m : sys.bodies) if (m.parent == 0) moons += fmt("%s%s %.0f K", moons.empty() ? "" : ", ", KINDS[m.type].shortName, m.tempK);
        const DrifterPast past = drifterPast(d);
        printf("  %2zu  %6lld %5lld %6lld  %6.1f ly  %-17s %6.0f km  own heat %4.0f K  moons %d%s%s\n", i + 1, (long long)d.sx, (long long)d.sy, (long long)d.sz, length(d.pos - home.pos) / SECTOR_KM,
               KINDS[b.type].name, b.radiusKm, b.heatK, (int)sys.bodies.size() - 1, moons.empty() ? "" : " (", moons.empty() ? "" : (moons + ")").c_str());
        if (past.found) printf("      left %s (%s, sector %lld %lld %lld) %.0f years ago at %.1f km/s: %.1f ly back, %.1f ly from it now\n", past.star.name.c_str(), STAR_CLASSES[past.star.cls].code,
                               (long long)past.star.sx, (long long)past.star.sy, (long long)past.star.sz, past.years, past.speedKms, past.travelLy, length(past.star.pos - d.pos) / SECTOR_KM);
        else printf("      left no star found along its way back (%.0f years, %.1f km/s)\n", past.years, past.speedKms);
        if (landable < 0 && KINDS[b.type].landable && b.radiusKm >= 1500) landable = (int)i;
    }
    {   // the stars there, from the field's density (every 16th sector a side, the near ones every 4th)
        double stars = 0;
        const int step = R <= 200 ? 2 : (R <= 600 ? 8 : 16), Ri = (int)R;
        for (int x = -Ri; x <= Ri; x += step) for (int y = -Ri; y <= Ri; y += step) for (int z = -Ri; z <= Ri; z += step) {
            if ((double)x * x + (double)y * y + (double)z * z > R * R) continue;
            stars += galaxyDensity(HOME_SX + x, HOME_SY + y, HOME_SZ + z) * step * step * step;
        }
        printf("the stars within %.0f ly: about %.3g; the drifters one for every %.3g of them (the field's rate %.3g a star)\n", R, stars, near.empty() ? 0.0 : stars / near.size(),
               DRIFT_CELL_MAX * DRIFT_RATE / ((double)DRIFT_CELL * DRIFT_CELL * DRIFT_CELL));
    }
    {   // the kinds within three times as far, against the table
        std::vector<Star> wide; driftersNear(home.pos, 3 * R, wide);
        int count[PT_COUNT] = {};
        for (const Star& d : wide) count[drifterBody(d).type]++;
        double W = 0; for (int k = 0; k < DARK_KIND_COUNT; k++) W += DARK_KINDS[k].weight;
        printf("the kinds of the %zu within %.0f ly (the dark's table's share in brackets):", wide.size(), 3 * R);
        for (int k = 0; k < DARK_KIND_COUNT; k++)
            printf(" %s %d (%.0f%%)%s", KINDS[DARK_KINDS[k].type].shortName, count[DARK_KINDS[k].type], 100 * DARK_KINDS[k].weight / W, k + 1 < DARK_KIND_COUNT ? "," : "\n");
    }
    if (landable < 0) { printf("no landable drifter within %.0f ly: no frames\n", R); return 0; }
    Star d; drifterInSector(near[landable].sx, near[landable].sy, near[landable].sz, d, true);
    TestGame game;
    game.savePrefix = "shots/tests/rogues_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.guidePath = "shots/tests/rogues_guide.txt";
    remove(game.guidePath.c_str());
    game.newGame(); game.settings.cabin = false; game.applySettings(); applyTestScale(game);
    Input in;
    auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
    auto press = [&](int key) { pressKey(game, in, key); };
    run(0.2);
    game.testSetRemote(d); press(KEY_V);   // the flight there as V flies it (its end outside the drifter, then the arrival)
    const double flight = game.testShipMode() == (int)ShipState::VIMANA ? game.testFlightDur() : 0;
    for (int i = 0; i < 30 * 120 && game.testShipMode() == (int)ShipState::VIMANA; i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
    printf("the flight there: %.0f s, %s\n", flight, game.testShipMode() == (int)ShipState::VIMANA ? "NOT ARRIVED" : "arrived");
    run(0.5);
    {
        const StarSystem& sys = game.testSystem(); const Body& b = sys.bodies[0]; const Vec3 P = game.testShipPos();
        const double dist = length(P - sys.star.pos);
        printf("the arrival at drifter %d (%s, %.0f km): the ship %.0f km off its centre (%.1f radii), the disc %.2f degrees wide, the class line '%s', the light there %.2f -> "
               "shots/tests/rogue_arrival.png\n",
                landable + 1, KINDS[b.type].name, b.radiusKm, dist, dist / b.radiusKm, 2 * std::asin(clampd(b.radiusKm / dist, 0, 1)) / DEG, sys.classString().c_str(),
                sys.lightAt(P, game.testGameTime()).factor);
        writePNG("shots/tests/rogue_arrival.png", game.output(), FBW, FBH);
    }
    game.testParkAt(d, 0, false); run(0.2);
    if (game.state == GameState::LANDING_MAP) press(KEY_ESCAPE);
    run(0.5);
    printf("parked: the ship %.0f km off the drifter's centre -> shots/tests/rogue_parked.png\n", length(game.testShipPos() - game.testSystem().star.pos));
    writePNG("shots/tests/rogue_parked.png", game.output(), FBW, FBH);
    game.testParkAt(d, 0); run(0.3);
    game.testLandCursor(20, 40); run(0.1); press(KEY_ENTER);
    for (int i = 0; i < 30 * 20 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
    if (game.state != GameState::SURFACE) { printf("the landing: not landed\n"); return 1; }
    run(1.0);
    const SurfaceEnvironment& e = game.testSurface().env;
    printf("landed at 20 N 40 E: the sun %.1f degrees up with light %.2f, the sky's brightness %.2f, the time of day %.2f, %+.0f C -> shots/tests/rogue_landing.png\n",
           e.sun.altitude / DEG, e.sun.lightFactor, e.skyBrightness, e.localTime, e.temperatureC);
    writePNG("shots/tests/rogue_landing.png", game.output(), FBW, FBH);
    return 0;
}
