// E-03: what the peoples leave: `shards`, `charts`, `ruins`, `graves`
#include "harness/harness.h"
#include "harness/test_worlds.h"

// H-05: the star charts by age (the unit's check, and `charts check` alone)
bool checkChartsByAge(std::string& detail) {
    {   // H-05: the star charts by age. Over every chart of the six age worlds' peoples: drawn in the way of its record's age and medium; the oldest
        // with animals and the moon's own count (the days of its month, or its moons in the year), the medieval with creatures and heroes, an astrolabe
        // with their equator and the ecliptic, a page with the ecliptic, the wanderers' ways within the field; an atlas with their grid and the stars
        // numbered by brightness; a survey with its magnitudes and only the peoples the lore heard ringed; a navigation chart from their own star to
        // the star it marks among its routes, flown or meant; the far future's a far mark (300 ly and more) with three places of nothing or more
        // joined to it; each way's age world showing it; every caption naming every figure; the same twice
        int charts = 0, bad = 0, byStyle[CHS_COUNT] = {}, ageHas = 0; std::string badWhat; double loreMax = 0;
        auto fail = [&](const std::string& w) { bad++; if (badWhat.size() < 300) badWhat += w + "; "; };
        for (int a = 0; a < AGE_COUNT; a++) {
            Star s; StarSystem sys; if (!systemAt(AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, s, sys, true)) continue; if (AGE_WORLDS[a].body >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[AGE_WORLDS[a].body]; BodyGen g = BodyGen::make(b); if (!g.hasTrait(TR_CIVILISATION)) continue;
            const int which = AGE_WORLDS[a].which;
            double t0 = nowSec(); Lore L = loreOf(sys, b, g, which); loreMax = std::max(loreMax, (nowSec() - t0) * 1e3);
            std::shared_ptr<const RecordCatalogue> Cat = recordsOf(g, which);
            const BodyGen pg = peopleGen(g, which);
            const bool animals = peopleAgeOf(pg, 0).age == AGE_PREHISTORIC;
            bool hasOwn = false;
            for (int i = 0; i < Cat->count; i++) {
                if (Cat->entries[i].category != RC_CHART) continue;
                StarChart ch, ch2;
                if (!chartOf(sys, b, g, L, i, ch) || !chartOf(sys, b, g, L, i, ch2)) { fail(fmt("%s S%d no chart", b.name.c_str(), i)); continue; }
                charts++; byStyle[std::max(0, std::min(CHS_COUNT - 1, ch.style))]++;
                const std::string at = fmt("%s S%d (%s)", b.name.c_str(), i, CHART_STYLE_NAMES[ch.style]);
                if (ch.stars.size() != ch2.stars.size() || ch.figures.size() != ch2.figures.size() || ch.curves.size() != ch2.curves.size() || ch.wanderers.size() != ch2.wanderers.size() || ch.mark.star.seed != ch2.mark.star.seed) fail(at + " not the same twice");
                if (ch.age != Cat->entries[i].age || ch.medium != Cat->entries[i].medium || ch.style != chartStyleOf(ch.age, ch.medium)) fail(at + " not its record's way");
                if (ch.age == a) hasOwn = true;
                const Shard sh = shardOf(sys, b, g, L, i);
                for (const ChartFigure& f : ch.figures) if (sh.text.find(f.name) == std::string::npos) fail(at + " caption lacks " + f.name);
                if (ch.markStar < 0 || ch.markStar >= (int)ch.stars.size() || !ch.stars[ch.markStar].mark) fail(at + " mark not on it");
                if ((ch.mark.how == 3) != (ch.style == CHS_BEYOND)) fail(at + " a far mark where it does not belong");
                switch (ch.style) {
                    case CHS_CEILING: case CHS_BONE: {
                        const int want = L.calendar == 1 ? L.monthDays : (L.calendar == 2 ? L.months : 0);
                        if (ch.tally != want || (want > 0) != (ch.tallyKind > 0)) fail(at + fmt(" counts %d for the calendar's %d", ch.tally, want));
                        for (const ChartFigure& f : ch.figures) if (f.glyph < 0 || f.glyph >= 12) fail(at + " " + f.name + " no animal");
                        if (!animals) fail(at + " the oldest way of a later people");
                        break;
                    }
                    case CHS_VAULT: case CHS_ASTROLABE: case CHS_PAGE: {
                        if (animals) fail(at + " a later way of the oldest people");
                        int eq = 0, ecl = 0; for (const ChartCurve& c : ch.curves) { if (c.kind == CC_EQUATOR) eq++; if (c.kind == CC_ECLIPTIC) ecl++; for (const auto& p : c.pts) if (std::hypot(p[0], p[1]) > 1.001) fail(at + " a curve off the field"); }
                        if (ch.style != CHS_VAULT && ecl == 0) fail(at + " no ecliptic");
                        if (ch.style == CHS_ASTROLABE && eq == 0) fail(at + " no equator");
                        for (const ChartPath& w : ch.wanderers) for (const auto& p : w.pts) if (std::hypot(p[0], p[1]) > 1.001) fail(at + " a wanderer off the field");
                        break;
                    }
                    case CHS_ATLAS: {
                        int dec = 0, hr = 0; for (const ChartCurve& c : ch.curves) { if (c.kind == CC_DECLINATION) dec++; if (c.kind == CC_HOUR) hr++; }
                        if (dec < 3 || hr < 3) fail(at + fmt(" grid %d/%d", dec, hr));
                        int next = 1; for (const ChartStar& c : ch.stars) { if (c.mark || c.number == 0) continue; if (c.number != next) fail(at + " numbers not by brightness"); next++; }
                        if (next == 1) fail(at + " no star numbered");
                        break;
                    }
                    case CHS_SURVEY: {
                        int heard = 0;
                        for (const ChartStar& c : ch.stars) if (c.heard) { heard++; bool marked = false; for (const ChartMark& m : L.marks) if (m.how == 2 && m.star.seed == c.star.seed) marked = true; if (!marked) fail(at + " rings a people not heard"); }
                        if (heard != ch.heardN || ch.magLimit < 1 || ch.magStep < 1.9 || ch.magStep > 4.1) fail(at + " magnitudes or heard wrong");
                        break;
                    }
                    case CHS_ROUTES: {
                        if (ch.stars.empty() || !ch.stars[0].home || ch.stars[0].star.seed != s.seed) fail(at + " home not at the middle");
                        bool toMark = false;
                        for (const ChartFigure& f : ch.figures) { if (f.kind != 1 || f.stars.empty() || f.stars[0] != 0 || f.lines.size() + 1 != f.stars.size()) fail(at + " a route not from home"); if (!f.stars.empty() && f.stars.back() == ch.markStar) toMark = true; }
                        if (!toMark || ch.reached + ch.planned != (int)ch.figures.size()) fail(at + " the routes do not add up");
                        break;
                    }
                    case CHS_BEYOND: {
                        if (ch.mark.how != 3 || ch.mark.ly < 300 || ch.signs < 3 || ch.farKind < 1 || ch.farKind > 4) fail(at + fmt(" far mark %.0f ly, %d signs", ch.mark.ly, ch.signs));
                        for (const ChartFigure& f : ch.figures) if (f.kind != 2 || f.stars.size() != 2 || f.stars[0] != ch.markStar || !ch.stars[f.stars[1]].sign) fail(at + " a line not to a place of nothing");
                        break;
                    }
                    default: break;
                }
            }
            if (hasOwn) ageHas++;
        }
        detail = fmt("%d charts of the six age worlds: ceiling %d, bone %d, vault %d, astrolabe %d, page %d, atlas %d, photograph %d, survey %d, navigation %d, far %d; %d of 6 age worlds draw their own age's; the lore %.1f ms at most; %d wrong%s%s",
                  charts, byStyle[0], byStyle[1], byStyle[2], byStyle[3], byStyle[4], byStyle[5], byStyle[6], byStyle[7], byStyle[8], byStyle[9], ageHas, loreMax, bad, bad ? ": " : "", badWhat.c_str());
        return bad == 0 && charts > 50 && ageHas == AGE_COUNT && loreMax < 60;
    }
}

// R-304/R-305: `traits [name]`: for every landform trait the first body that carries it, the spot on it where the landform
// is strongest (a scan of the sphere at 200 m detail by `SurfaceSample::landform`), a hillshade of 8 km round the spot, a
// standing frame from 250 m west of it and an aerial frame 350 m up; one sheet per trait in `shots/tests/trait_*.png`.
// Then the share of the first 300 bodies carrying each trait
// C-01: `ruins <sx> <sz> <body index> [latDeg lonDeg]`: a world's culture, the old seas, and the settlements of the 31 x 31 ruin cells
// round a point with the site test's verdict and its reasons (the planet function read without the drainage, as the view reads it)
// C-02: `shards <sx> <sz> <body index> [n] [full | <held> [full]]`: a world's lore, then (C-06) its people's tongue and the words of
// its language the computer learns first, then its first n shards (default fifty) as the decoder reads them with `held` shards
// of the world in hand (default one: about a third of the words, the rest in the people's tongue; ten: the whole text), in the
// order of their years with the tone and the kind of each; `full` prints the whole text under each
// D-05: the drifters near home that kept a people, nearest first (within 3,000 ly: thirteen)
std::vector<Star> drifterPeoplesNearHome(double reachLy) {
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    std::vector<Star> near, out; driftersNear(home.pos, reachLy, near);
    for (const Star& n0 : near) {
        Star d; drifterInSector(n0.sx, n0.sy, n0.sz, d, true);
        StarSystem sys; sys.generate(d);
        if (!sys.bodies.empty() && drifterHadPeople(sys.bodies[0])) out.push_back(d);
    }
    return out;
}
int runShards(int argc, char** argv) {
    int which = takePeopleArg(argc, argv);   // C-13: `second` for the world's second people
    if (argc >= 4 && std::string(argv[2]) == "rogue") {   // D-05: `shards rogue <k> [n] [full | <held> [full]] [disk]`: the k-th drifter people nearest home
        std::string disk;   // `disk` the tree's grammar files, `disk=<dir>` another copy's (writers side by side)
        for (int i = 3; i < argc; i++) {
            const std::string a = argv[i];
            if (a != "disk" && a.compare(0, 5, "disk=") != 0) continue;
            disk = a == "disk" ? std::string("src/peoples/grammar") : a.substr(5);
            for (int j = i; j + 1 < argc; j++) argv[j] = argv[j + 1];
            argc--; break;
        }
        if (!disk.empty()) { std::string report; const bool ok = shardGrammarFromDisk(disk, report); printf("the grammar from %s/: %s\n%s", disk.c_str(), ok ? "read and linked" : "ERRORS", report.c_str()); }
        // `<k>` the k-th nearest home (within 3,000 ly: `peoples rogues` lists them), `age=<age>` the nearest of that age (within 12,000 ly; an age's
        // name with '_' for its space)
        const std::string key = argv[3];
        int k = -1; std::vector<Star> list;
        if (key.compare(0, 4, "age=") == 0) {
            std::string an = key.substr(4); for (char& c : an) if (c == '_') c = ' ';
            int want = -1; for (int a = 0; a < AGE_COUNT; a++) if (an == AGE_NAMES[a] || an == std::to_string(a)) want = a;
            list = drifterPeoplesNearHome(12000);
            for (int i = 0; i < (int)list.size() && k < 0; i++) { StarSystem ds; ds.generate(list[i]); if (peopleAgeOf(BodyGen::make(ds.bodies[0]), 0).age == want) k = i; }
            if (k < 0) { printf("no drifter people of the age '%s' within 12,000 ly\n", an.c_str()); return 1; }
        } else {
            list = drifterPeoplesNearHome();
            k = atoi(argv[3]);
            if (k < 0 || k >= (int)list.size()) { printf("no drifter people %d (of %zu within 3,000 ly of home)\n", k, list.size()); return 1; }
        }
        Star s = list[k]; StarSystem sys; sys.generate(s);
        int n = argc > 4 ? atoi(argv[4]) : 50; bool full = false; int held = 1;
        if (argc > 5) { if (std::string(argv[5]) == "full") full = true; else held = atoi(argv[5]); }
        if (argc > 6 && std::string(argv[6]) == "full") full = true;
        printf("the drifter people %d nearest home: %s, sector %lld %lld %lld\n", k, s.name.c_str(), (long long)s.sx, (long long)s.sy, (long long)s.sz);
        return shardsOfWorld(s, sys, 0, which, n, full, held, s.sx, s.sz);
    }
    if (argc >= 3 && std::string(argv[2]) == "drifters") {   // D-05: `shards drifters [n] [held]`: the test drifters' records (fifty each), read in full
        const int n = argc > 3 ? atoi(argv[3]) : 50; int rc = 0;
        const bool full = argc <= 4; const int held = argc > 4 ? atoi(argv[4]) : 1;   // a number of shards held: as decoded with them
        for (const AgeWorld& aw : TEST_DRIFTERS) {
            Star s; if (!placeInSector(aw.sx, aw.sy, aw.sz, s, true)) { printf("no drifter at %lld %lld %lld\n", (long long)aw.sx, (long long)aw.sy, (long long)aw.sz); rc = 1; continue; }
            StarSystem sys; sys.generate(s);
            printf("== the test drifter %s, sector %lld %lld %lld\n", s.name.c_str(), (long long)s.sx, (long long)s.sy, (long long)s.sz);
            rc |= shardsOfWorld(s, sys, aw.body, which, n, full, held, s.sx, s.sz);
        }
        return rc;
    }
    if (argc < 5) { printf("shards <sx> <sz> <body index> [n] [full | <held> [full]], or shards rogue <k> [n] [full | <held> [full]] [disk], or shards drifters [n] [held] (D-05)\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    int n = argc > 5 ? atoi(argv[5]) : 50; bool full = false; int held = 1;
    if (argc > 6) { if (std::string(argv[6]) == "full") full = true; else held = atoi(argv[6]); }
    if (argc > 7 && std::string(argv[7]) == "full") full = true;
    Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    return shardsOfWorld(s, sys, bi, which, n, full, held, sx, sz);
}
int shardsOfWorld(const Star& s, StarSystem& sys, int bi, int which, int n, bool full, int held, int64_t sx, int64_t sz) {
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) printf("(%s had no people: the shards below are what it would have said)\n", b.name.c_str());
    Lore L = loreOf(sys, b, g, which);
    printf("%s, %s of %s (%s): the %s; god %s, river %s, mountain %s, sea %s, towns %s and %s, founder %s, moon %s, star %s, festival %s; names of style %d; a day of %.0f min of play, a year of %.0f of its days, %d moons, %d years from the founding\n",
           b.name.c_str(), KINDS[b.type].name, s.name.c_str(), STAR_CLASSES[s.cls].name, L.people.c_str(), L.god.c_str(), L.river.c_str(), L.mountain.c_str(), L.sea.c_str(), L.city.c_str(), L.city2.c_str(),
           L.founder.c_str(), L.moon.c_str(), L.star.c_str(), L.festival.c_str(), L.style, L.dayHours * 60, L.yearDays, L.moons, L.spanYears);
    // C-06: the tongue and the language
    Tongue T = tongueOf(g, L);
    std::vector<Shard> fifty; shardsOf(sys, b, g, L, SHARDS_PER_WORLD, fifty);
    Language lang; languageOf(L, fifty, T, lang);
    auto join = [](const std::vector<std::string>& v) { std::string r; for (const std::string& x : v) { if (!r.empty()) r += " "; r += x; } return r; };
    printf("tongue: onsets %s; nuclei %s; codas %s; a coda %.0f%% of the time, a bare first vowel %.0f%%\n", join(T.onsets).c_str(), join(T.nuclei).c_str(), join(T.codas).c_str(), 100 * T.codaChance, 100 * T.vowelStart);
    { Voice V = voiceOf(g, L, T); printf("voice: %s; %.0f Hz, %.1f syllables a second (C-05: `voice %lld %lld %d`)\n", voiceLine(V).c_str(), V.pitch, V.rate, (long long)sx, (long long)sz, bi); }
    printf("language: %d words used %d times in the fifty, %zu names; known with 1/2/3/5/7/10 shards: %d/%d/%d/%d/%d/%d words (%.0f/%.0f/%.0f/%.0f/%.0f/%.0f%% of the uses)\n", (int)lang.words.size(), lang.tokens, lang.names.size(),
           languageKnown(lang, 1), languageKnown(lang, 2), languageKnown(lang, 3), languageKnown(lang, 5), languageKnown(lang, 7), languageKnown(lang, 10),
           100 * languageShare(1), 100 * languageShare(2), 100 * languageShare(3), 100 * languageShare(5), 100 * languageShare(7), 100 * languageShare(10));
    printf("  the most used:");
    for (int k = 0; k < (int)lang.words.size() && k < 16; k++) printf(" %s=%s(%d)", lang.words[k].c_str(), lang.theirs[k].c_str(), lang.counts[k]);
    printf("\n");
    std::vector<Shard> sh; if (n <= SHARDS_PER_WORLD) sh.assign(fifty.begin(), fifty.begin() + n); else shardsOf(sys, b, g, L, n, sh);
    std::vector<int> order(n); for (int i = 0; i < n; i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int c) { return sh[a].year < sh[c].year; });
    int tones[ST_TONE_COUNT] = {0, 0, 0, 0}; int words = 0, read = 0, pieces = 0;
    Tradition trad = traditionOf(g, L);   // C-04
    for (int i : order) {
        const Shard& x = sh[i];
        tones[x.tone]++;
        if (x.music) { Piece P; pieceOf(trad, x, P); pieces++; printf("%3d  year %3d  %-8s  %-14s [music] %s, %.0f s, %zu notes\n", i, x.year, SHARD_TONE_NAMES[x.tone], "music", PIECE_FORM_NAMES[P.form], P.seconds, P.notes.size()); continue; }
        words += shardWords(x.text);
        std::vector<DecodedWord> w; read += decodeShard(x, lang, T, held, w);
        printf("%3d  year %3d  %-8s  %-14s %s\n", i, x.year, SHARD_TONE_NAMES[x.tone], x.chart ? "star chart" : shardKindName(x.kind), decodedText(w).c_str());   // C-10: a chart by its caption (`charts` draws it)
        if (full) printf("%40s %s\n", "", x.text.c_str());
    }
    printf("tones: ordinary %d, elegy %d, warning %d, the end %d; %d pieces of music; %.0f words a text; %d of %d words read with %d shard%s held\n", tones[0], tones[1], tones[2], tones[3], pieces, n - pieces > 0 ? (double)words / (n - pieces) : 0.0, read, words, held, held == 1 ? "" : "s");
    return 0;
}

// C-05: `voice <sx> <sz> <body index> [index] [wav]`: a world's voice (its pitch and range, its rate, its tract, its qualities,
// its stress and melody, its marks) and the sounds its tongue uses, then one text shard (the given index, else the first text)
// as it is spoken: each word as the decoder gives it with the language whole, the people's word, its sounds and its span in
// seconds; `wav` renders it through the synth to shots/tests/voice_<sx>_<sz>_<body>_S<index>.wav
// C-10: `charts <sx> <sz> <body index> [index]`: the stars a world's charts mark (the peoples it knew, the brightest of its
// nights), then each chart: its figures with their stars (the chart's x y, the brightness, the class, the sector, the light
// years from the world) and its caption; with an index, that chart sketched (the field 61 x 31: the mark '@', a bright star
// '*', a faint one '.', a figure's label star its letter)
// H-05: `charts ages`: a chart of each way (the oldest's cave ceiling and bone, the medieval vault, astrolabe and page, the industrial atlas
// and photograph, the modern survey on glass, printed and on a drive, the futuristic navigation chart, the far future's) found among the age
// worlds' records (and the worlds near home where an age world has none), printed with what its way shows, and drawn on the decoder in a
// `Game` at 2x: shots/tests/chart_age_<way>.png; then one held up to the sky at the people's own star of each of three ways
struct ChartPick { bool ok = false; int64_t sx = 0, sz = 0; int body = 0, which = 0, index = 0; std::string world, people; StarChart ch; std::string caption; };
static void printChartWay(const StarSystem& sys, const StarChart& ch) {
    std::string w;
    if (ch.tallyKind) w += fmt(" the moon's count %d (%s);", ch.tally, ch.tallyKind == 1 ? "the days of its month" : "its moons in the year");
    if (!ch.curves.empty()) { int k[6] = {}; for (const ChartCurve& c : ch.curves) k[std::min(5, std::max(0, c.kind))]++; w += fmt(" curves: %d equator, %d tropic, %d ecliptic, %d declination, %d hour runs;", k[0], k[1], k[2], k[3], k[4]); }
    if (!ch.wanderers.empty()) { w += fmt(" %zu wanderers:", ch.wanderers.size()); for (const ChartPath& p : ch.wanderers) w += fmt(" %s (%zu points)", sys.bodies[p.body].name.c_str(), p.pts.size()); w += ";"; }
    int numbered = 0; for (const ChartStar& c : ch.stars) if (c.number) numbered++;
    if (numbered) w += fmt(" %d stars numbered, their circle %d degrees, their day %d hours;", numbered, ch.circle, ch.hours);
    if (ch.plate) w += fmt(" plate/field %d;", ch.plate);
    if (ch.style == CHS_SURVEY) w += fmt(" magnitudes in steps of %.1f to the %dth, %d peoples heard on it;", ch.magStep, ch.magLimit, ch.heardN);
    if (ch.style == CHS_ROUTES) w += fmt(" %d routes flown, %d meant;", ch.reached, ch.planned);
    if (ch.style == CHS_BEYOND) w += fmt(" the far mark of kind %d at %.0f ly, %d places where nothing is;", ch.farKind, ch.farLy, ch.signs);
    printf("   %zu stars, %zu figures:%s\n", ch.stars.size(), ch.figures.size(), w.c_str());
    for (const ChartFigure& f : ch.figures) printf("     %-26s %s%zu stars, glyph %d\n", f.name.c_str(), f.kind == 1 ? (f.planned ? "a route meant, " : "a route flown, ") : (f.kind == 2 ? "to nothing, " : ""), f.stars.size(), f.glyph);
}
static int runChartsAges() {
    makeDir("shots"); makeDir("shots/tests");
    // the ways to show: the eleven looks (a survey has three)
    struct Want { int style, medium; const char* file; } wants[] = {
        {CHS_CEILING, -1, "prehistoric_ceiling"}, {CHS_BONE, -1, "prehistoric_bone"}, {CHS_VAULT, -1, "medieval_vault"}, {CHS_ASTROLABE, -1, "medieval_astrolabe"},
        {CHS_PAGE, -1, "medieval_page"}, {CHS_ATLAS, -1, "industrial_atlas"}, {CHS_PLATE, -1, "industrial_plate"}, {CHS_SURVEY, RM_GLASS, "modern_survey_glass"},
        {CHS_SURVEY, RM_PRINTED, "modern_survey_printed"}, {CHS_SURVEY, RM_DRIVE, "modern_survey_drive"}, {CHS_ROUTES, -1, "futuristic_routes"}, {CHS_BEYOND, -1, "far_future"}};
    const int W = (int)(sizeof(wants) / sizeof(wants[0]));
    std::vector<ChartPick> picks(W);
    auto better = [&](const Want& w, const StarChart& c, const ChartPick& old) {   // the more of its way shown, the better
        if (!old.ok) return true;
        auto score = [&](const StarChart& x) { return (x.tallyKind == 1 ? (x.tally >= 12 ? 6 : 3) : (x.tallyKind ? 1 : 0)) + (int)std::min<size_t>(3, x.wanderers.size()) + std::min(2, x.heardN) + (int)std::min<size_t>(4, x.figures.size()) + (x.style == CHS_ROUTES ? x.reached + x.planned : 0) + (x.style == CHS_BEYOND ? x.signs : 0); };
        (void)w; return score(c) > score(old.ch);
    };
    std::vector<std::array<int64_t, 3>> worlds;
    for (int a = 0; a < AGE_COUNT; a++) worlds.push_back({AGE_WORLDS[a].sx, AGE_WORLDS[a].sz, AGE_WORLDS[a].body});
    for (int64_t x = 150; x <= 215; x++) for (int64_t z = 20; z <= 80; z += 1) worlds.push_back({x, z, -1});
    int scanned = 0;
    auto wantMore = [&](int k) { return !picks[k].ok || ((wants[k].style == CHS_CEILING || wants[k].style == CHS_BONE) && (picks[k].ch.tallyKind != 1 || picks[k].ch.tally < 12)); };   // the oldest with a moon's days counted
    for (const auto& wv : worlds) {
        bool all = true; for (int k = 0; k < W; k++) if (wantMore(k)) all = false;
        if ((all && scanned >= AGE_COUNT) || scanned > 400) break;
        Star s; StarSystem sys; if (!systemAt(wv[0], 0, wv[1], s, sys, true)) continue;
        for (const Body& b : sys.bodies) {
            if (wv[2] >= 0 && b.index != wv[2]) continue;
            if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
            BodyGen g = BodyGen::make(b); if (!g.hasTrait(TR_CIVILISATION)) continue;
            scanned++;
            for (int which = 0; which < peoplesOf(g); which++) {
                Lore L = loreOf(sys, b, g, which);
                std::shared_ptr<const RecordCatalogue> C = recordsOf(g, which);
                for (int i = 0; i < C->count; i++) {
                    if (C->entries[i].category != RC_CHART) continue;
                    const int style = chartStyleOf(C->entries[i].age, C->entries[i].medium);
                    for (int k = 0; k < W; k++) {
                        if (wants[k].style != style || (wants[k].medium >= 0 && wants[k].medium != C->entries[i].medium)) continue;
                        if (!wantMore(k) && scanned > AGE_COUNT) continue;   // past the age worlds, the first found
                        StarChart ch; if (!chartOf(sys, b, g, L, i, ch)) continue;
                        if (!better(wants[k], ch, picks[k])) continue;
                        ChartPick& p = picks[k]; p.ok = true; p.sx = wv[0]; p.sz = wv[1]; p.body = b.index; p.which = which; p.index = i; p.world = b.name; p.people = L.people; p.ch = ch;
                        p.caption = shardOf(sys, b, g, L, i).text;
                    }
                }
            }
        }
    }
    int drawn = 0;
    for (int k = 0; k < W; k++) {
        ChartPick& p = picks[k];
        if (!p.ok) { printf("%s: none found\n", wants[k].file); continue; }
        Star s; starInSector(p.sx, 0, p.sz, s, true); StarSystem sys; sys.generate(s);
        printf("%s: %s (%lld 0 %lld body %d%s), the %s, S%d: %s on %s, year %d\n", wants[k].file, p.world.c_str(), (long long)p.sx, (long long)p.sz, p.body, p.which ? " second" : "", p.people.c_str(), p.index,
               AGE_NAMES[p.ch.age], RECORD_MEDIUM_NAMES[p.ch.medium], shardDrawOf(BodyGen::make(sys.bodies[p.body]), loreQuick(sys, sys.bodies[p.body], BodyGen::make(sys.bodies[p.body]), p.which), p.index).year);
        printChartWay(sys, p.ch);
        printf("   caption: %s\n", p.caption.c_str());
        // the decoder: the people's full count held (its names read as ours), the chart opened by index, its reading skipped
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.settings.renderScale = 2; game.applySettings();
        game.testParkAt(s, p.body); game.setState(GameState::SPACE);
        Input in;
        auto run = [&](double secs) { runFrames(game, in, secs); };
        auto press = [&](int key) { pressKey(game, in, key); };
        const BodyGen g = BodyGen::make(sys.bodies[p.body]);
        const int full = languageFull(recordCountOf(g, p.which));
        const std::string base = Guide::bodyKey(p.sx, 0, p.sz, p.body) + "/S";
        for (int i = 0; i < full; i++) game.guide.shards.insert(base + std::to_string(shardWorldIndex(p.which, i)));
        game.guide.shards.insert(base + std::to_string(shardWorldIndex(p.which, p.index)));
        run(0.2); game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
        press(KEY_ENTER); run(0.2);
        const bool opened = game.testShardsOpenIndex(p.index); run(0.2);
        press(KEY_ENTER); run(0.3);   // the reading skipped: read
        const std::string fn = fmt("shots/tests/chart_age_%s.png", wants[k].file);
        writePNG(fn.c_str(), game.output(), FBW, FBH);
        printf("   %s -> %s\n", opened ? game.testChartInfo().c_str() : "NOT OPENED", fn.c_str());
        { const double t0 = nowSec(); run(1.0); printf("   the decoder's page: %.2f ms a frame at 2x (30 frames, the whole frame)\n", (nowSec() - t0) * 1e3 / 30); }
        drawn += opened ? 1 : 0;
        if (opened && (wants[k].style == CHS_ATLAS || wants[k].style == CHS_ROUTES || wants[k].style == CHS_BEYOND || wants[k].style == CHS_CEILING)) {   // held up to the sky at their star
            press(KEY_O); run(0.1); press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.2);
            game.testAimAtChartMark(std::min(1, (int)p.ch.figures.size() - 1)); run(0.3);
            const std::string fh = fmt("shots/tests/chart_age_%s_held.png", wants[k].file);
            writePNG(fh.c_str(), game.output(), FBW, FBH);
            printf("   held up: %s -> %s\n", game.testChartInfo().c_str(), fh.c_str());
        }
    }
    printf("%d of %d ways drawn on the decoder (%d worlds read)\n", drawn, W, scanned);
    return drawn == W ? 0 : 1;
}

