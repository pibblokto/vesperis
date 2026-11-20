// R-432 and R-433 (docs/iteration-3/plans/requests/R-432-the-vials-by-world.md, R-433-the-culture-collection.md; lab_state.h): the laboratory's
// lists in folders and the culture collection. The microscope's vials are a folder a world (in the cold store's order), its cultures the flasks'
// folder and the collection's; the incubator's and the sequencer's choosers take the same folders. The collection keeps every species and strain
// grown in a flask (its curve past its lag), without limit and after its flask is emptied: an organism as regular as a world's, put on the stage,
// into a flask or on the sequencer, kept in the explorer's folders (N makes one, R renames it, M moves a culture, Backspace removes an empty one)
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

// a world's place in a folder's head: its star's sector and its body, as the player's worlds say it ("  (+195 +0 +46 / 3)")
std::string labFolderWhere(const std::string& key) {
    int64_t sx = 0, sy = 0, sz = 0;
    Guide::parseStarKey(key, sx, sy, sz);
    const size_t sl = key.find('/');
    return fmt("  (%+lld %+lld %+lld / %d)", (long long)sx, (long long)sy, (long long)sz, sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1));
}

// the cold store's vials by world, in the store's order (a world where its first vial is): each its name (the explorer's, a friend's in cyan,
// else UNKNOWN), its vials' count and how many read STRONG
std::vector<LabFolder> Game::vialFolders() const {
    std::vector<LabFolder> out;
    std::map<std::string, int> at;
    for (int i = 0; i < (int)coldStore.size(); i++) {
        const Vial& v = coldStore[i];
        const std::string key = Guide::bodyKey(v.sx, v.sy, v.sz, v.body);
        auto it = at.find(key);
        if (it == at.end()) {
            it = at.emplace(key, (int)out.size()).first;
            LabFolder f; f.key = key; out.push_back(f);
        }
        out[it->second].rows.push_back(i);
    }
    for (LabFolder& f : out) {
        f.name = upper(worldNameOfKey(f.key)); f.foreign = nameIsForeign(f.key);
        int strong = 0;
        for (int i : f.rows) strong += coldStore[i].activity == VA_STRONG;
        f.line = fmt("%d VIAL%s - %d HIGH ACTIVITY", (int)f.rows.size(), f.rows.size() == 1 ? "" : "S", strong);
    }
    return out;
}

// the cultures' folders: the flasks with a culture, UNSORTED (where the collection's new ones go), then the explorer's in the order made
std::vector<LabFolder> Game::cultureFolders() const {
    std::vector<LabFolder> out(2 + lab.folders.size());
    out[0].name = "FLASKS"; out[0].id = FOLDER_FLASKS;
    for (int i = 0; i < INCUBATOR_FLASKS; i++) if (lab.flasks[i].n > 0) out[0].rows.push_back(i);
    out[1].name = "UNSORTED"; out[1].id = 0;
    for (size_t f = 0; f < lab.folders.size(); f++) { out[2 + f].name = lab.folders[f].name; out[2 + f].id = lab.folders[f].id; }
    for (int k = 0; k < (int)lab.kept.size(); k++) {
        size_t at = 1;
        for (size_t f = 0; f < lab.folders.size(); f++) if (lab.folders[f].id == lab.kept[k].folder) at = 2 + f;
        out[at].rows.push_back(k);
    }
    for (LabFolder& f : out) f.line = f.rows.empty() ? std::string("EMPTY") : fmt("%d CULTURE%s", (int)f.rows.size(), f.rows.size() == 1 ? "" : "S");
    return out;
}

// ---- the collection ----------------------------------------------------------------------------------------------------------------------

// R-440: two kept cultures the same: one species (its strain) alone, or the same two together either way round
bool sameCulture(const KeptCulture& a, const KeptCulture& b) {
    auto same = [](const FlaskSpecies& x, const FlaskSpecies& y) { return x.sameSpecies(y) && x.strain == y.strain; };
    if (a.paired != b.paired) return false;
    if (!a.paired) return same(a.who, b.who);
    return (same(a.who, b.who) && same(a.partner, b.partner)) || (same(a.who, b.partner) && same(a.partner, b.who));
}

