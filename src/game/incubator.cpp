// Z-05: the incubator (R-425; docs/ideas/ITERATION-1.5.0.md Z-05, IDEAS-small-and-spacious.md 1.9; lab_state.h, life/cultures.h), the
// laboratory's second device: a screen over the room, E at its cabinet. Twelve flasks: Up and Down choose one, Left and Right the medium's
// field (its cell, its warmth, its light, its energy, its salt), + and - change it (Shift ten degrees), W sets it AS ITS WORLD again; Enter
// on an empty flask puts in a species seen in a vial or a flask's culture, on a culture a second; F a fresh broth (a hundredth into new
// medium), Backspace empties it, N names a strain. The lab's clock is the drones' kind (real seconds while the game simulates); a culture
// is closed-form from its events on it (`runCulture`); what happens in a flask is said and logged as the clock reaches it (WOKEN, STRAIN),
// and what its curve shows of its species, its stages and its growth, goes to the species' sheet
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

// a strain's move in words (its line said, the tree's twig)
std::string Game::strainMoveWords(int what, double value) {
    switch (what) {
        case SM_HOT: return fmt("GROWS UP TO %.0f C", value);
        case SM_COLD: return fmt("GROWS DOWN TO %.0f C", value);
        case SM_SIZE: return "CELLS A QUARTER BIGGER";
        case SM_SALT_LO: return std::string("GROWS IN ") + (value <= 0 ? "FRESH WATER" : "SEA SALT");
        case SM_SALT_HI: return std::string("GROWS IN ") + (value >= 2 ? "BRINE" : "SEA SALT");
        case SM_PIGMENT: return std::string("PIGMENT NOW ") + (value == PG_BLUE_GREEN ? "BLUE-GREEN" : (value == PG_ORANGE ? "ORANGE" : "BLACK"));
        default: return "A STRAIN";
    }
}

std::string Game::flaskSpeciesKey(const FlaskSpecies& w) const { return Guide::bodyKey(w.sx, w.sy, w.sz, w.body) + "/M" + std::to_string(w.species); }
std::string Game::flaskStrainKey(const FlaskSpecies& w) const { return w.strain > 0 ? flaskSpeciesKey(w) + "/S" + std::to_string(w.strain) : flaskSpeciesKey(w); }
// its name: a strain's own (else its species' and its number), a species' (else UNNAMED)
std::string Game::flaskSpeciesName(const FlaskSpecies& w) const {
    const std::string sp = nameOfKey(flaskSpeciesKey(w)), species = sp == "UNKNOWN" ? "UNNAMED" : upper(sp);
    if (w.strain <= 0) return species;
    const std::string st = nameOfKey(flaskStrainKey(w));
    return st == "UNKNOWN" ? species + " " + lineTag(w) : upper(st);
}
// B-442: its words in the lab's rows: its name (or its strain's), else what it looks like ("A ROD"; sized, "A ROD, 2.6 UM BY 0.5 UM") and an
// unnamed strain's number: a vial's unnamed species told apart
std::string Game::flaskSpeciesWords(const FlaskSpecies& w, const Morph& form, bool sized) const {
    const std::string n = flaskSpeciesName(w);
    if (n.compare(0, 7, "UNNAMED") != 0) return n;
    return (sized ? formWordsOf(form, CF_CELL) : std::string(planWords(form.plan))) + (w.strain > 0 ? " " + lineTag(w) : std::string());
}

