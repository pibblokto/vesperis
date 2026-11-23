// Z-06: the tree of kin's screen (R-425; docs/ideas/ITERATION-1.5.0.md Z-06, IDEAS-small-and-spacious.md 1.4 and 1.8; lab_state.h,
// life/sequences.h), over the room: E at its screen on the laboratory's port wall, or Tab from the sequencer (on the page of what it read). Y-01
// (R-444; IDEAS-genes.md section 2): its pages, Left and Right, and no tree of everything: the ship's own (home's life), each world with a
// species read, each of the collection's folders holding a culture. A page's species by kinship: a genesis a clade (headed by its letters, its
// hand and when it began), its leaves drawn as the tree is (each two neighbours' common ancestor) on the page's own years; the lab's lines (the
// strains) as twigs at today's edge under their parents, on their world's page as they arise, Backspace taking one off and A putting it back
// (the guide's `treeoff`); a folder's page what it holds. Up and Down choose a leaf (its nearest kin read anywhere and their common ancestor
// told), Enter on a kin's world sets the remote target to its star; Tab the sequencer
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <set>

namespace {
uint32_t treeColour(float k = 1) {
    const RGB& c = Game::cabinDevices().row[FACE_TREE].col;
    return rgb((int)clampd(c.r * k * 255, 0, 255), (int)clampd(c.g * k * 255, 0, 255), (int)clampd(c.b * k * 255, 0, 255));
}
uint32_t lineColour() { return rgb(170, 205, 175); }   // Y-01: a lab's line, paler than the wild leaves' green
const char* const SHIP_WHERE[SHIP_SPECIES] = {"THE GARDEN'S SOIL", "THE EXPLORER'S SKIN", "THE GARDEN POND"};
constexpr int TREE_X0 = 6, TREE_W = 76, TREE_Y0 = 22, TREE_ROW = 9, TREE_ROWS = 12, TWIG_DX = 7;
std::string yearsShort(double gyr) { std::string w = yearsWords(std::max(1.0, gyr * 1e9)); return w.compare(0, 6, "ABOUT ") == 0 ? w.substr(6) : w; }
std::string leafTag(const TreeLeaf& L) { return L.lab ? std::string() : (L.line.made ? " - MADE" : (L.drifter ? " - A DRIFTER'S" : "")); }
// a key's parts: its world's (`<body key>`), its species' (`<body key>/M<index>`), its strain's number (0 the species)
std::string pageWorldOf(const std::string& key) { const size_t m = key.rfind("/M"); return m == std::string::npos ? key : key.substr(0, m); }
std::string speciesKeyOf(const std::string& key) {
    const size_t m = key.rfind("/M"), s = key.rfind("/S");
    return m == std::string::npos || s == std::string::npos || s < m ? key : key.substr(0, s);
}
int strainOfKey(const std::string& key) {
    const size_t m = key.rfind("/M"), s = key.rfind("/S");
    return m == std::string::npos || s == std::string::npos || s < m ? 0 : atoi(key.c_str() + s + 2);
}
// what a page is, kept across its pages made again (a folder's or a world's page coming or going)
std::string pageId(const TreePage& P) { return P.kind == TP_SHIP ? std::string("SHIP") : (P.kind == TP_WORLD ? "W" + P.key : "F" + std::to_string(P.folder)); }
}   // namespace

// a genesis's colour: home's the screen's own, every other its id's of six (Z-07: the catalogue's kin heads too)
uint32_t Game::genesisColour(uint64_t id, float k) {
    if (id == HOME_GENESIS_ID) return treeColour(k);
    static const RGB C[6] = {RGB(1.0f, 0.6f, 0.3f), RGB(0.45f, 0.75f, 1.0f), RGB(1.0f, 0.5f, 0.8f), RGB(0.95f, 0.9f, 0.4f), RGB(0.6f, 1.0f, 0.85f), RGB(0.8f, 0.6f, 1.0f)};
    const RGB& c = C[mix64(id) % 6];
    return rgb((int)clampd(c.r * k * 255, 0, 255), (int)clampd(c.g * k * 255, 0, 255), (int)clampd(c.b * k * 255, 0, 255));
}
// a genesis's head: its letters, its hand, when it began
std::string Game::genesisWords(const Genesis& g) { return fmt("%s, %s, %s", alphabetOf(g.letters), g.mirrored ? "MIRRORED" : "OUR HAND", yearsShort(g.ageGyr).c_str()); }

