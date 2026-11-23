// Z-04: the microscope (R-425; docs/ideas/ITERATION-1.5.0.md Z-04, IDEAS-small-and-spacious.md 1.8 and 1.9; lab_state.h), the laboratory's first
// device: a camera entered from its bench (E). Its first page is its lists (R-432: the cold store's vials a folder a world, Enter putting one
// on the stage, Backspace emptying it; the cultures, R-433's collection among them, by folder: lab_lists.cpp); the slide's page shows the vial's mount (life/slides.h) in six ways of seeing (1-6, V), zoomed (+ and -, the wheel), focused
// through the drop ([ and ]), panned (the arrows, the mouse; the electron's cell turned by the mouse), a cell tracked (Enter), and the four things
// done to it (L a spot of light, M the magnet turned, H the stage's warmth typed, F a drop of food; R-436 to R-438: each undone by its key, what
// it is for said, what the view's kinds answer told and kept on their sheets). What is seen of each species is kept in the guide
// (its sheet's lines), a form's first sight said and logged (FORM); N names the species under the crosshair, Tab shows its sheet, P a micrograph
// to the gallery (the photograph's key, its caption the slide's)
#include "game.h"
#include "ui.h"
#include "galaxy/drifters.h"
#include <algorithm>
#include <cmath>

namespace {
// a form in words, what it looks like (never a name: the names are the explorer's)
const char* const FORM_WORDS[MP_COUNT] = {"A VESICLE", "A VESICLE OF LAYERS", "A DROPLET", "A SPHERE", "A ROD", "A CURVED ROD", "A SPIRAL", "A CORKSCREW",
                                          "A FILAMENT", "A SQUARE PLATE", "A STAR", "A CELL ON A STALK", "A BUDDING CELL", "A CELL OF CHANGING SHAPE",
                                          "AN AMOEBA", "A FLAGELLATE", "A CILIATE", "A BOX OF TWO HALVES", "A TEST OF GRAINS", "A LATTICE OF SPINES",
                                          "A CHAMBERED SHELL", "AN ARMOURED CELL", "A ROLLING COLONY", "A PLATE COLONY", "A CHAIN COLONY", "BRANCHING THREADS",
                                          "GATHERING AMOEBAE"};
const char* const MOTION_WORDS[MO_COUNT] = {"STILL, ONLY JIGGLING", "RUNS AND TUMBLES", "SWIMS IN A HELIX", "GLIDES", "TWITCHES", "CRAWLS", "ROLLS",
                                            "TURNS AS A CORKSCREW"};
const char* const PLACE_WORDS[FV_COUNT] = {"", "AT A POLE", "AT BOTH POLES", "IN A TUFT", "ALL ROUND", "FROM THE SIDE", "IN A GIRDLE"};
const char* const KIND_WORDS[FS_COUNT] = {"WHIPS", "TINSELS", "RIBBONS"};
const char* const MARK_WORDS[MK_COUNT] = {"SMOOTH", "PORES IN ROWS", "RIBBED", "SPINED", "A LATTICE", "PLATES", "A NET OF RIDGES", "GRAINS GLUED ON"};
const char* const PIGMENT_WORDS[PG_COUNT] = {"COLOURLESS", "GREEN", "PURPLE", "ORANGE", "RED", "BROWN", "BLUE-GREEN", "BLACK"};
const char* const STAGE_WORDS[CF_COUNT] = {"CELLS", "SPORES", "CYSTS", "DIVIDING", "A COLONY", "A SLUG", "A STALK", "LYSED", "FOSSILS"};
// how much of a body a pixel of the view at 1x must hold for a part to be seen: its form (5), its flagella (8), its DNA (6), its wall (24)
constexpr double SEE_FORM_PX = 5, SEE_FLAGELLA_PX = 8, SEE_DNA_PX = 6, SEE_WALL_PX = 24;

std::string umWords(double um) { return um < 10 ? fmt("%.1f UM", um) : (um < 1000 ? fmt("%.0f UM", um) : fmt("%.1f MM", um / 1000)); }
}   // namespace

// the magnification of a way's zoom step (the electron's own list)
double Game::microscopeStepMag(int seeing, int step) const {
    return seeing == WS_ELECTRON ? SCOPE_ELECTRON[clampi(step, 0, SCOPE_ELECTRON_STEPS - 1)] : SCOPE_LIGHT[clampi(step, 0, SCOPE_LIGHT_STEPS - 1)];
}

// U-05: E at the microscope: its lists, on what was last on the stage (R-432: in its folder)
void Game::microscopeEnter() {
    scope.page = 0; scope.sheet = false; scope.moving = false; keep.open = false;
    microscopeFind();
}

// R-432: the lists on what is (or was last) on the stage: its tab and its folder open on its row (a vial's world, the flasks, a kept culture's
// folder); with nothing there, the folders of the tab last shown
void Game::microscopeFind() {
    if (scope.vial >= 0 && scope.vial < (int)coldStore.size()) {
        const std::vector<LabFolder> F = vialFolders();
        for (int f = 0; f < (int)F.size(); f++)
            for (int r = 0; r < (int)F[f].rows.size(); r++)
                if (F[f].rows[r] == scope.vial) { scope.tab = 0; scope.folder = f; scope.open = true; scope.sel = r; return; }
    }
    if (scope.flask >= 0 || scope.kept >= 0) {
        const std::vector<LabFolder> F = cultureFolders();
        for (int f = 0; f < (int)F.size(); f++) {
            const int want = F[f].id == FOLDER_FLASKS ? scope.flask : scope.kept;
            for (int r = 0; want >= 0 && r < (int)F[f].rows.size(); r++)
                if (F[f].rows[r] == want) { scope.tab = 1; scope.folder = f; scope.open = true; scope.sel = r; return; }
        }
    }
    scope.open = false;
}

