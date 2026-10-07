// W-01: the almanac on the screen. Ctrl+T (the data sheet's T, the flight computer's devices page) opens the list of the
// next events at the place, each with its countdown, and a free row ("run for an hour"); Enter runs the clock to the
// one chosen: the warp climbs to x10000 and falls as the moment nears, (remaining / 3 s)^1.5 and x1 for the last three
// seconds, so the last minute of game time takes about five seconds of ours and the clock lands on the event itself.
// On the landing map the place is the cursor's site; on the ship the events are the system's as seen from the parking.
#include "game.h"
#include "ui.h"
#include <cmath>
#include <algorithm>

namespace {
const double FREE_STEPS[] = {60, 120, 300, 600, 1200, 1800, 3600, 7200, 3 * 3600.0, 6 * 3600.0, 12 * 3600.0, 86400, 2 * 86400.0, 5 * 86400.0};
const int FREE_COUNT = (int)(sizeof(FREE_STEPS) / sizeof(FREE_STEPS[0]));
const int ALMANAC_ROWS = 15;
const double GROUND_WINDOW = 5 * 86400.0, SHIP_WINDOW = 30 * 86400.0;
}

double Game::almanacWarpFor(double remaining) { return clampd(std::pow(std::max(0.0, remaining) / 3.0, 1.5), 1.0, 10000.0); }

std::string Game::almanacLabelOf(const AlmanacEvent& e) const {
    return almanacLabel(e, sys, [&](int j) { return bodyNameOf(j); });
}

// where the almanac is read: the explorer's feet, the landing map's cursor, or the ship's parking
void Game::buildAlmanac() {
    AlmanacPlace p;
    p.sys = sys.valid ? &sys : nullptr;
    p.t = t;
    if (almanacScene == 1 && surf.valid) {
        p.ground = true; p.body = surf.site.body; p.window = GROUND_WINDOW;
        surf.site.latLonAt(surf.player.x, surf.player.z, p.lat, p.lon);
        p.atmosphere = surf.site.atmosphere; p.ringProf = surf.site.ringProf;
    } else if (almanacScene == 2 && sys.valid && landBody >= 0 && landBody < (int)sys.bodies.size()) {
        const Body& b = sys.bodies[landBody];
        p.ground = true; p.body = landBody; p.window = GROUND_WINDOW; p.lat = landLat; p.lon = landLon;
        p.atmosphere = PLANET_TYPES[b.type].atmosphere;
        if (b.rings) p.ringProf = spaceR.ringProfile(sys, landBody);
    } else {
        p.ground = false; p.window = SHIP_WINDOW; p.pos = ship.pos;
        p.body = (sys.valid && ship.mode == ShipState::PARKED && ship.parkedBelt < 0) ? ship.parkedBody : -1;
        p.parkDir = ship.parkDir; p.parkDist = ship.parkDist; p.orbiting = ship.orbiting;
    }
    almanacAt = p;
    almanacEvents.clear();
    if (p.sys) almanacOf(p, almanacEvents);
    almanacBuiltT = t;
    if (almanacSel > (int)almanacEvents.size()) almanacSel = (int)almanacEvents.size();
}

void Game::openAlmanac() {
    almanacReturn = state == GameState::DATA ? returnState : state;
    almanacScene = (state == GameState::SURFACE || (state == GameState::DATA && returnState == GameState::SURFACE)) && surf.valid ? 1 : (state == GameState::LANDING_MAP ? 2 : 0);
    almanacSel = 0; almanacScroll = 0;
    buildAlmanac();
    state = GameState::ALMANAC;
    audio.beep = 4;
}

void Game::almanacRunTo(const AlmanacEvent& e) {
    almanacRunning = true; almanacRunT = e.t; almanacRunEvent = e;
    almanacRunLabel = almanacLabelOf(e);
    status(fmt("RUNNING TO %s - T STOPS", almanacRunLabel.c_str()), 3);
    audio.beep = 4;
}

void Game::almanacStop(bool arrived) {
    almanacRunning = false; timeWarp = 1;
    if (arrived) { status(fmt("%s - X1", almanacRunLabel.c_str()), 5); audio.beep = 2; }
    else status("RUN STOPPED - X1", 2);
}

void Game::updateAlmanac(const Input& in) {
    int n = (int)almanacEvents.size() + 1;   // the events and the free row
    if (in.wasPressed(KEY_UP)) almanacSel = (almanacSel + n - 1) % n;
    if (in.wasPressed(KEY_DOWN)) almanacSel = (almanacSel + 1) % n;
    if (in.wasPressed(KEY_PAGE_UP)) almanacSel = std::max(0, almanacSel - ALMANAC_ROWS);
    if (in.wasPressed(KEY_PAGE_DOWN)) almanacSel = std::min(n - 1, almanacSel + ALMANAC_ROWS);
    if (almanacSel == n - 1) {
        if (in.wasPressed(KEY_LEFT)) almanacFree = std::max(0, almanacFree - 1);
        if (in.wasPressed(KEY_RIGHT)) almanacFree = std::min(FREE_COUNT - 1, almanacFree + 1);
    }
    if (in.wasPressed(KEY_ESCAPE)) { state = almanacReturn; return; }
    if (!enterKey(in)) return;
    if (settings.clockMode == 1) { status("REAL-TIME CLOCK: THE SKY FOLLOWS THE WALL CLOCK, NO RUN", 3); audio.beep = 3; return; }
    if (almanacSel == n - 1) { AlmanacEvent e; e.kind = AL_FREE; e.t = t + FREE_STEPS[almanacFree]; e.value = FREE_STEPS[almanacFree]; almanacRunTo(e); }
    else almanacRunTo(almanacEvents[almanacSel]);
    state = almanacReturn == GameState::SHIPSCREEN ? GameState::SPACE : almanacReturn;   // the run is watched from the window
}

