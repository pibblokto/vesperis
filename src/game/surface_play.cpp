// The surface's play: the capsule's descent and ascent (DESCENT, ASCENT) and the ground (SURFACE): walking, the vehicles,
// the scanner, the ground's sounds
#include "game.h"
#include "peoples/plan.h"
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

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
            status(sys.sunless() && !surf.lamp ? "LANDED IN THE DARK - U THE LAMP, Q AT THE CAPSULE TO RETURN" : "LANDED - Q AT THE CAPSULE TO RETURN, H HELP", 6);
            {
                const Body& lb = sys.bodies[surf.site.body];
                guide.landed.insert(Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.site.body));
                guide.typesSeen.insert(lb.type);
                logEvent("LANDING", fmt("%s (%s) %.1f%s %.1f%s, %+.0f C, %.2f ATM, %.2f G, %s", upper(bodyNameOf(surf.site.body)).c_str(), KINDS[lb.type].name,
                                        std::fabs(surf.env.latDeg), surf.env.latDeg >= 0 ? "N" : "S", std::fabs(surf.env.lonDeg), surf.env.lonDeg >= 0 ? "E" : "W",
                                        surf.env.temperatureC, surf.env.pressureAtm, lb.gravity / 9.8, surf.env.sun.altitude > 0 ? "DAY" : "NIGHT"));
                walkMax = 0; lastWalkX = surf.player.x; lastWalkZ = surf.player.z; lastOdometer = 0; drivenLanding = 0; flownLanding = 0;
                sectorIndexOf(surf.env.latDeg, surf.env.lonDeg, lastSecX, lastSecY);   // O2: the landing sector is not "entered"
            }
        }
        audio.engine = 0.5 * (1 - u); audio.hum = 0.3;
        surf.prefetch(3.0);   // warm the caches while the capsule is still high
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
            logEvent("LAUNCH", fmt("LEFT %s, LONGEST WALK %.0f M, DROVE %.0f M%s", upper(bodyNameOf(surf.site.body)).c_str(), walkMax, drivenLanding, flownLanding > 0 ? fmt(", FLEW %.0f M", flownLanding).c_str() : ""));
        }
        audio.engine = 0.6 * u; audio.hum = 0.3;
    }
    audio.surfaceOff();   // the capsule hears nothing of the ground, going down or up (its last level went on through the climb)
    surf.update(realDt, in, t, false);
    updateShipMotion(dt);
}

// Q-03: the sounds of the ground the surface cues into the synth's levels: the dunes' hum at their note, the ice's song, a quake's shaking and
// its onset (consumed), all as much as the air carries (the wind's rule: a third of a bar and over whole, the thinnest air 0.15, none airless)
void groundSoundsTo(SurfaceView& surf, AudioState& audio) {
    audio.air = surf.site.atmosphere ? clampd(surf.env.pressureAtm * 3, 0.15, 1.0) : 0.0;
    audio.dunes = surf.duneSong; audio.duneHz = surf.duneHz; audio.ice = surf.iceSong; audio.quake = surf.quake;
    if (surf.quakeCue > 0) { audio.quakeHit = surf.quakeCue; surf.quakeCue = 0; }
}

