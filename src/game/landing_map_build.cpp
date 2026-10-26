// The landing map's pictures built (B-004's relief): the planet map's colours, the base map, the zoom's tiles, the shadows
#include "game.h"
#include "core/parallel.h"
#include "peoples/roads.h"
#include "peoples/plan.h"
#include "peoples/buildings.h"
#include <chrono>
#include "peoples/ruins.h"
#include "galaxy/drainage.h"
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>


// the one colouring of every map of a world (the landing map, its zoom, the sector map): the body's material ramps
// under the star's light, the ground's exposure rule and a hillshade
// Base colours of the landing map (bug B-004, N1-04): the body's material palette through the shared
// material ramp at the ground's noon stop of the texel's albedo, times the north-west hillshade from the
// height map and the star's light; lava emissive; optional cloud overlay. Rebuilt only when the body or
// the cloud toggle changes; the day/night term is applied per frame.
// the material ramps of a world under its star's light (the same as the globe and the ground, N1-05), and one
// texel colour from them: the ground's exposure rule times a hillshade, lava hot
void Game::mapRampsFor(const Body& b, MatRamp* ramps, double& lf, bool& atmo) {
    const BodyGen& g = spaceR.genFor(b);
    RGB lightC = lerp(sys.star.color, RGB(1, 1, 1), 0.5f);
    lf = sys.sunless() ? 1.0 : clampd(sys.lightAt(sys.bodyPos(b.index, t), t).factor, 0, 1.15);   // the survey's colours at the noon stop
    atmo = bodyHasAir(b);
    RGB skyL; bool hasSky = false;
    if (atmo) {
        RGB zen, hor;
        typeSky(g, zen, hor);   // the one pair per type
        skyL = lerp(zen, hor, 0.5f) * lightC; hasSky = true;
    }
    for (int mm = 0; mm < MAT_COUNT; mm++) ramps[mm] = materialRamp(g.matColor[mm] * lightC, hasSky ? &skyL : nullptr, 1.0);
}

// D-02: `survey` 0..1, a sunless world's survey look at that height
// the survey look of a world with no sun, the ship's altimeter and not its camera (the maps went black at a light of 0): the ground's
// colour at the noon stop, two parts in three the radar camera's cold phosphor, its brightness by the height (the map's lows dark, its highs
// pale) times the relief's hillshade; lava keeps its glow
uint32_t Game::mapColor(const MatRamp* ramps, int mat, double albedo, double lf, bool atmo, double shade, double survey) {
    if (survey >= 0 && mat != MAT_LAVA) {
        const RGB c = materialRampColor(ramps[mat], clampd(noonStop(albedo), 0, 63));
        const double k = (0.45 + 0.7 * clampd(survey, 0, 1)) * shade, l = clampd((0.3 * c.r + 0.59 * c.g + 0.11 * c.b) * k, 0, 1) * 255;
        const RGB ph((float)((10 + 0.55 * l) / 255), (float)((14 + 1.0 * l) / 255), (float)((16 + 0.92 * l) / 255));
        const RGB o = lerp(ph, c * (float)k, 0.35f);
        return rgb(clampi((int)(o.r * 255), 0, 255), clampi((int)(o.g * 255), 0, 255), clampi((int)(o.b * 255), 0, 255));
    }
    RGB c = materialRampColor(ramps[mat], clampd(exposureStop(albedo, lf, atmo) * shade, 0, 63));
    double r = c.r * 255, gg = c.g * 255, bl = c.b * 255;
    if (mat == MAT_LAVA) { r = 255; gg = 110 + 80 * albedo; bl = 20; }
    return rgb(clampi((int)r, 0, 255), clampi((int)gg, 0, 255), clampi((int)bl, 0, 255));
}

