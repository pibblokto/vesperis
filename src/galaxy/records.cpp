// T-04 (2026-10-09): where the records lie (records.h)
#include "records.h"
#include "instruments.h"
#include "history.h"
#include "works.h"
#include "buildings.h"
#include "drainage.h"
#include "core/rng.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <mutex>
#include <set>

const char* const RECORD_MEDIUM_NAMES[RM_COUNT] = {"carved bone", "carved stone", "painted wall", "clay tablet", "scroll", "codex", "engraved plate", "wax cylinder",
                                                   "glass plate", "printed pages", "reel", "disc", "drive", "crystal", "something that hums"};
const char* const RECORD_MEDIUM_HUD[RM_COUNT] = {"A CARVED BONE", "A CARVED STONE", "A PAINTED WALL", "A CLAY TABLET", "A SCROLL", "A CODEX", "AN ENGRAVED PLATE",
                                                 "A WAX CYLINDER", "A GLASS PLATE", "PRINTED PAGES", "A REEL", "A DISC", "A DRIVE", "A CRYSTAL", "SOMETHING THAT HUMS"};
const char* const RECORD_CATEGORY_NAMES[RC_COUNT] = {"text", "music", "star chart"};

// ---- how many ----------------------------------------------------------------------------------------------------------------------

namespace {
std::mutex countMutex;
std::map<uint64_t, int> countCache;
int whichOf(const BodyGen& g, int people) { return people > 0 && peoplesOf(g) > 1 ? 1 : 0; }
}

// the age's middling count and its people's middling numbers (log10, the age worlds' near home), the numbers moving it by 14 either way
// and the people's own draw by 6: a prehistoric band's 80 to about 100, a medieval kingdom's 90 to 120, an industrial people's 110 to 140,
// a modern people's 135 to 165, a futuristic people's 150 to 190, the far future's 170 to 200
int recordCountOf(const BodyGen& g, int people) {
    const int which = whichOf(g, people);
    const uint64_t key = mix64(g.seed ^ (0x7040C0ULL + (uint64_t)which));
    {
        std::lock_guard<std::mutex> lk(countMutex);
        auto it = countCache.find(key);
        if (it != countCache.end()) return it->second;
    }
    static const double BASE[AGE_COUNT] = {86, 102, 126, 150, 170, 184};
    static const double REF[AGE_COUNT] = {6.3, 8.4, 9.4, 9.4, 9.3, 8.5};
    const PeopleAge pa = peopleAgeOf(g, which);
    const PeopleExtent ex = peopleExtentOf(g, which);
    const int a = clampi(pa.age, 0, AGE_COUNT - 1);
    const double size = clampd(std::log10(std::max(1.0, ex.peak)) - REF[a], -1, 1);
    const double own = unitFromHash(mix64(peopleGen(g, which).seed ^ 0x7040C1ULL)) * 2 - 1;
    const int n = clampi((int)std::lround(BASE[a] + 14 * size + 6 * own), RECORDS_MIN, RECORDS_MAX);
    std::lock_guard<std::mutex> lk(countMutex);
    if (countCache.size() > 512) countCache.clear();
    countCache[key] = n;
    return n;
}

int shardsOfWorld(const BodyGen& g) {
    const int n = peoplesOf(g);
    int total = 0;
    for (int p = 0; p < std::max(1, n); p++) total += recordCountOf(g, p);
    return total;
}

// ---- when: the years of the age the people reached, weighed by what the years left -----------------------------------------------------

int recordYear(const PeopleAge& pa, double u) {
    // B-414: every record was made in the age the people reached (its earlier ages' are gone with their towns; a late people's histories
    // tell those years: `genre.history`, galaxy/grammar/history.vg). The years take a record's chance by e in 1,500 years: the later the likelier
    const double TAU = 1500;
    const int L = std::max(1, pa.lastYear), s = ageFromOf(pa);
    if (L <= s) return L;
    // tau (exp(-(L+1-y)/tau) - exp(-(L+1-s)/tau)) = t, of the whole tau (1 - exp(-(L+1-s)/tau))
    const double e0 = std::exp(-(L + 1.0 - s) / TAU), t = clampd(u, 0, 1) * (1 - e0);
    const double y = L + 1.0 + TAU * std::log(e0 + t);
    return clampi((int)std::floor(y), s, L);
}

// ---- what: music and charts past the fifty by the age, the medium by the age ----------------------------------------------------

int recordAgeAt(const PeopleAge& pa, int year, double* f) {
    if (f) *f = 1;
    for (const Era& er : pa.eras)
        if (year >= er.start && year <= er.end) { if (f) *f = (year - er.start) / std::max(1.0, (double)(er.end - er.start)); return er.age; }
    return pa.age;
}

namespace {
// H-05: past C-02's fifty a record is music or a star chart by the age of its year: the age's share leaned by the people's own fifty
// (C-04's twelve to eighteen pieces against fifteen, C-10's three to five charts against four). The prehistoric sang and danced more
// than it marked; the medieval wrote; the industrial printed its songs and cut them in wax; the modern pressed discs; the futuristic
// and the far future charted their routes and what lies past them. Within an age the records are taken by a hash of their own, so the
// count of each is exact (an age's share of its records, not a coin per record)
const double MUSIC_SHARE[AGE_COUNT] = {0.36, 0.22, 0.25, 0.32, 0.24, 0.26};
const double CHART_SHARE[AGE_COUNT] = {0.09, 0.08, 0.07, 0.06, 0.12, 0.15};
struct Categories { std::vector<int8_t> cat; std::vector<int> chart; int charts = 0; };   // by index: RC_*, and the chart's index among the people's (-1 none)
std::mutex catsMutex;
std::vector<std::pair<uint64_t, std::shared_ptr<const Categories>>> catsCache;   // the last sixteen, the newest last
std::shared_ptr<const Categories> categoriesOf(const BodyGen& pg, int count) {
    const uint64_t key = mix64(pg.seed ^ 0x7040C8ULL ^ ((uint64_t)count << 40));
    {
        std::lock_guard<std::mutex> lk(catsMutex);
        for (size_t i = 0; i < catsCache.size(); i++)
            if (catsCache[i].first == key) { auto p = catsCache[i].second; catsCache.erase(catsCache.begin() + i); catsCache.push_back({key, p}); return p; }
    }
    auto K = std::make_shared<Categories>();
    K->cat.assign(std::max(0, count), RC_TEXT); K->chart.assign(std::max(0, count), -1);
    const int first = chartShardsOf(pg);
    for (int i = 0; i < std::min(count, SHARDS_PER_WORLD); i++) {
        if (shardIsMusic(pg, i)) K->cat[i] = RC_MUSIC;
        else { const int c = chartIndexOf(pg, i); if (c >= 0) { K->cat[i] = RC_CHART; K->chart[i] = c; } }
    }
    K->charts = first;
    if (count > SHARDS_PER_WORLD) {
        const PeopleAge pa = peopleAgeOf(pg, 0);   // the people's generator read as its people (R-413: `peopleAgeOf(g, which)`'s)
        const int lastYear = std::max(1, pa.lastYear);
        const double leanM = musicShardsOf(pg) / 15.0, leanC = chartShardsOf(pg) / 4.0;
        const uint64_t base = mix64(pg.seed ^ 0x5A4D5ULL);   // a record's seed, its first draw its share of the years (`recordsOf`)
        std::vector<std::pair<uint64_t, int>> byAge[AGE_COUNT];
        for (int i = SHARDS_PER_WORLD; i < count; i++) {
            Rng rng(base + (uint64_t)i * 0x9E3779B97F4A7C15ULL);
            const int year = std::min(recordYear(pa, rng.uni()), std::max(1, lastYear - 1));
            const int a = clampi(recordAgeAt(pa, year, nullptr), 0, AGE_COUNT - 1);
            byAge[a].push_back({mix64(pg.seed ^ 0x7040C9ULL ^ ((uint64_t)(i + 1) * 0x9E3779B97F4A7C15ULL)), i});
        }
        for (int a = 0; a < AGE_COUNT; a++) {
            std::vector<std::pair<uint64_t, int>>& v = byAge[a];
            std::sort(v.begin(), v.end());
            const int n = (int)v.size();
            const int m = std::min(n, (int)std::lround(n * MUSIC_SHARE[a] * leanM)), c = std::min(n - m, (int)std::lround(n * CHART_SHARE[a] * leanC));
            for (int k = 0; k < m; k++) K->cat[v[k].second] = RC_MUSIC;
            for (int k = m; k < m + c; k++) K->cat[v[k].second] = RC_CHART;
        }
        for (int i = SHARDS_PER_WORLD; i < count; i++) if (K->cat[i] == RC_CHART) K->chart[i] = K->charts++;   // C-10's first, then by the index
    }
    std::lock_guard<std::mutex> lk(catsMutex);
    catsCache.push_back({key, K});
    if (catsCache.size() > 16) catsCache.erase(catsCache.begin());
    return K;
}
}

