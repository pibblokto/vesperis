// R-427 (2026-11-18): the maintenance room's vehicles (the user's: "remove it and put buggy and drone models on some maintenance platforms
// like it's maintenance room. Purely for display"). The buggy and the drone the capsule carries stand parked on their platforms, each the
// ground's shape (surface/vehicle_shape.h's stations, assembled as buggy.cpp and drone.cpp assemble them, at rest: unfolded, the wheels
// straight, the fans still, the lamps out) in the bay drones' primitives, drawn and lit as the rack's drones are (`drawDronePrims`, by the
// lamp over its platform) in the drones' paint. A parked model's faces are a polygon of six points at most: a wheel's or a fan's twelve-gon
// is three of them.
#include "game.h"
#include "ship_plan.h"
#include "surface/vehicle_shape.h"
#include <cmath>

namespace {
struct ModelOut {
    std::vector<DronePrim>& out;
    void face(std::initializer_list<Vec3> pts, const Vec3& nrm, int mat, double tint) {
        DronePrim p; p.kind = 0; p.mat = mat; p.tint = tint; p.nrm = normalize(nrm);
        for (const Vec3& v : pts) if (p.n < 6) p.p[p.n++] = v;
        out.push_back(p);
    }
    void tube(const Vec3& a, const Vec3& b, double r, int mat) { DronePrim p; p.kind = 1; p.n = 2; p.p[0] = a; p.p[1] = b; p.r = r; p.mat = mat; out.push_back(p); }
    void lamp(const Vec3& a, double size, int mat) { DronePrim p; p.kind = 2; p.n = 1; p.p[0] = a; p.r = size; p.mat = mat; out.push_back(p); }
    // a disc of twelve round `c` in the plane of `u` and `v` (its radius in them), facing `nrm`: three faces of six points
    void disc12(const Vec3& c, const Vec3& u, const Vec3& v, const Vec3& nrm, int mat, double tint) {
        Vec3 r[12];
        for (int k = 0; k < 12; k++) r[k] = c + u * std::cos(k * TAU / 12) + v * std::sin(k * TAU / 12);
        face({r[0], r[1], r[2], r[3], r[4], r[5]}, nrm, mat, tint);
        face({r[0], r[5], r[6], r[7], r[8], r[9]}, nrm, mat, tint);
        face({r[0], r[9], r[10], r[11]}, nrm, mat, tint);
    }
};
// the model's metres: x its right, y up, z its nose; a station along its length (f), across (s), up (u) as the ground's `at` takes them
Vec3 P(double f, double s, double u) { return Vec3(s, u, f); }
const Vec3 FWD(0, 0, 1), UP(0, 1, 0), SIDE(1, 0, 0);

// the wedge hull both share (buggy.cpp's and drone.cpp's): nose, hood, the front sensor band, roof, rear slope, tail; the flanks, the
// cabin's sides, the sensor band round them, the chrome trims; the band's height its own
void wedge(ModelOut& m, const VehicleWedge& S, double bandLo, double bandHi) {
    m.face({P(S.NOSE_F, -S.NOSE_W, S.NOSE_B), P(S.NOSE_F, S.NOSE_W, S.NOSE_B), P(S.NOSE_F, S.NOSE_W, S.NOSE_T), P(S.NOSE_F, -S.NOSE_W, S.NOSE_T)}, FWD, DM_PAINT, 0.85);
    m.face({P(S.NOSE_F, -S.NOSE_W, S.NOSE_T), P(S.NOSE_F, S.NOSE_W, S.NOSE_T), P(S.SHO_F, S.SHO_W, S.BELT), P(S.SHO_F, -S.SHO_W, S.BELT)}, UP * 0.96 + FWD * 0.28, DM_PAINT, 1.0);
    m.face({P(S.SHO_F, -S.SHO_W, S.BELT), P(S.SHO_F, S.SHO_W, S.BELT), P(S.CABF_F, S.CAB_W, S.ROOF), P(S.CABF_F, -S.CAB_W, S.ROOF)}, UP * 0.65 + FWD * 0.76, DM_DARK, 0.3);
    m.face({P(S.CABF_F, -S.CAB_W, S.ROOF), P(S.CABF_F, S.CAB_W, S.ROOF), P(S.CABR_F, S.CAB_W, S.ROOF), P(S.CABR_F, -S.CAB_W, S.ROOF)}, UP, DM_PAINT, 1.0);
    m.face({P(S.CABR_F, -S.CAB_W, S.ROOF), P(S.CABR_F, S.CAB_W, S.ROOF), P(S.TAIL_F, S.TAIL_W, S.TAIL_T), P(S.TAIL_F, -S.TAIL_W, S.TAIL_T)}, UP * 0.85 - FWD * 0.53, DM_PAINT, 0.9);
    m.face({P(S.TAIL_F, -S.TAIL_W, S.TAIL_B), P(S.TAIL_F, S.TAIL_W, S.TAIL_B), P(S.TAIL_F, S.TAIL_W, S.TAIL_T), P(S.TAIL_F, -S.TAIL_W, S.TAIL_T)}, FWD * -1.0, DM_PAINT, 0.7);
    for (int s = -1; s <= 1; s += 2) {
        const Vec3 nS = SIDE * s, off = nS * 0.012;
        m.face({P(S.NOSE_F, s * S.NOSE_W, S.NOSE_B), P(S.SHO_F, s * S.SHO_W, S.FLOOR), P(S.SHO_F, s * S.SHO_W, S.BELT), P(S.NOSE_F, s * S.NOSE_W, S.NOSE_T)}, nS * 0.8 + FWD * 0.6, DM_PAINT, 0.9);
        m.face({P(S.SHO_F, s * S.SHO_W, S.FLOOR), P(S.TAIL_F, s * S.TAIL_W, S.TAIL_B), P(S.TAIL_F, s * S.TAIL_W, S.BELT), P(S.SHO_F, s * S.SHO_W, S.BELT)}, nS, DM_PAINT, 0.92);
        m.face({P(S.SHO_F, s * S.SHO_W, S.BELT), P(S.TAIL_F, s * S.TAIL_W, S.BELT), P(S.TAIL_F, s * S.TAIL_W, S.TAIL_T), P(S.CABR_F, s * S.CAB_W, S.ROOF), P(S.CABF_F, s * S.CAB_W, S.ROOF)}, nS, DM_PAINT, 0.95);
        m.face({P(S.SHO_F - 0.05, s * S.SHO_W, bandLo) + off, P(S.CABR_F - 0.05, s * S.CAB_W, bandLo) + off, P(S.CABR_F - 0.05, s * S.CAB_W, bandHi) + off,
                P(S.SHO_F - 0.05, s * S.SHO_W, bandHi) + off}, nS, DM_DARK, 0.3);
        m.tube(P(S.NOSE_F, s * S.NOSE_W, S.NOSE_T), P(S.SHO_F, s * S.SHO_W, S.BELT), 0.02, DM_METAL);
        m.tube(P(S.SHO_F, s * S.SHO_W, S.BELT), P(S.TAIL_F, s * S.TAIL_W, S.BELT), 0.02, DM_METAL);
        m.tube(P(S.CABF_F, s * S.CAB_W, S.ROOF), P(S.CABR_F, s * S.CAB_W, S.ROOF), 0.02, DM_METAL);
    }
}
// the camera pod on the nose (its lens face dark, its lens out), the lidar puck on the roof, the mast at the rear with its light out, the
// headlight bar (unlit) and the tail light bar
void fittings(ModelOut& m, const VehicleWedge& S, double podF0, double puckF, double puckR, double puckU, const Vec3& mast0, const Vec3& mast1, double headW, double tailW) {
    const double f1 = S.NOSE_F, w = 0.13, u0 = S.NOSE_T, u1 = S.NOSE_T + 0.2;
    m.face({P(f1, -w, u0), P(f1, w, u0), P(f1, w, u1), P(f1, -w, u1)}, FWD, DM_DARK, 0.25);
    m.face({P(podF0, -w, u1), P(f1, -w, u1), P(f1, w, u1), P(podF0, w, u1)}, UP, DM_PAINT, 0.95);
    for (int s = -1; s <= 1; s += 2) m.face({P(podF0, s * w, u0), P(f1, s * w, u0), P(f1, s * w, u1), P(podF0, s * w, u1)}, SIDE * s, DM_PAINT, 0.85);
    m.lamp(P(f1 + 0.01, 0, S.CAM_U), 1.6, DM_EYE);
    Vec3 puck[6];
    for (int k = 0; k < 6; k++) puck[k] = P(puckF + std::cos(k * TAU / 6) * puckR, std::sin(k * TAU / 6) * puckR, puckU);
    m.face({puck[0], puck[1], puck[2], puck[3], puck[4], puck[5]}, UP, DM_METAL, 1.0);
    m.tube(mast0, mast1, 0.015, DM_METAL);
    m.lamp(mast1, 1.6, DM_BEACON);
    m.face({P(S.NOSE_F + 0.01, -headW, 0.5), P(S.NOSE_F + 0.01, headW, 0.5), P(S.NOSE_F + 0.01, headW, 0.6), P(S.NOSE_F + 0.01, -headW, 0.6)}, FWD, DM_METAL, 0.9);
    m.face({P(S.TAIL_F - 0.01, -tailW, 0.56), P(S.TAIL_F - 0.01, tailW, 0.56), P(S.TAIL_F - 0.01, tailW, 0.68), P(S.TAIL_F - 0.01, -tailW, 0.68)}, FWD * -1.0, DM_BEACON, 1.0);
}
}

