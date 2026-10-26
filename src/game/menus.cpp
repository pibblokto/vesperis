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
const int SETTINGS_ITEMS = 19;   // plus BACK (N5-05 added KEY BINDINGS)
// N5-05: the actions a player can rebind (logical key, label)
struct KeyAction { int key; const char* label; };
static const KeyAction KEY_ACTIONS[] = {
    {KEY_W, "FORWARD / DRIVE"}, {KEY_S, "BACK / REVERSE"}, {KEY_A, "LEFT / STEER"}, {KEY_D, "RIGHT / STEER"}, {KEY_SPACE, "JUMP / BRAKE"}, {KEY_LEFT_SHIFT, "SPRINT"},
    {KEY_E, "USE / VEHICLE"}, {KEY_Q, "BOARD CAPSULE"}, {KEY_B, "BUGGY / SIGNAL RADAR"}, {KEY_C, "CROUCH / CLOUDS"}, {KEY_Z, "STAND TALL / TELESCOPE"}, {KEY_V, "VIEW / VISION"}, {KEY_X, "CREATURES"},
    {KEY_M, "WAYPOINT"}, {KEY_N, "SECTOR MAP / NEXT STAR"}, {KEY_K, "RECALL CAPSULE"}, {KEY_I, "DATA SHEET"}, {KEY_T, "WARP / ALMANAC"}, {KEY_R, "AIM / SCANNER"},
    {KEY_L, "TARGET / MARK"}, {KEY_TAB, "LIST / READINGS"}, {KEY_O, "ORBIT / CHASE"}, {KEY_F, "FIELD AMP. / DRONE"}, {KEY_G, "GUIDE"}, {KEY_J, "LOG"}, {KEY_H, "HELP"}, {KEY_P, "SCREENSHOT"}, {KEY_U, "SHIP LIGHT / LAMP"}};
const int KEY_ACTION_COUNT = (int)(sizeof(KEY_ACTIONS) / sizeof(KEY_ACTIONS[0]));

}

void Game::renderTitle() {
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
    int n = helpPage == 0 ? (int)(sizeof(page0) / sizeof(page0[0])) : (helpPage == 1 ? (int)(sizeof(page1) / sizeof(page1[0])) : (helpPage == 3 ? (int)(sizeof(page3) / sizeof(page3[0])) : (int)(sizeof(page2) / sizeof(page2[0]))));
    for (int i = 0; i < n; i++) {
        if (!page[i][0]) continue;
        drawText(canvas, 10, 8 + i * 9, page[i], i == 0 ? HUD_AMBER : (i == n - 1 ? HUD_DIM : HUD_GREEN));
    }
}