bool recordIsMusic(const BodyGen& pg, int count, int index) {
    if (index < SHARDS_PER_WORLD) return shardIsMusic(pg, index);
    if (index >= count) return false;
    return categoriesOf(pg, count)->cat[index] == RC_MUSIC;
}
int recordChartIndex(const BodyGen& pg, int count, int index) {
    if (index < SHARDS_PER_WORLD) return chartIndexOf(pg, index);
    if (index >= count) return -1;
    return categoriesOf(pg, count)->chart[index];
}
int recordChartCount(const BodyGen& pg, int count) { return count <= SHARDS_PER_WORLD ? chartShardsOf(pg) : categoriesOf(pg, count)->charts; }
void recordChartRecords(const BodyGen& pg, int count, std::vector<int>& out) {
    out.assign(recordChartCount(pg, count), -1);
    std::shared_ptr<const Categories> K = categoriesOf(pg, std::max(count, SHARDS_PER_WORLD));
    for (int i = 0; i < (int)K->chart.size(); i++) if (K->chart[i] >= 0 && K->chart[i] < (int)out.size() && (i < SHARDS_PER_WORLD || i < count)) out[K->chart[i]] = i;
}
double recordMusicShare(int age) { return MUSIC_SHARE[clampi(age, 0, AGE_COUNT - 1)]; }
double recordChartShare(int age) { return CHART_SHARE[clampi(age, 0, AGE_COUNT - 1)]; }

namespace {
// the medium of a record of a category made in an age, `f` how far into its era (0 its start .. 1 its end): the early medieval years'
// tablets give way to codices, the modern years' printed pages and reels to drives
struct MediumW { int m; double w; };
int mediaOf(int age, int cat, double f, MediumW* c) {
    int n = 0;
    auto add = [&](int m, double w) { if (w > 0 && n < 6) c[n++] = {m, w}; };
    switch (clampi(age, 0, AGE_COUNT - 1)) {
        case AGE_PREHISTORIC:
            if (cat == RC_MUSIC) add(RM_BONE, 1);
            else if (cat == RC_CHART) { add(RM_PAINTED, 0.55); add(RM_BONE, 0.45); }
            else { add(RM_BONE, 0.45); add(RM_STONE, 0.35); add(RM_PAINTED, 0.2); }
            break;
        case AGE_MEDIEVAL:
            if (cat == RC_MUSIC) { add(RM_SCROLL, 0.5 * (1 - f) + 0.1); add(RM_CODEX, 0.5 * f + 0.3); }
            else if (cat == RC_CHART) { add(RM_PLATE, 0.45); add(RM_CODEX, 0.3); add(RM_PAINTED, 0.25); }
            else { add(RM_TABLET, 0.5 * (1 - f) + 0.02); add(RM_SCROLL, 0.35); add(RM_CODEX, 0.75 * f + 0.05); add(RM_STONE, 0.12); }
            break;
        case AGE_INDUSTRIAL:
            if (cat == RC_MUSIC) { add(RM_CYLINDER, 0.65); add(RM_PRINTED, 0.35); }
            else if (cat == RC_CHART) { add(RM_PRINTED, 0.55); add(RM_GLASS, 0.45); }
            else { add(RM_PRINTED, 0.55); add(RM_CODEX, 0.25); add(RM_CYLINDER, 0.2); }
            break;
        case AGE_MODERN:
            if (cat == RC_MUSIC) { add(RM_DISC, 0.55); add(RM_REEL, 0.4 * (1 - f) + 0.05); add(RM_DRIVE, 0.3 * f + 0.02); }
            else if (cat == RC_CHART) { add(RM_GLASS, 0.35 * (1 - f) + 0.05); add(RM_PRINTED, 0.3); add(RM_DRIVE, 0.3 * f + 0.1); }
            else { add(RM_PRINTED, 0.5 * (1 - f) + 0.1); add(RM_DISC, 0.15); add(RM_REEL, 0.2 * (1 - f) + 0.03); add(RM_DRIVE, 0.5 * f + 0.05); }
            break;
        case AGE_FUTURISTIC:
            if (cat == RC_MUSIC) { add(RM_DRIVE, 0.35); add(RM_CRYSTAL, 0.65); }
            else if (cat == RC_CHART) { add(RM_CRYSTAL, 0.65); add(RM_DRIVE, 0.35); }
            else { add(RM_DRIVE, 0.45); add(RM_CRYSTAL, 0.55); }
            break;
        default:
            if (cat == RC_MUSIC) { add(RM_HUM, 0.6); add(RM_CRYSTAL, 0.4); }
            else if (cat == RC_CHART) { add(RM_CRYSTAL, 0.5); add(RM_HUM, 0.5); }
            else { add(RM_CRYSTAL, 0.55); add(RM_HUM, 0.45); }
            break;
    }
    return n;
}
int mediumOf(int age, int cat, double f, uint64_t h) {
    MediumW c[6]; const int n = mediaOf(age, cat, f, c);
    double total = 0; for (int i = 0; i < n; i++) total += c[i].w;
    double u = unitFromHash(h) * total;
    for (int i = 0; i < n; i++) { u -= c[i].w; if (u < 0) return c[i].m; }
    return n > 0 ? c[n - 1].m : RM_STONE;
}

std::mutex catMutex;
std::vector<std::pair<uint64_t, std::shared_ptr<const RecordCatalogue>>> catCache;   // the last eight, the newest last
}

std::shared_ptr<const RecordCatalogue> recordsOf(const BodyGen& g, int people) {
    const int which = whichOf(g, people);
    const uint64_t key = mix64(g.seed ^ (0x7040C3ULL + (uint64_t)which));
    {
        std::lock_guard<std::mutex> lk(catMutex);
        for (size_t i = 0; i < catCache.size(); i++)
            if (catCache[i].first == key) { auto p = catCache[i].second; catCache.erase(catCache.begin() + i); catCache.push_back({key, p}); return p; }
    }
    const auto t0 = std::chrono::steady_clock::now();
    auto C = std::make_shared<RecordCatalogue>();
    const BodyGen pg = peopleGen(g, which);
    const PeopleAge pa = peopleAgeOf(g, which);
    C->people = which;
    C->count = recordCountOf(g, which);
    C->lastYear = std::max(1, pa.lastYear);
    C->last = lastShardOf(pg);
    C->entries.resize(C->count);
    const uint64_t base = mix64(pg.seed ^ 0x5A4D5ULL);   // C-02's: a shard's seed, its first draw its share of the years (`shardOf`)
    for (int i = 0; i < C->count; i++) {
        RecordEntry& e = C->entries[i];
        const uint64_t seed = base + (uint64_t)i * 0x9E3779B97F4A7C15ULL;
        Rng rng(seed);
        const double u = rng.uni();
        e.last = i == C->last;
        e.year = e.last ? C->lastYear : std::min(recordYear(pa, u), std::max(1, C->lastYear - 1));   // C-12: the last recording the latest of all
        double f = 1;
        e.age = recordAgeAt(pa, e.year, &f);
        e.category = recordIsMusic(pg, C->count, i) ? RC_MUSIC : (recordChartIndex(pg, C->count, i) >= 0 ? RC_CHART : RC_TEXT);
        if (e.category == RC_MUSIC) C->music++; else if (e.category == RC_CHART) C->charts++;
        e.medium = mediumOf(e.age, e.category, f, mix64(seed ^ 0x7040C4ULL));
    }
    C->buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::lock_guard<std::mutex> lk(catMutex);
    catCache.push_back({key, C});
    if (catCache.size() > 8) catCache.erase(catCache.begin());
    return C;
}

