// The star map (STAR_MAP, M3), and a star targeted by name, by its coordinates, from the history or home
#include "game.h"
#include "peoples/works.h"
#include "ui.h"
#include "peoples/buildings.h"   // a shard's place
#include "core/png.h"
#include "core/fs.h"
#include "core/rng.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

namespace {
uint32_t classColour(int cls) {
    const RGB& c = STAR_CLASSES[cls].color;
    return rgb((int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255));
}
}

// ---------------------------------------------------------------------------
// targets by name, coordinates, history, home
// ---------------------------------------------------------------------------

// the names given to a star's worlds (the star map's card)
// R-407: the names the explorer gave to a star's worlds (its key's bodies, not the sights, storms or pieces under them)
std::vector<std::string> Game::worldNamesAt(const std::string& starKey) const {
    std::vector<std::string> out;
    const std::string pre = starKey + "/";
    for (auto it = guide.names.lower_bound(pre); it != guide.names.end() && it->first.compare(0, pre.size(), pre) == 0; ++it)
        if (it->first.find('/', pre.size()) == std::string::npos) out.push_back(it->second);
    return out;
}

void Game::setRemoteStar(const Star& s) {
    if (sys.valid && s.seed == sys.star.seed) { refuse("THAT IS THE CURRENT STAR", 3); return; }
    jumpPlot.seed = 0;   // G-06: a target of any other kind is no plotted jump (`plotJump` sets it again after)
    ship.remote = s;
    if (ship.remote.name.empty()) placeInSector(s.sx, s.sy, s.sz, ship.remote, true);
    ship.hasRemote = true;
    ship.targeting = false;
    double ly = length(ship.remote.pos - ship.pos) / SECTOR_KM;
    if (ship.remote.cls == STAR_ROGUE) status(fmt("REMOTE TARGET: %s - NO STAR - %.2f LY - V TO FLY", upper(starNameOf(ship.remote)).c_str(), ly), 6);   // a drifter has no class to show
    else status(fmt("REMOTE TARGET: %s (%s) %.2f LY - V TO FLY", upper(starNameOf(ship.remote)).c_str(), STAR_CLASSES[ship.remote.cls].code, ly), 6);
    audio.beep = BEEP_LOCK;
}

void Game::targetStarByName(const std::string& name) {
    std::string want = upper(name);
    if (want.empty()) return;
    // your own names first (anywhere in the galaxy), then the generated names of the neighbourhood
    for (auto& kv : guide.names) {
        if (kv.first.find('/') != std::string::npos) continue;
        if (upper(kv.second).rfind(want, 0) != 0) continue;
        int64_t sx, sy, sz;
        if (!Guide::parseStarKey(kv.first, sx, sy, sz)) continue;
        Star s; if (placeInSector(sx, sy, sz, s, true)) { setRemoteStar(s); return; }   // D-01: a drifter named too
    }
    const Star* best = nullptr; double bestD = 1e300;
    for (const Star& s : nb.stars) {
        std::string n = upper(starNameOf(s));
        if (n.rfind(want, 0) != 0) continue;
        double d = length2(s.pos - ship.pos);
        if (d < bestD) { bestD = d; best = &s; }
    }
    if (!best) { refuse(fmt("NO STAR CALLED %s WITHIN %d LY", want.c_str(), 10)); return; }
    Star full; starInSector(best->sx, best->sy, best->sz, full, true);
    setRemoteStar(full);
}

void Game::targetPreviousStar() {
    for (int i = (int)guide.history.size() - 1; i >= 0; i--) {
        int64_t sx, sy, sz;
        if (!Guide::parseStarKey(guide.history[i], sx, sy, sz)) continue;
        if (sys.valid && sx == sys.star.sx && sy == sys.star.sy && sz == sys.star.sz) continue;
        Star s; if (placeInSector(sx, sy, sz, s, true)) { setRemoteStar(s); state = guideReturn; return; }   // D-01: a drifter visited too
    }
    refuse("NO PREVIOUS STAR IN THE HISTORY", 3);
}