void Game::renderMenu() {
    const char* const items[] = {"RESUME", "AUTO-EXPLORE", "SAVE EXPEDITION", "LOAD EXPEDITION", "SETTINGS", "HELP", "NEW EXPEDITION", "QUIT TO DESKTOP"};   // R-419: AUTO-EXPLORE
    const int n = (int)(sizeof(items) / sizeof(items[0]));
    // box: title, items, one blank line, the hint and the version line (bug B-001: nothing overlaps; R-419: eight rows, the box taller)
    const int top = 34, bottom = UH - 28, itemY0 = top + 22, rowH = 11;
    drawPanel(canvas, 60, top, UW - 60, bottom, rgb(0, 0, 0), 210);
    drawTextCentered(canvas, UW / 2, top + 8, "STARDRIFTER SYSTEMS", HUD_AMBER);
    for (int i = 0; i < n; i++) {
        int y = itemY0 + i * rowH;
        drawTextCentered(canvas, UW / 2, y, items[i], i == menuSel ? HUD_WHITE : HUD_GREEN);
        if (i == menuSel) drawText(canvas, UW / 2 - 60, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 20, "ARROWS + ENTER", HUD_DIM);
    drawTextCentered(canvas, UW / 2, bottom - 10, "VESPERIS", rgb(50, 80, 60));
}

void Game::adjustSetting(int item, int dir) {
    Settings& s = settings;
    if (item == 18) { keysSel = 0; keysCapture = false; keysPage = padInUse() ? 1 : 0; keysReturn = GameState::SETTINGS; padUI.capture = -1; state = GameState::KEYS; return; }   // N5-05; R-418: the controller's page first when a controller is in use
    switch (item) {
        case 0: s.mouseSensitivity += 0.1 * dir; break;
        case 1: s.invertY = !s.invertY; break;
        case 2: s.fovDeg += 5 * dir; break;
        case 3: s.masterVolume += 0.05 * dir; break;
        case 4: s.scanlines = !s.scanlines; break;
        case 5: s.windowScale += dir; s.clampAll(); wantsWindowScale = s.windowScale; break;
        case 6: s.renderScale += dir; break;
        case 7: s.mushMode = (s.mushMode + 4 + dir) % 4; break;
        case 8: s.shadingMode = (s.shadingMode + 3 + dir) % 3; break;
        case 9: s.crt = !s.crt; break;
        case 10: s.bloom = !s.bloom; break;
        case 11: s.sprintToggle = !s.sprintToggle; break;
        case 12: s.sprintSpeed += 0.25 * dir; break;
        case 13: s.cabin = !s.cabin; break;
        case 14: s.clockMode = 1 - s.clockMode; break;
        case 15: s.aspect = 1 - s.aspect; break;
        case 16: s.dither = !s.dither; break;
        case 17: s.hudColor = (s.hudColor + 3 + dir) % 3; break;
    }
    applySettings();
    audio.beep = BEEP_UI;
}

void Game::updateKeysScreen(const Input& in) {
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

void Game::updateSettingsScreen(const Input& in) {
    const int n = SETTINGS_ITEMS + 1;
    menuStep(in, settingsSel, n);
    int dir = (in.wasPressed(KEY_RIGHT) ? 1 : 0) - (in.wasPressed(KEY_LEFT) ? 1 : 0);
    if (enterKey(in) && settingsSel < SETTINGS_ITEMS) dir = 1;
    if (dir != 0 && settingsSel < SETTINGS_ITEMS) adjustSetting(settingsSel, dir);
    if (in.wasPressed(KEY_ESCAPE) || (enterKey(in) && settingsSel == SETTINGS_ITEMS)) {
        settings.save(settingsPath);
        state = GameState::MENU;
    }
}

void Game::renderSettings() {
    const Settings& s = settings;
    const int top = 6, bottom = UH - 6, itemY0 = top + 14, rowH = 8;   // 18 items + BACK fit 200 rows at an 8 px pitch (M6-01)
    drawPanel(canvas, 30, top, UW - 30, bottom, rgb(0, 0, 0), 215);
    drawTextCentered(canvas, UW / 2, top + 8, "SETTINGS", HUD_AMBER);
    std::string values[SETTINGS_ITEMS] = {
        fmt("%.1f", s.mouseSensitivity), s.invertY ? "ON" : "OFF", fmt("%.0f%c", s.fovDeg, CH_DEGREE),
        fmt("%.0f%%", s.masterVolume * 100), s.scanlines ? "ON" : "OFF", fmt("%dX  (%dX%d)", s.windowScale, 320 * s.windowScale, 200 * s.windowScale),
        fmt("%dX  (%dX%d)", s.renderScale, 320 * s.renderScale, 200 * s.renderScale), s.mushMode == 0 ? "CLASSIC" : (s.mushMode == 1 ? "SOFT" : (s.mushMode == 2 ? "SHARP" : "FINE")),
        s.shadingMode == 0 ? "FLAT CELLS" : (s.shadingMode == 1 ? "STEPPED" : "SMOOTH"), s.crt ? "ON" : "OFF", s.bloom ? "ON" : "OFF", s.sprintToggle ? "TOGGLE" : "HOLD", fmt("X%.2f", s.sprintSpeed), s.cabin ? "CABIN" : "COCKPIT", s.clockMode ? "REAL TIME (UTC)" : "GAME TIME",
        s.aspect ? "4:3 (TALL PIXELS)" : "SQUARE PIXELS", s.dither ? "ON" : "OFF", s.hudColor == 0 ? "GREEN" : (s.hudColor == 1 ? "AMBER" : "WHITE"), "ENTER TO OPEN"};
    const char* const names[SETTINGS_ITEMS] = {"MOUSE SENSITIVITY", "INVERT MOUSE Y", "FIELD OF VIEW", "MASTER VOLUME", "SCANLINES", "WINDOW SIZE", "RENDER SCALE", "MUSH FILTER", "TERRAIN SHADING", "CRT SHADER", "BLOOM", "SPRINT KEY", "SPRINT SPEED", "SHIP VIEW", "CLOCK", "ASPECT", "DITHER", "HUD COLOUR", "CONTROLS"};
    for (int i = 0; i <= SETTINGS_ITEMS; i++) {
        int y = itemY0 + i * rowH;
        bool sel = i == settingsSel;
        if (i == SETTINGS_ITEMS) { drawTextCentered(canvas, UW / 2, y + 2, "BACK", sel ? HUD_WHITE : HUD_GREEN); if (sel) drawText(canvas, UW / 2 - 30, y + 2, ">", HUD_AMBER); continue; }
        drawText(canvas, 44, y, names[i], sel ? HUD_WHITE : HUD_GREEN);
        std::string v = values[i];
        int vx = UW - 44 - textWidth(v.c_str());
        drawText(canvas, vx, y, v.c_str(), sel ? HUD_WHITE : HUD_GREEN);
        if (sel) { drawText(canvas, vx - 12, y, "<", HUD_AMBER); drawText(canvas, UW - 40, y, ">", HUD_AMBER); drawText(canvas, 34, y, ">", HUD_AMBER); }
    }
    drawTextCentered(canvas, UW / 2, bottom - 12, "UP/DOWN SELECT  LEFT/RIGHT CHANGE  ESC BACK", HUD_DIM);
}