namespace {
std::mutex homeMutex;
std::vector<std::pair<uint64_t, std::shared_ptr<const RecordHomes>>> homeCache;
}
std::shared_ptr<const RecordHomes> recordHomesOf(const BodyGen& g, int people) {
    const int which = whichOf(g, people);
    const uint64_t key = mix64(g.seed ^ (0x7040C6ULL + (uint64_t)which));
    {
        std::lock_guard<std::mutex> lk(homeMutex);
        for (size_t i = 0; i < homeCache.size(); i++)
            if (homeCache[i].first == key) { auto p = homeCache[i].second; homeCache.erase(homeCache.begin() + i); homeCache.push_back({key, p}); return p; }
    }
    std::shared_ptr<const RecordCatalogue> C = recordsOf(g, which);
    std::shared_ptr<const History> H = historyOfWorld(g, which);
    auto R = std::make_shared<RecordHomes>();
    R->rank.assign(C->count, -1);
    if (H) {
        const BodyGen pg = peopleGen(g, which);
        int lastRank = H->lastPlace >= 0 && H->lastPlace < (int)H->places.size() ? H->places[H->lastPlace].plan : -1;
        std::vector<int> cand; std::vector<double> w;
        for (int i = 0; i < C->count; i++) {
            const int year = C->entries[i].year;
            if (C->entries[i].last) { R->rank[i] = lastRank; continue; }
            cand.clear(); w.clear(); double total = 0;
            for (const HistoryPlace& p : H->places) {
                if (p.plan < 0 || year < p.founded || year > p.ended) continue;
                const double wt = p.sclass == SC_CAPITAL || p.sclass == SC_MONUMENT ? 10.0 : (p.sclass == SC_CITY ? 4.0 : (p.sclass == SC_TOWN ? 1.5 : 1.0));
                cand.push_back(p.plan); w.push_back(wt); total += wt;
            }
            if (cand.empty()) { R->rank[i] = lastRank; continue; }
            double u = unitFromHash(mix64(pg.seed ^ 0x7040C7ULL ^ ((uint64_t)(i + 1) * 0x9E3779B97F4A7C15ULL))) * total;
            int pick = (int)cand.size() - 1;
            for (size_t k = 0; k < cand.size(); k++) { u -= w[k]; if (u < 0) { pick = (int)k; break; } }
            R->rank[i] = cand[pick];
        }
    }
    std::lock_guard<std::mutex> lk(homeMutex);
    homeCache.push_back({key, R});
    if (homeCache.size() > 8) homeCache.erase(homeCache.begin());
    return R;
}

bool recordMediumFits(int medium, int age, int category) {
    for (double f : {0.0, 0.5, 1.0}) { MediumW c[6]; const int n = mediaOf(age, category, f, c); for (int i = 0; i < n; i++) if (c[i].m == medium) return true; }
    return false;
}

// ---- where ---------------------------------------------------------------------------------------------------------------------------

namespace {
void toSettlement(const Building& b, double lx, double lz, double& x, double& z) {
    x = b.x + std::cos(b.heading) * lx + std::sin(b.heading) * lz;
    z = b.z - std::sin(b.heading) * lx + std::cos(b.heading) * lz;
}
// a building's weight for one of a settlement's records (C-03's, the ages' kinds added: a town hall and a temple a scriptorium's, a chief's
// longhouse, offices, a station's hut at a mast's foot)
double recordWeight(int k) {
    switch (k) {
        case BK_HALL: case BK_TEMPLE: case BK_CHURCH: case BK_TOWN_HALL: case BK_LONGHOUSE: return 2.0;
        case BK_TOWER: case BK_ROTUNDA: case BK_STATION: case BK_ARCOLOGY: case BK_HABITAT: case BK_SPIRE: case BK_DOLMEN: case BK_STONE_CIRCLE:
        case BK_OBSERVATORY: case BK_MILL: case BK_HEADFRAME: case BK_HIGHRISE: return 1.5;
        case BK_STELA: case BK_MENHIR: case BK_RING: case BK_WRECK: case BK_SPIKE: case BK_HALO: case BK_PILLARS: case BK_SHAFT: case BK_MONOLITH: case BK_MAST: return 1.2;
        case BK_HOUSE: case BK_HUT: case BK_TENT_RING: case BK_ROW: case BK_TENEMENT: case BK_VILLA: case BK_BLOCK: case BK_SUBURB: case BK_POD: case BK_STADIUM:
        case BK_FOCUS: return 1.0;
        default: return 0;
    }
}
// where an age kept its archive, the likeliest first (a prehistoric chief's longhouse; a temple's scriptorium, a palace's hall; a town hall's
// archive; offices, a broadcaster's mast; a spire's or a dome's gate hall; the far future's structure, the archive that keeps itself)
const std::vector<int>& archiveKinds(int age) {
    static const std::vector<int> K[AGE_COUNT] = {
        {BK_LONGHOUSE, BK_DOLMEN, BK_HUT},
        {BK_TEMPLE, BK_HALL, BK_ROTUNDA, BK_TOWER, BK_CHURCH, BK_HOUSE},
        {BK_TOWN_HALL, BK_TEMPLE, BK_CHURCH, BK_HALL, BK_ROTUNDA, BK_STATION, BK_OBSERVATORY, BK_HOUSE},
        {BK_HIGHRISE, BK_TOWN_HALL, BK_TEMPLE, BK_HALL, BK_BLOCK, BK_CHURCH, BK_ROTUNDA, BK_STATION, BK_HOUSE},
        {BK_SPIRE, BK_ARCOLOGY, BK_HABITAT, BK_POD, BK_RING},
        {BK_SPIKE, BK_HALO, BK_PILLARS, BK_SHAFT, BK_MONOLITH, BK_SPIRE, BK_HABITAT}};
    return K[clampi(age, 0, AGE_COUNT - 1)];
}
// where an age would keep its last words: the chief's longhouse, the temple's inner room, the town hall, the capital's mast, a spire's gate
// hall, the structure
const std::vector<int>& lastKinds(int age) {
    static const std::vector<int> K[AGE_COUNT] = {
        {BK_LONGHOUSE, BK_DOLMEN, BK_STONE_CIRCLE, BK_HUT, BK_MENHIR},
        {BK_TEMPLE, BK_HALL, BK_TOWER, BK_ROTUNDA, BK_CHURCH, BK_HOUSE},
        {BK_TOWN_HALL, BK_CHURCH, BK_TEMPLE, BK_HALL, BK_STATION, BK_ROTUNDA, BK_HOUSE, BK_ROW},
        {BK_MAST, BK_HIGHRISE, BK_TOWN_HALL, BK_TEMPLE, BK_HALL, BK_BLOCK, BK_HOUSE, BK_SUBURB},
        {BK_SPIRE, BK_ARCOLOGY, BK_HABITAT, BK_POD, BK_RING},
        {BK_SPIKE, BK_HALO, BK_PILLARS, BK_SHAFT, BK_MONOLITH, BK_SPIRE, BK_HABITAT}};
    return K[clampi(age, 0, AGE_COUNT - 1)];
}

// B-421: the drawn surface of a piece where an object lies on the floor (0.06 m over the ground at its foot): the segments its faces cross that
// height in, and whether a point lies inside them or within `near` of one (a drift of sand, a low heap of rubble, a bank, a hearth's stone: the
// pieces with no collider hid what lay under them)
struct FloorSlice { double x0, z0, x1, z1; };
bool sliceHolds(const std::vector<FloorSlice>& S, double x, double z, double near) {
    int c[4] = {0, 0, 0, 0};
    for (const FloorSlice& s : S) {
        const double dx = s.x1 - s.x0, dz = s.z1 - s.z0, L2 = dx * dx + dz * dz;
        const double t = L2 > 1e-12 ? clampd(((x - s.x0) * dx + (z - s.z0) * dz) / L2, 0, 1) : 0;
        if (std::hypot(s.x0 + dx * t - x, s.z0 + dz * t - z) < near) return true;
        if ((s.z0 > z) != (s.z1 > z)) { const double xx = s.x0 + (z - s.z0) * dx / dz; c[xx > x ? 0 : 1]++; }
        if ((s.x0 > x) != (s.x1 > x)) { const double zz = s.z0 + (x - s.x0) * dz / dx; c[zz > z ? 2 : 3]++; }
    }
    return (c[0] & 1) && (c[1] & 1) && (c[2] & 1) && (c[3] & 1);
}
void floorSliceOf(const Form& e, std::vector<FloorSlice>& out) {
    out.clear();
    const double h = 0.06;   // a wall's stub of a hand's height hid half a record
    if (e.absolute || e.y0 > h + 0.01 || formTop(e) < h || e.kind == FK_CABLE || formWalkable(e)) return;
    FormMesh m; formMesh(e, 0, Vec3(e.x, 0, e.z), e.y0 < 0.01 ? 1.0 : 0.0, m, 1e9);
    for (const FormMesh::Poly& P : m.polys) {
        const FormMesh::V* v = &m.v[P.first];
        double ix[16], iz[16]; int ni = 0;
        for (int k = 0; k < P.n && ni < 16; k++) {
            const Vec3& a = v[k].p; const Vec3& b = v[(k + 1) % P.n].p;
            const double da = a.y - h, db = b.y - h;
            if ((da < 0) == (db < 0)) continue;
            const double t = da / (da - db);
            ix[ni] = a.x + (b.x - a.x) * t; iz[ni] = a.z + (b.z - a.z) * t; ni++;
        }
        if (ni >= 2) out.push_back({ix[0], iz[0], ix[ni - 1], iz[ni - 1]});
    }
}

struct Placer {
    const BodyGen& g;
    const RuinSpec& r;
    const RecordCatalogue& C;
    std::vector<ShardSite>& out;
    Rng& rng;
    std::vector<Form> el;
    std::vector<FormDisc> discs;
    std::vector<std::vector<FloorSlice>> slices;   // B-421: the drawn pieces at the floor, of the building being furnished and its neighbours
    std::vector<FormMesh> heaps;                   // B-421: and the banks, heaps and barrows among them (their surface over a spot)
    int discsOf = -2;
    std::set<int> used;   // the records here (distinct within the place)
    const std::function<bool(double, double)>* wetFn = nullptr;   // B-421: the view's water, where it gives it
    Placer(const BodyGen& g_, const RuinSpec& r_, const Culture& c, const RecordCatalogue& C_, std::vector<ShardSite>& o, Rng& rg) : g(g_), r(r_), C(C_), out(o), rng(rg) { ruinElements(r, c, 0, el); }

