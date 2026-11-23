// J-12 (R-448): `seed record`, a seeded world's record (docs/ideas/ITERATION-1.5.0.md J-12). A world's own sea line given a nerve poison (its
// STOP cut and its cells bound, so it overgrows too) let go on one of `seed climate`'s worlds with plants and animals, its 64th step reached:
// the explorer's sheets of the natives (those the line beat among them), the line found in the wild and read by the sequencer, four of its
// animals seen with the marks the ground would show (gone; dying and changed; named; plain). Drawn: the catalogue by world (the made kind on
// the world's head, DIED OUT, SEEDED, GONE), a native's sheet that died out, the line's, an animal's, the catalogue by kin (B-500: its animals
// under their world's genesis), the statistics' LIFE page: `record/`
#include "harness/harness.h"
#include "life/seeding.h"
#include "test_game.h"
#include "core/png.h"
#include <algorithm>

bool seedRecordCase(SeedRecordCase& out) {
    const std::vector<SeedAirCase> worlds = seedAirCases(40, 12);
    // (first a poisoner that makes a made kind as well: overgrowing, or giving off CO2; then any poisoner)
    const std::vector<std::vector<std::pair<int, int>>> tries = {{{GN_POISON, PZ_NERVE}, {GN_STOP, -2}, {GN_BIND, -1}}, {{GN_POISON, PZ_NERVE}, {GN_GAS, WG_CO2}},
                                                                 {{GN_POISON, PZ_NERVE}, {GN_STOP, -2}, {GN_BIND, -1}}, {{GN_POISON, PZ_NERVE}}};
    for (size_t t = 0; t < tries.size(); t++)
        for (const SeedAirCase& c : worlds) {
            const Body& b = c.sys.bodies[c.body];
            if (!kindHas(b.type, KT_FAUNA) || !kindHas(b.type, KT_FLORA)) continue;
            const std::vector<GeneEdit> edits = seedGeneEdits(c.wild.genes, tries[t]);
            const SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, edits);
            const Seeding s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
            if (!s.verdict.takes()) continue;
            const double end = (MADE_GROWTH_STEPS * MADE_STEP_S + 1) / std::max(0.05, s.verdict.pace);
            if (seedingDiedOut(lifeOf(c.sys)->bodies[c.body], s, end).empty()) continue;
            if (t < 2 && madeWorldOf(c.sys, c.body, {s}, end).kind == MDK_NONE) continue;
            out.c = c; out.edits = edits; out.line = line; out.end = end;
            return true;
        }
    return false;
}

void seedRecordGame(TestGame& g, const SeedRecordCase& rc, const std::string& name) {
    g.savePrefix = "shots/tests/" + name + "_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
    g.guidePath = "shots/tests/" + name + "_guide.txt";
    remove(g.guidePath.c_str());
    g.newGame(); g.settings.cabin = false; applyTestScale(g);
    const StarSystem& S = rc.c.sys;
    StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(rc.edits);
    g.guide.strains[rc.line.key + "/S1"] = rec;
    SeedRecord r; r.body = Guide::bodyKey(S.star.sx, S.star.sy, S.star.sz, rc.c.body); r.lat = rc.c.lat; r.lon = rc.c.lon; r.line = rc.line.key + "/S1"; r.edits = rec.edits;
    const Medium m = seedBestMedium(rc.line.kind);
    r.cell = m.cell; r.tempC = m.tempC; r.light = m.light; r.energy = m.energy; r.salt = m.salt; r.gen = GEN_VERSION; r.clock = g.guide.clock;
    g.guide.addSeeding(r);
    g.testParkAt(S.star, rc.c.body, false); g.setState(GameState::SPACE);
    g.testClockSkip(rc.end);
}

