// E-03: input: `fuzz` (random keys and mouse through every state), `input` (the pad and the keys, scripted)
#include "harness/harness.h"

// ---- random-walk fuzz (M0-08): random keys and mouse across every state -------------------
int runFuzz(int frames) {
    TestGame game;
    game.savePrefix = "shots/tests/fuzz_save";
    game.moviesDir = "shots/tests/test_movies"; game.recFixedTake = true;   // random Ctrl+R must not litter movies/
    game.settingsPath = "shots/tests/fuzz_settings.txt";
    game.guidePath = "shots/tests/fuzz_guide.txt"; game.keysPath = "shots/tests/fuzz_keys.txt";
    applyTestScale(game);
    Input in;
    Rng r(20260926);
    static const int keys[] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE, KEY_TAB, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_A, KEY_S, KEY_D,
                               KEY_R, KEY_V, KEY_L, KEY_X, KEY_O, KEY_C, KEY_T, KEY_Q, KEY_E, KEY_K, KEY_I, KEY_H, KEY_N, KEY_P,
                               KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_F1, KEY_F2, KEY_F5, KEY_F9, KEY_PAGE_UP, KEY_PAGE_DOWN};
    const int nk = sizeof(keys) / sizeof(keys[0]);
    int bad = 0, states[16] = {0};
    double t0 = nowSec();
    for (int f = 0; f < frames; f++) {
        in.newFrame();
        for (int i = 0; i < nk; i++) {
            int k = keys[i];
            if (r.chance(0.02)) { in.pressed[k] = true; in.down[k] = true; }
            else if (in.down[k] && r.chance(0.15)) in.down[k] = false;
        }
        in.mouseDx = r.sym(40); in.mouseDy = r.sym(25); in.wheel = r.irange(3) - 1;
        in.mousePressed[0] = r.chance(0.02); in.mouseDown[0] = in.mousePressed[0];
        game.frame(in, 1.0 / 60);
        game.wantsQuit = false; game.wantsScreenshot = false; game.wantsFullscreenToggle = false; game.wantsWindowScale = 0;
        states[(int)game.state & 15]++;
        std::string h = game.testHealth();
        if (!h.empty()) { printf("frame %d: non-finite %s\n", f, h.c_str()); if (++bad > 5) break; }
    }
    printf("fuzz: %d frames in %.1f s, %d health failures; frames per state:", frames, nowSec() - t0, bad);
    for (int s = 0; s < 12; s++) printf(" %d:%d", s, states[s]);
    printf("\n");
    return bad ? 1 : 0;
}

