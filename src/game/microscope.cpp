// Z-04: the microscope (R-425; docs/ideas/ITERATION-1.5.0.md Z-04, IDEAS-small-and-spacious.md 1.8 and 1.9; lab_state.h), the laboratory's first
// device: a camera entered from its bench (E). Its first page is the cold store's vials (Enter puts one on the stage, Backspace empties it and
// frees its place); the slide's page shows the vial's mount (life/slides.h) in six ways of seeing (1-6, V), zoomed (+ and -, the wheel), focused
// through the drop ([ and ]), panned (the arrows, the mouse; the electron's cell turned by the mouse), a cell tracked (Enter), and the four things
// done to it (L a spot of light, M the magnet turned, H the stage warmed, F a drop of food). What is seen of each species is kept in the guide
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
const char* const MOTION_WORDS[MO_COUNT] = {"STILL BUT FOR ITS JIGGLE", "RUNS AND TUMBLES", "SWIMS IN A HELIX", "GLIDES", "TWITCHES", "CRAWLS", "ROLLS",
                                            "TURNS AS A CORKSCREW"};
const char* const PLACE_WORDS[FV_COUNT] = {"", "AT A POLE", "AT BOTH POLES", "IN A TUFT", "ALL ROUND", "FROM ITS SIDE", "IN A GIRDLE"};
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

// U-05: E at the microscope: the vials' page, on the vial last looked at
void Game::microscopeEnter() {
    scope.page = 0; scope.sheet = false;
    scope.sel = clampi(scope.vial >= 0 ? scope.vial : scope.sel, 0, std::max(0, microscopeRows() - 1));
}

// Z-05: the list's rows: the cold store's vials, then each flask with a culture in it
int Game::microscopeRows() const {
    int n = (int)coldStore.size();
    for (const Flask& F : lab.flasks) n += F.n > 0;
    return n;
}
int Game::microscopeRowFlask(int row) const {
    int k = row - (int)coldStore.size();
    if (k < 0) return -1;
    for (int i = 0; i < INCUBATOR_FLASKS; i++) if (lab.flasks[i].n > 0 && k-- == 0) return i;
    return -1;
}

