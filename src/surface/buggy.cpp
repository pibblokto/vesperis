// N4 (R-203) and N4-06 (R-204): the buggy. One closed model (a low wedge hull with a blacked-out sensor band round
// the cabin, wheel pods over four knobby wheels on suspension arms, a camera pod on the nose, light bars front and
// rear, a sensor mast with a blinking light and a lidar puck on the roof) drawn parked and from the chase camera.
// The driver sees the world through the nose camera: `SurfaceView::render` mounts the eye in the pod and `cameraFeed`
// gives the picture its CCTV look; the hull itself is never drawn from inside. Driving: a torque curve to 32 m/s,
// traction by material and gravity, handbrake skids, spray per wheel, tread tracks and three engine gears.
#include "surface_view.h"
#include "core/noise.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

namespace {
const double WHEEL_R = 0.42, WHEEL_X = 1.1, WHEEL_S = 0.85;   // wheel radius, longitudinal and lateral offsets of the hubs
const double TOP_SPEED = 50.0, REVERSE_SPEED = 8.0;   // 180 km/h since O1-05 (115 before)
// the hull's stations (metres along the length from the centre) and their half widths and heights
const double NOSE_F = 1.62, SHO_F = 1.0, CABF_F = 0.5, CABR_F = -0.8, TAIL_F = -1.55;
const double NOSE_W = 0.42, SHO_W = 0.86, CAB_W = 0.9, TAIL_W = 0.72;
const double FLOOR = 0.32, NOSE_B = 0.44, NOSE_T = 0.74, BELT = 0.92, ROOF = 1.5, TAIL_B = 0.4, TAIL_T = 1.02;
const double CAM_F = 1.58, CAM_U = 0.84;   // where the nose camera sits (the pod spans 1.44..1.62 x 0.74..0.94)
}

// traction of the wheels on a material (1 = rock), scaled by gravity (light worlds spin their wheels)
static double tractionOf(int mat, double g) {
    double tr = mat == MAT_ICE ? 0.35 : (mat == MAT_SNOW ? 0.6 : (mat == MAT_SAND ? 0.8 : (mat == MAT_GRASS || mat == MAT_FOREST ? 0.9 : (mat == MAT_DUST ? 0.85 : 1.0))));
    return tr * clampd(g / 9.8, 0.3, 1.0);
}

void SurfaceView::buggyFrame(Vec3& fwdT, Vec3& sideT, Vec3& upT) const {
    const Buggy& b = buggy;
    double hx = std::sin(b.heading), hz = std::cos(b.heading);
    Vec3 fwd(hx, 0, hz), side(-hz, 0, hx), up(0, 1, 0);
    fwdT = normalize(fwd + up * std::tan(b.pitch));
    sideT = normalize(side + up * std::tan(b.roll));
    upT = normalize(cross(sideT, fwdT));
    if (upT.y < 0) upT = -upT;
}

// the eye of the driver: the nose camera's lens, in local metres (the pod rides the hull's pitch and roll)
Vec3 SurfaceView::buggyCameraMount() const {
    Vec3 fwdT, sideT, upT;
    buggyFrame(fwdT, sideT, upT);
    return Vec3(buggy.x, buggy.y, buggy.z) + fwdT * CAM_F + upT * CAM_U;
}

