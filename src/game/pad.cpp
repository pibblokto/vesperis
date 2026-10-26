// R-418: the controller. The platform reads it whole (core/input.h's PadState: the standard layout when the system knows the device, the
// raw buttons, axes and hats always) and `PadMap` binds its controls to sixteen buttons and two sticks by the Xbox names. Here each part of
// the game gives those buttons the keys they press (`padBindings`: the screens, the title, the ship, on foot, the buggy, the drone, the
// probe's camera, a recording, an instrument), the left stick walks (in the screens it moves the arrows) and the right stick looks as the
// mouse does. Y opens the command menu: every action of the screen in words, run as its keys, a page of the general ones and a page of
// every key; an on-screen keyboard types where the game wants letters; the CONTROLS screen's second page binds the buttons. Whatever the
// controller does goes through the game's own keys: one code path, and nothing a key does is out of a controller's reach
#include "game.h"
#include "peoples/instruments.h"
#include "ui.h"
#include <algorithm>
#include <cmath>

namespace {
enum PadSpecial { PSP_NONE, PSP_KEYBOARD, PSP_CONTROLLER, PSP_TYPE, PSP_TWICE, PSP_AUTO };   // PSP_AUTO auto-explore's profiles
const char* const ROLE_SHORT[PR_BUTTONS] = {"A", "B", "X", "Y", "LB", "RB", "LT", "RT", "SELECT", "START", "L3", "R3", "UP", "DOWN", "LEFT", "RIGHT"};
const char* const ROLE_SCREEN[PR_BUTTONS] = {
        "A  BOTTOM", "B  RIGHT", "X  LEFT", "Y  TOP", "LB", "RB", "LT", "RT", "SELECT", "START", "L3", "R3", "D-PAD UP", "D-PAD DOWN", "D-PAD LEFT", "D-PAD RIGHT"};
constexpr double REPEAT_DELAY = 0.4, REPEAT_EVERY = 0.09;   // the arrows held in the menus and the screens
constexpr double CAPTURE_S = 6;                             // the controller page waits this long for a press
constexpr int PAD_ROWS = 22;                                // the controller page: sixteen buttons, two sticks, three settings, the reset

// the keyboard: seven rows of ten units, a key its width in units (key -1 latches Shift, -2 Ctrl, -3 Alt for the next key)
struct KbKey { const char* label; int key; int w; };
const std::vector<std::vector<KbKey>>& kbRows() {
    static const std::vector<std::vector<KbKey>> rows = {
        {{"A", KEY_A, 1}, {"B", KEY_B, 1}, {"C", KEY_C, 1}, {"D", KEY_D, 1}, {"E", KEY_E, 1}, {"F", KEY_F, 1}, {"G", KEY_G, 1}, {"H", KEY_H, 1}, {"I", KEY_I, 1}, {"J", KEY_J, 1}},
        {{"K", KEY_K, 1}, {"L", KEY_L, 1}, {"M", KEY_M, 1}, {"N", KEY_N, 1}, {"O", KEY_O, 1}, {"P", KEY_P, 1}, {"Q", KEY_Q, 1}, {"R", KEY_R, 1}, {"S", KEY_S, 1}, {"T", KEY_T, 1}},
        {{"U", KEY_U, 1}, {"V", KEY_V, 1}, {"W", KEY_W, 1}, {"X", KEY_X, 1}, {"Y", KEY_Y, 1}, {"Z", KEY_Z, 1}, {"-", KEY_MINUS, 1}, {".", KEY_PERIOD, 1}, {"/", KEY_SLASH, 1}, {"=", KEY_EQUAL, 1}},
        {{"1", KEY_1, 1}, {"2", KEY_2, 1}, {"3", KEY_3, 1}, {"4", KEY_4, 1}, {"5", KEY_5, 1}, {"6", KEY_6, 1}, {"7", KEY_7, 1}, {"8", KEY_8, 1}, {"9", KEY_9, 1}, {"0", KEY_0, 1}},
        {{"SPACE", KEY_SPACE, 3}, {"BKSP", KEY_BACKSPACE, 2}, {"DEL", KEY_DELETE, 2}, {"ENTER", KEY_ENTER, 3}},
        {{"SHIFT", -1, 2}, {"CTRL", -2, 2}, {"ALT", -3, 2}, {"TAB", KEY_TAB, 2}, {"ESC", KEY_ESCAPE, 2}},
        {{"PGUP", KEY_PAGE_UP, 3}, {"PGDN", KEY_PAGE_DOWN, 3}, {"HOME", KEY_HOME, 2}, {"END", KEY_END, 2}}};
    return rows;
}
int kbCentre(int row, int col) { int u = 0; for (int i = 0; i < col; i++) u += kbRows()[row][i].w; return 2 * u + kbRows()[row][col].w; }   // in half units
int kbAt(int row, int centre) {   // the key of a row under a place (half units)
    int u = 0;
    for (int i = 0; i < (int)kbRows()[row].size(); i++) { u += kbRows()[row][i].w; if (centre < 2 * u) return i; }
    return (int)kbRows()[row].size() - 1;
}
int charKey(char c) {
    if (c >= 'A' && c <= 'Z') return KEY_A + (c - 'A');
    if (c >= 'a' && c <= 'z') return KEY_A + (c - 'a');
    if (c >= '0' && c <= '9') return KEY_0 + (c - '0');
    if (c == ' ') return KEY_SPACE;
    if (c == '-') return KEY_MINUS;
    if (c == '.') return KEY_PERIOD;
    if (c == '\n') return KEY_ENTER;
    return -1;
}
}

int Game::padContext() const {
    const int ctx = statePad(state);   // the state's own (its row); on the ground and at the case, what the explorer does there
    if (ctx == PCX_FOOT) return !surf.valid ? PCX_SCREEN : (photoMode ? PCX_FOOT : (surf.inBuggy ? PCX_BUGGY : (surf.inDrone ? PCX_DRONE : PCX_FOOT)));
    if (ctx == PCX_INSTRUMENT) return icase.level == 1 && icase.valid && !instrumentPlaysDisc(icase.inst) ? PCX_INSTRUMENT : PCX_SCREEN;
    return ctx;
}

std::string Game::padContextName(int ctx) const {
    switch (ctx) {
        case PCX_TITLE: return "VESPERIS";
        case PCX_SHIP: return tele.on ? "THE TELESCOPE" : (radar.on ? "THE RADAR" : (ship.targeting ? "THE AIM" : (settings.cabin ? "THE CABIN" : "THE SHIP")));
        case PCX_FOOT: return photoMode ? "PHOTO MODE" : "ON FOOT";
        case PCX_BUGGY: return "THE BUGGY";
        case PCX_DRONE: return "THE DRONE";
        case PCX_PROBE: return "THE PROBE";
        case PCX_RECORDING: return "THE RECORDING";
        case PCX_INSTRUMENT: return "THE INSTRUMENT";
    }
    switch (state) {
        case GameState::LANDING_MAP: return "LANDING MAP";
        case GameState::SECTOR_MAP: return "SECTOR MAP";
        case GameState::STAR_MAP: return mapPage == 1 ? "THE GALAXY" : "STAR MAP";
        case GameState::SHARDS: return "THE DECODER";
        case GameState::INSTRUMENTS: return "THE CASE";
        case GameState::DRONES: return "THE DRONE BAY";
        case GameState::ALMANAC: return "THE ALMANAC";
        case GameState::DATA: return "DATA SHEET";
        case GameState::SYSTEM_LIST: return "THE ANALYZER";
        case GameState::SHIPSCREEN: return "FLIGHT COMPUTER";
        case GameState::CONSOLE: return "GOES CONSOLE";
        case GameState::TEXT_ENTRY: return "THE ENTRY";
        case GameState::HELP: return "HELP";
        case GameState::DESCENT: case GameState::ASCENT: return "THE CAPSULE";
        case GameState::AUTO: return "AUTO-EXPLORE";
        default: return "COMMANDS";
    }
}

