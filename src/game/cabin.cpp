// The Stardrifter aboard (M2; I-02: three decks and eight spaces, R-426): its rooms drawn over the space view in ship-local metres, the
// explorer walking them, looking out and using the devices (the flight desk, the console arc and the GOES console on the bridge, the
// decoder and the case in the library, the telescope's and the radar's sets in the observatory, the capsule and the drone bay in the
// hangar). The classic cockpit camera remains a setting. The rooms, what stands in them, the walk and the sight are the plan's rows
// (ship_plan.h), the devices this file's.
#include "game.h"
#include "ship_plan.h"
#include "galaxy/nights.h"
#include "ui.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <algorithm>

namespace {
const double GLASS_SHEEN = 16;   // B-411: the shade of the cabin light on a glass hull
const int BANK_DRONE_PAINT = 30;   // C-18: the drones' paint, a light warm grey under the cabin's light
// I-03: the light aboard. A surface's shade: SHADE_FULL x its factors x (AMBIENT, the screens and the stars, + the lamps' light at its
// vertex x the light's level + the orb's x its pulse); the parked world's adds WORLD_SHADES x its light where a room's glass sees it; the
// hull outside OUT_AMBIENT + OUT_SUN x the sun's light x its turn to the sun + OUT_WORLD x the world's
const double SHADE_FULL = CABIN_SHADE_FULL, AMBIENT = CABIN_AMBIENT, WORLD_SHADES = 40, OUT_AMBIENT = 4, OUT_SUN = 46, OUT_WORLD = 30;
const double SUN_FLOOD = 30;   // I-03: with the hull depolarised the sun's light on every face turned to it (no window to cast a patch)
}

