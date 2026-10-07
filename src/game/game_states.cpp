// Descent, ascent and surface updates, and the overlay screens (title, help, menu, settings, list, data).
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace {
const int SETTINGS_ITEMS = 19;   // plus BACK (N5-05 added KEY BINDINGS)
// N5-05: the actions a player can rebind (logical key, label)
struct KeyAction { int key; const char* label; };
static const KeyAction KEY_ACTIONS[] = {
    {KEY_W, "FORWARD / DRIVE"}, {KEY_S, "BACK / REVERSE"}, {KEY_A, "LEFT / STEER"}, {KEY_D, "RIGHT / STEER"}, {KEY_SPACE, "JUMP / BRAKE"}, {KEY_LEFT_SHIFT, "SPRINT"},
    {KEY_E, "USE / VEHICLE"}, {KEY_Q, "BOARD CAPSULE"}, {KEY_B, "BUGGY / SIGNAL RADAR"}, {KEY_C, "CROUCH / CLOUDS"}, {KEY_Z, "STAND TALL / TELESCOPE"}, {KEY_V, "VIEW / VISION"}, {KEY_X, "CREATURES"},
    {KEY_M, "WAYPOINT"}, {KEY_N, "SECTOR MAP / NEXT STAR"}, {KEY_K, "RECALL CAPSULE"}, {KEY_I, "DATA SHEET"}, {KEY_T, "WARP / ALMANAC"}, {KEY_R, "AIM / SCANNER"},
    {KEY_L, "TARGET / MARK"}, {KEY_TAB, "ANALYZER"}, {KEY_O, "ORBIT / CHASE"}, {KEY_F, "FIELD AMP. / DRONE"}, {KEY_G, "GUIDE"}, {KEY_J, "LOG"}, {KEY_H, "HELP"}, {KEY_P, "SCREENSHOT"}, {KEY_U, "SHIP LIGHT"}};
const int KEY_ACTION_COUNT = (int)(sizeof(KEY_ACTIONS) / sizeof(KEY_ACTIONS[0]));

}

void Game::updateDescentAscent(const Input& in, double dt, double realDt) {
    transT += realDt;
    double dur = 7.0;
    double u = clampd(transT / dur, 0, 1);
    if (state == GameState::DESCENT) {
        double s = 1 - std::pow(1 - u, 2.2);
        surf.cameraOverrideAlt = 1800 * (1 - s);
        surf.cameraOverridePitch = (-60 + 60 * s) * DEG;
        fade = std::min(1.0, u * 4);
        surf.player.pitch = 0;
        if (u >= 1) {
            surf.cameraOverrideAlt = -1;
            state = GameState::SURFACE;
            status("LANDED - Q AT THE CAPSULE TO RETURN, H HELP", 6);
            {
                const Body& lb = sys.bodies[surf.site.body];
                guide.landed.insert(Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.site.body));
                guide.typesSeen.insert(lb.type);
                logEvent("LANDING", fmt("%s (%s) %.1f%s %.1f%s, %+.0f C, %.2f ATM, %.2f G, %s", upper(bodyNameOf(surf.site.body)).c_str(), PLANET_TYPES[lb.type].name,
                                        std::fabs(surf.env.latDeg), surf.env.latDeg >= 0 ? "N" : "S", std::fabs(surf.env.lonDeg), surf.env.lonDeg >= 0 ? "E" : "W",
                                        surf.env.temperatureC, surf.env.pressureAtm, lb.gravity / 9.8, surf.env.sun.altitude > 0 ? "DAY" : "NIGHT"));
                walkMax = 0; lastWalkX = surf.player.x; lastWalkZ = surf.player.z; lastOdometer = 0; drivenLanding = 0; flownLanding = 0;
                sectorIndexOf(surf.env.latDeg, surf.env.lonDeg, lastSecX, lastSecY);   // O2: the landing sector is not "entered"
            }
        }
        audio.engine = 0.5 * (1 - u); audio.hum = 0.3;
        surf.prefetch(3.0);   // KI-009: warm the caches while the capsule is still high
    } else {
        double s = u * u;
        surf.cameraOverrideAlt = 1800 * s;
        surf.cameraOverridePitch = (-70 * s) * DEG;
        fade = 1 - std::max(0.0, (u - 0.7) / 0.3);
        if (u >= 1) {
            state = GameState::SPACE;
            fade = 1;
            surf.cameraOverrideAlt = -1;
            status("BACK ABOARD THE STARDRIFTER", 4);
            logEvent("LAUNCH", fmt("LEFT %s, LONGEST WALK %.0f M, DROVE %.0f M%s", upper(bodyNameOf(surf.site.body)).c_str(), walkMax, drivenLanding, flownLanding > 0 ? fmt(", FLEW %.0f M", flownLanding).c_str() : ""));   // R-403
        }
        audio.engine = 0.6 * u; audio.hum = 0.3;
    }
    surf.update(realDt, in, t, false);
    updateShipMotion(dt);
}

