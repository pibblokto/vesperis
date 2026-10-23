#include "surface_view.h"
#include "galaxy/plan.h"
#include "galaxy/drainage.h"
#include <cstring>
#include <cstdlib>
#include "core/noise.h"
#include "core/parallel.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <atomic>

namespace {
struct SurfaceLook {
    RGB ground, secondary, tertiary, sand, snow, grass, flora2, zenith, horizon, nightZenith, nightHorizon;
    double skyDensity;     // brightness of the daytime sky
    double fogDistance;    // metres
    bool cloudsLayer;
};

// N1-03: the ground colours come from the body's material palette (BodyGen::matColor, shared with the globe
// and the landing map); the sky, the fog, the night and the cloud layer are the kind's row (P-00)
SurfaceLook lookFor(const BodyGen& g, const Star& star) {
    SurfaceLook L;
    const KindDef& K = KINDS[g.type];
    RGB lc = lerp(star.color, RGB(1, 1, 1), 0.5f);   // the star's light, softened (the globe and the map use the same tint, N1-05)
    RGB white(1, 1, 1);
    // O1 (B-302): the terrain is drawn to 53 km now, so the haze must let a clear day show it: felisian 12 km (was 5),
    // ocean 9, quartz 5, thin air 14, the airless worlds 45 (their "fog" is only the softening of distance)
    L.cloudsLayer = K.clouds;
    L.skyDensity = K.skyDensity;
    L.fogDistance = K.fog;
    L.nightZenith = K.ownNight ? K.nightZenith : RGB(0.01f, 0.01f, 0.03f);
    L.nightHorizon = K.ownNight ? K.nightHorizon : RGB(0.04f, 0.04f, 0.07f);
    L.zenith = RGB(0, 0, 0); L.horizon = RGB(0, 0, 0);
    typeSky(g, L.zenith, L.horizon);   // R-307: one pair per kind, shared with the globe and the landing map
    if (K.seaSky) { L.zenith = g.skyTint * 0.8f; L.horizon = lerp(g.skyTint, white, 0.5f); }   // the ocean's own sky over its sea
    if (K.lavaHorizon) L.horizon = K.horizonGlow;   // an airless horizon lit by the lava
    if (g.hasTrait(TR_HAZE)) { L.fogDistance *= 0.3; L.skyDensity *= 1.25; }   // R-304
    if (g.hasTrait(TR_FROZEN_AIR)) { L.fogDistance = 45000; L.cloudsLayer = false; }   // D-04: its air on its ground, its sky an airless world's
    const RGB* m = g.matColor;
    L.ground = m[familyRep(g.type, FAM_ROCK)];
    L.sand = m[familyRep(g.type, FAM_SAND)];
    L.snow = m[familyRep(g.type, FAM_SNOW)];
    L.grass = m[familyRep(g.type, FAM_GRASS)];   // P-00: a kind's first accent where it lends the grass bank
    L.flora2 = g.vegColor2;
    // bank 2 (secondary): water where there is a sea, lava on hot worlds, else the bright crystal/ejecta tint
    if ((K.tags & KT_SEA) || g.liquidLevel > -1e8) L.secondary = m[MAT_WATER];
    else if (isLavaWorld(g.type)) L.secondary = m[MAT_LAVA];
    else if (g.hasTrait(TR_GLASSED)) L.secondary = m[MAT_GLASS];   // S-03: the sheets of glass own bank 2 (the family's rep is the water the palette coloured as glass)
    else if (K.famRep[FAM_WATER] != MAT_WATER) L.secondary = m[K.famRep[FAM_WATER]];   // P-01: a kind's own in the water family's bank (the glass world's glass)
    else if (K.tags & KT_CRYSTAL) L.secondary = lerp(m[MAT_QUARTZ], white, 0.4f);
    else L.secondary = lerp(L.ground, white, 0.5f);
    // bank 3 (tertiary): the forest on living worlds (P-00: a kind's second accent), elsewhere boulders a shade lighter than the ground
    L.tertiary = ((K.tags & KT_FLORA) || K.accent[1] >= 0) ? m[familyRep(g.type, FAM_FOREST)] : lerp(L.ground, white, 0.25f);
    L.ground = L.ground * lc; L.secondary = L.secondary * lc; L.tertiary = L.tertiary * lc;
    L.sand = L.sand * lc; L.snow = L.snow * lc; L.grass = L.grass * lc; L.flora2 = L.flora2 * lc;
    L.zenith = L.zenith * lc; L.horizon = L.horizon * lc;
    return L;
}

inline double hash01(uint64_t h) { return unitFromHash(h); }
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
    // O6-02: the near ring sweeps fastest, so it takes every other job: near, lod0, near, lod1 (lod0 while it is off)
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
    auto tick = [&](const char* what) { static std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now(); auto now = std::chrono::steady_clock::now(); if (trace) fprintf(stderr, "  init %-14s %.1f ms\n", what, std::chrono::duration<double, std::milli>(now - last).count()); last = now; };
    tick("start");
    joinAhead();   // M7-01: a pending job refers to the old site
    site.init(sys, bodyIndex, lat, lon, t);
    site.clearRoads(); site.ensureRoads(0, 0); roadBuildX = 0; roadBuildZ = 0;   // C-09: the old roads round the site, before anything samples the ground
    site.clearMarks(); markCheckX = markCheckZ = 1e18;   // T-02
    site.clearEdits(); workCells.clear(); workNear.clear(); workEdited.clear();   // T-03
    worksHere = worldHadCivilisation(site.gen) ? worldWorksIfReady(site.gen) : nullptr;   // T-03: the world's works when built (never waited for)
    tick("site");
    grain.generate(site.gen.seed ^ 0x77, 9, 1);
    grainFine.generate(site.gen.seed ^ 0x99, 4, 0);
    grainEdge.generate(site.gen.seed ^ 0x5E, 64, 1);   // B-311: the wobble of material boundaries
    buildMacroTile(grainMacro, site.gen.seed);          // B-311: slow tone patches within a material
    buildLeafTiles(leafTile, leafEdge, site.gen.seed);   // B-315: the vegetation's tiles and looks
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
    drainagePrefetch(site.gen, site.up0, drawRadiusM() + 3000.0, false);   // O6-03: the drainage tiles the rings will read, before the caches fill
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
    buggy = Buggy(); inBuggy = false; chaseCam = false;   // B-205: no buggy carries over to a new landing
    drone = Drone(); inDrone = false;                      // R-403: nor a drone
    lastWindUpdate = -1; lastEnvT = -1;
    prefetchStage = 0; prefetchRing = 0;
    for (int m = 0; m < MAT_COUNT; m++) mesoBuilt[m] = false;
    footprints.clear(); footHead = 0; stepAccum = 0; lastStepX = 6; lastStepZ = -4;
    ruinCells.clear(); supportMeshes.clear(); supports.clear(); peoples = peoplesOf(site.gen) > 1 ? 2 : 1;   // C-01; C-13: the world's peoples, each with a culture and a lore (B-420: the footings' meshes with them)
    for (int k = 0; k < 2; k++) {
        cultures[k] = k < peoples ? cultureOf(site.gen, k) : Culture();
        lores[k] = worldHadCivilisation(site.gen) && k < peoples ? loreQuick(*site.sys, site.sys->bodies[site.body], site.gen, k) : Lore();   // C-12: for the graves' names and years
        palettes[k] = cultures[k].palette;   // A-02: its colours and windows
        traditions[k] = worldHadCivilisation(site.gen) && k < peoples ? traditionOf(site.gen, lores[k]) : Tradition();   // C-11: its music, its instruments
        if (worldHadCivilisation(site.gen) && k < peoples) instrumentsOf(traditions[k], lores[k].which, instrumentSets[k]); else instrumentSets[k] = PeopleInstruments();
    }
    placedCells.clear();   // A-02 (tests)
    refreshPlanNear();     // T-01: the plan's places round the landing (the plan was built at the approach)
    ensureTownMarks();     // T-02: the towns' streets and fields in the ground, before anything samples it
    if (!site.edits.empty()) { capsuleY = site.surfaceHeight(capsuleX, capsuleZ); player.y = site.surfaceHeight(player.x, player.z); }   // T-03: on the works' ground
    nearGrave = NearGrave();
    shardsFound.clear(); nearShard = NearShard();   // C-03 (the game refills the set from the guide after this)
    instrumentsFound.clear(); nearInstrument = NearInstrument();   // C-11 (likewise)
    hasWaypoint = false; eruptions.clear(); nextEruption = 30; devils.clear(); nextDevil = 20; quake = 0; nextQuake = 45; siteEpoch++;   // B-303: nothing of the last landing shows on this one's map
    quakeCue = 0; duneSong = 0; iceSong = 0; duneHz = duneSongHz(site.gen, site.gravity);   // Q-03
    tick("capsule+misc");
    joinScan(); echoes.clear(); scanned = false;   // R-408: the last site's echoes are gone; the scanner asks again
    tick("scan");
    spawnLife(lat, lon);   // N3: herds and flocks from the world's bestiary
    tick("life");
    buildBandMap();
    meteorShowersOf(*sys, bodyIndex, showers);   // W-04
    zodiDust = zodiacalDust(*sys, bodyIndex);
    lavaGlow = isLavaWorld(site.gen.type) || (site.gen.dark && site.gen.hasTrait(TR_EMBERS));   // D-04
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
    ringN = std::min(30, (int)std::floor(1.15 * site.R / 4));   // O6-02: 30 cells of 4 m
    if (ringN < 4) ringN = 0;
    invTwoR = 0.5 / site.R;
}

// O6-02: the near ring follows the explorer (or the buggy); B-313: at every speed
void SurfaceView::updateNearRing(double dt) {
    (void)dt;
    // B-313: the near ring is on at every speed (the buggy's suspension soaks up its relief, updateBuggy); it used to fade
    // out above 12 m/s and back in as the buggy slowed, and the 4 m relief rising over half a second as you stopped
    // and got out read as the hills changing shape in front of you
    nearBlend = (ringN && cameraOverrideAlt < 60) ? 1.0 : 0.0;
    double cx = inBuggy ? buggy.x : player.x, cz = inBuggy ? buggy.z : player.z;
    site.nearBlend = nearBlend; site.nearX = cx; site.nearZ = cz; site.nearR = ringN;
}

// The galaxy seen from inside, integrated once per landing (shared code in galaxy/starfield).
void SurfaceView::buildBandMap() {
    Vec3 obs = site.worldPos(0, 0, 0, 0) / SECTOR_KM;
    buildGalaxyBand(obs, bandMap, BAND_MAP_W, BAND_MAP_H);
    nebN = nebulaPatches(obs, nebP, 16);   // N5-01
}
double SurfaceView::bandAt(const Vec3& d) const { return sampleGalaxyBand(bandMap, d); }

void SurfaceView::relocateCapsule(double x, double z) {
    capsuleX = x; capsuleZ = z;
    capsuleY = site.surfaceHeight(x, z);
}

// F-03: the material tiles of `want` (a bit a material) not made yet, made together, a material to a worker (a tile is a function of
// its material and the seed, each into its own slot; ~1 ms each, five or six at a landing's first ring on a peopled world)
void SurfaceView::buildMesoTiles(uint32_t want) {
    static_assert(MAT_COUNT <= 32, "a bit a material");
    int mats[MAT_COUNT], k = 0;
    for (int m = 0; m < MAT_COUNT; m++) if ((want >> m & 1u) && !mesoBuilt[m] && materialHasMesoTile(m)) mats[k++] = m;
    parallelFor(k, 1, [&](int b, int e) { for (int i = b; i < e; i++) buildMesoTile(mesoTiles[mats[i]], mats[i], site.gen.seed); });
    for (int i = 0; i < k; i++) mesoBuilt[mats[i]] = true;
}

const GrainTexture* SurfaceView::mesoFor(int material) {
    if (material < 0 || material >= MAT_COUNT || !materialHasMesoTile(material)) return nullptr;
    if (!mesoBuilt[material]) { buildMesoTile(mesoTiles[material], material, site.gen.seed); mesoBuilt[material] = true; }
    return &mesoTiles[material];
}

// Fraction of direct sunlight reaching a vertex (M10-06): march toward the sun over the
// height cache; anything rising above the sun ray shadows it, with a soft edge.
double SurfaceView::castShadowLit(TerrainCache& cache, int cx, int cz, double h, const Vec3& sd) {
    double horiz = std::sqrt(sd.x * sd.x + sd.z * sd.z);
    if (horiz < 1e-4 || sd.y <= 0) return 1.0;
    double tanAlt = sd.y / horiz;
    if (tanAlt > 2.5) return 1.0;               // above 68 degrees nothing casts at this resolution
    double dx = sd.x / horiz, dz = sd.z / horiz;
    double cs = cache.cellSize;
    double x0 = cx * cs, z0 = cz * cs;
    double limit2 = (double)curRadius * cs * curRadius * cs;
    double maxDepth = 0;
    for (int i = 1; i <= 9; i++) {
        double s = cs * i * (1.0 + 0.12 * i);
        double px = x0 + dx * s, pz = z0 + dz * s;
        double ex = px - camPos.x, ez = pz - camPos.z;
        if (ex * ex + ez * ez > limit2) break;   // stay inside the cached ring
        double th = cache.at((int)std::lround(px / cs), (int)std::lround(pz / cs)).h;
        double rayH = h + s * tanAlt + 0.35;
        if (th - rayH > maxDepth) maxDepth = th - rayH;
    }
    return 1.0 - 0.6 * smoothstep(0.0, 3.5 + 0.03 * cs, maxDepth);   // never fully black: light bounces around
}

// Blob shadow (M10-11): a dark ellipse on the ground under an object, stretched away from the sun.
void SurfaceView::drawBlobShadow(Framebuffer& fb, double x, double z, double radius, double height) {
    if (!env.hasSun2 || env.sun2Share < 0.05 || env.sun2Share > 0.95) { drawBlobShadowFrom(fb, x, z, radius, height, env.sun.dirLocal, env.glow); return; }   // B-425: as strong as the light (a far sun alone casts a faint one)
    // M5-01 double shadows: one per sun, each as strong as its share of the light
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

// F-04, F-05: the rows a convex polygon can fill: clipped at the near plane as rasterPolygon clips it, projected as it projects, a row of
// slack each side; false when it fills none (wholly beside, above or below the frame: the rasteriser rejects it). `every` is set when a
// point projects to no finite place
static bool polygonRows(const Proj& pj, const Vec3* p, int np, int& y0, int& y1, bool& every) {
    double minx = 1e300, maxx = -1e300, miny = 1e300, maxy = -1e300;
    int m = 0;
    auto at = [&](double x, double y, double z) {
        const double iz = 1.0 / z, sx = pj.cx + pj.f * x * iz, sy = pj.cy - pj.f * y * iz;
        if (!std::isfinite(sx) || !std::isfinite(sy)) every = true;
        minx = std::min(minx, sx); maxx = std::max(maxx, sx); miny = std::min(miny, sy); maxy = std::max(maxy, sy);
        m++;
    };
    for (int i = 0; i < np; i++) {   // rasterPolygon's clip against z = NEAR_Z
        const Vec3& a = p[i]; const Vec3& b = p[(i + 1) % np];
        const bool ain = a.z >= NEAR_Z, bin = b.z >= NEAR_Z;
        if (ain) at(a.x, a.y, a.z);
        if (ain != bin) { const double t = (NEAR_Z - a.z) / (b.z - a.z); at(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t); }
    }
    if (m < 3 || maxx < -1 || minx > FBW + 1) return false;
    const double ylo = std::ceil(miny - 0.5) - 1, yhi = std::floor(maxy - 0.5) + 1;
    if (yhi < 0 || ylo > FBH - 1) return false;
    y0 = (int)std::max(-1.0, ylo); y1 = (int)std::min((double)FBH, yhi);
    return true;
}

// M1-04: the land mirrored about the sea level, blended into water pixels only (bank 2), far to near. F-05: the polygons gathered here
// (serially) with their rows, then drawn by the terrain's band threads (drawReflectionBand)
void SurfaceView::gatherReflections() {
    reflVerts.clear(); reflPolys.clear();
    struct Cell { double d2; int cx, cz; int lod; double level; };
    std::vector<Cell> cells;
    double sea = site.seaLevel + site.tideM;   // Q-02: the tide's
    for (int lod = 1; lod >= 0; lod--) {
        TerrainCache& cache = lod == 0 ? site.lod0 : site.lod1;
        double cs = cache.cellSize;
        int R = lod == 0 ? 20 : 20;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        for (int cz = pcz - R; cz < pcz + R; cz++)
            for (int cx = pcx - R; cx < pcx + R; cx++) {
                double dx = cx + 0.5 - camPos.x / cs, dz = cz + 0.5 - camPos.z / cs;
                double d2 = dx * dx + dz * dz;
                if (d2 > (double)R * R) continue;
                if (lod == 1 && d2 < 5.5 * 5.5) continue;   // covered by lod0
                double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
                double dd = std::sqrt(wx * wx + wz * wz);
                if (dd > cs * 3 && (wx * camLocal.m[2][0] + wz * camLocal.m[2][2]) / dd < -0.25) continue;
                double hmax = std::max(std::max(cache.at(cx, cz).h, cache.at(cx + 1, cz).h), std::max(cache.at(cx, cz + 1).h, cache.at(cx + 1, cz + 1).h));
                // B-320: a cell on the shore of a lake or a river (its vertices carry that water's level) mirrors round
                // that level; every cell above the sea mirrors round the sea
                double wl = std::max(std::max(site.waterOf(cache.at(cx, cz)), site.waterOf(cache.at(cx + 1, cz))), std::max(site.waterOf(cache.at(cx, cz + 1)), site.waterOf(cache.at(cx + 1, cz + 1))));
                if (wl > sea + 0.5 && hmax > wl + 0.3) cells.push_back({d2 * cs * cs, cx, cz, lod, wl});
                if (hmax < sea + 0.3) continue;
                cells.push_back({d2 * cs * cs, cx, cz, lod, sea});
            }
    }
    std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) { return a.d2 > b.d2; });
    for (const Cell& c : cells) {
        TerrainCache& cache = c.lod == 0 ? site.lod0 : site.lod1;
        std::vector<VtxCache>& vcache = c.lod == 0 ? vc0 : vc1;
        double cs = cache.cellSize;
        uint8_t bk;
        const RVert* vs[4] = {&vertexOf(cache, vcache, c.cx, c.cz, bk), &vertexOf(cache, vcache, c.cx + 1, c.cz, bk),
                              &vertexOf(cache, vcache, c.cx + 1, c.cz + 1, bk), &vertexOf(cache, vcache, c.cx, c.cz + 1, bk)};
        int ox[4] = {0, 1, 1, 0}, oz[4] = {0, 0, 1, 1};
        double flat = 0, hs[4], xs[4], zs[4];
        for (int i = 0; i < 4; i++) {
            xs[i] = (c.cx + ox[i]) * cs; zs[i] = (c.cz + oz[i]) * cs;
            hs[i] = cache.at(c.cx + ox[i], c.cz + oz[i]).h;
            flat += vs[i]->shade * 0.25;
        }
        // B-322: only the part of the cell that stands above the water is mirrored: each of the cell's two triangles (the
        // split drawTerrainLOD draws) clipped against the water level, then folded under it. The old quad put the wet
        // corners at the level itself, a slab under the whole cell: from a low eye (swimming) the slabs of the shore cells
        // passed under the camera and, clipped at the near plane, stood in the water as tall dark bars
        static const int tris[2][3] = {{0, 1, 3}, {1, 2, 3}};
        // B-322: the cell's dry side first (the shore cut the ground is drawn with), then the part of it above the level
        const TerrainVertex* tvc[4] = {&cache.at(c.cx, c.cz), &cache.at(c.cx + 1, c.cz), &cache.at(c.cx + 1, c.cz + 1), &cache.at(c.cx, c.cz + 1)};
        double wMean = 0; int wN = 0;
        for (int i = 0; i < 4; i++) if (tvc[i]->water > -1e8f) { wMean += site.waterOf(*tvc[i]); wN++; }
        if (wN) wMean /= wN;
        for (int k = 0; k < 2; k++) {
            double px[4], pz[4], ph[4]; int n = 0;
            if (wN) {
                double xs3[3], zs3[3], hs3[3], wl3[3], sh3[3];
                bool anyWet = false, anyDry = false;
                for (int e = 0; e < 3; e++) { int a = tris[k][e]; xs3[e] = xs[a]; zs3[e] = zs[a]; hs3[e] = hs[a]; wl3[e] = tvc[a]->water > -1e8f ? site.waterOf(*tvc[a]) : wMean; sh3[e] = vertexShore(cache, c.cx + ox[a], c.cz + oz[a]); if (sh3[e] < 0) anyWet = true; else anyDry = true; }
                if (anyWet && anyDry) {
                    ShoreSplit sp; shoreSplitTriangle(xs3, zs3, hs3, wl3, sh3, sp);
                    for (int i = 0; i < sp.nDry; i++) { px[n] = sp.dry[i].x; pz[n] = sp.dry[i].z; ph[n] = sp.dry[i].h; n++; }
                } else if (anyWet) continue;   // wholly under its water: nothing to mirror
            }
            if (n == 0) for (int e = 0; e < 3; e++) { int a = tris[k][e]; px[n] = xs[a]; pz[n] = zs[a]; ph[n] = hs[a]; n++; }
            RVert q[6]; int m = 0;
            for (int e = 0; e < n; e++) {
                int a = e, b = (e + 1) % n;
                bool ain = ph[a] >= c.level, bin = ph[b] >= c.level;
                auto emit = [&](double x, double h, double z) {
                    Vec3 v = toView(x, 2 * c.level - h, z);   // the curvature comes with toView (O1)
                    q[m].x = v.x; q[m].y = v.y; q[m].z = v.z; q[m].shade = flat * 0.85; m++;
                };
                if (ain) emit(px[a], ph[a], pz[a]);
                if (ain != bin) { double t = (c.level - ph[a]) / (ph[b] - ph[a]); emit(px[a] + (px[b] - px[a]) * t, c.level, pz[a] + (pz[b] - pz[a]) * t); }
            }
            if (m < 3) continue;
            ReflPoly rpo; rpo.first = (int)reflVerts.size(); rpo.n = m;
            Vec3 pv[6]; for (int e = 0; e < m; e++) pv[e] = Vec3(q[e].x, q[e].y, q[e].z);
            bool every = false;
            const bool fills = polygonRows(proj, pv, m, rpo.y0, rpo.y1, every);
            if (every) { rpo.y0 = -(1 << 30); rpo.y1 = 1 << 30; }
            else if (!fills) continue;   // it fills no row of the frame (the rasteriser rejects it)
            reflVerts.insert(reflVerts.end(), q, q + m);
            reflPolys.push_back(rpo);
        }
    }
}

