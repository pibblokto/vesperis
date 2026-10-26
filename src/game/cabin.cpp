// The Stardrifter's cabin (M2): a 5x5 m room with windows on every side and the roof, drawn
// over the space view in ship-local metres; the explorer walks inside, looks out, uses the
// front computer, the GOES console on the left wall, the screens on the right wall and the
// capsule cage in the middle. The classic cockpit camera remains a setting.
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <algorithm>

namespace {
const double CAB_HALF = 2.5, CAB_H = 2.4, EYE = 1.6, CAGE_R = 1.0;
const double GLASS_SHEEN = 16;   // B-411: the shade of the cabin light on a glass hull
const int BANK_DRONE_PAINT = 30;   // C-18: the drones' paint, a light warm grey under the cabin's light
}

Mat3 Game::viewBasis() const {
    Mat3 shipCam = cameraBasis(ship.yaw, ship.pitch);
    if (!settings.cabin || state == GameState::TITLE) return shipCam;
    return cameraBasis(cabin.yaw, cabin.pitch) * shipCam;
}

void Game::cabinPalette() {
    RGB hull(0.42f, 0.44f, 0.5f), scr(0.25f, 0.9f, 0.55f), warm(1.0f, 0.92f, 0.75f);
    double lit = cabin.lightLevel;
    setRamp(fb.pal, BANK_HULL, {{0, RGB(0, 0, 0)}, {20, hull * (float)(0.12 + 0.25 * lit)}, {40, hull * (float)(0.3 + 0.5 * lit)}, {63, hull * (float)(0.6 + 0.6 * lit)}});
    setRamp(fb.pal, BANK_SCREEN, {{0, RGB(0, 0, 0)}, {24, scr * 0.25f}, {50, scr * 0.8f}, {63, lerp(scr, RGB(1, 1, 1), 0.5f)}});
    for (int k = 1; k <= 11; k++) {   // R-404: the other computers' banks in their own colours (C-11: and the case's; C-18: the bay's)
        if (SCREENS[k].bank == BANK_SCREEN || SCREENS[k].bank == BANK_LIGHT) continue;
        RGB c = SCREENS[k].col;
        setRamp(fb.pal, SCREENS[k].bank, {{0, RGB(0, 0, 0)}, {24, c * 0.25f}, {50, c * 0.8f}, {63, lerp(c, RGB(1, 1, 1), 0.5f)}});
    }
    setRamp(fb.pal, BANK_LIGHT, {{0, RGB(0, 0, 0)}, {30, warm * 0.4f}, {63, warm}});
    const RGB paint(0.78f, 0.76f, 0.7f);
    setRamp(fb.pal, BANK_DRONE_PAINT, {{0, RGB(0, 0, 0)}, {20, paint * (float)(0.1 + 0.2 * lit)}, {44, paint * (float)(0.35 + 0.45 * lit)}, {63, paint * (float)(0.7 + 0.45 * lit)}});
}

// Walking in the cabin: WASD moves, the mouse looks; walls, the cage and the consoles block.
void Game::updateCabin(const Input& in, double realDt) {
    Cabin& c = cabin;
    c.yaw += in.mouseDx * 0.0032 * settings.mouseSensitivity;
    c.pitch -= in.mouseDy * 0.0032 * settings.mouseSensitivity * (settings.invertY ? -1 : 1);
    c.pitch = clampd(c.pitch, -85 * DEG, 85 * DEG);
    c.yaw = wrap2pi(c.yaw);
    double fwd = (in.isDown(KEY_W) ? 1 : 0) - (in.isDown(KEY_S) ? 1 : 0);
    double str = (in.isDown(KEY_D) ? 1 : 0) - (in.isDown(KEY_A) ? 1 : 0);
    if (in.moveY != 0) fwd = in.moveY;   // R-418: a controller's stick (it walked nothing in the cabin)
    if (in.moveX != 0) str = in.moveX;
    if (in.ctrl()) { fwd = 0; str = 0; }
    double sy = std::sin(c.yaw), cy = std::cos(c.yaw);
    double vx = (sy * fwd + cy * str) * 1.6, vz = (cy * fwd - sy * str) * 1.6;
    double nx = c.x + vx * realDt, nz = c.z + vz * realDt;
    if (c.onRoof) {
        nx = clampd(nx, -1.4, 1.4); nz = clampd(nz, -1.4, 1.4);   // the roof platform
    } else {
        nx = clampd(nx, -CAB_HALF + 0.35, CAB_HALF - 0.35); nz = clampd(nz, -CAB_HALF + 0.35, CAB_HALF - 0.35);
        double d = std::sqrt(nx * nx + nz * nz);                      // the capsule cage
        if (d < CAGE_R + 0.3 && d > 1e-6) { nx = nx / d * (CAGE_R + 0.3); nz = nz / d * (CAGE_R + 0.3); }
        if (nx < -1.9 && std::fabs(nz) < 0.9) nx = -1.9;              // GOES console on the left wall
        if (nz < -1.9 && std::fabs(nx) < 0.9) nz = -1.9;              // the shard decoder on the back wall
        if (nz < -1.9 && nx < -0.95) nz = -1.9;                       // the signal radar beside it
        if (nz < -1.9 && nx > 0.95) nz = -1.9;                        // the telescope's set at the wall's other end
        if (nx < -1.95 && nz < -0.85 && nz > -1.9) nx = -1.95;        // the instruments' case on the left wall behind the console
        if (nx < -1.62 && nz > 0.82 && nz < 2.05) nx = -1.62;         // the drone bay's rack by the front window
        if (nz > 1.95 && std::fabs(nx) < 1.5) nz = 1.95;              // the front desk
    }
    c.x = nx; c.z = nz;
    // the roof lifter
    if (in.wasPressed(KEY_PAGE_UP) && !c.onRoof) {
        c.onRoof = true;
        c.x = clampd(c.x, -1.4, 1.4);
        c.z = clampd(c.z, -1.4, 1.4);
        status("ON THE OBSERVATION DECK - PAGE DOWN TO COME BACK IN", 4);
        audio.beep = BEEP_UI;
    } else if (in.wasPressed(KEY_PAGE_DOWN) && c.onRoof) {
        c.onRoof = false;
        status("BACK IN THE CABIN", 3);
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_U) && !in.ctrl()) {   // with the hull glass, the light says so (Y lies beside U)
        c.light = !c.light; audio.beep = BEEP_UI;
        if (c.depolarised) status(c.light ? "CABIN LIGHT ON - Y POLARISES THE GLASS HULL" : "CABIN LIGHT OFF - Y POLARISES THE GLASS HULL", 4);
        else status(c.light ? "CABIN LIGHT ON" : "CABIN LIGHT OFF", 3);
    }
    if (in.wasPressed(KEY_Y) && !in.ctrl()) { c.depolarised = !c.depolarised; status(c.depolarised ? "HULL DEPOLARISED - THE WALLS ARE GLASS" : "HULL POLARISED", 3); audio.beep = BEEP_UI; }
    c.lightLevel += ((c.light ? 1.0 : 0.0) - c.lightLevel) * (1 - std::exp(-realDt * 3));
    // what the explorer faces: the spot most ahead within its reach (a CabinFacing; the three screens on the right wall are 3-5)
    c.facing = FACE_NOTHING;
    if (!c.onRoof) {
        struct Spot { double x, z; int id; double range; };
        const Spot spots[] = {{-2.3, 0.0, FACE_GOES, 1.6}, {0.0, 2.3, FACE_FLIGHT, 1.8}, {2.4, -1.4, FACE_STAR_MAP, 1.5}, {2.4, 0.0, FACE_LANDING, 1.5}, {2.4, 1.4, FACE_TARGET, 1.5},
                {0.0, 0.0, FACE_CAGE, 1.9}, {0.0, -2.3, FACE_DECODER, 1.6}, {-1.55, -2.3, FACE_RADAR, 1.4}, {1.55, -2.3, FACE_TELESCOPE, 1.4}, {-2.35, -1.38, FACE_CASE, 1.4},
                {-2.05, 1.435, FACE_BAY, 2.0}};
        double best = 0.55;
        for (const Spot& s : spots) {
            double dx = s.x - c.x, dz = s.z - c.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > s.range || d < 1e-6) continue;
            double facing = (dx / d) * sy + (dz / d) * cy;
            if (facing > best) { best = facing; c.facing = s.id; }
        }
    }
    if (in.wasPressed(KEY_E) && !in.ctrl()) {
        switch (c.facing) {
            case FACE_GOES: openConsole(); break;
            case FACE_FLIGHT: shipScreenPage = 0; shipScreenSel = 0; state = GameState::SHIPSCREEN; audio.beep = BEEP_UI; break;
            case FACE_STAR_MAP: guideReturn = GameState::SPACE; returnState = GameState::SPACE; openStarMap(); break;
            case FACE_LANDING: deployCapsule(); break;
            case FACE_TARGET: if (sys.valid && ship.localTarget >= 0) { returnState = GameState::SPACE; state = GameState::DATA; } else status("NO LOCAL TARGET ON THE SCREEN", 3); break;
            case FACE_CAGE: deployCapsule(); break;
            case FACE_DECODER: openShards(); break;
            case FACE_RADAR: radarToggle(); break;
            case FACE_TELESCOPE: telescopeToggle(); break;   // the eye to the eyepiece
            case FACE_CASE: openInstruments(); break;   // the instruments' case
            case FACE_BAY: openDroneBay(); break;      // the drone bay
            default: status("NOTHING TO USE HERE - WALK TO A CONSOLE, A SCREEN, THE DECODER, THE CASE OR THE CAPSULE", 3); break;
        }
    }
}

