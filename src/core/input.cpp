// Key names, the physical-to-logical key map (M6-02) and the gamepad-to-keys mapping.
#include "input.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cmath>

namespace {
struct KeyNameEntry { const char* name; int key; };
const KeyNameEntry KEY_NAMES[] = {
    {"SPACE", KEY_SPACE}, {"APOSTROPHE", KEY_APOSTROPHE}, {"COMMA", KEY_COMMA}, {"MINUS", KEY_MINUS}, {"PERIOD", KEY_PERIOD}, {"SLASH", KEY_SLASH},
    {"SEMICOLON", KEY_SEMICOLON}, {"EQUAL", KEY_EQUAL}, {"LBRACKET", KEY_LEFT_BRACKET}, {"BACKSLASH", KEY_BACKSLASH}, {"RBRACKET", KEY_RIGHT_BRACKET}, {"GRAVE", KEY_GRAVE},
    {"ESC", KEY_ESCAPE}, {"ESCAPE", KEY_ESCAPE}, {"ENTER", KEY_ENTER}, {"TAB", KEY_TAB}, {"BACKSPACE", KEY_BACKSPACE}, {"INSERT", KEY_INSERT}, {"DELETE", KEY_DELETE},
    {"RIGHT", KEY_RIGHT}, {"LEFT", KEY_LEFT}, {"DOWN", KEY_DOWN}, {"UP", KEY_UP}, {"PGUP", KEY_PAGE_UP}, {"PGDN", KEY_PAGE_DOWN}, {"HOME", KEY_HOME}, {"END", KEY_END},
    {"LSHIFT", KEY_LEFT_SHIFT}, {"LCTRL", KEY_LEFT_CONTROL}, {"LALT", KEY_LEFT_ALT}, {"RSHIFT", KEY_RIGHT_SHIFT}, {"RCTRL", KEY_RIGHT_CONTROL}, {"RALT", KEY_RIGHT_ALT},
};
}

int keyFromName(const std::string& raw) {
    std::string n;
    for (char c : raw) n += (char)toupper((unsigned char)c);
    if (n.size() == 1 && ((n[0] >= 'A' && n[0] <= 'Z') || (n[0] >= '0' && n[0] <= '9'))) return n[0];
    if (n.size() >= 2 && n[0] == 'F') { int f = atoi(n.c_str() + 1); if (f >= 1 && f <= 12 && n.find_first_not_of("0123456789", 1) == std::string::npos) return KEY_F1 + f - 1; }
    for (const KeyNameEntry& e : KEY_NAMES) if (n == e.name) return e.key;
    return -1;
}

std::string keyName(int key) {
    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) return std::string(1, (char)key);
    if (key >= KEY_F1 && key <= KEY_F12) return "F" + std::to_string(key - KEY_F1 + 1);
    for (const KeyNameEntry& e : KEY_NAMES) if (e.key == key) return e.name;
    return "?";
}

KeyMap::KeyMap() { for (int k = 0; k < KEY_MAX; k++) map[k] = k; }

bool KeyMap::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    int n = 0;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream is(line);
        std::string a, b;
        if (!(is >> a >> b)) continue;
        int ka = keyFromName(a), kb = keyFromName(b);
        if (ka < 0 || kb < 0) continue;
        map[ka] = kb;   // pressing the physical key `a` acts as the logical key `b`
        n++;
    }
    return n > 0;
}

bool KeyMap::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "# vesperis_keys.txt: physical key -> logical key, one pair per line (written by the key bindings screen)\n";
    for (int k = 0; k < KEY_MAX; k++) if (map[k] != k && map[k] >= 0) f << keyName(k) << " " << keyName(map[k]) << "\n";
    return true;
}

bool KeyMap::identity() const {
    for (int k = 0; k < KEY_MAX; k++) if (map[k] != k) return false;
    return true;
}

void KeyMap::apply(Input& in) const {
    if (identity()) return;
    bool down[KEY_MAX], pressed[KEY_MAX];
    memset(down, 0, sizeof down); memset(pressed, 0, sizeof pressed);
    for (int k = 0; k < KEY_MAX; k++) {
        int to = map[k];
        if (to < 0 || to >= KEY_MAX) continue;
        if (in.down[k]) down[to] = true;
        if (in.pressed[k]) pressed[to] = true;
    }
    memcpy(in.down, down, sizeof down); memcpy(in.pressed, pressed, sizeof pressed);
}

// The pad: left stick walks (analogue in moveX/moveY, also W/A/S/D beyond half deflection), the right stick
// looks (as mouse motion), buttons map to the keys listed in docs/reference/07 (A enter/E, B escape, X space,
// Y I, bumpers shift/V, triggers C (crouch) and shift (sprint), d-pad arrows, start escape, back tab).
void applyPad(const PadState& cur, PadState& prev, Input& in, double lookSens) {
    if (!cur.present) { prev = cur; return; }
    auto dead = [](double v) { return std::fabs(v) < 0.18 ? 0.0 : (v - (v > 0 ? 0.18 : -0.18)) / 0.82; };
    in.moveX = dead(cur.lx);
    in.moveY = -dead(cur.ly);
    in.mouseDx += dead(cur.rx) * 14.0 * lookSens;
    in.mouseDy += dead(cur.ry) * 14.0 * lookSens;
    static const int BUTTON_KEYS[PAD_BUTTONS] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE, KEY_I, KEY_LEFT_SHIFT, KEY_V, KEY_ESCAPE, KEY_TAB, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_C, KEY_LEFT_SHIFT, KEY_E, KEY_M};
    for (int b = 0; b < PAD_BUTTONS; b++) {
        int k = BUTTON_KEYS[b];
        if (cur.buttons[b]) { in.down[k] = true; if (!prev.buttons[b]) in.pressed[k] = true; }
    }
    // triggers as buttons 12 (left: crouch) and 13 (right: sprint)
    bool lt = cur.lt > 0.5, rt = cur.rt > 0.5, plt = prev.lt > 0.5, prt = prev.rt > 0.5;
    if (lt) { in.down[KEY_C] = true; if (!plt) in.pressed[KEY_C] = true; }
    if (rt) { in.down[KEY_LEFT_SHIFT] = true; if (!prt) in.pressed[KEY_LEFT_SHIFT] = true; }
    prev = cur;
}
