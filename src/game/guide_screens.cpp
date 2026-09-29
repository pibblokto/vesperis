// GUIDE screens (M3): the guide menu, text entry, star map, expedition log, gallery,
// statistics, and the actions behind them (rename, notes, targets by name/coordinates,
// history, home, export/import).
#include "game.h"
#include "ui.h"
#include "core/png.h"
#include "core/rng.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <sstream>

namespace {
const char* GUIDE_ITEMS[] = {"STAR MAP", "NAME THIS STAR", "NAME THIS WORLD", "NAME THE NEAREST LANDMARK", "WRITE A NOTE", "EXPEDITION LOG", "GALLERY", "STATISTICS",
                             "TARGET A STAR BY NAME", "TARGET BY COORDINATES", "RETURN TO THE PREVIOUS STAR", "TARGET THE HOME STAR",
                             "SET HOME HERE", "EXPORT THE GUIDE", "IMPORT AN INBOX FILE", "BACK"};
const int GUIDE_N = (int)(sizeof(GUIDE_ITEMS) / sizeof(GUIDE_ITEMS[0]));
uint32_t classColour(int cls) {
    const RGB& c = STAR_CLASSES[cls].color;
    return rgb((int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255));
}
}

// ---------------------------------------------------------------------------
// names
// ---------------------------------------------------------------------------

std::string Game::starKeyOf(const Star& s) const { return Guide::starKey(s.sx, s.sy, s.sz); }

// R-401: as in the original, every star, world, belt and landmark is UNKNOWN until an explorer names it (or a friend's
// inbox does). The generated names stay inside the generator (keys, landmark ids, the harness) and never reach the screen.
static const char* UNNAMED_LABEL = "UNKNOWN";

bool Game::starNamed(const Star& s) const { std::string key = starKeyOf(s); return guide.names.count(key) || guide.inbox.count(key); }

std::string Game::starNameOf(const Star& s) const {
    std::string key = starKeyOf(s);
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return it->second;
    it = guide.inbox.find(key);
    if (it != guide.inbox.end()) return it->second;
    return UNNAMED_LABEL;
}

bool Game::bodyNamed(int bi) const {
    if (!sys.valid || bi < 0 || bi >= (int)sys.bodies.size()) return false;
    std::string key = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, bi);
    return guide.names.count(key) || guide.inbox.count(key);
}

std::string Game::bodyNameOf(int bi) const {
    if (!sys.valid || bi < 0 || bi >= (int)sys.bodies.size()) return "?";
    std::string key = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, bi);
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return it->second;
    it = guide.inbox.find(key);
    if (it != guide.inbox.end()) return it->second;
    return UNNAMED_LABEL;
}

std::string Game::beltNameOf(int k) const { (void)k; return UNNAMED_LABEL; }   // belts cannot be named yet

// the local target's line: the display name and what it is
std::string Game::bodyLabelOf(int i) const {
    if (!sys.valid || i < 0 || i >= (int)sys.bodies.size()) return "";
    const Body& b = sys.bodies[i];
    std::string n = bodyNameOf(i);
    if (b.type == PT_COMPANION) return n + " (COMPANION STAR, " + STAR_CLASSES[b.starClass].name + ")";
    if (b.doublePlanet && b.parent >= 0) return n + " (" + PLANET_TYPES[b.type].name + ", twin planet)";
    return n + " (" + PLANET_TYPES[b.type].name + (b.parent >= 0 ? (sys.bodies[b.parent].type == PT_COMPANION ? ", of the companion)" : ", moon)") : ")");
}

bool Game::nameIsForeign(const std::string& key) const { return !guide.names.count(key) && guide.inbox.count(key); }

std::string Game::screenshotCaption() const {
    std::string where;
    if (surf.valid && (state == GameState::SURFACE || returnState == GameState::SURFACE || state == GameState::DESCENT || state == GameState::ASCENT))
        where = fmt("%s  %.1f%s %.1f%s  %s", upper(bodyNameOf(surf.site.body)).c_str(), std::fabs(surf.env.latDeg), surf.env.latDeg >= 0 ? "N" : "S",
                    std::fabs(surf.env.lonDeg), surf.env.lonDeg >= 0 ? "E" : "W", PLANET_TYPES[sys.bodies[surf.site.body].type].name);
    else if (sys.valid) where = ship.mode == ShipState::PARKED && ship.parkedBody >= 0 ? fmt("IN ORBIT OF %s", upper(bodyNameOf(ship.parkedBody)).c_str()) : fmt("SYSTEM %s (%s)", upper(starNameOf(sys.star)).c_str(), STAR_CLASSES[sys.star.cls].code);
    else where = "INTERSTELLAR SPACE";
    return where + "  " + epocString();
}

