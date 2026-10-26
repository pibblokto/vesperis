// C-10 (2026-10-03): star charts as shards (see charts.h).
#include "charts.h"
#include "records.h"
#include "signals.h"
#include "peoples.h"
#include "galaxy/drifters.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <set>

// ---- H-05 (2026-10-10): the ways a chart was made ---------------------------------------------------------------------------------

const char* const CHART_STYLE_NAMES[] = {"a cave's ceiling", "a carved bone", "a painted vault", "an astrolabe's plate", "a page of a codex", "an engraved atlas",
                                         "a photograph on glass", "a survey", "a navigation chart", "a chart of far places", "the sun's measure"};   // the last
const char* const CHART_STYLE_KEYS[] = {"ceiling", "bone", "vault", "astrolabe", "page", "atlas", "plate", "survey", "routes", "beyond", "sunmeasure"};
static_assert(sizeof(CHART_STYLE_NAMES) / sizeof(CHART_STYLE_NAMES[0]) == CHS_COUNT && sizeof(CHART_STYLE_KEYS) / sizeof(CHART_STYLE_KEYS[0]) == CHS_COUNT, "a name and a key a way");

int chartStyleOf(int age, int medium) {
    switch (age) {
        case AGE_PREHISTORIC: return medium == RM_BONE ? CHS_BONE : CHS_CEILING;
        case AGE_MEDIEVAL: return medium == RM_PLATE ? CHS_ASTROLABE : (medium == RM_PAINTED ? CHS_VAULT : CHS_PAGE);
        case AGE_INDUSTRIAL: return medium == RM_GLASS ? CHS_PLATE : CHS_ATLAS;
        case AGE_MODERN: return CHS_SURVEY;
        case AGE_FUTURISTIC: return CHS_ROUTES;
        default: return CHS_BEYOND;
    }
}

std::string chartStyleLine(int style, int medium) {
    switch (style) {
        case CHS_CEILING: return "ON A CAVE'S CEILING";
        case CHS_BONE: return "CARVED ON A BONE";
        case CHS_VAULT: return "ON A PAINTED VAULT";
        case CHS_ASTROLABE: return "AN ASTROLABE'S PLATE";
        case CHS_PAGE: return "A PAGE OF A CODEX";
        case CHS_ATLAS: return "AN ENGRAVED ATLAS";
        case CHS_PLATE: return "A PHOTOGRAPH ON GLASS";
        case CHS_SURVEY: return medium == RM_GLASS ? "A SURVEY ON GLASS" : (medium == RM_DRIVE ? "A SURVEY ON A DRIVE" : "A PRINTED SURVEY");
        case CHS_ROUTES: return medium == RM_DRIVE ? "A NAVIGATION CHART, A DRIVE" : "A NAVIGATION CHART, A CRYSTAL";
        case CHS_SUNMEASURE:
            switch (medium) {
                case RM_PAINTED: case RM_STONE: return "THE SUN'S MEASURE, PAINTED";
                case RM_BONE: return "THE SUN'S MEASURE ON A BONE";
                case RM_TABLET: case RM_SCROLL: case RM_CODEX: return "THE SUN'S MEASURE, A PAGE";
                case RM_PLATE: return "THE SUN'S MEASURE, ENGRAVED";
                case RM_PRINTED: case RM_GLASS: return "THE SUN'S MEASURE, A PRINTED GRAPH";
                default: return "THE SUN'S LIGHT CURVE";
            }
        default: return medium == RM_HUM ? "FAR PLACES, IN THE HUM" : "FAR PLACES, IN A CRYSTAL";
    }
}

// the animals (0..11) and the creatures and heroes (12..23) drawn with a figure, seven by six
const char* const CHART_GLYPH_ROWS[CHART_GLYPHS][CHART_GLYPH_H] = {
    {"....#.#", ".....#.", "....##.", "#####..", "#####..", "#...#.."},   // 0 a stag
    {"..##...", ".#####.", "#######", "######.", ".#..#..", ".#..#.."},   // 1 a bear
    {"....#.#", "....#.#", "...###.", ".####..", "#####..", ".#.#..."},   // 2 a hare
    {"#.....#", "##...##", ".##.##.", "..###..", "...#...", "..#.#.."},   // 3 a bird
    {".......", "#..##..", "#######", "######.", "#..##..", "......."},   // 4 a fish
    {".......", "......#", ".##..##", "#..##..", ".......", "......."},   // 5 a snake
    {".....##", "....##.", "#####..", "#####..", "#...#..", "#...#.."},   // 6 a horse
    {".......", ".####..", "######.", "#######", ".#..#.#", ".#..#.."},   // 7 a boar
    {".....#.", "#...###", ".####..", ".####..", ".#..#..", ".#..#.."},   // 8 a wolf
    {"#.#.#.#", ".#####.", "#.###.#", ".#####.", "#.#.#.#", "......."},   // 9 a spider
    {"#.....#", ".#####.", "##.#.##", ".#####.", ".#####.", "..#.#.."},   // 10 an owl
    {"#.....#", ".#...#.", ".#####.", ".##.##.", "..###..", "..#.#.."},   // 11 a bull's head
    {".......", "#..#..#", "##.#.##", "#######", "#######", "......."},   // 12 a crown
    {"......#", ".....#.", "....#..", ".#.#...", "..#....", ".#.#..."},   // 13 a sword
    {"...#...", "...##..", "...###.", "#######", ".#####.", "......."},   // 14 a ship
    {"##.....", ".##..##", "..####.", ".#..##.", "#....#.", "....#.."},   // 15 a dragon
    {".#.#.#.", ".#####.", "..###..", "..#.#..", "..###..", ".#####."},   // 16 a tower
    {".#.#...", "#..#...", "#######", "#..#...", ".#.#...", "......."},   // 17 a bow
    {"#.....#", "#.#.#.#", "#.#.#.#", "#.#.#.#", ".#####.", "......."},   // 18 a harp
    {".####..", ".####..", "...#...", "...#...", "...#...", "...#..."},   // 19 a hammer
    {"#.....#", ".#...#.", "..###..", "...#...", "...#...", ".#####."},   // 20 a cup
    {"..###..", ".#.#.#.", "#######", ".#.#.#.", "..###..", "......."},   // 21 a wheel
    {"...#...", ".#####.", "...#...", "..#.#..", ".#...#.", "......."},   // 22 a person
    {".......", ".#####.", "#..#..#", ".#####.", ".......", "......."}};  // 23 an eye

