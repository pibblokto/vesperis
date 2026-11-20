// Y-02: the gene editor (R-443; docs/ideas/IDEAS-genes.md 1.4 and 1.5, ITERATION-1.5.0.md Y-02; lab_state.h, life/genes.h), the laboratory's
// fourth device: a screen over the room, E at it on the bow bench past the sequencer. Enter (Tab once one is on) chooses a culture read on the
// sequencer, a flask's or a kept one; its genes on its genesis's map in two columns of nine places, each NOT KNOWN till seen (then its words,
// or its name: N). The arrows choose a gene, Enter opens its working part (Left and Right a letter, Up and Down it through the alphabet), C cuts
// it out (again: back in), Backspace makes it its parent's again; W writes the line in EDIT_S on the lab's clock, the screen closed or not,
// into an empty flask in its culture's medium (with none then, into the collection): the guide's `strain` line (SM_EDIT and its edits), said
// and logged (EDIT). What a gene does is learned as an edited line shows it against its parent: its flask's fit, its end and its glow
// (`flaskLearn`), its drop's shape, size, colour, motion and answers (`scopeLearn`, `scopeAnswerLearn`), a vital gene's death; and as a strain
// is read on the sequencer (`strainReadLearn`). Of a line's changed genes, those that could show it (not known to do another thing; NOTHING
// SEEN in a drop not one a drop shows): one is learned, several are said as candidates
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <sstream>

namespace {
uint32_t editColour(float k = 1) {
    const RGB& c = Game::cabinDevices().row[FACE_EDITOR].col;
    return rgb((int)clampd(c.r * k * 255, 0, 255), (int)clampd(c.g * k * 255, 0, 255), (int)clampd(c.b * k * 255, 0, 255));
}
uint32_t editText() {   // its colour a quarter toward white: its words on the dark screen
    const RGB& c = Game::cabinDevices().row[FACE_EDITOR].col;
    return rgb((int)(255 * (c.r * 0.75 + 0.25)), (int)(255 * (c.g * 0.75 + 0.25)), (int)(255 * (c.b * 0.75 + 0.25)));
}
constexpr int MAP_ROWS = GENE_PLACES / 2;   // the map's two columns
// "3", "3 OR 7", "3, 7 OR 9"
std::string placesWords(const std::vector<int>& p, const char* last) {
    std::string s;
    for (size_t i = 0; i < p.size(); i++) s += (i == 0 ? "" : (i + 1 == p.size() ? std::string(" ") + last + " " : std::string(", "))) + std::to_string(p[i] + 1);
    return s;
}
bool dropShows(int kind) { return kind == GN_SHAPE || kind == GN_SIZE || kind == GN_PIGMENT || kind == GN_MOTION; }
double ratio(double a, double b) { return a > 0 && b > 0 ? std::max(a / b, b / a) : (a == b ? 1 : 1e9); }
const char* planName(int p) { return p == MP_SPHERE ? "SPHERES" : (p == MP_ROD ? "RODS" : (p == MP_CURVED ? "CURVED" : (p == MP_SPIRAL ? "SPIRALS" : "CELLS"))); }
}   // namespace

// ---- a line's words: its number's letter, its edits -----------------------------------------------------------------------------------

bool Game::lineEdited(const std::string& lineKey) const {
    const auto it = guide.strains.find(lineKey);
    return it != guide.strains.end() && it->second.what == SM_EDIT;
}
std::string Game::lineTagOf(const std::string& speciesKey, int n) const { return (lineEdited(speciesKey + "/S" + std::to_string(n)) ? "E" : "S") + std::to_string(n); }
std::string Game::lineTag(const FlaskSpecies& w) const { return lineTagOf(flaskSpeciesKey(w), w.strain); }
std::string Game::lineMoveWords(const std::string& lineKey) const {
    const auto it = guide.strains.find(lineKey);
    if (it == guide.strains.end()) return "A STRAIN";
    if (it->second.what != SM_EDIT) return strainMoveWords(it->second.what, it->second.value);
    std::vector<int> changed, cut;
    for (const GeneEdit& e : geneEditsOf(it->second.edits)) (e.cut ? cut : changed).push_back(e.place);
    std::string s;
    if (!changed.empty()) s = (changed.size() == 1 ? "GENE " : "GENES ") + placesWords(changed, "AND") + " CHANGED";
    if (!cut.empty()) s += (s.empty() ? "" : ", ") + std::string(cut.size() == 1 ? "GENE " : "GENES ") + placesWords(cut, "AND") + " CUT OUT";
    return s.empty() ? std::string("EDITED") : s;
}

// ---- what a gene does, as learned ------------------------------------------------------------------------------------------------------

std::string Game::geneKey(uint64_t genesis, int place) const { return fmt("%016llx/%d", (unsigned long long)genesis, place + 1); }
const GeneKnown* Game::geneKnown(uint64_t genesis, int place) const {
    const auto it = guide.genes.find(geneKey(genesis, place));
    return it == guide.genes.end() ? nullptr : &it->second;
}
std::string Game::geneWords(uint64_t genesis, int place) const {
    const std::string named = nameOfKey("G" + geneKey(genesis, place));
    if (named != "UNKNOWN") return upper(named);
    const GeneKnown* g = geneKnown(genesis, place);
    if (!g) return "NOT KNOWN";
    return g->kind >= 0 && g->kind < GN_COUNT ? GENE_KIND_NAMES[g->kind] : "NOTHING SEEN";
}