// U-03: the ship's devices, a row a CabinFacing in its order: FACE_NOTHING's first (E there says what there is to use), then the GOES
// console in its alcove on the bridge, the flight desk, the console arc's three screens, the capsule's cradle in the hangar, the decoder
// in the library, the radar's and the telescope's sets in the observatory, the case in the library and the drone bay in the hangar
// (their bodies and screens: the plan's fixtures, ship_plan.cpp; I-02 their rooms); I-04: the bunk (the almanac, to rest until an event)
// and the desk (the log) in the quarters, the player's deck in the library (R-431: the player's screen), none with a screen of its own
const RowList<CabinDevice>& Game::cabinDevices() {
    static constexpr auto atGiant = [](const Game& g) {   // the landing screen and the cage serve the probe at a giant
        return g.sys.valid && g.ship.mode == ShipState::PARKED && g.ship.parkedBody >= 0 && g.ship.parkedBelt < 0 && isProbeGiant(g.sys.bodies[g.ship.parkedBody].type);
    };
    static constexpr auto relay = [](const Game& g) {
        return g.probe.endClock >= 0 ? "PROBE RELAY - E SHOWS LAST FRAME" : "PROBE RELAY - E TO WATCH";
    };
    // each: its facing, its room, its spot and reach, where the explorer stands to face it, its lines, its screen's bank and colour,
    // E's act, its screen's shade in the frame (the frame's flicker and its own state), its line while it works
    static constexpr CabinDevice ROWS[] = {
        {FACE_NOTHING, ROOM_BRIDGE, 0, 0, 0, {0, 0, 0, 0}, "", "", BANK_SCREEN, RGB(0.25f, 0.9f, 0.55f),
         [](Game& g) { g.status("NOTHING TO USE HERE - WALK TO A DEVICE", 3); }},
        {FACE_GOES, ROOM_BRIDGE, -4.3, 8.5, 1.6, {-3.2, 8.5, -PI / 2, -0.6}, "GOES CONSOLE - E TO USE", "THE GOES CONSOLE", 21, RGB(1.0f, 0.72f, 0.25f),   // amber
         [](Game& g) { g.openScreen(GameState::CONSOLE); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_GOES ? 50 : 30) * f.flick; }},
        {FACE_FLIGHT, ROOM_BRIDGE, 0.0, 10.2, 1.8, {0.0, 9.1, 0.0, -0.5}, "FLIGHT COMPUTER - E TO USE", "THE FLIGHT COMPUTER", BANK_SCREEN, RGB(0.25f, 0.9f, 0.55f),   // the green phosphor
         [](Game& g) { g.openScreen(GameState::SHIPSCREEN); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_FLIGHT ? 48 : 28) * f.flick; }},
        {FACE_STAR_MAP, ROOM_BRIDGE, 3.8, 8.9, 1.5, {2.7, 8.9, PI / 2, -0.6}, "STAR MAP SCREEN - E", "THE STAR MAP SCREEN", 22, RGB(0.4f, 0.55f, 1.0f),   // blue
         [](Game& g) { g.returnState = GameState::SPACE; g.openScreen(GameState::STAR_MAP); },
         [](const Game& g, const CabinFrame& f) { return g.cabin.facing == FACE_STAR_MAP ? 48 : 26 * f.flick; }},
        {FACE_LANDING, ROOM_BRIDGE, 3.687, 9.819, 1.5, {2.620, 9.553, 1.3265, -0.6}, "LANDING MAP SCREEN - E TO DEPLOY CAPSULE", "THE LANDING SCREEN", 23,
         RGB(1.0f, 0.45f, 0.8f),   // pink (an orange sat too near the radar's red)
         [](Game& g) { g.deployCapsule(); },
         [](const Game& g, const CabinFrame& f) {   // it relays the probe: it breathes while one falls, glows on its last frame
             return g.cabin.facing == FACE_LANDING ? 48 : (g.probe.active ? 44 * (g.probe.endClock >= 0 ? 0.75 + 0.25 * std::sin(g.realTime * 1.1) : 0.85 + 0.15 * std::sin(g.realTime * 2.4)) : 26 * f.flick);
         },
         [](const Game& g) { return g.probe.active ? relay(g) : (atGiant(g) ? "PROBE SCREEN - E TO LAUNCH A PROBE" : nullptr); }},
        {FACE_TARGET, ROOM_BRIDGE, 3.355, 10.684, 1.5, {2.384, 10.168, 1.0821, -0.6}, "TARGET DATA SCREEN - E", "THE TARGET SCREEN", 24, RGB(0.3f, 0.95f, 0.9f),   // cyan
         [](Game& g) {
             if (g.sys.valid && g.ship.localTarget >= 0) { g.returnState = GameState::SPACE; g.openScreen(GameState::DATA); }
             else g.status("NO LOCAL TARGET", 3);
         },
         [](const Game& g, const CabinFrame& f) { return g.cabin.facing == FACE_TARGET ? 48 : 26 * f.flick; }},
        {FACE_CAGE, ROOM_HANGAR, 4.4, 2.4, 3.2, {4.4, -0.6, 0.0, -0.35}, "SURFACE CAPSULE - E TO BOARD", "THE CAPSULE", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),   // no screen
         [](Game& g) { g.deployCapsule(); g.dropNext = g.state == GameState::LANDING_MAP; }, nullptr,   // I-04: boarded here, it drops from the hangar
         [](const Game& g) { return g.probe.active ? relay(g) : (atGiant(g) ? "PROBE BAY - E TO LAUNCH A PROBE" : nullptr); }},
        {FACE_DECODER, ROOM_LIBRARY, -10.3, 3.3, 1.6, {-10.3, 2.2, 0.0, -0.6}, "SHARD DECODER - E TO READ SHARDS", "THE SHARD DECODER", 25, RGB(0.75f, 0.45f, 1.0f),   // C-06: violet
         [](Game& g) { g.openScreen(GameState::SHARDS); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_DECODER ? 50 : (f.waiting ? 44 : 30)) * f.breathe; }},   // breathing while a shard waits
        {FACE_RADAR, ROOM_OBSERVATORY, -3.3586, 2.3516, 1.4, {-2.4575, 1.7207, -0.9599, -0.6}, "SIGNAL RADAR - E TO SWEEP", "THE SIGNAL RADAR", 26, RGB(1.0f, 0.35f, 0.3f),   // C-07: red
         [](Game& g) { g.radarToggle(); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_RADAR ? 50 : (g.radar.on ? 46 : 30)) * f.sweep; },
         [](const Game& g) { return g.radar.on ? "SIGNAL RADAR - E TO SWITCH OFF" : nullptr; }},
        {FACE_TELESCOPE, ROOM_OBSERVATORY, 1.7327, 3.7159, 1.4, {1.2678, 2.7190, 0.4363, -0.6}, "TELESCOPE - E TO LOOK THROUGH", "THE TELESCOPE", 27, RGB(0.8f, 0.9f, 1.0f),   // W-06: ice
         [](Game& g) { g.telescopeToggle(); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_TELESCOPE ? 50 : (g.tele.on ? 46 : 30)) * f.glow; },
         [](const Game& g) { return g.tele.on ? "TELESCOPE - E TO STOW" : nullptr; }},
        {FACE_CASE, ROOM_LIBRARY, -10.3, -3.44, 1.4, {-10.3, -2.34, PI, -0.5}, "INSTRUMENT CASE - E TO PLAY", "THE INSTRUMENT CASE", 28, RGB(0.72f, 1.0f, 0.3f),   // C-11: lime
         [](Game& g) { g.openScreen(GameState::INSTRUMENTS); },
         [](const Game& g, const CabinFrame& f) { return f.shine; }},   // brighter with what it holds and while one is played
        {FACE_BAY, ROOM_HANGAR, -7.55, 0.0, 2.0, {-5.8, 0.0, -PI / 2, -0.38}, "DRONE BAY - E TO SEND DRONES", "THE DRONE BAY", 29, RGB(1.0f, 1.0f, 0.45f),   // C-18: a pale yellow
         [](Game& g) { g.openScreen(GameState::DRONES); }, nullptr,   // its screen's cells: the rack's (drone_draw.cpp)
         [](const Game& g) { return g.bay.out ? "DRONE BAY - E TO FOLLOW DRONES" : nullptr; }},
        {FACE_BUNK, ROOM_QUARTERS, -3.4, -9.6, 1.6, {-2.6, -9.6, -PI / 2, -0.45}, "BUNK - E TO REST UNTIL ...", "THE BUNK", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),   // the almanac's run
         [](Game& g) { g.openScreen(GameState::ALMANAC); }},
        {FACE_DESK, ROOM_QUARTERS, 4.0, -9.4, 1.6, {3.2, -9.4, PI / 2, -0.4}, "DESK - E TO READ LOG", "THE DESK", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),
         [](Game& g) { g.returnState = GameState::SPACE; g.openScreen(GameState::LOG); }},
        {FACE_DECK, ROOM_LIBRARY, -7.3, 2.7, 1.4, {-8.1, 2.7, PI / 2, -0.45}, "PLAYER'S DECK - E TO PLAY", "THE PLAYER'S DECK", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),
         [](Game& g) { g.openScreen(GameState::PLAYER); },   // R-431: the player's own screen (C-11's decoder page before)
         nullptr, [](const Game& g) { return g.player.playing >= 0 ? "PLAYER'S DECK - E FOR LIST" : nullptr; }},
        {FACE_MICROSCOPE, ROOM_LAB, 9.0, -3.35, 1.5, {9.0, -2.2, PI, -0.5}, "MICROSCOPE - E TO LOOK AT VIALS", "THE MICROSCOPE", 31, RGB(0.15f, 0.6f, 0.7f),   // Z-04: teal
         [](Game& g) { g.openScreen(GameState::MICROSCOPE); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_MICROSCOPE ? 50 : 30) * f.flick; },
         [](const Game& g) { return g.coldStore.empty() ? "MICROSCOPE - NO VIALS IN COLD STORE" : nullptr; }},
        {FACE_INCUBATOR, ROOM_LAB, 11.25, -3.35, 1.5, {11.25, -2.2, PI, -0.45}, "INCUBATOR - E TO OPEN", "THE INCUBATOR", 20, RGB(0.8f, 0.45f, 0.0f),   // Z-05: amber
         [](Game& g) { g.openScreen(GameState::INCUBATOR); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_INCUBATOR ? 50 : 32) * f.flick; }},
        {FACE_SEQUENCER, ROOM_LAB, 8.95, 3.35, 1.5, {8.95, 2.2, 0.0, -0.45}, "SEQUENCER - E TO READ A GENOME", "THE SEQUENCER", 47, RGB(0.85f, 0.05f, 0.6f),   // Z-06: magenta (a home past the banks: 30 the drones' paint)
         [](Game& g) { g.openScreen(GameState::SEQUENCER); },
         [](const Game& g, const CabinFrame& f) {   // brighter as it reads, pulsing
             return (g.lab.seq.on && !g.lab.seq.told ? 42 + 8 * std::sin(g.realTime * 5) : (g.cabin.facing == FACE_SEQUENCER ? 50 : 30)) * f.flick; }},
        {FACE_TREE, ROOM_LAB, 6.9, 2.85, 1.6, {7.75, 2.85, -PI / 2, -0.05}, "TREE OF KIN - E TO LOOK", "THE TREE OF KIN", 46, RGB(0.1f, 0.85f, 0.2f),   // Z-06: a deep leaf green (R-404: 0.3 from every other)
         [](Game& g) { g.openScreen(GameState::TREE); },
         [](const Game& g, const CabinFrame& f) { return (g.cabin.facing == FACE_TREE ? 26 : 18) * f.flick; }},   // dim under its lines
        // R-429: the garden's beds in the hall's corners, in the plan's order (port astern, port forward, starboard astern, starboard
        // forward), each faced from toward the stair; E trims it back
        {FACE_BED_1, ROOM_HALL, -4.75, -4.75, 2.6, {-3.2, -3.2, -3 * PI / 4, -0.35}, "GARDEN BED - E TO TRIM", "THE GARDEN BED", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),
         [](Game& g) { g.gardenTrim(0); }, nullptr, [](const Game& g) { return g.gardenGrowth(0) < GARDEN_TRIM_MIN ? "GARDEN BED - GROWING BACK" : nullptr; }},
        {FACE_BED_2, ROOM_HALL, -4.75, 4.75, 2.6, {-3.2, 3.2, -PI / 4, -0.35}, "GARDEN BED - E TO TRIM", "THE GARDEN BED", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),
         [](Game& g) { g.gardenTrim(1); }, nullptr, [](const Game& g) { return g.gardenGrowth(1) < GARDEN_TRIM_MIN ? "GARDEN BED - GROWING BACK" : nullptr; }},
        {FACE_BED_3, ROOM_HALL, 4.75, -4.75, 2.6, {3.2, -3.2, 3 * PI / 4, -0.35}, "GARDEN BED - E TO TRIM", "THE GARDEN BED", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),
         [](Game& g) { g.gardenTrim(2); }, nullptr, [](const Game& g) { return g.gardenGrowth(2) < GARDEN_TRIM_MIN ? "GARDEN BED - GROWING BACK" : nullptr; }},
        {FACE_BED_4, ROOM_HALL, 4.75, 4.75, 2.6, {3.2, 3.2, PI / 4, -0.35}, "GARDEN BED - E TO TRIM", "THE GARDEN BED", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),
         [](Game& g) { g.gardenTrim(3); }, nullptr, [](const Game& g) { return g.gardenGrowth(3) < GARDEN_TRIM_MIN ? "GARDEN BED - GROWING BACK" : nullptr; }},
        // R-428: the quarters' wall of photographs, faced from the room; E hangs them
        {FACE_PRINTS, ROOM_QUARTERS, -3.15, -6.35, 2.4, {-3.15, -8.1, 0.0, 0.05}, "PHOTOGRAPHS - E TO HANG", "THE PHOTOGRAPHS", BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f),
         [](Game& g) { g.openScreen(GameState::WALL); }},
        // Y-02: the gene editor on the bow bench past the sequencer: a blue-violet (a home past the banks; 0.43 from
        // the nearest, the star map's)
        {FACE_EDITOR, ROOM_LAB, 11.55, 3.35, 1.5, {11.55, 2.2, 0.0, -0.45}, "GENE EDITOR - E TO EDIT GENES", "THE GENE EDITOR", 48, RGB(0.25f, 0.15f, 0.95f),
         [](Game& g) { g.openScreen(GameState::EDITOR); },
         [](const Game& g, const CabinFrame& f) {   // brighter as it writes, pulsing
             return (g.lab.edit.writing ? 42 + 8 * std::sin(g.realTime * 4) : (g.cabin.facing == FACE_EDITOR ? 50 : 30)) * f.flick; }},
    };
    static constexpr RowList<CabinDevice> LIST = rowsOf(ROWS);
    static_assert(devicesWhole(LIST), "a device a row, in CabinFacing's order, with its lines and its act");
    return LIST;
}

