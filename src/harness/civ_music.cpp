// E-03: the peoples' music (C-04): `music`
#include "harness/harness.h"
#include "harness/test_worlds.h"

// C-04: `music <sx> <sz> <body index> [index] [wav]`: a world's musical tradition (its scale in cents, its cycle, its tempo, its
// timbres, its ornament and texture), the pieces among its fifty shards (index, year, tone, form, length, notes), then one piece
// (the given index, else the first piece) as notation: a line per voice per cycle, a degree at each subdivision ('.' a held note,
// '-' silence, 'o'/'x' the drum's skins); `wav` renders it through the synth to shots/tests/music_<sx>_<sz>_<body>_S<index>.wav
// H-05: a piece through the synth (the game's master level, as C-04's and C-05's renders): its peak, rms, non-finite and at-the-ceiling counts,
// whether it ended, the synth's time; `fn` a WAV when given
struct MusicRender { double peak = 0, rms = 0, secs = 0, ms = 0, loudest = 0; int nan = 0, clip = 0; bool done = false; };
static MusicRender renderMusicWav(const Tradition& T, const Piece& P, const std::string& fn) {
    AudioSynth synth; AudioState st; st.piece = &P; st.tradition = &T; st.pieceStart = true;
    const int sr = 22050; std::vector<float> buf(2048); std::vector<int16_t> pcm; MusicRender r; double sum = 0; long cnt = 0;
    const int total = (int)(sr * (P.seconds + 3)); int frames = 0;
    double win = 0; int winN = 0;
    const double t0 = nowSec();
    while (frames < total) {
        synth.render(buf.data(), (int)buf.size(), sr, st);
        for (float v : buf) {
            if (!std::isfinite(v)) r.nan++;
            if (std::fabs(v) >= 0.999f) r.clip++;
            r.peak = std::max(r.peak, (double)std::fabs(v)); sum += (double)v * v; cnt++;
            win += (double)v * v; if (++winN == sr / 2) { r.loudest = std::max(r.loudest, std::sqrt(win / winN)); win = 0; winN = 0; }
            if (!fn.empty()) pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32000));
        }
        frames += (int)buf.size();
    }
    r.ms = (nowSec() - t0) * 1e3; r.rms = std::sqrt(sum / std::max(1L, cnt)); r.secs = (double)frames / sr; r.done = st.pieceDone;
    if (!fn.empty()) { makeDir("shots"); makeDir("shots/tests"); writeWav16(fn, pcm, sr); }
    return r;
}
// the notes of a piece as notation, cycle by cycle (C-04's print): each voice a line of the cycle's subdivisions
static void printPieceNotation(const Tradition& T, const Piece& P, int maxCycles) {
    const Ensemble& E = pieceEnsemble(T, P);
    int nv = (int)E.voices.size(), cu = T.beats * T.sub;
    if (P.age == AGE_FAR_FUTURE) {   // a field: each tone's spans in seconds
        for (int v = 0; v < nv; v++) {
            std::string line; int count = 0;
            for (const Note& nt : P.notes) if (nt.voice == v && count < 14) { line += fmt(" [%d %.0f-%.0f]", nt.degree, nt.t, nt.t + nt.dur); count++; }
            if (!line.empty()) printf("    %-16s%s%s\n", E.voices[v].name.c_str(), line.c_str(), count >= 14 ? " ..." : "");
        }
        return;
    }
    for (int c = 0; c < P.cycles && c < maxCycles; c++) {
        for (int v = -1; v < nv; v++) {
            std::string line(cu, '-'); bool any = false;
            for (const Note& nt : P.notes) {
                if ((v < 0) != (nt.voice < 0) || (v >= 0 && nt.voice != v)) continue;
                int u0 = (int)std::floor(nt.t * T.sub + 1e-6) - c * cu, u1 = (int)std::ceil((nt.t + nt.dur) * T.sub - 1e-6) - c * cu;
                if (u1 <= 0 || u0 >= cu) continue;
                for (int u = std::max(u0, 0); u < std::min(u1, cu); u++) {
                    char ch = '.';
                    if (u == u0) ch = v < 0 ? (char)('0' + std::min(nt.degree, 9)) : (char)(nt.degree >= 0 && nt.degree < 10 ? '0' + nt.degree : (nt.degree >= 10 ? 'A' + std::min(nt.degree - 10, 25) : 'a' + std::min(-nt.degree - 1, 25)));
                    if (line[u] == '-' || u == u0) line[u] = ch;
                }
                any = true;
            }
            if (any) printf("    cycle %2d %-16s %s\n", c + 1, v < 0 ? "kit" : trunc(E.voices[v].name, 16).c_str(), line.c_str());
        }
    }
}
// H-05: `music ages [wav]`: music by age. For each age, the age world's people: its tradition's core and its ensemble of that age, then two
// or three pieces of its records of that age (different forms; a record of another tone made to fill a form the records lack), each with
// its instruments, notes and length, and rendered (`wav`: shots/tests/music_age_<age>_<form>.wav) with its levels; the notation of the
// first; then one late people's pieces across its eras (the same people, three ages), two peoples of one age side by side, the synth's cost
static int musicDecoderFrames();
static int runMusicAges(int argc, char** argv) {
    bool wav = false; for (int i = 3; i < argc; i++) { if (std::string(argv[i]) == "wav") wav = true; if (std::string(argv[i]) == "frames") return musicDecoderFrames(); }
    static const char* AGE_FILE[AGE_COUNT] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "farfuture"};
    auto slug = [](const char* name) { std::string s; for (const char* c = name; *c; c++) { if (*c == ' ') { if (!s.empty() && s.back() != '_') s += '_'; } else if (isalpha((unsigned char)*c)) s += (char)tolower((unsigned char)*c); } if (s.compare(0, 2, "a_") == 0) s = s.substr(2); else if (s.compare(0, 3, "an_") == 0) s = s.substr(3); else if (s.compare(0, 4, "the_") == 0) s = s.substr(4); return s; };
    struct World { StarSystem sys; int body = -1, which = 0; BodyGen g; Lore L; Tradition T; bool ok = false; };
    auto load = [](int64_t sx, int64_t sz, int body, int which, World& w) {
        Star s; if (!starInSector(sx, 0, sz, s, true)) return false;
        w.sys.generate(s); if (body < 0 || body >= (int)w.sys.bodies.size()) return false;
        w.body = body; w.which = which; w.g = BodyGen::make(w.sys.bodies[body]); w.L = loreQuick(w.sys, w.sys.bodies[body], w.g, which); w.T = traditionOf(w.g, w.L); w.ok = true;
        return true;
    };
    int failures = 0; double worstPeak = 0, rmsLo = 1, rmsHi = 0; int rendered = 0;
    auto report = [&](const Tradition& T, const Piece& P, const std::string& tag, const std::string& file) {
        const MusicRender r = renderMusicWav(T, P, wav ? file : std::string());
        rendered++; worstPeak = std::max(worstPeak, r.peak); rmsLo = std::min(rmsLo, r.rms); rmsHi = std::max(rmsHi, r.rms);
        const bool ok = r.nan == 0 && r.clip == 0 && r.peak < 0.9 && r.rms >= 0.06 && r.rms <= 0.16 && r.done;
        if (!ok) failures++;
        printf("    %-24s %-22s %3.0f s %3.0f bpm %4zu notes  peak %.2f rms %.3f (loudest half second %.2f)%s%s%s  %s\n", tag.c_str(), PIECE_FORM_NAMES[P.form], P.seconds, P.bpm, P.notes.size(), r.peak, r.rms, r.loudest,
               r.nan ? fmt(" %d NON-FINITE", r.nan).c_str() : "", r.clip ? fmt(" %d AT THE CEILING", r.clip).c_str() : "", r.done ? "" : " NOT ENDED", ok ? "" : "  <- OUT OF BOUNDS");
        printf("      %s; %s\n", timbresLine(T, P).c_str(), cycleLine(T, P).c_str());
        if (wav) printf("      -> %s (rendered in %.0f ms: %.1f%% of real time)\n", file.c_str(), r.ms, 100 * r.ms / 1000 / std::max(1.0, r.secs));
        return r;
    };
    for (int a = 0; a < AGE_COUNT; a++) {
        World w; const AgeWorld& aw = AGE_WORLDS[a];
        if (!load(aw.sx, aw.sz, aw.body, aw.which, w)) { printf("== %s: the age world did not load\n", AGE_NAMES[a]); failures++; continue; }
        const Tradition& T = w.T;
        printf("== %s: %s, the %s (%d records)\n", AGE_NAMES[a], w.sys.bodies[w.body].name.c_str(), w.L.people.c_str(), w.L.records);
        std::string vw; for (int v : T.vowels) vw += "aeiou"[v];
        printf("  the core: %s (%s), the tonic %.0f Hz; %s; swing %.2f; the singers' vowels %s; ornament %d\n", traditionLine(T).c_str(), T.tuning == 0 ? fmt("%d equal steps", T.division).c_str() : "just ratios", T.base, cycleLine(T, T.bpm).c_str(), T.swing, vw.c_str(), T.ornament);
        printf("  the %s ensemble: %s\n", AGE_NAMES[a], ensembleLine(T.ages[a]).c_str());
        // its music records of this age, by form; then the forms they lack filled from records of other tones
        std::vector<Shard> pick; std::set<int> forms; int musicOfAge = 0;
        for (int i = 0; i < w.L.records; i++) {
            Shard d = shardDrawOf(w.g, w.L, i);
            if (!d.music || d.recordAge != a) continue;
            musicOfAge++;
            Piece P; pieceOf(T, d, P);
            if (forms.count(P.form) || pick.size() >= 3) continue;
            forms.insert(P.form); pick.push_back(d);
        }
        for (int tone = 1; tone < 3 && pick.size() < 3; tone++) {   // an elegy and a warning to fill
            Shard d = pick.empty() ? shardDrawOf(w.g, w.L, 0) : pick[0]; d.recordAge = a; d.tone = tone; d.seed = mix64(d.seed ^ (uint64_t)(tone * 7919));
            Piece P; pieceOf(T, d, P); if (!forms.count(P.form)) { forms.insert(P.form); pick.push_back(d); }
        }
        printf("  %d of its %d records are music of this age; rendered:\n", musicOfAge, w.L.records);
        bool first = true;
        for (const Shard& d : pick) {
            Piece P; pieceOf(T, d, P);
            report(T, P, fmt("S%d year %d", (int)(d.seed % 1000) >= 0 ? (int)0 : 0, d.year).substr(0, 0) + fmt("year %d (%s)", d.year, SHARD_TONE_NAMES[d.tone]), fmt("shots/tests/music_age_%s_%s.wav", AGE_FILE[a], slug(PIECE_FORM_NAMES[P.form]).c_str()));
            if (first) { printPieceNotation(T, P, 3); first = false; }
        }
    }
    {   // the same people, three ages: the modern world's records of its medieval, industrial and modern years
        World w; const AgeWorld& aw = AGE_WORLDS[AGE_MODERN];
        if (load(aw.sx, aw.sz, aw.body, aw.which, w)) {
            printf("== the same people, three ages: the %s of %s\n", w.L.people.c_str(), w.sys.bodies[w.body].name.c_str());
            for (int a = AGE_MEDIEVAL; a <= AGE_MODERN; a++) {
                for (int i = 0; i < w.L.records; i++) {
                    Shard d = shardDrawOf(w.g, w.L, i);
                    if (!d.music || d.recordAge != a || d.tone != ST_ORDINARY) continue;
                    Piece P; pieceOf(w.T, d, P);
                    report(w.T, P, fmt("%s, year %d", AGE_NAMES[a], d.year), fmt("shots/tests/music_people_%s.wav", AGE_FILE[a]));
                    break;
                }
            }
        }
    }
    {   // two peoples of one age: the industrial world's and the modern world's industrial years; the medieval world's and the industrial world's medieval years
        const int pairs[2][2] = {{AGE_INDUSTRIAL, AGE_MODERN}, {AGE_MEDIEVAL, AGE_INDUSTRIAL}};
        const int pairAge[2] = {AGE_INDUSTRIAL, AGE_MEDIEVAL};
        for (int pp = 0; pp < 2; pp++) {
            printf("== two peoples of the %s age\n", AGE_NAMES[pairAge[pp]]);
            for (int k = 0; k < 2; k++) {
                World w; const AgeWorld& aw = AGE_WORLDS[pairs[pp][k]];
                if (!load(aw.sx, aw.sz, aw.body, aw.which, w)) continue;
                printf("  the %s (%s): %s\n    %s\n", w.L.people.c_str(), w.sys.bodies[w.body].name.c_str(), cycleLine(w.T, w.T.bpm).c_str(), ensembleLine(w.T.ages[pairAge[pp]]).c_str());
                for (int i = 0; i < w.L.records; i++) {
                    Shard d = shardDrawOf(w.g, w.L, i);
                    if (!d.music || d.recordAge != pairAge[pp] || d.tone != ST_ORDINARY) continue;
                    Piece P; pieceOf(w.T, d, P);
                    report(w.T, P, fmt("the %s", w.L.people.c_str()), fmt("shots/tests/music_pair_%s_%s.wav", AGE_FILE[pairAge[pp]], slug(w.L.people.c_str()).c_str()));
                    break;
                }
            }
        }
    }
    {   // the synth's cost: the densest piece of every age world's own age, a minute of it
        double worst = 0; std::string what;
        for (int a = 0; a < AGE_COUNT; a++) {
            World w; const AgeWorld& aw = AGE_WORLDS[a]; if (!load(aw.sx, aw.sz, aw.body, aw.which, w)) continue;
            for (int i = 0; i < w.L.records; i++) {
                Shard d = shardDrawOf(w.g, w.L, i); if (!d.music || d.recordAge != a) continue;
                Piece P; pieceOf(w.T, d, P);
                const MusicRender r = renderMusicWav(w.T, P, std::string());
                const double perMin = r.ms / std::max(1.0, r.secs) * 60;
                if (perMin > worst) { worst = perMin; what = fmt("%s %s", AGE_NAMES[a], PIECE_FORM_NAMES[P.form]); }
                break;
            }
        }
        printf("== the synth: the dearest piece takes %.0f ms a minute of music (%s): %.1f%% of a core\n", worst, what.c_str(), worst / 600);
    }
    printf("music ages: %d rendered, peaks to %.2f, rms %.3f-%.3f, %d out of bounds (peak under 0.9, rms 0.06-0.16, ended, nothing at the ceiling)\n", rendered, worstPeak, rmsLo, rmsHi, failures);
    return failures ? 1 : 0;
}