std::string Game::landmarkKey(const Landmark& L) const {
    return Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.valid ? surf.site.body : ship.localTarget) + "/L" + std::to_string((unsigned long long)(L.id & 0xffffffffULL));
}
std::string Game::landmarkName(const Landmark& L) const {   // the explorer's name, else empty (R-401: the generated one is never shown)
    auto it = guide.names.find(landmarkKey(L));
    return it != guide.names.end() ? it->second : std::string();
}
std::string Game::landmarkLabel(const Landmark& L) const {   // "MY PEAK - PEAK", or "UNNAMED PEAK"
    std::string n = landmarkName(L);
    return n.empty() ? std::string("UNNAMED ") + LANDMARK_KIND_NAMES[L.kind] : n + " - " + LANDMARK_KIND_NAMES[L.kind];
}
// O6-06: the first sight of a landmark within a kilometre of the explorer (or the buggy) is written into the log with its
// generated name; the guide keeps the set, so a return finds it known
void Game::noteLandmarks() {
    if (!surf.valid) return;
    double px = surf.inBuggy ? surf.buggy.x : surf.player.x, pz = surf.inBuggy ? surf.buggy.z : surf.player.z;
    for (const SurfaceView::SiteLandmark& L : surf.landmarks) {
        double d = std::sqrt((L.x - px) * (L.x - px) + (L.z - pz) * (L.z - pz));
        if (d > 1000.0 + L.lm.radiusM) continue;
        std::string key = landmarkKey(L.lm);
        if (guide.landmarksSeen.count(key)) continue;
        guide.landmarksSeen.insert(key);
        std::string what;
        switch (L.lm.kind) {
            case LM_PEAK: what = fmt("A PEAK OF %.0f M, %.0f M OVER ITS COUNTRY", L.lm.heightM, L.lm.prominenceM); break;
            case LM_MESA: what = fmt("A MESA %.1f KM ACROSS, %.0f M HIGH", 2 * L.lm.radiusM / 1000.0, L.lm.prominenceM); break;
            case LM_CANYON: what = fmt("THE RIM OF A CANYON %.0f M DEEP", L.lm.prominenceM); break;
            case LM_CRATER: what = fmt("A CRATER %.1f KM ACROSS, %.0f M DEEP", 2 * L.lm.radiusM / 1000.0, L.lm.prominenceM); break;
            case LM_GEYSERS: what = fmt("A GEYSER FIELD %.1f KM ACROSS", 2 * L.lm.radiusM / 1000.0); break;
            case LM_LAKE: what = fmt("A LAKE %.1f KM ACROSS AT %.0f M", 2 * L.lm.radiusM / 1000.0, L.lm.heightM); break;
            case LM_RUIN: what = fmt("RUINS OF THE OLD ONES, %.0f M ACROSS", L.lm.prominenceM); break;
            default: what = "A FIELD OF CRYSTAL SPIRES"; break;
        }
        double edge = std::max(0.0, d - L.lm.radiusM);
        std::string given = landmarkName(L.lm);
        logEvent("LANDMARK", fmt("%s%s, %.0f M %s ON %s", given.empty() ? "" : (given + ", ").c_str(), what.c_str(), L.lm.radiusM > 300 ? edge : d, L.lm.radiusM > 300 ? "FROM ITS EDGE" : "AWAY", upper(bodyNameOf(surf.site.body)).c_str()));
        status(fmt("LANDMARK: %s", landmarkLabel(L.lm).c_str()), 5);
        audio.beep = 4;
    }
}

void Game::logEvent(const std::string& kind, const std::string& text) {
    guide.addLog(t, kind, epocString() + "  " + text);
    guide.save(guidePath);
}

void Game::noteVisit() {
    if (!sys.valid) return;
    std::string key = starKeyOf(sys.star);
    guide.visited.insert(key);
    guide.classesSeen.insert(sys.star.cls);
    if (guide.home.empty()) guide.home = key;
    if (!guide.home.empty()) {
        int64_t hx, hy, hz;
        if (Guide::parseStarKey(guide.home, hx, hy, hz)) {
            Star h; if (starInSector(hx, hy, hz, h)) guide.furthestFromHomeLY = std::max(guide.furthestFromHomeLY, length(h.pos - sys.star.pos) / SECTOR_KM);
        }
    }
}