// R-432: the lists' keys: Tab the other tab; Up and Down (Page Up and Down ten); on the folders Enter opens one and Esc closes the microscope; in a
// folder Enter puts its row on the stage and Esc goes back to the folders; Backspace empties a vial. R-433, the cultures: N makes a folder; on
// one of the explorer's R renames it and Backspace removes it once empty; on a kept culture M moves it (a chooser of the folders), Backspace
// discards it
void Game::microscopeListKeys(const Input& in) {
    const std::vector<LabFolder> F = scope.tab == 0 ? vialFolders() : cultureFolders();
    const int nf = (int)F.size();
    const bool del = in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE);
    const LabFolder* open = scope.open && scope.folder < nf && !F[scope.folder].rows.empty() ? &F[scope.folder] : nullptr;
    if (!open) scope.open = false;
    if (scope.moving) {   // M's chooser: UNSORTED and the explorer's folders
        const int n = 1 + (int)lab.folders.size();
        if (!open || open->id < 0 || in.wasPressed(KEY_ESCAPE)) { scope.moving = false; audio.beep = BEEP_UI; return; }
        menuStep(in, scope.moveSel, n);
        if (enterKey(in)) {
            const int k = open->rows[clampi(scope.sel, 0, (int)open->rows.size() - 1)], id = scope.moveSel == 0 ? 0 : lab.folders[clampi(scope.moveSel - 1, 0, n - 2)].id;
            if (scope.copying) keptCopy(k, id); else keptMove(k, id);   // R-440: C copies, M moves
            scope.moving = false; audio.beep = BEEP_UI;
        }
        return;
    }
    if (in.wasPressed(KEY_TAB)) { scope.tab = 1 - scope.tab; scope.open = false; scope.folder = 0; scope.folderTop = 0; audio.beep = BEEP_UI; return; }
    if (scope.tab == 1 && in.wasPressed(KEY_N) && !in.ctrl()) { textFolderId = -1; beginTextEntry("NAME THE NEW FOLDER", TEXT_FOLDER, ""); return; }
    if (!open) {
        if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); audio.beep = BEEP_UI; return; }
        if (nf == 0) return;
        menuStep(in, scope.folder, nf);
        scope.folder = clampi(scope.folder, 0, nf - 1);
        const LabFolder& f = F[scope.folder];
        if (enterKey(in)) {
            if (f.rows.empty()) refuse(f.id == FOLDER_FLASKS ? "NO CULTURES - GROW ONE IN THE INCUBATOR" : "FOLDER EMPTY", 3);
            else { scope.open = true; scope.sel = 0; scope.top = 0; audio.beep = BEEP_UI; }
            return;
        }
        const bool own = scope.tab == 1 && f.id > 0;   // the explorer's folders
        if (own && in.wasPressed(KEY_R) && !in.ctrl()) { textFolderId = f.id; beginTextEntry("RENAME FOLDER", TEXT_FOLDER, f.name); return; }
        if (own && del) { if (folderRemove(f.id)) audio.beep = BEEP_UI; return; }
        if (del) refuse(scope.tab == 0 ? "OPEN THE WORLD (ENTER) TO EMPTY ONE OF ITS VIALS" : "ONLY YOUR OWN FOLDERS CAN BE REMOVED", 3);
        else if (scope.tab == 1 && in.wasPressed(KEY_R) && !in.ctrl()) refuse("THIS FOLDER CANNOT BE RENAMED - N MAKES YOUR OWN", 3);
        return;
    }
    if (in.wasPressed(KEY_ESCAPE)) { scope.open = false; audio.beep = BEEP_UI; return; }
    const int n = (int)open->rows.size();
    menuStep(in, scope.sel, n);
    if (in.wasPressed(KEY_PAGE_UP)) scope.sel = std::max(0, scope.sel - 10);
    if (in.wasPressed(KEY_PAGE_DOWN)) scope.sel = std::min(n - 1, scope.sel + 10);
    scope.sel = clampi(scope.sel, 0, n - 1);
    const int row = open->rows[scope.sel];
    if (enterKey(in)) {
        if (open->id == FOLDER_WORLD ? microscopeStage(row) : (open->id == FOLDER_FLASKS ? microscopeStageFlask(row) : microscopeStageKept(row))) audio.beep = BEEP_UI;
        return;
    }
    if (open->id == FOLDER_WORLD) {   // its world's folder gone with its last vial: back to the worlds
        if (del) { const bool last = n == 1; microscopeEmpty(row); if (last) scope.open = false; }
        return;
    }
    if (open->id == FOLDER_FLASKS) {
        if (del) refuse("EMPTY FLASKS IN THE INCUBATOR", 3);
        else if ((in.wasPressed(KEY_M) || in.wasPressed(KEY_C)) && !in.ctrl()) refuse("GROWING CULTURES JOIN THE COLLECTION - K KEEPS A COPY", 3);
        return;
    }
    if (del) { keptDiscard(row); audio.beep = BEEP_UI; return; }
    const bool copy = in.wasPressed(KEY_C) && !in.ctrl();   // R-440
    if ((in.wasPressed(KEY_M) && !in.ctrl()) || copy) {
        if (lab.folders.empty() && !copy) { refuse("NO FOLDER TO MOVE IT TO - N MAKES ONE", 3); return; }
        scope.moving = true; scope.copying = copy; scope.moveSel = 0;
        for (size_t d = 0; d < lab.folders.size(); d++) if (lab.folders[d].id == lab.kept[row].folder) scope.moveSel = (int)d + 1;
        audio.beep = BEEP_UI;
    }
}

// a vial on the stage: its world's system (the explorer's own while it is the one there), its biota and forms, its sample and its mount; the
// view where the mount's first look rests, the slide's clock at 0, nothing done to it
bool Game::microscopeStage(int vial) {
    if (vial < 0 || vial >= (int)coldStore.size()) return false;
    const Vial& v = coldStore[vial];
    if (sys.valid && sys.star.sx == v.sx && sys.star.sy == v.sy && sys.star.sz == v.sz) scope.sys = sys;
    else {
        Star st;
        if (!placeInSector(v.sx, v.sy, v.sz, st, true)) { refuse("VIAL'S WORLD NO LONGER EXISTS"); return false; }
        scope.sys = StarSystem(); scope.sys.generate(st);
    }
    if (v.body < 0 || v.body >= (int)scope.sys.bodies.size()) { refuse("VIAL'S WORLD NO LONGER EXISTS"); return false; }
    scope.body = v.body; scope.vial = vial; scope.flask = -1; scope.kept = -1; scope.lineState[0] = scope.lineState[1] = -1;
    scope.life = lifeOf(scope.sys);
    scope.forms = morphsOf(scope.sys, v.body);
    Biota B = scope.life->bodies[v.body];
    scope.sample = sampleOf(B, v.hab);
    scope.keys.clear(); scope.worlds.clear(); scope.wild.clear();
    for (const Microbe& m : B.species) {
        scope.keys.push_back(Guide::bodyKey(v.sx, v.sy, v.sz, v.body) + "/M" + std::to_string(m.index)); scope.worlds.push_back(vialWorldName(v)); scope.wild.push_back("");
    }
    for (const VialLine& L : vialLines(v, scope.sys, scope.sample)) {   // J-03: a seeded world's lines where they spread, after its own species
        FlaskSpecies w;
        if (!speciesOfKey(L.key, w)) continue;
        Microbe m = L.line.kind.s; m.index = (int)B.species.size(); m.state = SS_LIVING;
        B.species.push_back(m); scope.forms.push_back(L.line.form);
        scope.keys.push_back(flaskSpeciesKey(w)); scope.worlds.push_back(nameOfKey(Guide::bodyKey(w.sx, w.sy, w.sz, w.body)));
        scope.wild.push_back(Guide::bodyKey(v.sx, v.sy, v.sz, v.body) + "/L" + std::to_string(L.seeding));   // J-12: found in the wild
        Held h; h.species = m.index; h.state = SS_LIVING; h.cells = L.cells; h.active = true;
        scope.sample.held.push_back(h);
    }
    scope.mount = mountOf(B, scope.forms, scope.sample, v.hab);
    cellRange(chemistryCell(B.chemistry), scope.stageLo, scope.stageHi);   // R-438: the drop liquid there
    scope.species = B.species;
    scope.title = upper(vialWorldName(v)); scope.line = v.name;
    microscopeView();
    return true;
}

// Z-05: a drop of a flask's culture on the stage, as it is on the lab's clock: its species' cells (more of them dividing while it grows; the
// gathered amoebae's slugs and stalks), their spores or cysts, the lysed; at the flask's warmth
bool Game::microscopeStageFlask(int i) {
    if (i < 0 || i >= INCUBATOR_FLASKS || lab.flasks[i].n == 0) return false;
    Flask& F = lab.flasks[i];
    if (!F.made) flaskMake(i);
    if (F.n == 0 || (int)F.forms.size() < F.n) { refuse("FLASK'S WORLD NO LONGER EXISTS"); return false; }
    const CultureNow now = flaskNow(i);
    Biota B;
    B.stage = LS_SIMPLE; B.chemistry = F.culture.kind[0].chemistry;
    scope.species.clear(); scope.keys.clear(); scope.worlds.clear(); scope.wild.clear();
    Sample S;
    double t0 = 0;
    for (const CultureEvent& e : F.events) if (e.kind == CD_START) { t0 = e.t; break; }
    std::vector<Morph> forms = F.forms;
    for (int k = 0; k < F.n; k++) {
        CultureKind kind = F.culture.kind[k];
        for (const CultureEvent& e : F.events) if (e.kind == CD_STRAIN && e.who == k) applyStrain(kind, e.what, e.value);
        scope.lineState[k] = -1;   // Y-04: a switched line as its flask's state has it (its genes off there: its form, its moves, its answers)
        if (!kind.switches.empty() && now.one[k].state >= 0 && k < (int)forms.size()) {
            switchedLine(kind, forms[k], F.genes[k], (uint32_t)now.one[k].state);
            scope.lineState[k] = now.one[k].state;
        }
        Microbe m = kind.s; m.index = k; m.state = SS_LIVING;
        B.species.push_back(m); scope.species.push_back(m);
        scope.keys.push_back(flaskSpeciesKey(F.who[k]));
        scope.worlds.push_back(nameOfKey(Guide::bodyKey(F.who[k].sx, F.who[k].sy, F.who[k].sz, F.who[k].body)));
        const CultureOne& o = now.one[k];
        const bool alive = o.stage != SG_DEAD && (o.fit == GF_GROWS || o.fit == GF_HUNGRY || o.fit == GF_DARK);
        if (o.cells >= 1) {
            Held h; h.species = k; h.state = SS_LIVING; h.cells = o.cells; h.active = alive;
            h.alive = alive || (o.stage != SG_DEAD && (o.fit == GF_COLD || o.fit == GF_FROZEN)) ? 1 : 0;   // R-438: a cold culture's cells wait for their warmth (J-01: a frozen one's too)
            if (o.stage == SG_GROWING || o.stage == SG_COLONY || o.stage == SG_CYCLING) h.dividing = 0.25;
            if (o.stage == SG_GATHERING) h.form = CF_SLUG;
            if (o.stage == SG_FRUITING) h.form = CF_STALK;
            S.held.push_back(h);
        }
        if (o.spores >= 1) { Held h; h.species = k; h.state = SS_DORMANT; h.cells = o.spores; S.held.push_back(h); }
        if (o.ghosts >= 1) { Held h; h.species = k; h.state = SS_LIVING; h.cells = o.ghosts; h.form = CF_GHOST; S.held.push_back(h); }
    }
    Microhabitat h;
    h.niche = NI_LIQUID; h.meanC = F.medium.tempC; h.salt = F.medium.salt;
    h.place = mix64(hashCombine((uint64_t)(i + 1) * 0x9E3779B97F4A7C15ULL, (uint64_t)std::llround(t0 * 1000)));
    scope.forms = forms;
    scope.sample = S;
    scope.mount = mountOf(B, scope.forms, S, h);
    cellRange(F.medium.cell, scope.stageLo, scope.stageHi);
    scope.vial = -1; scope.flask = i; scope.kept = -1; scope.life.reset(); scope.body = -1;
    scope.title = fmt("FLASK %d", i + 1);
    scope.line = upper(CULTURE_STAGE_NAMES[now.one[0].stage]) + fmt(", %.0f C", F.medium.tempC);
    microscopeView();
    return true;
}

