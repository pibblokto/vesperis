// Scene rendering entry points and the HUD overlays for space and surface.
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cmath>
#include <functional>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

void Game::renderSpace() {
    int a, b;
    choosePaletteBodies(a, b);
    if (ship.mode == ShipState::VIMANA) { a = -1; b = -1; }
    spaceR.setupPalette(fb, sys.valid ? &sys : nullptr, a, b, fade);
    SpaceContext c;
    c.sys = sys.valid ? &sys : nullptr;
    c.stars = &nb.stars;
    c.t = t;
    c.shipPos = ship.pos;
    bool titleCam = state == GameState::TITLE || (state == GameState::HELP && returnState == GameState::TITLE);
    c.cam = titleCam ? cameraBasis(titleYaw, 0.05) : viewBasis();
    if (titleCam && attractActive()) {
        // M6-06 attract mode: a slow tour of the system, fourteen seconds per body, circling each at four radii
        int nb = (int)sys.bodies.size();
        int bi = (int)(attractT / 14.0) % nb;
        double u = std::fmod(attractT, 14.0) / 14.0;
        const Body& bd = sys.bodies[bi];
        Vec3 bp = sys.bodyPos(bi, t);
        double ang = u * 1.6 + bi;
        double R = bd.radiusKm * (bd.rings ? std::max(4.0, bd.ringOuter + 1.5) : 4.0);
        Vec3 toStar = normalize(sys.star.pos - bp);
        Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
        Vec3 dir = normalize(toStar * (0.5 + 0.4 * std::cos(ang)) + side * std::sin(ang) + Vec3(0, 0.25, 0));
        c.shipPos = bp + dir * R;
        Vec3 fwd = normalize(bp - c.shipPos);
        c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
        a = bi; b = -1;
        spaceR.setupPalette(fb, &sys, a, b, fade);
    }
    c.vimana = ship.mode == ShipState::VIMANA;
    if (c.vimana) {
        double u = clampd(ship.flightT / ship.flightDur, 0, 1);
        c.vimanaSpeed = std::sin(u * PI);
        c.vimanaDirView = c.cam * normalize(ship.flightTo - ship.flightFrom);
    }
    c.bankBodyA = a; c.bankBodyB = b;
    c.starIntensity = (ship.targeting || fieldAmp || radar.on) ? 1.5 : 1.0;   // field amplification while aiming or by choice; C-07: the radar camera sees the stars amplified
    spaceR.render(fb, c);
    if (settings.cabin && !titleCam && state != GameState::LANDING_MAP && !radar.on) { cabinPalette(); drawCabin(); }   // M2; C-07: the radar camera is outside the cabin
    if (radar.on && !titleCam) radarCameraFeed(fb);   // C-07
    fb.mush(2);
    fb.toRGB(rgbBuf.data(), 1.0 + 1.3 * std::max(0.0, arrivalFlash), settings.dither);   // M1-10 arrival flash
}

void Game::drawVisor(uint32_t col) {
    int m = 6, l = 14;
    drawLineRGB(canvas, m, m, m + l, m, col); drawLineRGB(canvas, m, m, m, m + l, col);
    drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, col); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, col);
    drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, col); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, col);
    drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, col); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, col);
}

void Game::drawCommonHUD() {
    if (realTime < statusUntil && !statusMsg.empty()) {
        blendRectRGB(canvas, 0, UH - 12, UW - 1, UH - 1, rgb(0, 0, 0), 120);
        drawTextCentered(canvas, UW / 2, UH - 10, statusMsg.c_str(), HUD_AMBER);
    }
    if (timeWarp > 1) drawText(canvas, UW - 8 - textWidth(fmt("TIME X%.0f", timeWarp).c_str()), 24, fmt("TIME X%.0f", timeWarp).c_str(), HUD_CYAN);
}

