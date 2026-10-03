// The Stardrifter's cabin (M2): a 5x5 m room with windows on every side and the roof, drawn
// over the space view in ship-local metres; the explorer walks inside, looks out, uses the
// front computer, the GOES console on the left wall, the screens on the right wall and the
// capsule cage in the middle. The classic cockpit camera remains a setting.
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <algorithm>

namespace {
const double CAB_HALF = 2.5, CAB_H = 2.4, EYE = 1.6, CAGE_R = 1.0;
const int BANK_HULL = 8, BANK_SCREEN = 9, BANK_LIGHT = 10;
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
    setRamp(fb.pal, BANK_LIGHT, {{0, RGB(0, 0, 0)}, {30, warm * 0.4f}, {63, warm}});
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
        if (nz < -1.9 && std::fabs(nx) < 0.9) nz = -1.9;              // the shard decoder on the back wall (C-06)
        if (nz < -1.9 && nx < -0.95) nz = -1.9;                       // the signal radar beside it (C-07)
        if (nz > 1.95 && std::fabs(nx) < 1.5) nz = 1.95;              // the front desk
    }
    c.x = nx; c.z = nz;
    // the roof lifter
    if (in.wasPressed(KEY_PAGE_UP) && !c.onRoof) { c.onRoof = true; c.x = clampd(c.x, -1.4, 1.4); c.z = clampd(c.z, -1.4, 1.4); status("ON THE OBSERVATION DECK - PAGE DOWN TO COME BACK IN", 4); audio.beep = 4; }
    else if (in.wasPressed(KEY_PAGE_DOWN) && c.onRoof) { c.onRoof = false; status("BACK IN THE CABIN", 3); audio.beep = 4; }
    if (in.wasPressed(KEY_U) && !in.ctrl()) { c.light = !c.light; status(c.light ? "CABIN LIGHT ON" : "CABIN LIGHT OFF", 3); audio.beep = 4; }
    if (in.wasPressed(KEY_Y) && !in.ctrl()) { c.depolarised = !c.depolarised; status(c.depolarised ? "HULL DEPOLARISED - THE WALLS ARE GLASS" : "HULL POLARISED", 3); audio.beep = 4; }
    c.lightLevel += ((c.light ? 1.0 : 0.0) - c.lightLevel) * (1 - std::exp(-realDt * 3));
    // what the explorer is facing: 0 nothing, 1 GOES console, 2 front screen, 3 right-left, 4 right-middle, 5 right-right, 6 capsule cage, 7 the shard decoder (C-06), 8 the signal radar (C-07)
    c.facing = 0;
    if (!c.onRoof) {
        struct Spot { double x, z; int id; double range; };
        const Spot spots[] = {{-2.3, 0.0, 1, 1.6}, {0.0, 2.3, 2, 1.8}, {2.4, -1.4, 3, 1.5}, {2.4, 0.0, 4, 1.5}, {2.4, 1.4, 5, 1.5}, {0.0, 0.0, 6, 1.9}, {0.0, -2.3, 7, 1.6}, {-1.55, -2.3, 8, 1.4}};
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
            case 1: openConsole(); break;
            case 2: shipScreenPage = 0; shipScreenSel = 0; state = GameState::SHIPSCREEN; audio.beep = 4; break;
            case 3: guideReturn = GameState::SPACE; returnState = GameState::SPACE; openStarMap(); break;
            case 4: deployCapsule(); break;
            case 5: if (sys.valid && ship.localTarget >= 0) { returnState = GameState::SPACE; state = GameState::DATA; } else status("NO LOCAL TARGET ON THE SCREEN", 3); break;
            case 6: deployCapsule(); break;
            case 7: openShards(); break;   // C-06
            case 8: radarToggle(); break;   // C-07
            default: status("NOTHING TO USE HERE - WALK TO A CONSOLE, A SCREEN, THE DECODER, THE RADAR OR THE CAPSULE", 3); break;
        }
    }
}

