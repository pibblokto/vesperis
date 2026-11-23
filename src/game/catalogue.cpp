// Z-07: the catalogue (R-425; docs/ideas/ITERATION-1.5.0.md Z-07, IDEAS-small-and-spacious.md 1.9; lab_state.h), the guide's page of every
// species seen under the microscope (the guide's sheets) and of a friend's lent by their inbox file (in cyan): by world (a world's head, then
// its species) or, Tab, by kin (the sequenced in the tree's order, a genesis's head before its first; then those not read). Enter shows a
// species' sheet beside its form drawn live in a way of seeing it was seen in (V the next), Up and Down the next species; N names it. Nothing
// of it is saved: its worlds are drawn as it opens, the names read as it draws. J-12 (R-448): a seeded world's head names its made kind, its
// natives a line drove out read DIED OUT (the sheet says which line and how), the lines found wild there follow its species (SEEDED: the sheet
// says what the wild changed once the sequencer read one there), then its animals seen (GONE, DYING, CHANGED as the ground showed them)
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>

namespace {
constexpr int CAT_Y0 = 17, CAT_ROW = 9, CAT_ROWS = 18;           // the list: a row every 9 px under the title
constexpr int FORM_X = 6, FORM_Y = 16, FORM_W = 150, FORM_H = 120;   // the sheet's form, live, in a box of the slide's middle
std::string worldOfKey(const std::string& key) { const size_t m = key.rfind("/M"); return m == std::string::npos ? key : key.substr(0, m); }
int indexOfKey(const std::string& key) { const size_t m = key.rfind("/M"); return m == std::string::npos ? -1 : atoi(key.c_str() + m + 2); }
// J-12: an animal's key's world and index; a world's key read back into its star's sector and its body
std::string worldOfAnimal(const std::string& key) { const size_t a = key.rfind("/A"); return a == std::string::npos ? key : key.substr(0, a); }
int indexOfAnimal(const std::string& key) { const size_t a = key.rfind("/A"); return a == std::string::npos ? -1 : atoi(key.c_str() + a + 2); }
bool worldParts(const std::string& w, int64_t& sx, int64_t& sy, int64_t& sz, int& body) {
    const size_t s = w.find('/');
    if (s == std::string::npos || !Guide::parseStarKey(w, sx, sy, sz)) return false;
    body = atoi(w.c_str() + s + 1);
    return body >= 0;
}
// the form seen of it (its sheet's): a cell where any was seen, else its spore or its cyst, else its fossil
int formSeen(const MicrobeSheet& seen) {
    const uint32_t cells = (1u << CF_CELL) | (1u << CF_DIVIDING) | (1u << CF_COLONY) | (1u << CF_SLUG) | (1u << CF_STALK) | (1u << CF_GHOST);
    if (seen.forms & cells) return CF_CELL;
    if (seen.forms & (1u << CF_SPORE)) return CF_SPORE;
    if (seen.forms & (1u << CF_CYST)) return CF_CYST;
    return CF_FOSSIL;
}
// the ways of seeing its form was seen in: the light ways on any slide, the dark field its flagella, the glow its DNA, the electron its wall; a
// fossil's the section alone
std::vector<int> waysSeen(const MicrobeSheet& seen) {
    if (formSeen(seen) == CF_FOSSIL) return {WS_SECTION};
    std::vector<int> w = {WS_BRIGHT, WS_PHASE};
    if (seen.seen & SEEN_FLAGELLA) w.push_back(WS_DARK);
    if (seen.seen & SEEN_DNA) w.push_back(WS_GLOW);
    if (seen.seen & SEEN_WALL) w.push_back(WS_ELECTRON);
    return w;
}
// a scale bar's length: the roundest under three tenths of the box
double barUm(double widthUm) {
    static const double STEPS[] = {0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
    double b = STEPS[0];
    for (double s : STEPS) if (s <= widthUm * 0.3) b = s;
    return b;
}
uint64_t keySeed(const std::string& key) { uint64_t h = 1469598103934665603ULL; for (char c : key) h = (h ^ (uint8_t)c) * 1099511628211ULL; return mix64(h); }
}   // namespace

// its sheet: the explorer's own, else a friend's lent (none: empty)
const MicrobeSheet& Game::catalogueSheet(const std::string& key) const {
    static const MicrobeSheet none;
    const auto it = guide.microbes.find(key);
    if (it != guide.microbes.end()) return it->second;
    const auto jt = guide.lentMicrobes.find(key);
    return jt != guide.lentMicrobes.end() ? jt->second : none;
}

void Game::catalogueEnter() {
    cat.page = 0;
    catalogueBuild();
    audio.beep = BEEP_UI;
}

std::string Game::madeKindAt(const std::string& w, LabSystems& systems) {
    int64_t sx = 0, sy = 0, sz = 0; int body = -1;
    if (std::none_of(guide.seeded.begin(), guide.seeded.end(), [&](const SeedRecord& r) { return r.body == w; })) return "";
    const StarSystem* S = worldParts(w, sx, sy, sz, body) ? systems.get(sys, sx, sy, sz) : nullptr;
    if (!S || body >= (int)S->bodies.size()) return "";
    const std::vector<Seeding>& ss = seedingsAt(*S, body);
    return ss.empty() ? std::string() : madeKindName(madeWorldOf(*S, body, ss, guide.clock));
}

// a world as the catalogue reads it: its kind, its species and their forms, drawn once; J-12: its animals' words and ways once one is asked
// for; a seeded world's made kind and the natives its lines drove out at the guide's clock, made again with each list (the clock moves on)
CatalogueWorld& Game::catalogueWorld(const std::string& w, LabSystems& systems, bool animals) {
    CatalogueWorld& W = cat.worlds[w];
    int64_t sx = 0, sy = 0, sz = 0; int body = -1;
    const StarSystem* S = worldParts(w, sx, sy, sz, body) ? systems.get(sys, sx, sy, sz) : nullptr;
    if (!S || body >= (int)S->bodies.size()) return W;
    if (!W.ok) {
        W.kind = S->bodies[body].type;
        W.species = lifeOf(*S)->bodies[body].species;
        W.forms = morphsOf(*S, body);
        W.ok = true;
    }
    if (animals && !W.animalsMade) {   // (the bestiary reads the world as generated: a species' index is the record's)
        const Body& b = S->bodies[body];
        const Bestiary B = Bestiary::make(b, BodyGen::make(b));
        for (int i = 0; i < (int)B.species.size(); i++) { CatalogueAnimal a; a.words = B.words(i); a.way = B.species[i].way; a.social = B.species[i].social; W.animals.push_back(a); }
        W.animalsMade = true;
    }
    W.made.clear(); W.diedOut.clear(); W.took = 0;
    const std::vector<Seeding>& ss = seedingsAt(*S, body);
    if (!ss.empty()) {
        W.made = madeKindName(madeWorldOf(*S, body, ss, guide.clock));
        for (const Seeding& s : ss) W.took += s.verdict.takes();
        const Biota& B = lifeOf(*S)->bodies[body];
        for (const Seeding& s : ss)   // (the first line to drive one out did it)
            for (const DiedOut& d : seedingDiedOut(B, s, guide.clock)) if (!W.diedOut.count(d.species)) W.diedOut[d.species] = {seedLineWords(s.line), d.how};
    }
    return W;
}

// the rows: every species with a sheet (the explorer's, a friend's lent), its world drawn once (one no longer holding it left out), by world
// or by kin; J-12: the seeded lines found wild and the animals seen, by world; the one chosen kept where it is still listed
void Game::catalogueBuild() {
    CatalogueState& C = cat;
    std::vector<std::string> keys, animals;
    for (const auto& kv : guide.microbes) keys.push_back(kv.first);
    for (const auto& kv : guide.lentMicrobes) if (!guide.microbes.count(kv.first)) keys.push_back(kv.first);
    for (const auto& kv : guide.animals) animals.push_back(kv.first);
    LabSystems systems;
    std::set<std::string> made;   // (each world read once a list)
    auto world = [&](const std::string& w, bool withAnimals) -> CatalogueWorld& {
        if (made.insert(w).second || (withAnimals && !C.worlds[w].animalsMade)) return catalogueWorld(w, systems, withAnimals);
        return C.worlds[w];
    };
    for (const std::string& k : keys) world(worldOfKey(k), false);
    for (const std::string& a : animals) world(worldOfAnimal(a), true);
    C.wilds.clear();   // J-12: the seeded lines found wild, each its seeding's on its world
    for (const auto& [wk, read] : guide.wildFound) {
        const size_t l = wk.rfind("/L");
        if (l == std::string::npos) continue;
        const std::string w = wk.substr(0, l);
        const int k = atoi(wk.c_str() + l + 2);
        int64_t sx = 0, sy = 0, sz = 0; int body = -1;
        const StarSystem* S = worldParts(w, sx, sy, sz, body) ? systems.get(sys, sx, sy, sz) : nullptr;
        if (!S || body >= (int)S->bodies.size()) continue;
        const std::vector<Seeding>& ss = seedingsAt(*S, body);
        FlaskSpecies f;
        if (k < 0 || k >= (int)ss.size() || !ss[k].verdict.takes() || !speciesOfKey(ss[k].line, f)) continue;
        const Seeding& s = ss[k];
        CatalogueWild L; L.world = w; L.line = s.line; L.species = flaskSpeciesKey(f); L.form = s.wild.form; L.s = s.wild.kind.s; L.read = read; L.clock = s.clock;
        for (const GeneEdit& e : s.verdict.drift) L.drift += (L.drift.empty() ? "" : ", ") + std::to_string(e.place + 1);
        C.wilds[wk] = L;
        world(w, false);
    }
    auto held = [&](const std::string& k) {
        const auto it = C.worlds.find(worldOfKey(k));
        const int i = indexOfKey(k);
        return it != C.worlds.end() && it->second.ok && i >= 0 && i < (int)it->second.species.size() && i < (int)it->second.forms.size();
    };
    keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const std::string& k) { return !held(k); }), keys.end());
    animals.erase(std::remove_if(animals.begin(), animals.end(), [&](const std::string& a) {
        const auto it = C.worlds.find(worldOfAnimal(a));
        const int i = indexOfAnimal(a);
        return it == C.worlds.end() || i < 0 || i >= (int)it->second.animals.size();
    }), animals.end());
    std::sort(keys.begin(), keys.end(), [](const std::string& a, const std::string& b) {
        const std::string wa = worldOfKey(a), wb = worldOfKey(b);
        return wa != wb ? wa < wb : indexOfKey(a) < indexOfKey(b);
    });
    std::sort(animals.begin(), animals.end(), [](const std::string& a, const std::string& b) { return indexOfAnimal(a) < indexOfAnimal(b); });
    const bool any = C.sel >= 0 && C.sel < (int)C.rows.size();
    const std::string was = any ? C.rows[C.sel].key : std::string(), wasWild = any ? C.rows[C.sel].wild : std::string();
    const int wasKind = any ? C.rows[C.sel].kind : CR_SPECIES;
    C.rows.clear();
    auto species = [&](const std::string& k, uint64_t genesis) { CatalogueRow r; r.key = k; r.lent = guide.isLentMicrobe(k); r.genesis = genesis; C.rows.push_back(r); };
    if (C.order == 0) {   // by world: a world's head, then its species by their index, the lines found wild there, its animals seen
        std::set<std::string> worlds;
        for (const std::string& k : keys) worlds.insert(worldOfKey(k));
        for (const auto& kv : C.wilds) worlds.insert(kv.second.world);
        for (const std::string& a : animals) worlds.insert(worldOfAnimal(a));
        for (const std::string& w : worlds) {
            CatalogueRow h; h.head = true; h.key = w; C.rows.push_back(h);
            for (const std::string& k : keys) if (worldOfKey(k) == w) species(k, 0);
            for (const auto& kv : C.wilds) if (kv.second.world == w) { CatalogueRow r; r.kind = CR_WILD; r.key = kv.second.species; r.wild = kv.first; C.rows.push_back(r); }
            for (const std::string& a : animals) if (worldOfAnimal(a) == w) { CatalogueRow r; r.kind = CR_ANIMAL; r.key = a; C.rows.push_back(r); }
        }
    } else {   // by kin: the sequenced in the tree's order (`treeBefore`), a genesis's head before its first; then those not read
        std::vector<std::pair<Lineage, std::string>> read;
        std::vector<std::string> rest;
        for (const std::string& k : keys) {
            if (catalogueSheet(k).seen & SEEN_KIN) {
                const Lineage& L = lineageOfKey(k);
                if (L.valid) { read.push_back({L, k}); continue; }
            }
            rest.push_back(k);
        }
        std::stable_sort(read.begin(), read.end(), [](const std::pair<Lineage, std::string>& a, const std::pair<Lineage, std::string>& b) { return treeBefore(a.first, b.first); });
        for (size_t i = 0; i < read.size(); i++) {
            const Genesis& g = read[i].first.genesis;
            if (i == 0 || g.id != read[i - 1].first.genesis.id) { CatalogueRow h; h.head = true; h.genesis = g.id; h.line = genesisWords(g); C.rows.push_back(h); }
            species(read[i].second, g.id);
        }
        if (!rest.empty()) {
            CatalogueRow h; h.head = true; h.line = "NOT YET SEQUENCED"; C.rows.push_back(h);
            for (const std::string& k : rest) species(k, 0);
        }
    }
    C.sel = -1;
    for (int i = 0; i < (int)C.rows.size(); i++) if (!C.rows[i].head && C.rows[i].key == was && C.rows[i].kind == wasKind && C.rows[i].wild == wasWild) C.sel = i;
    if (C.sel < 0) for (int i = 0; i < (int)C.rows.size(); i++) if (!C.rows[i].head && C.rows[i].key == was && C.rows[i].kind == CR_SPECIES) C.sel = i;   // (by kin)
    if (C.sel < 0) for (int i = 0; i < (int)C.rows.size(); i++) if (!C.rows[i].head) { C.sel = i; break; }
}