void Game::geneSeen(int flask, int who, int kind, const std::string& what, int how) {
    if (flask < 0 || flask >= INCUBATOR_FLASKS || who < 0 || who > 1) return;
    Flask& F = lab.flasks[flask];
    if (!F.edited[who] || (F.geneTold[who] & (1u << kind))) return;
    F.geneTold[who] |= 1u << kind;
    std::vector<int> cand;
    int known = -1;
    for (int p : F.changed[who]) {
        const GeneKnown* g = geneKnown(F.genesis[who], p);
        if (g && g->kind == kind) { known = p; break; }
        if (g && g->kind >= 0) continue;                                         // known to do another thing
        if (g && g->how == GENE_SEEN_NOTHING && dropShows(kind)) continue;      // nothing seen in a drop: not what a drop shows
        cand.push_back(p);
    }
    // what was seen and what it says of the genes: logged at once (the lab's notebook), said in its turn (`geneSay`)
    const std::string seen = fmt("FLASK %d: ", flask + 1) + what;
    std::string says;
    if (known >= 0) says = fmt("GENE %d, %s", known + 1, GENE_KIND_NAMES[kind]);
    else if (cand.size() == 1) {
        GeneKnown g; g.kind = kind; g.how = how;
        guide.genes[geneKey(F.genesis[who], cand[0])] = g;
        says = fmt("GENE %d IS %s", cand[0] + 1, GENE_KIND_NAMES[kind]);
        lab.edit.made = false;   // the editor's map says it
    } else if (!cand.empty()) says = std::string(GENE_KIND_NAMES[kind]) + ": GENE " + placesWords(cand, "OR");
    logEvent("GENE", says.empty() ? seen : seen + ": " + says);
    guide.save(guidePath);
    lab.edit.said.push_back({fitText(seen, UW - 12), fitText(says, UW - 12)});
}

// the genes' findings said one at a time, each when the status line is free: what was seen, then what it says (`statusNext`)
void Game::geneSay() {
    auto& q = lab.edit.said;
    if (q.empty() || !statusNext.empty() || realTime < statusUntil) return;
    status(q.front().first, 5);
    if (!q.front().second.empty()) { statusNext = q.front().second; statusNextSecs = 6; }
    q.erase(q.begin());
    audio.beep = BEEP_UI;
}

// an edited line against its parent as the lab's clock runs, alone in its flask: a vital gene's death at once; where it grows and its parent
// would not, or the other way (its warmth, its salt, its food; its pigment's light), its end (resting or dying), its glow
void Game::flaskLearn(int i) {
    Flask& F = lab.flasks[i];
    if (!F.made || F.n == 0) return;
    double t0 = 0;
    for (const CultureEvent& e : F.events) if (e.kind == CD_START) { t0 = e.t; break; }
    const CultureNow now = flaskNow(i);
    for (int k = 0; k < F.n && k < 2; k++) {
        if (!F.edited[k] || F.changed[k].empty()) continue;
        const CultureKind& L = F.culture.kind[k];
        const CultureKind& P = F.parentKind[k];
        if (L.dead) { geneSeen(i, k, GN_VITAL, "IT DID NOT LIVE", GENE_SEEN_DEATH); continue; }
        if (F.n != 1 || lab.clock - t0 < 5) continue;   // its own fit alone, its curve begun
        const CultureOne& o = now.one[k];
        if (o.stage == SG_DEAD || o.stage == SG_EMPTY || o.stage == SG_ASLEEP) continue;
        const Medium& m = F.medium;
        const Growth gl = growthIn(L, m), gp = growthIn(P, m);
        const bool lg = gl.fit == GF_GROWS, pg = gp.fit == GF_GROWS;
        auto warm = [](int f) { return f == GF_HOT || f == GF_COLD; };
        if ((lg && warm(gp.fit)) || (warm(gl.fit) && pg) || (warm(gl.fit) && warm(gp.fit) && gl.fit != gp.fit))
            geneSeen(i, k, GN_WARMTH, lg ? fmt("IT GROWS AT %.0f C - ITS PARENT %s", m.tempC, gp.fit == GF_HOT ? "DIES" : "WAITS")
                                         : fmt("TOO %s AT %.0f C - ITS PARENT %s", gl.fit == GF_HOT ? "HOT" : "COLD", m.tempC, pg ? "GROWS" : (gp.fit == GF_HOT ? "DIES" : "WAITS")));
        if ((lg && gp.fit == GF_SALT) || (gl.fit == GF_SALT && pg))
            geneSeen(i, k, GN_SALT, lg ? fmt("IT GROWS IN %s - ITS PARENT DIES", CULTURE_SALT_NAMES[m.salt]) : fmt("%s KILLS IT - ITS PARENT GROWS", CULTURE_SALT_NAMES[m.salt]));
        if (L.s.way == LW_CHEMICAL && ((lg && gp.fit == GF_HUNGRY) || (gl.fit == GF_HUNGRY && pg)))
            geneSeen(i, k, GN_FOOD, lg ? fmt("IT EATS %s - ITS PARENT CANNOT", CULTURE_ENERGY_NAMES[m.energy]) : fmt("IT CANNOT EAT %s - ITS PARENT CAN", CULTURE_ENERGY_NAMES[m.energy]));
        if (L.s.way == LW_LIGHT && lg && pg && m.light != CL_DARK && L.s.pigment != P.s.pigment && ratio(gl.mu, gp.mu) > 1.5)
            geneSeen(i, k, GN_PIGMENT, fmt("%s THAN ITS PARENT IN %s LIGHT", gl.mu > gp.mu ? "FASTER" : "SLOWER", CULTURE_LIGHT_NAMES[m.light]));
        const int dl = cultureDecline(L), dp = cultureDecline(P);
        if (o.fit == GF_GROWS && o.stage == dl && dl != dp)
            geneSeen(i, k, GN_REST, dl == SG_DYING ? std::string("IT DIES AT ITS END - ITS PARENT RESTS") : std::string("IT RESTS AT ITS END - ITS PARENT DIES"));
        if (o.glowing && !P.s.glows) geneSeen(i, k, GN_GLOW, "IT GLOWS - ITS PARENT DOES NOT");
        if (!o.glowing && P.s.glows && !L.s.glows && o.stage == SG_CROWD) geneSeen(i, k, GN_GLOW, "NO GLOW AT ITS CROWD - ITS PARENT GLOWS");
    }
}

