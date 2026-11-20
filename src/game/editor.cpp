// Y-02: the gene editor (R-443; docs/ideas/IDEAS-genes.md 1.4 and 1.5, ITERATION-1.5.0.md Y-02; lab_state.h, life/genes.h), the laboratory's
// fourth device: a screen over the room, E at it on the bow bench past the sequencer. Enter (Tab once one is on) chooses a culture read on the
// sequencer, a flask's or a kept one; its genes on its genesis's map in two columns of nine places, each NOT KNOWN till seen (then its words,
// or its name: N). The arrows choose a gene, Enter opens its working part (Left and Right a letter, Up and Down it through the alphabet), C cuts
// it out (again: back in), Backspace makes it its parent's again; W writes the line in EDIT_S on the lab's clock, the screen closed or not,
// into an empty flask in its culture's medium (with none then, into the collection): the guide's `strain` line (SM_EDIT and its edits), said
// and logged (EDIT). What a gene does is learned as an edited line shows it against its parent: its flask's fit, its end and its glow
// (`flaskLearn`), its drop's shape, size, colour, motion and answers (`scopeLearn`, `scopeAnswerLearn`), a vital gene's death; and as a strain
// is read on the sequencer (`strainReadLearn`). Of a line's changed genes, those that could show it (not known to do another thing; NOTHING
// SEEN in a drop not one a drop shows): one is learned, several are said as candidates. Y-04: S switches the gene chosen (`editorSwitch`); a
// switched line is learned from as its flask's state has it (`flaskLineIn`), a gene switched alone and on there as its parent's
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
// Y-03: what a gene needs, as said once it shows nothing for want of it (`GeneNeed`)
const char* needWords(int need) {
    switch (need) {
        case NEED_MOTION: return "A SENSE DOES NOTHING IN A CELL THAT CANNOT MOVE";
        case NEED_EATER: return "FOOD GENE UNUSED: NOT A CHEMICAL EATER";
        case NEED_LIGHT: return "PIGMENT ONLY COLOURS: NOT A LIGHT FEEDER";
        case NEED_SHAPE: return "SHAPE GENE WORKS ONLY ON SPHERES, RODS AND SPIRALS";
        case NEED_MOVER: return "MOTION GENE UNUSED: THIS FORM CANNOT MOVE";
        default: return "";
    }
}
// Y-04: a flask line's gene whose edit is a switch alone, on in a state (-1 none): as its parent's there, it shows nothing of itself
bool switchedOn(const Flask& F, int who, int place, int state) {
    if (state < 0 || std::find(F.switchOnly[who].begin(), F.switchOnly[who].end(), place) == F.switchOnly[who].end()) return false;
    const MicrobeGene* G = F.genes[who].at(place);
    return G && switchOn(G->sw, (uint32_t)state);
}
// how far a gene came, as the editor says it
const char* farWords(int far) {
    return far == GFROM_WORLD ? "SAME-WORLD KIN" : (far == GFROM_SYSTEM ? "SAME-SYSTEM KIN" : (far == GFROM_STAR ? "KIN FROM ANOTHER STAR" : "THE SAME SPECIES"));
}
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
    std::vector<int> changed, cut, moved, switched;
    std::vector<std::pair<int, int>> copies;
    for (const GeneEdit& e : geneEditsOf(it->second.edits)) {
        if (e.sw >= 0) switched.push_back(e.place);   // Y-04
        if (e.cut) cut.push_back(e.place);
        else if (e.how == GX_MOVED) moved.push_back(e.place);   // Y-03
        else if (e.how == GX_COPY) copies.push_back({e.place, e.from});
        else if (!e.switchOnly()) changed.push_back(e.place);
    }
    std::string s;
    auto add = [&](const std::vector<int>& p, const char* what) {
        if (!p.empty()) s += (s.empty() ? "" : ", ") + std::string(p.size() == 1 ? "GENE " : "GENES ") + placesWords(p, "AND") + what;
    };
    add(changed, " CHANGED"); add(cut, " CUT OUT"); add(moved, " MOVED IN");
    for (const auto& c : copies) s += (s.empty() ? "" : ", ") + fmt("GENE %d COPIED FROM %d", c.first + 1, c.second + 1);
    add(switched, " SWITCHED");
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

