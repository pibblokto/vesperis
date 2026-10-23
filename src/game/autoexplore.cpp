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
// motion or button gives the controls back.
#include "game.h"
#include "galaxy/plan.h"
#include "galaxy/probe.h"
#include "galaxy/signals.h"
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
enum AutoLook { LF_NONE = 0, LF_CABIN, LF_SHIP, LF_GROUND, LF_VEHICLE, LF_PROBE };
enum AutoOuting { AO_VISTA = 0, AO_WATER, AO_LIFE, AO_SIGHT, AO_DRIVE, AO_FLY, AO_RUINS, AO_SKY, AO_COUNT };
enum AutoTravel { TR_FOOT = 0, TR_BUGGY, TR_DRONE };

// a profile's taste (the wanderer plays with another's at each star)
struct AutoTaste {
    int bodies0, bodies1;              // the bodies it visits in a system before the next star
    double longJump;                   // a move's chance of being a long jump (60 to 3,000 ly) rather than one to a neighbour
    double land, probe;                // its chance to land at a landable world, to drop a probe into a giant
    double tele, roof, glass, orbit;   // at a parking: the telescope, the roof deck, the glass hull, a lap round the world
    double stay0, stay1;               // seconds of looking at a parked world
    double ground0, ground1;           // seconds on the ground
    double w[AO_COUNT];                // the ground's outings: a view, the water, the life, a sight, a drive, a flight, the ruins, the sky
};
const AutoTaste TASTES[AP_COUNT] = {
    {1, 3, 0.15, 0.5, 0.4, 0.3, 0.3, 0.25, 0.4, 10, 20, 120, 240, {1.5, 1, 1.2, 1.2, 0.8, 0.8, 1.5, 1}},       // WANDERER (its whim's in play)
    {0, 2, 0.35, 0.05, 0.1, 0.6, 0.5, 0.45, 0.3, 14, 28, 60, 100, {2, 0.5, 0.5, 0.5, 0, 0.3, 0, 3}},           // STARGAZER
    {3, 6, 0.08, 0.3, 0.15, 0.4, 0.3, 0.25, 0.7, 14, 26, 60, 110, {2, 1, 1, 1, 0.3, 0.6, 0.3, 1}},             // PLANETFALL
    {1, 2, 0.05, 0.95, 0.0, 0.15, 0.1, 0.1, 0.3, 6, 12, 260, 480, {2, 1.5, 2, 2, 1.5, 1.5, 0.6, 0.8}},        // SURVEYOR
    {1, 2, 0.04, 0.2, 0.0, 0.2, 0.1, 0.1, 0.25, 6, 12, 300, 600, {0.4, 0.2, 0.3, 0.7, 0.2, 0.3, 6, 0.2}},     // ARCHAEOLOGIST
    {1, 3, 0.1, 0.25, 0.85, 0.45, 0.35, 0.3, 0.5, 12, 24, 90, 180, {1, 0.3, 0.5, 0.5, 0.3, 0.5, 0.2, 4}},     // GIANTS
};
// the stargazer's draw by a star's class: the strange and the rare first
double classDraw(int cls) {
    static const double W[STAR_DRAWN_COUNT] = {1, 1, 3, 3, 2, 4, 0.5, 1.5, 2, 3, 4, 5, 5, 6};
    return cls >= 0 && cls < STAR_DRAWN_COUNT ? W[cls] : 1;
}
inline Vec3 forwardOf(double yaw, double pitch) { return Vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)); }
inline double angleBetween(const Vec3& a, const Vec3& b) { return std::acos(clampd(dot(normalize(a), normalize(b)), -1, 1)); }
inline bool civWorld(int type) { return kindHas(type, KT_PEOPLE); }   // the only worlds a people lived on (C-01)
const double CAB_WINDOW_X = 0.62, CAB_WINDOW_Z = 1.72;   // where the explorer stands at the front window: between its bars (at 0 and 1.25 m), the desk at 1.95
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// beginning, ending

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
    audio.beep = 1;
    tapeList();                                  // R-420: the tape's tracks (its hint shown if there are any)
    if (settings.autoMusic) tapeStart(false);    // taken up again where it was left
}

void Game::autoStop() {
    if (!ax.on) return;
    ax.on = false; ax.plan.clear();
    if (settings.clockMode == 0 && timeWarp > 1 && !almanacRunning) timeWarp = 1;
    if (surf.valid) { surf.scanWanted = scanMode >= 0; surf.player.autoWalk = 0; }
    statusNext.clear();   // what its steps queued (a probe's chute hint) is not the player's
    status("AUTO-EXPLORE OFF - THE CONTROLS ARE YOURS", 4);
    audio.beep = 4;
    tapeHalt(false);   // R-420: the music fades with it, its place kept
}

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

void Game::autoPush(const char* what, double maxT, std::function<bool(double)> fn, bool timed) { AutoStep s; s.what = what; s.maxT = maxT; s.run = std::move(fn); s.timed = timed; ax.plan.push_back(std::move(s)); }

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
// space: the cabin's window, the systems, the moves between the stars

void Game::autoPlanSpace() {
    AutoState& A = ax;
    if (tele.on) { autoPush("the telescope stowed", 1, [this](double) { if (tele.on) autoTap(KEY_Z); return true; }); return; }
    if (radar.on && ship.mode != ShipState::VIMANA) { autoPush("the radar off", 1, [this](double) { if (radar.on) autoTap(KEY_B); return true; }); return; }
    if (settings.cabin && !autoAtWindow()) autoPush("to the window", 14, [this](double) { return autoCabinWalk(CAB_WINDOW_X, CAB_WINDOW_Z, 0.0); });
    if (ship.mode == ShipState::VIMANA) { autoPlanFlight(); return; }
    if (ship.mode == ShipState::APPROACH) { autoPush("the approach", 60, [this](double) { autoWindowLook(0.4); return ship.mode != ShipState::APPROACH; }); return; }
    if (!sys.valid) { autoPlanMove(); return; }
    if (sys.star.seed != A.sysSeed) { autoPlanArrival(); if (!A.plan.empty()) return; }   // begun at a parking: on to the world at once
    if (ship.mode == ShipState::PARKED && ship.parkedBody >= 0 && ship.parkedBody < (int)sys.bodies.size() && !A.bodyDone.count(ship.parkedBody)) { autoPlanParked(ship.parkedBody); return; }
    const int next = A.visits < A.quota ? autoPickBody() : -1;
    if (next >= 0) autoPlanVisit(next); else autoPlanMove();
}

// a new system: a whim for the wanderer, how many worlds to see; the star the ship came out at, then the brightest worlds in the window
void Game::autoPlanArrival() {
    AutoState& A = ax;
    if (A.sysSeed != 0) A.stars++;
    A.sysSeed = sys.star.seed; A.bodyDone.clear(); A.bodyTried.clear(); A.visits = 0; A.radarTried = false;
    if (A.profile == AP_WANDERER) A.focus = autoWhim();
    const AutoTaste& T = TASTES[A.focus];
    A.quota = T.bodies0 + A.rng.irange(T.bodies1 - T.bodies0 + 1);
    if (A.focus == AP_ARCHAEOLOGIST) {   // its people's worlds alone (none: on to the next star at once)
        A.quota = 0;
        for (const Body& b : sys.bodies) if (KINDS[b.type].landable && civWorld(b.type) && worldHadCivilisation(spaceR.genFor(b))) A.quota++;
    }
    if (A.focus == AP_GIANTS) {          // its giants and a moon of one (none: on)
        int giants = 0;
        for (const Body& b : sys.bodies) giants += isProbeGiant(b.type) ? 1 : 0;
        A.quota = giants ? std::min(giants + 1, 1 + A.rng.irange(giants + 1)) : 0;
    }
    if (ship.mode == ShipState::PARKED && ship.parkedBody >= 0) return;   // begun at a parking: the world first (the next plan)
    autoPush("the star", A.rng.range(6, 9), [this](double) { autoLookSpace(sys.star.pos - ship.pos, 0.6); return false; }, true);
    for (int b : autoBodiesInView(2)) autoPush("a world in the window", A.rng.range(3, 5), [this, b](double) {
        if (b >= (int)sys.bodies.size()) return true;
        autoLookSpace(sys.bodyPos(b, t) - ship.pos, 0.7); return false;
    }, true);
}

// the worlds in front of the ship with the widest discs (the arrival's glances)
std::vector<int> Game::autoBodiesInView(int n) {
    std::vector<std::pair<double, int>> v;
    const Mat3 cam = cameraBasis(ship.yaw, ship.pitch);
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        const Vec3 d = cam * (sys.bodyPos(i, t) - ship.pos);
        const double L = length(d);
        if (L < 1 || d.z < 0.55 * L || std::fabs(d.y) > 0.45 * L) continue;   // through the front window
        v.push_back({sys.bodies[i].radiusKm / L, i});
    }
    std::sort(v.begin(), v.end(), [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first > b.first; });
    std::vector<int> out;
    for (size_t k = 0; k < v.size() && (int)k < n; k++) if (v[k].first > 0.0015) out.push_back(v[k].second);
    return out;
}

// the wanderer's whim at a star: any taste, the system's own leading (its giants, a people's world, ground to walk)
int Game::autoWhim() {
    double w[AP_COUNT] = {0, 1.0, 1.2, 1.3, 0.8, 0.8};
    bool giant = false, land = false, civ = false;
    for (const Body& b : sys.bodies) {
        giant = giant || isProbeGiant(b.type);
        if (!KINDS[b.type].landable || b.type == PT_COMET) continue;
        land = true;
        if (civWorld(b.type) && worldHadCivilisation(spaceR.genFor(b))) civ = true;
    }
    w[AP_GIANTS] = giant ? 2.0 : 0.05;
    w[AP_ARCHAEOLOGIST] = civ ? 5.0 : 0.4;
    if (!land) w[AP_SURVEYOR] = 0.05;
    return ax.rng.pick(w, AP_COUNT);
}

// the next world of the system to see, by the taste
int Game::autoPickBody() {
    AutoState& A = ax;
    std::vector<double> w(sys.bodies.size(), 0.0);
    double total = 0;
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        if (A.bodyTried.count(i)) continue;
        const Body& b = sys.bodies[i];
        if (b.type == PT_COMPANION) continue;
        const bool landable = KINDS[b.type].landable, giant = isProbeGiant(b.type), moon = b.parent >= 0;
        const bool giantMoon = moon && b.parent < (int)sys.bodies.size() && isProbeGiant(sys.bodies[b.parent].type);
        double x = 1;
        switch (A.focus) {
            case AP_STARGAZER: x = (b.rings ? 3 : 1) * (giant ? 2 : 1) * (moon ? 0.4 : 1); break;
            case AP_PLANETFALL: x = (1 + (b.rings ? 1.5 : 0) + 0.3 * std::min(6, b.moonCount)) * (kindHas(b.type, KT_FLORA) ? 1.5 : 1) * (moon ? 0.7 : 1) * (b.type == PT_COMET ? 0.6 : 1); break;
            case AP_SURVEYOR: x = !landable ? 0.03 : (kindHas(b.type, KT_FLORA) ? 6 : (kindHas(b.type, KT_PEOPLE) ? 3 : (b.type == PT_COMET ? 0.5 : 2))); break;
            case AP_ARCHAEOLOGIST: x = landable && civWorld(b.type) && worldHadCivilisation(spaceR.genFor(b)) ? 1 : 0; break;
            case AP_GIANTS: x = giant ? 10 : (giantMoon && landable ? 2 : 0); break;
            default: break;
        }
        w[i] = x; total += x;
    }
    if (total <= 0) return -1;
    return A.rng.pick(w.data(), (int)w.size());
}

// to a world of the system: the nose turned onto it, a moment, then the fine approach (Enter on the local target)
void Game::autoPlanVisit(int b) {
    AutoState& A = ax;
    A.bodyTried.insert(b);
    autoPush("turning to a world", 8, [this, b](double) {
        const Vec3 d = sys.bodyPos(b, t) - ship.pos;
        autoAimShip(d); autoWindowLook(0.35);
        return ax.stepT > 2.5 && autoShipOff(d) < 3 * DEG;
    });
    autoPush("a moment", A.rng.range(1.0, 2.5), [this, b](double) { autoAimShip(sys.bodyPos(b, t) - ship.pos); autoWindowLook(0.3); return false; }, true);
    autoPush("the fine approach", 45, [this, b](double) {
        if (ax.stepFirst) { ship.localTarget = b; ship.targetBelt = -1; autoTap(KEY_ENTER); return false; }
        autoGazeOverBody(b);   // the eyes on the world as it grows
        return ship.mode == ShipState::PARKED || (ship.mode == ShipState::STANDBY && ax.stepT > 1);
    });
}

