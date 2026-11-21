// GUIDE screens (M3): the guide menu, text entry, the expedition log, statistics and the gallery (the names and what the
// guide keeps are names.cpp, the star map and the targets star_map.cpp).
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
#include <map>
#include <set>
#include <tuple>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------------------
// guide menu
// ---------------------------------------------------------------------------

// U-03: the guide menu's rows. The sights are marked on the sector map (N), not named here; the targets are set from the ship
const RowList<MenuRow>& Game::guideMenu() {
    static constexpr const char* SHIP_TARGETS = "TARGETING IS DONE FROM THE SHIP";
    static constexpr MenuRow ROWS[] = {
        {"STAR MAP", [](Game& g) { g.openScreen(GameState::STAR_MAP); }, "STAR MAP IS READ FROM THE SHIP"},
        {"NAME THIS STAR", [](Game& g) {   // D-02: a drifter's place is the explorer's to name as a system is
             if (!g.sys.valid) { g.status("NO STAR HERE", 3); return; }
             g.closeScreen(); g.beginTextEntry(g.sys.sunless() ? "NAME THIS PLACE" : "NAME THIS STAR", TEXT_STAR, g.starNamed(g.sys.star) ? upper(g.starNameOf(g.sys.star)) : std::string());
         }, nullptr, [](const Game& g) -> std::string {
             if (g.sys.valid && g.starNamed(g.sys.star)) return "RENAME " + trunc(upper(g.starNameOf(g.sys.star)), 22);
             return g.sys.valid && g.sys.sunless() ? "NAME THIS PLACE" : "";
         }},
        {"NAME THIS WORLD", [](Game& g) {
             int bi = g.returnState == GameState::SURFACE && g.surf.valid ? g.surf.site.body : g.ship.localTarget;
             if (!g.sys.valid || bi < 0 || bi >= (int)g.sys.bodies.size()) { g.status("NO LOCAL TARGET TO NAME", 3); return; }
             g.textBody = bi;
             g.closeScreen(); g.beginTextEntry("NAME THIS WORLD", TEXT_WORLD, g.bodyNamed(bi) ? upper(g.bodyNameOf(bi)) : std::string());
         }, nullptr, [](const Game& g) -> std::string {
             if (!g.sys.valid) return "";
             int bi = g.returnState == GameState::SURFACE && g.surf.valid ? g.surf.site.body : g.ship.localTarget;
             return bi >= 0 && bi < (int)g.sys.bodies.size() && g.bodyNamed(bi) ? "RENAME " + trunc(upper(g.bodyNameOf(bi)), 22) : "";
         }},
        {"WRITE A NOTE", [](Game& g) { g.closeScreen(); g.beginTextEntry("NOTE FOR LOG", TEXT_NOTE, ""); }},
        {"EXPEDITION LOG", [](Game& g) { g.openScreen(GameState::LOG); }},
        {"GALLERY", [](Game& g) { g.openScreen(GameState::GALLERY); }},
        {"STATISTICS", [](Game& g) { g.openScreen(GameState::STATS); }},
        {"THE CATALOGUE", [](Game& g) { g.openScreen(GameState::CATALOGUE); }},   // Z-07: the species seen
        {"SHARDS", [](Game& g) { g.openScreen(GameState::SHARDS); }, "SHARDS ARE READ ON THE SHIP"},
        {"TARGET A STAR BY NAME", [](Game& g) { g.closeScreen(); g.beginTextEntry("STAR NAME", TEXT_FIND_STAR, ""); }, SHIP_TARGETS},
        {"TARGET BY COORDINATES", [](Game& g) { g.closeScreen(); g.beginTextEntry("SECTOR X Y Z", TEXT_SECTOR, ""); }, SHIP_TARGETS},
        {"TARGET PREVIOUS STAR", [](Game& g) { if (g.targetPreviousStar()) g.closeScreen(); }, SHIP_TARGETS},
        {"TARGET HOME STAR", [](Game& g) { if (g.targetHome()) g.closeScreen(); }, SHIP_TARGETS},
        {"SET HOME HERE", [](Game& g) {
             if (g.sys.valid) {
                 g.guide.home = g.starKeyOf(g.sys.star); g.guide.save(g.guidePath);
                 g.status(fmt("HOME STAR: %s", upper(g.starNameOf(g.sys.star)).c_str()), 4); g.closeScreen();
             }
         }},
        {"EXPORT THE GUIDE", [](Game& g) {
             std::string p = g.guidePath.substr(0, g.guidePath.rfind('.')) + "_export.txt";
             g.status(g.guide.save(p) ? fmt("GUIDE EXPORTED TO %s", upper(p).c_str()) : "EXPORT FAILED", 5);
             g.closeScreen();
         }},
        {"IMPORT AN INBOX FILE", [](Game& g) { g.importInboxFile(); g.closeScreen(); }},   // the names into the inbox, the friend's read shards lent
        {"BACK", [](Game& g) { g.closeScreen(); }},
    };
    static constexpr RowList<MenuRow> LIST = rowsOf(ROWS);
    static_assert(rowsWhole(LIST), "each row its label and its act");
    return LIST;
}

