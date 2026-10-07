// GUIDE screens (M3): the guide menu, text entry, star map, expedition log, gallery,
// statistics, and the actions behind them (rename, notes, targets by name/coordinates,
// history, home, export/import).
#include "game.h"
#include "ui.h"
#include "core/png.h"
#include "core/fs.h"
#include "core/rng.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

namespace {
const char* GUIDE_ITEMS[] = {"STAR MAP", "NAME THIS STAR", "NAME THIS WORLD", "WRITE A NOTE", "EXPEDITION LOG", "GALLERY", "STATISTICS", "THE SHARDS",   // R-408: the sights are marked on the sector map (N), not named here
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
    if (state == GameState::PROBE && probe.active) where = probeCaption() + " - " + where;   // X-01: what the probe's camera shows and where
    if (state == GameState::RECORDING && replay.rec >= 0 && replay.rec < (int)guide.probes.size()) {   // X-04: a recording's frame
        const ProbeRecord& r = guide.probes[replay.rec];
        const double c = std::min(replay.clock, r.endClock);
        where = fmt("THE RECORDING OF %s AT T+%02d:%02d, %.2f ATM", recordTitle(r).c_str(), (int)(c / 60), (int)std::fmod(c, 60.0), probeBarAt(ProbeFlight{r.chuteOpen, r.chuteClose, r.crushBar}, c) * 0.98692) + " - " + where;
    }
    if (tele.on && state == GameState::SPACE) {   // W-06: a photo through the telescope names what the reticle rests on
        std::string line = telescopeTargetLine(true);
        where = fmt("THROUGH THE TELESCOPE X%.0f: %s", telescopeZoom(), line.empty() ? "THE SKY" : line.c_str()) + " - " + where;
    }
    return where + "  " + epocString();
}

// C-03: a shard taken with E goes to the guide ("shard <body key>/S<index>"), the log and the status; the view then hides
// every copy of it on this world (`SurfaceView::shardsFound`, refilled at a landing and a load), so what still glints is new
std::string Game::shardKey(int index) const {
    return Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.valid ? surf.site.body : ship.localTarget) + "/S" + std::to_string(index);
}
std::string Game::graveKey(uint64_t id) const {   // C-12
    return Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.valid ? surf.site.body : ship.localTarget) + fmt("/G%llx", (unsigned long long)id);
}
void Game::syncShards() {
    surf.shardsFound.clear();
    if (!surf.valid && surf.site.body < 0) return;
    std::string prefix = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.site.body) + "/S";
    for (const std::string& k : guide.shards) if (k.compare(0, prefix.size(), prefix) == 0) surf.shardsFound.insert(atoi(k.c_str() + prefix.size()));
}
void Game::takeShard() {
    const SurfaceView::NearShard ns = surf.nearShard;
    if (ns.index < 0 || ns.index >= 2 * SHARDS_PER_WORLD) return;
    guide.shards.insert(shardKey(ns.index));
    syncShards(); roadsMet.clear();   // C-09
    int here = (int)surf.shardsFound.size(), total = shardsOfWorld(surf.site.gen);   // C-13: a world of two peoples holds a hundred
    const char* where = ns.sclass == SC_MONUMENT ? "A LONE MONUMENT" : (ns.sclass == SC_HAMLET ? "A HAMLET" : (ns.sclass == SC_VILLAGE ? "A VILLAGE" : "A TOWN"));
    bool caught = guide.heard.count(shardKey(ns.index)) > 0;   // C-07: a recording the radar heard from afar
    logEvent("SHARD", fmt("A SHARD OF THE OLD PEOPLE TAKEN FROM %s IN THE RUINS OF %s ON %s (%d OF %d FOUND THERE)%s", SHARD_PLACE_NAMES[ns.place], where, upper(bodyNameOf(surf.site.body)).c_str(), here, total, caught ? " - THE RECORDING THE RADAR CAUGHT" : ""));
    status(caught ? fmt("A SHARD OF THE OLD PEOPLE - THE RECORDING THE RADAR CAUGHT (%d OF %d)", here, total) : fmt("A SHARD OF THE OLD PEOPLE - %d OF %d ON THIS WORLD", here, total), 5);
    audio.beep = 1;
}

