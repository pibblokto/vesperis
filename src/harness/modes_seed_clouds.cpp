// J-14 (R-454, a culture for any world): `seed clouds`. The clouds of the worlds with a cloud deck life lives in (a venusian world's, the first
// venusian and acid worlds): their heights (`seedHeightsOf`) and what a culture meets at each; a venusian world's own cloud line (`seedCloudSource`)
// wild at each height, the sea light-feeder of `seed worlds` refused there (WRONG LIQUID), the cloud line tuned for each height as a player would
// tune it (`seedTunedEdits`). Then that line dyed red and given a glow (`seedGlowDonor`) let go in the clouds: a venusian world's globe from
// orbit before and at its 24th step by day and by night, its data sheet; an acid world's the same by day; and the seed page in a venusian
// world's clouds, the line in flask 1
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/seeding.h"
#include <sys/stat.h>

namespace {
// a world's first living species of the clouds that lives on what they give (light, dead matter: a vial's of them), its index (-1 none)
int cloudSpecies(const StarSystem& sys, int body) {
    const auto life = lifeOf(sys);
    const Biota& B = life->bodies[body];
    for (const Microbe& m : B.species)
        if (m.state == SS_LIVING && (m.niches & nicheBit(NI_CLOUD)) && (m.way == LW_LIGHT || m.way == LW_DEAD)) return m.index;
    return -1;
}
// the guide's record of a line let go at a place `steps` of its steps ago (an edited strain of the explorer's)
void letGoAgo(TestGame& g, const StarSystem& ws, int wb, const SeedLine& line, const std::string& edits, double lat, double lon, double depthKm, double pace, int steps) {
    StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = edits;
    g.guide.strains[line.key] = rec;
    SeedRecord r; r.body = Guide::bodyKey(ws.star.sx, ws.star.sy, ws.star.sz, wb); r.lat = lat; r.lon = lon; r.depthKm = depthKm; r.line = line.key; r.edits = edits;
    r.cell = CC_ACID; r.tempC = std::round(line.kind.s.tOpt); r.light = CL_WHITE; r.energy = CE_BROTH; r.salt = 0; r.gen = GEN_VERSION;
    r.clock = g.guide.clock - (steps * MADE_STEP_S + 1) / std::max(0.1, pace);
    g.guide.addSeeding(r);
    g.testClockSkip(0);
}
}   // namespace

bool seedCloudSource(StarSystem& cs, int& cb, int& csp) {
    csp = -1;
    forTypeBodies(PT_VENUSIAN, 16, false, [&](const StarSystem& sys, int bi) {
        if (csp >= 0) return;
        const int s = cloudSpecies(sys, bi);
        if (s >= 0) { cs = sys; cb = bi; csp = s; }
    });
    return csp >= 0;
}

bool seedGlowDonor(GeneEdit& glow) {
    for (const SeedCase& dc : seedCases()) {
        const auto life = lifeOf(dc.sys);
        for (const Microbe& m : life->bodies[dc.body].species) {
            if (m.state != SS_LIVING || !m.glows) continue;
            const SeedLine d = seedLineFor(dc.sys, dc.body, m.index, {});
            for (const MicrobeGene& G : d.genes.genes)
                if (G.kind == GN_GLOW && !G.cut) {
                    glow = GeneEdit();
                    glow.place = G.place; glow.how = GX_MOVED; glow.part = G.part; glow.donor = d.key; glow.read = true;
                    glow.donorKind = d.kind; glow.donorForm = d.form; glow.donorGene = G; glow.far = GFROM_STAR; glow.origin = d.key;
                    return true;
                }
        }
    }
    return false;
}