// H-05: `music ages frames`: a piece of each age on the decoder (the age world's records of its age in the guide, the first opened and played two
// seconds): shots/tests/music_decoder_<age>.png, the roll in the parts' colours
static int musicDecoderFrames() {
    static const char* AGE_FILE[AGE_COUNT] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "farfuture"};
    TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_music.txt";
    remove("shots/tests/test_guide_music.txt");
    applyTestScale(game);
    game.newGame(); game.setState(GameState::SPACE);
    Input in;
    auto run = [&](double secs) { runFrames(game, in, secs); };
    auto press = [&](int key) { pressKey(game, in, key); };
    int shots = 0;
    for (int a = 0; a < AGE_COUNT; a++) {
        const AgeWorld& aw = AGE_WORLDS[a];
        Star s; if (!starInSector(aw.sx, 0, aw.sz, s, true)) continue;
        StarSystem sys; sys.generate(s); const Body& b = sys.bodies[aw.body]; BodyGen g = BodyGen::make(b);
        Lore L = loreQuick(sys, b, g, aw.which); Tradition T = traditionOf(g, L);
        int pick = -1;
        game.guide.shards.clear(); game.guide.decoded.clear();
        for (int i = 0; i < L.records; i++) {
            Shard d = shardDrawOf(g, L, i);
            if (!d.music || d.recordAge != a) continue;
            if (pick < 0) { Piece P; pieceOf(T, d, P); if (P.form != PF_LAST) pick = i; }
            game.guide.shards.insert(fmt("%lld,0,%lld/%d/S%d", (long long)aw.sx, (long long)aw.sz, aw.body, shardWorldIndex(L.which, i)));
            if (game.guide.shards.size() >= 6) break;
        }
        if (pick < 0) { printf("  %s: no piece of the age in the world's records\n", AGE_NAMES[a]); continue; }
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_ENTER); run(0.3);
        const bool opened = game.testShardsOpenIndex(pick); run(a == AGE_FAR_FUTURE ? 20.0 : 6.0);
        const std::string fn = fmt("shots/tests/music_decoder_%s.png", AGE_FILE[a]);
        writePNG(fn.c_str(), game.output(), FBW, FBH); shots++;
        printf("  %s: record %d %s -> %s (%s)\n", AGE_NAMES[a], pick, opened ? "opened" : "NOT OPENED", fn.c_str(), game.testShardsInfo().c_str());
        press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1); game.setState(GameState::SPACE); run(0.1);
    }
    return shots == AGE_COUNT ? 0 : 1;
}

