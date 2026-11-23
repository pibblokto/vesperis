// J-09 (R-448): the overgrowth's modes. `overgrowth plates`: each type on a test ground (`shots/tests/overgrowth/type_<name>.png`) and forty
// variants of them (`variant_NN_<tag>.png`: their colours, textures, grains, wetness, hardness, a glow at night, cilia), each a layer made by
// hand over the whole of a flora world, by day at a forest's edge (at night where it glows); the sheets `types.png` and `variants.png`.
// `overgrowth world`: a line that never stops and binds let go on the first world of `seed climate`'s with land and plants where its overgrowth
// takes; its overgrowth from orbit (by day, at night where it glows), on the landing map and on the ground at four of its steps
// (`overgrowth/world_<step>_<view>.png`)
#include "harness.h"
#include "life/seeding.h"
#include "test_worlds.h"
#include "core/sphere.h"
#include "peoples/plan.h"
#include <algorithm>
#include <sys/stat.h>

namespace {
struct GrowthPlate { const char* tag; int type, texture; RGB colour; double grain, wet; int hard; bool glow, hair; };
const RGB RED(0.62f, 0.16f, 0.18f), PINK(0.86f, 0.52f, 0.54f), PALE(0.86f, 0.78f, 0.72f), BRUISED(0.42f, 0.46f, 0.58f), PURPLE(0.42f, 0.20f, 0.40f),
        BROWN(0.40f, 0.26f, 0.20f), BLACK(0.12f, 0.09f, 0.09f), CLEAR(0.84f, 0.86f, 0.82f), WHITE(0.90f, 0.88f, 0.82f), TAN(0.80f, 0.72f, 0.56f),
        GREY(0.56f, 0.54f, 0.52f), ORANGE(0.88f, 0.56f, 0.20f), YELLOW(0.92f, 0.82f, 0.20f), SCARLET(0.86f, 0.30f, 0.20f), GREEN(0.30f, 0.52f, 0.22f),
        LICHEN(0.55f, 0.60f, 0.45f), BLUE(0.52f, 0.62f, 0.78f), SLATE(0.30f, 0.34f, 0.42f);
// each type at its plainest (its first colour and its own texture, a middling grain), then four variants of each
std::vector<GrowthPlate> typePlates() {
    return {
            {"flesh", OG_FLESH, OGT_BLISTERED, RED, 0.5, 0.4, 0, false, false},
            {"fungus", OG_FUNGUS, OGT_FIBROUS, WHITE, 0.5, 0.1, 0, false, false},
            {"slime_mould", OG_SLIME_MOULD, OGT_BLISTERED, YELLOW, 0.5, 0.8, 0, false, true},
            {"lichen", OG_LICHEN, OGT_FOLDED, LICHEN, 0.5, 0, 1, false, false},
            {"coral", OG_CORAL, OGT_FOLDED, PALE, 0.5, 0.1, 3, false, false},
            {"sponge", OG_SPONGE, OGT_BLISTERED, RGB(0.86f, 0.82f, 0.56f), 0.5, 0.3, 3, false, false},
            {"hair", OG_HAIR, OGT_FIBROUS, RGB(0.86f, 0.82f, 0.74f), 0.5, 0, 0, false, true},
            {"bladders", OG_BLADDERS, OGT_BLISTERED, RGB(0.86f, 0.62f, 0.66f), 0.5, 0.5, 0, false, false},
            {"carpets", OG_CARPETS, OGT_FIBROUS, GREEN, 0.5, 0.2, 0, false, false},
            {"films", OG_FILMS, OGT_BLISTERED, RGB(0.62f, 0.66f, 0.58f), 0.3, 0.9, 0, false, false},
    };
}
std::vector<GrowthPlate> variantPlates() {
    return {
            {"flesh_pink_folded_wet", OG_FLESH, OGT_FOLDED, PINK, 0.5, 0.9, 0, false, false},
            {"flesh_bruised_fibrous", OG_FLESH, OGT_FIBROUS, BRUISED, 0.7, 0.3, 0, false, false},
            {"flesh_black_coiled_glow", OG_FLESH, OGT_COILED, BLACK, 0.5, 0.5, 0, true, false},
            {"flesh_clear_ciliated", OG_FLESH, OGT_BLISTERED, CLEAR, 0.3, 0.7, 0, false, true},
            {"fungus_tan_coarse", OG_FUNGUS, OGT_FIBROUS, TAN, 0.9, 0.1, 0, false, false},
            {"fungus_grey_fine_wet", OG_FUNGUS, OGT_BLISTERED, GREY, 0.1, 0.7, 0, false, false},
            {"fungus_orange_glow", OG_FUNGUS, OGT_FIBROUS, ORANGE, 0.5, 0.2, 0, true, false},
            {"fungus_white_leathery", OG_FUNGUS, OGT_FOLDED, WHITE, 0.6, 0, 1, false, false},
            {"mould_orange", OG_SLIME_MOULD, OGT_FIBROUS, ORANGE, 0.5, 0.8, 0, false, true},
            {"mould_scarlet_wet", OG_SLIME_MOULD, OGT_BLISTERED, SCARLET, 0.4, 1.0, 0, false, true},
            {"mould_yellow_coarse", OG_SLIME_MOULD, OGT_COILED, YELLOW, 0.9, 0.6, 0, false, false},
            {"mould_yellow_glow", OG_SLIME_MOULD, OGT_BLISTERED, YELLOW, 0.5, 0.8, 0, true, true},
            {"lichen_orange", OG_LICHEN, OGT_FOLDED, RGB(0.85f, 0.55f, 0.15f), 0.5, 0, 1, false, false},
            {"lichen_yellow_fine", OG_LICHEN, OGT_BLISTERED, RGB(0.80f, 0.78f, 0.30f), 0.2, 0, 1, false, false},
            {"lichen_grey_coarse", OG_LICHEN, OGT_FIBROUS, LICHEN, 0.9, 0, 2, false, false},
            {"lichen_black_bony", OG_LICHEN, OGT_COILED, BLACK, 0.6, 0, 3, false, false},
            {"coral_pink", OG_CORAL, OGT_FOLDED, RGB(0.90f, 0.56f, 0.56f), 0.5, 0.2, 3, false, false},
            {"coral_orange_coiled", OG_CORAL, OGT_COILED, RGB(0.90f, 0.62f, 0.30f), 0.5, 0.2, 3, false, false},
            {"coral_white_plated", OG_CORAL, OGT_BLISTERED, WHITE, 0.2, 0, 2, false, false},
            {"coral_white_glow", OG_CORAL, OGT_FOLDED, WHITE, 0.5, 0.1, 3, true, true},
            {"sponge_grey", OG_SPONGE, OGT_BLISTERED, RGB(0.62f, 0.64f, 0.66f), 0.5, 0.5, 3, false, false},
            {"sponge_blue_coiled", OG_SPONGE, OGT_COILED, BLUE, 0.6, 0.5, 3, false, false},
            {"sponge_yellow_coarse", OG_SPONGE, OGT_FIBROUS, YELLOW, 0.9, 0.3, 2, false, false},
            {"sponge_glow", OG_SPONGE, OGT_BLISTERED, RGB(0.86f, 0.82f, 0.56f), 0.5, 0.4, 3, true, false},
            {"hair_black", OG_HAIR, OGT_FIBROUS, RGB(0.12f, 0.10f, 0.10f), 0.5, 0, 0, false, true},
            {"hair_red_coiled", OG_HAIR, OGT_COILED, RGB(0.62f, 0.24f, 0.16f), 0.5, 0.2, 0, false, true},
            {"hair_white_coarse", OG_HAIR, OGT_FIBROUS, WHITE, 0.9, 0, 0, false, true},
            {"hair_glow", OG_HAIR, OGT_FIBROUS, RGB(0.86f, 0.82f, 0.74f), 0.5, 0, 0, true, true},
            {"bladders_green", OG_BLADDERS, OGT_BLISTERED, RGB(0.62f, 0.78f, 0.56f), 0.5, 0.5, 0, false, false},
            {"bladders_pale_fine", OG_BLADDERS, OGT_FOLDED, RGB(0.80f, 0.80f, 0.62f), 0.2, 0.4, 0, false, false},
            {"bladders_pink_wet", OG_BLADDERS, OGT_BLISTERED, PINK, 0.6, 1.0, 0, false, false},
            {"bladders_glow", OG_BLADDERS, OGT_BLISTERED, RGB(0.86f, 0.62f, 0.66f), 0.5, 0.5, 0, true, false},
            {"carpets_purple", OG_CARPETS, OGT_FIBROUS, PURPLE, 0.5, 0.2, 0, false, false},
            {"carpets_red_folded", OG_CARPETS, OGT_FOLDED, RGB(0.62f, 0.20f, 0.16f), 0.5, 0.2, 0, false, false},
            {"carpets_brown_wet", OG_CARPETS, OGT_COILED, BROWN, 0.5, 0.9, 0, false, false},
            {"carpets_green_glow", OG_CARPETS, OGT_FIBROUS, GREEN, 0.5, 0.2, 0, true, false},
            {"films_brown", OG_FILMS, OGT_BLISTERED, RGB(0.46f, 0.40f, 0.24f), 0.3, 0.9, 0, false, false},
            {"films_slate", OG_FILMS, OGT_FOLDED, SLATE, 0.3, 1.0, 0, false, false},
            {"films_red", OG_FILMS, OGT_COILED, RED, 0.3, 0.9, 0, false, false},
            {"films_glow", OG_FILMS, OGT_BLISTERED, RGB(0.62f, 0.66f, 0.58f), 0.3, 0.9, 0, true, false},
    };
}
MadeGrowth plateGrowth(const GrowthPlate& P) { return growthOfType(P.type, P.texture, P.colour, P.grain, P.wet, P.hard, P.glow, P.hair); }
// the view: the explorer 40 m behind the camera (the ground's rings centre on them), a few frames to warm, then the frame mushed
std::vector<uint32_t> growthFrame(SurfaceView& sv, StarNeighborhood& nb, SpaceRenderer& sr, double t) {
    Input in;
    sv.player.x = sv.freePos.x - std::sin(sv.freeYaw) * 40; sv.player.z = sv.freePos.z - std::cos(sv.freeYaw) * 40;
    sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
    for (int k = 0; k < 3; k++) { sv.update(1.0 / 30, in, t, false); Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0); }
    Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
    std::vector<uint32_t> px((size_t)FBW * FBH);
    fb.toRGB(px.data(), 1.0);
    return px;
}
}   // namespace

