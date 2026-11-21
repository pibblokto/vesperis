// J-02 (R-448): the unit's `seeding` group: a seeded world's layer (galaxy/made.h, life/seeding) on any thread and in any order, a map
// made with it and without, the generators that take it off, the guide's `seeded` and `clock` lines, the clock skipped, stepped and
// reloaded, a loan
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/seeding.h"
#include "core/parallel.h"
#include "galaxy/drainage.h"
#include "core/rng.h"
#include <fstream>
#include <sstream>

namespace {
std::string fileText(const std::string& path) { std::ifstream f(path, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }

// a seeding of a world at a hashed place (the test's own: the life code reads only its place and its clock in J-02)
Seeding seedingAt(uint64_t h, double clock) {
    Rng r(h);
    Seeding s; s.lat = r.range(-1.2, 1.2); s.lon = r.range(-PI, PI); s.line = "0,0,0/1/M0"; s.clock = clock;
    return s;
}

SeedRecord recordOf(const std::string& body, double lat, double lon, double clock, const std::string& edits) {
    SeedRecord r; r.body = body; r.lat = lat; r.lon = lon; r.line = body + "/M2/S3"; r.edits = edits; r.cell = 1; r.tempC = 31.5; r.light = 2; r.energy = 3; r.salt = 1;
    r.gen = GEN_VERSION; r.clock = clock;
    return r;
}

// the layer `seed test` puts on a world: a tint round a point
MadeWorld testLayerOf(const Body& b) { return madeTestLayer(StarSystem::bodyFromLatLon(0.2, 0.5), 0.45, MAT_SNOW, 0.85, RGB(0.85f, 0.25f, 0.8f)); }
}

void unitSeedingAir(const Check& check);
void unitSeedingLooks(const Check& check);

void unitSeeding(const Check& check) {
    StarSystem fs; int fb = -1;
    if (!firstBodyOfType(PT_DESERT, fs, fb)) { check("seeding: a desert world", false, "none in the scan region"); return; }
    const Body& b = fs.bodies[fb];
    const MadeWorld layer = testLayerOf(b);
    {   // the layers of forty worlds' seedings the same in any order and on any thread: the dye line (`seed worlds`') let go at hashed places of
        // forty worlds and in its own world's sea, each worked out (its verdict, its drift) and its world's layer made, forwards, backwards, on the pool
        StarSystem ls; int lb = -1, sp = -1;
        SeedLine line;
        const bool src = seedSource(ls, lb, sp, line);
        if (src) line = seedLineFor(ls, lb, sp, seedLineEdits(line.genes, SL_DYE));
        struct Case { int sys = 0, body = -1; std::vector<Seeding> s; double clock = 0; };
        std::vector<StarSystem> systems;
        for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return (int)systems.size() < 6 ? 1 : 0; })) systems.push_back(sys);
        systems.push_back(ls);
        std::vector<Case> cases;
        for (size_t si = 0; si < systems.size(); si++)
            for (const Body& w : systems[si].bodies) {
                if (!KINDS[w.type].landable || cases.size() >= 40) continue;
                Case c; c.sys = (int)si; c.body = w.index; c.clock = 3600 + 37.0 * cases.size();
                for (int k = 0; k < 1 + (int)(cases.size() % 3); k++) {
                    const Seeding h = seedingAt(mix64(w.seed ^ (uint64_t)k), 60.0 * k);
                    Seeding sd; sd.lat = h.lat; sd.lon = h.lon; sd.line = line.key; sd.clock = h.clock; sd.sent = line;
                    c.s.push_back(sd);
                }
                cases.push_back(c);
            }
        if (src && lb >= 0) {   // and the line's own sea, where it takes
            Case c; c.sys = (int)systems.size() - 1; c.body = lb; c.clock = 5000;
            double la = 0, lo = 0;
            if (seedPlaceTaking(ls, lb, line, la, lo)) { Seeding sd; sd.lat = la; sd.lon = lo; sd.line = line.key; sd.sent = line; c.s.push_back(sd); }
            if (!c.s.empty()) cases.push_back(c);
        }
        auto runCase = [&](const Case& c0) {
            Case c = c0;
            for (Seeding& sd : c.s) seedingRun(systems[c.sys], c.body, sd);
            const MadeWorld m = madeWorldOf(systems[c.sys], c.body, c.s, c.clock);
            std::string v = fmt("%016llx", (unsigned long long)m.key);
            for (const Seeding& sd : c.s) v += fmt(" %d/%s/%.6f", sd.verdict.fate, geneEditsText(sd.verdict.drift).c_str(), sd.verdict.pace);
            return v;
        };
        std::vector<std::string> fwd(cases.size()), rev(cases.size()), par(cases.size());
        for (size_t i = 0; i < cases.size(); i++) fwd[i] = runCase(cases[i]);
        for (size_t i = cases.size(); i-- > 0;) rev[i] = runCase(cases[i]);
        parallelFor((int)cases.size(), 1, [&](int i0, int i1) { for (int i = i0; i < i1; i++) par[i] = runCase(cases[i]); });
        int keyed = 0, took = 0, sent = 0;
        for (size_t i = 0; i < cases.size(); i++) {
            keyed += fwd[i].compare(0, 16, "0000000000000000") != 0;
            Case c = cases[i];
            for (Seeding& sd : c.s) { seedingRun(systems[c.sys], c.body, sd); sent++; took += sd.verdict.takes(); }
        }
        // and the planet function under a layer: the same samples serially and on the pool
        BodyGen g = BodyGen::make(b); g.made = layer;
        const int N = 64;
        std::vector<SurfaceSample> s1(N * N), s2(N * N);
        auto at = [&](int i) { return StarSystem::bodyFromLatLon(0.2 + ((i / N) - N / 2) * 0.012, 0.5 + ((i % N) - N / 2) * 0.012); };
        {   // (without the rivers: a sample would build its tile on the spot)
            DrainageOff off(true);
            for (int i = 0; i < N * N; i++) s1[i] = sampleSurface(g, at(i), 500);
        }
        parallelFor(N * N, 16, [&](int i0, int i1) { DrainageOff off(true); for (int i = i0; i < i1; i++) s2[i] = sampleSurface(g, at(i), 500); });
        int same = 0, tinted = 0, covered = 0;
        for (int i = 0; i < N * N; i++) {
            same += s1[i].material == s2[i].material && s1[i].madeTint == s2[i].madeTint && s1[i].albedo == s2[i].albedo && s1[i].height == s2[i].height;
            tinted += s1[i].madeTint > 0.5; covered += s1[i].material == MAT_SNOW;
        }
        check("seeding: the layer, any order", line.valid && fwd == rev && fwd == par && keyed > 0 && same == N * N && tinted > 0 && covered > 0,
              fmt("(%zu worlds' verdicts, drifts and layers alike in order, reversed and on the pool: %d of %d seedings took, %d layers drawn; %d of %d samples "
                  "alike, %d tinted, %d covered)", cases.size(), took, sent, keyed, same, N * N, tinted, covered));
    }
    {   // a map made with the layer differs; taken off, the map is the first again, and asked again it is the cache's
        SpaceRenderer sr;
        const PlanetMap& m0 = sr.mapFor(b);
        const std::vector<uint8_t> mat0 = m0.material, alb0 = m0.albedo; const std::vector<float> h0 = m0.height;
        sr.setMade(b.seed, layer);
        const PlanetMap& m1 = sr.mapFor(b);
        int moved = 0; for (size_t i = 0; i < mat0.size(); i++) moved += m1.material[i] != mat0[i] || m1.albedo[i] != alb0[i];
        const bool with = moved > 0 && !m1.mtint.empty() && m1.madeKey == layer.key && sr.genFor(b).made.key == layer.key && m1.height == h0;
        sr.setMade(b.seed, MadeWorld());
        const PlanetMap& m2 = sr.mapFor(b);
        const bool back = m2.material == mat0 && m2.albedo == alb0 && m2.height == h0 && m2.mtint.empty() && m2.madeKey == 0 && !sr.genFor(b).made.on();
        const int made = sr.mapStats.generated;
        sr.mapFor(b);
        check("seeding: a map with and without", with && back && sr.mapStats.generated == made,
              fmt("(%d of %zu texels moved under it, the heights kept; off, the first map's bytes; asked again, %d made)", moved, mat0.size(), sr.mapStats.generated - made));
    }
    {   // the generators the peoples, the sights and the rivers read take the layer off
        BodyGen g = BodyGen::make(b), tmp, tmp2;
        g.made = layer;
        int deadType = -1; StarSystem ds; int db = -1;
        for (int k = 0; k < PT_COUNT && deadType < 0; k++) if (deadKind(k) && KINDS[k].landable && firstBodyOfType(k, ds, db)) deadType = k;
        bool deadOff = deadType < 0;
        if (deadType >= 0) { BodyGen d = BodyGen::make(ds.bodies[db]), dt; d.made = layer; deadOff = !thenWorld(d, dt).made.on(); }
        const bool off = !unseasoned(g, tmp).made.on() && !thenWorld(g, tmp2).made.on() && deadOff;
        check("seeding: the world as generated", off, fmt("(unseasoned and thenWorld without it%s)", deadType >= 0 ? fmt(", a dead %s's too", KINDS[deadType].name).c_str() : ""));
    }
    const std::string key = Guide::bodyKey(fs.star.sx, fs.star.sy, fs.star.sz, fb);
    {   // the guide byte-stable with `seeded` lines; a loan carries none
        Guide G; G.clock = 1234.56789012345;
        G.addSeeding(recordOf(key, 0.123456789012345, -2.34567890123456, 1000.98765432109, "3=ACCG;8=-"));
        G.addSeeding(recordOf(key, -0.5, 1.25, 1100.5, ""));
        G.addSeeding(recordOf(Guide::bodyKey(1, 2, 3, 4), 1.0 / 3.0, 2.0 / 3.0, 7.0 / 3.0, "19=GTCA"));
        const std::string p1 = "shots/tests/unit_seeding_guide.txt", p2 = "shots/tests/unit_seeding_guide_2.txt";
        G.save(p1);
        Guide G2; G2.load(p1); G2.save(p2);
        const std::string t1 = fileText(p1), t2 = fileText(p2);
        bool alike = G2.seeded.size() == G.seeded.size();
        for (size_t i = 0; alike && i < G.seeded.size(); i++) alike = seededLine(G2.seeded[i]) == seededLine(G.seeded[i]) && G2.seeded[i].lat == G.seeded[i].lat;
        int lines = 0; for (size_t p = 0; (p = t1.find("\nseeded ", p)) != std::string::npos; p++) lines++;
        Guide borrower; borrower.clock = 50;
        borrower.importInbox(p1);
        Guide none; const std::string p3 = "shots/tests/unit_seeding_guide_none.txt";
        none.save(p3);
        const bool plain = fileText(p3).find("clock") == std::string::npos;
        remove(p1.c_str()); remove(p2.c_str()); remove(p3.c_str());   // (the unit's files stay its own)
        check("seeding: the guide's lines", t1 == t2 && alike && lines == 3 && t1.find("\nclock ") != std::string::npos && borrower.seeded.empty() && borrower.clock == 50 && plain,
              fmt("(%zu bytes the same saved again, %d seeded lines read back alike; a loan of them: %zu lines, the clock its own; none, no clock line)", t1.size(), lines,
                      borrower.seeded.size()));
    }
    // J-03: the dye line of `seed worlds` (the first felisian world's sea light-feeder, DYE red), a strain of the explorer's in a game's guide
    StarSystem ls; int lb = -1, sp = -1;
    SeedLine wild;
    const bool src = seedSource(ls, lb, sp, wild);
    const std::vector<GeneEdit> dyeEdits = src ? seedLineEdits(wild.genes, SL_DYE) : std::vector<GeneEdit>();
    const SeedLine dye = src ? seedLineFor(ls, lb, sp, dyeEdits) : SeedLine();
    const std::string srcKey = Guide::bodyKey(ls.star.sx, ls.star.sy, ls.star.sz, lb), lineKey = dye.key + "/S1";
    double seaLat = 0, seaLon = 0;
    if (src) seedPlaceTaking(ls, lb, dye, seaLat, seaLon);
    const Seeding dyeSeeding = src ? seedingFor(ls, lb, dye, seaLat, seaLon, 0) : Seeding();
    auto gameAt = [&](TestGame& g, const char* name) {
        g.savePrefix = fmt("shots/tests/unit_seeding_%s_save", name); g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = fmt("shots/tests/unit_seeding_%s_guide.txt", name);
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(dyeEdits);
        g.guide.strains[lineKey] = rec;
        g.testParkAt(ls.star, lb, false); g.setState(GameState::SPACE);
    };
    auto dyeRecord = [&](double clock) {
        SeedRecord r; r.body = srcKey; r.lat = seaLat; r.lon = seaLon; r.line = lineKey; r.edits = geneEditsText(dyeEdits);
        r.cell = CC_WATER; r.tempC = 30; r.light = CL_WHITE; r.energy = CE_BROTH; r.salt = 1; r.gen = GEN_VERSION; r.clock = clock;
        return r;
    };
    {   // the clock: skipped in one step, stepped by the frames (real seconds), reloaded: the same clock and layer; its first step ten seconds on
        TestGame A, B, C;
        const double pace = dyeSeeding.verdict.pace;
        for (TestGame* g : {&A, &B}) { gameAt(*g, g == &A ? "skip" : "step"); g->testClockSkip(1000); g->guide.addSeeding(dyeRecord(1000 - (MADE_STEP_S - 10 * pace) / pace)); }
        const int stepBefore = A.testMadeNow(lb).step;
        A.testClockSkip(30);
        Input in;
        for (int i = 0; i < 300; i++) { B.frame(in, 0.1); in.newFrame(); }   // thirty seconds of frames, the clock on by each
        const MadeWorld la = A.testMadeNow(lb), lb2 = B.testMadeNow(lb);
        A.guide.save(A.guidePath);
        C.savePrefix = "shots/tests/unit_seeding_load_save"; C.settingsPath = "shots/tests/test_settings.txt"; C.keysPath = "shots/tests/test_keys.txt";
        C.guidePath = "shots/tests/unit_seeding_load_guide.txt";
        C.newGame(); C.settings.cabin = false; applyTestScale(C);
        C.guide = Guide(); C.guide.load(A.guidePath);
        C.testParkAt(ls.star, lb, false); C.setState(GameState::SPACE);
        const MadeWorld lc = C.testMadeNow(lb);
        const bool held = B.testMadeKeyHeld(lb) == lb2.key && B.testGenMadeKey(lb) == lb2.key;
        for (const TestGame* g : {&A, &B, &C}) remove(g->guidePath.c_str());
        check("seeding: the clock",
              dyeSeeding.verdict.takes() && stepBefore == 0 && la.step == 1 && la.key && la.key == lb2.key && la.key == lc.key && std::fabs(A.guide.clock - B.guide.clock) < 1e-6 &&
                      std::fabs(A.guide.clock - C.guide.clock) < 1e-6 && held,
              fmt("(the dye line in its world's sea at pace %.2f, %s; %.6f s skipped, %.6f stepped, %.6f reloaded; its step %d then %d, one layer; the generators %s)", pace,
                  seedVerdictWords(dyeSeeding.verdict).c_str(), A.guide.clock, B.guide.clock, C.guide.clock, stepBefore, la.step, held ? "hold it" : "DO NOT HOLD IT"));
    }
    // J-03: the six lines on forty worlds (`seed worlds`'), worked out serially and on the pool
    const std::vector<SeedCase> cases = seedCases();
    SeedLine lines[SL_COUNT];
    for (int k = 0; k < SL_COUNT && src; k++) lines[k] = seedLineFor(ls, lb, sp, seedLineEdits(wild.genes, k));
    std::vector<Seeding> runs(cases.size() * SL_COUNT), again(runs.size());
    for (size_t i = 0; i < runs.size(); i++) runs[i] = seedingFor(cases[i / SL_COUNT].sys, cases[i / SL_COUNT].body, lines[i % SL_COUNT], cases[i / SL_COUNT].lat, cases[i / SL_COUNT].lon, 0);
    parallelFor((int)runs.size(), 1, [&](int i0, int i1) {
        for (int i = i0; i < i1; i++) again[i] = seedingFor(cases[i / SL_COUNT].sys, cases[i / SL_COUNT].body, lines[i % SL_COUNT], cases[i / SL_COUNT].lat, cases[i / SL_COUNT].lon, 0);
    });
    {   // the verdict as a flask's: where the medium is the first failure its flask in the place's medium fails the same way; where it passed, its
        // flask grows there (a line needing no prey: a grazer's or a predator's food is the natives)
        int agree = 0, cases2 = 0, grew = 0, failed = 0;
        std::string bad;
        for (const Seeding& sd : runs) {
            const SeedVerdict& V = sd.verdict;
            if (V.fate == SF_FURNACE || sd.sent.kind.s.way == LW_GRAZER || sd.sent.kind.s.way == LW_PREDATOR) continue;
            FlaskCulture c; c.kind[0] = sd.sent.kind;
            CultureEvent e; e.kind = CD_START; e.t = 0; e.m = V.medium; c.events.push_back(e);
            const CultureRun r = runCulture(c);
            const CultureOne o = cultureAt(c, r, 1800).one[0];
            const bool medium = V.fate >= SF_SOLVENT && V.fate <= SF_BURNT;
            const bool ok = medium ? o.fit != GF_GROWS && o.fit == growthIn(sd.sent.kind, V.medium).fit : o.fit == GF_GROWS && o.cells > 0;
            cases2++; agree += ok; (medium ? failed : grew) += ok;
            if (!ok && bad.empty()) bad = fmt("; NOT ALIKE: %s in %s (fit %d)", SEED_FATE_NAMES[V.fate], CULTURE_CELL_NAMES[V.medium.cell], o.fit);
        }
        check("seeding: the verdict as a flask's", cases2 > 100 && agree == cases2 && grew > 0 && failed > 0,
              fmt("(%d of %d seedings of six lines on %zu worlds as their flask in the place's medium: %d grow there, %d fail as it fails%s)", agree, cases2, cases.size(), grew, failed,
                  bad.c_str()));
    }
    {   // the drift: fixed by the world, the line and the place (serially and on the pool alike; another place, another drift)
        int alike = 0, drifted = 0, took = 0, moved = 0, cmp = 0;
        for (size_t i = 0; i < runs.size(); i++) {
            alike += geneEditsText(runs[i].verdict.drift) == geneEditsText(again[i].verdict.drift) && runs[i].verdict.fate == again[i].verdict.fate &&
                     runs[i].verdict.pace == again[i].verdict.pace;
            if (!runs[i].verdict.takes()) continue;
            took++; drifted += !runs[i].verdict.drift.empty();
            const SeedCase& c = cases[i / SL_COUNT];
            const std::vector<GeneEdit> other = seedDriftOf(c.sys.bodies[c.body].seed, runs[i].sent.key, runs[i].verdict.place.hab.place + 1, runs[i].sent.genes, runs[i].verdict.doseGy);
            cmp++; moved += geneEditsText(other) != geneEditsText(runs[i].verdict.drift);
        }
        check("seeding: the drift", alike == (int)runs.size() && took > 0 && drifted > 0 && drifted < took && moved > 0,
              fmt("(%d of %zu verdicts and drifts alike serially and on the pool; %d of %d that took changed in the wild; another place moved %d of %d drifts)", alike, runs.size(), drifted,
                  took, moved, cmp));
    }
    {   // the trip: S at the bay over the dye line's world, its sea on the page, Enter: drone 1 down, the culture let go at its place (the guide's
        // `seeded` line, SEEDED in the log), its report at the dock; the flask keeps its culture, the bay charges; a save in the fall reloaded
        // carries the trip on; the world's layer drawn by its first steps, the data sheet's MADE
        TestGame g; gameAt(g, "trip");
        FlaskSpecies w; w.sx = ls.star.sx; w.sy = ls.star.sy; w.sz = ls.star.sz; w.body = lb; w.species = sp; w.strain = 1;
        const bool full = labFlaskFull(g, 0, w, seedBestMedium(dye.kind));
        g.openScreen(GameState::DRONES);
        Input in;
        auto press = [&](TestGame& G, int k) { in.pressed[k] = in.down[k] = true; G.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
        press(g, KEY_S);
        const bool page = g.state == GameState::SEEDING;
        const std::string pageNo = page ? std::string() : g.testStatus();
        g.testSeedPage(seaLat, seaLon);
        press(g, KEY_ENTER);
        const bool out = g.testBay().out && g.testBay().seedTrip && g.state == GameState::DRONES;
        g.testDronesSkip(6);
        g.saveSlot(1);
        TestGame h; h.savePrefix = g.savePrefix; h.settingsPath = g.settingsPath; h.keysPath = g.keysPath; h.guidePath = g.guidePath;
        h.guide.load(h.guidePath);
        const bool loaded = h.loadSlot(1) && h.testBay().seedTrip && h.testBay().out && !h.testBay().seedLet && h.testBay().seedLine == lineKey;
        h.testDronesSkip(30);
        const bool let = h.guide.seeded.size() == 1 && h.testBay().seedLet;
        int logged = 0;
        for (const LogEntry& e : h.guide.log) logged += e.kind == "SEEDED";
        h.testDronesSkip(400);
        const std::string report = h.testStatus();
        const bool back = !h.testBay().out && report.find("CULTURE GROWS") != std::string::npos && h.testLab().flasks[0].n == 1 && !h.testBayReady();
        h.testClockSkip(1800);
        const std::string made = h.testMadeWords(lb);
        const bool drawn = h.testMadeNow(lb).on() && made.find("RED") != std::string::npos;
        check("seeding: the trip", full && page && out && loaded && let && logged == 1 && back && drawn,
              fmt("(a FULL flask %s, the page %s, the drone %s; a save in its fall reloaded %s; let go %s, %d SEEDED in the log; back: '%s'; half an hour on: %s)", full ? "grown" : "NOT GROWN",
                  page ? "open" : ("NOT OPEN: " + pageNo).c_str(), out ? "out" : "NOT OUT", loaded ? "on its way" : "NOT CARRIED ON", let ? "in the guide" : "NOT", logged, report.c_str(),
                  made.empty() ? "NOTHING DRAWN" : made.c_str()));
        remove(g.guidePath.c_str()); remove(g.slotPath(1).c_str());   // (the unit's files stay its own)
    }
    {   // refused: at home, at a giant, out of orbit, with the drones away
        TestGame g; gameAt(g, "home");
        Input in;
        auto pressS = [&]() { in.pressed[KEY_S] = in.down[KEY_S] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[KEY_S] = false; return g.testStatus(); };
        g.newGame(); g.setState(GameState::SPACE);   // (parked at home, as a new game is)
        g.openScreen(GameState::DRONES);
        const std::string home = pressS();
        const bool stayed = g.state == GameState::DRONES;
        StarSystem gs; int giant = -1;
        for (int type = 0; type < PT_COUNT && giant < 0; type++) if (kindHas(type, KT_GIANT)) firstBodyOfType(type, gs, giant);
        std::string atGiant = "(no giant)";
        if (giant >= 0) { g.testParkAt(gs.star, giant, false); g.setState(GameState::SPACE); g.openScreen(GameState::DRONES); atGiant = pressS(); }
        g.setState(GameState::SPACE); g.testUnpark(); g.openScreen(GameState::DRONES);
        const std::string away = pressS();
        check("seeding: refused at home",
              home.find("HOME STAYS AS IT IS") != std::string::npos && stayed && (giant < 0 || atGiant.find("NO GROUND") != std::string::npos) && away.find("NOT IN ORBIT") != std::string::npos,
              fmt("(home: '%s', the bay %s; a giant: '%s'; out of orbit: '%s')", home.c_str(), stayed ? "stays" : "LEFT", atGiant.c_str(), away.c_str()));
        remove(g.guidePath.c_str());
    }
    {   // a loan leaves the borrower's world unchanged, and an unseeded world is as generated: a guide with the dye line in the sea lent to a
        // fresh game parked at the world; in the lender's game the other worlds of its system keep no layer and their maps' bytes
        TestGame A; gameAt(A, "lender");
        A.testClockSkip(4000);
        std::vector<std::vector<uint8_t>> before(ls.bodies.size());
        for (const Body& o : ls.bodies) if (o.index != lb && KINDS[o.type].landable) { A.testWaitMapOf(o.index); before[o.index] = A.testMapOf(o.index).material; }
        A.guide.addSeeding(dyeRecord(100));
        A.guide.save(A.guidePath);
        TestGame B; gameAt(B, "borrower");
        B.guide.importInbox(A.guidePath);
        B.testClockSkip(0);
        const bool lent = A.testMadeNow(lb).on() && !B.testMadeNow(lb).on() && B.guide.seeded.empty() && !B.testGenMadeKey(lb);
        A.testClockSkip(0);
        int others = 0, plain = 0;
        for (const Body& o : ls.bodies) {
            if (o.index == lb || !KINDS[o.type].landable) continue;
            others++;
            A.testWaitMapOf(o.index);
            plain += !A.testGenMadeKey(o.index) && A.testMapOf(o.index).material == before[o.index];
        }
        check("seeding: a loan, an unseeded world", lent && others > 0 && plain == others,
              fmt("(the lender's world drawn %s, the borrower's %s with %zu seeded lines; %d of %d other worlds of its system as generated, their maps' bytes)",
                  A.testMadeNow(lb).on() ? "made" : "PLAIN", B.testMadeNow(lb).on() ? "MADE" : "as generated", B.guide.seeded.size(), plain, others));
        remove(A.guidePath.c_str()); remove(B.guidePath.c_str());
    }
    {   // samples: a vial of the sea where the line spread holds it (its natives it beat fewer); one taken before it went down holds none
        TestGame g; gameAt(g, "vial");
        g.testClockSkip(5000);
        g.guide.addSeeding(dyeRecord(5000 - 1800));
        const std::vector<Seeding>& ss = g.testSeedings(lb);
        Vial v; v.sx = ls.star.sx; v.sy = ls.star.sy; v.sz = ls.star.sz; v.body = lb; v.unit = StarSystem::bodyFromLatLon(seaLat, seaLon);
        v.hab = ss.empty() ? Microhabitat() : ss[0].verdict.place.hab; v.clock = 5000;
        const Biota& B = lifeOf(ls)->bodies[lb];
        Sample plainS = sampleOf(B, v.hab), seededS = plainS;
        const std::vector<TestGame::VialLine> found = g.testVialLines(v, seededS);
        double before = 0, after = 0;
        for (const Held& h : plainS.held) before += h.cells;
        for (const Held& h : seededS.held) after += h.cells;
        Vial early = v; early.clock = 5000 - 1900;
        Sample earlyS = sampleOf(B, v.hab);
        const size_t none = g.testVialLines(early, earlyS).size();
        check("seeding: samples", found.size() == 1 && found[0].key == lineKey && none == 0 && after <= before,
              fmt("(the sea where it spread: %zu line%s (%s), the natives' cells %.3g, before it %.3g; a vial from before it went down: %zu)", found.size(), found.size() == 1 ? "" : "s",
                  found.empty() ? "-" : found[0].key.c_str(), after, before, none));
        remove(g.guidePath.c_str());
    }
    unitSeedingAir(check);   // J-04
    unitSeedingLooks(check);   // J-05
}

