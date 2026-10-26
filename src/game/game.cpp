// Game orchestration core: construction, settings, the frame loop, global keys.
// Each state's update and picture is in a file of its own (game.h's banners name them).
#include "game.h"
#include "peoples/works.h"
#include "peoples/plan.h"
#include "peoples/roads.h"
#include "galaxy/drainage.h"
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include "core/png.h"
#include "core/fs.h"

Game::~Game() { if (migrateThread.joinable()) migrateThread.join(); if (droneJob.joinable()) droneJob.join(); }

Game::Game() : rgbBuf((size_t)FBW * FBH, 0) {
    canvas.px = rgbBuf.data(); canvas.w = FBW; canvas.h = FBH;
    spaceR.proj = Proj::fromHFov(70);
    cabinGrain.generate(0x5A1D, 5, 1);
    newGame();
    applySettings();
    state = GameState::TITLE;
}

// settings and save file (the constructor touches no files)
void Game::loadFromDisk() {
    settings.load(settingsPath);
    applySettings();
    guide.load(guidePath);
    guide.genVersion = GEN_VERSION;
    padMap.load(padPath);
    hasSave = loadNewest();
    if (!hasSave) newGame();
    if (guide.migratedShards > 0) {   // T-04: the shards found before the records were the peoples' own, kept by their index: said once (the guide saved)
        logEvent("SHARD",
                fmt("THE OLD PEOPLES' RECORDS WERE REGENERATED: THE %d SHARDS FOUND ARE KEPT BY THEIR PLACE IN THEIR WORLDS' RECORDS, THE LANGUAGE AS LEARNT, THEIR CONTENTS NEW",
                        guide.migratedShards));
        statusNext = fmt("RECORDS REGENERATED - %d SHARDS KEPT, CONTENTS NEW", guide.migratedShards); statusNextSecs = 8;
        guide.migratedShards = 0;
        guide.save(guidePath);
    }
    state = GameState::TITLE;
}

void Game::applySettings() {
    settings.clampAll();
    setHudScheme(settings.hudColor);
    // resolution (M10-01): the framebuffer, the RGB output and every projection follow the scale
    setFramebufferScale(settings.renderScale);
    int S = FB_SCALE;
    g_mushKernel = settings.mushMode == 0 ? S + 1 : (settings.mushMode == 1 ? S + 2 : (settings.mushMode == 2 ? std::max(2, S) : std::max(2, S - 1)));
    if ((int)rgbBuf.size() != FBW * FBH || (int)fb.idx.size() != FBW * FBH) {
        fb = Framebuffer();
        rgbBuf.assign((size_t)FBW * FBH, 0);
    }
    canvas.px = rgbBuf.data(); canvas.w = FBW; canvas.h = FBH; canvas.scale = S;
    spaceR.proj = Proj::fromHFov(settings.fovDeg);
    surf.setFov(settings.fovDeg);
    surf.mouseSens = settings.mouseSensitivity;
    surf.shadingMode = settings.shadingMode;
    surf.sprintToggleMode = settings.sprintToggle;
    surf.sprintMultiplier = settings.sprintSpeed;
    surf.invertY = settings.invertY;
    audio.master = settings.masterVolume;
}

bool Game::mouseCaptureWanted() const {
    return stateHas(state, ST_MOUSE);
}

void Game::status(const std::string& m, double secs) { statusMsg = m; statusUntil = realTime + secs; }
void Game::refuse(const std::string& m, double secs) { status(m, secs); audio.beep = BEEP_ERROR; }

std::string Game::epocString() const { return epocOf(t); }

// X-04: a time's (a recording's)
std::string Game::epocOf(double secs) {
    int epoc = 6011 + (int)(secs / 1e9);
    int sinister = (int)std::fmod(secs / 1e6, 1000.0);
    int medius = (int)std::fmod(secs / 1e3, 1000.0);
    return fmt("EPOC %d:%03d.%03d", epoc, sinister, medius);
}

// M5-05: EPOC:SINISTER.MEDIUS.DEXTER
std::string Game::epocFullString() const {
    double secs = t;
    int epoc = 6011 + (int)(secs / 1e9);
    int sinister = (int)std::fmod(secs / 1e6, 1000.0);
    int medius = (int)std::fmod(secs / 1e3, 1000.0);
    int dexter = (int)std::fmod(secs, 1000.0);
    return fmt("EPOC %d:%03d.%03d.%03d", epoc, sinister, medius, dexter);
}

