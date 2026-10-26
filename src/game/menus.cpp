// The overlay screens: the title, help, the menu, the settings and the keys (the surface's play is surface_play.cpp, the
// system list and the data sheet data_sheet.cpp).
#include "game.h"
#include "peoples/plan.h"
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace {
// N5-05: the actions a player can rebind (logical key, label)
struct KeyAction { int key; const char* label; };
static const KeyAction KEY_ACTIONS[] = {{KEY_W, "FORWARD / DRIVE"}, {KEY_S, "BACK / REVERSE"}, {KEY_A, "LEFT / STEER"}, {KEY_D, "RIGHT / STEER"}, {KEY_SPACE, "JUMP / BRAKE"},
        {KEY_LEFT_SHIFT, "SPRINT"}, {KEY_E, "USE / VEHICLE"}, {KEY_Q, "BOARD CAPSULE"}, {KEY_B, "BUGGY / SIGNAL RADAR"}, {KEY_C, "CROUCH / CLOUDS"}, {KEY_Z, "STAND TALL / TELESCOPE"},
        {KEY_V, "VIEW / VISION"}, {KEY_X, "CREATURES"}, {KEY_M, "WAYPOINT"}, {KEY_N, "SECTOR MAP / NEXT STAR"}, {KEY_K, "RECALL CAPSULE"}, {KEY_I, "DATA SHEET"}, {KEY_T, "WARP / ALMANAC"},
        {KEY_R, "AIM / SCANNER"}, {KEY_L, "TARGET / MARK"}, {KEY_TAB, "LIST / READINGS"}, {KEY_O, "ORBIT / CHASE"}, {KEY_F, "FIELD AMP. / DRONE"}, {KEY_G, "GUIDE"}, {KEY_J, "LOG"}, {KEY_H, "HELP"},
        {KEY_P, "SCREENSHOT"}, {KEY_U, "SHIP LIGHT / LAMP"}};
const int KEY_ACTION_COUNT = (int)(sizeof(KEY_ACTIONS) / sizeof(KEY_ACTIONS[0]));
std::string onOff(bool on) { return on ? "ON" : "OFF"; }
}

// U-03: the Esc menu's rows (R-419 put AUTO-EXPLORE second)
const RowList<MenuRow>& Game::escMenu() {
    static constexpr MenuRow ROWS[] = {
        {"RESUME", [](Game& g) { g.state = g.returnState; }},
        {"AUTO-EXPLORE", [](Game& g) { g.openAutoScreen(GameState::MENU); }},
        {"SAVE EXPEDITION", [](Game& g) { g.openSlots(false); }},
        {"LOAD EXPEDITION", [](Game& g) { g.openSlots(true); }},
        {"SETTINGS", [](Game& g) { g.settingsSel = 0; g.state = GameState::SETTINGS; }},
        {"HELP", [](Game& g) { g.helpPage = g.returnState == GameState::SURFACE ? 1 : 0; g.state = GameState::HELP; }},
        {"NEW EXPEDITION", [](Game& g) { g.newGame(); }},
        {"QUIT TO DESKTOP", [](Game& g) { g.autosave(); g.wantsQuit = true; }},
    };
    static constexpr RowList<MenuRow> LIST = rowsOf(ROWS);
    static_assert(rowsWhole(LIST), "each row its label and its act");
    return LIST;
}

