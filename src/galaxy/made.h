// J-02 (R-448, the seeded worlds): what a world was made into at a step of its seedings, as data. `life/seeding` works it out from
// the guide's `seeded` lines and the guide's clock; the game puts it into the world's generator by value (`BodyGen::made`) where the
// drawing gets its generator (`SpaceRenderer::genFor`, `SurfaceSite::initFrame`, the landing map's probe site). `unseasoned` and
// `thenWorld` take it off, so the peoples' plans, the sights and the drainage read the world as generated. A key of 0 is no layer:
// every read does nothing then, to the bit.
#pragma once
#include "core/types.h"
#include "galaxy/kinds.h"
#include <cstdint>
#include <string>

constexpr int MADE_LINES = 3;            // a world takes three seedings at most, each acting on the world the ones before left
constexpr int MADE_STEPS = 24;           // a seeding's spread is read in 24 steps, as the seasons are
constexpr double MADE_STEP_S = 300.0;    // a step's seconds on the guide's clock: the whole world in two hours

// J-03: what a made colour lies on: the land (the test layer's tint) or the waters (a dye's, a glow's)
enum MadeOn { MADE_ON_LAND = 1, MADE_ON_WATER = 2 };
// J-03: the waters by their salt (`MadeSpread::waters`' bits): fresh (lakes, rivers, pools), the sea's, brine
enum MadeWaters { MADE_FRESH = 1, MADE_SEA = 2, MADE_BRINE = 4, MADE_ALL_WATERS = 7 };
// J-04: the made air's traits (`MadeWorld::air`'s bits), in the order the data sheet names them (life/seeding's words)
enum MadeAirTrait { MA_THICK = 1, MA_RUNAWAY = 2, MA_ORANGE_HAZE = 4, MA_SULPHUR = 8, MA_OZONE = 16, MA_THIN_OZONE = 32, MA_COLD_HAZE = 64,
                    MA_GLOW_CLOUDS = 128, MA_DYE_CLOUDS = 256, MA_DYE_RAIN = 512, MA_SEA_SMOKE = 1024, MA_ENDLESS_RAIN = 2048, MA_SPORE_CLOUDS = 4096,
                    MA_FREEZING = 8192 };   // (J-11: the ice running to the equator)
constexpr int MADE_AIR_TRAITS = 14;
// J-04: a glowing cloud's form (`MadeWorld::cloudGlowForm`): patches, veins, whole decks
enum MadeGlowForm { MG_PATCHES = 0, MG_VEINS, MG_DECKS };
// J-05: the classes of ground a made look lies on, each with a colour and a bank of its own in every view: the waters (J-03's tint), the
// land, the snow and ice, the plants (a world's grass and forest where it has plants)
enum MadeClass { MC_WATER = 0, MC_LAND, MC_SNOW, MC_PLANTS, MC_COUNT };
// J-05: the made traits of the waters (`MadeWorld::seas`' bits), the ground (`grounds`) and the plants (`flora`), in the data sheet's order
// (life/seeding's words: a coloured snow and new leaves are named by their dye)
enum MadeSeaTrait { MS_PURPLE = 1, MS_BLACK = 2, MS_MILK = 4, MS_FOAM = 8, MS_JELLY = 16, MS_SOUP = 32, MS_SOUR = 64, MS_RUST_RIVERS = 128, MS_BOILING = 256 };
constexpr int MADE_SEA_TRAITS = 9;
enum MadeGroundTrait { MGT_COLOURED_SNOW = 1, MGT_DARK_ICE = 2, MGT_CRUSTS = 4, MGT_MATS = 8, MGT_STROMATOLITES = 16, MGT_RUST = 32, MGT_CHALK = 64,
                       MGT_GLASS = 128, MGT_SULPHUR = 256, MGT_PITTED = 512, MGT_SLIME = 1024 };
