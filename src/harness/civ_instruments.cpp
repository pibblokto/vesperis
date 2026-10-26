// E-03: the instruments (C-11, C-17): `instruments`, the makers and the player, their checks
#include "harness/harness.h"
#include "harness/test_worlds.h"

// C-11: the instruments of an age world's people: each age's (its timbre, family, register, courses) and where they lie over the world's places
// (the plan's places and the countryside round the capital): how many sites, how many of the people's instruments they hold, by age
static int printInstruments(const AgeWorld& W, bool quiet = false) {
    Star s; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, s, sys, true)) { printf("  no star\n"); return 1; } if (W.body >= (int)sys.bodies.size()) { printf("  no body\n"); return 1; }
    const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
    if (!worldHadCivilisation(g)) { printf("  no people\n"); return 1; }
    const Lore L = loreQuick(sys, b, g, W.which);
    const Tradition T = traditionOf(g, L);
    PeopleInstruments P; instrumentsOf(T, L.which, P);
    const PeopleAge pa = peopleAgeOf(g, L.which);
    printf("  %s, the %s (%s at the end): %s\n", b.name.c_str(), L.people.c_str(), AGE_NAMES[pa.age], traditionLine(T).c_str());
    for (int a = 0; a < AGE_COUNT; a++) {
        if (quiet && a != pa.age) continue;
        std::string line;
        for (const Instrument& I : P.byAge[a]) {
            const Timbre* tb = instrumentTimbre(T, I);
            const InstrumentMake* M = instrumentMake(T, I);
            line += fmt("%s%s [%d: %s of %s/%s, %s, reg %+d, %d courses]", line.empty() ? "" : "; ", instrumentName(T, I).c_str(), I.id, M ? INSTRUMENT_PLANS[M->plan].name : "?",
                        M ? INSTRUMENT_MATERIAL_NAMES[M->mat] : "?", M ? INSTRUMENT_MATERIAL_NAMES[M->mat2] : "?", tb ? TIMBRE_FAMILY_NAMES[tb->family] : "its cylinder", I.reg, instrumentCourses(T, I));
        }
        printf("    %-11s %s\n", AGE_NAMES[a], line.c_str());
    }
    std::vector<RecordPlace> places; recordPlacesOf(g, W.which, places);
    std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
    int sites = 0, musicSites = 0, withPlaces = 0, byAge[AGE_COUNT] = {}, near = 0, far = 0;
    std::set<int> ids; std::map<int, int> perId;
    for (const RecordPlace& rp : places) {
        for (const ShardSite& x : rp.sites) if (C->entries[shardLocalOf(x.index)].category == RC_MUSIC) musicSites++;
        if (!rp.instruments.empty()) withPlaces++;
        for (const InstrumentSite& is : rp.instruments) {
            sites++; byAge[is.age]++;
            const int id = instrumentIdOf(P, is.age, is.pick); if (id >= 0) { ids.insert(id); perId[id]++; }
            double best = 1e9; for (const ShardSite& x : rp.sites) if (x.index == is.record) best = std::min(best, std::hypot(x.x - is.x, x.z - is.z));
            if (best <= 1.6) near++; else far++;
        }
    }
    int all = 0; for (int a = 0; a < AGE_COUNT; a++) all += (int)P.byAge[a].size();
    printf("    over %zu places: %d music records' copies, %d instruments beside them in %d places (%d within 1.6 m of their record, %d farther), %zu of the people's %d instruments among them;",
           places.size(), musicSites, sites, withPlaces, near, far, ids.size(), all);
    for (int a = 0; a < AGE_COUNT; a++) if (byAge[a]) printf(" %s %d", AGE_NAMES[a], byAge[a]);
    printf("\n");
    if (!quiet) { printf("    copies:"); for (const auto& kv : perId) { const Instrument* I = instrumentById(P, kv.first); printf(" %s %d,", I ? instrumentName(T, *I).c_str() : "?", kv.second); } printf("\n"); }
    return sites > 0 ? 0 : 1;
}

// C-11: a phrase on an instrument, as the case plays it (the synth's instrument voices at the game's master): up its scale through a period and
// back, a held note at the end; a far future's slow tones three at a time. The gramophone: its disc through its horn
struct InstrumentRender { RenderStats r; double seconds = 0, hz0 = 0, pitch = 0; int notes = 0; };
static InstrumentRender renderInstrument(const Tradition& T, const Instrument& I, std::vector<int16_t>* pcm) {
    InstrumentRender out; AudioSynth synth; AudioState st;
    const Timbre* tb = instrumentTimbre(T, I);
    Piece disc;
    if (instrumentPlaysDisc(I) || !tb) {   // the gramophone
        instrumentDiscOf(T, I, disc);
        st.piece = &disc; st.tradition = &T; st.pieceStart = true; st.medium = RM_CYLINDER;
        out.seconds = std::min(disc.seconds + 1.5, 30.0);
        out.r = renderReceiver(synth, st, out.seconds, pcm);
        out.notes = (int)disc.notes.size();
        return out;
    }
    st.instrument = tb;
    const int n = std::max(1, (int)T.scale.size());
    const bool slow = tb->family == TF_FIELD || tb->family == TF_SWARM || tb->family == TF_PAD;
    struct Hit { double t; int degree; double hold; };
    std::vector<Hit> hits;
    if (slow) { hits = {{0.0, 0, 3.0}, {1.2, n / 2, 3.0}, {2.4, n, 4.0}}; out.seconds = 10; }
    else {
        double t = 0;
        for (int d = 0; d <= n; d++) { hits.push_back({t, d, 0.26}); t += 0.32; }
        for (int d = n - 1; d >= std::max(0, n - 3); d--) { hits.push_back({t, d, 0.26}); t += 0.32; }
        hits.push_back({t, 0, 1.3}); t += 1.3;
        out.seconds = t + std::max(1.5, std::min(4.0, tb->release + tb->decay));
    }
    out.hz0 = instrumentHz(T, I, 0);
    double now = 0; size_t next = 0;
    auto feed = [&](double dt) {
        now += dt;
        while (next < hits.size() && hits[next].t <= now) {
            AudioState::InstrumentNote nt; nt.hz = instrumentHz(T, I, hits[next].degree); nt.vel = 0.8; nt.hold = hits[next].hold;
            if (I.pads) { nt.timbre = instrumentTimbre(T, I, hits[next].degree); nt.hz = 0; }   // a drum machine's pads
            st.instrumentHits.push_back(nt); next++; out.notes++;
        }
    };
    feed(0);
    std::vector<int16_t> local; std::vector<int16_t>* buf = pcm ? pcm : &local;
    out.r = renderReceiver(synth, st, out.seconds, buf, feed);
    if (!slow && !I.pads && !timbrePercussive(tb->family) && tb->family != TF_ROARER && tb->family != TF_METAL && tb->family != TF_SWARM) out.pitch = pcmPitch(*buf, 22050, 0.06, 0.24, out.hz0 * 0.4, out.hz0 * 2.6);   // the first note's
    return out;
}
static int instrumentsWavs() {
    makeDir("shots"); makeDir("shots/tests");
    int failures = 0;
    printf("each age world's instruments of its own age, a phrase each (the case's levels, the master at the game's 0.8):\n");
    for (int a = 0; a < AGE_COUNT; a++) {
        const AgeWorld& W = AGE_WORLDS[a];
        Star s; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, s, sys, true)) { failures++; continue; } const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
        const Lore L = loreQuick(sys, b, g, W.which); const Tradition T = traditionOf(g, L);
        PeopleInstruments P; instrumentsOf(T, L.which, P);
        printf("  %s: the %s (%s)\n", AGE_NAMES[a], L.people.c_str(), traditionLine(T).c_str());
        if (P.byAge[a].empty()) { printf("    NO INSTRUMENT\n"); failures++; continue; }
        for (const Instrument& I : P.byAge[a]) {
            std::vector<int16_t> pcm; const InstrumentRender ir = renderInstrument(T, I, &pcm);
            const std::string fn = fmt("shots/tests/instrument_%s_%d_%s.wav", fileWord(AGE_NAMES[a]).c_str(), I.slot, fileWord(instrumentName(T, I)).c_str());
            writeWav16(fn, pcm, 22050);
            const bool pitched = ir.pitch > 0;
            const double ratio = pitched ? ir.pitch / ir.hz0 : 1;
            const bool pitchOk = !pitched || std::fabs(ratio - 1) < 0.04 || std::fabs(ratio - 2) < 0.08 || std::fabs(ratio - 0.5) < 0.02 || std::fabs(ratio - 3) < 0.12;
            const bool bad = ir.r.nan > 0 || ir.r.peak > 0.95 || ir.r.peak < 0.05 || ir.r.rms < 0.006 || ir.r.rms > 0.25 || !pitchOk;
            if (bad) failures++;
            printf("    %-22s %2d notes, %4.1f s: peak %.3f rms %.4f%s%s -> %s\n", instrumentName(T, I).c_str(), ir.notes, ir.seconds, ir.r.peak, ir.r.rms,
                   pitched ? fmt(", its first note %.1f Hz heard %.1f Hz", ir.hz0, ir.pitch).c_str() : "", bad ? "  FAIL" : "", fn.c_str());
        }
    }
    return failures;
}

