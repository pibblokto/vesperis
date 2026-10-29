// E-08: on foot: `updateWalking` and what stands in its way (the settlements near a point and their buildings, a settlement's
// pieces as discs, the supports of B-420, the boulders, domes, chimneys and meteorites: one enumerator each, which the
// colliders and the drawing both read), the buggy's deploy; what stands on the ground drawn (`drawObjects`, the capsule), the
// blob shadows and the footprints
#include "surface_internal.h"

// Blob shadow (M10-11): a dark ellipse on the ground under an object, stretched away from the sun.
void SurfaceView::drawBlobShadow(Framebuffer& fb, double x, double z, double radius, double height) {
    // as strong as the light (a far sun alone casts a faint one)
    if (!env.hasSun2 || env.sun2Share < 0.05 || env.sun2Share > 0.95) { drawBlobShadowFrom(fb, x, z, radius, height, env.sun.dirLocal, env.glow); return; }
    // double shadows: one per sun, each as strong as its share of the light
    drawBlobShadowFrom(fb, x, z, radius, height, env.sunDisc.dirLocal, 1 - env.sun2Share);
    drawBlobShadowFrom(fb, x, z, radius, height, env.sun2.dirLocal, env.sun2Share);
}

void SurfaceView::drawBlobShadowFrom(Framebuffer& fb, double x, double z, double radius, double height, const Vec3& sd, double share) {
    double sunUp = smoothstep(0.0, 0.08, sd.y);
    if (sunUp < 0.05 || hasOpaqueDeck(site.gen.type)) return;
    double strength = sunUp * (site.atmosphere ? (1 - 0.7 * env.cloudCover) : 1.0) * share;
    if (strength < 0.05) return;
    double horiz = std::sqrt(sd.x * sd.x + sd.z * sd.z);
    double dx = horiz > 1e-4 ? -sd.x / horiz : 1, dz = horiz > 1e-4 ? -sd.z / horiz : 0;
    double len = std::min(height * horiz / std::max(sd.y, 0.12), radius * 4.0);
    double cx = x + dx * len * 0.5, cz = z + dz * len * 0.5;
    double dist = std::sqrt((cx - camPos.x) * (cx - camPos.x) + (cz - camPos.z) * (cz - camPos.z));
    if ((radius + len) / std::max(dist, 0.1) * proj.f < 1.5) return;
    RVert q[8];
    for (int k = 0; k < 8; k++) {
        double a = k * TAU / 8;
        double ex = std::cos(a) * radius, ey = std::sin(a) * (radius + len * 0.5);
        double wx = cx - dz * ex + dx * ey, wz = cz + dx * ex + dz * ey;
        double gy = site.groundHeight(wx, wz) + 0.12;
        Vec3 v = toView(wx, gy, wz);
        q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = 0;
    }
    RasterParams rp;
    rp.blend = BLEND_DARKEN; rp.darken = 1.0 - 0.45 * strength; rp.zwrite = false; rp.ztest = true;
    rasterPolygon(fb, q, 8, rp, proj);
}

int SurfaceView::settlementsNear(double x, double z, const RuinCell* near[SETTLEMENTS_NEAR]) const {
    int n = 0;
    auto take = [&](const Ruin& r, const RuinCell& c) {
        if (n >= SETTLEMENTS_NEAR) return;
        double dx = r.x - x, dz = r.z - z;
        if (dx * dx + dz * dz < (r.reach + 50) * (r.reach + 50)) near[n++] = &c;
    };
    for (const RuinCell& c : placedCells) take(c.r, c);   // A-02 (tests)
    if (!worldHadCivilisation(site.gen)) return n;
    forNearbyRuins(x, z, 1, [&](const Ruin& r, const RuinCell& c, int) { take(r, c); });
    return n;
}
bool SurfaceView::inSettlementBuilding(const RuinCell& c, double x, double z, double margin) {
    double dx = x - c.r.x, dz = z - c.r.z;
    if (dx * dx + dz * dz > (c.r.reach + 40) * (c.r.reach + 40)) return false;
    for (const Building& bd : c.spec.buildings) {
        double ex = dx - bd.x, ez = dz - bd.z;
        // F-02: a point beyond the circle round the rectangle is outside it at any heading (a turn keeps the length; the slack of 1e-9 is
        // a hundred times the rounding of the turn's products), so the far buildings of a town cost no sine and cosine
        const double ax = bd.hw + margin, az = bd.hd + margin;
        if (ex * ex + ez * ez > (ax * ax + az * az) * (1 + 1e-9)) continue;
        double lx = ex * std::cos(bd.heading) - ez * std::sin(bd.heading), lz = ex * std::sin(bd.heading) + ez * std::cos(bd.heading);
        if (std::fabs(lx) < ax && std::fabs(lz) < az) return true;
    }
    return false;
}

// the explorer's last 32 steps (`updateWalking` lays them): dark prints on the ground within 40 m
void SurfaceView::drawFootprints(Framebuffer& fb) {
    if (footprints.empty()) return;
    RasterParams rp;
    rp.blend = BLEND_DARKEN; rp.darken = 0.78; rp.zwrite = false; rp.ztest = true;
    for (const Footprint& f : footprints) {
        double dist = std::sqrt((f.x - camPos.x) * (f.x - camPos.x) + (f.z - camPos.z) * (f.z - camPos.z));
        if (dist > 40 || 0.3 / std::max(dist, 0.1) * proj.f < 1.0) continue;
        double hx = std::sin(f.heading), hz = std::cos(f.heading);
        double px = -hz, pz = hx;
        RVert q[4];
        double ax[4] = {-0.06, 0.06, 0.06, -0.06}, az[4] = {-0.14, -0.14, 0.14, 0.14};
        for (int k = 0; k < 4; k++) {
            double wx = f.x + px * ax[k] + hx * az[k], wz = f.z + pz * ax[k] + hz * az[k];
            Vec3 v = toView(wx, site.groundHeight(wx, wz) + 0.08, wz);
            q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = 0;
        }
        rasterPolygon(fb, q, 4, rp, proj);
    }
}

// C-01: a settlement's standing pieces as discs (A-02: each form's own, `formColliders`: rows along a wall, slices up a tower, an
// arch's piers), within 40 m of the point, each over the ground at its foot where it is drawn
void SurfaceView::settlementColliders(const Ruin& r, const RuinCell& cell, double x, double z, std::vector<Collider>& out) {
    double dcx = r.x - x, dcz = r.z - z;
    if (dcx * dcx + dcz * dcz > (r.reach + 45) * (r.reach + 45)) return;
    static thread_local std::vector<FormDisc> discs;
    for (const Form& e : cell.elems[0]) {
        double ex = r.x + e.x, ez = r.z + e.z, reach = 40 + std::max(e.hx + e.hz, formReach(e));
        if ((ex - x) * (ex - x) + (ez - z) * (ez - z) > reach * reach) continue;
        discs.clear();
        if (e.absolute) {   // a dam's wall, an aqueduct's arches: their heights are the reference level's
            Form f = e; f.y0 = 0; f.y1 = e.y1 - e.y0; f.absolute = false;
            formColliders(f, discs);
            for (const FormDisc& q : discs) out.push_back({r.x + q.x, r.z + q.z, q.r, 3, e.y0 + q.y0, e.y0 + q.y1});
            continue;
        }
        formColliders(e, discs);
        if (discs.empty()) continue;
        double gy, skirt;   // the piece's foot where it is drawn (a big one on the highest ground under it, its sides down to the lowest); none
        if (!pieceFootNear(e, ex, ez, gy, skirt)) continue;   // where it is not drawn (under water)
        for (const FormDisc& q : discs) out.push_back({r.x + q.x, r.z + q.z, q.r, 3, gy + q.y0 - (e.y0 < 0.01 && q.y0 <= e.y0 + 0.01 ? skirt : 0.0), gy + q.y1});
    }
}

