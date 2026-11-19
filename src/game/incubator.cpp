// Z-05: the incubator (R-425; docs/ideas/ITERATION-1.5.0.md Z-05, IDEAS-small-and-spacious.md 1.8 and 1.9; lab_state.h, life/cultures.h,
// life/jars.h), the laboratory's second device: a screen over the room, E at its cabinet. Twelve flasks: Up and Down choose one, Left and Right
// the medium's field (its cell, its warmth, its light, its energy, its salt), + and - change it (Shift ten degrees), W sets it AS ITS WORLD
// again; Enter on an empty flask puts in a species seen in a vial or a flask's culture, on a culture a second; F a fresh broth (a hundredth into
// new medium), Backspace empties it, N names a strain. Tab the jars' shelf: Enter makes a world's jar from a vial of its wet silt, a mat or
// soil, Backspace empties one. The lab's clock is the drones' kind (real seconds while the game simulates); a culture is closed-form from its
// events on it (`runCulture`); what happens in a flask is said and logged as the clock reaches it (WOKEN, STRAIN), and what its curve shows of
// its species, its stages and its growth, goes to the species' sheet
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace {
std::string moveWords(int what, double value) {
    switch (what) {
        case SM_HOT: return fmt("IT GROWS TO %.0f C", value);
        case SM_COLD: return fmt("IT GROWS DOWN TO %.0f C", value);
        case SM_SIZE: return "ITS CELLS A QUARTER BIGGER";
        case SM_SALT_LO: return std::string("IT GROWS IN ") + (value <= 0 ? "FRESH WATER" : "SEA SALT");
        case SM_SALT_HI: return std::string("IT GROWS IN ") + (value >= 2 ? "BRINE" : "SEA SALT");
        case SM_PIGMENT: return std::string("ITS PIGMENT ") + (value == PG_BLUE_GREEN ? "BLUE-GREEN" : (value == PG_ORANGE ? "ORANGE" : "BLACK"));
        default: return "A STRAIN";
    }
}
}   // namespace

std::string Game::flaskSpeciesKey(const FlaskSpecies& w) const { return Guide::bodyKey(w.sx, w.sy, w.sz, w.body) + "/M" + std::to_string(w.species); }
std::string Game::flaskStrainKey(const FlaskSpecies& w) const { return w.strain > 0 ? flaskSpeciesKey(w) + "/S" + std::to_string(w.strain) : flaskSpeciesKey(w); }
// its name: a strain's own (else its species' and its number), a species' (else UNNAMED)
std::string Game::flaskSpeciesName(const FlaskSpecies& w) const {
    const std::string sp = nameOfKey(flaskSpeciesKey(w)), species = sp == "UNKNOWN" ? "UNNAMED" : upper(sp);
    if (w.strain <= 0) return species;
    const std::string st = nameOfKey(flaskStrainKey(w));
    return st == "UNKNOWN" ? fmt("%s S%d", species.c_str(), w.strain) : upper(st);
}

// what a flask's species is: its world's (its values, its form), the strain it went in as (its moves from the wild one), and a vial's spores'
// sleep (its world's end, its drifter's leaving, the impact that carried them)
bool Game::flaskKind(const FlaskSpecies& w, CultureKind& kind, Morph& form, double& slept) const {
    LabSystems systems;
    const StarSystem* S = systems.get(sys, w.sx, w.sy, w.sz);
    if (!S || w.body < 0 || w.body >= (int)S->bodies.size()) return false;
    auto L = lifeOf(*S);
    const Biota& B = L->bodies[w.body];
    if (w.species < 0 || w.species >= (int)B.species.size()) return false;
    const std::vector<Morph> forms = morphsOf(*S, w.body);
    form = forms[w.species];
    kind = cultureKindOf(B, w.species, 2 * form.bodyUm());
    const Microbe& s = B.species[w.species];
    slept = s.state != SS_DORMANT ? 0 : (s.fromBody >= 0 ? s.ageYears : B.dormantYears);
    std::vector<StrainRecord> chain;   // its strain's moves, the wild one's first
    for (int n = w.strainIn, guard = 0; n > 0 && guard < 64; guard++) {
        const auto it = guide.strains.find(flaskSpeciesKey(w) + "/S" + std::to_string(n));
        if (it == guide.strains.end()) break;
        chain.push_back(it->second);
        n = it->second.parent;
    }
    for (auto r = chain.rbegin(); r != chain.rend(); ++r) {
        applyStrain(kind, r->what, r->value);
        if (r->what == SM_SIZE) { form.lengthUm *= r->value; form.widthUm *= r->value; }
        if (r->what == SM_PIGMENT) form.pigment = (int)r->value;
    }
    return true;
}