void Game::updateSurface(const Input& in, double dt, double realDt) {
    surf.update(realDt, in, t, true);
    updateShipMotion(dt);
    // M3-05 statistics: distance from the capsule, height, distance walked and driven
    {
        double dxs = surf.player.x - lastWalkX, dzs = surf.player.z - lastWalkZ;
        double step = std::sqrt(dxs * dxs + dzs * dzs);
        if (step < 50) { if (surf.inBuggy) { guide.totalDrivenM += step; drivenLanding += step; } else if (surf.inDrone) { guide.totalFlownM += step; flownLanding += step; } else guide.totalWalkedM += step; }   // flown apart
        lastWalkX = surf.player.x; lastWalkZ = surf.player.z;
        if (surf.env.capsuleDist > walkMax) walkMax = surf.env.capsuleDist;
        if (walkMax > guide.longestWalkM) guide.longestWalkM = walkMax;
        if (surf.player.y > guide.highestPointM) guide.highestPointM = surf.player.y;
    }
    {   // the surface is one continuous sphere; crossing into another one-degree sector is announced and logged
        int sx, sy; sectorIndexOf(surf.env.latDeg, surf.env.lonDeg, sx, sy);
        if (lastSecX < 0) { lastSecX = sx; lastSecY = sy; }
        else if (sx != lastSecX || sy != lastSecY) {
            lastSecX = sx; lastSecY = sy;
            const TerrainVertex& tvh = surf.site.lod0.at((int)std::floor(surf.player.x / 16), (int)std::floor(surf.player.z / 16));
            status(fmt("ENTERING SECTOR %03d:%03d - %s%s", sx, sy, MATERIAL_NAMES[tvh.material], tvh.biome ? (std::string(", ") + BIOME_NAMES[tvh.biome]).c_str() : ""), 4);
            logEvent("SECTOR", fmt("ENTERED %03d:%03d OF %s %s", sx, sy, upper(bodyNameOf(surf.site.body)).c_str(), surf.inBuggy ? "BY BUGGY" : (surf.inDrone ? "BY DRONE" : "ON FOOT")));
            audio.beep = BEEP_UI;
        }
    }
    if (!surf.site.roads.empty() && !(surf.inDrone && !surf.drone.landed)) {   // an old road under the feet or the wheels: a status and a log line the first time on each
        double d, along, hd; const SiteRoad* rd;
        if (surf.site.roadAt(surf.player.x, surf.player.z, 12, d, along, hd, rd) && d < rd->half + 1.0 && roadLeft(rd->id, along, rd->wear) > 0.3 && roadsMet.insert(rd->id).second) {
            std::string way = fmt("%s TO %s", compassName(wrap2pi(hd)), compassName(wrap2pi(hd + PI)));
            const char* kind = NET_KIND_HUD[clampi(rd->kind, 0, NK_COUNT - 1)];   // the network's kind
            status(fmt("AN OLD %s RUNS HERE, %s", kind, way.c_str()), 4);
            logEvent("ROAD", fmt("AN OLD %s ON %s, %s, UNDER THE %s", kind, upper(bodyNameOf(surf.site.body)).c_str(), way.c_str(), surf.inBuggy ? "WHEELS" : "FEET"));
        }
    }
    if (surf.nearGrave.k >= 0 && guide.graves.insert(graveKey(surf.nearGrave.id)).second) {   // a grave within reach is read once into the guide, like a landmark: a status and a log line
        std::string what = upper(surf.nearGrave.line);
        const char* where = surf.nearGrave.sclass == SC_MONUMENT ? "A LONE MONUMENT" : (surf.nearGrave.sclass == SC_HAMLET ? "A HAMLET" : (surf.nearGrave.sclass == SC_VILLAGE ? "A VILLAGE" : "A TOWN"));
        status(fmt("A GRAVE: %s", what.c_str()), 5);
        logEvent("GRAVE", fmt("A GRAVE OUTSIDE %s ON %s: %s", where, upper(bodyNameOf(surf.site.body)).c_str(), what.c_str()));
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_R) && !in.ctrl()) { if (in.shift()) scanToggleSkip(); else scanCycle(); }   // the scanner
    if (in.wasPressed(KEY_L) && !in.ctrl()) {   // a mark at what the crosshair rests on (the rangefinder's spot), else at the feet
        if (surf.lastRange > 0) beginMark(surf.lastRangeX, surf.lastRangeZ); else beginMark(surf.player.x, surf.player.z);
        return;
    }
    if (in.wasPressed(KEY_G) && !in.ctrl()) { openGuide(); return; }
    if (in.wasPressed(KEY_J) && !in.ctrl()) { guideReturn = GameState::SURFACE; returnState = GameState::SURFACE; logPage = 0; state = GameState::LOG; return; }
    // E enters or leaves the buggy when it is close, otherwise boards the capsule; R-403: the drone the same, on the ground
    if (in.wasPressed(KEY_E) && !in.ctrl()) {
        if (surf.inDrone) {
            double trip = surf.drone.odometer, top = surf.drone.topSpeed;
            if (surf.toggleDrone()) { status(fmt("OUT OF THE DRONE - %.2f KM FLOWN, TOP %.0f KM/H", trip / 1000.0, top * 3.6), 4); audio.beep = BEEP_UI; }
            else { refuse("LAND FIRST - HOLD SHIFT TO COME DOWN"); }
        }
        else if (surf.inBuggy) { guide.longestDriveM = std::max(guide.longestDriveM, surf.buggy.odometer); surf.toggleBuggy(); status(fmt("OUT OF THE BUGGY - %.2f KM, TOP %.0f KM/H", surf.buggy.odometer / 1000.0, surf.buggy.topSpeed * 3.6), 4); audio.beep = BEEP_UI; }
        else if (surf.buggy.deployed && surf.buggyDist() <= 3.5) { if (surf.toggleBuggy()) { status("NOSE CAMERA: W/S A/D DRIVE  SPACE BRAKE  V VIEW  E OUT", 5); audio.beep = BEEP_UI; } }
        else if (surf.drone.deployed && surf.droneDist() <= 3.5) { if (surf.toggleDrone()) { status("NOSE CAMERA: SPACE LIFTS OFF  W/S THRUST  A/D TURN  SHIFT DOWN", 5); audio.beep = BEEP_UI; } else { refuse("THE DRONE IS STILL UNFOLDING", 3); } }
        else if (surf.nearInstrument.id >= 0 && (surf.nearShard.index < 0 || surf.nearInstrument.dist < surf.nearShard.dist)) takeInstrument();   // the nearer of the two
        else if (surf.nearShard.index >= 0) takeShard();
        else if (surf.env.nearCapsule) { state = GameState::ASCENT; transT = 0; audio.beep = BEEP_ARRIVE; autosave(); }
        else { refuse(fmt("CAPSULE IS %.0f M AWAY (%s) - FOLLOW THE BEACON, OR PRESS K TO CALL IT", surf.env.capsuleDist, compassName(surf.env.capsuleBearing)), 5); }
    }
    if (in.wasPressed(KEY_Q) && !surf.inVehicle()) {
        if (surf.env.nearCapsule) { state = GameState::ASCENT; transT = 0; audio.beep = BEEP_ARRIVE; autosave(); }
        else { refuse(fmt("CAPSULE IS %.0f M AWAY (%s) - FOLLOW THE BEACON, OR PRESS K TO CALL IT", surf.env.capsuleDist, compassName(surf.env.capsuleBearing)), 5); }
    }
    if (in.wasPressed(KEY_B) && !in.ctrl() && !surf.inVehicle()) {
        // B at the capsule always unfolds a new buggy; the old one, wherever it was left, is scrapped
        bool had = surf.buggy.deployed; double oldDist = surf.buggyDist();
        if (surf.site.escapeVelocity < 30) { refuse("TOO LITTLE GRAVITY FOR THE BUGGY - IT WOULD FLOAT OFF AT THE FIRST BUMP"); }   // a comet
        else if (surf.deployBuggy()) { status(had ? fmt("NEW BUGGY UNFOLDING - THE OLD ONE (%.0f M AWAY) IS SCRAPPED", oldDist) : "BUGGY UNFOLDING BESIDE THE CAPSULE - E TO GET IN", 5); audio.beep = BEEP_LOCK; }
        else if (had) { refuse(fmt("THE BUGGY IS %.0f M AWAY - B AT THE CAPSULE UNFOLDS A NEW ONE", oldDist)); }
        else { refuse("DEPLOY THE BUGGY WITHIN 10 M OF THE CAPSULE"); }
    }
    if (in.wasPressed(KEY_F) && !in.ctrl() && !surf.inVehicle()) {   // F at the capsule unfolds a new drone; the old one, wherever it was left, is scrapped
        bool had = surf.drone.deployed; double oldDist = surf.droneDist();
        if (surf.site.escapeVelocity < 30) { refuse("TOO LITTLE GRAVITY FOR THE DRONE - AT SPEED IT WOULD LEAVE THE WORLD"); }
        else if (surf.deployDrone()) { status(had ? fmt("NEW DRONE UNFOLDING - THE OLD ONE (%.0f M AWAY) IS SCRAPPED", oldDist) : "DRONE UNFOLDING BESIDE THE CAPSULE - E TO GET IN", 5); audio.beep = BEEP_LOCK; }
        else if (had) { refuse(fmt("THE DRONE IS %.0f M AWAY - F AT THE CAPSULE UNFOLDS A NEW ONE", oldDist)); }
        else { refuse("DEPLOY THE DRONE WITHIN 10 M OF THE CAPSULE"); }
    }
    if (in.wasPressed(KEY_V) && surf.inVehicle()) { surf.chaseCam = !surf.chaseCam; audio.beep = BEEP_UI; }
    if (in.wasPressed(KEY_V) && !surf.inVehicle() && !in.ctrl()) {   // vision modes
        static const char* const names[] = {"NORMAL VISION", "RADIATION VISOR", "SUPERVISION", "INFRARED", "PLANT VISION"};
        visionMode = (visionMode + 1) % 5; status(names[visionMode], 3); audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_TAB) && !in.ctrl()) { readings = !readings; audio.beep = BEEP_UI; }   // the readings in and out of the view
    if (in.wasPressed(KEY_U) && !in.ctrl() && !surf.inVehicle()) { surf.lamp = !surf.lamp; status(surf.lamp ? "LAMP ON" : "LAMP OFF", 2); audio.beep = BEEP_UI; }   // the helmet lamp
    if (in.wasPressed(KEY_N) && !in.ctrl()) { returnState = GameState::SURFACE; state = GameState::SECTOR_MAP; mapCurX = surf.player.x; mapCurZ = surf.player.z; mapDelMark = -1; audio.beep = BEEP_UI; return; }   // M4-02; R-408: the cursor on the explorer
    if (in.wasPressed(KEY_X) && !in.ctrl()) { surf.highlightUntil = realTime + 4.0; status(fmt("%d CREATURES OF %d SPECIES, %d FLOCKS NEARBY", (int)surf.critters.size(), (int)surf.seenSpecies.size(), (int)surf.flocks.size()), 3); }
    // first sightings go to the log and the statistics
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
        else if (surf.lastRange > 8) {   // the waypoint goes to the spot under the crosshair, however far
            surf.hasWaypoint = true; surf.wpX = surf.lastRangeX; surf.wpZ = surf.lastRangeZ;
            status(fmt("WAYPOINT SET %s AWAY%s - M CLEARS IT", metresString(surf.lastRange).c_str(), surf.lastRangeWater ? " ON THE WATER" : ""), 4);
        } else { surf.hasWaypoint = true; surf.wpX = surf.player.x; surf.wpZ = surf.player.z; status("WAYPOINT SET HERE - M CLEARS IT", 4); }
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_K) && !in.ctrl()) {
        // the capsule always comes, wherever the buggy is; a buggy left far away is replaced with B
        surf.relocateCapsule(surf.player.x + 5, surf.player.z + 3);
        bool farBuggy = surf.buggy.deployed && !surf.inBuggy && surf.buggyDist() > 50, farDrone = surf.drone.deployed && !surf.inDrone && surf.droneDist() > 50;
        status(farBuggy ? fmt("CAPSULE RECALLED - THE BUGGY IS %.0f M AWAY, B UNFOLDS A NEW ONE", surf.buggyDist())
                        : (farDrone ? fmt("CAPSULE RECALLED - THE DRONE IS %.0f M AWAY, F UNFOLDS A NEW ONE", surf.droneDist()) : "CAPSULE RECALLED TO YOUR POSITION"), 4);
        audio.beep = BEEP_LOCK;
    }
    if (dataKey(in)) { returnState = GameState::SURFACE; state = GameState::DATA; }   // the sheet must return to the surface
    audio.hum = 0.05;
    audio.engine = surf.inBuggy ? 0.15 + 0.45 * std::fabs(surf.buggy.speed) / 50.0 : 0;
    {   // three gear steps (0-12, 12-28, 28-50 m/s since O1-05), the tyres sliding, suspension thumps, wind at speed
        double av = std::fabs(surf.buggy.speed);
        double lo = surf.buggy.gear == 0 ? 0 : (surf.buggy.gear == 1 ? 12 : 28), hi = surf.buggy.gear == 0 ? 12 : (surf.buggy.gear == 1 ? 28 : 50);
        audio.engineRpm = surf.inBuggy ? clampd((av - lo) / (hi - lo), 0, 1) : 0;
        audio.skid = surf.inBuggy ? surf.buggy.skid : 0;
        if (surf.buggy.thud > 0) { audio.thud = surf.buggy.thud; surf.buggy.thud = 0; }
        if (surf.inBuggy) guide.topSpeedKmh = std::max(guide.topSpeedKmh, av * 3.6);
    }
    {   // the drone's pods, heard from inside and, fading over 80 m, from outside
        const Drone& d = surf.drone;
        double near = surf.inDrone ? 1.0 : (d.deployed ? clampd(1 - surf.droneDist() / 80.0, 0, 1) * 0.6 : 0.0);
        audio.rotor = d.deployed ? d.rotor * near : 0;
        audio.rotorPitch = clampd(0.5 * std::fabs(d.speed) / SurfaceView::DRONE_TOP_SPEED + 0.5 * std::max(0.0, d.vy) / 15.0, 0, 1);
        if (d.thud > 0) { audio.thud = std::max(audio.thud, d.thud); surf.drone.thud = 0; }
    }
    audio.breath = surf.player.stamina < 60 ? (60 - surf.player.stamina) / 60.0 : 0;
    audio.wind = surf.site.atmosphere ? clampd(surf.env.windKnots / 35.0, 0.08, 1.0) * clampd(surf.env.pressureAtm * 3, 0.15, 1.0) : 0;
    if (surf.player.altAboveGround > 20) audio.wind = std::min(1.0, audio.wind + surf.player.altAboveGround / 300.0);
    if (surf.inBuggy && surf.site.atmosphere) audio.wind = std::min(1.0, audio.wind + 0.5 * std::fabs(surf.buggy.speed) / 50.0);   // the wind of speed
    if (surf.inDrone && surf.site.atmosphere) audio.wind = std::min(1.0, audio.wind + 0.6 * std::fabs(surf.drone.speed) / SurfaceView::DRONE_TOP_SPEED);
    // sound per world: the wind's family, surf near water, birds by day, a footstep per stride
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
    {   // leaves in the wind where the canopy is dense, plus the rustle of walking through undergrowth
        audio.leaves = 0;
        if (kindHas(surf.site.gen.type, KT_FLORA)) {
            const TerrainVertex& tvp = surf.site.lod0.at((int)std::floor(surf.player.x / 16), (int)std::floor(surf.player.z / 16));
            if (tvp.material == MAT_FOREST || tvp.material == MAT_GRASS) {
                double dens = surf.canopyDensityAt(surf.player.x, surf.player.z, tvp.veg) * (tvp.material == MAT_FOREST ? 1.0 : 0.35);
                bool moving = std::fabs(surf.player.vx) + std::fabs(surf.player.vz) > 0.5 && surf.player.onGround && !surf.inVehicle();
                audio.leaves = clampd(dens * (0.25 + clampd(surf.env.windKnots / 20.0, 0, 1)) + (moving && tvp.material == MAT_FOREST ? 0.25 * dens : 0), 0, 1);
            }
        }
    }
    {   // creature calls, hoof-steps and the dusk insects
        if (!surf.pendingCalls.empty()) { audio.call = surf.pendingCalls.back().kind; audio.callPitch = surf.pendingCalls.back().pitch; surf.pendingCalls.clear(); }
        audio.hoofs = surf.hoofLevel;
        bool waterNear2 = false;
        for (int k = 0; k < 4 && !waterNear2; k++) { double a = k * PI / 2; waterNear2 = surf.site.waterAt(surf.player.x + std::cos(a) * 30, surf.player.z + std::sin(a) * 30) > -1e8; }
        audio.insects = (kindHas(surf.site.gen.type, KT_FLORA) && waterNear2 && surf.env.skyBrightness > 0.05 && surf.env.skyBrightness < 0.45 && surf.env.temperatureC > 5) ? 0.5 : 0;
    }
    audio.rain = std::max(surf.env.rain, surf.env.hail);
    updateForecast();
    if (surf.env.dust > 0.05) audio.wind = std::min(1.0, audio.wind + 0.8 * surf.env.dust);
    if (surf.eruptionCue > 0) { audio.thunder = 0.6; surf.eruptionCue = 0; }
    {   // the kind's rumble (the molten ground's, R-307: a tectonic world's, more so in a quake)
        const KindDef& K = KINDS[surf.site.gen.type];
        audio.lava = K.rumbleQuake != 0 ? K.rumble + K.rumbleQuake * surf.quake : K.rumble;
        if (K.nightSideC > -1e8 && surf.site.gen.locked) audio.lava *= smoothstep(-0.25, 0.1, surf.env.sun.dirLocal.y);   // on a magma ocean's night side nothing moves
    }
    groundSoundsTo(surf, audio);
    if (surf.env.rain > 0.5 && flashTimer <= 0) {
        Rng r((uint64_t)(realTime * 100));
        if (r.chance(0.002)) { flashTimer = 2.0; audio.thunder = 0.8; }
    }
    if (flashTimer > 0) flashTimer -= realDt;
}