// B-420: the heaps, banks, barrows and tiers within `reach` of a point (each its form's own reach farther), each with its foot as drawn and its
// mesh built once at that foot's skirt
void SurfaceView::collectSupports(double x, double z, double reach) {
    supports.clear();
    if (!worldHadCivilisation(site.gen) && placedCells.empty()) return;
    auto take = [&](const Ruin& r, const RuinCell& cell) {
        const double dcx = r.x - x, dcz = r.z - z;
        if (dcx * dcx + dcz * dcz > (r.reach + 60 + reach) * (r.reach + 60 + reach)) return;
        for (const Form& e : cell.elems[0]) {
            if (!formWalkable(e)) continue;
            const double ex = r.x + e.x, ez = r.z + e.z, rr = formReach(e) + reach;
            if ((ex - x) * (ex - x) + (ez - z) * (ez - z) > rr * rr) continue;
            Support S; S.f = &e; S.x = ex; S.z = ez; S.reach = formReach(e);
            if (!pieceFootNear(e, ex, ez, S.gy, S.skirt)) continue;
            SupportMesh& M = supportMeshes[&e];
            if (std::fabs(M.skirt - S.skirt) > 0.05) { formMesh(e, 0, Vec3(0, 0, 0), S.skirt, M.m, 1e9); M.skirt = S.skirt; }
            supports.push_back(S);
        }
    };
    forNearbyRuins(x, z, 1, [&](const Ruin& r, const RuinCell& cell, int) { take(r, cell); });
    for (const RuinCell& c : placedCells) take(c.r, c);   // (tests)
}

// B-420: the floor at a point for feet at `feet`: the ground, or the highest drawn surface of a heap, a bank, a barrow or a tier there that is
// not over a step above them (`blocked`: one is; the explorer would walk into it)
double SurfaceView::floorAt(double x, double z, double feet, bool* blocked) {
    double g = site.groundHeight(x, z);
    if (blocked) *blocked = false;
    for (const Support& S : supports) {
        const double dx = x - S.x, dz = z - S.z;
        if (dx * dx + dz * dz > S.reach * S.reach) continue;
        auto it = supportMeshes.find(S.f);
        if (it == supportMeshes.end()) continue;
        const double top = formSupportAt(it->second.m, dx, dz);
        if (top < -1e17) continue;
        const double y = S.gy + top;
        if (y > feet + STEP_M) { if (blocked) *blocked = true; continue; }
        g = std::max(g, y);
    }
    return g;
}

// B-420: one enumerator of a 16 m cell's boulders, which the drawing and the colliders both read (as the trees' `forTrees`): none in a room,
// on a road's or a street's bed, on a work's ground or under the water; `keep` a cheap test first (the drawing's size over its distance: the
// costlier ones after it). The colliders mirrored the drawing's formulas and missed its last three tests: a boulder left off a street or a
// quarry's floor still stood there, unseen. Returns the cell's candidates (the bench's count)
namespace {
void rockParams(int type, double& density, double& scale) { density = KINDS[type].rockDensity; scale = KINDS[type].rockScale; }   // the kind's boulders
}   // namespace
int SurfaceView::forRocks(int cx, int cz, const std::function<bool(double, double, double)>& keep, const std::function<void(const RockInst&)>& fn) {
    const double cs = 16;
    const int type = site.sys->bodies[site.body].type;
    double rockDensity, rockScale; rockParams(type, rockDensity, rockScale);
    const uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xB0B);
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    double rd = rockDensity;
    if (livingGround(type) && (tv.material == MAT_GRASS || tv.material == MAT_ROCK) && tv.biome != BIO_WETLAND) rd = 0.7;   // R-306: open ground carries more stone
    rd *= 1 + 5.0 * tv.scree / 255.0;   // boulder fields on the scree under the cliffs
    int nrocks = (int)(rd * 2.0 * hash01(h));
    if (livingGround(type) && hash01(mix64(h + 5)) < 0.06 && tv.material != MAT_FOREST) nrocks += 3;   // an outcrop of three
    if (tv.material == MAT_SAND || tv.material == MAT_WATER || tv.material == MAT_LAVA || tv.material == MAT_MATS || tv.material == MAT_STROMATOLITE) nrocks = 0;   // nor on the mats' sediment
    // nor in a spring or on its sinter; Q-02: nor on the gypsum's dunes, a hot spring's terraces
    else if ((tv.material == MAT_BRINE || tv.material == MAT_SALT) && (kindHas(type, KT_VENT_LIFE) || site.gen.hasTrait(TR_GYPSUM) || site.gen.hasTrait(TR_HOT_SPRINGS))) nrocks = 0;
    if (tv.material == MAT_GLASS) nrocks = (int)(nrocks * 0.3);   // a few blast-thrown boulders on the sheets
    if (nrocks <= 0) return 0;
    const RuinCell* near[SETTLEMENTS_NEAR]; int nNear = -1;   // no rock in a room (the settlements near, found once a cell when a rock asks)
    for (int i = 0; i < nrocks; i++) {
        const uint64_t hr = mix64(h + 17 * (i + 1));
        RockInst R;
        R.x = cx * cs + hash01(hr) * cs; R.z = cz * cs + hash01(mix64(hr + 1)) * cs; R.h = hr;
        R.size = rockScale * (0.35 + 2.0 * std::pow(hash01(mix64(hr + 2)), 2.5)) * (1 + 0.6 * tv.scree / 255.0);   // bigger on the scree
        if (keep && !keep(R.x, R.z, R.size)) continue;
        if (nNear < 0) nNear = settlementsNear(cx * cs + 8, cz * cs + 8, near);
        { bool in = false; for (int q = 0; q < nNear && !in; q++) in = inSettlementBuilding(*near[q], R.x, R.z, 1.0); if (in) continue; }
        if (!site.roads.empty() && site.roadCover(R.x, R.z) > 0.3) continue;   // no rock on the road's bed
        if (!site.marks.empty() && site.markCover(R.x, R.z) > 0.3) continue;   // nor on a street
        if (!site.edits.empty() && site.editCover(R.x, R.z) > 0.3) continue;   // nor on a work's ground
        R.gy = site.groundHeight(R.x, R.z);
        if (R.gy < site.waterAt(R.x, R.z)) continue;
        fn(R);
    }
    return nrocks;
}