void Game::targetHome() {
    int64_t sx, sy, sz;
    if (guide.home.empty() || !Guide::parseStarKey(guide.home, sx, sy, sz)) { refuse("NO HOME STAR SET", 3); return; }
    Star s;
    if (!placeInSector(sx, sy, sz, s, true)) { status("THE HOME SECTOR HAS NO STAR", 3); return; }
    setRemoteStar(s);
    state = guideReturn;
}

// ---------------------------------------------------------------------------
// star map
// ---------------------------------------------------------------------------

void Game::openStarMap() {
    mapYaw = 0.6; mapPitch = 0.5; mapZoom = 1.0; mapClassMask = (1 << STAR_DRAWN_COUNT) - 1;
    if (ship.mode == ShipState::VIMANA) mapPage = 1;   // G-05: a flight shows the galaxy (the neighbourhood is read at a star)
    state = GameState::STAR_MAP;
}

void Game::updateStarMap(const Input& in, double dt, double realDt) {
    if (in.wasPressed(KEY_TAB)) {   // G-05: the neighbourhood and the galaxy; G-06: on a plotted jump the stars round its end (the look before the leap)
        if (mapPage == 1 && ship.mode == ShipState::VIMANA) { refuse("IN FLIGHT THE MAP SHOWS THE GALAXY", 3); }
        else if (mapPage == 1 && chartPlotted()) {
            gchart.look = true; gchart.lookCentre = ship.remote.pos; gchart.lookNb.update(ship.remote.pos);
            mapPage = 0; mapZoom = 1.0; audio.beep = BEEP_UI;
        } else { gchart.look = false; mapPage = 1 - mapPage; audio.beep = BEEP_UI; }
        return;
    }
    if (mapPage == 1) { updateGalaxyChart(in); return; }
    mapYaw += in.mouseDx * 0.004 * settings.mouseSensitivity;
    mapPitch += in.mouseDy * 0.004 * settings.mouseSensitivity;
    if (in.isDown(KEY_LEFT)) mapYaw -= 1.5 * lastRealDt;
    if (in.isDown(KEY_RIGHT)) mapYaw += 1.5 * lastRealDt;
    if (in.isDown(KEY_UP)) mapPitch += 1.0 * lastRealDt;
    if (in.isDown(KEY_DOWN)) mapPitch -= 1.0 * lastRealDt;
    mapPitch = clampd(mapPitch, -1.4, 1.4);
    if (in.wheel > 0 || in.wasPressed(KEY_EQUAL)) mapZoom = std::min(4.0, mapZoom * 1.25);
    if (in.wheel < 0 || in.wasPressed(KEY_MINUS)) mapZoom = std::max(0.5, mapZoom / 1.25);
    // the class filter cycles through ALL and every class of the table (C forward, X back), whatever their number;
    // 0 shows all (keys 1-9 could not reach a tenth class)
    if (in.wasPressed(KEY_C) || in.wasPressed(KEY_X)) {
        int cur = -1;   // -1 all, else the one class shown
        for (int c = 0; c < STAR_DRAWN_COUNT; c++) if (mapClassMask == (1 << c)) cur = c;
        int n = STAR_DRAWN_COUNT + 1;
        cur = ((cur + 1 + (in.wasPressed(KEY_C) ? 1 : -1)) % n + n) % n - 1;
        mapClassMask = cur < 0 ? (1 << STAR_DRAWN_COUNT) - 1 : (1 << cur);
    }
    if (in.wasPressed(KEY_0)) mapClassMask = (1 << STAR_DRAWN_COUNT) - 1;
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_M)) {
        if (gchart.look) { gchart.look = false; mapPage = 1; }   // the look is the chart's: the map opens on the chart again
        state = GameState::SPACE; return;
    }
    const std::vector<Star>& stars = mapStars().stars;
    if ((enterKey(in) || in.mousePressed[0]) && mapPick >= 0 && mapPick < (int)stars.size()) {
        Star full; starInSector(stars[mapPick].sx, stars[mapPick].sy, stars[mapPick].sz, full, true);
        if (gchart.look) {   // another star of the plotted end taken, the line kept for the log; back on the chart, Enter flies
            const JumpPlot keep = jumpPlot;
            setRemoteStar(full);
            if (ship.hasRemote && ship.remote.seed == full.seed) { jumpPlot = keep; jumpPlot.seed = full.seed; status("PLOTTED - ENTER FLIES, ESC KEEPS IT FOR V", 4); }
            gchart.look = false; mapPage = 1;
            return;
        }
        setRemoteStar(full);
        state = GameState::SPACE;
    }
}