// C-11: each age's instrument found in its ruins and played: landed by the scene world's capital (then its cities), the explorer before an
// instrument of the world's own age (else any), a frame, E, the case on the ship with it, a few notes played (frames `instruments_<age>_*.png`)
static int instrumentsScenes(int onlyAge = -1, std::vector<std::string>* lines = nullptr) {
    mkdir("shots/tests", 0755);
    int failures = 0;
    for (int age = 0; age < AGE_COUNT; age++) {
        if (onlyAge >= 0 && age != onlyAge) continue;
        const AgeWorld& aw = AGE_SCENE_WORLDS[age];
        const std::string an = fileWord(AGE_NAMES[age]);
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) { failures++; continue; }
        const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        const int which = std::min(aw.which, std::max(0, P->peoples - 1));
        if (P->plans[which].places.empty()) { failures++; continue; }
        remove("shots/tests/test_instruments_guide.txt");
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_instruments_guide.txt";
        game.newGame(); game.setState(GameState::SPACE); applyTestScale(game);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        auto shot = [&](const std::string& what) { const std::string fn = fmt("shots/tests/instruments_%s_%s.png", an.c_str(), what.c_str()); writePNG(fn.c_str(), game.output(), FBW, FBH); printf("    %s\n", fn.c_str()); };
        run(0.2); game.testParkAt(st, aw.body); run(0.3);
        int id = -1, tries = 0;
        for (size_t pi = 0; pi < P->plans[which].places.size() && id < 0 && tries < 5; pi++) {
            const PlanPlace& pl = P->plans[which].places[pi];
            if (pi > 0 && pl.rank > 0 && tries > 0 && pi > 12) break;
            tries++;
            if (game.state != GameState::SPACE) { game.setState(GameState::SPACE); game.testParkAt(st, aw.body); run(0.2); }
            game.testLandCursor(pl.lat / DEG, pl.lon / DEG); run(0.1); press(KEY_ENTER);
            for (int i = 0; i < 30 * 20 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
            if (game.state != GameState::SURFACE) continue;
            id = game.testWalkToInstrument(age);
            if (id < 0) id = game.testWalkToInstrument(-1);
        }
        if (id < 0) { printf("  %s: no instrument in reach of %d places\n", an.c_str(), tries); failures++; continue; }
        const double rot = std::max(600.0, std::fabs(sys.bodies[aw.body].rotPeriod));
        for (int k = 0; k < 64 && game.testSunHeight() < 0.5; k++) { game.testAdvanceTime(rot / 64); run(0.05); }   // the day: the sun well up
        if (game.testWalkToInstrument(age) < 0) game.testWalkToInstrument(-1);
        run(0.4);
        const int near = game.testNearInstrumentId();
        printf("  %s (%s): the instrument %d, the HUD's %s\n", an.c_str(), sys.bodies[aw.body].name.c_str(), id, near >= 0 ? "prompt up" : "PROMPT MISSING");
        shot("found");
        const size_t before = game.guide.instruments.size();
        for (int k = 0; k < 3 && near >= 0 && game.guide.instruments.size() == before; k++) { press(KEY_E); run(0.3); }   // a record lying nearer is taken first
        const bool taken = game.guide.instruments.size() == before + 1;
        shot("taken");
        // the case on the ship: the explorer before it, E, a few notes
        game.setState(GameState::SPACE); run(0.2);
        game.testCabinGoto(-1.9, -1.38, -PI / 2, -0.25); run(0.2);
        const int facing = game.testCabinFacing();
        press(KEY_E); run(0.2);
        press(KEY_ENTER); run(0.2);   // C-18: the case opens on its worlds (this one first, the ship over it): Enter opens it
        const bool open = game.state == GameState::INSTRUMENTS && game.testCaseLevel() == 1;
        const std::string info0 = game.testInstrumentInfo();
        const bool disc = info0.find("its disc") != std::string::npos;
        if (disc) { press(KEY_ENTER); run(1.0); }
        else { for (int k : {KEY_1, KEY_3, KEY_5, KEY_2}) { press(k); run(0.15); } press(KEY_RIGHT); press(KEY_6); run(0.05); }
        shot("case");
        const std::string info = game.testInstrumentInfo();
        const bool played = disc ? info.find("the disc turning") != std::string::npos : info.find(" 5 notes played") != std::string::npos;
        const std::string line = fmt("%s: %s; facing %d, the case %s; %s", an.c_str(), taken ? "taken" : "NOT TAKEN", facing, open ? "open" : "NOT OPEN", info.c_str());
        printf("    %s\n", line.c_str());
        if (lines) lines->push_back(line);
        if (near < 0 || !taken || !open || !played || facing != FACE_CASE) failures++;
        press(KEY_ESCAPE); run(0.1);
        remove("shots/tests/test_instruments_guide.txt");
    }
    return failures;
}

