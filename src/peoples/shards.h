#pragma once
// C-02 (2026-10-02): the story grammar. A dead people's recordings ("shards") are short texts a world generates from its
// own data: the names its people shared (`Lore`, from the body's seed), a vocabulary read from the system and the planet
// function (its sun, its sky, its land, its water, its crops and seasons) and a grammar of templates by kind and tone.
// Deterministic from the body's seed and the shard's index, no language model; judged on paper first (`vesperis_test
// shards`), the audio and the finding come in the later C milestones (`docs/archive/ideas/IDEAS-ruins-shards-signals.md`).
// the grammar is the engine's (peoples/grammar.h) and its files peoples/grammar's (files.inc); a people's texts are
// built as one catalogue (`shardTextsOf`), the engine's memory across its records.
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include "ruins.h"
#include "grammar.h"
#include "tongue.h"
#include "lore.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

Lore loreOf(const StarSystem& sys, const Body& b, const BodyGen& g, int which = 0);
std::string peopleNameOf(const BodyGen& g);   // C-10: what `loreOf` would call the world's people, without the rest (the other peoples a chart names); C-13: a people's by `peopleGen`
struct Shard;
Lore loreQuick(const StarSystem& sys, const Body& b, const BodyGen& g, int which = 0);   // C-12: the lore without the chart marks (the names, the span, the calendar, the last): the view's graves, the harness
// C-12: a person's given name, nine letters at most (the graves'); H-04's second pass: in the people's tongue (`tongueName`), the sex drawn
// from the seed where none is given
std::string personName(const Lore& L, uint64_t seed);
std::string personName(const Lore& L, uint64_t seed, bool female);
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
    bool last = false;      // the world's last recording (`lastShardOf`): the latest of the fifty, the end's tone, the one that closes the timeline
    std::string text;       // the whole text, as the decoder gives it once the language is learnt (C-06); empty for a piece of music
    std::string child;      // the child it names, when its kind names one: a name the decoder keeps as it is
    bool music = false;     // a piece of music, not a text (`pieceOf`, peoples/music.h): twelve to eighteen of the fifty (`shardIsMusic`)
    bool chart = false;     // a star chart (`chartOf`, peoples/charts.h): three to five of the fifty (`shardIsChart`); `text` is its caption, which names its figures and the star it marks
    uint64_t seed = 0;
    std::vector<std::string> names;   // the proper names its text wrote (the history's persons and places, the lore's): the decoder keeps them
    int recordAge = -1;     // the age of its year (AGE_*, the record's era: a modern people's medieval hymn is medieval), -1 past the records
    int medium = -1;        // what it was made on (RM_*, peoples/records.h: a clay tablet, a wax cylinder, a disc), -1 past the records
    int dark = 0;           // a drifter's people's sun in its year (`sunDarkAt`: 0 as ever .. 3 gone): its music by the cold
};
int musicShardsOf(const BodyGen& g);              // C-04: how many of the world's fifty are music (12..18)
bool shardIsMusic(const BodyGen& g, int index);   // C-04: whether that one is
int chartShardsOf(const BodyGen& g);              // C-10: how many of the fifty are star charts (3..5): the next of the music's shuffle, so the music stayed where it was
bool shardIsChart(const BodyGen& g, int index);   // C-10: whether that one is
int chartIndexOf(const BodyGen& g, int index);    // C-10: which of the world's charts that shard is (0..n-1), -1 when it is not one
uint64_t chartSeedOf(const BodyGen& g, int index);
int shardKindCount();               // H-03: the grammar's kinds (the alternatives of its root); H-04: the records' genres (`record`)
const char* shardKindName(int kind); // "letter", "annal", "the last" (the genre's rule, `genre.the-last.dry`, without its dots)
Shard shardOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& lore, int index);
Shard shardDrawOf(const BodyGen& g, const Lore& lore, int index);   // H-04: a shard's draws without its text (its seed, year, date, tone, music, chart)
void shardsOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& lore, int n, std::vector<Shard>& out);
int shardWords(const std::string& text);   // words, for the harness and the unit

// H-03 (2026-10-09): the records' grammar: `.vg` files (peoples/grammar/) compiled into the game, linked against the model's facts and
// persons (`shardSchema`); the harness reads the same files from disk (`shardGrammarFromDisk`: an edited rule is reprinted without a
// rebuild, the texts built again). H-04 (2026-10-09): the genres of each age written from the history (peoples/genres.h)
std::shared_ptr<const VgGrammar> shardGrammar();
const VgSchema& shardSchema();
int shardGrammarFiles(std::vector<std::string>* names);
const char* shardGrammarSource(int file);         // the compiled-in text of a file (the unit compares it with the tree's)
bool shardGrammarFromDisk(const std::string& dir, std::string& report);   // false: a file unread or the link's errors (in `report`; the grammar is still the one read)
std::vector<std::string> shardGrammarTree(const std::string& dir);   // E-11: the .vg files under dir, its folders walked: paths relative to it, in byte order
// a people's texts, built as one catalogue in index order (the engine's memory: an alternative the people's earlier records used weighs
// less): cached by the people (the last eight); the music and the charts empty. `rec` is what the engine reported (the harness's checks)
// age: the record's year's (T-04); H-04: its year, the history's events it told (in order; H-06: then the death an epitaph is about, the end the last
// recording tells), its writer, reader and subject (persons, -1 none)
// `ways`, the people's ways the text expressed: the facts its drawn alternatives were guarded by ("faith" = "manygods", "deadway" = "barrows",
// "cause" = "drying": a line written for a people of many gods, for one that buried its dead in barrows, for the drying that ended it)
struct ShardText { std::string text; int kind = -1; int age = -1; int year = 0; VgRecord rec; std::vector<int> told; int writer = -1, reader = -1, subject = -1;
                   std::vector<std::pair<std::string, std::string>> ways; };
