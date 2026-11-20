// Z-06: the sequencer (R-425; docs/ideas/ITERATION-1.5.0.md Z-06, IDEAS-small-and-spacious.md 1.8 and 1.9; lab_state.h, life/sequences.h,
// peoples/made.h), the laboratory's third device: a screen over the room, E at it on the bow bench. Enter chooses a species seen in a cold
// store's vial or growing in a flask (never a fossil's stone or a protocell's chemistry) and puts it on: its genome read in SEQUENCE_S on the
// lab's clock, shown as it reads in its genesis's alphabet (a chromatogram's peaks over the letters, its helix turning its own way), the
// screen closed or not; Backspace takes it off; Tab the tree's screen. Its result as the clock reaches it: the guide's (the sheet's kin), said
// and logged (KIN: its nearest kin read and their common ancestor, or a life whose beginning was not met before), a made species' watermark
// to the decoder (`guide.watermarks`)
#include "game.h"
#include "ui.h"
#include "peoples/made.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <sstream>

namespace {
uint32_t seqColour(float k = 1) {
    const RGB& c = Game::cabinDevices().row[FACE_SEQUENCER].col;
    return rgb((int)clampd(c.r * k * 255, 0, 255), (int)clampd(c.g * k * 255, 0, 255), (int)clampd(c.b * k * 255, 0, 255));
}
uint32_t letterColour(char c, float k = 1) { return letterDye(c, k); }   // (ui.h's, the gene editor's too)
const char* const LETTER_WORDS[9] = {"", "", "", "", "FOUR", "", "SIX", "", "EIGHT"};
std::string lettersWords(double n) {
    if (n >= 1e9) return fmt("%.1f BILLION", n / 1e9);
    if (n >= 1e6) return fmt("%.1f MILLION", n / 1e6);
    return thousands(std::round(n));
}
constexpr int SEQ_ROW = 40, SEQ_ROWS = 6;   // the reading's letters: a row of forty, six rows shown
}   // namespace

// a species' key (`<body key>/M<index>`) read back into its world and index
bool Game::speciesOfKey(const std::string& key, FlaskSpecies& w) const {
    const size_t m = key.rfind("/M");
    if (m == std::string::npos || !Guide::parseStarKey(key, w.sx, w.sy, w.sz)) return false;
    const size_t sl = key.find('/');
    w.body = sl == std::string::npos || sl >= m ? -1 : atoi(key.c_str() + sl + 1);
    w.species = atoi(key.c_str() + m + 2);
    w.strain = w.strainIn = 0;
    return w.body >= 0 && w.species >= 0;
}

// a sequenced species' lineage (its world's system drawn once, the made ones marked): kept while the game runs; B-443: its form with it
const Lineage& Game::lineageOfKey(const std::string& key) {
    auto it = tree.known.find(key);
    if (it != tree.known.end()) return it->second;
    kinDraw(key);
    return tree.known[key];
}
const Morph* Game::formOfKey(const std::string& key) {
    auto it = tree.forms.find(key);
    if (it == tree.forms.end()) { kinDraw(key); it = tree.forms.find(key); }
    return it == tree.forms.end() ? nullptr : &it->second;
}
void Game::kinDraw(const std::string& key) {
    Lineage L;
    FlaskSpecies w;
    if (speciesOfKey(key, w)) {
        LabSystems systems;
        const StarSystem* S = systems.get(sys, w.sx, w.sy, w.sz);
        if (S && w.body < (int)S->bodies.size()) {
            L = lineageOf(*S, w.body, w.species);
            L.made = madeSpeciesOf(*S, w.body).species == w.species;
            const std::vector<Morph> forms = morphsOf(*S, w.body);
            if (w.species < (int)forms.size()) tree.forms[key] = forms[w.species];
            tree.kinds[key.substr(0, key.rfind("/M"))] = S->bodies[w.body].type;   // Y-01: its world's page's name
        }
    }
    tree.known.emplace(key, L);   // (a lineage the sequencer set kept)
}