Mat3 Game::viewBasis() const {
    Mat3 shipCam = cameraBasis(ship.yaw, ship.pitch);
    if (!settings.cabin || state == GameState::TITLE) return shipCam;
    const Cabin c = cabinSeen();   // I-04: the hatch's drop seen from the hangar
    return cameraBasis(c.yaw, c.pitch) * shipCam;
}

// the hull's and the light's ramps, and each bank the frame dealt (I-02, `cabinSight`): a screen's colour (R-404: every computer its own)
// or the drones' paint; I-03: a finish's (its ramp fixed: the light is in the shades, the lamps' pools and the sun's patches; the hull
// outside's toward the star's colour)
void Game::cabinPalette() {
    RGB warm(1.0f, 0.92f, 0.75f);
    double lit = cabin.lightLevel;
    auto finishRamp = [&](int bank, const RGB& c) {
        const RGB top(std::min(1.0f, c.r * 1.55f), std::min(1.0f, c.g * 1.55f), std::min(1.0f, c.b * 1.55f));
        setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {16, c * 0.22f}, {34, c * 0.62f}, {50, c}, {63, lerp(top, RGB(1, 1, 1), 0.15f)}});
    };
    const ShipRows<ShipFinishRow> fins = shipFinishes();
    finishRamp(BANK_HULL, fins[FIN_HULL].col);
    const ShipBanks& B = cabinFrame.banks;
    for (int home = 0; home < SHIP_HOMES; home++) {
        const int b = B.of[home];
        if (b < 0) continue;
        RGB c = B.col[home];
        int finish = -1;
        for (int k = 0; k < SHIP_FINISHES; k++) if (k != FIN_HULL && fins[k].home == home) finish = k;
        if (home == BANK_DRONE_PAINT)
            setRamp(fb.pal, b, {{0, RGB(0, 0, 0)}, {20, c * (float)(0.1 + 0.2 * lit)}, {44, c * (float)(0.35 + 0.45 * lit)}, {63, c * (float)(0.7 + 0.45 * lit)}});
        else if (finish >= 0) {
            if (finish == FIN_OUTSIDE && sys.valid && !sys.sunless()) {
                const RGB& sc = sys.star.color;
                c = RGB(c.r * (0.4f + 0.6f * sc.r), c.g * (0.4f + 0.6f * sc.g), c.b * (0.4f + 0.6f * sc.b));
            }
            finishRamp(b, c);
        } else setRamp(fb.pal, b, {{0, RGB(0, 0, 0)}, {24, c * 0.25f}, {50, c * 0.8f}, {63, lerp(c, RGB(1, 1, 1), 0.5f)}});
    }
    setRamp(fb.pal, BANK_LIGHT, {{0, RGB(0, 0, 0)}, {30, warm * 0.4f}, {63, warm}});
}

// Walking aboard: WASD moves (Shift a run), the mouse looks; the walls, what stands in the rooms and the stair's rails block, the arches
// and the stair lead on (I-02: the plan's walk)
void Game::updateCabin(const Input& in, double realDt) {
    Cabin& c = cabin;
    const RowList<CabinDevice>& devices = cabinDevices();
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
    const double pace = in.shift() ? 3.2 : 1.6;
    double vx = (sy * fwd + cy * str) * pace, vz = (cy * fwd - sy * str) * pace;
    ShipPlace p = c.place();
    shipWalk(p, c.x + vx * realDt, c.z + vz * realDt);   // the room's floors and arches, its colliders in their order, the stair
    c.placeAt(p);
    // the lift (I-02: from any room up to the observatory, down to the bridge), its arrival the plan's place
    const ShipOpening& lift = shipLifter();
    if (in.wasPressed(KEY_PAGE_UP) && c.room != lift.to) {
        c.placeAt(shipPlaceOf(lift.to, lift.up.x, lift.up.z));
        status(lift.upSays, 4);
        audio.beep = BEEP_UI;
    } else if (in.wasPressed(KEY_PAGE_DOWN) && c.room == lift.to) {
        c.placeAt(shipPlaceOf(lift.from, lift.down.x, lift.down.z));
        status(lift.downSays, 3);
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_U) && !in.ctrl()) {   // with the hull glass, the light says so (Y lies beside U)
        c.light = !c.light; audio.beep = BEEP_UI;
        if (c.depolarised) status(c.light ? "CABIN LIGHT ON - Y POLARISES GLASS HULL" : "CABIN LIGHT OFF - Y POLARISES GLASS HULL", 4);
        else status(c.light ? "CABIN LIGHT ON" : "CABIN LIGHT OFF", 3);
    }
    if (in.wasPressed(KEY_Y) && !in.ctrl()) { c.depolarised = !c.depolarised; status(c.depolarised ? "HULL DEPOLARISED - WALLS ARE GLASS" : "HULL POLARISED", 3); audio.beep = BEEP_UI; }
    c.lightLevel += ((c.light ? 1.0 : 0.0) - c.lightLevel) * (1 - std::exp(-realDt * 3));
    // what the explorer faces: of the devices in the room stood in, the one whose spot is most ahead within its reach (nothing's has
    // no reach)
    c.facing = FACE_NOTHING;
    double best = 0.55;
    for (const CabinDevice& s : devices) {
        if (s.room != c.room) continue;
        double dx = s.x - c.x, dz = s.z - c.z, d = std::sqrt(dx * dx + dz * dz);
        if (d > s.reach || d < 1e-6) continue;
        double facing = (dx / d) * sy + (dz / d) * cy;
        if (facing > best) { best = facing; c.facing = s.facing; }
    }
    if (in.wasPressed(KEY_E) && !in.ctrl()) devices.row[c.facing].act(*this);
}

