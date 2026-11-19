#pragma once
// Z-05 (2026-11-17): a culture in the incubator's flasks (R-425; docs/ideas/IDEAS-small-and-spacious.md 1.8, ITERATION-1.5.0.md Z-05). A flask
// holds a medium (its cell, a chemistry's solvent: water, the acid cell, the ammonia cell, the cryostat; its temperature, its light, its energy,
// its salt) and one or two species put in it (a vial's cells or spores, or a flask's culture carried over). Its growth is Monod's (1949) as
// predictive microbiology writes it: a lag (Baranyi and Roberts 1994), the rise at the medium's rate (its temperature on a cardinal curve of the
// species' three, Rosso et al. 1993; its energy, its light by its pigment, its salt), the crowd its energy's yield allows, and after a while
// at it the decline: spores (a spore former), cysts (a large cell), the gathered amoebae's fruiting bodies, or lysis. Spores wake once the
// medium suits them. Two in a flask: one wins (Gause 1934), predator and prey cycle (Lotka 1925, Volterra 1926), a light-feeder and a
// heterotroph or a fermenter and a hydrogen-eater feed each other, mirrored handedness ignore each other. A culture grown at its range's
// edge gives a strain, one value moved (Lenski and Travisano 1994). What a flask holds at a time is closed-form in its clock from its events:
// nothing is integrated a frame at a time (`runCulture` lays its laws out once, `cultureAt` reads them)
#include "life/biota.h"
#include "life/samples.h"
#include <cstdint>
#include <vector>

// the incubator's cells: water's, the acid cell, the ammonia cell, the cryostat (85-105 K, liquid methane); each a chemistry's and its range
enum CultureCell { CC_WATER = 0, CC_ACID, CC_AMMONIA, CC_CRYOSTAT, CC_COUNT };
extern const char* const CULTURE_CELL_NAMES[CC_COUNT];   // "WATER", "ACID", "AMMONIA", "CRYOSTAT" (the screen's medium line holds the longest)
int cellChemistry(int cell);
int chemistryCell(int chemistry);   // the cell a chemistry's life grows in (none: water's)
// a species' rate against its three temperatures, 0 to 1 (Rosso et al. 1993); R-438: the slide's pace reads it too
double cardinalRate(double T, double lo, double opt, double hi);
void cellRange(int cell, double& loC, double& hiC);
enum CultureLight { CL_DARK = 0, CL_WHITE, CL_RED, CL_BLUE, CL_COUNT };
extern const char* const CULTURE_LIGHT_NAMES[CL_COUNT];   // "DARK", "WHITE", "RED", "BLUE"
int lightOfStar(int starClass);     // the light a world's sun gives (a red dwarf's and a giant's red, the hot stars' blue, the rest white)
// the energy a medium gives: the substrates' (`Substrate`'s numbers: a chemical way burns its own) and the broth (the dead way's, a grazer's slow)
enum CultureEnergy { CE_NONE = SU_NONE, CE_HYDROGEN = SU_HYDROGEN, CE_SULPHIDE = SU_SULPHIDE, CE_IRON = SU_IRON, CE_METHANE = SU_METHANE,
                     CE_AMMONIA = SU_AMMONIA, CE_ACETYLENE = SU_ACETYLENE, CE_BROTH = SU_COUNT, CE_COUNT };
extern const char* const CULTURE_ENERGY_NAMES[CE_COUNT];   // "NONE", "HYDROGEN", ... "BROTH"
extern const char* const CULTURE_SALT_NAMES[3];            // "FRESH", "SEA SALT", "BRINE"
struct Medium {
    int cell = CC_WATER;
    double tempC = 30;
    int light = CL_DARK;
    int energy = CE_BROTH;
    int salt = 0;
    bool operator==(const Medium& o) const { return cell == o.cell && tempC == o.tempC && light == o.light && energy == o.energy && salt == o.salt; }
    bool operator!=(const Medium& o) const { return !(*this == o); }
};

// what a culture reads of a species: its values (a strain's moves applied), its world's chemistry, its handedness against home's, its body
// across (who eats whom), whether it went in as spores or cysts
struct CultureKind {
    Microbe s;
    int chemistry = CH_WATER;
    bool mirrored = false;
    double bodyUm = 1;
    bool spores = false;
};
// a world's species as a flask takes it (its body across its form's: `Morph::bodyUm` twice); a dormant one goes in as its spores or cysts
CultureKind cultureKindOf(const Biota& B, int species, double bodyUm);
// AS ITS WORLD (IDEAS 1.8: the first guess): the medium of a sample's place: its chemistry's cell, the warmth read at the take, its sun's light
// where the sun reaches it, the energy its place offers (a spring's or a vent's sulphide, a plume's and the deep rock's hydrogen, a rust world's
// iron, the methane of the ammonia worlds, the acetylene of the hydrocarbon ones, else the broth of its dead matter), its salt
Medium mediumOfPlace(const Biota& B, int bodyType, int starClass, const Microhabitat& h, double readingC);

// how a species does in a medium: grows; too cold (it waits); too hot, the wrong salt or the wrong solvent (it dies, or sporulates); no energy it
// burns, no light for a light-feeder (it starves)
enum GrowthFit { GF_GROWS = 0, GF_COLD, GF_HOT, GF_SALT, GF_SOLVENT, GF_HUNGRY, GF_DARK, GF_COUNT };
extern const char* const GROWTH_FIT_NAMES[GF_COUNT];
// the edge of its range a growing culture is held at (a strain comes of it): its warmth's either side, its salt, a light its pigment takes poorly
enum CultureEdge { EG_NONE = 0, EG_HOT, EG_COLD, EG_SALT, EG_LIGHT, EG_COUNT };
struct Growth {
    double mu = 0;          // its rate (1/s)
    double crowdLog = 0;    // its crowd: log10 cells a millilitre
    int fit = GF_GROWS;
    int edge = EG_NONE;
};
// `fed`: its food is another in the flask (a grazer's prey, a heterotroph's light-feeder, a hydrogen-eater's fermenter)
Growth growthIn(const CultureKind& k, const Medium& m, bool fed = false);
// its doubling at its best (s), and where it is best: the warmth, the energy, the light
double bestDoublingS(const CultureKind& k);

