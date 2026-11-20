// The Y series (2026-11-19; the unit's `genes` group, after `incubator`): Y-01, R-444's tree of kin by world and by folder: its pages (the
// ship's own, each world with a species read, each folder holding a culture), a world's species read and its lab's lines as twigs under their
// parents, Backspace taking a line off and A putting it back (kept in the guide), a folder's page what it holds (a strain kept alone in its
// species' place), each page's years, Tab from the sequencer on the page of what it read. Y-02, R-443's gene editor: the genes' laws
// (`geneLaws`), a line written from a flask by the editor's keys into an empty flask and read everywhere a strain is (its culture, its drop, the
// sequencer, the collection, its twig), what a gene does learned (one change, several, a vital death, nothing seen, a strain read), the
// editor and its lines over a save and a load
#include "harness/harness.h"
#include "harness/test_game.h"
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
const MicrobeGene* geneOfKind(const MicrobeGenes& g, int kind) { for (const MicrobeGene& G : g.genes) if (G.kind == kind) return &G; return nullptr; }
// the log's last GENE line (each finding is logged as it is seen, said in its turn after)
std::string lastGene(const TestGame& g) {
    for (auto it = g.guide.log.rbegin(); it != g.guide.log.rend(); ++it) if (it->kind == "GENE") return it->text;
    return "";
}
}   // namespace

