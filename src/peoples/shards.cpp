// C-02 (2026-10-02): the records' texts (`shards.h`). The texts are translations, so their words are ours; what is theirs is the
// names, the sky, the land and the water, read from the world. H-03 (2026-10-09): they are written by the grammar engine
// (peoples/grammar.h) from the files in peoples/grammar/ (C-02's kinds, rebuilt): this file is the shard model, which hands the engine
// the world's facts, the record's (its tone, its share of the span, its year and age) and its persons (the kin, the child, the
// founder), and builds a people's texts as one catalogue so a world does not tell the same template twice while others are left.
// Everything is drawn from the shard's own seed, so a shard is the same text every time.
#include "shards.h"
#include "buildings.h"
#include "charts.h"   // the charts' marks and the chart a caption names
#include "grammar.h"  // the engine
#include "genres.h"   // the records' model
#include "peoples.h"  // the profile's end (the other people's fate)
#include "records.h"  // the records past the fifty, their years
#include "../core/rng.h"
#include "../core/text.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <set>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>

const char* const SHARD_TONE_NAMES[ST_TONE_COUNT] = {"ordinary", "elegy", "warning", "the end"};
const char* const SHARD_TONE_FACTS[ST_TONE_COUNT] = {"ordinary", "elegy", "warning", "end"};

namespace {

bool vowel(char c) { c = (char)std::tolower((unsigned char)c); return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }

// a name of at most `maxLen` letters: the generator's, retried, or cut at a syllable
std::string shortName(Rng& rng, int style, size_t maxLen) {
    std::string s;
    for (int tries = 0; tries < 12; tries++) { s = generateName(rng.next(), style); if (s.size() <= maxLen && s.find(' ') == std::string::npos) return s; }
    size_t sp = s.find(' '); if (sp != std::string::npos) s = s.substr(0, sp);
    if (s.size() > maxLen) { size_t cut = maxLen; while (cut > 3 && !vowel(s[cut - 1])) cut--; s = s.substr(0, cut); }
    return s;
}

// C-12: the end's tone is the last recording's alone (`lastShardOf`); the late shards that drew it before are warnings on a
// desert and elegies on a felisian world, so one shard of a world closes its record (one draw either way: the stream stays)
int pickTone(bool desert, bool dark, double age, Rng& rng) {
    double w[ST_TONE_COUNT];
    if (dark) {   // the sun going in their last years, the long dark after it
        if (age > 0.85) { w[0] = 0.15; w[1] = 0.45; w[2] = 0.4; w[3] = 0; }
        else if (age >= 0.6) { w[0] = 0.55; w[1] = 0.2; w[2] = 0.25; w[3] = 0; }
        else { w[0] = 0.83; w[1] = 0.12; w[2] = 0.05; w[3] = 0; }
    } else if (desert) {
        if (age > 0.85) { w[0] = 0.2; w[1] = 0.1; w[2] = 0.7; w[3] = 0; }
        else if (age >= 0.3) { w[0] = 0.37; w[1] = 0.08; w[2] = 0.55; w[3] = 0; }
        else { w[0] = 0.84; w[1] = 0.08; w[2] = 0.08; w[3] = 0; }
    } else {
        if (age > 0.9) { w[0] = 0.5; w[1] = 0.35; w[2] = 0.15; w[3] = 0; }
        else { w[0] = 0.83; w[1] = 0.12; w[2] = 0.05; w[3] = 0; }
    }
    return rng.pick(w, ST_TONE_COUNT);
}

// ---------------------------------------------------------------------------
// H-03: the grammar's files (compiled in; the harness reads the same names from disk); H-04: the records' genres, the world's and the
// people's words, the events in each register, the charts' captions, the lexicon (the model: genres.cpp; its schema: genres_schema.h)
// ---------------------------------------------------------------------------

const char* const GRAMMAR_FILES[][2] = {
#include "grammar/files.inc"
};
constexpr int GRAMMAR_FILE_COUNT = (int)(sizeof(GRAMMAR_FILES) / sizeof(GRAMMAR_FILES[0]));

// the active grammar: the compiled-in files, or the harness's from disk; its genres' names (the root's alternatives); a generation
// that moves on a reload, so the texts built from the old one are built again
struct GrammarState { std::shared_ptr<const VgGrammar> G; std::vector<std::string> kindNames; int generation = 0; };
std::mutex grammarMu;
GrammarState grammarState;
std::shared_ptr<VgGrammar> buildGrammarFrom(const std::vector<std::pair<std::string, std::string>>& files) {
    auto G = std::make_shared<VgGrammar>();
    for (const auto& f : files) vgParse(*G, f.second, f.first);
    vgLink(*G, recordSchema());
    return G;
}
GrammarState activeGrammar() {
    std::lock_guard<std::mutex> lk(grammarMu);
    if (!grammarState.G) {
        std::vector<std::pair<std::string, std::string>> files;
        for (int i = 0; i < GRAMMAR_FILE_COUNT; i++) files.push_back({GRAMMAR_FILES[i][0], GRAMMAR_FILES[i][1]});
        auto G = buildGrammarFrom(files);
        grammarState.kindNames = recordGenreNames(*G);
        grammarState.G = G;
    }
    return grammarState;
}

// a shard's draws before its text: its seed, its share of the span and year, its date, and C-02's draws in their order (the herd,
// the kin, the child's name), so the tone drawn after them is C-02's and C-04's pieces keep their forms (H-04: the herd, the kin and
// the child are drawn and set aside: the records' persons are the history's)
Shard shardDraw(const BodyGen& g, const Lore& L, int index) {
    Shard s;
    BodyGen pg = g; pg.seed = loreSeed(g, L);   // the people's draws, whichever generator the caller holds
    s.seed = mix64(pg.seed ^ 0x5A4D5ULL) + (uint64_t)index * 0x9E3779B97F4A7C15ULL;
    Rng rng(s.seed);
    s.age = rng.uni();
    s.last = index == (L.last >= 0 ? L.last : lastShardOf(pg));   // the last recording: the latest of all, the end's tone
    if (s.last) s.age = 1.0;
    {   // the year the people's records give it (its share of their years, the later the likelier), C-02's share of the lore's span else
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, L.which);
        bool in = C && index >= 0 && index < C->count;
        s.year = in ? C->entries[index].year : 1 + (int)std::lround(s.age * L.spanYears);
        if (in) { s.recordAge = C->entries[index].age; s.medium = C->entries[index].medium; }   // the age of its year and what it was made on
    }
    {   // the date within the year, from the seed and not the stream (the texts stayed where C-02 left them)
        uint64_t h = mix64(s.seed ^ 0xCA1E0ULL);
        if (L.months > 0) s.month = 1 + (int)(h % (uint64_t)L.months);
        int days = L.calendar == 1 ? L.monthDays : (L.calendar == 0 ? std::max(1, (int)std::lround(L.yearDays)) : 0);
        if (days > 0) s.day = 1 + (int)((h >> 24) % (uint64_t)days);
    }
    (void)rng.chance(0.5);              // C-02's herd
    (void)rng.irange(9);                // C-02's kin
    (void)shortName(rng, L.style, 7);   // C-02's child
    s.tone = pickTone(L.desert, L.dark, s.age, rng);
    if (s.last) s.tone = ST_END;
    s.music = recordIsMusic(pg, L.records, index);   // a piece of music (the tone stays, the piece's form follows it); T-04: past the fifty too
    if (g.drifter) s.dark = sunDarkAt(g, L.which, s.year);
    s.chart = !s.music && recordChartIndex(pg, L.records, index) >= 0;   // a star chart: its caption names its figures and the star it marks
    return s;
}

