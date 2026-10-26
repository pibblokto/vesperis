// A-01 (2026-10-08): the ages (see peoples.h).
#include "peoples.h"
#include "ruins.h"
#include "galaxy/drainage.h"
#include "galaxy/drifters.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

const char* const AGE_NAMES[AGE_COUNT] = {"prehistoric", "medieval", "industrial", "modern", "futuristic", "far future"};
const double AGE_SHARES[AGE_COUNT] = {0.28, 0.32, 0.18, 0.13, 0.07, 0.02};
// the late ages' works outlast the years (concrete, composites, a material that does not weather), so their ruins at a given
// age since the end are what a medieval people's are at a fraction of it; A-03 gives each material its own
double ageDurability(int age) { static const double D[AGE_COUNT] = {1, 1, 1.5, 2.5, 6, 40}; return D[std::clamp(age, 0, AGE_COUNT - 1)]; }

const char* const GOVERNMENT_NAMES[GOV_COUNT] = {"chiefs", "a monarchy", "a council", "a theocracy", "a republic", "an assembly", "the companies", "a machine"};
const char* const GOVERNMENT_RULERS[GOV_COUNT][2] = {{"chief", "chief"}, {"king", "queen"}, {"first elder", "first elder"}, {"high priest", "high priestess"},
                                                     {"consul", "consul"}, {"speaker", "speaker"}, {"director", "director"}, {"the mind", "the mind"}};
const char* const ECONOMY_NAMES[ECO_COUNT] = {"hunting", "herding", "farming", "trade", "industry", "information"};
const char* const FAITH_NAMES[FAITH_COUNT] = {"the ancestors", "the spirits of the land", "the sky", "many gods", "one god", "a philosophy", "the machine"};
const char* const RITE_NAMES[RITE_COUNT] = {"the first of the harvest", "a fire kept", "songs", "water poured", "silence", "fasting", "a pilgrimage", "the herd's firstborn", "the dead remembered by name"};
const char* const HOLY_NAMES[HOLY_COUNT] = {"a temple", "a grove", "a high place", "a spring", "a cave", "the house", "the tombs", "the open sky", "a hall"};
const char* const STAGE_NAMES[STAGE_COUNT] = {"the warning", "the onset", "the crisis", "the end"};

