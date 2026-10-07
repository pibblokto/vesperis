// The landing map: site selection on the body's planet map (B-004 relief shading).
#include "game.h"
#include "galaxy/roads.h"
#include <chrono>
#include "galaxy/ruins.h"
#include "galaxy/drainage.h"

namespace { const int MAP_LW = 256, MAP_LH = 128; }   // logical size of the map frame
#include "ui.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

void Game::beginLanding() {
    landBody = ship.parkedBody;
    // default site: the sub-ship point (what you are looking at from orbit)
    Mat3 frame = sys.bodyFrame(landBody, t);
    Vec3 bf = frame * ship.parkDir;
    StarSystem::latLonFromBody(bf, landLat, landLon);
    landLat = clampd(landLat, -85 * DEG, 85 * DEG);
    spaceR.mapFor(sys.bodies[landBody]);
    drainLat = 1e9; drainLon = 1e9;   // O6-03: the map's first update prefetches the drainage of the default site
    landZoom = 0;   // O2: start from the whole world
    state = GameState::LANDING_MAP;
}

bool Game::testDrainageReady() const {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return true;
    return drainageReady(BodyGen::make(sys.bodies[landBody]), StarSystem::bodyFromLatLon(landLat, landLon), 60000.0);
}

bool Game::landingZoomAllowed() const { return landBody >= 0 && landBody < (int)sys.bodies.size() && sys.bodies[landBody].radiusKm >= 100; }

void Game::updateLandingMap(const Input& in, double dt) {
    // O2: zoomed in, the arrows step a hundredth of a degree (about a kilometre), the whole-world view one degree
    double step = (landZoom ? (in.shift() ? 0.05 : 0.01) : (in.shift() ? 6.0 : 1.0)) * DEG;
    if (in.wasPressed(KEY_LEFT) || (in.isDown(KEY_LEFT) && in.shift())) landLon -= step;
    if (in.wasPressed(KEY_RIGHT) || (in.isDown(KEY_RIGHT) && in.shift())) landLon += step;
    if (in.wasPressed(KEY_UP) || (in.isDown(KEY_UP) && in.shift())) landLat += step;
    if (in.wasPressed(KEY_DOWN) || (in.isDown(KEY_DOWN) && in.shift())) landLat -= step;
    landLat = clampd(landLat, -88 * DEG, 88 * DEG);
    landLon = wrapAngle(landLon);
    // O6-03: the drainage tiles of the sector under the cursor are computed on their own thread while the site is chosen,
    // so the landing's first frame finds them in the cache (a tile takes 100-300 ms on a big world)
    if (std::fabs(landLat - drainLat) > 1e-9 || std::fabs(landLon - drainLon) > 1e-9) {
        drainLat = landLat; drainLon = landLon;
        drainagePrefetch(spaceR.genFor(sys.bodies[landBody]), StarSystem::bodyFromLatLon(landLat, landLon), 65000.0, true);
    }
    if (in.wasPressed(KEY_Z) && !in.ctrl()) {
        if (!landingZoomAllowed()) status("THE WHOLE WORLD ALREADY FITS THE MAP", 3);
        else { landZoom = !landZoom; if (landZoom) buildLandingZoom(); audio.beep = 4; }
    }
    if (in.wasPressed(KEY_R)) {
        // random lit land site
        Rng r((uint64_t)(realTime * 1000));
        const PlanetMap& m = spaceR.mapFor(sys.bodies[landBody]);
        Mat3 frame = sys.bodyFrame(landBody, t);
        Vec3 sunB = frame * normalize(sys.star.pos - sys.bodyPos(landBody, t));
        for (int k = 0; k < 200; k++) {
            double la = r.range(-70 * DEG, 70 * DEG), lo = r.range(-PI, PI);
            int mat = m.materialAt(lo, la);
            Vec3 u = StarSystem::bodyFromLatLon(la, lo);
            if (mat != MAT_WATER && dot(u, sunB) > 0.2) { landLat = la; landLon = lo; break; }
        }
    }
    if (in.wasPressed(KEY_C)) mapClouds = !mapClouds;   // the original's cloud filter
    if (in.wasPressed(KEY_J)) {   // M5-09: jump to the point where the biggest body in this world's sky stands highest
        int best = -1; double bestAng = 0;
        Vec3 bp = sys.bodyPos(landBody, t);
        for (int j = 0; j < (int)sys.bodies.size(); j++) {
            if (j == landBody || sys.bodies[j].type == PT_COMPANION) continue;
            double d = length(sys.bodyPos(j, t) - bp);
            double ang = std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1));
            if (ang > bestAng) { bestAng = ang; best = j; }
        }
        if (best >= 0) {
            Vec3 dir = sys.bodyFrame(landBody, t) * normalize(sys.bodyPos(best, t) - bp);
            StarSystem::latLonFromBody(dir, landLat, landLon);
            landLat = clampd(landLat, -85 * DEG, 85 * DEG);
            status(fmt("%s AT THE ZENITH HERE (%.1f%c WIDE)", upper(bodyNameOf(best)).c_str(), 2 * bestAng / DEG, CH_DEGREE), 3);
        } else status("NOTHING BIG IN THIS SKY", 2);
    }
    if (landZoom && (zoomImg.empty() || zoomSeed != sys.bodies[landBody].seed || std::fabs(landLat - zoomLat) > 0.75 * zoomHalfLat || std::fabs(wrapAngle(landLon - zoomLon)) > 0.75 * zoomHalfLon)) buildLandingZoom();
    else if (landZoom && zoomDry) {   // O6-03: the window was drawn without the drainage; again with it once its tiles are built
        const Body& zb = sys.bodies[landBody];
        const Vec3 zc = StarSystem::bodyFromLatLon(zoomLat, zoomLon);
        const double zr = std::max(zoomHalfLat, zoomHalfLon * std::max(std::cos(zoomLat), 0.15)) * zb.radiusKm * 1000.0 * 1.05;
        if (drainageReady(spaceR.genFor(zb), zc, zr)) buildLandingZoom();
        else if (realTime - zoomPrefetchT > 0.5) { drainagePrefetch(spaceR.genFor(zb), zc, zr, true); zoomPrefetchT = realTime; }   // the cursor's own prefetch supersedes a worker; ask again
    }
    if (in.wasPressed(KEY_ESCAPE)) state = GameState::SPACE;
    if (enterKey(in)) {
        state = GameState::DESCENT;
        transT = 0;
        fade = 0;
        {   // C-08 (KI-345): the capsule sets down outside a settlement's walls
            double la = landLat, lo = landLon;
            if (settlementClearance(BodyGen::make(sys.bodies[landBody]), la, lo)) { landLat = la; landLon = lo; status("THE CAPSULE KEEPS OUTSIDE THE RUINS", 4); }
        }
        surf.init(&sys, landBody, landLat, landLon, t);
        syncShards(); roadsMet.clear();   // C-03; C-09
        surf.cameraOverrideAlt = 1800;
        surf.cameraOverridePitch = -60 * DEG;
        surf.player.yaw = wrap2pi(landLon + 1.0);
        audio.beep = 2;
        autosave();
    }
    updateShipMotion(dt);
}


