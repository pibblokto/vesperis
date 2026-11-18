// Z-01: where life is (life/biota.h): the kinds' rows, a world's stage and species, a system's spores carried to its airless neighbours
#include "life/biota.h"
#include "galaxy/planetmap.h"
#include "galaxy/drifters.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <mutex>

const char* const LIFE_STAGE_NAMES[LS_COUNT] = {"STERILE", "PREBIOTIC", "SIMPLE", "COMPLEX", "COLONIAL", "DORMANT", "FOSSIL"};
const char* const NICHE_NAMES[NI_COUNT] = {"LIQUID", "SILT", "SOIL", "MAT", "CRUST", "SPRING", "VENT", "ICE", "SNOW", "SALT", "ROCK", "PLUME", "RUINS",
                                           "CLOUD", "DUST"};
const char* const MICROBE_PLAN_NAMES[MP_COUNT] = {"VESICLE", "LAYERED VESICLE", "DROPLET", "SPHERE", "ROD", "CURVED ROD", "SPIRAL", "CORKSCREW", "FILAMENT",
                                          "SQUARE", "STAR", "STALKED", "BUDDING", "CHANGING", "AMOEBA", "FLAGELLATE", "CILIATE", "SHELLED", "TEST",
                                          "LATTICE", "CHAMBERED", "ARMOURED", "ROLLING COLONY", "PLATE COLONY", "CHAIN", "BRANCHING", "GATHERING"};
const char* const WAY_NAMES[LW_COUNT] = {"NONE", "LIGHT", "CHEMICAL", "DEAD MATTER", "GRAZER", "PREDATOR"};
const char* const SUBSTRATE_NAMES[SU_COUNT] = {"NONE", "HYDROGEN", "SULPHIDE", "IRON", "METHANE", "AMMONIA", "ACETYLENE"};
const char* const PIGMENT_NAMES[PG_COUNT] = {"NONE", "GREEN", "PURPLE", "ORANGE", "RED", "BROWN", "BLUE-GREEN", "BLACK"};
const char* const CYCLE_NAMES[LC_BITS] = {"DORMANT", "WAKING", "GROWING", "DIVIDING", "EXCHANGING", "COLONY", "GATHERING", "FRUITING", "SPORULATING",
                                          "DYING"};
const char* const DIVISION_NAMES[DV_COUNT] = {"NONE", "FISSION", "BUDDING", "MULTIPLE", "FRAGMENTS"};
const char* const MOTION_NAMES[MO_COUNT] = {"NONE", "RUN AND TUMBLE", "HELICAL", "GLIDING", "TWITCHING", "CRAWLING", "ROLLING", "CORKSCREW"};
const char* const TAXIS_NAMES[TX_BITS] = {"LIGHT", "FOOD", "FIELD", "WARMTH"};
const char* const STATE_NAMES[SS_COUNT] = {"LIVING", "DORMANT", "FOSSIL", "PROTOCELL"};