// the next species' row each way (heads passed over), at the list's ends none
void Game::catalogueStep(int d) {
    CatalogueState& C = cat;
    int i = C.sel;
    for (int n = std::abs(d); n > 0; n--) {
        int j = i + (d > 0 ? 1 : -1);
        while (j >= 0 && j < (int)C.rows.size() && C.rows[j].head) j += d > 0 ? 1 : -1;
        if (j < 0 || j >= (int)C.rows.size()) break;
        i = j;
    }
    if (i != C.sel) { C.sel = i; audio.beep = BEEP_UI; }
}

// N: the species chosen named; J-12: an animal as on the ground, a line found wild as in the incubator (a strain's its own name)
void Game::catalogueName() {
    if (cat.sel < 0 || cat.sel >= (int)cat.rows.size()) { refuse("NO SPECIES TO NAME", 3); return; }
    const CatalogueRow& R = cat.rows[cat.sel];
    if (R.kind == CR_ANIMAL) {
        textAnimalKey = R.key;
        const auto it = guide.names.find(textAnimalKey);
        beginTextEntry("NAME THE ANIMAL", TEXT_ANIMAL, it != guide.names.end() ? it->second : "");
        return;
    }
    const auto w = R.kind == CR_WILD ? cat.wilds.find(R.wild) : cat.wilds.end();
    if (w != cat.wilds.end() && w->second.line != R.key) {
        textStrainKey = w->second.line;
        const std::string named = nameOfKey(textStrainKey);
        beginTextEntry("NAME THE STRAIN", TEXT_STRAIN, named == "UNKNOWN" ? std::string() : upper(named));
        return;
    }
    textSpeciesKey = R.key;
    const std::string named = nameOfKey(textSpeciesKey);
    beginTextEntry("NAME THE SPECIES", TEXT_SPECIES, named == "UNKNOWN" ? std::string() : upper(named));
}

