// Scene rendering entry points and the HUD overlays for space and surface.
#include "game.h"
#include "galaxy/plan.h"
#include <array>
#include "ui.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cmath>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

void Game::renderSpace() {
    spaceR.proj = telescopeProj();   // W-06: the telescope's magnification (the setting's field alone when it is stowed)
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
        c.flightFrom = ship.flightFrom; c.flightTo = ship.flightTo; c.flightT = ship.flightT; c.flightDur = ship.flightDur;   // G-07: the band built ahead along the line
    }
    c.bankBodyA = a; c.bankBodyB = b;
    c.starIntensity = (ship.targeting || fieldAmp || radar.on || tele.on) ? 1.5 : 1.0;   // field amplification while aiming or by choice; C-07: the radar camera sees the stars amplified; W-06: the telescope gathers light
    c.detailBody = titleCam ? -1 : telescopePlateBody();   // W-06: the eyepiece's body gets the planet function at its footprint (the plate)
    c.detailBudget = (wantsScreenshot || telePlateFinish) ? 0 : TELE_PLATE_BUDGET;   // a photo reads the plate out whole
    telePlateFinish = false;
    spaceR.render(fb, c);
    if (settings.cabin && !titleCam && (state != GameState::LANDING_MAP || ax.on) && !radar.on && !tele.on) { cabinPalette(); drawDroneLights(); drawCabin(); }   // R-419: auto-explore's landing map is the cabin still   // M2; C-07: the radar camera is outside the cabin; W-06: the eye is at the eyepiece; C-18: the drones outside the windows
    if (radar.on && !titleCam) radarCameraFeed(fb);   // C-07
    fb.mush(2);
    fb.toRGB(rgbBuf.data(), 1.0 + 1.3 * std::max(0.0, arrivalFlash), settings.dither);   // M1-10 arrival flash
}

// G-07: the galaxy page in a flight draws no scene, so the band's keyframes are kept building here (a share a frame), or closing the
// page would build one whole at once
void Game::keepFlightBand() {
    if (ship.mode != ShipState::VIMANA) return;
    SpaceContext c;
    c.vimana = true; c.flightFrom = ship.flightFrom; c.flightTo = ship.flightTo; c.flightT = ship.flightT; c.flightDur = ship.flightDur;
    spaceR.flightBand(c);
}

void Game::drawVisor(uint32_t col) {
    int m = 6, l = 14;
    drawLineRGB(canvas, m, m, m + l, m, col); drawLineRGB(canvas, m, m, m, m + l, col);
    drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, col); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, col);
    drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, col); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, col);
    drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, col); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, col);
}

void Game::drawCommonHUD(bool warp) {
    if (realTime < statusUntil && !statusMsg.empty()) {
        blendRectRGB(canvas, 0, UH - 12, UW - 1, UH - 1, rgb(0, 0, 0), 120);
        drawTextCentered(canvas, UW / 2, UH - 10, statusMsg.c_str(), HUD_AMBER);
    } else if (almanacRunning) {   // W-01: the run to an event holds the status row: what it runs to, what is left of the wait, the warp
        blendRectRGB(canvas, 0, UH - 12, UW - 1, UH - 1, rgb(0, 0, 0), 120);
        std::string s = fmt("TO %s  %s  X%.0f - T STOPS", trunc(almanacRunLabel, 26).c_str(), countdownString(std::max(0.0, almanacRunT - t)).c_str(), timeWarp);
        drawTextCentered(canvas, UW / 2, UH - 10, s.c_str(), HUD_CYAN);
    }
    if (timeWarp > 1 && warp) drawText(canvas, UW - 8 - textWidth(fmt("TIME X%.0f", timeWarp).c_str()), 24, fmt("TIME X%.0f", timeWarp).c_str(), HUD_CYAN);
}