// a flask's culture made again from its species and events: the kinds read from their worlds, its laws laid out, its forms, its names
void Game::flaskMake(int i) {
    if (i < 0 || i >= INCUBATOR_FLASKS) return;
    Flask& F = lab.flasks[i];
    F.culture = FlaskCulture(); F.run = CultureRun(); F.forms.clear();
    for (int k = 0; k < F.n && k < 2; k++) {
        CultureKind kind; Morph form; double slept = 0;
        if (!flaskKind(F.who[k], kind, form, slept)) { F.n = k; break; }   // its world no longer there
        // a later strain's moves on the form (its size, its pigment): the strains its events made
        for (const CultureEvent& e : F.events) if (e.kind == CD_STRAIN && e.who == k) {
            if (e.what == SM_SIZE) { form.lengthUm *= e.value; form.widthUm *= e.value; }
            if (e.what == SM_PIGMENT) form.pigment = (int)e.value;
        }
        F.culture.kind[k] = kind; F.forms.push_back(form); F.sleptYears[k] = slept;
        F.label[k] = flaskSpeciesName(F.who[k]);
    }
    F.culture.events = F.events;
    if (F.n > 0) F.run = runCulture(F.culture);
    F.made = true;
}
void Game::flaskMadeAll() { for (int i = 0; i < INCUBATOR_FLASKS; i++) if (!lab.flasks[i].made) flaskMake(i); }

CultureNow Game::flaskNow(int i) const {
    const Flask& F = lab.flasks[i];
    if (F.n == 0 || !F.made) return CultureNow();
    return cultureAt(F.culture, F.run, lab.clock);
}

// the lab's clock, every simulating frame (the drones' kind): what each flask comes to is said as the clock reaches it (its spores woken, a
// strain arisen: an event of its own then, the strain the guide's), its crowd's glow
void Game::updateLab(double realDt) {
    lab.clock += realDt;
    sequencerTick();   // Z-06: the sequencer's reading, its end as the clock reaches it
    for (int i = 0; i < INCUBATOR_FLASKS; i++) {
        Flask& F = lab.flasks[i];
        if (F.n == 0) continue;
        if (!F.made) flaskMake(i);
        for (int k = 0; k < F.n; k++) {
            if (F.run.strainT[k] < 0 || F.run.strainT[k] > lab.clock) continue;
            // a strain: its number its species' next, its record the guide's, the flask's event at the time it arose
            const std::string base = flaskSpeciesKey(F.who[k]);
            int n = 1;
            while (guide.strains.count(base + "/S" + std::to_string(n))) n++;
            StrainRecord r; r.parent = F.who[k].strain; r.what = F.run.strainWhat[k]; r.value = F.run.strainValue[k];
            guide.strains[base + "/S" + std::to_string(n)] = r;
            CultureEvent e; e.kind = CD_STRAIN; e.t = F.run.strainT[k]; e.m = F.medium; e.who = k; e.what = r.what; e.value = r.value;
            F.events.push_back(e);
            const std::string was = F.label[k];
            F.who[k].strain = n;
            F.told &= ~(FT_KEPT << k);   // R-433: the strain kept in its turn, as it grows
            flaskMake(i);
            const std::string line = fmt("A STRAIN OF %s IN FLASK %d: %s", was.c_str(), i + 1, moveWords(r.what, r.value).c_str());
            status(line + " - N NAMES IT", 6);
            logEvent("STRAIN", line);
            guide.save(guidePath);
        }
        for (int k = 0; k < F.n; k++)   // R-433: grown (its curve past its lag): kept in the collection
            if (!(F.told & (FT_KEPT << k)) && (cultureStagesBy(F.run, k, lab.clock) & LC_GROWING)) keepCulture(i, k);
        if (!(F.told & FT_WOKEN)) for (int k = 0; k < F.n; k++) {
            if (F.run.wokeT[k] < 0 || F.run.wokeT[k] > lab.clock) continue;
            F.told |= FT_WOKEN;
            const std::string line = F.sleptYears[k] > 0 ? fmt("FLASK %d: %s WOKEN AFTER %s", i + 1, F.label[k].c_str(), yearsWords(F.sleptYears[k]).c_str())
                                                         : fmt("FLASK %d: %s WOKEN", i + 1, F.label[k].c_str());
            status(line, 6);
            logEvent("WOKEN", line);
            MicrobeSheet& m = guide.microbes[flaskSpeciesKey(F.who[k])];
            m.stages |= LC_WAKING;
            m.sleptYears = std::max(m.sleptYears, F.sleptYears[k]);   // Z-07: the LIFE page's longest asleep
            guide.save(guidePath);
            break;
        }
        if (!(F.told & FT_GLOW)) {
            const CultureNow now = flaskNow(i);
            for (int k = 0; k < now.n; k++) if (now.one[k].glowing) { F.told |= FT_GLOW; status(fmt("FLASK %d GLOWS: %s, ITS CROWD'S OWN LIGHT", i + 1, F.label[k].c_str()), 5); break; }
        }
    }
}

