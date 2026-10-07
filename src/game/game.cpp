// Game orchestration core: construction, settings, the frame loop, global keys, test hooks.
// autopilot.cpp, hud.cpp, landing_map.cpp, game_states.cpp and persistence.cpp hold the rest.
#include "game.h"
#include "galaxy/roads.h"
#include "galaxy/drainage.h"
#include <chrono>
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

Game::~Game() { if (migrateThread.joinable()) migrateThread.join(); }

Game::Game() : rgbBuf((size_t)FBW * FBH, 0) {
    canvas.px = rgbBuf.data(); canvas.w = FBW; canvas.h = FBH;
    spaceR.proj = Proj::fromHFov(70);
    cabinGrain.generate(0x5A1D, 5, 1);
    newGame();
    applySettings();
    state = GameState::TITLE;
}

void Game::loadFromDisk() {
    settings.load(settingsPath);
    applySettings();
    guide.load(guidePath);
    guide.genVersion = GEN_VERSION;
    hasSave = loadNewest();
    if (!hasSave) newGame();
    state = GameState::TITLE;
}

void Game::applySettings() {
    settings.clampAll();
    setHudScheme(settings.hudColor);   // M6-01
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
    return state == GameState::SPACE || state == GameState::SURFACE || state == GameState::DESCENT || state == GameState::ASCENT || state == GameState::STAR_MAP || state == GameState::SECTOR_MAP;   // R-408: the mouse moves the map's cursor
}

void Game::status(const std::string& m, double secs) { statusMsg = m; statusUntil = realTime + secs; }

std::string Game::epocString() const { return epocOf(t); }

std::string Game::epocOf(double secs) {
    int epoc = 6011 + (int)(secs / 1e9);
    int sinister = (int)std::fmod(secs / 1e6, 1000.0);
    int medius = (int)std::fmod(secs / 1e3, 1000.0);
    return fmt("EPOC %d:%03d.%03d", epoc, sinister, medius);
}

std::string Game::epocFullString() const {
    double secs = t;
    int epoc = 6011 + (int)(secs / 1e9);
    int sinister = (int)std::fmod(secs / 1e6, 1000.0);
    int medius = (int)std::fmod(secs / 1e3, 1000.0);
    int dexter = (int)std::fmod(secs, 1000.0);
    return fmt("EPOC %d:%03d.%03d.%03d", epoc, sinister, medius, dexter);
}

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

// N0-01 (B-201): a comet's speed and where it is on its plunge; mean anomaly below PI means it has passed periapsis
std::string Game::cometMotionString(int bi) const {
    const Body& b = sys.bodies[bi];
    double spd = length(sys.bodyVel(bi, t));
    double M = sys.meanAnomaly(bi, t);
    double P = std::fabs(b.orbitPeriod);
    auto hours = [](double s) { return s > 48 * 3600 ? fmt("%.0f D", s / 86400) : fmt("%.1f H", s / 3600); };
    if (M < PI) return fmt("%.0f KM/S, RECEDING, APOAPSIS IN %s", spd, hours((PI - M) / TAU * P).c_str());
    return fmt("%.0f KM/S, PERIAPSIS IN %s", spd, hours((TAU - M) / TAU * P).c_str());
}

const char* Game::shortPlan(int plan) {
    static const char* names[PLAN_COUNT] = {"QUADRUPED", "BROWSER", "CRAWLER", "HOPPER", "STRIDER", "GIANT", "FLYER", "SWIMMER"};
    return plan >= 0 && plan < PLAN_COUNT ? names[plan] : "?";
}

bool Game::testDriveSetup() {
    if (state != GameState::SURFACE || !surf.valid) return false;
    if (surf.inDrone) { surf.drone.landed = true; surf.drone.y = surf.site.surfaceHeight(surf.drone.x, surf.drone.z); surf.toggleDrone(); }   // R-403: out of a flying drone first
    surf.relocateCapsule(surf.player.x + 5, surf.player.z + 3);
    if (!surf.buggy.deployed && !surf.deployBuggy()) return false;
    surf.buggy.unfold = 1;
    surf.player.x = surf.buggy.x + 1.5; surf.player.z = surf.buggy.z;
    return surf.inBuggy || surf.toggleBuggy();
}

double Game::testDriveOpen() {
    if (!surf.valid) return 0;
    double x, z, heading;
    double run = surf.findOpenRun(x, z, heading);
    surf.buggy.x = x; surf.buggy.z = z; surf.buggy.y = surf.site.surfaceHeight(x, z); surf.buggy.heading = heading; surf.buggy.speed = 0;
    surf.buggy.tracks.clear(); surf.buggy.trackHead = 0;
    surf.player.x = x; surf.player.z = z; surf.player.y = surf.buggy.y; surf.player.yaw = heading;
    return run;
}