// U-03: the settings' rows (N5-05 added CONTROLS; R-418: its controller's page first when a controller is in use)
const RowList<SettingRow>& Game::settingRows() {
    using S = Settings;
    static constexpr SettingRow ROWS[] = {
        {"MOUSE SENSITIVITY", [](const S& s) { return fmt("%.1f", s.mouseSensitivity); }, [](Game&, S& s, int d) { s.mouseSensitivity += 0.1 * d; }},
        {"INVERT MOUSE Y", [](const S& s) { return onOff(s.invertY); }, [](Game&, S& s, int) { s.invertY = !s.invertY; }},
        {"FIELD OF VIEW", [](const S& s) { return fmt("%.0f%c", s.fovDeg, CH_DEGREE); }, [](Game&, S& s, int d) { s.fovDeg += 5 * d; }},
        {"MASTER VOLUME", [](const S& s) { return fmt("%.0f%%", s.masterVolume * 100); }, [](Game&, S& s, int d) { s.masterVolume += 0.05 * d; }},
        {"SCANLINES", [](const S& s) { return onOff(s.scanlines); }, [](Game&, S& s, int) { s.scanlines = !s.scanlines; }},
        {"WINDOW SIZE", [](const S& s) { return fmt("%dX  (%dX%d)", s.windowScale, 320 * s.windowScale, 200 * s.windowScale); },
         [](Game& g, S& s, int d) { s.windowScale += d; s.clampAll(); g.wantsWindowScale = s.windowScale; }},
        {"RENDER SCALE", [](const S& s) { return fmt("%dX  (%dX%d)", s.renderScale, 320 * s.renderScale, 200 * s.renderScale); },
         [](Game&, S& s, int d) { s.renderScale += d; }},
        {"MUSH FILTER", [](const S& s) -> std::string { return s.mushMode == 0 ? "CLASSIC" : (s.mushMode == 1 ? "SOFT" : (s.mushMode == 2 ? "SHARP" : "FINE")); },
         [](Game&, S& s, int d) { s.mushMode = (s.mushMode + 4 + d) % 4; }},
        {"TERRAIN SHADING", [](const S& s) -> std::string { return s.shadingMode == 0 ? "FLAT CELLS" : (s.shadingMode == 1 ? "STEPPED" : "SMOOTH"); },
         [](Game&, S& s, int d) { s.shadingMode = (s.shadingMode + 3 + d) % 3; }},
        {"CRT SHADER", [](const S& s) { return onOff(s.crt); }, [](Game&, S& s, int) { s.crt = !s.crt; }},
        {"BLOOM", [](const S& s) { return onOff(s.bloom); }, [](Game&, S& s, int) { s.bloom = !s.bloom; }},
        {"SPRINT KEY", [](const S& s) -> std::string { return s.sprintToggle ? "TOGGLE" : "HOLD"; }, [](Game&, S& s, int) { s.sprintToggle = !s.sprintToggle; }},
        {"SPRINT SPEED", [](const S& s) { return fmt("X%.2f", s.sprintSpeed); }, [](Game&, S& s, int d) { s.sprintSpeed += 0.25 * d; }},
        {"SHIP VIEW", [](const S& s) -> std::string { return s.cabin ? "CABIN" : "COCKPIT"; }, [](Game&, S& s, int) { s.cabin = !s.cabin; }},
        {"CLOCK", [](const S& s) -> std::string { return s.clockMode ? "REAL TIME (UTC)" : "GAME TIME"; }, [](Game&, S& s, int) { s.clockMode = 1 - s.clockMode; }},
        {"ASPECT", [](const S& s) -> std::string { return s.aspect ? "4:3 (TALL PIXELS)" : "SQUARE PIXELS"; }, [](Game&, S& s, int) { s.aspect = 1 - s.aspect; }},
        {"DITHER", [](const S& s) { return onOff(s.dither); }, [](Game&, S& s, int) { s.dither = !s.dither; }},
        {"HUD COLOUR", [](const S& s) -> std::string { return s.hudColor == 0 ? "GREEN" : (s.hudColor == 1 ? "AMBER" : "WHITE"); },
         [](Game&, S& s, int d) { s.hudColor = (s.hudColor + 3 + d) % 3; }},
        {"CONTROLS", [](const S&) -> std::string { return "ENTER TO OPEN"; }, nullptr, [](Game& g) {
             g.keysSel = 0; g.keysCapture = false; g.keysPage = g.padInUse() ? 1 : 0; g.keysReturn = GameState::SETTINGS; g.padUI.capture = -1;
             g.state = GameState::KEYS;
         }},
    };
    static constexpr RowList<SettingRow> LIST = rowsOf(ROWS);
    static_assert(rowsWhole(LIST), "each row its label and its act");
    return LIST;
}