MadeGrowth growthOfType(int type, int texture, const RGB& colour, double grain, double wet, int hard, bool glow, bool hair) {
    MadeGrowth G;
    G.type = type; G.texture = texture; G.colour = colour; G.veins = colour * 0.55f;
    G.grain = grain; G.wet = wet; G.hard = hard; G.hair = hair;
    if (glow) { G.glow = 1; G.glowTint = RGB(0.30f, 0.86f, 0.80f); }
    G.age = G.steps = 16; G.reach = PI;   // (the whole world, its structures at their fullest)
    G.material = madeGrowthCover(G.type, G.hard);
    return G;
}
MadeWorld growthLayer(double radiusKm, const Vec3& place, const MadeGrowth& G) {
    MadeWorld m;
    m.lines = 1;
    m.spread[0].place = place;
    m.spread[0].growth = G;
    m.growths = 1 << G.type; m.growthLine = 0;
    m.radiusKm = radiusKm;
    m.key = madeKey(m);
    return m;
}
namespace {
// a place at a forest's edge (the planet function's: open grass, the forest on a fifth to two thirds of a ring 20 to 60 m round it, no water
// there, its relief under 10 m), a time its sun stands 25 to 55 degrees up within two turns and one 12 degrees under within forty; false where
// none is found (a world turned to its sun has no night)
bool forestEdge(StarSystem& sys, int bi, double& lat, double& lon, double& t, double& night) {
    const Body& b = sys.bodies[bi];
    const BodyGen g = BodyGen::make(b);
    const double R = b.radiusKm * 1000;
    Rng r(mix64(b.seed ^ 0x6E0E70ULL));
    auto at = [&](double dx, double dz, double detail) {
        return sampleSurface(g, StarSystem::bodyFromLatLon(lat + dz / R, lon + dx / (R * std::max(0.05, std::cos(lat)))), detail);
    };
    // round three of the world's land sites (within 3 km of each, where its rivers are worked out once), 150 places each
    std::vector<std::pair<double, double>> sites;
    landSitesOf(sys, bi, 0x6E0E, 3, sites);
    for (const auto& st : sites) {
        const double la0 = st.first, lo0 = st.second;
        for (int tries = 0; tries < 150; tries++) {
            lat = la0 + r.range(-3000, 3000) / R; lon = lo0 + r.range(-3000, 3000) / (R * std::max(0.05, std::cos(la0)));
            if (std::fabs(lat) > 55 * DEG) continue;
            const SurfaceSample o = at(0, 0, 16);
            if (o.material != MAT_GRASS || madeWetAt(o)) continue;
            int near = 0;   // (a first ring of eight at 40 m: some forest and some open)
            for (int k = 0; k < 8; k++) near += at(std::sin(k * TAU / 8) * 40, std::cos(k * TAU / 8) * 40, 16).material == MAT_FOREST;
            if (near < 1 || near > 6) continue;
            int forest = 0; double lo = 1e9, hi = -1e9; bool wet = false;
            for (int k = 0; k < 24; k++) {
                const double a = k * TAU / 24, d = 20 + 20 * (k % 3);
                const SurfaceSample v = at(std::sin(a) * d, std::cos(a) * d, 16);
                forest += v.material == MAT_FOREST;
                wet = wet || madeWetAt(v);
                lo = std::min(lo, v.height); hi = std::max(hi, v.height);
            }
            if (wet || forest < 5 || forest > 16 || hi - lo > 10) continue;
            SurfaceSite site; site.init(&sys, bi, lat, lon, 1000.0);
            const double day = std::max(600.0, std::fabs(b.rotPeriod));
            t = -1; night = -1;
            for (int k = 0; k < 192 && t < 0; k++) {
                const double tt = 1000 + day * 2 * k / 192, alt = site.sun(tt).altitude;
                if (alt > 25 * DEG && alt < 55 * DEG) t = tt;
            }
            if (t < 0) continue;
            for (int k = 1; k <= 4000 && night < 0; k++) if (site.sun(t + day * k / 100.0).altitude < -12 * DEG) night = t + day * k / 100.0;
            return night > 0;   // (a world turned to its sun has no night anywhere: the next world)
        }
    }
    return false;
}
}   // namespace