// parked at a world: the eyes over its disc a while, then as the taste likes the glass hull, a lap round it, the roof deck, the telescope;
// then down to it in the capsule, or a probe into a giant
void Game::autoPlanParked(int b) {
    AutoState& A = ax;
    const AutoTaste& T = TASTES[A.focus];
    const Body& B = sys.bodies[b];
    A.bodyDone.insert(b); A.bodyTried.insert(b); A.visits++; A.bodies++;
    const bool landable = KINDS[B.type].landable, giant = isProbeGiant(B.type);
    const bool civ = landable && civWorld(B.type) && worldHadCivilisation(spaceR.genFor(B));
    double land = landable ? T.land : 0;
    if (A.focus == AP_ARCHAEOLOGIST) land = civ ? 1.0 : 0.0;
    else if (A.focus == AP_GIANTS) land = B.parent >= 0 && B.parent < (int)sys.bodies.size() && isProbeGiant(sys.bodies[B.parent].type) ? 0.6 : 0.03;   // a giant's moon: down under it
    else if (civ) land = std::max(land, 0.6);
    if (B.type == PT_COMET) land *= 0.4;
    const bool goingDown = landable && A.rng.chance(land);
    const double stay = A.rng.range(T.stay0, T.stay1) * (goingDown ? 0.6 : 1.0);
    const bool glass = settings.cabin && !cabin.depolarised && A.rng.chance(T.glass);
    if (glass) autoPush("the glass hull", 1.5, [this](double) { if (ax.stepFirst && !cabin.depolarised) autoTap(KEY_Y); autoWindowLook(0.4); return ax.stepT > 1; });
    autoPush("the world", stay, [this, b](double) { autoAimShip(sys.bodyPos(b, t) - ship.pos); autoGazeOverBody(b); return false; }, true);
    if (A.rng.chance(T.orbit)) {
        const double e = (A.rng.chance(0.5) ? 1 : -1) * A.rng.range(0.25, 0.55), n = A.rng.range(-0.15, 0.15), dur = A.rng.range(12, 22);
        autoPush("round the world", dur, [this, e, n, dur](double) {
            const double on = ax.stepT < dur - 3 ? 1.0 : 0.0;   // eased in and out by the hands
            ax.wantOrbitE = e * on; ax.wantOrbitN = n * on;
            autoWindowLook(0.25);
            return false;
        }, true);
    }
    if (settings.cabin && A.rng.chance(T.roof)) autoPlanRoof(b);
    if (A.rng.chance(T.tele)) autoPlanTelescope(b);
    if (glass) autoPush("the hull polarised", 1.5, [this](double) { if (ax.stepFirst && cabin.depolarised) autoTap(KEY_Y); autoWindowLook(0.4); return ax.stepT > 1; });
    if (giant && !probe.active && A.rng.chance(T.probe)) { autoPlanProbeLaunch(); return; }
    if (goingDown) autoPlanLanding(b);
}

// up on the roof deck: the world, a slow look round the sky, the world again; back down
void Game::autoPlanRoof(int b) {
    AutoState& A = ax;
    autoPush("up to the roof deck", 2, [this](double) { if (ax.stepFirst && !cabin.onRoof) autoTap(KEY_PAGE_UP); return ax.stepT > 1; });
    const double dur = A.rng.range(16, 30), turn = (A.rng.chance(0.5) ? 1 : -1) * A.rng.range(0.10, 0.16);
    autoPush("the roof deck", dur, [this, b, dur, turn](double) {
        AutoState& S = ax;
        const double u = S.stepT;
        if (u < 7 || u > dur - 6) { autoLookSpace(sys.bodyPos(b, t) - ship.pos, 0.45); S.driftYaw = cabin.yaw; }
        else autoLook(wrapAngle(S.driftYaw + (u - 7) * turn), 0.25 + 0.15 * std::sin(u * 0.2), 0.4);
        return false;
    }, true);
    autoPush("back into the cabin", 2, [this](double) { if (ax.stepFirst && cabin.onRoof) autoTap(KEY_PAGE_DOWN); return ax.stepT > 1; });
}

// the telescope on the world, or on one of its moons: the nose onto it, the eye to the eyepiece, a few powers closer one at a time, a
// long look; stowed
void Game::autoPlanTelescope(int b) {
    AutoState& A = ax;
    int target = b;
    std::vector<int> moons;
    for (const Body& m : sys.bodies) if (m.parent == b) moons.push_back(m.index);
    if (!moons.empty() && A.rng.chance(0.4)) target = moons[A.rng.irange((int)moons.size())];
    autoPush("aiming the telescope", 12, [this, target](double) {
        const Vec3 d = sys.bodyPos(target, t) - ship.pos;
        autoAimShip(d); autoEyesFront(0.8);   // the eye to the front first: the eyepiece faces it at once
        return ax.stepT > 2 && autoShipOff(d) < 1.5 * DEG && (autoLookFrame() != LF_CABIN || (std::fabs(cabin.yaw) < 0.025 && std::fabs(cabin.pitch) < 0.025));
    });
    autoPush("the eyepiece", 2, [this](double) { if (ax.stepFirst && !tele.on) autoTap(KEY_Z); return ax.stepT > 1.2; });
    const int steps = 2 + A.rng.irange(3);
    for (int k = 0; k < steps; k++) autoPush("closer", A.rng.range(3, 5), [this](double) { if (ax.stepFirst && tele.on) autoTap(KEY_PAGE_UP); return !tele.on; }, true);
    autoPush("through the telescope", A.rng.range(7, 12), [this](double) { return !tele.on; }, true);
    autoPush("the telescope stowed", 2, [this](double) { if (ax.stepFirst && tele.on) autoTap(KEY_Z); return ax.stepT > 1; });
}

// down to the world: a site chosen (its map made first), the walk to the capsule's cage and E there; the landing map takes the site
void Game::autoPlanLanding(int b) {
    AutoState& A = ax;
    const Body& B = sys.bodies[b];
    A.siteSet = false;
    const bool byPlace = civWorld(B.type) && worldHadCivilisation(spaceR.genFor(B)) && (A.focus == AP_ARCHAEOLOGIST || A.rng.chance(0.6));   // beside a place of its people
    const bool underGiant = B.parent >= 0 && B.parent < (int)sys.bodies.size() && isProbeGiant(sys.bodies[B.parent].type) && (A.focus == AP_GIANTS || A.rng.chance(0.5));
    const bool low = A.rng.chance(0.2);   // the sun low: the long shadows
    autoPush("a site for the capsule", 30, [this, b, byPlace, underGiant, low](double) {
        autoAimShip(sys.bodyPos(b, t) - ship.pos); autoGazeOverBody(b);
        return ax.stepT > 1.5 && autoChooseSite(b, byPlace, underGiant, low);
    });
    if (settings.cabin) autoPush("to the capsule", 16, [this](double) { if (!ax.siteSet) return true; return autoCabinToCapsule(); });
    else autoPush("the capsule", 1, [this](double) { if (ax.siteSet) autoTap(KEY_C); return true; });
    autoPush("the capsule deploying", 3, [this](double) { return !ax.siteSet || state != GameState::SPACE; });
}

// a probe into the giant: E at the capsule's cage (the giant takes a probe), its camera from then on
void Game::autoPlanProbeLaunch() {
    if (settings.cabin) autoPush("to the probe's bay", 16, [this](double) { return autoCabinToCapsule(); });
    else autoPush("the probe", 1, [this](double) { autoTap(KEY_C); return true; });
    autoPush("the probe away", 3, [this](double) { return state == GameState::PROBE; });
}

// the Vimana flight: at the front window, the eyes wandering; on a long flight a while at a side window first
void Game::autoPlanFlight() {
    AutoState& A = ax;
    const double left = std::max(1.0, ship.flightDur - ship.flightT);
    if (settings.cabin && left > 22 && A.rng.chance(0.4)) {
        const double side = A.rng.chance(0.5) ? 1.0 : -1.0;
        autoPush("to a side window", 9, [this, side](double) { return ship.mode != ShipState::VIMANA || autoCabinWalk(1.7 * side, 0.62, side * PI / 2); });
        autoPush("the side window", left * 0.45, [this, side](double) {
            AutoState& S = ax;
            if (S.clock > S.driftNext) { S.driftYaw = S.rng.sym(0.3); S.driftPitch = S.rng.range(-0.08, 0.12); S.driftNext = S.clock + S.rng.range(5, 9); }
            autoLook(side * PI / 2 + S.driftYaw, S.driftPitch, 0.4);
            return ship.mode != ShipState::VIMANA;
        }, true);
        autoPush("back to the front", 9, [this](double) { return autoCabinWalk(CAB_WINDOW_X, CAB_WINDOW_Z, 0.0); });
    }
    autoPush("the flight", left + 8, [this](double) { autoWindowLook(0.4); return ship.mode != ShipState::VIMANA; });
}

// on to another star: a neighbour the taste likes (a long jump at times), the archaeologist after a people's transmission when no people's
// world is near; the nose turned onto it, the target, the Vimana
void Game::autoPlanMove() {
    AutoState& A = ax;
    const AutoTaste& T = TASTES[A.focus];
    Star dest;
    const bool archaeologist = A.focus == AP_ARCHAEOLOGIST;
    if (!A.rng.chance(T.longJump) && autoPickStar(dest)) {
        autoPush("turning to a star", 9, [this, dest](double) {
            const Vec3 d = dest.pos - ship.pos;
            autoAimShip(d); autoWindowLook(0.35);
            return ax.stepT > 3 && autoShipOff(d) < 3 * DEG;
        });
        autoPush("the remote target", 12, [this, dest](double) {
            if (ax.stepFirst) setRemoteStar(dest);
            const Vec3 d = dest.pos - ship.pos;
            autoAimShip(d); autoEyesFront(0.8);   // the eyes to the front and the nose on the star: the flight faces the front window and the star at once
            return ax.stepT > 1.5 && autoShipOff(d) < 1 * DEG && (autoLookFrame() != LF_CABIN || (std::fabs(cabin.yaw) < 0.025 && std::fabs(cabin.pitch) < 0.025));
        });
        autoPush("the Vimana", 2, [this](double) { if (ax.stepFirst && ship.mode != ShipState::VIMANA) autoTap(KEY_V); return ship.mode == ShipState::VIMANA || ax.stepT > 1; });
        return;
    }
    if (archaeologist && !A.radarTried && sys.valid) { autoPlanRadar(); return; }
    // a long jump: toward the core, along the arm, or anywhere in the plane; now and then out of it
    const Vec3 here = ship.pos / SECTOR_KM;
    Vec3 core = Vec3(GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ) - here; core.y = 0;
    const Vec3 toCore = length(core) > 1 ? normalize(core) : Vec3(0, 0, 1), along = normalize(cross(Vec3(0, 1, 0), toCore));
    const double r = A.rng.uni();
    Vec3 dir = r < 0.3 ? toCore : (r < 0.6 ? along * (A.rng.chance(0.5) ? 1.0 : -1.0) : Vec3(std::cos(A.rng.range(0, TAU)), 0, std::sin(A.rng.range(0, TAU))));
    dir = normalize(dir + Vec3(A.rng.sym(0.35), A.rng.chance(0.08) ? A.rng.sym(0.8) : A.rng.sym(0.06), A.rng.sym(0.35)));
    const double ly = std::exp(A.rng.range(std::log(60.0), std::log(A.focus == AP_STARGAZER ? 3000.0 : 600.0)));
    autoPlanJump(ly, dir);
}

// a long jump: the nose turned along the line, the jump plotted (the star nearest its end the target, as the galaxy chart's first Enter
// does), the nose onto that star and the eyes to the front (the flight faces the front window at once), then flown
void Game::autoPlanJump(double ly, const Vec3& dir) {
    autoPush("turning to the jump", 10, [this, dir](double) { autoAimShip(dir); autoWindowLook(0.35); return ax.stepT > 3 && autoShipOff(dir) < 4 * DEG; });
    autoPush("the jump plotted", 2, [this, ly, dir](double) { if (ax.stepFirst && !plotJump(ly, dir, false)) ax.abort = true; return true; });
    autoPush("onto the star", 12, [this](double) {
        if (!ship.hasRemote) return true;
        const Vec3 d = ship.remote.pos - ship.pos;
        autoAimShip(d); autoEyesFront(0.8);
        return ax.stepT > 1.5 && autoShipOff(d) < 1 * DEG && (autoLookFrame() != LF_CABIN || (std::fabs(cabin.yaw) < 0.025 && std::fabs(cabin.pitch) < 0.025));
    });
    autoPush("the jump", 2, [this](double) { if (ax.stepFirst && ship.hasRemote) flyJump(); return ship.mode == ShipState::VIMANA || ax.stepT > 1; });
}

