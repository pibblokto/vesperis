#pragma once
// A-01 (2026-10-08): the ages. A people is found at the age it had reached when it collapsed (`docs/ideas/IDEAS-civilisation-
// expansion.md`, section 1; the user's six): prehistoric, medieval, industrial, modern, futuristic, far future, the early ages
// commoner and the far future rare. Its profile is the facts every later block reads (what it built, the plan of its world, its
// history, the grammar of its records: `docs/ideas/RESEARCH-records.md`, section 6): the age reached and the eras it passed with
// their years in the people's own count, the cause of its collapse (by the age and the world) and its stages, how long ago it
// ended (C-08's span, longer for the late ages, whose works outlast them), how many they were and how much of the world they
// held, how they were governed, what they lived by, what they believed and what they valued. Deterministic from the people's
// generator (`peopleGen`, C-13) and the world. Two functions: `peopleAgeOf` reads the generator alone (the age, the eras, the
// end: what a ruin cell and the radar need), `peopleProfileOf` adds the rest (the cause reads the system: its star, its comets
// and belts, the orbit; the extent samples the land). Only the peoples that had radio (the late industrial age and after) are on
// the air (C-07's transmitters, `galaxy/signals.h`). No screen names an age: the ruins show it, the records tell it (H-06).
// The names below are the facts as the grammar will read them (lowercase noun phrases); the lore's names stay C-02's (`Lore`).
#include "system.h"
#include "planetmap.h"
#include <string>
#include <vector>

enum Age { AGE_PREHISTORIC = 0, AGE_MEDIEVAL, AGE_INDUSTRIAL, AGE_MODERN, AGE_FUTURISTIC, AGE_FAR_FUTURE, AGE_COUNT };
extern const char* const AGE_NAMES[AGE_COUNT];   // "prehistoric", "medieval", "industrial", "modern", "futuristic", "far future"
extern const double AGE_SHARES[AGE_COUNT];       // the decided shares: 0.28, 0.32, 0.18, 0.13, 0.07, 0.02
double ageDurability(int age);                   // how much longer its works outlast the years than a medieval people's (the decay reads years over this)

// the causes of a collapse (the ideas note's section 8, by age; the world's own disasters weigh in at every age)
enum Cause {
    CAUSE_FAMINE = 0, CAUSE_ICE_AGE, CAUSE_VOLCANIC_WINTER, CAUSE_FLOOD, CAUSE_SICKNESS, CAUSE_BAND_WAR, CAUSE_LEAVING,               // prehistoric
    CAUSE_GREAT_SICKNESS, CAUSE_CONQUEST, CAUSE_CROP_FAILURE, CAUSE_DRYING, CAUSE_COLD_CENTURIES, CAUSE_CIVIL_WAR, CAUSE_IMPACT,     // medieval
    CAUSE_POISONED, CAUSE_COAL_OUT, CAUSE_INDUSTRIAL_WAR, CAUSE_REVOLUTION, CAUSE_EPIDEMICS,                                         // industrial
    CAUSE_NUCLEAR, CAUSE_ENGINEERED_PLAGUE, CAUSE_CLIMATE, CAUSE_FOOD_SYSTEMS, CAUSE_CASCADE, CAUSE_WATER_WAR, CAUSE_FEWER_CHILDREN, // modern
    CAUSE_MACHINE, CAUSE_NANOTECH, CAUSE_GEOENGINEERING, CAUSE_FLARE, CAUSE_WORLD_WEAPONS, CAUSE_RETREAT, CAUSE_EXODUS,              // futuristic
    CAUSE_ELSEWHERE, CAUSE_PHYSICS, CAUSE_CHOSEN, CAUSE_UNDESCRIBED,                                                                 // far future
    CAUSE_COUNT
};
extern const char* const CAUSE_NAMES[CAUSE_COUNT];   // "famine", "an ice age", ... "something the records cannot describe"
bool causeWorldwide(int cause);   // it can end every people of a world at once (two peoples that ended together share one)
int causeSpeed(int cause);        // 0 sudden (an impact, a flare), 1 over years, 2 over generations (the drying, the climate)