// C-17: a people near home and its instruments as made (its tradition, its lists, its palette)
struct MadePeople { std::string world; int age = 0, which = 0; BodyGen g; Lore L; Tradition T; PeopleInstruments P; FormPalette pal; };
static bool madePeopleOf(int64_t sx, int64_t sy, int64_t sz, int body, int which, MadePeople& out) {
    Star s; StarSystem sys; if (!systemAt(sx, sy, sz, s, sys, true)) return false;
    if (body < 0 || body >= (int)sys.bodies.size()) return false;
    out.g = BodyGen::make(sys.bodies[body]);
    if (!worldHadCivilisation(out.g) || which >= peoplesOf(out.g)) return false;
    out.L = loreQuick(sys, sys.bodies[body], out.g, which);
    out.T = traditionOf(out.g, out.L);
    instrumentsOf(out.T, out.L.which, out.P);
    const Culture cu = cultureOf(out.g, which);
    out.pal = cu.palette; out.age = cu.age; out.which = which;
    out.world = sys.bodies[body].name + (which ? " (second)" : "");
    return true;
}
static std::vector<MadePeople> madePeoplesNearHome(int n) {
    std::vector<MadePeople> out;
    for (const NearPeople& np : peoplesNearHome(n)) { MadePeople m; if (madePeopleOf(np.x, np.y, np.z, np.body, np.which, m)) out.push_back(m); }
    return out;
}
// C-17: a sheet of instruments: a cell each (its picture as the case draws it, its name, a line of what it is), `cols` to a row
struct SheetCell { std::vector<InstrumentPart> parts; FormPalette pal; std::string name, line; };
static void drawInstrumentSheet(const std::vector<SheetCell>& cells, int cols, const std::vector<std::string>& rowLabels, int perRow, const std::string& fn, int scale) {
    const int CW = 160, CH = 92, LH = 20, lab = rowLabels.empty() ? 0 : 96;
    const int rows = std::max(1, ((int)cells.size() + cols - 1) / cols);
    const int W = lab + cols * CW, H = rows * (CH + LH) + 4;
    std::vector<uint32_t> buf((size_t)W * scale * H * scale, rgb(6, 7, 9));
    RGBCanvas cv{buf.data(), W * scale, H * scale, scale};
    for (size_t i = 0; i < cells.size(); i++) {
        const int r = (int)i / cols, c = (int)i % cols, x = lab + c * CW + 2, y = r * (CH + LH) + 2;
        drawInstrumentParts(cv, x, y, CW - 4, CH, cells[i].parts, cells[i].pal, std::vector<double>(), 0, true);
        drawText(cv, x + 1, y + CH + 2, cells[i].name.substr(0, 26).c_str(), rgb(230, 225, 205));
        drawText(cv, x + 1, y + CH + 10, cells[i].line.substr(0, 26).c_str(), rgb(120, 130, 120));
        if (c == 0 && !rowLabels.empty() && r * cols / std::max(1, perRow) < (int)rowLabels.size()) {
            const std::string& L = rowLabels[std::min((int)rowLabels.size() - 1, r)];
            size_t at = 0; int ly = y + 4;
            while (at < L.size() && ly < y + CH) { drawText(cv, 3, ly, L.substr(at, 15).c_str(), rgb(200, 190, 150)); at += 15; ly += 9; }
        }
    }
    writePNG(fn.c_str(), buf.data(), W * scale, H * scale);
    printf("  %s (%d instruments)\n", fn.c_str(), (int)cells.size());
}
static std::string makeLine(const InstrumentMake& M) {
    std::string s = std::string(INSTRUMENT_MATERIAL_NAMES[M.mat]) + (M.mat2 != M.mat ? std::string("/") + INSTRUMENT_MATERIAL_NAMES[M.mat2] : "");
    for (char& ch : s) ch = (char)std::toupper((unsigned char)ch);
    return s;
}
// C-17: each age's instruments of `n` peoples near home, a people a row
static int instrumentsSheet(int age, int n, int scale) {
    makeDir("shots"); makeDir("shots/tests");
    std::vector<SheetCell> cells; std::vector<std::string> labels;
    const int cols = 6;
    for (const MadePeople& m : madePeoplesNearHome(n * 3)) {
        if ((int)labels.size() >= n) break;
        const std::vector<Instrument>& L = m.P.byAge[age];
        if (L.empty()) continue;
        for (int i = 0; i < cols; i++) {
            SheetCell c; c.pal = m.pal;
            if (i < (int)L.size()) { instrumentParts(m.T, L[i], c.parts); c.name = instrumentName(m.T, L[i]); if (const InstrumentMake* M = instrumentMake(m.T, L[i])) c.line = makeLine(*M); }
            cells.push_back(c);
        }
        std::string lab = m.world; for (char& ch : lab) ch = (char)std::toupper((unsigned char)ch);
        labels.push_back(lab);
    }
    drawInstrumentSheet(cells, cols, labels, cols, fmt("shots/tests/instruments_sheet_%s.png", fileWord(AGE_NAMES[age]).c_str()), scale);
    return labels.empty() ? 1 : 0;
}
// C-17: every plan, made by a few peoples each at the age its class belongs to
static int instrumentsPlans(int peoples, int scale) {
    makeDir("shots"); makeDir("shots/tests");
    static const int CLASS_AGE[IC_COUNT] = {AGE_PREHISTORIC, AGE_MEDIEVAL, AGE_INDUSTRIAL, AGE_INDUSTRIAL, AGE_MEDIEVAL, AGE_PREHISTORIC, AGE_MEDIEVAL, AGE_MEDIEVAL,
        AGE_PREHISTORIC, AGE_FAR_FUTURE, AGE_INDUSTRIAL, AGE_MODERN, AGE_MODERN, AGE_FUTURISTIC, AGE_FUTURISTIC, AGE_FUTURISTIC, AGE_FAR_FUTURE,
        AGE_PREHISTORIC, AGE_PREHISTORIC, AGE_PREHISTORIC, AGE_MEDIEVAL, AGE_INDUSTRIAL, AGE_MODERN, AGE_INDUSTRIAL};
    const std::vector<MadePeople> M = madePeoplesNearHome(std::max(1, peoples));
    if (M.empty()) return 1;
    std::vector<SheetCell> cells;
    for (int p = 0; p < IP_COUNT; p++)
        for (int k = 0; k < std::max(1, peoples); k++) {
            const MadePeople& m = M[(p * 7 + k * 13) % M.size()];
            const InstrumentMake mk = instrumentMakeOfPlan(m.g, m.L, p, CLASS_AGE[INSTRUMENT_PLANS[p].cls], (uint64_t)k);
            SheetCell c; c.pal = m.pal; instrumentPartsOf(m.T, mk, c.parts); c.name = mk.name; c.line = std::string(INSTRUMENT_PLANS[p].name).substr(0, 26);
            cells.push_back(c);
        }
    const int cols = peoples <= 1 ? 8 : peoples * (8 / std::max(1, peoples));
    for (int part = 0; part * cols * 8 < (int)cells.size(); part++) {
        std::vector<SheetCell> some(cells.begin() + part * cols * 8, cells.begin() + std::min((int)cells.size(), (part + 1) * cols * 8));
        drawInstrumentSheet(some, cols, {}, cols, fmt("shots/tests/instruments_plans_%d.png", part + 1), scale);
    }
    return 0;
}
// C-17: how far apart the peoples' instruments are: for each age and class the first make of each people's; pairs sharing the plan, the plan
// and the body's material, the whole look; the plans and names in use; the prehistoric bone pipes, the industrial machines
// C-17: an instrument drawn small as the case draws it (no ruler), and how far two such pictures are apart (the mean of the channels' differences, 0..255)
static std::vector<uint32_t> instrumentThumb(const std::vector<InstrumentPart>& parts, const FormPalette& pal) {
    const int W = 144, H = 80;
    std::vector<uint32_t> buf((size_t)W * H, 0);
    RGBCanvas cv{buf.data(), W, H, 1};
    drawInstrumentParts(cv, 0, 0, W, H, parts, pal, std::vector<double>(), 0, false);
    return buf;
}
static double thumbDistance(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    double d = 0;
    for (size_t i = 0; i < a.size() && i < b.size(); i++) for (int s = 0; s < 24; s += 8) d += std::abs((int)((a[i] >> s) & 255) - (int)((b[i] >> s) & 255));
    return d / std::max<size_t>(1, a.size() * 3);
}
constexpr double INSTRUMENT_ALIKE = 6.0;   // two pictures closer than this look alike (the closest pairs' sheet: `instruments distinct`)
struct AlikePair { double d; SheetCell a, b; };
struct DistinctResult { int peoples = 0, pairs = 0, samePlan = 0, samePlanMat = 0, classes = 0, poorClasses = 0, bonePipes = 0, pipes = 0, machines = 0, industrial = 0, alike = 0; double minD = 1e9, q05 = 0, median = 0; std::string poor; std::vector<AlikePair> closest; };
static DistinctResult instrumentsDistinctOf(const std::vector<MadePeople>& M, bool print, bool pictures = true) {
    DistinctResult R; R.peoples = (int)M.size();
    std::vector<double> allD;
    for (int a = 0; a < AGE_COUNT; a++) {
        if (print) printf("  %s:\n", AGE_NAMES[a]);
        for (int c = 0; c < IC_COUNT; c++) {
            std::vector<const InstrumentMake*> v; std::vector<const MadePeople*> who;
            for (const MadePeople& m : M) for (const Instrument& I : m.P.byAge[a]) { const InstrumentMake* mk = instrumentMake(m.T, I); if (mk && mk->cls == c) { v.push_back(mk); who.push_back(&m); break; } }
            if (v.size() < 2) continue;
            std::vector<std::vector<uint32_t>> th(v.size()); std::vector<std::vector<InstrumentPart>> parts(v.size());
            if (pictures) for (size_t i = 0; i < v.size(); i++) { instrumentPartsOf(who[i]->T, *v[i], parts[i]); th[i] = instrumentThumb(parts[i], who[i]->pal); }
            int pairs = 0, sp = 0, spm = 0, alike = 0; std::map<int, int> plans; std::set<std::string> names;
            for (size_t i = 0; i < v.size(); i++) {
                plans[v[i]->plan]++; names.insert(v[i]->name);
                for (size_t j = i + 1; j < v.size(); j++) {
                    pairs++; if (v[i]->plan == v[j]->plan) { sp++; if (v[i]->mat == v[j]->mat) spm++; }
                    if (!pictures) continue;
                    const double d = thumbDistance(th[i], th[j]);
                    allD.push_back(d); if (d < INSTRUMENT_ALIKE) alike++;
                    if (R.closest.size() < 16 || d < R.closest.back().d) {
                        AlikePair ap; ap.d = d;
                        ap.a.parts = parts[i]; ap.a.pal = who[i]->pal; ap.a.name = v[i]->name; ap.a.line = who[i]->world;
                        ap.b.parts = parts[j]; ap.b.pal = who[j]->pal; ap.b.name = v[j]->name; ap.b.line = fmt("%s %.1f", who[j]->world.c_str(), d);
                        R.closest.push_back(ap);
                        std::sort(R.closest.begin(), R.closest.end(), [](const AlikePair& x, const AlikePair& y) { return x.d < y.d; });
                        if (R.closest.size() > 16) R.closest.pop_back();
                    }
                }
            }
            R.alike += alike;
            int most = 0; for (auto& kv : plans) most = std::max(most, kv.second);
            int avail = 0; for (int p = 0; p < IP_COUNT; p++) if (INSTRUMENT_PLANS[p].cls == c && a >= INSTRUMENT_PLANS[p].ageMin && a <= INSTRUMENT_PLANS[p].ageMax) avail++;
            R.pairs += pairs; R.samePlan += sp; R.samePlanMat += spm; R.classes++;
            if (v.size() >= 5 && avail >= 3 && (int)plans.size() < 3) { R.poorClasses++; R.poor += fmt(" %s %s (%zu plans)", AGE_NAMES[a], INSTRUMENT_CLASS_NAMES[c], plans.size()); }
            if (print) printf("    %-26s %3zu peoples: %2zu of %d plans (the commonest %2.0f%%), %3zu names; pairs %4d: the same plan %4.1f%%, and body %4.1f%%; pictures alike %4.1f%%\n",
                              INSTRUMENT_CLASS_NAMES[c], v.size(), plans.size(), avail, 100.0 * most / v.size(), names.size(), pairs, 100.0 * sp / std::max(1, pairs), 100.0 * spm / std::max(1, pairs), 100.0 * alike / std::max(1, pairs));
            if (a == AGE_PREHISTORIC && c == IC_EDGE) for (const InstrumentMake* mk : v) { R.pipes++; if (mk->mat == IM_BONE) R.bonePipes++; }
        }
    }
    for (const MadePeople& m : M) { R.industrial++; if (m.T.machine >= 0) R.machines++; }
    if (!allD.empty()) { std::sort(allD.begin(), allD.end()); R.minD = allD.front(); R.q05 = allD[allD.size() / 20]; R.median = allD[allD.size() / 2]; }
    return R;
}
static int instrumentsDistinct(int n) {
    const double t0 = nowSec();
    const std::vector<MadePeople> M = madePeoplesNearHome(n);
    printf("instruments distinct: %zu peoples near home (%.1f s)\n", M.size(), nowSec() - t0);
    const DistinctResult R = instrumentsDistinctOf(M, true);
    std::map<std::string, int> names; for (const MadePeople& m : M) for (const InstrumentMake& mk : m.T.makes) names[mk.name]++;
    std::vector<std::pair<int, std::string>> top; for (auto& kv : names) top.push_back({kv.second, kv.first});
    std::sort(top.rbegin(), top.rend());
    printf("  over every age and class: %d pairs, the same plan %.1f%%, the same plan and body %.1f%%; the pictures' distance: the closest %.1f, 5%% under %.1f, the median %.1f; alike (under %.0f) %d (%.2f%%); %d classes, %d with too few plans%s\n",
           R.pairs, 100.0 * R.samePlan / std::max(1, R.pairs), 100.0 * R.samePlanMat / std::max(1, R.pairs), R.minD, R.q05, R.median, INSTRUMENT_ALIKE, R.alike, 100.0 * R.alike / std::max(1, R.pairs), R.classes, R.poorClasses, R.poor.c_str());
    {   // the closest pairs side by side: are the most alike alike?
        std::vector<SheetCell> cells;
        for (const AlikePair& ap : R.closest) { cells.push_back(ap.a); cells.push_back(ap.b); }
        if (!cells.empty()) drawInstrumentSheet(cells, 4, {}, 4, "shots/tests/instruments_closest.png", 1);
    }
    {   // how heavy they are to draw: parts and forms an instrument
        std::vector<int> counts; int heaviest = 0; std::string heavy;
        for (const MadePeople& m : M) for (int a = 0; a < AGE_COUNT; a++) for (const Instrument& I : m.P.byAge[a]) {
            std::vector<Form> f; instrumentForms(I, m.T, 0, 0, 0, f); counts.push_back((int)f.size());
            if ((int)f.size() > heaviest) { heaviest = (int)f.size(); heavy = instrumentName(m.T, I); }
        }
        std::sort(counts.begin(), counts.end());
        double mean = 0; for (int c : counts) mean += c; mean /= std::max<size_t>(1, counts.size());
        if (!counts.empty()) printf("  forms an instrument: the mean %.0f, the median %d, 95%% under %d, the most %d (%s)\n", mean, counts[counts.size() / 2], counts[counts.size() * 95 / 100], heaviest, heavy.c_str());
    }
    printf("  prehistoric pipes of bone: %d of %d; peoples with a machine for their cylinders: %d of %d; %zu names, the commonest:", R.bonePipes, R.pipes, R.machines, R.industrial, names.size());
    for (size_t i = 0; i < top.size() && i < 12; i++) printf(" %s %d,", top[i].second.c_str(), top[i].first);
    printf("\n");
    if (getenv("VESPERIS_MAKERS")) for (const auto& kv : names) printf("    %-18s %d\n", kv.first.c_str(), kv.second);   // every name, for reading
    return 0;
}
// C-17: the unit's checks of the makers: every instrument made, within its bounds and of what its world gave, the same twice; the peoples'
// instruments apart by the measure; the decoder's names the ruins' names
void makerChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    const std::vector<MadePeople> M = madePeoplesNearHome(48);
    int makes = 0, bad = 0, longNames = 0, ungiven = 0, outBounds = 0, heavyParts = 0, fewCourses = 0, notSame = 0, unnamed = 0; std::string first; int notes = 0;
    const bool verbose = getenv("VESPERIS_MAKERS") != nullptr;
    auto note = [&](const std::string& s) { if (first.empty()) first = s; if (verbose && notes++ < 80) printf("    %s\n", s.c_str()); };
    for (const MadePeople& m : M) {
        const Tradition T2 = traditionOf(m.g, m.L);
        if (T2.makes.size() != m.T.makes.size()) notSame++;
        for (int a = 0; a < AGE_COUNT; a++) for (const Instrument& I : m.P.byAge[a]) {
            const InstrumentMake* mk = instrumentMake(m.T, I);
            if (!mk) { bad++; note(fmt("%s: no make", m.world.c_str())); continue; }
            makes++;
            if (mk->name.size() > 17 || mk->name.empty()) { longNames++; note(fmt("%s: the name '%s'", m.world.c_str(), mk->name.c_str())); }
            std::vector<InstrumentPart> parts, again; instrumentParts(m.T, I, parts); instrumentParts(T2, I, again);
            bool same = parts.size() == again.size();
            for (size_t k = 0; same && k < parts.size(); k++) same = parts[k].kind == again[k].kind && parts[k].u == again[k].u && parts[k].v == again[k].v && parts[k].mat == again[k].mat;
            if (!same) { notSame++; note(fmt("%s: %s made twice differently", m.world.c_str(), mk->name.c_str())); }
            double lu, lv, top; instrumentExtent(parts, lu, lv, top);
            bool finite = true; for (const InstrumentPart& p : parts) if (!std::isfinite(p.u) || !std::isfinite(p.v) || !std::isfinite(p.w1) || !std::isfinite(p.len) || !std::isfinite(p.r0)) finite = false;
            if (!finite || std::max(lu, lv) > 1.6 || std::max(lu, lv) < 0.05 || top > 0.6) { outBounds++; note(fmt("%s: %s %.2f x %.2f m, %.2f m high%s", m.world.c_str(), mk->name.c_str(), lu, lv, top, finite ? "" : ", NOT FINITE")); }
            if (parts.size() > 400 || parts.size() < 2) { heavyParts++; note(fmt("%s: %s of %zu parts", m.world.c_str(), mk->name.c_str(), parts.size())); }
            // a course a degree where the plan lays one out (holes, keys, bars, tongues, strings): not a drum, a rattle, a horn's few valves, a fiddle's
            // two strings stopped, one gong, a field
            const bool melodic = !(mk->cls == IC_SKIN || mk->cls == IC_RATTLE || mk->cls == IC_JINGLE || mk->cls == IC_CYMBAL || mk->cls == IC_WHIRL || mk->cls == IC_HUM || mk->cls == IC_MACHINE
                                   || mk->cls == IC_FIELD || mk->cls == IC_SWARM || mk->cls == IC_MOUTH_PLUCK || mk->cls == IC_LIP || mk->cls == IC_BOW || (mk->plan == IP_GONGS && mk->variant % 2));
            if (melodic && instrumentCoursesOf(parts) < 3) { fewCourses++; note(fmt("%s: %s (%s) with %d courses", m.world.c_str(), mk->name.c_str(), INSTRUMENT_PLANS[mk->plan].name, instrumentCoursesOf(parts))); }
            for (const InstrumentPart& p : parts) if (!instrumentMaterialGiven(m.g, m.L, p.mat, I.age)) { ungiven++; note(fmt("%s: %s of %s in the %s age (%s)", m.world.c_str(), mk->name.c_str(), INSTRUMENT_MATERIAL_NAMES[p.mat], AGE_NAMES[I.age], INSTRUMENT_PLANS[mk->plan].name)); break; }
            const Timbre* tb = instrumentTimbre(m.T, I);
            if (tb && !I.pads && tb->name != mk->name && tb->name != mk->plural) { unnamed++; note(fmt("%s: the voice '%s', the instrument '%s'", m.world.c_str(), tb->name.c_str(), mk->name.c_str())); }
        }
    }
    check("instruments: made by their peoples, of what their worlds gave (C-17)", !M.empty() && makes > 0 && bad == 0 && longNames == 0 && ungiven == 0 && outBounds == 0 && heavyParts == 0 && fewCourses == 0 && notSame == 0,
          fmt("%zu peoples near home, %d instruments: %d unmade, %d names past 17 letters, %d of what their world or age did not give, %d out of bounds, %d too many or few parts, %d melodic with under 3 courses, %d not the same made twice%s%s",
              M.size(), makes, bad, longNames, ungiven, outBounds, heavyParts, fewCourses, notSame, first.empty() ? "" : "; first: ", first.c_str()));
    const DistinctResult R = instrumentsDistinctOf(M, false);
    const double sp = 100.0 * R.samePlanMat / std::max(1, R.pairs), al = 100.0 * R.alike / std::max(1, R.pairs);
    check("instruments: as many as their peoples (C-17)", R.pairs > 0 && sp < 15 && al < 1.5 && R.poorClasses == 0 && R.bonePipes * 2 < R.pipes && R.machines * 4 > R.industrial && R.machines * 10 < R.industrial * 9,
          fmt("%d pairs of the same age's and class's instruments of %d peoples: %.1f%% the same plan and body, %.2f%% whose pictures look alike (the closest %.1f, the median %.1f); %d classes, %d with too few plans%s; prehistoric pipes of bone %d of %d; a cylinders' machine %d of %d peoples",
              R.pairs, R.peoples, sp, al, R.minD, R.median, R.classes, R.poorClasses, R.poor.c_str(), R.bonePipes, R.pipes, R.machines, R.industrial));
    check("instruments: the decoder names them as the ruins do (C-17)", unnamed == 0 && makes > 0, fmt("%d of %d instruments named otherwise on the decoder", unnamed, makes));
}

