// J-02 (R-448, the seeded worlds): a seeding as the life code reads it, and the layer a world's seedings make of it at the guide's
// clock (galaxy/made.h). J-03: where a culture went down (`seedPlaceOf`, the drone's look at the world map's scale), whether it took and
// why not (`seedVerdictOf`: the world, the line and the place, the first failure said), how the wild changed it (its drift), and its
// spread at its own pace through the waters, the land and the warmth it can live in, with what it does to what it reached: a dye's
// colour over the waters, a glow at night. Pure functions: the same seedings and clock give the same layer on any thread and in any order
#pragma once
#include "galaxy/made.h"
#include "galaxy/planetmap.h"
#include "galaxy/system.h"
#include "cultures.h"
#include "genes.h"
#include "morphs.h"
#include "samples.h"
#include <string>
#include <vector>

// J-03: what lies at a seeding's place, read from the planet function at the world map's scale (no rivers, springs or walls: those are a
// finer scale's), and the microhabitat a vial there would hold by the sampling's rules: its niche, its mean warmth, its salt
enum SeedWhere { AT_SEA = 0, AT_LAKE, AT_SHORE, AT_LAND, AT_SNOW, AT_ICE, AT_SALT, AT_MOLTEN, AT_COUNT };
constexpr double SEED_PLACE_M = 2500;   // the detail it is read at (over the drainage's 2,048 m: the world map's)
struct SeedPlace {
    int where = AT_LAND;
    std::string name;        // "THE SEA", "A LAKE", "THE SEA SHORE", "SOIL", "SAND", "BARE ROCK" (the seed page's PLACE line)
    Microhabitat hab;
    Vec3 unit;
    double heightM = 0;      // the ground's height, or the water's level
    bool dry = false;        // land beyond the water: a line needs DRY to live there
};
SeedPlace seedPlaceOf(const StarSystem& sys, int body, const BodyGen& g, const Biota& B, double lat, double lon);

// J-03: a line as a world takes it: its key (`<body>/M<index>`, a strain's `/S<n>` after it) and what it is (its kind: its values, its world
// genes, its genes' price; its form; its genes). The game reads it from the key as a flask does (its world, its lines before it)
struct SeedLine {
    std::string key;
    CultureKind kind;
    Morph form;
    MicrobeGenes genes;
    bool valid = false;
};
// how a seeding fared, the first failure said (IDEAS-seeded-worlds.md section 2): it grows; a furnace; the wrong liquid; too hot, too cold,
// the wrong salt, nothing it eats, no light for a light-feeder, frozen in its own ice, a line that does not live, burnt (the medium, as the
// incubator says it); oxygen that kills it, or none where it needs some; too acid, too alkaline; more radiation than it takes; dried out on
// the land; outgrown by the natives
enum SeedFate { SF_GROWS = 0, SF_FURNACE, SF_SOLVENT, SF_HOT, SF_COLD, SF_SALT, SF_HUNGRY, SF_DARK, SF_FROZEN, SF_UNVIABLE, SF_BURNT,
                SF_OXYGEN, SF_NO_OXYGEN, SF_ACID, SF_ALKALINE, SF_DOSE, SF_DRIED, SF_OUTGROWN, SF_COUNT };
extern const char* const SEED_FATE_NAMES[SF_COUNT];   // "GROWS", "A FURNACE", "WRONG LIQUID", "TOO HOT", ... "OUTGROWN BY THE NATIVES"
struct SeedVerdict {
    int fate = SF_GROWS;
    SeedPlace place;
    Medium medium;            // the place's (`mediumOfPlace`), as a flask there would hold it
    Growth growth;            // its growth there
    int oxygen = 0;           // the place's: 0 none, 1 a little (a sediment, a mat, a spring under an oxygen air), 2 an oxygen air's
    double ph = 7;            // the place's
    double doseGy = 0;        // the place's yearly dose, and what the line takes (its own, raised by its SHIELD)
    double doseTaken = 0;
    int native = -1;          // the native it met (its world index): the one that beat it, else the most of those it beat
    int pair = PA_NONE;       // how they met
    std::vector<int> beaten;  // the natives it outgrew, ate or poisoned there
    std::vector<GeneEdit> drift;   // its genes changed in the wild: a letter stepped, or the gene broken (cut)
    double pace = 1;          // its spread's pace (1: the whole world in two hours; a slow grower's under, a fast one's and LIFT's over)
    RGB water;                // the world's water as generated (a dye's colour is mixed into it)
    // J-04 (`seedingRun`): the share of the world's surface its line, as the wild left it, lives on (its waters, land and warmth: what its air
    // answers to) and the world's mean warmth as generated over the sphere (what a runaway passes)
    double share = 0, worldC = 0;
    // J-05: the world's forest as generated (a glowing forest's leaves, new leaves mixed into it) and whether it has reefs (a sour sea
    // bleaches them)
    RGB forest;
    bool reefs = false;
    double growLand = 0;   // J-11: the share of its land an overgrowth of its line could lie on (not snow or ice, within its band of warmth)
    bool takes() const { return fate == SF_GROWS; }
};
// the verdict: the furnace and the solvent, the medium (`growthIn`, its food a native it eats or one that feeds it), oxygen, pH and the dose,
// the land, the natives (`pairOf`: a competitor a tenth quicker there outgrows it, its genes' price paid; its poison beats them), its drift
SeedVerdict seedVerdictOf(const StarSystem& sys, int body, const SeedLine& line, double lat, double lon);
// "CULTURE GROWS", "CULTURE DIED, TOO COLD"
std::string seedVerdictWords(const SeedVerdict& v);
// its drift: none, one or two of its genes (never a vital one) stepped a letter or broken, by a hash of the world, the line and the place, more
// often under a strong ultraviolet; and the line as the drift left it
std::vector<GeneEdit> seedDriftOf(uint64_t worldSeed, const std::string& lineKey, uint64_t placeKey, const MicrobeGenes& genes, double doseGy);
SeedLine seedWildLine(const SeedLine& line, const std::vector<GeneEdit>& drift);