void unitLabGenes(const Check& check) {
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
    {   // Y-02 (R-443): the genes' laws, over forty worlds and the six alphabets' and hands' worlds (`geneLaws`)
        const GeneLaws R = geneLaws();
        check("genes: a living world of each alphabet and hand (Y-02)", R.worlds.ok, R.worlds.detail);
        check("genes: deterministic (Y-02)", R.deterministic.ok, R.deterministic.detail);
        check("genes: every species unedited is itself: its values, its culture's segments, its slide's kinds (Y-02)", R.itself.ok, R.itself.detail);
        check("genes: each gene's edit and cut as its code says (Y-02)", R.asCode.ok, R.asCode.detail);
        check("genes: a vital gene's edit kills, a quiet one shows nothing (Y-02)", R.vitalQuiet.ok, R.vitalQuiet.detail);
        check("genes: how it lives and its plan's family never move; its values within their limits (Y-02)", R.fixed.ok, R.fixed.detail);
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
        std::string twigWords, readLine;
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
                written = E.writing;
                g.closeScreen();
                g.testLabSkip(EDIT_S + 1);
                const auto rec = g.guide.strains.find(key + "/S1");
                recorded = rec != g.guide.strains.end() && rec->second.what == SM_EDIT && rec->second.parent == 0 && geneEditsOf(rec->second.edits).size() == 1;
                const Flask& F = g.testLab().flasks[1];
                inFlask = F.n == 1 && F.who[0].sameSpecies(X) && F.who[0].strain == 1 && F.edited[0] && !E.writing;
                cultured = inFlask && F.culture.kind[0].s.plan == MP_SPHERE && F.forms.size() == 1 && F.forms[0].plan == MP_SPHERE && F.parentForm[0].plan == MP_ROD;
                // its drop: spheres, its shape learned (one gene changed)
                g.testLabSkip(30);
                dropped = g.testScopeStageFlask(1) && g.testScope().forms.size() == 1 && g.testScope().forms[0].plan == MP_SPHERE;
                const GeneKnown* gk = g.guide.genes.count(g.testGeneKey(E.genesis.id, S->place)) ? &g.guide.genes[g.testGeneKey(E.genesis.id, S->place)] : nullptr;
                shapeLearned = gk && gk->kind == GN_SHAPE && gk->how == GENE_SEEN_EDIT && g.testGeneWords(E.genesis.id, S->place) == "ITS SHAPE";
                // read on the sequencer: its edits said
                FlaskSpecies line = X; line.strain = line.strainIn = 1;
                g.testSeqPutLine(line);
                g.testLabSkip(SEQUENCE_S + 1);
                for (const std::string& l : g.testLab().seq.result) if (l.compare(0, 7, "EDITED:") == 0) readLine = l;
                readIt = !readLine.empty() && g.testLab().seq.who.strain == 1;
                // kept as it grows; its twig on its world's tree
                for (int t = 0; t < 40; t++) g.testLabSkip(30);
                for (const KeptCulture& K : g.testLab().kept) keptIt = keptIt || (K.who.sameSpecies(X) && K.who.strain == 1);
                TreeState& T = g.testTree();
                g.testTreeShow(key);
                for (const TreeLeaf& L : T.shown) if (L.key == key + "/S1" && L.lab && L.depth == 1) { twig = true; twigWords = L.name; }
                twig = twig && twigWords.compare(0, 4, "E1 -") == 0;
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
    }
    {   // Y-02: what a gene does, learned. Of a six letters' world (ours): a line of its warmth and a quiet gene changed, in a flask past its
        // parent's top warmth: its warmth seen, the two said, nothing learned; one of its warmth alone there: learned (ITS WARMTH, by an edit);
        // one of a vital gene: dead at once, VITAL; one of a quiet gene alone, grown and its drop looked at: NOTHING SEEN. Of an eight letters'
        // world: a strain of a hotter top read on the sequencer: its warmth gene, the one its letters differ in (by a strain read)
        TestGame g; Input in;
        labGame(g);
        GeneWorld W[6];
        geneWorlds(W);
        bool candidates = false, warmth = false, vital = false, nothing = false, strain = false;
        std::string said[4];
        auto speciesOf = [&](const GeneWorld& w, FlaskSpecies& X, std::string& key) {
            for (const Microbe& m : lifeOf(w.sys)->bodies[w.body].species) {
                if (m.state != SS_LIVING || (m.cycle & LC_DORMANT)) continue;
                X.sx = w.sys.star.sx; X.sy = w.sys.star.sy; X.sz = w.sys.star.sz; X.body = w.body; X.species = m.index; X.strain = X.strainIn = 0;
                CultureKind k; Morph f; MicrobeGenes ge;
                if (!g.testFlaskKind(X, k, f, &ge) || !geneOfKind(ge, GN_QUIET) || growthIn(k, bestOf(k)).fit != GF_GROWS) continue;
                key = Guide::bodyKey(X.sx, X.sy, X.sz, w.body) + "/M" + std::to_string(m.index);
                g.guide.microbes[key].seen |= SEEN_FORM | SEEN_KIN;
                return true;
            }
            return false;
        };
        auto write = [&](const FlaskSpecies& X, const Medium& m, const std::vector<GeneEdit>& edits) {
            g.testEditorPut(X, m, true);
            for (const GeneEdit& e : edits) g.testEditorSet(e.place, e.cut, e.part);
            g.testEditorWrite();
            g.testLabSkip(EDIT_S + 1);
            for (int i = 0; i < INCUBATOR_FLASKS; i++) if (g.testLab().flasks[i].n == 1 && g.testLab().flasks[i].who[0].strain > 0 && g.testLab().flasks[i].who[0].sameSpecies(X) &&
                                                         g.testLab().flasks[i].events.size() == 1 && g.testLab().flasks[i].events[0].t > g.testLab().clock - 2) return i;
            return -1;
        };
        FlaskSpecies X; std::string key;
        if (W[2].body >= 0 && speciesOf(W[2], X, key)) {
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(X, k0, f0, &g0);
            const GeneCode c = geneCodeOf(g0.genesis);
            const MicrobeGene* Wm = geneOfKind(g0, GN_WARMTH);
            const MicrobeGene* Q = geneOfKind(g0, GN_QUIET);
            const MicrobeGene* V = geneOfKind(g0, GN_VITAL);
            const Medium best = bestOf(k0);
            Medium hot = best; hot.tempC = std::floor(k0.s.tMax) + 1;
            GeneEdit ew; ew.place = Wm->place; ew.part = heavier(c, Wm->part, 3);
            GeneEdit eq; eq.place = Q->place; eq.part = Q->part; eq.part[0] = eq.part[0] == 'A' ? 'C' : 'A';
            GeneEdit ev; ev.place = V->place; ev.cut = true; ev.part = V->part;
            // warmth and a quiet gene: said as candidates, nothing learned
            int f = write(X, best, {ew, eq});
            if (f >= 0) { g.testFlaskSet(f, hot); g.testLabSkip(6); said[0] = lastGene(g); }
            candidates = f >= 0 && !g.guide.genes.count(g.testGeneKey(g0.genesis.id, Wm->place)) && said[0].find("ITS WARMTH: GENE") != std::string::npos;
            // warmth alone: learned
            f = write(X, best, {ew});
            if (f >= 0) { g.testFlaskSet(f, hot); g.testLabSkip(6); said[1] = lastGene(g); }
            const auto kw = g.guide.genes.find(g.testGeneKey(g0.genesis.id, Wm->place));
            warmth = f >= 0 && kw != g.guide.genes.end() && kw->second.kind == GN_WARMTH && kw->second.how == GENE_SEEN_EDIT && said[1].find("IS ITS WARMTH") != std::string::npos;
            // a vital gene cut: dead at once
            f = write(X, best, {ev});
            said[2] = lastGene(g);
            const auto kv = g.guide.genes.find(g.testGeneKey(g0.genesis.id, V->place));
            vital = f >= 0 && g.testLab().flasks[f].culture.kind[0].dead && kv != g.guide.genes.end() && kv->second.kind == GN_VITAL && kv->second.how == GENE_SEEN_DEATH;
            // a quiet gene alone, grown and looked at: nothing seen
            for (int i = 0; i < INCUBATOR_FLASKS; i++) if (g.testLab().flasks[i].n && g.testLab().flasks[i].who[0].strain > 0) g.testFlaskEmpty(i);
            f = write(X, best, {eq});
            g.testLabSkip(150);
            if (f >= 0) g.testScopeStageFlask(f);
            said[3] = lastGene(g);
            const auto kq = g.guide.genes.find(g.testGeneKey(g0.genesis.id, Q->place));
            nothing = f >= 0 && kq != g.guide.genes.end() && kq->second.kind < 0 && kq->second.how == GENE_SEEN_NOTHING && g.testGeneWords(g0.genesis.id, Q->place) == "NOTHING SEEN";
        }
        FlaskSpecies Y; std::string ykey; std::string strainSaid;
        if (W[4].body >= 0 && speciesOf(W[4], Y, ykey)) {
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(Y, k0, f0, &g0);
            StrainRecord r; r.what = SM_HOT; r.value = k0.s.tMax + 5;
            g.guide.strains[ykey + "/S1"] = r;
            FlaskSpecies line = Y; line.strain = line.strainIn = 1;
            g.testSeqPutLine(line);
            g.testLabSkip(SEQUENCE_S + 1);
            const MicrobeGene* Wm = geneOfKind(g0, GN_WARMTH);
            const auto ks = Wm ? g.guide.genes.find(g.testGeneKey(g0.genesis.id, Wm->place)) : g.guide.genes.end();
            for (const std::string& l : g.testLab().seq.result) if (l.find("DIFFERS FROM ITS PARENT'S") != std::string::npos) strainSaid = l;
            strain = ks != g.guide.genes.end() && ks->second.kind == GN_WARMTH && ks->second.how == GENE_SEEN_STRAIN && strainSaid.find("ITS WARMTH") != std::string::npos;
        }
        check("genes: what a gene does learned: one change seen, candidates from two, a vital death, nothing seen, a strain read (Y-02)",
              candidates && warmth && vital && nothing && strain,
              fmt("two changed: %s (%s); its warmth alone: %s (%s); a vital cut: %s (%s); a quiet one: %s (%s); a strain read: %s (%s)", candidates ? "candidates" : "WRONG",
                  said[0].c_str(), warmth ? "learned" : "NOT LEARNED", said[1].c_str(), vital ? "VITAL" : "WRONG", said[2].c_str(), nothing ? "NOTHING SEEN" : "WRONG",
                  said[3].c_str(), strain ? "its warmth" : "WRONG", strainSaid.c_str()));
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
            g.testLabSkip(30);           // E2 writing, 90 s to go
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
}