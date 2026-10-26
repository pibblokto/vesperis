// TestGame's hooks defined out of line (test_game.h declares them; the game's own files keep none)
#include "harness/test_game.h"
#include "game/ui.h"
#include "core/fs.h"
#include "core/noise.h"
#include "core/parallel.h"
#include "core/png.h"
#include "core/rng.h"
#include "peoples/buildings.h"
#include "galaxy/drainage.h"
#include "peoples/instruments.h"
#include "galaxy/planetmap.h"
#include "peoples/plan.h"
#include "galaxy/probe.h"
#include "peoples/records.h"
#include "peoples/roads.h"
#include "peoples/ruins.h"
#include "peoples/signals.h"
#include "peoples/works.h"
#include "space/probe_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

bool TestGame::testDriveSetup() {
    if (state != GameState::SURFACE || !surf.valid) return false;
    if (surf.inDrone) { surf.drone.landed = true; surf.drone.y = surf.site.surfaceHeight(surf.drone.x, surf.drone.z); surf.toggleDrone(); }   // R-403: out of a flying drone first
    surf.relocateCapsule(surf.player.x + 5, surf.player.z + 3);
    if (!surf.buggy.deployed && !surf.deployBuggy()) return false;
    surf.buggy.unfold = 1;
    surf.player.x = surf.buggy.x + 1.5; surf.player.z = surf.buggy.z;
    return surf.inBuggy || surf.toggleBuggy();
}

double TestGame::testDriveOpen() {
    if (!surf.valid) return 0;
    double x, z, heading;
    double run = surf.findOpenRun(x, z, heading);
    surf.buggy.x = x; surf.buggy.z = z; surf.buggy.y = surf.site.surfaceHeight(x, z); surf.buggy.heading = heading; surf.buggy.speed = 0;
    surf.buggy.tracks.clear(); surf.buggy.trackHead = 0;
    surf.player.x = x; surf.player.z = z; surf.player.y = surf.buggy.y; surf.player.yaw = heading;
    return run;
}