// what the incubator's screen shows of a flask goes to its species' sheets: the stages its curve has passed, the quickest it was seen to grow
// and the medium then
void Game::flaskSeen(int i) {
    if (i < 0 || i >= INCUBATOR_FLASKS) return;
    Flask& F = lab.flasks[i];
    if (F.n == 0 || !F.made) return;
    const CultureNow now = flaskNow(i);
    bool changed = false;
    for (int k = 0; k < F.n; k++) {
        auto it = guide.microbes.find(flaskSpeciesKey(F.who[k]));
        if (it == guide.microbes.end()) continue;   // (put in from a sheet: never missing)
        MicrobeSheet& m = it->second;
        const uint32_t st = cultureStagesBy(F.run, k, lab.clock);
        if ((m.stages | st) != m.stages) { m.stages |= st; changed = true; }
        const CultureOne& o = now.one[k];
        if (o.stage == SG_GROWING || o.stage == SG_COLONY) {
            const Growth g = growthIn(F.culture.kind[k], F.medium);
            const double d = g.mu > 0 ? std::log(2.0) / g.mu : 0;
            if (d > 0 && (m.doublingS <= 0 || d < m.doublingS - 0.5)) {
                m.doublingS = std::round(d); m.cell = F.medium.cell; m.tempC = F.medium.tempC; m.light = F.medium.light; m.energy = F.medium.energy; m.salt = F.medium.salt;
                changed = true;
            }
        }
    }
    if (changed) guide.save(guidePath);
}

// ---- putting in, the medium, emptying -----------------------------------------------------------------------------------------------