void Game::geneSeen(int flask, int who, int kind, const std::string& what, int how, int state) {
    if (flask < 0 || flask >= INCUBATOR_FLASKS || who < 0 || who > 1) return;
    Flask& F = lab.flasks[flask];
    if (!F.edited[who] || (F.geneTold[who] & (1u << kind))) return;
    F.geneTold[who] |= 1u << kind;
    std::vector<int> cand;
    int known = -1;
    for (int p : F.changed[who]) {
        if (switchedOn(F, who, p, state)) continue;   // Y-04: a switch alone, on: as its parent's there
        const GeneKnown* g = geneKnown(F.genesis[who], p);
        if (g && g->kind == kind) { known = p; break; }
        if (g && g->kind >= 0) continue;                                         // known to do another thing
        if (g && g->how == GENE_SEEN_NOTHING && dropShows(kind)) continue;      // nothing seen in a drop: not what a drop shows
        cand.push_back(p);
    }
    // what was seen and what it says of the genes: logged at once (the lab's notebook), said in its turn (`geneSay`)
    const std::string seen = fmt("FLASK %d: ", flask + 1) + what;
    std::string says;
    if (known >= 0) says = fmt("GENE %d: %s GENE", known + 1, GENE_KIND_NAMES[kind]);
    else if (cand.size() == 1) {
        GeneKnown g; g.kind = kind; g.how = how;
        guide.genes[geneKey(F.genesis[who], cand[0])] = g;
        says = fmt("GENE %d IS THE %s GENE", cand[0] + 1, GENE_KIND_NAMES[kind]);
        lab.edit.made = false;   // the editor's map says it
    } else if (!cand.empty()) says = std::string(GENE_KIND_NAMES[kind]) + " GENE: " + placesWords(cand, "OR");
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

// Y-03: what a gene needs, said once a flask's line (a place's bit) where a known gene of it shows nothing for want of it: logged, said in turn
void Game::geneNeedSay(int flask, int who, int place, const std::string& seen, const std::string& says) {
    if (flask < 0 || flask >= INCUBATOR_FLASKS || who < 0 || who > 1 || place < 0 || place >= GENE_PLACES) return;
    Flask& F = lab.flasks[flask];
    if (F.needTold[who] & (1u << place)) return;
    F.needTold[who] |= 1u << place;
    const std::string line = fmt("FLASK %d: ", flask + 1) + seen;
    logEvent("GENE", line + ": " + says);
    guide.save(guidePath);
    lab.edit.said.push_back({fitText(line, UW - 12), fitText(says, UW - 12)});
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
        if (F.culture.kind[k].dead) { geneSeen(i, k, GN_VITAL, "DID NOT LIVE", GENE_SEEN_DEATH); continue; }
        const CultureOne& o = now.one[k];
        const int st = o.state;   // Y-04: a switched line (and its parent) as its flask's state has it now
        CultureKind L, P; Morph lf, pf;
        flaskLineIn(i, k, st, L, lf); flaskLineIn(i, k, st, P, pf, true);
        if (L.dead && !P.dead) { geneSeen(i, k, GN_VITAL, "DIES HERE, PARENT LIVES", GENE_SEEN_DEATH, st); continue; }
        if (F.n != 1 || lab.clock - t0 < 5) continue;   // its own fit alone, its curve begun
        if (o.stage == SG_DEAD || o.stage == SG_EMPTY || o.stage == SG_ASLEEP) continue;
        const Medium& m = F.medium;
        const Growth gl = growthIn(L, m), gp = growthIn(P, m);
        const bool lg = gl.fit == GF_GROWS, pg = gp.fit == GF_GROWS;
        auto warm = [](int f) { return f == GF_HOT || f == GF_COLD; };
        auto seen = [&](int kind, const std::string& what) { geneSeen(i, k, kind, what, GENE_SEEN_EDIT, st); };
        if ((lg && warm(gp.fit)) || (warm(gl.fit) && pg) || (warm(gl.fit) && warm(gp.fit) && gl.fit != gp.fit))
            seen(GN_WARMTH, lg ? fmt("GROWS AT %.0f C, PARENT %s", m.tempC, gp.fit == GF_HOT ? "DIES" : "WAITS")
                               : fmt("TOO %s AT %.0f C, PARENT %s", gl.fit == GF_HOT ? "HOT" : "COLD", m.tempC, pg ? "GROWS" : (gp.fit == GF_HOT ? "DIES" : "WAITS")));
        if ((lg && gp.fit == GF_SALT) || (gl.fit == GF_SALT && pg))
            seen(GN_SALT, lg ? fmt("GROWS IN %s, PARENT DIES", CULTURE_SALT_NAMES[m.salt]) : fmt("DIES IN %s, PARENT GROWS", CULTURE_SALT_NAMES[m.salt]));
        if (L.s.way == LW_CHEMICAL && ((lg && gp.fit == GF_HUNGRY) || (gl.fit == GF_HUNGRY && pg)))
            seen(GN_FOOD, lg ? fmt("EATS %s, PARENT CANNOT", CULTURE_ENERGY_NAMES[m.energy]) : fmt("CANNOT EAT %s, PARENT CAN", CULTURE_ENERGY_NAMES[m.energy]));
        if (L.s.way == LW_LIGHT && lg && pg && m.light != CL_DARK && L.s.pigment != P.s.pigment && ratio(gl.mu, gp.mu) > 1.5)
            seen(GN_PIGMENT, fmt("GROWS %s THAN PARENT IN %s LIGHT", gl.mu > gp.mu ? "FASTER" : "SLOWER", CULTURE_LIGHT_NAMES[m.light]));
        const int dl = cultureDecline(L), dp = cultureDecline(P);
        if (o.fit == GF_GROWS && o.stage == dl && dl != dp)
            seen(GN_REST, dl == SG_DYING ? std::string("DIES WHEN STARVED, PARENT RESTS") : std::string("RESTS WHEN STARVED, PARENT DIES"));
        if (o.glowing && !P.s.glows) seen(GN_GLOW, "GLOWS, PARENT DOES NOT");
        if (o.glowing && P.s.glows && L.glowAny && !P.glowAny && o.cells < P.quorum * std::pow(10.0, gp.crowdLog))   // Y-04: lit by its switch
            seen(GN_GLOW, "GLOWS EARLY, PARENT DOES NOT");
        if (!o.glowing && P.s.glows && !L.s.glows && o.stage == SG_CROWD) seen(GN_GLOW, "NO GLOW WHEN FULL, PARENT GLOWS");
        // Y-03: a food or a pigment known, moved into one that lives another way: what it needs, once it grows
        for (size_t j = 0; j < F.changed[k].size() && j < F.changedKind[k].size() && lg; j++) {
            const int p = F.changed[k][j], kind = F.changedKind[k][j];
            const GeneKnown* g = geneKnown(F.genesis[k], p);
            if (!g || g->kind != kind) continue;
            MicrobeGene t; t.kind = kind;
            const int need = geneNeeds(L, t);
            if (need == NEED_EATER) geneNeedSay(i, k, p, fmt("GENE %d (FOOD): EATS NOTHING NEW", p + 1), needWords(need));
            if (need == NEED_LIGHT && m.light != CL_DARK) geneNeedSay(i, k, p, fmt("GENE %d (PIGMENT): NOT FED BY LIGHT", p + 1), needWords(need));
        }
    }
}

