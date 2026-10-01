// Save slots, autosave and the save file format (M0-04).
#include "game.h"
#include "core/fs.h"
#include "ui.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace {
// `saved <unix time>` of a save file, -1 if the file is missing or not a save
long long savedStamp(const std::string& path) {
    std::ifstream f(path);
    if (!f) return -1;
    std::string line;
    std::getline(f, line);
    if (line.rfind("vesperis-save", 0) != 0) return -1;
    long long stamp = 0;
    while (std::getline(f, line)) {
        std::istringstream is(line);
        std::string key; is >> key;
        if (key == "saved") { is >> stamp; break; }
    }
    return stamp;
}
}

bool Game::saveSlot(int n) {
    currentSlot = n;
    bool ok = save(slotPath(n));
    status(ok ? fmt("EXPEDITION SAVED TO SLOT %d", n) : "COULD NOT WRITE THE SAVE FILE", 3);
    if (!ok) audio.beep = 3;
    return ok;
}

bool Game::autosave() {
    bool ok = save(slotPath(0));
    if (ok) status("AUTOSAVED", 2);
    return ok;
}

bool Game::loadSlot(int n) {
    if (!fileExists(slotPath(n))) { status(n == 0 ? "NO AUTOSAVE" : fmt("SLOT %d IS EMPTY", n), 3); audio.beep = 3; return false; }
    if (!load(slotPath(n))) { audio.beep = 3; return false; }   // load says why (G-01: a star gone with the galaxy)
    if (n > 0) currentSlot = n;
    status(n == 0 ? "AUTOSAVE LOADED" : fmt("EXPEDITION LOADED FROM SLOT %d", n), 3);
    return true;
}



// Continue = the most recent of the slots, the autosave and the legacy single file.
bool Game::loadNewest() {
    std::vector<std::string> paths = {slotPath(1), slotPath(2), slotPath(3), slotPath(0), savePrefix + ".txt"};
    std::string best; long long bestStamp = -1; int bestSlot = 1;
    for (size_t i = 0; i < paths.size(); i++) {
        long long st = savedStamp(paths[i]);
        if (st > bestStamp) { bestStamp = st; best = paths[i]; bestSlot = i < 3 ? (int)i + 1 : 1; }
    }
    if (bestStamp < 0) return false;
    if (!load(best)) return false;
    currentSlot = bestSlot;
    return true;
}

std::string Game::saveSummary(const std::string& path) const {
    std::ifstream f(path);
    if (!f) return "";
    std::string line;
    std::getline(f, line);
    if (line.rfind("vesperis-save", 0) != 0) return "";
    std::string name = "?"; double st = 0; int surface = 0, phase = -1;
    while (std::getline(f, line)) {
        std::istringstream is(line);
        std::string key; is >> key;
        if (key == "name") { std::getline(is, name); if (!name.empty() && name[0] == ' ') name.erase(0, 1); }
        else if (key == "t") is >> st;
        else if (key == "surface") is >> surface;
        else if (key == "phase") is >> phase;
    }
    int epoc = 6011 + (int)(st / 1e9);
    int sinister = (int)std::fmod(st / 1e6, 1000.0), medius = (int)std::fmod(st / 1e3, 1000.0);
    const char* where = phase == (int)GameState::DESCENT ? "DESCENDING" : (phase == (int)GameState::ASCENT ? "ASCENDING" : (surface ? "ON THE SURFACE" : "IN SPACE"));
    return fmt("%-12.12s %d:%03d.%03d  %s", upper(name).c_str(), epoc, sinister, medius, where);
}

void Game::openSlots(bool loadMode) {
    slotLoadMode = loadMode;
    slotSel = std::max(0, currentSlot - 1);
    slotSummaries.clear();
    for (int n = 1; n <= 3; n++) slotSummaries.push_back(saveSummary(slotPath(n)));
    slotSummaries.push_back(saveSummary(slotPath(0)));
    state = GameState::SLOTS;
}

