// J-11 (R-448): the unit's `seeding` group, the made kinds. Six lines (a world's own sea line with its gas set to CO2, methane, oxygen, a
// sulphide or a cold haze, or never stopping with its cells bound) on `seed climate`'s twenty worlds at their 64th step, serially and on the
// pool: each made kind by its rule, its words; a made hothouse's and a made snowball's ground (no water left, the land's heights and the rivers'
// beds as generated, the old forests dead); the ice's run step by step
#include "harness/harness.h"
#include "life/seeding.h"
#include "core/parallel.h"
#include <algorithm>

namespace {
const std::vector<std::pair<int, int>> KIND_EDITS[6] = {{{GN_GAS, WG_CO2}}, {{GN_GAS, WG_METHANE}}, {{GN_GAS, WG_OXYGEN}}, {{GN_GAS, WG_H2S}},
                                                        {{GN_GAS, WG_DMS}}, {{GN_STOP, -2}, {GN_BIND, -1}}};
double endOf(const Seeding& s) { return (MADE_GROWTH_STEPS * MADE_STEP_S + 1) / std::max(0.05, s.verdict.pace); }
}   // namespace

void unitSeedingKinds(const Check& check) {
    const std::vector<SeedAirCase> worlds = seedAirCases(20);
    struct Run { int w = 0, line = 0; Seeding s; MadeWorld m; };
    std::vector<Run> runs;
    for (size_t wi = 0; wi < worlds.size(); wi++)
        for (int k = 0; k < 6; k++) { Run r; r.w = (int)wi; r.line = k; runs.push_back(r); }
    auto work = [&](Run& r) {
        const SeedAirCase& c = worlds[r.w];
        const SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, KIND_EDITS[r.line]));
        r.s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
        r.m = madeWorldOf(c.sys, c.body, {r.s}, endOf(r.s));
    };
    std::vector<Run> pool = runs;
    for (Run& r : runs) work(r);
    parallelFor((int)pool.size(), 1, [&](int i0, int i1) { for (int i = i0; i < i1; i++) work(pool[i]); });
    {   // the same on any thread; each kind by its rule; its words, and none for a world nobody seeded
        int alike = 0, took = 0, bad = 0, counts[MDK_COUNT] = {};
        std::string firstBad;
        for (size_t i = 0; i < runs.size(); i++) {
            const Run& r = runs[i];
            const MadeWorld& m = r.m;
            alike += m.key == pool[i].m.key && m.kind == pool[i].m.kind && m.kindGrowth == pool[i].m.kindGrowth;
            if (!r.s.verdict.takes()) continue;
            took++; counts[clampi(m.kind, 0, MDK_COUNT - 1)]++;
            bool grownAll = false;
            for (int j = 0; j < m.lines && j < MADE_LINES; j++) {
                const MadeGrowth& G = m.spread[j].growth;
                grownAll = grownAll || (G.on() && G.type != OG_FILMS && G.reach >= PI && G.land >= 0.5 && G.type == m.kindGrowth);
            }
            std::string why;
            switch (m.kind) {
                case MDK_HOTHOUSE: if (!m.dried()) why = "a hothouse with seas"; break;
                case MDK_SNOWBALL: if (!m.frozen() || !(m.warmC < 0)) why = "a snowball not frozen"; break;
                case MDK_OVERGROWN: if (!grownAll || m.dried() || m.frozen()) why = "an overgrown world not covered"; break;
                case MDK_CHARNEL: if (!(m.fauna & MF_SILENCE)) why = "a charnel world with animals"; break;
                case MDK_PURPLE: if (!(m.seas & MS_PURPLE) || !(m.air & MA_SULPHUR)) why = "a purple world without its seas or air"; break;
                case MDK_RUST: if (!(m.grounds & MGT_RUST) || !(m.air & MA_OZONE)) why = "a rust world without its rust or ozone"; break;
                case MDK_HAZE: if (!(m.air & MA_ORANGE_HAZE)) why = "a haze world without its haze"; break;
                case MDK_GREENED: if (madeHarms(m) || !(m.flora & MP_LUSH)) why = "a greened world harmed"; break;
                default: if (m.dried() || m.frozen()) why = "a dried or frozen world with no kind"; break;
            }
            if (why.empty() && (m.kind != MDK_NONE) != !madeKindName(m).empty()) why = "a kind's name";
            if (why.empty() && m.kind != MDK_NONE && (madeKindSentence(m).empty() || madeKindPhrase(m).empty())) why = "a kind's words";
            if (!why.empty()) { bad++; if (firstBad.empty()) firstBad = fmt("; %s (%s, line %d)", why.c_str(), KINDS[worlds[r.w].sys.bodies[worlds[r.w].body].type].name, r.line); }
        }
        const MadeWorld none;
        const bool plain = none.kind == MDK_NONE && madeKindName(none).empty() && none.deadCover() == DC_NONE;
        int kinds = 0;
        std::string ks;
        for (int k = 1; k < MDK_COUNT; k++)
            if (counts[k]) { kinds++; MadeWorld one; one.kind = k; ks += fmt("%s%s %d", ks.empty() ? "" : ", ", madeKindName(one).c_str(), counts[k]); }
        check("seeding: the made kinds by their lines", alike == (int)runs.size() && took > 60 && kinds >= 5 && bad == 0 && plain,
              fmt("(%d of %zu seedings of six lines on %zu worlds alike serially and on the pool; %d took, %d ended as a made kind: %s; %d against their rule%s)", alike,
                  runs.size(), worlds.size(), took, took - counts[MDK_NONE], ks.empty() ? "none" : ks.c_str(), bad, firstBad.c_str()));
    }
    {   // a made hothouse's and a made snowball's ground: no water left anywhere (dried to their beds, frozen at their level), the land's heights
        // and the rivers' beds as generated, the old forests standing dead
        for (int kind : {MDK_HOTHOUSE, MDK_SNOWBALL}) {
            const Run* run = nullptr;
            for (const Run& r : runs) if (!run && r.m.kind == kind && kindHas(worlds[r.w].sys.bodies[worlds[r.w].body].type, KT_FLORA)) run = &r;
            const char* tag = kind == MDK_HOTHOUSE ? "a made hothouse's ground" : "a made snowball's ground";
            if (!run) { check(fmt("seeding: %s", tag).c_str(), false, "(none of the six lines made one on a world with plants)"); continue; }
            const Body& b = worlds[run->w].sys.bodies[worlds[run->w].body];
            const BodyGen g0 = BodyGen::make(b);
            BodyGen g1 = g0; g1.made = run->m;
            int wet = 0, left = 0, land = 0, moved = 0, beds = 0, bedsMoved = 0, forests = 0, deadForests = 0;
            for (int j = 0; j < 36; j++)
                for (int i = 0; i < 72; i++) {
                    const Vec3 u = StarSystem::bodyFromLatLon((j + 0.5) / 36 * PI - PI / 2, (i + 0.5) / 72 * TAU - PI);
                    const SurfaceSample a = sampleSurface(g0, u, 40000), z = sampleSurface(g1, u, 40000);
                    const bool wa = madeWetAt(a), wz = madeWetAt(z);
                    wet += wa; left += wz;
                    if (kind == MDK_SNOWBALL && wa) left += z.material != MAT_ICE && z.material != MAT_SNOW;   // (frozen at their level)
                    if (!wa && a.material != MAT_ICE && a.material != MAT_SNOW && a.height >= 0) { land++; moved += a.height != z.height; }
                    if (wa && a.waterKind != 0 && kind == MDK_HOTHOUSE) { beds++; bedsMoved += a.height != z.height; }   // (a river's or a lake's bed where it lay)
                    if (a.material == MAT_FOREST) { forests++; deadForests += z.material != MAT_FOREST && z.veg > 0.25; }
                }
            const bool dead = deadGroundOf(g1) && deadCoverOf(g1) == (kind == MDK_HOTHOUSE ? DC_SCORCHED : DC_FROZEN) && !deadGroundOf(g0);
            const std::string phrase = describeBody(b, g1);
            check(fmt("seeding: %s", tag).c_str(), wet > 0 && left == 0 && land > 0 && moved == 0 && bedsMoved == 0 && dead && forests > 0 && deadForests == forests &&
                          phrase.find(madeKindPhrase(run->m)) != std::string::npos,
                  fmt("(%s, ONCE %s: %d of its %d wet points left wet%s; %d of %d land points moved; %d of %d river and lake beds moved; %d of %d forests standing dead; "
                      "its ground %s)", madeKindName(run->m).c_str(), KINDS[b.type].name, left, wet, kind == MDK_SNOWBALL ? " or open" : "", moved, land, bedsMoved, beds,
                      deadForests, forests, dead ? "dead under its cover" : "NOT DEAD"));
        }
    }
    {   // the ice's run: a cold haze over a cold living world, step by step: nothing till its mean passes the snowball's line, FREEZING OVER from
        // that step, the world colder each step and its snow and ice spreading, then frozen six steps on (MADE SNOWBALL)
        const Run* cold = nullptr;
        for (const Run& r : runs) if (!cold && r.m.kind == MDK_SNOWBALL) cold = &r;
        int onset = -1, frozenAt = -1, warmer = 0, shrank = 0, n0 = 0, nEnd = 0, n = 0, open = -1;
        if (cold) {   // (by the clock's steps: the ice runs at the clock's pace, not its line's)
            const SeedAirCase& c = worlds[cold->w];
            const BodyGen g0 = BodyGen::make(c.sys.bodies[c.body]);
            double was = 1e9; int iceWas = -1;
            for (int st = 1; st <= 96 && (frozenAt < 0 || st <= frozenAt + 1); st++) {
                const MadeWorld m = madeWorldOf(c.sys, c.body, {cold->s}, st * MADE_STEP_S + 1);
                if (onset < 0 && (m.air & MA_FREEZING)) onset = st;
                if (frozenAt < 0 && m.kind == MDK_SNOWBALL) frozenAt = st;
                if (m.warmC > was + 1e-9) warmer++;
                was = m.warmC;
                if (onset < 0 || (frozenAt >= 0 && st > frozenAt)) continue;
                BodyGen g1 = g0; g1.made = m;
                int ice = 0, opened = 0; n = 0;
                for (int j = 0; j < 18; j++)
                    for (int i = 0; i < 36; i++) {
                        const SurfaceSample z = sampleSurface(g1, StarSystem::bodyFromLatLon((j + 0.5) / 18 * PI - PI / 2, (i + 0.5) / 36 * TAU - PI), 80000);
                        ice += z.material == MAT_ICE || z.material == MAT_SNOW; n++;
                        opened += madeWetAt(z) || z.material == MAT_FOREST || z.material == MAT_GRASS;   // (open water or living plants)
                    }
                if (st == frozenAt) open = opened;
                if (iceWas >= 0 && ice < iceWas) shrank++;
                if (iceWas < 0) n0 = ice;
                iceWas = ice; nEnd = ice;
            }
        }
        check("seeding: the ice's run", cold && onset > 0 && frozenAt == onset + 6 && warmer == 0 && shrank == 0 && open == 0 && n0 < nEnd,
              fmt("(%s: FREEZING OVER from the clock's step %d, frozen at its step %d; %d steps warmer than the one before; its snow and ice on %d of %d points as it "
                  "began, %d frozen (the rest bare rock and frost-thin soil), %d open water or living plants left%s)",
                  cold ? KINDS[worlds[cold->w].sys.bodies[worlds[cold->w].body].type].name : "none", onset, frozenAt, warmer, n0, n, nEnd, open, shrank ? "; IT SHRANK" : ""));
    }
}
