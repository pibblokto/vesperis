// The landing map: site selection on the body's planet map (its pictures are built in landing_map_build.cpp).
#include "game.h"
#include "core/parallel.h"
#include "peoples/roads.h"
#include "peoples/plan.h"
#include "peoples/buildings.h"
#include <chrono>
#include "peoples/ruins.h"
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
    spaceR.mapIfReady(sys.bodies[landBody]);   // F-02: asked, never waited for: the screen reads the world until its thread has made the map (the keys
                                               // work meanwhile as they did after the wait: Enter lands at the cursor, R waits for the map)
    worldPlanPrefetch(spaceR.genFor(sys.bodies[landBody]));   // (already on its way since the approach)
    if (worldHadCivilisation(spaceR.genFor(sys.bodies[landBody]))) droneWorldAsk(landBody);   // its records and their homes, for V
    drainLat = 1e9; drainLon = 1e9;   // the map's first update prefetches the drainage of the default site
    landSeenLat = landLat; landSeenLon = landLon; landMovedT = -1e9;   // at once (the cursor has not moved)
    landZoom = 0;   // start from the whole world
    state = GameState::LANDING_MAP;
}

bool Game::landingZoomAllowed() const { return landBody >= 0 && landBody < (int)sys.bodies.size() && sys.bodies[landBody].radiusKm >= 100; }