int seedRecord() {
    const double t0 = nowSec();
    SeedRecordCase rc;
    if (!seedRecordCase(rc)) { printf("seed record: no world where a poison line drives a native out by its 64th step\n"); return 1; }
    TestGame g; Input in;
    seedRecordGame(g, rc, "seed_record");
    for (const Body& ob : g.testSystem().bodies) if (ob.type != PT_COMPANION) g.testWaitMapOf(ob.index);   // (the layer's maps, made again on their thread)
    const StarSystem& S = rc.c.sys;
    const int bi = rc.c.body;
    const Body& b = S.bodies[bi];
    const std::string world = Guide::bodyKey(S.star.sx, S.star.sy, S.star.sz, bi);
    const std::vector<Seeding>& ss = g.testSeedings(bi);
    if (ss.empty() || !ss[0].verdict.takes()) { printf("seed record: the game's seeding did not take\n"); return 1; }
    const Seeding& s = ss[0];
    const Biota& B = lifeOf(S)->bodies[bi];
    const std::vector<DiedOut> gone = seedingDiedOut(B, s, g.guide.clock);
    const MadeWorld made = g.testMadeNow(bi);
    printf("seed record: %s at %lld %lld %lld body %d, its species %d given %s, let go at %.1f %.1f: %s, pace %.2f, its 64th step %.0f s on (%s)\n", KINDS[b.type].name,
           (long long)S.star.sx, (long long)S.star.sy, (long long)S.star.sz, bi, rc.line.kind.s.index, geneEditsText(rc.edits).c_str(), rc.c.lat / DEG, rc.c.lon / DEG,
           seedVerdictWords(s.verdict).c_str(), s.verdict.pace, rc.end, madeKindName(made).empty() ? "no made kind" : madeKindName(made).c_str());
    std::string beaten;
    for (size_t k = 0; k < s.verdict.beaten.size(); k++) {
        const int n = s.verdict.beaten[k];
        const bool out = std::any_of(gone.begin(), gone.end(), [&](const DiedOut& d) { return d.species == n; });
        beaten += fmt("%s%d (%s%s)", beaten.empty() ? "" : ", ", n, CULTURE_PAIR_NAMES[clampi(k < s.verdict.beatenHow.size() ? s.verdict.beatenHow[k] : 0, 0, PA_COUNT - 1)],
                      out ? ", DIED OUT" : ", lives on elsewhere");
    }
    printf("  natives beaten at the place: %s\n", beaten.empty() ? "none" : beaten.c_str());
    // the explorer's sheets: the line's species, each native it beat, then the world's first others to eight
    Guide& G = g.guide;
    const uint32_t cellForms = (1u << CF_CELL) | (1u << CF_DIVIDING);
    auto sheet = [&](int n) { MicrobeSheet& m = G.microbes[world + "/M" + std::to_string(n)]; m.seen |= SEEN_FORM | SEEN_MOTION; m.forms |= cellForms; };
    sheet(rc.line.kind.s.index);
    for (int n : s.verdict.beaten) sheet(n);
    for (const Microbe& m : B.species) if (G.microbes.size() < 8 && m.state == SS_LIVING) sheet(m.index);
    G.names[world + "/M" + std::to_string(rc.line.kind.s.index) + "/S1"] = "NERVE BLOOM";
    G.wildFound[world + "/L0"] = true;   // found in a vial of the world and read by the sequencer there
    // its animals seen: the first four of its bestiary
    const Bestiary A = Bestiary::make(b, BodyGen::make(b));
    for (int i = 0; i < 4 && i < (int)A.species.size(); i++) {
        AnimalRecord& r = G.animals[world + "/A" + std::to_string(i)];
        r.doing = (1u << CS_WALK) | (1u << (i == 0 ? DO_DEAD : CS_GRAZE)); r.nearest = 12 + 9 * i; r.most = i == 0 ? 1 : 3 + 2 * i;
        r.marks = i == 0 ? AM_GONE : (i == 1 ? AM_DYING | AM_CHANGED : 0);
    }
    if (A.species.size() > 2) G.names[world + "/A2"] = "MARSH STRIDER";
    G.samplesTaken = 6; G.sampled.insert(world);
    G.save(g.guidePath);
    makeDir("shots/tests/record");
    std::vector<std::vector<uint32_t>> cells;
    std::vector<std::string> names;
    auto grab = [&](const std::string& name) {
        g.testClearStatus(); g.frame(in, 1.0 / 30); in.newFrame(); g.testClearStatus(); g.frame(in, 1.0 / 30); in.newFrame();
        cells.emplace_back(g.output(), g.output() + (size_t)FBW * FBH);
        writePNG(("shots/tests/record/" + name + ".png").c_str(), g.output(), FBW, FBH);
        names.push_back(name);
    };
    g.openScreen(GameState::GUIDE); g.frame(in, 1.0 / 30); in.newFrame();
    const bool row = selectRow(g, in, "THE CATALOGUE");
    pressKey(g, in, KEY_ENTER);
    CatalogueState& C = g.testCat();
    int heads = 0, species = 0, died = 0, wildRows = 0, animalRows = 0, goneRows = 0;
    std::string head;
    for (const CatalogueRow& r : C.rows) {
        if (r.head) { heads++; if (r.key == world) head = g.testCatLine(r); continue; }
        species += r.kind == CR_SPECIES; wildRows += r.kind == CR_WILD; animalRows += r.kind == CR_ANIMAL;
        died += r.kind == CR_SPECIES && g.testCatLine(r).find("DIED OUT") != std::string::npos;
        goneRows += r.kind == CR_ANIMAL && g.testCatLine(r).find("GONE") != std::string::npos;
    }
    grab("catalogue");
    auto sheetOf = [&](const std::string& name, auto pick) {
        for (int i = 0; i < (int)C.rows.size(); i++) if (!C.rows[i].head && pick(C.rows[i])) { C.sel = i; break; }
        pressKey(g, in, KEY_ENTER); runFrames(g, in, 1.0);
        grab(name);
        pressKey(g, in, KEY_ESCAPE);
    };
    sheetOf("died_out", [&](const CatalogueRow& r) { return r.kind == CR_SPECIES && g.testCatLine(r).find("DIED OUT") != std::string::npos; });
    sheetOf("found_wild", [&](const CatalogueRow& r) { return r.kind == CR_WILD; });
    sheetOf("animal_gone", [&](const CatalogueRow& r) { return r.kind == CR_ANIMAL && g.testCatLine(r).find("GONE") != std::string::npos; });
    pressKey(g, in, KEY_TAB);   // B-500: by kin, the animals under their world's genesis
    grab("catalogue_kin");
    pressKey(g, in, KEY_TAB);
    pressKey(g, in, KEY_ESCAPE);
    g.openScreen(GameState::STATS); g.frame(in, 1.0 / 30); in.newFrame();
    pressKey(g, in, KEY_L);
    const CatalogueCounts N = C.counts;
    grab("life_page");
    pressKey(g, in, KEY_ESCAPE);
    writeSheet(cells, FBW, FBH, 3, "shots/tests/record/record.png");
    std::string kinds;
    for (const std::string& k : N.kinds) kinds += (kinds.empty() ? "" : ", ") + k;
    printf("  the catalogue (row %s): %d heads, %d species (%d DIED OUT), %d line found wild, %d animals (%d GONE); the world's head \"%s\"\n", row ? "found" : "NOT FOUND",
           heads, species, died, wildRows, animalRows, goneRows, head.c_str());
    printf("  the LIFE page: %d world%s seeded, %d of %d lines took, made %s; %d species died out, %d animal%s gone\n", N.worlds, N.worlds == 1 ? "" : "s", N.took, N.sent,
           kinds.empty() ? "nothing" : kinds.c_str(), N.diedOut, N.gone, N.gone == 1 ? "" : "s");
    printf("seed record: %zu frames in shots/tests/record/ (%s; record.png) (%.1f s)\n", names.size(), [&] { std::string s; for (const std::string& n : names) s += (s.empty() ? "" : ", ") + n; return s; }().c_str(),
           nowSec() - t0);
    remove(g.guidePath.c_str());
    const bool ok = row && died == (int)gone.size() && wildRows == 1 && goneRows == 1 && N.worlds == 1 && N.diedOut == (int)gone.size() && N.gone == 1 && names.size() == 6;
    return ok ? 0 : 1;
}