void Game::newGame() {
    t = 3.6e6;
    timeWarp = 1;
    almanacRunning = false;   // W-01
    telescopeOff(); tele = TelescopeState();   // W-06
    radarOff(); radar = RadarState();   // C-07
    chartUp = false; chartHeld = StarChart(); chartT = -1;   // C-10
    probe = ProbeState();   // X-01: a new expedition sends nothing
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
    if (in.wasPressed(KEY_F10) || (in.ctrl() && in.wasPressed(KEY_K))) { settings.scanlines = !settings.scanlines; settings.save(settingsPath); status(settings.scanlines ? "SCANLINES ON" : "SCANLINES OFF", 2); }
    // M6-05 photo tools
    bool scene = state == GameState::SPACE || state == GameState::SURFACE || state == GameState::PROBE || state == GameState::RECORDING;   // X-01: the probe's camera too; X-04: a recording
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
        makeDir(moviesDir);
        if (recFixedTake) recTake = 1; else do { recTake++; } while (fileExists(fmt("%s/take_%03d", moviesDir.c_str(), recTake)));
        makeDir(fmt("%s/take_%03d", moviesDir.c_str(), recTake));
        recFrame = 0;
        status(fmt("RECORDING TO %s/TAKE_%03d - CTRL+R STOPS", upper(moviesDir).c_str(), recTake), 4);
    } else status(fmt("RECORDING STOPPED: %d FRAMES", recFrame), 3);
}