namespace {
// a cause: its weight in each age (the ideas note's lists at 0.7-1.3 in their own age, a little in the next; the world's
// disasters at every age they can reach), how long it took (log-uniform years), how fast (0 sudden, 1 years, 2 generations)
// and whether it can end a whole world (two peoples that ended together share such a cause)
struct CauseInfo { const char* name; double w[AGE_COUNT]; double lo, hi; int speed; bool worldwide; };
//   the cause                                        P     M     I     Mo    F     FF       years        speed, worldwide
constexpr CauseInfo CAUSES[] = {
    {"famine",                                      {1.2,  0.3,  0,    0,    0,    0},      3, 30,       1, false},
    {"an ice age",                                  {0.8,  0.2,  0,    0,    0,    0},      150, 2000,   2, true},
    {"a volcanic winter",                           {0.6,  0.2,  0.1,  0,    0,    0},      3, 40,       0, true},
    {"a flood",                                     {0.8,  0.2,  0,    0,    0,    0},      1, 15,       0, true},
    {"sickness",                                    {1.0,  0,    0,    0,    0,    0},      3, 40,       1, true},
    {"war between bands",                           {0.9,  0,    0,    0,    0,    0},      10, 120,     1, false},
    {"a leaving",                                   {0.7,  0,    0,    0,    0,    0},      10, 200,     2, false},
    {"a great sickness",                            {0,    1.3,  0.2,  0,    0,    0},      5, 60,       1, true},
    {"conquest",                                    {0,    1.0,  0.2,  0,    0,    0},      5, 80,       1, false},
    {"crop failure",                                {0,    1.0,  0.2,  0,    0,    0},      5, 50,       1, false},
    {"the drying",                                  {0.2,  0.8,  0.3,  0,    0,    0},      60, 600,     2, true},
    {"the cold centuries",                          {0.2,  0.8,  0.1,  0,    0,    0},      100, 500,    2, true},
    {"civil war",                                   {0,    1.0,  0.2,  0.1,  0,    0},      5, 60,       1, false},
    {"an impact",                                   {0.15, 0.5,  0.15, 0.1,  0.05, 0},      1, 25,       0, true},
    {"poisoned air and rivers",                     {0,    0,    1.0,  0.2,  0,    0},      30, 150,     2, false},
    {"coal running out",                            {0,    0,    0.8,  0,    0,    0},      20, 120,     2, false},
    {"industrial war",                              {0,    0,    1.0,  0.2,  0,    0},      4, 30,       1, false},
    {"revolution",                                  {0,    0,    0.9,  0.1,  0,    0},      3, 40,       1, false},
    {"epidemics in the crowded cities",             {0,    0,    1.0,  0.1,  0,    0},      5, 40,       1, false},
    {"a nuclear exchange",                          {0,    0,    0,    1.0,  0.2,  0},      1, 20,       0, true},
    {"an engineered plague",                        {0,    0,    0,    0.8,  0.2,  0},      1, 12,       0, true},
    {"the climate",                                 {0,    0,    0.1,  1.0,  0.2,  0},      40, 300,     2, true},
    {"the food systems failing",                    {0,    0,    0,    0.8,  0,    0},      4, 40,       1, false},
    {"cascading failure",                           {0,    0,    0,    0.8,  0.3,  0},      2, 25,       1, false},
    {"a war for water",                             {0,    0,    0,    0.8,  0,    0},      8, 60,       1, false},
    {"fewer and fewer children",                    {0,    0,    0,    0.8,  0.4,  0.1},    80, 500,     2, false},
    {"a machine's decision",                        {0,    0,    0,    0,    1.0,  0.3},    1, 20,       0, true},
    {"a nanotech accident",                         {0,    0,    0,    0,    0.8,  0},      1, 8,        0, true},
    {"geoengineering gone wrong",                   {0,    0,    0,    0,    0.8,  0},      10, 120,     2, true},
    {"a stellar flare",                             {0.1,  0.1,  0.1,  0.1,  0.8,  0.1},    1, 12,       0, true},
    {"a war with weapons that end worlds",          {0,    0,    0,    0,    0.8,  0.2},    1, 5,        0, true},
    {"a retreat into the machines",                 {0,    0,    0,    0,    0.8,  0.5},    40, 400,     2, false},
    {"an exodus",                                   {0,    0,    0,    0.05, 0.9,  0.3},    15, 300,     2, false},
    {"a going elsewhere",                           {0,    0,    0,    0,    0.1,  1.2},    50, 1500,    2, false},
    {"an experiment with physics",                  {0,    0,    0,    0,    0,    1.0},    1, 3,        0, true},
    {"an ending chosen",                            {0,    0,    0,    0,    0,    1.0},    5, 200,      1, false},
    {"something the records cannot describe",       {0,    0,    0,    0,    0,    1.0},    1, 1000,     1, true},
    {"the loss of the sun",                         {0,    0,    0,    0,    0,    0},      40, 4000,    2, true},    // D-05: given, never drawn
};
static_assert(sizeof(CAUSES) / sizeof(CAUSES[0]) == CAUSE_COUNT, "a row a cause");
// the forms of government by age (the form at an era's close), what they lived by, what they believed, the rite and the
// holy place by the faith
const double GOV_W[AGE_COUNT][GOV_COUNT] = {
    // chiefs monarchy council theocracy republic assembly companies machine
    {5,   0.3, 3, 1,   0,   1,   0, 0},
    {0.6, 6,   2, 2,   1,   0.2, 0, 0},
    {0,   3,   1, 0.5, 4,   1,   1, 0},
    {0,   0.5, 1, 0.4, 5,   2,   2, 0},
    {0,   0.1, 1, 0.2, 2,   3,   3, 2},
    {0,   0,   1, 0.1, 0.3, 3,   0.5, 4}};
const double ECO_W[AGE_COUNT][ECO_COUNT] = {
    // hunting herding farming trade industry information
    {4,   3,   3,   0.3, 0,   0},
    {0.3, 2,   6,   2,   0.2, 0},
    {0,   0.3, 2,   2,   6,   0},
    {0,   0,   0.5, 3,   4,   3},
    {0,   0,   0.2, 1,   3,   6},
    {0,   0,   0.1, 0.5, 0.5, 8}};
const double FAITH_W[AGE_COUNT][FAITH_COUNT] = {
    // ancestors land sky many one philosophy machine
    {4,   4,   3,   1,   0.2, 0,   0},
    {2,   1,   1,   4,   4,   0.5, 0},
    {1,   0.3, 0.3, 2,   4,   2,   0},
    {1,   0.2, 0.2, 1,   3,   4,   0.2},
    {1,   0.2, 0.2, 0.3, 1,   5,   2},
    {0.2, 0.2, 1,   0.2, 0.2, 3,   3}};
const double RITE_W[FAITH_COUNT][RITE_COUNT] = {
    // first fruits, fire, song, water, silence, fast, pilgrimage, animals, memory
    {2, 2,   2, 1,   1,   0.5, 1,   1, 3},
    {3, 1,   2, 3,   1,   0.5, 2,   2, 0.5},
    {1, 3,   2, 1,   1,   1,   2,   1, 0.5},
    {3, 2,   2, 1,   0.5, 1,   2,   3, 0.5},
    {2, 2,   3, 1,   1,   3,   3,   1, 1},
    {0, 0.5, 1, 0.5, 4,   2,   1,   0, 2},
    {0, 0.5, 1, 0,   2,   0.5, 0.5, 0, 4}};
const double HOLY_W[FAITH_COUNT][HOLY_COUNT] = {
    // temple grove high-place spring cave house tombs open-sky hall
    {0.5, 1,   0.5, 0.5, 1,   4, 4,   0.5, 0.2},
    {0.3, 4,   2,   3,   2,   1, 0.5, 1,   0},
    {1,   0.5, 4,   0.5, 0.5, 0.5, 0.5, 4, 0.2},
    {5,   1,   2,   1,   0.5, 1, 1,   0.5, 0.5},
    {5,   0.2, 1,   0.3, 0.3, 1, 0.5, 0.5, 1},
    {0.3, 0.5, 0.5, 0.2, 0.3, 1, 0,   1,   5},
    {1,   0,   0,   0,   0,   1, 0,   0.5, 4}};

double logUni(Rng& r, double lo, double hi) { return std::exp(std::log(lo) + r.uni() * std::log(hi / lo)); }

// the age and its years from the people's own generator: every draw from fixed positions of the stream whatever the age, so
// a table's change moves no other number; the end's years from C-08's own hash (the same years where the bounds are C-08's).
// `age` the age reached where it is given (a world's second people reached its first's), the draw taken all the same
PeopleAge ageOfPeople(const BodyGen& pg, int age = -1) {
    PeopleAge a;
    Rng r(pg.seed ^ 0xA01A6EULL);
    a.age = r.pick(AGE_SHARES, AGE_COUNT);
    if (age >= 0) a.age = age;
    double lp = logUni(r, 3000, 20000), lm = logUni(r, 1000, 4500), li = r.range(120, 400), lmo = r.range(70, 300), lf = logUni(r, 250, 3000);
    double radioAt = r.range(0.55, 0.85);   // radio came this far into a whole industrial age
    double cut = r.range(0.25, 1.0);        // an industrial people got this far into it
    double uLast = r.uni();                 // the age reached ran this long before the end
    double last = 0;
    switch (a.age) {
        case AGE_PREHISTORIC: last = std::exp(std::log(400.0) + uLast * std::log(10.0)); break;    // 400-4,000: the villages' span
        case AGE_MEDIEVAL: last = std::exp(std::log(300.0) + uLast * std::log(4000.0 / 300.0)); break;
        case AGE_INDUSTRIAL: last = li * cut; break;
        case AGE_MODERN: last = 30 + 270 * uLast; break;
        case AGE_FUTURISTIC: last = std::exp(std::log(100.0) + uLast * std::log(30.0)); break;
        default: last = std::exp(std::log(800.0) + uLast * std::log(150000.0 / 800.0)); break;
    }
    const double whole[AGE_COUNT] = {lp, lm, li, lmo, lf, last};
    for (int k = 0; k < AGE_COUNT; k++) a.fullYears[k] = whole[k];
    Rng rg(pg.seed ^ 0xA01607ULL);   // the governments, an era at a time
    auto era = [&](int age, int start, int end) { Era e; e.age = age; e.start = start; e.end = std::max(start, end); e.government = rg.pick(GOV_W[age], GOV_COUNT); a.eras.push_back(e); return e.end; };
    if (a.age == AGE_PREHISTORIC) a.lastYear = era(AGE_PREHISTORIC, 1, (int)std::lround(last));
    else {
        era(AGE_PREHISTORIC, 1 - (int)std::lround(lp), 0);
        const double full[AGE_COUNT] = {lp, lm, li, lmo, lf, last};
        int y = 0;
        for (int k = AGE_MEDIEVAL; k <= a.age; k++) {
            int start = y + 1;
            y = era(k, start, y + std::max(1, (int)std::lround(k == a.age ? last : full[k])));
            if (k == AGE_INDUSTRIAL) {   // radio late in the industrial age: an industrial people had it if it got that far
                a.radio = a.age > AGE_INDUSTRIAL || cut > radioAt;
                if (a.radio) a.radioYear = start + (int)std::lround(li * radioAt);
            }
        }
        a.lastYear = y;
    }
    static const double E_LO[AGE_COUNT] = {200, 200, 200, 250, 400, 1000}, E_HI[AGE_COUNT] = {40000, 40000, 60000, 100000, 250000, 2000000};
    double uE = unitFromHash(mix64(pg.seed ^ 0xA6EDULL));   // C-08's draw: 200-40,000 log-uniform for the first two ages, longer for the late
    a.endedYearsAgo = std::exp(std::log(E_LO[a.age]) + uE * std::log(E_HI[a.age] / E_LO[a.age]));
    return a;
}

// D-05: a drifter's people's loss of its sun from the generator alone (the age, the culture and the profile read it alike): how long it took
// in the people's own years by the age (a prehistoric people's a few generations, a futuristic people's centuries below ground, a far-future
// people's over a thousand years), within the last age of both peoples (they ended together); its stages (the warning's omens, the onset's
// short summers, the crisis while the sun went, the end the long dark below); the last sunrise (the crisis's last year); and how long ago the
// end was: after the drifter left its star (`drifterLeftYears`) by the years they lived on in the dark
const double SUN_LOSS_YEARS[AGE_COUNT][2] = {{40, 150}, {60, 300}, {50, 250}, {60, 300}, {300, 1500}, {1000, 4000}};
struct SunLoss { int years = 1; double stage[STAGE_COUNT] = {0, 0.2, 0.6, 0.9}; int lastSunrise = 1; double ended = 1000; };
SunLoss sunLossOf(const BodyGen& g, const PeopleAge& first) {
    SunLoss L;
    const uint64_t ws = g.worldSeed ? g.worldSeed : g.seed;
    Rng r(ws ^ 0xD05C0ULL);
    const double want = logUni(r, SUN_LOSS_YEARS[first.age][0], SUN_LOSS_YEARS[first.age][1]);
    int lastLen = first.lastYear - first.eras.back().start + 1;
    if (peoplesOf(g) > 1) {   // C-13: the other people's last age bounds it too
        BodyGen w = g; w.seed = ws;
        const PeopleAge o = ageOfPeople(peopleGen(w, 1), first.age);
        lastLen = std::min(lastLen, o.lastYear - o.eras.back().start + 1);
    }
    L.years = std::max(1, (int)std::lround(std::min(want, 0.85 * lastLen)));
    L.stage[STAGE_WARNING] = 0;
    L.stage[STAGE_ONSET] = r.range(0.15, 0.35);
    L.stage[STAGE_CRISIS] = L.stage[STAGE_ONSET] + (1 - L.stage[STAGE_ONSET]) * r.range(0.15, 0.35);
    L.stage[STAGE_END] = L.stage[STAGE_CRISIS] + (1 - L.stage[STAGE_CRISIS]) * r.range(0.05, 0.25);
    const int collapseYear = first.lastYear - L.years + 1;
    int endFrom = collapseYear;   // the end's first year (`PeopleProfile::stageAt`'s rule)
    while (endFrom < first.lastYear && (endFrom - collapseYear + 1) / (double)L.years <= L.stage[STAGE_END]) endFrom++;
    L.lastSunrise = std::max(collapseYear, endFrom - 1);
    Star d; drifterAt(g.starSx, g.starSy, g.starSz, d, false);
    const double left = drifterLeftYears(d);
    L.ended = std::max(0.08 * left, left - (first.lastYear - L.lastSunrise));
    return L;
}

}   // namespace

