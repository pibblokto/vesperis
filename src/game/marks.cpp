// R-408 (2026-10-07): the explorer's own marks and the surface scanner. The user on O6-06's landmarks: a world's sights could be
// named only as "the nearest landmark", once, and finding the ruins meant landing, asking the guide, launching and landing again;
// "remove them as markers ... make the surface N map more interactive so you can place arbitrary marker on something YOU see as a
// landmark and name it", and "some sort of echo locator or scanner ... in different modes so you can tune it for ruins or for water
// to find lakes or for peaks", which can leave the sights already marked out. The sights stay what the terrain makes them
// (galaxy/landmarks.h); the scanner hears them and the maps show only what the explorer marked
#include "game.h"
#include "ui.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <cstdio>

static_assert(MARK_PLACE == LM_COUNT, "a mark of a place is the kind after the sights'");

namespace {
// the scanner's modes in the order R cycles them, and their names (by LandmarkKind)
const int SCAN_ORDER[LM_COUNT] = {LM_RUIN, LM_LAKE, LM_PEAK, LM_CRATER, LM_CANYON, LM_MESA, LM_GEYSERS, LM_CRYSTALS};
const char* SCAN_MODE_NAMES[LM_COUNT] = {"PEAKS", "MESAS", "CANYONS", "CRATERS", "GEYSERS", "CRYSTALS", "LAKES", "RUINS"};
const char* RUIN_SIZE_NAMES[5] = {"A STRUCTURE", "A HAMLET", "A VILLAGE", "A TOWN", "A MONUMENT"};   // LM_RUIN's `sub`
}

const std::vector<Game::SiteMark>& Game::marksHere() {
    if (!surf.valid) { siteMarks.clear(); siteMarksVersion = -1; return siteMarks; }
    if (siteMarksEpoch == surf.siteEpoch && siteMarksBody == surf.site.body && siteMarksVersion == marksVersion && siteMarksCount == guide.marks.size()) return siteMarks;
    siteMarks.clear();
    const std::string body = worldKeyOf(surf.site.body);
    for (int i = 0; i < (int)guide.marks.size(); i++) {
        const SurfaceMark& m = guide.marks[i];
        if (m.body != body) continue;
        SiteMark sm; sm.index = i;
        surf.site.localAt(StarSystem::bodyFromLatLon(m.lat, m.lon), sm.x, sm.z);
        TerrainVertex v = surf.site.sampleAt(sm.x, sm.z, 64.0);
        sm.y = std::max((double)v.h, (double)v.water);
        siteMarks.push_back(sm);
    }
    siteMarksEpoch = surf.siteEpoch; siteMarksBody = surf.site.body; siteMarksVersion = marksVersion; siteMarksCount = guide.marks.size();
    return siteMarks;
}

int Game::markNear(double x, double z, double radiusM) {
    int best = -1; double bd = radiusM * radiusM;
    for (const SiteMark& m : marksHere()) { double d2 = (m.x - x) * (m.x - x) + (m.z - z) * (m.z - z); if (d2 < bd) { bd = d2; best = m.index; } }
    return best;
}

std::string Game::markLabel(int mi) const {
    if (mi < 0 || mi >= (int)guide.marks.size()) return "";
    const SurfaceMark& m = guide.marks[mi];
    if (!m.name.empty()) return upper(m.name);
    return m.kind >= 0 && m.kind < LM_COUNT ? std::string("A MARK: ") + LANDMARK_KIND_NAMES[m.kind] : std::string("A MARK");
}

// a sight is marked when one of the explorer's own marks stands within its extent (at least 150 m) and 250 m more
bool Game::echoMarked(const SurfaceView::ScanEcho& e) {
    const double reach = std::max(e.lm.radiusM, 150.0) + 250.0;
    for (const SiteMark& m : marksHere()) if (!guide.marks[m.index].lent && (m.x - e.x) * (m.x - e.x) + (m.z - e.z) * (m.z - e.z) < reach * reach) return true;
    return false;
}