void SurfaceView::updateBuggy(double dt, const Input& in, double t) {
    Buggy& b = buggy;
    Player& p = player;
    double g = site.gravity;
    p.yaw += in.mouseDx * 0.0032 * mouseSens;
    p.pitch -= in.mouseDy * 0.0032 * mouseSens * (invertY ? -1 : 1);
    // R-204: the nose camera is bolted to the hull and pans 70 deg each way, tilts 35 down and 25 up: no looking back
    double look = clampd(wrapAngle(p.yaw - b.heading), -CAM_PAN, CAM_PAN);
    p.pitch = clampd(p.pitch, -CAM_TILT_DOWN, CAM_TILT_UP);
    double throttle = (in.isDown(KEY_W) || in.isDown(KEY_UP) ? 1 : 0) - (in.isDown(KEY_S) || in.isDown(KEY_DOWN) ? 1 : 0);
    double steerIn = (in.isDown(KEY_D) || in.isDown(KEY_RIGHT) ? 1 : 0) - (in.isDown(KEY_A) || in.isDown(KEY_LEFT) ? 1 : 0);
    if (in.moveY != 0 || in.moveX != 0) { throttle = in.moveY; steerIn = in.moveX; }   // M6-02
    if (in.ctrl()) { throttle = 0; steerIn = 0; }
    // a thump for the synth and a dip of the camera (the dip decays smoothly: no random shake, B-203)
    auto thump = [&](double v) { b.thud = std::max(b.thud, v); b.jolt = std::max(b.jolt, v); };
    // steering lock shrinks with speed (30 deg at rest, 9 deg at the top)
    double lock = (30 - 21 * clampd(std::fabs(b.speed) / TOP_SPEED, 0, 1)) * DEG;
    b.steer += (steerIn * lock - b.steer) * (1 - std::exp(-dt * 6));
    int mat = site.lod0.at((int)std::floor(b.x / 16), (int)std::floor(b.z / 16)).material;
    double rollRes = mat == MAT_SAND ? 1.6 : (mat == MAT_SNOW ? 1.5 : (mat == MAT_DUST ? 1.1 : (mat == MAT_ICE ? 0.5 : 1.0)));
    double traction = tractionOf(mat, g);
    double hx = std::sin(b.heading), hz = std::cos(b.heading);
    double v = b.speed, av = std::fabs(v);
    // N4-02 torque curve, retuned for 50 m/s (O1-05): 6 m/s^2 to 18 m/s, tapering to 3.4 at the top (the drag there is
    // 2.75 plus the rolling resistance, so the last metres per second come slowly); reverse 3; against the motion x2.5
    double accel = 0;
    if (throttle > 0) accel = av < 18 ? 6.0 : 6.0 - 2.6 * clampd((av - 18) / 32.0, 0, 1);
    else if (throttle < 0) accel = -3.0;
    if (v * throttle < 0) accel *= 2.5;
    accel *= (b.airborne ? 0.0 : (0.55 + 0.45 * traction));
    double drag = 0.0011 * v * av + rollRes * 0.3 * (v > 0 ? 1 : (v < 0 ? -1 : 0));
    double hAhead = site.groundHeight(b.x + hx * 1.5, b.z + hz * 1.5), hBehind = site.groundHeight(b.x - hx * 1.5, b.z - hz * 1.5);
    double slopeAng = std::atan((hAhead - hBehind) / 3.0);
    if (!b.airborne) accel -= g * std::sin(slopeAng) * 0.8;
    if (slopeAng > 30 * DEG && v > 0) accel -= 3.0;   // stall on steep climbs
    b.speed += (accel - drag) * dt;
    bool handbrake = in.isDown(KEY_SPACE);
    if (handbrake) b.speed *= std::exp(-dt * 3.0);
    b.brake = (v * throttle < 0 && av > 0.5) || handbrake;
    b.speed = clampd(b.speed, -REVERSE_SPEED, TOP_SPEED);
    if (std::fabs(b.speed) < 0.02 && throttle == 0) b.speed = 0;
    // turning: a bicycle model; low traction and the handbrake let the rear slide
    double turn = b.speed * std::tan(b.steer) / 2.2;
    double grip = handbrake ? traction * 0.35 : traction;
    b.heading = wrap2pi(b.heading + turn * dt * (0.4 + 0.6 * grip));
    p.yaw = wrap2pi(b.heading + look);   // the camera turns with the hull, keeping its pan
    double lateral = turn * b.speed * 0.25 * (1 - grip);   // m/s of sideways slide
    b.skid += (clampd(std::fabs(lateral) / 3.0 + (handbrake && av > 4 ? 0.6 : 0), 0, 1) - b.skid) * (1 - std::exp(-dt * 4));
    double nx = b.x + hx * b.speed * dt - hz * lateral * dt, nz = b.z + hz * b.speed * dt + hx * lateral * dt;
    // rocks, trunks, logs and water stop it: collisions cost 60% of the speed and push back
    {
        static std::vector<Collider> cols;
        collectColliders(nx, nz, cols);
        for (const Collider& c : cols) {
            double dx = nx - c.x, dz = nz - c.z, rr = c.r + 1.0;
            double d2 = dx * dx + dz * dz;
            if (d2 < rr * rr && d2 > 1e-9) {
                if (c.kind == 0 && c.r < 0.72) {   // a small rock (under 0.9 m): a bump, the wheels ride over it
                    if (b.bumpT <= 0) { b.speed *= 0.88; b.vibration += 0.35; thump(std::min(0.6, std::fabs(b.speed) / 30.0)); b.bumpT = 0.6; }
                    continue;
                }
                double d = std::sqrt(d2);
                nx = c.x + dx / d * rr; nz = c.z + dz / d * rr;
                if (std::fabs(b.speed) > 2) thump(std::min(1.0, std::fabs(b.speed) / 15.0));
                b.speed = -b.speed * 0.4;
                b.vibration += 0.5;
            }
        }
        b.bumpT -= dt;
        if (site.groundHeight(nx + hx * 1.2, nz + hz * 1.2) < site.waterAt(nx + hx * 1.2, nz + hz * 1.2) - 0.4) { b.speed = -b.speed * 0.2; nx = b.x; nz = b.z; }
    }
    b.x = nx; b.z = nz;
    // wheels and chassis: four contact heights, the chassis follows their mean, the wheels keep their travel
    double px = -hz, pz = hx;
    double hw[4];
    for (int w = 0; w < 4; w++) {
        double fx = (w & 1) ? WHEEL_X : -WHEEL_X, sx = (w & 2) ? -WHEEL_S : WHEEL_S;
        hw[w] = site.groundHeight(b.x + hx * fx + px * sx, b.z + hz * fx + pz * sx);
    }
    // B-313: the hull rides on the suspension: its reference height is the ground averaged over 16 m along the way (five
    // samples 4 m apart), kept within the wheels' travel of the contact mean, so the 4 m relief under a fast buggy is
    // soaked up by the wheels instead of throwing the hull (and the camera) off every bump; on a straight slope the
    // average is the ground itself, so the descent rule of B-307 sees the same ground it did
    double meanW = 0.25 * (hw[0] + hw[1] + hw[2] + hw[3]);
    double smooth = 0;
    for (int k = -2; k <= 2; k++) smooth += site.groundHeight(b.x + hx * (k * 4.0), b.z + hz * (k * 4.0));
    smooth *= 0.2;
    double chassisY = std::max(clampd(smooth, meanW - 0.3, meanW + 0.3), site.waterAt(b.x, b.z));
    // B-307: the ground contact's vertical speed along the way (the previous frame's contact height is kept, so it is
    // known in the air too, and a landing is judged by the speed relative to the slope, not the absolute fall)
    double vyGround = b.groundY > -1e8 ? (chassisY - b.groundY) / std::max(dt, 1e-4) : 0.0;
    b.groundY = chassisY;
    if (b.airborne) {
        b.vy -= g * dt;
        b.y += b.vy * dt;
        b.airT += dt;
        if (b.y <= chassisY) {
            double rel = std::max(0.0, vyGround - b.vy);   // the impact: how much faster than the ground it came down
            b.y = chassisY; b.airborne = false; b.vy = vyGround;
            b.vibration += std::min(1.0, rel * 0.06);
            b.speed *= 1 - 0.15 * clampd(rel / 6.0, 0, 1);
            if (rel > 1.0) thump(std::min(1.0, rel * 0.15));
        }
    } else {
        // the wheels leave the ground only when it falls away faster than gravity and the suspension can follow: the
        // vertical acceleration the ground asks for, (vyGround - vy) / dt, beyond four g and at least 40 m/s^2, at
        // speed. A steady descent asks for none, so the buggy rolls downhill; it used to launch, horizontally, as soon
        // as the ground dropped faster than 3 m/s (a 30% grade above 10 m/s), fall back with a thump and 15% of its
        // speed gone, and launch again: the descent became a ladder
        double needed = (vyGround - b.vy) / std::max(dt, 1e-4);
        double aFollow = std::max(4.0 * g, 40.0);
        if (needed < -aFollow && std::fabs(b.speed) > 6) { b.airborne = true; b.airT = 0; b.y += b.vy * dt; }   // vy kept: a crest throws it up, a lip lets it drop
        else { b.y = chassisY; b.vy = vyGround; }
    }
    for (int w = 0; w < 4; w++) {
        double target = b.airborne ? -0.25 : clampd(hw[w] - b.y, -0.35, 0.35);   // suspension travel relative to the chassis
        b.wheelY[w] += (target - b.wheelY[w]) * (1 - std::exp(-dt * 12));
    }
    double targetPitch = std::atan(((hw[1] + hw[3]) - (hw[0] + hw[2])) * 0.5 / 2.2), targetRoll = std::atan(((hw[0] + hw[1]) - (hw[2] + hw[3])) * 0.5 / 1.6);
    b.pitch += (targetPitch - b.pitch) * (1 - std::exp(-dt * 5));
    b.roll += (targetRoll - b.roll) * (1 - std::exp(-dt * 5));
    double rough = std::fabs(site.groundHeight(b.x + hx, b.z + hz) - site.groundHeight(b.x, b.z));
    b.vibration += std::fabs(b.speed) * 0.0025 * (1 + 4 * rough) * dt * 60;
    b.vibration *= std::exp(-dt * 4);
    b.jolt *= std::exp(-dt * 5);
    b.wheelSpin += b.speed * dt / WHEEL_R;
    b.odometer += std::fabs(b.speed) * dt;
    b.topSpeed = std::max(b.topSpeed, std::fabs(b.speed));
    b.lights = env.sun.dirLocal.y < 0.08;
    b.gear = av < 12 ? 0 : (av < 28 ? 1 : 2);   // O1-05: 0-12, 12-28, 28-50 m/s
    // dust settles on the body on dry dusty ground, rain washes it
    if (mat == MAT_SAND || mat == MAT_DUST) b.dust = std::min(1.0, b.dust + av * dt * 0.0004);
    if (env.rain > 0.2) b.dust = std::max(0.0, b.dust - dt * 0.05);
    // tracks (both rear wheels, tread width 0.3 m) and spray per wheel by material
    b.trackAccum += std::fabs(b.speed) * dt;
    if (b.trackAccum > 0.5 && !b.airborne) {
        b.trackAccum = 0;
        Buggy::Track l{(float)(b.x - hx * WHEEL_X + px * WHEEL_S), (float)(b.z - hz * WHEEL_X + pz * WHEEL_S)}, r{(float)(b.x - hx * WHEEL_X - px * WHEEL_S), (float)(b.z - hz * WHEEL_X - pz * WHEEL_S)};
        for (const Buggy::Track& tr : {l, r}) {
            if (b.tracks.size() < 400) b.tracks.push_back(tr); else { b.tracks[b.trackHead] = tr; b.trackHead = (b.trackHead + 1) % 400; }
        }
        int kind = (mat == MAT_SAND || mat == MAT_DUST) ? 0 : (mat == MAT_SNOW ? 1 : ((mat == MAT_GRASS && env.rain > 0.1) || (mat == MAT_GRASS && site.waterAt(b.x, b.z) > -1e8) ? 2 : (mat == MAT_GRASS ? 0 : -1)));
        if (av > 3 && kind >= 0) {
            Rng rr((uint64_t)(t * 1000) ^ 0xD05);
            int n = 1 + (int)(av / 10);
            for (int w = 0; w < 4; w++) {
                if (w & 1) continue;   // the rear wheels throw most of it
                double sx = (w & 2) ? -WHEEL_S : WHEEL_S;
                for (int i = 0; i < n; i++) b.puffs.push_back({(float)(b.x - hx * WHEEL_X + px * sx + rr.sym(0.3)), (float)(b.y + 0.2), (float)(b.z - hz * WHEEL_X + pz * sx + rr.sym(0.3)), 0.f, (uint8_t)kind});
            }
        }
    }
    p.x = b.x; p.z = b.z; p.y = b.y;
    p.vx = hx * b.speed; p.vz = hz * b.speed; p.vy = 0; p.onGround = true; p.swimming = false; p.underwater = false;
    p.altAboveGround = 0;
    p.stamina = clampd(p.stamina + dt * 16, 0, 100);
    p.bobY *= std::exp(-dt * 6); p.bobX *= std::exp(-dt * 6); p.landDip *= std::exp(-dt * 5);
    p.fovKick = 0; p.sprintRamp = 0; p.sprinting = false;
}