bool Game::recordFrame() {
    if (!recording) return false;
    std::string fn = fmt("%s/take_%03d/frame_%05d.png", moviesDir.c_str(), recTake, recFrame++);
    return writePNG(fn.c_str(), rgbBuf.data(), FBW, FBH);
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

void Game::frame(const Input& in, double realDt) {
    if (realDt > 0.1) realDt = 0.1;
    realTime += realDt;
    lastRealDt = realDt;
    if (!statusNext.empty() && realTime >= statusUntil) { status(statusNext, statusNextSecs); statusNext.clear(); }
    if (arrivalFlash > 0) arrivalFlash -= realDt * 1.6;
    if (state != GameState::TEXT_ENTRY && state != GameState::CONSOLE && !(state == GameState::KEYS && keysCapture)) handleGlobalKeys(in);
    if (audio.piece && !audio.radio && state != GameState::SHARDS && state != GameState::TEXT_ENTRY) stopPiece();   // C-04: the music plays on the decoder screen only (C-07: or through the radar)
    if (audio.speech && !audio.radio && state != GameState::SHARDS) stopSpeech();                                     // C-05: the voice too
    if (radar.on && (state == GameState::SURFACE || state == GameState::DESCENT || state == GameState::ASCENT || state == GameState::LANDING_MAP || state == GameState::TITLE || state == GameState::SHARDS || state == GameState::SECTOR_MAP || state == GameState::PROBE)) radarOff();   // C-07: a ship's instrument, off the ship
    if (tele.on && (state == GameState::SURFACE || state == GameState::DESCENT || state == GameState::ASCENT || state == GameState::LANDING_MAP || state == GameState::TITLE || state == GameState::SHARDS || state == GameState::SECTOR_MAP || state == GameState::PROBE)) telescopeOff();   // W-06: stowed off the window
    if (!visitNoted && sys.valid && state != GameState::TITLE) { noteVisit(); visitNoted = true; guide.save(guidePath); }
    collectMigration();   // R-408: the named sights found on the thread become marks
    bool simulating = state == GameState::SPACE || state == GameState::SURFACE || state == GameState::DESCENT ||
                      state == GameState::ASCENT || state == GameState::LANDING_MAP || state == GameState::SHIPSCREEN || state == GameState::CONSOLE || state == GameState::SHARDS || state == GameState::PROBE;
    if (in.wasPressed(KEY_T) && simulating) {
        if (in.ctrl()) { if (state != GameState::DESCENT && state != GameState::ASCENT && state != GameState::PROBE) { if (almanacRunning) almanacStop(false); openAlmanac(); } }   // W-01: the almanac (M5-06's 25 s time-lapse went with it: the free row runs for any span)
        else if (almanacRunning) almanacStop(false);
        else if (settings.clockMode == 1) status("REAL-TIME CLOCK: THE SKY FOLLOWS THE WALL CLOCK, NO WARP", 3);
        else { timeWarp = timeWarp >= 10000 ? 1 : timeWarp * 10; status(fmt("TIME WARP X%.0f", timeWarp), 2); }
    }
    if (almanacRunning && settings.clockMode == 1) almanacStop(false);   // W-01: the wall clock cannot be run
    bool running = almanacRunning && simulating && state != GameState::ALMANAC;
    if (running) timeWarp = almanacWarpFor(almanacRunT - t);   // W-01: the run's warp falls as the moment nears
    double dt = realDt * (simulating ? timeWarp : 0);
    bool arrived = running && dt >= almanacRunT - t;
    if (arrived) dt = std::max(0.0, almanacRunT - t);   // the clock lands on the event
    if (simulating) { if (settings.clockMode == 1) t = wallClockT(); else t += dt; }   // M5-05
    if (arrived) almanacStop(true);
    if (simulating) { autosaveTimer += realDt; if (autosaveTimer >= 300) { autosave(); autosaveTimer = 0; } }
    probe.fast = state == GameState::PROBE && in.shift() && probe.endClock < 0;   // R-406: Shift held on the probe's screen, this frame's clock x8
    if (simulating) updateProbe(realDt);   // X-01: the probe falls on the relay's clock (real time) wherever the explorer is aboard
    // state logic
    switch (state) {
        case GameState::TITLE:
            titleYaw += realDt * 0.04;
            titleIdle += realDt; attractT += realDt;   // M6-06
            { bool any = std::fabs(in.mouseDx) + std::fabs(in.mouseDy) > 0; for (int k = 0; k < KEY_MAX && !any; k++) any = in.pressed[k]; if (any) titleIdle = 0; }
            if (in.wasPressed(KEY_C) && !in.ctrl()) { returnState = GameState::TITLE; helpPage = 2; state = GameState::HELP; }   // credits
            updateShipMotion(realDt);
            if (enterKey(in)) {
                state = (returnState == GameState::SURFACE && surf.valid) ? GameState::SURFACE : GameState::SPACE;
                status(state == GameState::SURFACE ? "EXPEDITION RESUMED ON THE SURFACE - H FOR HELP" : "STARDRIFTER READY - H FOR HELP", 6);
                if (macHints && !macHintShown) { macHintShown = true; statusNext = "ON A MAC THE F KEYS NEED FN: USE H, I, P, CTRL+S, CTRL+L, CTRL+K, CTRL+F"; statusNextSecs = 7; }
            }
            if (in.wasPressed(KEY_N) && hasSave) { newGame(); state = GameState::SPACE; }
            if (helpKey(in)) { returnState = GameState::TITLE; helpPage = 0; state = GameState::HELP; }
            if (in.wasPressed(KEY_ESCAPE)) wantsQuit = true;
            break;
        case GameState::SPACE:
            if (in.wasPressed(KEY_ESCAPE)) { if (ship.targeting) ship.targeting = false; else if (radar.on) { radarOff(); status("RADAR CAMERA OFF", 3); } else if (tele.on) { telescopeOff(); status("TELESCOPE STOWED", 3); } else { returnState = state; menuSel = 0; state = GameState::MENU; } break; }   // C-07: Esc leaves the radar camera; W-06: and the telescope
            if (helpKey(in)) { returnState = state; helpPage = 0; state = GameState::HELP; break; }
            if (saveKey(in)) { saveSlot(currentSlot); break; }
            if (loadKey(in)) { loadSlot(currentSlot); break; }
            updateSpace(in, dt, realDt);
            break;
        case GameState::LANDING_MAP:
            updateLandingMap(in, dt);
            break;
        case GameState::DESCENT:
        case GameState::ASCENT:
            if (saveKey(in)) { saveSlot(currentSlot); break; }
            { auto t0 = std::chrono::steady_clock::now(); updateDescentAscent(in, dt, realDt); if (std::getenv("VESPERIS_TRACE") && transT < 0.05) fprintf(stderr, "  descent update %.1f ms\n", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count()); }
            break;
        case GameState::SURFACE:
            if (in.wasPressed(KEY_ESCAPE)) { returnState = state; menuSel = 0; state = GameState::MENU; break; }
            if (helpKey(in)) { returnState = state; helpPage = 1; state = GameState::HELP; break; }
            if (saveKey(in)) { saveSlot(currentSlot); break; }
            if (loadKey(in)) { loadSlot(currentSlot); break; }
            if (photoMode) { surf.updateFreeCam(realDt, in); updateShipMotion(dt); break; }   // M6-05
            updateSurface(in, dt, realDt);
            break;
        case GameState::HELP: {
            bool any = false;
            for (int k = 0; k < KEY_MAX; k++) if (in.pressed[k]) any = true;
            if (in.wasPressed(KEY_SPACE) && helpPage < 2) helpPage = 1 - helpPage;
            else if (any) state = returnState;
            break;
        }
        case GameState::MENU:
            if (in.wasPressed(KEY_UP)) menuSel = (menuSel + 6) % 7;
            if (in.wasPressed(KEY_DOWN)) menuSel = (menuSel + 1) % 7;
            if (in.wasPressed(KEY_ESCAPE)) state = returnState;
            if (enterKey(in)) {
                switch (menuSel) {
                    case 0: state = returnState; break;
                    case 1: openSlots(false); break;
                    case 2: openSlots(true); break;
                    case 3: settingsSel = 0; state = GameState::SETTINGS; break;
                    case 4: helpPage = returnState == GameState::SURFACE ? 1 : 0; state = GameState::HELP; break;
                    case 5: newGame(); break;
                    case 6: autosave(); wantsQuit = true; break;
                }
            }
            break;
        case GameState::SETTINGS:
            updateSettingsScreen(in);
            break;
        case GameState::KEYS:
            updateKeysScreen(in);
            break;
        case GameState::SLOTS:
            updateSlots(in);
            break;
        case GameState::GUIDE: updateGuideMenu(in); break;
        case GameState::TEXT_ENTRY: updateTextEntry(in); break;
        case GameState::STAR_MAP: updateStarMap(in); updateShipMotion(dt); break;
        case GameState::LOG: updateLog(in); break;
        case GameState::STATS: { bool any = false; for (int k = 0; k < KEY_MAX; k++) if (in.pressed[k]) any = true; if (any) state = guideReturn; break; }
        case GameState::GALLERY: updateGallery(in); break;
        case GameState::SHIPSCREEN: updateShipScreen(in); updateShipMotion(dt); break;
        case GameState::CONSOLE: updateConsole(in); updateShipMotion(dt); break;
        case GameState::SHARDS: updateShards(in, realDt); updateShipMotion(dt); break;   // C-06
        case GameState::SECTOR_MAP: updateSectorMap(in); break;
        case GameState::ALMANAC: updateAlmanac(in); break;   // W-01
        case GameState::PROBE: updateProbeScreen(in, dt, realDt); break;   // X-01
        case GameState::RECORDING: updateRecording(in, realDt); break;     // X-04
        case GameState::SYSTEM_LIST: {
            int nb = (int)sys.bodies.size(), n = nb + (int)sys.belts.size();   // O3: the belts follow the bodies
            if (n == 0) { state = GameState::SPACE; break; }
            if (in.wasPressed(KEY_UP)) listSel = (listSel + n - 1) % n;
            if (in.wasPressed(KEY_DOWN)) listSel = (listSel + 1) % n;
            if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_TAB)) state = GameState::SPACE;
            auto pick = [&]() { if (listSel < nb) { ship.localTarget = listSel; ship.targetBelt = -1; } else { ship.localTarget = -1; ship.targetBelt = listSel - nb; } };
            if (in.wasPressed(KEY_L)) { pick(); state = GameState::SPACE; status(fmt("LOCAL TARGET: %s", upper(localTargetLabel()).c_str()), 4); }
            if (enterKey(in)) { pick(); state = GameState::SPACE; if (listSel < nb) startApproach(listSel); else startApproachBelt(listSel - nb); }
            break;
        }
        case GameState::DATA: {
            if (in.wasPressed(KEY_T) && dataPage == 0) { openAlmanac(); break; }   // W-01: the sheet opens the almanac of the place
            const int pages = 1 + (int)dataProbes().size();   // X-04: a giant's sheet, then its probes' profiles
            if (pages > 1 && (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_LEFT))) { dataPage = (dataPage + (in.wasPressed(KEY_RIGHT) ? 1 : pages - 1)) % pages; break; }
            bool any = false;
            for (int k = 0; k < KEY_MAX; k++) if (in.pressed[k]) any = true;
            if (any) { state = returnState; dataPage = 0; }
            break;
        }
    }
    probeAudio();   // X-01: the relay's sound for this frame
    // rendering
    switch (state) {
        case GameState::TITLE: renderSpace(); renderTitle(); break;
        case GameState::SPACE: renderSpace(); if (!photoMode) renderSpaceHUD(); break;
        case GameState::LANDING_MAP: renderLandingMap(); break;
        case GameState::DESCENT:
        case GameState::ASCENT: renderSurfaceScene(); drawVisor(HUD_DIM); drawTextCentered(canvas, UW / 2, UH - 10, state == GameState::DESCENT ? "SURFACE CAPSULE DESCENDING" : "RETURNING TO THE STARDRIFTER", HUD_AMBER); break;
        case GameState::SURFACE: renderSurfaceScene(); if (!photoMode) renderSurfaceHUD(); break;
        case GameState::PROBE: renderProbe(); break;   // X-01: the probe's camera
        case GameState::RECORDING: renderRecording(); break;   // X-04: a probe's recording
        case GameState::ALMANAC:   // W-01: over the scene it was opened from (the landing map keeps its picture)
            if (almanacScene == 2) renderLandingMap(); else if (almanacScene == 1 && surf.valid) renderSurfaceScene(); else renderSpace();
            renderAlmanac(); break;
        case GameState::HELP:
        case GameState::MENU:
        case GameState::SETTINGS:
        case GameState::SLOTS:
        case GameState::GUIDE:
        case GameState::TEXT_ENTRY:
        case GameState::STAR_MAP:
        case GameState::LOG:
        case GameState::STATS:
        case GameState::GALLERY:
        case GameState::SHIPSCREEN:
        case GameState::CONSOLE:
        case GameState::SECTOR_MAP:
        case GameState::SYSTEM_LIST:
        case GameState::DATA:
        case GameState::KEYS:
        case GameState::SHARDS:
            if (state == GameState::TEXT_ENTRY && (guideReturn == GameState::PROBE || guideReturn == GameState::RECORDING)) {   // X-04: naming the storm over the probe's screen
                if (guideReturn == GameState::PROBE) renderProbe(); else renderRecording();
                renderTextEntry();
                break;
            }
            if (returnState == GameState::SURFACE || (surf.valid && returnState == GameState::SURFACE)) renderSurfaceScene();
            else if (returnState == GameState::TITLE) { renderSpace(); renderTitle(); }
            else renderSpace();
            if (state == GameState::HELP) renderHelp();
            else if (state == GameState::MENU) renderMenu();
            else if (state == GameState::SETTINGS) renderSettings();
            else if (state == GameState::KEYS) renderKeysScreen();
            else if (state == GameState::SLOTS) renderSlots();
            else if (state == GameState::GUIDE) renderGuideMenu();
            else if (state == GameState::TEXT_ENTRY) { if (guideReturn == GameState::SHARDS) renderShards(); else if (guideReturn == GameState::SECTOR_MAP) renderSectorMap(); renderTextEntry(); }   // C-04: naming a piece, over the decoder; R-408: a mark, over the map
            else if (state == GameState::STAR_MAP) renderStarMap();
            else if (state == GameState::LOG) renderLog();
            else if (state == GameState::STATS) renderStats();
            else if (state == GameState::GALLERY) renderGallery();
            else if (state == GameState::SHIPSCREEN) renderShipScreen();
            else if (state == GameState::CONSOLE) renderConsole();
            else if (state == GameState::SHARDS) renderShards();   // C-06
            else if (state == GameState::SECTOR_MAP) renderSectorMap();
            else if (state == GameState::SYSTEM_LIST) renderSystemList();
            else renderDataSheet();
            break;
    }
}

