// U-01: the tour (`tour`): every state opened as the game opens it, each from a fresh game at the same start (the home
// system, parked at its first world, every map waited for); its picture, the controller's command menu (each page as its
// lines and a picture), then its keys one a frame (its help's, its menus' rows by label, Esc last), each followed by the
// state, what is selected and the frame's hash, with a picture at each change of state. A key that leaves the state is
// followed by Esc a bounded number of times (a screen that will not let go is printed, not waited on), then by a fresh game
// opened again; the keys and rows that write a save or leave the start run last. Each state writes its saves, guide,
// settings, keys and controller files under shots/tests/tour/ with a prefix of its own (the directory emptied first). The
// openers and the keys are lists of the tour's own until U-06 reads the states' rows (docs/ideas/PLAN-screens.md, U-01)
#include "harness/harness.h"
#include "harness/test_worlds.h"
#include <memory>

namespace {

const char* const TOUR_DIR = "shots/tests/tour";

struct Tour;
// a state's stop: how the game opens it (printed), the opener from a fresh game, and its keys (Esc follows them all)
struct Stop {
    GameState state;
    const char* how;
    bool (*open)(Tour&);
    void (*keys)(Tour&);
};

// where the explorer stands in the cabin to face a device, and the controller's name for it ("USE <label>")
struct Device { const char* label; double x, z, yaw, pitch; };
const Device DEVICES[] = {
    {"THE GOES CONSOLE", -1.2, 0.0, -PI / 2, 0.0},   {"THE FLIGHT COMPUTER", 0.0, 1.3, 0.0, 0.0},    {"THE STAR MAP SCREEN", 1.6, -1.4, PI / 2, 0.0},
    {"THE LANDING SCREEN", 1.6, 0.0, PI / 2, 0.0},   {"THE TARGET SCREEN", 1.6, 1.4, PI / 2, 0.0},   {"THE CAPSULE", 0.0, -1.4, 0.0, -0.3},
    {"THE SHARD DECODER", 0.0, -0.8, PI, -0.65},     {"THE SIGNAL RADAR", -1.55, -1.2, PI, -0.3},    {"THE TELESCOPE", 1.55, -1.2, PI, -0.3},
    {"THE INSTRUMENT CASE", -1.1, -1.38, -PI / 2, -0.55}, {"THE DRONE BAY", -0.3, 1.435, -PI / 2, -0.38},
};

struct Tour {
    std::unique_ptr<TestGame> g;
    KeyMap keymap;               // the state's own key map (the keys screen binds into it)
    Input in;
    const Stop* stop = nullptr;  // the state toured
    std::string pre;             // its files' prefix: "07_menu"
    int steps = 0, pics = 0, lost = 0;   // the state's steps; pictures and steps not taken in all
    int64_t stormX = -1, stormZ = -1; int stormBody = -1;   // the storm's giant, once found

    GameState state() const { return g->state; }
    // a frame of 1/30 s; then what the window takes and clears, and what threads build waited for before the next frame
    void frame() {
        g->frame(in, 1.0 / 30);
        in.newFrame();
        g->wantsScreenshot = g->wantsFullscreenToggle = g->wantsPanorama = false; g->wantsWindowScale = 0;
        g->testSettle();
    }
    void run(double secs) { for (int i = 0, n = std::max(1, (int)(secs * 30)); i < n; i++) frame(); }
    // a key pressed for a frame of 1/30 s, its modifiers held with it
    void press(int key, int mods = 0) {
        if (mods & MOD_SHIFT) in.down[KEY_LEFT_SHIFT] = true;
        if (mods & MOD_CTRL) in.down[KEY_LEFT_CONTROL] = true;
        if (mods & MOD_ALT) in.down[KEY_LEFT_ALT] = true;
        in.pressed[key] = true; in.down[key] = true;
        frame();
        in.down[key] = in.down[KEY_LEFT_SHIFT] = in.down[KEY_LEFT_CONTROL] = in.down[KEY_LEFT_ALT] = false;
    }
    uint64_t hash() const { return fnv(1469598103934665603ULL, g->output(), (size_t)FBW * FBH * sizeof(uint32_t)); }
    std::string where() const { return fmt("%s %s #%016llx", stateName(g->state), g->testSelection().c_str(), (unsigned long long)hash()); }
    void picture(const std::string& what) {
        writePNG(fmt("%s/%s_%s.png", TOUR_DIR, pre.c_str(), what.c_str()).c_str(), g->output(), FBW, FBH);
        pics++;
    }
    void say(const std::string& what) { printf("  %s: %s\n", what.c_str(), where().c_str()); g->wantsQuit = false; }
    // the controller's buttons, a frame down and a frame up
    void tap(int b) { in.pad.buttons[b] = true; frame(); in.pad.buttons[b] = false; frame(); }