// a flask's species grown: kept, unless the collection holds it already (R-440: a pair once, either way round); said, and the flask told (its
// next strain kept in its turn)
void Game::keepCulture(int i, int k) {
    Flask& F = lab.flasks[i];
    F.told |= FT_KEPT << k;
    KeptCulture c;
    c.who = F.who[k]; c.who.strainIn = c.who.strain;
    c.medium = F.medium; c.asWorld = F.asWorld; c.t = lab.clock;
    c.plan = k < (int)F.forms.size() ? F.forms[k].plan : -1;
    CultureKind kind; Morph form; double slept = 0;   // B-441: grown on its partner's gift (alone it starves there): kept with it
    if (F.n == 2 && flaskKind(c.who, kind, form, slept) && growthIn(kind, F.medium).fit != GF_GROWS && growthIn(kind, F.medium, true).fit == GF_GROWS) {
        c.paired = true; c.partner = F.who[1 - k]; c.partner.strainIn = c.partner.strain;
        c.partnerPlan = 1 - k < (int)F.forms.size() ? F.forms[1 - k].plan : -1;
    }
    for (const KeptCulture& o : lab.kept) if (sameCulture(o, c)) return;
    lab.kept.push_back(c);
    status(fmt(c.paired ? "FLASK %d: %s GROWS - KEPT WITH ITS PARTNER" : "FLASK %d: %s GROWS - KEPT IN THE COLLECTION", i + 1, F.label[k].c_str()), 5);
}

// R-440: a copy kept by hand in a folder (0 UNSORTED), refused where that folder holds it already; said
bool Game::keptAdd(KeptCulture c, int folder) {
    c.folder = folder; c.t = lab.clock;
    for (const KeptCulture& o : lab.kept)
        if (o.folder == folder && sameCulture(o, c)) { refuse("FOLDER " + folderName(folder) + " ALREADY HAS IT", 3); return false; }
    lab.kept.push_back(c);
    status(keptName((int)lab.kept.size() - 1) + " KEPT IN " + folderName(folder), 4);
    audio.beep = BEEP_UI;
    return true;
}
void Game::keptCopy(int k, int folder) { if (k >= 0 && k < (int)lab.kept.size()) keptAdd(lab.kept[k], folder); }
std::string Game::folderName(int id) const {
    for (const CultureFolder& f : lab.folders) if (f.id == id) return f.name;
    return "UNSORTED";
}

// R-440: K's chooser of the folder a copy goes in (UNSORTED, the explorer's, A NEW FOLDER: its name typed, the copy kept in it once made)
void Game::keepOpen(const KeptCulture& c) {
    keep.open = true; keep.pending = false; keep.write = false; keep.what = c; keep.sel = 0;
    audio.beep = BEEP_UI;
}
bool Game::keepKeys(const Input& in) {
    if (!keep.open) return false;
    const int n = 2 + (int)lab.folders.size();
    if (in.wasPressed(KEY_ESCAPE)) { keep.open = false; audio.beep = BEEP_UI; return true; }
    menuStep(in, keep.sel, n);
    if (in.wasPressed(KEY_N) && !in.ctrl()) keep.sel = n - 1;
    if (!enterKey(in) && !(in.wasPressed(KEY_N) && !in.ctrl())) return true;
    if (keep.sel == n - 1) {
        keep.open = false; keep.pending = true; textFolderId = -1;
        beginTextEntry("NAME THE NEW FOLDER", TEXT_FOLDER, "");
        return true;
    }
    const int id = keep.sel == 0 ? 0 : lab.folders[clampi(keep.sel - 1, 0, n - 3)].id;
    if (keep.write) { keep.open = false; editorWriteName(id); return true; }   // R-445: then the line's name
    if (keptAdd(keep.what, id)) keep.open = false;
    return true;
}
// its panel over the screen: what is kept, the folders (one holding it marked), A NEW FOLDER
void Game::renderKeepPick(uint32_t colour) {
    if (!keep.open) return;
    constexpr int rowH = 9;
    const int n = 2 + (int)lab.folders.size(), shown = std::min(12, n), x0 = 40, x1 = UW - 40, h = shown * rowH + 40, y0 = (UH - h) / 2;
    drawPanel(canvas, x0, y0, x1, y0 + h, rgb(0, 0, 0), 235, colour);
    KeptCulture& w = keep.what;
    std::string what = keptWordsOf(w);
    drawText(canvas, x0 + 8, y0 + 6, fitText(keep.write ? std::string("WHICH FOLDER FOR THE NEW STRAIN?") : "KEEP " + what + " IN:", x1 - x0 - 16).c_str(), colour);
    keep.sel = clampi(keep.sel, 0, n - 1);
    const int first = std::max(0, std::min(keep.sel - shown / 2, n - shown));
    for (int i = first; i < n && i < first + shown; i++) {
        const int y = y0 + 20 + (i - first) * rowH, id = i == 0 ? 0 : (i < n - 1 ? lab.folders[i - 1].id : -1);
        bool there = false;
        for (const KeptCulture& o : lab.kept) there |= id >= 0 && o.folder == id && sameCulture(o, w);
        const std::string name = i == n - 1 ? std::string("NEW FOLDER") : folderName(id);
        if (i == keep.sel) drawText(canvas, x0 + 6, y, ">", colour);
        const char* mark = !there ? "" : (keep.write ? "  (PARENT HERE)" : "  (KEPT THERE)");
        drawText(canvas, x0 + 14, y, fitText(name + mark, x1 - x0 - 22).c_str(), i == keep.sel ? HUD_WHITE : (i == n - 1 ? HUD_AMBER : HUD_GREEN));
    }
    drawTextCentered(canvas, (x0 + x1) / 2, y0 + h - 12, keep.write ? "ENTER CHOOSE  N NEW FOLDER  ESC CANCEL" : "ENTER KEEP  N NEW FOLDER  ESC CANCEL", HUD_DIM);
}