void TestGame::testAimAtBody(int body) {
    if (body < 0 || body >= (int)sys.bodies.size()) return;
    Vec3 fwd = normalize(sys.bodyPos(body, t) - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
}

std::string TestGame::testRoadInfo() const {
    std::string s = fmt("zoom roads %d (%.0f ms); site roads %zu (%.1f ms, %s, wear %.2f)", (int)zoomRoads.size(), zoomRoadsMs, surf.site.roads.size(), surf.site.roadsMs,
            surf.site.roadCultures[0].paved ? "paved" : "beaten", surf.site.roadCultures[0].roadWear);
    double d, along, hd; const SiteRoad* rd;
    if (!surf.site.roads.empty() && surf.site.roadAt(surf.player.x, surf.player.z, 30, d, along, hd, rd))
        s += fmt("; the nearest road %.1f m off (half %.1f), %.0f m along, heading %.0f, left %.2f, cover at the feet %.2f", d, rd->half, along, wrap2pi(hd) / DEG, roadLeft(rd->id, along, rd->wear),
                surf.site.roadCover(surf.player.x, surf.player.z));
    else s += "; no road within 30 m";
    s += fmt("; roads met %zu", roadsMet.size());
    return s;
}

bool TestGame::testWalkToWork(double withinM) {
    if (!surf.valid || surf.inVehicle() || !worldHadCivilisation(surf.site.gen)) return false;
    const Vec3 here = surf.site.unitAt(surf.player.x, surf.player.z);
    std::vector<std::shared_ptr<const Work>> near; worksAround(surf.site.gen, here, std::min(withinM, 60000.0), near);   // B-418: the land's within 60 km
    const std::shared_ptr<const WorldWorks> W = worldWorksOf(surf.site.gen);
    for (const Work& w : W->works) near.push_back(std::shared_ptr<const Work>(W, &w));   // and the world's wherever they are
    const Work* best = nullptr; double bd = withinM;
    for (const std::shared_ptr<const Work>& w : near) { if (!w->heard) continue; const double d = std::acos(clampd(dot(w->unit, here), -1, 1)) * W->Rm; if (d < bd) { bd = d; best = w.get(); } }
    if (!best) return false;
    double x, z; surf.site.localAt(best->unit, x, z);
    const double r = best->radiusM + 150, dx = surf.player.x - x, dz = surf.player.z - z, d = std::max(1.0, std::hypot(dx, dz));
    surf.player.x = x + dx / d * r; surf.player.z = z + dz / d * r; surf.player.y = surf.site.surfaceHeight(surf.player.x, surf.player.z); surf.player.vx = surf.player.vz = 0;
    return true;
}

bool TestGame::testWalkToRoad(double withinM) {
    double d, along, hd; const SiteRoad* rd;
    if (surf.site.roads.empty() || !surf.site.roadAt(surf.player.x, surf.player.z, withinM, d, along, hd, rd)) return false;
    double bestD = 1e9, bx = 0, bz = 0;   // the nearest point of its centre line
    for (size_t i = 0; i + 1 < rd->x.size(); i++) {
        double ax = rd->x[i], az = rd->z[i], ex = rd->x[i + 1] - ax, ez = rd->z[i + 1] - az, L2 = ex * ex + ez * ez;
        double t = L2 > 1e-9 ? clampd(((surf.player.x - ax) * ex + (surf.player.z - az) * ez) / L2, 0, 1) : 0;
        double qx = ax + ex * t, qz = az + ez * t, dd = std::hypot(surf.player.x - qx, surf.player.z - qz);
        if (dd < bestD) { bestD = dd; bx = qx; bz = qz; }
    }
    surf.player.x = bx; surf.player.z = bz; surf.player.y = surf.site.surfaceHeight(bx, bz); surf.player.yaw = hd; surf.player.vx = surf.player.vz = 0;
    return true;
}

void TestGame::testLandSite() {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return;
    const PlanetMap& m = spaceR.mapFor(sys.bodies[landBody]);
    Mat3 frame = sys.bodyFrame(landBody, t);
    Vec3 sunB = frame * normalize(sys.star.pos - sys.bodyPos(landBody, t));
    double anyLat = 1e9, anyLon = 0;
    setDrainageEnabled(false);   // S-01: the level check probes the relief without the rivers' tiles (a tile per probe would cost seconds)
    for (int pass = 0; pass < 2; pass++) {   // grassland first (the herd), then any land
        Rng r(sys.bodies[landBody].seed ^ 0x1A4DULL);
        int probed = 0;
        for (int k = 0; k < 400 && probed < 24; k++) {
            double la = r.range(-60 * DEG, 60 * DEG), lo = r.range(-PI, PI);
            if (dot(StarSystem::bodyFromLatLon(la, lo), sunB) < 0.3) continue;
            if (pass == 0 && m.materialAt(lo, la) != MAT_GRASS) continue;
            bool land = true;   // the texel and its neighbours two degrees out: a coast or a lake shore is still a swim
            for (int j = -1; j <= 1 && land; j++)
                for (int i = -1; i <= 1 && land; i++)
                    if (m.materialAt(lo + i * 2 * DEG, la + j * 2 * DEG) == MAT_WATER) land = false;
            if (!land) continue;
            if (anyLat > 1e8) { anyLat = la; anyLon = lo; }
            SurfaceSite probe; probe.initFrame(&sys, landBody, la, lo, t); probed++;   // level enough for the drive (the grassland of a locked world's first site lay on a mountainside: 18 m of drive)
            if (siteSlope(probe) < 0.1) { setDrainageEnabled(true); landLat = la; landLon = lo; return; }
        }
    }
    setDrainageEnabled(true);
    if (anyLat < 1e8) { landLat = anyLat; landLon = anyLon; }
}

int TestGame::testLandableBody() const {
    for (auto& b : sys.bodies) if (KINDS[b.type].landable && b.parent < 0 && b.type != PT_COMET) return b.index;   // O6-03: the flow's buggy steps need gravity (a comet came first since GEN 9)
    for (auto& b : sys.bodies) if (KINDS[b.type].landable && b.type != PT_COMET) return b.index;
    for (auto& b : sys.bodies) if (KINDS[b.type].landable && b.parent < 0) return b.index;
    for (auto& b : sys.bodies) if (KINDS[b.type].landable) return b.index;
    return -1;
}

void TestGame::testParkAt(const Star& s, int body, bool waitMap) {
    arriveAtStar(s, Vec3(0, 0, -1));
    if (body < 0 || body >= (int)sys.bodies.size()) return;
    const Body& b = sys.bodies[body];
    Vec3 bp = sys.bodyPos(body, t);
    Vec3 toStar = normalize(sys.star.pos - bp);
    Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
    ship.parkDir = normalize(toStar * 0.7 + side * 0.7 + Vec3(0, 0.2, 0));
    ship.parkDist = parkDistanceFor(b);
    ship.parkedBody = body; ship.localTarget = body; ship.mode = ShipState::PARKED;
    ship.pos = bp + ship.parkDir * ship.parkDist;
    Vec3 fwd = normalize(bp - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(fwd.y);
    nb.update(ship.pos);
    state = GameState::SPACE;
    worldPlanPrefetch(spaceR.genFor(b));
    spaceR.setSeason(sys, t);   // F-02: the season the next frame draws (the renderer held the last system's: the map was made for it, then again)
    if (KINDS[b.type].landable) { beginLanding(); if (waitMap) testWaitMap(); }   // the tests' landing map drawn from their next frame, as before
}

void TestGame::testWaitMap() { testWaitMapOf(landBody); }
void TestGame::testWaitMapOf(int bi) {   // a wait on the map's thread, not one of the game's blocking calls (`mapStats.blocked` counts the game's)
    if (bi < 0 || bi >= (int)sys.bodies.size()) return;
    while (!spaceR.mapIfReady(sys.bodies[bi])) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

std::string TestGame::testBeltDraw() const {
    int n3 = 0, x0 = 1 << 30, x1 = -1, y0 = 1 << 30, y1 = -1;
    for (int y = 0; y < FBH; y++)
        for (int x = 0; x < FBW; x++) {
            Pix p = fb.idx[y * FBW + x];
            if (bankOf(p) == 20 && intenOf(p) > 0) {
                n3++;
                x0 = std::min(x0, x);
                x1 = std::max(x1, x);
                y0 = std::min(y0, y);
                y1 = std::max(y1, y);
            }
        }
    int a, b; const_cast<TestGame*>(this)->choosePaletteBodies(a, b);
    BeltRock rk; std::string where = "no rock";
    if (beltRockNow(rk)) {
        Vec3 v = viewBasis() * (rk.pos - ship.pos);
        Vec3 f = normalize(rk.pos - ship.pos);
        where = fmt("rock at view (%.0f, %.0f, %.0f) km, %.1f km across, ship %.0f km off; ship yaw %.2f pitch %.2f, rock yaw %.2f pitch %.2f, cabin %.2f/%.2f", v.x, v.y, v.z, 2 * rk.radiusKm,
                length(rk.pos - ship.pos), ship.yaw, ship.pitch, std::atan2(f.x, f.z), std::asin(clampd(f.y, -1, 1)), cabin.yaw, cabin.pitch);
    }
    return fmt("rocks drawn %d (%d meshes), bank-20 pixels %d in x %d..%d y %d..%d, palette bodies %d/%d, cabin %d; %s", spaceR.lastBeltRocks, spaceR.lastBeltMeshes, n3, x0, x1, y0, y1, a, b,
            (int)settings.cabin, where.c_str());
}

void TestGame::testParkAtBelt(const Star& s, int k) {
    arriveAtStar(s, Vec3(0, 0, -1));
    if (k < 0 || k >= (int)sys.belts.size()) return;
    startApproachBelt(k);
    ship.flightDur = 0.001;   // the next frame completes the approach
    state = GameState::SPACE;
}

std::string TestGame::testHealth() const {
    auto bad = [](double v) { return !std::isfinite(v); };
    std::string r;
    if (bad(ship.pos.x) || bad(ship.pos.y) || bad(ship.pos.z)) r += "ship.pos ";
    if (bad(ship.yaw) || bad(ship.pitch)) r += "ship.angles ";
    if (bad(t) || bad(timeWarp) || bad(realTime)) r += "time ";
    if (surf.valid && (bad(surf.player.x) || bad(surf.player.y) || bad(surf.player.z) || bad(surf.player.yaw) || bad(surf.player.pitch))) r += "player ";
    if (surf.valid && (bad(surf.capsuleX) || bad(surf.capsuleZ))) r += "capsule ";
    return r;
}

void TestGame::testTypeText(const std::string& s) {
    Input in;
    for (char c : s) {
        in.newFrame();
        int k = c == ' ' ? KEY_SPACE : (c == '-' ? KEY_MINUS : (c >= '0' && c <= '9' ? KEY_0 + (c - '0') : KEY_A + (toupper((unsigned char)c) - 'A')));
        in.pressed[k] = true; in.down[k] = true;
        frame(in, 1.0 / 30);
    }
    in.newFrame(); in.pressed[KEY_ENTER] = true; in.down[KEY_ENTER] = true; frame(in, 1.0 / 30);
}

std::string TestGame::testDroneInfo() const {
    const Drone& d = surf.drone;
    return fmt("deployed=%d in=%d landed=%d speed=%.1f m/s alt=%.0f m vs=%+.1f heading=%.0f odometer=%.0f m rotor=%.2f chase=%d", (int)d.deployed, (int)surf.inDrone, (int)d.landed, d.speed,
            d.altAboveGround, d.vy, wrap2pi(d.heading) / DEG, d.odometer, d.rotor, (int)surf.chaseCam);
}

bool TestGame::testFlySetup(double altM) {
    if (state != GameState::SURFACE || !surf.valid) return false;
    if (surf.inBuggy) surf.toggleBuggy();
    surf.relocateCapsule(surf.player.x + 5, surf.player.z + 3);
    if (!surf.drone.deployed && !surf.deployDrone()) return false;
    surf.drone.unfold = 1;
    surf.player.x = surf.drone.x + 1.5; surf.player.z = surf.drone.z;
    if (!surf.inDrone && !surf.toggleDrone()) return false;
    surf.drone.landed = false; surf.drone.rotor = 0.8; surf.drone.y = surf.site.surfaceHeight(surf.drone.x, surf.drone.z) + altM; surf.drone.altAboveGround = altM;
    surf.player.y = surf.drone.y;
    return true;
}

std::string TestGame::testBuggyInfo() const {
    const Buggy& b = surf.buggy;
    return fmt("deployed=%d in=%d speed=%.1f m/s heading=%.0f odometer=%.0f m tracks=%zu chase=%d", (int)b.deployed, (int)surf.inBuggy, b.speed, wrap2pi(b.heading) / DEG, b.odometer, b.tracks.size(),
            (int)surf.chaseCam);
}

std::string TestGame::testDebugInfo() const {
    std::string r = fmt("mode=%d parked=%d local=%d belt=%d/%d yaw=%.2f pitch=%.2f", (int)ship.mode, ship.parkedBody, ship.localTarget, ship.targetBelt, ship.parkedBelt, ship.yaw, ship.pitch);
    if (ship.parkedBody >= 0 && ship.parkedBody < (int)sys.bodies.size()) {
        Vec3 fwd = normalize(sys.bodyPos(ship.parkedBody, t) - ship.pos);
        r += fmt(" bodyYaw=%.2f bodyPitch=%.2f dist=%.0f R=%.0f", std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)), length(sys.bodyPos(ship.parkedBody, t) - ship.pos),
                sys.bodies[ship.parkedBody].radiusKm);
    }
    return r;
}

std::string TestGame::testGraveInfo() const {
    const SurfaceView::NearGrave& ng = surf.nearGrave;
    std::string last = guide.log.empty() ? std::string("-") : guide.log.back().kind + ": " + guide.log.back().text;
    return fmt("grave: %s; %zu graves in the guide; last log: %s", ng.k >= 0 ? fmt("%s (%.1f m, %s)", ng.line.c_str(), ng.dist, SETTLEMENT_CLASS_NAMES[ng.sclass]).c_str() : "none within reach",
            guide.graves.size(), last.c_str());
}
bool TestGame::testWalkToGrave() {   // C-12: a metre in front of the nearest settlement's first stone, facing it
    if (!surf.valid || surf.inVehicle()) return false;
    bool found = false; double bestD = 1e18, sx = 0, sz = 0, yaw = 0;
    surf.forNearbyRuins(surf.player.x, surf.player.z, 3, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        if (cell.graves.empty()) return;
        const Grave& g = cell.graves[0];
        double x = r.x + g.x, z = r.z + g.z, d = std::hypot(x - surf.player.x, z - surf.player.z);
        if (d < bestD) { bestD = d; sx = x + std::sin(g.heading); sz = z + std::cos(g.heading); yaw = g.heading + PI; found = true; }
    });
    if (!found) return false;
    surf.player.x = sx; surf.player.z = sz; surf.player.y = surf.site.surfaceHeight(sx, sz); surf.player.yaw = yaw; surf.player.pitch = -0.35;
    return true;
}