void Game::renderSpaceHUD() {
    if (radar.on) { renderRadarCamera(); drawCommonHUD(); return; }   // C-07: the radar camera's own frame
    if (tele.on) { renderTelescopeHUD(); drawCommonHUD(); return; }   // W-06: the telescope's own frame (the user's review: an instrument of its own, as the radar)
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
    if (player.playing >= 0 && player.playing < (int)player.items.size()) {   // C-11: the ship's player, under the system's lines
        const uint32_t col = player.held || player.paused || player.waiting ? HUD_DIM : HUD_GREEN;
        drawRecordKindGlyph(canvas, 8, 24, player.items[player.playing].music ? RC_MUSIC : RC_TEXT, col);
        drawTextShadow(canvas, 17, 24, playerHudLine().c_str(), col, HUD_SHADOW);
    }
    // mode
    const char* modeName = "STANDBY";
    if (ship.mode == ShipState::VIMANA) modeName = "VIMANA FLIGHT";
    else if (ship.mode == ShipState::APPROACH) modeName = "FINE APPROACH";
    else if (ship.mode == ShipState::PARKED) modeName = ship.parkedBelt >= 0 ? "IN THE BELT" : (ship.orbiting ? "ORBIT" : "CHASE");   // O3
    drawTextShadow(canvas, 8, UH - 24, modeName, HUD_AMBER, HUD_SHADOW);
    if (probe.active && probeHere()) {   // X-01: the probe falling (or its last frame waiting) while the explorer is in the cabin
        std::string pl = probe.endClock >= 0 ? "PROBE: SIGNAL LOST - C SHOWS ITS LAST FRAME"
                                             : fmt("PROBE: %s  %.2f ATM - C WATCHES", probeStageLabel(probe.clock).c_str(), probeBarAt(probe.flight, probe.clock) * 0.98692);
        drawTextShadow(canvas, 8, UH - 32, pl.c_str(), probe.endClock >= 0 ? HUD_RED : HUD_CYAN, HUD_SHADOW);
    }
    if (bay.out && droneBayHere() && !(probe.active && probeHere())) {   // C-18: the drones down on the world
        int found = 0; for (const DroneRun& R : bay.run) for (const DroneFindRec& f : R.finds) if (f.aboard || f.t <= bay.clock) found++;
        int docked = 0; for (const DroneRun& R : bay.run) if (R.docked) docked++;
        const double in = droneBackIn(); const int s = (int)std::ceil(std::max(0.0, in));
        const std::string when = in >= 0 ? fmt("DRONES BACK IN %d:%02d", s / 60, s % 60) : std::string("DRONES AWAY");   // the user's two minutes of searching: when they are back
        const std::string dl = docked ? fmt("%s - %d FOUND, %d ABOARD", when.c_str(), found, docked) : fmt("%s - %d FOUND", when.c_str(), found);
        drawTextShadow(canvas, 8, UH - 32, dl.c_str(), HUD_CYAN, HUD_SHADOW);
    }
    if (ship.mode == ShipState::VIMANA) {
        double u = clampd(ship.flightT / ship.flightDur, 0, 1);
        double remaining = length(ship.flightTo - ship.pos) / SECTOR_KM;
        drawTextShadow(canvas, 8, UH - 32, fmt("%s  %.2f LY TO GO  [%3.0f%%]", trunc(upper(starNameOf(ship.remote)), 16).c_str(), remaining, u * 100).c_str(), HUD_WHITE, HUD_SHADOW);
    }
    // remote target
    if (ship.hasRemote) {
        double ly = length(ship.remote.pos - ship.pos) / SECTOR_KM;
        std::string rt = fmt("REMOTE: %s %s %.2f LY%s", trunc(upper(starNameOf(ship.remote)), 16).c_str(), ship.remote.cls == STAR_ROGUE ? "NO STAR" : STAR_CLASSES[ship.remote.cls].code, ly, guide.visited.count(starKeyOf(ship.remote)) ? "  VISITED" : "");   // R-407: the stars already seen; D-02: a drifter no class
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
                    if (!ship.targeting) drawText(canvas, lx, ly, label.c_str(), HUD_GREEN);   // R-407: the aim's lines hold the bottom while aiming
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
            if (best && bestD < 14 * FB_SCALE && jumpDigits.empty()) {
                Star full; starInSector(best->sx, best->sy, best->sz, full, true);
                double ly = length(full.pos - ship.pos) / SECTOR_KM;
                std::string txt = fmt("%s  %s %s  %.2f LY%s", trunc(upper(starNameOf(full)), 16).c_str(), STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, ly, guide.visited.count(starKeyOf(full)) ? "  VISITED" : "");
                drawTextCentered(canvas, cx, cy + 14, txt.c_str(), HUD_CYAN);
            }
            // the six nearest stars carry a marker with their distance so a target can be found at a glance; R-407 (the user's "green
            // marker stars as unknown ... it's annoying"): six labels of UNKNOWN said nothing, so a star's label is its name only once it
            // has one, and VISITED in amber where the ship has been (the radar's loops led back to systems already named)
            std::vector<const Star*> near;
            if (jumpDigits.empty()) nearestStars(6, near);
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
                const bool visited = guide.visited.count(starKeyOf(full)) > 0;
                const std::string lab = aimStarLabel(full);
                int lx = sx + 6;
                if (lx + textWidth(lab.c_str()) > UW - 4) lx = sx - 6 - textWidth(lab.c_str());
                drawText(canvas, lx, sy - 3, lab.c_str(), visited && (int)k != targetCycle ? HUD_AMBER : mcol);
            }
            if (jumpDigits.empty()) { blendRectRGB(canvas, cx - 148, UH - 55, cx + 148, UH - 44, rgb(0, 0, 0), 150); drawTextCentered(canvas, cx, UH - 53, "N NEXT STAR  ENTER LOCKS  DIGITS JUMP  R CANCELS", HUD_DIM); }
            else {   // R-407: the jump being typed: its length, the way it runs in the galaxy and where its end lies
                const Vec3 dir = aimDirection(), end = ship.pos / SECTOR_KM + dir * std::atof(jumpDigits.c_str());
                const int64_t ex = (int64_t)std::floor(end.x), ey = (int64_t)std::floor(end.y), ez = (int64_t)std::floor(end.z);
                const double fromCore = std::sqrt((end.x - GALAXY_CENTRE_SX) * (end.x - GALAXY_CENTRE_SX) + (end.z - GALAXY_CENTRE_SZ) * (end.z - GALAXY_CENTRE_SZ));
                blendRectRGB(canvas, cx - 128, UH - 72, cx + 128, UH - 41, rgb(0, 0, 0), 150);
                drawTextCentered(canvas, cx, UH - 69, fmt("JUMP %s LY %s", jumpDigits.c_str(), galacticHeading(ship.pos / SECTOR_KM, dir).c_str()).c_str(), HUD_WHITE);
                drawTextCentered(canvas, cx, UH - 60, fmt("END: %s - %.0f LY FROM THE CORE", REGION_NAMES[galaxyRegion(ex, ey, ez)], fromCore).c_str(), HUD_CYAN);
                drawTextCentered(canvas, cx, UH - 51, "ENTER JUMPS  BACKSPACE  R CANCELS", HUD_DIM);
            }
        }
    }
    if (settings.cabin && !ship.targeting) renderCabinHUD();
    drawCommonHUD();
}

// R-407: a marker's label in the aim: the star's name once it has one, else its distance alone; VISITED where the ship has been
std::string Game::aimStarLabel(const Star& full) const {
    const double ly = length(full.pos - ship.pos) / SECTOR_KM;
    std::string lab = starNamed(full) ? fmt("%s %.1f", trunc(upper(starNameOf(full)), 10).c_str(), ly) : fmt("%.1f LY", ly);
    if (guide.visited.count(starKeyOf(full))) lab += " VISITED";
    return lab;
}