// what the world says about the ways it could end a people (H-02 reads it for the disasters of a history)
WorldRead readWorld(const StarSystem& sys, const Body& b, const BodyGen& g) {
    WorldRead w;
    w.desert = kindHas(nowKind(g), KT_DRY);   // a scorched world's people died as a desert's did, of the drying and the climate
    w.redDwarf = sys.star.cls == STAR_RED_DWARF;
    for (int t : {TR_CINDER_FIELD, TR_TRAPS, TR_RIFT, TR_GEYSERS, TR_CORONAE}) if (g.hasTrait(t)) w.volc += 0.5;
    int comets = 0; for (const Body& o : sys.bodies) if (o.type == PT_COMET) comets++;
    w.impacts = 0.5 * std::min(comets, 3) + 0.7 * std::min((int)sys.belts.size(), 2);
    if (!w.desert) w.cold = clampd((70.0 - g.iceCapLat) / 8.0, 0, 1);
    double ecc = b.parent >= 0 && b.parent < (int)sys.bodies.size() ? sys.bodies[b.parent].ecc : b.ecc;
    w.ecc = clampd((ecc - 0.04) / 0.16, 0, 1);
    if (!w.desert) w.wet = clampd(g.riverDensity, 0, 1) + (g.hasTrait(TR_LAKELAND) || g.hasTrait(TR_ARCHIPELAGO) ? 0.3 : 0);
    return w;
}
namespace {
double causeWorldFactor(int c, const WorldRead& w) {
    switch (c) {
        case CAUSE_DRYING: return w.desert ? 6.0 : 0.15;   // a desert people's world dried with it (C-01's old seas); a living world went on
        case CAUSE_FLOOD: return w.desert ? 0.2 : 0.6 + w.wet;
        case CAUSE_LEAVING: return w.desert ? 2.0 : 1.0;
        case CAUSE_FAMINE: case CAUSE_CROP_FAILURE: return w.desert ? 1.5 : 1.0;
        case CAUSE_CLIMATE: case CAUSE_WATER_WAR: case CAUSE_GEOENGINEERING: return w.desert ? 3.0 : 1.0;
        case CAUSE_ICE_AGE: return (w.desert ? 0.3 : 1.0) * (1 + 2 * w.cold);
        case CAUSE_COLD_CENTURIES: return 1 + 2 * w.cold + 4 * w.ecc;   // an eccentric orbit's long winters
        case CAUSE_FLARE: return w.redDwarf ? 6.0 : 0.3;
        case CAUSE_VOLCANIC_WINTER: return 0.4 + 2.5 * w.volc;
        case CAUSE_IMPACT: return 0.5 + w.impacts;          // the comets and the belts of the system
        default: return 1.0;
    }
}
// how the collapse's stages fall by its speed: the warning at 0, the onset, the crisis and the end's start as shares
void drawStages(Rng& r, int speed, double* st) {
    static const double F1[3][2] = {{0.02, 0.15}, {0.1, 0.35}, {0.25, 0.5}}, F2[3][2] = {{0.05, 0.3}, {0.25, 0.55}, {0.35, 0.65}}, F3[3][2] = {{0.3, 0.8}, {0.35, 0.75}, {0.4, 0.8}};
    st[STAGE_WARNING] = 0;
    st[STAGE_ONSET] = r.range(F1[speed][0], F1[speed][1]);
    st[STAGE_CRISIS] = st[STAGE_ONSET] + (1 - st[STAGE_ONSET]) * r.range(F2[speed][0], F2[speed][1]);
    st[STAGE_END] = st[STAGE_CRISIS] + (1 - st[STAGE_CRISIS]) * r.range(F3[speed][0], F3[speed][1]);
}
// the share of a world that was land in its people's time (a dead desert's old seas were water): 64 points of a spiral at 2 km
double landShareOf(const BodyGen& gSeason) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    DrainageOff off;
    const int N = 64; int land = 0;
    for (int i = 0; i < N; i++) {
        double z = 1 - (2 * i + 1.0) / N, rr = std::sqrt(std::max(0.0, 1 - z * z)), a = i * 2.399963229728653;
        SurfaceSample s = sampleSurface(g, Vec3(rr * std::cos(a), rr * std::sin(a), z), 2048.0);
        if (!(s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5)) land++;
    }
    return std::max(1.0 / N, (double)land / N);
}
// the share of the world's land a people held at its height (a people of a world of two held its side's) and how many they were then
void drawExtent(Rng& r, int age, int peoples, double radiusKm, double landShare, double& held, double& peak) {
    static const double HELD[AGE_COUNT][2] = {{0.04, 0.3}, {0.15, 0.65}, {0.35, 0.85}, {0.55, 1.0}, {0.6, 1.0}, {0.15, 1.0}};
    static const double DENS[AGE_COUNT][2] = {{0.01, 0.2}, {2, 12}, {8, 40}, {15, 120}, {5, 200}, {0.1, 500}};   // people a square km of the land they held, on average
    const int a = clampi(age, 0, AGE_COUNT - 1);
    held = r.range(HELD[a][0], HELD[a][1]) * (peoples > 1 ? 0.5 : 1.0);
    const double area = 4 * PI * radiusKm * radiusKm * landShare * held;
    peak = area * logUni(r, DENS[a][0], DENS[a][1]);
}
}

