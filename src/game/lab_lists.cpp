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
        f.line = fmt("%d VIAL%s - %d READ STRONG", (int)f.rows.size(), f.rows.size() == 1 ? "" : "S", strong);
    }
    return out;
}

// the cultures' folders: the flasks with a culture, UNSORTED (where the collection's new ones go), then the explorer's in the order made
std::vector<LabFolder> Game::cultureFolders() const {
    std::vector<LabFolder> out(2 + lab.folders.size());
    out[0].name = "THE FLASKS"; out[0].id = FOLDER_FLASKS;
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

// a flask's species grown: kept, unless the collection holds its strain already; said, and the flask told (its next strain kept in its turn)
void Game::keepCulture(int i, int k) {
    Flask& F = lab.flasks[i];
    F.told |= FT_KEPT << k;
    for (const KeptCulture& c : lab.kept) if (c.who.sameSpecies(F.who[k]) && c.who.strain == F.who[k].strain) return;
    KeptCulture c;
    c.who = F.who[k]; c.who.strainIn = c.who.strain;
    c.medium = F.medium; c.asWorld = F.asWorld; c.t = lab.clock;
    c.plan = k < (int)F.forms.size() ? F.forms[k].plan : -1;
    lab.kept.push_back(c);
    status(fmt("FLASK %d: %s GROWS - KEPT IN THE COLLECTION", i + 1, F.label[k].c_str()), 5);
}

// a kept culture's words: its strain's or its species' name (UNNAMED and a strain's number else), what it looks like where it has no name
std::string Game::keptName(int k) {
    if (k < 0 || k >= (int)lab.kept.size()) return "";
    KeptCulture& c = lab.kept[k];
    std::string n = flaskSpeciesName(c.who);
    if (n.compare(0, 7, "UNNAMED") != 0) return n;
    if (c.plan < 0) {
        CultureKind kind; Morph form; double slept = 0;
        if (flaskKind(c.who, kind, form, slept)) c.plan = form.plan;
    }
    return c.plan >= 0 ? n + " - " + planWords(c.plan) : n;
}

// a drop of a kept culture on the stage, as a young culture of it shows (its cells growing, a tenth of their crowd, more of them dividing) in
// the medium it grew in; its strain's moves on its values and its form
bool Game::microscopeStageKept(int k) {
    if (k < 0 || k >= (int)lab.kept.size()) return false;
    const KeptCulture& c = lab.kept[k];
    CultureKind kind; Morph form; double slept = 0;
    if (!flaskKind(c.who, kind, form, slept)) { refuse("ITS WORLD IS NO LONGER THERE"); return false; }
    Biota B;
    B.stage = LS_SIMPLE; B.chemistry = kind.chemistry;
    Microbe m = kind.s; m.index = 0; m.state = SS_LIVING;
    B.species.push_back(m);
    scope.species = {m};
    scope.keys = {flaskSpeciesKey(c.who)};
    scope.worlds = {nameOfKey(Guide::bodyKey(c.who.sx, c.who.sy, c.who.sz, c.who.body))};
    const Growth g = growthIn(kind, c.medium);
    Sample S;
    Held h; h.species = 0; h.state = SS_LIVING; h.cells = std::pow(10.0, std::max(5.0, g.crowdLog - 1)); h.active = g.fit == GF_GROWS; h.dividing = 0.25;
    S.held.push_back(h);
    Microhabitat hab;
    hab.niche = NI_LIQUID; hab.meanC = c.medium.tempC; hab.salt = c.medium.salt;
    hab.place = mix64(hashCombine(fnv1a(flaskStrainKey(c.who)), 0x4B3E7C0DULL));
    scope.forms = {form};
    scope.sample = S;
    scope.mount = mountOf(B, scope.forms, S, hab);
    scope.vial = -1; scope.flask = -1; scope.kept = k; scope.life.reset(); scope.body = -1;
    scope.title = flaskSpeciesName(c.who);
    scope.line = fmt("THE COLLECTION, %.0f C", c.medium.tempC);
    microscopeView();
    return true;
}

// N: a folder made (its name typed, none of the same name), its id the next after the highest; R: renamed; Backspace: removed once empty
int Game::folderNew(const std::string& name) {
    if (name.empty()) return -1;
    if (name == "UNSORTED" || name == "THE FLASKS") { refuse("THAT FOLDER IS THERE ALREADY", 3); return -1; }
    for (const CultureFolder& f : lab.folders) if (f.name == name) { refuse("A FOLDER OF THAT NAME IS THERE ALREADY", 3); return -1; }
    int id = 1;
    for (const CultureFolder& f : lab.folders) id = std::max(id, f.id + 1);
    lab.folders.push_back({id, name});
    status("THE FOLDER " + name + " MADE - M MOVES A CULTURE INTO IT", 4);
    return id;
}
bool Game::folderRename(int id, const std::string& name) {
    if (name.empty() || name == "UNSORTED" || name == "THE FLASKS") return false;
    for (const CultureFolder& f : lab.folders) if (f.name == name && f.id != id) { refuse("A FOLDER OF THAT NAME IS THERE ALREADY", 3); return false; }
    for (CultureFolder& f : lab.folders)
        if (f.id == id) { f.name = name; status("THE FOLDER IS NOW CALLED " + name, 3); return true; }
    return false;
}
bool Game::folderRemove(int id) {
    for (const KeptCulture& c : lab.kept) if (c.folder == id) { refuse("ONLY AN EMPTY FOLDER IS REMOVED - M MOVES ITS CULTURES OUT", 3); return false; }
    for (size_t f = 0; f < lab.folders.size(); f++)
        if (lab.folders[f].id == id) {
            status("THE FOLDER " + lab.folders[f].name + " REMOVED", 3);
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
        f << " " << c.t << "\n";
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
    if (!is.fail() && c.who.body >= 0 && c.who.species >= 0 && c.who.strain >= 0) lab.kept.push_back(c);
    return true;
}

// ---- the choosers' folders ---------------------------------------------------------------------------------------------------------------

// a chooser's keys over its folders: Up and Down (Page Up and Down a page), Enter opening a folder on its first row, Esc back to the folders: the
// row chosen (Enter on it) as its index in the chooser's rows, -1 none yet, -2 Esc on the folders (the chooser closes)
int chooserKeys(const Input& in, ChooserPage& P, int& sel) {
    const int nf = (int)P.folders.size();
    if (nf == 0) return -2;
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
    const int rows = std::max(1, (y1 - 12 - y0) / rowH);
    const int nf = (int)P.folders.size();
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
        drawTextCentered(c, (x0 + x1) / 2, y1 - 10, "ENTER OPENS A FOLDER  ESC BACK", HUD_DIM);
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
    drawTextCentered(c, (x0 + x1) / 2, y1 - 10, "ENTER CHOOSES IT  ESC THE FOLDERS", HUD_DIM);
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
            if (keys[i] == "FLASKS") { f.id = FOLDER_FLASKS; f.name = "THE FLASKS"; }
            else if (keys[i].compare(0, 5, "KEPT/") == 0) {
                f.id = atoi(keys[i].c_str() + 5); f.name = "THE COLLECTION: UNSORTED";
                for (const CultureFolder& d : lab.folders) if (d.id == f.id) f.name = "THE COLLECTION: " + d.name;
            } else { f.name = upper(worldNameOfKey(keys[i])) + labFolderWhere(keys[i]); f.foreign = nameIsForeign(keys[i]); }
            out.push_back(f);
        }
        out[it->second].rows.push_back(i);
    }
    for (LabFolder& f : out) f.line = fmt("%d", (int)f.rows.size());
    return out;
}
std::string Game::keptFolderKey(int k) const { return k >= 0 && k < (int)lab.kept.size() ? fmt("KEPT/%d", lab.kept[k].folder) : std::string(); }