// D-05: `charts rogues`: the drifters' peoples' charts of their years without the sun, an age at a time (the nearest people of each within
// 12,000 ly that drew them): the sun's measure (its dimming and its going: `shots/tests/chart_rogue_<age>_measure.png`) and the sky of the dark
// (from where the drifter was by then, their old sun marked: `_dark.png`), each printed and drawn on the decoder; the dark's held up to the sky
static int runChartsRogues() {
    makeDir("shots"); makeDir("shots/tests");
    struct Got { bool ok = false; Star d; int which = 0, index = -1; StarChart ch; std::string caption; };
    Got measure[AGE_COUNT], dark[AGE_COUNT];
    const std::vector<Star> list = drifterPeoplesNearHome(12000);
    for (const Star& d : list) {
        bool all = true; for (int a = 0; a < AGE_COUNT; a++) if (!measure[a].ok || !dark[a].ok) all = false;
        if (all) break;
        StarSystem sys; sys.generate(d);
        const Body& b = sys.bodies[0]; const BodyGen g = BodyGen::make(b);
        for (int which = 0; which < peoplesOf(g); which++) {
            const int age = peopleAgeOf(g, which).age;
            if (measure[age].ok && dark[age].ok) continue;
            const Lore L = loreOf(sys, b, g, which);
            std::shared_ptr<const RecordCatalogue> C = recordsOf(g, which);
            for (int i = 0; i < C->count; i++) {
                if (C->entries[i].category != RC_CHART) continue;
                StarChart ch; if (!chartOf(sys, b, g, L, i, ch) || ch.mark.how != 4) continue;
                Got& slot = ch.style == CHS_SUNMEASURE ? measure[age] : dark[age];
                if (slot.ok) continue;
                slot.ok = true; slot.d = d; slot.which = which; slot.index = i; slot.ch = ch; slot.caption = shardOf(sys, b, g, L, i).text;
            }
        }
    }
    int drawn = 0, want = 0;
    static const char* const AGE_FILE[AGE_COUNT] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "far_future"};
    for (int a = 0; a < AGE_COUNT; a++) for (int kind = 0; kind < 2; kind++) {
        Got& p = kind == 0 ? measure[a] : dark[a];
        const std::string file = fmt("chart_rogue_%s_%s", AGE_FILE[a], kind == 0 ? "measure" : "dark");
        want++;
        if (!p.ok) { printf("%s: none found\n", file.c_str()); continue; }
        StarSystem sys; sys.generate(p.d);
        const BodyGen g = BodyGen::make(sys.bodies[0]);
        printf("%s: %s (sector %lld %lld %lld%s), the %s, S%d: %s on %s (%s), year %d; marks %s (how %d) %.1f ly off\n", file.c_str(), p.d.name.c_str(), (long long)p.d.sx, (long long)p.d.sy, (long long)p.d.sz,
               p.which ? ", second people" : "", loreQuick(sys, sys.bodies[0], g, p.which).people.c_str(), p.index, AGE_NAMES[p.ch.age], RECORD_MEDIUM_NAMES[p.ch.medium], CHART_STYLE_NAMES[p.ch.style],
               shardDrawOf(g, loreQuick(sys, sys.bodies[0], g, p.which), p.index).year, p.ch.mark.star.name.c_str(), p.ch.mark.how, p.ch.mark.ly);
        if (kind == 0) { printf("   the disc, year by year:"); for (const auto& v : p.ch.sunCurve) printf(" %d:%.1f", (int)v[0], v[1]); printf(" (in %d parts)\n", p.ch.sunUnits); }
        else { const PeoplesWorld PW = peoplesWorldOf(sys, sys.bodies[0]); printChartWay(*PW.sys, p.ch); }   // E-02: the wanderers are its former system's
        printf("   caption: %s\n", p.caption.c_str());
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.settings.renderScale = 2; game.applySettings();
        game.testParkAt(p.d, 0); game.setState(GameState::SPACE);
        Input in;
        auto run = [&](double secs) { runFrames(game, in, secs); };
        auto press = [&](int key) { pressKey(game, in, key); };
        const int full = languageFull(recordCountOf(g, p.which));
        const std::string base = Guide::bodyKey(p.d.sx, p.d.sy, p.d.sz, 0) + "/S";
        for (int i = 0; i < full; i++) game.guide.shards.insert(base + std::to_string(shardWorldIndex(p.which, i)));
        game.guide.shards.insert(base + std::to_string(shardWorldIndex(p.which, p.index)));
        run(0.2); game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
        press(KEY_ENTER); run(0.2);
        const bool opened = game.testShardsOpenIndex(p.index); run(0.2);
        press(KEY_ENTER); run(0.3);   // the reading skipped: read
        writePNG(("shots/tests/" + file + ".png").c_str(), game.output(), FBW, FBH);
        printf("   %s -> shots/tests/%s.png\n", opened ? game.testChartInfo().c_str() : "NOT OPENED", file.c_str());
        drawn += opened ? 1 : 0;
        if (opened && kind == 1) {   // held up to the sky at the drifter: the old sun's diamond among the stars
            press(KEY_O); run(0.1); press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.2);
            game.testAimAtChartMark(-1); run(0.3);
            writePNG(("shots/tests/" + file + "_held.png").c_str(), game.output(), FBW, FBH);
            printf("   held up: %s -> shots/tests/%s_held.png\n", game.testChartInfo().c_str(), file.c_str());
        }
    }
    printf("%d of %d drawn on the decoder\n", drawn, want);
    return drawn == want ? 0 : 1;
}

