// R-403 (2026-10-02): the drone. The buggy's hull (a low wedge with the blacked-out sensor band, the camera pod on the
// nose, the mast and the lidar puck, the light bars) shortened and set on two skids, with four ducted thrust pods on
// arms where the wheels were: the second vehicle the capsule carries, for the ruins the buggy takes hours to reach. Flown
// through the nose camera like the buggy is driven (`SurfaceView::render` mounts the eye in the pod, `cameraFeed` gives
// the picture its look; the hull is never drawn from inside): Space lifts it off and climbs, Shift descends and lands,
// W/S thrust fore and aft to 320 km/h, A/D turn (the hull banks into the turn, the camera's gimbal keeps the picture
// level), a ceiling of 400 m over the ground, no fuel. The skids cannot go under the ground or the water; what the
// buggy collides with the drone collides with when its hull is at that height, and a hard arrival is a thump.
#include "surface_view.h"
#include "core/noise.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

namespace {
const double TOP = SurfaceView::DRONE_TOP_SPEED, REVERSE = 12.0;   // m/s: 320 km/h, and backwards
const double CLIMB = 15.0, DESCEND = 12.0;                          // m/s asked for with Space and Shift
// the hull's stations (metres along the length from the centre) and their half widths and heights: the buggy's wedge, shorter
const double NOSE_F = 1.25, SHO_F = 0.8, CABF_F = 0.4, CABR_F = -0.6, TAIL_F = -1.2;
const double NOSE_W = 0.36, SHO_W = 0.7, CAB_W = 0.72, TAIL_W = 0.55;
const double FLOOR = 0.35, NOSE_B = 0.45, NOSE_T = 0.72, BELT = 0.9, ROOF = 1.35, TAIL_B = 0.42, TAIL_T = 0.98;
const double CAM_F = 1.21, CAM_U = 0.82;                 // the nose camera's lens (the pod spans 1.07..1.25 x 0.72..0.92)
const double POD_F = 1.35, POD_S = 1.45, POD_U = 1.05, POD_R = 0.5;   // the thrust pods: fore and aft, sideways, their height and radius
const double SKID_S = 0.5;                               // the skids' half track
}

void SurfaceView::droneFrame(Vec3& fwdT, Vec3& sideT, Vec3& upT) const {
    const Drone& d = drone;
    double hx = std::sin(d.heading), hz = std::cos(d.heading);
    Vec3 fwd(hx, 0, hz), side(-hz, 0, hx), up(0, 1, 0);
    fwdT = normalize(fwd + up * std::tan(d.pitch));
    sideT = normalize(side + up * std::tan(d.roll));
    upT = normalize(cross(sideT, fwdT));
    if (upT.y < 0) upT = -upT;
}

// the eye of the pilot: the nose camera's lens, in local metres (the pod rides the hull; the gimbal levels the picture, not the mount)
Vec3 SurfaceView::droneCameraMount() const {
    Vec3 fwdT, sideT, upT;
    droneFrame(fwdT, sideT, upT);
    return Vec3(drone.x, drone.y, drone.z) + fwdT * CAM_F + upT * CAM_U;
}