namespace {
constexpr uint32_t L = 1u << NI_LIQUID, SI = 1u << NI_SILT, SO = 1u << NI_SOIL, MT = 1u << NI_MAT, CR = 1u << NI_CRUST, SP = 1u << NI_SPRING,
                   VE = 1u << NI_VENT, IC = 1u << NI_ICE, SN = 1u << NI_SNOW, SA = 1u << NI_SALT, RO = 1u << NI_ROCK, PL = 1u << NI_PLUME,
                   RU = 1u << NI_RUINS, CL = 1u << NI_CLOUD, DU = 1u << NI_DUST;
constexpr void stages(LifeRow& r, int chem, uint32_t habitats, double sterile, double prebiotic, double simple, double complex, double colonial,
                      double dormant, double fossils) {
    r.chemistry = chem; r.habitats = habitats;
    r.w[LS_STERILE] = sterile; r.w[LS_PREBIOTIC] = prebiotic; r.w[LS_SIMPLE] = simple; r.w[LS_COMPLEX] = complex; r.w[LS_COLONIAL] = colonial;
    r.w[LS_DORMANT] = dormant; r.fossils = fossils;
}
constexpr void furnace(LifeRow& r) { r.furnace = true; r.w[LS_STERILE] = 1; r.habitats = RO | DU; }   // grains, at most organics cooked
constexpr void keeps(LifeRow& r, bool underIce = false) { r.keepsSpores = true; r.underIce = underIce; }
constexpr void airless(LifeRow& r, uint32_t habitats) { stages(r, CH_WATER, habitats, 1, 0, 0, 0, 0, 0, 0); keeps(r); }

// the kinds' rows (IDEAS 1.1): what each kind's worlds hold, from the stage table's "where" column. A world with flora colonial; an ocean or
// an eyeball world large cells or colonies; an archean small cells, one in five large; a dead world spores over its fossils; a comet
// prebiotic, a hydrocarbon world too, one in six small cells; the furnaces nothing; a giant, a brown dwarf and the companion nothing (no life
// in a giant's clouds: X-03); the airless and icy keep a living neighbour's spores, and the moons with a sea under their ice its own small life
constexpr std::array<LifeRow, PT_COUNT> buildBiota() {
    std::array<LifeRow, PT_COUNT> R{};
    for (int t = 0; t < PT_COUNT; t++) R[t].kind = t;
    for (int t : {PT_MOLTEN, PT_VOLCANIC, PT_TECTONIC, PT_MAGMA_OCEAN, PT_CHTHONIAN, PT_GLASS, PT_STEAM}) furnace(R[t]);
    for (int t : {PT_CRATERED, PT_ROCKY, PT_ICY}) airless(R[t], DU | RO | IC);
    for (int t : {PT_METAL, PT_CARBON, PT_BOMBARDED}) airless(R[t], DU | RO);
    stages(R[PT_VENUSIAN], CH_ACID, CL | RO | DU, 0.45, 0.25, 0.3, 0, 0, 0, 0);   // cloud life at the temperate height (Seager et al. 2021)
    stages(R[PT_ACIDIC], CH_ACID, CL | L | SI | RO, 0.3, 0.3, 0.4, 0, 0, 0, 0);
    for (int t : {PT_FELISIAN, PT_SUPERHABITABLE, PT_TUNDRA}) stages(R[t], CH_WATER, L | SI | SO | RO | IC | SN, 0, 0, 0, 0, 1, 0, 0.6);
    stages(R[PT_HOTHOUSE], CH_WATER, L | SI | SO | RO, 0, 0, 0, 0, 1, 0, 0.6);   // no ice at either pole
    stages(R[PT_STEPPE], CH_WATER, L | SI | SO | CR | RO | SA | IC | SN, 0, 0, 0, 0, 1, 0, 0.6);
    stages(R[PT_TERMINATOR], CH_WATER, L | SI | SO | RO | IC | SN | SA, 0, 0, 0, 0, 1, 0, 0.5);
    stages(R[PT_THINATMO], CH_WATER, DU | RO | IC | CR | SA, 0.5, 0.15, 0.25, 0, 0, 0.1, 0.3);
    stages(R[PT_QUARTZ], CH_WATER, RO | DU | CR, 0.4, 0.1, 0.5, 0, 0, 0, 0);   // hypoliths under the translucent stones (Pointing et al. 2009)
    for (int t : {PT_OCEAN, PT_EYEBALL}) stages(R[t], CH_WATER, L | IC, 0, 0, 0, 0.5, 0.5, 0, 0);
    stages(R[PT_COMET], CH_WATER, IC | DU, 0, 1, 0, 0, 0, 0, 0);
    stages(R[PT_EUROPAN], CH_WATER, IC | PL | DU, 0.2, 0.4, 0.4, 0, 0, 0, 0); keeps(R[PT_EUROPAN], true);
    stages(R[PT_DESERT], CH_WATER, DU | CR | RO | SA, 0.15, 0, 0.4, 0.3, 0, 0.15, 0.5);
    stages(R[PT_HYDROCARBON], CH_HYDROCARBON, L | SI | DU, 0, 5, 1, 0, 0, 0, 0);   // azotosomes, doubted (Stevenson et al. 2015; Sandstrom and Rahm 2020)
    stages(R[PT_SALT], CH_WATER, SA | DU | RO, 0.2, 0, 0.6, 0, 0, 0.2, 0.6);
    stages(R[PT_RUST], CH_WATER, DU | RO | CR | SA, 0.1, 0, 0.2, 0, 0, 0.7, 1);
    stages(R[PT_ARCHEAN], CH_WATER, L | SI | MT | RO | DU | IC | SN, 0, 0, 0.8, 0.2, 0, 0, 0.5);
    stages(R[PT_LICHEN], CH_WATER, CR | RO | DU | IC, 0, 0, 0, 0.8, 0.2, 0, 0.3);
    stages(R[PT_HYCEAN], CH_WATER, L, 0, 0.25, 0.6, 0.15, 0, 0, 0);
    stages(R[PT_AMMONIA], CH_AMMONIA, L | SI | IC, 0.2, 0.4, 0.4, 0, 0, 0, 0);
    stages(R[PT_SNOWBALL], CH_WATER, L | IC | SN | RO, 0.1, 0, 0.55, 0.35, 0, 0, 0.4);
    stages(R[PT_CHEMOTROPH], CH_WATER, SP | VE | MT | RO | DU, 0, 0, 0.8, 0.2, 0, 0, 0.3);
    stages(R[PT_FROZEN_FELISIAN], CH_WATER, IC | SN | SO | RO, 0, 0, 0, 0, 0, 1, 1);
    stages(R[PT_SCORCHED_FELISIAN], CH_WATER, SA | DU | RO | SO, 0, 0, 0.15, 0, 0, 0.85, 1);
    for (int t : {PT_NITROGEN, PT_FROST_DWARF}) { stages(R[t], CH_WATER, IC | DU | RO, 0.35, 0.65, 0, 0, 0, 0, 0); keeps(R[t]); }   // tholins
    stages(R[PT_CRYOVOLCANIC], CH_WATER, IC | PL | DU, 0.3, 0.7, 0, 0, 0, 0, 0); keeps(R[PT_CRYOVOLCANIC]);
    stages(R[PT_DRY_ICE], CH_WATER, IC | DU | RO, 0.6, 0.4, 0, 0, 0, 0, 0.1); keeps(R[PT_DRY_ICE]);
    stages(R[PT_WANDERER], CH_WATER, IC | SN | DU | RO, 0.4, 0.2, 0, 0, 0, 0.4, 0.4); keeps(R[PT_WANDERER]);
    stages(R[PT_BRINE_DWARF], CH_WATER, SA | IC | DU | RO, 0.4, 0.5, 0.1, 0, 0, 0, 0); keeps(R[PT_BRINE_DWARF], true);
    stages(R[PT_GROOVED_ICE], CH_WATER, IC | DU | RO, 0.5, 0.4, 0.1, 0, 0, 0, 0); keeps(R[PT_GROOVED_ICE], true);
    stages(R[PT_TWO_FACED], CH_WATER, IC | DU, 0.7, 0.3, 0, 0, 0, 0, 0); keeps(R[PT_TWO_FACED]);
    stages(R[PT_TIGER_STRIPED], CH_WATER, IC | PL | SN, 0.1, 0.45, 0.45, 0, 0, 0, 0); keeps(R[PT_TIGER_STRIPED], true);
    stages(R[PT_PATCHWORK], CH_WATER, IC | DU | RO, 0.8, 0.2, 0, 0, 0, 0, 0); keeps(R[PT_PATCHWORK]);
    stages(R[PT_SPONGE], CH_WATER, IC | DU, 0.6, 0.4, 0, 0, 0, 0, 0); keeps(R[PT_SPONGE]);
    stages(R[PT_DARK_OCEAN], CH_WATER, L | VE | SI | RO, 0, 0, 0.7, 0.3, 0, 0, 0);   // chemotrophs at the vents of a sea in the dark
    return R;
}
// a row a kind in order; a furnace sterile and keeping nothing; a row with habitats a stage of life or a neighbour's spores, and a chemistry;
// a row without none (the unit holds them against KINDS: every landable kind has habitats, every other none)
constexpr bool biotaOk(const std::array<LifeRow, PT_COUNT>& R) {
    for (int t = 0; t < PT_COUNT; t++) {
        const LifeRow& r = R[t];
        double life = 0, all = 0;
        for (int s = 0; s < LS_FOSSIL; s++) { if (r.w[s] < 0) return false; all += r.w[s]; if (s != LS_STERILE) life += r.w[s]; }
        if (r.kind != t || r.fossils < 0 || r.fossils > 1) return false;
        if (r.furnace && (life > 0 || r.keepsSpores || r.fossils > 0 || r.underIce || !(r.w[LS_STERILE] > 0))) return false;
        if (r.habitats == 0) { if (all > 0 || r.keepsSpores || r.chemistry != CH_NONE) return false; continue; }
        if (!r.furnace && ((life <= 0 && !r.keepsSpores) || r.chemistry == CH_NONE)) return false;
        if (r.underIce && !r.keepsSpores) return false;
    }
    return true;
}
constexpr std::array<LifeRow, PT_COUNT> BUILT_BIOTA = buildBiota();
static_assert(biotaOk(buildBiota()), "the biota table: a row missing, out of order, a furnace with life, or a row without a stage");

// the world's draws' salts (generation 17), each decision its own, after every draw of 16
constexpr uint64_t SALT_STAGE = 0x11FE57A6E0000001ULL;     // the stage among the row's
constexpr uint64_t SALT_COUNTS = 0x11FE57A6E0000002ULL;    // how many species, protocells and fossils, the fossils' coin, the dormant years
constexpr uint64_t SALT_ORDER = 0x11FE57A6E0000003ULL;     // the habitats' order the first species take
constexpr uint64_t SALT_SPECIES = 0x11FE57A6E0000004ULL;   // a species' draws, each its own stream
constexpr uint64_t SALT_CARRIED = 0x11FE57A6E0000005ULL;   // which of a living world's spores reach a neighbour, and where they lie
constexpr uint64_t SALT_FIX = 0x11FE57A6E0000006ULL;       // the fixes after the draws (a spore former given, a glow given)
constexpr uint64_t SALT_DARK = 0x11FE57A6E0000007ULL;      // whether a drifter's airless or icy body keeps the spores of the star it left
constexpr double DARK_SPORES = 1.0 / 3;

// the ways to live by niche: light, chemistry, dead matter, grazing, preying (the large cells' alone)
constexpr double WAY_W[NI_COUNT][5] = {
    {3, 0.8, 2, 2, 1.2}, {1.5, 2, 3, 2, 1.2}, {0.8, 0.8, 4, 1.5, 1}, {4, 1.5, 1.5, 2, 0.6}, {3, 0.2, 0.8, 0.5, 0.2},   // liquid, silt, soil, mat, crust
    {1.5, 3, 1, 0.4, 0.2}, {0, 5, 1, 0.3, 0.2}, {1.2, 1.5, 1, 1, 0.3}, {4, 0.3, 0.6, 0.5, 0.2}, {1.5, 0.6, 1.5, 0.6, 0.2},   // spring, vent, ice, snow, salt
    {2, 2, 0.5, 0.2, 0.2}, {0, 3, 1, 0.2, 0.2}, {2, 0.5, 2, 0.5, 0.2}, {2, 1, 0.5, 0.2, 0.2}, {0.2, 1.5, 0.5, 0.2, 0.2}};    // rock, plume, ruins, cloud, dust
// the niches' share of a world's species beyond the first (one each, in a hashed order), and the niches a species of each also lives in
constexpr double NICHE_W[NI_COUNT] = {3, 3, 3, 3, 2, 2, 2, 1.5, 0.8, 1.2, 1.2, 2, 0.7, 3, 1};
struct Also { int niche; double p; };
constexpr Also ALSO[NI_COUNT][3] = {
    {{NI_SILT, 0.4}, {NI_ICE, 0.15}, {-1, 0}}, {{NI_LIQUID, 0.3}, {NI_MAT, 0.3}, {NI_SOIL, 0.2}}, {{NI_CRUST, 0.3}, {NI_SILT, 0.2}, {NI_RUINS, 0.3}},
    {{NI_SILT, 0.4}, {NI_SPRING, 0.3}, {-1, 0}}, {{NI_SOIL, 0.3}, {NI_ROCK, 0.4}, {NI_RUINS, 0.3}}, {{NI_MAT, 0.4}, {NI_VENT, 0.3}, {-1, 0}},
    {{NI_SPRING, 0.4}, {-1, 0}, {-1, 0}}, {{NI_SNOW, 0.4}, {NI_LIQUID, 0.2}, {-1, 0}}, {{NI_ICE, 0.4}, {-1, 0}, {-1, 0}},
    {{NI_SILT, 0.3}, {NI_LIQUID, 0.2}, {-1, 0}}, {{NI_CRUST, 0.3}, {NI_DUST, 0.3}, {NI_RUINS, 0.2}}, {{NI_ICE, 0.6}, {-1, 0}, {-1, 0}},
    {{NI_ROCK, 0.4}, {NI_SOIL, 0.3}, {-1, 0}}, {{-1, 0}, {-1, 0}, {-1, 0}}, {{NI_ROCK, 0.4}, {NI_ICE, 0.2}, {-1, 0}}};
// the small cells' plans by way (light, chemistry, dead matter): sphere, rod, curved rod, spiral, corkscrew, filament, square, star, stalked,
// budding, changing
constexpr double SMALL_W[3][11] = {{3, 1, 0.3, 0.4, 0, 2.5, 0.2, 0.1, 0.2, 0.2, 0.3}, {2, 3, 1, 0.6, 0.3, 1.5, 0.1, 0.2, 0.7, 0.4, 0.4},
                                   {2, 3, 1, 0.5, 0.7, 1, 0.1, 0.2, 0.4, 0.8, 0.5}};
// the large cells' by way (light, chemistry, dead matter, grazing, preying): amoeba, flagellate, ciliate, shelled, test, lattice, chambered,
// armoured (a diatom's box and a dinoflagellate's plates feed by light; Didinium preys)
constexpr double LARGE_W[5][8] = {{0, 2, 0, 2.5, 0, 0, 0, 1.5}, {0, 0, 0, 0, 0, 0, 0, 0}, {1, 1, 0.2, 0, 0.5, 0, 0, 0}, {2, 2, 2.5, 0, 0.8, 0, 0.8, 0.3},
                                  {1.5, 0.5, 2, 0, 0, 1, 0.5, 0.8}};
// the colonies' by way: roller, plate, chain, branching, gathering
constexpr double COLONY_W[5][5] = {{2, 1, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0.5, 2, 0}, {0, 0, 0, 0, 2}, {0, 0, 0, 0, 0.5}};

int pickW(Rng& r, const double* w, int n) {   // a weighted pick's index (-1 when every weight is 0), one draw whatever the weights
    double total = 0;
    for (int i = 0; i < n; i++) total += w[i];
    double x = r.uni() * total;
    if (!(total > 0)) return -1;
    for (int i = 0; i < n; i++) { x -= w[i]; if (x < 0 && w[i] > 0) return i; }
    for (int i = n - 1; i >= 0; i--) if (w[i] > 0) return i;
    return -1;
}
double logUni(Rng& r, double lo, double hi) { return std::pow(10.0, lo + (hi - lo) * r.uni()); }

// what a world is, as the draws read it
struct World {
    const StarSystem* sys = nullptr;
    const Body* b = nullptr;
    const LifeRow* row = nullptr;
    BodyGen g;
    double meanC = 0;
    bool dark = false, oxygen = false, field = false, sea = false;
    int light = STAR_YELLOW;   // the class of the sun it lives by
    int pastStage = LS_SIMPLE;  // the stage of the life a dormant world's spores and fossils are of
};

// the stage the life of a dead or dormant world had reached: its living kind's highest, else its own row's highest living stage
int pastStageOf(int type) {
    const LifeRow& r = BUILT_BIOTA[livingKindOf(type)];
    for (int s = LS_COLONIAL; s >= LS_SIMPLE; s--) if (r.w[s] > 0) return s;
    return LS_SIMPLE;
}

// whether a chemistry's life is active at a world's mean (its liquid's range); a cloud life always (its temperate height)
bool activeAt(int chem, double meanC) {
    switch (chem) {
        case CH_WATER: return meanC > -60 && meanC < 110;
        case CH_AMMONIA: return meanC > -110 && meanC < -10;
        case CH_HYDROCARBON: return meanC > -205 && meanC < -150;
        case CH_ACID: return true;
        default: return false;
    }
}

int pigmentByLight(Rng& r, int light, int niche) {
    // green, purple, orange, red, brown, blue-green, black
    double w[7] = {4, 1, 0.6, 0.3, 0.3, 2, 0};
    if (light == STAR_RED_DWARF) { const double d[7] = {0.6, 2, 0, 1, 1, 0, 3}; std::copy(d, d + 7, w); }   // darker toward black (Kiang 2007)
    else if (light == STAR_BLUE_GIANT || light == STAR_BLUE_WHITE || light == STAR_WHITE_DWARF || light == STAR_NEUTRON || light == STAR_PULSAR ||
             light == STAR_WOLF_RAYET || light == STAR_BLACK_HOLE) { const double d[7] = {1, 0, 3, 2, 0.5, 0, 0}; std::copy(d, d + 7, w); }
    else if (light == STAR_RED_GIANT || light == STAR_ORANGE_GIANT || light == STAR_CARBON) { const double d[7] = {0.5, 2, 0, 2, 1.5, 0, 1}; std::copy(d, d + 7, w); }
    if (niche == NI_SALT) { w[1] += 4; w[3] += 3; }   // the halophiles' rhodopsin and bacterioruberin (the pink lakes)
    if (niche == NI_SNOW) w[3] += 4;                  // the snow algae's red (Hoham and Remias 2020)
    if (niche == NI_SPRING || niche == NI_MAT) w[2] += 3;   // the mats' carotenoid orange
    return 1 + pickW(r, w, 7);
}

int pigmentElse(Rng& r, int niche, bool dry) {   // a pigment that does not feed it: carotenoids in the hot and the salt, melanin in the dry and the bare
    double w[8] = {6, 0, 0, 0.5, 0.3, 0.6, 0, 0.1};   // none, green, purple, orange, red, brown, blue-green, black
    if (niche == NI_SPRING || niche == NI_VENT) w[3] += 1.5;
    if (niche == NI_SALT) w[4] += 2.5;
    if (dry) { w[5] += 1.5; w[7] += 0.4; }
    return pickW(r, w, 8);
}

// one species of a world: its niche (the first ones each a habitat of the world's, in its hashed order), its way, its plan, and the rest
Microbe drawSpecies(const World& W, int k, int state, int stage, const int* order, int nh, uint32_t habitats, uint64_t genesis) {
    Rng r(hashCombine(W.b->seed ^ SALT_SPECIES, (uint64_t)k));
    Microbe s;
    s.index = k; s.state = state; s.genesis = state == SS_PROTO ? 0 : genesis;
    const int chem = W.row->chemistry;
    // the niche
    {
        double w[NI_COUNT];
        for (int n = 0; n < NI_COUNT; n++) w[n] = (habitats & nicheBit(n)) ? NICHE_W[n] : 0;
        if (state == SS_FOSSIL) { for (int n = 0; n < NI_COUNT; n++) w[n] = n == NI_ROCK ? 1 : 0; }
        if (state == SS_PROTO) for (int n : {NI_SOIL, NI_MAT, NI_CRUST, NI_RUINS, NI_SNOW, NI_VENT}) w[n] = 0;
        const int picked = pickW(r, w, NI_COUNT);
        s.niche = state == SS_LIVING && k < nh ? order[k] : (picked >= 0 ? picked : NI_ROCK);
        s.niches = nicheBit(s.niche);
        for (const Also& a : ALSO[s.niche]) if (a.niche >= 0 && r.uni() < a.p && (habitats & nicheBit(a.niche))) s.niches |= nicheBit(a.niche);
    }
    const int niche = s.niche;
    const bool dry = niche == NI_ROCK || niche == NI_DUST || niche == NI_CRUST || niche == NI_SALT;
    // the way to live, and the plan that lives it
    {
        double w[5];
        for (int i = 0; i < 5; i++) w[i] = WAY_W[niche][i];
        const bool large = stage == LS_COMPLEX || stage == LS_COLONIAL;
        if (!large) w[3] = w[4] = 0;
        if (W.dark || W.row->underIce) w[0] = 0;                       // no light to live by: the dark, the sea under the ice
        if (chem == CH_HYDROCARBON) { w[0] *= 0.1; w[1] += 3; }         // a dim surface under the haze: acetylene and hydrogen (McKay and Smith 2005)
        if (chem == CH_AMMONIA) w[0] *= 0.5;
        const int way = pickW(r, w, 5);
        s.way = (state == SS_PROTO || state == SS_FOSSIL) ? LW_NONE : (way >= 0 ? 1 + way : LW_CHEMICAL);
        const double fam = r.uni();
        if (state == SS_PROTO) {
            const double p[3] = {2, 1, 1.5};
            s.plan = MP_VESICLE + pickW(r, p, 3);
        } else {
            // the family: the small cells (always in a simple world, and the chemical way's), the large cells (the grazers' and the predators'),
            // the colonies (a colonial world's)
            const int wi = s.way == LW_NONE ? 2 : s.way - 1;
            int family = 0;
            if (large && (s.way == LW_GRAZER || s.way == LW_PREDATOR)) family = (stage == LS_COLONIAL && fam < 0.25) ? 2 : 1;
            else if (large && s.way != LW_CHEMICAL) family = fam < 0.5 ? 0 : (stage == LS_COLONIAL && fam > 0.8 ? 2 : 1);
            if (state == SS_FOSSIL) family = fam < 0.6 || stage == LS_SIMPLE ? 0 : 1;
            if (family == 1) {
                double w8[8];
                for (int i = 0; i < 8; i++) w8[i] = LARGE_W[wi][i];
                const bool sea = W.sea && (niche == NI_LIQUID || niche == NI_SILT);
                if (!sea) { w8[5] = 0; w8[6] = 0; }   // a lattice and chambers in the sea alone (radiolarians, forams)
                if (niche != NI_LIQUID) w8[7] = 0;    // the plates and the girdle in the open water
                if (!(niche == NI_SOIL || niche == NI_SILT || niche == NI_CRUST)) w8[4] = 0;   // a test in soil and silt
                if (state == SS_FOSSIL) { const double f[8] = {0, 0, 0, 2, 0.5, W.sea ? 1.0 : 0.0, W.sea ? 1.5 : 0.0, 0}; std::copy(f, f + 8, w8); }
                const int p = pickW(r, w8, 8);
                if (p >= 0) s.plan = MP_AMOEBA + p; else family = 0;
            } else if (family == 2) {
                double w5[5];
                for (int i = 0; i < 5; i++) w5[i] = COLONY_W[wi][i];
                if (!(niche == NI_LIQUID || niche == NI_SILT)) { w5[0] = 0; w5[1] = 0; }   // the rolling spheres and plates swim
                if (!(niche == NI_SOIL || niche == NI_SILT || niche == NI_RUINS || niche == NI_CRUST)) w5[4] = 0;   // the gathering on the ground
                const int p = pickW(r, w5, 5);
                if (p >= 0) s.plan = MP_ROLLER + p; else family = 0;
            }
            if (family == 0) {
                double w11[11];
                for (int i = 0; i < 11; i++) w11[i] = SMALL_W[std::min(wi, 2)][i];
                if (niche == NI_SALT) w11[6] *= 15;   // the square cells of the brine (Walsby 1980)
                if (niche == NI_SILT || niche == NI_LIQUID) w11[4] *= 2;
                if (state == SS_FOSSIL) { const double f[11] = {2, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0}; std::copy(f, f + 11, w11); }
                s.plan = MP_SPHERE + std::max(0, pickW(r, w11, 11));
            }
        }
    }
    // what it burns, its pigment, the oxygen it bears
    {
        double w[SU_COUNT] = {0, 1, 1, 1, 0, 0, 0};   // none, hydrogen, sulphide, iron, methane, ammonia, acetylene
        if (niche == NI_VENT) { w[1] = 2; w[2] = 3; w[3] = 1; }
        else if (niche == NI_SPRING) { w[1] = 1; w[2] = 3; w[3] = 1; }
        else if (niche == NI_SILT) { w[1] = 1; w[2] = 1; w[3] = 0.5; w[4] = 1.5; w[5] = 1; }   // the methanotrophs, the nitrifiers
        else if (niche == NI_ROCK) { w[1] = 2; w[2] = 0.3; w[3] = 1.5; }   // the deep rock's hydrogen (Stevens and McKinley 1995)
        else if (niche == NI_PLUME) { w[1] = 3; w[2] = 0.3; w[3] = 0.3; w[4] = 1; }   // the plumes' hydrogen (Waite et al. 2017)
        else if (niche == NI_LIQUID) { w[1] = 0.5; w[2] = 0.5; w[3] = 0.5; w[5] = 1; }
        if (W.b->type == PT_RUST) w[3] += 4;
        if (chem == CH_ACID) { w[1] = 0.2; w[2] = 2; w[3] = 2; w[4] = w[5] = 0; }
        if (chem == CH_AMMONIA) { w[2] = w[3] = 0.2; w[4] = 1; w[5] = 0; }
        if (chem == CH_HYDROCARBON) { w[2] = w[3] = w[4] = w[5] = 0; w[6] = 3; }
        const int su = pickW(r, w, SU_COUNT);
        if (s.way == LW_CHEMICAL) s.substrate = std::max(su, (int)SU_HYDROGEN);
        const int pl = pigmentByLight(r, W.light, niche), pe = pigmentElse(r, niche, dry);
        s.pigment = s.way == LW_LIGHT ? pl : (state == SS_PROTO ? PG_NONE : pe);
        const double ox = r.uni();
        if (!W.oxygen) s.oxygen = ox < 0.8 ? 0 : 1;
        else if (niche == NI_SILT || niche == NI_MAT || niche == NI_VENT || niche == NI_SPRING) s.oxygen = ox < 0.4 ? 0 : (ox < 0.7 ? 1 : 2);
        else s.oxygen = ox < 0.05 ? 0 : (ox < 0.3 ? 1 : 2);
        if (s.way == LW_LIGHT && W.oxygen) s.oxygen = std::max(s.oxygen, 1);
    }
    // the limits: where it grows round the niche's warmth (a spring's band, a vent, the ice; the world's mean elsewhere), the salt, the acid,
    // the dose it survives
    {
        const double u1 = r.uni(), u2 = r.uni(), u3 = r.uni(), u4 = r.uni();
        double base = std::max(-12.0, std::min(45.0, W.meanC)), lo = -25, hi = 122;
        if (niche == NI_SPRING) base = 45 + 50 * std::pow(u1, 1.3);   // the hotter bands the rarer (Brock 1978)
        else if (niche == NI_VENT) base = 65 + 45 * u1;               // to 122 C (Takai et al. 2008)
        else if (niche == NI_ICE) base = -2 + 12 * u1;                // the psychrophiles (Morita 1975; Mykytczuk et al. 2013)
        else if (niche == NI_SNOW) base = 8 * u1;
        else if (niche == NI_PLUME) base = 2 + 30 * u1;
        else base += 8 * (u1 - 0.5);
        if (chem == CH_ACID) { base = niche == NI_CLOUD ? 15 + 45 * u1 : std::max(0.0, std::min(60.0, W.meanC)); lo = -20; hi = 90; }
        if (chem == CH_AMMONIA) { base = std::max(-78.0, std::min(-35.0, W.meanC)) + 6 * (u1 - 0.5); lo = -100; hi = -30; }
        if (chem == CH_HYDROCARBON) { base = -180 + 6 * u1; lo = -185; hi = -170; }
        const double spanLo = chem == CH_HYDROCARBON ? 4 + 4 * u2 : (state == SS_PROTO ? 30 : 10 + 20 * u2);
        const double spanHi = chem == CH_HYDROCARBON ? 2 + 3 * u3 : (state == SS_PROTO ? 30 : 4 + 10 * u3);
        s.tOpt = std::max(lo, std::min(hi, base));
        s.tMin = std::max(lo, s.tOpt - spanLo);
        s.tMax = std::min(hi, s.tOpt + spanHi);
        // the salt: brine's halophiles, the sea's, the fresh
        const bool seaWater = W.sea && u4 < 0.6;
        if (niche == NI_SALT) { s.saltLo = u2 < 0.6 ? 2 : 1; s.saltHi = 2; }
        else if (niche == NI_LIQUID || niche == NI_VENT) { s.saltLo = seaWater || niche == NI_VENT ? 1 : 0; s.saltHi = s.saltLo + (u3 < 0.2 ? 1 : 0); }
        else if (niche == NI_SILT) { s.saltLo = seaWater ? 1 : 0; s.saltHi = s.saltLo + (u3 < 0.4 ? 1 : 0); }
        else if (niche == NI_ICE) { s.saltLo = u3 < 0.6 ? 1 : 0; s.saltHi = 2; }   // the brine veins between the crystals
        else { s.saltLo = 0; s.saltHi = u3 < 0.3 ? 1 : 0; }
        s.saltHi = std::min(2, std::max(s.saltLo, s.saltHi));
        const double p1 = r.uni(), p2 = r.uni();
        if (chem == CH_ACID) { s.phLo = -1.5 + p1; s.phHi = s.phLo + 1.5; }   // concentrated acid: what a pH barely says
        else if (chem == CH_AMMONIA) { s.phLo = 9 + 2 * p1; s.phHi = s.phLo + 2; }
        else if (W.b->type == PT_RUST || (niche == NI_SPRING && p2 < 0.4)) { s.phLo = 1.5 + p1; s.phHi = s.phLo + 2; }   // the acid springs, the rust's iron
        else if (niche == NI_SALT && p2 < 0.5) { s.phLo = 8 + 1.5 * p1; s.phHi = s.phLo + 2; }   // the soda lakes (Natron's)
        else { s.phLo = 5 + 2 * p1; s.phHi = s.phLo + 2 + 2 * p2; }
        const double d = r.uni();
        double logGy = dry ? 2 + 2.2 * d : 0.5 + 2 * d;   // the dry's resistance (Deinococcus: Mattimore and Battista 1996)
        s.doseGy = std::pow(10.0, logGy);
    }
    // its life cycle, how it divides, how it moves, what it turns toward, whether it glows
    {
        const double u1 = r.uni(), u2 = r.uni(), u3 = r.uni(), u4 = r.uni(), u5 = r.uni(), u6 = r.uni(), u7 = r.uni(), u8 = r.uni(), u9 = r.uni();
        const int p = s.plan;
        s.cycle = LC_GROWING | LC_DIVIDING | LC_DYING;
        bool spores = false;
        if (smallCellPlan(p)) spores = u1 < (dry || niche == NI_SOIL || niche == NI_ICE || niche == NI_SNOW ? 0.5 : 0.2) * (p == MP_ROD || p == MP_FILAMENT ? 1.3 : (p == MP_SPHERE ? 0.5 : 1));
        else if (largeCellPlan(p)) spores = u1 < (p == MP_SHELLED ? 0.2 : (p == MP_LATTICE || p == MP_CHAMBERED ? 0.05 : 0.4));   // cysts
        else if (colonyPlan(p)) spores = p == MP_GATHERING || u1 < (p == MP_BRANCHING ? 0.5 : (p == MP_ROLLER ? 0.3 : 0.1));
        if (state == SS_DORMANT) spores = true;
        if (spores) s.cycle |= LC_DORMANT | LC_WAKING | LC_SPORULATING;
        if (u2 < (p == MP_CILIATE ? 0.7 : (smallCellPlan(p) ? 0.2 : 0.15))) s.cycle |= LC_EXCHANGING;
        if (colonyPlan(p) || (p == MP_STALKED && u3 < 0.3) || (p == MP_FILAMENT && u3 < 0.4)) s.cycle |= LC_COLONY;
        if (p == MP_GATHERING) s.cycle |= LC_GATHERING | LC_FRUITING;
        if (p == MP_BRANCHING && u4 < 0.4) s.cycle |= LC_FRUITING;
        // division
        if (protocellPlan(p)) s.division = u5 < 0.6 ? DV_FRAGMENTS : DV_FISSION;   // a thread split (Zhu and Szostak 2009)
        else if (p == MP_BUDDING) s.division = DV_BUDDING;
        else if (p == MP_SPHERE) s.division = u5 < 0.85 ? DV_FISSION : (u5 < 0.95 ? DV_BUDDING : DV_MULTIPLE);
        else if (p == MP_FILAMENT) s.division = u5 < 0.6 ? DV_FRAGMENTS : DV_FISSION;
        else if (largeCellPlan(p)) s.division = u5 < 0.85 ? DV_FISSION : DV_MULTIPLE;
        else if (p == MP_ROLLER || p == MP_PLATE) s.division = DV_MULTIPLE;   // the daughters inside (Kirk 1998)
        else if (p == MP_CHAIN || p == MP_BRANCHING) s.division = DV_FRAGMENTS;
        else s.division = DV_FISSION;
        // motion
        struct M { int m; double w; };
        M ms[4] = {{MO_NONE, 1}, {MO_NONE, 0}, {MO_NONE, 0}, {MO_NONE, 0}};
        switch (p) {
            case MP_SPHERE: ms[0] = {MO_NONE, 0.7}; ms[1] = {MO_TWITCHING, 0.2}; ms[2] = {MO_RUN_TUMBLE, 0.1}; break;
            case MP_ROD: ms[0] = {MO_RUN_TUMBLE, 0.55}; ms[1] = {MO_NONE, 0.25}; ms[2] = {MO_GLIDING, 0.15}; ms[3] = {MO_TWITCHING, 0.05}; break;
            case MP_CURVED: ms[0] = {MO_RUN_TUMBLE, 0.6}; ms[1] = {MO_HELICAL, 0.4}; break;
            case MP_SPIRAL: ms[0] = {MO_HELICAL, 0.9}; ms[1] = {MO_NONE, 0.1}; break;
            case MP_CORKSCREW: ms[0] = {MO_CORKSCREW, 1}; break;
            case MP_FILAMENT: case MP_CHAIN: ms[0] = {MO_GLIDING, 0.5}; ms[1] = {MO_NONE, 0.5}; break;
            case MP_STALKED: ms[0] = {MO_NONE, 0.7}; ms[1] = {MO_RUN_TUMBLE, 0.3}; break;   // held by its stalk; its swarmer swims
            case MP_BUDDING: ms[0] = {MO_NONE, 0.6}; ms[1] = {MO_RUN_TUMBLE, 0.4}; break;
            case MP_CHANGING: ms[0] = {MO_TWITCHING, 0.4}; ms[1] = {MO_GLIDING, 0.3}; ms[2] = {MO_NONE, 0.3}; break;
            case MP_AMOEBA: case MP_TEST: case MP_CHAMBERED: case MP_GATHERING: ms[0] = {MO_CRAWLING, 1}; break;
            case MP_FLAGELLATE: case MP_CILIATE: case MP_ARMOURED: ms[0] = {MO_HELICAL, 1}; break;
            case MP_SHELLED: ms[0] = {MO_GLIDING, 0.6}; ms[1] = {MO_NONE, 0.4}; break;   // the raphid diatoms glide
            case MP_ROLLER: ms[0] = {MO_ROLLING, 1}; break;
            case MP_PLATE: ms[0] = {MO_HELICAL, 0.7}; ms[1] = {MO_ROLLING, 0.3}; break;
            default: break;   // the protocells, the square, the star, the lattice, the branching threads: still
        }
        double mw[4] = {ms[0].w, ms[1].w, ms[2].w, ms[3].w};
        Rng mr(r.next());
        s.motion = state == SS_FOSSIL ? MO_NONE : ms[std::max(0, pickW(mr, mw, 4))].m;
        if (s.motion != MO_NONE || p == MP_GATHERING) {
            if (s.way == LW_LIGHT ? u6 < 0.8 : u6 < 0.85) s.taxis |= s.way == LW_LIGHT ? TX_LIGHT : TX_FOOD;
            if (p == MP_GATHERING) s.taxis |= TX_FOOD;   // the amoebae's signal (Tomchik and Devreotes 1981)
            if (W.field && (niche == NI_SILT || niche == NI_LIQUID) && (smallCellPlan(p) || p == MP_FLAGELLATE) && u7 < (niche == NI_SILT ? 0.24 : 0.12))
                s.taxis |= TX_FIELD;   // the magnetotactic (Blakemore 1975)
            if ((niche == NI_SPRING || niche == NI_VENT) && u8 < 0.4) s.taxis |= TX_WARMTH;
        }
        s.glows = W.sea && niche == NI_LIQUID && smallCellPlan(p) && (s.way == LW_DEAD || s.way == LW_CHEMICAL) && u9 < 0.06;   // Vibrio's
        if (state == SS_FOSSIL || state == SS_PROTO) { s.cycle = state == SS_PROTO ? (LC_GROWING | LC_DIVIDING | LC_DYING) : 0; s.taxis = 0; s.glows = false; }
    }
    // its share, its clade, how long its spores last, its seed
    {
        const double a = r.uni(), c = r.uni(), sp = r.uni();
        s.abundance = 0.02 + 0.98 * a * a * a;   // a few that crowd, many rare
        const bool extreme = niche == NI_SALT || niche == NI_SPRING || niche == NI_VENT || s.substrate == SU_METHANE;
        s.clade = protocellPlan(s.plan) ? -1 : (smallCellPlan(s.plan) ? (c < (extreme ? 0.6 : 0.25) ? 1 : 0) : 2);
        if (s.cycle & LC_DORMANT) s.sporeYears = smallCellPlan(s.plan) ? std::pow(10.0, 2 + 5 * sp) : std::pow(10.0, 1 + 3.5 * sp);
        s.seed = r.next();
        if (state == SS_FOSSIL) s.ageYears = logUni(r, 6.5, 9.5);
    }
    return s;
}

// the world a body belongs to: a moon's planet (a companion's worlds are their own)
int worldOf(const StarSystem& sys, int i) {
    for (int n = 0; n < 4; n++) { const int p = sys.bodies[i].parent; if (p < 0 || p == sys.companion) break; i = p; }
    return i;
}
// a world in its sun's order: a planet of the primary or of the companion, neither a star, a comet nor a wanderer on its wild orbit
bool ordered(const StarSystem& sys, int i) {
    const Body& b = sys.bodies[i];
    return b.type != PT_COMPANION && b.type != PT_COMET && !b.wanderer && (b.parent < 0 || b.parent == sys.companion);
}
// two bodies are neighbours (impacts carry rock between them: Gladman et al. 1996) when they are of one family (a world and its moons) or
// their worlds circle one sun with no world of its order between them
bool neighbours(const StarSystem& sys, int a, int b) {
    const int wa = worldOf(sys, a), wb = worldOf(sys, b);
    if (wa == wb) return true;
    if (!ordered(sys, wa) || !ordered(sys, wb) || sys.ofCompanion(wa) != sys.ofCompanion(wb)) return false;
    const double lo = std::min(sys.bodies[wa].orbitRadiusKm, sys.bodies[wb].orbitRadiusKm), hi = std::max(sys.bodies[wa].orbitRadiusKm, sys.bodies[wb].orbitRadiusKm);
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        if (i == wa || i == wb || !ordered(sys, i) || sys.ofCompanion(i) != sys.ofCompanion(wa)) continue;
        if (sys.bodies[i].orbitRadiusKm > lo && sys.bodies[i].orbitRadiusKm < hi) return false;
    }
    return true;
}