struct Seeding {
    double lat = 0, lon = 0;   // the place it was let go (radians)
    std::string line;          // the line's key (`<body>/M<index>`, a strain's `/S<n>` after it)
    std::string edits;         // its edits copied (life/genes.h's text; "" none): a strain's number is its guide's own
    Medium medium;             // the flask's medium it grew in
    int gen = 0;               // the generation it went down in (`GEN_VERSION`)
    double clock = 0;          // the guide's clock when it went down
    // J-03: worked out once (`seedingRun`): the line as it went down (its key read by the game), its verdict, the line the wild left
    SeedLine sent;
    SeedVerdict verdict;
    SeedLine wild;
    bool run = false;
};
void seedingRun(const StarSystem& sys, int body, Seeding& s);

// the step a seeding has come to at the clock: 0 as it is let go, one more each MADE_STEP_S at its pace (`SeedVerdict::pace`), MADE_STEPS at
// most; -1 before it went down
int seedingStep(const Seeding& s, double clock);
// a world's layer at the clock from its seedings in the order they went down: the first MADE_LINES that took (a culture that died made
// nothing), each spread where its line lives, the latest dye's colour over the waters and the latest glow's light at night. A layer that
// draws nothing has no key (0): a world whose lines only spread is drawn as generated
MadeWorld madeWorldOf(const StarSystem& sys, int body, const std::vector<Seeding>& seedings, double clock);
// the test layer (`seed test`, the unit): a tint and a cover over the land within `reach` radians of a place, at the region's step
MadeWorld madeTestLayer(const Vec3& place, double reach, int cover, double coverAlbedo, const RGB& tint);
// what a layer shows, in the data sheet's words: "RED SEAS", "GLOWING SEAS"; J-05: the waters', the ground's and the plants' after them,
// "MILK SEAS", "CRUSTS", "RED SNOW", "BLIGHT"; J-04: the air's last, "THICK AIR", "ORANGE HAZE", "RED CLOUDS", "BLOOD RAIN" and the rest
// (none: empty); J-08: the animals' after the air's, "DIE-OFF", "WASTING", "RED HIDES", "SILENCE"
std::vector<std::string> madeTraitWords(const MadeWorld& m, bool seas);
// J-04: the made air's names (`MadeAirTrait`'s order; a dyed cloud's and rain's words are their dye's: `madeTraitWords`)
extern const char* const MADE_AIR_NAMES[MADE_AIR_TRAITS];
// J-05: the made waters', ground's and plants' names (`MadeSeaTrait`'s, `MadeGroundTrait`'s and `MadePlantTrait`'s order; a coloured snow and new
// leaves are named by their dye: "RED SNOW", "VIOLET LEAVES")
extern const char* const MADE_SEA_NAMES[MADE_SEA_TRAITS];
extern const char* const MADE_GROUND_NAMES[MADE_GROUND_TRAITS];
extern const char* const MADE_PLANT_NAMES[MADE_PLANT_TRAITS];
// J-08: the animals' fates' names (`MadeFaunaTrait`'s order; dyed hides are named by their dye: "RED HIDES"), and what killed them ("A NERVE
// POISON", "THE HEAT")
extern const char* const MADE_FAUNA_NAMES[MADE_FAUNA_TRAITS];
// J-09: the overgrowths' names (`MadeGrowthType`'s order: "FLESH", "FUNGUS", "SLIME MOULD" ... "FILMS"); the cover a type lays at a hardness
// (a coral's, a sponge's, a lichen's or a plated or bony one's hard ground; a fungus's, hair's or a carpet's threads; else a soft tissue)
extern const char* const MADE_GROWTH_NAMES[OG_COUNT];
inline int madeGrowthCover(int type, int hard) {
    if (type == OG_CORAL || type == OG_SPONGE || type == OG_LICHEN || hard >= 2) return MAT_HARD;
    return type == OG_FUNGUS || type == OG_HAIR || type == OG_CARPETS ? MAT_THREADS : MAT_TISSUE;
}
extern const char* const MADE_DEATH_NAMES[MD_COUNT];
// J-04: the data sheet's AIR line: "THICKER, 9 C WARMER THAN IT WAS", "HAZY, 4 C COLDER THAN IT WAS" (none: empty)
std::string madeAirWords(const MadeWorld& m);
// J-11: whether a layer harmed its world's life, and whether it made the world more livable where it harmed nothing (IDEAS-seeded-worlds.md
// section 5's shares, which `seed kinds` prints). Harm: a die-off, a sickness (a glow or a dye in the skin alone is a look), changed young or
// silence; plants blighted, rotted, frost-burnt, bleached, galled or dried by the heat; seas a soup, a jelly, sour, purple, black or boiling; a
// runaway, the ice running, a thinned ozone; an overgrowth over the land (films by the waters are a look). More livable: thriving animals, lush
// plants, a new ozone, a greened world
bool madeHarms(const MadeWorld& m);
bool madeLivable(const MadeWorld& m);