constexpr int MADE_GROUND_TRAITS = 11;
enum MadePlantTrait { MP_BLIGHT = 1, MP_ROT = 2, MP_FROST_BURN = 4, MP_GALLS = 8, MP_NEW_LEAVES = 16, MP_GLOWING = 32, MP_LUSH = 64, MP_BLEACHED = 128 };
constexpr int MADE_PLANT_TRAITS = 8;
// J-05: what a made waters' colour is (`MadeWorld::waterLook`): a dye's (J-03), a chalk's milk, a soup, a jelly, a rust's (the fresh waters
// alone), none but a foam on the shores (the waters' own colour)
enum MadeWaterLook { MWL_NONE = 0, MWL_DYE, MWL_MILK, MWL_SOUP, MWL_JELLY, MWL_RUST, MWL_FOAM };
// J-05: where on the land a line's land look lies (`MadeSpread::landOn`): bare ground (no plants, snow or ice), the shores and flats of its
// waters (within `MADE_SHORE_M` over them), rock, sand and dust
enum MadeLandOn { ML_BARE = 1, ML_SHORE = 2, ML_ROCK = 4, ML_SAND = 8 };
constexpr double MADE_SHORE_M = 8;
// J-08: the animals' fates (`MadeFate::fauna`'s bits), in the data sheet's order (IDEAS-seeded-worlds.md section 8): a die-off, the sicknesses
// its survivors show, their young born changed, a glow or a dye in their skins (dyed hides are named by their dye: "RED HIDES"), thriving,
// silence (every animal gone)
enum MadeFaunaTrait { MF_DIE_OFF = 1, MF_WASTING = 2, MF_MELTING = 4, MF_GROWTHS = 8, MF_SPROUTING = 16, MF_BLOATED = 32, MF_STONE_SKIN = 64,
                      MF_CRAZED = 128, MF_CHANGED_YOUNG = 256, MF_GLOWING = 512, MF_DYED = 1024, MF_THRIVING = 2048, MF_SILENCE = 4096 };
constexpr int MADE_FAUNA_TRAITS = 13;
constexpr int MF_SICKNESSES = MF_WASTING | MF_MELTING | MF_GROWTHS | MF_SPROUTING | MF_BLOATED | MF_STONE_SKIN | MF_CRAZED | MF_GLOWING | MF_DYED;
// J-08: what killed them (`MadeFate::death`): a nerve, a blood or a gut poison, a sulphide air, a thinned ozone, a flesh rotter, a shell rotter,
// the food lost (a soup's or a jelly's waters, a blight), the heat, the cold, an ammonia sea, a host in the blood
enum MadeDeath { MD_NONE = 0, MD_NERVE, MD_BLOOD, MD_GUT, MD_SULPHIDE, MD_THIN_OZONE, MD_ROT_FLESH, MD_ROT_SHELL, MD_STARVED, MD_HEAT, MD_COLD,
                 MD_AMMONIA, MD_HOST, MD_COUNT };
// J-08: the animals a harm reaches (`MadeFate::harm`'s order): the land's, the air's, the waters'
enum MadeFaunaClass { MFC_LAND = 0, MFC_AIR, MFC_WATER, MFC_COUNT };
// J-08: what steers the changed young (`MadeFate::steer`'s bits; section 8): a big or a small cell (giants, dwarfs), a dye (a new colour), a
// glow (spots, stripes), a spiral cell (twisted spines), a line that never stops (tumours, extra limbs), a flesh rotter (sores), a shell (stone
// plates, horns), a weak nerve poison (circling)
enum MadeSteer { MSR_BIG = 1, MSR_SMALL = 2, MSR_DYE = 4, MSR_GLOW = 8, MSR_SPIRAL = 16, MSR_STOP = 32, MSR_ROT = 64, MSR_SHELL = 128, MSR_NERVE = 256 };
constexpr int MADE_FATE_STEPS = 48;   // the remains' age runs on past the spread's 24 steps, till the last are bones
// J-08: what a fate does to a world's animals (a line's where it reached, the air's over the whole world): the traits named (`MadeFaunaTrait`
// bits); the harm on the land, in the air and in the waters, to the shelled alone, to the land's plant eaters alone (0..1 each: the share a
// species of middling resistance has lost); what killed the most; the sickness the survivors show (an MF_ bit) and the share of them that
// show it; the share of each herd born changed, what steers how and a new colour's dye (a dyed hide's too); how much they thrive (0..1); the
// steps since it began (a line's from its second step, the air's from its sixth; at a point, from when its spread reached it)
struct MadeFate {
    int fauna = 0, age = 0;
    double harm[MFC_COUNT] = {0, 0, 0};
    double shelled = 0, starve = 0;
    int death = MD_NONE;
    int sickness = 0; double sick = 0;
    double changed = 0; int steer = 0, dye = 0;
    double thrive = 0;
    bool on() const { return fauna != 0; }
};