// the sky of a star: every star of the cube the game draws round it, the brightest first (the star field's own measure:
// the luminosity over the squared distance in light years; ties by the seed, so no sector order shows through)
void chartSkyOf(const Star& home, std::vector<ChartStar>& out) {
    out.clear();
    for (int64_t x = home.sx - CHART_RADIUS_SECTORS; x <= home.sx + CHART_RADIUS_SECTORS; x++)
        for (int64_t y = home.sy - CHART_RADIUS_SECTORS; y <= home.sy + CHART_RADIUS_SECTORS; y++)
            for (int64_t z = home.sz - CHART_RADIUS_SECTORS; z <= home.sz + CHART_RADIUS_SECTORS; z++) {
                Star q; if (!starInSector(x, y, z, q, false) || q.seed == home.seed) continue;
                Vec3 d = q.pos - home.pos;
                double d2 = length2(d) / (SECTOR_KM * SECTOR_KM);
                if (d2 < 1e-6) continue;
                ChartStar cs; cs.star = q; cs.dir = normalize(d); cs.bright = q.luminosity / d2;
                out.push_back(cs);
            }
    std::sort(out.begin(), out.end(), [](const ChartStar& a, const ChartStar& b) { return a.bright != b.bright ? a.bright > b.bright : a.star.seed < b.star.seed; });
}

namespace {
// D-05: where a drifter was some years after its people's last sunrise: off the star it left along its way by its speed for those years (the
// game's light year is 1.5e10 km: a few km/s for a century is a light year and more)
Vec3 drifterThen(const StarSystem& dark, const Star& home, int yearsAfter) {
    const Vec3 dir = normalize(dark.star.pos - home.pos);
    return home.pos + dir * (drifterSpeedKms(dark.star) * std::max(1, yearsAfter) * 3.15576e7);
}
// the first people's world of a star's system, with its people's name and whether it had radio (A-01: only such a world is on
// the air, `starSignal`)
bool peoplesWorldOf(const Star& s, int& body, std::string& people, bool& onAir) {
    StarSystem sys; sys.generate(s);
    const int pw = systemPeoplesWorld(sys);   // C-01's kinds first, then a dead world's
    if (pw < 0) return false;
    const BodyGen g = BodyGen::make(sys.bodies[pw]);
    body = pw; people = peopleNameOf(g); onAir = peoplesOnAir(g) != 0;
    return true;
}
// H-05: a figure's name: the oldest age's are animals (the people's own way of saying one: plain, with a word of colour or of what it
// does, its young or its tracks, a pair, a person's), the later ages' creatures, heroes and things (C-10's twenty-four among them),
// each with its glyph; a people's figures all differ
struct FigWord { const char* one; const char* many; int glyph; };
const FigWord ANIMALS[] = {
    {"elk", "elk", 0}, {"stag", "stags", 0}, {"reindeer", "reindeer", 0}, {"bear", "bears", 1}, {"hare", "hares", 2}, {"crane", "cranes", 3}, {"eagle", "eagles", 3},
    {"raven", "ravens", 3}, {"goose", "geese", 3}, {"swan", "swans", 3}, {"salmon", "salmon", 4}, {"pike", "pike", 4}, {"seal", "seals", 4}, {"adder", "adders", 5},
    {"eel", "eels", 5}, {"lizard", "lizards", 5}, {"horse", "horses", 6}, {"boar", "boars", 7}, {"wolf", "wolves", 8}, {"fox", "foxes", 8}, {"dog", "dogs", 8},
    {"lynx", "lynxes", 8}, {"spider", "spiders", 9}, {"beetle", "beetles", 9}, {"owl", "owls", 10}, {"bull", "bulls", 11}, {"ibex", "ibex", 11}, {"bison", "bison", 11}};
const char* const ANIMAL_WAYS[] = {"white", "red", "black", "grey", "lame", "old", "great", "little", "running", "sleeping", "wounded", "hungry", "first", "one-eyed",
                                   "swimming", "crouching", "weeping", "laughing", "lost", "dancing"};
const char* const ANIMAL_PARTS[] = {"calf", "tracks", "eye", "tail", "skull", "den", "heart", "jaw", "mother", "shadow"};
const char* const ANIMAL_KEEPERS[] = {"hunter", "mother", "child", "grandmother", "healer", "fisher", "stranger", "first woman", "drummer", "old man"};
const FigWord HEROES[] = {   // the persons first (they own things: "the smith's hammer"), then the creatures, then the things
    {"king", "kings", 12}, {"queen", "queens", 12}, {"warrior", "warriors", 13}, {"archer", "archers", 17}, {"smith", "smiths", 19}, {"giant", "giants", 22},
    {"shepherd", "shepherds", 22}, {"maiden", "maidens", 22}, {"old woman", "old women", 22}, {"pilgrim", "pilgrims", 22}, {"hunter", "hunters", 17},
    {"weaver", "weavers", 21}, {"fool", "fools", 22}, {"horseman", "horsemen", 6}, {"dancer", "dancers", 22}, {"boatman", "boatmen", 14}, {"judge", "judges", 20},
    {"mother", "mothers", 22}, {"wanderer", "wanderers", 22},
    {"dragon", "dragons", 15}, {"serpent", "serpents", 5}, {"lion", "lions", 11}, {"eagle", "eagles", 3}, {"stag", "stags", 0}, {"great fish", "great fish", 4},
    {"wolf", "wolves", 8}, {"owl", "owls", 10}, {"bird", "birds", 3},
    {"ship", "ships", 14}, {"tower", "towers", 16}, {"crown", "crowns", 12}, {"sword", "swords", 13}, {"harp", "harps", 18}, {"cup", "cups", 20}, {"wheel", "wheels", 21},
    {"scales", "scales", 20}, {"plough", "ploughs", 21}, {"lamp", "lamps", 20}, {"gate", "gates", 16}, {"ladder", "ladders", 16}, {"eye", "eyes", 23},
    {"bridge", "bridges", 16}, {"hearth", "hearths", 20}, {"boat", "boats", 14}, {"river", "rivers", 5}, {"hand", "hands", 22}, {"well", "wells", 20},
    {"tree", "trees", 21}, {"net", "nets", 9}, {"fire", "fires", 20}};
constexpr int HERO_PERSONS = 19, HERO_CREATURES = 28;   // the persons are the first 19, the creatures to 28, the things after
const char* const HERO_WAYS[] = {"drowned", "sleeping", "blind", "old", "young", "lost", "first", "weeping", "laughing", "wounded", "golden", "silver", "iron",
                                 "crowned", "last", "faithful", "wandering", "broken", "hidden", "burning"};
const char* const HERO_THINGS[] = {"ship", "crown", "sword", "cup", "harp", "hammer", "spear", "wheel", "lamp", "shield", "cloak", "bow", "chair", "door"};
const char* const THING_WAYS[] = {"golden", "silver", "iron", "broken", "hidden", "burning", "last", "first", "old", "lost", "great", "little", "white", "black"};   // what a thing may be
const char* const NUMBER_WORDS[] = {"two", "three", "four", "five", "seven"};
// a navigation chart's routes (CHS_ROUTES) and the far future's places where nothing is (CHS_BEYOND)
const char* const ROUTE_WAYS[] = {"first", "long", "cold", "narrow", "slow", "dark", "quiet", "old", "short", "empty", "deep", "bright", "last", "second", "high",
                                  "low", "broken", "hidden", "straight", "bending"};
const char* const ROUTE_NOUNS[] = {"road", "crossing", "way", "run", "passage", "line", "reach", "track"};
const char* const SIGN_WAYS[] = {"empty", "quiet", "far", "cold", "last", "other", "hidden", "open", "waiting", "sleeping", "black", "white", "nameless", "unlit"};
const char* const SIGN_NOUNS[] = {"door", "shore", "place", "well", "fire", "deep", "gate", "mouth", "knot", "edge", "sea", "field", "hollow", "room"};
template <typename T, size_t N> constexpr int countOf(const T (&)[N]) { return (int)N; }

// one figure's name of its kind: an animal's way (`animals`), else a creature's, a hero's or a thing's
std::string figureName(Rng& rng, bool animals, int& glyph) {
    if (animals) {
        const FigWord& a = ANIMALS[rng.irange(countOf(ANIMALS))]; glyph = a.glyph;
        const double r = rng.uni();
        if (r < 0.32) return std::string("the ") + a.one;
        if (r < 0.62) return std::string("the ") + ANIMAL_WAYS[rng.irange(countOf(ANIMAL_WAYS))] + " " + a.one;
        if (r < 0.78) return std::string("the ") + a.one + "'s " + ANIMAL_PARTS[rng.irange(countOf(ANIMAL_PARTS))];
        if (r < 0.90) return std::string("the ") + NUMBER_WORDS[rng.irange(3)] + " " + a.many;
        return std::string("the ") + ANIMAL_KEEPERS[rng.irange(countOf(ANIMAL_KEEPERS))] + "'s " + a.one;
    }
    const int k = rng.irange(countOf(HEROES));
    const FigWord& h = HEROES[k]; glyph = h.glyph;
    const bool person = k < HERO_PERSONS, thing = k >= HERO_CREATURES;
    const double r = rng.uni();
    if (r < 0.36) return std::string("the ") + h.one;
    if (r < 0.70) return std::string("the ") + (thing ? THING_WAYS[rng.irange(countOf(THING_WAYS))] : HERO_WAYS[rng.irange(countOf(HERO_WAYS))]) + " " + h.one;
    if (r < 0.90 && person) return std::string("the ") + h.one + "'s " + HERO_THINGS[rng.irange(countOf(HERO_THINGS))];
    if (r < 0.96 && !thing && std::string(h.one) != h.many) return std::string("the ") + NUMBER_WORDS[rng.irange(2)] + " " + h.many;
    return std::string("the ") + h.one;
}
}

