// J-04 (R-448): the made air's modes. `seed climate`: each gas a line can give, let go on twenty worlds where the line takes, its world's mean
// warmth, pressure and traits before and after the whole world is its (ITERATION-1.5.0.md J-04's "the climate before and after")
#include "harness.h"
#include "life/seeding.h"
#include "test_game.h"
#include <algorithm>

std::vector<GeneEdit> seedGeneEdits(const MicrobeGenes& g, const std::vector<std::pair<int, int>>& wants) {
    std::vector<GeneEdit> out;
    if (!g.valid) return out;
    const GeneCode c = geneCodeOf(g.genesis);
    for (const auto& w : wants) {
        const MicrobeGene* G = nullptr;
        for (const MicrobeGene& x : g.genes) if (x.kind == w.first && !x.cut) { G = &x; break; }
        if (!G) continue;
        GeneEdit e; e.place = G->place;
        if (w.second == -2) { e.cut = true; out.push_back(e); continue; }   // J-05: the gene cut
        if (const std::vector<int>* t = worldTable(c, w.first)) {   // a table's run that reads the value, its weight's letters the heaviest
            const std::string run = runReading(c, *t, w.second, 2);
            if (run.size() != 2 || G->part.size() < 2) continue;
            e.part = run + heavier(c, G->part.substr(2), 99);
        } else e.part = heavier(c, G->part, 99);
        if (e.part != G->part) out.push_back(e);
    }
    std::sort(out.begin(), out.end(), [](const GeneEdit& a, const GeneEdit& b) { return a.place < b.place; });
    return out;
}

bool seedNativeLine(const StarSystem& sys, int body, SeedLine& wild) {
    const auto life = lifeOf(sys);
    const Biota& B = life->bodies[body];
    int sp = -1;   // its sea's first light-feeder, else its first that lives on the dead, else its first chemical-eater
    for (int way : {LW_LIGHT, LW_DEAD, LW_CHEMICAL})
        for (const Microbe& m : B.species) if (sp < 0 && m.state == SS_LIVING && m.niche == NI_LIQUID && m.way == way) sp = m.index;
    if (sp < 0) return false;
    wild = seedLineFor(sys, body, sp, {});
    return wild.valid;
}

std::vector<SeedAirCase> seedAirCases(int n) {
    static const int KINDS_TRIED[] = {PT_FELISIAN, PT_ARCHEAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR, PT_SNOWBALL, PT_EYEBALL,
                                      PT_OCEAN, PT_HYCEAN, PT_AMMONIA, PT_HYDROCARBON, PT_ACIDIC, PT_CHEMOTROPH};
    std::vector<SeedAirCase> out;
    for (int round = 0; round < 3 && (int)out.size() < n; round++)   // the first of each kind, then the second, then the third
        for (int type : KINDS_TRIED) {
            if ((int)out.size() >= n) break;
            int seen = 0;
            forTypeBodies(type, round + 1, false, [&](const StarSystem& sys, int bi) {
                if (seen++ != round || (int)out.size() >= n) return;
                SeedAirCase c; c.sys = sys; c.body = bi;
                if (!seedNativeLine(sys, bi, c.wild) || !seedPlaceTaking(sys, bi, c.wild, c.lat, c.lon)) return;
                out.push_back(c);
            });
        }
    return out;
}