// a flask's edited line in view on the microscope: its cells' shape, size, colour (in a bright field), motion (where its kind moves) against its
// parent's; nothing different there, grown past its lag: its changed genes not known NOTHING SEEN
void Game::scopeLearn() {
    if (scope.page != 1 || scope.flask < 0 || scope.flask >= INCUBATOR_FLASKS) return;
    Flask& F = lab.flasks[scope.flask];
    for (int k = 0; k < F.n && k < 2 && k < (int)F.forms.size(); k++) {
        if (!F.edited[k] || F.culture.kind[k].dead || F.changed[k].empty()) continue;
        const int st = scope.lineState[k];   // Y-04: a switched line (and its parent) as its drop was taken
        CultureKind LK, PK; Morph L, P;
        flaskLineIn(scope.flask, k, st, LK, L); flaskLineIn(scope.flask, k, st, PK, P, true);
        if (LK.dead) continue;
        auto seen = [&](int kind, const std::string& what) { geneSeen(scope.flask, k, kind, what, GENE_SEEN_EDIT, st); };
        bool looked = false, active = false;
        for (size_t j = 0; j < scope.mount.kinds.size() && j < scope.told.size(); j++) {
            const SlideKind& K = scope.mount.kinds[j];
            if (K.species != k || !(scope.told[j] & SCOPE_LOOKED) || K.form != CF_CELL) continue;
            looked = true; active = active || K.active;
        }
        if (!looked) continue;
        // Y-03: a known gene that cannot work in it (a sense in a still cell, a shape outside the four small cells', a motion in a still form)
        for (size_t j = 0; j < F.changed[k].size() && j < F.changedKind[k].size(); j++) {
            const int p = F.changed[k][j], kind = F.changedKind[k][j];
            const GeneKnown* g = geneKnown(F.genesis[k], p);
            if (!g || g->kind != kind) continue;
            MicrobeGene t; t.kind = kind;
            const int need = geneNeeds(LK, t);
            if (need == NEED_MOTION || need == NEED_SHAPE || need == NEED_MOVER)
                geneNeedSay(scope.flask, k, p, fmt("GENE %d (%s): NO EFFECT IN THE DROP", p + 1, GENE_KIND_NAMES[kind]), needWords(need));
        }
        bool differs = false;
        if (L.plan != P.plan || ratio(L.aspect, P.aspect) > 1.3) {
            differs = true;
            seen(GN_SHAPE, L.plan != P.plan ? fmt("CELLS: %s, PARENT: %s", planName(L.plan), planName(P.plan))
                                            : fmt("CELLS %s THAN PARENT'S", L.aspect > P.aspect ? "LONGER" : "SHORTER"));
        }
        if (ratio(L.widthUm, P.widthUm) > 1.2) {   // its cells' width (a new shape keeps it)
            differs = true;
            seen(GN_SIZE, fmt("CELLS %s PARENT'S", L.widthUm > P.widthUm ? (L.widthUm > 1.8 * P.widthUm ? "TWICE AS WIDE AS" : "WIDER THAN")
                                                                              : (L.widthUm < P.widthUm / 1.8 ? "HALF AS WIDE AS" : "NARROWER THAN")));
        }
        if (L.pigment != P.pigment) {
            differs = true;
            if (scope.seeing == WS_BRIGHT)
                seen(GN_PIGMENT, std::string(L.pigment ? PIGMENT_NAMES[L.pigment] : "PALE") + ", PARENT " + (P.pigment ? PIGMENT_NAMES[P.pigment] : "PALE"));
        }
        if (L.motion != P.motion || ratio(L.speed, P.speed) > 1.4) {
            differs = true;
            if (active)
                seen(GN_MOTION, L.motion == MO_NONE ? std::string("CELLS STILL, PARENT'S MOVE")
                               : (P.motion == MO_NONE ? std::string("CELLS MOVE, PARENT'S STILL")
                               : (L.motion != P.motion ? std::string("MOVES ANOTHER WAY: ") + MOTION_NAMES[L.motion]
                               : fmt("MOVES %s THAN PARENT", L.speed > P.speed ? "FASTER" : "SLOWER"))));
        }
        if (L.taxis != P.taxis || LK.s.flees != PK.s.flees) differs = true;   // a sense: its answers show it
        if (differs || (F.geneTold[k] & (1u << GN_QUIET)) || !(cultureStagesBy(F.run, k, lab.clock) & LC_GROWING)) continue;
        F.geneTold[k] |= 1u << GN_QUIET;   // grown and looked at, nothing different: NOTHING SEEN
        std::vector<int> fresh;
        for (size_t j = 0; j < F.changed[k].size(); j++) {
            const int p = F.changed[k][j];
            const GeneKnown* g = geneKnown(F.genesis[k], p);
            if (g || switchedOn(F, k, p, st)) continue;   // (Y-04: a switch alone, on: its work its parent's there)
            MicrobeGene t; t.kind = j < F.changedKind[k].size() ? F.changedKind[k][j] : GN_QUIET;
            const int need = geneNeeds(LK, t);
            if (need != NEED_NONE && need != NEED_LIGHT) continue;   // Y-03: one that cannot work in it shows nothing: nothing seen of it said
            GeneKnown n; n.how = GENE_SEEN_NOTHING;
            guide.genes[geneKey(F.genesis[k], p)] = n;
            fresh.push_back(p);
        }
        if (fresh.empty()) continue;
        const std::string nothing = fmt("FLASK %d: NOTHING DIFFERENT IN THE DROP", scope.flask + 1);
        const std::string says = fmt("GENE%s %s: NOTHING SEEN", fresh.size() == 1 ? "" : "S", placesWords(fresh, "AND").c_str());
        lab.edit.said.push_back({nothing, fitText(says, UW - 12)});
        logEvent("GENE", nothing + ": " + says);
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
    const int st = scope.lineState[k];   // Y-04: its parent as the drop's state has it
    CultureKind PK; Morph pf;
    flaskLineIn(scope.flask, k, st, PK, pf, true);
    const Microbe& P = PK.s;
    const bool pLight = (P.taxis & TX_LIGHT) != 0, pFlees = fleesLight(P), pFood = (P.taxis & TX_FOOD) != 0, pField = (P.taxis & TX_FIELD) != 0;
    auto seen = [&](const char* what) { geneSeen(scope.flask, k, GN_SENSE, what, GENE_SEEN_EDIT, st); };
    if ((bits & SA_LIGHT) && !pLight) seen("GATHERS IN LIGHT, PARENT DOES NOT");
    else if ((bits & SA_FLEES) && !pFlees) seen("FLEES LIGHT, PARENT DOES NOT");
    else if ((bits & SA_FOOD) && !pFood) seen("SWIMS TO FOOD, PARENT DOES NOT");
    else if ((bits & SA_FIELD) && !pField) seen("FOLLOWS THE MAGNET, PARENT DOES NOT");
    else if (unanswered == SD_LIGHT && (pLight || pFlees)) seen("IGNORES LIGHT, PARENT DOES NOT");
    else if (unanswered == SD_FOOD && pFood) seen("IGNORES FOOD, PARENT DOES NOT");
    else if (unanswered == SD_MAGNET && pField) seen("IGNORES THE MAGNET, PARENT DOES NOT");
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
    const std::string line = fmt("STRAIN READ: GENE %d IS THE %s GENE", p + 1, GENE_KIND_NAMES[kind]);
    logEvent("GENE", fmt("STRAIN READ: ONLY GENE %d DIFFERS FROM THE PARENT - THE %s GENE", p + 1, GENE_KIND_NAMES[kind]));
    lab.edit.made = false;
    return line;
}

// ---- the editor: its chooser, what is on it, the edits, the writing ---------------------------------------------------------------------

// the chooser's rows: each flask's species and line (as it is now), then the collection's (a pair's two each), by folder; those whose species
// the sequencer has not read marked (refused); Y-03: a kin's (K, M) the species read before them, its own world's first, a folder a world, and
// those of another life marked (refused with its words)
void Game::editorChoices(bool kin) {
    EditorState& E = lab.edit;
    E.choices.clear(); E.choiceSel = 0;
    E.pages = ChooserPage();
    std::vector<std::string> keys, looks, rests, traits;
    std::set<std::string> listed;
    auto read = [&](const FlaskSpecies& w) { const auto it = guide.microbes.find(flaskSpeciesKey(w)); return it != guide.microbes.end() && (it->second.seen & SEEN_KIN); };
    auto mark = [&](const FlaskSpecies& w, bool isRead) {
        if (!isRead) return std::string(" (NOT SEQUENCED)");
        if (!kin) return std::string();
        if (w.sameSpecies(E.who)) return std::string(" (SAME SPECIES)");
        return lineageOfKey(flaskSpeciesKey(w)).genesis.id != E.genesis.id ? std::string(" (ANOTHER LIFE)") : std::string();
    };
    if (kin && E.on) {   // Y-03: the species read, a folder a world, its own world's first
        const std::string mine = flaskSpeciesKey(E.who), home = mine.substr(0, mine.rfind("/M"));
        for (int pass = 0; pass < 2; pass++)
            for (const auto& kv : guide.microbes) {
                const std::string world = kv.first.substr(0, kv.first.rfind("/M"));
                FlaskSpecies w;
                if (!(kv.second.seen & SEEN_KIN) || (world == home) != (pass == 0) || !speciesOfKey(kv.first, w)) continue;
                const Morph* form = formOfKey(kv.first);
                if (!form) continue;
                EditorChoice c; c.who = w; c.from = upper(worldNameOfKey(world)); c.read = true;
                E.choices.push_back(c); keys.push_back(world); looks.push_back(flaskSpeciesWords(w, *form, true)); rests.push_back(mark(w, true));
                traits.push_back(formTraits(*form));
            }
    }
    flaskMadeAll();
    for (int f = 0; f < INCUBATOR_FLASKS; f++)
        for (int k = 0; k < lab.flasks[f].n && k < (int)lab.flasks[f].forms.size(); k++) {
            const Flask& F = lab.flasks[f];
            FlaskSpecies w = F.who[k]; w.strainIn = w.strain;
            if (!listed.insert("FLASKS|" + flaskStrainKey(w)).second) continue;
            EditorChoice c; c.who = w; c.medium = F.medium; c.asWorld = F.asWorld; c.from = fmt("FLASK %d", f + 1); c.read = read(w);
            E.choices.push_back(c); keys.push_back("FLASKS"); traits.push_back("");
            looks.push_back(fmt("FLASK %d: ", f + 1) + flaskSpeciesWords(w, F.forms[k], false)); rests.push_back(mark(w, c.read));
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
                E.choices.push_back(c); keys.push_back(keptFolderKey(k)); looks.push_back(look); rests.push_back(mark(w, c.read)); traits.push_back(trait);
            }
    apartRows(looks, rests, keys, traits);
    for (size_t i = 0; i < E.choices.size(); i++) E.choices[i].line = looks[i] + rests[i];
    E.pages.folders = chooserFolders(keys);
}

bool Game::editorPut(const EditorChoice& c) {
    if (!c.read) { refuse("SEQUENCE IT FIRST", 3); return false; }
    EditorState& E = lab.edit;
    E.on = true; E.who = c.who; E.who.strainIn = E.who.strain; E.medium = c.medium; E.asWorld = c.asWorld; E.from = c.from;
    E.edits.clear(); E.writ.clear(); E.letters = false; E.cursor = 0; E.writing = false;
    editorMake();
    if (!E.on || !E.genes.valid) { E = EditorState(); refuse("NO GENES TO EDIT", 3); return false; }
    if (E.kinOn && E.kin.sameSpecies(E.who) && E.kin.strain == E.who.strain) E.kinOn = false;   // Y-03: its kin stays beside it, of its genesis
    E.sel = E.genes.genes.empty() ? 0 : E.genes.genes[0].place;
    status(fitText("EDITING " + E.name + ", FROM " + E.from, UW - 12), 4);
    audio.beep = BEEP_UI;
    return true;
}

// Y-03: a kin put beside it (K, or M with none): a culture or a species read, of its genesis; another refused with its words (its alphabet, its
// hand, another life's map); its genes, kind, form and words made, its genome's share alike
bool Game::editorKinPut(const EditorChoice& c) {
    EditorState& E = lab.edit;
    if (!E.on) return false;
    if (!c.read) { refuse("SEQUENCE IT FIRST", 3); return false; }
    FlaskSpecies w = c.who; w.strainIn = w.strain;
    if (w.sameSpecies(E.who) && w.strain == E.who.strain) { refuse("THAT IS THE CULTURE BEING EDITED", 3); return false; }
    CultureKind k; Morph f; MicrobeGenes g; double slept = 0;
    if (!flaskKind(w, k, f, slept, &g) || !g.valid) { refuse("NO GENES TO READ", 3); return false; }
    const Genesis& a = E.genesis;
    const Genesis& b = g.genesis;
    if (b.id != a.id) {
        refuse(b.letters != a.letters ? "DIFFERENT GENE ALPHABET - GENES WON'T FIT"
                                      : (b.mirrored != a.mirrored ? "MIRRORED CHEMISTRY - GENES WON'T FIT" : "ANOTHER ORIGIN OF LIFE - GENES WON'T FIT"), 4);
        return false;
    }
    E.kinOn = true; E.kin = w; E.kinFrom = c.from; E.kinKind = k; E.kinForm = f; E.kinGenes = g;
    E.kinName = flaskSpeciesWords(w, f, false);
    E.kinAlike = markerAlike(lineageOfKey(flaskSpeciesKey(E.who)), lineageOfKey(flaskSpeciesKey(w)));
    status(fitText("COMPARING WITH " + E.kinName + ", FROM " + E.kinFrom, UW - 12), 4);
    audio.beep = BEEP_UI;
    return true;
}

// Y-03: M, its kin's gene at the place chosen moved in (its letters and its value the kin's) over its own, into a gap or a gene cut out; with no
// kin beside it, the chooser first
void Game::editorMove() {
    EditorState& E = lab.edit;
    if (!E.on) return;
    if (E.writing) { refuse("BUSY WRITING THE NEW STRAIN", 2); return; }
    if (!E.kinOn) {
        editorChoices(true);
        if (E.choices.empty()) { refuse("NO KIN SEQUENCED YET", 3); return; }
        E.choosing = true; E.choosingFor = EC_MOVE; audio.beep = BEEP_UI;
        return;
    }
    const MicrobeGene* K = E.kinGenes.at(E.sel);
    if (!K || K->cut) { refuse(fmt("KIN HAS NO GENE %d", E.sel + 1), 3); return; }
    const MicrobeGene* H = E.editedGenes.at(E.sel);
    if (H && !H->cut && H->part == K->part) { refuse(fmt("GENE %d IS THE SAME IN THE KIN", E.sel + 1), 3); return; }
    GeneEdit e; e.place = E.sel; e.how = GX_MOVED; e.part = K->part; e.donor = flaskStrainKey(E.kin);
    geneDonor(E.who, e);
    auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& x) { return x.place == e.place; });
    if (it != E.edits.end()) *it = e; else E.edits.push_back(e);
    std::sort(E.edits.begin(), E.edits.end(), [](const GeneEdit& a, const GeneEdit& b) { return a.place < b.place; });
    E.letters = false;
    editorEdited();
    status(fitText(fmt("GENE %d MOVED IN FROM ", E.sel + 1) + farWords(e.far), UW - 12), 3);
    audio.beep = BEEP_UI;
}

