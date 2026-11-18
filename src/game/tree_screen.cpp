// Z-06: the tree of kin's screen (R-425; docs/ideas/ITERATION-1.5.0.md Z-06, IDEAS-small-and-spacious.md 1.4 and 1.8; lab_state.h,
// life/sequences.h), over the room: E at its screen on the laboratory's port wall, or Tab from the sequencer. Every species sequenced and the
// ship's own at its root, by kinship: a genesis a clade (home's first, headed by its letters, its hand and when it began), its leaves drawn as
// the tree is (each two neighbours' common ancestor, billions of years back), the worlds as the explorer named them (or UNKNOWN); a drifter's
// life beside the life of the star it left, once both are read; the geneses counted. Up and Down choose a leaf (its nearest kin and their
// common ancestor told), Enter on a kin's world sets the remote target to its star; Tab the sequencer
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
const char* const SHIP_WHERE[SHIP_SPECIES] = {"THE GARDEN'S SOIL", "THE EXPLORER'S SKIN", "THE GARDEN POND"};
constexpr int TREE_X0 = 6, TREE_W = 76, TREE_Y0 = 22, TREE_ROW = 9, TREE_ROWS = 12;
std::string yearsShort(double gyr) { std::string w = yearsWords(std::max(1.0, gyr * 1e9)); return w.compare(0, 6, "ABOUT ") == 0 ? w.substr(6) : w; }
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

// every species sequenced (the guide's kin) and the ship's own, in the tree's order, each two neighbours' common ancestor
void Game::treeBuild() {
    tree.leaves.clear(); tree.join.clear();
    for (int k = 0; k < SHIP_SPECIES; k++) {
        TreeLeaf L; L.key = "SHIP/" + std::to_string(k); L.line = shipLineage(k); L.name = "THE SHIP'S OWN"; L.world = SHIP_WHERE[k];
        tree.leaves.push_back(L);
    }
    for (const auto& kv : guide.microbes) {
        if (!(kv.second.seen & SEEN_KIN)) continue;
        FlaskSpecies w;
        if (!speciesOfKey(kv.first, w)) continue;
        const Lineage& line = lineageOfKey(kv.first);
        if (!line.valid) continue;
        TreeLeaf L; L.key = kv.first; L.line = line; L.name = flaskSpeciesName(w);
        L.world = upper(nameOfKey(kv.first.substr(0, kv.first.rfind("/M"))));
        L.sx = w.sx; L.sy = w.sy; L.sz = w.sz; L.body = w.body;
        L.drifter = line.lx != line.wx || line.ly != line.wy || line.lz != line.wz;   // its line a star it left
        tree.leaves.push_back(L);
    }
    std::stable_sort(tree.leaves.begin(), tree.leaves.end(), [](const TreeLeaf& a, const TreeLeaf& b) { return treeBefore(a.line, b.line); });
    std::set<uint64_t> geneses;
    for (size_t k = 0; k < tree.leaves.size(); k++) {
        tree.join.push_back(k == 0 ? -1.0 : commonAncestorGyr(tree.leaves[k - 1].line, tree.leaves[k].line));
        geneses.insert(tree.leaves[k].line.genesis.id);
    }
    tree.geneses = (int)geneses.size();
    tree.sel = clampi(tree.sel, 0, std::max(0, (int)tree.leaves.size() - 1));
}

void Game::treeEnter() { treeBuild(); audio.beep = BEEP_UI; }

// a leaf's nearest kin in the tree (its index, -1 none) and their common ancestor
int Game::treeNearest(int i, double& gyr) const {
    int best = -1; gyr = -1;
    for (int j = 0; j < (int)tree.leaves.size(); j++) {
        if (j == i) continue;
        const double ca = commonAncestorGyr(tree.leaves[i].line, tree.leaves[j].line);
        if (ca >= 0 && (best < 0 || ca < gyr)) { best = j; gyr = ca; }
    }
    return best;
}