// C-14: a friend's export placed beside the guide as its inbox file: their names into the inbox (yours win), the shards they found
// and read lent to the decoder (`Guide::lent`: read there in cyan, counted in the world's language, never taken: the ruins keep them)
void Game::importInboxFile() {
    std::string p = guidePath.substr(0, guidePath.rfind('.')) + "_inbox.txt";
    int lentN = 0, probesN = 0, marksN = 0, n = guide.importInbox(p, &lentN, &probesN, &marksN);
    if (n < 0) { status(fmt("NO INBOX FILE (%s)", upper(p).c_str()), 5); return; }
    guide.save(guidePath);
    if (marksN > 0) { marksVersion++; logEvent("INBOX", fmt("%d MARK%s LENT BY THE INBOX, ON THE MAPS IN CYAN", marksN, marksN == 1 ? "" : "S")); }   // R-408: a friend's marks
    if (probesN > 0) logEvent("INBOX", fmt("%d PROBE RECORDING%s LENT BY THE INBOX, IN THE GALLERY", probesN, probesN == 1 ? "" : "S"));   // X-04: a friend's descents
    if (lentN > 0) {
        std::set<std::string> worlds;
        for (const std::string& k : guide.lent) { size_t q = k.rfind("/S"); if (q != std::string::npos) worlds.insert(k.substr(0, q)); }
        logEvent("INBOX", fmt("%d SHARDS OF %d WORLD%s LENT BY THE INBOX, TO READ ON THE DECODER", lentN, (int)worlds.size(), worlds.size() == 1 ? "" : "S"));
    }
    std::string got = fmt("INBOX: %d NAMES", n);   // 50 characters at most with all four
    if (lentN > 0) got += fmt(", %d SHARDS", lentN);
    if (probesN > 0) got += fmt(", %d RECORDINGS", probesN);
    if (marksN > 0) got += fmt(", %d MARKS", marksN);
    status(got, 5);
}

void Game::logEvent(const std::string& kind, const std::string& text) {
    guide.addLog(t, kind, epocString() + "  " + text);
    guide.save(guidePath);
}

void Game::noteVisit() {
    if (!sys.valid) return;
    migrateLandmarkNames();   // R-408: the sights named here before are marks now
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
        case 3: beginTextEntry("NOTE FOR THE LOG", 3, ""); break;
        case 4: logPage = 0; state = GameState::LOG; break;
        case 5: openGallery(); break;
        case 6: state = GameState::STATS; break;
        case 7: if (!onSurface) openShards(); else status("THE SHARDS ARE READ ON THE SHIP", 3); break;   // C-06
        case 8: if (!onSurface) beginTextEntry("STAR NAME", 4, ""); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 9: if (!onSurface) beginTextEntry("SECTOR X Y Z", 5, ""); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 10: if (!onSurface) targetPreviousStar(); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 11: if (!onSurface) targetHome(); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 12: if (sys.valid) { guide.home = starKeyOf(sys.star); guide.save(guidePath); status(fmt("HOME STAR: %s", upper(starNameOf(sys.star)).c_str()), 4); state = guideReturn; } break;
        case 13: { std::string p = guidePath.substr(0, guidePath.rfind('.')) + "_export.txt"; status(guide.save(p) ? fmt("GUIDE EXPORTED TO %s", upper(p).c_str()) : "EXPORT FAILED", 5); state = guideReturn; break; }
        case 14: importInboxFile(); state = guideReturn; break;   // C-14: the names into the inbox, the friend's read shards lent
        default: state = guideReturn; break;
    }
}