// F-05: the gathered reflections that reach this band's rows, in order, clipped to it (the caller's raster band)
void SurfaceView::drawReflectionBand(Framebuffer& fb, int bandY0, int bandY1) const {
    RasterParams rp;
    rp.blend = BLEND_REFLECT; rp.maskBank = 2; rp.reflectMix = 0.45; rp.ztest = false; rp.zwrite = false;
    for (const ReflPoly& p : reflPolys) {
        if (p.y1 < bandY0 || p.y0 >= bandY1) continue;
        rasterPolygon(fb, &reflVerts[p.first], p.n, rp, proj);
    }
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

// M4-07's jittered 2 km latitude/longitude grid, so a settlement stays where it is whatever the landing site or the anchor of the local
// frame. C-01: the grid, the draw and the settlements live in `galaxy/ruins.*`; the view keeps a cache per cell in local metres.
void SurfaceView::ruinCellAt(double x, double z, int& gLat, int& gLon) const {
    double lat, lon;
    site.latLonAt(x, z, lat, lon);
    ruinCellOf(site.gen, lat, lon, gLat, gLon);
}

const SurfaceView::RuinCell* SurfaceView::ruinCell(int gLat, int gLon) const {
    int nLon = std::max(1, ruinCellsAround(site.gen, gLat));
    int gl = ((gLon % nLon) + nLon) % nLon;
    uint64_t key = hash2i(gLat, gl, 0x5E77ULL);
    auto it = ruinCells.find(key);
    if (it != ruinCells.end()) return &it->second;
    RuinCell& c = ruinCells[key];
    c.has = ruinOfCell(site.gen, gLat, gl, c.spec, true);
    if (c.has) { DrainageOff off; c.has = ruinSiteOk(site.gen, c.spec); }   // the slope and the sea, without building tiles (the water's own check is at the draw)
    if (c.has) {
        Ruin& r = c.r;
        site.localAt(StarSystem::bodyFromLatLon(c.spec.lat, c.spec.lon), r.x, r.z);
        r.heading = c.spec.heading; r.size = c.spec.size; r.spec = &c.spec;
        {   // C-03: and its shards; C-12: and its graves, their stones drawn with the pieces; C-13: in its people's culture and lore
            const Culture& cu = cultures[c.spec.people == 1 && peoples > 1 ? 1 : 0]; const Lore& lo = lores[c.spec.people == 1 && peoples > 1 ? 1 : 0];
            for (int l = 0; l < 3; l++) ruinElements(c.spec, cu, l, c.elems[l]); recordSitesOf(site.gen, c.spec, cu, c.shards, &c.instruments); gravesOf(lo, c.spec, cu, c.graves); graveElements(c.graves, cu, 0, c.elems[0]);
            recordObjects(c);   // T-04
            instrumentObjects(c);   // C-11
            groupPieces(c);   // A-02: each building's pieces, for its own level of detail
        }
    }
    return &c;
}

// T-03: a work's cell: its pieces laid out once (`workSpec`, galaxy/works.h) as a settlement's are, in its people's culture, and a necropolis's
// or a battlefield's graves among them
const SurfaceView::RuinCell* SurfaceView::workCell(const Work& w) const {
    auto it = workCells.find(w.id);
    if (it != workCells.end()) return &it->second;
    RuinCell& c = workCells[w.id];
    workSpec(site.gen, w, c.spec);
    c.has = true;
    Ruin& r = c.r;
    site.localAt(w.unit, r.x, r.z);
    r.heading = c.spec.heading; r.size = c.spec.size; r.spec = &c.spec;
    const int k = w.people == 1 && peoples > 1 ? 1 : 0;
    for (int l = 0; l < 3; l++) ruinElements(c.spec, cultures[k], l, c.elems[l]);
    gravesOf(lores[k], c.spec, cultures[k], c.graves); graveElements(c.graves, cultures[k], 0, c.elems[0], true);
    recordSitesOf(site.gen, c.spec, cultures[k], c.shards); recordObjects(c);   // T-04: the works that kept records (the last shelter's last recording)
    groupPieces(c);
    return &c;
}

// T-02: the towns in reach (their cells within two of the explorer, a plan's core reaching in) put their streets, squares and fields into the
// ground, each once (the site keeps them until a landing or a re-anchor); the worker joined first, since the vertices read the list. T-03: and
// the works within six kilometres (of their reach) their ground
void SurfaceView::ensureTownMarks() {
    markCheckX = player.x; markCheckZ = player.z;
    if (!worldHadCivilisation(site.gen)) return;
    std::vector<const RuinCell*> todo;
    forNearbyRuins(player.x, player.z, 2, [&](const Ruin& r, const RuinCell& c, int) {
        if (c.spec.work < 0 && !c.marked && (!c.spec.streets.empty() || !c.spec.plazas.empty() || !c.spec.fields.empty())) todo.push_back(&c);
    });
    std::vector<const WorkNear*> works;
    for (const WorkNear& wn : workNear) {
        const Work& w = *wn.work;
        if (w.edits.empty() || workEdited.count(w.id) || std::hypot(wn.x - player.x, wn.z - player.z) - wn.radius > 6000) continue;
        works.push_back(&wn);
    }
    if (todo.empty() && works.empty()) return;
    joinAhead();
    for (const RuinCell* c : todo) {
        c->marked = true;
        const Culture& cu = cultures[c->spec.people == 1 && peoples > 1 ? 1 : 0];
        site.addTownMarks(c->spec, c->r.x, c->r.z, cu.ageYears + c->spec.before, cu.wet, cu.roadWear);
    }
    for (const WorkNear* wn : works) {
        const Work& w = *wn->work;
        workEdited.insert(w.id);
        const Culture& cu = cultures[w.people == 1 && peoples > 1 ? 1 : 0];
        site.addWorkEdits(w, wn->x, wn->z, cu.ageYears + w.before, cu.wet);
    }
}

void SurfaceView::refreshPlanNear() {   // T-01
    planNear.clear(); workNear.clear(); planNearX = player.x; planNearZ = player.z;
    if (!worldHadCivilisation(site.gen)) return;
    std::shared_ptr<const WorldPlan> P = worldPlanOf(site.gen);
    const Vec3 here = site.unitAt(player.x, player.z);
    std::vector<const PlanPlace*> near; planPlacesNear(*P, here, PLAN_NEAR_M, near);
    for (const PlanPlace* p : near) { PlanNear pn; pn.gLat = p->gLat; pn.gLon = p->gLon; site.localAt(p->unit, pn.x, pn.z); pn.radius = p->radiusM; planNear.push_back(pn); }
    if (worksHere) {   // T-03: the works; B-418: the world's and the land's
        std::vector<std::shared_ptr<const Work>> wn; worksAroundIfReady(site.gen, here, PLAN_NEAR_M, wn, LAND_NEAR_M);
        for (std::shared_ptr<const Work>& w : wn) { WorkNear n; site.localAt(w->unit, n.x, n.z); n.radius = w->radiusM; n.work = std::move(w); workNear.push_back(std::move(n)); }
    }
}

bool SurfaceView::ruinAt(int gLat, int gLon, Ruin& r) const {
    const RuinCell* c = ruinCell(gLat, gLon);
    if (!c->has) return false;
    r = c->r;
    return true;
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

// C-01: a settlement's pieces stand on the ground at their own foot, the near ones on the drawn ground (`groundHeight`, so the walls
// meet what the feet walk on and nothing floats as the near ring blends), the far ones on the planet function at 64 m, read once
// per building. A-02: every settlement in reach is gathered building by building and drawn as one batch (`drawFormItems`)
void SurfaceView::drawRuins(Framebuffer& fb) {
    lastRuinMs = 0; lastRuinElems = 0;
    if (!worldHadCivilisation(site.gen) && placedCells.empty()) return;
    auto tr0 = std::chrono::steady_clock::now();
    formItems.clear();
    for (const RuinCell& c : placedCells) gatherSettlement(c.r, c, std::hypot(c.r.x - camPos.x, c.r.z - camPos.z));   // A-02 (tests)
    static uint32_t drawStamp = 0; drawStamp++;
    forNearbyRuins(camPos.x, camPos.z, 2, [&](const Ruin& r, const RuinCell& cell, int) {
        cell.drawn = drawStamp;
        gatherSettlement(r, cell, std::hypot(r.x - camPos.x, r.z - camPos.z));
    });
    // T-01: the plan's places beyond the cells' reach, to PLAN_NEAR_M: a far city's skyline (each building at its level of detail, the far
    // ones a silhouette where they stand a pixel high)
    for (const PlanNear& pn : planNear) {
        const double d = std::hypot(pn.x - camPos.x, pn.z - camPos.z);
        if (d > PLAN_NEAR_M) continue;
        const RuinCell* c = ruinCell(pn.gLat, pn.gLon);
        if (c->has && c->drawn != drawStamp) { c->drawn = drawStamp; gatherSettlement(c->r, *c, d); }
    }
    gatherRecords();   // T-04
    lastRuinElems = (int)formItems.size();
    drawFormItems(fb);
    lastRuinMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tr0).count();
}

// M8-06: the buggy as a low box on four octagonal wheels, with a roll bar, an antenna light,
// its tracks and its dust.
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
    joinAhead();   // M7-01: cell coordinates change with the anchor
    double lat, lon;
    site.latLonAt(player.x, player.z, lat, lon);
    Vec3 capU = site.unitAt(capsuleX, capsuleZ);
    Vec3 bugU = site.unitAt(buggy.x, buggy.z);
    Vec3 drnU = site.unitAt(drone.x, drone.z);   // R-403
    Vec3 wpU = site.unitAt(wpX, wpZ);   // B-303: the waypoint moves with the frame too
    Vec3 oldNorth = site.north0;
    const StarSystem* sys = site.sys;
    int body = site.body;
    const auto ta1 = std::chrono::steady_clock::now();
    site.init(sys, body, lat, lon, lastT);
    const auto ta2 = std::chrono::steady_clock::now();
    site.clearRoads(); site.ensureRoads(0, 0); roadBuildX = 0; roadBuildZ = 0;   // C-09: the roads in the new frame's metres
    const auto ta3 = std::chrono::steady_clock::now();
    site.clearMarks(); markCheckX = markCheckZ = 1e18;   // T-02: added again from the cells in the new frame
    site.clearEdits(); workCells.clear(); workEdited.clear();   // T-03: the works too
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
    ruinCells.clear(); placedCells.clear(); supportMeshes.clear(); supports.clear();   // C-01: the cells' local metres moved with the frame (A-02: the harness's placed ones too; B-420: the footings' meshes)
    refreshPlanNear();   // T-01: the plan's places in the new frame's metres
    ensureTownMarks();   // T-02
    if (buggy.deployed) {   // the buggy moves with the frame too (B-205); a driven buggy is where the player is
        if (inBuggy) { buggy.x = 0; buggy.z = 0; }
        else { double bx, bz; site.localAt(bugU, bx, bz); buggy.x = bx; buggy.z = bz; }
        buggy.heading = wrap2pi(buggy.heading + dyaw);
        buggy.y = site.surfaceHeight(buggy.x, buggy.z);
        buggy.groundY = -1e9;   // B-307: the contact height is in the old frame
        buggy.tracks.clear(); buggy.trackHead = 0; buggy.puffs.clear();
    }
    if (drone.deployed) {   // R-403: the drone too; a flown drone is where the player is, at its height
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
    drainagePrefetch(site.gen, site.up0, drawRadiusM() + 3000.0, false);   // O6-03
    const auto ta5 = std::chrono::steady_clock::now();
    for (ScanEcho& e : echoes) site.localAt(e.lm.unit, e.x, e.z);   // R-408: the same echoes, in the new frame (a scan under way is taken in it)
    flocks.clear(); critters.clear(); herds.clear(); habitatT = -1e9;   // they lived near the old origin, 50 km away (B-316: the habitat cells respawn theirs)
    prefetchStage = 0; prefetchRing = 0;
    anchorFrame = frameStamp;   // F-01: the next frames' warm lines are traced (the rings' refill)
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
    if (trace) {
        auto ms = [](std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
        fprintf(stderr, "  reanchor: %.1f ms (the worker's join %.1f, the site's init %.1f, roads %.1f, marks, plan and the vehicles %.1f, the tiles asked %.1f)\n", ms(ta0, ta5), ms(ta0, ta1), ms(ta1, ta2), ms(ta2, ta3), ms(ta3, ta4), ms(ta4, ta5));
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
        std::vector<Landmark> works; worksHeardNear(g, at, SCAN_RANGE, works);   // T-03
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

void SurfaceView::computeEnvironment(double t) {
    const Body& b = site.sys->bodies[site.body];
    env.sun = site.sun(t);
    env.sunDisc = env.sun;
    env.lightColor = env.sun.color;   // the star's colour (S-01: whiter in a red dwarf's flare)
    env.hasSun2 = site.sun2(t, env.sun2);
    env.sun2Share = 0;
    env.glow = 1; env.primaryRel = 1;
    double sinAlt = env.sun.dirLocal.y;
    env.skyBrightness = site.atmosphere ? smoothstep(-0.12, 0.2, sinAlt) * (1 - 0.9 * env.sun.eclipse * env.sun.eclipse) : 0.0;   // M1-08
    if (env.hasSun2) {
        // M5-01: the light for shading blends both suns by their strength above the horizon; the sky and the
        // day brightness take the brighter of the two, so a companion still up after the primary set keeps the day alive.
        // B-425: each sun by its own light beside the world's own sun's (the far one's is faint: `SurfaceSite::sun2`); a sun under the
        // horizon adds none (the primary's whole light was added to a companion's still up, so a night under a far one was lit as a day)
        const bool ofK = site.sys->ofCompanion(site.body);
        const double ownLf = std::max(ofK ? env.sun2.lightFactor : env.sunDisc.lightFactor, 0.05);
        if (ofK) { env.primaryRel = clampd(env.sunDisc.lightFactor / ownLf, 0, 1); env.skyBrightness *= env.primaryRel; }
        double up1 = smoothstep(-0.12, 0.05, env.sunDisc.dirLocal.y), up2 = smoothstep(-0.12, 0.05, env.sun2.dirLocal.y);
        double w1 = env.sunDisc.lightFactor * up1, w2 = env.sun2.lightFactor * up2;
        if (w1 + w2 > 1e-6) {
            env.sun2Share = w2 / (w1 + w2);
            Vec3 dir = normalize(env.sunDisc.dirLocal * w1 + env.sun2.dirLocal * w2);
            if (env.sun2Share > 0.02) {
                env.sun.dirLocal = dir;
                env.sun.altitude = std::asin(clampd(dir.y, -1, 1));
                env.sun.azimuth = wrap2pi(std::atan2(dir.x, dir.z));
                env.sun.lightFactor = env.sunDisc.lightFactor * (1 - env.sun2Share) + (env.sunDisc.lightFactor * up1 + env.sun2.lightFactor) * env.sun2Share;
                env.sun.eclipse = env.sunDisc.eclipse * (1 - env.sun2Share);
                env.lightColor = lerp(env.sunDisc.color, env.sun2.color, (float)env.sun2Share);
            }
        }
        env.glow = clampd(env.sun.lightFactor / ownLf, 0, 1);
        double day2 = site.atmosphere ? smoothstep(-0.12, 0.2, env.sun2.dirLocal.y) * clampd(env.sun2.lightFactor / ownLf, 0, 1) : 0.0;
        env.skyBrightness = std::max(env.skyBrightness, day2);
        sinAlt = env.sun.dirLocal.y;
    }
    // Q-02 (TR_GREAT_TIDES): the sea's level now (the bulges of the world's moons and sun at the explorer's place: the whole site's, the tide a
    // few kilometres across being one)
    site.tideM = site.gen.tideRangeM > 0 && site.hasWater ? tideOffsetM(*site.sys, site.body, site.unitAt(player.x, player.z), t, site.gen.tideRangeM) : 0.0;
    env.ringShadow = env.sunDisc.ringShadow;   // O0-01
    env.skyBrightness *= 1 - 0.45 * env.ringShadow;
    env.localTime = env.hasSun2 && site.sys->ofCompanion(site.body) ? env.sun2.dayFraction : env.sun.dayFraction;   // B-425: the time of the world's own sun
    site.latLonAt(player.x, player.z, env.latDeg, env.lonDeg);
    env.latDeg /= DEG; env.lonDeg /= DEG;
    env.altitude = player.y;
    // weather: cloud cover from the planet map pattern plus slow drift
    double lat, lon;
    site.latLonAt(player.x, player.z, lat, lon);
    double cloud = 0;
    if (site.atmosphere && !hasOpaqueDeck(b.type)) {
        double drift = t * TAU / (std::fabs(b.rotPeriod) * 3.7) + 1.0;
        cloud = sampleCloudPattern(site.gen, lon + drift, lat);
        cloud = clampd(cloud * 1.3 + 0.25 * gnoise2(t / 900.0, site.gen.seed & 1023, site.gen.seed), 0, 1);
    }
    if (hasOpaqueDeck(b.type)) cloud = 1;
    env.cloudPattern = cloud;
    // W-03: the fronts. The world's fronts alive now (`frontsOf`, listed again every two minutes: a birth ramps up over
    // hours), their weather at the place (`frontWeatherAt`: the same function the globe's overlay and the forecast read),
    // and the nearest one's line in local metres for the sky, the ground's shadows and the wall of rain
    FrontWeather fw;
    localFront.on = false;
    if (site.atmosphere && !hasOpaqueDeck(b.type) && worldHasFronts(b)) {
        if (frontsSeed != b.seed || frontsT < -1e17 || std::fabs(t - frontsT) > 120) { frontsOf(b, t, fronts, 0); frontsT = t; frontsSeed = b.seed; }
        Vec3 u = site.unitAt(player.x, player.z);
        frontWeatherAt(fronts, u, t, site.R, fw);
        if (fw.nearest >= 0) {
            const Front& f = fronts[fw.nearest];
            Vec3 c, n; frontAt(f, t, c, n);
            Vec3 nt = n - u * dot(n, u);   // the motion along the ground here
            double nx = dot(nt, site.east0), nz = dot(nt, site.north0), L = std::sqrt(nx * nx + nz * nz);
            if (L > 1e-9) {
                localFront.on = true; localFront.nx = nx / L; localFront.nz = nz / L;
                localFront.x0 = player.x; localFront.z0 = player.z; localFront.ahead0 = fw.nearestAhead;
                localFront.width = f.width; localFront.gain = fw.nearestGain; localFront.tex = fw.nearestTex; localFront.strength = f.strength; localFront.dust = f.dust;
                localFront.rainVis = f.dust ? 400 : 1500;
            }
        }
    }
    env.frontCloud = fw.cloud; env.frontRain = fw.precip; env.frontCold = fw.cold; env.frontAhead = fw.nearest >= 0 ? fw.nearestAhead : 1e18;
    cloud = std::max(cloud, fw.cloud);
    env.cloudCover = cloud;
    double cloudP = env.cloudPattern;   // the pattern's cloud rains by its own rule; a front's band rains from its line, not from its first cloud
    const KindDef& K = KINDS[b.type];   // P-00: the kind's weather, temperature and pressure are its row's
    env.rain = 0;
    if (K.rain == RAIN_CLOUDS) env.rain = cloudP > K.rainThr ? clampd((cloudP - K.rainThr) / K.rainSpan, 0, 1) : 0;   // R-307: the acid rains often
    if (K.rain == RAIN_DRIZZLE) { double dz = gnoise2(t / 1800.0, 2.3, site.gen.seed + 27); env.rain = clampd((dz - 0.45) / 0.3, 0, 0.5); }   // a methane drizzle now and then
    if (K.tags & KT_FRONT_RAIN) env.rain = std::max(env.rain, fw.precip);   // W-03: the front's rain
    // M4-06: the same clouds snow when it is cold; thin air raises dust storms; fog banks drift by
    env.snow = 0; env.dust = 0; env.fogBank = 0; env.hail = 0;
    if (K.tags & KT_WATER_WEATHER) {
        double precip = std::max(cloudP > 0.62 ? clampd((cloudP - 0.62) / 0.3, 0, 1) : 0.0, fw.precip);
        if (precip > 0) {
            if (env.temperatureC < -1) { env.snow = precip; env.rain = 0; }
            else if (env.temperatureC < 8 && precip > 0.8) env.hail = (precip - 0.8) / 0.2;
        }
    }
    if (K.dustThreshold > 0 && site.atmosphere) {   // D-04: no dust rises where the air fell as snow
        double storm = gnoise2(t / 2400.0, 5.1, site.gen.seed + 21);
        env.dust = clampd((storm - K.dustThreshold) / 0.3, 0, 1);   // the storm's wind is applied with the wind update below (B-206); R-307: rarer on a desert world
        env.dust = std::max(env.dust, fw.precip);   // W-03: a dust front
    }
    if ((K.tags & KT_WATER_WEATHER) && site.atmosphere) {
        double fogN = gnoise2(t / 1500.0 + player.x / 4000.0, player.z / 4000.0, site.gen.seed + 33);
        env.fogBank = clampd((fogN - 0.45) / 0.25, 0, 1) * (1 - env.rain) * clampd(1 - std::fabs(env.temperatureC - 8) / 25.0, 0, 1);
    }
    // Q-02 (TR_FOG_SEAS): the valley fog of the morning. It gathers in the night's calm, fills the valleys to a level over their floors by
    // dawn and burns off as the sun climbs (local times 0.10-0.40, its height with it); no fog in rain, dust or a wind; how deep this
    // morning's is by the world and the day
    env.fogSea = 0; env.fogSeaTop = -1e9;
    if (site.atmosphere && site.gen.hasTrait(TR_FOG_SEAS)) {
        const double lt = env.localTime, day = std::floor(t / std::max(60.0, std::fabs(b.rotPeriod)));
        double fs = smoothstep(0.10, 0.22, lt) * (1 - smoothstep(0.30, 0.40, lt)) * (1 - env.rain) * (1 - env.dust) * smoothstep(28.0, 10.0, env.windKnots);
        if (fs > 0.01) {
            refreshFogFloor();
            const double depth = (60 + 110 * hash01(mix64(site.gen.seed ^ 0xF06A5EAULL))) * (0.6 + 0.4 * hash01(hash2i((int64_t)day, 7, site.gen.seed ^ 0xF06DULL)));
            env.fogSea = fs; env.fogSeaTop = fogFloorM + depth * fs;
        }
    }
    env.aurora = 0; env.auroraStorm = 0;
    env.flare = starFlare(site.sys->star, t);   // S-01: a red dwarf's flare opens the exposure (setupPalette) and is a storm of its own (auroralStorm)
    const bool greatAurorae = site.gen.hasTrait(TR_GREAT_AURORAE);   // Q-02: R-402's field set high
    if (site.atmosphere && !hasOpaqueDeck(b.type) && (std::fabs(env.latDeg) > 40 || greatAurorae)) {
        // R-402: the night's potential (the star's class, the world's magnetic field, the star's storm), the latitude ramp
        // pushed equatorward by the storm, the darkness, and the night's own slow variation. Q-02: under great aurorae the field's glow
        // lights most nights whatever the storm, and the ramp reaches the tropics
        env.auroraStorm = auroralStorm(site.sys->star, t);
        double latLo = greatAurorae ? 8 - 8 * env.auroraStorm : 52 - 10 * env.auroraStorm;
        double pot = auroraPotential(*site.sys, b, t);
        if (greatAurorae) pot = std::max(pot, 0.7 + 0.3 * env.auroraStorm);
        env.aurora = pot * smoothstep(latLo, latLo + 16, std::fabs(env.latDeg)) * (1 - env.skyBrightness) * (0.6 + 0.4 * gnoise2(t / 600.0, 8.8, site.gen.seed + 44));
    }
    if (lastWindUpdate < 0 || t - lastWindUpdate > 2.0) {
        // B-206: the wind is set from scratch here (base by type, a slow gust term, rain, the dust storm's x3.5);
        // it used to be multiplied by the storm factor every frame between these updates and ran away to 1e13 knots
        lastWindUpdate = t;
        double base = site.atmosphere ? K.wind : 0;
        const bool steady = (K.tags & KT_FIXED_SUN) && site.gen.locked;   // P-03: a locked world's surface wind blows toward the substellar point, and hardly changes
        env.windKnots = base * (steady ? 0.85 + 0.3 * (0.5 + 0.5 * gnoise2(t / 400.0, 3.3, site.gen.seed + 5)) : 0.4 + 0.8 * (0.5 + 0.5 * gnoise2(t / 400.0, 3.3, site.gen.seed + 5))) * (1 + env.rain * 1.5) * (1 + 2.5 * env.dust) * (1 + 1.0 * fw.windRise);   // W-03: the wind rises ahead of a front
        env.windDir = steady ? wrap2pi(std::atan2(env.sunDisc.dirLocal.x, env.sunDisc.dirLocal.z) + 0.2 * gnoise2(t / 2000.0, 7.7, site.gen.seed + 6)) : wrap2pi(1.7 + 2.0 * gnoise2(t / 2000.0, 7.7, site.gen.seed + 6));
        if (fw.windWeight > 0.01) {   // W-03: the front's wind takes over by its weight: along its line ahead of it, with its motion behind
            double fx = dot(fw.windTo, site.east0), fz = dot(fw.windTo, site.north0);
            double w = clampd(fw.windWeight, 0, 1);
            double vx = (1 - w) * std::sin(env.windDir) + w * fx, vz = (1 - w) * std::cos(env.windDir) + w * fz;
            if (vx * vx + vz * vz > 1e-6) env.windDir = wrap2pi(std::atan2(vx, vz));
        }
    }
    // the clouds' drift (sky layers and ground shadows) integrates the wind: 0.5 m per knot-second. It used to be
    // `wind x 0.5 x t`, which jumped by tens of kilometres whenever the wind changed (the sun flickering, B-206)
    if (lastEnvT < 0) { windDriftX = std::sin(env.windDir) * env.windKnots * 0.5 * t; windDriftZ = std::cos(env.windDir) * env.windKnots * 0.5 * t; }
    else if (t > lastEnvT) { windDriftX += std::sin(env.windDir) * env.windKnots * 0.5 * (t - lastEnvT); windDriftZ += std::cos(env.windDir) * env.windKnots * 0.5 * (t - lastEnvT); }
    lastEnvT = t;
    // temperature
    double base = b.tempK - 273.15;
    if (livingGround(b.type)) {
        // B-321: the biome model's mean temperature at this spot (the same number that chose the grass underfoot), with
        // a day-night swing round it; the HUD used to read -10 C on a grassland plateau from a model of its own
        double winter = clampd((env.latDeg >= 0 ? -site.season : site.season), 0, 1);
        double sub = site.gen.locked ? dot(site.unitAt(player.x, player.z), site.gen.lockedDir) : 0.0;
        base = climateTempC(site.gen, std::fabs(env.latDeg), player.y, winter, sub) - 3 + K.daySwing * sinAlt;   // P-02: the kind's swing (a steppe's wide)
    } else if (site.atmosphere && K.reachClimate && site.gen.locked) {   // P-03: the fixed sun's height decides it, the sea freezing at the edge of its reach (the eyeball's pupil)
        base = K.tempOffset + K.tempSwing * (sinAlt - std::cos(site.gen.seaReachDeg * DEG));
    } else if (site.atmosphere) {
        double offset = K.tempOffset, swing = K.tempSwing;   // R-307: each air has its greenhouse and its day-night swing
        base = base * 0.75 + offset;
        base += swing * sinAlt - 0.4 * std::fabs(env.latDeg);
    } else {
        base = base * 1.3 - 60 + (sinAlt > 0 ? 110 * sinAlt : -70);
        base -= 0.5 * std::fabs(env.latDeg);
    }
    if (!livingGround(b.type)) base -= 6.5 * (player.y / 1000.0);
    if (K.tempAdd != 0) base += K.tempAdd;   // the ground's own heat (a molten world's, a volcanic moon's)
    if (K.nightSideC > -1e8 && site.gen.locked) base = K.nightSideC + (base - K.nightSideC) * smoothstep(-0.12, 0.08, sinAlt);   // P-01: no air carries the day round to the night side
    // D-04: in the dark the ground is as warm as the world's own heat keeps it (`Body::tempK`: a drifter's by its kind, a moon's own and a
    // brown dwarf's glow on it), no day to swing it; an air cools with height over it
    if (site.sys->sunless()) base = b.tempK - 273.15 - (site.atmosphere ? 6.5 * player.y / 1000.0 : 0.0);
    env.temperatureC = std::max(base - fw.cold, -270.0);   // W-03: the cold behind a front
    env.pressureAtm = K.pressureSpan > 0 ? K.pressureLo + K.pressureSpan * hash01(site.gen.seed) : 0;
    if (airFrozen(b)) env.pressureAtm *= 1e-6;   // D-04: the vapour over its snow, a trace
    env.pressureAtm *= std::exp(-player.y / 8000.0);
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    env.fogDistance = L.fogDistance * (1 - 0.7 * env.rain) * (1 - 0.55 * env.snow) * (1 - 0.94 * env.dust) * (1 - 0.9 * env.fogBank) * (1 - 0.5 * env.frontRain);   // W-03: a front's squall closes the view further
    if (K.lowFogM > -1e8) env.fogDistance *= clampd((player.y - K.lowFogM) / K.lowFogSpan, 0.03, 1.0);   // P-01: walking down into the steam world's lowlands, the fog closes
    // Q-02: inside a fog sea the fog closes as in the steam world's lowlands (by the eye: `render` once the eye is placed)
    // O4: a comet vents harder the closer it dives to its star (the same activity as the tail seen from space);
    // the dust of the coma hangs over the ground
    env.cometActivity = 0;
    if (b.type == PT_COMET) {
        const StarSystem& S = *site.sys;
        double dStar = length(S.bodyPos(site.body, t) - S.star.pos);
        env.cometActivity = cometActivityAt(S, dStar);
        env.fogDistance *= 1 - 0.85 * env.cometActivity;
    }
    if (player.underwater) env.fogDistance = 22;
    // M10-09: planetshine / moonlight from the brightest body above the horizon
    env.moonLight = 0; env.moonBody = -1; env.moonDir = Vec3(0, 1, 0); env.moonColor = RGB(0.8f, 0.85f, 1.0f);
    {
        Mat3 L = site.localFrame(t);
        Vec3 sitePos = site.worldPos(t, player.x, player.z, 0.002);
        for (int bi = 0; bi < (int)site.sys->bodies.size(); bi++) {
            if (bi == site.body) continue;
            const Body& ob = site.sys->bodies[bi];
            Vec3 bp = site.sys->bodyPos(bi, t);
            Vec3 rel = bp - sitePos;
            double dist = length(rel);
            Vec3 dirL = L * (rel / dist);
            if (dirL.y < 0.02) continue;
            double illum = bodyShineAt(*site.sys, bi, sitePos, t, env.sun.lightFactor);   // W-04: the formula a moon's earthshine reads too
            bool glowing = kindHas(ob.type, KT_GLOWS);
            if (illum > env.moonLight) {
                env.moonLight = illum; env.moonBody = bi; env.moonDir = dirL;
                env.moonColor = glowing ? lerp(ob.color, RGB(1, 0.7f, 0.45f), 0.4f) : lerp(ob.color * lerp(site.sys->star.color, RGB(1, 1, 1), 0.5f), RGB(1, 1, 1), 0.35f);
            }
        }
    }
    double dx = capsuleX - player.x, dz = capsuleZ - player.z;
    env.capsuleDist = std::sqrt(dx * dx + dz * dz);
    env.capsuleBearing = wrap2pi(std::atan2(dx, dz));
    env.nearCapsule = env.capsuleDist < 7.0;
    findNearShard();   // C-03
    findNearGrave();   // C-12
    findNearInstrument();   // C-11
}

// Q-02 (TR_FOG_SEAS): the floor of the valleys round the explorer: a low quantile of the 512 m ring's ground over 6 km (a lake's or the sea's
// level where it is water: the fog lies on them as on the land), found again when the explorer has gone 1.5 km
void SurfaceView::refreshFogFloor() {
    if (std::fabs(player.x - fogFloorX) < 1500 && std::fabs(player.z - fogFloorZ) < 1500) return;
    fogFloorX = player.x; fogFloorZ = player.z;
    std::vector<double> hs;
    const int cx0 = (int)std::lround(player.x / site.lod2.cellSize), cz0 = (int)std::lround(player.z / site.lod2.cellSize);
    for (int dz = -6; dz <= 6; dz++)
        for (int dx = -6; dx <= 6; dx++) {
            const TerrainVertex& v = site.lod2.at(cx0 + dx, cz0 + dz);
            hs.push_back(v.water > -1e8f ? std::max((double)v.h, (double)v.water) : (double)v.h);
        }
    std::sort(hs.begin(), hs.end());
    fogFloorM = hs[hs.size() / 5];
}

// C-03: the nearest shard the explorer does not have within 2.2 m, on this floor (within 2 m of height), for E and the HUD. T-04: its object,
// its place (a work's kind where it lies in one), whether it is a painted wall, one of an archive's, the last recording
void SurfaceView::findNearShard() {
    nearShard = NearShard();
    if (inVehicle() || !worldHadCivilisation(site.gen)) return;
    double best = 2.2;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        for (const ShardSite& s : cell.shards) {
            if (shardsFound.count(s.index)) continue;
            double x = r.x + s.x, z = r.z + s.z, d = std::sqrt((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z));
            if (d >= best) continue;
            if (std::fabs(site.groundHeight(x, z) - player.y) > 2.0) continue;
            best = d; nearShard.index = s.index; nearShard.place = s.place; nearShard.sclass = cell.spec.sclass; nearShard.x = x; nearShard.z = z; nearShard.dist = d;
            nearShard.medium = s.medium; nearShard.work = cell.spec.work; nearShard.wall = s.wallD > 0; nearShard.archive = s.archive > 0; nearShard.last = s.last;
        }
    });
}

// C-11: the nearest instrument the explorer does not have within 2.2 m, on this floor, for E and the HUD
void SurfaceView::findNearInstrument() {
    nearInstrument = NearInstrument();
    if (inVehicle() || !worldHadCivilisation(site.gen)) return;
    double best = 2.2;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        if (cell.instrumentIds.size() != cell.instruments.size()) return;
        const int k = cell.spec.people == 1 && peoples > 1 ? 1 : 0;
        for (size_t i = 0; i < cell.instruments.size(); i++) {
            const int id = cell.instrumentIds[i];
            if (id < 0 || instrumentsFound.count(id)) continue;
            const InstrumentSite& s = cell.instruments[i];
            const double x = r.x + s.x, z = r.z + s.z, d = std::sqrt((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z));
            if (d >= best || std::fabs(site.groundHeight(x, z) - player.y) > 2.0) continue;
            const Instrument* I = instrumentById(instrumentSets[k], id);
            if (!I) continue;
            best = d; nearInstrument.id = id; nearInstrument.name = instrumentName(traditions[k], *I); nearInstrument.people = k; nearInstrument.x = x; nearInstrument.z = z; nearInstrument.dist = d;
        }
    });
}

// C-11: a cell's instruments: which each site holds (its people's instrument of the record's age the pick names) and its object
void SurfaceView::instrumentObjects(RuinCell& c) const {
    c.instrumentIds.clear(); c.instrumentForms.clear(); c.instrumentFirst.assign(1, 0);
    const int k = c.spec.people == 1 && peoples > 1 ? 1 : 0;
    std::vector<Form> one;
    for (const InstrumentSite& s : c.instruments) {
        const int id = instrumentIdOf(instrumentSets[k], s.age, s.pick);
        const bool again = std::find(c.instrumentIds.begin(), c.instrumentIds.end(), id) != c.instrumentIds.end();   // one of each in a settlement
        const Instrument* I = id >= 0 && !again ? instrumentById(instrumentSets[k], id) : nullptr;
        c.instrumentIds.push_back(I ? id : -1);
        if (I) { instrumentForms(*I, traditions[k], s.x, s.z, s.heading, one); c.instrumentForms.insert(c.instrumentForms.end(), one.begin(), one.end()); }
        c.instrumentFirst.push_back((int)c.instrumentForms.size());
    }
}

void SurfaceView::waterRecords() {
    if (!worldHadCivilisation(site.gen)) return;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cc, int) {
        if (cc.watered || std::hypot(r.x - player.x, r.z - player.z) > r.reach + 600) return;
        RuinCell& c = const_cast<RuinCell&>(cc);   // the view's own cache: its records laid again
        c.watered = true;
        if (c.shards.empty()) return;
        auto wetAt = [&](double sx, double sz) {   // a spot of the settlement's metres, and four round it a quarter of a cell off, 0.3 m over the water
            for (int k = 0; k < 5; k++) {
                const double x = r.x + sx + (k == 1 ? 4 : (k == 2 ? -4 : 0)), z = r.z + sz + (k == 3 ? 4 : (k == 4 ? -4 : 0)), w = site.waterAt(x, z);
                if (w > -1e8 && site.groundHeight(x, z) < w + 0.3) return true;
            }
            return false;
        };
        bool any = false;
        for (const ShardSite& s : c.shards) any = any || wetAt(s.x, s.z);
        for (const InstrumentSite& s : c.instruments) any = any || wetAt(s.x, s.z);
        if (!any) return;   // none in the water: as the generation laid them
        const std::function<bool(double, double)> fn = wetAt;
        const int k = c.spec.people == 1 && peoples > 1 ? 1 : 0;
        if (c.spec.work >= 0) recordSitesOf(site.gen, c.spec, cultures[k], c.shards, nullptr, &fn);
        else recordSitesOf(site.gen, c.spec, cultures[k], c.shards, &c.instruments, &fn);
        recordObjects(c);
        if (c.spec.work < 0) instrumentObjects(c);
    });
}

// C-12: the nearest grave within 2.2 m of the explorer on foot, for the HUD and the guide
void SurfaceView::findNearGrave() {
    nearGrave = NearGrave();
    if (inVehicle() || !worldHadCivilisation(site.gen)) return;
    double best = 2.2;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        for (size_t k = 0; k < cell.graves.size(); k++) {
            const Grave& g = cell.graves[k];
            double x = r.x + g.x, z = r.z + g.z, d = std::sqrt((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z));
            if (d >= best) continue;
            best = d; nearGrave.k = (int)k; nearGrave.id = g.id; nearGrave.sclass = cell.spec.sclass; nearGrave.line = graveLine(lores[cell.spec.people == 1 && peoples > 1 ? 1 : 0], g); nearGrave.hud = g.name + (g.born > 0 ? ", " + std::to_string(g.born) + "-" + std::to_string(g.died) : ", to " + std::to_string(g.died)); nearGrave.x = x; nearGrave.z = z; nearGrave.dist = d;
        }
    });
}

// T-04: a cell's records' objects (`recordForms`, galaxy/records.h), each from its own seed, kept with the cell (the frame's forms point at them)
void SurfaceView::recordObjects(RuinCell& c) {
    c.recordForms.clear(); c.recordFirst.assign(1, 0);
    std::vector<Form> one;
    for (const ShardSite& s : c.shards) {
        recordForms(s, mix64(c.spec.seed ^ ((uint64_t)(s.index + 1) * 0x9E3779B97F4A7C15ULL)), one);
        c.recordForms.insert(c.recordForms.end(), one.begin(), one.end());
        c.recordFirst.push_back((int)c.recordForms.size());
    }
}

// T-04: the records within 40 m of the camera into the frame's forms, on the floor where they lie (the drawn ground at the site), the ones the
// explorer has gone (a painted wall stays: it was recorded, not taken)
void SurfaceView::gatherRecords() {
    lastRecordForms = 0;
    if (!worldHadCivilisation(site.gen)) return;
    const double fwx = camLocal.m[2][0], fwz = camLocal.m[2][2];
    forNearbyRuins(camPos.x, camPos.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        if (cell.recordFirst.size() != cell.shards.size() + 1) return;
        const int people = cell.spec.people == 1 && peoples > 1 ? 1 : 0;
        for (size_t k = 0; k < cell.shards.size(); k++) {
            const ShardSite& s = cell.shards[k];
            if (shardsFound.count(s.index) && !(s.medium == RM_PAINTED && s.wallD > 0)) continue;
            const double x = r.x + s.x, z = r.z + s.z, dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 40 || (d > 3 && dx * fwx + dz * fwz < -1.5)) continue;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            const double fog = 1 - std::exp(-d / env.fogDistance);
            for (int i = cell.recordFirst[k]; i < cell.recordFirst[k + 1]; i++) {
                const Form& e = cell.recordForms[i];
                formItems.push_back({&e, Vec3(r.x + e.x, gy, r.z + e.z), 0.0, d, fog, 0, people});
                lastRecordForms++;
            }
        }
        if (cell.instrumentFirst.size() != cell.instruments.size() + 1 || cell.instrumentIds.size() != cell.instruments.size()) return;
        for (size_t k = 0; k < cell.instruments.size(); k++) {   // C-11: the instruments beside them, till taken
            const InstrumentSite& s = cell.instruments[k];
            if (cell.instrumentIds[k] < 0 || instrumentsFound.count(cell.instrumentIds[k])) continue;
            const double x = r.x + s.x, z = r.z + s.z, dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 40 || (d > 3 && dx * fwx + dz * fwz < -1.5)) continue;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            const double fog = 1 - std::exp(-d / env.fogDistance);
            for (int i = cell.instrumentFirst[k]; i < cell.instrumentFirst[k + 1]; i++) {
                const Form& e = cell.instrumentForms[i];
                if (d > 7 && (e.kind == FK_CABLE || formReach(e) < 0.012 * d / 7)) continue;   // C-17: far off, its strings and its small parts are under a pixel
                formItems.push_back({&e, Vec3(r.x + e.x, gy, r.z + e.z), 0.0, d, fog, 0, people});
                lastRecordForms++;
            }
        }
    });
}