// ---------------------------------------------------------------------------
// guide menu
// ---------------------------------------------------------------------------

void Game::openGuide() {
    guideSel = 0;
    guideReturn = state == GameState::SURFACE ? GameState::SURFACE : GameState::SPACE;
    if (state == GameState::SURFACE || state == GameState::SPACE) returnState = state;
    state = GameState::GUIDE;
}

void Game::beginTextEntry(const std::string& prompt, int kind, const std::string& initial) {
    textPrompt = prompt; textKind = kind; textBuffer = initial;
    state = GameState::TEXT_ENTRY;
}

void Game::updateGuideMenu(const Input& in) {
    if (in.wasPressed(KEY_UP)) guideSel = (guideSel + GUIDE_N - 1) % GUIDE_N;
    if (in.wasPressed(KEY_DOWN)) guideSel = (guideSel + 1) % GUIDE_N;
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_G)) { state = guideReturn; return; }
    if (!enterKey(in)) return;
    bool onSurface = guideReturn == GameState::SURFACE;
    switch (guideSel) {
        case 0: if (!onSurface) openStarMap(); else status("THE STAR MAP IS READ FROM THE SHIP", 3); break;
        case 1: if (sys.valid) beginTextEntry("NAME THIS STAR", 1, starNamed(sys.star) ? upper(starNameOf(sys.star)) : std::string()); else status("NO STAR HERE", 3); break;
        case 2: {
            int bi = onSurface && surf.valid ? surf.site.body : ship.localTarget;
            if (sys.valid && bi >= 0 && bi < (int)sys.bodies.size()) { textBody = bi; beginTextEntry("NAME THIS WORLD", 2, bodyNamed(bi) ? upper(bodyNameOf(bi)) : std::string()); }
            else status("NO LOCAL TARGET TO NAME", 3);
            break;
        }
        case 3: {   // O6-06: the nearest landmark within 12 km, once seen (logged) or standing on it
            const SurfaceView::SiteLandmark* L = onSurface && surf.valid ? surf.nearestLandmark(surf.player.x, surf.player.z, 12000.0) : nullptr;
            if (!L) { status(onSurface ? "NO LANDMARK WITHIN 12 KM" : "LANDMARKS ARE NAMED ON THE GROUND", 3); break; }
            beginTextEntry(fmt("NAME THE %s", LANDMARK_KIND_NAMES[L->lm.kind]), 6, landmarkName(L->lm));
            break;
        }
        case 4: beginTextEntry("NOTE FOR THE LOG", 3, ""); break;
        case 5: logPage = 0; state = GameState::LOG; break;
        case 6: openGallery(); break;
        case 7: state = GameState::STATS; break;
        case 8: if (!onSurface) beginTextEntry("STAR NAME", 4, ""); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 9: if (!onSurface) beginTextEntry("SECTOR X Y Z", 5, ""); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 10: if (!onSurface) targetPreviousStar(); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 11: if (!onSurface) targetHome(); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 12: if (sys.valid) { guide.home = starKeyOf(sys.star); guide.save(guidePath); status(fmt("HOME STAR: %s", upper(starNameOf(sys.star)).c_str()), 4); state = guideReturn; } break;
        case 13: { std::string p = guidePath.substr(0, guidePath.rfind('.')) + "_export.txt"; status(guide.save(p) ? fmt("GUIDE EXPORTED TO %s", upper(p).c_str()) : "EXPORT FAILED", 5); state = guideReturn; break; }
        case 14: { std::string p = guidePath.substr(0, guidePath.rfind('.')) + "_inbox.txt"; int n = guide.importInbox(p); if (n < 0) status(fmt("NO INBOX FILE (%s)", upper(p).c_str()), 5); else { guide.save(guidePath); status(fmt("%d NEW NAMES FROM THE INBOX", n), 5); } state = guideReturn; break; }
        default: state = guideReturn; break;
    }
}

