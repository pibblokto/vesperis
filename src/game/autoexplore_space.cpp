// Auto-explore (R-419) in space: the cabin's window, the systems, the moves between the stars; the landing's site; the
// probe's camera
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

namespace {
// the stargazer's draw by a star's class: the strange and the rare first
double classDraw(int cls) {
    static const double W[STAR_DRAWN_COUNT] = {1, 1, 3, 3, 2, 4, 0.5, 1.5, 2, 3, 4, 5, 5, 6};
    return cls >= 0 && cls < STAR_DRAWN_COUNT ? W[cls] : 1;
}
inline bool civWorld(int type) { return kindHas(type, KT_PEOPLE); }   // the only worlds a people lived on (C-01)
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