Biota drawWorld(const StarSystem& sys, int bi, const DrifterPast* past) {
    Biota B;
    const Body& b = sys.bodies[bi];
    if (b.type < 0 || b.type >= PT_COUNT || !KINDS[b.type].landable) return B;
    World W;
    W.sys = &sys; W.b = &b; W.row = &BUILT_BIOTA[b.type];
    const LifeRow& row = *W.row;
    W.g = BodyGen::make(b);
    W.meanC = biotaMeanC(sys, b);
    W.dark = sys.sunless();
    W.oxygen = bodyHasAir(b) && (KINDS[b.type].shapes & KS_OXYGEN_AIR);
    W.field = magneticClass(magneticField(b)) >= 1;
    W.sea = kindHas(b.type, KT_SEA);
    W.light = W.dark ? STAR_ROGUE : (sys.ofCompanion(bi) ? sys.companionStar().cls : sys.star.cls);
    W.pastStage = pastStageOf(b.type);
    B.meanC = W.meanC;
    B.chemistry = row.chemistry;
    // the habitats: the row's, the traits' (the springs, the salt, the ruins), the facts' (a sea frozen in the dark, a glassed ground)
    uint32_t h = row.habitats;
    if (!row.furnace) {
        if (W.g.hasTrait(TR_HOT_SPRINGS) || W.g.hasTrait(TR_GEYSERS)) h |= SP;
        if (W.g.hasTrait(TR_PINK_LAKES) || W.g.hasTrait(TR_SALT_FLATS) || W.g.hasTrait(TR_GYPSUM)) h |= SA;
        if (W.g.hasTrait(TR_CIVILISATION)) h |= RU;
        if (W.g.hasTrait(TR_SNOW_ALGAE)) h |= SN;   // Z-03: red snow
        if (W.g.frozenSeas && (h & L)) h = (h & ~(L | SI)) | IC;
        if (b.glassed) h &= ~(SO | MT | CR);
    }
    B.habitats = h;
    if (row.furnace) return B;
    // the stage: the row's weights, as the world's facts allow them (in the dark no active life but the dark ocean's and the seas under
    // ice; the active stages within their liquid's range; a glassed ground none but where its life is seen; a people's world at least
    // large cells)
    double w[LS_FOSSIL];
    for (int s = 0; s < LS_FOSSIL; s++) w[s] = row.w[s];
    const double active = w[LS_SIMPLE] + w[LS_COMPLEX] + w[LS_COLONIAL];
    // a kind whose life is seen on its ground (its flora, its animals, its mats and vents) lives whatever its mean; a sea under ice and the dark ocean
    const bool lives = livingGround(b.type) || kindHas(b.type, KT_FLORA | KT_FAUNA | KT_MICROBIAL | KT_VENT_LIFE) || row.underIce || b.type == PT_DARK_OCEAN;
    if (W.dark && !row.underIce && b.type != PT_DARK_OCEAN) { w[LS_DORMANT] += active; w[LS_SIMPLE] = w[LS_COMPLEX] = w[LS_COLONIAL] = 0; }
    else if (!lives && !activeAt(row.chemistry, W.meanC)) { w[LS_STERILE] += active; w[LS_SIMPLE] = w[LS_COMPLEX] = w[LS_COLONIAL] = 0; }
    if (b.glassed && !lives) { for (int s = 1; s < LS_FOSSIL; s++) { w[LS_STERILE] += w[s]; w[s] = 0; } }   // a chemotroph's vents outlived the glassing
    Rng sr(b.seed ^ SALT_STAGE);
    int stage = pickW(sr, w, LS_FOSSIL);
    if (stage < 0) stage = LS_STERILE;
    if (W.g.hasTrait(TR_CIVILISATION) && (stage == LS_STERILE || stage == LS_PREBIOTIC || stage == LS_SIMPLE)) stage = LS_COMPLEX;   // a people rose here
    if (W.g.hasTrait(TR_SNOW_ALGAE) && (stage == LS_STERILE || stage == LS_PREBIOTIC)) stage = LS_SIMPLE;   // Z-03: its red snow lives
    // a drifter's airless and icy bodies keep the spores of the life of the star it left, one in three (they were a living world's neighbours there)
    const bool darkSpores = W.dark && b.drifter && row.keepsSpores && past && past->found && stage == LS_STERILE && unitFromHash(mix64(b.seed ^ SALT_DARK)) < DARK_SPORES;
    if (darkSpores) stage = LS_DORMANT;
    B.stage = stage;
    Rng cr(b.seed ^ SALT_COUNTS);
    const double fossilCoin = cr.uni(), nu = cr.uni(), fu = cr.uni(), du = cr.uni();
    const bool fossils = (stage == LS_DORMANT && row.fossils >= 1) || ((livingStage(stage) || stage == LS_DORMANT) && fossilCoin < row.fossils);
    if (fossils) B.second = LS_FOSSIL;
    // the genesis: a wanderer's own, a drifter's its old star's, every other its system's
    if (stage != LS_STERILE && stage != LS_PREBIOTIC) {
        if (b.wanderer) B.genesis = wandererGenesis(b.seed, row.chemistry);
        else if (W.dark && past && past->found) B.genesis = systemGenesis(past->star, row.chemistry);
        else B.genesis = systemGenesis(sys.star, row.chemistry);
    }
    // how long a dormant world's life has slept: a drifter's since it left its star, a wanderer's since it was caught, a scorched world's since its
    // sun swelled, a rust world's since its water went
    if (stage == LS_DORMANT) {
        if (W.dark && b.drifter) B.dormantYears = drifterLeftYears(sys.star);
        else if (b.type == PT_RUST) B.dormantYears = std::pow(10.0, 5 + 3 * du);
        else if (b.type == PT_SCORCHED_FELISIAN) B.dormantYears = std::pow(10.0, 4 + 2.5 * du);
        else B.dormantYears = std::pow(10.0, 3 + 4 * du);
    }
    // the species: protocells, the living, a dormant world's spores, then the fossils
    int n = 0, state = SS_LIVING;
    switch (stage) {
        case LS_PREBIOTIC: n = 2 + std::min(2, (int)(nu * 3)); state = SS_PROTO; break;
        case LS_SIMPLE: n = 3 + std::min(5, (int)(nu * 6)); break;
        case LS_COMPLEX: n = 5 + std::min(7, (int)(nu * 8)); break;
        case LS_COLONIAL: n = 8 + std::min(8, (int)(nu * 9)); break;
        case LS_DORMANT: n = darkSpores ? 1 + (nu < 0.5 ? 0 : 1) : 1 + std::min(3, (int)(nu * 4)); state = SS_DORMANT; break;
        default: break;
    }
    // where its life lives: the niches with a warmth of their own (a spring, a vent, the ice and the snow, a plume, the clouds' temperate height)
    // and the ground where the world's mean lets its liquid be (none on a venusian's burning ground); a sea's under ice its ice, plumes and brine
    uint32_t livable = 0, protoIn = 0;
    const bool hotGround = W.meanC > 150;
    for (int k = 0; k < NI_COUNT; k++) {
        if (!(h & nicheBit(k))) continue;
        const bool warmth = k == NI_SPRING || k == NI_VENT || k == NI_ICE || k == NI_SNOW || k == NI_PLUME || k == NI_CLOUD;
        if (warmth || !hotGround) protoIn |= nicheBit(k);
        if (row.underIce ? (k == NI_ICE || k == NI_PLUME || k == NI_SALT) : (warmth || (!hotGround && (lives || activeAt(row.chemistry, W.meanC))))) livable |= nicheBit(k);
    }
    if (!livable) livable = h;
    if (!protoIn) protoIn = h;
    int order[NI_COUNT], nh = 0;
    {
        double key[NI_COUNT];
        Rng orr(b.seed ^ SALT_ORDER);
        for (int k = 0; k < NI_COUNT; k++) {
            const double v = orr.uni();
            if (!(livable & nicheBit(k)) || k == NI_RUINS) continue;   // the ruins' life is the ground's (a species reaches them from the soil and the rock)
            int i = nh++;
            while (i > 0 && key[i - 1] > v) { order[i] = order[i - 1]; key[i] = key[i - 1]; i--; }
            order[i] = k; key[i] = v;
        }
    }
    const int drawAs = stage == LS_DORMANT ? W.pastStage : stage;
    uint32_t own = state == SS_PROTO ? protoIn : livable;
    if (state == SS_DORMANT) own = h & (IC | SN | SO | SA | DU | RO | CR);   // where spores keep: the cold, the dry, the salt
    if (own == 0) own = h;
    for (int k = 0; k < n; k++) B.species.push_back(drawSpecies(W, k, state, drawAs, order, state == SS_LIVING ? nh : 0, own, B.genesis.id));
    if (fossils) {
        const int nf = 2 + std::min(2, (int)(fu * 3));
        for (int k = 0; k < nf; k++) B.species.push_back(drawSpecies(W, n + k, SS_FOSSIL, W.pastStage, order, 0, RO, B.genesis.id));
    }
    // the fixes, by hashes (no draw moves): a living world has a spore former (its neighbours' spores are of one); the glowing seas, the vent
    // world's spores glowing at night, the luminous forests' foxfire, the lights in a dark sea have their glowing species
    if (livingStage(stage)) {
        bool any = false;
        for (const Microbe& s : B.species) any = any || (s.state == SS_LIVING && (s.cycle & LC_DORMANT));
        if (!any) {
            for (Microbe& s : B.species) if (s.state == SS_LIVING && !colonyPlan(s.plan)) {
                s.cycle |= LC_DORMANT | LC_WAKING | LC_SPORULATING;
                s.sporeYears = std::pow(10.0, 2 + 5 * unitFromHash(mix64(s.seed ^ SALT_FIX)));
                break;
            }
        }
        auto glow = [&](auto fits) { for (Microbe& s : B.species) if (s.state == SS_LIVING && fits(s)) { s.glows = true; return; } };
        if (W.g.hasTrait(TR_MILKY_SEAS) || W.g.hasTrait(TR_LIGHTS_IN_WATER)) glow([](const Microbe& s) { return s.niche == NI_LIQUID && s.way != LW_LIGHT; });
        if (kindHas(b.type, KT_VENT_LIFE)) glow([](const Microbe& s) { return (s.cycle & LC_DORMANT) != 0; });
        if (W.g.hasTrait(TR_LUMINOUS_FLORA)) glow([](const Microbe& s) { return s.niche == NI_SOIL && s.way == LW_DEAD; });
        if (W.g.hasTrait(TR_SNOW_ALGAE)) {   // Z-03: red snow's alga (Hoham and Remias 2020): the snow's own species, else the first that lives by light, else the
            // first alive, made a psychrophile of the melt living by light, its astaxanthin red, its red cysts outlasting the winter
            Microbe* a = nullptr;
            for (Microbe& m : B.species) if (!a && m.state == SS_LIVING && m.niche == NI_SNOW) a = &m;
            for (Microbe& m : B.species) if (!a && m.state == SS_LIVING && m.way == LW_LIGHT) a = &m;
            for (Microbe& m : B.species) if (!a && m.state == SS_LIVING) a = &m;
            if (a) {
                a->niches |= SN; a->way = LW_LIGHT; a->substrate = SU_NONE; a->pigment = PG_RED;
                a->tMin = -6; a->tOpt = 1 + 4 * unitFromHash(mix64(a->seed ^ SALT_FIX ^ 0x5A0ULL)); a->tMax = a->tOpt + 8;
                a->cycle |= LC_DORMANT | LC_WAKING; if (a->sporeYears <= 0) a->sporeYears = 10;
                a->abundance = std::max(a->abundance, 0.6);
            }
        }
    }
    return B;
}