// the title: Enter goes on (on the ground where the expedition was, else in space), N begins afresh over a save, C the credits,
// H help, Esc quits; the ship moves on the real clock meanwhile
void Game::updateTitle(const Input& in, double dt, double realDt) {
    titleYaw += realDt * 0.04;
    titleIdle += realDt; attractT += realDt;
    { bool any = std::fabs(in.mouseDx) + std::fabs(in.mouseDy) > 0; for (int k = 0; k < KEY_MAX && !any; k++) any = in.pressed[k]; if (any) titleIdle = 0; }
    if (in.wasPressed(KEY_C) && !in.ctrl()) { returnState = GameState::TITLE; helpPage = 2; state = GameState::HELP; }   // credits
    updateShipMotion(realDt);
    if (enterKey(in)) {
        state = (returnState == GameState::SURFACE && surf.valid) ? GameState::SURFACE : GameState::SPACE;
        status(state == GameState::SURFACE ? "EXPEDITION RESUMED ON THE SURFACE - H FOR HELP" : "STARDRIFTER READY - H FOR HELP", 6);
        if (macHints && !macHintShown) { macHintShown = true; statusNext = "ON A MAC THE F KEYS NEED FN: USE H, I, P, CTRL+S, CTRL+L, CTRL+K, CTRL+F"; statusNextSecs = 7; }
    }
    if (in.wasPressed(KEY_N) && hasSave) { newGame(); state = GameState::SPACE; }
    if (helpKey(in)) { returnState = GameState::TITLE; helpPage = 0; state = GameState::HELP; }
    if (in.wasPressed(KEY_ESCAPE)) wantsQuit = true;
}

// U-04: the title's commands (the controller's first page; its name is its context's)
void Game::titlePad(PadMenu& m) const {
    m.item(hasSave ? "CONTINUE" : "BEGIN THE EXPEDITION", KEY_ENTER);
    if (hasSave) m.item("NEW EXPEDITION", KEY_N);
    m.item("HELP", KEY_H); m.item("CREDITS", KEY_C); m.item("QUIT", KEY_ESCAPE);
}

// help: Space turns its pages (not the credits'), any other key closes it
void Game::updateHelp(const Input& in, double dt, double realDt) {
    bool any = false;
    for (int k = 0; k < KEY_MAX; k++) if (in.pressed[k]) any = true;
    if (in.wasPressed(KEY_SPACE) && helpPage != 2) helpPage = helpPage == 0 ? 1 : (helpPage == 1 ? 3 : 0);   // the ship, the surface, the controller
    else if (any) state = returnState;
}

// U-04: help's commands (the controller's first page)
void Game::helpPad(PadMenu& m) const { m.title = "HELP"; m.item("THE NEXT PAGE", KEY_SPACE); m.item("CLOSE", KEY_ESCAPE); }

// the Esc menu: Enter does its row's act
void Game::updateMenu(const Input& in, double dt, double realDt) {
    const RowList<MenuRow>& m = escMenu();
    menuStep(in, menuSel, m.count);
    if (in.wasPressed(KEY_ESCAPE)) state = returnState;
    if (enterKey(in) && menuSel >= 0 && menuSel < m.count) m.row[menuSel].act(*this);
}

// the title, over space (the system its camera tours)
void Game::renderTitle() {
    renderSpace();
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 90);
    // the title carries the name and the choices, nothing else
    drawTextCentered(canvas, UW / 2 + 2, 42, "VESPERIS", rgb(20, 40, 30), 4);
    drawTextCentered(canvas, UW / 2, 40, "VESPERIS", HUD_GREEN, 4);
    if (padInUse()) {   // R-418: a controller's own buttons
        drawTextCentered(canvas, UW / 2, 100, hasSave ? "A: CONTINUE      Y: NEW EXPEDITION, HELP, QUIT" : "A: BEGIN EXPEDITION      Y: HELP, QUIT", HUD_AMBER);
        return;
    }
    drawTextCentered(canvas, UW / 2, 100, hasSave ? "ENTER: CONTINUE      N: NEW EXPEDITION" : "ENTER: BEGIN EXPEDITION", HUD_AMBER);
    drawTextCentered(canvas, UW / 2, 112, "H: HELP     C: CREDITS     ESC: QUIT", HUD_DIM);
}