// the buggy at rest (buggy.cpp's `drawBuggy` unfolded, its wheels straight and still): the hull, the pods over its wheels, the four
// wheels (a tyre of two faces and its tread, a hub and three spokes) on their suspension arms, its fittings
void buggyModel(std::vector<DronePrim>& out) {
    using namespace BuggyShape;
    out.clear();
    ModelOut m{out};
    wedge(m, BUGGY_WEDGE, 1.02, 1.32);
    for (int s = -1; s <= 1; s += 2)
        for (const double fx : {-WHEEL_X, WHEEL_X}) {   // five strips arcing over each wheel from the hull's side out past its face, the skirt down to the hub
            const double r = WHEEL_R + 0.16, wIn = s * 0.86, wOut = s * 1.06;
            for (int k = 0; k < 5; k++) {
                const double a0 = PI * (0.08 + 0.84 * k / 5.0), a1 = PI * (0.08 + 0.84 * (k + 1) / 5.0);
                m.face({P(fx + std::cos(a0) * r, wIn, 0.42 + std::sin(a0) * r), P(fx + std::cos(a0) * r, wOut, 0.42 + std::sin(a0) * r), P(fx + std::cos(a1) * r, wOut, 0.42 + std::sin(a1) * r),
                        P(fx + std::cos(a1) * r, wIn, 0.42 + std::sin(a1) * r)},
                       UP * std::sin((a0 + a1) * 0.5) + FWD * std::cos((a0 + a1) * 0.5), DM_PAINT, 0.95);
            }
            m.face({P(fx - r, wOut, 0.42), P(fx + r, wOut, 0.42), P(fx + r, wOut, 0.56), P(fx - r, wOut, 0.56)}, SIDE * s, DM_PAINT, 0.85);
        }
    for (int w = 0; w < 4; w++) {
        const double fx = (w & 1) ? WHEEL_X : -WHEEL_X, sx = (w & 2) ? -WHEEL_S : WHEEL_S, s = sx > 0 ? 1 : -1;
        const Vec3 hub = P(fx, sx, WHEEL_R), out0 = hub + SIDE * (s * 0.16), in0 = hub + SIDE * (s * -0.14);
        m.disc12(out0, FWD * WHEEL_R, UP * WHEEL_R, SIDE * s, DM_DARK, 0.6);
        m.disc12(in0, FWD * WHEEL_R, UP * WHEEL_R, SIDE * -s, DM_DARK, 0.4);
        for (int k = 0; k < 12; k++) {   // the tread round its rim
            const double a0 = k * TAU / 12, a1 = (k + 1) * TAU / 12, am = (a0 + a1) * 0.5;
            const Vec3 r0 = FWD * (std::cos(a0) * WHEEL_R) + UP * (std::sin(a0) * WHEEL_R), r1 = FWD * (std::cos(a1) * WHEEL_R) + UP * (std::sin(a1) * WHEEL_R);
            m.face({in0 + r0, out0 + r0, out0 + r1, in0 + r1}, FWD * std::cos(am) + UP * std::sin(am), DM_DARK, 0.5);
        }
        Vec3 h[6];
        for (int k = 0; k < 6; k++) h[k] = out0 + SIDE * (s * 0.02) + FWD * (std::cos(k * TAU / 6) * WHEEL_R * 0.45) + UP * (std::sin(k * TAU / 6) * WHEEL_R * 0.45);
        m.face({h[0], h[1], h[2], h[3], h[4], h[5]}, SIDE * s, DM_METAL, 0.8);
        for (int k = 0; k < 3; k++) {
            const Vec3 spoke = FWD * (std::cos(k * TAU / 3) * WHEEL_R * 0.4) + UP * (std::sin(k * TAU / 3) * WHEEL_R * 0.4);
            m.tube(out0 + SIDE * (s * 0.03) + spoke, out0 + SIDE * (s * 0.03) - spoke, 0.02, DM_METAL);
        }
        m.tube(P(fx * 0.55, sx * 0.55, FLOOR + 0.05), hub, 0.035, DM_METAL);   // the suspension arms
        m.tube(P(fx * 0.85, sx * 0.55, FLOOR + 0.25), hub + UP * 0.1, 0.03, DM_METAL);
    }
    fittings(m, BUGGY_WEDGE, 1.44, -0.5, 0.16, ROOF + 0.06, P(-1.15, 0.62, ROOF), P(-1.2, 0.66, ROOF + 0.7), 0.36, 0.6);
}