// what a flask's species is: its world's (its values, its form), the strain it went in as (its moves from the wild one), and a vial's spores'
// sleep (its world's end, its drifter's leaving, the impact that carried them); Y-02: its lines' genes (the wild ones its species', a strain's
// its move's gene stepped, an edited line's its edits: `applyEdits`)
bool Game::flaskKind(const FlaskSpecies& w, CultureKind& kind, Morph& form, double& slept, MicrobeGenes* genes, int depth) const {
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
    std::vector<std::pair<int, StrainRecord>> chain;   // its lines' records (a strain's move, an edited line's edits), the wild one's first
    bool edits = false;
    for (int n = w.strainIn, guard = 0; n > 0 && guard < 64; guard++) {
        const auto it = guide.strains.find(flaskSpeciesKey(w) + "/S" + std::to_string(n));
        if (it == guide.strains.end()) break;
        chain.push_back({n, it->second});
        edits = edits || it->second.what == SM_EDIT;
        n = it->second.parent;
    }
    // its genes, and what its world genes make and do as its species (J-01: a wild species' too, in every flask)
    MicrobeGenes G = microbeGenesOf(s, form, B.genesis);
    const GeneCode code = geneCodeOf(B.genesis);
    worldOf(kind.world, G);
    const CultureKind wild = kind;
    const Morph wildForm = form;
    for (auto r = chain.rbegin(); r != chain.rend(); ++r) {
        const StrainRecord& R = r->second;
        if (R.what == SM_EDIT) {
            std::vector<GeneEdit> ed = geneEditsOf(R.edits);
            for (GeneEdit& e : ed) if (e.how == GX_MOVED) geneDonor(w, e, depth + 1);   // Y-03: its genes' donors read
            applyEdits(kind, form, G, ed);
            continue;
        }
        if (G.valid) strainGenes(G, code, kind, R.what, R.value, fnv1a(flaskSpeciesKey(w) + "/S" + std::to_string(r->first)));
        applyStrain(kind, R.what, R.value);
        if (R.what == SM_SIZE) { form.lengthUm *= R.value; form.widthUm *= R.value; }
        if (R.what == SM_PIGMENT) form.pigment = (int)R.value;
    }
    if (edits && G.valid) {
        kind.burden = geneBurden(wild, wildForm, kind, form, G);   // Y-03: what its genes cost it
        for (CultureSwitch& s : kind.switches)                     // Y-04: a switched gene's part of it, not paid while it is off
            if (const MicrobeGene* g = G.at(s.place)) s.price = genePrice(wild, wildForm, kind, form, *g);
    }
    if (genes) *genes = G;
    return true;
}

// Y-04: a flask's line (or its parent) as a state of its culture has it (`CultureOne::state`): its genes switched off there lose their work, its
// form's too (the parent's warmth against its own best)
void Game::flaskLineIn(int i, int k, int state, CultureKind& kind, Morph& form, bool parent) const {
    const Flask& F = lab.flasks[clampi(i, 0, INCUBATOR_FLASKS - 1)];
    k = clampi(k, 0, 1);
    kind = parent ? F.parentKind[k] : F.culture.kind[k];
    form = parent ? F.parentForm[k] : F.forms[k];
    if (state < 0 || kind.switches.empty()) return;
    uint32_t st = (uint32_t)state;
    if (parent) st = (st & ~(uint32_t)CST_COLD) | (F.medium.tempC < kind.s.tOpt ? (uint32_t)CST_COLD : 0u);
    switchedLine(kind, form, parent ? F.parentGenes[k] : F.genes[k], st);
}

// Y-03: a moved gene's donor read: its key (its species' or its line's) back into a culture, made as a flask makes it (its own lines and their
// donors, a few deep at most), its gene at the edit's place; whose that gene was (its donor's, or the species it came from before) and how far
// it came to the line's species. False where the donor has no gene there to read (how far it came then by its key alone)
bool Game::geneDonor(const FlaskSpecies& line, GeneEdit& e, int depth) const {
    e.read = false;
    FlaskSpecies d;
    if (!speciesOfKey(e.donor, d)) { e.far = GFROM_STAR; return false; }
    const size_t sp = e.donor.rfind("/S");
    if (sp != std::string::npos && sp > e.donor.rfind("/M")) d.strain = d.strainIn = atoi(e.donor.c_str() + sp + 2);
    e.origin = flaskSpeciesKey(d);
    MicrobeGenes dg;
    double slept = 0;
    if (depth <= 6 && flaskKind(d, e.donorKind, e.donorForm, slept, &dg, depth)) {
        const MicrobeGene* G = dg.at(e.place);
        if (G && !G->cut) { e.donorGene = *G; e.read = true; if (!G->origin.empty()) e.origin = G->origin; }
    }
    e.far = geneFar(flaskSpeciesKey(line), e.origin);
    return e.read;
}
int Game::geneFar(const std::string& speciesKey, const std::string& origin) const {
    if (origin.empty() || origin == speciesKey) return GFROM_OWN;
    FlaskSpecies a, b;
    if (!speciesOfKey(speciesKey, a) || !speciesOfKey(origin, b) || a.sx != b.sx || a.sy != b.sy || a.sz != b.sz) return GFROM_STAR;
    return a.body == b.body ? GFROM_WORLD : GFROM_SYSTEM;
}

