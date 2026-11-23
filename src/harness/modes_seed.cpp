// J-02 (R-448): the seeded worlds' modes. `seed test`: a desert world's landing map, its zoom, the globe from the parking orbit, the
// telescope's plate and the ground drawn as generated, then with a test layer of a tint (`seed_test_*_tint.png`), then with one of a cover
// (`_cover.png`), then as generated again, which must be the first pictures pixel for pixel. Each layer has two spreads: one round the
// ground under the ship (the globe, the map, the plate) and one round the landing site (the zoom, the ground)
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/seeding.h"

namespace {
const char* const SEED_VIEWS[] = {"map", "zoom", "globe", "plate", "ground"};
constexpr int SEED_NVIEWS = 5;

struct SeedShots { std::vector<uint32_t> px[SEED_NVIEWS]; uint64_t siteKey = 0, genKey = 0; };

int pixelsDiffering(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    int n = 0;
    for (size_t i = 0; i < a.size() && i < b.size(); i++) n += a[i] != b[i];
    return n;
}
}

SeedLine seedLineFor(const StarSystem& sys, int body, int species, const std::vector<GeneEdit>& edits) {
    SeedLine L;
    if (body < 0 || body >= (int)sys.bodies.size()) return L;
    const auto life = lifeOf(sys);
    const Biota& B = life->bodies[body];
    if (species < 0 || species >= (int)B.species.size()) return L;
    const std::vector<Morph> forms = morphsOf(sys, body);
    L.form = forms[species];
    L.kind = cultureKindOf(B, species, 2 * L.form.bodyUm());
    L.genes = microbeGenesOf(B.species[species], L.form, B.genesis);
    worldOf(L.kind.world, L.genes);
    if (!edits.empty() && L.genes.valid) {   // its edits on its wild self, their price as a flask's (`Game::flaskKind`)
        const CultureKind wild = L.kind; const Morph wildForm = L.form;
        applyEdits(L.kind, L.form, L.genes, edits);
        L.kind.burden = geneBurden(wild, wildForm, L.kind, L.form, L.genes);
    }
    L.key = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, body) + "/M" + std::to_string(species);
    L.valid = true;
    return L;
}

std::string lighter(const GeneCode& c, std::string p, int d) {   // (`heavier`'s other way; J-08: shared, harness.h)
    const char* abc = alphabetOf(c.letters);
    for (int guard = 0; d > 0 && guard < 64; guard++)
        for (int i = 0; i < (int)p.size() && d > 0; i++) {
            const int w = geneWeight(c, p, i, 1);
            if (w <= 0) continue;
            for (int l = 0; l < c.letters; l++) if (c.weight[l] == w - 1) { p[i] = abc[l]; d--; break; }
        }
    return p;
}

const char* const SEED_LINE_NAMES[SL_COUNT] = {"wild", "warm", "cold", "a dye", "a methane-maker", "STOP cut"};

std::vector<GeneEdit> seedLineEdits(const MicrobeGenes& g, int kind) {
    std::vector<GeneEdit> out;
    if (!g.valid || kind == SL_WILD) return out;
    const GeneCode c = geneCodeOf(g.genesis);
    const MicrobeGene* G = nullptr;
    const int want = kind == SL_WARM || kind == SL_COLD ? GN_WARMTH : (kind == SL_DYE ? GN_DYE : (kind == SL_METHANE ? GN_GAS : GN_STOP));
    for (const MicrobeGene& x : g.genes) if (x.kind == want && !x.cut) { G = &x; break; }
    if (!G) return out;
    GeneEdit e; e.place = G->place;
    if (kind == SL_WARM) e.part = heavier(c, G->part, 6);
    else if (kind == SL_COLD) e.part = lighter(c, G->part, 6);
    else if (kind == SL_STOP) e.cut = true;
    else {   // a table's run that reads the dye or the gas, its weight's letters the heaviest
        const std::vector<int>* t = worldTable(c, want);
        const std::string run = t ? runReading(c, *t, kind == SL_DYE ? WD_RED : WG_METHANE, 2) : std::string();
        if (run.size() != 2 || G->part.size() < 2) return out;
        e.part = run + heavier(c, G->part.substr(2), 99);
    }
    if (!e.cut && e.part == G->part) return out;
    out.push_back(e);
    return out;
}

Seeding seedingFor(const StarSystem& sys, int body, const SeedLine& line, double lat, double lon, double clock) {
    Seeding s;
    s.lat = lat; s.lon = lon; s.line = line.key; s.gen = GEN_VERSION; s.clock = clock; s.sent = line;
    seedingRun(sys, body, s);
    return s;
}