// what is done to a flask: the culture started (its first species), the medium changed (the incubator's controls: a change of energy, salt or
// cell is a fresh broth of it), a fresh broth (the culture diluted a hundredfold into new medium: a serial transfer), a second species added,
// a strain arisen (a species' value moved: `what` and `value`, from `cultureStrain`)
enum CultureDo { CD_START = 0, CD_MEDIUM, CD_FRESH, CD_ADD, CD_STRAIN, CD_COUNT };
struct CultureEvent { int kind = CD_START; double t = 0; Medium m; int who = 0, what = 0; double value = 0; };
// a strain's move: its highest warmth up, its lowest down, its size up (Atkinson 1994's cold rule), its salt's range, its pigment
enum StrainMove { SM_NONE = 0, SM_HOT, SM_COLD, SM_SIZE, SM_SALT_LO, SM_SALT_HI, SM_PIGMENT, SM_COUNT };
void applyStrain(CultureKind& k, int what, double value);
struct FlaskCulture {
    CultureKind kind[2];               // the first put in, the second added (CD_ADD)
    std::vector<CultureEvent> events;  // in time's order, CD_START first
};

// the stages a culture is in, each species its own (IDEAS 1.8): spores asleep, waking, its lag, growing, in colonies, at its crowd, starving,
// too cold to grow, gathering, fruiting, sporulating, encysting, dying, dead; cycling with a predator or its prey; outgrown by another
enum CultureStage { SG_EMPTY = 0, SG_ASLEEP, SG_WAKING, SG_LAG, SG_GROWING, SG_COLONY, SG_CROWD, SG_HUNGRY, SG_WAITING, SG_GATHERING, SG_FRUITING,
                    SG_SPORULATING, SG_ENCYSTING, SG_DYING, SG_DEAD, SG_CYCLING, SG_OUTGROWN, SG_COUNT };
extern const char* const CULTURE_STAGE_NAMES[SG_COUNT];
uint32_t stageCycleBit(int stage);   // the LifeCycle bit a stage shows of its species (0 none)
// two in a flask: one wins, they cycle, they feed each other, mirrored they ignore each other
enum CulturePair { PA_NONE = 0, PA_COMPETE, PA_CYCLE, PA_FEED, PA_IGNORE, PA_COUNT };
extern const char* const CULTURE_PAIR_NAMES[PA_COUNT];
// how two would meet in a flask: the relation, the eater or the feeder (0 the first, 1 the second; -1 neither)
int pairOf(const CultureKind& a, const CultureKind& b, int& lead);

// a species' law over a stretch of the flask's clock (internal to the run, exposed for the harness's tables)
struct CultureSeg {
    double t0 = 0;
    int mode = 0;
    double N = 0, q = 0, Sp = 0, G = 0;   // at t0: cells, the lag's state (Baranyi's q), spores or cysts, the lysed (a millilitre)
    double mu = 0, K = 0;                 // the growth's rate and crowd
    double kd = 0, sigma = 0;             // the decline's rate and its share that go to spores, cysts or fruiting bodies
    int stage = SG_EMPTY;                 // what its decline or its rest is
    double declineT0 = -1;                // when its decline began (the gatherers' fruiting)
    double x0 = 0, y0 = 0, r = 0, m = 0, Pstar = 0, Zstar = 0;   // CYCLE: the joint law (log deviations, the prey's rate, the predator's death)
    int role = 0;                                                 // CYCLE: 0 the prey, 1 the predator
    int fit = GF_GROWS, edge = EG_NONE;                           // how its medium suits it
    bool glows = false, colony = false;
    bool capped = false;                                          // FEED: the fed one, held to a share of its feeder
};
struct CultureRun {
    int n = 0;                         // species in it (at the run's end)
    double tAdd = -1;                  // when the second came
    int pair = PA_NONE, lead = -1;     // their relation; the winner, the predator or the feeder
    double exT = -1, exRate = 0;       // COMPETE: when the winner took half its crowd, the loser's fall after it
    double cycleT = -1, period = 0;    // CYCLE: when it began, its turn (s)
    std::vector<CultureSeg> seg[2];
    double wokeT[2] = {-1, -1};        // when its spores first woke (a species put in as spores)
    double strainT[2] = {-1, -1};      // when the next strain arises (from the last strain's time), its move
    int strainWhat[2] = {0, 0}; double strainValue[2] = {0, 0};
};
CultureRun runCulture(const FlaskCulture& c);
// what a flask holds at a time: each species' cells, spores or cysts, its lysed, its stage, its glow, how its medium suits it
struct CultureOne { double cells = 0, spores = 0, ghosts = 0; int stage = SG_EMPTY; bool glowing = false; int fit = GF_GROWS; int edge = EG_NONE; };
struct CultureNow { int n = 0; CultureOne one[2]; };
CultureNow cultureAt(const FlaskCulture& c, const CultureRun& r, double t);
// the stages a species has passed by a time (LifeCycle bits), seen on its curve
uint32_t cultureStagesBy(const CultureRun& r, int who, double t);

constexpr double STRAIN_GENERATIONS = 12;   // the generations at its range's edge a strain takes
constexpr double CULTURE_EXTINCT = 0.01;    // under a cell in a hundred millilitres: none left
