// J-15 (R-454): and a greened desert's recipe, a green light-feeder tuned for the first desert of the forty (`seedDesertLine`), its carpet's grass
// by the steps (`shots/tests/kinds/desert_*.png`).
// J-11 (R-448): the made kinds. `seed kinds`: six lines on forty worlds (`seed climate`'s worlds of the living and the wet kinds, each with its
// own sea line and a place of its sea where that takes): the line with its gas set to CO2, methane, oxygen, hydrogen sulphide or dimethyl
// sulphide, or never stopping with its cells bound, each to its 64th step (an overgrowth's last). The made kind each world ends as, what it
// shows, and whether the seeding harmed the world's life, changed only its looks or made it more livable, with the shares of those that took
// (IDEAS-seeded-worlds.md section 5). Then each made kind's recipe on the first of those worlds where its lines take and make it: the globe
// from the parking orbit before and after, the data sheet and the ground after (`shots/tests/kinds/<kind>_<orbit_before|orbit|sheet|ground>.png`)
#include "harness.h"
#include "game/ui.h"
#include "life/seeding.h"
#include "test_game.h"
#include <sys/stat.h>

namespace {
// a kind's recipe: its lines' genes on the world's own sea species (one line each, let go at its sea's first place where it takes), the
// overgrowth's type it must have (OG_NONE any)
// the table's six lines: a world's own sea line with these genes set
const char* const KIND_LINE_NAMES[6] = {"CO2", "methane", "oxygen", "a sulphide", "a cold haze", "overgrowing"};
const std::vector<std::pair<int, int>> KIND_LINES[6] = {{{GN_GAS, WG_CO2}}, {{GN_GAS, WG_METHANE}}, {{GN_GAS, WG_OXYGEN}}, {{GN_GAS, WG_H2S}},
                                                        {{GN_GAS, WG_DMS}}, {{GN_STOP, -2}, {GN_BIND, -1}}};
struct KindRecipe {
    int kind;
    int growth;   // (OG_NONE any)
    int way;      // its species: -1 the world's own line (`seedNativeLine`'s), -2 each of its living sea species in turn, -3 (J-15) the first
                  // felisian world's sea light-feeder (`seedSource`) tuned for each dry world's place of the forty (`seedCases`, `madeGreens`)
    const char* tag;
    std::vector<std::vector<std::pair<int, int>>> lines;
};
std::vector<KindRecipe> kindRecipes() {
    return {
        {MDK_HOTHOUSE, OG_NONE, -1, "hothouse", {{{GN_GAS, WG_CO2}}}},
        {MDK_SNOWBALL, OG_NONE, -1, "snowball", {{{GN_GAS, WG_DMS}}}},
        {MDK_HAZE, OG_NONE, -1, "haze", {{{GN_GAS, WG_METHANE}}}},
        {MDK_PURPLE, OG_NONE, -1, "purple", {{{GN_GAS, WG_H2S}}}},
        {MDK_RUST, OG_NONE, -1, "rust", {{{GN_GAS, WG_OXYGEN}}}},
        {MDK_OVERGROWN, OG_FLESH, -2, "flesh", {{{GN_STOP, -2}, {GN_BIND, -1}}}},
        {MDK_OVERGROWN, OG_CORAL, -1, "coral", {{{GN_STOP, -2}, {GN_BIND, -1}, {GN_SHELL, SHL_CHALK}}}},
        {MDK_CHARNEL, OG_NONE, -1, "charnel", {{{GN_GAS, WG_H2S}}, {{GN_POISON, PZ_BLOOD}, {GN_HOST, HO_ANIMALS}, {GN_ROT, ROT_WOOD}, {GN_DRY, -1}}}},
        {MDK_GREENED, OG_NONE, -1, "greened", {{{GN_GAS, WG_CO2}}}},
        {MDK_GREENED_DESERT, OG_NONE, -3, "desert", {{{GN_STOP, -2}, {GN_BIND, -1}, {GN_SHELL, -2}, {GN_LIFT, -2}}}},
    };
}
// a recipe's species on a world: its own line, or each of its living sea species in turn (twelve at most)
std::vector<SeedLine> recipeWilds(const SeedAirCase& c, int way) {
    if (way == -1) return {c.wild};
    std::vector<SeedLine> out;
    for (const Microbe& mb : lifeOf(c.sys)->bodies[c.body].species)
        if (mb.state == SS_LIVING && mb.niche == NI_LIQUID && out.size() < 12) { SeedLine w = seedLineFor(c.sys, c.body, mb.index, {}); if (w.valid) out.push_back(w); }
    return out;
}
// the clock a seeding let go at 0 has its 64th step by
double endClock(const Seeding& s) { return (MADE_GROWTH_STEPS * MADE_STEP_S + 1) / std::max(0.05, s.verdict.pace); }
std::string wordsOf(const MadeWorld& m, bool seas) {
    std::string w;
    for (const std::string& t : madeTraitWords(m, seas)) w += (w.empty() ? "" : ", ") + t;
    return w;
}
const char* const SHARE_NAMES[4] = {"harms", "looks", "more livable", "nothing"};
int shareOf(const MadeWorld& m, bool seas) {
    if (madeHarms(m)) return 0;
    if (madeLivable(m)) return 2;
    return madeTraitWords(m, seas).empty() ? 3 : 1;
}
}   // namespace