// the chooser's rows: each species seen in a cold store's vial (never a fossil or a protocell: no genome to read), then each flask's, then (R-433)
// the collection's (a strain read as its species); R-432: in folders, a world's, the flasks', the collection's
void Game::sequencerChoices() {
    SequencerState& Q = lab.seq;
    Q.choices.clear(); Q.choiceSel = 0; Q.choiceTop = 0;
    Q.pages = ChooserPage();
    std::vector<std::string> keys;
    LabSystems systems;
    std::set<std::string> listed, cultured;   // each species once in the vials' tab, once in each folder of the cultures' (R-441)
    std::vector<std::string> looks, rests, traits;   // B-442: each row's look, the rest of its words, its form's colour and motion
    auto add = [&](const SequencerChoice& c, const std::string& key, const std::string& look, const std::string& rest, const std::string& trait) {
        Q.choices.push_back(c); keys.push_back(key); looks.push_back(look); rests.push_back(rest); traits.push_back(trait);
    };
    for (const Vial& V : coldStore) {
        const StarSystem* S = systems.get(sys, V.sx, V.sy, V.sz);
        if (!S || V.body < 0 || V.body >= (int)S->bodies.size()) continue;
        const Biota& B = lifeOf(*S)->bodies[V.body];
        std::vector<Morph> forms;
        for (const Held& H : sampleOf(B, V.hab).held) {
            if (H.state == SS_FOSSIL || H.state == SS_PROTO) continue;
            const std::string key = Guide::bodyKey(V.sx, V.sy, V.sz, V.body) + "/M" + std::to_string(H.species);
            const auto it = guide.microbes.find(key);
            if (it == guide.microbes.end() || !(it->second.seen & SEEN_FORM) || !listed.insert(key).second) continue;   // only what was seen of it, each once
            if (forms.empty()) forms = morphsOf(*S, V.body);
            SequencerChoice c; c.who.sx = V.sx; c.who.sy = V.sy; c.who.sz = V.sz; c.who.body = V.body; c.who.species = H.species;
            add(c, Guide::bodyKey(V.sx, V.sy, V.sz, V.body), flaskSpeciesWords(c.who, forms[H.species], true), " - FROM " + V.name + ((it->second.seen & SEEN_KIN) ? " (READ)" : ""),
                formTraits(forms[H.species]));   // B-442
        }
    }
    flaskMadeAll();   // (their forms)
    for (int f = 0; f < INCUBATOR_FLASKS; f++)
        for (int k = 0; k < lab.flasks[f].n && k < (int)lab.flasks[f].forms.size(); k++) {
            FlaskSpecies w = lab.flasks[f].who[k]; w.strainIn = w.strain;   // Y-02: its line as it is (a strain's, an edited line's genes)
            const std::string key = flaskSpeciesKey(w);
            if (!cultured.insert("FLASKS|" + flaskStrainKey(w)).second) continue;
            const auto it = guide.microbes.find(key);
            SequencerChoice c; c.who = w;
            add(c, "FLASKS", fmt("FLASK %d: ", f + 1) + flaskSpeciesWords(w, lab.flasks[f].forms[k], false),
                it != guide.microbes.end() && (it->second.seen & SEEN_KIN) ? " (READ)" : "", "");
        }
    for (const LabFolder& F : cultureFolders())   // R-433: the collection, its folders in their order (R-440: a pair's two each)
        for (int k : F.id >= 0 ? F.rows : std::vector<int>())
            for (int one = 0; one < (lab.kept[k].paired ? 2 : 1); one++) {
                FlaskSpecies w = one == 1 ? lab.kept[k].partner : lab.kept[k].who; w.strainIn = w.strain;
                const std::string key = flaskSpeciesKey(w);
                if (!cultured.insert(keptFolderKey(k) + "|" + flaskStrainKey(w)).second) continue;
                const auto it = guide.microbes.find(key);
                SequencerChoice c; c.who = w;
                std::string trait;
                const std::string look = keptRow(k, &trait, one);
                add(c, keptFolderKey(k), look, it != guide.microbes.end() && (it->second.seen & SEEN_KIN) ? " (READ)" : "", trait);
            }
    apartRows(looks, rests, keys, traits);
    for (size_t i = 0; i < Q.choices.size(); i++) Q.choices[i].line = looks[i] + rests[i];
    Q.pages.folders = chooserFolders(keys);
    chooserTabs(Q.pages, Q.lastTab);   // R-441: the vials, the cultures; the tab last shown
}