// a system's biotas: each world's, then the spores impacts carried from its living worlds to their airless and icy neighbours with no life of
// their own (Melosh 1988): from a living world of the neighbour's family (its planet, a sibling moon) most often, else from the nearest
// living world beside it; one or two of its spore formers, in the neighbour's shade and ice, those that outlast the time since their impact
// where any does
std::shared_ptr<const SystemLife> makeLife(const StarSystem& sys) {
    auto L = std::make_shared<SystemLife>();
    const int nb = (int)sys.bodies.size();
    DrifterPast past;
    const bool dark = sys.sunless();
    if (dark) past = drifterPast(sys.star);
    L->bodies.reserve(nb);
    for (int i = 0; i < nb; i++) L->bodies.push_back(drawWorld(sys, i, dark ? &past : nullptr));
    for (int n = 0; n < nb; n++) {
        const Body& N = sys.bodies[n];
        if (N.type < 0 || N.type >= PT_COUNT || !KINDS[N.type].landable || N.wanderer) continue;
        Biota& T = L->bodies[n];
        const LifeRow& row = BUILT_BIOTA[N.type];
        if (!row.keepsSpores || row.furnace || (T.stage != LS_STERILE && T.stage != LS_PREBIOTIC)) continue;
        int src = -1; bool family = false; double best = 1e300;
        for (int a = 0; a < nb; a++) {
            if (a == n || !livingStage(L->bodies[a].stage) || !neighbours(sys, a, n)) continue;
            const int wa = worldOf(sys, a), wn = worldOf(sys, n);
            const bool fam = wa == wn;
            const double d = fam ? 0 : std::fabs(std::log(std::max(sys.bodies[wa].orbitRadiusKm, 1.0) / std::max(sys.bodies[wn].orbitRadiusKm, 1.0)));
            if ((fam && !family) || (fam == family && d < best)) { src = a; family = fam; best = d; }
        }
        Rng r(N.seed ^ SALT_CARRIED);
        const double reach = r.uni(), many = r.uni();
        if (src < 0 || reach >= (family ? 0.85 : 0.45)) continue;
        const Biota& A = L->bodies[src];
        const int take = many < 0.5 ? 1 : 2;
        const uint32_t keep = T.habitats & (IC | DU | RO);
        for (int k = 0; k < take; k++) {
            const double age = std::pow(10.0, 3 + 3.5 * r.uni()), pick = r.uni(), place = r.uni();
            int outlast[64], no = 0, longest = -1;
            for (const Microbe& s : A.species) {
                if (s.state != SS_LIVING || !(s.cycle & LC_DORMANT)) continue;
                bool have = false;
                for (const Microbe& t : T.species) have = have || (t.fromBody == src && t.fromIndex == s.index);
                if (have) continue;
                if (s.sporeYears > age && no < 64) outlast[no++] = s.index;
                if (longest < 0 || s.sporeYears > A.species[longest].sporeYears) longest = s.index;
            }
            const int j = no > 0 ? outlast[std::min(no - 1, (int)(pick * no))] : longest;
            if (j < 0) break;
            Microbe c = A.species[j];
            c.state = SS_DORMANT; c.index = (int)T.species.size();
            c.niche = (keep & IC) && (place < 0.5 || !(keep & (DU | RO))) ? NI_ICE : ((keep & DU) ? NI_DUST : NI_ROCK);
            c.niches = nicheBit(c.niche);
            c.abundance = std::max(0.01, 0.1 * c.abundance);
            c.doseGy *= 3;
            c.ageYears = age;
            c.fromBody = src; c.fromIndex = j;
            T.species.push_back(c);
        }
        if (T.species.empty() || T.species.back().fromBody < 0) continue;
        if (T.stage == LS_PREBIOTIC) T.second = LS_PREBIOTIC;
        T.stage = LS_DORMANT;
        T.genesis = A.genesis; T.chemistry = A.chemistry;
        T.dormantYears = 0;
        for (const Microbe& s : T.species) if (s.fromBody >= 0) T.dormantYears = std::max(T.dormantYears, s.ageYears);
    }
    return L;
}