int runCharts(int argc, char** argv) {
    if (argc > 2 && std::string(argv[2]) == "ages") return runChartsAges();   // H-05
    if (argc > 2 && std::string(argv[2]) == "rogues") return runChartsRogues();   // D-05
    if (argc > 2 && std::string(argv[2]) == "check") { std::string d; bool ok = checkChartsByAge(d); printf("star charts by age: %s\n  %s\n", ok ? "ok" : "FAIL", d.c_str()); return ok ? 0 : 1; }   // H-05: the unit's check alone
    int which = takePeopleArg(argc, argv);   // C-13: `second` for the world's second people
    if (argc < 5) { printf("charts <sx> <sz> <body index> [index]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]); int want = argc > 5 ? atoi(argv[5]) : -1;
    Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi]; BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) printf("(%s had no people: the charts below are what it would have drawn)\n", b.name.c_str());
    double t0 = nowSec(); Lore L = loreOf(sys, b, g, which); double ms = (nowSec() - t0) * 1e3;
    static const char* const HOW[4] = {"the brightest of their nights", "a people's world among their bright stars", "a people they heard", "far off (H-05)"};
    printf("%s, %s of %s: the %s drew %d charts (%d of C-02's fifty; the lore in %.1f ms); they mark:\n", b.name.c_str(), KINDS[b.type].name, s.name.c_str(), L.people.c_str(), (int)L.marks.size(), chartShardsOf(peopleGen(g, which)), ms);
    for (size_t j = 0; j < L.marks.size(); j++) { const ChartMark& m = L.marks[j]; printf("  %zu: %s %s at %lld %lld %lld, %.1f ly: %s%s\n", j, STAR_CLASSES[m.star.cls].code, STAR_CLASSES[m.star.cls].name, (long long)m.star.sx, (long long)m.star.sy, (long long)m.star.sz, m.ly, HOW[m.how], m.people.empty() ? "" : fmt(" (body %d, the %s)", m.body, m.people.c_str()).c_str()); }
    std::vector<Shard> fifty; shardsOf(sys, b, g, L, L.records, fifty);   // H-05: every chart of the people's records
    int byStyle[CHS_COUNT] = {};
    for (int i = 0; i < (int)fifty.size(); i++) {
        if (!fifty[i].chart) continue;
        StarChart ch; if (!chartOf(sys, b, g, L, i, ch)) { printf("S%d: no chart\n", i); continue; }
        byStyle[ch.style]++;
        printf("S%d (chart %d, year %d, %s; %s on %s: %s): %zu stars, the mark %s at %lld %lld %lld (how %d, %.1f ly), %zu figures\n", i, ch.which, fifty[i].year, SHARD_TONE_NAMES[fifty[i].tone], AGE_NAMES[ch.age], RECORD_MEDIUM_NAMES[ch.medium],
               CHART_STYLE_NAMES[ch.style], ch.stars.size(), STAR_CLASSES[ch.mark.star.cls].code, (long long)ch.mark.star.sx, (long long)ch.mark.star.sy, (long long)ch.mark.star.sz, ch.mark.how, ch.mark.ly, ch.figures.size());
        {   // H-05: what its way shows
            std::string w;
            if (ch.tallyKind) w += fmt(" the moon's count %d (%s);", ch.tally, ch.tallyKind == 1 ? "the days of its month" : "its moons in the year");
            if (!ch.curves.empty()) { int k[6] = {}; for (const ChartCurve& c : ch.curves) k[std::min(5, std::max(0, c.kind))]++; w += fmt(" curves: %d equator, %d tropic, %d ecliptic, %d declination, %d hour runs;", k[0], k[1], k[2], k[3], k[4]); }
            if (!ch.wanderers.empty()) { w += fmt(" %zu wanderers (", ch.wanderers.size()); for (const ChartPath& p : ch.wanderers) w += fmt(" %s %zu points", sys.bodies[p.body].name.c_str(), p.pts.size()); w += " );"; }
            int numbered = 0; for (const ChartStar& c : ch.stars) if (c.number) numbered++;
            if (numbered) w += fmt(" %d stars numbered; their circle %d, their day %d hours;", numbered, ch.circle, ch.hours);
            if (ch.plate) w += fmt(" plate/field %d;", ch.plate);
            if (ch.style == CHS_SURVEY) w += fmt(" magnitudes in steps of %.1f to %d, %d peoples heard on it;", ch.magStep, ch.magLimit, ch.heardN);
            if (ch.style == CHS_ROUTES) w += fmt(" %d routes flown, %d meant;", ch.reached, ch.planned);
            if (ch.style == CHS_BEYOND) w += fmt(" the far mark of kind %d at %.0f ly, %d places where nothing is;", ch.farKind, ch.farLy, ch.signs);
            if (!w.empty()) printf(" %s\n", w.c_str());
        }
        for (const ChartFigure& f : ch.figures) {
            printf("  %-24s %s%zu stars (glyph %d):", f.name.c_str(), f.kind == 1 ? (f.planned ? "a route meant, " : "a route flown, ") : (f.kind == 2 ? "nothing, " : ""), f.stars.size(), f.glyph);
            for (int k : f.stars) { const ChartStar& c = ch.stars[k]; printf(" (%+.2f %+.2f b%.2f %s %lld %lld %lld %.1fly%s)", c.x, c.y, c.bright, c.sign ? "--" : STAR_CLASSES[c.star.cls].code, (long long)c.star.sx, (long long)c.star.sy, (long long)c.star.sz, c.ly, c.home ? " home" : ""); }
            printf("\n");
        }
        printf("  caption: %s\n", fifty[i].text.c_str());
        if (want == i) {   // the sketch
            const int W = 61, H = 31; std::vector<std::string> rows(H, std::string(W, ' '));
            for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { double u = (x - 30) / 30.0, v = (y - 15) / 15.0; if (u * u + v * v > 1.0 && u * u + v * v < 1.12) rows[y][x] = ':'; }
            auto put = [&](double cx, double cy, char c) { int x = (int)std::lround(30 + cx * 30), y = (int)std::lround(15 - cy * 15); if (x >= 0 && y >= 0 && x < W && y < H) rows[y][x] = c; };
            for (const ChartStar& c : ch.stars) put(c.x, c.y, c.bright > 0.6 ? '*' : '.');
            for (size_t fi = 0; fi < ch.figures.size(); fi++) { const ChartStar& c = ch.stars[ch.figures[fi].label]; put(c.x, c.y, (char)('A' + fi)); }
            put(ch.stars[ch.markStar].x, ch.stars[ch.markStar].y, '@');
            for (const std::string& r : rows) printf("    %s\n", r.c_str());
            for (size_t fi = 0; fi < ch.figures.size(); fi++) printf("    %c %s\n", (char)('A' + fi), ch.figures[fi].name.c_str());
        }
    }
    printf("the charts by their way:"); for (int k = 0; k < CHS_COUNT; k++) if (byStyle[k]) printf(" %s %d;", CHART_STYLE_NAMES[k], byStyle[k]); printf("\n");
    return 0;
}