void Game::updateCatalogue(const Input& in, double dt, double realDt) {
    CatalogueState& C = cat;
    const bool any = C.sel >= 0 && C.sel < (int)C.rows.size();
    if (C.page == 1 && any) {   // a species' sheet: back to the list, the next species, the next way of seeing, a name
        if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_BACKSPACE) || enterKey(in)) { C.page = 0; audio.beep = BEEP_UI; return; }
        const int was = C.sel;
        if (in.wasPressed(KEY_DOWN) || in.wasPressed(KEY_RIGHT)) catalogueStep(1);
        if (in.wasPressed(KEY_UP) || in.wasPressed(KEY_LEFT)) catalogueStep(-1);
        const std::vector<int> ways = waysSeen(catalogueSheet(C.rows[C.sel].key));
        if (C.sel != was) { C.seeing = ways[0]; C.t0 = realTime; }
        if (in.wasPressed(KEY_V) && C.rows[C.sel].kind == CR_ANIMAL) refuse("ANIMALS ARE SEEN ON THE GROUND", 3);   // J-12
        else if (in.wasPressed(KEY_V)) {
            const auto it = std::find(ways.begin(), ways.end(), C.seeing);
            C.seeing = it == ways.end() || it + 1 == ways.end() ? ways[0] : *(it + 1);
            if (ways.size() < 2) refuse("SEEN ONLY THIS WAY - THE MICROSCOPE SHOWS MORE", 3);
            else audio.beep = BEEP_UI;
        }
        if (in.wasPressed(KEY_N)) catalogueName();
        return;
    }
    C.page = 0;
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    if (in.wasPressed(KEY_TAB)) { C.order ^= 1; catalogueBuild(); audio.beep = BEEP_UI; return; }
    if (!any) return;
    if (in.wasPressed(KEY_DOWN)) catalogueStep(1);
    if (in.wasPressed(KEY_UP)) catalogueStep(-1);
    if (in.wasPressed(KEY_PAGE_DOWN)) catalogueStep(CAT_ROWS - 2);
    if (in.wasPressed(KEY_PAGE_UP)) catalogueStep(-(CAT_ROWS - 2));
    if (enterKey(in)) { C.page = 1; C.seeing = waysSeen(catalogueSheet(C.rows[C.sel].key))[0]; C.t0 = realTime; audio.beep = BEEP_UI; }
    if (in.wasPressed(KEY_N)) catalogueName();
}