// I-02: what the eye sees, before the space view: the rooms through the arches, the well and the oculus (each with the screen's box it is
// seen in), whether the outside is (a window, the oculus's dome, the glass hull: else the space view is not drawn and every bank is the
// ship's), and what asks for a bank (each seen device's screen by its nearness; the drones' paint and lamps while the rack or the drones
// outside are), dealt once the outside is drawn (`cabinDeal`)
void Game::cabinSight() {
    CabinFrame& f = cabinFrame;
    const Cabin c = cabinSeen();   // I-04: the explorer's eye, or the hatch's drop's camera
    f.look = cameraBasis(c.yaw, c.pitch);
    f.eye = Vec3(c.x, c.eyeY(), c.z);
    f.drop = cabinDropping() ? DROP_S + transT : -1;
    shipSight(c.place(), f.eye, f.look, spaceR.proj, c.depolarised, f.sight);
    if (harness.shipOutside || f.drop >= 0) f.sight.seesOut = true;   // the drop: the outside through the open hatch
    const RowList<CabinDevice>& devices = cabinDevices();
    std::vector<ShipBankAsk>& asks = f.asks;
    asks.clear();
    lifeSight(f, asks);   // I-04: the prints, the garden's home, the shelves' worlds (the decoder's bank for their shards)
    // I-03: the seen rooms' finishes, each at its nearest room (the room stood in's at once), and the hull outside's while the outside is
    // seen; then the screens by their nearness
    const ShipMesh& mesh = shipMesh();
    const ShipRows<ShipFinishRow> fins = shipFinishes();
    double near[SHIP_FINISHES];
    for (double& n : near) n = 1e30;
    const int eyeRoom = shipEyeRoom(c.place());
    for (int r = 0; r < SHIP_ROOMS; r++) {
        if (!f.sight.seen[r]) continue;
        const double d = r == eyeRoom ? 0 : shipRoomNear(r, f.eye);
        for (int k = 0; k < SHIP_FINISHES; k++) if (mesh.finishes[r] & (1u << k)) near[k] = std::min(near[k], d);
    }
    if (f.sight.seesOut) for (int k = 0; k < SHIP_FINISHES; k++) if (mesh.finishes[SHIP_OUTSIDE] & (1u << k)) near[k] = std::min(near[k], 0.5);
    for (int k = 0; k < SHIP_FINISHES; k++) if (k != FIN_HULL && near[k] < 1e29) asks.push_back({fins[k].home, finishColour(k), near[k]});
    for (const CabinDevice& d : devices) {
        if (d.facing == FACE_NOTHING || d.bank == BANK_LIGHT || !f.sight.seen[d.room]) continue;
        if ((f.look * (Vec3(d.x, f.eye.y, d.z) - f.eye)).z <= -0.75) continue;   // Z-06: behind the eye, nothing of its screen drawn: no bank of the pool
        asks.push_back({d.bank, d.col, std::hypot(d.x - c.x, d.z - c.z)});
    }
    const ShipFixture& rack = shipFixtureOf(FX_RACK);
    if (f.sight.seen[rack.room] || (bay.out && f.sight.seesOut)) {   // C-18's drones: their paint, the bay's accent, their eye's cyan, the beacon's red, the lamps' amber and green
        const double near = std::hypot(rack.droneX - c.x, rack.droneZ - c.z);
        asks.push_back({BANK_DRONE_PAINT, RGB(0.78f, 0.76f, 0.7f), near});
        for (int k : {FACE_BAY, FACE_TARGET, FACE_RADAR, FACE_GOES, FACE_FLIGHT}) asks.push_back({devices.row[k].bank, devices.row[k].col, near});
    }
    if (f.sight.seen[ROOM_MAINTENANCE]) {   // R-427: the buggy and the drone on their platforms, in the drones' paint, their tail lights red
        const double near = shipRoomNear(ROOM_MAINTENANCE, f.eye);
        asks.push_back({BANK_DRONE_PAINT, RGB(0.78f, 0.76f, 0.7f), near});
        asks.push_back({devices.row[FACE_RADAR].bank, devices.row[FACE_RADAR].col, near});
    }
}

// B-435: the banks dealt once the outside is drawn (it was the cabin's own eleven, and a far finish or screen fell to the hull's grey until
// the explorer came near it). B-437: looking out, the space view's pixels in the glass's box are sunk into one bank (`BANK_SPACE`), read
// through the space view's palette (the panes keep its pixels, the walls cover the rest): the pool is every bank but 0 (the cleared frame),
// the hull's, the light's and that one, whatever the outside shows. Keeping the banks the outside showed from the pool still left eleven
// where the glass showed two worlds' discs and a second sun, and the hangar asks twenty-one (KI-441)
void Game::cabinDeal() {
    CabinFrame& f = cabinFrame;
    uint32_t outside = 0;
    bool sunk = false;
    if (f.sight.seesOut) {
        // where the outside shows: the glass's box (the panes in view; the whole frame with the hull depolarised), the whole frame in the
        // hatch's drop (the open hatch is no pane); nowhere with no glass in view (the harness's outside, drawn under the walls)
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        if (f.drop >= 0) { x1 = FBW; y1 = FBH; }
        else if (f.sight.ox1 > f.sight.ox0 && f.sight.oy1 > f.sight.oy0) {
            x0 = clampi((int)std::floor(f.sight.ox0) - 1, 0, FBW); x1 = clampi((int)std::ceil(f.sight.ox1) + 1, 0, FBW);
            y0 = clampi((int)std::floor(f.sight.oy0) - 1, 0, FBH); y1 = clampi((int)std::ceil(f.sight.oy1) + 1, 0, FBH);
        }
        if (harness.shipBusyOutside) {   // the unit's worst outside: the space view's every bank in diagonal stripes across the box
            static const int SPACE_BANKS[18] = {1, 2, 3, 4, 5, 6, 7, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 31};
            for (int y = y0; y < y1; y++)
                for (int x = x0; x < x1; x++) fb.idx[(size_t)y * FBW + x] = pix(SPACE_BANKS[((x + y) / FB_SCALE) % 18], 40);
        }
        if (x1 > x0 && y1 > y0) { outside = fb.sinkUnder(BANK_SPACE, x0, y0, x1, y1); sunk = true; }
    }
    f.outsideBanks = outside;
    f.sunk = sunk;
    f.banks.deal(f.asks, 1u | (1u << BANK_HULL) | (1u << BANK_LIGHT) | (sunk ? 1u << BANK_SPACE : 0u));
}

