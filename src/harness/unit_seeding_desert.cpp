// The unit's `seeding` group, J-15 (R-454, a culture for any world): greening a desert. A green light-feeder tuned for the first desert of the
// forty with its STOP cut and its BIND set (its SHELL and LIFT cut, as a player cuts them for a carpet) lays a carpet over the desert's land
// that turns to grass and shrubs four steps behind its edge, thicker from twelve; the same line's carpet on a living world and a chalk line's
// coral never green; the desert is a GREENED DESERT once the grass covers half its land, its carpet harming nothing, its map mostly grass;
// its ground's grass and shrubs drawn in the grass bank in its pigment's green, nothing drawn before them; the editor's rule for what a line
// overgrows as is the layer's
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
                           phrase.find("GRASS AND SHRUBS") != std::string::npos && madeGreens(PT_DESERT) && !madeGreens(PT_FELISIAN) && !madeGreens(PT_SCORCHED_FELISIAN);
        check("seeding: a desert greened, a kind of its own (J-15)",
              first > 0 && mismatched == 0 && harmed == 0 && unlivable == 0 && wordsOff == 0 && named && grass * 2 > land,
              fmt("(GREENED DESERT from step %d, the land green %.0f%% then, %.0f%% the step before; %d steps off their rule, %d harmed by its "
                  "carpet, %d not more livable, %d with GRASS AND SHRUBS off; \"%s\", \"%s\", \"%s\"; at its 48th step %d of %d map texels of land grass)",
                  first, 100 * at, 100 * before, mismatched, harmed, unlivable, wordsOff, name.c_str(), shortName.c_str(), sentence.c_str(), grass, land));
    }
    {   // the ground: at its 48th step its place grass with shrubs and tufts drawn in the grass bank (its colour the pigment's green), none at its
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
            bank10 = 0;
            for (size_t i = 0; i < fb.idx.size(); i++) bank10 += ((fb.idx[i] >> 11) & 31) == 10;
            mat = sv.site.lod0.at(0, 0).material;
            const uint8_t* p = &fb.pal[(10 * 64 + 47) * 3];
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
              fmt("(its place %s at its 48th step: %d flora drawn, %d pixels in the grass bank, its lit colour %.2f %.2f %.2f; at its 11th %s, %d drawn; nobody's "
                  "%s, %d drawn; the editor's rule %s, the layer's %s)",
                  MATERIAL_NAMES[me], fe, be, le.r, le.g, le.b, MATERIAL_NAMES[m11], f11, MATERIAL_NAMES[m0], f0, MADE_GROWTH_NAMES[rule],
                  MADE_GROWTH_NAMES[end.spread[0].growth.type]));
    }
}