PeopleExtent peopleExtentOf(const BodyGen& g, int people, int age) {
    PeopleExtent e;
    const int n = std::max(1, peoplesOf(g)), which = people > 0 && n > 1 ? 1 : 0;
    Rng r(peopleGen(g, which).seed ^ 0xA01E7ULL);
    e.landShare = landShareOf(g);
    drawExtent(r, age >= 0 ? age : peopleAgeOf(g, which).age, n, g.R, e.landShare, e.held, e.peak);
    return e;
}

// R-413 (2026-10-11): the two peoples of a world reached one age, the first's (the user's: "it won't make sense to have futuristic
// society next to prehistoric [...] it would be cool if both societes were of the same age"); the second's years in it are its own.
// A second people's own generator read as people 0 (the charts' and the records' categories' `peopleAgeOf(pg, 0)`) is that people:
// its age is not its generator's draw alone, so the world's seed it keeps (`BodyGen::worldSeed`) says whose it is
PeopleAge peopleAgeOf(const BodyGen& g, int people) {
    if (people <= 0 && g.worldSeed && g.seed != g.worldSeed) {
        BodyGen w = g; w.seed = g.worldSeed;
        if (peopleGen(w, 1).seed == g.seed) return peopleAgeOf(w, 1);
    }
    PeopleAge first = ageOfPeople(peopleGen(g, 0));
    if (g.drifter) first.endedYearsAgo = sunLossOf(g, first).ended;   // after the drifter left its star
    if (people <= 0) return first;
    PeopleAge a = ageOfPeople(peopleGen(g, people), first.age);
    if (peoplesEndedTogether(g)) a.endedYearsAgo = first.endedYearsAgo;   // one catastrophe for both
    return a;
}