// open ground for a drive: a straight run walked every 8 m, clear of forest, water, heavy vegetation and any hard
// collider (trunks, logs, ruins, rocks over 0.9 m) within 3 m of the line; the longest such run within 800 m
double SurfaceView::findOpenRun(double& ox, double& oz, double& heading) {
    std::vector<Collider> cols;
    auto runFrom = [&](double x, double z, double ha) {
        double sx = std::sin(ha), cz = std::cos(ha);
        for (double d = 0; d <= 900; d += 8) {
            double px = x + sx * d, pz = z + cz * d;
            TerrainVertex tv = site.sampleAt(px, pz, 16);
            if (tv.material == MAT_FOREST || tv.material == MAT_WATER || tv.water > -1e8f || tv.veg > 0.55) return d;
            collectColliders(px, pz, cols);
            for (const Collider& c : cols) {
                if (c.kind == 0 && c.r < 0.72) continue;
                double dx = c.x - px, dz = c.z - pz;
                double along = dx * sx + dz * cz, across = -dx * cz + dz * sx;
                if (std::fabs(along) < 9 && std::fabs(across) < 3 + c.r) return d;
            }
        }
        return 900.0;
    };
    double bestRun = 0; ox = player.x; oz = player.z; heading = player.yaw;
    for (double r = 0; r <= 800 && bestRun < 700; r += 60)
        for (int k = 0; k < (r > 0 ? 12 : 1); k++) {
            double a = k * TAU / 12, x = player.x + std::cos(a) * r, z = player.z + std::sin(a) * r;
            if (x * x + z * z > 1500.0 * 1500.0) continue;
            for (int h = 0; h < 12; h++) {
                double ha = h * TAU / 12;
                double run = runFrom(x, z, ha);
                if (run > bestRun) { bestRun = run; ox = x; oz = z; heading = ha; }
            }
            if (bestRun >= 700) break;
        }
    return bestRun;
}