// I-03: the daylight aboard: the sun toward it in the ship's metres and its light (the system's brighter star's `lightAt`; none sunless,
// in a flight, or with a world before its disc: the share of the disc still showing), the parked world's way and its light (`bodyShineAt`:
// its lit share, its albedo, its size; a glowing world's own), and each room's sight of it through its glass
void Game::cabinDaylight(CabinFrame& f) {
    f.sunLight = 0; f.worldLight = 0;
    for (double& g : f.glassToWorld) g = 0;
    f.pulse = 0.7 + 0.3 * std::sin(realTime * 1.3);
    if (!sys.valid || ship.mode == ShipState::VIMANA) return;
    const Mat3 shipCam = cameraBasis(ship.yaw, ship.pitch);
    const StarSystem::Light L = sys.lightAt(ship.pos, t);
    if (L.factor > 0) {
        const Vec3 sunPos = L.fromCompanion ? sys.bodyPos(sys.companion, t) : sys.star.pos;
        const double sunR = std::max(0.002, std::asin(clampd((L.fromCompanion ? sys.bodies[sys.companion].radiusKm : sys.star.radiusKm) / std::max(1.0, length(sunPos - ship.pos)), 0, 1)));
        double showing = 1;
        for (int i = 0; i < (int)sys.bodies.size(); i++) {
            if (i == sys.companion) continue;
            const Vec3 bp = sys.bodyPos(i, t) - ship.pos;
            const double d = length(bp);
            if (d < 1 || dot(bp, L.dir) <= 0) continue;
            const double off = std::acos(clampd(dot(bp * (1.0 / d), L.dir), -1, 1)), br = std::asin(clampd(sys.bodies[i].radiusKm / d, 0, 1));
            showing = std::min(showing, clampd((off - (br - sunR)) / (2 * sunR), 0, 1));
        }
        f.sun = shipCam * L.dir;
        f.sunLight = L.factor * showing;
    }
    if (ship.mode == ShipState::PARKED && ship.parkedBody >= 0 && ship.parkedBelt < 0) {
        f.worldLight = bodyShineAt(sys, ship.parkedBody, ship.pos, t, L.factor);
        f.world = shipCam * normalize(sys.bodyPos(ship.parkedBody, t) - ship.pos);
        for (int r = 0; r < SHIP_ROOMS; r++) f.glassToWorld[r] = f.worldLight > 0 ? (cabin.depolarised ? 1.0 : shipGlassToward(r, f.world)) : 0;
    }
}

// The ship drawn over the space view (or over nothing when no window is in view, I-02), in ship-local metres (+z forward, +y up, +x right).
// F-05: in bands of rows on the pool, as the ground's rings are (B-310). I-01: what the frame shares is taken once (the decoder's wait
// builds a map and a set a call; the drones on the rack; each device's screen), the seen rooms' pieces are projected once in their serial
// order (`drawCabinPieces`), then every band fills those that reach its rows. The lamps after the bands, last as before (I-03: their points;
// their light lies on the rooms as pools)
void Game::drawCabin() {
    CabinFrame& f = cabinFrame;
    const Cabin& c = cabin;
    const RowList<CabinDevice>& devices = cabinDevices();
    f.lit = c.lightLevel;
    f.hullShade = 12 + 30 * f.lit;
    f.flick = 0.9 + 0.1 * std::sin(realTime * 7.0);
    f.drive = c.drive;   // I-04: the flight's (the core, the drive's glow)
    cabinDaylight(f);
    for (double& s : f.screen) s = 0;
    bool devicesSeen = false;   // a device in a seen room: their screens, the decoder's wait, the rack's drones
    for (const CabinDevice& d : devices) if (d.facing != FACE_NOTHING && f.sight.seen[d.room]) devicesSeen = true;
    if (devicesSeen) {
        f.waiting = shardsPending();
        f.breathe = f.waiting ? 0.85 + 0.15 * std::sin(realTime * 2.2) : f.flick;
        f.sweep = radar.on ? 0.8 + 0.2 * std::sin(realTime * 3.1) : f.flick;
        f.glow = tele.on ? 0.85 + 0.15 * std::sin(realTime * 2.6) : f.flick;
        const double held = std::min(1.0, guide.instruments.size() / 6.0);
        f.shine = (c.facing == FACE_CASE ? 50 : 26 + 14 * held) * f.flick;
        if (f.sight.seen[shipFixtureOf(FX_RACK).room]) droneRackFrame(f);
        for (const CabinDevice& d : devices) if (d.screen && f.sight.seen[d.room]) f.screen[d.facing] = d.screen(*this, f);
    }
    f.shapes.begin(spaceR.proj);
    drawCabinPieces(f);
    f.shapes.noClip();
    parallelFor(FBH, 32, [&](int yb, int ye) {
        setRasterBand(yb, ye);
        f.shapes.fill(fb);
        setRasterBand(0, 1 << 30);
    });
    // the lamps of the seen rooms: their points (the bulbs, the glows; I-04: a glow kept at U's night)
    const Mat3& look = f.look; const Vec3& eye = f.eye;
    const double lit = f.lit;
    for (const ShipLamp& L : shipLamps()) {
        if (!f.sight.seen[L.room]) continue;
        const Vec3 v = look * Vec3(L.x - eye.x, L.y - eye.y, L.z - eye.z);
        if (v.z <= NEAR_Z) continue;
        RVert p; p.x = v.x; p.y = v.y; p.z = v.z;
        p.shade = 20 + 43 * (L.kind == LAMP_GLOW ? lit + (1 - lit) * SHIP_NIGHT_GLOW : lit);
        rasterPoint3(fb, p, BANK_LIGHT, spaceR.proj, true, 2);
    }
}

