// Z-04: the unit's laboratory (group `lab`): a slide's frame closed-form in its clock (with what was done to it) and the same at any thread
// count, in every way of seeing; a species' sheet's lines only by what was seen of it; the guide's save byte-stable with its species; the vials'
// list emptied a vial at a time, a bloom kept in a vial's save line. Z-05: the cultures; Z-06: the genomes, the tree, the sequencer; Z-07: the record;
// R-432: the vials by world; R-433: the culture collection
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/slides.h"
#include "life/cultures.h"
#include "life/sequences.h"
#include <fstream>
#include <set>
#include <sstream>

namespace {
// a game in the microscope on a living world's vial (the first felisian's ground), its guide fresh
bool scopeOnVial(TestGame& g, Input& in, const char* guide) {
    g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = guide; g.keysPath = "shots/tests/test_keys.txt";
    remove(guide);
    g.newGame(); g.setState(GameState::SPACE);
    StarSystem sys; int bi = -1;
    Vial v;
    if (!firstBodyOfType(PT_FELISIAN, sys, bi) || !labVial(sys, bi, v)) return false;
    g.testColdStore().push_back(v);
    g.openScreen(GameState::MICROSCOPE);
    g.frame(in, 0); in.newFrame();
    return g.testScopeStage(0);
}
// the frame at a time: the slide's clock set, a frame drawn at one instant, the status cleared first
// R-437: the slide's told answers forgotten first, the HUD's lines of no history (what is said is the game's, not the slide's closed form)
void frameAt(TestGame& g, Input& in, double t) { g.testScope().t = t; g.testScopeFresh(); g.testClearStatus(); g.frame(in, 0); in.newFrame(); }
}   // namespace