// every species sequenced (the guide's kin; `with` too, one being read) and the ship's own, in the tree's order. B-443: a species with no name is
// its look and size (its form kept with its lineage), two of a world told apart by their colour and motion, then a number (B-442's `apartRows`)
std::vector<TreeLeaf> Game::treeLeaves(const std::string& with) {
    std::vector<TreeLeaf> leaves;
    for (int k = 0; k < SHIP_SPECIES; k++) {
        TreeLeaf L; L.key = "SHIP/" + std::to_string(k); L.line = shipLineage(k); L.name = "THE SHIP'S OWN"; L.world = SHIP_WHERE[k];
        leaves.push_back(L);
    }
    std::vector<std::string> keys;
    for (const auto& kv : guide.microbes) if ((kv.second.seen & SEEN_KIN) || kv.first == with) keys.push_back(kv.first);
    if (!with.empty() && !guide.microbes.count(with)) keys.push_back(with);
    for (const std::string& key : keys) {
        FlaskSpecies w;
        if (!speciesOfKey(key, w)) continue;
        const Lineage& line = lineageOfKey(key);
        if (!line.valid) continue;
        TreeLeaf L; L.key = key; L.line = line; L.name = flaskSpeciesName(w);
        L.world = upper(nameOfKey(key.substr(0, key.rfind("/M"))));
        L.sx = w.sx; L.sy = w.sy; L.sz = w.sz; L.body = w.body;
        L.drifter = line.lx != line.wx || line.ly != line.wy || line.lz != line.wz;   // its line a star it left
        leaves.push_back(L);
    }
    std::stable_sort(leaves.begin(), leaves.end(), [](const TreeLeaf& a, const TreeLeaf& b) { return treeBefore(a.line, b.line); });
    std::vector<std::string> looks, rests, worlds, traits;
    std::vector<int> at;
    for (int i = 0; i < (int)leaves.size(); i++) {
        const TreeLeaf& L = leaves[i];
        FlaskSpecies w;
        const Morph* form = L.body >= 0 && speciesOfKey(L.key, w) ? formOfKey(L.key) : nullptr;
        if (!form) continue;
        looks.push_back(flaskSpeciesWords(w, *form, true)); rests.push_back(", " + L.world + leafTag(L));
        worlds.push_back(L.key.substr(0, L.key.rfind("/M"))); traits.push_back(formTraits(*form));
        at.push_back(i);
    }
    apartRows(looks, rests, worlds, traits);
    for (size_t r = 0; r < at.size(); r++) leaves[at[r]].name = looks[r];
    return leaves;
}
std::string Game::treeWords(const std::string& key) {
    for (const TreeLeaf& L : treeLeaves(key)) if (L.key == key) return L.name;
    FlaskSpecies w;
    return speciesOfKey(key, w) ? flaskSpeciesName(w) : std::string("UNNAMED");
}
// Y-01: a lab's line in the lab's rows' words (a strain's: its name, else its species' and its number)
std::string Game::treeLineWords(const std::string& key) {
    FlaskSpecies w;
    if (!speciesOfKey(speciesKeyOf(key), w)) return key;
    w.strain = w.strainIn = strainOfKey(key);
    const Morph* form = formOfKey(speciesKeyOf(key));
    return form ? flaskSpeciesWords(w, *form, true) : flaskSpeciesName(w);
}
// Y-01: a twig's words under its species (its look said there): the strain's name, else its number, and its move ("S1 - IT GROWS TO 40 C")
std::string Game::treeTwigWords(const std::string& key) {
    const std::string named = nameOfKey(key), who = named == "UNKNOWN" ? lineTagOf(speciesKeyOf(key), strainOfKey(key)) : upper(named);
    const auto it = guide.strains.find(key);
    return it == guide.strains.end() ? who : who + " - " + lineMoveWords(key);   // Y-02: an edited line's its edits
}
// Y-01: a world's page's name: the explorer's, else UNKNOWN and its kind (the catalogue's world heads; KI-482: a made kind's)
std::string Game::treeWorldWords(const std::string& world) {
    const std::string n = upper(nameOfKey(world));
    const auto it = tree.kinds.find(world);
    if (n != "UNKNOWN" || it == tree.kinds.end()) return n;
    LabSystems systems;
    const std::string made = madeKindAt(world, systems);
    return n + " - " + (made.empty() ? std::string(KINDS[it->second].name) : made);
}

// a leaf's line: its name (or look), its world (`world`: not on a world's own page), a made one's or a drifter's tag; B-443: a long look
// before its world; Y-01: a line indented by its depth
std::string Game::treeLeafLine(const TreeLeaf& L, bool world) {
    const int w = UW - (TREE_X0 + TREE_W) - 14 - TWIG_DX * L.depth;
    const std::string tag = leafTag(L), whole = L.name + (world ? ", " + L.world : std::string()) + tag;
    return textWidth(whole.c_str()) <= w ? whole : fitHead(L.name, tag, w);
}

