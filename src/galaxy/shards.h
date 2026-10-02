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
};
Lore loreOf(const StarSystem& sys, const Body& b, const BodyGen& g);

enum ShardTone { ST_ORDINARY = 0, ST_ELEGY, ST_WARNING, ST_END, ST_TONE_COUNT };
extern const char* const SHARD_TONE_NAMES[ST_TONE_COUNT];

struct Shard {
    int kind = 0;           // index into the grammar's kinds (`shardKindName`)
    int tone = ST_ORDINARY;
    double age = 0;         // 0 the founding .. 1 the end: the catastrophe's worlds darken with it
    int year = 0;           // the world's own count from the founding
    std::string text;       // the whole text, as the decoder gives it once the language is learnt (C-06); empty for a piece of music
    std::string child;      // the child it names, when its kind names one: a name the decoder keeps as it is
    bool music = false;     // C-04: a piece of music, not a text (`pieceOf`, galaxy/music.h): twelve to eighteen of the fifty (`shardIsMusic`)
    uint64_t seed = 0;
};
int musicShardsOf(const BodyGen& g);              // C-04: how many of the world's fifty are music (12..18)
bool shardIsMusic(const BodyGen& g, int index);   // C-04: whether that one is
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
// view hides them), so what still glints is always new and the fifty are a world's whole collection
constexpr int SHARDS_PER_WORLD = 50;
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