// C-03: over each shard the explorer does not have, a point of light that pulses (the stars' bank), drawn within 250 m behind the depth test,
// so a glint through a doorway or over a fallen wall is what gives one away. T-04: the object itself is drawn with the forms (`gatherRecords`);
// a painted wall's light is on the wall, at the painting's height
void SurfaceView::drawShards(Framebuffer& fb, double t) {
    lastShardsDrawn = 0;
    if (!worldHadCivilisation(site.gen)) return;
    forNearbyRuins(camPos.x, camPos.z, 2, [&](const Ruin& r, const RuinCell& cell, int) {
        for (const ShardSite& s : cell.shards) {
            if (shardsFound.count(s.index)) continue;
            double x = r.x + s.x, z = r.z + s.z;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            double y = gy + 0.12;
            if (s.wallD > 0) { x -= std::sin(s.heading) * (s.wallD - 0.05); z -= std::cos(s.heading) * (s.wallD - 0.05); y = gy + 1.25; }
            else if (s.medium == RM_CRYSTAL || s.medium == RM_HUM) y = gy + 0.3;
            double dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 250) continue;
            double pulse = 0.5 + 0.5 * std::sin(t * 2.6 + s.index * 1.7);
            Vec3 v = toView(x, y, z);
            if (v.z > NEAR_Z) { RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = 48 + 15 * pulse; rasterPoint3(fb, q, 4, proj, true, d < 40 ? 2 : 1); }
            lastShardsDrawn++;
        }
        if (cell.instrumentIds.size() != cell.instruments.size()) return;
        for (size_t k = 0; k < cell.instruments.size(); k++) {   // C-11: an instrument's light: a double pulse, so it is told from a record's at a glance
            const int id = cell.instrumentIds[k];
            if (id < 0 || instrumentsFound.count(id)) continue;
            const InstrumentSite& s = cell.instruments[k];
            const double x = r.x + s.x, z = r.z + s.z;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            const double dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 250) continue;
            const double ph = std::fmod(t * 0.9 + id * 0.37, 1.0);
            const double pulse = std::exp(-std::pow((ph - 0.15) / 0.06, 2)) + std::exp(-std::pow((ph - 0.38) / 0.06, 2));
            Vec3 v = toView(x, gy + 0.35, z);
            if (v.z > NEAR_Z) { RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = 40 + 23 * std::min(1.0, pulse); rasterPoint3(fb, q, 4, proj, true, d < 40 ? 2 : 1); }
            lastShardsDrawn++;
        }
    });
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
    waterRecords();       // B-421
    if (inBuggy) updateBuggy(dt, in, t);
    else if (inDrone) updateDrone(dt, in, t);   // R-403
    else updateWalking(dt, in, t, controlsEnabled);
    prefetchAhead();   // M7-01
    collectScan();   // R-408
    if (scanWanted && !scanBusy.load(std::memory_order_acquire)) {   // R-408: the scanner on: a first scan, and another every SCAN_STEP
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
    // R-403: the drone's downwash settles, an unfolding drone keeps unfolding, a parked one sits on the ground with its pods winding down
    for (auto& p : drone.puffs) p.age += (float)dt;
    drone.puffs.erase(std::remove_if(drone.puffs.begin(), drone.puffs.end(), [](const Buggy::Puff& p) { return p.age > 1.5f; }), drone.puffs.end());
    if (drone.deployed && drone.unfold < 1) drone.unfold = std::min(1.0, drone.unfold + dt / 2.0);
    if (!inDrone && drone.deployed) {
        drone.y = site.surfaceHeight(drone.x, drone.z); drone.landed = true; drone.speed = 0; drone.vy = 0;
        drone.rotor *= std::exp(-dt * 0.8); drone.fanSpin += dt * TAU * (2.0 + 22.0 * drone.rotor);
        drone.vibration *= std::exp(-dt * 4); drone.jolt *= std::exp(-dt * 5);
    }
    {   // KI-007: keep the local frame close: 50 km on a planet, a third of the radius on a small body (O4)
        double reach = std::min(50e3, 0.35 * site.R);
        if (player.x * player.x + player.z * player.z > reach * reach) reanchor();
    }
    if (!worksHere && worldHadCivilisation(site.gen) && (worksHere = worldWorksIfReady(site.gen))) { refreshPlanNear(); markCheckX = markCheckZ = 1e18; }   // T-03: the works come in when built
    if (worldHadCivilisation(site.gen) && std::hypot(player.x - planNearX, player.z - planNearZ) > 1000) refreshPlanNear();   // T-01
    if (worldHadCivilisation(site.gen) && std::hypot(player.x - markCheckX, player.z - markCheckZ) > 250) ensureTownMarks();   // T-02: the towns coming into reach
    if (!site.roadCells.empty()) {   // C-09: the roads grow with the explorer: two cells from the last build, the next block (the worker joined first: the vertices read the list)
        double dx = player.x - roadBuildX, dz = player.z - roadBuildZ, two = 2 * ruinCellLat(site.gen) * site.R;
        if (dx * dx + dz * dz > two * two) {
            const auto tr0 = std::chrono::steady_clock::now();   // F-01: a rebuild's ms on the main thread (the worker's join with it)
            joinAhead(); site.ensureRoads(player.x, player.z); roadBuildX = player.x; roadBuildZ = player.z;
            lastRoadsMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tr0).count(); roadsBuilds++;
            static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
            if (trace) fprintf(stderr, "  roads grown %d: %.1f ms (%zu roads held)\n", roadsBuilds, lastRoadsMs, site.roads.size());
        }
    }
    if (t - drainCheckT > 3.0) {   // O6-03: the drainage tiles the far ring will reach next, computed ahead on their own thread
        drainCheckT = t;
        Vec3 u = site.unitAt(player.x, player.z);
        if (!drainageReady(site.gen, u, drawRadiusM() + 9000.0)) drainagePrefetch(site.gen, u, drawRadiusM() + 9000.0, true);
    }
    // M4-08: eruptions every minute or so on molten and volcanic worlds, within 400 m. R-307: a tectonic world's fountains
    // rise from its fissures (the nearest lava cell of the 16 m ring within 600 m) every 15-40 s and last 25 s; a bombarded
    // world takes a meteorite every 20-60 s within 1.2 km, and shakes for a moment
    const uint32_t events = KINDS[site.gen.type].events;   // P-00: the kind's events
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
    // R-307: the ground shakes on a tectonic world: a quake every 60-180 s, ten seconds long, the camera trembling by it. Q-03: on a world with
    // QUAKES every one and a half to four minutes; the quake's own sound at its onset (the tectonic world's thunder before)
    if ((events & EV_QUAKES) || site.gen.hasTrait(TR_QUAKES)) {
        nextQuake -= dt;
        if (nextQuake <= 0) { Rng qr((uint64_t)(t * 100) ^ 0x9A4E); quake = 0.6 + 0.4 * qr.uni(); nextQuake = (events & EV_QUAKES) ? 60 + 120 * qr.uni() : 90 + 150 * qr.uni(); quakeCue = quake; }
    }
    quake *= std::exp(-dt / 4.0);
    if (quake < 0.01) quake = 0;
    updateGroundSongs(dt, t);   // Q-03
    // R-307: dust devils on a desert world (and in a thin-air dust storm): up to four within 500 m, born every 20-50 s, each
    // walking with the wind for two to five minutes
    if (((events & EV_DEVILS) || ((events & EV_STORM_DEVILS) && env.dust > 0.3)) && env.skyBrightness > 0.3) {
        nextDevil -= dt;
        if (nextDevil <= 0 && devils.size() < 4) {
            Rng dr((uint64_t)(t * 100) ^ 0xD3B1);
            DustDevil d; d.x = player.x + dr.sym(500); d.z = player.z + dr.sym(500); d.height = 30 + 80 * dr.uni(); d.radius = 5 + 12 * dr.uni(); d.age = 0; d.life = 120 + 180 * dr.uni(); d.phase = dr.range(0, TAU);
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
    devils.erase(std::remove_if(devils.begin(), devils.end(), [&](const DustDevil& d) { double dx = d.x - player.x, dz = d.z - player.z; return d.age > d.life || dx * dx + dz * dz > 1500.0 * 1500.0; }), devils.end());
    updateLife(dt, t);
    computeEnvironment(t);
}

// Q-03: the note a world's singing dunes hum at: Andreotti's f = 0.4 sqrt(g / d) of its grains' size d (150-320 microns by a hash of the
// world: 70-100 Hz under Earth's gravity, about 50 on a Mars, 30 on a Titan)
double SurfaceView::duneSongHz(const BodyGen& g, double gravity) {
    const double d = (150 + 170 * unitFromHash(mix64(g.worldSeed ^ 0xD0E5ULL))) * 1e-6;
    return 0.4 * std::sqrt(std::max(gravity, 0.1) / d);
}

// Q-03: the cues of the sounds of the ground the world's traits give. The singing dunes: a dune's slip face of sand or dust (`slipFace`: its lee,
// however steep the world's dunes stand) crossed on foot or in the buggy slides; the hum louder the faster it is crossed and the more it is gone
// down, held a few seconds after as the sand runs on. The singing ice: the ice within 64 m (snow over it half), and how fast the day
// warms or cools it (the sun's height's rate over the world's day: the most at dawn and dusk, the cooling's more), a quarter of it always
void SurfaceView::updateGroundSongs(double dt, double t) {
    const BodyGen& g = site.gen;
    const double x = inBuggy ? buggy.x : player.x, z = inBuggy ? buggy.z : player.z;
    double dune = 0;
    if (g.hasTrait(TR_SINGING_DUNES) && site.atmosphere && !inDrone && (inBuggy ? !buggy.airborne : player.onGround && !player.swimming)) {
        const int mat = site.lod0.at((int)std::floor(x / 16), (int)std::floor(z / 16)).material;
        if (mat == MAT_SAND || mat == MAT_DUST) {
            const double gx = (site.groundHeight(x + 2, z) - site.groundHeight(x - 2, z)) / 4, gz = (site.groundHeight(x, z + 2) - site.groundHeight(x, z - 2)) / 4;
            const double slope = std::sqrt(gx * gx + gz * gz);
            const double vx = inBuggy ? std::sin(buggy.heading) * buggy.speed : player.vx, vz = inBuggy ? std::cos(buggy.heading) * buggy.speed : player.vz;
            const double down = slope > 1e-6 ? std::max(0.0, -(vx * gx + vz * gz) / slope) : 0.0;   // m/s down the face
            const double slide = clampd((0.35 * std::hypot(vx, vz) + 0.65 * down) / (inBuggy ? 6.0 : 2.5), 0, 1);
            if (slope > 0.03 && slide > 0.01) dune = slide * smoothstep(0.15, 0.5, sampleSurface(g, site.unitAt(x, z), 4).slipFace);   // the near ring's 4 m: every world's dunes whole
        }
    }
    duneSong = std::max(dune, duneSong * std::exp(-dt / 2.5));
    if (duneSong < 0.002) duneSong = 0;
    double ice = 0;
    if (g.hasTrait(TR_SINGING_ICE) && site.atmosphere && !inDrone) {
        const int cx = (int)std::floor(x / 16), cz = (int)std::floor(z / 16);
        double sum = 0; int n = 0;
        for (int dz = -4; dz <= 4; dz++)
            for (int dx = -4; dx <= 4; dx++) {
                if (dx * dx + dz * dz > 16) continue;
                const int m = site.lod0.at(cx + dx, cz + dz).material;
                sum += m == MAT_ICE ? 1.0 : (m == MAT_SNOW ? 0.5 : 0.0); n++;
            }
        const double share = sum / n;
        if (share > 0.05) {
            const double day = std::max(600.0, std::fabs(site.sys->bodies[site.body].rotPeriod));
            const double rate = (site.sun(t + 30).dirLocal.y - site.sun(t - 30).dirLocal.y) / 60.0 * day / TAU;   // -1..1: falling at dusk
            ice = smoothstep(0.05, 0.6, share) * (0.25 + 0.75 * clampd(std::fabs(rate) * (rate < 0 ? 1.0 : 0.7), 0, 1));
        }
    }
    iceSong += (ice - iceSong) * (1 - std::exp(-dt / 1.5));
    if (iceSong < 0.002 && ice == 0) iceSong = 0;
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
        if (e.absolute) {   // T-03: a dam's wall, an aqueduct's arches: their heights are the reference level's
            Form f = e; f.y0 = 0; f.y1 = e.y1 - e.y0; f.absolute = false;
            formColliders(f, discs);
            for (const FormDisc& q : discs) out.push_back({r.x + q.x, r.z + q.z, q.r, 3, e.y0 + q.y0, e.y0 + q.y1});
            continue;
        }
        formColliders(e, discs);
        if (discs.empty()) continue;
        double gy, skirt;   // B-420: the piece's foot where it is drawn (a big one on the highest ground under it, its sides down to the lowest); none
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
    for (const RuinCell& c : placedCells) take(c.r, c);   // A-02 (tests)
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
void rockParams(int type, double& density, double& scale) { density = KINDS[type].rockDensity; scale = KINDS[type].rockScale; }   // P-00: the kind's boulders
}   // namespace
int SurfaceView::forRocks(int cx, int cz, const std::function<bool(double, double, double)>& keep, const std::function<void(const RockInst&)>& fn) {
    const double cs = 16;
    const int type = site.sys->bodies[site.body].type;
    double rockDensity, rockScale; rockParams(type, rockDensity, rockScale);
    const uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xB0B);
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    double rd = rockDensity;
    if (livingGround(type) && (tv.material == MAT_GRASS || tv.material == MAT_ROCK) && tv.biome != BIO_WETLAND) rd = 0.7;   // R-306: open ground carries more stone
    rd *= 1 + 5.0 * tv.scree / 255.0;   // O6-04: boulder fields on the scree under the cliffs
    int nrocks = (int)(rd * 2.0 * hash01(h));
    if (livingGround(type) && hash01(mix64(h + 5)) < 0.06 && tv.material != MAT_FOREST) nrocks += 3;   // R-306: an outcrop of three
    if (tv.material == MAT_SAND || tv.material == MAT_WATER || tv.material == MAT_LAVA || tv.material == MAT_MATS || tv.material == MAT_STROMATOLITE) nrocks = 0;   // P-02: nor on the mats' sediment
    else if ((tv.material == MAT_BRINE || tv.material == MAT_SALT) && (kindHas(type, KT_VENT_LIFE) || site.gen.hasTrait(TR_GYPSUM) || site.gen.hasTrait(TR_HOT_SPRINGS))) nrocks = 0;   // P-03: nor in a spring or on its sinter; Q-02: nor on the gypsum's dunes, a hot spring's terraces
    if (tv.material == MAT_GLASS) nrocks = (int)(nrocks * 0.3);   // S-03: a few blast-thrown boulders on the sheets
    if (nrocks <= 0) return 0;
    const RuinCell* near[SETTLEMENTS_NEAR]; int nNear = -1;   // C-08: no rock in a room (the settlements near, found once a cell when a rock asks)
    for (int i = 0; i < nrocks; i++) {
        const uint64_t hr = mix64(h + 17 * (i + 1));
        RockInst R;
        R.x = cx * cs + hash01(hr) * cs; R.z = cz * cs + hash01(mix64(hr + 1)) * cs; R.h = hr;
        R.size = rockScale * (0.35 + 2.0 * std::pow(hash01(mix64(hr + 2)), 2.5)) * (1 + 0.6 * tv.scree / 255.0);   // O6-04: bigger on the scree
        if (keep && !keep(R.x, R.z, R.size)) continue;
        if (nNear < 0) nNear = settlementsNear(cx * cs + 8, cz * cs + 8, near);
        { bool in = false; for (int q = 0; q < nNear && !in; q++) in = inSettlementBuilding(*near[q], R.x, R.z, 1.0); if (in) continue; }
        if (!site.roads.empty() && site.roadCover(R.x, R.z) > 0.3) continue;   // C-09: no rock on the road's bed
        if (!site.marks.empty() && site.markCover(R.x, R.z) > 0.3) continue;   // T-02: nor on a street
        if (!site.edits.empty() && site.editCover(R.x, R.z) > 0.3) continue;   // T-03: nor on a work's ground
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
    const bool microbial = kindHas(site.gen.type, KT_MICROBIAL), ventLife = kindHas(site.gen.type, KT_VENT_LIFE);   // P-02, P-03
    const bool meteorites = site.gen.hasTrait(TR_METEORITES);   // Q-02
    double cs = 16;
    int pcx = (int)std::floor(x / cs), pcz = (int)std::floor(z / cs);
    // C-01: a settlement's standing pieces as rows of discs along their walls (within 40 m)
    forNearbyRuins(x, z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        settlementColliders(r, cell, x, z, out);
    });
    for (const RuinCell& c : placedCells) settlementColliders(c.r, c, x, z, out);   // A-02 (tests)
    for (int cz = pcz - 1; cz <= pcz + 1; cz++)
        for (int cx = pcx - 1; cx <= pcx + 1; cx++) {
            forRocks(cx, cz, [](double, double, double size) { return size >= 0.6; },   // a smaller one is stepped over
                     [&](const RockInst& R) { out.push_back({R.x, R.z, R.size * 0.8, 0, R.gy - R.size * 0.15, R.gy + R.size * 1.15}); });   // the peak stands up to 1.3 sizes over a base sunk 0.15
            // N2: trunks and fallen logs from the same enumerators that draw them
            forTrees(cx, cz, [&](const TreeInst& T) { out.push_back({T.x, T.z, 0.12 + (T.stump ? T.r * 0.14 : T.h * 0.02), 1, T.gy - 0.3, T.gy + T.h}); });   // P-04: a stone stump's girth
            if (microbial) forDomes(cx, cz, [&](const RockInst& R) { if (R.size >= 0.6) out.push_back({R.x, R.z, R.size * 0.85, 0, R.gy - 0.2, R.gy + 0.55 * R.size + 0.2}); });   // P-02
            if (ventLife) forChimneys(cx, cz, [&](const RockInst& R) { out.push_back({R.x, R.z, R.size, 0, R.gy - 0.1, R.gy + chimneyHeight(R.h)}); });   // P-03
            if (meteorites) forMeteorites(cx, cz, [&](const RockInst& R) { if (R.size >= 0.6) out.push_back({R.x, R.z, R.size * 0.9, 0, R.gy - 0.1, R.gy + 0.8 * R.size}); });   // Q-02
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
    if (site.escapeVelocity < 30) return false;   // O4: on a comet the buggy would float off at the first bump
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
        if (in.moveY != 0) fwd = in.moveY;   // M6-02: an analogue stick; R-418: each axis on its own
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
    if (gRel > 1.5) speed *= 0.85;           // heavy worlds (M8-09)
    if (site.escapeVelocity < 30) speed = std::min(speed, 0.3 * site.escapeVelocity);   // O4 microgravity: any faster and you would leave the ground
    if (p.crouch) speed *= 0.5;
    if (p.swimming) speed *= 0.45 * (p.swimStamina > 0.5 ? 1.0 : 0.5);
    p.fovKick = 4.0 * p.sprintRamp;
    double sy = std::sin(p.yaw), cy = std::cos(p.yaw);
    double tx = (sy * fwd + cy * str), tz = (cy * fwd - sy * str);
    double tl = std::sqrt(tx * tx + tz * tz);
    if (tl > 1 && p.autoWalk == 0) { tx /= tl; tz /= tl; }
    // B-420: what the explorer may stand on beside the ground: the heaps, banks, barrows and tiers near it (inside one, as after a landing on it,
    // it stands on its top)
    collectSupports(p.x, p.z);
    auto footing = [&](double x, double z, bool* blocked = nullptr) { bool in = false; double f = floorAt(x, z, p.y, &in); if (in && blocked) *blocked = true; return f; };
    bool insideHere = false; floorAt(p.x, p.z, p.y, &insideHere);
    // wading (M8-08): slower with depth until swimming takes over
    double groundHere = insideHere ? floorAt(p.x, p.z, 1e9) : footing(p.x, p.z);
    double waterHere = site.waterNowAt(p.x, p.z);   // Q-02: the tide's
    double depthHere = waterHere > -1e8 ? clampd(waterHere - groundHere, 0, 1.2) : 0;
    if (!p.swimming && p.onGround && depthHere > 0) speed *= 1.0 - 0.55 * depthHere / 1.2;   // B-406: wading, not flying over
    double accel = p.onGround ? (gRel > 1.5 ? 6.0 : 10.0) : (g < 3.0 ? 4.0 : 2.0);   // air control (M8-09)
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
    // not diving), so a lake is crossed and left by air; it used to be a trap
    p.jetOn = false;
    bool afloat = p.swimming && !p.diving && !p.underwater;
    if (controlsEnabled && in.isDown(KEY_SPACE) && (!p.onGround || afloat) && (!p.swimming || afloat) && p.jetHeat < 100 && p.altAboveGround < 300) {
        p.jetOn = true;
        p.vy += (g + 3.0) * dt;
        double vcap = site.escapeVelocity < 30 ? 0.35 * site.escapeVelocity : 12.0;   // O4: never toward escape velocity
        if (p.vy > vcap) p.vy = vcap;
        p.jetHeat = std::min(100.0, p.jetHeat + dt * 5.0);
    } else p.jetHeat = std::max(0.0, p.jetHeat - dt * 8.0);
    // O4: on light worlds Ctrl in the air fires the thrusters downward (a jump on a comet floats for minutes otherwise)
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
    // B-420: a heap's, a bank's or a tier's side more than a step high, or steeper than the ground's climb, is walked into, not up: the move
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
    // collision with rocks and trunks (M8-05)
    {
        static std::vector<Collider> cols;
        collectColliders(nx, nz, cols);
        for (const Collider& c : cols) {
            if (p.y > c.y1 - 0.1 || p.y + 1.6 < c.y0) continue;   // B-405: risen above it (the jetpack, a slope) or under it
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
    double ground = insideNow ? floorAt(p.x, p.z, 1e9) : floorAt(p.x, p.z, p.y);   // B-420: the footing (a heap, a bank, a tier), the ground else
    double water = site.waterNowAt(p.x, p.z);
    // B-406 (2026-10-02): deep water is swum once the body is down at its surface; above it the air is the air, so the jetpack
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
        if (!wasOnGround && !p.swimming && p.vy < -3.0) p.landDip = std::min(0.4, -p.vy * 0.045 * (gRel > 1.5 ? 1.4 : 1.0));   // landing dip (M8-01)
        p.y = floor;
        if (p.vy < 0) p.vy = 0;
        p.onGround = true;
    } else {
        p.onGround = p.y - floor < 0.02;
    }
    if (p.swimming) {
        p.onGround = true;
        if (p.diving) { p.vy = std::max(p.vy, -1.5); p.y = std::max(p.y - 1.5 * dt, std::max(ground + 0.3, water - 3.5)); }
        else if (!p.jetOn) { p.vy = std::max(p.vy, -1.0); p.y += (floor - p.y) * (1 - std::exp(-dt * 4)); }   // B-406: the jet lifts out of the water
    }
    p.underwater = p.swimming && water > -1e8 && (p.y + p.eyeHeight < water - 0.05);
    p.altAboveGround = std::max(0.0, p.y - ground);
    // head bob and sway (M8-01)
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

// ---------------------------------------------------------------------------
// The buggy (M8-06): arcade driving on the height field
// ---------------------------------------------------------------------------

// R-402: the aurora's colours per world: the lower curtain's and its tops', by the air and a hash (oxygen airs green with red or
// violet tops, as on Earth; thin air green or teal under violet; a desert's lime under pink; a sulphurous sky blue under
// magenta; an acid sky gold under orange)
static void auroraColours(const Body& b, RGB& low, RGB& high) {
    double u = unitFromHash(hashCombine(b.seed, 0xA0C));
    const RGB green(0.1f, 0.45f, 0.2f), teal(0.1f, 0.42f, 0.36f), lime(0.3f, 0.5f, 0.12f), blue(0.15f, 0.3f, 0.7f), gold(0.55f, 0.48f, 0.12f);
    const RGB red(0.55f, 0.12f, 0.16f), violet(0.4f, 0.18f, 0.6f), pink(0.6f, 0.22f, 0.4f), magenta(0.6f, 0.12f, 0.5f), orange(0.7f, 0.3f, 0.08f);
    switch (KINDS[b.type].aurora) {   // P-00: the kind's style
        case AU_THIN: low = u < 0.5 ? green : teal; high = violet; break;
        case AU_DESERT: low = u < 0.5 ? lime : green; high = pink; break;
        case AU_SULPHUR: low = blue; high = magenta; break;
        case AU_ACID: low = gold; high = orange; break;
        default: low = u < 0.7 ? green : (u < 0.85 ? teal : lime); high = u < 0.6 ? red : (u < 0.85 ? violet : pink); break;
    }
}

void SurfaceView::setupPalette(Framebuffer& fb) {
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    double day = env.skyBrightness;
    double sinAlt = env.sun.dirLocal.y;
    RGB white(1, 1, 1);
    RGB starC = lerp(env.lightColor, white, 0.5f);   // M5-01: both suns tint the light
    RGB zen = lerp(L.nightZenith, L.zenith, (float)day);
    RGB hor = lerp(L.nightHorizon, L.horizon, (float)day);
    // sunset tint near the horizon
    double sunset = (1 - smoothstep(0.02, 0.22, sinAlt)) * smoothstep(-0.16, -0.03, sinAlt);
    if (site.atmosphere) {
        RGB sunsetC = RGB(0.95f, 0.45f, 0.2f) * lerp(env.lightColor, white, 0.5f);
        if (kindHas(site.gen.type, KT_EXOTIC_TWILIGHT)) sunsetC = lerp(sunsetC, site.gen.skyTint * 1.3f, 0.65f);   // exotic twilight (M1-07)
        hor = lerp(hor, sunsetC, (float)(sunset * 0.8));
        zen = lerp(zen, sunsetC * 0.4f, (float)(sunset * 0.25));
    }
    double rainDim = (1 - 0.35 * env.rain) * (1 - 0.3 * env.frontCloud);   // W-03: a front's deck is a storm's: the sky's colours dim under it
    hor = hor * (float)rainDim; zen = zen * (float)rainDim;
    RGB fog = site.atmosphere ? hor : lerp(L.ground, white, 0.55f);
    RGB glow = lerp(hor, starC, 0.5f);
    RGB sunC = site.atmosphere ? lerp(starC, white, 0.6f) : lerp(env.sunDisc.color, white, 0.35f);
    if (env.hasSun2) {   // M5-01 bank 12: the companion's disc and glow in its own colour
        RGB kc = env.sun2.color;
        RGB kd = site.atmosphere ? lerp(lerp(kc, white, 0.5f), white, 0.6f) : lerp(kc, white, 0.35f);
        setRamp(fb.pal, 12, {{0, RGB(0, 0, 0)}, {20, kc * 0.45f}, {44, kc}, {56, kd}, {63, lerp(kd, white, 0.5f)}});
    }
    if (env.moonBody >= 0 && kindHas(site.sys->bodies[env.moonBody].type, KT_GLOWS)) {   // M5-02 bank 13: a glowing parent in the sky
        RGB g = site.sys->bodies[env.moonBody].color;
        setRamp(fb.pal, 13, {{0, RGB(0, 0, 0)}, {14, g * 0.4f}, {34, g}, {50, lerp(g, RGB(1, 0.55f, 0.25f), 0.6f)}, {63, RGB(1, 0.8f, 0.5f)}});
    }
    float lf = (float)clampd(env.sun.lightFactor, 0.5, 1.15 * (1 + 0.25 * env.flare));   // S-01: a flare opens the exposure a quarter past its clamp
    // M10-07: the dark end of a material ramp takes the sky's colour (hemisphere light), the lit end
    // is warmed at sunset by the low sun, and an overcast sky flattens the contrast
    RGB skyLight = lerp(zen, hor, 0.5f);
    double overcast = site.atmosphere ? env.cloudCover : 0.0;
    RGB sunsetTint = lerp(RGB(1, 1, 1), RGB(1.15f, 0.72f, 0.5f), (float)(site.atmosphere ? sunset * 0.7 : 0.0));
    auto matRamp = [&](int bank, RGB c) {
        // N1-05: the shared material ramp (hemisphere light included), then the surface's own terms: moonlight
        // in the dark end, the sunset tint and the overcast flattening in the lit end, the fog colour at the top
        MatRamp r = materialRamp(c, site.atmosphere && env.pressureAtm >= 0.001 ? &skyLight : nullptr, day);   // P-05: an air of millionths of a bar lights nothing
        RGB dark = r.stop[1];
        // planetshine lifts the dark end at night only (N1-05: on airless worlds `day` is the sky, which is always dark, so use the sun)
        double night = site.atmosphere ? (1 - day) : (1 - smoothstep(-0.05, 0.1, sinAlt));
        if (env.moonLight > 0.01) dark = lerp(dark, env.moonColor * (0.22f + 0.3f * (float)env.moonLight), (float)(night * clampd(env.moonLight * 2.5, 0, 0.7)));
        RGB lit = c * sunsetTint * (float)(1.0 - 0.12 * overcast);
        RGB mid = lerp(dark, lit, 0.55f);
        setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {14, dark}, {34, mid}, {47, lit}, {56, lerp(lit, white, 0.5f)}, {63, fog}});
    };
    matRamp(0, L.ground);
    if (lavaGlow) {
        setRamp(fb.pal, 2, {{0, RGB(0, 0, 0)}, {14, RGB(0.16f, 0.03f, 0.01f)}, {26, RGB(0.42f, 0.06f, 0)}, {40, RGB(0.8f, 0.14f, 0)}, {52, RGB(1, 0.5f, 0.05f)}, {60, RGB(1, 0.9f, 0.5f)}, {63, RGB(1, 1, 0.85f)}});
    } else if (site.hasWater) {   // R-307: the sea's ramp for every liquid (methane, acid, brine), not the living worlds' alone
        float dayF = 0.25f + 0.75f * (float)day;   // B-320: the water goes dark with the sky (the shade no longer carries the night)
        RGB w = L.secondary * (0.7f + 0.3f * lf) * dayF;
        RGB wb = lerp(hor, white, 0.2f);
        setRamp(fb.pal, 2, {{0, RGB(0, 0, 0)}, {16, w * 0.3f}, {34, w * 0.7f}, {46, w}, {56, lerp(w, wb, 0.6f)}, {63, fog}});
    } else {
        matRamp(2, L.secondary);
    }
    matRamp(3, L.tertiary);
    if (site.gen.hasTrait(TR_LUMINOUS_FLORA) && day < 0.5) {   // R-304: the leaves carry their own light at night
        RGB g3 = L.tertiary * 1.3f, gl = lerp(g3, RGB(0.6f, 1.0f, 0.7f), 0.35f);
        float k = (float)(1 - day * 2);
        setRamp(fb.pal, 3, {{0, RGB(0, 0, 0)}, {14, lerp(L.tertiary * 0.25f, g3 * 0.5f, k)}, {34, lerp(L.tertiary * 0.6f, g3, k)}, {47, lerp(L.tertiary, gl, k)}, {56, lerp(L.tertiary, white, 0.5f)}, {63, white}});
    }
    // N1-03 family banks: snow/ice 8 (bank 2 stays water-only for the reflection pass), sand/dust/sulphur/quartz 9, grass 10
    matRamp(8, L.snow);
    matRamp(9, L.sand);
    matRamp(10, L.grass);
    // N4-01 the buggy: bank 6 its hull (the ship's grey-blue), bank 7 chrome (tubes, rims, dials), bank 14 warning red (tail lights, needles)
    matRamp(6, RGB(0.52f, 0.55f, 0.6f));
    setRamp(fb.pal, 7, {{0, RGB(0, 0, 0)}, {20, RGB(0.42f, 0.44f, 0.5f)}, {44, RGB(0.82f, 0.84f, 0.88f)}, {63, white}});
    setRamp(fb.pal, 14, {{0, RGB(0, 0, 0)}, {14, RGB(0.3f, 0.05f, 0.03f)}, {40, RGB(0.9f, 0.15f, 0.1f)}, {63, RGB(1, 0.65f, 0.5f)}});
    {   // N2-05 bank 5: the second flora family's colour; deciduous leaves turn in the cold shoulder seasons
        double ph = seasonPhaseLocal();
        RGB f2 = (ph < 0 && ph >= -0.6) ? lerp(site.gen.vegColor, RGB(0.85f, 0.5f, 0.12f), 0.65f) * lerp(site.sys->star.color, white, 0.5f) : L.flora2;
        matRamp(5, f2);
    }
    {   // M4-06 aurora: bank 11 its lower colour, bank 21 (R-402) its tops' colour, per world; B-403: their dark end is the night
        // sky's own colour, so the curtains fade into the sky without a hue edge (a thin atmosphere's night sky is brown, and
        // black-green against it cut the curtains' faint top into patches)
        RGB low, high; auroraColours(site.sys->bodies[site.body], low, high);
        setRamp(fb.pal, 11, {{0, zen}, {10, lerp(zen, low, 0.4f)}, {30, low}, {63, lerp(low, white, 0.55f)}});
        setRamp(fb.pal, 21, {{0, zen}, {10, lerp(zen, high, 0.4f)}, {30, high}, {63, lerp(high, white, 0.45f)}});
    }
    {   // B-315 bank 16: bark, a brown of the planet's own (toward the second flora colour); bank 17: cut wood, pale. The
        // trunks used to sit on the ground bank, whose dark stops take the sky's colour: black trunks, grey-blue branches
        RGB bark = lerp(RGB(0.36f, 0.25f, 0.15f), site.gen.vegColor2 * 0.55f, 0.3f), wood(0.74f, 0.62f, 0.44f);
        const int cover = KINDS[site.gen.type].cover;   // P-04: a frozen world's dead wood white with frost, a scorched world's turned to stone
        if (cover == DC_FROZEN) { bark = RGB(0.80f, 0.84f, 0.90f); wood = RGB(0.86f, 0.88f, 0.92f); }
        if (cover == DC_SCORCHED) { bark = RGB(0.58f, 0.48f, 0.40f); wood = RGB(0.74f, 0.56f, 0.46f); }
        if (site.gen.hasTrait(TR_FUNGAL)) { bark = lerp(site.gen.matColor[MAT_FOREST], RGB(0.92f, 0.90f, 0.86f), 0.6f); wood = RGB(0.90f, 0.86f, 0.78f); }   // Q-02: the stems pale
        matRamp(16, bark);
        matRamp(17, wood);
    }
    // T-04 banks 29-31: the records' (galaxy/records.h): bone, parchment and paper pale; wax, leather, vinyl and dark glass; a painting's ochre
    // (clay is the cut wood's bank 17, metal the chrome's 7)
    matRamp(29, RGB(0.86f, 0.82f, 0.7f));
    matRamp(30, RGB(0.2f, 0.16f, 0.13f));
    matRamp(31, RGB(0.66f, 0.3f, 0.15f));
    {   // A-02: the peoples' palettes (galaxy/forms.h): the structure, the accent and metal and glass through the material ramp (the
        // structure the ground's own where the people built of it: C-01's walls of the rock, or the sand of adobe, the same ramps
        // as banks 0 and 9), light a ramp of its own that glows whatever the sun (its top the fog's, as every ramp's)
        RGB lcol = lerp(site.sys->star.color, white, 0.5f);
        for (int p = 0; p < 2; p++) {
            const FormPalette& P = palettes[p];
            // A-04: a slot of the ground's own tinted toward the people's colour (`tint`); slot 3 light only where the palette glows (the
            // futuristic and the far future's), else a material (timber, rust)
            for (int k = 0; k < 3; k++) matRamp(FORM_BANKS[p][k], P.own[k] ? lerp(cultures[p].family == 1 ? L.sand : L.ground, P.colour[k] * lcol, P.tint[k]) : P.colour[k] * lcol);
            RGB c = P.colour[3];
            if (P.glow) setRamp(fb.pal, FORM_BANKS[p][3], {{0, RGB(0, 0, 0)}, {24, c * 0.3f}, {46, c}, {56, lerp(c, white, 0.45f)}, {63, fog}});
            else matRamp(FORM_BANKS[p][3], c * lcol);
        }
    }
    if (site.gen.type == PT_COMET) setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {18, RGB(0.10f, 0.14f, 0.30f)}, {40, RGB(0.32f, 0.48f, 0.85f)}, {63, RGB(0.7f, 0.8f, 1.0f)}});   // O4 bank 15: the ion tail overhead
    else if (KINDS[site.gen.type].events & EV_SPRINGS) setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {20, RGB(0.06f, 0.16f, 0.13f)}, {44, RGB(0.36f, 0.88f, 0.74f)}, {63, RGB(0.80f, 1.0f, 0.94f)}});   // P-03 bank 15: the spores' own light
    {   // Q-02 (after the peoples' banks: it takes the second people's on worlds that have none): the water's trait colours, each a water's ramp in its own colour (a pink lake's, a bloom's, a lagoon's), and the milky sea's glow at
        // night (its dark end the night sea's, its light the bacteria's pale blue)
        tintBankW = tintBankL = -1; tintGlow = 0;
        const float dayF = 0.25f + 0.75f * (float)day;
        auto waterTint = [&](int bank, RGB c) {
            RGB w = c * (0.7f + 0.3f * lf) * dayF, wb = lerp(hor, white, 0.2f);
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {16, w * 0.3f}, {34, w * 0.7f}, {46, w}, {56, lerp(w, wb, 0.6f)}, {63, fog}});
        };
        // the peoples' banks are the traits' on a world without a people (its traits that take them move its ground: never on a people's), but
        // where the harness stages a people's forms (A-02's tests) they keep theirs
        const bool formsStaged = !placedCells.empty();
        if (site.gen.hasTrait(TR_PINK_LAKES) && !formsStaged) { tintBankW = 25; waterTint(25, site.gen.waterTintColour); }
        tintBankSprings = -1;
        if (site.gen.hasTrait(TR_HOT_SPRINGS) && !formsStaged) { tintBankSprings = 25; matRamp(25, site.gen.matColor[MAT_MATS] * lerp(site.sys->star.color, white, 0.5f)); }   // the springs' mats
        if (site.gen.hasTrait(TR_MILKY_SEAS)) {
            tintGlow = 1 - smoothstep(0.04, 0.3, day);
            if (tintGlow > 0.02) {
                tintBankW = 15;
                const RGB m = site.gen.waterTintColour, dark = L.secondary * 0.25f * dayF;
                setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {14, dark}, {28, lerp(dark, m * 0.45f, (float)tintGlow)}, {44, lerp(dark, m * 0.85f, (float)tintGlow)}, {63, lerp(dark, lerp(m, white, 0.35f), (float)tintGlow)}});
            }
        }
        if (waterLights) {   // D-04: a dark ocean's life, its own light: faint in the black water, in patches that drift (the sheet's)
            tintGlow = 1; tintBankW = 15;
            const RGB m = site.gen.waterTintColour, dark = L.secondary * 0.25f * dayF;
            setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {14, dark}, {28, lerp(dark, m * 0.3f, 0.8f)}, {44, m * 0.62f}, {63, lerp(m, white, 0.25f)}});
        }
        if (site.gen.hasTrait(TR_LIGHTS_LEFT_ON)) setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {22, RGB(0.30f, 0.20f, 0.08f)}, {44, RGB(0.86f, 0.62f, 0.30f)}, {58, RGB(1.0f, 0.86f, 0.56f)}, {63, RGB(1, 0.97f, 0.86f)}});   // D-04: the lamps' and the windows' light
        if ((site.gen.hasTrait(TR_BLOOMS) || site.gen.hasTrait(TR_REEFS)) && !formsStaged) { tintBankL = 26; waterTint(26, site.gen.hasTrait(TR_BLOOMS) ? lerp(site.gen.matColor[MAT_WATER], site.gen.lifeTintColour, 0.75f) : site.gen.lifeTintColour); }   // a bloom the water's own colour turned
        tintBankStrata = -1;
        if (site.gen.hasTrait(TR_STRATA) && !formsStaged) {   // the rainbow strata's accent: a red, an orange or a green by the world
            static const RGB A[3] = {RGB(0.74f, 0.30f, 0.20f), RGB(0.82f, 0.52f, 0.20f), RGB(0.46f, 0.56f, 0.36f)};
            tintBankStrata = 27; matRamp(27, A[(int)(hash01(mix64(site.gen.seed ^ 0x57A7BULL)) * 3) % 3] * lerp(site.sys->star.color, white, 0.5f));
        }
        if (site.gen.hasTrait(TR_PETRIFIED) && !formsStaged) {   // the stone trunks' jasper: red-brown, ochre or a purple grey by the world
            static const RGB J[3] = {RGB(0.62f, 0.32f, 0.22f), RGB(0.70f, 0.52f, 0.28f), RGB(0.54f, 0.42f, 0.52f)};
            matRamp(26, J[(int)(hash01(mix64(site.gen.seed ^ 0x9E7AULL)) * 3) % 3] * lerp(site.sys->star.color, white, 0.5f));
        }
    }
    // bank 4: stars (their own ramp, no longer borrowed from the ground); D-04: reddened through a thick air (the share it lets through)
    {
        const float red = site.atmosphere ? (float)(1.0 - KINDS[site.gen.type].starLight) : 0.0f;
        setRamp(fb.pal, 4, {{0, RGB(0, 0, 0)}, {40, lerp(RGB(0.55f, 0.57f, 0.66f), RGB(0.66f, 0.46f, 0.36f), red)}, {63, lerp(RGB(0.88f, 0.9f, 0.97f), RGB(0.98f, 0.80f, 0.64f), red)}});
    }
    {   // G-03 bank 22: the galactic band and the nebula patches, the unresolved starlight, laid over the sky at night by
        // `drawBand`: from the night sky's own colour at the dark end (no hue edge where it fades into the sky) to a pale
        // star-grey, a little of the horizon's tint through an atmosphere
        RGB pale = lerp(RGB(0.55f, 0.57f, 0.66f), hor, site.atmosphere ? 0.25f : 0.0f);
        if (site.sys->star.cls == STAR_PROTOSTAR) {   // S-04: the sky's glow takes the protostar's cloud's tone
            int tone = starNebulaTone(site.sys->star);
            pale = lerp(pale, tone == 0 ? RGB(0.45f, 0.55f, 0.85f) : (tone == 1 ? RGB(0.85f, 0.50f, 0.45f) : RGB(0.70f, 0.70f, 0.75f)), 0.6f);
        }
        // B-408: an airless sky lies in this bank all over, and the sun's disc and glow are drawn in it (`drawSky`): the band's
        // own shades (under 20) keep the star-grey, the glow round the sun turns to the sun's colour above them, and from 55 up
        // the stops are the sky bank's, so the disc is the colour it was
        if (site.atmosphere || site.sys->star.cls == STAR_PROTOSTAR) setRamp(fb.pal, 22, {{0, site.atmosphere ? zen : RGB(0, 0, 0)}, {40, pale}, {63, lerp(pale, white, 0.4f)}});
        else {
            auto lumOf = [](RGB c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; };
            RGB sunN = sunC * (lumOf(pale) / std::max(0.05f, lumOf(sunC)));   // the sun's hue at the pale grey's light
            setRamp(fb.pal, 22, {{0, RGB(0, 0, 0)}, {20, pale * 0.5f}, {40, lerp(pale, sunN, 0.7f)}, {55, sunC * 0.9f}, {60, sunC}, {63, lerp(sunC, white, 0.45f)}});
        }
    }
    // sky bank: zenith .. horizon .. glow .. sun/star white
    if (site.atmosphere) {
        setRamp(fb.pal, 1, {{0, zen}, {40, hor}, {50, glow}, {56, lerp(glow, sunC, 0.6f)}, {63, sunC}});
        // at night the upper part must still serve stars: brighten toward white
        if (day < 0.5) {
            RGB starWhite = RGB(0.8f, 0.82f, 0.9f);
            setRamp(fb.pal, 1, {{0, zen}, {36, hor}, {44, lerp(hor, starWhite, 0.25f)}, {54, lerp(hor, starWhite, (float)(0.6 + 0.3 * day))}, {63, lerp(starWhite, sunC, (float)day)}});
        }
        // W-03 bank 23: a front's deck and the rain under it. The sky bank runs from the zenith's colour to the horizon's, and on
        // a world whose two are close (a red dwarf's mauve) nothing drawn in it can be dark; this ramp shares the sky's stops
        // from the horizon up (the lit tops look the same in either bank) and goes down to a storm's grey instead of the zenith
        {
            RGB stormGrey = RGB(0.40f, 0.40f, 0.44f) * (float)rainDim;
            RGB g1 = lerp(zen, stormGrey, (float)day), g0 = g1 * 0.6f;
            if (day < 0.5) setRamp(fb.pal, 23, {{0, g0}, {18, g1}, {36, hor}, {44, lerp(hor, RGB(0.8f, 0.82f, 0.9f), 0.25f)}, {54, lerp(hor, RGB(0.8f, 0.82f, 0.9f), (float)(0.6 + 0.3 * day))}, {63, lerp(RGB(0.8f, 0.82f, 0.9f), sunC, (float)day)}});
            else setRamp(fb.pal, 23, {{0, g0}, {18, g1}, {36, lerp(g1, hor, 0.8f)}, {40, hor}, {50, glow}, {56, lerp(glow, sunC, 0.6f)}, {63, sunC}});
        }
    } else {
        RGB grey(0.42f, 0.43f, 0.48f);
        setRamp(fb.pal, 1, {{0, RGB(0, 0, 0)}, {30, RGB(0.05f, 0.05f, 0.07f)}, {44, lerp(grey, sunC, 0.3f) * 0.8f}, {50, lerp(grey, sunC, 0.5f)}, {55, sunC * 0.9f}, {60, sunC}, {63, lerp(sunC, white, 0.45f)}});
    }
    if (lightningFlash > 0) {
        for (int i = 0; i < (int)sizeof(fb.pal); i++) fb.pal[i] = (uint8_t)std::min(255.0, fb.pal[i] + 120 * lightningFlash);
    }
    if (env.dust > 0.05) {
        // M4-06 dust storm: everything goes the colour of the dust, the sky included
        RGB dc = site.gen.color * 0.85f;
        for (int i = 0; i < (int)sizeof(fb.pal); i += 3) {
            double f = 0.7 * env.dust;
            fb.pal[i] = (uint8_t)(fb.pal[i] * (1 - f) + dc.r * 255 * f * 0.9); fb.pal[i + 1] = (uint8_t)(fb.pal[i + 1] * (1 - f) + dc.g * 255 * f * 0.9); fb.pal[i + 2] = (uint8_t)(fb.pal[i + 2] * (1 - f) + dc.b * 255 * f * 0.9);
        }
    }
    if (player.underwater || viewUnderwater) {
        // M8-08: everything seen through water goes blue-green and dim
        for (int i = 0; i < (int)sizeof(fb.pal); i += 3) {
            fb.pal[i] = (uint8_t)(fb.pal[i] * 0.35); fb.pal[i + 1] = (uint8_t)(fb.pal[i + 1] * 0.75 + 20); fb.pal[i + 2] = (uint8_t)std::min(255.0, fb.pal[i + 2] * 0.9 + 45);
        }
    }
}