int peopleFirstAgeOf(uint64_t worldSeed) { Rng r(worldSeed ^ 0xA01A6EULL); return r.pick(AGE_SHARES, AGE_COUNT); }

int peoplesOnAir(const BodyGen& g) {
    int n = peoplesOf(g), mask = 0;
    for (int k = 0; k < n; k++) if (peopleAgeOf(g, k).radio) mask |= 1 << k;
    return mask;
}

bool causeWorldwide(int c) { return c >= 0 && c < CAUSE_COUNT && CAUSES[c].worldwide; }
int causeSpeed(int c) { return c >= 0 && c < CAUSE_COUNT ? CAUSES[c].speed : 1; }
const char* const CAUSE_NAMES[CAUSE_COUNT] = {
    CAUSES[0].name, CAUSES[1].name, CAUSES[2].name, CAUSES[3].name, CAUSES[4].name, CAUSES[5].name, CAUSES[6].name, CAUSES[7].name, CAUSES[8].name, CAUSES[9].name,
    CAUSES[10].name, CAUSES[11].name, CAUSES[12].name, CAUSES[13].name, CAUSES[14].name, CAUSES[15].name, CAUSES[16].name, CAUSES[17].name, CAUSES[18].name, CAUSES[19].name,
    CAUSES[20].name, CAUSES[21].name, CAUSES[22].name, CAUSES[23].name, CAUSES[24].name, CAUSES[25].name, CAUSES[26].name, CAUSES[27].name, CAUSES[28].name, CAUSES[29].name,
    CAUSES[30].name, CAUSES[31].name, CAUSES[32].name, CAUSES[33].name, CAUSES[34].name, CAUSES[35].name, CAUSES[36].name, CAUSES[37].name};