// a kept culture's words: its species' (or its strain's) name, else what it looks like and an unnamed strain's number (B-442); a pair's two
// (R-440: "A ROD + A SPHERE")
std::string Game::keptName(int k) { return k >= 0 && k < (int)lab.kept.size() ? keptWordsOf(lab.kept[k]) : std::string(); }
std::string Game::keptWordsOf(KeptCulture& c) const {
    auto words = [&](const FlaskSpecies& w, int& plan) {
        const std::string n = flaskSpeciesName(w);
        if (n.compare(0, 7, "UNNAMED") != 0) return n;
        if (plan < 0) { CultureKind kind; Morph form; double slept = 0; if (flaskKind(w, kind, form, slept)) plan = form.plan; }   // looked up once
        return (plan >= 0 ? std::string(planWords(plan)) : std::string("A CULTURE")) + (w.strain > 0 ? " " + lineTag(w) : std::string());
    };
    const std::string one = words(c.who, c.plan);
    return c.paired ? one + " + " + words(c.partner, c.partnerPlan) : one;
}

// a drop of a kept culture on the stage, as a young culture of it shows (its cells growing, a tenth of their crowd, more of them dividing) in
// the medium it grew in; its strain's moves on its values and its form
bool Game::microscopeStageKept(int k) {
    if (k < 0 || k >= (int)lab.kept.size()) return false;
    const KeptCulture& c = lab.kept[k];
    std::vector<FlaskSpecies> who = {c.who};
    if (c.paired) who.push_back(c.partner);   // B-441: with what fed it
    Biota B;
    B.stage = LS_SIMPLE;
    Sample S;
    std::vector<Morph> forms;
    scope.species.clear(); scope.keys.clear(); scope.worlds.clear();
    for (size_t i = 0; i < who.size(); i++) {
        CultureKind kind; Morph form; double slept = 0;
        MicrobeGenes genes;
        if (!flaskKind(who[i], kind, form, slept, &genes)) { refuse("ITS WORLD NO LONGER EXISTS"); return false; }
        if (i == 0) B.chemistry = kind.chemistry;
        const Growth g = growthIn(kind, c.medium, c.paired);
        if (!kind.switches.empty()) {   // Y-04: a switched line as a young culture of it is (lit or not, the cold, a tenth of its crowd)
            uint32_t st = (c.medium.light != CL_DARK ? CST_LIGHT : 0u) | (c.medium.tempC < kind.s.tOpt ? CST_COLD : 0u);
            if (kind.quorum <= 0.1) st |= CST_CROWDED;
            if (g.fit != GF_GROWS && g.fit != GF_COLD) st |= CST_STARVING;
            switchedLine(kind, form, genes, st);
        }
        Microbe m = kind.s; m.index = (int)i; m.state = SS_LIVING;
        B.species.push_back(m); scope.species.push_back(m); forms.push_back(form);
        scope.keys.push_back(flaskSpeciesKey(who[i]));
        scope.worlds.push_back(nameOfKey(Guide::bodyKey(who[i].sx, who[i].sy, who[i].sz, who[i].body)));
        Held h; h.species = (int)i; h.state = SS_LIVING; h.cells = std::pow(10.0, std::max(5.0, g.crowdLog - 1)); h.active = g.fit == GF_GROWS; h.dividing = 0.25;
        h.alive = g.fit == GF_HOT || g.fit == GF_SALT || g.fit == GF_SOLVENT ? 0 : 1;   // B-441, R-438: a hungry cell lives on the slide
        S.held.push_back(h);
    }
    Microhabitat hab;
    hab.niche = NI_LIQUID; hab.meanC = c.medium.tempC; hab.salt = c.medium.salt;
    hab.place = mix64(hashCombine(fnv1a(flaskStrainKey(c.who)), 0x4B3E7C0DULL));
    scope.forms = forms;
    scope.sample = S;
    scope.mount = mountOf(B, scope.forms, S, hab);
    cellRange(c.medium.cell, scope.stageLo, scope.stageHi);
    scope.vial = -1; scope.flask = -1; scope.kept = k; scope.life.reset(); scope.body = -1; scope.lineState[0] = scope.lineState[1] = -1;
    scope.title = flaskSpeciesName(c.who);
    scope.line = fmt("COLLECTION, %.0f C", c.medium.tempC);
    microscopeView();
    return true;
}

