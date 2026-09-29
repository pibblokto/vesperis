// Platform-independent input snapshot. Key codes follow the GLFW/raylib
// numbering so the raylib front end can copy them straight through.
#pragma once
#include <cstring>
#include <string>

enum Key {
    KEY_SPACE = 32, KEY_APOSTROPHE = 39, KEY_COMMA = 44, KEY_MINUS = 45, KEY_PERIOD = 46, KEY_SLASH = 47,
    KEY_0 = 48, KEY_1 = 49, KEY_2 = 50, KEY_3 = 51, KEY_4 = 52, KEY_5 = 53, KEY_6 = 54, KEY_7 = 55, KEY_8 = 56, KEY_9 = 57,
    KEY_SEMICOLON = 59, KEY_EQUAL = 61,
    KEY_A = 65, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
    KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_LEFT_BRACKET = 91, KEY_BACKSLASH = 92, KEY_RIGHT_BRACKET = 93, KEY_GRAVE = 96,
    KEY_ESCAPE = 256, KEY_ENTER = 257, KEY_TAB = 258, KEY_BACKSPACE = 259, KEY_INSERT = 260, KEY_DELETE = 261,
    KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265, KEY_PAGE_UP = 266, KEY_PAGE_DOWN = 267,
    KEY_HOME = 268, KEY_END = 269,
    KEY_F1 = 290, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_LEFT_SHIFT = 340, KEY_LEFT_CONTROL = 341, KEY_LEFT_ALT = 342, KEY_RIGHT_SHIFT = 344, KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_MAX = 400
};

struct Input {
    bool down[KEY_MAX];
    bool pressed[KEY_MAX];
    double mouseDx = 0, mouseDy = 0;
    double moveX = 0, moveY = 0;   // M6-02: analogue movement from a gamepad stick (-1..1), 0 when keys are used
    int wheel = 0;
    bool mouseDown[3];
    bool mousePressed[3];
    Input() { clear(); }
    void clear() {
        memset(down, 0, sizeof(down)); memset(pressed, 0, sizeof(pressed));
        memset(mouseDown, 0, sizeof(mouseDown)); memset(mousePressed, 0, sizeof(mousePressed));
        mouseDx = mouseDy = 0; wheel = 0; moveX = moveY = 0;
    }
    void newFrame() {
        memset(pressed, 0, sizeof(pressed)); memset(mousePressed, 0, sizeof(mousePressed));
        mouseDx = mouseDy = 0; wheel = 0; moveX = moveY = 0;
    }
    bool isDown(int k) const { return k >= 0 && k < KEY_MAX && down[k]; }
    bool wasPressed(int k) const { return k >= 0 && k < KEY_MAX && pressed[k]; }
    bool shift() const { return down[KEY_LEFT_SHIFT] || down[KEY_RIGHT_SHIFT]; }
    bool ctrl() const { return down[KEY_LEFT_CONTROL] || down[KEY_RIGHT_CONTROL]; }
    bool alt() const { return down[KEY_LEFT_ALT] || down[KEY_RIGHT_ALT]; }
};

// M6-02: key names ("W", "SPACE", "LSHIFT", "F5", ...) for the key map file and the help text
int keyFromName(const std::string& name);   // -1 when unknown
std::string keyName(int key);

// M6-02: physical-to-logical key map loaded from `vesperis_keys.txt` (lines "PHYSICAL LOGICAL", e.g. "Z W" on an
// AZERTY keyboard; swap both ways with two lines). Applied to the Input after the platform filled it.
struct KeyMap {
    int map[KEY_MAX];
    KeyMap();
    bool load(const std::string& path);
    bool identity() const;
    void apply(Input& in) const;
    bool save(const std::string& path) const;   // N5-05: writes the pairs that differ from the identity
};

// M6-02: a gamepad snapshot (raylib's numbering for the 16 buttons: 0 A, 1 B, 2 X, 3 Y, 4 LB, 5 RB, 6 start,
// 7 back, 8-11 d-pad up/down/left/right, 12 left thumb, 13 right thumb, 14 guide, 15 spare)
const int PAD_BUTTONS = 16;
struct PadState {
    bool present = false;
    double lx = 0, ly = 0, rx = 0, ry = 0, lt = -1, rt = -1;
    bool buttons[PAD_BUTTONS] = {false};
};
void applyPad(const PadState& cur, PadState& prev, Input& in, double lookSens);