void Game::testAimAtNearestStar() {
    const Star* best = nullptr; double bd = 1e300;
    for (const Star& s : nb.stars) {
        if (sys.valid && s.seed == sys.star.seed) continue;
        double d = length2(s.pos - ship.pos);
        if (d < bd) { bd = d; best = &s; }
    }
    if (!best) return;
    Vec3 fwd = normalize(best->pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
}

void Game::testAimAtBody(int body) {
    if (body < 0 || body >= (int)sys.bodies.size()) return;
    Vec3 fwd = normalize(sys.bodyPos(body, t) - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
}

std::string Game::testRoadInfo() const {   // C-09
    std::string s = fmt("zoom roads %d (%.0f ms); site roads %zu (%.1f ms, %s, wear %.2f)", (int)zoomRoads.size(), zoomRoadsMs, surf.site.roads.size(), surf.site.roadsMs, surf.site.roadCultures[0].paved ? "paved" : "beaten", surf.site.roadCultures[0].roadWear);
    double d, along, hd; const SiteRoad* rd;
    if (!surf.site.roads.empty() && surf.site.roadAt(surf.player.x, surf.player.z, 30, d, along, hd, rd))
        s += fmt("; the nearest road %.1f m off (half %.1f), %.0f m along, heading %.0f, left %.2f, cover at the feet %.2f", d, rd->half, along, wrap2pi(hd) / DEG, roadLeft(rd->id, along, rd->wear), surf.site.roadCover(surf.player.x, surf.player.z));
    else s += "; no road within 30 m";
    s += fmt("; roads met %zu", roadsMet.size());
    return s;
}

bool Game::testWalkToRoad(double withinM) {   // C-09
    double d, along, hd; const SiteRoad* rd;
    if (surf.site.roads.empty() || !surf.site.roadAt(surf.player.x, surf.player.z, withinM, d, along, hd, rd)) return false;
    double bestD = 1e9, bx = 0, bz = 0;   // the nearest point of its centre line
    for (size_t i = 0; i + 1 < rd->x.size(); i++) {
        double ax = rd->x[i], az = rd->z[i], ex = rd->x[i + 1] - ax, ez = rd->z[i + 1] - az, L2 = ex * ex + ez * ez;
        double t = L2 > 1e-9 ? clampd(((surf.player.x - ax) * ex + (surf.player.z - az) * ez) / L2, 0, 1) : 0;
        double qx = ax + ex * t, qz = az + ez * t, dd = std::hypot(surf.player.x - qx, surf.player.z - qz);
        if (dd < bestD) { bestD = dd; bx = qx; bz = qz; }
    }
    surf.player.x = bx; surf.player.z = bz; surf.player.y = surf.site.surfaceHeight(bx, bz); surf.player.yaw = hd; surf.player.vx = surf.player.vz = 0;
    return true;
}

void Game::testLandSite() {
    if (landBody < 0 || landBody >= (int)sys.bodies.size()) return;
    const PlanetMap& m = spaceR.mapFor(sys.bodies[landBody]);
    Mat3 frame = sys.bodyFrame(landBody, t);
    Vec3 sunB = frame * normalize(sys.star.pos - sys.bodyPos(landBody, t));
    double anyLat = 1e9, anyLon = 0;
    setDrainageEnabled(false);   // S-01: the level check probes the relief without the rivers' tiles (a tile per probe would cost seconds)
    for (int pass = 0; pass < 2; pass++) {   // grassland first (the herd), then any land
        Rng r(sys.bodies[landBody].seed ^ 0x1A4DULL);
        int probed = 0;
        for (int k = 0; k < 400 && probed < 24; k++) {
            double la = r.range(-60 * DEG, 60 * DEG), lo = r.range(-PI, PI);
            if (dot(StarSystem::bodyFromLatLon(la, lo), sunB) < 0.3) continue;
            if (pass == 0 && m.materialAt(lo, la) != MAT_GRASS) continue;
            bool land = true;   // the texel and its neighbours two degrees out: a coast or a lake shore is still a swim
            for (int j = -1; j <= 1 && land; j++)
                for (int i = -1; i <= 1 && land; i++)
                    if (m.materialAt(lo + i * 2 * DEG, la + j * 2 * DEG) == MAT_WATER) land = false;
            if (!land) continue;
            if (anyLat > 1e8) { anyLat = la; anyLon = lo; }
            SurfaceSite probe; probe.init(&sys, landBody, la, lo, t); probed++;   // S-01: level enough for the drive (the grassland of a locked world's first site lay on a mountainside: 18 m of drive)
            if (siteSlope(probe) < 0.1) { setDrainageEnabled(true); landLat = la; landLon = lo; return; }
        }
    }
    setDrainageEnabled(true);
    if (anyLat < 1e8) { landLat = anyLat; landLon = anyLon; }
}

int Game::testLandableBody() const {
    for (auto& b : sys.bodies) if (PLANET_TYPES[b.type].landable && b.parent < 0 && b.type != PT_COMET) return b.index;   // O6-03: the flow's buggy steps need gravity (a comet came first since GEN 9)
    for (auto& b : sys.bodies) if (PLANET_TYPES[b.type].landable && b.type != PT_COMET) return b.index;
    for (auto& b : sys.bodies) if (PLANET_TYPES[b.type].landable && b.parent < 0) return b.index;
    for (auto& b : sys.bodies) if (PLANET_TYPES[b.type].landable) return b.index;
    return -1;
}

void Game::testParkAt(const Star& s, int body) {
    arriveAtStar(s, Vec3(0, 0, -1));
    if (body < 0 || body >= (int)sys.bodies.size()) return;
    const Body& b = sys.bodies[body];
    Vec3 bp = sys.bodyPos(body, t);
    Vec3 toStar = normalize(sys.star.pos - bp);
    Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
    ship.parkDir = normalize(toStar * 0.7 + side * 0.7 + Vec3(0, 0.2, 0));
    ship.parkDist = parkDistanceFor(b);
    ship.parkedBody = body; ship.localTarget = body; ship.mode = ShipState::PARKED;
    ship.pos = bp + ship.parkDir * ship.parkDist;
    Vec3 fwd = normalize(bp - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(fwd.y);
    nb.update(ship.pos);
    state = GameState::SPACE;
    if (PLANET_TYPES[b.type].landable) beginLanding();
}

std::string Game::testBeltDraw() const {
    int n3 = 0, x0 = 1 << 30, x1 = -1, y0 = 1 << 30, y1 = -1;
    for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) { Pix p = fb.idx[y * FBW + x]; if (bankOf(p) == 20 && intenOf(p) > 0) { n3++; x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); y1 = std::max(y1, y); } }
    int a, b; const_cast<Game*>(this)->choosePaletteBodies(a, b);
    BeltRock rk; std::string where = "no rock";
    if (beltRockNow(rk)) { Vec3 v = viewBasis() * (rk.pos - ship.pos); Vec3 f = normalize(rk.pos - ship.pos); where = fmt("rock at view (%.0f, %.0f, %.0f) km, %.1f km across, ship %.0f km off; ship yaw %.2f pitch %.2f, rock yaw %.2f pitch %.2f, cabin %.2f/%.2f", v.x, v.y, v.z, 2 * rk.radiusKm, length(rk.pos - ship.pos), ship.yaw, ship.pitch, std::atan2(f.x, f.z), std::asin(clampd(f.y, -1, 1)), cabin.yaw, cabin.pitch); }
    return fmt("rocks drawn %d (%d meshes), bank-20 pixels %d in x %d..%d y %d..%d, palette bodies %d/%d, cabin %d; %s", spaceR.lastBeltRocks, spaceR.lastBeltMeshes, n3, x0, x1, y0, y1, a, b, (int)settings.cabin, where.c_str());
}

void Game::testParkAtBelt(const Star& s, int k) {
    arriveAtStar(s, Vec3(0, 0, -1));
    if (k < 0 || k >= (int)sys.belts.size()) return;
    startApproachBelt(k);
    ship.flightDur = 0.001;   // the next frame completes the approach
    state = GameState::SPACE;
}

std::string Game::testHealth() const {
    auto bad = [](double v) { return !std::isfinite(v); };
    std::string r;
    if (bad(ship.pos.x) || bad(ship.pos.y) || bad(ship.pos.z)) r += "ship.pos ";
    if (bad(ship.yaw) || bad(ship.pitch)) r += "ship.angles ";
    if (bad(t) || bad(timeWarp) || bad(realTime)) r += "time ";
    if (surf.valid && (bad(surf.player.x) || bad(surf.player.y) || bad(surf.player.z) || bad(surf.player.yaw) || bad(surf.player.pitch))) r += "player ";
    if (surf.valid && (bad(surf.capsuleX) || bad(surf.capsuleZ))) r += "capsule ";
    return r;
}

void Game::testTypeText(const std::string& s) {
    Input in;
    for (char c : s) {
        in.newFrame();
        int k = c == ' ' ? KEY_SPACE : (c == '-' ? KEY_MINUS : (c >= '0' && c <= '9' ? KEY_0 + (c - '0') : KEY_A + (toupper((unsigned char)c) - 'A')));
        in.pressed[k] = true; in.down[k] = true;
        frame(in, 1.0 / 30);
    }
    in.newFrame(); in.pressed[KEY_ENTER] = true; in.down[KEY_ENTER] = true; frame(in, 1.0 / 30);
}

std::string Game::testDroneInfo() const {   // R-403
    const Drone& d = surf.drone;
    return fmt("deployed=%d in=%d landed=%d speed=%.1f m/s alt=%.0f m vs=%+.1f heading=%.0f odometer=%.0f m rotor=%.2f chase=%d", (int)d.deployed, (int)surf.inDrone, (int)d.landed, d.speed, d.altAboveGround, d.vy, wrap2pi(d.heading) / DEG, d.odometer, d.rotor, (int)surf.chaseCam);
}

bool Game::testFlySetup(double altM) {   // R-403
    if (state != GameState::SURFACE || !surf.valid) return false;
    if (surf.inBuggy) surf.toggleBuggy();
    surf.relocateCapsule(surf.player.x + 5, surf.player.z + 3);
    if (!surf.drone.deployed && !surf.deployDrone()) return false;
    surf.drone.unfold = 1;
    surf.player.x = surf.drone.x + 1.5; surf.player.z = surf.drone.z;
    if (!surf.inDrone && !surf.toggleDrone()) return false;
    surf.drone.landed = false; surf.drone.rotor = 0.8; surf.drone.y = surf.site.surfaceHeight(surf.drone.x, surf.drone.z) + altM; surf.drone.altAboveGround = altM;
    surf.player.y = surf.drone.y;
    return true;
}

std::string Game::testBuggyInfo() const {
    const Buggy& b = surf.buggy;
    return fmt("deployed=%d in=%d speed=%.1f m/s heading=%.0f odometer=%.0f m tracks=%zu chase=%d", (int)b.deployed, (int)surf.inBuggy, b.speed, wrap2pi(b.heading) / DEG, b.odometer, b.tracks.size(), (int)surf.chaseCam);
}

std::string Game::testDebugInfo() const {
    std::string r = fmt("mode=%d parked=%d local=%d belt=%d/%d yaw=%.2f pitch=%.2f", (int)ship.mode, ship.parkedBody, ship.localTarget, ship.targetBelt, ship.parkedBelt, ship.yaw, ship.pitch);
    if (ship.parkedBody >= 0 && ship.parkedBody < (int)sys.bodies.size()) {
        Vec3 fwd = normalize(sys.bodyPos(ship.parkedBody, t) - ship.pos);
        r += fmt(" bodyYaw=%.2f bodyPitch=%.2f dist=%.0f R=%.0f", std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)), length(sys.bodyPos(ship.parkedBody, t) - ship.pos), sys.bodies[ship.parkedBody].radiusKm);
    }
    return r;
}