// J-04: the made air. Each gas a world's own sea line gives (`seed climate`'s twenty worlds, eight gases) at the whole world, worked out serially
// and on the pool; each world answering by its kind's rules; the air after the looks; the made climate's reads (the caps and the snow, the
// ground's heights, the HUD's TEMP and ATM); a runaway's seas dried to their old beds; the data sheet's words
void unitSeedingAir(const Check& check) {
    const std::vector<SeedAirCase> worlds = seedAirCases(20);
    struct Run { int w = 0, gas = 0; Seeding s; MadeWorld m; };
    std::vector<Run> runs;
    for (size_t wi = 0; wi < worlds.size(); wi++)
        for (int gas = WG_CO2; gas < WG_COUNT; gas++) { Run r; r.w = (int)wi; r.gas = gas; runs.push_back(r); }
    auto work = [&](Run& r) {
        const SeedAirCase& c = worlds[r.w];
        const SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, {{GN_GAS, r.gas}}));
        r.s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
        const std::vector<Seeding> one = {r.s};
        r.m = madeWorldOf(c.sys, c.body, one, (MADE_STEPS * MADE_STEP_S + 1) / std::max(0.1, r.s.verdict.pace) + 14 * MADE_STEP_S);
    };
    std::vector<Run> pool = runs;
    for (Run& r : runs) work(r);
    parallelFor((int)pool.size(), 1, [&](int i0, int i1) { for (int i = i0; i < i1; i++) work(pool[i]); });
    {   // the same on any thread; each gas answered by its world's kind (its air, oxygen in it, its seas, its pressure)
        int alike = 0, took = 0, aired = 0, bad = 0;
        std::string firstBad;
        for (size_t i = 0; i < runs.size(); i++) {
            const Run& r = runs[i];
            alike += r.m.key == pool[i].m.key && r.m.warmC == pool[i].m.warmC && r.m.air == pool[i].m.air;
            if (!r.s.verdict.takes()) continue;
            took++; aired += r.m.aired();
            const Body& b = worlds[r.w].sys.bodies[worlds[r.w].body];
            const KindDef& K = KINDS[b.type];
            const bool air = bodyHasAir(b), oxygen = air && (K.shapes & KS_OXYGEN_AIR), wet = (K.tags & KT_SEA) != 0;
            const double P = K.pressureSpan > 0 ? K.pressureLo + K.pressureSpan * hash01(b.seed) : 0;
            std::string why;
            if (!air && r.m.aired()) why = "an airless world's air moved";
            else if (r.gas == WG_CO2 && r.s.verdict.share > 0.05 && !(r.m.warmC > 0)) why = "CO2 did not warm it";
            else if (r.gas == WG_METHANE && oxygen && (r.m.warmC > 5 || (r.m.air & MA_ORANGE_HAZE))) why = "methane did not burn in oxygen";
            else if (r.gas == WG_OXYGEN && oxygen && r.m.air) why = "oxygen changed an oxygen air";
            else if ((r.m.air & MA_OZONE) && oxygen) why = "an ozone formed in an oxygen air";
            else if ((r.m.air & MA_THIN_OZONE) && !oxygen) why = "an ozone thinned where there was none";
            else if ((r.m.air & MA_SEA_SMOKE) && !wet) why = "sea smoke with no seas";
            else if (r.gas == WG_DMS && r.m.warmC > 0) why = "dimethyl sulphide warmed it";
            else if (r.gas == WG_HYDROGEN && P < 1 && r.m.warmC != 0) why = "hydrogen warmed a thin air";
            if (!why.empty()) { bad++; if (firstBad.empty()) firstBad = fmt("; %s (%s, %s)", why.c_str(), K.name, WORLD_GAS_NAMES[r.gas]); }
        }
        check("seeding: the air by its world", alike == (int)runs.size() && took > 40 && aired > 20 && bad == 0,
              fmt("(%d of %zu seedings of eight gases on %zu worlds alike serially and on the pool; %d took, %d moved their air; %d answered against their world's kind%s)", alike,
                  runs.size(), worlds.size(), took, aired, bad, firstBad.c_str()));
    }
    // a CO2 line that warms its world the most of the twenty: its air at its steps, its climate's reads
    const Run* hot = nullptr;
    for (const Run& r : runs) if (r.gas == WG_CO2 && r.s.verdict.takes() && livingGround(worlds[r.w].sys.bodies[worlds[r.w].body].type) && !(r.m.air & MA_RUNAWAY) && (!hot || r.m.warmC > hot->m.warmC)) hot = &r;
    const Run* runaway = nullptr;
    for (const Run& r : runs) if (!runaway && (r.m.air & MA_RUNAWAY) && r.m.dried()) runaway = &r;
    {   // the air after the looks: none before the line held its region (step 6), more each step, all of it with the whole world
        bool after = hot != nullptr, more = hot != nullptr;
        double was = 0, at5 = 0, at24 = 0;
        if (hot) {
            const SeedAirCase& c = worlds[hot->w];
            for (int st = 1; st <= MADE_STEPS; st++) {
                const std::vector<Seeding> one = {hot->s};
                const MadeWorld m = madeWorldOf(c.sys, c.body, one, (st * MADE_STEP_S + 1) / hot->s.verdict.pace);
                if (st == 5) at5 = m.warmC;
                if (st == MADE_STEPS) at24 = m.warmC;
                if (st <= 6 && m.aired()) after = false;
                if (m.warmC < was) more = false;
                was = m.warmC;
            }
        }
        check("seeding: the air after the looks", after && more && at24 > 0,
              fmt("(a CO2 line's world %.1f C warmer at step 5, %.1f at step 24%s%s)", at5, at24, after ? "" : "; MOVED BY ITS REGION", more ? "" : "; COLDER A STEP ON"));
    }
    {   // the made climate's reads: the caps and the snow drawn back, every height but the ice's as generated; on the ground TEMP warmer by the
        // made warmth and ATM by its pressure
        int snowBefore = 0, snowAfter = 0, moved = 0, n = 0;
        double dT = 0, dP = 0;
        bool landed = false;
        if (hot) {
            const SeedAirCase& c = worlds[hot->w];
            const Body& b = c.sys.bodies[c.body];
            const BodyGen g0 = BodyGen::make(b);
            BodyGen g1 = g0; g1.made = hot->m;
            for (int j = 0; j < 36; j++)
                for (int i = 0; i < 72; i++) {
                    const Vec3 u = StarSystem::bodyFromLatLon((j + 0.5) / 36 * PI - PI / 2, (i + 0.5) / 72 * TAU - PI);
                    const SurfaceSample a = sampleSurface(g0, u, 40000), z = sampleSurface(g1, u, 40000);
                    const bool ia = a.material == MAT_SNOW || a.material == MAT_ICE, iz = z.material == MAT_SNOW || z.material == MAT_ICE;
                    snowBefore += ia; snowAfter += iz; n++;
                    if (!ia && !iz && a.height != z.height) moved++;
                }
            TestGame g;
            g.savePrefix = "shots/tests/unit_seeding_air_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
            g.guidePath = "shots/tests/unit_seeding_air_guide.txt";
            remove(g.guidePath.c_str());
            g.newGame(); g.settings.cabin = false; applyTestScale(g);
            Input in;
            double la = 0, lo = 0;
            coastSite(g0, b.radiusKm, la, lo);
            auto land = [&](double& T, double& P) {
                g.testSetTime(30000); g.testParkAt(c.sys.star, c.body); g.testLandCursor(la, lo);
                in.pressed[KEY_ENTER] = in.down[KEY_ENTER] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[KEY_ENTER] = false;
                for (int k = 0; k < 30 * 40 && g.state != GameState::SURFACE; k++) runFramesAtLeastOne(g, in, 1.0 / 30);
                runFramesAtLeastOne(g, in, 0.2);
                T = g.testSurface().env.temperatureC; P = g.testSurface().env.pressureAtm;
                return g.state == GameState::SURFACE;
            };
            double T0 = 0, P0 = 0, T1 = 0, P1 = 0;
            landed = land(T0, P0);
            g.testMadeSet(c.body, hot->m);
            landed = land(T1, P1) && landed;
            dT = T1 - T0; dP = P0 > 0 ? P1 / P0 : 0;
            remove(g.guidePath.c_str()); remove(g.slotPath(0).c_str());   // (the unit's files stay its own: the landing's autosave too)
        }
        check("seeding: the made climate's reads",
              hot && snowAfter < snowBefore && moved == 0 && landed && std::fabs(dT - hot->m.warmC) < 0.05 && std::fabs(dP - hot->m.pressureK) < 0.005,
              fmt("(a CO2 line %.1f C over a %s: its snow and ice on %d of %d points, then %d; %d heights moved but the ice's; on the ground TEMP %+.2f C and ATM x%.3f, its layer "
                  "%+.1f C and x%.2f)", hot ? hot->m.warmC : 0.0, hot ? KINDS[worlds[hot->w].sys.bodies[worlds[hot->w].body].type].name : "-", snowBefore, n, snowAfter, moved, dT, dP,
                  hot ? hot->m.warmC : 0.0, hot ? hot->m.pressureK : 0.0));
    }
    {   // a runaway's seas gone to their old beds, their floors where they lay; the data sheet's words
        int wet = 0, dried = 0, same = 0;
        std::string made, airW;
        if (runaway) {
            const SeedAirCase& c = worlds[runaway->w];
            const BodyGen g0 = BodyGen::make(c.sys.bodies[c.body]);
            BodyGen g1 = g0; g1.made = runaway->m;
            for (int j = 0; j < 24; j++)
                for (int i = 0; i < 48; i++) {
                    const Vec3 u = StarSystem::bodyFromLatLon((j + 0.5) / 24 * PI - PI / 2, (i + 0.5) / 48 * TAU - PI);
                    const SurfaceSample a = sampleSurface(g0, u, 40000);
                    if (!madeWetAt(a) || a.waterKind != 0) continue;
                    const SurfaceSample z = sampleSurface(g1, u, 40000);
                    wet++; dried += !madeWetAt(z) && z.oldSea > 0.5; same += a.height == z.height;
                }
            for (const std::string& t : madeTraitWords(runaway->m, true)) made += (made.empty() ? "" : ", ") + t;
            airW = madeAirWords(runaway->m);
        }
        check("seeding: a runaway's seas", runaway && wet > 0 && dried == wet && same == wet && made.find("RUNAWAY") != std::string::npos && airW.find("THICKER") != std::string::npos &&
                      airW.find("C WARMER THAN IT WAS") != std::string::npos,
              fmt("(%s: %d of %d points of its sea dry, %d floors where they lay; MADE %s; AIR %s)", runaway ? KINDS[worlds[runaway->w].sys.bodies[worlds[runaway->w].body].type].name : "none",
                  dried, wet, same, made.empty() ? "-" : made.c_str(), airW.empty() ? "-" : airW.c_str()));
    }
}

