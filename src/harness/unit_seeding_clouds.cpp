// The unit's `seeding` group, J-14 (R-454, a culture for any world): the clouds. A world with a cloud deck life lives in has its heights (a
// venusian world's eight, from 70 C to 0 C; a cool acid world's base alone; none on a world without one); up there acid droplets whatever the
// world's life, lit, fresh, at the height's warmth: a venusian cloud line grows there, water life never (WRONG LIQUID); a cloud line's spread
// (nothing on the surface, its whole share in the air, its dye and glow in the clouds, no overgrowth, a vial of the clouds, its `seeded` line's
// height); a deck world's globe with them drawn; the seed page's heights and a trip up into the clouds, kept in a save
#include "harness/harness.h"
#include "life/seeding.h"
#include "test_game.h"
#include <algorithm>
#include <cmath>

namespace {
const SeedCase* caseOfKind(const std::vector<SeedCase>& cases, int type) {
    for (const SeedCase& c : cases) if (c.sys.bodies[c.body].type == type) return &c;
    return nullptr;
}
Seeding cloudSeeding(const StarSystem& sys, int body, const SeedLine& line, double lat, double lon, double km) {
    Seeding s;
    s.lat = lat; s.lon = lon; s.depthKm = -km; s.line = line.key; s.gen = GEN_VERSION; s.sent = line;
    seedingRun(sys, body, s);
    return s;
}
}   // namespace