bool seedDesertLine(const std::vector<std::pair<int, int>>& wants, SeedAirCase& world, std::vector<GeneEdit>& edits, Seeding& sd) {
    const std::vector<SeedCase> worlds = seedCases();
    StarSystem ls; int lb = -1, lsp = -1;   // (the first light-feeder of water with a green pigment living on the forty worlds: its grass green)
    for (const SeedCase& c : worlds) {
        if (lsp >= 0) break;
        const auto life = lifeOf(c.sys);
        if (life->bodies[c.body].chemistry != CH_WATER) continue;
        for (const Microbe& mb : life->bodies[c.body].species)
            if (mb.state == SS_LIVING && mb.way == LW_LIGHT && mb.pigment == PG_GREEN) { ls = c.sys; lb = c.body; lsp = mb.index; break; }
    }
    if (lsp < 0) return false;
    const SeedLine wild = seedLineFor(ls, lb, lsp, {});
    if (!wild.valid) return false;
    for (const SeedCase& c : worlds) {
        if (c.sys.bodies[c.body].type != PT_DESERT) continue;   // (J-16: J-15's desert, the first; other bare worlds green too)
        std::vector<GeneEdit> e = seedTunedEdits(ls, lb, lsp, seedPlaceFacts(c.sys, c.body, c.lat, c.lon));
        for (const GeneEdit& x : seedGeneEdits(wild.genes, wants)) e.push_back(x);
        SeedLine line = seedLineFor(ls, lb, lsp, e);
        line.key += "/S1";
        if (!line.valid) continue;
        const Seeding s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
        if (!s.verdict.takes()) continue;
        world.sys = c.sys; world.body = c.body; world.lat = c.lat; world.lon = c.lon; world.wild = line;
        edits = e; sd = s;
        return true;
    }
    return false;
}