// P-02: the stromatolites of a 16 m cell: on its reef flats (the cell's ground MAT_STROMATOLITE) seven to fifteen domes 0.7-3.2 m across, on its mats one in
// three cells one; none in a room, on a road or a street, or with its foot more than 40 cm under the water (a dome stands from the shallows)
void SurfaceView::forDomes(int cx, int cz, const std::function<void(const RockInst&)>& fn) {
    if (!kindHas(site.gen.type, KT_MICROBIAL)) return;
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    if (tv.material != MAT_STROMATOLITE && tv.material != MAT_MATS) return;
    const double cs = 16;
    const uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0x57A0ULL);
    const int n = tv.material == MAT_STROMATOLITE ? 7 + (int)(hash01(h) * 9) : (hash01(h) < 0.33 ? 1 : 0);
    const RuinCell* near[SETTLEMENTS_NEAR]; int nNear = -1;
    for (int i = 0; i < n; i++) {
        const uint64_t hr = mix64(h + 31 * (i + 1));
        RockInst R;
        R.x = cx * cs + hash01(hr) * cs; R.z = cz * cs + hash01(mix64(hr + 1)) * cs; R.h = hr;
        R.size = 0.35 + 1.25 * std::pow(hash01(mix64(hr + 2)), 1.6);   // the radius
        if (nNear < 0) nNear = settlementsNear(cx * cs + 8, cz * cs + 8, near);
        { bool in = false; for (int q = 0; q < nNear && !in; q++) in = inSettlementBuilding(*near[q], R.x, R.z, 1.0); if (in) continue; }
        if (!site.roads.empty() && site.roadCover(R.x, R.z) > 0.3) continue;
        if (!site.marks.empty() && site.markCover(R.x, R.z) > 0.3) continue;
        R.gy = site.groundHeight(R.x, R.z);
        if (R.gy < site.waterAt(R.x, R.z) - 0.4) continue;
        fn(R);
    }
}

// P-03: the chemotroph's sinter chimneys: on the sinter round its springs, none to three a 16 m cell, slim cones 1-4.5 m tall (the pipes the
// water builds of what it leaves), their foot 0.18-0.5 m across; the caller asks on a vent-life world alone
inline double chimneyHeight(uint64_t h) { return 1.0 + 3.5 * std::pow(unitFromHash(mix64(h + 4)), 1.4); }
void SurfaceView::forChimneys(int cx, int cz, const std::function<void(const RockInst&)>& fn) {
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    if (tv.material != MAT_SALT) return;
    const double cs = 16;
    const uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xC417ULL);
    const int n = (int)(hash01(h) * 4);
    for (int i = 0; i < n; i++) {
        const uint64_t hr = mix64(h + 37 * (i + 1));
        RockInst R;
        R.x = cx * cs + hash01(hr) * cs; R.z = cz * cs + hash01(mix64(hr + 1)) * cs; R.h = hr;
        R.size = 0.18 + 0.32 * hash01(mix64(hr + 2));   // the foot's radius
        R.gy = site.groundHeight(R.x, R.z);
        if (R.gy < site.waterAt(R.x, R.z) - 0.2) continue;
        fn(R);
    }
}

// Q-02: the iron meteorites of a strewn field's 16 m cell: none to four by the field's density at the cell (the ground's `gtint`), most a hand to a
// forearm across, one in twenty-five a mass of a metre or two; none in a room, on a road or a street, under the water
void SurfaceView::forMeteorites(int cx, int cz, const std::function<void(const RockInst&)>& fn) {
    if (!site.gen.hasTrait(TR_METEORITES)) return;
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    if (tv.gtint < 10) return;
    const double cs = 16;
    const uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0x3E7EULL);
    const double nx = tv.gtint / 255.0 * 2.6 * (0.5 + hash01(h));
    const int n = (int)nx + (hash01(mix64(h + 1)) < nx - (int)nx ? 1 : 0);
    const RuinCell* near[SETTLEMENTS_NEAR]; int nNear = -1;
    for (int i = 0; i < n; i++) {
        const uint64_t hr = mix64(h + 43 * (i + 1));
        RockInst R;
        R.x = cx * cs + hash01(hr) * cs; R.z = cz * cs + hash01(mix64(hr + 1)) * cs; R.h = hr;
        const double u = hash01(mix64(hr + 2));
        R.size = u > 0.96 ? 0.6 + 0.9 * hash01(mix64(hr + 3)) : 0.12 + 0.4 * u * u;   // the radius
        if (nNear < 0) nNear = settlementsNear(cx * cs + 8, cz * cs + 8, near);
        { bool in = false; for (int q = 0; q < nNear && !in; q++) in = inSettlementBuilding(*near[q], R.x, R.z, 1.0); if (in) continue; }
        if (!site.roads.empty() && site.roadCover(R.x, R.z) > 0.3) continue;
        if (!site.marks.empty() && site.markCover(R.x, R.z) > 0.3) continue;
        R.gy = site.groundHeight(R.x, R.z);
        if (R.gy < site.waterAt(R.x, R.z)) continue;
        fn(R);
    }
}

// Rocks larger than 0.6 m and tree trunks in the 3x3 cells around (x, z), as cylinders (M8-05). B-420: from the enumerators that draw them
void SurfaceView::collectColliders(double x, double z, std::vector<Collider>& out) {
    out.clear();
    const bool microbial = kindHas(site.gen.type, KT_MICROBIAL), ventLife = kindHas(site.gen.type, KT_VENT_LIFE);
    const bool meteorites = site.gen.hasTrait(TR_METEORITES);
    double cs = 16;
    int pcx = (int)std::floor(x / cs), pcz = (int)std::floor(z / cs);
    // a settlement's standing pieces as rows of discs along their walls (within 40 m)
    forNearbyRuins(x, z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        settlementColliders(r, cell, x, z, out);
    });
    for (const RuinCell& c : placedCells) settlementColliders(c.r, c, x, z, out);   // (tests)
    for (int cz = pcz - 1; cz <= pcz + 1; cz++)
        for (int cx = pcx - 1; cx <= pcx + 1; cx++) {
            forRocks(cx, cz, [](double, double, double size) { return size >= 0.6; },   // a smaller one is stepped over
                     [&](const RockInst& R) { out.push_back({R.x, R.z, R.size * 0.8, 0, R.gy - R.size * 0.15, R.gy + R.size * 1.15}); });   // the peak stands up to 1.3 sizes over a base sunk 0.15
            // trunks and fallen logs from the same enumerators that draw them
            forTrees(cx, cz, [&](const TreeInst& T) { out.push_back({T.x, T.z, 0.12 + (T.stump ? T.r * 0.14 : T.h * 0.02), 1, T.gy - 0.3, T.gy + T.h}); });   // a stone stump's girth
            if (microbial) forDomes(cx, cz, [&](const RockInst& R) { if (R.size >= 0.6) out.push_back({R.x, R.z, R.size * 0.85, 0, R.gy - 0.2, R.gy + 0.55 * R.size + 0.2}); });
            if (ventLife) forChimneys(cx, cz, [&](const RockInst& R) { out.push_back({R.x, R.z, R.size, 0, R.gy - 0.1, R.gy + chimneyHeight(R.h)}); });
            if (meteorites) forMeteorites(cx, cz, [&](const RockInst& R) { if (R.size >= 0.6) out.push_back({R.x, R.z, R.size * 0.9, 0, R.gy - 0.1, R.gy + 0.8 * R.size}); });
            forLogs(cx, cz, [&](const LogInst& L) {
                double dx = std::sin(L.heading), dz = std::cos(L.heading), lg = site.groundHeight(L.x, L.z);
                for (int k = -1; k <= 1; k++) out.push_back({L.x + dx * L.len * 0.33 * k, L.z + dz * L.len * 0.33 * k, L.radius + 0.35, 2, lg - 1.0, lg + 2 * L.radius + 0.6});
            });
        }
}

