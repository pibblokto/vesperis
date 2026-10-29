// The controller (game/pad.cpp): its bindings, the command menu's items and the screen state of the menu, the keyboard and
// the controller page
#pragma once
#include "core/input.h"
#include <string>
#include <utility>
#include <vector>

struct PadBind { int role = -1, key = -1, mods = 0; std::string label; };

struct PadItem { std::string label; int key = -1, mods = 0, special = 0; std::string text; };

// a line's special: the keyboard, the controller page, a line typed, a key pressed twice (a removal), auto-explore's profiles
enum PadSpecial { PSP_NONE, PSP_KEYBOARD, PSP_CONTROLLER, PSP_TYPE, PSP_TWICE, PSP_AUTO };

// U-04: a screen's controller, as its row's `padMenu` writes it in the screen's file: the command menu's first page (its tab's
// name and its lines), what the keyboard's buttons do at a typing screen, and a screen's own buttons and what its sticks do there
// (the maps'; a screen without them has the screens' buttons)
struct PadMenu {
    std::string title = "COMMANDS";
    std::vector<PadItem> items;
    const char* typing = "A TYPES  X DELETES  Y SPACE  START DONE  B CANCELS";
    std::vector<PadBind> buttons; const char* sticks = nullptr;
    void item(const std::string& label, int key, int mods = 0, int special = PSP_NONE) {
        PadItem it; it.label = label; it.key = key; it.mods = mods; it.special = special; items.push_back(it);
    }
    void typed(const std::string& label, const std::string& text) { PadItem it; it.label = label; it.special = PSP_TYPE; it.text = text; items.push_back(it); }
    void button(int role, int key, const char* label, int mods = 0) { PadBind b; b.role = role; b.key = key; b.mods = mods; b.label = label; buttons.push_back(b); }
};

struct PadUI {
    bool present = false;                 // a controller was there last frame
    std::string device; bool announce = false;   // the device in use (a new one is announced once in the cabin or on the ground)
    bool active = false;                  // the controller is the device used last (a key or the mouse takes it back)
    bool prev[PR_BUTTONS] = {};          // the buttons last frame
    bool held[4] = {}; double repeat[4] = {};   // the arrows held (up, down, left, right) and for how long: they repeat in the screens
    bool pulse[4] = {};                   // this frame's arrow presses (the first frame held, then the repeats)
    bool resync = false;                  // the next frame takes what is held as already held (after a binding changed)
    bool menu = false; int page = 0, sel = 0;   // the command menu: its page, its line
    bool keyboard = false; int kRow = 0, kCol = 0, latch = 0, kbFor = 0;   // the keyboard: its key under the cursor, the latched Shift/Ctrl/Alt; kbFor 1 the aim's digits
    bool kbShown = false;                 // the keyboard was up last frame
    std::vector<std::pair<int, int>> queue;   // keys pressed one a frame (a command typed for the console): key, modifiers
    // the controller page: the row, the role being set (and the stick's step), its clock, the axes at rest, last frame
    int screenSel = 0, capture = -1, capStep = 0; double capT = 0; bool capArmed = false; PadState capRest, capPrev;
    std::string note; double noteUntil = 0;   // the controller page's last word
};

// a command's modifiers, and the key it presses in words ("CTRL+SHIFT+L")
constexpr int MOD_SHIFT = 1, MOD_CTRL = 2, MOD_ALT = 4;
inline std::string keyLabel(int key, int mods) { return std::string(mods & MOD_CTRL ? "CTRL+" : "") + (mods & MOD_ALT ? "ALT+" : "") + (mods & MOD_SHIFT ? "SHIFT+" : "") + keyName(key); }
