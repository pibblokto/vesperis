// J-08 (R-448): the animals' fates' mode. `seed fates`: each fate a line can bring a world's animals, let go on the first world (of those with
// their own sea line, animals and plants) where it brings it, its layer taken once the remains' age has run out; what each species came to,
// printed. Each fate drawn on that world's ground (`shots/tests/fates/fate_<tag>.png`: a die-off's remains where the cells laid them, each
// sickness on a herd of the sick, glowing animals at night, dyed hides, changed young, thriving, silence); the five stages of remains
// (`fates/remains.png`) and the remains of each of a world's kinds (`fates/remains_kinds.png`); each change on a young beside one born whole
// (`fates/change_<name>.png`, all of them small in `fates/changes.png`); a seeded world's herd over its steps (`fates/steps_<NN>.png`)
#include "harness.h"
#include "life/seeding.h"
#include <algorithm>
#include <sys/stat.h>

namespace {
// a fate's recipe: its file's tag, the trait it brings (and the death it must be of, -1 any), the genes set (`seedGeneEdits`; a host of
// animals' weight set to its depth, 0 the skin to 3 the cells), the line (a glowing one, a spore former: else the world's own of the sea),
// how it is drawn (0 a die-off's remains where the cells laid them, 1 a herd of the sick, 2 changed young, 3 the cells' herds: thriving, silence),
// at night
struct FateRecipe {
    const char* tag;
    int trait, death;
    std::vector<std::pair<int, int>> genes;
    int depth;
    bool glow, spores;
    int show;
    bool night;
};
std::vector<FateRecipe> fateRecipes() {
    return {
        {"nerve_poison", MF_CRAZED, -1, {{GN_POISON, PZ_NERVE}}, -1, false, false, 1, false},
        {"blood_poison", MF_DIE_OFF, MD_BLOOD, {{GN_POISON, PZ_BLOOD}}, -1, false, false, 0, false},
        {"gut_poison", MF_WASTING, -1, {{GN_POISON, PZ_GUT}}, -1, false, false, 1, false},
        {"sulphide_air", MF_DIE_OFF, MD_SULPHIDE, {{GN_GAS, WG_H2S}}, -1, false, false, 0, false},
        {"glowing", MF_GLOWING, -1, {{GN_HOST, HO_ANIMALS}}, 0, true, false, 1, true},
        {"dyed_hides", MF_DYED, -1, {{GN_HOST, HO_ANIMALS}, {GN_DYE, WD_RED}}, 0, false, false, 1, false},
        {"gut_host", MF_WASTING, -1, {{GN_HOST, HO_ANIMALS}}, 1, false, false, 1, false},
        {"bloated", MF_BLOATED, -1, {{GN_HOST, HO_ANIMALS}, {GN_GAS, WG_HYDROGEN}}, 2, false, false, 1, false},
        {"growths", MF_GROWTHS, -1, {{GN_HOST, HO_ANIMALS}, {GN_STOP, -2}}, 2, false, false, 1, false},
        {"stone_skin", MF_STONE_SKIN, -1, {{GN_HOST, HO_ANIMALS}, {GN_SHELL, SHL_CHALK}}, 2, false, false, 1, false},
        {"melting", MF_MELTING, -1, {{GN_HOST, HO_ANIMALS}, {GN_ROT, ROT_FLESH}}, 2, false, false, 1, false},
        {"sprouting", MF_SPROUTING, -1, {{GN_HOST, HO_ANIMALS}, {GN_BIND, -1}}, 2, false, true, 1, false},
        {"changed_young", MF_CHANGED_YOUNG, -1, {{GN_HOST, HO_ANIMALS}, {GN_CARRIER, -1}}, 3, false, false, 2, false},
        {"thin_ozone", MF_CHANGED_YOUNG, -1, {{GN_GAS, WG_N2O}}, -1, false, false, 2, false},
        {"flesh_rotter", MF_DIE_OFF, MD_ROT_FLESH, {{GN_ROT, ROT_FLESH}}, -1, false, false, 0, false},
        {"shell_rotter", MF_DIE_OFF, MD_ROT_SHELL, {{GN_ROT, ROT_SHELL}}, -1, false, false, 0, false},
        {"soup_seas", MF_DIE_OFF, MD_STARVED, {{GN_STOP, -2}}, -1, false, false, 0, false},
        {"blight", MF_DIE_OFF, MD_STARVED, {{GN_ROT, ROT_WOOD}}, -1, false, false, 0, false},
        {"heat", MF_DIE_OFF, MD_HEAT, {{GN_GAS, WG_CO2}}, -1, false, false, 0, false},
        {"cold", MF_DIE_OFF, MD_COLD, {{GN_GAS, WG_DMS}}, -1, false, false, 0, false},
        {"ammonia_seas", MF_DIE_OFF, MD_AMMONIA, {{GN_GAS, WG_AMMONIA}}, -1, false, false, 0, false},
        {"thriving", MF_THRIVING, -1, {{GN_GAS, WG_OXYGEN}}, -1, false, false, 3, false},
        {"silence", MF_SILENCE, -1, {{GN_GAS, WG_CO2}, {GN_POISON, PZ_BLOOD}}, -1, false, false, 3, false},
    };
}

// the worlds: those of `seed climate`'s with animals and plants (each with its own sea line)
std::vector<SeedAirCase> fateCases() {
    std::vector<SeedAirCase> out;
    for (const SeedAirCase& c : seedAirCases(36))
        if (kindHas(c.sys.bodies[c.body].type, KT_FAUNA) && kindHas(c.sys.bodies[c.body].type, KT_FLORA)) out.push_back(c);
    return out;
}

// the recipe's line on a world: its own of the sea, a glowing one or a spore former of its waters; the recipe's genes set, a host's weight stepped
// from the lightest till it lives at the depth wanted
bool fateLine(const SeedAirCase& c, const FateRecipe& R, SeedLine& line) {
    SeedLine wild = c.wild;
    if (R.glow || R.spores) {
        const Biota& B = lifeOf(c.sys)->bodies[c.body];
        int sp = -1;
        for (const Microbe& m : B.species)
            if (sp < 0 && m.state == SS_LIVING && m.niche == NI_LIQUID && m.way != LW_NONE && (R.glow ? m.glows : m.sporeYears > 0)) sp = m.index;
        if (sp < 0) return false;
        wild = seedLineFor(c.sys, c.body, sp, {});
    }
    if (!wild.valid) return false;
    std::vector<GeneEdit> edits = seedGeneEdits(wild.genes, R.genes);
    if (R.depth >= 0) {
        const GeneCode code = geneCodeOf(wild.genes.genesis);
        for (GeneEdit& e : edits) {
            const MicrobeGene* G = nullptr;
            for (const MicrobeGene& x : wild.genes.genes) if (x.place == e.place) { G = &x; break; }
            if (!G || G->kind != GN_HOST || e.part.size() < 2) continue;
            const std::string run = e.part.substr(0, 2), light = lighter(code, G->part.substr(2), 99);
            bool found = false;
            for (int steps = 0; steps < 80 && !found; steps++) {
                GeneEdit t = e; t.part = run + heavier(code, light, steps);
                std::vector<GeneEdit> tried = edits;
                for (GeneEdit& u : tried) if (u.place == e.place) u = t;
                const SeedLine l = seedLineFor(c.sys, c.body, wild.kind.s.index, tried);
                const int d = l.valid && l.kind.world.hosted() ? clampi((int)(l.kind.world.hostRate * 4), 0, 3) : -1;
                if (d == R.depth) { e = t; found = true; }
            }
            if (!found) return false;
        }
    }
    line = seedLineFor(c.sys, c.body, wild.kind.s.index, edits);
    line.key += "/S1";   // (the key the game reads a strain by: the drift in the wild hashes it)
    return line.valid;
}

// a world's layer under a line let go at its sea's first place where it takes: at the end of its spread and on till the remains' age runs out
// (`steps` on instead: the layer at that step)
bool fateLayer(const SeedAirCase& c, const SeedLine& line, Seeding& sd, MadeWorld& m, int steps = -1) {
    double lat = c.lat, lon = c.lon;
    if (!seedPlaceTaking(c.sys, c.body, line, lat, lon)) return false;
    sd = seedingFor(c.sys, c.body, line, lat, lon, 0);
    if (!sd.verdict.takes()) return false;
    const int at = steps >= 0 ? steps : MADE_STEPS + MADE_FATE_STEPS;
    m = madeWorldOf(c.sys, c.body, {sd}, at * MADE_STEP_S / sd.verdict.pace + 1);
    return true;
}

std::string pct(double v) { return fmt("%.0f%%", 100 * v); }

// a view of a world's ground under a layer at a place, the explorer far off, the capsule out of the way; its stars
struct FateView {
    SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
    StarSystem sys;
    double t = 0;
    Input in;
    bool init(const StarSystem& s, int body, const MadeWorld& m, int need, bool night, int salt = 0) {
        sys = s;
        double lat, lon;
        if (!waysSite(sys, body, need, night, lat, lon, t, salt)) return false;
        sv.site.made = m;
        sv.init(&sys, body, lat, lon, t);
        nb.update(sv.site.worldPos(t, 0, 0, 0));
        sv.relocateCapsule(0, -300);
        sv.player.x = 0; sv.player.z = -400; sv.player.y = sv.site.surfaceHeight(0, -400);   // far off: none takes fright
        return true;
    }
    // at a given place and time
    bool initAt(const StarSystem& s, int body, const MadeWorld& m, double lat, double lon, double when) {
        sys = s; t = when;
        sv.site.made = m;
        sv.init(&sys, body, lat, lon, t);
        nb.update(sv.site.worldPos(t, 0, 0, 0));
        sv.relocateCapsule(0, -300);
        sv.player.x = 0; sv.player.z = -400; sv.player.y = sv.site.surfaceHeight(0, -400);
        return true;
    }
    void step(int frames) { for (int f = 0; f < frames; f++) { t += 1.0 / 30; sv.update(1.0 / 30, in, t, false); } }
    void cam(double cx, double cy, double cz, double lx, double ly, double lz) {
        sv.freeCam = true; sv.freePos = Vec3(cx, std::max(cy, sv.site.groundHeight(cx, cz) + 0.4), cz);
        sv.freeYaw = std::atan2(lx - cx, lz - cz);
        sv.freePitch = std::atan2(ly - sv.freePos.y, std::hypot(lx - cx, lz - cz));
    }
    // a shot: the explorer 40 m behind the camera first (the ground's rings centre on them: far off, the ground by the camera is a coarse
    // ring's, and low remains sink under it), then warm-up frames
    void shot(const std::string& fn) {
        sv.player.x = sv.freePos.x - std::sin(sv.freeYaw) * 40; sv.player.z = sv.freePos.z - std::cos(sv.freeYaw) * 40;
        sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
        for (int k = 0; k < 3; k++) { sv.update(1.0 / 30, in, t, false); Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0); }
        waysShot(sv, nb, sr, t, fn);
    }
    // the camera on a cluster of points (each with its animal's size): from the south-west, off by the cluster's spread and its biggest animal
    void frame(const std::vector<Vec3>& pts) {
        if (pts.empty()) return;
        double cx = 0, cz = 0, big = 0;
        for (const Vec3& p : pts) { cx += p.x / pts.size(); cz += p.z / pts.size(); big = std::max(big, p.y); }
        double spread = 0;
        for (const Vec3& p : pts) spread = std::max(spread, std::hypot(p.x - cx, p.z - cz));
        const double d = std::max(1.5, 1.0 * spread + 3.0 * big + 0.6), gy = sv.site.groundHeight(cx, cz);
        cam(cx - 0.55 * d, gy + 0.35 * d + 0.5 * big, cz - 0.83 * d, cx, gy + 0.25 * big, cz);
    }
    // the three nearest a point within 6 m of it (x, size, z), the point's own first
    static std::vector<Vec3> nearest(std::vector<Vec3> pts, double x, double z) {
        std::sort(pts.begin(), pts.end(), [&](const Vec3& a, const Vec3& b) { return std::hypot(a.x - x, a.z - z) < std::hypot(b.x - x, b.z - z); });
        std::vector<Vec3> out;
        for (const Vec3& p : pts) if (out.size() < 3 && std::hypot(p.x - x, p.z - z) < 6) out.push_back(p);
        return out;
    }
    // the cells' herds and remains round the place (their cells within reach of the explorer, put at the place for it)
    void cells() {
        sv.player.x = 0; sv.player.z = 0; sv.player.y = sv.site.surfaceHeight(0, 0);
        sv.habitatT = -1e9;
        step(3);
        sv.player.x = 0; sv.player.z = -400;
        sv.habitatT = 1e18;   // (held as they are)
    }
};