// B within 10 m of the capsule: a new buggy unfolds beside it; a buggy already out, wherever it was left, is scrapped
// (R-204: you can always redeploy)
bool SurfaceView::deployBuggy() {
    if (inVehicle()) return false;
    if (site.escapeVelocity < 30) return false;   // on a comet the buggy would float off at the first bump
    double dx = capsuleX - player.x, dz = capsuleZ - player.z;
    if (dx * dx + dz * dz > 10.0 * 10.0) return false;
    // beside the capsule, away from the explorer
    double ax = player.x - capsuleX, az = player.z - capsuleZ;
    double al = std::sqrt(ax * ax + az * az) + 1e-9;
    buggy = Buggy();
    buggy.deployed = true;
    buggy.x = capsuleX - az / al * 4.0; buggy.z = capsuleZ + ax / al * 4.0;
    buggy.heading = std::atan2(-az, -ax);
    buggy.y = site.surfaceHeight(buggy.x, buggy.z);
    buggy.groundY = -1e9; buggy.vy = 0; buggy.airborne = false;
    buggy.unfold = 0;
    return true;
}

bool SurfaceView::toggleBuggy() {
    if (inBuggy) {
        inBuggy = false;
        chaseCam = false;
        player.x = buggy.x - std::cos(buggy.heading) * 1.4; player.z = buggy.z + std::sin(buggy.heading) * 1.4;
        player.y = site.surfaceHeight(player.x, player.z);
        player.vx = player.vz = player.vy = 0;
        player.yaw = buggy.heading;
        return true;
    }
    if (inDrone || !buggy.deployed || buggy.unfold < 1 || buggyDist() > 3.5) return false;
    inBuggy = true;
    player.autoWalk = 0; player.sprinting = false; player.crouch = false; player.hindLegs = false;
    player.yaw = buggy.heading; player.pitch = 0;
    return true;
}