bool growthGround(GrowthGround& P, bool shore) {
    const std::vector<SeedAirCase> cases = seedAirCases(36);
    for (int pass = 0; pass < 2; pass++)   // a clear air first (an earthlike world's, a superhabitable one's, a steppe's), then any with plants
    for (const SeedAirCase& c : cases) {
        const int type = c.sys.bodies[c.body].type;
        if (!kindHas(type, KT_FLORA) || (pass == 0) != (type == PT_FELISIAN || type == PT_SUPERHABITABLE || type == PT_STEPPE)) continue;
        P.sys = c.sys; P.body = c.body;
        if (!shore) { if (forestEdge(P.sys, P.body, P.lat, P.lon, P.day, P.night)) return true; continue; }
        if (!waysSite(P.sys, P.body, 3, false, P.lat, P.lon, P.day, 7)) continue;
        // the same place's night (its solar day can be many turns long): its sun 12 degrees under, within forty turns
        SurfaceSite site; site.init(&P.sys, P.body, P.lat, P.lon, P.day);
        const double turn = std::max(600.0, std::fabs(P.sys.bodies[P.body].rotPeriod));
        P.night = -1;
        for (int k = 1; k <= 4000 && P.night < 0; k++) if (site.sun(P.day + turn * k / 100.0).altitude < -12 * DEG) P.night = P.day + turn * k / 100.0;
        if (P.night < 0) continue;
        return true;
    }
    return false;
}

namespace {
// the camera facing the place's forest (the bearing with the most covered forest 30 to 90 m on) or, on a shore, its water: a few metres
// behind the biggest structure ahead of the place (within 40 m along that bearing, 14 m to either side), its middle in view
void aimAtEdge(SurfaceView& sv, bool shore) {
    double best = -1, yaw = 0;
    for (int k = 0; k < 16; k++) {
        const double a = k * TAU / 16;
        int n = 0;
        for (double d = 30; d <= 90; d += 15) {
            const TerrainVertex& v = sv.site.lod0.at((int)std::floor(std::sin(a) * d / 16), (int)std::floor(std::cos(a) * d / 16));
            n += shore ? v.material == MAT_WATER : (madeGrowthMaterial(v.material) && v.veg >= 0.4);
        }
        if (n > best) { best = n; yaw = a; }
    }
    const double hx = std::sin(yaw), hz = std::cos(yaw);
    GrowthInst pick; double size = -1;
    for (int cz = -4; cz < 4; cz++)
        for (int cx = -4; cx < 4; cx++)
            sv.forGrowths(cx, cz, [&](const GrowthInst& S) {
                const double along = S.x * hx + S.z * hz, side = std::fabs(S.x * hz - S.z * hx);
                const double big = std::max(S.r, std::max(0.6 * S.h, 0.4 * S.len));
                if (along < 4 || along > 40 || side > 14 || big <= size) return;
                size = big; pick = S;
            });
    sv.freeCam = true;
    sv.freeYaw = yaw;
    if (size < 0) {   // none: the place itself
        const double gy = sv.site.groundHeight(0, 0);
        sv.freePos = Vec3(-hx * 8, gy + 2.4, -hz * 8); sv.freePitch = -0.12;
        return;
    }
    const double d = 3.5 + 2.2 * std::max(pick.r, std::max(0.7 * pick.h, 0.5 * pick.len));
    const double x = pick.x - hx * d, z = pick.z - hz * d, gy = sv.site.groundHeight(x, z);
    sv.freePos = Vec3(x, gy + 1.7, z);
    sv.freePitch = std::atan2(pick.gy + 0.45 * pick.h - (gy + 1.7), d);
}
std::string grownWords(const MadeWorld& m) {
    std::string w;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeGrowth& G = m.spread[i].growth;
        if (!G.on()) continue;
        w += fmt("%s%s (%s, grain %.1f, wet %.1f, hard %d%s%s, %d steps, %.0f km)", w.empty() ? "" : "; ", MADE_GROWTH_NAMES[G.type], MATERIAL_NAMES[G.material], G.grain,
                 G.wet, G.hard, G.glow > 0 ? ", glowing" : "", G.hair ? ", cilia" : "", G.age, G.reach * m.radiusKm);
    }
    return w.empty() ? "NONE" : w;
}
}   // namespace