    void collide(int bi) {   // B-415: every piece over its ground, a neighbour's too (a tower's top fallen across its room)
        if (discsOf == bi) return;
        discs.clear(); slices.clear(); heaps.clear();
        const Building& b = r.buildings[bi];
        const double reach = std::hypot(b.hw, b.hd) + 2;
        for (const Form& e : el) {
            if (!(e.building == bi || std::hypot(e.x - b.x, e.z - b.z) < reach + formReach(e))) continue;
            formColliders(e, discs);
            if (formWalkable(e)) { heaps.emplace_back(); formMesh(e, 0, Vec3(e.x, 0, e.z), 1.0, heaps.back(), 1e9); continue; }
            slices.emplace_back(); floorSliceOf(e, slices.back());
            if (slices.back().empty()) slices.pop_back();
        }
        discsOf = bi;
    }
    // B-421: a spot in water or at its edge: the view's water where it gives it (its rivers and lakes as drawn); else the sea's, the planet
    // function at the view's 16 m without the rivers (a frame cannot wait for their tiles), at the spot and half a cell round it, 0.3 m over the
    // water at least (a lighthouse's foot on its headland stood a record at the sea's edge; a city across a river stood buildings in it)
    bool wet(double x, double z) const {
        if (wetFn) return (*wetFn)(x, z);
        DrainageOff off;
        const double mPerRad = g.R * 1000.0, cosLat = std::max(std::cos(r.lat), 0.05);
        const bool seaWorld = kindHas(g.type, KT_SEA) || g.liquidLevel > -1e8;
        const double sea = g.liquidLevel > -1e8 ? g.liquidLevel : 0.0;
        for (int k = 0; k < 5; k++) {
            const double ex = x + (k == 1 ? 8 : (k == 2 ? -8 : 0)), nz = z + (k == 3 ? 8 : (k == 4 ? -8 : 0));
            const SurfaceSample q = sampleSurface(g, StarSystem::bodyFromLatLon(r.lat + nz / mPerRad, r.lon + ex / (mPerRad * cosLat)), 16.0);
            if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water + 0.3) || (seaWorld && q.height < sea + 0.3)) return true;
        }
        return false;
    }
    // a spot the explorer can stand at: `margin` clear of every piece standing on the ground, 0.45 m from the records already here (`apart`);
    // B-421: not inside a drawn piece at the floor (nor within 0.15 m of one), not under water
    bool clear(double x, double z, double margin, double apart = 0.45) const {
        for (const FormDisc& q : discs) { if (q.y0 > 1.0) continue; if (std::hypot(x - q.x, z - q.z) < q.r + margin) return false; }
        for (const ShardSite& o : out) if (std::hypot(o.x - x, o.z - z) < apart) return false;
        for (const auto& S : slices) if (sliceHolds(S, x, z, 0.3)) return false;
        for (const FormMesh& m : heaps)   // on a bank's, a heap's or a barrow's ground, or within a stride of its toe (the view stands a big one on the
            for (int k = 0; k < 5; k++)   // highest ground under it: its toe over a spot beside it on a slope)
                if (formSupportAt(m, x + (k == 1 ? 0.5 : (k == 2 ? -0.5 : 0)), z + (k == 3 ? 0.5 : (k == 4 ? -0.5 : 0))) > -0.5) return false;
        return !wet(x, z);
    }
    // B-421: a room's doorway open: a run of a metre along its line clear of every piece's collider at the knee (a tower's top fallen across its
    // lobby's doorway shut the records laid in it; the A-03 check's rule)
    bool doorOpen(const BuildingRoom& room) const {
        if (!room.door) return true;
        int run = 0, best = 0;
        for (double t = -room.doorL; t <= room.doorL + 1e-9; t += 0.1) {
            const double x = room.doorX + room.doorAx * t, z = room.doorZ + room.doorAz * t;
            bool blocked = false;
            for (const FormDisc& q : discs) if (q.y0 < 1.0 && q.y1 > 0.6 && std::hypot(q.x - x, q.z - z) < q.r + 0.35) { blocked = true; break; }
            run = blocked ? 0 : run + 1; best = std::max(best, run);
        }
        return best * 0.1 >= 1.0;
    }
    // a record drawn from `pool` that is not here yet (-1 none), music or a chart first where `want` asks it
    int draw(const std::vector<int>& pool, int want = -1) {
        if (want >= 0) {
            std::vector<int> c; for (int i : pool) if (!used.count(i) && C.entries[i].category == want) c.push_back(i);
            if (!c.empty()) return c[rng.irange((int)c.size())];
        }
        for (int tries = 0; tries < 40 && !pool.empty(); tries++) { const int i = pool[rng.irange((int)pool.size())]; if (!used.count(i)) return i; }
        for (int i : pool) if (!used.count(i)) return i;
        return -1;
    }
    ShardSite site(int bi, const BuildingRoom& room, double x, double z, int local) {
        ShardSite s;
        s.building = bi; s.place = r.buildings[bi].kind; s.outside = room.outside; s.x = x; s.z = z;
        s.heading = r.buildings[bi].heading + rng.sym(0.5);
        s.index = shardWorldIndex(r.people, local); s.medium = C.entries[local].medium; s.last = C.entries[local].last;
        return s;
    }
    // a painted record: on the face of the wall a line from it away from the doorway meets (the first of the building's wall boxes standing 1.4 m
    // and more), the painting centred on that face within its length, the site the floor 0.75 m before it; else (no wall: a foot, a stub) on a
    // slab on the ground
    void paint(ShardSite& s, int bi, const BuildingRoom& room) const {
        if (s.medium != RM_PAINTED || room.outside || !room.door) return;
        double dx = s.x - room.doorX, dz = s.z - room.doorZ; const double L = std::hypot(dx, dz);
        if (L < 0.3) return;
        dx /= L; dz /= L;
        double bestT = 6.0; const Form* hitBox = nullptr; double hitLx = 0, hitSide = 1;
        for (const Form& e : el) {
            if (e.building != bi || e.kind != FK_BOX || e.y0 > 0.6 || e.y1 < 1.4 || std::fabs(e.tilt) > 0.25 || e.hx < 0.45 || e.hz > 1.5) continue;
            const double ca = std::cos(e.heading), sa = std::sin(e.heading);
            const double qx = s.x - e.x, qz = s.z - e.z;
            const double ox = qx * ca - qz * sa, oz = qx * sa + qz * ca, ux = dx * ca - dz * sa, uz = dx * sa + dz * ca;   // the line in the box's frame
            if (std::fabs(uz) < 1e-6) continue;
            const double side = oz > 0 ? 1.0 : -1.0;   // the face toward the line's start
            const double t = (side * e.hz - oz) / uz;
            if (t < 0.15 || t >= bestT) continue;
            const double lx = ox + ux * t;
            if (std::fabs(lx) > e.hx) continue;
            bestT = t; hitBox = &e; hitLx = lx; hitSide = side;
        }
        if (!hitBox) return;
        const Form& e = *hitBox;
        const double ca = std::cos(e.heading), sa = std::sin(e.heading);
        const double lx = clampd(hitLx, -e.hx + 0.45, e.hx - 0.45), lz = hitSide * (e.hz + 0.025);
        const double fx = e.x + ca * lx + sa * lz, fz = e.z - sa * lx + ca * lz;   // the painting's middle, on the face
        const double nx = hitSide * sa, nz = hitSide * ca;                         // the face's normal, into the room
        const double sx = fx + nx * 0.75, sz = fz + nz * 0.75;
        if (!clear(sx, sz, 0.3)) return;
        s.x = sx; s.z = sz; s.heading = std::atan2(nx, nz); s.wallD = 0.75; s.wallTop = e.y1;
        s.wallTilt = hitSide * e.tilt;   // a leaning wall (a hut's): the painting leans with it, toward the room where it does
    }
    // one record in a building's room: its first clear spot, 1.5 m from the doorway
    bool one(int bi, int local) {
        const Building& b = r.buildings[bi];
        BuildingRoom room;
        if (local < 0 || !buildingRoom(b, room)) return false;
        collide(bi);
        if (!doorOpen(room)) return false;   // its doorway under something fallen: none laid where none can go
        for (const auto& sp : room.spots) {
            double x, z; toSettlement(b, sp.first, sp.second, x, z);
            if (room.door && std::hypot(x - room.doorX, z - room.doorZ) < 1.5) continue;
            if (!clear(x, z, 0.42)) continue;
            ShardSite s = site(bi, room, x, z, local);
            paint(s, bi, room);
            used.insert(local); out.push_back(s);
            return true;
        }
        return false;
    }
    // an archive: `k` records, the place's own first (`home`, as many as fit; a painted one is left for a wall), in rows across the middle of a
    // building's room (square to its doorway's line, half a metre apart, the rows 0.6 m apart toward its back), or in rings round a structure's foot.
    // B-426: `rest`, the place's own its rooms could not hold, laid the same way after what lies there (from `home` alone, a painted one as a slab
    // where a slab has room), an archive's rows only where the building is the place's archive (`isArchive`)
    int archive(int bi, int k, std::vector<int>& home, const std::vector<int>& pool, bool rest = false, bool isArchive = true) {
        const Building& b = r.buildings[bi];
        BuildingRoom room;
        if (!buildingRoom(b, room)) return 0;
        collide(bi);
        if (!doorOpen(room)) return 0;
        double cx, cz; toSettlement(b, room.cx, room.cz, cx, cz);
        if (room.outside && !room.spots.empty()) toSettlement(b, room.spots[0].first, room.spots[0].second, cx, cz);
        double dx = std::sin(b.heading + PI), dz = std::cos(b.heading + PI);   // the back, without a doorway
        if (room.door) { dx = cx - room.doorX; dz = cz - room.doorZ; const double L = std::hypot(dx, dz); if (L > 0.2) { dx /= L; dz /= L; } }
        double bx = cx, bz = cz;
        if (!room.outside && (room.round ? room.r : std::min(room.hw, room.hd)) > 2.5) { bx += dx * 0.6; bz += dz * 0.6; }   // a step past the middle, away from the door
        const double ax = dz, az = -dx;   // square to the doorway's line
        auto next = [&]() {   // the place's own first, then any not here yet
            for (int t = (int)home.size() - 1; t >= 0; t--) {
                const int i = home[t];
                if (used.count(i)) { home.erase(home.begin() + t); continue; }
                if (C.entries[i].medium == RM_PAINTED) continue;
                home.erase(home.begin() + t); return i;
            }
            int local = draw(pool);
            for (int t = 0; t < 6 && local >= 0 && C.entries[local].medium == RM_PAINTED; t++) local = draw(pool);   // a row of objects
            return local >= 0 && C.entries[local].medium == RM_PAINTED ? -1 : local;
        };
        int placed = 0;
        auto put = [&](double x, double z, double heading) {
            if (room.door && std::hypot(x - room.doorX, z - room.doorZ) < 1.5) return true;
            if (!room.outside) {   // on the room's floor
                const double qx = x - b.x, qz = z - b.z;
                const double lx = qx * std::cos(b.heading) - qz * std::sin(b.heading), lz = qx * std::sin(b.heading) + qz * std::cos(b.heading);
                if (room.round ? std::hypot(lx - room.cx, lz - room.cz) > room.r - 0.5 : (std::fabs(lx - room.cx) > room.hw - 0.5 || std::fabs(lz - room.cz) > room.hd - 0.5)) return true;
            }
            if (!clear(x, z, 0.3)) return true;
            int local = -1;
            if (rest) {   // the last of the place's own, a painted one where its slab has room (else the last of the others; none blocks the rest)
                for (int t = (int)home.size() - 1; t >= 0; t--) if (used.count(home[t])) home.erase(home.begin() + t);
                if (home.empty()) return false;
                bool slabNear = false;
                for (const ShardSite& o : out) if (o.medium == RM_PAINTED && o.wallD <= 0 && std::hypot(o.x - x, o.z - z) < 0.95) slabNear = true;
                const bool slabFits = !slabNear && clear(x, z, 0.6, 0.95);
                int t = (int)home.size() - 1;
                while (t >= 0 && C.entries[home[t]].medium == RM_PAINTED && !slabFits) t--;
                if (t < 0 || (slabNear && C.entries[home[t]].medium != RM_PAINTED)) return true;
                local = home[t]; home.erase(home.begin() + t);
            } else {
                local = next();
                if (local < 0) return false;
            }
            ShardSite s = site(bi, room, x, z, local);
            s.archive = isArchive ? 1 : 0; s.heading = heading + rng.sym(0.12);
            used.insert(local); out.push_back(s); placed++;
            return true;
        };
        if (room.outside) {   // rings round the structure, from its foot outward
            const double R0 = std::max(1.5, std::hypot(cx - b.x, cz - b.z)), a0 = std::atan2(cx - b.x, cz - b.z);
            for (int q = 0; q < (rest ? 16 : 8) && placed < k; q++) {
                const double R = R0 + 0.8 * q; const int n = std::max(6, (int)(TAU * R / 0.55));
                for (int j = 0; j < n && placed < k; j++) {
                    const double a = a0 + TAU * j / n;
                    if (!put(b.x + std::sin(a) * R, b.z + std::cos(a) * R, a + PI / 2)) return placed;
                }
            }
            return placed;
        }
        for (int q = 0; q < (rest ? 12 : 4) && placed < k; q++)
            for (int j = 0; j < 24 && placed < k; j++) {
                const double off = ((j + 1) / 2) * 0.5 * ((j & 1) ? 1 : -1);
                if (!put(bx - dx * 0.6 * q + ax * off, bz - dz * 0.6 * q + az * off, std::atan2(ax, az))) return placed;
            }
        return placed;
    }
    // C-11: a spot for an instrument beside a record in its room: on rings round it (0.75, 1.1, 1.45 m), clear of the pieces standing, the
    // records and the instruments here, within the room's walls and off its doorway; else the room's own spots
    bool beside(const ShardSite& s, const std::vector<InstrumentSite>& placed, Rng& irng, double& ox, double& oz) {
        const Building& b = r.buildings[s.building];
        BuildingRoom room;
        if (!buildingRoom(b, room)) return false;
        collide(s.building);
        auto ok = [&](double x, double z) {
            if (room.door && std::hypot(x - room.doorX, z - room.doorZ) < 1.3) return false;
            if (!room.outside) {
                const double qx = x - b.x, qz = z - b.z;
                const double lx = qx * std::cos(b.heading) - qz * std::sin(b.heading), lz = qx * std::sin(b.heading) + qz * std::cos(b.heading);
                if (room.round ? std::hypot(lx - room.cx, lz - room.cz) > room.r - 0.55 : (std::fabs(lx - room.cx) > room.hw - 0.55 || std::fabs(lz - room.cz) > room.hd - 0.55)) return false;
            }
            if (!clear(x, z, 0.45, 0.7)) return false;   // B-421: its body beside a record, not over it (an archive's rows were 0.45 m off)
            for (const InstrumentSite& q : placed) if (std::hypot(q.x - x, q.z - z) < 0.9) return false;
            return true;
        };
        const double a0 = irng.uni() * TAU;
        for (int ring = 0; ring < 3; ring++)
            for (int k = 0; k < 8; k++) {
                const double a = a0 + k * TAU / 8 + ring * 0.4, d = 0.75 + 0.35 * ring;
                const double x = s.x + std::sin(a) * d, z = s.z + std::cos(a) * d;
                if (ok(x, z)) { ox = x; oz = z; return true; }
            }
        for (const auto& sp : room.spots) { double x, z; toSettlement(b, sp.first, sp.second, x, z); if (std::hypot(x - s.x, z - s.z) > 0.6 && ok(x, z)) { ox = x; oz = z; return true; } }
        return false;
    }
    // the first building of the kinds' order present here with a room (the biggest of its kind)
    int firstOf(const std::vector<int>& kinds, const std::set<int>& skip) const {
        for (int k : kinds) {
            int best = -1; double area = -1;
            for (size_t i = 0; i < r.buildings.size(); i++) {
                const Building& b = r.buildings[i];
                if (b.kind != k || skip.count((int)i)) continue;
                BuildingRoom room; if (!buildingRoom(b, room)) continue;
                if (b.hw * b.hd > area) { area = b.hw * b.hd; best = (int)i; }
            }
            if (best >= 0) return best;
        }
        return -1;
    }
};
}   // namespace

