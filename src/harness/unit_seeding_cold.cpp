// The unit's `seeding` group, J-13 (R-454, a culture for any world): the cold worlds. A line that lives in brine and makes no ice of its own goes
// down to BRINE_COLDEST_C on the editor (its edits in any order), other water life to WATER_COLDEST_C, and a brine flask with it; a place is read
// at its own warmth, its water brine colder than any other stays liquid; the depths down a drill hole (the rock's warmth by the world's inner
// heat, a sea under the ice, a furnace closed at any depth) and a hydrogen eater tuned for one taking there; a deep line's spread (nothing on the
// surface but the cracks over a sea, its gases at part of its share, no overgrowth, its vials, its `seeded` line's depth); the seed page's depths,
// a trip down a drill hole and its save
#include "harness/harness.h"
#include "life/seeding.h"
#include "test_game.h"
#include <algorithm>
#include <cmath>

namespace {
const MicrobeGene* geneKindOf(const MicrobeGenes& g, int kind) {
    for (const MicrobeGene& x : g.genes) if (x.kind == kind && !x.cut) return &x;
    return nullptr;
}
const SeedCase* caseOfType(const std::vector<SeedCase>& cases, int type) {
    for (const SeedCase& c : cases) if (c.sys.bodies[c.body].type == type) return &c;
    return nullptr;
}
Seeding deepSeeding(const StarSystem& sys, int body, const SeedLine& line, double lat, double lon, double km) {
    Seeding s;
    s.lat = lat; s.lon = lon; s.depthKm = km; s.line = line.key; s.gen = GEN_VERSION; s.sent = line;
    seedingRun(sys, body, s);
    return s;
}
}   // namespace