// A-03: `ruins ages [scale] [town|village|hamlet|monument] [desert|felisian] [wide|near] [years ...]`: a settlement of each age (the rows)
// at ages since its end (the columns: 100, 1,000, 10,000 and 100,000 years unless given), each from outside its edge three quarters along
// its main street (`near`: from inside, a few metres from its first dwellings), in one sheet (`shots/tests/ruins_ages_<class>[_near]_<scale>x.png`);
// the far future's is its one structure
static int runRuinsAges(int argc, char** argv) {
    const int scale = argc > 3 ? std::max(1, atoi(argv[3])) : 1;
    const std::string cls = argc > 4 ? argv[4] : "town", world = argc > 5 ? argv[5] : "desert";
    const bool nearView = argc > 6 && std::string(argv[6]) == "near";
    std::vector<double> years;
    for (int i = 7; i < argc; i++) years.push_back(atof(argv[i]));
    if (years.empty()) years = {100, 1000, 10000, 100000};
    const int sclass = cls == "hamlet" ? SC_HAMLET : (cls == "village" ? SC_VILLAGE : (cls == "monument" ? SC_MONUMENT : SC_TOWN));
    mkdir("shots/tests", 0755);
    setFramebufferScale(scale);
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    const int type = world == "felisian" ? PT_FELISIAN : PT_DESERT;
    if (!setupSceneForType(type, 15, 40 * DEG, 1.3, 0.0, sv, sys, nb, t, SCENE_ANY)) { printf("no body\n"); return 1; }
    sv.flocks.clear(); sv.critters.clear();
    sv.relocateCapsule(-4000, -4000);
    const BodyGen& g = sv.site.gen;
    int gLat, gLon; sv.ruinCellAt(0, 0, gLat, gLon);
    printf("ruins ages: a %s of each age at %zu ages since the end on %s (%s), %dx\n", SETTLEMENT_CLASS_NAMES[sclass], years.size(), sys.bodies[sv.site.body].name.c_str(), KINDS[sys.bodies[sv.site.body].type].name, scale);
    const int cols = (int)years.size(), W = cols * FBW, H = AGE_COUNT * (FBH + 12 * scale) + 14 * scale;
    std::vector<uint32_t> sheet((size_t)W * H, rgb(16, 16, 20)), rgbBuf((size_t)FBW * FBH);
    RGBCanvas cv{sheet.data(), W, H, scale};
    for (int c = 0; c < cols; c++) drawText(cv, c * FBW / scale + 4, 3, fmt("%.0f YEARS SINCE THE END", years[c]).c_str(), rgb(200, 200, 160));
    Framebuffer fb;
    for (int a = 0; a < AGE_COUNT; a++) {
        for (int c = 0; c < cols; c++) {
            Culture cu = cultureAs(g, 0, a, years[c]);
            RuinSpec sp; settlementOf(g, gLat, gLon, cu, a == AGE_FAR_FUTURE ? SC_MONUMENT : sclass, sp);
            std::vector<Form> pieces[3];
            for (int l = 0; l < 3; l++) ruinElements(sp, cu, l, pieces[l]);
            double hMax = 0, reach = sp.size;
            for (const Form& e : pieces[0]) { hMax = std::max(hMax, formTop(e)); reach = std::max(reach, std::hypot(e.x, e.z) * 0.8); }
            sv.testClearPlaced();
            sv.cultures[0] = cu; sv.palettes[0] = cu.palette;
            sv.testPlaceSettlement(0, 0, sp, pieces);
            // from outside its edge along the main street (the spec's heading), raised to see over the walls
            const double h0 = sp.heading + 0.6;   // three quarters along the main street
            if (nearView && a != AGE_FAR_FUTURE) {   // inside: 6 m from the nearest dwelling to a point halfway out, looking at it
                const Building* nb0 = nullptr; double best = 1e18, px = std::sin(h0) * sp.size * 0.5, pz = std::cos(h0) * sp.size * 0.5;
                for (const Building& b : sp.buildings) {
                    if (b.kind == BK_WALL || b.kind == BK_PALISADE || b.kind == BK_WALL_TOWER || b.kind == BK_GATE || b.kind == BK_WINDWALL || b.kind == BK_MAGLEV) continue;
                    double d = std::hypot(b.x - px, b.z - pz); if (d < best) { best = d; nb0 = &b; }
                }
                if (nb0) {
                    double rr = std::max(nb0->hw, nb0->hd), dd = rr + 6 + 0.04 * hMax, ax = nb0->x - std::sin(h0) * dd, az = nb0->z - std::cos(h0) * dd;
                    ax = nb0->x + (px - nb0->x) / std::max(1.0, best) * dd; az = nb0->z + (pz - nb0->z) / std::max(1.0, best) * dd;
                    sv.player.x = ax; sv.player.z = az;
                    formsCamera(sv, 2.2 + 0.05 * rr, nb0->x, sv.site.surfaceHeight(nb0->x, nb0->z) + std::min(nb0->hWall * 0.4, 4.0), nb0->z, t);
                }
            } else {
                const double D = a == AGE_FAR_FUTURE ? std::max(reach * 2.4, hMax * 1.7) : std::max(reach * 1.3 + 30, hMax * 0.62), eye = 2 + D * 0.12;
                sv.player.x = std::sin(h0) * D; sv.player.z = std::cos(h0) * D;
                formsCamera(sv, eye, 0, sv.site.surfaceHeight(0, 0) + std::min(hMax * 0.3, 0.35 * D), 0, t);
            }
            sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(rgbBuf.data(), 1.0);
            const int dx0 = c * FBW, dy0 = 14 * scale + a * (FBH + 12 * scale);
            for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(dy0 + y) * W + dx0 + x] = rgbBuf[(size_t)y * FBW + x];
            std::map<std::string, int> kinds; for (const Building& b : sp.buildings) kinds[BUILDING_KIND_NAMES[b.kind]]++;
            std::string ks; for (auto& k : kinds) ks += fmt("%s%d %s", ks.empty() ? "" : ", ", k.second, k.first.c_str());
            printf("  %-11s %6.0f y: %s r %.0f m, %zu buildings (%s); %zu pieces, the tallest %.0f m; decay timber %.2f, dry stone %.2f, masonry %.2f, iron %.2f, concrete %.2f, composite %.2f; %d polygons\n",
                   AGE_NAMES[a], years[c], SETTLEMENT_CLASS_NAMES[sp.sclass], sp.size, sp.buildings.size(), ks.c_str(), pieces[0].size(), hMax,
                   materialDecay(MT_TIMBER, years[c], cu.wet), materialDecay(MT_DRYSTONE, years[c], cu.wet), materialDecay(MT_MASONRY, years[c], cu.wet), materialDecay(MT_IRON, years[c], cu.wet),
                   materialDecay(MT_CONCRETE, years[c], cu.wet), materialDecay(MT_COMPOSITE, years[c], cu.wet), sv.lastFormPolys);
        }
        drawTextShadow(cv, 4, 14 + a * (FBH / scale + 12) + FBH / scale + 1, fmt("%s", AGE_NAMES[a]).c_str(), rgb(230, 235, 240), rgb(0, 0, 0));
    }
    std::string fn = fmt("shots/tests/ruins_ages_%s%s_%dx.png", cls.c_str(), nearView ? "_near" : "", scale);
    writePNG(fn.c_str(), sheet.data(), W, H);
    printf("  -> %s\n", fn.c_str());
    return 0;
}