// a people's texts, cached by its seed, the world's kind, the grammar's generation (the last eight)
struct TextKey {
    uint64_t seed;
    int which;
    bool desert;
    int records;
    int generation;
    bool dark;   // the people's sun gone dark (a drifter's: `sunDarkAt`)
    bool operator==(const TextKey& o) const { return seed == o.seed && which == o.which && desert == o.desert && records == o.records && generation == o.generation && dark == o.dark; }
};
std::mutex textsMu;
std::list<std::pair<TextKey, std::shared_ptr<const ShardTexts>>> textsCache;
// H-04: the texts built on a thread of their own (a world's records need its history, and the history its plan: a few hundred
// milliseconds the first time a world is read, which the radar must not wait for)
struct TextJob { TextKey key; std::thread t; std::shared_ptr<std::atomic<bool>> done; };
std::list<TextJob> textJobs;   // under `textsMu`
struct TextJobsJoin { ~TextJobsJoin() { for (TextJob& j : textJobs) if (j.t.joinable()) j.t.join(); } } textJobsJoin;

}   // namespace

Shard shardDrawOf(const BodyGen& g, const Lore& L, int index) { return shardDraw(g, L, index); }

namespace {
// the names and the span: the lore's own draws (C-10 reads the people's name alone for the peoples a chart names)
void loreNames(const BodyGen& g, Lore& L) {
    Rng rng(g.seed ^ 0x10BE5ULL);
    double sw[3] = {0.4, 0.3, 0.3};
    L.style = rng.pick(sw, 3);   // C-02's draw, kept: the shards' stream reads it (their child's name, their tones after it)
    // H-04's second pass: the names in the people's own tongue (each its kind's shape: the people's, a god's, a river's...), all different
    const Tongue& T = tongueOfSeed(g.seed);
    std::vector<std::string> used;
    auto name = [&](int kind, uint64_t salt) {
        for (int tries = 0; tries < 16; tries++) {
            const std::string s = tongueName(T, mix64(g.seed ^ salt ^ (uint64_t)tries * 0x9E3779B97F4A7C15ULL), kind, (salt & 1) != 0);
            if (std::find(used.begin(), used.end(), s) == used.end()) { used.push_back(s); return s; }
        }
        return tongueName(T, mix64(g.seed ^ salt ^ 0x5151ULL), kind);
    };
    L.people = name(TN_PEOPLE, 0x10A01ULL); L.god = name(TN_GOD, 0x10A02ULL); L.river = name(TN_RIVER, 0x10A03ULL); L.mountain = name(TN_MOUNTAIN, 0x10A04ULL);
    L.sea = name(TN_SEA, 0x10A05ULL); L.city = name(TN_CAPITAL, 0x10A06ULL); L.city2 = name(TN_CITY, 0x10A07ULL);
    L.founder = name(TN_PERSON, 0x10A08ULL | (rng.chance(0.5) ? 1 : 0)); L.moon = name(TN_MOON, 0x10A0AULL); L.star = name(TN_STAR, 0x10A0CULL);
    L.festival = name(TN_FESTIVAL, 0x10A0EULL);
    L.spanYears = 150 + (int)(450 * rng.uni());
}
}

// C-12: the calendar. The day is the world's turn and the year its orbit (`yearDays` their ratio); the month is the nearest
// moon's round in those days where a moon rounds in three days at least and twice within the year. A world whose day is a quarter
// of its year or more (a locked world: its day is its year) counts no days: moons alone when it has one to count by, else years
namespace { void calendarOf(const StarSystem& sys, const Body& b, Lore& L) {
    L.dayHours = std::fabs(b.rotPeriod) / 3600.0;
    L.yearDays = b.orbitPeriod / std::max(std::fabs(b.rotPeriod), 1.0);
    L.moons = b.moonCount;
    double nearest = 1e18;
    for (const Body& m : sys.bodies) if (m.parent == b.index && m.orbitPeriod > 0 && m.orbitPeriod < nearest) nearest = m.orbitPeriod;
    bool days = L.yearDays >= 4;
    L.monthDays = 0; L.months = 0;
    if (nearest < 1e17) {
        if (days) {
            int md = (int)std::lround(nearest / std::max(std::fabs(b.rotPeriod), 1.0));
            if (md >= 3 && md * 2 <= L.yearDays) {
                L.monthDays = md;
                L.months = (int)std::floor(L.yearDays / md);
            }
        } else {
            int mo = (int)std::floor(b.orbitPeriod / nearest);
            if (mo >= 2) L.months = mo;
        }
    }
    L.calendar = days ? (L.months > 0 ? 1 : 0) : (L.months > 0 ? 2 : 3);
} }