// the view where a new mount's first look rests, the slide's clock at 0, nothing done to it
void Game::microscopeView() {
    scope.events.clear(); scope.t = 0;
    scope.tracking = false; scope.sheet = false; scope.under = -1;
    scope.cx = scope.mount.cx; scope.cy = scope.mount.cy; scope.focus = scope.mount.focus;
    scope.seeing = scope.mount.seeing; scope.turn = 0.6; scope.tilt = 0.55;
    scope.typing = false; scope.told.assign(scope.mount.kinds.size(), 0); scope.said.clear(); scope.fieldShown = -100;   // R-437
    for (int d = 0; d < SD_COUNT; d++) { scope.tried[d] = -1; scope.answered[d] = scope.judged[d] = false; }
    int best = 0;   // the zoom's step nearest the first look's magnification
    for (int i = 0; i < SCOPE_LIGHT_STEPS; i++) if (std::fabs(std::log(SCOPE_LIGHT[i] / scope.mount.mag)) < std::fabs(std::log(SCOPE_LIGHT[best] / scope.mount.mag))) best = i;
    scope.step = best; scope.logMag = std::log(SCOPE_LIGHT[best]);
    scope.page = 1;
    microscopeFrame();
}

// Backspace on a world's vials: the vial emptied (the stage's vial follows its place; the world's folder goes with its last vial)
void Game::microscopeEmpty(int vial) {
    if (vial < 0 || vial >= (int)coldStore.size()) return;
    coldStore.erase(coldStore.begin() + vial);
    if (scope.vial == vial) scope.vial = -1; else if (scope.vial > vial) scope.vial--;
    status(fmt("VIAL EMPTIED - %d LEFT IN THE COLD STORE", (int)coldStore.size()), 4);
    audio.beep = BEEP_UI;
}

// a way of seeing chosen: a drop has no thin section; to and from the electron the zoom keeps the nearest magnification its list has
void Game::microscopeWay(int seeing) {
    if (seeing == WS_SECTION && !scope.mount.section) { refuse("THIN SECTION IS FOR ROCK - THIS IS A DROP", 3); return; }
    if (seeing == scope.seeing) return;
    const double mag = microscopeStepMag(scope.seeing, scope.step);
    const int n = seeing == WS_ELECTRON ? SCOPE_ELECTRON_STEPS : SCOPE_LIGHT_STEPS;
    int best = 0;
    for (int i = 0; i < n; i++) if (std::fabs(std::log(microscopeStepMag(seeing, i) / mag)) < std::fabs(std::log(microscopeStepMag(seeing, best) / mag))) best = i;
    scope.seeing = seeing; scope.step = best; scope.logMag = std::log(microscopeStepMag(seeing, best));
    status(fmt("%s X%s", seeing == WS_BRIGHT ? "BRIGHT FIELD" : (seeing == WS_PHASE ? "PHASE CONTRAST" : (seeing == WS_DARK ? "DARK FIELD" :
                (seeing == WS_GLOW ? "FLUORESCENCE" : (seeing == WS_ELECTRON ? "SCANNING ELECTRON MICROSCOPE" : "THIN SECTION, POLARISED")))), thousands(microscopeMag()).c_str()), 2);
    audio.beep = BEEP_UI;
}

SlideLook Game::microscopeLook() const {
    SlideLook L;
    const double mag = microscopeMag(), um = umPerPxAt(mag);
    L.cx = scope.cx; L.cy = scope.cy; L.halfW = um * UW * 0.5; L.halfH = um * UH * 0.5; L.focus = scope.focus; L.mag = mag; L.seeing = scope.seeing;
    return L;
}