// the tree: every species read and the ship's own, each two neighbours' common ancestor, the geneses counted (the LIFE page's); Y-01: its
// pages (the ship's own, each world with a species read in the tree's order, each folder holding a culture: UNSORTED, then the explorer's)
// and the one shown, the same page again where it still is
void Game::treeBuild() {
    tree.leaves = treeLeaves(); tree.join.clear();
    std::set<uint64_t> geneses;
    for (size_t k = 0; k < tree.leaves.size(); k++) {
        tree.join.push_back(k == 0 ? -1.0 : commonAncestorGyr(tree.leaves[k - 1].line, tree.leaves[k].line));
        geneses.insert(tree.leaves[k].line.genesis.id);
    }
    tree.geneses = (int)geneses.size();
    const std::string was = tree.page >= 0 && tree.page < (int)tree.pages.size() ? pageId(tree.pages[tree.page]) : std::string("SHIP");
    tree.pages.clear();
    TreePage ship; ship.kind = TP_SHIP; ship.name = "THE SHIP";
    tree.pages.push_back(ship);
    std::set<std::string> worlds;
    for (const TreeLeaf& L : tree.leaves) {
        if (L.body < 0) continue;
        TreePage P; P.kind = TP_WORLD; P.key = pageWorldOf(L.key);
        if (!worlds.insert(P.key).second) continue;
        P.name = treeWorldWords(P.key);
        tree.pages.push_back(P);
    }
    std::vector<int> ids = {0};
    for (const CultureFolder& f : lab.folders) ids.push_back(f.id);
    for (int id : ids) {
        bool holds = false;
        for (const KeptCulture& K : lab.kept) holds = holds || K.folder == id;
        if (!holds) continue;
        TreePage P; P.kind = TP_FOLDER; P.folder = id; P.name = "FOLDER " + folderName(id);
        tree.pages.push_back(P);
    }
    tree.page = 0;
    for (size_t p = 0; p < tree.pages.size(); p++) if (pageId(tree.pages[p]) == was) tree.page = (int)p;
    treePage();
}

// Y-01: the page shown: its species in the tree's order, each followed by its lines (a line's lines under it), each species' common ancestor
// with the one before it (-1 a genesis's first; a line's -1, its depth saying what it is), and what is there and not read (a world's species
// seen, a folder's held). A line hangs under the nearest of its ancestors on the page, its species where none is; a folder's strain kept
// without its species stands in the species' place, a leaf of its own
void Game::treePage() {
    tree.shown.clear(); tree.shownJoin.clear(); tree.unread = 0;
    if (tree.pages.empty()) return;
    tree.page = clampi(tree.page, 0, (int)tree.pages.size() - 1);
    const TreePage& P = tree.pages[tree.page];
    std::set<std::string> species, lines;
    if (P.kind == TP_SHIP) {
        for (const TreeLeaf& L : tree.leaves) if (L.body < 0) species.insert(L.key);
    } else if (P.kind == TP_WORLD) {
        for (const TreeLeaf& L : tree.leaves) if (L.body >= 0 && pageWorldOf(L.key) == P.key) species.insert(L.key);
        for (const auto& kv : guide.microbes) tree.unread += pageWorldOf(kv.first) == P.key && !species.count(kv.first);
        for (const auto& kv : guide.strains) if (pageWorldOf(kv.first) == P.key && !guide.treeOff.count(kv.first)) lines.insert(kv.first);
    } else {
        std::set<std::string> read, unread;
        for (const TreeLeaf& L : tree.leaves) if (L.body >= 0) read.insert(L.key);
        for (const KeptCulture& K : lab.kept) {
            if (K.folder != P.folder) continue;
            for (int s = 0; s < (K.paired ? 2 : 1); s++) {
                const FlaskSpecies& w = s ? K.partner : K.who;
                const std::string sk = flaskSpeciesKey(w);
                if (!read.count(sk)) unread.insert(sk);
                else if (w.strain > 0) lines.insert(flaskStrainKey(w));
                else species.insert(sk);
            }
        }
        tree.unread = (int)unread.size();
    }
    for (const TreeLeaf& L : tree.leaves) {
        std::vector<std::string> mine;   // its lines on the page, by their numbers
        for (const std::string& k : lines) if (speciesKeyOf(k) == L.key) mine.push_back(k);
        std::sort(mine.begin(), mine.end(), [](const std::string& a, const std::string& b) { return strainOfKey(a) < strainOfKey(b); });
        const bool here = species.count(L.key) != 0;
        if (!here && mine.empty()) continue;
        if (here) tree.shown.push_back(L);
        auto up = [&](const std::string& k) {   // the nearest of a line's ancestors on the page, else its species
            auto it = guide.strains.find(k);
            int p = it == guide.strains.end() ? 0 : it->second.parent;
            for (int guard = 0; p > 0 && guard < 64; guard++) {
                const std::string pk = L.key + "/S" + std::to_string(p);
                if (std::find(mine.begin(), mine.end(), pk) != mine.end()) return pk;
                it = guide.strains.find(pk);
                p = it == guide.strains.end() ? 0 : it->second.parent;
            }
            return L.key;
        };
        std::function<void(const std::string&, int)> under = [&](const std::string& parent, int depth) {
            for (const std::string& k : mine) {
                if (up(k) != parent) continue;
                TreeLeaf T = L;
                T.key = k; T.lab = true; T.depth = depth; T.parent = depth > 0 ? parent : std::string();
                T.name = depth > 0 ? treeTwigWords(k) : treeLineWords(k);   // a twig under its species, else its look too
                tree.shown.push_back(T);
                under(k, depth + 1);
            }
        };
        under(L.key, here ? 1 : 0);
    }
    const TreeLeaf* last = nullptr;
    for (const TreeLeaf& L : tree.shown) {
        if (L.depth > 0) { tree.shownJoin.push_back(-1); continue; }
        tree.shownJoin.push_back(last ? commonAncestorGyr(last->line, L.line) : -1.0);
        last = &L;
    }
    // Y-03: a gene moved in, a line from its donor's row (a species or a line) to the line's, where both are on the page
    tree.donors.clear();
    for (size_t k = 0; k < tree.shown.size(); k++) {
        const auto it = tree.shown[k].lab ? guide.strains.find(tree.shown[k].key) : guide.strains.end();
        if (it == guide.strains.end() || it->second.what != SM_EDIT) continue;
        for (const GeneEdit& e : geneEditsOf(it->second.edits)) {
            if (e.how != GX_MOVED) continue;
            for (size_t j = 0; j < tree.shown.size(); j++)
                if (j != k && tree.shown[j].key == e.donor && std::find(tree.donors.begin(), tree.donors.end(), std::make_pair((int)j, (int)k)) == tree.donors.end())
                    tree.donors.push_back({(int)j, (int)k});
        }
    }
    tree.sel = clampi(tree.sel, 0, std::max(0, (int)tree.shown.size() - 1));
}