Lore loreQuick(const StarSystem& sys0, const Body& b0, const BodyGen& g, int which) {
    const PeoplesWorld PW = peoplesWorldOf(sys0, b0);   // D-05: a drifter's people's world at its former orbit
    const StarSystem& sys = *PW.sys; const Body& b = *PW.b;
    Lore L;
    L.peoples = peoplesOf(g) > 1 ? 2 : 1;   // a world without the trait is read as one people (the harness's "what it would have said")
    L.which = which > 0 && L.peoples > 1 ? 1 : 0;
    const BodyGen pg = peopleGen(g, L.which);
    L.seed = pg.seed;
    loreNames(pg, L);
    calendarOf(sys, b, L);
    L.desert = kindHas(livingKindOf(b.type), KT_DRY);   // the world of their time
    L.dark = g.drifter;   // the people who lost their sun
    L.last = lastShardOf(pg);
    L.records = recordCountOf(g, L.which);   // the people's records, and their years the history's (the founding, year 1, to the end)
    const PeopleAge pa = peopleAgeOf(g, L.which);
    L.endYear = std::max(2, pa.lastYear);
    L.spanYears = L.endYear - 1;
    L.ageFrom = std::min(ageFromOf(pa), L.spanYears);
    if (L.peoples > 1) {   // the other people by name, and whether they ended before this people, with them or after
        L.other = peopleNameOf(peopleGen(g, 1 - L.which));
        if (!peoplesEndedTogether(g)) L.otherFate = peopleAgeOf(g, 1 - L.which).endedYearsAgo > peopleAgeOf(g, L.which).endedYearsAgo ? -1 : 1;   // the profile's end
    }
    return L;
}

Lore loreOf(const StarSystem& sys, const Body& b, const BodyGen& g, int which) {
    Lore L = loreQuick(sys, b, g, which);
    BodyGen pg = g; pg.seed = L.seed;   // C-13: the people's draws
    chartMarksOf(sys, b, pg, L.marks, recordChartCount(pg, L.records));   // the stars their charts mark (the sky scanned once, a few milliseconds); T-04: every chart's
    return L;
}

std::string peopleNameOf(const BodyGen& g) { Lore L; loreNames(g, L); return L.people; }
std::string personName(const Lore& L, uint64_t seed, bool female) { return tongueName(tongueOfSeed(L.seed), seed, TN_PERSON, female); }   // C-12; H-04: the tongue's
std::string personName(const Lore& L, uint64_t seed) { return personName(L, seed, (mix64(seed ^ 0x5E7ULL) & 1) != 0); }

std::string shardDate(const Lore& L, const Shard& s) {
    std::string d = "year " + std::to_string(s.year) + " of " + L.city;
    if (L.calendar == 1) d += ", moon " + std::to_string(s.month) + ", day " + std::to_string(s.day);
    else if (L.calendar == 0) d += ", day " + std::to_string(s.day);
    else if (L.calendar == 2) d += ", moon " + std::to_string(s.month);
    return d;
}
std::string calendarLine(const Lore& L) {   // C-12: no unit of ours: their day is their own
    char buf[120];
    switch (L.calendar) {
        case 1: {
            int over = (int)std::lround(L.yearDays) - L.months * L.monthDays;
            if (over > 0) snprintf(buf, sizeof buf, "year: %d moons of %d days and %d over", L.months, L.monthDays, over);
            else snprintf(buf, sizeof buf, "year: %d moons of %d days", L.months, L.monthDays);
            break;
        }
        case 0: snprintf(buf, sizeof buf, "year: %.0f days   %s", L.yearDays, L.moons > 0 ? "the moon too quick to count" : "no moon to count by"); break;
        case 2: snprintf(buf, sizeof buf, "year: %d moons   the day is the year", L.months); break;
        default: snprintf(buf, sizeof buf, "the day is the year   no moon: years alone"); break;
    }
    return buf;
}