// the radar after a people's transmission: on, the beam swept past the signal and back (the meter rising and falling) until it locks; a
// while listening to what was sent; Enter flies to it. None in reach: the radar off and a long jump
void Game::autoPlanRadar() {
    AutoState& A = ax;
    A.radarTried = true;
    autoPush("the radar on", 2.5, [this](double) { if (ax.stepFirst && !radar.on) autoTap(KEY_B); return ax.stepT > 1.8; });
    const double way = A.rng.chance(0.5) ? 1.0 : -1.0;
    autoPush("the sweep", 30, [this, way](double) {
        if (!radar.on) return true;
        int k = -1; double best = 1e18;
        for (int i = 0; i < (int)radar.all.size(); i++) {
            const Signal& s = radar.all[i];
            if (s.kind != SIG_PEOPLE || s.local || guide.visited.count(Guide::starKey(s.star.sx, s.star.sy, s.star.sz))) continue;
            if (s.distLy < best) { best = s.distLy; k = i; }
        }
        if (k < 0) {   // nothing of a people on the air within reach
            if (ax.stepT > 5) { autoTap(KEY_B); ax.abort = true; return true; }
            autoLookSpace(forwardOf(ship.yaw + 0.4 * way, ship.pitch), 0.35);
            return false;
        }
        const Vec3 d = normalize(radar.all[k].pos - ship.pos);
        const double u = ax.stepT, over = u < 7 ? 0.22 * way * std::cos(u * 0.9) * (1 - u / 7) : 0.0;
        const Mat3 R = Mat3::axisAngle(Vec3(0, 1, 0), over);
        autoLookSpace(R * d, 0.45);
        return radar.locked >= 0 && radar.locked < (int)radar.all.size() && radar.all[radar.locked].kind == SIG_PEOPLE;
    });
    autoPush("listening", A.rng.range(10, 18), [this](double) {
        if (!radar.on || radar.locked < 0 || radar.locked >= (int)radar.all.size()) return true;
        autoLookSpace(radar.all[radar.locked].pos - ship.pos, 0.3); return false;
    });
    autoPush("to the signal", 3, [this](double) { if (ax.stepFirst && radar.on && radar.locked >= 0) autoTap(KEY_ENTER); return ax.stepT > 1; });
}

// a neighbour star (within 12 ly) the taste likes: the stargazer's by its class, the others' by what its system holds (looked up as the
// analyzer would on arrival: a world the taste wants), the stars not visited yet far before the others
bool Game::autoPickStar(Star& out) {
    AutoState& A = ax;
    std::vector<std::pair<double, const Star*>> near;
    for (const Star& s : nb.stars) {
        if (sys.valid && s.seed == sys.star.seed) continue;
        const double ly = length(s.pos - ship.pos) / SECTOR_KM;
        if (ly < 0.5 || ly > 12) continue;
        near.push_back({ly, &s});
    }
    std::sort(near.begin(), near.end(), [](const std::pair<double, const Star*>& a, const std::pair<double, const Star*>& b) { return a.first < b.first; });
    if (near.size() > 32) near.resize(32);
    std::vector<Star> full; std::vector<double> w;
    for (const auto& c : near) {
        Star f; if (!starInSector(c.second->sx, c.second->sy, c.second->sz, f, true)) continue;
        double x = 0.3 + std::exp(-std::pow((c.first - 5.0) / 3.5, 2));
        if (guide.visited.count(starKeyOf(f))) x *= 0.12;
        if (A.focus == AP_STARGAZER || A.focus == AP_WANDERER) x *= classDraw(f.cls);
        else {
            StarSystem ss; ss.generate(f);
            int giants = 0, land = 0, felis = 0, bodies = 0, rings = 0; bool civ = false, companion = false;
            for (const Body& b : ss.bodies) {
                bodies++; rings += b.rings ? 1 : 0;
                if (b.type == PT_COMPANION) companion = true;
                if (isProbeGiant(b.type)) giants++;
                if (KINDS[b.type].landable && b.type != PT_COMET) { land++; if (kindHas(b.type, KT_FLORA)) felis++; }
                if (civWorld(b.type) && worldHadCivilisation(BodyGen::make(b))) civ = true;
            }
            if (A.focus == AP_GIANTS) x *= giants ? 1 + 2.0 * giants : 0.04;
            else if (A.focus == AP_ARCHAEOLOGIST) x *= civ ? 40 : 0.1;
            else if (A.focus == AP_SURVEYOR) x *= land ? 1 + felis * 3 + land * 0.5 : 0.08;
            else if (A.focus == AP_PLANETFALL) x *= 0.3 + 0.15 * std::min(10, bodies) + 0.5 * rings;
            if (companion) x *= 1.4;
            if (A.focus == AP_ARCHAEOLOGIST && !civ) continue;   // the archaeologist goes nowhere but to a people's world (else the radar, a jump)
        }
        full.push_back(f); w.push_back(x);
    }
    if (full.empty()) return false;
    out = full[A.rng.pick(w.data(), (int)w.size())];
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the landing

// where the capsule goes down (what kind of place decided once a landing): beside one of its people's places in the light; on a giant's moon
// where the giant stands high; else a lit dry place, the sun well up (`low`: low, for the long shadows); false until the world's map is made
bool Game::autoChooseSite(int b, bool byPlace, bool underGiant, bool low) {
    AutoState& A = ax;
    const Body& B = sys.bodies[b];
    const PlanetMap* m = spaceR.mapIfReady(B);
    if (!m) return false;
    const Mat3 frame = sys.bodyFrame(b, t);
    const Vec3 sunB = frame * normalize(sys.star.pos - sys.bodyPos(b, t));
    const double R = B.radiusKm * 1000.0;
    auto dry = [&](double la, double lo, double step) {
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) if (m->materialAt(lo + i * step, la + j * step) == MAT_WATER) return false;
        return true;
    };
    auto set = [&](double la, double lo) { A.siteLat = clampd(la, -84 * DEG, 84 * DEG); A.siteLon = wrapAngle(lo); A.siteSet = true; return true; };
    auto offset = [&](const Vec3& u, double az, double metres) {   // a point `metres` from u along the azimuth
        Vec3 e = cross(Vec3(0, 0, 1), u); if (length(e) < 1e-6) e = Vec3(1, 0, 0);
        e = normalize(e); const Vec3 n = cross(u, e);
        return normalize(u + (e * std::sin(az) + n * std::cos(az)) * (metres / R));
    };
    const BodyGen& g = spaceR.genFor(B);
    if (byPlace && worldHadCivilisation(g)) {
        std::shared_ptr<const WorldPlan> P = worldPlanIfReady(g);
        if (!P && ax.stepT < 22) { worldPlanPrefetch(g); return false; }   // on its way since the approach: waited for a while
        if (P) {
            std::vector<const PlanPlace*> cand; std::vector<double> w;
            for (int k = 0; k < P->peoples && k < 2; k++) for (const PlanPlace& pl : P->plans[k].places) {
                const double lit = dot(pl.unit, sunB);
                if (lit < 0.12 || A.placesSeen.count(pl.seed)) continue;
                cand.push_back(&pl); w.push_back((pl.sclass == SC_CAPITAL ? 12.0 : (pl.sclass == SC_CITY ? 3.0 : 1.0)) * (0.4 + lit));   // the records gather at the capital (T-04's homes)
            }
            if (!cand.empty()) {
                // a capital not visited yet seven times in ten (its archive and a third or more of its people's records), else a city half the
                // time, else any by the weights
                int pick = -1;
                for (int pass = 0; pass < 2 && pick < 0; pass++) {
                    const int want = pass == 0 ? SC_CAPITAL : SC_CITY;
                    if (!A.rng.chance(pass == 0 ? 0.7 : 0.5)) continue;
                    std::vector<int> of;
                    for (size_t i = 0; i < cand.size(); i++) if (cand[i]->sclass == want) of.push_back((int)i);
                    if (!of.empty()) pick = of[A.rng.irange((int)of.size())];
                }
                if (pick < 0) pick = A.rng.pick(w.data(), (int)w.size());
                const PlanPlace& pl = *cand[pick];
                A.placesSeen.insert(pl.seed);
                for (int k = 0; k < 24; k++) {   // outside its core, on dry ground (the landing keeps the capsule outside the walls)
                    const Vec3 u = offset(pl.unit, A.rng.range(0, TAU), pl.radiusM + A.rng.range(110, 240));
                    double la, lo; StarSystem::latLonFromBody(u, la, lo);
                    if (m->materialAt(lo, la) != MAT_WATER) return set(la, lo);
                }
                double la, lo; StarSystem::latLonFromBody(pl.unit, la, lo);
                return set(la, lo);
            }
        }
    }
    if (underGiant) {
        const Vec3 up = frame * normalize(sys.bodyPos(B.parent, t) - sys.bodyPos(b, t));   // where the giant stands at the zenith
        for (int k = 0; k < 120; k++) {
            const Vec3 u = offset(up, A.rng.range(0, TAU), R * A.rng.range(20, 50) * DEG);
            if (dot(u, sunB) < 0.15) continue;
            double la, lo; StarSystem::latLonFromBody(u, la, lo);
            if (dry(la, lo, 1.2 * DEG)) return set(la, lo);
        }
    }
    const double lo0 = low ? 0.06 : 0.25, hi0 = low ? 0.25 : 0.85;
    for (int pass = 0; pass < 3; pass++)
        for (int k = 0; k < 300; k++) {
            const double la = A.rng.range(-65 * DEG, 65 * DEG), lo = A.rng.range(-PI, PI);
            const double s = dot(StarSystem::bodyFromLatLon(la, lo), sunB);
            if (pass < 2 && (s < lo0 || s > hi0)) continue;
            if (pass == 2 && s < 0.05) continue;
            if (!dry(la, lo, 1.5 * DEG)) continue;
            if (pass == 0 && A.focus == AP_SURVEYOR && kindHas(B.type, KT_FLORA)) { const int mat = m->materialAt(lo, la); if (mat != MAT_GRASS && mat != MAT_FOREST) continue; }
            return set(la, lo);
        }
    return false;   // no dry ground in the light (a sea world): not down this time
}