// an echo is heard off its place by up to 8% of its distance (3 km at most) in a direction of its own: far, a hint of the way;
// near, where it is
void Game::echoHeardAt(const SurfaceView::ScanEcho& e, double& x, double& z) const {
    const double dx = e.x - surf.player.x, dz = e.z - surf.player.z, off = std::min(0.08 * std::sqrt(dx * dx + dz * dz), 3000.0);
    const uint64_t h = mix64(e.lm.id ^ (0x5CA4ULL + (uint64_t)e.lm.kind * 0x9E3779B97F4A7C15ULL));
    const double a = unitFromHash(h) * TAU, r = 0.4 + 0.6 * unitFromHash(mix64(h + 1));
    x = e.x + std::cos(a) * off * r; z = e.z + std::sin(a) * off * r;
}

const SurfaceView::ScanEcho* Game::scanNearest(double& hx, double& hz) {
    if (scanMode < 0 || !surf.valid) return nullptr;
    const SurfaceView::ScanEcho* best = nullptr; double bd = 1e300;
    for (const SurfaceView::ScanEcho& e : surf.echoes) {
        if (e.lm.kind != scanMode || (scanSkipMarked && echoMarked(e))) continue;
        double d2 = (e.x - surf.player.x) * (e.x - surf.player.x) + (e.z - surf.player.z) * (e.z - surf.player.z);
        if (d2 < bd) { bd = d2; best = &e; }
    }
    if (best) echoHeardAt(*best, hx, hz);
    return best;
}

bool Game::scanModeHere(int mode) const { return surf.valid && mode >= 0 && mode < LM_COUNT && ((sightKindsOf(surf.site.gen) >> mode) & 1); }

void Game::scanCycle() {
    if (!surf.valid) return;
    const unsigned kinds = sightKindsOf(surf.site.gen);
    if (!kinds) { scanMode = -1; surf.scanWanted = false; status("THE SCANNER HAS NOTHING TO READ ON THIS WORLD", 4); audio.beep = 3; return; }
    int pos = -1;
    for (int i = 0; i < LM_COUNT; i++) if (SCAN_ORDER[i] == scanMode) pos = i;
    int next = -1;
    for (int i = pos + 1; i < LM_COUNT && next < 0; i++) if ((kinds >> SCAN_ORDER[i]) & 1) next = SCAN_ORDER[i];
    scanMode = next; surf.scanWanted = next >= 0;
    status(next < 0 ? "SCANNER OFF" : fmt("SCANNER: %s - R NEXT, SHIFT+R THE MARKED %s", SCAN_MODE_NAMES[next], scanSkipMarked ? "IN" : "OUT"), 4);
    audio.beep = 4;
}

void Game::scanToggleSkip() {
    scanSkipMarked = !scanSkipMarked;
    status(scanSkipMarked ? "SCANNER: THE SIGHTS YOU MARKED ARE LEFT OUT" : "SCANNER*: THE SIGHTS YOU MARKED ARE HEARD TOO", 4);
    audio.beep = 4;
}

std::string Game::echoDetail(const SurfaceView::ScanEcho& e) const {
    const Landmark& L = e.lm;
    switch (L.kind) {
        case LM_RUIN: return RUIN_SIZE_NAMES[clampi(L.sub, 0, 4)];
        case LM_PEAK: return fmt("%.0f M, %.0f OVER", L.heightM, L.prominenceM);
        case LM_MESA: return fmt("%.0f M HIGH", L.prominenceM);
        case LM_CANYON: return fmt("%.0f M DEEP", L.prominenceM);
        case LM_CRYSTALS: return "SPIRES";
        default: return fmt("%.1f KM ACROSS", 2 * L.radiusM / 1000.0);   // a lake, a crater, a geyser field
    }
}