// a flask's culture made again from its species and events: the kinds read from their worlds, its laws laid out, its forms, its names
void Game::flaskMake(int i) {
    if (i < 0 || i >= INCUBATOR_FLASKS) return;
    Flask& F = lab.flasks[i];
    F.culture = FlaskCulture(); F.run = CultureRun(); F.forms.clear();
    for (int k = 0; k < F.n && k < 2; k++) {
        CultureKind kind; Morph form; double slept = 0;
        F.genes[k] = MicrobeGenes(); F.parentGenes[k] = MicrobeGenes(); F.switchOnly[k].clear();
        if (!flaskKind(F.who[k], kind, form, slept, &F.genes[k])) { F.n = k; break; }   // its world no longer there
        // a later strain's moves on the form (its size, its pigment): the strains its events made
        for (const CultureEvent& e : F.events) if (e.kind == CD_STRAIN && e.who == k) {
            if (e.what == SM_SIZE) { form.lengthUm *= e.value; form.widthUm *= e.value; }
            if (e.what == SM_PIGMENT) form.pigment = (int)e.value;
        }
        F.culture.kind[k] = kind; F.forms.push_back(form); F.sleptYears[k] = slept;
        F.label[k] = flaskSpeciesWords(F.who[k], form, false);
        // Y-02: an edited line, compared with its parent as it grows and is looked at (a strain arisen in the flask since ends it)
        F.edited[k] = false; F.changed[k].clear(); F.changedKind[k].clear(); F.genesis[k] = 0;
        const auto rec = guide.strains.find(flaskSpeciesKey(F.who[k]) + "/S" + std::to_string(F.who[k].strainIn));
        if (F.who[k].strainIn > 0 && F.who[k].strain == F.who[k].strainIn && rec != guide.strains.end() && rec->second.what == SM_EDIT) {
            FlaskSpecies p = F.who[k]; p.strain = p.strainIn = rec->second.parent;
            MicrobeGenes pg;
            double ps = 0;
            if (flaskKind(p, F.parentKind[k], F.parentForm[k], ps, &pg)) {
                F.edited[k] = true; F.genesis[k] = pg.genesis.id;
                const GeneCode code = geneCodeOf(pg.genesis);
                for (const GeneEdit& e : geneEditsOf(rec->second.edits))
                    if (pg.at(e.place) || e.how != GX_HAND) {   // (Y-03: a gap's too; Y-04: a switch alone kept apart)
                        F.changed[k].push_back(e.place); F.changedKind[k].push_back(code.kind[e.place]);
                        if (e.switchOnly()) F.switchOnly[k].push_back(e.place);
                    }
                F.parentGenes[k] = pg;
            }
        }
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

// J-01: what a flask's world genes show now: each look by the species that makes it, as strong as its amount and its numbers (a gas only while
// its cells grow; a dye and a sediment from all it grew, the dead's too); overgrown, frozen, poisoned as its stages say; genes passed between
// two of one genesis once both grew, from the one that carries to the one that does not
FlaskShow Game::flaskShow(int i) const {
    FlaskShow S;
    const Flask& F = lab.flasks[clampi(i, 0, INCUBATOR_FLASKS - 1)];
    if (F.n == 0 || !F.made) return S;
    const CultureNow now = flaskNow(i);
    auto decades = [](double v, double from, double span) { return clampd((std::log10(std::max(1.0, v)) - from) / span, 0, 1); };
    for (int k = 0; k < now.n && k < 2; k++) {
        const CultureOne& o = now.one[k];
        const CultureWorld& W = F.culture.kind[k].world;
        const double d = decades(o.cells, 4, 5), all = decades(o.cells + o.ghosts + o.spores, 5, 4);
        const bool active = o.stage == SG_GROWING || o.stage == SG_COLONY || o.stage == SG_CYCLING || o.stage == SG_OVERGROWN;
        auto take = [&](double v, double& to, int& who) { if (v > to) { to = v; who = k; } };
        if (W.gives() && (active || o.stage == SG_CROWD)) {   // (at its crowd, fewer)
            const double b = W.gasRate * std::max(0.3, d) * (active ? 1 : 0.4);
            if (b > S.bubbles) { S.bubbles = b; S.gasWho = k; S.gas = W.gas; }
        }
        if (W.dyes()) { const double v = W.dyeRate * all; if (v > S.dyed) { S.dyed = v; S.dyeWho = k; S.dye = W.dye; } }
        if (W.lift > 0) take(W.lift * d, S.scum, S.scumWho);
        if (W.slime > 0) take(W.slime * d, S.slime, S.slimeWho);
        if (W.shells()) { const double v = W.shellRate * all; if (v > S.sediment) { S.sediment = v; S.shellWho = k; S.shell = W.shell; } }
        if (o.stage == SG_OVERGROWN) { S.overgrown = true; S.overWho = k; S.tissue = S.tissue || W.bind > 0; }
        else if (W.bind > 0) take(W.bind * d, S.clumps, S.clumpWho);
        if (o.stage == SG_FROZEN && !S.frozen) {
            S.frozen = true;
            S.iceWho = F.n > 1 && F.culture.kind[1].world.frostC > F.culture.kind[0].world.frostC ? 1 : 0;
        }
    }
    if (F.n == 2 && F.run.pair == PA_POISON && F.run.lead >= 0 && now.one[1 - F.run.lead].stage == SG_POISONED) S.poisoned = 1 - F.run.lead;
    if (F.n == 2 && F.run.pair != PA_IGNORE) {
        const CultureKind &a = F.culture.kind[0], &b = F.culture.kind[1];
        const bool ca = a.world.carrier > 0, cb = b.world.carrier > 0;
        const bool kin = a.s.genesis != 0 && a.s.genesis == b.s.genesis;
        const bool grew = (cultureStagesBy(F.run, 0, lab.clock) & LC_GROWING) && (cultureStagesBy(F.run, 1, lab.clock) & LC_GROWING);
        if (kin && grew && ca != cb && now.one[0].cells > 0 && now.one[1].cells > 0) S.passedTo = ca ? 1 : 0;
    }
    return S;
}

// the lab's clock, every simulating frame (the drones' kind): what each flask comes to is said as the clock reaches it (its spores woken, a
// strain arisen: an event of its own then, the strain the guide's), its crowd's glow
void Game::updateLab(double realDt) {
    lab.clock += realDt;
    sequencerTick();   // Z-06: the sequencer's reading, its end as the clock reaches it
    editorTick();      // Y-02: the gene editor's writing, its line into a flask as the clock reaches its end
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
            const std::string line = fmt("A STRAIN OF %s IN FLASK %d: %s", was.c_str(), i + 1, strainMoveWords(r.what, r.value).c_str());
            status(line + " - N TO NAME IT", 6);
            logEvent("STRAIN", line);
            guide.save(guidePath);
        }
        for (int k = 0; k < F.n; k++)   // R-433: grown (its curve past its lag): kept in the collection; B-441: with what fed it
            if (!(F.told & (FT_KEPT << k)) && (cultureStagesBy(F.run, k, lab.clock) & LC_GROWING)) keepCulture(i, k);
        flaskLearn(i);   // Y-02: an edited line against its parent
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
            for (int k = 0; k < now.n; k++) if (now.one[k].glowing) {   // (Y-04: a glow lit by its switch, its cells')
                F.told |= FT_GLOW;
                status(fmt("FLASK %d GLOWS: %s, %s", i + 1, F.label[k].c_str(), F.culture.kind[k].glowAny ? "SWITCHED ON" : "CROWDED"), 5);
                break;
            }
        }
    }
    geneSay();   // Y-02: what the genes showed, said in its turn
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