// Y-03: D, the gene chosen copied to the first free place of its kind (a gap, or a gene cut out), chosen then to be changed by hand
void Game::editorCopy() {
    EditorState& E = lab.edit;
    if (!E.on) return;
    if (E.writing) { refuse("BUSY WRITING THE NEW STRAIN", 2); return; }
    const MicrobeGene* H = E.editedGenes.at(E.sel);
    if (!H) { refuse(fmt("NO GENE %d TO COPY", E.sel + 1), 3); return; }
    if (H->cut) { refuse("CUT OUT - NOTHING TO COPY", 2); return; }
    int to = -1;
    for (int p = 0; p < GENE_PLACES && to < 0; p++) {
        if (p == E.sel || E.code.kind[p] != H->kind) continue;
        const MicrobeGene* F = E.editedGenes.at(p);
        if (!F || F->cut) to = p;
    }
    if (to < 0) { refuse(fmt("NO FREE PLACE FOR A COPY OF GENE %d", E.sel + 1), 3); return; }
    GeneEdit e; e.place = to; e.how = GX_COPY; e.from = E.sel; e.part = H->part;
    auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& x) { return x.place == to; });
    if (it != E.edits.end()) *it = e; else E.edits.push_back(e);
    std::sort(E.edits.begin(), E.edits.end(), [](const GeneEdit& a, const GeneEdit& b) { return a.place < b.place; });
    editorEdited();
    status(fmt("GENE %d COPIED TO %d - ENTER TO EDIT LETTERS", E.sel + 1, to + 1), 3);
    E.sel = to;
    audio.beep = BEEP_UI;
}