void Game::renderHelp() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), helpPage == 2 ? 245 : 200);
    static const char* const page0[] = {
        "STARDRIFTER - FLIGHT CONTROL", "",
        "MOUSE LOOKS, W A S D WALK IN THE CABIN, ARROWS TURN",
        "THE SHIP; E USES A CONSOLE, A SCREEN, THE DECODER",
        "(TAB: PLAYER), THE CASE, THE DRONES OR THE CAPSULE",   // C-18: the drone bay
        "U LIGHT  Y GLASS HULL  PGUP/PGDN ROOF DECK",
        "R             AIM AT A STAR: N NEXT, ENTER LOCKS IT",
        "R, DIGITS, ENTER  JUMP THAT MANY LY ALONG THE AIM",
        "V             VIMANA FLIGHT TO THE REMOTE TARGET",
        "L / CLICK     LOCAL TARGET: BODIES, THEN BELTS",
        "TAB           THE ANALYZER: BODIES AND BELTS",
        "ENTER         FINE APPROACH TO THE LOCAL TARGET",
        "X  CENTER ON TARGET   Z  TELESCOPE   F  FIELD AMP.",
        "B             THE RADAR: HOLD A RISE, ENTER FLIES",
        "O  ORBIT / CHASE   SHIFT+ARROWS  ROUND THE WORLD",
        "G GUIDE  M MAP, TAB GALAXY  J LOG  C CAPSULE/PROBE",
        "I  DATA SHEET  T  TIME WARP  CTRL+T  THE ALMANAC",
        "CTRL+S / L    SAVE / LOAD         P  SCREENSHOT",
        "CTRL+K / F    SCANLINES / FULLSCREEN   ESC MENU",
        "F1 F2 F5 F9 F10 F11 F12 DO THE SAME (MAC: HOLD FN)",
        "PAGE 1/3 - SPACE: NEXT PAGE, ANY OTHER KEY: CLOSE"};
    static const char* const page1[] = {
        "SURFACE EXPLORATION", "",
        "MOUSE         LOOK AROUND",
        "W A S D       WALK   SHIFT SPRINT (STAMINA)",
        "SPACE         JUMP; HOLD IN THE AIR: JETPACK",
        "1-9 / 0       AUTOWALK SPEED / STOP    C CROUCH",
        "Z (HOLD)      STAND TALL    CTRL (SWIMMING) DIVE",
        "B / F         A (NEW) BUGGY / DRONE AT THE CAPSULE",
        "E             IN/OUT, TAKE, THE CAPSULE (Q TOO)",
        "  BUGGY: W/S DRIVE  A/D STEER  SPACE BRAKE  V VIEW",
        "  DRONE: SPACE LIFTS OFF  W/S THRUST  A/D TURN",
        "         SPACE / SHIFT CLIMB, DESCEND, LAND  V VIEW",
        "  THE MOUSE PANS THE NOSE CAMERA 70 DEG",
        "M  WAYPOINT AT THE CROSSHAIR   K  CALL THE CAPSULE",
        "R / SHIFT+R   SCANNER MODE / THE MARKED OUT OR IN",
        "L / U         MARK AT THE CROSSHAIR / THE LAMP",
        "N             MAP: ENTER MARKS, SHIFT+ENTER SHOWS",
        "TAB           THE READINGS: TIME, SUN, AIR, GROUND",
        "I (F2)  DATA  T  TIME WARP  CTRL+T  THE ALMANAC",
        "CTRL+S  SAVE  P PHOTO  G GUIDE  H HELP  ESC MENU",
        "PAGE 2/3 - SPACE: NEXT PAGE, OTHER KEYS: CLOSE"};
    static const char* const page3[] = {   // the controller
        "CONTROLLER", "",
        "Y          THE COMMANDS: EVERY ACTION HERE, ANY KEY",
        "START MENU  SELECT TAB  D-PAD ARROWS  R STICK LOOKS",
        "SCREENS   A ENTER  B BACK  X SPACE  LB/RB PAGES",
        "          LT/RT ZOOM  L3 SHIFT  LEFT STICK ARROWS",
        "SHIP      A APPROACH, LOCK  B LEAVE THE AIM/RADAR",
        "          X USE  LT TARGET  L3 AIM  R3 CENTRE",
        "          LB/RB ROOF, ZOOM, GAIN  RT+D-PAD ORBIT",
        "ON FOOT   A JUMP  B CROUCH  X USE  LT STAND TALL",
        "          RT SPRINT  LB MAP  RB SCANNER  R3 VISION",
        "BUGGY     RT/LT DRIVE  A BRAKE  B VIEW  X OUT",
        "DRONE     A/RT CLIMB  B/LT DESCEND  X OUT  R3 VIEW",
        "PROBE     A CHUTE  B CABIN  X CENTRE  RT FAST",
        "PLAYING   A X LB RB LT RT L3 R3: THE NOTES 1-8",
        "TEXT      A TYPES  X DELETES  Y SPACE  START DONE",
        "MAPS      THE STICKS MOVE THE CURSOR, LT/RT ZOOM",
        "SETTINGS > CONTROLS: EACH BUTTON OF A CONTROLLER",
        "GAMECONTROLLERDB.TXT BESIDE THE GAME: NEW LAYOUTS", "",
        "PAGE 3/3 - SPACE: FIRST PAGE, OTHER KEYS: CLOSE"};
    static const char* const page2[] = {
        "CREDITS", "",
        "VESPERIS",
        "VIBE CODED. HEAVILY INSPIRED BY NOCTIS IV",
        "BY ALESSANDRO GHIGNOLA (2001).", "",
        "C++ AND RAYLIB.", "",
        "ANY KEY: BACK"};
    const char* const* page = helpPage == 0 ? page0 : (helpPage == 1 ? page1 : (helpPage == 3 ? page3 : page2));
    int n = helpPage == 0 ? (int)(sizeof(page0) / sizeof(page0[0]))
                          : (helpPage == 1 ? (int)(sizeof(page1) / sizeof(page1[0])) : (helpPage == 3 ? (int)(sizeof(page3) / sizeof(page3[0])) : (int)(sizeof(page2) / sizeof(page2[0]))));
    for (int i = 0; i < n; i++) {
        if (!page[i][0]) continue;
        drawText(canvas, 10, 8 + i * 9, page[i], i == 0 ? HUD_AMBER : (i == n - 1 ? HUD_DIM : HUD_GREEN));
    }
}