// N: a folder made (its name typed, none of the same name), its id the next after the highest; R: renamed; Backspace: removed once empty
int Game::folderNew(const std::string& name) {
    if (name.empty()) return -1;
    if (name == "UNSORTED" || name == "FLASKS") { refuse("THAT FOLDER ALREADY EXISTS", 3); return -1; }
    for (const CultureFolder& f : lab.folders) if (f.name == name) { refuse("A FOLDER WITH THAT NAME EXISTS", 3); return -1; }
    int id = 1;
    for (const CultureFolder& f : lab.folders) id = std::max(id, f.id + 1);
    lab.folders.push_back({id, name});
    status("FOLDER " + name + " MADE - M MOVES A CULTURE INTO IT", 4);
    return id;
}
bool Game::folderRename(int id, const std::string& name) {
    if (name.empty() || name == "UNSORTED" || name == "FLASKS") return false;
    for (const CultureFolder& f : lab.folders) if (f.name == name && f.id != id) { refuse("A FOLDER WITH THAT NAME EXISTS", 3); return false; }
    for (CultureFolder& f : lab.folders)
        if (f.id == id) { f.name = name; status("FOLDER RENAMED " + name, 3); return true; }
    return false;
}
bool Game::folderRemove(int id) {
    for (const KeptCulture& c : lab.kept) if (c.folder == id) { refuse("ONLY EMPTY FOLDERS CAN BE REMOVED - M MOVES CULTURES OUT", 3); return false; }
    for (size_t f = 0; f < lab.folders.size(); f++)
        if (lab.folders[f].id == id) {
            status("FOLDER " + lab.folders[f].name + " REMOVED", 3);
            lab.folders.erase(lab.folders.begin() + f);
            return true;
        }
    return false;
}
void Game::keptMove(int k, int id) {
    if (k < 0 || k >= (int)lab.kept.size()) return;
    std::string to = "UNSORTED";
    for (const CultureFolder& f : lab.folders) if (f.id == id) to = f.name;
    lab.kept[k].folder = to == "UNSORTED" ? 0 : id;
    status(keptName(k) + " MOVED TO " + to, 3);
}
// Backspace: a kept culture thrown out (a flask still growing it keeps it no more: its strain comes back only as a new flask grows it)
void Game::keptDiscard(int k) {
    if (k < 0 || k >= (int)lab.kept.size()) return;
    status(keptName(k) + " DISCARDED FROM THE COLLECTION", 3);
    lab.kept.erase(lab.kept.begin() + k);
    if (scope.kept == k) scope.kept = -1; else if (scope.kept > k) scope.kept--;
}

