// The HUD overlays for space and the surface (the scenes are drawn in scene_draw.cpp, the sector map in sector_map.cpp).
#include "game.h"
#include "peoples/plan.h"
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

void Game::drawVisor(uint32_t col) {
    int m = 6, l = 14;
    drawLineRGB(canvas, m, m, m + l, m, col); drawLineRGB(canvas, m, m, m, m + l, col);
    drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, col); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, col);
    drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, col); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, col);
    drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, col); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, col);
}

// X-04: a recording's screen leaves out the game's warp
void Game::drawCommonHUD(bool warp) {
    if (realTime < statusUntil && !statusMsg.empty()) {
        blendRectRGB(canvas, 0, UH - 12, UW - 1, UH - 1, rgb(0, 0, 0), 120);
        drawTextCentered(canvas, UW / 2, UH - 10, statusMsg.c_str(), HUD_AMBER);
    } else if (almanacRunning) {   // the run to an event holds the status row: what it runs to, what is left of the wait, the warp
        blendRectRGB(canvas, 0, UH - 12, UW - 1, UH - 1, rgb(0, 0, 0), 120);
        std::string s = fmt("TO %s  %s  X%.0f - T STOPS", trunc(almanacRunLabel, 26).c_str(), countdownString(std::max(0.0, almanacRunT - t)).c_str(), timeWarp);
        drawTextCentered(canvas, UW / 2, UH - 10, s.c_str(), HUD_CYAN);
    }
    if (timeWarp > 1 && warp) drawText(canvas, UW - 8 - textWidth(fmt("TIME X%.0f", timeWarp).c_str()), 24, fmt("TIME X%.0f", timeWarp).c_str(), HUD_CYAN);
}

// space's picture: the scene, and its HUD unless a photo is taken or auto-explore plays
void Game::renderSpaceScreen() { renderSpace(); if (!photoMode && !ax.on) renderSpaceHUD(); }

// the ground's picture, likewise
void Game::renderSurfaceScreen() { renderSurfaceScene(); if (!photoMode && !ax.on) renderSurfaceHUD(); }

// the capsule's way down and back up: the ground, the visor and its line (none while auto-explore plays)
void Game::renderCapsuleScreen() {
    renderSurfaceScene();
    if (!ax.on) {
        drawVisor(HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, state == GameState::DESCENT ? "SURFACE CAPSULE DESCENDING" : "RETURNING TO THE STARDRIFTER", HUD_AMBER);
    }
}