// The outside first, where its glass is seen (I-03: the roofs, the prow, the drive's ring, the mast, the drones' door; the hull's skin not
// with the hull depolarised), then the seen rooms, the room stood in first and on by their depth (I-02), each piece by piece in its serial
// order (the plan's mesh, ship_plan.cpp) within the screen's box its room is seen in, each projected once into the frame's shapes: its
// vertices seen from the eye, its shade by its material and the frame's light, its grain's u and v; a face turned from the eye left out; the
// rack and the mast where they come; then the room's sun patches (added in the surfaces' own banks). A window's glass is written to depth
// only (what is behind it stays the outside). A depolarised hull is glass (the outer walls, floors and ceilings): the outside tinted
// through it, and the cabin light caught on it as a sheen that clears with the light off; the inner walls and what stands in the rooms
// stay solid and lit. A screen dealt no bank dims to the hull's grey, a finish falls to the hull's bank
void Game::drawCabinPieces(CabinFrame& f) {
    const Cabin c = cabinSeen();
    const Mat3& look = f.look; const Vec3& eye = f.eye;
    const double lit = f.lit;
    const ShipMesh& mesh = shipMesh();
    const RowList<CabinDevice>& devices = cabinDevices();
    const ShipRows<ShipFinishRow> fins = shipFinishes();
    RasterParams fin[SHIP_FINISHES], plain[SHIP_FINISHES];   // a finish's: its dealt bank (the hull's when none), its grain; plain: none
    for (int k = 0; k < SHIP_FINISHES; k++) {
        const int b = k == FIN_HULL ? BANK_HULL : f.banks.bankOf(fins[k].home);
        fin[k].bank = plain[k].bank = b >= 0 ? b : BANK_HULL;
        if (fins[k].grainScale > 0) { fin[k].grain = &cabinGrain; fin[k].grainScale = fins[k].grainScale * FB_SCALE; fin[k].grainWeight = fins[k].grainWeight; }
    }
    RasterParams glass = fin[FIN_HULL];   // the depolarised hull (B-411)
    glass.blend = BLEND_DARKEN; glass.darken = 0.45; glass.sheen = true; glass.grain = nullptr;
    RasterParams tinted = plain[FIN_GLASS];   // a glass finish: what is behind darkened, the light caught on it
    tinted.blend = BLEND_DARKEN; tinted.darken = 0.6; tinted.sheen = true;
    RasterParams rs; rs.grain = &cabinGrain; rs.grainScale = 30.0 * FB_SCALE; rs.grainWeight = 0.5;   // a screen's (R-404: in its computer's own bank)
    RasterParams light; light.bank = BANK_LIGHT;
    RasterParams pane; pane.blend = BLEND_DARKEN; pane.darken = 1.0;   // the pixel kept, its depth written
    RasterParams patch; patch.blend = BLEND_ADD; patch.zbias = 1.0015; patch.zwrite = false;   // a sun patch: on the surface it lies on only
    CabinDraw draw; draw.fin = fin; draw.plain = plain; draw.light = &light; draw.pane = &pane; draw.tint = &tinted;   // I-04: what the explorer's things take
    const bool flood = c.depolarised && f.sunLight > 0;   // the hull glass: the sun on every face turned to it, no patches
    auto outShade = [&](const Vec3& n) {   // the hull outside: the sun's light by its turn to it, the parked world's
        return OUT_AMBIENT + OUT_SUN * f.sunLight * std::max(0.0, dot(n, f.sun)) + OUT_WORLD * f.worldLight * std::max(0.0, dot(n, f.world));
    };
    auto toView = [&](const Vec3& p, RVert& q) { const Vec3 w = look * Vec3(p.x - eye.x, p.y - eye.y, p.z - eye.z); q.x = w.x; q.y = w.y; q.z = w.z; };
    // the radar's mast: a post, and its dish at the top facing where it turns (slowly; faster while the radar sweeps), tilted up
    auto mast = [&]() {
        const ShipFixture& M = shipFixtureOf(FX_MAST);
        const RasterParams& prm = fin[FIN_OUTSIDE];
        const Vec3 foot(M.x[0], M.y[0], M.z[0]), top = foot + Vec3(0, M.height, 0);
        const double w = 0.08;
        for (int k = 0; k < 4; k++) {
            const Vec3 n = k == 0 ? Vec3(1, 0, 0) : (k == 1 ? Vec3(0, 0, 1) : (k == 2 ? Vec3(-1, 0, 0) : Vec3(0, 0, -1))), u = Vec3(n.z, 0, -n.x);
            const Vec3 o = n * w;
            if (dot(n, eye - (foot + o)) < 0) continue;
            const Vec3 pts[4] = {foot + o - u * w, foot + o + u * w, top + o + u * w, top + o - u * w};
            RVert q[4];
            for (int i = 0; i < 4; i++) { toView(pts[i], q[i]); q[i].shade = outShade(n); q[i].u = pts[i].x + pts[i].z * 0.31; q[i].v = pts[i].y + pts[i].z * 0.17; }
            f.shapes.polygon(q, 4, prm);
        }
        const double turn = realTime * (radar.on ? 1.2 : 0.35), tilt = 30 * DEG;
        const Vec3 face(std::sin(turn) * std::cos(tilt), std::sin(tilt), std::cos(turn) * std::cos(tilt)), u = normalize(cross(Vec3(0, 1, 0), face)), v = cross(face, u);
        const Vec3 mid = top + face * 0.2;
        const Vec3 n = dot(face, eye - mid) >= 0 ? face : face * -1.0;
        const int SEG = 10;
        for (int k = 0; k < SEG; k++) {
            const double a0 = k * TAU / SEG, a1 = (k + 1) * TAU / SEG;
            const Vec3 pts[3] = {mid, mid + (u * std::cos(a0) + v * std::sin(a0)) * M.radius - face * 0.18, mid + (u * std::cos(a1) + v * std::sin(a1)) * M.radius - face * 0.18};
            RVert q[3];
            for (int i = 0; i < 3; i++) { toView(pts[i], q[i]); q[i].shade = outShade(n) * (0.85 + 0.15 * std::cos(a0)); q[i].u = pts[i].x; q[i].v = pts[i].y; }
            f.shapes.polygon(q, 3, prm);
        }
        RVert a, b;
        toView(mid, a); toView(mid + face * 0.7, b);
        a.shade = b.shade = outShade(n);
        f.shapes.line(a, b, prm.bank, true, 1);
    };
    // a piece: its params and its vertices' shades by its material (the lamps' light at its vertices, the world's, the outside's sun)
    auto piece = [&](const ShipPiece& pc) {
        if (pc.kind == PIECE_RACK) { drawDroneRack(f); return; }   // the drone bay in the hangar (game/drone_draw.cpp)
        if (pc.kind == PIECE_MAST) { mast(); return; }
        if (pc.kind >= PIECE_SHELF) { drawLifePiece(f, pc, draw); return; }   // I-04: the explorer's things (game/cabin_life.cpp)
        if (pc.cull && dot(pc.nrm, eye - mesh.verts[pc.first].p) < 0) return;
        const RasterParams* prm = &fin[pc.finish];
        enum { LIT, FIXED } how = LIT;
        double shade = 0, scale = 1;
        if (pc.mat == MAT_SCREEN) {
            const int b = f.banks.bankOf(devices.row[pc.device].bank);
            rs.bank = b >= 0 ? b : BANK_HULL; prm = &rs;
            shade = f.screen[pc.device] * pc.a * (b >= 0 ? 1.0 : 0.45); how = FIXED;
        } else if (pc.mat == MAT_PANE) { prm = &pane; how = FIXED; }
        else if (pc.mat == MAT_OUTSIDE || pc.mat == MAT_FEATURE) {
            if (pc.mat == MAT_OUTSIDE && c.depolarised) return;
            shade = outShade(pc.nrm) * pc.a * pc.b; how = FIXED;
        } else if (pc.mat == MAT_GLOW) { prm = &light; shade = std::min(63.0, 20 + 14 * f.pulse + 26 * f.drive); how = FIXED; }   // I-04: rising in a flight
        else if (pc.mat == MAT_PLAIN) prm = &plain[pc.finish];
        else if (pc.mat == MAT_TINT) { prm = &tinted; scale = 0.5; }
        else if (pc.mat == MAT_HULL && c.depolarised) { prm = &glass; scale = -1; }
        double world = pc.room < SHIP_ROOMS && how == LIT && f.glassToWorld[pc.room] > 0 && pc.kind != PIECE_LINE
                           ? WORLD_SHADES * f.worldLight * f.glassToWorld[pc.room] * std::max(0.0, dot(pc.nrm, f.world)) : 0;
        if (flood && how == LIT && pc.kind != PIECE_LINE) world += SUN_FLOOD * f.sunLight * std::max(0.0, dot(pc.nrm, f.sun));
        RVert q[16];
        for (int i = 0; i < pc.n && i < 16; i++) {
            const ShipVert& v = mesh.verts[pc.first + i];
            toView(v.p, q[i]);
            if (how == FIXED) q[i].shade = shade;
            else if (scale < 0) q[i].shade = GLASS_SHEEN * lit * std::min(1.5, v.lamp) * pc.a * pc.b;   // the glass's sheen of the lamps' light
            else q[i].shade = (SHADE_FULL * (AMBIENT + lit * v.lamp + (1 - lit) * SHIP_NIGHT_GLOW * v.night) + world) * pc.a * pc.b * scale;
            q[i].u = v.u;
            q[i].v = pc.mat == MAT_SCREEN ? v.p.y * 3 + realTime * 0.4 : v.v;
        }
        if (pc.kind == PIECE_LINE) {   // a rail, a bar, a rib: a thin edge at a share of a face's light
            if (how == LIT) { q[0].shade *= 0.6; q[1].shade *= 0.6; }
            f.shapes.line(q[0], q[1], prm->bank, true, 1);
        } else f.shapes.polygon(q, std::min(pc.n, 16), *prm);
    };
    if (f.sight.seesOut && f.sight.ox1 > f.sight.ox0) {
        if (harness.shipWhole) f.shapes.noClip(); else f.shapes.clip(f.sight.ox0, f.sight.oy0, f.sight.ox1, f.sight.oy1);
        for (int pi = mesh.first[SHIP_OUTSIDE]; pi < mesh.first[SHIP_OUTSIDE + 1]; pi++) piece(mesh.pieces[pi]);
    }
    int order[SHIP_ROOMS], n = 0;
    for (int depth = 0; depth <= SHIP_SIGHT_DEPTH; depth++)
        for (int r = 0; r < SHIP_ROOMS; r++) if (f.sight.seen[r] && f.sight.depth[r] == depth) order[n++] = r;
    f.patches.clear();
    for (int oi = 0; oi < n; oi++) {
        const int r = order[oi];
        if (harness.shipWhole) f.shapes.noClip(); else f.shapes.clip(f.sight.x0[r], f.sight.y0[r], f.sight.x1[r], f.sight.y1[r]);
        for (int pi = mesh.first[r]; pi < mesh.first[r + 1]; pi++) piece(mesh.pieces[pi]);
        const size_t from = f.patches.size();   // the sun's patches through its glass, on its floors, walls and boxes' tops
        if (!flood) shipPatches(r, f.sun, f.sunLight, f.patches);
        for (size_t k = from; k < f.patches.size(); k++) {
            const ShipPatch& p = f.patches[k];
            RVert q[16];
            for (int i = 0; i < p.n; i++) { toView(p.p[i], q[i]); q[i].shade = p.shade; }
            f.shapes.polygon(q, p.n, patch);
        }
    }
}

