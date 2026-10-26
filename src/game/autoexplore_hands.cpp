// Auto-explore's (R-419) hands: the step of the moment turned into the frame's input as a person's hand gives it (the
// gaze on a spring, the stick pushed and let back), in the cabin, on foot, in the buggy and the drone
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
inline double angleBetween(const Vec3& a, const Vec3& b) { return std::acos(clampd(dot(normalize(a), normalize(b)), -1, 1)); }
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
    if (c.facing == FACE_CAGE && autoEvery(1.0)) autoTap(KEY_E);
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