int seedKinds() {
    const double t0 = nowSec();
    mkdir("shots/tests/kinds", 0755);
    // the table: six lines on forty worlds, each to its end
    const std::vector<SeedAirCase> cases = seedAirCases(40, 12);
    printf("seed kinds: %zu worlds' own sea lines with six edits, each at its 64th step\n", cases.size());
    int kinds[MDK_COUNT] = {}, shares[4] = {}, took = 0;
    std::string grown;
    for (size_t wi = 0; wi < cases.size(); wi++) {
        const SeedAirCase& c = cases[wi];
        const Body& b = c.sys.bodies[c.body];
        const bool seas = kindHas(b.type, KT_SEA);
        printf("world %2zu: %s at %lld %lld %lld body %d, species %d\n", wi + 1, KINDS[b.type].name, (long long)c.sys.star.sx, (long long)c.sys.star.sy, (long long)c.sys.star.sz,
               c.body, c.wild.kind.s.index);
        for (int k = 0; k < 6; k++) {
            const SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, KIND_LINES[k]));
            const Seeding sd = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
            if (!sd.verdict.takes()) { printf("  %-12s %s\n", KIND_LINE_NAMES[k], seedVerdictWords(sd.verdict).c_str()); continue; }
            took++;
            const MadeWorld m = madeWorldOf(c.sys, c.body, {sd}, endClock(sd));
            const int sh = shareOf(m, seas);
            shares[sh]++;
            kinds[clampi(m.kind, 0, MDK_COUNT - 1)]++;
            const std::string name = madeKindName(m);
            if (m.kind == MDK_OVERGROWN && grown.find(name) == std::string::npos) grown += (grown.empty() ? "" : ", ") + name;
            const std::string w = wordsOf(m, seas);
            printf("  %-12s %-20s %-12s %s\n", KIND_LINE_NAMES[k], name.empty() ? "-" : name.c_str(), SHARE_NAMES[sh], w.empty() ? "nothing" : w.c_str());
        }
    }
    std::string ks;
    for (int k = 1; k < MDK_COUNT; k++) {
        if (!kinds[k]) continue;
        MadeWorld one; one.kind = k;
        ks += fmt("%s%s %d", ks.empty() ? "" : ", ", k == MDK_OVERGROWN ? fmt("OVERGROWN (%s)", grown.c_str()).c_str() : madeKindName(one).c_str(), kinds[k]);
    }
    auto pct = [&](int n) { return took ? 100.0 * n / took : 0.0; };
    printf("seed kinds: %zu worlds, %d seedings took: %d harm their world's life (%.0f%%), %d change only its looks (%.0f%%), %d make it more livable (%.0f%%), %d show "
           "nothing (%.0f%%); %d end as a made kind%s%s\n",
           cases.size(), took, shares[0], pct(shares[0]), shares[1], pct(shares[1]), shares[2], pct(shares[2]), shares[3], pct(shares[3]), took - kinds[MDK_NONE],
           ks.empty() ? "" : ": ", ks.c_str());

    // the frames: each kind's recipe on the first world of `seed climate`'s where its lines take and make it (J-15: a greened desert's on the
    // first dry world of the forty where the tuned light-feeder takes and greens it)
    int drawn = 0;
    std::string missing;
    const std::vector<KindRecipe> recipes = kindRecipes();
    SeedAirCase desert;   // (J-15's world, kept while its recipe draws)
    for (const KindRecipe& R : recipes) {
        const SeedAirCase* W = nullptr;
        std::vector<Seeding> sds;
        std::vector<std::vector<GeneEdit>> edits;
        MadeWorld m;
        SeedLine rw;
        if (R.way == -3) {   // J-15: a green light-feeder tuned for the first dry world's place (its warmth, DRY), the recipe's genes on top
            std::vector<GeneEdit> e; Seeding sd;
            if (seedDesertLine(R.lines[0], desert, e, sd)) {
                sds = {sd}; edits = {e}; rw = desert.wild;
                m = madeWorldOf(desert.sys, desert.body, sds, endClock(sd));
                if (m.kind == R.kind) W = &desert;
            }
        }
        for (const SeedAirCase& c : cases) {
            if (R.way == -3) break;
            for (const SeedLine& w : recipeWilds(c, R.way)) {
                sds.clear(); edits.clear();
                rw = w;
                bool ok = true;
                for (size_t li = 0; li < R.lines.size() && ok; li++) {
                    const std::vector<GeneEdit> e = seedGeneEdits(rw.genes, R.lines[li]);
                    SeedLine line = seedLineFor(c.sys, c.body, rw.kind.s.index, e);
                    line.key += fmt("/S%zu", li + 1);
                    double la = 0, lo = 0;
                    if (!line.valid || (R.growth != OG_NONE && !line.kind.world.overgrows) || !seedPlaceTaking(c.sys, c.body, line, la, lo)) { ok = false; break; }
                    sds.push_back(seedingFor(c.sys, c.body, line, la, lo, 0));
                    edits.push_back(e);
                }
                if (!ok || sds.empty()) continue;
                double end = 0;
                for (const Seeding& s : sds) end = std::max(end, endClock(s));
                m = madeWorldOf(c.sys, c.body, sds, end);
                if (m.kind != R.kind || (R.growth != OG_NONE && m.kindGrowth != R.growth)) continue;
                if (m.kind == MDK_OVERGROWN && [&] { const RGB c = m.spread[m.growthLine].growth.colour; return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b; }() > 0.7) continue;   // (a pale one is hard to tell from bare ground)
                W = &c;
                break;
            }
            if (W) break;
        }
        if (!W) { missing += fmt("%s%s", missing.empty() ? "" : ", ", R.tag); printf("  %-10s no world of the kinds tried where its lines take and make it\n", R.tag); continue; }
        const StarSystem& ws = W->sys;
        const int wb = W->body;
        const Body& b = ws.bodies[wb];
        const BodyGen g0 = BodyGen::make(b);
        BodyGen gm = g0; gm.made = m;
        // the place: the land 200 m inland of the world's first coast (as generated), facing where its sea lay, by day; for a kind drawn on its
        // land (a rusted ground, an overgrowth, a greened world's plants) the nearest such ground (rings out to 400 km), facing the most of it
        auto onKind = [&](const SurfaceSample& q) {
            if (R.kind == MDK_RUST) return q.material == MAT_ROCK || q.material == MAT_SAND || q.material == MAT_DUST || q.material == MAT_MUD || q.material == MAT_BASALT;
            if (R.kind == MDK_OVERGROWN) return madeGrowthMaterial(q.material);
            if (R.kind == MDK_GREENED) return q.material == MAT_GRASS || q.material == MAT_FOREST;
            if (R.kind == MDK_GREENED_DESERT) return q.material == MAT_GRASS;
            return true;
        };
        auto at = [&](double laDeg, double loDeg, double km, double a) {
            const double d = km / b.radiusKm;
            return StarSystem::bodyFromLatLon(laDeg * DEG + d * std::cos(a), loDeg * DEG + d * std::sin(a) / std::max(0.05, std::cos(laDeg * DEG)));
        };
        double siteLa = 0, siteLo = 0, yaw = 0;
        if (R.way == -3 || !coastSite(g0, b.radiusKm, siteLa, siteLo)) { siteLa = sds[0].lat / DEG; siteLo = sds[0].lon / DEG; }   // (J-15: a dry world's, its place)
        const bool drained = drainageEnabled();
        setDrainageEnabled(false);   // (the rivers left out of the search)
        if (!onKind(sampleSurface(gm, at(siteLa, siteLo, 0, 0), 64))) {
            bool found = false;
            for (double km = 0.5; km < 400 && !found; km *= 1.25)
                for (int k = 0; k < 24 && !found; k++) {
                    const Vec3 u = at(siteLa, siteLo, km, k * TAU / 24);
                    if (!onKind(sampleSurface(gm, u, 64)) || madeWetAt(sampleSurface(gm, u, 64))) continue;
                    siteLa = std::asin(clampd(u.z, -1, 1)) / DEG; siteLo = std::atan2(u.y, u.x) / DEG; found = true;
                }
            int best = -1;
            for (int k = 0; k < 16; k++) {
                int n = 0;
                for (double dm : {80.0, 160.0, 320.0, 640.0}) n += onKind(sampleSurface(gm, at(siteLa, siteLo, dm / 1000.0, k * TAU / 16), 16));
                if (n > best) { best = n; yaw = k * TAU / 16; }
            }
        } else
            for (double dm : {250.0, 500.0, 900.0}) {
                bool found = false;
                for (int k = 0; k < 32 && !found; k++) {
                    const SurfaceSample q = sampleSurface(g0, at(siteLa, siteLo, dm / 1000.0, k * TAU / 32), 16);
                    if (madeWetAt(q) && q.waterKind == 0) { yaw = k * TAU / 32; found = true; }
                }
                if (found) break;
            }
        setDrainageEnabled(drained);
        const Vec3 site = StarSystem::bodyFromLatLon(siteLa * DEG, siteLo * DEG);
        const double turn = std::max(3600.0, std::fabs(b.rotPeriod));
        double tDay = 20000, sunBest = -2;   // (the first time the sun stands 37 degrees up, else its highest)
        for (int k = 0; k < 192 && sunBest <= 0.6; k++) {
            const double tt = 20000 + turn * k / 192;
            const StarSystem::Light L = ws.lightAt(ws.bodyPos(wb, tt), tt);   // (J-15: a second sun's world lit by its own)
            const double sun = dot(normalize(ws.bodyFrame(wb, tt) * (L.fromCompanion ? L.dir : normalize(ws.star.pos - ws.bodyPos(wb, tt)))), site);
            if (sun > sunBest) { sunBest = sun; tDay = tt; }
        }
        if (R.way == -3 && sunBest < 0.3) {   // (J-15: a moon whose place never sees its sun that turn: the sunniest green ground of the world then)
            double best = -2;
            for (int j = 0; j < 24; j++)
                for (int i = 0; i < 48; i++) {
                    const double la = (j + 0.5) / 24 * 180 - 90, lo = (i + 0.5) / 48 * 360 - 180;
                    const Vec3 u = StarSystem::bodyFromLatLon(la * DEG, lo * DEG);
                    const StarSystem::Light L = ws.lightAt(ws.bodyPos(wb, 20000), 20000);
                    const double sun = dot(normalize(ws.bodyFrame(wb, 20000) * L.dir), u);
                    if (sun > best && onKind(sampleSurface(gm, u, 64))) { best = sun; siteLa = la; siteLo = lo; }
                }
            tDay = 20000; sunBest = best;
        }
        TestGame g;
        g.savePrefix = "shots/tests/kinds_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/kinds_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        Input in;
        auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
        auto press = [&](int k) { in.pressed[k] = in.down[k] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
        std::vector<uint32_t> before, after;
        auto orbit = [&](const char* tag, std::vector<uint32_t>& px) {
            g.testSetTime(tDay); g.testParkAt(ws.star, wb, true, false); g.testLandCursor(siteLa, siteLo);
            run(0.3); g.testWaitMapOf(wb); run(0.1);
            press(KEY_ESCAPE); run(0.4);
            px.assign(g.output(), g.output() + FBW * FBH);
            writePNG(fmt("shots/tests/kinds/%s_%s.png", R.tag, tag).c_str(), g.output(), FBW, FBH);
        };
        orbit("orbit_before", before);
        // its lines strains of the explorer's (the editor's records of their edits), let go long enough ago that each is at its 64th step now
        double end = 0;
        for (const Seeding& s : sds) end = std::max(end, endClock(s));
        for (size_t li = 0; li < sds.size(); li++) {
            const Seeding& s = sds[li];
            StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(edits[li]);
            g.guide.strains[s.line] = rec;
            SeedRecord r; r.body = Guide::bodyKey(ws.star.sx, ws.star.sy, ws.star.sz, wb); r.lat = s.lat; r.lon = s.lon; r.line = s.line; r.edits = rec.edits;
            r.cell = chemistryCell(rw.kind.chemistry); r.tempC = std::round(rw.kind.s.tOpt); r.light = CL_WHITE; r.energy = CE_BROTH; r.salt = 1; r.gen = GEN_VERSION;
            r.clock = g.guide.clock - end + li * 1e-3;
            g.guide.addSeeding(r);
        }
        g.testClockSkip(0);
        orbit("orbit", after);
        const MadeWorld now = g.testMadeNow(wb);
        press(KEY_I); run(0.1);
        writePNG(fmt("shots/tests/kinds/%s_sheet.png", R.tag).c_str(), g.output(), FBW, FBH);
        press(KEY_ESCAPE); run(0.1);
        g.testSetTime(tDay); g.testParkAt(ws.star, wb); g.testLandCursor(siteLa, siteLo);
        press(KEY_ENTER);
        for (int n = 0; n < 30 * 40 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
        g.testSetYaw(yaw); g.testSetPitch(-0.05);
        run(2.0);
        const bool landed = g.state == GameState::SURFACE;
        if (landed) writePNG(fmt("shots/tests/kinds/%s_ground.png", R.tag).c_str(), g.output(), FBW, FBH);
        remove(g.guidePath.c_str()); remove(g.slotPath(0).c_str());
        int changed = 0;
        for (size_t i = 0; i < before.size() && i < after.size(); i++) changed += before[i] != after[i];
        const bool shown = now.kind == R.kind && changed > 0 && landed;
        drawn += shown;
        const std::string name = madeKindName(now);
        printf("  %-10s %s at %lld %lld %lld body %d, species %d, %zu line%s: %s, ONCE %s; %s; the globe %d px changed%s\n", R.tag, KINDS[b.type].name, (long long)ws.star.sx,
               (long long)ws.star.sy, (long long)ws.star.sz, wb, rw.kind.s.index, sds.size(), sds.size() > 1 ? "s" : "", name.empty() ? "NO MADE KIND" : name.c_str(),
               KINDS[b.type].name, wordsOf(now, kindHas(b.type, KT_SEA)).c_str(), changed, landed ? "" : "; NOT LANDED");
        BodyGen gn = g0; gn.made = now;
        printf("  %-10s \"%s\" \"%s\"\n", "", upper(madeKindSentence(now)).c_str(), upper(describeBody(b, gn)).c_str());
        if (R.kind == MDK_GREENED_DESERT) {   // J-15: the carpet and its grass by the steps: the place's ground, the share of the land green, the kind
            std::string by;
            const Vec3 place = StarSystem::bodyFromLatLon(sds[0].lat, sds[0].lon);
            for (int st = 8; st <= 40; st += 4) {
                const MadeWorld ms = madeWorldOf(ws, wb, sds, (st * MADE_STEP_S + 1) / std::max(0.05, sds[0].verdict.pace));
                BodyGen gs = g0; gs.made = ms;
                const std::string kn = madeKindName(ms);
                by += fmt("%s%d %s %.0f%%%s", by.empty() ? "" : ", ", st, MATERIAL_NAMES[sampleSurface(gs, place, 64).material], 100 * ms.green, kn.empty() ? "" : (" " + kn).c_str());
            }
            printf("  %-10s by its steps (the place's ground, the land green): %s\n", "", by.c_str());
        }
    }
    printf("seed kinds: %d of %zu kinds' recipes found a world and drew it%s  (%.1f s)\n", drawn, recipes.size(), missing.empty() ? "" : (", none for " + missing).c_str(),
           nowSec() - t0);
    return drawn == (int)recipes.size() ? 0 : 1;
}