// R-435: the chosen flask's warmth typed: a digit begins it (the warmth's field chosen); digits, the minus sign, Backspace, Up and Down a degree
// (Page Up and Down ten); Enter sets it within its cell's range as one change of its medium, Esc keeps the warmth it has. True while typing
bool Game::incubatorTypeKeys(const Input& in) {
    Flask& F = lab.flasks[clampi(incub.sel, 0, INCUBATOR_FLASKS - 1)];
    if (!incub.typing) {
        const int d = digitKey(in);
        if (d < 0) return false;
        if (F.n == 0) { refuse("PUT A CULTURE IN FIRST - ENTER", 2); return true; }
        incub.typing = true; incub.typed = std::string(1, (char)('0' + d)); incub.field = 1; audio.beep = BEEP_UI;
        return true;
    }
    if (in.wasPressed(KEY_ESCAPE) || F.n == 0) { incub.typing = false; status(fmt("FLASK %d STAYS AT %.0f C", incub.sel + 1, F.medium.tempC), 2); audio.beep = BEEP_UI; return true; }
    warmthTyping(in, incub.typed, F.medium.tempC);
    if (!enterKey(in)) return true;
    if (incub.typed.empty() || incub.typed == "-") { incub.typing = false; return true; }
    double lo, hi;
    cellRange(F.medium.cell, lo, hi);
    const int v = atoi(incub.typed.c_str());
    if (v < lo || v > hi) { refuse(fmt("%s CELL: %.0f TO %.0f C ONLY", CULTURE_CELL_NAMES[F.medium.cell], lo, hi), 3); return true; }
    incub.typing = false;
    Medium m = F.medium; m.tempC = v;
    flaskSet(incub.sel, m);
    status(fmt("FLASK %d AT %d C", incub.sel + 1, v), 3); audio.beep = BEEP_UI;
    return true;
}