// Base colours of the landing map (bug B-004, N1-04): the body's material palette through the shared
// material ramp at the ground's noon stop of the texel's albedo, times the north-west hillshade from the
// height map and the star's light; lava emissive; optional cloud overlay. Rebuilt only when the body or
// the cloud toggle changes; the day/night term is applied per frame.
// O2: the material ramps of a world under its star's light (the same as the globe and the ground, N1-05), and one
// texel colour from them: the ground's exposure rule times a hillshade, lava hot
void Game::mapRampsFor(const Body& b, MatRamp* ramps, double& lf, bool& atmo) {
    const BodyGen& g = spaceR.genFor(b);
    RGB lightC = lerp(sys.star.color, RGB(1, 1, 1), 0.5f);
    lf = clampd(sys.lightAt(sys.bodyPos(b.index, t), t).factor, 0, 1.15);
    atmo = PLANET_TYPES[b.type].atmosphere;
    RGB skyL; bool hasSky = false;
    if (atmo) {
        RGB zen, hor;
        typeSky(g, zen, hor);   // R-307: the one pair per type
        skyL = lerp(zen, hor, 0.5f) * lightC; hasSky = true;
    }
    for (int mm = 0; mm < MAT_COUNT; mm++) ramps[mm] = materialRamp(g.matColor[mm] * lightC, hasSky ? &skyL : nullptr, 1.0);
}

uint32_t Game::mapColor(const MatRamp* ramps, int mat, double albedo, double lf, bool atmo, double shade) {
    RGB c = materialRampColor(ramps[mat], clampd(exposureStop(albedo, lf, atmo) * shade, 0, 63));
    double r = c.r * 255, gg = c.g * 255, bl = c.b * 255;
    if (mat == MAT_LAVA) { r = 255; gg = 110 + 80 * albedo; bl = 20; }
    return rgb(clampi((int)r, 0, 255), clampi((int)gg, 0, 255), clampi((int)bl, 0, 255));
}

void Game::buildLandingMapBase() {
    const Body& b = sys.bodies[landBody];
    const PlanetMap& m = spaceR.mapFor(b);
    const BodyGen& g = spaceR.genFor(b);
    const int W = PlanetMap::W, H = PlanetMap::H;
    mapBase.assign(W * H, 0);
    mapEmissive.assign(W * H, 0);
    RGB lightC = lerp(sys.star.color, RGB(1, 1, 1), 0.5f);
    MatRamp ramps[MAT_COUNT];
    double lf; bool atmo;
    mapRampsFor(b, ramps, lf, atmo);
    // hillshade: gradients scaled by their RMS so every body gets the same relief strength
    auto hAt = [&](int x, int y) { x = (x + W) % W; y = clampi(y, 0, H - 1); return (double)m.height[y * W + x]; };
    double sum2 = 0; int cnt = 0;
    for (int y = 0; y < H; y += 2)
        for (int x = 0; x < W; x += 2) { double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y + 1) - hAt(x, y - 1); sum2 += gx * gx + gy * gy; cnt++; }
    double k = 0.5 / (std::sqrt(sum2 / std::max(1, cnt)) + 1e-9);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int i = y * W + x;
            int mat = m.material[i];
            double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y + 1) - hAt(x, y - 1);
            double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);   // light from the north-west
            uint32_t base = mapColor(ramps, mat, m.albedo[i] / 255.0, lf, atmo, shade);
            double r = base & 255, gg = (base >> 8) & 255, bl = (base >> 16) & 255;
            if (mat == MAT_LAVA) mapEmissive[i] = 1;
            if (mapClouds && atmo) {
                if (hasOpaqueDeck(b.type)) {
                    // an opaque cloud deck: the pattern only modulates its brightness; C shows the ground
                    double cl = 0.55 + 0.45 * (m.cloud[i] / 255.0);
                    RGB deck = g.matColor[MAT_CLOUD] * lightC;
                    r = 255 * deck.r * cl; gg = 255 * deck.g * cl; bl = 255 * deck.b * cl;
                    mapEmissive[i] = 0;
                } else {
                    double cl = m.cloud[i] / 255.0 * 0.55;
                    r += (245 - r) * cl; gg += (245 - gg) * cl; bl += (250 - bl) * cl;
                }
            }
            mapBase[i] = rgb(clampi((int)r, 0, 255), clampi((int)gg, 0, 255), clampi((int)bl, 0, 255));
        }
    mapBaseSeed = b.seed;
    mapBaseClouds = mapClouds;
}