void Game::renderSpaceHUD() {
    if (radar.on) { renderRadarCamera(); drawCommonHUD(); return; }   // C-07: the radar camera's own frame
    if (tele.on) { renderTelescopeHUD(); drawCommonHUD(); return; }   // the telescope's own frame (the user's review: an instrument of its own, as the radar)
    drawVisor(HUD_DIM);
    if (chartUp && ship.mode != ShipState::VIMANA) drawChartOverlay(viewBasis(), ship.pos, spaceR.proj, nullptr);   // the chart held up to the sky
    std::string sysName = sys.valid ? trunc(upper(starNameOf(sys.star)), 26) : "INTERSTELLAR SPACE";
    drawTextShadow(canvas, 8, 8, sysName.c_str(), HUD_GREEN, HUD_SHADOW);
    drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);
    if (recording) drawTextShadow(canvas, UW - 8 - textWidth("REC"), 16, "REC", ((int)(realTime * 2) & 1) ? HUD_RED : HUD_DIM, HUD_SHADOW);
    std::string coords = fmt("PARSIS %+lld %+lld %+lld", (long long)sectorOf(ship.pos.x), (long long)sectorOf(ship.pos.y), (long long)sectorOf(ship.pos.z));
    std::string line2 = sys.valid ? sys.classString() + "   " + coords : coords;
    line2 += std::string("  ") + REGION_NAMES[galaxyRegion(sectorOf(ship.pos.x), sectorOf(ship.pos.y), sectorOf(ship.pos.z))];
    drawTextShadow(canvas, 8, 16, line2.c_str(), HUD_DIM, HUD_SHADOW);
    if (player.playing >= 0 && player.playing < (int)player.items.size()) {   // the ship's player, under the system's lines
        const uint32_t col = player.held || player.paused || player.waiting ? HUD_DIM : HUD_GREEN;
        drawRecordKindGlyph(canvas, 8, 24, player.items[player.playing].music ? RC_MUSIC : RC_TEXT, col);
        drawTextShadow(canvas, 17, 24, playerHudLine().c_str(), col, HUD_SHADOW);
    }
    // mode
    const char* modeName = "STANDBY";
    if (ship.mode == ShipState::VIMANA) modeName = "VIMANA FLIGHT";
    else if (ship.mode == ShipState::APPROACH) modeName = "FINE APPROACH";
    else if (ship.mode == ShipState::PARKED) modeName = ship.parkedBelt >= 0 ? "IN THE BELT" : (ship.orbiting ? "ORBIT" : "CHASE");
    drawTextShadow(canvas, 8, UH - 24, modeName, HUD_AMBER, HUD_SHADOW);
    if (probe.active && probeHere()) {   // the probe falling (or its last frame waiting) while the explorer is in the cabin
        std::string pl = probe.endClock >= 0 ? "PROBE: SIGNAL LOST - C SHOWS ITS LAST FRAME"
                                             : fmt("PROBE: %s  %.2f ATM - C WATCHES", probeStageLabel(probe.clock).c_str(), probeBarAt(probe.flight, probe.clock) * 0.98692);
        drawTextShadow(canvas, 8, UH - 32, pl.c_str(), probe.endClock >= 0 ? HUD_RED : HUD_CYAN, HUD_SHADOW);
    }
    if (bay.out && droneBayHere() && !(probe.active && probeHere())) {   // the drones down on the world
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
        std::string rt = fmt("REMOTE: %s %s %.2f LY%s", trunc(upper(starNameOf(ship.remote)), 16).c_str(), ship.remote.cls == STAR_ROGUE ? "NO STAR" : STAR_CLASSES[ship.remote.cls].code, ly,
                guide.visited.count(starKeyOf(ship.remote)) ? "  VISITED" : "");   // the stars already seen; D-02: a drifter no class
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
    // a belt as the local target: its name, the distance to the rock the ship parks at, a bracket on that rock
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
        if (b.type == PT_COMET) {   // how fast it flies and where it is on its plunge
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
                    if (ly < 34) ly = sy + r + 3;                 // never over the top lines or the warning
                    if (ly > UH - 50) ly = UH - 50;
                    if (!ship.targeting) drawText(canvas, lx, ly, label.c_str(), HUD_GREEN);   // the aim's lines hold the bottom while aiming
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
        else if (cls == STAR_BLUE_WHITE) danger = 3.5e7 / d;   // the ultraviolet of a hot star, hazardous inside its first orbit
        else if (cls == STAR_NEUTRON) danger = 3e7 / d;   // the x-ray glare of a neutron star
        else if (cls == STAR_WOLF_RAYET) danger = 5e8 / d;   // the whole system is inside the hazard (a hull exposure to 5e8 km, high beyond)
        else if (cls == STAR_BLACK_HOLE) danger = 6e7 / d;   // the x-rays of the disc
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
                std::string txt = fmt("%s  %s %s  %.2f LY%s", trunc(upper(starNameOf(full)), 16).c_str(), STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, ly,
                        guide.visited.count(starKeyOf(full)) ? "  VISITED" : "");
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
            if (jumpDigits.empty()) {
                blendRectRGB(canvas, cx - 148, UH - 55, cx + 148, UH - 44, rgb(0, 0, 0), 150);
                drawTextCentered(canvas, cx, UH - 53, "N NEXT STAR  ENTER LOCKS  DIGITS JUMP  R CANCELS", HUD_DIM);
            } else {   // the jump being typed: its length, the way it runs in the galaxy and where its end lies
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
    // the HUD pared down (the user: "all the indicators of elevation, longitude, local time, elevation, temp, grav,
    // wind, rain and what not should be hidden in a separate menu you can open by some key"): on foot the world's name and the place,
    // the epoch, the compass, the scanner, the stamina and what E or Q would do here; the rest in the readings (Tab, `drawReadings`)
    std::string l1 = fmt("%s  %.1f%c%s %.1f%c%s", trunc(upper(bodyNameOf(surf.site.body)), 14).c_str(), std::fabs(e.latDeg), CH_DEGREE,
                         e.latDeg >= 0 ? "N" : "S", std::fabs(e.lonDeg), CH_DEGREE, e.lonDeg >= 0 ? "E" : "W");
    drawTextShadow(canvas, 8, 8, l1.c_str(), HUD_GREEN, HUD_SHADOW);
    if (!seat) drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);   // not from a vehicle's camera
    if (recording) drawTextShadow(canvas, UW - 8 - textWidth("REC"), 16, "REC", ((int)(realTime * 2) & 1) ? HUD_RED : HUD_DIM, HUD_SHADOW);
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
        else if (surf.nearInstrument.id >= 0 && (surf.nearShard.index < 0 || surf.nearInstrument.dist < surf.nearShard.dist)) use = surf.nearInstrument.name + " - E";   // an instrument, the nearer
        else if (surf.nearShard.index >= 0) use = std::string(RECORD_MEDIUM_HUD[clampi(surf.nearShard.medium, 0, RM_COUNT - 1)]) + " - E";   // C-03; T-04: the object ("A CODEX - E")
        if (!use.empty()) act.push_back({use, HUD_AMBER});
        if (surf.nearGrave.k >= 0) act.push_back({fmt("A GRAVE: %s", upper(surf.nearGrave.hud).c_str()), HUD_AMBER});   // the stone's name and years
        for (size_t i = 0; i < act.size(); i++) drawTextShadow(canvas, UW - 8 - textWidth(act[i].first.c_str()), UH - 32 - 8 * (int)i, act[i].first.c_str(), act[i].second, HUD_SHADOW);
        // stamina bar and pulse (hidden while driving or flying)
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
        // the nose camera, the buggy's and the drone's alike: the brackets inside the visor's, the blinking REC, the pan gauge
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
    // creature highlight
    if (surf.highlightUntil > realTime) {
        for (const Critter& c : surf.critters) {
            double sx, sy;
            if (!surf.projectPoint(c.x, surf.site.groundHeight(c.x, c.z) + 0.6 * c.size, c.z, sx, sy)) continue;
            int lx = (int)(sx / FB_SCALE), ly = (int)(sy / FB_SCALE);
            if (lx < 4 || ly < 30 || lx > UW - 4 || ly > UH - 50) continue;
            drawRectRGB(canvas, lx - 4, ly - 4, lx + 4, ly + 4, HUD_CYAN);
            double d = std::sqrt((c.x - pl.x) * (c.x - pl.x) + (c.z - pl.z) * (c.z - pl.z));
            const Species* sp = c.species >= 0 && c.species < (int)surf.bestiary.species.size() ? &surf.bestiary.species[c.species] : nullptr;
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
    if (readings) drawReadings();
    drawShownMark();     // the one mark the explorer shows over the view (R-408 drew the eight nearest)
    drawScanHUD();       // the scanner
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
    row(fmt("SECTOR %s", sectorName(e.latDeg, e.lonDeg).c_str()), HUD_CYAN);
    row(fmt("LOCAL %02d:%02d", (int)(e.localTime * 24), (int)(std::fmod(e.localTime * 24, 1.0) * 60)), HUD_WHITE);
    if (surf.site.sys->sunless()) row("NO SUN", HUD_WHITE);   // a drifter's ground (its sun at the nadir with no light: no reading)
    else row(fmt("SUN %+.0f%c AZ %03.0f%c", e.sun.altitude / DEG, CH_DEGREE, e.sun.azimuth / DEG, CH_DEGREE), HUD_WHITE);
    if (surf.inDrone) {   // the drone's numbers and its state
        const Drone& d = surf.drone;
        pack({fmt("HDG %03.0f %s", wrap2pi(d.heading) / DEG, compassName(wrap2pi(d.heading))), fmt("TRIP %.2f KM", d.odometer / 1000.0)}, HUD_GREEN);
        pack({fmt("ALT %.0f M", d.altAboveGround), fmt("VS %+.1f", d.vy)}, d.ceiling ? HUD_AMBER : HUD_GREEN);
        const std::string st =
                d.landed ? (d.rotor > 0.5 ? "LIFTING" : "LANDED") : (d.ceiling ? "AT ITS CEILING" : (d.vy > 1 ? "CLIMBING" : (d.vy < -1 ? "DESCENDING" : (std::fabs(d.speed) < 1 ? "HOVERING" : ""))));
        pack({st, d.lights ? "LIGHTS ON" : ""}, d.landed || d.ceiling ? HUD_AMBER : HUD_GREEN);
    } else if (surf.inBuggy) {   // the buggy's
        const Buggy& b = surf.buggy;
        pack({fmt("HDG %03.0f %s", wrap2pi(b.heading) / DEG, compassName(wrap2pi(b.heading))), fmt("TRIP %.2f KM", b.odometer / 1000.0)}, HUD_GREEN);
        const std::string st = b.airborne ? "AIRBORNE" : (b.skid > 0.3 ? "SKIDDING" : (b.brake ? "BRAKING" : ""));
        pack({fmt("GEAR %d%s", b.gear + 1, b.speed < -0.2 ? " REVERSE" : ""), st, b.lights ? "LIGHTS ON" : ""}, st.empty() ? HUD_GREEN : HUD_AMBER);
    }
    pack({fmt("ELEV %+.0f M", e.altitude), fmt("TEMP %+.0f C", e.temperatureC), surf.lamp && !surf.inVehicle() ? "LAMP ON" : ""}, HUD_GREEN);
    const double gg = surf.site.gravity / 9.8;
    if (gg < 0.01) pack({fmt("GRAV %.4f G", gg), fmt("ESCAPE %.1f M/S", surf.site.escapeVelocity)}, HUD_GREEN);   // a comet
    else pack({fmt("GRAV %.2f G", gg), fmt("%.2f ATM", e.pressureAtm)}, HUD_GREEN);
    {   // the wind and the weather
        std::vector<std::string> wx;
        if (surf.site.atmosphere) wx.push_back(e.windKnots > 0.5 ? fmt("WIND %.0f KT %s", e.windKnots, compassName(e.windDir)) : std::string("NO WIND"));
        if (e.ringShadow > 0.15) wx.push_back("RING SHADOW");
        if (e.cometActivity > 0.05) wx.push_back(fmt("VENTING %.0f%%", e.cometActivity * 100));
        if (e.rain > 0.05) wx.push_back(fmt("RAIN %.0f%%", e.rain * 100));
        if (e.snow > 0.05) wx.push_back(fmt("SNOW %.0f%%", e.snow * 100));
        if (e.hail > 0.05) wx.push_back("HAIL");
        if (e.dust > 0.05) wx.push_back(fmt("DUST %.0f%%", e.dust * 100));
        if (e.fogBank > 0.3) wx.push_back("FOG");
        if (e.aurora > 0.15 && e.skyBrightness < 0.4) wx.push_back(e.auroraStorm > 0.7 ? "AURORA STORM" : "AURORA");
        pack(wx, HUD_GREEN);
    }
    if (!surf.inDrone) {   // the suit's life sensor: the nearest herd or flock within 1.5 km (`X` still brackets what is in view)
        double best = 1e9, bx = 0, bz = 0; bool flock = false;
        for (const SurfaceView::Herd& hd : surf.herds) { double dx = hd.cx - pl.x, dz = hd.cz - pl.z, d = std::sqrt(dx * dx + dz * dz); if (d < best) { best = d; bx = dx; bz = dz; flock = false; } }
        for (const Flock& f : surf.flocks) {
            double a = f.phase + t * f.speed;
            double dx = f.cx + std::cos(a) * f.radius - pl.x, dz = f.cz + std::sin(a) * f.radius - pl.z, d = std::sqrt(dx * dx + dz * dz);
            if (d < best) {
                best = d;
                bx = dx;
                bz = dz;
                flock = true;
            }
        }
        if (best < 1500) row(best < 25 ? std::string("LIFE HERE") : fmt("LIFE %s %s%s", metresString(best).c_str(), compassName(wrap2pi(std::atan2(bx, bz))), flock ? " (FLYERS)" : ""), HUD_CYAN);
    }
    if (!surf.inDrone && !surf.site.roads.empty()) {   // the old road under the feet or the wheels and the way it runs (T-01: its kind)
        const double rx = surf.inBuggy ? surf.buggy.x : pl.x, rz = surf.inBuggy ? surf.buggy.z : pl.z;
        double d, along, hd; const SiteRoad* rd;
        if (surf.site.roadAt(rx, rz, 12, d, along, hd, rd) && d < rd->half + 1.5 && roadLeft(rd->id, along, rd->wear) > 0.3)
            row(fmt("OLD %s %s-%s", NET_KIND_HUD[clampi(rd->kind, 0, NK_COUNT - 1)], compassName(wrap2pi(hd)), compassName(wrap2pi(hd + PI))), HUD_CYAN);
    }
    if (surf.lastRange > 0) {   // the rangefinder: how far the crosshair rests and how long it takes to get there (walking, driving, flying)
        // a fast cruise over open ground, not the 50 m/s top; R-403: a cruise by air
        const double spd = surf.inDrone ? 80.0 : (surf.inBuggy ? 32.0 : (surf.site.escapeVelocity < 30 ? 0.3 * surf.site.escapeVelocity : 4.2));
        const double secs = surf.lastRange / spd;
        const std::string tt = secs < 90 ? fmt("%.0f S", secs) : (secs < 5400 ? fmt("%.0f MIN", secs / 60) : fmt("%.1f H", secs / 3600));
        row(fmt("AHEAD %s %s%s", metresString(surf.lastRange).c_str(), tt.c_str(), surf.lastRangeWater ? " WATER" : ""), HUD_CYAN);
        const int mk = markNear(surf.lastRangeX, surf.lastRangeZ, std::max(150.0, 0.03 * surf.lastRange));   // the mark the crosshair rests on
        if (mk >= 0) row(markLabel(mk), guide.marks[mk].lent ? HUD_CYAN : HUD_AMBER);
    }
    if (!surf.inVehicle()) {   // the walker's own
        if (pl.swimming) row(pl.underwater ? "DIVING" : "SWIMMING", HUD_CYAN);
        if (pl.jetOn || (pl.altAboveGround > 2 && !pl.onGround)) pack({fmt("JET ALT %.0f M", pl.altAboveGround), fmt("VS %+.1f", pl.vy)}, HUD_AMBER);
        if (pl.autoWalk > 0) row(fmt("AUTOWALK %d", pl.autoWalk), HUD_GREEN);
        static const char* const vn[] = {"", "RADIATION VISOR", "SUPERVISION", "INFRARED", "PLANT VISION"};
        if (visionMode > 0 && visionMode < 5) row(vn[visionMode], HUD_CYAN);
    }
    const bool seat = surf.inVehicle() && !surf.chaseCam;
    const int y0 = seat ? 44 : 36, maxRows = (UH - 56 - y0) / 8 - 1;   // under the pan gauge in a vehicle; over the prompts and the stamina on foot
    if ((int)rows.size() > maxRows) rows.resize(std::max(0, maxRows));
    int w = textWidth("READINGS");
    for (const auto& r : rows) w = std::max(w, textWidth(r.first.c_str()));
    const int x0 = UW - 8 - w, n = (int)rows.size() + 1;
    drawPanel(canvas, x0 - 3, y0 - 3, x0 + w + 2, y0 + n * 8 + 1, rgb(0, 0, 0), 150, rgb(40, 52, 40));
    drawText(canvas, x0, y0, "READINGS", HUD_AMBER);
    for (int i = 0; i < (int)rows.size(); i++) drawText(canvas, x0, y0 + 8 * (i + 1), rows[i].first.c_str(), rows[i].second);
}