std::mutex g_lifeMutex;
std::list<std::pair<uint64_t, std::shared_ptr<const SystemLife>>> g_life;   // the last sixteen, the newest first
uint64_t systemKey(const StarSystem& sys) {
    uint64_t h = hashCombine(hash3i(sys.star.sx, sys.star.sy, sys.star.sz, sys.star.seed), (uint64_t)sys.bodies.size());
    for (const Body& b : sys.bodies) h = hashCombine(h, b.seed ^ ((uint64_t)(b.type + 1) << 56) ^ ((uint64_t)(b.parent + 2) << 48) ^ (b.glassed ? 1ULL << 40 : 0));
    return h;
}
}   // namespace

const std::array<LifeRow, PT_COUNT> BIOTA = BUILT_BIOTA;

double biotaMeanC(const StarSystem& sys, const Body& b) {
    const KindDef& K = KINDS[b.type];
    if (sys.sunless()) return b.tempK - 273.15;
    if (livingGround(b.type)) return 15.0 + (b.tempK + K.greenhouseK - 288.0) * 0.3 + K.tempAdd;   // the climate's mean (B-321's curve, its bias)
    if (bodyHasAir(b)) return (b.tempK - 273.15) * 0.75 + K.tempOffset + K.tempAdd;
    return (b.tempK - 273.15) * 1.3 - 60 + K.tempAdd;
}