// A-03: `ruins kinds [scale] [years ...]`: every kind of building, a page an age (`shots/tests/ruins_kinds_<age>_<scale>x.png`), each kind a
// row and each age since the end a column (1, 300, 3,000 and 30,000 years unless given), alone on the scene world's ground and seen
// from a quarter turn off its front; the works every age built its own way on the page of the age that builds them most (a gate on
// the medieval page as a gatehouse)
static int runRuinsKinds(int argc, char** argv) {
    const int scale = argc > 3 ? std::max(1, atoi(argv[3])) : 1;
    std::vector<double> years;
    for (int i = 4; i < argc; i++) years.push_back(atof(argv[i]));
    if (years.empty()) years = {1, 300, 3000, 30000};
    mkdir("shots/tests", 0755);
    setFramebufferScale(scale);
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    if (!setupSceneForType(PT_DESERT, 15, 40 * DEG, 1.3, 0.0, sv, sys, nb, t, SCENE_ANY)) { printf("no body\n"); return 1; }
    sv.flocks.clear(); sv.critters.clear(); sv.relocateCapsule(-4000, -4000);
    const BodyGen& g = sv.site.gen;
    struct K { int kind, age; double hw, hd, h; int style; const char* name; };
    static const K KINDS[] = {
        {BK_HUT, 0, 3, 3, 1.7, 0, "HUT, WATTLE"}, {BK_HUT, 0, 3, 3, 1.7, 1, "HUT, DRY STONE"}, {BK_LONGHOUSE, 0, 3, 9, 1.9, 0, "LONGHOUSE"}, {BK_TENT_RING, 0, 2.5, 2.5, 2.8, 0, "TENT RING"},
        {BK_STONE_CIRCLE, 0, 12, 12, 3.6, 0, "STONE CIRCLE"}, {BK_MENHIR, 0, 1, 10, 3.2, 0, "STANDING STONES"}, {BK_DOLMEN, 0, 1.4, 2, 2, 0, "DOLMEN"}, {BK_PALISADE, 0, 12, 0.45, 2.8, 0, "PALISADE"},
        {BK_HOUSE, 1, 3, 4, 3, 0, "HOUSE"}, {BK_HALL, 1, 5.5, 8, 6, 0, "HALL"}, {BK_TOWER, 1, 3, 3, 13, 0, "KEEP"}, {BK_ROTUNDA, 1, 4.5, 4.5, 3.5, 0, "ROTUNDA"},
        {BK_TEMPLE, 1, 7, 16, 12, 0, "TEMPLE, BASILICA"}, {BK_TEMPLE, 1, 8, 16, 11, 1, "TEMPLE, DOMED"}, {BK_TEMPLE, 1, 9, 17, 11, 2, "TEMPLE, STEPPED"},
        {BK_GATE, 1, 1.9, 0.6, 6.5, 0, "GATEHOUSE"}, {BK_WALL_TOWER, 1, 3, 3, 8.5, 0, "WALL TOWER"}, {BK_PLATFORM, 1, 6, 6, 3.3, 0, "PLATFORM"},
        {BK_ROW, 2, 12, 4.7, 8, 0, "TERRACED ROW"}, {BK_TENEMENT, 2, 7, 7, 16, 0, "TENEMENT"}, {BK_VILLA, 2, 5.5, 5, 7, 0, "VILLA"}, {BK_CHURCH, 2, 7.5, 16, 13, 0, "CHURCH"},
        {BK_TOWN_HALL, 2, 16, 10, 16, 0, "TOWN HALL"}, {BK_STATION, 2, 16, 29, 12, 0, "STATION"}, {BK_MILL, 2, 8.5, 17, 18, 0, "MILL"}, {BK_GASOMETER, 2, 16, 16, 22, 0, "GASOMETER"},
        {BK_WATER_TOWER, 2, 5.5, 5.5, 20, 0, "WATER TOWER"}, {BK_HEADFRAME, 2, 25, 20, 25, 0, "HEADFRAME"}, {BK_OBSERVATORY, 2, 7.5, 7.5, 7, 0, "OBSERVATORY"},
        {BK_BLOCK, 3, 13, 7, 30, 0, "BLOCK"}, {BK_SUBURB, 3, 4.5, 5.5, 4, 0, "SUBURB"}, {BK_HIGHRISE, 3, 14, 12, 250, 0, "TOWER, SLAB"}, {BK_HIGHRISE, 3, 14, 12, 250, 1, "TOWER, SETBACKS"},
        {BK_HIGHRISE, 3, 13, 13, 250, 2, "TOWER, CYLINDER"}, {BK_STADIUM, 3, 55, 55, 25, 0, "STADIUM"}, {BK_MAST, 3, 3, 3, 200, 0, "MAST"}, {BK_POWER, 3, 85, 80, 1, 0, "POWER STATION"}, {BK_TANKS, 3, 40, 17, 27, 0, "TANKS"},
        {BK_ARCOLOGY, 4, 120, 120, 550, 0, "ARCOLOGY, STEPPED"}, {BK_ARCOLOGY, 4, 120, 120, 650, 1, "ARCOLOGY, CONE"}, {BK_ARCOLOGY, 4, 110, 110, 500, 2, "ARCOLOGY, GROWN"},
        {BK_HABITAT, 4, 55, 55, 33, 0, "HABITAT"}, {BK_SPIRE, 4, 20, 20, 700, 0, "SPIRE"}, {BK_RING, 4, 100, 6, 200, 0, "RING"}, {BK_MAGLEV, 4, 3, 140, 23, 0, "MAGLEV"},
        {BK_WRECK, 4, 18, 100, 30, 0, "WRECK"}, {BK_POD, 4, 10, 10, 3.4, 0, "POD"},
        {BK_SPIKE, 5, 80, 80, 1500, 0, "SPIKE"}, {BK_HALO, 5, 300, 6, 600, 0, "HALO"}, {BK_PILLARS, 5, 15, 400, 400, 0, "PILLARS"}, {BK_SHAFT, 5, 90, 90, 2, 0, "SHAFT"}, {BK_MONOLITH, 5, 150, 150, 300, 0, "MONOLITH"}};
    // A-04: the genome's works and the kinds' new forms, a page of their own (each in its age's way: `ruins_kinds_genome_<scale>x.png`)
    static const K WORKS[] = {
        {BK_FOCUS, 1, 3, 3, 40, FO_SPIRE, "FOCUS, SPIRE"}, {BK_FOCUS, 1, 8, 8, 13, FO_DOME, "FOCUS, DOME"}, {BK_FOCUS, 1, 4, 4, 12, FO_GATE, "FOCUS, GATE"}, {BK_FOCUS, 1, 3, 3, 25, FO_TOWER, "FOCUS, TOWER"},
        {BK_FOCUS, 1, 2.5, 2.5, 18, FO_COLOSSUS, "FOCUS, COLOSSUS"}, {BK_FOCUS, 2, 2.5, 2.5, 40, FO_MAST, "FOCUS, MAST (INDUSTRIAL)"}, {BK_FOCUS, 1, 6, 6, 12, FO_RING, "FOCUS, RING"},
        {BK_FOCUS, 1, 1.8, 1.8, 20, FO_OBELISK, "FOCUS, OBELISK"}, {BK_FOCUS, 1, 8, 6, 0.8, FO_POOL, "FOCUS, POOL"}, {BK_FOCUS, 1, 9, 9, 20, FO_TREE, "FOCUS, GREAT TREE"},
        {BK_STELA, 1, 0.45, 0.25, 4, MN_STELAE, "MONUMENT, STELA"}, {BK_STELA, 1, 0.6, 0.6, 5, MN_OBELISKS, "MONUMENT, OBELISK"}, {BK_STELA, 1, 0.9, 0.9, 6, MN_COLOSSI, "MONUMENT, STATUE"},
        {BK_STELA, 1, 2.6, 0.8, 6, MN_ARCHES, "MONUMENT, ARCH"}, {BK_STELA, 1, 0.7, 0.7, 5, MN_PILLARS, "MONUMENT, PILLAR"},
        {BK_WINDMILL, 1, 4, 4, 14, 0, "WINDMILL"}, {BK_HALL, 1, 6, 11, 6, 1, "MARKET HALL"}, {BK_HALL, 1, 12, 12, 7, 2, "PALACE"}, {BK_TEMPLE, 1, 7, 7, 11, 3, "TEMPLE, ROUND"},
        {BK_STADIUM, 1, 16, 16, 7, 0, "AMPHITHEATRE"}, {BK_AQUEDUCT, 1, 1.4, 40, 14, 0, "AQUEDUCT"}, {BK_TWIN, 1, 10, 3, 18, 0, "TWIN TOWERS"}, {BK_PORTAL, 1, 2.4, 3.5, 4.8, 0, "TUNNEL"},
        {BK_TOWN_HALL, 2, 16, 11, 16, 1, "TOWN HALL, DOME"}, {BK_TOWN_HALL, 2, 16, 11, 16, 2, "TOWN HALL, SPIRE"}, {BK_STATION, 2, 14, 22, 11, 1, "MARKET HALL (INDUSTRIAL)"},
        {BK_BLOCK, 3, 32, 20, 12, 1, "MALL"}, {BK_TEMPLE, 3, 10, 20, 18, 0, "TEMPLE (MODERN)"}, {BK_TWIN, 3, 35, 11, 160, 0, "TWIN TOWERS (MODERN)"},
        // D-05: a people that lost its sun's domes over the warm ground and its glasshouses
        {BK_DOME, 2, 30, 30, 2.6, 0, "DOME (INDUSTRIAL)"}, {BK_DOME, 3, 32, 32, 2.6, 1, "DOME (MODERN)"}, {BK_DOME, 5, 34, 34, 2.6, 2, "DOME (FAR FUTURE)"},
        {BK_GREENHOUSE, 2, 4, 12, 4, 0, "GLASSHOUSE (INDUSTRIAL)"}, {BK_GREENHOUSE, 3, 4.5, 18, 4.5, 1, "GLASSHOUSE (MODERN)"}, {BK_GREENHOUSE, 4, 6, 20, 5.4, 2, "GLASSHOUSE (FUTURISTIC)"}};
    const int cols = (int)years.size(), CW = FBW / 2, CH = FBH / 2;
    std::vector<uint32_t> rgbBuf((size_t)FBW * FBH);
    Framebuffer fb;
    for (int page = 0; page <= AGE_COUNT; page++) {
        const bool works = page == AGE_COUNT;
        std::vector<const K*> list;
        if (works) for (const K& k : WORKS) list.push_back(&k);
        else for (const K& k : KINDS) if (k.age == page) list.push_back(&k);
        const int rows = (int)list.size(), W = cols * CW, H = rows * (CH + 10 * scale) + 14 * scale;
        std::vector<uint32_t> sheet((size_t)W * H, rgb(16, 16, 20));
        RGBCanvas cv{sheet.data(), W, H, scale};
        for (int c = 0; c < cols; c++) drawText(cv, c * CW / scale + 3, 3, fmt("%.0f YEARS", years[c]).c_str(), rgb(200, 200, 160));
        for (int r = 0; r < rows; r++) {
            const K& k = *list[r];
            const int a = k.age;
            for (int c = 0; c < cols; c++) {
                Culture cu = cultureAs(g, 0, a, years[c]);
                if (!works) { cu.temple = k.style; cu.towerForm = k.style; cu.hut = k.kind == BK_HUT ? k.style : cu.hut; cu.great = k.kind == BK_ARCOLOGY ? k.style : cu.great; }
                cu.palette = culturePalette(cu, peopleGen(g, 0).seed);
                RuinSpec sp; sp.sclass = SC_MONUMENT; sp.heading = 0; sp.seed = 0xA03ULL + r;
                Building b; b.kind = k.kind; b.age = a; b.style = k.style; b.hw = k.hw; b.hd = k.hd; b.hWall = k.h; b.thick = k.kind == BK_TEMPLE || k.kind == BK_CHURCH ? 1.0 : 0.5; b.seed = mix64(0xA03B1DULL + r * 7919);
                sp.buildings.push_back(b);
                std::vector<Form> pieces[3]; for (int l = 0; l < 3; l++) ruinElements(sp, cu, l, pieces[l]);
                double hMax = 0, reach = std::max(k.hw, k.hd); for (const Form& e : pieces[0]) { hMax = std::max(hMax, formTop(e)); reach = std::max(reach, std::hypot(e.x, e.z) + std::min(formReach(e), 30.0)); }
                sp.size = reach;
                sv.testClearPlaced(); sv.cultures[0] = cu; sv.palettes[0] = cu.palette;
                sv.testPlaceSettlement(0, 0, sp, pieces);
                const double D = std::max(reach * 2.6 + 10, hMax * 1.9), ah = 0.5;   // from a quarter turn off its front, all of it in the half frame kept
                sv.player.x = std::sin(ah) * D; sv.player.z = std::cos(ah) * D;
                formsCamera(sv, 1.7 + D * 0.22, 0, sv.site.surfaceHeight(0, 0) + std::min(hMax * 0.42, D * 0.4), 0, t);
                sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(rgbBuf.data(), 1.0);
                const int dx0 = c * CW, dy0 = 14 * scale + r * (CH + 10 * scale), ox = FBW / 4, oy = FBH / 4;
                for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) sheet[(size_t)(dy0 + y) * W + dx0 + x] = rgbBuf[(size_t)(oy + y) * FBW + ox + x];
            }
            drawTextShadow(cv, 3, 14 + r * (CH / scale + 10) + CH / scale + 1, k.name, rgb(230, 235, 240), rgb(0, 0, 0));
        }
        std::string fn = fmt("shots/tests/ruins_kinds_%s_%dx.png", works ? "genome" : (page == AGE_FAR_FUTURE ? "far_future" : AGE_NAMES[page]), scale);
        writePNG(fn.c_str(), sheet.data(), W, H);
        printf("  %s: %d kinds -> %s\n", works ? "the genome's works" : AGE_NAMES[page], rows, fn.c_str());
    }
    return 0;
}