// the buttons' keys in a context (the d-pad is the arrows, Y the command menu and the sticks walking and looking everywhere)
void Game::padBindings(int ctx, std::vector<PadBind>& out) const {
    out.clear();
    auto add = [&](int role, int key, const char* label, int mods = 0) { PadBind b; b.role = role; b.key = key; b.mods = mods; b.label = label; out.push_back(b); };
    switch (ctx) {
        case PCX_TITLE:   // START begins, as a title's does; B quits nothing (the menu's QUIT does)
            add(PR_A, KEY_ENTER, hasSave ? "CONTINUE" : "BEGIN"); add(PR_START, KEY_ENTER, hasSave ? "CONTINUE" : "BEGIN");
            break;
        case PCX_SHIP: {
            const bool aim = ship.targeting;
            add(PR_A, KEY_ENTER, tele.on ? "TRACK" : (radar.on ? "FLY TO THE LOCK" : (aim ? "LOCK THE STAR" : "APPROACH")));
            if (aim || radar.on || tele.on) add(PR_B, KEY_ESCAPE, aim ? "CANCEL" : (radar.on ? "LEAVE THE RADAR" : "STOW"));
            add(PR_X, KEY_E, "USE");
            add(PR_LB, KEY_PAGE_UP, tele.on ? "ZOOM IN" : (radar.on ? "GAIN UP" : "ROOF DECK"));
            add(PR_RB, KEY_PAGE_DOWN, tele.on ? "ZOOM OUT" : (radar.on ? "GAIN DOWN" : "CABIN"));
            add(PR_LT, aim ? KEY_N : KEY_L, aim ? "NEXT STAR" : "TARGET");
            add(PR_RT, KEY_LEFT_SHIFT, "ROUND THE WORLD");
            add(PR_L3, KEY_R, "AIM");
            add(PR_R3, KEY_X, "CENTRE");
            add(PR_SELECT, KEY_TAB, "ANALYZER");
            add(PR_START, KEY_ESCAPE, "MENU");
            break;
        }
        case PCX_FOOT: {
            if (photoMode) { add(PR_A, KEY_SPACE, "UP"); add(PR_B, KEY_C, "DOWN"); add(PR_RT, KEY_LEFT_SHIFT, "FAST"); add(PR_START, KEY_ESCAPE, "MENU"); break; }
            const Player& p = surf.player;
            add(PR_A, KEY_SPACE, "JUMP");
            if (p.swimming) add(PR_B, KEY_LEFT_CONTROL, "DIVE");                                  // Ctrl: under the water
            else if (!p.onGround && surf.site.gravity < 1.0) add(PR_B, KEY_LEFT_CONTROL, "DOWN");   // and down on the lightest worlds (O4)
            else add(PR_B, KEY_C, "CROUCH");
            add(PR_X, KEY_E, "USE");
            add(PR_LB, KEY_N, "MAP");
            add(PR_RB, KEY_R, "SCANNER");
            add(PR_LT, KEY_Z, "STAND TALL");
            add(PR_RT, KEY_LEFT_SHIFT, "SPRINT");
            add(PR_L3, KEY_LEFT_SHIFT, "SPRINT");
            add(PR_R3, KEY_V, "VISION");
            add(PR_SELECT, KEY_TAB, "READINGS");
            add(PR_START, KEY_ESCAPE, "MENU");
            break;
        }
        case PCX_BUGGY:
            add(PR_A, KEY_SPACE, "BRAKE"); add(PR_B, KEY_V, "VIEW"); add(PR_X, KEY_E, "GET OUT");
            add(PR_LB, KEY_N, "MAP"); add(PR_RB, KEY_R, "SCANNER");
            add(PR_LT, KEY_S, "REVERSE"); add(PR_RT, KEY_W, "THROTTLE");
            add(PR_R3, KEY_V, "VIEW"); add(PR_SELECT, KEY_TAB, "READINGS"); add(PR_START, KEY_ESCAPE, "MENU");
            break;
        case PCX_DRONE:
            add(PR_A, KEY_SPACE, "CLIMB"); add(PR_B, KEY_LEFT_SHIFT, "DESCEND"); add(PR_X, KEY_E, "GET OUT");
            add(PR_LB, KEY_N, "MAP"); add(PR_RB, KEY_R, "SCANNER");
            add(PR_LT, KEY_LEFT_SHIFT, "DESCEND"); add(PR_RT, KEY_SPACE, "CLIMB");
            add(PR_R3, KEY_V, "VIEW"); add(PR_SELECT, KEY_TAB, "READINGS"); add(PR_START, KEY_ESCAPE, "MENU");
            break;
        case PCX_PROBE: {
            const bool ended = probe.active && probe.endClock >= 0;
            add(PR_A, ended ? KEY_ENTER : KEY_SPACE, ended ? "LET IT GO" : "CHUTE");
            add(PR_B, KEY_ESCAPE, "CABIN"); add(PR_X, KEY_X, "CENTRE"); add(PR_RT, KEY_LEFT_SHIFT, "FAST"); add(PR_START, KEY_ESCAPE, "CABIN");
            break;
        }
        case PCX_RECORDING:
            add(PR_A, KEY_SPACE, "PLAY/PAUSE"); add(PR_B, KEY_ESCAPE, "GALLERY"); add(PR_X, KEY_X, "CENTRE");
            add(PR_LB, KEY_PAGE_UP, "LAYER UP"); add(PR_RB, KEY_PAGE_DOWN, "LAYER DOWN"); add(PR_RT, KEY_LEFT_SHIFT, "FAST"); add(PR_START, KEY_ESCAPE, "GALLERY");
            break;
        case PCX_INSTRUMENT:   // the scale's first eight degrees (the menu has the ninth and the tenth, and the d-pad the register and the next)
            add(PR_A, KEY_1, "1"); add(PR_X, KEY_2, "2"); add(PR_LB, KEY_3, "3"); add(PR_RB, KEY_4, "4");
            add(PR_LT, KEY_5, "5"); add(PR_RT, KEY_6, "6"); add(PR_L3, KEY_7, "7"); add(PR_R3, KEY_8, "8");
            add(PR_B, KEY_ESCAPE, "BACK"); add(PR_START, KEY_ESCAPE, "BACK");
            break;
        default:
            // the maps: only the buttons that do something there, in their words (the cursors: the d-pad, the left stick, the right stick)
            if (state == GameState::LANDING_MAP) {
                add(PR_A, KEY_ENTER, "LAND"); add(PR_B, KEY_ESCAPE, "BACK");
                if (landZoom) add(PR_LT, KEY_Z, "THE WORLD"); else add(PR_RT, KEY_Z, "ZOOM IN");   // its zoom is Z, in and out
                add(PR_L3, KEY_LEFT_SHIFT, "BIG STEPS"); add(PR_START, KEY_ESCAPE, "BACK");
                break;
            }
            if (state == GameState::SECTOR_MAP) {
                add(PR_A, KEY_ENTER, "MARK"); add(PR_B, KEY_ESCAPE, "CLOSE"); add(PR_X, KEY_SPACE, "BACK TO YOU");
                add(PR_LT, KEY_MINUS, "ZOOM OUT"); add(PR_RT, KEY_EQUAL, "ZOOM IN"); add(PR_L3, KEY_LEFT_SHIFT, "FASTER");
                add(PR_SELECT, KEY_TAB, "NEXT MARK"); add(PR_START, KEY_ESCAPE, "CLOSE");
                break;
            }
            if (state == GameState::STAR_MAP && mapPage == 1) {   // the galaxy chart
                add(PR_A, KEY_ENTER, chartPlotted() ? "FLY" : "PLOT"); add(PR_B, KEY_ESCAPE, "CLOSE"); add(PR_X, KEY_SPACE, "THE SHIP");
                add(PR_LB, KEY_PAGE_UP, "HIGHER"); add(PR_RB, KEY_PAGE_DOWN, "LOWER");
                add(PR_LT, KEY_MINUS, "ZOOM OUT"); add(PR_RT, KEY_EQUAL, "ZOOM IN"); add(PR_L3, KEY_LEFT_SHIFT, "FASTER");
                add(PR_SELECT, KEY_TAB, chartPlotted() ? "LOOK THERE" : "NEIGHBOURHOOD"); add(PR_START, KEY_ESCAPE, "CLOSE");
                break;
            }
            if (state == GameState::STAR_MAP) {
                add(PR_A, KEY_ENTER, gchart.look ? "TAKE THIS STAR" : "TARGET"); add(PR_B, KEY_ESCAPE, "CLOSE");
                add(PR_LT, KEY_MINUS, "ZOOM OUT"); add(PR_RT, KEY_EQUAL, "ZOOM IN");
                add(PR_SELECT, KEY_TAB, gchart.look ? "THE CHART" : "THE GALAXY"); add(PR_START, KEY_ESCAPE, "CLOSE");
                break;
            }
            add(PR_A, KEY_ENTER, "ENTER"); add(PR_B, KEY_ESCAPE, "BACK"); add(PR_X, KEY_SPACE, "SPACE");
            add(PR_LB, KEY_PAGE_UP, "PAGE UP"); add(PR_RB, KEY_PAGE_DOWN, "PAGE DOWN");
            add(PR_LT, KEY_MINUS, "ZOOM OUT"); add(PR_RT, KEY_EQUAL, "ZOOM IN");
            add(PR_L3, KEY_LEFT_SHIFT, "SHIFT"); add(PR_SELECT, KEY_TAB, "TAB"); add(PR_START, KEY_ESCAPE, "BACK");
            break;
    }
}

