// J-05 (R-448): the made waters', ground's and plants' mode. `seed looks`: each trait a line can make of the waters, the ground and the plants
// it lives in, let go on a world where it makes it, drawn from orbit and on the ground before and after, as `seed traits` draws the air's
// (ITERATION-1.5.0.md J-05's "frames of each trait": `shots/tests/look_<trait>_<map|orbit|ground>_<before|after>.png`)
#include "harness.h"
#include "life/seeding.h"
#include "test_game.h"
#include <algorithm>

namespace {
// whose line a recipe sets its genes on (its world's own of the sea, a glowing one of the sea, one of its snow or ice) and where the ground's
// eye stands (facing the sea from its coast, by a river or a lake, on the land, the snow or the plants the look lies on)
enum LookLine { LL_SEA = 0, LL_GLOW, LL_SNOW };
enum LookSite { LS_SEA = 0, LS_FRESH, LS_LAND, LS_SNOW, LS_FOREST };
// a trait's recipe: its set (0 the waters' `MadeWorld::seas`, 1 the ground's `grounds`, 2 the plants' `flora`) and bit, its file's tag, the
// genes set (`seedGeneEdits`: -1 the heaviest weight, -2 the gene cut; `mid` a rot of wood's weight between a blight's and a rot's), its line
// and site; at night, on a world with reefs, two steps into a runaway; a look over the whole world (no share to find it by); the ground's look up
struct LookRecipe {
    int set, trait;
    const char* tag;
    std::vector<std::pair<int, int>> genes;
    int line = LL_SEA, site = LS_SEA;
    bool night = false, reefs = false, runaway = false, mid = false, wide = false, bare = false;
    double pitch = -0.1;
};
std::vector<LookRecipe> lookRecipes() {
    std::vector<LookRecipe> r;
    auto add = [&](int set, int trait, const char* tag, std::vector<std::pair<int, int>> genes, int line, int site) {
        LookRecipe R; R.set = set; R.trait = trait; R.tag = tag; R.genes = genes; R.line = line; R.site = site; r.push_back(R); return &r.back();
    };
    add(0, MS_PURPLE, "purple_seas", {{GN_GAS, WG_H2S}}, LL_SEA, LS_SEA)->wide = true;
    add(0, MS_BLACK, "black_seas", {{GN_DYE, WD_BLACK}}, LL_SEA, LS_SEA);
    add(0, MS_MILK, "milk_seas", {{GN_SHELL, SHL_CHALK}}, LL_SEA, LS_SEA);
    add(0, MS_FOAM, "foam_seas", {{GN_LIFT, -1}, {GN_SLIME, -1}}, LL_SEA, LS_SEA)->pitch = -0.22;
    add(0, MS_JELLY, "jelly_seas", {{GN_STOP, -2}, {GN_SLIME, -1}}, LL_SEA, LS_SEA);
    add(0, MS_SOUP, "soup_seas", {{GN_STOP, -2}}, LL_SEA, LS_SEA);
    { LookRecipe* R = add(0, MS_SOUR, "sour_seas", {{GN_GAS, WG_CO2}}, LL_SEA, LS_SEA); R->reefs = true; R->wide = true; R->pitch = -0.2; }
    add(0, MS_RUST_RIVERS, "rust_rivers", {{GN_SHELL, SHL_RUST}}, LL_SEA, LS_FRESH)->pitch = -0.18;
    { LookRecipe* R = add(0, MS_BOILING, "boiling_seas", {{GN_GAS, WG_CO2}}, LL_SEA, LS_SEA); R->runaway = true; R->wide = true; R->pitch = -0.25; }
    add(1, MGT_COLOURED_SNOW, "coloured_snow", {{GN_DYE, WD_RED}}, LL_SNOW, LS_SNOW);
    add(1, MGT_DARK_ICE, "dark_ice", {{GN_DYE, WD_BLACK}}, LL_SNOW, LS_SNOW);
    add(1, MGT_CRUSTS, "crusts", {{GN_DRY, -1}, {GN_BIND, -1}}, LL_SEA, LS_LAND);
    add(1, MGT_MATS, "mats", {{GN_SLIME, -1}}, LL_SEA, LS_SEA)->pitch = -0.2;
    add(1, MGT_STROMATOLITES, "stromatolites", {{GN_SLIME, -1}, {GN_SHELL, SHL_CHALK}}, LL_SEA, LS_SEA)->pitch = -0.2;
    { LookRecipe* R = add(1, MGT_RUST, "rust_ground", {{GN_GAS, WG_OXYGEN}}, LL_SEA, LS_LAND); R->wide = true; R->bare = true; }
    add(1, MGT_CHALK, "chalk_ground", {{GN_DRY, -1}, {GN_SHELL, SHL_CHALK}}, LL_SEA, LS_LAND);
    add(1, MGT_GLASS, "glass_sands", {{GN_DRY, -1}, {GN_SHELL, SHL_GLASS}}, LL_SEA, LS_LAND)->pitch = -0.25;
    add(1, MGT_SULPHUR, "sulphur_crusts", {{GN_DRY, -1}, {GN_SHELL, SHL_SULPHUR}}, LL_SEA, LS_LAND);
    add(1, MGT_PITTED, "pitted_stone", {{GN_ROT, ROT_STONE}}, LL_SEA, LS_LAND)->pitch = -0.3;
    add(1, MGT_SLIME, "slime", {{GN_DRY, -1}, {GN_SLIME, -1}}, LL_SEA, LS_LAND)->pitch = -0.25;
    add(2, MP_BLIGHT, "blight", {{GN_ROT, ROT_WOOD}}, LL_SEA, LS_FOREST)->mid = true;
    add(2, MP_ROT, "rot", {{GN_ROT, ROT_WOOD}}, LL_SEA, LS_FOREST);
    // (the rest with a wood rotter's own gene cut: its blight would hide their looks)
    add(2, MP_FROST_BURN, "frost_burn", {{GN_HOST, HO_PLANTS}, {GN_FROST, -1}, {GN_ROT, -2}}, LL_SEA, LS_FOREST);
    add(2, MP_GALLS, "galls", {{GN_HOST, HO_PLANTS}, {GN_CARRIER, -1}, {GN_ROT, -2}}, LL_SEA, LS_FOREST)->pitch = 0.05;
    add(2, MP_NEW_LEAVES, "new_leaves", {{GN_HOST, HO_PLANTS}, {GN_DYE, WD_RED}, {GN_ROT, -2}}, LL_SEA, LS_FOREST);
    add(2, MP_GLOWING, "glowing_forests", {{GN_HOST, HO_PLANTS}, {GN_ROT, -2}}, LL_GLOW, LS_FOREST)->night = true;
    add(2, MP_LUSH, "lush", {{GN_LIFT, -1}, {GN_FROST, -1}, {GN_ROT, -2}}, LL_SEA, LS_FOREST)->wide = true;
    add(2, MP_BLEACHED, "bleached", {{GN_GAS, WG_N2O}, {GN_ROT, -2}}, LL_SEA, LS_FOREST)->wide = true;
    return r;
}
int bitsOf(const MadeWorld& m, int set) { return set == 0 ? m.seas : (set == 1 ? m.grounds : m.flora); }

// a world for the recipes: its system, body, the line's place (a sea's, or a snow's for a snow line) and its wild line
struct LookCase { StarSystem sys; int body = -1; double lat = 0, lon = 0; SeedLine wild; bool snow = false; };

// the line's edits: the recipe's genes; a mid rot of wood stepped heavier from the wild letters till it rots between a blight and a rot
std::vector<GeneEdit> lookEdits(const StarSystem& sys, int body, const SeedLine& wild, const LookRecipe& R) {
    if (!R.mid) return seedGeneEdits(wild.genes, R.genes);
    const GeneCode c = geneCodeOf(wild.genes.genesis);
    const MicrobeGene* G = nullptr;
    for (const MicrobeGene& x : wild.genes.genes) if (x.kind == GN_ROT && !x.cut) { G = &x; break; }
    const std::vector<int>* t = worldTable(c, GN_ROT);
    if (!G || !t || G->part.size() < 2) return seedGeneEdits(wild.genes, R.genes);
    const std::string run = runReading(c, *t, ROT_WOOD, 2);
    for (int steps = 0; steps < 60 && run.size() == 2; steps++) {
        GeneEdit e; e.place = G->place; e.part = run + (steps ? heavier(c, G->part.substr(2), steps) : G->part.substr(2));
        const SeedLine l = seedLineFor(sys, body, wild.kind.s.index, {e});
        if (l.valid && l.kind.world.rots() && l.kind.world.rot == ROT_WOOD && l.kind.world.rotRate >= 0.4 && l.kind.world.rotRate < 0.6) return {e};
    }
    return seedGeneEdits(wild.genes, R.genes);
}

// the recipe's line on a world: its sea's own (`seedNativeLine`), a glowing sea species, or its snow's or ice's, with the recipe's genes set
bool lookLine(const LookCase& c, const LookRecipe& R, SeedLine& wild, SeedLine& line) {
    if (R.line == LL_SEA && !c.snow) wild = c.wild;
    else if (R.line == LL_SNOW && c.snow) wild = c.wild;
    else if (R.line == LL_GLOW && !c.snow) {
        const Biota& B = lifeOf(c.sys)->bodies[c.body];
        int sp = -1;
        for (const Microbe& m : B.species)
            if (sp < 0 && m.state == SS_LIVING && m.niche == NI_LIQUID && m.glows && m.way != LW_NONE && m.way != LW_PREDATOR && m.way != LW_GRAZER) sp = m.index;
        if (sp < 0) return false;
        wild = seedLineFor(c.sys, c.body, sp, {});
    } else return false;
    if (!wild.valid) return false;
    line = seedLineFor(c.sys, c.body, wild.kind.s.index, lookEdits(c.sys, c.body, wild, R));
    line.key += "/S1";   // (the key the game reads it by, a strain of the explorer's: the drift in the wild hashes it)
    return line.valid;
}

// the worlds: `seed climate`'s (the living and wet kinds, each with its own sea line), those with reefs, and the cold ones with a snow or ice
// species and a snow place where it takes
std::vector<LookCase> lookCases() {
    std::vector<LookCase> out;
    for (const SeedAirCase& a : seedAirCases(36)) { LookCase c; c.sys = a.sys; c.body = a.body; c.lat = a.lat; c.lon = a.lon; c.wild = a.wild; out.push_back(c); }
    int reefs = 0;
    for (int type : {PT_FELISIAN, PT_SUPERHABITABLE, PT_HOTHOUSE, PT_OCEAN})
        forTypeBodies(type, 60, false, [&](const StarSystem& sys, int bi) {
            if (reefs >= 3 || !BodyGen::make(sys.bodies[bi]).hasTrait(TR_REEFS)) return;
            LookCase c; c.sys = sys; c.body = bi;
            if (!seedNativeLine(sys, bi, c.wild) || !seedPlaceTaking(sys, bi, c.wild, c.lat, c.lon)) return;
            out.push_back(c); reefs++;
        });
    int snows = 0;
    for (int type : {PT_TUNDRA, PT_SNOWBALL, PT_TERMINATOR, PT_EYEBALL, PT_STEPPE, PT_FELISIAN})
        forTypeBodies(type, 4, false, [&](const StarSystem& sys, int bi) {
            if (snows >= 8) return;
            const Biota& B = lifeOf(sys)->bodies[bi];
            int sp = -1;
            for (const Microbe& m : B.species) if (sp < 0 && m.state == SS_LIVING && (m.niche == NI_SNOW || m.niche == NI_ICE)) sp = m.index;
            if (sp < 0) return;
            LookCase c; c.sys = sys; c.body = bi; c.snow = true;
            c.wild = seedLineFor(sys, bi, sp, {});
            if (!c.wild.valid) return;
            const BodyGen g = BodyGen::make(sys.bodies[bi]);
            bool placed = false;
            for (int i = 0; i < 600 && !placed; i++) {   // its snow, a point of the sphere at a time (Fibonacci), where it takes
                const double z = 1 - (2 * i + 1) / 600.0, la = std::asin(z), lo = std::fmod(i * 2.399963229728653, TAU) - PI;
                const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), SEED_PLACE_M);
                if (s.material != MAT_SNOW && s.material != MAT_ICE) continue;
                if (!seedVerdictOf(sys, bi, c.wild, la, lo).takes()) continue;
                c.lat = la; c.lon = lo; placed = true;
            }
            if (!placed) return;
            out.push_back(c); snows++;
        });
    return out;
}