int TestGame::testWalkToRecord(int medium, bool archive) {
    if (!surf.valid || surf.inVehicle()) return -1;
    double bestD = 1e18; int found = -1; ShardSite site; double rx = 0, rz = 0, bx = 0, bz = 0;
    surf.forNearbyRuins(surf.player.x, surf.player.z, 3, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        for (const ShardSite& st : cell.shards) {
            if (surf.shardsFound.count(st.index) || (medium >= 0 && st.medium != medium) || archive != (st.archive > 0)) continue;
            const double x = r.x + st.x, z = r.z + st.z, d = std::hypot(x - surf.player.x, z - surf.player.z);
            if (d >= bestD) continue;
            bestD = d; found = st.index; site = st; rx = r.x; rz = r.z;
            const Building& b = cell.spec.buildings[st.building]; bx = r.x + b.x; bz = r.z + b.z;
        }
    });
    if (found < 0) return -1;
    const double x = rx + site.x, z = rz + site.z;
    double dx, dz;
    if (site.wallD > 0) { dx = std::sin(site.heading); dz = std::cos(site.heading); }   // before the wall, facing it
    else if (site.archive) {   // square to the row: toward the room's middle, or away from the structure a ring of them stands round
        const double px = std::cos(site.heading), pz = -std::sin(site.heading);
        double side = px * (bx - x) + pz * (bz - z) < 0 ? -1.0 : 1.0;
        if (site.outside) side = -side;
        dx = px * side; dz = pz * side;
    } else {   // toward its building's middle from a record in a room, away from it at a foot
        dx = bx - x; dz = bz - z; double L = std::hypot(dx, dz);
        if (L < 0.3) { dx = std::sin(site.heading); dz = std::cos(site.heading); L = 1; }
        dx /= L; dz /= L;
        if (site.outside) { dx = -dx; dz = -dz; }
    }
    // the first stand clear of the pieces (a column, a pier, a wall) at a distance that frames it (an archive's row from either side)
    static const double BACK[2][5] = {{1.25, 1.0, 1.5, 0.8, 1.8}, {1.5, 1.2, 1.9, 1.0, 0.8}};
    std::vector<SurfaceView::Collider> cols;
    double sx = x + dx * 1.25, sz = z + dz * 1.25, back = 1.25;
    bool placed = false;
    for (int side = 0; side < (site.archive ? 2 : 1) && !placed; side++) {
        if (side == 1) { dx = -dx; dz = -dz; }
        for (int k = 0; k < 5 && !placed; k++) {
            const double bk = site.wallD > 0 ? 0.9 + 0.2 * k : BACK[site.archive ? 1 : 0][k];
            const double cx = x + dx * bk, cz = z + dz * bk, gy = surf.site.surfaceHeight(cx, cz);
            surf.collectColliders(cx, cz, cols);
            bool blocked = false;
            for (const SurfaceView::Collider& c : cols) if (std::hypot(c.x - cx, c.z - cz) < c.r + 0.3 && c.y1 > gy + 0.4 && c.y0 < gy + 1.8) { blocked = true; break; }
            if (!blocked) { sx = cx; sz = cz; back = bk; placed = true; }
        }
    }
    if (!placed) { if (site.archive) { dx = -dx; dz = -dz; } sx = x + dx * 1.25; sz = z + dz * 1.25; back = 1.25; }
    surf.player.x = sx; surf.player.z = sz; surf.player.y = surf.site.surfaceHeight(sx, sz);
    surf.player.yaw = std::atan2(-dx, -dz);
    surf.player.pitch = site.wallD > 0 ? -0.3 : -std::atan2(1.45, back + 0.4);   // the eye 1.6 m up, the object a little ahead of the frame's middle
    return found;
}