// O2 (R-301): the zoom window: one sector tall (a degree of latitude), two sectors' worth of ground wide (the frame is
// 2:1), sampled from the planet function at the far ring's detail (64-512 m), coloured like the whole-world map. The
// hillshade is normalised over the window, capped so a plain does not turn into a mountain range.
void Game::buildLandingZoom() {
    const Body& b = sys.bodies[landBody];
    const BodyGen& g = spaceR.genFor(b);
    zoomLat = landLat; zoomLon = landLon; zoomSeed = b.seed;
    zoomHalfLat = 0.5 * DEG;
    zoomHalfLon = zoomHalfLat * 2.0 / std::max(std::cos(zoomLat), 0.15);
    double texelM = 2 * zoomHalfLat * b.radiusKm * 1000.0 / ZH;
    double detail = clampd(0.6 * texelM, 64, 512);
    // O6-03: the window reads the drainage (rivers as bands, lakes) from tiles already built; a window whose tiles are
    // not built yet is drawn without the drainage at once, its tiles are asked for on the worker, and the window is
    // drawn again when they are there (the zoom itself must not compute tiles: nine of them took two seconds)
    const Vec3 zoomCentre = StarSystem::bodyFromLatLon(zoomLat, zoomLon);
    const double zoomRadiusM = std::max(zoomHalfLat, zoomHalfLon * std::max(std::cos(zoomLat), 0.15)) * b.radiusKm * 1000.0 * 1.05;
    zoomDry = !drainageReady(g, zoomCentre, zoomRadiusM);
    if (zoomDry) { drainagePrefetch(g, zoomCentre, zoomRadiusM, true); zoomPrefetchT = realTime; }
    const bool dry = zoomDry;
    zoomImg.assign(ZW * ZH, 0); zoomEmissive.assign(ZW * ZH, 0); zoomUnit.assign(ZW * ZH, Vec3());
    std::vector<float> hs(ZW * ZH), albs(ZW * ZH); std::vector<uint8_t> mats(ZW * ZH);
    parallelFor(ZH, 8, [&](int y0, int y1) {
        DrainageOff noDrain(dry);   // per worker thread
        for (int y = y0; y < y1; y++)
            for (int x = 0; x < ZW; x++) {
                double lat = clampd(zoomLat + (0.5 - (y + 0.5) / ZH) * 2 * zoomHalfLat, -PI * 0.5, PI * 0.5);
                double lon = zoomLon + ((x + 0.5) / ZW - 0.5) * 2 * zoomHalfLon;
                Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
                SurfaceSample s = sampleSurface(g, u, detail);
                int i = y * ZW + x;
                zoomUnit[i] = u; hs[i] = (float)s.height; mats[i] = (uint8_t)s.material; albs[i] = (float)s.albedo;
            }
    });
    MatRamp ramps[MAT_COUNT]; double lf; bool atmo;
    mapRampsFor(b, ramps, lf, atmo);
    auto hAt = [&](int x, int y) { x = clampi(x, 0, ZW - 1); y = clampi(y, 0, ZH - 1); return (double)hs[y * ZW + x]; };
    double sum2 = 0; int cnt = 0;
    for (int y = 0; y < ZH; y += 2)
        for (int x = 0; x < ZW; x += 2) { double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y + 1) - hAt(x, y - 1); sum2 += gx * gx + gy * gy; cnt++; }
    double k = std::min(0.5 / (std::sqrt(sum2 / std::max(1, cnt)) + 1e-9), 0.5 / (0.03 * 2 * texelM));   // a 3% grade at most for full shading
    for (int y = 0; y < ZH; y++)
        for (int x = 0; x < ZW; x++) {
            int i = y * ZW + x;
            double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y + 1) - hAt(x, y - 1);
            double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);
            zoomImg[i] = mapColor(ramps, mats[i], albs[i], lf, atmo, shade);
            zoomEmissive[i] = mats[i] == MAT_LAVA;
        }
    zoomRoads.clear(); zoomRoadsMs = 0;   // C-09: the old roads of the window (`galaxy/roads.*`), the same network the ground carries, as texel polylines thinned to a texel's step
    if (worldHadCivilisation(g)) {
        auto tr0 = std::chrono::steady_clock::now();
        std::vector<Road> rs; roadsNear(g, zoomCentre, zoomRadiusM, rs, true);
        zoomRoadsMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tr0).count();
        for (const Road& r : rs) {
            if (std::max(settlementRank(r.a.sclass), settlementRank(r.b.sclass)) < 2) continue;   // the roads of the villages and the towns: a lane between two hamlets is not seen from orbit, and the whole web was a honeycomb over the land
            std::vector<std::pair<float, float>> poly; float lx = 0, ly = 0;
            for (size_t k = 0; k < r.pts.size(); k++) {
                double la, lo; StarSystem::latLonFromBody(r.pts[k], la, lo);
                float x = (float)((wrapAngle(lo - zoomLon) / (2 * zoomHalfLon) + 0.5) * ZW), y = (float)((0.5 - (la - zoomLat) / (2 * zoomHalfLat)) * ZH);
                if (poly.empty() || k + 1 == r.pts.size() || std::hypot(x - lx, y - ly) >= 0.7f) { poly.push_back({x, y}); lx = x; ly = y; }
            }
            if (poly.size() >= 2) zoomRoads.push_back(poly);
        }
    }
}