// A settlement: the last recording first where this is the history's last place (in the building its age would keep it in), then the
// archive (the capital's, a city's, some towns'; a prehistoric people's in its chief's longhouse, the far future's round its structure),
// then C-03's few by the class, in buildings drawn by their weight (not the archive's), each from the records made before the place was
// left. A work: the works that kept records (the last shelter, an observatory, a necropolis, a shrine, standing stones, a station, a launch
// site's blockhouse, a lighthouse, B-418: a fort; D-05: a dome over the warm ground, a shelter of a place left before the end), a few each in
// its buildings' rooms or at their feet
void recordSitesOf(const BodyGen& gSeason, const RuinSpec& r, const Culture& c, std::vector<ShardSite>& out, std::vector<InstrumentSite>* instruments,
                   const std::function<bool(double, double)>* wet) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time) (B-421: its water read at a spot)
    out.clear();
    if (instruments) instruments->clear();
    if (r.buildings.empty()) return;
    std::shared_ptr<const RecordCatalogue> Cp = recordsOf(g, r.people);
    if (!Cp || Cp->count <= 0) return;
    const RecordCatalogue& C = *Cp;
    // the history where it is needed (a place of the plan, a work: both read it already); never for the countryside's cells, which a frame
    // may build before the plan is ready
    std::shared_ptr<const History> H = r.rank >= 0 || r.sclass == SC_WORKS ? historyOfWorld(g, r.people) : nullptr;
    Rng rng(r.seed ^ 0x7040C5ULL);
    const int endYear = C.lastYear - (int)std::lround(std::max(0.0, r.before));   // a place left early holds nothing written after
    std::vector<int> pool;
    for (int i = 0; i < C.count; i++) if (i != C.last && C.entries[i].year <= endYear) pool.push_back(i);   // none for the old time's stones (left before year 1)
    Placer P(g, r, c, C, out, rng);
    P.wetFn = wet;
    const int age = clampi(r.buildAge >= 0 ? r.buildAge : c.age, 0, AGE_COUNT - 1);
    std::set<int> taken;   // the buildings already holding one

    if (r.sclass == SC_WORKS) {
        int want = 0, cat = -1;
        switch (r.work) {
            case WO_SHELTER: {   // the last recording and the last ones kept with it, of the collapse's years
                // D-05: a shelter of a place left before the end (a people that lost its sun's, `before` its years) the last of its own years alone
                const bool last = r.before <= 0;
                pool.clear();
                const int from = H ? H->stageYear[STAGE_WARNING] : C.lastYear - std::max(5, C.lastYear / 30);
                for (int i = 0; i < C.count; i++) if (i != C.last && C.entries[i].year >= from && (last || C.entries[i].year <= endYear)) pool.push_back(i);
                if (pool.size() < 3) { std::vector<int> late; for (int i = 0; i < C.count; i++) if (i != C.last && (last || C.entries[i].year <= endYear)) late.push_back(i);
                                       std::sort(late.begin(), late.end(), [&](int a, int b) { return C.entries[a].year > C.entries[b].year; });
                                       for (int i = 0; i < (int)late.size() && pool.size() < 3; i++) if (std::find(pool.begin(), pool.end(), late[i]) == pool.end()) pool.push_back(late[i]); }
                if (last) for (size_t i = 0; i < r.buildings.size() && C.last >= 0; i++) if (P.one((int)i, C.last)) { taken.insert((int)i); break; }
                want = 1 + rng.irange(3);
                break;
            }
            case WO_DOME: want = 1 + rng.irange(2); break;   // D-05: of the years they lived under it
            case WO_OBSERVATORY: want = 1 + rng.irange(3); cat = RC_CHART; break;
            case WO_NECROPOLIS: want = 1 + rng.irange(3); break;
            case WO_SHRINE: want = 1 + rng.irange(2); break;
            case WO_HENGE: case WO_ANTENNA: case WO_LAUNCH: case WO_LIGHTHOUSE: case WO_FORT: want = 1; break;   // B-418: a fort's keep, its casemate
            default: return;
        }
        std::vector<int> cand; std::vector<double> wt;
        for (size_t i = 0; i < r.buildings.size(); i++) {
            if (taken.count((int)i)) continue;
            BuildingRoom room; if (!buildingRoom(r.buildings[i], room)) continue;
            cand.push_back((int)i); wt.push_back(room.outside ? 1.0 : 3.0);   // a work's room first (the dome, the keep, the keeper's house)
        }
        for (int n = 0; n < want && !cand.empty();) {
            const int pick = rng.pick(wt.data(), (int)wt.size());
            const int bi = cand[pick]; cand.erase(cand.begin() + pick); wt.erase(wt.begin() + pick);
            if (P.one(bi, P.draw(pool, cat))) n++;
        }
        return;
    }

    // the place's own records (a place of the plan: the records whose home it is, `recordHomesOf`), which it holds whatever else it holds
    std::vector<int> home;
    if (r.rank >= 0) {
        std::shared_ptr<const RecordHomes> Hm = recordHomesOf(g, r.people);
        for (int i = 0; i < C.count && i < (int)Hm->rank.size(); i++) if (i != C.last && Hm->rank[i] == r.rank && C.entries[i].year <= endYear) home.push_back(i);
        for (int i = (int)home.size() - 1; i > 0; i--) std::swap(home[i], home[rng.irange(i + 1)]);
    }
    // the last recording at the history's last place (no history: the capital)
    if (r.rank >= 0 && C.last >= 0) {
        int lastRank = 0;
        if (H && H->lastPlace >= 0 && H->lastPlace < (int)H->places.size() && H->places[H->lastPlace].plan >= 0) lastRank = H->places[H->lastPlace].plan;
        if (r.rank == lastRank) {
            const int bi = P.firstOf(lastKinds(age), taken);
            if (bi >= 0 && P.one(bi, C.last)) taken.insert(bi);
            else for (size_t i = 0; i < r.buildings.size(); i++) if (!taken.count((int)i) && P.one((int)i, C.last)) { taken.insert((int)i); break; }
        }
    }
    // the archive
    int k = 0;
    if (r.sclass == SC_CAPITAL) k = age == AGE_PREHISTORIC ? 4 + rng.irange(3) : 8 + rng.irange(7);
    else if (r.sclass == SC_CITY) k = 5 + rng.irange(4);
    else if (r.sclass == SC_TOWN && age != AGE_PREHISTORIC && rng.chance(0.35)) k = 3 + rng.irange(3);
    else if (r.sclass == SC_MONUMENT && r.rank >= 0 && age == AGE_FAR_FUTURE) k = 6 + rng.irange(6);   // the archive that keeps itself
    int archiveAt = -1;
    if (k > 0) {   // B-426: its count, the place's own first (it took them all: a prehistoric capital's thirty on its longhouse's floor)
        const int bi = P.firstOf(archiveKinds(age), std::set<int>());   // where the last recording lies too, if that is its age's archive
        if (bi >= 0 && P.archive(bi, k, home, pool) > 0) { taken.insert(bi); archiveAt = bi; }
    }
    // C-03's few by the class
    int want = 0;
    switch (r.sclass) {
        case SC_HAMLET: want = rng.chance(0.6) ? 1 : 0; break;
        case SC_VILLAGE: want = 1 + rng.irange(2); break;
        case SC_TOWN: want = 2 + rng.irange(3); break;
        case SC_CITY: want = 4 + rng.irange(4); break;
        case SC_CAPITAL: want = 6 + rng.irange(5); break;
        default: want = rng.chance(0.7) ? 1 : 0; break;
    }
    for (int i = (int)home.size() - 1; i >= 0; i--) if (P.used.count(home[i])) home.erase(home.begin() + i);
    want = std::max(want, (int)home.size());
    std::vector<int> cand; std::vector<double> wt;
    for (size_t i = 0; i < r.buildings.size(); i++) {
        const double w = recordWeight(r.buildings[i].kind);
        if (w > 0 && !taken.count((int)i)) { cand.push_back((int)i); wt.push_back(w); }
    }
    std::vector<int> held;   // the buildings holding one, for the place's own left over (a second in a room)
    for (int n = 0; n < want && !cand.empty();) {
        const int pick = rng.pick(wt.data(), (int)wt.size());
        const int bi = cand[pick]; cand.erase(cand.begin() + pick); wt.erase(wt.begin() + pick);
        const int local = !home.empty() ? home.back() : P.draw(pool);
        if (P.one(bi, local)) { n++; held.push_back(bi); if (!home.empty()) home.pop_back(); }
    }
    if (held.empty()) for (size_t i = 0; i < r.buildings.size(); i++) held.push_back((int)i);   // a structure alone: its foot
    for (size_t j = 0; !home.empty() && j < held.size() * 3; j++) if (P.one(held[j % held.size()], home.back())) home.pop_back();
    // B-426: the place's own its rooms' spots could not hold (a lone stone's foot holds two, a futuristic capital's pods two each): one at a time
    // beside what lies in each building holding one (across its room, round its foot), then after the archive's rows (the far future's structure:
    // its rings, the archive that keeps itself), so the home holds all of them
    for (int i = (int)home.size() - 1; i >= 0; i--) if (P.used.count(home[i])) home.erase(home.begin() + i);
    for (bool more = true; more && !home.empty();) {
        more = false;
        for (size_t j = 0; j < held.size() && !home.empty(); j++) if (held[j] != archiveAt && P.archive(held[j], 1, home, std::vector<int>(), true, false) > 0) more = true;
    }
    if (archiveAt >= 0 && !home.empty()) P.archive(archiveAt, (int)home.size(), home, std::vector<int>(), true, true);
    // C-11: the instruments beside the music records (`instrumentWithRecord`: by the record, so every copy of it has one), on a stream of their own
    if (instruments) {
        Rng irng(r.seed ^ 0xC11B5ULL);
        for (size_t i = 0; i < out.size(); i++) {
            const ShardSite s = out[i];
            const int local = shardLocalOf(s.index);
            if (local < 0 || local >= C.count || C.entries[local].category != RC_MUSIC || s.wallD > 0 || s.building < 0) continue;
            double pick = 0;
            if (!instrumentWithRecord(g.seed, r.people, local, pick)) continue;
            double x = 0, z = 0;
            if (!P.beside(s, *instruments, irng, x, z)) continue;
            InstrumentSite is; is.record = s.index; is.people = r.people; is.age = C.entries[local].age; is.pick = pick; is.building = s.building;
            is.x = x; is.z = z; is.heading = std::atan2(x - s.x, z - s.z) + PI / 2 + irng.sym(0.4);
            instruments->push_back(is);
        }
    }
}

