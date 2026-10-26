#include "surface_view.h"
#include "surface_internal.h"
#include "core/clock.h"

namespace {
// rotate a local (x east, z north) vector by a heading change
inline double p_rot_x(double x, double z, double a) { return x * std::cos(a) + z * std::sin(a); }
inline double p_rot_z(double x, double z, double a) { return -x * std::sin(a) + z * std::cos(a); }
}

void SurfaceView::buildDirLUT() {
    dirLUT.resize(FBW * FBH);
    double invf = 1.0 / proj.f;
    parallelFor(FBH, 16, [&](int y0, int y1) {   // F-02: on the pool (the sprint's kick rebuilds it on six frames at its start and its stop, 2.3 ms each at 4x alone)
        for (int y = y0; y < y1; y++)
            for (int x = 0; x < FBW; x++) {
                Vec3 d((x + 0.5 - proj.cx) * invf, -(y + 0.5 - proj.cy) * invf, 1.0);
                dirLUT[y * FBW + x] = normalize(d);
            }
    });
}

void SurfaceView::setFov(double deg) {
    baseFov = deg;
    proj = Proj::fromHFov(deg);
    buildDirLUT();
}

// M7-01: cells 24-46 cells ahead within 55 degrees of the velocity, not yet cached, sampled on a worker thread
void SurfaceView::prefetchAhead() {
    if (aheadBusy.load(std::memory_order_acquire)) return;
    if (aheadThread.joinable()) {
        aheadThread.join();
        TerrainCache& c = *aheadTarget;
        for (size_t i = 0; i < aheadCells.size() && i < aheadOut.size(); i++) {
            int cx = aheadCells[i].first, cz = aheadCells[i].second;
            TerrainVertex& slot = c.v[c.index(cx, cz)];
            if (slot.cx == cx && slot.cz == cz) continue;   // the main thread got there first
            slot = aheadOut[i]; slot.cx = cx; slot.cz = cz;
            aheadStat++;
        }
        aheadCells.clear(); aheadOut.clear();
    }
    double vx = inBuggy ? std::sin(buggy.heading) * buggy.speed : player.vx, vz = inBuggy ? std::cos(buggy.heading) * buggy.speed : player.vz;
    double speed = std::sqrt(vx * vx + vz * vz);
    if (speed < 1.0) return;
    double dx = vx / speed, dz = vz / speed;
    // the near ring sweeps fastest, so it takes every other job: near, lod0, near, lod1 (lod0 while it is off)
    aheadCache = (aheadCache + 1) & 3;
    int which = (aheadCache & 1) == 0 ? 2 : (aheadCache == 1 ? 0 : 1);
    if (which == 2 && (nearBlend <= 0 || !ringN)) which = 0;
    TerrainCache& c = which == 2 ? site.lodN : (which == 0 ? site.lod0 : site.lod1);
    aheadTarget = &c;
    double cs = c.cellSize;
    int r0 = which == 2 ? 22 : (which == 0 ? 24 : 20), r1 = which == 2 ? 50 : (which == 0 ? 46 : 40);
    size_t cap = which == 2 ? 640 : 320;
    int pcx = (int)std::floor(player.x / cs), pcz = (int)std::floor(player.z / cs);
    aheadCells.clear();
    for (int rz = -r1; rz <= r1 && aheadCells.size() < cap; rz++)
        for (int rx = -r1; rx <= r1 && aheadCells.size() < cap; rx++) {
            double d = std::sqrt((double)(rx * rx + rz * rz));
            if (d < r0 || d > r1) continue;
            if ((rx * dx + rz * dz) / d < 0.57) continue;   // within 55 degrees of the heading
            int cx = pcx + rx, cz = pcz + rz;
            const TerrainVertex& slot = c.v[c.index(cx, cz)];
            if (slot.cx == cx && slot.cz == cz) continue;
            aheadCells.push_back({cx, cz});
        }
    if (aheadCells.empty()) return;
    aheadOut.assign(aheadCells.size(), TerrainVertex());
    aheadBusy.store(true, std::memory_order_release);
    const SurfaceSite* s = &site;
    std::vector<std::pair<int, int>>* cells = &aheadCells;
    std::vector<TerrainVertex>* out = &aheadOut;
    std::atomic<bool>* busy = &aheadBusy;
    aheadThread = std::thread([s, cells, out, busy, cs]() {
        for (size_t i = 0; i < cells->size(); i++) (*out)[i] = s->sampleAt((*cells)[i].first * cs, (*cells)[i].second * cs, cs);
        busy->store(false, std::memory_order_release);
    });
}

void SurfaceView::joinAhead() {
    if (aheadThread.joinable()) aheadThread.join();
    aheadBusy.store(false);
    aheadCells.clear(); aheadOut.clear();
}

