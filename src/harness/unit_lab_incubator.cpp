// B-442 and R-439 to R-442 (2026-11-19; the unit's `incubator` group, after `experiments`): the lab's rows telling a vial's organisms apart by
// their looks (the incubator's chooser, the sequencer's, a flask's; B-443, the tree's leaves and the sequencer's title, kin and log); N naming a
// flask's organism; the choosers' two tabs; K keeping a copy (the cell under the crosshair alone, a flask's two together) in a folder, a new one
// made for it, C copying, a pair kept twice merged; the jars' bands in their places, told on the page and kept on the sheets, a jar's settling
// said once
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/cultures.h"
#include <map>
#include <set>

namespace {
const char* const GUIDE = "shots/tests/unit_lab_guide.txt";
// a game on the ship, its guide fresh, its files the harness's
void labGame(TestGame& g) {
    g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = GUIDE; g.keysPath = "shots/tests/test_keys.txt";
    remove(GUIDE);
    g.newGame(); g.setState(GameState::SPACE);
}
// the cold store given a vial of each of the scan region's first living worlds (`labVial`), every living or sleeping species of each seen
// (the guide's sheet, its form): the vials' count
int stockVials(TestGame& g, int want) {
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
            g.testColdStore().push_back(v);
            n++;
        }
    }
    return n;
}
// the cold store's first vial holding two living species (its place, the two), else -1
int firstPair(TestGame& g, int& a, int& b) {
    for (int v = 0; v < (int)g.testColdStore().size(); v++) {
        const Vial& V = g.testColdStore()[v];
        StarSystem sys; Star st;
        if (!placeInSector(V.sx, V.sy, V.sz, st, true)) continue;
        sys.generate(st);
        std::vector<int> living;
        for (const Held& H : sampleOf(lifeOf(sys)->bodies[V.body], V.hab).held) if (H.state == SS_LIVING && H.cells >= 1) living.push_back(H.species);
        if (living.size() < 2) continue;
        a = living[0]; b = living[1];
        return v;
    }
    return -1;
}
std::string speciesKeyOf(const Vial& V, int species) { return Guide::bodyKey(V.sx, V.sy, V.sz, V.body) + "/M" + std::to_string(species); }
void typeWord(TestGame& g, Input& in, const char* w) { for (const char* c = w; *c; c++) pressKey(g, in, KEY_A + (*c - 'A')); }
// a chooser's world folders: how many hold two rows or more, how many rows repeat one before them in their folder, how many read UNNAMED
void rowsApart(const std::vector<LabFolder>& folders, const std::vector<std::string>& lines, int& many, int& same, int& unnamed, std::string& example) {
    for (const LabFolder& f : folders) {
        if (f.id != FOLDER_WORLD) continue;
        many += f.rows.size() >= 2;
        std::set<std::string> seen;
        for (int r : f.rows) {
            const std::string& l = lines[r];
            if (!seen.insert(l).second) { same++; if (example.empty()) example = l; }
            unnamed += l.compare(0, 7, "UNNAMED") == 0;
        }
    }
}
}   // namespace