std::string Game::testGraveInfo() const {   // C-12
    const SurfaceView::NearGrave& ng = surf.nearGrave;
    std::string last = guide.log.empty() ? std::string("-") : guide.log.back().kind + ": " + guide.log.back().text;
    return fmt("grave: %s; %zu graves in the guide; last log: %s", ng.k >= 0 ? fmt("%s (%.1f m, %s)", ng.line.c_str(), ng.dist, SETTLEMENT_CLASS_NAMES[ng.sclass]).c_str() : "none within reach", guide.graves.size(), last.c_str());
}
bool Game::testWalkToGrave() {   // C-12: a metre in front of the nearest settlement's first stone, facing it
    if (!surf.valid || surf.inVehicle()) return false;
    bool found = false; double bestD = 1e18, sx = 0, sz = 0, yaw = 0;
    surf.forNearbyRuins(surf.player.x, surf.player.z, 3, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        if (r.kind != RK_SETTLEMENT || cell.graves.empty()) return;
        const Grave& g = cell.graves[0];
        double x = r.x + g.x, z = r.z + g.z, d = std::hypot(x - surf.player.x, z - surf.player.z);
        if (d < bestD) { bestD = d; sx = x + std::sin(g.heading); sz = z + std::cos(g.heading); yaw = g.heading + PI; found = true; }
    });
    if (!found) return false;
    surf.player.x = sx; surf.player.z = sz; surf.player.y = surf.site.surfaceHeight(sx, sz); surf.player.yaw = yaw; surf.player.pitch = -0.35;
    return true;
}

bool Game::testWalkToShard() {   // C-13: a metre from the nearest settlement's first shard not yet taken, facing it
    if (!surf.valid || surf.inVehicle()) return false;
    bool found = false; double bestD = 1e18, sx = 0, sz = 0, yaw = 0;
    surf.forNearbyRuins(surf.player.x, surf.player.z, 3, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        if (r.kind != RK_SETTLEMENT) return;
        for (const ShardSite& st : cell.shards) {
            if (surf.shardsFound.count(st.index)) continue;
            double x = r.x + st.x, z = r.z + st.z, d = std::hypot(x - surf.player.x, z - surf.player.z);
            if (d < bestD) { bestD = d; sx = x + std::sin(st.heading); sz = z + std::cos(st.heading); yaw = st.heading + PI; found = true; }
            break;
        }
    });
    if (!found) return false;
    surf.player.x = sx; surf.player.z = sz; surf.player.y = surf.site.surfaceHeight(sx, sz); surf.player.yaw = yaw; surf.player.pitch = -0.5;
    return true;
}
