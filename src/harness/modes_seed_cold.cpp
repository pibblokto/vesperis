// J-13 (R-454, a culture for any world): `seed cold`. The cold worlds: forty worlds (the first of each landable kind, `seed worlds`' places),
// each place's own warmth and its depths' (`seedDepthsOf`: the rock's levels, or a sea under the ice), and two lines tuned on the gene editor
// as a player would tune them for each (`seedTunedEdits`): the sea light-feeder of `seed worlds` for the ground (its warmth to the place's, brine
// and its frost cut where only a brine stays liquid, DRY where the ground is dry, a SHIELD where the dose asks), and a hydrogen eater for the
// deep (its warmth to a depth's, the shallowest that takes). It prints each verdict, and which worlds took a line on the ground, in the deep,
// or none, and why
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/seeding.h"
#include <sys/stat.h>

namespace {
const MicrobeGene* geneOfKind(const MicrobeGenes& g, int kind) {
    for (const MicrobeGene& x : g.genes) if (x.kind == kind && !x.cut) return &x;
    return nullptr;
}
void putEdit(std::vector<GeneEdit>& out, const GeneEdit& e) {
    for (GeneEdit& x : out) if (x.place == e.place) { x = e; return; }
    out.push_back(e);
}
// a weight gene `d` weights heavier (lighter under 0) from its wild letters
bool weighed(const GeneCode& c, const MicrobeGenes& g, int kind, int d, std::vector<GeneEdit>& out) {
    const MicrobeGene* G = geneOfKind(g, kind);
    if (!G) return false;
    GeneEdit e; e.place = G->place;
    e.part = d >= 0 ? heavier(c, G->part, d) : lighter(c, G->part, -d);
    if (e.part == G->part) return false;
    putEdit(out, e);
    return true;
}
}   // namespace

std::vector<GeneEdit> seedTunedEdits(const StarSystem& ls, int lb, int sp, const SeedVerdict& place) {
    std::vector<GeneEdit> out;
    const SeedLine wild = seedLineFor(ls, lb, sp, {});
    if (!wild.valid || !wild.genes.valid) return out;
    const MicrobeGenes& g = wild.genes;
    const GeneCode c = geneCodeOf(g.genesis);
    const double want = place.place.hab.meanC;
    const bool brine = place.medium.salt >= 2 || want < WATER_COLDEST_C;
    if (brine) weighed(c, g, GN_SALT, 99, out);                                      // its salt to brine
    if (want < WATER_COLDEST_C && wild.kind.world.frostC > FROST_NONE + 1) {          // its own ice cut (a brine's antifreeze)
        GeneEdit e; e.place = geneOfKind(g, GN_FROST) ? geneOfKind(g, GN_FROST)->place : -1; e.cut = true;
        if (e.place >= 0) putEdit(out, e);
    }
    if (place.place.dry && wild.kind.world.dry <= 0) weighed(c, g, GN_DRY, 99, out);
    if (place.doseGy > wild.kind.s.doseGy * (1 + 40 * wild.kind.world.shield)) weighed(c, g, GN_SHIELD, 99, out);
    // its warmth: the weights that bring its best nearest the place's, its salt and frost as set
    const MicrobeGene* W = geneOfKind(g, GN_WARMTH);
    if (!W) return out;
    double best = 1e9; std::vector<GeneEdit> bestEdits = out;
    for (int d = -60; d <= 60; d++) {
        std::vector<GeneEdit> e = out;
        if (d != 0) { GeneEdit x; x.place = W->place; x.part = d > 0 ? heavier(c, W->part, d) : lighter(c, W->part, -d); if (x.part == W->part) continue; putEdit(e, x); }
        const SeedLine L = seedLineFor(ls, lb, sp, e);
        const double miss = std::fabs(L.kind.s.tOpt - want) + (L.kind.s.tMin >= want || L.kind.s.tMax <= want ? 1000 : 0);
        if (miss < best) { best = miss; bestEdits = e; }
    }
    return bestEdits;
}