void SurfaceView::updateDrone(double dt, const Input& in, double t) {
    Drone& d = drone;
    Player& p = player;
    double g = site.gravity;
    p.yaw += in.mouseDx * 0.0032 * mouseSens;
    p.pitch -= in.mouseDy * 0.0032 * mouseSens * (invertY ? -1 : 1);
    // the nose camera pans 70 deg each way like the buggy's and tilts 60 down (the ground is what you look for) and 25 up
    double look = clampd(wrapAngle(p.yaw - d.heading), -CAM_PAN, CAM_PAN);
    p.pitch = clampd(p.pitch, -DRONE_TILT_DOWN, DRONE_TILT_UP);
    double throttle = (in.isDown(KEY_W) || in.isDown(KEY_UP) ? 1 : 0) - (in.isDown(KEY_S) || in.isDown(KEY_DOWN) ? 1 : 0);
    double steerIn = (in.isDown(KEY_D) || in.isDown(KEY_RIGHT) ? 1 : 0) - (in.isDown(KEY_A) || in.isDown(KEY_LEFT) ? 1 : 0);
    if (in.moveY != 0 || in.moveX != 0) { throttle = in.moveY; steerIn = in.moveX; }   // M6-02: a stick
    bool up = in.isDown(KEY_SPACE), down = in.shift();
    if (in.ctrl()) { throttle = 0; steerIn = 0; up = down = false; }   // Ctrl+S saves: not a flight
    auto thump = [&](double v) { d.thud = std::max(d.thud, v); d.jolt = std::max(d.jolt, v); };
    double hx = std::sin(d.heading), hz = std::cos(d.heading);
    double surface = site.surfaceHeight(d.x, d.z);
    d.altAboveGround = std::max(0.0, d.y - surface);
    d.ceiling = false;
    double vyT = 0, accel = 0;
    if (d.landed) {
        // on its skids: the pods idle, Space spins them up and lifts it off; nothing else moves it
        d.speed *= std::exp(-dt * 4); d.vy = 0; d.y = surface; d.steer *= std::exp(-dt * 4);
        d.rotor += ((up ? 1.0 : 0.3) - d.rotor) * (1 - std::exp(-dt * 1.6));
        if (up && d.rotor > 0.7) { d.landed = false; d.vy = 1.5; d.flightT = 0; }   // the spin-up takes a moment
        d.pitch *= std::exp(-dt * 3); d.roll *= std::exp(-dt * 3);
    } else {
        d.flightT += dt;
        double v = d.speed, av = std::fabs(v);
        // thrust fore and aft: 9 m/s^2 forward, 5 backwards, x1.8 against the motion (a brake); the drag is quadratic with a
        // little base drag, so the top speed is the clamp and a released stick coasts to a stop in a minute or two
        accel = throttle > 0 ? 9.0 : (throttle < 0 ? -5.0 : 0.0);
        if (v * throttle < 0) accel *= 1.8;
        double drag = 0.0008 * v * av + 0.5 * (v > 0.2 ? 1 : (v < -0.2 ? -1 : 0));
        d.speed = clampd(d.speed + (accel - drag) * dt, -REVERSE, TOP);
        if (std::fabs(d.speed) < 0.1 && throttle == 0) d.speed = 0;
        // turning: a yaw rate of 70 deg/s at rest shrinking to 24 at the top speed; the hull banks into the turn
        double lock = (70 - 46 * clampd(av / TOP, 0, 1)) * DEG;
        d.steer += (steerIn * lock - d.steer) * (1 - std::exp(-dt * 4));
        d.heading = wrap2pi(d.heading + d.steer * dt);
        hx = std::sin(d.heading); hz = std::cos(d.heading);
        // the vertical: Space climbs at 15 m/s, Shift descends at 12, neither holds the height; the ceiling is 400 m over the ground
        vyT = up ? CLIMB : (down ? -DESCEND : 0.0);
        d.ceiling = d.altAboveGround >= DRONE_CEILING - 1;
        if (d.ceiling && vyT > 0) vyT = 0;
        d.vy += (vyT - d.vy) * (1 - std::exp(-dt * 1.8));
        double nx = d.x + hx * d.speed * dt, nz = d.z + hz * d.speed * dt;
        // trunks, logs, walls and big rocks at the hull's height stop it (B-405: a collider spans what it blocks); small rocks pass under the skids
        {
            static std::vector<Collider> cols;
            collectColliders(nx, nz, cols);
            for (const Collider& c : cols) {
                if (c.kind == 0 && c.r < 0.72) continue;
                if (c.y1 < d.y + 0.2 || c.y0 > d.y + 1.5) continue;   // under the skids or over the roof
                double dx = nx - c.x, dz = nz - c.z, rr = c.r + 1.2, d2 = dx * dx + dz * dz;
                if (d2 < rr * rr && d2 > 1e-9) {
                    double dd = std::sqrt(d2);
                    nx = c.x + dx / dd * rr; nz = c.z + dz / dd * rr;
                    if (std::fabs(d.speed) > 2) thump(std::min(1.0, std::fabs(d.speed) / 20.0));
                    d.speed = -d.speed * 0.3;
                    d.vibration += 0.6;
                }
            }
        }
        d.x = nx; d.z = nz;
        // the ground (or the water): the skids cannot go under it; a hard arrival or a fast one is a thump, a scrape costs the speed;
        // settled on it with the speed off and Space released, it has landed
        double surfaceN = site.surfaceHeight(d.x, d.z);
        d.y += d.vy * dt;
        if (d.y <= surfaceN) {
            double rel = -d.vy, av2 = std::fabs(d.speed);
            d.y = surfaceN;
            if (rel > 2.5 || av2 > 8) { thump(std::min(1.0, std::max(0.0, rel) * 0.12 + av2 * 0.02)); d.vibration += std::min(1.0, std::max(0.0, rel) * 0.08 + av2 * 0.01); }
            if (av2 > 8) d.speed *= 0.5;
            d.speed *= std::exp(-dt * 2.0);
            if (d.vy < 0) d.vy = 0;
            if (!up && std::fabs(d.speed) < 3) { d.landed = true; d.speed = 0; d.vy = 0; }
        }
        d.altAboveGround = std::max(0.0, d.y - surfaceN);
        // the attitude, for the outside view: the hull banks into a turn (a coordinated turn's angle, at most 50 deg) and dips its
        // nose with the thrust and the speed
        double bank = clampd(std::atan(d.steer * d.speed / std::max(g, 1.0)), -50 * DEG, 50 * DEG);
        d.roll += (bank - d.roll) * (1 - std::exp(-dt * 3));
        double nose = -(accel * 0.02 + d.speed / TOP * 0.14);
        d.pitch += (nose - d.pitch) * (1 - std::exp(-dt * 3));
        double want = clampd(0.5 + 0.3 * std::max(0.0, d.vy / CLIMB) + 0.15 * std::fabs(throttle) + 0.1 * std::fabs(d.speed) / TOP, 0.4, 1.0);
        d.rotor += (want - d.rotor) * (1 - std::exp(-dt * 2.5));
        d.vibration += (0.3 * d.rotor + 0.004 * std::fabs(d.speed)) * dt * 4;
        d.odometer += std::fabs(d.speed) * dt;
        d.topSpeed = std::max(d.topSpeed, std::fabs(d.speed));
        // the downwash: dust, snow or spray thrown up from soft ground or water under a low hover
        if (d.altAboveGround < 8) {
            int mat = site.lod0.at((int)std::floor(d.x / 16), (int)std::floor(d.z / 16)).material;
            bool wet = site.waterAt(d.x, d.z) > -1e8 && site.groundHeight(d.x, d.z) < site.waterAt(d.x, d.z) - 0.2;
            int kind = wet ? 1 : ((mat == MAT_SAND || mat == MAT_DUST) ? 0 : (mat == MAT_SNOW ? 1 : (mat == MAT_GRASS && env.rain > 0.1 ? 2 : -1)));
            d.puffAccum += dt;
            if (kind >= 0 && d.puffAccum > 0.08) {
                d.puffAccum = 0;
                Rng rr((uint64_t)(t * 1000) ^ 0xD20E);
                double gy = surfaceN;
                for (int i = 0; i < 3; i++) { double a = rr.range(0, TAU), r = rr.range(1.5, 4.5) * (1 + 0.3 * (8 - d.altAboveGround)); d.puffs.push_back({(float)(d.x + std::cos(a) * r), (float)(gy + 0.2), (float)(d.z + std::sin(a) * r), 0.f, (uint8_t)kind}); }
            }
        }
    }
    d.vibration *= std::exp(-dt * 4);
    d.jolt *= std::exp(-dt * 5);
    d.fanSpin += dt * TAU * (2.0 + 22.0 * d.rotor);
    d.bumpT -= dt;
    d.lights = env.sun.dirLocal.y < 0.08;
    p.yaw = wrap2pi(d.heading + look);   // the camera turns with the hull, keeping its pan
    p.x = d.x; p.z = d.z; p.y = d.y;
    p.vx = hx * d.speed; p.vz = hz * d.speed; p.vy = d.vy; p.onGround = d.landed; p.swimming = false; p.underwater = false; p.diving = false;
    p.altAboveGround = d.altAboveGround;
    p.jetOn = false; p.jetHeat = std::max(0.0, p.jetHeat - dt * 8.0);
    p.stamina = clampd(p.stamina + dt * 16, 0, 100);
    p.bobY *= std::exp(-dt * 6); p.bobX *= std::exp(-dt * 6); p.landDip *= std::exp(-dt * 5);
    p.fovKick = 0; p.sprintRamp = 0; p.sprinting = false;
}