// the landing map: the chosen site under the cursor and Enter at once (the screen is never shown); none chosen, back to the cabin
void Game::autoLandingMap() {
    AutoState& A = ax;
    if (!A.siteSet) { if (A.sceneT > 0.5) { autoTap(KEY_ESCAPE); A.sceneT = 0; } return; }
    landLat = A.siteLat; landLon = A.siteLon; landZoom = 0;
    autoTap(KEY_ENTER);
    A.siteSet = false; A.landings++;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the ground

void Game::autoPlanGround() {
    AutoState& A = ax;
    const AutoTaste& T = TASTES[A.focus];
    if (!A.groundBegun) {
        A.groundBegun = true;
        A.groundUntil = A.clock + A.rng.range(T.ground0, T.ground1);
        A.outingsLeft = 2 + A.rng.irange(4);
        surf.scanWanted = true;   // the sights round the landing heard on their thread (the scanner's search; its readout stays hidden)
        if (surf.inVehicle()) { autoPlanVehicleOut(surf.inBuggy ? TR_BUGGY : TR_DRONE); return; }
        autoPush("the first look round", A.rng.range(10, 16), [this](double) { autoGazeRound(0.5); return false; }, true);
        return;
    }
    if (surf.inVehicle()) { autoPlanVehicleOut(surf.inBuggy ? TR_BUGGY : TR_DRONE); return; }
    const bool dark = surf.env.sun.altitude < -0.1 && A.clock > A.groundUntil - (T.ground1 - T.ground0) * 0.5;   // night has come: home sooner
    if (A.clock > A.groundUntil || A.outingsLeft <= 0 || dark) { autoPlanLeave(); return; }
    A.outingsLeft--;
    double w[AO_COUNT];
    for (int k = 0; k < AO_COUNT; k++) w[k] = T.w[k];
    if (!worldHadCivilisation(surf.site.gen)) w[AO_RUINS] = 0;
    if (surf.critters.empty() && surf.flocks.empty()) w[AO_LIFE] = 0;
    if (surf.site.escapeVelocity < 30) w[AO_DRIVE] = w[AO_FLY] = 0;
    if (!surf.scanned || surf.echoes.empty()) w[AO_SIGHT] *= 0.25;
    if (!surf.site.hasWater) w[AO_WATER] *= 0.4;
    if (A.lastOuting >= 0) w[A.lastOuting] *= 0.15;   // not the same twice running, as a rule
    for (int tries = 0; tries < 4; tries++) {
        const int k = A.rng.pick(w, AO_COUNT);
        if (autoPlanOuting(k)) { A.outings++; A.lastOuting = k; return; }
        w[k] = 0;
    }
    if (!autoOutingVista()) autoPush("a look round", A.rng.range(8, 14), [this](double) { autoGazeRound(0.5); return false; }, true);
}

bool Game::autoPlanOuting(int kind) {
    switch (kind) {
        case AO_VISTA: return autoOutingVista();
        case AO_WATER: return autoOutingWater();
        case AO_LIFE: return autoOutingLife();
        case AO_SIGHT: return autoOutingSight();
        case AO_DRIVE: return autoOutingDrive();
        case AO_FLY: return autoOutingFly();
        case AO_RUINS: return autoOutingRuins();
        case AO_SKY: return autoOutingSky();
        default: return false;
    }
}

bool Game::autoDryAt(double x, double z) { const double g = surf.site.groundHeight(x, z); return surf.site.waterNowAt(x, z) < g - 0.05; }   // Q-02: the tide's

// out of a vehicle (whichever it is in when the step comes): the buggy braked to a stop; the drone set down on dry, gentle ground near where
// it is (on its skids); then E
void Game::autoPlanVehicleOut(int which) {
    if (which != TR_DRONE) autoPush("the buggy stopping", 12, [this](double) {
        if (!surf.inBuggy) return true;
        const Buggy& b = surf.buggy;
        ax.wantFwd = b.speed > 1 ? -0.5 : 0; if (std::fabs(b.speed) < 4) autoHold(KEY_SPACE);
        autoLook(0, -0.05, 0.5);
        return std::fabs(b.speed) < 0.4 && ax.stepT > 0.5;
    });
    if (which != TR_BUGGY) autoPush("the drone setting down", 90, [this](double dt) {
        if (!surf.inDrone) return true;
        AutoState& S = ax;
        const Drone& d = surf.drone;
        if (d.landed) return true;
        if (S.stepFirst) {   // a dry, gentle spot within a hundred metres (where it hovers, if that will do)
            S.goX = d.x; S.goZ = d.z;
            for (int k = 0; k < 48; k++) {
                const double a = S.rng.range(0, TAU), r = k == 0 ? 0.0 : S.rng.range(5, 100), px = d.x + std::sin(a) * r, pz = d.z + std::cos(a) * r;
                const double g = surf.site.groundHeight(px, pz), slope = std::fabs(surf.site.groundHeight(px + 4, pz) - g) + std::fabs(surf.site.groundHeight(px, pz + 4) - g);
                if (autoDryAt(px, pz) && slope < 1.2) { S.goX = px; S.goZ = pz; break; }
            }
        }
        if (std::hypot(S.goX - d.x, S.goZ - d.z) > 12) { autoFly(S.goX, S.goZ, 10, 30, 18, dt); return false; }
        S.wantFwd = clampd(-d.speed * 0.3, -1, 1);
        if (std::fabs(d.speed) < 4) autoHold(KEY_LEFT_SHIFT);
        autoLook(0, -0.5, 0.5);
        return false;
    });
    autoPush("out of the vehicle", 4, [this](double) {   // the camera to the front first: the explorer steps out facing the hull's heading
        if (!surf.inVehicle()) return true;
        autoLook(0, 0, 0.8);
        const double look = wrapAngle(surf.player.yaw - (surf.inBuggy ? surf.buggy.heading : surf.drone.heading));
        if ((std::fabs(look) < 0.05 && std::fabs(surf.player.pitch) < 0.08) || ax.stepT > 2.5) { if (autoEvery(1.0)) autoTap(KEY_E); }
        return false;
    });
}

// home: out of a vehicle, the capsule called when it is far (K), the walk to it, a last look, aboard (Q)
void Game::autoPlanLeave() {
    if (surf.inVehicle()) autoPlanVehicleOut(surf.inBuggy ? TR_BUGGY : TR_DRONE);
    autoPush("calling the capsule", 3, [this](double) {
        if (surf.env.capsuleDist < 60) return true;
        if (ax.stepFirst) autoTap(KEY_K);
        autoGazeRound(0.5);
        return ax.stepT > 2;
    });
    autoPush("to the capsule", 150, [this](double dt) { return surf.env.capsuleDist < 3.5 || autoWalkTo(surf.capsuleX, surf.capsuleZ, 3.0, 0.8, dt) != 0; });
    autoPush("a last look", ax.rng.range(3, 6), [this](double) { autoGazeRound(0.45); return false; }, true);
    autoPush("aboard the capsule", 5, [this](double) {
        if (surf.env.nearCapsule && autoEvery(1.0)) autoTap(KEY_Q);
        else if (!surf.env.nearCapsule && ax.stepFirst) autoTap(KEY_K);
        return state != GameState::SURFACE;
    });
}

// a view: the highest ground within a few hundred metres, walked to, a long look round from it
bool Game::autoOutingVista() {
    double x, z;
    if (!autoFindRise(x, z)) return false;
    autoPush("up a rise", 25 + std::hypot(x - surf.player.x, z - surf.player.z) / 1.5, [this, x, z](double dt) { return autoWalkTo(x, z, 2.5, 1.0, dt) != 0; });
    autoPush("the view from it", ax.rng.range(14, 24), [this](double) { autoGazeRound(0.5); return false; }, true);
    return true;
}

bool Game::autoFindRise(double& bx, double& bz) {
    const Player& p = surf.player;
    const double h0 = surf.site.groundHeight(p.x, p.z);
    double best = 4; bool found = false;
    auto walkable = [&](double x, double z) {   // the straight way there: dry, and no stretch of 10 m steeper than a grade of 0.45
        const double L = std::hypot(x - p.x, z - p.z);
        double prev = h0;
        for (double s = 10; s <= L; s += 10) {
            const double u = s / L, px = p.x + (x - p.x) * u, pz = p.z + (z - p.z) * u, g = surf.site.groundHeight(px, pz);
            if (std::fabs(g - prev) > 4.5 || !autoDryAt(px, pz)) return false;
            prev = g;
        }
        return true;
    };
    for (int ring = 1; ring <= 4; ring++) {
        const double R = 60.0 * ring + ax.rng.range(-10, 10);
        for (int k = 0; k < 16; k++) {
            const double a = (k + ax.rng.uni()) * TAU / 16, x = p.x + std::sin(a) * R, z = p.z + std::cos(a) * R;
            if (!autoDryAt(x, z)) continue;
            const double score = surf.site.groundHeight(x, z) - h0 - 0.015 * R;
            if (score > best && walkable(x, z)) { best = score; bx = x; bz = z; found = true; }
        }
    }
    for (int k = 0; k < 24 && !found; k++) {   // no rise: a stroll, somewhere dry and walkable
        const double a = ax.rng.range(0, TAU), R = ax.rng.range(70, 160), x = p.x + std::sin(a) * R, z = p.z + std::cos(a) * R;
        if (autoDryAt(x, z) && walkable(x, z)) { bx = x; bz = z; found = true; }
    }
    return found;
}

// the water: the nearest within 450 m, walked to the dry ground a few metres short of its edge, a long look over it
bool Game::autoOutingWater() {
    const Player& p = surf.player;
    for (double R = 20; R <= 450; R += 20)
        for (int k = 0; k < 24; k++) {
            const double a = (k + 0.5 * ((int)(R / 20) & 1)) * TAU / 24, x = p.x + std::sin(a) * R, z = p.z + std::cos(a) * R;
            const double w = surf.site.waterNowAt(x, z);
            if (w < surf.site.groundHeight(x, z) + 0.3) continue;
            double sx = x, sz = z;
            for (double s = 2; s < R; s += 2) { sx = x - std::sin(a) * s; sz = z - std::cos(a) * s; if (autoDryAt(sx, sz)) break; }
            sx -= std::sin(a) * 3; sz -= std::cos(a) * 3;
            autoPush("down to the water", 25 + R / 1.5, [this, sx, sz](double dt) { return autoWalkTo(sx, sz, 1.5, 0.9, dt) != 0; });
            autoPush("the water", ax.rng.range(12, 20), [this, x, z, w](double) {
                AutoState& S = ax;
                if (S.clock > S.driftNext) { S.driftYaw = S.rng.sym(0.5); S.driftPitch = S.rng.range(-0.05, 0.08); S.driftNext = S.clock + S.rng.range(4, 7); }
                const Player& q = surf.player;
                const double h = std::atan2(x - q.x, z - q.z);
                autoLook(h + S.driftYaw, std::atan2(w - (q.y + q.eyeHeight), std::max(4.0, std::hypot(x - q.x, z - q.z))) * 0.6 + S.driftPitch, 0.45);
                return false;
            }, true);
            return true;
        }
    return false;
}

const Critter* Game::autoCritter(uint64_t seed) const { for (const Critter& c : surf.critters) if (c.seed == seed) return &c; return nullptr; }

// the life: the nearest creature within 240 m approached to 18-30 m (closer they flee) and watched; else a flock overhead
bool Game::autoOutingLife() {
    const Player& p = surf.player;
    const Critter* best = nullptr; double bd = 240;
    for (const Critter& c : surf.critters) { const double d = std::hypot(c.x - p.x, c.z - p.z); if (d < bd) { bd = d; best = &c; } }
    if (best) {
        const uint64_t seed = best->seed; const double keep = ax.rng.range(18, 30);
        autoPush("toward the animals", 150, [this, seed, keep](double dt) {
            const Critter* c = autoCritter(seed); if (!c) return true;
            if (std::hypot(c->x - surf.player.x, c->z - surf.player.z) < keep) return true;
            return autoWalkTo(c->x, c->z, keep, 0.65, dt, 4.0) != 0;
        });
        autoPush("watching them", ax.rng.range(16, 28), [this, seed](double) {
            const Critter* c = autoCritter(seed); if (!c) return true;
            autoLookAt(c->x, surf.site.groundHeight(c->x, c->z) + 0.7 * c->size, c->z, 0.5);
            return false;
        }, true);
        return true;
    }
    const Flock* fl = nullptr; bd = 600;
    for (const Flock& f : surf.flocks) { const double d = std::hypot(f.cx - p.x, f.cz - p.z); if (d < bd) { bd = d; fl = &f; } }
    if (!fl) return false;
    const size_t i = fl - surf.flocks.data();
    autoPush("the birds", ax.rng.range(12, 18), [this, i](double) {
        if (i >= surf.flocks.size()) return true;
        const Flock& f = surf.flocks[i];
        autoLookAt(f.cx, surf.site.groundHeight(f.cx, f.cz) + (f.landed > 0.5 ? 1.0 : f.alt), f.cz, 0.45);
        return false;
    }, true);
    return true;
}

// a sight the scanner heard (a peak, a canyon's rim, a crater's, the geysers, the crystals, a lake, a work): on foot when near, by buggy or
// drone farther; a long look at it. A ruin: the settlement's tour
bool Game::autoOutingSight() {
    if (!surf.scanned || surf.echoes.empty()) return false;
    const Player& p = surf.player;
    std::vector<const SurfaceView::ScanEcho*> cand; std::vector<double> w;
    const bool wheels = surf.site.escapeVelocity >= 30;
    for (const SurfaceView::ScanEcho& e : surf.echoes) {
        const double d = std::max(0.0, std::hypot(e.x - p.x, e.z - p.z) - e.lm.radiusM * 0.8);
        if (d > (wheels ? 12000.0 : 900.0)) continue;
        double x = 1;
        switch (e.lm.kind) {
            case LM_GEYSERS: case LM_CRYSTALS: x = 3; break;
            case LM_CANYON: case LM_CRATER: x = 2; break;
            case LM_CALDERA: case LM_LABYRINTH: case LM_CHAIN: case LM_MUD: case LM_PINGO: case LM_MAAR: case LM_PENITENTES: case LM_METEORITES: x = 2; break;   // Q-01; Q-02
            case LM_RUIN: case LM_WORK: x = ax.focus == AP_ARCHAEOLOGIST ? 6 : 0.8; break;
            default: x = 1.2; break;
        }
        if (ax.focus == AP_ARCHAEOLOGIST && e.lm.kind != LM_RUIN && e.lm.kind != LM_WORK) x *= 0.25;
        cand.push_back(&e); w.push_back(x / (1 + d / 2500));
    }
    if (cand.empty()) return false;
    const SurfaceView::ScanEcho e = *cand[ax.rng.pick(w.data(), (int)w.size())];
    const double dc = std::hypot(e.x - p.x, e.z - p.z);
    if (e.lm.kind == LM_RUIN || e.lm.kind == LM_WORK) {
        const double size = std::max(30.0, e.lm.radiusM);
        const double edge = dc - size;
        if (edge > 650 && wheels) { const double k = (size + 40) / std::max(dc, 1.0); autoPlanTravel(e.x + (p.x - e.x) * k, e.z + (p.z - e.z) * k, 25, TR_DRONE, size + 120); }
        autoPlanTown(e.x, e.z, size);
        return true;
    }
    // where to stand: a crater's or a canyon's rim, a lake's shore, near a peak's summit, among the geysers or the crystals (Q-01: a caldera's, a
    // chain crater's or a maar's rim, near a mud volcano's or a pingo's top)
    const bool rim = e.lm.kind == LM_CRATER || e.lm.kind == LM_CANYON || e.lm.kind == LM_LAKE || e.lm.kind == LM_MESA || e.lm.kind == LM_CALDERA || e.lm.kind == LM_CHAIN || e.lm.kind == LM_MAAR;
    const double frac = rim ? 0.95 : (e.lm.kind == LM_PEAK || e.lm.kind == LM_MUD || e.lm.kind == LM_PINGO ? 0.45 : 0.3);
    const double k = std::min(1.0, e.lm.radiusM * frac / std::max(dc, 1.0));
    const double sx = e.x + (p.x - e.x) * k, sz = e.z + (p.z - e.z) * k;
    const double far = std::hypot(sx - p.x, sz - p.z);
    const int how = far < 500 || !wheels ? TR_FOOT : (far < 2500 && ax.rng.chance(0.6) ? TR_BUGGY : TR_DRONE);
    autoPlanTravel(sx, sz, how == TR_FOOT ? 3.0 : 20.0, how, how == TR_DRONE ? std::max(150.0, e.lm.radiusM * 0.8) : 0.0);
    const double ex = e.x, ez = e.z;
    autoPush("the sight", ax.rng.range(14, 22), [this, ex, ez](double) {
        AutoState& S = ax;
        if (S.clock > S.driftNext) { S.driftYaw = S.rng.sym(0.45); S.driftPitch = S.rng.range(-0.06, 0.1); S.driftNext = S.clock + S.rng.range(4, 7); }
        const Player& q = surf.player;
        const double gy = surf.site.groundHeight(ex, ez), d = std::max(10.0, std::hypot(ex - q.x, ez - q.z));
        autoLook(std::atan2(ex - q.x, ez - q.z) + S.driftYaw, clampd(std::atan2(gy - (q.y + q.eyeHeight), d), -0.5, 0.5) + S.driftPitch, 0.45);
        return false;
    }, true);
    return true;
}

// a drive for its own sake: the buggy out at the capsule and down the longest dry, gentle run (or to a sight a kilometre or more off)
bool Game::autoOutingDrive() {
    if (surf.site.escapeVelocity < 30) return false;
    const Player& p = surf.player;
    double bestRun = 0, bx = 0, bz = 0;
    for (int k = 0; k < 16; k++) {
        const double h = (k + ax.rng.uni()) * TAU / 16;
        double run = 0, prev = surf.site.groundHeight(p.x, p.z);
        for (double s = 30; s <= 2600; s += 30) {
            const double x = p.x + std::sin(h) * s, z = p.z + std::cos(h) * s, g = surf.site.groundHeight(x, z);
            if (surf.site.waterNowAt(x, z) > g + 0.1 || std::fabs(g - prev) > 30 * 0.3) break;
            prev = g; run = s;
        }
        if (run > bestRun) { bestRun = run; bx = p.x + std::sin(h) * run; bz = p.z + std::cos(h) * run; }
    }
    if (bestRun < 400) return false;
    autoPlanTravel(bx, bz, 25, TR_BUGGY);
    autoPush("a look round from the buggy", ax.rng.range(8, 14), [this](double) { autoGazeRound(0.45); return false; }, true);
    return true;
}

// a flight for its own sake: the drone up and out two to six kilometres (to a sight when one is there), a ring round it, down and out
bool Game::autoOutingFly() {
    if (surf.site.escapeVelocity < 30) return false;
    const Player& p = surf.player;
    double x = 0, z = 0, ring = 0; bool have = false;
    if (surf.scanned)
        for (const SurfaceView::ScanEcho& e : surf.echoes) {
            const double d = std::hypot(e.x - p.x, e.z - p.z);
            const bool theirs = e.lm.kind == LM_RUIN || e.lm.kind == LM_WORK;
            if (d < 1500 || d > 9000 || (ax.focus == AP_ARCHAEOLOGIST && !theirs) || !ax.rng.chance(theirs && ax.focus == AP_ARCHAEOLOGIST ? 0.8 : 0.35)) continue;
            x = e.x; z = e.z; ring = std::max(150.0, e.lm.radiusM * 0.8); have = true; break;
        }
    for (int k = 0; k < 12 && !have; k++) {
        const double h = ax.rng.range(0, TAU), d = ax.rng.range(2000, 6000);
        x = p.x + std::sin(h) * d; z = p.z + std::cos(h) * d; ring = ax.rng.range(180, 320);
        have = autoDryAt(x, z);
    }
    if (!have) return false;
    autoPlanTravel(x, z, 30, TR_DRONE, ring);
    autoPush("a look round", ax.rng.range(8, 14), [this](double) { autoGazeRound(0.45); return false; }, true);
    return true;
}

// the ruins: of the settlements in reach, the one with the most records not taken for its distance (a capital's archive a few kilometres
// off before a hamlet's one record next door; with none left anywhere, the nearest), flown to when far; its tour
bool Game::autoOutingRuins() {
    const Player& p = surf.player;
    double cx = 0, cz = 0, size = 0, best = -1;
    surf.forNearbyRuins(p.x, p.z, 2, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        if (cell.spec.buildings.empty()) return;
        int left = 0; for (const ShardSite& s : cell.shards) if (!surf.shardsFound.count(s.index) && !ax.recordsTried.count(s.index)) left++;
        const double d = std::hypot(r.x - p.x, r.z - p.z), score = left ? 1.0 + left / (1.0 + d / 1500.0) : 1.0 / (1.0 + d / 300.0);
        if (score > best) { best = score; cx = r.x; cz = r.z; size = std::max(20.0, cell.spec.size); }
    });
    if (best < 0) return false;
    // the nearest record of it (else its edge): more than 400 m off, the drone there (a ring over it, set down near it), on foot from there
    double rx = 0, rz = 0;
    const bool rec = autoNearestRecord(cx, cz, size + 30, rx, rz) >= 0;
    const double dc = std::hypot(cx - p.x, cz - p.z);
    if (!rec) { const double k = std::min(1.0, (size + 40) / std::max(dc, 1.0)); rx = cx + (p.x - cx) * k; rz = cz + (p.z - cz) * k; }
    if (std::hypot(rx - p.x, rz - p.z) > 400 && surf.site.escapeVelocity >= 30) autoPlanTravel(rx, rz, 25, TR_DRONE, std::min(250.0, std::max(150.0, size * 0.5)));
    autoPlanTown(cx, cz, size);
    return true;
}