// what is on it made again (not saved): its lineage, its genome's length, its name and world, a made one's watermark (its letters drawn)
void Game::sequencerMake() {
    SequencerState& Q = lab.seq;
    Q.made = true; Q.artificial = false; Q.mark.clear(); Q.line = Lineage(); Q.letters = 0;
    if (!Q.on) return;
    LabSystems systems;
    const StarSystem* S = systems.get(sys, Q.who.sx, Q.who.sy, Q.who.sz);
    if (!S || Q.who.body < 0 || Q.who.body >= (int)S->bodies.size()) { Q.on = false; return; }   // its world no longer there
    const Biota& B = lifeOf(*S)->bodies[Q.who.body];
    if (Q.who.species < 0 || Q.who.species >= (int)B.species.size()) { Q.on = false; return; }
    Q.species = B.species[Q.who.species];
    Q.line = lineageOf(*S, Q.who.body, Q.who.species);
    Q.letters = genomeLetters(Q.species);
    Q.world = upper(nameOfKey(Guide::bodyKey(Q.who.sx, Q.who.sy, Q.who.sz, Q.who.body)));
    const MadeSpecies m = madeSpeciesOf(*S, Q.who.body);
    if (m.species == Q.who.species) {
        Q.artificial = true; Q.line.made = true;
        const Body& b = S->bodies[Q.who.body];
        Shard w;
        if (madeWatermarkOf(*S, Q.who.body, loreQuick(*S, b, BodyGen::make(b), m.people), m, w)) Q.mark = w.text;
    }
    const std::string key = flaskSpeciesKey(Q.who);
    tree.known[key] = Q.line;
    const std::vector<Morph> forms = morphsOf(*S, Q.who.body);
    if (Q.who.species < (int)forms.size()) tree.forms[key] = forms[Q.who.species];
    tree.kinds[key.substr(0, key.rfind("/M"))] = S->bodies[Q.who.body].type;
    Q.name = treeWords(key);   // B-443: its words on the tree, a look where it has no name
    if (Q.who.strain > 0) {    // Y-02: a line's name, else its species' words and its number
        const std::string named = nameOfKey(flaskStrainKey(Q.who));
        Q.name = named == "UNKNOWN" ? Q.name + " " + lineTag(Q.who) : upper(named);
    }
    if (Q.told) sequencerResult();   // a reading done before a load: its lines again
}

double Game::sequencerRead() const {
    const SequencerState& Q = lab.seq;
    return Q.on ? clampd((lab.clock - Q.t0) / SEQUENCE_S, 0, 1) : 0;
}

bool Game::sequencerPut(const FlaskSpecies& w) {
    SequencerState& Q = lab.seq;
    Q.on = true; Q.who = w; Q.who.strainIn = Q.who.strain; Q.t0 = lab.clock; Q.told = false; Q.result.clear();   // Y-02: a line read as it is
    sequencerMake();
    if (!Q.on) { refuse("ITS WORLD IS NO LONGER THERE"); return false; }
    if (!Q.line.valid) { Q.on = false; refuse("NO GENOME TO READ IN IT", 3); return false; }
    status(fitText(fmt("READING %s: ABOUT %.0f SECONDS", Q.name.c_str(), SEQUENCE_S), UW - 12), 4);
    audio.beep = BEEP_UI;
    return true;
}