// H-03: the grammar
const VgSchema& shardSchema() { return recordSchema(); }
std::shared_ptr<const VgGrammar> shardGrammar() { return activeGrammar().G; }
int shardGrammarFiles(std::vector<std::string>* names) {
    if (names) { names->clear(); for (int i = 0; i < GRAMMAR_FILE_COUNT; i++) names->push_back(GRAMMAR_FILES[i][0]); }
    return GRAMMAR_FILE_COUNT;
}
const char* shardGrammarSource(int file) { return file >= 0 && file < GRAMMAR_FILE_COUNT ? GRAMMAR_FILES[file][1] : ""; }
namespace { void grammarTreeWalk(const std::string& dir, const std::string& rel, std::vector<std::string>& out) {
    DIR* d = opendir((rel.empty() ? dir : dir + "/" + rel).c_str());
    if (!d) return;
    std::vector<std::string> names;
    while (dirent* e = readdir(d)) if (e->d_name[0] != '.') names.push_back(e->d_name);
    closedir(d);
    for (const std::string& n : names) {
        const std::string r = rel.empty() ? n : rel + "/" + n;
        struct stat st;
        if (stat((dir + "/" + r).c_str(), &st) == 0 && S_ISDIR(st.st_mode)) grammarTreeWalk(dir, r, out);
        else if (n.size() > 3 && n.compare(n.size() - 3, 3, ".vg") == 0) out.push_back(r);
    }
} }
std::vector<std::string> shardGrammarTree(const std::string& dir) {
    std::vector<std::string> out;
    grammarTreeWalk(dir, "", out);
    std::sort(out.begin(), out.end());
    return out;
}
bool shardGrammarFromDisk(const std::string& dir, std::string& report) {
    std::vector<std::pair<std::string, std::string>> files;
    report.clear();
    // H-04's second pass: every `.vg` under the directory (a new file is read before it is compiled in; the unit holds the two lists
    // alike), the compiled-in order first, the rest in the tree's (E-11: `shardGrammarTree`, its folders walked)
    std::vector<std::string> names;
    for (int i = 0; i < GRAMMAR_FILE_COUNT; i++) names.push_back(GRAMMAR_FILES[i][0]);
    for (const std::string& n : shardGrammarTree(dir)) if (std::find(names.begin(), names.end(), n) == names.end()) names.push_back(n);
    for (const std::string& n : names) {
        std::string path = dir + "/" + n;
        std::ifstream f(path);
        if (!f) { report += "cannot read " + path + "\n"; return false; }
        std::stringstream ss; ss << f.rdbuf();
        files.push_back({n, ss.str()});
    }
    auto G = buildGrammarFrom(files);
    for (const std::string& e : G->errors) report += e + "\n";
    {
        std::lock_guard<std::mutex> lk(grammarMu);
        grammarState.kindNames = recordGenreNames(*G);
        grammarState.G = G;
        grammarState.generation++;
    }
    { std::lock_guard<std::mutex> lk(textsMu); textsCache.clear(); }
    return G->errors.empty();
}

int shardKindCount() { return (int)activeGrammar().kindNames.size(); }
const char* shardKindName(int kind) {
    static thread_local std::string name;   // the active grammar's (it may be reloaded under the caller)
    GrammarState st = activeGrammar();
    name = kind >= 0 && kind < (int)st.kindNames.size() ? st.kindNames[kind] : std::string("?");
    return name.c_str();
}

int shardWords(const std::string& text) {
    int n = 0; bool in = false;
    for (char c : text) { if (c == ' ') in = false; else if (!in) { in = true; n++; } }
    return n;
}

// C-04: twelve to eighteen of a world's fifty are music, which ones by a shuffle of the fifty from the world's seed; C-10:
// the three to five after them in the same shuffle are star charts, so the music stayed where C-04 put it
namespace { void shardShuffle(const BodyGen& g, int* order) {
    for (int i = 0; i < SHARDS_PER_WORLD; i++) order[i] = i;
    Rng rng(mix64(g.seed ^ 0x3C04C1ULL));
    for (int i = SHARDS_PER_WORLD - 1; i > 0; i--) std::swap(order[i], order[rng.irange(i + 1)]);
} }
int musicShardsOf(const BodyGen& g) { return 12 + (int)(mix64(g.seed ^ 0x3C04C0ULL) % 7); }
bool shardIsMusic(const BodyGen& g, int index) {
    if (index < 0 || index >= SHARDS_PER_WORLD) return false;
    int order[SHARDS_PER_WORLD]; shardShuffle(g, order);
    int count = musicShardsOf(g);
    for (int i = 0; i < count; i++) if (order[i] == index) return true;
    return false;
}
int chartShardsOf(const BodyGen& g) { return 3 + (int)(mix64(g.seed ^ 0xC4A27ULL) % 3); }
int chartIndexOf(const BodyGen& g, int index) {
    if (index < 0 || index >= SHARDS_PER_WORLD) return -1;
    int order[SHARDS_PER_WORLD]; shardShuffle(g, order);
    int music = musicShardsOf(g), charts = chartShardsOf(g);
    for (int i = 0; i < charts; i++) if (order[music + i] == index) return i;
    return -1;
}
bool shardIsChart(const BodyGen& g, int index) { return chartIndexOf(g, index) >= 0; }
uint64_t chartSeedOf(const BodyGen& g, int index) { return mix64(g.seed ^ 0xC4A28ULL ^ ((uint64_t)(index + 1) * 0x9E3779B97F4A7C15ULL)); }

// C-12: the last recording: of the text shards (the music and the charts left out) the one whose age draw is the greatest
// (`shardOf`'s first draw of the shard's seed); `shardOf` sets its age to 1, so it is the latest of all fifty, and its tone the end
int lastShardOf(const BodyGen& g) {
    int order[SHARDS_PER_WORLD]; shardShuffle(g, order);
    bool skip[SHARDS_PER_WORLD] = {};
    int taken = musicShardsOf(g) + chartShardsOf(g);
    for (int i = 0; i < taken; i++) skip[order[i]] = true;
    int best = -1; double bestAge = -1;
    uint64_t base = mix64(g.seed ^ 0x5A4D5ULL);
    for (int i = 0; i < SHARDS_PER_WORLD; i++) {
        if (skip[i]) continue;
        Rng rng(base + (uint64_t)i * 0x9E3779B97F4A7C15ULL);
        double age = rng.uni();
        if (age > bestAge) { bestAge = age; best = i; }
    }
    return best;
}