void SurfaceView::updateWalking(double dt, const Input& in, double t, bool controlsEnabled) {
    Player& p = player;
    double g = site.gravity;
    double gRel = g / 9.8;
    p.lastVy = p.vy;
    double eyeTarget = p.swimming ? 1.65 : (p.crouch ? 0.9 : (p.hindLegs ? 2.1 : 1.65));   // B-322: afloat the body floats: no crouch (it put the eye 25 cm under the surface)
    p.eyeHeight += (eyeTarget - p.eyeHeight) * (1 - std::exp(-dt * 8));
    double fwd = 0, str = 0;
    bool wantSprint = false;
    if (controlsEnabled) {
        p.yaw += in.mouseDx * 0.0032 * mouseSens;
        p.pitch -= in.mouseDy * 0.0032 * mouseSens * (invertY ? -1 : 1);
        if (in.isDown(KEY_LEFT)) p.yaw -= 1.6 * dt;
        if (in.isDown(KEY_RIGHT)) p.yaw += 1.6 * dt;
        if (in.isDown(KEY_PAGE_UP)) p.pitch += 1.0 * dt;
        if (in.isDown(KEY_PAGE_DOWN)) p.pitch -= 1.0 * dt;
        p.pitch = clampd(p.pitch, -85 * DEG, 85 * DEG);
        p.yaw = wrap2pi(p.yaw);
        // postures (M8-11): C crouches, Z held stands on the hind legs
        if (in.wasPressed(KEY_C) && !in.ctrl()) p.crouch = !p.crouch;
        p.hindLegs = in.isDown(KEY_Z) && p.onGround && !p.crouch;
        // autowalk presets (M8-04): 1-9 hold a forward speed, 0 or any backward input stops
        for (int k = 0; k <= 9; k++) if (in.wasPressed(KEY_0 + k) && !in.ctrl()) p.autoWalk = k;
        fwd = (in.isDown(KEY_W) || in.isDown(KEY_UP) ? 1 : 0) - (in.isDown(KEY_S) || in.isDown(KEY_DOWN) ? 1 : 0);
        str = (in.isDown(KEY_D) ? 1 : 0) - (in.isDown(KEY_A) ? 1 : 0);
        if (in.moveY != 0) fwd = in.moveY;   // an analogue stick; R-418: each axis on its own
        if (in.moveX != 0) str = in.moveX;
        if (in.ctrl()) { fwd = 0; str = 0; }   // Ctrl+S saves, Ctrl+L loads: not a walk
        if (fwd < 0) p.autoWalk = 0;
        if (p.autoWalk > 0 && fwd == 0) fwd = std::min(2.25, p.autoWalk / 4.0);
        // sprint (M8-02): hold or toggle, with stamina
        if (sprintToggleMode) { if (in.wasPressed(KEY_LEFT_SHIFT) || in.wasPressed(KEY_RIGHT_SHIFT)) p.sprinting = !p.sprinting; wantSprint = p.sprinting; }
        else wantSprint = in.shift();
        if (p.hindLegs) { fwd = 0; str = 0; }
    } else {
        p.autoWalk = 0;
    }
    bool moving = std::fabs(fwd) > 0.01 || std::fabs(str) > 0.01;
    if (p.winded && p.stamina > 25) p.winded = false;
    if (p.stamina < 1) p.winded = true;
    bool sprintNow = wantSprint && moving && fwd > 0 && !p.winded && !p.crouch && !p.swimming;
    p.sprinting = sprintNow;
    p.sprintRamp += ((sprintNow ? 1.0 : 0.0) - p.sprintRamp) * (1 - std::exp(-dt / 0.6));
    if (sprintNow) p.stamina -= dt * 12.0 * std::max(0.6, gRel);
    else p.stamina += dt * (moving ? 8.0 : 16.0);
    p.stamina = clampd(p.stamina, 0, 100);
    if (p.swimming) { p.swimStamina = clampd(p.swimStamina - dt * 2.0, 0, 100); } else p.swimStamina = clampd(p.swimStamina + dt * 10, 0, 100);
    double sprintMul = clampd(sprintMultiplier * std::pow(1.0 / std::max(gRel, 0.2), 0.44), 1.5, 4.0);
    double speed = 4.2 * (1.0 + (sprintMul - 1.0) * p.sprintRamp) * p.speedMul;
    if (gRel > 1.5) speed *= 0.85;           // heavy worlds
    if (site.escapeVelocity < 30) speed = std::min(speed, 0.3 * site.escapeVelocity);   // microgravity: any faster and you would leave the ground
    if (p.crouch) speed *= 0.5;
    if (p.swimming) speed *= 0.45 * (p.swimStamina > 0.5 ? 1.0 : 0.5);
    p.fovKick = 4.0 * p.sprintRamp;
    double sy = std::sin(p.yaw), cy = std::cos(p.yaw);
    double tx = (sy * fwd + cy * str), tz = (cy * fwd - sy * str);
    double tl = std::sqrt(tx * tx + tz * tz);
    if (tl > 1 && p.autoWalk == 0) { tx /= tl; tz /= tl; }
    // what the explorer may stand on beside the ground: the heaps, banks, barrows and tiers near it (inside one, as after a landing on it,
    // it stands on its top)
    collectSupports(p.x, p.z);
    auto footing = [&](double x, double z, bool* blocked = nullptr) { bool in = false; double f = floorAt(x, z, p.y, &in); if (in && blocked) *blocked = true; return f; };
    bool insideHere = false; floorAt(p.x, p.z, p.y, &insideHere);
    // wading (M8-08): slower with depth until swimming takes over
    double groundHere = insideHere ? floorAt(p.x, p.z, 1e9) : footing(p.x, p.z);
    double waterHere = site.waterNowAt(p.x, p.z);   // the tide's
    double depthHere = waterHere > -1e8 ? clampd(waterHere - groundHere, 0, 1.2) : 0;
    if (!p.swimming && p.onGround && depthHere > 0) speed *= 1.0 - 0.55 * depthHere / 1.2;   // wading, not flying over
    double accel = p.onGround ? (gRel > 1.5 ? 6.0 : 10.0) : (g < 3.0 ? 4.0 : 2.0);   // air control
    if (p.jetOn) accel = 6.0;
    double k = 1 - std::exp(-dt * accel);
    p.vx += (tx * speed - p.vx) * k;
    p.vz += (tz * speed - p.vz) * k;
    // jump (M8-09): apex clamped between 0.6 and 6 m whatever the gravity
    if (controlsEnabled && in.wasPressed(KEY_SPACE) && p.onGround && !p.swimming && !p.crouch) {
        double hJump = clampd(1.1 * std::sqrt(9.8 / std::max(g, 0.5)), 0.6, 6.0);
        p.vy = std::sqrt(2 * g * hJump);
        p.onGround = false;
    }
    // jetpack (M8-03): hold Space in the air; heat limits continuous burns. B-406: it fires from the water's surface too (afloat,
    // not diving), so a lake is crossed and left by air, never a trap
    p.jetOn = false;
    bool afloat = p.swimming && !p.diving && !p.underwater;
    if (controlsEnabled && in.isDown(KEY_SPACE) && (!p.onGround || afloat) && (!p.swimming || afloat) && p.jetHeat < 100 && p.altAboveGround < 300) {
        p.jetOn = true;
        p.vy += (g + 3.0) * dt;
        double vcap = site.escapeVelocity < 30 ? 0.35 * site.escapeVelocity : 12.0;   // never toward escape velocity
        if (p.vy > vcap) p.vy = vcap;
        p.jetHeat = std::min(100.0, p.jetHeat + dt * 5.0);
    } else p.jetHeat = std::max(0.0, p.jetHeat - dt * 8.0);
    // on light worlds Ctrl in the air fires the thrusters downward (a jump on a comet floats for minutes otherwise)
    if (controlsEnabled && in.ctrl() && !p.onGround && !p.swimming && g < 1.0) p.vy -= 3.0 * dt;
    // diving (M8-08): Ctrl while swimming goes under, buoyancy brings you back
    p.diving = p.swimming && controlsEnabled && in.ctrl();
    p.vy -= g * dt;
    double terminal = site.atmosphere ? 55.0 : 80.0;
    if (p.vy < -terminal) p.vy = -terminal;
    double nx = p.x + p.vx * dt, nz = p.z + p.vz * dt;
    // slopes (M8-01): the ground gradient decides climbing and sliding (B-420: the ground, or the heap, bank or tier under the feet)
    double gx = (footing(p.x + 0.5, p.z) - footing(p.x - 0.5, p.z)),
           gz = (footing(p.x, p.z + 0.5) - footing(p.x, p.z - 0.5));
    double slope = std::sqrt(gx * gx + gz * gz);   // rise per run
    bool steep = slope > 1.35 && p.onGround && !p.swimming;
    double hNew = footing(nx, nz);
    double moveLen = std::sqrt((nx - p.x) * (nx - p.x) + (nz - p.z) * (nz - p.z));
    if (p.onGround && moveLen > 1e-6 && (hNew - groundHere) / std::max(moveLen, 0.05) > 1.35) {
        // too steep to climb: keep only the along-slope part of the motion
        double sl = std::sqrt(gx * gx + gz * gz) + 1e-9;
        double ux = gx / sl, uz = gz / sl;
        double along = p.vx * ux + p.vz * uz;
        if (along > 0) { p.vx -= ux * along; p.vz -= uz * along; }
        nx = p.x + p.vx * dt; nz = p.z + p.vz * dt;
    }
    if (steep) {
        // slide downhill with g sin(slope)
        double sl = slope + 1e-9, ang = std::atan(slope);
        p.vx -= gx / sl * g * std::sin(ang) * 0.8 * dt;
        p.vz -= gz / sl * g * std::sin(ang) * 0.8 * dt;
        nx = p.x + p.vx * dt; nz = p.z + p.vz * dt;
    }
    // a heap's, a bank's or a tier's side more than a step high, or steeper than the ground's climb, is walked into, not up: the move
    // along it where one axis is free
    if (!insideHere && !p.swimming && p.onGround) {
        auto barred = [&](double x, double z) {
            bool bl = false; const double f = footing(x, z, &bl);
            if (bl) return true;
            const double run = std::hypot(x - p.x, z - p.z);
            return f > site.groundHeight(x, z) + 0.01 && run > 1e-6 && (f - groundHere) / std::max(run, 0.05) > 1.35;   // a heap's steep side
        };
        if (barred(nx, nz)) {
            const bool bx = barred(nx, p.z), bz = barred(p.x, nz);
            if (!bx) { nz = p.z; p.vz = 0; } else if (!bz) { nx = p.x; p.vx = 0; } else { nx = p.x; nz = p.z; p.vx = p.vz = 0; }
        }
    } else if (!insideHere && !p.swimming) {   // in the air: into a side more than a step over the feet
        bool bl = false; footing(nx, nz, &bl);
        if (bl) {
            bool bx = false, bz = false; footing(nx, p.z, &bx); footing(p.x, nz, &bz);
            if (!bx) { nz = p.z; p.vz = 0; } else if (!bz) { nx = p.x; p.vx = 0; } else { nx = p.x; nz = p.z; p.vx = p.vz = 0; }
        }
    }
    // collision with rocks and trunks
    {
        static std::vector<Collider> cols;
        collectColliders(nx, nz, cols);
        for (const Collider& c : cols) {
            if (p.y > c.y1 - 0.1 || p.y + 1.6 < c.y0) continue;   // risen above it (the jetpack, a slope) or under it
            double dx = nx - c.x, dz = nz - c.z;
            double d2 = dx * dx + dz * dz, rr = c.r + 0.35;
            if (d2 < rr * rr && d2 > 1e-9) {
                double d = std::sqrt(d2);
                nx = c.x + dx / d * rr; nz = c.z + dz / d * rr;
                double vn = p.vx * dx / d + p.vz * dz / d;
                if (vn < 0) { p.vx -= dx / d * vn; p.vz -= dz / d * vn; }
            }
        }
    }
    p.x = nx; p.z = nz;
    // footprints (M10-05 decals) on soft ground, one every 0.7 m walked
    {
        double dxs = p.x - lastStepX, dzs = p.z - lastStepZ;
        stepAccum += std::sqrt(dxs * dxs + dzs * dzs);
        lastStepX = p.x; lastStepZ = p.z;
        if (stepAccum > 0.7 && p.onGround) {
            stepAccum = 0;
            int mat = site.lod0.at((int)std::floor(p.x / 16), (int)std::floor(p.z / 16)).material;
            if (mat == MAT_SAND || mat == MAT_SNOW || mat == MAT_DUST || mat == MAT_GRASS) {
                double heading = std::atan2(p.vx, p.vz);
                double side = footLeft ? -0.16 : 0.16;
                footLeft = !footLeft;
                Footprint f{(float)(p.x + std::cos(heading) * side), (float)(p.z - std::sin(heading) * side), (float)heading};
                if (footprints.size() < 32) footprints.push_back(f); else { footprints[footHead] = f; footHead = (footHead + 1) % 32; }
            }
        }
    }
    bool insideNow = false; floorAt(p.x, p.z, p.y, &insideNow);
    double ground = insideNow ? floorAt(p.x, p.z, 1e9) : floorAt(p.x, p.z, p.y);   // the footing (a heap, a bank, a tier), the ground else
    double water = site.waterNowAt(p.x, p.z);
    // deep water is swum once the body is down at its surface; above it the air is the air, so the jetpack
    // flies over a lake (the old rule made the explorer swim the moment the ground under them was 0.4 m under water, whatever
    // their height: a jet over the shore was pulled down to the float level)
    bool deep = water > -1e8 && ground < water - 0.4;
    double floatY = water - 1.15;
    double floor = deep ? (p.diving ? std::max(ground, water - 3.5) : floatY) : ground;
    p.y += p.vy * dt;
    bool wasSwimming = p.swimming;
    p.swimming = deep && (p.y <= floatY + 0.3 || (wasSwimming && !p.jetOn && p.y <= floatY + 0.6));
    bool wasOnGround = p.onGround;
    if (p.y <= floor) {
        if (!wasOnGround && !p.swimming && p.vy < -3.0) p.landDip = std::min(0.4, -p.vy * 0.045 * (gRel > 1.5 ? 1.4 : 1.0));   // landing dip
        p.y = floor;
        if (p.vy < 0) p.vy = 0;
        p.onGround = true;
    } else {
        p.onGround = p.y - floor < 0.02;
    }
    if (p.swimming) {
        p.onGround = true;
        if (p.diving) { p.vy = std::max(p.vy, -1.5); p.y = std::max(p.y - 1.5 * dt, std::max(ground + 0.3, water - 3.5)); }
        else if (!p.jetOn) { p.vy = std::max(p.vy, -1.0); p.y += (floor - p.y) * (1 - std::exp(-dt * 4)); }   // the jet lifts out of the water
    }
    p.underwater = p.swimming && water > -1e8 && (p.y + p.eyeHeight < water - 0.05);
    p.altAboveGround = std::max(0.0, p.y - ground);
    // head bob and sway
    double hs = std::sqrt(p.vx * p.vx + p.vz * p.vz);
    if (p.onGround && hs > 0.3 && !p.swimming) {
        p.stridePhase += hs * dt / (p.sprinting ? 2.4 : 1.6);
        double amp = 0.035 + 0.03 * p.sprintRamp;
        p.bobY += (amp * std::sin(p.stridePhase * TAU) - p.bobY) * (1 - std::exp(-dt * 12));
        p.bobX += (0.02 * std::sin(p.stridePhase * PI) - p.bobX) * (1 - std::exp(-dt * 12));
    } else {
        p.bobY *= std::exp(-dt * 6); p.bobX *= std::exp(-dt * 6);
    }
    p.landDip *= std::exp(-dt * 5);
    (void)t;
}