// C-11: the unit's checks of the instruments: each age world's people has instruments of its own age from its own ensemble (the voices the
// decoder's tradition plays its pieces on, read the same through the lore the view reads), none that a hand cannot carry, an industrial
// gramophone; they lie beside their music records of their age over the world's places, the records where they were without them; a phrase
// on each of a world's own age sounds within the levels at its scale's pitch; and each age's is found, taken and played (`instrumentsScenes`)
void instrumentChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    int worlds = 0, ownAge = 0, same = 0, compared = 0, gramophones = 0, heavy = 0, sites = 0, beside = 0, places = 0, placesWith = 0, recordsSame = 0, recordsCompared = 0;
    int rendered = 0, levelsOk = 0, pitched = 0, pitchOk = 0; std::string bad;
    for (int a = 0; a < AGE_COUNT; a++) {
        const AgeWorld& W = AGE_WORLDS[a];
        Star s; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, s, sys, true)) continue; if (W.body >= (int)sys.bodies.size()) continue;
        const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) continue;
        worlds++;
        const Lore Lq = loreQuick(sys, b, g, W.which), Lf = loreOf(sys, b, g, W.which);
        const Tradition Tq = traditionOf(g, Lq), Tf = traditionOf(g, Lf);
        PeopleInstruments P; instrumentsOf(Tq, Lq.which, P);
        if (!P.byAge[a].empty()) ownAge++;
        for (int x = 0; x < AGE_COUNT; x++) for (const Instrument& I : P.byAge[x]) {
            if (I.disc) gramophones++;
            const Timbre *q = instrumentTimbre(Tq, I), *f = instrumentTimbre(Tf, I);
            if (q && f) { compared++; if (q->name == f->name && q->family == f->family && q->gain == f->gain && q->attack == f->attack && q->decay == f->decay) same++; }
            if (q && !I.pads && (q->family == TF_VOICE || q->family == TF_SYNVOICE || q->family == TF_KEYS || (q->family == TF_ORGAN && q->kind != 2))) heavy++;
        }
        std::vector<RecordPlace> pl; recordPlacesOf(g, W.which, pl, 3);
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
        const Culture cu = cultureOf(g, std::min(W.which, std::max(0, peoplesOf(g) - 1)));
        for (size_t pi = 0; pi < pl.size(); pi++) {
            const RecordPlace& rp = pl[pi];
            places++; if (!rp.instruments.empty()) placesWith++;
            for (const InstrumentSite& is : rp.instruments) {
                sites++;
                const int local = shardLocalOf(is.record);
                const bool music = local >= 0 && local < C->count && C->entries[local].category == RC_MUSIC && C->entries[local].age == is.age;
                double d = 1e9; for (const ShardSite& x : rp.sites) if (x.index == is.record) d = std::min(d, std::hypot(x.x - is.x, x.z - is.z));
                if (music && d <= 1.6) beside++;
            }
            if (pi % 5 == 0 && rp.work < 0) {   // the records asked for without the instruments: the same, where they were
                std::vector<ShardSite> bare; recordSitesOf(g, rp.spec, cu, bare);
                recordsCompared++;
                bool eq = bare.size() == rp.sites.size();
                for (size_t k = 0; eq && k < bare.size(); k++) eq = bare[k].index == rp.sites[k].index && bare[k].x == rp.sites[k].x && bare[k].z == rp.sites[k].z && bare[k].heading == rp.sites[k].heading;
                if (eq) recordsSame++;
            }
        }
        for (const Instrument& I : P.byAge[a]) {
            const InstrumentRender ir = renderInstrument(Tq, I, nullptr);
            rendered++;
            if (ir.r.nan == 0 && ir.r.peak < 0.95 && ir.r.peak > 0.05 && ir.r.rms > 0.006 && ir.r.rms < 0.25) levelsOk++;
            else bad += fmt(" %s (peak %.2f rms %.3f)", instrumentName(Tq, I).c_str(), ir.r.peak, ir.r.rms);
            if (ir.pitch > 0) { pitched++; if (std::fabs(ir.pitch / ir.hz0 - 1) < 0.04) pitchOk++; else bad += fmt(" %s at %.1f Hz for %.1f", instrumentName(Tq, I).c_str(), ir.pitch, ir.hz0); }
        }
    }
    check("instruments: each age's, from its people's ensemble (C-11)", worlds == AGE_COUNT && ownAge == worlds && compared > 0 && same == compared && heavy == 0,
          fmt("%d age worlds, %d with instruments of their own age; %d of %d timbres the decoder's; %d too heavy to carry; %d cylinder machines (C-17: where their people made one)", worlds, ownAge, same, compared, heavy, gramophones));
    check("instruments: beside the music records (C-11)", sites > 0 && beside == sites && recordsCompared > 0 && recordsSame == recordsCompared,
          fmt("%d instruments over %d places (%d holding one), %d beside a music record of their age within 1.6 m; the records the same without them in %d of %d places", sites, places, placesWith, beside, recordsSame, recordsCompared));
    check("instruments: their sound (C-11)", rendered > 0 && levelsOk == rendered && pitched > 0 && pitchOk == pitched,
          fmt("%d of each world's own age played: %d within the levels, %d of %d pitched ones at their first note's pitch within 4%%%s", rendered, levelsOk, pitchOk, pitched, bad.c_str()));
    std::vector<std::string> lines;
    const int f = instrumentsScenes(-1, &lines);
    std::string sum; for (const std::string& l : lines) sum += " | " + l.substr(0, std::min<size_t>(l.size(), 60));
    check("instruments: one of each age found, taken and played (C-11)", f == 0 && (int)lines.size() == AGE_COUNT, fmt("%d failures%s", f, sum.c_str()));
}