// H-03: a people's texts as one catalogue: each text record in index order with the grammar's memory across them (an alternative
// the people's earlier records used weighs less), so a world does not tell one template twice while others are left; H-04: written from
// the history by their genres (peoples/genres.h)
std::shared_ptr<const ShardTexts> shardTextsOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L) {
    GrammarState st = activeGrammar();
    TextKey key{loreSeed(g, L), L.which, L.desert, L.records, st.generation, L.dark};
    {
        std::lock_guard<std::mutex> lk(textsMu);
        for (auto it = textsCache.begin(); it != textsCache.end(); ++it)
            if (it->first == key) { textsCache.splice(textsCache.begin(), textsCache, it); return textsCache.front().second; }
    }
    double t0 = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    auto T = std::make_shared<ShardTexts>();
    writeRecordTexts(sys, b, g, L, *st.G, *T);
    T->buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count() - t0;
    T->generation = st.generation;
    std::lock_guard<std::mutex> lk(textsMu);
    for (auto it = textsCache.begin(); it != textsCache.end(); ++it) if (it->first == key) return it->second;   // another thread built it first
    textsCache.push_front({key, T});
    while (textsCache.size() > 8) textsCache.pop_back();
    return T;
}

void shardTextsPrefetch(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L) {
    GrammarState st = activeGrammar();
    TextKey key{loreSeed(g, L), L.which, L.desert, L.records, st.generation, L.dark};
    std::lock_guard<std::mutex> lk(textsMu);
    for (auto it = textJobs.begin(); it != textJobs.end();) {   // the finished threads joined
        if (it->done->load()) { if (it->t.joinable()) it->t.join(); it = textJobs.erase(it); } else ++it;
    }
    for (const auto& c : textsCache) if (c.first == key) return;
    for (const TextJob& j : textJobs) if (j.key == key) return;
    TextJob job; job.key = key; job.done = std::make_shared<std::atomic<bool>>(false);
    auto done = job.done;
    StarSystem sc = sys; const int bi = b.index; const BodyGen gc = g; const Lore lc = L;
    job.t = std::thread([sc, bi, gc, lc, done]() {
        if (bi >= 0 && bi < (int)sc.bodies.size()) shardTextsOf(sc, sc.bodies[bi], gc, lc);
        done->store(true);
    });
    textJobs.push_back(std::move(job));
}

std::shared_ptr<const ShardTexts> shardTextsIfReady(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L) {
    GrammarState st = activeGrammar();
    TextKey key{loreSeed(g, L), L.which, L.desert, L.records, st.generation, L.dark};
    {
        std::lock_guard<std::mutex> lk(textsMu);
        for (const auto& c : textsCache) if (c.first == key) return c.second;
    }
    shardTextsPrefetch(sys, b, g, L);
    return nullptr;
}

bool shardChartRecord(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int index, VgRecord& R, std::string* people) {
    Shard d = shardDraw(g, L, index);
    if (!d.chart) return false;
    BodyGen pg = g; pg.seed = loreSeed(g, L);
    StarChart ch;
    if (!chartOf(sys, b, g, L, index, ch)) return false;   // H-05 (CHARTS): the world's generator (the chart reads the people's from the lore)
    std::vector<std::string> names; std::string inFig;
    for (const ChartFigure& f : ch.figures) { names.push_back(f.name); for (int k : f.stars) if (k == ch.markStar && f.kind == 0) inFig = f.name; }
    int named = 0; for (int j = 0; j < ch.which; j++) if (L.marks[j].how == 0) named++;   // the first star of no people is the one the lore names
    GrammarState st = activeGrammar();
    const VgSchema& S = recordSchema();
    struct ChartModel : VgModel {   // CHARTS: the people's own words for their things (`{~key}`), as the records have them
        uint64_t people = 0;
        bool tongue(const std::string& key, std::string& out) override { out = tongueTerm(tongueOfSeed(people), key); return !out.empty(); }
    } M;
    M.people = L.seed;
    recordWorldFacts(sys, b, g, L, nullptr, M);
    M.bound.assign(S.entities.size(), -1);
    M.culture = mix64(loreSeed(g, L) ^ 0xC0170E5ULL);   // H-04's second pass: the people's dialect
    M.set(S, "tone", SHARD_TONE_FACTS[std::max(0, std::min(ST_TONE_COUNT - 1, d.tone))]);
    {   // the season the chart was drawn in (its date's, as a record's)
        const int seasons = std::atoi(M.facts[std::find(S.facts.begin(), S.facts.end(), std::string("seasons")) - S.facts.begin()].c_str());
        const int n = std::max(1, seasons);
        M.set(S, "season", std::to_string(L.months > 0 && d.month > 0 ? std::min(n - 1, (d.month - 1) * n / std::max(1, L.months)) : (int)(mix64(d.seed ^ 0x5EA50ULL) % (uint64_t)n)));
    }
    M.set(S, "figs", names.empty() ? std::string("no figure we know") : chartFigureList(names));
    M.set(S, "infig", inFig);
    M.set(S, "markhow", std::to_string(ch.mark.how));
    M.set(S, "named", std::to_string(std::min(named, 3)));
    M.set(S, "markpeople", ch.mark.people.empty() ? std::string("others") : ch.mark.people);
    {   // CHARTS: the way it was made (its record's age and medium) and what that way shows
        static const char* const AGES[AGE_COUNT] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "farfuture"};
        static const char* const MEDIA[RM_COUNT] = {"bone", "stone", "painted", "tablet", "scroll", "codex", "plate", "cylinder", "glass", "printed", "reel", "disc", "drive", "crystal", "hum"};
        M.set(S, "age", AGES[std::max(0, std::min(AGE_COUNT - 1, ch.age))]);
        M.set(S, "medium", MEDIA[std::max(0, std::min(RM_COUNT - 1, ch.medium))]);
        M.set(S, "chartstyle", CHART_STYLE_KEYS[std::max(0, std::min(CHS_COUNT - 1, ch.style))]);
        int numbered = 0; for (const ChartStar& cs : ch.stars) if (cs.number > 0) numbered++;
        M.set(S, "tally", std::to_string(ch.tally)); M.set(S, "tallykind", std::to_string(ch.tallyKind));
        M.set(S, "wanderers", std::to_string((int)ch.wanderers.size())); M.set(S, "numbered", std::to_string(numbered));
        M.set(S, "plate", std::to_string(ch.plate)); M.set(S, "maglimit", std::to_string(ch.magLimit)); M.set(S, "heard", std::to_string(ch.heardN));
        M.set(S, "routes", std::to_string(ch.reached + ch.planned)); M.set(S, "reached", std::to_string(ch.reached)); M.set(S, "planned", std::to_string(ch.planned));
        M.set(S, "farly", std::to_string((int)std::lround(ch.farLy))); M.set(S, "farkind", std::to_string(ch.farKind)); M.set(S, "signs", std::to_string(ch.signs));
        if (ch.style == CHS_SUNMEASURE && !ch.sunCurve.empty()) {   // the sun's measure: its measures, the first and the last
            const double last = ch.sunCurve.back()[1];
            char buf[32]; if (last >= 10) snprintf(buf, sizeof buf, "%d", (int)std::lround(last)); else snprintf(buf, sizeof buf, "%.1f", last);
            M.set(S, "tally", std::to_string((int)ch.sunCurve.size())); M.set(S, "sunfrom", std::to_string(ch.sunUnits)); M.set(S, "sunsize", buf);
        }
    }
    R.seed = mix64(d.seed ^ 0xC4A2CULL); R.tense = VG_PRESENT;
    vgRecord(*st.G, "chart", M, nullptr, R);
    R.names.insert(R.names.end(), R.theirs.begin(), R.theirs.end());   // CHARTS: their own words, kept as the names are
    if (people) *people = ch.mark.people;
    return true;
}