bool seedSource(StarSystem& ls, int& lb, int& sp, SeedLine& wild) {
    if (!firstBodyOfType(PT_FELISIAN, ls, lb)) return false;
    const auto life = lifeOf(ls);
    const Biota& LB = life->bodies[lb];
    sp = -1;   // its sea's first light-feeder, else its first that lives on the dead
    for (int way : {LW_LIGHT, LW_DEAD})
        for (const Microbe& m : LB.species) if (sp < 0 && m.state == SS_LIVING && m.niche == NI_LIQUID && m.way == way) sp = m.index;
    if (sp < 0) return false;
    wild = seedLineFor(ls, lb, sp, {});
    return wild.valid;
}

bool seedPlaceFor(const StarSystem& sys, int body, double& lat, double& lon) {
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    double best = 1e9; int kind = 9;
    for (int j = 0; j < 24; j++)
        for (int i = 0; i < 48; i++) {
            const double la = (j + 0.5) / 24 * PI - PI / 2, lo = (i + 0.5) / 48 * TAU - PI;
            const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), SEED_PLACE_M);
            const bool wet = s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water);
            if (!wet) continue;
            const int k = s.waterKind == 0 ? 0 : 1;
            if (k < kind || (k == kind && std::fabs(la) < best)) { kind = k; best = std::fabs(la); lat = la; lon = lo; }
        }
    if (kind < 9) return true;
    Rng r(mix64(b.seed ^ 0x5EEDDULL));
    lat = r.range(-0.9, 0.9); lon = r.range(-PI, PI);
    return false;
}

bool seedPlaceTaking(const StarSystem& sys, int body, const SeedLine& line, double& lat, double& lon) {
    const BodyGen g = BodyGen::make(sys.bodies[body]);
    std::vector<std::pair<double, std::pair<double, double>>> seas;   // its sea's points, the equator's first
    for (int j = 0; j < 24; j++)
        for (int i = 0; i < 48; i++) {
            const double la = (j + 0.5) / 24 * PI - PI / 2, lo = (i + 0.5) / 48 * TAU - PI;
            const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), SEED_PLACE_M);
            if ((s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water)) && s.waterKind == 0) seas.push_back({std::fabs(la) + 1e-3 * i, {la, lo}});
        }
    std::sort(seas.begin(), seas.end());
    for (const auto& p : seas) {
        if (!seedVerdictOf(sys, body, line, p.second.first, p.second.second).takes()) continue;
        lat = p.second.first; lon = p.second.second;
        return true;
    }
    return false;
}

std::vector<SeedCase> seedCases() {
    std::vector<SeedCase> out;   // the first world of each landable kind, in the kinds' order, forty at most
    for (int type = 0; type < PT_COUNT && out.size() < 40; type++) {
        SeedCase c;
        if (!KINDS[type].landable || !firstBodyOfType(type, c.sys, c.body)) continue;
        c.wet = seedPlaceFor(c.sys, c.body, c.lat, c.lon);
        out.push_back(c);
    }
    return out;
}

bool labFlaskFull(TestGame& g, int i, const FlaskSpecies& w, const Medium& m) {
    Flask& F = g.testLab().flasks[i];
    F = Flask(); F.n = 1; F.who[0] = w; F.who[0].strainIn = w.strain; F.medium = m; F.asWorld = m;
    CultureEvent e; e.kind = CD_START; e.t = g.testLab().clock; e.m = m;
    F.events.push_back(e);
    g.testLabSkip(0);
    for (int q = 0; q < 240; q++) {
        const CultureNow n = g.testFlaskNow(i);
        if (n.n > 0 && (n.one[0].stage == SG_CROWD || n.one[0].stage == SG_OVERGROWN)) return true;
        g.testLabSkip(60);
    }
    return false;
}

Medium seedBestMedium(const CultureKind& k) {
    Medium m; m.cell = chemistryCell(k.chemistry); m.tempC = std::round(k.s.tOpt); m.salt = k.s.saltLo;
    m.light = k.s.way == LW_LIGHT ? CL_WHITE : CL_DARK;
    m.energy = k.s.way == LW_CHEMICAL ? k.s.substrate : (k.s.way == LW_LIGHT ? CE_NONE : CE_BROTH);
    return m;
}