void Game::renderSpaceHUD() {
    if (radar.on) { renderRadarCamera(); drawCommonHUD(); return; }   // C-07: the radar camera's own frame
    drawVisor(HUD_DIM);
    if (chartUp && ship.mode != ShipState::VIMANA) drawChartOverlay(viewBasis(), ship.pos, spaceR.proj, nullptr);   // C-10: the chart held up to the sky
    std::string sysName = sys.valid ? trunc(upper(starNameOf(sys.star)), 26) : "INTERSTELLAR SPACE";
    drawTextShadow(canvas, 8, 8, sysName.c_str(), HUD_GREEN, HUD_SHADOW);
    drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);
    if (recording) drawTextShadow(canvas, UW - 8 - textWidth("REC"), 16, "REC", ((int)(realTime * 2) & 1) ? HUD_RED : HUD_DIM, HUD_SHADOW);   // M6-05
    std::string coords = fmt("PARSIS %+lld %+lld %+lld", (long long)sectorOf(ship.pos.x), (long long)sectorOf(ship.pos.y), (long long)sectorOf(ship.pos.z));
    std::string line2 = sys.valid ? sys.classString() + "   " + coords : coords;
    line2 += std::string("  ") + REGION_NAMES[galaxyRegion(sectorOf(ship.pos.x), sectorOf(ship.pos.y), sectorOf(ship.pos.z))];
    drawTextShadow(canvas, 8, 16, line2.c_str(), HUD_DIM, HUD_SHADOW);
    // mode
    const char* modeName = "STANDBY";
    if (ship.mode == ShipState::VIMANA) modeName = "VIMANA FLIGHT";
    else if (ship.mode == ShipState::APPROACH) modeName = "FINE APPROACH";
    else if (ship.mode == ShipState::PARKED) modeName = ship.parkedBelt >= 0 ? "IN THE BELT" : (ship.orbiting ? "ORBIT" : "CHASE");   // O3
    drawTextShadow(canvas, 8, UH - 24, modeName, HUD_AMBER, HUD_SHADOW);
    if (ship.mode == ShipState::VIMANA) {
        double u = clampd(ship.flightT / ship.flightDur, 0, 1);
        double remaining = length(ship.flightTo - ship.pos) / SECTOR_KM;
        drawTextShadow(canvas, 8, UH - 32, fmt("%s  %.2f LY TO GO  [%3.0f%%]", trunc(upper(starNameOf(ship.remote)), 16).c_str(), remaining, u * 100).c_str(), HUD_WHITE, HUD_SHADOW);
    }
    // remote target
    if (ship.hasRemote) {
        double ly = length(ship.remote.pos - ship.pos) / SECTOR_KM;
        std::string rt = fmt("REMOTE: %s %s %.2f LY", trunc(upper(starNameOf(ship.remote)), 16).c_str(), STAR_CLASSES[ship.remote.cls].code, ly);
        drawTextShadow(canvas, UW - 8 - textWidth(rt.c_str()), UH - 40, rt.c_str(), HUD_CYAN, HUD_SHADOW);
        // marker on the remote star
        Mat3 cam = viewBasis();
        Vec3 v = cam * (ship.remote.pos - ship.pos);
        if (v.z > 0) {
            int sx = (int)(UW / 2 + spaceR.proj.f * v.x / v.z / FB_SCALE), sy = (int)(UH / 2 - spaceR.proj.f * v.y / v.z / FB_SCALE);
            if (sx > 4 && sy > 4 && sx < UW - 4 && sy < UH - 4) {
                drawRectRGB(canvas, sx - 4, sy - 4, sx + 4, sy + 4, HUD_CYAN);
            }
        }
    }
    // O3 (R-302): a belt as the local target: its name, the distance to the rock the ship parks at, a bracket on that rock
    if (sys.valid && ship.targetBelt >= 0 && ship.targetBelt < (int)sys.belts.size() && ship.mode != ShipState::VIMANA) {
        BeltRock rk;
        if (beltRockNow(rk)) {
            double d = length(rk.pos - ship.pos);
            std::string lt = fmt("LOCAL: %s %s", trunc(upper(beltNameOf(ship.targetBelt)), 18).c_str(), distanceString(std::max(0.0, d - rk.radiusKm)).c_str());
            drawTextShadow(canvas, UW - 8 - textWidth(lt.c_str()), UH - 24, lt.c_str(), HUD_GREEN, HUD_SHADOW);
            std::string rs = fmt("ROCK %.1f KM ACROSS%s", 2 * rk.radiusKm, ship.parkedBelt >= 0 ? " - NO GROUND FOR THE CAPSULE" : "");
            drawTextShadow(canvas, UW - 8 - textWidth(rs.c_str()), UH - 32, rs.c_str(), HUD_CYAN, HUD_SHADOW);
            Mat3 cam = viewBasis();
            Vec3 v = cam * (rk.pos - ship.pos);
            if (v.z > 0) {
                int sx = (int)(UW / 2 + spaceR.proj.f * v.x / v.z / FB_SCALE), sy = (int)(UH / 2 - spaceR.proj.f * v.y / v.z / FB_SCALE);
                int r = (int)std::max(6.0, spaceR.proj.f * rk.radiusKm / v.z / FB_SCALE + 5);
                if (sx > -r && sy > -r && sx < UW + r && sy < UH + r) {
                    int l = std::max(3, r / 3); uint32_t col = HUD_GREEN;
                    drawLineRGB(canvas, sx - r, sy - r, sx - r + l, sy - r, col); drawLineRGB(canvas, sx - r, sy - r, sx - r, sy - r + l, col);
                    drawLineRGB(canvas, sx + r, sy - r, sx + r - l, sy - r, col); drawLineRGB(canvas, sx + r, sy - r, sx + r, sy - r + l, col);
                    drawLineRGB(canvas, sx - r, sy + r, sx - r + l, sy + r, col); drawLineRGB(canvas, sx - r, sy + r, sx - r, sy + r - l, col);
                    drawLineRGB(canvas, sx + r, sy + r, sx + r - l, sy + r, col); drawLineRGB(canvas, sx + r, sy + r, sx + r, sy + r - l, col);
                }
            } else {
                double ang = std::atan2(-v.y, v.x);
                drawText(canvas, UW / 2 + (int)(std::cos(ang) * 140) - 2, UH / 2 + (int)(std::sin(ang) * 85) - 3, "+", HUD_GREEN);
            }
        }
    }
    // local target
    if (sys.valid && ship.localTarget >= 0 && ship.localTarget < (int)sys.bodies.size() && ship.mode != ShipState::VIMANA) {
        const Body& b = sys.bodies[ship.localTarget];
        double d = length(sys.bodyPos(ship.localTarget, t) - ship.pos);
        std::string lt = fmt("LOCAL: %s %s", trunc(upper(bodyNameOf(ship.localTarget)), 18).c_str(), distanceString(d - b.radiusKm).c_str());
        drawTextShadow(canvas, UW - 8 - textWidth(lt.c_str()), UH - 24, lt.c_str(), HUD_GREEN, HUD_SHADOW);
        if (b.type == PT_COMET) {   // N0-01: how fast it flies and where it is on its plunge
            std::string cm = cometMotionString(ship.localTarget);
            drawTextShadow(canvas, UW - 8 - textWidth(cm.c_str()), UH - 32, cm.c_str(), HUD_CYAN, HUD_SHADOW);
        }
        if (ship.localTarget < (int)spaceR.bodyInfo.size()) {
            const BodyScreenInfo& bi = spaceR.bodyInfo[ship.localTarget];
            if (bi.inFront) {
                int r = (int)std::max(6.0, bi.radiusPx / FB_SCALE + 5);
                int sx = (int)(bi.sx / FB_SCALE), sy = (int)(bi.sy / FB_SCALE);
                if (sx > -r && sy > -r && sx < UW + r && sy < UH + r) {
                    int l = std::max(3, r / 3);
                    uint32_t col = HUD_GREEN;
                    drawLineRGB(canvas, sx - r, sy - r, sx - r + l, sy - r, col); drawLineRGB(canvas, sx - r, sy - r, sx - r, sy - r + l, col);
                    drawLineRGB(canvas, sx + r, sy - r, sx + r - l, sy - r, col); drawLineRGB(canvas, sx + r, sy - r, sx + r, sy - r + l, col);
                    drawLineRGB(canvas, sx - r, sy + r, sx - r + l, sy + r, col); drawLineRGB(canvas, sx - r, sy + r, sx - r, sy + r - l, col);
                    drawLineRGB(canvas, sx + r, sy + r, sx + r - l, sy + r, col); drawLineRGB(canvas, sx + r, sy + r, sx + r, sy + r - l, col);
                    std::string label = trunc(upper(bodyNameOf(ship.localTarget)), 18);
                    int lx = sx + r + 3;
                    if (lx + textWidth(label.c_str()) > UW - 8) lx = sx - r - 3 - textWidth(label.c_str());
                    if (lx < 4) lx = 4;
                    int ly = sy - r;
                    if (ly < 34) ly = sy + r + 3;                 // KI-018: never over the top lines or the warning
                    if (ly > UH - 50) ly = UH - 50;
                    drawText(canvas, lx, ly, label.c_str(), HUD_GREEN);
                }
            } else {
                // off-screen: arrow at the edge
                Mat3 cam = viewBasis();
                Vec3 v = cam * (sys.bodyPos(ship.localTarget, t) - ship.pos);
                double ang = std::atan2(-v.y, v.x);
                int cx = UW / 2 + (int)(std::cos(ang) * 140), cy = UH / 2 + (int)(std::sin(ang) * 85);
                drawText(canvas, cx - 2, cy - 3, "+", HUD_GREEN);
            }
        }
    }
    // radiation warning near hostile stars
    if (sys.valid && ship.mode != ShipState::VIMANA) {
        int cls = sys.star.cls;
        double d = length(sys.star.pos - ship.pos);
        double danger = 0;
        if (cls == STAR_PULSAR) danger = 4e7 / d;
        else if (cls == STAR_WHITE_DWARF) danger = 1.2e7 / d;
        else if (cls == STAR_BLUE_GIANT) danger = 1.5e8 / d;
        else if (cls == STAR_BLUE_WHITE) danger = 3.5e7 / d;   // S-01: the ultraviolet of a hot star, hazardous inside its first orbit
        else if (cls == STAR_NEUTRON) danger = 3e7 / d;   // S-03: the x-ray glare of a neutron star
        else if (cls == STAR_WOLF_RAYET) danger = 5e8 / d;   // S-05: the whole system is inside the hazard (a hull exposure to 5e8 km, high beyond)
        else if (cls == STAR_BLACK_HOLE) danger = 6e7 / d;   // S-06: the x-rays of the disc
        if (danger > 0.4) {
            bool blink = std::fmod(realTime, 1.0) < 0.6;
            const char* msg = danger > 1.0 ? "!! RADIATION HAZARD - HULL EXPOSURE !!" : "RADIATION: HIGH - KEEP DISTANCE";
            if (blink || danger <= 1.0) drawTextCentered(canvas, UW / 2, 32, msg, danger > 1.0 ? HUD_RED : HUD_AMBER);
        }
    }
    // crosshair
    if (ship.targeting || (sys.valid && ship.mode != ShipState::VIMANA)) {
        uint32_t col = ship.targeting ? HUD_CYAN : HUD_DIM;
        int cx = UW / 2, cy = UH / 2, g = ship.targeting ? 3 : 2, l = ship.targeting ? 8 : 4;
        drawLineRGB(canvas, cx - g - l, cy, cx - g, cy, col); drawLineRGB(canvas, cx + g, cy, cx + g + l, cy, col);
        drawLineRGB(canvas, cx, cy - g - l, cx, cy - g, col); drawLineRGB(canvas, cx, cy + g, cx, cy + g + l, col);
        if (ship.targeting) {
            // name the star under the crosshair
            Mat3 cam = viewBasis();
            const Star* best = nullptr; double bestD = 1e9;
            for (const Star& s : nb.stars) {
                Vec3 v = cam * (s.pos - ship.pos);
                if (v.z <= 0) continue;
                double sx = spaceR.proj.f * v.x / v.z, sy = -spaceR.proj.f * v.y / v.z;
                double d = std::sqrt(sx * sx + sy * sy);
                if (d < bestD) { bestD = d; best = &s; }
            }
            if (best && bestD < 14 * FB_SCALE) {
                Star full; starInSector(best->sx, best->sy, best->sz, full, true);
                double ly = length(full.pos - ship.pos) / SECTOR_KM;
                std::string txt = fmt("%s  %s %s  %.2f LY", trunc(upper(starNameOf(full)), 16).c_str(), STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, ly);
                drawTextCentered(canvas, cx, cy + 14, txt.c_str(), HUD_CYAN);
            }
            // the six nearest stars carry a marker with name and distance so a target can be found at a glance
            std::vector<const Star*> near;
            nearestStars(6, near);
            for (size_t k = 0; k < near.size(); k++) {
                const Star* s = near[k];
                if (s == best) continue;
                Vec3 v = cam * (s->pos - ship.pos);
                if (v.z <= 0) continue;
                int sx = (int)(UW / 2 + spaceR.proj.f * v.x / v.z / FB_SCALE), sy = (int)(UH / 2 - spaceR.proj.f * v.y / v.z / FB_SCALE);
                if (sx < 4 || sy < 30 || sx >= UW - 4 || sy >= UH - 44) continue;
                uint32_t mcol = (int)k == targetCycle ? HUD_WHITE : HUD_DIM;
                drawLineRGB(canvas, sx - 4, sy, sx, sy - 4, mcol); drawLineRGB(canvas, sx, sy - 4, sx + 4, sy, mcol);
                drawLineRGB(canvas, sx + 4, sy, sx, sy + 4, mcol); drawLineRGB(canvas, sx, sy + 4, sx - 4, sy, mcol);
                Star full; starInSector(s->sx, s->sy, s->sz, full, true);
                std::string lab = fmt("%s %.1f", trunc(upper(starNameOf(full)), 10).c_str(), length(full.pos - ship.pos) / SECTOR_KM);
                int lx = sx + 6;
                if (lx + textWidth(lab.c_str()) > UW - 4) lx = sx - 6 - textWidth(lab.c_str());
                drawText(canvas, lx, sy - 3, lab.c_str(), mcol);
            }
        }
    }
    if (settings.cabin && !ship.targeting) renderCabinHUD();
    drawCommonHUD();
}