void unitLab(const Check& check) {
    {   // a slide's frame closed-form: the light at 2 s, the magnet at 4, the stage warmed at 6, food at 8 (the events in its list); the frame at 37.5 s
        // reached at once, by every half second, and at one thread, in each way of seeing of a drop at the first look's place one step out
        TestGame g; Input in;
        const bool ok = scopeOnVial(g, in, "shots/tests/unit_lab_guide.txt");
        int ways = 0, same = 0, specimens = 0; std::string off;
        if (ok) {
            MicroscopeState& M = g.testScope();
            const double cx = M.mount.cx, cy = M.mount.cy, focus = M.mount.focus;
            const double r = umPerPxAt(100) * UW * 0.12;
            M.events = {{SD_LIGHT, 2, cx, cy, r, true}, {SD_MAGNET, 4, 0, 0, 0, true}, {SD_WARM, 6, M.mount.stageC + 30, 0, 0, true}, {SD_FOOD, 8, cx + r, cy, r * 0.7, true}};
            for (int way = 0; way < WS_COUNT; way++) {
                if (way == WS_SECTION) continue;   // a drop's
                g.testScopeView(way, way == WS_ELECTRON ? 2 : 2, cx, cy, focus);
                frameAt(g, in, 37.5);
                const std::vector<Pix> a = g.fb.idx; const std::vector<uint32_t> ao(g.output(), g.output() + (size_t)FBW * FBH);
                specimens += (int)M.shown.size();
                for (double t = 0; t <= 37.5 + 1e-9; t += 0.5) frameAt(g, in, t);
                const std::vector<Pix> b = g.fb.idx; const std::vector<uint32_t> bo(g.output(), g.output() + (size_t)FBW * FBH);
                parallelThreadsOverride() = 1;
                frameAt(g, in, 37.5);
                parallelThreadsOverride() = 0;
                const bool eq = a == b && a == g.fb.idx && ao == bo && std::equal(ao.begin(), ao.end(), g.output());
                ways++; same += eq;
                if (!eq) off += std::string(off.empty() ? "" : ", ") + SEEING_NAMES[way];
            }
        }
        check("lab: a slide's frame closed-form in its clock, the same at any thread count (Z-04)", ok && ways == WS_COUNT - 1 && same == ways && specimens > 0,
              fmt("%s; %d ways of seeing with the light, the magnet, the warm stage and food: %d the same at 37.5 s skipped to, stepped and at one thread, %d specimens%s",
                  ok ? "a felisian's vial on the stage" : "NO VIAL STAGED", ways, same, specimens, off.empty() ? "" : ("; apart: " + off).c_str()));
    }
    {   // a sheet's lines only by what was seen: the first look's species before a frame, in a bright field, then a dark field, the glow and the electron
        TestGame g; Input in;
        const bool ok = scopeOnVial(g, in, "shots/tests/unit_lab_guide.txt");
        std::string steps; bool right = ok;
        if (ok) {
            MicroscopeState& M = g.testScope();
            const SlideKind* K = nullptr;
            for (const SlideKind& k : M.mount.kinds) if (M.mount.cells[k.first].x == M.mount.cx && M.mount.cells[k.first].y == M.mount.cy) K = &k;
            const int sp = K ? K->species : -1;
            g.guide.microbes.clear();
            auto has = [&](const char* prefix) { for (const std::string& l : g.testSheetLines(sp)) if (l.rfind(prefix, 0) == 0) return true; return false; };
            auto line = [&](const char* what) {
                const int n = (int)g.testSheetLines(sp).size();
                steps += fmt("%s%s: %d lines (%s%s%s%s%s)", steps.empty() ? "" : "; ", what, n, has("FORM:") ? "F" : "-", has("MOTION:") ? "M" : "-", has("FLAGELLA:") || has("CILIA") ? "L" : "-",
                             has("DNA:") ? "D" : "-", has("WALL:") ? "W" : "-");
            };
            line("unseen");
            right = right && sp >= 0 && !has("FORM:") && !has("FLAGELLA:") && !has("DNA:") && !has("WALL:");
            const Morph& m = M.forms[sp];
            const double lightMag = magnificationFor(m, WS_BRIGHT, 0.3);
            int step = 0;
            for (int i = 0; i < SCOPE_LIGHT_STEPS; i++) if (std::fabs(std::log(SCOPE_LIGHT[i] / lightMag)) < std::fabs(std::log(SCOPE_LIGHT[step] / lightMag))) step = i;
            const SlideCell& c = M.mount.cells[K->first];
            g.testScopeView(WS_BRIGHT, step, c.x, c.y, c.z); frameAt(g, in, 0.5);
            line("bright");
            right = right && has("FORM:") && !has("FLAGELLA:") && !has("DNA:") && !has("WALL:");
            g.testScopeView(WS_DARK, step, c.x, c.y, c.z); frameAt(g, in, 0.5);
            line("dark");
            right = right && has("FLAGELLA:") + has("CILIA") >= 1 && !has("DNA:") && !has("WALL:");
            g.testScopeView(WS_GLOW, step, c.x, c.y, c.z); frameAt(g, in, 0.5);
            line("glow");
            right = right && has("DNA:") && !has("WALL:");
            int estep = 0;   // the electron's step that holds the cell some 60 pixels across
            for (int i = 0; i < SCOPE_ELECTRON_STEPS; i++) if (2 * m.bodyUm() / umPerPxAt(SCOPE_ELECTRON[i]) <= 120) estep = i;
            g.testScopeView(WS_ELECTRON, estep, c.x, c.y, c.z); frameAt(g, in, 0.5);
            line("electron");
            right = right && has("WALL:");
        }
        check("lab: a sheet's lines only by what was seen (Z-04)", right, fmt("the first look's species: %s", steps.c_str()));
    }
    {   // the guide byte-stable with its species: seen (above's way) and named, saved, loaded and saved again
        TestGame g; Input in;
        const char* path = "shots/tests/unit_lab_guide.txt";
        bool ok = scopeOnVial(g, in, path);
        int microbes = 0; bool stable = false, line = false;
        if (ok) {
            frameAt(g, in, 1);
            const int sp = g.testScopeUnder() >= 0 ? g.testScopeUnder() : g.testScope().mount.kinds[0].species;
            g.guide.names[g.testSpeciesKey(sp)] = "TEST SPECIES";
            g.guide.save(path);
            std::ifstream f1(path); const std::string s1((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>());
            Guide back; ok = back.load(path);
            microbes = (int)back.microbes.size();
            back.save("shots/tests/unit_lab_guide2.txt");
            std::ifstream f2("shots/tests/unit_lab_guide2.txt"); const std::string s2((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());
            stable = s1 == s2;
            line = s1.find("\nmicrobes 1\n") != std::string::npos && s1.find("\nmicrobe ") != std::string::npos && s1.find("/M" + std::to_string(sp) + " TEST SPECIES") != std::string::npos;
        }
        check("lab: the guide's save byte-stable with its species (Z-04)", ok && stable && line && microbes > 0,
              fmt("%d species' sheets and a name, the microbes 1 line %s; loaded and saved again %s", microbes, line ? "there" : "MISSING", stable ? "the same bytes" : "DIFFERENT"));
    }
    {   // the vials' list: Backspace empties the one chosen, freeing its place; a vial's bloom kept in its save line
        TestGame g; Input in;
        const bool ok = scopeOnVial(g, in, "shots/tests/unit_lab_guide.txt");
        int before = 0, after = 0; bool bloom = false;
        if (ok) {
            g.testColdStore().push_back(g.testColdStore()[0]);
            g.testColdStore()[1].hab.bloom = true;
            g.save("shots/tests/unit_lab_vials.txt");
            g.testColdStore().clear();
            g.load("shots/tests/unit_lab_vials.txt");
            bloom = g.testColdStore().size() == 2 && !g.testColdStore()[0].hab.bloom && g.testColdStore()[1].hab.bloom;
            g.openScreen(GameState::MICROSCOPE); g.frame(in, 0); in.newFrame();   // its list's page
            before = (int)g.testColdStore().size();
            pressKey(g, in, KEY_BACKSPACE);
            after = (int)g.testColdStore().size();
        }
        check("lab: Backspace on the list empties a vial, a bloom kept in a vial's line (Z-04)", ok && before == 2 && after == 1 && bloom && g.state == GameState::MICROSCOPE,
              fmt("%d vials, %d after Backspace; a bloom through the save's lines %s", before, after, bloom ? "kept" : "LOST"));
    }
    {   // R-432: the vials by world. Seventy vials of two worlds in the cold store (past its old 64 places, the second world's every other of five),
        // the microscope opened on its worlds (the first's 42 vials, the second's 28, in the store's order); Enter the first world's vials, Down and
        // Enter its second on the stage; Esc back to the world's vials on it, Esc to the worlds, Tab the cultures (the flasks, UNSORTED), Esc out.
        // Every species of the two vials seen: the incubator's chooser a row a species and state in its world's folder, the sequencer's a row a species
        TestGame g; Input in;
        g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_lab_guide.txt"; g.keysPath = "shots/tests/test_keys.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.setState(GameState::SPACE);
        StarSystem sys; int bi = -1, b2 = -1;
        Vial va, vb;
        bool ok = firstBodyOfType(PT_FELISIAN, sys, bi) && labVial(sys, bi, va);
        for (int b = 0; ok && b < (int)sys.bodies.size() && b2 < 0; b++) if (b != bi && labVial(sys, b, vb)) b2 = b;
        ok = ok && b2 >= 0;
        bool worlds = false, opened = false, staged = false, back = false, folders = false, cultures = false, closed = false;
        int rowsI = 0, wantI = 0, rowsS = 0, wantS = 0, foldersI = 0, foldersS = 0;
        if (ok) {
            for (int k = 0; k < 70; k++) g.testColdStore().push_back(k % 5 == 1 || k % 5 == 3 ? vb : va);
            g.openScreen(GameState::MICROSCOPE); g.frame(in, 0); in.newFrame();
            const MicroscopeState& M = g.testScope();
            const std::vector<LabFolder> F = g.testVialFolders();
            worlds = F.size() == 2 && F[0].rows.size() == 42 && F[1].rows.size() == 28 && F[0].rows[1] == 2 && F[1].rows[0] == 1 && !M.open && M.tab == 0;
            pressKey(g, in, KEY_ENTER); opened = M.open && M.folder == 0;
            pressKey(g, in, KEY_DOWN); pressKey(g, in, KEY_ENTER); staged = M.page == 1 && M.vial == 2;
            pressKey(g, in, KEY_ESCAPE); back = M.page == 0 && M.open && M.folder == 0 && M.sel == 1;
            pressKey(g, in, KEY_ESCAPE); folders = M.page == 0 && !M.open;
            pressKey(g, in, KEY_TAB);
            const std::vector<LabFolder> C = g.testCultureFolders();
            cultures = M.tab == 1 && !M.open && C.size() == 2 && C[0].name == "FLASKS" && C[1].name == "UNSORTED";
            pressKey(g, in, KEY_ESCAPE); closed = g.state != GameState::MICROSCOPE;
            std::set<std::string> byState, bySpecies;
            for (const Vial* v : {&va, &vb}) {
                const Biota& B = lifeOf(sys)->bodies[v->body];
                for (const Held& H : sampleOf(B, v->hab).held) {
                    if (H.state == SS_FOSSIL || H.state == SS_PROTO) continue;
                    const std::string key = Guide::bodyKey(v->sx, v->sy, v->sz, v->body) + "/M" + std::to_string(H.species);
                    g.guide.microbes[key].seen |= SEEN_FORM;
                    byState.insert(key + (H.state == SS_DORMANT ? "/D" : "")); bySpecies.insert(key);
                }
            }
            wantI = (int)byState.size(); wantS = (int)bySpecies.size();
            rowsI = (int)g.testIncubatorChoices(0).size(); foldersI = (int)g.testIncub().pages.folders.size();
            rowsS = (int)g.testSequencerChoices().size(); foldersS = (int)g.testLab().seq.pages.folders.size();
        }
        check("lab: the vials by world past the cold store's old 64 places, the choosers by world a row a species (R-432)",
              ok && worlds && opened && staged && back && folders && cultures && closed && rowsI == wantI && rowsS == wantS && foldersI == 2 && foldersS == 2 && wantS > 0,
              fmt("70 vials: the worlds %s, Enter %s, its second on the stage %s, Esc back on it %s, the worlds again %s, Tab the cultures %s, closed %s; "
                  "the incubator's %d rows (%d asked) in %d folders, the sequencer's %d (%d asked) in %d",
                  worlds ? "42 and 28" : "WRONG", opened ? "opened the first" : "NOT", staged ? "yes" : "NO", back ? "yes" : "NO", folders ? "yes" : "NO", cultures ? "yes" : "NO",
                  closed ? "yes" : "NO", rowsI, wantI, foldersI, rowsS, wantS, foldersS));
    }
    // ---- Z-05: the incubator
    // a flask of the vial's first living species in its best medium (its warmth, its energy, its salt, white light for a light-feeder), held at its
    // hot edge from 5 minutes, a fresh broth at 10, the vial's next living species added at 12; the lab's clock run to 40 minutes by the stretches'
    // skips and by second-long steps
    auto culture = [&](TestGame& g, Input& in, bool steps, std::string& why) {
        if (!scopeOnVial(g, in, "shots/tests/unit_lab_guide.txt")) { why = "NO VIAL"; return false; }
        const MicroscopeState& M = g.testScope();
        std::vector<int> living;
        for (const Held& H : M.sample.held) if (H.state == SS_LIVING && M.species[H.species].way != LW_PREDATOR && M.species[H.species].way != LW_NONE) living.push_back(H.species);
        if (living.size() < 2) { why = "FEWER THAN TWO LIVING SPECIES IN THE VIAL"; return false; }
        auto run = [&](double s) { if (!steps) g.testLabSkip(s); else for (int k = 0; k < (int)s; k++) g.testLabSkip(1.0); };
        const Microbe& a = M.species[living[0]];
        if (!g.testFlaskPut(0, 0, living[0])) { why = "NOT PUT IN"; return false; }
        Medium m;
        m.cell = chemistryCell(g.testLab().flasks[0].culture.kind[0].chemistry); m.tempC = std::round(a.tOpt); m.salt = a.saltLo;
        m.energy = a.way == LW_CHEMICAL ? a.substrate : (a.way == LW_LIGHT ? CE_NONE : CE_BROTH);
        m.light = a.way == LW_LIGHT ? CL_WHITE : CL_DARK;
        g.testFlaskSet(0, m);
        run(300);
        m.tempC = std::round(10 * (a.tOpt + 0.85 * (a.tMax - a.tOpt))) / 10;
        g.testFlaskSet(0, m);
        run(300);
        g.testFlaskFresh(0);
        run(120);
        if (!g.testFlaskPut(0, 0, living[1])) { why = "NO SECOND"; return false; }
        run(2400 - 720);
        return true;
    };
    {   // a culture's state closed-form from its events: the same reached by the stretches' skips, by second-long steps, and laid out again from
        // its saved events alone
        TestGame a, b; Input ia, ib;
        std::string why;
        const bool ok = culture(a, ia, false, why) && culture(b, ib, true, why);
        bool sameEvents = false, sameState = false, sameLaid = false;
        int events = 0, strains = 0, n = 0;
        std::string stages;
        if (ok) {
            const Flask& A = a.testLab().flasks[0];
            const Flask& B = b.testLab().flasks[0];
            events = (int)A.events.size();
            for (const CultureEvent& e : A.events) strains += e.kind == CD_STRAIN;
            sameEvents = A.events.size() == B.events.size();
            for (size_t i = 0; sameEvents && i < A.events.size(); i++) {
                const CultureEvent &x = A.events[i], &y = B.events[i];
                sameEvents = x.kind == y.kind && x.t == y.t && x.m == y.m && x.who == y.who && x.what == y.what && x.value == y.value;
            }
            const CultureNow na = a.testFlaskNow(0), nb = b.testFlaskNow(0);
            FlaskCulture c = A.culture;   // its kinds as put in, its events as saved: laid out again
            c.events = A.events;
            const CultureNow nc = cultureAt(c, runCulture(c), a.testLab().clock);
            auto same = [](const CultureNow& x, const CultureNow& y) {
                bool eq = x.n == y.n;
                for (int k = 0; eq && k < x.n; k++)
                    eq = x.one[k].cells == y.one[k].cells && x.one[k].spores == y.one[k].spores && x.one[k].ghosts == y.one[k].ghosts && x.one[k].stage == y.one[k].stage &&
                         x.one[k].glowing == y.one[k].glowing;
                return eq;
            };
            sameState = same(na, nb) && a.testLab().clock == b.testLab().clock;
            sameLaid = same(na, nc);
            n = na.n;
            for (int k = 0; k < na.n; k++) stages += fmt("%s%s %.2f", k ? ", " : "", CULTURE_STAGE_NAMES[na.one[k].stage], na.one[k].cells > 0 ? std::log10(na.one[k].cells) : 0.0);
        }
        check("lab: a culture's state closed-form from its events (Z-05)", ok && sameEvents && sameState && sameLaid && n == 2 && events >= 5,
              ok ? fmt("%d events (%d strains) by skips and by steps %s; at 40 min %d species (%s): stepped %s, laid out again %s", events, strains, sameEvents ? "the same" : "DIFFERENT",
                       n, stages.c_str(), sameState ? "the same" : "DIFFERENT", sameLaid ? "the same" : "DIFFERENT")
                 : why);
        // the lab's save round-trip: the culture's flask and a jar of the vial (its silt or not: a jar's line is its place's) saved, loaded into a
        // fresh game, saved again: the same lines, the same state
        bool round = false, sameNow = false;
        int lines = 0;
        if (ok) {
            a.testLab().jars.push_back(Jar());
            Jar& J = a.testLab().jars.back();
            const Vial& v = a.testColdStore()[0];
            J.sx = v.sx; J.sy = v.sy; J.sz = v.sz; J.body = v.body; J.hab = v.hab; J.name = "WET SILT"; J.t0 = 600;
            a.save("shots/tests/unit_lab_cultures.txt");
            std::string s1; a.testLabSave(s1);
            TestGame c; c.savePrefix = "shots/tests/unit_lab_save"; c.settingsPath = "shots/tests/test_settings.txt"; c.guidePath = "shots/tests/unit_lab_guide.txt";
            c.keysPath = "shots/tests/test_keys.txt";
            c.newGame(); c.setState(GameState::SPACE);
            c.load("shots/tests/unit_lab_cultures.txt");
            std::string s2; c.testLabSave(s2);
            round = s1 == s2;
            for (char ch : s1) lines += ch == '\n';
            const CultureNow x = a.testFlaskNow(0), y = c.testFlaskNow(0);
            sameNow = x.n == y.n && x.n == 2 && x.one[0].cells == y.one[0].cells && x.one[1].cells == y.one[1].cells && x.one[0].stage == y.one[0].stage &&
                      x.one[1].stage == y.one[1].stage;
            remove("shots/tests/unit_lab_cultures.txt");
        }
        check("lab: the lab's save round-trip (Z-05)", ok && round && sameNow,
              fmt("%d lines (the clock, a flask with its species and events, a jar): loaded and saved again %s; the culture at its clock %s", lines, round ? "the same" : "DIFFERENT",
                  sameNow ? "the same" : "DIFFERENT"));
        // R-433: the culture collection. Every species and strain the culture grew kept as it grew (each once), kept after its flask is emptied; a
        // folder made and the first moved into it (removed only once empty); the first on the stage, into another flask in the medium it grew in;
        // with the cold store and the flasks empty, the choosers' rows the collection's alone; its lines through the save
        int kept = 0, mine = 0, strains2 = 0, rowsI = 0, rowsS = 0;
        bool distinct = true, afterEmpty = false, folderOk = false, stage = false, intoFlask = false, chooseOk = false, saved = false;
        if (ok) {
            const Flask& A = a.testLab().flasks[0];
            const FlaskSpecies w0 = A.who[0], w1 = A.who[1];
            const int n = A.n;
            const std::vector<KeptCulture>& K = a.testLab().kept;
            kept = (int)K.size();
            for (size_t i = 0; i < K.size(); i++) {
                mine += K[i].who.sameSpecies(w0) || (n > 1 && K[i].who.sameSpecies(w1));
                strains2 += K[i].who.strain > 0;
                for (size_t j = 0; j < i; j++) distinct = distinct && !(K[j].who.sameSpecies(K[i].who) && K[j].who.strain == K[i].who.strain);
            }
            a.testFlaskEmpty(0);
            afterEmpty = (int)a.testLab().kept.size() == kept;
            const int id = a.testFolderNew("GLOWING ONES");
            a.testKeptMove(0, id);
            const std::vector<LabFolder> F = a.testCultureFolders();
            folderOk = id > 0 && F.size() == 3 && F[2].name == "GLOWING ONES" && F[2].id == id && F[2].rows.size() == 1 && F[2].rows[0] == 0 && F[1].rows.size() == K.size() - 1 &&
                       !a.testFolderRemove(id);
            stage = a.testScopeStageKept(0) && a.testScope().kept == 0 && a.testScope().page == 1 && a.testScope().species.size() == 1 && !a.testScope().mount.cells.empty();
            a.testColdStore().clear();
            for (int i = 0; i < INCUBATOR_FLASKS; i++) a.testFlaskEmpty(i);
            a.testIncub().sel = 5;
            const std::vector<IncubatorChoice> IC = a.testIncubatorChoices(0);
            rowsI = (int)IC.size();
            chooseOk = rowsI == kept;
            for (const IncubatorChoice& c : IC) chooseOk = chooseOk && c.kept >= 0;
            const std::vector<SequencerChoice> SC = a.testSequencerChoices();
            rowsS = (int)SC.size();
            for (const LabFolder& f : a.testLab().seq.pages.folders) chooseOk = chooseOk && f.name.rfind("COLLECTION: ", 0) == 0;
            const KeptCulture c0 = a.testLab().kept[0];
            intoFlask = a.testFlaskPutKept(5, 0) && a.testLab().flasks[5].n == 1 && a.testLab().flasks[5].who[0].sameSpecies(c0.who) &&
                        a.testLab().flasks[5].who[0].strain == c0.who.strain && a.testLab().flasks[5].medium == c0.medium;
            a.save("shots/tests/unit_lab_kept.txt");
            TestGame c; c.savePrefix = "shots/tests/unit_lab_save"; c.settingsPath = "shots/tests/test_settings.txt"; c.guidePath = "shots/tests/unit_lab_guide.txt";
            c.keysPath = "shots/tests/test_keys.txt";
            c.newGame(); c.setState(GameState::SPACE);
            c.load("shots/tests/unit_lab_kept.txt");
            const std::vector<KeptCulture>& X = c.testLab().kept;
            const std::vector<KeptCulture>& Y = a.testLab().kept;
            saved = X.size() == Y.size() && c.testLab().folders.size() == 1 && c.testLab().folders[0].id == id && c.testLab().folders[0].name == "GLOWING ONES";
            for (size_t i = 0; saved && i < X.size(); i++)
                saved = X[i].who.sameSpecies(Y[i].who) && X[i].who.strain == Y[i].who.strain && X[i].folder == Y[i].folder && X[i].medium == Y[i].medium &&
                        X[i].asWorld == Y[i].asWorld && X[i].t == Y[i].t;
            remove("shots/tests/unit_lab_kept.txt");
        }
        check("lab: the culture collection: kept as grown, in a folder, on the stage, into a flask, chosen, saved (R-433)",
              ok && kept >= 2 && mine == kept && strains2 > 0 && distinct && afterEmpty && folderOk && stage && intoFlask && chooseOk && rowsS > 0 && saved,
              fmt("%d kept (%d strains), every one the flask's %s, each once %s; after the flask emptied %s; a folder, the first in it %s; on the stage %s; "
                  "into flask 6 in its medium %s; the choosers' rows the collection's %s (the incubator's %d, the sequencer's %d); through the save %s",
                  kept, strains2, mine == kept ? "yes" : "NO", distinct ? "yes" : "NO", afterEmpty ? "kept" : "LOST", folderOk ? "yes" : "NO", stage ? "yes" : "NO",
                  intoFlask ? "yes" : "NO", chooseOk ? "yes" : "NO", rowsI, rowsS, saved ? "the same" : "DIFFERENT"));
    }
    {   // the lab's clock as the drones' bay's: real seconds in a simulating state (the ship's, the incubator's), none in a screen that stops the
        // clock (the Esc menu)
        TestGame g; Input in;
        g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_lab_guide.txt"; g.keysPath = "shots/tests/test_keys.txt";
        g.newGame(); g.setState(GameState::SPACE);
        g.testBayClock() = 0;
        auto second = [&]() { for (int k = 0; k < 20; k++) { g.frame(in, 0.05); in.newFrame(); } };
        const double l0 = g.testLab().clock, d0 = g.testBayClock();
        second();
        const double l1 = g.testLab().clock, d1 = g.testBayClock();
        g.openScreen(GameState::MENU);
        second();
        const double l2 = g.testLab().clock, d2 = g.testBayClock();
        g.closeScreen(); g.openScreen(GameState::INCUBATOR);
        second();
        const double l3 = g.testLab().clock, d3 = g.testBayClock();
        auto near = [](double x, double y) { return std::fabs(x - y) < 1e-9; };
        const bool right = near(l1 - l0, 1) && near(d1 - d0, 1) && near(l2, l1) && near(d2, d1) && near(l3 - l2, 1) && near(d3 - d2, 1);
        check("lab: the lab's clock runs as the drones' does, in the simulating states (Z-05)", right,
              fmt("a second in space: the lab %+.2f s, the bay %+.2f; in the Esc menu %+.2f and %+.2f; at the incubator %+.2f and %+.2f", l1 - l0, d1 - d0, l2 - l1, d2 - d1, l3 - l2, d3 - d2));
    }
    {   // two in a flask, the four ways, of kinds made for it: two heterotrophs (one wins), a grazer and a small cell (they cycle), a light-feeder and a
        // heterotroph with no broth (they feed each other), a heterotroph of each hand (they ignore each other); an hour each, the second at 2 minutes
        auto kind = [](uint64_t seed, int way, int plan, double body, bool mirrored) {
            CultureKind k;
            k.s.state = SS_LIVING; k.s.seed = seed; k.s.way = way; k.s.plan = plan; k.s.pigment = way == LW_LIGHT ? PG_GREEN : PG_NONE;
            k.s.tMin = 10; k.s.tOpt = 30; k.s.tMax = 40; k.s.saltLo = k.s.saltHi = 0;
            k.s.cycle = LC_GROWING | LC_DIVIDING | LC_DYING;
            k.chemistry = CH_WATER; k.mirrored = mirrored; k.bodyUm = body;
            return k;
        };
        const CultureKind rod = kind(11, LW_DEAD, MP_ROD, 2, false), rod2 = kind(12, LW_DEAD, MP_ROD, 2.5, false), grazer = kind(13, LW_GRAZER, MP_FLAGELLATE, 20, false),
                          alga = kind(14, LW_LIGHT, MP_SPHERE, 4, false), mirror = kind(15, LW_DEAD, MP_ROD, 2, true);
        struct Two { const CultureKind *a, *b; int energy, light, want; const char* what; };
        const Two twos[] = {{&rod, &rod2, CE_BROTH, CL_DARK, PA_COMPETE, "two heterotrophs"}, {&grazer, &rod, CE_BROTH, CL_DARK, PA_CYCLE, "a grazer and a rod"},
                            {&alga, &rod, CE_NONE, CL_WHITE, PA_FEED, "a light-feeder and a heterotroph"}, {&rod, &mirror, CE_BROTH, CL_DARK, PA_IGNORE, "two of mirrored hands"}};
        int right = 0;
        std::string lines;
        for (const Two& w : twos) {
            FlaskCulture c;
            c.kind[0] = *w.a; c.kind[1] = *w.b;
            CultureEvent e; e.kind = CD_START; e.t = 0; e.m.cell = CC_WATER; e.m.tempC = 30; e.m.energy = w.energy; e.m.light = w.light;
            c.events.push_back(e);
            e.kind = CD_ADD; e.t = 120; c.events.push_back(e);
            const CultureRun R = runCulture(c);
            double lo = 1e30, hi = 0;   // the second's range of numbers over its last half hour (a cycle's swing)
            for (int t = 1800; t <= 3600; t += 30) { const double v = cultureAt(c, R, t).one[1].cells; lo = std::min(lo, v); hi = std::max(hi, v); }
            double most[2] = {0, 0};   // each one's most over the hour (B-444: its crowd held a while, then its decline)
            for (int t = 0; t <= 3600; t += 30) for (int k = 0; k < 2; k++) most[k] = std::max(most[k], cultureAt(c, R, t).one[k].cells);
            const CultureNow end = cultureAt(c, R, 3600), mid = cultureAt(c, R, 1800);
            bool ok = R.pair == w.want;
            if (w.want == PA_COMPETE) ok = ok && (end.one[0].stage == SG_OUTGROWN || end.one[1].stage == SG_OUTGROWN || end.one[0].cells == 0 || end.one[1].cells == 0);
            if (w.want == PA_CYCLE) ok = ok && R.period > 0 && R.cycleT > 0 && end.one[0].cells > 0 && end.one[1].cells > 0 && hi > 10 * lo;
            if (w.want == PA_FEED) ok = ok && mid.one[1].cells >= 1e5;   // the heterotroph fed with no broth of its own
            if (w.want == PA_IGNORE) ok = ok && most[0] >= 1e8 && most[1] >= 1e8;   // each to its crowd, as alone
            right += ok;
            lines += fmt("%s%s: %s%s", lines.empty() ? "" : "; ", w.what, CULTURE_PAIR_NAMES[R.pair], w.want == PA_CYCLE ? fmt(" (a turn of %.0f s, a %.1f-decade swing)", R.period, std::log10(hi / std::max(lo, 1e-9))).c_str() : "");
        }
        check("lab: two in a flask, the four ways (Z-05)", right == 4, fmt("%d of 4 as they should be: %s", right, lines.c_str()));
        {   // B-446: a switched prey in a grazer's cycle. The rod carries a gene moved in from its world's kin (3%) set WHEN CROWDED, so it pays
            // only past 3% of its crowd. Every 5 s from the cycle's start to the hour, its state must say CROWDED exactly when its numbers are
            // past that share, and each part of the cycle must grow at the rate of its state
            CultureKind line = rod;
            CultureSwitch sw; sw.place = 4; sw.when = GW_CROWDED; sw.price = 0.03;
            line.switches.push_back(sw); line.burden = 0.03;
            FlaskCulture c;
            c.kind[0] = grazer; c.kind[1] = line;
            CultureEvent e; e.kind = CD_START; e.t = 0; e.m.cell = CC_WATER; e.m.tempC = 30; e.m.energy = CE_BROTH; e.m.light = CL_DARK;
            c.events.push_back(e);
            e.kind = CD_ADD; e.t = 120; c.events.push_back(e);
            const CultureRun R = runCulture(c);
            const double Nq = line.quorum * std::pow(10.0, growthIn(line, e.m).crowdLog);
            int times = 0, match = 0, parts = 0, crowdedParts = 0, rightRate = 0;
            for (double t = std::max(0.0, R.cycleT) + 5; t <= 3600; t += 5) {
                const CultureOne o = cultureAt(c, R, t).one[1];
                if (o.cells <= 0 || std::fabs(std::log(o.cells / Nq)) < 1e-6) continue;   // on the line itself: either is right
                times++;
                match += (o.state >= 0 && ((o.state & CST_CROWDED) != 0) == (o.cells >= Nq));
            }
            double rFree = 0;
            for (const CultureSeg& s : R.seg[1]) if (s.stage == SG_CYCLING && s.state >= 0 && !(s.state & CST_CROWDED)) rFree = s.r;
            for (const CultureSeg& s : R.seg[1]) {
                if (s.stage != SG_CYCLING) continue;
                parts++;
                const bool crowded = s.state >= 0 && (s.state & CST_CROWDED);
                crowdedParts += crowded;
                rightRate += rFree > 0 && std::fabs(s.r / rFree - (crowded ? 0.97 : 1.0)) < 1e-9;
            }
            const bool ok = R.pair == PA_CYCLE && R.cycleT > 0 && times > 100 && match == times && crowdedParts > 0 && crowdedParts < parts && rightRate == parts;
            check("lab: a switched strain in a predator cycle follows its numbers (B-446)", ok,
                  fmt("the rod's gene WHEN CROWDED (3%%, past %.2e cells): its state right at %d of %d times; %d cycle parts, %d crowded, %d at their state's rate",
                      Nq, match, times, parts, crowdedParts, rightRate));
        }
    }
    {   // B-444: a culture's hold at its crowd ends. Every living species of forty worlds alone at its best (its warmth, its salt, its light and its
        // energy), once at its crowd, has moved on fifteen minutes and a second later (its hold's longest): resting, dying or fruiting, never held
        // at its crowd for good by its hold's end lost to a rounding
        int worlds = 0, species = 0, crowded = 0, onward = 0;
        std::string stuck;
        for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return worlds < 40; })) {
            auto L = lifeOf(sys);
            for (int b = 0; b < (int)sys.bodies.size() && worlds < 40; b++) {
                const Biota& B = L->bodies[b];
                if (!livingStage(B.stage)) continue;
                worlds++;
                const std::vector<Morph> forms = morphsOf(sys, b);
                for (const Microbe& m : B.species) {
                    if (m.state != SS_LIVING) continue;
                    species++;
                    FlaskCulture c;
                    c.kind[0] = cultureKindOf(B, m.index, 2 * forms[m.index].bodyUm());
                    CultureEvent e; e.kind = CD_START; e.t = 0;
                    e.m.cell = chemistryCell(c.kind[0].chemistry); e.m.tempC = std::round(m.tOpt); e.m.salt = m.saltLo;
                    e.m.energy = m.way == LW_CHEMICAL ? m.substrate : (m.way == LW_LIGHT ? CE_NONE : CE_BROTH); e.m.light = m.way == LW_LIGHT ? CL_WHITE : CL_DARK;
                    c.events.push_back(e);
                    const CultureRun R = runCulture(c);
                    double tc = -1;
                    for (const CultureSeg& s : R.seg[0]) if (tc < 0 && s.stage == SG_CROWD) tc = s.t0;
                    if (tc < 0) continue;
                    crowded++;
                    const int st = cultureAt(c, R, tc + 901).one[0].stage;
                    if (st != SG_CROWD) onward++;
                    else if (stuck.size() < 200) stuck += fmt("%s%s M%d", stuck.empty() ? "" : ", ", sys.bodies[b].name.c_str(), m.index);
                }
            }
        }
        check("lab: a culture's hold at its crowd ends: past it, it rests, dies or fruits (B-444)", crowded > 100 && onward == crowded,
              fmt("%d species of %d worlds at their best, %d at their crowd, %d moved on past their hold%s%s", species, worlds, crowded, onward, stuck.empty() ? "" : "; held: ",
                  stuck.c_str()));
    }
    // Z-06: the genomes and the tree, over the species of the systems within six sectors of home (each of its own, alive or asleep)
    struct Sp { StarSystem* sys; int body, species; Lineage line; };
    std::vector<std::unique_ptr<StarSystem>> systems;
    std::vector<Sp> sps;
    for (const Star& st : starsNearHome(6)) {
        systems.push_back(std::make_unique<StarSystem>()); systems.back()->generate(st);
        StarSystem* S = systems.back().get();
        auto L = lifeOf(*S);
        for (int i = 0; i < (int)S->bodies.size(); i++)
            for (const Microbe& m : L->bodies[i].species) { const Lineage ln = lineageOf(*S, i, m.index); if (ln.valid) sps.push_back({S, i, m.index, ln}); }
    }
    {   // sequences deterministic: a species' lineage and marker the same from its system drawn again (its cache gone), at one thread, and a carried
        // spore's its source's
        int same = 0, carried = 0, carriedSame = 0, n = 0;
        for (size_t k = 0; k < sps.size(); k += 7) {
            const Sp& p = sps[k];
            StarSystem again; again.generate(p.sys->star);
            const Lineage b = lineageOf(again, p.body, p.species);
            n++;
            same += b.valid && markerOf(b, 0, MARKER_LETTERS) == markerOf(p.line, 0, MARKER_LETTERS) && commonAncestorGyr(b, p.line) == commonAncestorGyr(p.line, p.line) &&
                    b.leaf == p.line.leaf && b.exchangeGyr == p.line.exchangeGyr;
        }
        for (const Sp& p : sps) {
            const Microbe& m = lifeOf(*p.sys)->bodies[p.body].species[p.species];
            if (m.fromBody < 0) continue;
            carried++;
            const Lineage src = lineageOf(*p.sys, m.fromBody, m.fromIndex);
            carriedSame += markerOf(src, 0, MARKER_LETTERS) == markerOf(p.line, 0, MARKER_LETTERS) && std::fabs(commonAncestorGyr(src, p.line) - m.ageYears * 1e-9) < 1e-12;
        }
        check("lab: a species' genome a pure function of it and its genesis (Z-06)", n > 50 && same == n && carriedSame == carried,
              fmt("%d of %d species the same drawn again, %d of %d carried spores their source's (%zu species within six sectors)", same, n, carriedSame, carried, sps.size()));
    }
    {   // the distances rank as the lineages: within a deep branch, every species' kin on its world nearer than any on another world of its system,
        // and those nearer than any of another star; the markers alike in that order on average
        long wrong = 0, pairs[3] = {0, 0, 0};
        double alike[3] = {0, 0, 0}; long alikeN[3] = {0, 0, 0};
        for (size_t i = 0; i < sps.size(); i++) {
            double hi[3] = {-1, -1, -1}, lo[3] = {1e9, 1e9, 1e9};
            for (size_t j = 0; j < sps.size(); j++) {
                if (i == j) continue;
                const Lineage &a = sps[i].line, &b = sps[j].line;
                if (a.genesis.id != b.genesis.id || a.clade != b.clade) continue;
                const int w = (a.lx != b.lx || a.ly != b.ly || a.lz != b.lz) ? 2 : ((a.world != b.world || a.wx != b.wx) ? 1 : 0);
                const double ca = commonAncestorGyr(a, b);
                hi[w] = std::max(hi[w], ca); lo[w] = std::min(lo[w], ca); pairs[w]++;
                if ((i + j) % 11 == 0) { alike[w] += markerAlike(a, b); alikeN[w]++; }
            }
            if (hi[0] >= 0 && lo[1] < 1e9 && hi[0] >= lo[1]) wrong++;
            if (hi[1] >= 0 && lo[2] < 1e9 && hi[1] >= lo[2]) wrong++;
        }
        double m[3]; for (int w = 0; w < 3; w++) m[w] = alikeN[w] ? alike[w] / alikeN[w] : 0;
        check("lab: the distances rank as the lineages: a world's, a system's, a cell's (Z-06)", wrong == 0 && pairs[0] > 100 && pairs[1] > 100 && pairs[2] > 100 && m[0] > m[1] && m[1] > m[2],
              fmt("%ld out of order; %ld, %ld and %ld pairs; their markers %.0f%%, %.0f%% and %.0f%% alike", wrong, pairs[0], pairs[1], pairs[2], 100 * m[0], 100 * m[1], 100 * m[2]));
    }
    {   // no common ancestor across geneses (and none read in two alphabets)
        long across = 0, none = 0, alphabets = 0;
        for (size_t i = 0; i < sps.size(); i += 3)
            for (size_t j = i + 1; j < sps.size(); j += 5) {
                if (sps[i].line.genesis.id == sps[j].line.genesis.id) continue;
                across++; none += commonAncestorGyr(sps[i].line, sps[j].line) < 0;
                if (sps[i].line.genesis.letters != sps[j].line.genesis.letters) alphabets += markerAlike(sps[i].line, sps[j].line) == 0;
                else alphabets++;
            }
        check("lab: no common ancestor across geneses (Z-06)", across > 1000 && none == across && alphabets == across, fmt("%ld pairs of two geneses, %ld with none", across, none));
    }
    {   // mirrored species ignore each other in a flask: a species of a mirrored genesis and one of home's hand, each a heterotroph of its own world,
        // always; two of one hand never
        int ignore = 0, mixed = 0, sameIgnore = 0, same = 0;
        std::vector<CultureKind> ours, theirs;
        for (const Sp& p : sps) {
            const Biota& B = lifeOf(*p.sys)->bodies[p.body];
            const Microbe& mm = B.species[p.species];
            if (mm.state != SS_LIVING || mm.way != LW_DEAD) continue;
            (p.line.genesis.mirrored ? theirs : ours).push_back(cultureKindOf(B, p.species, 2.0));
        }
        for (size_t i = 0; i < ours.size() && i < 20; i++) {
            int lead = 0;
            for (size_t j = 0; j < theirs.size() && j < 20; j++) { mixed++; ignore += pairOf(ours[i], theirs[j], lead) == PA_IGNORE; }
            for (size_t j = i + 1; j < ours.size() && j < 20; j++) { same++; sameIgnore += pairOf(ours[i], ours[j], lead) == PA_IGNORE; }
        }
        check("lab: mirrored species ignore each other in a flask (Z-06)", mixed >= 20 && ignore == mixed && same >= 20 && sameIgnore == 0,
              fmt("%d of %d pairs of two hands ignore each other, %d of %d of one hand", ignore, mixed, sameIgnore, same));
    }
    {   // the sequencer's reading on the lab's clock (told at its end, its line round-trip) and the tree: home's genesis first, the ship's own at its root
        TestGame g; Input in;
        g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_lab_guide3.txt"; g.keysPath = "shots/tests/test_keys.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.setState(GameState::SPACE);
        std::vector<std::string> keys;
        for (size_t k = 0; k < sps.size() && keys.size() < 6; k += 37) {
            const Sp& p = sps[k];
            keys.push_back(Guide::bodyKey(p.sys->star.sx, p.sys->star.sy, p.sys->star.sz, p.body) + "/M" + std::to_string(p.species));
            g.guide.microbes[keys.back()].seen |= SEEN_FORM;
        }
        const Sp& first = sps[0];
        const bool put = g.testSeqPut(first.sys->star.sx, first.sys->star.sy, first.sys->star.sz, first.body, first.species);
        g.testLabSkip(SEQUENCE_S - 1);
        const bool early = g.testLab().seq.told;
        std::string line; g.testSeqSave(line);
        TestGame g2; g2.newGame(); g2.setState(GameState::SPACE);
        g2.testLab().clock = g.testLab().clock;
        g2.testSeqLoad(line.substr(0, line.size() - 1));
        g.testLabSkip(2); g2.testLabSkip(2);
        const std::string k0 = Guide::bodyKey(first.sys->star.sx, first.sys->star.sy, first.sys->star.sz, first.body) + "/M" + std::to_string(first.species);
        int kin = 0; for (const LogEntry& e : g.guide.log) kin += e.kind == "KIN";
        const bool told = g.testLab().seq.told && g2.testLab().seq.told && (g.guide.microbes[k0].seen & SEEN_KIN) && kin == 1 && g.testLab().seq.result == g2.testLab().seq.result;
        for (const std::string& k : keys) g.guide.microbes[k].seen |= SEEN_KIN;
        const TreeState& T = g.testTree();
        int ship = 0; bool homeFirst = !T.leaves.empty() && T.leaves[0].line.genesis.id == HOME_GENESIS_ID, homeRun = true, seenOther = false;
        std::set<uint64_t> geneses;
        for (const TreeLeaf& L : T.leaves) {
            ship += L.line.ship;
            geneses.insert(L.line.genesis.id);
            if (L.line.genesis.id != HOME_GENESIS_ID) seenOther = true;
            else if (seenOther) homeRun = false;
        }
        bool joins = !T.join.empty() && T.join[0] < 0;
        for (size_t k = 1; k < T.leaves.size(); k++) joins = joins && (T.join[k] < 0) == (T.leaves[k].line.genesis.id != T.leaves[k - 1].line.genesis.id);
        check("lab: the sequencer told at its reading's end; home's species at the tree's root (Z-06)",
              put && !early && told && homeFirst && homeRun && ship == SHIP_SPECIES && joins && T.geneses == (int)geneses.size(),
              fmt("put %s, told at %.0f s %s, its line '%s' %s; the tree: %zu leaves, %s first, the ship's own %d, %d geneses",
                  put ? "on" : "NOT", SEQUENCE_S, early ? "EARLY" : (told ? "on time" : "NOT"), line.substr(0, line.size() - 1).c_str(), g2.testLab().seq.told ? "loaded" : "NOT loaded",
                  T.leaves.size(), homeFirst ? "home's" : "NOT home's", ship, T.geneses));
        remove(g.guidePath.c_str());
    }
    {   // Z-07: the record. A friend's file lends their own sheets (not those lent to them, nor one the explorer saw: the explorer's own wins), the
        // catalogue's rows of them cyan and no count (the LIFE page's and the data sheet's the explorer's own); the guide's new lines (the samples'
        // count and worlds, a woken spore's years, the sheets lent) read back and saved again byte for byte
        TestGame g;
        g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_lab_guide4.txt"; g.keysPath = "shots/tests/test_keys.txt";
        const std::string inbox = "shots/tests/unit_lab_guide4_inbox.txt", again = "shots/tests/unit_lab_guide4_again.txt";
        remove(g.guidePath.c_str()); remove(inbox.c_str());
        g.newGame(); g.setState(GameState::SPACE);
        std::vector<std::string> keys;
        for (size_t k = 0; k < sps.size() && keys.size() < 7; k += 29) {
            const Sp& p = sps[k];
            keys.push_back(Guide::bodyKey(p.sys->star.sx, p.sys->star.sy, p.sys->star.sz, p.body) + "/M" + std::to_string(p.species));
        }
        bool ok = keys.size() == 7;
        if (ok) {
            for (int i = 0; i < 3; i++) { MicrobeSheet& m = g.guide.microbes[keys[i]]; m.seen = SEEN_FORM; m.forms = 1u << CF_CELL; }
            MicrobeSheet& w = g.guide.microbes[keys[0]]; w.stages = LC_DORMANT | LC_WAKING; w.sleptYears = 4.5e7;
            g.guide.samplesTaken = 5; g.guide.sampled.insert(keys[0].substr(0, keys[0].rfind("/M")));
            g.guide.save(g.guidePath);
            Guide F;
            for (int i = 3; i < 5; i++) { MicrobeSheet& m = F.microbes[keys[i]]; m.seen = SEEN_FORM | SEEN_MOTION; m.forms = 1u << CF_CELL; }
            F.microbes[keys[2]].seen = SEEN_FORM | SEEN_DNA;   // the explorer saw it too
            F.lentMicrobes[keys[5]] = MicrobeSheet();          // lent to the friend: never travels on
            F.names[keys[3]] = "GLASSWORT";
            F.save(inbox);
        }
        g.importInboxFile();
        const bool lent = ok && g.guide.lentMicrobes.size() == 2 && g.guide.lentMicrobes.count(keys[3]) && g.guide.lentMicrobes.count(keys[4]) &&
                          !g.guide.lentMicrobes.count(keys[2]) && !g.guide.lentMicrobes.count(keys[5]) && g.guide.microbes.size() == 3 && g.guide.isLentMicrobe(keys[3]);
        g.openScreen(GameState::CATALOGUE);
        int lentRows = 0, ownRows = 0;
        for (const CatalogueRow& r : g.testCat().rows) { lentRows += !r.head && r.lent; ownRows += !r.head && !r.lent; }
        Guide R; R.load(g.guidePath); R.save(again);
        std::ifstream a(g.guidePath), b(again);
        std::stringstream sa, sb; sa << a.rdbuf(); sb << b.rdbuf();
        const bool back = ok && R.lentMicrobes.size() == 2 && R.microbes.size() == 3 && R.microbes[keys[0]].sleptYears == 4.5e7 && R.samplesTaken == 5 && R.sampled.size() == 1;
        check("lab: a friend's sheets lent, in cyan and no count; the record's lines byte-stable (Z-07)", lent && lentRows == 2 && ownRows == 3 && back && sa.str() == sb.str(),
              fmt("%zu lent of the friend's 3 own (1 seen by the explorer) and 1 lent to them; the catalogue's rows %d own, %d lent; the guide read back %s, saved again %s",
                  g.guide.lentMicrobes.size(), ownRows, lentRows, back ? "whole" : "NOT WHOLE", sa.str() == sb.str() ? "byte for byte" : "CHANGED"));
        remove(g.guidePath.c_str()); remove(inbox.c_str()); remove(again.c_str());
    }
    remove("shots/tests/unit_lab_guide.txt"); remove("shots/tests/unit_lab_guide2.txt"); remove("shots/tests/unit_lab_vials.txt");
}
