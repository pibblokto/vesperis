// KI-463 and KI-476 to KI-481 (B-460 to B-466): what a seeded line's overgrowth makes of the ground and its ruins, checked where each fix
// shows: a made crust in its form's texture and a lush world's denser forest; the structures fixed to the world (two landings a kilometre
// apart see the same ones) and a vial taken off one; a stone rotter's eating only where its line lives; the ruins' cover and crumble following
// the steps while the explorer is on the ground
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/seeding.h"
#include "life/samples.h"
#include "core/sphere.h"
#include "surface/textures.h"
#include "galaxy/drainage.h"
#include <algorithm>
#include <cmath>
#include "peoples/ruins.h"
#include "peoples/genres_schema.h"

namespace {
long tileDiff(const GrainTexture& a, const GrainTexture& b) {
    long d = 0;
    for (size_t i = 0; i < a.v.size() && i < b.v.size(); i++) d += std::abs((int)a.v[i] - (int)b.v[i]);
    return d;
}
}   // namespace

void unitSeedingGrown(const Check& check) {
    GrowthGround land;
    const bool landOk = growthGround(land, false);
    {   // B-460: the crust's three forms each its own tile (the world's own crust the rosettes, as before); a crust line's ground drawn in its
        // form; a lush world's forest 1.45 times as many trees or more (none where nothing made it lush)
        GrainTexture t[4];
        const uint64_t seed = 0x5EEDC2ULL;
        for (int k = 0; k < 4; k++) buildMesoTile(t[k], MAT_CRUST, seed, k - 1);   // the world's own, then polygons, ripples, rosettes
        const long d01 = tileDiff(t[1], t[2]), d12 = tileDiff(t[2], t[3]), d02 = tileDiff(t[1], t[3]), own = tileDiff(t[0], t[3]);
        long drawn = -1; int plain = 0, lush = 0;
        if (landOk) {
            StarSystem sys = land.sys;
            MadeWorld crust; crust.land.on = crust.land.tinted = true; crust.land.kind = MGT_CRUSTS; crust.land.variant = 1; crust.land.tint = RGB(0.5f, 0.45f, 0.4f);
            crust.key = 1;
            SurfaceView sv; sv.site.made = crust; sv.init(&sys, land.body, land.lat, land.lon, land.day);
            GrainTexture want; buildMesoTile(want, MAT_CRUST, sv.site.gen.seed, 1);
            const GrainTexture* got = sv.testMesoFor(MAT_CRUST);
            drawn = got ? tileDiff(*got, want) : -1;
            for (int pass = 0; pass < 2; pass++) {
                MadeWorld m; m.lush = pass; m.key = pass ? 1 : 0;
                SurfaceView S; S.site.made = m; S.init(&sys, land.body, land.lat, land.lon, land.day);
                int n = 0;
                for (int cz = -12; cz < 12; cz++) for (int cx = -12; cx < 12; cx++) S.forTrees(cx, cz, [&](const TreeInst& T) { n += !T.dead; });
                (pass ? lush : plain) = n;
            }
        }
        check("seeding: a crust's forms and a lush forest", d01 > 0 && d12 > 0 && d02 > 0 && own == 0 && drawn == 0 && plain > 20 && lush >= 1.45 * plain,
              fmt("(the crust's tiles: polygons and ripples %ld apart, ripples and rosettes %ld, polygons and rosettes %ld, the world's own and the rosettes %ld; a "
                  "crust line's ground %s; a forest's edge: %d trees, %d lush)", d01, d12, d02, own, drawn == 0 ? "in its form" : (drawn < 0 ? "NOT DRAWN" : "NOT IN ITS FORM"),
                  plain, lush));
    }
    if (!landOk) { check("seeding: an overgrowth's structures fixed to the world", false, "(no flora world with a forest's edge)"); return; }
    {   // B-461: a flesh over the forest's edge seen from a landing there and from one a kilometre north (walked to the first): within 64 m of the
        // first the same structures at the same places (5 cm) and of the same shapes; a vial off one names it, the cover's own elsewhere
        StarSystem sys = land.sys;
        const Body& b = sys.bodies[land.body];
        const double R = b.radiusKm * 1000;
        const MadeWorld m = growthLayer(b.radiusKm, StarSystem::bodyFromLatLon(land.lat, land.lon), growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.6f, 0.3f, 0.3f), 0.5, 0.3, 0, false, false));
        SurfaceView A, B;
        A.site.made = m; A.init(&sys, land.body, land.lat, land.lon, land.day);
        B.site.made = m; B.init(&sys, land.body, land.lat + 1000 / R, land.lon, land.day);
        double ax = 0, az = 0; B.site.localAt(A.site.up0, ax, az);
        B.player.x = ax; B.player.z = az; B.player.y = B.site.surfaceHeight(ax, az);
        { Input in; for (int k = 0; k < 30; k++) B.update(1.0 / 30, in, land.day + k / 30.0, false); }
        struct Seen { Vec3 u; int shape; };
        auto read = [&](SurfaceView& S, double x0, double z0) {
            std::vector<Seen> out;
            const int c0x = (int)std::floor(x0 / 16), c0z = (int)std::floor(z0 / 16);
            for (int cz = c0z - 6; cz <= c0z + 6; cz++)
                for (int cx = c0x - 6; cx <= c0x + 6; cx++) S.forGrowths(cx, cz, [&](const GrowthInst& g) { out.push_back({S.site.unitAt(g.x, g.z), g.shape}); });
            return out;
        };
        const std::vector<Seen> sa = read(A, 0, 0), sb = read(B, ax, az);
        int inner = 0, matched = 0;
        for (const Seen& p : sa) {
            if (std::acos(clampd(dot(p.u, A.site.up0), -1, 1)) * R > 64) continue;
            inner++;
            bool hit = false;
            for (const Seen& q : sb) hit = hit || (q.shape == p.shape && length(q.u - p.u) * R < 0.05);
            matched += hit;
        }
        // a vial off the nearest structure standing over a step, and off the cover 3 m clear of any
        std::string onOne, offAll;
        double best = 1e18; GrowthInst near;
        for (int cz = -4; cz < 4; cz++) for (int cx = -4; cx < 4; cx++)
            A.forGrowths(cx, cz, [&](const GrowthInst& g) { const double d = g.x * g.x + g.z * g.z; if (g.collides && g.r >= 0.4 && g.len == 0 && d < best) { best = d; near = g; } });
        if (best < 1e17) onOne = A.readSample(near.x, near.z, false).name;
        for (int k = 0; k < 400 && offAll.empty(); k++) {
            const double x = -60 + (k % 20) * 6.0, z = -60 + (k / 20) * 6.0;
            if (madeGrowthMaterial(A.site.sampleAt(x, z, 1.0).material) && A.growthAt(x, z, 3.0) < 0) offAll = A.readSample(x, z, false).name;
        }
        check("seeding: an overgrowth's structures fixed to the world", inner > 20 && matched >= 0.9 * inner && onOne.find(" FROM ") != std::string::npos &&
                                                                         !offAll.empty() && offAll.find(" FROM ") == std::string::npos,
              fmt("(a flesh over a %s's forest edge: %d structures within 64 m of a landing, %d of them seen again in their places from a landing a kilometre north; a vial "
                  "off one \"%s\", off the cover clear of them \"%s\")", KINDS[b.type].name, inner, matched, onOne.c_str(), offAll.c_str()));
    }
    {   // B-461: a vial off a seeded line's overgrowth holds the line: J-12's record case (a sea line poisoned, its STOP cut and its cells bound) at
        // its 64th step, a vial off its cover on the land nearest its place (its microhabitat a cover's, as a vial on the ground reads it)
        SeedRecordCase rc;
        int tried = 0, held = 0;
        std::string at = "no overgrown land found";
        if (seedRecordCase(rc)) {
            TestGame g; seedRecordGame(g, rc, "unit_seeding_grown");
            const Body& b = rc.c.sys.bodies[rc.c.body];
            BodyGen gen = BodyGen::make(b); gen.made = g.testMadeNow(rc.c.body);
            const Vec3 place = StarSystem::bodyFromLatLon(rc.c.lat, rc.c.lon);
            for (int k = 0; k < 4000 && tried < 8; k++) {   // out from its place in rings 20 km apart, sixteen points each
                const double d = (50 + 20 * (k / 16)) * 1000, a = (k % 16 + 0.5 * ((k / 16) & 1)) * TAU / 16;
                const Vec3 u = offsetUnit(place, std::sin(a) * d, std::cos(a) * d, b.radiusKm * 1000);
                SurfaceSample q; { DrainageOff off; q = sampleSurface(gen, u, 1.0); }
                if (!madeGrowthMaterial(q.material)) continue;
                Vial v; v.sx = rc.c.sys.star.sx; v.sy = rc.c.sys.star.sy; v.sz = rc.c.sys.star.sz; v.body = rc.c.body; v.unit = u; v.clock = g.guide.clock;
                v.hab.niche = NI_MAT; v.hab.bloom = true; v.hab.salt = 0;
                v.hab.meanC = std::round(placeMeanC(gen, b.tempK, rc.c.sys.sunless(), u, q.height) * 10) / 10;
                Sample smp = sampleOf(lifeOf(rc.c.sys)->bodies[rc.c.body], v.hab);
                tried++;
                held += !g.testVialLines(v, smp).empty();
                if (tried == 1) at = fmt("%.0f km from its place, %.1f C", d / 1000, v.hab.meanC);
            }
            remove(g.guidePath.c_str());
        }
        check("seeding: a vial off an overgrowth holds its line", tried >= 4 && held == tried,
              fmt("(J-12's record case at its 64th step: %d vials off its cover (the first %s), %d holding its line)", tried, at.c_str(), held));
    }
    {   // B-463: an arcade's four bays and a gate's way grow shut: the bays left standing (a broken arcade's fewer), a gate's opening as its
        // pieces are laid out (two piers, a gatehouse's arch or a palisade's posts), and on a building of the scene's town under a flesh eleven
        // steps on each bay of an arcade put against its wall gets a membrane shaped to its head
        Form arc; arc.kind = FK_ARCH; arc.sub = 0; arc.x = 3; arc.z = -2; arc.heading = 0.4; arc.hx = 6; arc.hz = 0.5; arc.y0 = 0; arc.y1 = 5;
        arc.p[0] = 4; arc.p[1] = 0.32; arc.p[2] = 2.5; arc.building = 0; arc.seed = 0xA7C;
        std::vector<FormOpening> bays, broken;
        archOpenings(arc, bays);
        Form ruined = arc; ruined.broken = 0.8;
        archOpenings(ruined, broken);
        StarSystem sys; SurfaceView sv; StarNeighborhood nb; double t = 0;
        int gates = 0, gateWays = 0, arcMembranes = 0, shaped = 0;
        std::string shutless;   // (the ages whose gate has no way to grow shut)
        if (setupSceneForType(-1, 25, 35 * DEG, 0.6, -0.05, sv, sys, nb, t, SCENE_TOWN)) {
            sv.flocks.clear(); sv.critters.clear();
            const SurfaceView::RuinCell* near[SurfaceView::SETTLEMENTS_NEAR];
            const int n = sv.settlementsNear(sv.player.x, sv.player.z, near);
            const SurfaceView::RuinCell* town = nullptr;
            for (int i = 0; i < n; i++) if (!town || near[i]->spec.buildings.size() > town->spec.buildings.size()) town = near[i];
            if (town) {
                Culture young = sv.cultures[town->spec.people == 1 && sv.peoples > 1 ? 1 : 0];
                young.ageYears = young.weatherYears = 300; young.decay = decayOf(300, young.wet);
                for (int a = 0; a < AGE_COUNT; a++) {   // a gate in each age's way: its opening among the openings, or its arch's bay
                    RuinSpec g = town->spec; g.people = 0; g.buildings.resize(1);
                    Building& B = g.buildings[0]; B.kind = BK_GATE; B.age = a; B.x = B.z = 0; B.hw = 2.2; B.hd = 1; B.hWall = a % 2 ? 6.0 : 3.0;
                    std::vector<Form> P; std::vector<FormOpening> ops;
                    ruinElements(g, young, 0, P, &ops);
                    if (P.empty()) continue;
                    gates++;
                    for (const Form& f : P) if (f.kind == FK_ARCH) archOpenings(f, ops);
                    gateWays += !ops.empty();
                    if (ops.empty()) shutless += std::string(shutless.empty() ? "" : ", ") + AGE_KEYS[a];
                }
                RuinSpec spec = town->spec; spec.people = 0;
                std::vector<Form> P[3]; std::vector<FormOpening> openings;
                for (int l = 0; l < 3; l++) ruinElements(spec, young, l, P[l], l == 0 ? &openings : nullptr);
                const Building& B0 = spec.buildings[0];
                Form a = arc; a.x = B0.x; a.z = B0.z; a.heading = B0.heading; a.hz = 0.4;   // (across its middle)
                std::vector<Form> with;
                for (const Form& f : P[0]) { with.push_back(f); }
                auto at = std::find_if(with.begin(), with.end(), [](const Form& f) { return f.building > 0; });   // (after building 0's own pieces)
                with.insert(at, a);
                P[0] = with;
                double tx = sv.player.x + 400, tz = sv.player.z;   // beside it on dry ground
                for (int k = 0; k < 8; k++) {
                    const double x = town->r.x + std::sin(k * TAU / 8) * (spec.size + 400), z = town->r.z + std::cos(k * TAU / 8) * (spec.size + 400);
                    const SurfaceSample q = sampleSurface(sv.site.gen, sv.site.unitAt(x, z), 16);
                    if (q.material != MAT_WATER && q.material != MAT_SNOW && q.material != MAT_ICE && !(q.water > -1e8 && q.height < q.water + 2)) { tx = x; tz = z; break; }
                }
                sv.testPlaceSettlement(tx, tz, spec, P, &openings);
                const MadeGrowth flesh = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, false, false);
                const SurfaceView::RuinCell& placed = sv.placedCells.back();
                growthLayerOn(sv, growthAtStep(sys.bodies[sv.site.body].radiusKm, sv.site.unitAt(placed.r.x, placed.r.z), flesh, 14));
                for (const Form& f : placed.elems[0]) if (f.growth == FG_MEMBRANE && f.building == 0 && f.p[0] >= 1) { arcMembranes++; shaped += f.p[2] > 0; }
            }
        }
        check("seeding: arches and gates grown shut", bays.size() == 4 && broken.size() < 4 && gates > 0 && gateWays == gates && arcMembranes == 4 && shaped == 4,
              fmt("(an arcade of four bays: %zu standing, %zu once broken; %d gates of the ages, %d with a way to grow shut%s%s; an arcade against a building of the "
                  "scene's town under a flesh fourteen steps on: %d of its bays grown shut, %d shaped to their heads)", bays.size(), broken.size(), gates, gateWays,
                  shutless.empty() ? "" : ", none: ", shutless.c_str(), arcMembranes, shaped));
    }
    {   // B-465: the scene's town: a stone rotter whose band of warmth leaves the town's out eats none of it, one whose band holds it eats it; a flesh
        // over the town at its 11th step handed on at its 14th while the explorer stands there: the cover climbs on, the ground's layer the landing's
        StarSystem sys; SurfaceView sv; StarNeighborhood nb; double t = 0;
        if (!setupSceneForType(-1, 25, 35 * DEG, 0.6, -0.05, sv, sys, nb, t, SCENE_TOWN)) { check("seeding: the ruins where the rotter lives and as the steps pass", false, "(no town)"); return; }
        sv.flocks.clear(); sv.critters.clear();
        const SurfaceView::RuinCell* near[SurfaceView::SETTLEMENTS_NEAR];
        const int n = sv.settlementsNear(sv.player.x, sv.player.z, near);
        const SurfaceView::RuinCell* town = nullptr;
        for (int i = 0; i < n; i++) if (!town || near[i]->spec.buildings.size() > town->spec.buildings.size()) town = near[i];
        if (!town) { check("seeding: the ruins where the rotter lives and as the steps pass", false, "(no settlement near the town scene)"); return; }
        const double R = sys.bodies[sv.site.body].radiusKm;
        const Vec3 mid = sv.site.unitAt(town->r.x, town->r.z);
        SurfaceSample here; { DrainageOff off; here = sampleSurface(sv.site.gen, mid, 16); }
        const double meanC = madeMeanAt(sv.site.gen, sv.site.gen.made, mid, here, false, 0);
        auto rotter = [&](double lo, double hi) {   // its line let go at the town's middle twenty steps ago, living in [lo, hi] C
            MadeWorld m; m.lines = 1; m.step = 20;
            MadeSpread& sp = m.spread[0];
            sp.place = mid; sp.reach = madeReachOfStep(20, R); sp.pits = 20; sp.makes = 1 << MC_LAND; sp.landOn = ML_ROCK; sp.land = true; sp.tLo = lo; sp.tHi = hi;
            m.land.on = m.land.tinted = true; m.land.kind = MGT_PITTED; m.grounds = MGT_PITTED; m.radiusKm = R; m.key = madeKey(m);
            return m;
        };
        auto eaten = [&](const MadeWorld& m) {
            sv.ruinsMade = m;
            int k = 0;
            for (const Building& B : town->spec.buildings) k += sv.stoneRotOf(town->r.x + B.x, town->r.z + B.z) > 0;
            return k;
        };
        const int outOfBand = eaten(rotter(meanC + 10, meanC + 40)), inBand = eaten(rotter(meanC - 25, meanC + 25)), nb0 = (int)town->spec.buildings.size();
        // the steps passing on the ground: the cover's climb after its 14th step handed on, the ground's layer kept
        const MadeGrowth flesh = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, false, false);
        growthLayerOn(sv, growthAtStep(R, mid, flesh, 11));
        const uint64_t groundKey = sv.site.gen.made.key;
        auto climbs = [&]() { double c = 0; for (size_t b = 0; b + 1 < town->climb.size(); b++) c += std::max(0.0f, town->climb[b]); return c; };
        const double before = climbs();
        sv.setMadeNow(growthAtStep(R, mid, flesh, 14));
        { Input in; sv.update(1.0 / 30, in, t, false); }
        const double after = climbs();
        check("seeding: the ruins where the rotter lives and as the steps pass", outOfBand == 0 && inBand > 0 && before > 0 && after > before + 1 &&
                                                                                    sv.site.gen.made.key == groundKey,
              fmt("(the scene's town of %d buildings at %.1f C: a stone rotter living from %.0f to %.0f C eats %d of them, one from %.0f to %.0f C %d; a flesh over it "
                  "climbed %.0f m in all at its 11th step, %.0f m once its 14th was handed on, the ground's layer %s)", nb0, meanC, meanC + 10, meanC + 40, outOfBand,
                  meanC - 25, meanC + 25, inBand, before, after, sv.site.gen.made.key == groundKey ? "the landing's" : "CHANGED"));
    }
}