void Game::updateSurface(const Input& in, double dt, double realDt) {
    surf.update(realDt, in, t, true);
    updateShipMotion(dt);
    // M3-05 statistics: distance from the capsule, height, distance walked and driven
    {
        double dxs = surf.player.x - lastWalkX, dzs = surf.player.z - lastWalkZ;
        double step = std::sqrt(dxs * dxs + dzs * dzs);
        if (step < 50) { if (surf.inBuggy) { guide.totalDrivenM += step; drivenLanding += step; } else if (surf.inDrone) { guide.totalFlownM += step; flownLanding += step; } else guide.totalWalkedM += step; }   // R-403: flown apart
        lastWalkX = surf.player.x; lastWalkZ = surf.player.z;
        if (surf.env.capsuleDist > walkMax) walkMax = surf.env.capsuleDist;
        if (walkMax > guide.longestWalkM) guide.longestWalkM = walkMax;
        if (surf.player.y > guide.highestPointM) guide.highestPointM = surf.player.y;
    }
    {   // O2 (R-301): the surface is one continuous sphere; crossing into another one-degree sector is announced and logged
        int sx, sy; sectorIndexOf(surf.env.latDeg, surf.env.lonDeg, sx, sy);
        if (lastSecX < 0) { lastSecX = sx; lastSecY = sy; }
        else if (sx != lastSecX || sy != lastSecY) {
            lastSecX = sx; lastSecY = sy;
            const TerrainVertex& tvh = surf.site.lod0.at((int)std::floor(surf.player.x / 16), (int)std::floor(surf.player.z / 16));
            status(fmt("ENTERING SECTOR %03d:%03d - %s%s", sx, sy, MATERIAL_NAMES[tvh.material], tvh.biome ? (std::string(", ") + BIOME_NAMES[tvh.biome]).c_str() : ""), 4);
            logEvent("SECTOR", fmt("ENTERED %03d:%03d OF %s %s", sx, sy, upper(bodyNameOf(surf.site.body)).c_str(), surf.inBuggy ? "BY BUGGY" : (surf.inDrone ? "BY DRONE" : "ON FOOT")));
            audio.beep = 4;
        }
    }
    if (!surf.site.roads.empty() && !(surf.inDrone && !surf.drone.landed)) {   // C-09: an old road under the feet or the wheels: a status and a log line the first time on each
        double d, along, hd; const SiteRoad* rd;
        if (surf.site.roadAt(surf.player.x, surf.player.z, 12, d, along, hd, rd) && d < rd->half + 1.0 && roadLeft(rd->id, along, rd->wear) > 0.3 && roadsMet.insert(rd->id).second) {
            std::string way = fmt("%s TO %s", compassName(wrap2pi(hd)), compassName(wrap2pi(hd + PI)));
            status(fmt("AN OLD ROAD RUNS HERE, %s", way.c_str()), 4);
            logEvent("ROAD", fmt("AN OLD ROAD ON %s, %s, UNDER THE %s", upper(bodyNameOf(surf.site.body)).c_str(), way.c_str(), surf.inBuggy ? "WHEELS" : "FEET"));
        }
    }
    if (surf.nearGrave.k >= 0 && guide.graves.insert(graveKey(surf.nearGrave.id)).second) {   // C-12: a grave within reach is read once into the guide, like a landmark: a status and a log line
        std::string what = upper(surf.nearGrave.line);
        const char* where = surf.nearGrave.sclass == SC_MONUMENT ? "A LONE MONUMENT" : (surf.nearGrave.sclass == SC_HAMLET ? "A HAMLET" : (surf.nearGrave.sclass == SC_VILLAGE ? "A VILLAGE" : "A TOWN"));
        status(fmt("A GRAVE: %s", what.c_str()), 5);
        logEvent("GRAVE", fmt("A GRAVE OUTSIDE %s ON %s: %s", where, upper(bodyNameOf(surf.site.body)).c_str(), what.c_str()));
        audio.beep = 4;
    }
    if (in.wasPressed(KEY_R) && !in.ctrl()) { if (in.shift()) scanToggleSkip(); else scanCycle(); }   // R-408: the scanner
    if (in.wasPressed(KEY_L) && !in.ctrl()) {   // R-408: a mark at what the crosshair rests on (the rangefinder's spot), else at the feet
        if (surf.lastRange > 0) beginMark(surf.lastRangeX, surf.lastRangeZ); else beginMark(surf.player.x, surf.player.z);
        return;
    }
    if (in.wasPressed(KEY_G) && !in.ctrl()) { openGuide(); return; }
    if (in.wasPressed(KEY_J) && !in.ctrl()) { guideReturn = GameState::SURFACE; returnState = GameState::SURFACE; logPage = 0; state = GameState::LOG; return; }
    // M8-06: E enters or leaves the buggy when it is close, otherwise boards the capsule; R-403: the drone the same, on the ground
    if (in.wasPressed(KEY_E) && !in.ctrl()) {
        if (surf.inDrone) {
            double trip = surf.drone.odometer, top = surf.drone.topSpeed;
            if (surf.toggleDrone()) { status(fmt("OUT OF THE DRONE - %.2f KM FLOWN, TOP %.0f KM/H", trip / 1000.0, top * 3.6), 4); audio.beep = 4; }
            else { status("LAND FIRST - HOLD SHIFT TO COME DOWN", 4); audio.beep = 3; }
        }
        else if (surf.inBuggy) { guide.longestDriveM = std::max(guide.longestDriveM, surf.buggy.odometer); surf.toggleBuggy(); status(fmt("OUT OF THE BUGGY - %.2f KM, TOP %.0f KM/H", surf.buggy.odometer / 1000.0, surf.buggy.topSpeed * 3.6), 4); audio.beep = 4; }
        else if (surf.buggy.deployed && surf.buggyDist() <= 3.5) { if (surf.toggleBuggy()) { status("NOSE CAMERA: W/S A/D DRIVE  SPACE BRAKE  V VIEW  E OUT", 5); audio.beep = 4; } }
        else if (surf.drone.deployed && surf.droneDist() <= 3.5) { if (surf.toggleDrone()) { status("NOSE CAMERA: SPACE LIFTS OFF  W/S THRUST  A/D TURN  SHIFT DOWN", 5); audio.beep = 4; } else { status("THE DRONE IS STILL UNFOLDING", 3); audio.beep = 3; } }   // R-403
        else if (surf.nearShard.index >= 0) takeShard();   // C-03
        else if (surf.env.nearCapsule) { state = GameState::ASCENT; transT = 0; audio.beep = 2; autosave(); }
        else { status(fmt("CAPSULE IS %.0f M AWAY (%s) - FOLLOW THE BEACON, OR PRESS K TO CALL IT", surf.env.capsuleDist, compassName(surf.env.capsuleBearing)), 5); audio.beep = 3; }
    }
    if (in.wasPressed(KEY_Q) && !surf.inVehicle()) {
        if (surf.env.nearCapsule) { state = GameState::ASCENT; transT = 0; audio.beep = 2; autosave(); }
        else { status(fmt("CAPSULE IS %.0f M AWAY (%s) - FOLLOW THE BEACON, OR PRESS K TO CALL IT", surf.env.capsuleDist, compassName(surf.env.capsuleBearing)), 5); audio.beep = 3; }
    }
    if (in.wasPressed(KEY_B) && !in.ctrl() && !surf.inVehicle()) {
        // R-204: B at the capsule always unfolds a new buggy; the old one, wherever it was left, is scrapped
        bool had = surf.buggy.deployed; double oldDist = surf.buggyDist();
        if (surf.site.escapeVelocity < 30) { status("TOO LITTLE GRAVITY FOR THE BUGGY - IT WOULD FLOAT OFF AT THE FIRST BUMP", 4); audio.beep = 3; }   // O4: a comet
        else if (surf.deployBuggy()) { status(had ? fmt("NEW BUGGY UNFOLDING - THE OLD ONE (%.0f M AWAY) IS SCRAPPED", oldDist) : "BUGGY UNFOLDING BESIDE THE CAPSULE - E TO GET IN", 5); audio.beep = 1; }
        else if (had) { status(fmt("THE BUGGY IS %.0f M AWAY - B AT THE CAPSULE UNFOLDS A NEW ONE", oldDist), 4); audio.beep = 3; }
        else { status("DEPLOY THE BUGGY WITHIN 10 M OF THE CAPSULE", 4); audio.beep = 3; }
    }
    if (in.wasPressed(KEY_F) && !in.ctrl() && !surf.inVehicle()) {   // R-403: F at the capsule unfolds a new drone; the old one, wherever it was left, is scrapped
        bool had = surf.drone.deployed; double oldDist = surf.droneDist();
        if (surf.site.escapeVelocity < 30) { status("TOO LITTLE GRAVITY FOR THE DRONE - AT SPEED IT WOULD LEAVE THE WORLD", 4); audio.beep = 3; }
        else if (surf.deployDrone()) { status(had ? fmt("NEW DRONE UNFOLDING - THE OLD ONE (%.0f M AWAY) IS SCRAPPED", oldDist) : "DRONE UNFOLDING BESIDE THE CAPSULE - E TO GET IN", 5); audio.beep = 1; }
        else if (had) { status(fmt("THE DRONE IS %.0f M AWAY - F AT THE CAPSULE UNFOLDS A NEW ONE", oldDist), 4); audio.beep = 3; }
        else { status("DEPLOY THE DRONE WITHIN 10 M OF THE CAPSULE", 4); audio.beep = 3; }
    }
    if (in.wasPressed(KEY_V) && surf.inVehicle()) { surf.chaseCam = !surf.chaseCam; audio.beep = 4; }
    if (in.wasPressed(KEY_V) && !surf.inVehicle() && !in.ctrl()) {   // M4-10 vision modes
        static const char* names[] = {"NORMAL VISION", "RADIATION VISOR", "SUPERVISION", "INFRARED", "PLANT VISION"};
        visionMode = (visionMode + 1) % 5; status(names[visionMode], 3); audio.beep = 4;
    }
    if (in.wasPressed(KEY_N) && !in.ctrl()) { returnState = GameState::SURFACE; state = GameState::SECTOR_MAP; mapCurX = surf.player.x; mapCurZ = surf.player.z; mapDelMark = -1; audio.beep = 4; return; }   // M4-02; R-408: the cursor on the explorer
    if (in.wasPressed(KEY_X) && !in.ctrl()) { surf.highlightUntil = realTime + 4.0; status(fmt("%d CREATURES OF %d SPECIES, %d FLOCKS NEARBY", (int)surf.critters.size(), (int)surf.seenSpecies.size(), (int)surf.flocks.size()), 3); }   // M4-05/N3-06
    // N3: first sightings go to the log and the statistics
    for (const std::string& sg : surf.sightings) {
        size_t p1 = sg.find('|'), p2 = sg.find('|', p1 + 1);
        std::string name = sg.substr(0, p1), plan = sg.substr(p1 + 1, p2 - p1 - 1), dist = sg.substr(p2 + 1);
        logEvent("SIGHTING", fmt("%s, A %s, %s M AWAY ON %s", upper(name).c_str(), plan.c_str(), dist.c_str(), upper(bodyNameOf(surf.site.body)).c_str()));
        guide.creaturesSeen++;
        status(fmt("A %s: %s", plan.c_str(), upper(name).c_str()), 4);
    }
    surf.sightings.clear();
    if (in.wasPressed(KEY_M) && !in.ctrl()) {
        if (surf.hasWaypoint) { surf.hasWaypoint = false; status("WAYPOINT CLEARED", 3); }
        else if (surf.lastRange > 8) {   // O1 (B-302): the waypoint goes to the spot under the crosshair, however far
            surf.hasWaypoint = true; surf.wpX = surf.lastRangeX; surf.wpZ = surf.lastRangeZ;
            status(fmt("WAYPOINT SET %s AWAY%s - M CLEARS IT", metresString(surf.lastRange).c_str(), surf.lastRangeWater ? " ON THE WATER" : ""), 4);
        } else { surf.hasWaypoint = true; surf.wpX = surf.player.x; surf.wpZ = surf.player.z; status("WAYPOINT SET HERE - M CLEARS IT", 4); }
        audio.beep = 4;
    }
    if (in.wasPressed(KEY_K) && !in.ctrl()) {
        // B-205: the capsule always comes, wherever the buggy is; a buggy left far away is replaced with B
        surf.relocateCapsule(surf.player.x + 5, surf.player.z + 3);
        bool farBuggy = surf.buggy.deployed && !surf.inBuggy && surf.buggyDist() > 50, farDrone = surf.drone.deployed && !surf.inDrone && surf.droneDist() > 50;
        status(farBuggy ? fmt("CAPSULE RECALLED - THE BUGGY IS %.0f M AWAY, B UNFOLDS A NEW ONE", surf.buggyDist())
                        : (farDrone ? fmt("CAPSULE RECALLED - THE DRONE IS %.0f M AWAY, F UNFOLDS A NEW ONE", surf.droneDist()) : "CAPSULE RECALLED TO YOUR POSITION"), 4);   // R-403
        audio.beep = 1;
    }
    if (dataKey(in)) { returnState = GameState::SURFACE; state = GameState::DATA; }   // B-006: the sheet must return to the surface
    audio.hum = 0.05;
    audio.engine = surf.inBuggy ? 0.15 + 0.45 * std::fabs(surf.buggy.speed) / 50.0 : 0;
    {   // N4-04: three gear steps (0-12, 12-28, 28-50 m/s since O1-05), the tyres sliding, suspension thumps, wind at speed
        double av = std::fabs(surf.buggy.speed);
        double lo = surf.buggy.gear == 0 ? 0 : (surf.buggy.gear == 1 ? 12 : 28), hi = surf.buggy.gear == 0 ? 12 : (surf.buggy.gear == 1 ? 28 : 50);
        audio.engineRpm = surf.inBuggy ? clampd((av - lo) / (hi - lo), 0, 1) : 0;
        audio.skid = surf.inBuggy ? surf.buggy.skid : 0;
        if (surf.buggy.thud > 0) { audio.thud = surf.buggy.thud; surf.buggy.thud = 0; }
        if (surf.inBuggy) guide.topSpeedKmh = std::max(guide.topSpeedKmh, av * 3.6);
    }
    {   // R-403: the drone's pods, heard from inside and, fading over 80 m, from outside
        const Drone& d = surf.drone;
        double near = surf.inDrone ? 1.0 : (d.deployed ? clampd(1 - surf.droneDist() / 80.0, 0, 1) * 0.6 : 0.0);
        audio.rotor = d.deployed ? d.rotor * near : 0;
        audio.rotorPitch = clampd(0.5 * std::fabs(d.speed) / SurfaceView::DRONE_TOP_SPEED + 0.5 * std::max(0.0, d.vy) / 15.0, 0, 1);
        if (d.thud > 0) { audio.thud = std::max(audio.thud, d.thud); surf.drone.thud = 0; }
    }
    audio.breath = surf.player.stamina < 60 ? (60 - surf.player.stamina) / 60.0 : 0;
    audio.wind = surf.site.atmosphere ? clampd(surf.env.windKnots / 35.0, 0.08, 1.0) * clampd(surf.env.pressureAtm * 3, 0.15, 1.0) : 0;
    if (surf.player.altAboveGround > 20) audio.wind = std::min(1.0, audio.wind + surf.player.altAboveGround / 300.0);
    if (surf.inBuggy && surf.site.atmosphere) audio.wind = std::min(1.0, audio.wind + 0.5 * std::fabs(surf.buggy.speed) / 50.0);   // N4-04 the wind of speed
    if (surf.inDrone && surf.site.atmosphere) audio.wind = std::min(1.0, audio.wind + 0.6 * std::fabs(surf.drone.speed) / SurfaceView::DRONE_TOP_SPEED);   // R-403
    // M4-09 sound per world: the wind's family, surf near water, birds by day, a footstep per stride
    {
        int mat = surf.stepMaterial = surf.site.lod0.at((int)std::floor(surf.player.x / 16), (int)std::floor(surf.player.z / 16)).material;
        bool waterNear = false;
        for (int k = 0; k < 4 && !waterNear; k++) { double a = k * PI / 2; waterNear = surf.site.waterAt(surf.player.x + std::cos(a) * 30, surf.player.z + std::sin(a) * 30) > -1e8; }
        audio.windTone = waterNear ? 0.25 : (mat == MAT_SAND || mat == MAT_DUST ? 0.6 : (std::fabs(surf.site.groundHeight(surf.player.x + 40, surf.player.z) - surf.site.groundHeight(surf.player.x - 40, surf.player.z)) > 12 ? 1.0 : 0.5));
        audio.surf = waterNear && surf.site.hasWater ? 0.6 : 0;
        audio.birds = (!surf.flocks.empty() && surf.env.skyBrightness > 0.3 && !surf.inVehicle()) ? 0.6 : 0;
        double stride = std::floor(surf.player.stridePhase);
        if (stride != surf.stepPhase && surf.player.onGround && !surf.inVehicle()) {
            surf.stepPhase = stride;
            audio.step = surf.player.swimming ? 3 : ((mat == MAT_SAND || mat == MAT_SNOW || mat == MAT_GRASS || mat == MAT_DUST || mat == MAT_FOREST) ? 1 : 2);
            audio.stepGain = surf.player.sprinting ? 0.8 : 0.5;
        }
    }
    {   // N2-06: leaves in the wind where the canopy is dense, plus the rustle of walking through undergrowth
        audio.leaves = 0;
        if (surf.site.gen.type == PT_FELISIAN) {
            const TerrainVertex& tvp = surf.site.lod0.at((int)std::floor(surf.player.x / 16), (int)std::floor(surf.player.z / 16));
            if (tvp.material == MAT_FOREST || tvp.material == MAT_GRASS) {
                double dens = surf.canopyDensityAt(surf.player.x, surf.player.z, tvp.veg) * (tvp.material == MAT_FOREST ? 1.0 : 0.35);
                bool moving = std::fabs(surf.player.vx) + std::fabs(surf.player.vz) > 0.5 && surf.player.onGround && !surf.inVehicle();
                audio.leaves = clampd(dens * (0.25 + clampd(surf.env.windKnots / 20.0, 0, 1)) + (moving && tvp.material == MAT_FOREST ? 0.25 * dens : 0), 0, 1);
            }
        }
    }
    {   // N3-05: creature calls, hoof-steps and the dusk insects
        if (!surf.pendingCalls.empty()) { audio.call = surf.pendingCalls.back().kind; audio.callPitch = surf.pendingCalls.back().pitch; surf.pendingCalls.clear(); }
        audio.hoofs = surf.hoofLevel;
        bool waterNear2 = false;
        for (int k = 0; k < 4 && !waterNear2; k++) { double a = k * PI / 2; waterNear2 = surf.site.waterAt(surf.player.x + std::cos(a) * 30, surf.player.z + std::sin(a) * 30) > -1e8; }
        audio.insects = (surf.site.gen.type == PT_FELISIAN && waterNear2 && surf.env.skyBrightness > 0.05 && surf.env.skyBrightness < 0.45 && surf.env.temperatureC > 5) ? 0.5 : 0;
    }
    audio.rain = std::max(surf.env.rain, surf.env.hail);
    updateForecast();   // W-03
    if (surf.env.dust > 0.05) audio.wind = std::min(1.0, audio.wind + 0.8 * surf.env.dust);
    if (surf.eruptionCue > 0) { audio.thunder = 0.6; surf.eruptionCue = 0; }
    audio.lava = surf.site.gen.type == PT_MOLTEN ? 0.4 : (surf.site.gen.type == PT_TECTONIC ? 0.35 + 0.6 * surf.quake : 0);   // R-307: the tectonic world rumbles, more so in a quake
    if (surf.env.rain > 0.5 && flashTimer <= 0) {
        Rng r((uint64_t)(realTime * 100));
        if (r.chance(0.002)) { flashTimer = 2.0; audio.thunder = 0.8; }
    }
    if (flashTimer > 0) flashTimer -= realDt;
}