void Game::renderGuideMenu() {
    const int top = 10, bottom = UH - 6, y0 = top + 18;   // C-06: seventeen items at 9 px; the box grew with them (sixteen already ran into the footer)
    blendRectRGB(canvas, 40, top, UW - 40, bottom, rgb(0, 0, 0), 215);
    drawRectRGB(canvas, 40, top, UW - 40, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 6, "THE GUIDE", HUD_AMBER);
    for (int i = 0; i < GUIDE_N; i++) {
        int y = y0 + i * 9;
        bool sel = i == guideSel;
        std::string label = GUIDE_ITEMS[i];
        if (i == 2 && sys.valid) { int bi = guideReturn == GameState::SURFACE && surf.valid ? surf.site.body : ship.localTarget; if (bi >= 0 && bi < (int)sys.bodies.size() && bodyNamed(bi)) label = "RENAME " + trunc(upper(bodyNameOf(bi)), 22); }
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
    if (textKind == 6 && in.wasPressed(KEY_TAB)) { textMarkKind = (textMarkKind + 1) % (MARK_PLACE + 1); audio.beep = 4; }   // R-408: what the mark marks
    if (in.wasPressed(KEY_ESCAPE)) { state = guideReturn; textMark = -1; return; }
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
        case 2: {
            if (text.empty() || !sys.valid || textBody < 0 || textBody >= (int)sys.bodies.size()) return;
            guide.names[Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, textBody)] = text; guide.save(guidePath);
            const Body& b = sys.bodies[textBody];   // W-06: the telescope names moons, comets and the second sun too
            const char* what = b.type == PT_COMPANION ? "SECOND SUN" : (b.type == PT_COMET ? "COMET" : (b.parent >= 0 ? "MOON" : "WORLD"));
            status(fmt("THE %s IS NOW CALLED %s", what, text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A %s %s", what, text.c_str()));
            break;
        }
        case 8: {   // W-06: a star of the neighbourhood seen through the telescope
            if (text.empty()) return;
            guide.names[starKeyOf(textStar)] = text; guide.save(guidePath);
            status(fmt("THE STAR IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", "NAMED A STAR IN THE SKY " + text);
            break;
        }
        case 3:
            if (text.empty()) return;
            logEvent("NOTE", text);
            status("NOTE WRITTEN IN THE LOG", 3);
            break;
        case 4: targetStarByName(text); break;
        case 6: finishMark(text); break;   // R-408: a mark placed or renamed (an empty name places an unnamed one)
        case 9: {   // X-04: the storm a probe fell into (its screen, its recording)
            if (text.empty() || textStormKey.empty()) return;
            guide.names[textStormKey] = text; guide.save(guidePath);
            status(fmt("THE STORM IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A STORM %s", text.c_str()));
            break;
        }
        case 7: {   // C-04: the piece's name (the shard on the decoder screen)
            if (text.empty() || textShardKey.empty()) return;
            guide.names[textShardKey] = text; guide.save(guidePath);
            status(fmt("THE PIECE IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A PIECE OF THE OLD PEOPLE'S MUSIC %s", text.c_str()));
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
    blendRectRGB(canvas, 6, top, UW - 6, bottom, rgb(0, 0, 0), 225);   // R-408: as wide as its last line (it ran past the box)
    drawRectRGB(canvas, 6, top, UW - 6, bottom, HUD_DIM);
    drawTextCentered(canvas, UW / 2, top + 8, textPrompt.c_str(), HUD_AMBER);
    std::string shown = textBuffer + (std::fmod(realTime, 0.8) < 0.4 ? "_" : " ");
    drawTextCentered(canvas, UW / 2, top + 26, shown.c_str(), HUD_WHITE);
    if (textKind == 6) {   // R-408: what the mark marks, its glyph on the maps
        std::string k = textMarkKind < LM_COUNT ? fmt("%s %s", LANDMARK_KIND_SYMBOLS[textMarkKind], LANDMARK_KIND_NAMES[textMarkKind]) : std::string("A PLACE");
        drawTextCentered(canvas, UW / 2, top + 38, fmt("MARKS %s - TAB CHANGES", k.c_str()).c_str(), HUD_CYAN);
    }
    drawTextCentered(canvas, UW / 2, bottom - 12, "LETTERS, DIGITS, SPACE, -   ENTER OK   ESC CANCEL", HUD_DIM);
}

// ---------------------------------------------------------------------------
// targets by name, coordinates, history, home
// ---------------------------------------------------------------------------

// R-407: the names the explorer gave to a star's worlds (its key's bodies, not the sights, storms or pieces under them)
std::vector<std::string> Game::worldNamesAt(const std::string& starKey) const {
    std::vector<std::string> out;
    const std::string pre = starKey + "/";
    for (auto it = guide.names.lower_bound(pre); it != guide.names.end() && it->first.compare(0, pre.size(), pre) == 0; ++it)
        if (it->first.find('/', pre.size()) == std::string::npos) out.push_back(it->second);
    return out;
}

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
    mapYaw = 0.6; mapPitch = 0.5; mapZoom = 1.0; mapClassMask = (1 << STAR_CLASS_COUNT) - 1;
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
    // S-02: the class filter cycles through ALL and every class of the table (C forward, X back), whatever their number;
    // 0 shows all. Keys 1-9 toggled S00-S08 until S-01's tenth class had no key
    if (in.wasPressed(KEY_C) || in.wasPressed(KEY_X)) {
        int cur = -1;   // -1 all, else the one class shown
        for (int c = 0; c < STAR_CLASS_COUNT; c++) if (mapClassMask == (1 << c)) cur = c;
        int n = STAR_CLASS_COUNT + 1;
        cur = ((cur + 1 + (in.wasPressed(KEY_C) ? 1 : -1)) % n + n) % n - 1;
        mapClassMask = cur < 0 ? (1 << STAR_CLASS_COUNT) - 1 : (1 << cur);
    }
    if (in.wasPressed(KEY_0)) mapClassMask = (1 << STAR_CLASS_COUNT) - 1;
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
        else if (guide.visited.count(key) && !current) {   // R-407: an unnamed star the ship has been to, by a world named there
            std::vector<std::string> w = worldNamesAt(key);
            if (!w.empty()) drawText(canvas, sx + 5, sy - 3, ("(" + trunc(upper(w[0]), 10) + ")").c_str(), HUD_DIM);
        }
        double dd = std::sqrt((double)(sx - cx) * (sx - cx) + (double)(sy - cy) * (sy - cy));
        if (dd < pickD && !current) { pickD = dd; mapPick = pr.second; }
    }
    // crosshair and the picked star's card
    drawLineRGB(canvas, cx - 8, cy, cx - 3, cy, HUD_DIM); drawLineRGB(canvas, cx + 3, cy, cx + 8, cy, HUD_DIM);
    drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, HUD_DIM); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, HUD_DIM);
    drawText(canvas, 6, 4, fmt("STAR MAP  %d STARS WITHIN 10 LY  ZOOM %.1fX", (int)order.size(), mapZoom).c_str(), HUD_AMBER);
    int solo = -1;   // S-02: the filter shows all or one class (the legend of every code outgrew the line at ten classes)
    for (int c = 0; c < STAR_CLASS_COUNT; c++) if (mapClassMask == (1 << c)) solo = c;
    std::string filt = solo < 0 ? "CLASSES: ALL" : fmt("CLASS: %s %s", STAR_CLASSES[solo].code, STAR_CLASSES[solo].name);
    drawText(canvas, 6, 13, filt.c_str(), solo < 0 ? HUD_DIM : HUD_AMBER);
    {   // R-407 (the user's "green marker stars as unknown"): what the marks mean
        const int hw = textWidth("HOME"), vw = textWidth("VISITED");
        const int hText = UW - 6 - hw, hx = hText - 6, vText = hx - 12 - vw, bx = vText - 10, y = 13;
        drawRectRGB(canvas, bx, y, bx + 6, y + 6, rgb(90, 150, 110)); drawText(canvas, vText, y, "VISITED", HUD_DIM);
        drawLineRGB(canvas, hx - 3, y + 3, hx, y, HUD_AMBER); drawLineRGB(canvas, hx, y, hx + 3, y + 3, HUD_AMBER); drawLineRGB(canvas, hx + 3, y + 3, hx, y + 6, HUD_AMBER); drawLineRGB(canvas, hx, y + 6, hx - 3, y + 3, HUD_AMBER);
        drawText(canvas, hText, y, "HOME", HUD_DIM);
    }
    if (mapPick >= 0 && pickD < 40) {
        const Star& s = nb.stars[mapPick];
        Star full; starInSector(s.sx, s.sy, s.sz, full, true);
        std::string key = Guide::starKey(s.sx, s.sy, s.sz);
        double ly = length(full.pos - ship.pos) / SECTOR_KM;
        std::string card = fmt("%s  %s %s  %.2f LY%s", upper(starNameOf(full)).c_str(), STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, ly, guide.visited.count(key) ? "  VISITED" : "");
        drawTextCentered(canvas, UW / 2, UH - 24, card.c_str(), nameIsForeign(key) ? HUD_CYAN : HUD_WHITE);
        std::vector<std::string> w = worldNamesAt(key);   // R-407: the worlds named there
        if (!w.empty()) {
            std::string names; for (const std::string& n : w) names += (names.empty() ? "" : ", ") + upper(n);
            drawTextCentered(canvas, UW / 2, UH - 33, trunc("YOUR WORLDS THERE: " + names, 52).c_str(), HUD_GREEN);
        }
    }
    drawTextCentered(canvas, UW / 2, UH - 12, "MOUSE/WHEEL TURN/ZOOM  C/X CLASS  0 ALL  ENTER TARGET", HUD_DIM);   // S-02: 53 characters, the width of the frame
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
    auto sub = [&](const std::string& s) { drawText(canvas, 12, y, s.c_str(), HUD_DIM); y += 9; };   // C-12: a dim row under a count, a pixel closer (the page is full)
    line("DISCOVERY STATISTICS", HUD_AMBER);
    line(fmt("SYSTEMS VISITED       %d", (int)guide.visited.size()), HUD_GREEN);
    line(fmt("WORLDS LANDED ON      %d", (int)guide.landed.size()), HUD_GREEN);
    line(fmt("STAR CLASSES SEEN     %d/%d", (int)guide.classesSeen.size(), STAR_CLASS_COUNT), HUD_GREEN);
    std::string cls; for (int c : guide.classesSeen) cls += std::string(STAR_CLASSES[c].code) + " ";
    { std::vector<std::string> rows = wrapText(cls, 48); for (size_t k = 0; k < rows.size() && k < 2; k++) sub("  " + rows[k]); }   // S-02: the codes on their own line (two past twelve classes), like the world types; ten of them overran the count's line
    std::string types; for (int c : guide.typesSeen) types += std::string(shortType(c)) + " ";
    line(fmt("WORLD TYPES WALKED    %d/%d", (int)guide.typesSeen.size(), landableTypeCount()), HUD_GREEN);
    sub("  " + trunc(types, 48));
    line(fmt("FURTHEST FROM HOME    %.2f LY", guide.furthestFromHomeLY), HUD_GREEN);
    line(fmt("LONGEST WALK          %.2f KM FROM THE CAPSULE", guide.longestWalkM / 1000.0), HUD_GREEN);
    line(fmt("HIGHEST POINT REACHED %.0f M", guide.highestPointM), HUD_GREEN);
    line(fmt("FOOT, DRIVEN, FLOWN   %.1f / %.1f / %.1f KM", guide.totalWalkedM / 1000.0, guide.totalDrivenM / 1000.0, guide.totalFlownM / 1000.0), HUD_GREEN);   // C-03: one line, the shards took the other; R-403: the drone's kilometres
    line(fmt("SHARDS TAKEN          %d FROM %d WORLD%s   DECODED %d", (int)guide.shards.size(), guide.shardWorlds(), guide.shardWorlds() == 1 ? "" : "S", (int)guide.decoded.size()), HUD_GREEN);   // C-03; C-06: read on the ship
    sub(fmt("  GRAVES READ %d   RECORDS ENDED %d   LENT %d", (int)guide.graves.size(), (int)guide.ended.size(), guide.lentCount()));   // C-12; C-14: the inbox's shards
    line(fmt("NAMES GIVEN           %d   FROM OTHERS %d", (int)guide.names.size(), (int)guide.inbox.size()), HUD_GREEN);
    line(fmt("LOG ENTRIES           %d   PHOTOGRAPHS %d", (int)guide.log.size(), guide.screenshots), HUD_GREEN);
    line(fmt("CREATURES SIGHTED     %d SPECIES   SIGNALS HEARD %d", guide.creaturesSeen, (int)guide.signals.size()), HUD_GREEN);   // N3-01; C-07: the radar's locks
    sub(guide.probesSent ? fmt("  PROBES SENT %d   DEEPEST DESCENT %.0f KM", guide.probesSent, guide.deepestProbeKm()) : std::string("  PROBES SENT 0"));   // X-04
    line(fmt("LONGEST DRIVE         %.2f KM   TOP SPEED %.0f KM/H", guide.longestDriveM / 1000.0, guide.topSpeedKmh), HUD_GREEN);   // N4-05
    int64_t hx, hy, hz;
    if (!guide.home.empty() && Guide::parseStarKey(guide.home, hx, hy, hz)) { Star h; if (starInSector(hx, hy, hz, h, true)) line(fmt("HOME STAR             %s (%s)", upper(starNameOf(h)).c_str(), STAR_CLASSES[h.cls].code), HUD_CYAN); }
    drawTextCentered(canvas, UW / 2, UH - 10, "ANY KEY TO CLOSE", HUD_DIM);
}