void Game::renderGuideMenu() {
    const int top = 20, bottom = UH - 18, y0 = top + 20;
    blendRectRGB(canvas, 40, top, UW - 40, bottom, rgb(0, 0, 0), 215);
    drawRectRGB(canvas, 40, top, UW - 40, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 7, "THE GUIDE", HUD_AMBER);
    for (int i = 0; i < GUIDE_N; i++) {
        int y = y0 + i * 9;
        bool sel = i == guideSel;
        std::string label = GUIDE_ITEMS[i];
        if (i == 2 && sys.valid) { int bi = guideReturn == GameState::SURFACE && surf.valid ? surf.site.body : ship.localTarget; if (bi >= 0 && bi < (int)sys.bodies.size() && bodyNamed(bi)) label = "RENAME " + trunc(upper(bodyNameOf(bi)), 22); }
        if (i == 3 && guideReturn == GameState::SURFACE && surf.valid) { const SurfaceView::SiteLandmark* L = surf.nearestLandmark(surf.player.x, surf.player.z, 12000.0); if (L) label = landmarkName(L->lm).empty() ? std::string("NAME THE ") + LANDMARK_KIND_NAMES[L->lm.kind] : "RENAME " + trunc(landmarkName(L->lm), 22); }
        if (i == 1 && sys.valid && starNamed(sys.star)) label = "RENAME " + trunc(upper(starNameOf(sys.star)), 22);
        drawText(canvas, 56, y, label.c_str(), sel ? HUD_WHITE : HUD_GREEN);
        if (sel) drawText(canvas, 46, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 10, "UP/DOWN  ENTER  ESC", HUD_DIM);
}

// ---------------------------------------------------------------------------
// text entry
// ---------------------------------------------------------------------------

void Game::updateTextEntry(const Input& in) {
    for (int k = KEY_A; k <= KEY_Z; k++) if (in.wasPressed(k) && textBuffer.size() < 28) textBuffer += (char)('A' + (k - KEY_A));
    for (int k = KEY_0; k <= KEY_9; k++) if (in.wasPressed(k) && textBuffer.size() < 28) textBuffer += (char)('0' + (k - KEY_0));
    if (in.wasPressed(KEY_SPACE) && textBuffer.size() < 28 && !textBuffer.empty()) textBuffer += ' ';
    if (in.wasPressed(KEY_MINUS) && textBuffer.size() < 28) textBuffer += '-';
    if (in.wasPressed(KEY_PERIOD) && textBuffer.size() < 28) textBuffer += '.';
    if (in.wasPressed(KEY_BACKSPACE) && !textBuffer.empty()) textBuffer.pop_back();
    if (in.wasPressed(KEY_ESCAPE)) { state = guideReturn; return; }
    if (!in.wasPressed(KEY_ENTER)) return;
    std::string text = textBuffer;
    while (!text.empty() && text.back() == ' ') text.pop_back();
    state = guideReturn;
    switch (textKind) {
        case 1:
            if (text.empty() || !sys.valid) return;
            guide.names[starKeyOf(sys.star)] = text; guide.save(guidePath);
            status(fmt("THE STAR IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", "NAMED THE STAR " + text);
            break;
        case 2:
            if (text.empty() || !sys.valid) return;
            guide.names[Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, textBody)] = text; guide.save(guidePath);
            status(fmt("THE WORLD IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", "NAMED A WORLD " + text);
            break;
        case 3:
            if (text.empty()) return;
            logEvent("NOTE", text);
            status("NOTE WRITTEN IN THE LOG", 3);
            break;
        case 4: targetStarByName(text); break;
        case 6: {   // O6-06: the landmark's name (the nearest, as offered by the menu)
            if (text.empty() || !surf.valid) return;
            const SurfaceView::SiteLandmark* L = surf.nearestLandmark(surf.player.x, surf.player.z, 12000.0);
            if (!L) return;
            guide.names[landmarkKey(L->lm)] = text; guide.landmarksSeen.insert(landmarkKey(L->lm)); guide.save(guidePath);
            status(fmt("THE %s IS NOW CALLED %s", LANDMARK_KIND_NAMES[L->lm.kind], text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED THE %s %s", LANDMARK_KIND_NAMES[L->lm.kind], text.c_str()));
            break;
        }
        case 5: {
            std::istringstream is(text);
            long long x, y, z;
            if (!(is >> x >> y >> z)) { status("USE THREE NUMBERS: X Y Z", 4); audio.beep = 3; return; }
            Star s;
            if (!starInSector(x, y, z, s, true)) { status(fmt("NO STAR IN SECTOR %lld %lld %lld", x, y, z), 4); audio.beep = 3; return; }
            setRemoteStar(s);
            break;
        }
    }
}

void Game::renderTextEntry() {
    const int top = 70, bottom = 130;
    blendRectRGB(canvas, 30, top, UW - 30, bottom, rgb(0, 0, 0), 225);
    drawRectRGB(canvas, 30, top, UW - 30, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 8, textPrompt.c_str(), HUD_AMBER);
    std::string shown = textBuffer + (std::fmod(realTime, 0.8) < 0.4 ? "_" : " ");
    drawTextCentered(canvas, UW / 2, top + 26, shown.c_str(), HUD_WHITE);
    drawTextCentered(canvas, UW / 2, bottom - 12, "LETTERS, DIGITS, SPACE, -   ENTER OK   ESC CANCEL", HUD_DIM);
}

// ---------------------------------------------------------------------------
// targets by name, coordinates, history, home
// ---------------------------------------------------------------------------

void Game::setRemoteStar(const Star& s) {
    if (sys.valid && s.seed == sys.star.seed) { status("THAT IS THE CURRENT STAR", 3); audio.beep = 3; return; }
    ship.remote = s;
    if (ship.remote.name.empty()) starInSector(s.sx, s.sy, s.sz, ship.remote, true);
    ship.hasRemote = true;
    ship.targeting = false;
    double ly = length(ship.remote.pos - ship.pos) / SECTOR_KM;
    status(fmt("REMOTE TARGET: %s (%s) %.2f LY - V TO FLY", upper(starNameOf(ship.remote)).c_str(), STAR_CLASSES[ship.remote.cls].code, ly), 6);
    audio.beep = 1;
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
        Star s; if (starInSector(sx, sy, sz, s, true)) { setRemoteStar(s); return; }
    }
    const Star* best = nullptr; double bestD = 1e300;
    for (const Star& s : nb.stars) {
        std::string n = upper(starNameOf(s));
        if (n.rfind(want, 0) != 0) continue;
        double d = length2(s.pos - ship.pos);
        if (d < bestD) { bestD = d; best = &s; }
    }
    if (!best) { status(fmt("NO STAR CALLED %s WITHIN %d LY", want.c_str(), 10), 4); audio.beep = 3; return; }
    Star full; starInSector(best->sx, best->sy, best->sz, full, true);
    setRemoteStar(full);
}

void Game::targetPreviousStar() {
    for (int i = (int)guide.history.size() - 1; i >= 0; i--) {
        int64_t sx, sy, sz;
        if (!Guide::parseStarKey(guide.history[i], sx, sy, sz)) continue;
        if (sys.valid && sx == sys.star.sx && sy == sys.star.sy && sz == sys.star.sz) continue;
        Star s; if (starInSector(sx, sy, sz, s, true)) { setRemoteStar(s); state = guideReturn; return; }
    }
    status("NO PREVIOUS STAR IN THE HISTORY", 3); audio.beep = 3;
}

void Game::targetHome() {
    int64_t sx, sy, sz;
    if (guide.home.empty() || !Guide::parseStarKey(guide.home, sx, sy, sz)) { status("NO HOME STAR SET", 3); audio.beep = 3; return; }
    Star s;
    if (!starInSector(sx, sy, sz, s, true)) { status("THE HOME SECTOR HAS NO STAR", 3); return; }
    setRemoteStar(s);
    state = guideReturn;
}

// ---------------------------------------------------------------------------
// star map
// ---------------------------------------------------------------------------

void Game::openStarMap() {
    mapYaw = 0.6; mapPitch = 0.5; mapZoom = 1.0; mapClassMask = 63;
    state = GameState::STAR_MAP;
}

void Game::updateStarMap(const Input& in) {
    mapYaw += in.mouseDx * 0.004 * settings.mouseSensitivity;
    mapPitch += in.mouseDy * 0.004 * settings.mouseSensitivity;
    if (in.isDown(KEY_LEFT)) mapYaw -= 1.5 * lastRealDt;
    if (in.isDown(KEY_RIGHT)) mapYaw += 1.5 * lastRealDt;
    if (in.isDown(KEY_UP)) mapPitch += 1.0 * lastRealDt;
    if (in.isDown(KEY_DOWN)) mapPitch -= 1.0 * lastRealDt;
    mapPitch = clampd(mapPitch, -1.4, 1.4);
    if (in.wheel > 0 || in.wasPressed(KEY_EQUAL)) mapZoom = std::min(4.0, mapZoom * 1.25);
    if (in.wheel < 0 || in.wasPressed(KEY_MINUS)) mapZoom = std::max(0.5, mapZoom / 1.25);
    for (int c = 0; c < STAR_CLASS_COUNT; c++) if (in.wasPressed(KEY_1 + c)) mapClassMask ^= (1 << c);
    if (in.wasPressed(KEY_0)) mapClassMask = 63;
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_M)) { state = GameState::SPACE; return; }
    if ((enterKey(in) || in.mousePressed[0]) && mapPick >= 0 && mapPick < (int)nb.stars.size()) {
        Star full; starInSector(nb.stars[mapPick].sx, nb.stars[mapPick].sy, nb.stars[mapPick].sz, full, true);
        setRemoteStar(full);
        state = GameState::SPACE;
    }
}

void Game::renderStarMap() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 215);
    Mat3 cam = cameraBasis(mapYaw, mapPitch);
    double pxPerLy = 14.0 * mapZoom;
    int cx = UW / 2, cy = UH / 2 + 4;
    auto project = [&](const Vec3& posKm, int& sx, int& sy, double& depth) {
        Vec3 rel = (posKm - ship.pos) / SECTOR_KM;
        Vec3 v = cam * rel;
        sx = cx + (int)std::lround(v.x * pxPerLy); sy = cy - (int)std::lround(v.y * pxPerLy); depth = v.z;
    };
    // the galactic plane through the ship: rings at 2, 5 and 10 LY, and the six directions
    for (double r : {2.0, 5.0, 10.0}) {
        int px = 0, py = 0; bool have = false;
        for (int i = 0; i <= 48; i++) {
            double a = i * TAU / 48;
            int sx, sy; double d;
            project(ship.pos + Vec3(std::cos(a), 0, std::sin(a)) * (r * SECTOR_KM), sx, sy, d);
            if (have) drawLineRGB(canvas, px, py, sx, sy, rgb(30, 55, 40));
            px = sx; py = sy; have = true;
        }
    }
    // stars, far to near
    std::vector<std::pair<double, int>> order;
    for (int i = 0; i < (int)nb.stars.size(); i++) {
        if (!((mapClassMask >> nb.stars[i].cls) & 1)) continue;
        int sx, sy; double d;
        project(nb.stars[i].pos, sx, sy, d);
        order.push_back({d, i});
    }
    std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    mapPick = -1; double pickD = 1e9;
    int64_t hx = 0, hy = 0, hz = 0; bool haveHome = !guide.home.empty() && Guide::parseStarKey(guide.home, hx, hy, hz);
    for (auto& pr : order) {
        const Star& s = nb.stars[pr.second];
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
        if (haveHome && s.sx == hx && s.sy == hy && s.sz == hz) { drawLineRGB(canvas, sx - 5, sy, sx, sy - 5, HUD_AMBER); drawLineRGB(canvas, sx, sy - 5, sx + 5, sy, HUD_AMBER); drawLineRGB(canvas, sx + 5, sy, sx, sy + 5, HUD_AMBER); drawLineRGB(canvas, sx, sy + 5, sx - 5, sy, HUD_AMBER); }
        if (guide.names.count(key) || guide.inbox.count(key)) drawText(canvas, sx + 5, sy - 3, trunc(upper(starNameOf(s)), 12).c_str(), nameIsForeign(key) ? HUD_CYAN : HUD_GREEN);
        double dd = std::sqrt((double)(sx - cx) * (sx - cx) + (double)(sy - cy) * (sy - cy));
        if (dd < pickD && !current) { pickD = dd; mapPick = pr.second; }
    }
    // crosshair and the picked star's card
    drawLineRGB(canvas, cx - 8, cy, cx - 3, cy, HUD_DIM); drawLineRGB(canvas, cx + 3, cy, cx + 8, cy, HUD_DIM);
    drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, HUD_DIM); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, HUD_DIM);
    drawText(canvas, 6, 4, fmt("STAR MAP  %d STARS WITHIN 10 LY  ZOOM %.1fX", (int)order.size(), mapZoom).c_str(), HUD_AMBER);
    std::string filt = "CLASSES:";
    for (int c = 0; c < STAR_CLASS_COUNT; c++) filt += std::string(" ") + ((mapClassMask >> c) & 1 ? STAR_CLASSES[c].code : "---");
    drawText(canvas, 6, 13, filt.c_str(), HUD_DIM);
    if (mapPick >= 0 && pickD < 40) {
        const Star& s = nb.stars[mapPick];
        Star full; starInSector(s.sx, s.sy, s.sz, full, true);
        std::string key = Guide::starKey(s.sx, s.sy, s.sz);
        double ly = length(full.pos - ship.pos) / SECTOR_KM;
        std::string card = fmt("%s  %s %s  %.2f LY%s", upper(starNameOf(full)).c_str(), STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, ly, guide.visited.count(key) ? "  VISITED" : "");
        drawTextCentered(canvas, UW / 2, UH - 24, card.c_str(), nameIsForeign(key) ? HUD_CYAN : HUD_WHITE);
    }
    drawTextCentered(canvas, UW / 2, UH - 12, "MOUSE/ARROWS TURN  WHEEL/+/- ZOOM  1-6 CLASSES  ENTER TARGET  ESC", HUD_DIM);
}