// M5-05: game seconds from the wall clock (epoch 2026-01-01 UTC)
double Game::wallClockT() {
    // the original ran on the real clock: here game time 3.6e6 s (a fresh expedition) is 2026-01-01 00:00 UTC
    double unixNow = (double)std::time(nullptr);
    return 3.6e6 + (unixNow - 1767225600.0);
}

std::string Game::distanceString(double km) const {
    if (km < 1e5) return fmt("%.0f KM", km);
    if (km < 1e8) return fmt("%.2f MKM", km / 1e6);
    return fmt("%.3f LY", km / SECTOR_KM);
}

// N0-01: "740 KM/S, PERIAPSIS IN 3.2 H"
// a comet's speed and where it is on its plunge; mean anomaly below PI means it has passed periapsis
std::string Game::cometMotionString(int bi) const {
    const Body& b = sys.bodies[bi];
    double spd = length(sys.bodyVel(bi, t));
    double M = sys.meanAnomaly(bi, t);
    double P = std::fabs(b.orbitPeriod);
    auto hours = [](double s) { return s > 48 * 3600 ? fmt("%.0f D", s / 86400) : fmt("%.1f H", s / 3600); };
    if (M < PI) return fmt("%.0f KM/S, RECEDING, APOAPSIS IN %s", spd, hours((PI - M) / TAU * P).c_str());
    return fmt("%.0f KM/S, PERIAPSIS IN %s", spd, hours((TAU - M) / TAU * P).c_str());
}

// N3: "QUADRUPED", "BROWSER", ...
const char* Game::shortPlan(int plan) {
    static const char* const names[PLAN_COUNT] = {"QUADRUPED", "BROWSER", "CRAWLER", "HOPPER", "STRIDER", "GIANT", "FLYER", "SWIMMER"};
    return plan >= 0 && plan < PLAN_COUNT ? names[plan] : "?";
}

// what an expedition had out goes with it (none of it is saved): the radar, the player and the instruments' case, the telescope (put
// back to its rest for a new expedition; a load keeps its power and aim), the chart held up, the probe and the drones (a load reads its
// own probe and bay from the save's lines after this; a drone thread's world, if any, is taken when it is done)
void Game::resetExpedition(bool freshTelescope) {
    radarOff(); radar = RadarState();
    playerStop(); stopInstrument(); player.items.clear(); icase = InstrumentCase();
    telescopeOff();
    if (freshTelescope) tele = TelescopeState();
    chartUp = false; chartHeld = StarChart(); chartT = -1;
    probe = ProbeState();
    bay = DroneBay();
}

void Game::newGame() {
    t = 3.6e6;
    timeWarp = 1;
    almanacRunning = false;
    resetExpedition(true);
    // G-02: the home star is pinned (HOME_SX/SZ, chosen with `vesperis_test home`: a yellow star with two living worlds, the
    // first temperate with two moons, the drainage tiles of its default landing site in under a second); the search of a
    // pleasant home system (a yellow/orange star with a felisian planet) is the fallback when a generation change takes it away
    Star best; bool found = false;
    int bestScore = -1;
    if (starInSector(HOME_SX, HOME_SY, HOME_SZ, best) && (best.cls == STAR_YELLOW || best.cls == STAR_ORANGE)) {
        StarSystem hs; hs.generate(best);
        for (auto& b : hs.bodies) if (b.type == PT_FELISIAN && b.parent < 0) { found = true; bestScore = 100; break; }
    }
    for (int64_t x = 176; x < 196 && bestScore < 100; x++)
        for (int64_t z = 36; z < 56; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            if (s.cls != STAR_YELLOW && s.cls != STAR_ORANGE) continue;
            StarSystem ss; ss.generate(s);
            int score = 0;
            for (auto& b : ss.bodies) { if (b.type == PT_FELISIAN && b.parent < 0) score += 50; if (b.parent < 0) score += 3; if (b.rings) score += 5; }
            if (score > bestScore) { bestScore = score; best = s; found = true; }
        }
    if (!found) starInSector(180, 0, 40, best);
    sys.generate(best);
    ship = ShipState();
    // park at the first felisian planet (or the second planet)
    int target = -1;
    for (auto& b : sys.bodies) if (b.type == PT_FELISIAN && b.parent < 0) { target = b.index; break; }
    if (target < 0 && sys.bodies.size() > 1) target = 1;
    if (target < 0 && !sys.bodies.empty()) target = 0;
    if (target >= 0) {
        const Body& b = sys.bodies[target];
        Vec3 bp = sys.bodyPos(target, t);
        Vec3 toStar = normalize(sys.star.pos - bp);
        Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
        ship.parkDir = normalize(toStar * 0.7 + side * 0.7 + Vec3(0, 0.2, 0));
        ship.parkDist = b.radiusKm * (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5);
        ship.parkedBody = target;
        ship.localTarget = target;
        ship.mode = ShipState::PARKED;
        ship.pos = bp + ship.parkDir * ship.parkDist;
        Vec3 fwd = normalize(bp - ship.pos);
        ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(fwd.y);
    } else {
        ship.pos = sys.star.pos + Vec3(0, 0, -sys.star.radiusKm * 40);
        ship.mode = ShipState::STANDBY;
    }
    nb.update(ship.pos);
    surf.valid = false;
    state = GameState::SPACE;
    status("STARDRIFTER READY - H FOR HELP", 6);
}