// what is on it made again (not saved): its genes (its lines' from its species'), its genesis's code, its kind and form, the edited ones, its words;
// Y-03: its moved genes' donors read (after a load), its kin beside it
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
    for (std::vector<GeneEdit>* v : {&E.edits, &E.writ})
        for (GeneEdit& e : *v) if (e.how == GX_MOVED && !e.read) geneDonor(E.who, e);
    if (E.kinOn) {
        FlaskSpecies k = E.kin; k.strainIn = k.strain;
        double ks = 0;
        if (!flaskKind(k, E.kinKind, E.kinForm, ks, &E.kinGenes) || !E.kinGenes.valid || E.kinGenes.genesis.id != E.genesis.id) E.kinOn = false;
        else {
            E.kinName = flaskSpeciesWords(k, E.kinForm, false);
            E.kinAlike = markerAlike(lineageOfKey(flaskSpeciesKey(E.who)), lineageOfKey(flaskSpeciesKey(k)));
        }
    }
    editorEdited();
}
void Game::editorEdited() {
    EditorState& E = lab.edit;
    E.editedKind = E.kind; E.editedForm = E.form; E.editedGenes = E.genes;
    applyEdits(E.editedKind, E.editedForm, E.editedGenes, E.writing ? E.writ : E.edits);
}

// a gene's edit made: its letters or its cut, dropped where it is its parent's again; the edited line made again. Y-03: a gene moved in or
// copied keeps being one as its letters change; cut where none was before, it is taken out again
void Game::editorSet(int place, bool cut, const std::string& part) {
    EditorState& E = lab.edit;
    const MicrobeGene* G = E.genes.at(place);
    auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == place; });
    const bool put = it != E.edits.end() && !it->cut && it->how != GX_HAND;
    if (put && !cut) { it->part = part; editorEdited(); return; }
    if (!G || G->cut) { if (put) { E.edits.erase(it); editorEdited(); } return; }
    const bool parents = !cut && part == G->part;
    const int sw = it != E.edits.end() && !cut ? it->sw : -1;   // Y-04: its switch kept as its letters change (a cut gene has none)
    if (parents && sw < 0) { if (it != E.edits.end()) E.edits.erase(it); }
    else {
        GeneEdit e; e.place = place; e.cut = cut; e.part = cut ? G->part : (parents ? std::string() : part); e.sw = sw;
        if (it != E.edits.end()) *it = e; else E.edits.push_back(e);
        std::sort(E.edits.begin(), E.edits.end(), [](const GeneEdit& a, const GeneEdit& b) { return a.place < b.place; });
    }
    editorEdited();
}

// Y-04: S, the gene chosen's switch the next (ALWAYS, NEVER, IN THE LIGHT, IN THE DARK, WHEN CROWDED, WHEN STARVING, WHEN COLD), its letters left;
// its own again (its parent's; a gene moved in or copied, as it came) the switch dropped
void Game::editorSwitch() {
    EditorState& E = lab.edit;
    if (!E.on) return;
    if (E.writing) { refuse("BUSY WRITING THE NEW STRAIN", 2); return; }
    const MicrobeGene* H = E.editedGenes.at(E.sel);
    if (!H) { refuse(fmt("NO GENE %d TO SWITCH", E.sel + 1), 3); return; }
    if (H->cut) { refuse("CUT OUT - NOTHING TO SWITCH", 2); return; }
    const int next = (H->sw + 1) % GW_COUNT;
    auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == E.sel; });
    int was = H->sw;   // its switch with none set: its parent's, or as it came
    {
        std::vector<GeneEdit> bare = E.edits;
        for (GeneEdit& e : bare) if (e.place == E.sel) e.sw = -1;
        CultureKind k = E.kind; Morph f = E.form; MicrobeGenes g = E.genes;
        applyEdits(k, f, g, bare);
        if (const MicrobeGene* B = g.at(E.sel)) was = B->sw;
    }
    if (it == E.edits.end()) {
        GeneEdit e; e.place = E.sel; e.sw = next;
        E.edits.push_back(e);
        std::sort(E.edits.begin(), E.edits.end(), [](const GeneEdit& a, const GeneEdit& b) { return a.place < b.place; });
    } else {
        it->sw = next == was ? -1 : next;
        if (it->sw < 0 && it->how == GX_HAND && !it->cut && it->part.empty()) E.edits.erase(it);   // nothing else changed there
    }
    editorEdited();
    status(fmt("GENE %d SWITCHED: %s", E.sel + 1, GENE_SWITCH_NAMES[next]), 3);
    audio.beep = BEEP_UI;
}

// R-445: W asks which folder the new strain goes in (the parent's folder chosen first), then its name. The name entry starts with the
// parent's name and the new strain's number ("VURYM E2"; an unnamed species' look, "A ROD E1"), so the default never repeats the parent's
// name. The writing begins once both are given; Esc at either writes nothing
bool Game::editorWrite() {
    EditorState& E = lab.edit;
    if (!E.on) return false;
    if (E.writing) { refuse("BUSY WRITING THE NEW STRAIN", 2); return false; }
    if (E.edits.empty()) { refuse("NOTHING CHANGED - EDIT A GENE FIRST", 3); return false; }
    bool empty = false;
    for (const Flask& F : lab.flasks) empty = empty || F.n == 0;
    if (!empty) { refuse("NO EMPTY FLASK - EMPTY ONE IN THE INCUBATOR", 3); return false; }
    KeptCulture parent;
    parent.who = E.who; parent.medium = E.medium; parent.asWorld = E.asWorld; parent.plan = E.form.plan;
    keepOpen(parent);
    keep.write = true;
    for (size_t f = 0; f < lab.folders.size() && keep.sel == 0; f++)
        for (const KeptCulture& o : lab.kept) if (o.folder == lab.folders[f].id && sameCulture(o, parent)) keep.sel = 1 + (int)f;
    return true;
}
void Game::editorWriteName(int folder) {
    EditorState& E = lab.edit;
    E.writeFolder = folder;
    const std::string base = flaskSpeciesKey(E.who), strainName = E.who.strain > 0 ? nameOfKey(flaskStrainKey(E.who)) : "UNKNOWN";
    const std::string speciesName = nameOfKey(base);
    std::string name = strainName != "UNKNOWN" ? upper(strainName) : (speciesName != "UNKNOWN" ? upper(speciesName) : std::string(planWords(E.form.plan)));
    int n = 1;
    while (guide.strains.count(base + "/S" + std::to_string(n))) n++;
    const std::string tag = " E" + std::to_string(n);
    if (name.size() + tag.size() > 28) name.resize(28 - tag.size());   // the entry holds 28 letters
    beginTextEntry("NAME THE NEW STRAIN", TEXT_LINE, name + tag);
}
bool Game::editorWriteBegin(const std::string& name) {
    EditorState& E = lab.edit;
    if (!E.on || E.writing || E.edits.empty()) return false;
    E.writing = true; E.t0 = lab.clock; E.writ = E.edits; E.letters = false; E.writeName = name;
    editorEdited();
    status(name.empty() ? std::string("WRITING THE STRAIN - TWO MINUTES") : "WRITING " + name + " - TWO MINUTES", 4);
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
    if (!E.writeName.empty()) guide.names[base + "/S" + std::to_string(n)] = E.writeName;   // R-445
    FlaskSpecies w = E.who; w.strain = w.strainIn = n;
    CultureKind kind; Morph form; double slept = 0;
    flaskKind(w, kind, form, slept);
    const std::string words = flaskSpeciesWords(w, form, false);
    // R-445: kept in its folder at once (a flask's own keeping finds it there and keeps no second copy)
    KeptCulture c; c.who = w; c.medium = E.medium; c.asWorld = E.asWorld; c.t = lab.clock; c.plan = form.plan; c.folder = E.writeFolder;
    lab.kept.push_back(c);
    int f = -1;
    for (int i = 0; i < INCUBATOR_FLASKS && f < 0; i++) if (lab.flasks[i].n == 0) f = i;
    std::string line;
    if (f >= 0) {
        Flask& F = lab.flasks[f];
        F = Flask(); F.n = 1; F.who[0] = w; F.medium = E.medium; F.asWorld = E.asWorld;
        CultureEvent e; e.kind = CD_START; e.t = lab.clock; e.m = F.medium;
        F.events.push_back(e);
        flaskMake(f);
        line = fmt("%s WRITTEN INTO FLASK %d, KEPT IN %s", words.c_str(), f + 1, folderName(E.writeFolder).c_str());
    } else line = fmt("NO EMPTY FLASK: %s KEPT IN %s", words.c_str(), folderName(E.writeFolder).c_str());
    E.writ.clear(); E.writeName.clear(); E.writeFolder = 0;
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

// its working part open: Left and Right a letter, Up and Down it through the alphabet, Backspace its parent's letter there (Y-03: a gene moved in
// or copied, its letter as it came); Enter or Esc the map
void Game::editorLetterKeys(const Input& in) {
    EditorState& E = lab.edit;
    const MicrobeGene* G = E.genes.at(E.sel);
    const MicrobeGene* H = E.editedGenes.at(E.sel);
    if (!H || H->cut || E.writing) { E.letters = false; return; }
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
        const auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == E.sel; });
        std::string base = G && !G->cut ? G->part : part;
        if (it != E.edits.end() && it->how == GX_MOVED && it->read) base = it->donorGene.part;
        else if (it != E.edits.end() && it->how == GX_COPY) { const MicrobeGene* S = E.editedGenes.at(it->from); if (S && !S->cut) base = S->part; }
        if ((int)base.size() == n) part[E.cursor] = base[E.cursor];
        editorSet(E.sel, false, part);
        audio.beep = BEEP_UI;
    }
}