int TestGame::testWalkToInstrument(int age) {   // C-11: before the nearest instrument not taken (of that age), as `testWalkToRecord` stands before a record
    if (!surf.valid || surf.inVehicle()) return -1;
    double bestD = 1e18; int found = -1; InstrumentSite site; double rx = 0, rz = 0, bx = 0, bz = 0;
    surf.forNearbyRuins(surf.player.x, surf.player.z, 3, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        if (cell.instrumentIds.size() != cell.instruments.size()) return;
        for (size_t i = 0; i < cell.instruments.size(); i++) {
            const InstrumentSite& st = cell.instruments[i];
            const int id = cell.instrumentIds[i];
            if (id < 0 || surf.instrumentsFound.count(id) || (age >= 0 && st.age != age)) continue;
            const double x = r.x + st.x, z = r.z + st.z, d = std::hypot(x - surf.player.x, z - surf.player.z);
            if (d >= bestD) continue;
            bestD = d; found = id; site = st; rx = r.x; rz = r.z;
            const Building& b = cell.spec.buildings[st.building]; bx = r.x + b.x; bz = r.z + b.z;
        }
    });
    if (found < 0) return -1;
    const double x = rx + site.x, z = rz + site.z;
    double dx = bx - x, dz = bz - z, L = std::hypot(dx, dz);
    if (L < 0.3) { dx = std::sin(site.heading); dz = std::cos(site.heading); L = 1; }
    dx /= L; dz /= L;
    std::vector<SurfaceView::Collider> cols;
    std::vector<std::pair<double, double>> records;   // the records not taken round it: a stand has the instrument nearer than any (E takes the nearer)
    surf.forNearbyRuins(x, z, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        for (const ShardSite& st : cell.shards) if (!surf.shardsFound.count(st.index) && std::hypot(r.x + st.x - x, r.z + st.z - z) < 4) records.push_back({r.x + st.x, r.z + st.z});
    });
    double sx = x + dx * 1.3, sz = z + dz * 1.3, back = 1.3;
    bool placed = false;
    for (int pass = 0; pass < 2 && !placed; pass++)
        for (int turn = 0; turn < 16 && !placed; turn++) {   // round it until a stand clear of the pieces frames it (first one where nothing else lies nearer)
            const double a = turn * PI / 8, ux = dx * std::cos(a) - dz * std::sin(a), uz = dx * std::sin(a) + dz * std::cos(a);
            for (double bk : {1.2, 0.9, 1.6}) {
                const double cx = x + ux * bk, cz = z + uz * bk, gy = surf.site.surfaceHeight(cx, cz);
                if (pass == 0) { bool nearer = false; for (const auto& rc : records) if (std::hypot(rc.first - cx, rc.second - cz) < bk + 0.05) nearer = true; if (nearer) continue; }
                surf.collectColliders(cx, cz, cols);
                bool blocked = false;
                for (const SurfaceView::Collider& c : cols) if (std::hypot(c.x - cx, c.z - cz) < c.r + 0.3 && c.y1 > gy + 0.4 && c.y0 < gy + 1.8) { blocked = true; break; }
                if (!blocked) { sx = cx; sz = cz; back = bk; dx = ux; dz = uz; placed = true; break; }
            }
        }
    surf.player.x = sx; surf.player.z = sz; surf.player.y = surf.site.surfaceHeight(sx, sz);
    surf.player.yaw = std::atan2(-dx, -dz);
    surf.player.pitch = -std::atan2(1.4, back + 0.3);
    return found;
}

bool TestGame::testWalkToShard() {   // C-13: a metre from the nearest settlement's first shard not yet taken, facing it
    if (!surf.valid || surf.inVehicle()) return false;
    bool found = false; double bestD = 1e18, sx = 0, sz = 0, yaw = 0;
    surf.forNearbyRuins(surf.player.x, surf.player.z, 3, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        for (const ShardSite& st : cell.shards) {
            if (surf.shardsFound.count(st.index)) continue;
            double x = r.x + st.x, z = r.z + st.z, d = std::hypot(x - surf.player.x, z - surf.player.z);
            if (d < bestD) { bestD = d; sx = x + std::sin(st.heading); sz = z + std::cos(st.heading); yaw = st.heading + PI; found = true; }
            break;
        }
    });
    if (!found) return false;
    surf.player.x = sx; surf.player.z = sz; surf.player.y = surf.site.surfaceHeight(sx, sz); surf.player.yaw = yaw; surf.player.pitch = -0.5;
    return true;
}

void TestGame::testTelescopeSettle() { Input in; telePlateFinish = true; frame(in, 1.0 / 30); }
std::string TestGame::testTelescopePlate() const { return spaceR.plateInfo(); }
double TestGame::testTelescopePlateProgress() const { int pb = telescopePlateBody(); return pb >= 0 && spaceR.plate().body == pb ? spaceR.plateProgress() : -1; }

// the plate's own cells (ones that hold their own sample) against the planet function sampled again at the cell's centre
// and the plate's detail: the same function with the same arguments must give the same numbers
void TestGame::testPlateCheck(int n, double& maxAlb, int& matMiss, double& maxH) {
    const DetailPlate& p = spaceR.plate();
    maxAlb = 0; matMiss = 0; maxH = 0;
    if (!p.valid() || p.body < 0 || !sys.valid || p.body >= (int)sys.bodies.size()) { matMiss = -1; return; }
    const BodyGen& g = spaceR.genFor(sys.bodies[p.body]);
    DrainageOff off(p.noDrainage);
    int checked = 0;
    for (int k = 0; k < n * 8 && checked < n; k++) {
        int i = (k * 37 + 11) % p.W, j = (k * 53 + 7) % p.H;
        size_t o = (size_t)j * p.W + i;
        if (!p.exact[o] || p.material[o] == 255) continue;
        Vec3 u = SpaceRenderer::plateCellUnit(p, i, j);
        SurfaceSample ss = sampleSurface(g, u, p.texelM);
        double lat, lon; StarSystem::latLonFromBody(u, lat, lon);
        double alb = ss.albedo;
        if (kindHas(g.type, KT_GAS_GIANT) && std::fabs(lat) > 76 * DEG) alb *= 1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat));
        maxAlb = std::max(maxAlb, std::fabs(alb - p.albedo[o] / 255.0));
        if (ss.material != p.material[o]) matMiss++;
        maxH = std::max(maxH, std::fabs(ss.height - p.height[o]));
        checked++;
    }
    if (!checked) matMiss = -1;
}

std::string TestGame::testTelescopeInfo() const {
    if (!tele.on) return "stowed";
    return fmt("x%.0f, field %.3f deg, target %d '%s', tracking %d%s, offsets %.5f/%.5f rad, yaw %.4f pitch %.4f", telescopeZoom(), telescopeFieldDeg(), tele.target, telescopeTargetLine().c_str(),
            tele.track, telescopeStabilised() ? " (stabilised)" : "", tele.offYaw, tele.offPitch, ship.yaw, ship.pitch);
}

long TestGame::testPlateBegun() const { return spaceR.plateBegun; }

// the ground under the reticle of the parked body: the central ray's point on its sphere, in the body's frame
bool TestGame::testTelescopeGround(double& lat, double& lon) const {
    int b = ship.parkedBody;
    if (!sys.valid || ship.mode != ShipState::PARKED || b < 0 || b >= (int)sys.bodies.size()) return false;
    return viewGround(b, lat, lon);
}

bool TestGame::testGroundScreen(int body, double lat, double lon, double& sx, double& sy) const {
    if (!sys.valid || body < 0 || body >= (int)sys.bodies.size()) return false;
    Vec3 at = sys.bodyPos(body, t) + sys.bodyFrame(body, t).transposed() * StarSystem::bodyFromLatLon(lat, lon) * sys.bodies[body].radiusKm;
    Vec3 v = viewBasis() * (at - ship.pos);
    if (v.z <= 0) return false;
    Proj pj = tele.on ? telescopeProj() : spaceR.proj;
    sx = pj.cx + pj.f * v.x / v.z; sy = pj.cy - pj.f * v.y / v.z;
    return true;
}

bool TestGame::testSubShip(double& lat, double& lon) const {
    int b = ship.parkedBody;
    if (!sys.valid || ship.mode != ShipState::PARKED || b < 0 || b >= (int)sys.bodies.size() || ship.parkedBelt >= 0) return false;
    StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), lat, lon);
    return true;
}

bool TestGame::testBodyScreen(int body, double& sx, double& sy, double& rpx) const {
    if (body < 0 || body >= (int)spaceR.bodyInfo.size() || spaceR.bodyInfo[body].body != body) return false;
    const BodyScreenInfo& bi = spaceR.bodyInfo[body];
    sx = bi.sx; sy = bi.sy; rpx = bi.radiusPx;
    return bi.inFront;
}