// The cabin drawn over the space view, in ship-local metres (+z forward, +y up, +x right). F-05: in bands of rows on the pool, as the
// ground's rings are (B-310): what the frame shares is taken once (the decoder's wait builds a map and a set a call; the drones on the
// rack), then every band walks every piece in the serial order with its fills clipped to its rows. The bulb's and the orb's glows are not
// clipped to a band: they come after the bands, last as before
void Game::drawCabin() {
    const Cabin& c = cabin;
    CabinFrame f;
    f.look = cameraBasis(c.yaw, c.pitch);
    f.eye = Vec3(c.x, c.onRoof ? CAB_H + 0.9 + EYE - 0.3 : EYE, c.z);
    f.lit = c.lightLevel;
    f.hullShade = 12 + 30 * f.lit;
    f.flick = 0.9 + 0.1 * std::sin(realTime * 7.0);
    if (!c.onRoof) {
        f.waiting = shardsPending();
        f.breathe = f.waiting ? 0.85 + 0.15 * std::sin(realTime * 2.2) : f.flick;
        f.sweep = radar.on ? 0.8 + 0.2 * std::sin(realTime * 3.1) : f.flick;
        f.glow = tele.on ? 0.85 + 0.15 * std::sin(realTime * 2.6) : f.flick;
        const double held = std::min(1.0, guide.instruments.size() / 6.0);
        f.shine = (c.facing == FACE_CASE ? 50 : 26 + 14 * held) * f.flick;
        droneRackFrame(f);
    }
    parallelFor(FBH, 32, [&](int yb, int ye) {
        setRasterBand(yb, ye);
        drawCabinBand(f);
        setRasterBand(0, 1 << 30);
    });
    if (c.onRoof) return;
    // the light bulb and the fuel orb
    const Mat3& look = f.look; const Vec3& eye = f.eye;
    auto toView = [&](double x, double y, double z) { return look * Vec3(x - eye.x, y - eye.y, z - eye.z); };
    const double lit = f.lit;
    {
        Vec3 bulb = toView(0, CAB_H - 0.15, 0);
        if (bulb.z > NEAR_Z) {
            RVert p; p.x = bulb.x; p.y = bulb.y; p.z = bulb.z; p.shade = 20 + 43 * lit;
            rasterPoint3(fb, p, BANK_LIGHT, spaceR.proj, true, 2);
            double sx, sy; if (projectPoint(p, spaceR.proj, sx, sy)) fb.glowDisc(sx, sy, (8 + 14 * lit) * FB_SCALE, 2 * FB_SCALE, (int)(14 * lit), BANK_LIGHT, true);
        }
        double pulse = 0.7 + 0.3 * std::sin(realTime * 1.3);
        Vec3 orb = toView(-2.1, 0.5, 2.1);   // the front-left corner (the telescope's set stands in the back-right)
        if (orb.z > NEAR_Z) {
            RVert p; p.x = orb.x; p.y = orb.y; p.z = orb.z; p.shade = 40 + 20 * pulse;
            rasterPoint3(fb, p, BANK_LIGHT, spaceR.proj, true, 3);
            double sx, sy; if (projectPoint(p, spaceR.proj, sx, sy)) fb.glowDisc(sx, sy, 10 * FB_SCALE / std::max(0.5, orb.z), 2 * FB_SCALE, (int)(12 * pulse), BANK_LIGHT, true);
        }
    }
}