void unitSeedingCold(const Check& check) {
    StarSystem ls; int lb = -1, sp = -1;
    SeedLine wild;
    if (!seedSource(ls, lb, sp, wild)) { check("seeding: a brine line goes colder (J-13)", false, "(no felisian sea light-feeder)"); return; }
    const MicrobeGenes& G = wild.genes;
    const GeneCode c = geneCodeOf(G.genesis);
    {   // its warmth's letters at their lightest: water life stops at -20 C, a brine line with no frost of its own goes on to -45 C, in either order;
        // a line that makes ice stays at -20 C in brine; the wild line as it was; a brine flask's water to -45 C, a sea's to -20 C
        const MicrobeGene* W = geneKindOf(G, GN_WARMTH);
        const MicrobeGene* S = geneKindOf(G, GN_SALT);
        const MicrobeGene* F = geneKindOf(G, GN_FROST);
        if (!W || !S || !F) { check("seeding: a brine line goes colder (J-13)", false, "(the line lacks a warmth, a salt or a frost gene)"); return; }
        GeneEdit cold; cold.place = W->place; cold.part = lighter(c, W->part, 99);
        GeneEdit brine; brine.place = S->place; brine.part = heavier(c, S->part, 99);
        GeneEdit ice; ice.place = F->place; ice.part = heavier(c, F->part, 99);
        const SeedLine a = seedLineFor(ls, lb, sp, {cold}), b = seedLineFor(ls, lb, sp, {cold, brine}), r = seedLineFor(ls, lb, sp, {brine, cold});
        const SeedLine f = seedLineFor(ls, lb, sp, {cold, brine, ice}), w = seedLineFor(ls, lb, sp, {});
        double lo = 0, hi = 0, loB = 0, hiB = 0;
        cellRange(CC_WATER, lo, hi); cellRange(CC_WATER, loB, hiB, 2);
        const bool same = b.kind.s.tMin == r.kind.s.tMin && b.kind.s.tOpt == r.kind.s.tOpt && b.kind.s.tMax == r.kind.s.tMax;
        // the editor's guide says so (the warmth's, the salt's, the frost's; nothing on another gene)
        const std::string ga = coldGuideWords(a.kind, GN_WARMTH), gb = coldGuideWords(b.kind, GN_SALT), gf = coldGuideWords(f.kind, GN_FROST);
        const bool guide = ga.find("COLDEST -20 C") != std::string::npos && gb.find("CAN GO DOWN TO -45 C") != std::string::npos &&
                           gf.find("CUT IT FOR -45 C") != std::string::npos && coldGuideWords(b.kind, GN_SIZE).empty();
        check("seeding: a brine line with no frost of its own goes down to -45 C, other water life to -20 C (J-13)",
              a.kind.s.tMin >= WATER_COLDEST_C && !brineAntifreeze(a.kind) && brineAntifreeze(b.kind) && b.kind.s.tMin < WATER_COLDEST_C &&
                      b.kind.s.tMin >= BRINE_COLDEST_C && same && f.kind.world.frostC > FROST_NONE + 1 && f.kind.s.tMin >= WATER_COLDEST_C &&
                      w.kind.s.tMin == wild.kind.s.tMin && lo == WATER_COLDEST_C && loB == BRINE_COLDEST_C && guide,
              fmt("(lightest warmth: %.0f to %.0f C; with brine %.0f to %.0f C, the other order %.0f; with its frost made too %.0f C; wild %.0f C; the cell %.0f C, brine's %.0f C; "
                  "the guide: '%s', '%s', '%s')", a.kind.s.tMin, a.kind.s.tMax, b.kind.s.tMin, b.kind.s.tMax, r.kind.s.tMin, f.kind.s.tMin, w.kind.s.tMin, lo, loB, ga.c_str(),
                  gb.c_str(), gf.c_str()));
    }
    const std::vector<SeedCase> cases = seedCases();
    const SeedCase* crater = caseOfType(cases, PT_CRATERED);
    const SeedCase* thin = caseOfType(cases, PT_THINATMO);
    const SeedCase* europan = caseOfType(cases, PT_EUROPAN);
    const SeedCase* molten = caseOfType(cases, PT_MOLTEN);
    if (!crater || !thin || !europan || !molten) { check("seeding: down a drill hole (J-13)", false, "(no cratered, thin-aired, europan or molten world)"); return; }
    {   // a place read at its own warmth (a flask's cell's range never held a world's): the cratered world's ground far under -20 C, its water brine,
        // and a brine line at -45 C too cold there
        const SeedVerdict P = seedPlaceFacts(crater->sys, crater->body, crater->lat, crater->lon);
        const MicrobeGene* W = geneKindOf(G, GN_WARMTH);
        const MicrobeGene* S = geneKindOf(G, GN_SALT);
        GeneEdit cold; cold.place = W->place; cold.part = lighter(c, W->part, 99);
        GeneEdit brine; brine.place = S->place; brine.part = heavier(c, S->part, 99);
        const SeedLine b = seedLineFor(ls, lb, sp, {cold, brine});
        const SeedVerdict V = seedVerdictOf(crater->sys, crater->body, b, crater->lat, crater->lon);
        check("seeding: a place at its own warmth, its water brine under -20 C (J-13)",
              P.medium.tempC == std::round(P.place.hab.meanC) && P.medium.tempC < WATER_COLDEST_C && P.medium.salt == 2 && V.fate == SF_COLD,
              fmt("(the cratered world's %s at %.0f C, its medium %.0f C %s; a brine line from %.0f C: %s)", P.place.name.c_str(), P.place.hab.meanC, P.medium.tempC,
                  CULTURE_SALT_NAMES[clampi(P.medium.salt, 0, 2)], b.kind.s.tMin, seedVerdictWords(V).c_str()));
    }
    // the deep's line: the first hydrogen eater of water on the forty worlds that lives without oxygen
    const StarSystem* hs = nullptr; int hb = -1, hsp = -1;
    for (const SeedCase& k : cases) {
        if (hsp >= 0) break;
        const auto life = lifeOf(k.sys);
        const Biota& B = life->bodies[k.body];
        if (B.chemistry != CH_WATER) continue;
        for (const Microbe& m : B.species)
            if (m.state == SS_LIVING && m.way == LW_CHEMICAL && m.substrate == SU_HYDROGEN && m.oxygen < 2) { hs = &k.sys; hb = k.body; hsp = m.index; break; }
    }
    if (hsp < 0) { check("seeding: down a drill hole (J-13)", false, "(no hydrogen eater on the forty worlds)"); return; }
    const SeedLine hwild = seedLineFor(*hs, hb, hsp, {});
    {   // the depths: a thin-aired world's rock its eight levels, warmer down and never colder than its ground, dark, hydrogen, no oxygen, sea-salt
        // or brine; a europan world's sea under its ice alone, a few degrees over freezing; a furnace closed at any depth; a hydrogen eater tuned for
        // five kilometres down takes there and not on the ground
        const std::vector<double> td = seedDepthsOf(thin->sys, thin->body), ed = seedDepthsOf(europan->sys, europan->body);
        const SeedVerdict top = seedPlaceFacts(thin->sys, thin->body, thin->lat, thin->lon);
        bool warmer = td.size() == 1 + SEED_DEPTHS, deepOk = true;
        double was = top.place.hab.meanC;
        for (size_t k = 1; k < td.size(); k++) {
            const SeedVerdict D = seedPlaceFacts(thin->sys, thin->body, thin->lat, thin->lon, td[k]);
            warmer = warmer && D.place.hab.meanC >= was && D.place.depthKm == td[k];
            was = D.place.hab.meanC;
            deepOk = deepOk && D.place.where == AT_DEEP && D.place.hab.niche == NI_ROCK && D.medium.cell == CC_WATER && D.medium.light == CL_DARK &&
                     D.medium.energy == CE_HYDROGEN && D.oxygen == 0 && !D.place.dry && D.medium.salt == (D.place.hab.meanC < WATER_COLDEST_C ? 2 : 1);
        }
        const double shell = iceShellKm(europan->sys.bodies[europan->body]);
        const SeedVerdict U = seedPlaceFacts(europan->sys, europan->body, europan->lat, europan->lon, shell);
        const bool sea = ed.size() == 2 && ed[1] == shell && shell > 0 && U.place.where == AT_UNDER_ICE && U.place.hab.niche == NI_PLUME && U.medium.salt == 1 &&
                         U.place.hab.meanC >= 1 && U.place.hab.meanC <= 4 && U.medium.light == CL_DARK;
        const SeedVerdict M = seedPlaceFacts(molten->sys, molten->body, molten->lat, molten->lon, 5);
        const SeedVerdict D5 = seedPlaceFacts(thin->sys, thin->body, thin->lat, thin->lon, 5);
        const SeedLine tuned = seedLineFor(*hs, hb, hsp, seedTunedEdits(*hs, hb, hsp, D5));
        const SeedVerdict deep = seedVerdictOf(thin->sys, thin->body, tuned, thin->lat, thin->lon, 5), ground = seedVerdictOf(thin->sys, thin->body, tuned, thin->lat, thin->lon);
        check("seeding: down a drill hole, its warmth the world's inside's (J-13)", warmer && deepOk && sea && M.fate == SF_FURNACE && deep.takes() && !ground.takes(),
              fmt("(the thin-aired world's %s at %.0f C, %zu depths%s, %s; the europan world's %s at %.0f C%s; the molten world 5 km down %s; a tuned hydrogen eater 5 km down: "
                  "%s, on the ground: %s)", top.place.name.c_str(), top.place.hab.meanC, td.size() - 1, warmer ? " warmer down" : " NOT WARMER DOWN",
                  deepOk ? "dark rock with hydrogen" : "NOT THE DEEP'S", U.place.name.c_str(), U.place.hab.meanC, sea ? "" : " NOT ITS SEA",
                  seedVerdictWords(M).c_str(), seedVerdictWords(deep).c_str(), seedVerdictWords(ground).c_str()));
    }
    {   // a deep line's spread: a red hydrogen eater in the rock colours nothing of the surface and its gases reach the air at part of its share; under
        // the europan world's ice it reddens the cracks (RED CRACKS) and is found in a plume's vial, never in its sea's or the rock's; a deep line that
        // never stops and binds grows over nothing; a `seeded` line keeps its depth, a line on the ground reads as before
        std::vector<GeneEdit> dye = seedLineEdits(hwild.genes, SL_DYE);
        const SeedVerdict D5 = seedPlaceFacts(thin->sys, thin->body, thin->lat, thin->lon, 5);
        std::vector<GeneEdit> e = seedTunedEdits(*hs, hb, hsp, D5);
        for (const GeneEdit& x : dye) e.push_back(x);
        SeedLine red = seedLineFor(*hs, hb, hsp, e); red.key = hwild.key + "/S1";
        const Seeding rock = deepSeeding(thin->sys, thin->body, red, thin->lat, thin->lon, 5);
        const double end = (MADE_STEPS * MADE_STEP_S + 1) / std::max(0.05, rock.verdict.pace);
        const MadeWorld mr = madeWorldOf(thin->sys, thin->body, {rock}, end);
        const bool rockNone = rock.verdict.takes() && red.kind.world.dyes() && mr.lines == 1 && mr.spread[0].deep == MADE_DEEP_ROCK && !mr.tinted && !mr.land.on && !mr.snow.on &&
                              mr.grounds == 0 && rock.verdict.share > 0 && rock.verdict.share <= DEEP_GAS_SHARE;
        const double shell = iceShellKm(europan->sys.bodies[europan->body]);
        std::vector<GeneEdit> eu = seedTunedEdits(*hs, hb, hsp, seedPlaceFacts(europan->sys, europan->body, europan->lat, europan->lon, shell));
        for (const GeneEdit& x : dye) eu.push_back(x);
        SeedLine redU = seedLineFor(*hs, hb, hsp, eu); redU.key = hwild.key + "/S2";
        const Seeding under = deepSeeding(europan->sys, europan->body, redU, europan->lat, europan->lon, shell);
        const MadeWorld mu = madeWorldOf(europan->sys, europan->body, {under}, (MADE_STEPS * MADE_STEP_S + 1) / std::max(0.05, under.verdict.pace));
        std::string words;
        for (const std::string& t : madeTraitWords(mu, false)) words += (words.empty() ? "" : ", ") + t;
        Microhabitat plume; plume.niche = NI_PLUME; plume.salt = 1; plume.meanC = 2;
        Microhabitat liquid = plume; liquid.niche = NI_LIQUID;
        Microhabitat rockHab = plume; rockHab.niche = NI_ROCK;
        const bool cracks = under.verdict.takes() && (mu.grounds & MGT_COLOURED_CRACKS) && words.find("RED CRACKS") != std::string::npos && mu.on() && !mu.tinted;
        const bool vials = seedingLivesIn(under, plume) && !seedingLivesIn(under, liquid) && !seedingLivesIn(rock, rockHab) && !seedingLivesIn(rock, plume);
        // never stops and binds, five kilometres down: no overgrowth
        std::vector<GeneEdit> og = e;
        for (const GeneEdit& x : seedLineEdits(hwild.genes, SL_STOP)) og.push_back(x);
        if (const MicrobeGene* B = geneKindOf(hwild.genes, GN_BIND)) { GeneEdit x; x.place = B->place; x.part = heavier(geneCodeOf(hwild.genes.genesis), B->part, 99); og.push_back(x); }
        SeedLine grows = seedLineFor(*hs, hb, hsp, og); grows.key = hwild.key + "/S3";
        const Seeding deepGrows = deepSeeding(thin->sys, thin->body, grows, thin->lat, thin->lon, 5);
        const MadeWorld mg = madeWorldOf(thin->sys, thin->body, {deepGrows}, 64 * MADE_STEP_S / std::max(0.05, deepGrows.verdict.pace));
        const bool noGrowth = grows.kind.world.overgrows && grows.kind.world.bind > 0 && !mg.grown();
        SeedRecord sr; sr.body = "B"; sr.line = "L"; sr.lat = 0.5; sr.lon = 1.5; sr.depthKm = 9; sr.clock = 12;
        SeedRecord back, flat;
        const std::string line = seededLine(sr);
        SeedRecord ground = sr; ground.depthKm = 0;
        const bool kept = parseSeeded(line.substr(7), back) && back.depthKm == 9 && parseSeeded(seededLine(ground).substr(7), flat) && flat.depthKm == 0 &&
                          seededLine(ground).find(" 12") == seededLine(ground).size() - 3;
        check("seeding: a deep line's spread (J-13)", rockNone && cracks && vials && noGrowth && kept,
              fmt("(a red line 5 km down: %s, %s, its gases' share %.2f; under the europan ice: %s, MADE %s; vials %s; never stopping 5 km down: %s; the `seeded` line '%s' %s)",
                  seedVerdictWords(rock.verdict).c_str(), rockNone ? "nothing on the surface" : "SEEN ON THE SURFACE", rock.verdict.share, seedVerdictWords(under.verdict).c_str(),
                  words.empty() ? "NOTHING" : words.c_str(), vials ? "a plume's alone" : "WRONG", noGrowth ? "no overgrowth" : "OVERGROWN OR NOT A GROWER", line.c_str(),
                  kept ? "read back" : "NOT READ BACK"));
    }
    {   // the seed page: Page Down takes it down the thin-aired world's levels (its THERE the deep's), refused past the last and above the ground;
        // Enter sends the drone with its depth, kept in a save, let go as a `seeded` line with it; the data sheet says how deep
        TestGame g;
        g.savePrefix = "shots/tests/unit_seeding_cold_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/unit_seeding_cold_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        const SeedVerdict D5 = seedPlaceFacts(thin->sys, thin->body, thin->lat, thin->lon, 5);
        const std::vector<GeneEdit> e = seedTunedEdits(*hs, hb, hsp, D5);
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(e);
        g.guide.strains[hwild.key + "/S1"] = rec;
        FlaskSpecies w; w.sx = hs->star.sx; w.sy = hs->star.sy; w.sz = hs->star.sz; w.body = hb; w.species = hsp; w.strain = 1;
        const bool full = labFlaskFull(g, 0, w, seedBestMedium(seedLineFor(*hs, hb, hsp, e).kind));
        g.testParkAt(thin->sys.star, thin->body, false); g.setState(GameState::SPACE);
        Input in;
        auto press = [&](TestGame& G, int k) { in.pressed[k] = in.down[k] = true; G.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
        g.openScreen(GameState::DRONES);
        press(g, KEY_S);
        g.testSeedPage(thin->lat, thin->lon);
        const std::string ground = g.testSeedWhere();
        press(g, KEY_PAGE_UP);
        const std::string above = g.testStatus();
        for (int k = 0; k < 4; k++) press(g, KEY_PAGE_DOWN);   // (1, 2, 3, 5 km)
        const std::string down5 = g.testSeedWhere();
        for (int k = 0; k < 4; k++) press(g, KEY_PAGE_DOWN);   // (8, 12, 20, 30 km)
        press(g, KEY_PAGE_DOWN);
        const std::string past = g.testStatus();
        for (int k = 0; k < 4; k++) press(g, KEY_PAGE_UP);
        const std::string back5 = g.testSeedWhere();
        press(g, KEY_ENTER);
        const bool out = g.testBay().out && g.testBay().seedTrip && g.testBay().seedDepth == 5;
        g.testDronesSkip(6);
        g.saveSlot(1);
        TestGame h; h.savePrefix = g.savePrefix; h.settingsPath = g.settingsPath; h.keysPath = g.keysPath; h.guidePath = g.guidePath;
        h.guide.load(h.guidePath);
        const bool loaded = h.loadSlot(1) && h.testBay().seedTrip && h.testBay().seedDepth == 5;
        h.testDronesSkip(30);
        const bool let = h.guide.seeded.size() == 1 && h.guide.seeded[0].depthKm == 5;
        h.testDronesSkip(400);
        const std::string report = h.testStatus();
        const std::vector<Seeding>& ss = h.testSeedings(thin->body);
        const bool grew = ss.size() == 1 && ss[0].depthKm == 5 && ss[0].verdict.takes() && report.find("CULTURE GROWS") != std::string::npos;
        check("seeding: the seed page's depths, a trip down a drill hole (J-13)",
              full && ground != down5 && down5.find("HYDROGEN, DARK") != std::string::npos && back5 == down5 && above.find("ON THE GROUND") != std::string::npos &&
                      past.find("AS DEEP AS THE DRILL GOES") != std::string::npos && out && loaded && let && grew,
              fmt("(flask 1 %s; THERE on the ground '%s', 5 km down '%s', back up to it '%s'; Page Up on the ground '%s', past 30 km '%s'; sent %s, a save %s, let go %s; back: '%s')",
                  full ? "full" : "NOT FULL", ground.c_str(), down5.c_str(), back5.c_str(), above.c_str(), past.c_str(), out ? "5 km down" : "NOT DOWN",
                  loaded ? "kept it" : "LOST IT", let ? "with its depth" : "WITHOUT ITS DEPTH", report.c_str()));
        remove(g.guidePath.c_str()); remove(g.slotPath(1).c_str());
    }
}