// a vial on the stage: its world's system (the explorer's own while it is the one there), its biota and forms, its sample and its mount; the
// view where the mount's first look rests, the slide's clock at 0, nothing done to it
bool Game::microscopeStage(int vial) {
    if (vial < 0 || vial >= (int)coldStore.size()) return false;
    const Vial& v = coldStore[vial];
    if (sys.valid && sys.star.sx == v.sx && sys.star.sy == v.sy && sys.star.sz == v.sz) scope.sys = sys;
    else {
        Star st;
        if (!placeInSector(v.sx, v.sy, v.sz, st, true)) { refuse("THE VIAL'S WORLD IS NO LONGER THERE"); return false; }
        scope.sys = StarSystem(); scope.sys.generate(st);
    }
    if (v.body < 0 || v.body >= (int)scope.sys.bodies.size()) { refuse("THE VIAL'S WORLD IS NO LONGER THERE"); return false; }
    scope.body = v.body; scope.vial = vial; scope.flask = -1;
    scope.life = lifeOf(scope.sys);
    scope.forms = morphsOf(scope.sys, v.body);
    const Biota& B = scope.life->bodies[v.body];
    scope.sample = sampleOf(B, v.hab);
    scope.mount = mountOf(B, scope.forms, scope.sample, v.hab);
    scope.species = B.species; scope.keys.clear(); scope.worlds.clear();
    for (const Microbe& m : B.species) { scope.keys.push_back(Guide::bodyKey(v.sx, v.sy, v.sz, v.body) + "/M" + std::to_string(m.index)); scope.worlds.push_back(vialWorldName(v)); }
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
    if (F.n == 0 || (int)F.forms.size() < F.n) { refuse("THE FLASK'S WORLD IS NO LONGER THERE"); return false; }
    const CultureNow now = flaskNow(i);
    Biota B;
    B.stage = LS_SIMPLE; B.chemistry = F.culture.kind[0].chemistry;
    scope.species.clear(); scope.keys.clear(); scope.worlds.clear();
    Sample S;
    double t0 = 0;
    for (const CultureEvent& e : F.events) if (e.kind == CD_START) { t0 = e.t; break; }
    for (int k = 0; k < F.n; k++) {
        CultureKind kind = F.culture.kind[k];
        for (const CultureEvent& e : F.events) if (e.kind == CD_STRAIN && e.who == k) applyStrain(kind, e.what, e.value);
        Microbe m = kind.s; m.index = k; m.state = SS_LIVING;
        B.species.push_back(m); scope.species.push_back(m);
        scope.keys.push_back(flaskSpeciesKey(F.who[k]));
        scope.worlds.push_back(nameOfKey(Guide::bodyKey(F.who[k].sx, F.who[k].sy, F.who[k].sz, F.who[k].body)));
        const CultureOne& o = now.one[k];
        const bool alive = o.stage != SG_DEAD && (o.fit == GF_GROWS || o.fit == GF_HUNGRY || o.fit == GF_DARK);
        if (o.cells >= 1) {
            Held h; h.species = k; h.state = SS_LIVING; h.cells = o.cells; h.active = alive;
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
    scope.forms = F.forms;
    scope.sample = S;
    scope.mount = mountOf(B, scope.forms, S, h);
    scope.vial = -1; scope.flask = i; scope.life.reset(); scope.body = -1;
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
    int best = 0;   // the zoom's step nearest the first look's magnification
    for (int i = 0; i < SCOPE_LIGHT_STEPS; i++) if (std::fabs(std::log(SCOPE_LIGHT[i] / scope.mount.mag)) < std::fabs(std::log(SCOPE_LIGHT[best] / scope.mount.mag))) best = i;
    scope.step = best; scope.logMag = std::log(SCOPE_LIGHT[best]);
    scope.page = 1;
    microscopeFrame();
}

// Backspace on the list: the vial emptied, its place in the cold store freed (the stage's vial follows its place)
void Game::microscopeEmpty(int vial) {
    if (vial < 0 || vial >= (int)coldStore.size()) return;
    coldStore.erase(coldStore.begin() + vial);
    if (scope.vial == vial) scope.vial = -1; else if (scope.vial > vial) scope.vial--;
    scope.sel = clampi(scope.sel, 0, std::max(0, (int)coldStore.size() - 1));
    status(fmt("THE VIAL IS EMPTIED - %d OF THE COLD STORE'S %d PLACES FILLED", (int)coldStore.size(), COLD_STORE), 4);
    audio.beep = BEEP_UI;
}

// a way of seeing chosen: a drop has no thin section; to and from the electron the zoom keeps the nearest magnification its list has
void Game::microscopeWay(int seeing) {
    if (seeing == WS_SECTION && !scope.mount.section) { refuse("SECTION IS FOR A THIN SECTION OF ROCK - THIS IS A DROP", 3); return; }
    if (seeing == scope.seeing) return;
    const double mag = microscopeStepMag(scope.seeing, scope.step);
    const int n = seeing == WS_ELECTRON ? SCOPE_ELECTRON_STEPS : SCOPE_LIGHT_STEPS;
    int best = 0;
    for (int i = 0; i < n; i++) if (std::fabs(std::log(microscopeStepMag(seeing, i) / mag)) < std::fabs(std::log(microscopeStepMag(seeing, best) / mag))) best = i;
    scope.seeing = seeing; scope.step = best; scope.logMag = std::log(microscopeStepMag(seeing, best));
    status(fmt("%s X%s", seeing == WS_BRIGHT ? "BRIGHT FIELD" : (seeing == WS_PHASE ? "PHASE CONTRAST" : (seeing == WS_DARK ? "DARK FIELD" :
                (seeing == WS_GLOW ? "FLUORESCENCE" : (seeing == WS_ELECTRON ? "THE SCANNING ELECTRON MICROSCOPE" : "THE THIN SECTION, POLARISED")))), thousands(microscopeMag()).c_str()), 2);
    audio.beep = BEEP_UI;
}

SlideLook Game::microscopeLook() const {
    SlideLook L;
    const double mag = microscopeMag(), um = umPerPxAt(mag);
    L.cx = scope.cx; L.cy = scope.cy; L.halfW = um * UW * 0.5; L.halfH = um * UH * 0.5; L.focus = scope.focus; L.mag = mag; L.seeing = scope.seeing;
    return L;
}

void Game::updateMicroscope(const Input& in, double dt, double realDt) {
    if (in.wasPressed(KEY_E) && !in.ctrl()) { closeScreen(); audio.beep = BEEP_UI; return; }
    if (scope.page == 0) {
        const int n = microscopeRows();
        if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); audio.beep = BEEP_UI; return; }
        if (n > 0) menuStep(in, scope.sel, n);
        const int flask = microscopeRowFlask(scope.sel);
        if (n > 0 && enterKey(in)) { if (flask >= 0 ? microscopeStageFlask(flask) : microscopeStage(scope.sel)) audio.beep = BEEP_UI; return; }
        if (n > 0 && (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE))) {
            if (flask >= 0) refuse("A FLASK IS EMPTIED AT THE INCUBATOR", 3); else microscopeEmpty(scope.sel);
        }
        return;
    }
    if (scope.vial < 0 && scope.flask < 0) { scope.page = 0; return; }
    if (in.wasPressed(KEY_ESCAPE)) {
        scope.page = 0; scope.sheet = false; audio.beep = BEEP_UI;
        if (scope.vial >= 0) scope.sel = scope.vial;
        else for (int r = 0; r < microscopeRows(); r++) if (microscopeRowFlask(r) == scope.flask) scope.sel = r;
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
    // the four things done to the slide, at the crosshair
    const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
    auto event = [&](int kind, double x, double y, double r, bool on, const char* said) {
        SlideEvent e; e.kind = kind; e.t = scope.t; e.x = x; e.y = y; e.r = r; e.on = on;
        scope.events.push_back(e);
        status(said, 3); audio.beep = BEEP_UI;
    };
    if (in.wasPressed(KEY_L) && !in.ctrl()) {
        if (st.spot) event(SD_LIGHT, st.spotX, st.spotY, st.spotR, false, "THE SPOT OF LIGHT TAKEN AWAY");
        else event(SD_LIGHT, scope.cx, scope.cy, UW * um * 0.12, true, "A SPOT OF LIGHT AT THE CROSSHAIR");
    }
    if (in.wasPressed(KEY_M) && !in.ctrl()) {
        if (!st.magnet) event(SD_MAGNET, 0, 0, 0, true, "THE MAGNET BESIDE THE STAGE: ITS FIELD ACROSS THE SLIDE");
        else event(SD_MAGNET, wrapAngle(st.field + PI), 0, 0, true, "THE MAGNET TURNED HALF ROUND");
    }
    if (in.wasPressed(KEY_H) && !in.ctrl()) event(SD_WARM, 0, 0, 0, !st.warming, st.warming ? "THE STAGE COOLING TO THE VIAL'S WARMTH" : "THE STAGE WARMING");
    if (in.wasPressed(KEY_F) && !in.ctrl()) event(SD_FOOD, scope.cx, scope.cy, UW * um * 0.08, true, "A DROP OF FOOD AT THE CROSSHAIR");
    // tracking: Enter takes the cell under the crosshair (or lets it go); the stage and the focus follow it
    if (enterKey(in)) {
        if (scope.tracking) { scope.tracking = false; status("TRACKING OFF", 2); audio.beep = BEEP_UI; }
        else if (scope.under >= 0 && scope.under < (int)scope.refs.size()) { scope.tracking = true; scope.track = scope.refs[scope.under]; status("TRACKING THE CELL", 2); audio.beep = BEEP_UI; }
        else refuse("NO CELL UNDER THE CROSSHAIR TO TRACK", 3);
    }
    if (scope.tracking) {
        Specimen s;
        if (mountSpecimen(scope.mount, scope.forms, scope.events, scope.t, microscopeLook(), scope.track, s)) {
            double x, y, a;
            specimenPlace(s, scope.t, x, y, a);
            scope.cx = x; scope.cy = y; scope.focus = clampd(mountDepth(scope.mount, scope.track, scope.t), 0, scope.mount.depthUm);
        } else scope.tracking = false;
    }
    microscopeFrame();
    // Tab: the sheet of the species under the crosshair (it stays on the one shown till Tab again); N names it
    if (in.wasPressed(KEY_TAB)) {
        if (scope.sheet) scope.sheet = false;
        else if (microscopeSpeciesUnder() >= 0) { scope.sheet = true; scope.sheetSpecies = microscopeSpeciesUnder(); }
        else refuse("NO CELL UNDER THE CROSSHAIR - ITS SHEET SHOWS WITH TAB", 3);
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_N) && !in.ctrl()) {
        const int sp = microscopeSpeciesUnder();
        if (sp < 0) { refuse("NO CELL UNDER THE CROSSHAIR TO NAME", 3); return; }
        textSpeciesKey = speciesKey(sp);
        const std::string named = nameOfKey(textSpeciesKey);
        beginTextEntry("NAME THE SPECIES", TEXT_SPECIES, named == "UNKNOWN" ? std::string() : upper(named));
    }
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
}