// a flask's edited line in view on the microscope: its cells' shape, size, colour (in a bright field), motion (where its kind moves) against its
// parent's; nothing different there, grown past its lag: its changed genes not known NOTHING SEEN
void Game::scopeLearn() {
    if (scope.page != 1 || scope.flask < 0 || scope.flask >= INCUBATOR_FLASKS) return;
    Flask& F = lab.flasks[scope.flask];
    for (int k = 0; k < F.n && k < 2 && k < (int)F.forms.size(); k++) {
        if (!F.edited[k] || F.culture.kind[k].dead || F.changed[k].empty()) continue;
        bool looked = false, active = false;
        for (size_t j = 0; j < scope.mount.kinds.size() && j < scope.told.size(); j++) {
            const SlideKind& K = scope.mount.kinds[j];
            if (K.species != k || !(scope.told[j] & SCOPE_LOOKED) || K.form != CF_CELL) continue;
            looked = true; active = active || K.active;
        }
        if (!looked) continue;
        const Morph& L = F.forms[k];
        const Morph& P = F.parentForm[k];
        bool differs = false;
        if (L.plan != P.plan || ratio(L.aspect, P.aspect) > 1.3) {
            differs = true;
            geneSeen(scope.flask, k, GN_SHAPE, L.plan != P.plan ? fmt("ITS CELLS %s, ITS PARENT'S %s", planName(L.plan), planName(P.plan))
                                                                  : fmt("ITS CELLS %s THAN ITS PARENT'S", L.aspect > P.aspect ? "LONGER" : "SHORTER"));
        }
        if (ratio(L.widthUm, P.widthUm) > 1.2) {   // its cells' width (a new shape keeps it)
            differs = true;
            geneSeen(scope.flask, k, GN_SIZE, fmt("ITS CELLS %s ITS PARENT'S", L.widthUm > P.widthUm ? (L.widthUm > 1.8 * P.widthUm ? "TWICE AS WIDE AS" : "WIDER THAN")
                                                                                                    : (L.widthUm < P.widthUm / 1.8 ? "HALF AS WIDE AS" : "NARROWER THAN")));
        }
        if (L.pigment != P.pigment) {
            differs = true;
            if (scope.seeing == WS_BRIGHT)
                geneSeen(scope.flask, k, GN_PIGMENT, std::string(L.pigment ? PIGMENT_NAMES[L.pigment] : "PALE") + ", ITS PARENT'S " + (P.pigment ? PIGMENT_NAMES[P.pigment] : "PALE"));
        }
        if (L.motion != P.motion || ratio(L.speed, P.speed) > 1.4) {
            differs = true;
            if (active)
                geneSeen(scope.flask, k, GN_MOTION, L.motion == MO_NONE ? std::string("ITS CELLS ARE STILL - ITS PARENT'S MOVE")
                                                   : (P.motion == MO_NONE ? std::string("ITS CELLS MOVE - ITS PARENT'S ARE STILL")
                                                   : (L.motion != P.motion ? std::string("IT MOVES ANOTHER WAY: ") + MOTION_NAMES[L.motion]
                                                   : fmt("IT MOVES %s THAN ITS PARENT", L.speed > P.speed ? "FASTER" : "SLOWER"))));
        }
        if (L.taxis != P.taxis || F.culture.kind[k].s.flees != F.parentKind[k].s.flees) differs = true;   // a sense: its answers show it
        if (differs || (F.geneTold[k] & (1u << GN_QUIET)) || !(cultureStagesBy(F.run, k, lab.clock) & LC_GROWING)) continue;
        F.geneTold[k] |= 1u << GN_QUIET;   // grown and looked at, nothing different: NOTHING SEEN
        std::vector<int> fresh;
        for (int p : F.changed[k]) {
            const GeneKnown* g = geneKnown(F.genesis[k], p);
            if (g) continue;
            GeneKnown n; n.how = GENE_SEEN_NOTHING;
            guide.genes[geneKey(F.genesis[k], p)] = n;
            fresh.push_back(p);
        }
        if (fresh.empty()) continue;
        const std::string seen = fmt("FLASK %d: NOTHING DIFFERENT IN ITS DROP", scope.flask + 1);
        const std::string says = fmt("GENE%s %s: NOTHING SEEN", fresh.size() == 1 ? "" : "S", placesWords(fresh, "AND").c_str());
        lab.edit.said.push_back({seen, fitText(says, UW - 12)});
        logEvent("GENE", seen + ": " + says);
        guide.save(guidePath);
        lab.edit.made = false;
    }
}