// ---------------------------------------------------------------------------
// log, statistics, gallery
// ---------------------------------------------------------------------------

void Game::updateLog(const Input& in) {
    int pages = std::max(1, ((int)guide.log.size() + 6) / 7);
    if (in.wasPressed(KEY_DOWN) || in.wasPressed(KEY_PAGE_DOWN) || in.wasPressed(KEY_RIGHT)) logPage = std::min(pages - 1, logPage + 1);
    if (in.wasPressed(KEY_UP) || in.wasPressed(KEY_PAGE_UP) || in.wasPressed(KEY_LEFT)) logPage = std::max(0, logPage - 1);
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_J) || enterKey(in)) state = guideReturn;
}

void Game::renderLog() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 215);
    int n = (int)guide.log.size();
    int pages = std::max(1, (n + 6) / 7);
    drawText(canvas, 6, 4, fmt("EXPEDITION LOG  %d ENTRIES  PAGE %d/%d", n, logPage + 1, pages).c_str(), HUD_AMBER);
    int y = 16;
    for (int i = 0; i < 7; i++) {
        int idx = n - 1 - (logPage * 7 + i);   // newest first
        if (idx < 0) break;
        const LogEntry& e = guide.log[idx];
        uint32_t col = e.kind == "NOTE" ? HUD_WHITE : (e.kind == "LANDING" ? HUD_AMBER : HUD_GREEN);
        // kind and EPOC on the first line, the text wrapped on up to two more
        std::string text = e.text, when;
        size_t sp = text.find("  ");
        if (sp != std::string::npos) { when = text.substr(0, sp); text = text.substr(sp + 2); }
        drawText(canvas, 6, y, fmt("%-8s %s", e.kind.c_str(), when.c_str()).c_str(), col);
        std::vector<std::string> lines = wrapText(text, 44);
        for (size_t k = 0; k < lines.size() && k < 2; k++) { drawText(canvas, 40, y + 8 + (int)k * 8, lines[k].c_str(), HUD_DIM); }
        y += 8 + 8 * (int)std::min<size_t>(2, std::max<size_t>(1, lines.size())) + 3;
    }
    if (n == 0) drawTextCentered(canvas, UW / 2, 90, "NOTHING WRITTEN YET", HUD_DIM);
    drawTextCentered(canvas, UW / 2, UH - 10, "UP/DOWN PAGE   ESC CLOSE", HUD_DIM);
}

