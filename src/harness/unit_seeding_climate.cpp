// The unit's `seeding` group, the made climate's known issues closed after J-12 (KI-456 to KI-458, KI-462, KI-484 to KI-486): every kind's
// caps and frosts moved by a made warmth, a living world's ice sheets and glaciers too; a later line read at the warmth the lines before it
// made; an ocean world run away to its dry floor; a line's air counting its plants; a made snowball's blue sea ice and a greened world's
// biomes; a made kind that remembers (a snowball warmed past its line still frozen, a hothouse cooled still dry); the cold's harm by the mean
// a world reaches
#include "harness/harness.h"
#include "life/seeding.h"
#include <algorithm>
#include <cmath>

namespace {
double endOfLine(const Seeding& s) { return s.clock + (MADE_GROWTH_STEPS * MADE_STEP_S + 1) / std::max(0.05, s.verdict.pace); }
double warmByOf(const Body& b) { return 0.6 + 0.9 * hash01(mix64(b.seed ^ 0xA1E0A2ULL)); }   // (the world's warming, as `airWorldOf` draws it)
// a line like `s` let go at `clock`, giving off `gas` at `rate` (its share, pace and place the same)
Seeding gasClone(const Seeding& s, double clock, int gas, double rate) {
    Seeding w = s;
    w.clock = clock; w.line += "/W";
    w.wild.kind.world.gas = gas; w.wild.kind.world.gasRate = rate;
    return w;
}
// the snow and the ice of a world's grid of 18 by 36 points (a steam world's polar seas), `bare` the ice on its land alone (a sheet's; the sea
// ice lies at the sea's level)
int iceOn(const BodyGen& g, int& bare) {
    int n = 0; bare = 0;
    for (int j = 0; j < 18; j++)
        for (int i = 0; i < 36; i++) {
            const SurfaceSample z = sampleSurface(g, StarSystem::bodyFromLatLon((j + 0.5) / 18 * PI - PI / 2, (i + 0.5) / 36 * TAU - PI), 80000);
            n += g.type == PT_STEAM ? z.material == MAT_WATER : (z.material == MAT_SNOW || z.material == MAT_ICE);
            bare += z.material == MAT_ICE && z.height > 1;
        }
    return n;
}
}   // namespace

