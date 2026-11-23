// The Y series (2026-11-19; the unit's `genes` group, after `incubator`): Y-01, R-444's tree of kin by world and by folder: its pages (the
// ship's own, each world with a species read, each folder holding a culture), a world's species read and its lab's lines as twigs under their
// parents, Backspace taking a line off and A putting it back (kept in the guide), a folder's page what it holds (a strain kept alone in its
// species' place), each page's years, Tab from the sequencer on the page of what it read. Y-02, R-443's gene editor: the genes' laws
// (`geneLaws`), a line written from a flask by the editor's keys into an empty flask and read everywhere a strain is (its culture, its drop, the
// sequencer, the collection, its twig), what a gene does learned (one change, several, a vital death, nothing seen, a strain read), the
// editor and its lines over a save and a load. Y-03, genes from others: a gene moved in (the law over the six worlds; K's kin, another life
// refused with its words, M over its own and into a gap), its donor's line on the tree, its price by distance, D's copy, what a gene needs, the
// kin and its genes over a save and a load. Y-04, the switches: the own switches every species' curve (the laws), each condition in the
// culture's segments (the light, the warmth against its best, starving, its crowd's share), a gene off costing nothing, S on the editor and a
// switched line's drop
#include "harness/harness.h"
#include "harness/test_game.h"
#include <fstream>
#include <map>
#include <set>

namespace {
const char* const GUIDE = "shots/tests/unit_genes_guide.txt";
// a game on the ship, its guide fresh, its files the harness's
void labGame(TestGame& g) {
    g.savePrefix = "shots/tests/unit_genes_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = GUIDE; g.keysPath = "shots/tests/test_keys.txt";
    remove(GUIDE);
    g.newGame(); g.setState(GameState::SPACE);
}
// the scan region's first living worlds' species as a vial of each shows them, every living or sleeping one seen (the guide's sheet): the
// worlds' count
int seeWorlds(TestGame& g, int want) {
    int n = 0;
    for (auto& [x, z, s, scanned] : scanSystems(150, 340, 20, 160, [&] { return n < want; })) {
        const StarSystem& sys = scanned;
        auto L = lifeOf(sys);
        for (int bi = 0; bi < (int)sys.bodies.size() && n < want; bi++) {
            if (!livingStage(L->bodies[bi].stage)) continue;
            Vial v;
            if (!labVial(sys, bi, v)) continue;
            for (const Held& H : sampleOf(L->bodies[bi], v.hab).held)
                if (H.state != SS_FOSSIL && H.state != SS_PROTO) g.guide.microbes[Guide::bodyKey(v.sx, v.sy, v.sz, bi) + "/M" + std::to_string(H.species)].seen |= SEEN_FORM;
            n++;
        }
    }
    return n;
}
std::string worldKey(const std::string& key) { return key.substr(0, key.rfind("/M")); }
FlaskSpecies flaskOf(TestGame& g, const std::string& key, int strain) {
    FlaskSpecies w;
    g.testSpeciesOfKey(key, w);
    w.strain = w.strainIn = strain;
    return w;
}
int rowOf(const TreeState& T, const std::string& key) {
    for (size_t k = 0; k < T.shown.size(); k++) if (T.shown[k].key == key) return (int)k;
    return -1;
}
int pageOf(const TreeState& T, int kind, const std::string& key) {
    for (size_t p = 0; p < T.pages.size(); p++) if (T.pages[p].kind == kind && T.pages[p].key == key) return (int)p;
    return -1;
}
// Y-02: a culture put straight into a flask (its species or line, a medium where it grows), its lab's clock begun there
void putIn(TestGame& g, int i, const FlaskSpecies& w, const Medium& m) {
    Flask& F = g.testLab().flasks[i];
    F = Flask(); F.n = 1; F.who[0] = w; F.who[0].strainIn = w.strain; F.medium = m; F.asWorld = m;
    CultureEvent e; e.kind = CD_START; e.t = g.testLab().clock; e.m = m;
    F.events.push_back(e);
    g.testLabSkip(0);
}
// where a kind grows best: its cell, its best warmth, its salt, its light and its energy
Medium bestOf(const CultureKind& k) {
    Medium m; m.cell = chemistryCell(k.chemistry); m.tempC = std::round(k.s.tOpt); m.salt = k.s.saltLo;
    m.light = k.s.way == LW_LIGHT ? CL_WHITE : CL_DARK;
    m.energy = k.s.way == LW_CHEMICAL ? k.s.substrate : (k.s.way == LW_LIGHT ? CE_NONE : CE_BROTH);
    return m;
}
}   // namespace

// a run of a gene's letters of a weight `d` heavier (one letter at a time, the first that can), or a table's run that reads `want`
std::string heavier(const GeneCode& c, std::string p, int d) {
    const char* abc = alphabetOf(c.letters);
    for (int guard = 0; d > 0 && guard < 64; guard++)
        for (int i = 0; i < (int)p.size() && d > 0; i++) {
            const int w = geneWeight(c, p, i, 1);
            if (w + 1 >= c.letters) continue;
            for (int l = 0; l < c.letters; l++) if (c.weight[l] == w + 1) { p[i] = abc[l]; d--; break; }
        }
    return p;
}
std::string runReading(const GeneCode& c, const std::vector<int>& t, int want, int n) {
    const char* abc = alphabetOf(c.letters);
    for (int r = 0; r < (int)t.size(); r++) {
        if (t[r] != want) continue;
        std::string s; int v = r;
        for (int i = 0; i < n; i++) { s += abc[v % c.letters]; v /= c.letters; }
        return s;
    }
    return "";
}

namespace {
const MicrobeGene* geneOfKind(const MicrobeGenes& g, int kind) { for (const MicrobeGene& G : g.genes) if (G.kind == kind) return &G; return nullptr; }
// the log's last GENE line (each finding is logged as it is seen, said in its turn after)
std::string lastGene(const TestGame& g) {
    for (auto it = g.guide.log.rbegin(); it != g.guide.log.rend(); ++it) if (it->kind == "GENE") return it->text;
    return "";
}
// Y-03: a world's living species as the flasks take them (their kinds, forms and genes), each read (the guide's sheet) where asked
struct Cult { FlaskSpecies w; std::string key; CultureKind k; Morph f; MicrobeGenes g; };
std::vector<Cult> culturesOf(TestGame& g, const StarSystem& sys, int body) {
    std::vector<Cult> out;
    for (const Microbe& m : lifeOf(sys)->bodies[body].species) {
        if (m.state != SS_LIVING) continue;
        Cult c; c.w.sx = sys.star.sx; c.w.sy = sys.star.sy; c.w.sz = sys.star.sz; c.w.body = body; c.w.species = m.index;
        c.key = Guide::bodyKey(c.w.sx, c.w.sy, c.w.sz, body) + "/M" + std::to_string(m.index);
        if (g.testFlaskKind(c.w, c.k, c.f, &c.g) && c.g.valid) out.push_back(c);
    }
    return out;
}
void readIt(TestGame& g, const Cult& c) { g.guide.microbes[c.key].seen |= SEEN_FORM | SEEN_KIN; }
// a moved gene as the editor makes it, its donor read from its culture
GeneEdit movedFrom(const Cult& d, int place, int far) {
    GeneEdit e; e.place = place; e.how = GX_MOVED; e.part = d.g.at(place)->part; e.donor = d.key; e.read = true;
    e.donorKind = d.k; e.donorForm = d.f; e.donorGene = *d.g.at(place); e.far = far; e.origin = d.key;
    return e;
}
uint32_t senseBitOf(int t) { return t == SE_LIGHT ? TX_LIGHT : (t == SE_FOOD ? TX_FOOD : (t == SE_FIELD ? TX_FIELD : (t == SE_WARMTH ? TX_WARMTH : 0u))); }
// Y-04: the scan region's first living species (of a genesis) a check wants, as a flask takes it
template <class Want> bool findCult(TestGame& g, Want want, Cult& out) {
    bool found = false;
    for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return !found; })) {
        auto L = lifeOf(sys);
        for (int b = 0; b < (int)sys.bodies.size() && !found; b++) {
            if (!livingStage(L->bodies[b].stage) || L->bodies[b].genesis.id == 0) continue;
            for (const Cult& c : culturesOf(g, sys, b)) if (!found && want(c)) { out = c; found = true; }
        }
    }
    return found;
}
// a line of a culture by its edits (the guide's strain line, its number n) as a flask reads it (its switches priced)
CultureKind lineOf(TestGame& g, const Cult& a, int n, const std::string& edits) {
    StrainRecord r; r.what = SM_EDIT; r.edits = edits;
    g.guide.strains[a.key + "/S" + std::to_string(n)] = r;
    FlaskSpecies line = a.w; line.strain = line.strainIn = n;
    CultureKind k; Morph f; MicrobeGenes ge;
    g.testFlaskKind(line, k, f, &ge);
    return k;
}
// a culture alone in a medium from 0; its growing segments' rates (the segments with a rate)
FlaskCulture aloneIn(const CultureKind& k, const Medium& m) {
    FlaskCulture c; c.kind[0] = k;
    CultureEvent e; e.kind = CD_START; e.t = 0; e.m = m;
    c.events.push_back(e);
    return c;
}
std::vector<const CultureSeg*> growing(const CultureRun& R) {
    std::vector<const CultureSeg*> v;
    for (const CultureSeg& s : R.seg[0]) if (s.mu > 0) v.push_back(&s);
    return v;
}
bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) <= tol * std::max(std::fabs(a), std::fabs(b)); }
}   // namespace