// ---- the objects -----------------------------------------------------------------------------------------------------------------------

namespace {
struct Obj {
    std::vector<Form>& out;
    double x, z, h;   // the object's middle and its heading
    uint32_t seed;
    // a piece `across` metres across the heading and `along` along it from the middle, its half extents, its height span, its part (and slot)
    Form& add(int kind, double across, double along, double hx, double hz, double y0, double y1, int part, int slot = 0, double turn = 0) {
        Form f; f.kind = kind; f.x = x + std::cos(h) * across + std::sin(h) * along; f.z = z - std::sin(h) * across + std::cos(h) * along;
        f.heading = h + turn; f.hx = hx; f.hz = hz; f.y0 = y0; f.y1 = y1; f.part = part; f.slot = slot; f.seed = seed;
        out.push_back(f);
        return out.back();
    }
    // a cylinder lying along the heading, centred on `along`, of radius `rad` and length `len`
    void lying(double across, double along, double rad, double len, int part, int slot = 0) {
        Form& f = add(FK_LATHE, across, along - len * 0.5, rad, rad, 0, len, part, slot);
        f.tilt = PI / 2;
    }
};
// a painting's strokes, `u` along the wall and `v` up it (metres), on the wall's face or flat on a slab: a creature, hands, a sky
void paintStrokes(Obj& o, bool wall, double vLo, double vHi, uint32_t seed, double lean = 0) {
    Rng rng(seed ^ 0x9A1D7ULL);
    const double s = std::min(1.0, (vHi - vLo) / 0.75);
    auto stroke = [&](double u, double v, double hu, double hv, int part) {
        if (wall) {
            Form& f = o.add(FK_BOX, u, 0, hu, 0.012, vLo + (v - hv) * s, vLo + (v + hv) * s, part);
            f.tilt = lean;   // about the wall's foot, as the wall leans
        } else o.add(FK_BOX, u, (v - 0.5) * 0.8, hu, hv * 0.8, 0.1, 0.106, part);
    };
    const int scene = rng.irange(3);
    if (scene == 0) {   // creatures: a body, four legs, a head and a horn, a second smaller behind the first
        for (int n = 0; n < 2; n++) {
            const double k = n == 0 ? 1.0 : 0.62, cu = n == 0 ? -0.08 : 0.3 * (rng.chance(0.5) ? 1 : -1), cv = n == 0 ? 0.4 : 0.68;
            stroke(cu, cv, 0.22 * k, 0.06 * k, FORM_PART_OCHRE);
            for (int l = 0; l < 4; l++) stroke(cu - 0.17 * k + l * 0.11 * k, cv - 0.13 * k, 0.022 * k, 0.08 * k, FORM_PART_OCHRE);
            stroke(cu + 0.26 * k, cv + 0.06 * k, 0.055 * k, 0.045 * k, FORM_PART_OCHRE);
            stroke(cu + 0.29 * k, cv + 0.14 * k, 0.012 * k, 0.045 * k, FORM_PART_OCHRE);
        }
    } else if (scene == 1) {   // hands, pale among the ochre
        for (int n = 0; n < 5; n++) {
            const double u = -0.34 + n * 0.17 + rng.sym(0.03), v = 0.32 + rng.uni() * 0.3;
            const int part = n % 2 ? FORM_PART_PALE : FORM_PART_OCHRE;
            stroke(u, v, 0.06, 0.07, part);
            for (int f = 0; f < 4; f++) stroke(u - 0.045 + f * 0.03, v + 0.11, 0.01, 0.04, part);
            stroke(u + 0.08, v + 0.03, 0.025, 0.012, part);
        }
    } else {   // a sky: the points of its figures, the lines joining them
        double pu[9], pv[9];
        for (int n = 0; n < 9; n++) { pu[n] = rng.sym(0.36); pv[n] = 0.22 + rng.uni() * 0.56; stroke(pu[n], pv[n], 0.03, 0.03, n % 3 ? FORM_PART_PALE : FORM_PART_OCHRE); }
        for (int n = 0; n + 1 < 9; n += 2) {   // a horizontal and a vertical bar between neighbours (the strokes stand upright)
            stroke(0.5 * (pu[n] + pu[n + 1]), pv[n], std::fabs(pu[n + 1] - pu[n]) * 0.5, 0.01, FORM_PART_OCHRE);
            stroke(pu[n + 1], 0.5 * (pv[n] + pv[n + 1]), 0.01, std::fabs(pv[n + 1] - pv[n]) * 0.5, FORM_PART_OCHRE);
        }
    }
}
}   // namespace