namespace {
// H-05: a far future's far mark: the k-th of its charts that look beyond, a real star thousands of light years off: toward the galaxy's
// heart (a share of the way to the centre), in a cluster (an open one within 1,500 ly or the nearest globular), out of the disc (as far
// up as the stars go) or along it (1,500 to 6,000 ly); found once a people (a cube of a few sectors searched where the stars are
// dense) and kept
std::mutex farMu;
std::map<uint64_t, std::pair<ChartMark, int>> farCache;
bool farMarkOf(const BodyGen& g, const Star& home, int k, ChartMark& m, int& kind) {
    const uint64_t key = mix64(g.seed ^ 0xFA4A1ULL ^ ((uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL));
    {
        std::lock_guard<std::mutex> lk(farMu);
        auto it = farCache.find(key);
        if (it != farCache.end()) { m = it->second.first; kind = it->second.second; return true; }
    }
    Rng rng(key);
    static const double KW[4] = {1.2, 1.0, 0.8, 1.0};
    kind = 1 + rng.pick(KW, 4);
    const Vec3 h = home.pos / SECTOR_KM;
    Vec3 target = h;
    const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z));
    if (kind == 2) {   // a cluster: an open one near (the cells round home), else the nearest globular
        std::vector<Vec3> found;
        const double ox = h.x - GALAXY_CENTRE_SX, oz = h.z - GALAXY_CENTRE_SZ;
        const int64_t cx0 = (int64_t)std::floor(ox / OPEN_CLUSTER_CELL), cz0 = (int64_t)std::floor(oz / OPEN_CLUSTER_CELL), cy0 = (int64_t)std::floor(h.y / OPEN_CLUSTER_CELL);
        for (int64_t cx = cx0 - 15; cx <= cx0 + 15; cx++) for (int64_t cz = cz0 - 15; cz <= cz0 + 15; cz++) for (int64_t cy = cy0 - 1; cy <= cy0 + 1; cy++) {
            Vec3 c; double r; if (!openClusterInCell(cx, cy, cz, c, r)) continue;
            const double d = length(c - h); if (d > 200 && d < 1500) found.push_back(c);
        }
        if (!found.empty() && rng.chance(0.6)) target = found[rng.irange((int)found.size())];
        else {
            double best = 1e18;
            const int64_t gx0 = (int64_t)std::floor(ox / GLOBULAR_CELL), gz0 = (int64_t)std::floor(oz / GLOBULAR_CELL);
            for (int64_t cx = gx0 - 5; cx <= gx0 + 5; cx++) for (int64_t cz = gz0 - 5; cz <= gz0 + 5; cz++) for (int64_t cy = -3; cy <= 2; cy++) {
                Vec3 c; double r; if (!globularInCell(cx, cy, cz, c, r)) continue;
                const double d = length(c - h); if (d < best) { best = d; target = c; }
            }
            if (best > 1e17) kind = 1;
        }
    }
    if (kind == 1) target = h + (Vec3(GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ) - h) * rng.range(0.3, 0.92);   // toward the heart
    else if (kind == 3) {   // out of the disc: a few hundred light years up or down and a little along
        const double a = rng.uni() * TAU;
        target = h + Vec3(std::cos(a), 0, std::sin(a)) * rng.range(100, 800) + Vec3(0, (rng.chance(0.5) ? 1 : -1) * rng.range(250, 600), 0);
    } else if (kind == 4) {   // along the disc, any way but toward the heart
        const double a = rng.range(0.5, TAU - 0.5);
        const Vec3 side(toCore.z, 0, -toCore.x);
        target = h + (toCore * std::cos(a) + side * std::sin(a)) * rng.range(1500, 6000);
    }
    Star st; bool ok = false;
    for (int R : {2, 4, 7}) if (nearestStarTo(target * SECTOR_KM, R, st, home.seed)) { ok = true; break; }
    if (!ok) return false;
    m = ChartMark(); m.star = st; m.body = -1; m.how = 3; m.ly = length(st.pos - home.pos) / SECTOR_KM;
    std::lock_guard<std::mutex> lk(farMu);
    if (farCache.size() > 4096) farCache.clear();
    farCache[key] = {m, kind};
    return true;
}
}