// what it read: its genome, its hand, its nearest kin among those read (the ship's own among them) and their common ancestor, or the geneses
// met; a made one's watermark (the result's lines, made again after a load); the log's line
std::string Game::sequencerResult() {
    SequencerState& Q = lab.seq;
    const std::string key = flaskSpeciesKey(Q.who);
    std::string kin, kinWorld; double nearest = -1;
    std::set<uint64_t> geneses = {HOME_GENESIS_ID};
    auto consider = [&](const Lineage& L, const std::string& name, const std::string& world) {
        if (!L.valid) return;
        geneses.insert(L.genesis.id);
        const double ca = commonAncestorGyr(Q.line, L);
        if (ca >= 0 && (nearest < 0 || ca < nearest)) { nearest = ca; kin = name; kinWorld = world; }
    };
    std::map<std::string, std::string> words;   // B-443: each species in its words on the tree (its look where it has no name)
    for (const TreeLeaf& L : treeLeaves(key)) words[L.key] = L.name;
    if (words.count(key)) Q.name = words[key];
    for (int k = 0; k < SHIP_SPECIES; k++) consider(shipLineage(k), "THE SHIP'S OWN", "");
    for (const auto& kv : guide.microbes) {
        if (!(kv.second.seen & SEEN_KIN) || kv.first == key) continue;
        FlaskSpecies w;
        if (!speciesOfKey(kv.first, w)) continue;
        consider(lineageOfKey(kv.first), words.count(kv.first) ? words[kv.first] : flaskSpeciesName(w), upper(nameOfKey(kv.first.substr(0, kv.first.rfind("/M")))));
    }
    geneses.insert(Q.line.genesis.id);
    Q.result.clear();
    Q.result.push_back(fmt("ITS GENOME: %s LETTERS OF %s (%s)", lettersWords(Q.letters).c_str(), LETTER_WORDS[std::min(8, Q.line.genesis.letters)], alphabetOf(Q.line.genesis.letters)));
    Q.result.push_back(Q.line.genesis.mirrored ? "ITS HELIX TURNS LEFT: A MIRRORED LIFE" : "ITS HELIX TURNS RIGHT, AS OURS DOES");
    std::string line;
    if (nearest < 0) {
        Q.result.push_back("NO COMMON ANCESTOR WITH ANY LIFE READ");
        Q.result.push_back(fmt("LIFE BEGAN AT LEAST %d TIMES", (int)geneses.size()));
        line = fmt("KIN: %s OF %s, NO COMMON ANCESTOR WITH ANY LIFE READ: LIFE BEGAN AT LEAST %d TIMES", Q.name.c_str(), Q.world.c_str(), (int)geneses.size());
    } else {
        const std::string who = kinWorld.empty() ? kin : kin + " OF " + kinWorld;
        Q.result.push_back(fitHead("ITS NEAREST KIN READ: " + kin, kinWorld.empty() ? "" : " OF " + kinWorld, UW - 12));   // B-443: its world left out first
        Q.result.push_back("A COMMON ANCESTOR " + yearsWords(std::max(nearest * 1e9, 1.0)) + " AGO");
        line = fmt("KIN: %s OF %s AND %s, A COMMON ANCESTOR %s AGO", Q.name.c_str(), Q.world.c_str(), who.c_str(), yearsWords(std::max(nearest * 1e9, 1.0)).c_str());
    }
    if (Q.artificial) Q.result.push_back("TOO REGULAR TO HAVE EVOLVED: A WATERMARK IN IT FOR THE DECODER");
    if (Q.who.strain > 0 && lineEdited(flaskStrainKey(Q.who))) {   // Y-02: an edited line: what its genes differ in from its parent's
        Q.result.push_back(fitText("EDITED: " + lineMoveWords(flaskStrainKey(Q.who)), UW - 12));
        line += " (" + lineTag(Q.who) + ", EDITED: " + lineMoveWords(flaskStrainKey(Q.who)) + ")";
    } else if (Q.who.strain > 0) {   // a strain: the one gene its move was in
        uint64_t genesis = 0;
        const int p = strainGene(Q.who, genesis);
        if (p >= 0) {
            const std::string words = geneWords(genesis, p);
            Q.result.push_back(fitText(fmt("GENE %d DIFFERS FROM ITS PARENT'S", p + 1) + (words == "NOT KNOWN" || words == "NOTHING SEEN" ? "" : ": " + words), UW - 12));
        }
    }
    return line;
}