// O0-04 (B-306): the shadow on the day side of the landing map: moons and the parent between a texel and the star (as
// the globe draws them, M1-08) and the world's own rings (the profile's density); 0..255 per texel
void Game::buildShadowMap(const std::vector<Vec3>& units, std::vector<uint8_t>& out) {
    const Body& b = sys.bodies[landBody];
    Vec3 bp = sys.bodyPos(landBody, t);
    Vec3 starDir = normalize(sys.star.pos - bp);
    Mat3 frameT = sys.bodyFrame(landBody, t).transposed();
    struct Occ { Vec3 pos; double r; };
    std::vector<Occ> occ;
    for (int j = 0; j < (int)sys.bodies.size(); j++) {
        if (j == landBody) continue;
        if (sys.bodies[j].parent == landBody || j == b.parent) occ.push_back({sys.bodyPos(j, t), sys.bodies[j].radiusKm});
    }
    const std::vector<float>* prof = b.rings ? &spaceR.ringProfile(sys, landBody) : nullptr;
    double R = b.radiusKm, R0 = b.ringInner * R, R1 = b.ringOuter * R;
    out.assign(units.size(), 0);
    if (occ.empty() && !prof) return;
    parallelFor((int)units.size(), 8192, [&](int i0, int i1) {
        for (int i = i0; i < i1; i++) {
            Vec3 nW = frameT * units[i];
            if (dot(nW, starDir) <= 0) continue;   // the night side needs no shadow
            Vec3 surfW = bp + nW * R;
            double sh = 0;
            for (const Occ& o : occ) {
                Vec3 rel = o.pos - surfW;
                double along = dot(rel, starDir);
                if (along <= 0) continue;
                double dp = std::sqrt(std::max(0.0, length2(rel) - along * along));
                if (dp < o.r) sh = std::max(sh, clampd((o.r - dp) / (o.r * 0.12), 0, 1));
            }
            if (prof) {
                double denom = dot(starDir, b.spinAxis);
                if (std::fabs(denom) > 1e-6) {
                    double tr = dot(bp - surfW, b.spinAxis) / denom;
                    if (tr > 0) {
                        double rr = length(surfW + starDir * tr - bp);
                        if (rr >= R0 && rr <= R1) sh = std::max(sh, 0.75 * (*prof)[clampi((int)((rr - R0) / (R1 - R0) * 255), 0, 255)]);
                    }
                }
            }
            out[i] = (uint8_t)(sh * 255 + 0.5);
        }
    });
}

// rebuilt for another body, every 20 s of game time and at most twice a second of real time (a moon's shadow crawls)
void Game::refreshMapShadows(const std::vector<Vec3>& unitLUT) {
    if (mapShadowBody == landBody && std::fabs(t - mapShadowT) < 20 && realTime - mapShadowReal < 0.5 && mapShadow.size() == unitLUT.size() && (!landZoom || zoomShadow.size() == zoomUnit.size())) return;
    mapShadowBody = landBody; mapShadowT = t; mapShadowReal = realTime;
    buildShadowMap(unitLUT, mapShadow);
    if (landZoom && !zoomUnit.empty()) buildShadowMap(zoomUnit, zoomShadow); else zoomShadow.clear();
}

double Game::testMapShadowMax() {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return 0;
    std::vector<Vec3> units(PlanetMap::W * PlanetMap::H);
    for (int y = 0; y < PlanetMap::H; y++)
        for (int x = 0; x < PlanetMap::W; x++) units[y * PlanetMap::W + x] = StarSystem::bodyFromLatLon((0.5 - (y + 0.5) / PlanetMap::H) * PI, ((x + 0.5) / PlanetMap::W) * TAU - PI);
    std::vector<uint8_t> sh;
    buildShadowMap(units, sh);
    int mx = 0; for (uint8_t v : sh) mx = std::max(mx, (int)v);
    return mx / 255.0;
}

// N1-06: the relief class at a site from the terrain itself: the steepest rise over 150 m and over 600 m
double Game::siteSlope(const SurfaceSite& probe) {
    // O6-02: the grade of the region: the mean of eight directional slopes (150 and 600 m in four directions at the
    // map's 64 m detail), scaled by 1 / 0.64 to the size of the gradient (a directional slope averages 2 / pi of it).
    // The steepest probe decided before, so one crack or crater wall made a flat site "MOUNTAINS"; the mean of the
    // region ignores features smaller than a few hundred metres, which is what the class is for.
    double h0 = probe.sampleAt(0, 0, 64).h, region = 0;
    for (int k = 0; k < 4; k++) {
        double dx = (k & 1) ? 0 : (k ? -1 : 1), dz = (k & 1) ? (k == 1 ? 1 : -1) : 0;
        region += std::fabs(probe.sampleAt(dx * 150, dz * 150, 64).h - h0) / 150.0 / 8;
        region += std::fabs(probe.sampleAt(dx * 600, dz * 600, 64).h - h0) / 600.0 / 8;
    }
    return region / 0.64;
}

// N0-02: the landing map's base colour at a texel, for the consistency diagnosis
uint32_t Game::testLandingMapTexel(int tx, int ty, bool clouds) {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return 0;
    mapClouds = clouds;
    const Body& b = sys.bodies[landBody];
    if (mapBaseSeed != b.seed || mapBaseClouds != mapClouds || mapBase.empty()) buildLandingMapBase();
    tx = ((tx % PlanetMap::W) + PlanetMap::W) % PlanetMap::W;
    ty = clampi(ty, 0, PlanetMap::H - 1);
    return mapBase[ty * PlanetMap::W + tx];
}

double Game::testFrontBandAt(double latDeg, double lonDeg) {   // W-03
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return 0;
    const SpaceRenderer::FrontMap* fm = spaceR.frontsFor(sys.bodies[landBody], t);
    return fm ? frontMapAt(fm->cloud, PlanetMap::W, PlanetMap::H, lonDeg * DEG, latDeg * DEG) : 0.0;
}

// N1-06: the relief class the landing map would print at a site
const char* Game::testReliefClass(double lat, double lon) {
    probeSite.init(&sys, landBody, lat, lon, t);
    probeBody = landBody; probeLat = lat; probeLon = lon;
    return reliefClass(siteSlope(probeSite));
}