// a settlement's tour: in to its middle, then the records not taken there one after another (walked to, looked at a moment, taken), a few of
// them; a look round among the ruins
void Game::autoPlanTown(double cx, double cz, double size) {
    {   // what is there to find (the harness's line)
        int n = 0, cls = -1;
        surf.forNearbyRuins(cx, cz, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
            for (const ShardSite& st : cell.shards) if (!surf.shardsFound.count(st.index) && std::hypot(r.x + st.x - cx, r.z + st.z - cz) <= size + 30) n++;
            if (std::hypot(r.x - cx, r.z - cz) < 1) cls = cell.spec.sclass;
        });
        ax.townLine = fmt("a settlement (class %d) %.0f m across, %.0f m off, %d records not taken", cls, 2 * size, std::hypot(cx - surf.player.x, cz - surf.player.z), n);
        ax.towns++;
    }
    // with records left: straight to the nearest of them (a capital's edge is a kilometre from its middle); without: in to its middle for a look
    double rx = 0, rz = 0;
    const bool records = autoNearestRecord(cx, cz, size + 30, rx, rz) >= 0;
    if (!records) {
        const double d0 = std::hypot(cx - surf.player.x, cz - surf.player.z);
        autoPush("into the settlement", 30 + std::min(d0, 400.0) / 1.2, [this, cx, cz, size](double dt) {
            const double d = std::hypot(cx - surf.player.x, cz - surf.player.z);
            return d < std::min(14.0, size * 0.4) || autoWalkTo(cx, cz, std::min(12.0, size * 0.35), 0.85, dt) != 0;
        });
        autoPush("a look round the settlement", ax.rng.range(8, 14), [this](double) { autoGazeRound(0.55); return false; }, true);
        return;
    }
    const int want = 3 + ax.rng.irange(6);
    autoPush("the records", 300 + std::hypot(rx - surf.player.x, rz - surf.player.z) / 1.2, [this, cx, cz, size, want](double dt) {
        AutoState& A = ax;
        if (A.stepFirst) { A.recIdx = -1; A.recTaken = 0; A.recPhase = 0; }
        if (A.recTaken >= want) return true;
        double x = 0, z = 0;
        if (A.recPhase != 2 && (A.recIdx < 0 || !autoRecordAt(A.recIdx, x, z))) {
            A.recIdx = autoNearestRecord(cx, cz, size + 30, x, z);
            if (A.recIdx < 0) return true;
            if (std::hypot(x - surf.player.x, z - surf.player.z) > 400 && surf.site.escapeVelocity >= 30) { A.recIdx = -1; return true; }   // far across the place: the next outing flies there
            A.recordsTried.insert(A.recIdx); A.recPhase = 0; A.recT = 0; A.goX = A.goZ = 1e18; A.unstick = 0;
        }
        if (A.recPhase == 2) {   // the first one taken: a look round the ruins before the next
            A.recT += dt; autoGazeRound(0.55);
            if (A.recT > 7) { A.recPhase = 0; A.recIdx = -1; }
            return false;
        }
        if (A.recPhase == 0) {
            const int w = autoWalkTo(x, z, 1.15, 0.75, dt);
            if (w < 0) { A.recIdx = -1; return false; }   // no way to it: the next
            if (w > 0) { A.recPhase = 1; A.recT = 0; }
            return false;
        }
        A.recT += dt;
        autoLookAt(x, surf.site.groundHeight(x, z) + 0.15, z, 0.7);
        if (A.recT > 2.2 && surf.nearShard.index >= 0) { autoTap(KEY_E); A.recTaken++; A.records++; A.recIdx = -1; if (A.recTaken == 1) { A.recPhase = 2; A.recT = 0; } }
        else if (A.recT > 6) A.recIdx = -1;   // out of reach after all (a floor above): the next
        return false;
    });
    autoPush("among the ruins", ax.rng.range(8, 14), [this](double) { autoGazeRound(0.5); return false; }, true);
}

bool Game::autoRecordAt(int index, double& x, double& z) {
    bool found = false;
    surf.forNearbyRuins(surf.player.x, surf.player.z, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        if (found) return;
        for (const ShardSite& s : cell.shards) if (s.index == index && !surf.shardsFound.count(s.index)) { x = r.x + s.x; z = r.z + s.z; found = true; return; }
    });
    return found;
}

int Game::autoNearestRecord(double cx, double cz, double radius, double& x, double& z) {
    int found = -1; double best = 1e18;
    surf.forNearbyRuins(cx, cz, 1, [&](const Ruin& r, const SurfaceView::RuinCell& cell, int) {
        for (const ShardSite& s : cell.shards) {
            if (surf.shardsFound.count(s.index) || ax.recordsTried.count(s.index)) continue;
            const double rx = r.x + s.x, rz = r.z + s.z;
            if (std::hypot(rx - cx, rz - cz) > radius) continue;
            const double d = std::hypot(rx - surf.player.x, rz - surf.player.z);
            if (d < best) { best = d; found = s.index; x = rx; z = rz; }
        }
    });
    return found;
}

// the sky: the biggest body over the horizon (a giant over its moon, a sister world), else the sky itself; sat down, a long look up, the
// clock run on a while now and then (more often at night: the stars wheel)
bool Game::autoOutingSky() {
    Vec3 dir;
    const bool have = autoBigSky(dir);
    const bool night = surf.env.sun.altitude < -0.05;
    const bool warp = settings.clockMode == 0 && ax.rng.chance(night ? 0.6 : 0.3);
    autoPush("sitting down", 2, [this](double) { if (ax.stepFirst && !surf.player.crouch) autoTap(KEY_C); return ax.stepT > 1; });
    const double dur = ax.rng.range(20, 34), up = ax.rng.range(0.35, 0.7);
    autoPush("the sky", dur, [this, have, warp, dur, up](double) {
        Vec3 d;
        if (have && autoBigSky(d)) autoLookLocal(d, 0.35);
        else { AutoState& S = ax; if (S.stepFirst) S.driftYaw = surf.player.yaw; autoLook(wrapAngle(S.driftYaw + S.stepT * 0.05), up, 0.35); }
        if (warp) ax.warpWant = ax.stepT > 3 && ax.stepT < dur - 5 ? 100 : 1;
        return false;
    }, true);
    autoPush("the clock at its pace", 4, [this](double) { ax.warpWant = 1; return timeWarp <= 1.01; });
    autoPush("standing up", 2, [this](double) { if (ax.stepFirst && surf.player.crouch) autoTap(KEY_C); return ax.stepT > 1; });
    return true;
}

bool Game::autoBigSky(Vec3& dirLocal) {
    if (!sys.valid || surf.site.body < 0 || surf.site.body >= (int)sys.bodies.size()) return false;
    const Vec3 here = sys.bodyPos(surf.site.body, t);
    const Mat3 L = surf.site.localFrame(t);
    double best = 0.5 * DEG;
    bool found = false;
    for (int j = 0; j < (int)sys.bodies.size(); j++) {
        if (j == surf.site.body || sys.bodies[j].type == PT_COMPANION) continue;
        const Vec3 d = sys.bodyPos(j, t) - here;
        const double ang = std::asin(clampd(sys.bodies[j].radiusKm / std::max(length(d), 1.0), 0, 1));
        const Vec3 l = L * normalize(d);
        if (l.y < std::sin(8 * DEG) || ang < best) continue;
        best = ang; dirLocal = l; found = true;
    }
    return found;
}