int overgrowthPlates() {
    const double t0 = nowSec();
    mkdir("shots/tests/overgrowth", 0755);
    const int scale0 = g_testScale;
    setFramebufferScale(2);   // (at 2x, as `bestiary ways`)
    GrowthGround land, shore;
    if (!growthGround(land, false) || !growthGround(shore, true)) { printf("overgrowth plates: no flora world with a forest's edge and a shore\n"); return 1; }
    printf("  the test ground: %s at %lld %lld %lld body %d, %.3f %.3f (the films' on its shore at %.3f %.3f)\n", KINDS[land.sys.bodies[land.body].type].name,
           (long long)land.sys.star.sx, (long long)land.sys.star.sy, (long long)land.sys.star.sz, land.body, land.lat / DEG, land.lon / DEG, shore.lat / DEG, shore.lon / DEG);
    int bad = 0;
    auto plates = [&](const std::vector<GrowthPlate>& list, const char* prefix, const char* sheet, int cols) {
        std::vector<std::vector<uint32_t>> cells;
        int k = 0;
        for (const GrowthPlate& P : list) {
            const GrowthGround& W = P.type == OG_FILMS ? shore : land;
            const MadeGrowth G = plateGrowth(P);
            StarSystem sys = W.sys;
            const double t = P.glow ? W.night : W.day;
            SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
            sv.site.made = growthLayer(sys.bodies[W.body].radiusKm, StarSystem::bodyFromLatLon(W.lat, W.lon), G);
            sv.init(&sys, W.body, W.lat, W.lon, t);
            nb.update(sv.site.worldPos(t, 0, 0, 0));
            sv.relocateCapsule(0, -300);
            aimAtEdge(sv, P.type == OG_FILMS);
            const std::vector<uint32_t> px = growthFrame(sv, nb, sr, t);
            const std::string fn = std::string(prefix) == "type" ? fmt("shots/tests/overgrowth/type_%s.png", P.tag) : fmt("shots/tests/overgrowth/variant_%02d_%s.png", k, P.tag);
            writePNG(fn.c_str(), px.data(), FBW, FBH);
            cells.push_back(px);
            // what it drew: its cover under the camera, its structures, the wrapped trees
            int covered = 0, cells16 = 0, structures = 0;
            for (int cz = -6; cz < 6; cz++)
                for (int cx = -6; cx < 6; cx++) {
                    cells16++;
                    covered += madeGrowthMaterial(sv.site.lod0.at(cx, cz).material);
                    sv.forGrowths(cx, cz, [&](const GrowthInst&) { structures++; });
                }
            const bool ok = P.type == OG_FILMS ? covered > 0 : covered > cells16 / 3 && structures > 0;   // (a film's on its shore's band alone)
            bad += !ok;
            printf("  %-28s %s: %d of %d cells covered, %d structures, %d drawn%s\n", P.tag, MATERIAL_NAMES[G.material], covered, cells16, structures,
                   sv.lastGrowthsDrawn, ok ? "" : "  (NONE)");
            k++;
        }
        writeSheet(cells, FBW, FBH, cols, fmt("shots/tests/overgrowth/%s.png", sheet));
    };
    plates(typePlates(), "type", "types", 5);
    plates(variantPlates(), "variant", "variants", 8);
    setFramebufferScale(scale0);
    printf("overgrowth plates: %d types, %d variants, %d with nothing drawn  (%.1f s)\n", (int)typePlates().size(), (int)variantPlates().size(), bad, nowSec() - t0);
    return bad ? 1 : 0;
}