// The cabin drawn over the space view, in ship-local metres (+z forward, +y up, +x right).
void Game::drawCabin() {
    const Cabin& c = cabin;
    Mat3 look = cameraBasis(c.yaw, c.pitch);
    Vec3 eye(c.x, c.onRoof ? CAB_H + 0.9 + EYE - 0.3 : EYE, c.z);
    auto toView = [&](double x, double y, double z) { return look * Vec3(x - eye.x, y - eye.y, z - eye.z); };
    double lit = c.lightLevel;
    double hullShade = 12 + 30 * lit;
    RasterParams rp; rp.bank = BANK_HULL; rp.grain = &cabinGrain; rp.grainScale = 12.0 * FB_SCALE; rp.grainWeight = 0.6;
    if (c.depolarised) { rp.blend = BLEND_DARKEN; rp.darken = 0.45; rp.grain = nullptr; }
    auto quad = [&](Vec3 a, Vec3 b, Vec3 cc, Vec3 d, double shade) {
        RVert q[4]; Vec3 pts[4] = {a, b, cc, d};
        for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; q[k].u = pts[k].x + pts[k].z * 0.31; q[k].v = pts[k].y + pts[k].z * 0.17; }
        rasterPolygon(fb, q, 4, rp, spaceR.proj);
    };
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
        RasterParams rf = rp; rf.blend = BLEND_REPLACE;
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
        for (int k = 0; k < 16; k++) { double a = k * TAU / 16; Vec3 v = toView(std::cos(a) * (CAGE_R - 0.1), 0.02, std::sin(a) * (CAGE_R - 0.1)); ring[k].x = v.x; ring[k].y = v.y; ring[k].z = v.z; ring[k].shade = hullShade * 0.5; }
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
    // the GOES console (left wall) and the front desk, with their screens
    RasterParams rs; rs.bank = BANK_SCREEN; rs.grain = &cabinGrain; rs.grainScale = 30.0 * FB_SCALE; rs.grainWeight = 0.5;
    auto screen = [&](Vec3 a, Vec3 b, Vec3 cc, Vec3 d, double shade) {
        RVert q[4]; Vec3 pts[4] = {a, b, cc, d};
        for (int k = 0; k < 4; k++) { Vec3 v = toView(pts[k].x, pts[k].y, pts[k].z); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; q[k].u = pts[k].x + pts[k].z; q[k].v = pts[k].y * 3 + realTime * 0.4; }
        rasterPolygon(fb, q, 4, rs, spaceR.proj);
    };
    double flick = 0.9 + 0.1 * std::sin(realTime * 7.0);
    {   // GOES console box against the left wall
        Vec3 a(-H, 0, -0.8), b(-1.9, 0, -0.8), cc(-1.9, 0, 0.8), d(-H, 0, 0.8);
        quad(a + Vec3(0, 0.95, 0), b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), d + Vec3(0, 0.95, 0), hullShade * 0.9);   // top
        quad(b, b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), cc, hullShade * 0.8);                                            // front face
        screen(Vec3(-1.89, 0.35, -0.6), Vec3(-1.89, 0.35, 0.6), Vec3(-1.89, 0.9, 0.6), Vec3(-1.89, 0.9, -0.6), (c.facing == 1 ? 50 : 30) * flick);
    }
    {   // C-06: the shard decoder against the back wall, a desk like the console's with its screen toward the room; the screen
        // glows and breathes while a shard waits to be read at the language's current share
        Vec3 a(-0.8, 0, -H), b(-0.8, 0, -1.9), cc(0.8, 0, -1.9), d(0.8, 0, -H);
        quad(a + Vec3(0, 0.95, 0), b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), d + Vec3(0, 0.95, 0), hullShade * 0.9);   // top
        quad(b, b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), cc, hullShade * 0.8);                                            // front face
        bool waiting = shardsPending();
        double breathe = waiting ? 0.85 + 0.15 * std::sin(realTime * 2.2) : flick;
        screen(Vec3(-0.6, 0.35, -1.89), Vec3(0.6, 0.35, -1.89), Vec3(0.6, 0.9, -1.89), Vec3(-0.6, 0.9, -1.89), (c.facing == 7 ? 50 : (waiting ? 44 : 30)) * breathe);
    }
    {   // C-07: the signal radar beside the decoder, a narrower set against the back wall; its screen brightens while the receiver is on
        Vec3 a(-2.1, 0, -H), b(-2.1, 0, -1.9), cc(-1.0, 0, -1.9), d(-1.0, 0, -H);
        quad(a + Vec3(0, 0.95, 0), b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), d + Vec3(0, 0.95, 0), hullShade * 0.9);   // top
        quad(b, b + Vec3(0, 0.95, 0), cc + Vec3(0, 0.95, 0), cc, hullShade * 0.8);                                            // front face
        double sweep = radar.on ? 0.8 + 0.2 * std::sin(realTime * 3.1) : flick;
        screen(Vec3(-1.95, 0.35, -1.89), Vec3(-1.15, 0.35, -1.89), Vec3(-1.15, 0.9, -1.89), Vec3(-1.95, 0.9, -1.89), (c.facing == 8 ? 50 : (radar.on ? 46 : 30)) * sweep);
    }
    {   // the front desk under the front window
        quad(Vec3(-1.5, 0, 1.95), Vec3(1.5, 0, 1.95), Vec3(1.5, 0, H), Vec3(-1.5, 0, H), hullShade * 0.8);
        quad(Vec3(-1.5, 0.85, 1.95), Vec3(1.5, 0.85, 1.95), Vec3(1.5, 0.85, H), Vec3(-1.5, 0.85, H), hullShade * 0.9);
        quad(Vec3(-1.5, 0, 1.95), Vec3(1.5, 0, 1.95), Vec3(1.5, 0.85, 1.95), Vec3(-1.5, 0.85, 1.95), hullShade * 0.75);
        screen(Vec3(-1.2, 0.86, 2.0), Vec3(1.2, 0.86, 2.0), Vec3(1.2, 0.95, H - 0.05), Vec3(-1.2, 0.95, H - 0.05), (c.facing == 2 ? 48 : 28) * flick);
    }
    {   // three screens on the right wall's lower band
        for (int k = 0; k < 3; k++) {
            double zc = (k - 1) * 1.4;
            screen(Vec3(H - 0.02, 0.3, zc - 0.55), Vec3(H - 0.02, 0.3, zc + 0.55), Vec3(H - 0.02, 0.85, zc + 0.55), Vec3(H - 0.02, 0.85, zc - 0.55), (c.facing == 3 + k ? 48 : 26) * flick);
        }
    }
    // the light bulb and the fuel orb
    {
        Vec3 bulb = toView(0, T - 0.15, 0);
        if (bulb.z > NEAR_Z) {
            RVert p; p.x = bulb.x; p.y = bulb.y; p.z = bulb.z; p.shade = 20 + 43 * lit;
            rasterPoint3(fb, p, BANK_LIGHT, spaceR.proj, true, 2);
            double sx, sy; if (projectPoint(p, spaceR.proj, sx, sy)) fb.glowDisc(sx, sy, (8 + 14 * lit) * FB_SCALE, 2 * FB_SCALE, (int)(14 * lit), BANK_LIGHT, true);
        }
        double pulse = 0.7 + 0.3 * std::sin(realTime * 1.3);
        Vec3 orb = toView(1.9, 0.5, -1.9);
        if (orb.z > NEAR_Z) {
            RVert p; p.x = orb.x; p.y = orb.y; p.z = orb.z; p.shade = 40 + 20 * pulse;
            rasterPoint3(fb, p, BANK_LIGHT, spaceR.proj, true, 3);
            double sx, sy; if (projectPoint(p, spaceR.proj, sx, sy)) fb.glowDisc(sx, sy, 10 * FB_SCALE / std::max(0.5, orb.z), 2 * FB_SCALE, (int)(12 * pulse), BANK_LIGHT, true);
        }
    }
}

