#include "surface_view.h"
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

namespace {
struct SurfaceLook {
    RGB ground, secondary, tertiary, sand, snow, grass, flora2, zenith, horizon, nightZenith, nightHorizon;
    double skyDensity;     // brightness of the daytime sky
    double fogDistance;    // metres
    bool cloudsLayer;
};

// N1-03: the ground colours come from the body's material palette (BodyGen::matColor, shared with the globe
// and the landing map); only the sky, the fog, the night and the cloud layer are still chosen per type here.
SurfaceLook lookFor(const BodyGen& g, const Star& star) {
    SurfaceLook L;
    RGB lc = lerp(star.color, RGB(1, 1, 1), 0.5f);   // the star's light, softened (the globe and the map use the same tint, N1-05)
    RGB white(1, 1, 1);
    L.cloudsLayer = false;
    L.skyDensity = 1.0;
    L.fogDistance = 6000;
    L.nightZenith = RGB(0.01f, 0.01f, 0.03f);
    L.nightHorizon = RGB(0.04f, 0.04f, 0.07f);
    L.zenith = RGB(0, 0, 0); L.horizon = RGB(0, 0, 0);
    // O1 (B-302): the terrain is drawn to 53 km now, so the haze must let a clear day show it: felisian 12 km (was 5),
    // ocean 9, quartz 5, thin air 14, the airless worlds 45 (their "fog" is only the softening of distance)
    typeSky(g, L.zenith, L.horizon);   // R-307: one pair per type, shared with the globe and the landing map
    switch (g.type) {
        case PT_MOLTEN: L.horizon = RGB(0.12f, 0.06f, 0.04f); L.fogDistance = 30000; break;
        case PT_CRATERED: L.fogDistance = 45000; break;
        case PT_VENUSIAN:
            L.fogDistance = 700; L.skyDensity = 1.3;
            L.nightZenith = RGB(0.08f, 0.06f, 0.04f); L.nightHorizon = RGB(0.12f, 0.09f, 0.06f); break;
        case PT_FELISIAN:
            L.fogDistance = 12000; L.cloudsLayer = true;
            L.nightZenith = RGB(0.02f, 0.02f, 0.06f); L.nightHorizon = RGB(0.06f, 0.06f, 0.1f); break;
        case PT_ROCKY: L.fogDistance = 45000; break;
        case PT_THINATMO:
            L.fogDistance = 14000; L.skyDensity = 0.5; L.cloudsLayer = true;
            L.nightZenith = RGB(0.01f, 0.01f, 0.02f); L.nightHorizon = RGB(0.03f, 0.02f, 0.03f); break;
        case PT_ICY: L.fogDistance = 45000; break;
        case PT_COMET: L.fogDistance = 16000; break;   // O4: cut down by the activity (dust in the coma) in computeEnvironment
        case PT_QUARTZ:
            L.fogDistance = 5000; L.skyDensity = 1.15; L.cloudsLayer = true;
            L.nightZenith = RGB(0.05f, 0.04f, 0.06f); L.nightHorizon = RGB(0.1f, 0.08f, 0.1f); break;
        case PT_OCEAN:
            L.zenith = g.skyTint * 0.8f; L.horizon = lerp(g.skyTint, white, 0.5f); L.fogDistance = 9000; L.cloudsLayer = true;
            L.nightZenith = RGB(0.02f, 0.02f, 0.06f); L.nightHorizon = RGB(0.06f, 0.06f, 0.1f); break;
        case PT_METAL: L.fogDistance = 45000; break;
        case PT_VOLCANIC: L.horizon = RGB(0.08f, 0.05f, 0.02f); L.fogDistance = 30000; break;
        case PT_CARBON: L.fogDistance = 45000; break;
        // R-307
        case PT_EUROPAN: L.fogDistance = 45000; break;
        case PT_BOMBARDED: L.fogDistance = 45000; break;
        case PT_TECTONIC:
            L.fogDistance = 15000; L.skyDensity = 0.6; L.cloudsLayer = true;
            L.nightZenith = RGB(0.04f, 0.02f, 0.01f); L.nightHorizon = RGB(0.09f, 0.04f, 0.02f); break;   // the glow of the fissures
        case PT_DESERT:
            L.fogDistance = 9000; L.skyDensity = 1.1; L.cloudsLayer = true;
            L.nightZenith = RGB(0.02f, 0.02f, 0.04f); L.nightHorizon = RGB(0.06f, 0.05f, 0.05f); break;
        case PT_HYDROCARBON:
            L.fogDistance = 2500; L.skyDensity = 1.6;   // an opaque deck: no cloud layer, no sun disc
            L.nightZenith = RGB(0.03f, 0.02f, 0.01f); L.nightHorizon = RGB(0.05f, 0.03f, 0.02f); break;
        case PT_ACIDIC:
            L.fogDistance = 2500; L.skyDensity = 1.3; L.cloudsLayer = true;
            L.nightZenith = RGB(0.03f, 0.04f, 0.02f); L.nightHorizon = RGB(0.06f, 0.06f, 0.03f); break;
        default: break;
    }
    if (g.hasTrait(TR_HAZE)) { L.fogDistance *= 0.3; L.skyDensity *= 1.25; }   // R-304
    const RGB* m = g.matColor;
    L.ground = m[familyRep(g.type, FAM_ROCK)];
    L.sand = m[familyRep(g.type, FAM_SAND)];
    L.snow = m[familyRep(g.type, FAM_SNOW)];
    L.grass = m[MAT_GRASS];
    L.flora2 = g.vegColor2;
    // bank 2 (secondary): water where there is a sea, lava on hot worlds, else the bright crystal/ejecta tint
    if (g.type == PT_FELISIAN || g.type == PT_OCEAN || g.liquidLevel > -1e8) L.secondary = m[MAT_WATER];
    else if (isLavaWorld(g.type)) L.secondary = m[MAT_LAVA];
    else if (g.hasTrait(TR_GLASSED)) L.secondary = m[MAT_GLASS];   // S-03: the sheets of glass own bank 2 (the family's rep is the water the palette coloured as glass)
    else if (g.type == PT_QUARTZ || g.type == PT_CARBON) L.secondary = lerp(m[MAT_QUARTZ], white, 0.4f);
    else L.secondary = lerp(L.ground, white, 0.5f);
    // bank 3 (tertiary): the forest on living worlds, elsewhere boulders a shade lighter than the ground
    L.tertiary = g.type == PT_FELISIAN ? m[MAT_FOREST] : lerp(L.ground, white, 0.25f);
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
    for (int y = 0; y < FBH; y++)
        for (int x = 0; x < FBW; x++) {
            Vec3 d((x + 0.5 - proj.cx) * invf, -(y + 0.5 - proj.cy) * invf, 1.0);
            dirLUT[y * FBW + x] = normalize(d);
        }
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
    ruinCells.clear(); culture = cultureOf(site.gen);   // C-01
    shardsFound.clear(); nearShard = NearShard();   // C-03 (the game refills the set from the guide after this)
    trail.clear(); hasWaypoint = false; eruptions.clear(); nextEruption = 30; devils.clear(); nextDevil = 20; quake = 0; nextQuake = 45; siteEpoch++;   // B-303: nothing of the last landing shows on this one's map
    tick("capsule+misc");
    findLandmarks();   // O6-06
    tick("landmarks");
    spawnLife(lat, lon);   // N3: herds and flocks from the world's bestiary
    tick("life");
    buildBandMap();
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
double SurfaceView::bandAt(const Vec3& d) const { return sampleGalaxyBand(bandMap, BAND_MAP_W, BAND_MAP_H, d); }

void SurfaceView::relocateCapsule(double x, double z) {
    capsuleX = x; capsuleZ = z;
    capsuleY = site.surfaceHeight(x, z);
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
    if (!env.hasSun2 || env.sun2Share < 0.05 || env.sun2Share > 0.95) { drawBlobShadowFrom(fb, x, z, radius, height, env.sun.dirLocal, 1.0); return; }
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

// M1-04: the land mirrored about the sea level, blended into water pixels only (bank 2), far to near.
void SurfaceView::drawReflections(Framebuffer& fb) {
    struct Cell { double d2; int cx, cz; int lod; double level; };
    std::vector<Cell> cells;
    double sea = site.seaLevel;
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
                double wl = std::max(std::max(cache.at(cx, cz).water, cache.at(cx + 1, cz).water), std::max(cache.at(cx, cz + 1).water, cache.at(cx + 1, cz + 1).water));
                if (wl > sea + 0.5 && hmax > wl + 0.3) cells.push_back({d2 * cs * cs, cx, cz, lod, wl});
                if (hmax < sea + 0.3) continue;
                cells.push_back({d2 * cs * cs, cx, cz, lod, sea});
            }
    }
    std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) { return a.d2 > b.d2; });
    RasterParams rp;
    rp.blend = BLEND_REFLECT; rp.maskBank = 2; rp.reflectMix = 0.45; rp.ztest = false; rp.zwrite = false;
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
        for (int i = 0; i < 4; i++) if (tvc[i]->water > -1e8f) { wMean += tvc[i]->water; wN++; }
        if (wN) wMean /= wN;
        for (int k = 0; k < 2; k++) {
            double px[4], pz[4], ph[4]; int n = 0;
            if (wN) {
                double xs3[3], zs3[3], hs3[3], wl3[3], sh3[3];
                bool anyWet = false, anyDry = false;
                for (int e = 0; e < 3; e++) { int a = tris[k][e]; xs3[e] = xs[a]; zs3[e] = zs[a]; hs3[e] = hs[a]; wl3[e] = tvc[a]->water > -1e8f ? tvc[a]->water : wMean; sh3[e] = vertexShore(cache, c.cx + ox[a], c.cz + oz[a]); if (sh3[e] < 0) anyWet = true; else anyDry = true; }
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
            if (m >= 3) rasterPolygon(fb, q, m, rp, proj);
        }
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

// M4-07: ruins on a jittered 2 km latitude/longitude grid of habitable and quartz worlds, so a ruin
// stays where it is whatever the landing site or the anchor of the local frame; the giant cube is rare.
// C-01: the grid, the draw and the settlements live in `galaxy/ruins.*`; the view keeps a cache per cell in local metres.
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
    if (c.has && c.spec.kind == RK_SETTLEMENT) { DrainageOff off; c.has = ruinSiteOk(site.gen, c.spec); }   // the slope and the sea, without building tiles (the water's own check is at the draw)
    if (c.has) {
        Ruin& r = c.r;
        site.localAt(StarSystem::bodyFromLatLon(c.spec.lat, c.spec.lon), r.x, r.z);
        r.heading = c.spec.heading; r.kind = c.spec.kind; r.style = c.spec.style; r.size = c.spec.size; r.spec = &c.spec;
        if (c.spec.kind == RK_SETTLEMENT) { for (int l = 0; l < 3; l++) ruinElements(c.spec, culture, l, c.elems[l]); shardSitesOf(c.spec, culture, c.shards); }   // C-03: and its shards
    }
    return &c;
}

bool SurfaceView::ruinAt(int gLat, int gLon, Ruin& r) const {
    const RuinCell* c = ruinCell(gLat, gLon);
    if (!c->has) return false;
    r = c->r;
    return true;
}