// the save's lines (the lab's, after the jars): `keptfolder <id> <name>`, then `kept <folder> <sector> <body> <species> <strain> <medium>
// <AS ITS WORLD> <t>` (a medium its cell, warmth, light, energy and salt)
void Game::keptSave(std::ostream& f) const {
    for (const CultureFolder& d : lab.folders) f << "keptfolder " << d.id << " " << d.name << "\n";
    auto medium = [&](const Medium& m) { f << " " << m.cell << " " << m.tempC << " " << m.light << " " << m.energy << " " << m.salt; };
    for (const KeptCulture& c : lab.kept) {
        f << "kept " << c.folder << " " << c.who.sx << " " << c.who.sy << " " << c.who.sz << " " << c.who.body << " " << c.who.species << " " << c.who.strain;
        medium(c.medium); medium(c.asWorld);
        f << " " << c.t;
        if (c.paired) f << " " << c.partner.sx << " " << c.partner.sy << " " << c.partner.sz << " " << c.partner.body << " " << c.partner.species << " " << c.partner.strain;   // B-441
        f << "\n";
    }
}
bool Game::keptLoad(const std::string& key, std::istream& is) {
    if (key == "keptfolder") {
        CultureFolder d;
        is >> d.id;
        std::getline(is, d.name); if (!d.name.empty() && d.name[0] == ' ') d.name.erase(0, 1);
        if (!is.fail() && d.id > 0 && !d.name.empty()) lab.folders.push_back(d);
        return true;
    }
    if (key != "kept") return false;
    KeptCulture c;
    auto medium = [&](Medium& m) { is >> m.cell >> m.tempC >> m.light >> m.energy >> m.salt; m.cell = clampi(m.cell, 0, CC_COUNT - 1); m.light = clampi(m.light, 0, CL_COUNT - 1);
                                   m.energy = clampi(m.energy, 0, CE_COUNT - 1); m.salt = clampi(m.salt, 0, 2); };
    is >> c.folder >> c.who.sx >> c.who.sy >> c.who.sz >> c.who.body >> c.who.species >> c.who.strain;
    medium(c.medium); medium(c.asWorld);
    is >> c.t;
    c.who.strainIn = c.who.strain;
    const bool ok = !is.fail() && c.who.body >= 0 && c.who.species >= 0 && c.who.strain >= 0;
    FlaskSpecies p;   // B-441: its partner, when kept with one
    is >> p.sx >> p.sy >> p.sz >> p.body >> p.species >> p.strain;
    if (!is.fail() && p.body >= 0 && p.species >= 0 && p.strain >= 0) { c.paired = true; c.partner = p; c.partner.strainIn = p.strain; }
    if (!ok) return true;
    for (const KeptCulture& o : lab.kept) if (o.folder == c.folder && sameCulture(o, c)) return true;   // R-440: a pair B-441 kept twice, once
    lab.kept.push_back(c);
    return true;
}

// ---- the choosers' folders ---------------------------------------------------------------------------------------------------------------

// a chooser's keys over its folders: Up and Down (Page Up and Down a page), Enter opening a folder on its first row, Esc back to the folders: the
// row chosen (Enter on it) as its index in the chooser's rows, -1 none yet, -2 Esc on the folders (the chooser closes)
int chooserKeys(const Input& in, ChooserPage& P, int& sel) {
    if (P.tabbed && in.wasPressed(KEY_TAB)) { chooserTab(P, 1 - P.tab); return -1; }   // R-441: the other tab
    const int nf = (int)P.folders.size();
    if (nf == 0) return P.tabbed && !in.wasPressed(KEY_ESCAPE) ? -1 : -2;   // (an empty tab waits for Tab or Esc)
    P.folder = clampi(P.folder, 0, nf - 1);
    if (!P.open) {
        if (in.wasPressed(KEY_ESCAPE)) return -2;
        menuStep(in, P.folder, nf);
        if (enterKey(in)) { P.open = true; sel = 0; }
        return -1;
    }
    const std::vector<int>& rows = P.folders[P.folder].rows;
    if (in.wasPressed(KEY_ESCAPE) || rows.empty()) { P.open = false; return -1; }
    menuStep(in, sel, (int)rows.size());
    if (in.wasPressed(KEY_PAGE_UP)) sel = std::max(0, sel - 10);
    if (in.wasPressed(KEY_PAGE_DOWN)) sel = std::min((int)rows.size() - 1, sel + 10);
    sel = clampi(sel, 0, (int)rows.size() - 1);
    return enterKey(in) ? rows[sel] : -1;
}