// U-05: the guide over the scene it is opened from
void Game::guideEnter() {
    guideSel = 0;
    if (state == GameState::SURFACE || state == GameState::SPACE) returnState = state;
}

void Game::beginTextEntry(const std::string& prompt, int kind, const std::string& initial) {
    textPrompt = prompt; textKind = kind; textBuffer = initial;
    openScreen(GameState::TEXT_ENTRY);
}

// the guide menu: Enter does its row's act, or says why the ground refuses it
void Game::updateGuideMenu(const Input& in, double dt, double realDt) {
    const RowList<MenuRow>& m = guideMenu();
    menuStep(in, guideSel, m.count);
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_G)) { closeScreen(); return; }
    if (!enterKey(in) || guideSel < 0 || guideSel >= m.count) return;
    const MenuRow& r = m.row[guideSel];
    if (r.ground && returnState == GameState::SURFACE) status(r.ground, 3);
    else r.act(*this);
}

void Game::renderGuideMenu() {
    const RowList<MenuRow>& m = guideMenu();
    const int top = 10, bottom = UH - 6, y0 = top + 18;   // C-06: a row every 9 px, the box grown with them (the rows had run into the footer)
    drawPanel(canvas, 40, top, UW - 40, bottom, rgb(0, 0, 0), 215);
    drawTextCentered(canvas, UW / 2, top + 6, "GUIDE", HUD_AMBER);
    for (int i = 0; i < m.count; i++) {
        int y = y0 + i * 9;
        bool sel = i == guideSel;
        std::string label = m.row[i].name ? m.row[i].name(*this) : std::string();
        if (label.empty()) label = m.row[i].label;
        drawText(canvas, 56, y, label.c_str(), sel ? HUD_WHITE : HUD_GREEN);
        if (sel) drawText(canvas, 46, y, ">", HUD_AMBER);
    }
    drawTextCentered(canvas, UW / 2, bottom - 10, "UP/DOWN  ENTER  ESC", HUD_DIM);
}

// ---------------------------------------------------------------------------
// text entry
// ---------------------------------------------------------------------------