void unitLabGenes(const Check& check) {
    {   // R-453: GREEN and PINK in every genesis's dye table (the six worlds' alphabets and hands): each of the eleven dyes read by a run, the two
        // new ones a run each, taken from J-01's synonyms, so every other run reads the dye J-01's table gave it (drawn again here as genes.cpp
        // drew it: its runs shuffled on the genesis's dye salt, the nine dyes first, the rest at random among them)
        TestGame g;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        int tables = 0, right = 0;
        std::string detail;
        for (const GeneWorld& w : W) {
            if (w.body < 0) continue;
            for (const Cult& cu : culturesOf(g, w.sys, w.body)) {
                const MicrobeGenes& ge = cu.g;
                if (!ge.valid) continue;
                const GeneCode c = geneCodeOf(ge.genesis);
                const int runs = c.letters * c.letters;
                std::vector<int> j01(runs), order(runs);
                Rng r(ge.genesis.id ^ 0x6E4E50000000000AULL);   // (genes.cpp's SALT_GENE_DYE)
                for (int i = 0; i < runs; i++) order[i] = i;
                for (int i = runs - 1; i > 0; i--) std::swap(order[i], order[r.irange(i + 1)]);
                for (int i = 0; i < runs; i++) j01[order[i]] = i < WD_FLUORESCENT + 1 ? i : r.irange(WD_FLUORESCENT + 1);
                int count[WD_COUNT] = {}, moved = 0;
                for (int i = 0; i < runs && i < (int)c.dye.size(); i++) { count[clampi(c.dye[i], 0, WD_COUNT - 1)]++; moved += c.dye[i] != j01[i]; }
                bool ok = (int)c.dye.size() == runs && count[WD_GREEN] == 1 && count[WD_PINK] == 1 && moved == 2;
                for (int d = 0; d < WD_COUNT; d++) ok = ok && count[d] >= 1;
                tables++; right += ok;
                detail += fmt("%s%d letters%s: %d runs, %d moved%s", tables > 1 ? "; " : "", c.letters, w.mirrored ? " mirrored" : "", runs, moved, ok ? "" : " WRONG");
                break;
            }
        }
        check("genes: GREEN and PINK in every genesis's dye table, a run each from its synonyms, the rest as J-01 drew them (R-453)", tables >= 5 && right == tables,
              fmt("%d of %d tables right: %s", right, tables, detail.c_str()));
    }
    {   // Y-01 (R-444): the tree by world and by folder. Twelve living worlds' species seen, every one read but one of the world with the most
        // (W); one of W's read (X) given three strains (S2 of S1); a folder MINE holding another world's species (Y), X's S2 alone and W's
        // unread one. The pages: the ship's, one a world with a species read, MINE (UNSORTED empty, none). W's page: its species read and no
        // other's, one not read, X's lines under it (S2 under S1); its years its own (its oldest ancestor's and up to a quarter, not past its
        // life's start). Backspace on S1: off (S2 under X), kept in the guide's file; on X: refused. A: S1 offered, Enter: back. MINE's page: Y,
        // S2 in X's place (a leaf of its own), one not read. Right from the last page: the first. X read on the sequencer, Tab: W's page, X
        // chosen
        TestGame g; Input in;
        labGame(g);
        const int worlds = seeWorlds(g, 12);
        std::map<std::string, std::vector<std::string>> byWorld;
        for (const auto& kv : g.guide.microbes) byWorld[worldKey(kv.first)].push_back(kv.first);
        std::string W;
        for (const auto& kv : byWorld) if (W.empty() || kv.second.size() > byWorld[W].size()) W = kv.first;
        const std::vector<std::string> wSpecies = W.empty() ? std::vector<std::string>() : byWorld[W];
        const std::string U = wSpecies.empty() ? std::string() : wSpecies.back();   // seen, not read
        for (auto& kv : g.guide.microbes) if (kv.first != U) kv.second.seen |= SEEN_KIN;
        TreeState& T = g.testTree();
        // X: W's first leaf read (a species with a genome); Y: another world's
        std::string X, Y;
        for (const TreeLeaf& L : T.leaves) {
            if (L.body < 0) continue;
            if (X.empty() && worldKey(L.key) == W) X = L.key;
            if (Y.empty() && worldKey(L.key) != W) Y = L.key;
        }
        bool pages = false, wPage = false, lines = false, years = false, off = false, kept = false, refused = false, back = false, folder = false,
             wraps = false, tab = false;
        std::string wLines, folderRows;
        int wRead = 0, wUnread = -1, nWorlds = 0;
        double wYears = 0, wBegan = 0;
        if (!X.empty() && !Y.empty()) {
            StrainRecord s1, s2, s3;
            s1.what = SM_HOT; s1.value = 40; s2.parent = 1; s2.what = SM_SIZE; s2.value = 1.25; s3.what = SM_COLD; s3.value = -5;
            g.guide.strains[X + "/S1"] = s1; g.guide.strains[X + "/S2"] = s2; g.guide.strains[X + "/S3"] = s3;
            CultureFolder f; f.id = 1; f.name = "MINE";
            g.testLab().folders.push_back(f);
            for (const FlaskSpecies& w : {flaskOf(g, Y, 0), flaskOf(g, X, 2), flaskOf(g, U, 0)}) { KeptCulture K; K.who = w; K.folder = 1; g.testLab().kept.push_back(K); }
            g.openScreen(GameState::TREE);
            std::set<std::string> readWorlds;
            for (const TreeLeaf& L : T.leaves) if (L.body >= 0) readWorlds.insert(worldKey(L.key));
            int ship = 0, folders = 0;
            for (const TreePage& P : T.pages) { ship += P.kind == TP_SHIP; nWorlds += P.kind == TP_WORLD; folders += P.kind == TP_FOLDER; }
            pages = T.pages.size() > 2 && T.pages[0].kind == TP_SHIP && ship == 1 && nWorlds == (int)readWorlds.size() && folders == 1 &&
                    T.pages.back().kind == TP_FOLDER && T.pages.back().name == "FOLDER MINE";
            // W's page: its species read, no other world's, the one not read counted; X's lines under it
            const int wp = pageOf(T, TP_WORLD, W);
            g.testTreePage(wp);
            int others = 0;
            for (const TreeLeaf& L : T.shown) {
                if (L.lab) { wLines += (wLines.empty() ? "" : ", ") + L.key.substr(L.key.rfind("/S") + 1) + fmt(" at %d", L.depth); continue; }
                if (worldKey(L.key) == W) wRead++; else others++;
            }
            wUnread = T.unread;
            wPage = wp > 0 && others == 0 && wRead == (int)wSpecies.size() - 1 && wUnread == 1;
            const int rx = rowOf(T, X), r1 = rowOf(T, X + "/S1"), r2 = rowOf(T, X + "/S2"), r3 = rowOf(T, X + "/S3");
            lines = rx >= 0 && r1 == rx + 1 && r2 == rx + 2 && r3 == rx + 3 && T.shown[r1].depth == 1 && T.shown[r1].parent == X && T.shown[r2].depth == 2 &&
                    T.shown[r2].parent == X + "/S1" && T.shown[r3].depth == 1 && T.shown[r3].parent == X;
            wYears = g.testTreeYears();
            for (const TreeLeaf& L : T.shown) wBegan = std::max(wBegan, L.line.genesis.ageGyr);
            double oldest = 0;
            for (size_t k = 0; k < T.shown.size(); k++) if (T.shown[k].depth == 0) oldest = std::max(oldest, T.shownJoin[k]);
            years = wYears > 0 && (oldest > 0 ? wYears >= 1.05 * oldest - 1e-9 && wYears <= 1.25 * oldest + 1e-9 && wYears <= std::max(wBegan, 1.05 * oldest) + 1e-9
                                                : wYears == wBegan);
            // Backspace on S1: off its tree, S2 under X; the guide's file keeps it; on X: refused
            T.sel = r1; g.frame(in, 0); in.newFrame();
            pressKey(g, in, KEY_BACKSPACE);
            const int s2At = rowOf(T, X + "/S2");
            off = g.guide.treeOff.count(X + "/S1") == 1 && rowOf(T, X + "/S1") < 0 && s2At == rowOf(T, X) + 1 && T.shown[s2At].depth == 1 && T.shown[s2At].parent == X;
            Guide G2; G2.load(GUIDE);
            kept = G2.treeOff.count(X + "/S1") == 1;
            T.sel = rowOf(T, X);
            pressKey(g, in, KEY_BACKSPACE);
            refused = g.guide.treeOff.size() == 1 && rowOf(T, X) >= 0;
            // A: S1 offered alone; Enter puts it back, S2 under it again
            pressKey(g, in, KEY_A);
            const bool offered = T.picking && T.pickKeys.size() == 1 && T.pickKeys[0] == X + "/S1";
            pressKey(g, in, KEY_ENTER);
            back = offered && !T.picking && g.guide.treeOff.empty() && rowOf(T, X + "/S1") >= 0 && T.shown[rowOf(T, X + "/S2")].depth == 2 &&
                   T.shown[T.sel].key == X + "/S1";
            // MINE's page: Y, S2 standing in X's place, the one not read
            g.testTreePage((int)T.pages.size() - 1);
            int ys = 0, s2s = 0, rest = 0;
            for (const TreeLeaf& L : T.shown) {
                folderRows += (folderRows.empty() ? "" : "; ") + TestGame::testTreeLine(L);
                if (L.key == Y && !L.lab) ys++;
                else if (L.key == X + "/S2" && L.lab && L.depth == 0) s2s++;
                else rest++;
            }
            folder = ys == 1 && s2s == 1 && rest == 0 && T.unread == 1;
            pressKey(g, in, KEY_RIGHT);
            wraps = T.page == 0;
            // read X on the sequencer; Tab: its world's page, X chosen
            g.closeScreen();
            FlaskSpecies wx = flaskOf(g, X, 0);
            g.testSeqPut(wx.sx, wx.sy, wx.sz, wx.body, wx.species);
            g.testLabSkip(SEQUENCE_S + 1);
            g.openScreen(GameState::SEQUENCER); g.frame(in, 0); in.newFrame();
            pressKey(g, in, KEY_TAB);
            tab = g.state == GameState::TREE && T.page == wp && T.sel >= 0 && T.sel < (int)T.shown.size() && T.shown[T.sel].key == X;
            g.closeScreen();
        }
        check("genes: the tree by world and by folder: its pages, a world's species and its lines, Backspace and A, a folder's tree (Y-01)",
              worlds == 12 && pages && wPage && lines && years && off && kept && refused && back && folder && wraps && tab,
              fmt("%d worlds seen; %zu pages (%d worlds, the folder's last) %s; W's page %d read, %d not read %s, its lines %s %s; its years %.3f Gyr "
                  "(its life began %.2f) %s; S1 off %s, in the guide's file %s, X's refused %s, put back %s; MINE: %s %s; Right from the last %s; "
                  "Tab from the sequencer %s",
                  worlds, T.pages.size(), nWorlds, pages ? "as made" : "WRONG", wRead, wUnread, wPage ? "as seen" : "WRONG", wLines.c_str(),
                  lines ? "under their parents" : "MISPLACED", wYears, wBegan, years ? "its own" : "WRONG", off ? "yes" : "NO", kept ? "yes" : "NO",
                  refused ? "yes" : "NO", back ? "yes" : "NO", folderRows.c_str(), folder ? "as held" : "WRONG", wraps ? "the first" : "NOT THE FIRST",
                  tab ? "on X's page" : "ELSEWHERE"));
        remove(GUIDE);
    }
    {   // Y-02 (R-443): the genes' laws, over forty worlds and the six alphabets' and hands' worlds (`geneLaws`); Y-03's gene moved in
        const GeneLaws R = geneLaws();
        check("genes: a living world of each alphabet and hand (Y-02)", R.worlds.ok, R.worlds.detail);
        check("genes: deterministic (Y-02)", R.deterministic.ok, R.deterministic.detail);
        check("genes: every species unedited is itself: its values, its culture's segments, its slide's kinds (Y-02)", R.itself.ok, R.itself.detail);
        check("genes: each gene's edit and cut as its code says (Y-02)", R.asCode.ok, R.asCode.detail);
        check("genes: a vital gene's edit kills, a quiet one shows nothing (Y-02)", R.vitalQuiet.ok, R.vitalQuiet.detail);
        check("genes: how it lives and its plan's family never move; its values within their limits (Y-02)", R.fixed.ok, R.fixed.detail);
        check("genes: a gene moved in takes its donor's letters and value, over its own or into a gap; its way and family fixed (Y-03)", R.moved.ok, R.moved.detail);
        check("genes: the own switches (a glow's WHEN CROWDED, a rest's WHEN STARVING, the rest ALWAYS) give every species' curve as before (Y-04)", R.wild.ok, R.wild.detail);
        check("genes: the cell's genes (places 1 to 18, the old tables, every edit of them) as before the world genes (J-01)", R.cellAsBefore.ok, R.cellAsBefore.detail);
        check("genes: every species carries the fourteen world genes at 19 to 32, read from what it is (J-01)", R.worldCarried.ok, R.worldCarried.detail);
        check("genes: a world product not its own (a gas, a dye, a slime, a shell, a poison) costs its line, its own nothing (J-01)", R.worldPrice.ok, R.worldPrice.detail);
    }
    {   // Y-02: a line written by the editor's keys and read everywhere a strain is. A rod of a four letters' world (ours), read, in flask 1
        // at its best: E on the editor opens the chooser (THE FLASKS), Enter its row; its shape gene's letters by hand (Enter, Up: an edit;
        // Backspace: its parent's again), then by the hook a sphere's run; W writes it, the lab's clock past EDIT_S: E1 in flask 2, the guide's
        // line its edits. Its culture a sphere's (its kind, its form), its drop spheres (learned: its shape), the sequencer's reading EDITED, the
        // collection's copy as it grows, its twig on its world's tree. A culture not read refused; nothing changed refused
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        const GeneWorld& w = W[0];
        FlaskSpecies X; std::string key;
        if (w.body >= 0) {
            for (const Microbe& m : lifeOf(w.sys)->bodies[w.body].species)
                if (key.empty() && m.state == SS_LIVING && m.plan == MP_ROD) {
                    X.sx = w.sys.star.sx; X.sy = w.sys.star.sy; X.sz = w.sys.star.sz; X.body = w.body; X.species = m.index;
                    key = Guide::bodyKey(X.sx, X.sy, X.sz, w.body) + "/M" + std::to_string(m.index);
                }
        }
        bool chosen = false, letters = false, undone = false, refusedUnread = false, refusedNone = false, written = false, inFlask = false, recorded = false,
             cultured = false, dropped = false, shapeLearned = false, readIt = false, keptIt = false, twig = false;
        std::string twigWords, readLine, edRow, seqRow;
        if (!key.empty()) {
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(X, k0, f0, &g0);
            const Medium best = bestOf(k0);
            putIn(g, 0, X, best);
            refusedUnread = !g.testEditorPut(X, best, false) && !g.testEditor().on;
            g.guide.microbes[key].seen |= SEEN_FORM | SEEN_KIN;
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            pressKey(g, in, KEY_ENTER);   // the chooser
            pressKey(g, in, KEY_ENTER);   // THE FLASKS
            pressKey(g, in, KEY_ENTER);   // its row
            EditorState& E = g.testEditor();
            chosen = E.on && E.who.sameSpecies(X) && E.who.strain == 0;
            pressKey(g, in, KEY_W);
            refusedNone = !E.writing;
            const MicrobeGene* S = geneOfKind(E.genes, GN_SHAPE);
            if (S) {
                E.sel = S->place;
                pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_UP);
                letters = E.letters && E.edits.size() == 1 && E.edits[0].place == S->place && E.edits[0].part != S->part;
                pressKey(g, in, KEY_BACKSPACE); pressKey(g, in, KEY_ENTER);
                undone = !E.letters && E.edits.empty();
                const GeneCode c = geneCodeOf(E.genesis);
                g.testEditorSet(S->place, false, runReading(c, c.shape, 0, 2) + S->part.substr(2));   // a sphere's run
                pressKey(g, in, KEY_W);
                pressKey(g, in, KEY_ENTER);   // R-445: UNSORTED; then its name, typed over its parent's
                for (int i = 0; i < 28; i++) pressKey(g, in, KEY_BACKSPACE);
                for (const char ch : std::string("ROUND")) pressKey(g, in, KEY_A + (ch - 'A'));
                pressKey(g, in, KEY_ENTER);
                written = E.writing;
                g.closeScreen();
                g.testLabSkip(EDIT_S + 1);
                const auto rec = g.guide.strains.find(key + "/S1");
                recorded = rec != g.guide.strains.end() && rec->second.what == SM_EDIT && rec->second.parent == 0 && geneEditsOf(rec->second.edits).size() == 1;
                const Flask& F = g.testLab().flasks[1];
                inFlask = F.n == 1 && F.who[0].sameSpecies(X) && F.who[0].strain == 1 && F.edited[0] && !E.writing;
                cultured = inFlask && F.culture.kind[0].s.plan == MP_SPHERE && F.forms.size() == 1 && F.forms[0].plan == MP_SPHERE && F.parentForm[0].plan == MP_ROD;
                // its drop: spheres; its shape known since it was changed (R-450)
                g.testLabSkip(30);
                dropped = g.testScopeStageFlask(1) && g.testScope().forms.size() == 1 && g.testScope().forms[0].plan == MP_SPHERE;
                const GeneKnown* gk = g.guide.genes.count(g.testGeneKey(X, S->place)) ? &g.guide.genes[g.testGeneKey(X, S->place)] : nullptr;
                shapeLearned = gk && gk->kind == GN_SHAPE && g.testGeneWords(X, S->place) == "SHAPE";
                // read on the sequencer: its edits said
                FlaskSpecies line = X; line.strain = line.strainIn = 1;
                g.testSeqPutLine(line);
                g.testLabSkip(SEQUENCE_S + 1);
                for (const std::string& l : g.testLab().seq.result) if (l.compare(0, 7, "EDITED:") == 0) readLine = l;
                readIt = !readLine.empty() && g.testLab().seq.who.strain == 1;
                // kept as it grows; its twig on its world's tree
                for (int t = 0; t < 40; t++) g.testLabSkip(30);
                for (const KeptCulture& K : g.testLab().kept) keptIt = keptIt || (K.who.sameSpecies(X) && K.who.strain == 1);
                // B-472: the kept line in the editor's and the sequencer's choosers by its own name, as the microscope's list has it
                for (const EditorChoice& c : g.testEditorChoices()) if (c.from == "THE COLLECTION" && c.who.sameSpecies(X) && c.who.strain == 1) edRow = c.line;
                for (const SequencerChoice& c : g.testSequencerChoices())
                    if (c.who.sameSpecies(X) && c.who.strain == 1 && c.line.compare(0, 6, "FLASK ") != 0) seqRow = c.line;
                TreeState& T = g.testTree();
                g.testTreeShow(key);
                for (const TreeLeaf& L : T.shown) if (L.key == key + "/S1" && L.lab && L.depth == 1) { twig = true; twigWords = L.name; }
                twig = twig && twigWords.compare(0, 8, "ROUND - ") == 0;
            }
        }
        check("genes: a line written on the editor into a flask, read as a strain is: its culture, its drop, the sequencer, the collection, its twig (Y-02)",
              !key.empty() && refusedUnread && chosen && refusedNone && letters && undone && written && recorded && inFlask && cultured && dropped && shapeLearned &&
              readIt && keptIt && twig,
              fmt("%s M%d; not read refused %s; chosen by its keys %s; nothing changed refused %s; a letter by hand %s, undone %s; written %s; the guide's line "
                  "%s; in flask 2 %s, a sphere's culture %s; its drop %s, its shape learned %s; read: %s; kept %s; its twig %s",
                  w.body >= 0 ? w.sys.bodies[w.body].name.c_str() : "NONE", X.species, refusedUnread ? "yes" : "NO", chosen ? "yes" : "NO",
                  refusedNone ? "yes" : "NO", letters ? "yes" : "NO", undone ? "yes" : "NO", written ? "yes" : "NO", recorded ? "SM_EDIT" : "WRONG",
                  inFlask ? "yes" : "NO", cultured ? "yes" : "NO", dropped ? "spheres" : "WRONG", shapeLearned ? "yes" : "NO", readLine.c_str(),
                  keptIt ? "yes" : "NO", twigWords.c_str()));
        remove(GUIDE);
        check("genes: a kept line by its own name in the editor's and the sequencer's choosers, as on the microscope (B-472)",
              edRow.compare(0, 5, "ROUND") == 0 && seqRow.compare(0, 5, "ROUND") == 0,
              fmt("the editor's row '%s', the sequencer's '%s'", edRow.c_str(), seqRow.c_str()));
    }
    {   // R-445: W asks for the new strain's folder, then its name. The same rod, named ROD by the player, kept in a folder MINE, one gene
        // changed: W opens the folders at MINE (the parent's); Esc writes nothing. W, Enter: the name entry holds "ROD E1" (the parent's name
        // and the new strain's number); E1 taken off and DARK typed, Enter: writing, its folder and name kept over a save. Past EDIT_S: ROD DARK
        // in a flask and in MINE, no second copy once it grows. Then W, N: a new folder's name (NEW), then the name entry ("ROD E2"); an empty
        // name refused, the entry open again
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        const GeneWorld& w = W[0];
        FlaskSpecies X; std::string key;
        if (w.body >= 0)
            for (const Microbe& m : lifeOf(w.sys)->bodies[w.body].species)
                if (key.empty() && m.state == SS_LIVING && m.plan == MP_ROD) {
                    X.sx = w.sys.star.sx; X.sy = w.sys.star.sy; X.sz = w.sys.star.sz; X.body = w.body; X.species = m.index;
                    key = Guide::bodyKey(X.sx, X.sy, X.sz, w.body) + "/M" + std::to_string(m.index);
                }
        bool startsThere = false, cancelled = false, prefilled = false, writing = false, saved = false, named = false, kept = false, once = false,
             newFolder = false, emptyRefused = false;
        std::string typed;
        if (!key.empty()) {
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(X, k0, f0, &g0);
            const Medium best = bestOf(k0);
            g.guide.microbes[key].seen |= SEEN_FORM | SEEN_KIN;
            g.guide.names[key] = "ROD";
            const int mine = g.testFolderNew("MINE");
            KeptCulture parent; parent.who = X; parent.medium = parent.asWorld = best;
            g.testLab().kept.push_back(parent);
            g.testLab().kept.back().folder = mine;
            putIn(g, 0, X, best);
            g.testEditorPut(X, best, true);
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            EditorState& E = g.testEditor();
            const MicrobeGene* S = geneOfKind(E.genes, GN_SHAPE);
            if (S) {
                const int place = S->place;
                const std::string part = S->part;
                const GeneCode c = geneCodeOf(E.genesis);
                g.testEditorSet(place, false, runReading(c, c.shape, 0, 2) + part.substr(2));
                pressKey(g, in, KEY_W);
                startsThere = g.testKeep().open && g.testKeep().write && g.testKeep().sel == 1 && !E.writing;
                pressKey(g, in, KEY_ESCAPE);
                cancelled = !g.testKeep().open && !E.writing && g.state == GameState::EDITOR;
                pressKey(g, in, KEY_W); pressKey(g, in, KEY_ENTER);
                prefilled = g.state == GameState::TEXT_ENTRY && g.testTextKind() == TEXT_LINE && g.testTextBuffer() == "ROD E1";
                for (const int k : {KEY_BACKSPACE, KEY_BACKSPACE, KEY_D, KEY_A, KEY_R, KEY_K}) pressKey(g, in, k);
                typed = g.testTextBuffer();
                pressKey(g, in, KEY_ENTER);
                writing = E.writing && E.writeName == "ROD DARK" && E.writeFolder == mine && g.state == GameState::EDITOR;
                std::string lines;
                g.testEditorSave(lines);
                g.testEditor() = EditorState();
                std::istringstream ls(lines);
                for (std::string l; std::getline(ls, l);) g.testEditorLoad(l);
                saved = g.testEditor().writing && g.testEditor().writeName == "ROD DARK" && g.testEditor().writeFolder == mine;
                g.closeScreen();
                g.testLabSkip(EDIT_S + 1);
                named = g.guide.names.count(key + "/S1") && g.guide.names[key + "/S1"] == "ROD DARK";
                const Flask& F = g.testLab().flasks[1];
                int inMine = 0, copies = 0;
                for (const KeptCulture& K : g.testLab().kept) if (K.who.sameSpecies(X) && K.who.strain == 1) { copies++; inMine += K.folder == mine; }
                kept = F.n == 1 && F.who[0].strain == 1 && inMine == 1;
                for (int t = 0; t < 40; t++) g.testLabSkip(30);   // grown past its lag: the flask keeps no second copy
                copies = 0;
                for (const KeptCulture& K : g.testLab().kept) copies += K.who.sameSpecies(X) && K.who.strain == 1;
                once = copies == 1;
                // a new folder for the next line, then its name; an empty name refused
                g.testEditorPut(X, best, true);
                g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
                g.testEditorSet(place, true, part);
                pressKey(g, in, KEY_W); pressKey(g, in, KEY_N);
                const bool folderAsked = g.state == GameState::TEXT_ENTRY && g.testTextKind() == TEXT_FOLDER;
                for (const int k : {KEY_N, KEY_E, KEY_W}) pressKey(g, in, k);
                pressKey(g, in, KEY_ENTER);
                const bool lineAsked = g.state == GameState::TEXT_ENTRY && g.testTextKind() == TEXT_LINE && g.testTextBuffer() == "ROD E2";
                int made = -1;
                for (const CultureFolder& f : g.testLab().folders) if (f.name == "NEW") made = f.id;
                newFolder = folderAsked && lineAsked && made > 0 && g.testEditor().writeFolder == made;
                for (int i = 0; i < 28; i++) pressKey(g, in, KEY_BACKSPACE);
                pressKey(g, in, KEY_ENTER);
                emptyRefused = g.state == GameState::TEXT_ENTRY && g.testTextKind() == TEXT_LINE && !g.testEditor().writing;
            }
        }
        check("genes: W asks for the new strain's folder, then its name, the parent's to start (R-445)",
              startsThere && cancelled && prefilled && writing && saved && named && kept && once && newFolder && emptyRefused,
              fmt("folders open at the parent's %s; Esc writes nothing %s; the name entry holds ROD E1 %s, typed '%s'; writing %s; over a save "
                  "%s; named ROD DARK %s; in flask 2 and MINE %s, one copy once grown %s; a new folder, then its name (ROD E2) %s; an empty name refused %s",
                  startsThere ? "yes" : "NO", cancelled ? "yes" : "NO", prefilled ? "yes" : "NO", typed.c_str(), writing ? "yes" : "NO",
                  saved ? "yes" : "NO", named ? "yes" : "NO", kept ? "yes" : "NO", once ? "yes" : "NO", newFolder ? "yes" : "NO",
                  emptyRefused ? "yes" : "NO"));
        remove(GUIDE);
    }
    {   // R-450 (Y-02's learning by experiment before it): a gene is known for its species the moment it is changed. Of a six letters' world
        // (ours), a species on the editor: its warmth NOT KNOWN, then changed by hand: WARMTH at once, before anything is written; the letters
        // open, the guide's choice for each letter at the cursor, the current one as the line has it and heavier letters warmer; a kin of its
        // world and genesis still NOT KNOWN there. Its vital gene cut: VITAL at once, the line would die and W refuses it. Undone, the warmth
        // line written into a flask past its parent's top warmth: its flask says what it saw with its warmth gene. Of an eight letters' world:
        // a strain of a hotter top read on the sequencer: its warmth gene learned for its species. A guide's lines by genesis (the old way, a
        // `gene` line and a gene's name) are dropped as it loads
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        bool atOnce = false, guided = false, perSpecies = false, vital = false, flaskSays = false, strain = false, dropped = false;
        std::string choices, refusal, flaskSaid, strainSaid;
        auto speciesOf = [&](const GeneWorld& w, FlaskSpecies& X, std::string& key, int skip) {
            for (const Microbe& m : lifeOf(w.sys)->bodies[w.body].species) {
                if (m.state != SS_LIVING || (m.cycle & LC_DORMANT) || m.index == skip) continue;
                X.sx = w.sys.star.sx; X.sy = w.sys.star.sy; X.sz = w.sys.star.sz; X.body = w.body; X.species = m.index; X.strain = X.strainIn = 0;
                CultureKind k; Morph f; MicrobeGenes ge;
                if (!g.testFlaskKind(X, k, f, &ge) || !geneOfKind(ge, GN_WARMTH) || !geneOfKind(ge, GN_VITAL) || growthIn(k, bestOf(k)).fit != GF_GROWS) continue;
                key = Guide::bodyKey(X.sx, X.sy, X.sz, w.body) + "/M" + std::to_string(m.index);
                g.guide.microbes[key].seen |= SEEN_FORM | SEEN_KIN;
                return true;
            }
            return false;
        };
        FlaskSpecies X, Z; std::string key, zkey;
        if (W[2].body >= 0 && speciesOf(W[2], X, key, -1)) {
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(X, k0, f0, &g0);
            const GeneCode c = geneCodeOf(g0.genesis);
            const MicrobeGene* Wm = geneOfKind(g0, GN_WARMTH);
            const MicrobeGene* V = geneOfKind(g0, GN_VITAL);
            const Medium best = bestOf(k0);
            g.testEditorPut(X, best, true);
            EditorState& E = g.testEditor();
            const bool before = g.testGeneWords(X, Wm->place) == "NOT KNOWN";
            g.testEditorSet(Wm->place, false, heavier(c, Wm->part, 3));
            atOnce = before && g.testGeneWords(X, Wm->place) == "WARMTH" && !E.writing;
            // its letters open at the first letter: what each letter of the alphabet makes of it there
            E.sel = Wm->place; E.letters = true; E.cursor = 0;
            const std::vector<std::string> alt = g.testEditorLetterChoices();
            const MicrobeGene* H = E.editedGenes.at(Wm->place);
            const char* abc = alphabetOf(c.letters);
            int cur = -1, lightest = -1, heaviest = -1;
            for (int i = 0; abc[i]; i++) {
                if (H && H->part[0] == abc[i]) cur = i;
                if (lightest < 0 || c.weight[i] < c.weight[lightest]) lightest = i;
                if (heaviest < 0 || c.weight[i] > c.weight[heaviest]) heaviest = i;
                choices += fmt("%s%c %s", i ? ", " : "", abc[i], i < (int)alt.size() ? alt[i].c_str() : "?");
            }
            auto bestC = [](const std::string& t) { return t.compare(0, 5, "BEST ") == 0 ? atof(t.c_str() + 5) : -999.0; };
            guided = (int)alt.size() == c.letters && cur >= 0 && H && alt[cur] == geneValueShort(E.editedKind, E.editedForm, *H, E.code) &&
                     bestC(alt[heaviest]) > bestC(alt[lightest]);
            E.letters = false;
            for (const Microbe& m : lifeOf(W[2].sys)->bodies[W[2].body].species) {   // a kin of its world and genesis, its warmth gene at the same place
                FlaskSpecies K = X; K.species = m.index;
                CultureKind kk; Morph kf; MicrobeGenes kg;
                if (m.index == X.species || m.state != SS_LIVING || !g.testFlaskKind(K, kk, kf, &kg) || !kg.valid || kg.genesis.id != g0.genesis.id) continue;
                if (const MicrobeGene* KW = kg.at(Wm->place)) if (KW->kind == GN_WARMTH) { Z = K; break; }
            }
            perSpecies = Z.species >= 0 && Z.species != X.species && g.testGeneWords(Z, Wm->place) == "NOT KNOWN";
            // its vital gene cut: known at once, the line would die, W refused
            g.testEditorSet(V->place, true, V->part);
            const bool dies = E.editedKind.dead && g.testGeneWords(X, V->place) == "VITAL";
            g.testEditorWrite();
            refusal = g.testStatus();
            vital = dies && !E.writing && refusal.find("THE STRAIN WOULD DIE") != std::string::npos;
            g.testEditorSet(V->place, false, V->part);   // put back in: only its warmth changed
            g.testEditorWrite();
            g.testLabSkip(EDIT_S + 1);
            int f = -1;
            for (int i = 0; i < INCUBATOR_FLASKS; i++) if (g.testLab().flasks[i].n == 1 && g.testLab().flasks[i].who[0].strain > 0 && g.testLab().flasks[i].who[0].sameSpecies(X)) f = i;
            Medium hot = best; hot.tempC = std::floor(k0.s.tMax) + 1;
            if (f >= 0) { g.testFlaskSet(f, hot); g.testLabSkip(6); flaskSaid = lastGene(g); }
            flaskSays = f >= 0 && flaskSaid.find(fmt("GENE %d: WARMTH GENE", Wm->place + 1)) != std::string::npos;
        }
        FlaskSpecies Y; std::string ykey;
        if (W[4].body >= 0 && speciesOf(W[4], Y, ykey, -1)) {
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(Y, k0, f0, &g0);
            StrainRecord r; r.what = SM_HOT; r.value = k0.s.tMax + 5;
            g.guide.strains[ykey + "/S1"] = r;
            FlaskSpecies line = Y; line.strain = line.strainIn = 1;
            g.testSeqPutLine(line);
            g.testLabSkip(SEQUENCE_S + 1);
            const MicrobeGene* Wm = geneOfKind(g0, GN_WARMTH);
            for (const std::string& l : g.testLab().seq.result) if (l.find("DIFFERS FROM THE PARENT'S") != std::string::npos) strainSaid = l;
            strain = Wm && g.testGeneWords(Y, Wm->place) == "WARMTH" && strainSaid.find(": WARMTH") != std::string::npos;
        }
        {   // an older guide's gene line and gene name, by genesis: dropped
            const std::string OLD = "shots/tests/unit_genes_old_guide.txt";
            g.guide.save(OLD);
            { std::ofstream o(OLD, std::ios::app); o << "gene 00000000deadbeef/3 2 1\nname G00000000deadbeef/3 MY GENE\n"; }
            Guide h; h.load(OLD);
            dropped = !h.genes.count("00000000deadbeef/3") && !h.names.count("G00000000deadbeef/3") && h.genes.size() == g.guide.genes.size();
            remove(OLD.c_str());
        }
        check("genes: a gene known for its species the moment it is changed; its letters' guide; a vital change refused (R-450)",
              atOnce && guided && perSpecies && vital && flaskSays && strain && dropped,
              fmt("its warmth changed: %s; its letters' guide %s (%s); its kin M%d %s; its vital gene cut: %s ('%s'); its flask past its parent's top %s ('%s'); "
                  "a strain read: %s ('%s'); an older guide's lines %s",
                  atOnce ? "WARMTH at once" : "NOT KNOWN", guided ? "right" : "WRONG", choices.c_str(), Z.species, perSpecies ? "not known" : "KNOWN TOO",
                  vital ? "refused" : "NOT REFUSED", refusal.c_str(), flaskSays ? "said it" : "DID NOT SAY IT", flaskSaid.c_str(), strain ? "its warmth learned" : "WRONG",
                  strainSaid.c_str(), dropped ? "dropped" : "KEPT"));
        remove(GUIDE);
    }
    {   // Y-02: over a save and a load: the editor (its culture, its edits, its writing begun), a line's flask and its record; the writing ends after
        // the load as before it
        const std::string SAVE = "shots/tests/unit_genes_save_1.txt";
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        bool kept = false, ends = false;
        std::string detail = "no world";
        if (W[0].body >= 0) {
            FlaskSpecies X; X.sx = W[0].sys.star.sx; X.sy = W[0].sys.star.sy; X.sz = W[0].sys.star.sz; X.body = W[0].body;
            for (const Microbe& m : lifeOf(W[0].sys)->bodies[W[0].body].species) if (m.state == SS_LIVING) { X.species = m.index; break; }
            const std::string key = Guide::bodyKey(X.sx, X.sy, X.sz, X.body) + "/M" + std::to_string(X.species);
            g.guide.microbes[key].seen |= SEEN_FORM | SEEN_KIN;
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(X, k0, f0, &g0);
            const MicrobeGene* Wm = geneOfKind(g0, GN_WARMTH);
            const GeneCode c = geneCodeOf(g0.genesis);
            g.testEditorPut(X, bestOf(k0), true);
            g.testEditorSet(Wm->place, false, heavier(c, Wm->part, 2));
            g.testEditorWrite();
            g.testLabSkip(EDIT_S + 1);   // E1 in a flask
            g.testEditorSet(Wm->place, false, heavier(c, Wm->part, 1));
            g.testEditorWrite();
            g.testLabSkip(EDIT_S / 2);   // E2 writing, half its time to go
            g.save(SAVE);
            const EditorState E = g.testEditor();
            CultureNow before = g.testFlaskNow(0);
            TestGame h;
            h.savePrefix = g.savePrefix; h.settingsPath = g.settingsPath; h.guidePath = GUIDE; h.keysPath = g.keysPath;
            h.newGame(); h.guide.load(GUIDE); h.load(SAVE);
            const EditorState& F = h.testEditor();
            CultureNow after = h.testFlaskNow(0);
            kept = F.on && F.who.sameSpecies(E.who) && F.who.strain == E.who.strain && F.writing && F.t0 == E.t0 && geneEditsText(F.edits) == geneEditsText(E.edits) &&
                   geneEditsText(F.writ) == geneEditsText(E.writ) && F.from == E.from && F.sel == E.sel && h.testLab().flasks[0].who[0].strain == 1 &&
                   h.guide.strains.count(key + "/S1") && h.guide.strains[key + "/S1"].edits == g.guide.strains[key + "/S1"].edits &&
                   std::fabs(after.one[0].cells - before.one[0].cells) <= 1e-9 * std::max(1.0, before.one[0].cells) && after.one[0].stage == before.one[0].stage;
            g.testLabSkip(EDIT_S); h.testLabSkip(EDIT_S);
            ends = !g.testEditor().writing && !h.testEditor().writing && g.guide.strains.count(key + "/S2") && h.guide.strains.count(key + "/S2") &&
                   g.guide.strains[key + "/S2"].edits == h.guide.strains[key + "/S2"].edits && h.testLab().flasks[1].who[0].strain == 2;
            detail = fmt("the editor %s (edits %s, its writing begun at %.0f s); flask 1's E1 %s; the writing's end after the load %s", kept ? "as saved" : "WRONG",
                         geneEditsText(F.edits).c_str(), E.t0, h.testLab().flasks[0].who[0].strain == 1 ? "as saved" : "WRONG", ends ? "as before it" : "WRONG");
        }
        check("genes: the editor, its lines and their flasks over a save and a load (Y-02)", kept && ends, detail);
        remove(GUIDE); remove(SAVE.c_str());
    }
    {   // Y-03 (R-443's genes from others): the kin beside it on the editor (K: the species read before the cultures, its own world's first,
        // another life marked); another alphabet, the other hand and another life's map refused with their words; M its kin's warmth gene in
        // (its letters and its range its kin's, from its world's kin) and a gene into a gap (its kin's letters); W writes the line, its donor's
        // line drawn on its world's tree from the kin's leaf to its twig
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        bool listed = false, marked = false, beside = false, alphabet = false, hand = false, life = false, warmth = false, gap = false, drawn = false;
        std::string words[3], detail = "no worlds", treeDetail = "no line";
        std::vector<Cult> cs;
        if (W[0].body >= 0) cs = culturesOf(g, W[0].sys, W[0].body);
        const Cult* A = nullptr; const Cult* D = nullptr; int gapAt = -1;
        for (const Cult& a : cs) {   // A grows at its best; D its kin with its warmth's letters other and a gene where A has none
            if (A || growthIn(a.k, bestOf(a.k)).fit != GF_GROWS) continue;
            for (const Cult& d : cs) {
                const MicrobeGene* wa = geneOfKind(a.g, GN_WARMTH);
                const MicrobeGene* wd = geneOfKind(d.g, GN_WARMTH);
                if (&d == &a || !wa || !wd || wa->part == wd->part) continue;
                int p = -1;
                for (const MicrobeGene& G : d.g.genes) if (p < 0 && !a.g.at(G.place) && G.kind != GN_QUIET) p = G.place;
                if (p >= 0) { A = &a; D = &d; gapAt = p; break; }
            }
        }
        std::vector<Cult> six, mirrored, other;
        if (W[2].body >= 0) six = culturesOf(g, W[2].sys, W[2].body);
        if (W[1].body >= 0) mirrored = culturesOf(g, W[1].sys, W[1].body);
        if (A) for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return other.empty(); })) {   // four letters, ours, another genesis
            auto L = lifeOf(sys);
            for (int b = 0; b < (int)sys.bodies.size() && other.empty(); b++) {
                const Biota& B = L->bodies[b];
                if (livingStage(B.stage) && B.genesis.id != 0 && B.genesis.letters == 4 && !B.genesis.mirrored && B.genesis.id != A->g.genesis.id) other = culturesOf(g, sys, b);
            }
        }
        if (A && !six.empty() && !mirrored.empty() && !other.empty()) {
            for (const Cult* c : std::initializer_list<const Cult*>{A, D, &six[0], &mirrored[0], &other[0]}) readIt(g, *c);
            putIn(g, 0, A->w, bestOf(A->k));
            g.testEditorPut(A->w, bestOf(A->k), true);
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            pressKey(g, in, KEY_K);
            EditorState& E = g.testEditor();
            const std::string home = worldKey(A->key);
            bool dRow = false;
            for (const EditorChoice& c : E.choices) {
                if (c.who.sameSpecies(D->w) && c.who.strain == 0) dRow = true;
                if (c.who.sameSpecies(six[0].w)) marked = c.line.find("(ANOTHER LIFE)") != std::string::npos;
            }
            listed = E.choosing && E.choosingFor == EC_KIN && !E.choices.empty() && worldKey(g.testFlaskKey(E.choices[0].who)) == home && dRow;
            E.choosing = false;
            g.testClearStatus(); alphabet = !g.testEditorKin(six[0].w) && !E.kinOn; words[0] = g.testStatus();
            g.testClearStatus(); hand = !g.testEditorKin(mirrored[0].w) && !E.kinOn; words[1] = g.testStatus();
            g.testClearStatus(); life = !g.testEditorKin(other[0].w) && !E.kinOn; words[2] = g.testStatus();
            alphabet = alphabet && words[0].compare(0, 23, "DIFFERENT GENE ALPHABET") == 0;
            hand = hand && words[1].compare(0, 18, "MIRRORED CHEMISTRY") == 0;
            life = life && words[2].compare(0, 22, "ANOTHER ORIGIN OF LIFE") == 0;
            beside = g.testEditorKin(D->w) && E.kinOn && E.kin.sameSpecies(D->w) && E.kinAlike > 0 && E.kinAlike <= 1;
            const MicrobeGene* wa = geneOfKind(A->g, GN_WARMTH);
            const MicrobeGene* wd = geneOfKind(D->g, GN_WARMTH);
            E.sel = wa->place;
            pressKey(g, in, KEY_M);
            const MicrobeGene* H = E.editedGenes.at(wa->place);
            warmth = E.edits.size() == 1 && E.edits[0].how == GX_MOVED && E.edits[0].read && E.edits[0].far == GFROM_WORLD && H && H->part == wd->part &&
                     E.editedKind.s.tMin == D->k.s.tMin && E.editedKind.s.tOpt == D->k.s.tOpt && E.editedKind.s.tMax == D->k.s.tMax;
            E.sel = gapAt;
            pressKey(g, in, KEY_M);
            const MicrobeGene* Gp = E.editedGenes.at(gapAt);
            gap = E.edits.size() == 2 && Gp && !Gp->cut && Gp->part == D->g.at(gapAt)->part && Gp->from == GFROM_WORLD;
            detail = fmt("%s M%d beside M%d (%.0f%% alike); its kin's rows %s, another life's %s; refused: %s / %s / %s; its warmth moved in %s (%.0f to %.0f C, its kin's "
                         "%.0f to %.0f), gene %d (%s) into a gap %s", W[0].sys.bodies[W[0].body].name.c_str(), A->w.species, D->w.species, 100 * E.kinAlike,
                         listed ? "its world's first" : "WRONG", marked ? "marked" : "NOT MARKED", words[0].c_str(), words[1].c_str(), words[2].c_str(),
                         warmth ? "as its kin's" : "WRONG", E.editedKind.s.tMin, E.editedKind.s.tMax, D->k.s.tMin, D->k.s.tMax, gapAt + 1,
                         GENE_KIND_NAMES[geneCodeOf(A->g.genesis).kind[gapAt]], gap ? "its kin's" : "WRONG");
            // written: its line on its world's tree, the kin's leaf to its twig
            pressKey(g, in, KEY_W); pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_ENTER);   // R-445: its folder, its name (its parent's)
            g.closeScreen();
            g.testLabSkip(EDIT_S + 1);
            const auto rec = g.guide.strains.find(A->key + "/S1");
            const bool recorded = rec != g.guide.strains.end() && rec->second.edits.find('+') != std::string::npos && rec->second.edits.find("@" + D->key) != std::string::npos;
            TreeState& T = g.testTree();
            g.testTreeShow(A->key);
            const int dr = rowOf(T, D->key), lr = rowOf(T, A->key + "/S1");
            for (const auto& link : T.donors) drawn = drawn || (link.first == dr && link.second == lr);
            drawn = drawn && recorded && dr >= 0 && lr >= 0;
            treeDetail = fmt("its record %s; its world's page: the kin at row %d, the line at %d; %zu donor's lines %s", rec != g.guide.strains.end() ? rec->second.edits.c_str() : "NONE",
                             dr, lr, T.donors.size(), drawn ? "the kin's to the line's" : "WRONG");
        }
        check("genes: a kin beside it on the editor, another life refused with its words; M moves its kin's gene in, over its own and into a gap (Y-03)",
              listed && marked && alphabet && hand && life && beside && warmth && gap, detail);
        check("genes: a moved gene's line on its world's tree, from its donor's leaf to the line's twig (Y-03)", drawn, treeDetail);
        remove(GUIDE);
    }
    {   // Y-03: the price of a gene not its own, by its donor's distance. A genesis's species on a world, a kin there, one on another world of its
        // system and one on another star: a line of one gene from each (a quiet one, a sense, a shape or a rest: nothing else it grows by changed)
        // doubles 1/0.97, 1/0.94 and 1/0.90 times as slowly as the same line at no price; the star's line beside its parent in a flask from the
        // start loses (the parent leads, the line falls: Gause)
        TestGame g;
        labGame(g);
        std::vector<StarSystem> systems;
        std::map<uint64_t, std::vector<std::pair<int, int>>> at;   // a genesis's living worlds: (system, body)
        bool found = false;
        int sa = -1, ba = -1, bs = -1, ss = -1, bt = -1;
        for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return !found; })) {
            auto L = lifeOf(sys);
            bool kept = false;
            for (int b = 0; b < (int)sys.bodies.size(); b++) {
                const Biota& B = L->bodies[b];
                int living = 0;
                for (const Microbe& m : B.species) living += m.state == SS_LIVING;
                if (!livingStage(B.stage) || B.genesis.id == 0 || living < 1) continue;
                if (!kept) { systems.push_back(sys); kept = true; }
                at[B.genesis.id].push_back({(int)systems.size() - 1, b});
            }
            for (auto& kv : at) {
                if (found) break;
                for (auto& w1 : kv.second) {
                    int alive = 0;
                    for (const Microbe& m : lifeOf(systems[w1.first])->bodies[w1.second].species) alive += m.state == SS_LIVING;
                    if (alive < 2) continue;
                    int s2 = -1, b2 = -1, s3 = -1, b3 = -1;
                    for (auto& w2 : kv.second) {
                        if (w2.first == w1.first && w2.second != w1.second && b2 < 0) { s2 = w2.first; b2 = w2.second; }
                        if (w2.first != w1.first && b3 < 0) { s3 = w2.first; b3 = w2.second; }
                    }
                    if (b2 >= 0 && b3 >= 0) { sa = w1.first; ba = w1.second; bs = b2; ss = s3; bt = b3; found = true; (void)s2; break; }
                }
            }
        }
        bool prices = false, flask = false;
        std::string detail = "no genesis on two worlds of a system and another star";
        if (found) {
            const std::vector<Cult> home = culturesOf(g, systems[sa], ba), sysKin = culturesOf(g, systems[sa], bs), starKin = culturesOf(g, systems[ss], bt);
            auto inert = [](int kind) { return kind == GN_QUIET || kind == GN_SENSE || kind == GN_SHAPE || kind == GN_REST; };
            // a place where the donor's gene of those kinds differs from A's (or A has none)
            auto placeOf = [&](const Cult& a, const Cult& d) {
                for (const MicrobeGene& G : d.g.genes) {
                    const MicrobeGene* H = a.g.at(G.place);
                    if (inert(G.kind) && (!H || H->part != G.part)) return G.place;
                }
                return -1;
            };
            const Cult* A = nullptr; const Cult* D[3] = {nullptr, nullptr, nullptr}; int P[3] = {-1, -1, -1};
            for (const Cult& a : home) {
                if (A || growthIn(a.k, bestOf(a.k)).fit != GF_GROWS) continue;
                const Cult* d[3] = {nullptr, nullptr, nullptr}; int q[3] = {-1, -1, -1};
                for (const Cult& c : home) if (&c != &a && !d[0] && (q[0] = placeOf(a, c)) >= 0) d[0] = &c;
                for (const Cult& c : sysKin) if (!d[1] && (q[1] = placeOf(a, c)) >= 0) d[1] = &c;
                for (const Cult& c : starKin) if (!d[2] && (q[2] = placeOf(a, c)) >= 0) d[2] = &c;
                if (d[0] && d[1] && d[2]) { A = &a; for (int i = 0; i < 3; i++) { D[i] = d[i]; P[i] = q[i]; } }
            }
            if (A) {
                const char* far[3] = {"its world's", "its system's", "another star's"};
                double burden[3] = {}, slower[3] = {};
                int ok = 0;
                detail.clear();
                for (int i = 0; i < 3; i++) {
                    StrainRecord r; r.what = SM_EDIT; r.edits = fmt("%d+%s@%s", P[i] + 1, D[i]->g.at(P[i])->part.c_str(), D[i]->key.c_str());
                    g.guide.strains[A->key + "/S" + std::to_string(i + 1)] = r;
                    FlaskSpecies line = A->w; line.strain = line.strainIn = i + 1;
                    CultureKind k; Morph f; MicrobeGenes ge;
                    g.testFlaskKind(line, k, f, &ge);
                    CultureKind free = k; free.burden = 0;
                    burden[i] = k.burden; slower[i] = bestDoublingS(k) / bestDoublingS(free);
                    const double price = GENE_PRICE[i + 1];
                    ok += std::fabs(burden[i] - price) < 1e-12 && std::fabs(slower[i] - 1 / (1 - price)) < 1e-9 && ge.at(P[i]) && ge.at(P[i])->from == i + 1;
                    detail += fmt("%sgene %d (%s) from %s kin: %.0f%%, doubling x%.4f", i ? "; " : "", P[i] + 1, GENE_KIND_NAMES[geneCodeOf(A->g.genesis).kind[P[i]]], far[i],
                                  100 * burden[i], slower[i]);
                }
                prices = ok == 3;
                // two in a flask from the start: its parent and the star's line
                Flask& F = g.testLab().flasks[0];
                F = Flask(); F.n = 2; F.who[0] = A->w; F.who[1] = A->w; F.who[1].strain = F.who[1].strainIn = 3;
                F.medium = F.asWorld = bestOf(A->k);
                CultureEvent e0; e0.kind = CD_START; e0.t = g.testLab().clock; e0.m = F.medium;
                CultureEvent e1 = e0; e1.kind = CD_ADD;
                F.events.push_back(e0); F.events.push_back(e1);
                g.testLabSkip(0);
                const CultureNow now = g.testFlaskNow(0);
                flask = F.made && F.run.pair == PA_COMPETE && F.run.lead == 0 && F.run.exT >= 0;
                detail += fmt("; beside its parent in a flask: %s (the parent %s, its fall from %.0f s)", flask ? "it loses" : "WRONG", F.run.lead == 0 ? "leads" : "DOES NOT LEAD",
                              F.run.exT - e0.t);
                (void)now;
            } else detail = "no species with kin of those kinds at the three distances";
        }
        check("genes: a gene not its own slows its line by its donor's distance: its doubling, and beside its parent in a flask (Y-03)", prices && flask, detail);
        remove(GUIDE);
    }
    {   // Y-03: D copies a gene to the first free place of its kind (a sense of a swimmer with a place to spare): its letters its gene's, nothing
        // changed; changed by hand it turns to something new; written, the line's culture turns to it too. A gene of a kind with no free place
        // (its warmth) refused
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        const Cult* X = nullptr; const MicrobeGene* S = nullptr;
        std::vector<Cult> all[6];
        for (int i = 0; i < 6 && !X; i++) {
            if (W[i].body < 0) continue;
            all[i] = culturesOf(g, W[i].sys, W[i].body);
            for (const Cult& c : all[i]) {
                if (X || c.k.s.motion == MO_NONE || growthIn(c.k, bestOf(c.k)).fit != GF_GROWS) continue;
                int senses = 0; const MicrobeGene* s1 = nullptr;
                for (const MicrobeGene& G : c.g.genes) if (G.kind == GN_SENSE) { senses++; if (!s1) s1 = &G; }
                if (s1 && senses < 3) { X = &c; S = s1; }
            }
        }
        bool placed = false, same = false, turned = false, refused = false, written = false;
        std::string detail = "no swimmer with a sense and a place to spare", refusedWords;
        if (X) {
            readIt(g, *X);
            putIn(g, 0, X->w, bestOf(X->k));
            g.testEditorPut(X->w, bestOf(X->k), true);
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            EditorState& E = g.testEditor();
            const GeneCode c = geneCodeOf(X->g.genesis);
            int first = -1;
            for (int p = 0; p < GENE_PLACES && first < 0; p++) if (c.kind[p] == GN_SENSE && !X->g.at(p)) first = p;
            E.sel = S->place;
            pressKey(g, in, KEY_D);
            const GeneEdit* cp = nullptr;
            for (const GeneEdit& e : E.edits) if (e.how == GX_COPY) cp = &e;
            placed = cp && cp->place == first && cp->from == S->place && cp->part == S->part && E.sel == first && E.editedGenes.at(first);
            same = placed && E.editedKind.s.taxis == X->k.s.taxis && fleesLight(E.editedKind.s) == fleesLight(X->k.s);
            int t = -1;
            for (int want : {SE_FOOD, SE_LIGHT, SE_FIELD}) if (t < 0 && !(X->k.s.taxis & senseBitOf(want))) t = want;
            if (placed && t >= 0) {
                g.testEditorSet(first, false, runReading(c, c.sense, t, 2));
                turned = (E.editedKind.s.taxis & senseBitOf(t)) != 0 && E.edits.size() == 1 && E.edits[0].how == GX_COPY;
                const size_t before = E.edits.size();
                E.sel = geneOfKind(X->g, GN_WARMTH)->place;
                g.testClearStatus();
                pressKey(g, in, KEY_D);
                refusedWords = g.testStatus();
                refused = E.edits.size() == before && refusedWords.compare(0, 13, "NO FREE PLACE") == 0;
                pressKey(g, in, KEY_W); pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_ENTER);   // R-445: its folder, its name (its parent's)
                g.closeScreen();
                g.testLabSkip(EDIT_S + 1);
                FlaskSpecies line = X->w; line.strain = line.strainIn = 1;
                CultureKind k; Morph f; MicrobeGenes ge;
                const auto rec = g.guide.strains.find(X->key + "/S1");
                written = rec != g.guide.strains.end() && rec->second.edits.find('*') != std::string::npos && g.testFlaskKind(line, k, f, &ge) && (k.s.taxis & senseBitOf(t)) != 0 &&
                          ge.at(first) && ge.at(first)->part == runReading(c, c.sense, t, 2);
            }
            detail = fmt("M%d's gene %d (a sense) copied to %d %s, nothing changed %s; changed by hand to %s %s; its warmth's copy %s (%s); written %s", X->w.species, S->place + 1,
                         first + 1, placed ? "(its kind's first free place)" : "WRONG", same ? "yes" : "NO", t >= 0 ? GENE_SENSE_NAMES[t] : "NONE", turned ? "it turns to it" : "WRONG",
                         refused ? "refused" : "NOT REFUSED", refusedWords.c_str(), written ? "its culture turns to it" : "WRONG");
        }
        check("genes: D copies a gene to a free place of its kind; changed by hand, the line does something new (Y-03)", placed && same && turned && refused && written, detail);
        remove(GUIDE);
    }
    {   // Y-03: what a gene needs. A light sense moved from a swimmer into a still kin, written and grown: its culture turns to the light, its drop's
        // cells answer nothing (a still cell's slide kind has no sense), and with the sense known it is said why. A pigment moved into a chemical-
        // eater colours it and feeds it nothing in the light; a food moved into a light-feeder feeds it nothing
        TestGame g; Input in;
        labGame(g);
        const Cult* A = nullptr; const Cult* D = nullptr; int place = -1;
        const Cult *C = nullptr, *Pg = nullptr, *Lf = nullptr, *Fd = nullptr;
        std::vector<std::vector<Cult>> kept;
        int worlds = 0;
        for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return (!A || !C || !Lf) && worlds < 60; })) {
            auto L = lifeOf(sys);
            for (int b = 0; b < (int)sys.bodies.size(); b++) {
                if (!livingStage(L->bodies[b].stage) || L->bodies[b].genesis.id == 0) continue;
                worlds++;
                kept.push_back(culturesOf(g, sys, b));
                const std::vector<Cult>& cs = kept.back();
                for (const Cult& a : cs) {
                    if (!A && a.k.s.motion == MO_NONE && growthIn(a.k, bestOf(a.k)).fit == GF_GROWS)
                        for (const Cult& d : cs) {
                            if (A || d.k.s.motion == MO_NONE) continue;
                            const GeneCode c = geneCodeOf(d.g.genesis);
                            for (const MicrobeGene& G : d.g.genes) if (!A && G.kind == GN_SENSE && geneTable(c, c.sense, G.part, 0, 2) == SE_LIGHT) { A = &a; D = &d; place = G.place; }
                        }
                    if (!C && a.k.s.way == LW_CHEMICAL && !geneOfKind(a.g, GN_PIGMENT))
                        for (const Cult& d : cs) if (!C && geneOfKind(d.g, GN_PIGMENT) && d.k.s.pigment != PG_NONE) { C = &a; Pg = &d; }
                    if (!Lf && a.k.s.way == LW_LIGHT && !geneOfKind(a.g, GN_FOOD))
                        for (const Cult& d : cs) if (!Lf && geneOfKind(d.g, GN_FOOD)) { Lf = &a; Fd = &d; }
                }
            }
        }
        bool turns = false, still = false, said = false, colours = false, light = false, food = false;
        std::string detail = "no still species with a swimmer kin turned to the light", saidLine;
        if (A) {
            readIt(g, *A); readIt(g, *D);
            g.testEditorPut(A->w, bestOf(A->k), true);
            g.testEditorKin(D->w);
            EditorState& E = g.testEditor();
            E.sel = place;
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            pressKey(g, in, KEY_M);
            g.testEditorWrite();
            g.closeScreen();
            g.testLabSkip(EDIT_S + 1);
            int f = -1;
            for (int i = 0; i < INCUBATOR_FLASKS && f < 0; i++) if (g.testLab().flasks[i].n == 1 && g.testLab().flasks[i].who[0].sameSpecies(A->w) && g.testLab().flasks[i].who[0].strain == 1) f = i;
            GeneKnown k; k.kind = GN_SENSE;
            g.guide.genes[g.testGeneKey(A->w, place)] = k;
            if (f >= 0) {
                turns = (g.testLab().flasks[f].culture.kind[0].s.taxis & TX_LIGHT) != 0 && g.testLab().flasks[f].culture.kind[0].s.motion == MO_NONE;
                g.testLabSkip(150);
                if (g.testScopeStageFlask(f)) {
                    int kinds = 0;
                    still = true;
                    for (const SlideKind& K : g.testScope().mount.kinds) if (K.species == 0 && K.form == CF_CELL) { kinds++; still = still && K.taxis == 0 && !K.flees; }
                    still = still && kinds > 0;
                }
                saidLine = lastGene(g);
                said = saidLine.find("A SENSE DOES NOTHING IN A CELL THAT CANNOT MOVE") != std::string::npos;
            }
            detail = fmt("M%d (still) took M%d's gene %d (to the light): its culture %s, its drop's cells %s; said: %s", A->w.species, D->w.species, place + 1,
                         turns ? "turns to the light" : "WRONG", still ? "answer nothing" : "ANSWER", saidLine.c_str());
        }
        if (C) {   // a pigment in a chemical-eater: its colour, its growth in the light its own
            const int p = geneOfKind(Pg->g, GN_PIGMENT)->place;
            CultureKind k = C->k; Morph f = C->f; MicrobeGenes ge = C->g;
            applyEdits(k, f, ge, {movedFrom(*Pg, p, GFROM_WORLD)});
            Medium m = bestOf(C->k); m.light = CL_WHITE;
            const Growth a = growthIn(C->k, m), b = growthIn(k, m);
            colours = k.s.pigment == Pg->k.s.pigment && f.pigment == Pg->k.s.pigment;
            light = a.fit == b.fit && a.mu == b.mu;
            detail += fmt("; a pigment (%s) moved into a chemical-eater: %s, %s in the light", PIGMENT_NAMES[Pg->k.s.pigment], colours ? "it colours it" : "WRONG",
                          light ? "growing as before" : "FED BY IT");
        } else detail += "; no chemical-eater with a pigmented kin";
        if (Lf) {   // a food in a light-feeder: its growth its own, in the light and on that food in the dark
            const int p = geneOfKind(Fd->g, GN_FOOD)->place;
            CultureKind k = Lf->k; Morph f = Lf->f; MicrobeGenes ge = Lf->g;
            applyEdits(k, f, ge, {movedFrom(*Fd, p, GFROM_WORLD)});
            Medium m = bestOf(Lf->k), dark = m;
            dark.light = CL_DARK; dark.energy = Fd->k.s.substrate;
            const Growth a = growthIn(Lf->k, m), b = growthIn(k, m), c2 = growthIn(k, dark);
            food = a.fit == b.fit && a.mu == b.mu && c2.fit == GF_DARK;
            detail += fmt("; a food (%s) moved into a light-feeder: %s", SUBSTRATE_NAMES[Fd->k.s.substrate], food ? "it feeds it nothing" : "WRONG");
        } else detail += "; no light-feeder with a chemical-eating kin";
        check("genes: what a gene needs: a sense in a still cell answers nothing (said); a pigment or a food in one that lives another way (Y-03)",
              turns && still && said && colours && light && food, detail);
        remove(GUIDE);
    }
    {   // Y-03: over a save and a load: the kin beside it, a gene moved in (its donor read again) and a copy, the edited line the same
        const std::string SAVE = "shots/tests/unit_genes_save_1.txt";
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        bool kept = false;
        std::string detail = "no world";
        std::vector<Cult> cs;
        if (W[0].body >= 0) cs = culturesOf(g, W[0].sys, W[0].body);
        const Cult* A = nullptr; const Cult* D = nullptr;
        for (const Cult& a : cs) for (const Cult& d : cs)
            if (!A && &a != &d && geneOfKind(a.g, GN_WARMTH) && geneOfKind(d.g, GN_WARMTH) && geneOfKind(a.g, GN_WARMTH)->part != geneOfKind(d.g, GN_WARMTH)->part) { A = &a; D = &d; }
        if (A) {
            readIt(g, *A); readIt(g, *D);
            g.testEditorPut(A->w, bestOf(A->k), true);
            g.testEditorKin(D->w);
            EditorState& E = g.testEditor();
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            E.sel = geneOfKind(A->g, GN_WARMTH)->place;
            pressKey(g, in, KEY_M);
            const MicrobeGene* Q = geneOfKind(A->g, GN_QUIET);
            if (Q) { E.sel = Q->place; pressKey(g, in, KEY_D); }   // (a copy where a quiet place is free)
            g.closeScreen();
            g.save(SAVE);
            const EditorState S = g.testEditor();
            TestGame h;
            h.savePrefix = g.savePrefix; h.settingsPath = g.settingsPath; h.guidePath = GUIDE; h.keysPath = g.keysPath;
            h.newGame(); h.load(SAVE);
            h.openScreen(GameState::EDITOR); h.frame(in, 0); in.newFrame();
            const EditorState& F = h.testEditor();
            bool movedRead = false;
            for (const GeneEdit& e : F.edits) movedRead = movedRead || (e.how == GX_MOVED && e.read && e.far == GFROM_WORLD);
            kept = F.on && F.kinOn && F.kin.sameSpecies(S.kin) && F.kin.strain == S.kin.strain && F.kinName == S.kinName && F.kinFrom == S.kinFrom &&
                   geneEditsText(F.edits) == geneEditsText(S.edits) && movedRead && F.editedKind.s.tMin == S.editedKind.s.tMin && F.editedKind.s.tMax == S.editedKind.s.tMax &&
                   F.editedKind.s.taxis == S.editedKind.s.taxis;
            detail = fmt("beside it %s (%s); its edits %s as saved %s, its moved gene's donor read again %s", F.kinOn ? F.kinName.c_str() : "NONE", F.kinFrom.c_str(),
                         geneEditsText(F.edits).c_str(), geneEditsText(F.edits) == geneEditsText(S.edits) ? "yes" : "NO", movedRead ? "yes" : "NO");
            h.closeScreen();
        }
        check("genes: the kin beside it, its moved and copied genes over a save and a load (Y-03)", kept, detail);
        remove(GUIDE); remove(SAVE.c_str());
    }
    {   // Y-04: a switch's condition in the culture's segments, and a gene off costing nothing. A heterotroph of the six worlds given a gene of
        // its world's kin (a quiet gene, a sense, a shape or a rest: 3%): WHEN CROWDED, its growth two laws, its species' rate up to its crowd's
        // quorum share and 1/0.97 as slow past it; IN THE DARK, its species' rate lit and paying in the dark; NEVER, its species' rate lit and dark
        // (the price listed, never paid). Its vital gene IN THE LIGHT: it grows lit, lyses at once in the dark. A spore former's rest WHEN COLD
        // (the warmth against its best), starved (R-449: a flask at its crowd is fed for good): spores at its end 3 C under its best, none 1 C
        // over it (its species' there). A luminous one's glow WHEN
        // STARVING: dark as it grows past its quorum share (its species aglow there), glowing at its crowd
        TestGame g;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        auto inert = [](int kind) { return kind == GN_QUIET || kind == GN_SENSE || kind == GN_SHAPE || kind == GN_REST; };
        const Cult* A = nullptr; const Cult* D = nullptr; int P = -1;
        std::vector<Cult> cs[6];
        for (int i = 0; i < 6 && !A; i++) {
            if (W[i].body < 0) continue;
            cs[i] = culturesOf(g, W[i].sys, W[i].body);
            for (const Cult& a : cs[i]) {
                if (A || a.k.s.way == LW_LIGHT || growthIn(a.k, bestOf(a.k)).fit != GF_GROWS || !geneOfKind(a.g, GN_VITAL)) continue;
                for (const Cult& d : cs[i])
                    for (const MicrobeGene& G : d.g.genes) {
                        const MicrobeGene* H = a.g.at(G.place);
                        if (!A && &d != &a && inert(G.kind) && (!H || H->part != G.part)) { A = &a; D = &d; P = G.place; }
                    }
            }
        }
        bool crowded = false, dark = false, never = false, light = false, cold = false, starving = false;
        std::string detail = "no heterotroph with a world's kin to move a gene from", costs = detail;
        if (A) {
            const std::string moved = fmt("%d+%s@%s", P + 1, D->g.at(P)->part.c_str(), D->key.c_str());
            const CultureKind kc = lineOf(g, *A, 1, moved + "~CROWDED"), kd = lineOf(g, *A, 2, moved + "~DARK"), kn = lineOf(g, *A, 3, moved + "~NEVER");
            Medium lit = bestOf(A->k), unlit = lit;
            lit.light = CL_WHITE; unlit.light = CL_DARK;
            auto muOf = [](const CultureRun& R) { const auto v = growing(R); return v.empty() ? 0.0 : v[0]->mu; };
            auto allAt = [](const CultureRun& R, double mu) { const auto v = growing(R); bool ok = !v.empty(); for (const CultureSeg* x : v) ok = ok && near(x->mu, mu); return ok; };
            const double mu0 = muOf(runCulture(aloneIn(A->k, lit))), muD = muOf(runCulture(aloneIn(A->k, unlit)));
            const CultureRun rc = runCulture(aloneIn(kc, lit));
            const std::vector<const CultureSeg*> gc = growing(rc);
            crowded = gc.size() >= 2 && near(gc[0]->mu, mu0) && near(gc[1]->mu, mu0 * (1 - GENE_PRICE[GFROM_WORLD])) && near(gc[1]->N, kc.quorum * gc[1]->K, 1e-6) &&
                      gc[0]->state >= 0 && !(gc[0]->state & CST_CROWDED) && (gc[1]->state & CST_CROWDED) && kc.switches.size() == 1 &&
                      near(kc.switches[0].price, GENE_PRICE[GFROM_WORLD]);
            const CultureRun rdl = runCulture(aloneIn(kd, lit)), rdd = runCulture(aloneIn(kd, unlit));
            dark = allAt(rdl, mu0) && allAt(rdd, muD * (1 - GENE_PRICE[GFROM_WORLD]));
            never = allAt(runCulture(aloneIn(kn, lit)), mu0) && allAt(runCulture(aloneIn(kn, unlit)), muD) && kn.switches.size() == 1 && near(kn.burden, GENE_PRICE[GFROM_WORLD]);
            costs = fmt("M%d given gene %d of its world's M%d (%.0f%%): IN THE DARK its species' rate lit x%.4f, dark x%.4f %s; NEVER its species' rate lit and dark, its price "
                        "listed %.0f%% %s", A->w.species, P + 1, D->w.species, 100 * GENE_PRICE[GFROM_WORLD], mu0 > 0 ? muOf(rdl) / mu0 : 0.0, muD > 0 ? muOf(rdd) / muD : 0.0,
                        dark ? "ok" : "WRONG", kn.switches.empty() ? 0.0 : 100 * kn.switches[0].price, never ? "ok" : "WRONG");
            const CultureKind kv = lineOf(g, *A, 4, fmt("%d~LIGHT", geneOfKind(A->g, GN_VITAL)->place + 1));
            const FlaskCulture cvl = aloneIn(kv, lit), cvd = aloneIn(kv, unlit);
            const CultureOne vl = cultureAt(cvl, runCulture(cvl), 1800).one[0], vd = cultureAt(cvd, runCulture(cvd), 60).one[0];
            light = vl.cells > 1e4 && vl.stage != SG_DEAD && vd.cells == 0 && vd.fit == GF_UNVIABLE;
            detail = fmt("M%d given gene %d (%s) of its world's M%d: WHEN CROWDED %s (%zu laws, x%.4f past %.1f%% of its crowd); IN THE DARK %s; its vital gene IN THE LIGHT %s "
                         "(%.1e cells lit at 30 min, %.0f dark at a minute)", A->w.species, P + 1, GENE_KIND_NAMES[geneCodeOf(A->g.genesis).kind[P]], D->w.species,
                         crowded ? "ok" : "WRONG", gc.size(), gc.size() >= 2 && gc[0]->mu > 0 ? gc[1]->mu / gc[0]->mu : 0.0, 100 * kc.quorum, dark ? "ok" : "WRONG",
                         light ? "ok" : "WRONG", vl.cells, vd.cells);
            Cult R;
            if (findCult(g, [](const Cult& c) {
                    const Microbe& m = c.k.s;
                    return (m.cycle & (LC_SPORULATING | LC_DORMANT)) && !(m.cycle & LC_GATHERING) && geneOfKind(c.g, GN_REST) && m.tOpt - 3 > m.tMin + 1 &&
                           m.tOpt + 1 < m.tMax - 1 && growthIn(c.k, bestOf(c.k)).fit == GF_GROWS;
                }, R)) {
                Medium under = bestOf(R.k);
                if (R.k.s.way == LW_LIGHT) under.light = CL_DARK; else under.energy = CE_NONE;   // nothing it eats: its hold, then its decline
                Medium over = under;
                under.tempC = R.k.s.tOpt - 3; over.tempC = R.k.s.tOpt + 1;
                const CultureKind kr = lineOf(g, R, 1, fmt("%d~COLD", geneOfKind(R.g, GN_REST)->place + 1));
                auto at4h = [](const CultureKind& k, const Medium& m) { const FlaskCulture c = aloneIn(k, m); return cultureAt(c, runCulture(c), 4 * 3600.0).one[0]; };
                const CultureOne ru = at4h(kr, under), ro = at4h(kr, over), so = at4h(R.k, over);
                cold = ru.spores > 1 && ro.spores < 1 && so.spores > 1;
                detail += fmt("; M%d's rest WHEN COLD: %.1e spores at %.0f C, %.0f at %.0f C (its species %.1e, %s %.1e cells; %s, %.0f to %.0f C) %s", R.w.species, ru.spores,
                              under.tempC, ro.spores, over.tempC, so.spores, CULTURE_STAGE_NAMES[so.stage], so.cells, MICROBE_PLAN_NAMES[R.k.s.plan], R.k.s.tMin, R.k.s.tMax,
                              cold ? "ok" : "WRONG");
            } else detail += "; NO SPORE FORMER";
            Cult G;
            if (findCult(g, [](const Cult& c) { return c.k.s.glows && c.k.s.way != LW_LIGHT && geneOfKind(c.g, GN_GLOW) && growthIn(c.k, bestOf(c.k)).fit == GF_GROWS; }, G)) {
                const CultureKind kg = lineOf(g, G, 1, fmt("%d~STARVING", geneOfKind(G.g, GN_GLOW)->place + 1));
                const FlaskCulture fs = aloneIn(G.k, bestOf(G.k)), fl = aloneIn(kg, bestOf(G.k));
                const CultureRun rs = runCulture(fs), rl = runCulture(fl);
                double tg = -1, tc = -1;
                for (int t = 0; t < 4 * 3600 && (tg < 0 || tc < 0); t += 5) {
                    const CultureOne a = cultureAt(fs, rs, t).one[0], b = cultureAt(fl, rl, t).one[0];
                    if (tg < 0 && a.glowing && (a.stage == SG_GROWING || a.stage == SG_COLONY)) tg = t;
                    if (tc < 0 && b.stage == SG_CROWD) tc = t;
                }
                const CultureOne lg = cultureAt(fl, rl, tg).one[0], lc = cultureAt(fl, rl, tc + 1).one[0];
                starving = tg >= 0 && tc >= 0 && !lg.glowing && (lg.stage == SG_GROWING || lg.stage == SG_COLONY) && lc.glowing && (lc.state & CST_STARVING) && kg.glowAny;
                detail += fmt("; M%d's glow WHEN STARVING: %s at %.0f s as it grows (its species aglow), %s at its crowd %s", G.w.species, lg.glowing ? "GLOWING" : "dark", tg,
                              lc.glowing ? "glowing" : "DARK", starving ? "ok" : "WRONG");
            } else detail += "; NO LUMINOUS ONE";
        }
        check("genes: a switch's condition in the culture's segments: the light, the warmth against its best, starving, its crowd's share (Y-04)",
              crowded && dark && light && cold && starving, detail);
        check("genes: a gene off costs nothing: a gene not its own switched off pays nothing, NEVER nowhere (Y-04)", dark && never, costs);
        remove(GUIDE);
    }
    {   // Y-04: S on the editor and a switched line's drop as its flask has it. A swimmer of the six worlds, read, its motion gene switched by S
        // five times (ALWAYS, NEVER, IN THE LIGHT, IN THE DARK, WHEN CROWDED: WHEN STARVING), kept over a save and a load, written into a flask:
        // its drop as it grows still, at its crowd swimming (its state starving)
        const std::string SAVE = "shots/tests/unit_genes_save_2.txt";
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        const Cult* X = nullptr;
        std::vector<Cult> all[6];
        for (int i = 0; i < 6 && !X; i++) {
            if (W[i].body < 0) continue;
            all[i] = culturesOf(g, W[i].sys, W[i].body);
            for (const Cult& c : all[i])
                if (!X && c.k.s.motion != MO_NONE && geneOfKind(c.g, GN_MOTION) && c.k.s.way != LW_LIGHT && growthIn(c.k, bestOf(c.k)).fit == GF_GROWS) X = &c;
        }
        bool keyed = false, kept = false, still = false, swims = false;
        std::string detail = "no swimmer", text;
        if (X) {
            readIt(g, *X);
            putIn(g, 0, X->w, bestOf(X->k));
            g.testEditorPut(X->w, bestOf(X->k), true);
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            EditorState& E = g.testEditor();
            const int pm = geneOfKind(X->g, GN_MOTION)->place;
            E.sel = pm;
            for (int i = 0; i < 5; i++) pressKey(g, in, KEY_S);
            text = geneEditsText(E.edits);
            keyed = text == fmt("%d~STARVING", pm + 1) && E.editedGenes.at(pm) && E.editedGenes.at(pm)->sw == GW_STARVING && E.editedKind.switches.size() == 1 &&
                    E.editedKind.s.motion == X->k.s.motion;
            g.closeScreen();
            g.save(SAVE);
            {
                TestGame h;
                h.savePrefix = g.savePrefix; h.settingsPath = g.settingsPath; h.guidePath = GUIDE; h.keysPath = g.keysPath;
                h.newGame(); h.load(SAVE);
                h.openScreen(GameState::EDITOR); h.frame(in, 0); in.newFrame();
                kept = geneEditsText(h.testEditor().edits) == text && h.testEditor().editedKind.switches.size() == 1;
                h.closeScreen();
            }
            g.openScreen(GameState::EDITOR); g.frame(in, 0); in.newFrame();
            pressKey(g, in, KEY_W); pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_ENTER);   // R-445: its folder, its name (its parent's)
            g.closeScreen();
            g.testLabSkip(EDIT_S + 1);
            int at = -1;
            for (int i = 0; i < INCUBATOR_FLASKS; i++) if (g.testLab().flasks[i].n == 1 && g.testLab().flasks[i].who[0].strain == 1) at = i;
            int sGrow = -1, sCrowd = -1, mGrow = -1, mCrowd = -1;
            for (int step = 0; at >= 0 && step < 400 && (sGrow < 0 || sCrowd < 0); step++) {
                const CultureOne o = g.testFlaskNow(at).one[0];
                if (sGrow < 0 && o.stage == SG_GROWING && g.testScopeStageFlask(at)) { sGrow = g.testScope().lineState[0]; mGrow = g.testScope().species[0].motion; }
                if (sCrowd < 0 && o.stage == SG_CROWD && g.testScopeStageFlask(at)) { sCrowd = g.testScope().lineState[0]; mCrowd = g.testScope().species[0].motion; }
                g.testLabSkip(10);
            }
            still = sGrow >= 0 && !(sGrow & CST_STARVING) && mGrow == MO_NONE;
            swims = sCrowd >= 0 && (sCrowd & CST_STARVING) && mCrowd == X->k.s.motion;
            detail = fmt("M%d's gene %d (its motion) by S five times: %s %s; over a save and a load %s; written into flask %d: its drop growing %s (state %d), at its crowd %s "
                         "(state %d)", X->w.species, pm + 1, text.c_str(), keyed ? "WHEN STARVING" : "WRONG", kept ? "kept" : "LOST", at + 1, mGrow == MO_NONE ? "still" : "MOVING",
                         sGrow, mCrowd == X->k.s.motion ? "swimming" : "STILL", sCrowd);
        }
        check("genes: S sets a gene's switch on the editor, kept over a save; a switched line's drop as its flask has it (Y-04)", keyed && kept && still && swims, detail);
        remove(GUIDE); remove(SAVE.c_str());
    }
}