void Game::renderTitle() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 90);
    // the title carries the name and the choices, nothing else
    drawTextCentered(canvas, UW / 2 + 2, 42, "VESPERIS", rgb(20, 40, 30), 4);
    drawTextCentered(canvas, UW / 2, 40, "VESPERIS", HUD_GREEN, 4);
    drawTextCentered(canvas, UW / 2, 100, hasSave ? "ENTER: CONTINUE      N: NEW EXPEDITION" : "ENTER: BEGIN EXPEDITION", HUD_AMBER);
    drawTextCentered(canvas, UW / 2, 112, "H: HELP     C: CREDITS     ESC: QUIT", HUD_DIM);
}

void Game::renderHelp() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), helpPage == 2 ? 245 : 200);
    static const char* page0[] = {
        "STARDRIFTER - FLIGHT CONTROL", "",
        "MOUSE LOOKS, W A S D WALK IN THE CABIN, ARROWS TURN",
        "THE SHIP; E USES A CONSOLE, A SCREEN, THE SHARD",
        "DECODER (BACK WALL) OR THE CAPSULE",
        "U LIGHT  Y GLASS HULL  PGUP/PGDN ROOF DECK",
        "R             AIM AT A STAR: N NEXT, ENTER LOCKS IT",
        "R, DIGITS, ENTER  JUMP THAT MANY LY ALONG THE AIM",
        "V             VIMANA FLIGHT TO THE REMOTE TARGET",
        "L / CLICK     LOCAL TARGET: BODIES, THEN BELTS",
        "TAB           THE ANALYZER: BODIES AND BELTS",
        "ENTER         FINE APPROACH TO THE LOCAL TARGET",
        "X  CENTER ON TARGET   Z  TELESCOPE   F  FIELD AMP.",
        "B             THE RADAR: HOLD A RISE, ENTER FLIES",
        "O  ORBIT / CHASE   SHIFT+ARROWS  ROUND THE WORLD",
        "G / M / J  GUIDE/STAR MAP/LOG  C CAPSULE / PROBE",
        "I  DATA SHEET  T  TIME WARP  CTRL+T  THE ALMANAC",
        "CTRL+S / L    SAVE / LOAD         P  SCREENSHOT",
        "CTRL+K / F    SCANLINES / FULLSCREEN   ESC MENU",
        "F1 F2 F5 F9 F10 F11 F12 DO THE SAME (MAC: HOLD FN)",
        "PAGE 1/2 - SPACE: NEXT PAGE, ANY OTHER KEY: CLOSE"};
    static const char* page1[] = {
        "SURFACE EXPLORATION", "",
        "MOUSE         LOOK AROUND",
        "W A S D       WALK   SHIFT SPRINT (STAMINA)",
        "SPACE         JUMP; HOLD IN THE AIR: JETPACK",
        "1-9 / 0       AUTOWALK SPEED / STOP    C CROUCH",
        "Z (HOLD)      STAND TALL    CTRL (SWIMMING) DIVE",
        "B / F         A (NEW) BUGGY / DRONE AT THE CAPSULE",
        "E             IN/OUT, A SHARD, THE CAPSULE (Q TOO)",
        "  BUGGY: W/S DRIVE  A/D STEER  SPACE BRAKE  V VIEW",
        "  DRONE: SPACE LIFTS OFF  W/S THRUST  A/D TURN",
        "         SPACE / SHIFT CLIMB, DESCEND, LAND  V VIEW",
        "  THE MOUSE PANS THE NOSE CAMERA 70 DEG",
        "M  WAYPOINT AT THE CROSSHAIR   K  CALL THE CAPSULE",
        "R / SHIFT+R   SCANNER MODE / THE MARKED OUT OR IN",
        "L             MARK WHERE THE CROSSHAIR RESTS",
        "N             MAP: CURSOR, ENTER MARKS, +/- ZOOM",
        "I (F2)  DATA  T  TIME WARP  CTRL+T  THE ALMANAC",
        "CTRL+S  SAVE  P PHOTO  G GUIDE  H HELP  ESC MENU",
        "PAGE 2/2 - SPACE: PREVIOUS PAGE, OTHER KEYS: CLOSE"};
    static const char* page2[] = {
        "CREDITS", "",
        "VESPERIS",
        "VIBE CODED. HEAVILY INSPIRED BY NOCTIS IV",
        "BY ALESSANDRO GHIGNOLA (2001).", "",
        "C++ AND RAYLIB.", "",
        "ANY KEY: BACK"};
    const char** page = helpPage == 0 ? page0 : (helpPage == 1 ? page1 : page2);
    int n = helpPage == 0 ? (int)(sizeof(page0) / sizeof(page0[0])) : (helpPage == 1 ? (int)(sizeof(page1) / sizeof(page1[0])) : (int)(sizeof(page2) / sizeof(page2[0])));
    for (int i = 0; i < n; i++) {
        if (!page[i][0]) continue;
        drawText(canvas, 10, 8 + i * 9, page[i], i == 0 ? HUD_AMBER : (i == n - 1 ? HUD_DIM : HUD_GREEN));
    }
}

