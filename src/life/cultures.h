#pragma once
// Z-05 (2026-11-17): a culture in the incubator's flasks (R-425; docs/archive/ideas/IDEAS-small-and-spacious.md 1.8, ITERATION-1.5.0.md Z-05). A flask
// holds a medium (its cell, a chemistry's solvent: water, the acid cell, the ammonia cell, the cryostat; its temperature, its light, its energy,
// its salt) and one or two species put in it (a vial's cells or spores, or a flask's culture carried over). Its growth is Monod's (1949) as
// predictive microbiology writes it: a lag (Baranyi and Roberts 1994), the rise at the medium's rate (its temperature on a cardinal curve of the
// species' three, Rosso et al. 1993; its energy, its light by its pigment, its salt), the crowd its energy's yield allows, and after a while
// at it the decline: spores (a spore former), cysts (a large cell), the gathered amoebae's fruiting bodies, or lysis. Spores wake once the
// medium suits them. Two in a flask: one wins (Gause 1934), predator and prey cycle (Lotka 1925, Volterra 1926), a light-feeder and a
// heterotroph or a fermenter and a hydrogen-eater feed each other, mirrored handedness ignore each other. A culture grown at its range's
// edge gives a strain, one value moved (Lenski and Travisano 1994). What a flask holds at a time is closed-form in its clock from its events:
// nothing is integrated a frame at a time (`runCulture` lays its laws out once, `cultureAt` reads them). Y-04: a line's genes switched to a
// condition: each stretch's laws by its state (the light, the cold against its best, starving, its crowd's quorum share), a law ending where
// its crowd crosses that share. J-01: a line's world genes (`CultureWorld`): its crowd moved or never stopped (STOP), its flask frozen by its
// ice (FROST), its shield under the ultraviolet light, its poison against the other in its flask
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
// J-13 (R-454): a brine flask's water (salt 2) goes down to BRINE_COLDEST_C, a supercooled brine that freezes only round a culture's ice
void cellRange(int cell, double& loC, double& hiC, int salt = 0);
constexpr double WATER_COLDEST_C = -20, BRINE_COLDEST_C = -45;
// J-01: and the ultraviolet lamp (a shield's test: little of it feeds a pigment, and it kills a cell with none)
enum CultureLight { CL_DARK = 0, CL_WHITE, CL_RED, CL_BLUE, CL_UV, CL_COUNT };
extern const char* const CULTURE_LIGHT_NAMES[CL_COUNT];   // "DARK", "WHITE", "RED", "BLUE", "UV"
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

// Y-04: a gene's switch (its promoter, life/genes.h; Jacob and Monod 1961), when it does its work: always, never, in the light, in the dark, its
// crowd past its quorum's share (a glow's own), starving (nothing it eats, no light or its medium killing it, judged of the line as written: a
// rest's own; B-477: a full flask is fed, not starving), colder than its best. The guide keeps a switch by its key
enum GeneSwitch { GW_ALWAYS = 0, GW_NEVER, GW_LIGHT, GW_DARK, GW_CROWDED, GW_STARVING, GW_COLD, GW_COUNT };
extern const char* const GENE_SWITCH_NAMES[GW_COUNT];   // "ALWAYS", "NEVER", "IN THE LIGHT", "IN THE DARK", "WHEN CROWDED", "WHEN STARVING", "WHEN COLD"
extern const char* const GENE_SWITCH_KEYS[GW_COUNT];    // "ALWAYS", "NEVER", "LIGHT", "DARK", "CROWDED", "STARVING", "COLD"
// a culture's state as a switch reads it (bits): its flask lit, colder than its best, its crowd past its quorum's share, starving
enum CultureState : uint32_t { CST_LIGHT = 1, CST_COLD = 2, CST_CROWDED = 4, CST_STARVING = 8 };
bool switchOn(int sw, uint32_t state);
// what a gene off loses in its culture, as one cut (`applyEdits`): its life (a vital gene), its warmth's range (round its best), its salt (fresh
// water), its pigment, its food, its rest at its end (`bits`: its cycle's), its glow; a motion, a sense, a size or a shape its drop's alone;
// J-01: a world gene its work (`CultureWorld`: its gas, dye, lift, frost, slime, shell, its stop (it never stops), its binding, its rot, poison,
// carrying, host, its life dried, its shield)
enum CultureWork { CX_NONE = 0, CX_LIFE, CX_WARMTH, CX_SALT, CX_PIGMENT, CX_FOOD, CX_REST, CX_GLOW,
                   CX_GAS, CX_DYE, CX_LIFT, CX_FROST, CX_SLIME, CX_SHELL, CX_STOP, CX_BIND, CX_ROT, CX_POISON, CX_CARRIER, CX_HOST, CX_DRY, CX_SHIELD };