// the faced device's line under the crosshair, in its computer's own colour (one with no screen, the cage, in amber)
void Game::renderCabinHUD() {
    if (cabin.facing <= FACE_NOTHING || cabin.facing >= FACE_COUNT) return;
    const CabinDevice& d = cabinDevices().row[cabin.facing];
    const char* now = d.hudNow ? d.hudNow(*this) : nullptr;
    const RGB& c = d.col;
    drawTextCentered(canvas, UW / 2, UH / 2 + 12, now ? now : d.hud, d.bank == BANK_LIGHT ? HUD_AMBER : rgb((int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255)));
}

// ---------------------------------------------------------------------------
// the front computer (M2-02): three pages of the ship's commands
// ---------------------------------------------------------------------------

// U-03: the flight computer's pages and their rows. A row's act runs at the window (SPACE) unless the row stays on the screen
const RowList<ShipPage>& Game::shipPages() {
    static constexpr auto inSystem = [](const Game& g) { return g.sys.valid && !g.sys.bodies.empty() && g.ship.mode != ShipState::VIMANA; };
    static constexpr ShipRow FLIGHT[] = {
        {"REMOTE TARGET: AIM AT A STAR", [](Game& g) {
             if (g.ship.mode != ShipState::VIMANA) { g.ship.targeting = true; g.targetCycle = -1; g.status("AIM: N NEXT NEAREST STAR, ENTER LOCKS, R CANCELS", 4); }
         }},
        {"VIMANA FLIGHT TO THE REMOTE TARGET", [](Game& g) { g.toggleVimana(); }},
        {"LOCAL TARGET: NEXT BODY", [](Game& g) {
             if (!inSystem(g)) return;
             g.ship.localTarget = (g.ship.localTarget + 1) % (int)g.sys.bodies.size();
             g.ship.targetBelt = -1;
             g.status(fmt("LOCAL TARGET: %s", upper(g.bodyNameOf(g.ship.localTarget)).c_str()), 4);
         }},
        {"SOLAR SYSTEM ANALYZER", [](Game& g) {
             if (inSystem(g)) { g.listSel = std::max(0, g.ship.localTarget); g.returnState = GameState::SPACE; g.openScreen(GameState::SYSTEM_LIST); }
         }},
        {"FINE APPROACH TO THE LOCAL TARGET", [](Game& g) { if (inSystem(g) && g.ship.localTarget >= 0) g.startApproach(g.ship.localTarget); }},
        {"CENTRE SHIP ON LOCAL TARGET", [](Game& g) {
             if (!inSystem(g) || g.ship.localTarget < 0) return;
             Vec3 fwd = normalize(g.sys.bodyPos(g.ship.localTarget, g.t) - g.ship.pos);
             g.ship.yaw = std::atan2(fwd.x, fwd.z);
             g.ship.pitch = std::asin(clampd(fwd.y, -1, 1));
             g.cabin.yaw = 0;
             g.cabin.pitch = 0;
         }},
        {"ORBIT / FIXED POINT CHASE", [](Game& g) {
             if (g.ship.mode == ShipState::PARKED) { g.ship.orbiting = !g.ship.orbiting; g.status(g.ship.orbiting ? "SYNCHRONOUS ORBIT" : "FIXED POINT CHASE", 3); }
         }},
        {"DEPLOY THE CAPSULE OR A PROBE", [](Game& g) { g.deployCapsule(); }},   // X-01: a probe at a giant
        {"TARGET HOME STAR", [](Game& g) { g.targetHome(); }},
        {"TARGET PREVIOUS STAR", [](Game& g) { g.targetPreviousStar(); }},
        {"VIMANA JUMP AHEAD", [](Game& g) {   // R-407: the aim along the nose (last, so the rows kept their places)
             if (g.ship.mode == ShipState::VIMANA) return;
             g.ship.targeting = true;
             g.targetCycle = -1;
             g.jumpDigits.clear();
             g.cabin.yaw = 0;
             g.cabin.pitch = 0;
             g.status("JUMP: TURN SHIP, TYPE LIGHT YEARS, ENTER", 6);
         }},
    };
    static constexpr ShipRow DEVICES[] = {   // C-07: the radar; W-01: the almanac; C-11: the case; C-18: the drones
        {"CABIN LIGHT", [](Game& g) { g.cabin.light = !g.cabin.light; }, [](const Game& g) -> std::string { return g.cabin.light ? "ON" : "OFF"; }, true},
        {"HULL POLARISATION", [](Game& g) { g.cabin.depolarised = !g.cabin.depolarised; },
         [](const Game& g) -> std::string { return g.cabin.depolarised ? "GLASS" : "OPAQUE"; }, true},
        {"OBSERVATORY LIFT", [](Game& g) {   // I-02: the plan's lift, up from any room to the observatory, down to the bridge
             const ShipOpening& l = shipLifter();
             g.cabin.placeAt(g.cabin.room == l.to ? shipPlaceOf(l.from, l.down.x, l.down.z) : shipPlaceOf(l.to, l.up.x, l.up.z));
         },
         [](const Game& g) -> std::string { return g.cabin.room == shipLifter().to ? "UP" : "DOWN"; }},
        {"FIELD AMPLIFICATOR", [](Game& g) { g.fieldAmp = !g.fieldAmp; }, [](const Game& g) -> std::string { return g.fieldAmp ? "ON" : "OFF"; }, true},
        {"TIME WARP", [](Game& g) { g.timeWarp = g.timeWarp >= 10000 ? 1 : g.timeWarp * 10; g.status(fmt("TIME WARP X%.0f", g.timeWarp), 2); },
         [](const Game& g) { return fmt("X%.0f", g.timeWarp); }, true},
        {"SCANLINES", [](Game& g) { g.settings.scanlines = !g.settings.scanlines; g.settings.save(g.settingsPath); },
         [](const Game& g) -> std::string { return g.settings.scanlines ? "ON" : "OFF"; }, true},
        {"SIGNAL RADAR", [](Game& g) { g.radarToggle(); }, [](const Game& g) -> std::string { return g.radar.on ? "ON" : "OFF"; }},   // at the window, where the sweep is
        {"ALMANAC", [](Game& g) { g.openScreen(GameState::ALMANAC); },   // from the parking: Esc returns here, a run goes to the window
         [](const Game& g) -> std::string { return g.almanacRunning ? "RUNNING" : ""; }, true},
        {"TELESCOPE", [](Game& g) { g.telescopeToggle(); },   // at the window, the eye at the eyepiece
         [](const Game& g) { return g.tele.on ? fmt("X%.0f", g.telescopeZoom()) : std::string("STOWED"); }},
        {"INSTRUMENTS", [](Game& g) { g.openScreen(GameState::INSTRUMENTS); }, [](const Game& g) { return fmt("%d", (int)g.guide.instruments.size()); }},   // in the case
        {"THE DRONE BAY", [](Game& g) { g.openScreen(GameState::DRONES); }, [](const Game& g) { return std::string(g.bay.out ? "AWAY" : "READY"); }},
    };
    static constexpr ShipRow PREFERENCES[] = {   // C-06: the shards
        {"SETTINGS", [](Game& g) { g.returnState = GameState::SPACE; g.openScreen(GameState::SETTINGS); }},
        {"GUIDE", [](Game& g) { g.openScreen(GameState::GUIDE); }},
        {"EXPEDITION LOG", [](Game& g) { g.returnState = GameState::SPACE; g.openScreen(GameState::LOG); }},
        {"SHARDS", [](Game& g) { g.openScreen(GameState::SHARDS); }},
        {"SAVE THE EXPEDITION", [](Game& g) { g.saveSlot(g.currentSlot); }},
    };
    static constexpr ShipPage PAGES[] = {{"FLIGHT CONTROL", rowsOf(FLIGHT)}, {"ONBOARD DEVICES", rowsOf(DEVICES)}, {"PREFERENCES", rowsOf(PREFERENCES)}};
    static constexpr RowList<ShipPage> LIST = rowsOf(PAGES);
    static_assert(rowsWhole(LIST), "each page its name and rows, each row its label and its act");
    return LIST;
}

