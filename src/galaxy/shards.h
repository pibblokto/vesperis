#pragma once
// C-02 (2026-10-02): the story grammar. A dead people's recordings ("shards") are short texts a world generates from its
// own data: the names its people shared (`Lore`, from the body's seed), a vocabulary read from the system and the planet
// function (its sun, its sky, its land, its water, its crops and seasons) and a grammar of templates by kind and tone.
// Deterministic from the body's seed and the shard's index, no language model; judged on paper first (`vesperis_test
// shards`), the audio and the finding come in the later C milestones (`docs/ideas/IDEAS-ruins-shards-signals.md`).
#include "system.h"
#include "planetmap.h"
#include "ruins.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// C-10 (2026-10-03): a star the people marked on one of their charts (`galaxy/charts.h`): a people's world among the bright
// stars of their sky, the star a transmission they heard came from, or the brightest of their nights, the one the lore names
struct ChartMark {
    Star star;              // the star (its name left out: the view resolves the explorer's)
    int body = -1;          // its people's world, -1 none
    std::string people;     // that world's people (empty when the star has none)
    int how = 0;            // 0 the brightest of their sky (`Lore::star` names the first such), 1 a people's world among their bright stars, 2 a transmitter they heard
    double ly = 0;          // from their star
};

// the proper nouns a world's shards share: the people's name, their god, their river, mountain, sea, two towns, the founder,
// the moon and the star they named, the festival; and the world's reckoning (the span of years the shards cover)
struct Lore {
    std::string people, god, river, mountain, sea, city, city2, founder, moon, star, festival;
    int style = 0;          // the names' phonotactic style (`generateName`): 0 plain, 1 hard, 2 flowing
    double yearDays = 0;    // the world's year in its own days
    double dayHours = 0;    // its day in hours
    int moons = 0;
    bool desert = false;    // the catastrophe's worlds (a felisian world went on without its people)
    int spanYears = 300;    // the years between the founding and the last shard
    // C-12: their calendar (`loreQuick`): the day is the world's turn, the month the nearest moon's round in those days, the year
    // the orbit; `calendar` 0 days and years, 1 moons, days and years, 2 moons and years (the day is the year: a locked world),
    // 3 years alone; `last` the index of the world's last recording (`lastShardOf`)
    int monthDays = 0, months = 0, calendar = 0, last = -1;
    std::vector<ChartMark> marks;   // C-10: the stars their charts mark, one per chart (`chartShardsOf`), the peoples they knew first
    // C-13: which of the world's peoples this lore is (`peoplesOf`, galaxy/ruins.h), the seed its draws read (`peopleGen`: the
    // world's own for the first people), and, where the world had two, the other people's name and their fate against this
    // people's end (-1 they ended first, 0 together, 1 they went on), which the grammar's "other" kinds speak of
    int which = 0, peoples = 1;
    uint64_t seed = 0;
    std::string other;
    int otherFate = 0;
};
Lore loreOf(const StarSystem& sys, const Body& b, const BodyGen& g, int which = 0);
std::string peopleNameOf(const BodyGen& g);   // C-10: what `loreOf` would call the world's people, without the rest (the other peoples a chart names); C-13: a people's by `peopleGen`
struct Shard;
Lore loreQuick(const StarSystem& sys, const Body& b, const BodyGen& g, int which = 0);   // C-12: the lore without the chart marks (the names, the span, the calendar, the last): the view's graves, the harness
// C-13: the seed a people's functions draw from: the lore's (the people's), the world's for a lore built without one. The
// functions that take a lore (`shardOf`, `tongueOf`, `voiceOf`, `traditionOf`, `chartOf`) read it and never the generator's
// seed, so they take the world's generator and give the lore's people; the seed-only ones (`shardIsMusic`, `chartShardsOf`,
// `lastShardOf`, `peopleNameOf`...) take the people's generator (`peopleGen`) and count a people's fifty from zero
inline uint64_t loreSeed(const BodyGen& g, const Lore& L) { return L.seed ? L.seed : g.seed; }
int loreStyleOf(const BodyGen& g);                  // C-12: the names' style alone
std::string personName(int style, uint64_t seed);   // C-12: a name in that style, nine letters at most (the graves')
int lastShardOf(const BodyGen& g);                  // C-12: the world's last recording: the text shard of the greatest age, its year the span's end, its tone the end
std::string shardDate(const Lore& L, const Shard& s);   // C-12: "year 212 of Kethra, moon 4, day 9" in the people's reckoning
std::string calendarLine(const Lore& L);               // C-12: "year: 15 moons of 27 days and 7 over" (no unit of ours: the day is theirs)

enum ShardTone { ST_ORDINARY = 0, ST_ELEGY, ST_WARNING, ST_END, ST_TONE_COUNT };
extern const char* const SHARD_TONE_NAMES[ST_TONE_COUNT];