// the reading's end, as the lab's clock reaches it (`updateLab`, the screen open or not): its sheet's kin, the tree's place said and logged,
// a made one's watermark to the decoder
void Game::sequencerTick() {
    SequencerState& Q = lab.seq;
    if (!Q.on) return;
    if (!Q.made) sequencerMake();
    if (!Q.on || Q.told || lab.clock < Q.t0 + SEQUENCE_S) return;
    Q.told = true;
    const std::string key = flaskSpeciesKey(Q.who);
    guide.microbes[key].seen |= SEEN_KIN;
    if (Q.artificial) guide.watermarks.insert(key);
    const std::string learned = strainReadLearn();   // Y-02: a strain's gene, the one its move was in
    const std::string line = sequencerResult();
    status(fitText(Q.artificial ? std::string("SEQUENCED: MADE, A WATERMARK FOR THE DECODER") : "SEQUENCED: " + Q.result[2], UW - 12), 6);
    logEvent("KIN", line);
    if (!learned.empty()) { statusNext = fitText(learned, UW - 12); statusNextSecs = 6; guide.save(guidePath); }
    audio.beep = BEEP_LOCK;
}

void Game::sequencerEnter() {
    lab.seq.choosing = false;
    if (lab.seq.on && !lab.seq.made) sequencerMake();
    audio.beep = BEEP_UI;
}

void Game::updateSequencer(const Input& in, double dt, double realDt) {
    SequencerState& Q = lab.seq;
    if (Q.choosing) {   // R-432: its folders, then the rows of the one open
        const int r = Q.choices.empty() ? -2 : chooserKeys(in, Q.pages, Q.choiceSel);
        if (Q.pages.tabbed) Q.lastTab = Q.pages.tab;
        if (r == -2) { Q.choosing = false; return; }
        if (r >= 0 && r < (int)Q.choices.size()) { Q.choosing = false; sequencerPut(Q.choices[r].who); }
        return;
    }
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    if (in.wasPressed(KEY_TAB)) { openScreen(GameState::TREE); return; }
    if (enterKey(in)) {
        if (Q.on && !Q.told) { refuse("IT IS STILL READING", 2); return; }
        sequencerChoices();
        if (Q.choices.empty()) { refuse("NOTHING SEEN TO READ: THE MICROSCOPE FIRST", 3); return; }
        Q.choosing = true; audio.beep = BEEP_UI;
        return;
    }
    if ((in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE)) && Q.on) { const int tab = Q.lastTab; Q = SequencerState(); Q.lastTab = tab; status("THE SEQUENCER IS EMPTY", 2); audio.beep = BEEP_UI; }
}