// F-05: the room's pieces in the serial order, by each band thread with its fills clipped to its rows (`drawCabin`); nothing here writes
// what another band reads
void Game::drawCabinBand(const CabinFrame& f) {
    const Cabin& c = cabin;
    const Mat3& look = f.look; const Vec3& eye = f.eye;
    auto toView = [&](double x, double y, double z) { return look * Vec3(x - eye.x, y - eye.y, z - eye.z); };
    const double lit = f.lit;
    const double hullShade = f.hullShade;
    // a depolarised hull is glass (the walls, the floor, the ceiling): the outside tinted through it, and the cabin light
    // caught on it as a sheen that clears with the light off; what stands in the room (the consoles, the sets, the case, the
    // desk) stays solid and lit, where it went to glass with the walls and the light had nothing left to light
    RasterParams solid; solid.bank = BANK_HULL; solid.grain = &cabinGrain; solid.grainScale = 12.0 * FB_SCALE; solid.grainWeight = 0.6;
    RasterParams rp = solid;
    if (c.depolarised) { rp.blend = BLEND_DARKEN; rp.darken = 0.45; rp.sheen = true; rp.grain = nullptr; }
    const double sheen = c.depolarised ? GLASS_SHEEN * lit / hullShade : 1;   // the glass's shade for a shade of the solid hull
    auto face = [&](const RasterParams& prm, Vec3 a, Vec3 b, Vec3 cc, Vec3 d, double shade) {
        RVert q[4]; Vec3 pts[4] = {a, b, cc, d};
        for (int k = 0; k < 4; k++) {
            Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z);
            q[k].x = v.x;
            q[k].y = v.y;
            q[k].z = v.z;
            q[k].shade = shade;
            q[k].u = pts[k].x + pts[k].z * 0.31;
            q[k].v = pts[k].y + pts[k].z * 0.17;
        }
        rasterPolygon(fb, q, 4, prm, spaceR.proj);
    };
    auto quad = [&](Vec3 a, Vec3 b, Vec3 cc, Vec3 d, double shade) { face(rp, a, b, cc, d, shade * sheen); };   // the hull
    auto box = [&](Vec3 a, Vec3 b, Vec3 cc, Vec3 d, double shade) { face(solid, a, b, cc, d, shade); };       // what stands in it
    double H = CAB_HALF, T = CAB_H;
    // walls: below the window, above it, and the pillars at the corners; the window band is open
    const double wy0 = 0.95, wy1 = 2.05, px = 0.45;   // window band, pillar half width
    if (!c.onRoof) {
        // floor with its grille
        quad(Vec3(-H, 0, -H), Vec3(H, 0, -H), Vec3(H, 0, H), Vec3(-H, 0, H), hullShade * 0.7);
        // ceiling as a ring around the round roof window (r 1.5)
        for (int k = 0; k < 12; k++) {
            double a0 = k * TAU / 12, a1 = (k + 1) * TAU / 12;
            Vec3 i0(std::cos(a0) * 1.5, T, std::sin(a0) * 1.5), i1(std::cos(a1) * 1.5, T, std::sin(a1) * 1.5);
            double r0 = std::max(std::fabs(std::cos(a0)), std::fabs(std::sin(a0))), r1 = std::max(std::fabs(std::cos(a1)), std::fabs(std::sin(a1)));
            Vec3 o0(std::cos(a0) / r0 * H, T, std::sin(a0) / r0 * H), o1(std::cos(a1) / r1 * H, T, std::sin(a1) / r1 * H);
            quad(i0, o0, o1, i1, hullShade * 0.85);
        }
    }
    for (int wall = 0; wall < 4; wall++) {
        // wall frame: n = outward normal, u = along the wall
        Vec3 n = wall == 0 ? Vec3(0, 0, 1) : (wall == 1 ? Vec3(1, 0, 0) : (wall == 2 ? Vec3(0, 0, -1) : Vec3(-1, 0, 0)));
        Vec3 u = wall == 0 ? Vec3(1, 0, 0) : (wall == 1 ? Vec3(0, 0, -1) : (wall == 2 ? Vec3(-1, 0, 0) : Vec3(0, 0, 1)));
        Vec3 base = n * H;
        auto P = [&](double along, double y) { return base + u * along + Vec3(0, y, 0); };
        double sh = hullShade * (wall == 0 ? 1.0 : (wall == 2 ? 0.8 : 0.9));
        if (!c.onRoof) {
            quad(P(-H, 0), P(H, 0), P(H, wy0), P(-H, wy0), sh);                 // below the window
            quad(P(-H, wy1), P(H, wy1), P(H, T), P(-H, T), sh * 0.9);           // above
            quad(P(-H, wy0), P(-H + px, wy0), P(-H + px, wy1), P(-H, wy1), sh); // pillars
            quad(P(H - px, wy0), P(H, wy0), P(H, wy1), P(H - px, wy1), sh);
            // mullions: thin bars across the window
            for (double m = -1.25; m <= 1.26; m += 1.25) quad(P(m - 0.03, wy0), P(m + 0.03, wy0), P(m + 0.03, wy1), P(m - 0.03, wy1), sh * 0.7);
        } else {
            // from the deck: the hull below the platform edge
            quad(P(-1.6, T), P(1.6, T), P(1.6, T + 0.9), P(-1.6, T + 0.9), sh * 0.8);
        }
    }
    if (c.onRoof) {
        RasterParams rf = solid;
        RVert q[4]; Vec3 pts[4] = {Vec3(-1.6, T + 0.9, -1.6), Vec3(1.6, T + 0.9, -1.6), Vec3(1.6, T + 0.9, 1.6), Vec3(-1.6, T + 0.9, 1.6)};
        for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = hullShade * 0.7; q[k].u = pts[k].x; q[k].v = pts[k].z; }
        rasterPolygon(fb, q, 4, rf, spaceR.proj);
        return;
    }
    // the capsule cage: eight bars and the hatch ring
    for (int k = 0; k < 8; k++) {
        double a = k * TAU / 8;
        Vec3 p0(std::cos(a) * CAGE_R, 0.05, std::sin(a) * CAGE_R), p1(std::cos(a) * CAGE_R, T - 0.05, std::sin(a) * CAGE_R);
        RVert l0, l1; Vec3 v0 = toView(p0.x, p0.y, p0.z), v1 = toView(p1.x, p1.y, p1.z);
        l0.x = v0.x; l0.y = v0.y; l0.z = v0.z; l0.shade = hullShade * 1.2; l1 = l0; l1.x = v1.x; l1.y = v1.y; l1.z = v1.z;
        rasterLine3(fb, l0, l1, BANK_HULL, spaceR.proj, true, 1);
    }
    {
        RVert ring[16];
        for (int k = 0; k < 16; k++) {
            double a = k * TAU / 16;
            Vec3 v = toView(std::cos(a) * (CAGE_R - 0.1), 0.02, std::sin(a) * (CAGE_R - 0.1));
            ring[k].x = v.x;
            ring[k].y = v.y;
            ring[k].z = v.z;
            ring[k].shade = hullShade * 0.5;
        }
        RasterParams rr; rr.bank = BANK_HULL;
        rasterPolygon(fb, ring, 16, rr, spaceR.proj);
        // the capsule itself, parked in the net: a dome in the light bank
        RasterParams rc; rc.bank = BANK_LIGHT;
        for (int e = 0; e < 3; e++) for (int a = 0; a < 8; a++) {
            double e0 = e * PI / 6, e1 = (e + 1) * PI / 6, a0 = a * TAU / 8, a1 = (a + 1) * TAU / 8, r = 0.7;
            Vec3 pts[4] = {Vec3(std::cos(a0) * std::cos(e0), std::sin(e0), std::sin(a0) * std::cos(e0)), Vec3(std::cos(a1) * std::cos(e0), std::sin(e0), std::sin(a1) * std::cos(e0)),
                           Vec3(std::cos(a1) * std::cos(e1), std::sin(e1), std::sin(a1) * std::cos(e1)), Vec3(std::cos(a0) * std::cos(e1), std::sin(e1), std::sin(a0) * std::cos(e1))};
            RVert q[4];
            for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x * r, 0.3 + pts[k].y * r * 0.9, pts[k].z * r); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = 18 + 20 * lit + 8 * pts[k].y; }
            rasterPolygon(fb, q, 4, rc, spaceR.proj);
        }
    }
    // the GOES console (left wall) and the front desk, with their screens (R-404: each screen in its computer's own bank)
    RasterParams rs; rs.bank = BANK_SCREEN; rs.grain = &cabinGrain; rs.grainScale = 30.0 * FB_SCALE; rs.grainWeight = 0.5;
    auto screen = [&](Vec3 a, Vec3 b, Vec3 cc, Vec3 d, double shade, int facingId) {
        RVert q[4]; Vec3 pts[4] = {a, b, cc, d};
        for (int k = 0; k < 4; k++) {
            Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z);
            q[k].x = v.x;
            q[k].y = v.y;
            q[k].z = v.z;
            q[k].shade = shade;
            q[k].u = pts[k].x + pts[k].z;
            q[k].v = pts[k].y * 3 + realTime * 0.4;
        }
        rs.bank = SCREENS[facingId].bank;
        rasterPolygon(fb, q, 4, rs, spaceR.proj);
    };
    const double flick = f.flick;
    {   // GOES console box against the left wall
        Vec3 a(-H, 0, -0.8), b(-1.9, 0, -0.8), cc(-1.9, 0, 0.8), d(-H, 0, 0.8);
        box(a + Vec3(0, 0.95, 0), b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), d + Vec3(0, 0.95, 0), hullShade * 0.9);   // top
        box(b, b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), cc, hullShade * 0.8);                                            // front face
        screen(Vec3(-1.89, 0.35, -0.6), Vec3(-1.89, 0.35, 0.6), Vec3(-1.89, 0.9, 0.6), Vec3(-1.89, 0.9, -0.6), (c.facing == FACE_GOES ? 50 : 30) * flick, 1);
    }
    {   // the shard decoder against the back wall, a desk like the console's with its screen toward the room; the screen
        // glows and breathes while a shard waits to be read at the language's current share
        Vec3 a(-0.8, 0, -H), b(-0.8, 0, -1.9), cc(0.8, 0, -1.9), d(0.8, 0, -H);
        box(a + Vec3(0, 0.95, 0), b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), d + Vec3(0, 0.95, 0), hullShade * 0.9);   // top
        box(b, b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), cc, hullShade * 0.8);                                            // front face
        const bool waiting = f.waiting;
        const double breathe = f.breathe;
        screen(Vec3(-0.6, 0.35, -1.89), Vec3(0.6, 0.35, -1.89), Vec3(0.6, 0.9, -1.89), Vec3(-0.6, 0.9, -1.89), (c.facing == FACE_DECODER ? 50 : (waiting ? 44 : 30)) * breathe, 7);
    }
    {   // the signal radar beside the decoder, a narrower set against the back wall; its screen brightens while the receiver is on
        Vec3 a(-2.1, 0, -H), b(-2.1, 0, -1.9), cc(-1.0, 0, -1.9), d(-1.0, 0, -H);
        box(a + Vec3(0, 0.95, 0), b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), d + Vec3(0, 0.95, 0), hullShade * 0.9);   // top
        box(b, b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), cc, hullShade * 0.8);                                            // front face
        const double sweep = f.sweep;
        screen(Vec3(-1.95, 0.35, -1.89), Vec3(-1.15, 0.35, -1.89), Vec3(-1.15, 0.9, -1.89), Vec3(-1.95, 0.9, -1.89), (c.facing == FACE_RADAR ? 50 : (radar.on ? 46 : 30)) * sweep, 8);
    }
    {   // (the user's review): the telescope's set at the back wall's other end, the radar's twin; its screen glows while the eye is at the eyepiece
        Vec3 a(1.0, 0, -H), b(1.0, 0, -1.9), cc(2.1, 0, -1.9), d(2.1, 0, -H);
        box(a + Vec3(0, 0.95, 0), b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), d + Vec3(0, 0.95, 0), hullShade * 0.9);   // top
        box(b, b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), cc, hullShade * 0.8);                                            // front face
        const double glow = f.glow;
        screen(Vec3(1.15, 0.35, -1.89), Vec3(1.95, 0.35, -1.89), Vec3(1.95, 0.9, -1.89), Vec3(1.15, 0.9, -1.89), (c.facing == FACE_TELESCOPE ? 50 : (tele.on ? 46 : 30)) * glow, 9);
    }
    {   // the instruments' case on the left wall behind the console: a cabinet, its glass front lit in the case's colour, brighter with
        // what it holds and while an instrument is played
        Vec3 a(-H, 0, -1.85), b(-1.98, 0, -1.85), cc(-1.98, 0, -0.9), d(-H, 0, -0.9);
        box(a + Vec3(0, 1.45, 0), b + Vec3(0, 1.45, 0), cc + Vec3(0, 1.45, 0), d + Vec3(0, 1.45, 0), hullShade * 0.9);   // top
        box(b, b + Vec3(0, 1.45, 0), cc + Vec3(0, 1.45, 0), cc, hullShade * 0.75);                                         // front frame
        box(a, b, b + Vec3(0, 1.45, 0), a + Vec3(0, 1.45, 0), hullShade * 0.6);                                            // its sides
        box(d, cc, cc + Vec3(0, 1.45, 0), d + Vec3(0, 1.45, 0), hullShade * 0.6);
        const double shine = f.shine;
        for (int shelf = 0; shelf < 3; shelf++) {   // three shelves behind the glass, each a band of light
            const double y0 = 0.18 + shelf * 0.42, y1 = y0 + 0.34;
            screen(Vec3(-1.97, y0, -1.78), Vec3(-1.97, y0, -0.97), Vec3(-1.97, y1, -0.97), Vec3(-1.97, y1, -1.78), shine * (0.8 + 0.1 * shelf), 10);
        }
    }
    drawDroneRack(f);   // the drone bay by the front window (game/drone_draw.cpp)
    {   // the front desk under the front window
        box(Vec3(-1.5, 0, 1.95), Vec3(1.5, 0, 1.95), Vec3(1.5, 0, H), Vec3(-1.5, 0, H), hullShade * 0.8);
        box(Vec3(-1.5, 0.85, 1.95), Vec3(1.5, 0.85, 1.95), Vec3(1.5, 0.85, H), Vec3(-1.5, 0.85, H), hullShade * 0.9);
        box(Vec3(-1.5, 0, 1.95), Vec3(1.5, 0, 1.95), Vec3(1.5, 0.85, 1.95), Vec3(-1.5, 0.85, 1.95), hullShade * 0.75);
        screen(Vec3(-1.2, 0.86, 2.0), Vec3(1.2, 0.86, 2.0), Vec3(1.2, 0.95, H - 0.05), Vec3(-1.2, 0.95, H - 0.05), (c.facing == FACE_FLIGHT ? 48 : 28) * flick, 2);
    }
    {   // three screens on the right wall's lower band
        for (int k = 0; k < 3; k++) {
            double zc = (k - 1) * 1.4;
            bool relay = k == 1 && probe.active;   // the landing screen relays the probe; it breathes while one falls, glows on its last frame
            double sh = c.facing == FACE_STAR_MAP + k ? 48 : (relay ? 44 * (probe.endClock >= 0 ? 0.75 + 0.25 * std::sin(realTime * 1.1) : 0.85 + 0.15 * std::sin(realTime * 2.4)) : 26 * flick);
            screen(Vec3(H - 0.02, 0.3, zc - 0.55), Vec3(H - 0.02, 0.3, zc + 0.55), Vec3(H - 0.02, 0.85, zc + 0.55), Vec3(H - 0.02, 0.85, zc - 0.55), sh, 3 + k);
        }
    }
}