// the flight computer: Left and Right turn its pages, Enter does the row's act
void Game::updateShipScreen(const Input& in, double dt, double realDt) {
    const RowList<ShipPage>& pages = shipPages();
    if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_TAB)) { shipScreenPage = (shipScreenPage + pages.count - 1) % pages.count; shipScreenSel = 0; }
    if (in.wasPressed(KEY_RIGHT)) { shipScreenPage = (shipScreenPage + 1) % pages.count; shipScreenSel = 0; }
    const RowList<ShipRow>& rows = pages.row[shipScreenPage].rows;
    menuStep(in, shipScreenSel, rows.count);
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_E)) { closeScreen(); return; }
    if (!enterKey(in)) return;
    const bool has = shipScreenSel >= 0 && shipScreenSel < rows.count;
    if (!has || !rows.row[shipScreenSel].stays) closeScreen();
    if (has) rows.row[shipScreenSel].act(*this);
}

// U-05: the flight computer opens on its first page's first row
void Game::shipScreenEnter() { shipScreenPage = 0; shipScreenSel = 0; audio.beep = BEEP_UI; }

// U-04: the flight computer's commands (the controller's first page)
void Game::shipScreenPad(PadMenu& m) const {
    m.title = "FLIGHT COMPUTER"; m.item("NEXT PAGE", KEY_RIGHT); m.item("PREVIOUS PAGE", KEY_LEFT); m.item("CLOSE", KEY_ESCAPE);
}

void Game::renderShipScreen() {
    const RowList<ShipPage>& pages = shipPages();
    const int top = 22, bottom = UH - 22;
    drawPanel(canvas, 24, top, UW - 24, bottom, rgb(0, 12, 6), 225);
    for (int p = 0; p < pages.count; p++) drawText(canvas, 34 + p * 96, top + 6, pages.row[p].name, p == shipScreenPage ? HUD_WHITE : HUD_DIM);
    const RowList<ShipRow>& rows = pages.row[shipScreenPage].rows;
    for (int i = 0; i < rows.count; i++) {
        int y = top + 22 + i * 10;
        const ShipRow& r = rows.row[i];
        if (r.status) { const std::string st = r.status(*this); drawText(canvas, UW - 44 - textWidth(st.c_str()), y, st.c_str(), HUD_AMBER); }
        drawText(canvas, 44, y, r.label, i == shipScreenSel ? HUD_WHITE : HUD_GREEN);
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

void Game::consoleEnter() {
    if (consoleLines.empty()) { consolePrint("GOES CONSOLE READY. TYPE HELP."); }
    consoleInput.clear();
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
        consolePrint("DL           LIST BODIES IN THIS SYSTEM");
        consolePrint("PAR NAME     SECTOR COORDINATES OF A STAR");
        consolePrint("ST NAME      SET REMOTE TARGET BY NAME");
        consolePrint("WHERE [NAME] POSITION OF SHIP OR STAR");
        consolePrint("CAST TEXT    WRITE A LOG NOTE");
        consolePrint("CAT          READ RECENT NOTES");
        consolePrint("HOME / PREV  TARGET HOME OR PREVIOUS STAR");
        consolePrint("JUMP LY      VIMANA JUMP ALONG SHIP'S NOSE");
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
            consolePrint(fmt("  %s%-16s %s", sys.bodies[i].parent >= 0 ? "  " : "", upper(bodyNameOf(i)).c_str(), kindNameNow(i).c_str()));
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
        if (rest.empty()) { consolePrint("CAST NEEDS TEXT"); return; }
        logEvent("NOTE", rest);
        consolePrint("NOTED.");
    } else if (cmd == "CAT") {
        int shown = 0;
        for (int i = (int)guide.log.size() - 1; i >= 0 && shown < 8; i--) if (guide.log[i].kind == "NOTE") { consolePrint("  " + trunc(guide.log[i].text, 48)); shown++; }
        if (!shown) consolePrint("NO NOTES YET");
    } else if (cmd == "HOME") { targetHome(); consolePrint(statusMsg); }
    else if (cmd == "PREV") { targetPreviousStar(); consolePrint(statusMsg); }
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
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    if (in.wasPressed(KEY_ENTER)) { std::string cmd = consoleInput; consoleInput.clear(); consoleCommand(cmd); }
}

// U-04: the console's commands (the controller's first page): its keyboard, its commands typed, and what the keyboard's buttons do
void Game::consolePad(PadMenu& m) const {
    m.title = "GOES CONSOLE";
    m.typing = "A TYPES  X DELETES  START RUNS  SELECT COMMANDS";
    m.item("KEYBOARD", -1, 0, PSP_KEYBOARD);
    m.typed("SL: NEARBY STARS", "SL\n"); m.typed("DL: THIS SYSTEM'S BODIES", "DL\n"); m.typed("WHERE: THE SHIP'S SECTOR", "WHERE\n");
    m.typed("HOME: TARGET HOME STAR", "HOME\n"); m.typed("PREV: TARGET PREVIOUS STAR", "PREV\n"); m.typed("CAT: RECENT NOTES", "CAT\n");
    m.typed("HELP: COMMANDS", "HELP\n");
    m.item("CLOSE", KEY_ESCAPE);
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
    if (ship.mode != ShipState::PARKED) { refuse("NOT IN ORBIT - APPROACH A PLANET FIRST"); }
    else if (ship.parkedBelt >= 0) { refuse("BELT ROCKS ARE TOO SMALL TO LAND ON"); }
    else if (isProbeGiant(sys.bodies[ship.parkedBody].type)) { if (probe.active) watchProbe(); else launchProbe(); }   // X-01: a giant takes a probe (a probe already sent: its screen)
    else if (!KINDS[sys.bodies[ship.parkedBody].type].landable) { refuse("SURFACE NOT CONSISTENT - LANDING IMPOSSIBLE"); }
    else openScreen(GameState::LANDING_MAP);
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
