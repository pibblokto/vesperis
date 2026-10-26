// R-419 (2026-10-18): auto-explore, the user's request for the 1.3.2 patch: "autoexploration. Some user requested it so game plays itself.
// What I suggest - several profiles (like focus on space, on planets, on surface of planets, on civilizations, gas giants, random - you
// choose and give ideas). Also I suggest no HUD while it's on and make sure movements are soft, non-robotic, everything is in chilling
// manner. So it feels like real person is playing".
//
// The explorer plays on its own. A mind keeps a short plan of steps for the scene it is in (the cabin and the ship, the ground, the probe's
// camera; the landing map and the capsule's fall pass between them) as the profile likes: the wanderer a new whim at each star, the
// stargazer the stars and the long flights, planetfall the worlds from orbit, the surveyor the ground, the archaeologist the old peoples'
// ruins and records, the giants profile the gas giants and the probes dropped into them. Hands turn the step of the moment into the frame's
// input as a person's hand gives it: the gaze eases off and onto what it looks at under a spring with a hand's limits and sways a few
// tenths of a degree at rest; the stick is pushed and let back, never a key's on and off; the ship's nose and its laps round a world ease
// in and out, the clock's pace in decades. The game reads that input as it reads the player's, so every rule holds (the walls, the slopes,
// the water, the vehicles' handling, the reach of a record); what it finds is the expedition's. No HUD meanwhile; any key, click, mouse
// motion or button gives the controls back. The plans are autoexplore_space.cpp and autoexplore_ground.cpp, the hands
// autoexplore_hands.cpp.
#include "game.h"
#include "autoexplore_internal.h"
#include "peoples/plan.h"
#include "galaxy/probe.h"
#include "peoples/signals.h"
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstring>
#include <ctime>
#include <queue>
#include <algorithm>

const char* const AUTO_PROFILE_NAMES[AP_COUNT] = {"WANDERER", "STARGAZER", "PLANETFALL", "SURVEYOR", "ARCHAEOLOGIST", "GIANTS"};
const char* const AUTO_PROFILE_LINES[AP_COUNT] = {"A LITTLE OF EVERYTHING, AT RANDOM", "STARS, SKIES AND LONG FLIGHTS", "WORLDS FROM ORBIT, MOONS AND RINGS",
                                                  "THE GROUND: WALKS, DRIVES, FLIGHTS", "THE OLD PEOPLES' RUINS AND RECORDS", "GAS GIANTS AND PROBES INTO THEM"};