void SurfaceView::materialLook(const TerrainVertex& v, uint8_t& bank, double& albedo) const {
    albedo = v.albedo;
    bank = 0;
    // N1-03: one bank per material family, the same families as the globe (P-00: a kind's accents in the grass and forest banks)
    if (tintBankSprings >= 0 && v.material == MAT_MATS) { bank = (uint8_t)tintBankSprings; return; }   // Q-02: the hot springs' mats in their own bank
    if (tintBankStrata >= 0 && v.gtint > 127) { bank = (uint8_t)tintBankStrata; return; }             // Q-02: a rainbow stratum's accent layer
    switch (matFamilyOn(site.gen.type, v.material)) {
        case FAM_WATER: bank = 2; break;
        case FAM_FOREST: bank = 3; if (v.material == MAT_FOREST) albedo = 0.28 + 0.1 * v.veg; break;
        case FAM_GRASS: bank = 10; if (v.material == MAT_GRASS) albedo = 0.36 + 0.2 * v.veg; break;
        case FAM_SNOW: bank = 8; if (v.material == MAT_SNOW) albedo = KINDS[site.gen.type].snowAlbedo; break;
        case FAM_SAND: bank = 9; break;
        default: bank = v.material == MAT_LAVA ? 2 : 0; break;
    }
}

const RVert& SurfaceView::vertexOf(TerrainCache& cache, std::vector<VtxCache>& vcache, int cx, int cz, uint8_t& bank) {
    int n = 1 << cache.logN;
    int idx = ((cz & (n - 1)) << cache.logN) | (cx & (n - 1));
    VtxCache& vc = vcache[idx];
    if (vc.stamp == frameStamp && vc.v.u == (double)(cx * cache.cellSize) && vc.v.v == (double)(cz * cache.cellSize)) {
        bank = vc.bank;
        return vc.v;
    }
    const TerrainVertex& tv = cache.at(cx, cz);   // F-04: after the hit test (a current vertex's cell was sampled when it was made)
    double cs = cache.cellSize;
    double x = cx * cs, z = cz * cs;
    double h = tv.h;
    if (tv.material == MAT_WATER && tv.water > -1e8f) h = std::min(h, (double)tv.water - 0.5);
    // B-313 geomorphing: within MORPH_CELLS of the ring's edge the vertex slides onto the coarser ring's surface (the
    // height of its drawn triangle at this spot, and its Gouraud shade when the coarser vertices were computed this
    // frame), by the vertex's continuous distance from the camera, so nothing pops as the edge sweeps the ground
    double morph = 0, hC = 0, shC = 0; bool shOk = false;
    double wy = site.waterOf(tv);   // B-320: the water level as drawn (Q-02: the tide's)
    double shore = vertexShore(cache, cx, cz);   // B-322: the side of the water's edge, morphed with the ground below
    if (morphCoarse && morphBand > 0) {
        double ddx = x - camPos.x, ddz = z - camPos.z;
        double dc = std::sqrt(ddx * ddx + ddz * ddz) / cs;
        morph = smoothstep(curRadius - morphBand, curRadius - 0.5, dc);
        if (morph > 0) {
            double csC = morphCoarse->cellSize;
            CoarseHit ch = coarseAt(*morphCoarse, x, z);
            int X = ch.X, Z = ch.Z; double fx = ch.fx, fz = ch.fz; bool lower = ch.lower;
            hC = ch.h;
            h += (hC - h) * morph;
            if (wy > -1e8f) {   // B-320: the water slides with the ground
                double wc = coarseWaterAt(*morphCoarse, x, z);
                if (wc > -1e8) wy += (wc - wy) * morph;
                else {   // O6-03: the coarser ring has no water here (a stream too small for its scale): the vertex dries out toward the edge, its plane sinking under the coarse ground, so the clamp below cannot hold it under a plane the coarser ring never drew (a 27 m seam)
                    wy += (hC - 1.0 - wy) * morph;
                    if (shore < 1e8) shore += (std::max(1.0, std::fabs(shore)) - shore) * morph;
                }
            }
            if (shore < 1e8) { double sc = coarseShoreAt(*morphCoarse, x, z); if (sc < 1e8) shore += (sc - shore) * morph; }   // B-322: and so does the edge
            int lc = morphCoarse->logN, mC = (1 << lc) - 1;
            const VtxCache& c00 = (*morphVc)[((Z & mC) << lc) | (X & mC)];
            const VtxCache& c10 = (*morphVc)[((Z & mC) << lc) | ((X + 1) & mC)];
            const VtxCache& c01 = (*morphVc)[(((Z + 1) & mC) << lc) | (X & mC)];
            const VtxCache& c11 = (*morphVc)[(((Z + 1) & mC) << lc) | ((X + 1) & mC)];
            auto cur = [&](const VtxCache& vcc, int cxx, int czz) { return vcc.stamp == frameStamp && vcc.v.u == (double)(cxx * csC) && vcc.v.v == (double)(czz * csC); };
            if (cur(c00, X, Z) && cur(c10, X + 1, Z) && cur(c01, X, Z + 1) && cur(c11, X + 1, Z + 1)) {
                shC = lower ? c00.v.shade + fx * (c10.v.shade - c00.v.shade) + fz * (c01.v.shade - c00.v.shade)
                            : c11.v.shade + (1 - fx) * (c01.v.shade - c11.v.shade) + (1 - fz) * (c10.v.shade - c11.v.shade);
                shOk = true;
            }
        }
    }
    // normal from neighbours (the curvature of the far ring is applied by toView, O1)
    double hl = cache.at(cx - 1, cz).h, hr = cache.at(cx + 1, cz).h, hd = cache.at(cx, cz - 1).h, hu = cache.at(cx, cz + 1).h;
    Vec3 nrm = normalize(Vec3(hl - hr, 2 * cs, hd - hu));
    const Vec3& sd = env.sun.dirLocal;
    double lambert = std::max(0.0, dot(nrm, sd));
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double lf = env.sun.lightFactor;
    double cloud = site.atmosphere ? env.cloudCover : 0.0;
    // M10-07: hemisphere ambient (the sky lights faces that look up), stronger and flatter when overcast
    double ambient = site.atmosphere ? (0.08 + 0.2 * env.skyBrightness) : 0.11;
    ambient *= (0.55 + 0.45 * clampd(nrm.y, 0, 1)) * (1 + 0.6 * cloud);
    double sunTerm = lambert * sunUp * lf * (1 - 0.6 * cloud);
    // M10-06: cast shadows of the terrain itself (lod0 and lod1 only)
    if (sunTerm > 0.01 && cache.cellSize <= 64 && !hasOpaqueDeck(site.gen.type)) sunTerm *= castShadowLit(cache, cx, cz, h, sd);
    // M10-14: cloud shadows on the ground (the sky's cloud layer, offset along the sun ray)
    if (cloudShadows && sunTerm > 0.01 && sd.y > 0.05) {
        double up = (2600.0 - h) / sd.y;
        double px = x + sd.x * up + cloudWx, pz = z + sd.z * up + cloudWz;
        double n = 0.5 + 0.5 * fbm2(px / 1800.0, pz / 1800.0, site.gen.seed + 77, 3, 2.2, 0.55);
        double thr = cloudThr;
        if (localFront.on) thr = 1.0 - std::max(env.cloudPattern, localFront.cloud(x + sd.x * up, z + sd.z * up)) * 0.75;   // W-03: the front's deck shadows the ground under it
        sunTerm *= 1 - 0.55 * smoothstep(thr, thr + 0.18, n);
    }
    double light = ambient + (1 - ambient) * sunTerm;
    // M10-09 secondary lights: planetshine at night, the capsule beacon pool, lava glow
    if (env.moonLight > 0.005) light += (1 - ambient) * env.moonLight * 0.6 * std::max(0.0, dot(nrm, env.moonDir)) * (1 - sunUp);
    if (nLamps) light += (1 - ambient) * lampAt(Vec3(x, h, z), &nrm);   // D-04: the lamp, the floodlights
    if (buggy.deployed && buggy.lights) {
        // headlights: a 25 m cone ahead of the buggy
        double hx = std::sin(buggy.heading), hz = std::cos(buggy.heading);
        double rx = x - buggy.x, rz = z - buggy.z;
        double f = rx * hx + rz * hz, l = std::fabs(-rx * hz + rz * hx);
        if (f > 0 && f < 25 && l < 0.35 * f + 1.5) light += 0.55 * (1 - f / 25) * (1 - sunUp);
    }
    if (drone.deployed && drone.lights && drone.altAboveGround < 30) {   // R-403: the drone's bar lights a pool under and ahead of it when it flies low
        double hx = std::sin(drone.heading), hz = std::cos(drone.heading);
        double rx = x - drone.x, rz = z - drone.z;
        double f = rx * hx + rz * hz, l = std::fabs(-rx * hz + rz * hx), reach = 25 + drone.altAboveGround;
        if (f > -4 && f < reach && l < 0.35 * (f + 4) + 1.5) light += 0.5 * (1 - (f + 4) / (reach + 4)) * (1 - drone.altAboveGround / 30) * (1 - sunUp);
    }
    if (sunUp < 0.9) {
        double bdx = x - capsuleX, bdz = z - capsuleZ;
        double bd2 = bdx * bdx + bdz * bdz;
        if (bd2 < 14.0 * 14.0) { double f = 1 - std::sqrt(bd2) / 14.0; light += 0.35 * f * f * (0.7 + 0.3 * std::sin(lastT * 3.0)) * (1 - sunUp); }
    }
    double albedo; uint8_t bk;
    materialLook(tv, bk, albedo);
    double fog0 = 0;   // B-311: the tone tile fades with distance (the haze flattens far ground)
    { double ddx = x - camPos.x, ddz = z - camPos.z; fog0 = clampd(std::sqrt(ddx * ddx + ddz * ddz) / (site.atmosphere ? env.fogDistance : 20000.0), 0, 1); }
    if (tv.water > -1e8f && tv.material != MAT_WATER && h < tv.water + (tv.material == MAT_ICE ? 0.25 : 1.5)) albedo *= 0.8;   // wet shore (B-322: a hand's breadth on ice, or every floe was dark)
    double emissive = 0;
    if (lavaGlow && tv.material != MAT_LAVA && cache.cellSize <= 16) {
        // lava glow: the brightest lava cell within two cells lights the rock around it
        for (int oz = -2; oz <= 2; oz++)
            for (int ox = -2; ox <= 2; ox++) {
                if (!ox && !oz) continue;
                const TerrainVertex& nv = cache.at(cx + ox, cz + oz);
                if (nv.material != MAT_LAVA) continue;
                double d = std::sqrt((double)(ox * ox + oz * oz)) * cs;
                emissive = std::max(emissive, (nv.glow / 255.0) * (1 - d / 40.0));
            }
    }
    // M10-10 quantised speculars on ice, quartz and snow
    double spec = 0;
    if ((tv.material == MAT_ICE || tv.material == MAT_QUARTZ || tv.material == MAT_SNOW || tv.material == MAT_METAL || tv.material == MAT_GLASS) && sunUp > 0.05) {
        Vec3 toCam = normalize(Vec3(camPos.x - x, camPos.y - h, camPos.z - z));
        Vec3 half = normalize(toCam + sd);
        double sp = std::pow(std::max(0.0, dot(nrm, half)), 48.0) * sunUp * lf;
        spec = sp > 0.55 ? (tv.material == MAT_GLASS ? 18 : 12) : (sp > 0.25 ? (tv.material == MAT_GLASS ? 9 : 6) : 0);   // S-03: the glass's glint is brighter
    }
    double shade;
    if (tv.material == MAT_LAVA) {
        // dark crust plates with bright veins between them
        Worley3 w = worley3(Vec3(x / 11.0, z / 11.0, env.sun.dayFraction * 3.0 + lastT * 0.03), site.gen.seed);   // M4-08: the crust drifts
        double vein = 1 - smoothstep(0.03, 0.14, w.f2 - w.f1);
        double n = 0.5 + 0.5 * gnoise2(x / 5.0, z / 5.0, site.gen.seed + 3);
        double glow = tv.glow / 255.0;
        shade = 8 + 12 * n + 36 * vein * glow + 6 * glow;
    } else {
        shade = exposureStop(albedo, light, site.atmosphere);   // N1-05: the one exposure rule
        if (tv.material == MAT_FOREST) shade *= 1 - 0.38 * (cache.cellSize <= 64 ? canopyDensityAt(x, z, tv.veg) : tv.veg * 0.55);   // N2-02: the floor is darker under the canopy (far ring: the average)
        shade += 16 * emissive + spec;
        // B-311: the broad tone tile (patches of 23-85 m, +-4 shades; a texel is two metres, or a cell for the far rings),
        // sampled per vertex: the cells are smaller than its patches, so the gradient carries it across them
        if (tv.material != MAT_WATER) {
            double g3s = cs <= 64 ? 0.5 : 32.0 / cs;
            shade += (cs <= 64 ? 0.4 : 0.25) * grainMacro.atLerp(x * g3s, z * g3s) * (1 - 0.7 * fog0);
        }
        if (shade > 62) shade = 62;
    }
    // B-322: the drawn height keeps to the drawn side of the edge: the morph blends heights and shore distances by the same
    // factor, and the two cross zero at different places, so a corner could turn dry by its edge yet sink under the plane
    if (wy > -1e8 && shore < 1e8) {   // the margins close to the cut's own lip (0.03 under the level) at the ring's edge, where the coarse cut surface is exact
        double wetLip = 0.3 * (1 - morph), dryLip = 0.02 * (1 - morph);   // O6-06: no lip at the edge: the coarse cut surface respects its water already, and a lip there opened the seam by 3 cm on a lake bed a millimetre under its level
        if (shore < 0) h = std::min(h, wy - wetLip); else h = std::max(h, wy + dryLip);
    }
    Vec3 vv = toView(x, h, z);
    double dist = length(vv);
    // M10-13 scattering lite: haze thicker toward the sun, thinner away from it, more when humid
    double fogD = env.fogDistance;
    if (site.atmosphere && dist > 1) {
        double toSun = dot(Vec3(x - camPos.x, h - camPos.y, z - camPos.z) / dist, sd);
        fogD *= (1 - 0.35 * std::max(0.0, toSun) * sunUp) * (1 + 0.25 * std::max(0.0, -toSun)) * (1 - 0.25 * cloud);
    }
    double fog = 1 - std::exp(-(dist / fogD + rainWall(x, z, dist) + fogSeaWall(x, h, z, dist)));   // W-03: the wall of rain between the camera and this ground; Q-02: a fog sea over it
    shade = shade + (63 - shade) * fog;
    if (shOk) shade += (shC - shade) * morph;   // B-313: onto the coarser ring's shade at the edge
    vc.v.x = vv.x; vc.v.y = vv.y; vc.v.z = vv.z;
    vc.v.shade = shade;
    vc.v.u = x; vc.v.v = z;
    vc.bank = bk;
    vc.wy = (float)wy;
    vc.hy = (float)h;
    vc.sh = (float)shore;
    vc.front = vv.z >= NEAR_Z;
    vc.sy = vc.front ? (float)(proj.cy - proj.f * vv.y / vv.z) : 0.f;
    if (site.hasWater && (double)vc.wy > -1e8) vc.wv = waterCorner(x, z, (double)vc.wy, (double)vc.hy);   // F-04: the stored floats, as the cells read them
    vc.stamp = frameStamp;
    bank = bk;
    return vc.v;
}

// D-04: the frame's lights. The helmet lamp a cone from the eye along the view: full within 11 degrees of its axis, gone at 22, a faint spill
// to 45; its light 3 at the eye falling as 1 / (1 + (d / 16 m)^2), to nothing from 42 m to 60 m. The capsule's floodlights on the way
// down (and up) a wide cone down and ahead from it, from 500 m. Both scaled by the night (a dayside's sun over three degrees up: none)
void SurfaceView::setupLamps() {
    nLamps = 0;
    const double night = 1 - smoothstep(-0.03, 0.06, env.sun.dirLocal.y);
    if (night < 0.01) return;
    if (lamp && !inVehicle() && cameraOverrideAlt < 0 && !freeCam) {
        LampLight& L = lamps[nLamps++];
        L.pos = camPos; L.dir = Vec3(camLocal.m[2][0], camLocal.m[2][1], camLocal.m[2][2]);
        L.cosIn = std::cos(11 * DEG); L.cosOut = std::cos(22 * DEG); L.cosSpill = std::cos(45 * DEG);
        L.nearM = 16; L.reach0 = 42; L.reach = 60; L.power = 3.0 * night;
    }
    if (floodlights && cameraOverrideAlt >= 0 && cameraOverrideAlt < 500) {
        LampLight& L = lamps[nLamps++];
        L.pos = camPos; L.dir = normalize(Vec3(camLocal.m[2][0], 0, camLocal.m[2][2]) * 0.6 + Vec3(0, -1, 0));   // down and ahead, where it comes down
        L.cosIn = std::cos(40 * DEG); L.cosOut = std::cos(70 * DEG); L.cosSpill = std::cos(85 * DEG);
        L.nearM = 140; L.reach0 = 480; L.reach = 650; L.power = 1.4 * night;
    }
}
double SurfaceView::lampAt(const Vec3& p, const Vec3* n) const {
    double sum = 0;
    for (int i = 0; i < nLamps; i++) {
        const LampLight& L = lamps[i];
        const Vec3 d = p - L.pos;
        const double d2 = dot(d, d);
        if (d2 > L.reach * L.reach || d2 < 1e-6) continue;
        const double dist = std::sqrt(d2);
        const Vec3 u = d / dist;
        const double c = dot(u, L.dir);
        if (c <= L.cosSpill) continue;
        const double core = smoothstep(L.cosOut, L.cosIn, c), cone = core + 0.22 * smoothstep(L.cosSpill, L.cosOut, c) * (1 - core);
        const double fall = (1 - smoothstep(L.reach0, L.reach, dist)) / (1 + d2 / (L.nearM * L.nearM));
        sum += L.power * cone * fall * (n ? 0.35 + 0.65 * std::max(0.0, -dot(*n, u)) : 0.8);   // a rough ground's grains catch a grazing light (no face lit at all only edge-on)
    }
    return std::min(sum, 1.0);   // a noon's light at the most: a face turned to the lamp near the eye glares no whiter than a sunlit one
}

RVert SurfaceView::waterCorner(double x, double z, double wl, double hy) const {
    double ddx = x - camPos.x, ddz = z - camPos.z;
    Vec3 vv = toView(x, wl, z);
    double depth = clampd(wl - hy, 0, 8);   // B-322: the depth under the drawn ground (morphed at the ring edges), or the shade jumped at every seam
    Vec3 toV = Vec3(ddx, wl - camPos.y, ddz);
    double dist = length(toV);
    Vec3 rd = normalize(Vec3(toV.x, -toV.y, toV.z));
    double glint = std::pow(std::max(0.0, dot(rd, env.sun.dirLocal)), 28.0) * 24 * waterSunUp * env.sun.lightFactor;
    // M1-04: wave rings coming in with the wind
    double wave = 2.5 * std::sin((x * waterWdx + z * waterWdz) / 2.6 - lastT * 1.4 + 0.8 * std::sin((x * waterWdz - z * waterWdx) / 7.0));
    // B-320: the water's own colour seen from above (deep dark, shallow lighter), the sky's colour at grazing
    // angles (a Fresnel term: nearly all of the far water mirrors the sky), the ramp darkened by the night
    double cosT = clampd(std::fabs(toV.y) / std::max(dist, 0.1), 0.04, 1);   // B-322: the waves' slopes keep the mirror short of grazing: from an eye at the surface the water stays water, not a sheet of sky
    double fres = 0.04 + 0.96 * std::pow(1 - cosT, 4.0);
    double body = 24 + (1 - depth / 8) * 8, skyR = 55;
    double base = body + (skyR - body) * fres + glint + wave * (0.5 + 0.5 * clampd(env.windKnots / 15.0, 0, 1));
    double fog = 1 - std::exp(-(dist / env.fogDistance + rainWall(x, z, dist) + fogSeaWall(x, wl, z, dist)));   // W-03; Q-02
    RVert w;
    w.x = vv.x; w.y = vv.y; w.z = vv.z;
    w.shade = base + (63 - base) * fog;
    w.u = x + lastT * 0.7; w.v = z + lastT * 0.4;
    return w;
}

// B-313: the coarser ring's drawn surface at (x, z): the cell's two triangles are split along the (cx + 1, cz) - (cx, cz + 1)
// diagonal (drawTerrainLOD), and a water vertex sits half a metre under its water, as vertexOf draws it
// B-322: the coarser ring's water level and shore value at a point, interpolated over the triangle of the coarse cell that
// holds it (the split drawTerrainLOD draws, a corner without water taking the mean of the others), exactly as the cut of that
// cell reads them: so a finer vertex morphed onto the coarse cut surface lands on the same side of the same edge
static void coarseTri(TerrainCache& c, double x, double z, int& X, int& Z, int ox[3], int oz[3], double& u, double& v) {
    double csC = c.cellSize;
    double fx = x / csC, fz = z / csC;
    X = (int)std::floor(fx); Z = (int)std::floor(fz);
    fx -= X; fz -= Z;
    if (fx + fz <= 1.0) { ox[0] = 0; oz[0] = 0; ox[1] = 1; oz[1] = 0; ox[2] = 0; oz[2] = 1; u = fx; v = fz; }   // (00, 10, 01): h = h0 + u (h1 - h0) + v (h2 - h0)
    else { ox[0] = 1; oz[0] = 1; ox[1] = 0; oz[1] = 1; ox[2] = 1; oz[2] = 0; u = 1 - fx; v = 1 - fz; }         // (11, 01, 10)
}
double SurfaceView::coarseWaterAt(TerrainCache& c, double x, double z) {
    int X, Z, ox[3], oz[3]; double u, v;
    coarseTri(c, x, z, X, Z, ox, oz, u, v);
    double wMean = 0; int wN = 0;
    for (int k = 0; k < 4; k++) { const TerrainVertex& t = c.at(X + (k & 1), Z + (k >> 1)); if (t.water > -1e8f) { wMean += site.waterOf(t); wN++; } }
    if (!wN) return -1e9;
    wMean /= wN;
    double w[3];
    for (int k = 0; k < 3; k++) { const TerrainVertex& t = c.at(X + ox[k], Z + oz[k]); w[k] = t.water > -1e8f ? site.waterOf(t) : wMean; }
    return w[0] + u * (w[1] - w[0]) + v * (w[2] - w[0]);
}

SurfaceView::CoarseHit SurfaceView::coarseAt(TerrainCache& c, double x, double z) {
    CoarseHit r;
    double csC = c.cellSize;
    double fx = x / csC, fz = z / csC;
    r.X = (int)std::floor(fx); r.Z = (int)std::floor(fz);
    r.fx = fx - r.X; r.fz = fz - r.Z;
    r.lower = r.fx + r.fz <= 1.0;
    r.h = cacheHeightAt(c, x, z);   // B-322: the coarser ring's surface as drawn: its two triangles cut at the water's edge
    return r;
}

double SurfaceView::coarseShoreAt(TerrainCache& c, double x, double z) {
    int X, Z, ox[3], oz[3]; double u, v;
    coarseTri(c, x, z, X, Z, ox, oz, u, v);
    double s[3];
    for (int k = 0; k < 3; k++) { s[k] = vertexShore(c, X + ox[k], Z + oz[k]); if (s[k] > 1e8) return 1e9; }
    return s[0] + u * (s[1] - s[0]) + v * (s[2] - s[0]);
}

void SurfaceView::testRingSeams(double seam[4], double raw[4]) {
    struct R { TerrainCache* c; std::vector<VtxCache>* vc; int r; };
    R rings[5] = {{&site.lodN, &vcN, nearBlend > 0 ? ringN : 0}, {&site.lod0, &vc0, ringR[0]}, {&site.lod1, &vc1, ringR[1]}, {&site.lod2, &vc2, ringR[2]}, {&site.lod3, &vc3, ringR[3]}};
    Mat3 camT = camLocal.transposed();
    for (int k = 0; k < 4; k++) {
        seam[k] = raw[k] = -1;
        if (rings[k].r <= 0) continue;
        R* coarse = nullptr;
        for (int j = k + 1; j < 5 && !coarse; j++) if (rings[j].r > 0) coarse = &rings[j];
        if (!coarse) continue;
        TerrainCache& c = *rings[k].c;
        double cs = c.cellSize; int r = rings[k].r;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        int n = 1 << c.logN, mk = n - 1;
        double worst = 0, worstRaw = 0; int cnt = 0;
        for (int cz = pcz - r - 1; cz <= pcz + r + 1; cz++)
            for (int cx = pcx - r - 1; cx <= pcx + r + 1; cx++) {
                double x = cx * cs, z = cz * cs;
                double dc = std::sqrt((x - camPos.x) * (x - camPos.x) + (z - camPos.z) * (z - camPos.z)) / cs;
                if (dc < r - 0.5 || dc > r + 0.75) continue;   // the edge vertices: morphed wholly onto the coarser surface
                const VtxCache& vc = (*rings[k].vc)[((cz & mk) << c.logN) | (cx & mk)];
                if (vc.stamp != frameStamp || vc.v.u != x || vc.v.v != z) continue;
                Vec3 local = camT * Vec3(vc.v.x, vc.v.y, vc.v.z);
                double d2 = local.x * local.x + local.z * local.z;
                double h = local.y + camPos.y + (site.smallBody ? 0.0 : d2 * invTwoR);   // toView inverted (planets: the paraboloid drop)
                double hC = coarseAt(*coarse->c, x, z).h;
                const TerrainVertex& tv = c.at(cx, cz);
                double own = tv.h; if (tv.material == MAT_WATER && tv.water > -1e8f) own = std::min(own, (double)tv.water - 0.5);
                if (std::fabs(h - hC) > worst && std::getenv("VESPERIS_TRACE")) fprintf(stderr, "    seam %s: cell %d %d h %.3f coarse %.3f own %.3f water %.2f shore %.2f sh(drawn) %.2f wy %.2f\n", c.cellSize == 512 ? "lod2" : "ring", cx, cz, h, hC, own, tv.water > -1e8f ? tv.water : -999.0, tv.shore < 1e8f ? tv.shore : 9999.0, vc.sh, vc.wy);
                worst = std::max(worst, std::fabs(h - hC)); worstRaw = std::max(worstRaw, std::fabs(own - hC)); cnt++;
            }
        if (cnt) { seam[k] = worst; raw[k] = worstRaw; }
    }
}

// The ground beyond the last terrain ring, out to the horizon: the sea, or a plain 300 m under the lowest far cell. O1: a
// fan of rings that follows the curvature (toView), from just inside the far ring's edge to ten times its radius, so
// from a mountain the sea or the plain really dips under the horizon instead of a flat plate hanging at eye level.
double SurfaceView::floorHeight() {
    if (site.smallBody) return 0;
    TerrainCache& far = ringR[3] ? site.lod3 : site.lod2;
    int farR = ringR[3] ? ringR[3] : ringR[2];
    double cs = far.cellSize;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
    double hmin = 1e9;
    for (int cz = pcz - farR; cz <= pcz + farR; cz += 2)
        for (int cx = pcx - farR; cx <= pcx + farR; cx += 2) {
            double ddx = cx + 0.5 - camPos.x / far.cellSize, ddz = cz + 0.5 - camPos.z / far.cellSize;
            if (ddx * ddx + ddz * ddz > (double)farR * farR) continue;   // O6-06: the disc the ring draws (the corners lie beyond the prefetched tiles)
            hmin = std::min(hmin, (double)far.at(cx, cz).h);
        }
    double h = hmin - 300.0;
    if (site.hasWater) h = site.seaLevel + site.tideM;   // Q-02
    return h;
}

void SurfaceView::drawFloor(Framebuffer& fb) {
    if (site.smallBody) return;   // O4: a small body's terrain disc reaches past its own horizon
    TerrainCache& far = ringR[3] ? site.lod3 : site.lod2;
    int farR = ringR[3] ? ringR[3] : ringR[2];
    double cs = far.cellSize;
    double h = floorH;
    const int floorFam = KINDS[site.gen.type].floorFamily;   // N1-03: the dominant family (P-00: the kind's row)
    uint8_t bank = site.hasWater ? 2 : (floorFam == FAM_SNOW ? 8 : (floorFam == FAM_SAND ? 9 : 0));
    double shade = site.atmosphere ? 63 : 8;
    RasterParams rp; rp.bank = bank; rp.ztest = true; rp.zwrite = true;
    double edge = farR * cs * 0.92;
    const double mul[7] = {1.0, 1.45, 2.1, 3.1, 4.6, 7.0, 10.5};
    const int NA = 24;
    for (int i = 0; i + 1 < 7; i++)
        for (int a = 0; a < NA; a++) {
            double a0 = a * TAU / NA, a1 = (a + 1) * TAU / NA, r0 = edge * mul[i], r1 = edge * mul[i + 1];
            double xs[4] = {std::cos(a0) * r0, std::cos(a1) * r0, std::cos(a1) * r1, std::cos(a0) * r1};
            double zs[4] = {std::sin(a0) * r0, std::sin(a1) * r0, std::sin(a1) * r1, std::sin(a0) * r1};
            RVert q[4];
            for (int k = 0; k < 4; k++) { Vec3 v = toView(camPos.x + xs[k], h - 0.3, camPos.z + zs[k]); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; }
            rasterPolygon(fb, q, 4, rp, proj);
        }
}

// B-310: fills the vertex cache (heights, shades, projections) and the material tiles of a ring, with the same cell walk
// as drawTerrainLOD, so that the band threads only read
// B-318: a coarser ring skips a cell only when the whole cell lies under the finer ring: its farthest corner within
// `coverM` metres of the camera, the finer ring's guaranteed cover (`coverOf` in render). A scalar skip radius on the
// cell centres left the outer corners of the skipped cells bare: sawtooth holes at every ring edge, plain from the
// capsule on the way down (`vesperis_test capsule`)
bool SurfaceView::cellCovered(double cs, int cx, int cz, double coverM) const {
    if (coverM <= 0) return false;
    double px = camPos.x / cs, pz = camPos.z / cs;
    double dxm = std::max(std::fabs(cx - px), std::fabs(cx + 1 - px)), dzm = std::max(std::fabs(cz - pz), std::fabs(cz + 1 - pz));
    return (dxm * dxm + dzm * dzm) * cs * cs < coverM * coverM;
}

void SurfaceView::warmTerrainLOD(TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM, TerrainCache* coarse, std::vector<VtxCache>* coarseVc, int band) {
    double cs = cache.cellSize;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
    curRadius = radius;
    morphCoarse = coarse; morphVc = coarseVc; morphBand = coarse ? band : 0;   // B-313: the coarser ring is warmed already (render warms coarse to fine)
    // B-313: the vertices are warmed three cells into the skip disc too: the finer ring's morph band reads this ring's
    // vertex shades there (its edge lies inside this ring's skipped cells), and a stale one would leave the vertex on
    // its own shade, a tooth on the skyline
    double r2 = (double)radius * radius, warmCover = std::max(0.0, coverM - 3 * cs);
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
    auto t0 = std::chrono::steady_clock::now();
    // the terrain cells first (a miss samples the planet function and writes the cache; the shadow march of vertexOf reads cells
    // anywhere within the radius), then the tiles of their materials
    // O6-06: within the disc (plus two cells for the normals): the square's corners lay 75 km out on the far ring, beyond
    // the drainage tiles a landing prefetches, and the first frame computed those tiles (270 ms)
    // F-03: the disc's misses listed first; over FILL_SERIAL_MAX of them they are sampled on the pool (a landing's first frame:
    // 12,000 cells, 14 ms here before), each listed cell by one worker into its own slot: the disc spans 2r + 3 cells a side, under
    // the torus's 1 << logN for every ring (the unit checks it), so distinct cells have distinct slots, and a sample is a pure
    // function of the cell and the site, whatever its thread asked before (the drainage's tiles and neighbourhood read through each
    // thread's own caches, the neighbourhood a function of its lattice cell: `buildNeigh`). The pool's join is the barrier.
    // A few misses (a walk's edge) are sampled by the walk below as before. The material tiles the disc's cells want are made here,
    // after, as before (a band thread reads a tile, it never makes one), the ones not made yet together (`buildMesoTiles`)
    // F-04: the disc reaches every cell the vertex pass reads (on the pool since B-310, every corner since F-04, so a cell it missed
    // would be sampled by two threads into one slot): a drawn cell's centre lies within the radius, its corners read their neighbours
    // for the normals and the shore's side (`vertexShore`: up to two cells on from the cell, 2.83 from its centre), on a lava world the
    // near rings' glow two cells round a corner (4.24); the shadow's march stays within the radius. Distinct slots still: 2r + 5 cells
    const bool glowReach = lavaGlow && cache.cellSize <= 16;
    const int edge = radius + (glowReach ? 2 : 1);
    const double rFill = radius + (glowReach ? 4.5 : 3.0), rFill2 = rFill * rFill;
    auto forDisc = [&](auto&& fn) {
        for (int cz = pcz - edge; cz <= pcz + edge; cz++)
            for (int cx = pcx - edge; cx <= pcx + edge; cx++) {
                double ddx = cx + 0.5 - camPos.x / cs, ddz = cz + 0.5 - camPos.z / cs;
                if (ddx * ddx + ddz * ddz > rFill2) continue;
                fn(cx, cz);
            }
    };
    fillMisses.clear();
    forDisc([&](int cx, int cz) { const TerrainVertex& tv = cache.v[cache.index(cx, cz)]; if (tv.cx != cx || tv.cz != cz) fillMisses.push_back({cx, cz}); });
    const int misses = (int)fillMisses.size();
    lastWarmMisses += misses;
    if (misses > FILL_SERIAL_MAX)
        parallelFor(misses, 16, [&](int b, int e) { for (int k = b; k < e; k++) cache.at(fillMisses[k].first, fillMisses[k].second); });
    uint32_t want = 0;
    forDisc([&](int cx, int cz) { const int m = cache.at(cx, cz).material; if (m < MAT_COUNT && !mesoBuilt[m]) want |= 1u << m; });
    if (want) buildMesoTiles(want);
    auto t1 = std::chrono::steady_clock::now();
    // then the vertices, the four corners of every cell the draw can reach, in parallel by rows of vertices (F-04: the thread of
    // vertex row vz warms the corners on it of the cells either side, cell rows vz - 1 and vz, so every vertex is written by one
    // thread; the cells' top corners had a serial walk of their own)
    auto warmed = [&](int cx, int cz) {
        if (cz < pcz - radius || cz >= pcz + radius) return false;
        double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
        double d2 = dx * dx + dz * dz;
        if (d2 > r2) return false;
        if (cellCovered(cs, cx, cz, warmCover)) return false;
        double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
        double fwdx = camLocal.m[2][0], fwdz = camLocal.m[2][2];
        double dd = std::sqrt(wx * wx + wz * wz);
        return !(dd > cs * 3 && (wx * fwdx + wz * fwdz) / dd < -0.25);
    };
    parallelFor(2 * radius + 1, 4, [&](int rb, int re) {
        for (int vz = pcz - radius + rb; vz < pcz - radius + re; vz++)
            for (int cx = pcx - radius; cx < pcx + radius; cx++) {
                if (!warmed(cx, vz) && !warmed(cx, vz - 1)) continue;
                uint8_t bk;
                vertexOf(cache, vcache, cx, vz, bk);
                vertexOf(cache, vcache, cx + 1, vz, bk);
            }
    });
    if (trace && (frameStamp <= 3 || frameStamp - anchorFrame <= 2)) { auto t2 = std::chrono::steady_clock::now(); fprintf(stderr, "    warm %4d m: %d misses, fill %.1f ms, vertices %.1f ms\n", (int)cs, misses, std::chrono::duration<double, std::milli>(t1 - t0).count(), std::chrono::duration<double, std::milli>(t2 - t1).count()); }
}