// X-04: the photographs and the probes' recordings (each by its final image: shots/probe_<n>.png, a lent one drawn), in the order
// they were taken: a photograph by its file's time, a recording by the time it came back (a lent one: its lending)
void Game::openGallery() {
    galleryItems.clear();
    std::vector<GalleryItem> photos, recs;
    std::string dir = shotsDir;
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* ent;
        while ((ent = readdir(d)) != nullptr) {
            std::string n = ent->d_name;
            if (n.rfind("screenshot_", 0) == 0 && n.size() > 4 && n.substr(n.size() - 4) == ".png") {
                GalleryItem g; g.file = dir + "/" + n;
                struct stat st;
                if (stat(g.file.c_str(), &st) == 0) g.when = (long long)st.st_mtime;
                photos.push_back(g);
            }
        }
        closedir(d);
    }
    std::sort(photos.begin(), photos.end(), [](const GalleryItem& a, const GalleryItem& b) {
        auto num = [](const std::string& s) { size_t p = s.rfind("screenshot_"); return atoi(s.c_str() + p + 11); };
        return num(a.file) < num(b.file);
    });
    for (int i = 0; i < (int)guide.probes.size(); i++) {
        const ProbeRecord& r = guide.probes[i];
        GalleryItem g; g.rec = i; g.when = r.saved;
        if (!r.lent) { std::string f = fmt("%s/probe_%d.png", dir.c_str(), r.id); if (fileExists(f)) g.file = f; }
        recs.push_back(g);
    }
    std::stable_sort(recs.begin(), recs.end(), [](const GalleryItem& a, const GalleryItem& b) { return a.when < b.when; });
    size_t i = 0, j = 0;
    while (i < photos.size() || j < recs.size()) galleryItems.push_back(j >= recs.size() || (i < photos.size() && photos[i].when <= recs[j].when) ? photos[i++] : recs[j++]);
    gallerySel = (int)galleryItems.size() - 1;
    galleryLoaded = -1;
    state = GameState::GALLERY;
}