// D-02: the heights' 2nd and 98th percentiles
void Game::surveyRange(const float* h, size_t n, double& lo, double& hi) {
    std::vector<float> s; s.reserve(n / 7 + 1);
    for (size_t i = 0; i < n; i += 7) s.push_back(h[i]);
    if (s.empty()) { lo = 0; hi = 1; return; }
    std::sort(s.begin(), s.end());
    lo = s[s.size() * 2 / 100]; hi = s[std::min(s.size() - 1, s.size() * 98 / 100)];
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
    const bool survey = sys.sunless();   // D-02: the altimeter's map (no clouds: it reads the ground)
    double hLo = 0, hHi = 1; if (survey) surveyRange(m.height.data(), m.height.size(), hLo, hHi);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int i = y * W + x;
            int mat = m.material[i];
            double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y + 1) - hAt(x, y - 1);
            double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);   // light from the north-west
            uint32_t base = mapColor(ramps, mat, m.albedo[i] / 255.0, lf, atmo, shade, survey ? surveyHeight(m.height[i], hLo, hHi) : -1);
            double r = base & 255, gg = (base >> 8) & 255, bl = (base >> 16) & 255;
            if (mat == MAT_LAVA) mapEmissive[i] = 1;
            if (mapClouds && atmo && !survey) {
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

// O2 (R-301): the landing map's zoom into the cursor's sector (Z): a window two sectors wide and one tall, sampled
// from the planet function at the far ring's detail, with the sector grid drawn over it
// the zoom window: one sector tall (a degree of latitude), two sectors' worth of ground wide (the frame is
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
    // the window reads the drainage (rivers as bands, lakes) from tiles already built; a window whose tiles are
    // not built yet is drawn without the drainage at once, its tiles are asked for on the worker, and the window is
    // drawn again when they are there (the zoom itself must not compute tiles: nine of them took two seconds)
    const Vec3 zoomCentre = StarSystem::bodyFromLatLon(zoomLat, zoomLon);
    const double zoomRadiusM = std::max(zoomHalfLat, zoomHalfLon * std::max(std::cos(zoomLat), 0.15)) * b.radiusKm * 1000.0 * 1.05;
    zoomDry = !drainageReady(g, zoomCentre, zoomRadiusM);
    if (zoomDry) { drainagePrefetch(g, zoomCentre, zoomRadiusM, true); zoomPrefetchT = realTime; }
    const bool dry = zoomDry;
    zoomImg.assign(ZW * ZH, 0); zoomEmissive.assign(ZW * ZH, 0); zoomUnit.assign(ZW * ZH, Vec3());
    std::vector<float> hs(ZW * ZH), albs(ZW * ZH); std::vector<uint8_t> mats(ZW * ZH);
    auto zt = std::chrono::steady_clock::now();   // the build's parts
    auto zoomPart = [&](int k) { auto n = std::chrono::steady_clock::now(); zoomBuildMs[k] = std::chrono::duration<double, std::milli>(n - zt).count(); zt = n; };
    for (double& ms : zoomBuildMs) ms = 0;
    zoomBuilds++;
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
    zoomPart(0);
    MatRamp ramps[MAT_COUNT]; double lf; bool atmo;
    mapRampsFor(b, ramps, lf, atmo);
    auto hAt = [&](int x, int y) { x = clampi(x, 0, ZW - 1); y = clampi(y, 0, ZH - 1); return (double)hs[y * ZW + x]; };
    double sum2 = 0; int cnt = 0;
    for (int y = 0; y < ZH; y += 2)
        for (int x = 0; x < ZW; x += 2) { double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y + 1) - hAt(x, y - 1); sum2 += gx * gx + gy * gy; cnt++; }
    double k = std::min(0.5 / (std::sqrt(sum2 / std::max(1, cnt)) + 1e-9), 0.5 / (0.03 * 2 * texelM));   // a 3% grade at most for full shading
    const bool survey = sys.sunless();   // the window's heights the survey's (its lows dark, its highs pale)
    double hLo = 0, hHi = 1; if (survey) surveyRange(hs.data(), hs.size(), hLo, hHi);
    for (int y = 0; y < ZH; y++)
        for (int x = 0; x < ZW; x++) {
            int i = y * ZW + x;
            double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y + 1) - hAt(x, y - 1);
            double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);
            zoomImg[i] = mapColor(ramps, mats[i], albs[i], lf, atmo, shade, survey ? surveyHeight(hs[i], hLo, hHi) : -1);
            zoomEmissive[i] = mats[i] == MAT_LAVA;
        }
    if (g.tideRangeM > 0) {   // the tide lines over the flats (the high water's pale, the low water's dark), as the sector map draws them
        const double S = g.liquidLevel > -1e8 ? g.liquidLevel : 0.0;
        for (int y = 0; y + 1 < ZH; y++)
            for (int x = 0; x + 1 < ZW; x++) {
                const int i = y * ZW + x;
                const double Rl = tideRangeAt(g, zoomUnit[i]), hi = S + 0.5 * Rl, lo = S - 0.5 * Rl, h0 = hs[i], hr = hs[i + 1], hd = hs[i + ZW];
                if ((h0 - hi) * (hr - hi) < 0 || (h0 - hi) * (hd - hi) < 0) zoomImg[i] = rgb(232, 226, 204);
                else if ((h0 - lo) * (hr - lo) < 0 || (h0 - lo) * (hd - lo) < 0) zoomImg[i] = rgb(40, 62, 120);
            }
    }
    zoomPart(1);
    // the old roads of the window (`peoples/roads.*`), the same network the ground carries, as texel polylines thinned to a texel's step
    zoomRoads.clear(); zoomRoadKinds.clear(); zoomRoadsMs = 0; zoomCores = 0;
    zoomCivImg = zoomImg; zoomPlaces.clear();   // the cores over a copy (the peoples' layer's), the land alone in `zoomImg`
    if (worldHadCivilisation(g)) {
        auto tr0 = std::chrono::steady_clock::now();
        auto texel = [&](const Vec3& u, float& x, float& y) {
            double la, lo;
            StarSystem::latLonFromBody(u, la, lo);
            x = (float)((wrapAngle(lo - zoomLon) / (2 * zoomHalfLon) + 0.5) * ZW);
            y = (float)((0.5 - (la - zoomLat) / (2 * zoomHalfLat)) * ZH);
        };
        auto addLine = [&](const std::vector<Vec3>& pts, int kind) {
            std::vector<std::pair<float, float>> poly; float lx = 0, ly = 0;
            for (size_t k = 0; k < pts.size(); k++) {
                float x, y; texel(pts[k], x, y);
                if (poly.empty() || k + 1 == pts.size() || std::hypot(x - lx, y - ly) >= 0.7f) { poly.push_back({x, y}); lx = x; ly = y; }
            }
            if (poly.size() >= 2) { zoomRoads.push_back(poly); zoomRoadKinds.push_back(kind); }
        };
        std::vector<Road> rs; roadsNear(g, zoomCentre, zoomRadiusM, rs, true);
        for (const Road& r : rs) {
            // the roads of the villages and the towns: a lane between two hamlets is not seen from orbit, and the whole web was a honeycomb
            // over the land
            if (std::max(settlementRank(r.a.sclass), settlementRank(r.b.sclass)) < 2) continue;
            addLine(r.pts, NK_ROAD);
        }
        zoomPart(2);
        // the world plan's ways through the window (their waypoints: a leg's walk wanders less than a texel) and its places' cores,
        // each a disc of its built ground over the map (the core's share of a texel, darkened by the years: what the ground holds)
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        std::vector<PlanEdgeRef> es; planEdgesNear(*P, zoomCentre, zoomRadiusM, es);
        for (const PlanEdgeRef& r : es) if (r.e->kind != NK_SEA) addLine(planEdgeWay(g, *P, r), r.e->kind);
        zoomPart(3);
        std::vector<const PlanPlace*> ps; planPlacesNear(*P, zoomCentre, zoomRadiusM, ps);
        MatRamp ramps2[MAT_COUNT]; double lf2; bool atmo2; mapRampsFor(b, ramps2, lf2, atmo2);
        const uint32_t rock = mapColor(ramps2, familyRep(g.type, FAM_ROCK), 0.45, lf2, atmo2, 1.0);
        for (const PlanPlace* p : ps) {
            zoomPlaces.push_back({p->people, p->rank, p->sclass, p->unit, p->radiusM / texelM});   // for the places the explorer knows
            const Culture cu = cultureOf(g, p->people);
            const RGB pc = cu.palette.colour[0];
            const double rr = (rock & 255) * 0.5 + pc.r * 140, gg = ((rock >> 8) & 255) * 0.5 + pc.g * 140, bb = ((rock >> 16) & 255) * 0.5 + pc.b * 140;
            const double standing = 0.75 - 0.45 * cu.decay;
            float cx, cy; texel(p->unit, cx, cy);
            const double rT = p->radiusM / texelM;
            bool any = false;
            for (int y = (int)std::floor(cy - rT - 1); y <= (int)std::ceil(cy + rT + 1); y++)
                for (int x = (int)std::floor(cx - rT - 1); x <= (int)std::ceil(cx + rT + 1); x++) {
                    if (x < 0 || x >= ZW || y < 0 || y >= ZH) continue;
                    const double d = std::hypot(x + 0.5 - cx, y + 0.5 - cy);
                    const double f = clampd(rT + 0.5 - d, 0, 1) * std::min(1.0, PI * rT * rT) * standing;   // the disc's share of the texel (a core under a texel its area's)
                    if (f <= 0.01) continue;
                    uint32_t& c = zoomCivImg[y * ZW + x];
                    const double r0 = c & 255, g0 = (c >> 8) & 255, b0 = (c >> 16) & 255;
                    c = rgb(clampi((int)(r0 + (rr - r0) * f), 0, 255), clampi((int)(g0 + (gg - g0) * f), 0, 255), clampi((int)(b0 + (bb - b0) * f), 0, 255));
                    any = true;
                }
            if (any) zoomCores++;
        }
        zoomRoadsMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tr0).count();
        zoomPart(4);
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

// O0-04 (B-306): the shadows of moons, the parent and the rings on the landing map and its zoom, as the globe shows them
// rebuilt for another body, every 20 s of game time and at most twice a second of real time (a moon's shadow crawls)
void Game::refreshMapShadows(const std::vector<Vec3>& unitLUT) {
    if (mapShadowBody == landBody && std::fabs(t - mapShadowT) < 20 && realTime - mapShadowReal < 0.5 && mapShadow.size() == unitLUT.size() && (!landZoom || zoomShadow.size() == zoomUnit.size()))
        return;
    mapShadowBody = landBody; mapShadowT = t; mapShadowReal = realTime;
    buildShadowMap(unitLUT, mapShadow);
    if (landZoom && !zoomUnit.empty()) buildShadowMap(zoomUnit, zoomShadow); else zoomShadow.clear();
}