void Game::renderCabinHUD() {
    const char* const names[] = {"", "GOES CONSOLE - E TO USE", "FLIGHT COMPUTER - E TO USE", "STAR MAP SCREEN - E", "LANDING MAP SCREEN - E TO DEPLOY THE CAPSULE", "TARGET DATA SCREEN - E",
            "SURFACE CAPSULE - E TO BOARD", "SHARD DECODER - E TO READ THE SHARDS", "SIGNAL RADAR - E TO SWEEP THE SKY", "TELESCOPE - E TO LOOK THROUGH IT", "INSTRUMENT CASE - E TO PLAY",
            "DRONE BAY - E TO SEND THE DRONES"};   // C-06; C-07; W-06; C-11; C-18
    if (cabin.facing > FACE_NOTHING) {
        const char* txt = cabin.facing == FACE_RADAR && radar.on ? "SIGNAL RADAR - E SWITCHES IT OFF" : (cabin.facing == FACE_TELESCOPE && tele.on ? "TELESCOPE - E STOWS IT" : names[cabin.facing]);
        if (cabin.facing == FACE_BAY && bay.out) txt = "DRONE BAY - E FOLLOWS THE DRONES";
        else if (cabin.facing == FACE_BAY && !droneBayReady()) txt = "DRONE BAY - THE DRONES ARE CHARGING";
        // the landing screen and the cage serve the probe at a giant
        bool atGiant = sys.valid && ship.mode == ShipState::PARKED && ship.parkedBody >= 0 && ship.parkedBelt < 0 && isProbeGiant(sys.bodies[ship.parkedBody].type);
        if ((cabin.facing == FACE_LANDING || cabin.facing == FACE_CAGE) && probe.active) txt = probe.endClock >= 0 ? "PROBE RELAY - E SHOWS ITS LAST FRAME" : "PROBE RELAY - E TO WATCH THE PROBE";
        else if (cabin.facing == FACE_LANDING && atGiant) txt = "PROBE SCREEN - E TO LAUNCH A PROBE";
        else if (cabin.facing == FACE_CAGE && atGiant) txt = "PROBE BAY - E TO LAUNCH A PROBE";
        const RGB& c = SCREENS[cabin.facing].col;   // the prompt in the computer's own colour
        drawTextCentered(canvas, UW / 2, UH / 2 + 12, txt, cabin.facing == FACE_CAGE ? HUD_AMBER : rgb((int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255)));
    }
}