// J-09: the overgrowth (IDEAS-seeded-worlds.md section 7): a line that never stops and binds its cells grows into a tissue over the land it
// reached (`MadeSpread::growth`). Its type by how the line lives and its genes; its texture by its cell's shape: blistered (spheres),
// fibrous like muscle (rods), folded like a brain (curved), coiled like gut (spirals)
enum MadeGrowthType { OG_NONE = 0, OG_FLESH, OG_FUNGUS, OG_SLIME_MOULD, OG_LICHEN, OG_CORAL, OG_SPONGE, OG_HAIR, OG_BLADDERS, OG_CARPETS, OG_FILMS,
                      OG_COUNT };
enum MadeGrowthTexture { OGT_BLISTERED = 0, OGT_FIBROUS, OGT_FOLDED, OGT_COILED, OGT_COUNT };
constexpr int MADE_GROWTH_START = 8;   // the overgrowth begins at its line's eighth step (after the animals) and creeps that far behind its spread
constexpr int MADE_GROWTH_STEPS = 64;  // J-10: its steps counted to here (every ruin taken whole by then)
constexpr double MADE_WET_GROUND = 0.3; // J-10: an overgrowth this wet squelches underfoot (WET GROUND)
// J-10: the overgrowths whose structures breathe (their kits' vents: surface/overgrowth.cpp), BREATHING
inline bool madeGrowthBreathes(int type) { return type == OG_FLESH || type == OG_SPONGE; }
// J-11: a world's made kind once its change passed a line (IDEAS-seeded-worlds.md section 10), the data sheet's kind line: its seas boiled away
// (MADE HOTHOUSE), frozen from pole to pole (MADE SNOWBALL), under a thick orange haze, purple seas under a sulphur air, a rusted ground under an
// oxygen sky, an overgrowth over the whole world (named by its type: FLESH WORLD and the rest), every animal and plant dead (CHARNEL WORLD), a
// cold or dry world made warmer or wetter (GREENED WORLD). Its heights and rivers stay as generated
enum MadeKind { MDK_NONE = 0, MDK_HOTHOUSE, MDK_SNOWBALL, MDK_HAZE, MDK_PURPLE, MDK_RUST, MDK_OVERGROWN, MDK_CHARNEL, MDK_GREENED, MDK_COUNT };
// J-09: a line's overgrowth: its type and texture, its colour and its veins' (a clear one's dark), its grain (smooth 0 .. knobbly 1: its cell's
// size), its wetness (dry and leathery 0 .. dripping 1: its slime), its hardness (soft 0, leathery 1, plated 2, bony 3: its shell), its glow at
// night (veins or spots, 0 none) and its colour, whether it is furred with cilia (its motion), how far it has crept round its line's place
// (radians) and for how many steps, the cover it lays (a soft tissue, threads, a hard one: `MAT_TISSUE`, `MAT_THREADS`, `MAT_HARD`)
struct MadeGrowth {
    int type = OG_NONE, texture = OGT_BLISTERED;
    RGB colour, veins;
    double grain = 0.5, wet = 0;
    int hard = 0;
    double glow = 0; RGB glowTint;
    bool hair = false;
    int age = 0;   // the steps since it began (0 .. MADE_STEPS: its structures grow with it)
    int steps = 0; // J-10: the same on past MADE_STEPS, to MADE_GROWTH_STEPS (its cover climbs the ruins with them)
    double reach = 0;
    int material = -1;
    double land = 0;   // J-11: the share of the world's land it can lie on (its line's band of warmth, no snow or ice: an overgrown world's past a half)
    bool on() const { return type != OG_NONE && reach > 0; }
};