// the chooser's rows: for a flask, each living or sleeping species seen in a cold store's vial (never a fossil or a protocell: they do not
// grow), each flask's culture (carried over) and (R-433) each culture of the collection; for the jars, the vials of wet silt, a mat or soil. R-432:
// in folders, a world's (a row a species and its state, cells or spores, from the first vial holding it), the flasks' and the collection's
void Game::incubatorChoices() {
    incub.choices.clear(); incub.choiceSel = 0; incub.choiceTop = 0;
    incub.pages = ChooserPage();
    std::vector<std::string> keys;
    LabSystems systems;
    if (incub.page == 1) {
        for (int v = 0; v < (int)coldStore.size(); v++) {
            const Vial& V = coldStore[v];
            if (!jarNiche(V.hab.niche)) continue;
            IncubatorChoice c; c.vial = v;
            c.line = fmt("%s, %.0f C", V.name.c_str(), std::fabs(V.readingC) < 0.5 ? 0.0 : V.readingC);
            incub.choices.push_back(c); keys.push_back(Guide::bodyKey(V.sx, V.sy, V.sz, V.body));
        }
        incub.pages.folders = chooserFolders(keys);
        return;
    }
    std::map<std::string, int> listed;   // a species' row by its key and its state (its cells and its spores apart)
    for (int v = 0; v < (int)coldStore.size(); v++) {
        const Vial& V = coldStore[v];
        const StarSystem* S = systems.get(sys, V.sx, V.sy, V.sz);
        if (!S || V.body < 0 || V.body >= (int)S->bodies.size()) continue;
        auto L = lifeOf(*S);
        const Biota& B = L->bodies[V.body];
        const Sample smp = sampleOf(B, V.hab);
        std::vector<Morph> forms;
        for (const Held& H : smp.held) {
            if (H.state == SS_FOSSIL || H.state == SS_PROTO) continue;
            const std::string world = Guide::bodyKey(V.sx, V.sy, V.sz, V.body), key = world + "/M" + std::to_string(H.species);
            const auto it = guide.microbes.find(key);
            if (it == guide.microbes.end() || !(it->second.seen & SEEN_FORM)) continue;   // only what was seen of it
            if (!listed.emplace(key + (H.state == SS_DORMANT ? "/D" : ""), (int)incub.choices.size()).second) continue;   // each once
            if (forms.empty()) forms = morphsOf(*S, V.body);
            const Morph& m = forms[H.species];
            const std::string nm = nameOfKey(key);
            IncubatorChoice c; c.vial = v; c.species = H.species;
            c.line = (nm == "UNKNOWN" ? std::string("UNNAMED") : upper(nm)) + " - " + (H.state == SS_DORMANT ? std::string(m.plan >= MP_AMOEBA ? "CYSTS" : "SPORES") + " FROM " : std::string("FROM ")) +
                     V.name;
            incub.choices.push_back(c); keys.push_back(world);
        }
    }
    for (int f = 0; f < INCUBATOR_FLASKS; f++) {
        if (f == incub.sel || lab.flasks[f].n == 0) continue;
        IncubatorChoice c; c.flask = f;
        c.line = fmt("FLASK %d: %s", f + 1, lab.flasks[f].label[0].c_str()) + (lab.flasks[f].n > 1 ? " AND " + lab.flasks[f].label[1] : std::string());
        incub.choices.push_back(c); keys.push_back("FLASKS");
    }
    for (const LabFolder& F : cultureFolders())   // R-433: the collection, its folders in their order
        for (int k : F.id >= 0 ? F.rows : std::vector<int>()) {
            IncubatorChoice c; c.kept = k;
            c.line = keptName(k);
            incub.choices.push_back(c); keys.push_back(keptFolderKey(k));
        }
    incub.pages.folders = chooserFolders(keys);
}

// a choice put in the flask chosen: a vial's species (its first, AS ITS WORLD the medium's first guess; or a second added), or a flask's culture
// carried over (its species and strains, its medium; or its first added as a second)
bool Game::flaskPut(int i, const IncubatorChoice& c) {
    if (i < 0 || i >= INCUBATOR_FLASKS) return false;
    Flask& F = lab.flasks[i];
    if (F.n >= 2) { refuse("TWO IN A FLASK ARE ENOUGH", 3); return false; }
    std::vector<FlaskSpecies> in;
    Medium asWorld = F.asWorld;
    if (c.flask >= 0 && c.flask < INCUBATOR_FLASKS && lab.flasks[c.flask].n > 0) {
        const Flask& S = lab.flasks[c.flask];
        for (int k = 0; k < S.n; k++) { FlaskSpecies w = S.who[k]; w.strainIn = w.strain; in.push_back(w); }
        if (F.n == 0) { F.medium = S.medium; asWorld = S.asWorld; }
    } else if (c.kept >= 0 && c.kept < (int)lab.kept.size()) {   // R-433: a kept culture, in the medium it grew in (AS ITS WORLD its world's)
        const KeptCulture& K = lab.kept[c.kept];
        FlaskSpecies w = K.who; w.strainIn = w.strain;
        in.push_back(w);
        if (F.n == 0) { F.medium = K.medium; asWorld = K.asWorld; }
    } else if (c.vial >= 0 && c.vial < (int)coldStore.size()) {
        const Vial& V = coldStore[c.vial];
        FlaskSpecies w; w.sx = V.sx; w.sy = V.sy; w.sz = V.sz; w.body = V.body; w.species = c.species;
        in.push_back(w);
        if (F.n == 0) {   // AS ITS WORLD: the vial's place's medium
            Star st; StarSystem S;
            const StarSystem* P = &sys;
            if (!(sys.valid && sys.star.sx == V.sx && sys.star.sy == V.sy && sys.star.sz == V.sz)) {
                if (!placeInSector(V.sx, V.sy, V.sz, st, true)) { refuse("THE VIAL'S WORLD IS NO LONGER THERE"); return false; }
                S.generate(st); P = &S;
            }
            if (V.body < 0 || V.body >= (int)P->bodies.size()) { refuse("THE VIAL'S WORLD IS NO LONGER THERE"); return false; }
            auto L = lifeOf(*P);
            const int cls = P->sunless() ? STAR_ROGUE : (P->ofCompanion(V.body) ? P->companionStar().cls : P->star.cls);
            asWorld = mediumOfPlace(L->bodies[V.body], P->bodies[V.body].type, cls, V.hab, V.readingC);
            F.medium = asWorld;
        }
    } else return false;
    for (const FlaskSpecies& w : in) for (int k = 0; k < F.n; k++) if (F.who[k].sameSpecies(w) && F.who[k].strain == w.strainIn) { refuse("IT IS IN THE FLASK ALREADY", 3); return false; }
    if (F.n + (int)in.size() > 2) { refuse("TWO IN A FLASK ARE ENOUGH", 3); return false; }
    const bool fresh = F.n == 0;
    for (const FlaskSpecies& w : in) {
        CultureEvent e; e.kind = F.n == 0 ? CD_START : CD_ADD; e.t = lab.clock; e.m = F.medium;
        if (F.n == 0) { F.events.clear(); F.told = 0; F.asWorld = asWorld; }
        F.who[F.n] = w; F.who[F.n].strain = w.strainIn;
        F.n++;
        F.events.push_back(e);
    }
    flaskMake(i);
    if (fresh) status(fmt(c.vial >= 0 ? "FLASK %d: %s PUT IN, AS ITS WORLD" : "FLASK %d: %s PUT IN", i + 1, F.label[0].c_str()), 4);
    else status(fmt("FLASK %d: %s ADDED", i + 1, F.label[F.n - 1].c_str()), 4);
    audio.beep = BEEP_UI;
    return true;
}