// F-04: a terrain cell's pieces, the same for the bin (the rows they reach) and the draw. B-320: the cell's water surface per corner
// (00, 10, 11, 01; the level as drawn, morphed with the ground); a corner without water takes the mean of the others, so the sheet is
// one continuous plane cut by the ground. Returns how many corners have their own level
static int cellWaterLevels(double w00, double w10, double w11, double w01, double wCorner[4]) {
    wCorner[0] = w00; wCorner[1] = w10; wCorner[2] = w11; wCorner[3] = w01;
    double wMean = 0; int wN = 0;
    for (int i = 0; i < 4; i++) if (wCorner[i] > -1e8) { wMean += wCorner[i]; wN++; }
    if (wN) { wMean /= wN; for (int i = 0; i < 4; i++) if (wCorner[i] < -1e8) wCorner[i] = wMean; }
    return wN;
}
// the corners of a cell's two triangles (0: 00 10 01, 1: 10 11 01) in the order 00, 10, 01, 11
static const int CELL_TRIS[2][3] = {{0, 1, 2}, {1, 3, 2}};
// B-322: triangle k of a cell that straddles its water's edge, split along the edge (`shoreSplitTriangle`); hy, wl and sh per corner
// in the order 00, 10, 01, 11 (the drawn height, the water level, the side of the edge)
static void cellShoreSplit(int k, int cx, int cz, double cs, const double hy[4], const double wl[4], const double sh[4], ShoreSplit& sp) {
    static const int oxv[4] = {0, 1, 0, 1}, ozv[4] = {0, 0, 1, 1};
    double xs[3], zs[3], hs[3], wls[3], shs[3];
    for (int e = 0; e < 3; e++) { int c = CELL_TRIS[k][e]; xs[e] = (cx + oxv[c]) * cs; zs[e] = (cz + ozv[c]) * cs; hs[e] = hy[c]; wls[e] = wl[c]; shs[e] = sh[c]; }
    shoreSplitTriangle(xs, zs, hs, wls, shs, sp);
}

