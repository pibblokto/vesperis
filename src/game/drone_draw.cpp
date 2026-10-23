// C-18 (2026-10-11): the bay's drones drawn. One model (`droneModel`, in the drone's own metres) is the drone on its rack in the cabin
// (filled and lit by the cabin's light, game.h's DroneMat in the cabin's banks) and the drone on the bay's screen (its visible faces' edges
// in lines, turning while it is aboard). The surface drone's family (R-403): a short hull with the dark band of its eye, four ducted fans on
// arms, a beacon on a mast; smaller, and with what the surface drone has not: two arms under its nose ending in three-fingered hands, a hold
// under its back, its number in lamps on its deck.
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>

namespace {
const int BANK_HULL = 8, BANK_SCREEN = 9, BANK_LIGHT = 10, BANK_AMBER = 21, BANK_EYE = 24, BANK_RED = 26, BANK_BAY = 29, BANK_PAINT = 30;
// the hull's outline from above (x its right, z its nose): 0 the nose's right corner, round its right side, its tail, its left side
const double HULL_X[8] = {0.07, 0.16, 0.16, 0.09, -0.09, -0.16, -0.16, -0.07};
const double HULL_Z[8] = {0.25, 0.12, -0.14, -0.24, -0.24, -0.14, 0.12, 0.25};
const double Y_BELLY = 0.10, Y_BELT = 0.17, Y_TOP = 0.23, TOP_K = 0.78, BELLY_K = 0.92;
const double DUCT_X = 0.31, DUCT_Z = 0.19, DUCT_Y = 0.19, DUCT_R = 0.09, DUCT_H = 0.026;
// the rack against the left wall by the front window (the cabin's metres: +x right, +y up, +z the front window): three shelves, a drone on
// each facing the room (drone 1 on the top shelf), the bay's screen across its head
const double RACK_X0 = -2.5, RACK_X1 = -1.88, RACK_Z0 = 0.92, RACK_Z1 = 1.95, RACK_TOP = 1.66, SCREEN_Y0 = 1.38, SCREEN_Y1 = 1.60;
const double SHELF_Y[DRONE_COUNT] = {1.00, 0.52, 0.04};
const double RACK_DX = -2.19, RACK_DZ = 1.435;   // where a drone stands on its shelf
const double BAY_OUT_S = 2.2, DOCK_S = 2.2;      // its way out of the rack into the hull, and back (game/drones.cpp's clock)

Vec3 lerpV(const Vec3& a, const Vec3& b, double u) { return a + (b - a) * u; }
double smooth01(double u) { u = clampd(u, 0, 1); return u * u * (3 - 2 * u); }
}