// C-01: a settlement's pieces (`ruinElements`) as boxes and domes on the ground at their own foot, the far side of every box
// culled, lit like the rocks, fogged; whole within 300 m of its edge, the walls as one box each within a kilometre, one box
// per building to four kilometres. Near pieces stand on the drawn ground (`groundHeight`, so the walls meet what the feet
// walk on and nothing floats as the near ring blends); the far ones on the planet function at 64 m, read once per cell
void SurfaceView::drawSettlement(Framebuffer& fb, const Ruin& r, const RuinCell& cell, double dist) {
    const RuinSpec& sp = *r.spec;
    double R = sp.size;
    if (dist - R > 4000) return;
    int lod = dist < 300 + R ? 0 : (dist < 1000 + R ? 1 : 2);
    if (lod == 2 && !cell.baseDone) {
        DrainageOff off;   // no tile is built for a far town
        cell.base.resize(sp.buildings.size());
        for (size_t i = 0; i < sp.buildings.size(); i++) cell.base[i] = site.sampleAt(r.x + sp.buildings[i].x, r.z + sp.buildings[i].z, 64).h;
        cell.baseDone = true;
    }
    const std::vector<RuinElem>& el = cell.elems[lod];
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y), lf = env.sun.lightFactor;
    double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    RasterParams rp; rp.bank = culture.family == 1 ? 9 : 0;
    if (culture.style == 1) { rp.grain2 = mesoFor(MAT_ROCK); rp.grain2Scale = 3.0; rp.grain = &grain; rp.grainScale = 3.0 * FB_SCALE; }
    double fwx = camLocal.m[2][0], fwz = camLocal.m[2][2];   // the camera's forward on the ground
    auto face = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, double fog) {
        Vec3 n = normalize(cross(b - a, d - a));
        if (dot(n, a - camPos) > 0) return;   // the far side
        double amb = ambient * (0.7 + 0.3 * std::max(0.0, n.y));   // a wall takes more of the sky's light than a rock's flank
        double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf;
        double shade = 46 * std::pow(light, 0.6);
        shade += (63 - shade) * fog;
        RVert q[4]; Vec3 pts[4] = {a, b, c, d};
        for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; q[k].u = pts[k].x + pts[k].y * 0.5; q[k].v = pts[k].z + pts[k].y * 0.5; }
        rasterPolygon(fb, q, 4, rp, proj);
    };
    auto box = [&](Vec3 c, double hx, double hy, double hz, double heading, double fog) {   // c the base's centre, hy the height
        Vec3 f(std::sin(heading), 0, std::cos(heading)), s(std::cos(heading), 0, -std::sin(heading));
        Vec3 p[8];
        for (int i = 0; i < 8; i++) p[i] = c + f * ((i & 1) ? hz : -hz) + s * ((i & 2) ? hx : -hx) + Vec3(0, (i & 4) ? hy : 0, 0);
        face(p[4], p[5], p[7], p[6], fog); face(p[0], p[1], p[5], p[4], fog); face(p[2], p[6], p[7], p[3], fog); face(p[1], p[3], p[7], p[5], fog); face(p[0], p[4], p[6], p[2], fog);
    };
    for (const RuinElem& e : el) {
        double ex = r.x + e.x, ez = r.z + e.z;
        double ddx = ex - camPos.x, ddz = ez - camPos.z, d = std::sqrt(ddx * ddx + ddz * ddz);
        double ext = 2 * std::max(std::max(e.hx, e.hz), (e.y1 - e.y0) * 0.5);
        if (d > 5 && ext / d * proj.f < 1.2) continue;
        if (d > ext + 2 && ddx * fwx + ddz * fwz < -ext) continue;   // behind the camera
        double gy;
        if (lod < 2) { gy = site.groundHeight(ex, ez); double w = site.waterAt(ex, ez); if (w > -1e8 && gy < w) continue; }
        else gy = cell.base[e.building < 0 ? 0 : e.building];
        double fog = 1 - std::exp(-d / env.fogDistance);
        bool onGround = e.y0 < 0.01;
        double y0 = gy + e.y0 - (onGround ? 1.0 : 0), hy = (e.y1 - e.y0) + (onGround ? 1.0 : 0);
        if (e.shape == 0) {
            box(Vec3(ex, y0, ez), e.hx, hy, e.hz, e.heading, fog);
            if (e.glyphs && d < 70 && lod == 0) {   // a line of glyphs cut into the front face: the three low bits of each letter of the star's name as strokes
                const std::string& nm = site.sys->star.name;
                Vec3 f(std::sin(e.heading), 0, std::cos(e.heading)), s(std::cos(e.heading), 0, -std::sin(e.heading));
                double wall = e.y1 - e.y0, width = 2 * e.hx * 0.8, rowY = gy + e.y0 + std::min(wall * 0.55, 1.6), gh = std::min(0.12 * wall, 0.35);
                Vec3 origin = Vec3(ex, rowY, ez) + f * (e.hz + 0.02) - s * (width * 0.5);
                double step = width / (double)std::max<size_t>(1, std::min<size_t>(nm.size(), 14));
                for (size_t i = 0; i < nm.size() && i < 14; i++) {
                    int code = (unsigned char)nm[i];
                    for (int seg = 0; seg < 3; seg++) {
                        if (!((code >> seg) & 1)) continue;
                        Vec3 p0 = origin + s * (i * step + seg * step * 0.3) + Vec3(0, seg * gh * 0.4, 0), p1 = p0 + Vec3(0, gh, 0) + s * (step * 0.15);
                        RVert a, b; Vec3 va = toView(p0.x, p0.y, p0.z), vb = toView(p1.x, p1.y, p1.z);
                        a.x = va.x; a.y = va.y; a.z = va.z; a.shade = 18; b.x = vb.x; b.y = vb.y; b.z = vb.z; b.shade = 18;
                        rasterLine3(fb, a, b, rp.bank, proj, true, 1);
                    }
                }
            }
        } else {   // a dome: bands of quads, the top ones gone where it fell in
            Vec3 base(ex, gy + e.y0, ez);
            double rr = e.hx;
            int keep = e.broken > 0 ? (int)std::floor(4 * (1 - e.broken)) : 4;
            for (int eb = 0; eb < keep; eb++)
                for (int a = 0; a < 10; a++) {
                    double e0 = eb * PI / 8, e1 = (eb + 1) * PI / 8, a0 = a * TAU / 10, a1 = (a + 1) * TAU / 10;
                    Vec3 pts[4] = {Vec3(std::cos(a0) * std::cos(e0), std::sin(e0), std::sin(a0) * std::cos(e0)), Vec3(std::cos(a1) * std::cos(e0), std::sin(e0), std::sin(a1) * std::cos(e0)),
                                   Vec3(std::cos(a1) * std::cos(e1), std::sin(e1), std::sin(a1) * std::cos(e1)), Vec3(std::cos(a0) * std::cos(e1), std::sin(e1), std::sin(a0) * std::cos(e1))};
                    face(base + pts[0] * rr, base + pts[3] * rr, base + pts[2] * rr, base + pts[1] * rr, fog);
                }
        }
        lastRuinElems++;
    }
}

void SurfaceView::drawRuins(Framebuffer& fb, double t) {
    lastRuinMs = 0; lastRuinElems = 0;
    if (!worldHasRuins(site.gen)) return;
    auto tr0 = std::chrono::steady_clock::now();
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    forNearbyRuins(camPos.x, camPos.z, worldHadCivilisation(site.gen) ? 2 : 1, [&](const Ruin& r, const RuinCell& cell, int ring) {
        double dist = std::sqrt((r.x - camPos.x) * (r.x - camPos.x) + (r.z - camPos.z) * (r.z - camPos.z));
        if (r.kind == RK_SETTLEMENT) { drawSettlement(fb, r, cell, dist); return; }
        if (ring > 1) return;   // the monoliths as M4-07 drew them: the 3 x 3 cells, within 1.8 km
        {
            if (dist > 1800 || r.size / dist * proj.f < 1.5) return;
            TerrainVertex tvr = site.sampleAt(r.x, r.z, 16);   // analytic: far ruins must not thrash the terrain cache
            double gy = tvr.h;
            if (tvr.water > -1e8f && gy < tvr.water) return;
            RasterParams rp; rp.bank = 0;
            if (r.style == 1) { rp.grain2 = mesoFor(MAT_ROCK); rp.grain2Scale = 3.0; rp.grain = &grain; rp.grainScale = 3.0 * FB_SCALE; }
            double fog = 1 - std::exp(-dist / env.fogDistance);
            auto face = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d) {
                Vec3 n = normalize(cross(b - a, d - a));
                double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
                double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp;
                double shade = 46 * std::pow(light, 0.6);
                shade += (63 - shade) * fog;
                RVert q[4]; Vec3 pts[4] = {a, b, c, d};
                for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; q[k].u = pts[k].x + pts[k].y * 0.5; q[k].v = pts[k].z + pts[k].y * 0.5; }
                rasterPolygon(fb, q, 4, rp, proj);
                if (r.style == 2) {   // glowing edges
                    for (int k = 0; k < 4; k++) {
                        RVert l0 = q[k], l1 = q[(k + 1) % 4];
                        l0.shade = l1.shade = 50 + 10 * std::sin(t * 1.5 + k);
                        rasterLine3(fb, l0, l1, 2, proj, true, 1);
                    }
                }
            };
            auto box = [&](Vec3 c, double hx, double hy, double hz, double heading) {
                Vec3 f(std::sin(heading), 0, std::cos(heading)), s(std::cos(heading), 0, -std::sin(heading));
                Vec3 p[8];
                for (int i = 0; i < 8; i++) p[i] = c + f * ((i & 1) ? hz : -hz) + s * ((i & 2) ? hx : -hx) + Vec3(0, (i & 4) ? hy * 2 : 0, 0);
                face(p[4], p[5], p[7], p[6]); face(p[0], p[1], p[5], p[4]); face(p[2], p[6], p[7], p[3]); face(p[1], p[3], p[7], p[5]); face(p[0], p[4], p[6], p[2]);
            };
            Vec3 base(r.x, gy, r.z);
            switch (r.kind) {
                case 0: for (int k = 0; k < 8; k++) { double a = r.heading + k * TAU / 8; box(base + Vec3(std::cos(a) * r.size * 0.5, 0, std::sin(a) * r.size * 0.5), 0.4, r.size * 0.35, 0.4, a); } break;
                case 1: case 4: box(base, r.size * 0.5, r.size * 0.5, r.size * 0.5, r.heading); break;
                case 2: {
                    RasterParams rd = rp;
                    for (int e = 0; e < 4; e++) for (int a = 0; a < 10; a++) {
                        double e0 = e * PI / 8, e1 = (e + 1) * PI / 8, a0 = a * TAU / 10, a1 = (a + 1) * TAU / 10, rr = r.size * 0.5;
                        Vec3 pts[4] = {Vec3(std::cos(a0) * std::cos(e0), std::sin(e0), std::sin(a0) * std::cos(e0)), Vec3(std::cos(a1) * std::cos(e0), std::sin(e0), std::sin(a1) * std::cos(e0)),
                                       Vec3(std::cos(a1) * std::cos(e1), std::sin(e1), std::sin(a1) * std::cos(e1)), Vec3(std::cos(a0) * std::cos(e1), std::sin(e1), std::sin(a0) * std::cos(e1))};
                        face(base + pts[0] * rr, base + pts[1] * rr, base + pts[2] * rr, base + pts[3] * rr);
                    }
                    (void)rd;
                    break;
                }
                case 3: for (int k = 0; k < 4; k++) { if (k == 1) continue; double a = r.heading + k * TAU / 4; box(base + Vec3(std::cos(a) * r.size * 0.5, 0, std::sin(a) * r.size * 0.5), r.size * 0.5, 0.75, 0.25, a + PI / 2); } break;
            }
            // glyphs from the star's name on the front of cubes and walls
            if ((r.kind == 1 || r.kind == 4) && dist < 200) {
                const std::string& nm = site.sys->star.name;
                Vec3 f(std::sin(r.heading), 0, std::cos(r.heading)), s(std::cos(r.heading), 0, -std::sin(r.heading));
                Vec3 origin = base + f * (-r.size * 0.5 - 0.02) + Vec3(0, r.size * 0.55, 0) - s * (r.size * 0.4);
                double step = r.size * 0.8 / std::max<size_t>(1, nm.size());
                for (size_t i = 0; i < nm.size() && i < 14; i++) {
                    int code = (unsigned char)nm[i];
                    for (int seg = 0; seg < 3; seg++) {
                        if (!((code >> seg) & 1)) continue;
                        Vec3 p0 = origin + s * (i * step + seg * step * 0.3) + Vec3(0, seg * r.size * 0.05, 0), p1 = p0 + Vec3(0, r.size * 0.12, 0) + s * (step * 0.15);
                        RVert a, b; Vec3 va = toView(p0.x, p0.y, p0.z), vb = toView(p1.x, p1.y, p1.z);
                        a.x = va.x; a.y = va.y; a.z = va.z; a.shade = 55; b.x = vb.x; b.y = vb.y; b.z = vb.z; b.shade = 55;
                        rasterLine3(fb, a, b, 2, proj, true, 1);
                    }
                }
            }
        }
    });
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
    joinAhead();   // M7-01: cell coordinates change with the anchor
    double lat, lon;
    site.latLonAt(player.x, player.z, lat, lon);
    Vec3 capU = site.unitAt(capsuleX, capsuleZ);
    Vec3 bugU = site.unitAt(buggy.x, buggy.z);
    Vec3 drnU = site.unitAt(drone.x, drone.z);   // R-403
    Vec3 wpU = site.unitAt(wpX, wpZ);   // B-303: the waypoint and the trail move with the frame too
    std::vector<Vec3> trailU; trailU.reserve(trail.size());
    for (const auto& tp : trail) trailU.push_back(site.unitAt(tp.first, tp.second));
    Vec3 oldNorth = site.north0;
    const StarSystem* sys = site.sys;
    int body = site.body;
    site.init(sys, body, lat, lon, lastT);
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
    for (size_t i = 0; i < trail.size(); i++) { double tx, tz; site.localAt(trailU[i], tx, tz); trail[i] = {(float)tx, (float)tz}; }
    siteEpoch++;
    ruinCells.clear();   // C-01: the cells' local metres moved with the frame
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
    drainagePrefetch(site.gen, site.up0, drawRadiusM() + 3000.0, false);   // O6-03
    findLandmarks();   // O6-06: the same sights, in the new frame
    flocks.clear(); critters.clear(); herds.clear(); habitatT = -1e9;   // they lived near the old origin, 50 km away (B-316: the habitat cells respawn theirs)
    prefetchStage = 0; prefetchRing = 0;
}

