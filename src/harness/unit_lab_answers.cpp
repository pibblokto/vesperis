// R-435 to R-438 and B-441 (2026-11-19; the unit's `experiments` group, after `lab`): the stage's warmth set (a kind of the slide waking in its
// range, its pace by its warmth, stopped in the cold, stressed past its top); every cell that turns to the light or the food within its reach
// answering it, the dark's swimmers fleeing the light, lit light-feeders drawing the eaters (Engelmann); the microscope's keys (the magnet's three
// presses, the food rinsed, the stage's warmth typed and kept) and what the view's kinds answer said and kept on their sheets through the guide's
// save; the incubator's warmth typed; a culture grown on its partner kept with it, staged and put in with it, repaired after a load
#include "harness/harness.h"
#include "harness/test_game.h"
#include "life/slides.h"
#include "life/cultures.h"
#include <cmath>
#include <map>
#include <tuple>

namespace {
// a game in the microscope on a vial, its guide fresh: the densest slide the lab knows (an abundant swimmer that turns to light among large
// armoured cells), else the first felisian's ground
bool scopeOn(TestGame& g, Input& in, const char* guide, bool dense) {
    g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = guide; g.keysPath = "shots/tests/test_keys.txt";
    remove(guide);
    g.newGame(); g.setState(GameState::SPACE);
    Vial v;
    StarSystem sys; int bi = -1;
    if (dense ? !labDenseVial(v) : (!firstBodyOfType(PT_FELISIAN, sys, bi) || !labVial(sys, bi, v))) return false;
    g.testColdStore().push_back(v);
    g.openScreen(GameState::MICROSCOPE);
    g.frame(in, 0); in.newFrame();
    return g.testScopeStage(0);
}
// the slide's most crowded kind that lives and swims at the vial's warmth (-1 none), another of them (none but it: -1)
int swimmerOf(const SlideMount& M, int not_ = -1) {
    int best = -1;
    for (int k = 0; k < (int)M.kinds.size(); k++) {
        const SlideKind& K = M.kinds[k];
        if (k != not_ && K.lives && K.active && K.speed > 0 && (best < 0 || K.n / (K.tile * K.tile) > M.kinds[best].n / (M.kinds[best].tile * M.kinds[best].tile))) best = k;
    }
    return best;
}
// a view of a half-width round a place (a bright field at the first look's magnification)
SlideLook lookAt(const SlideMount& M, double x, double y, double half) {
    SlideLook L; L.cx = x; L.cy = y; L.halfW = L.halfH = half; L.mag = M.mag; L.focus = M.focus; L.seeing = WS_BRIGHT;
    return L;
}
// a kind's copies at a time within a radius of a place
int copiesNear(const SlideMount& M, const std::vector<Morph>& forms, const std::vector<SlideEvent>& ev, double t, int kind, double x, double y, double r) {
    std::vector<Specimen> s; std::vector<SlideRef> refs;
    mountSpecimens(M, forms, ev, t, lookAt(M, x, y, r * 1.1), s, &refs);
    int n = 0;
    for (const SlideRef& f : refs) n += M.cells[f.cell].kind == kind && std::hypot(f.x - x, f.y - y) <= r;
    return n;
}
// the share of a kind's copies in a view that moved more than a micrometre between two times (a still cell's jiggle is less)
double movedShare(const SlideMount& M, const std::vector<Morph>& forms, const std::vector<SlideEvent>& ev, double t0, double t1, int kind, const SlideLook& L) {
    std::vector<Specimen> a, b; std::vector<SlideRef> ra, rb;
    mountSpecimens(M, forms, ev, t0, L, a, &ra);
    mountSpecimens(M, forms, ev, t1, L, b, &rb);
    std::map<std::tuple<int, long long, long long>, size_t> at;
    for (size_t i = 0; i < rb.size(); i++) at[{rb[i].cell, (long long)rb[i].ti, (long long)rb[i].tj}] = i;
    int n = 0, moved = 0;
    for (size_t i = 0; i < ra.size(); i++) {
        if (M.cells[ra[i].cell].kind != kind) continue;
        n++;
        const auto it = at.find({ra[i].cell, (long long)ra[i].ti, (long long)ra[i].tj});
        moved += it == at.end() || std::hypot(rb[it->second].x - ra[i].x, rb[it->second].y - ra[i].y) > 1;   // gone from the view: moved
    }
    return n ? (double)moved / n : 0;
}
// a game's slide run on to a time a half second a frame (the HUD's lines said in their turn), every status said collected
void runScope(TestGame& g, Input& in, double until, std::string& said) {
    while (g.testScope().t < until) {
        g.testScope().t += 0.5;
        g.frame(in, 0); in.newFrame();
        const std::string s = g.testStatusKept().first;
        if (!s.empty() && said.find(s) == std::string::npos) said += s + " | ";
        g.testClearStatus();
    }
}
void press(TestGame& g, Input& in, int key, bool shift = false) {
    if (shift) in.down[KEY_LEFT_SHIFT] = true;
    pressKey(g, in, key);
    in.down[KEY_LEFT_SHIFT] = false;
}
}   // namespace