int runRuins(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[2]) == "ages") return runRuinsAges(argc, argv);   // A-03
    if (argc >= 3 && std::string(argv[2]) == "kinds") return runRuinsKinds(argc, argv);   // A-03
    if (argc >= 3 && std::string(argv[2]) == "check") {   // A-03: the unit's checks of the ages' buildings alone
        int fails = 0;
        agesChecks(Check{nullptr, &fails});
        return fails ? 2 : 0;
    }
    if (argc < 5) { printf("ruins <sx> <sz> <body index> [latDeg lonDeg]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    double lat = argc > 6 ? atof(argv[5]) * DEG : 0.2, lon = argc > 6 ? atof(argv[6]) * DEG : 0.7;
    Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    int np = std::max(1, peoplesOf(g)); Culture cs[2] = {cultureOf(g, 0), np > 1 ? cultureOf(g, 1) : Culture()}; const Culture& c = cs[0];   // C-13
    printf("%s, %s, R %.0f km: %s; culture style %d, family %d, tall %.2f, decay %.2f (the end %.0f years ago, wet %.2f), %s roofs, towns' plan %d, towers %d, buried %.1f m; the sea stood at %.0f m\n",
           b.name.c_str(), KINDS[b.type].name, b.radiusKm, g.hasTrait(TR_CIVILISATION) ? "a people lived here" : "no civilisation", c.style, c.family, c.tall, c.decay, c.ageYears, c.wet,
           c.stoneRoofs ? "stone" : "no", c.plan, (int)c.towers, c.buried, g.oldSeaM > -1e8 ? g.oldSeaM : 0.0);
    if (np > 1) printf("  a second people (C-13): culture style %d, family %d, tall %.2f, decay %.2f (the end %.0f years ago, wet %.2f), %s roofs, towns' plan %d, towers %d, buried %.1f m; %s\n", cs[1].style, cs[1].family, cs[1].tall, cs[1].decay, cs[1].ageYears, cs[1].wet,
                       cs[1].stoneRoofs ? "stone" : "no", cs[1].plan, (int)cs[1].towers, cs[1].buried, peoplesEndedTogether(g) ? "the two ended together" : "the two ended apart");
    int gLat0, gLon0; ruinCellOf(g, lat, lon, gLat0, gLon0);
    int cnt[7] = {}, ok[7] = {}, printed = 0, shardsN[7] = {};   // by the class (a city's and the capital's cells too)
    int harbours = 0, dryHarbours = 0, terraced = 0, cisterns = 0, windwalls = 0, cairns = 0;   // C-08
    DrainageOff off;
    for (int dl = -15; dl <= 15; dl++)
        for (int dn = -15; dn <= 15; dn++) {
            RuinSpec sp;
            if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, true)) continue;
            cnt[sp.sclass]++;
            bool good = ruinSiteOk(g, sp);
            std::vector<ShardSite> ss; recordSitesOf(g, sp, cs[sp.people], ss); shardsN[sp.sclass] += (int)ss.size();   // C-03; C-13: its people's; T-04: its records
            if (good) { ok[sp.sclass]++; if (sp.harbour) { harbours++; if (sp.site.shoreDry) dryHarbours++; } if (sp.terraced) terraced++; if (sp.cistern) cisterns++; if (sp.windwall) windwalls++; if (sp.cairns) cairns++; }   // C-08
            if (printed < 14 || (good && printed < 20)) {
                Vec3 u = StarSystem::bodyFromLatLon(sp.lat, sp.lon);
                SurfaceSample s0 = sampleSurface(g, u, 64.0);
                double dAng = sp.size / (g.R * 1000.0), hmin = s0.height, hmax = s0.height; int wet = 0, sea = s0.oldSea > 0.5;
                for (int k = 0; k < 4; k++) {
                    double a = k * PI / 2;
                    SurfaceSample q = sampleSurface(g, StarSystem::bodyFromLatLon(sp.lat + std::cos(a) * dAng, sp.lon + std::sin(a) * dAng / std::max(std::cos(sp.lat), 0.05)), 64.0);
                    if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water)) wet++;
                    if (q.oldSea > 0.5) sea++;
                    hmin = std::min(hmin, q.height); hmax = std::max(hmax, q.height);
                }
                printf("  %-8s%s at %7.3f %8.3f: %2zu buildings, plan %d, r %3.0f m: %s (h %.0f %s, water %s, %d corners wet, %d on the old sea, spread %.0f m of %.0f)\n",
                       SETTLEMENT_CLASS_NAMES[sp.sclass], np > 1 ? fmt(" (people %d)", sp.people + 1).c_str() : "", sp.lat / DEG, sp.lon / DEG, sp.buildings.size(), sp.plan, sp.size, good ? "ok" : "no", s0.height, MATERIAL_NAMES[s0.material],
                       s0.water > -1e8 ? "yes" : "none", wet, sea, hmax - hmin, 0.5 * sp.size);
                printf("           the terrain: slope %.3f falling to %.0f deg, the shore %s, relief %.2f, the ridge at %.0f deg, the wind from %.0f deg%s%s\n", sp.site.slope, sp.site.downhill / DEG,
                       sp.site.shoreDist < 1e8 ? fmt("%.0f m at %.0f deg%s", sp.site.shoreDist, sp.site.shoreDir / DEG, sp.site.shoreDry ? " (dry)" : "").c_str() : "none within 500 m", sp.site.relief, sp.site.ridgeDir / DEG, sp.site.wind / DEG,
                       featureList(sp).empty() ? "" : "; features: ", featureList(sp).c_str());   // C-08
                if (!ss.empty()) { printf("           shards:"); for (const ShardSite& s : ss) printf("  %d in %s %d (%s, %.0f %.0f)", s.index, BUILDING_KIND_NAMES[sp.buildings[s.building].kind], s.building, buildingPlaceName(s.place), s.x, s.z); printf("\n"); }   // C-03
                printed++;
            }
        }
    printf("31 x 31 cells round %.2f %.2f: %d hamlets (%d sited), %d villages (%d), %d towns (%d), %d monuments (%d), %d cities and capitals (%d); shards: %d in the hamlets, %d in the villages, %d in the towns, %d at the monuments\n",
           lat / DEG, lon / DEG, cnt[0], ok[0], cnt[1], ok[1], cnt[2], ok[2], cnt[3], ok[3], cnt[SC_CITY] + cnt[SC_CAPITAL], ok[SC_CITY] + ok[SC_CAPITAL], shardsN[0], shardsN[1], shardsN[2], shardsN[3]);
    printf("the terrain's features among the sited (C-08): %d harbours (%d on the dry shore), %d terraced, %d cisterns, %d wind walls, %d with cairns\n", harbours, dryHarbours, terraced, cisterns, windwalls, cairns);
    return 0;
}