// O6-06: the sights within the drawn disc plus a cell (`landmarksNear` searches the 14 km grid cells, a few milliseconds
// each and cached), on a thread of its own: the HUD and the maps show them once `collectLandmarks` has them
void SurfaceView::findLandmarks() {
    joinLandmarks();
    landmarks.clear();
    if (!site.sys) return;
    BodyGen g = site.gen; std::string name = site.sys->bodies[site.body].name; Vec3 up = site.up0; double radius = drawRadiusM() + 8000.0;
    std::vector<Landmark>* out = &landmarkOut; std::atomic<bool>* busy = &landmarkBusy;
    landmarkOut.clear();
    landmarkBusy.store(true);
    landmarkThread = std::thread([g, name, up, radius, out, busy]() { landmarksNear(g, name, up, radius, *out); busy->store(false, std::memory_order_release); });
}
void SurfaceView::collectLandmarks() {
    if (!landmarkThread.joinable() || landmarkBusy.load(std::memory_order_acquire)) return;
    landmarkThread.join();
    landmarks.clear();
    for (const Landmark& L : landmarkOut) { SiteLandmark sl; sl.lm = L; site.localAt(L.unit, sl.x, sl.z); landmarks.push_back(sl); }
    landmarkOut.clear();
}
void SurfaceView::joinLandmarks() {
    if (landmarkThread.joinable()) landmarkThread.join();
    landmarkBusy.store(false); landmarkOut.clear();
}
const SurfaceView::SiteLandmark* SurfaceView::landmarkAt(double x, double z) const {
    const SiteLandmark* best = nullptr; double bd = 1e18;
    for (const SiteLandmark& s : landmarks) { double d2 = (s.x - x) * (s.x - x) + (s.z - z) * (s.z - z); if (d2 < s.lm.radiusM * s.lm.radiusM && d2 < bd) { bd = d2; best = &s; } }
    return best;
}
const SurfaceView::SiteLandmark* SurfaceView::nearestLandmark(double x, double z, double maxDist) const {
    const SiteLandmark* best = nullptr; double bd = maxDist * maxDist;
    for (const SiteLandmark& s : landmarks) { double d2 = (s.x - x) * (s.x - x) + (s.z - z) * (s.z - z); if (d2 < bd) { bd = d2; best = &s; } }
    return best;
}

void SurfaceView::computeEnvironment(double t) {
    const Body& b = site.sys->bodies[site.body];
    env.sun = site.sun(t);
    env.sunDisc = env.sun;
    env.lightColor = env.sun.color;   // the star's colour (S-01: whiter in a red dwarf's flare)
    env.hasSun2 = site.sun2(t, env.sun2);
    env.sun2Share = 0;
    double sinAlt = env.sun.dirLocal.y;
    env.skyBrightness = site.atmosphere ? smoothstep(-0.12, 0.2, sinAlt) * (1 - 0.9 * env.sun.eclipse * env.sun.eclipse) : 0.0;   // M1-08
    if (env.hasSun2) {
        // M5-01: the light for shading blends both suns by their strength above the horizon; the sky and the
        // day brightness take the brighter of the two, so a companion still up after the primary set keeps the day alive
        double up1 = smoothstep(-0.12, 0.05, env.sunDisc.dirLocal.y), up2 = smoothstep(-0.12, 0.05, env.sun2.dirLocal.y);
        double w1 = env.sunDisc.lightFactor * up1, w2 = env.sun2.lightFactor * up2;
        if (w1 + w2 > 1e-6) {
            env.sun2Share = w2 / (w1 + w2);
            Vec3 dir = normalize(env.sunDisc.dirLocal * w1 + env.sun2.dirLocal * w2);
            if (env.sun2Share > 0.02) {
                env.sun.dirLocal = dir;
                env.sun.altitude = std::asin(clampd(dir.y, -1, 1));
                env.sun.azimuth = wrap2pi(std::atan2(dir.x, dir.z));
                env.sun.lightFactor = env.sunDisc.lightFactor * (1 - env.sun2Share) + (env.sunDisc.lightFactor + env.sun2.lightFactor) * env.sun2Share;
                env.sun.eclipse = env.sunDisc.eclipse * (1 - env.sun2Share);
                env.lightColor = lerp(env.sunDisc.color, env.sun2.color, (float)env.sun2Share);
            }
        }
        double day2 = site.atmosphere ? smoothstep(-0.12, 0.2, env.sun2.dirLocal.y) * clampd(env.sun2.lightFactor / std::max(env.sunDisc.lightFactor, 0.05), 0, 1) : 0.0;
        env.skyBrightness = std::max(env.skyBrightness, day2);
        sinAlt = env.sun.dirLocal.y;
    }
    env.ringShadow = env.sunDisc.ringShadow;   // O0-01
    env.skyBrightness *= 1 - 0.45 * env.ringShadow;
    env.localTime = env.sun.dayFraction;
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
    env.cloudCover = cloud;
    env.rain = (b.type == PT_FELISIAN && cloud > 0.72) ? clampd((cloud - 0.72) / 0.25, 0, 1) : 0;
    if (b.type == PT_ACIDIC) env.rain = cloud > 0.6 ? clampd((cloud - 0.6) / 0.3, 0, 1) : 0;   // R-307: the acid rains often
    if (b.type == PT_HYDROCARBON) { double dz = gnoise2(t / 1800.0, 2.3, site.gen.seed + 27); env.rain = clampd((dz - 0.45) / 0.3, 0, 0.5); }   // a methane drizzle now and then
    // M4-06: the same clouds snow when it is cold; thin air raises dust storms; fog banks drift by
    env.snow = 0; env.dust = 0; env.fogBank = 0; env.hail = 0;
    if ((b.type == PT_FELISIAN || b.type == PT_OCEAN) && cloud > 0.62) {
        double precip = clampd((cloud - 0.62) / 0.3, 0, 1);
        if (env.temperatureC < -1) { env.snow = precip; env.rain = 0; }
        else if (env.temperatureC < 8 && precip > 0.8) env.hail = (precip - 0.8) / 0.2;
    }
    if (b.type == PT_THINATMO || b.type == PT_DESERT) {
        double storm = gnoise2(t / 2400.0, 5.1, site.gen.seed + 21);
        env.dust = clampd((storm - (b.type == PT_DESERT ? 0.45 : 0.35)) / 0.3, 0, 1);   // the storm's wind is applied with the wind update below (B-206); R-307: rarer on a desert world
    }
    if (b.type == PT_FELISIAN || b.type == PT_OCEAN) {
        double fogN = gnoise2(t / 1500.0 + player.x / 4000.0, player.z / 4000.0, site.gen.seed + 33);
        env.fogBank = clampd((fogN - 0.45) / 0.25, 0, 1) * (1 - env.rain) * clampd(1 - std::fabs(env.temperatureC - 8) / 25.0, 0, 1);
    }
    env.aurora = 0; env.auroraStorm = 0;
    env.flare = starFlare(site.sys->star, t);   // S-01: a red dwarf's flare opens the exposure (setupPalette) and is a storm of its own (auroralStorm)
    if (site.atmosphere && !hasOpaqueDeck(b.type) && std::fabs(env.latDeg) > 40) {
        // R-402: the night's potential (the star's class, the world's magnetic field, the star's storm), the latitude ramp
        // pushed equatorward by the storm, the darkness, and the night's own slow variation
        env.auroraStorm = auroralStorm(site.sys->star, t);
        double latLo = 52 - 10 * env.auroraStorm;
        env.aurora = auroraPotential(*site.sys, b, t) * smoothstep(latLo, latLo + 16, std::fabs(env.latDeg)) * (1 - env.skyBrightness) * (0.6 + 0.4 * gnoise2(t / 600.0, 8.8, site.gen.seed + 44));
    }
    if (lastWindUpdate < 0 || t - lastWindUpdate > 2.0) {
        // B-206: the wind is set from scratch here (base by type, a slow gust term, rain, the dust storm's x3.5);
        // it used to be multiplied by the storm factor every frame between these updates and ran away to 1e13 knots
        lastWindUpdate = t;
        double base = site.atmosphere ? (b.type == PT_THINATMO ? 25 : (b.type == PT_VENUSIAN ? 4 : (b.type == PT_DESERT ? 18 : (b.type == PT_HYDROCARBON ? 3 : 10)))) : 0;
        env.windKnots = base * (0.4 + 0.8 * (0.5 + 0.5 * gnoise2(t / 400.0, 3.3, site.gen.seed + 5))) * (1 + env.rain * 1.5) * (1 + 2.5 * env.dust);
        env.windDir = wrap2pi(1.7 + 2.0 * gnoise2(t / 2000.0, 7.7, site.gen.seed + 6));
    }
    // the clouds' drift (sky layers and ground shadows) integrates the wind: 0.5 m per knot-second. It used to be
    // `wind x 0.5 x t`, which jumped by tens of kilometres whenever the wind changed (the sun flickering, B-206)
    if (lastEnvT < 0) { windDriftX = std::sin(env.windDir) * env.windKnots * 0.5 * t; windDriftZ = std::cos(env.windDir) * env.windKnots * 0.5 * t; }
    else if (t > lastEnvT) { windDriftX += std::sin(env.windDir) * env.windKnots * 0.5 * (t - lastEnvT); windDriftZ += std::cos(env.windDir) * env.windKnots * 0.5 * (t - lastEnvT); }
    lastEnvT = t;
    // temperature
    double base = b.tempK - 273.15;
    if (b.type == PT_FELISIAN) {
        // B-321: the biome model's mean temperature at this spot (the same number that chose the grass underfoot), with
        // a day-night swing round it; the HUD used to read -10 C on a grassland plateau from a model of its own
        double winter = clampd((env.latDeg >= 0 ? -site.season : site.season), 0, 1);
        double sub = site.gen.locked ? dot(site.unitAt(player.x, player.z), site.gen.lockedDir) : 0.0;
        base = climateTempC(site.gen, std::fabs(env.latDeg), player.y, winter, sub) - 3 + 10 * sinAlt;
    } else if (site.atmosphere) {
        double offset = 6, swing = 12;   // R-307: each air has its greenhouse and its day-night swing
        switch (b.type) {
            case PT_VENUSIAN: offset = 420; break;
            case PT_THINATMO: offset = -25; break;
            case PT_DESERT: offset = 22; swing = 32; break;       // hot days, cold nights
            case PT_HYDROCARBON: offset = -75; swing = 3; break;  // the haze evens everything out
            case PT_ACIDIC: offset = 55; swing = 6; break;
            case PT_TECTONIC: offset = 65; swing = 14; break;     // the ground itself is warm
            default: break;
        }
        base = base * 0.75 + offset;
        base += swing * sinAlt - 0.4 * std::fabs(env.latDeg);
    } else {
        base = base * 1.3 - 60 + (sinAlt > 0 ? 110 * sinAlt : -70);
        base -= 0.5 * std::fabs(env.latDeg);
    }
    if (b.type != PT_FELISIAN) base -= 6.5 * (player.y / 1000.0);
    if (b.type == PT_MOLTEN) base += 180;
    if (b.type == PT_VOLCANIC) base += 60;
    env.temperatureC = std::max(base, -270.0);
    switch (b.type) {
        case PT_FELISIAN: env.pressureAtm = 0.7 + 0.6 * hash01(site.gen.seed); break;
        case PT_VENUSIAN: env.pressureAtm = 60 + 35 * hash01(site.gen.seed); break;
        case PT_THINATMO: env.pressureAtm = 0.006 + 0.04 * hash01(site.gen.seed); break;
        case PT_QUARTZ: env.pressureAtm = 0.4 + 0.4 * hash01(site.gen.seed); break;
        case PT_OCEAN: env.pressureAtm = 1.2 + 1.5 * hash01(site.gen.seed); break;
        case PT_DESERT: env.pressureAtm = 0.3 + 0.7 * hash01(site.gen.seed); break;        // R-307
        case PT_HYDROCARBON: env.pressureAtm = 1.4 + 0.3 * hash01(site.gen.seed); break;
        case PT_ACIDIC: env.pressureAtm = 2.0 + 3.0 * hash01(site.gen.seed); break;
        case PT_TECTONIC: env.pressureAtm = 0.08 + 0.3 * hash01(site.gen.seed); break;
        default: env.pressureAtm = 0; break;
    }
    env.pressureAtm *= std::exp(-player.y / 8000.0);
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    env.fogDistance = L.fogDistance * (1 - 0.7 * env.rain) * (1 - 0.55 * env.snow) * (1 - 0.94 * env.dust) * (1 - 0.9 * env.fogBank);
    // O4: a comet vents harder the closer it dives to its star (the same activity as the tail seen from space);
    // the dust of the coma hangs over the ground
    env.cometActivity = 0;
    if (b.type == PT_COMET) {
        const StarSystem& S = *site.sys;
        double dStar = length(S.bodyPos(site.body, t) - S.star.pos);
        double refKm = S.bodies.empty() ? AU_GAME_KM : std::max(AU_GAME_KM * 0.5, S.bodies[0].type == PT_COMET ? AU_GAME_KM : S.bodies[0].orbitRadiusKm);
        env.cometActivity = clampd((refKm * 1.6 / dStar) * (refKm * 1.6 / dStar), 0, 1);
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
            double angR = std::asin(clampd(ob.radiusKm / dist, 0, 1));
            double phase = 0.5 * (1 + dot(normalize(site.sys->star.pos - bp), normalize(sitePos - bp)));
            double illum = phase * PLANET_TYPES[ob.type].albedo * (angR / (5 * DEG)) * (angR / (5 * DEG)) * env.sun.lightFactor;
            bool glowing = ob.type == PT_SUBSTELLAR;
            if (glowing) illum = std::max(illum, 0.3 * clampd(ob.luminosity / 0.003, 0.55, 1.0) * (angR / (4 * DEG)) * (angR / (4 * DEG)));   // M5-02: warm light of its own
            illum = std::min(0.5, illum);
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
}

// C-03: the nearest shard the explorer does not have within 2.2 m, on this floor (within 2 m of height), for E and the HUD
void SurfaceView::findNearShard() {
    nearShard = NearShard();
    if (inVehicle() || !worldHadCivilisation(site.gen)) return;
    double best = 2.2;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        if (r.kind != RK_SETTLEMENT) return;
        for (const ShardSite& s : cell.shards) {
            if (shardsFound.count(s.index)) continue;
            double x = r.x + s.x, z = r.z + s.z, d = std::sqrt((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z));
            if (d >= best) continue;
            if (std::fabs(site.groundHeight(x, z) - player.y) > 2.0) continue;
            best = d; nearShard.index = s.index; nearShard.place = s.place; nearShard.sclass = cell.spec.sclass; nearShard.x = x; nearShard.z = z; nearShard.dist = d;
        }
    });
}