// whether a sample is of the ground a site looks for: the land's (not water, snow, ice or plants; `bare`: rock, sand, dust or mud), the snow's
// and ice's, the plants'
bool siteGround(const BodyGen& g, int site, const SurfaceSample& s, bool bare = false) {
    if (madeWetAt(s) || s.material == MAT_LAVA) return false;
    const int cls = madeClassOf(g, s.material, false), m = s.material;
    if (bare && !(m == MAT_ROCK || m == MAT_BASALT || m == MAT_SAND || m == MAT_DUST || m == MAT_MUD)) return false;
    return site == LS_SNOW ? cls == MC_SNOW : (site == LS_FOREST ? cls == MC_PLANTS && s.material == MAT_FOREST : cls == MC_LAND);
}
}   // namespace

// J-05: `seed looks`
int seedLooks() {
    const double t0 = nowSec();
    const std::vector<LookCase> worlds = lookCases();
    int drawn = 0, found = 0;
    std::string missing;
    for (const LookRecipe& R : lookRecipes()) {
        const LookCase* W = nullptr;
        SeedLine wild, line;
        double siteLa = 0, siteLo = 0, pace = 1, T0 = 20000, yaw = 0, placeLa = 0, placeLo = 0;
        MadeWorld mAt;
        for (const LookCase& c : worlds) {
            if (!lookLine(c, R, wild, line)) continue;
            const Body& b = c.sys.bodies[c.body];
            const BodyGen g = BodyGen::make(b);
            if (R.reefs && !g.hasTrait(TR_REEFS)) continue;
            // the line let go at its world's place (its sea's or its snow's), else at a few round it, where it takes and the wild leaves it as it
            // went down (else where it takes at all); its whole world two steps on
            Seeding sd;
            bool took = false;
            for (int pass = 0; pass < 2 && !took; pass++)
                for (int k = -1; k < 16 && !took; k++) {
                    const double a = k * TAU / 16, d = k < 0 ? 0.0 : 40.0 / b.radiusKm;
                    const double la = c.lat + d * std::cos(a), lo = c.lon + d * std::sin(a) / std::max(0.05, std::cos(c.lat));
                    sd = seedingFor(c.sys, c.body, line, la, lo, 0);
                    took = sd.verdict.takes() && (pass == 1 || sd.verdict.drift.empty());
                    if (took) { placeLa = la; placeLo = lo; }
                }
            if (!took) continue;
            const std::vector<Seeding> one = {sd};
            mAt = madeWorldOf(c.sys, c.body, one, (MADE_STEPS * MADE_STEP_S + 1) / sd.verdict.pace + 2 * MADE_STEP_S);
            if (!(bitsOf(mAt, R.set) & R.trait)) continue;
            BodyGen gm = g; gm.made = mAt;
            pace = sd.verdict.pace;
            // the site: a coast's (facing its sea), a river's or a lake's bank where the look lies on it, or the land, snow or forest it lies on
            bool sited = false;
            const bool drained = drainageEnabled();
            if (R.site == LS_SEA || R.site == LS_FRESH) {
                if (R.site == LS_SEA && !R.reefs) {   // the first coast's site (`coastSite`), facing its sea where the look lies on it
                    double cla = 0, clo = 0;
                    if (!coastSite(g, b.radiusKm, cla, clo)) continue;
                    for (double dm : {250.0, 500.0, 900.0})
                        for (int k = 0; k < 32 && !sited; k++) {
                            const double a = k * TAU / 32, d = dm / 1000.0 / b.radiusKm;
                            const SurfaceSample s = sampleSurface(gm, StarSystem::bodyFromLatLon(cla * DEG + d * std::cos(a), clo * DEG + d * std::sin(a) / std::max(0.05, std::cos(cla * DEG))), 16);
                            if (!madeWetAt(s) || s.waterKind != 0) continue;
                            if (!R.wide && R.set == 0 && s.madeTint < 0.5) continue;   // (its waters' look on the sea in view)
                            yaw = a; sited = true; siteLa = cla; siteLo = clo;
                        }
                } else if (R.site == LS_SEA) {   // a reef's lagoon, then the nearest land a metre and a half over the sea, facing the lagoon
                    setDrainageEnabled(false);
                    for (int i = 0; i < 2400 && !sited; i++) {
                        const double z = 1 - (2 * i + 1) / 2400.0, la = std::asin(z), lo = std::fmod(i * 2.399963229728653, TAU) - PI;
                        if (std::fabs(la) > 55 * DEG) continue;
                        const SurfaceSample s = sampleSurface(gm, StarSystem::bodyFromLatLon(la, lo), SEED_PLACE_M);
                        if (!madeWetAt(s) || s.waterKind != 0 || (R.reefs && s.lifeTint < 0.5) || (!R.wide && R.set == 0 && s.madeTint < 0.5)) continue;
                        for (int k = 0; k < 16 && !sited; k++) {
                            const double a = k * TAU / 16;
                            for (double km = 1; km <= 30 && !sited; km += 1) {
                                const double d = km / b.radiusKm;
                                const double la2 = la + d * std::cos(a), lo2 = lo + d * std::sin(a) / std::max(0.05, std::cos(la));
                                const SurfaceSample q = sampleSurface(gm, StarSystem::bodyFromLatLon(la2, lo2), 16);
                                if (madeWetAt(q) || q.height < 1.5) continue;
                                siteLa = la2 / DEG; siteLo = lo2 / DEG; yaw = a + PI; sited = true;
                            }
                        }
                    }
                    setDrainageEnabled(drained);
                } else {   // a river or a lake where the look lies: in a window of a degree round the world's first land sites (the `water` mode's, every
                    // 0.01 degrees), its water then a bank 30 to 80 m from it, facing it
                    setDrainageEnabled(true);
                    std::vector<std::pair<double, double>> sites; landSitesOf(c.sys, c.body, 0x3A, 3, sites);
                    for (const auto& st : sites) {
                        if (sited) break;
                        const double cla = st.first, cln = st.second;
                        for (int gz = -50; gz <= 50 && !sited; gz++)
                            for (int gx = -50; gx <= 50 && !sited; gx++) {
                                const double wla = cla + gz * 0.01 * DEG, wlo = cln + gx * 0.01 * DEG / std::max(0.2, std::cos(cla));
                                const SurfaceSample w = sampleSurface(gm, StarSystem::bodyFromLatLon(wla, wlo), 16);
                                if (!madeWetAt(w) || w.waterKind == 0 || w.madeTint < 0.5) continue;
                                for (double dm : {30.0, 55.0, 80.0})
                                    for (int j = 0; j < 16 && !sited; j++) {   // the bank: dry ground beside it
                                        const double b2 = j * TAU / 16, d2 = dm / 1000.0 / b.radiusKm;
                                        const double sla = wla + d2 * std::cos(b2), slo = wlo + d2 * std::sin(b2) / std::max(0.05, std::cos(wla));
                                        if (madeWetAt(sampleSurface(gm, StarSystem::bodyFromLatLon(sla, slo), 16))) continue;
                                        siteLa = sla / DEG; siteLo = slo / DEG; yaw = b2 + PI; sited = true;
                                    }
                            }
                    }
                    setDrainageEnabled(drained);
                }
            } else {   // the sphere's points where its look lies (or, for a whole world's, its ground), then a point within 600 m of one, facing more of it
                setDrainageEnabled(false);
                for (int i = 0; i < 1500 && !sited; i++) {
                    const double z = 1 - (2 * i + 1) / 1500.0, la = std::asin(z), lo = std::fmod(i * 2.399963229728653, TAU) - PI;
                    if (std::fabs(la) > 70 * DEG) continue;
                    const SurfaceSample s = sampleSurface(gm, StarSystem::bodyFromLatLon(la, lo), 20000);
                    if (!siteGround(gm, R.site, s) || (!R.wide && s.madeTint < 0.5)) continue;
                    auto looks = [&](const SurfaceSample& q) { return siteGround(gm, R.site, q, R.bare) && (R.wide || q.madeTint >= 0.5); };
                    setDrainageEnabled(true);   // (the fine ground with its rivers and lakes, as the landing builds it)
                    drainagePrefetch(gm, StarSystem::bodyFromLatLon(la, lo), 900, false);
                    for (double dm : {0.0, 150.0, 400.0})
                        for (int k = 0; k < 8 && !sited; k++) {
                            const double a = k * TAU / 8, d = dm / 1000.0 / b.radiusKm;
                            const double pla = la + d * std::cos(a), plo = lo + d * std::sin(a) / std::max(0.05, std::cos(la));
                            if (!looks(sampleSurface(gm, StarSystem::bodyFromLatLon(pla, plo), 16))) continue;
                            for (int j = 0; j < 8 && !sited; j++) {   // the bearing with it at three of 30, 60, 120 and 200 m on
                                const double b2 = j * TAU / 8;
                                int on = 0;
                                for (double dd : {30.0, 60.0, 120.0, 200.0}) {
                                    const double d2 = dd / 1000.0 / b.radiusKm;
                                    on += looks(sampleSurface(gm, StarSystem::bodyFromLatLon(pla + d2 * std::cos(b2), plo + d2 * std::sin(b2) / std::max(0.05, std::cos(pla))), 16));
                                }
                                if (on < 3) continue;
                                siteLa = pla / DEG; siteLo = plo / DEG; yaw = b2; sited = true;
                            }
                        }
                    setDrainageEnabled(false);
                }
                setDrainageEnabled(drained);
            }
            if (!sited) continue;
            // the time: the site's sun up (down for a night's trait) under the clearest sky
            const Vec3 at = StarSystem::bodyFromLatLon(siteLa * DEG, siteLo * DEG);
            const double turn = std::max(3600.0, std::fabs(b.rotPeriod));
            double best = -1e9;
            for (int k = 0; k < 192; k++) {
                const double tt = 20000 + turn * k / 192;
                const double sun = dot(normalize(c.sys.bodyFrame(c.body, tt) * normalize(c.sys.star.pos - c.sys.bodyPos(c.body, tt))), at);
                if (R.night ? sun > -0.25 : sun < 0.3) continue;
                const double drift = tt * TAU / (std::fabs(b.rotPeriod) * 3.7) + 1.0;
                const double score = -sampleCloudPattern(gm, siteLo * DEG + drift, siteLa * DEG) + 0.1 * (R.night ? -sun : sun);
                if (score > best) { best = score; T0 = tt; }
            }
            if (best < -1e8) continue;
            W = &c;
            break;
        }
        if (!W) { missing += fmt("%s%s", missing.empty() ? "" : ", ", R.tag); printf("  %-16s no world of those tried where its line takes and makes it\n", R.tag); continue; }
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
        std::vector<uint32_t> px[2][2];
        std::string ground[2];
        auto views = [&](const char* tag, int pass) {
            g.testSetTime(T0); g.testParkAt(ws.star, wb, true, R.night); g.testLandCursor(siteLa, siteLo);
            run(0.3); g.testWaitMapOf(wb); run(0.1);   // (the map of the layer the game took in its first frame)
            writePNG(fmt("shots/tests/look_%s_map_%s.png", R.tag, tag).c_str(), g.output(), FBW, FBH);   // the landing map
            press(KEY_ESCAPE); run(0.4);
            px[pass][0].assign(g.output(), g.output() + FBW * FBH);
            writePNG(fmt("shots/tests/look_%s_orbit_%s.png", R.tag, tag).c_str(), g.output(), FBW, FBH);
            g.testSetTime(T0); g.testParkAt(ws.star, wb); g.testLandCursor(siteLa, siteLo);
            press(KEY_ENTER);
            for (int n = 0; n < 30 * 40 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
            g.testSetYaw(yaw); g.testSetPitch(R.pitch);
            if (R.site == LS_SEA || R.site == LS_FRESH) {   // the drawn ground's nearest dry spot beside its water (the tide's), facing the water
                run(0.5);
                SurfaceSite& site = const_cast<SurfaceView&>(g.testSurface()).site;
                const SurfaceView& S = g.testSurface();
                auto wet = [&](double x, double z) { const double w = site.waterNowAt(x, z); return w > -1e8 && w > site.groundHeight(x, z) + 0.05; };
                const double px0 = S.player.x, pz0 = S.player.z;
                bool found = false;
                for (int r = 0; r <= 40 && !found; r++)
                    for (int i = -r; i <= r && !found; i++)
                        for (int j = -r; j <= r && !found; j++) {
                            if (std::max(std::abs(i), std::abs(j)) != r) continue;   // (the ring's cells)
                            const double x = px0 + i * 16.0, z = pz0 + j * 16.0;
                            if (wet(x, z)) continue;
                            for (int k = 0; k < 16 && !found; k++) {
                                const double a = k * TAU / 16, dx = std::sin(a), dz = std::cos(a);
                                if (!wet(x + 10 * dx, z + 10 * dz) || !wet(x + 30 * dx, z + 30 * dz)) continue;
                                g.testWalkTo(x - 3 * dx, z - 3 * dz); g.testSetYaw(a); found = true;
                            }
                        }
            }
            run(2.0);
            px[pass][1].assign(g.output(), g.output() + FBW * FBH);
            const SurfaceView& S = g.testSurface();
            const TerrainVertex& tv = const_cast<TerrainCache&>(S.site.lod0).at((int)std::floor(S.player.x / 16), (int)std::floor(S.player.z / 16));
            ground[pass] = fmt("%s%s", MATERIAL_NAMES[clampi(tv.material, 0, MAT_COUNT - 1)], tv.mtint > 127 ? " (made)" : "");
            writePNG(fmt("shots/tests/look_%s_ground_%s.png", R.tag, tag).c_str(), g.output(), FBW, FBH);
        };
        views("before", 0);
        {   // the line a strain of the explorer's (the editor's record of its edits), let go at its place so that it has the whole world now
            const std::string lineKey = line.key;
            const std::vector<GeneEdit> edits = lookEdits(ws, wb, wild, R);
            StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(edits);
            g.guide.strains[lineKey] = rec;
            SeedRecord r; r.body = Guide::bodyKey(ws.star.sx, ws.star.sy, ws.star.sz, wb); r.lat = placeLa; r.lon = placeLo; r.line = lineKey; r.edits = rec.edits;
            r.cell = chemistryCell(wild.kind.chemistry); r.tempC = std::round(wild.kind.s.tOpt); r.light = CL_WHITE; r.energy = CE_BROTH; r.salt = W->snow ? 0 : 1; r.gen = GEN_VERSION;
            r.clock = g.guide.clock - (MADE_STEPS * MADE_STEP_S + 1) / pace - 2 * MADE_STEP_S;
            g.guide.addSeeding(r);
            g.testClockSkip(0);
        }
        const MadeWorld m = g.testMadeNow(wb);
        views("after", 1);
        int changed[2] = {};
        for (int v = 0; v < 2; v++) for (size_t i = 0; i < px[0][v].size() && i < px[1][v].size(); i++) changed[v] += px[0][v][i] != px[1][v][i];
        const bool shows = (bitsOf(m, R.set) & R.trait) && changed[1] > 0;
        drawn += shows;
        std::string words;
        for (const std::string& t : madeTraitWords(m, kindHas(b.type, KT_SEA))) words += (words.empty() ? "" : ", ") + t;
        printf("  %-16s %s at %lld %lld %lld body %d, species %d %s: %s; the ground %s, then %s; the globe %d px changed, the ground %d\n", R.tag, KINDS[b.type].name,
               (long long)ws.star.sx, (long long)ws.star.sy, (long long)ws.star.sz, wb, wild.kind.s.index, geneEditsText(lookEdits(ws, wb, wild, R)).c_str(),
               words.empty() ? "NOTHING" : words.c_str(), ground[0].c_str(), ground[1].c_str(), changed[0], changed[1]);
    }
    printf("seed looks: %d of %d traits found a world, %d drawn on the ground%s  (%.1f s)\n", found, (int)lookRecipes().size(), drawn,
           missing.empty() ? "" : (", none for " + missing).c_str(), nowSec() - t0);
    return drawn == (int)lookRecipes().size() ? 0 : 1;
}