void SurfaceView::init(const StarSystem* sys, int bodyIndex, double lat, double lon, double t) {
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;   // VESPERIS_TRACE=1: the sections' times on stderr
    auto tick = [&](const char* what) {
        static std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (trace) fprintf(stderr, "  init %-14s %.1f ms\n", what, std::chrono::duration<double, std::milli>(now - last).count());
        last = now;
    };
    tick("start");
    joinAhead();   // M7-01: a pending job refers to the old site
    site.init(sys, bodyIndex, lat, lon, t);
    site.clearRoads(); site.ensureRoads(0, 0); roadBuildX = 0; roadBuildZ = 0;   // the old roads round the site, before anything samples the ground
    site.clearMarks(); markCheckX = markCheckZ = 1e18;
    site.clearEdits(); workCells.clear(); workNear.clear(); workEdited.clear();
    worksHere = worldHadCivilisation(site.gen) ? worldWorksIfReady(site.gen) : nullptr;   // the world's works when built (never waited for)
    tick("site");
    grain.generate(site.gen.seed ^ 0x77, 9, 1);
    grainFine.generate(site.gen.seed ^ 0x99, 4, 0);
    grainEdge.generate(site.gen.seed ^ 0x5E, 64, 1);   // the wobble of material boundaries
    buildMacroTile(grainMacro, site.gen.seed);          // slow tone patches within a material
    buildLeafTiles(leafTile, leafEdge, site.gen.seed);   // the vegetation's tiles and looks
    buildBarkTile(barkTile, site.gen.seed);
    buildLooks();
    buildDirLUT();
    tick("tiles+looks");
    vc0.assign((size_t)1 << 14, VtxCache());
    vc1.assign((size_t)1 << 14, VtxCache());
    vc2.assign((size_t)1 << 12, VtxCache());
    vc3.assign((size_t)1 << 12, VtxCache());
    vcN.assign((size_t)1 << 16, VtxCache());
    computeRings();
    drainagePrefetch(site.gen, site.up0, drawRadiusM() + 3000.0, false);   // the drainage tiles the rings will read, before the caches fill
    tick("drainage");
    nearBlend = ringN ? 1.0 : 0.0;
    lastRange = -1;
    weatherRng = Rng(site.gen.seed ^ (uint64_t)(lat * 1000) ^ (uint64_t)(lon * 7000));
    // capsule at the origin, the explorer beside it, facing away
    capsuleX = 0; capsuleZ = 0;
    capsuleY = site.surfaceHeight(0, 0);
    player = Player();
    player.x = 6; player.z = -4;
    player.y = site.surfaceHeight(player.x, player.z);
    player.yaw = 0.3; player.pitch = 0.0;
    player.onGround = true;
    buggy = Buggy(); inBuggy = false; chaseCam = false;   // no buggy carries over to a new landing
    drone = Drone(); inDrone = false;                      // nor a drone
    lastWindUpdate = -1; lastEnvT = -1;
    prefetchStage = 0; prefetchRing = 0;
    for (int m = 0; m < MAT_COUNT; m++) mesoBuilt[m] = false;
    footprints.clear(); footHead = 0; stepAccum = 0; lastStepX = 6; lastStepZ = -4;
    // C-01; C-13: the world's peoples, each with a culture and a lore (B-420: the footings' meshes with them)
    ruinCells.clear(); supportMeshes.clear(); supports.clear(); peoples = peoplesOf(site.gen) > 1 ? 2 : 1;
    for (int k = 0; k < 2; k++) {
        cultures[k] = k < peoples ? cultureOf(site.gen, k) : Culture();
        lores[k] = worldHadCivilisation(site.gen) && k < peoples ? loreQuick(*site.sys, site.sys->bodies[site.body], site.gen, k) : Lore();   // for the graves' names and years
        palettes[k] = cultures[k].palette;   // its colours and windows
        traditions[k] = worldHadCivilisation(site.gen) && k < peoples ? traditionOf(site.gen, lores[k]) : Tradition();   // its music, its instruments
        if (worldHadCivilisation(site.gen) && k < peoples) instrumentsOf(traditions[k], lores[k].which, instrumentSets[k]); else instrumentSets[k] = PeopleInstruments();
    }
    placedCells.clear();   // (tests)
    refreshPlanNear();     // the plan's places round the landing (the plan was built at the approach)
    ensureTownMarks();     // the towns' streets and fields in the ground, before anything samples it
    if (!site.edits.empty()) { capsuleY = site.surfaceHeight(capsuleX, capsuleZ); player.y = site.surfaceHeight(player.x, player.z); }   // on the works' ground
    nearGrave = NearGrave();
    shardsFound.clear(); nearShard = NearShard();   // (the game refills the set from the guide after this)
    instrumentsFound.clear(); nearInstrument = NearInstrument();   // (likewise)
    hasWaypoint = false; eruptions.clear(); nextEruption = 30; devils.clear(); nextDevil = 20; quake = 0; nextQuake = 45; siteEpoch++;   // nothing of the last landing shows on this one's map
    quakeCue = 0; duneSong = 0; iceSong = 0; duneHz = duneSongHz(site.gen, site.gravity);
    tick("capsule+misc");
    joinScan(); echoes.clear(); scanned = false;   // the last site's echoes are gone; the scanner asks again
    tick("scan");
    spawnLife(lat, lon);   // herds and flocks from the world's bestiary
    tick("life");
    buildBandMap();
    meteorShowersOf(*sys, bodyIndex, showers);
    zodiDust = zodiacalDust(*sys, bodyIndex);
    lavaGlow = isLavaWorld(site.gen.type) || (site.gen.dark && site.gen.hasTrait(TR_EMBERS));
    waterLights = site.gen.hasTrait(TR_LIGHTS_IN_WATER) && site.hasWater;
    computeEnvironment(t);
    tick("band+env");
    valid = true;
}

// O1/O4: how many cells each terrain ring draws: 40 x 16 m, 36 x 64 m, 26 x 512 m and 26 x 2048 m (53 km) on a planet;
// on a small body the rings stop at 1.15 radii (66 degrees round the sphere, past its own horizon) and a ring that would
// be under four cells is dropped
void SurfaceView::computeRings() {
    const int base[4] = {40, 36, 26, 26}, cells[4] = {16, 64, 512, 2048};
    for (int k = 0; k < 4; k++) {
        int r = std::min(base[k], (int)std::floor(1.15 * site.R / cells[k]));
        ringR[k] = r < 4 ? 0 : r;
    }
    ringN = std::min(30, (int)std::floor(1.15 * site.R / 4));   // 30 cells of 4 m
    if (ringN < 4) ringN = 0;
    invTwoR = 0.5 / site.R;
}

// O6-02: the near ring follows the explorer (or the buggy); B-313: at every speed
void SurfaceView::updateNearRing(double dt) {
    (void)dt;
    // the near ring is on at every speed (the buggy's suspension soaks up its relief, updateBuggy): faded out above
    // 12 m/s and back in as the buggy slowed, the 4 m relief rising over half a second as the explorer stops and gets out
    // would read as the hills changing shape
    nearBlend = (ringN && cameraOverrideAlt < 60) ? 1.0 : 0.0;
    double cx = inBuggy ? buggy.x : player.x, cz = inBuggy ? buggy.z : player.z;
    site.nearBlend = nearBlend; site.nearX = cx; site.nearZ = cz; site.nearR = ringN;
}

void SurfaceView::relocateCapsule(double x, double z) {
    capsuleX = x; capsuleZ = z;
    capsuleY = site.surfaceHeight(x, z);
}