int chartFarKind(const BodyGen& g, const Star& home, int k) { ChartMark m; int kind = 0; return farMarkOf(g, home, k, m, kind) ? kind : 0; }

// the stars a world's charts mark. A people's world is at one star in eight, so a chart that marks one is a choice, not a
// chance ("sometimes it points at another dead world"): the first chart marks the peoples it knew (the nearest it heard over
// the radio's reach, else the brightest near one among the stars of its sky), the second the brightest star of its nights
// (the one the lore names), the rest a people (heard and near in turns) or a bright star by the chart's own coin; no star twice.
// H-05: a chart of the far future marks a star thousands of light years off instead (`farMarkOf`); the sky's charts keep C-10's order
// among themselves
void chartMarksOf(const StarSystem& sys0, const Body& b0, const BodyGen& g, std::vector<ChartMark>& out, int countIn) {
    const PeoplesWorld PW = peoplesWorldOf(sys0, b0);   // a drifter's people's world at its former orbit
    const StarSystem& sys = *PW.sys;
    out.clear();
    const Star& home = sys.star;
    std::vector<ChartStar> sky; chartSkyOf(home, sky);
    std::vector<ChartMark> near, heard;
    const PeopleAge pa = peopleAgeOf(g, 0);
    const bool listened = pa.radio;   // only a people that had radio heard anyone (`g` is the people's generator)
    for (int k = 0; k < (int)sky.size() && k < CHART_NEAR_CANDIDATES; k++) {
        int body = -1; std::string people; bool onAir = false;
        if (!peoplesWorldOf(sky[k].star, body, people, onAir)) continue;
        ChartMark m; m.star = sky[k].star; m.body = body; m.people = people; m.ly = length(sky[k].star.pos - home.pos) / SECTOR_KM;
        m.how = listened && onAir && sectorTransmits(m.star.sx, m.star.sy, m.star.sz) ? 2 : 1;   // a near people that transmits is known by its voice
        (m.how == 2 ? heard : near).push_back(m);
    }
    std::vector<Signal> far; if (listened) signalsNear(home.pos, far);
    for (const Signal& s : far) {
        if (s.kind != SIG_PEOPLE || s.star.seed == home.seed) continue;
        bool have = false; for (const ChartMark& m : heard) if (m.star.seed == s.star.seed) have = true;
        if (have) continue;
        int body = -1; std::string people; bool onAir = false;
        if (!peoplesWorldOf(s.star, body, people, onAir)) continue;
        ChartMark m; m.star = s.star; m.body = body; m.people = people; m.how = 2; m.ly = s.distLy;
        heard.push_back(m);
    }
    std::sort(heard.begin(), heard.end(), [](const ChartMark& a, const ChartMark& c) { return a.ly < c.ly; });
    const int count = countIn >= 0 ? countIn : chartShardsOf(g);   // the charts past the fifty after C-10's (the same marks first)
    // each chart's way by its record's age and medium (the world's catalogue: `g` is the people's generator, the body the world), and
    // whether its year was one of their radio's: a chart drawn before it marks no voice (a near people that transmits is a world among
    // their bright stars to it, a far one nothing)
    std::vector<int> styles(count, CHS_PAGE); std::vector<char> canHear(count, listened ? 1 : 0);
    std::vector<char> lostSun(count, 0);   // a drifter's people's chart of a year without the sun
    ChartMark sunMark;
    {
        const BodyGen wg = BodyGen::make(b0);   // the world's own generator (a drifter's, not its former orbit's)
        const int which = g.seed == wg.seed ? 0 : 1;
        std::shared_ptr<const RecordCatalogue> C = recordsOf(wg, which);
        std::vector<int> recs; recordChartRecords(g, C->count, recs);
        int onset = 0, lastSunrise = 0;
        const bool lost = sunLossYears(wg, which, onset, lastSunrise);
        for (int j = 0; j < count && j < (int)recs.size(); j++) if (recs[j] >= 0 && recs[j] < C->count) {
            styles[j] = chartStyleOf(C->entries[recs[j]].age, C->entries[recs[j]].medium);
            canHear[j] = listened && C->entries[recs[j]].year >= pa.radioYear ? 1 : 0;
            if (lost && C->entries[recs[j]].year >= onset) lostSun[j] = 1;
        }
        if (lost) {   // the star that was their sun, as far as the drifter was from it at their end
            sunMark.star = home; sunMark.body = -1; sunMark.how = 4;
            sunMark.ly = length(drifterThen(sys0, home, peopleAgeOf(wg, which).lastYear - lastSunrise) - home.pos) / SECTOR_KM;
        }
    }
    std::set<uint64_t> used;
    auto take = [&](const ChartMark& m) { if (used.count(m.star.seed)) return false; used.insert(m.star.seed); out.push_back(m); return true; };
    size_t hi = 0, ni = 0, si = 0;
    std::vector<ChartMark> quiet;   // the peoples near enough to be seen among their bright stars, for a chart drawn before their radio
    for (const ChartMark& m : near) quiet.push_back(m);
    for (const ChartMark& m : heard) for (int k = 0; k < (int)sky.size() && k < CHART_NEAR_CANDIDATES; k++) if (sky[k].star.seed == m.star.seed) { ChartMark q = m; q.how = 1; quiet.push_back(q); break; }
    std::sort(quiet.begin(), quiet.end(), [](const ChartMark& a, const ChartMark& c) { return a.ly < c.ly; });
    size_t qi = 0;
    auto takePeople = [&](int turn, bool hear) {   // the heard and the near in turns, whichever is left; before their radio the near alone
        if (!hear) { while (qi < quiet.size()) if (take(quiet[qi++])) return true; return false; }
        bool fromHeard = (turn % 2 == 0) ? hi < heard.size() : ni >= near.size();
        while (hi < heard.size() || ni < near.size()) {
            if (fromHeard && hi < heard.size()) { if (take(heard[hi++])) return true; }
            else if (ni < near.size()) { if (take(near[ni++])) return true; }
            else if (hi < heard.size()) { if (take(heard[hi++])) return true; }
        }
        return false;
    };
    auto takeBright = [&]() { for (; si < sky.size(); si++) { ChartMark m; m.star = sky[si].star; m.how = 0; m.ly = length(sky[si].star.pos - home.pos) / SECTOR_KM; if (take(m)) { si++; return true; } } return false; };
    uint64_t coin = mix64(g.seed ^ 0xC4A29ULL);
    int jj = 0, farK = 0;   // the sky's charts in C-10's order; the far future's in their own
    for (int j = 0; j < count; j++) {
        if (lostSun[j]) { out.push_back(sunMark); continue; }
        if (styles[j] == CHS_BEYOND) {
            ChartMark m; int kind = 0;
            if (farMarkOf(g, home, farK++, m, kind)) { out.push_back(m); continue; }
        }
        bool people = jj == 0 || (jj >= 2 && ((coin >> jj) & 1));
        bool got = people ? (takePeople(jj, canHear[j]) || takeBright()) : (takeBright() || takePeople(jj, canHear[j]));
        jj++;
        if (!got) break;
    }
}