void SurfaceView::binTerrainLOD(TerrainBin& bin, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM, bool water,
                                bool skirt, int rowB, int rowE, int& stale, int& allRows) {
    const double cs = cache.cellSize;
    const int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs), mk = (1 << cache.logN) - 1;
    const double r2 = (double)radius * radius;
    auto outside = [&](int ncx, int ncz) { double ex = ncx + 0.5 - (camPos.x / cs), ez = ncz + 0.5 - (camPos.z / cs); return ex * ex + ez * ez > r2; };
    auto current = [&](const VtxCache& vc, int vx, int vz) { return vc.stamp == frameStamp && vc.v.u == (double)(vx * cache.cellSize) && vc.v.v == (double)(vz * cache.cellSize); };
    for (int j = rowB; j < rowE; j++) {
        const int cz = pcz - radius + j;
        TerrainCellRef* out = &bin.cells[(size_t)j * bin.cols];
        int n = 0;
        for (int cx = pcx - radius; cx < pcx + radius; cx++) {
            double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
            double d2 = dx * dx + dz * dz;
            if (d2 > r2) continue;
            if (cellCovered(cs, cx, cz, coverM)) continue;   // B-318: wholly under the finer ring
            // frustum-ish cull: cell centre direction vs camera forward
            {
                double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
                double fwdx = camLocal.m[2][0], fwdz = camLocal.m[2][2];
                double dd = std::sqrt(wx * wx + wz * wz);
                if (dd > cs * 3 && (wx * fwdx + wz * fwdz) / dd < -0.25) continue;
            }
            const VtxCache& c00 = vcache[((cz & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c10 = vcache[((cz & mk) << cache.logN) | ((cx + 1) & mk)];
            const VtxCache& c01 = vcache[(((cz + 1) & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c11 = vcache[(((cz + 1) & mk) << cache.logN) | ((cx + 1) & mk)];
            // the warm made every corner of every cell drawn (the same culls, its cover three cells shorter); a corner it missed is made
            // here as the draw made it before (counted: the tests want none)
            stale += !current(c00, cx, cz) + !current(c10, cx + 1, cz) + !current(c01, cx, cz + 1) + !current(c11, cx + 1, cz + 1);
            uint8_t bk;
            const RVert& v00 = vertexOf(cache, vcache, cx, cz, bk);
            const RVert& v10 = vertexOf(cache, vcache, cx + 1, cz, bk);
            const RVert& v01 = vertexOf(cache, vcache, cx, cz + 1, bk);
            const RVert& v11 = vertexOf(cache, vcache, cx + 1, cz + 1, bk);
            if (v00.z < NEAR_Z && v10.z < NEAR_Z && v01.z < NEAR_Z && v11.z < NEAR_Z) continue;
            double wCorner[4];
            const int wN = cellWaterLevels(c00.wy, c10.wy, c11.wy, c01.wy, wCorner);
            bool anyWet = false, anyDry = false;
            const double shc[4] = {c00.sh, c10.sh, c01.sh, c11.sh};
            if (water && wN > 0) for (int k = 0; k < 4; k++) { if (shc[k] < 0) anyWet = true; else anyDry = true; }
            const bool wet = water && wN > 0 && anyWet, cut = wet && anyDry;
            // the rows: each piece (a triangle of the ground or of its shore cut, a wall, the water's sheet) projected as the rasteriser
            // projects it, clipped at the near plane where it clips it; a piece wholly beside, above or below the frame adds nothing (the
            // rasteriser rejects it: a deep sea's bed under its sheet), and the foam's sparkles put the cell in every band
            int y0 = 1 << 30, y1 = -(1 << 30);
            bool every = false;
            auto piece = [&](const Vec3* p, int np) {
                int a, b;
                if (polygonRows(proj, p, np, a, b, every)) { y0 = std::min(y0, a); y1 = std::max(y1, b); }
            };
            auto V = [](const RVert& r) { return Vec3(r.x, r.y, r.z); };
            const RVert* vs[4] = {&v00, &v10, &v01, &v11};
            if (!cut) {
                const Vec3 tA[3] = {V(v00), V(v10), V(v01)}, tB[3] = {V(v10), V(v11), V(v01)};
                piece(tA, 3); piece(tB, 3);
            } else {
                const double hyv[4] = {c00.hy, c10.hy, c01.hy, c11.hy}, wlv[4] = {wCorner[0], wCorner[1], wCorner[3], wCorner[2]};
                for (int k = 0; k < 2; k++) {
                    ShoreSplit sp;
                    cellShoreSplit(k, cx, cz, cs, hyv, wlv, shc, sp);
                    auto cutPiece = [&](const ShorePt* pts, int np, bool dry) {
                        if (np < 3) return;
                        Vec3 q[4];
                        for (int i = 0; i < np; i++) q[i] = pts[i].a == pts[i].b ? V(*vs[CELL_TRIS[k][pts[i].a]]) : shoreCutView(pts[i], dry);
                        piece(q, np);
                    };
                    cutPiece(sp.dry, sp.nDry, true);
                    cutPiece(sp.wet, sp.nWet, false);
                }
            }
            if (skirt) {
                auto wall = [&](const RVert& a, const RVert& b, double ax, double az, double bx, double bz) {
                    const Vec3 q[4] = {V(a), V(b), skirtFoot(cache, bx, bz), skirtFoot(cache, ax, az)};   // the draw's a, b and their feet
                    piece(q, 4);
                };
                if (outside(cx + 1, cz)) wall(v10, v11, (cx + 1) * cs, cz * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx - 1, cz)) wall(v00, v01, cx * cs, cz * cs, cx * cs, (cz + 1) * cs);
                if (outside(cx, cz + 1)) wall(v01, v11, cx * cs, (cz + 1) * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx, cz - 1)) wall(v00, v10, cx * cs, cz * cs, (cx + 1) * cs, cz * cs);
            }
            if (wet) {
                const VtxCache* vcw[4] = {&c00, &c10, &c11, &c01};
                static const int ox[4] = {0, 1, 1, 0}, oz[4] = {0, 0, 1, 1};
                Vec3 q[4];
                for (int i = 0; i < 4; i++) q[i] = (double)vcw[i]->wy > -1e8 ? V(vcw[i]->wv) : toView((cx + ox[i]) * cs, wCorner[i], (cz + oz[i]) * cs);   // waterCorner's place
                piece(q, 4);
                double cdist = std::sqrt(d2) * cs;
                if (cdist < 90 && cache.cellSize <= 16 && (cs >= 16 || ((cx & 3) == 0 && (cz & 3) == 0)))
                    for (int i = 0; i < 4; i++) if (!(std::fabs(cache.at(cx + ox[i], cz + oz[i]).h - wCorner[i]) > 0.6)) every = true;   // the foam
            }
            if (!every && y0 > y1) continue;   // nothing of it reaches the frame
            TerrainCellRef& r = out[n];
            r.cx = cx; r.cz = cz;
            if (every) { r.y0 = INT16_MIN; r.y1 = INT16_MAX; allRows++; }
            else { r.y0 = (int16_t)y0; r.y1 = (int16_t)y1; }
            n++;
        }
        bin.count[j] = n;
    }
}

void SurfaceView::drawTerrainLOD(Framebuffer& fb, const TerrainBin& bin, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius,
                                 double coverM, bool water, double t, int bandY0, int bandY1, bool skirt, BandTrace* bt) {
    using TraceClock = std::chrono::steady_clock;   // F-01: with `bt` (VESPERIS_TRACE), the band's cells counted and its time split
    auto traceMs = [](TraceClock::time_point a, TraceClock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    double cs = cache.cellSize;
    RasterParams rp;
    rp.grain = &grain;
    rp.grainScale = (cache.cellSize <= 16 ? 3.0 : 0.25) * FB_SCALE;   // same on-screen grain size at every scale
    rp.edge = &grainEdge;                                              // B-311: the boundary wobble
    if (coverM > 0) rp.zbias = 0.999;   // B-313: where a coarser ring overlaps the finer one's morphed edge (coincident surfaces) the finer wins
    if (shadingMode == 1) rp.quantize = 63.0 / 12.0;
    RasterParams rpw;
    rpw.grain = &grainFine; rpw.grainScale = 0.5 * FB_SCALE;
    rpw.zbias = rp.zbias;   // B-322: the water of a ring is pushed back with its ground, or from a kilometre away the metre-high shore lost to the plane in every wet cell and the coast snapped to the grid (KI-321)
    double r2 = (double)radius * radius;
    const int mk = (1 << cache.logN) - 1;
    for (int j = 0; j < (int)bin.count.size(); j++) {   // F-04: the ring's cells as binned, in the walk's order
        const TerrainCellRef* row = &bin.cells[(size_t)j * bin.cols];
        for (int s = 0; s < bin.count[j]; s++) {
            const TerrainCellRef& ref = row[s];
            if (bt) bt->walked++;
            if (ref.y1 < bandY0 || ref.y0 >= bandY1) continue;   // its pieces miss this band's rows
            const int cx = ref.cx, cz = ref.cz;
            double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
            double d2 = dx * dx + dz * dz;
            // the corners as the warm made them (the bin found each current: vertexOf's hit)
            const VtxCache& c00 = vcache[((cz & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c10 = vcache[((cz & mk) << cache.logN) | ((cx + 1) & mk)];
            const VtxCache& c01 = vcache[(((cz + 1) & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c11 = vcache[(((cz + 1) & mk) << cache.logN) | ((cx + 1) & mk)];
            const RVert& v00 = c00.v; const RVert& v10 = c10.v; const RVert& v01 = c01.v; const RVert& v11 = c11.v;
            const uint8_t b00 = c00.bank, b10 = c10.bank, b01 = c01.bank, b11 = c11.bank;
            auto outside = [&](int ncx, int ncz) { double ex = ncx + 0.5 - (camPos.x / cs), ez = ncz + 0.5 - (camPos.z / cs); return ex * ex + ez * ez > r2; };
            const TerrainVertex& t00 = cache.at(cx, cz);
            const TerrainVertex& t10 = cache.at(cx + 1, cz);
            const TerrainVertex& t01 = cache.at(cx, cz + 1);
            const TerrainVertex& t11 = cache.at(cx + 1, cz + 1);
            double wCorner[4];   // B-320: the water surface per corner (00, 10, 11, 01), one continuous plane cut by the ground
            const int wN = cellWaterLevels(c00.wy, c10.wy, c11.wy, c01.wy, wCorner);
            // B-322: the sheet is drawn where a corner lies on the water's side of its edge (`shoreValue`); it used to be drawn
            // wherever a corner lay within half a metre of the level, which reached over the low ground beyond a river's plain
            // as a moat of cells
            bool anyWet = false, anyDry = false;
            double shc[4] = {c00.sh, c10.sh, c01.sh, c11.sh};   // in the vs order: 00, 10, 01, 11 (as drawn: morphed at the ring's edge)
            if (water && wN > 0) for (int k = 0; k < 4; k++) { if (shc[k] < 0) anyWet = true; else anyDry = true; }
            bool wet = water && wN > 0 && anyWet;
            TraceClock::time_point tA;
            if (bt) { bt->drawn++; tA = TraceClock::now(); }
            RVert tri[3];
            // the bank by majority; B-311: a cell whose corners differ in family also gets the second bank and per-corner
            // weights, and the rasteriser draws the boundary as a noisy contour through the cell (it used to be the
            // cell's square edge, so sand, rock and grass met in staircases of plates)
            const uint8_t bks[4] = {b00, b10, b01, b11};
            const RVert* vs[4] = {&v00, &v10, &v01, &v11};
            int counts[BANKS] = {0};
            for (int k = 0; k < 4; k++) counts[bks[k]]++;
            int bank = b00;
            for (int k = 0; k < BANKS; k++) if (counts[k] > counts[bank]) bank = k;
            int bank2 = -1;
            for (int k = 0; k < BANKS; k++) if (k != bank && counts[k] > 0 && (bank2 < 0 || counts[k] > counts[bank2])) bank2 = k;
            rp.bank = bank; rp.bank2 = bank2; rp.shade2 = 0; rp.grain2b = nullptr;
            // M10-05: the material's meso tile on top of the micro grain, both fading with distance; B-311: the far rings
            // carry the tile too, stretched so that it spans two cells
            {
                double cd = std::sqrt(d2) * cs;
                rp.grainWeight = clampd(1.0 - (cd - 120.0) / 500.0, 0.2, 1.0);
                rp.grain2Scale = cs <= 64 ? 4.0 : 128.0 / (2 * cs);
            }
            // flat shading per quadrant, as the original's polymap did; the grain texture and the
            // mush filter give the surface its grain and soften the steps (M10-12: or a quantised gradient)
            double flat, w[4] = {1, 1, 1, 1};
            if (bank2 < 0) {
                rp.grain2 = mesoFor(cache.at(cx, cz).material);
                flat = 0.25 * (v00.shade + v10.shade + v01.shade + v11.shade);
            } else {
                int matA = -1, matB = -1, nA = 0, nB = 0; double sA = 0, sB = 0;
                for (int k = 0; k < 4; k++) {
                    const TerrainVertex& tk = cache.at(cx + (k & 1), cz + (k >> 1));
                    if (bks[k] == bank) { w[k] = 1; sA += vs[k]->shade; nA++; if (matA < 0) matA = tk.material; }
                    else { w[k] = 0; sB += vs[k]->shade; nB++; if (matB < 0 || bks[k] == bank2) matB = tk.material; }
                }
                flat = sA / nA;
                if (shadingMode == 0) rp.shade2 = sB / nB - flat;   // the gradient modes carry each corner's own shade
                rp.grain2 = mesoFor(matA); rp.grain2b = mesoFor(matB);
                rp.edgeScale = 4.0 / cs;   // four texels of wobble per cell
            }
            // B-322: a cell that straddles a water's edge is cut along it (`shoreSplitTriangle`), the cut at the water level:
            // the shoreline follows the shore distance's contour instead of the two triangles' crossing of the plane
            bool cut = wet && anyDry;
            static const char* skipEnv = std::getenv("VESPERIS_SKIP");   // diagnostics (B-322's method): "cut", "wq" (the water quads), "refl", "near" turn one pass off
            if (skipEnv && std::strstr(skipEnv, "cut")) cut = false;
            if (!cut) {
                tri[0] = v00; tri[1] = v10; tri[2] = v01;
                tri[0].w = w[0]; tri[1].w = w[1]; tri[2].w = w[2];
                if (shadingMode == 0) tri[0].shade = tri[1].shade = tri[2].shade = flat;
                rasterTriangle(fb, tri, rp, proj);
                tri[0] = v10; tri[1] = v11; tri[2] = v01;
                tri[0].w = w[1]; tri[1].w = w[3]; tri[2].w = w[2];
                if (shadingMode == 0) tri[0].shade = tri[1].shade = tri[2].shade = flat;
                rasterTriangle(fb, tri, rp, proj);
            } else {
                const double hyv[4] = {c00.hy, c10.hy, c01.hy, c11.hy};
                const double wlv[4] = {wCorner[0], wCorner[1], wCorner[3], wCorner[2]};   // wCorner runs 00, 10, 11, 01
                for (int k = 0; k < 2; k++) {
                    ShoreSplit sp;
                    cellShoreSplit(k, cx, cz, cs, hyv, wlv, shc, sp);
                    auto emitPiece = [&](const ShorePt* pts, int n, bool dry) {
                        if (n < 3) return;
                        RVert q[4];
                        for (int i = 0; i < n; i++) {
                            int a = CELL_TRIS[k][pts[i].a], b = CELL_TRIS[k][pts[i].b];
                            const RVert& va = *vs[a]; const RVert& vb = *vs[b];
                            double t = pts[i].t;
                            if (pts[i].a == pts[i].b) { q[i] = va; q[i].w = w[a]; }
                            else {
                                // O6-03: the dry piece's edge stands SHORE_LIP over the water, the wet piece's lies 3 cm under it: with one shared
                                // point under the plane, the sheet covered the dry piece out to wherever it rose through the plane, which on a
                                // flat shore was near the next corner, and the water's edge ran along the cells' edges in stairs
                                Vec3 vv = shoreCutView(pts[i], dry);
                                q[i].x = vv.x; q[i].y = vv.y; q[i].z = vv.z;
                                q[i].shade = va.shade + (vb.shade - va.shade) * t;
                                q[i].u = va.u + (vb.u - va.u) * t; q[i].v = va.v + (vb.v - va.v) * t;
                                q[i].w = w[a] + (w[b] - w[a]) * t;
                            }
                            if (shadingMode == 0) q[i].shade = flat;
                        }
                        rasterPolygon(fb, q, n, rp, proj);
                    };
                    emitPiece(sp.dry, sp.nDry, true);
                    emitPiece(sp.wet, sp.nWet, false);
                }
            }
            // skirts (KI-006): cells on the outer edge of the outermost ring hang a wall down over the floor. B-322: the inner
            // rings hang none: since B-313 their edges lie on the coarser ring's surface, and a wall there peeked through
            // the coincident ground as a thin bright line along the ring's edge (its depth bias differs from the coarser ring's)
            if (skirt) {
                auto wall = [&](const RVert& a, const RVert& b, double ax, double az, double bx, double bz) {
                    Vec3 la = skirtFoot(cache, ax, az), lb = skirtFoot(cache, bx, bz);
                    RVert q[4] = {a, b, b, a};
                    q[2].x = lb.x; q[2].y = lb.y; q[2].z = lb.z; q[3].x = la.x; q[3].y = la.y; q[3].z = la.z;
                    for (int k = 0; k < 4; k++) q[k].shade = flat * 0.8;
                    rasterPolygon(fb, q, 4, rp, proj);
                };
                if (outside(cx + 1, cz)) wall(v10, v11, (cx + 1) * cs, cz * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx - 1, cz)) wall(v00, v01, cx * cs, cz * cs, cx * cs, (cz + 1) * cs);
                if (outside(cx, cz + 1)) wall(v01, v11, cx * cs, (cz + 1) * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx, cz - 1)) wall(v00, v10, cx * cs, cz * cs, (cx + 1) * cs, cz * cs);
            }
            if (bt) { const TraceClock::time_point tB = TraceClock::now(); bt->fillMs += traceMs(tA, tB); tA = tB; }
            if (water && !(skipEnv && std::strstr(skipEnv, "wq"))) {
                if (wet) {
                    RVert w[4];
                    const TerrainVertex* tv[4] = {&t00, &t10, &t11, &t01};
                    const VtxCache* vcw[4] = {&c00, &c10, &c11, &c01};
                    int ox[4] = {0, 1, 1, 0}, oz[4] = {0, 0, 1, 1};
                    for (int i = 0; i < 4; i++)   // F-04: a corner with its own level was made with its vertex; one without takes the cell's mean
                        w[i] = (double)vcw[i]->wy > -1e8 ? vcw[i]->wv : waterCorner((cx + ox[i]) * cs, (cz + oz[i]) * cs, wCorner[i], vcw[i]->hy);
                    if (bt) { const TraceClock::time_point tC = TraceClock::now(); bt->waterMs += traceMs(tA, tC); bt->wet++; tA = tC; }
                    rpw.bank = 2; rpw.bank2 = -1;
                    if (tintBankW >= 0 || tintBankL >= 0) {   // Q-02: the water's trait colour over the cell's corners (a bloom's or a lagoon's first, else
                        // a pink lake's or the milky glow), its edge wobbled as two materials' are; the glow lit by its share
                        double tw[4] = {0, 0, 0, 0}, mx = 0; int tb = -1;
                        if (tintBankL >= 0) for (int i = 0; i < 4; i++) { tw[i] = tv[i]->ltint / 255.0; mx = std::max(mx, tw[i]); }
                        if (mx > 0.25) tb = tintBankL;
                        else if (tintBankW >= 0) {
                            mx = 0;
                            for (int i = 0; i < 4; i++) {
                                tw[i] = tv[i]->wtint / 255.0;
                                if (tintBankW == 15) tw[i] *= waterLights ? 0.5 + 0.5 * (0.5 + 0.5 * gnoise2((cx + ox[i]) * cs / 260.0 + lastT * 0.02, (cz + oz[i]) * cs / 260.0 - lastT * 0.013, site.gen.seed + 0x6108))   // D-04: drifting
                                                                    : 0.7 + 0.3 * (0.5 + 0.5 * gnoise2((cx + ox[i]) * cs / 700.0, (cz + oz[i]) * cs / 700.0, site.gen.seed + 0x6107));
                                mx = std::max(mx, tw[i]);
                            }
                            if (mx > (waterLights ? 0.12 : 0.25)) tb = tintBankW;
                        }
                        if (tb >= 0) {
                            double wmin = 1, wmax = 0;
                            const double lo = waterLights && tb == 15 ? 0.12 : 0.25, hi = waterLights && tb == 15 ? 0.42 : 0.75;   // D-04: a dark ocean's lights a fainter field, its patches whole sooner
                            for (int i = 0; i < 4; i++) {
                                w[i].w = 1 - smoothstep(lo, hi, tw[i]);   // a corner well inside is the colour's whatever the wobble (it flipped pixels back to the sea)
                                wmin = std::min(wmin, w[i].w); wmax = std::max(wmax, w[i].w);
                                if (tb == 15) w[i].shade = waterLights ? 15 + 18 * tw[i] : 20 + 19 * tw[i] * tintGlow;
                            }
                            // a cell wholly the colour's (the wobble's 0.35 never lifts a pixel to a half) is drawn in its bank alone, as the split draws
                            // it, without the split's noise a pixel; one wholly the water's stays the water's
                            if (wmax < 0.149) rpw.bank = tb;
                            else if (wmin < 0.851) { rpw.bank2 = tb; rpw.edge = &grainEdge; rpw.edgeScale = 4.0 / cs; rpw.edgeAmp = 0.35; rpw.shade2 = 0; }
                        }
                    }
                    rasterPolygon(fb, w, 4, rpw, proj);
                    // foam sparkle where the shore is shallow and close (Q-02: none where the sea glows, its dark night bank would dot it)
                    double cdist = std::sqrt(d2) * cs;
                    if (cdist < 90 && cache.cellSize <= 16 && (cs >= 16 || ((cx & 3) == 0 && (cz & 3) == 0)) && rpw.bank2 != 15 && rpw.bank != 15) {   // O6-02: one near cell in sixteen keeps the density
                        for (int i = 0; i < 4; i++) {
                            if (std::fabs(tv[i]->h - wCorner[i]) > 0.6) continue;
                            uint64_t hf = hash2i(cx + ox[i], cz + oz[i], site.gen.seed ^ 0xF0A);
                            for (int k = 0; k < 3; k++) {
                                double fx = (cx + ox[i]) * cs + (hash01(mix64(hf + k)) - 0.5) * cs * 0.9;
                                double fz = (cz + oz[i]) * cs + (hash01(mix64(hf + k + 7)) - 0.5) * cs * 0.9;
                                double ph = std::sin(t * 2.2 + hash01(mix64(hf + k + 13)) * TAU);
                                if (ph < 0.3) continue;
                                Vec3 fv = toView(fx, wCorner[i] + 0.05, fz);
                                if (fv.z < NEAR_Z) continue;
                                RVert p; p.x = fv.x; p.y = fv.y; p.z = fv.z; p.shade = 50 + 12 * ph;
                                rasterPoint3(fb, p, 2, proj, true, 1);
                            }
                        }
                    }
                    if (bt) bt->fillMs += traceMs(tA, TraceClock::now());
                }
            }
        }
    }
}

// F-10: the sky's powers of a cosine as products of squares (libm's pow was the sky's largest cost; these are within a few dozen ulp of
// it): the day's two glows round a sun, 9 c^6 + 14 c^50; the sun's disc in an airless sky, c^300; a comet's tail, c^18
static inline double sunGlow(double c) { const double c2 = c * c, c6 = c2 * c2 * c2, c12 = c6 * c6, c24 = c12 * c12; return c6 * 9 + c24 * c24 * c2 * 14; }
static inline double pow300(double c) { const double c2 = c * c, c6 = c2 * c2 * c2, c12 = c6 * c6, c24 = c12 * c12, c50 = c24 * c24 * c2, c150 = c50 * c50 * c50; return c150 * c150; }
static inline double pow18(double c) { const double c2 = c * c, c6 = c2 * c2 * c2; return c6 * c6 * c6; }

void SurfaceView::drawSky(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr) {
    const Body& b = site.sys->bodies[site.body];
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    Mat3 camT = camLocal.transposed();
    const Vec3 sd = env.sun.dirLocal;
    double sinAlt = sd.y;
    double day = env.skyBrightness;
    double sunUp = smoothstep(-0.28, 0.0, sinAlt) * env.glow;   // B-425: a far sun alone up glows as faintly as it lights
    bool venus = hasOpaqueDeck(b.type);   // R-307: the hydrocarbon haze is a deck like the venusian clouds
    double density = L.skyDensity * (1 - 0.3 * env.rain);
    const double limbHaze = KINDS[b.type].limbHaze;
    // Q-02 (TR_ICE_HALOS): an air whose ice crystals hang aloft whatever the ground's warmth: the 22-degree ring brighter and every day the
    // sun is up through a sky not overcast, the parhelia (sundogs) beside the sun at its height, the parhelic circle through it round the
    // sky, the 46-degree ring faint, and at night the ring round the brightest moon
    const bool iceHalos = site.gen.hasTrait(TR_ICE_HALOS) && site.atmosphere && !venus;
    double halo = (site.atmosphere && !venus && env.temperatureC < -5 && env.cloudCover < 0.35 && sinAlt > 0.05) ? 0.8 : 0.0;
    if (iceHalos && env.cloudCover < 0.85 && sinAlt > 0.02) halo = 1.5 * (1 - 0.6 * env.rain);
    Vec3 dogL, dogR; double dogs = 0, moonHalo = 0;
    if (iceHalos && halo > 0 && sinAlt < 0.86) {   // the plates' parhelia: at the sun's height, a little outside the ring as the sun climbs (none past 60 degrees)
        const double h = std::asin(clampd(sinAlt, -1, 1)), rp = (22.0 + 0.15 * std::max(0.0, h / DEG)) * DEG;
        const double ch = std::cos(h), dA = std::acos(clampd((std::cos(rp) - sinAlt * sinAlt) / std::max(1e-6, ch * ch), -1, 1)), az = std::atan2(sd.x, sd.z);
        dogL = Vec3(ch * std::sin(az - dA), sinAlt, ch * std::cos(az - dA)); dogR = Vec3(ch * std::sin(az + dA), sinAlt, ch * std::cos(az + dA));
        dogs = halo * smoothstep(0.86, 0.6, sinAlt);
    }
    if (iceHalos && env.moonBody >= 0 && env.moonLight > 0.004 && sinAlt < -0.05) moonHalo = clampd(std::sqrt(env.moonLight / 0.05), 0, 1) * (1 - env.rain) * (1 - 0.7 * env.cloudCover);   // a moon of a twentieth of the sun's light at its full
    const Vec3 moonD = env.moonDir;
    const double cosMoonRingLo = std::cos(24.01 * DEG), cosMoonRingHi = std::cos(19.99 * DEG);
    // Q-02 (TR_NOCTILUCENT): silver-blue clouds 83 km up in a high latitude's summer twilight, lit from below the horizon after the sun has
    // set (6-16 degrees under it) while the air under them is in the planet's shadow: low in the sky toward the sun's side, in bands and billows
    double nlc = 0, nlcLimitDeg = 0;
    const double nlcH = 83.0, RkmS = site.R / 1000.0;
    if (site.gen.hasTrait(TR_NOCTILUCENT) && site.atmosphere && !venus) {
        const double dep = -std::asin(clampd(sinAlt, -1, 1)) / DEG, summer = clampd(0.5 + (env.latDeg >= 0 ? site.season : -site.season), 0, 1);
        nlc = smoothstep(3.5, 6.5, dep) * smoothstep(17.0, 12.5, dep) * smoothstep(35.0, 50.0, std::fabs(env.latDeg)) * (0.45 + 0.55 * summer) * (1 - env.rain) * (1 - 0.85 * env.cloudCover);
        nlcLimitDeg = std::acos(RkmS / (RkmS + nlcH)) / DEG;   // the depression past which the layer lies in the shadow (9 degrees on a world Earth's size)
    }
    // N5-02: a sun pillar in cold, clear air when the sun is low (ice crystals in the air mirror it into a column)
    double pillar = (site.atmosphere && !venus && env.temperatureC < -5 && env.cloudCover < 0.45 && sinAlt > -0.03 && sinAlt < 0.2) ? (1 - std::max(0.0, sinAlt) / 0.2) : 0.0;
    double sunAltA = std::asin(clampd(sinAlt, -1, 1));
    double wedge = (site.atmosphere && !venus && sinAlt > -0.17 && sinAlt < 0.06) ? 1.0 - std::fabs(sinAlt + 0.05) / 0.11 : 0.0;
    if (wedge < 0) wedge = 0;
    Vec3 sdh = normalize(Vec3(sd.x, 0, sd.z + 1e-9));
    // cloud grid
    bool clouds = site.atmosphere && L.cloudsLayer && (env.cloudCover > 0.03 || localFront.on);   // W-03: a front's deck in view counts
    double aurora = env.aurora;
    const int CELL = 4 * FB_SCALE;
    const int GX = FBW / CELL + 1, GY = FBH / CELL + 1;
    if (clouds) {
        cloudGrid.assign(GX * GY * 3, 0.f);   // per cell: the density, the index, and (W-03) the share that is a front's deck
        double thrP = 1.0 - env.cloudPattern * 0.75;   // W-03: the pattern's threshold; a front's deck is read per cell from its line
        double wx = windDriftX, wz = windDriftZ;
        // two layers (M10-14): low cumulus at 2600 m with lit tops and shaded bases, high thin veils at 6000 m. F-05: the grid's rows on the
        // pool, as the aurora's (a cell reads pure functions and writes its own three floats)
        parallelFor(GY, 4, [&](int gy0, int gy1) {
        for (int gy = gy0; gy < gy1; gy++)
            for (int gx = 0; gx < GX; gx++) {
                int px = std::min(gx * CELL, FBW - 1), py = std::min(gy * CELL, FBH - 1);
                Vec3 d = camT * dirLUT[py * FBW + px];
                double el = d.y;
                float dens = 0, idx = 0, storm = 0;
                if (el > 0.012) {
                    double tt = 2600.0 / el;
                    double x = camPos.x + d.x * tt + wx, z = camPos.z + d.z * tt + wz;
                    double thr = thrP, lp = 0, veil = 0, fcl = 0;
                    if (localFront.on) {   // W-03: the front's deck where this cell's ray meets the layer (the band moves with the front, its texture with the wind), and its veil ahead
                        double xw = camPos.x + d.x * tt, zw = camPos.z + d.z * tt;
                        fcl = localFront.cloud(xw, zw);
                        thr = 1.0 - std::max(env.cloudPattern, fcl) * 0.75;
                        lp = localFront.precip(xw, zw);
                        veil = localFront.veil(xw, zw);
                    }
                    double n = 0.5 + 0.5 * fbm2(x / 1800.0, z / 1800.0, site.gen.seed + 77, 4, 2.2, 0.55);
                    double dLow = smoothstep(thr, thr + 0.18, n) * smoothstep(0.012, 0.09 - 0.05 * clampd(fcl, 0, 1), el);   // W-03: a front's solid deck is drawn nearer the horizon (a dark band over it when the wall is far)
                    double cosSun = dot(d, sd);
                    // denser toward the sun means this part of the cloud is its shaded underside
                    double nSun = 0.5 + 0.5 * fbm2((x + sd.x * 260) / 1800.0, (z + sd.z * 260) / 1800.0, site.gen.seed + 77, 3, 2.2, 0.55);
                    double shadeSide = clampd((nSun - n) * 4.0, 0, 1) * (0.6 + 0.4 * sd.y);
                    double lit = 0.62 + 0.38 * cosSun - 0.32 * shadeSide;
                    double structure = 0.8 + 0.35 * clampd((n - thr) / 0.35, 0, 1) - 0.15 * clampd((n - thr - 0.4) / 0.3, 0, 1);
                    double idxLow = day * (33 + 9 * lit) * structure + (1 - day) * 4;
                    // high veil: thin, bright, slow
                    double th = 6000.0 / el;
                    double hx = camPos.x + d.x * th + wx * 0.4, hz = camPos.z + d.z * th + wz * 0.4;
                    double nh = 0.5 + 0.5 * fbm2(hx / 4200.0 + 31.0, hz / 4200.0, site.gen.seed + 78, 3, 2.0, 0.5);
                    double thrH = thr + 0.05 - 0.35 * veil;   // W-03: the milky veil hundreds of kilometres ahead of a front
                    double dHigh = smoothstep(thrH, thr + 0.3, nh) * (0.4 + 0.3 * veil) * smoothstep(0.012, 0.06, el);
                    double idxHigh = day * (38 + 6 * cosSun) + (1 - day) * 5;
                    double dAll = dLow + dHigh * (1 - dLow);
                    dens = (float)dAll;
                    storm = (float)(clampd(fcl * 1.5, 0, 1) * (dAll > 1e-6 ? dLow / dAll : 0.0));   // W-03: the deck's share of the cell (the veil is the sky's)
                    idx = (float)(dAll > 1e-6 ? (idxLow * dLow + idxHigh * dHigh * (1 - dLow)) / dAll : 0);
                    { double rr = std::max(env.rain, lp); if (rr > 0 || fcl > 0) idx *= (float)std::max(0.3, 1 - 0.35 * rr - 0.35 * fcl - 0.1 * lp); }   // W-03: a front's deck is dark underneath, darker over its rain (without a front: the rain's 0.35 as before)
                }
                cloudGrid[(gy * GX + gx) * 3] = dens;
                cloudGrid[(gy * GX + gx) * 3 + 1] = idx;
                cloudGrid[(gy * GX + gx) * 3 + 2] = storm;
            }
        });
    }
    // B-403: the aurora as curtains. Three sheets of light hang 90-250 km up along the auroral oval, which lies a few degrees
    // of latitude poleward of the site (over it and beyond, inside the oval): slabs 10-16 km thick, their light a gamma profile
    // in height (nothing under 90 km, a bright lower border at 114, mostly gone by 250 with a faint tail to 400), folded by two slow
    // waves, rayed by a noise along them, breathing. A cell's value is the light along its ray: the heights where the ray
    // enters and leaves each slab give the profile's integral, divided by the ray's climb at the border (the ground curving
    // away under it), so a sheet seen along its length is bright and one crossed steeply is faint; a ray under the border
    // sees nothing. Sampled on the cloud grid and drawn after the stars, on sky pixels only, with no threshold: it fades into
    // the night sky. It used to be five lobes of azimuth, opaque and hard-edged, which met in a flower at the zenith.
    const bool auroraOn = aurora > 0.02 && day < 0.4;
    if (auroraOn) {
        auroraGrid.assign(GX * GY * 2, 0.f);
        const double Rkm = site.R / 1000.0, kmDeg = Rkm * DEG;
        const double pole = env.latDeg >= 0 ? 1.0 : -1.0;
        const double storm = env.auroraStorm;
        // km poleward of the site to the oval's centre (nearer the pole on a strong field, pushed equatorward by a storm), kept
        // within sight on every world, drifting over five minutes
        const double P0 = clampd((auroraOvalNow(b) - 6 * storm - std::fabs(env.latDeg)) * kmDeg, -120.0, 450.0) + 30.0 * std::sin(t / 310.0);   // Q-02: a great aurorae's oval lower
        lastAuroraP0 = P0;
        // the profile in height, in two parts: the lower curtain f(u) = u / H^2 exp(-u / H), u = height - h0, its peak at h0 + H
        // (H 24 km: nothing under 90 km, a bright border at 114, gone by 200), in bank 11's colour; and the tops, the same shape
        // with H 70 km reaching 400 km, in bank 21's. G is a part's integral from the ground
        const double h0 = 90.0, H = 24.0, H2 = 70.0;
        auto G = [](double u, double Hk) { return u <= 0 ? 0.0 : 1.0 - std::exp(-u / Hk) * (1 + u / Hk); };
        struct Sheet { double off, w, gain, ph; };
        // km from the centre, half-thickness, gain, phase: a storm brings the outer sheets up
        const Sheet sheets[3] = {{0, 5, 1.0, 0.0}, {150, 4, 0.3 + 0.4 * storm, 2.1}, {-110, 3.5, 0.25 + 0.5 * storm, 4.2}};
        const double camKmX = camPos.x / 1000.0, camKmZ = camPos.z / 1000.0;
        const double hPeak = h0 + H;
        // the tops: faint on a quiet night, their colour taking over the heights in a storm, more on some worlds than others
        const double topGain = (0.25 + 0.75 * storm) * (0.6 + 0.8 * unitFromHash(hashCombine(site.gen.seed, 0xA0D))) * (site.gen.hasTrait(TR_GREAT_AURORAE) ? 1.5 : 1.0);   // Q-02: a strong field's red tops
        parallelFor(GY, 4, [&](int gy0, int gy1) {
        for (int gy = gy0; gy < gy1; gy++)
            for (int gx = 0; gx < GX; gx++) {
                int px = std::min(gx * CELL, FBW - 1), py = std::min(gy * CELL, FBH - 1);
                Vec3 d = camT * dirLUT[py * FBW + px];
                double el = d.y;
                if (el < 0.005) continue;
                double hl = std::sqrt(std::max(1e-9, 1 - el * el));
                // the height over the curved ground at slant s is s el + (s hl)^2 / (2 R); the ray's climb per km of slant at the border
                double a = hl * hl / (2 * Rkm);
                double sPeak = 2 * hPeak / (el + std::sqrt(el * el + 4 * a * hPeak));
                double climb = el + 2 * a * sPeak;
                double dz = d.z * pole;   // the ray's rate of going poleward
                if (std::fabs(dz) < 1e-4) dz = dz < 0 ? -1e-4 : 1e-4;
                // the light along the ray through a slab |P - Pf| < w, both parts: the profiles' integrals between the heights
                // where the ray enters and leaves it, over the climb
                auto slab = [&](double Pf, double w, double wgt, double& low, double& top) {
                    double sA = (Pf - w) / dz, sB = (Pf + w) / dz;
                    if (sA > sB) std::swap(sA, sB);
                    if (sB <= 0) return;      // the slab lies behind
                    if (sA < 0) sA = 0;       // the ray starts inside it
                    double uA = sA * el + sA * sA * a - h0, uB = sB * el + sB * sB * a - h0;
                    low += wgt * (G(uB, H) - G(uA, H)) / climb;
                    top += wgt * (G(uB, H2) - G(uA, H2)) / climb;
                };
                double E = sPeak * d.x + camKmX;   // km east along the sheets where the ray reaches the border's height
                double sumLow = 0, sumTop = 0;
                for (const Sheet& sh : sheets) {
                    double Pc = P0 + sh.off - camKmZ * pole;
                    double Pf = Pc + 14 * std::sin(E / 45.0 + t * 0.07 + sh.ph) + 6 * std::sin(E / 16.0 - t * 0.11 + 2 * sh.ph);   // the folds
                    double low = 0, top = 0;   // a dense core in thinner envelopes: soft edges
                    slab(Pf, sh.w, 0.5, low, top); slab(Pf, 1.7 * sh.w, 0.3, low, top); slab(Pf, 2.6 * sh.w, 0.2, low, top);
                    if (low + top < 1e-4) continue;
                    // the rays: two octaves of noise along the sheet, read where the ray crosses it (so from under a curtain they
                    // converge toward the zenith), strong on a sheet near by, a far arc a smooth band; a ray running along the
                    // sheet passes many of them and sees their average
                    double Er = clampd(std::fabs(Pf / dz), 0.6 * sPeak, 2.5 * sPeak) * d.x + camKmX;
                    double rayAmp = 0.7 * clampd(1.3 - sPeak / 450.0, 0.2, 1.0) * clampd(1.2 - sh.w * std::fabs(d.x / dz) / 20.0, 0.25, 1.0);
                    double n = 0.5 + 0.67 * (gnoise2(Er / 12.0, t / 40.0 + sh.ph, site.gen.seed + 46) + 0.5 * gnoise2(Er / 4.5, t / 25.0 + sh.ph, site.gen.seed + 47));
                    double rays = 1 - rayAmp * (1 - clampd(n, 0, 1));
                    // the patches: a slow noise along the sheet thins it to a fifth here and there; a breathing over seventeen seconds
                    double patch = 0.2 + 0.8 * std::pow(clampd(0.5 + 0.67 * gnoise2(E / 260.0, t / 120.0 + sh.ph, site.gen.seed + 48), 0, 1), 1.4);
                    double breath = 0.7 + 0.3 * std::sin(t / 17.0 + sh.ph);
                    double m = sh.gain * rays * patch * breath;
                    sumLow += m * low; sumTop += m * top * topGain;
                }
                double sum = (sumLow + sumTop) * (1 - env.rain) * (1 - env.dust);   // rain and a dust storm take it away, as the rain takes the stars (the night clouds hide neither: KI-336)
                if (sum > 1e-4) {
                    auroraGrid[(gy * GX + gx) * 2] = (float)(44.0 * (1 - std::exp(-4.0 * aurora * sum)));   // a soft knee: the sheets seen along their length saturate, a steep crossing stays faint
                    auroraGrid[(gy * GX + gx) * 2 + 1] = (float)(sumTop / (sumLow + sumTop));                  // the tops' share: the colour
                }
            }
        });
    }
    const int step = FB_SCALE >= 3 ? 2 : 1;   // the sky is smooth: half resolution at 3x and 4x
    Mat3 toWorld = site.localFrame(t).transposed();
    // M5-01: the companion sun's own glow in the sky
    const bool twoSuns = env.hasSun2 && env.sun2.dirLocal.y > -0.3;
    const Vec3 sd2 = twoSuns ? env.sun2.dirLocal : Vec3(0, -1, 0);
    const double sun2Up = twoSuns ? smoothstep(-0.28, 0.0, sd2.y) * clampd(env.sun2.lightFactor / std::max(env.sunDisc.lightFactor, 0.05), 0.15, 1.0) : 0.0;
    // M5-04: the ring plane of a ringed world seen from its own surface, an arc across the sky
    const bool ringArc = b.rings && !venus;
    Vec3 ringOrig; Vec3 ringN = b.spinAxis; Vec3 ringSunW; double ringR0 = 0, ringR1 = 0, ringLf = 1;
    const std::vector<float>* ringProf = nullptr;
    if (ringArc) {
        ringOrig = site.worldPos(t, camPos.x, camPos.z, camPos.y / 1000.0) - site.sys->bodyPos(site.body, t);   // km, from the centre
        ringSunW = toWorld * env.sunDisc.dirLocal;
        ringR0 = b.ringInner * b.radiusKm; ringR1 = b.ringOuter * b.radiusKm;
        ringProf = &sr.ringProfile(*site.sys, site.body);
        ringLf = clampd(env.sun.lightFactor, 0.3, 1.1);
    }
    // O0-01: which face of the ring we see: the lit face when the sun is on our side of the ring plane, else the
    // unlit face, dim with the light that comes through
    const double ringFace = ringArc ? ((dot(ringOrig, ringN) * dot(ringSunW, ringN) >= 0) ? 1.0 : 0.5) : 1.0;
    double ringPow[256];   // F-07: the arc's pow of each of the profile's 256 densities, once a frame
    if (ringArc) for (int i = 0; i < 256; i++) ringPow[i] = std::pow(clampd((double)(*ringProf)[i] * 1.3, 0, 1), 0.35);
    const double cosHaloLo = std::cos((22.0 + 1.401) * DEG), cosHaloHi = std::cos((22.0 - 1.401) * DEG);
    const double pillarElLo = std::sin(sunAltA - 2.001 * DEG), pillarElHi = std::sin(sunAltA + 11.001 * DEG), pillarTanAz = std::tan(2.501 * DEG);
    // O4: on a comet the coma hazes the whole sky, brightest toward the sun, and the ion tail streams from the anti-solar
    // point; both grow with the activity
    const bool comet = b.type == PT_COMET && env.cometActivity > 0.02;
    const double act = env.cometActivity;
    double knotT[5]; for (int k = 0; k < 5; k++) knotT[k] = SpaceRenderer::cometKnot(k, t);
    // W-03: the wall of rain (or dust) under a front's deck: toward the band the haze thickens to the horizon's grey between
    // the horizon and the cloud base, by the rain the ray crosses (`LocalFront::wall`, the same box the ground is fogged by)
    const bool curtain = site.atmosphere && localFront.on;
    const double curtainV = day * 20 + (1 - day) * 3;   // a dark wall of the sky's own colour, between the zenith's and the horizon's
    int rowsTotal = (FBH + step - 1) / step;
    parallelFor(rowsTotal, 40, [&](int rb, int re) {
    for (int y = rb * step; y < re * step && y < FBH; y += step) {
        for (int x = 0; x < FBW; x += step) {
            int o = y * FBW + x;
            Vec3 d = camT * dirLUT[o];
            double el = d.y;
            double v;
            double tailV = 0, stormW = 0;   // W-03: how much of the pixel is a front's deck or its rain (bank 23 past a half)
            double paleV = 0;   // Q-02: a noctilucent cloud's or the moon's ring's shade in the galactic band's bank (22: the sky's colour to a pale silver)
            if (!site.atmosphere) {
                // F-07: each pow only where its base is over 0 (elsewhere its term is +0, as the formula gives)
                double cosSun = dot(d, sd);
                v = cosSun > 0 ? pow300(cosSun) * 22 * env.glow : 0.0;
                if (twoSuns) { const double c2 = dot(d, sd2); if (c2 > 0) v += pow300(c2) * 22 * sun2Up; }
                if (comet) {
                    double cs = std::max(0.0, cosSun);
                    v += act * (6 + 14 * cs * cs * cs);
                    double tail = cosSun < 0 ? pow18(-cosSun) * act : 0.0;   // a cone some 15 degrees wide round the anti-solar point
                    if (tail > 0.02 && el > -0.05) {
                        // the knots of the ion tail stream outward: brightness along the anti-solar distance
                        double f = std::acos(clampd(-cosSun, -1, 1)) / (0.5 * PI);   // 0 at the anti-solar point .. 1 at 90 degrees
                        double knots = 0;
                        for (int k = 0; k < 5; k++) { double dd = f - knotT[k]; knots += std::exp(-dd * dd / (2 * 0.06 * 0.06)); }
                        tailV = 6 + 26 * tail * (0.8 + 0.3 * knots);
                    }
                }
            } else {
                double hz = el <= 0 ? 1.0 : std::pow(1 - clampd(el, 0, 1), 2.2);   // F-07: pow(1, 2.2) is 1 under the horizon
                double base;
                if (venus) base = 26 + 14 * hz;
                else base = day * (15 + 25 * hz) + (1 - day) * (2 + 5 * hz);
                base += 6.0 * day * smoothstep(0.09, 0.0, el);   // M1-07: haze band on the horizon
                base *= density;
                if (limbHaze > 0) base += limbHaze * day * smoothstep(0.08, 0.0, el);   // P-05: a haze's layers low over a dark sky
                double cosSun = dot(d, sd);
                double cs = std::max(0.0, cosSun);
                // F-07: the glows' pows only where the sun is up and the pixel faces it (elsewhere the term is +0)
                double g = sunUp > 0 && cs > 0 ? sunGlow(cs) * sunUp * (venus ? 1.6 : 1.0) : 0.0;
                if (twoSuns) { double cs2 = std::max(0.0, dot(d, sd2)); if (sun2Up > 0 && cs2 > 0) g += sunGlow(cs2) * sun2Up; }
                v = base + g;
                // M10-14: 22-degree halo in cold, clear air; twilight wedge opposite a low sun. F-07: the ring's and the pillar's tests
                // first on the cosine, the height and the azimuth's tangent (their bounds a thousandth of a degree outside the ones after)
                if (halo > 0 && cosSun > cosHaloLo && cosSun < cosHaloHi) { double ang = std::acos(clampd(cosSun, -1, 1)) / DEG; double dh = std::fabs(ang - 22.0); if (dh < 1.4) v += 5.0 * halo * (1 - dh / 1.4); }
                if (iceHalos && halo > 0) {   // Q-02: the sundogs, the parhelic circle, the 46-degree ring
                    if (dogs > 0) for (const Vec3& dg : {dogL, dogR}) {
                        const double cd = dot(d, dg);
                        if (cd > 0.9945) {   // within 6 degrees: a bright spot drawn out along the sun's height, a white tail outward
                            const double dAlt = (std::asin(clampd(el, -1, 1)) - std::asin(clampd(dg.y, -1, 1))) / DEG, dAz = std::acos(clampd(cd, -1, 1)) / DEG;
                            const double outward = dot(d, dg) > 0 && dot(d - dg, dg - sd) > 0 ? 1.0 : 0.0;
                            v += dogs * (17.0 * std::exp(-(dAz * dAz) / 0.6 - (dAlt * dAlt) / 0.3) + 5.0 * std::exp(-(dAz * dAz) / 4.0 - (dAlt * dAlt) / 1.2) + 3.0 * outward * std::exp(-std::fabs(dAlt) / 0.4) * std::exp(-dAz / 5.0));
                        }
                    }
                    const double dh = std::fabs(std::asin(clampd(el, -1, 1)) - sunAltA) / DEG;
                    if (dh < 0.7 && cosSun < 0.94) v += 2.2 * halo * (1 - dh / 0.7) * smoothstep(0.94, 0.85, cosSun);   // the parhelic circle, from past the ring
                    if (cosSun > cosHaloHi) v -= 2.5 * halo * smoothstep(cosHaloHi, 0.95, cosSun) * smoothstep(0.995, 0.97, cosSun);   // the sky inside the ring darker (no light is bent into it)
                    if (cosSun > 0.66 && cosSun < 0.72) { const double d46 = std::fabs(std::acos(clampd(cosSun, -1, 1)) / DEG - 46.0); if (d46 < 2.0) v += 1.8 * halo * (1 - d46 / 2.0); }
                }
                if (moonHalo > 0) {   // Q-02: the moon's ring at night
                    const double cm = dot(d, moonD);
                    if (cm > cosMoonRingLo && cm < cosMoonRingHi) {   // pale in the night's ramp (black under its shade 36, as the noctilucent clouds are drawn), soft
                        const double dh = std::fabs(std::acos(clampd(cm, -1, 1)) / DEG - 22.0);
                        if (dh < 2.0) { const double f = 1 - dh / 2.0, k = moonHalo * f * f; paleV = std::max(paleV, 30.0 * k); }
                    }
                }
                if (nlc > 0 && el > 0.012 && el < 0.5) {   // Q-02: the noctilucent clouds where the ray meets their layer
                    const double sl = nlcH / std::max(el, 0.03), hl = std::sqrt(d.x * d.x + d.z * d.z);
                    const double X = camPos.x / 1000.0 + d.x * sl, Z = camPos.z / 1000.0 + d.z * sl;
                    const double along = hl > 1e-6 ? (d.x * sdh.x + d.z * sdh.z) / hl * sl : 0.0;   // km toward the sun's azimuth
                    const double depHere = -sunAltA / DEG - along / RkmS / DEG;                      // the sun's depression under the cloud
                    const double lit = smoothstep(nlcLimitDeg + 1.5, nlcLimitDeg - 1.0, depHere) * smoothstep(-2.0, 1.5, depHere);
                    if (lit > 0.01) {
                        // long bands tens of kilometres apart drawn out along one way, fine billows across them where the ray is steep enough
                        // to resolve them (a distant layer seen edge-on keeps the bands alone), the patches hundreds of kilometres wide
                        const double patch = smoothstep(0.38, 0.68, 0.5 + 0.5 * fbm2(X / 420.0, Z / 420.0, site.gen.seed + 91, 2, 2.0, 0.5));
                        const double bands = 0.5 + 0.5 * std::sin((X * 0.8 + Z * 0.6) / 14.0 + 2.5 * fbm2(X / 90.0, Z / 90.0, site.gen.seed + 92, 2, 2.0, 0.5));
                        const double billows = 1.0 - 0.3 * smoothstep(0.04, 0.12, el) * (0.5 + 0.5 * std::sin((Z * 0.8 - X * 0.6) / 5.5 + t * 0.003));
                        const double nv = nlc * lit * patch * (0.45 + 0.55 * bands * billows) * (1.15 - 0.5 * smoothstep(0.05, 0.4, el));   // brightest low
                        if (nv > 0.02) paleV = std::max(paleV, 46.0 * nv);
                    }
                }
                if (pillar > 0 && cosSun > 0.85 && el > pillarElLo && el < pillarElHi && std::fabs(d.x * sdh.z - d.z * sdh.x) < pillarTanAz * (d.x * sdh.x + d.z * sdh.z)) {
                    double dAz = std::atan2(d.x * sdh.z - d.z * sdh.x, d.x * sdh.x + d.z * sdh.z);
                    double dAlt = std::asin(clampd(el, -1, 1)) - sunAltA;
                    if (std::fabs(dAz) < 2.5 * DEG && dAlt > -2 * DEG && dAlt < 11 * DEG)
                        v += 8.0 * pillar * std::exp(-(dAz / (0.9 * DEG)) * (dAz / (0.9 * DEG))) * std::exp(-std::max(0.0, dAlt) / (4.5 * DEG)) * (dAlt < 0 ? std::exp(dAlt / (1.0 * DEG)) : 1.0);
                }
                if (wedge > 0 && el > 0 && el < 0.12) {
                    double opp = -(d.x * sdh.x + d.z * sdh.z);
                    if (opp > 0.5) v *= 1 - 0.28 * wedge * (opp - 0.5) / 0.5 * (1 - el / 0.12);
                }
                if (clouds) {
                    int gx = x / CELL, gy = y / CELL;
                    double fx = (x - gx * CELL) / (double)CELL, fy = (y - gy * CELL) / (double)CELL;
                    const float* c00 = &cloudGrid[(gy * GX + gx) * 3];
                    const float* c10 = &cloudGrid[(gy * GX + gx + 1) * 3];
                    const float* c01 = &cloudGrid[((gy + 1) * GX + gx) * 3];
                    const float* c11 = &cloudGrid[((gy + 1) * GX + gx + 1) * 3];
                    double dens = (c00[0] * (1 - fx) + c10[0] * fx) * (1 - fy) + (c01[0] * (1 - fx) + c11[0] * fx) * fy;
                    double ci = (c00[1] * (1 - fx) + c10[1] * fx) * (1 - fy) + (c01[1] * (1 - fx) + c11[1] * fx) * fy;
                    v = v + (ci - v) * dens;
                    paleV *= 1 - dens;   // Q-02: the clouds under them hide them
                    stormW = ((c00[2] * (1 - fx) + c10[2] * fx) * (1 - fy) + (c01[2] * (1 - fx) + c11[2] * fx) * fy) * dens;   // W-03
                }
                if (curtain && el < 0.45) {   // over the clouds: the rain hangs in front of the deck's base, up to where the ray meets it
                    double hl = std::sqrt(d.x * d.x + d.z * d.z);
                    if (hl > 1e-6) {
                        double g2 = (d.x * localFront.nx + d.z * localFront.nz) / hl;   // metres ahead per metre of ground travelled
                        double reach = el > 0.012 ? std::min(2600.0 / el * hl, 60000.0) : 60000.0;   // the ground distance to the cloud base, or far
                        double ext = localFront.wall(localFront.ahead0, localFront.ahead0 + g2 * reach, reach / hl, env.frontRain);
                        if (ext > 1e-4) { double e = 1 - std::exp(-ext); v = v + (curtainV - v) * e; stormW = std::max(stormW, e); }
                    }
                }
            }
            if (ringArc && el > 0.0) {
                Vec3 dW = toWorld * d;
                double denom = dot(dW, ringN);
                if (std::fabs(denom) > 1e-6) {
                    double s = -dot(ringOrig, ringN) / denom;
                    if (s > 0) {
                        Vec3 P = ringOrig + dW * s;
                        double rr = length(P);
                        if (rr >= ringR0 && rr <= ringR1) {
                            double u = (rr - ringR0) / (ringR1 - ringR0);
                            double dens = (*ringProf)[clampi((int)(u * 255), 0, 255)];
                            // the world's own shadow falls across the ring at night: that part stays dark
                            double tcs = -dot(P, ringSunW);
                            bool shadowed = tcs > 0 && length2(P + ringSunW * tcs) < b.radiusKm * b.radiusKm;
                            if (dens > 0.02 && !shadowed) {
                                // O0-01 (B-301): the arc reads like a moon: pale and bright on the sky bank (it used to peak at
                                // shade 39, which is near black on that bank). Opening angle, the face we see and the light factor
                                double litRing = std::fabs(dot(ringN, ringSunW)) * 0.7 + 0.3;
                                double lit = (0.55 + 0.45 * litRing) * ringFace * std::max(ringLf, 0.5);
                                double rv = 36 + 24 * ringPow[clampi((int)(u * 255), 0, 255)] * lit;   // the sky bank is black below 30
                                if (site.atmosphere) rv = rv * (1 - 0.3 * day) + v * 0.45 * day;   // washed by daylight like the day moon
                                if (rv > v) v = rv;
                            }
                        }
                    }
                }
            }
            Pix pv = pix(stormW > 0.5 ? 23 : 1, clampd(v, 0, 51.0));   // W-03: a front's deck in the storm bank
            if (paleV > 0.6 * v && paleV > 1.5 && stormW <= 0.5) pv = pix(22, clampd(std::max(paleV, 0.6 * v), 0, 50.0));   // Q-02: over the night's dark sky
            if (tailV > v) pv = pix(15, clampd(tailV, 0, 50));   // O4: the comet's tail in its own blue bank
            if (step == 1) fb.idx[o] = pv;
            else {
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) fb.idx[yy * FBW + xx] = pv;
            }
        }
    }
    });
    // other bodies of the system in the sky (they occlude stars)
    if (!venus) {
        SpaceContext sc;
        sc.sys = site.sys; sc.t = t;
        sc.shipPos = site.worldPos(t, player.x, player.z, camPos.y / 1000.0);
        sc.cam = camLocal * site.localFrame(t);
        sc.excludeBody = site.body;
        sc.skyMode = true;
        sc.skyDark = site.atmosphere ? 1 - day : 1;
        sr.drawSkyBodies(fb, sc);
    }
    // stars, dimmed by sky brightness
    double starScale = (site.atmosphere ? (1.0 - 0.9 * day) * (1 - env.rain) : 1.0) * (site.atmosphere ? KINDS[site.gen.type].starLight : 1.0);   // D-04: a thick air's share
    if (starScale > 0.03 && !venus) {
        Mat3 L2 = site.localFrame(t);
        Mat3 cam = camLocal * L2;
        Vec3 obs = site.worldPos(t, player.x, player.z, camPos.y / 1000.0);
        drawStarField(fb, stars, obs, cam, proj, starScale * 0.85, 4, false, fb.idx.data(), 22.0, 0, Vec3(), site.atmosphere ? 1.0 : 0.0, t);
    }
    // G-03: the galactic band (and the nebula patches, N5-01) over the sky pixels, after the bodies and the stars, as the aurora
    // below: the map's brightness in bank 22, hidden by the clouds, written where its light beats the sky pixel's own (both
    // read from the palette, since the two ramps differ); a star in front keeps its pixel, a sky body's disc is brighter.
    // The band used to be added to the sky's value in the sky bank, whose night ramp is too dark to show it.
    // B-408: an airless sky is the band's bank all over, the sky's own light (a comet's coma, the halo round the sun) carried
    // into it by its light. The suns' glows add on what lies under them in the pixel's bank: they lit the band's pixels in the
    // band's ramp and the sky's in the sky's, which is black under shade 30, so where the band's patch ended the glow round a
    // sun had a hard edge (a pale wedge with straight sides across the sun of a comet); now it lands on one bank
    // whatever lies behind it. It reaches down to the horizon of the drawn ground, not to the horizontal (where it ended in a
    // straight edge across the sky of a small body, whose sky shows under the horizontal all round), and lies behind the bodies
    const double bandGain = venus ? 0 : 20.0 * (site.atmosphere ? (1.0 - day) * (1 - env.rain) * KINDS[site.gen.type].starLight : 1.0);   // the bulge 17 shades on a dark night, the plane 6-8, the wings under 2 left to the sky
    NebulaPatch nebAll[18]; int nebAllN = nebN;   // S-04: the field's patches and a protostar's own cloud round the star's direction now
    for (int k = 0; k < nebN; k++) nebAll[k] = nebP[k];
    nebAllN += starNebulaPatches(site.sys->star, normalize(site.sys->star.pos - site.sys->bodyPos(site.body, t)), nebAll + nebAllN);
    double bandMinEl = -1.0;
    if (!site.smallBody) { double c = (site.R + floorH) / (site.R + std::max(camPos.y, floorH)); bandMinEl = -std::sqrt(std::max(0.0, 1 - c * c)); }
    // W-04: the zodiacal light, the dust of a young system or a near belt lit by the sun (`zodiacalLight`: a cone along the plane
    // of the worlds from the sun, the faint patch opposite it), added to the band's light as the band is drawn and faded by day
    // as the band is; smooth, so it is read on the cell grid and interpolated
    const bool zodiOn = zodiDust >= ZODIACAL_MIN_DUST && bandGain > 0.05;
    if (zodiOn) {
        zodiGrid.assign(GX * GY, 0.f);
        const Vec3 sunW = toWorld * env.sunDisc.dirLocal;
        const double gain = bandGain * zodiDust;
        parallelFor(GY, 4, [&](int gy0, int gy1) {   // F-05: the rows on the pool (zodiacalLight is pure)
            for (int gy = gy0; gy < gy1; gy++)
                for (int gx = 0; gx < GX; gx++) {
                    int px = std::min(gx * CELL, FBW - 1), py = std::min(gy * CELL, FBH - 1);
                    Vec3 dW = toWorld * (camT * dirLUT[py * FBW + px]);
                    zodiGrid[gy * GX + gx] = (float)(gain * zodiacalLight(dW, sunW));
                }
        });
    }
    const bool airless = !site.atmosphere;
    if (bandGain > 0.05) {
        double lumSky[64], lumBand[64];
        for (int k = 0; k < 64; k++) {
            const uint8_t* c1 = &fb.pal[(1 * 64 + k) * 3]; lumSky[k] = 0.2126 * c1[0] + 0.7152 * c1[1] + 0.0722 * c1[2];
            const uint8_t* c2 = &fb.pal[(22 * 64 + k) * 3]; lumBand[k] = 0.2126 * c2[0] + 0.7152 * c2[1] + 0.0722 * c2[2];
        }
        // airless: the intensity in the band's bank with the light of each intensity in the sky's (both ramps rise from black;
        // the halo the airless sky draws round a sun is no light of a hole's)
        std::vector<int> skyToBand;
        const double cosDisc1 = site.sys->star.cls == STAR_BLACK_HOLE ? 2.0 : std::cos(env.sunDisc.angularRadius), cosDisc2 = std::cos(env.sun2.angularRadius);   // a hole has no disc (and its lens shows what lies behind it)
        if (airless) {
            skyToBand.assign(INTEN_MASK + 1, 0);
            auto lumAt = [](const double* lum, double sh) { int k = std::min(62, (int)sh); return lum[k] + (lum[k + 1] - lum[k]) * std::min(1.0, sh - k); };
            const bool ownLight = site.sys->star.cls != STAR_BLACK_HOLE;
            int k = 0;
            for (int i = 0; i <= INTEN_MASK && ownLight; i++) {
                double L = lumAt(lumSky, std::min(63.0, i / (double)INTEN_PER_SHADE));
                while (k < 62 && lumBand[k + 1] < L) k++;
                double f = lumBand[k + 1] > lumBand[k] ? clampd((L - lumBand[k]) / (lumBand[k + 1] - lumBand[k]), 0, 1) : 0.0;
                skyToBand[i] = (int)((k + f) * INTEN_PER_SHADE + 0.5);
            }
        }
        parallelFor(rowsTotal, 40, [&](int rb, int re) {
        for (int y = rb * step; y < re * step && y < FBH; y += step) {
            for (int x = 0; x < FBW; x += step) {
                int o = y * FBW + x;
                if (bankOf(fb.idx[o]) != 1) continue;
                Vec3 d = camT * dirLUT[o];
                if (d.y <= bandMinEl) continue;
                Vec3 dW = toWorld * d;
                double band = bandAt(dW);
                if (nebAllN) { int tone; band = std::max(band, 0.9 * nebulaGlow(nebAll, nebAllN, dW, tone)); }
                double bs = band * bandGain;
                if (zodiOn) {   // W-04
                    int gx = x / CELL, gy = y / CELL;
                    double fx = (x - gx * CELL) / (double)CELL, fy = (y - gy * CELL) / (double)CELL;
                    const float* z0 = &zodiGrid[gy * GX + gx]; const float* z1 = z0 + GX;
                    bs += (z0[0] * (1 - fx) + z0[1] * fx) * (1 - fy) + (z1[0] * (1 - fx) + z1[1] * fx) * fy;
                }
                if (clouds) {   // the clouds hide it
                    int gx = x / CELL, gy = y / CELL;
                    double fx = (x - gx * CELL) / (double)CELL, fy = (y - gy * CELL) / (double)CELL;
                    double dens = (cloudGrid[(gy * GX + gx) * 3] * (1 - fx) + cloudGrid[(gy * GX + gx + 1) * 3] * fx) * (1 - fy) + (cloudGrid[((gy + 1) * GX + gx) * 3] * (1 - fx) + cloudGrid[((gy + 1) * GX + gx + 1) * 3] * fx) * fy;
                    bs *= 1 - dens;
                }
                if (airless) {   // B-408: every sky pixel, the band added to the sky's own light
                    // but those a sun's disc covers: its disc is never dimmer than the sky round it, read from the shade there
                    if (dot(d, env.sunDisc.dirLocal) > cosDisc1 || (env.hasSun2 && dot(d, env.sun2.dirLocal) > cosDisc2)) continue;
                    for (int yy = y; yy < y + step && yy < FBH; yy++)
                        for (int xx = x; xx < x + step && xx < FBW; xx++) {
                            Pix& p = fb.idx[yy * FBW + xx];
                            if (bankOf(p) != 1 || fb.invz[yy * FBW + xx] > 1e-12f) continue;
                            p = pixI(22, std::min(INTEN_MASK, skyToBand[intenOf(p)] + (int)(bs * INTEN_PER_SHADE + 0.5)));
                        }
                    continue;
                }
                if (bs < 1.5) continue;   // the wings would only lift the sky by a level or two: leave the sky bank its pixels
                int bi = std::min(63, (int)bs);
                Pix pv = pix(22, bs);
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) {
                        Pix& p = fb.idx[yy * FBW + xx];
                        if (bankOf(p) != 1 || lumBand[bi] <= lumSky[std::min(63, intenOf(p) / INTEN_PER_SHADE)]) continue;
                        if (fb.invz[yy * FBW + xx] > 1e-12f) continue;   // B-408: behind every body, as the stars (a night side is the sky's own pixel with its depth)
                        p = pv;
                    }
            }
        }
        });
    }
    // Q-02 (TR_AIRGLOW): the oxygen's own glow over the night sky, as the aurora is drawn (banks 11 and 21 over the sky's pixels where it beats
    // them): the green line's layer 97 km up brightest some ten degrees over the horizon, where the eye looks along it, the red line's
    // fainter higher up, both rippled by the gravity waves that cross the layer in bands tens of kilometres apart; gone with the day, the rain
    // and the clouds
    const double airglow = site.gen.hasTrait(TR_AIRGLOW) && site.atmosphere && !venus ? (1 - smoothstep(0.02, 0.18, day)) * (1 - env.rain) * (1 - 0.8 * env.cloudCover) : 0.0;
    if (airglow > 0.02) {
        const double wx0 = camPos.x / 1000.0, wz0 = camPos.z / 1000.0, ph = t / 900.0;
        parallelFor(rowsTotal, 40, [&](int rb, int re) {
        for (int y = rb * step; y < re * step && y < FBH; y += step) {
            for (int x = 0; x < FBW; x += step) {
                const int o = y * FBW + x;
                const int pb = bankOf(fb.idx[o]);
                if ((pb != 1 && pb != 22) || fb.invz[o] > 1e-12f) continue;
                const Vec3 d = camT * dirLUT[o];
                const double el = d.y;
                if (el < 0.005) continue;
                const double sl = 97.0 / std::max(el, 0.06);   // where the ray meets the layer (km): the waves' place
                const double X = wx0 + d.x * sl, Z = wz0 + d.z * sl;
                // the waves where the ray is steep enough to resolve their 40 km (low down they average out)
                const double waves = 1.0 - 0.3 * smoothstep(0.12, 0.3, el) * (0.5 + 0.5 * std::sin((X * 0.6 + Z * 0.8) / 6.5 + ph + 1.5 * gnoise2(X / 120.0, Z / 120.0, site.gen.seed + 95)));
                const double green = airglow * 11.0 * smoothstep(0.0, 0.06, el) * (0.35 + 0.65 * std::exp(-(el - 0.15) * (el - 0.15) / 0.015)) * waves;
                const double red = airglow * 4.5 * smoothstep(0.2, 0.35, el) * std::exp(-(el - 0.5) * (el - 0.5) / 0.04) * waves;
                const bool g = green >= red;
                const double ae = g ? green : red;
                if (ae < 1.5) continue;
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) {
                        Pix& p = fb.idx[yy * FBW + xx];
                        const int b0 = bankOf(p);
                        if (b0 == 1) { if (shadeOf(p) < ae) p = pix(g ? 11 : 21, ae); }
                        else if (b0 == 22) p = pix(g ? 11 : 21, ae + 0.6 * shadeOf(p));   // the galactic band's light shows through it (added, not hidden)
                    }
            }
        }
        });
    }
    // B-403: the aurora over the sky pixels, after the bodies and the stars: the grid's value interpolated per pixel, written
    // in bank 11 (or 21 where the tops' colour has the greater share) where it beats the pixel's own shade on the sky bank. The
    // stars (bank 4) show through it; a sky body's lit disc is brighter than it and stays, its night side (depth only, the
    // sky's own pixel) shows it in front: the curtains hang 100-250 km up, far under any moon or parent
    if (auroraOn) {
        parallelFor(rowsTotal, 40, [&](int rb, int re) {
        for (int y = rb * step; y < re * step && y < FBH; y += step) {
            int gy = y / CELL;
            double fy = (y - gy * CELL) / (double)CELL;
            const float* r0 = &auroraGrid[gy * GX * 2];
            const float* r1 = &auroraGrid[(gy + 1) * GX * 2];
            for (int x = 0; x < FBW; x += step) {
                int gx = x / CELL;
                float c00 = r0[gx * 2], c10 = r0[gx * 2 + 2], c01 = r1[gx * 2], c11 = r1[gx * 2 + 2];
                if (c00 <= 0 && c10 <= 0 && c01 <= 0 && c11 <= 0) continue;
                double fx = (x - gx * CELL) / (double)CELL;
                double w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
                double av = c00 * w00 + c10 * w10 + c01 * w01 + c11 * w11;
                if (av < 0.5) continue;
                double tops = (c00 * r0[gx * 2 + 1] * w00 + c10 * r0[gx * 2 + 3] * w10 + c01 * r1[gx * 2 + 1] * w01 + c11 * r1[gx * 2 + 3] * w11) / av;
                int bank = tops > 0.5 ? 21 : 11;
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) {
                        int o = yy * FBW + xx;
                        Pix& p = fb.idx[o];
                        if (bankOf(p) != 1 && bankOf(p) != 22) continue;   // G-03: over the galactic band's pixels too (its ramp is as bright per shade)
                        double sky = shadeOf(p), ae = bankOf(p) == 1 ? av * smoothstep(26.0, 6.0, sky) : av;   // the twilight glow washes it out smoothly rather than cutting holes in it
                        if (sky >= ae) continue;
                        p = pix(bank, ae);
                    }
            }
        }
        });
    }
    // M10-14: meteors at night: a short streak that lives half a second, a few per minute
    if (starScale > 0.3 && !venus) {
        uint64_t slot = (uint64_t)(t * 2.0);
        Rng mr(slot ^ (site.gen.seed << 1));
        if (mr.chance(0.06)) {
            double az = mr.range(0, TAU), el = mr.range(0.25, 1.2), len = mr.range(0.05, 0.12), dirA = mr.range(0, TAU);
            double f = t * 2.0 - (double)slot;
            Vec3 p0(std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az));
            Vec3 tang = normalize(cross(p0, Vec3(std::sin(dirA), 0.3, std::cos(dirA))));
            Vec3 a = normalize(p0 + tang * (len * f)), bb = normalize(p0 + tang * (len * (f + 0.25)));
            Vec3 va = camLocal * a, vb = camLocal * bb;
            RVert ra, rb;
            ra.x = va.x * 900; ra.y = va.y * 900; ra.z = va.z * 900; ra.shade = 40 * starScale;
            rb.x = vb.x * 900; rb.y = vb.y * 900; rb.z = vb.z * 900; rb.shade = 55 * starScale;
            if (va.z > 0.05 && vb.z > 0.05) rasterLine3(fb, ra, rb, 4, proj, true, 1);
        }
    }
    // W-04: a meteor shower's streaks (`showerMeteorsAt`), drawn as the sporadic ones in the stars' bank
    if (starScale > 0.3 && !venus && !showers.empty()) {
        std::vector<ShowerMeteor> ms; showerMeteorsAt(t, ms);
        for (const ShowerMeteor& m : ms) {
            Vec3 va = camLocal * m.tail, vb = camLocal * m.head;
            if (va.z <= 0.05 || vb.z <= 0.05 || m.bright * starScale <= 4) continue;
            RVert ra, rb;
            ra.x = va.x * 900; ra.y = va.y * 900; ra.z = va.z * 900; ra.shade = 0.55 * m.bright * starScale;
            rb.x = vb.x * 900; rb.y = vb.y * 900; rb.z = vb.z * 900; rb.shade = m.bright * starScale;
            rasterLine3(fb, ra, rb, 4, proj, true, 1);
        }
    }
    // M5-01: the companion sun's disc, in its own colour (bank 12), drawn first so the primary's flare wins
    if (twoSuns) {
        Vec3 v = camLocal * sd2;
        if (sd2.y > -0.05 || !site.atmosphere) {
            double haze = site.atmosphere ? (0.6 + 0.4 * env.cloudCover) : 0.0;
            double sunI = (venus ? 0.55 : 1.0) * (site.atmosphere ? clampd(0.55 + 0.45 * smoothstep(-0.05, 0.1, sd2.y), 0, 1) : 1.0) * clampd(0.5 + 0.5 * sun2Up, 0.5, 1.0);
            SpaceRenderer::drawSun(fb, site.sys->companionStar(), v, env.sun2.angularRadius, t, sunI, 12, site.atmosphere, haze, true, proj);   // B-308: by angle
        }
    }
    // the sun
    sunVeil = 0;
    if (site.sys->sunless()) return;   // D-01: a drifter's sky has none (its place at the nadir is only where the readers of the sun find night)
    {
        const Vec3 sdd = env.sunDisc.dirLocal;
        const double sinAltD = sdd.y;
        Vec3 v = camLocal * sdd;
        {
            // the sun is depth-tested against the bodies drawn before it, so a moon or the parent
            // planet carves a partial eclipse out of the disc (M1-08); the corona grows as the disc goes
            double inten = venus ? 0.55 : 1.0;
            double haze = site.atmosphere ? (0.6 + 0.4 * env.cloudCover) : 0.0;
            if (sinAltD > -0.05 || !site.atmosphere) {
                double sunI = inten * (site.atmosphere ? clampd(0.55 + 0.45 * smoothstep(-0.05, 0.1, sinAltD), 0, 1) : 1.0) * clampd(0.5 + 0.5 * env.primaryRel, 0.5, 1.0);   // B-425: a far primary as the companion's disc is drawn
                Vec3 discUp = camLocal * (site.localFrame(t) * Vec3(0, 1, 0));   // S-04: the orbital plane's normal, for a protostar's disc
                // B-408: on an airless sky in the band's bank, which the sky lies in (a hole's disc and a protostar's keep the sky bank's colours)
                const int sunBank = !site.atmosphere && site.sys->star.cls != STAR_BLACK_HOLE && site.sys->star.cls != STAR_PROTOSTAR ? 22 : 1;
                SpaceRenderer::drawSun(fb, site.sys->star, v, env.sunDisc.angularRadius, t, sunI, sunBank, site.atmosphere, haze, true, proj, &discUp);   // B-308: by angle
                if (v.z <= 0.001) return;   // the eclipse glow and the flare need the projected centre
                double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
                double rpx = proj.f * std::tan(env.sunDisc.angularRadius) / v.z;
                // B-309: the veiling glare over the finished picture (drawVeil), from the sun's place and brightness
                sunSx = sx; sunSy = sy; sunRpx = rpx;
                sunVeil = site.sys->star.cls == STAR_BLACK_HOLE ? 0.0 : sunI * (1 - env.sunDisc.eclipse) * (site.atmosphere ? (1 - 0.6 * env.cloudCover) * (1 - env.rain) * smoothstep(-0.02, 0.12, sinAltD) : 0.7) * env.primaryRel;   // S-06: no glare from a hole; B-425: nor much from a far primary
                if (env.sunDisc.eclipse > 0.3 && !venus) {
                    double r = std::max(rpx, 1.5 * FB_SCALE), e = env.sunDisc.eclipse;
                    fb.glowDisc(sx, sy, r * 4.0, r * 1.05, (int)(34 * e * e), sunBank, true);
                    fb.glowDisc(sx, sy, r * 9.0, r * 1.1, (int)(10 * e * e), sunBank, true);
                }
                int ix = (int)sx, iy = (int)sy;
                bool centreFree = ix >= 0 && iy >= 0 && ix < FBW && iy < FBH && fb.invz[iy * FBW + ix] <= 1e-12f;
                // M1-03 lens flare, dimmed by haze and rain, only with the sun well up and unobstructed
                double flareI = sunI * (1 - env.sunDisc.eclipse) * (site.atmosphere ? (1 - 0.5 * env.cloudCover) * (1 - env.rain) * smoothstep(0.0, 0.15, sinAltD) : 0.8) * env.primaryRel;
                if (!venus && centreFree && flareI > 0.05 && site.sys->star.cls != STAR_BLACK_HOLE) SpaceRenderer::drawLensFlare(fb, sx, sy, std::max(rpx, 2.0 * FB_SCALE), flareI, sunBank, proj);   // S-06: nor a flare
            }
        }
    }
}

// W-04: the shower meteors alight at t. While a stream is active (`showerActivity`) meteors light up round its radiant (fixed
// among the stars: the system's axes through the local frame, so it rises and sets with them) and run away from it, up to
// SHOWER_PEAK_PER_MIN a minute at the peak with the radiant overhead (the rate times the sine of its altitude; none while it
// is under the horizon). Quarter-second slots hashed with the stream decide each one, so the sky is the same at a time; the
// nearer the radiant the shorter the path (it comes at you), a quick fade in and a slower one out, faster streams brighter
void SurfaceView::showerMeteorsAt(double t, std::vector<ShowerMeteor>& out) const {
    out.clear();
    if (showers.empty()) return;
    Mat3 Lw = site.localFrame(t);
    for (int k = 0; k < (int)showers.size(); k++) {
        const MeteorShower& s = showers[k];
        double act = showerActivity(s, t);
        if (act < 0.004) continue;
        Vec3 rad = Lw * s.radiant;
        if (rad.y <= 0.02) continue;
        double perSec = SHOWER_PEAK_PER_MIN / 60.0 * act * rad.y;
        const double SLOT = 0.25, LIFE = 1.0;
        Vec3 e1 = normalize(cross(rad, std::fabs(rad.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)));
        Vec3 e2 = cross(rad, e1);
        int64_t s0 = (int64_t)std::floor((t - LIFE) / SLOT), s1 = (int64_t)std::floor(t / SLOT);
        for (int64_t sl = s0; sl <= s1; sl++) {
            Rng r(mix64((uint64_t)sl * 0x9E3779B97F4A7C15ULL) ^ mix64((uint64_t)s.comet + 0x5A0EULL) ^ site.gen.seed);
            if (!r.chance(std::min(0.9, perSec * SLOT))) continue;
            double start = (sl + r.uni()) * SLOT, life = 0.3 + 0.6 * r.uni(), age = t - start;
            if (age < 0 || age > life) continue;
            double th0 = r.range(6, 70) * DEG, psi = r.uni() * TAU;
            double rate = r.range(16, 34) * DEG * (0.35 + 0.65 * std::sin(th0 + 0.3));   // radians a second along the sky
            double f = age / life;
            double head = th0 + rate * age, tail = std::max(th0, head - rate * 0.35);
            Vec3 side = e1 * std::cos(psi) + e2 * std::sin(psi);
            ShowerMeteor m;
            m.tail = rad * std::cos(tail) + side * std::sin(tail);
            m.head = rad * std::cos(head) + side * std::sin(head);
            if (m.tail.y < 0 && m.head.y < 0) continue;
            double fade = smoothstep(0.0, 0.25, f) * (1 - smoothstep(0.7, 1.0, f));
            m.bright = r.range(42, 62) * (0.6 + 0.4 * std::min(1.0, s.speedKms / 120.0)) * fade;
            m.shower = k;
            out.push_back(m);
        }
    }
}

// B-309: a veiling glare round the sun over the finished picture: a broad, faint brightening that the mush melts into
// the scene, in place of the HUD glint arcs (which spun round the screen centre and wobbled with every head bob). Its
// strength follows how much of the disc is still open to the sky once the ground, the trees and the objects are drawn,
// so it fades smoothly behind a ridge or a canopy instead of switching.
void SurfaceView::drawVeil(Framebuffer& fb) {
    if (sunVeil < 0.02 || sunSx < -FBW || sunSx > 2 * FBW || sunSy < -FBH || sunSy > 2 * FBH) return;
    double rr = std::max(sunRpx, 2.0 * FB_SCALE);
    int open = 0, n = 0;
    for (int j = -2; j <= 2; j++)
        for (int i = -2; i <= 2; i++) {
            int x = (int)(sunSx + i * rr * 0.4), y = (int)(sunSy + j * rr * 0.4);
            if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue;
            n++;
            if (fb.invz[y * FBW + x] <= 1e-12f) open++;
        }
    if (!n) return;
    double veil = sunVeil * open / n;
    if (veil < 0.02) return;
    double radius = 0.42 * FBW;
    fb.glowDisc(sunSx, sunSy, radius, rr * 1.5, (int)((site.atmosphere ? 5.0 : 3.0) * veil + 0.5), -1, false);
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
    const bool flora = kindHas(type, KT_FLORA), microbial = kindHas(type, KT_MICROBIAL), ventLife = kindHas(type, KT_VENT_LIFE);   // P-02: the stromatolites' pass on a microbial world alone (its std::function a cell otherwise); P-03 the chimneys' on a vent-life world
    const bool meteorites = site.gen.hasTrait(TR_METEORITES);   // Q-02
    RasterParams rp; rp.bank = (flora || KINDS[type].accent[1] >= 0) ? 0 : 3;   // N1-03: boulders in the rock colour (bank 3 is the forest on living worlds, P-00: an accent's where a kind lends it)
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
            lastRocksCandidates += forRocks(cx, cz, [&](double rx, double rz, double size) { const double dist = std::sqrt((rx - camPos.x) * (rx - camPos.x) + (rz - camPos.z) * (rz - camPos.z)); return size / dist * proj.f >= 0.8; },
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
                    if (nLamps) light += (1 - amb) * lampAt((p0 + p1 + peak) * (1.0 / 3.0), &n);   // D-04
                    double shade = 50 * std::pow(light, 0.6);
                    shade = fogShade(shade, dist);
                    RVert tri[3];
                    Vec3 vs[3] = {toView(p0.x, p0.y, p0.z), toView(p1.x, p1.y, p1.z), toView(peak.x, peak.y, peak.z)};
                    Vec3 ws[3] = {p0, p1, peak};
                    for (int q = 0; q < 3; q++) {
                        tri[q].x = vs[q].x; tri[q].y = vs[q].y; tri[q].z = vs[q].z; tri[q].shade = shade;
                        tri[q].u = ws[q].x + ws[q].y * 0.7; tri[q].v = ws[q].z + ws[q].y * 0.4;   // world-mapped facets
                    }
                    rasterTriangle(fb, tri, rp, proj);
                }
            });
            // Q-02: a strewn field's iron meteorites: dark lumps pitted with thumbprints, their fusion crust black-brown in the chrome's ramp,
            // a facet that faces the sun from the eye's side glinting
            if (meteorites) forMeteorites(cx, cz, [&](const RockInst& R) {
                const double dist = std::sqrt((R.x - camPos.x) * (R.x - camPos.x) + (R.z - camPos.z) * (R.z - camPos.z));
                if (R.size / dist * proj.f < 0.6) return;
                const double hgt = R.size * (0.55 + 0.35 * hash01(mix64(R.h + 4))), a0 = hash01(mix64(R.h + 3)) * TAU;
                const int NA = dist < 30 ? 6 : 4;
                if (R.size > 0.4) drawBlobShadow(fb, R.x, R.z, R.size * 0.9, hgt);
                Vec3 ring[6];
                for (int a = 0; a < NA; a++) { const double ang = a0 + a * TAU / NA, wob = 0.75 + 0.5 * hash01(mix64(R.h + 10 + a)); ring[a] = Vec3(R.x + std::cos(ang) * R.size * wob, R.gy - 0.05 + 0.3 * hgt * hash01(mix64(R.h + 20 + a)), R.z + std::sin(ang) * R.size * wob); }
                const Vec3 top(R.x + 0.2 * R.size * std::cos(a0), R.gy + hgt, R.z + 0.2 * R.size * std::sin(a0));
                const Vec3 toCam = normalize(Vec3(camPos.x - R.x, camPos.y - R.gy, camPos.z - R.z));
                for (int a = 0; a < NA; a++) {
                    const Vec3& p0 = ring[a]; const Vec3& p1 = ring[(a + 1) % NA];
                    Vec3 n = normalize(cross(p1 - p0, top - p0)); if (n.y < 0) n = -n;
                    const double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
                    const double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt((p0 + p1 + top) * (1.0 / 3.0), &n) : 0.0);   // D-04: the lamp
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
                    double shade = fogShade(46 + 14 * std::max(sunUp, std::min(1.0, nLamps ? lampAt(Vec3(rx, gy + 0.5 * hgt, rz)) : 0.0)) * hash01(mix64(hc + 5)), dist);   // D-04: the lamp lights its facets
                    RVert tri[3];
                    Vec3 vs[3] = {toView(p0.x, p0.y, p0.z), toView(p1.x, p1.y, p1.z), toView(pk.x, pk.y, pk.z)};
                    for (int q = 0; q < 3; q++) { tri[q].x = vs[q].x; tri[q].y = vs[q].y; tri[q].z = vs[q].z; tri[q].shade = shade; }
                    rasterTriangle(fb, tri, rp2, proj);
                }
            }
            // P-02: stromatolites, low lumpy domes dark with the mats that grow over them (the mats' bank, the kind's first accent: bank 10) on the
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
                    const double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt((p0 + p1 + p2) * (1.0 / 3.0), &n) : 0.0);   // D-04: the lamp
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
            // P-03: sinter chimneys, slim pale cones on the sinter round the springs (the sinter's bank, the kind's second accent: bank 3)
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
                    const double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt((p0 + p1 + p2) * (1.0 / 3.0), &n) : 0.0);   // D-04: the lamp
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
            if (nLamps) light = std::min(1.0, light + (1 - ambient) * lampAt(Vec3(cx + n.x * rad, cy + 0.2 + n.y * rad * 0.8, cz + n.z * rad), &n));   // D-04
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