struct Shard {
    int kind = 0;           // index into the grammar's kinds (`shardKindName`)
    int tone = ST_ORDINARY;
    double age = 0;         // 0 the founding .. 1 the end: the catastrophe's worlds darken with it
    int year = 0;           // the world's own count from the founding
    int month = 0, day = 0; // C-12: the date within the year in the people's reckoning (`shardDate`); 0 where they counted none
    bool last = false;      // C-12: the world's last recording (`lastShardOf`): the latest of the fifty, the end's tone, the one that closes the timeline
    std::string text;       // the whole text, as the decoder gives it once the language is learnt (C-06); empty for a piece of music
    std::string child;      // the child it names, when its kind names one: a name the decoder keeps as it is
    bool music = false;     // C-04: a piece of music, not a text (`pieceOf`, galaxy/music.h): twelve to eighteen of the fifty (`shardIsMusic`)
    bool chart = false;     // C-10: a star chart (`chartOf`, galaxy/charts.h): three to five of the fifty (`shardIsChart`); `text` is its caption, which names its figures and the star it marks
    uint64_t seed = 0;
};
int musicShardsOf(const BodyGen& g);              // C-04: how many of the world's fifty are music (12..18)
bool shardIsMusic(const BodyGen& g, int index);   // C-04: whether that one is
int chartShardsOf(const BodyGen& g);              // C-10: how many of the fifty are star charts (3..5): the next of the music's shuffle, so the music stayed where it was
bool shardIsChart(const BodyGen& g, int index);   // C-10: whether that one is
int chartIndexOf(const BodyGen& g, int index);    // C-10: which of the world's charts that shard is (0..n-1), -1 when it is not one
uint64_t chartSeedOf(const BodyGen& g, int index);
int shardKindCount();
const char* shardKindName(int kind);
Shard shardOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& lore, int index);
void shardsOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& lore, int n, std::vector<Shard>& out);
int shardWords(const std::string& text);   // words, for the harness and the unit

// C-03 (2026-10-02): the shards as objects. A world has fifty (`SHARDS_PER_WORLD`, the catalogue `shardOf` reads); a
// settlement holds a few copies of some of them by its class (a hamlet: one, three times in five; a village: one or two; a
// town: two to four; a lone monument: one, seven times in ten), distinct within the settlement, each on the floor of a room (a
// house, a hall, a tower, a rotunda) or at the foot of a stela, clear of the pieces and the doorway; deterministic from the
// settlement's seed. Once the explorer has a shard every copy of it on the world is gone (the guide keeps the indices, the
// view hides them), so what still glints is always new and the fifty are a world's whole collection. C-13: the fifty are a
// people's; a world of two peoples holds a hundred, the second people's at the indices from fifty up in the guide's keys and
// the view's sites (`shardPeopleOf`, `shardLocalOf`), each people's catalogue counted from zero
constexpr int SHARDS_PER_WORLD = 50;
inline int shardPeopleOf(int index) { return index / SHARDS_PER_WORLD; }       // C-13: whose fifty a world index is from
inline int shardLocalOf(int index) { return index % SHARDS_PER_WORLD; }        // C-13: its place in that people's fifty
inline int shardWorldIndex(int people, int local) { return people * SHARDS_PER_WORLD + local; }
inline int shardsOfWorld(const BodyGen& g) { return SHARDS_PER_WORLD * (peoplesOf(g) > 1 ? 2 : 1); }   // C-13: the world's whole collection
enum ShardPlace { SHARD_IN_HOUSE = 0, SHARD_IN_HALL, SHARD_IN_TOWER, SHARD_IN_ROTUNDA, SHARD_AT_STELA, SHARD_PLACE_COUNT };
extern const char* const SHARD_PLACE_NAMES[SHARD_PLACE_COUNT];   // "A HOUSE", "A HALL", "A TOWER", "A ROTUNDA", "THE FOOT OF A STELA"
struct ShardSite {
    int index = 0;          // which of the world's fifty
    int building = -1;      // the settlement's building it lies in (or at)
    int place = 0;          // ShardPlace
    double x = 0, z = 0;    // metres east and north of the settlement's centre
    double heading = 0;     // the slab's orientation (radians from north)
};
void shardSitesOf(const RuinSpec& r, const Culture& c, std::vector<ShardSite>& out);

// C-06 (2026-10-02): the decoding on the ship. A shard is in the people's own tongue; the ship's computer learns the
// language from the shards the explorer holds of that world, the words its fifty use most first, so the first shard read
// gives about a third of its words, each further shard of the world more and the tenth all (`languageShare`,
// `languageKnown`). What is not learnt yet shows as the people's own word (`Tongue`: the shapes of their words, drawn
// from the style of their names, one word for one of ours wherever it recurs; C-05 gives them a voice); a name is kept
// as it is from the start. Everything is a function of the world and the count held, so a shard re-read after more of
// its world were found decodes again with fewer gaps, and the whole text is what ten shards give
struct Tongue {
    std::vector<std::string> onsets, nuclei, codas;   // this people's syllable parts
    double codaChance = 0.5, vowelStart = 0.2;        // a syllable's chance of a coda, a word's of a bare first vowel
    uint64_t seed = 0;
};
Tongue tongueOf(const BodyGen& g, const Lore& L);
std::string tongueWord(const Tongue& T, const std::string& word, int attempt = 0);   // the people's word for one of ours (lowercase in and out; `attempt` draws another)

struct Language {
    std::vector<std::string> words, theirs;   // the words of the world's fifty (lowercase, the names and the numerals left out), the most used first, and the people's word for each
    std::vector<int> counts;
    std::map<std::string, int> index;         // a word's rank
    std::set<std::string> names;              // the lore's names and the shards' children, lowercase: kept as they are
    int tokens = 0;                           // the words of the fifty, the names left out
};
void languageOf(const Lore& L, const std::vector<Shard>& fifty, const Tongue& T, Language& out);
double languageShare(int shardsHeld);                   // 0 with none, 0.3 with one, 1 with ten
int languageKnown(const Language& L, int shardsHeld);   // the words known at that share, from the most used down
struct DecodedWord {
    std::string pre, post;   // the punctuation round the word (a possessive 's counts as after it)
    std::string ours;        // the word as the text has it
    std::string theirs;      // the people's word for it, capitalised as ours is
    bool known = false;      // read: shown as ours; else shown as theirs
    bool name = false;       // a name: kept as it is
};
// a shard read with that many shards of its world held; returns how many of its words are read
int decodeShard(const Shard& s, const Language& L, const Tongue& T, int shardsHeld, std::vector<DecodedWord>& out);
std::string decodedText(const std::vector<DecodedWord>& w);   // one line, as the screen shows it