void droneModel(const DronePose& pose, std::vector<DronePrim>& out) {
    out.clear();
    auto face = [&](std::initializer_list<Vec3> pts, int mat, double tint, const Vec3& from) {
        DronePrim p; p.kind = 0; p.mat = mat; p.tint = tint;
        for (const Vec3& v : pts) if (p.n < 6) p.p[p.n++] = v;
        Vec3 c(0, 0, 0); for (int i = 0; i < p.n; i++) c = c + p.p[i];
        c = c * (1.0 / p.n);
        Vec3 n = normalize(cross(p.p[1] - p.p[0], p.p[2] - p.p[0]));
        if (dot(n, c - from) < 0) n = -n;   // outward: away from what it wraps
        p.nrm = n;
        out.push_back(p);
    };
    auto tube = [&](const Vec3& a, const Vec3& b, double r, int mat) { DronePrim p; p.kind = 1; p.n = 2; p.p[0] = a; p.p[1] = b; p.r = r; p.mat = mat; out.push_back(p); };
    auto lamp = [&](const Vec3& a, double size, int mat) { DronePrim p; p.kind = 2; p.n = 1; p.p[0] = a; p.r = size; p.mat = mat; out.push_back(p); };
    const Vec3 mid(0, (Y_BELLY + Y_TOP) * 0.5, 0);
    // ---- the hull: its sides, the bevel to its deck (the nose's three the dark band of its eye), its deck, its belly
    Vec3 lo[8], belt[8], top[8];
    for (int i = 0; i < 8; i++) { lo[i] = Vec3(HULL_X[i] * BELLY_K, Y_BELLY, HULL_Z[i] * BELLY_K); belt[i] = Vec3(HULL_X[i], Y_BELT, HULL_Z[i]); top[i] = Vec3(HULL_X[i] * TOP_K, Y_TOP, HULL_Z[i] * TOP_K); }
    for (int i = 0; i < 8; i++) {
        const int j = (i + 1) % 8;
        const bool nose = i == 6 || i == 7 || i == 0;
        face({lo[i], lo[j], belt[j], belt[i]}, DM_PAINT, 0.88, mid);
        face({belt[i], belt[j], top[j], top[i]}, nose ? DM_DARK : DM_PAINT, nose ? 0.5 : 1.0, mid);
    }
    face({top[0], top[1], top[2], top[3]}, DM_PAINT, 1.06, mid); face({top[3], top[4], top[5], top[6]}, DM_PAINT, 1.06, mid); face({top[6], top[7], top[0], top[3]}, DM_PAINT, 1.06, mid);
    face({lo[0], lo[1], lo[2], lo[3]}, DM_PAINT, 0.6, mid); face({lo[3], lo[4], lo[5], lo[6]}, DM_PAINT, 0.6, mid); face({lo[6], lo[7], lo[0], lo[3]}, DM_PAINT, 0.6, mid);
    // its eye in the band (three lenses across it), the bay's stripe down its deck, its number in lamps by its tail, the beacon on its mast
    for (int i = -1; i <= 1; i++) lamp(Vec3(i * 0.034, (Y_BELT + Y_TOP) * 0.5 + 0.004, (HULL_Z[0] + HULL_Z[0] * TOP_K) * 0.5 + 0.008), i ? 1.6 : 2.2, DM_EYE);
    face({Vec3(-0.022, Y_TOP + 0.002, -0.09), Vec3(0.022, Y_TOP + 0.002, -0.09), Vec3(0.022, Y_TOP + 0.002, 0.15), Vec3(-0.022, Y_TOP + 0.002, 0.15)}, DM_ACCENT, 1.0, Vec3(0, 0, 0));
    for (int i = 0; i < std::max(1, std::min(3, pose.number)); i++) lamp(Vec3((i - (std::max(1, std::min(3, pose.number)) - 1) * 0.5) * 0.045, Y_TOP + 0.004, -0.135), 1.4, DM_NUMBER);
    tube(Vec3(0, Y_TOP, -0.16), Vec3(0, Y_TOP + 0.07, -0.16), 0.005, DM_METAL);
    lamp(Vec3(0, Y_TOP + 0.075, -0.16), 1.6, DM_BEACON);
    // ---- the four fans: a duct of ten facets on a strut from the hull, the fan's dark disc, its three blades while they turn slowly
    for (int q = 0; q < 4; q++) {
        const double sx = (q & 1) ? -1 : 1, sz = (q & 2) ? -1 : 1;
        const Vec3 c(sx * DUCT_X, DUCT_Y, sz * DUCT_Z);
        const Vec3 from(sx * 0.15, Y_BELT - 0.006, sz * 0.09);
        tube(from, c - normalize(Vec3(c.x - from.x, 0, c.z - from.z)) * DUCT_R, 0.013, DM_METAL);
        for (int i = 0; i < 10; i++) {
            const double a0 = i * TAU / 10, a1 = (i + 1) * TAU / 10;
            const Vec3 r0(std::cos(a0) * DUCT_R, 0, std::sin(a0) * DUCT_R), r1(std::cos(a1) * DUCT_R, 0, std::sin(a1) * DUCT_R);
            face({c + r0 - Vec3(0, DUCT_H, 0), c + r1 - Vec3(0, DUCT_H, 0), c + r1 + Vec3(0, DUCT_H, 0), c + r0 + Vec3(0, DUCT_H, 0)}, DM_PAINT, 0.95, c);
        }
        DronePrim disc; disc.kind = 0; disc.mat = DM_DARK; disc.tint = 0.3 + 0.5 * pose.blur; disc.nrm = Vec3(0, 1, 0);
        for (int i = 0; i < 6; i++) { const double a = i * TAU / 6; disc.p[disc.n++] = c + Vec3(std::cos(a) * DUCT_R * 0.9, -0.004, std::sin(a) * DUCT_R * 0.9); }
        out.push_back(disc);
        if (pose.blur < 0.7) {
            const double spin = pose.spin * ((q == 0 || q == 3) ? 1 : -1);
            for (int b = 0; b < 3; b++) { const double a = spin + b * TAU / 3; tube(c, c + Vec3(std::cos(a) * DUCT_R * 0.84, 0.002, std::sin(a) * DUCT_R * 0.84), 0.007, DM_METAL); }
        }
    }
    // ---- two skids (the surface drone's), the hold under its back
    for (int s = -1; s <= 1; s += 2) {
        tube(Vec3(s * 0.115, 0.006, -0.17), Vec3(s * 0.115, 0.006, 0.12), 0.008, DM_METAL);
        tube(Vec3(s * 0.115, 0.006, 0.12), Vec3(s * 0.115, 0.035, 0.16), 0.007, DM_METAL);
        for (const double z : {-0.1, 0.07}) tube(Vec3(s * 0.115, 0.006, z), Vec3(s * 0.085, Y_BELLY, z * 0.85), 0.006, DM_METAL);
    }
    {
        const double x0 = -0.065, x1 = 0.065, z0 = -0.2, z1 = -0.07, y0 = 0.045, y1 = Y_BELLY;
        const Vec3 hc(0, (y0 + y1) * 0.5, (z0 + z1) * 0.5);
        face({Vec3(x0, y0, z0), Vec3(x1, y0, z0), Vec3(x1, y0, z1), Vec3(x0, y0, z1)}, DM_METAL, 0.7, hc);
        face({Vec3(x0, y0, z1), Vec3(x1, y0, z1), Vec3(x1, y1, z1), Vec3(x0, y1, z1)}, DM_METAL, 0.85, hc);
        face({Vec3(x0, y0, z0), Vec3(x1, y0, z0), Vec3(x1, y1, z0), Vec3(x0, y1, z0)}, DM_METAL, 0.85, hc);
        face({Vec3(x0, y0, z0), Vec3(x0, y0, z1), Vec3(x0, y1, z1), Vec3(x0, y1, z0)}, DM_METAL, 0.8, hc);
        face({Vec3(x1, y0, z0), Vec3(x1, y0, z1), Vec3(x1, y1, z1), Vec3(x1, y1, z0)}, DM_METAL, 0.8, hc);
    }
    // ---- the two arms on the nose's flanks: a shoulder, the upper arm, the elbow, the forearm, the wrist, the hand's three fingers of two
    // joints each (folded, the upper arm down and back and the forearm forward, the hands held beside the nose like a crab's claws; reaching,
    // down and forward under it; the fingers closing on a find)
    const double a = smooth01(pose.arms), grip = clampd(pose.grip, 0, 1);
    for (int s = -1; s <= 1; s += 2) {
        const Vec3 sh(s * 0.165, 0.13, 0.10);
        const Vec3 u = normalize(lerpV(normalize(Vec3(s * 0.25, -0.55, -0.8)), normalize(Vec3(s * 0.2, -0.85, 0.45)), a));
        const Vec3 f = normalize(lerpV(normalize(Vec3(s * 0.02, 0.25, 0.97)), normalize(Vec3(-s * 0.12, -0.95, 0.25)), a));
        const Vec3 el = sh + u * 0.13, wr = el + f * 0.15, palm = wr + f * 0.036;
        tube(sh - Vec3(s * 0.016, 0, 0), sh + Vec3(s * 0.02, 0, 0), 0.02, DM_METAL);     // the shoulder's joint
        tube(sh, el, 0.014, DM_ACCENT);
        tube(el - Vec3(s * 0.016, 0, 0), el + Vec3(s * 0.016, 0, 0), 0.016, DM_METAL);   // the elbow
        tube(el, wr, 0.012, DM_ACCENT);
        tube(wr, palm, 0.017, DM_METAL);                                                  // the wrist and the palm
        Vec3 e1 = cross(f, Vec3(1, 0, 0));
        if (length(e1) < 0.2) e1 = cross(f, Vec3(0, 1, 0));
        e1 = normalize(e1);
        const Vec3 e2 = normalize(cross(f, e1));
        const double open = 0.8 - 0.62 * grip;   // radians off the hand's axis
        for (int j = 0; j < 3; j++) {
            const double ang = j * TAU / 3 + (s > 0 ? 0.0 : PI / 3);
            const Vec3 rad = e1 * std::cos(ang) + e2 * std::sin(ang);
            const Vec3 k0 = palm + rad * 0.016;
            const Vec3 k1 = k0 + normalize(f * std::cos(open) + rad * std::sin(open)) * 0.046;
            const Vec3 k2 = k1 + normalize(f * std::cos(open - 0.95) + rad * std::sin(open - 0.95)) * 0.036;
            tube(k0, k1, 0.009, DM_METAL);
            tube(k1, k2, 0.008, DM_METAL);
        }
    }
    (void)pose.lit;
}