// ---------------------------------------------------------------------------
// the front computer (M2-02): three pages of the ship's commands
// ---------------------------------------------------------------------------

namespace {
const char* const PAGE_NAMES[3] = {"FLIGHT CONTROL", "ONBOARD DEVICES", "PREFERENCES"};
const char* const PAGE0[] = {"REMOTE TARGET: AIM AT A STAR", "VIMANA FLIGHT TO THE REMOTE TARGET", "LOCAL TARGET: NEXT BODY", "SOLAR SYSTEM ANALYZER", "FINE APPROACH TO THE LOCAL TARGET",
                       "CENTER THE SHIP ON THE LOCAL TARGET", "ORBIT / FIXED POINT CHASE", "DEPLOY THE CAPSULE OR A PROBE", "TARGET THE HOME STAR", "RETURN TO THE PREVIOUS STAR",
                       "VIMANA JUMP: LIGHT YEARS ALONG THE AIM"};   // X-01: a probe at a giant; R-407: the jump (last, so the rows kept their places)
const char* const PAGE1[] = {"CABIN LIGHT", "HULL POLARISATION", "OBSERVATION DECK LIFTER", "FIELD AMPLIFICATOR", "TIME WARP", "SCANLINES", "SIGNAL RADAR", "THE ALMANAC", "THE TELESCOPE",
        "THE INSTRUMENTS", "THE DRONE BAY"};   // C-07: the radar; W-01: the almanac; C-11: the case; C-18: the drones
const char* const PAGE2[] = {"SETTINGS", "THE GUIDE", "EXPEDITION LOG", "THE SHARDS", "SAVE THE EXPEDITION"};   // C-06: the shards
}
const MenuLabels SHIP_PAGE_LABELS[3] = {{PAGE0, (int)(sizeof(PAGE0) / sizeof(PAGE0[0]))}, {PAGE1, (int)(sizeof(PAGE1) / sizeof(PAGE1[0]))},
                                        {PAGE2, (int)(sizeof(PAGE2) / sizeof(PAGE2[0]))}};