void Game::updateTree(const Input& in, double dt, double realDt) {
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    if (in.wasPressed(KEY_TAB)) { openScreen(GameState::SEQUENCER); return; }
    const int n = (int)tree.leaves.size();
    if (n == 0) return;
    menuStep(in, tree.sel, n);
    if (in.wasPressed(KEY_PAGE_UP)) tree.sel = std::max(0, tree.sel - 10);
    if (in.wasPressed(KEY_PAGE_DOWN)) tree.sel = std::min(n - 1, tree.sel + 10);
    if (enterKey(in)) {   // a kin's world: its star the remote target
        const TreeLeaf& L = tree.leaves[tree.sel];
        Star st;
        if (L.body < 0) { refuse("IT LIVES ABOARD", 2); return; }
        if (!placeInSector(L.sx, L.sy, L.sz, st, true)) { refuse("ITS STAR IS NO LONGER THERE", 2); return; }
        setRemoteStar(st);
        status(fitText("REMOTE TARGET: THE STAR OF " + (L.world == "UNKNOWN" ? std::string("AN UNNAMED WORLD") : L.world), UW - 12), 4);
        audio.beep = BEEP_LOCK;
    }
}

void Game::renderTree() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 218);
    drawText(canvas, 6, 4, "THE TREE OF KIN", treeColour());
    const std::string began = tree.geneses == 1 ? std::string("LIFE BEGAN AT LEAST ONCE") : fmt("LIFE BEGAN AT LEAST %d TIMES", tree.geneses);
    drawText(canvas, UW - 6 - textWidth(began.c_str()), 4, began.c_str(), HUD_WHITE);
    const int n = (int)tree.leaves.size();
    // the rows: a genesis's head before its first leaf, then its leaves
    struct Row { int leaf; bool head; };
    std::vector<Row> rows; int selRow = 0;
    for (int k = 0; k < n; k++) {
        if (tree.join[k] < 0) rows.push_back({k, true});
        if (k == tree.sel) selRow = (int)rows.size();
        rows.push_back({k, false});
    }
    tree.top = clampi(tree.top, std::max(0, selRow - TREE_ROWS + 2), std::max(0, selRow - 1));
    tree.top = clampi(tree.top, 0, std::max(0, (int)rows.size() - TREE_ROWS));
    std::vector<int> yOf(n, -1000);
    for (int r = 0; r < (int)rows.size(); r++) if (!rows[r].head) yOf[rows[r].leaf] = TREE_Y0 + (r - tree.top) * TREE_ROW + 3;
    const int vy0 = TREE_Y0 - 2, vy1 = TREE_Y0 + TREE_ROWS * TREE_ROW;
    double maxAge = 0.5;
    for (const TreeLeaf& L : tree.leaves) maxAge = std::max(maxAge, L.line.genesis.ageGyr);
    const int tx = TREE_X0 + TREE_W;
    auto xOf = [&](double gyr) { return tx - (int)std::lround(std::max(0.0, gyr) / maxAge * TREE_W); };
    auto hline = [&](int xa, int xb, int y, uint32_t c) { if (y >= vy0 && y <= vy1) drawLineRGB(canvas, std::min(xa, xb), y, std::max(xa, xb), y, c); };
    auto vline = [&](int x, int ya, int yb, uint32_t c) {
        const int a = std::max(vy0, std::min(ya, yb)), b = std::min(vy1, std::max(ya, yb));
        if (a <= b) drawLineRGB(canvas, x, a, x, b, c);
    };
    // each genesis's leaves drawn as their tree: a node at the oldest common ancestor between its two sides (each leaf's neighbours' joins)
    struct Sub { int y; double age; };
    std::function<Sub(int, int, uint32_t)> draw = [&](int lo, int hi, uint32_t c) -> Sub {
        if (lo == hi) return {yOf[lo], 0.0};
        int k = lo + 1;
        for (int j = lo + 2; j <= hi; j++) if (tree.join[j] > tree.join[k]) k = j;
        const Sub a = draw(lo, k - 1, c), b = draw(k, hi, c);
        const double age = tree.join[k];
        hline(xOf(a.age), xOf(age), a.y, c); hline(xOf(b.age), xOf(age), b.y, c);
        vline(xOf(age), a.y, b.y, c);
        return {(a.y + b.y) / 2, age};
    };
    for (int lo = 0; lo < n;) {
        int hi = lo;
        while (hi + 1 < n && tree.join[hi + 1] >= 0) hi++;
        const uint32_t c = genesisColour(tree.leaves[lo].line.genesis.id, 0.8f);
        const Sub root = draw(lo, hi, c);
        hline(xOf(root.age), xOf(tree.leaves[lo].line.genesis.ageGyr), root.y, c);   // its stem back to its beginning
        lo = hi + 1;
    }
    // the leaves' and the heads' lines
    for (int r = tree.top; r < (int)rows.size() && r < tree.top + TREE_ROWS; r++) {
        const int y = TREE_Y0 + (r - tree.top) * TREE_ROW;
        const TreeLeaf& L = tree.leaves[rows[r].leaf];
        if (rows[r].head) {
            const Genesis& g = L.line.genesis;
            drawText(canvas, tx + 8, y, fitText(genesisWords(g), UW - tx - 14).c_str(), genesisColour(g.id));
            continue;
        }
        const bool sel = rows[r].leaf == tree.sel;
        std::string tag = L.line.made ? " - MADE" : (L.drifter ? " - A DRIFTER'S" : "");
        const std::string line = L.name + ", " + (L.world == "UNKNOWN" ? std::string("UNKNOWN") : L.world) + tag;
        if (sel) drawText(canvas, tx + 2, y, ">", treeColour());
        drawText(canvas, tx + 8, y, fitText(line, UW - tx - 14).c_str(), sel ? HUD_WHITE : (L.line.made ? HUD_AMBER : HUD_GREEN));
    }
    // the years' axis
    {
        const int y = vy1 + 2;
        drawLineRGB(canvas, xOf(maxAge), y, tx, y, HUD_DIM);
        for (int b = 0; b <= (int)maxAge; b += maxAge > 6 ? 2 : 1) {
            const int x = xOf(b);
            drawLineRGB(canvas, x, y, x, y + 2, HUD_DIM);
            drawTextCentered(canvas, x, y + 4, fmt("%d", b).c_str(), HUD_DIM);
        }
        drawText(canvas, tx + 8, y + 4, "BILLION YEARS AGO", HUD_DIM);
    }
    // the leaf chosen: its kin and their common ancestor, what it is
    if (n > 0) {
        const TreeLeaf& L = tree.leaves[tree.sel];
        int y = UH - 44;
        double gyr = -1;
        const int k = treeNearest(tree.sel, gyr);
        std::string kin, when;
        if (k < 0) kin = "NO COMMON ANCESTOR WITH ANY LIFE READ";
        else {
            const TreeLeaf& K = tree.leaves[k];
            kin = "ITS NEAREST KIN: " + K.name + (K.body >= 0 ? " OF " + K.world : ", " + K.world);
            when = gyr < 1e-3 ? "ONE SPECIES, CARRIED APART " + yearsWords(std::max(gyr * 1e9, 1.0)) + " AGO" : "A COMMON ANCESTOR " + yearsWords(gyr * 1e9) + " AGO";
        }
        drawText(canvas, 6, y, fitText(kin, UW - 12).c_str(), HUD_WHITE); y += 9;
        if (!when.empty()) { drawText(canvas, 6, y, fitText(when, UW - 12).c_str(), HUD_WHITE); y += 9; }
        std::string more;
        if (L.line.made) more = "TOO REGULAR TO HAVE EVOLVED: MADE BY A PEOPLE";
        else if (L.drifter) {
            bool beside = false;
            for (const TreeLeaf& o : tree.leaves) beside = beside || (!o.drifter && o.body >= 0 && o.line.genesis.id == L.line.genesis.id && o.line.lx == L.line.lx && o.line.ly == L.line.ly && o.line.lz == L.line.lz);
            more = beside ? "A DRIFTER'S, BESIDE THE LIFE OF THE STAR IT LEFT" : "A DRIFTER'S: ITS LIFE THE STAR'S IT LEFT";
        } else if (L.body < 0) more = "THE SHIP'S OWN, OF HOME'S LIFE";
        if (!more.empty()) { drawText(canvas, 6, y, fitText(more, UW - 12).c_str(), HUD_AMBER); y += 9; }
    }
    drawTextCentered(canvas, UW / 2, UH - 12, "UP DOWN A LEAF  ENTER ITS STAR THE TARGET  TAB THE SEQUENCER", HUD_DIM);
}

void Game::treePad(PadMenu& m) const {
    m.title = "THE TREE OF KIN";
    m.item("THE NEXT", KEY_DOWN); m.item("THE ONE BEFORE", KEY_UP); m.item("ITS STAR THE TARGET", KEY_ENTER); m.item("THE SEQUENCER", KEY_TAB);
    m.item("BACK", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "TARGET"); m.button(PR_B, KEY_ESCAPE, "BACK"); m.button(PR_SELECT, KEY_TAB, "SEQUENCER");
}