    void fresh();                                   // a new game at its title, the home system's maps made
    bool start();                                   // Enter at the title: the ship in its parking
    bool open();                                    // a fresh game, the state opened
    void padMenu();                                 // the command menu's pages, as lines and pictures
    void step(const std::string& what, int key, int mods = 0);   // a key, and what it did; a picture when the state changed
    void back();                                    // to the state toured again: Esc a bounded number of times, else opened again
    void keys(const char* spec);                    // keys by name, each followed by `back`; a group "A,B,C" is pressed without it
    bool row(const std::string& label);             // the open menu's row of that label entered (Enter), then `back`
    void rows(const MenuLabels& m, const char* const* last);   // every row of a menu by label, those named in `last` (null-ended) last
    void type(const std::string& text);             // letters typed, a line for the whole, then Enter
    bool device(const Device& d, bool printed = true);   // E at a cabin device, found by the controller's line for it
    bool deviceNamed(const char* label);              // an opener's: E pressed, not printed
    bool stormGiant(Star& st, int& gi);             // the first gas giant with a great storm
    bool sendProbe();                               // parked at it, C: the probe's camera
    bool probeRecord();                             // a probe sent into a giant and its fall skipped to the end: a record in the guide
};

// a test controller, known to the game's mappings (its standard layout)
PadState tourPad() {
    PadState p; p.present = true; p.mapped = true;
    snprintf(p.guid, sizeof p.guid, "%s", "0300tour"); snprintf(p.name, sizeof p.name, "TOUR PAD");
    return p;
}

std::string lowerName(GameState s) { std::string n = stateName(s); for (char& c : n) c = (char)tolower((unsigned char)c); return n; }

// a key as the key map names it ("ESC", "PGUP", "T"), CTRL+, SHIFT+ or ALT+ before it
bool parseKey(const std::string& tok, int& key, int& mods) {
    std::string t = tok;
    mods = 0;
    for (bool more = true; more;) {
        more = false;
        if (t.rfind("CTRL+", 0) == 0) { mods |= MOD_CTRL; t = t.substr(5); more = true; }
        else if (t.rfind("SHIFT+", 0) == 0) { mods |= MOD_SHIFT; t = t.substr(6); more = true; }
        else if (t.rfind("ALT+", 0) == 0) { mods |= MOD_ALT; t = t.substr(4); more = true; }
    }
    key = keyFromName(t);
    return key >= 0;
}

// the scenes, which Esc leaves for the Esc menu: the tour opens the state again from a fresh game instead
bool scene(GameState s) { return s == GameState::SPACE || s == GameState::SURFACE || s == GameState::TITLE; }

void Tour::fresh() {
    g.reset();   // one Game at a time (the game's globals): the old one goes first
    g = std::make_unique<TestGame>();
    const std::string p = std::string(TOUR_DIR) + "/" + pre;
    g->savePrefix = p + "_save"; g->settingsPath = p + "_settings.txt"; g->guidePath = p + "_guide.txt"; g->keysPath = p + "_keys.txt";
    g->padPath = p + "_pad.txt"; g->shotsDir = p + "_shots"; g->moviesDir = p + "_movies"; g->recFixedTake = true;
    keymap = KeyMap(); g->keymap = &keymap;
    applyTestScale(*g);
    in = Input();
    for (int i = 0; i < (int)g->testSystem().bodies.size(); i++) g->testWaitMapOf(i);   // made on their thread at the binary's own pace
    run(0.2);
}

bool Tour::start() {
    press(KEY_ENTER);
    run(0.5);
    return state() == GameState::SPACE;
}

bool Tour::open() {
    fresh();
    return stop->open(*this) && state() == stop->state;
}

// the command menu as the controller opens it (Y; over a text entry's keyboard, SELECT), each page's lines and picture, then
// B closes it and the controller is taken away (the keys take over again)
void Tour::padMenu() {
    in.pad = tourPad();
    frame();   // the controller arrives (what it holds as it arrives presses nothing)
    tap(PB_Y);
    if (!g->testPadMenu()) tap(PB_BACK);
    if (!g->testPadMenu()) { say("pad: NO COMMAND MENU"); lost++; }
    else {
        const int first = g->testPadPage();
        for (int k = 0; k < 3; k++) {
            const int page = g->testPadPage();
            std::string items = g->testPadItems(page);
            for (char& c : items) if (c == '\n') c = ';';
            printf("  pad page %d: %s\n", page, page == 2 ? "the keyboard" : items.c_str());
            picture(fmt("pad%d", page));
            tap(PB_RB);
            if (g->testPadPage() == first) break;
        }
        tap(PB_B);
    }
    in.pad = PadState();
    frame();
    say("pad away");
}

void Tour::step(const std::string& what, int key, int mods) {
    const GameState before = state();
    press(key, mods);
    steps++;
    say(what);
    if (state() != before) picture(fmt("%02d_%s", steps, lowerName(state()).c_str()));
}

void Tour::back() {
    const GameState home = stop->state;
    for (int i = 0; i < 4 && state() != home && !(scene(state()) && home != GameState::MENU); i++) {
        press(KEY_ESCAPE);
        say("  esc");
    }
    // the ship's own modes, which Esc leaves before the Esc menu: the aim, the radar's camera, the telescope
    for (int i = 0; i < 3 && state() == GameState::SPACE && home == GameState::SPACE && (g->testAiming() || g->testRadarOn() || g->testTelescopeOn()); i++) {
        press(KEY_ESCAPE);
        say("  esc");
    }
    if (state() == home) return;
    const bool ok = open();
    say("  opened again");
    if (!ok) lost++;
}

void Tour::keys(const char* spec) {
    std::istringstream is(spec);
    std::string tok;
    while (is >> tok) {
        std::istringstream group(tok);
        std::string k;
        while (std::getline(group, k, ',')) {
            int key, mods;
            if (!parseKey(k, key, mods)) { printf("  key %s: UNKNOWN\n", k.c_str()); lost++; continue; }
            step("key " + k, key, mods);
        }
        back();
    }
}

bool Tour::row(const std::string& label) {
    for (int turn = 0; turn < 3; turn++) {   // the flight computer's pages: a label on another page is reached by Right
        const std::vector<std::string> rs = g->testMenuRows();
        if (std::find(rs.begin(), rs.end(), label) != rs.end()) {
            if (!selectRow(*g, in, label)) break;
            step("row \"" + label + "\"", KEY_ENTER);
            back();
            return true;
        }
        if (state() != GameState::SHIPSCREEN) break;
        press(KEY_RIGHT);
    }
    printf("  row \"%s\": NOT FOUND in %s\n", label.c_str(), where().c_str());
    lost++;
    return false;
}

void Tour::rows(const MenuLabels& m, const char* const* last) {
    auto isLast = [&](const std::string& l) { for (const char* const* p = last; p && *p; p++) if (l == *p) return true; return false; };
    for (int i = 0; i < m.count; i++) if (!isLast(m.label[i])) row(m.label[i]);
    for (const char* const* p = last; p && *p; p++) row(*p);
}

void Tour::type(const std::string& text) {
    for (char c : text) {
        const int k = c == ' ' ? KEY_SPACE : (c == '-' ? KEY_MINUS : (c == '.' ? KEY_PERIOD : (c >= '0' && c <= '9' ? KEY_0 + (c - '0') : KEY_A + (toupper((unsigned char)c) - 'A'))));
        press(k);
    }
    step("type \"" + text + "\", Enter", KEY_ENTER);
    back();
}

bool Tour::device(const Device& d, bool printed) {
    g->testCabinGoto(d.x, d.z, d.yaw, d.pitch);
    frame();
    if (g->testPadItems(0).find(std::string("USE ") + d.label + "|") == std::string::npos) {
        printf("  E at %s: NOT FACED (%s)\n", d.label, where().c_str());
        lost++;
        return false;
    }
    if (printed) step(std::string("E at ") + d.label, KEY_E);
    else press(KEY_E);
    return true;
}

bool Tour::deviceNamed(const char* label) {
    for (const Device& d : DEVICES) if (strcmp(d.label, label) == 0) return device(d, false);
    return false;
}

// the probe's giant: the first gas giant with a great storm, searched as the unit's probe group searches (x 150-320, z 20-140)
bool Tour::stormGiant(Star& st, int& gi) {
    if (stormX < 0)
        for (int64_t x = 150; x < 320 && stormX < 0; x++)
            for (int64_t z = 20; z < 140 && stormX < 0; z++) {
                Star s; StarSystem ss;
                if (!systemAt(x, 0, z, s, ss, true)) continue;
                for (int i = 0; i < (int)ss.bodies.size() && stormX < 0; i++) {
                    if (!isProbeGiant(ss.bodies[i].type)) continue;
                    const GiantCharacter c = giantAtmosphereOf(ss, i).ch;
                    if (c.greatStorm && c.kind == GK_GAS) { stormX = x; stormZ = z; stormBody = i; }
                }
            }
    gi = stormBody;
    return stormX >= 0 && starInSector(stormX, 0, stormZ, st, true);
}

// parked at the storm's giant, C sends a probe aimed into the great storm (its N then names the storm: the entry, its way back)
bool Tour::sendProbe() {
    Star st; int gi = -1;
    if (!stormGiant(st, gi)) return false;
    g->testParkAt(st, gi);
    run(0.3);
    const Body& b = g->testSystem().bodies[gi];
    const BodyGen bg = BodyGen::make(b);
    g->harness.aimOverride = true; g->harness.aimLat = bg.gsLat; g->harness.aimLon = wrapAngle(bg.gsLon - giantJetTurn(b, bg.bandCount, bg.gsLat, g->testGameTime()));
    press(KEY_C);
    g->harness.aimOverride = false;
    return state() == GameState::PROBE;
}

// a probe sent, its fall skipped to its end, Esc: its record in the guide
bool Tour::probeRecord() {
    if (!sendProbe()) return false;
    run(0.1);
    g->testProbeSkip(1e6);
    run(0.2);
    press(KEY_ESCAPE);
    run(0.1);
    pngFlush();
    return state() == GameState::SPACE && !g->guide.probes.empty();
}

// ---- the openers: from a fresh game at its title, the state opened as the game opens it
bool openTitle(Tour& t) { (void)t; return true; }
bool openSpace(Tour& t) { return t.start(); }
bool openLanding(Tour& t) {
    if (!t.start()) return false;
    t.press(KEY_C);
    t.g->testWaitMap();
    t.frame();
    return true;
}
bool openDescent(Tour& t) {
    if (!openLanding(t) || t.state() != GameState::LANDING_MAP) return false;
    t.press(KEY_ENTER);
    t.run(1.0);
    return true;
}
bool openSurface(Tour& t) {
    if (!openDescent(t)) return false;
    for (int i = 0; i < 300 && t.state() == GameState::DESCENT; i++) t.frame();
    t.run(0.5);
    return true;
}
bool openAscent(Tour& t) {
    if (!openSurface(t) || t.state() != GameState::SURFACE) return false;
    t.press(KEY_K);
    t.run(0.3);
    t.press(KEY_Q);
    t.run(1.0);
    return true;
}
bool openHelp(Tour& t) { if (!t.start()) return false; t.press(KEY_H); return true; }
bool openMenu(Tour& t) { if (!t.start()) return false; t.press(KEY_ESCAPE); return true; }
bool openSystemList(Tour& t) { if (!t.start()) return false; t.press(KEY_TAB); return true; }
bool openData(Tour& t) { if (!t.start()) return false; t.press(KEY_I); return true; }
bool openMenuRow(Tour& t, const char* label) {
    if (!openMenu(t) || !selectRow(*t.g, t.in, label)) return false;
    t.press(KEY_ENTER);
    return true;
}
bool openSettings(Tour& t) { return openMenuRow(t, "SETTINGS"); }
bool openSlots(Tour& t) { return openMenuRow(t, "SAVE EXPEDITION"); }
bool openGuide(Tour& t) { if (!t.start()) return false; t.press(KEY_G); return true; }
bool openGuideRow(Tour& t, const char* label) {
    if (!openGuide(t) || !selectRow(*t.g, t.in, label)) return false;
    t.press(KEY_ENTER);
    return true;
}
bool openTextEntry(Tour& t) { return openGuideRow(t, "WRITE A NOTE"); }
bool openStarMap(Tour& t) { if (!t.start()) return false; t.press(KEY_M); return true; }
bool openLog(Tour& t) { if (!t.start()) return false; t.press(KEY_J); return true; }
bool openStats(Tour& t) { return openGuideRow(t, "STATISTICS"); }
bool openGallery(Tour& t) {
    if (!t.start() || !t.probeRecord()) return false;
    t.press(KEY_G);
    if (!selectRow(*t.g, t.in, "GALLERY")) return false;
    t.press(KEY_ENTER);
    return true;
}
bool openShipScreen(Tour& t) { return t.start() && t.deviceNamed("THE FLIGHT COMPUTER"); }
bool openConsole(Tour& t) { return t.start() && t.deviceNamed("THE GOES CONSOLE"); }
bool openSectorMap(Tour& t) { if (!openSurface(t) || t.state() != GameState::SURFACE) return false; t.press(KEY_N); return true; }
bool openKeys(Tour& t) {
    if (!openSettings(t) || !selectRow(*t.g, t.in, "CONTROLS")) return false;
    t.press(KEY_ENTER);
    return true;
}
bool openShards(Tour& t) {
    for (int i = 0; i < 3; i++) t.g->guide.shards.insert("151,0,25/1/S" + std::to_string(i));   // the flow's three records of Aieliaalas II
    return t.start() && t.deviceNamed("THE SHARD DECODER");
}
bool openAlmanac(Tour& t) { if (!t.start()) return false; t.press(KEY_T, MOD_CTRL); return true; }
bool openProbe(Tour& t) { return t.start() && t.sendProbe(); }
bool openRecording(Tour& t) {
    if (!openGallery(t) || t.state() != GameState::GALLERY) return false;
    t.press(KEY_ENTER);
    return true;
}
// the case's instruments: the first of the industrial age's world's people and of the modern's (as the drones' mode puts them)
bool openInstruments(Tour& t) {
    for (const AgeWorld& W : {AGE_WORLDS[AGE_INDUSTRIAL], AGE_WORLDS[AGE_MODERN]}) {
        Star is; StarSystem isys;
        if (!starInSector(W.sx, 0, W.sz, is, true)) continue;
        isys.generate(is);
        if (W.body >= (int)isys.bodies.size()) continue;
        const BodyGen ig = BodyGen::make(isys.bodies[W.body]);
        const Lore L = loreQuick(isys, isys.bodies[W.body], ig, W.which);
        PeopleInstruments P; instrumentsOf(traditionOf(ig, L), L.which, P);
        for (int a = 0; a < AGE_COUNT; a++)
            if (!P.byAge[a].empty()) { t.g->guide.instruments.insert(Guide::bodyKey(W.sx, 0, W.sz, W.body) + "/I" + std::to_string(P.byAge[a][0].id)); break; }
    }
    return t.start() && t.deviceNamed("THE INSTRUMENT CASE");
}
bool openDrones(Tour& t) {
    if (!t.start() || !t.deviceNamed("THE DRONE BAY")) return false;
    t.g->testDronesSettle();
    return true;
}
bool openAuto(Tour& t) { return openMenuRow(t, "AUTO-EXPLORE"); }

// ---- the keys: its help's and its footer's, its menus' rows by label; what writes a save or leaves the start, last
void keysTitle(Tour& t) { t.keys("H C N ENTER"); }
void keysSpace(Tour& t) {
    t.keys("H G M J I TAB CTRL+T LEFT RIGHT UP DOWN R,N,N,ENTER R,1,0,BACKSPACE,R B,PGUP,PGDN,B Z,EQUAL,MINUS,PGUP,PGDN,ENTER,N Z F F X O O U U Y Y "
           "PGUP PGDN T,T,T,T,T L P CTRL+K CTRL+K CTRL+F C");
    for (const Device& d : DEVICES) { if (t.device(d)) t.back(); }
    t.keys("V ENTER SHIFT+LEFT CTRL+S CTRL+L");
}
void keysLanding(Tour& t) { t.keys("LEFT RIGHT UP DOWN SHIFT+LEFT C C J R Z Z V V CTRL+T ENTER"); }
void keysDescent(Tour& t) { t.keys("T ESC H CTRL+S"); }
void keysSurface(Tour& t) {
    t.keys("W A S D SPACE C Z 1 0 TAB TAB V V V V V U U X M M R SHIFT+R G J I H N L B E E F K CTRL+S CTRL+L Q");
}
void keysAscent(Tour& t) { t.keys("T ESC H CTRL+S"); }
void keysHelp(Tour& t) { t.keys("SPACE,SPACE,SPACE A"); }
void keysMenu(Tour& t) {
    t.keys("UP DOWN");
    static const char* const LAST[] = {"NEW EXPEDITION", "QUIT TO DESKTOP", nullptr};
    t.rows(MENU_LABELS, LAST);
}
void keysSystemList(Tour& t) { t.keys("DOWN DOWN UP L TAB ENTER"); }
void keysData(Tour& t) { t.keys("RIGHT T A"); }
// each setting changed (Right) by its label, the clock put back at once (the wall clock kept out); then Esc saves them, the
// screen opened again shows them kept; CONTROLS and BACK last
void keysSettings(Tour& t) {
    for (int i = 0; i < SETTINGS_LABELS.count; i++) {
        const std::string l = SETTINGS_LABELS.label[i];
        if (l == "CONTROLS") continue;
        if (!selectRow(*t.g, t.in, l)) { printf("  row \"%s\": NOT FOUND\n", l.c_str()); t.lost++; continue; }
        t.step("\"" + l + "\" right", KEY_RIGHT);
        if (l == "CLOCK") t.step("\"" + l + "\" left", KEY_LEFT);
    }
    t.step("key ESC (saved)", KEY_ESCAPE);
    if (selectRow(*t.g, t.in, "SETTINGS")) t.step("row \"SETTINGS\" (kept)", KEY_ENTER);
    t.back();
    t.row("CONTROLS");
    t.row("BACK");
}
// the third slot saved; then, from the Esc menu, the load page and the third slot loaded
void keysSlots(Tour& t) {
    t.keys("DOWN DOWN DOWN UP ENTER");
    t.step("key ESC", KEY_ESCAPE);
    if (t.state() == GameState::MENU && selectRow(*t.g, t.in, "LOAD EXPEDITION")) {
        t.step("row \"LOAD EXPEDITION\"", KEY_ENTER);
        t.keys("DOWN,DOWN,ENTER");
    }
}
void keysGuide(Tour& t) {
    t.keys("UP DOWN G");
    static const char* const LAST[] = {"SET HOME HERE", "EXPORT THE GUIDE", "IMPORT AN INBOX FILE", nullptr};
    t.rows(GUIDE_LABELS, LAST);
}
void keysTextEntry(Tour& t) { t.keys("A,B,C,1,2,SPACE,MINUS,PERIOD,BACKSPACE,TAB ENTER"); }
void keysStarMap(Tour& t) {
    t.keys("LEFT RIGHT UP DOWN EQUAL MINUS C X 0 TAB,EQUAL,MINUS,N,SHIFT+N,C,PGUP,PGDN,LEFT,RIGHT,UP,DOWN,L TAB,TAB M ENTER");
}
void keysLog(Tour& t) { t.keys("DOWN UP PGDN PGUP RIGHT LEFT J ENTER"); }
void keysStats(Tour& t) { t.keys("K A"); }
void keysGallery(Tour& t) { t.keys("LEFT RIGHT UP DOWN ENTER"); }
void keysShipScreen(Tour& t) {
    t.keys("LEFT RIGHT TAB UP DOWN E");
    static const char* const LAST[] = {"FINE APPROACH TO THE LOCAL TARGET", "DEPLOY THE CAPSULE OR A PROBE", "VIMANA FLIGHT TO THE REMOTE TARGET", "SAVE THE EXPEDITION", nullptr};
    for (int p = 0; p < 3; p++) {
        auto isLast = [&](const std::string& l) { for (const char* const* q = LAST; *q; q++) if (l == *q) return true; return false; };
        for (int i = 0; i < SHIP_PAGE_LABELS[p].count; i++) if (!isLast(SHIP_PAGE_LABELS[p].label[i])) t.row(SHIP_PAGE_LABELS[p].label[i]);
    }
    for (const char* const* q = LAST; *q; q++) t.row(*q);
}
void keysConsole(Tour& t) {
    for (const char* c : {"HELP", "SL", "SL 5", "DL", "WHERE", "PAR", "CAT", "HOME", "PREV", "XYZZY"}) t.type(c);
    t.keys("BACKSPACE");
    t.type("CAST THE TOUR WAS HERE");
    t.type("JUMP 2");
}
void keysSectorMap(Tour& t) { t.keys("EQUAL MINUS LEFT RIGHT UP DOWN C SPACE TAB M M R SHIFT+R SHIFT+ENTER DELETE,DELETE N ENTER"); }
// the keyboard's page, a binding captured (the key's raw press on the frame after Enter) and reset, the controller's page
void keysKeys(Tour& t) {
    t.keys("DOWN UP RIGHT LEFT TAB,DOWN,UP,ENTER,TAB ENTER");
    t.g->rawKey = KEY_Z; t.frame(); t.g->rawKey = -1;
    t.steps++; t.say("raw key Z");
    t.keys("BACKSPACE ENTER");
    t.g->rawKey = KEY_Z; t.frame(); t.g->rawKey = -1;
    t.steps++; t.say("raw key Z");
}
void keysShards(Tour& t) { t.keys("TAB,TAB DOWN UP ENTER,TAB,SHIFT+TAB,RIGHT,LEFT,DOWN,UP,ENTER,SPACE,RIGHT,LEFT,ESC,ESC"); }
void keysAlmanac(Tour& t) { t.keys("DOWN UP PGDN PGUP UP,LEFT,RIGHT ENTER"); }
// the fall's first seconds (the chute and the storm's name refused), the descent (the chute opened and cut), inside the storm
// (named), then its end
void keysProbe(Tour& t) {
    t.keys("X LEFT RIGHT UP DOWN N SPACE");
    t.g->testProbeSkip(60); t.frame();
    t.steps++; t.say("the clock at 60 s (the descent)");
    t.keys("SPACE SPACE");
    t.g->testProbeSkip(120); t.frame();
    t.steps++; t.say("the clock at 120 s");
    t.keys("N N,R,E,D,ENTER");
    t.g->testProbeSkip(1e6); t.frame();
    t.steps++; t.say("the clock at the end");
    t.keys("ENTER");
}
void keysRecording(Tour& t) { t.keys("SPACE SPACE HOME END RIGHT LEFT DOWN UP PGDN PGUP X N ENTER"); }
void keysInstruments(Tour& t) { t.keys("DOWN UP ENTER,DOWN,UP,LEFT,RIGHT,1,2,9,0,ENTER,SPACE,ESC"); }
void keysDrones(Tour& t) { t.keys("DOWN UP ENTER"); }
void keysAuto(Tour& t) { t.keys("DOWN UP UP,ENTER"); }   // Up from the first row is BACK; a profile's Enter is the unit's `auto` group's

// a stop a state, in the table's order
constexpr Stop STOPS[] = {
    {GameState::TITLE, "the game's start", openTitle, keysTitle},
    {GameState::SPACE, "Enter at the title", openSpace, keysSpace},
    {GameState::LANDING_MAP, "C in the parking", openLanding, keysLanding},
    {GameState::DESCENT, "Enter on the landing map", openDescent, keysDescent},
    {GameState::SURFACE, "the descent's end", openSurface, keysSurface},
    {GameState::ASCENT, "K, then Q on the ground", openAscent, keysAscent},
    {GameState::HELP, "H in space", openHelp, keysHelp},
    {GameState::MENU, "Esc in space", openMenu, keysMenu},
    {GameState::SYSTEM_LIST, "Tab in space", openSystemList, keysSystemList},
    {GameState::DATA, "I in space", openData, keysData},
    {GameState::SETTINGS, "the Esc menu's SETTINGS", openSettings, keysSettings},
    {GameState::SLOTS, "the Esc menu's SAVE EXPEDITION", openSlots, keysSlots},
    {GameState::GUIDE, "G in space", openGuide, keysGuide},
    {GameState::TEXT_ENTRY, "the guide's WRITE A NOTE", openTextEntry, keysTextEntry},
    {GameState::STAR_MAP, "M in space", openStarMap, keysStarMap},
    {GameState::LOG, "J in space", openLog, keysLog},
    {GameState::STATS, "the guide's STATISTICS", openStats, keysStats},
    {GameState::GALLERY, "a probe's record (the storm's giant), then the guide's GALLERY", openGallery, keysGallery},
    {GameState::SHIPSCREEN, "E at the flight computer", openShipScreen, keysShipScreen},
    {GameState::CONSOLE, "E at the GOES console", openConsole, keysConsole},
    {GameState::SECTOR_MAP, "N on the ground", openSectorMap, keysSectorMap},
    {GameState::KEYS, "the settings' CONTROLS", openKeys, keysKeys},
    {GameState::SHARDS, "three records in the guide, then E at the shard decoder", openShards, keysShards},
    {GameState::ALMANAC, "Ctrl+T in space", openAlmanac, keysAlmanac},
    {GameState::PROBE, "C parked at a giant with a great storm", openProbe, keysProbe},
    {GameState::RECORDING, "Enter on a probe's record in the gallery", openRecording, keysRecording},
    {GameState::INSTRUMENTS, "two instruments in the guide, then E at the case", openInstruments, keysInstruments},
    {GameState::DRONES, "E at the drone bay", openDrones, keysDrones},
    {GameState::AUTO, "the Esc menu's AUTO-EXPLORE", openAuto, keysAuto},
};
static_assert(sizeof(STOPS) / sizeof(STOPS[0]) == STATE_COUNT, "a stop a state");
constexpr bool stopsInOrder() {
    for (int i = 0; i < STATE_COUNT; i++)
        if ((int)STOPS[i].state != i) return false;
    return true;
}
static_assert(stopsInOrder(), "each stop in its state's place");

// the tour's directory emptied: its files and its states' folders (a gallery's pictures, a recorder's frames)
void emptyTourDir() {
    mkdir("shots", 0755); mkdir("shots/tests", 0755); mkdir(TOUR_DIR, 0755);
    DIR* d = opendir(TOUR_DIR);
    if (!d) return;
    std::vector<std::string> names;
    while (struct dirent* e = readdir(d)) if (e->d_name[0] != '.') names.push_back(e->d_name);
    closedir(d);
    for (const std::string& n : names) {
        const std::string p = std::string(TOUR_DIR) + "/" + n;
        if (DIR* sub = opendir(p.c_str())) {
            std::vector<std::string> inner;
            while (struct dirent* e = readdir(sub)) if (e->d_name[0] != '.') inner.push_back(p + "/" + e->d_name);
            closedir(sub);
            for (const std::string& f : inner) remove(f.c_str());
        }
        remove(p.c_str());
    }
}

}   // namespace