void chartFigureNames(uint64_t worldSeed, int n, std::vector<std::string>& out, std::vector<int>* glyphs, bool animals) {
    out.clear(); if (glyphs) glyphs->clear();
    Rng rng(mix64(worldSeed ^ 0x9A3E5ULL));
    std::set<std::string> heads;   // no two figures of one word ("the elk" and "the lame elk" would read as one)
    for (int i = 0, tries = 0; i < n && tries < 400; tries++) {
        int glyph = -1; const std::string nm = figureName(rng, animals, glyph);
        const std::string head = nm.substr(nm.rfind(' ') + 1);
        if (heads.count(head) || std::find(out.begin(), out.end(), nm) != out.end()) continue;
        heads.insert(head); out.push_back(nm); if (glyphs) glyphs->push_back(glyph); i++;
    }
}

std::string chartFigureList(const std::vector<std::string>& names) {
    std::string s;
    for (size_t i = 0; i < names.size(); i++) {
        if (i > 0) s += i + 1 == names.size() ? " and " : ", ";
        s += names[i];
    }
    return s;
}

bool chartProject(const StarChart& c, const Vec3& dir, double& x, double& y) {
    double ang = std::acos(clampd(dot(dir, c.centre), -1, 1));
    double px = dot(dir, c.right), py = dot(dir, c.up), l = std::sqrt(px * px + py * py);
    if (l < 1e-9) { x = 0; y = 0; return ang <= CHART_FIELD; }
    double r = ang / CHART_FIELD;
    x = px / l * r; y = py / l * r;
    return ang <= CHART_FIELD;
}

void skyFiguresOf(const BodyGen& g, const std::vector<ChartStar>& sky, std::vector<ChartFigure>& out) {
    out.clear();
    int N = std::min((int)sky.size(), CHART_BRIGHT);
    if (N < 2) return;
    uint64_t seed = mix64(g.seed ^ 0xF16A5ULL);
    int n = CHART_FIGURES_MIN + (int)(seed % CHART_FIGURES_VAR);
    std::vector<std::string> names; std::vector<int> glyphs;
    chartFigureNames(g.seed, n, names, &glyphs, peopleAgeOf(g, 0).age == AGE_PREHISTORIC);   // H-05: a people of the oldest age names animals
    Rng rng(mix64(seed ^ 0x51ULL));
    std::vector<char> used(N, 0);
    auto angBetween = [&](int a, int c) { return std::acos(clampd(dot(sky[a].dir, sky[c].dir), -1, 1)); };
    for (int f = 0; f < n; f++) {
        ChartFigure fig; fig.name = f < (int)names.size() ? names[f] : names.back(); fig.glyph = f < (int)glyphs.size() ? glyphs[f] : -1;
        int seedStar = -1;
        for (int tries = 0; tries < 2 && seedStar < 0; tries++)   // with the spacing first, then without
            for (int k = 0; k < N && seedStar < 0; k++) {
                if (used[k]) continue;
                bool apart = true;
                if (tries == 0) for (int j = 0; j < N && apart; j++) if (used[j] && angBetween(k, j) < CHART_APART) apart = false;
                if (apart) seedStar = k;
            }
        int size = 3 + rng.irange(4);
        if (seedStar < 0) break;
        fig.stars.push_back(seedStar); used[seedStar] = 1;
        while ((int)fig.stars.size() < size) {
            int best = -1, link = -1; double bd = CHART_LINK;
            for (int k = 0; k < N; k++) {
                if (used[k]) continue;
                for (int s : fig.stars) { double a = angBetween(k, s); if (a < bd) { bd = a; best = k; link = s; } }
            }
            if (best < 0) break;
            used[best] = 1; fig.stars.push_back(best); fig.lines.push_back({link, best});
        }
        if (fig.stars.size() < 2) continue;
        fig.label = seedStar;
        out.push_back(fig);
    }
}

namespace {
// H-05: a circle of the sky (p(t) = cos(dec) (cos t e1 + sin t e2) + sin(dec) axis, t from t0 to t1) on the chart, in runs within its field
void addCircle(StarChart& c, int kind, double value, const Vec3& axis, const Vec3& e1, const Vec3& e2, double dec, double t0, double t1, int steps) {
    ChartCurve run; run.kind = kind; run.value = value;
    for (int i = 0; i <= steps; i++) {
        const double t = t0 + (t1 - t0) * i / steps;
        const Vec3 p = (e1 * std::cos(t) + e2 * std::sin(t)) * std::cos(dec) + axis * std::sin(dec);
        double x, y;
        if (chartProject(c, normalize(p), x, y)) { run.pts.push_back({(float)x, (float)y}); continue; }
        if (run.pts.size() >= 2) c.curves.push_back(run);
        run.pts.clear();
    }
    if (run.pts.size() >= 2) c.curves.push_back(run);
}
// a meridian of their sky: from their south pole to their north at the angle `a` of their hour circles
void addMeridian(StarChart& c, double value, const Vec3& axis, const Vec3& e1, const Vec3& e2, double a, int steps) {
    const Vec3 m = e1 * std::cos(a) + e2 * std::sin(a);
    addCircle(c, CC_HOUR, value, cross(m, axis), m, axis, 0, -PI / 2, PI / 2, steps);   // p(t) = cos t m + sin t axis: through both poles
}
Vec3 orbitNormalOf(const Body& o) {
    const Vec3 u(std::cos(o.orbitNode), 0, std::sin(o.orbitNode));
    const Vec3 v = cross(Vec3(0, 1, 0), u) * std::cos(o.orbitIncl) + Vec3(0, 1, 0) * std::sin(o.orbitIncl);
    return normalize(cross(u, v));
}
// the system's other planets seen from the people's world over a while round the time each stands nearest the field's middle: their
// ways along the ecliptic, a loop where an outer one turns back (the brightest five, the brighter the bigger and the nearer)
void addWanderers(StarChart& c, const StarSystem& sys, const Body& b) {
    const int host = b.parent >= 0 && sys.bodies[b.parent].type != PT_COMPANION ? b.parent : b.index;
    const Body& H = sys.bodies[host];
    struct W { int j; double bright; };
    std::vector<W> ws;
    for (const Body& o : sys.bodies) {
        if (o.index == host || o.index == b.index || o.parent != (H.parent >= 0 ? H.parent : -1) || o.type == PT_COMPANION || o.type == PT_COMET) continue;
        const double d = std::max(std::fabs(o.orbitRadiusKm - H.orbitRadiusKm), 0.05 * H.orbitRadiusKm);
        ws.push_back({o.index, o.radiusKm * o.radiusKm / (o.orbitRadiusKm * o.orbitRadiusKm * d * d)});
    }
    std::sort(ws.begin(), ws.end(), [](const W& a, const W& z) { return a.bright != z.bright ? a.bright > z.bright : a.j < z.j; });
    if (ws.size() > 5) ws.resize(5);
    const double brightest = ws.empty() ? 1 : ws[0].bright;
    for (const W& w : ws) {
        const Body& o = sys.bodies[w.j];
        const double inv = std::fabs(1 / H.orbitPeriod - 1 / o.orbitPeriod);
        if (inv < 1e-15) continue;
        const double S = 1 / inv;
        double tBest = 0, aBest = 10;
        for (int i = 0; i < 64; i++) {
            const double t = S * i / 64;
            const double a = std::acos(clampd(dot(normalize(sys.bodyPos(w.j, t) - sys.bodyPos(host, t)), c.centre), -1, 1));
            if (a < aBest) { aBest = a; tBest = t; }
        }
        if (aBest > CHART_FIELD * 0.92) continue;
        ChartPath path; path.body = w.j; path.size = 1 + 2 * std::sqrt(w.bright / brightest);
        for (int i = 0; i <= 48; i++) {
            const double t = tBest + S * (-0.3 + 0.6 * i / 48);
            double x, y;
            if (chartProject(c, normalize(sys.bodyPos(w.j, t) - sys.bodyPos(host, t)), x, y)) path.pts.push_back({(float)x, (float)y});
            else if (path.pts.size() >= 2) break;
            else path.pts.clear();
        }
        if (path.pts.size() >= 2) c.wanderers.push_back(path);
    }
}
// the charts' ways by their index (`g` the people's generator, `wg` the world's)
void chartStyles(const BodyGen& g, const BodyGen& wg, int which, int count, std::vector<int>& styles) {
    styles.assign(count, CHS_PAGE);
    std::shared_ptr<const RecordCatalogue> C = recordsOf(wg, which);
    std::vector<int> recs; recordChartRecords(g, C->count, recs);
    for (int j = 0; j < count && j < (int)recs.size(); j++) if (recs[j] >= 0 && recs[j] < C->count) styles[j] = chartStyleOf(C->entries[recs[j]].age, C->entries[recs[j]].medium);
}
}