void Game::flaskEmpty(int i) {
    if (i < 0 || i >= INCUBATOR_FLASKS || lab.flasks[i].n == 0) return;
    lab.flasks[i] = Flask();
    lab.flasks[i].made = true;
    status(fmt("FLASK %d EMPTIED AND WASHED", i + 1), 3);
    audio.beep = BEEP_UI;
}

// the incubator's controls on a culture: its medium from now (a change of energy, salt or cell a fresh broth of it: the culture's)
void Game::flaskSet(int i, const Medium& m) {
    if (i < 0 || i >= INCUBATOR_FLASKS) return;
    Flask& F = lab.flasks[i];
    if (F.n == 0 || m == F.medium) return;
    F.medium = m;
    CultureEvent e; e.kind = CD_MEDIUM; e.t = lab.clock; e.m = m;
    F.events.push_back(e);
    flaskMake(i);
}

// F: a hundredth of the culture into fresh broth of its medium (a serial transfer: its lag again, its crowd's energy new)
void Game::flaskFresh(int i) {
    if (i < 0 || i >= INCUBATOR_FLASKS || lab.flasks[i].n == 0) return;
    Flask& F = lab.flasks[i];
    CultureEvent e; e.kind = CD_FRESH; e.t = lab.clock; e.m = F.medium;
    F.events.push_back(e);
    flaskMake(i);
    status(fmt("FLASK %d: A HUNDREDTH INTO FRESH BROTH", i + 1), 3);
    audio.beep = BEEP_UI;
}

Medium Game::flaskAsItsWorld(int i) const { return lab.flasks[clampi(i, 0, INCUBATOR_FLASKS - 1)].asWorld; }

// ---- the jars ---------------------------------------------------------------------------------------------------------------------------

void Game::jarMake(int j) {
    if (j < 0 || j >= (int)lab.jars.size()) return;
    Jar& J = lab.jars[j];
    J.made = true; J.recipe = JarRecipe();
    Star st; StarSystem S;
    const StarSystem* P = &sys;
    if (!(sys.valid && sys.star.sx == J.sx && sys.star.sy == J.sy && sys.star.sz == J.sz)) {
        if (!placeInSector(J.sx, J.sy, J.sz, st, true)) return;
        S.generate(st); P = &S;
    }
    if (J.body < 0 || J.body >= (int)P->bodies.size()) return;
    auto L = lifeOf(*P);
    const Biota& B = L->bodies[J.body];
    J.recipe = jarRecipe(B, sampleOf(B, J.hab), J.hab);
}

