// Key names, the physical-to-logical key map (M6-02) and the controller's controls and bindings (R-418).
#include "input.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

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

// ---- R-418: the controller --------------------------------------------------------------------------------------------------------------

namespace {
const char* const STD_BUTTON_NAMES[PAD_BUTTONS] = {"A", "B", "X", "Y", "LB", "RB", "BACK", "START", "GUIDE", "L3", "R3", "UP", "RIGHT", "DOWN", "LEFT"};
const char* const STD_BUTTON_LABELS[PAD_BUTTONS] = {"A", "B", "X", "Y", "LB", "RB", "SELECT", "START", "GUIDE", "L3", "R3", "D-PAD UP", "D-PAD RIGHT", "D-PAD DOWN", "D-PAD LEFT"};
const char* const STD_AXIS_NAMES[PAD_AXES] = {"LX", "LY", "RX", "RY", "LT", "RT"};
const char* const ROLE_NAMES[PR_COUNT] = {"A", "B", "X", "Y", "LB", "RB", "LT", "RT", "SELECT", "START", "L3", "R3", "UP", "DOWN", "LEFT", "RIGHT", "WALK_X", "WALK_Y", "LOOK_X", "LOOK_Y"};
const char HAT_DIRS[4] = {'U', 'R', 'D', 'L'};
const char* const HAT_LABELS[4] = {"UP", "RIGHT", "DOWN", "LEFT"};
double clamp01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
}

double padControlValue(const PadState& p, int c) {
    if (c < 0) return 0;
    if (c < PAD_BUTTONS) return p.mapped && p.buttons[c] ? 1 : 0;
    if (c < PADC_STD_AXIS) return 0;
    if (c < PADC_STD_AXIS + 2 * PAD_AXES) {
        if (!p.mapped) return 0;
        const int a = (c - PADC_STD_AXIS) / 2; const bool pos = (c - PADC_STD_AXIS) & 1; const double v = p.axes[a];
        if (a >= 4) return pos ? clamp01((v + 1) * 0.5) : 0;   // a trigger, from its rest at -1
        return clamp01(pos ? v : -v);
    }
    if (c < PADC_RAW_BUTTON) return 0;
    if (c < PADC_RAW_AXIS) { const int n = c - PADC_RAW_BUTTON; return n < p.rawButtonCount && p.rawButtons[n] ? 1 : 0; }
    if (c < PADC_RAW_HAT) { const int a = (c - PADC_RAW_AXIS) / 2; if (a >= p.rawAxisCount) return 0; const double v = p.rawAxes[a]; return clamp01((c - PADC_RAW_AXIS) & 1 ? v : -v); }
    if (c < PADC_RAW_TRIGGER) { const int h = (c - PADC_RAW_HAT) / 4, d = (c - PADC_RAW_HAT) % 4; return h < p.rawHatCount && (p.rawHats[h] & (1 << d)) ? 1 : 0; }
    if (c < PADC_RAW_TRIGGER_INV) { const int a = c - PADC_RAW_TRIGGER; return a < p.rawAxisCount ? clamp01((p.rawAxes[a] + 1) * 0.5) : 0; }
    if (c < PADC_COUNT) { const int a = c - PADC_RAW_TRIGGER_INV; return a < p.rawAxisCount ? clamp01((1 - p.rawAxes[a]) * 0.5) : 0; }
    return 0;
}

double padAxisValue(const PadState& p, int c) {
    if (c >= PADC_STD_AXIS && c < PADC_STD_AXIS + 8) {   // a stick's half: the axis signed
        if (!p.mapped) return 0;
        const int a = (c - PADC_STD_AXIS) / 2; const double v = p.axes[a];
        return (c - PADC_STD_AXIS) & 1 ? v : -v;
    }
    if (c >= PADC_RAW_AXIS && c < PADC_RAW_HAT) {
        const int a = (c - PADC_RAW_AXIS) / 2; if (a >= p.rawAxisCount) return 0;
        const double v = p.rawAxes[a];
        return (c - PADC_RAW_AXIS) & 1 ? v : -v;
    }
    return padControlValue(p, c);   // a button or a trigger: one way only
}