void Game::updateMicroscope(const Input& in, double dt, double realDt) {
    if (scope.page == 1 && scope.typing) {   // R-438: the stage's warmth typed (the slide lives on meanwhile)
        if (scope.seeing != WS_ELECTRON) scope.t += realDt;
        microscopeTypeKeys(in);
        microscopeFrame();
        return;
    }
    if (scope.page == 1 && keep.open) {   // R-440: the folder a copy of the species under the crosshair goes in (the slide lives on meanwhile)
        if (scope.seeing != WS_ELECTRON) scope.t += realDt;
        keepKeys(in);
        microscopeFrame();
        return;
    }
    if (in.wasPressed(KEY_E) && !in.ctrl()) { closeScreen(); audio.beep = BEEP_UI; return; }
    if (scope.page == 0) { microscopeListKeys(in); return; }
    if (scope.vial < 0 && scope.flask < 0 && scope.kept < 0) { scope.page = 0; return; }
    if (in.wasPressed(KEY_ESCAPE)) {   // back to the lists, in the folder of what is on the stage
        scope.page = 0; scope.sheet = false; audio.beep = BEEP_UI;
        microscopeFind();
        return;
    }
    if (saveKey(in)) { saveSlot(currentSlot); return; }
    if (scope.seeing != WS_ELECTRON) scope.t += realDt;   // the slide lives on, but under the beam (a fixed, dried sample)
    // the ways of seeing: 1-6, V the next (Shift+V the one before)
    for (int k = 0; k < WS_COUNT; k++) if (in.wasPressed(KEY_1 + k)) microscopeWay(k);
    if (in.wasPressed(KEY_V) && !in.ctrl()) {
        int w = scope.seeing;
        for (int i = 0; i < WS_COUNT; i++) { w = (w + (in.shift() ? WS_COUNT - 1 : 1)) % WS_COUNT; if (w != WS_SECTION || scope.mount.section) break; }
        microscopeWay(w);
    }
    // the zoom: +/-, the wheel, Page Up and Down; the shown magnification eases to its step's
    const int steps = scope.seeing == WS_ELECTRON ? SCOPE_ELECTRON_STEPS : SCOPE_LIGHT_STEPS;
    if (in.wheel > 0 || in.wasPressed(KEY_EQUAL) || in.wasPressed(KEY_PAGE_UP)) { if (scope.step < steps - 1) { scope.step++; audio.beep = BEEP_UI; } }
    if (in.wheel < 0 || in.wasPressed(KEY_MINUS) || in.wasPressed(KEY_PAGE_DOWN)) { if (scope.step > 0) { scope.step--; audio.beep = BEEP_UI; } }
    scope.logMag += (std::log(microscopeStepMag(scope.seeing, scope.step)) - scope.logMag) * (1 - std::exp(-realDt * 14));
    const double mag = microscopeMag(), um = umPerPxAt(mag), dof = depthOfField(mag);
    // the focus through the drop: [ down toward the slide's glass, ] up toward the cover; a press a half depth of field, held a steady turn
    {
        const double rate = std::max(2.0, dof * 2.5);
        double f = scope.focus;
        if (in.wasPressed(KEY_RIGHT_BRACKET)) f += dof * 0.5; else if (in.isDown(KEY_RIGHT_BRACKET)) f += rate * realDt;
        if (in.wasPressed(KEY_LEFT_BRACKET)) f -= dof * 0.5; else if (in.isDown(KEY_LEFT_BRACKET)) f -= rate * realDt;
        scope.focus = clampd(f, 0, scope.mount.depthUm);
    }
    // the stage: the arrows (half the view a second) and the mouse (not while a cell is tracked: a hand on it would let the cell go); under
    // the electron the mouse turns its cell and tilts it
    double px = 0, py = 0;
    if (in.isDown(KEY_LEFT)) px -= 0.5 * UW * um * realDt;
    if (in.isDown(KEY_RIGHT)) px += 0.5 * UW * um * realDt;
    if (in.isDown(KEY_UP)) py -= 0.5 * UW * um * realDt;
    if (in.isDown(KEY_DOWN)) py += 0.5 * UW * um * realDt;
    const double ms = 0.5 * settings.mouseSensitivity;
    if (scope.seeing == WS_ELECTRON) {
        scope.turn = wrapAngle(scope.turn + in.mouseDx * 0.006 * ms);
        scope.tilt = clampd(scope.tilt + in.mouseDy * 0.004 * ms * (settings.invertY ? -1 : 1), 0.05, 1.2);
    } else if (!scope.tracking) { px += in.mouseDx * um * ms; py += in.mouseDy * um * ms; }
    if (px != 0 || py != 0) { scope.cx += px; scope.cy += py; if (scope.tracking) { scope.tracking = false; status("TRACKING OFF", 2); } }
    // the four things done to the slide, at the crosshair: R-436 and R-437, each undone by its key again (the magnet turned half round first),
    // what it is for said as it is done; R-438, H types the stage's warmth (Shift+H the vial's again)
    const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
    auto event = [&](int kind, double x, double y, double r, bool on, const std::string& said) {
        SlideEvent e; e.kind = kind; e.t = scope.t; e.x = x; e.y = y; e.r = r; e.on = on;
        scope.events.push_back(e);
        scope.tried[kind] = on ? scope.t : -1; scope.answered[kind] = scope.judged[kind] = false;
        status(said, 4); audio.beep = BEEP_UI;
    };
    if (in.wasPressed(KEY_L) && !in.ctrl()) {
        if (st.spot) event(SD_LIGHT, st.spotX, st.spotY, st.spotR, false, "LIGHT OFF");
        else event(SD_LIGHT, scope.cx, scope.cy, UW * um * 0.12, true, "SPOT OF LIGHT ON - LIGHT-FEEDERS SWIM INTO IT");
    }
    if (in.wasPressed(KEY_M) && !in.ctrl()) {
        if (!st.magnet) event(SD_MAGNET, 0, 0, 0, true, "MAGNET ON - MAGNETOTACTIC CELLS FOLLOW ITS FIELD");
        else if (std::cos(st.field) > 0) event(SD_MAGNET, wrapAngle(st.field + PI), 0, 0, true, "MAGNET TURNED - FIELD NOW POINTS LEFT");
        else event(SD_MAGNET, st.field, 0, 0, false, "MAGNET OFF");
        scope.fieldShown = scope.t;
    }
    if (in.wasPressed(KEY_H) && !in.ctrl()) {
        if (!in.shift()) { scope.typing = true; scope.typed.clear(); audio.beep = BEEP_UI; }
        else if (std::fabs(st.setC - scope.mount.stageC) < 0.05) refuse("STAGE AT THE VIAL'S WARMTH - H SETS ANOTHER", 3);
        else event(SD_WARM, scope.mount.stageC, 0, 0, true, fmt("STAGE BACK TO THE VIAL'S %.0f C", scope.mount.stageC));
    }
    if (in.wasPressed(KEY_F) && !in.ctrl()) {
        if (st.food) event(SD_FOOD, st.foodX, st.foodY, st.foodR, false, "SLIDE RINSED - FOOD WASHED AWAY");
        else event(SD_FOOD, scope.cx, scope.cy, UW * um * 0.08, true, "DROP OF BROTH ADDED - EATERS SWIM TO ITS SCENT");
    }
    // tracking: Enter takes the cell under the crosshair (or lets it go); the stage and the focus follow it
    if (enterKey(in)) {
        if (scope.tracking) { scope.tracking = false; status("TRACKING OFF", 2); audio.beep = BEEP_UI; }
        else if (scope.under >= 0 && scope.under < (int)scope.refs.size()) { scope.tracking = true; scope.track = scope.refs[scope.under]; status("TRACKING CELL", 2); audio.beep = BEEP_UI; }
        else refuse("NO CELL UNDER THE CROSSHAIR TO TRACK", 3);
    }
    if (scope.tracking) {
        Specimen s;
        if (mountSpecimen(scope.mount, scope.forms, scope.events, scope.t, microscopeLook(), scope.track, s)) {
            double x, y, a;
            specimenPlace(s, scope.t, x, y, a);
            scope.cx = x; scope.cy = y; scope.focus = clampd(mountDepth(scope.mount, scope.track, scope.t, &scope.events), 0, scope.mount.depthUm);
        } else scope.tracking = false;
    }
    microscopeFrame();
    if (!scope.said.empty() && realTime >= statusUntil) { status(scope.said.front(), 4); scope.said.erase(scope.said.begin()); }   // R-437
    // Tab: the sheet of the species under the crosshair (it stays on the one shown till Tab again); N names it
    if (in.wasPressed(KEY_TAB)) {
        if (scope.sheet) scope.sheet = false;
        else if (microscopeSpeciesUnder() >= 0) { scope.sheet = true; scope.sheetSpecies = microscopeSpeciesUnder(); }
        else refuse("NO CELL UNDER THE CROSSHAIR FOR A SHEET", 3);
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_N) && !in.ctrl()) {
        const int sp = microscopeSpeciesUnder();
        if (sp < 0) { refuse("NO CELL UNDER THE CROSSHAIR TO NAME", 3); return; }
        textSpeciesKey = speciesKey(sp);
        const std::string named = nameOfKey(textSpeciesKey);
        beginTextEntry("NAME THE SPECIES", TEXT_SPECIES, named == "UNKNOWN" ? std::string() : upper(named));
        return;
    }
    if (in.wasPressed(KEY_K) && !in.ctrl()) microscopeKeep();   // R-440
}

// R-440: K keeps the species under the crosshair alone, a copy picked out of the drop (a vial's, a flask's, a kept culture's: each keeps its
// own) into a folder: a vial's in its place's medium (AS ITS WORLD), a culture's in its own; never a fossil, a protocell or a dead culture
void Game::microscopeKeep() {
    const int sp = microscopeSpeciesUnder();
    if (sp < 0 || sp >= (int)scope.species.size() || sp >= (int)scope.forms.size()) { refuse("NO CELL UNDER THE CROSSHAIR TO KEEP", 3); return; }
    const Microbe& s = scope.species[sp];
    if (s.state == SS_FOSSIL) { refuse("FOSSILS CANNOT BE KEPT - ONLY LIVING OR SLEEPING CELLS", 3); return; }
    if (s.state == SS_PROTO) { refuse("PROTOCELLS DO NOT GROW - NOTHING TO KEEP", 3); return; }
    KeptCulture c;
    if (scope.flask >= 0 && scope.flask < INCUBATOR_FLASKS && sp < lab.flasks[scope.flask].n) {
        const Flask& F = lab.flasks[scope.flask];
        const CultureOne o = flaskNow(scope.flask).one[sp];
        if (o.stage == SG_DEAD || (o.cells < 1 && o.spores < 1)) { refuse(F.label[sp] + " IS DEAD - NOTHING TO KEEP", 3); return; }
        c.who = F.who[sp]; c.medium = F.medium; c.asWorld = F.asWorld;
    } else if (scope.kept >= 0 && scope.kept < (int)lab.kept.size()) {
        const KeptCulture& K = lab.kept[scope.kept];
        c.who = sp == 1 && K.paired ? K.partner : K.who; c.medium = K.medium; c.asWorld = K.asWorld;
    } else if (scope.vial >= 0 && scope.vial < (int)coldStore.size()) {
        const Vial& V = coldStore[scope.vial];
        c.who.sx = V.sx; c.who.sy = V.sy; c.who.sz = V.sz; c.who.body = V.body; c.who.species = sp;
        if (!vialMedium(V, c.asWorld)) { refuse("VIAL'S WORLD NO LONGER EXISTS"); return; }
        c.medium = c.asWorld;
    } else return;
    c.who.strainIn = c.who.strain;
    c.plan = scope.forms[sp].plan;
    keepOpen(c);
}