// U-04: the star map's controller, each page's: its commands, its buttons, what the sticks do (the galaxy chart's cursor; the
// neighbourhood turns)
void Game::starMapPad(PadMenu& m) const {
    if (mapPage == 1) {   // the galaxy chart
        m.title = "THE GALAXY";
        m.item(chartPlotted() ? "FLY THE PLOTTED JUMP" : "PLOT A JUMP TO THE CURSOR", KEY_ENTER);
        m.item("THE NEXT STOP", KEY_N); m.item("THE STOP BEFORE", KEY_N, MOD_SHIFT);
        m.item("MARK THE PLACE", KEY_L);
        m.item("REMOVE THE MARK", KEY_DELETE, 0, PSP_TWICE);
        m.item("THE CURSOR BACK TO THE SHIP", KEY_SPACE);
        m.item("ZOOM IN", KEY_EQUAL); m.item("ZOOM OUT", KEY_MINUS);
        m.item(chartPlotted() ? "LOOK AT THE PLOTTED END" : "THE NEIGHBOURHOOD", KEY_TAB);
        m.button(PR_A, KEY_ENTER, chartPlotted() ? "FLY" : "PLOT"); m.button(PR_B, KEY_ESCAPE, "CLOSE"); m.button(PR_X, KEY_SPACE, "THE SHIP");
        m.button(PR_LB, KEY_PAGE_UP, "HIGHER"); m.button(PR_RB, KEY_PAGE_DOWN, "LOWER");
        m.button(PR_LT, KEY_MINUS, "ZOOM OUT"); m.button(PR_RT, KEY_EQUAL, "ZOOM IN"); m.button(PR_L3, KEY_LEFT_SHIFT, "FASTER");
        m.button(PR_SELECT, KEY_TAB, chartPlotted() ? "LOOK THERE" : "NEIGHBOURHOOD"); m.button(PR_START, KEY_ESCAPE, "CLOSE");
        m.sticks = "STICKS MOVE THE CURSOR";
    } else {
        m.title = "STAR MAP";
        m.item(gchart.look ? "TAKE THIS STAR AS THE END" : "TARGET THE STAR", KEY_ENTER);
        m.item("THE NEXT CLASS", KEY_C); m.item("THE CLASS BEFORE", KEY_X); m.item("EVERY CLASS", KEY_0);
        m.item("ZOOM IN", KEY_EQUAL); m.item("ZOOM OUT", KEY_MINUS);
        m.item(gchart.look ? "BACK TO THE CHART" : "THE GALAXY", KEY_TAB);
        m.button(PR_A, KEY_ENTER, gchart.look ? "TAKE THIS STAR" : "TARGET"); m.button(PR_B, KEY_ESCAPE, "CLOSE");
        m.button(PR_LT, KEY_MINUS, "ZOOM OUT"); m.button(PR_RT, KEY_EQUAL, "ZOOM IN");
        m.button(PR_SELECT, KEY_TAB, gchart.look ? "THE CHART" : "THE GALAXY"); m.button(PR_START, KEY_ESCAPE, "CLOSE");
        m.sticks = "STICKS TURN IT";
    }
    m.item("CLOSE THE MAP", KEY_ESCAPE);
}

// the galaxy chart covers the frame: no scene under it
bool Game::starMapCovers() const { return mapPage == 1; }