void Game::updateEditor(const Input& in, double dt, double realDt) {
    EditorState& E = lab.edit;
    if (keepKeys(in)) return;   // R-445: the folder of the line to write
    if (E.choosing) {
        const int r = E.choices.empty() ? -2 : chooserKeys(in, E.pages, E.choiceSel);
        if (r == -2) { E.choosing = false; return; }
        if (r >= 0 && r < (int)E.choices.size()) {
            E.choosing = false;
            const EditorChoice c = E.choices[r];
            if (E.choosingFor == EC_CULTURE) editorPut(c);
            else if (editorKinPut(c) && E.choosingFor == EC_MOVE) editorMove();   // Y-03: a kin beside it, its gene moved in where M asked
        }
        return;
    }
    if (E.on && !E.made) editorMake();
    if (E.letters) { editorLetterKeys(in); return; }
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    auto choose = [&](bool kin) {
        if (E.writing) { refuse("BUSY WRITING THE NEW STRAIN", 2); return; }
        editorChoices(kin);
        if (E.choices.empty()) { refuse("NOTHING TO EDIT - GROW OR KEEP A CULTURE FIRST", 3); return; }
        E.choosing = true; E.choosingFor = kin ? EC_KIN : EC_CULTURE; audio.beep = BEEP_UI;
    };
    if (!E.on) { if (enterKey(in)) choose(false); return; }
    if (in.wasPressed(KEY_TAB)) { choose(false); return; }
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
    if (in.wasPressed(KEY_K) && !in.ctrl()) { choose(true); return; }   // Y-03: a kin beside it
    if (in.wasPressed(KEY_M) && !in.ctrl()) { editorMove(); return; }
    if (in.wasPressed(KEY_D) && !in.ctrl()) { editorCopy(); return; }
    if (in.wasPressed(KEY_S) && !in.ctrl()) { editorSwitch(); return; }   // Y-04
    const bool act = enterKey(in) || (in.wasPressed(KEY_C) && !in.ctrl()) || in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE);
    if (in.wasPressed(KEY_W) && !in.ctrl()) { editorWrite(); return; }
    if (!act) return;
    if (E.writing) { refuse(fmt("WRITING THE STRAIN - %.0f SECONDS LEFT", std::ceil(std::max(0.0, E.t0 + EDIT_S - lab.clock))), 2); return; }
    const MicrobeGene* G = E.genes.at(E.sel);
    const auto it = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == E.sel; });
    const bool put = it != E.edits.end() && !it->cut && it->how != GX_HAND;   // Y-03: a gene moved in or copied there
    if (!put && !G) { refuse(fmt(E.kinOn ? "NO GENE %d HERE - M MOVES THE KIN'S IN" : "NO GENE %d HERE - A KIN MAY HAVE ONE", E.sel + 1), 3); return; }
    if (!put && G->cut) { refuse("CUT OUT IN THE PARENT - M MOVES ONE IN", 3); return; }
    const bool cutNow = it != E.edits.end() && it->cut;
    if (enterKey(in)) {
        if (cutNow) { refuse("CUT OUT - C PUTS IT BACK", 2); return; }
        E.letters = true; E.cursor = 0; audio.beep = BEEP_UI;
        return;
    }
    if (in.wasPressed(KEY_C)) {
        if (put && (!G || G->cut)) { E.edits.erase(it); editorEdited(); status(fmt("GENE %d TAKEN OUT AGAIN", E.sel + 1), 2); audio.beep = BEEP_UI; return; }
        editorSet(E.sel, !cutNow, G->part);
        status(fmt(cutNow ? "GENE %d BACK IN" : "GENE %d CUT OUT", E.sel + 1), 2);
        audio.beep = BEEP_UI;
        return;
    }
    if (it != E.edits.end()) { E.edits.erase(it); editorEdited(); status(fmt("GENE %d RESET TO THE PARENT'S", E.sel + 1), 2); audio.beep = BEEP_UI; }
}