void Game::updateGallery(const Input& in) {
    int n = (int)galleryItems.size();
    if (n > 0) {
        if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_UP)) gallerySel = (gallerySel + n - 1) % n;
        if (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_DOWN)) gallerySel = (gallerySel + 1) % n;
    }
    if (n > 0 && enterKey(in) && galleryItems[gallerySel].rec >= 0) {   // X-04: a recording plays on the ship's screen
        if (guideReturn == GameState::SURFACE) { status("THE RECORDINGS PLAY ON THE SHIP'S SCREEN", 3); audio.beep = 3; }
        else openRecording(galleryItems[gallerySel].rec);
        return;
    }
    if (in.wasPressed(KEY_ESCAPE) || enterKey(in)) state = guideReturn;
}

void Game::renderGallery() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 235);
    int n = (int)galleryItems.size();
    if (n == 0) { drawTextCentered(canvas, UW / 2, 90, "NO PHOTOGRAPHS YET - P TAKES ONE", HUD_DIM); drawTextCentered(canvas, UW / 2, UH - 10, "ESC CLOSE", HUD_DIM); return; }
    gallerySel = clampi(gallerySel, 0, n - 1);
    const GalleryItem& it = galleryItems[gallerySel];
    const ProbeRecord* rec = it.rec >= 0 && it.rec < (int)guide.probes.size() ? &guide.probes[it.rec] : nullptr;
    if (galleryLoaded != gallerySel) {
        galleryLoaded = gallerySel;
        galleryCaption.clear();
        if (!it.file.empty()) galleryOk = readPNG(it.file.c_str(), galleryPix, galleryW, galleryH);
        else if (rec) { recordingPoster(it.rec, galleryPix); galleryW = FBW; galleryH = FBH; galleryOk = galleryPix.size() == (size_t)FBW * FBH; }   // a lent recording: its last frame drawn
        else galleryOk = false;
        if (rec) galleryCaption = recordLine(*rec);
        else {
            std::string side = it.file.substr(0, it.file.size() - 4) + ".txt";
            std::ifstream f(side);
            if (f) std::getline(f, galleryCaption);
        }
    }
    // the picture scaled into a 256x160 logical frame
    const int fx = 32, fy = 8, fw = 256, fh = 160;   // X-04: four up, the caption's second line clear of the keys
    int S = FB_SCALE;
    if (galleryOk && galleryW > 0) {
        for (int y = 0; y < fh * S; y++)
            for (int x = 0; x < fw * S; x++) {
                int sx = (int)((double)x / (fw * S) * galleryW), sy = (int)((double)y / (fh * S) * galleryH);
                canvas.px[(size_t)(fy * S + y) * FBW + fx * S + x] = galleryPix[(size_t)sy * galleryW + sx];
            }
    } else drawTextCentered(canvas, UW / 2, 90, "CANNOT READ THIS FILE", HUD_RED);
    drawRectRGB(canvas, fx - 1, fy - 1, fx + fw, fy + fh, rec ? (rec->lent ? HUD_CYAN : HUD_AMBER) : HUD_DIM);
    if (rec) {
        drawText(canvas, fx, fy + fh + 4, fmt("%d/%d  %s%s", gallerySel + 1, n, recordTitle(*rec).c_str(), rec->lent ? "  LENT" : "").c_str(), rec->lent ? HUD_CYAN : HUD_AMBER);
        drawText(canvas, fx, fy + fh + 13, trunc(galleryCaption, 42).c_str(), HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 8, "LEFT/RIGHT BROWSE  ENTER PLAYS  ESC CLOSE", HUD_DIM);
        return;
    }
    std::string name = it.file.substr(it.file.rfind('/') + 1);
    drawText(canvas, fx, fy + fh + 4, fmt("%d/%d  %s", gallerySel + 1, n, upper(name).c_str()).c_str(), HUD_AMBER);
    drawText(canvas, fx, fy + fh + 13, trunc(galleryCaption, 42).c_str(), HUD_GREEN);
    drawTextCentered(canvas, UW / 2, UH - 8, "LEFT/RIGHT BROWSE   ESC CLOSE", HUD_DIM);
}