// ---- the rack in the cabin -----------------------------------------------------------------------------------------------------------

// where a drone stands in the cabin now (its shelf; on its way into the hull or out of it), its height off the shelf and its fans' blur;
// false while it is away
bool Game::droneRackPlace(int k, double& x, double& lift, double& blur) const {
    x = RACK_DX; lift = 0; blur = 0;
    if (bay.clock < 0 || k < 0 || k >= DRONE_COUNT) return true;
    const DroneRun& R = bay.run[k];
    if (R.docked) {   // aboard: its fans winding down after a dock
        if (bay.planned && R.dock > 0 && bay.clock >= R.dock) blur = clampd(1 - (bay.clock - R.dock) / 1.5, 0, 1);
        return true;
    }
    const double tau = bay.clock - R.out;
    if (tau < 0) return true;                                   // waiting its turn on the shelf
    if (tau < BAY_OUT_S) {                                      // lifting off its shelf and backing into the hull
        lift = 0.06 * smooth01(tau / 0.6); blur = smooth01(tau / 0.6);
        x = RACK_DX - 0.8 * smooth01((tau - 0.6) / (BAY_OUT_S - 0.6));
        return true;
    }
    if (bay.planned && R.dock > 0) {
        const double sigma = R.dock - bay.clock;                // the way back: out of the hull and down onto its shelf
        if (sigma >= 0 && sigma < DOCK_S) {
            blur = 1; lift = 0.06 * smooth01(sigma / 0.6);
            x = RACK_DX - 0.8 * smooth01((sigma - 0.6) / (DOCK_S - 0.6));
            return true;
        }
    }
    return false;
}