// its answers on the slide (R-437's, as they are told; `unanswered` an experiment judged with no answer from it while it moved): to the light,
// away from it, to the food, along the field where its parent would not, or none where its parent would
void Game::scopeAnswerLearn(int kind, uint32_t bits, int unanswered) {
    if (scope.flask < 0 || scope.flask >= INCUBATOR_FLASKS || kind < 0 || kind >= (int)scope.mount.kinds.size()) return;
    const int k = scope.mount.kinds[kind].species;
    Flask& F = lab.flasks[scope.flask];
    if (k < 0 || k > 1 || !F.edited[k]) return;
    const Microbe& P = F.parentKind[k].s;
    const bool pLight = (P.taxis & TX_LIGHT) != 0, pFlees = fleesLight(P), pFood = (P.taxis & TX_FOOD) != 0, pField = (P.taxis & TX_FIELD) != 0;
    if ((bits & SA_LIGHT) && !pLight) geneSeen(scope.flask, k, GN_SENSE, "IT GATHERS IN THE LIGHT, NOT ITS PARENT");
    else if ((bits & SA_FLEES) && !pFlees) geneSeen(scope.flask, k, GN_SENSE, "IT FLEES THE LIGHT, NOT ITS PARENT");
    else if ((bits & SA_FOOD) && !pFood) geneSeen(scope.flask, k, GN_SENSE, "IT SWIMS TO THE FOOD, NOT ITS PARENT");
    else if ((bits & SA_FIELD) && !pField) geneSeen(scope.flask, k, GN_SENSE, "IT SWIMS ALONG THE FIELD, NOT ITS PARENT");
    else if (unanswered == SD_LIGHT && (pLight || pFlees)) geneSeen(scope.flask, k, GN_SENSE, "ITS PARENT ANSWERS THE LIGHT, NOT IT");
    else if (unanswered == SD_FOOD && pFood) geneSeen(scope.flask, k, GN_SENSE, "ITS PARENT FOLLOWS THE FOOD, NOT IT");
    else if (unanswered == SD_MAGNET && pField) geneSeen(scope.flask, k, GN_SENSE, "ITS PARENT FOLLOWS THE FIELD, NOT IT");
}

// a strain's one gene whose letters differ from its parent's (its move's: -1 none, or an edited line), and its genesis
int Game::strainGene(const FlaskSpecies& w, uint64_t& genesis) const {
    genesis = 0;
    if (w.strain <= 0) return -1;
    const auto it = guide.strains.find(flaskSpeciesKey(w) + "/S" + std::to_string(w.strain));
    if (it == guide.strains.end() || it->second.what == SM_EDIT) return -1;
    FlaskSpecies a = w, b = w;
    a.strainIn = a.strain;
    b.strain = b.strainIn = it->second.parent;
    CultureKind ka, kb; Morph fa, fb; double slept = 0;
    MicrobeGenes ga, gb;
    if (!flaskKind(a, ka, fa, slept, &ga) || !flaskKind(b, kb, fb, slept, &gb) || !ga.valid) return -1;
    const std::vector<int> d = genesDiffer(ga, gb);
    genesis = ga.genesis.id;
    return d.size() == 1 ? d[0] : -1;
}

// the sequencer's reading of a strain (its move a flask's): the gene its move was in learned (its kind the move's), said after the reading
std::string Game::strainReadLearn() {
    const SequencerState& Q = lab.seq;
    uint64_t genesis = 0;
    const int p = strainGene(Q.who, genesis);
    if (p < 0) return "";
    const int what = guide.strains.at(flaskSpeciesKey(Q.who) + "/S" + std::to_string(Q.who.strain)).what;
    const int kind = what == SM_HOT || what == SM_COLD ? GN_WARMTH : (what == SM_SIZE ? GN_SIZE : (what == SM_PIGMENT ? GN_PIGMENT : GN_SALT));
    const GeneKnown* g = geneKnown(genesis, p);
    if (g && g->kind >= 0) return "";
    GeneKnown n; n.kind = kind; n.how = GENE_SEEN_STRAIN;
    guide.genes[geneKey(genesis, p)] = n;
    const std::string line = fmt("GENE %d IS %s: A STRAIN READ", p + 1, GENE_KIND_NAMES[kind]);
    logEvent("GENE", fmt("A STRAIN READ: GENE %d DIFFERS FROM ITS PARENT'S - GENE %d IS %s", p + 1, p + 1, GENE_KIND_NAMES[kind]));
    lab.edit.made = false;
    return line;
}

// ---- the editor: its chooser, what is on it, the edits, the writing ---------------------------------------------------------------------

// the chooser's rows: each flask's species and line (as it is now), then the collection's (a pair's two each), by folder; those whose species
// the sequencer has not read marked (refused)
void Game::editorChoices() {
    EditorState& E = lab.edit;
    E.choices.clear(); E.choiceSel = 0;
    E.pages = ChooserPage();
    std::vector<std::string> keys, looks, rests, traits;
    std::set<std::string> listed;
    auto read = [&](const FlaskSpecies& w) { const auto it = guide.microbes.find(flaskSpeciesKey(w)); return it != guide.microbes.end() && (it->second.seen & SEEN_KIN); };
    flaskMadeAll();
    for (int f = 0; f < INCUBATOR_FLASKS; f++)
        for (int k = 0; k < lab.flasks[f].n && k < (int)lab.flasks[f].forms.size(); k++) {
            const Flask& F = lab.flasks[f];
            FlaskSpecies w = F.who[k]; w.strainIn = w.strain;
            if (!listed.insert("FLASKS|" + flaskStrainKey(w)).second) continue;
            EditorChoice c; c.who = w; c.medium = F.medium; c.asWorld = F.asWorld; c.from = fmt("FLASK %d", f + 1); c.read = read(w);
            E.choices.push_back(c); keys.push_back("FLASKS"); traits.push_back("");
            looks.push_back(fmt("FLASK %d: ", f + 1) + flaskSpeciesWords(w, F.forms[k], false)); rests.push_back(c.read ? "" : " (NOT READ)");
        }
    for (const LabFolder& L : cultureFolders())
        for (int k : L.id >= 0 ? L.rows : std::vector<int>())
            for (int one = 0; one < (lab.kept[k].paired ? 2 : 1); one++) {
                const KeptCulture& K = lab.kept[k];
                FlaskSpecies w = one == 1 ? K.partner : K.who; w.strainIn = w.strain;
                if (!listed.insert(keptFolderKey(k) + "|" + flaskStrainKey(w)).second) continue;
                EditorChoice c; c.who = w; c.medium = K.medium; c.asWorld = K.asWorld; c.from = "THE COLLECTION"; c.read = read(w);
                std::string trait;
                const std::string look = keptRow(k, &trait, one);
                E.choices.push_back(c); keys.push_back(keptFolderKey(k)); looks.push_back(look); rests.push_back(c.read ? "" : " (NOT READ)"); traits.push_back(trait);
            }
    apartRows(looks, rests, keys, traits);
    for (size_t i = 0; i < E.choices.size(); i++) E.choices[i].line = looks[i] + rests[i];
    E.pages.folders = chooserFolders(keys);
}