// ---- putting in, the medium, emptying -----------------------------------------------------------------------------------------------

// the chooser's rows: for a flask, each living or sleeping species seen in a cold store's vial (never a fossil or a protocell: they do not
// grow), each flask's culture (carried over) and (R-433) each culture of the collection. R-432:
// in folders, a world's (a row a species and its state, cells or spores, from the first vial holding it), the flasks' and the collection's
void Game::incubatorChoices() {
    flaskMadeAll();   // (the flasks' words)
    incub.choices.clear(); incub.choiceSel = 0; incub.choiceTop = 0;
    incub.pages = ChooserPage();
    std::vector<std::string> keys;
    LabSystems systems;
    std::map<std::string, int> listed;   // a species' row by its key and its state (its cells and its spores apart)
    std::vector<std::string> looks, rests, traits;   // B-442: each row's look, the rest of its words, its form's colour and motion
    auto add = [&](const IncubatorChoice& c, const std::string& key, const std::string& look, const std::string& rest, const std::string& trait) {
        incub.choices.push_back(c); keys.push_back(key); looks.push_back(look); rests.push_back(rest); traits.push_back(trait);
    };
    for (int v = 0; v < (int)coldStore.size(); v++) {
        const Vial& V = coldStore[v];
        const StarSystem* S = systems.get(sys, V.sx, V.sy, V.sz);
        if (!S || V.body < 0 || V.body >= (int)S->bodies.size()) continue;
        auto L = lifeOf(*S);
        const Biota& B = L->bodies[V.body];
        Sample smp = sampleOf(B, V.hab);
        std::vector<Morph> forms;
        for (const VialLine& SL : vialLines(V, *S, smp)) {   // J-03: a seeded line where it spread, once seen (in a flask as it went down)
            FlaskSpecies w;
            if (!speciesOfKey(SL.key, w)) continue;
            const size_t sp = SL.key.rfind("/S");
            if (sp != std::string::npos && sp > SL.key.rfind("/M")) w.strain = w.strainIn = atoi(SL.key.c_str() + sp + 2);
            const auto it = guide.microbes.find(flaskSpeciesKey(w));
            if (it == guide.microbes.end() || !(it->second.seen & SEEN_FORM)) continue;
            if (!listed.emplace(SL.key + "|" + std::to_string(v), (int)incub.choices.size()).second) continue;
            IncubatorChoice c; c.vial = v; c.lineKey = SL.key;
            add(c, Guide::bodyKey(V.sx, V.sy, V.sz, V.body), flaskSpeciesWords(w, SL.line.form, true), " - SEEDED, FROM " + V.name, formTraits(SL.line.form));
        }
        for (const Held& H : smp.held) {
            if (H.state == SS_FOSSIL || H.state == SS_PROTO) continue;
            const std::string world = Guide::bodyKey(V.sx, V.sy, V.sz, V.body), key = world + "/M" + std::to_string(H.species);
            const auto it = guide.microbes.find(key);
            if (it == guide.microbes.end() || !(it->second.seen & SEEN_FORM)) continue;   // only what was seen of it
            if (!listed.emplace(key + (H.state == SS_DORMANT ? "/D" : ""), (int)incub.choices.size()).second) continue;   // each once
            if (forms.empty()) forms = morphsOf(*S, V.body);
            const Morph& m = forms[H.species];
            FlaskSpecies w; w.sx = V.sx; w.sy = V.sy; w.sz = V.sz; w.body = V.body; w.species = H.species;
            IncubatorChoice c; c.vial = v; c.species = H.species;
            add(c, world, flaskSpeciesWords(w, m, true), " - " + (H.state == SS_DORMANT ? std::string(m.plan >= MP_AMOEBA ? "CYSTS" : "SPORES") + " FROM " : std::string("FROM ")) +
                V.name, formTraits(m));   // B-442: its look and its size where it has no name
        }
    }
    for (int f = 0; f < INCUBATOR_FLASKS; f++) {
        if (f == incub.sel || lab.flasks[f].n == 0) continue;
        IncubatorChoice c; c.flask = f;
        add(c, "FLASKS", fmt("FLASK %d: %s", f + 1, lab.flasks[f].label[0].c_str()) + (lab.flasks[f].n > 1 ? " + " + lab.flasks[f].label[1] : std::string()), "", "");
    }
    for (const LabFolder& F : cultureFolders())   // R-433: the collection, its folders in their order
        for (int k : F.id >= 0 ? F.rows : std::vector<int>()) {
            IncubatorChoice c; c.kept = k;
            std::string trait;
            const std::string look = keptRow(k, &trait);
            add(c, keptFolderKey(k), look, "", trait);
        }
    apartRows(looks, rests, keys, traits);
    for (size_t i = 0; i < incub.choices.size(); i++) incub.choices[i].line = looks[i] + rests[i];
    incub.pages.folders = chooserFolders(keys);
    chooserTabs(incub.pages, incub.lastTab);   // R-441: the vials, the cultures; the tab last shown
}