// M8-07: a dotted line on the ground toward the waypoint (or the capsule when far from it).
void SurfaceView::drawWaypointLine(Framebuffer& fb) {
    double tx, tz;
    if (hasWaypoint) { tx = wpX; tz = wpZ; }
    else { tx = capsuleX; tz = capsuleZ; if (env.capsuleDist < 40) return; }
    double dx = tx - player.x, dz = tz - player.z;
    double d = std::sqrt(dx * dx + dz * dz);
    if (d < 5) return;
    dx /= d; dz /= d;
    for (int i = 1; i <= 12; i++) {
        double s = i * 2.5;
        if (s > d) break;
        double x = player.x + dx * s, z = player.z + dz * s;
        Vec3 v = toView(x, site.groundHeight(x, z) + 0.15, z);
        if (v.z < NEAR_Z) continue;
        RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = 52 - i * 1.5;
        rasterPoint3(fb, q, 1, proj, true, 1);
    }
}

bool SurfaceView::projectPoint(double x, double y, double z, double& sx, double& sy) const {
    Vec3 v = toView(x, y, z);
    if (v.z < NEAR_Z) return false;
    sx = proj.cx + proj.f * v.x / v.z; sy = proj.cy - proj.f * v.y / v.z;
    return true;
}

void SurfaceView::prefetch(double budgetMs) {
    using namespace std::chrono;
    auto t0 = steady_clock::now();
    // coarse to fine: lod3 to 26 cells, lod2 to 26, lod1 to 36, lod0 to 40 (the render radii, computeRings)
    const int radii[5] = {ringR[3], ringR[2], ringR[1], ringR[0], ringN};
    TerrainCache* caches[5] = {&site.lod3, &site.lod2, &site.lod1, &site.lod0, &site.lodN};   // O6-02: the near ring last
    while (prefetchStage < 5) {
        TerrainCache& c = *caches[prefetchStage];
        int R = radii[prefetchStage];
        for (; prefetchRing <= R; prefetchRing++) {
            int r = prefetchRing;
            if (r == 0) c.at(0, 0);
            else {
                for (int i = -r; i <= r; i++) { c.at(i, -r); c.at(i, r); }
                for (int i = -r + 1; i < r; i++) { c.at(-r, i); c.at(r, i); }
            }
            double ms = duration_cast<duration<double, std::milli>>(steady_clock::now() - t0).count();
            if (ms > budgetMs) { prefetchRing++; return; }
        }
        prefetchStage++;
        prefetchRing = 0;
    }
}