bool Game::editorPut(const EditorChoice& c) {
    if (!c.read) { refuse("READ IT ON THE SEQUENCER FIRST", 3); return false; }
    EditorState& E = lab.edit;
    E.on = true; E.who = c.who; E.who.strainIn = E.who.strain; E.medium = c.medium; E.asWorld = c.asWorld; E.from = c.from;
    E.edits.clear(); E.writ.clear(); E.letters = false; E.cursor = 0; E.writing = false;
    editorMake();
    if (!E.on || !E.genes.valid) { E = EditorState(); refuse("NO GENES TO EDIT IN IT", 3); return false; }
    E.sel = E.genes.genes.empty() ? 0 : E.genes.genes[0].place;
    status(fitText("ON THE GENE EDITOR: " + E.name + ", FROM " + E.from, UW - 12), 4);
    audio.beep = BEEP_UI;
    return true;
}

// what is on it made again (not saved): its genes (its lines' from its species'), its genesis's code, its kind and form, the edited ones, its words
void Game::editorMake() {
    EditorState& E = lab.edit;
    E.made = true;
    if (!E.on) return;
    double slept = 0;
    FlaskSpecies w = E.who; w.strainIn = w.strain;
    if (!flaskKind(w, E.kind, E.form, slept, &E.genes)) { E.on = false; return; }
    E.genesis = E.genes.genesis;
    E.code = geneCodeOf(E.genesis);
    E.name = flaskSpeciesWords(w, E.form, false);
    editorEdited();
}
void Game::editorEdited() {
    EditorState& E = lab.edit;
    E.editedKind = E.kind; E.editedForm = E.form; E.editedGenes = E.genes;
    applyEdits(E.editedKind, E.editedForm, E.editedGenes, E.writing ? E.writ : E.edits);
}

// a gene's edit made: its letters or its cut, dropped where it is its parent's again; the edited line made again
void Game::editorSet(int place, bool cut, const std::string& part) {
    EditorState& E = lab.edit;
    const MicrobeGene* G = E.genes.at(place);
    if (!G) return;
    auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == place; });
    const bool parents = !cut && part == G->part;
    if (parents) { if (it != E.edits.end()) E.edits.erase(it); }
    else {
        GeneEdit e; e.place = place; e.cut = cut; e.part = cut ? G->part : part;
        if (it != E.edits.end()) *it = e; else E.edits.push_back(e);
        std::sort(E.edits.begin(), E.edits.end(), [](const GeneEdit& a, const GeneEdit& b) { return a.place < b.place; });
    }
    editorEdited();
}

bool Game::editorWrite() {
    EditorState& E = lab.edit;
    if (!E.on) return false;
    if (E.writing) { refuse("IT IS WRITING ITS LINE", 2); return false; }
    if (E.edits.empty()) { refuse("NOTHING CHANGED - EDIT A GENE FIRST", 3); return false; }
    bool empty = false;
    for (const Flask& F : lab.flasks) empty = empty || F.n == 0;
    if (!empty) { refuse("NO EMPTY FLASK FOR IT - EMPTY ONE ON THE INCUBATOR", 3); return false; }
    E.writing = true; E.t0 = lab.clock; E.writ = E.edits; E.letters = false;
    editorEdited();
    status("WRITING ITS LINE: TWO MINUTES, THEN INTO A FLASK", 4);
    audio.beep = BEEP_UI;
    return true;
}

// the writing's end, as the lab's clock reaches it (`updateLab`, the screen open or not): the line its species' next (the guide's), into the first
// empty flask in its culture's medium (none then: into the collection), said and logged; its edits stay on the editor
void Game::editorTick() {
    EditorState& E = lab.edit;
    if (!E.on || !E.writing) return;
    if (!E.made) editorMake();
    if (!E.on || lab.clock < E.t0 + EDIT_S) return;
    E.writing = false;
    const std::string base = flaskSpeciesKey(E.who);
    int n = 1;
    while (guide.strains.count(base + "/S" + std::to_string(n))) n++;
    StrainRecord r; r.parent = E.who.strain; r.what = SM_EDIT; r.edits = geneEditsText(E.writ);
    guide.strains[base + "/S" + std::to_string(n)] = r;
    FlaskSpecies w = E.who; w.strain = w.strainIn = n;
    CultureKind kind; Morph form; double slept = 0;
    flaskKind(w, kind, form, slept);
    const std::string words = flaskSpeciesWords(w, form, false);
    int f = -1;
    for (int i = 0; i < INCUBATOR_FLASKS && f < 0; i++) if (lab.flasks[i].n == 0) f = i;
    std::string line;
    if (f >= 0) {
        Flask& F = lab.flasks[f];
        F = Flask(); F.n = 1; F.who[0] = w; F.medium = E.medium; F.asWorld = E.asWorld;
        CultureEvent e; e.kind = CD_START; e.t = lab.clock; e.m = F.medium;
        F.events.push_back(e);
        flaskMake(f);
        line = fmt("FLASK %d: %s, WRITTEN BY THE GENE EDITOR", f + 1, words.c_str());
    } else {
        KeptCulture c; c.who = w; c.medium = E.medium; c.asWorld = E.asWorld; c.t = lab.clock; c.plan = form.plan;
        lab.kept.push_back(c);
        line = fmt("NO EMPTY FLASK: KEPT IN THE COLLECTION, %s", words.c_str());
    }
    E.writ.clear();
    editorEdited();
    status(fitText(line, UW - 12), 6);
    logEvent("EDIT", line + ": " + lineMoveWords(base + "/S" + std::to_string(n)));
    guide.save(guidePath);   // (its twig on its world's tree as the tree is next built: Y-01)
    audio.beep = BEEP_LOCK;
}