void Game::renderEditor() {
    EditorState& E = lab.edit;
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 218);
    drawText(canvas, 6, 4, "GENE EDITOR", editText());
    const auto selEdit = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == E.sel; });
    const bool put = selEdit != E.edits.end() && !selEdit->cut && selEdit->how != GX_HAND;   // Y-03: the gene chosen moved in or copied
    if (!E.on) {
        drawTextCentered(canvas, UW / 2, UH / 2 - 12, "EMPTY", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH / 2, "ENTER: CHOOSE A SEQUENCED CULTURE", HUD_DIM);
    } else {
        const char* tab = "TAB: ANOTHER CULTURE";
        drawText(canvas, UW - 6 - textWidth(tab), 4, tab, HUD_DIM);
        drawText(canvas, 6, 14, fitHead(E.name, ", FROM " + E.from, UW - 12).c_str(), HUD_WHITE);
        if (E.kinOn)   // Y-03: its kin beside it, their genomes' share alike
            drawText(canvas, 6, 23, fitHead("KIN: " + E.kinName, fmt(", %.0f%% ALIKE", 100 * E.kinAlike), UW - 12).c_str(), HUD_CYAN);
        // the map: two columns of nine places, each its number, its words, its mark (= changed, - cut out, + moved in, * a copy; a gap's dash);
        // Y-03: with a kin beside it, how many of their working parts' letters differ there, + where its kin has one and it has none; Y-04: ~ a
        // switch set, after the mark (B-445 drew the tilde)
        const int my = 33, rh = 8, cw = (UW - 12) / 2;
        for (int p = 0; p < GENE_PLACES; p++) {
            const int x = 6 + (p / MAP_ROWS) * cw, y = my + (p % MAP_ROWS) * rh;
            const MicrobeGene* H = E.editedGenes.at(p);
            const auto ed = std::find_if(E.edits.begin(), E.edits.end(), [&](const GeneEdit& e) { return e.place == p; });
            if (p == E.sel) fillRectRGB(canvas, x - 2, y - 1, x + cw - 6, y + 6, editColour(0.35f));
            drawText(canvas, x, y, fmt("%2d", p + 1).c_str(), H ? editText() : HUD_DIM);
            const std::string w = !H ? std::string("-") : (H->cut ? std::string("CUT OUT") : geneWords(E.genesis.id, p));
            const uint32_t col = !H ? HUD_DIM : (H->cut ? HUD_RED : (w == "NOT KNOWN" ? HUD_DIM : HUD_GREEN));
            drawText(canvas, x + 16, y, fitText(w, cw - (E.kinOn ? 50 : 34)).c_str(), col);
            if (E.kinOn) {
                const MicrobeGene* K = E.kinGenes.at(p);
                std::string d;
                uint32_t dc = HUD_CYAN;
                if (K && !K->cut && H && !H->cut && K->part.size() == H->part.size()) {
                    int n = 0;
                    for (size_t i = 0; i < K->part.size(); i++) n += K->part[i] != H->part[i];
                    d = fmt("%d", n);
                    if (n == 0) dc = HUD_DIM;
                } else if (K && !K->cut) d = "+";
                if (!d.empty()) drawText(canvas, x + cw - 22 - textWidth(d.c_str()), y, d.c_str(), dc);
            }
            if (ed != E.edits.end() && !ed->switchOnly()) drawText(canvas, x + cw - 16, y, ed->cut ? "-" : (ed->how == GX_MOVED ? "+" : (ed->how == GX_COPY ? "*" : "=")), HUD_AMBER);
            if (ed != E.edits.end() && ed->sw >= 0 && !ed->cut) drawText(canvas, x + cw - 10, y, "~", HUD_AMBER);
        }
        // the gene chosen: its number, its words and length; its working part (the edited letters bright); Y-03: under it where a moved gene came
        // from, a copy's gene, or its kin's letters (those that differ bright); its value where it is known, its parent's and its kin's
        int y = my + MAP_ROWS * rh + 4;
        const MicrobeGene* G = E.genes.at(E.sel);
        const MicrobeGene* H = E.editedGenes.at(E.sel);
        const MicrobeGene* K = E.kinOn ? E.kinGenes.at(E.sel) : nullptr;
        if (K && K->cut) K = nullptr;
        if (!H) drawText(canvas, 6, y, fmt(K ? "GENE %d: NONE - M MOVES THE KIN'S IN" : (E.kinOn ? "GENE %d: NONE HERE OR IN THE KIN" : "GENE %d: NONE - A KIN MAY HAVE ONE"),
                                           E.sel + 1).c_str(), HUD_DIM);
        else {
            const std::string words = geneWords(E.genesis.id, E.sel);
            drawText(canvas, 6, y, fitText(fmt("GENE %d, %s LETTERS: ", E.sel + 1, thousands(H->length).c_str()) + (words == "NOT KNOWN" ? "FUNCTION NOT KNOWN" : words), UW - 12).c_str(), HUD_WHITE);
            y += 10;
            drawText(canvas, 6, y, "WORKING PART:", HUD_DIM);
            const int lx = 6 + textWidth("WORKING PART: ");
            const std::string& part = H->part;
            const bool own = G && !G->cut && G->part.size() == part.size();
            for (int i = 0; i < (int)part.size(); i++) {
                const char c1[2] = {part[i], 0};
                const bool changed = own && part[i] != G->part[i];
                if (E.letters && i == E.cursor) drawRectRGB(canvas, lx + i * 11 - 2, y - 2, lx + i * 11 + 6, y + 8, HUD_WHITE);
                drawText(canvas, lx + i * 11, y, c1, H->cut ? HUD_DIM : letterDye(part[i], changed || !own ? 1.0f : 0.6f));
                if (changed) drawLineRGB(canvas, lx + i * 11, y + 8, lx + i * 11 + 5, y + 8, letterDye(part[i]));
            }
            y += 10;
            if (put && selEdit->how == GX_MOVED) {
                drawText(canvas, 6, y, fitText(std::string("MOVED IN FROM ") + farWords(selEdit->far) + (selEdit->read ? "" : " - NOT THERE NOW"), UW - 12).c_str(), HUD_AMBER);
                y += 10;
            } else if (put) {
                drawText(canvas, 6, y, fmt("COPY OF GENE %d", selEdit->from + 1).c_str(), HUD_AMBER);
                y += 10;
            } else if (E.kinOn) {
                if (!K) drawText(canvas, 6, y, "KIN HAS NONE", HUD_DIM);
                else {
                    drawText(canvas, 6, y, "KIN'S:", HUD_CYAN);
                    for (int i = 0; i < (int)K->part.size(); i++) {
                        const char c1[2] = {K->part[i], 0};
                        drawText(canvas, lx + i * 11, y, c1, letterDye(K->part[i], i < (int)part.size() && K->part[i] != part[i] ? 1.0f : 0.4f));
                    }
                }
                y += 10;
            }
            if (!H->cut) {   // Y-04: its switch (amber where set, what it was beside it)
                const bool set = selEdit != E.edits.end() && selEdit->sw >= 0;
                std::string sl = std::string("SWITCH: ") + GENE_SWITCH_NAMES[clampi(H->sw, 0, GW_COUNT - 1)];
                if (set && G && !G->cut && G->sw != H->sw) sl += std::string(" (WAS ") + GENE_SWITCH_NAMES[clampi(G->sw, 0, GW_COUNT - 1)] + ")";
                drawText(canvas, 6, y, fitText(sl, UW - 12).c_str(), set ? HUD_AMBER : HUD_DIM);
                y += 9;
            }
            y += 1;
            const GeneKnown* k = geneKnown(E.genesis.id, E.sel);
            if (k && k->kind >= 0 && !H->cut) {   // its value as its letters give it, beside its parent's and its kin's where they differ
                auto bare = [](std::string w) { return w.compare(0, 3, "IT ") == 0 ? w.substr(3) : w; };
                const std::string now = bare(geneValueWords(E.editedKind, E.editedForm, *H, E.code));
                const std::string was = G && !G->cut ? bare(geneValueWords(E.kind, E.form, *G, E.code)) : std::string(G ? "CUT OUT" : "NONE");
                drawText(canvas, 6, y, fitText((now == was ? "" : "NOW: ") + now, UW - 12).c_str(), HUD_GREEN); y += 9;
                if (now != was) { drawText(canvas, 6, y, fitText("PARENT: " + was, UW - 12).c_str(), HUD_DIM); y += 9; }
                if (K) {
                    const std::string kv = bare(geneValueWords(E.kinKind, E.kinForm, *K, E.code));
                    if (kv != now) { drawText(canvas, 6, y, fitText("KIN: " + kv, UW - 12).c_str(), HUD_CYAN); y += 9; }
                }
            } else if (k && k->kind >= 0) { drawText(canvas, 6, y, "CUT OUT", HUD_RED); y += 9; }
            else { drawText(canvas, 6, y, k ? "NOTHING SEEN WHEN CHANGED" : "UNKNOWN - CHANGE IT AND GROW THE STRAIN TO SEE", HUD_DIM); y += 9; }
        }
        // the edits, the writing (Y-04: the edits' line left to the map's marks where the gene's lines fill the screen)
        y = std::max(y + 2, my + MAP_ROWS * rh + 49);
        if (E.writing) {
            const double share = clampd((lab.clock - E.t0) / EDIT_S, 0, 1);
            drawRectRGB(canvas, 6, y, UW - 7, y + 5, editColour(0.4f));
            fillRectRGB(canvas, 7, y + 1, 7 + (int)std::lround((UW - 15) * share), y + 4, editColour());
            drawText(canvas, 6, y + 8, fmt("WRITING THE STRAIN: %.0f SECONDS LEFT", std::ceil(std::max(0.0, E.t0 + EDIT_S - lab.clock))).c_str(), HUD_DIM);
        } else if (!E.edits.empty() && y <= UH - 31) {
            std::vector<int> changed, cut, moved, copied, switched;
            for (const GeneEdit& e : E.edits) {
                if (e.sw >= 0 && !e.cut) switched.push_back(e.place);
                if (!e.switchOnly()) (e.cut ? cut : (e.how == GX_MOVED ? moved : (e.how == GX_COPY ? copied : changed))).push_back(e.place);
            }
            std::string s = "EDITS:";
            if (!changed.empty()) s += " = " + placesWords(changed, "AND");
            if (!cut.empty()) s += " - " + placesWords(cut, "AND");
            if (!moved.empty()) s += " + " + placesWords(moved, "AND");
            if (!copied.empty()) s += " * " + placesWords(copied, "AND");
            if (!switched.empty()) s += " ~ " + placesWords(switched, "AND");
            drawText(canvas, 6, y, fitText(s, UW - 12).c_str(), HUD_AMBER);
        }
    }
    if (E.letters) {
        drawTextCentered(canvas, UW / 2, UH - 21, "<- -> LETTER  UP DOWN CHANGE", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 12, put ? "BKSP AS IT CAME  ENTER MAP" : "BKSP PARENT'S LETTER  ENTER MAP", HUD_DIM);
    } else if (E.writing) {
        drawTextCentered(canvas, UW / 2, UH - 12, "N NAME GENE  WRITING GOES ON IF YOU LEAVE", HUD_DIM);
    } else if (E.on) {
        drawTextCentered(canvas, UW / 2, UH - 21, "ENTER LETTERS  S SWITCH  C CUT  D COPY  BKSP UNDO", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 12, "N NAME  K KIN  M MOVE KIN'S GENE IN  W WRITE", HUD_DIM);
    }
    if (E.choosing) {   // the chooser over it: the cultures' folders, then the open one's rows
        const int rowsN = 14, x0 = 20, y0 = 30;
        drawPanel(canvas, x0 - 6, y0 - 14, UW - x0 + 6, y0 + rowsN * 9 + 18, rgb(8, 8, 14), 235, editColour(0.6f));
        const std::string what = E.choosingFor == EC_MOVE ? fmt("GENE %d FROM WHICH KIN", E.sel + 1) : (E.choosingFor == EC_KIN ? std::string("KIN TO COMPARE") : std::string("WHAT TO EDIT"));
        const std::string head = E.pages.open && E.pages.folder < (int)E.pages.folders.size() ? what + ": " + E.pages.folders[E.pages.folder].name : what;
        drawText(canvas, x0, y0 - 10, fitText(head, UW - 2 * x0).c_str(), editText());
        std::vector<std::string> lines;
        for (const EditorChoice& c : E.choices) lines.push_back(c.line);
        drawChooser(canvas, x0 - 6, y0, UW - x0, y0 + rowsN * 9 + 14, E.pages, E.choiceSel, E.pages.top, lines, editColour());
    }
    renderKeepPick(editColour());   // R-445
}