int TestGame::testTelescopePick() const {
    int best = -1; double bestAng = 0;
    if (!sys.valid) return -1;
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        if (i == ship.parkedBody) continue;
        double ang = sys.bodies[i].radiusKm / std::max(1.0, length(sys.bodyPos(i, t) - ship.pos));
        if (ang > bestAng) { bestAng = ang; best = i; }
    }
    return best;
}

double TestGame::testProbeFlash(double clock) const { return probe.active ? flashAt(probe.atm, probe.flight, probe.seed, clock, nullptr, 0, 0, nullptr, nullptr).strength : 0; }
bool TestGame::testProbeBolt(double clock) const {
    if (!probe.active || !probe.bands.valid()) return false;
    double px[GIANT_MAX_DECKS], pz[GIANT_MAX_DECKS] = {0, 0, 0};
    for (int k = 0; k < GIANT_MAX_DECKS; k++) px[k] = -probeDeckDrift(k, clock);
    return flashAt(probe.atm, probe.flight, probe.seed, clock, &probe.decks, probe.descYaw + probe.camYaw, giantAltitudeKm(probe.atm, std::max(probeBarAt(probe.flight, clock), 1e-9)), px, pz).bolt;
}
int TestGame::testProbeStage() const { return !probe.active ? -1 : (probe.endClock >= 0 ? PS_END : probeStageAt(probe.atm, probe.flight, probe.clock)); }
double TestGame::testProbeBar() const { return probe.active ? probeBarAt(probe.flight, probe.clock) : 0; }
double TestGame::testProbeEndClock() const { return probe.active ? probeEndClock(probe.atm, probe.flight) : -1; }
void TestGame::testProbeSkip(double toClock) {
    if (!probe.active || probe.endClock >= 0 || toClock <= probe.clock) return;
    double endC = probeEndClock(probe.atm, probe.flight);
    probe.clock = std::min(toClock, endC);
    if (probe.flight.chuteOpen >= 0 && probe.clock >= probe.flight.chuteClose) probe.tornNoted = true;
    if (probe.clock >= endC) probeEnd();
}
std::string TestGame::testProbeViewInfo() const {
    const ProbeViewStats& v = probeViewStats();
    return fmt("floor %d (%.0f%%, shade %.0f) ceiling %d (%.0f%%, shade %.0f) fog %d (T %.2f) pano %dx%d; %.1f + %.1f + %.1f + %.1f ms; sun %.0f deg, light %.2f/%.2f/%.2f, albedo %.2f", v.floor,
                   v.covFloor * 100, v.shadeFloor, v.ceil, v.covCeil * 100, v.shadeCeil, v.fog, v.fogT, v.columns, v.rows, v.msPano, v.msFog, v.msNodes, v.msSpread, v.sunElDeg, v.direct0, v.diffuse0,
                   v.inside0, v.alb0) +
            (v.flash > 0 ? fmt("; flash %.2f at %.0f %.0f %.0f km, frame %.0f,%.0f", v.flash, v.flashPos.x, v.flashPos.y, v.flashPos.z, v.flashSx, v.flashSy) : std::string()) +
            (v.inHole ? std::string("; in a hole") : std::string()) + (v.aurora > 0.0005 ? fmt("; aurora %.3f", v.aurora) : std::string());
}
std::string TestGame::testProbeInfo() const {
    if (!probe.active) return "no probe";
    double c = probe.clock, bar = probeBarAt(probe.flight, c);
    return fmt("body %d aim %.2f/%.2f deg clock %.1f s stage %s bar %.4f alt %.1f km T %.0f K wind %.0f m/s chute %.1f..%.1f crush %.1f bar end %.1f s%s", probe.body, probe.aimLat / DEG,
            probe.aimLon / DEG, c, probeStageLabel(c).c_str(), bar, bar > 0 ? giantAltitudeKm(probe.atm, bar) : 0.0, bar > 0 ? giantTemperatureK(probe.atm, bar) : 0.0,
            giantWindEast(probe.atm, probe.aimLat, bar), probe.flight.chuteOpen, probe.flight.chuteClose, probe.flight.crushBar, probeEndClock(probe.atm, probe.flight),
            probe.endClock >= 0 ? fmt(" ENDED (%s)", probe.endCause == 1 ? "hull" : "heat").c_str() : "");
}

bool TestGame::testDronesLaunch() { const bool was = bay.out; droneBayLaunch(); return !was && bay.out; }

void TestGame::testDronesSettle() {
    while (droneJobBusy) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    droneCollect();
    if (bay.out && !bay.planned) dronePlanTrip();
}

void TestGame::testDronesSkip(double clock) {
    while (bay.clock >= 0 && bay.clock < clock) updateDrones(std::min(0.25, clock - bay.clock + 1e-9));
}

int TestGame::testDronesFinds(int kind) const {
    int n = 0;
    for (const DroneRun& R : bay.run) for (const DroneFindRec& f : R.finds) if (kind < 0 || f.kind == kind) n++;
    return n;
}
int TestGame::testDronesAboard() const {
    int n = 0;
    for (const DroneRun& R : bay.run) for (const DroneFindRec& f : R.finds) if (f.aboard) n++;
    return n;
}
std::string TestGame::testDronesFindKey(int k, int i) const {
    if (k < 0 || k >= DRONE_COUNT || i < 0 || i >= (int)bay.run[k].finds.size()) return std::string();
    return droneFindKey(bay.run[k].finds[i]);
}

int TestGame::testDronesWorldItems(int kind, bool unheld) {
    if (!sys.valid || ship.parkedBody < 0) return -1;
    droneWorldAsk(ship.parkedBody);
    testDronesSettle();
    if (!droneWorldHere(sys.star.seed, ship.parkedBody)) return -1;
    const std::string wk = worldKeyOf(ship.parkedBody);
    int n = 0;
    for (const DroneItem& it : bay.world.items) {
        if (kind >= 0 && it.kind != kind) continue;
        const std::string key = findKeyOf(wk, it.kind, it.index);
        if (unheld && (it.kind == DF_INSTRUMENT ? guide.instruments.count(key) : guide.shards.count(key))) continue;
        n++;
    }
    return n;
}

std::string TestGame::testDronesInfo() const {
    std::string s = fmt("the bay: clock %.1f, %s%s, cooldown %.0f s; ", bay.clock, bay.out ? "out" : (droneBayReady() ? "ready" : "charging"), bay.planned ? ", planned" : "", droneCooldownLeft());
    if (bay.worldValid)
        s += fmt("the world's items %d (%d places, homes %s, read in %.0f ms); ", (int)bay.world.items.size(), (int)bay.world.places.size(), bay.world.homes ? "yes" : "no", bay.world.buildMs);
    for (int k = 0; k < DRONE_COUNT; k++) {
        const DroneRun& R = bay.run[k];
        s += fmt("drone %d: %s, wanted %d, %d finds, %d stops (out %.1f down %.1f up %.1f dock %.1f)%s", k + 1, droneStateLine(k).c_str(), R.wanted, (int)R.finds.size(), (int)R.stops.size(), R.out,
                R.down, R.up, R.dock, k + 1 < DRONE_COUNT ? "; " : "");
    }
    return s;
}

bool TestGame::testDrainageReady() const {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return true;
    return drainageReady(BodyGen::make(sys.bodies[landBody]), StarSystem::bodyFromLatLon(landLat, landLon), 60000.0);
}

