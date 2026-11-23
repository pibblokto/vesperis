// The unit's `seeding` group, J-15 (R-454, a culture for any world): greening a desert. A green light-feeder tuned for the first desert of the
// forty with its STOP cut and its BIND set (its SHELL and LIFT cut, as a player cuts them for a carpet) lays a carpet over the desert's land
// that turns to grass and shrubs four steps behind its edge, thicker from twelve; the same line's carpet on a living world and a chalk line's
// coral never green; the desert is a GREENED DESERT once the grass covers half its land, its carpet harming nothing, its map mostly grass;
// its ground's grass and shrubs drawn in its line's bank in its pigment's green, nothing drawn before them; the editor's rule for what a line
// overgrows as is the layer's. B-482 (J-16): two lines of two colours green one desert each in its own, and a bare world with air that is no
// desert greens too
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/seeding.h"
#include <algorithm>
#include <cmath>

namespace {
const std::vector<std::pair<int, int>> CARPET = {{GN_STOP, -2}, {GN_BIND, -1}, {GN_SHELL, -2}, {GN_LIFT, -2}};
double clockAt(const Seeding& s, int step) { return (step * MADE_STEP_S + 1) / std::max(0.05, s.verdict.pace); }
// the point at an arc `d` (radians) from `p` toward bearing `a`
Vec3 arcFrom(const Vec3& p, double d, double a) {
    const Vec3 e1 = normalize(cross(p, std::fabs(p.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(p, e1);
    return normalize(p * std::cos(d) + (e1 * std::cos(a) + e2 * std::sin(a)) * std::sin(d));
}
}   // namespace

void unitSeedingDesert(const Check& check) {
    SeedAirCase D; std::vector<GeneEdit> edits; Seeding sd;
    if (!seedDesertLine(CARPET, D, edits, sd)) {
        check("seeding: a carpet's grass behind its edge on a desert (J-15)", false, "(no green light-feeder takes on a desert of the forty)");
        return;
    }
    const Body& b = D.sys.bodies[D.body];
    const BodyGen g0 = BodyGen::make(b);
    const Vec3 place = StarSystem::bodyFromLatLon(D.lat, D.lon);
    {   // at the line's 28th step: on 40 rays out from its place, each point its carpet's edge passed five steps ago or more grass, none passed
        // under three steps ago grass, the grass sparse near its front and thick from twelve steps behind; at its 11th the cover alone; the same
        // line on its own living world and a coral on the desert never green
        const MadeWorld m = madeWorldOf(D.sys, D.body, {sd}, clockAt(sd, 28));
        const MadeGrowth& G = m.spread[0].growth;
        BodyGen g = g0; g.made = m;
        int grass = 0, cover = 0, late = 0, early = 0, sparse = 0, sparseOf = 0, thick = 0, thickOf = 0;
        for (int r = 0; r < 40; r++)
            for (int k = 1; k <= 30; k++) {
                const Vec3 u = arcFrom(place, std::min(PI, G.reach * 1.1) * k / 30, r * TAU / 40);
                const SurfaceSample s = sampleSurface(g, u, 64);
                const double since = madeGrowthSince(m, u, g.seed);
                cover += madeGrowthMaterial(s.material); grass += s.material == MAT_GRASS;
                if (since >= MADE_GREEN_AFTER + 1 && madeGrowthMaterial(s.material)) late++;
                if (since >= 0 && since < MADE_GREEN_AFTER - 1 && s.material == MAT_GRASS) early++;
                if (s.material == MAT_GRASS && since < MADE_GREEN_AFTER + 3) { sparseOf++; sparse += s.veg < 0.45; }
                if (s.material == MAT_GRASS && since >= MADE_GREEN_THICK + 1) { thickOf++; thick += s.veg > 0.6; }
            }
        const MadeWorld m11 = madeWorldOf(D.sys, D.body, {sd}, clockAt(sd, 11));
        BodyGen g11 = g0; g11.made = m11;
        const int at11 = sampleSurface(g11, place, 64).material;
        // the same line on the living world it came from (its sea), and a chalk shell's coral on the desert
        bool living = false, livingGreens = true, coralGreens = true;
        std::string livingWords = "NO LIVING WORLD", coralWords = "NO CORAL";
        for (const SeedCase& c : seedCases()) {
            const Body& cb = c.sys.bodies[c.body];
            if (!kindHas(cb.type, KT_FLORA) || !kindHas(cb.type, KT_SEA)) continue;
            SeedLine wild;
            if (!seedNativeLine(c.sys, c.body, wild) || wild.kind.s.way != LW_LIGHT) continue;
            SeedLine line = seedLineFor(c.sys, c.body, wild.kind.s.index, seedGeneEdits(wild.genes, CARPET));
            double la = 0, lo = 0;
            if (!line.valid || !seedPlaceTaking(c.sys, c.body, line, la, lo)) continue;
            const Seeding s = seedingFor(c.sys, c.body, line, la, lo, 0);
            const MadeWorld mw = madeWorldOf(c.sys, c.body, {s}, clockAt(s, 64));
            livingGreens = mw.green > 0 || mw.spread[0].growth.greens || mw.kind == MDK_GREENED_DESERT;
            livingWords = fmt("%s's %s", KINDS[cb.type].name, MADE_GROWTH_NAMES[mw.spread[0].growth.type]);
            living = true;
            break;
        }
        {
            std::vector<std::pair<int, int>> chalk = {{GN_STOP, -2}, {GN_BIND, -1}, {GN_SHELL, SHL_CHALK}, {GN_LIFT, -2}};
            SeedAirCase C; std::vector<GeneEdit> ce; Seeding cs;
            if (seedDesertLine(chalk, C, ce, cs)) {
                const MadeWorld mc = madeWorldOf(C.sys, C.body, {cs}, clockAt(cs, 64));
                coralGreens = mc.green > 0 || mc.spread[0].growth.greens;
                coralWords = fmt("%s, %s", MADE_GROWTH_NAMES[mc.spread[0].growth.type], madeKindName(mc).empty() ? "no kind" : madeKindName(mc).c_str());
            }
        }
        check("seeding: a carpet's grass behind its edge on a desert (J-15)",
              G.greens && grass > 100 && cover > 20 && late == 0 && early == 0 && sparseOf > 0 && sparse >= sparseOf * 9 / 10 && thickOf > 100 &&
                      thick >= thickOf * 9 / 10 && madeGrowthMaterial(at11) && living && !livingGreens && !coralGreens,
              fmt("(%s, its %s: at its 28th step %d grass and %d cover of 1200 points, %d not grass five steps behind its edge, %d grass under three; "
                  "%d of %d sparse by its front, %d of %d thick twelve steps behind; at its 11th its place %s; %s%s; %s%s)",
                  KINDS[b.type].name, MADE_GROWTH_NAMES[G.type], grass, cover, late, early, sparse, sparseOf, thick, thickOf, MATERIAL_NAMES[at11],
                  livingWords.c_str(), livingGreens ? " GREENS" : " never greens", coralWords.c_str(), coralGreens ? " GREENS" : " never greens"));
    }
    {   // the kind by the steps: GREENED DESERT exactly where the grass covers half its land, its carpet harming nothing at any step and the
        // desert more livable once named (its waters' soup aside), its data sheet's words GRASS AND SHRUBS from the grass's first; its names;
        // at its 48th step its map mostly grass
        int first = -1, mismatched = 0, harmed = 0, unlivable = 0, wordsOff = 0;
        double before = 0, at = 0;
        for (int st = 8; st <= 48; st++) {
            const MadeWorld m = madeWorldOf(D.sys, D.body, {sd}, clockAt(sd, st));
            const bool gd = m.kind == MDK_GREENED_DESERT;
            mismatched += gd != (m.green >= 0.5);
            if (gd && first < 0) { first = st; at = m.green; }
            if (!gd && first < 0) before = m.green;
            MadeWorld dry = m; dry.seas = 0;   // (its carpet's harm alone: a soup in the desert's springs harms as a soup does anywhere)
            harmed += madeHarms(dry);
            unlivable += gd && !madeLivable(dry);
            const std::vector<std::string> w = madeTraitWords(m, false);
            wordsOff += (std::find(w.begin(), w.end(), "GRASS AND SHRUBS") != w.end()) != (m.green > 0);
        }
        const MadeWorld end = madeWorldOf(D.sys, D.body, {sd}, clockAt(sd, 48));
        BodyGen ge = g0; ge.made = end;
        PlanetMap pm; pm.generate(ge);
        int land = 0, grass = 0;
        for (size_t i = 0; i < pm.material.size(); i++) {
            if (pm.material[i] == MAT_WATER || pm.material[i] == MAT_LAVA) continue;
            land++; grass += pm.material[i] == MAT_GRASS;
        }
        const std::string name = madeKindName(end), shortName = madeKindShort(end), sentence = upper(madeKindSentence(end)), phrase = upper(madeKindPhrase(end));
        const bool named = name == "GREENED DESERT" && !shortName.empty() && shortName.size() <= 10 && sentence.find("GRASS AND SHRUBS") != std::string::npos &&
                           phrase.find("GRASS AND SHRUBS") != std::string::npos && madeGreens(PT_DESERT) && madeGreens(PT_RUST) && madeGreens(PT_SCORCHED_FELISIAN) &&
                           !madeGreens(PT_FELISIAN) && !madeGreens(PT_OCEAN);   // (B-482: any bare world with air, no plants or seas of its own)
        check("seeding: a desert greened, a kind of its own (J-15)",
              first > 0 && mismatched == 0 && harmed == 0 && unlivable == 0 && wordsOff == 0 && named && grass * 2 > land,
              fmt("(GREENED DESERT from step %d, the land green %.0f%% then, %.0f%% the step before; %d steps off their rule, %d harmed by its "
                  "carpet, %d not more livable, %d with GRASS AND SHRUBS off; \"%s\", \"%s\", \"%s\"; at its 48th step %d of %d map texels of land grass)",
                  first, 100 * at, 100 * before, mismatched, harmed, unlivable, wordsOff, name.c_str(), shortName.c_str(), sentence.c_str(), grass, land));
    }
    {   // the ground: at its 48th step its place grass with shrubs and tufts drawn in its line's bank (B-482; its colour the pigment's green), none at its
        // 11th (the carpet alone) nor on the desert nobody seeded; the editor's rule for what a cut STOP overgrows as the layer's own
        auto ground = [&](const MadeWorld& m, int& flora, int& bank10, int& mat, RGB& lit) {
            StarSystem sys = D.sys;
            SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
            sv.site.made = m;
            sv.init(&sys, D.body, D.lat, D.lon, 20000);
            nb.update(sv.site.worldPos(20000, 0, 0, 0));
            Input in;
            for (int k = 0; k < 3; k++) { sv.update(1.0 / 30, in, 20000, false); Framebuffer fb0; sv.render(fb0, 20000, nb.stars, sr, 1.0); }
            Framebuffer fb; sv.render(fb, 20000, nb.stars, sr, 1.0);
            flora = sv.lastFloraPoints + sv.lastFloraPolys;
            const int gb = sv.testGrowthLineBank(0) >= 0 ? sv.testGrowthLineBank(0) : 10;   // (B-482: its line's bank; the grass bank as generated)
            bank10 = 0;
            for (size_t i = 0; i < fb.idx.size(); i++) bank10 += ((fb.idx[i] >> 11) & 31) == gb;
            mat = sv.site.lod0.at(0, 0).material;
            const uint8_t* p = &fb.pal[(gb * 64 + 47) * 3];
            lit = RGB(p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f);
        };
        int fe = 0, be = 0, me = 0, f11 = 0, b11 = 0, m11 = 0, f0 = 0, b0 = 0, m0 = 0;
        RGB le, l11, l0;
        ground(madeWorldOf(D.sys, D.body, {sd}, clockAt(sd, 48)), fe, be, me, le);
        ground(madeWorldOf(D.sys, D.body, {sd}, clockAt(sd, 11)), f11, b11, m11, l11);
        ground(MadeWorld(), f0, b0, m0, l0);
        const MadeWorld end = madeWorldOf(D.sys, D.body, {sd}, clockAt(sd, 48));
        const int rule = overgrowthTypeOf(sd.wild.kind);
        const bool green = le.g > le.r * 1.2f && le.g > le.b * 1.2f;
        check("seeding: a greened desert's ground, grass and shrubs (J-15)",
              me == MAT_GRASS && fe > 50 && be > 500 && green && f11 == 0 && f0 == 0 && rule == end.spread[0].growth.type,
              fmt("(its place %s at its 48th step: %d flora drawn, %d pixels in its line's bank, its lit colour %.2f %.2f %.2f; at its 11th %s, %d drawn; nobody's "
                  "%s, %d drawn; the editor's rule %s, the layer's %s)",
                  MATERIAL_NAMES[me], fe, be, le.r, le.g, le.b, MATERIAL_NAMES[m11], f11, MATERIAL_NAMES[m0], f0, MADE_GROWTH_NAMES[rule],
                  MADE_GROWTH_NAMES[end.spread[0].growth.type]));
    }
    {   // B-482 (KI-509, KI-510): a second carpet line of another colour (the first light-feeder of water on the forty with another pigment than
        // green, tuned for the desert's far side and let go there with the first) greens the same desert in its own colour: on the ground each
        // line's grass and shrubs in its own line's bank, in its own plants' colour, at their 24th step (each in its own half: where both lie the
        // later's grass); and a bare world with air of the forty that is no desert (no plants or seas of its own) greens under the green line
        // tuned for its place (most refuse it: too cold, or the ultraviolet)
        StarSystem gs, ps; int gb = -1, gsp = -1, pb = -1, psp = -1;
        for (const SeedCase& c : seedCases()) {
            const auto life = lifeOf(c.sys);
            if (life->bodies[c.body].chemistry != CH_WATER) continue;
            for (const Microbe& mb : life->bodies[c.body].species) {
                if (mb.state != SS_LIVING || mb.way != LW_LIGHT || mb.pigment == PG_NONE) continue;
                if (mb.pigment == PG_GREEN && gsp < 0) { gs = c.sys; gb = c.body; gsp = mb.index; }
                if (mb.pigment != PG_GREEN && psp < 0) { ps = c.sys; pb = c.body; psp = mb.index; }
            }
        }
        const double la2 = -D.lat, lo2 = D.lon > 0 ? D.lon - PI : D.lon + PI;
        bool two = false, own = false, apart = false;
        int px1 = 0, px2 = 0, bank1 = -1, bank2 = -1;
        RGB c1, c2, p1, p2;
        std::string twoWords = "NO SECOND LINE";
        if (psp >= 0) {
            std::vector<GeneEdit> e2 = seedTunedEdits(ps, pb, psp, seedPlaceFacts(D.sys, D.body, la2, lo2));
            for (const GeneEdit& x : seedGeneEdits(seedLineFor(ps, pb, psp, {}).genes, CARPET)) e2.push_back(x);
            SeedLine line2 = seedLineFor(ps, pb, psp, e2); line2.key += "/S2";
            const std::vector<Seeding> first = {sd};
            const Seeding s2 = seedingFor(D.sys, D.body, line2, la2, lo2, 0, &first);
            const MadeWorld m2 = madeWorldOf(D.sys, D.body, {sd, s2}, std::max(clockAt(sd, 24), clockAt(s2, 24)));   // (each its own half: where both lie, the later's)
            two = s2.verdict.takes() && m2.lines == 2 && m2.spread[0].growth.greens && m2.spread[1].growth.greens;
            twoWords = fmt("a %s line's %s", PIGMENT_NAMES[clampi(line2.kind.s.pigment, 0, PG_COUNT - 1)], seedVerdictWords(s2.verdict).c_str());
            auto ground = [&](double la, double lo, int line, int& px, RGB& lit, int& bank) {
                StarSystem sys = D.sys;
                SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
                sv.site.made = m2;
                sv.init(&sys, D.body, la, lo, 20000);
                nb.update(sv.site.worldPos(20000, 0, 0, 0));
                Input in;
                for (int k = 0; k < 3; k++) { sv.update(1.0 / 30, in, 20000, false); Framebuffer fb0; sv.render(fb0, 20000, nb.stars, sr, 1.0); }
                Framebuffer fb; sv.render(fb, 20000, nb.stars, sr, 1.0);
                bank = sv.testGrowthLineBank(line);
                px = 0;
                for (size_t i = 0; i < fb.idx.size() && bank >= 0; i++) px += bankOf(fb.idx[i]) == bank;
                const uint8_t* q = &fb.pal[((bank >= 0 ? bank : 10) * 64 + 47) * 3];
                lit = RGB(q[0] / 255.0f, q[1] / 255.0f, q[2] / 255.0f);
            };
            if (two) {
                ground(D.lat, D.lon, 0, px1, c1, bank1);
                ground(la2, lo2, 1, px2, c2, bank2);
                p1 = m2.spread[0].growth.plants; p2 = m2.spread[1].growth.plants;
                auto chroma = [](const RGB& c) { const float s = std::max(1e-3f, c.r + c.g + c.b); return RGB(c.r / s, c.g / s, c.b / s); };
                auto far = [&](const RGB& a, const RGB& b) { const RGB x = chroma(a), y = chroma(b); return std::fabs(x.r - y.r) + std::fabs(x.g - y.g) + std::fabs(x.b - y.b); };
                own = px1 > 500 && px2 > 500 && far(c1, p1) < far(c1, p2) && far(c2, p2) < far(c2, p1);
                apart = bank1 >= 0 && bank2 >= 0 && bank1 != bank2 && far(p1, p2) > 0.1;
            }
        }
        // the bare worlds with air of the forty that are no desert
        int bare = 0, took = 0, greened = 0, named = 0;
        std::string bareWords;
        if (gsp >= 0)
            for (const SeedCase& c : seedCases()) {
                const int t = c.sys.bodies[c.body].type;
                if (t == PT_DESERT || !KINDS[t].landable || !KINDS[t].atmosphere || kindHas(t, KT_FLORA) || kindHas(t, KT_SEA)) continue;
                bare++;
                std::vector<GeneEdit> e = seedTunedEdits(gs, gb, gsp, seedPlaceFacts(c.sys, c.body, c.lat, c.lon));
                for (const GeneEdit& x : seedGeneEdits(seedLineFor(gs, gb, gsp, {}).genes, CARPET)) e.push_back(x);
                SeedLine line = seedLineFor(gs, gb, gsp, e); line.key += "/S1";
                const Seeding s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
                if (!s.verdict.takes()) continue;
                took++;
                const MadeWorld m = madeWorldOf(c.sys, c.body, {s}, clockAt(s, 64));
                greened += m.green > 0; named += m.kind == MDK_GREENED_DESERT;
                bareWords += fmt("%s%s %s%s", bareWords.empty() ? "" : ", ", KINDS[t].name, m.green > 0 ? fmt("%.0f%% green", 100 * m.green).c_str() : "not green",
                                 m.kind == MDK_GREENED_DESERT ? " GREENED DESERT" : "");
            }
        check("seeding: two lines green a desert each in its own colour, any bare world with air greens (B-482)",
              two && own && apart && took > 0 && greened > 0,
              fmt("(%s; at the first's place %d pixels in its bank %d, lit %.2f %.2f %.2f for its plants %.2f %.2f %.2f; at the second's %d in its bank %d, lit %.2f %.2f %.2f "
                  "for %.2f %.2f %.2f; %d bare worlds with air that are no desert, %d took the green line, %d named GREENED DESERT: %s)",
                  twoWords.c_str(), px1, bank1, c1.r, c1.g, c1.b, p1.r, p1.g, p1.b, px2, bank2, c2.r, c2.g, c2.b, p2.r, p2.g, p2.b, bare, took, named, bareWords.c_str()));
    }
}