// F within 10 m of the capsule: a new drone unfolds on the capsule's other side from the buggy's; a drone already out, wherever it
// was left, is scrapped (as the buggy, R-204)
bool SurfaceView::deployDrone() {
    if (inVehicle()) return false;
    if (site.escapeVelocity < 30) return false;   // O4: on a comet 320 km/h is an escape
    double dx = capsuleX - player.x, dz = capsuleZ - player.z;
    if (dx * dx + dz * dz > 10.0 * 10.0) return false;
    double ax = player.x - capsuleX, az = player.z - capsuleZ;
    double al = std::sqrt(ax * ax + az * az) + 1e-9;
    drone = Drone();
    drone.deployed = true;
    drone.x = capsuleX + az / al * 5.0; drone.z = capsuleZ - ax / al * 5.0;
    drone.heading = std::atan2(-az, -ax);
    drone.y = site.surfaceHeight(drone.x, drone.z);
    drone.landed = true; drone.unfold = 0; drone.rotor = 0;
    return true;
}

// E: in when it stands unfolded within 3.5 m, out when it is on the ground (never in the air)
bool SurfaceView::toggleDrone() {
    if (inDrone) {
        if (!drone.landed) return false;
        inDrone = false;
        chaseCam = false;
        player.x = drone.x - std::cos(drone.heading) * 1.8; player.z = drone.z + std::sin(drone.heading) * 1.8;
        player.y = site.surfaceHeight(player.x, player.z);
        player.vx = player.vz = player.vy = 0;
        player.yaw = drone.heading;
        return true;
    }
    if (inBuggy || !drone.deployed || drone.unfold < 1 || !drone.landed || droneDist() > 3.5) return false;
    inDrone = true;
    player.autoWalk = 0; player.sprinting = false; player.crouch = false; player.hindLegs = false;
    player.yaw = drone.heading; player.pitch = 0;
    return true;
}