// what the explorer sees of each species in the frame: a cell in the view's middle, in focus, big enough for the part (its form and its stage
// on any slide, its motion on a living one, its flagella in a dark field, its DNA in the glow, its wall under the electron); a form's first sight
// is the HUD's line and the log's FORM
void Game::microscopeSee() {
    if ((scope.vial < 0 && scope.flask < 0) || scope.species.empty()) return;
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
    if (firsts > 0) status(firsts == 1 ? "A FORM NOT SEEN BEFORE: " + firstLine : fmt("%d FORMS NOT SEEN BEFORE - THE FIRST: %s", firsts, firstLine.c_str()), 5);
    if (changed) guide.save(guidePath);
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
    if (form == CF_SPORE) return fmt("A SPORE, %s, ITS COAT %s", umWords(std::max(0.3, m.widthUm * m.sporeSize)).c_str(), COAT_NAMES[clampi(m.coat, 0, CW_COUNT - 1)]);
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
        if (m.cilia > 0) f = m.ciliaRows > 0 ? fmt("CILIA IN %d ROWS", m.ciliaRows) : std::string("CILIA OVER ITS EDGE");
        out.push_back(f);
    }
    if (seen.seen & SEEN_DNA) {
        std::string d = s.state == SS_PROTO ? "DNA: NONE (NO GENOME YET)" : (m.nucleus > 0 ? "DNA: IN A NUCLEUS" : "DNA: A NUCLEOID");
        if (m.plastids > 0) d += ", RED PLASTIDS";
        else if (m.way == LW_LIGHT) d += ", RED ALL THROUGH";
        if (m.glows) d += ", ITS OWN LIGHT";
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
    if (seen.seen & SEEN_KIN) out.push_back("SEQUENCED: ITS KIN ON THE TREE");   // Z-06
    if (!cell) { out.push_back(spore ? "ITS LIVING CELL NOT SEEN" : "ONLY ITS FOSSILS SEEN"); return out; }
    std::string more;   // the ways that would show more of it
    if (!(seen.seen & SEEN_MOTION) && s.state == SS_LIVING) more += " A LIVING SLIDE";
    if (!(seen.seen & SEEN_FLAGELLA)) more += " DARK";
    if (!(seen.seen & SEEN_DNA)) more += " GLOW";
    if (!(seen.seen & SEEN_WALL)) more += " ELECTRON";
    if (seen.doublingS <= 0 && s.state != SS_FOSSIL && s.state != SS_PROTO) more += " A CULTURE";   // Z-05
    if (!(seen.seen & SEEN_KIN) && s.state != SS_FOSSIL && s.state != SS_PROTO) more += " THE SEQUENCER";   // Z-06
    if (!more.empty()) out.push_back("MORE IN:" + more);
    return out;
}