void Game::editorEnter() {
    lab.edit.choosing = false; lab.edit.letters = false;
    if (lab.edit.on && !lab.edit.made) editorMake();
    audio.beep = BEEP_UI;
}

// its working part open: Left and Right a letter, Up and Down it through the alphabet, Backspace its parent's letter there; Enter or Esc the map
void Game::editorLetterKeys(const Input& in) {
    EditorState& E = lab.edit;
    const MicrobeGene* G = E.genes.at(E.sel);
    const MicrobeGene* H = E.editedGenes.at(E.sel);
    if (!G || !H || H->cut || E.writing) { E.letters = false; return; }
    if (in.wasPressed(KEY_ESCAPE) || enterKey(in)) { E.letters = false; audio.beep = BEEP_UI; return; }
    const int n = (int)H->part.size();
    if (in.wasPressed(KEY_LEFT)) E.cursor = (E.cursor + n - 1) % n;
    if (in.wasPressed(KEY_RIGHT)) E.cursor = (E.cursor + 1) % n;
    E.cursor = clampi(E.cursor, 0, n - 1);
    const char* abc = alphabetOf(E.genes.letters);
    const int L = (int)std::strlen(abc);
    std::string part = H->part;
    int d = (in.wasPressed(KEY_UP) ? 1 : 0) - (in.wasPressed(KEY_DOWN) ? 1 : 0);
    if (d != 0) {
        const char* at = std::strchr(abc, part[E.cursor]);
        const int i = at ? (int)(at - abc) : 0;
        part[E.cursor] = abc[(i + d + L) % L];
        editorSet(E.sel, false, part);
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE)) {
        part[E.cursor] = G->part[E.cursor];
        editorSet(E.sel, false, part);
        audio.beep = BEEP_UI;
    }
}

void Game::updateEditor(const Input& in, double dt, double realDt) {
    EditorState& E = lab.edit;
    if (E.choosing) {
        const int r = E.choices.empty() ? -2 : chooserKeys(in, E.pages, E.choiceSel);
        if (r == -2) { E.choosing = false; return; }
        if (r >= 0 && r < (int)E.choices.size()) { E.choosing = false; editorPut(E.choices[r]); }
        return;
    }
    if (E.on && !E.made) editorMake();
    if (E.letters) { editorLetterKeys(in); return; }
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    auto choose = [&] {
        if (E.writing) { refuse("IT IS WRITING ITS LINE", 2); return; }
        editorChoices();
        if (E.choices.empty()) { refuse("NO CULTURE TO EDIT: A FLASK OR THE COLLECTION FIRST", 3); return; }
        E.choosing = true; audio.beep = BEEP_UI;
    };
    if (!E.on) { if (enterKey(in)) choose(); return; }
    if (in.wasPressed(KEY_TAB)) { choose(); return; }
    int col = E.sel / MAP_ROWS, row = E.sel % MAP_ROWS;
    if (in.wasPressed(KEY_UP)) row = (row + MAP_ROWS - 1) % MAP_ROWS;
    if (in.wasPressed(KEY_DOWN)) row = (row + 1) % MAP_ROWS;
    if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_RIGHT)) col = 1 - col;
    E.sel = col * MAP_ROWS + row;
    if (in.wasPressed(KEY_N) && !in.ctrl()) {   // the gene named (its genesis's: every kin's)
        textGeneKey = "G" + geneKey(E.genesis.id, E.sel);
        const std::string named = nameOfKey(textGeneKey);
        beginTextEntry(fmt("NAME GENE %d", E.sel + 1), TEXT_GENE, named == "UNKNOWN" ? std::string() : upper(named));
        return;
    }
    const bool act = enterKey(in) || (in.wasPressed(KEY_C) && !in.ctrl()) || in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE);
    if (in.wasPressed(KEY_W) && !in.ctrl()) { editorWrite(); return; }
    if (!act) return;
    if (E.writing) { refuse(fmt("IT IS WRITING ITS LINE - %.0f SECONDS TO GO", std::ceil(std::max(0.0, E.t0 + EDIT_S - lab.clock))), 2); return; }
    const MicrobeGene* G = E.genes.at(E.sel);
    if (!G) { refuse(fmt("NO GENE %d IN IT - ITS KIN MAY HAVE ONE", E.sel + 1), 3); return; }
    if (G->cut) { refuse("CUT OUT IN ITS PARENT - EDIT THE PARENT", 3); return; }
    const auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == E.sel; });
    const bool cutNow = it != E.edits.end() && it->cut;
    if (enterKey(in)) {
        if (cutNow) { refuse("CUT OUT - C PUTS IT BACK", 2); return; }
        E.letters = true; E.cursor = 0; audio.beep = BEEP_UI;
        return;
    }
    if (in.wasPressed(KEY_C)) {
        editorSet(E.sel, !cutNow, G->part);
        status(fmt(cutNow ? "GENE %d BACK IN" : "GENE %d CUT OUT", E.sel + 1), 2);
        audio.beep = BEEP_UI;
        return;
    }
    if (it != E.edits.end()) { E.edits.erase(it); editorEdited(); status(fmt("GENE %d AS ITS PARENT'S", E.sel + 1), 2); audio.beep = BEEP_UI; }
}