namespace {
// the share of a world's waters a layer tints, read from the planet function on a grid of 48 by 24 (its made pass)
double watersTinted(const BodyGen& g0, const MadeWorld& m, int& waters) {
    BodyGen g = g0; g.made = m;
    int wet = 0, tinted = 0;
    for (int j = 0; j < 24; j++)
        for (int i = 0; i < 48; i++) {
            const double la = (j + 0.5) / 24 * PI - PI / 2, lo = (i + 0.5) / 48 * TAU - PI;
            const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 50000);
            if (!(s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water))) continue;
            wet++; tinted += s.madeTint > 0.5;
        }
    waters = wet;
    return wet ? (double)tinted / wet : 0.0;
}
std::string medWords(const Medium& m) {
    return fmt("%s %.0f C %s %s %s", CULTURE_CELL_NAMES[clampi(m.cell, 0, CC_COUNT - 1)], m.tempC, CULTURE_LIGHT_NAMES[clampi(m.light, 0, CL_COUNT - 1)],
               CULTURE_ENERGY_NAMES[clampi(m.energy, 0, CE_COUNT - 1)], CULTURE_SALT_NAMES[clampi(m.salt, 0, 2)]);
}
}   // namespace

// J-03: `seed worlds`: six lines of one species (the first felisian world's first sea light-feeder, ITERATION-1.5.0.md J-03's six) let go on
// forty worlds (the first of each landable kind), each at its sea where it has one: the verdict and why, the place's medium and the native met, the drift, the spread's steps
// (its reach at steps 1, 6, 12 and 24, the share of the world's waters it tinted) and what each shows
int seedWorlds() {
    const double t0 = nowSec();
    StarSystem ls; int lb = -1, sp = -1;
    SeedLine wild;
    if (!seedSource(ls, lb, sp, wild)) { printf("seed worlds: no felisian world with a sea light-feeder\n"); return 1; }
    SeedLine lines[SL_COUNT];
    for (int k = 0; k < SL_COUNT; k++) lines[k] = seedLineFor(ls, lb, sp, seedLineEdits(wild.genes, k));
    const Microbe& w0 = wild.kind.s;
    printf("seed worlds: the lines of species %d of the felisian world at %lld %lld %lld body %d (%s, %s, %s, %.0f to %.0f C, salt %d to %d, %s)\n", sp,
           (long long)ls.star.sx, (long long)ls.star.sy, (long long)ls.star.sz, lb, MICROBE_PLAN_NAMES[w0.plan], NICHE_NAMES[w0.niche], WAY_NAMES[w0.way], w0.tMin, w0.tMax,
           w0.saltLo, w0.saltHi, w0.glows ? "glows" : "no glow");
    for (int k = 0; k < SL_COUNT; k++) {
        const CultureKind& K = lines[k].kind;
        printf("  line %-16s %s: %.0f to %.0f C, %s, %s, %s, price %.1f%%\n", SEED_LINE_NAMES[k], geneEditsText(seedLineEdits(wild.genes, k)).c_str(), K.s.tMin, K.s.tMax,
               K.world.dyes() ? WORLD_DYE_NAMES[K.world.dye] : "no dye", K.world.gives() ? WORLD_GAS_NAMES[K.world.gas] : "no gas", K.world.overgrows ? "never stops" : "stops",
               100 * K.burden);
    }
    const std::vector<SeedCase> worlds = seedCases();
    int fates[SF_COUNT] = {}, took[SL_COUNT] = {}, drifted = 0, shows = 0;
    for (size_t wi = 0; wi < worlds.size(); wi++) {
        const StarSystem& sys = worlds[wi].sys;
        const int bi = worlds[wi].body;
        const Body& b = sys.bodies[bi];
        const double lat = worlds[wi].lat, lon = worlds[wi].lon;
        const bool wet = worlds[wi].wet;
        const BodyGen g = BodyGen::make(b);
        printf("world %2zu: %s at %lld %lld %lld body %d (R %.0f km), %s at %.1f %.1f\n", wi + 1, KINDS[b.type].name, (long long)sys.star.sx, (long long)sys.star.sy,
               (long long)sys.star.sz, bi, b.radiusKm, wet ? "its water" : "its land", lat / DEG, lon / DEG);
        for (int k = 0; k < SL_COUNT; k++) {
            const Seeding sd = seedingFor(sys, bi, lines[k], lat, lon, 0);
            const SeedVerdict& V = sd.verdict;
            fates[V.fate]++; took[k] += V.takes(); drifted += !V.drift.empty();
            std::string line = fmt("  %-16s %-32s %s, %s", SEED_LINE_NAMES[k], seedVerdictWords(V).c_str(), V.place.name.c_str(), medWords(V.medium).c_str());
            if (V.native >= 0) line += fmt("; native %d (%s, %zu beaten)", V.native, CULTURE_PAIR_NAMES[clampi(V.pair, 0, PA_COUNT - 1)], V.beaten.size());
            if (!V.drift.empty()) line += "; drift " + geneEditsText(V.drift);
            printf("%s\n", line.c_str());
            if (!V.takes()) continue;
            std::string steps = fmt("      pace %.2f:", V.pace);
            std::string seen;
            for (int st : {1, 6, 12, 24}) {
                const double clock = (st * MADE_STEP_S + 1) / V.pace;
                const std::vector<Seeding> one = {sd};
                const MadeWorld m = madeWorldOf(sys, bi, one, clock);
                int waters = 0;
                const double share = m.on() ? watersTinted(g, m, waters) : 0.0;
                const double reach = madeReachOfStep(st, b.radiusKm) * b.radiusKm;
                steps += fmt(" step %d %s %.0f%% of the waters;", st, reach >= PI * b.radiusKm ? "the world," : fmt("%.0f km,", reach).c_str(), 100 * share);
                if (st == 24) for (const std::string& t : madeTraitWords(m, kindHas(b.type, KT_SEA))) { seen += (seen.empty() ? "" : ", ") + t; shows++; }
            }
            printf("%s shows %s\n", steps.c_str(), seen.empty() ? "nothing" : seen.c_str());
        }
    }
    std::string fs;
    for (int f = 0; f < SF_COUNT; f++) if (fates[f]) fs += fmt("%s%s %d", fs.empty() ? "" : ", ", SEED_FATE_NAMES[f], fates[f]);
    std::string ts;
    for (int k = 0; k < SL_COUNT; k++) ts += fmt("%s%s %d", ts.empty() ? "" : ", ", SEED_LINE_NAMES[k], took[k]);
    printf("seed worlds: %zu worlds, %d seedings: %s; taken by line: %s; %d drifted, %d looks shown  (%.1f s)\n", worlds.size(), (int)worlds.size() * SL_COUNT, fs.c_str(),
           ts.c_str(), drifted, shows, nowSec() - t0);
    return 0;
}