void Game::renderSurfaceScene() {
    surf.render(fb, t, nb.stars, spaceR, fade);
    fb.mush(surf.valid && hasOpaqueDeck(surf.site.gen.type) ? 3 : 2);   // M1-07: the thick air blurs everything
    bool feed = surf.inVehicle() && !surf.chaseCam && !photoMode && surf.cameraOverrideAlt < 0;   // R-204: driving is seen through the nose camera; R-403: flying too
    if (feed) surf.cameraFeed(fb, realTime);
    else if (visionMode != 0) applyVision();
    fb.toRGB(rgbBuf.data(), visionMode == 1 ? 1.25 : 1.0, settings.dither);
}

// M4-10: vision modes are palette remaps applied after the mush, before the RGB conversion.
void Game::applyVision() {
    uint8_t* pal = fb.pal;
    for (int b = 0; b < BANKS; b++)
        for (int i = 0; i < 64; i++) {
            uint8_t* c = pal + (b * 64 + i) * 3;
            double lum = (0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2]);
            switch (visionMode) {
                case 1: {   // radiation visor: brighter, greyer, grainy
                    double g = std::min(255.0, lum * 1.6 + 20);
                    c[0] = (uint8_t)(g * 0.95); c[1] = (uint8_t)g; c[2] = (uint8_t)(g * 0.9);
                    break;
                }
                case 2: {   // supervision: blue scale
                    c[0] = (uint8_t)(lum * 0.25); c[1] = (uint8_t)(lum * 0.55); c[2] = (uint8_t)std::min(255.0, lum * 1.2 + 25);
                    break;
                }
                case 3: {   // infrared: heat shows, the rest is dark; lava and creatures are hot
                    bool hot = (b == 2 && isLavaWorld(surf.site.gen.type)) || b == 3;
                    double v = hot ? std::min(255.0, i * 4.5 + 60) : lum * 0.35;
                    c[0] = (uint8_t)std::min(255.0, v); c[1] = (uint8_t)(hot ? v * 0.6 : v * 0.5); c[2] = (uint8_t)(hot ? v * 0.15 : v * 0.7);
                    break;
                }
                case 4: {   // plant vision: vegetation magenta, everything else grey
                    bool plant = b == 3 && surf.site.gen.type == PT_FELISIAN;
                    if (plant) { c[0] = (uint8_t)std::min(255.0, i * 4.0 + 40); c[1] = (uint8_t)(i * 0.8); c[2] = (uint8_t)std::min(255.0, i * 3.5 + 30); }
                    else { c[0] = c[1] = c[2] = (uint8_t)(lum * 0.6); }
                    break;
                }
            }
        }
    if (visionMode == 1) {
        // grain: a little noise on the intensities
        Rng r((uint64_t)(realTime * 60));
        for (size_t i = 0; i < fb.idx.size(); i += 3) fb.idx[i] = pixI(bankOf(fb.idx[i]), intenOf(fb.idx[i]) + (int)(r.sym(80)));   // +-2.5 shades
    }
}

// M4-02: the sector map: heights, water, the capsule, the buggy, the waypoint, the trail and you.
static const double SECTOR_HALF_W[6] = {1000, 4000, 16000, 64000, 256000, 1024000};   // metres (Game::SECTOR_LEVELS)
static const double SECTOR_DETAIL[6] = {16, 64, 512, 2048, 4096, 16384};

// O1-04: the widest level whose half-width stays within 1.2 radii of the world (a comet has two levels, a planet six)
int Game::sectorMaxLevel() const {
    int m = 0;
    for (int k = 1; k < SECTOR_LEVELS; k++) if (SECTOR_HALF_W[k] <= 1.2 * surf.site.R) m = k;
    return m;
}

void Game::updateSectorMap(const Input& in) {
    if (in.wasPressed(KEY_EQUAL) || in.wasPressed(KEY_UP)) sectorZoom = std::max(0, sectorZoom - 1);
    if (in.wasPressed(KEY_MINUS) || in.wasPressed(KEY_DOWN)) sectorZoom = std::min(sectorMaxLevel(), sectorZoom + 1);
    if (in.wasPressed(KEY_M) && !in.ctrl()) {
        if (surf.hasWaypoint) { surf.hasWaypoint = false; status("WAYPOINT CLEARED", 3); }
        else { surf.hasWaypoint = true; surf.wpX = surf.player.x; surf.wpZ = surf.player.z; status("WAYPOINT SET HERE", 3); }
    }
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_N) || enterKey(in)) state = GameState::SURFACE;
}

