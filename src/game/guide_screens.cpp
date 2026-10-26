// GUIDE screens (M3): the guide menu, text entry, the expedition log, statistics and the gallery (the names and what the
// guide keeps are names.cpp, the star map and the targets star_map.cpp).
#include "game.h"
#include "peoples/works.h"
#include "ui.h"
#include "peoples/buildings.h"   // A-03: a shard's place
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
const char* const GUIDE_ITEMS[] = {"STAR MAP", "NAME THIS STAR", "NAME THIS WORLD", "WRITE A NOTE", "EXPEDITION LOG", "GALLERY", "STATISTICS", "THE SHARDS",   // R-408: the sights are marked on the sector map (N), not named here
                             "TARGET A STAR BY NAME", "TARGET BY COORDINATES", "RETURN TO THE PREVIOUS STAR", "TARGET THE HOME STAR",
                             "SET HOME HERE", "EXPORT THE GUIDE", "IMPORT AN INBOX FILE", "BACK"};
const int GUIDE_N = (int)(sizeof(GUIDE_ITEMS) / sizeof(GUIDE_ITEMS[0]));
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
    menuStep(in, guideSel, GUIDE_N);
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_G)) { state = guideReturn; return; }
    if (!enterKey(in)) return;
    bool onSurface = guideReturn == GameState::SURFACE;
    switch (guideSel) {
        case 0: if (!onSurface) openStarMap(); else status("THE STAR MAP IS READ FROM THE SHIP", 3); break;
        case 1: if (sys.valid) beginTextEntry(sys.sunless() ? "NAME THIS PLACE" : "NAME THIS STAR", TEXT_STAR, starNamed(sys.star) ? upper(starNameOf(sys.star)) : std::string()); else status("NO STAR HERE", 3); break;   // D-02: a drifter's place
        case 2: {
            int bi = onSurface && surf.valid ? surf.site.body : ship.localTarget;
            if (sys.valid && bi >= 0 && bi < (int)sys.bodies.size()) { textBody = bi; beginTextEntry("NAME THIS WORLD", TEXT_WORLD, bodyNamed(bi) ? upper(bodyNameOf(bi)) : std::string()); }
            else status("NO LOCAL TARGET TO NAME", 3);
            break;
        }
        case 3: beginTextEntry("NOTE FOR THE LOG", TEXT_NOTE, ""); break;
        case 4: logPage = 0; state = GameState::LOG; break;
        case 5: openGallery(); break;
        case 6: state = GameState::STATS; break;
        case 7: if (!onSurface) openShards(); else status("THE SHARDS ARE READ ON THE SHIP", 3); break;   // C-06
        case 8: if (!onSurface) beginTextEntry("STAR NAME", TEXT_FIND_STAR, ""); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
        case 9: if (!onSurface) beginTextEntry("SECTOR X Y Z", TEXT_SECTOR, ""); else status("TARGETING IS DONE FROM THE SHIP", 3); break;
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
    drawPanel(canvas, 40, top, UW - 40, bottom, rgb(0, 0, 0), 215);
    drawTextCentered(canvas, UW / 2, top + 6, "THE GUIDE", HUD_AMBER);
    for (int i = 0; i < GUIDE_N; i++) {
        int y = y0 + i * 9;
        bool sel = i == guideSel;
        std::string label = GUIDE_ITEMS[i];
        if (i == 2 && sys.valid) { int bi = guideReturn == GameState::SURFACE && surf.valid ? surf.site.body : ship.localTarget; if (bi >= 0 && bi < (int)sys.bodies.size() && bodyNamed(bi)) label = "RENAME " + trunc(upper(bodyNameOf(bi)), 22); }
        if (i == 1 && sys.valid && starNamed(sys.star)) label = "RENAME " + trunc(upper(starNameOf(sys.star)), 22);
        else if (i == 1 && sys.valid && sys.sunless()) label = "NAME THIS PLACE";   // D-02: a drifter's place is the explorer's to name as a system is
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
    if (textKind == TEXT_MARK && in.wasPressed(KEY_TAB)) { textMarkKind = (textMarkKind + 1) % (MARK_PLACE + 1); audio.beep = BEEP_UI; }   // R-408: what the mark marks
    if (in.wasPressed(KEY_ESCAPE)) { state = guideReturn; textMark = -1; textGalaxyMark = -1; return; }
    if (!in.wasPressed(KEY_ENTER)) return;
    std::string text = textBuffer;
    while (!text.empty() && text.back() == ' ') text.pop_back();
    state = guideReturn;
    switch (textKind) {
        case TEXT_STAR:
            if (text.empty() || !sys.valid) return;
            guide.names[starKeyOf(sys.star)] = text; guide.save(guidePath);
            status(fmt(sys.sunless() ? "THE PLACE IS NOW CALLED %s" : "THE STAR IS NOW CALLED %s", text.c_str()), 4);   // D-02
            logEvent("NOTE", (sys.sunless() ? "NAMED THE PLACE WITH NO STAR " : "NAMED THE STAR ") + text);
            break;
        case TEXT_WORLD: {
            if (text.empty() || !sys.valid || textBody < 0 || textBody >= (int)sys.bodies.size()) return;
            guide.names[Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, textBody)] = text; guide.save(guidePath);
            const Body& b = sys.bodies[textBody];   // W-06: the telescope names moons, comets and the second sun too
            const char* what = b.type == PT_COMPANION ? "SECOND SUN" : (b.type == PT_COMET ? "COMET" : (b.parent >= 0 ? "MOON" : "WORLD"));
            status(fmt("THE %s IS NOW CALLED %s", what, text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A %s %s", what, text.c_str()));
            break;
        }
        case TEXT_SKY_STAR: {   // W-06: a star of the neighbourhood seen through the telescope
            if (text.empty()) return;
            guide.names[starKeyOf(textStar)] = text; guide.save(guidePath);
            status(fmt("THE STAR IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", "NAMED A STAR IN THE SKY " + text);
            break;
        }
        case TEXT_NOTE:
            if (text.empty()) return;
            logEvent("NOTE", text);
            status("NOTE WRITTEN IN THE LOG", 3);
            break;
        case TEXT_FIND_STAR: targetStarByName(text); break;
        case TEXT_MARK: finishMark(text); break;   // R-408: a mark placed or renamed (an empty name places an unnamed one)
        case TEXT_PLACE: finishGalaxyMark(text); break;   // G-06: a mark of the galaxy, likewise
        case TEXT_STORM: {   // X-04: the storm a probe fell into (its screen, its recording)
            if (text.empty() || textStormKey.empty()) return;
            guide.names[textStormKey] = text; guide.save(guidePath);
            status(fmt("THE STORM IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A STORM %s", text.c_str()));
            break;
        }
        case TEXT_PIECE: {   // C-04: the piece's name (the shard on the decoder screen)
            if (text.empty() || textShardKey.empty()) return;
            guide.names[textShardKey] = text; guide.save(guidePath);
            status(fmt("THE PIECE IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A PIECE OF THE OLD PEOPLE'S MUSIC %s", text.c_str()));
            break;
        }
        case TEXT_SECTOR: {
            std::istringstream is(text);
            long long x, y, z;
            if (!(is >> x >> y >> z)) { refuse("USE THREE NUMBERS: X Y Z"); return; }
            Star s;
            if (!placeInSector(x, y, z, s, true)) { refuse(fmt("NO STAR IN SECTOR %lld %lld %lld", x, y, z)); return; }   // D-01: or its drifter
            setRemoteStar(s);
            break;
        }
    }
}

void Game::renderTextEntry() {
    const int top = padKeyboardShown() ? 18 : 70, bottom = top + 60;   // R-418: over the controller's keyboard
    drawPanel(canvas, 6, top, UW - 6, bottom, rgb(0, 0, 0), 225);   // R-408: as wide as its last line (it ran past the box)
    drawTextCentered(canvas, UW / 2, top + 8, textPrompt.c_str(), HUD_AMBER);
    std::string shown = textBuffer + (std::fmod(realTime, 0.8) < 0.4 ? "_" : " ");
    drawTextCentered(canvas, UW / 2, top + 26, shown.c_str(), HUD_WHITE);
    if (textKind == TEXT_MARK) {   // R-408: what the mark marks, its glyph on the maps
        std::string k = textMarkKind < LM_COUNT ? fmt("%s %s", LANDMARK_KIND_SYMBOLS[textMarkKind], LANDMARK_KIND_NAMES[textMarkKind]) : std::string("A PLACE");
        drawTextCentered(canvas, UW / 2, top + 38, fmt("MARKS %s - TAB CHANGES", k.c_str()).c_str(), HUD_CYAN);
    }
    if (textKind == TEXT_PLACE) drawTextCentered(canvas, UW / 2, top + 38, trunc(textGalaxyLine, 50).c_str(), HUD_CYAN);   // G-06: the place marked
    drawTextCentered(canvas, UW / 2, bottom - 12, "LETTERS, DIGITS, SPACE, -   ENTER OK   ESC CANCEL", HUD_DIM);
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

// P-00: the world types walked on one line of 48 characters (two spaces in): their short names as far as they go, the count of the
// rest after them; past that the statistics offer a page of their own (K)
std::string Game::statsKindsLine() const {
    std::string all;
    for (int c : guide.typesSeen) all += std::string(shortType(c)) + " ";
    if (all.size() <= 48) return all;
    std::string out; int shown = 0;
    for (int c : guide.typesSeen) {
        std::string next = out + shortType(c) + " ";
        if (next.size() + 4 > 48) break;   // room for "+NN"
        out = next; shown++;
    }
    return out + "+" + std::to_string((int)guide.typesSeen.size() - shown);
}
bool Game::statsKindsOverflow() const {
    size_t n = 0;
    for (int c : guide.typesSeen) n += std::strlen(shortType(c)) + 1;
    return n > 48;
}

void Game::renderStats() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 215);
    int y = 8;
    if (statsKinds) {   // P-00: the world types walked, by name, in three columns (sixty fit)
        drawText(canvas, 12, y, fmt("WORLD TYPES WALKED    %d/%d", (int)guide.typesSeen.size(), landableTypeCount()).c_str(), HUD_AMBER); y += 14;
        int i = 0;
        for (int c : guide.typesSeen) { drawText(canvas, 12 + (i % 3) * 100, y + (i / 3) * 9, KINDS[c].name, HUD_GREEN); i++; }
        drawTextCentered(canvas, UW / 2, UH - 10, "ANY KEY TO CLOSE", HUD_DIM);
        return;
    }
    auto line = [&](const std::string& s, uint32_t c) { drawText(canvas, 12, y, s.c_str(), c); y += 10; };
    auto sub = [&](const std::string& s) { drawText(canvas, 12, y, s.c_str(), HUD_DIM); y += 9; };   // C-12: a dim row under a count, a pixel closer (the page is full)
    line("DISCOVERY STATISTICS", HUD_AMBER);
    line(fmt("SYSTEMS VISITED       %d", (int)guide.visited.size()), HUD_GREEN);
    line(fmt("WORLDS LANDED ON      %d", (int)guide.landed.size()), HUD_GREEN);
    line(fmt("STAR CLASSES SEEN     %d/%d", (int)guide.classesSeen.size(), STAR_DRAWN_COUNT), HUD_GREEN);
    std::string cls; for (int c : guide.classesSeen) cls += std::string(STAR_CLASSES[c].code) + " ";
    { std::vector<std::string> rows = wrapText(cls, 48); for (size_t k = 0; k < rows.size() && k < 2; k++) sub("  " + rows[k]); }   // S-02: the codes on their own line (two past twelve classes), like the world types; ten of them overran the count's line
    line(fmt("WORLD TYPES WALKED    %d/%d", (int)guide.typesSeen.size(), landableTypeCount()), HUD_GREEN);
    sub("  " + statsKindsLine());
    line(fmt("FURTHEST FROM HOME    %.2f LY", guide.furthestFromHomeLY), HUD_GREEN);
    line(fmt("LONGEST WALK          %.2f KM FROM THE CAPSULE", guide.longestWalkM / 1000.0), HUD_GREEN);
    line(fmt("HIGHEST POINT REACHED %.0f M", guide.highestPointM), HUD_GREEN);
    line(fmt("FOOT, DRIVEN, FLOWN   %.1f / %.1f / %.1f KM", guide.totalWalkedM / 1000.0, guide.totalDrivenM / 1000.0, guide.totalFlownM / 1000.0), HUD_GREEN);   // C-03: one line, the shards took the other; R-403: the drone's kilometres
    line(fmt("SHARDS TAKEN          %d FROM %d WORLD%s   DECODED %d", (int)guide.shards.size(), guide.shardWorlds(), guide.shardWorlds() == 1 ? "" : "S", (int)guide.decoded.size()), HUD_GREEN);   // C-03; C-06: read on the ship
    sub(fmt("  GRAVES READ %d   ENDED %d   LENT %d   INSTRUMENTS %d", (int)guide.graves.size(), (int)guide.ended.size(), guide.lentCount(), (int)guide.instruments.size()));   // C-12; C-14: the inbox's shards; C-11: the case
    line(fmt("NAMES GIVEN           %d   FROM OTHERS %d", (int)guide.names.size(), (int)guide.inbox.size()), HUD_GREEN);
    line(fmt("LOG ENTRIES           %d   PHOTOGRAPHS %d", (int)guide.log.size(), guide.screenshots), HUD_GREEN);
    line(fmt("CREATURES SIGHTED     %d SPECIES   SIGNALS HEARD %d", guide.creaturesSeen, (int)guide.signals.size()), HUD_GREEN);   // N3-01; C-07: the radar's locks
    sub(guide.probesSent ? fmt("  PROBES SENT %d, DEEPEST %.0f KM   DRONE TRIPS %d", guide.probesSent, guide.deepestProbeKm(), guide.droneTrips)   // X-04; C-18: the drones (51 characters at most)
                         : fmt("  PROBES SENT 0   DRONE TRIPS %d, %d FINDS", guide.droneTrips, guide.droneFinds));
    line(fmt("LONGEST DRIVE         %.2f KM   TOP SPEED %.0f KM/H", guide.longestDriveM / 1000.0, guide.topSpeedKmh), HUD_GREEN);   // N4-05
    int64_t hx, hy, hz;
    if (!guide.home.empty() && Guide::parseStarKey(guide.home, hx, hy, hz)) { Star h; if (placeInSector(hx, hy, hz, h, true)) line(fmt("HOME STAR             %s (%s)", upper(starNameOf(h)).c_str(), STAR_CLASSES[h.cls].code), HUD_CYAN); }
    drawTextCentered(canvas, UW / 2, UH - 10, statsKindsOverflow() ? "K WORLD TYPES   ANY OTHER KEY CLOSES" : "ANY KEY TO CLOSE", HUD_DIM);
}

// X-04: the photographs and the probes' recordings (each by its final image: shots/probe_<n>.png, a lent one drawn), in the order
// they were taken: a photograph by its file's time, a recording by the time it came back (a lent one: its lending)
void Game::openGallery() {
    pngFlush();   // F-02: a photograph or a probe's image still with the writer is in the directory first
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
        menuStep(in, gallerySel, n, true);
    }
    if (n > 0 && enterKey(in) && galleryItems[gallerySel].rec >= 0) {   // X-04: a recording plays on the ship's screen
        if (guideReturn == GameState::SURFACE) { refuse("THE RECORDINGS PLAY ON THE SHIP'S SCREEN", 3); }
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