void SurfaceView::reanchor() {
    const auto ta0 = std::chrono::steady_clock::now();   // F-01: the parts' ms under VESPERIS_TRACE (KI-302: a comet's every 0.35 radii)
    joinAhead();   // cell coordinates change with the anchor
    double lat, lon;
    site.latLonAt(player.x, player.z, lat, lon);
    Vec3 capU = site.unitAt(capsuleX, capsuleZ);
    Vec3 bugU = site.unitAt(buggy.x, buggy.z);
    Vec3 drnU = site.unitAt(drone.x, drone.z);
    Vec3 wpU = site.unitAt(wpX, wpZ);   // the waypoint moves with the frame too
    Vec3 oldNorth = site.north0;
    const StarSystem* sys = site.sys;
    int body = site.body;
    const auto ta1 = std::chrono::steady_clock::now();
    site.init(sys, body, lat, lon, lastT);
    const auto ta2 = std::chrono::steady_clock::now();
    site.clearRoads(); site.ensureRoads(0, 0); roadBuildX = 0; roadBuildZ = 0;   // the roads in the new frame's metres
    const auto ta3 = std::chrono::steady_clock::now();
    site.clearMarks(); markCheckX = markCheckZ = 1e18;   // added again from the cells in the new frame
    site.clearEdits(); workCells.clear(); workEdited.clear();   // the works too
    double cx, cz;
    site.localAt(capU, cx, cz);
    // headings are measured from local north, which turns with the meridians
    double dyaw = std::atan2(dot(oldNorth, site.east0), dot(oldNorth, site.north0));
    player.yaw = wrap2pi(player.yaw + dyaw);
    double vx = p_rot_x(player.vx, player.vz, dyaw), vz = p_rot_z(player.vx, player.vz, dyaw);
    player.vx = vx; player.vz = vz;
    player.x = 0; player.z = 0;
    capsuleX = cx; capsuleZ = cz;
    capsuleY = site.surfaceHeight(cx, cz);
    site.localAt(wpU, wpX, wpZ);
    siteEpoch++;
    // the cells' local metres moved with the frame (A-02: the harness's placed ones too; B-420: the footings' meshes)
    ruinCells.clear(); placedCells.clear(); supportMeshes.clear(); supports.clear();
    refreshPlanNear();   // the plan's places in the new frame's metres
    ensureTownMarks();
    if (buggy.deployed) {   // the buggy moves with the frame too (B-205); a driven buggy is where the player is
        if (inBuggy) { buggy.x = 0; buggy.z = 0; }
        else { double bx, bz; site.localAt(bugU, bx, bz); buggy.x = bx; buggy.z = bz; }
        buggy.heading = wrap2pi(buggy.heading + dyaw);
        buggy.y = site.surfaceHeight(buggy.x, buggy.z);
        buggy.groundY = -1e9;   // the contact height is in the old frame
        buggy.tracks.clear(); buggy.trackHead = 0; buggy.puffs.clear();
    }
    if (drone.deployed) {   // the drone too; a flown drone is where the player is, at its height
        if (inDrone) { drone.x = 0; drone.z = 0; }
        else { double dx, dz; site.localAt(drnU, dx, dz); drone.x = dx; drone.z = dz; drone.y = site.surfaceHeight(dx, dz); }
        drone.heading = wrap2pi(drone.heading + dyaw);
        drone.puffs.clear();
    }
    double ground = site.surfaceHeight(0, 0);
    if (player.y < ground) player.y = ground;
    vc0.assign((size_t)1 << 14, VtxCache());
    vc1.assign((size_t)1 << 14, VtxCache());
    vc2.assign((size_t)1 << 12, VtxCache());
    vc3.assign((size_t)1 << 12, VtxCache());
    vcN.assign((size_t)1 << 16, VtxCache());
    computeRings();
    const auto ta4 = std::chrono::steady_clock::now();
    drainagePrefetch(site.gen, site.up0, drawRadiusM() + 3000.0, false);
    const auto ta5 = std::chrono::steady_clock::now();
    for (ScanEcho& e : echoes) site.localAt(e.lm.unit, e.x, e.z);   // the same echoes, in the new frame (a scan under way is taken in it)
    flocks.clear(); critters.clear(); herds.clear(); habitatT = -1e9;   // they lived near the old origin, 50 km away (B-316: the habitat cells respawn theirs)
    prefetchStage = 0; prefetchRing = 0;
    anchorFrame = frameStamp;   // the next frames' warm lines are traced (the rings' refill)
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
    if (trace) {
        auto ms = [](std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
        fprintf(stderr, "  reanchor: %.1f ms (the worker's join %.1f, the site's init %.1f, roads %.1f, marks, plan and the vehicles %.1f, the tiles asked %.1f)\n", ms(ta0, ta5), ms(ta0, ta1),
                ms(ta1, ta2), ms(ta2, ta3), ms(ta3, ta4), ms(ta4, ta5));
    }
}

// R-408: the scanner's search on a thread of its own (a scan of 40 km reads a few dozen cells of 14 km, a few milliseconds each and
// cached, the lakes' drainage tiles built as needed, and a thousand cells of the ruins' grid): the HUD and the map read the echoes
// once `collectScan` has them; a new landing clears them, a re-anchor moves them into the new frame
void SurfaceView::startScan() {
    joinScan();
    if (!site.sys) return;
    BodyGen g = site.gen;
    Vec3 at = site.unitAt(player.x, player.z);
    scanU = at;
    std::vector<Landmark>* out = &scanOut; std::atomic<bool>* busy = &scanBusy; double* ms = &scanOutMs;
    scanOut.clear();
    scanBusy.store(true);
    scanThread = std::thread([g, at, out, busy, ms]() {
        auto t0 = std::chrono::steady_clock::now();
        std::vector<Landmark> cells, ruins;
        sightsNear(g, at, SCAN_RANGE, sightKindsOf(g) & ~(1u << LM_RUIN) & ~(1u << LM_WORK), cells);
        ruinsNear(g, at, SCAN_RANGE, ruins);
        std::vector<Landmark> works; worksHeardNear(g, at, SCAN_RANGE, works);
        *out = cells; out->insert(out->end(), ruins.begin(), ruins.end()); out->insert(out->end(), works.begin(), works.end());
        *ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        busy->store(false, std::memory_order_release);
    });
}
void SurfaceView::collectScan() {
    if (!scanThread.joinable() || scanBusy.load(std::memory_order_acquire)) return;
    scanThread.join();
    echoes.clear();
    for (const Landmark& L : scanOut) { ScanEcho e; e.lm = L; site.localAt(L.unit, e.x, e.z); echoes.push_back(e); }
    scanOut.clear(); scanned = true; scanMs = scanOutMs;
}
void SurfaceView::joinScan() {
    if (scanThread.joinable()) scanThread.join();
    scanBusy.store(false); scanOut.clear();
}

// M6-05: the free camera starts where the eye is and flies with W A S D (Space up, C down, Shift fast)
void SurfaceView::enterFreeCam() {
    freeCam = true;
    freePos = camPos;
    freeYaw = inVehicle() && chaseCam ? (inBuggy ? buggy.heading : drone.heading) : player.yaw;
    freePitch = inVehicle() && chaseCam ? -12 * DEG : player.pitch;
}

void SurfaceView::updateFreeCam(double dt, const Input& in) {
    if (!freeCam) return;
    freeYaw += in.mouseDx * 0.0032 * mouseSens;
    freePitch -= in.mouseDy * 0.0032 * mouseSens * (invertY ? -1 : 1);
    freePitch = clampd(freePitch, -88 * DEG, 88 * DEG);
    double fwd = (in.isDown(KEY_W) || in.isDown(KEY_UP) ? 1 : 0) - (in.isDown(KEY_S) || in.isDown(KEY_DOWN) ? 1 : 0);
    double str = (in.isDown(KEY_D) ? 1 : 0) - (in.isDown(KEY_A) ? 1 : 0);
    if (in.moveY != 0) fwd = in.moveY;   // R-418: each axis of a stick on its own (the keys keep the other)
    if (in.moveX != 0) str = in.moveX;
    double up = (in.isDown(KEY_SPACE) ? 1 : 0) - (in.isDown(KEY_C) ? 1 : 0);
    if (in.ctrl()) { fwd = 0; str = 0; up = 0; }
    double speed = in.shift() ? 40.0 : 8.0;
    Vec3 f(std::sin(freeYaw) * std::cos(freePitch), std::sin(freePitch), std::cos(freeYaw) * std::cos(freePitch));
    Vec3 r(std::cos(freeYaw), 0, -std::sin(freeYaw));
    freePos = freePos + (f * fwd + r * str + Vec3(0, up, 0)) * (speed * dt);
    double floor = site.surfaceHeight(freePos.x, freePos.z) + 0.4;
    if (freePos.y < floor) freePos.y = floor;
    if (freePos.y > floor + 400) freePos.y = floor + 400;
    computeEnvironment(lastT);
}

void SurfaceView::update(double dt, const Input& in, double t, bool controlsEnabled) {
    if (!valid) return;
    if (dt > 0.1) dt = 0.1;
    updateNearRing(dt);   // O6-02: before the movement, so the ground underfoot is the ground drawn
    waterRecords();
    if (inBuggy) updateBuggy(dt, in, t);
    else if (inDrone) updateDrone(dt, in, t);
    else updateWalking(dt, in, t, controlsEnabled);
    prefetchAhead();
    collectScan();
    if (scanWanted && !scanBusy.load(std::memory_order_acquire)) {   // the scanner on: a first scan, and another every SCAN_STEP
        double sx = 0, sz = 0; if (scanned) site.localAt(scanU, sx, sz);
        if (!scanned || std::hypot(player.x - sx, player.z - sz) > SCAN_STEP) startScan();
    }
    // buggy dust settles, an unfolding buggy keeps unfolding
    for (auto& p : buggy.puffs) p.age += (float)dt;
    buggy.puffs.erase(std::remove_if(buggy.puffs.begin(), buggy.puffs.end(), [](const Buggy::Puff& p) { return p.age > 1.5f; }), buggy.puffs.end());
    if (buggy.deployed && buggy.unfold < 1) buggy.unfold = std::min(1.0, buggy.unfold + dt / 2.0);
    if (!inBuggy && buggy.deployed) {
        buggy.speed *= std::exp(-dt * 2.0);
        buggy.y = site.surfaceHeight(buggy.x, buggy.z);
    }
    // the drone's downwash settles, an unfolding drone keeps unfolding, a parked one sits on the ground with its pods winding down
    for (auto& p : drone.puffs) p.age += (float)dt;
    drone.puffs.erase(std::remove_if(drone.puffs.begin(), drone.puffs.end(), [](const Buggy::Puff& p) { return p.age > 1.5f; }), drone.puffs.end());
    if (drone.deployed && drone.unfold < 1) drone.unfold = std::min(1.0, drone.unfold + dt / 2.0);
    if (!inDrone && drone.deployed) {
        drone.y = site.surfaceHeight(drone.x, drone.z); drone.landed = true; drone.speed = 0; drone.vy = 0;
        drone.rotor *= std::exp(-dt * 0.8); drone.fanSpin += dt * TAU * (2.0 + 22.0 * drone.rotor);
        drone.vibration *= std::exp(-dt * 4); drone.jolt *= std::exp(-dt * 5);
    }
    {   // keep the local frame close: 50 km on a planet, a third of the radius on a small body
        double reach = std::min(50e3, 0.35 * site.R);
        if (player.x * player.x + player.z * player.z > reach * reach) reanchor();
    }
    if (!worksHere && worldHadCivilisation(site.gen) && (worksHere = worldWorksIfReady(site.gen))) { refreshPlanNear(); markCheckX = markCheckZ = 1e18; }   // the works come in when built
    if (worldHadCivilisation(site.gen) && std::hypot(player.x - planNearX, player.z - planNearZ) > 1000) refreshPlanNear();
    if (worldHadCivilisation(site.gen) && std::hypot(player.x - markCheckX, player.z - markCheckZ) > 250) ensureTownMarks();   // the towns coming into reach
    if (!site.roadCells.empty()) {   // the roads grow with the explorer: two cells from the last build, the next block (the worker joined first: the vertices read the list)
        double dx = player.x - roadBuildX, dz = player.z - roadBuildZ, two = 2 * ruinCellLat(site.gen) * site.R;
        if (dx * dx + dz * dz > two * two) {
            const auto tr0 = std::chrono::steady_clock::now();   // a rebuild's ms on the main thread (the worker's join with it)
            joinAhead(); site.ensureRoads(player.x, player.z); roadBuildX = player.x; roadBuildZ = player.z;
            lastRoadsMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tr0).count(); roadsBuilds++;
            static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
            if (trace) fprintf(stderr, "  roads grown %d: %.1f ms (%zu roads held)\n", roadsBuilds, lastRoadsMs, site.roads.size());
        }
    }
    if (t - drainCheckT > 3.0) {   // the drainage tiles the far ring will reach next, computed ahead on their own thread
        drainCheckT = t;
        Vec3 u = site.unitAt(player.x, player.z);
        if (!drainageReady(site.gen, u, drawRadiusM() + 9000.0)) drainagePrefetch(site.gen, u, drawRadiusM() + 9000.0, true);
    }
    // eruptions every minute or so on molten and volcanic worlds, within 400 m. R-307: a tectonic world's fountains
    // rise from its fissures (the nearest lava cell of the 16 m ring within 600 m) every 15-40 s and last 25 s; a bombarded
    // world takes a meteorite every 20-60 s within 1.2 km, and shakes for a moment
    const uint32_t events = KINDS[site.gen.type].events;   // the kind's events
    if (events & (EV_ERUPTIONS | EV_FOUNTAINS | EV_STRIKES)) {
        nextEruption -= dt;
        if (nextEruption <= 0) {
            Rng er((uint64_t)(t * 100));
            if (events & EV_FOUNTAINS) {
                double bx = 0, bz = 0; bool found = false;
                for (int tries = 0; tries < 40 && !found; tries++) {
                    double x = player.x + er.sym(600), z = player.z + er.sym(600);
                    if (site.lod0.at((int)std::floor(x / 16), (int)std::floor(z / 16)).material == MAT_LAVA) { bx = x; bz = z; found = true; }
                }
                if (found) { eruptions.push_back({bx, bz, t, 1}); eruptionCue = 1.0; }
                nextEruption = 15 + 25 * er.uni();
            } else if (events & EV_STRIKES) {
                eruptions.push_back({player.x + er.sym(1200), player.z + er.sym(1200), t, 2});
                nextEruption = 20 + 40 * er.uni();
                eruptionCue = 1.0; quake = std::max(quake, 0.5);
            } else {
                eruptions.push_back({player.x + er.sym(400), player.z + er.sym(400), t, 0});
                nextEruption = 40 + 50 * er.uni();
                eruptionCue = 1.0;
            }
        }
        eruptions.erase(std::remove_if(eruptions.begin(), eruptions.end(), [t](const Eruption& e) { return t - e.start > (e.kind == 1 ? 30 : 9); }), eruptions.end());
    }
    // the ground shakes on a tectonic world: a quake every 60-180 s, ten seconds long, the camera trembling by it. Q-03: on a world with
    // QUAKES every one and a half to four minutes; the quake's own sound at its onset (the tectonic world's thunder before)
    if ((events & EV_QUAKES) || site.gen.hasTrait(TR_QUAKES)) {
        nextQuake -= dt;
        if (nextQuake <= 0) { Rng qr((uint64_t)(t * 100) ^ 0x9A4E); quake = 0.6 + 0.4 * qr.uni(); nextQuake = (events & EV_QUAKES) ? 60 + 120 * qr.uni() : 90 + 150 * qr.uni(); quakeCue = quake; }
    }
    quake *= std::exp(-dt / 4.0);
    if (quake < 0.01) quake = 0;
    updateGroundSongs(dt, t);
    // dust devils on a desert world (and in a thin-air dust storm): up to four within 500 m, born every 20-50 s, each
    // walking with the wind for two to five minutes
    if (((events & EV_DEVILS) || ((events & EV_STORM_DEVILS) && env.dust > 0.3)) && env.skyBrightness > 0.3) {
        nextDevil -= dt;
        if (nextDevil <= 0 && devils.size() < 4) {
            Rng dr((uint64_t)(t * 100) ^ 0xD3B1);
            DustDevil d;
            d.x = player.x + dr.sym(500);
            d.z = player.z + dr.sym(500);
            d.height = 30 + 80 * dr.uni();
            d.radius = 5 + 12 * dr.uni();
            d.age = 0;
            d.life = 120 + 180 * dr.uni();
            d.phase = dr.range(0, TAU);
            devils.push_back(d);
            nextDevil = 20 + 30 * dr.uni();
        }
    }
    for (DustDevil& d : devils) {
        d.age += dt;
        double sp = 1.5 + 0.06 * env.windKnots;
        d.x += std::sin(env.windDir) * sp * dt + std::sin(t * 0.7 + d.phase) * 0.6 * dt;
        d.z += std::cos(env.windDir) * sp * dt + std::cos(t * 0.5 + d.phase) * 0.6 * dt;
    }
    devils.erase(std::remove_if(devils.begin(), devils.end(),
                         [&](const DustDevil& d) {
                             double dx = d.x - player.x, dz = d.z - player.z;
                             return d.age > d.life || dx * dx + dz * dz > 1500.0 * 1500.0;
                         }),
            devils.end());
    updateLife(dt, t);
    computeEnvironment(t);
}