// Y-01: the page's years (Gyr): its oldest common ancestor and a quarter, never past its oldest life's beginning (a twentieth past the
// ancestor at least); a page of leaves a genesis each, the oldest genesis's beginning
double Game::treeYears() const {
    double oldest = 0, began = 0;
    for (size_t k = 0; k < tree.shown.size(); k++) {
        if (tree.shown[k].depth > 0) continue;
        oldest = std::max(oldest, tree.shownJoin[k]);
        began = std::max(began, tree.shown[k].line.genesis.ageGyr);
    }
    return oldest > 0 ? std::min(oldest * 1.25, std::max(began, oldest * 1.05)) : std::max(began, 0.001);
}

// Y-01: a species' world's page, its leaf chosen (the sequencer's Tab: what it read)
void Game::treeShow(const std::string& key) {
    const std::string world = pageWorldOf(key);
    for (size_t p = 0; p < tree.pages.size(); p++)
        if (tree.pages[p].kind == TP_WORLD && tree.pages[p].key == world) { tree.page = (int)p; tree.top = 0; treePage(); break; }
    for (size_t k = 0; k < tree.shown.size(); k++) if (tree.shown[k].key == key) tree.sel = (int)k;
}

void Game::treeEnter() {
    treeBuild();
    tree.picking = false;
    if (state == GameState::SEQUENCER && lab.seq.on && lab.seq.told) treeShow(flaskSpeciesKey(lab.seq.who));
    audio.beep = BEEP_UI;
}

// a leaf's nearest kin among every species read (its index in `leaves`, -1 none) and their common ancestor
int Game::treeNearest(int i, double& gyr) const {
    int best = -1; gyr = -1;
    for (int j = 0; j < (int)tree.leaves.size(); j++) {
        if (j == i) continue;
        const double ca = commonAncestorGyr(tree.leaves[i].line, tree.leaves[j].line);
        if (ca >= 0 && (best < 0 || ca < gyr)) { best = j; gyr = ca; }
    }
    return best;
}