void SurfaceView::drawObjects(Framebuffer& fb, double t) {
    const Body& b = site.sys->bodies[site.body];
    int type = b.type;
    double cs = 16;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double lf = env.sun.lightFactor;
    double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    int radius = 22;
    lastRocksDrawn = 0; lastRocksCandidates = 0;
    // P-02: the stromatolites' pass on a microbial world alone (its std::function a cell otherwise); P-03 the chimneys' on a vent-life
    // world
    const bool flora = kindHas(type, KT_FLORA), microbial = kindHas(type, KT_MICROBIAL), ventLife = kindHas(type, KT_VENT_LIFE);
    const bool meteorites = site.gen.hasTrait(TR_METEORITES);
    RasterParams rp; rp.bank = (flora || KINDS[type].accent[1] >= 0) ? 0 : 3;   // boulders in the rock colour (bank 3 is the forest on living worlds, P-00: an accent's where a kind lends it)
    rp.grain = &grain; rp.grainScale = 3.0 * FB_SCALE;
    rp.grain2 = mesoFor(flora ? MAT_FOREST : MAT_ROCK); rp.grain2Scale = 4.0;   // facets and lichen speckle
    RasterParams rpPlain; rpPlain.bank = rp.bank;
    RasterParams rp2; rp2.bank = 2;
    double fogD = env.fogDistance;
    auto fogShade = [&](double shade, double dist) { double f = 1 - std::exp(-dist / fogD); return shade + (63 - shade) * f; };
    for (int cz = pcz - radius; cz <= pcz + radius; cz++)
        for (int cx = pcx - radius; cx <= pcx + radius; cx++) {
            double ddx = (cx + 0.5) * cs - camPos.x, ddz = (cz + 0.5) * cs - camPos.z;
            double cellDist = std::sqrt(ddx * ddx + ddz * ddz);
            if (cellDist > radius * cs) continue;
            if (cellDist > cs * 2 && (ddx * camLocal.m[2][0] + ddz * camLocal.m[2][2]) / cellDist < -0.2) continue;
            uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xB0B);
            // rocks (B-420: the colliders' enumerator)
            lastRocksCandidates += forRocks(
                    cx, cz,
                    [&](double rx, double rz, double size) {
                        const double dist = std::sqrt((rx - camPos.x) * (rx - camPos.x) + (rz - camPos.z) * (rz - camPos.z));
                        return size / dist * proj.f >= 0.8;
                    },
                    [&](const RockInst& R) {
                        const double rx = R.x, rz = R.z, size = R.size, gy = R.gy;
                        const uint64_t hr = R.h;
                        double dist = std::sqrt((rx - camPos.x) * (rx - camPos.x) + (rz - camPos.z) * (rz - camPos.z));
                        double a0 = hash01(mix64(hr + 3)) * TAU;
                        Vec3 base(rx, gy - size * 0.15, rz);
                        Vec3 pts[3];
                        for (int k = 0; k < 3; k++) pts[k] = base + Vec3(std::cos(a0 + k * 2.094) * size, 0, std::sin(a0 + k * 2.094) * size);
                        double peakH = size * (0.7 + 0.6 * hash01(mix64(hr + 4)));
                        Vec3 peak = base + Vec3(std::cos(a0 + 1.0) * size * 0.25, peakH, std::sin(a0 + 1.0) * size * 0.25);
                        lastRocksDrawn++;
                        if (size > 0.5) drawBlobShadow(fb, rx, rz, size * 0.9, peakH);
                        rp.grainWeight = clampd(1.0 - (dist - 60.0) / 200.0, 0.2, 1.0);
                        for (int k = 0; k < 3; k++) {
                            Vec3 p0 = pts[k], p1 = pts[(k + 1) % 3];
                            Vec3 n = normalize(cross(p1 - p0, peak - p0));
                            if (n.y < 0) n = -n;
                            double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
                            double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf;
                            if (nLamps) light += (1 - amb) * lampAt((p0 + p1 + peak) * (1.0 / 3.0), &n);
                            double shade = 50 * std::pow(light, 0.6);
                            shade = fogShade(shade, dist);
                            RVert tri[3];
                            Vec3 vs[3] = {toView(p0.x, p0.y, p0.z), toView(p1.x, p1.y, p1.z), toView(peak.x, peak.y, peak.z)};
                            Vec3 ws[3] = {p0, p1, peak};
                            for (int q = 0; q < 3; q++) {
                                tri[q].x = vs[q].x;
                                tri[q].y = vs[q].y;
                                tri[q].z = vs[q].z;
                                tri[q].shade = shade;
                                tri[q].u = ws[q].x + ws[q].y * 0.7;
                                tri[q].v = ws[q].z + ws[q].y * 0.4;   // world-mapped facets
                            }
                            rasterTriangle(fb, tri, rp, proj);
                        }
                    });
            // a strewn field's iron meteorites: dark lumps pitted with thumbprints, their fusion crust black-brown in the chrome's ramp,
            // a facet that faces the sun from the eye's side glinting
            if (meteorites) forMeteorites(cx, cz, [&](const RockInst& R) {
                const double dist = std::sqrt((R.x - camPos.x) * (R.x - camPos.x) + (R.z - camPos.z) * (R.z - camPos.z));
                if (R.size / dist * proj.f < 0.6) return;
                const double hgt = R.size * (0.55 + 0.35 * hash01(mix64(R.h + 4))), a0 = hash01(mix64(R.h + 3)) * TAU;
                const int NA = dist < 30 ? 6 : 4;
                if (R.size > 0.4) drawBlobShadow(fb, R.x, R.z, R.size * 0.9, hgt);
                Vec3 ring[6];
                for (int a = 0; a < NA; a++) {
                    const double ang = a0 + a * TAU / NA, wob = 0.75 + 0.5 * hash01(mix64(R.h + 10 + a));
                    ring[a] = Vec3(R.x + std::cos(ang) * R.size * wob, R.gy - 0.05 + 0.3 * hgt * hash01(mix64(R.h + 20 + a)), R.z + std::sin(ang) * R.size * wob);
                }
                const Vec3 top(R.x + 0.2 * R.size * std::cos(a0), R.gy + hgt, R.z + 0.2 * R.size * std::sin(a0));
                const Vec3 toCam = normalize(Vec3(camPos.x - R.x, camPos.y - R.gy, camPos.z - R.z));
                for (int a = 0; a < NA; a++) {
                    const Vec3& p0 = ring[a]; const Vec3& p1 = ring[(a + 1) % NA];
                    Vec3 n = normalize(cross(p1 - p0, top - p0)); if (n.y < 0) n = -n;
                    const double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
                    const double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt((p0 + p1 + top) * (1.0 / 3.0), &n) : 0.0);   // the lamp
                    const double glint = std::pow(std::max(0.0, dot(normalize(toCam + sd), n)), 40.0) * sunUp;
                    const double shade = fogShade(5 + 9 * light + (glint > 0.4 ? 30 : 0), dist);   // black-brown, the chrome's bright stops a glint's alone
                    RVert tri[3]; const Vec3 vs[3] = {toView(p0.x, p0.y, p0.z), toView(p1.x, p1.y, p1.z), toView(top.x, top.y, top.z)};
                    bool ok = true;
                    for (int q = 0; q < 3; q++) { tri[q].x = vs[q].x; tri[q].y = vs[q].y; tri[q].z = vs[q].z; tri[q].shade = shade; if (vs[q].z < NEAR_Z) ok = false; }
                    if (ok) { RasterParams rm; rm.bank = 7; rm.grain = &grain; rm.grainScale = 4.0 * FB_SCALE; rasterTriangle(fb, tri, rm, proj); }
                }
            });
            // quartz crystals
            if (kindHas(type, KT_CLUSTERS) && hash01(mix64(h + 99)) < 0.5) {
                int nc = 1 + (int)(hash01(mix64(h + 98)) * 2);
                for (int i = 0; i < nc; i++) {
                    uint64_t hc = mix64(h + 300 + i);
                    double rx = cx * cs + hash01(hc) * cs, rz = cz * cs + hash01(mix64(hc + 1)) * cs;
                    double dist = std::sqrt((rx - camPos.x) * (rx - camPos.x) + (rz - camPos.z) * (rz - camPos.z));
                    double hgt = 1.0 + 3.0 * hash01(mix64(hc + 2));
                    if (hgt / dist * proj.f < 1.0) continue;
                    double gy = site.groundHeight(rx, rz);
                    double a0 = hash01(mix64(hc + 3)) * TAU;
                    double w = 0.25 + 0.3 * hash01(mix64(hc + 4));
                    Vec3 p0(rx + std::cos(a0) * w, gy, rz + std::sin(a0) * w), p1(rx - std::cos(a0) * w, gy, rz - std::sin(a0) * w);
                    Vec3 pk(rx + 0.2 * w, gy + hgt, rz);
                    // the lamp lights its facets
                    double shade = fogShade(46 + 14 * std::max(sunUp, std::min(1.0, nLamps ? lampAt(Vec3(rx, gy + 0.5 * hgt, rz)) : 0.0)) * hash01(mix64(hc + 5)), dist);
                    RVert tri[3];
                    Vec3 vs[3] = {toView(p0.x, p0.y, p0.z), toView(p1.x, p1.y, p1.z), toView(pk.x, pk.y, pk.z)};
                    for (int q = 0; q < 3; q++) { tri[q].x = vs[q].x; tri[q].y = vs[q].y; tri[q].z = vs[q].z; tri[q].shade = shade; }
                    rasterTriangle(fb, tri, rp2, proj);
                }
            }
            // stromatolites, low lumpy domes dark with the mats that grow over them (the mats' bank, the kind's first accent: bank 10) on the
            // reef flat's pale carbonate (Shark Bay's), lit like the boulders
            if (microbial) forDomes(cx, cz, [&](const RockInst& R) {
                const double dist = std::sqrt((R.x - camPos.x) * (R.x - camPos.x) + (R.z - camPos.z) * (R.z - camPos.z));
                if (R.size / dist * proj.f < 0.8) return;
                const double hgt = R.size * (0.4 + 0.35 * hash01(mix64(R.h + 4))), a0 = hash01(mix64(R.h + 3)) * TAU;
                const int NA = dist < 40 ? 7 : 5;
                if (R.size > 0.5) drawBlobShadow(fb, R.x, R.z, R.size * 0.9, hgt);
                RasterParams rd; rd.bank = 10; rd.grain = &grain; rd.grainScale = 3.0 * FB_SCALE; rd.grain2 = mesoFor(MAT_MATS); rd.grain2Scale = 3.0;
                rd.grainWeight = clampd(1.0 - (dist - 30.0) / 120.0, 0.2, 1.0);
                Vec3 ring[2][8];
                for (int a = 0; a < NA; a++) {   // the foot sunk 0.15 m, a shoulder at 0.6 of the height, the crown; each ring's radius wobbles by the dome's own hash
                    const double ang = a0 + a * TAU / NA, wob = 0.85 + 0.3 * hash01(mix64(R.h + 10 + a));
                    ring[0][a] = Vec3(R.x + std::cos(ang) * R.size * wob, R.gy - 0.15, R.z + std::sin(ang) * R.size * wob);
                    ring[1][a] = Vec3(R.x + std::cos(ang + 0.3) * R.size * 0.72 * wob, R.gy + 0.6 * hgt, R.z + std::sin(ang + 0.3) * R.size * 0.72 * wob);
                }
                const Vec3 crown(R.x, R.gy + hgt, R.z);
                auto face = [&](const Vec3& p0, const Vec3& p1, const Vec3& p2) {
                    Vec3 n = normalize(cross(p1 - p0, p2 - p0)); if (n.y < 0) n = -n;
                    const double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
                    const double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt((p0 + p1 + p2) * (1.0 / 3.0), &n) : 0.0);   // the lamp
                    const double shade = fogShade(42 * std::pow(light, 0.6), dist);   // darker than the flat they stand on
                    RVert tri[3]; const Vec3 ws[3] = {p0, p1, p2};
                    for (int q = 0; q < 3; q++) {
                        const Vec3 v = toView(ws[q].x, ws[q].y, ws[q].z);
                        tri[q].x = v.x; tri[q].y = v.y; tri[q].z = v.z; tri[q].shade = shade; tri[q].u = ws[q].x + ws[q].y * 0.7; tri[q].v = ws[q].z + ws[q].y * 0.4;
                    }
                    rasterTriangle(fb, tri, rd, proj);
                };
                for (int a = 0; a < NA; a++) {
                    const int b2 = (a + 1) % NA;
                    face(ring[0][a], ring[0][b2], ring[1][b2]); face(ring[0][a], ring[1][b2], ring[1][a]);
                    face(ring[1][a], ring[1][b2], crown);
                }
                lastRocksDrawn++;
            });
            // sinter chimneys, slim pale cones on the sinter round the springs (the sinter's bank, the kind's second accent: bank 3)
            if (ventLife) forChimneys(cx, cz, [&](const RockInst& R) {
                const double dist = std::sqrt((R.x - camPos.x) * (R.x - camPos.x) + (R.z - camPos.z) * (R.z - camPos.z));
                const double hgt = chimneyHeight(R.h);
                if (hgt / dist * proj.f < 1.0) return;
                const double a0 = hash01(mix64(R.h + 3)) * TAU;
                const int NA = dist < 30 ? 6 : 4;
                drawBlobShadow(fb, R.x, R.z, R.size * 1.4, hgt);
                RasterParams rc; rc.bank = 3; rc.grain = &grain; rc.grainScale = 3.0 * FB_SCALE; rc.grain2 = mesoFor(MAT_SALT); rc.grain2Scale = 3.0;
                rc.grainWeight = clampd(1.0 - (dist - 20.0) / 100.0, 0.2, 1.0);
                Vec3 ring[3][6];
                static const double RF[3] = {1.0, 0.62, 0.36}, YF[3] = {0.0, 0.45, 0.88};   // the foot, the shoulder, the neck (radius and height fractions)
                for (int k = 0; k < 3; k++)
                    for (int a = 0; a < NA; a++) {
                        const double ang = a0 + (a + 0.5 * k) * TAU / NA, wob = 0.85 + 0.3 * hash01(mix64(R.h + 10 + 7 * k + a));
                        ring[k][a] = Vec3(R.x + std::cos(ang) * R.size * RF[k] * wob, R.gy + (k == 0 ? -0.1 : YF[k] * hgt), R.z + std::sin(ang) * R.size * RF[k] * wob);
                    }
                const Vec3 crown(R.x + 0.05 * std::cos(a0), R.gy + hgt, R.z + 0.05 * std::sin(a0));
                auto face = [&](const Vec3& p0, const Vec3& p1, const Vec3& p2) {
                    Vec3 n = normalize(cross(p1 - p0, p2 - p0)); if (n.y < 0) n = -n;
                    const double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
                    const double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt((p0 + p1 + p2) * (1.0 / 3.0), &n) : 0.0);   // the lamp
                    const double shade = fogShade(50 * std::pow(light, 0.6), dist);
                    RVert tri[3]; const Vec3 ws[3] = {p0, p1, p2};
                    for (int q = 0; q < 3; q++) {
                        const Vec3 v = toView(ws[q].x, ws[q].y, ws[q].z);
                        tri[q].x = v.x; tri[q].y = v.y; tri[q].z = v.z; tri[q].shade = shade; tri[q].u = ws[q].x + ws[q].y * 0.7; tri[q].v = ws[q].z + ws[q].y * 0.4;
                    }
                    rasterTriangle(fb, tri, rc, proj);
                };
                for (int k = 0; k < 2; k++)
                    for (int a = 0; a < NA; a++) {
                        const int b2 = (a + 1) % NA;
                        face(ring[k][a], ring[k][b2], ring[k + 1][b2]); face(ring[k][a], ring[k + 1][b2], ring[k + 1][a]);
                    }
                for (int a = 0; a < NA; a++) face(ring[2][a], ring[2][(a + 1) % NA], crown);
                lastRocksDrawn++;
            });
        }
}