// a site by a seeding's place under a layer (its made climate moves the snow, and the herds off it): the nearest land (rings out to the far
// side, a point 300 m in from its shore, 300 tried at most) whose sun
// stands 25 to 55 degrees up at some time of two days (12 under at night: `t` that time) and, where `herd` is wanted, a habitat cell within 700 m
// holding a herd of four or more (of a species in `dying` where given; within 120 m of water where `shore`; else the first lit land); false
// where there is none
bool siteNear(StarSystem& sys, int body, const MadeWorld& made, double lat0, double lon0, bool night, bool herd, double& lat, double& lon, double& t,
              const std::vector<int>* dying = nullptr, bool shore = false) {
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    auto dry = [&](double la, double lo) {
        const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 16);
        return !(madeWetAt(s) || s.material == MAT_SNOW || s.material == MAT_ICE || s.material == MAT_LAVA);
    };
    const double day = std::max(600.0, std::fabs(b.rotPeriod));
    bool haveLit = false, haveSmall = false;
    double litLat = 0, litLon = 0, litT = 0, smallLat = 0, smallLon = 0, smallT = 0;   // (the first lit land; the first with a herd of small ones)
    int tried = 0;
    for (double km = 1; km <= PI * b.radiusKm && tried < 300; km *= 1.25)
        for (int k = 0; k < 24 && tried < 300; k++) {
            const double a = k * TAU / 24, c = std::max(0.05, std::cos(lat0));
            const double d = km / b.radiusKm, la = lat0 + d * std::cos(a), lo = lon0 + d * std::sin(a) / c;
            if (!dry(la, lo)) continue;
            const double in = (km + 0.3) / b.radiusKm, la2 = lat0 + in * std::cos(a), lo2 = lon0 + in * std::sin(a) / c;
            const bool inland = !shore && dry(la2, lo2);   // (by a shore: the coast itself)
            const double sla = inland ? la2 : la, slo = inland ? lo2 : lo;
            tried++;
            SurfaceSite site; site.made = made; site.init(&sys, body, sla, slo, 1000.0);
            double tt = -1;
            for (int q = 0; q < 192 && tt < 0; q++) {
                const double tq = 1000 + day * 2 * q / 192;
                const SunInfo si = site.sun(tq);
                if (night ? si.altitude < -12 * DEG : (si.altitude > 25 * DEG && si.altitude < 55 * DEG)) tt = tq;
            }
            if (tt < 0) continue;
            if (!haveLit) { haveLit = true; litLat = sla; litLon = slo; litT = tt; }
            if (!herd) { lat = sla; lon = slo; t = tt; return true; }
            const Bestiary B = Bestiary::make(b, site.gen);
            int gLat0, gLon0;
            SurfaceView::lifeCellAt(site, 0, 0, gLat0, gLon0);
            for (int dl = -2; dl <= 2; dl++)
                for (int dn = -3; dn <= 3; dn++) {
                    SurfaceView::Herd hd; int spi, n;
                    if (!SurfaceView::planCellHerd(site, B, gLat0 + dl, gLon0 + dn, hd, spi, n) || n < 4 || std::hypot(hd.cx, hd.cz) > 700) continue;
                    if (dying && std::find(dying->begin(), dying->end(), spi) == dying->end()) continue;
                    if (shore) {
                        bool wet = false;
                        for (int q = 0; q < 24 && !wet; q++) {   // (as the cell's remains read it: the planet function at 16 m)
                            const TerrainVertex v = site.sampleAt(hd.cx + std::sin(q * TAU / 12) * (q < 12 ? 50.0 : 110.0), hd.cz + std::cos(q * TAU / 12) * (q < 12 ? 50.0 : 110.0), 16);
                            wet = v.material == MAT_WATER || (v.water > -1e8f && v.h < v.water + 0.3);
                        }
                        if (!wet) continue;
                    }
                    if (B.species[spi].size < 0.5) {   // (big enough to see, else kept as a fallback)
                        if (!haveSmall) { haveSmall = true; smallLat = sla; smallLon = slo; smallT = tt; }
                        continue;
                    }
                    lat = sla; lon = slo; t = tt;
                    return true;
                }
        }
    if (haveSmall) { lat = smallLat; lon = smallLon; t = smallT; return true; }
    if (!haveLit) return false;
    lat = litLat; lon = litLon; t = litT;
    return true;
}
// B-471: a coast by the place where the sea's dead wash up: along each of 24 rays out from it every coast (the water's end, marched, then halved
// to 20 m), the nearest first, lit as `siteNear` wants it over two of its solar days (a world turned to its sun keeps its night: its lit
// coasts lie on its far side), with a habitat cell within 600 m whose herd's spot (its middle where it holds none) has the water within 150 m,
// as `cellRemains` reads its shore (the planet function at 16 m); false where there is none
bool coastNear(StarSystem& sys, int body, const MadeWorld& made, double lat0, double lon0, bool night, double& lat, double& lon, double& t) {
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    auto dry = [&](double la, double lo) {
        const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 16);
        return !(madeWetAt(s) || s.material == MAT_SNOW || s.material == MAT_ICE || s.material == MAT_LAVA);
    };
    const double turn = 1 / b.rotPeriod - 1 / std::max(1.0, b.orbitPeriod);   // (a solar day: its turn against its year; a slow one's long)
    const double c = std::max(0.05, std::cos(lat0)), day = std::max(600.0, std::min(1e9, std::fabs(1 / (std::fabs(turn) > 1e-12 ? turn : 1e-12))));
    auto at = [&](double a, double d, double& la, double& lo) { la = lat0 + d / b.radiusKm * std::cos(a); lo = lon0 + d / b.radiusKm * std::sin(a) / c; };
    std::vector<std::pair<double, double>> coasts;   // (its distance, its ray)
    for (int k = 0; k < 24; k++) {
        const double a = k * TAU / 24;
        double la, lo, wetKm = -1;
        for (double km = 0.1; km < PI * b.radiusKm; km += km < 2 ? 0.1 : (km < 20 ? 0.25 : std::max(1.0, 0.01 * km))) {
            at(a, km, la, lo);
            if (!dry(la, lo)) { wetKm = km; continue; }
            if (wetKm < 0) continue;   // (inland: the next coast on)
            double lo2 = wetKm, hi = km;
            for (int q = 0; q < 6; q++) { const double m = 0.5 * (lo2 + hi); at(a, m, la, lo); (dry(la, lo) ? hi : lo2) = m; }
            coasts.push_back({hi, a});
            wetKm = -1;
        }
    }
    std::sort(coasts.begin(), coasts.end());
    for (const auto& [km, a] : coasts) {
        double la, lo;
        at(a, km + 0.02, la, lo);
        SurfaceSite site; site.made = made; site.init(&sys, body, la, lo, 1000.0);
        double tt = -1;
        for (int q = 0; q < 192 && tt < 0; q++) {
            const double tq = 1000 + day * 2 * q / 192;
            const SunInfo si = site.sun(tq);
            if (night ? si.altitude < -12 * DEG : (si.altitude > 25 * DEG && si.altitude < 55 * DEG)) tt = tq;
        }
        if (tt < 0) continue;
        const Bestiary B = Bestiary::make(b, site.gen);
        int gLat0, gLon0;
        SurfaceView::lifeCellAt(site, 0, 0, gLat0, gLon0);
        for (int dl = -2; dl <= 2; dl++)
            for (int dn = -3; dn <= 3; dn++) {
                SurfaceView::Herd hd; int spi, n;
                if (!SurfaceView::planCellHerd(site, B, gLat0 + dl, gLon0 + dn, hd, spi, n)) {
                    const double dLat = 512.0 / site.R, latc = (gLat0 + dl + 0.5) * dLat, dLon = dLat / std::max(std::cos(latc), 0.05);
                    site.localAt(StarSystem::bodyFromLatLon(latc, (gLon0 + dn + 0.5) * dLon), hd.cx, hd.cz);
                }
                if (std::hypot(hd.cx, hd.cz) > 600) continue;
                for (int q = 0; q < 12; q++)
                    for (double d = 8; d <= 150; d += 12) {
                        const TerrainVertex v = site.sampleAt(hd.cx + std::sin(q * TAU / 12) * d, hd.cz + std::cos(q * TAU / 12) * d, 16);
                        if (v.material == MAT_WATER || (v.water > -1e8f && v.h < v.water + 0.3)) { lat = la; lon = lo; t = tt; return true; }
                    }
            }
    }
    return false;
}
// remains counted by stage within a reach of a point
std::string remainsCount(const SurfaceView& sv, double x, double z, double within) {
    int n[RS_COUNT] = {};
    for (const Remains& r : sv.remains) if (std::hypot(r.x - x, r.z - z) < within) n[clampi(r.stage, 0, RS_COUNT - 1)]++;
    std::string s;
    for (int k = RS_FRESH; k < RS_COUNT; k++) if (n[k]) s += fmt("%s%d %s", s.empty() ? "" : ", ", n[k], REMAINS_NAMES[k]);
    return s.empty() ? "no remains" : s;
}
// the first land species with legs (a quadruped's first), else the first land one
int firstWalker(const Bestiary& B) {
    for (int i = 0; i < B.landCount; i++) if (B.species[i].plan == PLAN_QUADRUPED) return i;
    for (int i = 0; i < B.landCount; i++) if (B.species[i].body.pairs >= 2) return i;
    return B.landCount ? 0 : -1;
}
std::vector<uint32_t> frameOf(FateView& V) {
    V.sv.player.x = V.sv.freePos.x - std::sin(V.sv.freeYaw) * 40; V.sv.player.z = V.sv.freePos.z - std::cos(V.sv.freeYaw) * 40;
    V.sv.player.y = V.sv.site.surfaceHeight(V.sv.player.x, V.sv.player.z);
    for (int k = 0; k < 2; k++) { V.sv.update(1.0 / 30, V.in, V.t, false); Framebuffer fb0; V.sv.render(fb0, V.t, V.nb.stars, V.sr, 1.0); }
    Framebuffer fb; V.sv.render(fb, V.t, V.nb.stars, V.sr, 1.0); fb.mush(2);
    std::vector<uint32_t> px((size_t)FBW * FBH);
    fb.toRGB(px.data(), 1.0);
    return px;
}
std::string lowerTag(const char* s) { std::string t; for (const char* p = s; *p; p++) t += *p == ' ' || *p == '-' ? '_' : (char)std::tolower((unsigned char)*p); return t; }
}   // namespace

