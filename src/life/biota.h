#pragma once
// Z-01 (2026-11-16): where life is (R-425; docs/ideas/IDEAS-small-and-spacious.md section 1, docs/ideas/ITERATION-1.5.0.md). Every
// landable world has a biota: the stage its life reached, the chemistry it is built on, the genesis it descends from (life/lineages.h)
// and its species, each with a niche among the world's microhabitats, a way to live, its limits, a life cycle, a behaviour, a body plan
// and the seed its form grows from (Z-02). A row of BIOTA a kind says what its worlds can hold; `biotaOf` draws a world's on salts of
// its own, after every draw of generation 16, from the body, its system and its traits (`BodyGen::make`): no star, system, body, map
// or ground moves (generation 17). A system's biotas are drawn together (its living worlds' spores reach their airless neighbours),
// computed on request and cached by system (`lifeOf`). The enums are saved keys (the guide's species, Z-04 on): appended, never
// inserted or renumbered
#include "life/lineages.h"
#include "galaxy/system.h"
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

// the stage a world's life reached (the stages of an organism's own life are `LifeCycle`'s): nothing; chemistry that is not yet life
// (protocells, no genome); small cells; large cells; colonies and the first of many cells; spores and cysts alone, of a life that ended
// or was carried; cells turned to stone (a second stage only: a world's old rock)
enum LifeStage { LS_STERILE = 0, LS_PREBIOTIC, LS_SIMPLE, LS_COMPLEX, LS_COLONIAL, LS_DORMANT, LS_FOSSIL, LS_COUNT };
extern const char* const LIFE_STAGE_NAMES[LS_COUNT];
inline bool livingStage(int s) { return s == LS_SIMPLE || s == LS_COMPLEX || s == LS_COLONIAL; }

// the microhabitats a sample is taken from (Z-03 reads them at the crosshair): in the world's liquid (water, brine, acid, ammonia water,
// methane); wet silt at its edge and in its shallows; soil; microbial mats and stromatolites; dry crusts (lichen, the deserts' crusts);
// a hot spring's rim and channel; a vent's chimney; ice (brine veins, sea ice, permafrost); snow; salt crusts and brine pools; inside
// rock (endoliths, hypoliths under quartz, fossils in old rock); frost from a plume's fall; the old walls of a dead people; cloud
// droplets (the capsule's scoop); regolith and dust
enum Niche { NI_LIQUID = 0, NI_SILT, NI_SOIL, NI_MAT, NI_CRUST, NI_SPRING, NI_VENT, NI_ICE, NI_SNOW, NI_SALT, NI_ROCK, NI_PLUME, NI_RUINS, NI_CLOUD,
             NI_DUST, NI_COUNT };
extern const char* const NICHE_NAMES[NI_COUNT];
constexpr uint32_t nicheBit(int n) { return 1u << n; }

// what a species is, by its stage: protocells; small cells' shapes (Walsby 1980's square, Stella's star, Caulobacter's stalk); large cells
// (amoebae, flagellates, ciliates, a silica box in two halves, a test of the world's grains, a lattice with spines: Haeckel 1904, chambers
// on a spiral, plates and a girdle); colonies (a rolling sphere with daughters inside, a plate, a chain, branching threads, amoebae that
// gather and rise into a stalk)
enum MicrobePlan { MP_VESICLE = 0, MP_LAMELLAR, MP_DROPLET,
                MP_SPHERE, MP_ROD, MP_CURVED, MP_SPIRAL, MP_CORKSCREW, MP_FILAMENT, MP_SQUARE, MP_STAR, MP_STALKED, MP_BUDDING, MP_CHANGING,
                MP_AMOEBA, MP_FLAGELLATE, MP_CILIATE, MP_SHELLED, MP_TEST, MP_LATTICE, MP_CHAMBERED, MP_ARMOURED,
                MP_ROLLER, MP_PLATE, MP_CHAIN, MP_BRANCHING, MP_GATHERING, MP_COUNT };
extern const char* const MICROBE_PLAN_NAMES[MP_COUNT];
inline bool protocellPlan(int p) { return p <= MP_DROPLET; }
inline bool smallCellPlan(int p) { return p >= MP_SPHERE && p <= MP_CHANGING; }
inline bool largeCellPlan(int p) { return p >= MP_AMOEBA && p <= MP_ARMOURED; }
inline bool colonyPlan(int p) { return p >= MP_ROLLER && p < MP_COUNT; }

// how it lives: by light (its pigment), by chemistry (what it burns: `Substrate`), on the dead matter of others, on others alive
enum LifeWay { LW_NONE = 0, LW_LIGHT, LW_CHEMICAL, LW_DEAD, LW_GRAZER, LW_PREDATOR, LW_COUNT };
extern const char* const WAY_NAMES[LW_COUNT];
enum Substrate { SU_NONE = 0, SU_HYDROGEN, SU_SULPHIDE, SU_IRON, SU_METHANE, SU_AMMONIA, SU_ACETYLENE, SU_COUNT };
extern const char* const SUBSTRATE_NAMES[SU_COUNT];
// its pigment: chlorophyll-like green, rhodopsin purple (DasSarma and Schwieterman 2021), carotenoid orange and red, melanin brown,
// phycobilin blue-green, the dark of a red dwarf's light (Kiang et al. 2007)
enum Pigment { PG_NONE = 0, PG_GREEN, PG_PURPLE, PG_ORANGE, PG_RED, PG_BROWN, PG_BLUE_GREEN, PG_BLACK, PG_COUNT };
extern const char* const PIGMENT_NAMES[PG_COUNT];