void Game::editorPad(PadMenu& m) const {
    const EditorState& E = lab.edit;
    m.title = "GENE EDITOR";
    if (keep.open) { m.item("CHOOSE", KEY_ENTER); m.item("NEW FOLDER", KEY_N); m.item("CANCEL", KEY_ESCAPE); return; }   // R-445
    if (E.choosing) { m.item(E.pages.open ? (E.choosingFor == EC_CULTURE ? "EDIT IT" : "COMPARE") : "OPEN FOLDER", KEY_ENTER); m.item("BACK", KEY_ESCAPE); return; }
    if (E.letters) {
        m.item("NEXT LETTER", KEY_RIGHT); m.item("CHANGE IT", KEY_UP); m.item("PARENT'S LETTER", KEY_BACKSPACE); m.item("BACK TO MAP", KEY_ENTER);
        m.button(PR_A, KEY_ENTER, "MAP"); m.button(PR_B, KEY_ESCAPE, "MAP");
        return;
    }
    if (!E.on) { m.item("CHOOSE A CULTURE", KEY_ENTER); m.item("BACK", KEY_ESCAPE); m.button(PR_A, KEY_ENTER, "CHOOSE"); m.button(PR_B, KEY_ESCAPE, "BACK"); return; }
    m.item("LETTERS", KEY_ENTER); m.item("CUT OUT", KEY_C); m.item("RESET TO PARENT'S", KEY_BACKSPACE); m.item("NAME GENE", KEY_N);
    m.item("COMPARE WITH KIN", KEY_K); m.item("MOVE KIN'S GENE IN", KEY_M); m.item("COPY GENE", KEY_D);   // Y-03
    m.item("SWITCH", KEY_S);   // Y-04
    m.item("WRITE STRAIN", KEY_W); m.item("ANOTHER CULTURE", KEY_TAB); m.item("BACK", KEY_ESCAPE);
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
    if (E.writing) f << "editorwriting " << E.writeFolder << " " << E.writeName << "\n";   // R-445: its folder, its name
    f << "editorfrom " << E.from << "\n";
    if (E.kinOn) {   // Y-03: its kin beside it
        f << "editorkin " << E.kin.sx << " " << E.kin.sy << " " << E.kin.sz << " " << E.kin.body << " " << E.kin.species << " " << E.kin.strain << "\n";
        f << "editorkinfrom " << E.kinFrom << "\n";
    }
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
    if (key == "editorwriting") {   // R-445
        is >> E.writeFolder;
        std::getline(is, E.writeName);
        if (!E.writeName.empty() && E.writeName[0] == ' ') E.writeName.erase(0, 1);
        return true;
    }
    if (key == "editorfrom" || key == "editorkinfrom") {
        std::string& t = key == "editorfrom" ? E.from : E.kinFrom;
        std::getline(is, t);
        if (!t.empty() && t[0] == ' ') t.erase(0, 1);
        return true;
    }
    if (key == "editorkin") {   // Y-03
        FlaskSpecies k;
        is >> k.sx >> k.sy >> k.sz >> k.body >> k.species >> k.strain;
        if (is.fail()) return true;
        k.strainIn = k.strain; E.kin = k; E.kinOn = true; E.made = false;
        return true;
    }
    return false;
}

void Game::editorReset() { lab.edit = EditorState(); }