std::string padControlName(int c) {
    char b[16];
    if (c < 0) return "-";
    if (c < PAD_BUTTONS) return STD_BUTTON_NAMES[c];
    if (c >= PADC_STD_AXIS && c < PADC_STD_AXIS + 2 * PAD_AXES) { snprintf(b, sizeof b, "%s%c", STD_AXIS_NAMES[(c - PADC_STD_AXIS) / 2], (c - PADC_STD_AXIS) & 1 ? '+' : '-'); return b; }
    if (c >= PADC_RAW_BUTTON && c < PADC_RAW_AXIS) { snprintf(b, sizeof b, "B%d", c - PADC_RAW_BUTTON); return b; }
    if (c >= PADC_RAW_AXIS && c < PADC_RAW_HAT) { snprintf(b, sizeof b, "A%d%c", (c - PADC_RAW_AXIS) / 2, (c - PADC_RAW_AXIS) & 1 ? '+' : '-'); return b; }
    if (c >= PADC_RAW_HAT && c < PADC_RAW_TRIGGER) { snprintf(b, sizeof b, "H%d%c", (c - PADC_RAW_HAT) / 4, HAT_DIRS[(c - PADC_RAW_HAT) % 4]); return b; }
    if (c >= PADC_RAW_TRIGGER && c < PADC_RAW_TRIGGER_INV) { snprintf(b, sizeof b, "T%d", c - PADC_RAW_TRIGGER); return b; }
    if (c >= PADC_RAW_TRIGGER_INV && c < PADC_COUNT) { snprintf(b, sizeof b, "T%dR", c - PADC_RAW_TRIGGER_INV); return b; }
    return "-";
}

int padControlFromName(const std::string& raw) {
    std::string s;
    for (char ch : raw) s += (char)toupper((unsigned char)ch);
    for (int c = 0; c < PADC_COUNT; c++) if (padControlName(c) == s) return c;
    return -1;
}

std::string padControlLabel(int c) {
    char b[24];
    if (c < 0) return "-";
    if (c < PAD_BUTTONS) return STD_BUTTON_LABELS[c];
    if (c >= PADC_STD_AXIS && c < PADC_STD_AXIS + 2 * PAD_AXES) {
        const int a = (c - PADC_STD_AXIS) / 2; const bool pos = (c - PADC_STD_AXIS) & 1;
        if (a >= 4) return std::string(STD_AXIS_NAMES[a]) + (pos ? "" : "-");
        snprintf(b, sizeof b, "%s STICK %s", a < 2 ? "L" : "R", a % 2 == 0 ? (pos ? "RIGHT" : "LEFT") : (pos ? "DOWN" : "UP"));
        return b;
    }
    if (c >= PADC_RAW_BUTTON && c < PADC_RAW_AXIS) { snprintf(b, sizeof b, "BUTTON %d", c - PADC_RAW_BUTTON); return b; }
    if (c >= PADC_RAW_AXIS && c < PADC_RAW_HAT) { snprintf(b, sizeof b, "AXIS %d%c", (c - PADC_RAW_AXIS) / 2, (c - PADC_RAW_AXIS) & 1 ? '+' : '-'); return b; }
    if (c >= PADC_RAW_HAT && c < PADC_RAW_TRIGGER) {
        const int h = (c - PADC_RAW_HAT) / 4;
        if (h) snprintf(b, sizeof b, "HAT%d %s", h + 1, HAT_LABELS[(c - PADC_RAW_HAT) % 4]); else snprintf(b, sizeof b, "HAT %s", HAT_LABELS[(c - PADC_RAW_HAT) % 4]);
        return b;
    }
    if (c >= PADC_RAW_TRIGGER && c < PADC_RAW_TRIGGER_INV) { snprintf(b, sizeof b, "AXIS %d", c - PADC_RAW_TRIGGER); return b; }
    if (c >= PADC_RAW_TRIGGER_INV && c < PADC_COUNT) { snprintf(b, sizeof b, "AXIS %d INV", c - PADC_RAW_TRIGGER_INV); return b; }
    return "-";
}

PadMap::PadMap() { PadState none; defaults(none, deflt); for (int r = 0; r < PR_COUNT; r++) bind[r] = deflt[r]; }