// M6-02: the key map file and the gamepad mapping, checked without a window
// the controller: the bindings (a known device's standard layout, a raw device's guess, the capture, the file a device), then the
// game driven by a controller alone: the title, the cabin (walking, using, the command menu and its pages, the keys page, the aim's digits),
// the guide's text entry by the keyboard, the controller page (a button set from the next press, the reset), a landing, on foot (a jump,
// a sprint, the menu's buggy) and the buggy (in, the throttle). PNGs of each overlay in shots/tests/pad_*.png
static int testPad() {
    int fails = 0;
    auto report = [&](const char* name, bool ok, const std::string& detail) { printf("%s: %s %s\n", name, ok ? "PASS" : "FAIL", detail.c_str()); fflush(stdout); if (!ok) fails++; };
    auto stdPad = [](const char* guid) { PadState p; p.present = true; p.mapped = true; snprintf(p.guid, sizeof p.guid, "%s", guid); snprintf(p.name, sizeof p.name, "TEST PAD"); return p; };
    auto rawPad = [](const char* guid) { PadState p; p.present = true; p.mapped = false; snprintf(p.guid, sizeof p.guid, "%s", guid); snprintf(p.name, sizeof p.name, "ODD PAD");
                                         p.rawButtonCount = 12; p.rawAxisCount = 6; p.rawHatCount = 1; p.rawAxes[2] = -1; p.rawAxes[5] = -1; return p; };
    {   // a known device: its standard layout, the round dead zone, a trigger read from its rest
        PadState p = stdPad("0300test"); PadMap m; m.use(p);
        p.buttons[PB_A] = true; p.axes[0] = 0.9f; p.axes[1] = -1.0f; p.axes[5] = 0.8f; p.axes[2] = 0.1f;
        const PadRead r = m.read(p, 0.18);
        const bool ok = r.down[PR_A] && !r.down[PR_B] && r.down[PR_RT] && !r.down[PR_LT] && r.walkX > 0.5 && r.walkY < -0.7 && r.lookX == 0;
        report("pad: the standard layout", ok, fmt("A %d RT %d LT %d walk %+.2f %+.2f look %+.2f", (int)r.down[PR_A], (int)r.down[PR_RT], (int)r.down[PR_LT], r.walkX, r.walkY, r.lookX));
    }
    {   // a raw device: buttons 0-3 the face, the hat the d-pad, axes 3 and 4 the right stick and 2 and 5 the triggers with six axes
        PadState p = rawPad("raw-one"); PadMap m; m.use(p);
        p.rawButtons[0] = true; p.rawHats[0] = 1; p.rawAxes[3] = 1.0f; p.rawAxes[5] = 1.0f;
        const PadRead r = m.read(p, 0.18);
        const bool ok = r.down[PR_A] && r.down[PR_UP] && !r.down[PR_DOWN] && r.lookX > 0.9 && r.down[PR_RT] && !r.down[PR_LT] && padControlName(m.bind[PR_LT]) == "T2";
        report("pad: a raw device's guess", ok, fmt("A %d UP %d look %+.2f RT %d LT %s", (int)r.down[PR_A], (int)r.down[PR_UP], r.lookX, (int)r.down[PR_RT], padControlName(m.bind[PR_LT]).c_str()));
    }
    {   // the guess reads the axes at rest: one resting off-centre is no stick (it would walk by itself); triggers resting in the middle are no
        // triggers, and four axes or more without them put the right stick on the third and fourth (DirectInput's order)
        PadState p = rawPad("raw-odd"); p.rawAxes[1] = 1.0f;
        PadMap m; m.use(p);
        PadState d = rawPad("raw-dinput"); d.rawAxes[2] = 0; d.rawAxes[5] = 0;
        PadMap m2; m2.use(d);
        const bool ok = m.bind[PR_WALK_X] == PADC_NONE && m.bind[PR_WALK_Y] == PADC_NONE && m.read(p, 0.18).walkY == 0 &&
                        padControlName(m2.bind[PR_LOOK_X]) == "A2+" && padControlName(m2.bind[PR_LOOK_Y]) == "A3+" && m2.bind[PR_LT] == PADC_NONE && !m2.read(d, 0.18).down[PR_LT];
        report("pad: the guess reads the axes at rest", ok,
                "walk " + padControlName(m.bind[PR_WALK_X]) + " " + padControlName(m.bind[PR_WALK_Y]) + ", dinput look " + padControlName(m2.bind[PR_LOOK_X]) + " " +
                        padControlName(m2.bind[PR_LOOK_Y]) + " LT " + padControlName(m2.bind[PR_LT]));
    }
    {   // the capture: a button, a stick's half, a hat, a trigger resting at one end; a stick's capture takes no button; the standard names first
        PadState rest = rawPad("raw-cap"), now = rest;
        // a button held since last frame: none
        now.rawButtons[5] = true; const int c1 = padCapture(rest, rest, now, false), c5 = padCapture(rest, rest, now, true), c8 = padCapture(now, rest, now, false);
        now = rest; now.rawAxes[1] = -0.9f; const int c2 = padCapture(rest, rest, now, true);
        now = rest; now.rawHats[0] = 4; const int c3 = padCapture(rest, rest, now, false);
        now = rest; now.rawAxes[2] = 0.9f; const int c4 = padCapture(rest, rest, now, false);
        PadState srest = stdPad("std-cap"), snow = srest; srest.rawButtonCount = 10;
        snow.buttons[PB_X] = true; snow.rawButtonCount = 10; snow.rawButtons[2] = true; const int c6 = padCapture(srest, srest, snow, false);
        snow = srest; snow.axes[4] = 1.0f; const int c7 = padCapture(srest, srest, snow, false);
        const bool ok = padControlName(c1) == "B5" && padControlName(c2) == "A1-" && padControlName(c3) == "H0D" && padControlName(c4) == "T2" && c5 == -1 && c8 == -1 && padControlName(c6) == "X" &&
                padControlName(c7) == "LT+";
        report("pad: the capture", ok,
                padControlName(c1) + " " + padControlName(c2) + " " + padControlName(c3) + " " + padControlName(c4) + " " + std::to_string(c5) + " " + std::to_string(c8) + " " + padControlName(c6) +
                        " " + padControlName(c7));
    }
    {   // every control's name reads back as itself
        int bad = 0, named = 0;
        for (int c = 0; c < PADC_COUNT; c++) { const std::string n = padControlName(c); if (n == "-") continue; named++; if (padControlFromName(n) != c) bad++; }
        report("pad: the controls' names", bad == 0 && named > 120, fmt("%d named, %d wrong", named, bad));
    }
    {   // the file: a set a device; another device keeps its defaults; the reset
        PadState a = rawPad("raw-file-a"), b = rawPad("raw-file-b");
        PadMap m; m.use(a); m.set(a, PR_A, PADC_RAW_BUTTON + 3); m.set(a, PR_WALK_Y, PADC_RAW_AXIS + 2 * 4 + 1); m.set(a, PR_B, PADC_RAW_BUTTON + 1);   // B to its default: not kept
        const bool saved = m.save("shots/tests/test_pad.txt");
        PadMap m2; const bool loaded = m2.load("shots/tests/test_pad.txt");
        m2.use(a); const bool aOk = m2.bind[PR_A] == PADC_RAW_BUTTON + 3 && m2.bind[PR_WALK_Y] == PADC_RAW_AXIS + 9 && m2.bind[PR_B] == PADC_RAW_BUTTON + 1 && m2.custom(a);
        m2.use(b); const bool bOk = m2.bind[PR_A] == PADC_RAW_BUTTON + 0 && !m2.custom(b);
        m2.use(a); m2.reset(a); const bool reset = m2.bind[PR_A] == PADC_RAW_BUTTON + 0 && !m2.custom(a);
        std::ifstream f("shots/tests/test_pad.txt"); std::string all((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        report("pad: a set for each controller", saved && loaded && aOk && bOk && reset && all.find("\nB ") == std::string::npos,
                fmt("saved %d loaded %d a %d b %d reset %d", (int)saved, (int)loaded, (int)aOk, (int)bOk, (int)reset));
    }
    // the game by a controller alone
    for (const char* f :
            {"shots/tests/pad_save_1.txt", "shots/tests/pad_save_auto.txt", "shots/tests/pad_settings.txt", "shots/tests/pad_keys.txt", "shots/tests/pad_guide.txt", "shots/tests/pad_pad.txt"})
        remove(f);
    TestGame game;
    game.savePrefix = "shots/tests/pad_save";
    game.settingsPath = "shots/tests/pad_settings.txt";
    game.keysPath = "shots/tests/pad_keys.txt";
    game.guidePath = "shots/tests/pad_guide.txt";
    game.padPath = "shots/tests/pad_pad.txt";
    game.settings.renderScale = 2; game.applySettings();
    KeyMap km; game.keymap = &km;
    Input gi; gi.pad = stdPad("0300test");
    auto frames = [&](int n) { for (int i = 0; i < n; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); } };
    auto tap = [&](int b) { gi.pad.buttons[b] = true; frames(1); gi.pad.buttons[b] = false; frames(1); };
    auto taps = [&](int b, int n) { for (int i = 0; i < n; i++) tap(b); };
    auto shot = [&](const char* name) { writePNG(fmt("shots/tests/pad_%s.png", name).c_str(), game.output(), FBW, FBH); };
    auto itemIndex = [&](int page, const std::string& label) {
        std::istringstream is(game.testPadItems(page));
        std::string line;
        int i = 0;
        while (std::getline(is, line)) {
            if (line.substr(0, line.find('|')) == label) return i;
            i++;
        }
        return -1;
    };
    auto runItem = [&](int page, const std::string& label) {   // Y, the page, down to the line, A
        const int k = itemIndex(page, label);
        if (k < 0) return false;
        tap(PB_Y);
        for (int i = 0; i < 3 && game.testPadPage() != page; i++) tap(PB_RB);
        taps(PB_DOWN, k);
        tap(PB_A);
        return true;
    };
    frames(2);
    tap(PB_A);
    report("pad: A begins at the title", game.state == GameState::SPACE, fmt("state %d", (int)game.state));
    frames(3);
    report("pad: the controller announced", game.testStatus().find("CONTROLLER") != std::string::npos || game.testStatusNext().find("CONTROLLER") != std::string::npos,
            game.testStatus() + " / " + game.testStatusNext());
    {   // the left stick walks in the cabin (it walked nothing there before)
        double x0, z0, y0, x1, z1, y1; game.testCabinGoto(1.6, -1.0, 0.0); frames(1); game.testCabinAt(x0, z0, y0);   // clear of the capsule's cage
        gi.pad.axes[1] = -1.0f; frames(20); gi.pad.axes[1] = 0; frames(1); game.testCabinAt(x1, z1, y1);
        gi.pad.axes[2] = 0.8f; frames(10); gi.pad.axes[2] = 0; frames(1); double x2, z2, y2; game.testCabinAt(x2, z2, y2);
        report("pad: the cabin walks and looks", z1 - z0 > 0.5 && std::fabs(y2 - y1) > 0.2, fmt("z %.2f -> %.2f, yaw %.2f -> %.2f", z0, z1, y1, y2));
    }
    game.testCabinGoto(0.0, 1.3, 0.0); frames(2);
    tap(PB_X);
    report("pad: X uses what the explorer faces", game.state == GameState::SHIPSCREEN, fmt("state %d", (int)game.state));
    tap(PB_B);
    report("pad: B leaves a screen", game.state == GameState::SPACE, fmt("state %d", (int)game.state));
    {   // the command menu: Y, the pages, a line run as its key
        tap(PB_Y);
        const bool open = game.testPadMenu() && game.testPadPage() == 0;
        shot("menu_cabin");
        tap(PB_RB); const int p1 = game.testPadPage(); shot("menu_general");
        tap(PB_RB); const int p2 = game.testPadPage(); shot("menu_keys");
        tap(PB_RB); const int p0 = game.testPadPage();
        tap(PB_B); const bool closed = !game.testPadMenu();
        const bool ran = runItem(0, "FIELD AMPLIFICATOR ON");
        frames(1);
        report("pad: the command menu", open && p1 == 1 && p2 == 2 && p0 == 0 && closed && ran && game.testStatus() == "FIELD AMPLIFICATOR ON" && !game.testPadMenu(),
               fmt("open %d pages %d %d %d closed %d ran %d status '%s'", (int)open, p1, p2, p0, (int)closed, (int)ran, game.testStatus().c_str()));
    }
    {   // the keys page: F from the keyboard (the cursor from A, five to the right) toggles the amplificator back
        tap(PB_Y); taps(PB_RB, 2); taps(PB_RIGHT, 5); tap(PB_A); tap(PB_B);
        report("pad: the keys page presses a key", game.testStatus() == "FIELD AMPLIFICATOR OFF", "status '" + game.testStatus() + "'");
    }
    {   // the aim: L3, then the menu's keyboard types the light years
        tap(PB_L3);
        const bool aiming = game.testAiming();
        const bool ran = runItem(0, "JUMP: TYPE THE LIGHT YEARS");
        const bool kb = game.testPadKeyboard();
        tap(PB_A); taps(PB_RIGHT, 9); tap(PB_A); tap(PB_A);   // 1, 0, 0
        shot("keyboard_aim");
        const std::string digits = game.testJumpDigits();
        tap(PB_B);   // the keyboard closes, the aim stays
        const bool stillAiming = game.testAiming() && !game.testPadKeyboard();
        tap(PB_B);   // B leaves the aim
        report("pad: the aim's digits by the keyboard", aiming && ran && kb && digits == "100" && stillAiming && !game.testAiming(),
                fmt("aim %d ran %d kb %d digits '%s' still %d", (int)aiming, (int)ran, (int)kb, digits.c_str(), (int)stillAiming));
    }
    {   // the guide's entry by the keyboard: NAME THIS STAR, A then B, START
        const bool ran = runItem(1, "THE GUIDE");
        const bool guide = game.state == GameState::GUIDE;
        tap(PB_DOWN); tap(PB_A);
        const bool entry = game.state == GameState::TEXT_ENTRY && game.testPadKeyboard();
        tap(PB_A); tap(PB_RIGHT); tap(PB_A);
        shot("keyboard_text");
        tap(PB_START);
        report("pad: a name typed with the keyboard", ran && guide && entry && game.testStarName() == "AB" && game.state == GameState::SPACE,
                fmt("ran %d guide %d entry %d name '%s' state %d", (int)ran, (int)guide, (int)entry, game.testStarName().c_str(), (int)game.state));
    }
    {   // the console by the keyboard: SELECT opens the commands over it, WHERE typed and run
        game.testCabinGoto(-1.3, 0.0, -PI / 2); frames(2);
        tap(PB_X);
        const bool console = game.state == GameState::CONSOLE && game.testPadKeyboard();
        frames(1); shot("keyboard_console");
        tap(PB_BACK);   // SELECT
        const bool menu = game.testPadMenu();
        const int k = itemIndex(0, "WHERE: THE SHIP'S SECTOR");
        taps(PB_DOWN, k); tap(PB_A); frames(8);
        const bool back = game.state == GameState::CONSOLE && !game.testPadMenu();
        tap(PB_B);
        report("pad: the console's commands over the keyboard", console && menu && k >= 0 && back && game.state == GameState::SPACE,
                fmt("console %d menu %d item %d back %d state %d", (int)console, (int)menu, k, (int)back, (int)game.state));
    }
    {   // the controller page: A set from the next press (Y), Y then acting as A, the reset; Esc saves
        const bool ran = runItem(1, "CONTROLLER SETUP");
        const bool page = game.state == GameState::KEYS && game.testKeysPage() == 1;
        frames(1); shot("controller");
        tap(PB_A);                                     // Enter on A's row: the capture, armed once A is let go
        const bool capturing = game.testPadCapture() == PR_A;
        gi.pad.buttons[PB_Y] = true; frames(2); gi.pad.buttons[PB_Y] = false; frames(2);
        const bool bound = game.padMap.bind[PR_A] == PB_Y && game.testPadCapture() < 0;
        taps(PB_DOWN, 8);                              // the walk stick's row (down the first column past RT)
        tap(PB_Y);                                     // Y is A now (the two changed places): Enter
        gi.pad.axes[3] = -1.0f; frames(2); gi.pad.axes[3] = 0; frames(2);     // up: the right stick's (Y negative up)
        gi.pad.axes[2] = 1.0f; frames(2); gi.pad.axes[2] = 0; frames(2);      // right
        const bool walkSet = padControlName(game.padMap.bind[PR_WALK_Y]) == "RY+" && padControlName(game.padMap.bind[PR_WALK_X]) == "RX+" && game.testPadCapture() < 0;
        taps(PB_UP, 9);                                // back to the first row, then up: the reset's
        const int resetRow = game.testPadScreenSel();
        tap(PB_Y);                                     // Y is A now: Enter resets
        const bool reset = game.padMap.bind[PR_A] == PB_A && padControlName(game.padMap.bind[PR_WALK_Y]) == "LY+";
        tap(PB_B);
        report("pad: the controller page", ran && page && capturing && bound && walkSet && resetRow == 21 && reset && game.state == GameState::SPACE,
                fmt("ran %d page %d capturing %d bound %d walk stick %d reset row %d reset %d state %d", (int)ran, (int)page, (int)capturing, (int)bound, (int)walkSet, resetRow, (int)reset,
                        (int)game.state));
    }
    {   // the help's controller page (Space twice from the first)
        const bool ran = runItem(1, "HELP");
        tap(PB_X); tap(PB_X);   // X is Space in the screens
        shot("help");
        tap(PB_B);
        report("pad: the help's controller page", ran && game.state == GameState::SPACE, fmt("ran %d state %d", (int)ran, (int)game.state));
    }
    {   // the star map and the galaxy chart by a controller: the right stick turns the view, the triggers zoom, the menu's class filter; SELECT
        // to the chart, the left stick moves its cursor, X back to the ship, a place marked through the keyboard and removed from the menu
        const bool ran = runItem(1, "THE STAR MAP");
        double yaw0, zoom0, yaw1, zoom1; int mask0, mask1, lvl0, lvl1, gm0, gm1;
        game.testStarMapAt(yaw0, zoom0, mask0, lvl0, gm0);
        gi.pad.axes[2] = 0.7f; frames(15); gi.pad.axes[2] = 0; frames(1);
        gi.pad.axes[5] = 1.0f; frames(2); gi.pad.axes[5] = -1.0f; frames(2);
        game.testStarMapAt(yaw1, zoom1, mask1, lvl1, gm1);
        const bool turned = std::fabs(yaw1 - yaw0) > 0.05 && std::fabs(yaw1 - yaw0) < 1.5 && zoom1 > zoom0 * 1.2;
        const bool cls = runItem(0, "THE NEXT CLASS");
        int maskC; game.testStarMapAt(yaw1, zoom1, maskC, lvl1, gm1);
        runItem(0, "EVERY CLASS");
        int maskA; game.testStarMapAt(yaw1, zoom1, maskA, lvl1, gm1);
        tap(PB_Y); shot("menu_starmap"); tap(PB_B);
        tap(PB_BACK);   // SELECT: the galaxy
        const bool chart = game.state == GameState::STAR_MAP && game.testMapPage() == 1;
        gi.pad.axes[0] = 1.0f; frames(20); gi.pad.axes[0] = 0; frames(1);
        const bool moved = !game.testChartOnShip();
        gi.pad.axes[5] = 1.0f; frames(2); gi.pad.axes[5] = -1.0f; frames(2);
        int lvl2; game.testStarMapAt(yaw1, zoom1, maskA, lvl2, gm1);
        const bool marked = runItem(0, "MARK THE PLACE") && game.state == GameState::TEXT_ENTRY;
        tap(PB_START);
        int gm2; game.testStarMapAt(yaw1, zoom1, maskA, lvl2, gm2);
        tap(PB_Y); shot("menu_galaxy"); tap(PB_B);
        runItem(0, "REMOVE THE MARK"); frames(2);
        int gm3; game.testStarMapAt(yaw1, zoom1, maskA, lvl2, gm3);
        tap(PB_X);   // Space: the cursor back to the ship
        const bool home = game.testChartOnShip();
        tap(PB_B);
        report("pad: the star map and the galaxy chart",
                ran && turned && cls && maskC != mask0 && maskA == mask0 && chart && moved && lvl2 == lvl0 + 1 && marked && gm2 == gm0 + 1 && gm3 == gm0 && home && game.state == GameState::SPACE,
                fmt("ran %d turned %.2f zoom %.2f->%.2f classes %d/%d/%d chart %d moved %d level %d->%d marks %d->%d->%d home %d state %d", (int)ran, yaw1 - yaw0, zoom0, zoom1, mask0, maskC, maskA,
                        (int)chart, (int)moved, lvl0, lvl2, gm0, gm2, gm3, (int)home, (int)game.state));
    }
    {   // a landing, on foot, the buggy
        const Star home = game.testSystem().star;
        const int body = game.testLandableBody();
        game.testParkAt(home, body);
        frames(2);
        {   // the right stick moves the landing map's cursor smoothly (the d-pad steps it a degree)
            double la0, lo0, la1, lo1, la2, lo2; int z0, z1;
            game.testLandCursorAt(la0, lo0, z0);
            gi.pad.axes[2] = 1.0f; frames(30); gi.pad.axes[2] = 0; frames(1);
            game.testLandCursorAt(la1, lo1, z1);
            tap(PB_DOWN); game.testLandCursorAt(la2, lo2, z1);
            double dl = lo1 - lo0; if (dl < -180) dl += 360; if (dl > 180) dl -= 360;
            report("pad: the landing map's cursor by the stick and the d-pad", dl > 20 && dl < 90 && std::fabs(la1 - la0) < 0.5 && std::fabs(la1 - la2 - 1.0) < 0.01,
                   fmt("a second of the stick: %+.1f deg east, %+.2f north; the d-pad down: %.2f deg", dl, la1 - la0, la1 - la2));
            tap(PB_Y); shot("menu_landmap"); tap(PB_B);
        }
        gi.pad.axes[5] = 1.0f; frames(2); gi.pad.axes[5] = -1.0f; frames(2);   // RT: the sector's zoom (Z)
        const bool zoomed = game.testPadLegend().find("LT THE WORLD") != std::string::npos;
        gi.pad.axes[4] = 1.0f; frames(2); gi.pad.axes[4] = -1.0f; frames(2);   // LT: the whole world again
        const bool whole = game.testPadLegend().find("RT ZOOM IN") != std::string::npos;
        report("pad: the landing map's triggers zoom", zoomed && whole, fmt("zoomed %d whole %d", (int)zoomed, (int)whole));
        tap(PB_A);   // land here
        for (int i = 0; i < 300 && game.state != GameState::SURFACE; i++) frames(1);
        const bool landed = game.state == GameState::SURFACE;
        frames(30);
        tap(PB_Y); shot("menu_foot"); tap(PB_B);
        gi.pad.buttons[PB_A] = true; frames(1); gi.pad.buttons[PB_A] = false;
        double alt = 0; for (int i = 0; i < 15; i++) { frames(1); alt = std::max(alt, game.testPlayerAlt()); }
        frames(40);
        const bool buggy = runItem(0, "UNFOLD A BUGGY");
        frames(90);
        const double st0 = game.testPlayerStamina();
        gi.pad.axes[1] = -1.0f; gi.pad.axes[5] = 1.0f; frames(45); gi.pad.axes[1] = 0; gi.pad.axes[5] = -1.0f; frames(1);
        const double st1 = game.testPlayerStamina();
        const int scan0 = game.testScanMode();
        gi.pad.axes[5] = 1.0f; frames(2); tap(PB_RB); gi.pad.axes[5] = -1.0f; frames(1);   // RB while RT sprints: the scanner's next kind, not Shift+R
        const bool scanned = game.testScanMode() != scan0;
        report("pad: on foot (a jump, the sprint, the scanner, the menu's buggy)",
                landed && alt > 0.3 && st1 < st0 - 2 && scanned && buggy && game.testBuggyInfo().find("deployed=1") != std::string::npos,
                fmt("landed %d jump %.2f m, stamina %.0f -> %.0f, scanner %d -> %d, buggy %d (%s)", (int)landed, alt, st0, st1, scan0, game.testScanMode(), (int)buggy, game.testBuggyInfo().c_str()));
        {   // the sector map: LB opens it, the right stick moves the cursor (half a throw: a cursor's pace), the triggers zoom, A marks the place
            // (the keyboard's START places it unnamed), the menu removes it, X brings the cursor home, B closes
            tap(PB_LB);
            const bool open = game.state == GameState::SECTOR_MAP;
            double x0, z0, x1, z1; int zm0, zm1, dry;
            game.testSectorMapAt(x0, z0, zm0);
            gi.pad.axes[2] = 0.5f; frames(20); gi.pad.axes[2] = 0; frames(1);
            game.testSectorMapAt(x1, z1, zm1);
            const double hw = zm0 == 1 ? 4000 : 1;   // the default level is 8 km wide
            const bool cursor = x1 - x0 > 0.02 * hw && x1 - x0 < 0.5 * hw;
            gi.pad.axes[5] = 1.0f; frames(2); gi.pad.axes[5] = -1.0f; frames(2);
            int zm2; game.testSectorMapAt(x1, z1, zm2);
            gi.pad.axes[4] = 1.0f; frames(2); gi.pad.axes[4] = -1.0f; frames(2);
            int zm3; game.testSectorMapAt(x1, z1, zm3);
            const int marks0 = game.testMarksHere(dry);
            tap(PB_A); const bool entry = game.state == GameState::TEXT_ENTRY; tap(PB_START); frames(2);
            const int marks1 = game.testMarksHere(dry);
            tap(PB_Y); shot("menu_sectormap"); tap(PB_B);
            runItem(0, "REMOVE THE MARK"); frames(2);
            const int marks2 = game.testMarksHere(dry);
            tap(PB_X);
            double x3, z3; int zm4; game.testSectorMapAt(x3, z3, zm4);
            const bool homeAgain = std::fabs(x3 - game.testPlayerX()) < 0.5 && std::fabs(z3 - game.testPlayerZ()) < 0.5;
            tap(PB_B);
            report("pad: the sector map", open && cursor && zm2 == zm0 - 1 && zm3 == zm0 && entry && marks1 == marks0 + 1 && marks2 == marks0 && homeAgain && game.state == GameState::SURFACE,
                    fmt("open %d cursor %+.0f m (level %d) zoom %d->%d->%d mark %d: %d->%d->%d home %d state %d", (int)open, x1 - x0, zm0, zm0, zm2, zm3, (int)entry, marks0, marks1, marks2,
                            (int)homeAgain, (int)game.state));
        }
        game.testWalkToBuggy(); frames(2);
        tap(PB_X);
        const bool in = game.testBuggyInfo().find("in=1") != std::string::npos;
        gi.pad.axes[5] = 1.0f; frames(60); const double sp = game.testBuggySpeed();
        tap(PB_Y); shot("menu_buggy"); tap(PB_B);
        gi.pad.axes[5] = -1.0f; frames(2);
        report("pad: the buggy (X gets in, RT drives)", in && sp > 2, fmt("in %d speed %.1f m/s", (int)in, sp));
    }
    printf("pad: the legend in the buggy: '%s'\n", game.testPadLegend().c_str());
    return fails;
}

int testInput() {
    int fails = 0;
    {
        FILE* f = fopen("shots/tests/test_keys.txt", "w");
        fprintf(f, "# AZERTY: Z acts as W and W as Z, Q as A and A as Q\nZ W\nW Z\nQ A\nA Q\nF7 P\n");
        fclose(f);
        KeyMap km;
        bool ok = km.load("shots/tests/test_keys.txt");
        Input in;
        in.down[KEY_Z] = true; in.pressed[KEY_Z] = true; in.down[KEY_F7] = true; in.pressed[KEY_F7] = true;
        km.apply(in);
        bool good = ok && in.isDown(KEY_W) && in.wasPressed(KEY_W) && !in.isDown(KEY_Z) && in.wasPressed(KEY_P) && !in.isDown(KEY_F7);
        printf("keymap: %s (Z->W, F7->P)\n", good ? "PASS" : "FAIL");
        if (!good) fails++;
        // N5-05: the key bindings screen: open it from the settings, rebind FORWARD to Z, reset it, save
        {
            TestGame game;
            game.savePrefix = "shots/tests/test_save";
            game.settingsPath = "shots/tests/test_settings.txt";
            game.keysPath = "shots/tests/test_keys2.txt";
            game.guidePath = "shots/tests/test_guide.txt";
            game.padPath = "shots/tests/test_pad2.txt";   // the CONTROLS screen saves the controller's file too
            KeyMap km2; game.keymap = &km2;
            Input gi;
            auto press = [&](int key) { pressKey(game, gi, key); };
            press(KEY_ENTER); for (int i = 0; i < 3; i++) game.frame(gi, 1.0 / 30);
            press(KEY_ESCAPE); selectRow(game, gi, "SETTINGS"); press(KEY_ENTER);   // menu > settings > CONTROLS, by their labels (U-01)
            selectRow(game, gi, "CONTROLS");
            press(KEY_ENTER);
            bool opened = game.state == GameState::KEYS;
            press(KEY_ENTER);                                 // rebind FORWARD
            game.rawKey = KEY_Z; game.frame(gi, 1.0 / 30); game.rawKey = -1;
            bool bound = km2.map[KEY_Z] == KEY_W;
            writePNG("shots/tests/keys.png", game.output(), FBW, FBH);
            press(KEY_BACKSPACE);
            bool reset = km2.map[KEY_Z] == KEY_Z;
            press(KEY_ENTER); game.rawKey = KEY_Z; game.frame(gi, 1.0 / 30); game.rawKey = -1;
            press(KEY_ESCAPE);
            KeyMap km3; bool saved = km3.load("shots/tests/test_keys2.txt") && km3.map[KEY_Z] == KEY_W;
            bool ok2 = opened && bound && reset && saved && game.state == GameState::SETTINGS;
            printf("key bindings screen: %s (opened %d, Z->W %d, reset %d, saved and reloaded %d) -> shots/keys.png\n", ok2 ? "PASS" : "FAIL", (int)opened, (int)bound, (int)reset, (int)saved);
            if (!ok2) fails++;
        }
        if (!good) fails++;
        printf("key names: %s %s %s %s\n", keyName(keyFromName("lshift")).c_str(), keyName(keyFromName("F12")).c_str(), keyName(keyFromName("space")).c_str(), keyName(KEY_PAGE_UP).c_str());
    }
    fails += testPad();
    return fails;
}