int seedClouds() {
    const double t0 = nowSec();
    // the line: the first venusian worlds' first cloud species that lives on what the clouds give
    StarSystem cs; int cb = -1, csp = -1;
    if (!seedCloudSource(cs, cb, csp)) { printf("seed clouds: no venusian world with a cloud species of light or dead matter\n"); return 1; }
    const SeedLine cwild = seedLineFor(cs, cb, csp, {});
    StarSystem ls; int lb = -1, lsp = -1;
    SeedLine water;
    if (!seedSource(ls, lb, lsp, water)) { printf("seed clouds: no felisian world with a sea light-feeder\n"); return 1; }
    printf("seed clouds: the cloud species %d of the %s world at %lld %lld %lld body %d (%s, %s, %.0f to %.0f C, pH %.1f to %.1f%s); the water line %s\n", csp,
           KINDS[cs.bodies[cb].type].name, (long long)cs.star.sx, (long long)cs.star.sy, (long long)cs.star.sz, cb, CULTURE_CELL_NAMES[chemistryCell(cwild.kind.chemistry)],
           seedNeedsWords(cwild.kind).c_str(),
           cwild.kind.s.tMin, cwild.kind.s.tMax, cwild.kind.s.phLo, cwild.kind.s.phHi, cwild.kind.s.glows ? ", GLOWS" : "", water.key.c_str());
    // the worlds: the line's own, then the first venusian and acid worlds; each one's heights, what each gives, the lines' verdicts
    struct World { StarSystem sys; int body = -1; double lat = 0, lon = 0; };
    std::vector<World> worlds;
    { World w; w.sys = cs; w.body = cb; worlds.push_back(w); }
    for (int type : {PT_VENUSIAN, PT_ACIDIC}) {
        World w;
        if (!firstBodyOfType(type, w.sys, w.body)) continue;
        if (w.sys.star.seed == cs.star.seed && w.body == cb) continue;
        seedPlaceFor(w.sys, w.body, w.lat, w.lon);
        worlds.push_back(w);
    }
    int heightsTook = 0, tunedTook = 0, waterRefused = 0, levels = 0;
    for (const World& w : worlds) {
        const Body& b = w.sys.bodies[w.body];
        const std::vector<double> H = seedHeightsOf(w.sys, w.body);
        const SeedVerdict ground = seedPlaceFacts(w.sys, w.body, w.lat, w.lon);
        printf("  %s at %lld %lld %lld body %d, %.0f %.0f: the ground %s; %zu heights\n", KINDS[b.type].name, (long long)w.sys.star.sx, (long long)w.sys.star.sy,
               (long long)w.sys.star.sz, w.body, w.lat / DEG, w.lon / DEG, seedPlaceWords(ground).c_str(), H.size());
        for (double km : H) {
            levels++;
            const SeedVerdict P = seedPlaceFacts(w.sys, w.body, w.lat, w.lon, -km);
            const SeedVerdict vw = seedVerdictOf(w.sys, w.body, cwild, w.lat, w.lon, -km);
            const SeedVerdict vwater = seedVerdictOf(w.sys, w.body, water, w.lat, w.lon, -km);
            const std::vector<GeneEdit> e = seedTunedEdits(cs, cb, csp, P);
            const SeedLine tuned = seedLineFor(cs, cb, csp, e);
            const SeedVerdict vt = seedVerdictOf(w.sys, w.body, tuned, w.lat, w.lon, -km);
            heightsTook += vw.takes(); tunedTook += vt.takes(); waterRefused += vwater.fate == SF_SOLVENT;
            printf("    %s: %s, pH %.1f, %.2f Gy; wild: %s; water: %s; tuned %s (%.0f to %.0f C): %s\n", P.place.name.c_str(), seedPlaceWords(P).c_str(), P.ph, P.doseGy,
                   seedWhyWords(vw, cwild.kind).c_str(), seedWhyWords(vwater, water.kind).c_str(), geneEditsText(e).c_str(), tuned.kind.s.tMin, tuned.kind.s.tMax,
                   seedWhyWords(vt, tuned.kind).c_str());
        }
    }
    printf("seed clouds: %d heights on %zu worlds: the wild cloud line takes at %d, tuned at %d; the water line refused (WRONG LIQUID) at %d\n", levels, worlds.size(),
           heightsTook, tunedTook, waterRefused);
    // the frames (`shots/tests/clouds/`): the line dyed red and given a glow (a glowing species' GLOW gene moved in, as a player moves one), let go in
    // the clouds of the first venusian world at the height nearest 35 C where it takes: its globe by day before and at its 24th step, by night at
    // its 24th step, its data sheet; the same in the clouds of the first acid world by day; the seed page in the venusian world's clouds, the line
    // in flask 1
    mkdir("shots/tests/clouds", 0755);
    GeneEdit glow;
    const bool glows = seedGlowDonor(glow);
    auto redLine = [&](const SeedVerdict& place, std::vector<GeneEdit>& e) {
        e = seedTunedEdits(cs, cb, csp, place);
        for (const GeneEdit& x : seedLineEdits(cwild.genes, SL_DYE)) e.push_back(x);
        if (glows) e.push_back(glow);
        SeedLine L = seedLineFor(cs, cb, csp, e);
        L.key = cwild.key + "/S1";
        return L;
    };
    auto runAt = [&](const StarSystem& sys, int bi, const SeedLine& line, double lat, double lon, double depthKm) {
        Seeding sd = seedingFor(sys, bi, line, lat, lon, 0);
        sd.depthKm = depthKm; sd.run = false; seedingRun(sys, bi, sd);
        return sd;
    };
    bool venusShown = false, acidShown = false, pageShown = false;
    for (size_t wi = 1; wi < worlds.size(); wi++) {
        const StarSystem& ws = worlds[wi].sys;
        const int wb = worlds[wi].body;
        const Body& b = ws.bodies[wb];
        const bool venus = b.type == PT_VENUSIAN;
        if ((venus && venusShown) || (!venus && acidShown)) continue;
        const double lat = worlds[wi].lat, lon = worlds[wi].lon;
        const std::vector<double> H = seedHeightsOf(ws, wb);
        double km = 0, bestMiss = 1e9;
        std::vector<GeneEdit> e;
        SeedLine red;
        for (double h : H) {
            const SeedVerdict P = seedPlaceFacts(ws, wb, lat, lon, -h);
            std::vector<GeneEdit> eh;
            const SeedLine L = redLine(P, eh);
            if (!runAt(ws, wb, L, lat, lon, -h).verdict.takes()) continue;
            const double miss = std::fabs(P.place.hab.meanC - 35);
            if (miss < bestMiss) { bestMiss = miss; km = h; e = eh; red = L; }
        }
        if (km <= 0) { printf("seed clouds: the red line takes at no height of %s\n", KINDS[b.type].name); continue; }
        const Seeding sd = runAt(ws, wb, red, lat, lon, -km);
        const Vec3 site = StarSystem::bodyFromLatLon(lat, lon);
        const double turn = std::max(3600.0, std::fabs(b.rotPeriod));
        double tDay = 20000, tNight = 20000, sunBest = -2, sunWorst = 2;
        for (int k = 0; k < 192; k++) {
            const double tt = 20000 + turn * k / 192;
            const double sun = dot(normalize(ws.bodyFrame(wb, tt) * normalize(ws.star.pos - ws.bodyPos(wb, tt))), site);
            if (sun > sunBest) { sunBest = sun; tDay = tt; }
            if (sun < sunWorst) { sunWorst = sun; tNight = tt; }
        }
        TestGame g;
        g.savePrefix = "shots/tests/clouds_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/clouds_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        Input in;
        auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
        auto press = [&](int key) { in.pressed[key] = in.down[key] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; };
        const std::string tag = venus ? "venus" : "acid";
        std::vector<uint32_t> before, after;
        auto orbit = [&](double tt, const std::string& name, std::vector<uint32_t>* px, bool night = false) {
            g.testSetTime(tt); g.testParkAt(ws.star, wb, true, night); g.testLandCursor(lat / DEG, lon / DEG);
            run(0.3); g.testWaitMapOf(wb); run(0.1);
            press(KEY_ESCAPE); run(0.4);
            if (px) px->assign(g.output(), g.output() + FBW * FBH);
            writePNG(("shots/tests/clouds/" + name + ".png").c_str(), g.output(), FBW, FBH);
        };
        orbit(tDay, tag + "_orbit_before", &before);
        letGoAgo(g, ws, wb, red, geneEditsText(e), lat, lon, -km, sd.verdict.pace, MADE_STEPS);
        orbit(tDay, tag + "_orbit_after", &after);
        const std::vector<Seeding>& ss = g.testSeedings(wb);
        const bool took = !ss.empty() && ss.back().verdict.takes();
        const MadeWorld now = g.testMadeNow(wb);
        std::string words;
        for (const std::string& t : madeTraitWords(now, kindHas(b.type, KT_SEA))) words += (words.empty() ? "" : ", ") + t;
        int changed = 0;
        for (size_t i = 0; i < before.size() && i < after.size(); i++) changed += before[i] != after[i];
        int night = 0;
        if (venus) {   // (its night side as generated and with the line: the guide's seeding taken out and put back)
            std::vector<uint32_t> darkAfter, darkBefore;
            orbit(tNight, tag + "_orbit_night", &darkAfter, true);
            const std::vector<SeedRecord> kept = g.guide.seeded;
            g.guide.seeded.clear(); g.testClockSkip(0);
            orbit(tNight, tag + "_orbit_night_before", &darkBefore, true);
            g.guide.seeded = kept; g.testClockSkip(0);
            for (size_t i = 0; i < darkBefore.size() && i < darkAfter.size(); i++) night += darkBefore[i] != darkAfter[i];
        }
        press(KEY_I); run(0.1);
        writePNG(("shots/tests/clouds/" + tag + "_sheet.png").c_str(), g.output(), FBW, FBH);
        press(KEY_ESCAPE); run(0.1);
        printf("seed clouds: a red%s line let go %.0f km up in the clouds of %s at %lld %lld %lld body %d: %s (the game's: %s), share %.2f; at its 24th step %s (clouds %.2f, "
               "glow %.2f); the globe %d px changed%s\n", glows ? ", glowing" : "", km, KINDS[b.type].name, (long long)ws.star.sx, (long long)ws.star.sy, (long long)ws.star.sz,
               wb, seedWhyWords(sd.verdict, red.kind).c_str(), took ? "GROWS" : "NOT TAKEN", sd.verdict.share, words.empty() ? "NOTHING" : words.c_str(), now.cloudTintK,
               now.cloudGlow, changed, venus ? fmt(", its night side %d", night).c_str() : "");
        const bool shown = took && changed > 0 && (now.air & MA_DYE_CLOUDS) && (!venus || !glows || night > 0);
        (venus ? venusShown : acidShown) = shown;
        remove(g.guidePath.c_str()); remove(g.slotPath(0).c_str());
        if (!venus || pageShown) continue;
        // the seed page: the line in flask 1, the drone over the place, Page Up into the clouds to the height
        TestGame p;
        p.savePrefix = "shots/tests/clouds_save"; p.settingsPath = "shots/tests/test_settings.txt"; p.keysPath = "shots/tests/test_keys.txt";
        p.guidePath = "shots/tests/clouds_guide.txt";
        remove(p.guidePath.c_str());
        p.newGame(); p.settings.cabin = false; applyTestScale(p);
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(e);
        p.guide.strains[red.key] = rec;
        FlaskSpecies w; w.sx = cs.star.sx; w.sy = cs.star.sy; w.sz = cs.star.sz; w.body = cb; w.species = csp; w.strain = 1;
        const bool full = labFlaskFull(p, 0, w, seedBestMedium(red.kind));
        p.testParkAt(ws.star, wb, false); p.setState(GameState::SPACE);
        Input pin;
        auto ppress = [&](int key) { pin.pressed[key] = pin.down[key] = true; p.frame(pin, 1.0 / 30); pin.newFrame(); pin.down[key] = false; };
        p.openScreen(GameState::DRONES);
        ppress(KEY_S);
        p.testSeedPage(lat, lon);
        for (int n = 0; n < 30 * 20 && (p.testBay().mapW <= 0 || p.testStatusOn()); n++) p.frame(pin, 1.0 / 30);
        const std::string ground = p.testSeedWhere();
        int ups = 0;
        for (size_t k = 0; k < H.size(); k++) if (H[k] == km) ups = (int)k + 1;
        for (int k = 0; k < ups; k++) ppress(KEY_PAGE_UP);
        p.frame(pin, 1.0 / 30);
        writePNG("shots/tests/clouds/seed_page_clouds.png", p.output(), FBW, FBH);
        const std::string there = p.testSeedWhere();
        for (int k = 0; k < (int)H.size() + 1; k++) ppress(KEY_PAGE_UP);   // (past the top: refused)
        printf("seed clouds: the seed page on %s, flask 1 %s: on the ground '%s', %.0f km up '%s'; past the top '%s'\n", KINDS[b.type].name, full ? "full" : "NOT FULL",
               ground.c_str(), km, there.c_str(), p.testStatusOn() ? p.testStatus().c_str() : "");
        pageShown = full && p.state == GameState::SEEDING && there != ground && p.testBay().pageDepth == -(int)H.size();
        remove(p.guidePath.c_str()); remove(p.slotPath(0).c_str());
    }
    printf("seed clouds: the glow's donor %s; the venusian world's clouds %s, the acid world's %s, the seed page %s  (%.1f s)\n", glows ? glow.donor.c_str() : "NONE",
           venusShown ? "drawn" : "NOT DRAWN", acidShown ? "drawn" : "NOT DRAWN", pageShown ? "drawn" : "NOT DRAWN", nowSec() - t0);
    return levels > 0 && tunedTook > 0 && waterRefused == levels && venusShown && acidShown && pageShown ? 0 : 1;
}