int seedCold() {
    const double t0 = nowSec();
    StarSystem ls; int lb = -1, sp = -1;
    SeedLine wild;
    if (!seedSource(ls, lb, sp, wild)) { printf("seed cold: no felisian world with a sea light-feeder\n"); return 1; }
    const std::vector<SeedCase> worlds = seedCases();
    // the deep's line: the first hydrogen eater of water living on the forty worlds (a vent's or the rock's)
    int hw = -1, hsp = -1;
    for (size_t wi = 0; wi < worlds.size() && hsp < 0; wi++) {
        const auto life = lifeOf(worlds[wi].sys);
        const Biota& B = life->bodies[worlds[wi].body];
        if (B.chemistry != CH_WATER) continue;
        for (const Microbe& m : B.species)
            if (m.state == SS_LIVING && m.way == LW_CHEMICAL && m.substrate == SU_HYDROGEN && m.oxygen < 2) { hw = (int)wi; hsp = m.index; break; }
    }
    if (hsp < 0) { printf("seed cold: no hydrogen eater on the forty worlds\n"); return 1; }
    // and a dead matter's eater that lives without oxygen (a fermenter), for an airless sea's or a hydrogen air's
    int fw = -1, fsp = -1;
    for (size_t wi = 0; wi < worlds.size() && fsp < 0; wi++) {
        const auto life = lifeOf(worlds[wi].sys);
        const Biota& B = life->bodies[worlds[wi].body];
        if (B.chemistry != CH_WATER) continue;
        for (const Microbe& m : B.species)
            if (m.state == SS_LIVING && m.way == LW_DEAD && m.oxygen == 0) { fw = (int)wi; fsp = m.index; break; }
    }
    if (fsp < 0) { printf("seed cold: no fermenter on the forty worlds\n"); return 1; }
    const StarSystem& fs = worlds[fw].sys;
    const int fb = worlds[fw].body;
    const StarSystem& hs = worlds[hw].sys;
    const int hb = worlds[hw].body;
    const SeedLine hwild = seedLineFor(hs, hb, hsp, {});
    printf("seed cold: the light-feeder species %d of the felisian world at %lld %lld %lld body %d (%.0f to %.0f C); the hydrogen eater species %d of %s at %lld %lld "
           "%lld body %d (%s, %.0f to %.0f C, salt %d to %d); the fermenter species %d of %s at %lld %lld %lld body %d\n", sp, (long long)ls.star.sx, (long long)ls.star.sy,
           (long long)ls.star.sz, lb, wild.kind.s.tMin, wild.kind.s.tMax, hsp, KINDS[hs.bodies[hb].type].name, (long long)hs.star.sx, (long long)hs.star.sy, (long long)hs.star.sz,
           hb, SUBSTRATE_NAMES[hwild.kind.s.substrate], hwild.kind.s.tMin, hwild.kind.s.tMax, hwild.kind.s.saltLo, hwild.kind.s.saltHi, fsp, KINDS[fs.bodies[fb].type].name,
           (long long)fs.star.sx, (long long)fs.star.sy, (long long)fs.star.sz, fb);
    int onGround = 0, inDeep = 0, none = 0, furnace = 0, wildTook = 0;
    std::string noneList;
    struct Tuned { SeedVerdict v; SeedLine line; std::vector<GeneEdit> edits; };
    auto tune = [&](const StarSystem& srcSys, int srcBody, int srcSp, const StarSystem& sys, int bi, double lat, double lon, double km) {
        Tuned t;
        const SeedVerdict place = seedPlaceFacts(sys, bi, lat, lon, km);
        t.edits = seedTunedEdits(srcSys, srcBody, srcSp, place);
        t.line = seedLineFor(srcSys, srcBody, srcSp, t.edits);
        t.v = seedVerdictOf(sys, bi, t.line, lat, lon, km);
        return t;
    };
    auto said = [&](const char* where, const std::string& name, const Tuned& t) {
        printf("  %s %s: %s (%.0f to %.0f C, salt %d to %d%s): %s; %s\n", where, name.c_str(), geneEditsText(t.edits).c_str(), t.line.kind.s.tMin, t.line.kind.s.tMax,
               t.line.kind.s.saltLo, t.line.kind.s.saltHi, brineAntifreeze(t.line.kind) ? ", BRINE ANTIFREEZE" : "", seedPlaceWords(t.v).c_str(), seedWhyWords(t.v, t.line.kind).c_str());
    };
    for (size_t wi = 0; wi < worlds.size(); wi++) {
        const StarSystem& sys = worlds[wi].sys;
        const int bi = worlds[wi].body;
        const Body& b = sys.bodies[bi];
        const double lat = worlds[wi].lat, lon = worlds[wi].lon;
        const SeedVerdict P = seedPlaceFacts(sys, bi, lat, lon);
        const std::vector<double> depths = seedDepthsOf(sys, bi);
        std::string ds;
        for (size_t k = 1; k < depths.size(); k++) {
            const SeedVerdict D = seedPlaceFacts(sys, bi, lat, lon, depths[k]);
            ds += fmt("%s%.0f km %.0f C", ds.empty() ? "" : ", ", D.place.depthKm, D.place.hab.meanC);
        }
        printf("world %2zu: %s (R %.0f km, %.1f C a km%s) at %.1f %.1f: %s, %.0f C; down: %s\n", wi + 1, KINDS[b.type].name, b.radiusKm, deepGradientC(sys, bi),
               iceShellKm(b) > 0 ? ", a sea under its ice" : "", lat / DEG, lon / DEG, P.place.name.c_str(), P.place.hab.meanC, ds.c_str());
        if (P.fate == SF_FURNACE) { furnace++; printf("  a furnace: closed at every depth\n"); continue; }
        wildTook += seedVerdictOf(sys, bi, wild, lat, lon).takes();
        // the ground: at its place and at its mildest (the warmth nearest 15 C on a grid of 24 by 12), the sea light-feeder or the hydrogen eater
        double mLat = lat, mLon = lon, mMiss = 1e9;
        for (int j = 0; j < 12; j++)
            for (int i = 0; i < 24; i++) {
                const double la = (j + 0.5) / 12 * PI - PI / 2, lo = (i + 0.5) / 24 * TAU - PI;
                const SeedVerdict Q = seedPlaceFacts(sys, bi, la, lo);
                if (Q.place.where == AT_MOLTEN) continue;
                const double miss = std::fabs(Q.place.hab.meanC - 15);
                if (miss < mMiss) { mMiss = miss; mLat = la; mLon = lo; }
            }
        Tuned ground; bool groundTook = false; std::string groundAt;
        const char* const WHO[3] = {"the light-feeder", "the hydrogen eater", "the fermenter"};
        for (int at = 0; at < 2 && !groundTook; at++)
            for (int who = 0; who < 3 && !groundTook; who++) {
                const double la = at ? mLat : lat, lo = at ? mLon : lon;
                ground = who == 2 ? tune(fs, fb, fsp, sys, bi, la, lo, 0) : (who ? tune(hs, hb, hsp, sys, bi, la, lo, 0) : tune(ls, lb, sp, sys, bi, la, lo, 0));
                groundTook = ground.v.takes();
                groundAt = fmt("%s at %.0f %.0f, %s", WHO[who], la / DEG, lo / DEG, ground.v.place.name.c_str());
            }
        said("ground", groundAt, ground);
        // the deep: the hydrogen eater tuned to each depth, the shallowest that takes; else the one nearest 20 C, and why
        Tuned deep; bool deepTook = false; double nearest = 1e9; Tuned nearestT;
        for (int at = 0; at < 2 && !deepTook; at++)   // (at its place, then at its mildest)
            for (size_t k = 1; k < depths.size() && !deepTook; k++) {
                deep = tune(hs, hb, hsp, sys, bi, at ? mLat : lat, at ? mLon : lon, depths[k]);
                deepTook = deep.v.takes();
                const double miss = std::fabs(deep.v.place.hab.meanC - 20);
                if (miss < nearest) { nearest = miss; nearestT = deep; }
            }
        if (!deepTook && depths.size() > 1) deep = nearestT;
        if (depths.size() > 1) said("deep", deep.v.place.name, deep);
        if (groundTook) onGround++;
        else if (deepTook) inDeep++;
        else { none++; noneList += fmt("%s%s", noneList.empty() ? "" : ", ", KINDS[b.type].name); }
    }
    // the frames (`shots/tests/cold/`): the seed page on the first thin-aired world, on its ground and five kilometres down, with the hydrogen eater
    // tuned for that depth in flask 1; a red hydrogen eater let go in the sea under the first europan world's ice, its globe before and at its
    // 24th step, its data sheet, and its ground on a stained linea
    mkdir("shots/tests/cold", 0755);
    bool pageShown = false, cracksShown = false;
    int cracksChanged = 0;
    std::string cracksWords;
    for (size_t wi = 0; wi < worlds.size() && !pageShown; wi++) {
        const StarSystem& sys = worlds[wi].sys;
        const int bi = worlds[wi].body;
        if (sys.bodies[bi].type != PT_THINATMO) continue;
        const double lat = worlds[wi].lat, lon = worlds[wi].lon;
        const std::vector<double> depths = seedDepthsOf(sys, bi);
        int k5 = 0;
        for (size_t k = 0; k < depths.size(); k++) if (depths[k] == 5) k5 = (int)k;
        const std::vector<GeneEdit> de = seedTunedEdits(hs, hb, hsp, seedPlaceFacts(sys, bi, lat, lon, 5));
        TestGame g;
        g.savePrefix = "shots/tests/cold_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/cold_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(de);
        g.guide.strains[hwild.key + "/S1"] = rec;
        FlaskSpecies w; w.sx = hs.star.sx; w.sy = hs.star.sy; w.sz = hs.star.sz; w.body = hb; w.species = hsp; w.strain = 1;
        const bool full = labFlaskFull(g, 0, w, seedBestMedium(seedLineFor(hs, hb, hsp, de).kind));
        g.testParkAt(sys.star, bi, false); g.setState(GameState::SPACE);
        Input in;
        auto press = [&](int key) { in.pressed[key] = in.down[key] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; };
        g.openScreen(GameState::DRONES);
        press(KEY_S);
        g.testSeedPage(lat, lon);
        for (int n = 0; n < 30 * 20 && (g.testBay().mapW <= 0 || g.testStatusOn()); n++) g.frame(in, 1.0 / 30);   // (its map drawn, the arrival's status gone)
        writePNG("shots/tests/cold/seed_page_ground.png", g.output(), FBW, FBH);
        const std::string ground = g.testSeedWhere();
        for (int k = 0; k < k5; k++) press(KEY_PAGE_DOWN);
        g.frame(in, 1.0 / 30);
        writePNG("shots/tests/cold/seed_page_deep.png", g.output(), FBW, FBH);
        printf("seed cold: the seed page on %s, flask 1 %s: on the ground '%s', 5 km down '%s'\n", KINDS[sys.bodies[bi].type].name, full ? "full" : "NOT FULL", ground.c_str(),
               g.testSeedWhere().c_str());
        pageShown = full && g.state == GameState::SEEDING && g.testSeedWhere() != ground;
        remove(g.guidePath.c_str()); remove(g.slotPath(0).c_str());
    }
    for (size_t wi = 0; wi < worlds.size() && !cracksShown; wi++) {
        const StarSystem& ws = worlds[wi].sys;
        const int wb = worlds[wi].body;
        const Body& b = ws.bodies[wb];
        if (b.type != PT_EUROPAN) continue;
        const double lat = worlds[wi].lat, lon = worlds[wi].lon, shell = iceShellKm(b);
        std::vector<GeneEdit> e = seedTunedEdits(hs, hb, hsp, seedPlaceFacts(ws, wb, lat, lon, shell));
        for (const GeneEdit& x : seedLineEdits(hwild.genes, SL_DYE)) putEdit(e, x);
        SeedLine red = seedLineFor(hs, hb, hsp, e);
        red.key = hwild.key + "/S1";
        const Seeding sd = seedingFor(ws, wb, red, lat, lon, 0);
        Seeding deepSd = sd; deepSd.depthKm = shell; deepSd.run = false; seedingRun(ws, wb, deepSd);
        // a site on the ice by a stained linea near the place (its ground's dust), facing it, the sun high over it
        const BodyGen g0 = BodyGen::make(b);
        auto at = [&](double laR, double loR, double km, double a) {
            const double d = km / b.radiusKm;
            return StarSystem::bodyFromLatLon(laR + d * std::cos(a), loR + d * std::sin(a) / std::max(0.05, std::cos(laR)));
        };
        double siteLa = lat / DEG, siteLo = lon / DEG, yaw = 0;
        bool onLinea = false;
        for (double km = 0.5; km < 600 && !onLinea; km *= 1.3)
            for (int k = 0; k < 24 && !onLinea; k++) {
                const Vec3 u = at(lat, lon, km, k * TAU / 24);
                if (sampleSurface(g0, u, 64).material != MAT_DUST) continue;
                const double la = std::asin(clampd(u.z, -1, 1)), lo = std::atan2(u.y, u.x);
                for (double m = 0.1; m < 12 && !onLinea; m *= 1.3)   // (out to the ice beside it, 150 m on)
                    for (int j = 0; j < 16 && !onLinea; j++) {
                        const Vec3 v = at(la, lo, m + 0.15, j * TAU / 16);
                        if (sampleSurface(g0, at(la, lo, m, j * TAU / 16), 64).material != MAT_ICE || sampleSurface(g0, v, 64).material != MAT_ICE) continue;
                        siteLa = std::asin(clampd(v.z, -1, 1)) / DEG; siteLo = std::atan2(v.y, v.x) / DEG; yaw = j * TAU / 16 + PI; onLinea = true;
                    }
            }
        const Vec3 site = StarSystem::bodyFromLatLon(siteLa * DEG, siteLo * DEG);
        const double turn = std::max(3600.0, std::fabs(b.rotPeriod));
        double tDay = 20000, sunBest = -2;
        for (int k = 0; k < 192 && sunBest <= 0.6; k++) {
            const double tt = 20000 + turn * k / 192;
            const double sun = dot(normalize(ws.bodyFrame(wb, tt) * normalize(ws.star.pos - ws.bodyPos(wb, tt))), site);
            if (sun > sunBest) { sunBest = sun; tDay = tt; }
        }
        TestGame g;
        g.savePrefix = "shots/tests/cold_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/cold_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        Input in;
        auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
        auto press = [&](int key) { in.pressed[key] = in.down[key] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; };
        std::vector<uint32_t> before, after;
        auto orbit = [&](const char* tag, std::vector<uint32_t>& px) {
            g.testSetTime(tDay); g.testParkAt(ws.star, wb, true, false); g.testLandCursor(siteLa, siteLo);
            run(0.3); g.testWaitMapOf(wb); run(0.1);
            press(KEY_ESCAPE); run(0.4);
            px.assign(g.output(), g.output() + FBW * FBH);
            writePNG(fmt("shots/tests/cold/cracks_%s.png", tag).c_str(), g.output(), FBW, FBH);
        };
        orbit("orbit_before", before);
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(e);
        g.guide.strains[red.key] = rec;
        SeedRecord r; r.body = Guide::bodyKey(ws.star.sx, ws.star.sy, ws.star.sz, wb); r.lat = lat; r.lon = lon; r.depthKm = shell; r.line = red.key; r.edits = rec.edits;
        r.cell = CC_WATER; r.tempC = std::round(red.kind.s.tOpt); r.light = CL_DARK; r.energy = CE_HYDROGEN; r.salt = 1; r.gen = GEN_VERSION;
        r.clock = g.guide.clock - (MADE_STEPS * MADE_STEP_S + 1) / std::max(0.1, deepSd.verdict.pace);
        g.guide.addSeeding(r);
        g.testClockSkip(0);
        orbit("orbit_after", after);
        const MadeWorld now = g.testMadeNow(wb);
        for (const std::string& t : madeTraitWords(now, kindHas(b.type, KT_SEA))) cracksWords += (cracksWords.empty() ? "" : ", ") + t;
        press(KEY_I); run(0.1); press(KEY_RIGHT); run(0.1);   // (its second page: SEEDED, MADE)
        writePNG("shots/tests/cold/cracks_sheet.png", g.output(), FBW, FBH);
        press(KEY_ESCAPE); run(0.1);
        g.testSetTime(tDay); g.testParkAt(ws.star, wb); g.testLandCursor(siteLa, siteLo);
        press(KEY_ENTER);
        for (int n = 0; n < 30 * 40 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
        g.testSetYaw(yaw); g.testSetPitch(-0.12);
        run(2.0);
        const bool landed = g.state == GameState::SURFACE;
        if (landed) writePNG("shots/tests/cold/cracks_ground.png", g.output(), FBW, FBH);
        for (size_t i = 0; i < before.size() && i < after.size(); i++) cracksChanged += before[i] != after[i];
        printf("seed cold: a red hydrogen eater let go %.0f km under the ice of %s at %lld %lld %lld body %d: %s; at its 24th step %s; the globe %d px changed, %s%s\n", shell,
               KINDS[b.type].name, (long long)ws.star.sx, (long long)ws.star.sy, (long long)ws.star.sz, wb, seedWhyWords(deepSd.verdict, red.kind).c_str(),
               cracksWords.empty() ? "NOTHING" : cracksWords.c_str(), cracksChanged, onLinea ? "landed on a stained linea" : "NO LINEA NEAR", landed ? "" : ", NOT LANDED");
        cracksShown = deepSd.verdict.takes() && cracksChanged > 0 && landed && (now.grounds & MGT_COLOURED_CRACKS);
        remove(g.guidePath.c_str()); remove(g.slotPath(0).c_str());
    }
    printf("seed cold: %zu worlds: the wild line took on %d; tuned, a line takes on the ground of %d, in the deep of %d more; none on %d (%s); %d furnaces; the seed page %s, "
           "the cracks %s  (%.1f s)\n", worlds.size(), wildTook, onGround, inDeep, none, noneList.empty() ? "-" : noneList.c_str(), furnace, pageShown ? "drawn" : "NOT DRAWN",
           cracksShown ? "drawn" : "NOT DRAWN", nowSec() - t0);
    return pageShown && cracksShown ? 0 : 1;
}