void Game::renderSurfaceScene() {
    surf.floodlights = state == GameState::DESCENT || state == GameState::ASCENT;   // D-04: the capsule's, on its way down and up
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
                    bool hot = (b == 2 && surf.lavaGlow) || b == 3;
                    double v = hot ? std::min(255.0, i * 4.5 + 60) : lum * 0.35;
                    c[0] = (uint8_t)std::min(255.0, v); c[1] = (uint8_t)(hot ? v * 0.6 : v * 0.5); c[2] = (uint8_t)(hot ? v * 0.15 : v * 0.7);
                    break;
                }
                case 4: {   // plant vision: vegetation magenta, everything else grey
                    bool plant = b == 3 && kindHas(surf.site.gen.type, KT_FLORA);
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

// M4-02: the sector map: heights, water, the capsule, the buggy, the waypoint and you (R-414: M4-02's trail of where you walked is gone;
// the user: "remove these lines that show what routes you traveled - they seem to be a good idea but they confuse you a lot").
static const double SECTOR_HALF_W[6] = {1000, 4000, 16000, 64000, 256000, 1024000};   // metres (Game::SECTOR_LEVELS)
static const double SECTOR_DETAIL[6] = {16, 64, 512, 2048, 4096, 16384};

// O1-04: the widest level whose half-width stays within 1.2 radii of the world (a comet has two levels, a planet six)
int Game::sectorMaxLevel() const {
    int m = 0;
    for (int k = 1; k < SECTOR_LEVELS; k++) if (SECTOR_HALF_W[k] <= 1.2 * surf.site.R) m = k;
    return m;
}

// R-408: the map's cursor (the user: "make the surface N map more interactive so you can place arbitrary marker on something YOU
// see as a landmark and name it"): the arrows (Shift faster) and the mouse move it within the frame, C or Space bring it back to the
// explorer; Enter marks the place under it (the name's entry; on a mark: renames it), Delete twice removes the mark under it, M puts
// the waypoint there (on the waypoint: clears it), Tab goes to the next mark; +/- and the wheel zoom (the arrows did), R and Shift+R
// work the scanner as on the ground. B-412: Shift+Enter or Shift+click on a mark shows it over the view (the one shown), again hides it
void Game::updateSectorMap(const Input& in) {
    if (in.wasPressed(KEY_EQUAL) || in.wheel > 0) sectorZoom = std::max(0, sectorZoom - 1);
    if (in.wasPressed(KEY_MINUS) || in.wheel < 0) sectorZoom = std::min(sectorMaxLevel(), sectorZoom + 1);
    const double hw = SECTOR_HALF_W[std::min(sectorZoom, sectorMaxLevel())], mpp = 2 * hw / 200, hh = hw * 0.8;
    const double px = surf.player.x, pz = surf.player.z;
    const double sp = (in.shift() ? 3.0 : 1.0) * 0.6 * hw * lastRealDt;
    if (in.isDown(KEY_LEFT)) mapCurX -= sp;
    if (in.isDown(KEY_RIGHT)) mapCurX += sp;
    if (in.isDown(KEY_UP)) mapCurZ += sp;
    if (in.isDown(KEY_DOWN)) mapCurZ -= sp;
    mapCurX += in.mouseDx * 0.5 * settings.mouseSensitivity * mpp;
    mapCurZ -= in.mouseDy * 0.5 * settings.mouseSensitivity * mpp;
    if (in.wasPressed(KEY_C) || in.wasPressed(KEY_SPACE)) { mapCurX = px; mapCurZ = pz; }
    if (in.wasPressed(KEY_TAB)) {   // the next mark in the frame, nearest the explorer first
        std::vector<std::pair<double, const SiteMark*>> in_;
        for (const SiteMark& m : marksHere()) if (std::fabs(m.x - px) <= hw && std::fabs(m.z - pz) <= hh) in_.push_back({std::hypot(m.x - px, m.z - pz), &m});
        std::sort(in_.begin(), in_.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        if (in_.empty()) status("NO MARK IN THE FRAME - ZOOM OUT (-)", 3);
        else {
            size_t k = 0;
            for (size_t i = 0; i < in_.size(); i++) if (in_[i].second->index == mapTabMark) { k = (i + 1) % in_.size(); break; }
            mapTabMark = in_[k].second->index; mapCurX = in_[k].second->x; mapCurZ = in_[k].second->z;
        }
        audio.beep = 4;
    }
    mapCurX = clampd(mapCurX, px - hw, px + hw); mapCurZ = clampd(mapCurZ, pz - hh, pz + hh);
    const int onMark = markNear(mapCurX, mapCurZ, 5 * mpp);
    if ((enterKey(in) || in.mousePressed[0]) && in.shift()) {   // B-412
        if (onMark >= 0) showMark(onMark);
        else { status("NO MARK UNDER THE CURSOR", 2); audio.beep = 3; }
    } else if (enterKey(in) || in.mousePressed[0]) { if (onMark >= 0) beginRenameMark(onMark); else beginMark(mapCurX, mapCurZ); return; }
    if (in.wasPressed(KEY_DELETE) || in.wasPressed(KEY_BACKSPACE)) {
        if (onMark >= 0 && onMark == mapDelMark && realTime < mapDelUntil) { removeMark(onMark); mapDelMark = -1; }
        else if (onMark >= 0) { mapDelMark = onMark; mapDelUntil = realTime + 3; status(fmt("REMOVE %s? DELETE AGAIN", trunc(markLabel(onMark), 24).c_str()), 3); audio.beep = 3; }
        else { status("NO MARK UNDER THE CURSOR", 2); audio.beep = 3; }
    }
    if (in.wasPressed(KEY_M) && !in.ctrl()) {
        if (surf.hasWaypoint && std::hypot(surf.wpX - mapCurX, surf.wpZ - mapCurZ) < 5 * mpp) { surf.hasWaypoint = false; status("WAYPOINT CLEARED", 3); }
        else { surf.hasWaypoint = true; surf.wpX = mapCurX; surf.wpZ = mapCurZ; status(std::hypot(mapCurX - px, mapCurZ - pz) < 2 * mpp ? std::string("WAYPOINT SET HERE") : fmt("WAYPOINT SET %s AWAY", metresString(std::hypot(mapCurX - px, mapCurZ - pz)).c_str()), 3); }
        audio.beep = 4;
    }
    if (in.wasPressed(KEY_R) && !in.ctrl()) { if (in.shift()) scanToggleSkip(); else scanCycle(); }
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_N)) state = GameState::SURFACE;
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
        const bool survey = sys.sunless();   // D-02: the survey look of a world with no sun, by the frame's heights
        double hLo = 0, hHi = 1; if (survey) surveyRange(hs.data(), hs.size(), hLo, hHi);
        for (int y = 0; y < sectorImgH; y++)
            for (int x = 0; x < sectorImgW; x++) {
                const TerrainVertex& v = tv[y * sectorImgW + x];
                double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y - 1) - hAt(x, y + 1);
                double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);
                int mat = v.material;
                if (v.water > -1e8f && v.h < v.water && !(v.material == MAT_SAND && surf.site.seaVertex(v))) mat = MAT_WATER;   // Q-02: a tidal flat its wet sand
                sectorImg[y * sectorImgW + x] = mapColor(ramps, mat, v.albedo, lf, atmo, shade, survey ? surveyHeight(v.h, hLo, hHi) : -1);
            }
        if (surf.site.gen.tideRangeM > 0 && surf.site.hasWater) {   // Q-02: the tide lines, the high water's pale along the flat's landward edge, the low water's dark
            const double S = surf.site.seaLevel;
            for (int y = 0; y + 1 < sectorImgH; y++)
                for (int x = 0; x + 1 < sectorImgW; x++) {
                    const double Rl = tideRangeAt(surf.site.gen, surf.site.unitAt(px + (x - sectorImgW / 2) * cell, pz - (y - sectorImgH / 2) * cell));
                    const double hi = S + 0.5 * Rl, lo = S - 0.5 * Rl, h0 = hAt(x, y), hr = hAt(x + 1, y), hd = hAt(x, y + 1);
                    if ((h0 - hi) * (hr - hi) < 0 || (h0 - hi) * (hd - hi) < 0) sectorImg[y * sectorImgW + x] = rgb(232, 226, 204);
                    else if ((h0 - lo) * (hr - lo) < 0 || (h0 - lo) * (hd - lo) < 0) sectorImg[y * sectorImgW + x] = rgb(40, 62, 120);
                }
        }
    }
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    int S = FB_SCALE;
    double shiftX = (px - sectorImgX) / mpp, shiftZ = (pz - sectorImgZ) / mpp;   // the image is a little stale between rebuilds
    // F-05: the image's column of every pixel column and its row of every pixel row, by the per-pixel expressions, then the rows on the pool
    static std::vector<int> ixs, iys;
    ixs.resize(MW * S); iys.resize(MH * S);
    for (int x = 0; x < MW * S; x++) ixs[x] = (int)((x / (double)S + shiftX) / 2);
    for (int y = 0; y < MH * S; y++) iys[y] = (int)((y / (double)S - shiftZ) / 2);
    parallelFor(MH * S, 64, [&](int yb, int ye) {
        for (int y = yb; y < ye; y++)
            for (int x = 0; x < MW * S; x++) {
                int ix = ixs[x], iy = iys[y];
                uint32_t col = (ix >= 0 && iy >= 0 && ix < sectorImgW && iy < sectorImgH) ? sectorImg[iy * sectorImgW + ix] : rgb(10, 10, 10);
                canvas.px[(size_t)(oy * S + y) * FBW + ox * S + x] = col;
            }
    });
    if (surf.localFront.on) {   // W-03: the nearest front's rain line across the map and its band behind it, hatched
        const LocalFront& F = surf.localFront;
        const double s0 = FRONT_RAIN_END * F.width;
        const uint32_t lineC = F.dust ? HUD_AMBER : HUD_CYAN;
        for (int y = 0; y < MH; y++)
            for (int x = 0; x < MW; x++) {
                double sA = F.ahead(px + (x - MW / 2 + 0.5) * mpp, pz - (y - MH / 2 + 0.5) * mpp);
                if (std::fabs(sA) <= 0.6 * mpp) fillRectRGB(canvas, ox + x, oy + y, ox + x, oy + y, lineC);
                else if (sA < 0 && sA > s0 && ((x + y) & 3) == 0) blendRectRGB(canvas, ox + x, oy + y, ox + x, oy + y, lineC, 110);
            }
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
    if (mark(surf.capsuleX, surf.capsuleZ, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_AMBER); drawText(canvas, sx + 4, sy - 3, "CAPSULE", HUD_AMBER); }
    if (surf.buggy.deployed && mark(surf.buggy.x, surf.buggy.z, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_CYAN); drawText(canvas, sx + 4, sy - 3, "BUGGY", HUD_CYAN); }
    if (surf.drone.deployed && mark(surf.drone.x, surf.drone.z, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_CYAN); drawText(canvas, sx + 4, sy - 3, "DRONE", HUD_CYAN); }   // R-403
    if (scanMode >= 0) {   // R-408: the scanner's echoes of its mode where it hears them (the marked left out unless taken in), the nearest 30
        std::vector<std::pair<double, const SurfaceView::ScanEcho*>> heard;
        for (const SurfaceView::ScanEcho& e : surf.echoes) if (e.lm.kind == scanMode && !(scanSkipMarked && echoMarked(e))) heard.push_back({(e.x - px) * (e.x - px) + (e.z - pz) * (e.z - pz), &e});
        std::sort(heard.begin(), heard.end(), [](const auto& a, const auto& b2) { return a.first < b2.first; });
        if (heard.size() > 30) heard.resize(30);
        for (size_t i = 0; i < heard.size(); i++) {
            double hx, hz; echoHeardAt(*heard[i].second, hx, hz);
            if (!mark(hx, hz, sx, sy)) continue;
            if (i == 0) { drawRectRGB(canvas, sx - 3, sy - 3, sx + 3, sy + 3, HUD_CYAN); fillRectRGB(canvas, sx - 1, sy - 1, sx + 1, sy + 1, HUD_CYAN); }
            else { fillRectRGB(canvas, sx - 1, sy - 1, sx + 1, sy + 1, rgb(0, 0, 0)); const Landmark& L = heard[i].second->lm; const bool big = L.kind == LM_RUIN ? (L.sub == SC_VILLAGE || L.sub == SC_TOWN || L.sub >= SC_CITY) : L.radiusM > 400; fillRectRGB(canvas, sx, sy, sx + (big ? 1 : 0), sy + (big ? 1 : 0), HUD_CYAN); }   // a bigger echo a bigger dot (a village, a town, T-01's cities)
        }
    }
    const int onMark = markNear(mapCurX, mapCurZ, 5 * mpp);
    {   // R-408: the explorer's marks (a friend's in cyan): the glyph of what they mark (a place: a small diamond), the names of the
        // nearest twelve where they do not run into one another; B-412: the one shown over the view boxed, its name first
        std::vector<std::pair<double, const SiteMark*>> shown;
        for (const SiteMark& m : marksHere()) if (mark(m.x, m.z, sx, sy)) shown.push_back({m.index == guide.shownMark ? -1.0 : (m.x - px) * (m.x - px) + (m.z - pz) * (m.z - pz), &m});
        std::sort(shown.begin(), shown.end(), [](const auto& a, const auto& b2) { return a.first < b2.first; });
        std::vector<std::array<int, 4>> boxes;
        for (size_t i = 0; i < shown.size(); i++) {
            const SiteMark& m = *shown[i].second; const SurfaceMark& gm = guide.marks[m.index];
            mark(m.x, m.z, sx, sy);
            const bool armed = m.index == mapDelMark && realTime < mapDelUntil;
            const uint32_t col = armed ? (((int)(realTime * 4) & 1) ? HUD_RED : HUD_WHITE) : (gm.lent ? HUD_CYAN : HUD_WHITE);
            if (gm.kind < LM_COUNT) drawText(canvas, sx - 2, sy - 3, LANDMARK_KIND_SYMBOLS[gm.kind], col);
            else { drawLineRGB(canvas, sx - 2, sy, sx, sy - 2, col); drawLineRGB(canvas, sx, sy - 2, sx + 2, sy, col); drawLineRGB(canvas, sx + 2, sy, sx, sy + 2, col); drawLineRGB(canvas, sx, sy + 2, sx - 2, sy, col); }
            if (m.index == onMark || m.index == guide.shownMark) drawRectRGB(canvas, sx - 4, sy - 5, sx + 4, sy + 4, m.index == onMark ? HUD_AMBER : col);
            if (i >= 12 || gm.name.empty()) continue;
            std::string nm = trunc(upper(gm.name), 14);
            int tw = textWidth(nm.c_str()), lx = sx + 5, ly = sy - 3;
            if (lx + tw > ox + MW - 1) lx = sx - 5 - tw;
            bool clear = lx >= ox && ly >= oy;
            for (const auto& b2 : boxes) if (clear && lx < b2[2] + 2 && lx + tw > b2[0] - 2 && ly < b2[3] + 1 && ly + 7 > b2[1] - 1) clear = false;
            if (!clear) continue;
            drawText(canvas, lx, ly, nm.c_str(), col); boxes.push_back({lx, ly, lx + tw, ly + 7});
        }
    }
    if (surf.hasWaypoint && mark(surf.wpX, surf.wpZ, sx, sy)) { drawLineRGB(canvas, sx - 3, sy, sx + 3, sy, HUD_WHITE); drawLineRGB(canvas, sx, sy - 3, sx, sy + 3, HUD_WHITE); drawText(canvas, sx + 4, sy - 3, "WAYPOINT", HUD_WHITE); }
    {   // you, with your heading
        int cx = ox + MW / 2, cy = oy + MH / 2;
        double hx = std::sin(surf.player.yaw), hz = std::cos(surf.player.yaw);
        drawLineRGB(canvas, cx, cy, cx + (int)(hx * 6), cy - (int)(hz * 6), HUD_WHITE);
        drawRectRGB(canvas, cx - 1, cy - 1, cx + 1, cy + 1, HUD_WHITE);
    }
    if (mark(mapCurX, mapCurZ, sx, sy)) {   // R-408: the cursor
        const uint32_t cc = HUD_AMBER;
        drawLineRGB(canvas, sx - 7, sy, sx - 3, sy, cc); drawLineRGB(canvas, sx + 3, sy, sx + 7, sy, cc);
        drawLineRGB(canvas, sx, sy - 7, sx, sy - 3, cc); drawLineRGB(canvas, sx, sy + 3, sx, sy + 7, cc);
    }
    const int tx = ox + MW + 6;
    drawText(canvas, tx, oy, "SECTOR MAP", HUD_AMBER);
    drawText(canvas, tx, oy + 9, fmt("%.0f KM WIDE", 2 * hw / 1000).c_str(), HUD_GREEN);
    drawText(canvas, tx, oy + 18, "+/- ZOOM N CLOSE", HUD_DIM);
    {   // R-408: the cursor: its way and distance from the explorer, its place, the mark or the waypoint under it
        const double cd = std::hypot(mapCurX - px, mapCurZ - pz);
        double clat, clon; surf.site.latLonAt(mapCurX, mapCurZ, clat, clon);
        const double la = clat / DEG, lo = wrapAngle(clon) / DEG;
        drawText(canvas, tx, oy + 31, "CURSOR", HUD_DIM);
        drawText(canvas, tx, oy + 40, cd < 2 * mpp ? "HERE" : fmt("%s %s", metresString(cd).c_str(), compassName(wrap2pi(std::atan2(mapCurX - px, mapCurZ - pz)))).c_str(), HUD_WHITE);
        drawText(canvas, tx, oy + 49, fmt("%.2f%s %.2f%s", std::fabs(la), la >= 0 ? "N" : "S", std::fabs(lo), lo >= 0 ? "E" : "W").c_str(), HUD_DIM);
        drawText(canvas, tx, oy + 58, fmt("S %s", sectorName(la, lo).c_str()).c_str(), HUD_CYAN);   // O2
        if (onMark >= 0) {
            drawText(canvas, tx, oy + 67, trunc(markLabel(onMark), 17).c_str(), guide.marks[onMark].lent ? HUD_CYAN : HUD_AMBER);
            drawText(canvas, tx, oy + 76, onMark == guide.shownMark ? "SHIFT+ENTER HIDES" : "SHIFT+ENTER SHOWS", HUD_DIM);   // B-412: over the view
        }
        else if (surf.hasWaypoint && std::hypot(surf.wpX - mapCurX, surf.wpZ - mapCurZ) < 5 * mpp) drawText(canvas, tx, oy + 67, "THE WAYPOINT", HUD_WHITE);
    }
    {   // R-408: the scanner
        if (scanMode < 0) drawText(canvas, tx, oy + 89, "R SCANNER OFF", HUD_DIM);
        else {
            drawText(canvas, tx, oy + 89, fmt("SCAN %s", SCAN_MODE_NAMES[scanMode]).c_str(), HUD_CYAN);   // Q-01: the scanner's own names
            double hx, hz; const SurfaceView::ScanEcho* e = scanModeHere(scanMode) && surf.scanned ? scanNearest(hx, hz) : nullptr;
            if (!scanModeHere(scanMode)) drawText(canvas, tx, oy + 98, "NONE ON THIS WORLD", HUD_DIM);
            else if (!surf.scanned) drawText(canvas, tx, oy + 98, "SCANNING", HUD_DIM);
            else if (!e) drawText(canvas, tx, oy + 98, "NONE IN 40 KM", HUD_DIM);
            else {
                drawText(canvas, tx, oy + 98, fmt("%s %s", metresString(std::hypot(hx - px, hz - pz)).c_str(), compassName(wrap2pi(std::atan2(hx - px, hz - pz)))).c_str(), HUD_CYAN);
                drawText(canvas, tx, oy + 107, trunc(echoDetail(*e), 17).c_str(), HUD_DIM);
            }
            drawText(canvas, tx, oy + 116, scanSkipMarked ? "MARKED LEFT OUT" : "MARKED HEARD (*)", HUD_DIM);
        }
    }
    drawText(canvas, tx, oy + 127, fmt("ELEV %+.0f", surf.player.y).c_str(), HUD_DIM);
    for (const FrontForecast& f : forecast) {   // W-03: the next front, or the one overhead
        if (f.tClear <= t) continue;
        const uint32_t c = f.dust ? HUD_AMBER : HUD_CYAN;
        drawText(canvas, tx, oy + 144, f.dust ? "DUST FRONT" : "RAIN FRONT", c);
        drawText(canvas, tx, oy + 153, (f.tArrive <= t ? "ENDS " + countdownString(f.tClear - t) : "IN " + countdownString(f.tArrive - t)).c_str(), c);
        break;
    }
    drawText(canvas, ox, oy + MH + 4, "ENTER MARK  DEL REMOVE  M WAYPOINT  TAB NEXT  C HOME", HUD_DIM);
}