// C-12: `graves <sx> <sz> <body index> [latDeg lonDeg] [cells]`: a world's calendar (`calendarLine`), its span and its last recording
// (`lastShardOf`: the index, the year, the text), then the graves (`peoples/graves.h`) of the settlements within `cells` (default 4)
// of the point's ruin cell: each settlement's class and count, each stone's name, years, standing or fallen, and its place
int runGraves(int argc, char** argv) {
    int which = takePeopleArg(argc, argv);   // C-13: `second` for the world's second people
    if (argc < 5) { printf("graves <sx> <sz> <body index> [latDeg lonDeg] [cells]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    double lat = argc > 6 ? atof(argv[5]) * DEG : 0.2, lon = argc > 6 ? atof(argv[6]) * DEG : 0.7;
    int H = argc > 7 ? atoi(argv[7]) : 4;
    Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) { printf("%s: no civilisation, no graves\n", b.name.c_str()); return 0; }
    int np = std::max(1, peoplesOf(g)); which = np > 1 ? which : 0;   // C-13: each settlement's graves are its people's
    Culture cs[2] = {cultureOf(g, 0), np > 1 ? cultureOf(g, 1) : Culture()}; Lore Ls[2] = {loreQuick(sys, b, g, 0), np > 1 ? loreQuick(sys, b, g, 1) : Lore()};
    const Culture& c = cs[which]; const Lore& L = Ls[which]; (void)c;
    printf("%s, %s: the %s count %s; %d years from the founding of %s to the last recording, their year %.1f of their days, %d moons\n", b.name.c_str(), KINDS[b.type].name, L.people.c_str(), calendarLine(L).c_str(), L.spanYears + 1, L.city.c_str(), L.yearDays, L.moons);
    {   // the last recording, and the end's count among the fifty
        Lore full = loreOf(sys, b, g, which);
        std::vector<Shard> fifty; shardsOf(sys, b, g, full, SHARDS_PER_WORLD, fifty);
        int ends = 0, latest = -1; for (int i = 0; i < SHARDS_PER_WORLD; i++) { if (fifty[i].tone == ST_END) ends++; if (latest < 0 || fifty[i].year > fifty[latest].year) latest = i; }
        printf("the last recording: S%d (%s), %d of the fifty with the end's tone, the latest year S%d's; its date %s:\n  %s\n", L.last, L.last >= 0 ? shardKindName(fifty[L.last].kind) : "-", ends, latest, L.last >= 0 ? shardDate(full, fifty[L.last]).c_str() : "-", L.last >= 0 ? fifty[L.last].text.c_str() : "-");
        printf("the dates of the first five: "); for (int i = 0; i < 5; i++) printf("%s%s", i ? "; " : "", shardDate(full, fifty[i]).c_str()); printf("\n");
    }
    int gLat0, gLon0; ruinCellOf(g, lat, lon, gLat0, gLon0);
    int settle = 0, stones = 0, fallen = 0, byClass[4] = {0, 0, 0, 0}, settleBy[4] = {0, 0, 0, 0}; double t0 = nowSec();
    for (int dl = -H; dl <= H; dl++) for (int dn = -H; dn <= H; dn++) {
        RuinSpec sp;
        if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, true)) continue;
        { DrainageOff off; if (!ruinSiteOk(g, sp)) continue; }
        std::vector<Grave> gr; gravesOf(Ls[sp.people], sp, cs[sp.people], gr);
        settle++; settleBy[sp.sclass]++; stones += (int)gr.size(); byClass[sp.sclass] += (int)gr.size();
        printf("%s %d %d (radius %.0f m, %zu buildings%s%s): %zu graves\n", SETTLEMENT_CLASS_NAMES[sp.sclass], gLat0 + dl, gLon0 + dn, sp.size, sp.buildings.size(), sp.walled ? ", walled" : "", np > 1 ? fmt(", of the %s", Ls[sp.people].people.c_str()).c_str() : "", gr.size());
        for (const Grave& x : gr) { if (x.fallen) fallen++; printf("  %-40s %s  at %+6.1f %+6.1f (%.0f m out, facing %.0f deg)\n", graveLine(Ls[sp.people], x).c_str(), x.fallen ? "fallen  " : "standing", x.x, x.z, std::sqrt(x.x * x.x + x.z * x.z), x.heading / DEG); }
    }
    printf("%d settlements (%d hamlets, %d villages, %d towns, %d monuments) with %d graves (%d, %d, %d, %d by class), %d fallen, in %.1f ms\n", settle, settleBy[0], settleBy[1], settleBy[2], settleBy[3], stones, byClass[0], byClass[1], byClass[2], byClass[3], fallen, 1000 * (nowSec() - t0));
    return 0;
}