// J-05: the made waters, ground and plants. The genes `seed looks` sets (a sulphide, a black dye, a chalk shell, lift and slime, a jelly and a
// soup, CO2, a rust shell; a dried line's crusts, chalk, glass, sulphur, slime; a stone rotter; a wood rotter; a host of plants with a frost,
// a carrier, a dye; lift and frost; nitrous oxide) on twenty worlds' own sea lines at the whole world, serially and on the pool; each answering
// by its world's kind; the looks where they lie (on their own ground, no height moved, the covers on the land); the ground's banks and a host's
// galls; the data sheet's words
void unitSeedingLooks(const Check& check) {
    const std::vector<SeedAirCase> worlds = seedAirCases(20);
    const std::vector<std::vector<std::pair<int, int>>> RECIPES = {
        {{GN_GAS, WG_H2S}}, {{GN_DYE, WD_BLACK}}, {{GN_SHELL, SHL_CHALK}}, {{GN_LIFT, -1}, {GN_SLIME, -1}}, {{GN_STOP, -2}, {GN_SLIME, -1}}, {{GN_STOP, -2}},
        {{GN_GAS, WG_CO2}}, {{GN_SHELL, SHL_RUST}}, {{GN_DRY, -1}, {GN_BIND, -1}}, {{GN_DRY, -1}, {GN_SHELL, SHL_CHALK}}, {{GN_DRY, -1}, {GN_SHELL, SHL_GLASS}},
        {{GN_DRY, -1}, {GN_SHELL, SHL_SULPHUR}}, {{GN_ROT, ROT_STONE}}, {{GN_DRY, -1}, {GN_SLIME, -1}}, {{GN_ROT, ROT_WOOD}}, {{GN_HOST, HO_PLANTS}, {GN_FROST, -1}},
        {{GN_HOST, HO_PLANTS}, {GN_CARRIER, -1}}, {{GN_HOST, HO_PLANTS}, {GN_DYE, WD_RED}}, {{GN_HOST, HO_PLANTS}, {GN_CARRIER, -1}, {GN_DYE, WD_RED}},
        {{GN_LIFT, -1}, {GN_FROST, -1}}, {{GN_GAS, WG_N2O}}, {{GN_GAS, WG_OXYGEN}}};
    struct Run { int w = 0, r = 0; Seeding s; MadeWorld m; };
    std::vector<Run> runs;
    for (size_t wi = 0; wi < worlds.size(); wi++)
        for (size_t ri = 0; ri < RECIPES.size(); ri++) { Run r; r.w = (int)wi; r.r = (int)ri; runs.push_back(r); }
    auto work = [&](Run& r) {
        const SeedAirCase& c = worlds[r.w];
        const SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, RECIPES[r.r]));
        r.s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
        const std::vector<Seeding> one = {r.s};
        r.m = madeWorldOf(c.sys, c.body, one, (MADE_STEPS * MADE_STEP_S + 1) / std::max(0.1, r.s.verdict.pace) + 2 * MADE_STEP_S);
    };
    std::vector<Run> pool = runs;
    for (Run& r : runs) work(r);
    parallelFor((int)pool.size(), 1, [&](int i0, int i1) { for (int i = i0; i < i1; i++) work(pool[i]); });
    {   // the same on any thread; each look answered by its world's kind (its plants, its seas, its air, its reefs), each named one drawn
        int alike = 0, took = 0, made = 0, bad = 0;
        std::string firstBad;
        for (size_t i = 0; i < runs.size(); i++) {
            const Run& r = runs[i];
            const MadeWorld& m = r.m;
            alike += m.key == pool[i].m.key && m.seas == pool[i].m.seas && m.grounds == pool[i].m.grounds && m.flora == pool[i].m.flora;
            if (!r.s.verdict.takes()) continue;
            took++; made += m.j05();
            const Body& b = worlds[r.w].sys.bodies[worlds[r.w].body];
            const KindDef& K = KINDS[b.type];
            const bool oxygen = bodyHasAir(b) && (K.shapes & KS_OXYGEN_AIR), waterSeas = (K.tags & KT_SEA) && b.type != PT_HYDROCARBON && b.type != PT_AMMONIA;
            const bool flora = (K.tags & KT_FLORA) != 0;
            std::string why;
            if (m.flora && !flora) why = "a plants' look on a world without plants";
            else if ((m.seas & (MS_PURPLE | MS_SOUR)) && !waterSeas) why = "a sulphide's or a sour sea's look with no seas of water";
            else if ((m.seas & MS_SOUR) && !r.s.verdict.reefs) why = "a sour sea with no reefs";
            else if (m.rustGround > 0 && oxygen) why = "an oxygen air's ground rusted";
            else if ((m.lush > 0 || m.bleached > 0) && !(flora && livingGround(b.type))) why = "lush or bleached plants on a world without living plants";
            else if ((m.seas & MS_MILK) && m.waterLook != MWL_MILK) why = "milk seas named, not drawn";
            else if ((m.seas & MS_SOUP) && m.waterLook != MWL_SOUP) why = "soup seas named, not drawn";
            else if ((m.seas & MS_JELLY) && (m.waterLook != MWL_JELLY || m.jelly <= 0)) why = "jelly seas named, not drawn";
            else if ((m.seas & MS_FOAM) && m.foam <= 0) why = "foam seas named, not drawn";
            else if ((m.seas & MS_RUST_RIVERS) && !(m.waterLook == MWL_RUST && m.spread[0].lookWaters == MADE_FRESH)) why = "rust rivers off their fresh waters";
            else if (m.grounds && m.grounds != MGT_RUST && !m.land.on && !m.snow.on) why = "a ground's look named, not drawn";
            else if ((m.flora & ~(MP_LUSH | MP_BLEACHED)) && !m.plant.on) why = "a plants' look named, not drawn";
            if (!why.empty()) { bad++; if (firstBad.empty()) firstBad = fmt("; %s (%s, recipe %d)", why.c_str(), K.name, r.r); }
        }
        check("seeding: the looks by their world", alike == (int)runs.size() && took > 100 && made > 60 && bad == 0,
              fmt("(%d of %zu seedings of %zu gene sets on %zu worlds alike serially and on the pool; %d took, %d changed waters, ground or plants; %d against their world%s)",
                  alike, runs.size(), RECIPES.size(), worlds.size(), took, made, bad, firstBad.c_str()));
    }
    // a land look with a cover and a plants' look, the first of the runs (the climate unmoved, so no ice moves a height)
    const Run* landRun = nullptr; const Run* plantRun = nullptr;
    for (const Run& r : runs) {
        if (!landRun && r.m.land.on && r.m.spread[0].cover >= 0 && !r.m.aired()) landRun = &r;
        if (!plantRun && r.m.plant.tinted && r.m.galls > 0 && !r.m.aired()) plantRun = &r;
    }
    {   // where they lie: no height moved, a cover only on the land, every share on its own class's ground
        int n = 0, moved = 0, coveredBad = 0, covered = 0, shared = 0, offClass = 0;
        for (const Run* rp : {landRun, plantRun}) {
            if (!rp) continue;
            const SeedAirCase& c = worlds[rp->w];
            const BodyGen g0 = BodyGen::make(c.sys.bodies[c.body]);
            BodyGen g1 = g0; g1.made = rp->m;
            for (int j = 0; j < 36; j++)
                for (int i = 0; i < 72; i++) {
                    const Vec3 u = StarSystem::bodyFromLatLon((j + 0.5) / 36 * PI - PI / 2, (i + 0.5) / 72 * TAU - PI);
                    const SurfaceSample a = sampleSurface(g0, u, 40000), z = sampleSurface(g1, u, 40000);
                    n++; moved += a.height != z.height;
                    if (z.material != a.material) { covered++; coveredBad += madeWetAt(a) || a.material == MAT_LAVA || z.material == MAT_WATER; }
                    if (z.madeTint <= 0) continue;
                    shared++;
                    const int cls = madeClassOf(g1, z.material, madeWetAt(z));
                    const bool ok = cls == MC_WATER ? (rp->m.tinted && rp->m.tintOn == MADE_ON_WATER) : rp->m.lookOf(cls)->on;
                    offClass += !ok;
                }
        }
        check("seeding: the looks where they lie", landRun && plantRun && moved == 0 && coveredBad == 0 && covered > 0 && shared > 0 && offClass == 0,
              fmt("(%s's %s and %s's %s: %d points twice, %d heights moved, %d covered (%d on water or lava), %d shared, %d off their class's ground)",
                  landRun ? KINDS[worlds[landRun->w].sys.bodies[worlds[landRun->w].body].type].name : "-", landRun ? MADE_GROUND_NAMES[__builtin_ctz((unsigned)landRun->m.land.kind)] : "-",
                  plantRun ? KINDS[worlds[plantRun->w].sys.bodies[worlds[plantRun->w].body].type].name : "-", plantRun ? "NEW LEAVES AND GALLS" : "-", n, moved, covered, coveredBad, shared, offClass));
    }
    {   // on the ground: the land's and the plants' looks in banks of their own (lent ones the traits leave free), a host's galls on the trees
        int banks = 0, trees = 0, made = 0, galls = 0;
        bool landed = false;
        if (plantRun) {
            const SeedAirCase& c = worlds[plantRun->w];
            const Body& b = c.sys.bodies[c.body];
            TestGame g;
            g.savePrefix = "shots/tests/unit_seeding_looks_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
            g.guidePath = "shots/tests/unit_seeding_looks_guide.txt";
            remove(g.guidePath.c_str());
            g.newGame(); g.settings.cabin = false; applyTestScale(g);
            Input in;
            // a forest where the look lies: the sphere's first point of forest under it at the world map's scale
            double la = 0, lo = 0;
            {
                BodyGen gm = BodyGen::make(b); gm.made = plantRun->m;
                const DrainageOff off;
                for (int i = 0; i < 2000; i++) {
                    const double z = 1 - (2 * i + 1) / 2000.0, a = std::asin(z), o = std::fmod(i * 2.399963229728653, TAU) - PI;
                    const SurfaceSample s = sampleSurface(gm, StarSystem::bodyFromLatLon(a, o), 20000);
                    if (s.material == MAT_FOREST && s.madeTint > 0.9) { la = a / DEG; lo = o / DEG; break; }
                }
            }
            g.testSetTime(30000); g.testParkAt(c.sys.star, c.body); g.testLandCursor(la, lo);
            g.testMadeSet(c.body, plantRun->m);   // (keyed by its system's star: once parked there)
            in.pressed[KEY_ENTER] = in.down[KEY_ENTER] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[KEY_ENTER] = false;
            for (int k = 0; k < 30 * 40 && g.state != GameState::SURFACE; k++) runFramesAtLeastOne(g, in, 1.0 / 30);
            runFramesAtLeastOne(g, in, 0.3);
            landed = g.state == GameState::SURFACE && (la != 0 || lo != 0);
            SurfaceView& S = const_cast<SurfaceView&>(g.testSurface());
            for (int cls : {MC_LAND, MC_SNOW, MC_PLANTS}) banks += S.testMadeBank(cls) >= 0;
            const int cx0 = (int)std::floor(S.player.x / 16), cz0 = (int)std::floor(S.player.z / 16);
            for (int cz = cz0 - 12; cz <= cz0 + 12; cz++)
                for (int cx = cx0 - 12; cx <= cx0 + 12; cx++)
                    S.forTrees(cx, cz, [&](const TreeInst& T) { trees++; made += T.made; galls += T.galls; });
            remove(g.guidePath.c_str()); remove(g.slotPath(0).c_str());   // (the unit's files stay its own: the landing's autosave too)
        }
        const bool bankOk = plantRun && banks >= 1;
        check("seeding: the made ground drawn", landed && bankOk && trees > 0 && made > 0 && galls > 0 && galls <= made,
              fmt("(%d of three looks in banks of their own; %d trees round the landing, %d in the plants' look, %d with galls)", banks, trees, made, galls));
    }
    {   // the data sheet's words: every trait by its name, a coloured snow and new leaves by their dye, a black dye's waters named once
        MadeWorld m; m.key = 1;
        m.seas = (1 << MADE_SEA_TRAITS) - 1; m.grounds = (1 << MADE_GROUND_TRAITS) - 1; m.flora = (1 << MADE_PLANT_TRAITS) - 1;
        m.snow.dye = WD_RED; m.plant.dye = WD_VIOLET;
        m.tinted = true; m.tintOn = MADE_ON_WATER; m.dye = WD_BLACK;
        std::string w;
        for (const std::string& t : madeTraitWords(m, true)) w += (w.empty() ? "" : ", ") + t;
        int black = 0;
        for (size_t at = w.find("BLACK SEAS"); at != std::string::npos; at = w.find("BLACK SEAS", at + 1)) black++;
        const bool all = w.find("PURPLE SEAS") != std::string::npos && w.find("BOILING SEAS") != std::string::npos && w.find("RED SNOW") != std::string::npos &&
                         w.find("COLOURED SNOW") == std::string::npos && w.find("SLIME") != std::string::npos && w.find("VIOLET LEAVES") != std::string::npos &&
                         w.find("NEW LEAVES") == std::string::npos && w.find("BLEACHED") != std::string::npos;
        check("seeding: the looks' words", all && black == 1, fmt("(MADE %s)", w.c_str()));
    }
}