// AS ITS WORLD: a vial's place's medium (its world's chemistry and light, the vial's warmth and salt); false where its world is no longer there
bool Game::vialMedium(const Vial& V, Medium& out) {
    Star st; StarSystem S;
    const StarSystem* P = &sys;
    if (!(sys.valid && sys.star.sx == V.sx && sys.star.sy == V.sy && sys.star.sz == V.sz)) {
        if (!placeInSector(V.sx, V.sy, V.sz, st, true)) return false;
        S.generate(st); P = &S;
    }
    if (V.body < 0 || V.body >= (int)P->bodies.size()) return false;
    auto L = lifeOf(*P);
    const int cls = P->sunless() ? STAR_ROGUE : (P->ofCompanion(V.body) ? P->companionStar().cls : P->star.cls);
    out = mediumOfPlace(L->bodies[V.body], P->bodies[V.body].type, cls, V.hab, V.readingC);
    return true;
}

// a choice put in the flask chosen: a vial's species (its first, AS ITS WORLD the medium's first guess; or a second added), or a flask's culture
// carried over (its species and strains, its medium; or its first added as a second)
bool Game::flaskPut(int i, const IncubatorChoice& c) {
    if (i < 0 || i >= INCUBATOR_FLASKS) return false;
    Flask& F = lab.flasks[i];
    if (F.n >= 2) { refuse("TWO PER FLASK AT MOST", 3); return false; }
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
        if (K.paired) { FlaskSpecies p = K.partner; p.strainIn = p.strain; in.push_back(p); }   // B-441: with what fed it
        if (F.n == 0) { F.medium = K.medium; asWorld = K.asWorld; }
    } else if (c.vial >= 0 && c.vial < (int)coldStore.size()) {
        const Vial& V = coldStore[c.vial];
        FlaskSpecies w; w.sx = V.sx; w.sy = V.sy; w.sz = V.sz; w.body = V.body; w.species = c.species;
        if (!c.lineKey.empty()) {   // J-03: a seeded line, as it went down
            if (!speciesOfKey(c.lineKey, w)) return false;
            const size_t sp = c.lineKey.rfind("/S");
            if (sp != std::string::npos && sp > c.lineKey.rfind("/M")) w.strain = w.strainIn = atoi(c.lineKey.c_str() + sp + 2);
        }
        in.push_back(w);
        if (F.n == 0) {   // AS ITS WORLD: the vial's place's medium
            if (!vialMedium(V, asWorld)) { refuse("VIAL'S WORLD NO LONGER EXISTS"); return false; }
            F.medium = asWorld;
        }
    } else return false;
    for (const FlaskSpecies& w : in) for (int k = 0; k < F.n; k++) if (F.who[k].sameSpecies(w) && F.who[k].strain == w.strainIn) { refuse("ALREADY IN THIS FLASK", 3); return false; }
    if (F.n + (int)in.size() > 2) { refuse("TWO PER FLASK AT MOST", 3); return false; }
    const bool fresh = F.n == 0;
    for (const FlaskSpecies& w : in) {
        CultureEvent e; e.kind = F.n == 0 ? CD_START : CD_ADD; e.t = lab.clock; e.m = F.medium;
        if (F.n == 0) { F.events.clear(); F.told = 0; F.asWorld = asWorld; }
        F.who[F.n] = w; F.who[F.n].strain = w.strainIn;
        F.n++;
        F.events.push_back(e);
    }
    flaskMake(i);
    if (fresh) status(fmt(c.vial >= 0 ? "FLASK %d: %s PUT IN, MEDIUM LIKE ITS WORLD" : "FLASK %d: %s PUT IN", i + 1, F.label[0].c_str()), 4);
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
    status(fmt("FLASK %d: 1%% MOVED TO FRESH BROTH", i + 1), 3);
    audio.beep = BEEP_UI;
}

Medium Game::flaskAsItsWorld(int i) const { return lab.flasks[clampi(i, 0, INCUBATOR_FLASKS - 1)].asWorld; }

// ---- the screen ------------------------------------------------------------------------------------------------------------------------