void SurfaceView::drawCapsule(Framebuffer& fb, double t) {
    double cx = capsuleX, cz = capsuleZ, cy = capsuleY;
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double ambient = site.atmosphere ? (0.1 + 0.25 * env.skyBrightness) : 0.06;
    double rad = 1.6;
    drawBlobShadow(fb, cx, cz, rad * 1.25, rad * 0.9);
    RasterParams rp; rp.bank = 1;
    const int NA = 10, NE = 4;
    for (int e = 0; e < NE; e++) {
        double e0 = (double)e / NE * PI * 0.5, e1 = (double)(e + 1) / NE * PI * 0.5;
        for (int a = 0; a < NA; a++) {
            double a0 = (double)a / NA * TAU, a1 = (double)(a + 1) / NA * TAU;
            Vec3 pts[4] = {
                Vec3(std::cos(a0) * std::cos(e0), std::sin(e0), std::sin(a0) * std::cos(e0)),
                Vec3(std::cos(a1) * std::cos(e0), std::sin(e0), std::sin(a1) * std::cos(e0)),
                Vec3(std::cos(a1) * std::cos(e1), std::sin(e1), std::sin(a1) * std::cos(e1)),
                Vec3(std::cos(a0) * std::cos(e1), std::sin(e1), std::sin(a0) * std::cos(e1))};
            Vec3 n = normalize(pts[0] + pts[1] + pts[2] + pts[3]);
            double light = ambient + (1 - ambient) * std::max(0.0, dot(n, sd)) * sunUp;
            if (nLamps) light = std::min(1.0, light + (1 - ambient) * lampAt(Vec3(cx + n.x * rad, cy + 0.2 + n.y * rad * 0.8, cz + n.z * rad), &n));
            double shade = 34 + 26 * light;
            RVert q[4];
            for (int i = 0; i < 4; i++) {
                Vec3 v = toView(cx + pts[i].x * rad, cy + 0.2 + pts[i].y * rad * 0.8, cz + pts[i].z * rad);
                q[i].x = v.x; q[i].y = v.y; q[i].z = v.z; q[i].shade = shade;
            }
            rasterPolygon(fb, q, 4, rp, proj);
        }
    }
    // base ring
    {
        RVert q[NA];
        for (int a = 0; a < NA; a++) {
            double an = (double)a / NA * TAU;
            Vec3 v = toView(cx + std::cos(an) * rad * 1.3, cy + 0.15, cz + std::sin(an) * rad * 1.3);
            q[a].x = v.x; q[a].y = v.y; q[a].z = v.z; q[a].shade = 30;
        }
        rasterPolygon(fb, q, NA, rp, proj);
    }
    // beacon beam
    double dist = std::sqrt((cx - camPos.x) * (cx - camPos.x) + (cz - camPos.z) * (cz - camPos.z));
    double pulse = 0.7 + 0.3 * std::sin(t * 3.0);
    RVert a, b2;
    Vec3 va = toView(cx, cy + 0.2 + rad * 0.8, cz), vb = toView(cx, cy + 600, cz);
    a.x = va.x; a.y = va.y; a.z = va.z; a.shade = 63 * pulse;
    b2.x = vb.x; b2.y = vb.y; b2.z = vb.z; b2.shade = 40 * pulse;
    rasterLine3(fb, a, b2, 1, proj, true, dist < 60 ? 2 : 1);
}