void Game::updateShipScreen(const Input& in, double dt, double realDt) {
    int counts[3] = {11, 11, 5};
    if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_TAB)) { shipScreenPage = (shipScreenPage + 2) % 3; shipScreenSel = 0; }
    if (in.wasPressed(KEY_RIGHT)) { shipScreenPage = (shipScreenPage + 1) % 3; shipScreenSel = 0; }
    int n = counts[shipScreenPage];
    menuStep(in, shipScreenSel, n);
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_E)) { state = GameState::SPACE; return; }
    if (!enterKey(in)) return;
    bool inSystem = sys.valid && !sys.bodies.empty() && ship.mode != ShipState::VIMANA;
    state = GameState::SPACE;
    if (shipScreenPage == 0) switch (shipScreenSel) {
        case 0: if (ship.mode != ShipState::VIMANA) { ship.targeting = true; targetCycle = -1; status("AIM: N NEXT NEAREST STAR, ENTER LOCKS, R CANCELS", 4); } break;
        case 1: toggleVimana(); break;
        case 2:
            if (inSystem) {
                ship.localTarget = (ship.localTarget + 1) % (int)sys.bodies.size();
                ship.targetBelt = -1;
                status(fmt("LOCAL TARGET: %s", upper(bodyNameOf(ship.localTarget)).c_str()), 4);
            }
            break;
        case 3: if (inSystem) { listSel = std::max(0, ship.localTarget); returnState = GameState::SPACE; state = GameState::SYSTEM_LIST; } break;
        case 4: if (inSystem && ship.localTarget >= 0) startApproach(ship.localTarget); break;
        case 5:
            if (inSystem && ship.localTarget >= 0) {
                Vec3 fwd = normalize(sys.bodyPos(ship.localTarget, t) - ship.pos);
                ship.yaw = std::atan2(fwd.x, fwd.z);
                ship.pitch = std::asin(clampd(fwd.y, -1, 1));
                cabin.yaw = 0;
                cabin.pitch = 0;
            }
            break;
        case 6: if (ship.mode == ShipState::PARKED) { ship.orbiting = !ship.orbiting; status(ship.orbiting ? "SYNCHRONOUS ORBIT" : "FIXED POINT CHASE", 3); } break;
        case 7: deployCapsule(); break;
        case 8: guideReturn = GameState::SPACE; targetHome(); state = GameState::SPACE; break;
        case 9: guideReturn = GameState::SPACE; targetPreviousStar(); state = GameState::SPACE; break;
        case 10:
            if (ship.mode != ShipState::VIMANA) {
                ship.targeting = true;
                targetCycle = -1;
                jumpDigits.clear();
                cabin.yaw = 0;
                cabin.pitch = 0;
                status("JUMP: TURN THE SHIP, TYPE THE LIGHT YEARS, ENTER", 6);
            }
            break;   // R-407: the aim along the nose
    } else if (shipScreenPage == 1) switch (shipScreenSel) {
        case 0: cabin.light = !cabin.light; state = GameState::SHIPSCREEN; break;
        case 1: cabin.depolarised = !cabin.depolarised; state = GameState::SHIPSCREEN; break;
        case 2: cabin.onRoof = !cabin.onRoof; cabin.x = clampd(cabin.x, -1.4, 1.4); cabin.z = clampd(cabin.z, -1.4, 1.4); break;
        case 3: fieldAmp = !fieldAmp; state = GameState::SHIPSCREEN; break;
        case 4: timeWarp = timeWarp >= 10000 ? 1 : timeWarp * 10; status(fmt("TIME WARP X%.0f", timeWarp), 2); state = GameState::SHIPSCREEN; break;
        case 5: settings.scanlines = !settings.scanlines; settings.save(settingsPath); state = GameState::SHIPSCREEN; break;
        case 6: radarToggle(); break;   // back to the window, where the sweep is
        case 7: state = GameState::SHIPSCREEN; openAlmanac(); break;   // the almanac from the parking (Esc returns here, a run goes to the window)
        case 8: telescopeToggle(); break;   // back to the window, the eye at the eyepiece
        case 9: openInstruments(); break;
        case 10: openDroneBay(); break;
    } else switch (shipScreenSel) {
        case 0: returnState = GameState::SPACE; settingsSel = 0; state = GameState::SETTINGS; break;
        case 1: openGuide(); break;
        case 2: guideReturn = GameState::SPACE; returnState = GameState::SPACE; logPage = 0; state = GameState::LOG; break;
        case 3: openShards(); break;
        case 4: saveSlot(currentSlot); break;
    }
}

void Game::renderShipScreen() {
    const int top = 22, bottom = UH - 22;
    drawPanel(canvas, 24, top, UW - 24, bottom, rgb(0, 12, 6), 225);
    for (int p = 0; p < 3; p++) drawText(canvas, 34 + p * 96, top + 6, PAGE_NAMES[p], p == shipScreenPage ? HUD_WHITE : HUD_DIM);
    const char* const* items = shipScreenPage == 0 ? PAGE0 : (shipScreenPage == 1 ? PAGE1 : PAGE2);
    int n = shipScreenPage == 0 ? 11 : (shipScreenPage == 1 ? 11 : 5);
    for (int i = 0; i < n; i++) {
        int y = top + 22 + i * 10;
        std::string label = items[i];
        if (shipScreenPage == 1) {
            std::string st;
            switch (i) {
                case 0: st = cabin.light ? "ON" : "OFF"; break;
                case 1: st = cabin.depolarised ? "GLASS" : "OPAQUE"; break;
                case 2: st = cabin.onRoof ? "UP" : "DOWN"; break;
                case 3: st = fieldAmp ? "ON" : "OFF"; break;
                case 4: st = fmt("X%.0f", timeWarp); break;
                case 5: st = settings.scanlines ? "ON" : "OFF"; break;
                case 6: st = radar.on ? "ON" : "OFF"; break;
                case 7: st = almanacRunning ? "RUNNING" : ""; break;
                case 8: st = tele.on ? fmt("X%.0f", telescopeZoom()) : "STOWED"; break;
                case 9: st = fmt("%d", (int)guide.instruments.size()); break;   // C-11: in the case
                default: {   // the bay
                    if (bay.out) st = "AWAY";
                    else if (!droneBayReady()) { const int sec = (int)std::ceil(droneCooldownLeft()); st = fmt("%d:%02d", sec / 60, sec % 60); }
                    else st = "READY";
                    break;
                }
            }
            drawText(canvas, UW - 44 - textWidth(st.c_str()), y, st.c_str(), HUD_AMBER);
        }
        drawText(canvas, 44, y, label.c_str(), i == shipScreenSel ? HUD_WHITE : HUD_GREEN);
        if (i == shipScreenSel) drawText(canvas, 34, y, ">", HUD_AMBER);
    }
    if (shipScreenPage == 0 && sys.valid) {
        std::string st = ship.hasRemote ? fmt("REMOTE %s  ", trunc(upper(starNameOf(ship.remote)), 12).c_str()) : "NO REMOTE TARGET  ";
        st += ship.localTarget >= 0 ? fmt("LOCAL %s", trunc(upper(bodyNameOf(ship.localTarget)), 14).c_str()) : "NO LOCAL TARGET";
        drawText(canvas, 34, bottom - 22, st.c_str(), HUD_CYAN);
    }
    drawTextCentered(canvas, UW / 2, bottom - 10, "LEFT/RIGHT PAGE  UP/DOWN  ENTER  E/ESC LEAVE", HUD_DIM);
}