void Game::renderSectorMap() {
    const int MW = 200, MH = 160, ox = 8, oy = 14;   // logical frame
    sectorZoom = std::min(sectorZoom, sectorMaxLevel());
    double hw = SECTOR_HALF_W[sectorZoom], mpp = 2 * hw / MW;   // metres per logical pixel
    double px = surf.player.x, pz = surf.player.z;
    const Body& body = sys.bodies[surf.site.body];
    // rebuild the height image when the view moved by a tenth, the zoom changed, or the site was re-initialised or
    // re-anchored (B-303: a new landing near the old one's coordinates showed the old landing's image)
    if (sectorImgZoom != sectorZoom || sectorImgEpoch != surf.siteEpoch || std::fabs(px - sectorImgX) > hw * 0.1 || std::fabs(pz - sectorImgZ) > hw * 0.1) {
        sectorImgZoom = sectorZoom; sectorImgEpoch = surf.siteEpoch; sectorImgX = px; sectorImgZ = pz;
        sectorImg.assign(sectorImgW * sectorImgH, 0);
        double cell = mpp * 2;   // one sample per 2x2 logical pixels
        double detail = SECTOR_DETAIL[sectorZoom];
        std::vector<float> hs(sectorImgW * sectorImgH);
        std::vector<TerrainVertex> tv(sectorImgW * sectorImgH);
        parallelFor(sectorImgH, 8, [&](int y0, int y1) {
            for (int y = y0; y < y1; y++)
                for (int x = 0; x < sectorImgW; x++) {
                    double wx = px + (x - sectorImgW / 2) * cell, wz = pz - (y - sectorImgH / 2) * cell;
                    tv[y * sectorImgW + x] = surf.site.sampleAt(wx, wz, detail);
                    hs[y * sectorImgW + x] = tv[y * sectorImgW + x].h;
                }
        });
        // O2: the same colouring as the landing map (material ramps, exposure rule, hillshade from the north-west), so
        // the map you land by and the map you walk by agree
        MatRamp ramps[MAT_COUNT]; double lf; bool atmo;
        mapRampsFor(body, ramps, lf, atmo);
        auto hAt = [&](int x, int y) { x = clampi(x, 0, sectorImgW - 1); y = clampi(y, 0, sectorImgH - 1); return (double)hs[y * sectorImgW + x]; };
        double sum2 = 0; int cnt = 0;
        for (int y = 0; y < sectorImgH; y++) for (int x = 0; x < sectorImgW; x++) { double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y - 1) - hAt(x, y + 1); sum2 += gx * gx + gy * gy; cnt++; }
        double k = std::min(0.5 / (std::sqrt(sum2 / std::max(1, cnt)) + 1e-9), 0.5 / (0.03 * 2 * cell));   // a 3% grade at most for full shading
        for (int y = 0; y < sectorImgH; y++)
            for (int x = 0; x < sectorImgW; x++) {
                const TerrainVertex& v = tv[y * sectorImgW + x];
                double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y - 1) - hAt(x, y + 1);
                double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);
                int mat = v.material;
                if (v.water > -1e8f && v.h < v.water) mat = MAT_WATER;
                sectorImg[y * sectorImgW + x] = mapColor(ramps, mat, v.albedo, lf, atmo, shade);
            }
    }
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    int S = FB_SCALE;
    double shiftX = (px - sectorImgX) / mpp, shiftZ = (pz - sectorImgZ) / mpp;   // the image is a little stale between rebuilds
    for (int y = 0; y < MH * S; y++)
        for (int x = 0; x < MW * S; x++) {
            int ix = (int)((x / (double)S + shiftX) / 2), iy = (int)((y / (double)S - shiftZ) / 2);
            uint32_t col = (ix >= 0 && iy >= 0 && ix < sectorImgW && iy < sectorImgH) ? sectorImg[iy * sectorImgW + ix] : rgb(10, 10, 10);
            canvas.px[(size_t)(oy * S + y) * FBW + ox * S + x] = col;
        }
    drawRectRGB(canvas, ox - 1, oy - 1, ox + MW, oy + MH, HUD_DIM);
    auto mark = [&](double wx, double wz, int& sx, int& sy) { sx = ox + MW / 2 + (int)((wx - px) / mpp); sy = oy + MH / 2 - (int)((wz - pz) / mpp); return sx >= ox && sy >= oy && sx < ox + MW && sy < oy + MH; };
    int sx, sy;
    if (sectorZoom >= 2) {   // O2: the sector grid with the sectors' names on the wide levels
        // B-402: the grid's step grows with the zoom (1, 2, 5, 10, 15, 30 degrees) so its lines stay at least 24 px apart;
        // the one-degree cells carry the sectors' names when they are wide enough, the coarser lines their latitude and longitude
        auto toPx = [&](double lat, double lon, int& gx, int& gy) { double x, z; surf.site.localAt(StarSystem::bodyFromLatLon(lat, lon), x, z); gx = ox + MW / 2 + (int)((x - px) / mpp); gy = oy + MH / 2 - (int)((z - pz) / mpp); };
        double pxPerDeg = DEG * surf.site.R / mpp;
        static const int STEPS[] = {1, 2, 5, 10, 15, 30};
        int step = 30;
        for (int s : STEPS) if (s * pxPerDeg >= 24) { step = s; break; }
        double latC = surf.env.latDeg * DEG, lonC = surf.env.lonDeg * DEG;
        double span = hw / surf.site.R * 1.1, lonSpan = span / std::max(std::cos(latC), 0.15);
        int j0 = (int)std::floor((latC - span) / DEG / step) * step, j1 = (int)std::ceil((latC + span) / DEG / step) * step;
        int k0 = (int)std::floor((lonC - lonSpan) / DEG / step) * step, k1 = (int)std::ceil((lonC + lonSpan) / DEG / step) * step;
        j0 = std::max(j0, -90); j1 = std::min(j1, 90);
        const uint32_t gridC = rgb(95, 95, 60);
        auto polyline = [&](int n, const std::function<void(int, int&, int&)>& pointAt) {
            int lx = 0, ly = 0; bool have = false;
            for (int i = 0; i <= n; i++) { int gx, gy; pointAt(i, gx, gy); bool in = gx >= ox && gx < ox + MW && gy >= oy && gy < oy + MH; if (have && in) drawLineRGB(canvas, lx, ly, gx, gy, gridC); lx = gx; ly = gy; have = in; }
        };
        for (int j = j0; j <= j1; j += step) polyline(24, [&](int i, int& gx, int& gy) { toPx(j * DEG, (k0 + (k1 - k0) * i / 24.0) * DEG, gx, gy); });
        for (int k = k0; k <= k1; k += step) polyline(24, [&](int i, int& gx, int& gy) { toPx((j0 + (j1 - j0) * i / 24.0) * DEG, k * DEG, gx, gy); });
        if (step == 1 && pxPerDeg >= 46) {
            for (int j = j0; j < j1; j++)
                for (int k = k0; k < k1; k++) {
                    int gx, gy; toPx((j + 0.5) * DEG, (k + 0.5) * DEG, gx, gy);
                    if (gx - 21 < ox || gx + 21 > ox + MW || gy - 3 < oy || gy + 4 > oy + MH) continue;
                    drawText(canvas, gx - 21, gy - 3, sectorName(j + 0.5, wrapAngle((k + 0.5) * DEG) / DEG).c_str(), HUD_DIM);
                }
        } else if (step > 1) {   // the coarse lines' values along the map's edges
            for (int j = j0; j <= j1; j += step) {
                int gx, gy; toPx(j * DEG, lonC, gx, gy);
                if (gy - 3 < oy || gy + 4 > oy + MH) continue;
                drawText(canvas, ox + 2, gy - 3, fmt("%d%s", std::abs(j), j >= 0 ? "N" : "S").c_str(), HUD_DIM);
            }
            for (int k = k0; k <= k1; k += step) {
                int gx, gy; toPx(latC, k * DEG, gx, gy);
                if (gx < ox + 2 || gx + 16 > ox + MW) continue;
                int kw = ((k % 360) + 360) % 360; if (kw > 180) kw -= 360;
                drawText(canvas, gx + 2, oy + 2, fmt("%d%s", std::abs(kw), kw >= 0 ? "E" : "W").c_str(), HUD_DIM);
            }
        }
    }
    for (size_t i = 1; i < surf.trail.size(); i++) if (mark(surf.trail[i].first, surf.trail[i].second, sx, sy)) fillRectRGB(canvas, sx, sy, sx, sy, rgb(230, 200, 120));
    if (mark(surf.capsuleX, surf.capsuleZ, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_AMBER); drawText(canvas, sx + 4, sy - 3, "CAPSULE", HUD_AMBER); }
    if (surf.buggy.deployed && mark(surf.buggy.x, surf.buggy.z, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_CYAN); drawText(canvas, sx + 4, sy - 3, "BUGGY", HUD_CYAN); }
    if (surf.drone.deployed && mark(surf.drone.x, surf.drone.z, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_CYAN); drawText(canvas, sx + 4, sy - 3, "DRONE", HUD_CYAN); }   // R-403
    {   // O6-06, B-401: the sights, but only the ones already found (logged within a kilometre, or named), the nearest 24,
        // a glyph each and the explorer's own name where the map is wide enough: a fresh landing shows an empty map
        std::vector<std::pair<double, const SurfaceView::SiteLandmark*>> known;
        for (const SurfaceView::SiteLandmark& L : surf.landmarks)
            if (guide.landmarksSeen.count(landmarkKey(L.lm))) known.push_back({(L.x - px) * (L.x - px) + (L.z - pz) * (L.z - pz), &L});
        std::sort(known.begin(), known.end(), [](const auto& a, const auto& b2) { return a.first < b2.first; });
        if (known.size() > 24) known.resize(24);
        for (const auto& k : known) {
            const SurfaceView::SiteLandmark& L = *k.second;
            if (!mark(L.x, L.z, sx, sy)) continue;
            drawText(canvas, sx - 2, sy - 3, LANDMARK_KIND_SYMBOLS[L.lm.kind], HUD_WHITE);
            std::string given = landmarkName(L.lm);
            if (!given.empty() && sectorZoom >= 1 && sectorZoom <= 3) drawText(canvas, sx + 5, sy - 3, trunc(given, 14).c_str(), HUD_WHITE);
        }
    }
    if (surf.hasWaypoint && mark(surf.wpX, surf.wpZ, sx, sy)) { drawLineRGB(canvas, sx - 3, sy, sx + 3, sy, HUD_WHITE); drawLineRGB(canvas, sx, sy - 3, sx, sy + 3, HUD_WHITE); drawText(canvas, sx + 4, sy - 3, "WAYPOINT", HUD_WHITE); }
    {   // you, with your heading
        int cx = ox + MW / 2, cy = oy + MH / 2;
        double hx = std::sin(surf.player.yaw), hz = std::cos(surf.player.yaw);
        drawLineRGB(canvas, cx, cy, cx + (int)(hx * 6), cy - (int)(hz * 6), HUD_WHITE);
        drawRectRGB(canvas, cx - 1, cy - 1, cx + 1, cy + 1, HUD_WHITE);
    }
    drawText(canvas, ox + MW + 6, oy, "SECTOR MAP", HUD_AMBER);
    drawText(canvas, ox + MW + 6, oy + 12, fmt("%.0f KM", 2 * hw / 1000).c_str(), HUD_GREEN);
    drawText(canvas, ox + MW + 6, oy + 21, "WIDE", HUD_DIM);
    drawText(canvas, ox + MW + 6, oy + 40, "+/- ZOOM", HUD_DIM);
    drawText(canvas, ox + MW + 6, oy + 49, "M WAYPT", HUD_DIM);
    drawText(canvas, ox + MW + 6, oy + 58, "N/ESC", HUD_DIM);
    drawText(canvas, ox + MW + 6, oy + 67, "CLOSE", HUD_DIM);
    drawText(canvas, ox + MW + 6, oy + 90, fmt("%.1f%s", std::fabs(surf.env.latDeg), surf.env.latDeg >= 0 ? "N" : "S").c_str(), HUD_DIM);
    drawText(canvas, ox + MW + 6, oy + 99, fmt("%.1f%s", std::fabs(surf.env.lonDeg), surf.env.lonDeg >= 0 ? "E" : "W").c_str(), HUD_DIM);
    drawText(canvas, ox + MW + 6, oy + 108, fmt("S %s", sectorName(surf.env.latDeg, surf.env.lonDeg).c_str()).c_str(), HUD_CYAN);   // O2
    drawText(canvas, ox + MW + 6, oy + 126, fmt("ELEV %+.0f", surf.player.y).c_str(), HUD_DIM);
}