// the chart: the hemisphere of the people's sky round the star they marked (the brightest hundred of the cube, those
// within ninety degrees of it; the mark itself added dim when it is not among them: a far transmitter is a star of no
// account in their sky), with the people's figures that fit whole within the field. H-05: and what its way shows; a navigation
// chart the map of the people's neighbourhood with its routes; the far future's its far mark with the places where nothing is
bool chartOf(const StarSystem& sys0, const Body& b0, const BodyGen& g0, const Lore& L, int index, StarChart& out) {
    const PeoplesWorld PW = peoplesWorldOf(sys0, b0);   // a drifter's people's world at its former orbit
    const StarSystem& sys = *PW.sys; const Body& b = *PW.b;
    out = StarChart();
    BodyGen g = g0; g.seed = loreSeed(g0, L);   // the people's draws
    int which = recordChartIndex(g, L.records, index);   // past the fifty too
    if (which < 0 || which >= (int)L.marks.size()) return false;
    out.index = index; out.which = which; out.seed = chartSeedOf(g, index); out.from = sys.star.pos; out.mark = L.marks[which];
    {   // its record's age and medium
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g0, L.which);
        const bool in = C && index >= 0 && index < C->count;
        out.age = in ? C->entries[index].age : peopleAgeOf(g, 0).age;
        out.medium = in ? C->entries[index].medium : RM_PAINTED;
        out.style = chartStyleOf(out.age, out.medium);
    }
    bool lostSky = false;   // a chart of the dark after the last sunrise: the sky from where the drifter was by its year
    if (out.mark.how == 4) {
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g0, L.which);
        const int year = C && index >= 0 && index < C->count ? C->entries[index].year : 1;
        int onset = 0, lastSunrise = 0;
        if (sunLossYears(g0, L.which, onset, lastSunrise)) {
            if (sunDarkOf(sunLightOf(onset, lastSunrise, year), year, lastSunrise) <= 2) {   // the dimming and the going: the sun's measure
                out.baseStyle = out.style; out.style = CHS_SUNMEASURE;
                static const int UNITS[5] = {60, 100, 120, 32, 24};   // the observatory's table's (`@suntable`, peoples/genres.cpp)
                out.sunUnits = UNITS[mix64(g.seed ^ 0xD055AULL) % 5];
                const int from = onset - 1, to = std::max(onset, year), n = std::min(12, to - from + 1);   // from the last year of the whole sun
                for (int i = 0; i < n; i++) {
                    const int y = n == 1 ? to : from + (int)std::lround((double)(to - from) * i / (n - 1));
                    out.sunCurve.push_back({(float)y, (float)(out.sunUnits * std::sqrt(sunLightOf(onset, lastSunrise, y)))});
                }
                out.centre = Vec3(0, 0, 1); out.up = Vec3(0, 1, 0); out.right = Vec3(1, 0, 0);
                ChartStar cs; cs.star = out.mark.star; cs.dir = out.centre; cs.mark = true; cs.bright = 1;
                out.markStar = 0; out.stars.push_back(cs);
                out.valid = true;
                return true;
            }
            out.from = drifterThen(sys0, sys.star, year - lastSunrise);
            out.mark.ly = length(out.from - out.mark.star.pos) / SECTOR_KM;
            if (out.style == CHS_ROUTES || out.style == CHS_BEYOND) out.style = CHS_SURVEY;   // the late ages' sky surveyed from the dark, not a map
            lostSky = true;
        }
    }
    Rng rs(mix64(out.seed ^ 0x45C4A1ULL));   // the chart's own draws (its plate, its people's units)
    out.centre = normalize(out.mark.star.pos - out.from);
    Vec3 pole = std::fabs(out.centre.y) < 0.98 ? Vec3(0, 1, 0) : Vec3(0, 0, 1);
    out.up = normalize(pole - out.centre * dot(pole, out.centre));
    out.right = cross(out.up, out.centre);
    std::vector<ChartStar> sky; chartSkyOf(sys.star, sky);
    if (lostSky) for (ChartStar& cs : sky) {   // the same sky, its figures theirs, seen from where the drifter was
        const Vec3 d = cs.star.pos - out.from; const double d2 = std::max(length2(d) / (SECTOR_KM * SECTOR_KM), 1e-6);
        cs.dir = normalize(d); cs.bright = cs.star.luminosity / d2;
    }
    if (out.style == CHS_ROUTES) {
        // the map of their neighbourhood (the galaxy's heart up): their star in the middle, the brightest within CHART_MAP_LY and every one a
        // route reaches; the routes from their star to the star this chart marks and to the worlds of the peoples they knew, the near
        // stars and the marks of their other charts within reach, flown or only meant, the far ones by a star on the way
        const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX * SECTOR_KM - out.from.x, 0, GALAXY_CENTRE_SZ * SECTOR_KM - out.from.z));
        const Vec3 east(toCore.z, 0, -toCore.x);
        auto place = [&](ChartStar& cs) {
            const Vec3 rel = (cs.star.pos - out.from) / SECTOR_KM;
            cs.ly = length(rel); cs.x = dot(rel, east) / CHART_MAP_LY; cs.y = dot(rel, toCore) / CHART_MAP_LY; cs.rise = rel.y / CHART_MAP_LY;
            cs.dir = cs.ly > 1e-9 ? rel / cs.ly : Vec3(0, 0, 1); cs.ang = 0;
        };
        { ChartStar h; h.star = sys.star; h.home = true; h.bright = 1e9; place(h); out.stars.push_back(h); }
        std::vector<int> idxOf;   // the sky's index -> the chart's
        for (size_t k = 0; k < sky.size() && out.stars.size() < 60; k++) {
            ChartStar cs = sky[k]; place(cs);
            if (cs.ly > CHART_MAP_LY) continue;
            out.stars.push_back(cs);
        }
        auto starOf = [&](const Star& st) {   // the chart's index of a star, added when it is not on it (a far one lies past the map's edge)
            for (size_t k = 0; k < out.stars.size(); k++) if (out.stars[k].star.seed == st.seed) return (int)k;
            ChartStar cs; cs.star = st; cs.bright = 0; place(cs); out.stars.push_back(cs); return (int)out.stars.size() - 1;
        };
        std::vector<int> dest;
        dest.push_back(starOf(out.mark.star)); out.markStar = dest[0]; out.stars[dest[0]].mark = true;
        for (const ChartMark& m : L.marks) { if ((int)dest.size() >= 5) break; if (m.how == 3 || m.ly > 30) continue; int k = starOf(m.star); if (std::find(dest.begin(), dest.end(), k) == dest.end()) dest.push_back(k); }
        {   // the nearest stars as the first crossings
            std::vector<std::pair<double, int>> nearest;
            for (size_t k = 1; k < out.stars.size(); k++) if (out.stars[k].ly <= CHART_MAP_LY) nearest.push_back({out.stars[k].ly, (int)k});
            std::sort(nearest.begin(), nearest.end());
            for (size_t i = 0; i < nearest.size() && (int)dest.size() < 6 && i < 4; i++) if (std::find(dest.begin(), dest.end(), nearest[i].second) == dest.end() && rs.chance(0.7)) dest.push_back(nearest[i].second);
        }
        std::vector<std::string> names;
        std::set<std::string> heads;
        for (int d : dest) {
            ChartFigure f; f.kind = 1;
            f.stars.push_back(0);
            const ChartStar& D = out.stars[d];
            if (D.ly > 7) {   // a far one by a star on the way: the map's star nearest the line's middle, within two light years of the line
                int via = -1; double best = 2.0;
                const Vec3 a(0, 0, 0), e = (D.star.pos - out.from) / SECTOR_KM;
                for (size_t k = 1; k < out.stars.size(); k++) {
                    if ((int)k == d) continue;
                    const Vec3 p = (out.stars[k].star.pos - out.from) / SECTOR_KM;
                    const double t = dot(p - a, e) / std::max(1e-9, dot(e, e));
                    if (t < 0.25 || t > 0.75) continue;
                    const double off = length(p - e * t);
                    if (off < best) { best = off; via = (int)k; }
                }
                if (via >= 0) f.stars.push_back(via);
            }
            f.stars.push_back(d);
            for (size_t k = 1; k < f.stars.size(); k++) f.lines.push_back({f.stars[k - 1], f.stars[k]});
            f.label = d;
            f.planned = !rs.chance(d == out.markStar ? 0.6 : 0.5);
            for (int tries = 0; tries < 60; tries++) {
                const std::string way = ROUTE_WAYS[rs.irange(countOf(ROUTE_WAYS))], noun = ROUTE_NOUNS[rs.irange(countOf(ROUTE_NOUNS))];
                if (heads.count(way) || heads.count(noun)) continue;
                heads.insert(way); heads.insert(noun);
                f.name = "the " + way + " " + noun; break;
            }
            if (f.name.empty()) continue;
            (f.planned ? out.planned : out.reached)++;
            out.figures.push_back(f);
        }
        out.valid = true;
        return true;
    }
    std::vector<ChartFigure> figures; skyFiguresOf(g, sky, figures);
    const int keep = out.style == CHS_BONE ? 45 : CHART_BRIGHT;   // a bone holds the brightest it could scratch
    std::vector<int> onChart(std::min((int)sky.size(), CHART_BRIGHT), -1);   // a sky star's index on the chart
    for (size_t k = 0; k < sky.size() && (int)k < CHART_BRIGHT; k++) {
        ChartStar cs = sky[k];
        cs.ang = std::acos(clampd(dot(cs.dir, out.centre), -1, 1));
        if (cs.ang > CHART_FIELD) continue;
        const bool isMark = cs.star.seed == out.mark.star.seed;
        if ((int)out.stars.size() >= keep && !isMark) continue;
        chartProject(out, cs.dir, cs.x, cs.y);
        cs.ly = length(cs.star.pos - out.from) / SECTOR_KM;
        if (isMark) { cs.mark = true; out.markStar = (int)out.stars.size(); }
        onChart[k] = (int)out.stars.size();
        out.stars.push_back(cs);
    }
    if (out.markStar < 0) {
        ChartStar cs; cs.star = out.mark.star; cs.dir = out.centre; cs.ang = 0; cs.x = 0; cs.y = 0; cs.mark = true;
        double d2 = length2(cs.star.pos - out.from) / (SECTOR_KM * SECTOR_KM); cs.bright = cs.star.luminosity / std::max(d2, 1e-6); cs.ly = std::sqrt(d2);
        out.markStar = (int)out.stars.size(); out.stars.push_back(cs);
    }
    if (out.style != CHS_BEYOND)
        for (const ChartFigure& f : figures) {
            bool whole = true;
            for (int k : f.stars) if (onChart[k] < 0) whole = false;
            if (!whole) continue;
            ChartFigure cf; cf.name = f.name; cf.glyph = f.glyph; cf.label = onChart[f.label];
            for (int k : f.stars) cf.stars.push_back(onChart[k]);
            for (const auto& ln : f.lines) cf.lines.push_back({onChart[ln.first], onChart[ln.second]});
            out.figures.push_back(cf);
        }
    switch (out.style) {
        case CHS_CEILING: case CHS_BONE:   // the moon's turns counted beside them: the days of its month, else its moons in the year
            if (L.calendar == 1 && L.monthDays > 0) { out.tally = L.monthDays; out.tallyKind = 1; }
            else if (L.calendar == 2 && L.months > 0) { out.tally = L.months; out.tallyKind = 2; }
            break;
        case CHS_ASTROLABE: case CHS_PAGE: {   // their equator and tropics (the plate's), the ecliptic, the wanderers' paths
            const int host = b.parent >= 0 && sys.bodies[b.parent].type != PT_COMPANION ? b.parent : b.index;
            const Vec3 n = orbitNormalOf(sys.bodies[host]);
            const Vec3 n1 = normalize(cross(n, std::fabs(n.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0))), n2 = cross(n, n1);
            addCircle(out, CC_ECLIPTIC, 0, n, n1, n2, 0, 0, TAU, 180);
            if (out.style == CHS_ASTROLABE) {
                const double tilt = std::fabs(b.axialTilt);
                addCircle(out, CC_EQUATOR, 0, b.spinAxis, b.ref0, b.ref1, 0, 0, TAU, 180);
                if (tilt > 2 * DEG && tilt < 80 * DEG) { addCircle(out, CC_TROPIC, 1, b.spinAxis, b.ref0, b.ref1, tilt, 0, TAU, 180); addCircle(out, CC_TROPIC, -1, b.spinAxis, b.ref0, b.ref1, -tilt, 0, TAU, 180); }
            }
            addWanderers(out, sys, b);
            break;
        }
        case CHS_ATLAS: {   // the grid of their coordinates (their pole the world's axis, their hours from its own meridian) and the stars numbered
            static const int CIRCLES[8] = {360, 400, 240, 480, 300, 432, 600, 256}, HOURS[8] = {24, 20, 16, 12, 10, 30, 36, 18};
            Rng ru(mix64(g.seed ^ 0xA71A5ULL));   // the people's units, the same on every plate of theirs
            out.circle = CIRCLES[ru.irange(8)]; out.hours = HOURS[ru.irange(8)];
            out.plate = 1 + rs.irange(60);
            for (int k = -5; k <= 5; k++) addCircle(out, CC_DECLINATION, k * out.circle / 24.0, b.spinAxis, b.ref0, b.ref1, k * 15 * DEG, 0, TAU, 180);
            const int meridians = out.hours % 12 == 0 ? 12 : (out.hours % 10 == 0 ? 10 : 9);
            for (int k = 0; k < meridians; k++) addMeridian(out, (double)k * out.hours / meridians, b.spinAxis, b.ref0, b.ref1, k * TAU / meridians, 90);
            int numbered = 0;
            for (ChartStar& cs : out.stars) if (numbered < 24 && !cs.mark) cs.number = ++numbered;   // by brightness: the list is the brightest first
            break;
        }
        case CHS_PLATE: out.plate = 100 + rs.irange(900); break;
        case CHS_SURVEY: {   // the magnitudes in their steps, the peoples heard by radio within the field
            static const double STEPS[4] = {2.0, 2.5, 3.0, 4.0};
            Rng ru(mix64(g.seed ^ 0x5A4E1ULL));
            out.magStep = STEPS[ru.irange(4)];
            out.plate = 1 + rs.irange(800);
            double top = 0; for (const ChartStar& cs : out.stars) top = std::max(top, cs.bright);
            for (ChartStar& cs : out.stars) { cs.mag = cs.bright > 0 && top > 0 ? std::log(top / cs.bright) / std::log(out.magStep) : 0; out.magLimit = std::max(out.magLimit, (int)std::ceil(cs.mag)); }   // the field's own stars: a far people heard is fainter than the survey went
            for (const ChartMark& m : L.marks) {
                if (m.how != 2) continue;
                const Vec3 d = normalize(m.star.pos - out.from);
                double x, y; if (!chartProject(out, d, x, y)) continue;
                int k = -1; for (size_t i = 0; i < out.stars.size(); i++) if (out.stars[i].star.seed == m.star.seed) k = (int)i;
                if (k < 0) { ChartStar cs; cs.star = m.star; cs.dir = d; cs.ang = std::acos(clampd(dot(d, out.centre), -1, 1)); cs.x = x; cs.y = y; cs.ly = m.ly; double d2 = m.ly * m.ly; cs.bright = m.star.luminosity / std::max(d2, 1e-6); cs.mag = top > 0 ? std::log(top / std::max(cs.bright, 1e-12)) / std::log(out.magStep) : 0; out.stars.push_back(cs); k = (int)out.stars.size() - 1; }
                if (!out.stars[k].heard) { out.stars[k].heard = true; out.heardN++; }
            }
            break;
        }
        case CHS_BEYOND: {   // the far mark in the middle, the places where nothing is round it, each joined to it, named
            int k = 0; { std::vector<int> styles; chartStyles(g, g0, L.which, (int)L.marks.size(), styles); for (int j = 0; j < which; j++) if (styles[j] == CHS_BEYOND) k++; }
            out.farKind = chartFarKind(g, sys.star, k); out.farLy = out.mark.ly;
            const int n = 3 + rs.irange(4);
            std::set<std::string> heads;
            for (int i = 0; i < n; i++) {
                const double r = rs.range(0.25, 0.9), a = rs.uni() * TAU;   // where on the chart: a direction in the field
                const double ang = r * CHART_FIELD;
                const Vec3 dir = normalize(out.centre * std::cos(ang) + (out.right * std::cos(a) + out.up * std::sin(a)) * std::sin(ang));
                ChartStar cs; cs.sign = true; cs.dir = dir; cs.ang = ang; chartProject(out, dir, cs.x, cs.y);
                cs.ly = rs.range(20000, 150000); cs.star.pos = out.from + dir * (cs.ly * SECTOR_KM); cs.star.seed = mix64(out.seed ^ (0x5161ULL + (uint64_t)i));
                out.stars.push_back(cs);
                ChartFigure f; f.kind = 2; f.stars = {out.markStar, (int)out.stars.size() - 1}; f.lines = {{out.markStar, (int)out.stars.size() - 1}}; f.label = (int)out.stars.size() - 1;
                for (int tries = 0; tries < 60; tries++) {
                    const std::string head = SIGN_NOUNS[rs.irange(countOf(SIGN_NOUNS))], way = SIGN_WAYS[rs.irange(countOf(SIGN_WAYS))];
                    if (heads.count(head) || heads.count(way)) continue;
                    heads.insert(head); heads.insert(way); f.name = "the " + way + " " + head; break;
                }
                if (f.name.empty()) { out.stars.pop_back(); continue; }
                out.signs++;
                out.figures.push_back(f);
            }
            break;
        }
        default: break;
    }
    out.valid = true;
    return true;
}
