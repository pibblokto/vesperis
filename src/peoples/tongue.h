#pragma once
// C-06 (2026-10-02): a people's tongue, the shape of the words the decoder shows for what it has not learnt yet. H-04's second pass
// (2026-10-10): the tongue is the people's whole naming language, after the naming languages of Martin O'Leary's map generator and
// Mark Rosenfelder's kit: every people's names were drawn from one generator in three styles, so the peoples of a hundred worlds
// sounded like one. Now each people draws its own:
//
// * the sounds: a consonant inventory from a few dozen real-world-like sets (a small Polynesian one, a Caucasian one rich in
//   affricates, a Bantu one with its nasals...), a few dropped and a few added; a vowel set (three to eight, long ones, diphthongs);
//   the liquids and the sibilants that may join a cluster, the consonants that may end a syllable;
// * the syllable's shapes (CV, CVC, C?L?VC, S?CVF...) and the length of a root (one to three syllables);
// * the spelling: each sound's letters chosen among its ways (sh, sch, x or sj; kh, ch or x; y, j or i), never two sounds alike, a
//   doubled consonant between vowels in some tongues, an apostrophe for a glottal stop in others;
// * the morphemes: the people's words for a town, a river, a hill, holy, old, new, the people... that its place names are built with,
//   before the root or after it, joined or hyphened; a pool of name roots that come back in its persons' and places' names (as "-ton"
//   and "Wil-" come back in ours); its persons' endings by sex where the tongue marks it, its families' affix.
//
// Deterministic from the people's seed (`tongueOfSeed`, cached). The lore's names (the people, the god, the river, the capital...),
// the history's (its persons, families and places) and the graves' are this tongue's; the decoder's words for what is not learnt yet
// (`tongueWord`) and the people's own words a record keeps (`tongueTerm`, the grammar's `{~coin}`) are too, and the voice speaks its
// sounds (`voiceOf` reads `onsets`, `nuclei`, `codas` and `style`).
#include <cstdint>
#include <string>
#include <vector>

struct BodyGen;
struct Lore;

// the generic meanings a name is built with
enum TongueMeaning { TM_TOWN = 0, TM_VILLAGE, TM_CITY, TM_FORT, TM_HILL, TM_MOUNT, TM_RIVER, TM_WATER, TM_LAKE, TM_SEA, TM_ISLE, TM_LAND, TM_FIELD,
                     TM_FORD, TM_GATE, TM_HOME, TM_HOLY, TM_GREAT, TM_SMALL, TM_NEW, TM_OLD, TM_NORTH, TM_SOUTH, TM_RED, TM_BLACK, TM_WHITE, TM_STONE,
                     TM_WOOD, TM_GOD, TM_FEAST, TM_STAR, TM_MOON, TM_PEOPLE, TM_COUNT };

struct Tongue {
    uint64_t seed = 0;
    uint64_t people = 0;              // the people's seed it was drawn from
    // the sounds as spelt: what begins a syllable (a consonant, a cluster), its vowels, what ends one (the voice reads the three)
    std::vector<std::string> onsets, nuclei, codas;
    double codaChance = 0.5, vowelStart = 0.2;   // a syllable's chance of an ending consonant, a word's of a bare first vowel (C-06's)
    // the people's own endings, put on the word for a root as ours put "-s", "-ed", "-ing" on it
    std::string endS, endPast, endIng;
    // H-04's second pass: the naming language
    std::vector<std::string> cons, vows, liquids, sibs, finals;   // spelt
    std::vector<std::string> shapes;  // its syllables' shapes, the first the commonest ("CVC", "C?L?VC"...)
    int minSyl = 1, maxSyl = 2;       // a root's syllables
    double gem = 0;                   // a single consonant between vowels written double
    std::vector<std::string> roots;   // the name roots that come back
    std::string morph[TM_COUNT];      // the generic words (lowercase)
    std::string joiner;               // between a name's parts: "", "-" or "'"
    bool genericFirst = false;        // "Tel-Arad" (the generic before the root) or "Arad-tel"
    bool capSecond = false;           // a hyphened name's second part capitalised
    std::string femEnd, mascEnd;      // a given name's ending by sex ("" where the tongue marks none)
    int family = 0;                   // a family's name: 0 a root and the affix after it, 1 the affix before it, 2 two roots
    std::string famAffix;
    int style = 0;                    // how it sounds: 0 plain, 1 hard (stops, back fricatives), 2 flowing (liquids, vowels)
    std::string inventory;            // the consonant set it began from (the harness's)
};

const Tongue& tongueOfSeed(uint64_t peopleSeed);   // cached (thread-safe)
Tongue tongueOf(const BodyGen& g, const Lore& L);  // the lore's people's (`loreSeed`)
std::string tongueWord(const Tongue& T, const std::string& word, int attempt = 0);   // the people's word for one of ours (lowercase in and out; `attempt` draws another)
std::string tongueTerm(const Tongue& T, const std::string& key);   // their word for a thing of theirs with none of ours (`{~coin}`), lowercase

// a name in the tongue: a person's given name (by sex), a family's, a place's by its rank, a river's, a mountain's, a sea's, the people's,
// a god's, a festival's, a moon's, a star's. Capitalised, one word (a hyphen or an apostrophe within), at most nine letters for a
// person, ten for a family, twelve else
enum TongueNameKind { TN_PERSON = 0, TN_FAMILY, TN_HAMLET, TN_VILLAGE, TN_TOWN, TN_CITY, TN_CAPITAL, TN_RIVER, TN_MOUNTAIN, TN_SEA, TN_PEOPLE,
                      TN_GOD, TN_FESTIVAL, TN_MOON, TN_STAR, TN_THING, TN_COUNT };
std::string tongueName(const Tongue& T, uint64_t seed, int kind, bool female = false);
int tongueNameKindOfRank(int sclass);   // a settlement's rank (`SC_HAMLET` ..) to its kind of name
bool tongueReadable(const std::string& name);   // a name as the tongue keeps them: letters (a hyphen or an apostrophe within), no letter thrice, no rude word of ours