// ---------------------------------------------------------------------------
// the GOES console (M2-03): a text terminal
// ---------------------------------------------------------------------------

void Game::consolePrint(const std::string& line) {
    consoleLines.push_back(line);
    if (consoleLines.size() > 200) consoleLines.erase(consoleLines.begin());
}

void Game::openConsole() {
    if (consoleLines.empty()) { consolePrint("GOES CONSOLE READY. TYPE HELP."); }
    consoleInput.clear();
    state = GameState::CONSOLE;
    audio.beep = BEEP_UI;
}

void Game::consoleCommand(const std::string& raw) {
    std::string line = upper(raw);
    consolePrint("> " + line);
    std::istringstream is(line);
    std::string cmd; is >> cmd;
    std::string rest; std::getline(is, rest); if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
    if (cmd.empty()) return;
    if (cmd == "HELP") {
        consolePrint("SL [LY]      LIST STARS WITHIN A RANGE (DEFAULT 3)");
        consolePrint("DL           LIST THE BODIES OF THIS SYSTEM");
        consolePrint("PAR NAME     SECTOR COORDINATES OF A STAR");
        consolePrint("ST NAME      SET THE REMOTE TARGET BY NAME");
        consolePrint("WHERE [NAME] POSITION OF THE SHIP OR OF A STAR");
        consolePrint("CAST TEXT    WRITE A NOTE INTO THE LOG");
        consolePrint("CAT          READ THE LAST NOTES");
        consolePrint("HOME / PREV  TARGET THE HOME OR THE PREVIOUS STAR");
        consolePrint("JUMP LY      A VIMANA JUMP ALONG THE SHIP'S NOSE");
    } else if (cmd == "SL") {
        double range = rest.empty() ? 3.0 : atof(rest.c_str());
        std::vector<std::pair<double, const Star*>> list;
        for (const Star& s : nb.stars) { double ly = length(s.pos - ship.pos) / SECTOR_KM; if (ly <= range && !(sys.valid && s.seed == sys.star.seed)) list.push_back({ly, &s}); }
        std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        consolePrint(fmt("%d STARS WITHIN %.1f LY", (int)list.size(), range));
        for (size_t i = 0; i < list.size() && i < 12; i++) {
            Star full;
            starInSector(list[i].second->sx, list[i].second->sy, list[i].second->sz, full, true);
            consolePrint(fmt(
                    "  %-16s %s  %.2f LY  %+lld %+lld %+lld", upper(starNameOf(full)).c_str(), STAR_CLASSES[full.cls].code, list[i].first, (long long)full.sx, (long long)full.sy, (long long)full.sz));
        }
    } else if (cmd == "DL") {
        if (!sys.valid) { consolePrint("NO SYSTEM HERE"); return; }
        consolePrint(fmt("%s: %d BODIES, %d BELTS", upper(starNameOf(sys.star)).c_str(), (int)sys.bodies.size(), (int)sys.belts.size()));
        for (int i = 0; i < (int)sys.bodies.size() && i < 14; i++)
            consolePrint(fmt("  %s%-16s %s", sys.bodies[i].parent >= 0 ? "  " : "", upper(bodyNameOf(i)).c_str(), KINDS[sys.bodies[i].type].name));
        for (size_t k = 0; k < sys.belts.size(); k++)
            consolePrint(fmt("  %-16s BELT %s - %s", upper(beltNameOf((int)k)).c_str(), distanceString(sys.belts[k].innerKm).c_str(), distanceString(sys.belts[k].outerKm).c_str()));
    } else if (cmd == "PAR" || cmd == "WHERE") {
        if (rest.empty()) {
            consolePrint(fmt("SHIP AT SECTOR %+lld %+lld %+lld, %s", (long long)sectorOf(ship.pos.x), (long long)sectorOf(ship.pos.y), (long long)sectorOf(ship.pos.z),
                    REGION_NAMES[galaxyRegion(sectorOf(ship.pos.x), sectorOf(ship.pos.y), sectorOf(ship.pos.z))]));
            return;
        }
        const Star* best = nullptr; double bd = 1e300;
        for (const Star& s : nb.stars) { if (upper(starNameOf(s)).rfind(rest, 0) != 0) continue; double d = length2(s.pos - ship.pos); if (d < bd) { bd = d; best = &s; } }
        if (!best) { consolePrint("NO SUCH STAR NEARBY"); return; }
        Star full; starInSector(best->sx, best->sy, best->sz, full, true);
        consolePrint(fmt("%s: SECTOR %+lld %+lld %+lld, %s %s, %.2f LY", upper(starNameOf(full)).c_str(), (long long)full.sx, (long long)full.sy, (long long)full.sz, STAR_CLASSES[full.cls].code,
                STAR_CLASSES[full.cls].name, std::sqrt(bd) / SECTOR_KM));
    } else if (cmd == "ST") {
        if (rest.empty()) { consolePrint("ST NEEDS A NAME"); return; }
        targetStarByName(rest);
        consolePrint(statusMsg);
    } else if (cmd == "CAST") {
        if (rest.empty()) { consolePrint("CAST NEEDS A TEXT"); return; }
        logEvent("NOTE", rest);
        consolePrint("NOTED.");
    } else if (cmd == "CAT") {
        int shown = 0;
        for (int i = (int)guide.log.size() - 1; i >= 0 && shown < 8; i--) if (guide.log[i].kind == "NOTE") { consolePrint("  " + trunc(guide.log[i].text, 48)); shown++; }
        if (!shown) consolePrint("NO NOTES YET");
    } else if (cmd == "HOME") { guideReturn = GameState::CONSOLE; targetHome(); state = GameState::CONSOLE; consolePrint(statusMsg); }
    else if (cmd == "PREV") { guideReturn = GameState::CONSOLE; targetPreviousStar(); state = GameState::CONSOLE; consolePrint(statusMsg); }
    else if (cmd == "JUMP") {   // R-407: along the ship's nose (the console faces the cabin's wall, not the way)
        double ly = std::atof(rest.c_str());
        if (ly < 1) { consolePrint("JUMP NEEDS LIGHT YEARS, 1 TO 100000"); return; }
        startJump(ly, normalize(Vec3(std::sin(ship.yaw) * std::cos(ship.pitch), std::sin(ship.pitch), std::cos(ship.yaw) * std::cos(ship.pitch))));
        consolePrint(statusMsg);
    }
    else consolePrint("UNKNOWN COMMAND - TYPE HELP");
}