double TestGame::testMapShadowMax() {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return 0;
    std::vector<Vec3> units(PlanetMap::W * PlanetMap::H);
    for (int y = 0; y < PlanetMap::H; y++)
        for (int x = 0; x < PlanetMap::W; x++) units[y * PlanetMap::W + x] = StarSystem::bodyFromLatLon((0.5 - (y + 0.5) / PlanetMap::H) * PI, ((x + 0.5) / PlanetMap::W) * TAU - PI);
    std::vector<uint8_t> sh;
    buildShadowMap(units, sh);
    int mx = 0; for (uint8_t v : sh) mx = std::max(mx, (int)v);
    return mx / 255.0;
}

// N0-02: the landing map's base colour at a texel, for the consistency diagnosis
uint32_t TestGame::testLandingMapTexel(int tx, int ty, bool clouds) {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return 0;
    mapClouds = clouds;
    const Body& b = sys.bodies[landBody];
    if (mapBaseSeed != b.seed || mapBaseClouds != mapClouds || mapBase.empty()) buildLandingMapBase();
    tx = ((tx % PlanetMap::W) + PlanetMap::W) % PlanetMap::W;
    ty = clampi(ty, 0, PlanetMap::H - 1);
    return mapBase[ty * PlanetMap::W + tx];
}

double TestGame::testFrontBandAt(double latDeg, double lonDeg) {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return 0;
    const SpaceRenderer::FrontMap* fm = spaceR.frontsFor(sys.bodies[landBody], t);
    return fm ? frontMapAt(fm->cloud, PlanetMap::W, PlanetMap::H, lonDeg * DEG, latDeg * DEG) : 0.0;
}

// N1-06: the relief class the landing map would print at a site
const char* TestGame::testReliefClass(double lat, double lon) {
    probeSite.initFrame(&sys, landBody, lat, lon, t);
    probeBody = landBody; probeLat = lat; probeLon = lon;
    return reliefClass(siteSlope(probeSite));
}

std::string TestGame::testLandPeoplesInfo() {
    const LandKnown K = landKnownOf(landBody);
    std::string s = fmt("the peoples %s; %s", landPeoples ? "shown" : "hidden", K.ready ? "read" : "being read");
    for (int k = 0; k < K.peoples; k++) s += fmt("; people %d '%s' %d of %d held, %zu places known%s", k + 1, K.name[k].c_str(), K.held[k], K.total[k], K.ranks[k].size(), K.all(k) ? ", ALL" : "");
    s += fmt("; drawn: %d places, %d ways, %d land pixels, %d marks, %d rings, %d capitals named", landDrawn.places, landDrawn.ways, landDrawn.land, landDrawn.marks, landDrawn.rings,
            landDrawn.capital);
    return s;
}

int TestGame::testPlayerItems() { playerBuildList(); return (int)player.items.size(); }
std::string TestGame::testPlayerItemLine(int i) const {
    if (i < 0 || i >= (int)player.items.size()) return std::string();
    const PlayerItem& it = player.items[i];
    bool foreign = false; const std::string* nm = it.music ? shardNameOf(playerItemKey(it), foreign) : nullptr;
    return fmt("%s %s%s, %s, year %d, %s%s", playerItemKey(it).c_str(), it.music ? "music" : "a recording", it.heard ? " off the air" : "", playerItemTitle(it, nm).c_str(), it.year, it.people.c_str(),
            it.seconds > 0 ? fmt(", %.1f s", it.seconds).c_str() : "");
}
bool TestGame::testPlayerPlay(int item) { if (player.items.empty()) playerBuildList(); if (item < 0 || item >= (int)player.items.size()) return false; playerPlay(item); return true; }
std::string TestGame::testPlayerInfo() const {
    if (player.playing < 0) return fmt("the player: nothing plays (%d items, %d played to their end)", (int)player.items.size(), player.finished);
    return fmt("the player: item %d '%s - %s', %.1f of %.1f s%s%s%s; the synth's piece %s, speech %s, medium %d; %d played to their end", player.playing, player.title.c_str(), player.people.c_str(),
            player.t, player.secs, player.paused ? ", paused" : "", player.held ? ", held" : "", player.waiting ? ", waiting for its world" : "",
            audio.piece == &player.world.piece ? "the player's" : (audio.piece ? "another's" : "none"), audio.speech == &player.world.speech ? "the player's" : (audio.speech ? "another's" : "none"),
            audio.medium, player.finished);
}

std::string TestGame::testAutoInfo() const {
    return fmt("%s (%s) %s: %s; towns %d, stars %d, parkings %d, landings %d, probes %d, records %d, outings %d, steps %d (%d given up), unsticks %d",
               AUTO_PROFILE_NAMES[ax.profile], AUTO_PROFILE_NAMES[ax.focus], ax.on ? "on" : "off", ax.doing.c_str(), ax.towns, ax.stars, ax.bodies, ax.landings, ax.probes,
               ax.records, ax.outings, ax.steps, ax.timeouts, ax.unsticks);
}

int TestGame::testAutoCount(int what) const {
    const int v[8] = {ax.stars, ax.bodies, ax.landings, ax.probes, ax.records, ax.outings, ax.timeouts, ax.unsticks};
    return what >= 0 && what < 8 ? v[what] : -1;
}

Vec3 TestGame::testViewDir() const {
    if (stateHas(state, ST_GROUND)) return forwardOf(surf.player.yaw, surf.player.pitch);
    if (state == GameState::PROBE) return forwardOf(probe.camYaw, probe.camPitch);
    return viewBasis().row(2);
}

std::string TestGame::testAngles() const { return fmt("ship %.4f %.4f cabin %.4f %.4f mode %d", ship.yaw, ship.pitch, cabin.yaw, cabin.pitch, (int)ship.mode); }

bool TestGame::testCivilisedHere() const {
    if (!sys.valid) return false;
    for (const Body& b : sys.bodies) if (kindHas(b.type, KT_PEOPLE) && worldHadCivilisation(BodyGen::make(b))) return true;
    return false;
}