void Game::renderSequencer() {
    const SequencerState& Q = lab.seq;
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 218);
    drawText(canvas, 6, 4, "THE SEQUENCER", seqColour());
    drawText(canvas, UW - 6 - textWidth("TAB THE TREE"), 4, "TAB THE TREE", HUD_DIM);
    if (!Q.on) {
        drawTextCentered(canvas, UW / 2, UH / 2 - 12, "EMPTY", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH / 2, "ENTER CHOOSES A SPECIES SEEN IN A VIAL OR A FLASK", HUD_DIM);
    } else {
        const double read = sequencerRead();
        const char* abc = alphabetOf(Q.line.genesis.letters);
        drawText(canvas, 6, 16, fitHead(Q.name, " OF " + (Q.world == "UNKNOWN" ? std::string("AN UNNAMED WORLD") : Q.world), UW - 70).c_str(), HUD_WHITE);
        // the progress and the letters read
        const int bx0 = 6, by0 = 28, bw = UW - 70;
        drawRectRGB(canvas, bx0, by0, bx0 + bw, by0 + 5, seqColour(0.4f));
        fillRectRGB(canvas, bx0 + 1, by0 + 1, bx0 + 1 + (int)std::lround((bw - 2) * read), by0 + 4, seqColour());
        drawText(canvas, bx0, by0 + 9, fmt("READ %s OF %s LETTERS", lettersWords(Q.letters * read).c_str(), lettersWords(Q.letters).c_str()).c_str(), HUD_DIM);
        // the alphabet's legend
        int lx = bx0;
        drawText(canvas, lx, by0 + 19, "ITS LETTERS:", HUD_DIM); lx += textWidth("ITS LETTERS: ");
        for (const char* p = abc; *p; p++) { const char s[2] = {*p, 0}; drawText(canvas, lx, by0 + 19, s, letterColour(*p)); lx += 9; }
        // the letters as they are read: the marker's, a made one's watermark between its rows (too regular), the newest row's chromatogram
        const int shown = (int)std::floor(read * MARKER_LETTERS * 2), rows = std::min(SEQ_ROWS, shown / SEQ_ROW + 1);
        const int firstRow = std::max(0, shown / SEQ_ROW + 1 - SEQ_ROWS);
        const int gx0 = 6, gy0 = 88, cw = 6;
        for (int r = 0; r < rows; r++) {
            const int row = firstRow + r, from = row * SEQ_ROW;
            const bool mark = Q.artificial && !Q.mark.empty() && (row % 3) == 1;
            const std::string letters = mark ? watermarkLetters(Q.mark, Q.line.genesis.letters, from, SEQ_ROW) : markerOf(Q.line, from, SEQ_ROW);
            const int upto = std::min(SEQ_ROW, shown - from);
            for (int i = 0; i < upto; i++) { const char s[2] = {letters[i], 0}; drawText(canvas, gx0 + i * cw, gy0 + r * 9, s, letterColour(letters[i], mark ? 0.75f : 1.0f)); }
            if (r == rows - 1 && read < 1) {   // the newest row's trace over the block: a peak a letter in its dye, filled
                const int base = gy0 - 5;
                drawLineRGB(canvas, gx0, base + 1, gx0 + SEQ_ROW * cw, base + 1, rgb(60, 50, 56));
                for (int i = 0; i < upto; i++) {
                    const uint64_t h = mix64(Q.species.seed ^ ((uint64_t)(from + i) * 0x9E3779B97F4A7C15ULL));
                    const int peak = 9 + (int)(h % 14);
                    for (int dx = -2; dx <= 2; dx++) {
                        const int top = base - (int)std::lround(peak * std::exp(-dx * dx / 2.2));
                        drawLineRGB(canvas, gx0 + i * cw + 2 + dx, top, gx0 + i * cw + 2 + dx, base, letterColour(letters[i], dx == 0 ? 1.0f : 0.6f));
                    }
                }
            }
        }
        // its helix, turning its own way (a mirrored life's the other)
        {
            const int hx = UW - 32, hy0 = 22, hh = 120;
            const double turn = (Q.line.genesis.mirrored ? -1 : 1) * realTime * 1.4;
            for (int y = 0; y < hh; y++) {
                const double a = y * 0.16 * (Q.line.genesis.mirrored ? -1 : 1) + turn;
                for (int s = 0; s < 2; s++) {
                    const double ph = a + s * PI, z = std::cos(ph);
                    const int x = hx + (int)std::lround(12 * std::sin(ph));
                    fillRectRGB(canvas, x, hy0 + y, x, hy0 + y, seqColour((float)(0.45 + 0.55 * (z * 0.5 + 0.5))));
                }
                if (y % 7 == 0) {   // a rung, its pair's two letters
                    const int xa = hx + (int)std::lround(12 * std::sin(a)), xb = hx + (int)std::lround(12 * std::sin(a + PI));
                    drawLineRGB(canvas, std::min(xa, xb), hy0 + y, std::max(xa, xb), hy0 + y, letterColour(abc[(y / 7) % std::strlen(abc)], 0.5f));
                }
            }
        }
        // the result, once read
        int y = gy0 + SEQ_ROWS * 9 + 6;
        if (Q.told) for (const std::string& l : Q.result) { if (y < UH - 22) drawText(canvas, 6, y, fitText(l, UW - 12).c_str(), l.compare(0, 3, "TOO") == 0 ? HUD_AMBER : HUD_GREEN); y += 9; }
        else drawText(canvas, 6, y, fmt("READING: %.0f SECONDS TO GO", std::ceil(std::max(0.0, Q.t0 + SEQUENCE_S - lab.clock))).c_str(), HUD_DIM);
    }
    drawTextCentered(canvas, UW / 2, UH - 12, "ENTER A SPECIES  BACKSPACE TAKES IT OFF  TAB THE TREE", HUD_DIM);
    if (Q.choosing) {   // the chooser over it (R-432: its folders, then the open one's rows)
        const int rowsN = 14, x0 = 20, y0 = 30;
        drawPanel(canvas, x0 - 6, y0 - 14, UW - x0 + 6, y0 + rowsN * 9 + 18, rgb(10, 8, 12), 235, seqColour(0.6f));
        const std::string head = Q.pages.open && Q.pages.folder < (int)Q.pages.folders.size() ? "WHAT TO READ: " + Q.pages.folders[Q.pages.folder].name : std::string("WHAT TO READ");
        drawText(canvas, x0, y0 - 10, fitText(head, UW - 2 * x0).c_str(), seqColour());
        std::vector<std::string> lines;
        for (const SequencerChoice& c : Q.choices) lines.push_back(c.line);
        drawChooser(canvas, x0 - 6, y0, UW - x0, y0 + rowsN * 9 + 14, lab.seq.pages, lab.seq.choiceSel, lab.seq.choiceTop, lines, seqColour());
    }
}