// C-11: the unit's checks of the ship's player: the decoder's page lists a piece and a recording held and a recording the radar caught; a piece
// played there plays on as the decoder closes and through a Vimana flight (its clock running, its line on the space HUD), moves on to the next
// when it ends, is held while the decoder plays a record of its own and taken up where it was, plays what the radar caught off the air, and
// stops at a landing; the synth begins a piece and a speech where it is told
void playerChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    const AgeWorld& W = AGE_WORLDS[AGE_MODERN];
    Star s; StarSystem sys; bool ok = starInSector(W.sx, 0, W.sz, s, true);
    if (ok) { sys.generate(s); ok = W.body < (int)sys.bodies.size(); }
    if (!ok) { check("the player: through a flight to a landing (C-11)", false, "no modern age world"); return; }
    const BodyGen g = BodyGen::make(sys.bodies[W.body]);
    std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
    int piece = -1, rec = -1, heard = -1;
    for (int i = 0; i < C->count; i++) {
        const RecordEntry& e = C->entries[i];
        if (e.last) continue;
        if (e.category == RC_MUSIC && piece < 0) piece = i;
        else if (e.category == RC_TEXT && recordMediumSounds(e.medium) && rec < 0) rec = i;
        else if (e.category == RC_TEXT && !recordMediumSounds(e.medium) && heard < 0) heard = i;
    }
    if (piece < 0 || rec < 0 || heard < 0) { check("the player: through a flight to a landing (C-11)", false, fmt("records: piece %d recording %d heard %d", piece, rec, heard)); return; }
    remove("shots/tests/test_player_guide.txt");
    TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_player_guide.txt";
    applyTestScale(game); game.newGame(); game.setState(GameState::SPACE);
    Input gi;
    auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
    auto press = [&](int key) { pressKey(game, gi, key); };
    const std::string key = Guide::bodyKey(s.sx, s.sy, s.sz, W.body) + "/S";
    game.guide.shards.insert(key + std::to_string(shardWorldIndex(W.which, piece)));
    game.guide.shards.insert(key + std::to_string(shardWorldIndex(W.which, rec)));
    game.guide.heard.insert(key + std::to_string(shardWorldIndex(W.which, heard)));
    game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_TAB); run(0.1);
    const int n = game.testPlayerItems();
    int pi = -1, ri = -1, hi = -1;
    for (int i = 0; i < n; i++) { const std::string l = game.testPlayerItemLine(i); if (l.find(" music") != std::string::npos) pi = i; else if (l.find("off the air") != std::string::npos) hi = i; else ri = i; }
    check("the player: lists the pieces, the recordings and what the radar caught (C-11)", n == 3 && pi >= 0 && ri >= 0 && hi >= 0 && game.testShardsTop() == 1,
          fmt("%d items on the page%s; %s", n, game.testShardsTop() == 1 ? "" : " (NOT the player's page)", n > 0 ? game.testPlayerItemLine(0).c_str() : "-"));
    for (int k = 0; k < pi; k++) press(KEY_DOWN);
    press(KEY_ENTER);
    // KI-410 (F-07): the world's records are written on a thread the first time: the wait held at one instant (frames at dt 0), so a
    // loaded machine's late thread is not read as a piece that did not play
    for (const double tw = nowSec(); game.testPlayerWaiting() && nowSec() - tw < 20;) { std::this_thread::sleep_for(std::chrono::milliseconds(2)); game.frame(gi, 0); gi.newFrame(); }
    run(0.3);
    const bool playing0 = game.testPlayerPlaying() == pi && game.testPlayerOwnsPiece();
    press(KEY_TAB); press(KEY_ESCAPE); run(0.2);
    const bool afterClose = game.state == GameState::SPACE && game.testPlayerOwnsPiece();
    press(KEY_R); press(KEY_N); run(0.2); press(KEY_ENTER); run(0.2); press(KEY_V); run(2.0);   // the nearest star, the drive
    const bool flying = game.testShipMode() == (int)ShipState::VIMANA;
    const double t1 = game.testPlayerT(); run(1.0); const double t2 = game.testPlayerT();
    const std::string hud = game.testPlayerHud();
    const bool through = flying && game.testPlayerOwnsPiece() && t2 > t1 + 0.9 && !hud.empty();
    const std::string infoFlight = game.testPlayerInfo();
    // its end: the list moves on
    double secs = 0; { const std::string inf = game.testPlayerInfo(); const size_t of = inf.find(" of "); if (of != std::string::npos) secs = atof(inf.c_str() + of + 4); }
    game.testPlayerSkipTo(std::max(0.0, secs - 0.2)); run(2.2);
    const int next = game.testPlayerPlaying();
    const bool movedOn = game.testPlayerFinished() == 1 && (pi + 1 < n ? next == pi + 1 && (game.testPlayerOwnsSpeech() || game.testPlayerOwnsPiece()) : next < 0);   // the next down the list, or the list's end
    // the recording through the flight too
    game.testPlayerPlay(ri); run(0.3);
    const double r1 = game.testPlayerT(); run(1.0); const double r2 = game.testPlayerT();
    const bool recThrough = game.testShipMode() == (int)ShipState::VIMANA && game.testPlayerOwnsSpeech() && r2 > r1 + 0.9 && game.audio.medium == C->entries[rec].medium;
    // held while the decoder plays a record of its own, taken up where it was
    game.testPlayerSkipTo(3.0); run(0.1);
    const double heldAt0 = game.testPlayerT();
    game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
    if (game.testShardsTop() == 1) press(KEY_TAB);
    press(KEY_ENTER); run(0.1); game.testShardsOpenIndex(piece); run(0.3);
    const bool heldNow = game.testPlayerHeld() && !game.testPlayerOwnsPiece() && !game.testPlayerOwnsSpeech() && game.audio.piece;
    const double heldAt = game.testPlayerT(); run(1.0);
    const bool kept = game.testPlayerHeld() && std::fabs(game.testPlayerT() - heldAt) < 1e-6;
    press(KEY_ESCAPE); run(1.0 / 30);
    const bool resumed = !game.testPlayerHeld() && (game.testPlayerOwnsSpeech() || game.testPlayerOwnsPiece()) && std::fabs(game.testPlayerT() - heldAt) < 0.1 &&
                         (game.testPlayerOwnsSpeech() ? std::fabs(game.audio.speechFrom - heldAt) < 0.1 : game.audio.pieceFrom > 0);
    press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
    // the synth begins where it is told
    double beat = -1, said = -1;
    {
        const Lore L = loreQuick(sys, sys.bodies[W.body], g, W.which); const Tradition T = traditionOf(g, L);
        Shard sh = shardDrawOf(g, L, piece); sh.recordAge = C->entries[piece].age;
        Piece P; pieceOf(T, sh, P);
        AudioSynth syn; AudioState st; st.piece = &P; st.tradition = &T; st.pieceStart = true; st.pieceFrom = std::min(16.0, P.beats * 0.5);
        std::vector<float> buf(512); syn.render(buf.data(), (int)buf.size(), 22050, st); beat = st.pieceBeat - std::min(16.0, P.beats * 0.5);
        const Tongue tg = tongueOf(g, L); const Voice V = voiceOf(g, L, tg);
        std::vector<DecodedWord> w; for (const char* x : {"one", "two", "three", "four", "five", "six"}) { DecodedWord d; d.ours = d.theirs = x; d.known = true; w.push_back(d); }
        Speech S; speechOf(V, w, 77, S);
        AudioSynth syn2; AudioState st2; st2.speech = &S; st2.voice = &V; st2.speechStart = true; st2.speechFrom = S.seconds * 0.5;
        syn2.render(buf.data(), (int)buf.size(), 22050, st2); said = st2.speechT - S.seconds * 0.5;
    }
    check("the player: held by the decoder's own record, taken up where it was (C-11)", heldNow && kept && resumed && beat >= 0 && beat < 0.2 && said >= 0 && said < 0.1,
          fmt("held %s at %.2f s (%.2f before the decoder), kept %s, resumed %s; the synth begun %.3f beats and %.3f s past where it was told", heldNow ? "yes" : "NO", heldAt, heldAt0, kept ? "yes" : "NO", resumed ? "yes" : "NO", beat, said));
    // what the radar caught: off the air
    game.testPlayerPlay(hi); run(0.2);
    const bool air = game.testPlayerOwnsSpeech() && game.audio.medium == MEDIUM_AIR;
    check("the player: a recording the radar caught plays as kept off the air (C-11)", air, game.testPlayerInfo());
    // a landing stops it
    game.testPlayerPlay(pi); run(0.2);
    const bool before = game.testPlayerOwnsPiece();
    press(KEY_V); run(0.2);
    game.testParkAt(s, W.body); run(0.3);
    { std::shared_ptr<const WorldPlan> WP = worldPlanOf(g); const int wh = std::min(W.which, std::max(0, WP->peoples - 1)); if (!WP->plans[wh].places.empty()) game.testLandCursor(WP->plans[wh].places[0].lat / DEG, WP->plans[wh].places[0].lon / DEG); }
    run(0.1); press(KEY_ENTER); run(0.5);
    const bool landed = game.state == GameState::DESCENT || game.state == GameState::SURFACE;
    const bool stopped = landed && game.testPlayerPlaying() < 0 && !game.testPlayerOwnsPiece() && !game.testPlayerOwnsSpeech();
    check("the player: plays on through a Vimana flight, stops at a landing (C-11)", playing0 && afterClose && through && movedOn && recThrough && before && stopped,
          fmt("the piece played %s, after the decoder %s, in the flight %s (%.1f -> %.1f s, the HUD '%s'), at its end %s (item %d of %d); the recording in the flight %s (%.1f -> %.1f s, medium %d); the landing %s (state %d)",
              playing0 ? "yes" : "NO", afterClose ? "on" : "OFF", through ? "on" : "OFF", t1, t2, hud.c_str(), movedOn ? "the list on" : "NOT MOVED ON", next, n, recThrough ? "on" : "OFF", r1, r2, game.audio.medium,
              stopped ? "stopped it" : "DID NOT STOP IT", (int)game.state));
    (void)infoFlight;
    remove("shots/tests/test_player_guide.txt");
}