std::shared_ptr<const SystemLife> lifeOf(const StarSystem& sys) {
    const uint64_t key = systemKey(sys);
    {
        std::lock_guard<std::mutex> lk(g_lifeMutex);
        for (auto it = g_life.begin(); it != g_life.end(); ++it)
            if (it->first == key) { auto hit = *it; g_life.erase(it); g_life.push_front(hit); return hit.second; }
    }
    std::shared_ptr<const SystemLife> L = makeLife(sys);
    std::lock_guard<std::mutex> lk(g_lifeMutex);
    g_life.push_front({key, L});
    if (g_life.size() > 16) g_life.pop_back();
    return L;
}

Biota biotaOf(const StarSystem& sys, int body) {
    if (body < 0 || body >= (int)sys.bodies.size()) return Biota{};
    return lifeOf(sys)->bodies[body];
}

Microbe shipSpecies(int k) {
    // the garden's soil (a rod on dead matter, its spores), the explorer's skin (a sphere at blood's warmth, the salt of sweat), the garden pond's
    // film (a green filament): home's, their limits Earth's commonest
    Microbe s;
    const Genesis g = homeGenesis();
    s.index = k; s.state = SS_LIVING; s.genesis = g.id; s.seed = hashCombine(g.id, 0x5A1B0000ULL + (uint64_t)k);
    s.cycle = LC_GROWING | LC_DIVIDING | LC_DYING; s.division = DV_FISSION; s.oxygen = 2; s.abundance = 0.5; s.clade = 0;
    s.phLo = 5.5; s.phHi = 8.5; s.doseGy = 50;
    if (k == 0) {
        s.plan = MP_ROD; s.niche = NI_SOIL; s.way = LW_DEAD; s.motion = MO_RUN_TUMBLE; s.taxis = TX_FOOD;
        s.tMin = 8; s.tOpt = 30; s.tMax = 45; s.saltHi = 1; s.cycle |= LC_DORMANT | LC_WAKING | LC_SPORULATING; s.sporeYears = 1e4;
    } else if (k == 1) {
        s.plan = MP_SPHERE; s.niche = NI_SOIL; s.way = LW_DEAD; s.motion = MO_NONE; s.oxygen = 1;
        s.tMin = 15; s.tOpt = 37; s.tMax = 45; s.saltLo = 0; s.saltHi = 2; s.pigment = PG_ORANGE;
    } else {
        s.plan = MP_FILAMENT; s.niche = NI_LIQUID; s.way = LW_LIGHT; s.motion = MO_GLIDING; s.taxis = TX_LIGHT; s.pigment = PG_BLUE_GREEN;
        s.tMin = 5; s.tOpt = 25; s.tMax = 38; s.division = DV_FRAGMENTS; s.cycle |= LC_COLONY;
    }
    s.niches = nicheBit(s.niche);
    return s;
}