// R-204: the nose camera's picture. Lifted blacks and half the saturation with a cold cast (palette), then
// scanlines, a vignette, sensor noise and a slow rolling band on the intensities. Applied after the mush so the
// lines and the grain stay crisp, before the RGB conversion, like the vision modes.
void SurfaceView::cameraFeed(Framebuffer& fb, double t) {
    for (int i = 0; i < BANKS * 64; i++) {
        uint8_t* c = fb.pal + i * 3;
        double lum = 0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2];
        double r = lum + (c[0] - lum) * 0.45, g = lum + (c[1] - lum) * 0.45, bl = lum + (c[2] - lum) * 0.45;
        c[0] = (uint8_t)clampd(16 + r * 0.86, 0, 255); c[1] = (uint8_t)clampd(16 + g * 0.93, 0, 255); c[2] = (uint8_t)clampd(16 + bl * 0.98, 0, 255);
    }
    int S = FB_SCALE;
    static std::vector<float> vx, vy;
    if ((int)vx.size() != FBW) { vx.resize(FBW); for (int x = 0; x < FBW; x++) { double u = (x + 0.5) / FBW * 2 - 1; vx[x] = (float)(u * u); } }
    if ((int)vy.size() != FBH) { vy.resize(FBH); for (int y = 0; y < FBH; y++) { double u = (y + 0.5) / FBH * 2 - 1; vy[y] = (float)(u * u); } }
    int bandY = (int)(std::fmod(t * 0.22, 1.0) * (FBH + 40 * S)) - 20 * S;   // rolls down the picture every 4.5 s
    uint32_t rs = (uint32_t)(t * 60) * 2654435761u + 0x9E37u;
    for (int y = 0; y < FBH; y++) {
        double line = ((y / S) & 1) ? 0.82 : 1.0;
        double band = std::abs(y - bandY) < 10 * S ? 1.05 : 1.0;
        Pix* row = &fb.idx[(size_t)y * FBW];
        for (int x = 0; x < FBW; x++) {
            rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
            double vig = 1 - 0.45 * std::max(0.0, (double)(vx[x] + vy[y]) - 0.45);
            int inten = (int)(intenOf(row[x]) * line * band * vig) + (int)(rs % 97) - 48;   // +-1.5 shades of noise (32 units per shade)
            row[x] = pixI(bankOf(row[x]), inten);
        }
    }
}