// a sheet of cells side by side (each a frame halved: the mean of four), `cols` to a row
void writeSheet(const std::vector<std::vector<uint32_t>>& cells, int w, int h, int cols, const std::string& fn) {
    if (cells.empty()) return;
    const int cw = w / 2, ch = h / 2, rows = ((int)cells.size() + cols - 1) / cols;
    std::vector<uint32_t> out((size_t)cw * cols * ch * rows, 0xFF000000u);
    for (size_t k = 0; k < cells.size(); k++) {
        const int ox = (int)(k % cols) * cw, oy = (int)(k / cols) * ch;
        for (int y = 0; y < ch; y++)
            for (int x = 0; x < cw; x++) {
                uint32_t r = 0, g = 0, b = 0;
                for (int dy = 0; dy < 2; dy++)
                    for (int dx = 0; dx < 2; dx++) {
                        const uint32_t p = cells[k][(size_t)(2 * y + dy) * w + 2 * x + dx];
                        r += p & 0xFF; g += (p >> 8) & 0xFF; b += (p >> 16) & 0xFF;
                    }
                out[(size_t)(oy + y) * cw * cols + ox + x] = 0xFF000000u | (r / 4) | ((g / 4) << 8) | ((b / 4) << 16);
            }
    }
    writePNG(fn.c_str(), out.data(), cw * cols, ch * rows);
}