// a jar of a vial's wet silt: one a world on the shelf (its places eight); the vial's sample stays in the cold store
bool Game::jarPut(int vial) {
    if (vial < 0 || vial >= (int)coldStore.size()) return false;
    const Vial& V = coldStore[vial];
    if (!jarNiche(V.hab.niche)) { refuse("A JAR NEEDS WET SILT, A MAT OR SOIL", 3); return false; }
    for (const Jar& J : lab.jars) if (J.sx == V.sx && J.sy == V.sy && J.sz == V.sz && J.body == V.body) { refuse("THE SHELF HOLDS THIS WORLD'S JAR ALREADY", 3); return false; }
    if ((int)lab.jars.size() >= JAR_PLACES) { refuse(fmt("THE SHELF'S %d PLACES ARE FULL", JAR_PLACES), 3); return false; }
    Jar J;
    J.sx = V.sx; J.sy = V.sy; J.sz = V.sz; J.body = V.body; J.hab = V.hab; J.name = V.name; J.t0 = lab.clock;
    lab.jars.push_back(J);
    jarMake((int)lab.jars.size() - 1);
    status("A JAR OF " + V.name + " ON THE SHELF - IT SETTLES OVER TWENTY MINUTES", 4);
    audio.beep = BEEP_UI;
    return true;
}
void Game::jarEmpty(int j) {
    if (j < 0 || j >= (int)lab.jars.size()) return;
    lab.jars.erase(lab.jars.begin() + j);
    status("THE JAR IS EMPTIED", 3);
    audio.beep = BEEP_UI;
}

// ---- the screen ------------------------------------------------------------------------------------------------------------------------

void Game::incubatorEnter() {
    incub.choosing = false;
    incub.sel = clampi(incub.sel, 0, INCUBATOR_FLASKS - 1);
    flaskMadeAll();
    for (int j = 0; j < (int)lab.jars.size(); j++) if (!lab.jars[j].made) jarMake(j);
}

void Game::updateIncubator(const Input& in, double dt, double realDt) {
    if (in.wasPressed(KEY_E) && !in.ctrl()) { closeScreen(); audio.beep = BEEP_UI; return; }
    if (incub.choosing) {   // R-432: its folders, then the rows of the one open
        const int r = incub.choices.empty() ? -2 : chooserKeys(in, incub.pages, incub.choiceSel);
        if (r == -2) { incub.choosing = false; audio.beep = BEEP_UI; return; }
        if (r >= 0 && r < (int)incub.choices.size()) {
            const IncubatorChoice c = incub.choices[r];
            incub.choosing = false;
            if (incub.page == 1) jarPut(c.vial); else flaskPut(incub.sel, c);
        }
        return;
    }
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); audio.beep = BEEP_UI; return; }
    if (saveKey(in)) { saveSlot(currentSlot); return; }
    if (in.wasPressed(KEY_TAB)) { incub.page = 1 - incub.page; audio.beep = BEEP_UI; return; }
    if (incub.page == 1) {   // the jars
        menuStep(in, incub.jar, JAR_PLACES);
        const bool has = incub.jar < (int)lab.jars.size();
        if (enterKey(in)) {
            if (has) refuse("THE JAR IS SETTLING - BACKSPACE EMPTIES IT", 3);
            else {
                incubatorChoices();
                if (incub.choices.empty()) refuse("NO VIAL OF WET SILT, A MAT OR SOIL IN THE COLD STORE", 3);
                else { incub.choosing = true; audio.beep = BEEP_UI; }
            }
        }
        if (has && (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE))) jarEmpty(incub.jar);
        return;
    }
    menuStep(in, incub.sel, INCUBATOR_FLASKS);
    Flask& F = lab.flasks[incub.sel];
    if (in.wasPressed(KEY_LEFT)) { incub.field = (incub.field + 4) % 5; audio.beep = BEEP_UI; }
    if (in.wasPressed(KEY_RIGHT)) { incub.field = (incub.field + 1) % 5; audio.beep = BEEP_UI; }
    const int step = in.wasPressed(KEY_EQUAL) ? 1 : (in.wasPressed(KEY_MINUS) ? -1 : 0);
    if (step != 0) {
        if (F.n == 0) refuse("PUT A CULTURE IN FIRST - ENTER", 2);
        else {
            Medium m = F.medium;
            double lo, hi;
            switch (incub.field) {
                case 0: m.cell = (m.cell + step + CC_COUNT) % CC_COUNT; cellRange(m.cell, lo, hi); m.tempC = clampd(m.tempC, lo, hi); break;
                case 1: cellRange(m.cell, lo, hi); m.tempC = clampd(m.tempC + step * (in.shift() ? 10 : 1), lo, hi); break;
                case 2: m.light = (m.light + step + CL_COUNT) % CL_COUNT; break;
                case 3: m.energy = (m.energy + step + CE_COUNT) % CE_COUNT; break;
                default: m.salt = (m.salt + step + 3) % 3; break;
            }
            flaskSet(incub.sel, m);
            audio.beep = BEEP_UI;
        }
    }
    if (in.wasPressed(KEY_W)) {
        if (F.n == 0) refuse("PUT A CULTURE IN FIRST - ENTER", 2);
        else { flaskSet(incub.sel, F.asWorld); status(fmt("FLASK %d: THE MEDIUM AS ITS WORLD", incub.sel + 1), 3); audio.beep = BEEP_UI; }
    }
    if (in.wasPressed(KEY_F) && !in.ctrl()) { if (F.n == 0) refuse("THE FLASK IS EMPTY", 2); else flaskFresh(incub.sel); }
    if (enterKey(in)) {
        if (F.n >= 2) refuse("TWO IN A FLASK ARE ENOUGH", 3);
        else {
            incubatorChoices();
            if (incub.choices.empty()) refuse("NOTHING SEEN TO PUT IN - THE MICROSCOPE SHOWS WHAT A VIAL HOLDS", 4);
            else { incub.choosing = true; audio.beep = BEEP_UI; }
        }
    }
    if (F.n > 0 && (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE))) flaskEmpty(incub.sel);
    if (in.wasPressed(KEY_N) && !in.ctrl()) {
        int k = -1;
        for (int j = 0; j < F.n && k < 0; j++) if (F.who[j].strain > 0) k = j;
        if (k < 0) { refuse("NO STRAIN IN THE FLASK TO NAME - ONE COMES OF A CULTURE HELD AT ITS RANGE'S EDGE", 4); return; }
        textStrainKey = flaskStrainKey(F.who[k]);
        const std::string named = nameOfKey(textStrainKey);
        beginTextEntry("NAME THE STRAIN", TEXT_STRAIN, named == "UNKNOWN" ? std::string() : upper(named));
        return;
    }
    flaskSeen(incub.sel);
}