void Game::renderStats() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 215);
    int y = 8;
    auto line = [&](const std::string& s, uint32_t c) { drawText(canvas, 12, y, s.c_str(), c); y += 10; };
    line("DISCOVERY STATISTICS", HUD_AMBER); y += 4;
    line(fmt("SYSTEMS VISITED       %d", (int)guide.visited.size()), HUD_GREEN);
    line(fmt("WORLDS LANDED ON      %d", (int)guide.landed.size()), HUD_GREEN);
    std::string cls; for (int c : guide.classesSeen) cls += std::string(STAR_CLASSES[c].code) + " ";
    line(fmt("STAR CLASSES SEEN     %d/%d  %s", (int)guide.classesSeen.size(), STAR_CLASS_COUNT, cls.c_str()), HUD_GREEN);
    std::string types; for (int c : guide.typesSeen) types += std::string(shortType(c)) + " ";
    line(fmt("WORLD TYPES WALKED    %d/%d", (int)guide.typesSeen.size(), landableTypeCount()), HUD_GREEN);
    line("  " + trunc(types, 48), HUD_DIM);
    line(fmt("FURTHEST FROM HOME    %.2f LY", guide.furthestFromHomeLY), HUD_GREEN);
    line(fmt("LONGEST WALK          %.2f KM FROM THE CAPSULE", guide.longestWalkM / 1000.0), HUD_GREEN);
    line(fmt("HIGHEST POINT REACHED %.0f M", guide.highestPointM), HUD_GREEN);
    line(fmt("DISTANCE ON FOOT      %.1f KM", guide.totalWalkedM / 1000.0), HUD_GREEN);
    line(fmt("DISTANCE DRIVEN       %.1f KM", guide.totalDrivenM / 1000.0), HUD_GREEN);
    line(fmt("NAMES GIVEN           %d   FROM OTHERS %d", (int)guide.names.size(), (int)guide.inbox.size()), HUD_GREEN);
    line(fmt("LOG ENTRIES           %d   PHOTOGRAPHS %d", (int)guide.log.size(), guide.screenshots), HUD_GREEN);
    line(fmt("CREATURES SIGHTED     %d SPECIES", guide.creaturesSeen), HUD_GREEN);   // N3-01
    line(fmt("LONGEST DRIVE         %.2f KM   TOP SPEED %.0f KM/H", guide.longestDriveM / 1000.0, guide.topSpeedKmh), HUD_GREEN);   // N4-05
    int64_t hx, hy, hz;
    if (!guide.home.empty() && Guide::parseStarKey(guide.home, hx, hy, hz)) { Star h; if (starInSector(hx, hy, hz, h, true)) line(fmt("HOME STAR             %s (%s)", upper(starNameOf(h)).c_str(), STAR_CLASSES[h.cls].code), HUD_CYAN); }
    drawTextCentered(canvas, UW / 2, UH - 10, "ANY KEY TO CLOSE", HUD_DIM);
}