// J-12: a seeded line's name as the incubator gives it (a strain's own, else its species' and its tag: "UNNAMED E2")
std::string Game::catalogueLineName(const std::string& lineKey) const {
    FlaskSpecies w;
    if (!speciesOfKey(lineKey, w)) return "A CULTURE";
    const size_t sp = lineKey.rfind("/S");
    if (sp != std::string::npos && sp > lineKey.rfind("/M")) w.strain = w.strainIn = atoi(lineKey.c_str() + sp + 2);
    return flaskSpeciesName(w);
}

// J-12: an animal's name (the explorer's, else a friend's; "" none) and its marks as the data sheet says them (GONE over DYING, then CHANGED)
std::string Game::catalogueAnimalName(const std::string& key) const {
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return upper(it->second);
    it = guide.inbox.find(key);
    return it != guide.inbox.end() ? upper(it->second) : std::string();
}
std::string Game::catalogueAnimalMarks(const std::string& key) const {
    const auto it = guide.animals.find(key);
    const uint32_t mk = it != guide.animals.end() ? it->second.marks : 0;
    std::string s = (mk & AM_GONE) ? "GONE" : ((mk & AM_DYING) ? "DYING" : "");
    if (mk & AM_CHANGED) s += s.empty() ? "CHANGED" : ", CHANGED";
    return s;
}