bool TestGame::testAimAtSignal(int which) {
    if (which < 0 || which >= (int)radar.all.size()) return false;
    Vec3 fwd = normalize(radar.all[which].pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
    cabin.yaw = 0; cabin.pitch = 0;
    return true;
}

std::string TestGame::testRadarInfo() const {
    const RadarState& r = radar;
    std::string s = fmt("radar: %s, gain x%.0f, %zu far (%d cells, %.1f ms) + %zu local", r.on ? "on" : "off", radarGain(), r.far.size(), r.scanCells, r.scanMs, r.local.size());
    if (r.gainStep > 0)
        s += r.deepReady ? fmt(" + %d deep of %zu (%d frames, %.1f ms)", (int)std::count_if(r.deep.begin(), r.deep.end(), [&](const Signal& d) { return d.needGain <= radarGain(); }), r.deep.size(),
                                   r.deepFrames, r.deepMs)
                         : fmt(" + deep scan at %d cells", r.deepScan.cells);
    s += fmt(", reading %.2f (the beam's sum %.2f, the floor %.2f)%s", r.reading, r.sum, r.floorNow, r.overload ? " OVERLOAD" : "");
    s += fmt(", the sky %.2f", r.sky);
    if (r.under >= 0 && r.under < (int)r.all.size())
        s += fmt(", under it %s %.0f ly (%.1f deg off, %.2f through%s)", SIGNAL_KIND_NAMES[r.all[r.under].kind], r.all[r.under].distLy, r.angle[r.under] / DEG, audio.radarSignal2,
                audio.radarHet > 0 ? fmt(", a whistle at %.0f Hz", audio.radarHetHz).c_str() : "");
    if (r.rise.on) s += fmt(", a false rise %.1f deg off (%.2f%s)", r.riseAngle / DEG, r.riseGain, r.rise.spent ? ", spent" : "");
    if (r.nothingT > 0) s += ", NOTHING THERE";
    if (r.wordT >= 0) s += fmt(", a word back from the static: \"%s\"", r.wordSaid.c_str());
    if (r.best >= 0 && r.best < (int)r.all.size())
        s += fmt(", best %d (%s, %.1f deg off, gain %.2f, %.0f ly, strength %.2f)", r.best, SIGNAL_KIND_NAMES[r.all[r.best].kind], r.bestAngle / DEG, r.bestGain, r.all[r.best].distLy,
                r.all[r.best].strength);
    s += fmt(", hold %.1f of %.1f s, locked %d", r.holdT, r.lockSecs, r.locked);
    if (r.locked >= 0 && r.locked < (int)r.all.size()) s += fmt(" (%s)", SIGNAL_KIND_NAMES[r.all[r.locked].kind]);
    s += fmt(", clarity %.2f, content %s", r.clarity,
            r.contentT < 0
                    ? "none"
                    : fmt("S%d %s %.1f of %.0f s%s", r.shard, r.prog.kind >= 0 && r.prog.kind < RP_COUNT ? RADIO_PROGRAMME_NAMES[r.prog.kind] : "?", r.contentT, r.contentSecs,
                              audio.radio ? (audio.piece ? " (a piece set)"
                                                         : (audio.speech ? (r.prog.repeats > 1 ? fmt(" (a speech set, repeat %d of %d)", r.loopI + 1, r.prog.repeats).c_str() : " (a speech set)")
                                                                         : (r.prog.machine ? " (a machine)" : " (NO pointer)")))
                                          : " (not radio)")
                              .c_str());
    return s;
}

std::string TestGame::testTapeInfo() const {
    static const char* const PH[] = {"off", "deck", "play", "fade"};
    std::string author, title;
    const double a = tapeCaption(author, title);
    return fmt("%s, track %d of %zu from %.1f s, level %.3f (gain %.2f), duck %.2f, music \"%s\", caption %.2f \"%s / %s\"", PH[clampi(tape.phase, 0, 3)], tape.track,
               tape.tracks.size(), tape.from, audio.musicGain, tape.gain, audio.duck, audio.music.c_str(), a, author.c_str(), title.c_str());
}

std::string TestGame::testTapeTrack(size_t i) const {
    if (i >= tape.tracks.size()) return std::string();
    const TapeTrack& k = tape.tracks[i];
    return fmt("%s | %s | %s | %+.1f dB", k.file.c_str(), k.author.c_str(), k.title.c_str(), k.gainDb);
}

std::string TestGame::testShardsInfo() const {
    int read = 0; for (const DecodedWord& w : shardWords) if (w.known) read++;
    std::string piece;
    if (shardsLevel == 2 && shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].music)
        piece = fmt("; piece: %s, %.0f s, %zu notes, beat %.1f of %.0f, %s", PIECE_FORM_NAMES[shardWorld.piece.form], shardWorld.piece.seconds, shardWorld.piece.notes.size(), pieceBeatNow(),
                shardWorld.piece.beats, pieceEnded() ? "ended" : (piecePaused ? "paused" : (audio.piece ? "playing" : "stopped")));
    std::string speech;
    if (shardsLevel == 2 && shardWorld.valid && !shardHeld.empty() && !shardWorld.shards[shardHeld[shardSel]].music)
        speech = fmt("; speech: %.1f of %.1f s, word %d, %s", speechT >= 0 ? speechT : 0.0, shardWorld.speech.seconds, speechT >= 0 ? speechWordAt(shardWorld.speech, speechT) : -1,
                     speechT < 0 ? "stopped" : (speechEnded() ? "ended" : (speechPaused ? "paused" : "speaking")));
    int lentN = 0; for (int i : shardHeld) if (shardLent(i)) lentN++;
    std::string page = shardsLevel == 2 && shardPageStart.size() > 1 ? fmt(", page %d of %zu", shardPage + 1, shardPageStart.size()) : std::string();
    return fmt("decoder: level %d, %zu worlds, %zu held of %s (people %d of %d, %d lent), shard %d: %d of %zu words read%s, %s%s%s", shardsLevel, shardWorldKeys.size(), shardHeld.size(),
            shardWorld.valid ? shardWorld.key.c_str() : "-", shardWorld.which + 1, shardWorld.peoples, lentN, shardSel, read, shardWords.size(), page.c_str(),
            shardFresh ? fmt("decoding %.0f%%", 100 * clampd(speechT / std::max(shardWorld.speech.seconds, 1e-9), 0, 1)).c_str() : "idle", speech.c_str(), piece.c_str());
}

bool TestGame::testShardsOpenIndex(int idx) {
    if (shardsLevel != 1 || !shardWorld.valid) return false;
    for (size_t i = 0; i < shardHeld.size(); i++) if (shardHeld[i] == idx) { openShard((int)i); return true; }
    return false;
}

std::string TestGame::testPadItems(int page) const {
    std::string s;
    for (const PadItem& it : padItems(page)) s += it.label + "|" + (it.key >= 0 ? keyLabel(it.key, it.mods) : std::string("*")) + "\n";
    return s;
}
std::string TestGame::testPadLegend() const { return padLegend(padContext(), 52); }

std::string TestGame::testScanInfo() {
    std::string s = fmt("scan mode %d skip %d scanned %d (%.0f ms), %zu echoes", scanMode, scanSkipMarked ? 1 : 0, surf.scanned ? 1 : 0, surf.scanMs, surf.echoes.size());
    int kinds[LM_COUNT] = {0};
    for (const auto& e : surf.echoes) if (e.lm.kind >= 0 && e.lm.kind < LM_COUNT) kinds[e.lm.kind]++;
    for (int k = 0; k < LM_COUNT; k++) if (kinds[k]) s += fmt(" %s %d", LANDMARK_KIND_NAMES[k], kinds[k]);
    double hx, hz;
    if (const SurfaceView::ScanEcho* e = scanNearest(hx, hz))
        s += fmt("; nearest at %.0f %.0f (%.0f m, %s), heard at %.0f %.0f", e->x, e->z, std::hypot(e->x - surf.player.x, e->z - surf.player.z), echoDetail(*e).c_str(), hx, hz);
    s += fmt("; %zu marks here", marksHere().size());
    return s;
}

bool TestGame::testEchoTrue(double& x, double& z, double& radius) {
    double hx, hz;
    const SurfaceView::ScanEcho* e = scanNearest(hx, hz);
    if (!e) return false;
    x = e->x; z = e->z; radius = e->lm.radiusM;
    return true;
}