// O4 (R-303): on an active comet the sunlit ground vents: hashed vents on a 250 m grid, each a column of dust
// points rising a few metres a second for half a minute and bending away from the sun (radiation pressure)
void SurfaceView::drawCometJets(Framebuffer& fb, double t) {
    double act = env.cometActivity;
    const Vec3& sd = env.sun.dirLocal;
    if (act < 0.05 || sd.y < 0.03) return;
    double horiz = std::sqrt(sd.x * sd.x + sd.z * sd.z);
    double ax = horiz > 1e-4 ? -sd.x / horiz : 1, az = horiz > 1e-4 ? -sd.z / horiz : 0;   // away from the sun, along the ground
    const double cs = 250;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
    for (int cz = pcz - 3; cz <= pcz + 3; cz++)
        for (int cx = pcx - 3; cx <= pcx + 3; cx++) {
            uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0x1E75);
            if (hash01(h) > 0.06 + 0.16 * act) continue;
            double vx = cx * cs + hash01(mix64(h + 1)) * cs, vz = cz * cs + hash01(mix64(h + 2)) * cs;
            double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 750) continue;
            double gy = site.groundHeight(vx, vz);
            double strength = 0.5 + 0.5 * hash01(mix64(h + 3));
            double rise = (3.0 + 4.0 * strength) * (0.5 + 0.5 * act);   // m/s
            int n = (int)(18 + 30 * act * strength);
            Rng r(h);
            for (int i = 0; i < n; i++) {
                double ph = r.uni(), a = r.range(0, TAU), spread = r.range(0.3, 1.0);
                double age = std::fmod(t * 0.5 + ph * 30.0, 30.0);   // seconds since this puff left the vent
                double up = rise * age, out = (0.6 + 0.6 * age * 0.25) * spread;
                double px = vx + std::cos(a) * out + ax * age * 1.2 * spread, pz = vz + std::sin(a) * out + az * age * 1.2 * spread;
                Vec3 v = toView(px, gy + up, pz);
                if (v.z < NEAR_Z) continue;
                RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 60 - age * 1.3;
                rasterPoint3(fb, p, 8, proj, true, dist < 120 && age < 8 ? 2 : 1);
            }
            {   // the vent itself: a bright spot of fresh ice
                Vec3 v = toView(vx, gy + 0.2, vz);
                if (v.z > NEAR_Z) { RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 62; rasterPoint3(fb, p, 8, proj, true, 2); }
            }
        }
}

// R-307: the geysers and the fountains. A vent's jet (`drawJet`) is a dense core column of camera-facing quads, cut ragged
// at its edges by the boundary tile and streaked by the fine grain scrolling up at the jet's own speed, bright at the throat
// and fraying toward the top; a crown of spray where it breaks; and a veil of drops falling back on ballistic arcs in the
// world's gravity, widening as they fall. Its height and width follow the vent's class (small, tall, huge by hash) and its
// strength this frame (`jetStrength`): about a third of the vents blow steadily, pulsing a little, the rest in cycles of one
// to three minutes that start hard (a third over full height in the first seconds) and die over the last tenth
double SurfaceView::jetStrength(uint64_t h, double t, bool constant, double cycle, double duty) {
    double phase = hash01(mix64(h + 5));
    if (constant) return 0.82 + 0.18 * std::sin(t * 0.9 + phase * TAU) + 0.04 * std::sin(t * 6.3 + phase * 11.0);
    double ph = std::fmod(t / cycle + phase, 1.0);
    if (ph > duty) return 0;
    double build = smoothstep(0.0, 0.035, ph), burst = 1.0 + 0.35 * std::exp(-ph / 0.05);
    return build * burst * smoothstep(duty, duty - 0.1, ph);
}

double SurfaceView::burstJetStrength(uint64_t id, double t) { return jetStrength(id, t, false, 30 + 60 * hash01(mix64(id + 3)), 0.15 + 0.15 * hash01(mix64(id + 4))); }

double SurfaceView::basinJetStrength(uint64_t id, double t) {
    bool constant = hash01(mix64(id + 9)) < 0.3;
    return jetStrength(id, t, constant, 60 + 120 * hash01(mix64(id + 3)), 0.2 + 0.3 * hash01(mix64(id + 4)));
}

void SurfaceView::drawJet(Framebuffer& fb, double t, const JetSpec& j, double strength) {
    if (strength <= 0.02) return;
    double gAcc = std::max(0.3, site.gravity);
    double sunUp = smoothstep(-0.03, 0.06, env.sun.dirLocal.y);
    double H = j.heightM * (0.55 + 0.45 * std::min(strength, 1.35)), W = j.widthM * (0.7 + 0.3 * std::min(strength, 1.0));
    double gy = site.groundHeight(j.x, j.z);
    double baseShade = j.lava ? 61 : (site.sys->sunless() ? 20 : 38 + 22 * sunUp) + j.shadeAdd;   // steam and ice are lit by the sun; lava lights itself (D-04: in a drifter's dark only faintly seen)
    double vJet = std::sqrt(2 * gAcc * H);                // the ejection speed: the streaks climb the column at it
    // the core column: eight segments, each two quads round the axis so the weight peaks at the centre and the tile cuts the sides ragged
    {
        RasterParams rp;
        rp.bank = j.bank; rp.grain = &grainFine; rp.grainScale = 0.9; rp.grainWeight = 1.0;
        rp.cutout = true; rp.edge = &grainEdge; rp.edgeScale = 0.35 / std::max(1.0, W * 0.15); rp.edgeAmp = 0.75;
        const int NSEG = 8;
        for (int k = 0; k < NSEG; k++) {
            double f0 = (double)k / NSEG, f1 = (double)(k + 1) / NSEG;
            double y0 = H * f0, y1 = H * f1;
            double r0 = W * 0.5 * (0.7 + 1.4 * std::pow(f0, 1.6)), r1 = W * 0.5 * (0.7 + 1.4 * std::pow(f1, 1.6));
            Vec3 c0 = toView(j.x, gy + y0, j.z), c1 = toView(j.x, gy + y1, j.z);
            if (c0.z < NEAR_Z || c1.z < NEAR_Z) continue;
            double core0 = 1.15 * (1 - 0.55 * f0 * f0), core1 = 1.15 * (1 - 0.55 * f1 * f1);   // the column frays toward the top
            double sh0 = baseShade - (j.lava ? 9 : 14) * f0, sh1 = baseShade - (j.lava ? 9 : 14) * f1;
            double v0 = (y0 - t * vJet) * 0.35, v1 = (y1 - t * vJet) * 0.35;
            for (int side = -1; side <= 1; side += 2) {
                RVert q[4];
                q[0].x = c0.x; q[0].y = c0.y; q[0].z = c0.z; q[0].w = core0; q[0].shade = sh0 + 1; q[0].u = 0; q[0].v = v0;
                q[1].x = c0.x + side * r0; q[1].y = c0.y; q[1].z = c0.z; q[1].w = 0.2; q[1].shade = sh0 - 4; q[1].u = side * r0 * 1.5; q[1].v = v0;
                q[2].x = c1.x + side * r1; q[2].y = c1.y; q[2].z = c1.z; q[2].w = 0.2; q[2].shade = sh1 - 4; q[2].u = side * r1 * 1.5; q[2].v = v1;
                q[3].x = c1.x; q[3].y = c1.y; q[3].z = c1.z; q[3].w = core1; q[3].shade = sh1 + 1; q[3].u = 0; q[3].v = v1;
                rasterPolygon(fb, q, 4, rp, proj);
            }
        }
    }
    bool near = j.dist < 350;
    double s = std::min(strength, 1.35);
    // the crown: spray thrown out where the column breaks, falling from it for a few seconds
    {
        int n = (int)((near ? 340 : 130) * s);
        Rng r(j.h ^ 0xC20);
        for (int i = 0; i < n; i++) {
            double u = r.uni(), a = r.range(0, TAU), rr = W * (0.5 + 2.8 * std::sqrt(u));
            double tau = std::fmod(t * 1.3 + r.uni() * 3.0, 3.0);   // seconds since this drop left the crown
            double y = H * (0.86 + 0.22 * r.uni()) + 0.15 * vJet * tau - 0.5 * gAcc * tau * tau;
            if (y < 0) continue;
            double out = rr + tau * (2.0 + 0.1 * vJet) * (0.4 + 0.6 * u);
            Vec3 v = toView(j.x + std::cos(a) * out, gy + y, j.z + std::sin(a) * out);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = baseShade - 6 - 8 * (tau / 3.0);
            rasterPoint3(fb, p, j.bank, proj, true, near ? 2 : 1);
        }
    }
    // the veil: drops on full ballistic arcs from the throat, the outer ones widening as they fall
    {
        int n = (int)((near ? 300 : 110) * s);
        double flight = 2 * vJet / gAcc;
        Rng r(j.h ^ 0x7E1);
        for (int i = 0; i < n; i++) {
            double u = r.uni(), a = r.range(0, TAU), spread = r.range(0.15, 1.0), tilt = r.range(0.02, 0.2);
            double age = std::fmod(t * 0.7 + u * flight, flight);
            double up = vJet * std::cos(tilt) * age - 0.5 * gAcc * age * age;
            if (up < 0) continue;
            double out = W * 0.4 + vJet * std::sin(tilt) * age * spread;
            Vec3 v = toView(j.x + std::cos(a) * out, gy + up, j.z + std::sin(a) * out);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = baseShade - 4 - 10 * (age / flight);
            rasterPoint3(fb, p, j.bank, proj, true, near && age < flight * 0.5 ? 2 : 1);
        }
    }
    {   // the throat: a bright pool with the fallout splashing round it
        Vec3 v = toView(j.x, gy + 0.3, j.z);
        if (v.z > NEAR_Z) { RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = j.lava ? 62 : baseShade + 2; rasterPoint3(fb, p, j.bank, proj, true, near ? 3 : 2); }
    }
}

void SurfaceView::drawGeysers(Framebuffer& fb, double t) {
    // a vent's class by hash: small, tall or huge, and whether it blows steadily
    auto classOf = [](uint64_t h, double& hM, double& wM, const double hs[3], const double ws[3]) {
        double u = hash01(mix64(h + 7)), v = hash01(mix64(h + 8));
        int c = u < 0.45 ? 0 : (u < 0.85 ? 1 : 2);
        hM = hs[c] * (0.8 + 0.4 * v); wM = ws[c] * (0.8 + 0.4 * v);
    };
    const uint32_t events = KINDS[site.gen.type].events;
    if (events & EV_CRACK_VENTS) {   // the cracks: vents hashed on 300 m cells where the 16 m ring is stained
        static const double hs[3] = {160, 400, 800}, ws[3] = {7, 15, 30};
        const double cs = 300;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        for (int cz = pcz - 8; cz <= pcz + 8; cz++)
            for (int cx = pcx - 8; cx <= pcx + 8; cx++) {
                uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0x6E75);
                if (hash01(h) > 0.35) continue;
                double vx = cx * cs + hash01(mix64(h + 1)) * cs, vz = cz * cs + hash01(mix64(h + 2)) * cs;
                if (site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != MAT_DUST) continue;
                double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
                if (dist > 2500) continue;
                JetSpec j; j.x = vx; j.z = vz; j.h = h; j.dist = dist; j.bank = 8; j.lava = false;
                classOf(h, j.heightM, j.widthM, hs, ws);
                bool constant = hash01(mix64(h + 9)) < 0.35;
                drawJet(fb, t, j, jetStrength(h, t, constant, 90 + 90 * hash01(mix64(h + 3)), 0.25 + 0.3 * hash01(mix64(h + 4))));
            }
    }
    if (site.gen.hasTrait(TR_GEYSERS)) {   // the basins: a hot spring at each sinter mound's vent
        static const double hs[3] = {30, 80, 180}, ws[3] = {2.5, 5, 10};
        std::vector<GeyserVent> vents;
        geyserVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 2500) continue;
            if (site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != MAT_SALT) continue;   // where the landform put its sinter
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            classOf(gv.id, j.heightM, j.widthM, hs, ws);
            drawJet(fb, t, j, basinJetStrength(gv.id, t));
        }
    }
    if (events & EV_STEAM_BURSTS) {   // P-01: steam bursting now and then from the vents of the kind's own ground (the steam world's sinter mounds)
        const int vent = KINDS[site.gen.type].eventMaterial;
        std::vector<GeyserVent> vents;
        steamVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 2500) continue;
            if (site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != vent) continue;   // a mound in a field of vents
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            j.heightM = 50 + 150 * hash01(mix64(gv.id + 7)); j.widthM = 5 + 8 * hash01(mix64(gv.id + 8));
            drawJet(fb, t, j, burstJetStrength(gv.id, t));
        }
    }
    if (site.gen.hasTrait(TR_HOT_SPRINGS)) {   // Q-02: steam rising steadily off each terraced spring's vent pool
        std::vector<GeyserVent> vents;
        hotSpringsNear(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 1500) continue;
            if (site.lod0.at((int)std::floor(vx / 16 + 0.5), (int)std::floor(vz / 16 + 0.5)).material != MAT_BRINE) continue;   // where the landform put its pool
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            j.heightM = 6 + 14 * hash01(mix64(gv.id + 7)); j.widthM = gv.radiusKm * 1000.0 * 0.9;
            drawJet(fb, t, j, jetStrength(gv.id, t, true, 1, 1));
        }
    }
    if (events & EV_SPRINGS) {   // P-03: the chemotroph's hot springs: steam rising steadily off every pool its ground made (`springVents`), one in five a geyser now and then
        std::vector<GeyserVent> vents;
        springVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 1200) continue;
            if (site.lod0.at((int)std::floor(vx / 16 + 0.5), (int)std::floor(vz / 16 + 0.5)).material != MAT_BRINE) continue;   // a pool of the spring fields (the vertex nearest its centre)
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            const bool geyser = hash01(mix64(gv.id + 6)) < 0.2;
            j.heightM = geyser ? 20 + 40 * hash01(mix64(gv.id + 7)) : 3 + 7 * hash01(mix64(gv.id + 7));
            j.widthM = gv.radiusKm * 1000.0 * (geyser ? 0.25 : 0.7);
            drawJet(fb, t, j, geyser ? jetStrength(gv.id, t, false, 90 + 120 * hash01(mix64(gv.id + 3)), 0.15 + 0.15 * hash01(mix64(gv.id + 4))) : jetStrength(gv.id, t, true, 1, 1));
        }
    }
    if (events & EV_FUMAROLES) {   // fumaroles and hot springs on the sulphur crusts, steady fountains on the lava (the great ones come and go through the eruption list)
        const double cs = 200;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        for (int cz = pcz - 5; cz <= pcz + 5; cz++)
            for (int cx = pcx - 5; cx <= pcx + 5; cx++) {
                uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xF0DA);
                if (hash01(h) > 0.4) continue;
                double vx = cx * cs + hash01(mix64(h + 1)) * cs, vz = cz * cs + hash01(mix64(h + 2)) * cs;
                int mat = site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material;
                if (mat != MAT_SULPHUR && mat != MAT_LAVA) continue;
                double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
                if (dist > 1100) continue;
                JetSpec j; j.x = vx; j.z = vz; j.h = h; j.dist = dist;
                if (mat == MAT_LAVA) {   // a steady fountain of 30-90 m
                    j.bank = 2; j.lava = true; j.heightM = 30 + 60 * hash01(mix64(h + 7)); j.widthM = 3 + 5 * hash01(mix64(h + 8));
                    drawJet(fb, t, j, jetStrength(h, t, true, 1, 1));
                } else {
                    bool spring = hash01(mix64(h + 6)) < 0.25;   // one crust vent in four is a periodic hot spring, the rest steam steadily
                    j.bank = 8; j.lava = false;
                    j.heightM = spring ? 60 + 90 * hash01(mix64(h + 7)) : 15 + 25 * hash01(mix64(h + 7)); j.widthM = spring ? 4 + 4 * hash01(mix64(h + 8)) : 2 + 2 * hash01(mix64(h + 8));
                    drawJet(fb, t, j, jetStrength(h, t, !spring, 70 + 80 * hash01(mix64(h + 3)), 0.2 + 0.25 * hash01(mix64(h + 4))));
                }
            }
    }
}

// P-03: the chemotroph's spores at night: a haze of glowing points over each spring near the explorer (its pool and its mats), rising slowly on
// the warm air, drifting with the wind, pulsing and fading as they climb; in their own light (bank 15), so only the dark shows them
void SurfaceView::drawSpores(Framebuffer& fb, double t) {
    const double dark = 1 - smoothstep(0.2, 0.55, env.skyBrightness);
    if (dark <= 0.01) return;
    std::vector<GeyserVent> vents;
    springVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
    const double wx = std::sin(env.windDir), wz = std::cos(env.windDir), lean = 0.04 * env.windKnots;   // metres downwind a metre of rise
    for (const GeyserVent& gv : vents) {
        double vx, vz;
        site.localAt(gv.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 500) continue;
        if (site.lod0.at((int)std::floor(vx / 16 + 0.5), (int)std::floor(vz / 16 + 0.5)).material != MAT_BRINE) continue;
        const double gy = site.groundHeight(vx, vz), r = gv.radiusKm * 1000.0;
        const int n = (int)((dist < 150 ? 260 : 90) * dark);
        Rng sr(gv.id ^ 0x5B0EULL);
        for (int i = 0; i < n; i++) {
            const double a = sr.range(0, TAU), rr = r * (0.2 + 2.8 * std::pow(sr.uni(), 0.9)), top = 4 + 12 * sr.uni(), speed = 0.12 + 0.25 * sr.uni(), ph = sr.uni();
            const double age = std::fmod(t * speed / top + ph, 1.0), up = age * top;   // 0 at the ground .. 1 at its top
            const double sx = 0.5 * std::sin(t * 0.6 + 9.0 * ph), sz = 0.5 * std::cos(t * 0.5 + 7.0 * ph);
            const Vec3 v = toView(vx + std::cos(a) * rr + wx * lean * up + sx, gy + 0.2 + up, vz + std::sin(a) * rr + wz * lean * up + sz);
            if (v.z < NEAR_Z) continue;
            const double pulse = 0.65 + 0.35 * std::sin(t * (1.2 + 2.0 * ph) + 11.0 * ph);
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 30 + 33 * dark * pulse * (1 - 0.6 * age);
            rasterPoint3(fb, p, 15, proj, true, dist < 70 && ph < 0.4 ? 2 : 1);
        }
    }
}