// a known device: its standard layout. A raw one: the common order of the kernels' and XInput's drivers (A B X Y, the bumpers, SELECT and
// START, the thumbs after them, past a guide button when there are eleven or more), the first hat the d-pad, axes 0 and 1 the left stick
// and, with six axes, the triggers at 2 and 5 with the right stick at 3 and 4 (Linux's order), else the right stick at 2 and 3
void PadMap::defaults(const PadState& p, int out[PR_COUNT]) {
    if (p.mapped || !p.present) {
        const int layout[PR_COUNT] = {PB_A, PB_B, PB_X, PB_Y, PB_LB, PB_RB, PADC_STD_AXIS + 9, PADC_STD_AXIS + 11, PB_BACK, PB_START, PB_L3, PB_R3, PB_UP, PB_DOWN, PB_LEFT, PB_RIGHT,
                                   PADC_STD_AXIS + 1, PADC_STD_AXIS + 3, PADC_STD_AXIS + 5, PADC_STD_AXIS + 7};
        for (int r = 0; r < PR_COUNT; r++) out[r] = layout[r];
        return;
    }
    for (int r = 0; r < PR_COUNT; r++) out[r] = PADC_NONE;
    const int nb = p.rawButtonCount, na = std::min(p.rawAxisCount, PAD_RAW_AXES);
    auto button = [&](int n) { return n < nb ? PADC_RAW_BUTTON + n : PADC_NONE; };
    out[PR_A] = button(0); out[PR_B] = button(1); out[PR_X] = button(2); out[PR_Y] = button(3); out[PR_LB] = button(4); out[PR_RB] = button(5);
    out[PR_SELECT] = button(6); out[PR_START] = button(7);
    out[PR_L3] = button(nb >= 11 ? 9 : 8); out[PR_R3] = button(nb >= 11 ? 10 : 9);
    if (p.rawHatCount > 0) { out[PR_UP] = PADC_RAW_HAT + 0; out[PR_RIGHT] = PADC_RAW_HAT + 1; out[PR_DOWN] = PADC_RAW_HAT + 2; out[PR_LEFT] = PADC_RAW_HAT + 3; }
    // the axes as they rest when the device arrives: a stick's in the middle, a trigger's at one end (a guessed stick resting off-centre would
    // walk on by itself, a guessed trigger resting in the middle would hold its button: neither is bound)
    auto centred = [&](int a) { return a < na && std::fabs(p.rawAxes[a]) < 0.5; };
    auto low = [&](int a) { return a < na && p.rawAxes[a] < -0.8; };
    auto stick = [&](int rx, int ry, int ax, int ay) { if (centred(ax) && centred(ay)) { out[rx] = PADC_RAW_AXIS + 2 * ax + 1; out[ry] = PADC_RAW_AXIS + 2 * ay + 1; } };
    stick(PR_WALK_X, PR_WALK_Y, 0, 1);
    if (na >= 6 && low(2) && low(5)) { stick(PR_LOOK_X, PR_LOOK_Y, 3, 4); out[PR_LT] = PADC_RAW_TRIGGER + 2; out[PR_RT] = PADC_RAW_TRIGGER + 5; }   // Linux's order
    else if (na >= 4) {
        stick(PR_LOOK_X, PR_LOOK_Y, 2, 3);   // the right stick on the third and fourth axes (DirectInput's Z and its rotation), the triggers after them
        if (na >= 6 && low(4) && low(5)) { out[PR_LT] = PADC_RAW_TRIGGER + 4; out[PR_RT] = PADC_RAW_TRIGGER + 5; }
    }
}

std::string PadMap::deviceKey(const PadState& p) {
    std::string k = p.guid[0] ? p.guid : p.name;
    for (char& ch : k) if (ch == ' ' || ch == '\t') ch = '_';
    return k.empty() ? "PAD" : k;
}

void PadMap::use(const PadState& p) {
    const std::string k = (p.mapped ? "S:" : "R:") + deviceKey(p);   // a device the system learns to map later starts again from the standard layout
    if (k == device) return;
    device = k;
    defaults(p, deflt);   // as the device arrives (its axes at rest)
    for (int r = 0; r < PR_COUNT; r++) bind[r] = deflt[r];
    auto it = kept.find(k);
    if (it != kept.end()) for (const auto& rc : it->second.second) if (rc.first >= 0 && rc.first < PR_COUNT) bind[rc.first] = rc.second;
}

void PadMap::set(const PadState& p, int role, int control) {
    if (role < 0 || role >= PR_COUNT) return;
    use(p);
    auto& entry = kept[device];
    entry.first = p.name;
    if (control == deflt[role]) entry.second.erase(role); else entry.second[role] = control;
    bind[role] = control;
    dirty = true;
}

void PadMap::reset(const PadState& p) { use(p); kept.erase(device); for (int r = 0; r < PR_COUNT; r++) bind[r] = deflt[r]; dirty = true; }

bool PadMap::custom(const PadState& p) const {
    auto it = kept.find((p.mapped ? "S:" : "R:") + deviceKey(p));
    return it != kept.end() && !it->second.second.empty();
}