void Game::renderLandingMap() {
    // background: keep the space view, dimmed
    renderSpace();
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 150);
    const Body& b = sys.bodies[landBody];
    const PlanetMap& m = spaceR.mapFor(b);
    Mat3 frame = sys.bodyFrame(landBody, t);
    Vec3 sunB = frame * normalize(sys.star.pos - sys.bodyPos(landBody, t));
    int ox = 32, oy = 12;
    const int S = FB_SCALE;
    if (mapBaseSeed != b.seed || mapBaseClouds != mapClouds || mapBase.empty()) buildLandingMapBase();
    static std::vector<Vec3> unitLUT;   // body-frame unit vector per texel
    if (unitLUT.empty()) {
        unitLUT.resize(PlanetMap::W * PlanetMap::H);
        for (int y = 0; y < PlanetMap::H; y++)
            for (int x = 0; x < PlanetMap::W; x++)
                unitLUT[y * PlanetMap::W + x] = StarSystem::bodyFromLatLon((0.5 - (y + 0.5) / PlanetMap::H) * PI, ((x + 0.5) / PlanetMap::W) * TAU - PI);
    }
    refreshMapShadows(unitLUT);   // O0-04: the moons', the parent's and the rings' shadows on the day side
    const int PW = MAP_LW * S, PH = MAP_LH * S;
    // W-03: the fronts' bands (the same overlay the globe draws), under the cloud filter like the pattern's cloud
    const SpaceRenderer::FrontMap* fmc = mapClouds && PLANET_TYPES[b.type].atmosphere && !hasOpaqueDeck(b.type) ? spaceR.frontsFor(b, t) : nullptr;
    const std::vector<uint8_t>* fm = fmc ? &fmc->cloud : nullptr;
    if (fm && landZoom && !zoomUnit.empty() && (zoomFront.size() != zoomUnit.size() || std::fabs(t - zoomFrontT) > 240 || zoomFrontLat != zoomLat || zoomFrontLon != zoomLon)) {
        zoomFront.resize(zoomUnit.size()); zoomFrontT = t; zoomFrontLat = zoomLat; zoomFrontLon = zoomLon;
        for (size_t i = 0; i < zoomUnit.size(); i++) { double la, lo; StarSystem::latLonFromBody(zoomUnit[i], la, lo); zoomFront[i] = (uint8_t)(frontMapAt(*fm, PlanetMap::W, PlanetMap::H, lo, la) * 255 + 0.5); }
    }
    auto withFront = [&](uint32_t c, double fc) {   // the band whitens the texel as the cloud filter does
        double r = c & 255, gg = (c >> 8) & 255, bl = (c >> 16) & 255;
        fc *= 0.6; r += (245 - r) * fc; gg += (245 - gg) * fc; bl += (250 - bl) * fc;
        return rgb((int)r, (int)gg, (int)bl);
    };
    if (landZoom && !zoomImg.empty()) {
        // O2: the zoom window, one texel per logical pixel, with the day side lit as on the whole-world map
        for (int py = 0; py < PH; py++) {
            int ty = py / S;
            uint32_t* row = canvas.px + (size_t)(oy * S + py) * FBW + ox * S;
            for (int px = 0; px < PW; px++) {
                int i = ty * ZW + px / S;
                double lit = dot(zoomUnit[i], sunB);
                double l = 0.22 + 0.78 * smoothstep(-0.05, 0.15, lit) * (zoomShadow.size() == zoomUnit.size() ? 1 - 0.9 * zoomShadow[i] / 255.0 : 1.0);
                if (zoomEmissive[i]) l = std::max(l, 0.95);
                uint32_t c = zoomImg[i];
                if (fm && zoomFront.size() == zoomUnit.size() && zoomFront[i]) c = withFront(c, zoomFront[i] / 255.0);
                row[px] = rgb((int)((c & 255) * l), (int)(((c >> 8) & 255) * l), (int)(((c >> 16) & 255) * l));
            }
        }
        for (const auto& poly : zoomRoads) {   // C-09: the old roads as faint lines, the ground's own colour darkened under the texel's light
            int tx = clampi((int)poly[0].first, 0, ZW - 1), ty = clampi((int)poly[0].second, 0, ZH - 1), i = ty * ZW + tx;
            double l = 0.22 + 0.78 * smoothstep(-0.05, 0.15, dot(zoomUnit[i], sunB)) * (zoomShadow.size() == zoomUnit.size() ? 1 - 0.9 * zoomShadow[i] / 255.0 : 1.0);
            uint32_t c = zoomImg[i]; l *= 0.7;
            uint32_t col = rgb((int)((c & 255) * l), (int)(((c >> 8) & 255) * l), (int)(((c >> 16) & 255) * l));
            // a hairline in the frame's own pixels (a logical line would be a block S wide: the grid's weight, not a faint road)
            const int X0 = ox * S, Y0 = oy * S, X1 = (ox + MAP_LW) * S, Y1 = (oy + MAP_LH) * S;
            for (size_t k = 0; k + 1 < poly.size(); k++) {
                int x0 = (int)((ox + poly[k].first) * S), y0 = (int)((oy + poly[k].second) * S), x1 = (int)((ox + poly[k + 1].first) * S), y1 = (int)((oy + poly[k + 1].second) * S);
                if (x0 < X0 || x1 < X0 || x0 >= X1 || x1 >= X1 || y0 < Y0 || y1 < Y0 || y0 >= Y1 || y1 >= Y1) continue;
                int steps = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
                for (int q = 0; q <= steps; q++) { int x = x0 + (steps ? (x1 - x0) * q / steps : 0), y = y0 + (steps ? (y1 - y0) * q / steps : 0); canvas.px[(size_t)y * FBW + x] = col; }
            }
        }
    } else {
        // the 512x256 map fills a 256x128 logical frame: one texel per pixel at 2x, every second texel at 1x
        for (int py = 0; py < PH; py++) {
            int ty = py * PlanetMap::H / PH;
            uint32_t* row = canvas.px + (size_t)(oy * S + py) * FBW + ox * S;
            for (int px = 0; px < PW; px++) {
                int tx = px * PlanetMap::W / PW;
                int i = ty * PlanetMap::W + tx;
                double lit = dot(unitLUT[i], sunB);
                double l = 0.22 + 0.78 * smoothstep(-0.05, 0.15, lit) * (mapShadow.size() == unitLUT.size() ? 1 - 0.9 * mapShadow[i] / 255.0 : 1.0);
                if (mapEmissive[i]) l = std::max(l, 0.95);
                uint32_t c = mapBase[i];
                if (fm && (*fm)[i]) c = withFront(c, (*fm)[i] / 255.0);
                row[px] = rgb((int)((c & 255) * l), (int)(((c >> 8) & 255) * l), (int)(((c >> 16) & 255) * l));
            }
        }
    }
    (void)m;
    drawRectRGB(canvas, ox - 1, oy - 1, ox + MAP_LW, oy + MAP_LH, HUD_DIM);
    int cx, cy;
    if (landZoom && !zoomImg.empty()) {
        // O2: the sector grid (one-degree lines) with the sectors' names, a scale bar, and the cursor within the window
        const uint32_t gridC = rgb(80, 80, 50);
        int k0 = (int)std::ceil((zoomLon - zoomHalfLon) / DEG), k1 = (int)std::floor((zoomLon + zoomHalfLon) / DEG);
        for (int k = k0; k <= k1; k++) { int x = ox + (int)(((k * DEG - zoomLon) / (2 * zoomHalfLon) + 0.5) * MAP_LW); if (x > ox && x < ox + MAP_LW - 1) drawLineRGB(canvas, x, oy, x, oy + MAP_LH - 1, gridC); }
        int j0 = (int)std::ceil((zoomLat - zoomHalfLat) / DEG), j1 = (int)std::floor((zoomLat + zoomHalfLat) / DEG);
        for (int j = j0; j <= j1; j++) { int y = oy + (int)((0.5 - (j * DEG - zoomLat) / (2 * zoomHalfLat)) * MAP_LH); if (y > oy && y < oy + MAP_LH - 1) drawLineRGB(canvas, ox, y, ox + MAP_LW - 1, y, gridC); }
        int cellW = (int)(DEG / (2 * zoomHalfLon) * MAP_LW), cellH = (int)(DEG / (2 * zoomHalfLat) * MAP_LH);
        if (cellW >= 44 && cellH >= 10)
            for (int k = k0 - 1; k <= k1; k++)
                for (int j = j0 - 1; j <= j1; j++) {
                    double clat = (j + 0.5) * DEG, clon = (k + 0.5) * DEG;
                    int lx = ox + (int)(((clon - zoomLon) / (2 * zoomHalfLon) + 0.5) * MAP_LW) - 21, ly = oy + (int)((0.5 - (clat - zoomLat) / (2 * zoomHalfLat)) * MAP_LH) - 3;
                    if (lx < ox + 1 || ly < oy + 1 || lx + 42 > ox + MAP_LW || ly + 7 > oy + MAP_LH) continue;
                    drawText(canvas, lx, ly, sectorName(clat / DEG, wrapAngle(clon) / DEG).c_str(), HUD_DIM);
                }
        {   // scale bar: the window is 2 halfLon of longitude, i.e. this many km across
            double widthKm = 2 * zoomHalfLon * std::cos(zoomLat) * b.radiusKm;
            double barKm = widthKm > 120 ? 50 : (widthKm > 40 ? 20 : (widthKm > 12 ? 5 : 1));
            int barPx = (int)(barKm / widthKm * MAP_LW);
            int bx = ox + MAP_LW - 6 - barPx, by = oy + MAP_LH - 6;
            drawLineRGB(canvas, bx, by, bx + barPx, by, HUD_WHITE); drawLineRGB(canvas, bx, by - 2, bx, by + 2, HUD_WHITE); drawLineRGB(canvas, bx + barPx, by - 2, bx + barPx, by + 2, HUD_WHITE);
            std::string lab = fmt("%.0f KM", barKm);
            drawText(canvas, bx + barPx - textWidth(lab.c_str()), by - 10, lab.c_str(), HUD_WHITE);
        }
        int named = 0;
        struct LabelBox { int x0, y0, x1, y1; }; std::vector<LabelBox> labels;   // no name over another's, nor over the map's own text
        const std::string bkey = worldKeyOf(landBody);
        for (const SurfaceMark& m : guide.marks) {   // R-408: the explorer's marks of this world (a glyph each, a friend's in cyan), eight names at most
            if (m.body != bkey) continue;
            int lx = ox + (int)((wrapAngle(m.lon - zoomLon) / (2 * zoomHalfLon) + 0.5) * MAP_LW), ly = oy + (int)((0.5 - (m.lat - zoomLat) / (2 * zoomHalfLat)) * MAP_LH);
            if (lx < ox + 1 || ly < oy + 1 || lx >= ox + MAP_LW - 1 || ly >= oy + MAP_LH - 1) continue;
            const uint32_t lc = m.lent ? HUD_CYAN : HUD_WHITE;
            if (m.kind < LM_COUNT) drawText(canvas, lx - 2, ly - 3, LANDMARK_KIND_SYMBOLS[m.kind], lc);
            else { drawLineRGB(canvas, lx - 2, ly, lx, ly - 2, lc); drawLineRGB(canvas, lx, ly - 2, lx + 2, ly, lc); drawLineRGB(canvas, lx + 2, ly, lx, ly + 2, lc); drawLineRGB(canvas, lx, ly + 2, lx - 2, ly, lc); }
            if (!m.name.empty() && named < 8 && lx + 60 < ox + MAP_LW && ly - 3 > oy + 8 && ly + 5 < oy + MAP_LH - 8) {
                std::string nm = trunc(upper(m.name), 14);
                LabelBox bx{lx + 5, ly - 3, lx + 5 + textWidth(nm.c_str()), ly + 4};
                bool clear = true;
                for (const LabelBox& o2 : labels) if (bx.x0 < o2.x1 + 2 && bx.x1 + 2 > o2.x0 && bx.y0 < o2.y1 + 1 && bx.y1 + 1 > o2.y0) { clear = false; break; }
                if (clear) { drawText(canvas, bx.x0, bx.y0, nm.c_str(), lc); labels.push_back(bx); named++; }
            }
        }
        cx = ox + (int)((wrapAngle(landLon - zoomLon) / (2 * zoomHalfLon) + 0.5) * MAP_LW);
        cy = oy + (int)((0.5 - (landLat - zoomLat) / (2 * zoomHalfLat)) * MAP_LH);
    } else {
        cx = ox + (int)((wrap2pi(landLon + PI) / TAU) * MAP_LW); cy = oy + (int)((0.5 - landLat / PI) * MAP_LH);
        const std::string bkey = worldKeyOf(landBody);
        for (const SurfaceMark& m : guide.marks) {   // R-408: the explorer's marks of this world, a dot each (Z zooms in on them)
            if (m.body != bkey) continue;
            int mx = ox + (int)((wrap2pi(m.lon + PI) / TAU) * MAP_LW), my = oy + (int)((0.5 - m.lat / PI) * MAP_LH);
            fillRectRGB(canvas, mx - 1, my - 1, mx + 1, my + 1, rgb(0, 0, 0)); fillRectRGB(canvas, mx, my, mx, my, m.lent ? HUD_CYAN : HUD_WHITE);
        }
    }
    cx = clampi(cx, ox, ox + MAP_LW - 1); cy = clampi(cy, oy, oy + MAP_LH - 1);
    drawLineRGB(canvas, ox, cy, ox + MAP_LW - 1, cy, rgb(255, 255, 255));
    drawLineRGB(canvas, cx, oy, cx, oy + MAP_LH - 1, rgb(255, 255, 255));
    drawRectRGB(canvas, cx - 3, cy - 3, cx + 3, cy + 3, HUD_AMBER);
    // info
    if (probeBody != landBody || probeLat != landLat || probeLon != landLon) {
        probeSite.init(&sys, landBody, landLat, landLon, t);
        probeBody = landBody; probeLat = landLat; probeLon = landLon;
    }
    const SurfaceSite& probe = probeSite;
    SunInfo si = probe.sun(t);
    // O6-03: the probe reads the drainage only once the site's tiles are built (the cursor's prefetch); a probe that
    // built them froze the map for half a second at every move of the cursor
    DrainageOff probeDry(!drainageReady(probe.gen, probe.up0, 1500.0));
    TerrainVertex tvp = probe.sampleAt(0, 0, 64);
    int mat = tvp.material;
    double hgt = tvp.h;
    int ry = oy + MAP_LH + 5;
    std::string l1 = fmt("%s  %.2f%c%s %.2f%c%s  SECTOR %s", trunc(upper(bodyNameOf(landBody)), 18).c_str(), std::fabs(landLat / DEG), CH_DEGREE,
                         landLat >= 0 ? "N" : "S", std::fabs(landLon / DEG), CH_DEGREE, landLon >= 0 ? "E" : "W", sectorName(landLat / DEG, landLon / DEG).c_str());
    drawText(canvas, 8, ry, l1.c_str(), HUD_GREEN);
    const char* phase = si.altitude > 0.05 ? "DAY" : (si.altitude > -0.1 ? "TWILIGHT" : "NIGHT");
    // N1-04/06: what you will see: material, elevation, relief class, and the ground colour at noon as a swatch
    std::string l2 = fmt("%s %+.0fM  %s", MATERIAL_NAMES[mat], hgt, reliefClass(siteSlope(probe)));
    drawText(canvas, 8, ry + 8, l2.c_str(), si.altitude > 0.05 ? HUD_WHITE : HUD_DIM);
    {
        const BodyGen& g = spaceR.genFor(b);
        RGB gc = materialRampColor(g.matColor[mat] * lerp(sys.star.color, RGB(1, 1, 1), 0.5f), exposureStop(tvp.albedo, clampd(si.lightFactor, 0, 1.15), PLANET_TYPES[b.type].atmosphere));
        int sx0 = 8 + textWidth(l2.c_str()) + 8;
        drawText(canvas, sx0, ry + 8, "GROUND", HUD_DIM);
        int bx = sx0 + textWidth("GROUND") + 4;
        blendRectRGB(canvas, bx, ry + 7, bx + 23, ry + 14, rgb((int)(gc.r * 255), (int)(gc.g * 255), (int)(gc.b * 255)), 255);
        drawRectRGB(canvas, bx - 1, ry + 6, bx + 24, ry + 15, HUD_DIM);
    }
    std::string sn = tvp.biome ? seasonName(probe.season, landLat, b.axialTilt) : "";
    std::string l3 = fmt("%s%s%s%sSUN %+.0f%c AZ %03.0f%c %s", tvp.biome ? BIOME_NAMES[tvp.biome] : "", (tvp.biome && !sn.empty()) ? ", " : "", sn.c_str(), tvp.biome ? "  " : "",
                         si.altitude / DEG, CH_DEGREE, si.azimuth / DEG, CH_DEGREE, phase);
    if (b.type == PT_FELISIAN && tvp.biome) {   // N3-04: the species most likely met here
        Bestiary B = Bestiary::make(b, spaceR.genFor(b));
        int spi = B.pickLand(hash2i((int64_t)(landLat * 100), (int64_t)(landLon * 100), b.seed), tvp.biome);
        if (spi >= 0 && tvp.biome != BIO_ICE && tvp.biome != BIO_OCEAN) l3 += fmt("  LIFE: %s", trunc(upper(B.species[spi].name), 10).c_str());
    }
    drawText(canvas, 8, ry + 16, l3.c_str(), si.altitude > 0.05 ? HUD_WHITE : HUD_DIM);
    double lockRatio = std::fabs(1.0 - b.rotPeriod / b.orbitPeriod);
    std::string dayLen = (b.parent < 0 && lockRatio < 0.02) ? "TIDALLY LOCKED" : fmt("DAY %.1f H", std::fabs(b.rotPeriod) / 3600.0 * (b.parent < 0 ? 1.0 / lockRatio : 1.0));
    std::string grav = b.gravity / 9.8 < 0.01 ? fmt("GRAV %.4f G", b.gravity / 9.8) : fmt("GRAV %.2f G", b.gravity / 9.8);   // O4: a comet's microgravity
    std::string l4 = fmt("LOCAL %02d:%02d  %s  %s  %s", (int)(si.dayFraction * 24), (int)(std::fmod(si.dayFraction * 24, 1.0) * 60),
                         dayLen.c_str(), grav.c_str(), PLANET_TYPES[b.type].atmosphere ? "ATMOSPHERE" : "NO ATMOSPHERE");
    drawText(canvas, 8, ry + 24, l4.c_str(), HUD_DIM);
    {   // M5-09 (R-001): what hangs in the sky over this sector right now
        Mat3 L = probe.localFrame(t);
        Vec3 sitePos = probe.worldPos(t, 0, 0, 0.002);
        int best = -1; double bestAng = 0, bestAlt = 0, bestPhase = 0; int points = 0;
        for (int j = 0; j < (int)sys.bodies.size(); j++) {
            if (j == landBody) continue;
            Vec3 bp = sys.bodyPos(j, t);
            Vec3 rel = bp - sitePos;
            double d = length(rel);
            Vec3 dl = L * (rel / d);
            if (dl.y < 0.0) continue;
            double ang = std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1));
            if (ang < 0.25 * DEG) { points++; continue; }
            if (ang > bestAng) { bestAng = ang; best = j; bestAlt = std::asin(dl.y); bestPhase = 0.5 * (1 + dot(normalize(sys.star.pos - bp), normalize(sitePos - bp))); }
        }
        std::string l5;
        // O0-01: the world's own rings: how high their arc stands in this site's sky
        std::string ringNote;
        if (b.rings) {
            double bestEl = -90;
            double mid = 0.5 * (b.ringInner + b.ringOuter) * b.radiusKm;
            for (int a = 0; a < 72; a++) {
                double an = a * TAU / 72;
                Vec3 d = normalize(Vec3(std::cos(an) * mid, std::sin(an) * mid, 0) - probe.up0 * b.radiusKm);
                bestEl = std::max(bestEl, std::asin(clampd(dot(d, probe.up0), -1, 1)) / DEG);
            }
            ringNote = std::fabs(landLat) < 1.5 * DEG ? "RINGS EDGE-ON OVERHEAD" : (bestEl > 3 ? fmt("RINGS %.0f%c UP", bestEl, CH_DEGREE) : "RINGS BELOW THE HORIZON");
        }
        bool bestRinged = best >= 0 && sys.bodies[best].rings;
        if (best >= 0) {
            const char* ph = bestPhase > 0.85 ? "FULL" : (bestPhase > 0.6 ? "GIBBOUS" : (bestPhase > 0.4 ? "HALF LIT" : (bestPhase > 0.15 ? "CRESCENT" : "NEW")));
            l5 = fmt("SKY: %s%s %.0f%c UP, %.1f%c WIDE, %s%s", ringNote.empty() ? "" : (ringNote + "; ").c_str(), trunc(upper(bodyNameOf(best)), ringNote.empty() ? 14 : 10).c_str(), bestAlt / DEG, CH_DEGREE, 2 * bestAng / DEG, CH_DEGREE,
                     bestRinged ? "RINGED" : ph, (points && ringNote.empty() && !bestRinged) ? fmt(" +%d PTS", points).c_str() : "");
        } else l5 = (ringNote.empty() ? std::string("SKY: ") : "SKY: " + ringNote + "; ") + (points ? fmt("%d WORLDS AS POINTS OF LIGHT", points) : std::string("NOTHING BUT STARS"));
        drawText(canvas, 8, ry + 32, l5.c_str(), (best >= 0 || !ringNote.empty()) ? HUD_CYAN : HUD_DIM);
    }
    if (landZoom) drawTextCentered(canvas, UW / 2, UH - 12, "ARROWS 1 KM (SHIFT 5)  Z WORLD MAP  R RANDOM  ENTER LAND  ESC", HUD_AMBER);
    else drawTextCentered(canvas, UW / 2, UH - 12, PLANET_TYPES[b.type].atmosphere ? "ARROWS  Z SECTOR ZOOM  R RANDOM  C CLOUDS  J SKY  ENTER LAND  ESC" : "ARROWS  Z SECTOR ZOOM  R RANDOM  J SKY  ENTER LAND  ESC", HUD_AMBER);
    drawTextCentered(canvas, UW / 2, 2, landZoom ? fmt("SURFACE CAPSULE - SECTOR %s - PICK THE SITE", sectorName(landLat / DEG, landLon / DEG).c_str()).c_str() : "SURFACE CAPSULE - SELECT LANDING SECTOR", HUD_CYAN);
}