// P-05: the cryovolcanic world's plumes (Triton's: Soderblom et al. 1990): from each active vent on its cap within 220 km (`plumeVents`; half of
// them active, by hash) a dark column 6-9 km high and 200-500 m wide, its top carried downwind along the prevailing wind (the ground's streaks'
// direction) as a flag 40-90 km long, thickening from a few hundred metres to two kilometres and fading; grey by day, a dim shape at night;
// the curvature hides the far columns' feet (`toView`); a far column is kept a pixel wide
void SurfaceView::drawPlumes(Framebuffer& fb, double t) {
    std::vector<GeyserVent> vents;
    plumeVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
    const double sunUp = smoothstep(-0.05, 0.15, env.sun.dirLocal.y);
    RasterParams rp;
    rp.bank = 9; rp.grain = &grainFine; rp.grainScale = 0.9; rp.grainWeight = 1.0;
    rp.cutout = true; rp.edge = &grainEdge; rp.edgeAmp = 0.75;
    for (const GeyserVent& v : vents) {
        if (!plumeActive(v.id)) continue;
        double vx, vz; site.localAt(v.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 220000) continue;
        double wx, wz; site.localAt(normalize(v.unit + prevailingWind(site.gen, v.unit) * (1000.0 / site.R)), wx, wz);   // a kilometre downwind
        double dx = wx - vx, dz = wz - vz; const double dl = std::max(1e-6, std::sqrt(dx * dx + dz * dz)); dx /= dl; dz /= dl;
        double water = 0;
        const double gy = dist < 40000 ? heightNear(vx, vz, dist, water) : (double)site.sampleAt(vx, vz, 2048).h;
        const double H = 6000 + 3000 * hash01(mix64(v.id + 2)), W = std::max(600 + 900 * hash01(mix64(v.id + 3)), dist * 0.006);
        const double Lf = std::min(v.radiusKm * 800.0, 90000.0), sway = 0.08 * std::sin(t * 0.05 + 6.0 * hash01(mix64(v.id + 4)));
        const double base = 20 + 16 * sunUp, ph = TAU * hash01(mix64(v.id + 5));
        const int NSEG = 10;
        rp.edgeScale = 0.35 / std::max(1.0, W * 0.15);
        for (int k = 0; k < NSEG; k++) {   // the column, a ribbon facing the camera, widening a little toward its top
            const double f0 = (double)k / NSEG, f1 = (double)(k + 1) / NSEG;
            const Vec3 c0 = toView(vx, gy + H * f0, vz), c1 = toView(vx, gy + H * f1, vz);
            if (c0.z < NEAR_Z || c1.z < NEAR_Z) continue;
            const double r0 = W * 0.5 * (1 + 0.5 * f0), r1 = W * 0.5 * (1 + 0.5 * f1), v0 = (H * f0 - t * 60) * 0.35, v1 = (H * f1 - t * 60) * 0.35;
            for (int side = -1; side <= 1; side += 2) {
                RVert q[4];
                q[0].x = c0.x; q[0].y = c0.y; q[0].z = c0.z; q[0].w = 1.15; q[0].shade = base - 4 * f0; q[0].u = 0; q[0].v = v0;
                q[1].x = c0.x + side * r0; q[1].y = c0.y; q[1].z = c0.z; q[1].w = 0.2; q[1].shade = base - 6 - 4 * f0; q[1].u = side * W * 0.75; q[1].v = v0;
                q[2].x = c1.x + side * r1; q[2].y = c1.y; q[2].z = c1.z; q[2].w = 0.2; q[2].shade = base - 6 - 4 * f1; q[2].u = side * W * 0.75; q[2].v = v1;
                q[3].x = c1.x; q[3].y = c1.y; q[3].z = c1.z; q[3].w = 1.15; q[3].shade = base - 4 * f1; q[3].u = 0; q[3].v = v1;
                rasterPolygon(fb, q, 4, rp, proj);
            }
        }
        // the flag: a cross of two ribbons along the wind (an upright one, seen from the side, and a flat one, seen from under it), each a
        // dense middle and ragged edges, billowing a little, thickening from 600 m to three kilometres and fraying out toward its end
        const int NF = 16;
        const double px = -dz, pz = dx;   // across the wind
        auto at = [&](double f, double& x, double& y, double& z, double& th) {
            const double sq = Lf * f, a = sway * f;
            x = vx + (dx + px * a) * sq; z = vz + (dz + pz * a) * sq;
            y = gy + H - 700 * (1 - smoothstep(0.0, 0.12, f)) + 900 * f + 260 * std::sin(sq / 9000.0 + ph + t * 0.02) * f;   // bending over from the column's top
            th = std::max(W * 0.7 + 1300 * f, (dist + sq) * 0.004);
        };
        for (int k = 0; k < NF; k++) {
            const double f0 = (double)k / NF, f1 = (double)(k + 1) / NF;
            double x0, y0, z0, th0, x1, y1, z1, th1;
            at(f0, x0, y0, z0, th0); at(f1, x1, y1, z1, th1);
            const double sh0 = base - 2 - 12 * f0, sh1 = base - 2 - 12 * f1, w0 = 1.15 - 0.6 * f0, w1 = 1.15 - 0.6 * f1;
            rp.edgeScale = 0.35 / std::max(1.0, th0 * 0.3);
            for (int ribbon = 0; ribbon < 2; ribbon++)
                for (int side = -1; side <= 1; side += 2) {
                    const Vec3 m0 = toView(x0, y0, z0), m1 = toView(x1, y1, z1);
                    const Vec3 e0 = ribbon == 0 ? toView(x0, y0 + side * th0, z0) : toView(x0 + px * side * th0, y0, z0 + pz * side * th0);
                    const Vec3 e1 = ribbon == 0 ? toView(x1, y1 + side * th1, z1) : toView(x1 + px * side * th1, y1, z1 + pz * side * th1);
                    if (m0.z < NEAR_Z || m1.z < NEAR_Z || e0.z < NEAR_Z || e1.z < NEAR_Z) continue;
                    RVert q[4];
                    q[0].x = m0.x; q[0].y = m0.y; q[0].z = m0.z; q[0].w = w0; q[0].shade = sh0; q[0].u = 0; q[0].v = Lf * f0 * 0.35 - t * 3;
                    q[1].x = e0.x; q[1].y = e0.y; q[1].z = e0.z; q[1].w = 0.15; q[1].shade = sh0 - 5; q[1].u = side * th0; q[1].v = Lf * f0 * 0.35 - t * 3;
                    q[2].x = e1.x; q[2].y = e1.y; q[2].z = e1.z; q[2].w = 0.15; q[2].shade = sh1 - 5; q[2].u = side * th1; q[2].v = Lf * f1 * 0.35 - t * 3;
                    q[3].x = m1.x; q[3].y = m1.y; q[3].z = m1.z; q[3].w = w1; q[3].shade = sh1; q[3].u = 0; q[3].v = Lf * f1 * 0.35 - t * 3;
                    rasterPolygon(fb, q, 4, rp, proj);
                }
        }
    }
}

// P-05: the dry ice world's spring (Kieffer 2006): while the hemisphere's sun climbs out of its winter (its season between -0.75 and 0.35 and
// rising) and the sun is up, gas under the seasonal frost bursts out at the spiders' centres (`springJets`) in jets of dark dust 15-60 m high,
// a minute or two each, a third of their cycle (the fans on the frost are what they lay)
bool SurfaceView::springJetsNow(double t) const {
    if (env.sun.dirLocal.y < 0.05 || !site.sys || site.body < 0) return false;
    const Body& b = site.sys->bodies[site.body];
    const double dt = std::max(3600.0, 0.002 * std::fabs(b.orbitPeriod));
    const double north = site.unitAt(camPos.x, camPos.z).z >= 0 ? 1 : -1;
    const double s0 = north * site.sys->seasonOf(site.body, t), s1 = north * site.sys->seasonOf(site.body, t + dt);
    return s1 > s0 && s0 > -0.75 && s0 < 0.35;
}
double SurfaceView::springJetStrength(uint64_t id, double t) { return jetStrength(id, t, false, 50 + 90 * hash01(mix64(id + 3)), 0.25 + 0.2 * hash01(mix64(id + 4))); }
void SurfaceView::drawSpringJets(Framebuffer& fb, double t) {
    if (!springJetsNow(t)) return;
    std::vector<GeyserVent> vents;
    springJets(site.gen, site.unitAt(camPos.x, camPos.z), vents);
    for (const GeyserVent& gv : vents) {
        double vx, vz; site.localAt(gv.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 1500) continue;
        JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 10; j.lava = false; j.shadeAdd = -24;   // the fans' dark dust (the accent's bank)
        j.heightM = 15 + 45 * hash01(mix64(gv.id + 7)); j.widthM = 2 + 3 * hash01(mix64(gv.id + 8));
        drawJet(fb, t, j, springJetStrength(gv.id, t));
    }
}

// P-06: the tiger stripes' curtains (Enceladus's: Porco et al. 2006, Spitale et al. 2015). Along each stripe within 300 km a sheet of fine ice
// rising 25-60 km, densest low and thinning upward, its top wavering along the stripe and with time, bright in the sun (a glow over the dark sky:
// the brighter of it and what is behind, so it never darkens the ground), drawn in segments of 4 km; and within 5 km of the explorer its jets one
// by one (`curtainVents`: R-307's columns, 0.8-2.5 km high, their fallout drifting down for minutes in the moon's weak gravity)
void SurfaceView::drawCurtains(Framebuffer& fb, double t) {
    const Vec3 here = site.unitAt(camPos.x, camPos.z);
    if (here.z > -0.3) return;
    const TigerStripes T = tigerStripesOf(site.gen);
    const double Rk = site.gen.R, sunUp = smoothstep(-0.05, 0.15, env.sun.dirLocal.y), base = 18 + 18 * sunUp;
    RasterParams rp;
    rp.bank = 8; rp.grain = &grainFine; rp.grainScale = 0.9; rp.grainWeight = 1.0;
    rp.cutout = true; rp.edge = &grainEdge; rp.edgeAmp = 0.75; rp.blend = BLEND_MAX; rp.zwrite = false;
    for (int k = 0; k < 4; k++) {
        const double half = 0.5 * T.len[k];
        double xp = 0, zp = 0, gp = 0, Hp = 0; bool have = false;
        for (double a = -half; a <= half + 1e-6; a += 4.0) {
            double x, z; site.localAt(stripePoint(T, Rk, k, a), x, z);
            const double d = std::sqrt((x - camPos.x) * (x - camPos.x) + (z - camPos.z) * (z - camPos.z));
            if (d > 300000) { have = false; continue; }
            double water = 0;
            const double gy = d < 40000 ? heightNear(x, z, d, water) : (double)site.sampleAt(x, z, 2048).h;
            const double H = 1000.0 * (25.0 + 35.0 * (0.5 + 0.5 * std::sin(a / 23.0 + 1.7 * k))) * (0.85 + 0.15 * std::sin(t * 0.05 + a / 7.0 + k));
            if (have && d > 6000 && std::sqrt((xp - camPos.x) * (xp - camPos.x) + (zp - camPos.z) * (zp - camPos.z)) > 6000) {   // the near part is its jets
                rp.edgeScale = 1.0 / (0.012 * d); rp.grainScale = rp.edgeScale;   // the veil's grain three pixels or so across at any distance (u and v in metres)
                const int NS = 4;
                for (int j = 0; j < NS; j++) {
                    const double f0 = (double)j / NS, f1 = (double)(j + 1) / NS;
                    const Vec3 a0 = toView(xp, gp + Hp * f0, zp), b0 = toView(x, gy + H * f0, z), b1 = toView(x, gy + H * f1, z), a1 = toView(xp, gp + Hp * f1, zp);
                    if (a0.z < NEAR_Z || b0.z < NEAR_Z || b1.z < NEAR_Z || a1.z < NEAR_Z) continue;
                    const double w0 = 0.8 - 0.6 * f0, w1 = 0.8 - 0.6 * f1, s0 = base - 12 * f0, s1 = base - 12 * f1;   // a veil: thin, thinner upward
                    RVert q[4];
                    q[0].x = a0.x; q[0].y = a0.y; q[0].z = a0.z; q[0].w = w0; q[0].shade = s0; q[0].u = a * 1000.0; q[0].v = f0 * Hp - t * 300.0;
                    q[1].x = b0.x; q[1].y = b0.y; q[1].z = b0.z; q[1].w = w0; q[1].shade = s0; q[1].u = (a + 4.0) * 1000.0; q[1].v = f0 * H - t * 300.0;
                    q[2].x = b1.x; q[2].y = b1.y; q[2].z = b1.z; q[2].w = w1; q[2].shade = s1; q[2].u = (a + 4.0) * 1000.0; q[2].v = f1 * H - t * 300.0;
                    q[3].x = a1.x; q[3].y = a1.y; q[3].z = a1.z; q[3].w = w1; q[3].shade = s1; q[3].u = a * 1000.0; q[3].v = f1 * Hp - t * 300.0;
                    rasterPolygon(fb, q, 4, rp, proj);
                }
            }
            xp = x; zp = z; gp = gy; Hp = H; have = true;
        }
    }
    std::vector<GeyserVent> vents;
    curtainVents(site.gen, here, vents);
    for (const GeyserVent& gv : vents) {
        double vx, vz; site.localAt(gv.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 5000) continue;
        JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
        j.heightM = 800 + 1700 * gv.radiusKm; j.widthM = 15 + 25 * gv.radiusKm;
        drawJet(fb, t, j, jetStrength(gv.id, t, true, 1, 1));
    }
}

// D-04 (TR_LIGHTS_LEFT_ON, D-05's late peoples): the lamps a dead people left burning along its streets, one every 30 m on each side in turn
// within a kilometre and a half (its towns' streets: T-02's marks), each a point of the lamps' light (bank 15) on a 4 m post, more of them dark
// where the years left less of the street; the lit windows are the forms' (`drawFormItems`)
void SurfaceView::drawStreetLamps(Framebuffer& fb) {
    for (const GroundMark& m : site.marks) {
        if (m.kind != 0) continue;
        const double dx = m.bx - m.ax, dz = m.bz - m.az, L = std::sqrt(dx * dx + dz * dz);
        if (L < 1) continue;
        const double mx = 0.5 * (m.ax + m.bx) - camPos.x, mz = 0.5 * (m.az + m.bz) - camPos.z;
        if (std::sqrt(mx * mx + mz * mz) > 1500 + 0.5 * L) continue;
        const double ux = dx / L, uz = dz / L;
        const int n = (int)(L / 30.0);
        for (int k = 0; k <= n; k++) {
            const uint64_t h = mix64((uint64_t)m.id * 977 + (uint64_t)k);
            if (hash01(h) > 0.25 + 0.7 * m.left) continue;   // a broken lamp
            const double side = (k & 1) ? 1.0 : -1.0, along = std::min(L, 15.0 + 30.0 * k);
            const double x = m.ax + ux * along - uz * side * (m.half + 1.0), z = m.az + uz * along + ux * side * (m.half + 1.0);
            const double d = std::sqrt((x - camPos.x) * (x - camPos.x) + (z - camPos.z) * (z - camPos.z));
            if (d > 1500) continue;
            const Vec3 v = toView(x, site.groundHeight(x, z) + 4.0, z);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 58 + 4 * hash01(mix64(h + 1));
            rasterPoint3(fb, p, 15, proj, true, d < 60 ? 3 : (d < 180 ? 2 : 1));
        }
    }
}

void SurfaceView::drawWeather(Framebuffer& fb, double t) {
    const uint32_t events = KINDS[site.gen.type].events;   // P-00: the kind's vents and jets
    if (site.gen.hasTrait(TR_LIGHTS_LEFT_ON)) drawStreetLamps(fb);   // D-04
    if (events & EV_COMET_JETS) drawCometJets(fb, t);   // O4
    if (events & EV_PLUMES) drawPlumes(fb, t);   // P-05
    if (events & EV_SPRING_JETS) drawSpringJets(fb, t);   // P-05
    if (events & EV_CURTAINS) drawCurtains(fb, t);   // P-06
    if ((events & (EV_CRACK_VENTS | EV_FUMAROLES | EV_STEAM_BURSTS | EV_SPRINGS)) || site.gen.hasTrait(TR_GEYSERS) || site.gen.hasTrait(TR_HOT_SPRINGS)) drawGeysers(fb, t);   // R-307; P-01; P-03; Q-02
    if (events & EV_SPRINGS) drawSpores(fb, t);   // P-03
    // R-307: dust devils: a column of dust points spiralling up, leaning with the wind, a skirt at the foot
    for (const DustDevil& d : devils) {
        double dist = std::sqrt((d.x - camPos.x) * (d.x - camPos.x) + (d.z - camPos.z) * (d.z - camPos.z));
        if (dist > 1400) continue;
        double life = smoothstep(0, 15, d.age) * smoothstep(d.life, d.life - 25, d.age);
        double gy = site.groundHeight(d.x, d.z);
        double lean = 0.15 * clampd(env.windKnots / 20.0, 0, 1);
        int n = (int)((dist < 300 ? 420 : 160) * life);
        Rng r((uint64_t)(d.phase * 1000));
        for (int i = 0; i < n; i++) {
            double u = r.uni(), a0 = r.range(0, TAU), spin = r.range(2.0, 4.0);
            double yy = u * u * d.height;   // dense at the foot, thinning up
            double rr = d.radius * (0.3 + 1.0 * u) * (0.75 + 0.25 * std::sin(t * 3 + u * 20)) * r.range(0.5, 1.0);
            double a = a0 + t * spin + u * 6.0;
            Vec3 v = toView(d.x + std::cos(a) * rr + std::sin(env.windDir) * lean * yy, gy + yy, d.z + std::sin(a) * rr + std::cos(env.windDir) * lean * yy);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 30 + 14 * env.skyBrightness - 8 * u;
            rasterPoint3(fb, p, 9, proj, true, dist < 250 && u < 0.6 ? 2 : 1);
        }
    }
    // M4-08 eruptions: a column of glowing points that rises and spreads for eight seconds; R-307: the tectonic fountain
    // (kind 1) throws its lava two hundred metres and more for half a minute, the meteorite strike (kind 2) flashes and then
    // fountains dark ejecta for eight seconds
    for (const Eruption& e : eruptions) {
        double age = t - e.start;
        if (e.kind == 1) {
            if (age < 0 || age > 30) continue;
            Rng er((uint64_t)(e.start * 10));
            double ramp = smoothstep(0, 2.5, age) * (1.0 + 0.35 * std::exp(-age / 3.0)) * smoothstep(30, 22, age);
            JetSpec j; j.x = e.x; j.z = e.z; j.h = (uint64_t)(e.start * 10); j.bank = 2; j.lava = true;
            j.heightM = 250 + 200 * er.uni(); j.widthM = 14 + 12 * er.uni();
            j.dist = std::sqrt((e.x - camPos.x) * (e.x - camPos.x) + (e.z - camPos.z) * (e.z - camPos.z));
            drawJet(fb, t, j, ramp);
            continue;
        }
        if (e.kind == 2) {
            if (age < 0 || age > 8) continue;
            double gy = site.groundHeight(e.x, e.z);
            if (age < 0.35) {   // the flash: a white burst that swells and fades
                Vec3 v = toView(e.x, gy + 3, e.z);
                if (v.z > NEAR_Z) { RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 63; rasterPoint3(fb, p, 1, proj, false, (int)(2 + 6 * (1 - age / 0.35))); }
            }
            Rng er((uint64_t)(e.start * 10));
            double gAcc = std::max(0.5, site.gravity);
            int n = (int)(140 * std::min(1.0, age / 0.6));
            for (int i = 0; i < n; i++) {
                double a = er.range(0, TAU), sp = er.range(0.3, 1.0), tilt = er.range(0.3, 1.2), v0 = er.range(25, 70);
                double fa = age - 0.1;
                if (fa < 0) continue;
                double up = v0 * std::cos(tilt) * fa - 0.5 * gAcc * fa * fa;
                if (up < 0) continue;
                double out = v0 * std::sin(tilt) * fa * sp;
                Vec3 v = toView(e.x + std::cos(a) * out, gy + up, e.z + std::sin(a) * out);
                if (v.z < NEAR_Z) continue;
                RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = i % 5 == 0 ? 56 : 18 + 6 * sp;   // dark rock with a few glowing chips
                rasterPoint3(fb, p, 0, proj, true, 1);
            }
            continue;
        }
        if (age < 0 || age > 8) continue;
        Rng er((uint64_t)(e.start * 10));
        int n = (int)(60 * std::min(1.0, age / 2.0));
        for (int i = 0; i < n; i++) {
            double ph = er.uni(), a = er.range(0, TAU), sp = er.range(0.5, 2.0);
            double life = std::fmod(age * 0.8 + ph * 4.0, 4.0);
            double up = 30.0 * life - 3.0 * life * life * (site.gravity / 9.8), out = sp * life * 3.0;
            Vec3 v = toView(e.x + std::cos(a) * out, site.groundHeight(e.x, e.z) + up, e.z + std::sin(a) * out);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 62 - 14 * life;
            rasterPoint3(fb, p, 2, proj, true, life < 2 ? 2 : 1);
        }
    }
    // M4-06 snow, hail and dust
    if (env.snow > 0.02 || env.hail > 0.02 || env.dust > 0.05) {
        Rng r((uint64_t)(t * 20.0));
        int n = (int)(env.snow * 220 + env.hail * 120 + env.dust * 260);
        double wx = std::sin(env.windDir) * env.windKnots * 0.03, wz = std::cos(env.windDir) * env.windKnots * 0.03;
        for (int i = 0; i < n; i++) {
            double x = camPos.x + r.range(-6, 6), z = camPos.z + r.range(-6, 6), y = camPos.y + r.range(-3, 4);
            // flakes fall slowly and swing; dust streams sideways
            bool dustP = env.dust > 0.05 && r.uni() < env.dust * 260.0 / std::max(1, n);
            double sway = dustP ? 0 : std::sin(t * 2 + x) * 0.3;
            Vec3 va = toView(x + sway, y - (dustP ? 0 : std::fmod(t * 0.9 + i * 0.37, 7.0)), z);
            Vec3 vb = toView(x + sway + wx * (dustP ? 3.0 : 0.3), y - (dustP ? 0.05 : std::fmod(t * 0.9 + i * 0.37, 7.0) + 0.12), z + wz * (dustP ? 3.0 : 0.3));
            if (va.z < NEAR_Z && vb.z < NEAR_Z) continue;
            RVert a, b;
            a.x = va.x; a.y = va.y; a.z = va.z; a.shade = dustP ? 30 + 10 * env.skyBrightness : 44 + 12 * env.skyBrightness;
            b.x = vb.x; b.y = vb.y; b.z = vb.z; b.shade = a.shade;
            if (dustP) rasterLine3(fb, a, b, 0, proj, true, 1);
            else rasterPoint3(fb, a, site.hasWater ? 8 : 1, proj, true, env.hail > 0.02 ? 1 : (r.chance(0.3) ? 2 : 1));
        }
    }
    if (env.rain <= 0.02) return;
    Rng r((uint64_t)(t * 60.0));
    int n = (int)(env.rain * 300);
    // drops live in the world around the camera (KI-017): depth-tested against the terrain, so
    // they never paint over the ground when looking down, and the wind drifts them
    double wx = std::sin(env.windDir) * env.windKnots * 0.03, wz = std::cos(env.windDir) * env.windKnots * 0.03;
    for (int i = 0; i < n; i++) {
        double x = camPos.x + r.range(-6, 6), z = camPos.z + r.range(-6, 6), y = camPos.y + r.range(-3, 4);
        Vec3 va = toView(x, y, z), vb = toView(x + wx * 0.2, y - 0.5, z + wz * 0.2);
        if (va.z < NEAR_Z && vb.z < NEAR_Z) continue;
        RVert a, b;
        a.x = va.x; a.y = va.y; a.z = va.z; a.shade = 38 + 10 * env.skyBrightness;
        b.x = vb.x; b.y = vb.y; b.z = vb.z; b.shade = a.shade;
        rasterLine3(fb, a, b, 1, proj, true, 1);
    }
    // lightning
    if (env.rain > 0.5) {
        if (lightningFlash > 0) lightningFlash *= 0.6;
        if (r.chance(0.004 * env.rain)) lightningFlash = 1.0;
        if (lightningFlash < 0.02) lightningFlash = 0;
    } else lightningFlash = 0;
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
            // R-204: the eye is the camera pod on the nose, bolted to the hull: it pans within the pod's range, takes the
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
    } else if (inDrone) {   // R-403
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
    if (freeCam) { camX = freePos.x; camZ = freePos.z; eyeY = freePos.y; yaw = freeYaw; pitch = freePitch; roll = 0; }   // M6-05
    if (quake > 0 && cameraOverrideAlt < 0) {   // R-307: the ground's shaking, a slow heave with a fine tremor on it
        double q = quake * (0.4 + 0.6 * std::sin(t * 1.3));
        pitch += q * (1.1 * DEG * std::sin(t * 9.7) + 0.3 * DEG * std::sin(t * 23.1));
        roll += q * 0.8 * DEG * std::sin(t * 7.3 + 1.0);
        eyeY += q * 0.1 * std::sin(t * 11.0);
    }
    camPos = Vec3(camX, eyeY, camZ);
    camLocal = cameraBasis(yaw, pitch);
    {   // B-322: the picture follows the eye, not the body: under the drawn water surface (a dive's first frames, the eye
        // within a few centimetres of the surface, the free camera) everything is seen through water
        double wlEye = site.hasWater || site.gen.liquidLevel > -1e8 ? site.waterNowAt(camX, camZ) : -1e9;
        viewUnderwater = wlEye > -1e8 && eyeY < wlEye + 0.03;
        if (viewUnderwater) env.fogDistance = std::min(env.fogDistance, 22.0);
    }
    if (env.fogSea > 0) {   // Q-02: an eye inside the fog sea sees a few hundred metres
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
    waterWdx = std::sin(env.windDir), waterWdz = std::cos(env.windDir);   // F-04: the water corners' (waterCorner)
    waterSunUp = smoothstep(-0.03, 0.06, env.sun.dirLocal.y);
    setupLamps();   // D-04
    {
        SurfaceLook L = lookFor(site.gen, site.sys->star);
        cloudShadows = site.atmosphere && L.cloudsLayer && (env.cloudCover > 0.03 || localFront.on) && !hasOpaqueDeck(site.gen.type);   // W-03: a front's deck in reach counts
        cloudThr = 1.0 - env.cloudCover * 0.75;
        cloudWx = windDriftX; cloudWz = windDriftZ;
    }
    auto nowMs = []() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); };
    double tSec = nowMs();
    auto section = [&](int k) { double n = nowMs(); lastRenderMs[k] = n - tSec; tSec = n; };
    setupPalette(fb);
    if (fade < 1) scalePalette(fb.pal, fade);
    fb.clearDepth();
    floorH = floorHeight();   // B-408: the sky's galactic band reaches down to its horizon
    drawSky(fb, t, stars, sr);
    section(0);
    int r0 = ringR[0], r1 = ringR[1], r2 = ringR[2], r3 = ringR[3];
    int rN = nearBlend > 0 ? ringN : 0;   // O6-02: the near ring (B-313: at every speed, on foot and in the buggy; off when the capsule is high)
    if (cameraOverrideAlt > 800) r0 = 0;          // high above the site the 16 m cells are invisible
    else if (cameraOverrideAlt > 300) r0 = std::min(r0, 16);
    {   // B-310: the rings near to far, each thread drawing every ring within its band of rows (the vertex caches and the
        // tiles are filled first, serially, so the threads only read; the pixels come out as the serial loop's)
        struct Ring { TerrainCache* c; std::vector<VtxCache>* vc; int r; double cover; };
        // B-313: a coarser ring skips the cells the finer one covers, up to a cell of overlap (it used to overlap by two or
        // three cells: 4 km of both rings drawn at the far edge); the finer ring's edge is morphed onto the coarser surface.
        // B-318: the cover is a radius in metres, tested against every corner of a coarse cell (`cellCovered`): a finer ring
        // of rf cells draws the cells whose centre lies within rf, so any point within rf - 0.71 cells lies in one of them
        auto coverOf = [](int rf, double csf) { return rf > 0 ? (rf - 0.71) * csf : 0.0; };
        Ring rings[5] = {{&site.lodN, &vcN, rN, 0}, {&site.lod0, &vc0, r0, coverOf(rN, 4)}, {&site.lod1, &vc1, r1, coverOf(r0, 16)},
                         {&site.lod2, &vc2, r2, coverOf(r1, 64)}, {&site.lod3, &vc3, r3, coverOf(r2, 512)}};   // O1: the far ring to 53 km
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
            if (k == 1) { site.lod0R = g.r; site.lod0Band = coarse ? band : 0; site.camX = camPos.x; site.camZ = camPos.z; }   // B-319: groundHeight follows lod0's morph
            warmTerrainLOD(*g.c, *g.vc, g.r, g.cover, coarse ? coarse->c : nullptr, coarse ? coarse->vc : nullptr, band);
        }
        morphCoarse = nullptr; morphBand = 0;
        section(1);
        // F-04: the cells binned once, on the pool by rows of cells (every ring's rows in one dispatch), then each band thread draws
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
        // F-05: the reflections gathered (serially: they fill the caches on a miss) for the bands to draw after the rings; the floor is
        // drawn first in every band, as it was drawn before the rings
        static const char* skipEnv2 = std::getenv("VESPERIS_SKIP");
        const bool reflections = site.hasWater && !(skipEnv2 && std::strstr(skipEnv2, "refl"));
        if (reflections) gatherReflections();
        static const bool traceBands = std::getenv("VESPERIS_TRACE") != nullptr;   // F-01: each band's time split (its ms and rows always)
        std::atomic<int> bandSlot{0};
        // F-06: the frame's rows dealt in chunks (a quarter of a thread's share): the chunk that holds the horizon's thousands of small cells
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
    drawShards(fb, t);   // C-03: after the walls, so the depth test hides what lies behind them
    if (buggy.deployed) drawBuggy(fb, t);
    if (drone.deployed) drawDrone(fb, t);   // R-403
    drawLife(fb, t);
    if (cameraOverrideAlt < 0) drawCapsule(fb, t);   // B-322: while the capsule flies (the descent, the ascent) it is not on the ground: its beacon beam, seen from above, cut a bright line across the land
    drawWeather(fb, t);
    drawVeil(fb);
    section(5);
    // O1: what the crosshair rests on, for the HUD's range readout and M (skipped while the capsule flies)
    lastRange = cameraOverrideAlt >= 0 ? -1 : rangeToGround(drawRadiusM(), lastRangeX, lastRangeZ, lastRangeWater);
    { static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr; if (trace && frameStamp <= 3) {
        fprintf(stderr, "  render %d: palette+sky %.1f, warm %.1f, terrain %.1f, footprints..objects %.1f, flora %.1f, rest %.1f ms\n", frameStamp, lastRenderMs[0], lastRenderMs[1], lastRenderMs[2], lastRenderMs[3], lastRenderMs[4], lastRenderMs[5]);
        for (int f = 0; f < 2; f++) { const std::string bl = bandTraceLine(f == 1); if (!bl.empty()) fprintf(stderr, "    %s\n", bl.c_str()); }   // F-01
    } }
}

// F-01: the bands of the last frame as one line: rows and ms; the terrain's split and counts when they were traced
std::string SurfaceView::bandTraceLine(bool flora) const {
    const BandTrace* bs = flora ? lastFloraBands : lastTerrainBands;
    const int n = flora ? lastFloraBandN : lastTerrainBandN;
    if (n == 0) return std::string();
    char buf[160];
    std::string s = "flora bands:";
    if (!flora) { snprintf(buf, sizeof buf, "terrain bands (%d cells binned in %.2f ms, %d in every band):", lastTerrainCells, lastTerrainBinMs, lastTerrainAllRows); s = buf; }   // F-04
    for (int i = 0; i < n; i++) {
        const BandTrace& b = bs[i];
        if (flora) snprintf(buf, sizeof buf, " %d-%d %.2f ms (%d visited, %d drawn)%s", b.y0, b.y1, b.ms, b.walked, b.drawn, i + 1 < n ? "," : "");
        else if (b.walked > 0) snprintf(buf, sizeof buf, " %d-%d %.2f ms (walk %.2f, water set-up %.2f, fill %.2f; %d walked, %d drawn, %d wet)%s", b.y0, b.y1, b.ms, b.ms - b.waterMs - b.fillMs, b.waterMs, b.fillMs, b.walked, b.drawn, b.wet, i + 1 < n ? "," : "");
        else snprintf(buf, sizeof buf, " %d-%d %.2f%s", b.y0, b.y1, b.ms, i + 1 < n ? "," : "");
        s += buf;
    }
    return s;
}

// the cached height under a point at `dist` from the camera: bilinear lod0 close by, the coarser rings further out
// (so the march never thrashes the fine cache), with the water level of the same cell
double SurfaceView::heightNear(double x, double z, double dist, double& water) {
    if (dist < ringR[0] * 16.0 * 0.9) { water = site.waterNowAt(x, z); return site.groundHeight(x, z); }
    TerrainCache& c = dist < ringR[1] * 64.0 * 0.9 ? site.lod1 : (dist < ringR[2] * 512.0 * 0.9 || !ringR[3] ? site.lod2 : site.lod3);
    const TerrainVertex& tv = c.at((int)std::lround(x / c.cellSize), (int)std::lround(z / c.cellSize));
    water = site.waterOf(tv);
    return tv.h;
}

double SurfaceView::drawnGround(double x, double z, double dist, double& water, int& ring) {
    ring = drawnRing(dist);
    if (ring == 0) { water = site.waterNowAt(x, z); return site.groundHeight(x, z); }
    TerrainCache& c = ring == 1 ? site.lod1 : (ring == 2 ? site.lod2 : site.lod3);
    water = site.waterOf(c.at((int)std::lround(x / c.cellSize), (int)std::lround(z / c.cellSize)));
    return cacheHeightAt(c, x, z);
}

double SurfaceView::rangeToGround(double maxDist, double& hitX, double& hitZ, bool& water) {
    Vec3 f = camLocal.row(2);   // the view direction in local metres
    double fh = std::sqrt(f.x * f.x + f.z * f.z);
    double dirx = fh > 1e-9 ? f.x / fh : 0, dirz = fh > 1e-9 ? f.z / fh : 0;
    // the ground's height relative to the camera at a horizontal advance sH along the sight line (with the curvature)
    auto groundRel = [&](double sH, double& px, double& pz, bool& w) -> double {
        double arc = sH, ang = 0;
        if (site.smallBody) { ang = std::asin(std::min(1.0, sH / site.R)); arc = site.R * ang; }
        px = camPos.x + dirx * arc; pz = camPos.z + dirz * arc;
        double wl; double h = heightNear(px, pz, arc, wl);
        w = wl > -1e8 && wl > h;
        if (w) h = wl;
        if (site.smallBody) return (site.R + h) * std::cos(ang) - (site.R + camPos.y);
        return h - camPos.y - sH * sH * invTwoR;
    };
    double prev = 0, s = 0.6, px, pz; bool w;
    while (s < maxDist) {
        double sH = s * fh;
        if (site.smallBody && sH >= site.R * 0.999) break;
        if (s * f.y <= groundRel(sH, px, pz, w)) {
            double lo = prev, hi = s;
            for (int i = 0; i < 9; i++) { double mid = 0.5 * (lo + hi); if (mid * f.y <= groundRel(mid * fh, px, pz, w)) hi = mid; else lo = mid; }
            groundRel(hi * fh, hitX, hitZ, water);
            return hi;
        }
        prev = s; s += std::max(0.6, s * 0.03);
    }
    return -1;
}