void Game::renderEditor() {
    EditorState& E = lab.edit;
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 218);
    drawText(canvas, 6, 4, "THE GENE EDITOR", editText());
    if (!E.on) {
        drawTextCentered(canvas, UW / 2, UH / 2 - 12, "EMPTY", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH / 2, "ENTER CHOOSES A CULTURE READ ON THE SEQUENCER", HUD_DIM);
    } else {
        const char* tab = "TAB A CULTURE";
        drawText(canvas, UW - 6 - textWidth(tab), 4, tab, HUD_DIM);
        drawText(canvas, 6, 14, fitHead(E.name, ", FROM " + E.from, UW - 12).c_str(), HUD_WHITE);
        // the map: two columns of nine places, each its number, its words and its mark (= changed, - cut out, a gap's dash)
        const int my = 26, cw = (UW - 12) / 2;
        for (int p = 0; p < GENE_PLACES; p++) {
            const int x = 6 + (p / MAP_ROWS) * cw, y = my + (p % MAP_ROWS) * 9;
            const MicrobeGene* G = E.genes.at(p);
            const MicrobeGene* H = E.editedGenes.at(p);
            const bool sel = p == E.sel;
            const bool edited = std::any_of(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == p; });
            if (sel) fillRectRGB(canvas, x - 2, y - 1, x + cw - 6, y + 7, editColour(0.35f));
            drawText(canvas, x, y, fmt("%2d", p + 1).c_str(), G ? editText() : HUD_DIM);
            std::string w = !G ? std::string("-") : (H && H->cut ? std::string("CUT OUT") : geneWords(E.genesis.id, p));
            const uint32_t col = !G ? HUD_DIM : (H && H->cut ? HUD_RED : (w == "NOT KNOWN" ? HUD_DIM : HUD_GREEN));
            drawText(canvas, x + 16, y, fitText(w, cw - 34).c_str(), col);
            if (edited) drawText(canvas, x + cw - 16, y, H && H->cut ? "-" : "=", HUD_AMBER);
        }
        // the gene chosen: its number, its words and length; its working part (the edited letters bright); its value where it is known
        int y = my + MAP_ROWS * 9 + 4;
        const MicrobeGene* G = E.genes.at(E.sel);
        const MicrobeGene* H = E.editedGenes.at(E.sel);
        if (!G) drawText(canvas, 6, y, fmt("GENE %d: NOT IN IT - ITS KIN MAY HAVE ONE", E.sel + 1).c_str(), HUD_DIM);
        else {
            const std::string words = geneWords(E.genesis.id, E.sel);
            drawText(canvas, 6, y, fitText(fmt("GENE %d, %s LETTERS: ", E.sel + 1, thousands(G->length).c_str()) + (words == "NOT KNOWN" ? "WHAT IT DOES NOT KNOWN" : words), UW - 12).c_str(), HUD_WHITE);
            y += 11;
            drawText(canvas, 6, y, "ITS WORKING PART:", HUD_DIM);
            const int lx = 6 + textWidth("ITS WORKING PART: ");
            const std::string& part = H ? H->part : G->part;
            for (int i = 0; i < (int)part.size(); i++) {
                const char s[2] = {part[i], 0};
                const bool changed = H && i < (int)G->part.size() && part[i] != G->part[i];
                drawText(canvas, lx + i * 11, y, s, H && H->cut ? HUD_DIM : letterDye(part[i], changed ? 1.0f : 0.6f));
                if (changed) drawLineRGB(canvas, lx + i * 11, y + 8, lx + i * 11 + 5, y + 8, letterDye(part[i]));
                if (E.letters && i == E.cursor) drawText(canvas, lx + i * 11, y + 9, "^", HUD_WHITE);
            }
            y += 20;
            const GeneKnown* k = geneKnown(E.genesis.id, E.sel);
            if (k && k->kind >= 0 && H) {   // its value as its letters give it, beside its parent's where they differ
                auto bare = [](std::string w) { return w.compare(0, 3, "IT ") == 0 ? w.substr(3) : w; };
                const std::string now = bare(geneValueWords(E.editedKind, E.editedForm, *H, E.code)), was = bare(geneValueWords(E.kind, E.form, *G, E.code));
                drawText(canvas, 6, y, fitText((now == was ? "" : "NOW: ") + now, UW - 12).c_str(), HUD_GREEN); y += 9;
                if (now != was) { drawText(canvas, 6, y, fitText("ITS PARENT: " + was, UW - 12).c_str(), HUD_DIM); y += 9; }
            } else { drawText(canvas, 6, y, k ? "NOTHING SEEN WHEN IT CHANGED" : "WHAT IT DOES: CHANGE IT, GROW IT AND LOOK", HUD_DIM); y += 9; }
        }
        // the edits, the writing
        y = std::max(y + 2, my + MAP_ROWS * 9 + 49);
        if (E.writing) {
            const double share = clampd((lab.clock - E.t0) / EDIT_S, 0, 1);
            drawRectRGB(canvas, 6, y, UW - 7, y + 5, editColour(0.4f));
            fillRectRGB(canvas, 7, y + 1, 7 + (int)std::lround((UW - 15) * share), y + 4, editColour());
            drawText(canvas, 6, y + 8, fmt("WRITING ITS LINE: %.0f SECONDS TO GO", std::ceil(std::max(0.0, E.t0 + EDIT_S - lab.clock))).c_str(), HUD_DIM);
        } else if (!E.edits.empty()) {
            std::vector<int> changed, cut;
            for (const GeneEdit& e : E.edits) (e.cut ? cut : changed).push_back(e.place);
            std::string s = "ITS EDITS:";
            if (!changed.empty()) s += " = " + placesWords(changed, "AND");
            if (!cut.empty()) s += " - " + placesWords(cut, "AND");
            drawText(canvas, 6, y, fitText(s, UW - 12).c_str(), HUD_AMBER);
        }
    }
    if (E.letters) {
        drawTextCentered(canvas, UW / 2, UH - 21, "<- -> A LETTER  UP DOWN CHANGES IT", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 12, "BACKSPACE ITS PARENT'S  ENTER THE MAP", HUD_DIM);
    } else if (E.writing) {
        drawTextCentered(canvas, UW / 2, UH - 21, "ARROWS A GENE  N NAMES IT", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 12, "IT WRITES ON WITH THE SCREEN CLOSED", HUD_DIM);
    } else if (E.on) {
        drawTextCentered(canvas, UW / 2, UH - 21, "ARROWS A GENE  ENTER ITS LETTERS  C CUT  BKSP UNDO", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 12, "N NAMES THE GENE  W WRITES IT INTO A FLASK", HUD_DIM);
    }
    if (E.choosing) {   // the chooser over it: the cultures' folders, then the open one's rows
        const int rowsN = 14, x0 = 20, y0 = 30;
        drawPanel(canvas, x0 - 6, y0 - 14, UW - x0 + 6, y0 + rowsN * 9 + 18, rgb(8, 8, 14), 235, editColour(0.6f));
        const std::string head = E.pages.open && E.pages.folder < (int)E.pages.folders.size() ? "WHAT TO EDIT: " + E.pages.folders[E.pages.folder].name : std::string("WHAT TO EDIT");
        drawText(canvas, x0, y0 - 10, fitText(head, UW - 2 * x0).c_str(), editText());
        std::vector<std::string> lines;
        for (const EditorChoice& c : E.choices) lines.push_back(c.line);
        drawChooser(canvas, x0 - 6, y0, UW - x0, y0 + rowsN * 9 + 14, E.pages, E.choiceSel, E.pages.top, lines, editColour());
    }
}