void Game::renderMenu() {
    const RowList<MenuRow>& m = escMenu();
    // box: title, the rows, one blank line, the hint and the version line (bug B-001: nothing overlaps; U-03: the box sized from
    // the rows, its bottom 28 px under the last)
    const int top = 34, itemY0 = top + 22, rowH = 11, bottom = itemY0 + m.count * rowH + 28;
    drawPanel(canvas, 60, top, UW - 60, bottom, rgb(0, 0, 0), 210);
    drawTextCentered(canvas, UW / 2, top + 8, "STARDRIFTER SYSTEMS", HUD_AMBER);
    for (int i = 0; i < m.count; i++) {
        int y = itemY0 + i * rowH;
        drawTextCentered(canvas, UW / 2, y, m.row[i].label, i == menuSel ? HUD_WHITE : HUD_GREEN);
        if (i == menuSel) drawText(canvas, UW / 2 - 60, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 20, "ARROWS + ENTER", HUD_DIM);
    drawTextCentered(canvas, UW / 2, bottom - 10, "VESPERIS", rgb(50, 80, 60));
}

void Game::updateKeysScreen(const Input& in, double dt, double realDt) {
    if (!keymap) keymap = &ownKeymap;
    if (keysPage == 1) { updatePadScreen(in); return; }   // R-418: the controller's page
    if (!keysCapture && in.wasPressed(KEY_TAB)) { keysPage = 1; padUI.screenSel = 0; padUI.capture = -1; audio.beep = BEEP_UI; return; }
    if (keysCapture) {
        if (rawKey == KEY_ESCAPE || (rawKey < 0 && in.wasPressed(KEY_ESCAPE))) { keysCapture = false; return; }   // a controller's B (no key of the keyboard) cancels too
        if (rawKey >= 32 && rawKey < KEY_MAX) {
            int logical = KEY_ACTIONS[keysSel].key;
            keymap->map[rawKey] = logical;   // pressing rawKey now acts as the action's key
            keysCapture = false;
            audio.beep = BEEP_UI;
        }
        return;
    }
    menuStep(in, keysSel, KEY_ACTION_COUNT);
    if (in.wasPressed(KEY_LEFT)) keysSel = std::max(0, keysSel - (KEY_ACTION_COUNT + 1) / 2);
    if (in.wasPressed(KEY_RIGHT)) keysSel = std::min(KEY_ACTION_COUNT - 1, keysSel + (KEY_ACTION_COUNT + 1) / 2);
    if (enterKey(in)) { keysCapture = true; return; }
    if (in.wasPressed(KEY_BACKSPACE)) {   // reset this action: only its own key acts as it
        int logical = KEY_ACTIONS[keysSel].key;
        for (int k = 0; k < KEY_MAX; k++) if (keymap->map[k] == logical && k != logical) keymap->map[k] = k;
        keymap->map[logical] = logical;
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_ESCAPE)) {
        keymap->save(keysPath);
        if (padMap.dirty && padMap.save(padPath)) padMap.dirty = false;
        state = keysReturn;
    }
}