void Game::handleGlobalKeys(const Input& in) {
    // every F-key function has a letter or control-key twin: Mac laptops give the top row to
    // brightness and volume unless Fn is held (bug B-002)
    if (in.wasPressed(KEY_F12) || (in.wasPressed(KEY_P) && !in.ctrl() && !in.alt())) wantsScreenshot = true;
    if (in.wasPressed(KEY_F11) || (in.ctrl() && in.wasPressed(KEY_F)) || (in.alt() && in.wasPressed(KEY_ENTER))) wantsFullscreenToggle = true;
    if (in.wasPressed(KEY_F10) || (in.ctrl() && in.wasPressed(KEY_K))) {
        settings.scanlines = !settings.scanlines;
        settings.save(settingsPath);
        status(settings.scanlines ? "SCANLINES ON" : "SCANLINES OFF", 2);
    }
    // photo tools
    bool scene = stateHas(state, ST_PHOTO);
    if (in.ctrl() && in.wasPressed(KEY_P) && !in.shift() && scene) togglePhotoMode();
    if (in.ctrl() && in.wasPressed(KEY_P) && in.shift() && scene) wantsPanorama = true;
    if (in.ctrl() && in.wasPressed(KEY_R) && scene) toggleRecording();
}

void Game::togglePhotoMode() {
    photoMode = !photoMode;
    if (state == GameState::SURFACE && surf.valid) { if (photoMode) surf.enterFreeCam(); else surf.leaveFreeCam(); }
    status(photoMode ? (state == GameState::SURFACE ? "PHOTO MODE: FREE CAMERA (W A S D, SPACE/C, SHIFT FAST) - CTRL+P ENDS" : "PHOTO MODE - CTRL+P ENDS") : "PHOTO MODE OFF", 4);
}

void Game::toggleRecording() {
    recording = !recording;
    if (recording) {
        pngFlush();   // F-02: a take before it written out (the harness's fixed take is the same directory)
        makeDir(moviesDir);
        if (recFixedTake) recTake = 1; else do { recTake++; } while (fileExists(fmt("%s/take_%03d", moviesDir.c_str(), recTake)));
        makeDir(fmt("%s/take_%03d", moviesDir.c_str(), recTake));
        recFrame = 0;
        status(fmt("RECORDING TO %s/TAKE_%03d - CTRL+R STOPS", upper(moviesDir).c_str(), recTake), 4);
    } else status(fmt("RECORDING STOPPED: %d FRAMES", recFrame), 3);
}

// writes movies/take_NNN/frame_NNNNN.png
bool Game::recordFrame() {
    if (!recording) return false;
    std::string fn = fmt("%s/take_%03d/frame_%05d.png", moviesDir.c_str(), recTake, recFrame++);
    writePNGAsync(fn.c_str(), rgbBuf.data(), FBW, FBH);   // F-02: encoded and written by the PNG writer's thread (27.8 ms a frame at 4x on the main thread before)
    return true;
}

// M6-05: three views at yaw -fov, 0, +fov stitched side by side, the HUD off
void Game::renderPanorama(std::vector<uint32_t>& out, int& w, int& h) {
    w = FBW * 3; h = FBH;
    out.assign((size_t)w * h, 0);
    bool wasPhoto = photoMode;
    photoMode = true;
    double fov = settings.fovDeg * DEG;
    for (int k = -1; k <= 1; k++) {
        double off = k * fov;
        if (state == GameState::SURFACE && surf.valid) {
            if (surf.freeCam) surf.freeYaw += off; else surf.player.yaw += off;
            renderSurfaceScene();
            if (surf.freeCam) surf.freeYaw -= off; else surf.player.yaw -= off;
        } else {
            ship.yaw += off;
            renderSpace();
            ship.yaw -= off;
        }
        for (int y = 0; y < FBH; y++) memcpy(&out[(size_t)y * w + (k + 1) * FBW], &rgbBuf[(size_t)y * FBW], FBW * sizeof(uint32_t));
    }
    photoMode = wasPhoto;
    wantsPanorama = false;
}