int runInstruments(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    const std::string what = argc > 2 ? argv[2] : "ages";
    if (what == "wav") { const int f = instrumentsWavs(); printf("instruments wav: %d failure%s\n", f, f == 1 ? "" : "s"); return f ? 1 : 0; }
    if (what == "scenes") {
        int age = -1; if (argc > 3) for (int a = 0; a < AGE_COUNT; a++) if (std::string(argv[3]) == AGE_NAMES[a] || (a == AGE_FAR_FUTURE && std::string(argv[3]) == "far")) age = a;
        const int f = instrumentsScenes(age); printf("instruments scenes: %d failure%s\n", f, f == 1 ? "" : "s"); return f ? 1 : 0;
    }
    if (what == "ages") { int f = 0; for (int a = 0; a < AGE_COUNT; a++) { printf("== %s (the age test world)\n", AGE_NAMES[a]); f += printInstruments(AGE_WORLDS[a]); } printf("instruments ages: %d failure%s\n", f, f == 1 ? "" : "s"); return f ? 1 : 0; }
    if (what == "sheet") {   // C-17: each age's instruments of peoples near home, a people a row
        int age = -1; if (argc > 3) for (int a = 0; a < AGE_COUNT; a++) if (std::string(argv[3]) == AGE_NAMES[a] || (a == AGE_FAR_FUTURE && std::string(argv[3]) == "far")) age = a;
        const int n = argc > 4 ? std::max(1, atoi(argv[4])) : 10, scale = argc > 5 ? std::max(1, atoi(argv[5])) : 1;
        int f = 0; for (int a = 0; a < AGE_COUNT; a++) if (age < 0 || a == age) f += instrumentsSheet(a, n, scale);
        return f ? 1 : 0;
    }
    if (what == "plans") { const int k = argc > 3 ? std::max(1, atoi(argv[3])) : 2, scale = argc > 4 ? std::max(1, atoi(argv[4])) : 1; return instrumentsPlans(k, scale); }   // C-17: every plan
    if (what == "distinct") return instrumentsDistinct(argc > 3 ? std::max(2, atoi(argv[3])) : 100);   // C-17: the measure
    if (what == "check") {   // the unit's C-11 checks alone (the instruments' and the player's)
        int failures = 0; const Check ck = modeCheck(failures);
        instrumentChecks(ck); makerChecks(ck); playerChecks(ck);
        printf("instruments check: %d failure%s\n", failures, failures == 1 ? "" : "s"); return failures ? 1 : 0;
    }
    const int which = takePeopleArg(argc, argv);
    if (argc > 4) return printInstruments(AgeWorld{atoll(argv[2]), atoll(argv[3]), atoi(argv[4]), which});
    printf("instruments [ages] | instruments <sx> <sz> <body> [second] | instruments wav | instruments scenes [age] | instruments check | instruments sheet [age] [peoples] [scale] | instruments plans [peoples] [scale] | instruments distinct [peoples]\n");
    return 1;
}