int runMusic(int argc, char** argv) {
    if (argc > 2 && std::string(argv[2]) == "ages") return runMusicAges(argc, argv);   // H-05
    int which = takePeopleArg(argc, argv);   // C-13: `second` for the world's second people
    // H-05: `age=N` and `tone=N` play the record as if it were of that age and tone (the harness's comparisons)
    int forceAge = -1, forceTone = -1; std::vector<std::string> rest;
    for (int i = 2; i < argc; i++) { std::string x = argv[i]; if (x.compare(0, 4, "age=") == 0) forceAge = atoi(x.c_str() + 4); else if (x.compare(0, 5, "tone=") == 0) forceTone = atoi(x.c_str() + 5); else rest.push_back(x); }
    if (rest.size() < 3) { printf("music <sx> <sz> <body index> [index] [wav] [age=N] [tone=N] [second] | music ages [wav]\n"); return 1; }
    int64_t sx = atoll(rest[0].c_str()), sz = atoll(rest[1].c_str()); int bi = atoi(rest[2].c_str());
    int want = rest.size() > 3 && rest[3] != "wav" ? atoi(rest[3].c_str()) : -1;
    bool wav = (rest.size() > 3 && rest[3] == "wav") || (rest.size() > 4 && rest[4] == "wav");
    Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    StarSystem sys; sys.generate(s);
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) printf("(%s had no people: the music below is what it would have played)\n", b.name.c_str());
    Lore L = loreQuick(sys, b, g, which);
    Tradition T = traditionOf(g, L);
    printf("%s, %s of %s: the music of the %s (names of style %d)\n", b.name.c_str(), KINDS[b.type].name, s.name.c_str(), L.people.c_str(), L.style);
    printf("  %s (%s), the period %.3f, the tonic %.1f Hz; degrees in cents:", traditionLine(T).c_str(), T.tuning == 0 ? fmt("%d equal steps", T.division).c_str() : "just ratios", T.period, T.base);
    for (int d = 0; d < (int)T.scale.size(); d++) printf(" %.0f", traditionCents(T, d));
    printf("; resting on degree %d (%.0f cents)\n", T.rest, traditionCents(T, T.rest));
    std::string vw; for (int v : T.vowels) vw += "aeiou"[v];
    printf("  %s, each beat in %d, drifting %.0f%%, swing %.2f, the singers' vowels %s\n", cycleLine(T, T.bpm).c_str(), T.sub, 100 * T.drift, T.swing, vw.c_str());
    static const char* SECOND[4] = {"no second voice", "a second voice in parallel below", "a second voice answering a phrase later", "a second voice doubling a period below"};
    static const char* ORN[5] = {"no ornament", "grace notes before a leap", "trills on long notes", "slides between neighbours", "mordents"};
    printf("  its own voices (C-04's): %s; %s; %s; %s %.0f%% of the time\n", timbresLine(T, T.hasDrum).c_str(), T.drone ? "a drone" : "no drone", SECOND[T.second], ORN[T.ornament], 100 * T.ornamentChance);
    for (int a = 0; a < AGE_COUNT; a++) printf("  %-11s %s\n", AGE_NAMES[a], ensembleLine(T.ages[a]).c_str());
    // its records' pieces (H-05: every record of the people, each of its year's age), by year
    std::vector<Shard> pieces;
    for (int i = 0; i < L.records; i++) { Shard d = shardDrawOf(g, L, i); if (!d.music) continue; if (forceAge >= 0) d.recordAge = forceAge; if (forceTone >= 0) d.tone = forceTone; pieces.push_back(d); }
    std::vector<int> order(pieces.size()); for (size_t i = 0; i < order.size(); i++) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [&](int a2, int c) { return pieces[a2].year < pieces[c].year; });
    int forms[PF_COUNT] = {0}; double secs = 0; int byAge[AGE_COUNT] = {0}; int first = -1;
    std::vector<int> indexOf(pieces.size(), -1);
    { int k = 0; for (int i = 0; i < L.records; i++) { Shard d = shardDrawOf(g, L, i); if (d.music) indexOf[k++] = i; } }
    for (int oi : order) {
        const Shard& d = pieces[oi]; Piece P; pieceOf(T, d, P); forms[P.form]++; secs += P.seconds; byAge[P.age]++;
        if (first < 0) first = oi;
        printf("  %3d  year %4d  %-11s %-8s  %-22s %3.0f s  %3d bpm  %3zu notes  %s\n", indexOf[oi], d.year, AGE_NAMES[P.age], SHARD_TONE_NAMES[d.tone], PIECE_FORM_NAMES[P.form], P.seconds, (int)std::lround(P.bpm), P.notes.size(), timbresLine(T, P).c_str());
    }
    printf("%zu pieces of %d records, %.0f s in all; by age:", pieces.size(), L.records, secs);
    for (int a = 0; a < AGE_COUNT; a++) if (byAge[a]) printf(" %s %d;", AGE_NAMES[a], byAge[a]);
    printf(" by form:");
    for (int f = 0; f < PF_COUNT; f++) if (forms[f]) printf(" %s %d;", PIECE_FORM_NAMES[f], forms[f]);
    printf("\n");
    int pick = -1;
    if (want >= 0) { for (size_t i = 0; i < pieces.size(); i++) if (indexOf[i] == want) pick = (int)i; }
    else pick = first;
    if (pick < 0) { printf("record %d is not a piece of music\n", want); return pieces.empty() ? 1 : 0; }
    const Shard& d = pieces[pick];
    Piece P; pieceOf(T, d, P);
    printf("record %d, year %d, the %s age: %s, %.0f s at %.0f bpm, %d cycles, %zu notes; %s; %s\n", indexOf[pick], d.year, AGE_NAMES[P.age], PIECE_FORM_NAMES[P.form], P.seconds, P.bpm, P.cycles, P.notes.size(), timbresLine(T, P).c_str(), cycleLine(T, P).c_str());
    printPieceNotation(T, P, 64);
    printf("  (degrees: 0-9 the tonic up, A-Z past nine, a-z below the tonic; the kit's pieces by their index)\n");
    if (wav) {
        const std::string fn = fmt("shots/tests/music_%lld_%lld_%d_S%d.wav", (long long)sx, (long long)sz, bi, indexOf[pick]);
        const MusicRender r = renderMusicWav(T, P, fn);
        printf("rendered %.1f s: peak %.3f, rms %.3f, %d non-finite, %d at the ceiling, %s -> %s (%.0f ms)\n", r.secs, r.peak, r.rms, r.nan, r.clip, r.done ? "the piece ended" : "the piece NOT ended", fn.c_str(), r.ms);
    }
    return 0;
}