// U-04: the microscope's commands (the controller's first page) and its buttons
void Game::microscopePad(PadMenu& m) const {
    m.title = "THE MICROSCOPE";
    if (scope.page == 0) {
        if (!coldStore.empty()) { m.item("PUT IT ON THE STAGE", KEY_ENTER); m.item("EMPTY THE VIAL", KEY_BACKSPACE); }
        m.item("BACK TO THE LABORATORY", KEY_ESCAPE);
        return;
    }
    m.item(scope.tracking ? "STOP TRACKING" : "TRACK THE CELL", KEY_ENTER);
    m.item("THE NEXT WAY OF SEEING", KEY_V);
    for (int k = 0; k < WS_COUNT; k++) if (k != WS_SECTION || scope.mount.section) m.item(fmt("%d %s", k + 1, SEEING_NAMES[k]), KEY_1 + k);
    const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
    m.item(st.spot ? "TAKE THE LIGHT AWAY" : "A SPOT OF LIGHT", KEY_L); m.item(st.magnet ? "TURN THE MAGNET" : "THE MAGNET BESIDE THE STAGE", KEY_M);
    m.item(st.warming ? "COOL THE STAGE" : "WARM THE STAGE", KEY_H); m.item("A DROP OF FOOD", KEY_F);
    m.item("ITS SHEET", KEY_TAB); m.item("NAME THE SPECIES", KEY_N); m.item("A MICROGRAPH", KEY_P);
    m.item("THE VIALS", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "TRACK"); m.button(PR_B, KEY_ESCAPE, "VIALS"); m.button(PR_X, KEY_V, "WAY"); m.button(PR_SELECT, KEY_TAB, "SHEET");
    m.button(PR_LB, KEY_MINUS, "ZOOM OUT"); m.button(PR_RB, KEY_EQUAL, "ZOOM IN");
    m.button(PR_LT, KEY_LEFT_BRACKET, "FOCUS DOWN"); m.button(PR_RT, KEY_RIGHT_BRACKET, "FOCUS UP");
    m.button(PR_START, KEY_ESCAPE, "VIALS");
    m.sticks = "STICKS MOVE THE STAGE";
}