// the frame's specimens (the mount's copies in the view), the one under the crosshair, and what is seen of them
void Game::microscopeFrame() {
    const SlideLook L = microscopeLook();
    mountSpecimens(scope.mount, scope.forms, scope.events, scope.t, L, scope.shown, &scope.refs);
    const double um = umPerPxAt(L.mag);
    scope.under = -1;
    double best = 1e30;
    for (size_t i = 0; i < scope.shown.size(); i++) {
        const Specimen& s = scope.shown[i];
        if (!s.m || s.blurUm > um * 3) continue;
        const double d = std::hypot(scope.refs[i].x - L.cx, scope.refs[i].y - L.cy), r = std::max(s.m->bodyUm() * 1.1, 5 * um);
        if (d < r && d < best) { best = d; scope.under = (int)i; }
    }
    microscopeSee();
    microscopeAnswers();
    scopeLearn();   // Y-02: a flask's edited line against its parent
}

// what the explorer sees of each species in the frame: a cell in the view's middle, in focus, big enough for the part (its form and its stage
// on any slide, its motion on a living one, its flagella in a dark field, its DNA in the glow, its wall under the electron); a form's first sight
// is the HUD's line and the log's FORM
void Game::microscopeSee() {
    if ((scope.vial < 0 && scope.flask < 0 && scope.kept < 0) || scope.species.empty()) return;
    const SlideLook L = microscopeLook();
    const double um = umPerPxAt(L.mag);
    bool changed = false;
    std::string firstLine;
    int firsts = 0;
    for (size_t i = 0; i < scope.shown.size(); i++) {
        const Specimen& s = scope.shown[i];
        if (!s.m || s.blurUm > um * 0.6) continue;
        if (std::fabs(scope.refs[i].x - L.cx) > L.halfW * 0.85 || std::fabs(scope.refs[i].y - L.cy) > L.halfH * 0.85) continue;
        const double px = 2 * s.m->bodyUm() / um;
        if (px < SEE_FORM_PX) continue;
        const SlideKind& K = scope.mount.kinds[scope.mount.cells[scope.refs[i].cell].kind];
        MicrobeSheet seen;
        seen.seen = SEEN_FORM; seen.forms = 1u << s.form;
        const bool cell = s.form == CF_CELL || s.form == CF_DIVIDING || s.form == CF_COLONY;   // a spore's coat, a fossil's outline show no more
        if (cell && K.active && s.act == AN_LIVE && (L.seeing == WS_BRIGHT || L.seeing == WS_PHASE || L.seeing == WS_DARK)) seen.seen |= SEEN_MOTION;
        if (cell && L.seeing == WS_DARK && px >= SEE_FLAGELLA_PX) seen.seen |= SEEN_FLAGELLA;
        if (cell && L.seeing == WS_GLOW && px >= SEE_DNA_PX) seen.seen |= SEEN_DNA;
        if (cell && L.seeing == WS_ELECTRON && px >= SEE_WALL_PX) seen.seen |= SEEN_WALL;
        const std::string key = speciesKey(K.species);
        const std::string wild = K.species >= 0 && K.species < (int)scope.wild.size() ? scope.wild[K.species] : std::string();
        if (!wild.empty() && !guide.wildFound.count(wild)) {   // J-12: a seeded line found in the wild, in the log with its world
            guide.wildFound[wild] = false;
            changed = true;
            const std::vector<Seeding>& ss = seedingsAt(scope.sys, scope.body);
            const int k = atoi(wild.c_str() + wild.rfind("/L") + 2);
            if (k >= 0 && k < (int)ss.size()) logEvent("SEEDED", fmt("%s: %s FOUND IN THE WILD", scope.title.c_str(), seedLineWords(ss[k].line).c_str()));
        }
        auto it = guide.microbes.find(key);
        if (it == guide.microbes.end()) {
            guide.microbes[key] = seen;
            changed = true;
            const std::string line = formWords(K.species, s.form);
            logEvent("FORM", fmt("A NEW FORM FROM %s: %s", upper(scope.worlds[clampi(K.species, 0, (int)scope.worlds.size() - 1)]).c_str(), line.c_str()));
            if (firsts++ == 0) firstLine = line;
            continue;
        }
        if ((it->second.seen | seen.seen) != it->second.seen || (it->second.forms | seen.forms) != it->second.forms) {
            it->second.seen |= seen.seen; it->second.forms |= seen.forms;
            changed = true;
        }
    }
    if (firsts > 0) status(firsts == 1 ? "NEW FORM: " + firstLine : fmt("%d NEW FORMS - FIRST: %s", firsts, firstLine.c_str()), 5);
    if (changed) guide.save(guidePath);
}

// R-438: the stage's warmth typed (H): digits, the minus sign, Backspace, Up and Down a degree (Page Up and Down ten); Enter sets it, within the
// drop's liquid range; Esc keeps the warmth it has
void Game::microscopeTypeKeys(const Input& in) {
    const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
    if (in.wasPressed(KEY_ESCAPE)) { scope.typing = false; status(fmt("STAGE STAYS AT %.0f C", st.setC), 2); audio.beep = BEEP_UI; return; }
    warmthTyping(in, scope.typed, st.setC);
    if (!enterKey(in)) return;
    scope.typing = false;
    if (scope.typed.empty() || scope.typed == "-") return;
    const int v = atoi(scope.typed.c_str());
    if (v < scope.stageLo || v > scope.stageHi) {
        scope.typing = true;
        refuse(fmt("DROP STAYS LIQUID FROM %.0f TO %.0f C", scope.stageLo, scope.stageHi), 3);
        return;
    }
    SlideEvent e; e.kind = SD_WARM; e.t = scope.t; e.x = v;
    scope.events.push_back(e);
    scope.tried[SD_WARM] = scope.t; scope.answered[SD_WARM] = scope.judged[SD_WARM] = false;
    status(fmt("STAGE AT %d C - KINDS MOVE ONLY IN THEIR RANGE", v), 4);
    audio.beep = BEEP_UI;
}

// R-437: who answers, as the HUD says it: the species' name (the explorer's), else its form's words ("A CURVED ROD")
std::string Game::answerWho(int species) const {
    const std::string n = nameOfKey(speciesKey(species));
    if (n != "UNKNOWN") return upper(n).substr(0, 24);
    return species >= 0 && species < (int)scope.forms.size() ? planWords(scope.forms[species].plan) : "A CELL";
}