// the stages of its own life (bits): a spore or cyst, waking, growing, dividing, exchanging genes, a colony, gathering, fruiting,
// sporulating, dying
enum LifeCycle : uint32_t { LC_DORMANT = 1, LC_WAKING = 2, LC_GROWING = 4, LC_DIVIDING = 8, LC_EXCHANGING = 16, LC_COLONY = 32, LC_GATHERING = 64,
                            LC_FRUITING = 128, LC_SPORULATING = 256, LC_DYING = 512 };
constexpr int LC_BITS = 10;
extern const char* const CYCLE_NAMES[LC_BITS];
enum Division { DV_NONE = 0, DV_FISSION, DV_BUDDING, DV_MULTIPLE, DV_FRAGMENTS, DV_COUNT };
extern const char* const DIVISION_NAMES[DV_COUNT];
// how it moves at low Reynolds number (Purcell 1977): not at all, runs and tumbles (Berg and Brown 1972), helical swimming, gliding,
// twitching, crawling, rolling, a corkscrew
enum Motion { MO_NONE = 0, MO_RUN_TUMBLE, MO_HELICAL, MO_GLIDING, MO_TWITCHING, MO_CRAWLING, MO_ROLLING, MO_CORKSCREW, MO_COUNT };
extern const char* const MOTION_NAMES[MO_COUNT];
// what it turns toward (bits): light, food, a field (Blakemore 1975: a world without a field has none), warmth
enum Taxis : uint32_t { TX_LIGHT = 1, TX_FOOD = 2, TX_FIELD = 4, TX_WARMTH = 8 };
constexpr int TX_BITS = 4;
extern const char* const TAXIS_NAMES[TX_BITS];
// what an entry of a world's list is: a species alive, one asleep (spores and cysts: some wake), a fossil, a kind of protocell
enum MicrobeState { SS_LIVING = 0, SS_DORMANT, SS_FOSSIL, SS_PROTO, SS_COUNT };
extern const char* const STATE_NAMES[SS_COUNT];

struct Microbe {
    int index = 0;              // its place in its world's list (the guide's M<index>, Z-04)
    int state = SS_LIVING;
    int plan = MP_SPHERE;
    int niche = NI_LIQUID;      // where it lives most
    uint32_t niches = 0;        // every niche it lives in (its own among them)
    int way = LW_NONE;
    int substrate = SU_NONE;    // what a chemical way burns
    int pigment = PG_NONE;
    int oxygen = 0;             // 0 none tolerated, 1 tolerated, 2 needed
    double tMin = 0, tOpt = 0, tMax = 0;   // C: where it grows, its best
    int saltLo = 0, saltHi = 0;            // 0 fresh, 1 the sea's, 2 brine
    double phLo = 6, phHi = 8;
    double doseGy = 0;          // the dose of radiation it survives
    double sporeYears = 0;      // how long its spores or cysts last (0: it makes none)
    uint32_t cycle = 0;         // LifeCycle bits
    int division = DV_NONE;
    int motion = MO_NONE;
    uint32_t taxis = 0;         // Taxis bits
    bool glows = false;         // its crowd glows (quorum: Nealson and Hastings 1979)
    double abundance = 0;       // its share where it lives, 0..1
    int clade = -1;             // its genesis's deep branch (0, 1 the small cells', 2 the large cells'; -1 a protocell's)
    uint64_t genesis = 0;       // the genesis it descends from (0: a protocell)
    uint64_t seed = 0;          // what its form, its genome and its strains grow from (Z-02, Z-06)
    double ageYears = 0;        // a fossil's age; a carried spore's time since the impact that carried it
    int fromBody = -1, fromIndex = -1;   // a carried spore: the world of its system it came from, and its species there
};

struct Biota {
    int stage = LS_STERILE;
    int second = LS_STERILE;    // a second stage it shows (fossils in old rock; the protocells under carried spores), STERILE none
    int chemistry = CH_NONE;
    Genesis genesis;            // its life's (none: sterile or prebiotic)
    uint32_t habitats = 0;      // the microhabitats it has (Niche bits)
    double dormantYears = 0;    // DORMANT: how long its own life has slept
    double meanC = 0;           // its ground's mean temperature, as the species' limits read it
    std::vector<Microbe> species;
    int count(int state) const { int n = 0; for (const Microbe& s : species) n += s.state == state; return n; }
};

// what a kind's worlds can hold: the weights of the stages its own life reaches (FOSSIL is `fossils`: the share whose old rock keeps
// them), its chemistry, whether its worlds keep a living neighbour's spores (airless or icy), whether its life lives in a sea under its
// ice (the surface's cold does not end it), its microhabitats; a furnace holds nothing and keeps nothing
struct LifeRow {
    int kind = -1;
    bool furnace = false;
    double w[LS_FOSSIL] = {0, 0, 0, 0, 0, 0};   // STERILE ... DORMANT
    double fossils = 0;
    int chemistry = CH_NONE;
    bool keepsSpores = false;
    bool underIce = false;
    uint32_t habitats = 0;
};
extern const std::array<LifeRow, PT_COUNT> BIOTA;

// every body's biota of a system, cached by system (the last sixteen); `biotaOf` one body's
struct SystemLife { std::vector<Biota> bodies; };
std::shared_ptr<const SystemLife> lifeOf(const StarSystem& sys);
Biota biotaOf(const StarSystem& sys, int body);
// the ship's own species, of home's genesis (the garden's soil, the explorer's skin, the water tank's film): the tree's root (Z-06)
constexpr int SHIP_SPECIES = 3;
Microbe shipSpecies(int k);
// the ground's mean temperature the species' limits read (C): the living ground's climate, an air's, an airless world's, the dark's
double biotaMeanC(const StarSystem& sys, const Body& b);