void Game::incubatorPad(PadMenu& m) const {
    m.title = "THE INCUBATOR";
    if (incub.choosing) { m.item(incub.pages.open ? "PUT IT IN" : "OPEN THE FOLDER", KEY_ENTER); m.item("BACK", KEY_ESCAPE); return; }
    if (incub.page == 1) {
        m.item("A JAR FROM A VIAL", KEY_ENTER); m.item("EMPTY THE JAR", KEY_BACKSPACE); m.item("THE FLASKS", KEY_TAB); m.item("BACK TO THE LABORATORY", KEY_ESCAPE);
        m.button(PR_A, KEY_ENTER, "JAR"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_SELECT, KEY_TAB, "FLASKS");
        return;
    }
    const Flask& F = lab.flasks[incub.sel];
    m.item(F.n == 0 ? "PUT IN A CULTURE" : "ADD A SECOND", KEY_ENTER);
    m.item("THE NEXT FIELD", KEY_RIGHT); m.item("RAISE IT", KEY_EQUAL); m.item("LOWER IT", KEY_MINUS);
    m.item("AS ITS WORLD", KEY_W); m.item("FRESH BROTH", KEY_F); m.item("EMPTY THE FLASK", KEY_BACKSPACE); m.item("NAME THE STRAIN", KEY_N);
    m.item("THE JARS", KEY_TAB); m.item("BACK TO THE LABORATORY", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "PUT IN"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_X, KEY_W, "AS WORLD"); m.button(PR_Y, KEY_F, "FRESH");
    m.button(PR_LB, KEY_MINUS, "LOWER"); m.button(PR_RB, KEY_EQUAL, "RAISE"); m.button(PR_SELECT, KEY_TAB, "JARS");
}

// ---- the save's lines: the lab's clock, each flask's medium and species and events, the jars (the line after them the screen's own) -----