void unitLabAnswers(const Check& check) {
    {   // R-438: the stage set to a warmth. The dense slide's swimmer made one sampled where it is too warm for it (its range set under the vial's
        // warmth): still there, over its range; the stage set to its range's middle, it moves (slideAnswer and its copies); its pace by its warmth
        // (its own clock quicker set at its best than near its floor); set past its top, stressed into its last form; set under its floor, still
        TestGame g; Input in;
        const bool ok = scopeOn(g, in, "shots/tests/unit_lab_guide.txt", true);
        int k = -1, sp = -1; bool hot = false, woke = false, quick = false, stressed = false, cold = false;
        double stillMoved = 1, wokeMoved = 0, clockBest = 0, clockFloor = 0, lastForm = 0;
        if (ok) {
            SlideMount M = g.testScope().mount;
            const std::vector<Morph>& forms = g.testScope().forms;
            k = swimmerOf(M);
            if (k >= 0) {
                SlideKind& K = M.kinds[k];
                sp = K.species;
                const double T0 = M.stageC;
                K.tMax = T0 - 4; K.tMin = T0 - 24; K.tOpt = T0 - 9; K.active = false; K.paceRef = 1;
                const SlideLook L = lookAt(M, M.cx, M.cy, umPerPxAt(M.mag) * UW * 0.5);
                auto setTo = [](double c) { return std::vector<SlideEvent>{{SD_WARM, 1, c, 0, 0, true}}; };
                hot = (slideAnswer(M, {}, k, 30) & SA_HOT) != 0;
                stillMoved = movedShare(M, forms, {}, 20, 30, k, L);
                const std::vector<SlideEvent> mid = setTo(T0 - 14);
                woke = (slideAnswer(M, mid, k, 61) & SA_MOVING) != 0;
                wokeMoved = movedShare(M, forms, mid, 50, 60, k, L);
                auto clockAt = [&](double c) {
                    std::vector<Specimen> s; std::vector<SlideRef> r;
                    mountSpecimens(M, forms, setTo(c), 61, L, s, &r);
                    for (size_t i = 0; i < s.size(); i++) if (M.cells[r[i].cell].kind == k) return s[i].clock;
                    return -1.0;
                };
                clockBest = clockAt(K.tOpt); clockFloor = clockAt(K.tMin + 1);
                quick = clockFloor > 0 && clockBest > clockFloor * 1.5;
                const std::vector<SlideEvent> past = setTo(K.tMax + 10);
                stressed = (slideAnswer(M, past, k, 120) & SA_STRESSED) != 0;
                if (K.stress >= 0) {   // its copies in their last form (or still acting) by two minutes
                    std::vector<Specimen> s; std::vector<SlideRef> r;
                    mountSpecimens(M, forms, past, 120, L, s, &r);
                    int n = 0, done = 0;
                    for (size_t i = 0; i < s.size(); i++) if (M.cells[r[i].cell].kind == k) { n++; done += s[i].form == K.stressForm || s[i].act == K.stress; }
                    lastForm = n ? (double)done / n : 0;
                } else lastForm = 1;
                cold = (slideAnswer(M, setTo(K.tMin - 6), k, 120) & SA_COLD) != 0;
            }
        }
        check("experiments: the stage set to a warmth: a kind still out of its range moves in it, at its warmth's pace, stressed past its top, still in the cold (R-438)",
              ok && k >= 0 && hot && stillMoved < 0.05 && woke && wokeMoved > 0.5 && quick && stressed && lastForm > 0.9 && cold,
              ok ? fmt("the swimmer M%d made one too warm at the vial's: over its range %s, %.0f%% of its copies moved; set to its middle: moving %s, %.0f%% moved; "
                       "its clock at its best %.1f s against %.1f near its floor; past its top stressed %s (%.0f%% in their last form); under its floor still %s",
                       sp, hot ? "yes" : "NO", stillMoved * 100, woke ? "yes" : "NO", wokeMoved * 100, clockBest, clockFloor, stressed ? "yes" : "NO", lastForm * 100,
                       cold ? "yes" : "NO")
                 : std::string("NO DENSE SLIDE"));
    }
    {   // R-438: every cell within its reach answers. The dense slide's swimmer: its copies within four radii of a spot at the first look's place in
        // the spot's crowd (three radii) by 40 s, four in five of them at least; made an eater, the same for a drop of food by 60 s; made a dark-
        // dweller, its copies in the spot at 20 s a quarter of those at 0 at most; made an eater beside light-feeders (another kind, or itself, made
        // one), drawn in by the light alone: no more at 8 s, the crowd by 50 s, its answer SA_LIT
        TestGame g; Input in;
        const bool ok = scopeOn(g, in, "shots/tests/unit_lab_guide.txt", true);
        int k = -1, sp = -1, reachL = 0, inL = 0, reachF = 0, inF = 0, in0 = 0, in20 = 0, lit0 = 0, lit8 = 0, lit50 = 0;
        bool litSaid = false;
        if (ok) {
            const SlideMount M0 = g.testScope().mount;
            const std::vector<Morph>& forms = g.testScope().forms;
            k = swimmerOf(M0);
            if (k >= 0) {
                sp = M0.kinds[k].species;
                const double r = umPerPxAt(M0.mag) * UW * 0.12, x = M0.cx, y = M0.cy;
                const std::vector<SlideEvent> spot = {{SD_LIGHT, 1, x, y, r, true}}, drop = {{SD_FOOD, 1, x, y, r * 0.67, true}};
                SlideMount M = M0;
                M.kinds[k].taxis = TX_LIGHT; M.kinds[k].flees = false;
                reachL = copiesNear(M, forms, {}, 1, k, x, y, r * 4); inL = copiesNear(M, forms, spot, 41, k, x, y, r * 3);
                M.kinds[k].taxis = TX_FOOD;
                reachF = copiesNear(M, forms, {}, 1, k, x, y, r * 0.67 * 4); inF = copiesNear(M, forms, drop, 61, k, x, y, r * 0.67 * 3);
                M.kinds[k].taxis = 0; M.kinds[k].flees = true;
                in0 = copiesNear(M, forms, {}, 1, k, x, y, r); in20 = copiesNear(M, forms, spot, 21, k, x, y, r);
                M.kinds[k].taxis = TX_FOOD; M.kinds[k].flees = false;
                for (int j = 0; j < (int)M.kinds.size(); j++) if (j != k && M.kinds[j].lives && M.kinds[j].active) M.kinds[j].lightFeeder = true;
                M.kinds[k].lightFeeder = true;   // (its own crowd under the light lit too: the gift is the light-feeders')
                lit0 = copiesNear(M, forms, {}, 1, k, x, y, r * 3); lit8 = copiesNear(M, forms, spot, 8, k, x, y, r * 3); lit50 = copiesNear(M, forms, spot, 50, k, x, y, r * 3);
                litSaid = (slideAnswer(M, spot, k, 50) & SA_LIT) != 0;
            }
        }
        check("experiments: every cell within its reach answers: into the light, to the food; the dark's flee the light; lit light-feeders draw the eaters (R-438)",
              ok && k >= 0 && reachL > 1 && inL * 5 >= reachL * 4 && reachF > 1 && inF * 5 >= reachF * 4 && in0 > 0 && in20 * 4 <= in0 && lit8 <= lit0 + 1 &&
                  lit50 * 5 >= reachL * 4 && litSaid,
              ok && k >= 0 ? fmt("the swimmer M%d: in the light's crowd at 40 s %d of the %d within its reach; the food's at 60 s %d of %d; a dark-dweller, in the spot %d "
                                 "at 0 s, %d at 20; an eater by lit light-feeders: %d near at 0 s, %d at 8, %d at 50 (%s)",
                                 sp, inL, reachL, inF, reachF, in0, in20, lit0, lit8, lit50, litSaid ? "SA_LIT" : "NOT TOLD")
                           : std::string(ok ? "NO SWIMMER" : "NO DENSE SLIDE"));
    }
    {   // R-436 to R-438: the microscope's keys on the dense slide. M three times: beside the stage (its field to the right), turned (to the left),
        // taken away; F twice: a drop, rinsed; L twice; H and a warmth typed, Enter: the stage set to it; H, a digit, Esc: kept, the slide still on;
        // a warmth past the drop's liquid range refused, the entry still open; Shift+H: the vial's again. Then what is told: the light on, what
        // gathers in it said and kept on its sheet (TO LIGHT); the magnet's while judged (what follows it said, or that nothing does and where its
        // cells live; a moving kind not answering it kept so); the sheets' answers through the guide's save
        TestGame g; Input in;
        const bool ok = scopeOn(g, in, "shots/tests/unit_lab_guide.txt", true);
        bool magnet = false, food = false, light = false, typed = false, kept = false, refused = false, back = false, told = false, sheet = false, field = false,
             fieldKept = false, saved = false;
        int want = 0;
        std::string said, words;
        if (ok) {
            MicroscopeState& S = g.testScope();
            auto state = [&]() { return slideStateAt(S.mount, S.events, S.t); };
            auto typeNumber = [&](int v) { if (v < 0) press(g, in, KEY_MINUS); for (char c : std::to_string(std::abs(v))) press(g, in, KEY_0 + (c - '0')); };
            press(g, in, KEY_M); const SlideState m1 = state();
            press(g, in, KEY_M); const SlideState m2 = state();
            press(g, in, KEY_M); const SlideState m3 = state();
            magnet = m1.magnet && std::cos(m1.field) > 0.99 && m2.magnet && std::cos(m2.field) < -0.99 && !m3.magnet;
            press(g, in, KEY_F); const bool f1 = state().food; press(g, in, KEY_F); food = f1 && !state().food;
            press(g, in, KEY_L); const bool l1 = state().spot; press(g, in, KEY_L); light = l1 && !state().spot;
            const double vial = S.mount.stageC;
            want = (int)clampd(std::round(vial) + 4, S.stageLo, S.stageHi);
            press(g, in, KEY_H); const bool typing = S.typing;
            typeNumber(want); press(g, in, KEY_ENTER);
            typed = typing && !S.typing && std::fabs(state().setC - want) < 1e-9;
            press(g, in, KEY_H); press(g, in, KEY_5); press(g, in, KEY_ESCAPE);
            kept = !S.typing && std::fabs(state().setC - want) < 1e-9 && g.state == GameState::MICROSCOPE && S.page == 1;
            press(g, in, KEY_H); typeNumber((int)S.stageHi + 50); press(g, in, KEY_ENTER);
            refused = S.typing && std::fabs(state().setC - want) < 1e-9;
            press(g, in, KEY_ESCAPE);
            press(g, in, KEY_H, true);
            back = std::fabs(state().setC - vial) < 1e-9;
            const int k = swimmerOf(S.mount);
            const std::string key = k >= 0 ? g.testSpeciesKey(S.mount.kinds[k].species) : std::string();
            press(g, in, KEY_L);
            runScope(g, in, S.t + 12, said);
            told = said.find("GATHERS IN LIGHT") != std::string::npos;
            const auto it = g.guide.microbes.find(key);
            sheet = it != g.guide.microbes.end() && (it->second.answers & AW_LIGHT);
            if (k >= 0) for (const std::string& l : g.testSheetLines(S.mount.kinds[k].species)) if (l.rfind("TO LIGHT: ", 0) == 0) words = l;
            press(g, in, KEY_L);
            press(g, in, KEY_M);
            runScope(g, in, S.t + 20, said);
            field = said.find("FOLLOWS THE MAGNET") != std::string::npos ||
                    (said.find("NOTHING IN VIEW FOLLOWS THE MAGNET") != std::string::npos && said.find("SILT AND MUD") != std::string::npos);
            fieldKept = it != g.guide.microbes.end() && (it->second.answers & (AW_FIELD | AW_FIELD_NONE));
            g.guide.save("shots/tests/unit_lab_answers_guide.txt");
            Guide h;
            h.load("shots/tests/unit_lab_answers_guide.txt");
            if (it != g.guide.microbes.end() && h.microbes.count(key)) {
                const MicrobeSheet &a = it->second, &b = h.microbes[key];
                saved = a.answers == b.answers && a.movingLo == b.movingLo && a.movingHi == b.movingHi && a.stillCold == b.stillCold && a.stillHot == b.stillHot &&
                        a.stressAct == b.stressAct && a.answered();
            }
            remove("shots/tests/unit_lab_answers_guide.txt");
        }
        check("experiments: the microscope's keys: the magnet's three presses, the food rinsed, the stage's warmth typed and kept; what answers told and kept (R-436, R-437)",
              ok && magnet && food && light && typed && kept && refused && back && told && sheet && words == "TO LIGHT: GATHERS IN IT" && field && fieldKept && saved,
              ok ? fmt("M: beside, turned, away %s; F: a drop, rinsed %s; L on and off %s; H %d typed %s, Esc kept it %s, past the drop's range refused %s, Shift+H "
                       "the vial's %s; the light's answer said %s, on its sheet %s ('%s'); the magnet's while %s, kept %s; through the guide's save %s",
                       magnet ? "yes" : "NO", food ? "yes" : "NO", light ? "yes" : "NO", want, typed ? "set" : "NOT SET", kept ? "yes" : "NO", refused ? "yes" : "NO",
                       back ? "yes" : "NO", told ? "yes" : "NO", sheet ? "yes" : "NO", words.c_str(), field ? "said" : "NOT SAID", fieldKept ? "yes" : "NO",
                       saved ? "the same" : "DIFFERENT")
                 : std::string("NO DENSE SLIDE"));
    }
    {   // R-435: the incubator's warmth typed. A flask of the felisian vial's first living species: 3, 7, Enter on its page sets 37 C as one change of
        // its medium; 1, 5, 0, Enter (past the water cell's 125 C) refused, the entry open; Esc keeps 37 C, the screen the incubator's; 5, -, Enter
        // sets -5 C
        TestGame g; Input in;
        const bool ok = scopeOn(g, in, "shots/tests/unit_lab_guide.txt", false);
        bool put = false, set37 = false, one = false, refused = false, kept = false, minus = false;
        if (ok) {
            int sp = -1;
            for (const Held& H : g.testScope().sample.held) if (H.state == SS_LIVING) { sp = H.species; break; }
            put = sp >= 0 && g.testFlaskPut(0, 0, sp);
            if (put) {
                g.openScreen(GameState::INCUBATOR); g.frame(in, 0); in.newFrame();
                g.testIncub().sel = 0; g.testIncub().page = 0;
                const Flask& F = g.testLab().flasks[0];
                const size_t ev0 = F.events.size();
                press(g, in, KEY_3); press(g, in, KEY_7); press(g, in, KEY_ENTER);
                set37 = F.medium.tempC == 37; one = F.events.size() == ev0 + 1;
                press(g, in, KEY_1); press(g, in, KEY_5); press(g, in, KEY_0); press(g, in, KEY_ENTER);
                refused = g.testIncub().typing && F.medium.tempC == 37;
                press(g, in, KEY_ESCAPE);
                kept = !g.testIncub().typing && F.medium.tempC == 37 && g.state == GameState::INCUBATOR;
                press(g, in, KEY_5); press(g, in, KEY_MINUS); press(g, in, KEY_ENTER);
                minus = F.medium.tempC == -5;
            }
        }
        check("experiments: the incubator's warmth typed: set as one change, refused past its cell's range, kept by Esc, below zero (R-435)",
              ok && put && set37 && one && refused && kept && minus,
              fmt("a flask %s; 3 7 Enter: 37 C %s, one change of its medium %s; 150 refused %s; Esc kept it, the incubator on %s; 5 - Enter: -5 C %s",
                  put ? "put in" : "NOT PUT IN", set37 ? "yes" : "NO", one ? "yes" : "NO", refused ? "yes" : "NO", kept ? "yes" : "NO", minus ? "yes" : "NO"));
    }
    {   // B-441: a culture grown on its partner's gift kept with it. The scan region's first world with a light-feeder and a heterotroph that feed each
        // other in a flask of white light and no energy of its own: the heterotroph kept (alone it reads NOTHING IT EATS there) with the light-feeder;
        // staged, the drop holds both; put in an empty flask, both go in; through the save its partner kept; R-440: the pair kept once (either way
        // round); kept alone (by hand), on the stage its cells live (a hungry cell swims)
        TestGame g; Input in;
        g.savePrefix = "shots/tests/unit_lab_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_lab_guide.txt";
        g.keysPath = "shots/tests/test_keys.txt";
        remove(g.guidePath.c_str());
        g.newGame(); g.setState(GameState::SPACE);
        bool found = false, kept = false, paired = false, staged = false, into = false, saved = false, once = false, lives = false;
        std::string world;
        int tries = 0;
        for (auto& [x, z, s, scanned] : scanSystems(150, 340, 20, 160, [&] { return !found && tries < 40; })) {
            const StarSystem& sys = scanned;
            auto L = lifeOf(sys);
            for (int bi = 0; bi < (int)sys.bodies.size() && !found && tries < 40; bi++) {
                const Biota& B = L->bodies[bi];
                if (!livingStage(B.stage)) continue;
                for (const Microbe& a : B.species) {
                    if (found || tries >= 40 || a.state != SS_LIVING || a.way != LW_LIGHT) continue;
                    for (const Microbe& b : B.species) {
                        if (found || tries >= 40 || b.state != SS_LIVING || b.way != LW_DEAD) continue;
                        const double lo = std::max(a.tMin, b.tMin) + 1, hi = std::min(a.tMax, b.tMax) - 1;
                        const int salt = std::max(a.saltLo, b.saltLo);
                        if (hi <= lo || salt > std::min(a.saltHi, b.saltHi)) continue;
                        double warm = lo, both = -1;   // the warmth both grow best at, each at two fifths of its best at least
                        for (double c = std::ceil(lo); c <= hi; c += 1) {
                            const double f = std::min(cardinalRate(c, a.tMin, a.tOpt, a.tMax), cardinalRate(c, b.tMin, b.tOpt, b.tMax));
                            if (f > both) { both = f; warm = c; }
                        }
                        if (both < 0.4) continue;
                        tries++;
                        for (int i = 0; i < INCUBATOR_FLASKS; i++) g.testFlaskEmpty(i);
                        g.testColdStore().clear(); g.testLab().kept.clear();
                        Vial v; v.sx = sys.star.sx; v.sy = sys.star.sy; v.sz = sys.star.sz; v.body = bi;
                        v.hab.niche = NI_LIQUID; v.hab.meanC = warm; v.hab.salt = salt; v.hab.place = sys.bodies[bi].seed; v.readingC = v.hab.meanC; v.name = "WATER";
                        g.testColdStore().push_back(v);
                        if (!g.testFlaskPut(0, 0, a.index) || !g.testFlaskPut(0, 0, b.index)) continue;
                        Medium m = g.testLab().flasks[0].medium;
                        m.tempC = warm; m.light = CL_WHITE; m.energy = CE_NONE; m.salt = salt;
                        g.testFlaskSet(0, m);
                        g.testFlaskNow(0);
                        if (g.testLab().flasks[0].run.pair != PA_FEED) continue;
                        found = true; world = fmt("M%d and M%d of %s", a.index, b.index, Guide::bodyKey(v.sx, v.sy, v.sz, bi).c_str());
                    }
                }
            }
        }
        if (found) {
            for (int s = 0; s < 120 && !(g.testLab().flasks[0].told & (FT_KEPT << 1)); s++) g.testLabSkip(30);
            const FlaskSpecies feeder = g.testLab().flasks[0].who[0], fed = g.testLab().flasks[0].who[1];
            int k = -1;
            for (int i = 0; i < (int)g.testLab().kept.size(); i++) if (g.testLab().kept[i].who.sameSpecies(fed)) k = i;
            kept = k >= 0;
            if (kept) {
                const KeptCulture& c = g.testLab().kept[k];
                paired = c.paired && c.partner.sameSpecies(feeder);
                staged = g.testScopeStageKept(k) && g.testScope().species.size() == 2 && g.testScope().mount.kinds.size() >= 2;
                g.testFlaskEmpty(5);
                into = g.testFlaskPutKept(5, k) && g.testLab().flasks[5].n == 2;
                g.save("shots/tests/unit_lab_pair.txt");
                TestGame c2; c2.savePrefix = "shots/tests/unit_lab_save"; c2.settingsPath = "shots/tests/test_settings.txt"; c2.guidePath = "shots/tests/unit_lab_guide.txt";
                c2.keysPath = "shots/tests/test_keys.txt";
                c2.newGame(); c2.setState(GameState::SPACE);
                c2.load("shots/tests/unit_lab_pair.txt");
                for (const KeptCulture& x : c2.testLab().kept) if (x.who.sameSpecies(fed)) saved = x.paired && x.partner.sameSpecies(feeder) && x.partner.strain == feeder.strain;
                remove("shots/tests/unit_lab_pair.txt");
                for (int s = 0; s < 120 && !(g.testLab().flasks[0].told & FT_KEPT); s++) g.testLabSkip(30);   // the feeder's turn too
                int pairs = 0;
                for (const KeptCulture& x : g.testLab().kept) pairs += x.paired;
                once = pairs == 1;
                KeptCulture alone = g.testLab().kept[k];
                alone.paired = false;
                g.testLab().kept.push_back(alone);
                g.testScopeStageKept((int)g.testLab().kept.size() - 1);
                for (const SlideKind& K : g.testScope().mount.kinds) if (K.species == 0) lives = K.lives;
            }
        }
        check("experiments: a culture grown on its partner kept with it: staged and put in together, saved, the pair kept once; alone on the stage its cells live (B-441)",
              found && kept && paired && staged && into && saved && once && lives,
              found ? fmt("%s feed each other (%d pairs tried): the fed one kept %s, with its feeder %s; staged both %s; into flask 6 both %s; through the save %s; "
                          "the pair kept once %s; alone on the stage alive %s",
                          world.c_str(), tries, kept ? "yes" : "NO", paired ? "yes" : "NO", staged ? "yes" : "NO", into ? "yes" : "NO", saved ? "yes" : "NO",
                          once ? "yes" : "NO", lives ? "yes" : "NO")
                    : fmt("NO PAIR THAT FEEDS EACH OTHER in %d tried", tries));
    }
    remove("shots/tests/unit_lab_guide.txt");   // (the lab group's own guide, removed again after it)
}