void Game::openGallery() {
    galleryFiles.clear();
    std::string dir = shotsDir;
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* ent;
        while ((ent = readdir(d)) != nullptr) {
            std::string n = ent->d_name;
            if (n.rfind("screenshot_", 0) == 0 && n.size() > 4 && n.substr(n.size() - 4) == ".png") galleryFiles.push_back(dir + "/" + n);
        }
        closedir(d);
    }
    std::sort(galleryFiles.begin(), galleryFiles.end(), [](const std::string& a, const std::string& b) {
        auto num = [](const std::string& s) { size_t p = s.rfind("screenshot_"); return atoi(s.c_str() + p + 11); };
        return num(a) < num(b);
    });
    gallerySel = (int)galleryFiles.size() - 1;
    galleryLoaded = -1;
    state = GameState::GALLERY;
}

void Game::updateGallery(const Input& in) {
    int n = (int)galleryFiles.size();
    if (n > 0) {
        if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_UP)) gallerySel = (gallerySel + n - 1) % n;
        if (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_DOWN)) gallerySel = (gallerySel + 1) % n;
    }
    if (in.wasPressed(KEY_ESCAPE) || enterKey(in)) state = guideReturn;
}

void Game::renderGallery() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 235);
    int n = (int)galleryFiles.size();
    if (n == 0) { drawTextCentered(canvas, UW / 2, 90, "NO PHOTOGRAPHS YET - P TAKES ONE", HUD_DIM); drawTextCentered(canvas, UW / 2, UH - 10, "ESC CLOSE", HUD_DIM); return; }
    if (galleryLoaded != gallerySel) {
        galleryLoaded = gallerySel;
        galleryOk = readPNG(galleryFiles[gallerySel].c_str(), galleryPix, galleryW, galleryH);
        galleryCaption.clear();
        std::string side = galleryFiles[gallerySel].substr(0, galleryFiles[gallerySel].size() - 4) + ".txt";
        std::ifstream f(side);
        if (f) std::getline(f, galleryCaption);
    }
    // the picture scaled into a 256x160 logical frame
    const int fx = 32, fy = 12, fw = 256, fh = 160;
    int S = FB_SCALE;
    if (galleryOk && galleryW > 0) {
        for (int y = 0; y < fh * S; y++)
            for (int x = 0; x < fw * S; x++) {
                int sx = (int)((double)x / (fw * S) * galleryW), sy = (int)((double)y / (fh * S) * galleryH);
                canvas.px[(size_t)(fy * S + y) * FBW + fx * S + x] = galleryPix[(size_t)sy * galleryW + sx];
            }
    } else drawTextCentered(canvas, UW / 2, 90, "CANNOT READ THIS FILE", HUD_RED);
    drawRectRGB(canvas, fx - 1, fy - 1, fx + fw, fy + fh, HUD_DIM);
    std::string name = galleryFiles[gallerySel].substr(galleryFiles[gallerySel].rfind('/') + 1);
    drawText(canvas, fx, fy + fh + 4, fmt("%d/%d  %s", gallerySel + 1, n, upper(name).c_str()).c_str(), HUD_AMBER);
    drawText(canvas, fx, fy + fh + 13, trunc(galleryCaption, 42).c_str(), HUD_GREEN);
    drawTextCentered(canvas, UW / 2, UH - 8, "LEFT/RIGHT BROWSE   ESC CLOSE", HUD_DIM);
}