// the context's buttons in words, wrapped to `width` characters (lines by '\n')
std::string Game::padLegend(int ctx, int width) const {
    std::vector<PadBind> binds; padBindings(ctx, binds);
    std::vector<std::string> parts;
    for (const PadBind& b : binds) if (b.role != PR_START) parts.push_back(std::string(ROLE_SHORT[b.role]) + " " + b.label);
    if (ctx == PCX_SCREEN && (state == GameState::LANDING_MAP || state == GameState::SECTOR_MAP || state == GameState::STAR_MAP))   // what the sticks do on a map
        parts.push_back(state == GameState::STAR_MAP && mapPage == 0 ? "STICKS TURN IT" : "STICKS MOVE THE CURSOR");
    parts.push_back("Y COMMANDS");
    std::string out, line;
    for (const std::string& p : parts) {
        if (!line.empty() && (int)(line.size() + 2 + p.size()) > width) { out += line + "\n"; line.clear(); }
        line += (line.empty() ? "" : "  ") + p;
    }
    return out + line;
}

// the command menu's pages: 0 the context's actions, 1 the general ones (the keys page is the keyboard). Each runs as the keys it names
std::vector<Game::PadItem> Game::padItems(int page) const {
    std::vector<PadItem> v;
    auto item = [&](const std::string& label, int key, int mods = 0, int special = PSP_NONE) { PadItem it; it.label = label; it.key = key; it.mods = mods; it.special = special; v.push_back(it); };
    auto typed = [&](const std::string& label, const std::string& text) { PadItem it; it.label = label; it.special = PSP_TYPE; it.text = text; v.push_back(it); };
    const bool inSystem = sys.valid && !sys.bodies.empty() && ship.mode != ShipState::VIMANA;
    const bool scene = state == GameState::SPACE || state == GameState::SURFACE;
    auto scanItems = [&]() {
        item(scanMode < 0 ? "THE SCANNER ON" : "THE SCANNER'S NEXT KIND", KEY_R);
        item(scanSkipMarked ? "THE SCANNER: MARKED SIGHTS IN" : "THE SCANNER: MARKED SIGHTS OUT", KEY_R, MOD_SHIFT);
    };
    if (page == 1) {
        if (scene) item("THE GUIDE", KEY_G);
        if (state == GameState::SPACE) item("THE STAR MAP", KEY_M);
        if (scene) item("THE LOG", KEY_J);
        if (state == GameState::SURFACE || (state == GameState::SPACE && inSystem)) item("THE DATA SHEET", KEY_I);
        if (scene || state == GameState::LANDING_MAP) {
            item(almanacRunning ? "STOP THE ALMANAC'S RUN" : fmt("TIME WARP X%.0f TO X%.0f", timeWarp, timeWarp >= 10000 ? 1.0 : timeWarp * 10), KEY_T);
            item("THE ALMANAC", KEY_T, MOD_CTRL);
        }
        if (scene || state == GameState::PROBE || state == GameState::RECORDING) {
            item("PHOTOGRAPH", KEY_P);
            item(photoMode ? "END PHOTO MODE" : "PHOTO MODE", KEY_P, MOD_CTRL);
            if (scene) item("PANORAMA", KEY_P, MOD_CTRL | MOD_SHIFT);
            item(recording ? "STOP RECORDING" : "RECORD THE FRAMES", KEY_R, MOD_CTRL);
        }
        if (scene || state == GameState::DESCENT || state == GameState::ASCENT) item(fmt("SAVE (SLOT %d)", currentSlot), KEY_S, MOD_CTRL);
        if (scene) item(fmt("LOAD (SLOT %d)", currentSlot), KEY_L, MOD_CTRL);
        if (scene || state == GameState::TITLE) item("HELP", KEY_H);
        item(settings.scanlines ? "SCANLINES OFF" : "SCANLINES ON", KEY_K, MOD_CTRL);
        item("FULLSCREEN", KEY_F, MOD_CTRL);
        if (state != GameState::KEYS) item("CONTROLLER SETUP", -1, 0, PSP_CONTROLLER);
        if (scene) item("AUTO-EXPLORE", -1, 0, PSP_AUTO);
        if (scene) item("THE GAME MENU", KEY_ESCAPE);
        return v;
    }
    switch (state) {
        case GameState::TITLE:
            item(hasSave ? "CONTINUE" : "BEGIN THE EXPEDITION", KEY_ENTER);
            if (hasSave) item("NEW EXPEDITION", KEY_N);
            item("HELP", KEY_H); item("CREDITS", KEY_C); item("QUIT", KEY_ESCAPE);
            break;
        case GameState::SPACE: {
            const bool parkedBody = ship.mode == ShipState::PARKED && ship.parkedBody >= 0 && ship.parkedBody < (int)sys.bodies.size() && ship.parkedBelt < 0;
            const bool giant = parkedBody && isProbeGiant(sys.bodies[ship.parkedBody].type);
            if (tele.on) {
                item("ZOOM IN", KEY_PAGE_UP); item("ZOOM OUT", KEY_PAGE_DOWN);
                item(tele.track != TELE_NONE ? "STOP TRACKING" : "TRACK THE TARGET", KEY_ENTER);
                item("NAME THE TARGET", KEY_N); item("PHOTOGRAPH IT", KEY_P);
                if (giant) item("SEND A PROBE THERE", KEY_C);
                item("STOW THE TELESCOPE", KEY_Z);
            } else if (radar.on) {
                item("GAIN UP", KEY_PAGE_UP); item("GAIN DOWN", KEY_PAGE_DOWN);
                if (radar.locked >= 0) item("FLY TO THE SIGNAL", KEY_ENTER);
                item("LEAVE THE RADAR CAMERA", KEY_B);
            } else if (ship.targeting) {
                item("LOCK THE STAR IN THE CROSSHAIR", KEY_ENTER);
                item("THE NEXT NEAREST STAR", KEY_N);
                item("JUMP: TYPE THE LIGHT YEARS", -1, 0, PSP_KEYBOARD);
                item("CANCEL THE AIM", KEY_R);
            } else {
                static const char* const FACING[FACE_COUNT] = {"", "THE GOES CONSOLE", "THE FLIGHT COMPUTER", "THE STAR MAP SCREEN", "THE LANDING SCREEN", "THE TARGET SCREEN", "THE CAPSULE",
                                                  "THE SHARD DECODER", "THE SIGNAL RADAR", "THE TELESCOPE", "THE INSTRUMENT CASE", "THE DRONE BAY"};
                if (settings.cabin && cabin.facing > FACE_NOTHING && cabin.facing < FACE_COUNT) item(std::string("USE ") + FACING[cabin.facing], KEY_E);
                if (ship.mode == ShipState::VIMANA) item("ABORT THE FLIGHT", KEY_V);
                else {
                    item("AIM AT A STAR", KEY_R);
                    if (ship.hasRemote) item("VIMANA FLIGHT TO THE TARGET", KEY_V);
                }
                if (inSystem) { item("LOCAL TARGET", KEY_L); item("THE ANALYZER", KEY_TAB); }
                if (inSystem && (ship.localTarget >= 0 || ship.targetBelt >= 0)) {
                    if (ship.mode != ShipState::APPROACH) item("FINE APPROACH", KEY_ENTER);
                    item("CENTRE ON THE TARGET", KEY_X);
                }
                if (ship.mode == ShipState::PARKED) item(ship.orbiting ? "FIXED POINT CHASE" : "SYNCHRONOUS ORBIT", KEY_O);
                if (parkedBody) item(giant ? (probe.active ? "THE PROBE'S CAMERA" : "SEND A PROBE") : "DEPLOY THE CAPSULE", KEY_C);
                item("THE TELESCOPE", KEY_Z);
                item("THE SIGNAL RADAR", KEY_B);
                item(fieldAmp ? "FIELD AMPLIFICATOR OFF" : "FIELD AMPLIFICATOR ON", KEY_F);
                if (settings.cabin) {
                    item(cabin.light ? "CABIN LIGHT OFF" : "CABIN LIGHT ON", KEY_U);
                    item(cabin.depolarised ? "POLARISE THE HULL" : "GLASS HULL", KEY_Y);
                    if (cabin.onRoof) item("BACK INTO THE CABIN", KEY_PAGE_DOWN); else item("UP TO THE ROOF DECK", KEY_PAGE_UP);
                }
            }
            break;
        }
        case GameState::SURFACE:
            if (!surf.valid) break;
            if (photoMode) { item("END PHOTO MODE", KEY_P, MOD_CTRL); item("PANORAMA", KEY_P, MOD_CTRL | MOD_SHIFT); item("PHOTOGRAPH", KEY_P); break; }
            if (surf.inVehicle()) {
                item(surf.inBuggy ? "GET OUT OF THE BUGGY" : "GET OUT OF THE DRONE", KEY_E);
                item(surf.chaseCam ? "THE NOSE CAMERA" : "THE CHASE VIEW", KEY_V);
            } else {
                std::string use;
                if (surf.buggy.deployed && surf.buggyDist() <= 3.5) use = "GET IN THE BUGGY";
                else if (surf.drone.deployed && surf.droneDist() <= 3.5) use = "GET IN THE DRONE";
                else if (surf.nearInstrument.id >= 0 && (surf.nearShard.index < 0 || surf.nearInstrument.dist < surf.nearShard.dist)) use = "TAKE THE INSTRUMENT";
                else if (surf.nearShard.index >= 0) use = "TAKE THE RECORD";
                else if (surf.env.nearCapsule) use = "BOARD THE CAPSULE";
                if (!use.empty()) item(use, KEY_E);
                item("UNFOLD A BUGGY", KEY_B);
                item("UNFOLD A DRONE", KEY_F);
            }
            item("CALL THE CAPSULE", KEY_K);
            item("THE SECTOR MAP", KEY_N);
            scanItems();
            item("MARK WHERE THE CROSSHAIR RESTS", KEY_L);
            item(surf.hasWaypoint ? "CLEAR THE WAYPOINT" : "WAYPOINT AT THE CROSSHAIR", KEY_M);
            item(readings ? "PUT THE READINGS AWAY" : "THE READINGS", KEY_TAB);
            if (!surf.inVehicle()) {
                item(surf.lamp ? "PUT THE LAMP OUT" : "LIGHT THE LAMP", KEY_U);
                item("VISION MODE", KEY_V);
                item("CREATURES NEARBY", KEY_X);
                const int aw = surf.player.autoWalk, next = aw == 0 ? 3 : (aw < 6 ? 6 : (aw < 9 ? 9 : 0));
                item(next ? fmt("AUTOWALK %d", next) : "STOP THE AUTOWALK", KEY_0 + next);
            }
            break;
        case GameState::LANDING_MAP:
            item("LAND HERE", KEY_ENTER);
            if (landingZoomAllowed()) item(landZoom ? "THE WHOLE WORLD" : "ZOOM INTO THE SECTOR", KEY_Z);
            item("A RANDOM LIT SITE", KEY_R);
            item(mapClouds ? "HIDE THE CLOUDS" : "SHOW THE CLOUDS", KEY_C);
            item("UNDER THE SKY'S BIGGEST BODY", KEY_J);
            item(landPeoples ? "HIDE THE PEOPLES" : "THE PEOPLES", KEY_V);
            item("THE ALMANAC OF THE SITE", KEY_T, MOD_CTRL);
            item("BACK TO THE SHIP", KEY_ESCAPE);
            break;
        case GameState::SECTOR_MAP:
            item("MARK HERE, OR RENAME THE MARK", KEY_ENTER);
            item("SHOW THE MARK OVER THE VIEW", KEY_ENTER, MOD_SHIFT);
            item("REMOVE THE MARK", KEY_DELETE, 0, PSP_TWICE);
            item("WAYPOINT AT THE CURSOR", KEY_M);
            item("THE NEXT MARK", KEY_TAB);
            item("THE CURSOR BACK TO YOU", KEY_SPACE);
            item("ZOOM IN", KEY_EQUAL); item("ZOOM OUT", KEY_MINUS);
            scanItems();
            item("CLOSE THE MAP", KEY_ESCAPE);
            break;
        case GameState::STAR_MAP:
            if (mapPage == 1) {
                item(chartPlotted() ? "FLY THE PLOTTED JUMP" : "PLOT A JUMP TO THE CURSOR", KEY_ENTER);
                item("THE NEXT STOP", KEY_N); item("THE STOP BEFORE", KEY_N, MOD_SHIFT);
                item("MARK THE PLACE", KEY_L);
                item("REMOVE THE MARK", KEY_DELETE, 0, PSP_TWICE);
                item("THE CURSOR BACK TO THE SHIP", KEY_SPACE);
                item("ZOOM IN", KEY_EQUAL); item("ZOOM OUT", KEY_MINUS);
                item(chartPlotted() ? "LOOK AT THE PLOTTED END" : "THE NEIGHBOURHOOD", KEY_TAB);
            } else {
                item(gchart.look ? "TAKE THIS STAR AS THE END" : "TARGET THE STAR", KEY_ENTER);
                item("THE NEXT CLASS", KEY_C); item("THE CLASS BEFORE", KEY_X); item("EVERY CLASS", KEY_0);
                item("ZOOM IN", KEY_EQUAL); item("ZOOM OUT", KEY_MINUS);
                item(gchart.look ? "BACK TO THE CHART" : "THE GALAXY", KEY_TAB);
            }
            item("CLOSE THE MAP", KEY_ESCAPE);
            break;
        case GameState::SHARDS:
            if (shardsLevel == 0 && shardsTop == 1) {
                item("PLAY FROM HERE", KEY_ENTER);
                if (player.playing >= 0) { item(player.paused ? "GO ON" : "PAUSE", KEY_SPACE); item("STOP", KEY_BACKSPACE); }
                item("THE WORLDS", KEY_TAB);
            } else if (shardsLevel == 0) {
                if (!shardWorldKeys.empty()) item("OPEN THE WORLD", KEY_ENTER);
                item("THE SHIP'S PLAYER", KEY_TAB);
            } else if (shardsLevel == 1) {
                item(shardsPage == 2 ? "OPEN THE RECORD THAT TOLD IT" : "READ THE RECORD", KEY_ENTER);
                item("THE NEXT PAGE", KEY_TAB); item("THE PAGE BEFORE", KEY_TAB, MOD_SHIFT);
                if (shardsPage == 0) { item("THE NEXT KIND", KEY_RIGHT); item("THE KIND BEFORE", KEY_LEFT); }
                item("BACK TO THE WORLDS", KEY_ESCAPE);
                break;
            } else {
                const bool valid = shardWorld.valid && !shardHeld.empty() && shardSel >= 0 && shardSel < (int)shardHeld.size();
                const Shard* s = valid ? &shardWorld.shards[shardHeld[shardSel]] : nullptr;
                if (s && s->music) { if (pieceEnded()) item("PLAY IT AGAIN", KEY_ENTER); else item(piecePaused ? "GO ON" : "PAUSE", KEY_SPACE); item("NAME THE PIECE", KEY_N); }
                else if (s && s->chart) {
                    if (chartT >= 0) item("SKIP THE WAIT", KEY_ENTER);
                    else { item("TARGET THE STAR IT MARKS", KEY_ENTER); item(chartUp && chartHeldKey == shardWorld.key ? "PUT A CHART DOWN" : "HOLD IT UP TO THE SKY", KEY_O); }
                } else if (s) {
                    const bool speaking = speechT >= 0 && !speechEnded();
                    item(speaking ? "STOP THE VOICE" : "SPEAK THE PAGE", KEY_ENTER);
                    if (speaking) item(speechPaused ? "GO ON" : "PAUSE THE VOICE", KEY_SPACE);
                    if (shardPageStart.size() > 1) { item("THE NEXT PAGE", KEY_DOWN); item("THE PAGE BEFORE", KEY_UP); }
                }
                item("THE NEXT RECORD", KEY_RIGHT); item("THE RECORD BEFORE", KEY_LEFT);
                item("BACK TO THE LIST", KEY_ESCAPE);
                break;
            }
            item("CLOSE THE DECODER", KEY_ESCAPE);
            break;
        case GameState::INSTRUMENTS:
            if (icase.level == 0) { if (!icase.worlds.empty()) item("OPEN THE FOLDER", KEY_ENTER); item("CLOSE THE CASE", KEY_ESCAPE); break; }
            if (icase.valid && instrumentPlaysDisc(icase.inst)) item(icase.discOn ? "STOP THE MACHINE" : "WIND THE MACHINE", KEY_ENTER);
            else if (icase.valid) { item("LOWER", KEY_LEFT); item("HIGHER", KEY_RIGHT); item("THE NINTH NOTE", KEY_9); item("THE TENTH NOTE", KEY_0); }
            item("THE NEXT INSTRUMENT", KEY_DOWN); item("THE ONE BEFORE", KEY_UP);
            item("BACK TO THE FOLDERS", KEY_ESCAPE);
            break;
        case GameState::DRONES: item("SEND THE DRONES", KEY_ENTER); item("BACK TO THE CABIN", KEY_ESCAPE); break;
        case GameState::PROBE:
            if (probe.active && probe.endClock >= 0) item("LET IT GO", KEY_ENTER);
            else item("THE CHUTE: OPEN, OR CUT", KEY_SPACE);
            item("CENTRE THE CAMERA", KEY_X); item("NAME THE STORM", KEY_N); item("PHOTOGRAPH", KEY_P);
            item("BACK TO THE CABIN", KEY_ESCAPE);
            break;
        case GameState::RECORDING:
            item(replay.playing ? "PAUSE" : "PLAY", KEY_SPACE);
            item("TO THE START", KEY_HOME); item("TO THE END", KEY_END);
            item("THE NEXT LAYER", KEY_PAGE_DOWN); item("THE LAYER BEFORE", KEY_PAGE_UP);
            item("CENTRE THE VIEW", KEY_X); item("NAME THE STORM", KEY_N);
            item("BACK TO THE GALLERY", KEY_ESCAPE);
            break;
        case GameState::ALMANAC: item("RUN THE CLOCK TO IT", KEY_ENTER); if (almanacRunning) item("STOP THE RUN", KEY_T); item("BACK", KEY_ESCAPE); break;
        case GameState::DATA: item("THE ALMANAC", KEY_T); if (!dataProbes().empty()) item("THE PROBES' PROFILES", KEY_RIGHT); item("CLOSE", KEY_ESCAPE); break;
        case GameState::SYSTEM_LIST: item("APPROACH IT", KEY_ENTER); item("MAKE IT THE TARGET", KEY_L); item("CLOSE", KEY_ESCAPE); break;
        case GameState::SHIPSCREEN: item("THE NEXT PAGE", KEY_RIGHT); item("THE PAGE BEFORE", KEY_LEFT); item("CLOSE", KEY_ESCAPE); break;
        case GameState::CONSOLE:
            item("THE KEYBOARD", -1, 0, PSP_KEYBOARD);
            typed("SL: THE STARS NEAR", "SL\n"); typed("DL: THIS SYSTEM'S BODIES", "DL\n"); typed("WHERE: THE SHIP'S SECTOR", "WHERE\n");
            typed("HOME: TARGET THE HOME STAR", "HOME\n"); typed("PREV: TARGET THE STAR BEFORE", "PREV\n"); typed("CAT: THE LAST NOTES", "CAT\n"); typed("HELP: THE COMMANDS", "HELP\n");
            item("CLOSE THE CONSOLE", KEY_ESCAPE);
            break;
        case GameState::TEXT_ENTRY:
            if (!padKeyboardShown()) item("THE KEYBOARD", -1, 0, PSP_KEYBOARD);
            item("DONE", KEY_ENTER); item("CANCEL", KEY_ESCAPE);
            if (textKind == TEXT_MARK) item("WHAT IT MARKS: THE NEXT", KEY_TAB);
            break;
        case GameState::HELP: item("THE NEXT PAGE", KEY_SPACE); item("CLOSE", KEY_ESCAPE); break;
        default: break;
    }
    return v;
}