enum Government { GOV_CHIEFS = 0, GOV_MONARCHY, GOV_COUNCIL, GOV_THEOCRACY, GOV_REPUBLIC, GOV_ASSEMBLY, GOV_CORPORATE, GOV_MACHINE, GOV_COUNT };
extern const char* const GOVERNMENT_NAMES[GOV_COUNT];     // "chiefs", "a monarchy", "a council", ...
extern const char* const GOVERNMENT_RULERS[GOV_COUNT][2]; // the one at the head, as a man and as a woman ("king", "queen"; "consul", "consul")
enum Economy { ECO_HUNTING = 0, ECO_HERDING, ECO_FARMING, ECO_TRADE, ECO_INDUSTRY, ECO_INFORMATION, ECO_COUNT };
extern const char* const ECONOMY_NAMES[ECO_COUNT];        // "hunting", "herding", "farming", "trade", "industry", "information"
enum Value { VAL_WAR = 0, VAL_PIETY, VAL_TRADE, VAL_LEARNING, VAL_ART, VAL_COUNT };
extern const char* const VALUE_NAMES[VAL_COUNT];          // "war", "piety", "trade", "learning", "art"
enum Faith { FAITH_ANCESTORS = 0, FAITH_LAND, FAITH_SKY, FAITH_MANY_GODS, FAITH_ONE_GOD, FAITH_PHILOSOPHY, FAITH_MACHINE, FAITH_COUNT };
extern const char* const FAITH_NAMES[FAITH_COUNT];        // "the ancestors", "the spirits of the land", "the sky", "many gods", "one god", "a philosophy", "the machine"
enum Rite { RITE_FIRST_FRUITS = 0, RITE_FIRE, RITE_SONG, RITE_WATER, RITE_SILENCE, RITE_FAST, RITE_PILGRIMAGE, RITE_ANIMALS, RITE_MEMORY, RITE_COUNT };
extern const char* const RITE_NAMES[RITE_COUNT];          // "the first of the harvest", "a fire kept", ...
enum Holy { HOLY_TEMPLE = 0, HOLY_GROVE, HOLY_HIGH_PLACE, HOLY_SPRING, HOLY_CAVE, HOLY_HOUSE, HOLY_TOMBS, HOLY_OPEN_SKY, HOLY_HALL, HOLY_COUNT };
extern const char* const HOLY_NAMES[HOLY_COUNT];          // "a temple", "a grove", "a high place", ...
enum Stage { STAGE_WARNING = 0, STAGE_ONSET, STAGE_CRISIS, STAGE_END, STAGE_COUNT };
extern const char* const STAGE_NAMES[STAGE_COUNT];        // "the warning", "the onset", "the crisis", "the end"

// an age the people passed: its years in the people's own count (year 1 the reckoning's start: the first towns' founding, or a
// prehistoric people's first village; the ages before it are counted back, negative) and how it was governed at its close
struct Era {
    int age = AGE_PREHISTORIC;
    int start = 1, end = 1;   // the first and the last year (the last era ends with the people)
    int government = GOV_CHIEFS;
};
// the age reached and its years: the generator alone
struct PeopleAge {
    int age = AGE_MEDIEVAL;
    std::vector<Era> eras;          // one per age passed, the oldest first; the last is the age reached
    int lastYear = 1;               // the people's count at the end
    double endedYearsAgo = 2000;    // C-08's age: how long ago the end was (two peoples that ended together share it)
    bool radio = false;             // they had radio before the end (late in the industrial age, and every later age)
    int radioYear = 0;              // the year it began in their count (radio only)
    // H-02: how long each whole age ran for them, as drawn (the age reached was cut by the end: the history reads how far into it
    // they got; the far future's is its own span)
    double fullYears[AGE_COUNT] = {0, 0, 0, 0, 0, 0};
};
PeopleAge peopleAgeOf(const BodyGen& g, int people = 0);   // the world's generator and which of its peoples (C-13)
int peoplesOnAir(const BodyGen& g);                        // the peoples of a world that had radio: bit 0 the first, bit 1 the second (0 none)

// the whole profile (`docs/ideas/RESEARCH-records.md`, section 6, item 1)
struct PeopleProfile : PeopleAge {
    uint64_t seed = 0;              // the people's own (`peopleGen`)
    int which = 0, peoples = 1;     // C-13
    int cause = CAUSE_FAMINE;       // what ended them (shared by two peoples that ended together)
    int collapseYear = 1;           // the year the collapse began (the warning) in their count
    int collapseYears = 1;          // how long it took
    double stage[STAGE_COUNT] = {0, 0.2, 0.6, 0.9};   // where each stage began, as a share of the collapse (the warning at 0)
    double landShare = 0.3;         // the share of the world that was land in their time (a dead desert's old seas were water)
    double held = 0.3;              // the share of that land they held at their height
    double peak = 1e6;              // how many they were at their height
    int government = GOV_CHIEFS;    // at the end (the last era's)
    int economy = ECO_FARMING, economy2 = ECO_HERDING;   // what they lived by, first and second
    double values[VAL_COUNT] = {0.2, 0.2, 0.2, 0.2, 0.2};   // what they valued, the weights summing to one
    int faith = FAITH_ANCESTORS, rite = RITE_FIRE, holy = HOLY_HOUSE;   // what they believed (the god is the lore's), how they kept it, where
    int stageAt(int year) const;    // the collapse's stage in a year of their count, the latest begun by its end (-1 before the warning)
    int eraAt(int year) const;      // the index of the era a year falls in (the first before it, the last after)
};
PeopleProfile peopleProfileOf(const StarSystem& sys, const Body& b, const BodyGen& g, int people = 0);
// what the world says about the ways it could end a people (the cause's weights; H-02's disasters): a desert, a red dwarf's sun, the
// volcanoes (0 .. 2.5), the system's comets and belts (0 .. 2.9), a cold world (0 .. 1), an eccentric orbit (0 .. 1), the water (0 .. 1.3)
struct WorldRead { bool desert = false, redDwarf = false; double volc = 0, impacts = 0, cold = 0, ecc = 0, wet = 0; };
WorldRead readWorld(const StarSystem& sys, const Body& b, const BodyGen& g);
// T-01: the profile's extent alone (the land of their time, the share of it they held, how many they were), from the generator: the
// plan of the world reads it without the system; `age` -1 the people's own (the harness's plans of the ages give another)
struct PeopleExtent { double landShare = 0.3, held = 0.3, peak = 1e6; };
PeopleExtent peopleExtentOf(const BodyGen& g, int people = 0, int age = -1);
std::string peopleCount(double n);   // "40 thousand", "3.2 million", "11 billion" (the harness)