// R-437: what the view's kinds do on the slide (`slideAnswer`): each answer said once a slide (its state as found not said: a change of it is),
// its sheet's answers and the warmths it was seen at kept; an experiment's while (the food's: its scent across half the view; the warmth's: the
// stage settled) judged once, the moving kinds in view not answering it kept so, and with none in view answering it said so (the magnet's with
// where its cells live)
void Game::microscopeAnswers() {
    if (scope.seeing == WS_ELECTRON || (scope.vial < 0 && scope.flask < 0 && scope.kept < 0)) return;
    const int nk = (int)scope.mount.kinds.size();
    if ((int)scope.told.size() != nk) scope.told.assign(nk, 0);
    const double um = umPerPxAt(microscopeMag());
    std::vector<char> inView(nk, 0);
    for (size_t i = 0; i < scope.shown.size(); i++) if (scope.shown[i].blurUm <= um * 3) inView[scope.mount.cells[scope.refs[i].cell].kind] = 1;
    const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
    const int stageC = (int)std::lround(st.stageC);
    const uint32_t STATE = SA_MOVING | SA_COLD | SA_HOT;
    std::vector<uint32_t> now(nk, 0);
    for (int k = 0; k < nk; k++) {
        if (!inView[k]) continue;
        const SlideKind& K = scope.mount.kinds[k];
        const uint32_t a = now[k] = slideAnswer(scope.mount, scope.events, k, scope.t);
        uint32_t& told = scope.told[k];
        if (!(told & SCOPE_LOOKED)) told = SCOPE_LOOKED | (a & (STATE | SA_STRESSED));
        const uint32_t fresh = a & ~told;
        const std::string who = answerWho(K.species);
        auto say = [&](const std::string& l, int d) { scope.said.push_back(l); scope.answered[d] = true; };
        if (fresh & SA_LIGHT) say(who + " GATHERS IN LIGHT", SD_LIGHT);
        if (fresh & SA_FLEES) say(who + " FLEES LIGHT", SD_LIGHT);
        if (fresh & SA_LIT) say(who + " GATHERS ROUND LIT CELLS", SD_LIGHT);
        if (fresh & SA_FOOD) say(who + " SWIMS TO FOOD", SD_FOOD);
        if (fresh & SA_FIELD) say(who + " FOLLOWS THE MAGNET", SD_MAGNET);
        if (fresh & SA_MOVING) say(fmt("%s MOVES AT %d C", who.c_str(), stageC), SD_WARM);
        if (fresh & SA_COLD) say(fmt("%s STOPS IN THE COLD AT %d C", who.c_str(), stageC), SD_WARM);
        if (fresh & SA_HOT) say(fmt("%s STOPS IN THE HEAT AT %d C", who.c_str(), stageC), SD_WARM);
        if (fresh & SA_STRESSED)
            say(fmt("%s %s AT %.0f C", who.c_str(), K.stress == AN_SPORULATE ? "FORMS SPORES" : (K.stress == AN_ENCYST ? "ENCYSTS" : "BURSTS"), K.tMax + 2), SD_WARM);
        told = (told & ~STATE) | a | SCOPE_LOOKED;
        if (fresh & (SA_LIGHT | SA_FLEES | SA_FOOD | SA_FIELD)) scopeAnswerLearn(k, fresh);   // Y-02: an edited line's against its parent's
        if (scope.flask >= 0 && K.species >= 0 && K.species < 2 && lab.flasks[scope.flask].edited[K.species]) continue;   // (its answers its line's, not its species' sheet's)
        auto it = guide.microbes.find(speciesKey(K.species));   // its sheet (made at its form's first sight)
        if (it == guide.microbes.end()) continue;
        MicrobeSheet& m = it->second;
        const MicrobeSheet was = m;
        if (a & SA_LIGHT) m.answers = (m.answers | AW_LIGHT) & ~AW_LIGHT_NONE;
        if (a & SA_FLEES) m.answers = (m.answers | AW_FLEES) & ~AW_LIGHT_NONE;
        if (a & SA_LIT) m.answers = (m.answers | AW_LIT) & ~AW_FOOD_NONE;
        if (a & SA_FOOD) m.answers = (m.answers | AW_FOOD) & ~AW_FOOD_NONE;
        if (a & SA_FIELD) m.answers = (m.answers | AW_FIELD) & ~AW_FIELD_NONE;
        if (a & SA_MOVING) { m.movingLo = std::min(m.movingLo, (double)stageC); m.movingHi = std::max(m.movingHi, (double)stageC); }
        if (a & SA_COLD) m.stillCold = std::max(m.stillCold, (double)stageC);
        if (a & SA_HOT) m.stillHot = std::min(m.stillHot, (double)stageC);
        if ((a & SA_STRESSED) && m.stressAct < 0) { m.stressC = std::round(K.tMax + 2); m.stressAct = K.stress; }
        if (m.answers != was.answers || m.movingLo != was.movingLo || m.movingHi != was.movingHi || m.stillCold != was.stillCold || m.stillHot != was.stillHot ||
            m.stressAct != was.stressAct) scope.answersDirty = true;
    }
    // the experiments' whiles
    const double half = UW * um * 0.5, scent1 = std::max(1e-9, slideScentUm(1));
    for (int d = 0; d < SD_COUNT; d++) {
        if (scope.tried[d] < 0 || scope.judged[d]) continue;
        const double wait = d == SD_FOOD ? clampd(half * half / (scent1 * scent1), 15, 45) : (d == SD_WARM ? 8 : 15);
        if (scope.t - scope.tried[d] < wait || (d == SD_WARM && std::fabs(st.stageC - st.setC) > 0.5)) continue;
        scope.judged[d] = true;
        if (d != SD_WARM) {   // the moving kinds in view that did not answer it, kept so
            const uint32_t want = d == SD_LIGHT ? (SA_LIGHT | SA_FLEES) : (d == SD_FOOD ? SA_FOOD : SA_FIELD);
            const uint32_t none = d == SD_LIGHT ? AW_LIGHT_NONE : (d == SD_FOOD ? AW_FOOD_NONE : AW_FIELD_NONE);
            const uint32_t yes = d == SD_LIGHT ? (AW_LIGHT | AW_FLEES) : (d == SD_FOOD ? (AW_FOOD | AW_LIT) : AW_FIELD);
            for (int k = 0; k < nk; k++) {
                if (!inView[k] || !(now[k] & SA_MOVING) || (scope.told[k] & want)) continue;
                scopeAnswerLearn(k, 0, d);   // Y-02: an edited line that did not answer where its parent would
                const int sp = scope.mount.kinds[k].species;
                if (scope.flask >= 0 && sp >= 0 && sp < 2 && lab.flasks[scope.flask].edited[sp]) continue;
                auto it = guide.microbes.find(speciesKey(scope.mount.kinds[k].species));
                if (it != guide.microbes.end() && !(it->second.answers & (yes | none))) { it->second.answers |= none; scope.answersDirty = true; }
            }
        }
        if (scope.answered[d]) continue;
        if (d == SD_LIGHT) scope.said.push_back("NOTHING IN VIEW ANSWERS LIGHT");
        else if (d == SD_FOOD) scope.said.push_back("NOTHING IN VIEW FOLLOWS FOOD");
        else if (d == SD_MAGNET) { scope.said.push_back("NOTHING IN VIEW FOLLOWS THE MAGNET"); scope.said.push_back("MAGNETOTACTIC CELLS LIVE IN SILT AND MUD"); }
        else scope.said.push_back(fmt("NOTHING IN VIEW CHANGES AT %d C", stageC));
    }
    if (scope.answersDirty && realTime - scope.answersSavedAt >= 2) { guide.save(guidePath); scope.answersDirty = false; scope.answersSavedAt = realTime; }
}

// the species under the crosshair (its index in its world's list), else -1
int Game::microscopeSpeciesUnder() const {
    if (scope.under < 0 || scope.under >= (int)scope.refs.size()) return -1;
    const SlideRef& r = scope.refs[scope.under];
    return scope.mount.kinds[scope.mount.cells[r.cell].kind].species;
}

// a species' key in the guide: its world's (the vial's) and its index in the world's list
std::string Game::speciesKey(int species) const { return species >= 0 && species < (int)scope.keys.size() ? scope.keys[species] : std::string(); }
std::string Game::speciesName(int species) const {
    const std::string n = nameOfKey(speciesKey(species));
    return n == "UNKNOWN" ? "UNNAMED" : upper(n);
}
std::string Game::vialWorldName(const Vial& v) const { return nameOfKey(Guide::bodyKey(v.sx, v.sy, v.sz, v.body)); }