// the ground's state now and when it changes: "DAY  DARK IN 2 H 05 MIN"
std::string Game::almanacStateLine() const {
    const AlmanacPlace& p = almanacAt;
    if (!p.sys) return "NO SYSTEM HERE";
    if (!p.ground) return p.body >= 0 ? fmt("%d EVENTS WITHIN 30 DAYS, FROM THE PARKING", (int)almanacEvents.size()) : fmt("%d EVENTS WITHIN 30 DAYS", (int)almanacEvents.size());
    double angR, alt = almanacSunAltitude(p, t, &angR);
    const char* now = alt + angR > 0 ? "DAY" : (p.atmosphere && std::sin(alt) > -0.12 ? "TWILIGHT" : "NIGHT");
    bool dark = alt + angR <= 0 && !(p.atmosphere && std::sin(alt) > -0.12);
    int want = dark ? (p.atmosphere ? AL_DAWN : AL_SUNRISE) : (p.atmosphere ? AL_DUSK : AL_SUNSET);
    for (const AlmanacEvent& e : almanacEvents)
        if (e.kind == want) return fmt("%s  %s IN %s", now, dark ? "LIGHT" : "DARK", countdownString(e.t - t).c_str());
    const Body& b = p.sys->bodies[p.body];
    return fmt("%s  %s", now, b.locked ? "THE SUN STANDS STILL HERE" : "NO CHANGE WITHIN 5 DAYS");
}

void Game::renderAlmanac() {
    const int top = 10, bottom = UH - 10, x0 = 14;
    blendRectRGB(canvas, x0, top, UW - x0, bottom, rgb(0, 0, 0), 240);
    drawRectRGB(canvas, x0, top, UW - x0, bottom, HUD_DIM);
    const AlmanacPlace& p = almanacAt;
    std::string place;
    if (p.ground && p.sys) place = fmt("%s %.1f%s %.1f%s", trunc(upper(bodyNameOf(p.body)), 14).c_str(), std::fabs(p.lat / DEG), p.lat >= 0 ? "N" : "S", std::fabs(p.lon / DEG), p.lon >= 0 ? "E" : "W");
    else if (p.sys && p.body >= 0) place = "THE STARDRIFTER AT " + trunc(upper(bodyNameOf(p.body)), 14);
    else place = "THE STARDRIFTER";
    drawText(canvas, x0 + 6, top + 5, ("THE ALMANAC - " + place).c_str(), HUD_AMBER);
    drawText(canvas, x0 + 6, top + 14, almanacStateLine().c_str(), HUD_DIM);
    int n = (int)almanacEvents.size() + 1;
    if (almanacSel < almanacScroll) almanacScroll = almanacSel;
    if (almanacSel >= almanacScroll + ALMANAC_ROWS) almanacScroll = almanacSel - ALMANAC_ROWS + 1;
    const int y0 = top + 26;
    for (int r = 0; r < ALMANAC_ROWS; r++) {
        int i = almanacScroll + r;
        if (i >= n) break;
        int y = y0 + r * 9;
        bool sel = i == almanacSel, free = i == n - 1;
        std::string label = free ? "RUN FOR " + countdownString(FREE_STEPS[almanacFree]) : almanacLabelOf(almanacEvents[i]);
        drawText(canvas, x0 + 12, y, trunc(label, 36).c_str(), sel ? HUD_WHITE : (free ? HUD_AMBER : HUD_GREEN));
        std::string right = free ? std::string("LEFT/RIGHT") : "IN " + countdownString(almanacEvents[i].t - t);
        drawText(canvas, UW - x0 - 6 - textWidth(right.c_str()), y, right.c_str(), free ? HUD_DIM : (sel ? HUD_WHITE : HUD_CYAN));
        if (sel) drawText(canvas, x0 + 4, y, ">", HUD_AMBER);
    }
    if (almanacScroll > 0) drawText(canvas, UW - x0 - 10, y0 - 8, "\x02", HUD_DIM);
    if (almanacScroll + ALMANAC_ROWS < n) drawText(canvas, UW - x0 - 10, y0 + ALMANAC_ROWS * 9, "\x03", HUD_DIM);
    const char* foot = settings.clockMode == 1 ? "REAL-TIME CLOCK: NO RUNNING TO AN EVENT  ESC CLOSE" : "UP/DOWN SELECT  ENTER RUNS TO IT  ESC CLOSE";
    drawTextCentered(canvas, UW / 2, bottom - 9, foot, HUD_DIM);
}

std::string Game::testAlmanacInfo() const {
    std::string s = fmt("%s; %s; %zu events", almanacAt.ground ? "ground" : "ship", almanacStateLine().c_str(), almanacEvents.size());
    for (size_t i = 0; i < almanacEvents.size() && i < 12; i++) s += fmt("%s %s in %s", i ? "," : ":", almanacLabelOf(almanacEvents[i]).c_str(), countdownString(almanacEvents[i].t - t).c_str());
    if (almanacEvents.size() > 12) s += fmt(", +%zu more", almanacEvents.size() - 12);
    return s;
}

bool Game::testAlmanacSelect(int kind, int body) {
    for (size_t i = 0; i < almanacEvents.size(); i++)
        if ((kind < 0 || almanacEvents[i].kind == kind) && (body < 0 || almanacEvents[i].body == body)) { almanacSel = (int)i; return true; }
    return false;
}
