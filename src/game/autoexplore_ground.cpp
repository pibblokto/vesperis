// Auto-explore (R-419) on the ground: the outings (a view, the water, the life, a sight, a drive, a flight, the ruins, the
// sky), a town's walk, the travel between places
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
enum AutoTravel { TR_FOOT = 0, TR_BUGGY, TR_DRONE };
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the ground

// 1 the buggy, 2 the drone
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

// to a place of the ground: 0 on foot, 1 by buggy, 2 by drone (`circle`: a ring flown round it first)
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