void Game::renderSurfaceHUD() {
    if (chartUp && !photoMode && surf.env.skyBrightness < 0.5) {   // C-10: the chart held up to the night sky, the figures kept above the horizon
        Mat3 lf = surf.site.localFrame(t);
        drawChartOverlay(surf.testCamWorld(t), surf.site.worldPos(t, surf.player.x, surf.player.z, 0.002), surf.proj, &lf);
    }
    drawVisor(HUD_DIM);
    const SurfaceEnvironment& e = surf.env;
    const Player& pl = surf.player;
    const bool seat = surf.inVehicle() && !surf.chaseCam;
    // (B-309: the visor glint arcs of N5-02 are gone; their angle spun whenever the sun sat near the screen centre
    // and they wobbled with every head bob. The surface draws a soft veiling glare round the sun instead.)
    // R-414 (2026-10-11): the HUD pared down (the user: "all the indicators of elevation, longitude, local time, elevation, temp, grav,
    // wind, rain and what not should be hidden in a separate menu you can open by some key"): on foot the world's name and the place,
    // the epoch, the compass, the scanner, the stamina and what E or Q would do here; the rest in the readings (Tab, `drawReadings`)
    std::string l1 = fmt("%s  %.1f%c%s %.1f%c%s", trunc(upper(bodyNameOf(surf.site.body)), 14).c_str(), std::fabs(e.latDeg), CH_DEGREE,
                         e.latDeg >= 0 ? "N" : "S", std::fabs(e.lonDeg), CH_DEGREE, e.lonDeg >= 0 ? "E" : "W");
    drawTextShadow(canvas, 8, 8, l1.c_str(), HUD_GREEN, HUD_SHADOW);
    if (!seat) drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);   // R-414: not from a vehicle's camera
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
    if (!surf.inVehicle()) {
        // what E or Q does here, over the stamina from the bottom up (R-414: the capsule's, the buggy's and the drone's distances are gone:
        // K calls the capsule, B and F unfold a new buggy or drone at it; the user: "there is no need for that if you can always call
        // capsule in 1 button and buggy/drone in 2")
        std::vector<std::pair<std::string, uint32_t>> act;
        if (e.nearCapsule) act.push_back({"CAPSULE HERE - Q", HUD_AMBER});
        std::string use;   // E's in `updateSurface`'s order
        if (surf.buggy.deployed && surf.buggyDist() <= 3.5) use = "BUGGY HERE - E";
        else if (surf.drone.deployed && surf.droneDist() <= 3.5) use = "DRONE HERE - E";
        else if (surf.nearInstrument.id >= 0 && (surf.nearShard.index < 0 || surf.nearInstrument.dist < surf.nearShard.dist)) use = surf.nearInstrument.name + " - E";   // C-11: an instrument, the nearer
        else if (surf.nearShard.index >= 0) use = std::string(RECORD_MEDIUM_HUD[clampi(surf.nearShard.medium, 0, RM_COUNT - 1)]) + " - E";   // C-03; T-04: the object ("A CODEX - E")
        if (!use.empty()) act.push_back({use, HUD_AMBER});
        if (surf.nearGrave.k >= 0) act.push_back({fmt("A GRAVE: %s", upper(surf.nearGrave.hud).c_str()), HUD_AMBER});   // C-12: the stone's name and years
        for (size_t i = 0; i < act.size(); i++) drawTextShadow(canvas, UW - 8 - textWidth(act[i].first.c_str()), UH - 32 - 8 * (int)i, act[i].first.c_str(), act[i].second, HUD_SHADOW);
        // M8-02 stamina bar and pulse (hidden while driving or flying)
        int bx = UW - 68, by = UH - 24;
        if (!pl.swimming) {
            drawRectRGB(canvas, bx, by, bx + 40, by + 5, HUD_DIM);
            int w = (int)(pl.stamina / 100.0 * 39);
            if (w > 0) fillRectRGB(canvas, bx + 1, by + 1, bx + w, by + 4, pl.winded ? HUD_RED : (pl.sprinting ? HUD_AMBER : HUD_GREEN));
            int pulse = (int)(60 + 80 * (1 - pl.stamina / 100.0) + 30 * pl.sprintRamp);
            drawText(canvas, bx + 44, by - 1, fmt("%d", pulse).c_str(), pl.winded ? HUD_RED : HUD_DIM);
        }
        if (pl.jetOn || (pl.altAboveGround > 2 && !pl.onGround)) {   // the jetpack's heat (R-414: its height and climb in the readings)
            std::string jet = fmt("JET HEAT %.0f%%%s", pl.jetHeat, surf.site.escapeVelocity < 30 ? "  CTRL DESCENDS" : "");
            drawTextCentered(canvas, UW / 2, UH - 52, jet.c_str(), pl.jetHeat > 90 ? HUD_RED : HUD_AMBER);
        }
    } else if (seat) {
        // R-204, R-403: the nose camera, the buggy's and the drone's alike: the brackets inside the visor's, the blinking REC, the pan gauge
        // under the compass (where the camera looks, its 70 deg limits), the crosshair, the speed in double-size digits and the turn under
        // the picture (right is right). R-414 (the user: "local time, hdg, alt, vs, trip and other things (pan, tilt, cam 02 drone) - gone,
        // descent and climb also. Only keep speed"): the channel's name, the pan and tilt, the height, the climb, the trip, the gear, the
        // heading, the state, the capsule, the local time and the epoch went, the most of them to the readings (Tab)
        const bool drone = surf.inDrone;
        const double heading = drone ? surf.drone.heading : surf.buggy.heading, speed = drone ? surf.drone.speed : surf.buggy.speed;
        const double turn = drone ? surf.drone.steer / (70 * DEG) : surf.buggy.steer / (30 * DEG);
        const uint32_t fc = HUD_WHITE, fd = HUD_DIM;
        {   // brackets inside the visor's
            int m = 14, l = 26;
            drawLineRGB(canvas, m, m, m + l, m, fc); drawLineRGB(canvas, m, m, m, m + l, fc);
            drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, fc); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, fc);
            drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, fc); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, fc);
            drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, fc); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, fc);
        }
        if ((int)(realTime * 2) & 1) drawTextShadow(canvas, 8, 16, "\x07REC", HUD_RED, HUD_SHADOW);
        {   // the pan gauge
            const double look = wrapAngle(pl.yaw - heading);
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
        drawTextShadow(canvas, 8, UH - 48, fmt("%3.0f", std::fabs(speed) * 3.6).c_str(), fc, HUD_SHADOW, 2);
        drawTextShadow(canvas, 8 + textWidth("000", 2) + 4, UH - 42, speed < -0.2 ? "KM/H BACK" : "KM/H", fd, HUD_SHADOW);
        {   // the turn bar
            int cx = UW / 2, y = UH - 28;
            drawLineRGB(canvas, cx - 30, y, cx + 30, y, fd); drawLineRGB(canvas, cx, y - 2, cx, y + 2, fd);
            int sx = cx + (int)std::lround(clampd(turn, -1, 1) * 30);
            fillRectRGB(canvas, sx - 1, y - 3, sx + 1, y + 3, HUD_AMBER);
            drawTextCentered(canvas, cx, y + 5, drone ? "TURN" : "STEER", fd);
        }
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
    if (readings) drawReadings();   // R-414
    drawShownMark();     // B-412: the one mark the explorer shows over the view (R-408 drew the eight nearest)
    drawScanHUD();       // R-408: the scanner
    drawCommonHUD();
}