double Game::testDroneRackX(int k) const { double x, lift, blur; return droneRackPlace(k, x, lift, blur) ? x : -99; }
double Game::testDroneRackY(int k) const { double x, lift, blur; return droneRackPlace(k, x, lift, blur) && k >= 0 && k < DRONE_COUNT ? SHELF_Y[k] + lift : -99; }

// F-05: the rack's drones for the cabin's bands, once a frame: where each stands, its pose and its model (`drawDroneRack`)
void Game::droneRackFrame(CabinFrame& f) const {
    f.ready = droneBayReady();
    for (int k = 0; k < DRONE_COUNT; k++) {
        f.here[k] = droneRackPlace(k, f.x[k], f.lift[k], f.blur[k]);
        if (!f.here[k]) continue;
        f.pose[k] = dronePoseAt(k);
        f.pose[k].blur = std::max(f.pose[k].blur, f.blur[k]);
        droneModel(f.pose[k], f.prims[k]);
    }
}

void Game::drawDroneRack(const CabinFrame& f) {
    const Mat3& look = f.look; const Vec3& eye = f.eye;
    const double hullShade = f.hullShade;
    auto toView = [&](const Vec3& p) { return look * (p - eye); };
    RasterParams solid; solid.bank = BANK_HULL; solid.grain = &cabinGrain; solid.grainScale = 12.0 * FB_SCALE; solid.grainWeight = 0.6;
    auto quad = [&](const RasterParams& prm, Vec3 a, Vec3 b, Vec3 c, Vec3 d, double shade) {
        RVert q[4]; const Vec3 pts[4] = {a, b, c, d};
        for (int k = 0; k < 4; k++) { const Vec3 v = toView(pts[k]); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; q[k].u = pts[k].x + pts[k].z * 0.31; q[k].v = pts[k].y + pts[k].z * 0.17; }
        rasterPolygon(fb, q, 4, prm, spaceR.proj);
    };
    auto lampAt = [&](const Vec3& p, int bank, double shade, int size) {
        const Vec3 v = toView(p);
        if (v.z <= NEAR_Z) return;
        RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = shade;
        rasterPoint3(fb, q, bank, spaceR.proj, true, size);
    };
    const double flick = f.flick;
    // ---- the rack: its back against the hull (a port behind each shelf, open while its drone is away), its sides, its shelves, its head
    // with the bay's screen across it (three cells, a drone's each: bright aboard and ready, breathing while the bay charges, dim while away)
    quad(solid, Vec3(RACK_X0 + 0.02, 0, RACK_Z0), Vec3(RACK_X0 + 0.02, 0, RACK_Z1), Vec3(RACK_X0 + 0.02, RACK_TOP, RACK_Z1), Vec3(RACK_X0 + 0.02, RACK_TOP, RACK_Z0), hullShade * 0.55);
    for (const double z : {RACK_Z0, RACK_Z1}) quad(solid, Vec3(RACK_X0, 0, z), Vec3(RACK_X1, 0, z), Vec3(RACK_X1, RACK_TOP, z), Vec3(RACK_X0, RACK_TOP, z), hullShade * 0.7);
    quad(solid, Vec3(RACK_X0, RACK_TOP, RACK_Z0), Vec3(RACK_X1, RACK_TOP, RACK_Z0), Vec3(RACK_X1, RACK_TOP, RACK_Z1), Vec3(RACK_X0, RACK_TOP, RACK_Z1), hullShade * 0.9);
    quad(solid, Vec3(RACK_X1, SCREEN_Y0 - 0.06, RACK_Z0), Vec3(RACK_X1, SCREEN_Y0 - 0.06, RACK_Z1), Vec3(RACK_X1, RACK_TOP, RACK_Z1), Vec3(RACK_X1, RACK_TOP, RACK_Z0), hullShade * 0.8);
    const bool ready = f.ready;
    for (int k = 0; k < DRONE_COUNT; k++) {
        const double y = SHELF_Y[k];
        quad(solid, Vec3(RACK_X0, y, RACK_Z0), Vec3(RACK_X1, y, RACK_Z0), Vec3(RACK_X1, y, RACK_Z1), Vec3(RACK_X0, y, RACK_Z1), hullShade * 0.85);
        quad(solid, Vec3(RACK_X1, y - 0.03, RACK_Z0), Vec3(RACK_X1, y - 0.03, RACK_Z1), Vec3(RACK_X1, y, RACK_Z1), Vec3(RACK_X1, y, RACK_Z0), hullShade * 0.7);
        const double x = f.x[k];
        const bool here = f.here[k];
        const bool docked = bay.clock < 0 || bay.run[k].docked;
        if (!docked || x < RACK_DX - 0.01) {   // the port in the back open: its drone out or on its way
            RasterParams dark; dark.bank = 0;
            quad(dark, Vec3(RACK_X0 + 0.03, y + 0.02, RACK_Z0 + 0.06), Vec3(RACK_X0 + 0.03, y + 0.02, RACK_Z1 - 0.06), Vec3(RACK_X0 + 0.03, y + 0.36, RACK_Z1 - 0.06), Vec3(RACK_X0 + 0.03, y + 0.36, RACK_Z0 + 0.06), 2);
        }
        // the shelf's lamp: green aboard and ready, amber charging, red blinking while its drone is out
        const Vec3 lampP(RACK_X1 + 0.004, y - 0.015, RACK_Z1 - 0.07);
        if (docked && here) lampAt(lampP, ready ? BANK_SCREEN : BANK_AMBER, ready ? 58 : 40 + 14 * std::sin(realTime * 2.4), 2);
        else lampAt(lampP, BANK_RED, std::fmod(realTime, 1.0) < 0.5 ? 60 : 22, 2);
        // the screen's cell
        const double z0 = RACK_Z0 + 0.05 + k * (RACK_Z1 - RACK_Z0 - 0.1) / 3, z1 = z0 + (RACK_Z1 - RACK_Z0 - 0.1) / 3 - 0.02;
        const double cell = cabin.facing == 11 ? 46 : (docked && here ? (ready ? 38 : 28 + 7 * std::sin(realTime * 2.4)) : (std::fmod(realTime + k * 0.3, 1.2) < 0.6 ? 24 : 14));
        RasterParams rs; rs.bank = BANK_BAY; rs.grain = &cabinGrain; rs.grainScale = 30.0 * FB_SCALE; rs.grainWeight = 0.5;
        RVert q[4]; const Vec3 pts[4] = {Vec3(RACK_X1 + 0.006, SCREEN_Y0, z1), Vec3(RACK_X1 + 0.006, SCREEN_Y0, z0), Vec3(RACK_X1 + 0.006, SCREEN_Y1, z0), Vec3(RACK_X1 + 0.006, SCREEN_Y1, z1)};
        for (int i = 0; i < 4; i++) { const Vec3 v = toView(pts[i]); q[i].x = v.x; q[i].y = v.y; q[i].z = v.z; q[i].shade = cell * flick; q[i].u = pts[i].x + pts[i].z; q[i].v = pts[i].y * 3 + realTime * 0.4; }
        rasterPolygon(fb, q, 4, rs, spaceR.proj);
    }
    // ---- the drones: each where it stands, facing the room (its nose +x, its right -z), lit by the cabin's lamp over the cage
    for (int k = 0; k < DRONE_COUNT; k++)
        if (f.here[k]) drawDronePrims(look, eye, Vec3(f.x[k], SHELF_Y[k] + f.lift[k], RACK_DZ), Vec3(1, 0, 0), Vec3(0, 1, 0), f.pose[k], nullptr, f.prims[k]);
}