// a key into the queue
void Game::padPress(int key, int mods) { if (key >= 0 && key < KEY_MAX) padUI.queue.push_back({key, mods}); }

void Game::padRunItem(const PadItem& it) {
    switch (it.special) {
        case PSP_KEYBOARD: padUI.keyboard = true; padUI.kbFor = state == GameState::SPACE && ship.targeting ? 1 : 0; padUI.kRow = padUI.kbFor == 1 ? 3 : 0; padUI.kCol = 0; padUI.latch = 0; return;
        case PSP_CONTROLLER: openPadScreen(); return;
        case PSP_AUTO: returnState = state; openAutoScreen(state); return;
        case PSP_TYPE: for (char c : it.text) padPress(charKey(c), 0); return;
        case PSP_TWICE: padPress(it.key, it.mods); padPress(it.key, it.mods); return;
        default: padPress(it.key, it.mods); return;
    }
}

// the keyboard over the frame: asked for, or by itself at a text entry and the console
bool Game::padKeyboardShown() const {
    if (padUI.keyboard) return true;
    return padUI.present && padUI.active && stateHas(state, ST_TYPING);
}

// the CONTROLS screen on its controller page, from anywhere
void Game::openPadScreen() {
    if (state == GameState::SPACE || state == GameState::SURFACE || state == GameState::TITLE) returnState = state;
    keysReturn = state; keysPage = 1; keysCapture = false;
    padUI.screenSel = 0; padUI.capture = -1; padUI.menu = false;
    state = GameState::KEYS;
}