// a line's gene switched to a condition: its place, its switch, what it loses off, its price (its part of the line's burden: not paid off)
struct CultureSwitch { int place = 0, when = GW_ALWAYS, work = CX_NONE; double price = 0; uint32_t bits = 0; };

// J-01: what a line's world genes make and do (life/genes.h, places 19 to 32; IDEAS-seeded-worlds.md section 4), a wild species' read from what
// it is: the gas it gives off (Thauer et al. 2008's methanogens; an oxygen-maker's in its light) and how much, the dye its cells leak (red snow's
// astaxanthin: Remias et al. 2005), its lift (gas vesicles: Walsby 1994), the warmth its ice forms at (an ice-nucleating protein: Lindow et al.
// 1982; none, the flask's water stays liquid below its freezing), its slime, its shell's mineral or sediment and how much, its crowd moved
// (decades from its species'; `overgrows`: it never stops), its cells bound after dividing, what it rots and how fast, its poison (a toxin, its
// target an animal's: a microbe it kills in a flask) and its strength, how readily its genes pass to others (Griffith 1928), the host it lives
// in and how deep, how long it lives dried out (anhydrobiosis: Billi and Potts 2002), the ultraviolet it takes. Amounts 0 to 1 (nothing below a
// quarter of its gene's weight); `FROST_NONE` no ice
enum WorldGas { WG_NONE = 0, WG_CO2, WG_METHANE, WG_OXYGEN, WG_H2S, WG_DMS, WG_N2O, WG_AMMONIA, WG_HYDROGEN, WG_COUNT };
extern const char* const WORLD_GAS_NAMES[WG_COUNT];     // "NO GAS", "CO2", "METHANE", "OXYGEN", "HYDROGEN SULPHIDE", ..., "HYDROGEN"
// R-453: GREEN (a green alga's chlorophyll) and PINK (a salt lake's carotenoids) after J-01's, appended so nothing before them moves
enum WorldDye { WD_NONE = 0, WD_RED, WD_CRIMSON, WD_ORANGE, WD_YELLOW, WD_VIOLET, WD_BLUE, WD_BLACK, WD_FLUORESCENT, WD_GREEN, WD_PINK, WD_COUNT };
extern const char* const WORLD_DYE_NAMES[WD_COUNT];     // "NO DYE", "RED", "CRIMSON", ... "BLACK", "FLUORESCENT", "GREEN", "PINK"
enum WorldShell { SHL_NONE = 0, SHL_CHALK, SHL_GLASS, SHL_RUST, SHL_SULPHUR, SHL_COUNT };
extern const char* const WORLD_SHELL_NAMES[SHL_COUNT];  // "NONE", "CHALK", "GLASS", "RUST", "SULPHUR"
enum WorldRot { ROT_NONE = 0, ROT_WOOD, ROT_FLESH, ROT_SHELL, ROT_STONE, ROT_COUNT };
extern const char* const WORLD_ROT_NAMES[ROT_COUNT];    // "NOTHING", "WOOD", "FLESH", "SHELL", "STONE"
enum WorldPoison { PZ_NONE = 0, PZ_NERVE, PZ_BLOOD, PZ_GUT, PZ_COUNT };
extern const char* const WORLD_POISON_NAMES[PZ_COUNT];  // "NONE", "NERVE", "BLOOD", "GUT"
enum WorldHost { HO_NONE = 0, HO_PLANTS, HO_ANIMALS, HO_BOTH, HO_COUNT };
extern const char* const WORLD_HOST_NAMES[HO_COUNT];    // "NONE", "PLANTS", "ANIMALS", "PLANTS AND ANIMALS"
constexpr double FROST_NONE = -99;
struct CultureWorld {
    int gas = WG_NONE; double gasRate = 0;
    int dye = WD_NONE; double dyeRate = 0;
    double lift = 0;
    double frostC = FROST_NONE;
    double slime = 0;
    int shell = SHL_NONE; double shellRate = 0;
    double stopShift = 0; bool overgrows = false;
    double bind = 0;
    int rot = ROT_NONE; double rotRate = 0;
    int poison = PZ_NONE; double poisonRate = 0;
    double carrier = 0;
    int host = HO_NONE; double hostRate = 0;
    double dry = 0, shield = 0;
    bool gives() const { return gas != WG_NONE && gasRate > 0; }
    bool dyes() const { return dye != WD_NONE && dyeRate > 0; }
    bool shells() const { return shell != SHL_NONE && shellRate > 0; }
    bool poisons() const { return poison != PZ_NONE && poisonRate > 0; }
    bool rots() const { return rot != ROT_NONE && rotRate > 0; }
    bool hosted() const { return host != HO_NONE && hostRate > 0; }
};
constexpr double OVERGROWN_DECADES = 2;   // J-01: a line that never stops, its crowd past its food's
constexpr double UV_SHIELD = 0.35;         // the shield a cell needs to grow under the ultraviolet light
double freezingC(int salt);                // where a flask's water freezes: fresh 0 C, the sea's -2, brine -20
// a flask frozen: water's cell, colder than its salt's freezing, at or under the ice's warmth of the culture that forms it (none: liquid)
bool flaskIce(const Medium& m, double frostC);
// J-13 (R-454): a line that lives in brine and makes no ice of its own (its FROST none, cut or switched off): its warmth gene can take it down
// to BRINE_COLDEST_C, where other water life stops at WATER_COLDEST_C (perchlorate brines stay liquid far colder: Toner et al. 2014)
struct CultureKind;
bool brineAntifreeze(const CultureKind& k);
// the looks' colours: a dye's in water, a shell's sediment (chalk, glass, rust, sulphur), a culture grown into tissue, a flask's ice
RGB dyeColour(int dye);
RGB shellColour(int shell);
constexpr RGB TISSUE_COLOUR(0.72f, 0.28f, 0.32f), ICE_COLOUR(0.82f, 0.9f, 1.0f);