// a place of the ground reached: on foot; by the buggy (the capsule called near, the buggy unfolded beside it, in, driven, out); by the
// drone (unfolded, in, up, flown, a ring round the place when asked, set down on dry gentle ground near it, out)
void Game::autoPlanTravel(double x, double z, double r, int how, double circle) {
    AutoState& A = ax;
    if (how == TR_FOOT) { autoPush("walking", 30 + std::hypot(x - surf.player.x, z - surf.player.z) / 1.5, [this, x, z, r](double dt) { return autoWalkTo(x, z, r, 1.0, dt) != 0; }); return; }
    const bool buggy = how == TR_BUGGY;
    autoPush("to the capsule", 60, [this](double dt) {
        if (surf.env.capsuleDist < 8) return true;
        if (surf.env.capsuleDist > 45) { if (ax.stepFirst) autoTap(KEY_K); autoGazeRound(0.5); return ax.stepT > 1.5; }
        return autoWalkTo(surf.capsuleX, surf.capsuleZ, 6, 0.8, dt) != 0;
    });
    autoPush(buggy ? "the buggy unfolding" : "the drone unfolding", 10, [this, buggy](double) {
        if (ax.stepFirst) autoTap(buggy ? KEY_B : KEY_F);
        const bool out = buggy ? surf.buggy.deployed : surf.drone.deployed;
        if (out) autoLookAt(buggy ? surf.buggy.x : surf.drone.x, buggy ? surf.buggy.y + 0.5 : surf.drone.y + 0.5, buggy ? surf.buggy.z : surf.drone.z, 0.6);
        return ax.stepT > 0.5 && out && (buggy ? surf.buggy.unfold : surf.drone.unfold) >= 1;
    });
    autoPush(buggy ? "into the buggy" : "into the drone", 25, [this, buggy](double dt) {
        if (buggy ? surf.inBuggy : surf.inDrone) return true;
        if (!(buggy ? surf.buggy.deployed : surf.drone.deployed)) { ax.abort = true; return true; }   // it never unfolded: the outing dropped
        const double vx = buggy ? surf.buggy.x : surf.drone.x, vz = buggy ? surf.buggy.z : surf.drone.z;
        if ((buggy ? surf.buggyDist() : surf.droneDist()) < 2.6) {   // the eyes along its heading first: the seat faces that way
            const double h = buggy ? surf.buggy.heading : surf.drone.heading;
            autoLook(h, 0, 0.8);
            if (std::fabs(wrapAngle(surf.player.yaw - h)) < 0.02 && std::fabs(surf.player.pitch) < 0.02 && autoEvery(0.8)) autoTap(KEY_E);
            return false;
        }
        return autoWalkTo(vx, vz, 2.2, 0.7, dt) < 0;
    });
    const bool chase = A.rng.chance(0.4);
    if (chase) autoPush("the chase camera", 1, [this](double) { if ((surf.inBuggy || surf.inDrone) && !surf.chaseCam) autoTap(KEY_V); return true; });
    const double cruise = A.rng.range(14, 22);
    if (buggy) autoPush("driving", 420, [this, x, z, r, cruise](double dt) { return autoDrive(x, z, r, cruise, dt) != 0; });
    else {
        const double alt = A.rng.range(50, 130), speed = A.rng.range(45, 75);
        autoPush("lifting off", 20, [this, alt](double) { autoHold(KEY_SPACE); autoLook(0, 0.05, 0.5); return !surf.drone.landed && surf.drone.altAboveGround > alt * 0.6; });
        autoPush("flying", 420, [this, x, z, alt, speed](double dt) { return autoFly(x, z, 60, alt, speed, dt) != 0; });
        if (circle > 0) {
            const double dur = A.rng.range(20, 32);
            autoPush("a ring round it", dur, [this, x, z, circle, alt](double) { if (!surf.inDrone) return true; autoCircle(x, z, circle, std::max(40.0, alt * 0.7)); return false; }, true);
        }
        autoPush("over the place", 60, [this, x, z](double dt) { return autoFly(x, z, 25, 30, 18, dt) != 0; });   // down over it, slowly
    }
    autoPlanVehicleOut(how);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the probe's camera

// the fall at its own pace for a while, then on at eight times (Shift), the entry at its own, the descent in stretches at eight with
// breaks; the chute once in the air; the eyes round the clouds; at the end its last frame a while, then back to the cabin
void Game::autoPlanProbe() {
    ax.probeEndT = -1; ax.chuteAt = ax.rng.range(15, 60);
    if (probe.active && probe.clock < 2) ax.probes++;   // one it sent (not one it came back to)
    autoPush("the probe's camera", 1200, [this](double) {
        AutoState& A = ax;
        if (!probe.active) return true;
        if (probe.endClock >= 0) {
            if (A.probeEndT < 0) A.probeEndT = A.clock;
            if (A.clock - A.probeEndT > 6) { autoTap(KEY_E); return true; }
            return false;
        }
        const double c = probe.clock, ds = probeDescentStart();
        const bool fast = (c > 12 && c < PROBE_FALL_S - 3) || (c > ds + 25 && std::fmod(A.stepT, 45) < 30);
        if (fast) autoHold(KEY_LEFT_SHIFT);
        if (probe.flight.chuteOpen < 0 && c > ds + A.chuteAt && autoEvery(2.0)) autoTap(KEY_SPACE);
        if (A.clock > A.driftNext) {
            const double r = A.rng.uni();
            A.driftYaw = r < 0.55 ? A.rng.sym(0.35) : A.rng.sym(1.4);
            A.driftPitch = r < 0.55 ? A.rng.range(-0.25, 0.1) : (A.rng.chance(0.5) ? A.rng.range(-0.75, -0.4) : A.rng.range(0.2, 0.5));
            A.driftNext = A.clock + A.rng.range(6, 11);
        }
        autoLook(A.driftYaw, A.driftPitch, 0.4);
        return false;
    });
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the hands' vocabulary

void Game::autoTap(int key) { if (key >= 0 && key < KEY_MAX) ax.taps.push_back(key); }
void Game::autoHold(int key) { if (key >= 0 && key < KEY_MAX) ax.hold[key] = true; }
bool Game::autoEvery(double secs) { if (ax.clock < ax.everyT) return false; ax.everyT = ax.clock + secs; return true; }   // a press at most once in `secs` (whatever the frame rate)

// the sprint on a long walk while the breath allows (Shift held; with the toggle setting, a press when it should change)
void Game::autoSprint(bool want) {
    const Player& p = surf.player;
    if (!settings.sprintToggle) { if (want) autoHold(KEY_LEFT_SHIFT); return; }
    if (want != p.sprinting && ax.sprintTapT <= 0) { autoTap(KEY_LEFT_SHIFT); ax.sprintTapT = 0.8; }
}

void Game::autoLook(double yaw, double pitch, double pace) { ax.wantYaw = yaw; ax.wantPitch = pitch; ax.pace = pace; ax.lookSet = true; }

// a direction of the galaxy as the look's frame turns it: the head in the cabin (the ship's frame), the ship's nose otherwise
void Game::autoLookSpace(const Vec3& dir, double pace) {
    const Vec3 d = normalize(dir);
    if (autoLookFrame() == LF_CABIN) { const Vec3 l = cameraBasis(ship.yaw, ship.pitch) * d; autoLook(std::atan2(l.x, l.z), std::asin(clampd(l.y, -1, 1)), pace); }
    else autoLook(std::atan2(d.x, d.z), std::asin(clampd(d.y, -1, 1)), pace);
}

// a direction of the ground's frame (east, up, north); in a vehicle from its nose
void Game::autoLookLocal(const Vec3& dir, double pace) {
    const Vec3 d = normalize(dir);
    double yaw = std::atan2(d.x, d.z);
    if (autoLookFrame() == LF_VEHICLE) yaw = wrapAngle(yaw - (surf.inBuggy ? surf.buggy.heading : surf.drone.heading));
    autoLook(yaw, std::asin(clampd(d.y, -1, 1)), pace);
}

void Game::autoLookAt(double x, double y, double z, double pace) {
    const Player& p = surf.player;
    autoLookLocal(Vec3(x - p.x, y - (p.y + p.eyeHeight), z - p.z), pace);
}

// the ship's nose onto a direction, eased by the hands (the arrows turn it at a key's pace, too sudden for a calm hand; X snaps). The
// cockpit's view is the nose itself: the look turns it there
void Game::autoAimShip(const Vec3& dir) {
    if (!settings.cabin || radar.on) { autoLookSpace(dir, 0.45); return; }
    ax.shipAim = true; ax.shipDir = normalize(dir);
}
double Game::autoShipOff(const Vec3& dir) const { return angleBetween(forwardOf(ship.yaw, ship.pitch), dir); }

// out of the front window, the eyes wandering a little every few seconds (the cockpit's view holds: the look is the ship there)
// the head to the front window (the cabin's own; the cockpit's view is the nose, which its aim turns)
void Game::autoEyesFront(double pace) { if (autoLookFrame() == LF_CABIN) autoLook(0, 0, pace); }

void Game::autoWindowLook(double pace) {
    AutoState& A = ax;
    if (autoLookFrame() != LF_CABIN) return;
    if (A.clock > A.driftNext) { A.driftYaw = A.rng.sym(0.32); A.driftPitch = A.rng.range(-0.1, 0.14); A.driftNext = A.clock + A.rng.range(5, 9); }
    autoLook(A.driftYaw, A.driftPitch, pace);
}

// over a parked world's disc: a point of it (to its limb) every few seconds
void Game::autoGazeOverBody(int b) {
    AutoState& A = ax;
    const Vec3 d = sys.bodyPos(b, t) - ship.pos;
    const double ang = std::asin(clampd(sys.bodies[b].radiusKm / std::max(length(d), 1.0), 0, 1));
    if (A.clock > A.driftNext) { const double r = A.rng.range(0.0, 0.85) * std::min(ang, 0.5), a = A.rng.range(0, TAU); A.driftYaw = r * std::cos(a); A.driftPitch = 0.8 * r * std::sin(a); A.driftNext = A.clock + A.rng.range(4.5, 8.5); }
    const Vec3 f = normalize(d), rt = normalize(cross(Vec3(0, 1, 0), f)), up = cross(f, rt);
    autoLookSpace(f + rt * std::tan(A.driftYaw) + up * std::tan(A.driftPitch), 0.45);
}

// round the horizon on the ground: a new way every few seconds, a turn of a half to two radians, the eyes level or a little up or down; in a
// vehicle the camera's pan
void Game::autoGazeRound(double pace) {
    AutoState& A = ax;
    const bool vehicle = autoLookFrame() == LF_VEHICLE;
    if (A.clock > A.driftNext || A.stepFirst) {
        A.driftYaw = vehicle ? A.rng.sym(0.8) : wrapAngle(surf.player.yaw + (A.rng.chance(0.5) ? 1 : -1) * A.rng.range(0.5, 1.9));
        A.driftPitch = A.rng.range(-0.12, 0.16);
        A.driftNext = A.clock + A.rng.range(3.5, 6.5);
    }
    autoLook(A.driftYaw, A.driftPitch, pace);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the cabin

bool Game::autoAtWindow() const { return !cabin.onRoof && std::hypot(cabin.x - CAB_WINDOW_X, cabin.z - CAB_WINDOW_Z) < 0.35; }

// a walk in the cabin to (x, z) round the capsule's cage, then a turn to `yaw`; true there, facing it
bool Game::autoCabinWalk(double tx, double tz, double yaw) {
    const Cabin& c = cabin;
    if (c.onRoof) { if (autoEvery(1.0)) autoTap(KEY_PAGE_DOWN); return false; }
    double wx = tx, wz = tz;
    const double dx = tx - c.x, dz = tz - c.z, d = std::hypot(dx, dz);
    if (d > 0.3) {   // the cage in the way: by a point at its side
        const double u = clampd(-(c.x * dx + c.z * dz) / (d * d), 0, 1), px = c.x + dx * u, pz = c.z + dz * u;
        if (std::hypot(px, pz) < 1.5) {
            double sx = -dz / d, sz = dx / d;
            if (sx * c.x + sz * c.z < 0) { sx = -sx; sz = -sz; }
            wx = sx * 1.7; wz = sz * 1.7;
            if (std::hypot(wx - c.x, wz - c.z) < 0.35) { wx = tx; wz = tz; }
        }
    }
    if (d < 0.14) {
        autoLook(yaw, 0, 0.7);
        return std::fabs(wrapAngle(c.yaw - yaw)) < 0.12;
    }
    const double h = std::atan2(wx - c.x, wz - c.z), rel = wrapAngle(h - c.yaw);
    autoLook(h, -0.12, 1.0);
    const double sp = (std::fabs(rel) > 0.9 ? 0.2 : 0.7) * std::min(1.0, d / 0.6 + 0.25);
    ax.wantFwd = sp * std::cos(rel); ax.wantSide = sp * std::sin(rel);
    return false;
}

// to the capsule's cage, facing it, and E (the capsule deploys; at a giant, a probe goes)
bool Game::autoCabinToCapsule() {
    const Cabin& c = cabin;
    double vx = c.x, vz = c.z, L = std::hypot(vx, vz);
    if (L < 1e-3) { vx = 0; vz = 1; L = 1; }
    const double sx = vx / L * 1.55, sz = vz / L * 1.55, face = std::atan2(-sx, -sz);
    if (!autoCabinWalk(sx, sz, face)) return false;
    autoLook(face, -0.25, 0.8);
    if (c.facing == 6 && autoEvery(1.0)) autoTap(KEY_E);
    return state != GameState::SPACE;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the walk, the drive, the flight

// the explorer's walk toward (x, z): the way planned in legs round what stands in it (`autoWay`), the stick pushed along it at `speed` (1
// a walk; the sprint on long legs while the breath allows) relative to where the eyes point, slowed in a sharp turn and before the stop; the
// eyes along the way on the ground a few metres ahead. No progress in three seconds: a jump with the jetpack over it (not on the lightest
// worlds), the way planned again; given up after five. 0 on the way, 1 there, -1 given up
int Game::autoWalkTo(double gx, double gz, double r, double speed, double dt, double replanMove) {
    AutoState& A = ax;
    Player& p = surf.player;
    if (surf.inVehicle()) return -1;
    const double d = std::hypot(gx - p.x, gz - p.z);
    if (d <= r) return 1;
    if (std::hypot(gx - A.goX, gz - A.goZ) > replanMove) { A.goX = gx; A.goZ = gz; A.way.clear(); A.wayI = 0; A.bestD = d; A.stillT = 0; }
    if (A.wayI >= A.way.size()) { autoWay(p.x, p.z, gx, gz, A.way); A.wayI = 0; if (A.way.empty()) A.way.push_back({gx, gz}); }
    while (A.wayI + 1 < A.way.size() && std::hypot(A.way[A.wayI].first - p.x, A.way[A.wayI].second - p.z) < 0.9) A.wayI++;
    const double tx = A.way[A.wayI].first, tz = A.way[A.wayI].second;
    if (A.wayI + 1 == A.way.size() && std::hypot(tx - p.x, tz - p.z) < 0.6) { A.wayI = A.way.size(); return 0; }   // the leg's end: the next leg next frame
    const double h = std::atan2(tx - p.x, tz - p.z);
    const double ahead = surf.site.groundHeight(p.x + std::sin(h) * 6, p.z + std::cos(h) * 6) - surf.site.groundHeight(p.x, p.z);
    autoLook(h, clampd(std::atan2(ahead, 6.0) * 0.6 - 0.07, -0.4, 0.3), 1.0);
    const double rel = wrapAngle(h - p.yaw);
    double sp = speed * (std::fabs(rel) > 0.9 ? 0.3 : 1.0);
    if (d < 4) sp *= std::max(0.3, d / 4);
    A.wantFwd = sp * std::cos(rel); A.wantSide = 0.8 * sp * std::sin(rel);
    autoSprint(d > 80 && speed >= 1 && p.stamina > (p.sprinting ? 25 : 60) && !p.swimming);
    if (A.jetT > 0) { A.jetT -= dt; autoHold(KEY_SPACE); }
    if (d < A.bestD - 0.6) { A.bestD = d; A.stillT = 0; } else A.stillT += dt;
    if (A.stillT > 3.0) {
        A.stillT = 0; A.unstick++; A.unsticks++;
        if (A.unstick > 5) return -1;
        if ((A.unstick & 1) && surf.site.gravity > 2 && !p.swimming) { autoTap(KEY_SPACE); A.jetT = 1.0; }
        A.way.clear(); A.wayI = 0;
    }
    return 0;
}

// the way round what stands between here and a goal (a leg of up to 24 m toward it): a grid of 40 cm cells round the leg, the colliders
// that stand at a body's height marked a body's width wide, the ground too steep to walk blocked and the deep water dear; A* over it to the
// goal, or to the reachable cell nearest it; the cells' path pulled straight where they see one another
void Game::autoWay(double x0, double z0, double x1, double z1, std::vector<std::pair<double, double>>& out) {
    out.clear();
    const int N = 150; const double C = 0.4, LEG = 24;
    const double dx = x1 - x0, dz = z1 - z0, L = std::hypot(dx, dz);
    const bool clipped = L > LEG;
    if (clipped) { x1 = x0 + dx / L * LEG; z1 = z0 + dz / L * LEG; }
    const double ox = 0.5 * (x0 + x1) - N * C / 2, oz = 0.5 * (z0 + z1) - N * C / 2;
    std::vector<uint8_t> blocked(N * N, 0);
    std::vector<float> cost(N * N, 1.0f), h(N * N);
    std::vector<SurfaceView::Collider> cols, all;
    for (int q = 0; q < 4; q++) { surf.collectColliders(ox + N * C * (q & 1 ? 0.75 : 0.25), oz + N * C * (q & 2 ? 0.75 : 0.25), cols); all.insert(all.end(), cols.begin(), cols.end()); }
    for (const SurfaceView::Collider& c : all) {
        const double gy = surf.site.groundHeight(c.x, c.z);
        if (c.y1 - 0.1 <= gy || c.y0 >= gy + 1.6) continue;
        const double rr = c.r + 0.28;
        const int i0 = std::max(0, (int)std::floor((c.x - rr - ox) / C)), i1 = std::min(N - 1, (int)std::floor((c.x + rr - ox) / C));
        const int j0 = std::max(0, (int)std::floor((c.z - rr - oz) / C)), j1 = std::min(N - 1, (int)std::floor((c.z + rr - oz) / C));
        for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) {
            const double cx = ox + (i + 0.5) * C - c.x, cz = oz + (j + 0.5) * C - c.z;
            if (cx * cx + cz * cz < rr * rr) blocked[j * N + i] = 1;
        }
    }
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) h[j * N + i] = (float)surf.site.groundHeight(ox + (i + 0.5) * C, oz + (j + 0.5) * C);
    for (int j = 0; j < N; j += 5) for (int i = 0; i < N; i += 5) {   // the water on a 2 m grid
        const double x = ox + (i + 2.5) * C, z = oz + (j + 2.5) * C;
        if (surf.site.waterNowAt(x, z) <= h[std::min(N - 1, j + 2) * N + std::min(N - 1, i + 2)] + 0.8) continue;
        for (int jj = j; jj < std::min(N, j + 5); jj++) for (int ii = i; ii < std::min(N, i + 5); ii++) cost[jj * N + ii] = 6.0f;
    }
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        const float hc = h[j * N + i];
        const float g = std::max(i + 1 < N ? std::fabs(h[j * N + i + 1] - hc) : 0.0f, j + 1 < N ? std::fabs(h[(j + 1) * N + i] - hc) : 0.0f);
        if (g > 1.15 * C) blocked[j * N + i] = 1;
    }
    auto cellOf = [&](double x, double z, int& i, int& j) { i = clampi((int)std::floor((x - ox) / C), 0, N - 1); j = clampi((int)std::floor((z - oz) / C), 0, N - 1); };
    int si, sj, gi, gj; cellOf(x0, z0, si, sj); cellOf(x1, z1, gi, gj);
    for (int j = sj - 1; j <= sj + 1; j++) for (int i = si - 1; i <= si + 1; i++) if (i >= 0 && j >= 0 && i < N && j < N) blocked[j * N + i] = 0;   // where the explorer stands
    const int s = sj * N + si, goal = gj * N + gi;
    std::vector<float> g(N * N, 1e30f); std::vector<int> par(N * N, -1); std::vector<uint8_t> closed(N * N, 0);
    auto hf = [&](int i, int j) { const int a = std::abs(i - gi), b = std::abs(j - gj); return (float)(std::max(a, b) + 0.4142 * std::min(a, b)); };
    typedef std::pair<float, int> QE;
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> open;
    g[s] = 0; open.push({hf(si, sj), s});
    int best = s; float bestH = hf(si, sj);
    while (!open.empty()) {
        const int u = open.top().second; open.pop();
        if (closed[u]) continue;
        closed[u] = 1;
        const int ui = u % N, uj = u / N;
        const float hu = hf(ui, uj);
        if (hu < bestH) { bestH = hu; best = u; }
        if (u == goal) break;
        for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
            if (!di && !dj) continue;
            const int vi = ui + di, vj = uj + dj;
            if (vi < 0 || vj < 0 || vi >= N || vj >= N) continue;
            const int v = vj * N + vi;
            if (blocked[v] || closed[v]) continue;
            if (di && dj && (blocked[uj * N + vi] || blocked[vj * N + ui])) continue;   // no corner cut
            const float ng = g[u] + (di && dj ? 1.4142f : 1.0f) * cost[v];
            if (ng < g[v]) { g[v] = ng; par[v] = u; open.push({ng + hf(vi, vj), v}); }
        }
    }
    std::vector<int> path;
    for (int u = best; u >= 0; u = par[u]) path.push_back(u);
    std::reverse(path.begin(), path.end());
    if (path.size() < 2) return;
    auto clear = [&](int a, int b) {   // the straight line between two cells crosses no blocked cell
        const double ax0 = a % N + 0.5, az0 = a / N + 0.5, bx0 = b % N + 0.5, bz0 = b / N + 0.5;
        const int n = (int)std::ceil(std::hypot(bx0 - ax0, bz0 - az0) * 3) + 1;
        for (int k = 0; k <= n; k++) { const double u = (double)k / n; const int i = (int)(ax0 + (bx0 - ax0) * u), j = (int)(az0 + (bz0 - az0) * u); if (blocked[j * N + i] || cost[j * N + i] > 1.5f) return false; }
        return true;
    };
    size_t anchor = 0;
    while (anchor + 1 < path.size()) {
        size_t far = anchor + 1;
        for (size_t k = path.size() - 1; k > anchor + 1; k--) if (clear(path[anchor], path[k])) { far = k; break; }
        out.push_back({ox + (path[far] % N + 0.5) * C, oz + (path[far] / N + 0.5) * C});
        anchor = far;
    }
    if (best == goal && !clipped) out.back() = {x1, z1};
}