// a form in words as it is seen (a cell, a spore and its coat, a cyst, a fossil's outline, a lysed cell's ghost): what it is, its size; the log's
// FORM, the HUD's first sight and the line under the crosshair
std::string Game::formWords(int species, int form) const {
    if (species < 0 || species >= (int)scope.forms.size()) return "";
    return formWordsOf(scope.forms[species], form);
}
// Z-07: a form in words, of any species' form (the catalogue's); its plan's words alone ("A ROD")
const char* Game::planWords(int plan) { return FORM_WORDS[clampi(plan, 0, MP_COUNT - 1)]; }
std::string Game::formWordsOf(const Morph& m, int form) {
    if (form == CF_SPORE) return fmt("A SPORE, %s, COAT %s", umWords(std::max(0.3, m.widthUm * m.sporeSize)).c_str(), COAT_NAMES[clampi(m.coat, 0, CW_COUNT - 1)]);
    if (form == CF_CYST) return fmt("A CYST, %s ACROSS", umWords(m.widthUm * 0.9).c_str());
    const double L = std::max(m.lengthUm, m.widthUm), W = std::min(m.lengthUm, m.widthUm);
    const std::string what = std::string(form == CF_FOSSIL ? "FOSSIL OF " : (form == CF_GHOST ? "GHOST OF " : "")) + FORM_WORDS[clampi(m.plan, 0, MP_COUNT - 1)];
    if (m.plan == MP_FILAMENT || m.plan == MP_CHAIN) return what + " OF " + umWords(W) + " CELLS";
    if (colonyPlan(m.plan)) return what + ", " + umWords(2 * m.bodyUm()) + " ACROSS";
    return what + ", " + (L > W * 1.6 ? fmt("%s BY %s", umWords(L).c_str(), umWords(W).c_str()) : umWords(L) + " ACROSS");
}

// a species' sheet (IDEAS 1.9): its name and world, then a line for each part of it seen and the forms of its life seen; nothing of what was not
// seen, and the ways that would show more
std::vector<std::string> Game::sheetLines(int species) const {
    if (species < 0 || species >= (int)scope.forms.size() || species >= (int)scope.species.size() || species >= (int)scope.worlds.size()) return {};
    const auto it = guide.microbes.find(speciesKey(species));
    return sheetLinesOf(speciesKey(species), scope.species[species], scope.forms[species], scope.worlds[species], it == guide.microbes.end() ? MicrobeSheet() : it->second);
}
// Z-07: the forms of its life seen and the stages its cultures showed, in the sheet's words (the LIFE page's too: a large cell's spores a cyst's)
std::string Game::formsSeenWords(uint32_t forms) {
    std::string f;
    for (int k = 0; k < CF_COUNT; k++) if (forms & (1u << k)) f += std::string(f.empty() ? "" : ", ") + STAGE_WORDS[k];
    return f;
}
std::string Game::stagesSeenWords(uint32_t stages, bool largeCell) {
    static const char* const WORDS[LC_BITS] = {"ASLEEP", "WAKING", "GROWING", "", "", "IN COLONIES", "GATHERING", "FRUITING", "SPORULATING", "DYING"};
    std::string f;
    for (int b = 0; b < LC_BITS; b++) {
        if (!(stages & (1u << b)) || !WORDS[b][0]) continue;
        f += std::string(f.empty() ? "" : ", ") + ((1u << b) == LC_SPORULATING && largeCell ? "ENCYSTING" : WORDS[b]);
    }
    return f;
}
// Z-07: any species' sheet (the catalogue's: its world's species and form, a friend's sheet lent), its name the explorer's (or a friend's)
std::vector<std::string> Game::sheetLinesOf(const std::string& key, const Microbe& s, const Morph& m, const std::string& world, const MicrobeSheet& seen) const {
    std::vector<std::string> out;
    const std::string named = nameOfKey(key);
    out.push_back(named == "UNKNOWN" ? "UNNAMED" : upper(named));
    out.push_back("FROM " + upper(world));
    const uint32_t cells = (1u << CF_CELL) | (1u << CF_DIVIDING) | (1u << CF_COLONY) | (1u << CF_SLUG) | (1u << CF_STALK) | (1u << CF_GHOST);
    const bool cell = (seen.forms & cells) != 0, spore = (seen.forms & ((1u << CF_SPORE) | (1u << CF_CYST))) != 0;
    if (!(seen.seen & SEEN_FORM)) { out.push_back("NOT SEEN YET"); return out; }
    out.push_back("FORM: " + formWordsOf(m, cell ? CF_CELL : (spore ? ((seen.forms & (1u << CF_SPORE)) ? CF_SPORE : CF_CYST) : CF_FOSSIL)));
    if (cell) {
        std::string look = std::string("COLOUR: ") + PIGMENT_WORDS[clampi(m.pigment, 0, PG_COUNT - 1)];
        if (m.arrangement != AR_SINGLE && !colonyPlan(m.plan)) look += m.arrangement == AR_PAIRS ? ", IN PAIRS" : (m.arrangement == AR_CHAINS ? ", IN CHAINS" :
                                       (m.arrangement == AR_CLUSTERS ? ", IN CLUSTERS" : (m.arrangement == AR_FOURS ? ", IN FOURS" :
                                       (m.arrangement == AR_PACKETS ? ", IN PACKETS" : (m.arrangement == AR_PALISADES ? ", SIDE BY SIDE" : ", IN ROSETTES")))));
        out.push_back(look);
    }
    if (seen.seen & SEEN_MOTION) out.push_back("MOTION: " + std::string(MOTION_WORDS[clampi(m.motion, 0, MO_COUNT - 1)]) +
                                               (m.motion != MO_NONE && m.speed > 0 ? fmt(m.speed < 10 ? ", ABOUT %.1f UM/S" : ", ABOUT %.0f UM/S", m.speed) : std::string()));
    if (seen.seen & SEEN_FLAGELLA) {
        std::string f = m.flagella > 0 ? fmt("FLAGELLA: %d %s, %s", m.flagella, PLACE_WORDS[clampi(m.flagPlace, 0, FV_COUNT - 1)], KIND_WORDS[clampi(m.flagKind, 0, FS_COUNT - 1)])
                                       : std::string("FLAGELLA: NONE");
        if (m.cilia > 0) f = m.ciliaRows > 0 ? fmt("CILIA IN %d ROWS", m.ciliaRows) : std::string("CILIA ROUND THE EDGE");
        out.push_back(f);
    }
    if (seen.seen & SEEN_DNA) {
        std::string d = s.state == SS_PROTO ? "DNA: NONE (NO GENOME YET)" : (m.nucleus > 0 ? "DNA: IN A NUCLEUS" : "DNA: A NUCLEOID");
        if (m.plastids > 0) d += ", RED PLASTIDS";
        else if (m.way == LW_LIGHT) d += ", RED ALL THROUGH";
        if (m.glows) d += ", GLOWS";
        out.push_back(d);
    }
    if (seen.seen & SEEN_WALL) {
        std::string w = fmt("WALL: %s, %s", MINERAL_NAMES[clampi(m.mineral, 0, MI_COUNT - 1)], MARK_WORDS[clampi(m.mark, 0, MK_COUNT - 1)]);
        if (m.spines > 0) w += fmt(", %d SPINES", m.spines);
        out.push_back(w);
    }
    if (seen.forms) out.push_back("SEEN AS: " + formsSeenWords(seen.forms));
    if (seen.stages) {   // Z-05: the stages its cultures' curves showed, its quickest growth and the medium of it
        const std::string w = stagesSeenWords(seen.stages, largeCellPlan(m.plan));
        if (!w.empty()) out.push_back("STAGES SEEN: " + w);
    }
    if (seen.doublingS > 0)
        out.push_back(fmt("GROWS: DOUBLING IN ABOUT %.0f S AT %.0f C, %s, %s, %s", seen.doublingS, seen.tempC, CULTURE_ENERGY_NAMES[clampi(seen.energy, 0, CE_COUNT - 1)],
                          CULTURE_LIGHT_NAMES[clampi(seen.light, 0, CL_COUNT - 1)], CULTURE_SALT_NAMES[clampi(seen.salt, 0, 2)]));
    if (seen.sleptYears > 0) out.push_back("WOKEN AFTER " + yearsWords(seen.sleptYears));   // Z-07: its spores' sleep, as the flask said it
    // R-437: what it answered on the slide, and the stage's warmths it was seen at
    if (seen.answers & (AW_LIGHT | AW_FLEES | AW_LIGHT_NONE))
        out.push_back(std::string("TO LIGHT: ") + (seen.answers & AW_LIGHT ? "GATHERS IN IT" : (seen.answers & AW_FLEES ? "FLEES IT" : "NO ANSWER SEEN")));
    if (seen.answers & (AW_FOOD | AW_LIT | AW_FOOD_NONE)) {
        std::string f = seen.answers & AW_FOOD ? "FOLLOWS THE SCENT" : "";
        if (seen.answers & AW_LIT) f += std::string(f.empty() ? "" : ", ") + "GATHERS ROUND LIT CELLS";
        out.push_back("TO FOOD: " + (f.empty() ? std::string("NO ANSWER SEEN") : f));
    }
    if (seen.answers & (AW_FIELD | AW_FIELD_NONE)) out.push_back(std::string("TO THE MAGNET: ") + (seen.answers & AW_FIELD ? "FOLLOWS IT" : "NO ANSWER SEEN"));
    {
        std::string w, still;
        if (seen.movingHi >= seen.movingLo) w = seen.movingHi > seen.movingLo ? fmt("MOVES AT %.0f TO %.0f C", seen.movingLo, seen.movingHi) : fmt("MOVES AT %.0f C", seen.movingLo);
        if (seen.stillCold > -1e8) still = fmt("%.0f", seen.stillCold);
        if (seen.stillHot < 1e8) still += (still.empty() ? "" : " AND ") + fmt("%.0f", seen.stillHot);
        if (!still.empty()) w += std::string(w.empty() ? "" : ", ") + "STILL AT " + still + " C";
        if (seen.stressAct >= 0)
            w += std::string(w.empty() ? "" : ", ") + (seen.stressAct == AN_SPORULATE ? "FORMS SPORES" : (seen.stressAct == AN_ENCYST ? "ENCYSTS" : "BURSTS")) + fmt(" AT %.0f C", seen.stressC);
        if (!w.empty()) out.push_back("WARMTH: " + w);
    }
    if (seen.seen & SEEN_KIN) out.push_back("SEQUENCED: KIN SHOWN ON THE TREE");   // Z-06
    if (!cell) { out.push_back(spore ? "LIVING CELLS NOT SEEN" : "ONLY FOSSILS SEEN"); return out; }
    std::string more;   // the ways that would show more of it
    if (!(seen.seen & SEEN_MOTION) && s.state == SS_LIVING) more += " LIVING SLIDE";
    if (!(seen.seen & SEEN_FLAGELLA)) more += " DARK";
    if (!(seen.seen & SEEN_DNA)) more += " GLOW";
    if (!(seen.seen & SEEN_WALL)) more += " ELECTRON";
    if (seen.doublingS <= 0 && s.state != SS_FOSSIL && s.state != SS_PROTO) more += " CULTURE";   // Z-05
    if (!(seen.seen & SEEN_KIN) && s.state != SS_FOSSIL && s.state != SS_PROTO) more += " SEQUENCER";   // Z-06
    if (!more.empty()) out.push_back("MORE IN:" + more);
    return out;
}

