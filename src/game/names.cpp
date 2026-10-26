// What the guide keeps (M3, R-401): the names (UNKNOWN until the explorer gives one), the keys of the shards and the
// instruments and what is taken, a friend's inbox file, the log's events and the visits
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

// ---------------------------------------------------------------------------
// names
// ---------------------------------------------------------------------------

std::string Game::starKeyOf(const Star& s) const { return Guide::starKey(s.sx, s.sy, s.sz); }

// R-401: as in the original, every star, world, belt and landmark is UNKNOWN until an explorer names it (or a friend's
// inbox does). The generated names stay inside the generator (keys, landmark ids, the harness) and never reach the screen.
static const char* UNNAMED_LABEL = "UNKNOWN";

bool Game::starNamed(const Star& s) const { std::string key = starKeyOf(s); return guide.names.count(key) || guide.inbox.count(key); }

// R-401: the explorer's (or the inbox's) name, else "UNKNOWN"
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
    if (b.doublePlanet && b.parent >= 0) return n + " (" + KINDS[b.type].name + ", twin planet)";
    return n + " (" + KINDS[b.type].name + (b.parent >= 0 ? (sys.bodies[b.parent].type == PT_COMPANION ? ", of the companion)" : ", moon)") : ")");
}

bool Game::nameIsForeign(const std::string& key) const { return !guide.names.count(key) && guide.inbox.count(key); }

// for the sidecar text next to a screenshot
std::string Game::screenshotCaption() const {
    std::string where;
    if (surf.valid && (stateHas(state, ST_GROUND) || returnState == GameState::SURFACE))
        where = fmt("%s  %.1f%s %.1f%s  %s", upper(bodyNameOf(surf.site.body)).c_str(), std::fabs(surf.env.latDeg), surf.env.latDeg >= 0 ? "N" : "S",
                    std::fabs(surf.env.lonDeg), surf.env.lonDeg >= 0 ? "E" : "W", KINDS[sys.bodies[surf.site.body].type].name);
    else if (sys.valid)
        where = ship.mode == ShipState::PARKED && ship.parkedBody >= 0 ? fmt("IN ORBIT OF %s", upper(bodyNameOf(ship.parkedBody)).c_str())
                                                                       : fmt("SYSTEM %s (%s)", upper(starNameOf(sys.star)).c_str(), STAR_CLASSES[sys.star.cls].code);
    else where = "INTERSTELLAR SPACE";
    if (state == GameState::PROBE && probe.active) where = probeCaption() + " - " + where;   // X-01: what the probe's camera shows and where
    if (state == GameState::RECORDING && replay.rec >= 0 && replay.rec < (int)guide.probes.size()) {   // a recording's frame
        const ProbeRecord& r = guide.probes[replay.rec];
        const double c = std::min(replay.clock, r.endClock);
        where = fmt("THE RECORDING OF %s AT T+%02d:%02d, %.2f ATM", recordTitle(r).c_str(), (int)(c / 60), (int)std::fmod(c, 60.0),
                        probeBarAt(ProbeFlight{r.chuteOpen, r.chuteClose, r.crushBar}, c) * 0.98692) +
                " - " + where;
    }
    if (tele.on && state == GameState::SPACE) {   // a photo through the telescope names what the reticle rests on
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
// C-12: "<body key>/G<id>" on the surface's body
std::string Game::graveKey(uint64_t id) const {
    return Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.valid ? surf.site.body : ship.localTarget) + fmt("/G%llx", (unsigned long long)id);
}
// C-03: the view hides this world's shards the guide already holds (C-11: and its instruments)
void Game::syncShards() {
    surf.shardsFound.clear(); surf.instrumentsFound.clear();
    if (!surf.valid && surf.site.body < 0) return;
    std::string prefix = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.site.body) + "/S";
    for (const std::string& k : guide.shards) if (k.compare(0, prefix.size(), prefix) == 0) surf.shardsFound.insert(atoi(k.c_str() + prefix.size()));
    const std::string iprefix = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.site.body) + "/I";   // and the instruments
    for (const std::string& k : guide.instruments) if (k.compare(0, iprefix.size(), iprefix) == 0) surf.instrumentsFound.insert(atoi(k.c_str() + iprefix.size()));
}
// C-11: "<body key>/I<id>" on the surface's body
// an instrument taken with E goes to the guide ("instrument <body key>/I<id>"), the log and the status, and into the cabin's case; every
// copy of it on this world is then gone (`SurfaceView::instrumentsFound`)
std::string Game::instrumentKey(int id) const {
    return Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.valid ? surf.site.body : ship.localTarget) + "/I" + std::to_string(id);
}
// C-11: the instrument within reach into the guide and the cabin's case
void Game::takeInstrument() {
    const SurfaceView::NearInstrument ni = surf.nearInstrument;
    if (ni.id < 0) return;
    guide.instruments.insert(instrumentKey(ni.id));
    syncShards(); roadsMet.clear();
    const std::string what = ni.name.empty() ? std::string("AN INSTRUMENT") : ni.name;
    int here = 0; const std::string prefix = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, surf.site.body) + "/I";
    for (const std::string& k : guide.instruments) if (k.compare(0, prefix.size(), prefix) == 0) here++;
    logEvent("INSTRUMENT", fmt("%s OF THE OLD PEOPLE TAKEN ON %s, TO THE CABIN'S CASE (%d FROM THIS WORLD)", what.c_str(), upper(bodyNameOf(surf.site.body)).c_str(), here));
    status(fmt("%s OF THE OLD PEOPLE - TO THE CABIN'S CASE", what.c_str()), 5);
    audio.beep = BEEP_LOCK;
}
// C-03: E at a shard: the guide, the log, the status
void Game::takeShard() {
    const SurfaceView::NearShard ns = surf.nearShard;
    if (ns.index < 0 || shardPeopleOf(ns.index) > 1 || shardLocalOf(ns.index) >= RECORDS_MAX) return;
    guide.shards.insert(shardKey(ns.index));
    syncShards(); roadsMet.clear();
    int here = (int)surf.shardsFound.size(), total = shardsOfWorld(surf.site.gen);   // both peoples' records; T-04: 80 to 200 each
    // the object and where it lay: the settlement's class, or the work it lay in
    const std::string what = RECORD_MEDIUM_HUD[clampi(ns.medium, 0, RM_COUNT - 1)];
    const std::string where = ns.sclass == SC_WORKS && ns.work >= 0 && ns.work < WO_COUNT ? std::string("AT ") + WORK_HEARD[ns.work]
                                                                                          : std::string("IN THE RUINS OF ") +
                    (ns.sclass == SC_MONUMENT
                                    ? "A LONE MONUMENT"
                                    : (ns.sclass == SC_HAMLET ? "A HAMLET"
                                                              : (ns.sclass == SC_VILLAGE ? "A VILLAGE" : (ns.sclass == SC_CITY ? "A CITY" : (ns.sclass == SC_CAPITAL ? "A GREAT CITY" : "A TOWN")))));
    const char* verb = ns.wall ? "RECORDED" : "TAKEN";   // a painted wall stays where it was painted
    bool caught = guide.heard.count(shardKey(ns.index)) > 0;   // a recording the radar heard from afar
    logEvent("SHARD",
            fmt("%s OF THE OLD PEOPLE %s %s %s %s ON %s (%d OF %d FOUND THERE)%s", what.c_str(), verb, ns.wall ? "IN" : "FROM",
                    ns.archive ? (std::string("AN ARCHIVE IN ") + buildingPlaceName(ns.place)).c_str() : buildingPlaceName(ns.place), where.c_str(), upper(bodyNameOf(surf.site.body)).c_str(), here,
                    total, caught ? " - THE RECORDING THE RADAR CAUGHT" : ""));
    status(caught ? fmt("%s OF THE OLD PEOPLE - THE RECORDING THE RADAR CAUGHT (%d OF %d)", what.c_str(), here, total)
                  : fmt("%s OF THE OLD PEOPLE - %d OF %d ON THIS WORLD", what.c_str(), here, total),
            5);
    audio.beep = BEEP_LOCK;
}

