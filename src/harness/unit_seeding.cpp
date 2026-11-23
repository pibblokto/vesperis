// J-02 (R-448): the unit's `seeding` group: a seeded world's layer (galaxy/made.h, life/seeding) on any thread and in any order, a map
// made with it and without, the generators that take it off, the guide's `seeded` and `clock` lines, the clock skipped, stepped and
// reloaded, a loan
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/seeding.h"
#include "core/parallel.h"
#include "galaxy/drainage.h"
#include "core/rng.h"
#include "core/sphere.h"
#include <algorithm>
#include <array>
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
void unitSeedingGrowth(const Check& check);
void unitSeedingRuins(const Check& check);
void unitSeedingKinds(const Check& check);
void unitSeedingRecord(const Check& check);

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
    unitSeedingGrowth(check);   // J-09
    unitSeedingRuins(check);    // J-10
    unitSeedingKinds(check);    // J-11 (unit_seeding_kinds.cpp)
    unitSeedingRecord(check);   // J-12 (unit_seeding_record.cpp)
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

// J-09: the overgrowth. Six gene sets on twelve worlds' own sea lines (never stopping and binding; with a chalk shell, a glass shell or lift;
// never stopping and sliming, its binding cut; binding alone), serially and on the pool: each type as its genes make it, none before its
// line's eighth step, its reach growing with its steps. The cover a pure function of place and step (the same twice and on the pool, never on
// water, snow, ice or lava, a point it covered covered at every later step). On a flora world's ground under each type over the whole world:
// every structure standing over a step walked round (its colliders at its foot), and no tree in a structure, the trees of its forest wrapped
void unitSeedingGrowth(const Check& check) {
    const std::vector<SeedAirCase> worlds = seedAirCases(12);
    const std::vector<std::vector<std::pair<int, int>>> RECIPES = {
        {{GN_STOP, -2}, {GN_BIND, -1}}, {{GN_STOP, -2}, {GN_BIND, -1}, {GN_SHELL, SHL_CHALK}}, {{GN_STOP, -2}, {GN_BIND, -1}, {GN_SHELL, SHL_GLASS}},
        {{GN_STOP, -2}, {GN_BIND, -1}, {GN_LIFT, -1}}, {{GN_STOP, -2}, {GN_SLIME, -1}, {GN_BIND, -2}}, {{GN_BIND, -1}}};
    static const int STEPS[5] = {7, 9, 14, 20, 32};
    struct Run { int w = 0, r = 0; Seeding s; SeedLine line; MadeWorld m[5]; };
    std::vector<Run> runs;
    for (size_t wi = 0; wi < worlds.size(); wi++)
        for (size_t ri = 0; ri < RECIPES.size(); ri++) { Run r; r.w = (int)wi; r.r = (int)ri; runs.push_back(r); }
    auto work = [&](Run& r) {
        const SeedAirCase& c = worlds[r.w];
        r.line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, RECIPES[r.r]));
        r.s = seedingFor(c.sys, c.body, r.line, c.lat, c.lon, 0);
        const std::vector<Seeding> one = {r.s};
        for (int k = 0; k < 5; k++) r.m[k] = madeWorldOf(c.sys, c.body, one, STEPS[k] * MADE_STEP_S / std::max(0.1, r.s.verdict.pace) + 1);
    };
    std::vector<Run> pool = runs;
    for (Run& r : runs) work(r);
    parallelFor((int)pool.size(), 1, [&](int i0, int i1) { for (int i = i0; i < i1; i++) work(pool[i]); });
    const Run* grown = nullptr;
    {
        int alike = 0, took = 0, grew = 0, bad = 0, types = 0;
        std::string firstBad;
        for (size_t i = 0; i < runs.size(); i++) {
            const Run& r = runs[i];
            bool same = true;
            for (int k = 0; k < 5; k++) same = same && r.m[k].key == pool[i].m[k].key && r.m[k].growths == pool[i].m[k].growths;
            alike += same;
            if (!r.s.verdict.takes()) continue;
            took++;
            const MadeWorld& m = r.m[4];
            const CultureWorld& w = r.line.kind.world;
            const int type = m.grown() ? m.spread[m.growthLine].growth.type : OG_NONE;
            types |= 1 << type;
            grew += type != OG_NONE;
            std::string why;
            const bool shouldGrow = w.overgrows && (w.bind > 0 || w.slime > 0);
            if (shouldGrow != (type != OG_NONE)) why = shouldGrow ? "a line that never stops and binds grew nothing" : "an overgrowth from a line that stops";
            else if (r.m[0].grown()) why = "an overgrowth before its line's eighth step";
            else if (type != OG_NONE) {
                const MadeGrowth& G = m.spread[m.growthLine].growth;
                const double reach[4] = {r.m[1].grown() ? r.m[1].spread[0].growth.reach : 0, r.m[2].spread[0].growth.reach, r.m[3].spread[0].growth.reach, G.reach};
                if (!(reach[0] > 0 && reach[0] < reach[1] && reach[1] < reach[2] && reach[2] <= reach[3])) why = "its reach not growing with its steps";
                else if (G.material != madeGrowthCover(G.type, G.hard)) why = "its cover not its type's";
                else if (w.bind <= 0 && G.type != OG_FILMS) why = "a slime without binding not films";
                else if (w.bind > 0 && w.shells() && w.shell == SHL_CHALK && G.type != OG_CORAL) why = "a chalk shell's not coral";
                else if (w.bind > 0 && w.shells() && w.shell == SHL_GLASS && G.type != OG_SPONGE) why = "a glass shell's not a sponge";
                else if (w.bind > 0 && !w.shells() && w.lift > 0 && G.type != OG_BLADDERS) why = "a lift's not bladders";
                else if (G.age != STEPS[4] - MADE_GROWTH_START + 1 && G.age != MADE_STEPS) why = "its age not its steps since it began";
                if (why.empty() && !grown && r.r == 0 && kindHas(worlds[r.w].sys.bodies[worlds[r.w].body].type, KT_FLORA)) grown = &r;
            }
            if (!why.empty()) { bad++; if (firstBad.empty()) firstBad = fmt("; %s (%s, recipe %d)", why.c_str(), KINDS[worlds[r.w].sys.bodies[worlds[r.w].body].type].name, r.r); }
        }
        std::string named;
        for (int t = 1; t < OG_COUNT; t++) if ((types >> t) & 1) named += (named.empty() ? "" : ", ") + std::string(MADE_GROWTH_NAMES[t]);
        check("seeding: the overgrowth by its genes", alike == (int)runs.size() && took > 30 && grew > 15 && bad == 0 && ((types >> OG_CORAL) & 1) && ((types >> OG_FILMS) & 1),
              fmt("(%d of %zu seedings at five steps alike serially and on the pool; %d took, %d grew: %s; %d against their genes%s)", alike, runs.size(), took, grew,
                  named.empty() ? "none" : named.c_str(), bad, firstBad.c_str()));
    }
    if (grown) {   // the cover over the sphere at its four steps: twice, on the pool, on its own ground, never taken back
        const SeedAirCase& c = worlds[grown->w];
        const BodyGen g0 = BodyGen::make(c.sys.bodies[c.body]);
        const int N = 600;
        std::vector<Vec3> pts(N);
        for (int i = 0; i < N; i++) { const double z = 1 - (2 * i + 1.0) / N, la = std::asin(z), lo = std::fmod(i * 2.399963229728653, TAU) - PI; pts[i] = StarSystem::bodyFromLatLon(la, lo); }
        int covered[5] = {}, differ = 0, offGround = 0, takenBack = 0;
        std::vector<int> last(N, 0);
        for (int k = 1; k < 5; k++) {
            BodyGen gm = g0; gm.made = grown->m[k];
            std::vector<int> a(N), b(N);
            for (int i = 0; i < N; i++) a[i] = sampleSurface(gm, pts[i], 16).material;
            parallelFor(N, 8, [&](int i0, int i1) { for (int i = i0; i < i1; i++) b[i] = sampleSurface(gm, pts[i], 16).material; });
            BodyGen bare = g0;
            for (int i = 0; i < N; i++) {
                differ += a[i] != b[i];
                const bool on = madeGrowthMaterial(a[i]);
                covered[k] += on;
                if (on) {
                    const SurfaceSample s0 = sampleSurface(bare, pts[i], 16);
                    if (s0.material == MAT_WATER || s0.material == MAT_SNOW || s0.material == MAT_ICE || s0.material == MAT_LAVA || madeWetAt(s0)) offGround++;
                }
                if (last[i] && !on) takenBack++;
                last[i] = on;
            }
        }
        const MadeGrowth& G = grown->m[4].spread[grown->m[4].growthLine].growth;
        check("seeding: the overgrowth's cover by place and step", differ == 0 && offGround == 0 && takenBack == 0 && covered[2] < covered[4] && covered[1] <= covered[2] &&
                                                                     covered[2] <= covered[3] && covered[3] <= covered[4],
              fmt("(a %s's on a %s, %d of %d points covered at its steps 9, 14, 20 and 32: %d, %d, %d, %d; %d differing on the pool, %d on water, snow, ice or lava, "
                  "%d taken back)", MADE_GROWTH_NAMES[G.type], KINDS[c.sys.bodies[c.body].type].name, covered[4], N, covered[1], covered[2], covered[3], covered[4], differ, offGround, takenBack));
    } else check("seeding: the overgrowth's cover by place and step", false, "(no line that never stops and binds grew on a flora world)");
    {   // the ground under each type: the structures walked round, no tree in one, the forest's trees wrapped
        GrowthGround land;
        if (!growthGround(land, false)) { check("seeding: the overgrowth's structures and trees", false, "(no flora world with a forest's edge)"); return; }
        int structures = 0, colliding = 0, unmatched = 0, walkedThrough = 0, trees = 0, wrapped = 0, inside = 0, shapes = 0, walked = 0, passed = 0;
        std::string firstBad;
        for (int type = 1; type < OG_COUNT; type++) {
            if (type == OG_FILMS) continue;   // (on a shore alone: `overgrowth plates`)
            StarSystem sys = land.sys;
            SurfaceView sv;
            sv.site.made = growthLayer(sys.bodies[land.body].radiusKm, StarSystem::bodyFromLatLon(land.lat, land.lon),
                                       growthOfType(type, OGT_FOLDED, RGB(0.6f, 0.3f, 0.3f), 0.5, 0.3, type == OG_CORAL || type == OG_SPONGE ? 3 : 0, false, type == OG_HAIR));
            sv.init(&sys, land.body, land.lat, land.lon, land.day);
            std::vector<SurfaceView::Collider> cols;
            for (int cz = -8; cz < 8; cz++)
                for (int cx = -8; cx < 8; cx++) {
                    std::vector<GrowthInst> here;
                    sv.forGrowths(cx, cz, [&](const GrowthInst& S) { here.push_back(S); });
                    for (const GrowthInst& S : here) {
                        structures++; shapes |= 1 << S.shape;
                        if (!S.collides) { walkedThrough += S.h >= SurfaceView::STEP_M; continue; }
                        colliding++;
                        GrowthFoot feet[GROWTH_FEET];
                        const int nf = growthFeet(S, feet, GROWTH_FEET);
                        for (int f = 0; f < nf; f++) {   // each foot in a collider the walk reads, its span over the ground there
                            sv.collectColliders(feet[f].x, feet[f].z, cols);
                            bool met = false;
                            for (const SurfaceView::Collider& q : cols)
                                met = met || ((feet[f].x - q.x) * (feet[f].x - q.x) + (feet[f].z - q.z) * (feet[f].z - q.z) <= q.r * q.r + 1e-9 && q.y0 <= S.gy + 0.3 && q.y1 >= S.gy + 0.4);
                            if (!met) { unmatched++; if (firstBad.empty()) firstBad = fmt("; a %s's shape %d at %.1f %.1f with no collider at its foot %d", MADE_GROWTH_NAMES[type], S.shape, S.x, S.z, f); }
                        }
                    }
                    sv.forTrees(cx, cz, [&](const TreeInst& T) {
                        trees++; wrapped += T.wrapped;
                        for (const GrowthInst& S : here)
                            if (growthHolds(S, T.x, T.z, 0.3)) { inside++; if (firstBad.empty()) firstBad = fmt("; a tree in a %s's shape %d", MADE_GROWTH_NAMES[type], S.shape); }
                    });
                }
            // walked into: the two nearest round ones standing over a step, the explorer 3 m off facing it, three seconds forward; none walked through
            std::vector<GrowthInst> round;
            for (int cz = -3; cz < 3; cz++)
                for (int cx = -3; cx < 3; cx++)
                    sv.forGrowths(cx, cz, [&](const GrowthInst& S) {
                        if (S.collides && S.len == 0 && S.shape != GS_CAP && S.r >= 0.4) round.push_back(S);
                    });
            std::sort(round.begin(), round.end(), [](const GrowthInst& a, const GrowthInst& b) { return a.x * a.x + a.z * a.z < b.x * b.x + b.z * b.z; });
            for (size_t k = 0; k < round.size() && k < 2; k++) {
                const GrowthInst& S = round[k];
                GrowthFoot foot[GROWTH_FEET];
                growthFeet(S, foot, GROWTH_FEET);
                const double a = hash01(mix64(S.hs + 5)) * TAU, d = foot[0].r + 3;
                sv.player.x = S.x - std::sin(a) * d; sv.player.z = S.z - std::cos(a) * d; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                sv.player.yaw = a; sv.player.pitch = 0;
                Input in;
                double closest = 1e9;
                for (int f = 0; f < 90; f++) {
                    in.down[KEY_W] = true;
                    sv.update(1.0 / 30, in, land.day + f / 30.0, true);   // (J-10: with the controls on: off, the explorer stood still)
                    in.newFrame();
                    sv.player.yaw = std::atan2(S.x - sv.player.x, S.z - sv.player.z);   // (held toward it)
                    closest = std::min(closest, std::hypot(sv.player.x - S.x, sv.player.z - S.z));
                }
                walked++;
                const bool through = closest < 0.9 * foot[0].r;
                passed += through;
                if (through && firstBad.empty()) firstBad = fmt("; walked into a %s's shape %d to %.2f m of its middle (its disc %.2f m)", MADE_GROWTH_NAMES[type], S.shape, closest, foot[0].r);
            }
        }
        int nShapes = 0;
        for (int k = 0; k < GS_COUNT; k++) nShapes += (shapes >> k) & 1;
        check("seeding: the overgrowth's structures and trees", structures > 100 && colliding > 20 && unmatched == 0 && walked >= 12 && passed == 0 && trees > 20 && wrapped == trees &&
                                                                    inside == 0 && nShapes >= 14,
              fmt("(nine types on a %s's ground, 256 cells each: %d structures of %d shapes, %d walked round (a collider at every foot but %d), %d walked through over a step; %d "
                  "walked into, %d passed; %d trees, %d wrapped, %d in a structure%s)", KINDS[land.sys.bodies[land.body].type].name, structures, nShapes, colliding, unmatched,
                  walkedThrough, walked, passed, trees, wrapped, inside, firstBad.c_str()));
    }
}