void Game::renderCabinHUD() {
    const char* names[] = {"", "GOES CONSOLE - E TO USE", "FLIGHT COMPUTER - E TO USE", "STAR MAP SCREEN - E", "LANDING MAP SCREEN - E TO DEPLOY THE CAPSULE", "TARGET DATA SCREEN - E", "SURFACE CAPSULE - E TO BOARD",
                           "SHARD DECODER - E TO READ THE SHARDS", "SIGNAL RADAR - E TO SWEEP THE SKY"};   // C-06; C-07
    if (cabin.facing > 0) drawTextCentered(canvas, UW / 2, UH / 2 + 12, cabin.facing == 8 && radar.on ? "SIGNAL RADAR - E SWITCHES IT OFF" : names[cabin.facing], HUD_AMBER);
}

// ---------------------------------------------------------------------------
// the front computer (M2-02): three pages of the ship's commands
// ---------------------------------------------------------------------------

namespace {
const char* PAGE_NAMES[3] = {"FLIGHT CONTROL", "ONBOARD DEVICES", "PREFERENCES"};
const char* PAGE0[] = {"REMOTE TARGET: AIM AT A STAR", "VIMANA FLIGHT TO THE REMOTE TARGET", "LOCAL TARGET: NEXT BODY", "SOLAR SYSTEM ANALYZER", "FINE APPROACH TO THE LOCAL TARGET",
                       "CENTER THE SHIP ON THE LOCAL TARGET", "ORBIT / FIXED POINT CHASE", "DEPLOY THE SURFACE CAPSULE", "TARGET THE HOME STAR", "RETURN TO THE PREVIOUS STAR"};
const char* PAGE1[] = {"CABIN LIGHT", "HULL POLARISATION", "OBSERVATION DECK LIFTER", "FIELD AMPLIFICATOR", "TIME WARP", "SCANLINES", "SIGNAL RADAR"};   // C-07: the radar
const char* PAGE2[] = {"SETTINGS", "THE GUIDE", "EXPEDITION LOG", "THE SHARDS", "SAVE THE EXPEDITION"};   // C-06: the shards
}