// J-04: `seed climate`
int seedClimate() {
    const double t0 = nowSec();
    const std::vector<SeedAirCase> worlds = seedAirCases(20);
    printf("seed climate: each world's first sea species (a light-feeder, else a dead-eater, else a chemical-eater), its gas gene set to each gas at its heaviest\n");
    int traits[MADE_AIR_TRAITS] = {}, runaways = 0, warmer = 0, colder = 0, took = 0;
    for (size_t wi = 0; wi < worlds.size(); wi++) {
        const StarSystem& sys = worlds[wi].sys;
        const int bi = worlds[wi].body;
        const Body& b = sys.bodies[bi];
        const KindDef& K = KINDS[b.type];
        const SeedLine& wild = worlds[wi].wild;
        const double P0 = K.pressureSpan > 0 ? K.pressureLo + K.pressureSpan * hash01(b.seed) : 0;
        const Seeding probe = seedingFor(sys, bi, wild, worlds[wi].lat, worlds[wi].lon, 0);
        printf("world %2zu: %s at %lld %lld %lld body %d (R %.0f km), %s air, %.2f atm, mean %.1f C; species %d (%s, %s), on %.0f%% of it, at %.1f %.1f\n", wi + 1, K.name,
               (long long)sys.star.sx, (long long)sys.star.sy, (long long)sys.star.sz, bi, b.radiusKm,
               !bodyHasAir(b) ? "no" : ((K.shapes & KS_OXYGEN_AIR) ? "an oxygen" : "an oxygenless"), P0, probe.verdict.worldC, wild.kind.s.index, WAY_NAMES[wild.kind.s.way],
               wild.kind.world.gives() ? WORLD_GAS_NAMES[wild.kind.world.gas] : "no gas", 100 * probe.verdict.share, worlds[wi].lat / DEG, worlds[wi].lon / DEG);
        for (int gas = WG_CO2; gas < WG_COUNT; gas++) {
            const SeedLine line = seedLineFor(sys, bi, wild.kind.s.index, seedGeneEdits(wild.genes, {{GN_GAS, gas}}));
            const Seeding sd = seedingFor(sys, bi, line, worlds[wi].lat, worlds[wi].lon, 0);
            if (!sd.verdict.takes()) { printf("  %-18s %s\n", WORLD_GAS_NAMES[gas], seedVerdictWords(sd.verdict).c_str()); continue; }
            took++;
            const std::vector<Seeding> one = {sd};
            const MadeWorld m = madeWorldOf(sys, bi, one, (MADE_STEPS * MADE_STEP_S + 1) / sd.verdict.pace + 12 * MADE_STEP_S);   // the whole world, and a runaway's hour
            std::string seen;
            for (const std::string& t : madeTraitWords(m, kindHas(b.type, KT_SEA))) seen += (seen.empty() ? "" : ", ") + t;
            for (int i = 0; i < MADE_AIR_TRAITS; i++) traits[i] += (m.air >> i) & 1;
            runaways += (m.air & MA_RUNAWAY) != 0; warmer += m.warmC >= 1; colder += m.warmC <= -1;
            printf("  %-18s mean %6.1f to %6.1f C, %7.2f to %7.2f atm; %s%s\n", WORLD_GAS_NAMES[gas], sd.verdict.worldC, sd.verdict.worldC + m.warmC, P0, P0 * m.pressureK,
                   seen.empty() ? "nothing" : seen.c_str(), m.dried() ? " (its seas gone)" : "");
        }
    }
    std::string ts;
    for (int i = 0; i < MADE_AIR_TRAITS; i++) if (traits[i]) ts += fmt("%s%s %d", ts.empty() ? "" : ", ", MADE_AIR_NAMES[i], traits[i]);
    printf("seed climate: %zu worlds, %d seedings took: %d warmer, %d colder, %d ran away; %s  (%.1f s)\n", worlds.size(), took, warmer, colder, runaways,
           ts.empty() ? "no air traits" : ts.c_str(), nowSec() - t0);
    return 0;
}