// a row's words: a world's head (its name or UNKNOWN, its kind; J-12: its made kind), a genesis's; a species' name and its form as seen, on the
// kin page its world where it has room (`world` false: without it); J-12: DIED OUT where a line drove it out, a line found wild SEEDED, an
// animal's name and words, its marks
std::string Game::catalogueLine(const CatalogueRow& R, bool world) {
    if (R.head) {
        if (!R.line.empty()) return R.line;
        const auto it = cat.worlds.find(R.key);
        const std::string kind = it == cat.worlds.end() ? "" : (!it->second.made.empty() ? it->second.made : std::string(KINDS[it->second.kind].name));
        return upper(nameOfKey(R.key)) + (kind.empty() ? "" : " - " + kind);
    }
    if (R.kind == CR_ANIMAL) {
        const CatalogueWorld& W = cat.worlds[worldOfAnimal(R.key)];
        const int i = indexOfAnimal(R.key);
        const std::string words = i >= 0 && i < (int)W.animals.size() ? W.animals[i].words : std::string("ANIMAL");
        const std::string named = catalogueAnimalName(R.key), marks = catalogueAnimalMarks(R.key);
        return (named.empty() ? words : named + " - " + words) + (marks.empty() ? "" : " - " + marks);
    }
    if (R.kind == CR_WILD) {
        const auto it = cat.wilds.find(R.wild);
        return it == cat.wilds.end() ? std::string() : catalogueLineName(it->second.line) + " - " + planWords(it->second.form.plan) + " - SEEDED";
    }
    const CatalogueWorld& W = cat.worlds[worldOfKey(R.key)];
    const MicrobeSheet& seen = catalogueSheet(R.key);
    const int form = formSeen(seen);
    const std::string named = nameOfKey(R.key);
    std::string s = (named == "UNKNOWN" ? std::string("UNNAMED") : upper(named)) + " - " +
                    (form == CF_CELL ? std::string(planWords(W.forms[indexOfKey(R.key)].plan)) : (form == CF_FOSSIL ? "FOSSILS" : (form == CF_SPORE ? "SPORES" : "CYSTS")));
    if ((seen.seen & SEEN_KIN) && lineageOfKey(R.key).made) s += " - MADE";   // before its world: a long line cuts the world first
    if (W.diedOut.count(indexOfKey(R.key))) s += " - DIED OUT";   // J-12
    if (cat.order == 1 && world) s += ", " + upper(nameOfKey(worldOfKey(R.key)));
    return s;
}

// J-12: the LIFE page's seeded worlds, made as it opens (each read again at the guide's clock): the lines that took, the made kinds (each
// name once), the explorer's species seen there that died out, the animals seen gone
void Game::catalogueCountsMake() {
    CatalogueCounts& N = cat.counts;
    N = CatalogueCounts();
    if (guide.seeded.empty()) return;
    std::set<std::string> worlds;
    for (const SeedRecord& r : guide.seeded) worlds.insert(r.body);
    N.worlds = (int)worlds.size(); N.sent = (int)guide.seeded.size();
    LabSystems systems;
    for (const std::string& w : worlds) {
        const CatalogueWorld& W = catalogueWorld(w, systems, false);
        N.took += W.took;
        if (!W.made.empty() && std::find(N.kinds.begin(), N.kinds.end(), W.made) == N.kinds.end()) N.kinds.push_back(W.made);
    }
    for (const auto& kv : guide.microbes) {
        const auto it = worlds.count(worldOfKey(kv.first)) ? cat.worlds.find(worldOfKey(kv.first)) : cat.worlds.end();
        N.diedOut += it != cat.worlds.end() && it->second.diedOut.count(indexOfKey(kv.first));
    }
    for (const auto& kv : guide.animals) N.gone += (kv.second.marks & AM_GONE) != 0;
}