// J-01 (2026-11-20, R-448; the unit's `world` group, after `genes`): the world genes in the lab. Each gene the lab shows changed once in a line
// alone in its flask, its look in the flask and learned against its parent: GAS bubbles of METHANE, DYE the water RED, LIFT a scum, SLIME, SHELL a
// CHALK sediment, STOP cut overgrown, BIND clumps, FROST its ice at -4 C, SHIELD growing under the ultraviolet light; POISON and CARRIER with another
// in its flask; the editor's two pages walked by the arrows and turned by Page Down; a line with world genes over a save and a load
namespace {
// the scan region's first two living species of one world a check wants together, as the flasks take them
template <class Want> bool findPair(TestGame& g, Want want, Cult& a, Cult& b) {
    bool found = false;
    for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return !found; })) {
        auto L = lifeOf(sys);
        for (int bi = 0; bi < (int)sys.bodies.size() && !found; bi++) {
            if (!livingStage(L->bodies[bi].stage) || L->bodies[bi].genesis.id == 0) continue;
            const std::vector<Cult> cs = culturesOf(g, sys, bi);
            for (size_t i = 0; i < cs.size() && !found; i++)
                for (size_t j = 0; j < cs.size() && !found; j++)
                    if (i != j && want(cs[i], cs[j])) { a = cs[i]; b = cs[j]; found = true; }
        }
    }
    return found;
}
bool growsIn(const Cult& c, const Medium& m) { return growthIn(c.k, m).fit == GF_GROWS; }
// the flask's GENE finding in the log (its flask's number first), "" none
std::string flaskFinding(const TestGame& g, int i) {
    const std::string at = fmt("  FLASK %d:", i + 1);   // (after the log's date)
    for (auto it = g.guide.log.rbegin(); it != g.guide.log.rend(); ++it) {
        const size_t p = it->kind == "GENE" ? it->text.find(at) : std::string::npos;
        if (p != std::string::npos) return it->text.substr(p + 2);
    }
    return "";
}
// a second species put in a flask with the first (the incubator's add), on its lab's clock
void addIn(TestGame& g, int i, const FlaskSpecies& w) {
    Flask& F = g.testLab().flasks[i];
    F.n = 2; F.who[1] = w; F.who[1].strainIn = w.strain;
    CultureEvent e; e.kind = CD_ADD; e.t = g.testLab().clock; e.m = F.medium; e.who = 1;
    F.events.push_back(e);
    F.made = false;
    g.testLabSkip(0);
}
}   // namespace