struct ShardTexts { std::vector<ShardText> items; double buildMs = 0; int generation = 0; };
std::shared_ptr<const ShardTexts> shardTextsOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L);
// H-04: the same without waiting: null while they are built on a thread of their own (started here when they never were); the radar
// reads a world's records this way, the decoder asks for them early (`shardTextsPrefetch`) and waits only if they are not done
std::shared_ptr<const ShardTexts> shardTextsIfReady(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L);
void shardTextsPrefetch(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L);

// C-03 (2026-10-02): the shards as objects. A world had fifty (`SHARDS_PER_WORLD`, the catalogue `shardOf` reads), copies of them lying
// in its settlements' rooms; once the explorer has one every copy of it on the world is gone (the guide keeps the indices, the view hides
// them). C-13: the fifty are a people's. T-04: a people left 80 to 200 (`recordCountOf`, peoples/records.h), C-02's fifty the first of them;
// where they lie, their objects and the world's indices are peoples/records.h's (`recordSitesOf`, `shardWorldIndex`)

// the decoding on the ship. A shard is in the people's own tongue; the ship's computer learns the
// language from the shards the explorer holds of that world, the words its fifty use most first, so the first shard read
// gives about a third of its words, each further shard of the world more and the tenth all (`languageShare`,
// `languageKnown`). What is not learnt yet shows as the people's own word (`Tongue`: the shapes of their words, drawn
// from the style of their names, one word for one of ours wherever it recurs; C-05 gives them a voice); a name is kept
// as it is from the start. Everything is a function of the world and the count held, so a shard re-read after more of
// its world were found decodes again with fewer gaps, and the whole text is what ten shards give
// the people's tongue (`Tongue`, `tongueOf`, `tongueWord`): peoples/tongue.h (H-04's second pass: the whole naming language)

// the language is learnt by root words (lemmas): "walk", "walks" and "walked" are one word of theirs with their own endings, so
// the tongue of a people's two hundred records grows by its roots and not by every form of them; the share learnt is reached by the
// people's number of records (ten shards for C-02's fifty, more for a people that left more)
struct Language {
    std::vector<std::string> words, theirs;   // the roots of the people's records (lowercase, the names and the numerals left out), the most used first, and the people's word for each
    std::vector<int> counts;
    std::map<std::string, int> index;         // a root's rank
    std::map<std::string, std::pair<std::string, int>> forms;   // a word as written -> its root and its ending (0 none, 1 "-s", 2 "-ed", 3 "-ing")
    std::set<std::string> names;              // every name the records wrote (the history's persons and places, the lore's), lowercase, word by word: kept as they are
    int tokens = 0;                           // the words of the records, the names left out
    int records = 50;                         // the people's records (the share's reach)
};
void languageOf(const Lore& L, const std::vector<Shard>& records, const Tongue& T, Language& out);
double languageShare(int shardsHeld, int records = 50);  // 0 with none, 0.3 with one, 1 with ten of fifty records (H-04: of eighty; more of more, up to 24)
int languageFull(int records);                          // H-04: the shards held that read a people's whole tongue
int languageKnown(const Language& L, int shardsHeld);   // the roots known at that share, from the most used down
struct DecodedWord {
    bool par = false;        // H-04: a paragraph begins with this word
    std::string pre, post;   // the punctuation round the word (a possessive 's counts as after it)
    std::string ours;        // the word as the text has it
    std::string theirs;      // the people's word for it, capitalised as ours is
    bool known = false;      // read: shown as ours; else shown as theirs
    bool name = false;       // a name: kept as it is
};
// a shard read with that many shards of its world held; returns how many of its words are read
int decodeShard(const Shard& s, const Language& L, const Tongue& T, int shardsHeld, std::vector<DecodedWord>& out);
std::string decodedText(const std::vector<DecodedWord>& w);   // one line, as the screen shows it
// H-06: a line the ship writes of what the records told (the decoder's history page), read as the shards held read them: `names` (lowercase
// words) kept as names besides the records' own, a word the records never wrote read only once the whole tongue is; returns the words read
int decodeLine(const std::string& text, const Language& L, const Tongue& T, int shardsHeld, const std::set<std::string>& names, std::vector<DecodedWord>& out);
// H-04: how many words a decoded text's opening has: its paragraphs while they come to under `maxWords`, at least the first sentence
// past twenty words (the radar's programmes carry the opening of a long record, as the decoder's voice speaks its first page)
int shardOpening(const std::vector<DecodedWord>& words, int maxWords = 60);

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do
// a star chart's caption as the engine wrote it (the lore's marks needed: `loreOf`); false when the record is no chart or no chart is drawn
bool shardChartRecord(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int index, VgRecord& R, std::string* people = nullptr);