int seedFates() {
    const double t0 = nowSec();
    mkdir("shots/tests/fates", 0755);
    const int savedScale = FB_SCALE;
    setFramebufferScale(2);
    struct DrainOff { bool was; DrainOff() : was(drainageEnabled()) { setDrainageEnabled(false); } ~DrainOff() { setDrainageEnabled(was); } } drainOff;
    const std::vector<SeedAirCase> worlds = fateCases();
    printf("seed fates: %zu worlds with animals, plants and a sea line\n", worlds.size());
    int found = 0, drawn = 0;
    std::string missing;
    const SeedAirCase* firstWorld = nullptr, *dyedWorld = nullptr;
    MadeWorld firstLayer, dyedLayer;
    Seeding firstSeeding;
    for (const FateRecipe& R : fateRecipes()) {
        const SeedAirCase* W = nullptr;
        MadeWorld m;
        Seeding sd;
        SeedLine line;
        int tried = 0, brought = 0;
        for (const SeedAirCase& c : worlds) {
            if (!fateLine(c, R, line)) continue;
            MadeWorld mm; Seeding ss;
            if (!fateLayer(c, line, ss, mm)) continue;
            tried++;
            const bool ok = (mm.fauna & R.trait) && (R.death < 0 || mm.fate.death == R.death || std::any_of(mm.spread, mm.spread + mm.lines, [&](const MadeSpread& s) { return s.fate.death == R.death; })) &&
                            (R.trait == MF_DIE_OFF || R.trait == MF_SILENCE || !(mm.fauna & MF_SILENCE));   // (a sickness on a world with some living)
            if (!ok) continue;
            brought++;
            if (!W) { W = &c; m = mm; sd = ss; }
        }
        if (!W) { missing += fmt("%s%s", missing.empty() ? "" : ", ", R.tag); printf("  %-14s brought by none of the %d worlds its line took on\n", R.tag, tried); continue; }
        found++;
        const Body& b = W->sys.bodies[W->body];
        std::string traits;
        for (int i = 0; i < MADE_FAUNA_TRAITS; i++) if ((m.fauna >> i) & 1) traits += fmt("%s%s", traits.empty() ? "" : ", ", MADE_FAUNA_NAMES[i]);
        const MadeFate here = madeFateAt(m, StarSystem::bodyFromLatLon(sd.lat, sd.lon), b.seed);
        printf("  %-14s %s at %lld %lld %lld body %d (%d of %d worlds): %s; killed by %s, land %s air %s water %s%s%s\n", R.tag, KINDS[b.type].name,
               (long long)W->sys.star.sx, (long long)W->sys.star.sy, (long long)W->sys.star.sz, W->body, brought, tried, traits.c_str(), MADE_DEATH_NAMES[here.death],
               pct(here.harm[MFC_LAND]).c_str(), pct(here.harm[MFC_AIR]).c_str(), pct(here.harm[MFC_WATER]).c_str(),
               here.sick > 0 ? fmt(", %s of the rest show it", pct(here.sick).c_str()).c_str() : "", here.changed > 0 ? fmt(", %s of the young born changed", pct(here.changed).c_str()).c_str() : "");
        const Bestiary B = Bestiary::make(b, BodyGen::make(b));
        int steer = m.fate.steer;
        for (int i = 0; i < m.lines; i++) steer |= m.spread[i].fate.steer;
        for (int i = 0; i < (int)B.species.size(); i++) {
            int ch[3]; const int nc = animalChangesOf(B.species[i], steer, ch);
            const AnimalFate af = animalFateOf(B.species[i], here, ch, nc);
            std::string changes;
            for (int k = 0; k < af.nChanges; k++) changes += fmt("%s%s", changes.empty() ? "" : ", ", CHANGE_NAMES[af.changes[k]]);
            printf("      %-26s %-9s %s dead%s%s%s\n", B.words(i).c_str(), ANIMAL_WAY_NAMES[B.species[i].way], pct(af.dead).c_str(),
                   af.sick > 0 ? fmt(", %s of the rest %s", pct(af.sick).c_str(), MADE_FAUNA_NAMES[__builtin_ctz((unsigned)af.sickness)]).c_str() : "",
                   af.changed > 0 ? fmt(", %s born changed (%s)", pct(af.changed).c_str(), changes.c_str()).c_str() : "", af.thrive > 0 ? fmt(", thriving %s", pct(af.thrive).c_str()).c_str() : "");
        }
        if (!firstWorld && R.show == 0) { firstWorld = W; firstLayer = m; firstSeeding = sd; }
        if (!dyedWorld && R.trait == MF_DYED) { dyedWorld = W; dyedLayer = m; }
        // its frame on that world's ground: a sickness or changed young anywhere at the end; a die-off by the place, at its step 16 (the dead
        // from fresh to bones)
        FateView V;
        bool placed = false, watersOnly = false;
        if (R.show == 0 || R.show == 3) {   // (twelve steps after its spread reached the site, the dead from fresh to bones)
            double la, lo, when;
            StarSystem ws = W->sys;
            std::vector<int> dying;   // (the land species that die there; with none, a herd by a shore its swimmers wash up on)
            for (int i = 0; i < B.landCount; i++) {
                int ch[3]; const int nc = animalChangesOf(B.species[i], steer, ch);
                if (animalFateOf(B.species[i], here, ch, nc).dead >= 0.2) dying.push_back(i);
            }
            watersOnly = dying.empty() && R.trait != MF_THRIVING;
            const bool near = watersOnly ? coastNear(ws, W->body, m, sd.lat, sd.lon, R.night, la, lo, when)   // (a coast its swimmers wash up on)
                                         : siteNear(ws, W->body, m, sd.lat, sd.lon, R.night, true, la, lo, when, R.trait == MF_THRIVING ? nullptr : &dying);
            if (!near && watersOnly) {   // (no herd by a shore near: a shore walked to from the sea, the spread over the world by then)
                const MadeWorld late = madeWorldOf(W->sys, W->body, {sd}, 26 * MADE_STEP_S / sd.verdict.pace + 1);
                placed = V.init(W->sys, W->body, late, 3, R.night);
            } else if (near) {
                const double d = std::acos(clampd(dot(StarSystem::bodyFromLatLon(sd.lat, sd.lon), StarSystem::bodyFromLatLon(la, lo)), -1, 1));
                int arrive = 0;
                while (arrive < MADE_STEPS && madeReachOfStep(arrive, b.radiusKm) < d) arrive++;
                const MadeWorld mid = madeWorldOf(W->sys, W->body, {sd}, std::max(16, arrive + 12) * MADE_STEP_S / sd.verdict.pace + 1);
                placed = V.initAt(W->sys, W->body, mid, la, lo, when);
            }
        } else placed = V.init(W->sys, W->body, m, 0, R.night);
        if (!placed) { printf("      no land in the light to draw it on (the place at %.1f %.1f)\n", sd.lat / DEG, sd.lon / DEG); continue; }
        SurfaceView& sv = V.sv;
        const std::string fn = fmt("shots/tests/fates/fate_%s.png", R.tag);
        std::string what;
        if (R.show == 1 || R.show == 2) {   // a herd of the sick (each the recipe's sickness), or of changed young (its species' changes)
            sv.critters.clear(); sv.flocks.clear(); sv.herds.clear(); sv.remains.clear(); sv.habitatT = 1e18;
            int spi = R.show == 1 ? firstWalker(sv.bestiary) : -1;
            for (int i = 0; i < sv.bestiary.landCount && spi < 0; i++) if (sv.speciesChanges[i].n > 0 && sv.bestiary.species[i].body.pairs >= 2) spi = i;
            for (int i = 0; i < sv.bestiary.landCount && spi < 0; i++) if (sv.speciesChanges[i].n > 0) spi = i;
            if (spi < 0) spi = firstWalker(sv.bestiary);
            if (spi < 0) { printf("      no land animal to draw it on\n"); continue; }
            const Species& sp = sv.bestiary.species[spi];
            const double gap = 1.15 * sp.body.length + 0.5 * sp.body.neck + 0.3 * sp.body.tailLen + 0.4, near = std::max(3.0, 2.4 * gap + 1.5 * sp.size);
            {   // four side by side, side on, standing a while
                SurfaceView::Herd hd; hd.cx = 0; hd.cz = near; hd.origin = true;
                sv.herds.push_back(hd);
                Rng r(mix64(b.seed ^ 0xFA7EULL));
                for (int i = 0; i < 4; i++) {
                    Critter c;
                    c.herd = 0; c.species = spi; c.x = (i - 1.5) * gap; c.z = near + (i & 1 ? 0.25 : -0.25) * gap;
                    c.heading = PI / 2 + r.sym(0.25); c.timer = 30; c.size = sp.size * (0.95 + 0.1 * r.uni()); c.seed = r.next(); c.state = CS_IDLE;
                    c.homeX = c.x; c.homeZ = c.z;
                    sv.critters.push_back(c);
                }
            }
            int sickness = 0;
            for (int i = 0; i < MADE_FAUNA_TRAITS && !sickness; i++) if (((m.fauna >> i) & 1) && ((1 << i) & MF_SICKNESSES)) sickness = 1 << i;
            for (Critter& c : sv.critters) {
                if (R.show == 1) c.sick = sickness;
                else c.changed = true;
            }
            if (R.night) sv.lamp = true;
            V.step(8);
            if (sickness == MF_CRAZED && !sv.critters.empty()) { sv.critters[0].fallT = 1.0; sv.critters[0].lift = 1; }   // one of them down
            V.cam(0, sv.site.groundHeight(0, 0) + 0.5 + 0.7 * sp.size, 0, 0, sv.site.groundHeight(0, near) + 0.45 * sp.size, near);
            const SurfaceView::SpeciesChanges& sc = sv.speciesChanges[spi];
            std::string changes;
            for (int k = 0; k < sc.n; k++) changes += fmt("%s%s", changes.empty() ? "" : ", ", CHANGE_NAMES[sc.changes[k]]);
            what = R.show == 1 ? fmt("four %s %s", sv.bestiary.words(spi).c_str(), sickness ? MADE_FAUNA_NAMES[__builtin_ctz((unsigned)sickness)] : "WELL")
                               : fmt("four %s born changed (%s)", sv.bestiary.words(spi).c_str(), changes.c_str());
        } else {   // the cells' herds and remains round the place: framed at the nearest remains (thriving: the nearest herd)
            V.cells();
            double bx = 0, bz = 0, best = 1e18;
            if (R.trait == MF_THRIVING) {
                for (const Critter& c : sv.critters) { const double d = std::hypot(c.x, c.z); if (d < best) { best = d; bx = c.x; bz = c.z; } }
            } else {
                for (const Remains& r : sv.remains) {   // (the sea's dead alone: its swimmers washed up)
                    if (watersOnly && (r.species < sv.bestiary.firstSwimmer() || r.old)) continue;
                    int near = 0;
                    for (const Remains& o : sv.remains) near += std::hypot(o.x - r.x, o.z - r.z) < 25;
                    const double d = std::hypot(r.x, r.z) - 40.0 * near;   // (nearest with the most round it)
                    if (d < best) { best = d; bx = r.x; bz = r.z; }
                }
            }
            if (best > 1e17) {
                printf("      nothing near to frame (%zu animals, %zu remains; the layer's step %d, age %d, fauna %d, key %llx)\n", sv.critters.size(), sv.remains.size(),
                       sv.site.gen.made.step, sv.site.gen.made.fateAge, sv.site.gen.made.fauna, (unsigned long long)sv.site.gen.made.key);
                continue;
            }
            std::vector<Vec3> pts;   // (x, its size, z)
            if (R.trait == MF_THRIVING) { for (const Critter& c : sv.critters) if (std::hypot(c.x - bx, c.z - bz) < 25) pts.push_back(Vec3(c.x, c.size, c.z)); }
            else for (const Remains& r : sv.remains) pts.push_back(Vec3(r.x, r.size, r.z));
            V.frame(R.trait == MF_THRIVING ? pts : FateView::nearest(pts, bx, bz));
            int living = 0;
            for (const Critter& c : sv.critters) living += std::hypot(c.x - bx, c.z - bz) < 300;
            what = fmt("%d animals living within 300 m; %s", living, remainsCount(sv, bx, bz, 300).c_str());
        }
        V.shot(fn);
        drawn++;
        printf("      drawn: %s (%s)\n", fn.c_str() + 12, what.c_str());
    }

    // ---- B-469: a flock of the dyed hides' world's flyers low over open ground, every member dyed as its herds' are (the air's sick drawn)
    if (dyedWorld) {
        FateView V;
        if (V.init(dyedWorld->sys, dyedWorld->body, dyedLayer, 0, false) && V.sv.bestiary.flyerCount) {
            SurfaceView& sv = V.sv;
            sv.critters.clear(); sv.flocks.clear(); sv.herds.clear(); sv.remains.clear(); sv.habitatT = 1e18;
            const int spi = sv.bestiary.firstFlyer();
            const Species& sp = sv.bestiary.species[spi];
            Flock f;
            f.species = spi; f.seed = mix64(dyedWorld->sys.bodies[dyedWorld->body].seed ^ 0xF10CULL); f.n = 8;
            f.cx = 0; f.cz = 6 + 4 * sp.size; f.radius = 3; f.phase = 0; f.speed = 0.05; f.alt = f.cruiseAlt = 2 + 2 * sp.size; f.restTimer = 1e9;
            f.sickness = MF_DYED; f.sick = 1;
            sv.flocks.push_back(f);
            V.step(30);
            // (the camera on the member nearest the flock's middle, where `drawLife` puts it: its offsets drawn from the flock's seed in order)
            const Flock& g = sv.flocks[0];
            const double a = g.phase + V.t * g.speed, fcx = g.cx + std::cos(a) * g.radius, fcz = g.cz + std::sin(a) * g.radius;
            const double base = std::max(sv.site.groundHeight(fcx, fcz), sv.site.waterAt(fcx, fcz)) + g.alt;
            Rng r(g.seed);
            double bx = fcx, by = base, bz = fcz, best = 1e9;
            for (int i = 0; i < g.n; i++) {
                const double ox = r.sym(18), oz = r.sym(18), oy = r.sym(6), ph = r.range(0, TAU);
                r.uni();
                if (std::hypot(ox, oz) < best) {
                    best = std::hypot(ox, oz);
                    bx = fcx + ox + 3 * std::sin(V.t * 0.7 + ph); bz = fcz + oz + 3 * std::cos(V.t * 0.5 + ph); by = base + oy + 1.5 * std::sin(V.t * 0.9 + ph);
                }
            }
            const double d = 2.5 + 2.5 * sp.size;
            V.cam(bx - 0.6 * d, by - 0.4 * d, bz - 0.8 * d, bx, by, bz);
            V.shot("shots/tests/fates/fate_dyed_flock.png");
            drawn++;
            printf("  dyed_flock: eight %s, every one with its dyed hide, %.0f m up (fates/fate_dyed_flock.png)\n", sv.bestiary.words(spi).c_str(), f.alt);
        }
    }
    // ---- the five stages of remains, and each of a world's kinds dead (on the first die-off's world)
    if (firstWorld) {
        FateView V;
        if (V.init(firstWorld->sys, firstWorld->body, firstLayer, 0, false)) {
            SurfaceView& sv = V.sv;
            sv.critters.clear(); sv.flocks.clear(); sv.herds.clear(); sv.remains.clear(); sv.habitatT = 1e18;
            const int spi = firstWalker(sv.bestiary);
            if (spi >= 0) {
                const Species& sp = sv.bestiary.species[spi];
                const double gap = std::max(1.6, 1.3 * (sp.body.length + sp.body.leg)), near = 2 + 2.6 * gap;
                for (int k = RS_FRESH; k < RS_COUNT; k++) {
                    Remains r; r.species = spi; r.x = (k - 3) * gap; r.z = near; r.heading = PI / 2 + 0.3 * (k - 3); r.size = sp.size; r.stage = k; r.seed = mix64(0xB0E5ULL + k);
                    r.cell = false;
                    sv.testRemainsAdd(r);
                }
                V.cam(0, sv.site.groundHeight(0, 0) + 0.7 * gap + 0.6 * sp.size, 0, 0, sv.site.groundHeight(0, near), near);
                V.shot("shots/tests/fates/remains.png");
                drawn++;
                printf("  remains: %s fresh, bloated, sunken, bones and scattered, left to right (fates/remains.png)\n", sv.bestiary.words(spi).c_str());
                sv.remains.clear();
                std::string kinds;
                const int n = std::min((int)sv.bestiary.species.size(), 6);
                double span = 0;
                for (int i = 0; i < n; i++) span += std::max(2.0, 0.7 * (std::min(sv.bestiary.species[i].size, 3.0) / std::max(sv.bestiary.species[i].size, 0.05)) * (sv.bestiary.species[i].body.length + sv.bestiary.species[i].body.span + sv.bestiary.species[i].body.leg));
                double x = -0.5 * span;
                for (int i = 0; i < n; i++) {
                    const Species& s2 = sv.bestiary.species[i];
                    const double k = std::min(s2.size, 3.0) / std::max(s2.size, 0.05), w = std::max(2.0, 0.7 * k * (s2.body.length + s2.body.span + s2.body.leg));
                    Remains r; r.species = i; r.x = x + 0.5 * w; r.z = 0.75 * span; r.heading = PI / 2; r.size = std::min(s2.size, 3.0); r.stage = RS_FRESH; r.seed = mix64(0xB0E6ULL + i); r.cell = false;
                    x += w;
                    sv.testRemainsAdd(r);
                    kinds += fmt("%s%s", kinds.empty() ? "" : ", ", sv.bestiary.words(i).c_str());
                }
                V.cam(0, sv.site.groundHeight(0, 0) + 0.35 * span, 0, 0, sv.site.groundHeight(0, 0.75 * span), 0.75 * span);
                V.shot("shots/tests/fates/remains_kinds.png");
                drawn++;
                printf("  remains of each kind, fresh: %s (fates/remains_kinds.png)\n", kinds.c_str());
            }
        }
    }

    // ---- each change on a young of the first world's walker, a whole one beside it
    if (firstWorld) {
        FateView V;
        if (V.init(firstWorld->sys, firstWorld->body, firstLayer, 0, false, 1)) {
            SurfaceView& sv = V.sv;
            sv.critters.clear(); sv.flocks.clear(); sv.herds.clear(); sv.remains.clear(); sv.habitatT = 1e18;
            const int spi = firstWalker(sv.bestiary);
            std::vector<std::vector<uint32_t>> cells;
            std::string legend;
            if (spi >= 0 && spi < (int)sv.changedRigs.size()) {
                const Species& sp = sv.bestiary.species[spi];
                const double near = std::max(2.5, 2.4 * sp.size + 1.0 * sp.body.length);
                for (int ch = 0; ch < CHG_COUNT; ch++) {
                    SurfaceView::SpeciesChanges& sc = sv.speciesChanges[spi];
                    sc.n = 1; sc.changes[0] = ch; sc.drawn = changeDrawn(ch) ? 1ull << ch : 0;
                    sv.changedRigs[spi] = rigOf(changedBody(sp.body, sc.changes, 1));
                    const double gx = 0.5 * (sp.body.length + 0.5 * sp.body.neck) + 0.35;
                    sv.testSpecimen(spi, Vec3(-gx, sv.site.groundHeight(-gx, near), near), PI / 2 - 0.45, 0, 0);
                    sv.testSpecimenFate(0, 0, true, ch == CHG_GIANT ? 1.6 : (ch == CHG_DWARF ? 0.55 : 1.0));
                    sv.testSpecimenAdd(spi, Vec3(gx, sv.site.groundHeight(gx, near), near), PI / 2 - 0.45, 0, 0);
                    V.cam(0, sv.site.groundHeight(0, 0) + 0.6 + 0.9 * sp.size, 0, 0, sv.site.groundHeight(0, near) + 0.45 * sp.size, near);
                    const std::vector<uint32_t> px = frameOf(V);
                    writePNG(fmt("shots/tests/fates/change_%s.png", lowerTag(CHANGE_NAMES[ch]).c_str()).c_str(), px.data(), FBW, FBH);
                    cells.push_back(px);
                    legend += fmt("%s%s", legend.empty() ? "" : ", ", CHANGE_NAMES[ch]);
                    drawn++;
                }
                writeSheet(cells, FBW, FBH, 7, "shots/tests/fates/changes.png");
                printf("  changes: %d on %s (changed on the left, born whole on the right), in rows of seven: %s (fates/changes.png)\n", CHG_COUNT,
                       sv.bestiary.words(spi).c_str(), legend.c_str());
            }
        }
    }

    // ---- a seeded world's herd over its steps: the blood poison's line on the first world where it takes and a herd's animals stand 0.8 m or
    // more, the cell with the most of them by the land nearest the place
    const SeedAirCase* stepsWorld = nullptr;
    Seeding stepsSeeding;
    MadeWorld stepsEnd;
    for (const SeedAirCase& c : worlds) {
        const Body& b = c.sys.bodies[c.body];
        const Bestiary B = Bestiary::make(b, BodyGen::make(b));
        bool big = false;
        for (int i = 0; i < B.landCount; i++) big = big || (B.species[i].size >= 0.8 && B.species[i].social == SOCIAL_HERD);
        SeedLine line;
        if (!big || !fateLine(c, fateRecipes()[1], line) || !fateLayer(c, line, stepsSeeding, stepsEnd)) continue;
        stepsWorld = &c;
        break;
    }
    if (stepsWorld) {
        const SeedAirCase& c = *stepsWorld;
        const Seeding& firstSeeding = stepsSeeding;
        const MadeWorld& firstLayer = stepsEnd;
        const Body& b = c.sys.bodies[c.body];
        const BodyGen g = BodyGen::make(b);
        double siteLat = 0, siteLon = 0, tSun = 1000;
        (void)g;
        StarSystem sys = c.sys;
        if (!siteNear(sys, c.body, firstLayer, firstSeeding.lat, firstSeeding.lon, false, true, siteLat, siteLon, tSun)) printf("  steps: no lit land by the place\n");
        else {
            int gLat = 0, gLon = 0; bool cellSet = false;
            std::string steps;
            for (int st : {0, 2, 4, 6, 8, 12, 16, 24, 36, 48}) {
                const MadeWorld m = madeWorldOf(c.sys, c.body, {firstSeeding}, st * MADE_STEP_S / firstSeeding.verdict.pace + 1);
                FateView V;
                if (!V.initAt(sys, c.body, m, siteLat, siteLon, tSun)) continue;
                SurfaceView& sv = V.sv;
                V.cells();
                const SurfaceView::Herd* hd = nullptr;
                if (!cellSet) {   // (the cell whose herd is the most of animals 0.7 m or more, else the most; the same at every step)
                    double most = 0;
                    for (int pass = 0; pass < 2 && !hd; pass++)
                        for (int h = 0; h < (int)sv.herds.size(); h++) {
                            if (sv.herds[h].origin || sv.herds[h].empty) continue;
                            double n = 0;
                            for (const Critter& cr : sv.critters) n += cr.herd == h && (pass || cr.size >= 0.7) ? 1.0 : 0.0;
                            if (n > most) { most = n; hd = &sv.herds[h]; }
                        }
                    if (hd) {
                        gLat = hd->gLat; gLon = hd->gLon; cellSet = true;
                        for (const Critter& cr : sv.critters)
                            if (cr.herd == (int)(hd - sv.herds.data())) { steps += fmt("  (the herd: %s, %.1f m)\n", sv.bestiary.words(cr.species).c_str(), cr.size); break; }
                    }
                } else
                    for (const SurfaceView::Herd& h : sv.herds) if (!h.origin && h.gLat == gLat && h.gLon == gLon) hd = &h;
                if (!hd) { steps += fmt("  step %d: no herd cell near\n", st); continue; }
                const double hx = hd->cx, hz = hd->cz;
                V.step(30);
                std::vector<Vec3> pts;   // the herd's dead within 40 m of its spot, else its living
                for (const Remains& r : sv.remains) if (std::hypot(r.x - hx, r.z - hz) < 40) pts.push_back(Vec3(r.x, r.size, r.z));
                if (pts.empty()) for (const Critter& cr : sv.critters) if (std::hypot(cr.x - hx, cr.z - hz) < 40) pts.push_back(Vec3(cr.x, cr.size, cr.z));
                if (pts.empty()) pts.push_back(Vec3(hx, 1.0, hz));
                V.frame(FateView::nearest(pts, pts[0].x, pts[0].z));
                V.shot(fmt("shots/tests/fates/steps_%02d.png", st));
                drawn++;
                int living = 0;
                for (const Critter& cr : sv.critters) living += std::hypot(cr.x - hx, cr.z - hz) < 120;
                std::string traits;
                for (const std::string& w : madeTraitWords(m, true)) traits += (traits.empty() ? "" : ", ") + w;
                steps += fmt("  step %2d (age %2d): %d living within 120 m of the herd's spot; %s; MADE %s\n", st, m.fateAge, living, remainsCount(sv, hx, hz, 120).c_str(),
                             traits.empty() ? "NOTHING" : traits.c_str());
            }
            printf("  a seeded world's herd over its steps (%s at %lld %lld %lld body %d, the cell %d %d by the place; fates/steps_NN.png):\n%s", KINDS[b.type].name,
                   (long long)c.sys.star.sx, (long long)c.sys.star.sy, (long long)c.sys.star.sz, c.body, gLat, gLon, steps.c_str());
        }
    }
    setFramebufferScale(savedScale);
    printf("seed fates: %d of %d fates found a world%s; %d frames  (%.1f s)\n", found, (int)fateRecipes().size(), missing.empty() ? "" : fmt(" (none for %s)", missing.c_str()).c_str(),
           drawn, nowSec() - t0);
    return missing.empty() ? 0 : 1;
}