// its page in a panel's box from its top row down: the folders (each its name and its line at the right) or the open one's rows (`lines`, by
// the chooser's row, cut at a word to fit), the chosen one's marker in the chooser's colour; its footer's words
void drawChooser(RGBCanvas& c, int x0, int y0, int x1, int y1, ChooserPage& P, int sel, int& top, const std::vector<std::string>& lines, uint32_t colour) {
    constexpr int rowH = 9;
    if (P.tabbed) {   // R-441: the tabs over the rows, the one shown in amber
        const char* a = "VIALS";
        const char* b = "CULTURES";
        drawText(c, x1 - textWidth(b), y0, b, P.tab == 1 ? HUD_AMBER : HUD_DIM);
        drawText(c, x1 - textWidth(b) - 12 - textWidth(a), y0, a, P.tab == 0 ? HUD_AMBER : HUD_DIM);
        y0 += 12;
    }
    const int rows = std::max(1, (y1 - 12 - y0) / rowH);
    const int nf = (int)P.folders.size();
    const char* other = P.tab == 0 ? "TAB CULTURES" : "TAB VIALS";
    if (P.tabbed && nf == 0) {
        drawTextCentered(c, (x0 + x1) / 2, y0 + 30, P.tab == 0 ? "NO SPECIES SEEN IN A VIAL YET" : "NO CULTURES YET", HUD_DIM);
        drawTextCentered(c, (x0 + x1) / 2, y0 + 42, P.tab == 0 ? "LOOK AT A VIAL UNDER THE MICROSCOPE" : "GROW ONE IN A FLASK - K KEEPS A COPY", HUD_DIM);
        drawTextCentered(c, (x0 + x1) / 2, y1 - 10, fmt("%s  ESC BACK", other).c_str(), HUD_DIM);
        return;
    }
    if (!P.open) {
        P.folder = clampi(P.folder, 0, std::max(0, nf - 1));
        if (P.folder < P.top) P.top = P.folder;
        if (P.folder >= P.top + rows) P.top = P.folder - rows + 1;
        for (int i = P.top; i < nf && i < P.top + rows; i++) {
            const LabFolder& F = P.folders[i];
            const int y = y0 + (i - P.top) * rowH;
            const bool on = i == P.folder;
            if (on) drawText(c, x0, y, ">", colour);
            const std::string name = fitText(F.name, x1 - x0 - 16 - textWidth(F.line.c_str()) - 12);
            drawText(c, x0 + 8, y, name.c_str(), on ? HUD_WHITE : (F.foreign ? HUD_CYAN : HUD_GREEN));
            drawText(c, x1 - textWidth(F.line.c_str()), y, F.line.c_str(), HUD_DIM);
        }
        drawTextCentered(c, (x0 + x1) / 2, y1 - 10, P.tabbed ? fmt("ENTER OPEN  %s  ESC BACK", other).c_str() : "ENTER OPEN FOLDER  ESC BACK", HUD_DIM);
        return;
    }
    const std::vector<int>& R = P.folders[clampi(P.folder, 0, std::max(0, nf - 1))].rows;
    const int n = (int)R.size();
    if (sel < top) top = sel;
    if (sel >= top + rows) top = sel - rows + 1;
    top = clampi(top, 0, std::max(0, n - rows));
    for (int i = top; i < n && i < top + rows; i++) {
        const int y = y0 + (i - top) * rowH;
        const bool on = i == sel;
        if (on) drawText(c, x0, y, ">", colour);
        const std::string& l = R[i] >= 0 && R[i] < (int)lines.size() ? lines[R[i]] : std::string();
        drawText(c, x0 + 8, y, fitText(l, x1 - x0 - 8).c_str(), on ? HUD_WHITE : HUD_GREEN);
    }
    drawTextCentered(c, (x0 + x1) / 2, y1 - 10, "ENTER CHOOSE  ESC FOLDERS", HUD_DIM);
}