// `tour [state ...]`: every state (or those named), its steps printed; exit 1 when a state, a row or a device was not reached
int runTour(int argc, char** argv) {
    std::vector<std::string> only;
    for (int i = 2; i < argc; i++) { std::string s = argv[i]; for (char& c : s) c = (char)toupper((unsigned char)c); only.push_back(s); }
    emptyTourDir();
    Tour t;
    const double t0 = nowSec();
    int states = 0;
    printf("tour: %d states, each from a fresh game at its title (the home system, parked), %dx\n", STATE_COUNT, g_testScale);
    for (const Stop& s : STOPS) {
        if (!only.empty() && std::find(only.begin(), only.end(), std::string(stateName(s.state))) == only.end()) continue;
        t.stop = &s;
        t.steps = 0;
        t.pre = fmt("%02d_%s", (int)s.state, lowerName(s.state).c_str());
        printf("== %s: %s\n", stateName(s.state), s.how);
        const bool ok = t.open();
        t.say("opened");
        t.picture("open");
        if (!ok) { printf("  NOT OPENED\n"); t.lost++; continue; }
        states++;
        t.padMenu();
        s.keys(t);
        if (t.state() != s.state) t.back();
        for (int i = 0; i < 4; i++) {   // Esc last, until a scene (bounded: a screen that will not let go is printed)
            t.step("key ESC", KEY_ESCAPE);
            if (scene(t.state())) break;
        }
        pngFlush();
    }
    t.g.reset();
    printf("tour: %d states toured, %d pictures, %d steps not taken (%.1f s)\n", states, t.pics, t.lost, nowSec() - t0);
    return t.lost ? 1 : 0;
}