// what a culture reads of a species: its values (a strain's moves applied), its world's chemistry, its handedness against home's, its body
// across (who eats whom), whether it went in as spores or cysts; Y-02: an edited line's (life/genes.h): whether it lives (a vital gene changed:
// it does not), the share of its crowd its glow begins at; Y-03: the share of its growth its genes cost it (`geneBurden`: genes not its own, a
// motion or a glow past its species' own), its best rate's; Y-04: its genes switched to a condition (none: its kind in every state), its glow
// lit by its switch (any cells, while on) rather than at its crowd's share; J-01: what its world genes make and do
struct CultureKind {
    Microbe s;
    int chemistry = CH_WATER;
    bool mirrored = false;
    double bodyUm = 1;
    bool spores = false;
    bool dead = false;
    double quorum = 0.03;
    double burden = 0;
    std::vector<CultureSwitch> switches;
    bool glowAny = false;
    CultureWorld world;
};
// Y-04: a work lost (a gene cut, or off by its switch); a kind in a state: its switched genes off there lose their work and their price
void cultureWorkOff(CultureKind& k, int work, uint32_t bits = 0);
CultureKind switchedKind(const CultureKind& k, uint32_t state);
// a world's species as a flask takes it (its body across its form's: `Morph::bodyUm` twice); a dormant one goes in as its spores or cysts
CultureKind cultureKindOf(const Biota& B, int species, double bodyUm);
// AS ITS WORLD (IDEAS 1.8: the first guess): the medium of a sample's place: its chemistry's cell, the warmth read at the take, its sun's light
// where the sun reaches it, the energy its place offers (a spring's or a vent's sulphide, a plume's and the deep rock's hydrogen, a rust world's
// iron, the methane of the ammonia worlds, the acetylene of the hydrocarbon ones, else the broth of its dead matter), its salt
Medium mediumOfPlace(const Biota& B, int bodyType, int starClass, const Microhabitat& h, double readingC);

// J-16 (KI-507): an acid's water carries dissolved iron and sulphur (acid mine waters, Colmer and Hinkle 1947; a venusian deck's sulphur and
// iron chloride), so an iron or a sulphide eater finds its food in an acid medium whatever else it gives
inline bool acidFeeds(const Medium& m, int substrate) { return m.cell == CC_ACID && (substrate == SU_IRON || substrate == SU_SULPHIDE); }
// how a species does in a medium: grows; too cold (it waits); too hot, the wrong salt or the wrong solvent (it dies, or sporulates); no energy it
// burns, no light for a light-feeder (it starves); Y-02: a line whose vital gene was changed (it lyses at once, anywhere); J-01: its flask
// frozen (it waits in the ice), the ultraviolet light without a shield (it dies)
enum GrowthFit { GF_GROWS = 0, GF_COLD, GF_HOT, GF_SALT, GF_SOLVENT, GF_HUNGRY, GF_DARK, GF_UNVIABLE, GF_FROZEN, GF_BURNT, GF_COUNT };
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
// Y-02: the stage its starved crowd declines into (spores, cysts, the gatherers' fruiting, or dying)
int cultureDecline(const CultureKind& k);