void Game::renderMenu() {
    const char* items[] = {"RESUME", "SAVE EXPEDITION", "LOAD EXPEDITION", "SETTINGS", "HELP", "NEW EXPEDITION", "QUIT TO DESKTOP"};
    const int n = (int)(sizeof(items) / sizeof(items[0]));
    // box: title, items, one blank line, the hint and the version line (bug B-001: nothing overlaps)
    const int top = 40, bottom = UH - 34, itemY0 = top + 24, rowH = 12;
    blendRectRGB(canvas, 60, top, UW - 60, bottom, rgb(0, 0, 0), 210);
    drawRectRGB(canvas, 60, top, UW - 60, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 8, "STARDRIFTER SYSTEMS", HUD_AMBER);
    for (int i = 0; i < n; i++) {
        int y = itemY0 + i * rowH;
        drawTextCentered(canvas, UW / 2, y, items[i], i == menuSel ? HUD_WHITE : HUD_GREEN);
        if (i == menuSel) drawText(canvas, UW / 2 - 60, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 20, "ARROWS + ENTER", HUD_DIM);
    drawTextCentered(canvas, UW / 2, bottom - 10, "VESPERIS", rgb(50, 80, 60));
}

void Game::adjustSetting(int item, int dir) {
    Settings& s = settings;
    if (item == 18) { keysSel = 0; keysCapture = false; state = GameState::KEYS; return; }   // N5-05
    switch (item) {
        case 0: s.mouseSensitivity += 0.1 * dir; break;
        case 1: s.invertY = !s.invertY; break;
        case 2: s.fovDeg += 5 * dir; break;
        case 3: s.masterVolume += 0.05 * dir; break;
        case 4: s.scanlines = !s.scanlines; break;
        case 5: s.windowScale += dir; s.clampAll(); wantsWindowScale = s.windowScale; break;
        case 6: s.renderScale += dir; break;
        case 7: s.mushMode = (s.mushMode + 4 + dir) % 4; break;
        case 8: s.shadingMode = (s.shadingMode + 3 + dir) % 3; break;
        case 9: s.crt = !s.crt; break;
        case 10: s.bloom = !s.bloom; break;
        case 11: s.sprintToggle = !s.sprintToggle; break;
        case 12: s.sprintSpeed += 0.25 * dir; break;
        case 13: s.cabin = !s.cabin; break;
        case 14: s.clockMode = 1 - s.clockMode; break;   // M5-05
        case 15: s.aspect = 1 - s.aspect; break;          // M6-01
        case 16: s.dither = !s.dither; break;
        case 17: s.hudColor = (s.hudColor + 3 + dir) % 3; break;
    }
    applySettings();
    audio.beep = 4;
}

void Game::updateKeysScreen(const Input& in) {
    if (!keymap) keymap = &ownKeymap;
    if (keysCapture) {
        if (rawKey == KEY_ESCAPE) { keysCapture = false; return; }
        if (rawKey >= 32 && rawKey < KEY_MAX) {
            int logical = KEY_ACTIONS[keysSel].key;
            keymap->map[rawKey] = logical;   // pressing rawKey now acts as the action's key
            keysCapture = false;
            audio.beep = 4;
        }
        return;
    }
    if (in.wasPressed(KEY_UP)) keysSel = (keysSel + KEY_ACTION_COUNT - 1) % KEY_ACTION_COUNT;
    if (in.wasPressed(KEY_DOWN)) keysSel = (keysSel + 1) % KEY_ACTION_COUNT;
    if (in.wasPressed(KEY_LEFT)) keysSel = std::max(0, keysSel - (KEY_ACTION_COUNT + 1) / 2);
    if (in.wasPressed(KEY_RIGHT)) keysSel = std::min(KEY_ACTION_COUNT - 1, keysSel + (KEY_ACTION_COUNT + 1) / 2);
    if (enterKey(in)) { keysCapture = true; return; }
    if (in.wasPressed(KEY_BACKSPACE)) {   // reset this action: only its own key acts as it
        int logical = KEY_ACTIONS[keysSel].key;
        for (int k = 0; k < KEY_MAX; k++) if (keymap->map[k] == logical && k != logical) keymap->map[k] = k;
        keymap->map[logical] = logical;
        audio.beep = 4;
    }
    if (in.wasPressed(KEY_ESCAPE)) {
        keymap->save(keysPath);
        state = GameState::SETTINGS;
    }
}

void Game::renderKeysScreen() {
    if (!keymap) keymap = &ownKeymap;
    const int top = 6, bottom = UH - 6;
    blendRectRGB(canvas, 10, top, UW - 10, bottom, rgb(0, 0, 0), 225);
    drawRectRGB(canvas, 10, top, UW - 10, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 6, "KEY BINDINGS", HUD_AMBER);
    int half = (KEY_ACTION_COUNT + 1) / 2;
    for (int i = 0; i < KEY_ACTION_COUNT; i++) {
        int col = i / half, row = i % half;
        int x = 16 + col * 152, y = top + 18 + row * 8;
        bool sel = i == keysSel;
        int logical = KEY_ACTIONS[i].key;
        std::string keys;
        for (int k = 32; k < KEY_MAX; k++) if (keymap->map[k] == logical) keys += (keys.empty() ? "" : "/") + keyName(k);
        if (keys.empty()) keys = "-";
        drawText(canvas, x, y, trunc(KEY_ACTIONS[i].label, 15).c_str(), sel ? HUD_WHITE : HUD_GREEN);
        std::string v = (sel && keysCapture) ? "PRESS A KEY" : trunc(keys, 9);
        drawText(canvas, x + 96, y, v.c_str(), (sel && keysCapture) ? HUD_AMBER : (sel ? HUD_WHITE : HUD_CYAN));
        if (sel) drawText(canvas, x - 6, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 18, keysCapture ? "PRESS THE KEY TO USE FOR THIS ACTION (ESC CANCELS)" : "ARROWS SELECT  ENTER REBIND  BACKSPACE RESET  ESC SAVE", HUD_DIM);
    drawTextCentered(canvas, UW / 2, bottom - 10, "SAVED TO VESPERIS_KEYS.TXT NEXT TO THE GAME", HUD_DIM);
}

void Game::updateSettingsScreen(const Input& in) {
    const int n = SETTINGS_ITEMS + 1;
    if (in.wasPressed(KEY_UP)) settingsSel = (settingsSel + n - 1) % n;
    if (in.wasPressed(KEY_DOWN)) settingsSel = (settingsSel + 1) % n;
    int dir = (in.wasPressed(KEY_RIGHT) ? 1 : 0) - (in.wasPressed(KEY_LEFT) ? 1 : 0);
    if (enterKey(in) && settingsSel < SETTINGS_ITEMS) dir = 1;
    if (dir != 0 && settingsSel < SETTINGS_ITEMS) adjustSetting(settingsSel, dir);
    if (in.wasPressed(KEY_ESCAPE) || (enterKey(in) && settingsSel == SETTINGS_ITEMS)) {
        settings.save(settingsPath);
        state = GameState::MENU;
    }
}

void Game::renderSettings() {
    const Settings& s = settings;
    const int top = 6, bottom = UH - 6, itemY0 = top + 14, rowH = 8;   // 18 items + BACK fit 200 rows at an 8 px pitch (M6-01)
    blendRectRGB(canvas, 30, top, UW - 30, bottom, rgb(0, 0, 0), 215);
    drawRectRGB(canvas, 30, top, UW - 30, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 8, "SETTINGS", HUD_AMBER);
    std::string values[SETTINGS_ITEMS] = {
        fmt("%.1f", s.mouseSensitivity), s.invertY ? "ON" : "OFF", fmt("%.0f%c", s.fovDeg, CH_DEGREE),
        fmt("%.0f%%", s.masterVolume * 100), s.scanlines ? "ON" : "OFF", fmt("%dX  (%dX%d)", s.windowScale, 320 * s.windowScale, 200 * s.windowScale),
        fmt("%dX  (%dX%d)", s.renderScale, 320 * s.renderScale, 200 * s.renderScale), s.mushMode == 0 ? "CLASSIC" : (s.mushMode == 1 ? "SOFT" : (s.mushMode == 2 ? "SHARP" : "FINE")),
        s.shadingMode == 0 ? "FLAT CELLS" : (s.shadingMode == 1 ? "STEPPED" : "SMOOTH"), s.crt ? "ON" : "OFF", s.bloom ? "ON" : "OFF", s.sprintToggle ? "TOGGLE" : "HOLD", fmt("X%.2f", s.sprintSpeed), s.cabin ? "CABIN" : "COCKPIT", s.clockMode ? "REAL TIME (UTC)" : "GAME TIME",
        s.aspect ? "4:3 (TALL PIXELS)" : "SQUARE PIXELS", s.dither ? "ON" : "OFF", s.hudColor == 0 ? "GREEN" : (s.hudColor == 1 ? "AMBER" : "WHITE"), "ENTER TO OPEN"};
    const char* names[SETTINGS_ITEMS] = {"MOUSE SENSITIVITY", "INVERT MOUSE Y", "FIELD OF VIEW", "MASTER VOLUME", "SCANLINES", "WINDOW SIZE", "RENDER SCALE", "MUSH FILTER", "TERRAIN SHADING", "CRT SHADER", "BLOOM", "SPRINT KEY", "SPRINT SPEED", "SHIP VIEW", "CLOCK", "ASPECT", "DITHER", "HUD COLOUR", "KEY BINDINGS"};
    for (int i = 0; i <= SETTINGS_ITEMS; i++) {
        int y = itemY0 + i * rowH;
        bool sel = i == settingsSel;
        if (i == SETTINGS_ITEMS) { drawTextCentered(canvas, UW / 2, y + 2, "BACK", sel ? HUD_WHITE : HUD_GREEN); if (sel) drawText(canvas, UW / 2 - 30, y + 2, ">", HUD_AMBER); continue; }
        drawText(canvas, 44, y, names[i], sel ? HUD_WHITE : HUD_GREEN);
        std::string v = values[i];
        int vx = UW - 44 - textWidth(v.c_str());
        drawText(canvas, vx, y, v.c_str(), sel ? HUD_WHITE : HUD_GREEN);
        if (sel) { drawText(canvas, vx - 12, y, "<", HUD_AMBER); drawText(canvas, UW - 40, y, ">", HUD_AMBER); drawText(canvas, 34, y, ">", HUD_AMBER); }
    }
    drawTextCentered(canvas, UW / 2, bottom - 12, "UP/DOWN SELECT  LEFT/RIGHT CHANGE  ESC BACK", HUD_DIM);
}

void Game::renderSystemList() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    std::string head = fmt("SYSTEM ANALYZER - %s (%s)", trunc(upper(starNameOf(sys.star)), 16).c_str(), sys.classString().c_str());
    drawText(canvas, 8, 6, head.c_str(), HUD_AMBER);
    std::vector<std::string> desc = wrapText(upper(STAR_CLASSES[sys.star.cls].description), 50);   // S-02: the long rows wrap onto a second line (they were cut at 50 characters)
    for (size_t k = 0; k < desc.size() && k < 2; k++) drawText(canvas, 8, 15 + 8 * (int)k, desc[k].c_str(), HUD_DIM);
    int listY = desc.size() > 1 ? 32 : 28;
    int n = (int)sys.bodies.size();
    int rows = desc.size() > 1 ? 16 : 17;
    if (listSel < listScroll) listScroll = listSel;
    if (listSel >= listScroll + rows) listScroll = listSel - rows + 1;   // O3: the belts count as rows after the bodies
    for (int r = 0; r < rows; r++) {
        int i = listScroll + r;
        if (i >= n) break;
        const Body& b = sys.bodies[i];
        double d = length(sys.bodyPos(i, t) - ship.pos);
        std::string line = fmt("%s%-18.18s %-10s %10s%s%s", b.parent >= 0 ? "  " : "", upper(bodyNameOf(i)).c_str(), shortType(b.type),
                               distanceString(d).c_str(), b.rings ? " R" : "", b.moonCount ? fmt(" %dM", b.moonCount).c_str() : "");
        if (b.type == PT_COMET) line += fmt(" %.0f KM/S", length(sys.bodyVel(i, t)));   // N0-01: comets are the only bodies that visibly hurry
        uint32_t col = i == listSel ? HUD_WHITE : (i == ship.localTarget ? HUD_AMBER : (PLANET_TYPES[b.type].landable ? HUD_GREEN : HUD_DIM));
        drawText(canvas, 14, listY + r * 9, line.c_str(), col);
        if (i == listSel) drawText(canvas, 6, listY + r * 9, ">", HUD_AMBER);
    }
    // M9-12 belts after the bodies; O3: selectable like the bodies (Enter approaches, L targets)
    for (size_t k = 0; k < sys.belts.size(); k++) {
        int r = n - listScroll + (int)k;
        if (r < 0 || r >= rows) continue;
        const Belt& bl = sys.belts[k];
        uint32_t col = n + (int)k == listSel ? HUD_WHITE : (ship.targetBelt == (int)k ? HUD_AMBER : HUD_DIM);
        drawText(canvas, 14, listY + r * 9, fmt("%-18.18s %-10s %s - %s", upper(beltNameOf((int)k)).c_str(), "BELT", distanceString(bl.innerKm).c_str(), distanceString(bl.outerKm).c_str()).c_str(), col);
        if (n + (int)k == listSel) drawText(canvas, 6, listY + r * 9, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, UH - 10, "UP/DOWN SELECT  ENTER APPROACH  L TARGET  ESC CLOSE", HUD_DIM);
}

// W-03: the forecast at the feet
void Game::updateForecast() {
    if (!surf.valid || !sys.valid || surf.site.body < 0 || surf.site.body >= (int)sys.bodies.size()) return;
    const Body& b = sys.bodies[surf.site.body];
    if (!worldHasFronts(b)) { forecast.clear(); forecastBody = surf.site.body; forecastT = t; return; }
    if (forecastBody == surf.site.body && (std::fabs(t - forecastT) < 30 || realTime - forecastReal < 0.5)) return;   // thirty seconds of game time, and twice a second of ours under a warp
    frontsAhead(b, surf.site.unitAt(surf.player.x, surf.player.z), t, 5 * 86400.0, forecast);
    forecastBody = surf.site.body; forecastT = t; forecastReal = realTime;
}

std::string Game::forecastLine() const {
    if (!surf.valid || !sys.valid || surf.site.body < 0 || surf.site.body >= (int)sys.bodies.size()) return "";
    const Body& b = sys.bodies[surf.site.body];
    if (!worldHasFronts(b)) return "";
    for (const FrontForecast& f : forecast) {
        if (f.tClear <= t) continue;
        if (f.tArrive <= t) return fmt("IN A %sFRONT, CLEARING IN %s", f.dust ? "DUST " : "", countdownString(f.tClear - t).c_str());
        const char* what = f.dust ? "DUST" : (b.type == PT_QUARTZ ? "A FRONT" : (surf.env.temperatureC < -1 ? "SNOW" : "RAIN"));
        return fmt("%s FROM THE %s IN %s", what, frontCompass(f.fromBearing), countdownString(f.tArrive - t).c_str());
    }
    return "NO FRONT WITHIN 5 DAYS";
}

std::string Game::testFrontInfo() const {
    if (!surf.valid) return "no surface";
    const SurfaceEnvironment& e = surf.env;
    return fmt("forecast '%s'; line %s, ahead %s, cloud %.2f (pattern %.2f), rain %.2f, cold %.1f C, wind %.0f kt %s, temp %+.0f C, fog %.0f m",
               forecastLine().c_str(), surf.localFront.on ? "on" : "off", e.frontAhead < 1e17 ? fmt("%.1f km", e.frontAhead / 1000).c_str() : "none",
               e.frontCloud, e.cloudPattern, e.frontRain, e.frontCold, e.windKnots, compassName(e.windDir), e.temperatureC, e.fogDistance);
}

void Game::renderDataSheet() {
    if (dataPage > 0) {   // X-04: a probe's profile
        const std::vector<int> pr = dataProbes();
        if (dataPage <= (int)pr.size()) { renderProbeProfile(pr[dataPage - 1]); return; }
        dataPage = 0;
    }
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    int bi = state == GameState::DATA && surf.valid && returnState == GameState::SURFACE ? surf.site.body : ship.localTarget;
    int y = 10;
    auto line = [&](const std::string& s, uint32_t c) { drawText(canvas, 10, y, s.c_str(), c); y += 9; };
    drawText(canvas, UW - 10 - textWidth(epocFullString().c_str()), y, epocFullString().c_str(), HUD_DIM);   // M5-05
    if (bi < 0 && ship.targetBelt >= 0 && ship.targetBelt < (int)sys.belts.size() && !(surf.valid && returnState == GameState::SURFACE)) {
        // O3 (R-302): the belt's sheet
        const Belt& bl = sys.belts[ship.targetBelt];
        line(upper(beltNameOf(ship.targetBelt)), HUD_AMBER);
        line("ASTEROID BELT", HUD_GREEN);
        for (const std::string& w : wrapText("A BAND OF ROCKS AND DUST LEFT IN THE GAP BETWEEN TWO ORBITS. MOST OF ITS STONES ARE UNDER A KILOMETRE; A FEW ARE MOUNTAINS TWENTY KILOMETRES ACROSS. THE INNER ONES OUTRUN THE OUTER, SO THE FIELD SHEARS SLOWLY.", 50)) line(w, HUD_DIM);
        y += 3;
        double w = bl.outerKm - bl.innerKm, mid = 0.5 * (bl.innerKm + bl.outerKm);
        line(fmt("INNER EDGE  %s FROM THE STAR", distanceString(bl.innerKm).c_str()), HUD_GREEN);
        line(fmt("OUTER EDGE  %s   WIDTH %s", distanceString(bl.outerKm).c_str(), distanceString(w).c_str()), HUD_GREEN);
        line(fmt("PERIOD      %.1f H AT THE INNER EDGE, %.1f H AT THE OUTER", bl.period / 3600, TAU / bl.rateAt(bl.outerKm) / 3600), HUD_GREEN);
        double cells = (w / BELT_CELL_KM) * (TAU * mid / BELT_CELL_KM) * std::max(1.0, 2 * 0.015 * mid / BELT_CELL_KM);
        double rocks = cells * 1.1;
        line(fmt("ROCKS       ABOUT %s, THE SPARKLE BAND IS THE SUNLIT DUST", rocks > 1e6 ? fmt("%.1f MILLION", rocks / 1e6).c_str() : fmt("%.0f THOUSAND", rocks / 1e3).c_str()), HUD_GREEN);
        line("LANDABLE    NO - THE CAPSULE NEEDS A WORLD UNDER IT", HUD_RED);
        BeltRock rk;
        if (beltRockNow(rk)) line(fmt("%s     A ROCK %.1f KM ACROSS, %s OFF, TUMBLING SLOWLY", ship.parkedBelt >= 0 ? "PARKED " : "TARGET ", 2 * rk.radiusKm, distanceString(length(rk.pos - ship.pos)).c_str()), HUD_CYAN);
        y += 3;
        line(fmt("STAR        %s, %s", trunc(upper(starNameOf(sys.star)), 18).c_str(), sys.classString().c_str()), HUD_CYAN);
        line(fmt("            L=%.2f  R=%.0f KM", sys.star.luminosity, sys.star.radiusKm), HUD_CYAN);
        for (const std::string& ww : wrapText(upper(describeStar(sys.star)), 50)) line(ww, HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "T THE ALMANAC  ANY OTHER KEY CLOSES", HUD_DIM);
        return;
    }
    if (bi < 0 || bi >= (int)sys.bodies.size()) { drawTextCentered(canvas, UW / 2, 90, "NO LOCAL TARGET", HUD_DIM); return; }
    const Body& b = sys.bodies[bi];
    line(fmt("%s", upper(bodyNameOf(bi)).c_str()), HUD_AMBER);
    line(fmt("%s%s", PLANET_TYPES[b.type].name, b.parent >= 0 ? fmt(" - MOON OF %s", trunc(upper(bodyNameOf(b.parent)), 20).c_str()).c_str() : ""), HUD_GREEN);
    for (const std::string& w : wrapText(upper(PLANET_TYPES[b.type].description), 50)) line(w, HUD_DIM);
    for (const std::string& w : wrapText(upper(describeBody(b, spaceR.genFor(b))), 50)) line(w, HUD_CYAN);   // M9-14
    { std::string tl = traitList(spaceR.genFor(b)); if (!tl.empty()) line(fmt("TRAITS      %s", tl.c_str()), HUD_AMBER); }   // R-304
    if (surf.valid && returnState == GameState::SURFACE && bi == surf.site.body) {
        int bio = surf.site.lod0.at((int)std::floor(surf.player.x / 16), (int)std::floor(surf.player.z / 16)).biome;
        std::string sn = seasonName(surf.site.season, surf.site.lat0, b.axialTilt);
        if (bio || !sn.empty()) line(fmt("HERE        %s%s%s", bio ? BIOME_NAMES[bio] : "", (bio && !sn.empty()) ? ", " : "", sn.c_str()), HUD_AMBER);
    }
    y += 3;
    line(fmt("RADIUS      %.0f KM   MASS %.3f EARTHS   TILT %.1f%c", b.radiusKm, b.massEarths, b.axialTilt / DEG, CH_DEGREE), HUD_GREEN);
    line(fmt("GRAVITY     %.2f G (%.1f M/S2)   EQUILIBRIUM %.0f K (%+.0f C)", b.gravity / 9.8, b.gravity, b.tempK, b.tempK - 273.15), HUD_GREEN);
    line(fmt("ORBIT       %s FROM %s%s", distanceString(b.orbitRadiusKm).c_str(), b.parent >= 0 ? (sys.bodies[b.parent].type == PT_COMPANION ? "ITS SUN" : "ITS PLANET") : "THE STAR", b.ecc > 0.05 ? fmt(", ECC %.2f", b.ecc).c_str() : ""), HUD_GREEN);
    line(fmt("PERIOD      %.1f H   ROTATION %.2f H%s", std::fabs(b.orbitPeriod) / 3600, std::fabs(b.rotPeriod) / 3600, b.rotPeriod < 0 ? " (RETROGRADE)" : (b.locked ? " (LOCKED)" : "")), HUD_GREEN);
    if (b.type == PT_COMET) {   // N0-01: where it is now on its plunge
        line(fmt("NOW         %s FROM THE STAR", distanceString(length(sys.bodyPos(bi, t) - sys.star.pos)).c_str()), HUD_CYAN);
        line(fmt("            %s", cometMotionString(bi).c_str()), HUD_CYAN);
    }
    line(fmt("ATMOSPHERE  %s   MOONS %d%s   LANDABLE %s", PLANET_TYPES[b.type].atmosphere ? "YES" : "NONE", b.moonCount, b.rings ? "   RINGS" : "", PLANET_TYPES[b.type].landable ? "YES" : "NO"), PLANET_TYPES[b.type].landable ? HUD_GREEN : HUD_RED);
    {   // R-402: the magnetic field and what it means for the nights (the star's class counts too)
        int mc = magneticClass(magneticField(b));
        double quiet = auroraPotentialAt(sys, b, 0), peak = auroraPotentialAt(sys, b, 1);
        const char* hint = quiet > 0.15 ? "   AURORAE ON MOST NIGHTS" : (peak > 0.3 ? "   AURORAE IN STORMS" : (peak > 0.12 ? "   A FAINT AURORA IN A GREAT STORM" : ""));
        line(fmt("MAGNETIC    %s%s", MAGNETIC_CLASS_NAMES[mc], hint), HUD_GREEN);
    }
    y += 3;
    // M5-06 seasons readout, M5-09 what is up in the sky (surface only)
    if (surf.valid && returnState == GameState::SURFACE && bi == surf.site.body) {
        double lat = surf.site.lat0;
        double sinDec = sys.seasonOf(bi, t) * std::sin(b.axialTilt) / std::max(1e-9, std::sin(std::max(b.axialTilt, 1e-6)));
        sinDec = clampd(sys.seasonOf(bi, t), -1, 1);   // dot(spin axis, to the star) = sin of the declination
        double dec = std::asin(sinDec);
        double cosH0 = -std::tan(lat) * std::tan(dec);
        std::string dayLen = cosH0 <= -1 ? "POLAR DAY" : (cosH0 >= 1 ? "POLAR NIGHT" : fmt("DAY %.1f H OF %.1f", std::fabs(b.rotPeriod) / 3600.0 * std::acos(cosH0) / PI, std::fabs(b.rotPeriod) / 3600.0));
        // next equinox and solstice by scanning the orbit
        double P = std::fabs(b.orbitPeriod), toEq = -1, toSol = -1, prev = sinDec, prevD = 0;
        for (int k = 1; k <= 144 && (toEq < 0 || toSol < 0); k++) {
            double v = sys.seasonOf(bi, t + P * k / 144.0);
            if (toEq < 0 && ((prev < 0) != (v < 0))) toEq = P * k / 144.0;
            double d = v - prev;
            if (toSol < 0 && k > 1 && ((prevD > 0) != (d > 0))) toSol = P * (k - 1) / 144.0;
            prev = v; prevD = d;
        }
        auto hours = [&](double secs) { return secs < 0 ? std::string("-") : (secs > 48 * 3600 ? fmt("%.0f D", secs / 86400) : fmt("%.0f H", secs / 3600)); };
        if (b.axialTilt > 0.5 * DEG) line(fmt("SEASON      DECL %+.1f%c  %s  EQUINOX IN %s  SOLSTICE IN %s", dec / DEG, CH_DEGREE, dayLen.c_str(), hours(toEq).c_str(), hours(toSol).c_str()), HUD_AMBER);
        else line(fmt("SEASON      NONE (NO TILT)  %s", dayLen.c_str()), HUD_AMBER);
        bool forecastShown = false;
        { std::string fc = forecastLine(); if (!fc.empty()) { line(fmt("FORECAST    %s", fc.c_str()), HUD_AMBER); forecastShown = true; } }   // W-03
        std::string sky;
        int nsky = 0;
        Mat3 L = surf.site.localFrame(t);
        Vec3 sitePos = surf.site.worldPos(t, surf.player.x, surf.player.z, 0.002);
        std::vector<std::pair<double, std::string>> up;
        for (int j = 0; j < (int)sys.bodies.size(); j++) {
            if (j == bi) continue;
            Vec3 rel = sys.bodyPos(j, t) - sitePos;
            double d = length(rel);
            Vec3 dl = L * (rel / d);
            if (dl.y < 0) continue;
            double ang = std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1)) / DEG;
            up.push_back({ang, fmt("%s %.0f%c%s", trunc(upper(bodyNameOf(j)), 18).c_str(), std::asin(dl.y) / DEG, CH_DEGREE, ang > 0.25 ? fmt(" (%.1f%c WIDE)", 2 * ang, CH_DEGREE).c_str() : "")});
        }
        std::sort(up.begin(), up.end(), [](auto& a, auto& b2) { return a.first > b2.first; });
        for (auto& u : up) { if (nsky++ >= 2) break; sky += (sky.empty() ? "" : ", ") + u.second; }
        if (up.size() > 2) sky += fmt(" +%d", (int)up.size() - 2);
        line(fmt("IN THE SKY  %s", sky.empty() ? "NOTHING ABOVE THE HORIZON" : sky.c_str()), HUD_AMBER);
        if (surf.env.hasSun2) line(fmt("SECOND SUN  %s AT %+.0f%c", upper(bodyNameOf(sys.companion)).c_str(), surf.env.sun2.altitude / DEG, CH_DEGREE), HUD_AMBER);
        if (!surf.bestiary.species.empty()) {   // N3-01: the world's bestiary, sighted species marked
            std::string life;
            for (int i = 0; i < (int)surf.bestiary.species.size(); i++) {
                const Species& sp = surf.bestiary.species[i];
                life += (life.empty() ? "" : ", ") + upper(sp.name) + (surf.seenSpecies.count(i) ? "*" : "") + " (" + shortPlan(sp.plan) + ")";
            }
            std::vector<std::string> ws = wrapText("LIFE        " + life, 52);
            for (size_t w = 0; w < ws.size() && w < (forecastShown ? 2u : 3u); w++) line(ws[w], HUD_AMBER);   // W-03: the forecast takes the third life line's row (the sheet was already past the frame on a world of many species)
        }
    }
    if (settings.clockMode == 1) line("CLOCK       REAL TIME (UTC)", HUD_DIM);
    line(fmt("STAR        %s, %s", trunc(upper(starNameOf(sys.star)), 18).c_str(), sys.classString().c_str()), HUD_CYAN);
    line(fmt("            L=%.2f  R=%.0f KM", sys.star.luminosity, sys.star.radiusKm), HUD_CYAN);
    for (const std::string& w : wrapText(upper(describeStar(sys.star)), 50)) line(w, HUD_DIM);
    drawTextCentered(canvas, UW / 2, UH - 10, dataProbes().empty() ? "T THE ALMANAC  ANY OTHER KEY CLOSES" : "T THE ALMANAC  RIGHT PROFILES  ANY OTHER KEY CLOSES", HUD_DIM);   // X-04: a giant's probes
}