std::string TestGame::testHistoryInfo() {
    ensureTold();
    int named = 0, persons = 0;
    std::string eras;
    if (shardWorld.history) {
        const History& H = *shardWorld.history;
        for (size_t k = 0; k < H.eras.size(); k++)
            eras += fmt("%s%s%s", k ? ", " : "", H.eras[k].name.c_str(), k < told.eraRead.size() && told.eraRead[k] > 0 ? fmt(" (%d read)", told.eraRead[k]).c_str() : " (none read)");
        std::set<int> ps;
        for (const ToldEvent& t : told.events) { const HistoryEvent& e = H.events[t.event]; for (int p : {e.a, e.b, e.c}) if (p >= 0) ps.insert(p); }
        persons = (int)ps.size(); for (int p : ps) if (toldPersonNamed(H, told, p)) named++;
    }
    int rows = 0; for (const LegendRow& r : legendRows) if (r.kind == 1) rows++;
    return fmt("history: page %d, %d records read (%d texts), %d events learnt in %d lines (built in %.1f ms), %d of %d persons in them named, the age %s (%s), %s; eras: %s", shardsPage, told.read,
            told.texts, (int)told.events.size(), rows, toldMs, named, persons, told.ageTold ? "told" : "not told", AGE_TOLD_NAMES[std::max(0, std::min(AGE_COUNT - 1, told.age))],
            told.ended ? "ended" : "not ended", eras.c_str());
}
std::vector<std::string> TestGame::testLegendLines() {
    ensureTold();
    std::vector<std::string> out;
    for (const LegendRow& r : legendRows) {
        if (r.kind == 2) { out.push_back("## " + r.text); continue; }
        if (r.kind == 0) {
            std::string h = "== ";
            for (const DecodedWord& w : r.words) h += w.pre + (w.name ? "{" + w.ours + "}" : (w.known ? w.ours : "~" + w.theirs)) + w.post + " ";
            out.push_back(h + r.text);
            continue;
        }
        std::string s = r.kind == 3 ? (r.first ? fmt("%6s  ", r.text.c_str()) : std::string(8, ' ')) : (r.first ? fmt("%6s  ", thousands(r.year).c_str()) : std::string(8, ' '));
        for (const DecodedWord& w : r.words) s += w.pre + (w.name ? "{" + w.ours + "}" : (w.known ? w.ours : "~" + w.theirs)) + w.post + " ";
        out.push_back(s);
    }
    return out;
}

double TestGame::testDroneRackX(int k) const { double x, lift, blur; return droneRackPlace(k, x, lift, blur) ? x : -99; }
double TestGame::testDroneRackY(int k) const { double x, lift, blur; return droneRackPlace(k, x, lift, blur) && k >= 0 && k < DRONE_COUNT ? SHELF_Y[k] + lift : -99; }

std::string TestGame::testChartInfo() const {
    std::string s = fmt("chart: level %d", shardsLevel);
    if (shardsLevel == 2 && shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].chart) {
        const StarChart& C = shardWorld.chart;
        s += fmt(", on the screen S%d (chart %d of the %s, %s): %zu stars, %zu figures, the mark %s at %lld %lld %lld (how %d, %.0f ly, %s), %s", shardHeld[shardSel], C.which,
                shardWorld.lore.people.c_str(), CHART_STYLE_NAMES[std::max(0, std::min(CHS_COUNT - 1, C.style))], C.stars.size(), C.figures.size(), STAR_CLASSES[C.mark.star.cls].code,
                (long long)C.mark.star.sx, (long long)C.mark.star.sy, (long long)C.mark.star.sz, C.mark.how, C.mark.ly, C.mark.people.empty() ? "no people" : ("the " + C.mark.people).c_str(),
                chartT >= 0 ? fmt("reading %.0f%%", 100 * clampd(chartT / CHART_READ_S, 0, 1)).c_str() : (shardFresh ? "unread" : "read"));
    }
    s += chartUp ? fmt("; held up: S%d of %s (%zu stars, %zu figures), %d lines drawn", chartHeldIndex, chartHeldKey.c_str(), chartHeld.stars.size(), chartHeld.figures.size(), chartLinesDrawn)
                 : "; none held up";
    return s;
}

bool TestGame::testAimAtChartMark(int figure) {
    if (!chartUp || !chartHeld.valid) return false;
    if (figure >= (int)chartHeld.figures.size()) return false;
    const Star& at = figure < 0 ? chartHeld.mark.star : chartHeld.stars[chartHeld.figures[figure].label].star;
    Vec3 fwd = normalize(at.pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
    cabin.yaw = 0; cabin.pitch = 0;
    return true;
}

std::string TestGame::testAlmanacInfo() const {
    std::string s = fmt("%s; %s; %zu events", almanacAt.ground ? "ground" : "ship", almanacStateLine().c_str(), almanacEvents.size());
    for (size_t i = 0; i < almanacEvents.size() && i < 12; i++) s += fmt("%s %s in %s", i ? "," : ":", almanacLabelOf(almanacEvents[i]).c_str(), countdownString(almanacEvents[i].t - t).c_str());
    if (almanacEvents.size() > 12) s += fmt(", +%zu more", almanacEvents.size() - 12);
    return s;
}

bool TestGame::testAlmanacSelect(int kind, int body) {
    for (size_t i = 0; i < almanacEvents.size(); i++)
        if ((kind < 0 || almanacEvents[i].kind == kind) && (body < 0 || almanacEvents[i].body == body)) { almanacSel = (int)i; return true; }
    return false;
}

std::string TestGame::testInstrumentInfo() const {
    if (!icase.valid) return fmt("the case: %d instruments, none on the screen", (int)icase.keys.size());
    const Timbre* tb = instrumentTimbre(icase.tradition, icase.inst);
    return fmt("the case: %d instruments; on the screen %s (id %d, people %d, age %s, voice %d, %s), %d courses, register %+d, %d notes played, the synth's timbre %s%s", (int)icase.keys.size(),
            instrumentName(icase.tradition, icase.inst).c_str(), icase.inst.id, icase.inst.people, AGE_NAMES[std::max(0, std::min(AGE_COUNT - 1, icase.inst.age))], icase.inst.voice,
            tb ? (tb->name + ", " + TIMBRE_FAMILY_NAMES[std::max(0, std::min(TF_COUNT - 1, tb->family))]).c_str() : "its disc", (int)icase.glow.size(), icase.inst.reg + icase.regShift, icase.notes,
            audio.instrument == tb && tb ? "set" : (audio.instrument ? "another's" : "none"), icase.discOn ? ", the disc turning" : "");
}

std::string TestGame::testFrontInfo() const {
    if (!surf.valid) return "no surface";
    const SurfaceEnvironment& e = surf.env;
    return fmt("forecast '%s'; line %s, ahead %s, cloud %.2f (pattern %.2f), rain %.2f, cold %.1f C, wind %.0f kt %s, temp %+.0f C, fog %.0f m",
               forecastLine().c_str(), surf.localFront.on ? "on" : "off", e.frontAhead < 1e17 ? fmt("%.1f km", e.frontAhead / 1000).c_str() : "none",
               e.frontCloud, e.cloudPattern, e.frontRain, e.frontCold, e.windKnots, compassName(e.windDir), e.temperatureC, e.fogDistance);
}

// R-404: the computers with a colour of their own and the smallest distance between two of them (the cage left out)
int TestGame::testScreenColours(double& minDist) const {
    minDist = 9; int n = 0;
    for (int i = 1; i <= 11; i++) {
        if (i == 6) continue;
        n++;
        for (int j = i + 1; j <= 11; j++) {
            if (j == 6) continue;
            const RGB &a = SCREENS[i].col, &b = SCREENS[j].col;
            minDist = std::min(minDist, (double)std::sqrt((a.r - b.r) * (a.r - b.r) + (a.g - b.g) * (a.g - b.g) + (a.b - b.b) * (a.b - b.b)));
        }
    }
    return n;
}