void unitSeedingClimate(const Check& check) {
    const std::vector<SeedAirCase> worlds = seedAirCases(20);
    const std::vector<std::pair<int, int>> LINES[5] = {{{GN_GAS, WG_CO2}}, {{GN_GAS, WG_DMS}}, {{GN_HOST, HO_PLANTS}}, {{GN_GAS, WG_METHANE}}, {}};
    struct Run { int w = 0, line = 0; Seeding s; MadeWorld m; };
    std::vector<Run> runs;
    for (size_t wi = 0; wi < worlds.size(); wi++)
        for (int k = 0; k < 5; k++) {
            const SeedAirCase& c = worlds[wi];
            Run r; r.w = (int)wi; r.line = k;
            r.s = seedingFor(c.sys, c.body, seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, LINES[k])), c.lat, c.lon, 0);
            if (r.s.verdict.takes()) r.m = madeWorldOf(c.sys, c.body, {r.s}, endOfLine(r.s));
            runs.push_back(r);
        }
    auto bodyOf = [&](const Run& r) -> const Body& { return worlds[r.w].sys.bodies[worlds[r.w].body]; };

    {   // KI-456: a made warmth moves every kind's caps and frosts (a thin air's, an ocean's floes, a lichen's, a steam world's polar seas, an
        // eyeball's pupil, a dry ice world's, an ammonia world's frost line), as it moves a living world's; a living world's ice sheets lose
        // their snow to bare ice and its glaciers melt back, their heights as generated
        int kinds = 0, moved = 0, sheets = 0;
        std::string still;
        for (int type : {PT_THINATMO, PT_OCEAN, PT_LICHEN, PT_STEAM, PT_EYEBALL, PT_DRY_ICE, PT_AMMONIA, PT_HYCEAN, PT_TUNDRA}) {
            StarSystem S; int bi = -1;
            if (!firstBodyOfType(type, S, bi)) continue;
            const BodyGen g0 = BodyGen::make(S.bodies[bi]);
            BodyGen gw = g0, gc = g0;
            gw.made.warmC = 10; gc.made.warmC = -10;
            int b0, bw, bc;
            const int n0 = iceOn(g0, b0), nw = iceOn(gw, bw), nc = iceOn(gc, bc);
            kinds++;
            const bool ok = (nw < n0 || n0 == 0) && n0 < nc;   // (a world with none as generated gains some colder)
            moved += ok;
            if (type == PT_TUNDRA) sheets = bw - b0;   // (its sheets' snow melted to bare ice)
            if (!ok) still += fmt("%s%s %d/%d/%d", still.empty() ? "" : ", ", KINDS[type].name, nc, n0, nw);
        }
        check("seeding: a made warmth moves every kind's caps", kinds >= 8 && moved == kinds && sheets > 0,
              fmt("(%d of %d kinds' snow and ice, a steam world's polar seas, fewer 10 C warmer and more 10 C colder%s%s; a tundra's ice sheets %d more points of bare "
                  "ice on its land warmed)", moved, kinds,
                  still.empty() ? "" : ": NOT ", still.c_str(), sheets));
    }
    {   // KI-457: a line let go after another is judged at the warmth the first made (its place's mean), and a spread's band reads it too
        int later = 0, right = 0, bands = 0;
        std::string first;
        for (const Run& r : runs) {
            if (r.line != 0 || !r.s.verdict.takes() || r.m.warmC < 3 || r.s.verdict.place.hab.niche != NI_LIQUID) continue;
            const SeedAirCase& c = worlds[r.w];
            const double at = endOfLine(r.s);
            const std::vector<Seeding> earlier = {r.s};
            const Seeding second = seedingFor(c.sys, c.body, c.wild, c.lat, c.lon, at, &earlier);
            const MadeWorld before = madeWorldOf(c.sys, c.body, {r.s}, at);
            const double d = second.verdict.place.hab.meanC - r.s.verdict.place.hab.meanC;
            later++;
            const bool ok = std::fabs(d - before.warmC) < 0.25 || second.verdict.place.hab.niche != NI_LIQUID ||   // (a sea the first froze or dried is its ice or its floor;
                            r.s.verdict.place.hab.meanC <= -1.95;   // B-475: one at its freezing point as generated, its own mean under it)
            right += ok;
            if (first.empty() || !ok) first = fmt("; %s's sea %.1f C as generated, %.1f C for a line let go once its first made it %.1f C warmer", KINDS[bodyOf(r).type].name,
                                           r.s.verdict.place.hab.meanC, second.verdict.place.hab.meanC, before.warmC);
            const BodyGen g = BodyGen::make(bodyOf(r));
            const Vec3 u = StarSystem::bodyFromLatLon(c.lat, c.lon);
            const SurfaceSample s = sampleSurface(g, u, 1000);
            MadeWorld none = before; none.warmC = 0;
            bands += std::fabs(madeMeanAt(g, before, u, s, madeWetAt(s), 1) - madeMeanAt(g, none, u, s, madeWetAt(s), 1) - before.warmC) < 1e-9 ||
                     r.s.verdict.place.hab.meanC <= -1.95;   // (B-475: a sea at its freezing point as generated)
        }
        check("seeding: a later line read at the warmth made", later > 0 && right == later && bands == later,
              fmt("(%d of %d later lines' seas at the warmth the first made, %d bands read at it%s)", right, later, bands, first.c_str()));
    }
    {   // KI-458: a world of seas that is not a living one (an ocean, a hycean, an eyeball) runs away too, its seas boiled off to a dry floor of
        // salt and dust (MADE HOTHOUSE), its floor's heights as generated
        int oceans = 0, ran = 0, wetLeft = 0, salt = 0, dust = 0;
        std::string what;
        for (const Run& r : runs) {
            const Body& b = bodyOf(r);
            if (r.line != 0 || !r.s.verdict.takes() || !kindHas(b.type, KT_SEA) || livingGround(b.type)) continue;
            oceans++;
            const SeedAirCase& c = worlds[r.w];
            std::vector<Seeding> hot;   // (a strong greenhouse in a world's three lines: these worlds lie at -15 to -33 C, and one gas warms 45 C at most)
            for (int gas : {WG_CO2, WG_AMMONIA, WG_METHANE}) hot.push_back(gasClone(r.s, (double)hot.size(), gas, 1.5));
            const MadeWorld m = madeWorldOf(c.sys, c.body, hot, endOfLine(hot.back()));
            if (m.kind != MDK_HOTHOUSE) continue;
            ran++;
            BodyGen g = BodyGen::make(b); g.made = m;
            for (int j = 0; j < 18; j++)
                for (int i = 0; i < 36; i++) {
                    const SurfaceSample z = sampleSurface(g, StarSystem::bodyFromLatLon((j + 0.5) / 18 * PI - PI / 2, (i + 0.5) / 36 * TAU - PI), 80000);
                    wetLeft += madeWetAt(z); salt += z.material == MAT_SALT; dust += z.material == MAT_DUST;
                }
            if (what.empty()) what = fmt("; %s: \"%s\"", KINDS[b.type].name, madeKindSentence(m).c_str());
        }
        check("seeding: an ocean world runs away", oceans > 0 && ran > 0 && wetLeft == 0 && salt > 0 && dust > 0,
              fmt("(%d of %d worlds of seas that are not living ones dried by three strong greenhouse lines; %d wet points left, %d salt, %d dust%s)", ran, oceans, wetLeft, salt, dust,
                  what.c_str()));
    }
    {   // KI-462: a host of plants' share counts the plants it lives in (the waters and the land alone gave it less)
        int hosts = 0, more = 0;
        for (const Run& r : runs) {
            if (r.line != 2 || !r.s.verdict.takes() || !kindHas(bodyOf(r).type, KT_FLORA)) continue;
            const Run& plain = runs[&r - &runs[0] + 2];   // (the world's own line, no edit)
            if (!plain.s.verdict.takes()) continue;
            hosts++; more += r.s.verdict.share > plain.s.verdict.share;
        }
        check("seeding: a line's air counts its plants", hosts > 0 && more > 0, fmt("(%d of %d hosts of plants on more of their world than their own line alone)", more, hosts));
    }
    {   // KI-484: a made snowball's sea ice the frozen felisian's blue (in the grass family's bank), a greened world's biomes moved by its
        // warmth, its heights as generated
        const Run* snow = nullptr; const Run* green = nullptr;
        for (const Run& r : runs) {
            if (!snow && r.m.kind == MDK_SNOWBALL) snow = &r;
            if (!green && r.m.kind == MDK_GREENED) green = &r;
        }
        bool blue = false; int biomes = 0, heights = 0;
        if (snow) {
            BodyGen g = BodyGen::make(bodyOf(*snow)); g.made = snow->m;
            const RGB c = madeMatColour(g, MAT_ICE);
            blue = matFamilyOnGen(g, MAT_ICE) == FAM_GRASS && c.b > c.r + 0.15f && madeMatColour(g, familyRep(g.type, FAM_GRASS)).b == c.b;
        }
        if (green) {
            const BodyGen g0 = BodyGen::make(bodyOf(*green));
            BodyGen g1 = g0; g1.made = green->m;
            for (int j = 0; j < 18; j++)
                for (int i = 0; i < 36; i++) {
                    const Vec3 u = StarSystem::bodyFromLatLon((j + 0.5) / 18 * PI - PI / 2, (i + 0.5) / 36 * TAU - PI);
                    const SurfaceSample a = sampleSurface(g0, u, 80000), z = sampleSurface(g1, u, 80000);
                    biomes += a.biome != z.biome && a.biome != BIO_ICE && z.biome != BIO_ICE;   // (beyond the ice's edge, where J-04 moved them)
                    heights += a.height >= 0 && !madeWetAt(a) && a.material != MAT_ICE && a.height != z.height;   // (the land's: sea ice lies at the sea's level)
                }
        }
        check("seeding: a made snowball's ice and a greened world's biomes", snow && blue && green && biomes > 0 && heights == 0,
              fmt("(%s; %s: %d of 648 points' biomes moved beyond the ice's edge, %d of its land's heights)", !snow ? "NO MADE SNOWBALL" : (blue ? "a made snowball's sea ice blue in the grass bank" : "ITS SEA ICE WHITE"),
                  green ? KINDS[bodyOf(*green).type].name : "NO GREENED WORLD", biomes, heights));
    }
    {   // KI-485: a made kind remembers: a snowball's gases warmed past its line by a later CO2 line stays frozen until they pass its ice's own
        // cooling too; a hothouse's seas cooled under its line by a later cold haze stay boiled off
        const Run* cold = nullptr; const Run* hot = nullptr;
        for (const Run& r : runs) {
            if (!cold && r.m.kind == MDK_SNOWBALL) cold = &r;
            if (!hot && r.m.kind == MDK_HOTHOUSE) hot = &r;
        }
        int held = 0, wrong = 0, thawed = 0, dryHeld = 0, back = 0;
        if (cold) {
            const SeedAirCase& c = worlds[cold->w];
            const double wb = warmByOf(bodyOf(*cold)), line = -6.0 + 20.0 * wb;
            for (int k = 1; k <= 15; k++) {
                const Seeding warm = gasClone(cold->s, endOfLine(cold->s), WG_CO2, 0.1 * k);
                const MadeWorld m = madeWorldOf(c.sys, c.body, {cold->s, warm}, endOfLine(warm));
                const bool frozen = m.kind == MDK_SNOWBALL;
                const double gases = m.worldC + m.warmC + (frozen ? 20.0 * wb : 0.0);
                if (frozen && gases > -5.5 && gases < line - 0.5) held++;
                if (!frozen && gases < line - 0.5) wrong++;
                if (!frozen && gases >= line - 0.5) thawed++;
            }
        }
        if (hot) {
            const SeedAirCase& c = worlds[hot->w];
            for (int k = 1; k <= 15; k++) {
                const Seeding cool = gasClone(hot->s, endOfLine(hot->s), WG_DMS, 0.1 * k);
                const MadeWorld m = madeWorldOf(c.sys, c.body, {hot->s, cool}, endOfLine(cool));
                const double gases = m.worldC + m.warmC - 25.0;   // (the steam's own 25 C left out)
                if (m.dried() && gases < 42) dryHeld++;
                back += !m.dried();
            }
        }
        check("seeding: a made kind remembers", cold && hot && held > 0 && wrong == 0 && dryHeld > 0 && back == 0,
              fmt("(a %s snowball warmed by fifteen CO2 lines: %d still frozen with its gases past -6 C, %d thawed under the ice's own line, %d thawed past it; a %s "
                  "hothouse cooled by fifteen cold hazes: %d still dry with its gases under 42 C, %d with its seas back)", cold ? KINDS[bodyOf(*cold).type].name : "NO",
                  held, wrong, thawed, hot ? KINDS[bodyOf(*hot).type].name : "NO", dryHeld, back));
    }
    {   // KI-486: the cold harms by the mean a world reaches: a warm world cooled into the temperate keeps its animals, a cold one cooled loses them
        int spared = 0, warmCooled = 0, coldHarmed = 0, coldCooled = 0;
        for (const Run& r : runs) {
            if (r.line != 1 || !r.s.verdict.takes() || !kindHas(bodyOf(r).type, KT_FAUNA) || r.m.warmC > -4) continue;
            const double reached = r.m.worldC + r.m.warmC;
            if (reached >= 15) { warmCooled++; spared += r.m.fate.death != MD_COLD; }
            if (reached <= 5 && !r.m.frozen()) { coldCooled++; coldHarmed += r.m.fate.death == MD_COLD; }
        }
        check("seeding: the cold harms by the mean reached", warmCooled > 0 && spared == warmCooled,
              fmt("(%d of %d warm worlds cooled 4 C or more into the temperate or over spared the cold; %d of %d cooled to 5 C or under lost animals to it)", spared,
                  warmCooled, coldHarmed, coldCooled));
    }
    {   // B-476 (KI-499): a line's share is read at the warmth its own gas makes too (with the lines before it) and kept where that is more, so
        // none lives on more of its world at the warmth it made than its gas was given off for (one of the habitat's 96 points at most); and a
        // line its own warmth drives back keeps its air (its share never under the one it went down with)
        int changing = 0, more = 0, less = 0, opened = 0; double worst = 0; std::string worstAt;
        for (const Run& r : runs) {
            if (!r.s.verdict.takes() || std::fabs(r.m.warmC) < 1) continue;
            const SeedAirCase& c = worlds[r.w];
            const double made = madeWorldOf(c.sys, c.body, {r.s}, 1e18).warmC, at = seedShareAt(c.sys, c.body, r.s, made);
            const double first = seedShareAt(c.sys, c.body, r.s, 0);   // (the world as generated: the share it went down with)
            changing++; more += at > r.s.verdict.share + 1.5 / 96; less += r.s.verdict.share < first - 0.5 / 96; opened += r.s.verdict.share > first + 0.5 / 96;
            if (at - r.s.verdict.share > worst) {
                worst = at - r.s.verdict.share;
                worstAt = fmt("; the worst a %s's: %.3f given off for, %.3f at the %+.1f C it made", KINDS[bodyOf(r).type].name, r.s.verdict.share, at, made);
            }
        }
        check("seeding: a line's share read at its own warming (B-476)", changing > 10 && more == 0 && less == 0,
              fmt("(%d of %d lines that warm or cool their world 1 C or more live on more of it at the warmth they made than their gas was given off for%s; %d "
                  "under the share they went down with; %d spread further on the world they made)", more, changing, worstAt.c_str(), less, opened));
    }
    {   // B-493 (KI-500): a warmed living world's ice sheets draw back: their margin, out to a share of the dome that grows with the warmth, melts
        // to dark ice (a dark zone), the white ice poleward of it shrinking; every height as generated (the first tundra, 36 by 72 points)
        StarSystem S; int bi = -1;
        int white[3] = {}, dark[3] = {}; double moved = 0;
        if (firstBodyOfType(PT_TUNDRA, S, bi)) {
            const BodyGen g0 = BodyGen::make(S.bodies[bi]);
            const double warm[3] = {0, 3, 6};
            std::vector<double> h0;
            for (int w = 0; w < 3; w++) {
                BodyGen g = g0; g.made.warmC = warm[w];
                for (int j = 0; j < 36; j++)
                    for (int i = 0; i < 72; i++) {
                        const SurfaceSample z = sampleSurface(g, StarSystem::bodyFromLatLon((j + 0.5) / 36 * PI - PI / 2, (i + 0.5) / 72 * TAU - PI), 20000);
                        const bool sheet = z.height > 1 && (z.material == MAT_SNOW || z.material == MAT_ICE);
                        if (sheet) (z.albedo < 0.4 ? dark[w] : white[w])++;
                        if (w == 0) h0.push_back(sheet ? z.height : -1e9);   // (the sheets' heights alone: a sea's ice melts to its water)
                        else if (h0[j * 72 + i] > -1e8) moved = std::max(moved, std::fabs(z.height - h0[j * 72 + i]));
                    }
            }
        }
        check("seeding: a warmed ice sheet's margin melts dark, its white ice drawn back, its heights as generated (B-493)",
              bi >= 0 && dark[0] == 0 && dark[1] > 0 && dark[2] > dark[1] && white[2] < white[1] && white[1] < white[0] && moved == 0,
              fmt("(the first tundra's ice on its land at 0, +3 and +6 C: white %d, %d, %d; dark %d, %d, %d; the most a height moved %.1f m)", white[0], white[1], white[2],
                  dark[0], dark[1], dark[2], moved));
    }
    {   // B-475 (KI-498): a line's gas answers by how much of the room its kind of place has on its world it fills (its waters of its salts, or its
        // land), so a sea line on a terminator world, whose seas are a hundredth of it, changes its air; over `seed kinds`' forty worlds and six
        // lines, under one seeding in twelve shows nothing (the design: none but where a line has no room to change)
        const std::vector<SeedAirCase> forty = seedAirCases(40, 12);
        const std::vector<std::pair<int, int>> SIX[6] = {{{GN_GAS, WG_CO2}}, {{GN_GAS, WG_METHANE}}, {{GN_GAS, WG_OXYGEN}}, {{GN_GAS, WG_H2S}},
                                                         {{GN_GAS, WG_DMS}}, {{GN_STOP, -2}, {GN_BIND, -1}}};
        int took = 0, nothing = 0, terminators = 0, terminatorsNamed = 0;
        std::string first;
        for (const SeedAirCase& c : forty) {
            const bool seas = kindHas(c.sys.bodies[c.body].type, KT_SEA), term = c.sys.bodies[c.body].type == PT_TERMINATOR;
            for (int k = 0; k < 6; k++) {
                const Seeding s = seedingFor(c.sys, c.body, seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, SIX[k])), c.lat, c.lon, 0);
                if (!s.verdict.takes()) continue;
                took++;
                const MadeWorld m = madeWorldOf(c.sys, c.body, {s}, (MADE_GROWTH_STEPS * MADE_STEP_S + 1) / std::max(0.05, s.verdict.pace));
                const bool none = !madeHarms(m) && !madeLivable(m) && madeTraitWords(m, seas).empty();
                nothing += none;
                if (term && k < 5) { terminators++; terminatorsNamed += !none; }
                if (none && first.empty()) first = fmt("; the first a %s's line of %.3f of its world", KINDS[c.sys.bodies[c.body].type].name, s.verdict.share);
            }
        }
        check("seeding: a line's gas by the room it fills, a terminator world's sea line seen (B-475)", took > 200 && nothing * 12 < took && terminators > 0 &&
                      terminatorsNamed * 2 > terminators,
              fmt("(%d of %d seedings of six lines on forty worlds show nothing%s; %d of %d gas lines on terminator worlds change something)", nothing, took, first.c_str(),
                  terminatorsNamed, terminators));
    }
}