// the buggy toward (x, z) at up to `cruise` m/s: steered onto the goal's heading, else the nearest heading either side whose next 36 m are dry
// and gentle (looked at twice a second); the throttle eased toward the speed the way allows (slower in a bend, toward the goal); a stall
// backed out of with the wheels the other way. The nose camera a little into the bend. 1 stopped within r, -1 given up
int Game::autoDrive(double gx, double gz, double r, double cruise, double dt) {
    AutoState& A = ax;
    if (!surf.inBuggy) return -1;
    const Buggy& b = surf.buggy;
    const double d = std::hypot(gx - b.x, gz - b.z);
    if (d <= r) {
        A.wantFwd = b.speed > 1 ? -0.5 : 0;
        if (std::fabs(b.speed) < 4) autoHold(KEY_SPACE);
        autoLook(0, -0.05, 0.6);
        return std::fabs(b.speed) < 0.5 ? 1 : 0;
    }
    const double hg = std::atan2(gx - b.x, gz - b.z);
    A.driveEvalT -= dt;
    if (A.driveEvalT <= 0) {
        A.driveEvalT = 0.5;
        static const double OFF[] = {0, 0.35, -0.35, 0.7, -0.7, 1.05, -1.05, 1.5, -1.5};
        double pick = 0; bool any = false;
        for (double o : OFF) if (autoDriveClear(b.x, b.z, hg + o)) { pick = o; any = true; break; }
        A.driveOff = any ? pick : 0;
    }
    const double e = wrapAngle(hg + A.driveOff - b.heading);
    if (A.reverseT > 0) { A.reverseT -= dt; A.wantFwd = -0.7; A.wantSide = e > 0 ? -1 : 1; autoLook(0, -0.05, 0.6); return 0; }
    const double vT = std::min(cruise, 0.45 * d + 3) * (std::fabs(e) > 0.6 ? 0.45 : 1.0);
    A.wantFwd = clampd((vT - b.speed) * 0.3, -1, 1);
    if (A.wantFwd < 0 && b.speed < vT + 2) A.wantFwd = 0;   // coasting rather than braking a little over
    // the steering within a calm turn (about 35 degrees a second): the buggy turns `speed tan(steer) / 2.2` radians a second, its lock 30
    // degrees at rest and 9 at its top speed (buggy.cpp), so the stick's share is held under what that turn allows at this speed
    const double lock = (30 - 21 * clampd(std::fabs(b.speed) / 50.0, 0, 1)) * DEG;
    const double steerMax = clampd(std::atan(0.6 * 2.2 / std::max(std::fabs(b.speed), 0.5)) / lock, 0.15, 0.8);
    A.wantSide = clampd(e * 1.8, -steerMax, steerMax);
    if (std::fabs(b.speed) < 0.8 && A.wantFwd > 0.3) A.stillT += dt; else A.stillT = 0;
    if (A.stillT > 2.5) { A.stillT = 0; A.reverseT = 1.8; A.unsticks++; if (++A.unstick > 4) return -1; }
    autoLook(clampd(e * 0.4, -0.5, 0.5), -0.06, 0.6);
    return 0;
}

bool Game::autoDriveClear(double x, double z, double heading) {
    const double hx = std::sin(heading), hz = std::cos(heading);
    double prev = surf.site.groundHeight(x, z);
    for (int k = 1; k <= 6; k++) {
        const double px = x + hx * 6 * k, pz = z + hz * 6 * k, g = surf.site.groundHeight(px, pz);
        if (surf.site.waterNowAt(px, pz) > g + 0.2 || std::fabs(g - prev) > 6 * 0.45) return false;
        prev = g;
    }
    return true;
}

// the drone toward (x, z): its height held `alt` over the highest ground of the way ahead (Space climbs, Shift descends), turned onto the
// goal, the thrust eased toward the speed the way allows (slower turning, climbing hard, nearing); the camera on the ground ahead. 1 within r
// and slowed, -1 not in the drone
int Game::autoFly(double gx, double gz, double r, double alt, double cruise, double dt) {
    AutoState& A = ax;
    if (!surf.inDrone) return -1;
    const Drone& d = surf.drone;
    const double dist = std::hypot(gx - d.x, gz - d.z), e = wrapAngle(std::atan2(gx - d.x, gz - d.z) - d.heading);
    const double hx = std::sin(d.heading), hz = std::cos(d.heading), reach = std::min(400.0, 60 + std::fabs(d.speed) * 5);
    double top = surf.site.surfaceHeight(d.x, d.z);
    for (double s = 40; s <= reach; s += 40) top = std::max(top, surf.site.surfaceHeight(d.x + hx * s, d.z + hz * s));
    const double wantY = top + alt;
    if (d.landed || d.y < wantY - 4) autoHold(KEY_SPACE);
    else if (d.y > wantY + 12 && d.altAboveGround > alt * 0.8) autoHold(KEY_LEFT_SHIFT);
    autoLook(clampd(e * 0.35, -0.6, 0.6), -0.22, 0.6);
    if (d.landed) { A.wantFwd = 0; return 0; }
    if (dist <= r) { A.wantFwd = clampd(-d.speed * 0.25, -1, 1); return std::fabs(d.speed) < 2.5 ? 1 : 0; }
    const double vT = std::min(cruise, 0.3 * dist + 4) * (std::fabs(e) > 0.5 ? 0.35 : 1.0) * (d.y < wantY - 25 ? 0.4 : 1.0);
    A.wantFwd = clampd((vT - d.speed) * 0.12, -1, 1);
    A.wantSide = clampd(e * 1.1, -0.5, 0.5);   // half the stick at most: a turn of 35 degrees a second, not the drone's 70
    return 0;
}