int overgrowthWorld() {
    const double t0 = nowSec();
    mkdir("shots/tests/overgrowth", 0755);
    // the line: a world's own of the sea, cut never to stop and to bind its cells
    const std::vector<std::pair<int, int>> wants = {{GN_STOP, -2}, {GN_BIND, -1}};
    for (const SeedAirCase& c : seedAirCases(36)) {
        const Body& b = c.sys.bodies[c.body];
        if (!kindHas(b.type, KT_FLORA) || !c.wild.valid) continue;
        const SeedLine line0 = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, wants));
        if (!line0.valid || !line0.kind.world.overgrows || line0.kind.world.bind <= 0) continue;
        SeedLine line = line0; line.key += "/S1";
        // let go in its sea a kilometre to eight off the world's first coast, where it takes (so its overgrowth reaches the land in its first steps)
        const BodyGen bg = BodyGen::make(b);
        double cla = 0, clo = 0, la = 0, lo = 0;
        if (!coastSite(bg, b.radiusKm, cla, clo)) continue;
        Seeding sd;
        bool took = false;
        for (double km : {1.0, 2.0, 4.0, 8.0})
            for (int k = 0; k < 16 && !took; k++) {
                const double a = k * TAU / 16, d = km / b.radiusKm;
                la = cla * DEG + d * std::cos(a); lo = clo * DEG + d * std::sin(a) / std::max(0.05, std::cos(cla * DEG));
                const SurfaceSample q = sampleSurface(bg, StarSystem::bodyFromLatLon(la, lo), 64);
                if (!madeWetAt(q) || q.waterKind != 0) continue;
                sd = seedingFor(c.sys, c.body, line, la, lo, 0);
                took = sd.verdict.takes();
            }
        if (!took) continue;
        const double pace = sd.verdict.pace;
        const MadeWorld full = madeWorldOf(c.sys, c.body, {sd}, 32 * MADE_STEP_S / pace + 1);
        if (!full.grown()) continue;
        printf("  %s at %lld %lld %lld body %d, species %d %s let go at %.2f %.2f: %s\n", KINDS[b.type].name, (long long)c.sys.star.sx, (long long)c.sys.star.sy,
               (long long)c.sys.star.sz, c.body, c.wild.kind.s.index, geneEditsText(seedGeneEdits(c.wild.genes, wants)).c_str(), la / DEG, lo / DEG, grownWords(full).c_str());
        // the game: the line a strain of the explorer's, let go at its place; the layer at each of four steps of its overgrowth
        TestGame g;
        g.savePrefix = "shots/tests/overgrowth_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/overgrowth_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        Input in;
        auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
        auto press = [&](int k) { in.pressed[k] = in.down[k] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[k] = false; };
        const std::vector<GeneEdit> edits = seedGeneEdits(c.wild.genes, wants);
        StrainRecord rec; rec.parent = 0; rec.what = SM_EDIT; rec.edits = geneEditsText(edits);
        g.guide.strains[line.key] = rec;
        SeedRecord r; r.body = Guide::bodyKey(c.sys.star.sx, c.sys.star.sy, c.sys.star.sz, c.body); r.lat = la; r.lon = lo; r.line = line.key; r.edits = rec.edits;
        r.cell = chemistryCell(c.wild.kind.chemistry); r.tempC = std::round(c.wild.kind.s.tOpt); r.light = CL_WHITE; r.energy = CE_BROTH; r.salt = 1; r.gen = GEN_VERSION;
        r.clock = g.guide.clock;
        g.guide.addSeeding(r);
        // the time: the place's sun up, and down for the night's frame
        const Vec3 at = StarSystem::bodyFromLatLon(la, lo);
        const double turn = std::max(3600.0, std::fabs(b.rotPeriod));
        double tDay = -1;
        for (int k = 0; k < 192 && tDay < 0; k++) {
            const double tt = 20000 + turn * k / 192;
            if (dot(normalize(c.sys.bodyFrame(c.body, tt) * normalize(c.sys.star.pos - c.sys.bodyPos(c.body, tt))), at) > 0.5) tDay = tt;
        }
        if (tDay < 0) tDay = 20000;
        int shown = 0;
        double skipped = 0;
        for (int step : {9, 14, 20, 32}) {
            const double want = step * MADE_STEP_S / pace + 1;   // (the clock moved on to its step)
            g.testClockSkip(want - skipped);
            skipped = want;
            g.testSetTime(tDay); g.testParkAt(c.sys.star, c.body, true, false); g.testLandCursor(la / DEG, lo / DEG);
            run(0.3); g.testWaitMapOf(c.body); run(0.1);
            const MadeWorld m = g.testMadeNow(c.body);   // (read once the game holds the system)
            printf("    step %2d: %s\n", step, grownWords(m).c_str());
            writePNG(fmt("shots/tests/overgrowth/world_%02d_map.png", step).c_str(), g.output(), FBW, FBH);
            press(KEY_ESCAPE); run(0.4);
            writePNG(fmt("shots/tests/overgrowth/world_%02d_orbit.png", step).c_str(), g.output(), FBW, FBH);
            if (step == 32 && m.grown()) {   // its veins lit on the night side (a line that does not glow given a glow, as a test layer)
                MadeWorld lit = m;
                MadeGrowth& G = lit.spread[lit.growthLine].growth;
                if (G.glow <= 0) { G.glow = 1; G.glowTint = RGB(0.30f, 0.86f, 0.80f); lit.key = madeKey(lit); g.testMadeSet(c.body, lit); }
                g.testSetTime(tDay); g.testParkAt(c.sys.star, c.body, true, true); run(0.3); g.testWaitMapOf(c.body); run(0.1);
                press(KEY_ESCAPE); run(0.4);
                writePNG("shots/tests/overgrowth/world_32_orbit_night.png", g.output(), FBW, FBH);
                g.testMadeClear();
            }
            // the ground: the covered land nearest the place (rings out to 400 km), landed there facing more of it
            BodyGen gm = bg; gm.made = m;
            double gla = la, glo = lo;
            bool covered = false;
            const bool drained = drainageEnabled();
            setDrainageEnabled(false);   // (the rivers left out of the search)
            for (double km = 0.5; km < 400 && !covered; km *= 1.25)
                for (int k = 0; k < 24 && !covered; k++) {
                    const double a = k * TAU / 24, d = km / b.radiusKm;
                    gla = la + d * std::cos(a); glo = lo + d * std::sin(a) / std::max(0.05, std::cos(la));
                    covered = madeGrowthMaterial(sampleSurface(gm, StarSystem::bodyFromLatLon(gla, glo), 64).material);
                }
            setDrainageEnabled(drained);
            if (!covered) { printf("      no covered land within 400 km\n"); continue; }
            g.testSetTime(tDay); g.testParkAt(c.sys.star, c.body); g.testLandCursor(gla / DEG, glo / DEG);
            press(KEY_ENTER);
            for (int n = 0; n < 30 * 40 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
            run(0.5);
            const SurfaceView& S = g.testSurface();
            SurfaceSite& site = const_cast<SurfaceView&>(S).site;
            const double px0 = S.player.x, pz0 = S.player.z;
            bool found = false;
            for (int rr = 0; rr <= 24 && !found; rr++)
                for (int i = -rr; i <= rr && !found; i++)
                    for (int j = -rr; j <= rr && !found; j++) {
                        if (std::max(std::abs(i), std::abs(j)) != rr) continue;
                        const int cx = (int)std::floor(px0 / 16) + i, cz = (int)std::floor(pz0 / 16) + j;
                        bool all = true;
                        for (int q = 0; q < 4 && all; q++) all = madeGrowthMaterial(site.lod0.at(cx + (q & 1), cz + (q >> 1)).material);
                        if (!all) continue;
                        g.testWalkTo(cx * 16.0 + 8, cz * 16.0 + 8); found = true;
                    }
            if (found) {
                double best = -1, yaw = 0;
                for (int k = 0; k < 16; k++) {
                    const double a = k * TAU / 16;
                    int n = 0;
                    for (double d = 16; d <= 80; d += 16) n += madeGrowthMaterial(site.lod0.at((int)std::floor((S.player.x + std::sin(a) * d) / 16), (int)std::floor((S.player.z + std::cos(a) * d) / 16)).material);
                    if (n > best) { best = n; yaw = a; }
                }
                g.testSetYaw(yaw); g.testSetPitch(-0.15);
            }
            run(2.0);
            const TerrainVertex& tv = site.lod0.at((int)std::floor(S.player.x / 16), (int)std::floor(S.player.z / 16));
            writePNG(fmt("shots/tests/overgrowth/world_%02d_ground.png", step).c_str(), g.output(), FBW, FBH);
            printf("      landed %.1f km from the place, the ground %s%s, %d structures drawn\n", std::acos(clampd(dot(StarSystem::bodyFromLatLon(la, lo), StarSystem::bodyFromLatLon(gla, glo)), -1, 1)) * b.radiusKm,
                   MATERIAL_NAMES[clampi(tv.material, 0, MAT_COUNT - 1)], found ? "" : " (no covered ground near)", S.lastGrowthsDrawn);
            shown += found && madeGrowthMaterial(tv.material);
        }
        printf("overgrowth world: a %s overgrowth, %d of 4 steps drawn on its ground  (%.1f s)\n", MADE_GROWTH_NAMES[full.spread[full.growthLine].growth.type], shown, nowSec() - t0);
        return shown >= 3 ? 0 : 1;
    }
    printf("overgrowth world: no world of those tried where an overgrowth takes  (%.1f s)\n", nowSec() - t0);
    return 1;
}