// Y-01: Backspace takes the chosen line off its world's tree (the guide's `treeoff`); a world's own species, the ship's own and a folder's stay
void Game::treeTakeOff() {
    const TreePage& P = tree.pages[tree.page];
    const TreeLeaf& L = tree.shown[tree.sel];
    if (P.kind == TP_FOLDER) { refuse("SORT FOLDERS ON THE MICROSCOPE'S LISTS", 3); return; }
    if (!L.lab) { refuse(P.kind == TP_SHIP ? "THE SHIP'S SPECIES CANNOT BE TAKEN OFF" : "SPECIES CANNOT BE TAKEN OFF - ONLY STRAINS", 3); return; }
    const std::string words = L.name;
    guide.treeOff.insert(L.key); guide.save(guidePath);
    treePage();
    status(fitText(words + " OFF THE TREE - A PUTS IT BACK", UW - 12), 4);
    audio.beep = BEEP_UI;
}
// Y-01: A opens a chooser of the world's lines taken off; Enter puts the chosen back
void Game::treePickOpen() {
    const TreePage& P = tree.pages[tree.page];
    if (P.kind == TP_FOLDER) { refuse("SORT FOLDERS ON THE MICROSCOPE'S LISTS", 3); return; }
    tree.pickKeys.clear();
    if (P.kind == TP_WORLD)
        for (const std::string& k : guide.treeOff) if (pageWorldOf(k) == P.key && guide.strains.count(k)) tree.pickKeys.push_back(k);
    if (tree.pickKeys.empty()) { refuse(P.kind == TP_WORLD ? "NO STRAIN OF THIS WORLD IS OFF THE TREE" : "ALL THE SHIP'S SPECIES ARE ON THE TREE", 3); return; }
    tree.picking = true; tree.pickSel = 0;
    audio.beep = BEEP_UI;
}
void Game::treePickKeys(const Input& in) {
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_A)) { tree.picking = false; return; }
    const int n = (int)tree.pickKeys.size();
    if (n == 0) { tree.picking = false; return; }
    menuStep(in, tree.pickSel, n);
    if (!enterKey(in)) return;
    const std::string k = tree.pickKeys[clampi(tree.pickSel, 0, n - 1)];
    guide.treeOff.erase(k); guide.save(guidePath);
    tree.picking = false;
    treePage();
    for (size_t i = 0; i < tree.shown.size(); i++) if (tree.shown[i].key == k) tree.sel = (int)i;
    status(fitText(treeLineWords(k) + " BACK ON THE TREE", UW - 12), 4);
    audio.beep = BEEP_UI;
}

void Game::updateTree(const Input& in, double dt, double realDt) {
    if (tree.picking) { treePickKeys(in); return; }
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    if (in.wasPressed(KEY_TAB)) { openScreen(GameState::SEQUENCER); return; }
    const int pages = (int)tree.pages.size();
    if (pages > 1 && (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_RIGHT))) {   // Y-01: the next tree or the one before
        tree.page = (tree.page + (in.wasPressed(KEY_RIGHT) ? 1 : pages - 1)) % pages;
        tree.sel = tree.top = 0;
        treePage();
        audio.beep = BEEP_UI;
        return;
    }
    if (pages > 0 && in.wasPressed(KEY_A)) { treePickOpen(); return; }
    const int n = (int)tree.shown.size();
    if (n == 0) return;
    menuStep(in, tree.sel, n);
    if (in.wasPressed(KEY_PAGE_UP)) tree.sel = std::max(0, tree.sel - 10);
    if (in.wasPressed(KEY_PAGE_DOWN)) tree.sel = std::min(n - 1, tree.sel + 10);
    if (in.wasPressed(KEY_BACKSPACE)) { treeTakeOff(); return; }
    if (enterKey(in)) {   // a kin's world: its star the remote target
        const TreeLeaf& L = tree.shown[tree.sel];
        Star st;
        if (L.body < 0) { refuse("LIVES ABOARD - NO STAR", 2); return; }
        if (!placeInSector(L.sx, L.sy, L.sz, st, true)) { refuse("ITS STAR NO LONGER EXISTS", 2); return; }
        setRemoteStar(st);
        status(fitText("REMOTE TARGET: STAR OF " + (L.world == "UNKNOWN" ? std::string("AN UNNAMED WORLD") : L.world), UW - 12), 4);
        audio.beep = BEEP_LOCK;
    }
}