void Game::incubatorSave(std::string& lines) const {
    std::ostringstream f;
    f.precision(17);
    f << "lab " << lab.clock << "\n";
    auto medium = [&](const Medium& m) { f << m.cell << " " << m.tempC << " " << m.light << " " << m.energy << " " << m.salt; };
    for (int i = 0; i < INCUBATOR_FLASKS; i++) {
        const Flask& F = lab.flasks[i];
        if (F.n == 0) continue;
        f << "flask " << i << " " << F.n << " " << F.told << " ";
        medium(F.medium); f << " "; medium(F.asWorld); f << "\n";
        for (int k = 0; k < F.n; k++) {
            const FlaskSpecies& w = F.who[k];
            f << "flaskwho " << i << " " << k << " " << w.sx << " " << w.sy << " " << w.sz << " " << w.body << " " << w.species << " " << w.strain << " " << w.strainIn << "\n";
        }
        for (const CultureEvent& e : F.events) {
            f << "flaskev " << i << " " << e.kind << " " << e.t << " ";
            medium(e.m);
            f << " " << e.who << " " << e.what << " " << e.value << "\n";
        }
    }
    for (const Jar& J : lab.jars)
        f << "jar " << J.sx << " " << J.sy << " " << J.sz << " " << J.body << " " << J.hab.niche << " " << J.hab.meanC << " " << J.hab.salt << " "
          << ((J.hab.shade ? 1 : 0) | (J.hab.bloom ? 2 : 0)) << " " << (J.hab.oldRock ? 1 : 0) << " " << J.hab.place << " " << J.t0 << " " << J.name << "\n";
    keptSave(f);   // R-433: the collection's folders and cultures
    lines += f.str();
}

bool Game::incubatorLoad(const std::string& text) {
    std::istringstream is(text);
    std::string key;
    is >> key;
    auto medium = [&](Medium& m) { is >> m.cell >> m.tempC >> m.light >> m.energy >> m.salt; m.cell = clampi(m.cell, 0, CC_COUNT - 1); m.light = clampi(m.light, 0, CL_COUNT - 1);
                                   m.energy = clampi(m.energy, 0, CE_COUNT - 1); m.salt = clampi(m.salt, 0, 2); };
    if (key == "lab") { double c = 0; is >> c; if (!is.fail()) lab.clock = c; return true; }
    if (key == "flask") {
        int i = -1, n = 0; uint32_t told = 0; Medium m, w;
        is >> i >> n >> told; medium(m); medium(w);
        if (is.fail() || i < 0 || i >= INCUBATOR_FLASKS) return true;
        Flask& F = lab.flasks[i];
        F = Flask(); F.n = clampi(n, 0, 2); F.told = told; F.medium = m; F.asWorld = w;
        return true;
    }
    if (key == "flaskwho") {
        int i = -1, k = -1; FlaskSpecies w;
        is >> i >> k >> w.sx >> w.sy >> w.sz >> w.body >> w.species >> w.strain >> w.strainIn;
        if (!is.fail() && i >= 0 && i < INCUBATOR_FLASKS && k >= 0 && k < 2) { lab.flasks[i].who[k] = w; lab.flasks[i].made = false; }
        return true;
    }
    if (key == "flaskev") {
        int i = -1; CultureEvent e;
        is >> i >> e.kind >> e.t; medium(e.m); is >> e.who >> e.what >> e.value;
        if (!is.fail() && i >= 0 && i < INCUBATOR_FLASKS && e.kind >= 0 && e.kind < CD_COUNT) { lab.flasks[i].events.push_back(e); lab.flasks[i].made = false; }
        return true;
    }
    if (key == "jar") {
        Jar J; int shade = 0, old = 0;
        is >> J.sx >> J.sy >> J.sz >> J.body >> J.hab.niche >> J.hab.meanC >> J.hab.salt >> shade >> old >> J.hab.place >> J.t0;
        if (is.fail() || J.hab.niche < 0 || J.hab.niche >= NI_COUNT) return true;
        std::getline(is, J.name); if (!J.name.empty() && J.name[0] == ' ') J.name.erase(0, 1);
        J.hab.shade = (shade & 1) != 0; J.hab.bloom = (shade & 2) != 0; J.hab.oldRock = old != 0;
        if ((int)lab.jars.size() < JAR_PLACES) lab.jars.push_back(J);
        return true;
    }
    return keptLoad(key, is);   // R-433
}

void Game::incubatorReset() { lab = LabState(); incub = IncubatorState(); for (Flask& F : lab.flasks) F.made = true; }