// C-03: the shards of the settlements round the camera: a slab of dark glass (0.36 x 0.24 m, 6 cm thick) on the floor of a
// room or at a stela's foot, lit like the walls but never pale, and over it a point of light that pulses (the stars' bank),
// drawn within 250 m behind the depth test, so a glint through a doorway or over a fallen wall is what gives one away; the
// ones the explorer has are gone
void SurfaceView::drawShards(Framebuffer& fb, double t) {
    lastShardsDrawn = 0;
    if (!worldHadCivilisation(site.gen)) return;
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y), lf = env.sun.lightFactor;
    double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    RasterParams rp; rp.bank = 7;
    forNearbyRuins(camPos.x, camPos.z, 2, [&](const Ruin& r, const RuinCell& cell, int) {
        if (r.kind != RK_SETTLEMENT) return;
        for (const ShardSite& s : cell.shards) {
            if (shardsFound.count(s.index)) continue;
            double x = r.x + s.x, z = r.z + s.z;
            double dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 250) continue;
            double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            double fog = 1 - std::exp(-d / env.fogDistance);
            if (d < 60) {   // the slab: its top and its four sides, the far ones culled
                Vec3 f(std::sin(s.heading), 0, std::cos(s.heading)), sx(std::cos(s.heading), 0, -std::sin(s.heading)), c(x, gy, z);
                const double hx = 0.18, hz = 0.12, hy = 0.06;
                Vec3 p[8];
                for (int i = 0; i < 8; i++) p[i] = c + f * ((i & 1) ? hz : -hz) + sx * ((i & 2) ? hx : -hx) + Vec3(0, (i & 4) ? hy : 0, 0);
                auto face = [&](Vec3 a, Vec3 b, Vec3 cc, Vec3 dd) {
                    Vec3 n = normalize(cross(b - a, dd - a));
                    if (dot(n, a - camPos) > 0) return;
                    double light = ambient + (1 - ambient) * std::max(0.0, dot(n, sd)) * sunUp * lf;
                    double shade = 4 + 10 * light;   // bank 7 reaches a mid grey by 20: the slab stays under 0.3 of white, darker than the walls
                    shade += (63 - shade) * fog;
                    RVert q[4]; Vec3 pts[4] = {a, b, cc, dd};
                    for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; }
                    rasterPolygon(fb, q, 4, rp, proj);
                };
                face(p[4], p[5], p[7], p[6]); face(p[0], p[1], p[5], p[4]); face(p[2], p[6], p[7], p[3]); face(p[1], p[3], p[7], p[5]); face(p[0], p[4], p[6], p[2]);
            }
            double pulse = 0.5 + 0.5 * std::sin(t * 2.6 + s.index * 1.7);
            Vec3 v = toView(x, gy + 0.12, z);
            if (v.z > NEAR_Z) { RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = 48 + 15 * pulse; rasterPoint3(fb, q, 4, proj, true, d < 40 ? 2 : 1); }
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
    if (in.moveY != 0 || in.moveX != 0) { fwd = in.moveY; str = in.moveX; }
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
    if (inBuggy) updateBuggy(dt, in, t);
    else if (inDrone) updateDrone(dt, in, t);   // R-403
    else updateWalking(dt, in, t, controlsEnabled);
    prefetchAhead();   // M7-01
    collectLandmarks();   // O6-06
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
    if (t - drainCheckT > 3.0) {   // O6-03: the drainage tiles the far ring will reach next, computed ahead on their own thread
        drainCheckT = t;
        Vec3 u = site.unitAt(player.x, player.z);
        if (!drainageReady(site.gen, u, drawRadiusM() + 9000.0)) drainagePrefetch(site.gen, u, drawRadiusM() + 9000.0, true);
    }
    // M4-08: eruptions every minute or so on molten and volcanic worlds, within 400 m. R-307: a tectonic world's fountains
    // rise from its fissures (the nearest lava cell of the 16 m ring within 600 m) every 15-40 s and last 25 s; a bombarded
    // world takes a meteorite every 20-60 s within 1.2 km, and shakes for a moment
    if (isLavaWorld(site.gen.type) || site.gen.type == PT_BOMBARDED) {
        nextEruption -= dt;
        if (nextEruption <= 0) {
            Rng er((uint64_t)(t * 100));
            if (site.gen.type == PT_TECTONIC) {
                double bx = 0, bz = 0; bool found = false;
                for (int tries = 0; tries < 40 && !found; tries++) {
                    double x = player.x + er.sym(600), z = player.z + er.sym(600);
                    if (site.lod0.at((int)std::floor(x / 16), (int)std::floor(z / 16)).material == MAT_LAVA) { bx = x; bz = z; found = true; }
                }
                if (found) { eruptions.push_back({bx, bz, t, 1}); eruptionCue = 1.0; }
                nextEruption = 15 + 25 * er.uni();
            } else if (site.gen.type == PT_BOMBARDED) {
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
    // R-307: the ground shakes on a tectonic world: a quake every 60-180 s, ten seconds long, the camera trembling by it
    if (site.gen.type == PT_TECTONIC) {
        nextQuake -= dt;
        if (nextQuake <= 0) { Rng qr((uint64_t)(t * 100) ^ 0x9A4E); quake = 0.6 + 0.4 * qr.uni(); nextQuake = 60 + 120 * qr.uni(); eruptionCue = 1.0; }
    }
    quake *= std::exp(-dt / 4.0);
    if (quake < 0.01) quake = 0;
    // R-307: dust devils on a desert world (and in a thin-air dust storm): up to four within 500 m, born every 20-50 s, each
    // walking with the wind for two to five minutes
    if ((site.gen.type == PT_DESERT || (site.gen.type == PT_THINATMO && env.dust > 0.3)) && env.skyBrightness > 0.3) {
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
    // M4-02: the trail
    {
        double dxs = player.x - (trail.empty() ? player.x : trail.back().first), dzs = player.z - (trail.empty() ? player.z : trail.back().second);
        if (trail.empty() || dxs * dxs + dzs * dzs > 25.0 * 25.0) { trail.push_back({(float)player.x, (float)player.z}); if (trail.size() > 600) trail.erase(trail.begin()); }
    }
    updateLife(dt, t);
    computeEnvironment(t);
}

// Rocks larger than 0.6 m and tree trunks in the 3x3 cells around (x, z), as cylinders (M8-05).
// The placement formulas mirror drawObjects.
void SurfaceView::collectColliders(double x, double z, std::vector<Collider>& out) {
    out.clear();
    int type = site.sys->bodies[site.body].type;
    double rockDensity = 0.0, rockScale = 1.0;
    switch (type) {
        case PT_CRATERED: rockDensity = 1.1; rockScale = 1.1; break;
        case PT_ROCKY: rockDensity = 1.6; rockScale = 1.6; break;
        case PT_THINATMO: rockDensity = 1.2; rockScale = 0.9; break;
        case PT_MOLTEN: rockDensity = 0.7; rockScale = 1.1; break;
        case PT_ICY: rockDensity = 0.3; rockScale = 1.3; break;
        case PT_FELISIAN: rockDensity = 0.35; rockScale = 0.9; break;
        case PT_VENUSIAN: rockDensity = 0.6; rockScale = 1.4; break;
        case PT_QUARTZ: rockDensity = 0.5; rockScale = 0.8; break;
        case PT_METAL: rockDensity = 1.3; rockScale = 1.2; break;
        case PT_VOLCANIC: rockDensity = 0.8; rockScale = 1.0; break;
        case PT_CARBON: rockDensity = 0.9; rockScale = 1.1; break;
        case PT_COMET: rockDensity = 1.4; rockScale = 1.2; break;   // O4
        case PT_OCEAN: rockDensity = 0.0; break;
        case PT_EUROPAN: rockDensity = 0.25; rockScale = 1.0; break;   // R-307: ice blocks by the chaos
        case PT_TECTONIC: rockDensity = 1.0; rockScale = 1.2; break;
        case PT_DESERT: rockDensity = 0.9; rockScale = 1.0; break;
        case PT_HYDROCARBON: rockDensity = 0.3; rockScale = 0.9; break;
        case PT_BOMBARDED: rockDensity = 1.8; rockScale = 1.3; break;   // ejecta boulders
        case PT_ACIDIC: rockDensity = 0.6; rockScale = 1.0; break;
    }
    double cs = 16;
    double treeScale = std::sqrt(9.8 / std::max(site.gravity, 1.0));
    int pcx = (int)std::floor(x / cs), pcz = (int)std::floor(z / cs);
    // ruins (M4-07) as one cylinder each; C-01: a settlement's standing pieces as rows of discs along their walls (within 40 m)
    forNearbyRuins(x, z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        if (r.kind != RK_SETTLEMENT) {
            double gy = site.groundHeight(r.x, r.z), top = r.kind == 0 ? r.size * 0.7 : (r.kind == 2 ? r.size * 0.5 : (r.kind == 3 ? 1.5 : r.size));
            out.push_back({r.x, r.z, r.kind == 3 ? r.size * 0.15 : r.size * 0.55, 3, gy, gy + top});
            return;
        }
        double dcx = r.x - x, dcz = r.z - z;
        if (dcx * dcx + dcz * dcz > (r.size + 45) * (r.size + 45)) return;
        for (const RuinElem& e : cell.elems[0]) {
            if (e.shape != 0 || e.part == 4 || e.y0 > 1.2 || e.y1 < 0.9) continue;
            double ex = r.x + e.x, ez = r.z + e.z, reach = 40 + e.hx + e.hz;
            if ((ex - x) * (ex - x) + (ez - z) * (ez - z) > reach * reach) continue;
            double hx = e.hx, hz = e.hz, heading = e.heading, gy = site.groundHeight(ex, ez);   // the piece's feet where it is drawn
            if (hz > hx) { std::swap(hx, hz); heading += PI / 2; }
            double rr = clampd(hz, 0.2, 0.6), ax = std::cos(heading), az = -std::sin(heading), bx = std::sin(heading), bz = std::cos(heading);
            int n = std::max(1, (int)std::ceil(hx / rr)), m = std::max(1, (int)std::ceil(hz / rr));
            for (int i = 0; i < n; i++) {
                double ta = n > 1 ? -hx + rr + (2 * hx - 2 * rr) * i / (n - 1) : 0;
                for (int j = 0; j < m; j++) {
                    double tb = m > 1 ? -hz + rr + (2 * hz - 2 * rr) * j / (m - 1) : 0;
                    out.push_back({ex + ax * ta + bx * tb, ez + az * ta + bz * tb, rr, 3, gy + e.y0, gy + e.y1});
                }
            }
        }
    });
    for (int cz = pcz - 1; cz <= pcz + 1; cz++)
        for (int cx = pcx - 1; cx <= pcx + 1; cx++) {
            uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xB0B);
            const TerrainVertex& tv = site.lod0.at(cx, cz);
            bool lava = tv.material == MAT_LAVA;
            double rd = rockDensity;
            if (type == PT_FELISIAN && (tv.material == MAT_GRASS || tv.material == MAT_ROCK) && tv.biome != BIO_WETLAND) rd = 0.7;   // R-306: open ground carries more stone
            rd *= 1 + 5.0 * tv.scree / 255.0;   // O6-04: boulder fields on the scree under the cliffs
            int nrocks = (int)(rd * 2.0 * hash01(h));
            if (type == PT_FELISIAN && hash01(mix64(h + 5)) < 0.06 && tv.material != MAT_FOREST) nrocks += 3;   // R-306: an outcrop of three
            if (tv.material == MAT_SAND || tv.material == MAT_WATER || lava) nrocks = 0;
            if (tv.material == MAT_GLASS) nrocks = (int)(nrocks * 0.3);   // S-03: a few blast-thrown boulders on the sheets
            for (int i = 0; i < nrocks; i++) {
                uint64_t hr = mix64(h + 17 * (i + 1));
                double rx = cx * cs + hash01(hr) * cs, rz = cz * cs + hash01(mix64(hr + 1)) * cs;
                double size = rockScale * (0.35 + 2.0 * std::pow(hash01(mix64(hr + 2)), 2.5)) * (1 + 0.6 * tv.scree / 255.0);   // O6-04: bigger on the scree
                if (size < 0.6) continue;   // stepped over
                double gy = site.groundHeight(rx, rz);
                if (gy < site.waterAt(rx, rz)) continue;
                out.push_back({rx, rz, size * 0.8, 0, gy - size * 0.15, gy + size * 1.15});   // the peak stands up to 1.3 sizes over a base sunk 0.15
            }
            // N2: trunks and fallen logs from the same enumerators that draw them
            forTrees(cx, cz, [&](const TreeInst& T) { out.push_back({T.x, T.z, 0.12 + T.h * 0.02, 1, T.gy - 0.3, T.gy + T.h}); });
            forLogs(cx, cz, [&](const LogInst& L) {
                double dx = std::sin(L.heading), dz = std::cos(L.heading), lg = site.groundHeight(L.x, L.z);
                for (int k = -1; k <= 1; k++) out.push_back({L.x + dx * L.len * 0.33 * k, L.z + dz * L.len * 0.33 * k, L.radius + 0.35, 2, lg - 1.0, lg + 2 * L.radius + 0.6});
            });
        }
    (void)treeScale;
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
        if (in.moveY != 0 || in.moveX != 0) { fwd = in.moveY; str = in.moveX; }   // M6-02: an analogue stick
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
    // wading (M8-08): slower with depth until swimming takes over
    double groundHere = site.groundHeight(p.x, p.z);
    double waterHere = site.waterAt(p.x, p.z);
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
    // slopes (M8-01): the ground gradient decides climbing and sliding
    double gx = (site.groundHeight(p.x + 0.5, p.z) - site.groundHeight(p.x - 0.5, p.z)),
           gz = (site.groundHeight(p.x, p.z + 0.5) - site.groundHeight(p.x, p.z - 0.5));
    double slope = std::sqrt(gx * gx + gz * gz);   // rise per run
    bool steep = slope > 1.35 && p.onGround && !p.swimming;
    double hNew = site.groundHeight(nx, nz);
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
    double ground = site.groundHeight(p.x, p.z);
    double water = site.waterAt(p.x, p.z);
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
    switch (b.type) {
        case PT_THINATMO: low = u < 0.5 ? green : teal; high = violet; break;
        case PT_DESERT: low = u < 0.5 ? lime : green; high = pink; break;
        case PT_TECTONIC: low = blue; high = magenta; break;
        case PT_ACIDIC: low = gold; high = orange; break;
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
        if (site.gen.type == PT_THINATMO) sunsetC = lerp(sunsetC, site.gen.skyTint * 1.3f, 0.65f);   // exotic twilight (M1-07)
        hor = lerp(hor, sunsetC, (float)(sunset * 0.8));
        zen = lerp(zen, sunsetC * 0.4f, (float)(sunset * 0.25));
    }
    double rainDim = 1 - 0.35 * env.rain;
    hor = hor * (float)rainDim; zen = zen * (float)rainDim;
    RGB fog = site.atmosphere ? hor : lerp(L.ground, white, 0.55f);
    RGB glow = lerp(hor, starC, 0.5f);
    RGB sunC = site.atmosphere ? lerp(starC, white, 0.6f) : lerp(env.sunDisc.color, white, 0.35f);
    if (env.hasSun2) {   // M5-01 bank 12: the companion's disc and glow in its own colour
        RGB kc = env.sun2.color;
        RGB kd = site.atmosphere ? lerp(lerp(kc, white, 0.5f), white, 0.6f) : lerp(kc, white, 0.35f);
        setRamp(fb.pal, 12, {{0, RGB(0, 0, 0)}, {20, kc * 0.45f}, {44, kc}, {56, kd}, {63, lerp(kd, white, 0.5f)}});
    }
    if (env.moonBody >= 0 && site.sys->bodies[env.moonBody].type == PT_SUBSTELLAR) {   // M5-02 bank 13: a glowing parent in the sky
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
        MatRamp r = materialRamp(c, site.atmosphere ? &skyLight : nullptr, day);
        RGB dark = r.stop[1];
        // planetshine lifts the dark end at night only (N1-05: on airless worlds `day` is the sky, which is always dark, so use the sun)
        double night = site.atmosphere ? (1 - day) : (1 - smoothstep(-0.05, 0.1, sinAlt));
        if (env.moonLight > 0.01) dark = lerp(dark, env.moonColor * (0.22f + 0.3f * (float)env.moonLight), (float)(night * clampd(env.moonLight * 2.5, 0, 0.7)));
        RGB lit = c * sunsetTint * (float)(1.0 - 0.12 * overcast);
        RGB mid = lerp(dark, lit, 0.55f);
        setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {14, dark}, {34, mid}, {47, lit}, {56, lerp(lit, white, 0.5f)}, {63, fog}});
    };
    matRamp(0, L.ground);
    if (isLavaWorld(site.gen.type)) {
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
        RGB bark = lerp(RGB(0.36f, 0.25f, 0.15f), site.gen.vegColor2 * 0.55f, 0.3f);
        matRamp(16, bark);
        matRamp(17, RGB(0.74f, 0.62f, 0.44f));
    }
    if (site.gen.type == PT_COMET) setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {18, RGB(0.10f, 0.14f, 0.30f)}, {40, RGB(0.32f, 0.48f, 0.85f)}, {63, RGB(0.7f, 0.8f, 1.0f)}});   // O4 bank 15: the ion tail overhead
    // bank 4: stars (their own ramp, no longer borrowed from the ground)
    setRamp(fb.pal, 4, {{0, RGB(0, 0, 0)}, {40, RGB(0.55f, 0.57f, 0.66f)}, {63, RGB(0.88f, 0.9f, 0.97f)}});
    {   // G-03 bank 22: the galactic band and the nebula patches, the unresolved starlight, laid over the sky at night by
        // `drawBand`: from the night sky's own colour at the dark end (no hue edge where it fades into the sky) to a pale
        // star-grey, a little of the horizon's tint through an atmosphere
        RGB pale = lerp(RGB(0.55f, 0.57f, 0.66f), hor, site.atmosphere ? 0.25f : 0.0f);
        if (site.sys->star.cls == STAR_PROTOSTAR) {   // S-04: the sky's glow takes the protostar's cloud's tone
            int tone = starNebulaTone(site.sys->star);
            pale = lerp(pale, tone == 0 ? RGB(0.45f, 0.55f, 0.85f) : (tone == 1 ? RGB(0.85f, 0.50f, 0.45f) : RGB(0.70f, 0.70f, 0.75f)), 0.6f);
        }
        setRamp(fb.pal, 22, {{0, site.atmosphere ? zen : RGB(0, 0, 0)}, {40, pale}, {63, lerp(pale, white, 0.4f)}});
    }
    // sky bank: zenith .. horizon .. glow .. sun/star white
    if (site.atmosphere) {
        setRamp(fb.pal, 1, {{0, zen}, {40, hor}, {50, glow}, {56, lerp(glow, sunC, 0.6f)}, {63, sunC}});
        // at night the upper part must still serve stars: brighten toward white
        if (day < 0.5) {
            RGB starWhite = RGB(0.8f, 0.82f, 0.9f);
            setRamp(fb.pal, 1, {{0, zen}, {36, hor}, {44, lerp(hor, starWhite, 0.25f)}, {54, lerp(hor, starWhite, (float)(0.6 + 0.3 * day))}, {63, lerp(starWhite, sunC, (float)day)}});
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
    // N1-03: one bank per material family, the same families as the globe (matFamily)
    switch (matFamily(v.material)) {
        case FAM_WATER: bank = 2; break;
        case FAM_FOREST: bank = 3; albedo = 0.28 + 0.1 * v.veg; break;
        case FAM_GRASS: bank = 10; albedo = 0.36 + 0.2 * v.veg; break;
        case FAM_SNOW: bank = 8; if (v.material == MAT_SNOW) albedo = site.gen.type == PT_FELISIAN ? 0.95 : 0.9; break;
        case FAM_SAND: bank = 9; break;
        default: bank = v.material == MAT_LAVA ? 2 : 0; break;
    }
}

const RVert& SurfaceView::vertexOf(TerrainCache& cache, std::vector<VtxCache>& vcache, int cx, int cz, uint8_t& bank) {
    int n = 1 << cache.logN;
    int idx = ((cz & (n - 1)) << cache.logN) | (cx & (n - 1));
    VtxCache& vc = vcache[idx];
    const TerrainVertex& tv = cache.at(cx, cz);
    if (vc.stamp == frameStamp && vc.v.u == (double)(cx * cache.cellSize) && vc.v.v == (double)(cz * cache.cellSize)) {
        bank = vc.bank;
        return vc.v;
    }
    double cs = cache.cellSize;
    double x = cx * cs, z = cz * cs;
    double h = tv.h;
    if (tv.material == MAT_WATER && tv.water > -1e8f) h = std::min(h, (double)tv.water - 0.5);
    // B-313 geomorphing: within MORPH_CELLS of the ring's edge the vertex slides onto the coarser ring's surface (the
    // height of its drawn triangle at this spot, and its Gouraud shade when the coarser vertices were computed this
    // frame), by the vertex's continuous distance from the camera, so nothing pops as the edge sweeps the ground
    double morph = 0, hC = 0, shC = 0; bool shOk = false;
    double wy = tv.water;   // B-320: the water level as drawn
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
        sunTerm *= 1 - 0.55 * smoothstep(cloudThr, cloudThr + 0.18, n);
    }
    double light = ambient + (1 - ambient) * sunTerm;
    // M10-09 secondary lights: planetshine at night, the capsule beacon pool, lava glow
    if (env.moonLight > 0.005) light += (1 - ambient) * env.moonLight * 0.6 * std::max(0.0, dot(nrm, env.moonDir)) * (1 - sunUp);
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
    if (isLavaWorld(site.gen.type) && tv.material != MAT_LAVA && cache.cellSize <= 16) {
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
    double fog = 1 - std::exp(-dist / fogD);
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
    vc.stamp = frameStamp;
    bank = bk;
    return vc.v;
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
    for (int k = 0; k < 4; k++) { const TerrainVertex& t = c.at(X + (k & 1), Z + (k >> 1)); if (t.water > -1e8f) { wMean += t.water; wN++; } }
    if (!wN) return -1e9;
    wMean /= wN;
    double w[3];
    for (int k = 0; k < 3; k++) { const TerrainVertex& t = c.at(X + ox[k], Z + oz[k]); w[k] = t.water > -1e8f ? t.water : wMean; }
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
void SurfaceView::drawFloor(Framebuffer& fb) {
    if (site.smallBody) return;   // O4: a small body's terrain disc reaches past its own horizon
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
    if (site.hasWater) h = site.seaLevel;
    int ty = site.gen.type;
    uint8_t bank = site.hasWater ? 2 : ((ty == PT_ICY || ty == PT_EUROPAN) ? 8 : ((ty == PT_CRATERED || ty == PT_THINATMO || ty == PT_QUARTZ || ty == PT_VOLCANIC || ty == PT_DESERT || ty == PT_BOMBARDED) ? 9 : 0));   // N1-03: the dominant family
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
    int misses = 0;
    // the terrain cells of the whole square first, serially (a miss samples the planet function and writes the cache;
    // the shadow march of vertexOf reads cells anywhere within the radius), and the tiles of their materials
    // O6-06: within the disc (plus two cells for the normals): the square's corners lay 75 km out on the far ring, beyond
    // the drainage tiles a landing prefetches, and the first frame computed those tiles (270 ms)
    double rFill2 = (double)(radius + 2) * (radius + 2);
    for (int cz = pcz - radius - 1; cz <= pcz + radius + 1; cz++)
        for (int cx = pcx - radius - 1; cx <= pcx + radius + 1; cx++) {
            double ddx = cx + 0.5 - camPos.x / cs, ddz = cz + 0.5 - camPos.z / cs;
            if (ddx * ddx + ddz * ddz > rFill2) continue;
            if (trace) { const TerrainVertex& tv = cache.v[cache.index(cx, cz)]; if (tv.cx != cx || tv.cz != cz) misses++; }
            mesoFor(cache.at(cx, cz).material);
        }
    auto t1 = std::chrono::steady_clock::now();
    // then the vertices, rows of cells in parallel: a vertex belongs to one row, so the writes are disjoint
    int rows = 2 * radius;
    parallelFor(rows, 4, [&](int rb, int re) {
        for (int cz = pcz - radius + rb; cz < pcz - radius + re; cz++) {
            for (int cx = pcx - radius; cx < pcx + radius; cx++) {
                double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
                double d2 = dx * dx + dz * dz;
                if (d2 > r2) continue;
                if (cellCovered(cs, cx, cz, warmCover)) continue;
                {
                    double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
                    double fwdx = camLocal.m[2][0], fwdz = camLocal.m[2][2];
                    double dd = std::sqrt(wx * wx + wz * wz);
                    if (dd > cs * 3 && (wx * fwdx + wz * fwdz) / dd < -0.25) continue;
                }
                uint8_t bk;
                vertexOf(cache, vcache, cx, cz, bk);
                vertexOf(cache, vcache, cx + 1, cz, bk);
            }
        }
    });
    // the row above the last one (its vertices belong to cells drawn from the row below)
    for (int cz = pcz - radius; cz < pcz + radius; cz++) {
        for (int cx = pcx - radius; cx < pcx + radius; cx++) {
            double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
            double d2 = dx * dx + dz * dz;
            if (d2 > r2) continue;
            if (cellCovered(cs, cx, cz, warmCover)) continue;
            {
                double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
                double fwdx = camLocal.m[2][0], fwdz = camLocal.m[2][2];
                double dd = std::sqrt(wx * wx + wz * wz);
                if (dd > cs * 3 && (wx * fwdx + wz * fwdz) / dd < -0.25) continue;
            }
            uint8_t bk;
            vertexOf(cache, vcache, cx, cz + 1, bk);
            vertexOf(cache, vcache, cx + 1, cz + 1, bk);
        }
    }
    if (trace && frameStamp <= 3) { auto t2 = std::chrono::steady_clock::now(); fprintf(stderr, "    warm %4d m: %d misses, fill %.1f ms, vertices %.1f ms\n", (int)cs, misses, std::chrono::duration<double, std::milli>(t1 - t0).count(), std::chrono::duration<double, std::milli>(t2 - t1).count()); }
}

void SurfaceView::drawTerrainLOD(Framebuffer& fb, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius,
                                 double coverM, bool water, double t, int bandY0, int bandY1, bool skirt) {
    double cs = cache.cellSize;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
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
    double sunUp = smoothstep(-0.03, 0.06, env.sun.dirLocal.y);
    for (int cz = pcz - radius; cz < pcz + radius; cz++) {
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
            uint8_t b00, b10, b01, b11;
            const RVert& v00 = vertexOf(cache, vcache, cx, cz, b00);
            const RVert& v10 = vertexOf(cache, vcache, cx + 1, cz, b10);
            const RVert& v01 = vertexOf(cache, vcache, cx, cz + 1, b01);
            const RVert& v11 = vertexOf(cache, vcache, cx + 1, cz + 1, b11);
            if (v00.z < NEAR_Z && v10.z < NEAR_Z && v01.z < NEAR_Z && v11.z < NEAR_Z) continue;
            auto outside = [&](int ncx, int ncz) { double ex = ncx + 0.5 - (camPos.x / cs), ez = ncz + 0.5 - (camPos.z / cs); return ex * ex + ez * ez > r2; };
            const TerrainVertex& t00 = cache.at(cx, cz);
            const TerrainVertex& t10 = cache.at(cx + 1, cz);
            const TerrainVertex& t01 = cache.at(cx, cz + 1);
            const TerrainVertex& t11 = cache.at(cx + 1, cz + 1);
            double hmin = std::min(std::min(t00.h, t10.h), std::min(t01.h, t11.h));
            int n = 1 << cache.logN, mk = n - 1;
            const VtxCache& c00 = vcache[((cz & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c10 = vcache[((cz & mk) << cache.logN) | ((cx + 1) & mk)];
            const VtxCache& c01 = vcache[(((cz + 1) & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c11 = vcache[(((cz + 1) & mk) << cache.logN) | ((cx + 1) & mk)];
            // B-320: this cell's water surface per corner (the level as drawn, morphed with the ground); a corner without
            // water takes the mean of the others, so the sheet is one continuous plane cut by the ground
            double wCorner[4] = {c00.wy, c10.wy, c11.wy, c01.wy};
            double wMean = 0, wMax = -1e9; int wN = 0;
            for (int i = 0; i < 4; i++) if (wCorner[i] > -1e8) { wMean += wCorner[i]; wN++; wMax = std::max(wMax, wCorner[i]); }
            if (wN) { wMean /= wN; for (int i = 0; i < 4; i++) if (wCorner[i] < -1e8) wCorner[i] = wMean; }
            // B-322: the sheet is drawn where a corner lies on the water's side of its edge (`shoreValue`); it used to be drawn
            // wherever a corner lay within half a metre of the level, which reached over the low ground beyond a river's plain
            // as a moat of cells
            bool anyWet = false, anyDry = false;
            double shc[4] = {c00.sh, c10.sh, c01.sh, c11.sh};   // in the vs order: 00, 10, 01, 11 (as drawn: morphed at the ring's edge)
            if (water && wN > 0) for (int k = 0; k < 4; k++) { if (shc[k] < 0) anyWet = true; else anyDry = true; }
            bool wet = water && wN > 0 && anyWet;
            (void)hmin; (void)wMax;
            {   // B-310: the band cull: a cell whose four corners project wholly above or below this thread's rows is skipped,
                // unless a skirt or its water could reach into them
                if (c00.front && c10.front && c01.front && c11.front) {
                    float ymin = std::min(std::min(c00.sy, c10.sy), std::min(c01.sy, c11.sy)), ymax = std::max(std::max(c00.sy, c10.sy), std::max(c01.sy, c11.sy));
                    if ((ymax < bandY0 - 1 || ymin > bandY1 + 1) && !wet && !(outside(cx + 1, cz) || outside(cx - 1, cz) || outside(cx, cz + 1) || outside(cx, cz - 1))) continue;
                }
            }
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
                const VtxCache* vcs[4] = {&c00, &c10, &c01, &c11};
                const int oxv[4] = {0, 1, 0, 1}, ozv[4] = {0, 0, 1, 1};
                const double wlv[4] = {wCorner[0], wCorner[1], wCorner[3], wCorner[2]};   // wCorner runs 00, 10, 11, 01
                static const int trisV[2][3] = {{0, 1, 2}, {1, 3, 2}};
                for (int k = 0; k < 2; k++) {
                    double xs[3], zs[3], hs[3], wl[3], sh[3];
                    for (int e = 0; e < 3; e++) { int c = trisV[k][e]; xs[e] = (cx + oxv[c]) * cs; zs[e] = (cz + ozv[c]) * cs; hs[e] = vcs[c]->hy; wl[e] = wlv[c]; sh[e] = shc[c]; }
                    ShoreSplit sp;
                    shoreSplitTriangle(xs, zs, hs, wl, sh, sp);
                    auto emitPiece = [&](const ShorePt* pts, int n, bool dry) {
                        if (n < 3) return;
                        RVert q[4];
                        for (int i = 0; i < n; i++) {
                            int a = trisV[k][pts[i].a], b = trisV[k][pts[i].b];
                            const RVert& va = *vs[a]; const RVert& vb = *vs[b];
                            double t = pts[i].t;
                            if (pts[i].a == pts[i].b) { q[i] = va; q[i].w = w[a]; }
                            else {
                                // O6-03: the dry piece's edge stands SHORE_LIP over the water, the wet piece's lies 3 cm under it: with one shared
                                // point under the plane, the sheet covered the dry piece out to wherever it rose through the plane, which on a
                                // flat shore was near the next corner, and the water's edge ran along the cells' edges in stairs
                                Vec3 vv = toView(pts[i].x, pts[i].h + (dry ? SHORE_LIP : 0.0), pts[i].z);
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
                double skirt = cs * 1.5;
                auto wall = [&](const RVert& a, const RVert& b, double ax, double az, double bx, double bz) {
                    const TerrainVertex& ta = cache.at((int)std::lround(ax / cs), (int)std::lround(az / cs));
                    const TerrainVertex& tb = cache.at((int)std::lround(bx / cs), (int)std::lround(bz / cs));
                    Vec3 la = toView(ax, ta.h - skirt, az), lb = toView(bx, tb.h - skirt, bz);
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
            if (water && !(skipEnv && std::strstr(skipEnv, "wq"))) {
                if (wet) {
                    RVert w[4];
                    const TerrainVertex* tv[4] = {&t00, &t10, &t11, &t01};
                    const VtxCache* vcw[4] = {&c00, &c10, &c11, &c01};
                    int ox[4] = {0, 1, 1, 0}, oz[4] = {0, 0, 1, 1};
                    double wdx = std::sin(env.windDir), wdz = std::cos(env.windDir);
                    for (int i = 0; i < 4; i++) {
                        double x = (cx + ox[i]) * cs, z = (cz + oz[i]) * cs;
                        double ddx = x - camPos.x, ddz = z - camPos.z;
                        double wl = wCorner[i];
                        Vec3 vv = toView(x, wl, z);
                        double depth = clampd(wl - vcw[i]->hy, 0, 8);   // B-322: the depth under the drawn ground (morphed at the ring edges), or the shade jumped at every seam
                        Vec3 toV = Vec3(ddx, wl - camPos.y, ddz);
                        double dist = length(toV);
                        Vec3 rd = normalize(Vec3(toV.x, -toV.y, toV.z));
                        double glint = std::pow(std::max(0.0, dot(rd, env.sun.dirLocal)), 28.0) * 24 * sunUp * env.sun.lightFactor;
                        // M1-04: wave rings coming in with the wind
                        double wave = 2.5 * std::sin((x * wdx + z * wdz) / 2.6 - t * 1.4 + 0.8 * std::sin((x * wdz - z * wdx) / 7.0));
                        // B-320: the water's own colour seen from above (deep dark, shallow lighter), the sky's colour at grazing
                        // angles (a Fresnel term: nearly all of the far water mirrors the sky), the ramp darkened by the night
                        double cosT = clampd(std::fabs(toV.y) / std::max(dist, 0.1), 0.04, 1);   // B-322: the waves' slopes keep the mirror short of grazing: from an eye at the surface the water stays water, not a sheet of sky
                        double fres = 0.04 + 0.96 * std::pow(1 - cosT, 4.0);
                        double body = 24 + (1 - depth / 8) * 8, skyR = 55;
                        double base = body + (skyR - body) * fres + glint + wave * (0.5 + 0.5 * clampd(env.windKnots / 15.0, 0, 1));
                        double fog = 1 - std::exp(-dist / env.fogDistance);
                        w[i].x = vv.x; w[i].y = vv.y; w[i].z = vv.z;
                        w[i].shade = base + (63 - base) * fog;
                        w[i].u = x + t * 0.7; w[i].v = z + t * 0.4;
                    }
                    rpw.bank = 2;
                    rasterPolygon(fb, w, 4, rpw, proj);
                    // foam sparkle where the shore is shallow and close
                    double cdist = std::sqrt(d2) * cs;
                    if (cdist < 90 && cache.cellSize <= 16 && (cs >= 16 || ((cx & 3) == 0 && (cz & 3) == 0))) {   // O6-02: one near cell in sixteen keeps the density
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
                }
            }
        }
    }
}

void SurfaceView::drawSky(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr) {
    const Body& b = site.sys->bodies[site.body];
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    Mat3 camT = camLocal.transposed();
    const Vec3 sd = env.sun.dirLocal;
    double sinAlt = sd.y;
    double day = env.skyBrightness;
    double sunUp = smoothstep(-0.28, 0.0, sinAlt);
    bool venus = hasOpaqueDeck(b.type);   // R-307: the hydrocarbon haze is a deck like the venusian clouds
    double density = L.skyDensity * (1 - 0.3 * env.rain);
    double halo = (site.atmosphere && !venus && env.temperatureC < -5 && env.cloudCover < 0.35 && sinAlt > 0.05) ? 0.8 : 0.0;
    // N5-02: a sun pillar in cold, clear air when the sun is low (ice crystals in the air mirror it into a column)
    double pillar = (site.atmosphere && !venus && env.temperatureC < -5 && env.cloudCover < 0.45 && sinAlt > -0.03 && sinAlt < 0.2) ? (1 - std::max(0.0, sinAlt) / 0.2) : 0.0;
    double sunAltA = std::asin(clampd(sinAlt, -1, 1));
    double wedge = (site.atmosphere && !venus && sinAlt > -0.17 && sinAlt < 0.06) ? 1.0 - std::fabs(sinAlt + 0.05) / 0.11 : 0.0;
    if (wedge < 0) wedge = 0;
    Vec3 sdh = normalize(Vec3(sd.x, 0, sd.z + 1e-9));
    // cloud grid
    bool clouds = site.atmosphere && L.cloudsLayer && env.cloudCover > 0.03;
    double aurora = env.aurora;
    const int CELL = 4 * FB_SCALE;
    const int GX = FBW / CELL + 1, GY = FBH / CELL + 1;
    if (clouds) {
        cloudGrid.assign(GX * GY * 2, 0.f);
        double thr = 1.0 - env.cloudCover * 0.75;
        double wx = windDriftX, wz = windDriftZ;
        // two layers (M10-14): low cumulus at 2600 m with lit tops and shaded bases, high thin veils at 6000 m
        for (int gy = 0; gy < GY; gy++)
            for (int gx = 0; gx < GX; gx++) {
                int px = std::min(gx * CELL, FBW - 1), py = std::min(gy * CELL, FBH - 1);
                Vec3 d = camT * dirLUT[py * FBW + px];
                double el = d.y;
                float dens = 0, idx = 0;
                if (el > 0.012) {
                    double tt = 2600.0 / el;
                    double x = camPos.x + d.x * tt + wx, z = camPos.z + d.z * tt + wz;
                    double n = 0.5 + 0.5 * fbm2(x / 1800.0, z / 1800.0, site.gen.seed + 77, 4, 2.2, 0.55);
                    double dLow = smoothstep(thr, thr + 0.18, n) * smoothstep(0.012, 0.09, el);
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
                    double dHigh = smoothstep(thr + 0.05, thr + 0.3, nh) * 0.4 * smoothstep(0.012, 0.06, el);
                    double idxHigh = day * (38 + 6 * cosSun) + (1 - day) * 5;
                    double dAll = dLow + dHigh * (1 - dLow);
                    dens = (float)dAll;
                    idx = (float)(dAll > 1e-6 ? (idxLow * dLow + idxHigh * dHigh * (1 - dLow)) / dAll : 0);
                    if (env.rain > 0) idx *= (float)(1 - 0.35 * env.rain);
                }
                cloudGrid[(gy * GX + gx) * 2] = dens;
                cloudGrid[(gy * GX + gx) * 2 + 1] = idx;
            }
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
        const double P0 = clampd((auroraOvalLat(b) - 6 * storm - std::fabs(env.latDeg)) * kmDeg, -120.0, 450.0) + 30.0 * std::sin(t / 310.0);
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
        const double topGain = (0.25 + 0.75 * storm) * (0.6 + 0.8 * unitFromHash(hashCombine(site.gen.seed, 0xA0D)));
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
    // O4: on a comet the coma hazes the whole sky, brightest toward the sun, and the ion tail streams from the anti-solar
    // point; both grow with the activity
    const bool comet = b.type == PT_COMET && env.cometActivity > 0.02;
    const double act = env.cometActivity;
    double knotT[5]; for (int k = 0; k < 5; k++) knotT[k] = SpaceRenderer::cometKnot(k, t);
    int rowsTotal = (FBH + step - 1) / step;
    parallelFor(rowsTotal, 40, [&](int rb, int re) {
    for (int y = rb * step; y < re * step && y < FBH; y += step) {
        for (int x = 0; x < FBW; x += step) {
            int o = y * FBW + x;
            Vec3 d = camT * dirLUT[o];
            double el = d.y;
            double v;
            double tailV = 0;
            if (!site.atmosphere) {
                double cosSun = dot(d, sd);
                v = std::pow(std::max(0.0, cosSun), 300.0) * 22;
                if (twoSuns) v += std::pow(std::max(0.0, dot(d, sd2)), 300.0) * 22 * sun2Up;
                if (comet) {
                    double cs = std::max(0.0, cosSun);
                    v += act * (6 + 14 * cs * cs * cs);
                    double tail = std::pow(std::max(0.0, -cosSun), 18.0) * act;   // a cone some 15 degrees wide round the anti-solar point
                    if (tail > 0.02 && el > -0.05) {
                        // the knots of the ion tail stream outward: brightness along the anti-solar distance
                        double f = std::acos(clampd(-cosSun, -1, 1)) / (0.5 * PI);   // 0 at the anti-solar point .. 1 at 90 degrees
                        double knots = 0;
                        for (int k = 0; k < 5; k++) { double dd = f - knotT[k]; knots += std::exp(-dd * dd / (2 * 0.06 * 0.06)); }
                        tailV = 6 + 26 * tail * (0.8 + 0.3 * knots);
                    }
                }
            } else {
                double hz = std::pow(1 - clampd(el, 0, 1), 2.2);
                double base;
                if (venus) base = 26 + 14 * hz;
                else base = day * (15 + 25 * hz) + (1 - day) * (2 + 5 * hz);
                base += 6.0 * day * smoothstep(0.09, 0.0, el);   // M1-07: haze band on the horizon
                base *= density;
                double cosSun = dot(d, sd);
                double cs = std::max(0.0, cosSun);
                double g = (std::pow(cs, 6.0) * 9 + std::pow(cs, 50.0) * 14) * sunUp * (venus ? 1.6 : 1.0);
                if (twoSuns) { double cs2 = std::max(0.0, dot(d, sd2)); g += (std::pow(cs2, 6.0) * 9 + std::pow(cs2, 50.0) * 14) * sun2Up; }
                v = base + g;
                // M10-14: 22-degree halo in cold, clear air; twilight wedge opposite a low sun
                if (halo > 0) { double ang = std::acos(clampd(cosSun, -1, 1)) / DEG; double dh = std::fabs(ang - 22.0); if (dh < 1.4) v += 5.0 * halo * (1 - dh / 1.4); }
                if (pillar > 0 && cosSun > 0.85) {
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
                    const float* c00 = &cloudGrid[(gy * GX + gx) * 2];
                    const float* c10 = &cloudGrid[(gy * GX + gx + 1) * 2];
                    const float* c01 = &cloudGrid[((gy + 1) * GX + gx) * 2];
                    const float* c11 = &cloudGrid[((gy + 1) * GX + gx + 1) * 2];
                    double dens = (c00[0] * (1 - fx) + c10[0] * fx) * (1 - fy) + (c01[0] * (1 - fx) + c11[0] * fx) * fy;
                    double ci = (c00[1] * (1 - fx) + c10[1] * fx) * (1 - fy) + (c01[1] * (1 - fx) + c11[1] * fx) * fy;
                    v = v + (ci - v) * dens;
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
                                double rv = 36 + 24 * std::pow(clampd(dens * 1.3, 0, 1), 0.35) * lit;   // the sky bank is black below 30
                                if (site.atmosphere) rv = rv * (1 - 0.3 * day) + v * 0.45 * day;   // washed by daylight like the day moon
                                if (rv > v) v = rv;
                            }
                        }
                    }
                }
            }
            Pix pv = pix(1, clampd(v, 0, 51.0));
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
    double starScale = site.atmosphere ? (1.0 - 0.9 * day) * (1 - env.rain) : 1.0;
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
    const double bandGain = venus ? 0 : 20.0 * (site.atmosphere ? (1.0 - day) * (1 - env.rain) : 1.0);   // the bulge 17 shades on a dark night, the plane 6-8, the wings under 2 left to the sky
    NebulaPatch nebAll[18]; int nebAllN = nebN;   // S-04: the field's patches and a protostar's own cloud round the star's direction now
    for (int k = 0; k < nebN; k++) nebAll[k] = nebP[k];
    nebAllN += starNebulaPatches(site.sys->star, normalize(site.sys->star.pos - site.sys->bodyPos(site.body, t)), nebAll + nebAllN);
    if (bandGain > 0.05) {
        double lumSky[64], lumBand[64];
        for (int k = 0; k < 64; k++) {
            const uint8_t* c1 = &fb.pal[(1 * 64 + k) * 3]; lumSky[k] = 0.2126 * c1[0] + 0.7152 * c1[1] + 0.0722 * c1[2];
            const uint8_t* c2 = &fb.pal[(22 * 64 + k) * 3]; lumBand[k] = 0.2126 * c2[0] + 0.7152 * c2[1] + 0.0722 * c2[2];
        }
        parallelFor(rowsTotal, 40, [&](int rb, int re) {
        for (int y = rb * step; y < re * step && y < FBH; y += step) {
            for (int x = 0; x < FBW; x += step) {
                int o = y * FBW + x;
                if (bankOf(fb.idx[o]) != 1) continue;
                Vec3 d = camT * dirLUT[o];
                if (d.y <= 0) continue;
                Vec3 dW = toWorld * d;
                double band = bandAt(dW);
                if (nebAllN) { int tone; band = std::max(band, 0.9 * nebulaGlow(nebAll, nebAllN, dW, tone)); }
                double bs = band * bandGain;
                if (clouds) {   // the clouds hide it
                    int gx = x / CELL, gy = y / CELL;
                    double fx = (x - gx * CELL) / (double)CELL, fy = (y - gy * CELL) / (double)CELL;
                    double dens = (cloudGrid[(gy * GX + gx) * 2] * (1 - fx) + cloudGrid[(gy * GX + gx + 1) * 2] * fx) * (1 - fy) + (cloudGrid[((gy + 1) * GX + gx) * 2] * (1 - fx) + cloudGrid[((gy + 1) * GX + gx + 1) * 2] * fx) * fy;
                    bs *= 1 - dens;
                }
                if (bs < 1.5) continue;   // the wings would only lift the sky by a level or two: leave the sky bank its pixels
                int bi = std::min(63, (int)bs);
                Pix pv = pix(22, bs);
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) {
                        Pix& p = fb.idx[yy * FBW + xx];
                        if (bankOf(p) != 1 || lumBand[bi] <= lumSky[std::min(63, intenOf(p) / INTEN_PER_SHADE)]) continue;
                        p = pv;
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
                double sunI = inten * (site.atmosphere ? clampd(0.55 + 0.45 * smoothstep(-0.05, 0.1, sinAltD), 0, 1) : 1.0);
                Vec3 discUp = camLocal * (site.localFrame(t) * Vec3(0, 1, 0));   // S-04: the orbital plane's normal, for a protostar's disc
                SpaceRenderer::drawSun(fb, site.sys->star, v, env.sunDisc.angularRadius, t, sunI, 1, site.atmosphere, haze, true, proj, &discUp);   // B-308: by angle
                if (v.z <= 0.001) return;   // the eclipse glow and the flare need the projected centre
                double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
                double rpx = proj.f * std::tan(env.sunDisc.angularRadius) / v.z;
                // B-309: the veiling glare over the finished picture (drawVeil), from the sun's place and brightness
                sunSx = sx; sunSy = sy; sunRpx = rpx;
                sunVeil = site.sys->star.cls == STAR_BLACK_HOLE ? 0.0 : sunI * (1 - env.sunDisc.eclipse) * (site.atmosphere ? (1 - 0.6 * env.cloudCover) * (1 - env.rain) * smoothstep(-0.02, 0.12, sinAltD) : 0.7);   // S-06: no glare from a hole
                if (env.sunDisc.eclipse > 0.3 && !venus) {
                    double r = std::max(rpx, 1.5 * FB_SCALE), e = env.sunDisc.eclipse;
                    fb.glowDisc(sx, sy, r * 4.0, r * 1.05, (int)(34 * e * e), 1, true);
                    fb.glowDisc(sx, sy, r * 9.0, r * 1.1, (int)(10 * e * e), 1, true);
                }
                int ix = (int)sx, iy = (int)sy;
                bool centreFree = ix >= 0 && iy >= 0 && ix < FBW && iy < FBH && fb.invz[iy * FBW + ix] <= 1e-12f;
                // M1-03 lens flare, dimmed by haze and rain, only with the sun well up and unobstructed
                double flareI = sunI * (1 - env.sunDisc.eclipse) * (site.atmosphere ? (1 - 0.5 * env.cloudCover) * (1 - env.rain) * smoothstep(0.0, 0.15, sinAltD) : 0.8);
                if (!venus && centreFree && flareI > 0.05 && site.sys->star.cls != STAR_BLACK_HOLE) SpaceRenderer::drawLensFlare(fb, sx, sy, std::max(rpx, 2.0 * FB_SCALE), flareI, 1, proj);   // S-06: nor a flare
            }
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
    double rockDensity = 0.0, rockScale = 1.0;
    switch (type) {
        case PT_CRATERED: rockDensity = 1.1; rockScale = 1.1; break;
        case PT_ROCKY: rockDensity = 1.6; rockScale = 1.6; break;
        case PT_THINATMO: rockDensity = 1.2; rockScale = 0.9; break;
        case PT_MOLTEN: rockDensity = 0.7; rockScale = 1.1; break;
        case PT_ICY: rockDensity = 0.3; rockScale = 1.3; break;
        case PT_FELISIAN: rockDensity = 0.35; rockScale = 0.9; break;
        case PT_VENUSIAN: rockDensity = 0.6; rockScale = 1.4; break;
        case PT_QUARTZ: rockDensity = 0.5; rockScale = 0.8; break;
        case PT_METAL: rockDensity = 1.3; rockScale = 1.2; break;
        case PT_VOLCANIC: rockDensity = 0.8; rockScale = 1.0; break;
        case PT_CARBON: rockDensity = 0.9; rockScale = 1.1; break;
        case PT_COMET: rockDensity = 1.4; rockScale = 1.2; break;   // O4: boulders of dirty ice
        case PT_OCEAN: rockDensity = 0.0; break;
        case PT_EUROPAN: rockDensity = 0.25; rockScale = 1.0; break;   // R-307
        case PT_TECTONIC: rockDensity = 1.0; rockScale = 1.2; break;
        case PT_DESERT: rockDensity = 0.9; rockScale = 1.0; break;
        case PT_HYDROCARBON: rockDensity = 0.3; rockScale = 0.9; break;
        case PT_BOMBARDED: rockDensity = 1.8; rockScale = 1.3; break;
        case PT_ACIDIC: rockDensity = 0.6; rockScale = 1.0; break;
    }
    int radius = 22;
    lastRocksDrawn = 0; lastRocksCandidates = 0;
    RasterParams rp; rp.bank = type == PT_FELISIAN ? 0 : 3;   // N1-03: boulders in the rock colour (bank 3 is the forest on living worlds)
    rp.grain = &grain; rp.grainScale = 3.0 * FB_SCALE;
    rp.grain2 = mesoFor(type == PT_FELISIAN ? MAT_FOREST : MAT_ROCK); rp.grain2Scale = 4.0;   // facets and lichen speckle
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
            const TerrainVertex& tv = site.lod0.at(cx, cz);
            bool lava = tv.material == MAT_LAVA;
            // rocks
            double rd = rockDensity;
            if (type == PT_FELISIAN && (tv.material == MAT_GRASS || tv.material == MAT_ROCK) && tv.biome != BIO_WETLAND) rd = 0.7;   // R-306: open ground carries more stone
            rd *= 1 + 5.0 * tv.scree / 255.0;   // O6-04: boulder fields on the scree under the cliffs
            int nrocks = (int)(rd * 2.0 * hash01(h));
            if (type == PT_FELISIAN && hash01(mix64(h + 5)) < 0.06 && tv.material != MAT_FOREST) nrocks += 3;   // R-306: an outcrop of three
            if (tv.material == MAT_SAND || tv.material == MAT_WATER || lava) nrocks = 0;
            if (tv.material == MAT_GLASS) nrocks = (int)(nrocks * 0.3);   // S-03: a few blast-thrown boulders on the sheets
            lastRocksCandidates += nrocks;
            for (int i = 0; i < nrocks; i++) {
                uint64_t hr = mix64(h + 17 * (i + 1));
                double rx = cx * cs + hash01(hr) * cs, rz = cz * cs + hash01(mix64(hr + 1)) * cs;
                double size = rockScale * (0.35 + 2.0 * std::pow(hash01(mix64(hr + 2)), 2.5)) * (1 + 0.6 * tv.scree / 255.0);   // O6-04: bigger on the scree
                double dist = std::sqrt((rx - camPos.x) * (rx - camPos.x) + (rz - camPos.z) * (rz - camPos.z));
                if (size / dist * proj.f < 0.8) continue;
                double gy = site.groundHeight(rx, rz);
                if (gy < site.waterAt(rx, rz)) continue;
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
            }
            // quartz crystals
            if (type == PT_QUARTZ && hash01(mix64(h + 99)) < 0.5) {
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
                    double shade = fogShade(46 + 14 * sunUp * hash01(mix64(hc + 5)), dist);
                    RVert tri[3];
                    Vec3 vs[3] = {toView(p0.x, p0.y, p0.z), toView(p1.x, p1.y, p1.z), toView(pk.x, pk.y, pk.z)};
                    for (int q = 0; q < 3; q++) { tri[q].x = vs[q].x; tri[q].y = vs[q].y; tri[q].z = vs[q].z; tri[q].shade = shade; }
                    rasterTriangle(fb, tri, rp2, proj);
                }
            }
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
    double baseShade = j.lava ? 61 : 38 + 22 * sunUp;   // steam and ice are lit by the sun; lava lights itself
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
    if (site.gen.type == PT_EUROPAN) {   // the cracks: vents hashed on 300 m cells where the 16 m ring is stained
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
    if (site.gen.type == PT_TECTONIC) {   // fumaroles and hot springs on the sulphur crusts, steady fountains on the lava (the great ones come and go through the eruption list)
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

void SurfaceView::drawWeather(Framebuffer& fb, double t) {
    if (site.gen.type == PT_COMET) drawCometJets(fb, t);   // O4
    if (site.gen.type == PT_EUROPAN || site.gen.type == PT_TECTONIC || site.gen.hasTrait(TR_GEYSERS)) drawGeysers(fb, t);   // R-307
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
        eyeY = site.surfaceHeight(player.x, player.z) + cameraOverrideAlt;
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
        double wlEye = site.hasWater || site.gen.liquidLevel > -1e8 ? site.waterAt(camX, camZ) : -1e9;
        viewUnderwater = wlEye > -1e8 && eyeY < wlEye + 0.03;
        if (viewUnderwater) env.fogDistance = std::min(env.fogDistance, 22.0);
    }
    if (std::fabs(roll) > 1e-4) camLocal = Mat3::axisAngle(Vec3(0, 0, 1), roll) * camLocal;
    // sprint FOV kick (M8-02): rebuild the sky table only when the change is noticeable
    {
        double wantF = (UW * 0.5 * FB_SCALE) / std::tan((baseFov + player.fovKick) * 0.5 * DEG);
        if (std::fabs(wantF - proj.f) > proj.f * 0.012) { proj = Proj::fromHFov(baseFov + player.fovKick); buildDirLUT(); }
    }
    lastT = t;
    {
        SurfaceLook L = lookFor(site.gen, site.sys->star);
        cloudShadows = site.atmosphere && L.cloudsLayer && env.cloudCover > 0.03 && !hasOpaqueDeck(site.gen.type);
        cloudThr = 1.0 - env.cloudCover * 0.75;
        cloudWx = windDriftX; cloudWz = windDriftZ;
    }
    auto nowMs = []() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); };
    double tSec = nowMs();
    auto section = [&](int k) { double n = nowMs(); lastRenderMs[k] = n - tSec; tSec = n; };
    setupPalette(fb);
    if (fade < 1) scalePalette(fb.pal, fade);
    fb.clearDepth();
    drawSky(fb, t, stars, sr);
    section(0);
    { auto tf0 = std::chrono::steady_clock::now(); drawFloor(fb); static const bool traceF = std::getenv("VESPERIS_TRACE") != nullptr; if (traceF && frameStamp <= 3) fprintf(stderr, "    floor %.1f ms\n", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tf0).count()); }
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
        site.lod0R = 0; site.lod0Band = 0;
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
        parallelFor(FBH, 32, [&](int yb, int ye) {
            setRasterBand(yb, ye);
            for (int k = 0; k < 5; k++) {
                Ring& g = rings[k];
                if (g.r <= 0) continue;
                bool outermost = true;
                for (int j = k + 1; j < 5; j++) if (rings[j].r > 0) outermost = false;
                drawTerrainLOD(fb, *g.c, *g.vc, g.r, g.cover, site.hasWater, t, yb, ye, outermost);
            }
            setRasterBand(0, 1 << 30);
        });
        section(2);
    }
    { static const char* skipEnv2 = std::getenv("VESPERIS_SKIP"); if (site.hasWater && !(skipEnv2 && std::strstr(skipEnv2, "refl"))) drawReflections(fb); }
    drawFootprints(fb);
    if (cameraOverrideAlt < 0 && !inVehicle()) drawBlobShadow(fb, player.x, player.z, 0.32, 1.7);
    drawWaypointLine(fb);
    drawObjects(fb, t);
    section(3);
    drawFlora(fb, t);
    section(4);
    drawRuins(fb, t);
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
    { static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr; if (trace && frameStamp <= 3) fprintf(stderr, "  render %d: palette+sky %.1f, floor+warm %.1f, terrain %.1f, refl..objects %.1f, flora %.1f, rest %.1f ms\n", frameStamp, lastRenderMs[0], lastRenderMs[1], lastRenderMs[2], lastRenderMs[3], lastRenderMs[4], lastRenderMs[5]); }
}

// the cached height under a point at `dist` from the camera: bilinear lod0 close by, the coarser rings further out
// (so the march never thrashes the fine cache), with the water level of the same cell
double SurfaceView::heightNear(double x, double z, double dist, double& water) {
    if (dist < ringR[0] * 16.0 * 0.9) { water = site.waterAt(x, z); return site.groundHeight(x, z); }
    TerrainCache& c = dist < ringR[1] * 64.0 * 0.9 ? site.lod1 : (dist < ringR[2] * 512.0 * 0.9 || !ringR[3] ? site.lod2 : site.lod3);
    const TerrainVertex& tv = c.at((int)std::lround(x / c.cellSize), (int)std::lround(z / c.cellSize));
    water = tv.water;
    return tv.h;
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