void Game::renderTree() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 218);
    drawText(canvas, 6, 4, "TREE OF KIN", treeColour());
    const int np = (int)tree.pages.size();
    const bool worldPage = np > 0 && tree.pages[tree.page].kind == TP_WORLD;
    if (np > 0) {   // Y-01: the page, its place among them, what is there and not read
        const TreePage& P = tree.pages[tree.page];
        const std::string of = fmt(" >  %d OF %d", tree.page + 1, np);
        const int room = UW - 12 - textWidth("TREE OF KIN") - 12 - textWidth(("< " + of).c_str());
        const std::string head = "< " + fitText(P.name, room) + of;
        drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_WHITE);
        if (tree.unread > 0)
            drawText(canvas, 6, 13, (P.kind == TP_FOLDER ? fmt("%d IN IT NOT YET READ", tree.unread) : fmt("%d MORE SEEN HERE, NOT YET READ", tree.unread)).c_str(), HUD_DIM);
    }
    const int n = (int)tree.shown.size();
    // the rows: a genesis's head before its first leaf, then its leaves, each species' lines under it
    struct Row { int leaf; bool head; };
    std::vector<Row> rows; int selRow = 0;
    for (int k = 0; k < n; k++) {
        if (tree.shown[k].depth == 0 && tree.shownJoin[k] < 0) rows.push_back({k, true});
        if (k == tree.sel) selRow = (int)rows.size();
        rows.push_back({k, false});
    }
    tree.top = clampi(tree.top, std::max(0, selRow - TREE_ROWS + 2), std::max(0, selRow - 1));
    tree.top = clampi(tree.top, 0, std::max(0, (int)rows.size() - TREE_ROWS));
    std::vector<int> yOf(n, -1000);
    for (int r = 0; r < (int)rows.size(); r++) if (!rows[r].head) yOf[rows[r].leaf] = TREE_Y0 + (r - tree.top) * TREE_ROW + 3;
    const int vy0 = TREE_Y0 - 2, vy1 = TREE_Y0 + TREE_ROWS * TREE_ROW;
    const double maxAge = treeYears();
    const int tx = TREE_X0 + TREE_W;
    auto xOf = [&](double gyr) { return tx - (int)std::lround(clampd(gyr, 0, maxAge) / maxAge * TREE_W); };
    auto hline = [&](int xa, int xb, int y, uint32_t c) { if (y >= vy0 && y <= vy1) drawLineRGB(canvas, std::min(xa, xb), y, std::max(xa, xb), y, c); };
    auto vline = [&](int x, int ya, int yb, uint32_t c) {
        const int a = std::max(vy0, std::min(ya, yb)), b = std::min(vy1, std::max(ya, yb));
        if (a <= b) drawLineRGB(canvas, x, a, x, b, c);
    };
    // each genesis's leaves drawn as their tree (its lines are twigs, after): a node at the oldest common ancestor between its two sides
    std::vector<int> idx;
    for (int k = 0; k < n; k++) if (tree.shown[k].depth == 0) idx.push_back(k);
    auto J = [&](int i) { return tree.shownJoin[idx[i]]; };
    struct Sub { int y; double age; };
    std::function<Sub(int, int, uint32_t)> draw = [&](int lo, int hi, uint32_t c) -> Sub {
        if (lo == hi) return {yOf[idx[lo]], 0.0};
        int k = lo + 1;
        for (int j = lo + 2; j <= hi; j++) if (J(j) > J(k)) k = j;
        const Sub a = draw(lo, k - 1, c), b = draw(k, hi, c);
        const double age = J(k);
        hline(xOf(a.age), xOf(age), a.y, c); hline(xOf(b.age), xOf(age), b.y, c);
        vline(xOf(age), a.y, b.y, c);
        return {(a.y + b.y) / 2, age};
    };
    for (int lo = 0; lo < (int)idx.size();) {
        int hi = lo;
        while (hi + 1 < (int)idx.size() && J(hi + 1) >= 0) hi++;
        const Genesis& g = tree.shown[idx[lo]].line.genesis;
        const Sub root = draw(lo, hi, genesisColour(g.id, 0.8f));
        hline(xOf(root.age), xOf(g.ageGyr), root.y, genesisColour(g.id, 0.8f));   // its stem back to its beginning (to the page's edge before it)
        lo = hi + 1;
    }
    // the lab's lines: a dotted twig down from its parent's row
    for (int k = 0; k < n; k++) {
        const TreeLeaf& L = tree.shown[k];
        if (L.depth == 0) continue;
        int pk = -1;
        for (int j = k - 1; j >= 0 && pk < 0; j--) if (tree.shown[j].key == L.parent) pk = j;
        if (pk < 0) continue;
        const int x = tx + 8 + TWIG_DX * (L.depth - 1) + 2;
        for (int y = std::max(vy0, yOf[pk] + 4); y <= std::min(vy1, yOf[k]); y += 2) fillRectRGB(canvas, x, y, x, y, treeColour(0.9f));
        if (k != tree.sel && yOf[k] >= vy0 && yOf[k] <= vy1) for (int x2 = x; x2 <= x + 3; x2 += 2) fillRectRGB(canvas, x2, yOf[k], x2, yOf[k], treeColour(0.9f));   // (the chosen's mark its own)
    }
    // Y-03: a moved gene's line, dotted amber from its donor's row to the line's in the lane before the names (three lanes, by its order)
    for (size_t i = 0; i < tree.donors.size(); i++) {
        const int a = tree.donors[i].first, b = tree.donors[i].second, x = tx + 2 + 2 * (int)(i % 3);
        if (a < 0 || b < 0 || a >= n || b >= n) continue;
        const int ya = std::min(yOf[a], yOf[b]), yb = std::max(yOf[a], yOf[b]);
        for (int y = std::max(vy0, ya); y <= std::min(vy1, yb); y += 2) fillRectRGB(canvas, x, y, x, y, HUD_AMBER);
        for (int k : {a, b}) if (yOf[k] >= vy0 && yOf[k] <= vy1) for (int x2 = x; x2 <= tx + 6; x2 += 2) fillRectRGB(canvas, x2, yOf[k], x2, yOf[k], HUD_AMBER);
    }
    // the leaves' and the heads' lines
    for (int r = tree.top; r < (int)rows.size() && r < tree.top + TREE_ROWS; r++) {
        const int y = TREE_Y0 + (r - tree.top) * TREE_ROW;
        const TreeLeaf& L = tree.shown[rows[r].leaf];
        if (rows[r].head) {
            const Genesis& g = L.line.genesis;
            drawText(canvas, tx + 8, y, fitText(genesisWords(g), UW - tx - 14).c_str(), genesisColour(g.id));
            continue;
        }
        const bool sel = rows[r].leaf == tree.sel;
        const int x = tx + 8 + TWIG_DX * L.depth;
        if (sel) drawText(canvas, x - 6, y, ">", treeColour());
        drawText(canvas, x, y, treeLeafLine(L, !worldPage).c_str(), sel ? HUD_WHITE : (L.lab ? lineColour() : (L.line.made ? HUD_AMBER : HUD_GREEN)));
    }
    // the years' axis: a step of one, two or five a power of ten, at most four marks; in millions under a billion and a half
    if (n > 0) {
        const int y = vy1 + 2;
        drawLineRGB(canvas, TREE_X0, y, tx, y, HUD_DIM);
        const bool billions = maxAge >= 1.5;
        double step = 5.0;
        for (double s : {1e-3, 2e-3, 5e-3, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0}) if (maxAge / s <= 3.2) { step = s; break; }
        for (int b = 0; b * step <= maxAge + 1e-9; b++) {
            const int x = xOf(b * step);
            drawLineRGB(canvas, x, y, x, y + 2, HUD_DIM);
            const std::string label = fmt("%g", b * step * (billions ? 1 : 1000));
            drawText(canvas, std::max(1, x - textWidth(label.c_str()) / 2), y + 4, label.c_str(), HUD_DIM);
        }
        drawText(canvas, tx + 8, y + 4, billions ? "BILLION YEARS AGO" : "MILLION YEARS AGO", HUD_DIM);
    }
    // the leaf chosen: its nearest kin read anywhere and their common ancestor, what it is; a line's species and its move
    if (n > 0) {
        const TreeLeaf& L = tree.shown[tree.sel];
        int y = UH - 50;
        if (L.lab) {
            const auto it = guide.strains.find(L.key);
            const int parent = it == guide.strains.end() ? 0 : it->second.parent;
            std::string of;
            for (const TreeLeaf& S : tree.leaves) if (S.key == speciesKeyOf(L.key)) of = S.name;
            if (parent > 0) of = treeLineWords(speciesKeyOf(L.key) + "/S" + std::to_string(parent));
            const bool edited = it != guide.strains.end() && it->second.what == SM_EDIT;   // Y-02
            drawText(canvas, 6, y, fitText((edited ? "A STRAIN EDITED FROM " : "A STRAIN OF ") + of, UW - 12).c_str(), HUD_WHITE); y += 9;
            if (it != guide.strains.end()) { drawText(canvas, 6, y, fitText(lineMoveWords(L.key), UW - 12).c_str(), HUD_WHITE); y += 9; }
            std::string from;   // Y-03: a gene moved in, its donor's words
            if (edited)
                for (const GeneEdit& e : geneEditsOf(it->second.edits))
                    if (e.how == GX_MOVED && from.empty()) from = fmt("GENE %d FROM ", e.place + 1) + treeLineWords(e.donor);
            drawText(canvas, 6, y, fitText(!from.empty() ? from : (edited ? "MADE ON THE GENE EDITOR" : "GROWN IN THE LAB"), UW - 12).c_str(), HUD_AMBER);
        } else {
            int i = -1;
            for (int j = 0; j < (int)tree.leaves.size() && i < 0; j++) if (tree.leaves[j].key == L.key) i = j;
            double gyr = -1;
            const int k = i < 0 ? -1 : treeNearest(i, gyr);
            std::string kin, when;
            if (k < 0) kin = "NO COMMON ANCESTOR WITH ANY LIFE SEQUENCED";
            else {
                const TreeLeaf& K = tree.leaves[k];
                kin = fitHead("NEAREST KIN: " + K.name, (K.body >= 0 ? " OF " : ", ") + K.world, UW - 12);
                when = gyr < 1e-3 ? "ONE SPECIES, CARRIED APART " + yearsWords(std::max(gyr * 1e9, 1.0)) + " AGO" : "COMMON ANCESTOR " + yearsWords(gyr * 1e9) + " AGO";
            }
            drawText(canvas, 6, y, fitText(kin, UW - 12).c_str(), HUD_WHITE); y += 9;
            if (!when.empty()) { drawText(canvas, 6, y, fitText(when, UW - 12).c_str(), HUD_WHITE); y += 9; }
            std::string more;
            if (L.line.made) more = "TOO REGULAR TO HAVE EVOLVED: MADE BY A PEOPLE";
            else if (L.drifter) {
                bool beside = false;
                for (const TreeLeaf& o : tree.leaves) beside = beside || (!o.drifter && o.body >= 0 && o.line.genesis.id == L.line.genesis.id && o.line.lx == L.line.lx && o.line.ly == L.line.ly && o.line.lz == L.line.lz);
                more = beside ? "FROM A DRIFTER: KIN OF THE STAR IT LEFT" : "FROM A DRIFTER: LIFE OF THE STAR IT LEFT";
            } else if (L.body < 0) more = "THE SHIP'S OWN, FROM HOME";
            if (!more.empty()) drawText(canvas, 6, y, fitText(more, UW - 12).c_str(), HUD_AMBER);
        }
    } else drawTextCentered(canvas, UW / 2, UH / 2, "NOTHING ON THIS TREE YET", HUD_DIM);
    drawTextCentered(canvas, UW / 2, UH - 21, np > 1 ? "<- -> TREE  UP DOWN LEAF  ENTER TARGET STAR" : "UP DOWN LEAF  ENTER TARGET ITS STAR", HUD_DIM);
    drawTextCentered(canvas, UW / 2, UH - 12, worldPage ? "TAB SEQUENCER  BKSP STRAIN OFF  A PUT ONE BACK" : "TAB SEQUENCER", HUD_DIM);
    if (tree.picking) {   // A's chooser: the world's lines taken off its tree
        const int all = (int)tree.pickKeys.size(), rowsN = std::min(10, all), x0 = 30, y0 = 44;
        drawPanel(canvas, x0 - 6, y0 - 14, UW - x0 + 6, y0 + rowsN * 9 + 14, rgb(8, 14, 9), 235, treeColour(0.6f));
        drawText(canvas, x0, y0 - 10, "PUT BACK ON THE TREE", treeColour());
        const int top = clampi(tree.pickSel - rowsN + 1, 0, std::max(0, all - rowsN));
        for (int i = 0; i < rowsN; i++) {
            const int k = top + i;
            const bool sel = k == tree.pickSel;
            if (sel) drawText(canvas, x0, y0 + i * 9, ">", treeColour());
            drawText(canvas, x0 + 8, y0 + i * 9, fitText(treeLineWords(tree.pickKeys[k]), UW - 2 * x0 - 8).c_str(), sel ? HUD_WHITE : lineColour());
        }
        drawText(canvas, x0, y0 + rowsN * 9 + 3, "ENTER PUT BACK  ESC LEAVE", HUD_DIM);
    }
}