// one seeding's spread: where the line was let go (a unit vector in the body's frame) and how far it has reached (radians of arc). J-03:
// where its line can live within that reach (the made pass's habitat): the waters by their salt, the land (a line that lives dried out,
// or in snow and ice), the band of mean warmth it grows in (none: -1e9 to 1e9); and whether it colours or lights what it reached (its
// share is the layer's tint's)
// J-05: the classes its line makes a look in beside the waters (`makes`, `MadeClass` bits: the land, the snow and ice, the plants), whether
// it lives in the plants (a host of plants, a rotter of wood: whatever its life dried), where on the land its land look lies (`MadeLandOn`;
// the shores' look by the waters it lives in), the waters its waters' look lies on (a rust's the fresh ones), the cover its land look lays at
// a share over a half (-1 none) and its albedo, and the mean warmth its plants' look stops at (a frost burn's: where frosts come)
// J-08: what its line does to the animals where it reached (`fate`, by its share of a point); J-09: what it grows into (`growth`)
struct MadeSpread {
    Vec3 place;
    double reach = 0;
    int waters = MADE_ALL_WATERS;
    bool land = true;
    double tLo = -1e9, tHi = 1e9;
    bool looks = true;
    int makes = 0;
    bool plants = false;
    int landOn = 0;
    int lookWaters = MADE_ALL_WATERS;
    int cover = -1;
    double coverAlbedo = 0.5;
    double coldC = 1e9;
    MadeFate fate;
    MadeGrowth growth;
    int pits = 0;   // J-10: a stone rotter's steps since it was let go, to MADE_GROWTH_STEPS (0: its line pits no stone): the ruins in its reach crumble
    bool j05() const { return makes != 0 || plants || landOn != 0 || lookWaters != MADE_ALL_WATERS || cover >= 0 || coldC < 1e8; }
};

// J-05: a made look over one class of the ground (the land, the snow and ice, the plants): whether a line makes it (its share is set by the
// made pass where that line lives), its colour (none: the ground's own, its effects alone), the trait it is (`kind`, a `Made*Trait` bit) and
// its form (`variant`: a crust's polygons, ripples or rosettes; a blight's spots, stands or all), the dye it is named by (a coloured snow's,
// new leaves'; 0 none), an albedo over the ground's (0: its own)
struct MadeLook {
    bool on = false, tinted = false;
    RGB tint;
    int kind = 0, variant = 0, dye = 0;
    double albedo = 0;
};