static_assert(CAUSE_COUNT == 38, "CAUSE_NAMES lists every cause");
const char* const SUN_WAY_NAMES[SW_COUNT] = {"the wanderer", "the intruder", "the dark", "the dance"};

// the latest stage begun by the year's end (the collapse's share gone by then), so its last year is the end's even when the
// whole collapse took a year or two
int PeopleProfile::stageAt(int year) const {
    if (year < collapseYear) return -1;
    double f = (year - collapseYear + 1) / (double)std::max(1, collapseYears);
    int s = STAGE_WARNING;
    for (int k = 1; k < STAGE_COUNT; k++) if (f > stage[k]) s = k;
    return s;
}
int PeopleProfile::eraAt(int year) const {
    for (int k = 0; k < (int)eras.size(); k++) if (year <= eras[k].end) return k;
    return eras.empty() ? 0 : (int)eras.size() - 1;
}

PeopleProfile peopleProfileOf(const StarSystem& sys0, const Body& b0, const BodyGen& g, int people) {
    const PeoplesWorld PW = peoplesWorldOf(sys0, b0);   // D-05: a drifter's people's world at its former orbit
    const StarSystem& sys = *PW.sys; const Body& b = *PW.b;
    PeopleProfile P;
    int n = std::max(1, peoplesOf(g));
    P.peoples = n; P.which = people > 0 && n > 1 ? 1 : 0;
    const BodyGen pg = peopleGen(g, P.which);
    P.seed = pg.seed;
    static_cast<PeopleAge&>(P) = peopleAgeOf(g, P.which);
    const WorldRead w = readWorld(sys, b, g);
    // the cause: two peoples that ended together share one that can end a world, drawn from the world's seed by their age (R-413:
    // one age); else the people's own, by its age and the world, and where the other people went on after it, by them (conquest, a war)
    double cw[CAUSE_COUNT];
    bool together = n > 1 && peoplesEndedTogether(g);
    PeopleAge other; if (n > 1) other = peopleAgeOf(g, 1 - P.which);
    int lastLen = P.lastYear - P.eras.back().start + 1;
    if (g.drifter) {   // the loss of the sun, given (the 37 never drawn for them): its length and stages the generator's (`sunLossOf`), its way by
        // what the system it left had (a giant to throw it, a companion to dance with), the world's own draw (two peoples lost one sun)
        const PeopleAge first = n > 1 && P.which > 0 ? peopleAgeOf(g, 0) : static_cast<const PeopleAge&>(P);
        const SunLoss L = sunLossOf(g, first);
        P.cause = CAUSE_SUN_LOST;
        P.collapseYears = std::max(1, std::min(L.years, lastLen));
        for (int k = 0; k < STAGE_COUNT; k++) P.stage[k] = L.stage[k];
        P.collapseYear = P.lastYear - P.collapseYears + 1;
        int endFrom = P.collapseYear;
        while (endFrom < P.lastYear && P.stageAt(endFrom) < STAGE_END) endFrom++;
        P.lastSunrise = std::max(P.collapseYear, endFrom - 1);
        int giant = -1; double gd = 1e300;
        for (const Body& o : sys.bodies) {
            if (o.parent >= 0 || o.former || !(KINDS[o.type].tags & KT_GIANT)) continue;
            const double d = std::fabs(std::log(std::max(o.orbitRadiusKm, 1.0) / std::max(b.orbitRadiusKm, 1.0)));
            if (d < gd) { gd = d; giant = o.index; }
        }
        P.sunWay = drifterSunWayOf(g.worldSeed ? g.worldSeed : g.seed);   // the system it left was chosen to allow it (`drifterPast`); where none did, the intruder's
        if ((P.sunWay == SW_WANDERER && giant < 0) || (P.sunWay == SW_DANCE && sys.companion < 0)) P.sunWay = SW_INTRUDER;
        P.sunGiant = P.sunWay == SW_WANDERER ? giant : -1;
    } else if (together) {
        int otherLen = other.lastYear - other.eras.back().start + 1;
        for (int c = 0; c < CAUSE_DRAWN; c++) cw[c] = CAUSES[c].worldwide ? CAUSES[c].w[P.age] * causeWorldFactor(c, w) : 0;
        Rng rs(g.seed ^ 0xA01C5ULL);
        P.cause = rs.pick(cw, CAUSE_DRAWN);
        double years = logUni(rs, CAUSES[P.cause].lo, CAUSES[P.cause].hi);
        P.collapseYears = std::max(1, (int)std::lround(std::min(years, 0.85 * std::min(lastLen, otherLen))));
        drawStages(rs, CAUSES[P.cause].speed, P.stage);
    } else {
        bool otherWentOn = n > 1 && other.endedYearsAgo < P.endedYearsAgo;
        for (int c = 0; c < CAUSE_DRAWN; c++) {
            cw[c] = CAUSES[c].w[P.age] * causeWorldFactor(c, w);
            if (!otherWentOn) continue;   // by the other people: a band's war with them, a conquest by them, an industrial war
            if (c == CAUSE_BAND_WAR && P.age == AGE_PREHISTORIC) cw[c] *= 2;
            if (c == CAUSE_CONQUEST && P.age > AGE_PREHISTORIC) cw[c] = std::max(cw[c], 0.5) * 3;
            if (c == CAUSE_INDUSTRIAL_WAR) cw[c] *= 1.5;
        }
        Rng rc(pg.seed ^ 0xA01CAULL);
        P.cause = rc.pick(cw, CAUSE_DRAWN);
        double years = logUni(rc, CAUSES[P.cause].lo, CAUSES[P.cause].hi);
        P.collapseYears = std::max(1, (int)std::lround(std::min(years, 0.85 * lastLen)));
        drawStages(rc, CAUSES[P.cause].speed, P.stage);
    }
    if (!g.drifter) P.collapseYear = P.lastYear - P.collapseYears + 1;
    // the extent: the land of their time, the share they held (a people of a world of two held its side's), how dense they lived
    // (T-01: drawn by `drawExtent`, which `peopleExtentOf` shares, so the plan of the world reads the same numbers without the system)
    Rng r(pg.seed ^ 0xA01E7ULL);
    P.landShare = landShareOf(g);
    drawExtent(r, P.age, n, b.radiusKm, P.landShare, P.held, P.peak);
    // how they were governed at the end, what they lived by, what they believed and valued
    P.government = P.eras.back().government;
    double ew[ECO_COUNT];
    for (int e = 0; e < ECO_COUNT; e++) {
        ew[e] = ECO_W[P.age][e];
        if (w.desert) ew[e] *= e == ECO_HERDING ? 2.0 : (e == ECO_FARMING ? 0.6 : (e == ECO_HUNTING ? 0.7 : 1.0));
        if (P.landShare < 0.25 && e == ECO_TRADE) ew[e] *= 1.5;   // a world of islands trades
    }
    P.economy = r.pick(ew, ECO_COUNT);
    ew[P.economy] = 0;
    double rest = 0; for (int e = 0; e < ECO_COUNT; e++) rest += ew[e];
    P.economy2 = rest > 0 ? r.pick(ew, ECO_COUNT) : P.economy;
    P.faith = r.pick(FAITH_W[P.age], FAITH_COUNT);
    P.rite = r.pick(RITE_W[P.faith], RITE_COUNT);
    P.holy = r.pick(HOLY_W[P.faith], HOLY_COUNT);
    double sum = 0;
    for (int v = 0; v < VAL_COUNT; v++) P.values[v] = -std::log(1.0 - r.uni() * 0.999);
    static const double AGE_TILT[AGE_COUNT][VAL_COUNT] = {{1, 1.4, 1, 0.6, 1}, {1, 1.3, 1, 0.9, 1}, {1, 1, 1.2, 1.1, 1}, {1, 0.8, 1.1, 1.2, 1}, {0.9, 0.6, 1, 1.4, 1.1}, {0.5, 0.8, 0.7, 1.5, 1.3}};
    for (int v = 0; v < VAL_COUNT; v++) P.values[v] *= AGE_TILT[P.age][v];
    switch (P.government) {
        case GOV_CHIEFS: P.values[VAL_WAR] *= 1.4; break;
        case GOV_MONARCHY: P.values[VAL_WAR] *= 1.5; break;
        case GOV_COUNCIL: P.values[VAL_PIETY] *= 1.2; break;
        case GOV_THEOCRACY: P.values[VAL_PIETY] *= 2.5; break;
        case GOV_REPUBLIC: P.values[VAL_TRADE] *= 1.3; P.values[VAL_LEARNING] *= 1.3; break;
        case GOV_ASSEMBLY: P.values[VAL_LEARNING] *= 1.3; P.values[VAL_ART] *= 1.3; break;
        case GOV_CORPORATE: P.values[VAL_TRADE] *= 2.0; break;
        default: P.values[VAL_LEARNING] *= 2.0; P.values[VAL_ART] *= 0.6; P.values[VAL_PIETY] *= 0.5; break;   // a machine
    }
    if (P.economy == ECO_TRADE) P.values[VAL_TRADE] *= 1.5;
    if (P.economy == ECO_HERDING) P.values[VAL_WAR] *= 1.2;
    if (P.economy == ECO_INFORMATION) P.values[VAL_LEARNING] *= 1.5;
    if (P.faith == FAITH_PHILOSOPHY) { P.values[VAL_LEARNING] *= 1.3; P.values[VAL_PIETY] *= 0.7; }
    if (P.faith == FAITH_ONE_GOD) P.values[VAL_PIETY] *= 1.3;
    for (int v = 0; v < VAL_COUNT; v++) sum += P.values[v];
    for (int v = 0; v < VAL_COUNT; v++) P.values[v] /= sum;
    return P;
}