void SurfaceView::drawBuggy(Framebuffer& fb, double t) {
    const Buggy& b = buggy;
    if (inBuggy && !chaseCam) return;   // from inside you see the nose camera's picture, never the hull (R-204)
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    double dist = std::sqrt((b.x - camPos.x) * (b.x - camPos.x) + (b.z - camPos.z) * (b.z - camPos.z));
    // tracks (tread: alternate darkening) and spray first, they lie on the ground
    {
        RasterParams rt; rt.blend = BLEND_DARKEN; rt.zwrite = false;
        int k = 0;
        for (const Buggy::Track& tr : b.tracks) {
            double td = std::sqrt((tr.x - camPos.x) * (tr.x - camPos.x) + (tr.z - camPos.z) * (tr.z - camPos.z));
            if (td > 70) continue;
            rt.darken = (k++ & 1) ? 0.72 : 0.86;
            RVert q[4];
            double ax[4] = {-0.15, 0.15, 0.15, -0.15}, az[4] = {-0.25, -0.25, 0.25, 0.25};
            for (int m = 0; m < 4; m++) { Vec3 v = toView(tr.x + ax[m], site.groundHeight(tr.x + ax[m], tr.z + az[m]) + 0.06, tr.z + az[m]); q[m].x = v.x; q[m].y = v.y; q[m].z = v.z; q[m].shade = 0; }
            rasterPolygon(fb, q, 4, rt, proj);
        }
        for (const Buggy::Puff& pf : b.puffs) {
            double a = 1 - pf.age / 1.5;
            Vec3 v = toView(pf.x, pf.y + pf.age * (pf.kind == 2 ? 0.9 : 0.6), pf.z);
            if (v.z < NEAR_Z) continue;
            RVert q; q.x = v.x; q.y = v.y; q.z = v.z;
            int bank = pf.kind == 1 ? 8 : (pf.kind == 2 ? 0 : 9);
            q.shade = pf.kind == 2 ? 10 + 8 * a : 30 + 14 * env.skyBrightness * a;
            rasterPoint3(fb, q, bank, proj, true, a > 0.5 ? 2 : 1);
        }
    }
    double sc = 0.3 + 0.7 * b.unfold;   // the hull grows out of its folded state
    if (b.unfold < 1) drawBlobShadow(fb, b.x, b.z, 1.2 * b.unfold, 0.6); else drawBlobShadow(fb, b.x, b.z, 1.5, 0.8);
    Vec3 fwdT, sideT, upT;
    buggyFrame(fwdT, sideT, upT);
    Vec3 base(b.x, b.y, b.z);
    auto at = [&](double f, double s, double u) { return base + fwdT * (f * sc) + sideT * (s * sc) + upT * (u * sc); };
    double fog = 1 - std::exp(-dist / env.fogDistance);
    double dirt = 1 - 0.25 * b.dust;
    RasterParams rpBody; rpBody.bank = 6; rpBody.grain = &grain; rpBody.grainScale = 3.0 * FB_SCALE;
    RasterParams rpDark; rpDark.bank = 0;   // the blacked-out sensor band, the lens, the rubber
    double headGlow = b.lights ? 0.25 : 0.0;   // the headlight bar spills onto the hood
    auto shadeN = [&](const Vec3& n, double tint) {
        double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
        double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp;
        light = std::max(light, 0.16 + headGlow * std::max(0.0, dot(n, fwdT) + 0.5));   // never black: the mast light glows on it
        double s = 48 * std::pow(light, 0.6) * tint;
        return s + (63 - s) * fog;
    };
    auto poly = [&](const Vec3* pts, int n, const Vec3& nrm, double tint, RasterParams& rp) {
        double shade = shadeN(nrm, tint);
        RVert q[8];
        for (int k = 0; k < n; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); if (v.z < NEAR_Z) return; q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; q[k].u = pts[k].x + pts[k].y; q[k].v = pts[k].z; }
        rasterPolygon(fb, q, n, rp, proj);
    };
    auto quad = [&](Vec3 a, Vec3 bq, Vec3 c, Vec3 d, const Vec3& n, double tint, RasterParams& rp) { Vec3 p4[4] = {a, bq, c, d}; poly(p4, 4, n, tint, rp); };
    auto tube = [&](const Vec3& a, const Vec3& bq, double radiusM, int bank, double shade) {
        RVert va, vb; Vec3 v0 = toView(a.x, a.y, a.z), v1 = toView(bq.x, bq.y, bq.z);
        if (v0.z < NEAR_Z || v1.z < NEAR_Z) return;
        double dz = std::min(v0.z, v1.z);
        int th = clampi((int)std::lround(2 * radiusM / dz * proj.f / FB_SCALE), 1, 5);
        va.x = v0.x; va.y = v0.y; va.z = v0.z; va.shade = shade; vb.x = v1.x; vb.y = v1.y; vb.z = v1.z; vb.shade = shade;
        rasterLine3(fb, va, vb, bank, proj, true, th);
    };
    Vec3 up(0, 1, 0);
    double chrome = shadeN(up, 1.15), chromeDark = shadeN(-up, 0.8);
    // ---- the hull: nose, hood, sensor band, roof, rear slope, tail
    quad(at(NOSE_F, -NOSE_W, NOSE_B), at(NOSE_F, NOSE_W, NOSE_B), at(NOSE_F, NOSE_W, NOSE_T), at(NOSE_F, -NOSE_W, NOSE_T), fwdT, 0.85 * dirt, rpBody);
    quad(at(NOSE_F, -NOSE_W, NOSE_T), at(NOSE_F, NOSE_W, NOSE_T), at(SHO_F, SHO_W, BELT), at(SHO_F, -SHO_W, BELT), normalize(upT * 0.96 + fwdT * 0.28), 1.0 * dirt, rpBody);
    quad(at(SHO_F, -SHO_W, BELT), at(SHO_F, SHO_W, BELT), at(CABF_F, CAB_W, ROOF), at(CABF_F, -CAB_W, ROOF), normalize(upT * 0.65 + fwdT * 0.76), 0.3, rpDark);   // no windows: the front sensor band
    quad(at(CABF_F, -CAB_W, ROOF), at(CABF_F, CAB_W, ROOF), at(CABR_F, CAB_W, ROOF), at(CABR_F, -CAB_W, ROOF), upT, 1.0 * dirt, rpBody);
    quad(at(CABR_F, -CAB_W, ROOF), at(CABR_F, CAB_W, ROOF), at(TAIL_F, TAIL_W, TAIL_T), at(TAIL_F, -TAIL_W, TAIL_T), normalize(upT * 0.85 - fwdT * 0.53), 0.9 * dirt, rpBody);
    quad(at(TAIL_F, -TAIL_W, TAIL_B), at(TAIL_F, TAIL_W, TAIL_B), at(TAIL_F, TAIL_W, TAIL_T), at(TAIL_F, -TAIL_W, TAIL_T), -fwdT, 0.7 * dirt, rpBody);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 nS = sideT * s;
        quad(at(NOSE_F, s * NOSE_W, NOSE_B), at(SHO_F, s * SHO_W, FLOOR), at(SHO_F, s * SHO_W, BELT), at(NOSE_F, s * NOSE_W, NOSE_T), normalize(nS * 0.8 + fwdT * 0.6), 0.9 * dirt, rpBody);   // nose flank
        quad(at(SHO_F, s * SHO_W, FLOOR), at(TAIL_F, s * TAIL_W, TAIL_B), at(TAIL_F, s * TAIL_W, BELT), at(SHO_F, s * SHO_W, BELT), nS, 0.92 * dirt, rpBody);                             // the belt
        Vec3 upper[5] = {at(SHO_F, s * SHO_W, BELT), at(TAIL_F, s * TAIL_W, BELT), at(TAIL_F, s * TAIL_W, TAIL_T), at(CABR_F, s * CAB_W, ROOF), at(CABF_F, s * CAB_W, ROOF)};
        poly(upper, 5, nS, 0.95 * dirt, rpBody);                                                                                                                                                 // the cabin side
        Vec3 off = nS * 0.012;
        quad(at(SHO_F - 0.05, s * SHO_W, 1.02) + off, at(CABR_F - 0.05, s * CAB_W, 1.02) + off, at(CABR_F - 0.05, s * CAB_W, 1.32) + off, at(SHO_F - 0.05, s * SHO_W, 1.32) + off, nS, 0.3, rpDark);   // the sensor band round the cabin
        // chrome trim: the shoulder line from the nose to the tail and the roof edge
        tube(at(NOSE_F, s * NOSE_W, NOSE_T), at(SHO_F, s * SHO_W, BELT), 0.02, 7, chrome * 0.9);
        tube(at(SHO_F, s * SHO_W, BELT), at(TAIL_F, s * TAIL_W, BELT), 0.02, 7, chrome * 0.9);
        tube(at(CABF_F, s * CAB_W, ROOF), at(CABR_F, s * CAB_W, ROOF), 0.02, 7, chrome * 0.85);
        // wheel pods: five strips arcing over each wheel, from the hull's side out past the wheel's face
        for (int w = 0; w < 4; w++) {
            double fx = (w & 1) ? WHEEL_X : -WHEEL_X, sx = (w & 2) ? -WHEEL_S : WHEEL_S;
            if ((sx > 0) != (s > 0)) continue;
            double r = WHEEL_R + 0.16, wIn = s * 0.86, wOut = s * 1.06;
            for (int k = 0; k < 5; k++) {
                double a0 = PI * (0.08 + 0.84 * k / 5.0), a1 = PI * (0.08 + 0.84 * (k + 1) / 5.0);
                Vec3 p0 = at(fx + std::cos(a0) * r, wOut, 0.42 + std::sin(a0) * r), p1 = at(fx + std::cos(a1) * r, wOut, 0.42 + std::sin(a1) * r);
                Vec3 p0i = at(fx + std::cos(a0) * r, wIn, 0.42 + std::sin(a0) * r), p1i = at(fx + std::cos(a1) * r, wIn, 0.42 + std::sin(a1) * r);
                Vec3 n = normalize(upT * std::sin((a0 + a1) * 0.5) + fwdT * std::cos((a0 + a1) * 0.5));
                quad(p0i, p0, p1, p1i, n, 0.95 * dirt, rpBody);
            }
            // the pod's outer skirt, down to the hub height, closes the arch
            quad(at(fx - r, wOut, 0.42), at(fx + r, wOut, 0.42), at(fx + r, wOut, 0.42 + 0.14), at(fx - r, wOut, 0.42 + 0.14), nS, 0.85 * dirt, rpBody);
        }
    }
    // ---- wheels: 12-gons with a knobby rim (alternating tread shade), a hub and spokes, on suspension arms; the front pair steers
    for (int w = 0; w < 4; w++) {
        double fx = (w & 1) ? WHEEL_X : -WHEEL_X, sx = (w & 2) ? -WHEEL_S : WHEEL_S;
        double steer = (w & 1) ? b.steer : 0;
        Vec3 wf = fwdT * std::cos(steer) + sideT * std::sin(steer), ws = sideT * std::cos(steer) - fwdT * std::sin(steer);
        double drop = (1 - b.unfold) * 0.45;
        Vec3 hub = base + fwdT * (fx * sc) + sideT * (sx * sc) + upT * ((WHEEL_R + b.wheelY[w] + drop) * sc);
        double s = sx > 0 ? 1 : -1;
        for (int face = 0; face < 2; face++) {
            Vec3 c = hub + ws * (s * (face == 0 ? 0.16 : -0.14) * sc);
            RVert polyW[12];
            bool ok = true;
            for (int k = 0; k < 12; k++) {
                double a = k * TAU / 12 + b.wheelSpin;
                Vec3 pp = c + wf * (std::cos(a) * WHEEL_R * sc) + upT * (std::sin(a) * WHEEL_R * sc);
                Vec3 v = toView(pp.x, pp.y, pp.z);
                polyW[k].x = v.x; polyW[k].y = v.y; polyW[k].z = v.z; polyW[k].shade = (face == 0 ? 9 : 6) + 5 * env.skyBrightness;
                if (v.z < NEAR_Z) ok = false;
            }
            if (ok) rasterPolygon(fb, polyW, 12, rpDark, proj);
            if (ok && face == 0) {
                // the tread: knobs around the rim, and the hub with spokes
                for (int k = 0; k < 12; k++) {
                    double a0 = k * TAU / 12 + b.wheelSpin, a1 = (k + 0.5) * TAU / 12 + b.wheelSpin;
                    Vec3 p0 = c + wf * (std::cos(a0) * WHEEL_R * sc) + upT * (std::sin(a0) * WHEEL_R * sc), p1 = c + wf * (std::cos(a1) * WHEEL_R * sc) + upT * (std::sin(a1) * WHEEL_R * sc);
                    tube(p0, p1, 0.04, 0, 16 + 8 * env.skyBrightness);
                }
                RVert hubP[6]; bool okh = true;
                for (int k = 0; k < 6; k++) { double a = k * TAU / 6 + b.wheelSpin; Vec3 pp = c + ws * (s * 0.02) + wf * (std::cos(a) * WHEEL_R * 0.45 * sc) + upT * (std::sin(a) * WHEEL_R * 0.45 * sc); Vec3 v = toView(pp.x, pp.y, pp.z); hubP[k].x = v.x; hubP[k].y = v.y; hubP[k].z = v.z; hubP[k].shade = chrome * 0.7; if (v.z < NEAR_Z) okh = false; }
                RasterParams rh; rh.bank = 7;
                if (okh) rasterPolygon(fb, hubP, 6, rh, proj);
                for (int k = 0; k < 3; k++) {
                    double a = k * TAU / 3 + b.wheelSpin;
                    Vec3 p0 = c + ws * (s * 0.03) + wf * (std::cos(a) * WHEEL_R * 0.4 * sc) + upT * (std::sin(a) * WHEEL_R * 0.4 * sc), p1 = c + ws * (s * 0.03) - wf * (std::cos(a) * WHEEL_R * 0.4 * sc) - upT * (std::sin(a) * WHEEL_R * 0.4 * sc);
                    tube(p0, p1, 0.02, 7, chrome * 0.8);
                }
            }
        }
        // suspension arms from the underbody to the hub
        tube(at(fx * 0.55, sx * 0.55, FLOOR + 0.05), hub, 0.035, 7, chromeDark);
        tube(at(fx * 0.85, sx * 0.55, FLOOR + 0.25), hub + upT * (0.1 * sc), 0.03, 7, chromeDark);
    }
    // ---- the camera pod on the nose: a box with a dark lens face, a bright lens dot and a red LED while it is live
    {
        double f0 = 1.44, f1 = 1.62, w = 0.13, u0 = NOSE_T, u1 = NOSE_T + 0.2;
        quad(at(f1, -w, u0), at(f1, w, u0), at(f1, w, u1), at(f1, -w, u1), fwdT, 0.25, rpDark);   // the lens face
        quad(at(f0, -w, u1), at(f1, -w, u1), at(f1, w, u1), at(f0, w, u1), upT, 0.95 * dirt, rpBody);
        for (int s = -1; s <= 1; s += 2) quad(at(f0, s * w, u0), at(f1, s * w, u0), at(f1, s * w, u1), at(f0, s * w, u1), sideT * s, 0.85 * dirt, rpBody);
        Vec3 lens = at(f1 + 0.01, 0, CAM_U), led = at(f1 + 0.01, w * 0.6, u1 - 0.04);
        Vec3 vl = toView(lens.x, lens.y, lens.z), ve = toView(led.x, led.y, led.z);
        if (vl.z > NEAR_Z) { RVert q; q.x = vl.x; q.y = vl.y; q.z = vl.z; q.shade = 58; rasterPoint3(fb, q, 7, proj, true, dist < 12 ? 2 : 1); }
        if (ve.z > NEAR_Z && inBuggy) { RVert q; q.x = ve.x; q.y = ve.y; q.z = ve.z; q.shade = std::fmod(t, 1.0) < 0.5 ? 60 : 20; rasterPoint3(fb, q, 14, proj, true, 1); }
    }
    // ---- the lidar puck on the roof and the sensor mast at the rear with its blinking light
    {
        RVert puck[6]; bool ok = true;
        for (int k = 0; k < 6; k++) { double a = k * TAU / 6; Vec3 pp = at(-0.5 + std::cos(a) * 0.16, std::sin(a) * 0.16, ROOF + 0.06); Vec3 v = toView(pp.x, pp.y, pp.z); puck[k].x = v.x; puck[k].y = v.y; puck[k].z = v.z; puck[k].shade = chrome; if (v.z < NEAR_Z) ok = false; }
        RasterParams rd; rd.bank = 7;
        if (ok) rasterPolygon(fb, puck, 6, rd, proj);
        Vec3 ant0 = at(-1.15, 0.62, ROOF), ant1 = at(-1.2, 0.66, ROOF + 0.7 * b.unfold);
        tube(ant0, ant1, 0.015, 7, chrome * 0.9);
        Vec3 va = toView(ant1.x, ant1.y, ant1.z);
        if (va.z > NEAR_Z) { RVert q; q.x = va.x; q.y = va.y; q.z = va.z; q.shade = std::fmod(t, 1.0) < 0.5 ? 60 : 24; rasterPoint3(fb, q, 14, proj, true, dist < 40 ? 2 : 1); }
    }
    // ---- lights: a headlight bar across the nose (bright when on), a tail light bar (red bank 14, brighter when braking)
    {
        RasterParams rl; rl.bank = b.lights ? 1 : 7;
        double hs = b.lights ? 62 : chrome * 0.9;
        RVert q[4]; Vec3 pts[4] = {at(NOSE_F + 0.01, -0.36, 0.5), at(NOSE_F + 0.01, 0.36, 0.5), at(NOSE_F + 0.01, 0.36, 0.6), at(NOSE_F + 0.01, -0.36, 0.6)};
        bool ok = true;
        for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = hs; if (v.z < NEAR_Z) ok = false; }
        if (ok) rasterPolygon(fb, q, 4, rl, proj);
        RasterParams rt; rt.bank = 14;
        double ts = b.brake ? 58 : (b.lights ? 30 : 12);
        RVert tq[4]; Vec3 tp[4] = {at(TAIL_F - 0.01, -0.6, 0.56), at(TAIL_F - 0.01, 0.6, 0.56), at(TAIL_F - 0.01, 0.6, 0.68), at(TAIL_F - 0.01, -0.6, 0.68)};
        ok = true;
        for (int k = 0; k < 4; k++) { Vec3 v = toView(tp[k].x, tp[k].y, tp[k].z); tq[k].x = v.x; tq[k].y = v.y; tq[k].z = v.z; tq[k].shade = ts; if (v.z < NEAR_Z) ok = false; }
        if (ok) rasterPolygon(fb, tq, 4, rt, proj);
    }
}