void Game::incubatorEnter() {
    incub.choosing = false; incub.typing = false; incub.pick = PICK_NONE; keep.open = false;
    incub.sel = clampi(incub.sel, 0, INCUBATOR_FLASKS - 1);
    flaskMadeAll();
}

void Game::updateIncubator(const Input& in, double dt, double realDt) {
    if (keepKeys(in)) return;   // R-440: the folder a copy goes in
    if (incub.pick) { incubatorPickKeys(in); return; }   // R-440, R-442: what K keeps or N names of a flask of two
    if (!incub.choosing && incubatorTypeKeys(in)) return;   // R-435: its warmth typed (the other keys wait)
    if (in.wasPressed(KEY_E) && !in.ctrl()) { closeScreen(); audio.beep = BEEP_UI; return; }
    if (incub.choosing) {   // R-432: its folders, then the rows of the one open
        const int r = incub.choices.empty() ? -2 : chooserKeys(in, incub.pages, incub.choiceSel);
        if (incub.pages.tabbed) incub.lastTab = incub.pages.tab;
        if (r == -2) { incub.choosing = false; audio.beep = BEEP_UI; return; }
        if (r >= 0 && r < (int)incub.choices.size()) {
            const IncubatorChoice c = incub.choices[r];
            incub.choosing = false;
            flaskPut(incub.sel, c);
        }
        return;
    }
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); audio.beep = BEEP_UI; return; }
    if (saveKey(in)) { saveSlot(currentSlot); return; }
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
        else { flaskSet(incub.sel, F.asWorld); status(fmt("FLASK %d: MEDIUM SET LIKE ITS WORLD", incub.sel + 1), 3); audio.beep = BEEP_UI; }
    }
    if (in.wasPressed(KEY_F) && !in.ctrl()) { if (F.n == 0) refuse("FLASK EMPTY", 2); else flaskFresh(incub.sel); }
    if (enterKey(in)) {
        if (F.n >= 2) refuse("TWO PER FLASK AT MOST", 3);
        else {
            incubatorChoices();
            if (incub.choices.empty()) refuse("NOTHING TO PUT IN - LOOK AT A VIAL UNDER THE MICROSCOPE", 4);
            else { incub.choosing = true; audio.beep = BEEP_UI; }
        }
    }
    if (F.n > 0 && (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE))) flaskEmpty(incub.sel);
    if (in.wasPressed(KEY_N) && !in.ctrl()) { incubatorName(); return; }   // R-442
    if (in.wasPressed(KEY_K) && !in.ctrl()) { incubatorKeep(); return; }   // R-440
    flaskSeen(incub.sel);
}