struct MadeWorld {
    uint64_t key = 0;               // 0 none; else the hash of every field below (`madeKey`), what the caches key on beside the seed
    int step = 0;                   // the latest seeding's step (0 when it was just let go, MADE_STEPS when it reached the whole world)
    int lines = 0;                  // the spreads in `spread`
    MadeSpread spread[MADE_LINES];
    // what the spreads made of the land they reached (J-02: the test layer's; J-03 on, what the lines' genes make there)
    int cover = -1;                 // a material laid over the land (never over water or lava), -1 none
    double coverAlbedo = 0.5;       // and its albedo
    bool tinted = false;            // a colour over the land or the waters, drawn in a bank of its own in each view
    RGB tint;
    // J-03: what the tint lies on (`MadeOn`: the test's the land, a line's the waters), its word (life's `WorldDye`; 0 none: the waters'
    // own colour under a glow), the made waters' glow at night (0 none) and its colour; the body's warmth for the spreads' bands
    // (`placeMeanC`: its equilibrium, a sunless world's)
    int tintOn = MADE_ON_LAND;
    int dye = 0;
    double glow = 0;
    RGB glowTint;
    double tempK = 0;
    bool sunless = false;
    double freezeC = 0;             // J-03: where the world's liquid freezes fresh (water 0 C; the sea's 2 degrees under)
    // J-04: the made air and the climate it makes (IDEAS-seeded-worlds.md sections 5 and 6). The world's mean warmth now over its generated
    // (C; under 0 a haze's cooling) and its air's pressure over its generated (a factor); the traits the data sheet names (`MadeAirTrait`
    // bits); a haze over the sky and its colour (an orange haze's, a cold haze's); a thick air's pale horizon and a sulphur air's yellow one
    // (0..1); an ozone formed (+1) or thinned (-1); the plants dried by the heat (0..1); a runaway's seas (0 none, steaming from a sixth to
    // 1, dry to their old beds at 2); more cloud, more rain, more snow on a cold world (0..1); the clouds' colour (its share, a dye's or the
    // spores'), their glow at night (0 none; its colour and form), the rain's colour; a fog lying on the seas; spores in the air. Each look
    // is 0 under the share its trait is named at, so what is drawn is what the data sheet says
    double warmC = 0, pressureK = 1;
    int air = 0;
    double haze = 0; RGB hazeTint;
    double thick = 0, sulphur = 0;
    int ozone = 0;
    double wilt = 0;
    double runaway = 0;
    double cloudMore = 0, rainMore = 0, snowMore = 0;
    double cloudTintK = 0; RGB cloudTint; int cloudDye = 0;
    double cloudGlow = 0; RGB cloudGlowTint; int cloudGlowForm = MG_PATCHES;
    double rainTintK = 0; RGB rainTint; int rainDye = 0;
    double seaSmoke = 0;
    double spores = 0; RGB sporeTint;
    // J-05: the waters, the ground and the plants (IDEAS-seeded-worlds.md sections 5 and 6). The traits the data sheet names (`Made*Trait`
    // bits); what the waters' colour is (`MadeWaterLook`); a foam on their shores (its colour), their waves slowed by a jelly, the seas
    // boiling (a runaway's steam); every water's colour turned by a sulphide (purple, black without a sun) and that colour, the reefs bleached
    // by a sour sea; the land's, the snow's and the plants' looks; the share of the trees a plants' look strikes, their glow at night and its
    // colour, galls on them, trees fallen; the ground rusted by an oxygen, the plants lush under an ozone or an endless rain, bleached under a
    // thin ozone (0..1 each)
    int seas = 0, grounds = 0, flora = 0;
    int waterLook = MWL_NONE;
    double foam = 0; RGB foamTint;
    double jelly = 0, boil = 0;
    double seaShade = 0; RGB seaShadeTint;
    double sour = 0;
    MadeLook land, snow, plant;
    double plantShare = 1, plantGlow = 0; RGB plantGlowTint;
    double galls = 0, rot = 0;
    double rustGround = 0, lush = 0, bleached = 0;
    // J-08: the animals' fates: the air's over the whole world (`fate`; a line's where it reached is its spread's), every trait named of all of
    // them (`fauna`), the steps since the first came (`fateAge`, to MADE_FATE_STEPS: the remains' age), whether remains rot fast (a flesh
    // rotter's)
    MadeFate fate;
    int fauna = 0, fateAge = 0;
    bool rotFast = false;
    double radiusKm = 0;            // (the body's, for where a spread had reached at each step: `madeFateAt`)
    // J-09: the overgrowths' types named (`MadeGrowthType` bits, 1 << type), and the latest line's that grows (its index in `spread`, -1 none:
    // the one the ground's tile and bank and the globe's stain follow)
    int growths = 0, growthLine = -1;
    // J-11: the ice's run on a living wet world cooled past the snowball's line (0 none, a sixth to 1 while it spreads to the equator, 2 frozen
    // over), and the world's made kind (`MadeKind`; an overgrown world's type, `MadeGrowthType`; a greened world's was cold, else dry)
    double freeze = 0;
    int kind = MDK_NONE, kindGrowth = OG_NONE;
    bool kindCold = false;
    bool grown() const { return growths != 0; }
    bool frozen() const { return freeze >= 2; }
    // J-11: the dead cover a made kind lays over its living ground (`DeadCover`): a made hothouse the scorched felisian's, a made snowball the
    // frozen felisian's
    int deadCover() const { return kind == MDK_HOTHOUSE ? DC_SCORCHED : (kind == MDK_SNOWBALL ? DC_FROZEN : DC_NONE); }
    bool on() const { return key != 0; }
    bool fated() const { return fauna != 0; }
    bool aired() const { return air != 0 || warmC != 0 || pressureK != 1; }   // J-04: its air or its climate moved
    bool j05() const {                                                         // J-05: its waters, ground or plants changed (a dye's waters are J-03's)
        return seas != 0 || grounds != 0 || flora != 0 || waterLook > MWL_DYE || foam > 0 || jelly > 0 || boil > 0 || seaShade > 0 || sour > 0 || land.on ||
               snow.on || plant.on || galls > 0 || rot > 0 || rustGround > 0 || lush > 0 || bleached > 0;
    }
    bool ground() const { return cover >= 0 || tinted || runaway >= 2 || land.on || snow.on || plant.on || grown(); }   // anything the planet function's last pass draws
    bool looks() const { return ground() || aired() || j05() || fated() || grown(); }    // anything drawn or read (J-08: the animals' fates too)
    bool dried() const { return runaway >= 2; }                                // J-04: a runaway's seas gone to their old beds
    bool shares() const { return tinted || land.on || snow.on || plant.on; }  // J-05: a share at each point (`SurfaceSample::madeTint`)
    const MadeLook* lookOf(int cls) const { return cls == MC_LAND ? &land : (cls == MC_SNOW ? &snow : (cls == MC_PLANTS ? &plant : nullptr)); }
};