double sunLightOf(int onsetYear, int lastSunrise, int year) {
    const int ls = std::max(onsetYear, lastSunrise);
    if (year < onsetYear) return 1;
    if (year > ls) return 0;
    const double p = (year - onsetYear + 1) / (double)(ls - onsetYear + 1), d = 1 + 9 * p * p;
    return 1 / (d * d);
}
int sunDarkOf(double light, int year, int lastSunrise) { return year > lastSunrise ? 3 : (light < 0.25 ? 2 : (light < 0.9 ? 1 : 0)); }
bool sunLossYears(const BodyGen& g, int people, int& onset, int& lastSunrise) {
    if (!g.drifter) return false;
    const int n = std::max(1, peoplesOf(g)), which = people > 0 && n > 1 ? 1 : 0;
    PeopleProfile P;
    static_cast<PeopleAge&>(P) = peopleAgeOf(g, which);
    const SunLoss L = sunLossOf(g, which > 0 ? peopleAgeOf(g, 0) : static_cast<const PeopleAge&>(P));
    const int lastLen = P.lastYear - P.eras.back().start + 1;
    P.collapseYears = std::max(1, std::min(L.years, lastLen));
    for (int k = 0; k < STAGE_COUNT; k++) P.stage[k] = L.stage[k];
    P.collapseYear = P.lastYear - P.collapseYears + 1;
    onset = P.collapseYear; while (onset < P.lastYear && P.stageAt(onset) < STAGE_ONSET) onset++;
    int endFrom = onset; while (endFrom < P.lastYear && P.stageAt(endFrom) < STAGE_END) endFrom++;
    lastSunrise = std::max(P.collapseYear, endFrom - 1);
    return true;
}
int sunDarkAt(const BodyGen& g, int people, int year) {
    int onset = 0, lastSunrise = 0;
    if (!sunLossYears(g, people, onset, lastSunrise)) return 0;
    return sunDarkOf(sunLightOf(onset, lastSunrise, year), year, lastSunrise);
}

std::string peopleCount(double n) {
    char buf[48];
    if (n < 1e3) snprintf(buf, sizeof buf, "%.0f", n);
    else if (n < 1e6) snprintf(buf, sizeof buf, "%.0f thousand", n / 1e3);
    else if (n < 1e7) snprintf(buf, sizeof buf, "%.1f million", n / 1e6);
    else if (n < 1e9) snprintf(buf, sizeof buf, "%.0f million", n / 1e6);
    else if (n < 1e10) snprintf(buf, sizeof buf, "%.1f billion", n / 1e9);
    else snprintf(buf, sizeof buf, "%.0f billion", n / 1e9);
    return buf;
}