void SurfaceView::drawDrone(Framebuffer& fb, double t) {
    const Drone& d = drone;
    drawPuffs(fb, d.puffs);
    if (inDrone && !chaseCam) return;   // from inside you see the nose camera's picture, never the hull (R-204)
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    double dist = std::sqrt((d.x - camPos.x) * (d.x - camPos.x) + (d.z - camPos.z) * (d.z - camPos.z));
    double sc = 0.3 + 0.7 * d.unfold;   // the hull grows out of its folded state
    if (d.altAboveGround < 40) drawBlobShadow(fb, d.x, d.z, 1.6 * sc * (1 - d.altAboveGround / 40), std::min(d.altAboveGround + 0.8, 6.0));   // the shadow shrinks away with the height
    Vec3 fwdT, sideT, upT;
    droneFrame(fwdT, sideT, upT);
    Vec3 base(d.x, d.y, d.z);
    auto at = [&](double f, double s, double u) { return base + fwdT * (f * sc) + sideT * (s * sc) + upT * (u * sc); };
    double fog = 1 - std::exp(-dist / env.fogDistance);
    RasterParams rpBody; rpBody.bank = 6; rpBody.grain = &grain; rpBody.grainScale = 3.0 * FB_SCALE;
    RasterParams rpDark; rpDark.bank = 0;   // the blacked-out sensor band, the lens, the fans
    double headGlow = d.lights ? 0.25 : 0.0;
    auto shadeN = [&](const Vec3& n, double tint) {
        double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
        double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp;
        light = std::max(light, 0.16 + headGlow * std::max(0.0, dot(n, fwdT) + 0.5));
        double s = 48 * std::pow(light, 0.6) * tint;
        return s + (63 - s) * fog;
    };
    auto poly = [&](const Vec3* pts, int n, const Vec3& nrm, double tint, RasterParams& rp) {
        double shade = shadeN(nrm, tint);
        RVert q[8];
        for (int k = 0; k < n; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); if (v.z < NEAR_Z) return; q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; q[k].u = pts[k].x + pts[k].y; q[k].v = pts[k].z; }
        rasterPolygon(fb, q, n, rp, proj);
    };
    auto quad = [&](Vec3 a, Vec3 bq, Vec3 c, Vec3 dq, const Vec3& n, double tint, RasterParams& rp) { Vec3 p4[4] = {a, bq, c, dq}; poly(p4, 4, n, tint, rp); };
    auto tube = [&](const Vec3& a, const Vec3& bq, double radiusM, int bank, double shade) {
        RVert va, vb; Vec3 v0 = toView(a.x, a.y, a.z), v1 = toView(bq.x, bq.y, bq.z);
        if (v0.z < NEAR_Z || v1.z < NEAR_Z) return;
        double dz = std::min(v0.z, v1.z);
        int th = clampi((int)std::lround(2 * radiusM / dz * proj.f / FB_SCALE), 1, 5);
        va.x = v0.x; va.y = v0.y; va.z = v0.z; va.shade = shade; vb.x = v1.x; vb.y = v1.y; vb.z = v1.z; vb.shade = shade;
        rasterLine3(fb, va, vb, bank, proj, true, th);
    };
    auto point = [&](const Vec3& pp, int bank, double shade, int size) {
        Vec3 v = toView(pp.x, pp.y, pp.z);
        if (v.z > NEAR_Z) { RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = shade; rasterPoint3(fb, q, bank, proj, true, size); }
    };
    Vec3 up(0, 1, 0);
    double chrome = shadeN(up, 1.15), chromeDark = shadeN(-up, 0.8);
    // ---- the hull: nose, hood, sensor band, roof, rear slope, tail, the flanks (the buggy's wedge, shorter) and the belly
    quad(at(NOSE_F, -NOSE_W, NOSE_B), at(NOSE_F, NOSE_W, NOSE_B), at(NOSE_F, NOSE_W, NOSE_T), at(NOSE_F, -NOSE_W, NOSE_T), fwdT, 0.85, rpBody);
    quad(at(NOSE_F, -NOSE_W, NOSE_T), at(NOSE_F, NOSE_W, NOSE_T), at(SHO_F, SHO_W, BELT), at(SHO_F, -SHO_W, BELT), normalize(upT * 0.96 + fwdT * 0.28), 1.0, rpBody);
    quad(at(SHO_F, -SHO_W, BELT), at(SHO_F, SHO_W, BELT), at(CABF_F, CAB_W, ROOF), at(CABF_F, -CAB_W, ROOF), normalize(upT * 0.65 + fwdT * 0.76), 0.3, rpDark);   // no windows: the front sensor band
    quad(at(CABF_F, -CAB_W, ROOF), at(CABF_F, CAB_W, ROOF), at(CABR_F, CAB_W, ROOF), at(CABR_F, -CAB_W, ROOF), upT, 1.0, rpBody);
    quad(at(CABR_F, -CAB_W, ROOF), at(CABR_F, CAB_W, ROOF), at(TAIL_F, TAIL_W, TAIL_T), at(TAIL_F, -TAIL_W, TAIL_T), normalize(upT * 0.85 - fwdT * 0.53), 0.9, rpBody);
    quad(at(TAIL_F, -TAIL_W, TAIL_B), at(TAIL_F, TAIL_W, TAIL_B), at(TAIL_F, TAIL_W, TAIL_T), at(TAIL_F, -TAIL_W, TAIL_T), -fwdT, 0.7, rpBody);
    quad(at(NOSE_F, -NOSE_W, NOSE_B), at(SHO_F, -SHO_W, FLOOR), at(SHO_F, SHO_W, FLOOR), at(NOSE_F, NOSE_W, NOSE_B), -upT, 0.6, rpBody);   // the belly, seen from below in flight
    quad(at(SHO_F, -SHO_W, FLOOR), at(TAIL_F, -TAIL_W, TAIL_B), at(TAIL_F, TAIL_W, TAIL_B), at(SHO_F, SHO_W, FLOOR), -upT, 0.6, rpBody);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 nS = sideT * s;
        quad(at(NOSE_F, s * NOSE_W, NOSE_B), at(SHO_F, s * SHO_W, FLOOR), at(SHO_F, s * SHO_W, BELT), at(NOSE_F, s * NOSE_W, NOSE_T), normalize(nS * 0.8 + fwdT * 0.6), 0.9, rpBody);   // nose flank
        quad(at(SHO_F, s * SHO_W, FLOOR), at(TAIL_F, s * TAIL_W, TAIL_B), at(TAIL_F, s * TAIL_W, BELT), at(SHO_F, s * SHO_W, BELT), nS, 0.92, rpBody);                             // the belt
        Vec3 upper[5] = {at(SHO_F, s * SHO_W, BELT), at(TAIL_F, s * TAIL_W, BELT), at(TAIL_F, s * TAIL_W, TAIL_T), at(CABR_F, s * CAB_W, ROOF), at(CABF_F, s * CAB_W, ROOF)};
        poly(upper, 5, nS, 0.95, rpBody);                                                                                                                                                 // the cabin side
        Vec3 off = nS * 0.012;
        quad(at(SHO_F - 0.05, s * SHO_W, 1.0) + off, at(CABR_F - 0.05, s * CAB_W, 1.0) + off, at(CABR_F - 0.05, s * CAB_W, 1.24) + off, at(SHO_F - 0.05, s * SHO_W, 1.24) + off, nS, 0.3, rpDark);   // the sensor band round the cabin
        tube(at(NOSE_F, s * NOSE_W, NOSE_T), at(SHO_F, s * SHO_W, BELT), 0.02, 7, chrome * 0.9);   // chrome trim: the shoulder line and the roof edge
        tube(at(SHO_F, s * SHO_W, BELT), at(TAIL_F, s * TAIL_W, BELT), 0.02, 7, chrome * 0.9);
        tube(at(CABF_F, s * CAB_W, ROOF), at(CABR_F, s * CAB_W, ROOF), 0.02, 7, chrome * 0.85);
        // ---- the skids: a runner with an upturned tip and two struts up to the belly
        tube(at(-1.0, s * SKID_S, 0.0), at(0.95, s * SKID_S, 0.0), 0.04, 7, chromeDark);
        tube(at(0.95, s * SKID_S, 0.0), at(1.15, s * SKID_S, 0.14), 0.035, 7, chromeDark);
        tube(at(-0.6, s * SKID_S, 0.0), at(-0.5, s * 0.5, FLOOR + 0.02), 0.03, 7, chromeDark);
        tube(at(0.6, s * SKID_S, 0.0), at(0.5, s * 0.5, FLOOR + 0.02), 0.03, 7, chromeDark);
    }
    // ---- the four thrust pods on arms from the belt: a duct (a band of twelve facets), the fan inside (a dark disc, three chrome
    // blades turning while the spin is slow, a blur of a lighter disc when it is fast), the hub, and a glow under each in flight
    double ext = 0.45 + 0.55 * d.unfold;   // the arms unfold outward
    for (int k = 0; k < 4; k++) {
        double fx = (k & 1) ? POD_F : -POD_F, sx = (k & 2) ? -POD_S : POD_S;
        Vec3 c = at(fx * ext, sx * ext, POD_U);
        tube(at(fx * 0.5, sx * 0.45, BELT - 0.05), c, 0.05, 7, chromeDark);
        for (int i = 0; i < 12; i++) {
            double a0 = i * TAU / 12, a1 = (i + 1) * TAU / 12, am = (a0 + a1) * 0.5;
            Vec3 r0 = fwdT * (std::cos(a0) * POD_R * sc) + sideT * (std::sin(a0) * POD_R * sc), r1 = fwdT * (std::cos(a1) * POD_R * sc) + sideT * (std::sin(a1) * POD_R * sc);
            Vec3 n = normalize(fwdT * std::cos(am) + sideT * std::sin(am));
            quad(c + r0 - upT * (0.11 * sc), c + r1 - upT * (0.11 * sc), c + r1 + upT * (0.11 * sc), c + r0 + upT * (0.11 * sc), n, 0.9, rpBody);
        }
        RVert disc[12]; bool ok = true;
        for (int i = 0; i < 12; i++) {
            double a = i * TAU / 12;
            Vec3 pp = c + fwdT * (std::cos(a) * POD_R * 0.9 * sc) + sideT * (std::sin(a) * POD_R * 0.9 * sc);
            Vec3 v = toView(pp.x, pp.y, pp.z);
            disc[i].x = v.x; disc[i].y = v.y; disc[i].z = v.z; disc[i].shade = 7 + 5 * env.skyBrightness + 9 * d.rotor;
            if (v.z < NEAR_Z) ok = false;
        }
        if (ok) rasterPolygon(fb, disc, 12, rpDark, proj);
        if (ok && d.rotor < 0.75) {
            double spin = d.fanSpin * ((k & 1) ? 1 : -1);
            for (int b = 0; b < 3; b++) { double a = spin + b * TAU / 3; tube(c + upT * (0.02 * sc), c + upT * (0.02 * sc) + fwdT * (std::cos(a) * POD_R * 0.82 * sc) + sideT * (std::sin(a) * POD_R * 0.82 * sc), 0.03, 7, chrome * 0.8); }
        }
        point(c + upT * (0.03 * sc), 7, chrome * 0.7, dist < 10 ? 2 : 1);
        if (!d.landed || d.rotor > 0.5) point(c - upT * (0.2 * sc), 1, 42 + 20 * d.rotor, dist < 25 ? 2 : 1);
    }
    // ---- the camera pod on the nose: a box with a dark lens face, a bright lens dot and a red LED while it is live
    {
        double f0 = 1.07, f1 = 1.25, w = 0.13, u0 = NOSE_T, u1 = NOSE_T + 0.2;
        quad(at(f1, -w, u0), at(f1, w, u0), at(f1, w, u1), at(f1, -w, u1), fwdT, 0.25, rpDark);
        quad(at(f0, -w, u1), at(f1, -w, u1), at(f1, w, u1), at(f0, w, u1), upT, 0.95, rpBody);
        for (int s = -1; s <= 1; s += 2) quad(at(f0, s * w, u0), at(f1, s * w, u0), at(f1, s * w, u1), at(f0, s * w, u1), sideT * s, 0.85, rpBody);
        point(at(f1 + 0.01, 0, CAM_U), 7, 58, dist < 12 ? 2 : 1);
        if (inDrone) point(at(f1 + 0.01, w * 0.6, u1 - 0.04), 14, std::fmod(t, 1.0) < 0.5 ? 60 : 20, 1);
    }
    // ---- the lidar puck on the roof and the sensor mast at the rear with its blinking light
    {
        RVert puck[6]; bool ok = true;
        for (int k = 0; k < 6; k++) { double a = k * TAU / 6; Vec3 pp = at(-0.35 + std::cos(a) * 0.14, std::sin(a) * 0.14, ROOF + 0.05); Vec3 v = toView(pp.x, pp.y, pp.z); puck[k].x = v.x; puck[k].y = v.y; puck[k].z = v.z; puck[k].shade = chrome; if (v.z < NEAR_Z) ok = false; }
        RasterParams rd; rd.bank = 7;
        if (ok) rasterPolygon(fb, puck, 6, rd, proj);
        Vec3 ant0 = at(-0.9, 0.5, ROOF - 0.05), ant1 = at(-0.95, 0.54, ROOF + 0.6 * d.unfold);
        tube(ant0, ant1, 0.015, 7, chrome * 0.9);
        point(ant1, 14, std::fmod(t, 1.0) < 0.5 ? 60 : 24, dist < 40 ? 2 : 1);
    }
    // ---- lights: a headlight bar across the nose (bright when on), a tail light bar (red, brighter in a descent)
    {
        RasterParams rl; rl.bank = d.lights ? 1 : 7;
        double hs = d.lights ? 62 : chrome * 0.9;
        RVert q[4]; Vec3 pts[4] = {at(NOSE_F + 0.01, -0.3, 0.5), at(NOSE_F + 0.01, 0.3, 0.5), at(NOSE_F + 0.01, 0.3, 0.6), at(NOSE_F + 0.01, -0.3, 0.6)};
        bool ok = true;
        for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = hs; if (v.z < NEAR_Z) ok = false; }
        if (ok) rasterPolygon(fb, q, 4, rl, proj);
        RasterParams rt; rt.bank = 14;
        double ts = d.vy < -1 ? 58 : (d.lights ? 30 : 12);
        RVert tq[4]; Vec3 tp[4] = {at(TAIL_F - 0.01, -0.45, 0.56), at(TAIL_F - 0.01, 0.45, 0.56), at(TAIL_F - 0.01, 0.45, 0.68), at(TAIL_F - 0.01, -0.45, 0.68)};
        ok = true;
        for (int k = 0; k < 4; k++) { Vec3 v = toView(tp[k].x, tp[k].y, tp[k].z); tq[k].x = v.x; tq[k].y = v.y; tq[k].z = v.z; tq[k].shade = ts; if (v.z < NEAR_Z) ok = false; }
        if (ok) rasterPolygon(fb, tq, 4, rt, proj);
    }
}