// the menu's controller: up and down (repeating), LB and RB the pages, A runs the line, B, Y or START close it
void Game::padMenuUpdate(const bool* pressed, double realDt) {
    PadUI& P = padUI;
    if (pressed[PR_B] || pressed[PR_Y] || pressed[PR_START]) { P.menu = false; audio.beep = BEEP_UI; return; }
    auto turn = [&](int dir) {
        P.page = (P.page + 3 + dir) % 3;
        if (P.page == 0 && padItems(0).empty()) P.page = (P.page + 3 + dir) % 3;
        P.sel = 0; audio.beep = BEEP_UI;
    };
    if (pressed[PR_LB] || (P.page != 2 && P.pulse[2])) { turn(-1); return; }
    if (pressed[PR_RB] || (P.page != 2 && P.pulse[3])) { turn(1); return; }
    if (P.page == 2) { padKeyboardUpdate(pressed, true); return; }
    const std::vector<PadItem> items = padItems(P.page);
    const int n = (int)items.size();
    if (n == 0) return;
    P.sel = std::max(0, std::min(n - 1, P.sel));
    if (P.pulse[0]) { P.sel = (P.sel + n - 1) % n; audio.beep = BEEP_UI; }
    if (P.pulse[1]) { P.sel = (P.sel + 1) % n; audio.beep = BEEP_UI; }
    if (pressed[PR_A] || pressed[PR_X]) { P.menu = false; padRunItem(items[P.sel]); }
}

// the keyboard's controller: the arrows move over the keys, A presses the one under the cursor (the latched Shift, Ctrl and Alt with it);
// at a text entry X deletes, Y puts a space, START is Enter and B Esc
void Game::padKeyboardUpdate(const bool* pressed, bool menuPage) {
    PadUI& P = padUI;
    const auto& rows = kbRows();
    P.kRow = std::max(0, std::min((int)rows.size() - 1, P.kRow));
    P.kCol = std::max(0, std::min((int)rows[P.kRow].size() - 1, P.kCol));
    if (P.pulse[0] || P.pulse[1]) {
        const int centre = kbCentre(P.kRow, P.kCol);
        P.kRow = (P.kRow + (int)rows.size() + (P.pulse[1] ? 1 : -1)) % (int)rows.size();
        P.kCol = kbAt(P.kRow, centre);
    }
    if (P.pulse[2] || P.pulse[3]) P.kCol = (P.kCol + (int)rows[P.kRow].size() + (P.pulse[3] ? 1 : -1)) % (int)rows[P.kRow].size();
    if (pressed[PR_A]) {
        const KbKey& k = rows[P.kRow][P.kCol];
        if (k.key < 0) P.latch ^= (k.key == -1 ? MOD_SHIFT : (k.key == -2 ? MOD_CTRL : MOD_ALT));
        else { padPress(k.key, P.latch); P.latch = 0; }
        audio.beep = BEEP_UI;
    }
    if (menuPage) return;
    if (pressed[PR_SELECT]) { P.menu = true; P.page = padItems(0).empty() ? 1 : 0; P.sel = 0; audio.beep = BEEP_UI; return; }   // the commands over the keyboard
    if (pressed[PR_X]) padPress(KEY_BACKSPACE, 0);
    if (pressed[PR_Y]) padPress(KEY_SPACE, 0);
    const bool digits = P.keyboard && P.kbFor == 1;
    if (pressed[PR_START]) { padPress(KEY_ENTER, 0); if (digits) { P.keyboard = false; P.kbFor = 0; } }
    if (pressed[PR_B]) { if (digits) { P.keyboard = false; P.kbFor = 0; } else padPress(KEY_ESCAPE, 0); }
}