namespace {
enum AutoScene { AS_NONE = 0, AS_SPACE, AS_LANDING, AS_FALL, AS_GROUND, AS_PROBE, AS_TITLE, AS_SCREEN };
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// beginning, ending

// the explorer plays on its own from this frame (`seed` 0: the clock's)
void Game::autoStart(int profile, uint64_t seed) {
    const int p = clampi(profile, 0, AP_COUNT - 1);
    ax = AutoState();
    ax.on = true; ax.profile = p; ax.focus = p == AP_WANDERER ? AP_PLANETFALL : p;
    ax.rng = Rng(seed ? seed : ((uint64_t)std::time(nullptr) * 0x9E3779B97F4A7C15ULL) ^ (uint64_t)(realTime * 1e6) ^ (uint64_t)(p + 1));
    for (double& ph : ax.swayPh) ph = ax.rng.range(0, TAU);
    ax.circleDir = ax.rng.chance(0.5) ? 1 : -1;
    settings.autoProfile = p; settings.save(settingsPath);
    if (photoMode) togglePhotoMode();
    if (tele.on) telescopeOff();
    if (radar.on) radarOff();
    ship.targeting = false; jumpDigits.clear();
    if (almanacRunning) almanacStop(false);
    if (settings.clockMode == 0) timeWarp = 1;
    if (state != GameState::SPACE && state != GameState::SURFACE) state = returnState == GameState::SURFACE && surf.valid ? GameState::SURFACE : GameState::SPACE;
    padUI.menu = false; padUI.keyboard = false;
    statusMsg.clear(); statusNext.clear();
    audio.beep = BEEP_LOCK;
    tapeList();                                  // R-420: the tape's tracks (its hint shown if there are any)
    if (settings.autoMusic) tapeStart(false);    // taken up again where it was left
}

// the controls given back
void Game::autoStop() {
    if (!ax.on) return;
    ax.on = false; ax.plan.clear();
    if (settings.clockMode == 0 && timeWarp > 1 && !almanacRunning) timeWarp = 1;
    if (surf.valid) { surf.scanWanted = scanMode >= 0; surf.player.autoWalk = 0; }
    statusNext.clear();   // what its steps queued (a probe's chute hint) is not the player's
    status("AUTO-EXPLORE OFF - THE CONTROLS ARE YOURS", 4);
    audio.beep = BEEP_UI;
    tapeHalt(false);   // R-420: the music fades with it, its place kept
}

// a key, a click, the mouse or a button: the end (true), the press swallowed
// any key, click or turn of the wheel, a hand on the mouse (a few pixels at once, or a dozen within a moment: a nudge of the desk is not
// one; none in a scene's first half second, when the window takes the mouse) or the controller's buttons and sticks (their keys and moves,
// `padFrame` having run) end it; the press that does is not acted on. R-420: but M (the controller's SELECT) starts and stops the tape
bool Game::autoTakeover(Input& in, double realDt) {
    if (in.pressed[KEY_M] && !in.ctrl()) { in.pressed[KEY_M] = false; tapeToggle(); }
    bool any = in.wheel != 0 || in.mousePressed[0] || in.mousePressed[1] || in.mousePressed[2] || in.moveX != 0 || in.moveY != 0;
    for (int k = 32; k < KEY_MAX && !any; k++) any = in.pressed[k];
    const double m = ax.clock < 0.6 || ax.sceneT < 0.5 ? 0.0 : std::fabs(in.mouseDx) + std::fabs(in.mouseDy);
    ax.mouseAcc = ax.mouseAcc * std::exp(-realDt * 3) + m;
    if (m > 6 || ax.mouseAcc > 14) any = true;
    if (!any) return false;
    autoStop();
    memset(in.pressed, 0, sizeof(in.pressed)); memset(in.mousePressed, 0, sizeof(in.mousePressed));
    in.mouseDx = in.mouseDy = 0; in.wheel = 0;
    return true;
}

int Game::autoScene() const {
    switch (state) {
        case GameState::SPACE: return AS_SPACE;
        case GameState::LANDING_MAP: return AS_LANDING;
        case GameState::DESCENT: case GameState::ASCENT: return AS_FALL;
        case GameState::SURFACE: return AS_GROUND;
        case GameState::PROBE: return AS_PROBE;
        case GameState::TITLE: return AS_TITLE;
        default: return AS_SCREEN;
    }
}

// what the mouse turns now: the head in the cabin, the ship's nose in the cockpit view and the radar's camera, the explorer's look on the
// ground, a vehicle's camera on its gimbal, the probe's camera; nothing at the eyepiece (the telescope holds its view), in the capsule's fall
int Game::autoLookFrame() const {
    switch (state) {
        case GameState::SPACE: return tele.on ? LF_NONE : (radar.on || !settings.cabin ? LF_SHIP : LF_CABIN);
        case GameState::SURFACE: return photoMode ? LF_NONE : (surf.inVehicle() ? LF_VEHICLE : LF_GROUND);
        case GameState::PROBE: return probe.active && probe.endClock < 0 ? LF_PROBE : LF_NONE;
        default: return LF_NONE;
    }
}

// the frame's input made by the explorer
void Game::autoInput(Input& in, double realDt) {
    AutoState& A = ax;
    A.clock += realDt; A.captionT += realDt;
    A.taps.clear(); memset(A.hold, 0, sizeof(A.hold));
    A.wantFwd = A.wantSide = 0; A.shipAim = false; A.wantOrbitE = A.wantOrbitN = 0; A.lookSet = false;
    A.sprintTapT -= realDt;
    const int scene = autoScene();
    if (scene != A.scene) {
        if (state == GameState::DESCENT) A.groundBegun = false;   // a new landing
        A.plan.clear(); A.scene = scene; A.sceneT = 0; A.stepT = 0; A.stepFirst = true; A.warpWant = 1;
    } else A.sceneT += realDt;
    switch (scene) {
        case AS_SPACE: case AS_GROUND: case AS_PROBE: autoRun(realDt); break;
        case AS_LANDING: autoLandingMap(); break;
        case AS_TITLE: if (A.sceneT > 1.5) { autoTap(KEY_ENTER); A.sceneT = 0; } break;
        case AS_SCREEN: if (A.sceneT > 0.8) { autoTap(KEY_ESCAPE); A.sceneT = 0; } break;   // a screen it did not mean to open: closed
        default: break;
    }
    autoHands(in, realDt);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the mind: the plan and its steps

void Game::autoPush(const char* what, double maxT, std::function<bool(double)> fn, bool timed) {
    AutoStep s;
    s.what = what;
    s.maxT = maxT;
    s.run = std::move(fn);
    s.timed = timed;
    ax.plan.push_back(std::move(s));
}

void Game::autoRun(double dt) {
    AutoState& A = ax;
    if (A.plan.empty()) { autoPlan(); A.stepT = 0; A.stepFirst = true; }
    if (A.plan.empty()) return;
    if (A.stepFirst) { A.goX = A.goZ = 1e18; A.stillT = 0; A.unstick = 0; A.jetT = 0; A.reverseT = 0; A.driveEvalT = 0; A.way.clear(); A.wayI = 0; A.everyT = 0; }
    AutoStep& s = A.plan.front();
    A.doing = s.what;
    const bool done = s.run(dt);
    A.stepFirst = false; A.stepT += dt;
    if (A.abort) { A.abort = false; A.plan.clear(); A.stepT = 0; A.stepFirst = true; return; }
    if (done || A.stepT > s.maxT) {
        if (!done && !s.timed) A.timeouts++;
        A.plan.pop_front(); A.steps++;
        A.stepT = 0; A.stepFirst = true;
    }
}

void Game::autoPlan() {
    switch (ax.scene) {
        case AS_SPACE: autoPlanSpace(); break;
        case AS_GROUND: autoPlanGround(); break;
        case AS_PROBE: autoPlanProbe(); break;
        default: break;
    }
    if (ax.plan.empty()) autoPush("a breath", 2.0, [](double) { return false; }, true);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the chooser (the menu's AUTO-EXPLORE, the controller's commands)

void Game::openAutoScreen(GameState back) { autoSel = clampi(settings.autoProfile, 0, AP_COUNT - 1); autoBack = back; state = GameState::AUTO; audio.beep = BEEP_UI; tapeList(); }

void Game::updateAutoScreen(const Input& in, double dt, double realDt) {
    const int n = AP_COUNT + 1;   // the profiles and BACK
    if (menuStep(in, autoSel, n)) audio.beep = BEEP_UI;
    if (in.wasPressed(KEY_ESCAPE) || (enterKey(in) && autoSel == AP_COUNT)) { state = autoBack; audio.beep = BEEP_UI; return; }
    if (enterKey(in)) autoStart(autoSel);
}

void Game::renderAutoScreen() {
    const int top = 30, bottom = UH - 30, x0 = 8, x1 = UW - 8;
    drawPanel(canvas, x0, top, x1, bottom, rgb(0, 0, 0), 215);
    drawTextCentered(canvas, UW / 2, top + 8, "AUTO-EXPLORE", HUD_AMBER);
    for (int i = 0; i <= AP_COUNT; i++) {
        const int y = top + 24 + i * 12;
        const bool sel = i == autoSel;
        if (i == AP_COUNT) { drawTextCentered(canvas, UW / 2, y + 2, "BACK", sel ? HUD_WHITE : HUD_GREEN); if (sel) drawText(canvas, UW / 2 - 30, y + 2, ">", HUD_AMBER); continue; }
        drawText(canvas, 18, y, AUTO_PROFILE_NAMES[i], sel ? HUD_WHITE : HUD_GREEN);
        drawText(canvas, 18 + 14 * FONT_ADV, y, AUTO_PROFILE_LINES[i], sel ? HUD_WHITE : HUD_DIM);
        if (sel) drawText(canvas, 11, y, ">", HUD_AMBER);
    }
    if (!tape.tracks.empty()) {   // R-420: the tape
        drawTextCentered(canvas, UW / 2, bottom - 30, padInUse() ? "A BEGINS  B BACK" : "ENTER BEGINS  ESC BACK", HUD_DIM);
        drawTextCentered(canvas, UW / 2, bottom - 20, padInUse() ? "THEN SELECT STARTS AND STOPS THE MUSIC" : "THEN M STARTS AND STOPS THE MUSIC", HUD_DIM);
        drawTextCentered(canvas, UW / 2, bottom - 10, padInUse() ? "AND ANY OTHER BUTTON TAKES THE CONTROLS BACK" : "AND ANY OTHER KEY TAKES THE CONTROLS BACK", HUD_DIM);
        return;
    }
    drawTextCentered(canvas, UW / 2, bottom - 20, padInUse() ? "A BEGINS  B BACK" : "ENTER BEGINS  ESC BACK", HUD_DIM);
    drawTextCentered(canvas, UW / 2, bottom - 10, padInUse() ? "THEN ANY BUTTON TAKES THE CONTROLS BACK" : "THEN ANY KEY TAKES THE CONTROLS BACK", HUD_DIM);
}