void SurfaceView::render(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr, double fade) {
    if (!valid) return;
    frameStamp++;
    double eyeY = player.y + player.eyeHeight + player.bobY - player.landDip;
    double pitch = player.pitch, yaw = player.yaw, roll = 0;
    double camX = player.x, camZ = player.z;
    if (cameraOverrideAlt >= 0) {
        eyeY = site.surfaceHeight(player.x, player.z) + std::max(cameraOverrideAlt, player.eyeHeight);   // B-422: the descent ends at the eye, not in the ground
        pitch = cameraOverridePitch;
    } else if (inBuggy) {
        double hx = std::sin(buggy.heading), hz = std::cos(buggy.heading);
        if (chaseCam) {
            camX = buggy.x - hx * 4.5; camZ = buggy.z - hz * 4.5;
            eyeY = std::max(buggy.y + 2.2, site.groundHeight(camX, camZ) + 1.0);
            yaw = buggy.heading;   // the chase camera follows the heading
            pitch = -12 * DEG;
        } else {
            // the eye is the camera pod on the nose, bolted to the hull: it pans within the pod's range, takes the
            // chassis' pitch and roll, and trembles with the speed and the ground as a smooth sum of sines (at most
            // 1.5 cm and 0.4 deg; B-203: the old per-frame random offsets were the shaking), dipping after a thump
            Vec3 mount = buggyCameraMount();
            double look = clampd(wrapAngle(player.yaw - buggy.heading), -CAM_PAN, CAM_PAN);
            double amp = clampd(buggy.vibration, 0, 3) * 0.005;
            double tremor = amp * (std::sin(t * TAU * 7.3) + 0.5 * std::sin(t * TAU * 11.9));
            camX = mount.x; camZ = mount.z;
            eyeY = mount.y + tremor - buggy.jolt * 0.05;
            yaw = wrap2pi(buggy.heading + look);
            pitch = player.pitch + buggy.pitch * std::cos(look) - buggy.roll * std::sin(look) + amp * 0.45 * std::sin(t * TAU * 9.1) + buggy.jolt * 0.03;
            roll = buggy.roll * std::cos(look);
        }
    } else if (inDrone) {
        double hx = std::sin(drone.heading), hz = std::cos(drone.heading);
        if (chaseCam) {
            camX = drone.x - hx * 7.0; camZ = drone.z - hz * 7.0;
            eyeY = std::max(drone.y + 2.8, site.groundHeight(camX, camZ) + 1.0);
            yaw = drone.heading;
            pitch = -14 * DEG;
        } else {
            // the nose camera on a gimbal: it pans within the pod's range and tilts, the hull's bank and nose stay out of the
            // picture; the pods' buzz is a faster, smaller tremor than the buggy's road, and a thump dips it
            Vec3 mount = droneCameraMount();
            double look = clampd(wrapAngle(player.yaw - drone.heading), -CAM_PAN, CAM_PAN);
            double amp = clampd(drone.vibration, 0, 3) * 0.003;
            double tremor = amp * (std::sin(t * TAU * 23.0) + 0.5 * std::sin(t * TAU * 31.7));
            camX = mount.x; camZ = mount.z;
            eyeY = mount.y + tremor - drone.jolt * 0.05;
            yaw = wrap2pi(drone.heading + look);
            pitch = player.pitch + amp * 0.4 * std::sin(t * TAU * 19.3) + drone.jolt * 0.03;
            roll = 0;
        }
    } else {
        // head sway: a small lateral offset along the right vector
        camX += std::cos(player.yaw) * player.bobX; camZ -= std::sin(player.yaw) * player.bobX;
    }
    if (freeCam) { camX = freePos.x; camZ = freePos.z; eyeY = freePos.y; yaw = freeYaw; pitch = freePitch; roll = 0; }
    if (quake > 0 && cameraOverrideAlt < 0) {   // the ground's shaking, a slow heave with a fine tremor on it
        double q = quake * (0.4 + 0.6 * std::sin(t * 1.3));
        pitch += q * (1.1 * DEG * std::sin(t * 9.7) + 0.3 * DEG * std::sin(t * 23.1));
        roll += q * 0.8 * DEG * std::sin(t * 7.3 + 1.0);
        eyeY += q * 0.1 * std::sin(t * 11.0);
    }
    camPos = Vec3(camX, eyeY, camZ);
    camLocal = cameraBasis(yaw, pitch);
    {   // the picture follows the eye, not the body: under the drawn water surface (a dive's first frames, the eye
        // within a few centimetres of the surface, the free camera) everything is seen through water
        double wlEye = site.hasWater || site.gen.liquidLevel > -1e8 ? site.waterNowAt(camX, camZ) : -1e9;
        viewUnderwater = wlEye > -1e8 && eyeY < wlEye + 0.03;
        if (viewUnderwater) env.fogDistance = std::min(env.fogDistance, 22.0);
    }
    if (env.fogSea > 0) {   // an eye inside the fog sea sees a few hundred metres
        const double in = smoothstep(env.fogSeaTop + 6.0, env.fogSeaTop - 14.0, eyeY);
        if (in > 0) env.fogDistance = std::min(env.fogDistance, env.fogDistance + (240.0 - env.fogDistance) * in);
    }
    if (std::fabs(roll) > 1e-4) camLocal = Mat3::axisAngle(Vec3(0, 0, 1), roll) * camLocal;
    // sprint FOV kick (M8-02): rebuild the sky table only when the change is noticeable
    {
        double wantF = (UW * 0.5 * FB_SCALE) / std::tan((baseFov + player.fovKick) * 0.5 * DEG);
        if (std::fabs(wantF - proj.f) > proj.f * 0.012) { proj = Proj::fromHFov(baseFov + player.fovKick); buildDirLUT(); }
    }
    lastT = t;
    waterWdx = std::sin(env.windDir), waterWdz = std::cos(env.windDir);   // the water corners' (waterCorner)
    waterSunUp = smoothstep(-0.03, 0.06, env.sun.dirLocal.y);
    setupLamps();
    {
        SurfaceLook L = lookFor(site.gen, site.sys->star);
        cloudShadows = site.atmosphere && L.cloudsLayer && (env.cloudCover > 0.03 || localFront.on) && !hasOpaqueDeck(site.gen.type);   // a front's deck in reach counts
        cloudThr = 1.0 - env.cloudCover * 0.75;
        cloudWx = windDriftX; cloudWz = windDriftZ;
    }
    double tSec = nowMs();
    auto section = [&](int k) { double n = nowMs(); lastRenderMs[k] = n - tSec; tSec = n; };
    setupPalette(fb);
    if (fade < 1) scalePalette(fb.pal, fade);
    fb.clearDepth();
    floorH = floorHeight();   // the sky's galactic band reaches down to its horizon
    drawSky(fb, t, stars, sr);
    section(0);
    int r0 = ringR[0], r1 = ringR[1], r2 = ringR[2], r3 = ringR[3];
    int rN = nearBlend > 0 ? ringN : 0;   // the near ring (B-313: at every speed, on foot and in the buggy; off when the capsule is high)
    if (cameraOverrideAlt > 800) r0 = 0;          // high above the site the 16 m cells are invisible
    else if (cameraOverrideAlt > 300) r0 = std::min(r0, 16);
    {   // the rings near to far, each thread drawing every ring within its band of rows (the vertex caches and the
        // tiles are filled first, serially, so the threads only read; the pixels come out as the serial loop's)
        struct Ring { TerrainCache* c; std::vector<VtxCache>* vc; int r; double cover; };
        // a coarser ring skips the cells the finer one covers, up to a cell of overlap (two or three cells would
        // draw 4 km of both rings at the far edge); the finer ring's edge is morphed onto the coarser surface.
        // the cover is a radius in metres, tested against every corner of a coarse cell (`cellCovered`): a finer ring
        // of rf cells draws the cells whose centre lies within rf, so any point within rf - 0.71 cells lies in one of them
        auto coverOf = [](int rf, double csf) { return rf > 0 ? (rf - 0.71) * csf : 0.0; };
        Ring rings[5] = {{&site.lodN, &vcN, rN, 0}, {&site.lod0, &vc0, r0, coverOf(rN, 4)}, {&site.lod1, &vc1, r1, coverOf(r0, 16)},
                         {&site.lod2, &vc2, r2, coverOf(r1, 64)}, {&site.lod3, &vc3, r3, coverOf(r2, 512)}};   // the far ring to 53 km
        // the morph band of a ring reaches from the finer ring's edge (plus two cells) to its own edge: the gap between
        // two rings is two octaves of the relief (hundreds of metres at the 13 km edge, KI-310), and a narrow band would
        // twist the cells; the near ring keeps a full-detail core of ten cells (40 m) round the camera
        site.lod0R = 0; site.lod0Band = 0; lastWarmMisses = 0;
        for (int k = 4; k >= 0; k--) {   // coarse to fine: a ring morphs onto the next coarser one that is drawn
            Ring& g = rings[k];
            if (g.r <= 0) continue;
            Ring* coarse = nullptr;
            for (int j = k + 1; j < 5 && !coarse; j++) if (rings[j].r > 0) coarse = &rings[j];
            double cover = k == 0 ? 10.0 : (rings[k - 1].r > 0 ? rings[k - 1].r * (double)rings[k - 1].c->cellSize / g.c->cellSize : 10.0);
            int band = std::max(4, (int)std::floor(g.r - cover - 2));
            if (k == 0) { site.nearBand = band; }
            if (k == 1) { site.lod0R = g.r; site.lod0Band = coarse ? band : 0; site.camX = camPos.x; site.camZ = camPos.z; }   // groundHeight follows lod0's morph
            warmTerrainLOD(*g.c, *g.vc, g.r, g.cover, coarse ? coarse->c : nullptr, coarse ? coarse->vc : nullptr, band);
        }
        morphCoarse = nullptr; morphBand = 0;
        section(1);
        // the cells binned once, on the pool by rows of cells (every ring's rows in one dispatch), then each band thread draws
        // the cells that reach its rows
        bool outermost[5]; int firstRow[6], rowsAll = 0;
        for (int k = 0; k < 5; k++) {
            outermost[k] = true;
            for (int j = k + 1; j < 5; j++) if (rings[j].r > 0) outermost[k] = false;
            firstRow[k] = rowsAll;
            if (rings[k].r <= 0) continue;
            TerrainBin& bin = terrainBins[k];
            const int side = 2 * rings[k].r;
            bin.cols = side;
            if (bin.cells.size() < (size_t)side * side) bin.cells.resize((size_t)side * side);
            bin.count.assign(side, 0);
            rowsAll += side;
        }
        firstRow[5] = rowsAll;
        {
            const auto tb0 = std::chrono::steady_clock::now();
            std::atomic<int> staleN{0}, everyN{0};
            parallelFor(rowsAll, 4, [&](int b, int e) {
                int stale = 0, every = 0;
                for (int k = 0; k < 5; k++) {
                    const int lo = std::max(b, firstRow[k]), hi = std::min(e, firstRow[k + 1]);
                    if (lo < hi) binTerrainLOD(terrainBins[k], *rings[k].c, *rings[k].vc, rings[k].r, rings[k].cover, site.hasWater, outermost[k], lo - firstRow[k], hi - firstRow[k], stale, every);
                }
                staleN += stale; everyN += every;
            });
            lastStaleCorners = staleN; lastTerrainAllRows = everyN; lastTerrainCells = 0;
            for (int k = 0; k < 5; k++) if (rings[k].r > 0) for (int n : terrainBins[k].count) lastTerrainCells += n;
            lastTerrainBinMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tb0).count();
        }
        // the reflections gathered (serially: they fill the caches on a miss) for the bands to draw after the rings; the floor is
        // drawn first in every band, as it was drawn before the rings
        static const char* skipEnv2 = std::getenv("VESPERIS_SKIP");
        const bool reflections = site.hasWater && !(skipEnv2 && std::strstr(skipEnv2, "refl"));
        if (reflections) gatherReflections();
        static const bool traceBands = std::getenv("VESPERIS_TRACE") != nullptr;   // each band's time split (its ms and rows always)
        std::atomic<int> bandSlot{0};
        // the frame's rows dealt in chunks (a quarter of a thread's share): the chunk that holds the horizon's thousands of small cells
        // is one of thirty-odd, and the threads take the others meanwhile (cuts by a cost per cell and row lost: 4.31 -> 5.09 ms at 4x)
        parallelFor(FBH, 32, [&](int yb, int ye) {
            const auto tb0 = std::chrono::steady_clock::now();
            const int slot = bandSlot.fetch_add(1);
            BandTrace* bt = slot < TRACE_BANDS ? &lastTerrainBands[slot] : nullptr;
            if (bt) *bt = BandTrace();
            setRasterBand(yb, ye);
            drawFloor(fb);
            for (int k = 0; k < 5; k++) {
                Ring& g = rings[k];
                if (g.r <= 0) continue;
                drawTerrainLOD(fb, terrainBins[k], *g.c, *g.vc, g.r, g.cover, site.hasWater, t, yb, ye, outermost[k], traceBands ? bt : nullptr);
            }
            if (reflections) drawReflectionBand(fb, yb, ye);
            setRasterBand(0, 1 << 30);
            if (bt) { bt->y0 = yb; bt->y1 = ye; bt->ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tb0).count(); }
        });
        lastTerrainBandN = std::min(bandSlot.load(), TRACE_BANDS);
        std::sort(lastTerrainBands, lastTerrainBands + lastTerrainBandN, [](const BandTrace& a, const BandTrace& b) { return a.y0 < b.y0; });
        section(2);
    }
    drawFootprints(fb);
    if (cameraOverrideAlt < 0 && !inVehicle()) drawBlobShadow(fb, player.x, player.z, 0.32, 1.7);
    drawWaypointLine(fb);
    drawObjects(fb, t);
    section(3);
    drawFlora(fb, t);
    section(4);
    drawRuins(fb);
    drawShards(fb, t);   // after the walls, so the depth test hides what lies behind them
    if (buggy.deployed) drawBuggy(fb, t);
    if (drone.deployed) drawDrone(fb, t);
    drawLife(fb, t);
    if (cameraOverrideAlt < 0) drawCapsule(fb, t);   // while the capsule flies (the descent, the ascent) it is not on the ground: its beacon beam, seen from above, cut a bright line across the land
    drawWeather(fb, t);
    drawVeil(fb);
    section(5);
    // what the crosshair rests on, for the HUD's range readout and M (skipped while the capsule flies)
    lastRange = cameraOverrideAlt >= 0 ? -1 : rangeToGround(drawRadiusM(), lastRangeX, lastRangeZ, lastRangeWater);
    { static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr; if (trace && frameStamp <= 3) {
            fprintf(stderr, "  render %d: palette+sky %.1f, warm %.1f, terrain %.1f, footprints..objects %.1f, flora %.1f, rest %.1f ms\n", frameStamp, lastRenderMs[0], lastRenderMs[1],
                    lastRenderMs[2], lastRenderMs[3], lastRenderMs[4], lastRenderMs[5]);
            for (int f = 0; f < 2; f++) {
                const std::string bl = bandTraceLine(f == 1);
                if (!bl.empty()) fprintf(stderr, "    %s\n", bl.c_str());
            }
    } }
}