// R-414 (2026-10-11): the readings, Tab on the ground and in the vehicles (the user: "should be hidden in a separate menu you can open by
// some key"): what the HUD showed of the place, the sky, the air, the ground and the vehicle, in a panel at the right under the epoch's
// lines (the right of the view is free on foot over the stamina and in a vehicle, whose speed is at the left), rows of 23 characters at
// most so it stops short of the crosshair (x 150-170); Tab again puts it away
void Game::drawReadings() {
    const SurfaceEnvironment& e = surf.env;
    const Player& pl = surf.player;
    const size_t W = 23;
    std::vector<std::pair<std::string, uint32_t>> rows;
    auto row = [&](const std::string& s, uint32_t c) { rows.push_back({trunc(s, (int)W), c}); };
    auto pack = [&](const std::vector<std::string>& items, uint32_t c) {   // two spaces apart, a row while they fit
        std::string line;
        for (const std::string& it : items) {
            if (it.empty()) continue;
            if (!line.empty() && line.size() + 2 + it.size() > W) { row(line, c); line.clear(); }
            line += (line.empty() ? "" : "  ") + it;
        }
        if (!line.empty()) row(line, c);
    };
    row(fmt("SECTOR %s", sectorName(e.latDeg, e.lonDeg).c_str()), HUD_CYAN);   // O2 (R-301)
    row(fmt("LOCAL %02d:%02d", (int)(e.localTime * 24), (int)(std::fmod(e.localTime * 24, 1.0) * 60)), HUD_WHITE);
    if (surf.site.sys->sunless()) row("NO SUN", HUD_WHITE);   // D-04: a drifter's ground (its sun at the nadir with no light: no reading)
    else row(fmt("SUN %+.0f%c AZ %03.0f%c", e.sun.altitude / DEG, CH_DEGREE, e.sun.azimuth / DEG, CH_DEGREE), HUD_WHITE);
    if (surf.inDrone) {   // R-403: the drone's numbers and its state
        const Drone& d = surf.drone;
        pack({fmt("HDG %03.0f %s", wrap2pi(d.heading) / DEG, compassName(wrap2pi(d.heading))), fmt("TRIP %.2f KM", d.odometer / 1000.0)}, HUD_GREEN);
        pack({fmt("ALT %.0f M", d.altAboveGround), fmt("VS %+.1f", d.vy)}, d.ceiling ? HUD_AMBER : HUD_GREEN);
        const std::string st = d.landed ? (d.rotor > 0.5 ? "LIFTING" : "LANDED") : (d.ceiling ? "AT ITS CEILING" : (d.vy > 1 ? "CLIMBING" : (d.vy < -1 ? "DESCENDING" : (std::fabs(d.speed) < 1 ? "HOVERING" : ""))));
        pack({st, d.lights ? "LIGHTS ON" : ""}, d.landed || d.ceiling ? HUD_AMBER : HUD_GREEN);
    } else if (surf.inBuggy) {   // R-204: the buggy's
        const Buggy& b = surf.buggy;
        pack({fmt("HDG %03.0f %s", wrap2pi(b.heading) / DEG, compassName(wrap2pi(b.heading))), fmt("TRIP %.2f KM", b.odometer / 1000.0)}, HUD_GREEN);
        const std::string st = b.airborne ? "AIRBORNE" : (b.skid > 0.3 ? "SKIDDING" : (b.brake ? "BRAKING" : ""));
        pack({fmt("GEAR %d%s", b.gear + 1, b.speed < -0.2 ? " REVERSE" : ""), st, b.lights ? "LIGHTS ON" : ""}, st.empty() ? HUD_GREEN : HUD_AMBER);
    }
    pack({fmt("ELEV %+.0f M", e.altitude), fmt("TEMP %+.0f C", e.temperatureC), surf.lamp && !surf.inVehicle() ? "LAMP ON" : ""}, HUD_GREEN);   // D-04
    const double gg = surf.site.gravity / 9.8;
    if (gg < 0.01) pack({fmt("GRAV %.4f G", gg), fmt("ESCAPE %.1f M/S", surf.site.escapeVelocity)}, HUD_GREEN);   // O4: a comet
    else pack({fmt("GRAV %.2f G", gg), fmt("%.2f ATM", e.pressureAtm)}, HUD_GREEN);
    {   // the wind and the weather
        std::vector<std::string> wx;
        if (surf.site.atmosphere) wx.push_back(e.windKnots > 0.5 ? fmt("WIND %.0f KT %s", e.windKnots, compassName(e.windDir)) : std::string("NO WIND"));
        if (e.ringShadow > 0.15) wx.push_back("RING SHADOW");   // O0-01
        if (e.cometActivity > 0.05) wx.push_back(fmt("VENTING %.0f%%", e.cometActivity * 100));   // O4
        if (e.rain > 0.05) wx.push_back(fmt("RAIN %.0f%%", e.rain * 100));
        if (e.snow > 0.05) wx.push_back(fmt("SNOW %.0f%%", e.snow * 100));
        if (e.hail > 0.05) wx.push_back("HAIL");
        if (e.dust > 0.05) wx.push_back(fmt("DUST %.0f%%", e.dust * 100));
        if (e.fogBank > 0.3) wx.push_back("FOG");
        if (e.aurora > 0.15 && e.skyBrightness < 0.4) wx.push_back(e.auroraStorm > 0.7 ? "AURORA STORM" : "AURORA");   // R-402
        pack(wx, HUD_GREEN);
    }
    if (!surf.inDrone) {   // B-321: the suit's life sensor: the nearest herd or flock within 1.5 km (`X` still brackets what is in view)
        double best = 1e9, bx = 0, bz = 0; bool flock = false;
        for (const SurfaceView::Herd& hd : surf.herds) { double dx = hd.cx - pl.x, dz = hd.cz - pl.z, d = std::sqrt(dx * dx + dz * dz); if (d < best) { best = d; bx = dx; bz = dz; flock = false; } }
        for (const Flock& f : surf.flocks) { double a = f.phase + t * f.speed; double dx = f.cx + std::cos(a) * f.radius - pl.x, dz = f.cz + std::sin(a) * f.radius - pl.z, d = std::sqrt(dx * dx + dz * dz); if (d < best) { best = d; bx = dx; bz = dz; flock = true; } }
        if (best < 1500) row(best < 25 ? std::string("LIFE HERE") : fmt("LIFE %s %s%s", metresString(best).c_str(), compassName(wrap2pi(std::atan2(bx, bz))), flock ? " (FLYERS)" : ""), HUD_CYAN);
    }
    if (!surf.inDrone && !surf.site.roads.empty()) {   // C-09: the old road under the feet or the wheels and the way it runs (T-01: its kind)
        const double rx = surf.inBuggy ? surf.buggy.x : pl.x, rz = surf.inBuggy ? surf.buggy.z : pl.z;
        double d, along, hd; const SiteRoad* rd;
        if (surf.site.roadAt(rx, rz, 12, d, along, hd, rd) && d < rd->half + 1.5 && roadLeft(rd->id, along, rd->wear) > 0.3)
            row(fmt("OLD %s %s-%s", NET_KIND_HUD[clampi(rd->kind, 0, NK_COUNT - 1)], compassName(wrap2pi(hd)), compassName(wrap2pi(hd + PI))), HUD_CYAN);
    }
    if (surf.lastRange > 0) {   // O1 (B-302): the rangefinder: how far the crosshair rests and how long it takes to get there (walking, driving, flying)
        const double spd = surf.inDrone ? 80.0 : (surf.inBuggy ? 32.0 : (surf.site.escapeVelocity < 30 ? 0.3 * surf.site.escapeVelocity : 4.2));   // a fast cruise over open ground, not the 50 m/s top; R-403: a cruise by air
        const double secs = surf.lastRange / spd;
        const std::string tt = secs < 90 ? fmt("%.0f S", secs) : (secs < 5400 ? fmt("%.0f MIN", secs / 60) : fmt("%.1f H", secs / 3600));
        row(fmt("AHEAD %s %s%s", metresString(surf.lastRange).c_str(), tt.c_str(), surf.lastRangeWater ? " WATER" : ""), HUD_CYAN);
        const int mk = markNear(surf.lastRangeX, surf.lastRangeZ, std::max(150.0, 0.03 * surf.lastRange));   // R-408: the mark the crosshair rests on
        if (mk >= 0) row(markLabel(mk), guide.marks[mk].lent ? HUD_CYAN : HUD_AMBER);
    }
    if (!surf.inVehicle()) {   // the walker's own
        if (pl.swimming) row(pl.underwater ? "DIVING" : "SWIMMING", HUD_CYAN);
        if (pl.jetOn || (pl.altAboveGround > 2 && !pl.onGround)) pack({fmt("JET ALT %.0f M", pl.altAboveGround), fmt("VS %+.1f", pl.vy)}, HUD_AMBER);
        if (pl.autoWalk > 0) row(fmt("AUTOWALK %d", pl.autoWalk), HUD_GREEN);
        static const char* vn[] = {"", "RADIATION VISOR", "SUPERVISION", "INFRARED", "PLANT VISION"};
        if (visionMode > 0 && visionMode < 5) row(vn[visionMode], HUD_CYAN);
    }
    const bool seat = surf.inVehicle() && !surf.chaseCam;
    const int y0 = seat ? 44 : 36, maxRows = (UH - 56 - y0) / 8 - 1;   // under the pan gauge in a vehicle; over the prompts and the stamina on foot
    if ((int)rows.size() > maxRows) rows.resize(std::max(0, maxRows));
    int w = textWidth("READINGS");
    for (const auto& r : rows) w = std::max(w, textWidth(r.first.c_str()));
    const int x0 = UW - 8 - w, n = (int)rows.size() + 1;
    blendRectRGB(canvas, x0 - 3, y0 - 3, x0 + w + 2, y0 + n * 8 + 1, rgb(0, 0, 0), 150);
    drawRectRGB(canvas, x0 - 3, y0 - 3, x0 + w + 2, y0 + n * 8 + 1, rgb(40, 52, 40));
    drawText(canvas, x0, y0, "READINGS", HUD_AMBER);
    for (int i = 0; i < (int)rows.size(); i++) drawText(canvas, x0, y0 + 8 * (i + 1), rows[i].first.c_str(), rows[i].second);
}