void Game::editorPad(PadMenu& m) const {
    const EditorState& E = lab.edit;
    m.title = "THE GENE EDITOR";
    if (E.choosing) { m.item(E.pages.open ? "EDIT IT" : "OPEN THE FOLDER", KEY_ENTER); m.item("BACK", KEY_ESCAPE); return; }
    if (E.letters) {
        m.item("THE NEXT LETTER", KEY_RIGHT); m.item("CHANGE IT", KEY_UP); m.item("ITS PARENT'S LETTER", KEY_BACKSPACE); m.item("BACK TO THE MAP", KEY_ENTER);
        m.button(PR_A, KEY_ENTER, "MAP"); m.button(PR_B, KEY_ESCAPE, "MAP");
        return;
    }
    if (!E.on) { m.item("A CULTURE TO EDIT", KEY_ENTER); m.item("BACK TO THE LABORATORY", KEY_ESCAPE); m.button(PR_A, KEY_ENTER, "CHOOSE"); m.button(PR_B, KEY_ESCAPE, "BACK"); return; }
    m.item("ITS LETTERS", KEY_ENTER); m.item("CUT IT OUT", KEY_C); m.item("AS ITS PARENT'S", KEY_BACKSPACE); m.item("NAME THE GENE", KEY_N);
    m.item("WRITE THE LINE", KEY_W); m.item("ANOTHER CULTURE", KEY_TAB); m.item("BACK TO THE LABORATORY", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "LETTERS"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_X, KEY_C, "CUT"); m.button(PR_Y, KEY_W, "WRITE");
    m.button(PR_SELECT, KEY_TAB, "CULTURE");
}

// ---- the save's lines: what is on it, its edits, its writing ------------------------------------------------------------------------------

void Game::editorSave(std::string& lines) const {
    const EditorState& E = lab.edit;
    if (!E.on) return;
    std::ostringstream f;
    f.precision(17);
    auto medium = [&](const Medium& m) { f << m.cell << " " << m.tempC << " " << m.light << " " << m.energy << " " << m.salt; };
    f << "editor " << E.who.sx << " " << E.who.sy << " " << E.who.sz << " " << E.who.body << " " << E.who.species << " " << E.who.strain << " ";
    medium(E.medium); f << " "; medium(E.asWorld);
    f << " " << (E.writing ? 1 : 0) << " " << E.t0 << " " << E.sel << "\n";
    if (!E.edits.empty()) f << "editoredits " << geneEditsText(E.edits) << "\n";
    if (E.writing && !E.writ.empty()) f << "editorwrit " << geneEditsText(E.writ) << "\n";
    f << "editorfrom " << E.from << "\n";
    lines += f.str();
}

bool Game::editorLoad(const std::string& text) {
    std::istringstream is(text);
    std::string key;
    is >> key;
    EditorState& E = lab.edit;
    auto medium = [&](Medium& m) { is >> m.cell >> m.tempC >> m.light >> m.energy >> m.salt; m.cell = clampi(m.cell, 0, CC_COUNT - 1); m.light = clampi(m.light, 0, CL_COUNT - 1);
                                   m.energy = clampi(m.energy, 0, CE_COUNT - 1); m.salt = clampi(m.salt, 0, 2); };
    if (key == "editor") {
        EditorState N; int writing = 0;
        is >> N.who.sx >> N.who.sy >> N.who.sz >> N.who.body >> N.who.species >> N.who.strain;
        medium(N.medium); medium(N.asWorld);
        is >> writing >> N.t0 >> N.sel;
        if (is.fail()) return true;
        N.who.strainIn = N.who.strain; N.on = true; N.writing = writing != 0; N.sel = clampi(N.sel, 0, GENE_PLACES - 1); N.made = false;
        E = N;
        return true;
    }
    if (key == "editoredits" || key == "editorwrit") {
        std::string t; is >> t;
        (key == "editoredits" ? E.edits : E.writ) = geneEditsOf(t);
        E.made = false;
        return true;
    }
    if (key == "editorfrom") {
        std::getline(is, E.from);
        if (!E.from.empty() && E.from[0] == ' ') E.from.erase(0, 1);
        return true;
    }
    return false;
}

void Game::editorReset() { lab.edit = EditorState(); }