MadeWorld growthAtStep(double radiusKm, const Vec3& place, MadeGrowth G, int step) {
    G.steps = clampi(step - MADE_GROWTH_START + 1, 0, MADE_GROWTH_STEPS);
    G.age = std::min(G.steps, MADE_STEPS);
    G.reach = madeReachOfStep(G.age, radiusKm);
    return growthLayer(radiusKm, place, G);
}
void growthLayerOn(SurfaceView& S, const MadeWorld& m) {
    S.testWaitResample();   // (KI-491: a worker sampling the rings reads the layer)
    S.site.made = m; S.site.gen.made = m;
    for (TerrainCache* c : {&S.site.lodN, &S.site.lod0, &S.site.lod1, &S.site.lod2, &S.site.lod3}) c->invalidate(-3e5, -3e5, 3e5, 3e5);
    S.testRegrowRuins();
}
namespace {
// J-10: a stone rotter's layer: its pitted look over its spread, `steps` steps since it was let go at `place`
MadeWorld pitsLayer(double radiusKm, const Vec3& place, int steps) {
    MadeWorld m;
    m.lines = 1; m.step = std::min(steps, MADE_STEPS);
    MadeSpread& s = m.spread[0];
    s.place = place; s.reach = madeReachOfStep(m.step, radiusKm); s.pits = steps; s.makes = 1 << MC_LAND; s.landOn = ML_ROCK;
    m.land.on = m.land.tinted = true; m.land.kind = MGT_PITTED; m.land.tint = RGB(0.34f, 0.32f, 0.30f); m.grounds = MGT_PITTED;
    m.radiusKm = radiusKm; m.key = madeKey(m);
    return m;
}
// what the cover grew on the settlements within two cells of a point: the climb's least and most over their buildings, their membranes,
// nodules and drapes, their pieces
// KI-477: and the most the climb differs between the walls of one building; KI-478: the arches' bays standing and those grown shut, beside the
// doorways and windows grown shut
struct RuinsGrown {
    int cells = 0, buildings = 0, covered = 0, membranes = 0, nodules = 0, drapes = 0, pieces = 0, bays = 0, baysShut = 0, doorsShut = 0;
    double lo = 1e9, hi = -1, spread = 0;
};
RuinsGrown ruinsGrown(const SurfaceView& sv, double x, double z) {
    RuinsGrown R;
    sv.forNearbyRuins(x, z, 2, [&](const Ruin&, const SurfaceView::RuinCell& c, int) {
        R.cells++; R.buildings += (int)c.spec.buildings.size(); R.pieces += (int)c.elems[0].size();
        for (size_t b = 0; b + 1 < c.climb.size(); b++)
            if (c.climb[b] >= 0) { R.covered++; R.lo = std::min(R.lo, (double)c.climb[b]); R.hi = std::max(R.hi, (double)c.climb[b]); }
        std::vector<double> lo(c.climb.size(), 1e9), hi(c.climb.size(), -1e9);
        std::vector<FormOpening> bays;
        for (const Form& f : c.elems[0]) {
            R.membranes += f.growth == FG_MEMBRANE; R.nodules += f.growth == FG_NODULE; R.drapes += f.growth == FG_DRAPE;
            if (f.growth == FG_MEMBRANE) (f.p[0] >= 1 ? R.baysShut : R.doorsShut)++;
            if (f.kind == FK_ARCH && f.part != FORM_PART_GROWTH) archOpenings(f, bays);
            if (f.kind == FK_BOX && f.part == 0 && f.building >= 0 && f.building < (int)c.climb.size() && c.climb[f.building] >= 0) {
                const double k = SurfaceView::pieceClimb(c, f.building, f.x, f.z, f.seed);
                lo[f.building] = std::min(lo[f.building], k); hi[f.building] = std::max(hi[f.building], k);
            }
        }
        R.bays += (int)bays.size();
        for (size_t b = 0; b < lo.size(); b++) if (hi[b] > lo[b]) R.spread = std::max(R.spread, hi[b] - lo[b]);
    });
    return R;
}
std::string ruinsGrownLine(const RuinsGrown& R) {
    if (!R.covered) return fmt("%d settlements, %d buildings, none covered", R.cells, R.buildings);
    return fmt("%d settlements, %d of %d buildings covered %.1f to %.1f m up (walls of one building up to %.1f m apart), %d membranes (%d doorways and windows, "
               "%d of %d arches' bays), %d nodules, %d drapes", R.cells, R.covered, R.buildings, R.lo, R.hi, R.spread, R.membranes, R.doorsShut, R.baysShut, R.bays,
               R.nodules, R.drapes);
}
}   // namespace