// what is done to a flask: the culture started (its first species), the medium changed (the incubator's controls: a change of energy, salt or
// cell is a fresh broth of it), a fresh broth (the culture diluted a hundredfold into new medium: a serial transfer), a second species added,
// a strain arisen (a species' value moved: `what` and `value`, from `cultureStrain`)
enum CultureDo { CD_START = 0, CD_MEDIUM, CD_FRESH, CD_ADD, CD_STRAIN, CD_COUNT };
struct CultureEvent { int kind = CD_START; double t = 0; Medium m; int who = 0, what = 0; double value = 0; };
// a strain's move: its highest warmth up, its lowest down, its size up (Atkinson 1994's cold rule), its salt's range, its pigment; Y-02: a line
// written by the gene editor (its edits the guide's, life/genes.h: `applyStrain` moves nothing)
enum StrainMove { SM_NONE = 0, SM_HOT, SM_COLD, SM_SIZE, SM_SALT_LO, SM_SALT_HI, SM_PIGMENT, SM_EDIT, SM_COUNT };
void applyStrain(CultureKind& k, int what, double value);
struct FlaskCulture {
    CultureKind kind[2];               // the first put in, the second added (CD_ADD)
    std::vector<CultureEvent> events;  // in time's order, CD_START first
};

// the stages a culture is in, each species its own (IDEAS 1.8): spores asleep, waking, its lag, growing, in colonies, at its crowd, starving,
// too cold to grow, gathering, fruiting, sporulating, encysting, dying, dead; cycling with a predator or its prey; outgrown by another; J-01:
// overgrown (past its crowd, never stopping), frozen in its flask's ice, poisoned by the other
enum CultureStage { SG_EMPTY = 0, SG_ASLEEP, SG_WAKING, SG_LAG, SG_GROWING, SG_COLONY, SG_CROWD, SG_HUNGRY, SG_WAITING, SG_GATHERING, SG_FRUITING,
                    SG_SPORULATING, SG_ENCYSTING, SG_DYING, SG_DEAD, SG_CYCLING, SG_OUTGROWN, SG_OVERGROWN, SG_FROZEN, SG_POISONED, SG_COUNT };
extern const char* const CULTURE_STAGE_NAMES[SG_COUNT];
uint32_t stageCycleBit(int stage);   // the LifeCycle bit a stage shows of its species (0 none)
// two in a flask: one wins, they cycle, they feed each other, mirrored they ignore each other; J-01: one poisons the other
enum CulturePair { PA_NONE = 0, PA_COMPETE, PA_CYCLE, PA_FEED, PA_IGNORE, PA_POISON, PA_COUNT };
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
    double quorum = 0.03;                                         // Y-02: the share of its crowd its glow begins at (Y-04: 0, lit by its switch)
    bool capped = false;                                          // FEED: the fed one, held to a share of its feeder
    int state = -1;                                               // Y-04: a switched species' state (`CultureState`'s bits; -1 none)
};
struct CultureRun {
    int n = 0;                         // species in it (at the run's end)
    double tAdd = -1;                  // when the second came
    int pair = PA_NONE, lead = -1;     // their relation; the winner, the predator or the feeder
    double exT = -1, exRate = 0;       // COMPETE: when the winner took half its crowd, the loser's fall after it (J-01: POISON, a hundredth)
    double cycleT = -1, period = 0;    // CYCLE: when it began, its turn (s)
    std::vector<CultureSeg> seg[2];
    double wokeT[2] = {-1, -1};        // when its spores first woke (a species put in as spores)
    double strainT[2] = {-1, -1};      // when the next strain arises (from the last strain's time), its move
    int strainWhat[2] = {0, 0}; double strainValue[2] = {0, 0};
};
CultureRun runCulture(const FlaskCulture& c);
// what a flask holds at a time: each species' cells, spores or cysts, its lysed, its stage, its glow, how its medium suits it; Y-04: a switched
// species' state (-1 none) and its rate while it grows (its switches' price paid or not)
struct CultureOne {
    double cells = 0, spores = 0, ghosts = 0; int stage = SG_EMPTY; bool glowing = false; int fit = GF_GROWS; int edge = EG_NONE;
    int state = -1; double mu = 0;
};
struct CultureNow { int n = 0; CultureOne one[2]; };
CultureNow cultureAt(const FlaskCulture& c, const CultureRun& r, double t);
// the stages a species has passed by a time (LifeCycle bits), seen on its curve
uint32_t cultureStagesBy(const CultureRun& r, int who, double t);

constexpr double FLASK_QUICK = 0.2;         // R-449: a flask's times (its lag, its rise to its crowd, its spores' waking) a fifth of Z-05's
constexpr double STRAIN_GENERATIONS = 12;   // the generations at its range's edge a strain takes
constexpr double CULTURE_EXTINCT = 0.01;    // under a cell in a hundred millilitres: none left