// R-441: a chooser's folders in two tabs, the vials' worlds and the cultures (the flasks', the collection's); the one asked for shown, or the other
// where it holds none
void chooserTabs(ChooserPage& P, int tab) {
    P.tabbed = true;
    P.tabs[0].clear(); P.tabs[1].clear();
    for (const LabFolder& f : P.folders) P.tabs[f.id == FOLDER_WORLD ? 0 : 1].push_back(f);
    tab = clampi(tab, 0, 1);
    chooserTab(P, P.tabs[tab].empty() && !P.tabs[1 - tab].empty() ? 1 - tab : tab);
}
void chooserTab(ChooserPage& P, int tab) {
    P.tab = clampi(tab, 0, 1);
    P.folders = P.tabs[P.tab]; P.open = false; P.folder = 0; P.top = 0;
}

// a chooser's rows put in folders: each row its folder's key (a world's body key, "FLASKS" for a flask's, "KEPT/<id>" for a collection's), the
// folders in the order their first rows come; a world's named as the vials' folders are, a flask's THE FLASKS, a collection's by its folder
std::vector<LabFolder> Game::chooserFolders(const std::vector<std::string>& keys) const {
    std::vector<LabFolder> out;
    std::map<std::string, int> at;
    for (int i = 0; i < (int)keys.size(); i++) {
        auto it = at.find(keys[i]);
        if (it == at.end()) {
            it = at.emplace(keys[i], (int)out.size()).first;
            LabFolder f; f.key = keys[i];
            if (keys[i] == "FLASKS") { f.id = FOLDER_FLASKS; f.name = "FLASKS"; }
            else if (keys[i].compare(0, 5, "KEPT/") == 0) {
                f.id = atoi(keys[i].c_str() + 5); f.name = "COLLECTION: UNSORTED";
                for (const CultureFolder& d : lab.folders) if (d.id == f.id) f.name = "COLLECTION: " + d.name;
            } else { f.name = upper(worldNameOfKey(keys[i])) + labFolderWhere(keys[i]); f.foreign = nameIsForeign(keys[i]); }
            out.push_back(f);
        }
        out[it->second].rows.push_back(i);
    }
    for (LabFolder& f : out) f.line = fmt("%d", (int)f.rows.size());
    return out;
}
std::string Game::keptFolderKey(int k) const { return k >= 0 && k < (int)lab.kept.size() ? fmt("KEPT/%d", lab.kept[k].folder) : std::string(); }

// B-442: a form's colour and motion, which tell two species alike in look and size apart (", GREEN, GLIDING"; ", COLOURLESS, STILL")
std::string Game::formTraits(const Morph& m) {
    return fmt(", %s, %s", m.pigment > PG_NONE && m.pigment < PG_COUNT ? PIGMENT_NAMES[m.pigment] : "COLOURLESS", m.motion > MO_NONE && m.motion < MO_COUNT ? MOTION_NAMES[m.motion] : "STILL");
}
void apartRows(std::vector<std::string>& looks, const std::vector<std::string>& rests, const std::vector<std::string>& keys, const std::vector<std::string>& traits) {
    for (int pass = 0; pass < 2; pass++) {
        std::map<std::string, std::vector<int>> alike;
        for (int i = 0; i < (int)looks.size(); i++) alike[keys[i] + "|" + looks[i] + rests[i]].push_back(i);
        for (const auto& kv : alike)
            for (size_t j = 0; kv.second.size() > 1 && j < kv.second.size(); j++) looks[kv.second[j]] += pass == 0 ? traits[kv.second[j]] : fmt(" (%d)", (int)j + 1);
    }
}
// B-442: a kept culture's row in a chooser: one species' look and size (or its name), a pair's two looks; its traits for `apartRows`. `one`:
// its first species (0) or its partner (1) alone, as its species (the sequencer reads one)
std::string Game::keptRow(int k, std::string* traits, int one) {
    if (k < 0 || k >= (int)lab.kept.size()) return "";
    const KeptCulture& c = lab.kept[k];
    if (c.paired && one < 0) { if (traits) traits->clear(); return keptName(k); }
    FlaskSpecies w = one == 1 && c.paired ? c.partner : c.who;
    if (one >= 0) w.strain = 0;
    CultureKind kind; Morph form; double slept = 0;
    if (!flaskKind(w, kind, form, slept)) { if (traits) traits->clear(); return flaskSpeciesName(w); }
    if (traits) *traits = formTraits(form);
    return flaskSpeciesWords(w, form, true);
}