void Game::updateConsole(const Input& in, double dt, double realDt) {
    for (int k = KEY_A; k <= KEY_Z; k++) if (in.wasPressed(k) && consoleInput.size() < 40) consoleInput += (char)('A' + (k - KEY_A));
    for (int k = KEY_0; k <= KEY_9; k++) if (in.wasPressed(k) && consoleInput.size() < 40) consoleInput += (char)('0' + (k - KEY_0));
    if (in.wasPressed(KEY_SPACE) && consoleInput.size() < 40 && !consoleInput.empty()) consoleInput += ' ';
    if (in.wasPressed(KEY_MINUS) && consoleInput.size() < 40) consoleInput += '-';
    if (in.wasPressed(KEY_PERIOD) && consoleInput.size() < 40) consoleInput += '.';
    if (in.wasPressed(KEY_BACKSPACE) && !consoleInput.empty()) consoleInput.pop_back();
    if (in.wasPressed(KEY_ESCAPE)) { state = GameState::SPACE; return; }
    if (in.wasPressed(KEY_ENTER)) { std::string cmd = consoleInput; consoleInput.clear(); consoleCommand(cmd); }
}

void Game::renderConsole() {
    drawPanel(canvas, 6, 6, UW - 6, UH - 6, rgb(0, 10, 4), 235);
    drawText(canvas, 12, 10, "GOES CONSOLE", HUD_AMBER);
    const bool kb = padKeyboardShown();   // R-418: the controller's keyboard takes the lower half; the last lines and the prompt over it
    int rows = kb ? 8 : 17;
    int n = (int)consoleLines.size();
    for (int i = 0; i < rows; i++) {
        int idx = n - rows + i;
        if (idx < 0) continue;
        drawText(canvas, 12, 20 + i * 9, trunc(consoleLines[idx], 50).c_str(), consoleLines[idx][0] == '>' ? HUD_WHITE : HUD_GREEN);
    }
    std::string prompt = "> " + consoleInput + (std::fmod(realTime, 0.8) < 0.4 ? "_" : " ");
    drawText(canvas, 12, kb ? 20 + rows * 9 + 2 : UH - 18, prompt.c_str(), HUD_WHITE);
}

void Game::deployCapsule() {
    if (ship.mode != ShipState::PARKED) { refuse("DEPLOY THE CAPSULE FROM ORBIT: APPROACH A PLANET FIRST"); }
    else if (ship.parkedBelt >= 0) { refuse("NO WORLD UNDER THE SHIP - THE BELT'S ROCKS ARE TOO SMALL FOR THE CAPSULE"); }
    else if (isProbeGiant(sys.bodies[ship.parkedBody].type)) { if (probe.active) watchProbe(); else launchProbe(); }   // X-01: a giant takes a probe (a probe already sent: its screen)
    else if (!KINDS[sys.bodies[ship.parkedBody].type].landable) { refuse("SURFACE NOT CONSISTENT - LANDING IMPOSSIBLE"); }
    else beginLanding();
}

void Game::toggleVimana() {
    if (ship.mode == ShipState::VIMANA) {
        ship.mode = ShipState::STANDBY;
        status("VIMANA FLIGHT ABORTED", 4);
        nb.update(ship.pos);
        const Star* near = nb.nearest(ship.pos);
        if (near) { Star full; starInSector(near->sx, near->sy, near->sz, full, true); sys.generate(full); }
    } else if (!ship.hasRemote) { refuse("NO REMOTE TARGET - PRESS R TO SELECT A STAR"); }
    else {
        if (jumpPlot.seed && jumpPlot.seed == ship.remote.seed) {   // R-407 / G-06: a plotted jump, logged as it is flown
            const Star& r = ship.remote;
            logEvent("JUMP",
                    fmt("%.0f LY %s FROM %s TO PARSIS %+lld %+lld %+lld, %s%s%s", jumpPlot.ly, jumpPlot.way.c_str(), jumpPlot.from.c_str(), (long long)r.sx, (long long)r.sy, (long long)r.sz,
                            REGION_NAMES[galaxyRegion(r.sx, r.sy, r.sz)], jumpPlot.shortLy > 0 ? fmt(" (%.0f LY SHORT: THE VOID)", jumpPlot.shortLy).c_str() : "",
                            jumpPlot.onChart ? " - PLOTTED ON THE CHART" : ""));
            jumpPlot.seed = 0;
        }
        ship.mode = ShipState::VIMANA;
        radarOff();   // nothing is heard in the flight
        ship.flightFrom = ship.pos;
        Vec3 dir = normalize(ship.flightFrom - ship.remote.pos);
        double firstOrbit = std::max(ship.remote.radiusKm * STAR_CLASSES[ship.remote.cls].firstOrbitMult, STAR_CLASSES[ship.remote.cls].minFirstOrbitKm);
        ship.flightTo = ship.remote.pos + dir * (ship.remote.cls == STAR_ROGUE ? drifterStandOffKm(ship.remote) : std::max(ship.remote.radiusKm * 25.0, firstOrbit * 0.55));   // off a drifter
        ship.flightT = 0;
        double ly = length(ship.flightTo - ship.flightFrom) / SECTOR_KM;
        ship.flightDur = vimanaSeconds(ly);   // 7 + 2 sqrt(ly) to 100 ly, then 6 s a doubling
        ship.parkedBody = -1; ship.localTarget = -1; ship.targeting = false;
        ship.targetBelt = -1; ship.parkedBelt = -1;
        sys.valid = false;
        Vec3 fwd = normalize(ship.flightTo - ship.flightFrom);
        ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
        if (std::sqrt(fwd.x * fwd.x + fwd.z * fwd.z) < 0.01) {
            // a flight straight out of the disc has no heading (its yaw would come from the target's few light years aside, and the
            // galactic centre could lie under the floor, the one side without a window): the ship turns its right side to the centre,
            // so the side windows watch the disc fall away; the nose stays within 0.6 degrees of the line
            const Vec3 c = Vec3(GALAXY_CENTRE_SX * SECTOR_KM, 0, GALAXY_CENTRE_SZ * SECTOR_KM) - ship.flightFrom;
            if (c.x * c.x + c.z * c.z > 1) ship.yaw = std::atan2(-c.z, c.x);
        }
        cabin.yaw = 0; cabin.pitch = 0;   // face the front window for the flight
        status(fmt("VIMANA FLIGHT TO %s - %.2f LY", upper(starNameOf(ship.remote)).c_str(), ly), 5);
        audio.beep = BEEP_LOCK;
    }
}