void recordForms(const ShardSite& s, uint64_t seed, std::vector<Form>& out) {
    out.clear();
    Obj o{out, s.x, s.z, s.heading, (uint32_t)(seed ^ (seed >> 32))};
    Rng rng(seed ^ 0x0B1EC7ULL);
    switch (s.medium) {
        case RM_BONE:
            o.add(FK_BOX, 0, 0, 0.022, 0.13, 0, 0.034, FORM_PART_PALE);
            for (int e = -1; e <= 1; e += 2) o.add(FK_BOX, 0, e * 0.145, 0.04, 0.028, 0, 0.048, FORM_PART_PALE);
            for (int n = 0; n < 4; n++) o.add(FK_BOX, 0, -0.07 + n * 0.045, 0.023, 0.004, 0.034, 0.037, FORM_PART_DARK);
            break;
        case RM_STONE:
            o.add(FK_BOX, 0, 0, 0.17, 0.12, 0, 0.07, 3, 0);
            for (int n = 0; n < 3; n++) o.add(FK_BOX, 0, -0.06 + n * 0.06, 0.12, 0.007, 0.07, 0.074, FORM_PART_DARK);
            break;
        case RM_PAINTED:
            if (s.wallD > 0) {   // on the wall before the site: its face `wallD` back along the heading
                Obj w{out, s.x - std::sin(s.heading) * s.wallD, s.z - std::cos(s.heading) * s.wallD, s.heading, o.seed};   // `across` runs along the wall
                const double top = std::max(1.2, std::min(1.85, s.wallTop - 0.08));
                paintStrokes(w, true, 0.5, top, o.seed, s.wallTilt);
            } else {   // a painted slab on the ground
                o.add(FK_BOX, 0, 0, 0.42, 0.32, 0, 0.1, 3, 0);
                paintStrokes(o, false, 0, 0.75, o.seed);
            }
            break;
        case RM_TABLET:
            o.add(FK_BOX, 0, 0, 0.08, 0.11, 0, 0.034, FORM_PART_CLAY);
            for (int n = 0; n < 5; n++) o.add(FK_BOX, 0, -0.08 + n * 0.04, 0.062, 0.004, 0.034, 0.037, FORM_PART_DARK);
            break;
        case RM_SCROLL:
            o.lying(0, 0, 0.032, 0.3, FORM_PART_PALE);
            o.lying(0, 0, 0.013, 0.37, FORM_PART_DARK);
            break;
        case RM_CODEX:   // open on the floor: its boards and two blocks of pages
            o.add(FK_BOX, 0, 0, 0.2, 0.145, 0, 0.012, FORM_PART_DARK);
            for (int e = -1; e <= 1; e += 2) o.add(FK_BOX, e * 0.098, 0, 0.094, 0.135, 0.012, 0.03, FORM_PART_PALE);
            break;
        case RM_PLATE: {
            Form& d = o.add(FK_LATHE, 0, 0, 0.11, 0.11, 0, 0.008, FORM_PART_METAL); (void)d;
            Form& rim = o.add(FK_TORUS, 0, 0, 0.11, 0.11, 0, 0.012, FORM_PART_METAL); rim.p[0] = 0.007;
            o.add(FK_BOX, 0, 0, 0.008, 0.1, 0.008, 0.013, FORM_PART_METAL, 0, rng.uni() * PI);
            break;
        }
        case RM_CYLINDER:
            o.add(FK_LATHE, 0, 0, 0.028, 0.028, 0, 0.105, FORM_PART_DARK);
            o.lying(0.09, 0, 0.034, 0.115, FORM_PART_PALE);
            break;
        case RM_GLASS:
            o.add(FK_BOX, 0, -0.05, 0.105, 0.075, 0, 0.005, FORM_PART_PALE);
            o.add(FK_BOX, 0, 0.02, 0.1, 0.125, 0.005, 0.011, FORM_PART_DARK);
            break;
        case RM_PRINTED:
            o.add(FK_BOX, 0, 0, 0.15, 0.21, 0, 0.026, FORM_PART_PALE);
            o.add(FK_BOX, 0.01, 0.01, 0.15, 0.21, 0.026, 0.029, FORM_PART_PALE, 0, 0.18);
            for (int n = -1; n <= 1; n++) o.add(FK_BOX, n * 0.085, 0, 0.03, 0.17, 0.029, 0.031, FORM_PART_DARK, 0, 0.18);
            break;
        case RM_REEL:
            o.add(FK_LATHE, 0, 0, 0.09, 0.09, 0, 0.004, FORM_PART_METAL);
            o.add(FK_LATHE, 0, 0, 0.065, 0.065, 0.004, 0.026, FORM_PART_DARK);
            o.add(FK_LATHE, 0, 0, 0.09, 0.09, 0.026, 0.03, FORM_PART_METAL);
            break;
        case RM_DISC:
            o.add(FK_BOX, 0, 0, 0.155, 0.155, 0, 0.006, 3, 1);   // its sleeve in its people's accent
            o.add(FK_LATHE, 0, 0.07, 0.15, 0.15, 0.006, 0.009, FORM_PART_DARK);
            break;
        case RM_DRIVE:
            o.add(FK_BOX, 0, 0, 0.05, 0.075, 0, 0.02, FORM_PART_DARK);
            o.add(FK_BOX, 0, -0.01, 0.04, 0.055, 0.02, 0.022, FORM_PART_METAL);
            o.add(FK_BOX, 0.03, 0.06, 0.006, 0.006, 0.02, 0.025, 3, 3);   // its light (its people's, where they built with light)
            break;
        case RM_CRYSTAL: {   // a cluster: three hexagonal prisms on a dark foot, each with its point, the side ones leaning out
            Form& base = o.add(FK_LATHE, 0, 0, 0.07, 0.07, 0, 0.02, FORM_PART_DARK); base.sides = 6;
            for (int k = 0; k < 3; k++) {
                const double a = k * TAU / 3, d = k == 0 ? 0.0 : 0.035, len = k == 0 ? 0.2 : 0.13 + 0.03 * k, r0 = k == 0 ? 0.032 : 0.022;
                const double ax = k == 0 ? 0 : std::sin(a) * d, az = k == 0 ? 0 : std::cos(a) * d, lean = k == 0 ? 0.0 : 0.25;
                Form& c = o.add(FK_LATHE, ax, az, r0, r0, 0.02, 0.02 + len, 3, 3, a); c.sides = 6; c.tilt = lean;
                Form& tip = o.add(FK_LATHE, ax, az, r0, r0, 0.02 + len, 0.02 + len + r0 * 1.6, 3, 3, a);   // tipped about the same foot as its prism
                tip.sub = 1; tip.p[0] = 0; tip.sides = 6; tip.tilt = lean;
            }
            break;
        }
        default: {   // something that hums: a sphere over a dark ring
            Form& ring = o.add(FK_TORUS, 0, 0, 0.16, 0.16, 0, 0.02, FORM_PART_DARK); ring.p[0] = 0.014;
            Form& b = o.add(FK_LATHE, 0, 0, 0.1, 0.1, 0.04, 0.24, 3, 3); b.sub = 4; b.p[0] = 1;
            break;
        }
    }
}