// J-03: `seed sea`: the dye line let go in the sea off a coast of the first world of a kind with seas where it takes (the kinds' order), seen
// from its orbit (the globe), on the landing map and its zoom, and on the ground at the coast facing the sea, before it went down and at its
// eighth step (`seed_sea_<map|zoom|globe|ground>_<before|after>.png`); the guide's line is a strain record of its edits, as the editor writes one
int seedSea() {
    const double t0 = nowSec();
    StarSystem ls; int lb = -1, sp = -1;
    SeedLine wild;
    if (!seedSource(ls, lb, sp, wild)) { printf("seed sea: no felisian world with a sea light-feeder\n"); return 1; }
    const std::vector<GeneEdit> dye = seedLineEdits(wild.genes, SL_DYE);
    const SeedLine line = seedLineFor(ls, lb, sp, dye);
    StarSystem ws; int wb = -1; double siteLa = 0, siteLo = 0, seaLat = 0, seaLon = 0, pace = 1;
    for (int type = 0; type < PT_COUNT && wb < 0; type++) {
        StarSystem ks; int kb = -1;
        if (!KINDS[type].landable || !kindHas(type, KT_SEA) || !firstBodyOfType(type, ks, kb)) continue;
        const Body& b = ks.bodies[kb];
        const BodyGen g = BodyGen::make(b);
        double la = 0, lo = 0;
        if (!coastSite(g, b.radiusKm, la, lo)) continue;
        for (int k = 0; k < 16 && wb < 0; k++) {   // the sea within a few kilometres of the coast's site, where the line takes
            const double a = k * TAU / 16, d = 4.0 / b.radiusKm;
            const double sLat = la * DEG + d * std::cos(a), sLon = lo * DEG + d * std::sin(a) / std::max(0.05, std::cos(la * DEG));
            const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(sLat, sLon), SEED_PLACE_M);
            if (!(s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water)) || s.waterKind != 0) continue;
            const Seeding sd = seedingFor(ks, kb, line, sLat, sLon, 0);
            if (!sd.verdict.takes()) continue;
            ws = ks; wb = kb; siteLa = la; siteLo = lo; seaLat = sLat; seaLon = sLon; pace = sd.verdict.pace;
        }
    }
    if (wb < 0) { printf("seed sea: no world of seas where the dye line takes\n"); return 1; }
    const Body& b = ws.bodies[wb];
    TestGame g;
    g.savePrefix = "shots/tests/seed_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
    g.guidePath = "shots/tests/seed_guide.txt";
    remove(g.guidePath.c_str());
    g.newGame(); g.settings.cabin = false; applyTestScale(g);
    Input in;
    auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
    auto press = [&](int k) { in.pressed[k] = in.down[k] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
    // the time the ground under the parked ship is the sea let go in (its day side: the parking's), within a turn of the world
    double T0 = 20000;
    {
        g.testSetTime(T0); g.testParkAt(ws.star, wb);
        const StarSystem& S = g.testSystem();
        const Vec3 at = StarSystem::bodyFromLatLon(seaLat, seaLon);
        double best = -2;
        const double turn = std::max(3600.0, std::fabs(b.rotPeriod));
        for (int k = 0; k < 192; k++) {
            const double tt = 20000 + turn * k / 192;
            const Vec3 bp = S.bodyPos(wb, tt), toStar = normalize(S.star.pos - bp), side = normalize(cross(toStar, Vec3(0, 1, 0)));
            const Vec3 sub = normalize(S.bodyFrame(wb, tt) * normalize(toStar * 0.7 + side * 0.7 + Vec3(0, 0.2, 0)));
            if (dot(sub, at) > best) { best = dot(sub, at); T0 = tt; }
        }
    }
    // the ground's bearing from the coast's site to the sea
    const Vec3 su = StarSystem::bodyFromLatLon(siteLa * DEG, siteLo * DEG), wu = StarSystem::bodyFromLatLon(seaLat, seaLon);
    const Vec3 east = normalize(cross(Vec3(0, 0, 1), su)), north = cross(su, east);
    const double yaw = std::atan2(dot(wu - su, east), dot(wu - su, north));
    std::vector<uint32_t> px[2][4];
    auto views = [&](const char* tag, int pass) {
        auto shot = [&](int v, const char* view) {
            px[pass][v].assign(g.output(), g.output() + FBW * FBH);
            writePNG(fmt("shots/tests/seed_sea_%s_%s.png", view, tag).c_str(), g.output(), FBW, FBH);
        };
        g.testSetTime(T0); g.testParkAt(ws.star, wb); g.testLandCursor(siteLa, siteLo);
        run(0.3); shot(0, "map");
        press(KEY_Z); run(0.5); shot(1, "zoom"); press(KEY_Z); run(0.1);
        press(KEY_ESCAPE); run(0.3); shot(2, "globe");
        g.testSetTime(T0 + 300); g.testParkAt(ws.star, wb); g.testLandCursor(siteLa, siteLo);
        press(KEY_ENTER);
        for (int n = 0; n < 30 * 40 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
        g.testSetYaw(yaw); run(2.0); shot(3, "ground");
    };
    views("before", 0);
    {   // the line a strain of the explorer's (the editor's record of its edits), let go in the sea at its eighth step on the guide's clock
        const std::string lineKey = line.key + "/S1";
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(dye);
        g.guide.strains[lineKey] = rec;
        SeedRecord r; r.body = Guide::bodyKey(ws.star.sx, ws.star.sy, ws.star.sz, wb); r.lat = seaLat; r.lon = seaLon; r.line = lineKey; r.edits = rec.edits;
        r.cell = CC_WATER; r.tempC = 30; r.light = CL_WHITE; r.energy = CE_BROTH; r.salt = 1; r.gen = GEN_VERSION;
        r.clock = g.guide.clock - (8 * MADE_STEP_S + 1) / pace;
        g.guide.addSeeding(r);
        g.testClockSkip(0);
    }
    views("after", 1);
    const MadeWorld m = g.testMadeNow(wb);
    const char* const names[4] = {"map", "zoom", "globe", "ground"};
    int moved = 0;
    printf("seed sea: the dye line (species %d of %lld %lld %lld body %d) let go in the sea of %s at %lld %lld %lld body %d (R %.0f km) at %.3f %.3f, off the coast's site at "
           "%.3f %.3f; its layer at step %d: %s\n", sp, (long long)ls.star.sx, (long long)ls.star.sy, (long long)ls.star.sz, lb, KINDS[b.type].name, (long long)ws.star.sx,
           (long long)ws.star.sy, (long long)ws.star.sz, wb, b.radiusKm, seaLat / DEG, seaLon / DEG, siteLa, siteLo, m.step, m.on() ? "drawn" : "NONE");
    for (int v = 0; v < 4; v++) {
        int n = 0;
        for (size_t i = 0; i < px[0][v].size() && i < px[1][v].size(); i++) n += px[0][v][i] != px[1][v][i];
        moved += n > 0;
        printf("  %-6s %6d px changed\n", names[v], n);
    }
    std::string shows;
    for (const std::string& t : madeTraitWords(m, true)) shows += (shows.empty() ? "" : ", ") + t;
    // GLOWING SEAS: the first glowing sea species of a world of seas (the kinds' order) let go wild in its own sea where it takes, at its eighth
    // step; the ground at the coast at night facing the sea, before and after (`seed_sea_glow_<before|after>.png`)
    int glowMoved = -1;
    std::string glowWhere = "none found";
    for (int type = 0; type < PT_COUNT && glowMoved < 0; type++) {
        StarSystem ks; int kb = -1;
        if (!KINDS[type].landable || !kindHas(type, KT_SEA) || !firstBodyOfType(type, ks, kb)) continue;
        const Biota& KB = lifeOf(ks)->bodies[kb];
        int gsp = -1;
        for (const Microbe& mm : KB.species)
            if (gsp < 0 && mm.state == SS_LIVING && mm.glows && mm.niche == NI_LIQUID && mm.way != LW_NONE && mm.way != LW_PREDATOR && mm.way != LW_GRAZER) gsp = mm.index;
        if (gsp < 0) continue;
        const SeedLine glow = seedLineFor(ks, kb, gsp, {});
        double gla = 0, glo = 0, cla = 0, clo = 0;
        if (!seedPlaceTaking(ks, kb, glow, gla, glo) || !coastSite(BodyGen::make(ks.bodies[kb]), ks.bodies[kb].radiusKm, cla, clo)) continue;
        const double gpace = seedingFor(ks, kb, glow, cla * DEG, clo * DEG, 0).verdict.pace;
        // the coast's sea where it takes: the place nearest the coast's site of a few round it, else its first
        for (int k = 0; k < 16; k++) {
            const double a = k * TAU / 16, d = 4.0 / ks.bodies[kb].radiusKm;
            const double sLat = cla * DEG + d * std::cos(a), sLon = clo * DEG + d * std::sin(a) / std::max(0.05, std::cos(cla * DEG));
            if (seedVerdictOf(ks, kb, glow, sLat, sLon).takes()) { gla = sLat; glo = sLon; break; }
        }
        TestGame h;
        h.savePrefix = "shots/tests/seed_save"; h.settingsPath = "shots/tests/test_settings.txt"; h.keysPath = "shots/tests/test_keys.txt"; h.guidePath = "shots/tests/seed_guide.txt";
        remove(h.guidePath.c_str());
        h.newGame(); h.settings.cabin = false; applyTestScale(h);
        // a time the coast's site is in the night (its sun under the horizon), within a turn of the world
        double TN = 20000;
        {
            h.testSetTime(TN); h.testParkAt(ks.star, kb);
            const StarSystem& S = h.testSystem();
            const Vec3 at = StarSystem::bodyFromLatLon(cla * DEG, clo * DEG);
            double darkest = 2;
            const double turn = std::max(3600.0, std::fabs(ks.bodies[kb].rotPeriod));
            for (int k = 0; k < 96; k++) {
                const double tt = 20000 + turn * k / 96;
                const Vec3 sun = normalize(S.bodyFrame(kb, tt) * normalize(S.star.pos - S.bodyPos(kb, tt)));
                if (dot(sun, at) < darkest) { darkest = dot(sun, at); TN = tt; }
            }
        }
        const Vec3 su2 = StarSystem::bodyFromLatLon(cla * DEG, clo * DEG), wu2 = StarSystem::bodyFromLatLon(gla, glo);
        const Vec3 e2 = normalize(cross(Vec3(0, 0, 1), su2)), n2 = cross(su2, e2);
        const double yaw2 = std::atan2(dot(wu2 - su2, e2), dot(wu2 - su2, n2));
        Input hin;
        std::vector<uint32_t> gpx[2];
        auto ground = [&](const char* tag, int pass) {
            h.testSetTime(TN); h.testParkAt(ks.star, kb); h.testLandCursor(cla, clo);
            hin.pressed[KEY_ENTER] = hin.down[KEY_ENTER] = true; h.frame(hin, 1.0 / 30); hin.newFrame(); hin.down[KEY_ENTER] = false;
            for (int n = 0; n < 30 * 40 && h.state != GameState::SURFACE; n++) runFramesAtLeastOne(h, hin, 1.0 / 30);
            h.testSetYaw(yaw2); runFramesAtLeastOne(h, hin, 2.0);
            gpx[pass].assign(h.output(), h.output() + FBW * FBH);
            writePNG(fmt("shots/tests/seed_sea_glow_%s.png", tag).c_str(), h.output(), FBW, FBH);
        };
        ground("before", 0);
        SeedRecord r; r.body = Guide::bodyKey(ks.star.sx, ks.star.sy, ks.star.sz, kb); r.lat = gla; r.lon = glo; r.line = glow.key;
        r.cell = CC_WATER; r.tempC = 30; r.light = CL_DARK; r.energy = CE_BROTH; r.salt = 1; r.gen = GEN_VERSION;
        r.clock = h.guide.clock - (8 * MADE_STEP_S + 1) / gpace;
        h.guide.addSeeding(r);
        h.testClockSkip(0);
        ground("after", 1);
        glowMoved = 0;
        for (size_t i = 0; i < gpx[0].size() && i < gpx[1].size(); i++) glowMoved += gpx[0][i] != gpx[1][i];
        std::string gs;
        for (const std::string& t : madeTraitWords(h.testMadeNow(kb), true)) gs += (gs.empty() ? "" : ", ") + t;
        glowWhere = fmt("species %d of %s at %lld %lld %lld body %d, %s, the ground at night %d px changed", gsp, KINDS[type].name, (long long)ks.star.sx, (long long)ks.star.sy,
                        (long long)ks.star.sz, kb, gs.empty() ? "NOTHING" : gs.c_str(), glowMoved);
    }
    printf("seed sea: a glowing line: %s\n", glowWhere.c_str());
    printf("seed sea: %d of 4 views show it (%s)  (%.1f s)\n", moved, shows.empty() ? "NOTHING" : shows.c_str(), nowSec() - t0);
    return moved == 4 && m.on() && glowMoved != 0 ? 0 : 1;
}

int runSeed(int argc, char** argv) {
    const std::string what = argc > 2 ? argv[2] : "";
    if (what == "worlds") return seedWorlds();
    if (what == "sea") return seedSea();
    if (what == "climate") return seedClimate();
    if (what == "traits") return seedTraits();
    if (what == "looks") return seedLooks();
    if (what == "fates") return seedFates();
    if (what == "kinds") return seedKinds();
    if (what == "record") return seedRecord();
    if (what != "test") {
        printf("seed: test | worlds | sea | climate | traits | looks | fates | kinds | record (J-02: the test layer in every view, and off again; J-03: six lines on forty "
               "worlds, a seeded sea; J-04: each gas on twenty worlds, each made air's trait drawn; J-05: each made water's, ground's and plants' trait drawn; "
               "J-08: each fate of the animals and each change drawn, a seeded world's herds over its steps; J-11: the made kind each of forty worlds and six "
               "lines ends as, the shares of harm, looks and more livable, each made kind drawn; J-12: a seeded world's record in the catalogue and on the LIFE page)\n");
        return 1;
    }
    const double t0 = nowSec();
    StarSystem fs; int fb = -1;
    if (!firstBodyOfType(PT_DESERT, fs, fb)) { printf("seed test: no desert world in the scan region\n"); return 1; }
    TestGame g;
    g.savePrefix = "shots/tests/seed_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
    g.guidePath = "shots/tests/seed_guide.txt";
    remove(g.guidePath.c_str());
    g.newGame(); g.settings.cabin = false; applyTestScale(g);
    Input in;
    auto run = [&](double s) { runFramesAtLeastOne(g, in, s); };
    auto press = [&](int k) { in.pressed[k] = in.down[k] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
    // the places: the ground under the parked ship and a lit landing site
    const double T0 = 20000;
    g.testSetTime(T0); g.testParkAt(fs.star, fb); g.testLandSite();
    double subLat = 0, subLon = 0, siteLatDeg = 0, siteLonDeg = 0; int zoom = 0;
    g.testSubShip(subLat, subLon); g.testLandCursorAt(siteLatDeg, siteLonDeg, zoom);
    const Body& b = g.testSystem().bodies[fb];
    const double siteLat = siteLatDeg * DEG, siteLon = siteLonDeg * DEG;
    const Vec3 sub = StarSystem::bodyFromLatLon(subLat, subLon), site = StarSystem::bodyFromLatLon(siteLat, siteLon);
    const double smallR = 2.0 / b.radiusKm;   // 2 km round the site: the explorer stands in it, its edge toward the horizon
    const Vec3 near = site;
    auto layerOf = [&](bool tint) {
        MadeWorld m = madeTestLayer(sub, 0.45, tint ? -1 : MAT_SNOW, 0.85, RGB(0.85f, 0.25f, 0.8f));
        if (!tint) m.tinted = false;
        m.lines = 2; m.spread[1].place = near; m.spread[1].reach = smallR;
        m.key = madeKey(m);
        return m;
    };
    auto views = [&](const char* tag, SeedShots& out) {   // (the first, the warm, and the last, the same as the first, not written)
        auto shot = [&](int v) {
            out.px[v].assign(g.output(), g.output() + FBW * FBH);
            if (tag[0]) writePNG(fmt("shots/tests/seed_test_%s_%s.png", SEED_VIEWS[v], tag).c_str(), g.output(), FBW, FBH);
        };
        g.testSetTime(T0); g.testParkAt(fs.star, fb); g.testLandCursor(siteLatDeg, siteLonDeg);
        out.genKey = g.testGenMadeKey(fb);
        run(0.3); shot(0);
        press(KEY_Z); run(0.5); shot(1); press(KEY_Z); run(0.1);
        press(KEY_ESCAPE); run(0.3); shot(2);
        g.testAimAtBody(fb); press(KEY_Z); run(0.3);
        for (int i = 0; i < 10 && g.testTelescopePlateProgress() < 0; i++) { press(KEY_EQUAL); run(0.05); }
        g.testTelescopeSettle(); shot(3);
        press(KEY_ESCAPE); run(0.1);
        g.testSetTime(T0 + 300); g.testParkAt(fs.star, fb); g.testLandCursor(siteLatDeg, siteLonDeg);
        press(KEY_ENTER);
        for (int n = 0; n < 30 * 40 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
        g.testSetYaw(PI / 2); run(2.0); shot(4);
        out.siteKey = g.testSiteMadeKey();
    };
    SeedShots warm, none, tint, cover, after;
    views("", warm);   // every cache and tile the views ask for built once (the drainage's tiles, the plate's rivers)
    views("none", none);
    g.testMadeSet(fb, layerOf(true));
    views("tint", tint);
    g.testMadeSet(fb, layerOf(false));
    views("cover", cover);
    g.testMadeClear();
    views("", warm);    // the telescope's slots as the first pictures had them (a plate of another layer is no one's fallback)
    views("", after);
    printf("seed test: the star at %lld %lld %lld, body %d (%s, R %.0f km); the ground under the ship %.2f %.2f, the landing site %.3f %.3f\n", (long long)fs.star.sx, (long long)fs.star.sy,
            (long long)fs.star.sz, fb, KINDS[b.type].name, b.radiusKm, subLat / DEG, subLon / DEG, siteLatDeg, siteLonDeg);
    int drewTint = 0, drewCover = 0, same = 0;
    for (int v = 0; v < SEED_NVIEWS; v++) {
        const int dt = pixelsDiffering(none.px[v], tint.px[v]), dc = pixelsDiffering(none.px[v], cover.px[v]), da = pixelsDiffering(none.px[v], after.px[v]);
        drewTint += dt > 0; drewCover += dc > 0; same += da == 0;
        printf("  %-6s the tint moved %6d px, the cover %6d px; taken off, %d px differ from the first%s\n", SEED_VIEWS[v], dt, dc, da, da ? "  NOT AS BEFORE" : "");
    }
    printf("  the generators' layers: %s before, %s with the tint, %s with the cover, %s after; the ground's: %s, %s, %s, %s\n", none.genKey ? "ON" : "none", tint.genKey ? "on" : "NONE",
            cover.genKey ? "on" : "NONE", after.genKey ? "ON" : "none", none.siteKey ? "ON" : "none", tint.siteKey ? "on" : "NONE", cover.siteKey ? "on" : "NONE", after.siteKey ? "ON" : "none");
    const bool ok = drewTint == SEED_NVIEWS && drewCover == SEED_NVIEWS && same == SEED_NVIEWS && !none.genKey && tint.genKey && cover.genKey && !after.genKey && !none.siteKey &&
                    tint.siteKey && cover.siteKey && !after.siteKey;
    printf("seed test: %d of %d views drew the tint, %d the cover, %d drew as before once it was off: %s  (%.1f s)\n", drewTint, SEED_NVIEWS, drewCover, same, ok ? "ok" : "FAILED",
            nowSec() - t0);
    return ok ? 0 : 1;
}