// C-14: a friend's export placed beside the guide as its inbox file: their names into the inbox (yours win), the shards they found
// and read lent to the decoder (`Guide::lent`: read there in cyan, counted in the world's language, never taken: the ruins keep them)
void Game::importInboxFile() {
    std::string p = guidePath.substr(0, guidePath.rfind('.')) + "_inbox.txt";
    int lentN = 0, probesN = 0, worldMarksN = 0, galaxyN = 0, n = guide.importInbox(p, &lentN, &probesN, &worldMarksN, &galaxyN);
    if (n < 0) { status(fmt("NO INBOX FILE (%s)", upper(p).c_str()), 5); return; }
    guide.save(guidePath);
    const int marksN = worldMarksN + galaxyN;   // the marks of the galaxy with the worlds' (the chart is a map too)
    if (worldMarksN > 0) marksVersion++;
    if (marksN > 0) logEvent("INBOX", fmt("%d MARK%s LENT BY THE INBOX, ON THE MAPS IN CYAN", marksN, marksN == 1 ? "" : "S"));   // a friend's marks
    if (probesN > 0) logEvent("INBOX", fmt("%d PROBE RECORDING%s LENT BY THE INBOX, IN THE GALLERY", probesN, probesN == 1 ? "" : "S"));   // a friend's descents
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
    if (!sys.sunless()) guide.classesSeen.insert(sys.star.cls);   // a drifter's place is no class of star
    if (guide.home.empty()) guide.home = key;
    if (!guide.home.empty()) {
        int64_t hx, hy, hz;
        if (Guide::parseStarKey(guide.home, hx, hy, hz)) {
            Star h; if (placeInSector(hx, hy, hz, h)) guide.furthestFromHomeLY = std::max(guide.furthestFromHomeLY, length(h.pos - sys.star.pos) / SECTOR_KM);
        }
    }
}