void Game::updateTextEntry(const Input& in, double dt, double realDt) {
    for (int k = KEY_A; k <= KEY_Z; k++) if (in.wasPressed(k) && textBuffer.size() < 28) textBuffer += (char)('A' + (k - KEY_A));
    for (int k = KEY_0; k <= KEY_9; k++) if (in.wasPressed(k) && textBuffer.size() < 28) textBuffer += (char)('0' + (k - KEY_0));
    if (in.wasPressed(KEY_SPACE) && textBuffer.size() < 28 && !textBuffer.empty()) textBuffer += ' ';
    if (in.wasPressed(KEY_MINUS) && textBuffer.size() < 28) textBuffer += '-';
    if (in.wasPressed(KEY_PERIOD) && textBuffer.size() < 28) textBuffer += '.';
    if (in.wasPressed(KEY_BACKSPACE) && !textBuffer.empty()) textBuffer.pop_back();
    if (textKind == TEXT_MARK && in.wasPressed(KEY_TAB)) { textMarkKind = (textMarkKind + 1) % (MARK_PLACE + 1); audio.beep = BEEP_UI; }   // R-408: what the mark marks
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); textMark = -1; textGalaxyMark = -1; keep.pending = false; return; }
    if (!in.wasPressed(KEY_ENTER)) return;
    std::string text = textBuffer;
    while (!text.empty() && text.back() == ' ') text.pop_back();
    closeScreen();
    switch (textKind) {
        case TEXT_STAR:
            if (text.empty() || !sys.valid) return;
            guide.names[starKeyOf(sys.star)] = text; guide.save(guidePath);
            status(fmt(sys.sunless() ? "THE PLACE IS NOW CALLED %s" : "THE STAR IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", (sys.sunless() ? "NAMED THE PLACE WITH NO STAR " : "NAMED THE STAR ") + text);
            break;
        case TEXT_WORLD: {
            if (text.empty() || !sys.valid || textBody < 0 || textBody >= (int)sys.bodies.size()) return;
            guide.names[Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, textBody)] = text; guide.save(guidePath);
            const Body& b = sys.bodies[textBody];   // the telescope names moons, comets and the second sun too
            const char* what = b.type == PT_COMPANION ? "SECOND SUN" : (b.type == PT_COMET ? "COMET" : (b.parent >= 0 ? "MOON" : "WORLD"));
            status(fmt("THE %s IS NOW CALLED %s", what, text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A %s %s", what, text.c_str()));
            break;
        }
        case TEXT_SKY_STAR: {   // a star of the neighbourhood seen through the telescope
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
        case TEXT_MARK: finishMark(text); break;   // a mark placed or renamed (an empty name places an unnamed one)
        case TEXT_PLACE: finishGalaxyMark(text); break;   // a mark of the galaxy, likewise
        case TEXT_STORM: {   // the storm a probe fell into (its screen, its recording)
            if (text.empty() || textStormKey.empty()) return;
            guide.names[textStormKey] = text; guide.save(guidePath);
            status(fmt("THE STORM IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A STORM %s", text.c_str()));
            break;
        }
        case TEXT_SPECIES: {   // Z-04: the species under the microscope's crosshair; R-442: a flask's
            if (text.empty() || textSpeciesKey.empty()) return;
            guide.names[textSpeciesKey] = text; guide.save(guidePath);
            for (int i = 0; i < INCUBATOR_FLASKS; i++) lab.flasks[i].made = false;   // the flasks' lines say its name
            flaskMadeAll();
            lab.seq.made = false;   // B-443: the sequencer's title and lines too
            status(fmt("THE SPECIES IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A SPECIES %s", text.c_str()));
            break;
        }
        case TEXT_ANIMAL: {   // J-07: the animal species the crosshair rested on
            if (text.empty() || textAnimalKey.empty()) return;
            guide.names[textAnimalKey] = text; guide.save(guidePath);
            status(fmt("THE ANIMAL IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED AN ANIMAL %s", text.c_str()));
            break;
        }
        case TEXT_STRAIN: {   // Z-05: a strain in the incubator's flask
            if (text.empty() || textStrainKey.empty()) return;
            guide.names[textStrainKey] = text; guide.save(guidePath);
            for (int i = 0; i < INCUBATOR_FLASKS; i++) lab.flasks[i].made = false;   // its lines say its name
            flaskMadeAll();
            status(fmt("THE STRAIN IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A STRAIN %s", text.c_str()));
            break;
        }
        case TEXT_GENE: {   // Y-02: a gene of a genesis, on the gene editor (every kin's)
            if (text.empty() || textGeneKey.empty()) return;
            guide.names[textGeneKey] = text; guide.save(guidePath);
            status(fmt("THE GENE IS NOW CALLED %s", text.c_str()), 4);
            logEvent("NOTE", fmt("NAMED A GENE %s", text.c_str()));
            break;
        }
        case TEXT_FOLDER: {   // R-433: a folder of the culture collection made (and chosen on the microscope's cultures) or renamed
            const bool keeping = keep.pending;   // R-440: made for a copy K keeps, kept in it
            keep.pending = false;
            if (text.empty()) return;
            if (textFolderId >= 0) { folderRename(textFolderId, text); break; }
            const int id = folderNew(text);
            if (id >= 0 && keeping) { if (keep.write) editorWriteName(id); else keptAdd(keep.what, id); break; }   // R-445: a new line's folder
            for (size_t f = 0; f < lab.folders.size(); f++) if (id >= 0 && lab.folders[f].id == id) { scope.tab = 1; scope.open = false; scope.folder = 2 + (int)f; }
            break;
        }
        case TEXT_LINE:   // R-445: the name of the strain the gene editor writes; then the writing begins
            if (text.empty()) { refuse("TYPE A NAME FOR THE NEW STRAIN", 3); beginTextEntry("NAME THE NEW STRAIN", TEXT_LINE, ""); return; }
            editorWriteBegin(text);
            break;
        case TEXT_PIECE: {   // the piece's name (the shard on the decoder screen)
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
            if (!placeInSector(x, y, z, s, true)) { refuse(fmt("NO STAR IN SECTOR %lld %lld %lld", x, y, z)); return; }   // or its drifter
            setRemoteStar(s);
            break;
        }
    }
}

// U-04: a text entry's commands (the controller's first page)
void Game::textEntryPad(PadMenu& m) const {
    m.title = "ENTRY";
    if (!padKeyboardShown()) m.item("KEYBOARD", -1, 0, PSP_KEYBOARD);
    m.item("DONE", KEY_ENTER); m.item("CANCEL", KEY_ESCAPE);
    if (textKind == TEXT_MARK) m.item("MARKS: NEXT", KEY_TAB);
}

// a text entry covers the frame over the galaxy chart, the probe's screen, a recording or the microscope (its opener), drawn under it
bool Game::textEntryCovers() const {
    const GameState from = openedFrom[(int)GameState::TEXT_ENTRY];
    return (from == GameState::STAR_MAP && mapPage == 1) || from == GameState::PROBE || from == GameState::RECORDING || from == GameState::MICROSCOPE;
}

void Game::renderTextEntry() {
    // over what it names: a mark on the galaxy chart, a storm on the probe's screen or a recording, a piece on the decoder; R-408: a
    // mark on the sector map (from the guide menu or the star map's first page, over the scene alone); Z-04 a species, Z-05 a strain
    const GameState from = openedFrom[(int)GameState::TEXT_ENTRY];   // U-05: its opener
    if (from == GameState::STAR_MAP && mapPage == 1) renderStarMap();
    else if (from == GameState::PROBE) renderProbe();
    else if (from == GameState::RECORDING) renderRecording();
    else if (from == GameState::SHARDS) renderShards();
    else if (from == GameState::SECTOR_MAP) renderSectorMap();
    else if (from == GameState::MICROSCOPE) renderMicroscope();
    else if (from == GameState::INCUBATOR) renderIncubator();   // Z-05: a strain's name over its flask
    else if (from == GameState::CATALOGUE) renderCatalogue();   // Z-07: a species' name over its row or its sheet
    else if (from == GameState::EDITOR) renderEditor();         // R-445: a new line's name, a gene's
    const int top = padKeyboardShown() ? 18 : 70, bottom = top + 60;   // R-418: over the controller's keyboard
    drawPanel(canvas, 6, top, UW - 6, bottom, rgb(0, 0, 0), 225);   // as wide as its last line (it ran past the box)
    drawTextCentered(canvas, UW / 2, top + 8, textPrompt.c_str(), HUD_AMBER);
    std::string shown = textBuffer + (std::fmod(realTime, 0.8) < 0.4 ? "_" : " ");
    drawTextCentered(canvas, UW / 2, top + 26, shown.c_str(), HUD_WHITE);
    if (textKind == TEXT_MARK) {   // what the mark marks, its glyph on the maps
        std::string k = textMarkKind < LM_COUNT ? fmt("%s %s", LANDMARK_KIND_SYMBOLS[textMarkKind], LANDMARK_KIND_NAMES[textMarkKind]) : std::string("A PLACE");
        drawTextCentered(canvas, UW / 2, top + 38, fmt("MARKS %s - TAB CHANGES", k.c_str()).c_str(), HUD_CYAN);
    }
    if (textKind == TEXT_PLACE) drawTextCentered(canvas, UW / 2, top + 38, trunc(textGalaxyLine, 50).c_str(), HUD_CYAN);   // the place marked
    drawTextCentered(canvas, UW / 2, bottom - 12, "LETTERS, DIGITS, SPACE, -   ENTER OK   ESC CANCEL", HUD_DIM);
}

// ---------------------------------------------------------------------------
// log, statistics, gallery
// ---------------------------------------------------------------------------

void Game::logEnter() { logPage = 0; }   // U-05: the log opens on its first page

void Game::updateLog(const Input& in, double dt, double realDt) {
    int pages = std::max(1, ((int)guide.log.size() + 6) / 7);
    if (in.wasPressed(KEY_DOWN) || in.wasPressed(KEY_PAGE_DOWN) || in.wasPressed(KEY_RIGHT)) logPage = std::min(pages - 1, logPage + 1);
    if (in.wasPressed(KEY_UP) || in.wasPressed(KEY_PAGE_UP) || in.wasPressed(KEY_LEFT)) logPage = std::max(0, logPage - 1);
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_J) || enterKey(in)) closeScreen();
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

// the statistics: any key closes; P-00: K opens the world types walked where they outgrow their line; Z-07: L the LIFE page (the tree built
// for its count of geneses)
void Game::updateStats(const Input& in, double dt, double realDt) {
    bool any = false; for (int k = 0; k < KEY_MAX; k++) if (in.pressed[k]) any = true;
    if (any && !statsKinds && !statsLife && in.pressed[KEY_K] && statsKindsOverflow()) statsKinds = true;
    else if (any && !statsKinds && !statsLife && in.pressed[KEY_L]) { statsLife = true; treeBuild(); }
    else if (any) { statsKinds = statsLife = false; closeScreen(); }
}

// Z-07: the LIFE page: what was found of the living small, the explorer's own (a friend's lent in no count but its own line), in the other
// screens' words: the species the catalogue's rows, their forms and stages their sheets', the woken the flasks' words, the geneses the tree's
void Game::renderStatsLife() {
    int y = 8;
    auto line = [&](const std::string& s, uint32_t c) { drawText(canvas, 12, y, s.c_str(), c); y += 10; };
    auto sub = [&](const std::string& s) { for (const std::string& w : wrapText(s, 48)) { drawText(canvas, 12, y, ("  " + w).c_str(), HUD_DIM); y += 9; } };
    auto plural = [](int n, const char* one, const char* many) { return fmt("%d %s", n, n == 1 ? one : many); };
    line("LIFE STATISTICS", HUD_AMBER);
    line("SAMPLES TAKEN         " + fmt("%d ON ", guide.samplesTaken) + plural((int)guide.sampled.size(), "WORLD", "WORLDS"), HUD_GREEN);
    std::set<std::string> worlds;
    int named = 0, woken = 0, read = 0;
    uint32_t forms = 0, stages = 0;
    double slept = 0;
    for (const auto& kv : guide.microbes) {
        worlds.insert(kv.first.substr(0, kv.first.rfind("/M")));
        if (guide.names.count(kv.first)) named++;
        const MicrobeSheet& m = kv.second;
        forms |= m.forms; stages |= m.stages;
        if (m.stages & LC_WAKING) { woken++; slept = std::max(slept, m.sleptYears); }
        if (m.seen & SEEN_KIN) read++;
    }
    line(fmt("SPECIES SEEN          %d ON ", (int)guide.microbes.size()) + plural((int)worlds.size(), "WORLD", "WORLDS") + fmt(", %d NAMED", named), HUD_GREEN);
    if (forms) sub("SEEN AS: " + formsSeenWords(forms));
    if (stages && !stagesSeenWords(stages, false).empty()) sub("STAGES SEEN: " + stagesSeenWords(stages, false));
    line(fmt("WOKEN FROM SPORES     %d", woken), HUD_GREEN);
    if (slept > 0) sub("LONGEST AFTER " + yearsWords(slept));
    int strains = 0, strainsNamed = 0, edited = 0, genesKnown = 0;   // Y-02: the edited lines apart from the strains, the genes known
    for (const auto& kv : guide.strains) {
        if (kv.second.what == SM_EDIT) { edited++; continue; }
        strains++;
        if (guide.names.count(kv.first)) strainsNamed++;
    }
    for (const auto& kv : guide.genes) genesKnown += kv.second.kind >= 0;
    line(fmt("STRAINS               %d, %d NAMED", strains, strainsNamed), HUD_GREEN);
    if (edited || genesKnown) line(fmt("EDITED STRAINS        %d, ", edited) + plural(genesKnown, "GENE KNOWN", "GENES KNOWN"), HUD_GREEN);
    line(fmt("SEQUENCED             %d", read) + (guide.watermarks.empty() ? std::string() : fmt(", %d MADE BY A PEOPLE", (int)guide.watermarks.size())), HUD_GREEN);
    if (read > 0) sub(tree.geneses == 1 ? std::string("LIFE BEGAN AT LEAST ONCE") : fmt("LIFE BEGAN AT LEAST %d TIMES", tree.geneses));
    {   // kin across stars: the species read whose kin were read at another star (the tree's leaves on worlds), the farthest two stars apart
        int across = 0; double farLy = 0;
        std::map<std::tuple<int64_t, int64_t, int64_t>, Vec3> at;
        auto where = [&](const TreeLeaf& L) -> Vec3 {
            const auto k = std::make_tuple(L.sx, L.sy, L.sz);
            auto it = at.find(k);
            if (it != at.end()) return it->second;
            Star st; const Vec3 p = placeInSector(L.sx, L.sy, L.sz, st, true) ? st.pos : Vec3();
            return at[k] = p;
        };
        for (size_t i = 0; i < tree.leaves.size(); i++) {
            const TreeLeaf& A = tree.leaves[i];
            if (A.body < 0) continue;
            bool kin = false;
            for (size_t j = 0; j < tree.leaves.size(); j++) {
                const TreeLeaf& B = tree.leaves[j];
                if (j == i || B.body < 0 || (A.sx == B.sx && A.sy == B.sy && A.sz == B.sz) || commonAncestorGyr(A.line, B.line) < 0) continue;
                kin = true;
                farLy = std::max(farLy, length(where(A) - where(B)) / SECTOR_KM);
            }
            across += kin;
        }
        line(fmt("KIN ACROSS STARS      ") + plural(across, "SPECIES", "SPECIES"), HUD_GREEN);
        if (across > 0) sub(fmt("FARTHEST %.1f LY APART", farLy));
    }
    int lentN = 0;
    for (const auto& kv : guide.lentMicrobes) if (!guide.microbes.count(kv.first)) lentN++;
    if (lentN > 0) line(fmt("LENT BY INBOX         %d SPECIES, IN CATALOGUE", lentN), HUD_CYAN);
    drawTextCentered(canvas, UW / 2, UH - 10, "ANY KEY TO CLOSE", HUD_DIM);
}

void Game::renderStats() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 215);
    int y = 8;
    if (statsLife) { renderStatsLife(); return; }
    if (statsKinds) {   // P-00: the world types walked, by name, in three columns (sixty fit)
        drawText(canvas, 12, y, fmt("WORLD TYPES WALKED    %d/%d", (int)guide.typesSeen.size(), landableTypeCount()).c_str(), HUD_AMBER); y += 14;
        int i = 0;
        for (int c : guide.typesSeen) { drawText(canvas, 12 + (i % 3) * 100, y + (i / 3) * 9, KINDS[c].name, HUD_GREEN); i++; }
        drawTextCentered(canvas, UW / 2, UH - 10, "ANY KEY TO CLOSE", HUD_DIM);
        return;
    }
    auto line = [&](const std::string& s, uint32_t c) { drawText(canvas, 12, y, s.c_str(), c); y += 10; };
    auto sub = [&](const std::string& s) { drawText(canvas, 12, y, s.c_str(), HUD_DIM); y += 9; };   // a dim row under a count, a pixel closer (the page is full)
    line("DISCOVERY STATISTICS", HUD_AMBER);
    line(fmt("SYSTEMS VISITED       %d", (int)guide.visited.size()), HUD_GREEN);
    line(fmt("WORLDS LANDED ON      %d", (int)guide.landed.size()), HUD_GREEN);
    line(fmt("STAR CLASSES SEEN     %d/%d", (int)guide.classesSeen.size(), STAR_DRAWN_COUNT), HUD_GREEN);
    std::string cls; for (int c : guide.classesSeen) cls += std::string(STAR_CLASSES[c].code) + " ";
    // the codes on their own line (two past twelve classes), like the world types; ten of them overran the count's line
    { std::vector<std::string> rows = wrapText(cls, 48); for (size_t k = 0; k < rows.size() && k < 2; k++) sub("  " + rows[k]); }
    line(fmt("WORLD TYPES WALKED    %d/%d", (int)guide.typesSeen.size(), landableTypeCount()), HUD_GREEN);
    sub("  " + statsKindsLine());
    line(fmt("FURTHEST FROM HOME    %.2f LY", guide.furthestFromHomeLY), HUD_GREEN);
    line(fmt("LONGEST WALK          %.2f KM FROM CAPSULE", guide.longestWalkM / 1000.0), HUD_GREEN);
    line(fmt("HIGHEST POINT REACHED %.0f M", guide.highestPointM), HUD_GREEN);
    // one line, the shards took the other; R-403: the drone's kilometres
    line(fmt("FOOT, DRIVEN, FLOWN   %.1f / %.1f / %.1f KM", guide.totalWalkedM / 1000.0, guide.totalDrivenM / 1000.0, guide.totalFlownM / 1000.0), HUD_GREEN);
    // C-03; C-06: read on the ship
    line(fmt("SHARDS TAKEN          %d FROM %d WORLD%s   DECODED %d", (int)guide.shards.size(), guide.shardWorlds(), guide.shardWorlds() == 1 ? "" : "S", (int)guide.decoded.size()), HUD_GREEN);
    // C-12; C-14: the inbox's shards; C-11: the case
    sub(fmt("  GRAVES READ %d   ENDED %d   LENT %d   INSTRUMENTS %d", (int)guide.graves.size(), (int)guide.ended.size(), guide.lentCount(), (int)guide.instruments.size()));
    line(fmt("NAMES GIVEN           %d   FROM OTHERS %d", (int)guide.names.size(), (int)guide.inbox.size()), HUD_GREEN);
    line(fmt("LOG ENTRIES           %d   PHOTOGRAPHS %d", (int)guide.log.size(), guide.screenshots), HUD_GREEN);
    line(fmt("CREATURES SIGHTED     %d SPECIES   SIGNALS HEARD %d", guide.creaturesSeen, (int)guide.signals.size()), HUD_GREEN);   // N3-01; C-07: the radar's locks
    sub(guide.probesSent ? fmt("  PROBES SENT %d, DEEPEST %.0f KM   DRONE TRIPS %d", guide.probesSent, guide.deepestProbeKm(), guide.droneTrips)   // X-04; C-18: the drones (51 characters at most)
                         : fmt("  PROBES SENT 0   DRONE TRIPS %d, %d FINDS", guide.droneTrips, guide.droneFinds));
    line(fmt("LONGEST DRIVE         %.2f KM   TOP SPEED %.0f KM/H", guide.longestDriveM / 1000.0, guide.topSpeedKmh), HUD_GREEN);
    int64_t hx, hy, hz;
    if (!guide.home.empty() && Guide::parseStarKey(guide.home, hx, hy, hz)) {
        Star h;
        if (placeInSector(hx, hy, hz, h, true)) line(fmt("HOME STAR             %s (%s)", upper(starNameOf(h)).c_str(), STAR_CLASSES[h.cls].code), HUD_CYAN);
    }
    drawTextCentered(canvas, UW / 2, UH - 10, statsKindsOverflow() ? "L LIFE   K WORLD TYPES   ANY OTHER KEY CLOSES" : "L LIFE   ANY OTHER KEY CLOSES", HUD_DIM);   // Z-07
}

// X-04: the photographs and the probes' recordings (each by its final image: shots/probe_<n>.png, a lent one drawn), in the order
// they were taken: a photograph by its file's time, a recording by the time it came back (a lent one: its lending)
void Game::galleryEnter() {
    pngFlush();   // a photograph or a probe's image still with the writer is in the directory first
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
}

void Game::updateGallery(const Input& in, double dt, double realDt) {
    int n = (int)galleryItems.size();
    if (n > 0) {
        menuStep(in, gallerySel, n, true);
    }
    if (n > 0 && enterKey(in) && galleryItems[gallerySel].rec >= 0) {   // X-04: a recording plays on the ship's screen
        if (returnState == GameState::SURFACE) { refuse("RECORDINGS PLAY ON THE SHIP", 3); }
        else openRecording(galleryItems[gallerySel].rec);
        return;
    }
    if (in.wasPressed(KEY_ESCAPE) || enterKey(in)) closeScreen();
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