void Game::renderSurfaceHUD() {
    if (chartUp && !photoMode && surf.env.skyBrightness < 0.5) {   // C-10: the chart held up to the night sky, the figures kept above the horizon
        Mat3 lf = surf.site.localFrame(t);
        drawChartOverlay(surf.testCamWorld(t), surf.site.worldPos(t, surf.player.x, surf.player.z, 0.002), surf.proj, &lf);
    }
    drawVisor(HUD_DIM);
    const SurfaceEnvironment& e = surf.env;
    // (B-309: the visor glint arcs of N5-02 are gone; their angle spun whenever the sun sat near the screen centre
    // and they wobbled with every head bob. The surface draws a soft veiling glare round the sun instead.)
    std::string l1 = fmt("%s  %.1f%c%s %.1f%c%s", trunc(upper(bodyNameOf(surf.site.body)), 14).c_str(), std::fabs(e.latDeg), CH_DEGREE,
                         e.latDeg >= 0 ? "N" : "S", std::fabs(e.lonDeg), CH_DEGREE, e.lonDeg >= 0 ? "E" : "W");
    drawTextShadow(canvas, 8, 8, l1.c_str(), HUD_GREEN, HUD_SHADOW);
    drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);
    if (recording) drawTextShadow(canvas, UW - 8 - textWidth("REC"), 16, "REC", ((int)(realTime * 2) & 1) ? HUD_RED : HUD_DIM, HUD_SHADOW);   // M6-05
    // compass strip
    {
        int cx = UW / 2, y = 22;
        double yawDeg = wrap2pi(surf.player.yaw) / DEG;
        drawLineRGB(canvas, cx - 60, y + 6, cx + 60, y + 6, HUD_DIM);
        for (int d = -60; d <= 60; d += 15) {
            double az = yawDeg + d;
            int mark = (int)std::floor(az / 15 + 0.5) * 15;
            double off = (mark - yawDeg);
            if (off < -60 || off > 60) continue;
            int x = cx + (int)off;
            int m = ((mark % 360) + 360) % 360;
            if (m % 90 == 0) {
                const char* n = m == 0 ? "N" : (m == 90 ? "E" : (m == 180 ? "S" : "W"));
                drawText(canvas, x - 2, y - 3, n, HUD_GREEN);
            } else drawLineRGB(canvas, x, y + 4, x, y + 6, HUD_DIM);
        }
        drawText(canvas, cx - 2, y + 8, "^", HUD_AMBER);
        std::string hd = fmt("%03.0f", yawDeg);
        drawText(canvas, cx + 66, y - 1, hd.c_str(), HUD_AMBER);
    }
    const Player& pl = surf.player;
    bool seat = surf.inVehicle() && !surf.chaseCam;
    if (surf.lastRange > 0) {   // O1 (B-302): the rangefinder: what the crosshair rests on, how far, and how long it takes to get there
        double spd = surf.inDrone ? 80.0 : (surf.inBuggy ? 32.0 : (surf.site.escapeVelocity < 30 ? 0.3 * surf.site.escapeVelocity : 4.2));   // a fast cruise over open ground, not the 50 m/s top; R-403: a cruise by air
        double secs = surf.lastRange / spd;
        std::string tt = secs < 90 ? fmt("%.0f S", secs) : (secs < 5400 ? fmt("%.0f MIN", secs / 60) : fmt("%.1f H", secs / 3600));
        std::string rg = fmt("%s%s  %s %s", metresString(surf.lastRange).c_str(), surf.lastRangeWater ? " WATER" : "", tt.c_str(), surf.inDrone ? "FLIGHT" : (surf.inBuggy ? "DRIVE" : "WALK"));
        drawTextCentered(canvas, UW / 2, seat ? 44 : 34, rg.c_str(), HUD_CYAN);
        if (const SurfaceView::SiteLandmark* L = surf.landmarkAt(surf.lastRangeX, surf.lastRangeZ))   // O6-06: what the crosshair rests on
            drawTextCentered(canvas, UW / 2, seat ? 52 : 42, landmarkLabel(L->lm).c_str(), HUD_AMBER);
    }
    std::string cap = e.nearCapsule ? "CAPSULE HERE - Q" : fmt("CAPSULE %s %s", metresString(e.capsuleDist).c_str(), compassName(e.capsuleBearing));
    if (!seat) {
        // sun and environment readouts (the buggy's cowl replaces them from the seat)
        std::string sun = fmt("SUN ALT %+.1f%c AZ %03.0f%c  LOCAL %02d:%02d  ELEV %+.0fM", e.sun.altitude / DEG, CH_DEGREE, e.sun.azimuth / DEG, CH_DEGREE,
                              (int)(e.localTime * 24), (int)(std::fmod(e.localTime * 24, 1.0) * 60), e.altitude);
        drawTextShadow(canvas, 8, UH - 40, sun.c_str(), HUD_WHITE, HUD_SHADOW);
        std::string envl = fmt("TEMP %+.0f C  PRESS %.2f ATM", e.temperatureC, e.pressureAtm);
        drawTextShadow(canvas, 8, UH - 32, envl.c_str(), HUD_GREEN, HUD_SHADOW);
        double gg = surf.site.gravity / 9.8;
        std::string envl2 = gg < 0.01 ? fmt("GRAV %.4f G  ESCAPE %.1f M/S", gg, surf.site.escapeVelocity)   // O4: a comet
                                      : fmt("GRAV %.2f G  WIND %.0f KT %s", gg, e.windKnots, e.windKnots > 0.5 ? compassName(e.windDir) : "");
        if (e.ringShadow > 0.15) envl2 += "  RING SHADOW";   // O0-01
        if (e.cometActivity > 0.05) envl2 += fmt("  VENTING %.0f%%", e.cometActivity * 100);   // O4
        if (e.rain > 0.05) envl2 += fmt("  RAIN %.0f%%", e.rain * 100);
        if (e.snow > 0.05) envl2 += fmt("  SNOW %.0f%%", e.snow * 100);
        if (e.hail > 0.05) envl2 += "  HAIL";
        if (e.dust > 0.05) envl2 += fmt("  DUST %.0f%%", e.dust * 100);   // B-206: "DUST STORM" ran into the stamina bar
        if (e.fogBank > 0.3) envl2 += "  FOG";
        if (e.aurora > 0.15 && e.skyBrightness < 0.4) envl2 += e.auroraStorm > 0.7 ? "  AURORA STORM" : "  AURORA";   // R-402
        drawTextShadow(canvas, 8, UH - 24, envl2.c_str(), HUD_GREEN, HUD_SHADOW);
        drawTextShadow(canvas, UW - 8 - textWidth(cap.c_str()), UH - 32, cap.c_str(), e.nearCapsule ? HUD_AMBER : HUD_CYAN, HUD_SHADOW);
        if (surf.nearShard.index >= 0) drawTextShadow(canvas, UW - 8 - textWidth("A SHARD HERE - E"), UH - 40, "A SHARD HERE - E", HUD_AMBER, HUD_SHADOW);   // C-03
        if (surf.nearGrave.k >= 0) { std::string gl = fmt("A GRAVE: %s", upper(surf.nearGrave.hud).c_str()); drawTextShadow(canvas, UW - 8 - textWidth(gl.c_str()), UH - 56, gl.c_str(), HUD_AMBER, HUD_SHADOW); }   // C-12: the stone's name and years
        drawTextShadow(canvas, 8, 16, fmt("SECTOR %s", sectorName(e.latDeg, e.lonDeg).c_str()).c_str(), HUD_DIM, HUD_SHADOW);   // O2 (R-301)
        {   // B-321: the suit's life sensor: the nearest herd or flock within 1.5 km, its bearing and distance (`X` still brackets what is in view)
            double best = 1e9, bx = 0, bz = 0; bool flock = false;
            for (const SurfaceView::Herd& hd : surf.herds) { double dx = hd.cx - pl.x, dz = hd.cz - pl.z, d = std::sqrt(dx * dx + dz * dz); if (d < best) { best = d; bx = dx; bz = dz; flock = false; } }
            for (const Flock& f : surf.flocks) { double a = f.phase + t * f.speed; double dx = f.cx + std::cos(a) * f.radius - pl.x, dz = f.cz + std::sin(a) * f.radius - pl.z, d = std::sqrt(dx * dx + dz * dz); if (d < best) { best = d; bx = dx; bz = dz; flock = true; } }
            if (best < 1500) {
                std::string life = best < 25 ? "LIFE HERE" : fmt("LIFE %s %s%s", metresString(best).c_str(), compassName(wrap2pi(std::atan2(bx, bz))), flock ? " (FLYERS)" : "");
                drawTextShadow(canvas, UW - 8 - textWidth(life.c_str()), UH - 40, life.c_str(), HUD_CYAN, HUD_SHADOW);
            }
        }
        if (!surf.site.roads.empty()) {   // C-09: the old road under the feet and the way it runs
            double d, along, hd; const SiteRoad* rd;
            if (surf.site.roadAt(pl.x, pl.z, 12, d, along, hd, rd) && d < rd->half + 1.5 && roadLeft(rd->id, along, rd->wear) > 0.3) {
                std::string rl = fmt("OLD ROAD %s-%s", compassName(wrap2pi(hd)), compassName(wrap2pi(hd + PI)));
                drawTextShadow(canvas, UW - 8 - textWidth(rl.c_str()), UH - 48, rl.c_str(), HUD_CYAN, HUD_SHADOW);
            }
        }
    }
    if (pl.swimming) drawTextShadow(canvas, UW - 8 - textWidth(pl.underwater ? "DIVING" : "SWIMMING"), UH - 24, pl.underwater ? "DIVING" : "SWIMMING", HUD_CYAN, HUD_SHADOW);
    // M8-02 stamina bar and pulse (hidden while driving or flying)
    if (!surf.inVehicle()) {
        int bx = UW - 68, by = UH - 24;
        if (!pl.swimming) {
            drawRectRGB(canvas, bx, by, bx + 40, by + 5, HUD_DIM);
            int w = (int)(pl.stamina / 100.0 * 39);
            if (w > 0) fillRectRGB(canvas, bx + 1, by + 1, bx + w, by + 4, pl.winded ? HUD_RED : (pl.sprinting ? HUD_AMBER : HUD_GREEN));
            int pulse = (int)(60 + 80 * (1 - pl.stamina / 100.0) + 30 * pl.sprintRamp);
            drawText(canvas, bx + 44, by - 1, fmt("%d", pulse).c_str(), pl.winded ? HUD_RED : HUD_DIM);
        }
        if (pl.jetOn || (pl.altAboveGround > 2 && !pl.onGround)) {
            std::string jet = fmt("JET  ALT %.0f M  VS %+.1f  HEAT %.0f%%%s", pl.altAboveGround, pl.vy, pl.jetHeat, surf.site.escapeVelocity < 30 ? "  CTRL DESCENDS" : "");
            drawTextCentered(canvas, UW / 2, UH - 52, jet.c_str(), pl.jetHeat > 90 ? HUD_RED : HUD_AMBER);
        }
        if (pl.autoWalk > 0) drawText(canvas, 8, UH - 48, fmt("AUTOWALK %d", pl.autoWalk).c_str(), HUD_DIM);
        if (visionMode) { static const char* vn[] = {"", "RADIATION VISOR", "SUPERVISION", "INFRARED", "PLANT VISION"}; drawText(canvas, UW / 2 - textWidth(vn[visionMode]) / 2, UH - 48, vn[visionMode], HUD_CYAN); }
        if (surf.buggy.deployed) drawText(canvas, UW - 8 - textWidth(fmt("BUGGY %.0f M", surf.buggyDist()).c_str()), UH - 40, fmt("BUGGY %.0f M", surf.buggyDist()).c_str(), HUD_DIM);
        if (surf.drone.deployed) drawText(canvas, UW - 8 - textWidth(fmt("DRONE %.0f M", surf.droneDist()).c_str()), UH - 48, fmt("DRONE %.0f M", surf.droneDist()).c_str(), HUD_DIM);   // R-403
    } else if (surf.inDrone && !surf.chaseCam) {
        // R-403: the drone's nose camera interface: the buggy's frame (brackets, the channel with its REC, pan and tilt, the pan
        // gauge, the crosshair) with the flight's numbers: the speed in double-size digits, the height over the ground and the
        // vertical speed, the trip on the left; a bar under the picture for the turn; the heading, the state (LANDED, LIFTING,
        // CLIMB, DESCENT, CEILING, HOVER, LIGHTS ON or the temperature), the capsule and the local time on the right
        const Drone& d = surf.drone;
        const uint32_t fc = HUD_WHITE, fd = HUD_DIM;
        {   // brackets inside the visor's
            int m = 14, l = 26;
            drawLineRGB(canvas, m, m, m + l, m, fc); drawLineRGB(canvas, m, m, m, m + l, fc);
            drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, fc); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, fc);
            drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, fc); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, fc);
            drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, fc); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, fc);
        }
        double look = wrapAngle(pl.yaw - d.heading);
        drawTextShadow(canvas, 8, 16, "CAM 02 DRONE", fc, HUD_SHADOW);
        if ((int)(realTime * 2) & 1) drawTextShadow(canvas, 8 + textWidth("CAM 02 DRONE") + 6, 16, "\x07REC", HUD_RED, HUD_SHADOW);
        drawTextShadow(canvas, 8, 24, fmt("PAN %+03.0f  TILT %+03.0f", look / DEG, pl.pitch / DEG).c_str(), fd, HUD_SHADOW);
        {   // the pan gauge
            int cx = UW / 2, y = 36;
            drawLineRGB(canvas, cx - 35, y, cx + 35, y, fd);
            drawLineRGB(canvas, cx - 35, y - 2, cx - 35, y + 2, fd); drawLineRGB(canvas, cx + 35, y - 2, cx + 35, y + 2, fd); drawLineRGB(canvas, cx, y - 1, cx, y + 1, fd);
            int px = cx + (int)std::lround(look / SurfaceView::CAM_PAN * 35);
            fillRectRGB(canvas, px - 1, y - 2, px + 1, y + 2, HUD_AMBER);
        }
        {   // crosshair
            int cx = UW / 2, cy = UH / 2;
            drawLineRGB(canvas, cx - 10, cy, cx - 4, cy, fd); drawLineRGB(canvas, cx + 4, cy, cx + 10, cy, fd);
            drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, fd); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, fd);
        }
        double kmh = std::fabs(d.speed) * 3.6;
        drawTextShadow(canvas, 8, UH - 48, fmt("%3.0f", kmh).c_str(), fc, HUD_SHADOW, 2);
        drawTextShadow(canvas, 8 + textWidth("000", 2) + 4, UH - 42, d.speed < -0.2 ? "KM/H BACK" : "KM/H", fd, HUD_SHADOW);
        drawTextShadow(canvas, 8, UH - 32, fmt("ALT %.0f M  VS %+.1f", d.altAboveGround, d.vy).c_str(), d.ceiling ? HUD_AMBER : HUD_GREEN, HUD_SHADOW);
        drawTextShadow(canvas, 8, UH - 24, fmt("TRIP %.2f KM", d.odometer / 1000.0).c_str(), HUD_GREEN, HUD_SHADOW);
        {   // the turn bar
            int cx = UW / 2, y = UH - 28;
            drawLineRGB(canvas, cx - 30, y, cx + 30, y, fd); drawLineRGB(canvas, cx, y - 2, cx, y + 2, fd);
            int sx = cx + (int)std::lround(clampd(d.steer / (70 * DEG), -1, 1) * 30);
            fillRectRGB(canvas, sx - 1, y - 3, sx + 1, y + 3, HUD_AMBER);
            drawTextCentered(canvas, cx, y + 5, "TURN", fd);
        }
        std::string hd = fmt("HDG %03.0f %s", wrap2pi(d.heading) / DEG, compassName(wrap2pi(d.heading)));
        drawTextShadow(canvas, UW - 8 - textWidth(hd.c_str()), UH - 48, hd.c_str(), HUD_GREEN, HUD_SHADOW);
        std::string st = d.landed ? (d.rotor > 0.5 ? "LIFTING" : "LANDED") : (d.ceiling ? "CEILING" : (d.vy > 1 ? "CLIMB" : (d.vy < -1 ? "DESCENT" : (std::fabs(d.speed) < 1 ? "HOVER" : (d.lights ? "LIGHTS ON" : fmt("%+.0f C", e.temperatureC))))));
        drawTextShadow(canvas, UW - 8 - textWidth(st.c_str()), UH - 40, st.c_str(), d.landed || d.ceiling || std::fabs(d.vy) > 1 ? HUD_AMBER : HUD_GREEN, HUD_SHADOW);
        drawTextShadow(canvas, UW - 8 - textWidth(cap.c_str()), UH - 32, cap.c_str(), e.nearCapsule ? HUD_AMBER : HUD_CYAN, HUD_SHADOW);
        std::string lt = fmt("%02d:%02d LOCAL", (int)(e.localTime * 24), (int)(std::fmod(e.localTime * 24, 1.0) * 60));
        drawTextShadow(canvas, UW - 8 - textWidth(lt.c_str()), UH - 24, lt.c_str(), fd, HUD_SHADOW);
    } else if (!surf.chaseCam) {
        // R-204: the nose camera's interface. The driver sees the picture from the pod on the nose, framed by the
        // camera's own overlay: bigger brackets, the channel with a blinking REC, the pod's pan and tilt with a pan
        // gauge under the compass, a crosshair, the speed, gear and trip on the left, the heading, the state, the
        // capsule and the local time on the right, and the steering bar under the picture (right is right)
        const Buggy& b = surf.buggy;
        const uint32_t fc = HUD_WHITE, fd = HUD_DIM;
        {   // brackets inside the visor's
            int m = 14, l = 26;
            drawLineRGB(canvas, m, m, m + l, m, fc); drawLineRGB(canvas, m, m, m, m + l, fc);
            drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, fc); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, fc);
            drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, fc); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, fc);
            drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, fc); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, fc);
        }
        double look = wrapAngle(pl.yaw - b.heading);
        drawTextShadow(canvas, 8, 16, "CAM 01 NOSE", fc, HUD_SHADOW);
        if ((int)(realTime * 2) & 1) drawTextShadow(canvas, 8 + textWidth("CAM 01 NOSE") + 6, 16, "\x07REC", HUD_RED, HUD_SHADOW);
        drawTextShadow(canvas, 8, 24, fmt("PAN %+03.0f  TILT %+03.0f", look / DEG, pl.pitch / DEG).c_str(), fd, HUD_SHADOW);
        {   // the pan gauge: the pod's 70 deg limits and where it points
            int cx = UW / 2, y = 36;
            drawLineRGB(canvas, cx - 35, y, cx + 35, y, fd);
            drawLineRGB(canvas, cx - 35, y - 2, cx - 35, y + 2, fd); drawLineRGB(canvas, cx + 35, y - 2, cx + 35, y + 2, fd); drawLineRGB(canvas, cx, y - 1, cx, y + 1, fd);
            int px = cx + (int)std::lround(look / SurfaceView::CAM_PAN * 35);
            fillRectRGB(canvas, px - 1, y - 2, px + 1, y + 2, HUD_AMBER);
        }
        {   // crosshair
            int cx = UW / 2, cy = UH / 2;
            drawLineRGB(canvas, cx - 10, cy, cx - 4, cy, fd); drawLineRGB(canvas, cx + 4, cy, cx + 10, cy, fd);
            drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, fd); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, fd);
        }
        double kmh = std::fabs(b.speed) * 3.6;
        drawTextShadow(canvas, 8, UH - 48, fmt("%3.0f", kmh).c_str(), fc, HUD_SHADOW, 2);
        drawTextShadow(canvas, 8 + textWidth("000", 2) + 4, UH - 42, "KM/H", fd, HUD_SHADOW);
        drawTextShadow(canvas, 8, UH - 32, fmt("GEAR %d%s", b.gear + 1, b.speed < -0.2 ? "  REVERSE" : "").c_str(), HUD_GREEN, HUD_SHADOW);
        if (!surf.site.roads.empty()) {   // C-09: the old road under the wheels and the way it runs
            double d, along, hd; const SiteRoad* rd;
            if (surf.site.roadAt(b.x, b.z, 12, d, along, hd, rd) && d < rd->half + 1.5 && roadLeft(rd->id, along, rd->wear) > 0.3)
                drawTextShadow(canvas, 8, UH - 58, fmt("OLD ROAD %s-%s", compassName(wrap2pi(hd)), compassName(wrap2pi(hd + PI))).c_str(), HUD_CYAN, HUD_SHADOW);
        }
        drawTextShadow(canvas, 8, UH - 24, fmt("TRIP %.2f KM", b.odometer / 1000.0).c_str(), HUD_GREEN, HUD_SHADOW);
        {   // the steering bar
            int cx = UW / 2, y = UH - 28;
            drawLineRGB(canvas, cx - 30, y, cx + 30, y, fd); drawLineRGB(canvas, cx, y - 2, cx, y + 2, fd);
            int sx = cx + (int)std::lround(clampd(b.steer / (30 * DEG), -1, 1) * 30);
            fillRectRGB(canvas, sx - 1, y - 3, sx + 1, y + 3, HUD_AMBER);
            drawTextCentered(canvas, cx, y + 5, "STEER", fd);
        }
        std::string hd = fmt("HDG %03.0f %s", wrap2pi(b.heading) / DEG, compassName(wrap2pi(b.heading)));
        drawTextShadow(canvas, UW - 8 - textWidth(hd.c_str()), UH - 48, hd.c_str(), HUD_GREEN, HUD_SHADOW);
        std::string st = b.airborne ? "AIRBORNE" : (b.skid > 0.3 ? "SKID" : (b.brake ? "BRAKE" : (b.lights ? "LIGHTS ON" : fmt("%+.0f C", e.temperatureC))));
        drawTextShadow(canvas, UW - 8 - textWidth(st.c_str()), UH - 40, st.c_str(), b.airborne || b.skid > 0.3 || b.brake ? HUD_AMBER : HUD_GREEN, HUD_SHADOW);
        drawTextShadow(canvas, UW - 8 - textWidth(cap.c_str()), UH - 32, cap.c_str(), e.nearCapsule ? HUD_AMBER : HUD_CYAN, HUD_SHADOW);
        std::string lt = fmt("%02d:%02d LOCAL", (int)(e.localTime * 24), (int)(std::fmod(e.localTime * 24, 1.0) * 60));
        drawTextShadow(canvas, UW - 8 - textWidth(lt.c_str()), UH - 24, lt.c_str(), fd, HUD_SHADOW);
    }
    // M4-05 creature highlight
    if (surf.highlightUntil > realTime) {
        for (const Critter& c : surf.critters) {
            double sx, sy;
            if (!surf.projectPoint(c.x, surf.site.groundHeight(c.x, c.z) + 0.6 * c.size, c.z, sx, sy)) continue;
            int lx = (int)(sx / FB_SCALE), ly = (int)(sy / FB_SCALE);
            if (lx < 4 || ly < 30 || lx > UW - 4 || ly > UH - 50) continue;
            drawRectRGB(canvas, lx - 4, ly - 4, lx + 4, ly + 4, HUD_CYAN);
            double d = std::sqrt((c.x - pl.x) * (c.x - pl.x) + (c.z - pl.z) * (c.z - pl.z));
            const Species* sp = c.species >= 0 && c.species < (int)surf.bestiary.species.size() ? &surf.bestiary.species[c.species] : nullptr;   // N3-06
            drawText(canvas, lx + 6, ly - 3, fmt("%s %s %.0fM", sp ? trunc(upper(sp->name), 10).c_str() : "CREATURE", CRITTER_STATE_NAMES[clampi(c.state, 0, 7)], d).c_str(), HUD_CYAN);
        }
        for (const Flock& f : surf.flocks) {
            double a = f.phase + t * f.speed;
            double fx = f.cx + std::cos(a) * f.radius, fz = f.cz + std::sin(a) * f.radius;
            double sx, sy;
            if (!surf.projectPoint(fx, surf.site.groundHeight(fx, fz) + f.alt, fz, sx, sy)) continue;
            int lx = (int)(sx / FB_SCALE), ly = (int)(sy / FB_SCALE);
            if (lx < 4 || ly < 30 || lx > UW - 4 || ly > UH - 50) continue;
            drawRectRGB(canvas, lx - 5, ly - 5, lx + 5, ly + 5, HUD_CYAN);
            const Species* sp = f.species >= 0 && f.species < (int)surf.bestiary.species.size() ? &surf.bestiary.species[f.species] : nullptr;
            drawText(canvas, lx + 7, ly - 3, fmt("%d %s%s", f.n, sp ? trunc(upper(sp->name), 10).c_str() : "FLYERS", f.landed > 0 ? (f.perched ? " (PERCHED)" : " (RESTING)") : "").c_str(), HUD_CYAN);
        }
    }
    if (surf.hasWaypoint) {
        double dx = surf.wpX - pl.x, dz = surf.wpZ - pl.z;
        std::string wp = fmt("WAYPOINT %s %s", metresString(std::sqrt(dx * dx + dz * dz)).c_str(), compassName(wrap2pi(std::atan2(dx, dz))));
        drawTextShadow(canvas, UW - 8 - textWidth(wp.c_str()), 16, wp.c_str(), HUD_CYAN, HUD_SHADOW);
    }
    drawCommonHUD();
}