Shard shardOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int index) {
    Shard s = shardDraw(g, L, index);
    if (s.music) return s;
    BodyGen pg = g; pg.seed = loreSeed(g, L);
    if (s.chart) {   // C-10: the caption names the figures drawn and the star marked; its frame takes the tone
        s.kind = -1;
        VgRecord R;
        if (shardChartRecord(sys, b, g, L, index, R, &s.child)) { s.text = R.text; s.names = R.names; }   // the child: the other people's name, kept as it is by the decoder
        return s;
    }
    if (index >= 0 && index < L.records) {   // a text record, as its people's catalogue wrote it (past the records: none)
        std::shared_ptr<const ShardTexts> T = shardTextsOf(sys, b, g, L);
        if (index < (int)T->items.size()) {
            const ShardText& t = T->items[index];
            s.text = t.text;
            s.kind = t.kind;
            s.names = t.rec.names;
            s.names.insert(s.names.end(), t.rec.theirs.begin(), t.rec.theirs.end());
        }
    }
    return s;
}

void shardsOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int n, std::vector<Shard>& out) {
    out.clear();
    for (int i = 0; i < n; i++) out.push_back(shardOf(sys, b, g, L, i));
}

// ---------------------------------------------------------------------------
// C-06: the decoding
// ---------------------------------------------------------------------------

namespace {
// a token of a text: the punctuation before, the word (letters, digits, hyphens, an inner apostrophe), the rest after; a possessive 's counts as after
void splitToken(const std::string& t, std::string& pre, std::string& core, std::string& post) {
    size_t i = 0, n = t.size();
    while (i < n && !std::isalnum((unsigned char)t[i])) i++;
    pre = t.substr(0, i);
    size_t j = i;
    while (j < n && (std::isalnum((unsigned char)t[j]) || t[j] == '-' || t[j] == '\'')) j++;
    core = t.substr(i, j - i); post = t.substr(j);
    if (core.size() > 2 && core[core.size() - 2] == '\'' && (core.back() == 's' || core.back() == 'S')) { post = core.substr(core.size() - 2) + post; core.erase(core.size() - 2); }
    while (!core.empty() && (core.back() == '\'' || core.back() == '-')) { post = core.back() + post; core.pop_back(); }
}
bool allDigits(const std::string& s) { if (s.empty()) return false; for (unsigned char c : s) if (!std::isdigit(c)) return false; return true; }
// H-04: a numeral, or an ordinal written in digits ("212th"): read from the start as a numeral is
bool numeralLike(const std::string& s) {
    size_t d = 0; while (d < s.size() && std::isdigit((unsigned char)s[d])) d++;
    if (d == 0) return false;
    const std::string rest = s.substr(d);
    return rest.empty() || rest == "st" || rest == "nd" || rest == "rd" || rest == "th";
}
}   // namespace