// J-10: `overgrowth ruins`: the medieval test world's first city and its capital under a flesh's overgrowth let go at each one's middle, by day at
// the line's steps 9, 14, 20 and 32 (from a street inside its edge and from 30 m over it; the capital from over its edge), a doorway grown shut at
// step 14, and the city crumbled by a stone rotter beside the city as generated
int overgrowthRuins() {
    const double t0 = nowSec();
    mkdir("shots/tests/overgrowth", 0755);
    const AgeWorld& w = AGE_WORLDS[AGE_INDUSTRIAL];
    Star st;
    if (!starInSector(w.sx, 0, w.sz, st, true)) { printf("overgrowth ruins: no industrial test world\n"); return 1; }
    StarSystem sys; sys.generate(st);
    const Body& body = sys.bodies[w.body];
    const BodyGen bg = BodyGen::make(body);
    const double Rkm = body.radiusKm;
    const MadeGrowth flesh = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, false, false);
    int framed = 0, wanted = 0;
    for (int which = 0; which < 4; which++) {   // the city, the capital, the city crumbled, the city as generated
        TestGame g;
        g.savePrefix = "shots/tests/overgrowth_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.guidePath = "shots/tests/overgrowth_guide.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.settings.cabin = false; applyTestScale(g);
        Input in;
        auto run = [&](double sec) { runFramesAtLeastOne(g, in, sec); };
        worldPlanClearCache();
        g.testParkAt(st, w.body);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(bg);   // (waited for, not framed through: the frames run count the lab's clock)
        const PlanPlace* pl = nullptr;
        for (const PlanPlace& q : P->plans[std::min(w.which, P->peoples - 1)].places) {   // (a city whose middle is dry with the rivers: some lie under a lake)
            if (which == 1 ? q.sclass != SC_CAPITAL : q.sclass != SC_CITY) continue;
            const SurfaceSample cs = sampleSurface(bg, q.unit, 16);
            if (which != 1 && (cs.material == MAT_WATER || (cs.water > -1e8 && cs.height < cs.water + 1))) continue;
            pl = &q; break;
        }
        static const char* const NAMES[4] = {"city", "capital", "crumbled", "uncrumbled"};
        if (!pl) { printf("  no %s on the industrial test world\n", which == 1 ? "capital" : "city"); continue; }
        const char* name = NAMES[which];
        // the line let go 10 km south of its middle (its edge reaches the settlement at step 10: bare at 9, half taken at 14)
        const Vec3 place = offsetUnit(pl->unit, 0, -10000, Rkm * 1000);
        // the time: the place's sun well up
        const double turn = std::max(3600.0, std::fabs(body.rotPeriod));
        double tDay = 20000;
        for (int k = 0; k < 192; k++) {
            const double tt = 20000 + turn * k / 192;
            if (dot(normalize(sys.bodyFrame(w.body, tt) * normalize(sys.star.pos - sys.bodyPos(w.body, tt))), place) > 0.6) { tDay = tt; break; }
        }
        if (which == 2) g.testMadeSet(w.body, pitsLayer(Rkm, place, 20));
        else if (which != 3) g.testMadeSet(w.body, growthAtStep(Rkm, place, flesh, 9));
        const double Rm = Rkm * 1000, off = pl->radiusM + 45;
        g.testSetTime(tDay); g.testParkAt(st, w.body);
        g.testLandCursor((pl->lat + off / Rm) / DEG, pl->lon / DEG);
        in.pressed[KEY_ENTER] = in.down[KEY_ENTER] = true; g.frame(in, 1.0 / 30); in.newFrame(); in.down[KEY_ENTER] = false;
        for (int n = 0; n < 30 * 60 && g.state != GameState::SURFACE; n++) run(1.0 / 30);
        if (g.state != GameState::SURFACE) { printf("  the %s: no landing\n", name); continue; }
        run(4.0);
        SurfaceView& S = const_cast<SurfaceView&>(g.testSurface());
        int dryK = 0;   // a dry day (its sun as high: whole turns on)
        for (; dryK < 40 && (S.env.rain > 0.02 || S.env.cloudCover > 0.6); dryK++) { g.testSetTime(tDay + (dryK + 1) * 3 * turn); run(0.3); }
        S.flocks.clear(); S.critters.clear();
        // the settlement's middle in the local metres; the building of the biggest footprint on dry ground within 400 m of the landing (the layout's,
        // whatever stands of it), seen from 40 m on the landing's side (the street) and from 30 m up 90 m off
        double cx = 0, cz = 0; S.site.localAt(pl->unit, cx, cz);
        const double lx = S.player.x, lz = S.player.z, dl = std::max(1.0, std::hypot(cx - lx, cz - lz));
        double bx = cx, bz = cz, bestArea = 0, bestTop = 0;
        S.forNearbyRuins(lx, lz, 2, [&](const Ruin& r, const SurfaceView::RuinCell& c, int) {
            std::vector<double> top(c.spec.buildings.size(), 0.0);
            for (const Form& f : c.elems[0]) if (f.kind == FK_BOX && f.part == 0 && f.building >= 0 && f.building < (int)top.size()) top[f.building] = std::max(top[f.building], f.y1);
            for (size_t k = 0; k < top.size(); k++) {
                const Building& B = c.spec.buildings[k];
                const double x = r.x + B.x, z = r.z + B.z, wl = S.site.waterAt(x, z), area = B.hw * B.hd;
                if (area <= bestArea || B.kind >= BK_MOUND || std::hypot(x - lx, z - lz) > 400 || (wl > -1e8 && S.site.groundHeight(x, z) < wl + 0.5)) continue;
                bestArea = area; bestTop = top[k]; bx = x; bz = z;
            }
        });
        const double db = std::max(1.0, std::hypot(bx - lx, bz - lz)), ux = (bx - lx) / db, uz = (bz - lz) / db, toward = std::atan2(ux, uz);
        const double ix = bx - ux * 40, iz = bz - uz * 40;
        printf("  the %s (%s, %d buildings, radius %.0f m), landed %.0f m from its middle; seen: a building of %.0f m2 %.0f m off, its walls %.1f m high\n", name,
               SETTLEMENT_CLASS_NAMES[pl->sclass], pl->buildings, pl->radiusM, dl, 4 * bestArea, db, bestTop);
        auto frame = [&](const std::string& file) {
            run(0.6);
            writePNG(file.c_str(), g.output(), FBW, FBH);
            framed++;
            int covered = 0, shut = 0, lit = 0;
            S.testCoverFaces(covered, shut, lit);
            if (covered + shut + lit > 0) printf("      %s: %d faces drawn wholly under the cover, %d with windows grown shut, %d lit at night\n", file.substr(file.rfind('/') + 1).c_str(),
                                                 covered, shut, lit);
        };
        auto street = [&]() { S.leaveFreeCam(); S.player.x = ix; S.player.z = iz; S.player.y = S.site.surfaceHeight(ix, iz); g.testSurfaceFace(toward, 0.1); };
        auto above = [&]() {   // 30 m up, 90 m off the building on the landing's side, looking at its foot
            S.player.x = bx - ux * 90; S.player.z = bz - uz * 90; S.player.y = S.site.surfaceHeight(S.player.x, S.player.z);
            formsCamera(S, 30.0, bx, S.site.surfaceHeight(bx, bz) + 2, bz, g.testT());
        };
        if (which >= 2) {   // a stone rotter twenty steps on (its pieces laid out as it left them), or the city as generated
            wanted++;
            above();
            int pieces = 0; double tops = 0; int walls = 0;
            S.forNearbyRuins(ix, iz, 1, [&](const Ruin&, const SurfaceView::RuinCell& c, int) {
                pieces += (int)c.elems[0].size();
                for (const Form& f : c.elems[0]) if (f.kind == FK_BOX && f.part == 0) { tops += f.y1; walls++; }
            });
            printf("    %s: %d pieces near, their walls %.2f m high on average\n", which == 2 ? "crumbled by a stone rotter twenty steps on" : "as generated", pieces,
                   walls ? tops / walls : 0.0);
            frame(fmt("shots/tests/overgrowth/ruins_%s.png", name));
            continue;
        }
        for (int step : {9, 14, 20, 32}) {
            wanted += which == 0 ? 2 : 1;
            if (step != 9) {   // (the game's layer too: it hands the ground's ruins the world now)
                const MadeWorld L = growthAtStep(Rkm, place, flesh, step);
                g.testMadeSet(w.body, L); growthLayerOn(S, L);
            }
            printf("    step %2d: %s\n", step, ruinsGrownLine(ruinsGrown(S, ix, iz)).c_str());
            if (which == 0) { street(); frame(fmt("shots/tests/overgrowth/ruins_city_%02d_street.png", step)); }
            above(); frame(fmt("shots/tests/overgrowth/ruins_%s_%02d.png", name, step));
            S.leaveFreeCam();
            if (step == 14 && which == 0) {   // a doorway grown shut, from 3.5 m outside it
                wanted++;
                double best = 1e18, mx = 0, mz = 0, nx = 0, nz = 0;
                S.forNearbyRuins(ix, iz, 1, [&](const Ruin& r, const SurfaceView::RuinCell& c, int) {
                    for (const Form& f : c.elems[0]) {
                        if (f.growth != FG_MEMBRANE || f.building < 0 || f.y1 - f.y0 < 1.6) continue;
                        const Building& B = c.spec.buildings[f.building];
                        double ax = std::sin(f.heading), az = std::cos(f.heading);   // its face's normal, out of the building
                        if (ax * (f.x - B.x) + az * (f.z - B.z) < 0) { ax = -ax; az = -az; }
                        const double x = r.x + f.x, z = r.z + f.z, d = std::hypot(x - ix, z - iz);
                        if (d < best) { best = d; mx = x; mz = z; nx = ax; nz = az; }
                    }
                });
                if (best < 1e17) {
                    S.player.x = mx + nx * 3.5; S.player.z = mz + nz * 3.5; S.player.y = S.site.surfaceHeight(S.player.x, S.player.z);
                    g.testSurfaceFace(std::atan2(-nx, -nz), -0.05);
                    printf("    a doorway grown shut %.0f m from the street's view\n", best);
                    frame("shots/tests/overgrowth/ruins_doorway.png");
                } else printf("    no doorway grown shut near\n");
            }
        }
    }
    {   // KI-481: the scene's town (as if left 300 years ago) at night (the sun 25 degrees under) under a wet flesh whose veins glow, fourteen steps
        // on, from 30 m up 90 m off its biggest building: its cover's light on the walls taken and on its pieces (the test world's cities lie in a
        // day that does not end)
        StarSystem ts; SurfaceView sv; StarNeighborhood nb; SpaceRenderer sr; double t = 0;
        wanted++;
        if (setupSceneForType(-1, 25, -25 * DEG, 0.6, -0.05, sv, ts, nb, t, SCENE_TOWN)) {
            sv.flocks.clear(); sv.critters.clear();
            const SurfaceView::RuinCell* near[SurfaceView::SETTLEMENTS_NEAR];
            const int n = sv.settlementsNear(sv.player.x, sv.player.z, near);
            const SurfaceView::RuinCell* town = nullptr;
            for (int i = 0; i < n; i++) if (!town || near[i]->spec.buildings.size() > town->spec.buildings.size()) town = near[i];
            if (town) {   // (the town as if its people ended 300 years ago, its walls standing, laid out beside it on dry ground)
                Culture young = sv.cultures[town->spec.people == 1 && sv.peoples > 1 ? 1 : 0];
                young.ageYears = young.weatherYears = 300; young.decay = decayOf(300, young.wet);
                RuinSpec spec = town->spec; spec.people = 0;
                std::vector<Form> P[3]; std::vector<FormOpening> openings;
                for (int l = 0; l < 3; l++) ruinElements(spec, young, l, P[l], l == 0 ? &openings : nullptr);
                double tx = town->r.x, tz = town->r.z;
                for (int k = 0; k < 8; k++) {
                    const double x = town->r.x + std::sin(k * TAU / 8) * (spec.size + 400), z = town->r.z + std::cos(k * TAU / 8) * (spec.size + 400);
                    const SurfaceSample q = sampleSurface(sv.site.gen, sv.site.unitAt(x, z), 16);
                    if (q.material != MAT_WATER && q.material != MAT_SNOW && q.material != MAT_ICE && !(q.water > -1e8 && q.height < q.water + 2)) { tx = x; tz = z; break; }
                }
                sv.testPlaceSettlement(tx, tz, spec, P, &openings);
                const MadeGrowth glowing = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, true, false);
                growthLayerOn(sv, growthAtStep(ts.bodies[sv.site.body].radiusKm, sv.site.unitAt(tx, tz), glowing, 14));
                double bx = tx, bz = tz, best = 0;
                for (const Building& B : spec.buildings) if (B.hw * B.hd > best && B.kind < BK_MOUND) { best = B.hw * B.hd; bx = tx + B.x; bz = tz + B.z; }
                sv.player.x = tx; sv.player.z = tz - spec.size - 30;
                const double lx = sv.player.x, lz = sv.player.z, db = std::max(1.0, std::hypot(bx - lx, bz - lz));
                sv.player.x = bx - (bx - lx) / db * 90; sv.player.z = bz - (bz - lz) / db * 90; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                formsCamera(sv, 30.0, bx, sv.site.surfaceHeight(bx, bz) + 2, bz, t);
                const std::vector<uint32_t> px = growthFrame(sv, nb, sr, t);
                writePNG("shots/tests/overgrowth/ruins_night.png", px.data(), FBW, FBH);
                framed++;
                int covered = 0, shut = 0, lit = 0;
                sv.testCoverFaces(covered, shut, lit);
                printf("  the scene's town as if left 300 years ago at night (the sun %.0f degrees up, the cover's light %.2f): %d faces drawn wholly under the "
                       "cover, %d with windows grown shut, %d lit at night\n", sv.env.sun.altitude / DEG, sv.growthGlow, covered, shut, lit);
            } else printf("  no settlement by the town scene at night\n");
        } else printf("  no town scene at night\n");
    }
    printf("overgrowth ruins: %d of %d frames  (%.1f s)\n", framed, wanted, nowSec() - t0);
    return framed == wanted && wanted > 0 ? 0 : 1;
}