void Game::treePad(PadMenu& m) const {
    m.title = "TREE OF KIN";
    if (tree.picking) {
        m.item("PUT BACK", KEY_ENTER); m.item("NEXT", KEY_DOWN); m.item("PREVIOUS", KEY_UP); m.item("LEAVE", KEY_ESCAPE);
        m.button(PR_A, KEY_ENTER, "PUT BACK"); m.button(PR_B, KEY_ESCAPE, "LEAVE");
        return;
    }
    m.item("NEXT", KEY_DOWN); m.item("PREVIOUS", KEY_UP);
    if (tree.pages.size() > 1) { m.item("NEXT TREE", KEY_RIGHT); m.item("PREVIOUS TREE", KEY_LEFT); }
    m.item("TARGET ITS STAR", KEY_ENTER);
    if (!tree.pages.empty() && tree.pages[tree.page].kind == TP_WORLD) { m.item("TAKE STRAIN OFF", KEY_BACKSPACE); m.item("PUT A STRAIN BACK", KEY_A); }
    m.item("SEQUENCER", KEY_TAB); m.item("BACK", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "TARGET"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_SELECT, KEY_TAB, "SEQUENCER");
    if (tree.pages.size() > 1) { m.button(PR_LB, KEY_LEFT, "PREV TREE"); m.button(PR_RB, KEY_RIGHT, "NEXT TREE"); }
}