namespace {
// a trait's recipe for `seed traits`: the genes set on a world's own sea species (each {kind, value}: `seedGeneEdits`), whether that species
// must glow or make spores, the time of day, the ground's look up (radians), and the frames after the whole world (a runaway's: steaming, then
// dry an hour on)
struct TraitRecipe {
    int trait;
    const char* tag;
    std::vector<std::pair<int, int>> genes;
    bool glows, spores, night;
    double pitch, rise;   // the ground's look up, and the eye raised over the shore (a fog lying on the sea seen from over it)
};
std::vector<TraitRecipe> traitRecipes() {
    return {
        {MA_THICK, "thick_air", {{GN_GAS, WG_CO2}}, false, false, false, 0.06, 0},
        {MA_RUNAWAY, "runaway", {{GN_GAS, WG_CO2}}, false, false, false, -0.12, 150},
        {MA_ORANGE_HAZE, "orange_haze", {{GN_GAS, WG_METHANE}}, false, false, false, 0.06, 0},
        {MA_SULPHUR, "sulphur_air", {{GN_GAS, WG_H2S}}, false, false, false, 0.04, 0},
        {MA_OZONE, "ozone", {{GN_GAS, WG_OXYGEN}}, false, false, false, 0.45, 0},
        {MA_THIN_OZONE, "thin_ozone", {{GN_GAS, WG_N2O}}, false, false, false, 0.45, 0},
        {MA_COLD_HAZE, "cold_haze", {{GN_GAS, WG_DMS}}, false, false, false, 0.06, 0},
        {MA_GLOW_CLOUDS, "glowing_clouds", {{GN_LIFT, -1}}, true, false, true, 0.35, 0},
        {MA_DYE_CLOUDS, "coloured_clouds", {{GN_LIFT, -1}, {GN_DYE, WD_RED}}, false, false, false, 0.35, 0},
        {MA_DYE_RAIN, "blood_rain", {{GN_LIFT, -1}, {GN_DYE, WD_RED}, {GN_FROST, -1}}, false, false, false, -0.3, 0},
        {MA_SEA_SMOKE, "sea_smoke", {{GN_GAS, WG_DMS}}, false, false, false, -0.12, 90},
        {MA_ENDLESS_RAIN, "endless_rain", {{GN_LIFT, -1}, {GN_FROST, -1}}, false, false, false, 0.1, 0},
        {MA_SPORE_CLOUDS, "spore_clouds", {{GN_LIFT, -1}}, false, true, false, 0.12, 0},
    };
}
// a world's sea species that glows (or makes spores), else its own line; with the recipe's genes set
bool traitLine(const StarSystem& sys, int body, const TraitRecipe& r, SeedLine& wild, SeedLine& line) {
    if (r.glows || r.spores) {
        const Biota& B = lifeOf(sys)->bodies[body];
        int sp = -1;
        for (const Microbe& m : B.species)
            if (sp < 0 && m.state == SS_LIVING && m.niche == NI_LIQUID && m.way != LW_NONE && m.way != LW_PREDATOR && m.way != LW_GRAZER && (r.glows ? m.glows : m.sporeYears > 0)) sp = m.index;
        if (sp < 0) return false;
        wild = seedLineFor(sys, body, sp, {});
    } else if (!seedNativeLine(sys, body, wild)) return false;
    if (!wild.valid) return false;
    line = seedLineFor(sys, body, wild.kind.s.index, seedGeneEdits(wild.genes, r.genes));
    return line.valid;
}
}   // namespace