void Game::renderStarMap() {
    if (mapPage == 1) { keepFlightBand(); renderGalaxyChart(); return; }
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 215);
    Mat3 cam = cameraBasis(mapYaw, mapPitch);
    double pxPerLy = 14.0 * mapZoom;
    int cx = UW / 2, cy = UH / 2 + 4;
    const Vec3 centre = mapCentre();   // G-06: the ship's place, or the plotted end's in the look before the leap
    const std::vector<Star>& stars = mapStars().stars;
    auto project = [&](const Vec3& posKm, int& sx, int& sy, double& depth) {
        Vec3 rel = (posKm - centre) / SECTOR_KM;
        Vec3 v = cam * rel;
        sx = cx + (int)std::lround(v.x * pxPerLy); sy = cy - (int)std::lround(v.y * pxPerLy); depth = v.z;
    };
    // the galactic plane through the ship: rings at 2, 5 and 10 LY, and the six directions
    for (double r : {2.0, 5.0, 10.0}) {
        int px = 0, py = 0; bool have = false;
        for (int i = 0; i <= 48; i++) {
            double a = i * TAU / 48;
            int sx, sy; double d;
            project(centre + Vec3(std::cos(a), 0, std::sin(a)) * (r * SECTOR_KM), sx, sy, d);
            if (have) drawLineRGB(canvas, px, py, sx, sy, rgb(30, 55, 40));
            px = sx; py = sy; have = true;
        }
    }
    // stars, far to near
    std::vector<std::pair<double, int>> order;
    for (int i = 0; i < (int)stars.size(); i++) {
        if (!((mapClassMask >> stars[i].cls) & 1)) continue;
        int sx, sy; double d;
        project(stars[i].pos, sx, sy, d);
        order.push_back({d, i});
    }
    std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    mapPick = -1; double pickD = 1e9;
    int64_t hx = 0, hy = 0, hz = 0; bool haveHome = !guide.home.empty() && Guide::parseStarKey(guide.home, hx, hy, hz);
    for (auto& pr : order) {
        const Star& s = stars[pr.second];
        int sx, sy; double d;
        project(s.pos, sx, sy, d);
        if (sx < 2 || sy < 12 || sx >= UW - 2 || sy >= UH - 14) continue;
        double depthF = clampd(0.55 + 0.45 * (-d / 10.0), 0.35, 1.0);   // nearer (toward the viewer) is brighter
        uint32_t col = classColour(s.cls);
        int r = (col & 255) * depthF, g = ((col >> 8) & 255) * depthF, b = ((col >> 16) & 255) * depthF;
        col = rgb(r, g, b);
        int size = s.luminosity > 20 ? 2 : (s.luminosity > 0.5 ? 1 : 0);
        fillRectRGB(canvas, sx - size, sy - size, sx + size, sy + size, col);
        std::string key = Guide::starKey(s.sx, s.sy, s.sz);
        bool current = sys.valid && s.seed == sys.star.seed;
        if (guide.visited.count(key) && !current) drawRectRGB(canvas, sx - 3, sy - 3, sx + 3, sy + 3, rgb(90, 150, 110));
        if (current) { drawLineRGB(canvas, sx - 6, sy, sx + 6, sy, HUD_WHITE); drawLineRGB(canvas, sx, sy - 6, sx, sy + 6, HUD_WHITE); }
        if (ship.hasRemote && s.seed == ship.remote.seed) drawRectRGB(canvas, sx - 5, sy - 5, sx + 5, sy + 5, HUD_CYAN);
        if (haveHome && s.sx == hx && s.sy == hy && s.sz == hz) {
            drawLineRGB(canvas, sx - 5, sy, sx, sy - 5, HUD_AMBER);
            drawLineRGB(canvas, sx, sy - 5, sx + 5, sy, HUD_AMBER);
            drawLineRGB(canvas, sx + 5, sy, sx, sy + 5, HUD_AMBER);
            drawLineRGB(canvas, sx, sy + 5, sx - 5, sy, HUD_AMBER);
        }
        if (guide.names.count(key) || guide.inbox.count(key)) drawText(canvas, sx + 5, sy - 3, trunc(upper(starNameOf(s)), 12).c_str(), nameIsForeign(key) ? HUD_CYAN : HUD_GREEN);
        else if (guide.visited.count(key) && !current) {   // an unnamed star the ship has been to, by a world named there
            std::vector<std::string> w = worldNamesAt(key);
            if (!w.empty()) drawText(canvas, sx + 5, sy - 3, ("(" + trunc(upper(w[0]), 10) + ")").c_str(), HUD_DIM);
        }
        double dd = std::sqrt((double)(sx - cx) * (sx - cx) + (double)(sy - cy) * (sy - cy));
        const bool centreStar = gchart.look && ship.hasRemote && s.seed == ship.remote.seed;   // the look's middle is the plotted star, as the ship's is the ship's
        if (dd < pickD && !current && !centreStar) { pickD = dd; mapPick = pr.second; }
    }
    // crosshair and the picked star's card
    drawLineRGB(canvas, cx - 8, cy, cx - 3, cy, HUD_DIM); drawLineRGB(canvas, cx + 3, cy, cx + 8, cy, HUD_DIM);
    drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, HUD_DIM); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, HUD_DIM);
    // the page's name (49 characters with four digits; G-06's look 50)
    drawText(canvas, 6, 4, fmt("%s  %d STARS IN 10 LY  ZOOM %.1fX", gchart.look ? "AT THE PLOTTED END" : "THE NEIGHBOURHOOD", (int)order.size(), mapZoom).c_str(), HUD_AMBER);
    int solo = -1;   // the filter shows all or one class (the legend of every code outgrew the line at ten classes)
    for (int c = 0; c < STAR_DRAWN_COUNT; c++) if (mapClassMask == (1 << c)) solo = c;
    std::string filt = solo < 0 ? "CLASSES: ALL" : fmt("CLASS: %s %s", STAR_CLASSES[solo].code, STAR_CLASSES[solo].name);
    drawText(canvas, 6, 13, filt.c_str(), solo < 0 ? HUD_DIM : HUD_AMBER);
    {   // (the user's "green marker stars as unknown"): what the marks mean
        const int hw = textWidth("HOME"), vw = textWidth("VISITED");
        const int hText = UW - 6 - hw, hx = hText - 6, vText = hx - 12 - vw, bx = vText - 10, y = 13;
        drawRectRGB(canvas, bx, y, bx + 6, y + 6, rgb(90, 150, 110)); drawText(canvas, vText, y, "VISITED", HUD_DIM);
        drawLineRGB(canvas, hx - 3, y + 3, hx, y, HUD_AMBER);
        drawLineRGB(canvas, hx, y, hx + 3, y + 3, HUD_AMBER);
        drawLineRGB(canvas, hx + 3, y + 3, hx, y + 6, HUD_AMBER);
        drawLineRGB(canvas, hx, y + 6, hx - 3, y + 3, HUD_AMBER);
        drawText(canvas, hText, y, "HOME", HUD_DIM);
    }
    if (mapPick >= 0 && pickD < 40) {
        const Star& s = stars[mapPick];
        Star full; starInSector(s.sx, s.sy, s.sz, full, true);
        std::string key = Guide::starKey(s.sx, s.sy, s.sz);
        double ly = length(full.pos - centre) / SECTOR_KM;   // in the look, from the plotted star
        std::string card = fmt("%s  %s %s  %.2f LY%s", upper(starNameOf(full)).c_str(), STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, ly, guide.visited.count(key) ? "  VISITED" : "");
        drawTextCentered(canvas, UW / 2, UH - 24, card.c_str(), nameIsForeign(key) ? HUD_CYAN : HUD_WHITE);
        std::vector<std::string> w = worldNamesAt(key);   // the worlds named there
        if (!w.empty()) {
            std::string names; for (const std::string& n : w) names += (names.empty() ? "" : ", ") + upper(n);
            drawTextCentered(canvas, UW / 2, UH - 33, trunc("YOUR WORLDS THERE: " + names, 52).c_str(), HUD_GREEN);
        }
    }
    // within the frame's 53 characters; G-05: the mouse's turn left to be found, TAB in; G-06: the status over it
    if (!drawMapStatus()) drawTextCentered(canvas, UW / 2, UH - 12, "WHEEL ZOOM  C/X/0 CLASS  ENTER TARGET  TAB GALAXY", HUD_DIM);
}