// J-12: whether a row's species is gone from its world (a native a line drove out, an animal the ground showed GONE): drawn dim
bool Game::catalogueGone(const CatalogueRow& R) {
    if (R.head || R.kind == CR_WILD) return false;
    if (R.kind == CR_ANIMAL) { const auto it = guide.animals.find(R.key); return it != guide.animals.end() && (it->second.marks & AM_GONE); }
    const auto it = cat.worlds.find(worldOfKey(R.key));
    return it != cat.worlds.end() && it->second.diedOut.count(indexOfKey(R.key));
}

void Game::renderCatalogue() {
    CatalogueState& C = cat;
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 222);
    if (C.page == 1 && C.sel >= 0 && C.sel < (int)C.rows.size()) { renderCatalogueSheet(); return; }
    drawText(canvas, 6, 4, "CATALOGUE", HUD_AMBER);
    const char* order = C.order == 0 ? "BY WORLD" : "BY KIN";
    drawText(canvas, UW - 6 - textWidth(order), 4, order, HUD_WHITE);
    if (C.rows.empty()) {
        drawTextCentered(canvas, UW / 2, 90, "NO SPECIES SEEN YET", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "ESC CLOSE", HUD_DIM);
        return;
    }
    C.top = clampi(C.top, std::max(0, C.sel - CAT_ROWS + 2), std::max(0, C.sel - 1));   // its head in view above it where it fits
    C.top = clampi(C.top, 0, std::max(0, (int)C.rows.size() - CAT_ROWS));
    for (int r = C.top; r < (int)C.rows.size() && r < C.top + CAT_ROWS; r++) {
        const int y = CAT_Y0 + (r - C.top) * CAT_ROW;
        const CatalogueRow& R = C.rows[r];
        if (R.head) { drawText(canvas, 8, y, fitText(catalogueLine(R), UW - 16).c_str(), R.genesis ? genesisColour(R.genesis) : HUD_AMBER); continue; }
        const bool sel = r == C.sel;
        if (sel) drawText(canvas, 10, y, ">", HUD_AMBER);
        std::string line = catalogueLine(R);
        if (textWidth(line.c_str()) > UW - 24) line = catalogueLine(R, false);   // its world left out before its words are cut
        drawText(canvas, 18, y, fitText(line, UW - 24).c_str(), sel ? HUD_WHITE : (R.lent ? HUD_CYAN : (catalogueGone(R) ? HUD_DIM : HUD_GREEN)));
    }
    drawTextCentered(canvas, UW / 2, UH - 10, C.order == 0 ? "ENTER SHEET  TAB BY KIN  N NAME  ESC CLOSE" : "ENTER SHEET  TAB BY WORLD  N NAME  ESC CLOSE",
                     HUD_DIM);
}