// the first thing a frame does: the controller's state, the menu or the keyboard if one is up, else the context's layout into the keys
void Game::padFrame(Input& in, double realDt) {
    PadUI& P = padUI;
    lastPad = in.pad;
    bool keyUsed = std::fabs(in.mouseDx) + std::fabs(in.mouseDy) > 0.5 || in.wheel != 0 || in.mousePressed[0] || in.mousePressed[1] || in.mousePressed[2];
    for (int k = 32; k < KEY_MAX && !keyUsed; k++) keyUsed = in.pressed[k];
    if (keyUsed) P.active = false;   // a key or the mouse takes the controller's place
    if (!in.pad.present) {
        if (P.present) { P.present = false; P.menu = false; P.keyboard = false; P.active = false; P.capture = -1; P.queue.clear(); }
        return;
    }
    const std::string device = PadMap::deviceKey(in.pad);
    padMap.use(in.pad);
    const PadRead r = padMap.read(in.pad, settings.padDeadZone);
    const bool fresh = !P.present || device != P.device || P.resync;
    if (device != P.device) { P.device = device; P.announce = true; }   // a new device is announced (the same one back is not)
    P.present = true; P.resync = false;
    if (fresh) for (int i = 0; i < PR_BUTTONS; i++) P.prev[i] = r.down[i];   // what is held as it arrives presses nothing
    bool pressed[PR_BUTTONS]; bool any = false;
    for (int i = 0; i < PR_BUTTONS; i++) { pressed[i] = r.down[i] && !P.prev[i]; any = any || pressed[i]; P.prev[i] = r.down[i]; }
    if (any || r.walkX != 0 || r.walkY != 0 || r.lookX != 0 || r.lookY != 0) P.active = true;
    // the arrows: the d-pad, and the left stick past half its throw, each repeating after a moment held (the menus, the keyboard, the screens)
    const bool dir[4] = {r.down[PR_UP] || r.walkY < -0.5, r.down[PR_DOWN] || r.walkY > 0.5, r.down[PR_LEFT] || r.walkX < -0.5, r.down[PR_RIGHT] || r.walkX > 0.5};
    bool firstHeld[4] = {};
    for (int d = 0; d < 4; d++) {
        if (!dir[d]) { P.held[d] = false; P.repeat[d] = 0; P.pulse[d] = false; continue; }
        const bool first = firstHeld[d] = !P.held[d];
        const double before = P.repeat[d];
        P.held[d] = true; P.repeat[d] = first ? 0 : before + realDt;
        P.pulse[d] = first || (P.repeat[d] >= REPEAT_DELAY && (before < REPEAT_DELAY || std::floor((P.repeat[d] - REPEAT_DELAY) / REPEAT_EVERY) > std::floor((before - REPEAT_DELAY) / REPEAT_EVERY)));
    }
    if (P.announce && (state == GameState::SPACE || state == GameState::SURFACE)) {   // a new controller, said once where the status shows
        P.announce = false;
        const std::string name = trunc(upper(in.pad.name[0] ? in.pad.name : "A CONTROLLER"), 22);
        const std::string msg = in.pad.mapped ? fmt("CONTROLLER: %s - Y OPENS THE COMMANDS", name.c_str())
                                              : fmt("%s: NOT RECOGNISED - SETTINGS > CONTROLS SETS ITS BUTTONS", name.c_str());
        if (realTime < statusUntil) { statusNext = msg; statusNextSecs = 6; } else status(msg, 6);
    }
    auto deliver = [&]() {   // the queue's next key, its modifiers held with it
        if (P.queue.empty()) return;
        const std::pair<int, int> k = P.queue.front();
        P.queue.erase(P.queue.begin());
        in.down[k.first] = true; in.pressed[k.first] = true;
        if (k.second & MOD_SHIFT) in.down[KEY_LEFT_SHIFT] = true;
        if (k.second & MOD_CTRL) in.down[KEY_LEFT_CONTROL] = true;
        if (k.second & MOD_ALT) in.down[KEY_LEFT_ALT] = true;
    };
    if (P.keyboard && P.kbFor == 1 && !(state == GameState::SPACE && ship.targeting)) { P.keyboard = false; P.kbFor = 0; }   // the aim's digits went with the aim
    if (P.keyboard && P.kbFor == 0 && !stateHas(state, ST_TYPING)) P.keyboard = false;
    if (state == GameState::KEYS && keysPage == 1 && P.capture >= 0) { deliver(); return; }   // the controller page reads the controller itself
    const bool shown = padKeyboardShown(), justUp = shown && !P.kbShown;
    if (justUp) { P.kRow = P.keyboard && P.kbFor == 1 ? 3 : 0; P.kCol = 0; P.latch = 0; }   // a keyboard just up: its cursor on A (on 1 for the digits)
    P.kbShown = shown;
    if (P.menu) { padMenuUpdate(pressed, realDt); deliver(); return; }
    if (shown) { if (!justUp) padKeyboardUpdate(pressed, false); deliver(); return; }   // the press that brought it up types nothing
    if (ax.on && pressed[PR_SELECT]) { in.pressed[KEY_M] = in.down[KEY_M] = true; pressed[PR_SELECT] = false; }   // R-420: auto-explore's tape (SELECT's own key would end it)
    if (pressed[PR_Y]) {
        P.menu = true; P.page = padItems(0).empty() ? 1 : 0; P.sel = 0; audio.beep = BEEP_UI;
        deliver(); return;
    }
    const int ctx = padContext();
    std::vector<PadBind> binds; padBindings(ctx, binds);
    const bool shiftFree = (ctx == PCX_FOOT || ctx == PCX_DRONE) && pressed[PR_RB];   // RB's scanner while RT sprints (or the drone descends) is the scanner, not Shift+R
    for (const PadBind& b : binds) {
        if (b.role < 0 || b.role >= PR_BUTTONS || b.key < 0 || !r.down[b.role]) continue;
        if (shiftFree && b.key == KEY_LEFT_SHIFT) continue;
        in.down[b.key] = true;
        if (pressed[b.role]) in.pressed[b.key] = true;
        if (b.mods & MOD_SHIFT) in.down[KEY_LEFT_SHIFT] = true;
        if (b.mods & MOD_CTRL) in.down[KEY_LEFT_CONTROL] = true;
    }
    static const int ARROWS[4] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
    static const int DPAD[4] = {PR_UP, PR_DOWN, PR_LEFT, PR_RIGHT};
    const bool sceneCtx = ctx == PCX_SHIP || ctx == PCX_FOOT || ctx == PCX_BUGGY || ctx == PCX_DRONE;
    for (int d = 0; d < 4; d++) {
        if (sceneCtx) { if (r.down[DPAD[d]]) { in.down[ARROWS[d]] = true; if (pressed[DPAD[d]]) in.pressed[ARROWS[d]] = true; } }   // the d-pad turns the ship, walks, drives
        else if (ctx == PCX_PROBE || ctx == PCX_RECORDING) { if (dir[d]) { in.down[ARROWS[d]] = true; if (firstHeld[d]) in.pressed[ARROWS[d]] = true; } }   // a camera: held, no repeat
        else if (dir[d]) { in.down[ARROWS[d]] = true; if (P.pulse[d]) in.pressed[ARROWS[d]] = true; }   // the screens: the left stick too, repeating
    }
    if (sceneCtx && (r.walkX != 0 || r.walkY != 0)) { in.moveX = r.walkX; in.moveY = -r.walkY; }
    if (r.lookX != 0 || r.lookY != 0) {   // the right stick as the mouse: a curve for the fine aim, the speed its own (the mouse's sensitivity and inversion taken back out)
        auto curve = [](double v) { const double a = std::fabs(v); return (v < 0 ? -1 : 1) * (0.25 * a + 0.75 * a * a * a); };
        const double rate = ctx == PCX_SCREEN ? 400.0 : 900.0;   // a map's cursor or view (the sector map, the charts, the landing map), slower than a head
        const double k = rate * settings.padLookSpeed * realDt / std::max(0.05, settings.mouseSensitivity);
        in.mouseDx += curve(r.lookX) * k;
        in.mouseDy += curve(r.lookY) * k * (settings.padInvertY ? -1 : 1) * (settings.invertY && ctx != PCX_SCREEN ? -1 : 1);
    }
    deliver();
}