namespace {
bool romanNumeral(const std::string& w) {   // "II", "IV": a ruler's number, after a name
    if (w.empty() || w.size() > 5) return false;
    for (char c : w) if (c != 'I' && c != 'V' && c != 'X') return false;
    return true;
}
// a word's root among the words the records wrote: an irregular form's (the realiser's tables), else the word less "-s", "-es",
// "-ies", "-ed", "-ing" when what is left is a word the records wrote too; the ending (1 "-s", 2 "-ed", 3 "-ing", 0 none)
std::pair<std::string, int> rootOf(const std::string& w, const std::set<std::string>& all) {
    std::string irr = vgIrregularLemma(w);
    if (!irr.empty()) return {irr, (w == "is" || w == "has" || w == "does") ? 1 : 2};
    auto has = [&](const std::string& x) { return x.size() >= 2 && all.count(x) > 0; };
    const size_t n = w.size();
    if (n > 4 && w.compare(n - 3, 3, "ies") == 0 && has(w.substr(0, n - 3) + "y")) return {w.substr(0, n - 3) + "y", 1};
    if (n > 3 && w.compare(n - 2, 2, "es") == 0 && has(w.substr(0, n - 2))) return {w.substr(0, n - 2), 1};
    if (n > 3 && w.back() == 's' && w[n - 2] != 's' && has(w.substr(0, n - 1))) return {w.substr(0, n - 1), 1};
    if (n > 4 && w.compare(n - 3, 3, "ied") == 0 && has(w.substr(0, n - 3) + "y")) return {w.substr(0, n - 3) + "y", 2};
    if (n > 3 && w.compare(n - 2, 2, "ed") == 0) {
        if (has(w.substr(0, n - 1))) return {w.substr(0, n - 1), 2};                   // "used" -> "use"
        if (has(w.substr(0, n - 2))) return {w.substr(0, n - 2), 2};                   // "walked" -> "walk"
        if (n > 5 && w[n - 3] == w[n - 4] && has(w.substr(0, n - 3))) return {w.substr(0, n - 3), 2};   // "stopped" -> "stop"
    }
    if (n > 5 && w.compare(n - 3, 3, "ing") == 0) {
        if (has(w.substr(0, n - 3))) return {w.substr(0, n - 3), 3};
        if (has(w.substr(0, n - 3) + "e")) return {w.substr(0, n - 3) + "e", 3};
        if (w[n - 4] == w[n - 5] && has(w.substr(0, n - 4))) return {w.substr(0, n - 4), 3};
    }
    return {w, 0};
}
// a text's tokens: split at spaces and at the paragraphs' line ends (the paragraph marked on the word after it)
template <class F> void forTokens(const std::string& text, F f) {
    size_t p = 0; bool par = false;
    while (p < text.size()) {
        size_t q = text.find_first_of(" \n", p); if (q == std::string::npos) q = text.size();
        if (q > p) { f(text.substr(p, q - p), par); par = false; }
        if (q < text.size() && text[q] == '\n') par = true;
        p = q + 1;
    }
}
void addNameWords(const std::string& name, std::set<std::string>& out) {
    size_t p = 0;
    while (p <= name.size()) {
        size_t q = name.find(' ', p); if (q == std::string::npos) q = name.size();
        std::string w = name.substr(p, q - p);
        if (!w.empty() && !romanNumeral(w)) out.insert(lowerOf(w));
        p = q + 1;
    }
}
}

// the roots of the people's records by how often they are used (ties by a hash of the root, so no alphabet shows through), each with
// its word in the people's tongue, drawn again when it would repeat one already given
void languageOf(const Lore& L, const std::vector<Shard>& records, const Tongue& T, Language& out) {
    out = Language();
    out.records = std::max(1, L.records);
    for (const std::string& nm : {L.people, L.god, L.river, L.mountain, L.sea, L.city, L.city2, L.founder, L.moon, L.star, L.festival}) addNameWords(nm, out.names);
    if (!L.other.empty()) addNameWords(L.other, out.names);   // C-13: the other people's name, kept as it is
    for (const Shard& s : records) {
        if (!s.child.empty()) addNameWords(s.child, out.names);
        for (const std::string& nm : s.names) addNameWords(nm, out.names);   // every name the records wrote
    }
    std::map<std::string, int> count;   // the words as written (a ruler's number after a name is the name's)
    for (const Shard& s : records) {
        bool afterName = false;
        forTokens(s.text, [&](const std::string& tok, bool) {
            std::string pre, core, post; splitToken(tok, pre, core, post);
            std::string key = lowerOf(core);
            const bool name = out.names.count(key) > 0, numeral = afterName && romanNumeral(core);
            if (!key.empty() && !numeralLike(key) && !name && !numeral) count[key]++;
            afterName = (name || numeral) && post.empty();
        });
    }
    std::set<std::string> all; for (const auto& kv : count) all.insert(kv.first);
    std::map<std::string, int> roots;
    for (const auto& kv : count) { auto r = rootOf(kv.first, all); out.forms[kv.first] = r; roots[r.first] += kv.second; }
    std::vector<std::pair<std::string, int>> v(roots.begin(), roots.end());
    std::stable_sort(v.begin(), v.end(),
            [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) { return a.second != b.second ? a.second > b.second : mix64(fnv1a(a.first)) < mix64(fnv1a(b.first)); });
    std::set<std::string> given;
    for (const auto& kv : v) {
        out.index[kv.first] = (int)out.words.size(); out.words.push_back(kv.first); out.counts.push_back(kv.second); out.tokens += kv.second;
        std::string w;
        for (int attempt = 0; attempt < 8; attempt++) { w = tongueWord(T, kv.first, attempt); if (!given.count(w)) break; }
        given.insert(w); out.theirs.push_back(w);
    }
}

int languageFull(int records) { return std::max(10, std::min(24, (int)std::lround(records * 0.12))); }   // 80 records: ten, as C-02's fifty; 200: 24
double languageShare(int shardsHeld, int records) {
    if (shardsHeld <= 0) return 0;
    const int full = languageFull(records);
    if (shardsHeld >= full) return 1;
    return 0.3 + 0.7 * (shardsHeld - 1) / (double)(full - 1);
}

// the most used roots down to where their uses reach the share of the records' words (the first root past it counts)
int languageKnown(const Language& L, int shardsHeld) {
    double share = languageShare(shardsHeld, L.records);
    if (share >= 1) return (int)L.words.size();
    double target = share * L.tokens; int cum = 0, k = 0;
    while (k < (int)L.words.size() && cum < target) { cum += L.counts[k]; k++; }
    return k;
}