void Game::updateSlots(const Input& in) {
    int n = slotLoadMode ? 4 : 3;
    if (in.wasPressed(KEY_UP)) slotSel = (slotSel + n - 1) % n;
    if (in.wasPressed(KEY_DOWN)) slotSel = (slotSel + 1) % n;
    if (in.wasPressed(KEY_ESCAPE)) { state = GameState::MENU; return; }
    if (enterKey(in)) {
        int slot = slotSel == 3 ? 0 : slotSel + 1;
        if (slotLoadMode) { if (loadSlot(slot)) return; }
        else { saveSlot(slot); state = returnState; }
    }
}

void Game::renderSlots() {
    const int top = 44, bottom = UH - 44, y0 = top + 26;
    blendRectRGB(canvas, 16, top, UW - 16, bottom, rgb(0, 0, 0), 215);
    drawRectRGB(canvas, 16, top, UW - 16, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 8, slotLoadMode ? "LOAD EXPEDITION" : "SAVE EXPEDITION", HUD_AMBER);
    int n = slotLoadMode ? 4 : 3;
    for (int i = 0; i < n; i++) {
        int y = y0 + i * 12;
        bool sel = i == slotSel;
        std::string label = i == 3 ? "AUTO  " : fmt("SLOT %d", i + 1);
        std::string sum = slotSummaries[i].empty() ? "- EMPTY -" : slotSummaries[i];
        drawText(canvas, 30, y, (label + "  " + sum).c_str(), sel ? HUD_WHITE : (slotSummaries[i].empty() ? HUD_DIM : HUD_GREEN));
        if (sel) drawText(canvas, 22, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 12, slotLoadMode ? "ENTER LOAD  ESC BACK" : "ENTER SAVE  ESC BACK", HUD_DIM);
}

bool Game::save(const std::string& path) {
    guide.save(guidePath);
    std::ofstream f(path);
    if (!f) return false;
    f.precision(17);
    f << "vesperis-save 2\n";
    f << "gen " << GEN_VERSION << "\n";
    f << "saved " << (long long)std::time(nullptr) << "\n";
    f << "name " << (sys.valid ? starNameOf(sys.star) : std::string("deep space")) << "\n";
    f << "t " << t << "\n";
    f << "warp " << timeWarp << "\n";
    f << "star " << sys.star.sx << " " << sys.star.sy << " " << sys.star.sz << " " << (sys.valid ? 1 : 0) << "\n";
    f << "ship " << ship.pos.x << " " << ship.pos.y << " " << ship.pos.z << " " << ship.yaw << " " << ship.pitch << "\n";
    f << "mode " << (int)ship.mode << " " << ship.parkedBody << " " << ship.localTarget << " " << (ship.orbiting ? 1 : 0) << "\n";
    f << "park " << ship.parkDir.x << " " << ship.parkDir.y << " " << ship.parkDir.z << " " << ship.parkDist << "\n";
    f << "belt " << ship.targetBelt << " " << ship.parkedBelt << " " << ship.rockIr << " " << ship.rockIa << " " << ship.rockIy << " " << ship.rockM << " " << ship.rockDist << "\n";   // O3
    f << "remote " << (ship.hasRemote ? 1 : 0) << " " << ship.remote.sx << " " << ship.remote.sy << " " << ship.remote.sz << "\n";
    f << "cabin " << cabin.x << " " << cabin.z << " " << cabin.yaw << " " << cabin.pitch << " " << (cabin.light ? 1 : 0) << " " << (cabin.depolarised ? 1 : 0) << " " << (cabin.onRoof ? 1 : 0) << "\n";
    // where the explorer is: on the surface in the surface states, and in every overlay state (menus, guide screens,
    // the title after a load, ...) when the overlay returns to the surface. The title was missing here, so closing
    // the window on the title (or `--frames`) autosaved the expedition as "in space" and lost the landing (2026-09-27)
    bool inPlay = state == GameState::SPACE || state == GameState::LANDING_MAP || state == GameState::SURFACE || state == GameState::DESCENT || state == GameState::ASCENT;
    bool onSurface = surf.valid && (state == GameState::SURFACE || state == GameState::DESCENT || state == GameState::ASCENT || (!inPlay && returnState == GameState::SURFACE));
    f << "surface " << (onSurface ? 1 : 0) << "\n";
    if (onSurface) {
        f << "site " << surf.site.body << " " << surf.site.lat0 << " " << surf.site.lon0 << "\n";
        f << "player " << surf.player.x << " " << surf.player.z << " " << surf.player.yaw << " " << surf.player.pitch << "\n";
        f << "capsule " << surf.capsuleX << " " << surf.capsuleZ << "\n";
        if (state == GameState::DESCENT || state == GameState::ASCENT) f << "phase " << (int)state << " " << transT << "\n";
        f << "buggy " << (surf.buggy.deployed ? 1 : 0) << " " << surf.buggy.x << " " << surf.buggy.z << " " << surf.buggy.heading << " " << surf.buggy.odometer << " " << (surf.inBuggy ? 1 : 0) << "\n";
        f << "waypoint " << (surf.hasWaypoint ? 1 : 0) << " " << surf.wpX << " " << surf.wpZ << "\n";
        f << "stamina " << surf.player.stamina << "\n";
    }
    hasSave = true;
    return true;
}

bool Game::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    std::getline(f, line);
    if (line.rfind("vesperis-save", 0) != 0) { status("NOT A VESPERIS SAVE", 3); return false; }   // versions 1 and 2 share the keys below
    int64_t sx = 0, sy = 0, sz = 0; int valid = 0;
    int mode = 0, parked = -1, local = -1, orbiting = 1, hasRemote = 0; int64_t rx = 0, ry = 0, rz = 0;
    int onSurface = 0, siteBody = -1; double slat = 0, slon = 0, px = 0, pz = 0, pyaw = 0, ppitch = 0, capx = 0, capz = 0;
    int phase = -1; double phaseT = 0;
    int bugDep = 0, bugIn = 0, wpHas = 0; double bugX = 0, bugZ = 0, bugH = 0, bugOdo = 0, wpx = 0, wpz = 0, stam = 100;
    int gen = 1;
    Cabin cab; int cl = 1, cd = 0, cr = 0;
    ShipState ns;
    double nt = t, warp = 1;
    while (std::getline(f, line)) {
        std::istringstream is(line);
        std::string key; is >> key;
        if (key == "t") is >> nt;
        else if (key == "warp") is >> warp;
        else if (key == "star") is >> sx >> sy >> sz >> valid;
        else if (key == "ship") is >> ns.pos.x >> ns.pos.y >> ns.pos.z >> ns.yaw >> ns.pitch;
        else if (key == "mode") is >> mode >> parked >> local >> orbiting;
        else if (key == "park") is >> ns.parkDir.x >> ns.parkDir.y >> ns.parkDir.z >> ns.parkDist;
        else if (key == "belt") is >> ns.targetBelt >> ns.parkedBelt >> ns.rockIr >> ns.rockIa >> ns.rockIy >> ns.rockM >> ns.rockDist;
        else if (key == "remote") is >> hasRemote >> rx >> ry >> rz;
        else if (key == "surface") is >> onSurface;
        else if (key == "site") is >> siteBody >> slat >> slon;
        else if (key == "player") is >> px >> pz >> pyaw >> ppitch;
        else if (key == "capsule") is >> capx >> capz;
        else if (key == "phase") is >> phase >> phaseT;
        else if (key == "buggy") is >> bugDep >> bugX >> bugZ >> bugH >> bugOdo >> bugIn;
        else if (key == "waypoint") is >> wpHas >> wpx >> wpz;
        else if (key == "stamina") is >> stam;
        else if (key == "gen") is >> gen;
        else if (key == "cabin") { is >> cab.x >> cab.z >> cab.yaw >> cab.pitch >> cl >> cd >> cr; cab.light = cl != 0; cab.depolarised = cd != 0; cab.onRoof = cr != 0; }
    }
    Star s;
    if (!starInSector(sx, sy, sz, s, true)) {   // G-01: the galaxy of generation 11 keeps about half of the old stars near home
        status(fmt("THE GALAXY WAS REBUILT SINCE THIS SAVE (GEN %d, NOW %d) - ITS STAR IS GONE", gen, GEN_VERSION), 8);
        return false;
    }
    t = nt; timeWarp = warp;
    sys.generate(s);
    sys.valid = valid != 0;
    ship = ns;
    cabin = cab; cabin.lightLevel = cabin.light ? 1 : 0;
    ship.mode = (ShipState::Mode)mode;
    if (ship.mode == ShipState::VIMANA || ship.mode == ShipState::APPROACH) ship.mode = ShipState::STANDBY;
    ship.parkedBody = parked < (int)sys.bodies.size() ? parked : -1;
    ship.localTarget = local < (int)sys.bodies.size() ? local : -1;
    ship.orbiting = orbiting != 0;
    if (hasRemote && starInSector(rx, ry, rz, ship.remote, true)) ship.hasRemote = true;
    if (ship.targetBelt >= (int)sys.belts.size()) ship.targetBelt = -1;   // O3
    if (ship.parkedBelt >= (int)sys.belts.size()) ship.parkedBelt = -1;
    if (ship.mode == ShipState::PARKED && ship.parkedBody < 0 && ship.parkedBelt < 0) ship.mode = ShipState::STANDBY;
    nb.update(ship.pos);
    surf.valid = false;
    lastSecX = lastSecY = -1; landZoom = 0; sectorImgEpoch = -1;   // O2, B-303
    if (onSurface && siteBody >= 0 && siteBody < (int)sys.bodies.size()) {
        surf.init(&sys, siteBody, slat, slon, t);
        surf.player.x = px; surf.player.z = pz; surf.player.yaw = pyaw; surf.player.pitch = ppitch;
        surf.player.y = surf.site.surfaceHeight(px, pz);
        surf.relocateCapsule(capx, capz);
        if (bugDep) { surf.buggy = Buggy(); surf.buggy.deployed = true; surf.buggy.unfold = 1; surf.buggy.x = bugX; surf.buggy.z = bugZ; surf.buggy.heading = bugH; surf.buggy.odometer = bugOdo; surf.buggy.y = surf.site.surfaceHeight(bugX, bugZ); if (bugIn) surf.toggleBuggy(); }
        surf.hasWaypoint = wpHas != 0; surf.wpX = wpx; surf.wpZ = wpz;
        surf.player.stamina = clampd(stam, 0, 100);
        state = GameState::SURFACE;
        if (phase == (int)GameState::DESCENT || phase == (int)GameState::ASCENT) {
            // resume the transition where it was: the camera path is a function of transT
            state = (GameState)phase;
            transT = clampd(phaseT, 0, 7.0);
            surf.cameraOverrideAlt = 0;
            fade = state == GameState::DESCENT ? 0 : 1;
        }
    } else state = GameState::SPACE;
    returnState = state == GameState::SURFACE ? GameState::SURFACE : GameState::SPACE;
    hasSave = true;
    autosaveTimer = 0;
    if (gen < 11) { statusNext = fmt("THE GALAXY WAS REBUILT SINCE THIS SAVE (GEN %d, NOW %d) - THIS STAR IS STILL HERE", gen, GEN_VERSION); statusNextSecs = 8; }   // G-01
    else if (gen < GEN_VERSION) { statusNext = fmt("WORLDS WERE REGENERATED SINCE THIS SAVE (GEN %d, NOW %d) - STARS ARE WHERE THEY WERE", gen, GEN_VERSION); statusNextSecs = 8; }
    return true;
}