// R-442: N names the flask's organism: each species (its look, or its name) and its strain, one named at once, more chosen from rows first
void Game::incubatorName() {
    const Flask& F = lab.flasks[incub.sel];
    if (F.n == 0 || !F.made) { refuse("FLASK EMPTY - ENTER PUTS IN A CULTURE", 3); return; }
    incub.pickLines.clear(); incub.pickKeys.clear(); incub.pickStrain.clear();
    for (int k = 0; k < F.n && k < (int)F.forms.size(); k++) {
        FlaskSpecies w = F.who[k]; w.strain = 0;
        incub.pickLines.push_back(flaskSpeciesWords(w, F.forms[k], true));
        incub.pickKeys.push_back(flaskSpeciesKey(w)); incub.pickStrain.push_back(0);
        if (F.who[k].strain <= 0) continue;
        const std::string st = nameOfKey(flaskStrainKey(F.who[k]));
        incub.pickLines.push_back(std::string("STRAIN ") + lineTag(F.who[k]) + (st == "UNKNOWN" ? std::string() : ": " + upper(st)));
        incub.pickKeys.push_back(flaskStrainKey(F.who[k])); incub.pickStrain.push_back(1);
    }
    if (incub.pickKeys.size() == 1) { incubatorNameKey(incub.pickKeys[0], incub.pickStrain[0]); return; }
    incub.pick = PICK_NAME; incub.pickSel = 0; audio.beep = BEEP_UI;
}
void Game::incubatorNameKey(const std::string& key, bool strain) {
    const std::string named = nameOfKey(key);
    (strain ? textStrainKey : textSpeciesKey) = key;
    beginTextEntry(strain ? "NAME THE STRAIN" : "NAME THE SPECIES", strain ? TEXT_STRAIN : TEXT_SPECIES, named == "UNKNOWN" ? std::string() : upper(named));
}
// R-440: K keeps a copy of the flask's culture in a folder (the flask grows on): its one species, or of two either alone or both
void Game::incubatorKeep() {
    const Flask& F = lab.flasks[incub.sel];
    if (F.n == 0 || !F.made) { refuse("FLASK EMPTY - ENTER PUTS IN A CULTURE", 3); return; }
    if (F.n == 1) { keepFromFlask(incub.sel, 0); return; }
    incub.pickLines = {F.label[0] + " ALONE", F.label[1] + " ALONE", "BOTH TOGETHER"};
    incub.pick = PICK_KEEP; incub.pickSel = 0; audio.beep = BEEP_UI;
}
// `which`: its first alone, its second alone, both together; a species dead in it refused
void Game::keepFromFlask(int i, int which) {
    const Flask& F = lab.flasks[i];
    const CultureNow now = flaskNow(i);
    for (int k = 0; k < F.n; k++) {
        const CultureOne& o = now.one[k];
        if ((which == 2 || which == k) && (o.stage == SG_DEAD || (o.cells < 1 && o.spores < 1))) { refuse(F.label[k] + " IS DEAD - NOTHING TO KEEP", 3); return; }
    }
    KeptCulture c;
    const int k = which == 2 ? 0 : clampi(which, 0, F.n - 1);
    c.who = F.who[k]; c.who.strainIn = c.who.strain;
    c.medium = F.medium; c.asWorld = F.asWorld;
    c.plan = k < (int)F.forms.size() ? F.forms[k].plan : -1;
    if (which == 2 && F.n == 2) {
        c.paired = true; c.partner = F.who[1]; c.partner.strainIn = c.partner.strain;
        c.partnerPlan = F.forms.size() > 1 ? F.forms[1].plan : -1;
    }
    keepOpen(c);
}
void Game::incubatorPickKeys(const Input& in) {
    const int n = (int)incub.pickLines.size();
    if (in.wasPressed(KEY_ESCAPE) || n == 0) { incub.pick = PICK_NONE; audio.beep = BEEP_UI; return; }
    menuStep(in, incub.pickSel, n);
    if (!enterKey(in)) return;
    const int r = clampi(incub.pickSel, 0, n - 1), what = incub.pick;
    incub.pick = PICK_NONE;
    if (what == PICK_NAME && r < (int)incub.pickKeys.size()) incubatorNameKey(incub.pickKeys[r], incub.pickStrain[r]);
    else if (what == PICK_KEEP) keepFromFlask(incub.sel, r);
}

void Game::incubatorPad(PadMenu& m) const {
    m.title = "INCUBATOR";
    if (keep.open) { m.item("KEEP A COPY THERE", KEY_ENTER); m.item("NEW FOLDER", KEY_N); m.item("BACK", KEY_ESCAPE); return; }   // R-440
    if (incub.pick) { m.item(incub.pick == PICK_NAME ? "NAME IT" : "KEEP IT", KEY_ENTER); m.item("BACK", KEY_ESCAPE); return; }
    if (incub.choosing) {
        m.item(incub.pages.open ? "PUT IN" : "OPEN FOLDER", KEY_ENTER);
        if (incub.pages.tabbed) m.item(incub.pages.tab == 0 ? "CULTURES" : "VIALS", KEY_TAB);   // R-441
        m.item("BACK", KEY_ESCAPE);
        return;
    }
    const Flask& F = lab.flasks[incub.sel];
    m.item(F.n == 0 ? "PUT IN A CULTURE" : "ADD A SECOND", KEY_ENTER);
    m.item("NEXT FIELD", KEY_RIGHT); m.item("RAISE", KEY_EQUAL); m.item("LOWER", KEY_MINUS);
    m.item("LIKE ITS WORLD", KEY_W); m.item("FRESH BROTH", KEY_F); m.item("EMPTY FLASK", KEY_BACKSPACE); m.item("NAME IT", KEY_N);
    m.item("KEEP A COPY", KEY_K);
    m.item("BACK", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "PUT IN"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_X, KEY_W, "AS WORLD"); m.button(PR_Y, KEY_F, "FRESH");
    m.button(PR_LB, KEY_MINUS, "LOWER"); m.button(PR_RB, KEY_EQUAL, "RAISE");
}

// ---- the save's lines: the lab's clock, each flask's medium and species and events (the line after them the screen's own) -------------
// An older save's `jar` lines (R-447: the jars taken out) are skipped, as is any line no row takes

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
    return keptLoad(key, is);   // R-433
}

void Game::incubatorReset() { lab = LabState(); incub = IncubatorState(); for (Flask& F : lab.flasks) F.made = true; }