int decodeShard(const Shard& s, const Language& L, const Tongue& T, int shardsHeld, std::vector<DecodedWord>& out) {
    out.clear();
    int known = languageKnown(L, shardsHeld), read = 0;
    bool afterName = false;
    forTokens(s.text, [&](const std::string& tok, bool par) {
        DecodedWord w; w.par = par;
        splitToken(tok, w.pre, w.ours, w.post);
        std::string key = lowerOf(w.ours);
        if (w.ours.empty()) { w.known = true; w.theirs = w.ours; }
        else if (L.names.count(key) || (afterName && romanNumeral(w.ours))) { w.name = true; w.known = true; w.theirs = w.ours; }   // H-04: "King Nilzhos II"
        else if (numeralLike(key)) {   // a numeral is read from the start; spoken digit by digit in the people's words for them (C-05; H-04: an ordinal's too)
            static const char* const DIGITS[10] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
            w.known = true; w.theirs.clear();
            for (char c : key) { if (!std::isdigit((unsigned char)c)) break; if (!w.theirs.empty()) w.theirs += '-'; w.theirs += tongueWord(T, DIGITS[c - '0']); }
            if (!allDigits(key)) w.theirs += T.endIng;   // the ordinal's ending, as their own
        }
        else {   // by its root: known when the root is; the people's word for the root and their ending
            auto f = L.forms.find(key);
            const std::string root = f != L.forms.end() ? f->second.first : key;
            const int end = f != L.forms.end() ? f->second.second : 0;
            auto it = L.index.find(root);
            w.known = it != L.index.end() && it->second < known;
            w.theirs = it != L.index.end() ? L.theirs[it->second] : tongueWord(T, root);
            if (end == 1) w.theirs += T.endS; else if (end == 2) w.theirs += T.endPast; else if (end == 3) w.theirs += T.endIng;
            if (std::isupper((unsigned char)w.ours[0]) && !w.theirs.empty()) w.theirs[0] = (char)std::toupper((unsigned char)w.theirs[0]);
        }
        afterName = w.name && w.post.empty();
        if (w.known) read++;
        out.push_back(w);
    });
    return read;
}

// H-06: a line the ship writes of what the records told (the decoder's history page), read as the records are read: a name kept (the
// line's own, given as lowercase words, or one the records wrote), a numeral read, a word by its root as the shards held read it; a word
// the records never wrote is known once the whole tongue is (the ship learns their words from their records), else shown as theirs
int decodeLine(const std::string& text, const Language& L, const Tongue& T, int shardsHeld, const std::set<std::string>& names, std::vector<DecodedWord>& out) {
    out.clear();
    const int known = languageKnown(L, shardsHeld);
    const bool whole = languageShare(shardsHeld, L.records) >= 1;
    int read = 0;
    bool afterName = false;
    forTokens(text, [&](const std::string& tok, bool par) {
        DecodedWord w; w.par = par;
        splitToken(tok, w.pre, w.ours, w.post);
        const std::string key = lowerOf(w.ours);
        if (w.ours.empty()) { w.known = true; w.theirs = w.ours; }
        else if (names.count(key) || L.names.count(key) || (afterName && romanNumeral(w.ours))) { w.name = true; w.known = true; w.theirs = w.ours; }
        else if (numeralLike(key)) {
            static const char* const DIGITS[10] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
            w.known = true; w.theirs.clear();
            for (char c : key) { if (!std::isdigit((unsigned char)c)) break; if (!w.theirs.empty()) w.theirs += '-'; w.theirs += tongueWord(T, DIGITS[c - '0']); }
            if (!allDigits(key)) w.theirs += T.endIng;
        } else {
            std::string root = key; int end = 0;
            auto f = L.forms.find(key);
            if (f != L.forms.end()) { root = f->second.first; end = f->second.second; }
            else if (!L.index.count(key)) {   // a form the records never wrote of a root they did: "besieged" of "besiege", "battles" of "battle"
                const size_t n = key.size();
                auto has = [&](const std::string& x) { return x.size() >= 2 && L.index.count(x) > 0; };
                if (n > 3 && key.back() == 's' && has(key.substr(0, n - 1))) { root = key.substr(0, n - 1); end = 1; }
                else if (n > 3 && key.compare(n - 2, 2, "ed") == 0 && has(key.substr(0, n - 1))) { root = key.substr(0, n - 1); end = 2; }
                else if (n > 3 && key.compare(n - 2, 2, "ed") == 0 && has(key.substr(0, n - 2))) { root = key.substr(0, n - 2); end = 2; }
                else if (n > 5 && key.compare(n - 3, 3, "ing") == 0 && has(key.substr(0, n - 3))) { root = key.substr(0, n - 3); end = 3; }
            }
            auto it = L.index.find(root);
            w.known = it != L.index.end() ? it->second < known : whole;
            w.theirs = it != L.index.end() ? L.theirs[it->second] : tongueWord(T, root);
            if (end == 1) w.theirs += T.endS; else if (end == 2) w.theirs += T.endPast; else if (end == 3) w.theirs += T.endIng;
            if (std::isupper((unsigned char)w.ours[0]) && !w.theirs.empty()) w.theirs[0] = (char)std::toupper((unsigned char)w.theirs[0]);
        }
        afterName = w.name && w.post.empty();
        if (w.known) read++;
        out.push_back(w);
    });
    return read;
}

int shardOpening(const std::vector<DecodedWord>& w, int maxWords) {
    const int n = (int)w.size();
    if (n <= maxWords) return n;
    int lastPar = 0, lastStop = 0;
    for (int i = 1; i < n && i <= maxWords; i++) {
        if (w[i].par) lastPar = i;
        const std::string& post = w[i - 1].post;
        if (post.find('.') != std::string::npos || post.find('?') != std::string::npos || post.find('!') != std::string::npos) lastStop = i;
    }
    if (lastPar >= 20) return lastPar;
    if (lastStop >= 20) return lastStop;
    for (int i = maxWords; i < n; i++) { const std::string& post = w[i - 1].post; if (post.find('.') != std::string::npos) return i; }   // the sentence it is in, finished
    return n;
}

std::string decodedText(const std::vector<DecodedWord>& w) {
    std::string out;
    for (const DecodedWord& d : w) { if (!out.empty()) out += ' '; out += d.pre + (d.known ? d.ours : d.theirs) + d.post; }
    return out;
}