void Game::updateLandingMap(const Input& in, double dt, double realDt) {
    // O2: zoomed in, the arrows step a hundredth of a degree (about a kilometre), the whole-world view one degree
    double step = (landZoom ? (in.shift() ? 0.05 : 0.01) : (in.shift() ? 6.0 : 1.0)) * DEG;
    if (in.wasPressed(KEY_LEFT) || (in.isDown(KEY_LEFT) && in.shift())) landLon -= step;
    if (in.wasPressed(KEY_RIGHT) || (in.isDown(KEY_RIGHT) && in.shift())) landLon += step;
    if (in.wasPressed(KEY_UP) || (in.isDown(KEY_UP) && in.shift())) landLat += step;
    if (in.wasPressed(KEY_DOWN) || (in.isDown(KEY_DOWN) && in.shift())) landLat -= step;
    if (in.mouseDx != 0 || in.mouseDy != 0) {   // the mouse's motion moves the cursor smoothly: a controller's right stick (the mouse is not held here, so it is the stick's)
        const double k = (landZoom ? 0.0013 : 0.15) * settings.mouseSensitivity * DEG;   // degrees a mouse pixel: about 60 a second at the stick's full throw, half a degree zoomed in
        landLon += in.mouseDx * k / (landZoom ? std::max(0.15, std::cos(landLat)) : 1.0);   // the zoom's window is wider in longitude by the latitude
        landLat -= in.mouseDy * k;
    }
    landLat = clampd(landLat, -88 * DEG, 88 * DEG);
    landLon = wrapAngle(landLon);
    // the drainage tiles of the sector under the cursor are computed on their own thread while the site is chosen,
    // so the landing's first frame finds them in the cache (a tile takes 100-300 ms on a big world). B-422: once the cursor
    // rests 0.4 s: every step asked for its own tiles, and a cursor held with Shift kept every core busy (46 threads, 8 cores
    // of 10) and the Enter after it waited three seconds on the places it had passed
    if (landLat != landSeenLat || landLon != landSeenLon) { landSeenLat = landLat; landSeenLon = landLon; landMovedT = realTime; }
    if ((std::fabs(landLat - drainLat) > 1e-9 || std::fabs(landLon - drainLon) > 1e-9) && realTime - landMovedT >= 0.4) {
        drainLat = landLat; drainLon = landLon;
        drainagePrefetch(spaceR.genFor(sys.bodies[landBody]), StarSystem::bodyFromLatLon(landLat, landLon), 65000.0, true);
    }
    if (in.wasPressed(KEY_Z) && !in.ctrl()) {
        if (!landingZoomAllowed()) status("THE WHOLE WORLD ALREADY FITS THE MAP", 3);
        else { landZoom = !landZoom; if (landZoom) buildLandingZoom(); audio.beep = BEEP_UI; }
    }
    const PlanetMap* rMap = in.wasPressed(KEY_R) ? spaceR.mapIfReady(sys.bodies[landBody]) : nullptr;   // R once the world's map is made
    if (rMap) {
        // random lit land site
        Rng r((uint64_t)(realTime * 1000));
        const PlanetMap& m = *rMap;
        Mat3 frame = sys.bodyFrame(landBody, t);
        Vec3 sunB = frame * normalize(sys.star.pos - sys.bodyPos(landBody, t));
        for (int k = 0; k < 200; k++) {
            double la = r.range(-70 * DEG, 70 * DEG), lo = r.range(-PI, PI);
            int mat = m.materialAt(lo, la);
            Vec3 u = StarSystem::bodyFromLatLon(la, lo);
            if (mat != MAT_WATER && (sys.sunless() || dot(u, sunB) > 0.2)) { landLat = la; landLon = lo; break; }   // no side lit where there is no sun
        }
    }
    if (in.wasPressed(KEY_C)) mapClouds = !mapClouds;   // the original's cloud filter
    if (in.wasPressed(KEY_V) && !in.ctrl()) { landPeoples = !landPeoples; audio.beep = BEEP_UI; }   // the peoples over the map, off by default
    if (in.wasPressed(KEY_J)) {   // jump to the point where the biggest body in this world's sky stands highest
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
    if (landZoom && (zoomImg.empty() || zoomSeed != sys.bodies[landBody].seed || std::fabs(landLat - zoomLat) > 0.75 * zoomHalfLat || std::fabs(wrapAngle(landLon - zoomLon)) > 0.75 * zoomHalfLon))
        buildLandingZoom();
    else if (landZoom && zoomDry) {   // the window was drawn without the drainage; again with it once its tiles are built
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
        {   // the capsule sets down outside a settlement's walls
            double la = landLat, lo = landLon;
            if (settlementClearance(BodyGen::make(sys.bodies[landBody]), la, lo)) { landLat = la; landLon = lo; status("THE CAPSULE KEEPS OUTSIDE THE RUINS", 4); }
        }
        surf.init(&sys, landBody, landLat, landLon, t);
        syncShards(); roadsMet.clear();   // C-03; C-09
        surf.cameraOverrideAlt = 1800;
        surf.cameraOverridePitch = -60 * DEG;
        surf.player.yaw = wrap2pi(landLon + 1.0);
        audio.beep = BEEP_ARRIVE;
        autosave();
    }
    updateShipMotion(dt);
}

// U-04: the landing map's controller: its commands, and only the buttons that do something there, in their words (the cursor: the
// d-pad and the sticks)
void Game::landingMapPad(PadMenu& m) const {
    m.title = "LANDING MAP";
    m.item("LAND HERE", KEY_ENTER);
    if (landingZoomAllowed()) m.item(landZoom ? "THE WHOLE WORLD" : "ZOOM INTO THE SECTOR", KEY_Z);
    m.item("A RANDOM LIT SITE", KEY_R);
    m.item(mapClouds ? "HIDE THE CLOUDS" : "SHOW THE CLOUDS", KEY_C);
    m.item("UNDER THE SKY'S BIGGEST BODY", KEY_J);
    m.item(landPeoples ? "HIDE THE PEOPLES" : "THE PEOPLES", KEY_V);
    m.item("THE ALMANAC OF THE SITE", KEY_T, MOD_CTRL);
    m.item("BACK TO THE SHIP", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "LAND"); m.button(PR_B, KEY_ESCAPE, "BACK");
    if (landZoom) m.button(PR_LT, KEY_Z, "THE WORLD"); else m.button(PR_RT, KEY_Z, "ZOOM IN");   // its zoom is Z, in and out
    m.button(PR_L3, KEY_LEFT_SHIFT, "BIG STEPS"); m.button(PR_START, KEY_ESCAPE, "BACK");
    m.sticks = "STICKS MOVE THE CURSOR";
}

// N1-06: the relief class at a site from the terrain itself: the steepest rise over 150 m and over 600 m
double Game::siteSlope(const SurfaceSite& probe) {
    // the grade of the region: the mean of eight directional slopes (150 and 600 m in four directions at the
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

// the landing map's picture: the map, or space while auto-explore chooses its site in its head
void Game::renderLandingScreen() { if (ax.on) renderSpace(); else renderLandingMap(); }

void Game::renderLandingMap() {
    // background: keep the space view, dimmed
    renderSpace();
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 150);
    const Body& b = sys.bodies[landBody];
    int ox = 32, oy = 12;
    const PlanetMap* mp = spaceR.mapIfReady(b);
    if (!mp) {   // F-02: the world's map is being made on its thread (a world not mapped yet held this screen 150-200 ms a map)
        drawRectRGB(canvas, ox - 1, oy - 1, ox + MAP_LW, oy + MAP_LH, HUD_DIM);
        drawTextCentered(canvas, ox + MAP_LW / 2, oy + MAP_LH / 2 - 3, "READING THE WORLD...", HUD_DIM);
        drawTextCentered(canvas, UW / 2, 2, "SURFACE CAPSULE - SELECT LANDING SECTOR", HUD_CYAN);
        return;
    }
    const PlanetMap& m = *mp;
    Mat3 frame = sys.bodyFrame(landBody, t);
    Vec3 sunB = frame * normalize(sys.star.pos - sys.bodyPos(landBody, t));
    const double dayFloor = sys.sunless() ? 1.0 : 0.22;   // the survey has no night side
    const int S = FB_SCALE;
    if (mapBaseSeed != b.seed || mapBaseClouds != mapClouds || mapBase.empty()) buildLandingMapBase();
    static std::vector<Vec3> unitLUT;   // body-frame unit vector per texel
    if (unitLUT.empty()) {
        unitLUT.resize(PlanetMap::W * PlanetMap::H);
        for (int y = 0; y < PlanetMap::H; y++)
            for (int x = 0; x < PlanetMap::W; x++)
                unitLUT[y * PlanetMap::W + x] = StarSystem::bodyFromLatLon((0.5 - (y + 0.5) / PlanetMap::H) * PI, ((x + 0.5) / PlanetMap::W) * TAU - PI);
    }
    refreshMapShadows(unitLUT);   // the moons', the parent's and the rings' shadows on the day side
    const int PW = MAP_LW * S, PH = MAP_LH * S;
    // the fronts' bands (the same overlay the globe draws), under the cloud filter like the pattern's cloud
    const SpaceRenderer::FrontMap* fmc = mapClouds && bodyHasAir(b) && !hasOpaqueDeck(b.type) ? spaceR.frontsFor(b, t) : nullptr;
    const std::vector<uint8_t>* fm = fmc ? &fmc->cloud : nullptr;
    if (fm && landZoom && !zoomUnit.empty() && (zoomFront.size() != zoomUnit.size() || std::fabs(t - zoomFrontT) > 240 || zoomFrontLat != zoomLat || zoomFrontLon != zoomLon)) {
        zoomFront.resize(zoomUnit.size()); zoomFrontT = t; zoomFrontLat = zoomLat; zoomFrontLon = zoomLon;
        for (size_t i = 0; i < zoomUnit.size(); i++) {
            double la, lo;
            StarSystem::latLonFromBody(zoomUnit[i], la, lo);
            zoomFront[i] = (uint8_t)(frontMapAt(*fm, PlanetMap::W, PlanetMap::H, lo, la) * 255 + 0.5);
        }
    }
    auto withFront = [&](uint32_t c, double fc) {   // the band whitens the texel as the cloud filter does
        double r = c & 255, gg = (c >> 8) & 255, bl = (c >> 16) & 255;
        fc *= 0.6; r += (245 - r) * fc; gg += (245 - gg) * fc; bl += (250 - bl) * fc;
        return rgb((int)r, (int)gg, (int)bl);
    };
    if (landZoom && !zoomImg.empty()) {
        // the zoom window, one texel per logical pixel, with the day side lit as on the whole-world map. F-05: the rows on the pool, a
        // texel's colour made once for the pixels that share it (along a row, and a row whose texels the band's row above had is copied)
        parallelFor(PH, 32, [&](int pyb, int pye) {
            int lastTy = -1;
            for (int py = pyb; py < pye; py++) {
                int ty = py / S;
                uint32_t* row = canvas.px + (size_t)(oy * S + py) * FBW + ox * S;
                if (ty == lastTy) { std::copy(row - FBW, row - FBW + PW, row); continue; }
                lastTy = ty;
                int lastI = -1; uint32_t col = 0;
                for (int px = 0; px < PW; px++) {
                    int i = ty * ZW + px / S;
                    if (i != lastI) {
                        lastI = i;
                        double lit = dot(zoomUnit[i], sunB);
                        double l = dayFloor + (1 - dayFloor) * smoothstep(-0.05, 0.15, lit) * (zoomShadow.size() == zoomUnit.size() ? 1 - 0.9 * zoomShadow[i] / 255.0 : 1.0);
                        if (zoomEmissive[i]) l = std::max(l, 0.95);
                        uint32_t c = landPeoples && zoomCivImg.size() == zoomImg.size() ? zoomCivImg[i] : zoomImg[i];   // the cores with the peoples
                        if (fm && zoomFront.size() == zoomUnit.size() && zoomFront[i]) c = withFront(c, zoomFront[i] / 255.0);
                        col = rgb((int)((c & 255) * l), (int)(((c >> 8) & 255) * l), (int)(((c >> 16) & 255) * l));
                    }
                    row[px] = col;
                }
            }
        });
        for (size_t ri = 0; landPeoples && ri < zoomRoads.size(); ri++) {   // the old roads as faint lines, the ground's own colour darkened under the texel's light (R-415: with the peoples)
            const auto& poly = zoomRoads[ri];
            int tx = clampi((int)poly[0].first, 0, ZW - 1), ty = clampi((int)poly[0].second, 0, ZH - 1), i = ty * ZW + tx;
            double l = dayFloor + (1 - dayFloor) * smoothstep(-0.05, 0.15, dot(zoomUnit[i], sunB)) * (zoomShadow.size() == zoomUnit.size() ? 1 - 0.9 * zoomShadow[i] / 255.0 : 1.0);
            uint32_t c = zoomImg[i];
            // by its kind: a path fainter, a railway darker, a highway and a maglev's pylons pale, a canal's channel blue-dark
            const int kind = ri < zoomRoadKinds.size() ? zoomRoadKinds[ri] : NK_ROAD;
            static const double KL[NK_COUNT] = {0.85, 0.7, 0.5, 0.6, 1.3, 1.25, 1.0};
            l *= KL[clampi(kind, 0, NK_COUNT - 1)];
            double cr = (c & 255) * l, cg = ((c >> 8) & 255) * l, cb = ((c >> 16) & 255) * l;
            if (kind == NK_CANAL) { cr *= 0.75; cg *= 0.85; cb = std::min(255.0, cb * 1.3 + 12); }
            uint32_t col = rgb(clampi((int)cr, 0, 255), clampi((int)cg, 0, 255), clampi((int)cb, 0, 255));
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
        // the 512x256 map fills a 256x128 logical frame: one texel per pixel at 2x, every second texel at 1x. F-05: the rows on the pool, a
        // texel's colour made once for the pixels that share it (at 3x and 4x; at 1x the skipped texels are never made)
        parallelFor(PH, 32, [&](int pyb, int pye) {
            int lastTy = -1;
            for (int py = pyb; py < pye; py++) {
                int ty = py * PlanetMap::H / PH;
                uint32_t* row = canvas.px + (size_t)(oy * S + py) * FBW + ox * S;
                if (ty == lastTy) { std::copy(row - FBW, row - FBW + PW, row); continue; }
                lastTy = ty;
                int lastTx = -1; uint32_t col = 0;
                for (int px = 0; px < PW; px++) {
                    int tx = px * PlanetMap::W / PW;
                    if (tx != lastTx) {
                        lastTx = tx;
                        int i = ty * PlanetMap::W + tx;
                        double lit = dot(unitLUT[i], sunB);
                        double l = dayFloor + (1 - dayFloor) * smoothstep(-0.05, 0.15, lit) * (mapShadow.size() == unitLUT.size() ? 1 - 0.9 * mapShadow[i] / 255.0 : 1.0);
                        if (mapEmissive[i]) l = std::max(l, 0.95);
                        uint32_t c = mapBase[i];
                        if (fm && (*fm)[i]) c = withFront(c, (*fm)[i] / 255.0);
                        col = rgb((int)((c & 255) * l), (int)(((c >> 8) & 255) * l), (int)(((c >> 16) & 255) * l));
                    }
                    row[px] = col;
                }
            }
        });
    }
    (void)m;
    drawRectRGB(canvas, ox - 1, oy - 1, ox + MAP_LW, oy + MAP_LH, HUD_DIM);
    int cx, cy;
    if (landZoom && !zoomImg.empty()) {
        // the sector grid (one-degree lines) with the sectors' names, a scale bar, and the cursor within the window
        const uint32_t gridC = rgb(80, 80, 50);
        int k0 = (int)std::ceil((zoomLon - zoomHalfLon) / DEG), k1 = (int)std::floor((zoomLon + zoomHalfLon) / DEG);
        for (int k = k0; k <= k1; k++) {
            int x = ox + (int)(((k * DEG - zoomLon) / (2 * zoomHalfLon) + 0.5) * MAP_LW);
            if (x > ox && x < ox + MAP_LW - 1) drawLineRGB(canvas, x, oy, x, oy + MAP_LH - 1, gridC);
        }
        int j0 = (int)std::ceil((zoomLat - zoomHalfLat) / DEG), j1 = (int)std::floor((zoomLat + zoomHalfLat) / DEG);
        for (int j = j0; j <= j1; j++) {
            int y = oy + (int)((0.5 - (j * DEG - zoomLat) / (2 * zoomHalfLat)) * MAP_LH);
            if (y > oy && y < oy + MAP_LH - 1) drawLineRGB(canvas, ox, y, ox + MAP_LW - 1, y, gridC);
        }
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
        for (const SurfaceMark& m : guide.marks) {   // the explorer's marks of this world (a glyph each, a friend's in cyan), eight names at most
            if (m.body != bkey || m.kind == LM_RUIN || m.kind == LM_WORK) continue;   // the ruins and the works with the peoples (V)
            int lx = ox + (int)((wrapAngle(m.lon - zoomLon) / (2 * zoomHalfLon) + 0.5) * MAP_LW), ly = oy + (int)((0.5 - (m.lat - zoomLat) / (2 * zoomHalfLat)) * MAP_LH);
            if (lx < ox + 1 || ly < oy + 1 || lx >= ox + MAP_LW - 1 || ly >= oy + MAP_LH - 1) continue;
            const uint32_t lc = m.lent ? HUD_CYAN : HUD_WHITE;
            if (m.kind < LM_COUNT) drawText(canvas, lx - 2, ly - 3, LANDMARK_KIND_SYMBOLS[m.kind], lc);
            else {
                drawLineRGB(canvas, lx - 2, ly, lx, ly - 2, lc);
                drawLineRGB(canvas, lx, ly - 2, lx + 2, ly, lc);
                drawLineRGB(canvas, lx + 2, ly, lx, ly + 2, lc);
                drawLineRGB(canvas, lx, ly + 2, lx - 2, ly, lc);
            }
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
        for (const SurfaceMark& m : guide.marks) {   // the explorer's marks of this world, a dot each (Z zooms in on them)
            if (m.body != bkey || m.kind == LM_RUIN || m.kind == LM_WORK) continue;   // the ruins and the works with the peoples (V)
            int mx = ox + (int)((wrap2pi(m.lon + PI) / TAU) * MAP_LW), my = oy + (int)((0.5 - m.lat / PI) * MAP_LH);
            fillRectRGB(canvas, mx - 1, my - 1, mx + 1, my + 1, rgb(0, 0, 0)); fillRectRGB(canvas, mx, my, mx, my, m.lent ? HUD_CYAN : HUD_WHITE);
        }
    }
    cx = clampi(cx, ox, ox + MAP_LW - 1); cy = clampi(cy, oy, oy + MAP_LH - 1);
    if (landPeoples) drawLandPeoples(ox, oy, cx, cy); else landDrawn = {};
    drawLineRGB(canvas, ox, cy, ox + MAP_LW - 1, cy, rgb(255, 255, 255));
    drawLineRGB(canvas, cx, oy, cx, oy + MAP_LH - 1, rgb(255, 255, 255));
    drawRectRGB(canvas, cx - 3, cy - 3, cx + 3, cy + 3, HUD_AMBER);
    // info
    if (probeBody != landBody || probeLat != landLat || probeLon != landLon) {
        probeSite.initFrame(&sys, landBody, landLat, landLon, t);   // sampled, never drawn (the five caches cost 0.3-0.5 ms a step)
        probeBody = landBody; probeLat = landLat; probeLon = landLon;
    }
    const SurfaceSite& probe = probeSite;
    SunInfo si = probe.sun(t);
    if (sys.ofCompanion(landBody)) probe.sun2(t, si);   // a world of the companion has its day by the companion
    // the probe reads the drainage only once the site's tiles are built (the cursor's prefetch); a probe that
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
    const bool noSun = sys.sunless();   // the lines of a world with no sun: its colour as a lamp will show it, no sun, its turn
    // N1-04/06: what you will see: material, elevation, relief class, and the ground colour at noon as a swatch
    std::string l2 = fmt("%s %+.0fM  %s", MATERIAL_NAMES[mat], hgt, reliefClass(siteSlope(probe)));
    drawText(canvas, 8, ry + 8, l2.c_str(), si.altitude > 0.05 ? HUD_WHITE : HUD_DIM);
    {
        const BodyGen& g = spaceR.genFor(b);
        RGB gc = materialRampColor(g.matColor[mat] * lerp(sys.star.color, RGB(1, 1, 1), 0.5f), exposureStop(tvp.albedo, noSun ? 1.0 : clampd(si.lightFactor, 0, 1.15), bodyHasAir(b)));
        int sx0 = 8 + textWidth(l2.c_str()) + 8;
        drawText(canvas, sx0, ry + 8, "GROUND", HUD_DIM);
        int bx = sx0 + textWidth("GROUND") + 4;
        blendRectRGB(canvas, bx, ry + 7, bx + 23, ry + 14, rgb((int)(gc.r * 255), (int)(gc.g * 255), (int)(gc.b * 255)), 255);
        drawRectRGB(canvas, bx - 1, ry + 6, bx + 24, ry + 15, HUD_DIM);
    }
    std::string sn = tvp.biome && !sys.sunless() ? seasonName(probe.season, landLat, b.axialTilt) : "";   // no sun, no season
    std::string l3 = noSun ? fmt("%s%sNO SUN - THE DARK", tvp.biome ? BIOME_NAMES[tvp.biome] : "", tvp.biome ? "  " : "")
                           : fmt("%s%s%s%sSUN %+.0f%c AZ %03.0f%c %s", tvp.biome ? BIOME_NAMES[tvp.biome] : "", (tvp.biome && !sn.empty()) ? ", " : "", sn.c_str(), tvp.biome ? "  " : "",
                                 si.altitude / DEG, CH_DEGREE, si.azimuth / DEG, CH_DEGREE, phase);
    if (kindHas(b.type, KT_FLORA) && tvp.biome) {   // the species most likely met here
        Bestiary B = Bestiary::make(b, spaceR.genFor(b));
        int spi = B.pickLand(hash2i((int64_t)(landLat * 100), (int64_t)(landLon * 100), b.seed), tvp.biome);
        if (spi >= 0 && tvp.biome != BIO_ICE && tvp.biome != BIO_OCEAN) l3 += fmt("  LIFE: %s", trunc(upper(B.species[spi].name), 10).c_str());
    }
    drawText(canvas, 8, ry + 16, l3.c_str(), si.altitude > 0.05 ? HUD_WHITE : HUD_DIM);
    double lockRatio = std::fabs(1.0 - b.rotPeriod / b.orbitPeriod);
    std::string dayLen = (b.parent < 0 && lockRatio < 0.02) ? "TIDALLY LOCKED" : fmt("DAY %.1f H", std::fabs(b.rotPeriod) / 3600.0 * (b.parent < 0 ? 1.0 / lockRatio : 1.0));
    std::string grav = b.gravity / 9.8 < 0.01 ? fmt("GRAV %.4f G", b.gravity / 9.8) : fmt("GRAV %.2f G", b.gravity / 9.8);   // a comet's microgravity
    std::string l4 = noSun ? fmt("TURNS IN %.1f H  %s  %s", std::fabs(b.rotPeriod) / 3600.0, grav.c_str(), KINDS[b.type].atmosphere ? (airFrozen(b) ? "AIR FROZEN" : "ATMOSPHERE") : "NO ATMOSPHERE")
                           : fmt("LOCAL %02d:%02d  %s  %s  %s", (int)(si.dayFraction * 24), (int)(std::fmod(si.dayFraction * 24, 1.0) * 60),
                                 dayLen.c_str(), grav.c_str(), KINDS[b.type].atmosphere ? (airFrozen(b) ? "AIR FROZEN" : "ATMOSPHERE") : "NO ATMOSPHERE");
    drawText(canvas, 8, ry + 24, l4.c_str(), HUD_DIM);
    {   // what hangs in the sky over this sector right now
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
        // the world's own rings: how high their arc stands in this site's sky
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
            const char* ph = noSun ? "UNLIT" : (bestPhase > 0.85 ? "FULL" : (bestPhase > 0.6 ? "GIBBOUS" : (bestPhase > 0.4 ? "HALF LIT" : (bestPhase > 0.15 ? "CRESCENT" : "NEW"))));
            l5 = fmt("SKY: %s%s %.0f%c UP, %.1f%c WIDE, %s%s", ringNote.empty() ? "" : (ringNote + "; ").c_str(), trunc(upper(bodyNameOf(best)), ringNote.empty() ? 14 : 10).c_str(), bestAlt / DEG,
                    CH_DEGREE, 2 * bestAng / DEG, CH_DEGREE, bestRinged ? "RINGED" : ph, (points && ringNote.empty() && !bestRinged) ? fmt(" +%d PTS", points).c_str() : "");
        } else l5 = (ringNote.empty() ? std::string("SKY: ") : "SKY: " + ringNote + "; ") + (points ? fmt("%d WORLDS AS POINTS OF LIGHT", points) : std::string("NOTHING BUT STARS"));
        drawText(canvas, 8, ry + 32, l5.c_str(), (best >= 0 || !ringNote.empty()) ? HUD_CYAN : HUD_DIM);
    }
    // V the peoples; the lines fit the 53 characters of the screen (they ran off both edges at 61 and 66)
    if (landZoom) drawTextCentered(canvas, UW / 2, UH - 12, "ARROWS 1 KM  Z WORLD  R RANDOM  V PEOPLES  ENTER", HUD_AMBER);
    // the survey has no clouds
    else drawTextCentered(canvas, UW / 2, UH - 12, bodyHasAir(b) && !noSun ? "Z ZOOM  R RANDOM  C CLOUDS  J SKY  V PEOPLES  ENTER" : "Z ZOOM  R RANDOM  J SKY  V PEOPLES  ENTER LAND", HUD_AMBER);
    drawTextCentered(canvas, UW / 2, 2, landZoom ? fmt("SURFACE CAPSULE - SECTOR %s - PICK THE SITE", sectorName(landLat / DEG, landLon / DEG).c_str()).c_str()
                                                 : (noSun ? "SURFACE CAPSULE - NO LIGHT: THE ALTIMETER'S SURVEY" : "SURFACE CAPSULE - SELECT LANDING SECTOR"), HUD_CYAN);
}

// ---- R-415 (2026-10-11): the peoples on the landing map --------------------------------------------------------------------------------
// The user: "on the capsule deployment map there are these dots for places were civilization ruins were found - they are too huge and
// non-subtle. Please, make them turned of by-default and make it a separate button ... There should be some map like the one you used when
// you were adding cities layout, it just should be a button toggle. Also It would be cool to have something like whole map autocompleted when
// you find all the shards for a civilization, as if you have enough information to know where is what, where to find cities, capital, etc".
// V shows them, off on every map: the explorer's marks of ruins and works as small points (the names of those by the cursor beside it), in the
// zoom the old roads and the places' cores the ground holds (C-09's, T-01's); the places the held records were kept at (each record's home,
// peoples/records.h's `recordHomesOf`, read with the world's items as the drone bay reads them: `droneWorldAsk`), the capital named; and a
// people whose records the ship holds all, its whole plan: its land tinted to its frontier, every place and every way, coloured by kind as
// the plan's maps (`NET_KIND_COLOURS`)
namespace {
const uint32_t PEOPLE_TINT[2] = {0xFF64D2FFu, 0xFFFFE16Eu};   // the first people amber (255, 210, 100), the second cyan (110, 225, 255)
uint32_t mixRGB(uint32_t a, uint32_t b, double t) {
    auto ch = [&](int s) { return (uint32_t)clampi((int)std::lround(((a >> s) & 255) * (1 - t) + ((b >> s) & 255) * t), 0, 255) << s; };
    return 0xFF000000u | ch(0) | ch(8) | ch(16);
}
}

Game::LandKnown Game::landKnownOf(int body) {
    LandKnown K;
    if (!sys.valid || body < 0 || body >= (int)sys.bodies.size()) return K;
    if (!worldHadCivilisation(spaceR.genFor(sys.bodies[body]))) { K.ready = true; return K; }
    if (!droneWorldHere(sys.star.seed, body)) { droneWorldAsk(body); return K; }
    const DroneWorld& W = bay.world;
    K.ready = true; K.peoples = std::min(2, std::max(1, W.peoples));
    const std::string prefix = worldKeyOf(body) + "/S";
    for (const DroneItem& it : W.items) {
        if (it.kind == DF_INSTRUMENT || it.people < 0 || it.people >= K.peoples) continue;
        K.total[it.people]++;
        if (!guide.shards.count(prefix + std::to_string(it.index))) continue;
        K.held[it.people]++;
        if (it.place >= 0 && it.place < (int)W.places.size()) K.ranks[it.people].insert(W.places[it.place].rank);
    }
    // named once a record of theirs is held (the decoder's rule)
    for (int k = 0; k < K.peoples; k++) if (K.held[k] > 0 && k < (int)W.peopleNames.size()) K.name[k] = "THE " + upper(W.peopleNames[k]);
    return K;
}

// R-415: the landing map's peoples (V, off by default): the old roads and the cores over the zoom, the explorer's marks of ruins and works,
// the places the records held were kept at, and a people's whole land, its places and its ways once all its records are held
// the layer over the map (the world's or the zoom's), the cursor at (cx, cy)
void Game::drawLandPeoples(int ox, int oy, int cx, int cy) {
    landDrawn = {};
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return;
    const Body& b = sys.bodies[landBody];
    const BodyGen& g = spaceR.genFor(b);
    const int S = std::max(1, canvas.scale);
    const bool zoom = landZoom && !zoomImg.empty();
    auto toMap = [&](double lat, double lon, double& x, double& y) {   // logical pixels; within the frame?
        if (zoom) { x = ox + (wrapAngle(lon - zoomLon) / (2 * zoomHalfLon) + 0.5) * MAP_LW; y = oy + (0.5 - (lat - zoomLat) / (2 * zoomHalfLat)) * MAP_LH; }
        else { x = ox + wrap2pi(lon + PI) / TAU * MAP_LW; y = oy + (0.5 - lat / PI) * MAP_LH; }
        return x >= ox && y >= oy && x < ox + MAP_LW && y < oy + MAP_LH;
    };
    const int X0 = ox * S, Y0 = oy * S, X1 = (ox + MAP_LW) * S, Y1 = (oy + MAP_LH) * S;
    auto plot = [&](int X, int Y, uint32_t c) { if (X >= X0 && Y >= Y0 && X < X1 && Y < Y1) canvas.px[(size_t)Y * canvas.w + X] = c; };
    auto block = [&](double x, double y, int half, uint32_t c, bool rim) {   // a square of (2 half + 1) frame pixels round a place, a dark rim round it
        const int X = (int)std::floor(x * S), Y = (int)std::floor(y * S);
        if (rim) for (int dy = -half - 1; dy <= half + 1; dy++) for (int dx = -half - 1; dx <= half + 1; dx++) plot(X + dx, Y + dy, HUD_SHADOW);
        for (int dy = -half; dy <= half; dy++) for (int dx = -half; dx <= half; dx++) plot(X + dx, Y + dy, c);
    };
    const LandKnown K = landKnownOf(landBody);
    std::shared_ptr<const WorldPlan> P = K.ready && K.peoples > 0 ? worldPlanIfReady(g) : nullptr;
    // ---- a people's whole land (the world's map): its territory tinted, its frontier drawn
    if (!zoom && P && (K.all(0) || K.all(1))) {
        if (landTerrSeed != b.seed || landTerr.size() != (size_t)MAP_LW * MAP_LH) {
            landTerrSeed = b.seed; landTerr.assign((size_t)MAP_LW * MAP_LH, 0);
            std::vector<uint8_t> in[2];
            for (int k = 0; k < std::min(2, P->peoples); k++) {
                const std::vector<uint8_t>& T = P->plans[k].territory;
                if (T.empty()) continue;
                in[k].assign((size_t)MAP_LW * MAP_LH, 0);
                for (int y = 0; y < MAP_LH; y++)
                    for (int x = 0; x < MAP_LW; x++) {
                        const double lat = (0.5 - (y + 0.5) / MAP_LH) * PI, lon = (x + 0.5) / MAP_LW * TAU - PI;
                        const int cell = P->grid.cellOf(lat, lon);
                        in[k][(size_t)y * MAP_LW + x] = cell >= 0 && cell < (int)T.size() && T[cell];
                    }
                for (int y = 0; y < MAP_LH; y++)
                    for (int x = 0; x < MAP_LW; x++) {
                        if (!in[k][(size_t)y * MAP_LW + x]) continue;
                        const bool edge = !in[k][(size_t)y * MAP_LW + (x + 1) % MAP_LW] || !in[k][(size_t)y * MAP_LW + (x + MAP_LW - 1) % MAP_LW] ||
                                          (y > 0 && !in[k][(size_t)(y - 1) * MAP_LW + x]) || (y + 1 < MAP_LH && !in[k][(size_t)(y + 1) * MAP_LW + x]);
                        landTerr[(size_t)y * MAP_LW + x] |= (uint8_t)((edge ? 2 : 1) << (2 * k));
                    }
            }
        }
        for (int y = 0; y < MAP_LH; y++)
            for (int x = 0; x < MAP_LW; x++) {
                const uint8_t m = landTerr[(size_t)y * MAP_LW + x];
                for (int k = 0; k < 2; k++) {
                    const int v = (m >> (2 * k)) & 3;
                    if (!v || !K.all(k)) continue;
                    landDrawn.land++;
                    for (int sy = 0; sy < S; sy++)
                        for (int sx = 0; sx < S; sx++) {
                            uint32_t& c = canvas.px[(size_t)((oy + y) * S + sy) * canvas.w + (ox + x) * S + sx];
                            c = mixRGB(c, PEOPLE_TINT[k], v == 2 ? 0.45 : 0.15);
                        }
                }
            }
    }
    // ---- its ways (the world's map: from place to place; the zoom draws the ground's own, C-09's and the plan's, under the layer)
    if (!zoom && P)
        for (int k = 0; k < std::min(K.peoples, P->peoples); k++) {
            if (!K.all(k)) continue;
            const PeoplePlan& pp = P->plans[k];
            for (const PlanEdge& e : pp.edges) {
                if (e.kind == NK_SEA || e.a < 0 || e.b < 0 || e.a >= (int)pp.places.size() || e.b >= (int)pp.places.size()) continue;
                double xa, ya, xb, yb;
                toMap(pp.places[e.a].lat, pp.places[e.a].lon, xa, ya); toMap(pp.places[e.b].lat, pp.places[e.b].lon, xb, yb);
                if (std::fabs(xb - xa) > MAP_LW / 2) continue;   // across the map's seam
                const int ax = (int)std::floor(xa * S), ay = (int)std::floor(ya * S), bx = (int)std::floor(xb * S), by = (int)std::floor(yb * S);
                const int n = std::max(std::abs(bx - ax), std::abs(by - ay));
                const uint32_t col = NET_KIND_COLOURS[clampi(e.kind, 0, NK_COUNT - 1)];
                for (int q = 0; q <= n; q++) plot(ax + (n ? (bx - ax) * q / n : 0), ay + (n ? (by - ay) * q / n : 0), col);
                landDrawn.ways++;
            }
        }
    // ---- the places: those the held records were kept at, every one of a people whose records are all held
    struct Spot { int people, sclass; double lat, lon; double radiusT; };
    std::vector<Spot> spots;
    if (zoom) {
        for (const ZoomPlace& z : zoomPlaces) {
            if (z.people < 0 || z.people >= K.peoples || (!K.all(z.people) && !K.ranks[z.people].count(z.rank))) continue;
            double la, lo; StarSystem::latLonFromBody(z.unit, la, lo);
            spots.push_back({z.people, z.sclass, la, lo, z.radiusT});
        }
    } else
        for (int k = 0; k < K.peoples; k++) {
            if (P && k < P->peoples && K.all(k)) { for (const PlanPlace& p : P->plans[k].places) spots.push_back({k, p.sclass, p.lat, p.lon, 0}); continue; }
            for (int r : K.ranks[k]) {
                if (P && k < P->peoples && r >= 0 && r < (int)P->plans[k].places.size()) { const PlanPlace& p = P->plans[k].places[r]; spots.push_back({k, p.sclass, p.lat, p.lon, 0}); continue; }
                for (const DronePlace& dp : bay.world.places) if (dp.people == k && dp.rank == r) { spots.push_back({k, dp.sclass, dp.lat, dp.lon, 0}); break; }
            }
        }
    std::stable_sort(spots.begin(), spots.end(), [](const Spot& a, const Spot& c) { return (a.sclass == SC_CAPITAL) < (c.sclass == SC_CAPITAL); });   // the capital over the rest
    for (const Spot& sp : spots) {
        double x, y;
        if (!toMap(sp.lat, sp.lon, x, y)) continue;
        const uint32_t tint = PEOPLE_TINT[clampi(sp.people, 0, 1)];
        const bool capital = sp.sclass == SC_CAPITAL, great = sp.sclass == SC_CITY || sp.sclass == SC_MONUMENT;
        if (zoom) {   // a ring round the core the ground draws
            const double r = (std::max(1.5, sp.radiusT) + 2) * S;
            const int n = std::max(16, (int)(r * 6.3 / 2));
            for (int q = 0; q < n; q += 1 + (capital ? 0 : 1)) plot((int)std::floor(x * S + r * std::cos(q * TAU / n)), (int)std::floor(y * S + r * std::sin(q * TAU / n)), tint);
            landDrawn.rings++;
        } else block(x, y, capital ? S : S / 2, capital ? HUD_WHITE : (great ? mixRGB(tint, HUD_WHITE, 0.45) : tint), capital || great);   // a town a point, a city rimmed and paler, the capital white
        landDrawn.places++;
        if (capital) {   // named: where the people's capital stood
            const int lx = (int)x + (zoom ? (int)(sp.radiusT + 4) : 4), ly = (int)y - 3;
            if (lx + textWidth("CAPITAL") < ox + MAP_LW && ly > oy && ly + 7 < oy + MAP_LH) { drawTextShadow(canvas, lx, ly, "CAPITAL", tint, HUD_SHADOW); landDrawn.capital++; }
        }
    }
    // ---- the explorer's marks of ruins and works: small points, the names of the three nearest the cursor beside it
    const std::string bkey = worldKeyOf(landBody);
    std::vector<std::pair<double, const SurfaceMark*>> near;
    for (const SurfaceMark& m : guide.marks) {
        if (m.body != bkey || (m.kind != LM_RUIN && m.kind != LM_WORK)) continue;
        double x, y;
        if (!toMap(m.lat, m.lon, x, y)) continue;
        block(x, y, S / 2, m.lent ? HUD_CYAN : HUD_WHITE, true);
        landDrawn.marks++;
        const double d = std::hypot(x - cx, y - cy);
        if (d < 10 && !m.name.empty()) near.push_back({d, &m});
    }
    std::sort(near.begin(), near.end(), [](const auto& a, const auto& c) { return a.first < c.first; });
    for (size_t i = 0; i < near.size() && i < 3; i++) {
        const std::string nm = trunc(upper(near[i].second->name), 14);
        const int lx = std::min(cx + 6, ox + MAP_LW - 2 - textWidth(nm.c_str())), ly = std::min(cy + 5 + 8 * (int)i, oy + MAP_LH - 8);
        blendRectRGB(canvas, lx - 1, ly - 1, lx + textWidth(nm.c_str()), ly + 7, rgb(0, 0, 0), 150);
        drawText(canvas, lx, ly, nm.c_str(), near[i].second->lent ? HUD_CYAN : HUD_WHITE);
    }
    // ---- the legend: each people, its records held, its whole land once they are all
    std::vector<std::pair<std::string, uint32_t>> rows;
    if (!K.ready) rows.push_back({"READING THE WORLD...", HUD_DIM});
    else if (K.peoples == 0) rows.push_back({"NO PEOPLE LIVED ON THIS WORLD", HUD_DIM});
    for (int k = 0; k < K.peoples; k++) {
        const std::string who = K.name[k].empty() ? (k == 0 ? "A PEOPLE" : "ANOTHER PEOPLE") : trunc(K.name[k], 16);
        rows.push_back({K.all(k) ? fmt("%s: ALL %d SHARDS, THE WHOLE MAP", who.c_str(), K.total[k]) : fmt("%s: %d OF %d SHARDS", who.c_str(), K.held[k], K.total[k]), PEOPLE_TINT[k]});
    }
    for (size_t i = 0; i < rows.size(); i++) {
        const int lx = ox + 2, ly = oy + 2 + 9 * (int)i;
        blendRectRGB(canvas, lx - 1, ly - 1, lx + textWidth(rows[i].first.c_str()), ly + 7, rgb(0, 0, 0), 160);
        drawText(canvas, lx, ly, rows[i].first.c_str(), rows[i].second);
    }
}