void Game::updateShipScreen(const Input& in) {
    int counts[3] = {10, 7, 5};
    if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_TAB)) { shipScreenPage = (shipScreenPage + 2) % 3; shipScreenSel = 0; }
    if (in.wasPressed(KEY_RIGHT)) { shipScreenPage = (shipScreenPage + 1) % 3; shipScreenSel = 0; }
    int n = counts[shipScreenPage];
    if (in.wasPressed(KEY_UP)) shipScreenSel = (shipScreenSel + n - 1) % n;
    if (in.wasPressed(KEY_DOWN)) shipScreenSel = (shipScreenSel + 1) % n;
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_E)) { state = GameState::SPACE; return; }
    if (!enterKey(in)) return;
    bool inSystem = sys.valid && !sys.bodies.empty() && ship.mode != ShipState::VIMANA;
    state = GameState::SPACE;
    if (shipScreenPage == 0) switch (shipScreenSel) {
        case 0: if (ship.mode != ShipState::VIMANA) { ship.targeting = true; targetCycle = -1; status("AIM: N NEXT NEAREST STAR, ENTER LOCKS, R CANCELS", 4); } break;
        case 1: toggleVimana(); break;
        case 2: if (inSystem) { ship.localTarget = (ship.localTarget + 1) % (int)sys.bodies.size(); status(fmt("LOCAL TARGET: %s", upper(bodyNameOf(ship.localTarget)).c_str()), 4); } break;
        case 3: if (inSystem) { listSel = std::max(0, ship.localTarget); returnState = GameState::SPACE; state = GameState::SYSTEM_LIST; } break;
        case 4: if (inSystem && ship.localTarget >= 0) startApproach(ship.localTarget); break;
        case 5: if (inSystem && ship.localTarget >= 0) { Vec3 fwd = normalize(sys.bodyPos(ship.localTarget, t) - ship.pos); ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1)); cabin.yaw = 0; cabin.pitch = 0; } break;
        case 6: if (ship.mode == ShipState::PARKED) { ship.orbiting = !ship.orbiting; status(ship.orbiting ? "SYNCHRONOUS ORBIT" : "FIXED POINT CHASE", 3); } break;
        case 7: deployCapsule(); break;
        case 8: guideReturn = GameState::SPACE; targetHome(); state = GameState::SPACE; break;
        case 9: guideReturn = GameState::SPACE; targetPreviousStar(); state = GameState::SPACE; break;
    } else if (shipScreenPage == 1) switch (shipScreenSel) {
        case 0: cabin.light = !cabin.light; state = GameState::SHIPSCREEN; break;
        case 1: cabin.depolarised = !cabin.depolarised; state = GameState::SHIPSCREEN; break;
        case 2: cabin.onRoof = !cabin.onRoof; cabin.x = clampd(cabin.x, -1.4, 1.4); cabin.z = clampd(cabin.z, -1.4, 1.4); break;
        case 3: fieldAmp = !fieldAmp; state = GameState::SHIPSCREEN; break;
        case 4: timeWarp = timeWarp >= 10000 ? 1 : timeWarp * 10; status(fmt("TIME WARP X%.0f", timeWarp), 2); state = GameState::SHIPSCREEN; break;
        case 5: settings.scanlines = !settings.scanlines; settings.save(settingsPath); state = GameState::SHIPSCREEN; break;
        case 6: radarToggle(); break;   // C-07: back to the window, where the sweep is
    } else switch (shipScreenSel) {
        case 0: returnState = GameState::SPACE; settingsSel = 0; state = GameState::SETTINGS; break;
        case 1: openGuide(); break;
        case 2: guideReturn = GameState::SPACE; returnState = GameState::SPACE; logPage = 0; state = GameState::LOG; break;
        case 3: openShards(); break;   // C-06
        case 4: saveSlot(currentSlot); break;
    }
}