// the scanner's two lines at the top left (on foot under the sector's line, in a vehicle under its camera's) and its mark on the
// compass: an arrow over the strip at the echo's bearing, or at the strip's end toward it
void Game::drawScanHUD() {
    if (scanMode < 0 || !surf.valid) return;
    const bool seat = surf.inVehicle() && !surf.chaseCam;
    const int y0 = seat ? 32 : 24;
    const double px = surf.player.x, pz = surf.player.z;
    double hx = 0, hz = 0;
    const SurfaceView::ScanEcho* e = nullptr;
    std::string l2;
    if (!scanModeHere(scanMode)) l2 = "NONE ON THIS WORLD";
    else if (!surf.scanned) l2 = std::string("SCANNING") + std::string((int)(realTime * 2) % 4, '.');
    else if (!(e = scanNearest(hx, hz))) l2 = "NONE IN 40 KM";
    else {
        const double d = std::hypot(hx - px, hz - pz);
        l2 = std::hypot(e->x - px, e->z - pz) < std::max(e->lm.radiusM, 60.0) ? "HERE" : fmt("%s %s", metresString(d).c_str(), compassName(wrap2pi(std::atan2(hx - px, hz - pz))));
    }
    drawTextShadow(canvas, 8, y0, fmt("SCAN %s%s", SCAN_MODE_NAMES[scanMode], scanSkipMarked ? "" : "*").c_str(), HUD_DIM, HUD_SHADOW);   // * the marked heard too
    drawTextShadow(canvas, 8, y0 + 8, l2.c_str(), HUD_CYAN, HUD_SHADOW);
    if (!e || l2 == "HERE") return;
    const int cx = UW / 2, y = 28;   // the compass strip's line (renderSurfaceHUD: the strip at 22, its line 6 under)
    const double off = wrapAngle(std::atan2(hx - px, hz - pz) - surf.player.yaw) / DEG;
    if (std::fabs(off) <= 60) {   // a diamond on the strip's line at the echo's bearing
        const int x = cx + (int)std::lround(off);
        for (int r = 3; r >= 2; r--) { const uint32_t c = r == 3 ? HUD_SHADOW : HUD_CYAN; drawLineRGB(canvas, x - r, y, x, y - r, c); drawLineRGB(canvas, x, y - r, x + r, y, c); drawLineRGB(canvas, x + r, y, x, y + r, c); drawLineRGB(canvas, x, y + r, x - r, y, c); }
        fillRectRGB(canvas, x - 1, y - 1, x + 1, y + 1, HUD_CYAN);
    } else {   // an arrow at the strip's end the echo lies beyond
        char glyph[2] = {off < 0 ? CH_LEFT : CH_RIGHT, 0};
        drawTextShadow(canvas, off < 0 ? cx - 69 : cx + 63, y - 4, glyph, HUD_CYAN, HUD_SHADOW);
    }
}