void Game::renderKeysScreen() {
    if (!keymap) keymap = &ownKeymap;
    if (keysPage == 1) { renderPadScreen(); return; }
    const int top = 6, bottom = UH - 6;
    drawPanel(canvas, 10, top, UW - 10, bottom, rgb(0, 0, 0), 225);
    drawTextCentered(canvas, UW / 2, top + 6, "CONTROLS  [KEYBOARD]  CONTROLLER", HUD_AMBER);   // R-418: Tab turns to the controller's page
    int half = (KEY_ACTION_COUNT + 1) / 2;
    for (int i = 0; i < KEY_ACTION_COUNT; i++) {
        int col = i / half, row = i % half;
        int x = 16 + col * 152, y = top + 18 + row * 8;
        bool sel = i == keysSel;
        int logical = KEY_ACTIONS[i].key;
        std::string keys;
        for (int k = 32; k < KEY_MAX; k++) if (keymap->map[k] == logical) keys += (keys.empty() ? "" : "/") + keyName(k);
        if (keys.empty()) keys = "-";
        drawText(canvas, x, y, trunc(KEY_ACTIONS[i].label, 15).c_str(), sel ? HUD_WHITE : HUD_GREEN);
        std::string v = (sel && keysCapture) ? "PRESS A KEY" : trunc(keys, 9);
        drawText(canvas, x + 96, y, v.c_str(), (sel && keysCapture) ? HUD_AMBER : (sel ? HUD_WHITE : HUD_CYAN));
        if (sel) drawText(canvas, x - 6, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 18, keysCapture ? "PRESS THE KEY TO USE FOR THIS ACTION (ESC CANCELS)" : "ENTER REBIND  BACKSPACE RESET  TAB CONTROLLER  ESC SAVE", HUD_DIM);
    drawTextCentered(canvas, UW / 2, bottom - 10, "SAVED TO VESPERIS_KEYS.TXT NEXT TO THE GAME", HUD_DIM);
}

// the settings: a row's change by Left and Right (Enter: Right), the settings applied after; a row that opens a screen opens it
// whichever; BACK after the rows
void Game::updateSettingsScreen(const Input& in, double dt, double realDt) {
    const RowList<SettingRow>& rows = settingRows();
    menuStep(in, settingsSel, rows.count + 1);
    int dir = (in.wasPressed(KEY_RIGHT) ? 1 : 0) - (in.wasPressed(KEY_LEFT) ? 1 : 0);
    if (enterKey(in) && settingsSel < rows.count) dir = 1;
    if (dir != 0 && settingsSel >= 0 && settingsSel < rows.count) {
        const SettingRow& r = rows.row[settingsSel];
        if (r.open) r.open(*this);
        else { r.change(*this, settings, dir); applySettings(); audio.beep = BEEP_UI; }
    }
    if (in.wasPressed(KEY_ESCAPE) || (enterKey(in) && settingsSel == rows.count)) {
        settings.save(settingsPath);
        state = GameState::MENU;
    }
}

void Game::renderSettings() {
    const RowList<SettingRow>& rows = settingRows();
    const int top = 6, bottom = UH - 6, itemY0 = top + 14, rowH = 8;   // the rows and BACK at an 8 px pitch, to fit 200 pixel rows (M6-01)
    drawPanel(canvas, 30, top, UW - 30, bottom, rgb(0, 0, 0), 215);
    drawTextCentered(canvas, UW / 2, top + 8, "SETTINGS", HUD_AMBER);
    for (int i = 0; i <= rows.count; i++) {
        int y = itemY0 + i * rowH;
        bool sel = i == settingsSel;
        if (i == rows.count) { drawTextCentered(canvas, UW / 2, y + 2, "BACK", sel ? HUD_WHITE : HUD_GREEN); if (sel) drawText(canvas, UW / 2 - 30, y + 2, ">", HUD_AMBER); continue; }
        drawText(canvas, 44, y, rows.row[i].label, sel ? HUD_WHITE : HUD_GREEN);
        std::string v = rows.row[i].value(settings);
        int vx = UW - 44 - textWidth(v.c_str());
        drawText(canvas, vx, y, v.c_str(), sel ? HUD_WHITE : HUD_GREEN);
        if (sel) { drawText(canvas, vx - 12, y, "<", HUD_AMBER); drawText(canvas, UW - 40, y, ">", HUD_AMBER); drawText(canvas, 34, y, ">", HUD_AMBER); }
    }
    drawTextCentered(canvas, UW / 2, bottom - 12, "UP/DOWN SELECT  LEFT/RIGHT CHANGE  ESC BACK", HUD_DIM);
}