// a drone's model drawn in the cabin's frame (the ship's metres): its feet at `origin`, its nose along `fwd`; lit by the cabin's lamp, or
// outside the hull by the sun (`sun`: the ship's metres, toward it)
void Game::drawDroneMesh(const Mat3& look, const Vec3& eye, const Vec3& origin, const Vec3& fwd, const Vec3& up, const DronePose& pose, const Vec3* sun) {
    std::vector<DronePrim> prims;
    droneModel(pose, prims);
    drawDronePrims(look, eye, origin, fwd, up, pose, sun, prims);
}

// F-05: a drone's model already made (`droneModel`), so the cabin's bands draw the rack's three without making them in every band
void Game::drawDronePrims(const Mat3& look, const Vec3& eye, const Vec3& origin, const Vec3& fwd, const Vec3& up, const DronePose& pose, const Vec3* sun, const std::vector<DronePrim>& prims) {
    const double lit = cabin.lightLevel;
    const Vec3 right = normalize(cross(up, fwd));
    auto toCab = [&](const Vec3& p) { return origin + right * p.x + up * p.y + fwd * p.z; };
    auto toView = [&](const Vec3& p) { return look * (p - eye); };
    const Vec3 bulb(0, 2.25, 0);
    for (const DronePrim& pr : prims) {
        if (pr.kind == 0) {
            const Vec3 n = right * pr.nrm.x + up * pr.nrm.y + fwd * pr.nrm.z;
            Vec3 c(0, 0, 0); for (int i = 0; i < pr.n; i++) c = c + toCab(pr.p[i]);
            c = c * (1.0 / pr.n);
            const double l = sun ? 0.14 + 0.86 * std::max(0.0, dot(n, *sun)) : 0.45 + 0.55 * std::max(0.0, dot(n, normalize(bulb - c)));
            const double li = sun ? 1.0 : lit;
            RasterParams rp; double base;
            switch (pr.mat) {
                case DM_DARK: rp.bank = 0; base = 6 + 10 * pr.tint; break;
                case DM_METAL: rp.bank = BANK_HULL; base = (16 + 34 * li) * pr.tint * l; break;
                case DM_ACCENT: rp.bank = BANK_BAY; base = (18 + 26 * li) * l; break;
                default: rp.bank = BANK_PAINT; base = (14 + 38 * li) * pr.tint * l; break;
            }
            RVert q[6]; bool ok = true;
            for (int i = 0; i < pr.n; i++) {
                const Vec3 v = toView(toCab(pr.p[i]));
                if (v.z < NEAR_Z) { ok = false; break; }
                q[i].x = v.x; q[i].y = v.y; q[i].z = v.z; q[i].shade = std::min(63.0, base);
            }
            if (ok) rasterPolygon(fb, q, pr.n, rp, spaceR.proj);
        } else if (pr.kind == 1) {
            const Vec3 a = toView(toCab(pr.p[0])), b = toView(toCab(pr.p[1]));
            if (a.z < NEAR_Z || b.z < NEAR_Z) continue;
            const int bank = pr.mat == DM_ACCENT ? BANK_BAY : BANK_HULL;
            const double li = sun ? 0.8 : lit;
            const double shade = pr.mat == DM_ACCENT ? 22 + 24 * li : 20 + 32 * li;
            const int th = clampi((int)std::lround(2 * pr.r / std::min(a.z, b.z) * spaceR.proj.f / FB_SCALE), 1, 4);
            RVert va, vb; va.x = a.x; va.y = a.y; va.z = a.z; va.shade = shade; vb = va; vb.x = b.x; vb.y = b.y; vb.z = b.z;
            rasterLine3(fb, va, vb, bank, spaceR.proj, true, th);
        } else {
            const Vec3 v = toView(toCab(pr.p[0]));
            if (v.z < NEAR_Z) continue;
            int bank = BANK_LIGHT; double shade = 58;
            if (pr.mat == DM_EYE) { bank = BANK_EYE; shade = pose.lit ? 60 : 20; }
            else if (pr.mat == DM_BEACON) { bank = BANK_RED; shade = pose.lit && std::fmod(pose.blink, 1.3) < 0.22 ? 63 : 18; }
            else if (pr.mat == DM_NUMBER) { bank = BANK_BAY; shade = 60; }
            RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = shade;
            rasterPoint3(fb, q, bank, spaceR.proj, true, v.z < 1.6 ? 2 : 1);
        }
    }
}

