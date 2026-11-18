// Z-04: the laboratory (`lab microscope`): the microscope opened in a game on samples of each stage and chemistry found in the scan region (a
// world's living water, acid, ammonia and hydrocarbon life, a dormant world's spores, old rock's fossils, a prebiotic world's protocells, and
// living samples whose cells turn to light and to the field), each put on the stage as the cold store's vial; frames of every way of seeing at
// every zoom, a cell tracked, the light's crowd, the magnet's turn, the warm stage, a drop of food, a fossil's section; what each sample is
// made into (life/slides.h) and the sheets of what was seen (game/microscope.cpp)
#include "harness/harness.h"
#include "life/slides.h"
#include "life/morphs.h"

namespace {
enum LabWhat { LAB_WATER, LAB_ACID, LAB_AMMONIA, LAB_HYDROCARBON, LAB_DORMANT, LAB_FOSSIL, LAB_PREBIOTIC, LAB_LIGHT, LAB_MAGNET, LAB_FOOD, LAB_WARM, LAB_COUNT };
const char* const LAB_NAMES[LAB_COUNT] = {"water", "acid", "ammonia", "hydrocarbon", "dormant", "fossil", "prebiotic", "light", "magnet", "food", "warm"};
struct LabSample { StarSystem sys; int body = -1, species = -1; Microhabitat hab; Sample sample; };

// a sample of a world that holds a species: its niche (old rock for a fossil), a warmth it grows at, its salt; square metres tried until the
// sample holds it (growing, if it lives) and at least three more, else the fullest of 400
bool sampleWith(const StarSystem& sys, int body, const Biota& B, int sp, LabSample& out) {
    const Microbe& s = B.species[sp];
    Microhabitat h;
    h.niche = s.state == SS_FOSSIL ? NI_ROCK : s.niche;
    h.meanC = s.state == SS_LIVING ? clampd(B.meanC, s.tMin + 1, s.tMax - 1) : B.meanC;
    h.salt = s.saltLo; h.oldRock = s.state == SS_FOSSIL;
    int best = -1;
    for (int k = 0; k < 400; k++) {
        h.place = mix64(sys.bodies[body].seed ^ (uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL);
        const Sample S = sampleOf(B, h);
        bool has = false;
        for (const Held& H : S.held) if (H.species == sp && (s.state != SS_LIVING || H.active)) has = true;
        if (!has || (int)S.held.size() <= best) continue;
        best = (int)S.held.size(); out.hab = h; out.sample = S;
        if (best >= 4) break;
    }
    if (best < 0) return false;
    out.sys = sys; out.body = body; out.species = sp;
    return true;
}

// the scan region's first world of each kind of sample (its species of interest the most abundant of it): a living one's from a drop (water,
// silt, soil, a mat, a spring), the light's and the food's of a cell that swims to them at 8 um/s or more
void findSamples(LabSample (&out)[LAB_COUNT], bool (&found)[LAB_COUNT]) {
    for (bool& f : found) f = false;
    auto all = [&] { for (bool f : found) if (!f) return false; return true; };
    for (auto& [x, z, s, scanned] : scanSystems(150, 340, 20, 160, [&] { return !all(); })) {
        const StarSystem& sys = scanned;   // (a lambda takes no structured binding in C++17)
        auto L = lifeOf(sys);
        for (int i = 0; i < (int)sys.bodies.size(); i++) {
            const Biota& B = L->bodies[i];
            if (B.species.empty()) continue;
            const std::vector<Morph> forms = livingStage(B.stage) && (!found[LAB_LIGHT] || !found[LAB_FOOD]) ? morphsOf(sys, i) : std::vector<Morph>();
            auto drop = [](const Microbe& m) { return m.niche == NI_LIQUID || m.niche == NI_SILT || m.niche == NI_SOIL || m.niche == NI_MAT || m.niche == NI_SPRING; };
            auto swims = [&](const Microbe& m, uint32_t taxis) {
                return m.state == SS_LIVING && m.motion != MO_NONE && (m.taxis & taxis) && drop(m) && m.index < (int)forms.size() && forms[m.index].speed >= 8;
            };
            auto take = [&](int what, auto pick) {
                if (found[what]) return;
                int best = -1;
                for (const Microbe& m : B.species) if (pick(m) && (best < 0 || m.abundance > B.species[best].abundance)) best = m.index;
                if (best >= 0 && sampleWith(sys, i, B, best, out[what])) found[what] = true;
            };
            const bool living = livingStage(B.stage);
            const int ch = B.chemistry;
            if (living) take(ch == CH_WATER ? LAB_WATER : (ch == CH_ACID ? LAB_ACID : (ch == CH_AMMONIA ? LAB_AMMONIA : LAB_HYDROCARBON)),
                             [&](const Microbe& m) { return m.state == SS_LIVING && drop(m); });
            if (B.stage == LS_DORMANT) take(LAB_DORMANT, [](const Microbe& m) { return m.state == SS_DORMANT; });
            take(LAB_FOSSIL, [](const Microbe& m) { return m.state == SS_FOSSIL; });
            if (B.stage == LS_PREBIOTIC) take(LAB_PREBIOTIC, [](const Microbe& m) { return m.state == SS_PROTO; });
            if (living) take(LAB_LIGHT, [&](const Microbe& m) { return swims(m, TX_LIGHT) && m.abundance >= 0.25; });
            if (living) take(LAB_MAGNET, [](const Microbe& m) { return m.state == SS_LIVING && m.motion != MO_NONE && (m.taxis & TX_FIELD); });
            if (living) take(LAB_FOOD, [&](const Microbe& m) { return swims(m, TX_FOOD) && !(m.taxis & TX_FIELD) && m.abundance >= 0.25; });
            if (living) take(LAB_WARM, [&](const Microbe& m) {   // a spore former of a drop, crowded
                return m.state == SS_LIVING && drop(m) && (m.plan == MP_ROD || m.plan == MP_SPHERE || m.plan == MP_CURVED) && (m.cycle & LC_SPORULATING) && m.abundance >= 0.25;
            });
        }
    }
}

struct Lab {
    TestGame game;
    Input in;
    struct Panel { std::vector<uint32_t> px; std::string label; };
    Lab() {
        game.savePrefix = "shots/tests/lab_save"; game.settingsPath = "shots/tests/lab_settings.txt"; game.guidePath = "shots/tests/lab_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str()); remove(game.settingsPath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        applyTestScale(game);
    }
    // the slide at a time: a frame for what is seen (its first sights said), the status cleared, the frame drawn
    void at(double t) {
        game.testScope().t = t;
        game.frame(in, 0); in.newFrame();
        game.testClearStatus();
        game.frame(in, 0); in.newFrame();
    }
    Panel panel(const std::string& label) { Panel p; p.px.assign(game.output(), game.output() + (size_t)FBW * FBH); p.label = label; return p; }
    void press(int key) { game.testClearStatus(); pressKey(game, in, key); }
    // panels in a grid, `cols` across, each labelled
    void sheet(const std::vector<Panel>& ps, int cols, const std::string& title, const std::string& fn) {
        if (ps.empty()) return;
        const int gap = 4, head = 14, rows = ((int)ps.size() + cols - 1) / cols;
        Sheet sh(cols * (FBW + gap) + gap, rows * (FBH + gap) + gap + head);
        sh.label(4, 3, title);
        for (size_t i = 0; i < ps.size(); i++) {
            const int x = gap + (int)(i % cols) * (FBW + gap), y = head + gap + (int)(i / cols) * (FBH + gap);
            sh.blit(ps[i].px, FBW, FBH, x, y);
            sh.label(x + 2, y + FBH - 11, ps[i].label);
        }
        sh.save(fn);
    }
};

std::string bitsList(uint32_t taxis) {
    std::string s;
    for (int b = 0; b < TX_BITS; b++) if (taxis & (1u << b)) s += std::string(" ") + TAXIS_NAMES[b];
    return s;
}

std::string mountLine(const SlideMount& M, const std::vector<Morph>& forms) {
    std::string s = fmt("%s, prep %+d, depth %.0f um, stage %.1f C; first look %s x%.0f at %.1f %.1f, focus %.1f um; %zu cells in %zu kinds:", M.section ? "a section" : "a drop", M.prep,
                        M.depthUm, M.stageC, SEEING_NAMES[M.seeing], M.mag, M.cx, M.cy, M.focus, M.cells.size(), M.kinds.size());
    for (const SlideKind& K : M.kinds)
        s += fmt("\n      M%d %s %s%s, %d in a tile of %.0f um, depths %.1f-%.1f%s%s", K.species, MICROBE_PLAN_NAMES[forms[K.species].plan], CYCLE_FORM_NAMES[K.form],
                 K.active ? " active" : "", K.n, K.tile, K.z0, K.z1, K.taxis ? (" turns to" + bitsList(K.taxis)).c_str() : "", K.stress >= 0 ? fmt(", stressed at %.0f C", K.tMax).c_str() : "");
    return s;
}
}   // namespace

// a vial of a world's ground, as the kit would take it: its most abundant living species of a drop (else any species), at a place that holds it
bool labVial(const StarSystem& sys, int body, Vial& out) {
    const Biota B = biotaOf(sys, body);
    int best = -1;
    auto drop = [](const Microbe& m) { return m.niche == NI_LIQUID || m.niche == NI_SILT || m.niche == NI_SOIL || m.niche == NI_MAT || m.niche == NI_SPRING; };
    for (const Microbe& m : B.species) if (m.state == SS_LIVING && drop(m) && (best < 0 || m.abundance > B.species[best].abundance)) best = m.index;
    for (const Microbe& m : B.species) if (best < 0 && m.state != SS_FOSSIL) best = m.index;
    LabSample S;
    if (best < 0 || !sampleWith(sys, body, B, best, S)) return false;
    out = Vial();
    out.sx = sys.star.sx; out.sy = sys.star.sy; out.sz = sys.star.sz; out.body = body; out.hab = S.hab;
    out.name = upper(NICHE_NAMES[S.hab.niche]); out.readingC = S.hab.meanC; out.activity = S.sample.activity;
    return true;
}

// the densest slide the lab knows (`lab time`'s, the bench's): the scan region's first world with an abundant swimmer that turns to light, a
// vial of its ground (large armoured cells in a crowd of small ones)
bool labDenseVial(Vial& out) {
    LabSample S[LAB_COUNT];
    bool found[LAB_COUNT];
    findSamples(S, found);
    if (!found[LAB_LIGHT]) return false;
    out = Vial();
    out.sx = S[LAB_LIGHT].sys.star.sx; out.sy = S[LAB_LIGHT].sys.star.sy; out.sz = S[LAB_LIGHT].sys.star.sz; out.body = S[LAB_LIGHT].body; out.hab = S[LAB_LIGHT].hab;
    out.name = "THE DENSE SAMPLE"; out.readingC = S[LAB_LIGHT].hab.meanC; out.activity = S[LAB_LIGHT].sample.activity;
    return true;
}

// `lab microscope`: the samples, their mounts and frames, the strips, the sheets; exit 1 when a kind of sample was not found or a strip showed
// nothing of what it should
int labMicroscope() {
    const double t0 = nowSec();
    LabSample S[LAB_COUNT];
    bool found[LAB_COUNT];
    findSamples(S, found);
    Lab lab;
    TestGame& g = lab.game;
    int missing = 0;
    for (int w = 0; w < LAB_COUNT; w++) {
        if (!found[w]) { printf("lab microscope: no %s sample in the scan region\n", LAB_NAMES[w]); missing++; continue; }
        Vial v;
        v.sx = S[w].sys.star.sx; v.sy = S[w].sys.star.sy; v.sz = S[w].sys.star.sz; v.body = S[w].body; v.hab = S[w].hab;
        v.name = fmt("%s (%s)", upper(NICHE_NAMES[S[w].hab.niche]).c_str(), upper(LAB_NAMES[w]).c_str()); v.readingC = S[w].hab.meanC; v.activity = S[w].sample.activity;
        g.testColdStore().push_back(v);
    }
    g.openScreen(GameState::MICROSCOPE);
    int vial = 0, bad = 0;
    int at[LAB_COUNT];
    for (int w = 0; w < LAB_COUNT; w++) at[w] = found[w] ? vial++ : -1;
    // each sample: its mount, then every way of seeing at every zoom at its first look (the slide at 5 s)
    for (int w = 0; w < LAB_COUNT; w++) {
        if (at[w] < 0) continue;
        const double ts = nowSec();
        if (!g.testScopeStage(at[w])) { printf("  %s: NOT STAGED\n", LAB_NAMES[w]); bad++; continue; }
        const MicroscopeState& M = g.testScope();
        const Biota& B = M.life->bodies[M.body];
        printf("%s: %s (%s), %s %s, %s, %.1f C, the sample %d held (%d living), activity %s\n  mount: %s\n", LAB_NAMES[w], S[w].sys.bodies[S[w].body].name.c_str(),
               KINDS[S[w].sys.bodies[S[w].body].type].name, LIFE_STAGE_NAMES[B.stage], CHEMISTRY_NAMES[B.chemistry], NICHE_NAMES[S[w].hab.niche], S[w].hab.meanC,
               (int)M.sample.held.size(), M.sample.living(), VIAL_ACTIVITY_NAMES[M.sample.activity], mountLine(M.mount, M.forms).c_str());
        std::vector<Lab::Panel> ps;
        const double cx = M.mount.cx, cy = M.mount.cy, focus = M.mount.focus;
        int cols = 0;
        for (int way = 0; way < WS_COUNT; way++) {
            if (way == WS_SECTION && !M.mount.section) continue;
            const int steps = way == WS_ELECTRON ? SCOPE_ELECTRON_STEPS : SCOPE_LIGHT_STEPS;
            cols = std::max(cols, steps);
            for (int st = 0; st < SCOPE_LIGHT_STEPS; st++) {
                if (st >= steps) { ps.push_back(Lab::Panel{std::vector<uint32_t>((size_t)FBW * FBH, rgb(12, 12, 14)), ""}); continue; }
                g.testScopeView(way, st, cx, cy, focus);
                lab.at(5);
                ps.push_back(lab.panel(fmt("%s X%.0f: %zu", SEEING_NAMES[way], g.testScopeMag(), g.testScope().shown.size())));
            }
        }
        lab.sheet(ps, SCOPE_LIGHT_STEPS, fmt("lab microscope: the %s sample, every way of seeing at every zoom (the slide at 5 s)", LAB_NAMES[w]),
                  fmt("shots/tests/lab_microscope_sample_%s.png", LAB_NAMES[w]));
        printf("  (%.1f s)\n", nowSec() - ts);
    }
    // the strips: a sample put on the stage again, the view on the first cell of its species of interest at the step whose view holds about
    // a dozen of its bodies' height (`up` steps closer), its depth in focus; then what is done to it
    auto stage = [&](int w, int up) -> bool {
        if (at[w] < 0 || !g.testScopeStage(at[w])) return false;
        const MicroscopeState& M = g.testScope();
        for (const SlideKind& K : M.mount.kinds) {
            if (K.species != S[w].species) continue;
            const SlideCell& c = M.mount.cells[K.first];
            const double mag = magnificationFor(M.forms[K.species], M.seeing, 0.08);
            int best = 0;
            for (int i = 0; i < SCOPE_LIGHT_STEPS; i++) if (std::fabs(std::log(SCOPE_LIGHT[i] / mag)) < std::fabs(std::log(SCOPE_LIGHT[best] / mag))) best = i;
            g.testScopeView(M.seeing, std::max(0, std::min(SCOPE_LIGHT_STEPS - 1, best + up)), c.x, c.y, c.z);
            return true;
        }
        return false;
    };
    auto strip = [&](const char* what, std::vector<Lab::Panel>& ps) { lab.sheet(ps, (int)ps.size(), fmt("lab microscope: %s", what), fmt("shots/tests/lab_microscope_%s.png", what)); };
    {   // a cell tracked: Enter on the one under the crosshair, the stage following it for ten seconds
        std::vector<Lab::Panel> ps;
        const int w = found[LAB_LIGHT] ? LAB_LIGHT : LAB_WATER;
        if (stage(w, 2)) {
            lab.at(0);
            {   // the crosshair on the cell of its species nearest it (a swimmer is off its place by its wander)
                const MicroscopeState& M = g.testScope();
                double best = 1e30, bx = M.cx, by = M.cy, bz = M.focus;
                for (size_t i = 0; i < M.shown.size(); i++) {
                    if (M.mount.kinds[M.mount.cells[M.refs[i].cell].kind].species != S[w].species) continue;
                    const double d = std::hypot(M.refs[i].x - M.cx, M.refs[i].y - M.cy);
                    if (d < best) { best = d; bx = M.refs[i].x; by = M.refs[i].y; bz = mountDepth(M.mount, M.refs[i], M.t); }
                }
                g.testScopeView(M.seeing, M.step, bx, by, bz);
                lab.at(0);
            }
            lab.press(KEY_ENTER);
            const bool on = g.testScope().tracking;
            double moved = 0, x0 = g.testScope().cx, y0 = g.testScope().cy;
            for (double t : {0.0, 2.0, 4.0, 6.0, 8.0, 10.0}) { lab.at(t); moved = std::max(moved, std::hypot(g.testScope().cx - x0, g.testScope().cy - y0)); ps.push_back(lab.panel(fmt("T %.0f S", t))); }
            printf("track (%s): %s; the stage followed it %.1f um\n", LAB_NAMES[w], on ? "tracking" : "NOT TRACKING", moved);
            if (!on) bad++;
            strip("track", ps);
        }
    }
    // the light's crowd: L at the crosshair, the phototactic cells drawn into the spot
    auto crowdIn = [&](double cx, double cy, double r, int kindTaxis) {   // the copies of the turning kinds within the spot now
        int n = 0;
        const MicroscopeState& M = g.testScope();
        for (size_t i = 0; i < M.shown.size(); i++) {
            const SlideKind& K = M.mount.kinds[M.mount.cells[M.refs[i].cell].kind];
            if (!(K.taxis & kindTaxis)) continue;
            if (std::hypot(M.refs[i].x - cx, M.refs[i].y - cy) < r * 1.5) n++;
        }
        return n;
    };
    for (int pass = 0; pass < 2; pass++) {   // the light, then the food
        std::vector<Lab::Panel> ps;
        const int w = pass == 0 ? LAB_LIGHT : LAB_FOOD;
        if (!stage(w, 0)) continue;
        lab.at(0);
        lab.press(pass == 0 ? KEY_L : KEY_F);
        const SlideState st = slideStateAt(g.testScope().mount, g.testScope().events, 1);
        const double ex = pass == 0 ? st.spotX : st.foodX, ey = pass == 0 ? st.spotY : st.foodY, er = pass == 0 ? st.spotR : st.foodR;
        int first = 0, last = 0;
        for (double t : {0.0, 5.0, 10.0, 20.0, 40.0}) {
            lab.at(t);
            const int n = crowdIn(ex, ey, er, pass == 0 ? TX_LIGHT : TX_FOOD);
            if (t == 0) first = n;
            last = n;
            ps.push_back(lab.panel(fmt("T %.0f S: %d IN IT", t, n)));
        }
        printf("%s (%s): %d of the turning kinds' cells within it at 0 s, %d at 40 s\n", pass == 0 ? "light" : "food", LAB_NAMES[w], first, last);
        if (last <= first) bad++;
        strip(pass == 0 ? "light" : "food", ps);
    }
    {   // the magnet's turn: M brings it, the magnetotactic cells stream along its field; M at 8 s turns it, they turn about
        std::vector<Lab::Panel> ps;
        if (stage(LAB_MAGNET, 1)) {
            lab.at(0);
            lab.press(KEY_M);
            auto heading = [&]() {   // the field-turning cells' mean drift along the x axis
                double sx = 0; int n = 0;
                const MicroscopeState& M = g.testScope();
                for (size_t i = 0; i < M.shown.size(); i++) {
                    const SlideKind& K = M.mount.kinds[M.mount.cells[M.refs[i].cell].kind];
                    if (!(K.taxis & TX_FIELD)) continue;
                    sx += M.shown[i].vx; n++;
                }
                return n ? sx / n : 0.0;
            };
            double before = 0, after = 0;
            for (double t : {0.0, 3.0, 6.0}) { lab.at(t); before = heading(); ps.push_back(lab.panel(fmt("T %.0f S: %+.0f UM/S", t, before))); }
            g.testScope().t = 8; lab.press(KEY_M);
            for (double t : {8.5, 9.5, 11.0, 15.0}) { lab.at(t); after = heading(); ps.push_back(lab.panel(fmt("T %.1f S: %+.0f UM/S", t, after))); }
            printf("magnet (%s): the field's cells drifting %+.1f um/s along x before the turn, %+.1f after\n", LAB_NAMES[LAB_MAGNET], before, after);
            if (!(before > 0 && after < 0)) bad++;
            strip("magnet", ps);
        }
    }
    {   // the warm stage: H, the stage warming 30 C over the vial's, the stressed cells' spores, cysts and lysis
        std::vector<Lab::Panel> ps;
        if (stage(LAB_WARM, 1)) {
            lab.at(0);
            lab.press(KEY_H);
            for (double t : {0.0, 15.0, 30.0, 45.0, 60.0, 90.0}) {
                lab.at(t);
                const SlideState st = slideStateAt(g.testScope().mount, g.testScope().events, t);
                int stressed = 0;
                for (const Specimen& s : g.testScope().shown) stressed += s.act != AN_LIVE || s.form == CF_SPORE || s.form == CF_CYST || s.form == CF_GHOST;
                ps.push_back(lab.panel(fmt("T %.0f S: %.0f C, %d", t, st.stageC, stressed)));
            }
            strip("warm", ps);
        }
    }
    {   // a fossil's section: the old rock's thin section at each objective, then in a bright field
        std::vector<Lab::Panel> ps;
        if (at[LAB_FOSSIL] >= 0 && g.testScopeStage(at[LAB_FOSSIL])) {
            const MicroscopeState& M = g.testScope();
            for (int st = 0; st < 5; st++) { g.testScopeView(WS_SECTION, st, M.mount.cx, M.mount.cy, M.mount.focus); lab.at(1); ps.push_back(lab.panel(fmt("SECTION X%.0f", g.testScopeMag()))); }
            g.testScopeView(WS_BRIGHT, 3, M.mount.cx, M.mount.cy, M.mount.focus); lab.at(1); ps.push_back(lab.panel("BRIGHT X400"));
            strip("section", ps);
        }
    }
    // the sheets: what was seen of each sample's species
    int species = 0, lines = 0;
    for (int w = 0; w < LAB_COUNT; w++) {
        if (at[w] < 0 || !g.testScopeStage(at[w])) continue;
        const MicroscopeState& M = g.testScope();
        for (const SlideKind& K : M.mount.kinds) {
            const std::vector<std::string> L = g.testSheetLines(K.species);
            species++; lines += (int)L.size();
            std::string all;
            for (const std::string& l : L) all += (all.empty() ? "" : " | ") + l;
            printf("  sheet %s %s: %s\n", LAB_NAMES[w], g.testSpeciesKey(K.species).c_str(), all.c_str());
        }
    }
    printf("lab microscope: %d samples (%d not found), %d species' sheets of %d lines, %zu in the guide, %d strips not as they should be (%.1f s)\n", LAB_COUNT - missing, missing, species,
           lines, g.guide.microbes.size(), bad, nowSec() - t0);
    return missing || bad ? 1 : 0;
}

// `lab time`: the light sample's slide in each way at x100 and x400 and under the electron at x200, its frame's parts timed (the specimens made,
// the slide drawn, the mush and the colours, the whole frame), ten frames each
int labTime() {
    Vial v;
    if (!labDenseVial(v)) { printf("lab time: no dense sample\n"); return 1; }
    Lab lab;
    TestGame& g = lab.game;
    g.testColdStore().push_back(v);
    g.openScreen(GameState::MICROSCOPE);
    if (!g.testScopeStage(0)) return 1;
    const MicroscopeState& M = g.testScope();
    struct View { int way, step; } views[] = {{WS_BRIGHT, 1}, {WS_BRIGHT, 3}, {WS_PHASE, 1}, {WS_PHASE, 3}, {WS_DARK, 3}, {WS_GLOW, 3}, {WS_ELECTRON, 0}, {WS_ELECTRON, 2}};
    for (const View& w : views) {
        g.testScopeView(w.way, w.step, M.mount.cx, M.mount.cy, M.mount.focus);
        lab.at(5);
        double make = 0, draw = 0, rest = 0, whole = 0;
        for (int k = 0; k < 10; k++) {
            const double a = nowSec();
            g.testScopeFrame();
            const double b = nowSec();
            Slide sl; sl.seeing = w.way; sl.umPerPx = umPerPxAt(g.testScopeMag()); sl.cx = M.cx; sl.cy = M.cy; sl.t = M.t; sl.lod = true; sl.seed = M.mount.seed;
            sl.step = FB_SCALE >= 3 ? 2 : 1;
            sl.turn = M.turn; sl.tilt = M.tilt;
            drawSlide(g.fb, sl, M.shown);
            const double c = nowSec();
            g.fb.mush(2);
            const double d = nowSec();
            g.frame(lab.in, 0); lab.in.newFrame();
            const double e = nowSec();
            make += b - a; draw += c - b; rest += d - c; whole += e - d;
        }
        printf("%-8s x%-6.0f %4zu specimens: made %.1f ms, drawn %.1f ms, mushed %.1f ms; the whole frame %.1f ms\n", SEEING_NAMES[w.way], g.testScopeMag(), M.shown.size(), make * 100,
               draw * 100, rest * 100, whole * 100);
    }
    return 0;
}

int runLab(int argc, char** argv) {
    const std::string sub = argc > 2 ? argv[2] : "microscope";
    if (sub == "microscope") return labMicroscope();
    if (sub == "time") return labTime();
    if (sub == "cultures") return labCultures();
    if (sub == "jars") return labJars();
    if (sub == "tree") return labTree();
    if (sub == "kin") return treeKin(argc > 3 ? atoi(argv[3]) : 12);
    if (sub == "catalogue") return labCatalogue();   // Z-07
    printf("lab microscope | time | cultures | jars | tree | kin [r] | catalogue\n");
    return 2;
}