// the explorer's marks over the view: the eight nearest within 25 km in front, a diamond over the ground and the name with the distance
void Game::drawMarksInView() {
    const std::vector<SiteMark>& ms = marksHere();
    if (ms.empty()) return;
    const double px = surf.player.x, pz = surf.player.z;
    std::vector<std::pair<double, const SiteMark*>> near;
    for (const SiteMark& m : ms) { double d = std::hypot(m.x - px, m.z - pz); if (d < 25000 && d > 4) near.push_back({d, &m}); }
    std::sort(near.begin(), near.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    if (near.size() > 8) near.resize(8);
    for (const auto& nm : near) {
        const SiteMark& m = *nm.second;
        double sx, sy;
        if (!surf.projectPoint(m.x, m.y + 6, m.z, sx, sy)) continue;
        const int lx = (int)(sx / FB_SCALE), ly = (int)(sy / FB_SCALE);
        if (lx < 6 || ly < 44 || lx > UW - 6 || ly > UH - 52) continue;
        const uint32_t col = guide.marks[m.index].lent ? HUD_CYAN : HUD_WHITE;
        drawLineRGB(canvas, lx - 3, ly, lx, ly - 3, col); drawLineRGB(canvas, lx, ly - 3, lx + 3, ly, col); drawLineRGB(canvas, lx + 3, ly, lx, ly + 3, col); drawLineRGB(canvas, lx, ly + 3, lx - 3, ly, col);
        std::string lab = trunc(markLabel(m.index), 18) + " " + metresString(nm.first);
        int tx = lx + 6;
        if (tx + textWidth(lab.c_str()) > UW - 4) tx = lx - 6 - textWidth(lab.c_str());
        drawTextShadow(canvas, tx, ly - 3, lab.c_str(), col, HUD_SHADOW);
    }
}

// a new mark: the kind the scanner hears at the place (the mode's first), else a place; the name's entry follows (Tab changes the
// kind, Enter places it, an empty name too; Esc places nothing)
void Game::beginMark(double x, double z) {
    if (!surf.valid) return;
    surf.site.latLonAt(x, z, textMarkLat, textMarkLon);
    textMark = -1; textMarkKind = MARK_PLACE;
    double best = 1e300;
    for (const SurfaceView::ScanEcho& e : surf.echoes) {
        const double d = std::hypot(e.x - x, e.z - z), reach = std::max(e.lm.radiusM, 150.0) + 150.0;
        const double score = d - (e.lm.kind == scanMode ? 1e6 : 0);
        if (d < reach && score < best) { best = score; textMarkKind = e.lm.kind; }
    }
    guideReturn = state == GameState::SECTOR_MAP ? GameState::SECTOR_MAP : GameState::SURFACE;
    returnState = GameState::SURFACE;   // the entry is drawn over the ground (and the map)
    beginTextEntry("NAME THE MARK", 6, "");
}

void Game::beginRenameMark(int mi) {
    if (mi < 0 || mi >= (int)guide.marks.size()) return;
    textMark = mi; textMarkKind = guide.marks[mi].kind;
    guideReturn = state == GameState::SECTOR_MAP ? GameState::SECTOR_MAP : GameState::SURFACE;
    returnState = GameState::SURFACE;
    beginTextEntry("RENAME THE MARK", 6, upper(guide.marks[mi].name));
}

void Game::finishMark(const std::string& name) {
    if (textMark >= 0) {   // a rename (a friend's mark renamed is the explorer's own)
        if (textMark >= (int)guide.marks.size()) return;
        SurfaceMark& m = guide.marks[textMark];
        const bool renamed = m.name != name;
        m.name = name; m.kind = textMarkKind; m.lent = false;
        marksVersion++; guide.save(guidePath);
        if (renamed && !name.empty()) logEvent("NOTE", fmt("NAMED A MARK %s", name.c_str()));
        status(name.empty() ? "THE MARK HAS NO NAME NOW" : fmt("THE MARK IS NOW CALLED %s", name.c_str()), 4);
        textMark = -1;
        return;
    }
    if (!surf.valid) return;
    SurfaceMark m; m.body = worldKeyOf(surf.site.body); m.lat = textMarkLat; m.lon = textMarkLon; m.kind = textMarkKind; m.name = name; m.when = (long long)std::time(nullptr);
    guide.marks.push_back(m); marksVersion++;
    const std::string what = m.kind < LM_COUNT ? fmt(" (%s)", LANDMARK_KIND_NAMES[m.kind]) : std::string();
    logEvent("MARK", fmt("%s%s ON %s AT %.2f%s %.2f%s", name.empty() ? "A MARK" : name.c_str(), what.c_str(), upper(bodyNameOf(surf.site.body)).c_str(),
                         std::fabs(m.lat / DEG), m.lat >= 0 ? "N" : "S", std::fabs(wrapAngle(m.lon) / DEG), wrapAngle(m.lon) >= 0 ? "E" : "W"));   // saves the guide
    status(name.empty() ? "A MARK PLACED - ENTER ON IT IN THE MAP NAMES IT" : fmt("MARKED: %s", name.c_str()), 4);
    audio.beep = 1;
}

void Game::removeMark(int mi) {
    if (mi < 0 || mi >= (int)guide.marks.size()) return;
    const std::string label = markLabel(mi);
    guide.marks.erase(guide.marks.begin() + mi);
    marksVersion++; guide.save(guidePath);
    status(fmt("MARK REMOVED: %s", trunc(label, 30).c_str()), 3);
    audio.beep = 4;
}

// the sights the explorer named under O6-06 (`<world>/L<id>` names, the explorer's and the inbox's) become marks where they stand,
// with their names and kinds, for this system's worlds; a name whose sight is not found again stays as it was. The cells are found
// on a thread of their own (a system's dozen names asked a dozen cells and their drainage tiles: seconds on a cold start, which
// froze the arrival), `collectMigration` puts the marks in the guide
void Game::migrateLandmarkNames() {
    if (!sys.valid || migrateBusy.load(std::memory_order_acquire)) return;
    if (migrateThread.joinable()) migrateThread.join();
    const std::string star = starKeyOf(sys.star) + "/";
    std::map<int, std::set<uint32_t>> want;
    for (const std::map<std::string, std::string>* names : {&guide.names, &guide.inbox})
        for (auto it = names->lower_bound(star); it != names->end() && it->first.compare(0, star.size(), star) == 0; ++it) {
            const size_t p = it->first.find("/L", star.size());
            if (p == std::string::npos || migrateTried.count(it->first)) continue;
            const int body = std::atoi(it->first.c_str() + star.size());
            if (body >= 0 && body < (int)sys.bodies.size()) want[body].insert((uint32_t)std::strtoul(it->first.c_str() + p + 2, nullptr, 10));
        }
    if (want.empty()) return;
    std::vector<std::pair<BodyGen, std::pair<std::string, std::set<uint32_t>>>> work;
    for (const auto& w : want) work.push_back({BodyGen::make(sys.bodies[w.first]), {worldKeyOf(w.first), w.second}});
    migrateOut.clear();
    migrateBusy.store(true);
    std::vector<MigrationJob>* out = &migrateOut; std::atomic<bool>* busy = &migrateBusy;
    migrateThread = std::thread([work, out, busy]() {
        const auto t0 = std::chrono::steady_clock::now();
        for (const auto& w : work) {
            std::map<uint32_t, std::pair<int, int>> cells;
            cellsOfLegacyIds(w.first, w.second.second, cells);
            for (uint32_t id : w.second.second) {
                MigrationJob j; j.body = w.second.first; j.key = w.second.first + "/L" + std::to_string(id);
                auto c = cells.find(id);
                CellSights S;
                j.found = c != cells.end() && sightsOfCell(w.first, c->second.first, c->second.second, S, true) && legacyLandmarkOf(S, j.lm);
                out->push_back(j);
            }
        }
        if (std::getenv("VESPERIS_TRACE")) fprintf(stderr, "  landmark names to marks: %zu asked in %.0f ms\n", out->size(), std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
        busy->store(false, std::memory_order_release);
    });
}

void Game::collectMigration() {
    if (!migrateThread.joinable() || migrateBusy.load(std::memory_order_acquire)) return;
    migrateThread.join();
    int moved = 0;
    for (const MigrationJob& j : migrateOut) {
        migrateTried.insert(j.key);
        if (!j.found) continue;
        for (int pass = 0; pass < 2; pass++) {   // the explorer's name, then a friend's (lent)
            std::map<std::string, std::string>& names = pass == 0 ? guide.names : guide.inbox;
            auto it = names.find(j.key);
            if (it == names.end()) continue;
            SurfaceMark m; m.body = j.body; StarSystem::latLonFromBody(j.lm.unit, m.lat, m.lon); m.kind = j.lm.kind; m.name = it->second; m.lent = pass == 1; m.when = (long long)std::time(nullptr);
            guide.marks.push_back(m); moved++;
            names.erase(it);
        }
    }
    migrateOut.clear();
    if (moved) {
        marksVersion++;
        logEvent("NOTE", fmt("%d SIGHT%s NAMED IN THIS SYSTEM %s MARKS ON THE MAPS NOW", moved, moved == 1 ? "" : "S", moved == 1 ? "IS ONE OF THE" : "ARE"));   // saves the guide
        status(fmt("YOUR %d NAMED SIGHT%s HERE %s NOW MARKS ON THE MAPS", moved, moved == 1 ? "" : "S", moved == 1 ? "IS" : "ARE"), 6);
    }
    migrateLandmarkNames();   // the ship may be at another system's names by now
}

std::string Game::testScanInfo() {
    std::string s = fmt("scan mode %d skip %d scanned %d (%.0f ms), %zu echoes", scanMode, scanSkipMarked ? 1 : 0, surf.scanned ? 1 : 0, surf.scanMs, surf.echoes.size());
    int kinds[LM_COUNT] = {0};
    for (const auto& e : surf.echoes) if (e.lm.kind >= 0 && e.lm.kind < LM_COUNT) kinds[e.lm.kind]++;
    for (int k = 0; k < LM_COUNT; k++) if (kinds[k]) s += fmt(" %s %d", LANDMARK_KIND_NAMES[k], kinds[k]);
    double hx, hz;
    if (const SurfaceView::ScanEcho* e = scanNearest(hx, hz))
        s += fmt("; nearest at %.0f %.0f (%.0f m, %s), heard at %.0f %.0f", e->x, e->z, std::hypot(e->x - surf.player.x, e->z - surf.player.z), echoDetail(*e).c_str(), hx, hz);
    s += fmt("; %zu marks here", marksHere().size());
    return s;
}

bool Game::testEchoTrue(double& x, double& z, double& radius) {
    double hx, hz;
    const SurfaceView::ScanEcho* e = scanNearest(hx, hz);
    if (!e) return false;
    x = e->x; z = e->z; radius = e->lm.radiusM;
    return true;
}