PadRead PadMap::read(const PadState& p, double deadZone) const {
    PadRead r;
    if (!p.present) return r;
    for (int i = 0; i < PR_BUTTONS; i++) r.down[i] = padControlValue(p, bind[i]) > 0.4;
    auto stick = [&](int rx, int ry, double& x, double& y) {   // a round dead zone, the rest rescaled to reach 1 at the rim
        x = padAxisValue(p, bind[rx]); y = padAxisValue(p, bind[ry]);
        const double m = std::sqrt(x * x + y * y);
        if (m <= deadZone || m < 1e-9) { x = y = 0; return; }
        const double s = std::min(1.0, (m - deadZone) / std::max(1e-6, 1 - deadZone)) / m;
        x *= s; y *= s;
    };
    stick(PR_WALK_X, PR_WALK_Y, r.walkX, r.walkY);
    stick(PR_LOOK_X, PR_LOOK_Y, r.lookX, r.lookY);
    return r;
}

bool PadMap::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    kept.clear(); device.clear(); dirty = false;
    std::string line, cur;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream is(line);
        std::string a, b;
        if (!(is >> a >> b)) continue;
        if (a == "pad") { cur = b; std::string name; std::getline(is, name); while (!name.empty() && name[0] == ' ') name.erase(0, 1); kept[cur].first = name; continue; }
        if (cur.empty()) continue;
        int role = -1;
        for (int r = 0; r < PR_COUNT; r++) if (a == ROLE_NAMES[r]) role = r;
        const int c = b == "-" ? PADC_NONE : padControlFromName(b);
        if (role < 0 || (c < 0 && b != "-")) continue;
        kept[cur].second[role] = c;
    }
    return true;
}

bool PadMap::save(const std::string& path) const {
    bool any = false;
    for (const auto& d : kept) any = any || !d.second.second.empty();
    if (!any && !std::ifstream(path)) return true;   // nothing kept and no file: none made (one there is emptied)
    std::ofstream f(path);
    if (!f) return false;
    f << "# vesperis_pad.txt: the controller's buttons by device (written by SETTINGS > CONTROLS); \"ROLE CONTROL\" lines under each \"pad\"\n";
    for (const auto& d : kept) {
        if (d.second.second.empty()) continue;
        f << "pad " << d.first << " " << d.second.first << "\n";
        for (const auto& rc : d.second.second) f << ROLE_NAMES[rc.first] << " " << padControlName(rc.second) << "\n";
    }
    return true;
}

int padCapture(const PadState& prev, const PadState& rest, const PadState& now, bool axes) {
    if (!now.present) return -1;
    if (!axes) {
        if (now.mapped) for (int b = 0; b < PAD_BUTTONS; b++) if (b != PB_GUIDE && now.buttons[b] && !prev.buttons[b]) return b;
        if (now.mapped) for (int a = 4; a < PAD_AXES; a++) if (now.axes[a] - rest.axes[a] > 0.6) return PADC_STD_AXIS + 2 * a + 1;   // a trigger pulled
    }
    if (now.mapped) {   // a stick's half (the standard names first: they hold for any controller of the same layout)
        int best = -1; double bestD = 0.6;
        for (int a = 0; a < 4; a++) { const double d = now.axes[a] - rest.axes[a]; if (std::fabs(d) > bestD && std::fabs(now.axes[a]) > 0.5) { bestD = std::fabs(d); best = PADC_STD_AXIS + 2 * a + (d > 0 ? 1 : 0); } }
        if (best >= 0) return best;
    }
    if (!axes) {
        for (int n = 0; n < now.rawButtonCount && n < PAD_RAW_BUTTONS; n++) if (now.rawButtons[n] && !prev.rawButtons[n]) return PADC_RAW_BUTTON + n;
        for (int h = 0; h < now.rawHatCount && h < PAD_RAW_HATS; h++)
            for (int d = 0; d < 4; d++) if ((now.rawHats[h] & (1 << d)) && !(prev.rawHats[h] & (1 << d))) return PADC_RAW_HAT + 4 * h + d;
    }
    int best = -1; double bestD = 0.6;
    for (int a = 0; a < now.rawAxisCount && a < PAD_RAW_AXES; a++) {
        const double r0 = rest.rawAxes[a], d = now.rawAxes[a] - r0;
        if (std::fabs(d) <= bestD) continue;
        bestD = std::fabs(d);
        if (!axes && r0 < -0.8 && d > 0) best = PADC_RAW_TRIGGER + a;          // a trigger resting at one end
        else if (!axes && r0 > 0.8 && d < 0) best = PADC_RAW_TRIGGER_INV + a;
        else best = PADC_RAW_AXIS + 2 * a + (d > 0 ? 1 : 0);
    }
    return best;
}