void Game::renderShipScreen() {
    const int top = 22, bottom = UH - 22;
    blendRectRGB(canvas, 24, top, UW - 24, bottom, rgb(0, 12, 6), 225);
    drawRectRGB(canvas, 24, top, UW - 24, bottom, HUD_DIM);
    for (int p = 0; p < 3; p++) drawText(canvas, 34 + p * 96, top + 6, PAGE_NAMES[p], p == shipScreenPage ? HUD_WHITE : HUD_DIM);
    const char** items = shipScreenPage == 0 ? PAGE0 : (shipScreenPage == 1 ? PAGE1 : PAGE2);
    int n = shipScreenPage == 0 ? 10 : (shipScreenPage == 1 ? 7 : 5);
    for (int i = 0; i < n; i++) {
        int y = top + 22 + i * 10;
        std::string label = items[i];
        if (shipScreenPage == 1) {
            std::string st = i == 0 ? (cabin.light ? "ON" : "OFF") : (i == 1 ? (cabin.depolarised ? "GLASS" : "OPAQUE") : (i == 2 ? (cabin.onRoof ? "UP" : "DOWN") : (i == 3 ? (fieldAmp ? "ON" : "OFF") : (i == 4 ? fmt("X%.0f", timeWarp) : (i == 5 ? std::string(settings.scanlines ? "ON" : "OFF") : std::string(radar.on ? "ON" : "OFF"))))));
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
    audio.beep = 4;
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
    } else if (cmd == "SL") {
        double range = rest.empty() ? 3.0 : atof(rest.c_str());
        std::vector<std::pair<double, const Star*>> list;
        for (const Star& s : nb.stars) { double ly = length(s.pos - ship.pos) / SECTOR_KM; if (ly <= range && !(sys.valid && s.seed == sys.star.seed)) list.push_back({ly, &s}); }
        std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        consolePrint(fmt("%d STARS WITHIN %.1f LY", (int)list.size(), range));
        for (size_t i = 0; i < list.size() && i < 12; i++) { Star full; starInSector(list[i].second->sx, list[i].second->sy, list[i].second->sz, full, true); consolePrint(fmt("  %-16s %s  %.2f LY  %+lld %+lld %+lld", upper(starNameOf(full)).c_str(), STAR_CLASSES[full.cls].code, list[i].first, (long long)full.sx, (long long)full.sy, (long long)full.sz)); }
    } else if (cmd == "DL") {
        if (!sys.valid) { consolePrint("NO SYSTEM HERE"); return; }
        consolePrint(fmt("%s: %d BODIES, %d BELTS", upper(starNameOf(sys.star)).c_str(), (int)sys.bodies.size(), (int)sys.belts.size()));
        for (int i = 0; i < (int)sys.bodies.size() && i < 14; i++) consolePrint(fmt("  %s%-16s %s", sys.bodies[i].parent >= 0 ? "  " : "", upper(bodyNameOf(i)).c_str(), PLANET_TYPES[sys.bodies[i].type].name));
        for (size_t k = 0; k < sys.belts.size(); k++) consolePrint(fmt("  %-16s BELT %s - %s", upper(beltNameOf((int)k)).c_str(), distanceString(sys.belts[k].innerKm).c_str(), distanceString(sys.belts[k].outerKm).c_str()));   // O3
    } else if (cmd == "PAR" || cmd == "WHERE") {
        if (rest.empty()) { consolePrint(fmt("SHIP AT SECTOR %+lld %+lld %+lld, %s", (long long)sectorOf(ship.pos.x), (long long)sectorOf(ship.pos.y), (long long)sectorOf(ship.pos.z), REGION_NAMES[galaxyRegion(sectorOf(ship.pos.x), sectorOf(ship.pos.y), sectorOf(ship.pos.z))])); return; }
        const Star* best = nullptr; double bd = 1e300;
        for (const Star& s : nb.stars) { if (upper(starNameOf(s)).rfind(rest, 0) != 0) continue; double d = length2(s.pos - ship.pos); if (d < bd) { bd = d; best = &s; } }
        if (!best) { consolePrint("NO SUCH STAR NEARBY"); return; }
        Star full; starInSector(best->sx, best->sy, best->sz, full, true);
        consolePrint(fmt("%s: SECTOR %+lld %+lld %+lld, %s %s, %.2f LY", upper(starNameOf(full)).c_str(), (long long)full.sx, (long long)full.sy, (long long)full.sz, STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, std::sqrt(bd) / SECTOR_KM));
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
    else consolePrint("UNKNOWN COMMAND - TYPE HELP");
}

void Game::updateConsole(const Input& in) {
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
    blendRectRGB(canvas, 6, 6, UW - 6, UH - 6, rgb(0, 10, 4), 235);
    drawRectRGB(canvas, 6, 6, UW - 6, UH - 6, HUD_DIM);
    drawText(canvas, 12, 10, "GOES CONSOLE", HUD_AMBER);
    int rows = 17;
    int n = (int)consoleLines.size();
    for (int i = 0; i < rows; i++) {
        int idx = n - rows + i;
        if (idx < 0) continue;
        drawText(canvas, 12, 20 + i * 9, trunc(consoleLines[idx], 50).c_str(), consoleLines[idx][0] == '>' ? HUD_WHITE : HUD_GREEN);
    }
    std::string prompt = "> " + consoleInput + (std::fmod(realTime, 0.8) < 0.4 ? "_" : " ");
    drawText(canvas, 12, UH - 18, prompt.c_str(), HUD_WHITE);
}

void Game::deployCapsule() {
    if (ship.mode != ShipState::PARKED) { status("DEPLOY THE CAPSULE FROM ORBIT: APPROACH A PLANET FIRST", 4); audio.beep = 3; }
    else if (ship.parkedBelt >= 0) { status("NO WORLD UNDER THE SHIP - THE BELT'S ROCKS ARE TOO SMALL FOR THE CAPSULE", 4); audio.beep = 3; }   // O3
    else if (!PLANET_TYPES[sys.bodies[ship.parkedBody].type].landable) { status("SURFACE NOT CONSISTENT - LANDING IMPOSSIBLE", 4); audio.beep = 3; }
    else beginLanding();
}

void Game::toggleVimana() {
    if (ship.mode == ShipState::VIMANA) {
        ship.mode = ShipState::STANDBY;
        status("VIMANA FLIGHT ABORTED", 4);
        nb.update(ship.pos);
        const Star* near = nb.nearest(ship.pos);
        if (near) { Star full; starInSector(near->sx, near->sy, near->sz, full, true); sys.generate(full); }
    } else if (!ship.hasRemote) { status("NO REMOTE TARGET - PRESS R TO SELECT A STAR", 4); audio.beep = 3; }
    else {
        ship.mode = ShipState::VIMANA;
        radarOff();   // C-07: nothing is heard in the flight
        ship.flightFrom = ship.pos;
        Vec3 dir = normalize(ship.flightFrom - ship.remote.pos);
        double firstOrbit = std::max(ship.remote.radiusKm * STAR_CLASSES[ship.remote.cls].firstOrbitMult, STAR_CLASSES[ship.remote.cls].minFirstOrbitKm);
        ship.flightTo = ship.remote.pos + dir * std::max(ship.remote.radiusKm * 25.0, firstOrbit * 0.55);
        ship.flightT = 0;
        double ly = length(ship.flightTo - ship.flightFrom) / SECTOR_KM;
        ship.flightDur = 7.0 + 2.0 * std::sqrt(ly);
        ship.parkedBody = -1; ship.localTarget = -1; ship.targeting = false;
        ship.targetBelt = -1; ship.parkedBelt = -1;   // O3
        sys.valid = false;
        Vec3 fwd = normalize(ship.flightTo - ship.flightFrom);
        ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
        cabin.yaw = 0; cabin.pitch = 0;   // face the front window for the flight
        status(fmt("VIMANA FLIGHT TO %s - %.2f LY", upper(starNameOf(ship.remote)).c_str(), ly), 5);
        audio.beep = 1;
    }
}