// a species' sheet: its form live on the left (its slide's middle), its lines on the right; J-12: a line found wild as the wild left it (when
// it went down, what the wild changed once the sequencer read it from there), a native's DIED OUT with the line that did it; an animal's alone
void Game::renderCatalogueSheet() {
    CatalogueState& C = cat;
    const CatalogueRow& R = C.rows[C.sel];
    if (R.kind == CR_ANIMAL) { renderCatalogueAnimal(); return; }
    const auto wl = R.kind == CR_WILD ? C.wilds.find(R.wild) : C.wilds.end();
    if (R.kind == CR_WILD && wl == C.wilds.end()) return;
    const CatalogueWild* L = wl != C.wilds.end() ? &wl->second : nullptr;
    const CatalogueWorld& W = C.worlds[L ? L->world : worldOfKey(R.key)];
    const int i = indexOfKey(R.key);
    const Morph& form = L ? L->form : W.forms[i];
    const Microbe& sp = L ? L->s : W.species[i];
    const MicrobeSheet& seen = catalogueSheet(R.key);
    const double viewUm = renderCatalogueForm(form, seen, keySeed(L ? R.wild : R.key));
    drawRectRGB(canvas, FORM_X - 1, FORM_Y - 1, FORM_X + FORM_W, FORM_Y + FORM_H, R.lent ? HUD_CYAN : HUD_DIM);
    {   // its scale bar, its way of seeing
        const double bar = barUm(viewUm), widthUm = viewUm * FORM_W / FORM_H;
        const int bx = FORM_X + 6, by = FORM_Y + FORM_H - 8, bw = std::max(2, (int)std::lround(bar / widthUm * FORM_W));
        fillRectRGB(canvas, bx - 1, by - 1, bx + bw + 1, by + 2, rgb(0, 0, 0));
        fillRectRGB(canvas, bx, by, bx + bw, by + 1, rgb(255, 255, 255));
        drawText(canvas, bx + bw + 4, by - 3, bar < 1 ? fmt("%.1f UM", bar).c_str() : fmt("%.0f UM", bar).c_str(), HUD_WHITE);
    }
    const std::vector<int> ways = waysSeen(seen);
    drawText(canvas, FORM_X, FORM_Y + FORM_H + 4, SEEING_NAMES[C.seeing], HUD_AMBER);
    if (ways.size() > 1) drawText(canvas, FORM_X, FORM_Y + FORM_H + 13, fmt("V NEXT OF %d WAYS SEEN", (int)ways.size()).c_str(), HUD_DIM);
    if (R.lent) drawText(canvas, FORM_X, FORM_Y + FORM_H + 22, "LENT BY A FRIEND", HUD_CYAN);
    const std::string world = nameOfKey(worldOfKey(R.key));
    std::vector<std::string> lines = sheetLinesOf(R.key, sp, form, world, seen);
    std::vector<uint32_t> cols(lines.size());
    for (size_t k = 0; k < lines.size(); k++)
        cols[k] = k == 0 ? (R.lent || nameIsForeign(R.key) ? HUD_CYAN : HUD_WHITE) : (k == 1 || lines[k].rfind("MORE IN:", 0) == 0 ? HUD_DIM : HUD_GREEN);
    auto after = [&](size_t at, const std::string& s, uint32_t c) {   // (J-12's lines under its world's)
        at = std::min(at, lines.size());
        lines.insert(lines.begin() + (long)at, s); cols.insert(cols.begin() + (long)at, c);
    };
    if (L) {
        if (!lines.empty()) lines[0] = catalogueLineName(L->line);
        const double ago = std::max(0.0, guide.clock - L->clock);
        after(2, "LET GO ON " + upper(nameOfKey(L->world)) + ", " + (ago < 3600 ? fmt("%.0f MIN AGO", ago / 60) : fmt("%.1f H AGO", ago / 3600)), HUD_GREEN);
        after(3, !L->read ? std::string("NOT YET SEQUENCED FROM THE WILD")
                          : (L->drift.empty() ? std::string("UNCHANGED IN THE WILD")
                                              : fmt("CHANGED IN THE WILD: GENE%s %s", L->drift.find(',') != std::string::npos ? "S" : "", L->drift.c_str())), HUD_GREEN);
    } else {
        const auto d = W.diedOut.find(i);
        if (d != W.diedOut.end()) after(2, std::string("DIED OUT: ") + diedOutWords(d->second.second) + " " + d->second.first, HUD_RED);
    }
    int y = FORM_Y;
    const int x0 = FORM_X + FORM_W + 8, wrap = (UW - 6 - x0) / 6;
    for (size_t k = 0; k < lines.size(); k++)
        for (const std::string& w : wrapText(lines[k], (size_t)wrap)) {
            if (y > UH - 22) break;
            drawText(canvas, x0, y, w.c_str(), cols[k]);
            y += 8;
        }
    drawTextCentered(canvas, UW / 2, UH - 10, "UP/DOWN NEXT  V SEEING  N NAME  ESC LIST", HUD_DIM);
}

// J-12: an animal species' sheet, in words alone (the ground shows its body): its name, its world, its words, its way, what it was seen doing,
// the nearest it came and the most seen at once, its fate as the ground showed it
void Game::renderCatalogueAnimal() {
    const CatalogueRow& R = cat.rows[cat.sel];
    const CatalogueWorld& W = cat.worlds[worldOfAnimal(R.key)];
    const int i = indexOfAnimal(R.key);
    if (i < 0 || i >= (int)W.animals.size()) return;
    const CatalogueAnimal& A = W.animals[i];
    const auto rec = guide.animals.find(R.key);
    const AnimalRecord r = rec != guide.animals.end() ? rec->second : AnimalRecord();
    const std::string named = catalogueAnimalName(R.key), marks = catalogueAnimalMarks(R.key);
    std::vector<std::pair<std::string, uint32_t>> lines;
    lines.push_back({named.empty() ? A.words : named, nameIsForeign(R.key) ? HUD_CYAN : HUD_WHITE});
    lines.push_back({"FROM " + upper(nameOfKey(worldOfAnimal(R.key))), HUD_DIM});
    if (!named.empty()) lines.push_back({A.words, HUD_GREEN});
    lines.push_back({std::string("WAY: ") + ANIMAL_WAY_NAMES[clampi(A.way, 0, WAY_COUNT - 1)] + ", " + SOCIAL_NAMES[clampi(A.social, 0, SOCIAL_COUNT - 1)], HUD_GREEN});
    std::string doing;
    for (int d = 0; d < DO_COUNT; d++) if (r.doing & (1u << d)) doing += (doing.empty() ? "" : ", ") + std::string(DOING_NAMES[d]);
    if (!doing.empty()) lines.push_back({"SEEN: " + doing, HUD_GREEN});
    if (r.most > 0) lines.push_back({fmt("NEAREST %d M, %d SEEN AT ONCE", r.nearest, r.most), HUD_GREEN});
    if (!marks.empty()) lines.push_back({"FATE: " + marks, HUD_RED});
    int y = FORM_Y;
    for (const auto& [s, c] : lines)
        for (const std::string& w : wrapText(s, (size_t)((UW - 12) / 6))) {
            if (y > UH - 22) break;
            drawText(canvas, FORM_X, y, w.c_str(), c);
            y += 9;
        }
    drawTextCentered(canvas, UW / 2, UH - 10, "UP/DOWN NEXT  N NAME  ESC LIST", HUD_DIM);
}

