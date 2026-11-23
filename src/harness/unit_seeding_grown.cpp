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
        long drawn = -1, ownDrawn = -1; int plain = 0, lush = 0;
        if (landOk) {
            StarSystem sys = land.sys;
            MadeWorld crust; crust.land.on = crust.land.tinted = true; crust.land.kind = MGT_CRUSTS; crust.land.variant = 1; crust.land.tint = RGB(0.5f, 0.45f, 0.4f);
            crust.key = 1;
            SurfaceView sv; sv.site.made = crust; sv.init(&sys, land.body, land.lat, land.lon, land.day);
            GrainTexture want, mine; buildMesoTile(want, MAT_CRUST, sv.site.gen.seed, 1); buildMesoTile(mine, MAT_CRUST, sv.site.gen.seed);
            TerrainVertex made; made.material = MAT_CRUST; made.mtint = 255;   // (the crust line's crust: its look's share over a half)
            const GrainTexture* got = sv.testTileOf(made);
            drawn = got ? tileDiff(*got, want) : -1;
            TerrainVertex own; own.material = MAT_CRUST;   // B-487: the world's own crust on the same landing
            const GrainTexture* gotOwn = sv.testTileOf(own);
            ownDrawn = gotOwn ? tileDiff(*gotOwn, mine) : -1;
            for (int pass = 0; pass < 2; pass++) {
                MadeWorld m; m.lush = pass; m.key = pass ? 1 : 0;
                SurfaceView S; S.site.made = m; S.init(&sys, land.body, land.lat, land.lon, land.day);
                int n = 0;
                for (int cz = -12; cz < 12; cz++) for (int cx = -12; cx < 12; cx++) S.forTrees(cx, cz, [&](const TreeInst& T) { n += !T.dead; });
                (pass ? lush : plain) = n;
            }
        }
        check("seeding: a crust's forms and a lush forest", d01 > 0 && d12 > 0 && d02 > 0 && own == 0 && drawn == 0 && ownDrawn == 0 && plain > 20 && lush >= 1.45 * plain,
              fmt("(the crust's tiles: polygons and ripples %ld apart, ripples and rosettes %ld, polygons and rosettes %ld, the world's own and the rosettes %ld; a "
                  "crust line's ground %s, the world's own crust beside it %s; a forest's edge: %d trees, %d lush)", d01, d12, d02, own,
                  drawn == 0 ? "in its form" : (drawn < 0 ? "NOT DRAWN" : "NOT IN ITS FORM"), ownDrawn == 0 ? "in its own" : (ownDrawn < 0 ? "NOT DRAWN" : "IN THE LINE'S FORM"),
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
    {   // B-483: a flesh over the whole world seen from a landing on one of the twelve edges of the cube its structures' lattice is cut from:
        // every structure of the world cells within 80 m stands in one local cell, those of the cells along the edge too (by the old code a
        // structure whose local cell's middle lay on the other face stood in none: a strip along the edge went bare)
        StarSystem sys = land.sys;
        const Body& b = sys.bodies[land.body];
        const BodyGen gen = BodyGen::make(b);
        Vec3 edge; bool found = false;
        for (int e = 0; e < 12 && !found; e++)   // (each edge: along one axis, the other two at +-1; the first point the cover can lie on)
            for (int k = 0; k < 48 && !found; k++) {
                const int ax = e / 4;
                double c[3]; c[ax] = -0.85 + 1.7 * k / 47.0; c[(ax + 1) % 3] = (e & 1) ? -1 : 1; c[(ax + 2) % 3] = (e & 2) ? -1 : 1;
                const Vec3 u = normalize(Vec3(c[0], c[1], c[2]));
                SurfaceSample q; { DrainageOff off; q = sampleSurface(gen, u, 16); }
                if (q.material == MAT_LAVA || q.material == MAT_SNOW || q.material == MAT_ICE || madeWetAt(q)) continue;
                edge = u; found = true;
            }
        int total = 0, alongEdge = 0, lost = 0, twice = 0, faces = 0;
        if (found) {
            double lat, lon; StarSystem::latLonFromBody(edge, lat, lon);
            SurfaceView A; A.site.made = growthLayer(b.radiusKm, edge, growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.6f, 0.3f, 0.3f), 0.5, 0.3, 0, false, false));
            A.init(&sys, land.body, lat, lon, land.day);
            A.growthsOwned(0, 0);   // (the lattice kept under this layer's key from here: a cell's list read below is not emptied under it)
            std::vector<uint64_t> cells;
            int N = 1;
            for (double z = -96; z <= 96; z += 2)
                for (double x = -96; x <= 96; x += 2) {
                    int f, i, j;
                    N = SurfaceView::growthLatticeAt(A.site.unitAt(x, z), A.site.R, f, i, j);
                    const uint64_t key = ((uint64_t)f << 42) | ((uint64_t)i << 21) | (uint64_t)j;
                    if (std::find(cells.begin(), cells.end(), key) == cells.end()) cells.push_back(key);
                }
            for (uint64_t key : cells) {
                const int f = (int)(key >> 42), i = (int)((key >> 21) & 0x1FFFFF), j = (int)(key & 0x1FFFFF);
                faces |= 1 << f;
                const bool rim = i == 0 || j == 0 || i == N - 1 || j == N - 1;   // (a cell along a face's edge)
                const std::vector<GrowthInst> list = A.growthCellOf(f, i, j, N).s;   // (a copy: `growthsOwned` below adds cells to the lattice)
                for (const GrowthInst& S : list) {
                    if (std::fabs(S.x) > 80 || std::fabs(S.z) > 80) continue;
                    total++; alongEdge += rim;
                    int owned = 0;
                    for (const GrowthInst& T : A.growthsOwned((int)std::floor(S.x / 16), (int)std::floor(S.z / 16))) owned += T.hs == S.hs;
                    lost += owned == 0; twice += owned > 1;
                }
            }
        }
        int nFaces = 0; for (int f = 0; f < 6; f++) nFaces += (faces >> f) & 1;
        check("seeding: an overgrowth's structures across the lattice's cube edges", found && nFaces >= 2 && alongEdge > 20 && lost == 0 && twice == 0,
              fmt("(a flesh over a %s, landed on a cube's edge (%d faces round it): %d structures within 80 m, %d of them in the cells along the edge; %d in no local "
                  "cell, %d in two)", KINDS[b.type].name, nFaces, total, alongEdge, lost, twice));
    }
    {   // B-484: a flesh whose cover's edge runs through a landing, seen from there and from a landing a kilometre north walked to it: within 64 m
        // the same structures each way, those by the edge too (by the old code a cell by the edge stood or not by the landing's own 16 m samples)
        StarSystem sys = land.sys;
        const Body& b = sys.bodies[land.body];
        const double R = b.radiusKm * 1000;
        const Vec3 here = StarSystem::bodyFromLatLon(land.lat, land.lon), place = offsetUnit(here, 0, -50000, R);
        MadeGrowth G = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.6f, 0.3f, 0.3f), 0.5, 0.3, 0, false, false);
        const uint64_t seed = BodyGen::make(b).seed;
        double lo = 0, hi = 0.1;   // (its reach in radians, halved toward the one whose edge stands at the landing)
        for (int it = 0; it < 60; it++) {
            G.reach = 0.5 * (lo + hi);
            int line; const double k = madeGrowthAt(growthLayer(b.radiusKm, place, G), here, seed, line);
            (k < 0.5 ? lo : hi) = G.reach;
        }
        G.reach = hi;
        const MadeWorld m = growthLayer(b.radiusKm, place, G);
        SurfaceView A, B;
        A.site.made = m; A.init(&sys, land.body, land.lat, land.lon, land.day);
        B.site.made = m; B.init(&sys, land.body, land.lat + 1000 / R, land.lon, land.day);
        double ax = 0, az = 0; B.site.localAt(A.site.up0, ax, az);
        B.player.x = ax; B.player.z = az; B.player.y = B.site.surfaceHeight(ax, az);
        { Input in; for (int k = 0; k < 30; k++) B.update(1.0 / 30, in, land.day + k / 30.0, false); }
        auto read = [&](SurfaceView& S, double x0, double z0) {
            std::vector<std::pair<Vec3, int>> out;
            const int c0x = (int)std::floor(x0 / 16), c0z = (int)std::floor(z0 / 16);
            for (int cz = c0z - 6; cz <= c0z + 6; cz++)
                for (int cx = c0x - 6; cx <= c0x + 6; cx++) S.forGrowths(cx, cz, [&](const GrowthInst& g) { out.push_back({S.site.unitAt(g.x, g.z), g.shape}); });
            return out;
        };
        const auto sa = read(A, 0, 0), sb = read(B, ax, az);
        int innerA = 0, matchedA = 0, innerB = 0, matchedB = 0, covered = 0, bare = 0;
        auto within = [&](const Vec3& u) { return std::acos(clampd(dot(u, A.site.up0), -1, 1)) * R <= 64; };
        for (const auto& p : sa) {
            if (!within(p.first)) continue;
            innerA++;
            bool hit = false;
            for (const auto& q : sb) hit = hit || (q.second == p.second && length(q.first - p.first) * R < 0.05);
            matchedA += hit;
        }
        for (const auto& q : sb) {
            if (!within(q.first)) continue;
            innerB++;
            bool hit = false;
            for (const auto& p : sa) hit = hit || (q.second == p.second && length(q.first - p.first) * R < 0.05);
            matchedB += hit;
        }
        for (int k = 0; k < 64; k++) {   // (the cover's share of the ground within 64 m: its edge runs through it)
            const double x = -56 + (k % 8) * 16.0, z = -56 + (k / 8) * 16.0;
            (madeGrowthMaterial(A.site.sampleAt(x, z, 16).material) ? covered : bare)++;
        }
        check("seeding: an overgrowth's structures by its cover's edge fixed to the world", innerA > 10 && covered >= 8 && bare >= 8 && matchedA == innerA && matchedB == innerB,
              fmt("(a flesh's cover's edge through a landing on a %s, %d of 64 points round it covered: %d structures within 64 m, %d of them seen again from a landing a "
                  "kilometre north; it saw %d there, %d of them seen from the first)", KINDS[b.type].name, covered, innerA, matchedA, innerB, matchedB));
    }
    {   // B-485: a landing 25 km from a flesh's place at its 9th step (its cover 5 km out) handed its 12th (63 km) while the explorer stands there:
        // the ground round the landing is covered now and its structures stand, the ground's layer the world now (by the old code the ground kept
        // the landing's layer: bare)
        StarSystem sys = land.sys;
        const Body& b = sys.bodies[land.body];
        const Vec3 here = StarSystem::bodyFromLatLon(land.lat, land.lon), place = offsetUnit(here, 0, -25000, b.radiusKm * 1000);
        const MadeGrowth flesh = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, false, false);
        const MadeWorld at9 = growthAtStep(b.radiusKm, place, flesh, 9), at12 = growthAtStep(b.radiusKm, place, flesh, 12);
        SurfaceView sv; sv.site.made = at9; sv.init(&sys, land.body, land.lat, land.lon, land.day);
        auto counts = [&](int& cover, int& structures) {
            cover = structures = 0;
            for (int cz = -8; cz <= 8; cz++) for (int cx = -8; cx <= 8; cx++) cover += madeGrowthMaterial(sv.site.lod0.at(cx, cz).material);
            for (int cz = -6; cz < 6; cz++) for (int cx = -6; cx < 6; cx++) sv.forGrowths(cx, cz, [&](const GrowthInst&) { structures++; });
        };
        int cover0, structures0, cover1, structures1;
        { Input in; sv.update(1.0 / 30, in, land.day, false); }
        counts(cover0, structures0);
        sv.setMadeNow(at12);
        { Input in; sv.update(1.0 / 30, in, land.day + 1.0 / 30, false); }
        sv.testWaitResample();
        counts(cover1, structures1);
        const bool now = sv.site.gen.made.key == at12.key;
        check("seeding: the ground's cover and structures follow the steps", cover0 == 0 && structures0 == 0 && cover1 > 144 && structures1 > 20 && now,
              fmt("(a landing on a %s 25 km from a flesh's place: at its 9th step %d of 289 points round it covered, %d structures; its 12th handed on, %d covered, %d "
                  "structures, the ground's layer %s)", KINDS[b.type].name, cover0, structures0, cover1, structures1, now ? "the world now" : "THE LANDING'S"));
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
    {   // B-486 and B-488: the scene's town as if left 300 years ago, placed beside it at night, fourteen steps under a glowing wet flesh: every face
        // wholly under its cover lit in its veins at night as the ground's cover is, every face across its edge with its cover in its own light
        // (by the old code in patches of the edge's noise, the wall's light); then a fungus that does not glow over part of it, the latest line: each
        // building takes the line whose cover lies at its own place, the flesh's faces still lit in their veins, the fungus's not (by the old code
        // the whole town took the line at its middle, its light the latest's)
        StarSystem sys; SurfaceView sv; StarNeighborhood nb; SpaceRenderer sr; double t = 0;
        int whole = 0, veined = 0, other = 0, otherVeined = 0, across = 0, acrossLit = 0, nb0 = 0;
        int byFlesh = 0, byFungus = 0, matched = 0, w2 = 0, v2 = 0, o2 = 0, ov2 = 0, a2 = 0, al2 = 0;
        bool placed = false;
        if (setupSceneForType(-1, 25, -25 * DEG, 0.6, -0.05, sv, sys, nb, t, SCENE_TOWN)) {
            sv.flocks.clear(); sv.critters.clear();
            const SurfaceView::RuinCell* near[SurfaceView::SETTLEMENTS_NEAR];
            const int n = sv.settlementsNear(sv.player.x, sv.player.z, near);
            const SurfaceView::RuinCell* town = nullptr;
            for (int i = 0; i < n; i++) if (!town || near[i]->spec.buildings.size() > town->spec.buildings.size()) town = near[i];
            if (town) {
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
                placed = true;
                nb0 = (int)spec.buildings.size();
                const double R = sys.bodies[sv.site.body].radiusKm;
                const Vec3 mid = sv.site.unitAt(tx, tz);
                const MadeGrowth glowing = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, true, false);
                const MadeWorld flesh = growthAtStep(R, mid, glowing, 14);
                double bx = tx, bz = tz, best = 0;
                for (const Building& B : spec.buildings) if (B.hw * B.hd > best && B.kind < BK_MOUND) { best = B.hw * B.hd; bx = tx + B.x; bz = tz + B.z; }
                auto frameOf = [&](const MadeWorld& m, double bx, double bz) {
                    growthLayerOn(sv, m);
                    sv.player.x = tx; sv.player.z = tz - spec.size - 30;
                    const double lx = sv.player.x, lz = sv.player.z, db = std::max(1.0, std::hypot(bx - lx, bz - lz));
                    sv.player.x = bx - (bx - lx) / db * 90; sv.player.z = bz - (bz - lz) / db * 90; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                    formsCamera(sv, 30.0, bx, sv.site.surfaceHeight(bx, bz) + 2, bz, t);
                    Input in;
                    sv.player.x = sv.freePos.x - std::sin(sv.freeYaw) * 40; sv.player.z = sv.freePos.z - std::cos(sv.freeYaw) * 40;
                    sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                    for (int k = 0; k < 4; k++) { sv.update(1.0 / 30, in, t, false); Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); }
                };
                frameOf(flesh, bx, bz);
                sv.testCoverLight(whole, veined, other, otherVeined, across, acrossLit);
                // the fungus's place 2 km east of the town's middle, its reach halved toward the one whose edge stands at the middle
                const Vec3 eastward = offsetUnit(mid, 2000, 0, R * 1000);
                MadeGrowth fungus = growthOfType(OG_FUNGUS, OGT_BLISTERED, RGB(0.2f, 0.3f, 0.7f), 0.5, 0.2, 0, false, false);
                double lo = 0, hi = 0.01;
                for (int it = 0; it < 60; it++) {
                    fungus.reach = 0.5 * (lo + hi);
                    MadeWorld one = growthLayer(R, eastward, fungus);
                    int line; (madeGrowthAt(one, mid, sv.site.gen.seed, line) < 0.5 ? lo : hi) = fungus.reach;
                }
                fungus.reach = hi;
                MadeWorld two = flesh;
                two.lines = 2; two.spread[1].place = eastward; two.spread[1].growth = fungus;
                two.growths |= 1 << OG_FUNGUS; two.growthLine = 1; two.key = madeKey(two);
                growthLayerOn(sv, two);
                const SurfaceView::RuinCell& c = sv.placedCells.back();
                double ax = bx, az = bz, nearest = 1e18;   // (the camera on a building of the fungus beside one of the flesh: both in view)
                for (int b = 0; b < nb0 && b < (int)c.lines.size(); b++)
                    for (int o = 0; o < nb0 && o < (int)c.lines.size(); o++) {
                        if (c.lines[b] != 1 || c.lines[o] != 0 || c.climb[b] < 0 || c.climb[o] < 0) continue;
                        const double d = std::hypot(c.spec.buildings[b].x - c.spec.buildings[o].x, c.spec.buildings[b].z - c.spec.buildings[o].z);
                        if (d < nearest) { nearest = d; ax = c.r.x + 0.5 * (c.spec.buildings[b].x + c.spec.buildings[o].x); az = c.r.z + 0.5 * (c.spec.buildings[b].z + c.spec.buildings[o].z); }
                    }
                frameOf(two, ax, az);
                for (int b = 0; b < nb0 && b < (int)c.lines.size(); b++) {
                    int line = -1;
                    madeGrowthAt(two, sv.site.unitAt(c.r.x + c.spec.buildings[b].x, c.r.z + c.spec.buildings[b].z), sv.site.gen.seed, line);
                    if (c.climb[b] < 0) continue;
                    (c.lines[b] == 0 ? byFlesh : byFungus)++;
                    matched += c.lines[b] == line;
                }
                sv.testCoverLight(w2, v2, o2, ov2, a2, al2);
            }
        }
        check("seeding: a ruin's cover at night in its veins and across its edge in its own light", placed && whole > 100 && veined == whole && across > 20 && acrossLit == across,
              fmt("(the scene's town of %d buildings placed beside it at night, fourteen steps under a glowing flesh: %d faces drawn wholly under its cover, %d lit in its "
                  "veins; %d across its edge, %d with its cover in its own light)", nb0, whole, veined, across, acrossLit));
        check("seeding: each building of a settlement in its own line's cover", placed && byFlesh > 5 && byFungus > 5 && matched == byFlesh + byFungus && o2 > 20 && ov2 == o2 && w2 > 20 && v2 == 0,
              fmt("(a fungus that does not glow over part of it, the latest line: %d covered buildings take the flesh, %d the fungus, %d of them the line whose cover lies "
                  "at their own place; %d faces of the flesh's buildings wholly under it, %d lit in its veins at night; %d of the fungus's, %d lit)", byFlesh, byFungus, matched,
                  o2, ov2, w2, v2));
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
        // the steps passing on the ground: the cover's climb after its 14th step handed on, the ground's layer the world now (B-485)
        const MadeGrowth flesh = growthOfType(OG_FLESH, OGT_FOLDED, RGB(0.62f, 0.16f, 0.18f), 0.5, 0.6, 0, false, false);
        growthLayerOn(sv, growthAtStep(R, mid, flesh, 11));
        const MadeWorld at14 = growthAtStep(R, mid, flesh, 14);
        auto climbs = [&]() { double c = 0; for (size_t b = 0; b + 1 < town->climb.size(); b++) c += std::max(0.0f, town->climb[b]); return c; };
        const double before = climbs();
        sv.setMadeNow(at14);
        { Input in; sv.update(1.0 / 30, in, t, false); }
        const double after = climbs();
        check("seeding: the ruins where the rotter lives and as the steps pass", outOfBand == 0 && inBand > 0 && before > 0 && after > before + 1 &&
                                                                                    sv.site.gen.made.key == at14.key,
              fmt("(the scene's town of %d buildings at %.1f C: a stone rotter living from %.0f to %.0f C eats %d of them, one from %.0f to %.0f C %d; a flesh over it "
                  "climbed %.0f m in all at its 11th step, %.0f m once its 14th was handed on, the ground's layer %s)", nb0, meanC, meanC + 10, meanC + 40, outOfBand,
                  meanC - 25, meanC + 25, inBand, before, after, sv.site.gen.made.key == at14.key ? "the world now" : "THE LANDING'S"));
    }
}