// ---- the drones outside: lights leaving the ship for the world and coming back to it -------------------------------------------------

void Game::drawDroneLights() {
    if (!bay.out || bay.clock < 0 || !droneBayHere()) return;
    const Mat3 look = cameraBasis(cabin.yaw, cabin.pitch);
    const Vec3 eye(cabin.x, cabin.onRoof ? 2.4 + 0.9 + 1.6 - 0.3 : 1.6, cabin.z);
    const Mat3 shipCam = cameraBasis(ship.yaw, ship.pitch);
    const Vec3 down = normalize(shipCam * (-ship.parkDir));   // toward the world, in the ship's metres
    const Vec3 sunDir = sys.valid ? normalize(shipCam * (sys.star.pos - ship.pos)) : Vec3(0, 1, 0);
    const Vec3 port(RACK_X0 - 0.4, 0.6, RACK_DZ);
    // its place `tau` seconds out of the hull: out from the hull's side, then down to the world faster and faster (the way back the same)
    auto placeAt = [&](int k, double tau) {   // out from the hull's side, rising to the windows and drifting ahead, then down to the world faster and faster
        const double side = 5 * (1 - std::exp(-tau * 1.3)), rise = 0.9 * (1 - std::exp(-tau * 2)) + (k - 1) * 0.5 * (1 - std::exp(-tau)), along = 3 * tau * tau * tau;
        return port + Vec3(-side, rise, 2.5 * tau) + down * along + Vec3(0, 0, (k - 1) * 0.12 * along);
    };
    for (int k = 0; k < DRONE_COUNT; k++) {
        const DroneRun& R = bay.run[k];
        if (R.docked) continue;
        double tau = bay.clock - (R.out + BAY_OUT_S);
        bool back = false;
        if (bay.planned && R.dock > 0 && bay.clock > R.dock - DOCK_S - 8) { tau = std::min(tau, R.dock - DOCK_S - bay.clock); back = true; }
        if (tau < 0 || tau > 8) continue;
        const Vec3 p = placeAt(k, tau);
        if (tau < 2.6) {   // near the hull: the drone itself past the windows, its nose along its way
            Vec3 way = placeAt(k, tau + 0.05) - p;
            if (back) way = -way;
            way = length(way) > 1e-6 ? normalize(way) : down;
            Vec3 upv = std::fabs(way.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
            upv = normalize(upv - way * dot(upv, way));
            DronePose pose = dronePoseAt(k); pose.blur = 1;
            drawDroneMesh(look, eye, p - upv * 0.15, way, upv, pose, &sunDir);
            continue;
        }
        const Vec3 v = look * (p - eye);   // far: its strobe
        if (v.z <= NEAR_Z) continue;
        const bool flash = std::fmod(realTime * 1.6 + k * 0.37, 1.0) < 0.18;
        RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = flash ? 63 : clampd(56 - tau * 3, 34, 60);
        rasterPoint3(fb, q, BANK_LIGHT, spaceR.proj, false, tau < 4.5 ? 3 : 2);
        double sx, sy;
        if (projectPoint(q, spaceR.proj, sx, sy)) fb.glowDisc(sx, sy, (flash ? 14 : 9) * FB_SCALE / (1 + 0.15 * tau), FB_SCALE, flash ? 22 : 12, BANK_LIGHT, true);
    }
}

// ---- the drone in lines on the bay's screen --------------------------------------------------------------------------------------------

namespace {
// a line on the canvas in its own pixels (finer than the logical grid at 2x and over), clipped to a box of them
void wireLine(RGBCanvas& c, double x0, double y0, double x1, double y1, uint32_t col, int bx0, int by0, int bx1, int by1) {
    const int n = (int)std::ceil(std::max(std::fabs(x1 - x0), std::fabs(y1 - y0))) + 1;
    for (int i = 0; i <= n; i++) {
        const double u = n ? (double)i / n : 0;
        const int x = (int)std::lround(x0 + (x1 - x0) * u), y = (int)std::lround(y0 + (y1 - y0) * u);
        if (x < bx0 || y < by0 || x > bx1 || y > by1) continue;
        c.px[(size_t)y * c.w + x] = col;
    }
}
}

void Game::drawDroneWire(int x0, int y0, int w, int h, const DronePose& pose, double yaw, uint32_t col) {
    std::vector<DronePrim> prims;
    droneModel(pose, prims);
    const int S = std::max(1, canvas.scale);
    const double cp = std::cos(-0.42), sp = std::sin(-0.42), cy = std::cos(yaw), sy = std::sin(yaw);
    auto rot = [&](const Vec3& p) { const double x = p.x * cy + p.z * sy, z = -p.x * sy + p.z * cy; return Vec3(x, p.y * cp - z * sp, p.y * sp + z * cp); };
    const double scale = std::min(w / 0.9, h / 0.5) * S;
    const double cx = (x0 + w * 0.5) * S, cyy = (y0 + h * (0.6 - 0.2 * clampd(pose.arms, 0, 1))) * S;   // its arms reaching down: the drone higher in the box
    auto px = [&](const Vec3& v, double& sx, double& sy2) { sx = cx + v.x * scale; sy2 = cyy - (v.y - 0.12) * scale; };
    const int bx0 = x0 * S, by0 = y0 * S, bx1 = (x0 + w) * S - 1, by1 = (y0 + h) * S - 1;
    const uint32_t dim = rgb((int)((col & 255) * 0.45), (int)(((col >> 8) & 255) * 0.45), (int)(((col >> 16) & 255) * 0.45));
    for (const DronePrim& pr : prims) {
        if (pr.kind == 0) {
            if (rot(pr.nrm).z > 0.02) continue;   // its back to the eye
            const uint32_t c = pr.mat == DM_DARK ? dim : col;
            for (int i = 0; i < pr.n; i++) {
                double ax, ay, bx, by; px(rot(pr.p[i]), ax, ay); px(rot(pr.p[(i + 1) % pr.n]), bx, by);
                wireLine(canvas, ax, ay, bx, by, c, bx0, by0, bx1, by1);
            }
        } else if (pr.kind == 1) {
            double ax, ay, bx, by; px(rot(pr.p[0]), ax, ay); px(rot(pr.p[1]), bx, by);
            wireLine(canvas, ax, ay, bx, by, pr.mat == DM_ACCENT ? HUD_AMBER : col, bx0, by0, bx1, by1);
        } else {
            double ax, ay; px(rot(pr.p[0]), ax, ay);
            uint32_t c = HUD_WHITE;
            if (pr.mat == DM_EYE) c = pose.lit ? HUD_CYAN : dim;
            else if (pr.mat == DM_BEACON) c = pose.lit && std::fmod(pose.blink, 1.3) < 0.22 ? HUD_RED : dim;
            const int r = std::max(1, S / 2);
            for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
                const int x = (int)std::lround(ax) + dx, y = (int)std::lround(ay) + dy;
                if (x >= bx0 && y >= by0 && x <= bx1 && y <= by1) canvas.px[(size_t)y * canvas.w + x] = c;
            }
        }
    }
}