void unitLabWorld(const Check& check) {
    {   // J-01: each world gene the lab shows, one change in a line alone in its flask: its look, said with its gene (R-450: known as the editor
        // would have it, from its change)
        TestGame g;
        labGame(g);
        int flask = 0, strain = 1, shown = 0, learned = 0, cases = 0;
        std::string detail;
        auto one = [&](int kind, const std::function<bool(const Cult&)>& want, const std::function<std::string(const GeneCode&, const MicrobeGene&)>& part,
                       const std::function<void(Medium&)>& med, const std::function<bool(const FlaskShow&, const CultureOne&)>& shows, int minutes) {
            cases++;
            Cult c;
            const bool found = findCult(g, [&](const Cult& x) { return geneOfKind(x.g, kind) && want(x); }, c);
            if (!found) { detail += fmt(" [%s: no culture]", GENE_KIND_NAMES[kind]); return; }
            readIt(g, c);
            const GeneCode code = geneCodeOf(c.g.genesis);
            const MicrobeGene* G = geneOfKind(c.g, kind);
            const std::string p = part(code, *G);
            const int n = strain++;
            lineOf(g, c, n, fmt("%d=%s", G->place + 1, p.c_str()));
            g.guide.genes[g.testGeneKey(c.w, G->place)].kind = kind;   // (known since its change on the editor)
            FlaskSpecies line = c.w; line.strain = line.strainIn = n;
            Medium m = bestOf(c.k);
            med(m);
            const int i = flask++;
            putIn(g, i, line, m);
            bool saw = false;
            for (int t = 0; t < minutes; t++) {
                g.testLabSkip(60);
                saw = saw || shows(g.testFlaskShow(i), g.testFlaskNow(i).one[0]);
            }
            const std::string said = flaskFinding(g, i);
            const bool told = said.find(fmt("GENE %d: %s GENE", G->place + 1, GENE_KIND_NAMES[kind])) != std::string::npos;
            shown += saw; learned += told;
            detail += fmt(" [%s, gene %d %s: %s, %s: %s]", c.key.substr(c.key.rfind('/') + 1).c_str(), G->place + 1, GENE_KIND_NAMES[kind], saw ? "shown" : "NOT SHOWN",
                          told ? "said" : "NOT SAID", said.c_str());
        };
        auto table = [](int value) { return [value](const GeneCode& c, const MicrobeGene& G) { return runReading(c, *worldTable(c, G.kind), value, 2) + G.part.substr(2); }; };
        auto heaviest = [](const GeneCode& c, const MicrobeGene& G) { return heavier(c, G.part, 99); };
        auto best = [](Medium&) {};
        auto grows = [](const Cult& x) { return growsIn(x, bestOf(x.k)); };
        one(GN_GAS, [&](const Cult& x) { return grows(x) && !x.k.world.gives(); }, table(WG_METHANE), best,
            [](const FlaskShow& S, const CultureOne&) { return S.bubbles > 0.03 && S.gas == WG_METHANE; }, 30);
        one(GN_DYE, [&](const Cult& x) { return grows(x) && !x.k.world.dyes(); }, table(WD_RED), best,
            [](const FlaskShow& S, const CultureOne&) { return S.dyed > 0.05 && S.dye == WD_RED; }, 30);
        one(GN_LIFT, [&](const Cult& x) { return grows(x) && x.k.world.lift <= 0; }, heaviest, best,
            [](const FlaskShow& S, const CultureOne&) { return S.scum > 0.05; }, 30);
        one(GN_SLIME, [&](const Cult& x) { return grows(x) && x.k.world.slime <= 0; }, heaviest, best,
            [](const FlaskShow& S, const CultureOne&) { return S.slime > 0.05; }, 30);
        one(GN_SHELL, [&](const Cult& x) { return grows(x) && !x.k.world.shells(); }, table(SHL_CHALK), best,
            [](const FlaskShow& S, const CultureOne&) { return S.sediment > 0.05 && S.shell == SHL_CHALK; }, 30);
        one(GN_STOP, grows, [](const GeneCode&, const MicrobeGene&) { return std::string("-"); }, best,
            [](const FlaskShow& S, const CultureOne& o) { return S.overgrown && o.stage == SG_OVERGROWN; }, 60);
        one(GN_BIND, [&](const Cult& x) { return grows(x) && x.k.world.bind <= 0 && (x.k.s.plan == MP_SPHERE || x.k.s.plan == MP_ROD); }, heaviest, best,
            [](const FlaskShow& S, const CultureOne&) { return S.clumps > 0.05; }, 30);
        one(GN_FROST, [&](const Cult& x) { return x.k.chemistry == CH_WATER && x.k.world.frostC <= FROST_NONE + 1 && x.k.s.saltLo <= 1 && x.k.s.tMax > 0; },
            heaviest, [](Medium& m) { m.tempC = -4; m.salt = 0; }, [](const FlaskShow& S, const CultureOne& o) { return S.frozen && o.stage == SG_FROZEN; }, 5);
        one(GN_SHIELD, [&](const Cult& x) { return grows(x) && x.k.world.shield < UV_SHIELD && x.k.s.way != LW_LIGHT; }, heaviest, [](Medium& m) { m.light = CL_UV; },
            [](const FlaskShow&, const CultureOne& o) { return o.stage == SG_GROWING || o.stage == SG_CROWD || o.stage == SG_COLONY; }, 30);
        check("world: each world gene the lab shows, changed once in a line alone, shows in its flask and is said with its gene (J-01, R-450)",
              shown == cases && learned == cases && cases == 9, fmt("%d of %d shown, %d said:%s", shown, cases, learned, detail.c_str()));
        remove(GUIDE);
    }
    {   // J-01: POISON and CARRIER, a line with another of its world in its flask
        TestGame g;
        labGame(g);
        std::string detail;
        bool poisons = false, passes = false;
        Cult a, b;
        auto pairs = [&](const Cult& x, const Cult& y) {
            const Medium m = bestOf(x.k);
            int lead = -1;
            return growsIn(x, m) && growsIn(y, m) && !x.k.world.poisons() && !y.k.world.poisons() && x.k.world.carrier <= 0 && y.k.world.carrier <= 0 &&
                   pairOf(x.k, y.k, lead) == PA_COMPETE && x.k.s.genesis == y.k.s.genesis;
        };
        if (findPair(g, pairs, a, b)) {
            readIt(g, a);
            const GeneCode c = geneCodeOf(a.g.genesis);
            const MicrobeGene* P = geneOfKind(a.g, GN_POISON);
            const MicrobeGene* C = geneOfKind(a.g, GN_CARRIER);
            lineOf(g, a, 1, fmt("%d=%s", P->place + 1, (runReading(c, c.poison, PZ_NERVE, 2) + heavier(c, P->part.substr(2), 99)).c_str()));
            lineOf(g, a, 2, fmt("%d=%s", C->place + 1, heavier(c, C->part, 99).c_str()));
            g.guide.genes[g.testGeneKey(a.w, P->place)].kind = GN_POISON; g.guide.genes[g.testGeneKey(a.w, C->place)].kind = GN_CARRIER;   // (R-450: known since changed)
            for (int n = 1; n <= 2; n++) {
                FlaskSpecies line = a.w; line.strain = line.strainIn = n;
                putIn(g, n - 1, line, bestOf(a.k));
                addIn(g, n - 1, b.w);
            }
            bool killed = false, passed = false;
            for (int t = 0; t < 40; t++) {
                g.testLabSkip(60);
                killed = killed || (g.testFlaskShow(0).poisoned == 1 && g.testFlaskNow(0).one[1].stage == SG_POISONED);
                passed = passed || g.testFlaskShow(1).passedTo == 1;
            }
            poisons = killed && g.testLab().flasks[0].run.pair == PA_POISON && flaskFinding(g, 0).find(fmt("GENE %d: POISON GENE", P->place + 1)) != std::string::npos;
            passes = passed && flaskFinding(g, 1).find(fmt("GENE %d: CARRIER GENE", C->place + 1)) != std::string::npos;
            detail = fmt("%s with %s: its NERVE poison %s, %s (%s); its genes %s, %s (%s)", a.key.substr(a.key.rfind('/') + 1).c_str(), b.key.substr(b.key.rfind('/') + 1).c_str(),
                         killed ? "kills the other" : "KILLS NOTHING", poisons ? "said" : "NOT SAID", flaskFinding(g, 0).c_str(), passed ? "pass to the other" : "PASS NOWHERE",
                         passes ? "said" : "NOT SAID", flaskFinding(g, 1).c_str());
        } else detail = "no pair found";
        check("world: POISON kills the other in its flask, CARRIER passes its genes to it; each said with its gene (J-01, R-450)", poisons && passes, detail);
        remove(GUIDE);
    }
    {   // J-01: the editor's two pages: the arrows walk the four columns across both, Page Down and Page Up turn; a line with a world gene written,
        // its look in its flask, the editor on its world page over a save and a load
        const std::string SAVE = "shots/tests/unit_genes_save_1.txt";
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        bool walked = false, kept = false;
        std::string detail = "no world";
        if (W[0].body >= 0) {
            FlaskSpecies X; X.sx = W[0].sys.star.sx; X.sy = W[0].sys.star.sy; X.sz = W[0].sys.star.sz; X.body = W[0].body;
            CultureKind k0; Morph f0; MicrobeGenes g0;
            for (const Microbe& m : lifeOf(W[0].sys)->bodies[W[0].body].species) {
                if (m.state != SS_LIVING) continue;
                X.species = m.index;
                if (g.testFlaskKind(X, k0, f0, &g0) && !k0.world.dyes() && growthIn(k0, bestOf(k0)).fit == GF_GROWS) break;
            }
            const std::string key = Guide::bodyKey(X.sx, X.sy, X.sz, X.body) + "/M" + std::to_string(X.species);
            g.guide.microbes[key].seen |= SEEN_FORM | SEEN_KIN;
            g.testEditorPut(X, bestOf(k0), true);
            EditorState& E = g.testEditor();
            g.openScreen(GameState::EDITOR);
            E.sel = 0;
            std::vector<int> at;
            for (int k : {KEY_PAGE_DOWN, KEY_RIGHT, KEY_RIGHT, KEY_LEFT, KEY_PAGE_UP, KEY_DOWN, KEY_PAGE_DOWN}) { pressKey(g, in, k); at.push_back(E.sel); }
            walked = at == std::vector<int>{18, 25, 0, 25, 9, 10, 26};   // its world page, its right column, round to the cell's first, back, the cell's page, down, turned
            const GeneCode c = geneCodeOf(g0.genesis);
            const MicrobeGene* D = geneOfKind(g0, GN_DYE);
            g.testEditorSet(D->place, false, runReading(c, c.dye, WD_VIOLET, 2) + D->part.substr(2));
            g.testEditorWrite();
            g.testLabSkip(EDIT_S + 1);   // E1 in a flask
            g.testLabSkip(30 * 60);
            E.sel = D->place;
            g.save(SAVE);
            const FlaskShow S = g.testFlaskShow(0);
            TestGame h;
            h.savePrefix = g.savePrefix; h.settingsPath = g.settingsPath; h.guidePath = GUIDE; h.keysPath = g.keysPath;
            h.newGame(); h.guide.load(GUIDE); h.load(SAVE);
            const FlaskShow T = h.testFlaskShow(0);
            const EditorState& F = h.testEditor();
            kept = S.dyed > 0.05 && S.dye == WD_VIOLET && T.dye == S.dye && std::fabs(T.dyed - S.dyed) < 1e-9 && F.on && F.sel == D->place &&
                   geneEditsText(F.edits) == geneEditsText(E.edits) && h.testLab().flasks[0].culture.kind[0].world.dye == WD_VIOLET;
            detail = fmt("walked %d %d %d %d %d %d %d; gene %d (DYE) VIOLET written: its flask %s %.2f, after a load %s %.2f; the editor on gene %d after it",
                         at[0] + 1, at[1] + 1, at[2] + 1, at[3] + 1, at[4] + 1, at[5] + 1, at[6] + 1, D->place + 1, WORLD_DYE_NAMES[S.dye], S.dyed, WORLD_DYE_NAMES[T.dye], T.dyed,
                         F.sel + 1);
        }
        check("world: the editor's two pages walked and turned; a world gene written, its flask's look and the editor over a save and a load (J-01)", walked && kept, detail);
        remove(GUIDE); remove(SAVE.c_str());
    }
}