// its form live: one specimen in the way chosen, the view following it, drawn in the catalogue's framebuffer (every bank its way's) and its
// middle put in the box; the slide's height in view (um). Under the electron it is still, as on the microscope's stage (its clock stopped
// under the beam): drawn once for the species and kept (a large cell's mesh takes 35 ms at 4x)
double Game::renderCatalogueForm(const Morph& m, const MicrobeSheet& seen, uint64_t seed) {
    CatalogueState& C = cat;
    if (!C.fb || C.fb->idx.size() != (size_t)FBW * FBH) { C.fb = std::make_unique<Framebuffer>(); C.still = 0; }
    Framebuffer& fb = *C.fb;
    const int form = formSeen(seen);
    const bool still = C.seeing == WS_ELECTRON;
    Slide v; v.seeing = C.seeing; v.t = still ? 0.0 : std::max(0.0, realTime - C.t0); v.seed = seed;
    v.step = FB_SCALE >= 3 ? 2 : 1;   // as the microscope's at the higher scales
    v.turn = 0.6; v.tilt = 0.55; v.stage = 0.35 + 0.1 * std::sin(v.t * 0.5);   // the section's stage turning
    Specimen s; s.m = &m; s.form = form; s.act = AN_LIVE; s.seed = seed; s.angle = 0.3;
    s.still = form != CF_CELL || !(seen.seen & SEEN_MOTION);   // its jiggle alone where its motion was never seen
    double ang;
    specimenPlace(s, v.t, v.cx, v.cy, ang);   // the view follows it
    double viewH = 2.2 * m.viewUm();
    if (form != CF_CELL) {
        double x0, y0, x1, y1;
        specimenBox(s, v.t, x0, y0, x1, y1);
        if (x0 <= x1) viewH = 2.2 * std::max(std::max(v.cx - x0, x1 - v.cx), std::max(v.cy - y0, y1 - v.cy));
    }
    viewH = std::max(viewH, 0.5);
    v.umPerPx = viewH / FORM_H;
    const uint64_t drawn = still ? mix64(seed ^ (uint64_t)FBW) : 0;
    if (!still || C.still != drawn || C.rgb.size() != (size_t)FBW * FBH) {
        slidePalette(fb.pal, C.seeing);
        drawSlide(fb, v, {s});
        fb.mush(2);
        C.rgb.resize((size_t)FBW * FBH);
        fb.toRGB(C.rgb.data(), 1.0);
        C.still = drawn;
    }
    const int S = FB_SCALE, ox = (FBW - FORM_W * S) / 2, oy = (FBH - FORM_H * S) / 2;
    for (int y = 0; y < FORM_H * S; y++)
        std::memcpy(&canvas.px[(size_t)(FORM_Y * S + y) * FBW + FORM_X * S], &C.rgb[(size_t)(oy + y) * FBW + ox], (size_t)FORM_W * S * sizeof(uint32_t));
    return viewH;
}

void Game::cataloguePad(PadMenu& m) const {
    m.title = "CATALOGUE";
    if (cat.page == 1) {
        m.item("NEXT", KEY_DOWN); m.item("PREVIOUS", KEY_UP); m.item("NEXT WAY OF SEEING", KEY_V); m.item("NAME SPECIES", KEY_N);
        m.item("BACK TO LIST", KEY_ESCAPE);
        m.button(PR_B, KEY_ESCAPE, "LIST"); m.button(PR_X, KEY_V, "SEEING"); m.button(PR_Y, KEY_N, "NAME");
        return;
    }
    if (!cat.rows.empty()) { m.item("SHEET", KEY_ENTER); m.item("NAME SPECIES", KEY_N); }
    m.item(cat.order == 0 ? "BY KIN" : "BY WORLD", KEY_TAB);
    m.item("CLOSE", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "SHEET"); m.button(PR_B, KEY_ESCAPE, "CLOSE"); m.button(PR_SELECT, KEY_TAB, cat.order == 0 ? "BY KIN" : "BY WORLD");
}