// ---- the bay's map of the world ----------------------------------------------------------------------------------------------------

void Game::buildDroneMap(int body) {
    bay.map.clear(); bay.mapW = bay.mapH = 0; bay.mapSeed = 0;
    if (!sys.valid || body < 0 || body >= (int)sys.bodies.size()) return;
    const Body& b = sys.bodies[body];
    const PlanetMap* mp = spaceR.mapIfReady(b);   // F-02: never waited for (a world not mapped yet held the screen 150-200 ms); the screen asks again
    if (!mp) return;
    const PlanetMap& m = *mp;
    MatRamp ramps[MAT_COUNT]; double lf; bool atmo;
    mapRampsFor(b, ramps, lf, atmo);
    const int W = 376, H = 188;   // twice the screen's box: crisp at 2x and over
    bay.map.assign((size_t)W * H, 0);
    auto hAt = [&](double lon, double lat) { return m.heightAt(lon, lat); };
    const double dl = TAU / W;
    double sum2 = 0; int cnt = 0;
    for (int y = 0; y < H; y += 4) for (int x = 0; x < W; x += 4) {
        const double lon = (x + 0.5) / W * TAU - PI, lat = (0.5 - (y + 0.5) / H) * PI;
        const double gx = hAt(lon + dl, lat) - hAt(lon - dl, lat), gy = hAt(lon, lat - dl) - hAt(lon, lat + dl);
        sum2 += gx * gx + gy * gy; cnt++;
    }
    const double k = 0.5 / (std::sqrt(sum2 / std::max(1, cnt)) + 1e-9);
    const bool survey = sys.sunless();   // D-02: a sunless world's map the survey's, as the landing map's
    double hLo = 0, hHi = 1; if (survey) surveyRange(m.height.data(), m.height.size(), hLo, hHi);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        const double lon = (x + 0.5) / W * TAU - PI, lat = (0.5 - (y + 0.5) / H) * PI;
        const int i = m.texelIndex(lon, lat);
        const double gx = hAt(lon + dl, lat) - hAt(lon - dl, lat), gy = hAt(lon, lat - dl) - hAt(lon, lat + dl);
        const double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);
        const uint32_t c = mapColor(ramps, m.material[i], m.albedo[i] / 255.0, lf, atmo, shade, survey ? surveyHeight(m.height[i], hLo, hHi) : -1);
        // dimmed and cooled under the screen's lines
        const int r = (int)((c & 255) * 0.5), g = (int)(((c >> 8) & 255) * 0.55), bl = (int)(((c >> 16) & 255) * 0.6);
        bay.map[(size_t)y * W + x] = rgb(r, g, bl);
    }
    bay.mapW = W; bay.mapH = H; bay.mapSeed = b.seed;
}