// J-04: `seed traits`
int seedTraits() {
    const double t0 = nowSec();
    const std::vector<SeedAirCase> worlds = seedAirCases(36);
    int drawn = 0, found = 0;
    std::string missing;
    for (const TraitRecipe& R : traitRecipes()) {
        // the first world where its line takes in the sea off a coast and makes the trait once it has the whole world, its coast's site in the
        // day (in the night for a night's trait) at a time under a clear sky, or a cloudy one for a cloud's trait, and its shore's bearing
        const bool cloudy = R.trait & (MA_GLOW_CLOUDS | MA_DYE_CLOUDS | MA_DYE_RAIN | MA_ENDLESS_RAIN | MA_SPORE_CLOUDS);
        const SeedAirCase* W = nullptr;
        SeedLine wild, line;
        double siteLa = 0, siteLo = 0, seaLat = 0, seaLon = 0, pace = 1, T0 = 20000, yaw = 0;
        for (const SeedAirCase& c : worlds) {
            if (!traitLine(c.sys, c.body, R, wild, line)) continue;
            const Body& b = c.sys.bodies[c.body];
            const BodyGen g = BodyGen::make(b);
            double la = 0, lo = 0;
            if (!coastSite(g, b.radiusKm, la, lo)) continue;
            bool ok = false;
            MadeWorld m;
            for (int k = 0; k < 16 && !ok; k++) {   // the sea within a few kilometres of the coast's site, where the line takes
                const double a = k * TAU / 16, d = 4.0 / b.radiusKm;
                const double sLat = la * DEG + d * std::cos(a), sLon = lo * DEG + d * std::sin(a) / std::max(0.05, std::cos(la * DEG));
                const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(sLat, sLon), SEED_PLACE_M);
                if (!madeWetAt(s) || s.waterKind != 0) continue;
                const Seeding sd = seedingFor(c.sys, c.body, line, sLat, sLon, 0);
                if (!sd.verdict.takes()) continue;
                const std::vector<Seeding> one = {sd};
                m = madeWorldOf(c.sys, c.body, one, (MADE_STEPS * MADE_STEP_S + 1) / sd.verdict.pace + 14 * MADE_STEP_S);
                if (!(m.air & R.trait)) continue;
                ok = true; siteLa = la; siteLo = lo; seaLat = sLat; seaLon = sLon; pace = sd.verdict.pace;
            }
            if (!ok) continue;
            // the time: the site's sun up (or down for a night's trait), the sky over it as the ground reads its cloud
            BodyGen gm = g; gm.made = m;
            const Vec3 at = StarSystem::bodyFromLatLon(siteLa * DEG, siteLo * DEG);
            const double turn = std::max(3600.0, std::fabs(b.rotPeriod));
            double best = -1e9;
            for (int k = 0; k < 192; k++) {
                const double tt = 20000 + turn * k / 192;
                const double sun = dot(normalize(c.sys.bodyFrame(c.body, tt) * normalize(c.sys.star.pos - c.sys.bodyPos(c.body, tt))), at);
                if (R.night ? sun > -0.25 : sun < 0.3) continue;
                const double drift = tt * TAU / (std::fabs(b.rotPeriod) * 3.7) + 1.0;
                const double cl = sampleCloudPattern(gm, siteLo * DEG + drift, siteLa * DEG);
                const double score = (cloudy ? cl : -cl) + 0.1 * (R.night ? -sun : sun);
                if (score > best) { best = score; T0 = tt; }
            }
            if (best < -1e8 || (cloudy && best < 0.5)) continue;   // (no night there, or no day: a locked world's; no cloud over it for a cloud's trait)
            // the shore: the bearing of the nearest sea from the site
            bool shore = false;
            for (double dm : {250.0, 500.0, 900.0})
                for (int k = 0; k < 32 && !shore; k++) {
                    const double a = k * TAU / 32, d = dm / 1000.0 / b.radiusKm;
                    const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(siteLa * DEG + d * std::cos(a), siteLo * DEG + d * std::sin(a) / std::max(0.05, std::cos(siteLa * DEG))), 16);
                    if (madeWetAt(s) && s.waterKind == 0) { yaw = a; shore = true; }
                }
            if (!shore) continue;
            W = &c;
            break;
        }
        if (!W) { missing += fmt("%s%s", missing.empty() ? "" : ", ", R.tag); printf("  %-16s no world of the kinds tried where its line takes and makes it\n", R.tag); continue; }
        found++;
        const StarSystem& ws = W->sys;
        const int wb = W->body;
        const Body& b = ws.bodies[wb];
        TestGame g;
        g.savePrefix = "shots/tests/seed_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/seed_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        Input in;
        auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
        auto press = [&](int k) { in.pressed[k] = in.down[k] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
        std::vector<uint32_t> px[3][2];
        double tempC[3] = {}, atm[3] = {}, fogM[3] = {}, fogSea[3] = {}, rain[3] = {}, cloudC[3] = {};
        auto views = [&](const char* tag, int pass) {
            g.testSetTime(T0); g.testParkAt(ws.star, wb, true, R.night); g.testLandCursor(siteLa, siteLo);
            run(0.3); press(KEY_ESCAPE); run(0.4);
            px[pass][0].assign(g.output(), g.output() + FBW * FBH);
            writePNG(fmt("shots/tests/made_%s_orbit_%s.png", R.tag, tag).c_str(), g.output(), FBW, FBH);
            g.testSetTime(T0); g.testParkAt(ws.star, wb); g.testLandCursor(siteLa, siteLo);
            press(KEY_ENTER);
            for (int n = 0; n < 30 * 40 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
            g.testSetYaw(yaw); g.testSetPitch(R.pitch);
            if (R.rise > 0) g.testFreeCamUp(R.rise, yaw, R.pitch);
            run(2.0);
            px[pass][1].assign(g.output(), g.output() + FBW * FBH);
            const SurfaceEnvironment& E = g.testSurface().env;
            tempC[pass] = E.temperatureC; atm[pass] = E.pressureAtm; fogM[pass] = E.fogDistance; fogSea[pass] = E.fogSea > 0 ? E.fogSeaTop : -1; rain[pass] = std::max(E.rain, E.snow); cloudC[pass] = E.cloudCover;
            writePNG(fmt("shots/tests/made_%s_ground_%s.png", R.tag, tag).c_str(), g.output(), FBW, FBH);
        };
        views("before", 0);
        {   // the line a strain of the explorer's (the editor's record of its edits), let go in the sea so that it has the whole world now
            const std::string lineKey = line.key + "/S1";
            const std::vector<GeneEdit> edits = seedGeneEdits(wild.genes, R.genes);
            StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(edits);
            g.guide.strains[lineKey] = rec;
            SeedRecord r; r.body = Guide::bodyKey(ws.star.sx, ws.star.sy, ws.star.sz, wb); r.lat = seaLat; r.lon = seaLon; r.line = lineKey; r.edits = rec.edits;
            r.cell = chemistryCell(wild.kind.chemistry); r.tempC = std::round(wild.kind.s.tOpt); r.light = CL_WHITE; r.energy = CE_BROTH; r.salt = 1; r.gen = GEN_VERSION;
            r.clock = g.guide.clock - (MADE_STEPS * MADE_STEP_S + 1) / pace - (R.trait == MA_RUNAWAY ? 2 * MADE_STEP_S : 0.0);
            g.guide.addSeeding(r);
            g.testClockSkip(0);
        }
        const MadeWorld m = g.testMadeNow(wb);
        views("after", 1);
        MadeWorld md;
        if (R.trait == MA_RUNAWAY) {   // an hour on: its seas gone to their old beds
            g.testClockSkip(12 * MADE_STEP_S);
            md = g.testMadeNow(wb);
            views("dry", 2);
        }
        int changed[2] = {};
        for (int v = 0; v < 2; v++) for (size_t i = 0; i < px[0][v].size() && i < px[1][v].size(); i++) changed[v] += px[0][v][i] != px[1][v][i];
        drawn += (m.air & R.trait) && changed[0] > 0 && changed[1] > 0;
        std::string shows;
        for (const std::string& t : madeTraitWords(m, kindHas(b.type, KT_SEA))) shows += (shows.empty() ? "" : ", ") + t;
        auto fogText = [&](int p) { return fmt("fog %.0f m%s, cloud %.2f, rain %.2f", fogM[p], fogSea[p] > -1 ? fmt(" under a fog to %.0f m", fogSea[p]).c_str() : "", cloudC[p], rain[p]); };
        printf("  %-16s %s at %lld %lld %lld body %d, species %d %s: %s; AIR %s; TEMP %.0f to %.0f C, ATM %.2f to %.2f; %s, then %s; the globe %d px changed, the ground %d\n",
               R.tag, KINDS[b.type].name, (long long)ws.star.sx, (long long)ws.star.sy, (long long)ws.star.sz, wb, wild.kind.s.index, geneEditsText(seedGeneEdits(wild.genes, R.genes)).c_str(),
               shows.empty() ? "NOTHING" : shows.c_str(), madeAirWords(m).empty() ? "as it was" : madeAirWords(m).c_str(), tempC[0], tempC[1], atm[0], atm[1], fogText(0).c_str(),
               fogText(1).c_str(), changed[0], changed[1]);
        if (R.trait == MA_RUNAWAY)
            printf("  %-16s an hour on: %s; TEMP %.0f C, ATM %.2f, %s%s\n", "", madeAirWords(md).c_str(), tempC[2], atm[2], fogText(2).c_str(), md.dried() ? ", its seas gone" : ", ITS SEAS STILL THERE");
    }
    printf("seed traits: %d of %d traits found a world, %d drawn from orbit and on the ground%s  (%.1f s)\n", found, (int)traitRecipes().size(), drawn,
           missing.empty() ? "" : (", none for " + missing).c_str(), nowSec() - t0);
    return drawn == (int)traitRecipes().size() ? 0 : 1;
}