void unitLabIncubator(const Check& check) {
    {   // B-442: a vial's organisms told apart. The scan region's first forty living worlds a vial each, every species of each seen: in the
        // incubator's chooser and the sequencer's each world's rows differ and none reads UNNAMED (each its look and its size); a flask of a vial's
        // first two living species reads their looks
        TestGame g; Input in;
        labGame(g);
        const int vials = stockVials(g, 40);
        int many = 0, same = 0, unnamed = 0, manyQ = 0, sameQ = 0, unnamedQ = 0;
        std::string example, exampleQ;
        {
            const std::vector<IncubatorChoice>& C = g.testIncubatorChoices(0);
            std::vector<std::string> lines;
            for (const IncubatorChoice& c : C) lines.push_back(c.line);
            rowsApart(g.testIncub().pages.folders, lines, many, same, unnamed, example);
        }
        {
            const std::vector<SequencerChoice>& C = g.testSequencerChoices();
            std::vector<std::string> lines;
            for (const SequencerChoice& c : C) lines.push_back(c.line);
            rowsApart(g.testLab().seq.pages.folders, lines, manyQ, sameQ, unnamedQ, exampleQ);
        }
        std::string labels;
        bool flaskApart = false;
        for (int v = 0; v < (int)g.testColdStore().size() && labels.empty(); v++) {
            const Vial& V = g.testColdStore()[v];
            StarSystem sys; Star st;
            if (!placeInSector(V.sx, V.sy, V.sz, st, true)) continue;
            sys.generate(st);
            std::vector<int> living;
            for (const Held& H : sampleOf(lifeOf(sys)->bodies[V.body], V.hab).held) if (H.state == SS_LIVING) living.push_back(H.species);
            if (living.size() < 2) continue;
            g.testFlaskEmpty(0);
            if (!g.testFlaskPut(0, v, living[0]) || !g.testFlaskPut(0, v, living[1])) continue;
            const Flask& F = g.testLab().flasks[0];
            labels = F.label[0] + " + " + F.label[1];
            flaskApart = F.label[0] != F.label[1] && F.label[0].compare(0, 7, "UNNAMED") != 0 && F.label[1].compare(0, 7, "UNNAMED") != 0;
        }
        check("incubator: a vial's organisms told apart by their looks: the incubator's chooser, the sequencer's, a flask's rows (B-442)",
              vials == 40 && many > 0 && same == 0 && unnamed == 0 && manyQ > 0 && sameQ == 0 && unnamedQ == 0 && flaskApart,
              fmt("%d vials; the incubator's chooser: %d worlds of two rows or more, %d rows repeated%s, %d UNNAMED; the sequencer's: %d, %d%s, %d; "
                  "a flask of two: %s",
                  vials, many, same, example.empty() ? "" : (" (" + example + ")").c_str(), unnamed, manyQ, sameQ,
                  exampleQ.empty() ? "" : (" (" + exampleQ + ")").c_str(), unnamedQ, labels.empty() ? "NONE PUT IN" : labels.c_str()));
    }
    {   // B-443 (KI-447): the tree's leaves and the sequencer in looks. The scan region's first forty living worlds a vial each, every species of
        // each seen and read but a vial's first living one: no leaf UNNAMED, none drawn alike another of its world; that one read last: the
        // sequencer's title its leaf's words, its kin in theirs, the log's KIN without UNNAMED; named ROSY on a flask (N), the sequencer's title ROSY
        TestGame g; Input in;
        labGame(g);
        const int vials = stockVials(g, 40);
        int a = -1, b = -1;
        const int v = firstPair(g, a, b);
        const std::string ka = v >= 0 ? speciesKeyOf(g.testColdStore()[v], a) : std::string();
        for (auto& kv : g.guide.microbes) if (kv.first != ka) kv.second.seen |= SEEN_KIN;
        int leaves = 0, same = 0, unnamed = 0;
        std::string example;
        std::map<std::string, std::set<std::string>> drawn;   // each world's leaves as drawn
        for (const TreeLeaf& L : g.testTree().leaves) {
            if (L.body < 0) continue;
            leaves++;
            const std::string line = TestGame::testTreeLine(L);
            if (!drawn[L.key.substr(0, L.key.rfind("/M"))].insert(line).second) { same++; if (example.empty()) example = line; }
            unnamed += L.name.compare(0, 7, "UNNAMED") == 0;
        }
        std::string title, leaf, kin, logged, renamed;
        bool kinLeaf = false;
        if (v >= 0) {
            const Vial& V = g.testColdStore()[v];
            g.testSeqPut(V.sx, V.sy, V.sz, V.body, a);
            title = g.testLab().seq.name;
            g.testLabSkip(SEQUENCE_S + 1);
            const std::vector<std::string>& R = g.testLab().seq.result;
            kin = R.size() > 2 && R[2].compare(0, 22, "ITS NEAREST KIN READ: ") == 0 ? R[2].substr(22) : std::string();
            for (const TreeLeaf& L : g.testTree().leaves) {
                if (L.key == ka) leaf = L.name;
                else if (!kin.empty() && kin.compare(0, L.name.size(), L.name) == 0) kinLeaf = true;
            }
            for (const LogEntry& e : g.guide.log) if (e.kind == "KIN") logged = e.text.substr(std::min(e.text.size(), e.text.find("KIN: ")));   // (after its date)
            if (g.testFlaskPut(0, v, a)) {   // N names it on its flask: the sequencer says the name
                g.openScreen(GameState::INCUBATOR); g.frame(in, 0); in.newFrame();
                g.testIncub().sel = 0; g.testIncub().page = 0;
                pressKey(g, in, KEY_N); typeWord(g, in, "ROSY"); pressKey(g, in, KEY_ENTER);
                g.testLabSkip(0.1);
                renamed = g.testLab().seq.name;
            }
        }
        check("incubator: the tree's leaves and the sequencer in looks: none UNNAMED, none of a world alike; its title, kin and log (B-443)",
              vials == 40 && leaves > 100 && same == 0 && unnamed == 0 && !title.empty() && title.compare(0, 7, "UNNAMED") != 0 && title == leaf &&
                  kinLeaf && logged.compare(0, 5 + title.size(), "KIN: " + title) == 0 && logged.find("UNNAMED") == std::string::npos && renamed == "ROSY",
              fmt("%d vials; the tree: %d leaves of species, %d drawn alike another of their world%s, %d UNNAMED; read last: '%s' (its leaf '%s'), its kin "
                  "'%s' %s; the log '%s'; named on its flask: '%s'",
                  vials, leaves, same, example.empty() ? "" : (" (" + example + ")").c_str(), unnamed, title.c_str(), leaf.c_str(), kin.c_str(),
                  kinLeaf ? "a leaf's" : "NOT A LEAF'S", logged.c_str(), renamed.c_str()));
    }
    {   // R-442: N names a flask's organism. A flask of a vial's first two living species: N on its page offers both (each its look), the second
        // chosen and named ROSY by its keys; the flask's row says ROSY; a flask of one: N names it at once
        TestGame g; Input in;
        labGame(g);
        stockVials(g, 40);
        int a = -1, b = -1;
        const int v = firstPair(g, a, b);
        bool rows = false, typed = false, named = false, label = false, once = false;
        std::string offered;
        if (v >= 0 && g.testFlaskPut(0, v, a) && g.testFlaskPut(0, v, b) && g.testFlaskPut(1, v, a)) {
            g.openScreen(GameState::INCUBATOR); g.frame(in, 0); in.newFrame();
            g.testIncub().sel = 0; g.testIncub().page = 0;
            pressKey(g, in, KEY_N);
            rows = g.testIncub().pick == PICK_NAME && g.testIncub().pickLines.size() == 2;
            for (const std::string& l : g.testIncub().pickLines) offered += (offered.empty() ? "" : " / ") + l;
            pressKey(g, in, KEY_DOWN); pressKey(g, in, KEY_ENTER);
            typed = g.state == GameState::TEXT_ENTRY;
            typeWord(g, in, "ROSY"); pressKey(g, in, KEY_ENTER);
            const Vial& V = g.testColdStore()[v];
            named = g.state == GameState::INCUBATOR && g.guide.names[speciesKeyOf(V, b)] == "ROSY" && !g.guide.names.count(speciesKeyOf(V, a));
            label = g.testLab().flasks[0].label[1] == "ROSY" && g.testLab().flasks[0].label[0] != "ROSY";
            g.testIncub().sel = 1;
            pressKey(g, in, KEY_N);
            once = g.state == GameState::TEXT_ENTRY && g.testIncub().pick == PICK_NONE;
            pressKey(g, in, KEY_ESCAPE);
        }
        check("incubator: N names a flask's organism: of two the one chosen, of one at once; its rows say the name (R-442)",
              v >= 0 && rows && typed && named && label && once,
              fmt("a flask of two %s: offered %s (%s); the second typed %s, named ROSY %s, the flask's row %s; a flask of one named at once %s",
                  v >= 0 ? "put in" : "NONE FOUND", rows ? "both" : "NOT BOTH", offered.c_str(), typed ? "yes" : "NO", named ? "yes" : "NO",
                  label ? "says it" : "DOES NOT", once ? "yes" : "NO"));
    }
    {   // R-441: the choosers' two tabs. Vials of ten worlds (every species seen), a flask's culture and a kept one: Enter on an empty flask opens on
        // the vials (each folder a world's); Tab the cultures (the flasks', the collection's, no world's); Esc, and Enter opens on the cultures again
        // (the tab last shown); the sequencer's likewise; the jars' chooser one list (its vials)
        TestGame g; Input in;
        labGame(g);
        stockVials(g, 10);
        Vial silt = g.testColdStore()[0]; silt.hab.niche = NI_SILT; silt.name = "WET SILT";
        g.testColdStore().push_back(silt);
        int a = -1, b = -1;
        const int v = firstPair(g, a, b);
        bool put = v >= 0 && g.testFlaskPut(0, v, a), vials = false, cultures = false, again = false, seq = false, jar = false;
        if (put) {
            KeptCulture c; c.who = g.testLab().flasks[0].who[0]; c.medium = c.asWorld = g.testLab().flasks[0].medium;
            g.testLab().kept.push_back(c);
            g.openScreen(GameState::INCUBATOR); g.frame(in, 0); in.newFrame();
            g.testIncub().sel = 1; g.testIncub().page = 0;
            pressKey(g, in, KEY_ENTER);
            auto only = [](const ChooserPage& P, bool worlds) {
                if (P.folders.empty()) return false;
                for (const LabFolder& f : P.folders) if ((f.id == FOLDER_WORLD) != worlds) return false;
                return true;
            };
            const ChooserPage& P = g.testIncub().pages;
            vials = g.testIncub().choosing && P.tabbed && P.tab == 0 && only(P, true);
            pressKey(g, in, KEY_TAB);
            bool kept = false;
            for (const LabFolder& f : P.folders) kept |= f.id >= 0;
            cultures = P.tab == 1 && only(P, false) && kept;
            pressKey(g, in, KEY_ESCAPE);
            pressKey(g, in, KEY_ENTER);
            again = g.testIncub().choosing && P.tab == 1;
            pressKey(g, in, KEY_ESCAPE);
            g.testIncub().page = 1; g.testIncub().jar = 0;
            pressKey(g, in, KEY_ENTER);
            jar = g.testIncub().choosing && !g.testIncub().pages.tabbed;
            pressKey(g, in, KEY_ESCAPE);
            g.closeScreen();
            g.openScreen(GameState::SEQUENCER); g.frame(in, 0); in.newFrame();
            pressKey(g, in, KEY_ENTER);
            const ChooserPage& Q = g.testLab().seq.pages;
            const bool first = g.testLab().seq.choosing && Q.tabbed && Q.tab == 0 && only(Q, true);
            pressKey(g, in, KEY_TAB);
            seq = first && Q.tab == 1 && only(Q, false);
        }
        check("incubator: the choosers' two tabs: the vials' worlds, the cultures; the tab last shown first; the sequencer's too, the jars' one list (R-441)",
              put && vials && cultures && again && seq && jar,
              fmt("the incubator's on the vials %s; Tab the cultures %s; opened again on them %s; the sequencer's %s; the jars' untabbed %s",
                  vials ? "yes" : "NO", cultures ? "yes" : "NO", again ? "yes" : "NO", seq ? "yes" : "NO", jar ? "yes" : "NO"));
    }
    {   // R-440: K keeps a copy. The dense vial on the microscope, a living cell tracked: K, Enter keeps it alone in UNSORTED (the vial
        // kept); again refused there; K, N, IRON, Enter makes the folder IRON and keeps it in it; a flask of two: K, the third row (both), Enter keeps
        // the pair (the flask grows on); C copies a kept one to IRON, twice refused; a pair B-441 kept twice (each way round) merged at a load
        TestGame g; Input in;
        labGame(g);
        Vial dense;
        const bool ok = labDenseVial(dense);
        bool under = false, alone = false, twice = false, folder = false, both = false, copied = false, merged = false;
        int sp = -1;
        if (ok) {
            g.testColdStore().push_back(dense);
            g.openScreen(GameState::MICROSCOPE); g.frame(in, 0); in.newFrame();
            if (g.testScopeStage(0)) {
                g.testScopeFrame();
                const MicroscopeState& S = g.testScope();
                for (size_t i = 0; i < S.shown.size() && sp < 0; i++) {
                    const int k = S.mount.cells[S.refs[i].cell].kind, s0 = S.mount.kinds[k].species;
                    if (!S.shown[i].m || g.testScope().species[s0].state != SS_LIVING) continue;
                    g.testScopeView(S.seeing, S.step, S.refs[i].x, S.refs[i].y, S.focus);
                    g.testScopeFrame();
                    if (g.testScopeUnder() >= 0 && g.testScope().species[g.testScopeUnder()].state == SS_LIVING) sp = g.testScopeUnder();
                }
                if (sp >= 0) pressKey(g, in, KEY_ENTER);   // tracked: the crosshair follows it
                under = sp >= 0 && g.testScope().tracking && g.testScopeUnder() == sp;
            }
            if (under) {
                const size_t n0 = g.testLab().kept.size(), vials0 = g.testColdStore().size();
                pressKey(g, in, KEY_K);
                const bool open = g.testKeep().open;
                pressKey(g, in, KEY_ENTER);
                const KeptCulture* c = g.testLab().kept.size() == n0 + 1 ? &g.testLab().kept.back() : nullptr;
                alone = open && c && !c->paired && c->who.species == sp && c->folder == 0 && g.testColdStore().size() == vials0;
                pressKey(g, in, KEY_K); pressKey(g, in, KEY_ENTER);
                twice = g.testLab().kept.size() == n0 + 1 && g.testKeep().open;
                pressKey(g, in, KEY_ESCAPE);
                pressKey(g, in, KEY_K); pressKey(g, in, KEY_N);
                const bool typing = g.state == GameState::TEXT_ENTRY;
                typeWord(g, in, "IRON"); pressKey(g, in, KEY_ENTER);
                int iron = -1;
                for (const CultureFolder& f : g.testLab().folders) if (f.name == "IRON") iron = f.id;
                folder = typing && iron > 0 && g.testLab().kept.size() == n0 + 2 && g.testLab().kept.back().folder == iron && g.state == GameState::MICROSCOPE;
                // a flask of two, both kept
                int a = -1, b = -1;
                for (const Held& H : g.testScope().sample.held) if (H.state == SS_LIVING && H.cells >= 1) { if (a < 0) a = H.species; else if (b < 0) b = H.species; }
                g.closeScreen();
                if (b >= 0 && g.testFlaskPut(0, 0, a) && g.testFlaskPut(0, 0, b)) {
                    g.openScreen(GameState::INCUBATOR); g.frame(in, 0); in.newFrame();
                    g.testIncub().sel = 0; g.testIncub().page = 0;
                    pressKey(g, in, KEY_K);
                    const bool rows = g.testIncub().pick == PICK_KEEP && g.testIncub().pickLines.size() == 3;
                    pressKey(g, in, KEY_DOWN); pressKey(g, in, KEY_DOWN); pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_ENTER);
                    const KeptCulture& p = g.testLab().kept.back();
                    both = rows && p.paired && p.who.species == a && p.partner.species == b && p.folder == 0 && g.testLab().flasks[0].n == 2;
                    g.closeScreen();
                }
                const size_t n1 = g.testLab().kept.size();
                g.testKeptCopy(n0, iron);   // the one kept alone, into IRON (its copy from the new folder there already: refused)
                const bool refusedThere = g.testLab().kept.size() == n1;
                g.testKeptCopy((int)n1 - 1, iron);
                copied = refusedThere && g.testLab().kept.size() == n1 + 1 && g.testLab().kept.back().folder == iron && g.testLab().kept.back().paired;
                // B-441's pair kept twice, each way round: one at a load
                KeptCulture x = g.testLab().kept.back(), y = x;
                x.folder = y.folder = 0;
                std::swap(y.who, y.partner);
                g.testLab().kept.clear(); g.testLab().kept.push_back(x); g.testLab().kept.push_back(y);
                g.save("shots/tests/unit_lab_keep.txt");
                TestGame h; labGame(h);
                h.load("shots/tests/unit_lab_keep.txt");
                merged = h.testLab().kept.size() == 1 && h.testLab().kept[0].paired;
                remove("shots/tests/unit_lab_keep.txt");
            }
        }
        check("incubator: K keeps a copy: the cell under the crosshair alone, never twice in a folder, a new folder made for it; a flask's two together; C copies; a pair kept twice merged (R-440)",
              ok && under && alone && twice && folder && both && copied && merged,
              fmt("the crosshair on a living cell %s (M%d); kept alone in UNSORTED, the vial kept %s; again refused %s; a new folder IRON with it %s; a flask's two "
                  "kept together, the flask growing on %s; C copied into IRON, twice refused %s; a pair kept each way round one at a load %s",
                  under ? "yes" : "NO", sp, alone ? "yes" : "NO", twice ? "yes" : "NO", folder ? "yes" : "NO", both ? "yes" : "NO", copied ? "yes" : "NO",
                  merged ? "yes" : "NO"));
    }
    {   // R-439: the jars made clear. Every jar of the scan region's first sixty living worlds with wet silt (a silt place each): its bands top to
        // bottom, none over another; every eater of a chemical that needs air (but sulphide's) at the air's edge. A game: a jar of the first with two
        // bands or more, its species seen; at twenty minutes the page's bands go to their sheets (kept through the guide's save, the sheet's IN A JAR
        // line); its settling said once, a jar settled before a load not again
        TestGame g; Input in;
        labGame(g);
        int jars = 0, bands = 0, over = 0, edge = 0, edgeWrong = 0;
        Vial jv; bool have = false;
        for (auto& [x, z, s, scanned] : scanSystems(150, 340, 20, 160, [&] { return jars < 60; })) {
            const StarSystem& sys = scanned;
            auto L = lifeOf(sys);
            for (int i = 0; i < (int)sys.bodies.size() && jars < 60; i++) {
                const Biota& B = L->bodies[i];
                if (!livingStage(B.stage) || !(B.habitats & nicheBit(NI_SILT))) continue;
                Microhabitat h; h.niche = NI_SILT; h.meanC = B.meanC; h.place = mix64(sys.bodies[i].seed ^ 0x9E3779B97F4A7C15ULL);
                const JarRecipe r = jarRecipe(B, sampleOf(B, h), h);
                jars++;
                for (size_t k = 0; k < r.bands.size(); k++) {
                    const JarBand& d = r.bands[k];
                    bands++;
                    if (k > 0 && d.top < r.bands[k - 1].bottom - 1e-9 && d.top < 0.97) over++;
                    const Microbe& m = B.species[d.species];
                    const bool other = m.way == LW_CHEMICAL && m.substrate != SU_IRON && m.substrate != SU_SULPHIDE, needs = other && m.oxygen >= 2;
                    edge += needs;
                    edgeWrong += other && needs != (d.layer == JL_EDGE);
                }
                if (!have && r.bands.size() >= 2) {
                    jv.sx = sys.star.sx; jv.sy = sys.star.sy; jv.sz = sys.star.sz; jv.body = i; jv.hab = h; jv.name = "WET SILT"; jv.readingC = h.meanC;
                    have = true;
                }
            }
        }
        bool sheets = false, kept = false, line = false, said = false, once = false, loaded = false;
        std::string sheetLine;
        if (have) {
            g.testColdStore().push_back(jv);
            g.testJarPut(0);
            const Jar& J = g.testLab().jars[0];
            for (const JarBand& d : J.recipe.bands) g.guide.microbes[speciesKeyOf(jv, d.species)].seen |= SEEN_FORM;
            g.testClearStatus();
            g.testLabSkip(JAR_SETTLED_S + 1);
            said = g.testStatus().find("HAS SETTLED") != std::string::npos;
            g.testClearStatus();
            g.testLabSkip(30);
            once = g.testStatus().find("HAS SETTLED") == std::string::npos;
            g.testJarSeen(0);
            sheets = true;
            for (const JarBand& d : J.recipe.bands) sheets &= g.guide.microbes[speciesKeyOf(jv, d.species)].jarLayer == d.layer;
            Guide back; back.load(GUIDE);
            kept = true;
            for (const JarBand& d : J.recipe.bands) kept &= back.microbes[speciesKeyOf(jv, d.species)].jarLayer == d.layer;
            StarSystem sys; Star st;
            if (placeInSector(jv.sx, jv.sy, jv.sz, st, true)) {
                sys.generate(st);
                const int s0 = J.recipe.bands[0].species;
                const std::vector<std::string> L = g.testSheetLinesOf(speciesKeyOf(jv, s0), lifeOf(sys)->bodies[jv.body].species[s0], morphsOf(sys, jv.body)[s0],
                                                                      g.guide.microbes[speciesKeyOf(jv, s0)]);
                for (const std::string& l : L) if (l.compare(0, 9, "IN A JAR:") == 0) { line = true; sheetLine = l; }
            }
            g.save("shots/tests/unit_lab_jar.txt");
            TestGame h; labGame(h);
            h.load("shots/tests/unit_lab_jar.txt");
            h.testClearStatus();
            h.testLabSkip(1);
            loaded = h.testLab().jars.size() == 1 && h.testStatus().find("HAS SETTLED") == std::string::npos;
            remove("shots/tests/unit_lab_jar.txt");
        }
        check("incubator: the jars made clear: bands in their places, none over another, an eater that needs air at the air's edge; kept on the sheets; "
              "a jar's settling said once (R-439)",
              jars == 60 && over == 0 && edgeWrong == 0 && have && sheets && kept && line && said && once && loaded,
              fmt("%d jars, %d bands: %d over another; %d eaters of a chemical needing air, %d not at the air's edge; a jar of two bands or more %s: its bands "
                  "on their sheets %s, through the guide's save %s, the sheet's line %s (%s); its settling said %s, once %s, not again after a load %s",
                  jars, bands, over, edge, edgeWrong, have ? "made" : "NONE", sheets ? "yes" : "NO", kept ? "yes" : "NO", line ? "yes" : "NO",
                  sheetLine.c_str(), said ? "yes" : "NO", once ? "yes" : "NO", loaded ? "yes" : "NO"));
    }
    remove(GUIDE);   // (the group's guide, removed after it)
}