void Game::renderPadKeyboard(int x0, int y0, bool menuPage) {
    const auto& rows = kbRows();
    const int cw = 28, ch = 11;
    for (int rr = 0; rr < (int)rows.size(); rr++) {
        int u = 0;
        for (int c = 0; c < (int)rows[rr].size(); c++) {
            const KbKey& k = rows[rr][c];
            const int x = x0 + u * cw, y = y0 + rr * ch, w = k.w * cw;
            u += k.w;
            const bool sel = rr == padUI.kRow && c == padUI.kCol;
            const bool latched = k.key < 0 && (padUI.latch & (k.key == -1 ? MOD_SHIFT : (k.key == -2 ? MOD_CTRL : MOD_ALT)));
            if (sel) fillRectRGB(canvas, x + 1, y, x + w - 2, y + ch - 2, HUD_DIM);
            else drawRectRGB(canvas, x + 1, y, x + w - 2, y + ch - 2, rgb(40, 60, 48));
            drawTextCentered(canvas, x + w / 2, y + 2, k.label, sel ? HUD_WHITE : (latched ? HUD_AMBER : HUD_GREEN));
        }
    }
    (void)menuPage;
}

// the command menu (a panel over the frame: the pages' tabs, the lines, the context's buttons in words) or the keyboard at a text entry
void Game::renderPad() {
    const PadUI& P = padUI;
    if (!P.present) return;
    if (!P.menu) {
        if (!padKeyboardShown()) return;
        const bool digits = P.keyboard && P.kbFor == 1;
        const int y0 = digits ? 4 : UH - 94;   // the aim keeps the crosshair and its jump line clear: the keyboard at the top
        drawPanel(canvas, 8, y0, UW - 8, y0 + 90, rgb(0, 0, 0), 225);
        drawTextCentered(canvas, UW / 2, y0 + 3, digits ? "DIGITS: THE LIGHT YEARS  START JUMPS  B CLOSES"
                                                : (state == GameState::CONSOLE ? "A TYPES  X DELETES  START RUNS  SELECT COMMANDS" : "A TYPES  X DELETES  Y SPACE  START DONE  B CANCELS"), HUD_DIM);
        renderPadKeyboard(20, y0 + 12, false);
        return;
    }
    const int x0 = 14, x1 = UW - 14, y0 = 10, y1 = UH - 10;
    drawPanel(canvas, x0, y0, x1, y1, rgb(0, 0, 0), 242);
    const int ctx = padContext();
    const std::string tabs[3] = {padContextName(ctx), "GENERAL", "KEYS"};
    int tx = x0 + 6;
    for (int i = 0; i < 3; i++) {
        if (i == 0 && padItems(0).empty()) continue;
        const std::string t = i == P.page ? "[" + tabs[i] + "]" : tabs[i];
        drawText(canvas, tx, y0 + 4, t.c_str(), i == P.page ? HUD_AMBER : HUD_DIM);
        tx += textWidth(t.c_str()) + 10;
    }
    drawText(canvas, x1 - 6 - textWidth("LB RB"), y0 + 4, "LB RB", HUD_DIM);
    const std::string legend = padLegend(ctx, 46);
    std::vector<std::string> legendLines;
    { size_t a = 0; while (a <= legend.size()) { size_t b = legend.find('\n', a); if (b == std::string::npos) b = legend.size(); legendLines.push_back(legend.substr(a, b - a)); a = b + 1; } }
    if (legendLines.size() > 3) legendLines.resize(3);
    const int legendY = y1 - 4 - 8 * (int)legendLines.size();
    for (size_t i = 0; i < legendLines.size(); i++) drawText(canvas, x0 + 6, legendY + 8 * (int)i, legendLines[i].c_str(), rgb(70, 110, 85));
    drawLineRGB(canvas, x0 + 4, legendY - 4, x1 - 4, legendY - 4, rgb(40, 60, 48));
    if (P.page == 2) {
        renderPadKeyboard(x0 + 8, y0 + 18, true);
        drawTextCentered(canvas, UW / 2, y0 + 18 + 7 * 11 + 4, "A PRESSES THE KEY  SHIFT CTRL ALT: WITH THE NEXT", HUD_DIM);
        return;
    }
    const std::vector<PadItem> items = padItems(P.page);
    std::vector<PadBind> binds; padBindings(ctx, binds);
    const int rowH = 9, listY = y0 + 17, rows = std::max(1, (legendY - 8 - listY) / rowH);
    const int n = (int)items.size();
    const int sel = std::max(0, std::min(n - 1, P.sel));
    int top = std::max(0, std::min(sel - rows / 2, n - rows));
    for (int i = 0; i < rows && top + i < n; i++) {
        const PadItem& it = items[top + i];
        const int y = listY + i * rowH;
        const bool s = top + i == sel;
        if (s) drawText(canvas, x0 + 5, y, ">", HUD_AMBER);
        drawText(canvas, x0 + 13, y, trunc(it.label, 34).c_str(), s ? HUD_WHITE : HUD_GREEN);
        std::string hint;   // the button that does it at once here, else the key the line presses
        for (const PadBind& b : binds) if (it.special == PSP_NONE && b.key == it.key && b.mods == it.mods && b.role != PR_START) { hint = ROLE_SHORT[b.role]; break; }
        if (hint.empty() && it.special == PSP_NONE && it.key >= 0) hint = "(" + keyLabel(it.key, it.mods) + ")";
        if (!hint.empty()) drawText(canvas, x1 - 6 - textWidth(hint.c_str()), y, hint.c_str(), hint[0] == '(' ? rgb(70, 110, 85) : HUD_CYAN);
    }
    if (top > 0) drawText(canvas, x1 - 12, listY - 7, "\x02", HUD_DIM);
    if (top + rows < n) drawText(canvas, x1 - 12, legendY - 11, "\x03", HUD_DIM);
}

// ---- the CONTROLS screen's controller page ---------------------------------------------------------------------------------------------