void Game::sequencerPad(PadMenu& m) const {
    m.title = "THE SEQUENCER";
    if (lab.seq.choosing) {
        m.item(lab.seq.pages.open ? "READ IT" : "OPEN THE FOLDER", KEY_ENTER);
        if (lab.seq.pages.tabbed) m.item(lab.seq.pages.tab == 0 ? "THE CULTURES" : "THE VIALS", KEY_TAB);   // R-441
        m.item("BACK", KEY_ESCAPE);
        return;
    }
    m.item("A SPECIES TO READ", KEY_ENTER); m.item("TAKE IT OFF", KEY_BACKSPACE); m.item("THE TREE OF KIN", KEY_TAB); m.item("BACK TO THE LABORATORY", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "READ"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_SELECT, KEY_TAB, "TREE");
}

// ---- the save's line: what is on it, when its reading began, whether its result was told -------------------------------------------------

void Game::sequencerSave(std::string& lines) const {
    const SequencerState& Q = lab.seq;
    if (!Q.on) return;
    std::ostringstream f;
    f.precision(17);
    f << "seq " << Q.who.sx << " " << Q.who.sy << " " << Q.who.sz << " " << Q.who.body << " " << Q.who.species << " " << Q.t0 << " " << (Q.told ? 1 : 0);
    if (Q.who.strain > 0) f << " " << Q.who.strain;   // Y-02: a line read
    f << "\n";
    lines += f.str();
}

bool Game::sequencerLoad(const std::string& text) {
    std::istringstream is(text);
    std::string key;
    is >> key;
    if (key != "seq") return false;
    SequencerState Q; int told = 0;
    is >> Q.who.sx >> Q.who.sy >> Q.who.sz >> Q.who.body >> Q.who.species >> Q.t0 >> told;
    if (is.fail()) return true;
    int strain = 0;
    if (is >> strain && strain > 0) Q.who.strain = Q.who.strainIn = strain;
    Q.on = true; Q.told = told != 0; Q.made = false;
    lab.seq = Q;
    return true;
}

void Game::sequencerReset() { lab.seq = SequencerState(); }