// the drone round a place: along the circle's tangent pulled onto it, slowly, the camera turned onto the place (its pan's 70 degrees keep it
// in the frame from the ring)
void Game::autoCircle(double cx, double cz, double radius, double alt) {
    AutoState& A = ax;
    const Drone& d = surf.drone;
    const double rr = std::max(1.0, std::hypot(d.x - cx, d.z - cz)), a = std::atan2(d.x - cx, d.z - cz);
    const double h = a + A.circleDir * PI / 2 + A.circleDir * clampd((rr - radius) / radius, -0.6, 0.6) * 0.9;
    const double e = wrapAngle(h - d.heading);
    const double gy = std::max(surf.site.surfaceHeight(d.x, d.z), surf.site.surfaceHeight(cx, cz));
    if (d.y < gy + alt - 4) autoHold(KEY_SPACE); else if (d.y > gy + alt + 10) autoHold(KEY_LEFT_SHIFT);
    A.wantFwd = clampd((16 - d.speed) * 0.12, -1, 1);
    A.wantSide = clampd(e * 1.1, -0.5, 0.5);
    const double gc = surf.site.surfaceHeight(cx, cz);
    autoLook(clampd(wrapAngle(std::atan2(cx - d.x, cz - d.z) - d.heading), -1.15, 1.15), clampd(std::atan2(gc + 4 - (d.y + 1), rr), -1.0, 0.3), 0.5);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the hands

void Game::autoHands(Input& in, double dt) {
    AutoState& A = ax;
    Input out;
    out.pad = in.pad;
    for (int k = 0; k < KEY_MAX; k++) out.down[k] = A.hold[k];
    for (int k : A.taps) { out.pressed[k] = true; out.down[k] = true; }
    // the stick, eased toward what the step asks (pushed and let back, never a key's on and off)
    const double rate = 2.2 * dt;
    A.fwd += clampd(A.wantFwd - A.fwd, -rate, rate); A.side += clampd(A.wantSide - A.side, -rate, rate);
    if (A.wantFwd == 0 && std::fabs(A.fwd) < 0.02) A.fwd = 0;
    if (A.wantSide == 0 && std::fabs(A.side) < 0.02) A.side = 0;
    out.moveY = A.fwd; out.moveX = A.side;
    // the gaze: a critically damped spring onto the target with a hand's limits (it eases off the rest and onto the aim), and the sway of a
    // hand at rest, three slow waves of a few tenths of a degree, less while it moves; a turn of the game's own (the approach's aim, a
    // camera's limit) taken up as the new rest
    const int lf = autoLookFrame();
    if (lf != A.lookFrame) { A.lookFrame = lf; A.setYaw = 1e9; }
    if (lf != LF_NONE) {
        double cy = 0, cp = 0, p0 = -85 * DEG, p1 = 85 * DEG, y0 = -PI, y1 = PI;
        switch (lf) {
            case LF_CABIN: cy = cabin.yaw; cp = cabin.pitch; break;
            case LF_SHIP: cy = ship.yaw; cp = ship.pitch; p0 = -89 * DEG; p1 = 89 * DEG; break;
            case LF_GROUND: cy = surf.player.yaw; cp = surf.player.pitch; break;
            case LF_VEHICLE:
                cy = wrapAngle(surf.player.yaw - (surf.inBuggy ? surf.buggy.heading : surf.drone.heading)); cp = surf.player.pitch;
                p0 = surf.inBuggy ? -SurfaceView::CAM_TILT_DOWN : -SurfaceView::DRONE_TILT_DOWN; p1 = surf.inBuggy ? SurfaceView::CAM_TILT_UP : SurfaceView::DRONE_TILT_UP;
                y0 = -SurfaceView::CAM_PAN; y1 = SurfaceView::CAM_PAN;
                break;
            case LF_PROBE: cy = probe.camYaw; cp = probe.camPitch; p0 = -75 * DEG; p1 = 85 * DEG; break;
            default: break;
        }
        if (A.setYaw > 1e8 || std::fabs(wrapAngle(cy - A.setYaw)) > 0.035 || std::fabs(cp - A.setPitch) > 0.035) { A.gYaw = cy; A.gPitch = cp; A.vYaw = A.vPitch = 0; }
        if (!A.lookSet) { A.wantYaw = A.gYaw; A.wantPitch = A.gPitch; A.pace = 0.6; }
        const double wy = y1 < PI ? clampd(wrapAngle(A.wantYaw), y0 + 0.03, y1 - 0.03) : A.wantYaw, wp = clampd(A.wantPitch, p0 + 0.03, p1 - 0.03);
        const double w = 1.7 * A.pace, vmax = 0.75 * A.pace, amax = 1.4 * A.pace;
        auto spring = [&](double err, double& v) { const double a = clampd(w * w * err - 2 * w * v, -amax, amax); v = clampd(v + a * dt, -vmax, vmax); return v * dt; };
        A.gYaw = wrapAngle(A.gYaw + spring(wrapAngle(wy - A.gYaw), A.vYaw));
        A.gPitch = clampd(A.gPitch + spring(wp - A.gPitch, A.vPitch), p0, p1);
        const double still = 1.0 / (1.0 + 3.0 * (std::fabs(A.vYaw) + std::fabs(A.vPitch))), c = A.clock, amp = A.sway * still * DEG * (lf == LF_VEHICLE ? 0.5 : 1.0);
        const double sy = amp * (0.55 * std::sin(0.23 * c + A.swayPh[0]) + 0.3 * std::sin(0.51 * c + A.swayPh[1]) + 0.15 * std::sin(1.17 * c + A.swayPh[2]));
        const double sp = amp * 0.6 * (0.55 * std::sin(0.19 * c + A.swayPh[3]) + 0.3 * std::sin(0.43 * c + A.swayPh[4]) + 0.15 * std::sin(0.97 * c + A.swayPh[5]));
        double ty = wrapAngle(A.gYaw + sy);
        if (y1 < PI) ty = clampd(ty, y0, y1);
        const double tp = clampd(A.gPitch + sp, p0, p1);
        const double k = 0.0032 * settings.mouseSensitivity;
        out.mouseDx = wrapAngle(ty - cy) / k;
        out.mouseDy = -(tp - cp) / k * (settings.invertY ? -1 : 1);
        A.setYaw = ty; A.setPitch = tp;
    }
    // the ship's nose eased onto its aim (parked or standing by; the approach and the flight turn it themselves, the telescope holds it)
    if (A.shipAim && state == GameState::SPACE && (ship.mode == ShipState::PARKED || ship.mode == ShipState::STANDBY) && !tele.on) {
        if (A.sYaw > 1e8 || std::fabs(wrapAngle(ship.yaw - A.sYaw)) > 0.02 || std::fabs(ship.pitch - A.sPitch) > 0.02) A.svYaw = A.svPitch = 0;
        const double ty = std::atan2(A.shipDir.x, A.shipDir.z), tp = clampd(std::asin(clampd(A.shipDir.y, -1, 1)), -88 * DEG, 88 * DEG);
        const double w = 1.1, vmax = 0.45, amax = 0.6;
        auto spring = [&](double err, double& v) { const double a = clampd(w * w * err - 2 * w * v, -amax, amax); v = clampd(v + a * dt, -vmax, vmax); return v * dt; };
        ship.yaw = wrap2pi(ship.yaw + spring(wrapAngle(ty - ship.yaw), A.svYaw));
        ship.pitch = clampd(ship.pitch + spring(tp - ship.pitch, A.svPitch), -89 * DEG, 89 * DEG);
        A.sYaw = ship.yaw; A.sPitch = ship.pitch;
    } else { A.svYaw = A.svPitch = 0; A.sYaw = 1e9; }
    // a lap round the parked world, eased in and out (Shift+arrows' turn at a hand's pace)
    if (state == GameState::SPACE && ship.mode == ShipState::PARKED && ship.parkedBody >= 0 && ship.parkedBelt < 0 && !tele.on) {
        const double k = 1 - std::exp(-dt / 1.2);
        A.orbitE += (A.wantOrbitE - A.orbitE) * k; A.orbitN += (A.wantOrbitN - A.orbitN) * k;
        if (std::fabs(A.orbitE) + std::fabs(A.orbitN) > 0.003) orbitShip(A.orbitE, A.orbitN, dt);
    } else A.orbitE = A.orbitN = 0;
    // the clock's pace, a decade at a time over a second and a half
    if (settings.clockMode == 0 && !almanacRunning && (state == GameState::SURFACE || state == GameState::SPACE)) {
        const double want = std::max(1.0, A.warpWant), now = std::max(1.0, timeWarp);
        if (std::fabs(std::log10(want / now)) > 0.01) timeWarp = std::pow(10.0, std::log10(now) + clampd(std::log10(want / now), -dt / 1.5, dt / 1.5));
        else timeWarp = want;
    }
    in = out;
}

// the first seconds: what plays and how to take the controls back, fading out (the only words on the screen while it plays, with the
// tape's track as it begins)
void Game::autoRender() {
    tapeRender();   // R-420
    const double a = ax.captionT < 5 ? 1.0 : clampd(1 - (ax.captionT - 5) / 1.5, 0, 1);
    if (a <= 0) return;
    const std::string s = fmt("AUTO-EXPLORE: %s - ANY %s ENDS IT", AUTO_PROFILE_NAMES[ax.profile], padInUse() ? "BUTTON" : "KEY");
    const bool music = !tape.tracks.empty();   // R-420: the tape's key, when there is music to play
    blendRectRGB(canvas, 0, UH - (music ? 21 : 12), UW - 1, UH - 1, rgb(0, 0, 0), (int)(120 * a));
    const uint32_t c = HUD_AMBER;
    const uint32_t faded = rgb((int)((c & 255) * a), (int)(((c >> 8) & 255) * a), (int)(((c >> 16) & 255) * a));
    if (music) drawTextCentered(canvas, UW / 2, UH - 19, padInUse() ? "SELECT STARTS AND STOPS THE MUSIC" : "M STARTS AND STOPS THE MUSIC", faded);
    drawTextCentered(canvas, UW / 2, UH - 10, s.c_str(), faded);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the chooser (the menu's AUTO-EXPLORE, the controller's commands)

void Game::openAutoScreen(GameState back) { autoSel = clampi(settings.autoProfile, 0, AP_COUNT - 1); autoBack = back; state = GameState::AUTO; audio.beep = 4; tapeList(); }

void Game::updateAutoScreen(const Input& in) {
    const int n = AP_COUNT + 1;   // the profiles and BACK
    if (in.wasPressed(KEY_UP)) { autoSel = (autoSel + n - 1) % n; audio.beep = 4; }
    if (in.wasPressed(KEY_DOWN)) { autoSel = (autoSel + 1) % n; audio.beep = 4; }
    if (in.wasPressed(KEY_ESCAPE) || (enterKey(in) && autoSel == AP_COUNT)) { state = autoBack; audio.beep = 4; return; }
    if (enterKey(in)) autoStart(autoSel);
}

void Game::renderAutoScreen() {
    const int top = 30, bottom = UH - 30, x0 = 8, x1 = UW - 8;
    blendRectRGB(canvas, x0, top, x1, bottom, rgb(0, 0, 0), 215);
    drawRectRGB(canvas, x0, top, x1, bottom, HUD_DIM);
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

// ---------------------------------------------------------------------------------------------------------------------------------------
// the harness

std::string Game::testAutoInfo() const {
    return fmt("%s (%s) %s: %s; towns %d, stars %d, parkings %d, landings %d, probes %d, records %d, outings %d, steps %d (%d given up), unsticks %d",
               AUTO_PROFILE_NAMES[ax.profile], AUTO_PROFILE_NAMES[ax.focus], ax.on ? "on" : "off", ax.doing.c_str(), ax.towns, ax.stars, ax.bodies, ax.landings, ax.probes,
               ax.records, ax.outings, ax.steps, ax.timeouts, ax.unsticks);
}

int Game::testAutoCount(int what) const {
    const int v[8] = {ax.stars, ax.bodies, ax.landings, ax.probes, ax.records, ax.outings, ax.timeouts, ax.unsticks};
    return what >= 0 && what < 8 ? v[what] : -1;
}

Vec3 Game::testViewDir() const {
    if (state == GameState::SURFACE || state == GameState::DESCENT || state == GameState::ASCENT) return forwardOf(surf.player.yaw, surf.player.pitch);
    if (state == GameState::PROBE) return forwardOf(probe.camYaw, probe.camPitch);
    return viewBasis().row(2);
}

std::string Game::testAngles() const { return fmt("ship %.4f %.4f cabin %.4f %.4f mode %d", ship.yaw, ship.pitch, cabin.yaw, cabin.pitch, (int)ship.mode); }