void unitSeedingClouds(const Check& check) {
    const std::vector<SeedCase> cases = seedCases();
    const SeedCase* venus = caseOfKind(cases, PT_VENUSIAN);
    const SeedCase* acid = caseOfKind(cases, PT_ACIDIC);
    const SeedCase* felis = caseOfKind(cases, PT_FELISIAN);
    const SeedCase* molten = caseOfKind(cases, PT_MOLTEN);
    StarSystem cs; int cb = -1, csp = -1;
    StarSystem ls; int lb = -1, lsp = -1;
    SeedLine water;
    if (!venus || !acid || !felis || !molten || !seedCloudSource(cs, cb, csp) || !seedSource(ls, lb, lsp, water)) {
        check("seeding: the clouds (J-14)", false, "(no venusian, acid, felisian or molten world, no cloud line or no water line)");
        return;
    }
    const SeedLine cwild = seedLineFor(cs, cb, csp, {});
    const std::vector<double> H = seedHeightsOf(venus->sys, venus->body);
    {   // the heights: a venusian world's eight, each higher and colder (8 C a kilometre) from about 70 C to about 0 C at the equator; a cool acid
        // world's base alone, 3 km up; none on a felisian world or a furnace
        bool rising = H.size() == (size_t)CLOUD_LEVELS;
        std::string ws;
        double first = 0, last = 0;
        for (size_t k = 0; k < H.size(); k++) {
            const SeedVerdict P = seedPlaceFacts(venus->sys, venus->body, 0, 0, -H[k]);
            if (k > 0) rising = rising && H[k] > H[k - 1];
            if (k == 0) first = P.place.hab.meanC;
            last = P.place.hab.meanC;
            ws += fmt("%s%.0f km %.0f C", k ? ", " : "", H[k], P.place.hab.meanC);
        }
        const std::vector<double> A = seedHeightsOf(acid->sys, acid->body);
        const SeedVerdict A0 = seedPlaceFacts(acid->sys, acid->body, acid->lat, acid->lon);
        const bool acidOk = !A.empty() && A[0] >= CLOUD_BASE_KM && seedPlaceFacts(acid->sys, acid->body, acid->lat, acid->lon, -A[0]).place.hab.meanC < A0.place.hab.meanC;
        check("seeding: a cloud deck's heights, from 70 C to 0 C (J-14)",
              rising && std::fabs(first - 70) <= 6 && std::fabs(last) <= 6 && acidOk && seedHeightsOf(felis->sys, felis->body).empty() &&
                      seedHeightsOf(molten->sys, molten->body).empty() && seedCloudDeck(venus->sys.bodies[venus->body]),
              fmt("(the venusian world's %s; the acid world's %zu from %.0f km; the felisian's %zu, the molten's %zu)", ws.c_str(), A.size(), A.empty() ? 0.0 : A[0],
                  seedHeightsOf(felis->sys, felis->body).size(), seedHeightsOf(molten->sys, molten->body).size()));
    }
    // the height of the venusian world's place nearest 35 C, where the cloud line is let go below
    double km = 0, best = 1e9;
    for (double h : H) {
        const double c = seedPlaceFacts(venus->sys, venus->body, venus->lat, venus->lon, -h).place.hab.meanC;
        if (std::fabs(c - 35) < best) { best = std::fabs(c - 35); km = h; }
    }
    const SeedVerdict P = seedPlaceFacts(venus->sys, venus->body, venus->lat, venus->lon, -km);
    const std::vector<GeneEdit> tuned = seedTunedEdits(cs, cb, csp, P);
    {   // up there: acid droplets whatever the world's life (its niche the clouds', lit, fresh, not dry), named by the height; the cloud line tuned for
        // it grows, and the water line never, whatever its warmth (no gene changes its liquid)
        const SeedLine t = seedLineFor(cs, cb, csp, tuned);
        const SeedVerdict vt = seedVerdictOf(venus->sys, venus->body, t, venus->lat, venus->lon, -km);
        const SeedVerdict vw = seedVerdictOf(venus->sys, venus->body, water, venus->lat, venus->lon, -km);
        const SeedVerdict vwt = seedVerdictOf(venus->sys, venus->body, seedLineFor(ls, lb, lsp, seedTunedEdits(ls, lb, lsp, P)), venus->lat, venus->lon, -km);
        const SeedVerdict ground = seedVerdictOf(venus->sys, venus->body, t, venus->lat, venus->lon);
        const bool place = P.place.where == AT_CLOUDS && P.place.hab.niche == NI_CLOUD && P.place.aloft() && P.place.depthKm == -km && P.medium.cell == CC_ACID &&
                           P.medium.light != CL_DARK && P.medium.salt == 0 && !P.place.dry && P.place.name == fmt("THE CLOUDS %.0f KM UP", km) && P.fate != SF_FURNACE;
        check("seeding: up in the clouds, acid life grows and water life never (J-14)",
              place && vt.takes() && vw.fate == SF_SOLVENT && vwt.fate == SF_SOLVENT && ground.fate == SF_HOT,
              fmt("(%s: %s; the cloud line tuned %s: %s; the water line: %s, tuned: %s; on the ground: %s)", P.place.name.c_str(), seedPlaceWords(P).c_str(),
                  geneEditsText(tuned).c_str(), seedWhyWords(vt, t.kind).c_str(), seedWhyWords(vw, water.kind).c_str(), seedVerdictWords(vwt).c_str(),
                  seedVerdictWords(ground).c_str()));
    }
    {   // its spread: in no water and on no land of the surface, its whole share in the air (the deck round the world), its dye in the clouds at
        // its 24th step and its glow (a GLOW gene moved in) with no lift; never stopping and binding: no overgrowth; a vial of the clouds within
        // its band holds it, a vial of the ground never; the `seeded` line keeps its height (under 0)
        std::vector<GeneEdit> e = tuned;
        for (const GeneEdit& x : seedLineEdits(cwild.genes, SL_DYE)) e.push_back(x);
        GeneEdit glow;
        const bool glows = seedGlowDonor(glow);
        if (glows) e.push_back(glow);
        SeedLine red = seedLineFor(cs, cb, csp, e); red.key = cwild.key + "/S2";
        const Seeding s = cloudSeeding(venus->sys, venus->body, red, venus->lat, venus->lon, km);
        const MadeWorld m = madeWorldOf(venus->sys, venus->body, {s}, (MADE_STEPS * MADE_STEP_S + 1) / std::max(0.05, s.verdict.pace));
        std::string words;
        for (const std::string& t : madeTraitWords(m, false)) words += (words.empty() ? "" : ", ") + t;
        const bool spread = s.verdict.takes() && m.lines == 1 && m.spread[0].deep == MADE_CLOUDS && m.spread[0].waters == 0 && !m.spread[0].land &&
                            !madeLives(m.spread[0], true, 0, 20) && !madeLives(m.spread[0], false, 0, 20) && s.verdict.share > 0.5 && !m.tinted;
        const bool air = (m.air & MA_DYE_CLOUDS) && m.cloudTintK > 0 && words.find("RED CLOUDS") != std::string::npos && red.kind.world.lift <= 0 &&
                         (!glows || ((m.air & MA_GLOW_CLOUDS) && m.cloudGlow > 0));
        std::vector<GeneEdit> og = e;
        for (const GeneEdit& x : seedLineEdits(cwild.genes, SL_STOP)) og.push_back(x);
        for (const MicrobeGene& B : cwild.genes.genes)
            if (B.kind == GN_BIND && !B.cut) { GeneEdit x; x.place = B.place; x.part = heavier(geneCodeOf(cwild.genes.genesis), B.part, 99); og.push_back(x); }
        SeedLine grows = seedLineFor(cs, cb, csp, og); grows.key = cwild.key + "/S3";
        const Seeding sg = cloudSeeding(venus->sys, venus->body, grows, venus->lat, venus->lon, km);
        const MadeWorld mg = madeWorldOf(venus->sys, venus->body, {sg}, 64 * MADE_STEP_S / std::max(0.05, sg.verdict.pace));
        const bool noGrowth = grows.kind.world.overgrows && grows.kind.world.bind > 0 && !mg.grown();
        Microhabitat cloud; cloud.niche = NI_CLOUD; cloud.salt = 0; cloud.meanC = std::round(0.5 * (red.kind.s.tMin + red.kind.s.tMax));
        Microhabitat cold = cloud; cold.meanC = red.kind.s.tMin - 5;
        Microhabitat rock = cloud; rock.niche = NI_ROCK;
        const bool vials = seedingLivesIn(s, cloud) && !seedingLivesIn(s, cold) && !seedingLivesIn(s, rock);
        SeedRecord sr; sr.body = "B"; sr.line = "L"; sr.lat = 0.5; sr.lon = 1.5; sr.depthKm = -47; sr.clock = 12;
        SeedRecord back;
        const bool kept = parseSeeded(seededLine(sr).substr(7), back) && back.depthKm == -47;
        check("seeding: a cloud line's spread, its clouds dyed and glowing (J-14)", spread && air && noGrowth && vials && kept,
              fmt("(a red%s line %.0f km up: %s, its share %.2f, %s; MADE %s, clouds %.2f, glow %.2f; never stopping: %s; vials %s; the `seeded` line '%s' %s)",
                  glows ? ", glowing" : "", km, seedVerdictWords(s.verdict).c_str(), s.verdict.share, spread ? "in the clouds alone" : "NOT IN THE CLOUDS ALONE",
                  words.empty() ? "NOTHING" : words.c_str(), m.cloudTintK, m.cloudGlow, noGrowth ? "no overgrowth" : "OVERGROWN OR NOT A GROWER",
                  vials ? "a cloud's within its band alone" : "WRONG", seededLine(sr).c_str(), kept ? "read back" : "NOT READ BACK"));
    }
    {   // a deck world's globe: a cloud line's dye in the deck's streaks by day, its glow on the night side (the test layer's made air), each in a bank
        // the world left free; as generated without it
        TestGame g;
        g.savePrefix = "shots/tests/unit_seeding_clouds_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/unit_seeding_clouds_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        Input in;
        auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
        auto press = [&](int k) { in.pressed[k] = in.down[k] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
        auto globe = [&](bool night, const MadeWorld* m, std::vector<uint32_t>& px) {   // (each at one moment: the deck drifts; the layer set once parked)
            g.testSetTime(20000); g.testParkAt(venus->sys.star, venus->body, true, night);
            if (m) g.testMadeSet(venus->body, *m); else g.testMadeClear();
            run(0.2); g.testWaitMapOf(venus->body); run(0.1);
            press(KEY_ESCAPE); run(0.3);
            px.assign(g.output(), g.output() + FBW * FBH);
        };
        MadeWorld m;
        m.air = MA_DYE_CLOUDS | MA_GLOW_CLOUDS; m.cloudTintK = 0.6; m.cloudTint = RGB(0.75f, 0.12f, 0.10f); m.cloudGlow = 1; m.cloudGlowTint = RGB(0.30f, 0.86f, 0.80f);
        m.cloudGlowForm = MG_DECKS;
        m.key = madeKey(m);
        std::vector<uint32_t> day0, night0, day1, night1, day2;
        globe(false, nullptr, day0); globe(false, nullptr, day0);   // (the first: its neighbours' maps made)
        globe(true, nullptr, night0);
        globe(false, &m, day1); globe(true, &m, night1);
        globe(false, nullptr, day2);
        int dayChanged = 0, nightChanged = 0, back = 0;   // (within the disc's middle, 60 px round the frame's: its neighbours' palettes aside)
        for (int y = 0; y < FBH; y++)
            for (int x = 0; x < FBW; x++) {
                if ((x - FBW / 2) * (x - FBW / 2) + (y - FBH / 2) * (y - FBH / 2) > 60 * 60) continue;
                const size_t i = (size_t)y * FBW + x;
                dayChanged += day0[i] != day1[i]; nightChanged += night0[i] != night1[i]; back += day0[i] != day2[i];
            }
        check("seeding: a deck world's cloud line drawn from orbit (J-14)", dayChanged > 300 && nightChanged > 3000 && back == 0,
              fmt("(the middle of the venusian world's globe with a red glowing deck: %d px changed by day, %d by night; as generated again: %d px apart)", dayChanged,
                  nightChanged, back));
        remove(g.guidePath.c_str());
    }
    {   // the seed page: Page Up climbs into the venusian world's clouds (its THERE acid), refused past the highest; Enter sends the drone with its
        // height, kept in a save, let go as a `seeded` line with it; it grows
        TestGame g;
        g.savePrefix = "shots/tests/unit_seeding_clouds_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/unit_seeding_clouds_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(tuned);
        g.guide.strains[cwild.key + "/S1"] = rec;
        FlaskSpecies w; w.sx = cs.star.sx; w.sy = cs.star.sy; w.sz = cs.star.sz; w.body = cb; w.species = csp; w.strain = 1;
        const bool full = labFlaskFull(g, 0, w, seedBestMedium(seedLineFor(cs, cb, csp, tuned).kind));
        g.testParkAt(venus->sys.star, venus->body, false); g.setState(GameState::SPACE);
        Input in;
        auto press = [&](TestGame& G, int k) { in.pressed[k] = in.down[k] = true; G.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
        g.openScreen(GameState::DRONES);
        press(g, KEY_S);
        g.testSeedPage(venus->lat, venus->lon);
        const std::string ground = g.testSeedWhere();
        int ups = 0;
        for (size_t k = 0; k < H.size(); k++) if (H[k] == km) ups = (int)k + 1;
        for (int k = 0; k < (int)H.size(); k++) press(g, KEY_PAGE_UP);
        press(g, KEY_PAGE_UP);
        const std::string past = g.testStatus();
        for (int k = ups; k < (int)H.size(); k++) press(g, KEY_PAGE_DOWN);
        const std::string there = g.testSeedWhere();
        press(g, KEY_ENTER);
        const bool out = g.testBay().out && g.testBay().seedTrip && g.testBay().seedDepth == -km;
        g.testDronesSkip(6);
        g.saveSlot(1);
        TestGame h; h.savePrefix = g.savePrefix; h.settingsPath = g.settingsPath; h.keysPath = g.keysPath; h.guidePath = g.guidePath;
        h.guide.load(h.guidePath);
        const bool loaded = h.loadSlot(1) && h.testBay().seedTrip && h.testBay().seedDepth == -km;
        h.testDronesSkip(30);
        const bool let = h.guide.seeded.size() == 1 && h.guide.seeded[0].depthKm == -km;
        h.testDronesSkip(400);
        const std::string report = h.testStatus();
        const std::vector<Seeding>& ss = h.testSeedings(venus->body);
        const bool grew = ss.size() == 1 && ss[0].depthKm == -km && ss[0].verdict.place.aloft() && ss[0].verdict.takes() && report.find("CULTURE GROWS") != std::string::npos;
        check("seeding: the seed page's heights, a trip up into the clouds (J-14)",
              full && ground != there && there.find("ACID") == 0 && past.find("AS HIGH AS THE CLOUDS GO") != std::string::npos && out && loaded && let && grew,
              fmt("(flask 1 %s; THERE on the ground '%s', %.0f km up '%s'; past the highest '%s'; sent %s, a save %s, let go %s; back: '%s')", full ? "full" : "NOT FULL",
                  ground.c_str(), km, there.c_str(), past.c_str(), out ? "up" : "NOT UP", loaded ? "kept it" : "LOST IT", let ? "with its height" : "WITHOUT ITS HEIGHT",
                  report.c_str()));
        remove(g.guidePath.c_str()); remove(g.slotPath(1).c_str());
    }
}