// the drone at rest (drone.cpp's `drawDrone` landed and unfolded, its fans still): the hull and its belly, the skids, the four ducted
// pods on their arms (a duct of twelve facets, the fan's dark disc, three blades), its fittings
void capsuleDroneModel(std::vector<DronePrim>& out) {
    using namespace DroneShape;
    out.clear();
    ModelOut m{out};
    wedge(m, DRONE_WEDGE, 1.0, 1.24);
    m.face({P(NOSE_F, -NOSE_W, NOSE_B), P(SHO_F, -SHO_W, FLOOR), P(SHO_F, SHO_W, FLOOR), P(NOSE_F, NOSE_W, NOSE_B)}, UP * -1.0, DM_PAINT, 0.6);   // the belly
    m.face({P(SHO_F, -SHO_W, FLOOR), P(TAIL_F, -TAIL_W, TAIL_B), P(TAIL_F, TAIL_W, TAIL_B), P(SHO_F, SHO_W, FLOOR)}, UP * -1.0, DM_PAINT, 0.6);
    for (int s = -1; s <= 1; s += 2) {   // the skids: a runner with an upturned tip, two struts up to the belly
        m.tube(P(-1.0, s * SKID_S, 0.0), P(0.95, s * SKID_S, 0.0), 0.04, DM_METAL);
        m.tube(P(0.95, s * SKID_S, 0.0), P(1.15, s * SKID_S, 0.14), 0.035, DM_METAL);
        m.tube(P(-0.6, s * SKID_S, 0.0), P(-0.5, s * 0.5, FLOOR + 0.02), 0.03, DM_METAL);
        m.tube(P(0.6, s * SKID_S, 0.0), P(0.5, s * 0.5, FLOOR + 0.02), 0.03, DM_METAL);
    }
    for (int k = 0; k < 4; k++) {
        const double fx = (k & 1) ? POD_F : -POD_F, sx = (k & 2) ? -POD_S : POD_S;
        const Vec3 c = P(fx, sx, POD_U);
        m.tube(P(fx * 0.5, sx * 0.45, BELT - 0.05), c, 0.05, DM_METAL);
        for (int i = 0; i < 12; i++) {
            const double a0 = i * TAU / 12, a1 = (i + 1) * TAU / 12, am = (a0 + a1) * 0.5;
            const Vec3 r0 = FWD * (std::cos(a0) * POD_R) + SIDE * (std::sin(a0) * POD_R), r1 = FWD * (std::cos(a1) * POD_R) + SIDE * (std::sin(a1) * POD_R);
            m.face({c + r0 - UP * 0.11, c + r1 - UP * 0.11, c + r1 + UP * 0.11, c + r0 + UP * 0.11}, FWD * std::cos(am) + SIDE * std::sin(am), DM_PAINT, 0.9);
        }
        m.disc12(c, FWD * (POD_R * 0.9), SIDE * (POD_R * 0.9), UP, DM_DARK, 0.4);
        for (int b = 0; b < 3; b++) {
            const double a = b * TAU / 3 + (k & 1 ? 0.3 : -0.3);
            m.tube(c + UP * 0.02, c + UP * 0.02 + FWD * (std::cos(a) * POD_R * 0.82) + SIDE * (std::sin(a) * POD_R * 0.82), 0.03, DM_METAL);
        }
    }
    fittings(m, DRONE_WEDGE, 1.07, -0.35, 0.14, ROOF + 0.05, P(-0.9, 0.5, ROOF - 0.05), P(-0.95, 0.54, ROOF + 0.6), 0.3, 0.45);
}

// a vehicle on its platform, where it comes in its room's order: its model once made, at its feet's place, its nose its fixture's way,
// lit by the lamp over it (the room's bulb nearest it), its lamps out
void Game::drawVehicle(CabinFrame& f, const ShipFixture& F) {
    static std::vector<DronePrim> models[2];
    if (models[0].empty()) { buggyModel(models[0]); capsuleDroneModel(models[1]); }
    const Vec3 origin(F.x[0], F.y[0], F.z[0]), fwd = normalize(Vec3(F.x[1] - F.x[0], 0, F.z[1] - F.z[0]));
    Vec3 bulb = origin + Vec3(0, 4, 0);
    double best = 1e30;
    for (const ShipLamp& L : shipLamps())
        if (L.room == F.room && L.kind == LAMP_BULB) {
            const double d = std::hypot(L.x - origin.x, L.z - origin.z);
            if (d < best) { best = d; bulb = Vec3(L.x, L.y, L.z); }
        }
    DronePose pose; pose.lit = false;
    drawDronePrims(f.look, f.eye, origin, fwd, Vec3(0, 1, 0), pose, nullptr, models[clampi(F.slots, 0, 1)], f.shapes, &bulb);
}