// F-01: the bands of the last frame as one line: rows and ms; the terrain's split and counts when they were traced
std::string SurfaceView::bandTraceLine(bool flora) const {
    const BandTrace* bs = flora ? lastFloraBands : lastTerrainBands;
    const int n = flora ? lastFloraBandN : lastTerrainBandN;
    if (n == 0) return std::string();
    char buf[160];
    std::string s = "flora bands:";
    if (!flora) { snprintf(buf, sizeof buf, "terrain bands (%d cells binned in %.2f ms, %d in every band):", lastTerrainCells, lastTerrainBinMs, lastTerrainAllRows); s = buf; }
    for (int i = 0; i < n; i++) {
        const BandTrace& b = bs[i];
        if (flora) snprintf(buf, sizeof buf, " %d-%d %.2f ms (%d visited, %d drawn)%s", b.y0, b.y1, b.ms, b.walked, b.drawn, i + 1 < n ? "," : "");
        else if (b.walked > 0)
            snprintf(buf, sizeof buf, " %d-%d %.2f ms (walk %.2f, water set-up %.2f, fill %.2f; %d walked, %d drawn, %d wet)%s", b.y0, b.y1, b.ms, b.ms - b.waterMs - b.fillMs, b.waterMs, b.fillMs,
                    b.walked, b.drawn, b.wet, i + 1 < n ? "," : "");
        else snprintf(buf, sizeof buf, " %d-%d %.2f%s", b.y0, b.y1, b.ms, i + 1 < n ? "," : "");
        s += buf;
    }
    return s;
}