// J-10: the overgrowth on the ruins: its climb by place and step, a town's colliders the same under it, its pieces a pure function of the cell,
// its membranes pushed through; a stone rotter's ruins lower; its sounds' words
void unitSeedingRuins(const Check& check) {
    const MadeGrowth flesh = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, false, false);
    {   // the climb: the steps since the edge passed, never down as the steps go on, none where the cover is not, the same on the pool
        const double R = 6000;
        const Vec3 place = StarSystem::bodyFromLatLon(0.3, 1.1);
        const uint64_t seed = 0x7E57C11BULL;
        const int N = 500;
        std::vector<Vec3> pts(N);
        for (int i = 0; i < N; i++) {   // within 2,000 km of the place, nearer ones oftener
            const double d = 2000.0 / R * std::pow((i + 0.5) / N, 2), a = std::fmod(i * 2.399963229728653, TAU);
            pts[i] = offsetUnit(place, std::sin(a) * d * R * 1000, std::cos(a) * d * R * 1000, R * 1000);
        }
        std::vector<double> last(N, -1);
        int down = 0, mismatch = 0, differ = 0, covered = 0, steps = 0;
        double top = 0;
        for (int step = 8; step <= 76; step += 4, steps++) {
            const MadeWorld m = growthAtStep(R, place, flesh, step);
            std::vector<double> a(N), b(N);
            for (int i = 0; i < N; i++) a[i] = madeGrowthSince(m, pts[i], seed);
            parallelFor(N, 8, [&](int i0, int i1) { for (int i = i0; i < i1; i++) b[i] = madeGrowthSince(m, pts[i], seed); });
            for (int i = 0; i < N; i++) {
                int line;
                differ += a[i] != b[i];
                mismatch += (a[i] >= 0) != (madeGrowthAt(m, pts[i], seed, line) >= 0.5);
                down += a[i] < last[i] - 1e-9;
                last[i] = a[i]; top = std::max(top, a[i]);
            }
        }
        for (double v : last) covered += v >= 0;
        check("seeding: the ruins' cover by place and step", down == 0 && mismatch == 0 && differ == 0 && covered == N && top >= MADE_GROWTH_STEPS - 1,
              fmt("(%d points within 2,000 km at %d steps from 8 to 76: %d covered at the last, the longest lain %.1f steps; %d gone down, %d against the cover, %d "
                  "differing on the pool)", N, steps, covered, top, down, mismatch, differ));
    }
    {   // a people's town (the scene's, laid out as if its people ended 300 years ago, so its walls stand) under a flesh let go at its middle: its colliders as
        // they were, its pieces the same grown twice, its membranes pushed through; then the same town with its stone eaten
        StarSystem sys; SurfaceView sv; StarNeighborhood nb; double t = 0;
        if (!setupSceneForType(-1, 25, 35 * DEG, 0.6, -0.05, sv, sys, nb, t, SCENE_TOWN)) { check("seeding: the ruins under an overgrowth", false, "(no town)"); return; }
        sv.flocks.clear(); sv.critters.clear();
        const SurfaceView::RuinCell* near[SurfaceView::SETTLEMENTS_NEAR];
        const int n = sv.settlementsNear(sv.player.x, sv.player.z, near);
        const SurfaceView::RuinCell* old = nullptr;
        for (int i = 0; i < n; i++) if (!old || near[i]->spec.buildings.size() > old->spec.buildings.size()) old = near[i];
        if (!old) { check("seeding: the ruins under an overgrowth", false, "(no settlement near the town scene)"); return; }
        Culture young = sv.cultures[old->spec.people == 1 && sv.peoples > 1 ? 1 : 0];
        young.ageYears = young.weatherYears = 300; young.decay = decayOf(300, young.wet);
        RuinSpec spec = old->spec; spec.people = 0;
        std::vector<Form> P[3]; std::vector<FormOpening> openings;
        for (int l = 0; l < 3; l++) ruinElements(spec, young, l, P[l], l == 0 ? &openings : nullptr);
        double tx = 0, tz = 0; bool placed = false;   // beside it, 400 m past its edge on dry, gentle ground
        for (int k = 0; k < 8 && !placed; k++) {
            const double a = k * TAU / 8, d = spec.size + 400;
            tx = old->r.x + std::sin(a) * d; tz = old->r.z + std::cos(a) * d;
            const SurfaceSample q = sampleSurface(sv.site.gen, sv.site.unitAt(tx, tz), 16);
            placed = q.material != MAT_WATER && !(q.water > -1e8 && q.height < q.water + 2) && ruinSiteOk(sv.site.gen, [&] { RuinSpec r = spec; StarSystem::latLonFromBody(sv.site.unitAt(tx, tz), r.lat, r.lon); return r; }());
        }
        if (!placed) { check("seeding: the ruins under an overgrowth", false, "(no dry ground beside the town)"); return; }
        sv.testPlaceSettlement(tx, tz, spec, P, &openings);
        const SurfaceView::RuinCell& town = sv.placedCells.back();
        sv.player.x = tx; sv.player.z = tz - spec.size - 30; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
        { Input in; for (int k = 0; k < 10; k++) sv.update(1.0 / 30, in, t, false); }   // (the ground there loaded)
        auto collidersOf = [&]() {
            std::vector<SurfaceView::Collider> out;
            sv.settlementColliders(town.r, town, town.r.x, town.r.z, out);
            std::vector<std::array<double, 5>> v;
            for (const SurfaceView::Collider& q : out) v.push_back({q.x, q.z, q.r, q.y0, q.y1});
            std::sort(v.begin(), v.end());
            return v;
        };
        const auto plain = collidersOf();
        const int plainPieces = (int)town.elems[0].size();
        const double R = sys.bodies[sv.site.body].radiusKm;
        growthLayerOn(sv, growthAtStep(R, sv.site.unitAt(town.r.x, town.r.z), flesh, 11));
        const auto grown = collidersOf();
        const std::vector<Form> once = town.elems[0];
        sv.testRegrowRuins();
        const std::vector<Form>& twice = town.elems[0];
        bool same = once.size() == twice.size();
        for (size_t i = 0; same && i < once.size(); i++)
            same = once[i].kind == twice[i].kind && once[i].growth == twice[i].growth && once[i].x == twice[i].x && once[i].z == twice[i].z && once[i].y0 == twice[i].y0 &&
                   once[i].y1 == twice[i].y1 && once[i].hx == twice[i].hx && once[i].seed == twice[i].seed;
        int membranes = 0, nodules = 0, drapes = 0, unparted = 0;
        for (const Form& f : twice) {
            membranes += f.growth == FG_MEMBRANE; nodules += f.growth == FG_NODULE; drapes += f.growth == FG_DRAPE;
            unparted += f.growth != FG_NONE && f.part != FORM_PART_GROWTH;
        }
        double lo = 1e9, hi = -1;
        for (size_t b = 0; b + 1 < town.climb.size(); b++) if (town.climb[b] >= 0) { lo = std::min(lo, (double)town.climb[b]); hi = std::max(hi, (double)town.climb[b]); }
        // pushed through: each of up to four doorways' membranes from 2.5 m outside, three seconds forward with the controls; crossed, and torn
        int tried = 0, crossed = 0, torn = 0;
        std::string firstBad;
        for (const Form& f : twice) {
            if (tried >= 4) break;
            if (f.growth != FG_MEMBRANE || f.building < 0 || f.y1 - f.y0 < 1.6 || f.hx < 0.8) continue;
            const Building& B = town.spec.buildings[f.building];
            double nx = std::sin(f.heading), nz = std::cos(f.heading);   // out of the building
            if (nx * (f.x - B.x) + nz * (f.z - B.z) < 0) { nx = -nx; nz = -nz; }
            const double mx = town.r.x + f.x, mz = town.r.z + f.z;
            if (sv.site.waterAt(mx, mz) > -1e8 && sv.site.groundHeight(mx, mz) < sv.site.waterAt(mx, mz) + 0.3) continue;
            tried++;
            sv.player.x = mx + nx * 2.5; sv.player.z = mz + nz * 2.5; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
            sv.player.vx = sv.player.vz = 0; sv.player.yaw = std::atan2(-nx, -nz); sv.player.pitch = 0; sv.tearCue = 0;
            Input in;
            bool tore = false;
            for (int k = 0; k < 90; k++) {
                in.down[KEY_W] = true;
                sv.update(1.0 / 30, in, t + k / 30.0, true);
                in.newFrame();
                sv.player.yaw = std::atan2(-nx, -nz);
                tore = tore || sv.tearCue > 0;
                sv.tearCue = 0;
            }
            const double inside = (sv.player.x - mx) * nx + (sv.player.z - mz) * nz;
            crossed += inside < -0.3; torn += tore;
            if (inside >= -0.3 && firstBad.empty()) firstBad = fmt("; stopped %.2f m outside a membrane %.1f m wide", inside, 2 * f.hx);
        }
        check("seeding: the ruins under an overgrowth", plain == grown && same && plainPieces < (int)twice.size() && membranes > 0 && nodules > 0 && drapes > 0 && unparted == 0 &&
                                                          tried >= 2 && crossed == tried && torn == tried,
              fmt("(the scene's %s of %d buildings as if left 300 years ago, a flesh let go at its middle eleven steps on: its walls taken %.1f to %.1f m up; %d colliders, the "
                  "same as without it %s; %d pieces grown on its %d (%d membranes, %d nodules, %d drapes), the same grown twice %s; %d membranes walked into, %d pushed "
                  "through, %d torn%s)", SETTLEMENT_CLASS_NAMES[spec.sclass], (int)spec.buildings.size(), lo, hi, (int)grown.size(), plain == grown ? "yes" : "NO",
                  (int)twice.size() - plainPieces, plainPieces, membranes, nodules, drapes, same ? "yes" : "NO", tried, crossed, torn, firstBad.c_str()));
        // a stone rotter's: the same young town with its stone eaten, its walls lower
        Culture rotted = young;
        rotted.stoneRot = 1;
        std::vector<Form> a, b;
        ruinElements(spec, young, 0, a); ruinElements(spec, rotted, 0, b);
        auto standing = [](const std::vector<Form>& v) { double h = 0; for (const Form& f : v) if (f.kind == FK_BOX && f.part == 0) h += std::max(0.0, f.y1 - f.y0); return h; };
        const double ha = standing(a), hb = standing(b);
        check("seeding: a stone rotter's ruins", hb < 0.8 * ha,
              fmt("(the town's walls standing %.0f m in all as left 300 years ago, %.0f m with their stone eaten; %d pieces, %d)", ha, hb, (int)a.size(), (int)b.size()));
    }
    {   // the silence of a dead world: a herd's ground heard for twenty seconds (birds, hoofs, calls, the insects by the water), then the same gone silent
        StarSystem sys; SurfaceView sv; StarNeighborhood nb; double t = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv, sys, nb, t, SCENE_HERD)) {
            auto listen = [&](bool dead, double t0) {
                sv.silence = dead;
                int heard = 0;
                AudioState a; Input in;
                for (int k = 0; k < 600; k++) {
                    sv.update(1.0 / 30, in, t0 + k / 30.0, false);
                    lifeSoundsTo(sv, a, true);
                    heard += a.birds > 0 || a.hoofs > 0.01 || a.insects > 0 || a.call > 0;
                    a.call = 0;
                }
                return heard;
            };
            const int alive = listen(false, t), dead = listen(true, t + 20);
            check("seeding: the silence of a dead world", alive > 0 && dead == 0,
                  fmt("(a herd's ground on a %s: the living heard in %d of 600 frames; gone silent, in %d)", KINDS[sys.bodies[sv.site.body].type].name, alive, dead));
        } else check("seeding: the silence of a dead world", false, "(no herd's ground)");
    }
    {   // the sounds' words: a wet flesh's WET GROUND and BREATHING, a dry lichen's neither
        const MadeWorld wet = growthAtStep(6000, StarSystem::bodyFromLatLon(0, 0), flesh, 20);
        const MadeWorld dry = growthAtStep(6000, StarSystem::bodyFromLatLon(0, 0), growthOfType(OG_LICHEN, OGT_FIBROUS, RGB(0.5f, 0.6f, 0.3f), 0.5, 0.0, 1, false, false), 20);
        std::string ww, wd;
        for (const std::string& x : madeTraitWords(wet, true)) ww += (ww.empty() ? "" : ", ") + x;
        for (const std::string& x : madeTraitWords(dry, true)) wd += (wd.empty() ? "" : ", ") + x;
        check("seeding: an overgrowth's sounds named", ww.find("WET GROUND") != std::string::npos && ww.find("BREATHING") != std::string::npos &&
                                                         wd.find("WET GROUND") == std::string::npos && wd.find("BREATHING") == std::string::npos,
              fmt("(a wet flesh's MADE %s; a dry lichen's %s)", ww.c_str(), wd.c_str()));
    }
}