// U-04: the microscope's commands (the controller's first page) and its buttons
void Game::microscopePad(PadMenu& m) const {
    m.title = "MICROSCOPE";
    if (scope.page == 0) {   // R-432: the lists' (the folders, or a folder's rows; R-433's own folders and kept cultures)
        if (scope.moving) { m.item(scope.copying ? "COPY IT THERE" : "MOVE IT THERE", KEY_ENTER); m.item("BACK", KEY_ESCAPE); return; }
        const std::vector<LabFolder> F = scope.tab == 0 ? vialFolders() : cultureFolders();
        const LabFolder* f = scope.folder < (int)F.size() ? &F[scope.folder] : nullptr;
        if (!scope.open) {
            if (f) m.item(scope.tab == 0 ? "OPEN WORLD'S VIALS" : "OPEN FOLDER", KEY_ENTER);
            if (scope.tab == 1) m.item("NEW FOLDER", KEY_N);
            if (f && scope.tab == 1 && f->id > 0) { m.item("RENAME FOLDER", KEY_R); m.item("REMOVE FOLDER", KEY_BACKSPACE); }
            m.item(scope.tab == 0 ? "CULTURES" : "VIALS", KEY_TAB);
            m.item("BACK", KEY_ESCAPE);
            m.button(PR_A, KEY_ENTER, "OPEN"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_SELECT, KEY_TAB, scope.tab == 0 ? "CULTURES" : "VIALS");
            return;
        }
        m.item("PUT ON STAGE", KEY_ENTER);
        if (f && f->id == FOLDER_WORLD) m.item("EMPTY VIAL", KEY_BACKSPACE);
        if (f && f->id >= 0) { m.item("MOVE TO FOLDER", KEY_M); m.item("COPY TO FOLDER", KEY_C); m.item("DISCARD", KEY_BACKSPACE); }
        if (scope.tab == 1) m.item("NEW FOLDER", KEY_N);
        m.item("FOLDERS", KEY_ESCAPE);
        m.button(PR_A, KEY_ENTER, "STAGE"); m.button(PR_B, KEY_ESCAPE, "FOLDERS"); m.button(PR_SELECT, KEY_TAB, scope.tab == 0 ? "CULTURES" : "VIALS");
        return;
    }
    if (keep.open) { m.item("KEEP A COPY THERE", KEY_ENTER); m.item("NEW FOLDER", KEY_N); m.item("BACK", KEY_ESCAPE); return; }   // R-440
    if (scope.typing) {   // R-438: the stage's warmth typed
        m.item("SET", KEY_ENTER); m.item("1 DEGREE WARMER", KEY_UP); m.item("1 DEGREE COOLER", KEY_DOWN); m.item("10 WARMER", KEY_PAGE_UP);
        m.item("10 COOLER", KEY_PAGE_DOWN); m.item("CANCEL", KEY_ESCAPE);
        m.button(PR_A, KEY_ENTER, "SET"); m.button(PR_B, KEY_ESCAPE, "KEEP");
        return;
    }
    m.item(scope.tracking ? "STOP TRACKING" : "TRACK CELL", KEY_ENTER);
    m.item("NEXT WAY OF SEEING", KEY_V);
    for (int k = 0; k < WS_COUNT; k++) if (k != WS_SECTION || scope.mount.section) m.item(fmt("%d %s", k + 1, SEEING_NAMES[k]), KEY_1 + k);
    const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
    m.item(st.spot ? "LIGHT OFF" : "SPOT OF LIGHT", KEY_L);
    m.item(!st.magnet ? "MAGNET ON" : (std::cos(st.field) > 0 ? "TURN MAGNET HALF ROUND" : "MAGNET OFF"), KEY_M);
    m.item(st.food ? "RINSE SLIDE" : "DROP OF FOOD", KEY_F);
    if (std::fabs(st.setC - scope.mount.stageC) >= 0.05) m.item("STAGE TO VIAL'S WARMTH", KEY_H, MOD_SHIFT);
    m.item("SHEET", KEY_TAB); m.item("NAME SPECIES", KEY_N); m.item("MICROGRAPH", KEY_P);
    m.item("SET STAGE WARMTH", KEY_H);   // (last: its entry's page follows)
    m.item("KEEP IN FOLDER", KEY_K);      // R-440 (after the warmth's: its folders' page follows)
    m.item("LISTS", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "TRACK"); m.button(PR_B, KEY_ESCAPE, "LISTS"); m.button(PR_X, KEY_V, "WAY"); m.button(PR_SELECT, KEY_TAB, "SHEET");
    m.button(PR_LB, KEY_MINUS, "ZOOM OUT"); m.button(PR_RB, KEY_EQUAL, "ZOOM IN");
    m.button(PR_LT, KEY_LEFT_BRACKET, "FOCUS DOWN"); m.button(PR_RT, KEY_RIGHT_BRACKET, "FOCUS UP");
    m.button(PR_START, KEY_ESCAPE, "LISTS");
    m.sticks = "STICKS MOVE THE STAGE";
}