// rows: the sixteen buttons in two columns of eight, the two sticks, the look speed, its inversion, the dead zone, the reset. Enter sets a
// row from the next press (a stick from two pushes, up then right), Left/Right change a setting, Tab the keyboard's page, Esc saves
void Game::updatePadScreen(const Input& in) {
    PadUI& P = padUI;
    const double dt = lastRealDt;
    if (P.capture >= 0) {
        P.capT += dt;
        const bool nothing = !in.pad.present;
        if (in.wasPressed(KEY_ESCAPE) || P.capT > CAPTURE_S || nothing) {
            P.capture = -1; P.resync = true;
            P.note = nothing ? "NO CONTROLLER" : "NOTHING SET"; P.noteUntil = realTime + 3;
            return;
        }
        const bool stick = P.capture >= PR_BUTTONS;
        if (!P.capArmed) {   // a button is taken as it goes down (one held before, the press that began it among them, is not); a stick's step waits for the sticks back where they rested
            bool back = true;
            if (stick) {
                if (in.pad.mapped) for (int a = 0; a < 4; a++) back = back && std::fabs(in.pad.axes[a] - P.capRest.axes[a]) < 0.3;
                for (int a = 0; a < in.pad.rawAxisCount && a < PAD_RAW_AXES; a++) back = back && std::fabs(in.pad.rawAxes[a] - P.capRest.rawAxes[a]) < 0.3;
            }
            P.capPrev = in.pad;
            if (back) P.capArmed = true;
            return;
        }
        const int c = padCapture(P.capPrev, P.capRest, in.pad, stick);
        P.capPrev = in.pad;
        if (c < 0) return;
        if (!stick) {   // a control another button had changes places with this one's (two roles never share a button)
            const int old = padMap.bind[P.capture];
            std::string swapped;
            for (int r2 = 0; r2 < PR_BUTTONS; r2++)
                if (r2 != P.capture && padMap.bind[r2] == c) {
                    padMap.set(in.pad, r2, old);
                    swapped = fmt(", %s TOOK %s", ROLE_SHORT[r2], padControlLabel(old).c_str());
                }
            padMap.set(in.pad, P.capture, c);
            P.note = fmt("%s IS NOW %s%s", ROLE_SHORT[P.capture], padControlLabel(c).c_str(), swapped.c_str());
        } else if (P.capStep == 0) {   // up: the half read as down is the other one
            padMap.set(in.pad, P.capture == PR_BUTTONS ? PR_WALK_Y : PR_LOOK_Y, c ^ 1);
            P.capStep = 1; P.capArmed = false; P.capT = 0;
            return;
        } else {
            padMap.set(in.pad, P.capture == PR_BUTTONS ? PR_WALK_X : PR_LOOK_X, c);
            P.note = fmt("THE %s STICK IS SET", P.capture == PR_BUTTONS ? "WALK" : "LOOK");
        }
        P.noteUntil = realTime + 3;
        P.capture = -1; P.resync = true; audio.beep = BEEP_UI;
        return;
    }
    if (in.wasPressed(KEY_TAB)) { keysPage = 0; audio.beep = BEEP_UI; return; }
    if (in.wasPressed(KEY_ESCAPE)) {
        if (padMap.dirty && padMap.save(padPath)) padMap.dirty = false;   // only what changed: a file of the explorer's is not rewritten for nothing
        settings.save(settingsPath);
        if (keymap) keymap->save(keysPath);
        state = keysReturn; audio.beep = BEEP_UI;
        return;
    }
    int& s = P.screenSel;
    if (in.wasPressed(KEY_UP)) s = s < PR_BUTTONS ? (s % 8 == 0 ? PAD_ROWS - 1 : s - 1) : (s == PR_BUTTONS ? 7 : s - 1);
    if (in.wasPressed(KEY_DOWN)) s = s < PR_BUTTONS ? (s % 8 == 7 ? PR_BUTTONS : s + 1) : (s == PAD_ROWS - 1 ? 0 : s + 1);
    const int dir = (in.wasPressed(KEY_RIGHT) ? 1 : 0) - (in.wasPressed(KEY_LEFT) ? 1 : 0);
    if (dir && s < PR_BUTTONS) { s = s < 8 ? s + 8 : s - 8; return; }
    if (dir && s == 18) { settings.padLookSpeed = std::round((settings.padLookSpeed + 0.1 * dir) * 10) / 10; settings.clampAll(); audio.beep = BEEP_UI; }
    if (dir && s == 19) { settings.padInvertY = !settings.padInvertY; audio.beep = BEEP_UI; }
    if (dir && s == 20) { settings.padDeadZone = std::round((settings.padDeadZone + 0.02 * dir) * 100) / 100; settings.clampAll(); audio.beep = BEEP_UI; }
    if (!enterKey(in)) return;
    if (s == 19) { settings.padInvertY = !settings.padInvertY; audio.beep = BEEP_UI; return; }
    if (s == 21) { padMap.reset(in.pad); P.note = "THIS CONTROLLER IS BACK TO ITS DEFAULTS"; P.noteUntil = realTime + 3; audio.beep = BEEP_UI; return; }
    if (s >= 18) return;
    if (!in.pad.present) { P.note = "NO CONTROLLER"; P.noteUntil = realTime + 3; audio.beep = BEEP_ERROR; return; }
    P.capture = s;   // 16 the walk stick, 17 the look stick
    P.capStep = 0; P.capArmed = false; P.capT = 0; P.capRest = in.pad;   // the axes as they rest now
    audio.beep = BEEP_UI;
}

void Game::renderPadScreen() {
    const PadUI& P = padUI;
    const int top = 6, bottom = UH - 6;
    drawPanel(canvas, 10, top, UW - 10, bottom, rgb(0, 0, 0), 225);
    drawText(canvas, 16, top + 5, "CONTROLS", HUD_AMBER);
    drawText(canvas, 16 + textWidth("CONTROLS") + 12, top + 5, "KEYBOARD", HUD_DIM);
    drawText(canvas, 16 + textWidth("CONTROLS") + 12 + textWidth("KEYBOARD") + 10, top + 5, "[CONTROLLER]", HUD_WHITE);
    const PadState& pad = lastPad;
    std::string head = !pad.present ? "NO CONTROLLER CONNECTED" : trunc(upper(pad.name[0] ? pad.name : "A CONTROLLER"), 24) + (pad.mapped ? " - STANDARD LAYOUT" : " - NOT RECOGNISED, READ RAW");
    drawText(canvas, 16, top + 15, head.c_str(), pad.present ? HUD_CYAN : HUD_RED);
    const PadRead live = padMap.read(pad, settings.padDeadZone);
    auto valueOf = [&](int row) -> std::string {
        if (P.capture >= 0 && row == P.capture) {
            const int left = std::max(0, (int)std::ceil(CAPTURE_S - P.capT));
            if (P.capture < PR_BUTTONS) return fmt("PRESS IT  %d", left);
            if (!P.capArmed) return "LET THE STICK GO";
            return fmt("%s  %d", P.capStep == 0 ? "PUSH IT UP" : "PUSH IT RIGHT", left);
        }
        if (row < PR_BUTTONS) return padControlLabel(padMap.bind[row]);
        if (row == 16) return padControlName(padMap.bind[PR_WALK_X]) + " " + padControlName(padMap.bind[PR_WALK_Y]) + fmt("  %+.1f %+.1f", live.walkX, live.walkY);
        if (row == 17) return padControlName(padMap.bind[PR_LOOK_X]) + " " + padControlName(padMap.bind[PR_LOOK_Y]) + fmt("  %+.1f %+.1f", live.lookX, live.lookY);
        if (row == 18) return fmt("%.1f", settings.padLookSpeed);
        if (row == 19) return settings.padInvertY ? "ON" : "OFF";
        if (row == 20) return fmt("%.0f%%", settings.padDeadZone * 100);
        return "";
    };
    const int y0 = top + 28;
    for (int row = 0; row < PAD_ROWS; row++) {
        int x, y;
        if (row < PR_BUTTONS) { x = row < 8 ? 20 : 160; y = y0 + (row % 8) * 9; }
        else { x = 20; y = y0 + 8 * 9 + 5 + (row - PR_BUTTONS) * 9; }
        const bool sel = row == P.screenSel;
        const bool held = row < PR_BUTTONS && live.down[row];
        static const char* const REST[6] = {"WALK STICK", "LOOK STICK", "LOOK SPEED", "INVERT LOOK", "DEAD ZONE", "RESET THIS CONTROLLER"};
        const char* label = row < PR_BUTTONS ? ROLE_SCREEN[row] : REST[row - PR_BUTTONS];
        if (sel) drawText(canvas, x - 7, y, ">", HUD_AMBER);
        drawText(canvas, x, y, label, held ? HUD_CYAN : (sel ? HUD_WHITE : HUD_GREEN));
        const std::string v = valueOf(row);
        const bool capturing = P.capture >= 0 && row == P.capture;
        drawText(canvas, x + (row < 8 ? 62 : (row < PR_BUTTONS ? 70 : 84)), y, trunc(v, row < 8 ? 12 : (row < PR_BUTTONS ? 13 : 32)).c_str(), capturing ? HUD_AMBER : (sel ? HUD_WHITE : HUD_DIM));
    }
    const std::string foot = P.capture >= 0 ? "ESC OR WAIT: NOTHING CHANGES" : (realTime < P.noteUntil ? P.note : "ENTER SETS  \x04\x05 CHANGE  TAB KEYBOARD  ESC SAVES");
    drawTextCentered(canvas, UW / 2, bottom - 19, foot.c_str(), realTime < P.noteUntil && P.capture < 0 ? HUD_AMBER : HUD_DIM);
    drawTextCentered(canvas, UW / 2, bottom - 10, "SAVED TO VESPERIS_PAD.TXT, A SET EACH CONTROLLER", HUD_DIM);
}