// the hash of a layer's fields, never 0
uint64_t madeKey(const MadeWorld& m);
// how much of a point the spreads reached, 0..1: 1 well inside, 0 outside, a ragged edge between. The edge's noise is at the reach's own
// scale and never fades with the detail (B-404's rule), so the globe, the maps and the ground draw one edge
double madeReachAt(const MadeWorld& m, const Vec3& unit, uint64_t seed);
// J-03: one spread's (`i`) share of a point, as above
double madeSpreadAt(const MadeWorld& m, int i, const Vec3& unit, uint64_t seed);
// J-03: whether a spread's line lives at a point of this kind: in water of this salt (0 fresh, 1 the sea's, 2 brine) or on the land, at this
// mean warmth (C; read only where the spread has a band)
bool madeLives(const MadeSpread& s, bool wet, int salt, double meanC);
// a spread's reach at a step on a world of this radius: the place (2 km) at step 1, the region (150 km, the landing map's zoom) at step 6,
// the whole world at MADE_STEPS (J-03 makes it follow the warmth, the waters and the land the line can live in)
double madeReachOfStep(int step, double radiusKm);
// J-04: the share of the sphere a reach covers (its cap's area over the whole: 0 none, 1 the whole world)
double madeCapShare(double reach);
// J-08: the animals' fate at a point: the air's, and each line's by its spread's share there (harms and shares joined as chances, 1 - the
// product of what each spares; the sickness, the steering and the dye the latest line's that has them; the age the oldest's there, a line's
// counted from the step its spread reached the point)
MadeFate madeFateAt(const MadeWorld& m, const Vec3& unit, uint64_t seed);
// J-09: how much of a point an overgrowth has covered, 0..1 (the latest line's that covers it: `line`, -1 none): its creep behind its spread
// with a ragged edge of fingers, at the reach's own scale and at a few hundred metres (neither fades with the detail)
double madeGrowthAt(const MadeWorld& m, const Vec3& unit, uint64_t seed, int& line);
// J-10: the steps an overgrowth has lain at a point since its edge passed there (0 at the edge, the fraction by how far past the step's reach
// the point lies; -1 where none covers it): the latest line's that covers it, its edge read as `madeGrowthAt` reads it
double madeGrowthSince(const MadeWorld& m, const Vec3& unit, uint64_t seed);
// J-11: a made kind's words: its name ("PURPLE WORLD"; an overgrown world's by its type, "FLESH WORLD"; empty for none), the data sheet's
// sentence in its kind's place ("LIVING TISSUE COVERS THE LAND FROM COAST TO COAST.") and `describeBody`'s phrase in place of its kind's
std::string madeKindName(const MadeWorld& m);
std::string madeKindSentence(const MadeWorld& m);
std::string madeKindPhrase(const MadeWorld& m);
