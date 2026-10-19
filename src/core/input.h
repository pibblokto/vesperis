// Platform-independent input snapshot. Key codes follow the GLFW/raylib
// numbering so the raylib front end can copy them straight through.
#pragma once
#include <cstring>
#include <map>
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

// R-418: the first controller as the platform reads it (platform_raylib.cpp): its standard layout when the system knows the device
// (GLFW's gamepad mappings, SDL's controller database: the Xbox names by position, a PlayStation pad's cross being A), and the raw
// device always (every button, axis and hat it reports), so a controller nobody mapped can still be bound (`PadMap`)
const int PAD_BUTTONS = 15;   // the standard layout's buttons, GLFW's order
enum PadButton { PB_A, PB_B, PB_X, PB_Y, PB_LB, PB_RB, PB_BACK, PB_START, PB_GUIDE, PB_L3, PB_R3, PB_UP, PB_RIGHT, PB_DOWN, PB_LEFT };
const int PAD_AXES = 6;       // LX LY RX RY LT RT: the sticks -1..1 (right and down positive), the triggers -1 at rest to 1
const int PAD_RAW_BUTTONS = 32, PAD_RAW_AXES = 16, PAD_RAW_HATS = 4;
struct PadState {
    bool present = false;
    bool mapped = false;                          // the standard layout below is the device's
    bool buttons[PAD_BUTTONS] = {};
    float axes[PAD_AXES] = {0, 0, 0, 0, -1, -1};
    int rawButtonCount = 0, rawAxisCount = 0, rawHatCount = 0;
    bool rawButtons[PAD_RAW_BUTTONS] = {};
    float rawAxes[PAD_RAW_AXES] = {};
    unsigned char rawHats[PAD_RAW_HATS] = {};     // bits: 1 up, 2 right, 4 down, 8 left
    char name[64] = {};
    char guid[40] = {};
};

struct Input {
    bool down[KEY_MAX];
    bool pressed[KEY_MAX];
    double mouseDx = 0, mouseDy = 0;
    double moveX = 0, moveY = 0;   // M6-02: analogue movement from a controller's stick (-1..1, forward and right positive), 0 when keys are used
    PadState pad;                  // R-418: the controller this frame (the game translates it: game/pad.cpp)
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

// R-418: a physical control of the controller, as a number: the standard layout's buttons (0-14, `PadButton`), the halves of its axes
// (16 + 2 axis + 1 for the positive half), the raw buttons (32 + n), the halves of the raw axes (64 + 2 axis + 1 positive), the raw hats'
// directions (96 + 4 hat + 0 up, 1 right, 2 down, 3 left) and a raw axis read whole as a trigger, from its rest at -1 (112 + axis) or at
// +1 (128 + axis)
constexpr int PADC_NONE = -1, PADC_STD_AXIS = 16, PADC_RAW_BUTTON = 32, PADC_RAW_AXIS = 64, PADC_RAW_HAT = 96, PADC_RAW_TRIGGER = 112,
              PADC_RAW_TRIGGER_INV = 128, PADC_COUNT = 144;
double padControlValue(const PadState& p, int c);   // how far it is pressed, 0..1 (a stick's half from the centre, a trigger from its rest)
double padAxisValue(const PadState& p, int c);      // an axis half read signed, -1..1 (its own half positive); a button or a trigger one way, 0..1
bool padIsAxisHalf(int c);
std::string padControlName(int c);                  // the file's: "A", "LT+", "B7", "A2-", "H0U", "T5", "T5R"
int padControlFromName(const std::string& s);       // -1 when unknown
std::string padControlLabel(int c);                 // the screen's: "A", "D-PAD UP", "LEFT STICK UP", "BUTTON 7", "AXIS 2-", "HAT UP", "AXIS 5"

// R-418: what the game reads of a controller: sixteen buttons by the Xbox names and places (A the bottom face button, B the right, X the
// left, Y the top) and two sticks, each bound to any physical control of the device in use. Each device starts from its defaults (the
// standard layout, or a guess at a raw device's: buttons 0-3 the face, the first hat the d-pad, axes 0-1 the left stick); the explorer's
// changes are kept by device in `vesperis_pad.txt` (the CONTROLS screen writes it)
enum PadRole { PR_A, PR_B, PR_X, PR_Y, PR_LB, PR_RB, PR_LT, PR_RT, PR_SELECT, PR_START, PR_L3, PR_R3, PR_UP, PR_DOWN, PR_LEFT, PR_RIGHT,
               PR_BUTTONS, PR_WALK_X = PR_BUTTONS, PR_WALK_Y, PR_LOOK_X, PR_LOOK_Y, PR_COUNT };   // a stick's axis is bound to the half read as right, or down
const char* padRoleName(int r);                     // the file's: "A" .. "RIGHT", "WALK_X" .. "LOOK_Y"
struct PadRead { bool down[PR_BUTTONS] = {}; double walkX = 0, walkY = 0, lookX = 0, lookY = 0; };   // the sticks past the dead zone (right and down positive)
struct PadMap {
    int bind[PR_COUNT];
    int deflt[PR_COUNT];                            // the device's defaults, taken as it arrived
    std::string device;                             // the device the binding is of (its GUID, else its name), "" none yet
    std::map<std::string, std::pair<std::string, std::map<int, int>>> kept;   // by device: its name and the roles the explorer bound
    bool dirty = false;                             // a binding changed since the file was read or written
    PadMap();
    static void defaults(const PadState& p, int out[PR_COUNT]);
    static std::string deviceKey(const PadState& p);
    void use(const PadState& p);                    // the device in use: its defaults, then what the explorer bound of it (once per device)
    void set(const PadState& p, int role, int control);
    void reset(const PadState& p);                  // the device back to its defaults
    bool custom(const PadState& p) const;
    PadRead read(const PadState& p, double deadZone) const;
    bool load(const std::string& path);
    bool save(const std::string& path) const;
};
// R-418: the control the explorer just moved: a button or a hat's direction pressed since `prev` (last frame), an axis half more than 0.6
// from where it rested in `rest`; -1 none. `axes` takes the axis halves only (a stick's capture)
int padCapture(const PadState& prev, const PadState& rest, const PadState& now, bool axes);
