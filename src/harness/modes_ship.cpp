// I-02: the ship's harness (R-426): `ship rooms` (each space from its arch and from its middle: the rooms the eye sees, the space view
// drawn or not, the banks dealt, and the same picture with every room seen unclipped and with the space view drawn) and `ship walk` (auto-
// explore's walk to every device and through every arch: the rooms passed, the floor's height along the stairs, every wall kept)
#include "harness/harness.h"
#include "harness/test_worlds.h"
#include <cstring>

namespace {
// a space from its middle: the plan's views
const char* const MIDDLE[SHIP_ROOMS] = {"THE BRIDGE'S MIDDLE", "THE HALL'S MIDDLE", "THE LIBRARY'S MIDDLE", "THE LABORATORY'S MIDDLE", "THE QUARTERS' MIDDLE",
                                        "THE HANGAR'S MIDDLE", "THE MAINTENANCE ROOM'S MIDDLE", "THE OBSERVATORY'S MIDDLE", nullptr};

// a space from its arch: its first walked way's point inside it, facing in (away from the way)
bool archStand(int room, ShipStand& at) {
    for (int i = 0; i < shipOpenings().count; i++) {
        const ShipOpening& o = shipOpenings()[i];
        if ((o.kind != OPEN_ARCH && o.kind != OPEN_STAIR) || (o.from != room && o.to != room)) continue;
        double ax, az, bx, bz;
        shipWayPoints(i, room, ax, az, bx, bz);
        at = {ax, az, std::atan2(ax - bx, az - bz), -0.05};
        return true;
    }
    return false;
}

// a room's name for a file: "THE MAINTENANCE ROOM" -> maintenance_room
std::string slugOf(const char* name) {
    std::string s = name;
    if (s.rfind("THE ", 0) == 0) s = s.substr(4);
    for (char& c : s) c = c == ' ' ? '_' : (char)tolower((unsigned char)c);
    return s;
}

// what a frame saw of the ship: the rooms (their depth, their rows), the space view, the banks dealt
std::string sightLine(const CabinFrame& f) {
    std::string s;
    for (int r = 0; r < SHIP_ROOMS; r++) {
        if (!f.sight.seen[r]) continue;
        const bool whole = f.sight.x0[r] <= 0 && f.sight.y0[r] <= 0 && f.sight.x1[r] >= FBW && f.sight.y1[r] >= FBH;
        s += fmt("%s%s %d%s", s.empty() ? "" : ", ", slugOf(shipRooms()[r].name).c_str(), f.sight.depth[r],
                 whole ? "" : fmt(" (%.0f-%.0f x %.0f-%.0f)", f.sight.x0[r], f.sight.x1[r], f.sight.y0[r], f.sight.y1[r]).c_str());
    }
    return s;
}
std::string banksLine(const ShipBanks& b) {
    std::string s;
    for (int h = 0; h < 32; h++) if (b.of[h] >= 0) s += fmt("%s%d>%d", s.empty() ? "" : " ", h, b.of[h]);
    return fmt("%s (%d asked, %d dealt, %d dimmed, a pool of %d)", s.empty() ? "none" : s.c_str(), b.asked, b.dealt, b.dimmed, b.pool);
}

void shipGame(TestGame& game, const char* tag) {
    game.savePrefix = fmt("shots/tests/ship_%s_save", tag);
    game.settingsPath = fmt("shots/tests/ship_%s_settings.txt", tag);
    game.keysPath = "shots/tests/test_keys.txt";
    game.guidePath = fmt("shots/tests/ship_%s_guide.txt", tag);
    remove(game.guidePath.c_str()); remove(game.settingsPath.c_str());
    game.newGame(); game.setState(GameState::SPACE);
}

// `ship rooms [scale ...]`: each space from its arch and from its middle at each scale (1 and 4 by default), the light on and the hull
// polarised, parked at the start's world; each drawn at one instant once it holds still, written to shots/tests/ship/<room>_<arch|middle>
// _<scale>x.png. Printed: the rooms seen (a room's depth through the ways; its box on the screen when it is not the whole frame), whether
// the space view was drawn, the banks dealt, the frame's time; then the same frame with every room seen unclipped and with the space view
// drawn whatever is in view: the pixels that differ (none: the clip and the skip leave out only what is hidden). The exit code: the frames
// that differ
int roomsMode(int argc, char** argv) {
    std::vector<int> scales;
    for (int i = 3; i < argc; i++) scales.push_back(clampi(atoi(argv[i]), 1, 4));
    if (scales.empty()) scales = {1, 4};
    makeDir("shots/tests"); makeDir("shots/tests/ship");
    TestGame game;
    shipGame(game, "rooms");
    game.settings.renderScale = 1; game.applySettings();
    Input gi;
    auto press = [&](int key) { pressKey(game, gi, key); };
    runFramesAtLeastOne(game, gi, 0.3);
    const int lights = game.testCabinLights();
    if (!(lights & 1)) press(KEY_U);
    if (lights & 2) press(KEY_Y);
    runFramesAtLeastOne(game, gi, 1.5);
    for (const Body& b : game.testSystem().bodies) if (b.type != PT_COMPANION) game.testWaitMapOf(b.index);
    auto out = [&]() { return fnv(1469598103934665603ULL, game.output(), (size_t)FBW * FBH * sizeof(uint32_t)); };
    int shots = 0, differ = 0, skipped = 0;
    printf("ship rooms: %d spaces from their arches and their middles, %d threads in the pool\n", SHIP_ROOMS - 1, parallelThreads());
    for (int r = 0; r < SHIP_ROOMS; r++) {
        if (!MIDDLE[r]) continue;
        for (int v = 0; v < 2; v++) {
            ShipStand at{};
            if (v == 0 && !archStand(r, at)) continue;
            if (v == 1) at = shipView(MIDDLE[r]).at;
            game.testCabinPlace(r, at);
            const std::string name = slugOf(shipRooms()[r].name) + (v == 0 ? "_arch" : "_middle");
            for (int s : scales) {
                game.settings.renderScale = s; game.applySettings();
                uint64_t last = 0; int still = 0;
                for (int n = 0; n < 200 && still < 10; n++) {
                    game.frame(gi, 0);
                    const uint64_t h = out();
                    still = n > 0 && h == last ? still + 1 : 0;
                    last = h;
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                const size_t np = (size_t)FBW * FBH;
                const double t0 = nowSec(); game.frame(gi, 0); const double ms = (nowSec() - t0) * 1000;
                const std::vector<uint32_t> w(game.output(), game.output() + np);
                const CabinFrame& f = game.testCabinFrame();
                const bool outside = f.sight.seesOut;
                const std::string seen = sightLine(f), banks = banksLine(f.banks);
                game.harness.shipWhole = true; game.frame(gi, 0); game.harness.shipWhole = false;
                const FrameDiff dw = diffFrames(w.data(), game.output(), FBW, FBH);
                game.harness.shipOutside = true; game.frame(gi, 0); game.harness.shipOutside = false;
                const FrameDiff ds = diffFrames(w.data(), game.output(), FBW, FBH);
                writePNG(fmt("shots/tests/ship/%s_%dx.png", name.c_str(), s).c_str(), w.data(), FBW, FBH);
                shots++;
                if (!outside) skipped++;
                if (dw.differ || ds.differ) differ++;
                printf("  %-20s %dx %6.2f ms, the space view %s; seen: %s\n", name.c_str(), s, ms, outside ? "drawn" : "SKIPPED", seen.c_str());
                printf("  %-20s     banks %s\n", "", banks.c_str());
                printf("  %-20s     unclipped: %s; with the space view drawn: %s%s\n", "", dw.differ ? diffLine(dw, FBW, FBH).c_str() : "the same", ds.differ ? diffLine(ds, FBW, FBH).c_str() : "the same",
                       still < 10 ? " (never held still)" : "");
            }
        }
    }
    game.settings.renderScale = 1; game.applySettings();
    printf("ship rooms: %d pictures in shots/tests/ship, %d with the space view skipped; %d differ unclipped or with the space view drawn\n", shots, skipped, differ);
    return differ;
}

// `ship walk`: from the start, auto-explore's hands walk to every device's stand (facing it there), then to the laboratory, the quarters
// and the engine room (every arch walked), then into each room's corners (the stick held toward each from the room's middle for six
// seconds: kept on the plan's floors, through an arch where a wall slides it into one); every frame's place kept on the plan's floors and off its colliders, the floor's height continuous (its largest step in a
// frame printed, the stairs' heights sampled every half second). The exit code: the places not reached, the walls left, the jumps
int walkMode(int argc, char** argv) {
    TestGame game;
    shipGame(game, "walk");
    game.settings.renderScale = 1; game.applySettings();
    Input in;
    const double dt = 1.0 / 30;
    int fails = 0, left = 0, frames = 0;
    double worstStep = 0, stairLo = 1e9, stairHi = -1e9;
    auto frame = [&](Input& i) {
        const double y0 = game.testCabinFloor();
        game.frame(i, dt); i.newFrame(); frames++;
        const double y1 = game.testCabinFloor();
        worstStep = std::max(worstStep, std::fabs(y1 - y0));
        if (!shipWalkable(game.testCabinPlaceNow())) left++;
        if (game.testCabinRoom() == ROOM_STAIR) { stairLo = std::min(stairLo, y1); stairHi = std::max(stairHi, y1); }
    };
    runFramesAtLeastOne(game, in, 0.3);
    struct Goal { std::string name; int room; ShipStand at; int facing; };
    std::vector<Goal> goals;
    for (const CabinDevice& d : Game::cabinDevices()) if (d.facing != FACE_NOTHING) goals.push_back({d.use, d.room, d.stand, d.facing});
    for (const char* v : {"THE LABORATORY'S MIDDLE", "THE QUARTERS' MIDDLE", "THE MAINTENANCE ROOM'S MIDDLE", "THE BRIDGE'S MIDDLE"}) goals.push_back({v, shipView(v).room, shipView(v).at, -1});
    printf("ship walk: %zu places from the start, auto-explore's hands at 30 frames a second\n", goals.size());
    for (const Goal& g : goals) {
        const double t0 = frames * dt;
        std::string rooms = slugOf(shipRooms()[game.testCabinRoom()].name);
        int lastRoom = game.testCabinRoom();
        bool there = false;
        std::string stair;
        double nextSample = 0;
        for (int i = 0; i < 90 * 30 && !there; i++) {
            Input step;
            there = game.testShipWalkStep(g.room, g.at.x, g.at.z, g.at.yaw, step, dt);
            frame(step);
            if (game.testCabinRoom() != lastRoom) { lastRoom = game.testCabinRoom(); rooms += " > " + slugOf(shipRooms()[lastRoom].name); }
            if (game.testCabinRoom() == ROOM_STAIR && frames * dt >= nextSample) { stair += fmt(" %.2f", game.testCabinFloor()); nextSample = frames * dt + 0.5; }
        }
        const bool faced = g.facing < 0 || game.testCabinFacing() == g.facing;
        if (!there || !faced) fails++;
        printf("  %-34s %s in %5.1f s: %s%s\n", g.name.c_str(), there ? (faced ? "reached, faced" : "reached, NOT FACED") : "NOT REACHED", frames * dt - t0, rooms.c_str(),
               faced || g.facing < 0 ? "" : fmt(" (facing %d)", game.testCabinFacing()).c_str());
        if (!stair.empty()) printf("  %-34s the stair's floor every half second:%s\n", "", stair.c_str());
    }
    int wallsLeft = 0;
    for (int r = 0; r < SHIP_ROOMS; r++) {   // into the corners of each room from its middle (an arch in a wall's middle is a way out)
        if (!MIDDLE[r] || shipRooms()[r].shape == RS_STAIR) continue;
        const ShipView& m = shipView(MIDDLE[r]);
        std::string ends;
        for (int k = 0; k < 4; k++) {
            game.testCabinPlace(r, m.at);
            game.testCabinTurn(PI / 4 + k * PI / 2, 0);
            for (int i = 0; i < 180; i++) { Input step; step.moveY = 1; frame(step); }
            const bool kept = shipWalkable(game.testCabinPlaceNow());
            if (!kept) wallsLeft++;
            ends += fmt(" (%.2f, %.2f)%s%s", game.testCabinPlaceNow().x, game.testCabinPlaceNow().z, game.testCabinRoom() != r ? fmt(" through an arch to the %s", slugOf(shipRooms()[game.testCabinRoom()].name).c_str()).c_str() : "",
                        kept ? "" : " OFF THE FLOOR");
        }
        printf("  into %-29s the corners at%s\n", slugOf(shipRooms()[r].name).c_str(), ends.c_str());
    }
    const bool smooth = worstStep < 0.08;
    printf("ship walk: %d places not reached or faced; %d frames of %d off the floors or in a collider, %d walls left; the floor's largest step in a frame %.3f m%s; the stair's "
           "floor from %.2f to %.2f m\n",
           fails, left, frames, wallsLeft, worstStep, smooth ? "" : " (A JUMP)", stairLo, stairHi);
    return fails + (left ? 1 : 0) + wallsLeft + (smooth ? 0 : 1);
}

// `ship light [scale]`: I-03's light at a scale (2 by default), parked at the start's world: each space from its arch with the light on and
// off; the sun's patches on the bridge's floor through the great window and on the library's under its window at three of the sun's angles;
// the patches moved by a degree of the ship's turn (the pixels that differ, their box); the hull from the dome with the sun high (and
// depolarised, with the bridge so), then at a drifter with no sun. Each frame's daylight and banks printed: none over 32 banks, no bank two hues. To shots/tests/ship/light_*.png; the
// exit code the frames whose banks fail and the pictures missing their light (no patch where the sun shines in, the same lit and dark)
int lightMode(int argc, char** argv) {
    const int scale = argc > 3 ? clampi(atoi(argv[3]), 1, 4) : 2;
    makeDir("shots/tests"); makeDir("shots/tests/ship");
    TestGame game;
    shipGame(game, "light");
    game.settings.renderScale = scale; game.applySettings();
    Input gi;
    runFramesAtLeastOne(game, gi, 0.3);
    const int lights = game.testCabinLights();
    if (!(lights & 1)) pressKey(game, gi, KEY_U);
    if (lights & 2) pressKey(game, gi, KEY_Y);
    runFramesAtLeastOne(game, gi, 1.5);
    for (const Body& b : game.testSystem().bodies) if (b.type != PT_COMPANION) game.testWaitMapOf(b.index);
    const size_t np = (size_t)FBW * FBH;
    int fails = 0, frames = 0, worstBanks = 0;
    // a frame at one instant once it holds still, written; its daylight and banks printed and checked
    auto shot = [&](const std::string& name, std::vector<uint32_t>* keep = nullptr) {
        uint64_t last = 0; int still = 0;
        for (int n = 0; n < 200 && still < 6; n++) {
            game.frame(gi, 0);
            const uint64_t h = fnv(1469598103934665603ULL, game.output(), np * sizeof(uint32_t));
            still = n > 0 && h == last ? still + 1 : 0;
            last = h;
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        const CabinFrame& f = game.testCabinFrame();
        int used = 0; std::string why;
        const bool ok = shipBanksWhole(f, used, why);
        frames++; worstBanks = std::max(worstBanks, used);
        if (!ok) fails++;
        writePNG(fmt("shots/tests/ship/light_%s_%dx.png", name.c_str(), scale).c_str(), game.output(), FBW, FBH);
        if (keep) keep->assign(game.output(), game.output() + np);
        printf("  %-24s sun %.2f (%+.2f %+.2f %+.2f), world %.3f; %d patches; banks %s; %d in use%s%s\n", name.c_str(), f.sunLight, f.sun.x, f.sun.y, f.sun.z, f.worldLight,
               (int)f.patches.size(), banksLine(f.banks).c_str(), used, ok ? "" : ", NOT WHOLE:", why.c_str());
    };
    auto settle = [&](double secs) { for (int i = 0; i < (int)(secs * 30); i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); } };
    printf("ship light: %dx, the spaces lit and dark, the sun's patches, the hull outside\n", scale);
    // each space from its arch, lit and dark (the light eases out over a second or two)
    for (int r = 0; r < SHIP_ROOMS; r++) {
        if (!MIDDLE[r]) continue;
        ShipStand at{};
        if (!archStand(r, at)) at = shipView(MIDDLE[r]).at;
        game.testCabinPlace(r, at);
        std::vector<uint32_t> lit, dark;
        shot(slugOf(shipRooms()[r].name) + "_lit", &lit);
        pressKey(game, gi, KEY_U); settle(3);
        shot(slugOf(shipRooms()[r].name) + "_dark", &dark);
        pressKey(game, gi, KEY_U); settle(3);
        const FrameDiff d = diffFrames(lit.data(), dark.data(), FBW, FBH);
        if (!d.differ) { fails++; printf("  %-24s THE SAME LIT AND DARK\n", ""); }
    }
    // the sun through the great window onto the bridge's floor, and through the library's window onto its floor, at three angles each;
    // B-434: low through the laboratory's window onto its port wall, seen from the hall through the arch (no patch over the arch)
    struct Sun { const char* view; const char* name; double az, el; };
    const Sun SUNS[] = {{"THE BRIDGE'S FLOOR", "bridge_sun_a", -25, 50}, {"THE BRIDGE'S FLOOR", "bridge_sun_b", 0, 32}, {"THE BRIDGE'S FLOOR", "bridge_sun_c", 30, 18},
                        {"THE LIBRARY'S FLOOR", "library_sun_a", -70, 45}, {"THE LIBRARY'S FLOOR", "library_sun_b", -95, 30}, {"THE LIBRARY'S FLOOR", "library_sun_c", -120, 15},
                        {"THE LABORATORY'S ARCH", "lab_arch_sun", 90, 15}};
    auto sunAt = [&](double az, double el) { return Vec3(std::sin(az * DEG) * std::cos(el * DEG), std::sin(el * DEG), std::cos(az * DEG) * std::cos(el * DEG)); };
    for (const Sun& s : SUNS) {
        game.testCabinView(s.view);
        if (!game.testSunAt(sunAt(s.az, s.el))) printf("  %-24s the sun could not be turned to %.0f, %.0f\n", s.name, s.az, s.el);
        shot(s.name);
        if (game.testCabinFrame().patches.empty()) { fails++; printf("  %-24s NO PATCH\n", ""); }
    }
    // the patches' movement: the bridge's at the middle angle, then the ship turned a degree
    {
        game.testCabinView("THE BRIDGE'S FLOOR");
        game.testSunAt(sunAt(0, 32));
        std::vector<uint32_t> a, b;
        shot("bridge_turn_a", &a);
        game.testShipTurnBy(1 * DEG);
        shot("bridge_turn_b", &b);
        const FrameDiff d = diffFrames(a.data(), b.data(), FBW, FBH);
        printf("  the ship turned a degree: %s\n", d.differ ? diffLine(d, FBW, FBH).c_str() : "NOTHING MOVED");
        if (!d.differ) fails++;
    }
    // the hull outside from the dome: the sun high over the starboard bow, then a drifter's sky (no sun)
    game.testCabinView("OVER THE PROW");
    game.testSunAt(sunAt(40, 40));
    shot("bridge_prow");
    game.testCabinView("OVER THE ROOFS");
    shot("dome_sunlit");
    pressKey(game, gi, KEY_Y);   // the hull depolarised: its skin glass (not drawn), what stands outside it kept; the dome glass as ever
    shot("dome_glass");
    game.testCabinView("TOWARD THE BOW");
    shot("bridge_glass");
    pressKey(game, gi, KEY_Y);
    Star d;
    if (placeInSector(TEST_DRIFTERS[1].sx, TEST_DRIFTERS[1].sy, TEST_DRIFTERS[1].sz, d, true)) {
        game.testParkAt(d, TEST_DRIFTERS[1].body, false);
        game.setState(GameState::SPACE);
        runFramesAtLeastOne(game, gi, 1.0);
        for (const Body& b : game.testSystem().bodies) if (b.type != PT_COMPANION) game.testWaitMapOf(b.index);
        game.testCabinView("OVER THE ROOFS");
        shot("dome_sunless");
        if (game.testCabinFrame().sunLight != 0) { fails++; printf("  %-24s A SUN IN A SUNLESS SYSTEM\n", ""); }
    } else { fails++; printf("  the test drifter not found\n"); }
    game.settings.renderScale = 1; game.applySettings();
    printf("ship light: %d pictures in shots/tests/ship; %d frames' banks not whole, at most %d banks in use; %d failures\n", frames, fails, worstBanks, fails);
    return fails;
}

// `ship life [scale]`: I-04's life aboard at a scale (2 by default), parked at the start's world. Twelve photographs taken first (the
// frames of twelve of the plan's views, written as the game writes its photographs into a gallery of the harness's); then the quarters'
// wall lit and by night (U: the bulbs out, the picture lamp and the bedside lamp kept), the garden at the expedition's start and two months
// on, the shards' shelves empty and full (sixty-four worlds' shards, a world's glow by its count), the case's instruments, the player's
// deck, the drive's core and the engine room at rest and in a flight, the capsule in its cradle and the hatch's drop at five instants (the
// landing map's Enter at the capsule). To shots/tests/ship/life_*.png; each frame's banks checked (none over 32, no bank two hues) and its
// time printed. The exit code: the frames whose banks fail and what is missing (no print on the wall, the garden the same at its two
// times, the shelves the same full and empty, the core the same in a flight, the capsule not gone through the hatch)
int lifeMode(int argc, char** argv) {
    const int scale = argc > 3 ? clampi(atoi(argv[3]), 1, 4) : 2;
    makeDir("shots/tests"); makeDir("shots/tests/ship");
    const std::string gallery = "shots/tests/ship_life_gallery";
    makeDir(gallery);
    for (int i = 0; i < 64; i++) remove(fmt("%s/screenshot_%d.png", gallery.c_str(), i).c_str());
    TestGame game;
    shipGame(game, "life");
    game.shotsDir = gallery;
    game.settings.renderScale = scale; game.applySettings();
    Input gi;
    runFramesAtLeastOne(game, gi, 0.3);
    const int lights = game.testCabinLights();
    if (!(lights & 1)) pressKey(game, gi, KEY_U);
    if (lights & 2) pressKey(game, gi, KEY_Y);
    runFramesAtLeastOne(game, gi, 1.5);
    for (const Body& b : game.testSystem().bodies) if (b.type != PT_COMPANION) game.testWaitMapOf(b.index);
    const size_t np = (size_t)FBW * FBH;
    int fails = 0, frames = 0, worstBanks = 0;
    auto still = [&]() {
        uint64_t last = 0; int same = 0;
        for (int n = 0; n < 200 && same < 6; n++) {
            game.frame(gi, 0);
            const uint64_t h = fnv(1469598103934665603ULL, game.output(), np * sizeof(uint32_t));
            same = n > 0 && h == last ? same + 1 : 0;
            last = h;
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    };
    auto shot = [&](const std::string& name, std::vector<uint32_t>* keep = nullptr, const std::string& what = "") {
        still();
        const double t0 = nowSec(); game.frame(gi, 0); const double ms = (nowSec() - t0) * 1000;
        const CabinFrame& f = game.testCabinFrame();
        int used = 0; std::string why;
        const bool ok = shipBanksWhole(f, used, why);
        frames++; worstBanks = std::max(worstBanks, used);
        if (!ok) fails++;
        writePNG(fmt("shots/tests/ship/life_%s_%dx.png", name.c_str(), scale).c_str(), game.output(), FBW, FBH);
        if (keep) keep->assign(game.output(), game.output() + np);
        printf("  %-20s %6.2f ms; %d banks in use%s%s%s%s\n", name.c_str(), ms, used, ok ? "" : ", NOT WHOLE:", why.c_str(), what.empty() ? "" : "; ", what.c_str());
    };
    auto settle = [&](double secs) { for (int i = 0; i < (int)(secs * 30); i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); } };
    auto differ = [&](const char* what, const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
        const FrameDiff d = diffFrames(a.data(), b.data(), FBW, FBH);
        printf("  %-20s %s\n", what, d.differ ? diffLine(d, FBW, FBH).c_str() : "THE SAME");
        if (!d.differ) fails++;
    };
    printf("ship life: %dx, the photographs on the quarters' wall, the garden, the shelves, the case, the deck, the microscope, the cold store, the incubator, the jars, "
           "the core, the capsule's drop\n", scale);
    // twelve photographs, the oldest first
    const char* const PHOTOS[12] = {"THE FRONT WINDOW", "THE HALL'S MIDDLE", "THE LEFT WINDOW", "OVER THE ROOFS", "THE BRIDGE'S MIDDLE", "THE LIBRARY'S MIDDLE",
                                    "THE MAINTENANCE ROOM'S MIDDLE", "OVER THE PROW", "THE OBSERVATORY'S MIDDLE", "THE HANGAR'S MIDDLE", "THE RIGHT WINDOW", "TOWARD THE BOW"};
    game.photoMode = true;   // the HUD out of them
    for (int i = 0; i < 12; i++) {
        game.testCabinView(PHOTOS[i]);
        still();
        writePNG(fmt("%s/screenshot_%d.png", gallery.c_str(), i).c_str(), game.output(), FBW, FBH);
        game.noteScreenshot();
    }
    game.photoMode = false;
    // the quarters' wall, lit and by night
    game.testCabinView("THE QUARTERS' WALL");
    game.frame(gi, 0);
    game.testWaitPrints();
    const int printed = game.testPrints();
    std::vector<uint32_t> lit, hung;
    shot("quarters_lit", &lit, fmt("%d prints on the wall", printed));
    if (printed != 12) { fails++; printf("  %-20s %d PRINTS, NOT TWELVE\n", "", printed); }
    pressKey(game, gi, KEY_U); settle(3);
    shot("quarters_night");
    pressKey(game, gi, KEY_U); settle(3);
    // R-429: the garden as planted and an hour of play on (grown full); the clock and the time played put back after it as I-04's garden
    // left them, so the frames after are taken when they were (the bed trimmed by E comes last, with the wall)
    game.testCabinView("THE GARDEN");
    std::vector<uint32_t> young, grown, trimmed;
    const double t0 = game.testGameTime(), p0 = game.testPlayed();
    game.testGardenPlanted(); settle(0.2);
    shot("garden_planted", &young, fmt("grown %.2f, the home world's flora %s", game.testGardenGrowth(), game.testGardenHome() ? "found" : "NOT FOUND"));
    game.testSetPlayed(GARDEN_GROW_S); settle(0.2);
    shot("garden_grown", &grown, fmt("grown %.2f", game.testGardenGrowth()));
    differ("the garden grown", young, grown);
    if (!game.testGardenHome()) fails++;
    game.testSetTime(t0); game.testSetPlayed(p0 + 0.4); settle(0.2);
    // the shelves empty, then sixty-four worlds' shards (a world k holds k % 16 + 1)
    game.testCabinView("AT THE SHELVES");
    std::vector<uint32_t> empty, full;
    shot("shelves_empty", &empty);
    for (int w = 0; w < 64; w++)
        for (int i = 0; i <= w % 16; i++) game.guide.shards.insert(fmt("900,0,%d/1/S%d", 100 + w, i));
    shot("shelves_full", &full);
    printf("  %-20s %d worlds' shards on the shelves\n", "", game.testShelfWorlds());
    differ("the shelves filled", empty, full);
    game.testCabinView("THE SHELVES");
    shot("library_shelves");
    // the case's instruments (twelve held), the player's deck
    for (int i = 0; i < 12; i++) game.guide.instruments.insert(fmt("900,0,%d/1/I%d", 300 + i, 1000 + i * 7));
    game.testCabinView("THE CASE'S GLASS");
    shot("case");
    game.testCabinAtDevice(FACE_DECK);
    shot("deck");
    // Z-04: the laboratory's microscope from its stand, the cold store's door empty and with forty vials (five of its eight lamps lit)
    game.testCabinAtDevice(FACE_MICROSCOPE);
    shot("microscope");
    game.testCabinView("THE COLD STORE");
    std::vector<uint32_t> storeEmpty, storeFull;
    game.testColdStore().clear();
    shot("store_empty", &storeEmpty);
    game.testColdStore().assign(40, Vial());
    shot("store_full", &storeFull, "forty vials");
    differ("the cold store filled", storeEmpty, storeFull);
    game.testColdStore().clear();
    // Z-05: the incubator empty, then its twelve flasks of a living world's species in their best media (white, red and blue light, a crowd's
    // own light in the dark) fifteen minutes on, and by night; the jars' shelf with four worlds' jars settled; the stern bench whole
    game.testCabinView("THE INCUBATOR");
    std::vector<uint32_t> incEmpty, incFull;
    shot("incubator_empty", &incEmpty);
    const int stocked = labStock(game);
    shot("incubator", &incFull, fmt("%d flasks", stocked));
    differ("the incubator stocked", incEmpty, incFull);
    if (stocked != INCUBATOR_FLASKS) { fails++; printf("  %-20s %d FLASKS, NOT %d\n", "", stocked, INCUBATOR_FLASKS); }
    pressKey(game, gi, KEY_U); settle(3);
    shot("incubator_night");
    pressKey(game, gi, KEY_U); settle(3);
    game.testCabinView("THE JARS");
    shot("jars", nullptr, fmt("%zu jars", game.testLab().jars.size()));
    game.testCabinView("THE STERN BENCH");
    shot("lab_bench");
    // Z-06: the sequencer on the bow bench idle, then reading (its screen pulsing); the tree of kin's screen on the port wall (the status
    // before them put back after, the frames after as they were)
    const auto keptStatus = game.testStatusKept();
    game.testCabinView("THE SEQUENCER");
    std::vector<uint32_t> seqIdle, seqReading;
    shot("sequencer", &seqIdle);
    {
        const Flask& F = game.testLab().flasks[0];
        if (F.n > 0) game.testSeqPut(F.who[0].sx, F.who[0].sy, F.who[0].sz, F.who[0].body, F.who[0].species);
    }
    game.testLabSkip(SEQUENCE_S / 3);
    shot("sequencer_reading", &seqReading, game.testLab().seq.on ? "reading" : "NOTHING ON IT");
    differ("the sequencer reading", seqIdle, seqReading);
    game.testCabinView("THE TREE OF KIN");
    shot("tree");
    game.testStatusPut(keptStatus);
    // the drive's core at rest and in a flight; R-427: the maintenance room, the buggy and the drone on their platforms
    std::vector<uint32_t> rest, flight;
    game.testCabinView("THE CORE");
    shot("core_rest", &rest);
    game.testCabinDrive(1);
    shot("core_flight", &flight);
    differ("the core in a flight", rest, flight);
    game.testCabinDrive(0);
    game.testCabinView("THE MAINTENANCE ROOM'S MIDDLE");
    shot("maintenance");
    game.testCabinView("THE PLATFORMS");
    shot("maintenance_platforms");
    // R-428: E at the wall: its frames as the wall holds them; the photographs for the third frame, the oldest hung there, the sixth taken
    // down; the wall in the room as the explorer hung it. R-429: the bed in view grown full, then trimmed back by E at it
    game.testCabinAtDevice(FACE_PRINTS); settle(0.2); pressKey(game, gi, KEY_E);
    shot("wall_screen", nullptr, game.state == GameState::WALL ? "the wall's screen, the newest twelve" : "NOT THE WALL'S SCREEN");
    pressKey(game, gi, KEY_RIGHT); pressKey(game, gi, KEY_RIGHT); pressKey(game, gi, KEY_ENTER);
    for (int i = 0; i < 9; i++) pressKey(game, gi, KEY_RIGHT);
    shot("wall_pick", nullptr, fmt("level %d, the frame %d", game.testWallLevel(), 3));
    pressKey(game, gi, KEY_ENTER); pressKey(game, gi, KEY_DOWN); pressKey(game, gi, KEY_LEFT); pressKey(game, gi, KEY_BACKSPACE);
    const bool asHung = game.testWallChosen() && game.testWallNumber(2) == 0 && game.testWallNumber(5) < 0 && game.testWallNumber(0) == 11;
    pressKey(game, gi, KEY_ESCAPE);
    game.testCabinView("THE QUARTERS' WALL"); settle(0.2); game.testWaitPrints(); settle(0.1);
    shot("wall_hung", &hung, fmt("%d prints; the third frame photograph %d, the sixth %s", game.testPrints(), game.testPrintAt(2), game.testPrintAt(5) < 0 ? "empty" : "NOT EMPTY"));
    differ("the wall as hung", lit, hung);
    if (!asHung || game.testPrints() != 11 || game.testPrintAt(2) != 0) { fails++; printf("  %-20s THE WALL NOT AS HUNG\n", ""); }
    game.testSetPlayed(game.testPlayed() + GARDEN_GROW_S);
    game.testCabinAtDevice(FACE_BED_4); settle(0.2); pressKey(game, gi, KEY_E);
    const std::string said = game.testStatus();
    game.testCabinView("THE GARDEN"); settle(0.2);
    shot("garden_trimmed", &trimmed, fmt("grown %.2f, '%s'", game.testGardenGrowth(), said.c_str()));
    differ("the bed trimmed", grown, trimmed);
    // the capsule in its cradle, then the hatch's drop: E at the capsule, Enter on the landing map, five instants of it
    game.testCabinView("THE HATCH'S DROP");
    std::vector<uint32_t> cradle, gone;
    shot("hangar_capsule", &cradle);
    game.testCabinAtDevice(FACE_CAGE);
    game.frame(gi, 0);
    pressKey(game, gi, KEY_E);
    game.testWaitMap();
    pressKey(game, gi, KEY_ENTER);
    printf("  E at the capsule, Enter on the landing map: %s, the descent's clock %.2f s\n", game.state == GameState::DESCENT ? "the descent" : "NOT THE DESCENT", game.testTransT());
    for (double at : {0.4, 1.0, 1.6, 2.4, 3.2}) {
        while (game.testTransT() < at - TestGame::DROP_SECONDS) { game.frame(gi, 1.0 / 60); gi.newFrame(); }
        shot(fmt("drop_%02.0f", at * 10), at > 3 ? &gone : nullptr, fmt("%.2f s into the drop", game.testTransT() + TestGame::DROP_SECONDS));
    }
    differ("the capsule dropped", cradle, gone);
    game.settings.renderScale = 1; game.applySettings();
    printf("ship life: %d pictures in shots/tests/ship; at most %d banks in use; %d failures\n", frames, worstBanks, fails);
    return fails;
}
}

bool shipBanksWhole(const CabinFrame& f, int& used, std::string& why) {
    const ShipBanks& b = f.banks;
    int owner[32];
    for (int& o : owner) o = -1;
    bool ok = true;
    for (int h = 0; h < SHIP_HOMES; h++) {
        const int d = b.of[h];
        if (d < 0) continue;
        if (owner[d] >= 0) { ok = false; why += fmt(" bank %d to %d and %d", d, owner[d], h); }
        owner[d] = h;
        if (d == 0 || d == BANK_HULL || d == BANK_LIGHT) { ok = false; why += fmt(" bank %d dealt to %d", d, h); }
        if (f.sunk && d == BANK_SPACE) { ok = false; why += fmt(" bank %d (the outside's) to %d", d, h); }
    }
    used = b.dealt + 3 + (f.sunk ? 1 : 0);   // B-437: the ship's palette: the dealt, 0, the hull's, the light's and the outside's one (its own palette apart)
    if (used > 32) { ok = false; why += fmt(" %d banks", used); }
    return ok;
}

int runShip(int argc, char** argv) {
    const std::string what = argc > 2 ? argv[2] : "rooms";
    if (what == "rooms") return roomsMode(argc, argv);
    if (what == "walk") return walkMode(argc, argv);
    if (what == "light") return lightMode(argc, argv);
    if (what == "life") return lifeMode(argc, argv);
    printf("ship rooms [scale ...] | walk | light [scale] | life [scale]\n");
    return 1;
}