void Game::frame(const Input& inRaw, double realDt) {
    if (realDt > 0.1) realDt = 0.1;
    realTime += realDt;
    lastRealDt = realDt;
    Input in = inRaw;        // R-418: the controller's buttons become this frame's keys (game/pad.cpp)
    padFrame(in, realDt);
    if (ax.on && !autoTakeover(in, realDt)) autoInput(in, realDt);   // the explorer plays on its own (any key, click or button gives the controls back)
    tapeUpdate(realDt);   // auto-explore's tape (its deck, its tracks, its fade after auto-explore)
    if (!statusNext.empty() && realTime >= statusUntil) { status(statusNext, statusNextSecs); statusNext.clear(); }
    if (arrivalFlash > 0) arrivalFlash -= realDt * 1.6;
    if (!stateHas(state, ST_TYPING) && !(state == GameState::KEYS && keysCapture)) handleGlobalKeys(in);
    for (const StateRow& r : STATE_ROWS) if (r.away && r.state != state) (this->*r.away)();   // what a screen stops off it (U-02)
    if (radar.on && stateHas(state, ST_STOWS)) radarOff();   // a ship's instrument, off the ship
    if (tele.on && stateHas(state, ST_STOWS)) telescopeOff();   // stowed off the window
    if (!visitNoted && sys.valid && state != GameState::TITLE) { noteVisit(); visitNoted = true; guide.save(guidePath); }
    collectMigration();   // the named sights found on the thread become marks
    bool simulating = stateHas(state, ST_SIMULATES);
    if (in.wasPressed(KEY_T) && simulating) {
        // the almanac (M5-06's 25 s time-lapse went with it: the free row runs for any span)
        if (in.ctrl()) { if (state != GameState::DESCENT && state != GameState::ASCENT && state != GameState::PROBE) { if (almanacRunning) almanacStop(false); openAlmanac(); } }
        else if (almanacRunning) almanacStop(false);
        else if (settings.clockMode == 1) status("REAL-TIME CLOCK: THE SKY FOLLOWS THE WALL CLOCK, NO WARP", 3);
        else { timeWarp = timeWarp >= 10000 ? 1 : timeWarp * 10; status(fmt("TIME WARP X%.0f", timeWarp), 2); }
    }
    if (almanacRunning && settings.clockMode == 1) almanacStop(false);   // the wall clock cannot be run
    bool running = almanacRunning && simulating && state != GameState::ALMANAC;
    if (running) timeWarp = almanacWarpFor(almanacRunT - t);   // the run's warp falls as the moment nears
    double dt = realDt * (simulating ? timeWarp : 0);
    bool arrived = running && dt >= almanacRunT - t;
    if (arrived) dt = std::max(0.0, almanacRunT - t);   // the clock lands on the event
    if (simulating) { if (settings.clockMode == 1) t = wallClockT(); else t += dt; }
    if (arrived) almanacStop(true);
    if (simulating) { autosaveTimer += realDt; if (autosaveTimer >= 300) { autosave(); autosaveTimer = 0; } }
    probe.fast = state == GameState::PROBE && in.shift() && probe.endClock < 0;   // Shift held on the probe's screen, this frame's clock x8
    if (simulating) updateProbe(realDt);   // the probe falls on the relay's clock (real time) wherever the explorer is aboard
    if (simulating) updateDrones(realDt);  // the drones' trip and the bay's cooldown on real time, wherever the explorer is
    // the state's update; the ship moves on where its row says, the row taken before (a screen that closes still moves it)
    const StateRow& current = STATE_ROWS[(int)state];
    (this->*current.update)(in, dt, realDt);
    if (current.traits & ST_SHIP_MOVES) updateShipMotion(dt);
    probeAudio();   // the relay's sound for this frame
    darkRadio();    // a drifter's voice on the cabin's radio or the suit's
    updatePlayer(realDt);   // the ship's player, wherever the ship is (a landing stops it)
    // the picture: the state's own, an overlay's over the scene it was opened from unless it covers the frame
    const StateRow& shown = STATE_ROWS[(int)state];
    if ((shown.traits & ST_OVERLAY) && !(shown.covers && (this->*shown.covers)())) renderUnder();
    (this->*shown.render)();
    if (ax.on) autoRender();   // its caption in the first seconds, nothing else
    else renderPad();          // the command menu or the keyboard over everything
}
