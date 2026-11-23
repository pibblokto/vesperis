// J-02: a world's seedings and the layer they make of it (life/seeding.h); J-03: the place, the verdict, the drift and the spread
#include "seeding.h"
#include "sequences.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

const char* const SEED_FATE_NAMES[SF_COUNT] = {"GROWS", "A FURNACE", "WRONG LIQUID", "TOO HOT", "TOO COLD", "WRONG SALT", "NO FOOD", "NO LIGHT", "FROZEN",
                                               "DOES NOT LIVE", "BURNT", "OXYGEN KILLED IT", "NO OXYGEN", "TOO ACID", "TOO ALKALINE", "RADIATION", "DRIED OUT",
                                               "OUTGROWN BY THE NATIVES"};

const char* const MADE_AIR_NAMES[MADE_AIR_TRAITS] = {"THICK AIR", "RUNAWAY", "ORANGE HAZE", "SULPHUR AIR", "OZONE", "THIN OZONE", "COLD HAZE",
                                                      "GLOWING CLOUDS", "COLOURED CLOUDS", "COLOURED RAIN", "SEA SMOKE", "ENDLESS RAIN", "SPORE CLOUDS",
                                                      "FREEZING OVER", "SILVER CLOUDS"};
const char* const MADE_SEA_NAMES[MADE_SEA_TRAITS] = {"PURPLE SEAS", "BLACK SEAS", "MILK SEAS", "FOAM SEAS", "JELLY SEAS", "SOUP SEAS", "SOUR SEAS",
                                                      "RUST RIVERS", "BOILING SEAS"};
const char* const MADE_GROUND_NAMES[MADE_GROUND_TRAITS] = {"COLOURED SNOW", "DARK ICE", "CRUSTS", "MATS", "STROMATOLITES", "RUST GROUND", "CHALK GROUND",
                                                            "GLASS SANDS", "SULPHUR CRUSTS", "PITTED STONE", "SLIME", "COLOURED CRACKS",
                                                            "COLOURED SEEPS"};
const char* const MADE_PLANT_NAMES[MADE_PLANT_TRAITS] = {"BLIGHT", "ROT", "FROST BURN", "GALLS", "NEW LEAVES", "GLOWING FORESTS", "LUSH", "BLEACHED"};
const char* const MADE_FAUNA_NAMES[MADE_FAUNA_TRAITS] = {"DIE-OFF", "WASTING", "MELTING", "GROWTHS", "SPROUTING", "BLOATED", "STONE SKIN", "CRAZED",
                                                          "CHANGED YOUNG", "GLOWING ANIMALS", "DYED HIDES", "THRIVING", "SILENCE"};
const char* const MADE_GROWTH_NAMES[OG_COUNT] = {"NO OVERGROWTH", "FLESH", "FUNGUS", "SLIME MOULD", "LICHEN", "CORAL", "SPONGE", "HAIR", "BLADDERS",
                                                  "CARPETS", "FILMS"};
const char* const MADE_DEATH_NAMES[MD_COUNT] = {"NOTHING", "A NERVE POISON", "A BLOOD POISON", "A GUT POISON", "A SULPHIDE AIR", "A THIN OZONE",
                                                "A FLESH ROTTER", "A SHELL ROTTER", "HUNGER", "THE HEAT", "THE COLD", "AN AMMONIA SEA", "A HOST IN THE BLOOD"};

namespace {
bool rockFamily(int m) { return m == MAT_ROCK || m == MAT_BASALT || m == MAT_METAL || m == MAT_GRAPHITE || m == MAT_GLASS || m == MAT_QUARTZ; }
bool wetAt(const SurfaceSample& s) { return s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water); }

// the place's pH: an acid world's, an ammonia world's, the rust's iron, a spring's (an acid one in four tenths), a salt flat's (a soda lake's
// in half), the sea's, fresh water's, the ground's
double placePH(int type, int chem, const Microhabitat& h) {
    if (chem == CH_ACID) return -0.5;
    if (chem == CH_AMMONIA) return 11.5;
    if (type == PT_RUST) return 3;
    const double u = hash01(mix64(h.place ^ 0x9A7E5ULL));
    if (h.niche == NI_SPRING) return u < 0.4 ? 2.5 : 7.5;
    if (h.niche == NI_SALT) return u < 0.5 ? 9.8 : 7.8;
    if (h.niche == NI_LIQUID) return h.salt >= 1 ? 8.0 : 7.2;
    return 6.8;
}
// the place's dose in a year (Gy): under an air, or the bare sky's; ten times with no field to turn the particles; a hot star's ultraviolet,
// a neutron star's and a Wolf-Rayet's radiation; less under water, within the ice, the rock, the soil and the silt, under snow and a mat
double placeDoseGy(const Body& b, const BodyGen& g, int starClass, int niche) {
    double d = g.hasAir ? 1.0 : 300.0;
    if (magneticClass(magneticField(b)) < 1) d *= 10;
    if (lightOfStar(starClass) == CL_BLUE) d *= 3;
    if (starClass == STAR_PULSAR || starClass == STAR_NEUTRON || starClass == STAR_WOLF_RAYET) d *= 10;
    switch (niche) {
        case NI_LIQUID: d *= 0.01; break;
        case NI_ICE: case NI_ROCK: case NI_SOIL: case NI_SILT: case NI_PLUME: d *= 0.03; break;
        case NI_SNOW: case NI_MAT: d *= 0.3; break;
        default: break;
    }
    return d;
}
int fateOfFit(int fit) {
    switch (fit) {
        case GF_COLD: return SF_COLD;
        case GF_HOT: return SF_HOT;
        case GF_SALT: return SF_SALT;
        case GF_SOLVENT: return SF_SOLVENT;
        case GF_HUNGRY: return SF_HUNGRY;
        case GF_DARK: return SF_DARK;
        case GF_UNVIABLE: return SF_UNVIABLE;
        case GF_FROZEN: return SF_FROZEN;
        case GF_BURNT: return SF_BURNT;
        default: return SF_GROWS;
    }
}
double freezeOf(int chem) { return chem == CH_HYDROCARBON ? -183 : (chem == CH_AMMONIA ? -97 : 0); }
// a glowing line's light in the waters (the dark oceans' own three: a sea-green, a blue, a green), by its key
const RGB GLOW_COLOURS[3] = {RGB(0.30f, 0.86f, 0.80f), RGB(0.36f, 0.62f, 0.95f), RGB(0.52f, 0.92f, 0.46f)};

// J-05: the colours of the made waters, ground and plants: a chalk's milk (a coccolithophore bloom's turquoise white: Holligan et al. 1993), a
// jelly's grey green, a bloom's murky soup (a light-feeder's own pigment where it has one), a rust's orange (the Rio Tinto's); foam white, yellow
// or pink; the mats' olive, orange or purple brown, a stromatolite's grey tan, a sulphur crust's yellow, chalk's white, glass's pale grey blue,
// rust, a pitted stone's grey, slime's wet black green, a crust's dark brown (a desert's biological crust: Belnap 2003); a dark ice's grey brown
// (Greenland's dark zone: Wientjes and Oerlemans 2010), snow's white; a blight's grey, black or rust, a frost burn's black, a rot's brown; a
// sulphide's purple waters (the purple sulphur bacteria of a Canfield ocean) or, with no sun to feed them, black
const RGB MILK_SEA(0.66f, 0.84f, 0.84f), JELLY_SEA(0.46f, 0.56f, 0.46f), MURKY_SOUP(0.40f, 0.38f, 0.22f), RUST_WATER(0.72f, 0.36f, 0.12f);
const RGB FOAM_LOOKS[3] = {RGB(0.94f, 0.94f, 0.90f), RGB(0.92f, 0.86f, 0.56f), RGB(0.94f, 0.76f, 0.78f)};
const RGB MAT_LOOKS[3] = {RGB(0.36f, 0.40f, 0.18f), RGB(0.58f, 0.36f, 0.14f), RGB(0.40f, 0.26f, 0.34f)};
const RGB STROMATOLITE_LOOK(0.64f, 0.60f, 0.48f), SULPHUR_LOOK(0.88f, 0.78f, 0.24f), CHALK_LOOK(0.90f, 0.89f, 0.84f), GLASS_LOOK(0.76f, 0.82f, 0.86f);
const RGB RUST_LOOK(0.62f, 0.30f, 0.16f), PITTED_LOOK(0.34f, 0.32f, 0.30f), SLIME_LOOK(0.20f, 0.27f, 0.17f), CRUST_LOOK(0.24f, 0.20f, 0.15f);
const RGB DARK_ICE(0.42f, 0.38f, 0.34f), SNOW_WHITE(0.93f, 0.94f, 0.96f);
const RGB CRACK_STAIN(0.46f, 0.30f, 0.20f);   // J-13: the brown of Europa's lineae (salts and what wells up with them), under a dye's colour
const RGB SEEP_STAIN(0.52f, 0.40f, 0.30f);    // J-16 (KI-506): the ochre of a spring's seep on bare rock (its iron and salts), under a dye's colour
const RGB BLIGHT_LOOKS[3] = {RGB(0.50f, 0.50f, 0.46f), RGB(0.14f, 0.13f, 0.12f), RGB(0.58f, 0.32f, 0.14f)};
const RGB FROST_BURN(0.16f, 0.12f, 0.09f), ROT_BROWN(0.44f, 0.36f, 0.22f);
const RGB PURPLE_SEA(0.40f, 0.18f, 0.46f), BLACK_SEA(0.07f, 0.07f, 0.06f);
double lumaOf(const RGB& c) { return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b; }

// J-05: what a line makes of the waters, the land, the snow and ice and the plants it lives in (IDEAS-seeded-worlds.md section 5): its
// waters' look and colour (a dye's is J-03's, kept by the layer), the waters it lies on, a foam on their shores and its colour, a jelly's slow
// waves; its land's look, where it lies, its cover; its snow's; its plants' (the share of the trees struck, a glow and its colour, galls, the
// trees fallen, where frosts come); whether it lives in the plants; the traits
struct LineLooks {
    int water = MWL_NONE; RGB waterTint; int waterOn = MADE_ALL_WATERS;
    double foam = 0, jelly = 0; RGB foamTint;
    MadeLook land, snow, plant;
    int landOn = 0, cover = -1; double coverAlbedo = 0.5;
    double plantShare = 1, plantGlow = 0, galls = 0, rot = 0, coldC = 1e9; RGB plantGlowTint;
    double landGlow = 0; RGB landGlowTint;   // J-16 (KI-506): a deep line's cracks' or seeps' night light
    bool plants = false;
    int seas = 0, grounds = 0, flora = 0;
};
LineLooks looksOf(const Seeding& s, const MadeSpread& sp) {
    LineLooks L;
    if (!s.run) return L;
    const CultureKind& k = s.wild.kind;
    const CultureWorld& w = k.world;
    const uint64_t h = fnv1a(s.line);
    const RGB water = s.verdict.water, forest = s.verdict.forest;
    const bool slime = w.slime > 0, lift = w.lift > 0, frost = w.frostC > FROST_NONE + 1, dyes = w.dyes(), glows = k.s.glows;
    const int shell = w.shells() ? w.shell : SHL_NONE;
    const bool hosts = w.hosted() && (w.host == HO_PLANTS || w.host == HO_BOTH);
    const bool rotsWood = w.rots() && w.rot == ROT_WOOD, rotsStone = w.rots() && w.rot == ROT_STONE;
    const bool wet = sp.waters != 0, onLand = sp.land;   // (every line lives in the waters of its salt)
    L.plants = hosts || rotsWood;
    // its waters: a bloom that never ends (a jelly where its cells slime), a chalk shell's milk, a rust's fresh waters (a stone rotter's acid
    // with a sulphur shell's too); a foam on their shores where its lift and its slime meet
    if (wet) {
        if (w.overgrows && slime) { L.water = MWL_JELLY; L.waterTint = lerp(water, JELLY_SEA, 0.6f); L.jelly = clampd(0.5 + 0.5 * w.slime, 0, 1); L.seas |= MS_JELLY; }
        else if (w.overgrows) {
            const RGB soup = k.s.way == LW_LIGHT && k.s.pigment != PG_NONE ? pigmentColour(k.s.pigment) * 0.7f : MURKY_SOUP;
            L.water = MWL_SOUP; L.waterTint = lerp(water, soup, 0.7f); L.seas |= MS_SOUP;
        } else if (shell == SHL_CHALK && !dyes) {   // (a dye's colour is the waters' own: J-03's)
            L.water = MWL_MILK; L.waterTint = lerp(water, MILK_SEA, (float)(0.45 + 0.35 * clampd(w.shellRate, 0, 1))); L.seas |= MS_MILK;
        } else if ((shell == SHL_RUST || (rotsStone && shell == SHL_SULPHUR)) && (sp.waters & MADE_FRESH) && !dyes && !glows) {   // (a glow keeps its waters, as a dye)
            L.water = MWL_RUST; L.waterOn = MADE_FRESH; L.waterTint = lerp(water, RUST_WATER, 0.7f); L.seas |= MS_RUST_RIVERS;
        }
        if (dyes && w.dye == WD_BLACK) L.seas |= MS_BLACK;   // (a black dye's waters: the data sheet names them once)
        if (lift && slime && !w.overgrows) {
            L.foam = clampd(0.4 + 0.6 * std::min(w.lift, w.slime), 0, 1); L.foamTint = FOAM_LOOKS[h % 3]; L.seas |= MS_FOAM;
            if (L.water == MWL_NONE) { L.water = MWL_FOAM; L.waterTint = water; }
        }
    }
    // the land: a line of the waters builds on the shores and flats of its waters (stromatolites where its slime builds on a chalk shell, mats
    // where it slimes alone); a line of the land (one that lives dried out, or in snow and ice) on the ground it lives on (a sulphur shell's
    // crusts, chalk, glass sands, rust; a stone rotter's pits; crusts where its cells bind, its slime on the rocks); a stone rotter in the rock
    auto land = [&](int trait, const RGB& c, int on, int cover, double coverAlbedo, double albedo) {
        L.land.on = L.land.tinted = true; L.land.kind = trait; L.land.tint = c; L.land.albedo = albedo;
        L.landOn = on; L.cover = cover; L.coverAlbedo = coverAlbedo; L.grounds |= trait;
    };
    if (sp.deep != MADE_SURFACE) {   // J-13: a line under the ground shows only where a sea under the ice comes up: its dye in the cracks and stains
        // (J-14: a cloud line's dye and glow are the air's: `airSumAt`); J-16 (KI-506): a line in the rock where its water seeps up through the bare
        // rock, in patches (SEEPS); a glowing one lights either at night, its own stain's colour where it has no dye
        if ((sp.deep == MADE_UNDER_ICE || sp.deep == MADE_DEEP_ROCK) && (dyes || glows)) {
            const bool ice = sp.deep == MADE_UNDER_ICE;
            const RGB stain = ice ? CRACK_STAIN : SEEP_STAIN;
            land(ice ? MGT_COLOURED_CRACKS : MGT_COLOURED_SEEPS, dyes ? lerp(stain, dyeColour(w.dye), 0.75f) : stain, ice ? ML_BARE : ML_ROCK, -1, 0.5, 0);
            L.land.dye = dyes ? w.dye : 0;
            if (glows) { L.landGlow = 1; L.landGlowTint = GLOW_COLOURS[h % 3]; }
        }
        L.plants = false;
        return L;
    }
    const bool ofLand = w.dry > 0 || k.s.niche == NI_SNOW || k.s.niche == NI_ICE;
    if (!ofLand && slime && shell == SHL_CHALK) land(MGT_STROMATOLITES, STROMATOLITE_LOOK, ML_SHORE, MAT_STROMATOLITE, 0.46, 0);
    else if (!ofLand && slime && !w.overgrows) land(MGT_MATS, MAT_LOOKS[(h >> 8) % 3], ML_SHORE, MAT_MATS, 0.24, 0);
    else if (ofLand) {
        if (shell == SHL_SULPHUR) land(MGT_SULPHUR, SULPHUR_LOOK, ML_BARE, MAT_SULPHUR, 0.62, 0);
        else if (shell == SHL_CHALK) land(MGT_CHALK, CHALK_LOOK, ML_BARE, -1, 0.5, 0.72);
        else if (shell == SHL_GLASS) land(MGT_GLASS, GLASS_LOOK, ML_SAND, -1, 0.5, 0.6);
        else if (shell == SHL_RUST) land(MGT_RUST, RUST_LOOK, ML_BARE, -1, 0.5, 0);
        else if (rotsStone) land(MGT_PITTED, PITTED_LOOK, ML_ROCK, -1, 0.5, 0);
        else if (w.bind > 0) {
            land(MGT_CRUSTS, dyes ? lerp(CRUST_LOOK, dyeColour(w.dye), 0.5f) : CRUST_LOOK, ML_BARE, MAT_CRUST, 0.2, 0);
            L.land.variant = (int)((h >> 12) % 3);   // in polygons, ripples or rosettes
        } else if (slime) land(MGT_SLIME, SLIME_LOOK, ML_ROCK, -1, 0.5, 0);
    } else if (rotsStone) land(MGT_PITTED, PITTED_LOOK, ML_ROCK, -1, 0.5, 0);
    // the snow and ice it lives on (where its warmth reaches under freezing): its dye's colour, a dark dye's dark ice
    if (onLand && dyes && sp.tLo < 0) {
        const RGB d = dyeColour(w.dye);
        L.snow.on = L.snow.tinted = true; L.snow.dye = w.dye;
        if (lumaOf(d) < 0.2) { L.snow.kind = MGT_DARK_ICE; L.snow.tint = lerp(DARK_ICE, d, 0.3f); L.snow.albedo = 0.3; L.grounds |= MGT_DARK_ICE; }
        else { L.snow.kind = MGT_COLOURED_SNOW; L.snow.tint = lerp(SNOW_WHITE, d, (float)(0.3 + 0.2 * clampd(w.dyeRate, 0, 1))); L.grounds |= MGT_COLOURED_SNOW; }   // (red snow's pink)
    }
    // the plants it lives in: a host's glow (its dye's new leaves with it), its dye's new leaves, its frost's burn where frosts come (an ice
    // nucleating bacterium's: Lindow et al. 1982); a rotter of wood's blight (in spots, stands or all, by how fast it rots), rotting faster the
    // forests fallen; a host's galls where it never stops or passes its genes on (a crown gall's: Agrobacterium)
    if (L.plants) {
        auto plant = [&](int trait, const RGB& c, bool tinted) { L.plant.on = true; L.plant.tinted = tinted; L.plant.kind = trait; L.plant.tint = c; L.flora |= trait; };
        if (hosts && glows) {
            plant(MP_GLOWING, dyes ? lerp(forest, dyeColour(w.dye), 0.6f) : forest, dyes);   // (undyed: the forest's own colour by day)
            L.plantGlow = 1; L.plantGlowTint = GLOW_COLOURS[h % 3];
            if (dyes) { L.flora |= MP_NEW_LEAVES; L.plant.dye = w.dye; }
        } else if (hosts && dyes) {
            plant(MP_NEW_LEAVES, lerp(forest, dyeColour(w.dye), (float)(0.55 + 0.25 * clampd(w.dyeRate, 0, 1))), true); L.plant.dye = w.dye;
        } else if (hosts && frost) { plant(MP_FROST_BURN, FROST_BURN, true); L.coldC = 10; }
        else if (rotsWood && w.rotRate < 0.6) {
            plant(MP_BLIGHT, BLIGHT_LOOKS[(h >> 16) % 3], true);
            L.plant.variant = w.rotRate < 0.4 ? 0 : (w.rotRate < 0.5 ? 1 : 2);   // spots, stands, all
            L.plantShare = L.plant.variant == 0 ? 0.3 : (L.plant.variant == 1 ? 0.6 : 1.0);
        } else if (rotsWood) { plant(MP_ROT, lerp(forest, ROT_BROWN, 0.45f), true); L.rot = clampd(w.rotRate, 0, 1); }
        if (hosts && (w.overgrows || w.carrier > 0)) { L.plant.on = true; L.galls = 1; L.flora |= MP_GALLS; if (!L.plant.kind) L.plant.kind = MP_GALLS; }
    }
    return L;
}

// J-04: where a line lives on its world (the made pass's habitat): the waters of its salt (as `growthIn` passes them), the land where it
// lives dried out (or in snow and ice), its band of warmth; and whether it colours or lights what it reached. J-05: a stone rotter lives in
// the rock; what it makes of the waters, the land, the snow and the plants (`looksOf`, into `looks` when asked)
MadeSpread spreadOf(const Seeding& s, LineLooks* looks = nullptr) {
    MadeSpread sp;
    sp.place = StarSystem::bodyFromLatLon(s.lat, s.lon);
    if (!s.run) return sp;
    const CultureKind& k = s.wild.kind;
    sp.waters = 0;
    for (int salt = 0; salt < 3; salt++) if (salt >= k.s.saltLo - 1 && salt <= k.s.saltHi + 1) sp.waters |= 1 << salt;
    sp.land = k.world.dry > 0 || k.s.niche == NI_SNOW || k.s.niche == NI_ICE || (k.world.rots() && k.world.rot == ROT_STONE);
    sp.tLo = k.s.tMin; sp.tHi = k.s.tMax;
    if (s.depthKm > 0) { sp.deep = s.verdict.place.where == AT_UNDER_ICE ? MADE_UNDER_ICE : MADE_DEEP_ROCK; sp.waters = 0; sp.land = false; }   // J-13
    if (s.verdict.place.aloft()) { sp.deep = MADE_CLOUDS; sp.waters = 0; sp.land = false; }   // J-14: in the clouds
    const LineLooks L = looksOf(s, sp);
    sp.looks = sp.deep == MADE_SURFACE && (k.world.dyes() || k.s.glows || L.water != MWL_NONE);
    sp.makes = (L.land.on ? 1 << MC_LAND : 0) | (L.snow.on ? 1 << MC_SNOW : 0) | (L.plant.on ? 1 << MC_PLANTS : 0);
    sp.plants = L.plants; sp.landOn = L.landOn; sp.lookWaters = L.waterOn; sp.cover = L.cover; sp.coverAlbedo = L.coverAlbedo; sp.coldC = L.coldC;
    if (looks) *looks = L;
    return sp;
}

// J-09: what a line that never stops grows into (IDEAS-seeded-worlds.md section 7). One that binds its cells grows a tissue over the land it
// reached: a chalk shell's coral, a glass shell's sponge, lift's bladders, flagella's hair, a light-feeder that lives dried out under a shield a
// lichen, another light-feeder a carpet, a dead-eater that rests a fungus, a grazer that moves a slime mould, else flesh (a chemical-eater's,
// a hunter's); one that slimes without binding lays films on the wet ground. Its variants by its genes: its colour (its dye, a light-feeder's
// pigment, else one of its type's own by its key), its texture by its cell's shape, its grain by its size, its wetness by its slime, its
// hardness by its shell (a dried-out one's leathery), its glow, cilia where it moves. It begins at its line's eighth step and creeps behind
// its spread, the whole world by its 32nd
const RGB FLESH_LOOKS[8] = {RGB(0.62f, 0.16f, 0.18f), RGB(0.86f, 0.52f, 0.54f), RGB(0.86f, 0.78f, 0.72f), RGB(0.42f, 0.46f, 0.58f),
                            RGB(0.42f, 0.20f, 0.40f), RGB(0.40f, 0.26f, 0.20f), RGB(0.12f, 0.09f, 0.09f), RGB(0.84f, 0.86f, 0.82f)};   // red, pink, pale, bruised grey-blue, purple, brown, black, clear
const RGB FUNGUS_LOOKS[4] = {RGB(0.90f, 0.88f, 0.82f), RGB(0.80f, 0.72f, 0.56f), RGB(0.56f, 0.54f, 0.52f), RGB(0.88f, 0.56f, 0.20f)};
const RGB MOULD_LOOKS[3] = {RGB(0.92f, 0.82f, 0.20f), RGB(0.90f, 0.50f, 0.12f), RGB(0.86f, 0.30f, 0.20f)};
const RGB LICHEN_LOOKS[3] = {RGB(0.55f, 0.60f, 0.45f), RGB(0.85f, 0.55f, 0.15f), RGB(0.80f, 0.78f, 0.30f)};
const RGB CORAL_LOOKS[3] = {RGB(0.92f, 0.88f, 0.80f), RGB(0.90f, 0.56f, 0.56f), RGB(0.90f, 0.62f, 0.30f)};
const RGB SPONGE_LOOKS[3] = {RGB(0.86f, 0.82f, 0.56f), RGB(0.62f, 0.64f, 0.66f), RGB(0.52f, 0.62f, 0.78f)};
const RGB HAIR_LOOKS[3] = {RGB(0.86f, 0.82f, 0.74f), RGB(0.12f, 0.10f, 0.10f), RGB(0.62f, 0.24f, 0.16f)};
const RGB BLADDER_LOOKS[3] = {RGB(0.86f, 0.62f, 0.66f), RGB(0.62f, 0.78f, 0.56f), RGB(0.80f, 0.80f, 0.62f)};
const RGB FILM_LOOKS[3] = {RGB(0.62f, 0.66f, 0.58f), RGB(0.46f, 0.40f, 0.24f), RGB(0.30f, 0.34f, 0.42f)};
}   // namespace

int overgrowthTypeOf(const CultureKind& k) {
    const CultureWorld& w = k.world;
    if (!w.overgrows || (w.bind <= 0 && w.slime <= 0)) return OG_NONE;
    const bool flagella = k.s.motion == MO_RUN_TUMBLE || k.s.motion == MO_HELICAL || k.s.motion == MO_CORKSCREW;
    if (w.bind <= 0) return OG_FILMS;
    if (w.shells() && w.shell == SHL_CHALK) return OG_CORAL;
    if (w.shells() && w.shell == SHL_GLASS) return OG_SPONGE;
    if (w.lift > 0) return OG_BLADDERS;
    if (flagella) return OG_HAIR;
    if (k.s.way == LW_LIGHT && w.dry > 0 && w.shield > 0) return OG_LICHEN;
    if (k.s.way == LW_LIGHT) return OG_CARPETS;
    if (k.s.way == LW_DEAD && k.s.sporeYears > 0) return OG_FUNGUS;
    if (k.s.way == LW_GRAZER && k.s.motion != MO_NONE) return OG_SLIME_MOULD;
    return OG_FLESH;
}

namespace {
MadeGrowth growthOf(const Seeding& s, int lineSteps, double radiusKm) {
    MadeGrowth G;
    if (!s.run) return G;
    const CultureKind& k = s.wild.kind;
    const CultureWorld& w = k.world;
    if (!w.overgrows || (w.bind <= 0 && w.slime <= 0) || s.depthKm > 0 || s.verdict.place.aloft()) return G;   // (J-13, J-14: a deep or a cloud line, no ground)
    const uint64_t h = mix64(fnv1a(s.line) ^ 0x06E0ULL);
    G.type = overgrowthTypeOf(k);   // (J-15: the editor says it too)
    const int pick = (int)(h >> 8);
    switch (G.type) {
        case OG_FUNGUS: G.colour = FUNGUS_LOOKS[pick % 4]; break;
        case OG_SLIME_MOULD: G.colour = MOULD_LOOKS[pick % 3]; break;
        case OG_LICHEN: G.colour = LICHEN_LOOKS[pick % 3]; break;
        case OG_CORAL: G.colour = CORAL_LOOKS[pick % 3]; break;
        case OG_SPONGE: G.colour = SPONGE_LOOKS[pick % 3]; break;
        case OG_HAIR: G.colour = HAIR_LOOKS[pick % 3]; break;
        case OG_BLADDERS: G.colour = BLADDER_LOOKS[pick % 3]; break;
        case OG_CARPETS: G.colour = k.s.pigment != PG_NONE ? pigmentColour(k.s.pigment) : FLESH_LOOKS[4]; break;
        case OG_FILMS: G.colour = FILM_LOOKS[pick % 3]; break;
        default: G.colour = FLESH_LOOKS[pick % 8]; break;
    }
    if (w.dyes()) G.colour = lerp(G.colour, dyeColour(w.dye), 0.7f);
    else if (k.s.pigment != PG_NONE && G.type != OG_CARPETS && G.type != OG_FILMS) G.colour = lerp(G.colour, pigmentColour(k.s.pigment), 0.35f);
    const bool clear = G.type == OG_FLESH && pick % 8 == 7;
    G.veins = clear ? RGB(0.30f, 0.08f, 0.10f) : G.colour * 0.55f;
    switch (k.s.plan) {
        case MP_SPHERE: case MP_BUDDING: case MP_ROLLER: case MP_VESICLE: case MP_DROPLET: G.texture = OGT_BLISTERED; break;
        case MP_ROD: case MP_FILAMENT: case MP_CHAIN: case MP_BRANCHING: G.texture = OGT_FIBROUS; break;
        case MP_CURVED: case MP_LAMELLAR: case MP_PLATE: case MP_SHELLED: G.texture = OGT_FOLDED; break;
        case MP_SPIRAL: case MP_CORKSCREW: G.texture = OGT_COILED; break;
        default: G.texture = (int)((h >> 20) % OGT_COUNT); break;
    }
    G.grain = clampd((std::log(std::max(0.2, k.bodyUm)) + 0.5) / 4.0, 0, 1);
    G.wet = clampd(w.slime, 0, 1);
    G.hard = w.shells() ? (w.shell == SHL_CHALK || w.shell == SHL_GLASS ? 3 : 2) : (w.dry > 0 ? 1 : 0);
    if (k.s.glows) { G.glow = 1; G.glowTint = GLOW_COLOURS[fnv1a(s.line) % 3]; }
    G.hair = G.type == OG_HAIR || k.s.motion != MO_NONE;
    G.material = madeGrowthCover(G.type, G.hard);
    G.land = s.verdict.growLand;   // (J-11)
    G.steps = clampi(lineSteps - MADE_GROWTH_START + 1, 0, MADE_GROWTH_STEPS);
    G.age = std::min(G.steps, MADE_STEPS);
    G.reach = madeReachOfStep(G.age, radiusKm);
    return G;
}

double roundTo(double v, double q);
// J-08: a harm joined to what a fate already has, as a chance: the share spared is the product of what each spares
void harmAdd(double& h, double x) { h = 1 - (1 - h) * (1 - clampd(x, 0, 1)); }
// what a fate names and keeps (each look 0 under the share its trait is named at, so what is drawn is what the data sheet says): a die-off
// past 0.15 of a class the world has, a sickness past 0.15, changed young past 0.1, thriving past 0.2 where nothing died
void fateNamed(MadeFate& f, bool land, bool sea) {
    const bool has[MFC_COUNT] = {land, true, sea};
    double most = 0;
    for (int c = 0; c < MFC_COUNT; c++) { if (!has[c] || f.harm[c] < 0.15) f.harm[c] = 0; most = std::max(most, f.harm[c]); }
    if (!land || f.shelled < 0.15) f.shelled = 0;
    if (!land || f.starve < 0.15) f.starve = 0;
    most = std::max(most, std::max(f.shelled, f.starve));
    f.fauna = 0;
    if (most > 0) f.fauna |= MF_DIE_OFF; else f.death = MD_NONE;
    if (f.sickness && f.sick >= 0.15) f.fauna |= f.sickness; else { f.sickness = 0; f.sick = 0; }
    if (f.changed >= 0.1) f.fauna |= MF_CHANGED_YOUNG; else { f.changed = 0; f.steer = 0; }
    if (!(f.fauna & MF_DYED) && !(f.steer & MSR_DYE)) f.dye = 0;
    if (f.thrive >= 0.2 && most < 0.1) f.fauna |= MF_THRIVING; else f.thrive = 0;
    for (double* v : {&f.harm[0], &f.harm[1], &f.harm[2], &f.shelled, &f.starve, &f.sick, &f.changed, &f.thrive}) *v = roundTo(*v, 0.01);
}
// J-08: what a line does to the animals where it lives (IDEAS-seeded-worlds.md sections 5 and 8), by its genes, from its second step to its
// tenth. A working poison: a nerve poison crazes them, a gut poison wastes them, a blood poison kills. A host of animals by its depth: on the
// skin its glow or its dye, else bare patches; in the gut wasting; in the blood or the cells by its other genes (bloated with a gas, growths
// where it never stops, stone skin with a shell, melting with a flesh rotter, sprouting with spores and binding cells, else it kills); in the
// cells with a carrier, or a carrier at any depth, their young born changed. A flesh rotter festers wounds (and rots remains fast), a shell
// rotter softens the shelled, a soup's or a jelly's waters starve their swimmers, a wood rotter's blight the plant eaters. Its reach: the
// waters' animals where it lives in the sea (less in fresh waters or brine), the land's where it lives on the land or in the fresh waters
// they drink, the air's half of the most of those; a host's every animal it meets
MadeFate lineFateOf(const Seeding& s, const MadeSpread& sp, const LineLooks& L, int step, bool land, bool sea) {
    MadeFate f;
    if (!s.run || step < 2) return f;
    const CultureKind& k = s.wild.kind;
    const CultureWorld& w = k.world;
    const double ramp = smoothstep(1.0, 10.0, (double)step);
    const double inWater = (sp.waters & MADE_SEA) ? 1.0 : ((sp.waters & MADE_FRESH) ? 0.5 : 0.2);
    const double onLand = sp.land ? 1.0 : ((sp.waters & MADE_FRESH) ? 0.8 : 0.3);
    const double reach[MFC_COUNT] = {onLand, 0.5 * std::max(onLand, inWater), inWater};
    double worst = 0;
    auto harm = [&](double x, int death, bool host) {   // x on every class by its reach (a host's on every one)
        for (int c = 0; c < MFC_COUNT; c++) harmAdd(f.harm[c], x * (host ? 1.0 : reach[c]));
        if (x > worst) { worst = x; f.death = death; }
    };
    auto sicken = [&](int sickness, double share) { if (share > f.sick) { f.sickness = sickness; f.sick = share; } };
    if (w.poisons()) {
        const double str = clampd(0.25 + 0.75 * w.poisonRate, 0, 1);
        if (w.poison == PZ_NERVE) { harm(0.8 * str, MD_NERVE, false); sicken(MF_CRAZED, 0.5); }
        else if (w.poison == PZ_BLOOD) harm(0.95 * str, MD_BLOOD, false);
        else { harm(0.5 * str, MD_GUT, false); sicken(MF_WASTING, 0.5); }
    }
    const bool rotsFlesh = w.rots() && w.rot == ROT_FLESH;
    if (w.hosted() && (w.host == HO_ANIMALS || w.host == HO_BOTH)) {
        const int depth = clampi((int)(w.hostRate * 4), 0, 3);
        if (depth == 0) {
            if (k.s.glows) sicken(MF_GLOWING, 0.6);
            else if (w.dyes()) { sicken(MF_DYED, 0.6); f.dye = w.dye; }
            else { sicken(MF_WASTING, 0.4); harm(0.1, MD_HOST, true); }
        } else if (depth == 1) { sicken(MF_WASTING, 0.6); harm(0.3, MD_HOST, true); }
        else if (!(depth == 3 && w.carrier > 0)) {
            if (w.gives()) { sicken(MF_BLOATED, 0.5); harm(0.35, MD_HOST, true); }
            else if (w.overgrows) { sicken(MF_GROWTHS, 0.5); harm(0.3, MD_HOST, true); }
            else if (w.shells()) { sicken(MF_STONE_SKIN, 0.5); harm(0.3, MD_HOST, true); }
            else if (rotsFlesh) { sicken(MF_MELTING, 0.6); harm(0.5, MD_ROT_FLESH, true); }
            else if (k.s.sporeYears > 0 && w.bind > 0) { sicken(MF_SPROUTING, 0.4); harm(0.5, MD_HOST, true); }
            else { sicken(MF_WASTING, 0.3); harm(0.6, MD_HOST, true); }
        }
        if (w.carrier > 0) f.changed = depth == 3 ? 0.25 + 0.5 * w.carrier : 0.15 + 0.3 * w.carrier;
    }
    if (rotsFlesh && f.sickness != MF_MELTING) harm(0.25 * w.rotRate, MD_ROT_FLESH, false);
    if (w.rots() && w.rot == ROT_SHELL) { harmAdd(f.shelled, 0.7 * w.rotRate); if (0.7 * w.rotRate > worst) { worst = 0.7 * w.rotRate; f.death = MD_ROT_SHELL; } }
    if (L.water == MWL_SOUP || L.water == MWL_JELLY) { harmAdd(f.harm[MFC_WATER], 0.75 * inWater); if (0.75 * inWater > worst) { worst = 0.75 * inWater; f.death = MD_STARVED; } }
    if (L.flora & (MP_BLIGHT | MP_ROT)) {   // the plant eaters' food lost (a rot's fallen forests most)
        const double x = (L.flora & MP_ROT) ? 0.6 : 0.45 * L.plantShare;
        harmAdd(f.starve, x);
        if (x > worst) { worst = x; f.death = MD_STARVED; }
        sicken(MF_WASTING, 0.4);
    }
    // what steers the changed young: the cell's size, the line's dye and glow, a spiral cell, a line that never stops, a flesh rotter, a shell,
    // a weak nerve poison
    if (f.changed > 0) {
        if (k.bodyUm >= 12) f.steer |= MSR_BIG;
        else if (k.bodyUm <= 1.2) f.steer |= MSR_SMALL;
        if (w.dyes()) { f.steer |= MSR_DYE; f.dye = w.dye; }
        if (k.s.glows) f.steer |= MSR_GLOW;
        if (k.s.plan == MP_SPIRAL || k.s.plan == MP_CORKSCREW) f.steer |= MSR_SPIRAL;
        if (w.overgrows) f.steer |= MSR_STOP;
        if (rotsFlesh) f.steer |= MSR_ROT;
        if (w.shells()) f.steer |= MSR_SHELL;
        if (w.poison == PZ_NERVE && w.poisonRate < 0.5) f.steer |= MSR_NERVE;
    }
    for (double& h : f.harm) h *= ramp;
    f.shelled *= ramp; f.starve *= ramp; f.sick *= ramp; f.changed *= ramp;
    fateNamed(f, land, sea);
    return f;
}

// J-04: how a world's air answers what its lines give it (IDEAS-seeded-worlds.md section 5): whether it has air, oxygen in it and methane
// (the archean's haze, a hydrocarbon world's); its seas of water, a living ground's climate (its ice and snow move), its plants, its clouds and
// whether they rain; its pressure (the HUD's), its mean warmth as generated; and three hashed values: how much a gas warms it, how much its
// seas take up, and its haze's colour
struct AirWorld {
    bool air = false, oxygen = false, methane = false, wet = false, living = false, flora = false, clouds = false, rains = false;
    bool runs = false;   // KI-458: its seas can run away (water seas, a living world's or an ocean's)
    double pressure = 0, meanC = 0, warmBy = 1, seaTake = 0;
    int hazeLook = 0;
};
AirWorld airWorldOf(const Body& b, double worldC) {
    const KindDef& K = KINDS[b.type];
    AirWorld w;
    w.air = bodyHasAir(b);
    w.oxygen = w.air && (K.shapes & KS_OXYGEN_AIR);
    w.methane = w.air && (b.type == PT_ARCHEAN || b.type == PT_HYDROCARBON);
    w.wet = (K.tags & KT_SEA) != 0;
    w.living = livingGround(b.type);
    w.runs = w.wet;   // (every world with seas has water ones: the methane and the ammonia worlds' are no sea)
    w.flora = (K.tags & KT_FLORA) != 0;
    w.clouds = w.air && (K.clouds || seedCloudDeck(b));   // (J-14: a venusian world's deck)
    w.rains = w.air && (K.rain != RAIN_NONE || (K.tags & (KT_FRONT_RAIN | KT_WATER_WEATHER)));
    w.pressure = K.pressureSpan > 0 ? K.pressureLo + K.pressureSpan * hash01(b.seed) : 0;
    w.meanC = worldC;
    w.warmBy = 0.6 + 0.9 * hash01(mix64(b.seed ^ 0xA1E0A2ULL));
    w.seaTake = hash01(mix64(b.seed ^ 0xA1E0A3ULL));
    w.hazeLook = (int)(hash01(mix64(b.seed ^ 0xA1E0A4ULL)) * 4) & 3;
    return w;
}
// what the living lines give the air at a clock: each gas's amount, and what their lift carries up (a glow, a dye, ice nuclei, spores), each
// by its gene's amount times its answer (`livedOf`), lift's by its lift too (J-14: a cloud line's whole, lift or none); the latest line's colours
struct AirSum {
    double gas[WG_COUNT] = {};
    double glow = 0, dye = 0, frost = 0, spores = 0;
    int dyeOf = WD_NONE, glowOf = 0, sporeOf = 0;
    double darkIce = 0, chalk = 0;   // J-05: the lines that darken the ice and whiten the waters, by their answer
};
// the share of the world a line lives on that gives its gas's whole answer, and the air's answer by the line's step: none before it held
// the region (step 6), all of it once it reached the whole world
constexpr double AIR_SHARE = 0.15;
// J-16 (KI-498): or of its room, where its kind of place covers less than that (a terminator world's ring of seas)
double livedOf(const Seeding& s, int step) {
    return clampd(s.verdict.share / std::min(AIR_SHARE, std::max(s.verdict.room, 1.0 / 96)), 0, 1) * smoothstep(6.0, (double)MADE_STEPS, (double)step);
}
AirSum airSumAt(const std::vector<const Seeding*>& took, double clock) {
    AirSum a;
    for (const Seeding* s : took) {
        const int step = seedingStep(*s, clock);
        if (step < 1) continue;
        const double lived = livedOf(*s, step);
        const CultureKind& k = s->wild.kind;
        if (k.world.gives()) a.gas[clampi(k.world.gas, 0, WG_COUNT - 1)] += k.world.gasRate * lived;
        {   // J-05: a dark ice warms the world (its albedo), a milk sea cools it
            const LineLooks L = looksOf(*s, spreadOf(*s));
            if (L.snow.kind == MGT_DARK_ICE) a.darkIce += lived;
            if (L.water == MWL_MILK) a.chalk += lived;
        }
        const double up = (s->verdict.place.aloft() ? 1.0 : k.world.lift) * lived;   // (J-14: a cloud line's all up there)
        if (up <= 0) continue;
        const uint64_t h = fnv1a(s->line);
        if (k.s.glows) { a.glow = std::max(a.glow, up); a.glowOf = (int)(h % 3); }   // (its colour its waters' glow's)
        if (k.world.dyes()) { a.dye = std::max(a.dye, up * (0.3 + 0.3 * clampd(k.world.dyeRate, 0, 1))); a.dyeOf = k.world.dye; }
        if (k.world.frostC > FROST_NONE + 1) a.frost = std::max(a.frost, up * clampd((k.world.frostC + 12) / 12, 0.25, 1));
        if (k.s.sporeYears > 0) { a.spores = std::max(a.spores, up); a.sporeOf = (int)(mix64(h ^ 0x5B0E5ULL) % 3); }
    }
    for (double& g : a.gas) g = std::min(g, 1.5);
    a.darkIce = std::min(a.darkIce, 1.0); a.chalk = std::min(a.chalk, 1.0);
    return a;
}
// the greenhouse's warming of the gases (C): CO2 (less where seas take it up), methane (a little where oxygen burns it), ammonia (more on a
// cold world), hydrogen (in a thick air only: a thin one loses it), nitrous oxide; by how much a gas warms the world
double greenhouseOf(const AirWorld& w, const AirSum& a) {
    if (!w.air) return 0;
    double t = 20 * a.gas[WG_CO2] * (w.wet ? 1 - 0.4 * w.seaTake : 1.0);
    t += (w.oxygen ? 3 : 24) * a.gas[WG_METHANE];
    t += (w.meanC < 5 ? 14 : 5) * a.gas[WG_AMMONIA];
    if (w.pressure >= 1) t += 12 * a.gas[WG_HYDROGEN] * std::min(1.0, w.pressure / 3);
    t += 4 * a.gas[WG_N2O];
    return t * w.warmBy;
}
// the hazes: methane's orange where no oxygen burns it, as it thickens; dimethyl sulphide's pale one
double orangeHazeOf(const AirWorld& w, const AirSum& a) { return w.air && !w.oxygen ? smoothstep(0.35, 0.9, a.gas[WG_METHANE]) : 0.0; }
double coldHazeOf(const AirWorld& w, const AirSum& a) { return w.air ? smoothstep(0.2, 0.7, a.gas[WG_DMS]) : 0.0; }
// the warming a world's air now gives it (C), but a runaway's: the greenhouse, less the hazes' cooling and the methane an oxygen-maker takes
// out of a methane air
double airWarmthOf(const AirWorld& w, const AirSum& a) {
    if (!w.air) return 0;
    double t = greenhouseOf(w, a) - (18 * orangeHazeOf(w, a) + 8 * coldHazeOf(w, a)) * w.warmBy;
    if (w.methane && !w.oxygen) t -= 10 * a.gas[WG_OXYGEN] * w.warmBy;
    if (a.darkIce > 0 || a.chalk > 0) t += (1.5 * a.darkIce - 1.5 * a.chalk) * w.warmBy;   // J-05: a dark ice's sunlight kept, a milk sea's thrown back
    return t;
}
// the haze's colours (a methane haze tan, orange, brown or olive by the world; a cold haze pale), the spores' (gold, rust or white by the
// line); a glowing cloud's light is its line's in the waters (`GLOW_COLOURS`)
const RGB HAZE_LOOKS[4] = {RGB(0.74f, 0.62f, 0.44f), RGB(0.86f, 0.54f, 0.24f), RGB(0.56f, 0.40f, 0.26f), RGB(0.60f, 0.58f, 0.32f)};
const RGB COLD_HAZE(0.80f, 0.82f, 0.84f);
const RGB SPORE_LOOKS[3] = {RGB(0.86f, 0.72f, 0.30f), RGB(0.66f, 0.34f, 0.18f), RGB(0.90f, 0.90f, 0.86f)};
constexpr double RUNAWAY_C = 42;    // a wet world's mean past this runs away (about Kasting 1988's moist greenhouse)
constexpr int RUNAWAY_STEAM = 6;    // its seas steam for six steps, then are gone
// J-11: a wet world's mean cooled under this from over it ices to its equator (Budyko 1969's ice-albedo line: the caps' white throws back the
// sun that warmed it), the ice running for six steps and its white cooling the world by up to 20 C more, then frozen from pole to pole
constexpr double SNOWBALL_C = -6;
constexpr int SNOWBALL_RUN = 6;
constexpr double SNOWBALL_COOLS = 20;
double roundTo(double v, double q) { return std::round(v / q) * q; }
// J-11: how much of a warming's heat a world's plants and animals feel: none while its mean stays under 5 C, all of it from 15 C; and of a
// cooling's cold: none while its mean stays over 15 C, all of it from 5 C (KI-486: a hot world cooled into the temperate is spared)
double heatFelt(const AirWorld& w, double warmC) { return smoothstep(5.0, 15.0, w.meanC + warmC); }
double coldFelt(const AirWorld& w, double warmC) { return smoothstep(15.0, 5.0, w.meanC + warmC); }
// J-04, J-11: the steps since the world's mean passed a line, walked over every line's step boundaries in order (its warmth as `airWarmthOf`
// reads it, without the steam's or the ice's own), at the clock's own pace; -1 where none is passed at the clock. KI-485: a runaway's from the
// first boundary its mean passed `RUNAWAY_C`, whatever comes after (its steam keeps it going, and seas boiled off are gone); the ice's from the
// first its mean fell under `SNOWBALL_C`, until one at which the gases warm it past the ice's own white too (its cooling so far) and it thaws
int stepsPast(const AirWorld& w, const std::vector<const Seeding*>& took, double clock, bool hot) {
    std::vector<double> marks;
    for (const Seeding* s : took) {
        const double pace = s->verdict.pace > 0 ? s->verdict.pace : 1.0;
        for (int j = 1; j <= MADE_STEPS; j++) { const double c = s->clock + j * MADE_STEP_S / pace + 1e-6; if (c <= clock) marks.push_back(c); }
    }
    std::sort(marks.begin(), marks.end());
    double onset = 0;
    bool past = false;   // (a clock can be under 0: a line let go before the guide's clock began)
    for (double c : marks) {
        const double mean = w.meanC + airWarmthOf(w, airSumAt(took, c));
        if (hot) { if (mean >= RUNAWAY_C) { onset = c; past = true; break; } continue; }
        if (!past) { if (mean <= SNOWBALL_C) { onset = c; past = true; } continue; }
        const double ran = std::min(1.0, (std::floor((c - onset) / MADE_STEP_S + 1e-9) + 1) / SNOWBALL_RUN);   // (the ice's share of its run by then)
        if (mean >= SNOWBALL_C + SNOWBALL_COOLS * w.warmBy * ran) past = false;
    }
    return past ? (int)std::floor((clock - onset) / MADE_STEP_S + 1e-9) : -1;
}

// the layer's air at the clock from the lines that took (in their order): the warming and the pressure, the hazes, the horizon, the ozone, a
// runaway (from the first step its world's mean passed RUNAWAY_C), the plants' health, what the lift carries up; each look zero under the
// share its trait is named at
void madeAirOf(const Body& b, const std::vector<const Seeding*>& took, double clock, MadeWorld& m) {
    if (took.empty()) return;
    const AirWorld w = airWorldOf(b, took[0]->verdict.worldC);
    if (!w.air) return;   // a gas leaves an airless world, and nothing is lifted
    const AirSum a = airSumAt(took, clock);
    const double green = greenhouseOf(w, a), orange = orangeHazeOf(w, a), cold = coldHazeOf(w, a);
    double warm = airWarmthOf(w, a);
    double dP = 0.5 * a.gas[WG_CO2] + 0.1 * a.gas[WG_METHANE] + 0.1 * a.gas[WG_AMMONIA] + (w.pressure >= 1 ? 0.2 * a.gas[WG_HYDROGEN] : 0.0);
    // a runaway: the first step boundary of any line at which the world's mean passed RUNAWAY_C, its steps since (at the clock's own pace);
    // KI-458: any world with seas, a living one's or an ocean's; one already over it as generated keeps its seas (a dark ocean's under a deep
    // air: they are what it is), as the ice's run leaves one already under its line
    const int hotSteps = w.runs && w.meanC < RUNAWAY_C ? stepsPast(w, took, clock, true) : -1;
    if (hotSteps >= 0) {
        const int steps = hotSteps;
        m.runaway = steps < RUNAWAY_STEAM ? (steps + 1) / (double)RUNAWAY_STEAM : 2.0;
        warm += 25 * std::min(1.0, m.runaway);   // the steam's own greenhouse
        dP += 1.5 * std::min(1.0, m.runaway);
        m.air |= MA_RUNAWAY;
    }
    // J-11: the ice's run on a living wet world its lines cooled under SNOWBALL_C (one already under it as generated keeps its own climate)
    const int coldSteps = w.living && w.wet && w.meanC > SNOWBALL_C && m.runaway <= 0 ? stepsPast(w, took, clock, false) : -1;
    if (coldSteps >= 0) {
        const int steps = coldSteps;
        m.freeze = steps < SNOWBALL_RUN ? (steps + 1) / (double)SNOWBALL_RUN : 2.0;
        warm -= SNOWBALL_COOLS * std::min(1.0, m.freeze) * w.warmBy;   // the ice's white
        m.air |= MA_FREEZING;
    }
    const bool iced = m.frozen();   // (its seas ice: no fog lies on them, nothing colours them)
    m.warmC = std::fabs(warm) < 0.5 ? 0.0 : roundTo(warm, 0.1);
    m.worldC = roundTo(w.meanC, 0.1);
    m.pressureK = roundTo(1 + dP, 0.01);
    if (green >= 4 || dP >= 0.12) m.air |= MA_THICK;
    m.thick = (m.air & MA_THICK) ? roundTo(std::max(clampd(green / 20, 0, 1), std::min(1.0, m.runaway)), 0.01) : 0.0;
    if (orange >= 0.2) m.air |= MA_ORANGE_HAZE;
    if (cold >= 0.2) m.air |= MA_COLD_HAZE;
    const double ho = (m.air & MA_ORANGE_HAZE) ? orange : 0.0, hc = (m.air & MA_COLD_HAZE) ? 0.75 * cold : 0.0;
    m.haze = roundTo(std::max(ho, hc), 0.01);
    if (m.haze > 0) m.hazeTint = ho >= hc ? HAZE_LOOKS[w.hazeLook] : COLD_HAZE;
    const double sulphur = smoothstep(0.15, 0.6, a.gas[WG_H2S]);
    if (sulphur >= 0.2) { m.air |= MA_SULPHUR; m.sulphur = roundTo(sulphur, 0.01); }
    // KI-483: methane an oxygen air burns turns to water high up, frozen into silver clouds in the summer twilights (Earth's have spread with
    // its methane since the nineteenth century)
    if (w.oxygen) { const double sv = smoothstep(0.15, 0.6, a.gas[WG_METHANE]); if (sv >= 0.2) { m.air |= MA_SILVER_CLOUDS; m.silver = roundTo(sv, 0.01); } }
    if (!w.oxygen && a.gas[WG_OXYGEN] >= 0.3) { m.air |= MA_OZONE; m.ozone = 1; }
    else if (w.oxygen && a.gas[WG_N2O] + a.gas[WG_H2S] >= 0.3) { m.air |= MA_THIN_OZONE; m.ozone = -1; }
    // (J-11: by how warm the world now is too: a cold world warmed into the temperate is greened, not scorched)
    if (w.living && w.flora) m.wilt = roundTo(std::max(smoothstep(5.0, 18.0, m.warmC) * heatFelt(w, m.warmC), m.runaway >= 1 ? 1.0 : 0.0), 0.01);
    // a fog on the seas: dimethyl sulphide's smoke, a runaway's steam while its seas last
    const double smoke = w.wet && !iced ? smoothstep(0.1, 0.5, a.gas[WG_DMS]) : 0.0;
    if (smoke >= 0.2) { m.air |= MA_SEA_SMOKE; m.seaSmoke = roundTo(smoke, 0.01); }
    if (m.runaway > 0 && m.runaway <= 1) m.seaSmoke = std::max(m.seaSmoke, roundTo(m.runaway, 0.01));
    if (m.runaway > 0) m.cloudMore = roundTo(0.5 * std::min(1.0, m.runaway), 0.01);
    // what the lift carries up: a glow into the clouds (its form by how much: patches, veins, whole decks), a dye into the clouds and the
    // rain, ice nuclei that make every cloud rain (snow on a cold world), spores
    if (w.clouds && a.glow >= 0.1) {
        m.air |= MA_GLOW_CLOUDS; m.cloudGlow = roundTo(a.glow, 0.01); m.cloudGlowTint = GLOW_COLOURS[a.glowOf];
        m.cloudGlowForm = a.glow < 0.35 ? MG_PATCHES : (a.glow < 0.7 ? MG_VEINS : MG_DECKS);
    }
    if (w.clouds && a.dye >= 0.1) { m.air |= MA_DYE_CLOUDS; m.cloudTintK = roundTo(a.dye, 0.01); m.cloudTint = dyeColour(a.dyeOf); m.cloudDye = a.dyeOf; }
    if (w.rains && a.dye >= 0.1) { m.air |= MA_DYE_RAIN; m.rainTintK = roundTo(std::min(1.0, 1.4 * a.dye), 0.01); m.rainTint = dyeColour(a.dyeOf); m.rainDye = a.dyeOf; }
    if ((w.clouds || w.rains) && a.frost >= 0.15) {
        m.air |= MA_ENDLESS_RAIN; m.rainMore = roundTo(a.frost, 0.01); m.cloudMore = std::max(m.cloudMore, roundTo(0.6 * a.frost, 0.01));
        if (w.living) m.snowMore = roundTo(a.frost * smoothstep(6.0, -2.0, w.meanC + m.warmC), 0.01);
    }
    if (a.spores >= 0.1) {
        m.air |= MA_SPORE_CLOUDS; m.spores = roundTo(a.spores, 0.01); m.sporeTint = SPORE_LOOKS[a.sporeOf];
        if (w.clouds && !(m.air & MA_DYE_CLOUDS)) { m.cloudTintK = roundTo(0.5 * a.spores, 0.01); m.cloudTint = m.sporeTint; }
    }
    // J-05: what the air does to the waters, the ground and the plants: a sulphide turns every water purple (black where no sun feeds the purple
    // bacteria), a sour sea bleaches the reefs, an oxygen rusts an oxygenless world's ground; the plants lush under a new ozone or an endless
    // rain, bleached under a thin ozone; a runaway's seas boil while they steam
    const bool waterSeas = w.wet && b.type != PT_HYDROCARBON && b.type != PT_AMMONIA && !iced;
    if (waterSeas) {
        const double sh = smoothstep(0.3, 0.9, a.gas[WG_H2S]);
        if (sh >= 0.2) { m.seas |= m.sunless ? MS_BLACK : MS_PURPLE; m.seaShade = roundTo(sh, 0.01); m.seaShadeTint = m.sunless ? BLACK_SEA : PURPLE_SEA; }
        const double sour = took[0]->verdict.reefs ? smoothstep(0.3, 0.9, a.gas[WG_CO2]) : 0.0;
        if (sour >= 0.2) { m.seas |= MS_SOUR; m.sour = roundTo(sour, 0.01); }
    }
    if (!w.oxygen) {
        const double rust = smoothstep(0.3, 1.0, a.gas[WG_OXYGEN]);
        if (rust >= 0.2) { m.grounds |= MGT_RUST; m.rustGround = roundTo(rust, 0.01); }
    }
    if (w.living && w.flora) {
        const double lush = std::max(m.ozone > 0 ? smoothstep(0.3, 1.0, a.gas[WG_OXYGEN]) : 0.0, 0.7 * m.rainMore);
        if (lush >= 0.2) { m.flora |= MP_LUSH; m.lush = roundTo(lush, 0.01); }
        const double bleach = m.ozone < 0 ? smoothstep(0.3, 0.9, a.gas[WG_N2O] + a.gas[WG_H2S]) : 0.0;
        if (bleach >= 0.2) { m.flora |= MP_BLEACHED; m.bleached = roundTo(bleach, 0.01); }
    }
    if (m.runaway > 0 && m.runaway <= 1 && waterSeas) { m.seas |= MS_BOILING; m.boil = roundTo(m.runaway, 0.01); }
}

// J-08: what the made air does to the animals over the whole world (IDEAS-seeded-worlds.md sections 5 and 8): a sulphide air kills them (its
// purple seas their swimmers first: the end-Permian's proposed killer), a thinned ozone changes their young and kills a few, the heat past 6 C
// warmer kills (a runaway every one), the cold past 4 C colder, an ammonia sea its swimmers; a new ozone or an endless rain over the plants
// (the lush) and an oxygen air made richer make them thrive. Silence where every class died to the last. Then every trait named, the steps
// since the first fate came (a line's from its second step, the air's from its sixth: the remains' age) and a flesh rotter's fast rotting
void madeFaunaOf(const Body& b, const std::vector<const Seeding*>& took, double clock, const int* lineSteps, MadeWorld& m) {
    if (took.empty() || !kindHas(b.type, KT_FAUNA)) return;
    const bool land = kindHas(b.type, KT_FLORA), sea = kindHas(b.type, KT_SEA);
    MadeFate f;
    const AirWorld w = airWorldOf(b, took[0]->verdict.worldC);
    if (w.air) {
        const AirSum a = airSumAt(took, clock);
        double worst = 0;
        auto harm = [&](int c, double x, int death) { if (x <= 0) return; harmAdd(f.harm[c], x); if (x > worst) { worst = x; f.death = death; } };
        const double sul = smoothstep(0.15, 0.8, a.gas[WG_H2S]);
        harm(MFC_LAND, 0.9 * sul, MD_SULPHIDE); harm(MFC_AIR, 0.9 * sul, MD_SULPHIDE); harm(MFC_WATER, smoothstep(0.1, 0.6, a.gas[WG_H2S]), MD_SULPHIDE);
        if (m.ozone < 0) {
            const double thin = smoothstep(0.3, 0.9, a.gas[WG_N2O] + a.gas[WG_H2S]);
            f.changed = 0.4 * thin;
            for (int c = 0; c < MFC_COUNT; c++) harm(c, 0.1 * thin, MD_THIN_OZONE);
        }
        const double heat = m.runaway >= 1 ? 1.0 : smoothstep(6.0, 22.0, m.warmC) * heatFelt(w, m.warmC);
        harm(MFC_LAND, heat, MD_HEAT); harm(MFC_AIR, heat, MD_HEAT); harm(MFC_WATER, m.dried() ? 1.0 : 0.8 * heat, MD_HEAT);
        const double cold = smoothstep(4.0, 16.0, -m.warmC) * coldFelt(w, m.warmC);
        harm(MFC_LAND, cold, MD_COLD); harm(MFC_AIR, cold, MD_COLD); harm(MFC_WATER, 0.6 * cold, MD_COLD);
        if (w.wet && b.type != PT_HYDROCARBON && b.type != PT_AMMONIA) harm(MFC_WATER, smoothstep(0.2, 0.8, a.gas[WG_AMMONIA]), MD_AMMONIA);
        f.thrive = std::max(m.lush, w.oxygen ? smoothstep(0.3, 1.0, a.gas[WG_OXYGEN]) : 0.0);
    }
    fateNamed(f, land, sea);
    {   // silence: every class dead to the last over the whole world, of the air's harm and the lines' that reached all of it
        double h[MFC_COUNT];
        for (int c = 0; c < MFC_COUNT; c++) h[c] = f.harm[c];
        for (int i = 0; i < m.lines && i < MADE_LINES; i++)
            if (m.spread[i].reach >= PI) for (int c = 0; c < MFC_COUNT; c++) harmAdd(h[c], m.spread[i].fate.harm[c]);
        if (h[MFC_AIR] >= 0.95 && (!land || h[MFC_LAND] >= 0.95) && (!sea || h[MFC_WATER] >= 0.95)) f.fauna |= MF_SILENCE | MF_DIE_OFF;
    }
    m.fate = f;
    m.fauna = f.fauna;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) m.fauna |= m.spread[i].fate.fauna;
    if (m.fauna & MF_DIE_OFF) {   // no thriving named where any die-off is: what thrives is the share spared
        m.fauna &= ~MF_THRIVING; m.fate.fauna &= ~MF_THRIVING; m.fate.thrive = 0;
        for (int i = 0; i < m.lines && i < MADE_LINES; i++) { m.spread[i].fate.fauna &= ~MF_THRIVING; m.spread[i].fate.thrive = 0; }
    }
    int age = -1;
    int airAge = -1;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        if (m.spread[i].fate.on()) age = std::max(age, lineSteps[i] - 2);
        if (f.on()) airAge = std::max(airAge, lineSteps[i] - 6);
    }
    age = std::max(age, airAge);
    if (m.fate.on()) m.fate.age = clampi(airAge, 0, MADE_FATE_STEPS);
    for (const Seeding* s : took) {
        const CultureWorld& cw = s->wild.kind.world;
        if (cw.rots() && cw.rot == ROT_FLESH && seedingStep(*s, clock) >= 2) m.rotFast = true;
    }
    m.fateAge = m.fauna ? clampi(age, 0, MADE_FATE_STEPS) : 0;
    if (!m.fauna) m.rotFast = false;
}
}   // namespace

namespace {
// (the layer's harm read before its key is set: `madeKindOf`'s)
// J-15: an overgrowth harms, but films by the waters and a dry world's carpet that grows grass and shrubs (it smothers no plants or animals)
bool harmsOf(const MadeWorld& m) {
    if (m.fauna & (MF_DIE_OFF | MF_SILENCE | MF_CHANGED_YOUNG | (MF_SICKNESSES & ~(MF_GLOWING | MF_DYED)))) return true;
    if ((m.flora & (MP_BLIGHT | MP_ROT | MP_FROST_BURN | MP_BLEACHED | MP_GALLS)) || m.wilt >= 0.2) return true;
    if ((m.seas & (MS_SOUP | MS_JELLY | MS_SOUR | MS_PURPLE | MS_BLACK | MS_BOILING)) || (m.air & (MA_RUNAWAY | MA_FREEZING | MA_THIN_OZONE))) return true;
    for (int i = 0; i < m.lines && i < MADE_LINES; i++) {
        const MadeGrowth& G = m.spread[i].growth;
        if (G.on() && G.type != OG_FILMS && !G.greens) return true;
    }
    return false;
}
}   // namespace
bool madeHarms(const MadeWorld& m) { return m.on() && harmsOf(m); }
// KI-483: and a world its made climate moved 5 C or more toward a temperate 15 C, where nothing was harmed (a hot world cooled, a cold one warmed)
bool madeLivable(const MadeWorld& m) {
    const bool milder = m.warmC != 0 && std::fabs(m.worldC + m.warmC - 15) <= std::fabs(m.worldC - 15) - 5;
    return m.on() && !madeHarms(m) && ((m.fauna & MF_THRIVING) || (m.flora & MP_LUSH) || (m.air & MA_OZONE) || m.kind == MDK_GREENED || m.kind == MDK_GREENED_DESERT ||
                                       milder);
}

namespace {
// J-11: a world's made kind (IDEAS-seeded-worlds.md section 10), the first whose line it passed: its seas dried by a runaway (MADE HOTHOUSE), its
// ice run to the equator (MADE SNOWBALL), an overgrowth over the whole world and on half its land at least (KIND_GROWN; the latest line's that
// has; films lie by the waters alone), every
// animal gone and the plants with them or none (CHARNEL WORLD), purple seas under a sulphur air, an oxygenless ground rusted under its new ozone,
// a thick orange haze (each look past KIND_LINE), a cold world (its mean under 5 C as generated) made 3 C warmer or a dry one (a steppe's grass
// band) rained on, where nothing was harmed (GREENED WORLD)
constexpr double KIND_LINE = 0.6, KIND_GROWN = 0.5;
void madeKindOf(const Body& b, const std::vector<const Seeding*>& took, double clock, MadeWorld& m) {
    m.kind = MDK_NONE; m.kindGrowth = OG_NONE; m.kindCold = false; m.kindFlora = kindHas(b.type, KT_FLORA);
    if (took.empty()) return;
    if (m.dried()) { m.kind = MDK_HOTHOUSE; return; }
    if (m.frozen()) { m.kind = MDK_SNOWBALL; return; }
    for (int i = std::min(m.lines, MADE_LINES) - 1; i >= 0; i--) {
        const MadeGrowth& G = m.spread[i].growth;
        if (G.on() && G.greens && m.green >= KIND_GROWN) { m.kind = MDK_GREENED_DESERT; m.kindGrowth = G.type; return; }   // J-15: its grass and shrubs over half its land
        if (G.on() && G.type != OG_FILMS && G.reach >= PI && G.land >= KIND_GROWN) { m.kind = MDK_OVERGROWN; m.kindGrowth = G.type; return; }
    }
    const bool flora = kindHas(b.type, KT_FLORA);
    if (m.fauna & MF_SILENCE) {
        bool rotsAll = false;
        for (int i = 0; i < m.lines && i < MADE_LINES; i++) rotsAll = rotsAll || (m.spread[i].plants && m.spread[i].reach >= PI);
        const bool felled = rotsAll && (((m.flora & MP_ROT) && m.rot >= KIND_LINE) || ((m.flora & MP_BLIGHT) && m.plant.kind == MP_BLIGHT && m.plant.variant == 2));
        if (!flora || felled || m.wilt >= 0.8) { m.kind = MDK_CHARNEL; return; }
    }
    if ((m.seas & MS_PURPLE) && m.seaShade >= KIND_LINE && (m.air & MA_SULPHUR) && m.sulphur >= KIND_LINE) { m.kind = MDK_PURPLE; return; }
    if ((m.grounds & MGT_RUST) && m.rustGround >= KIND_LINE && (m.air & MA_OZONE)) { m.kind = MDK_RUST; return; }
    const AirWorld w = airWorldOf(b, took[0]->verdict.worldC);
    if ((m.air & MA_ORANGE_HAZE) && orangeHazeOf(w, airSumAt(took, clock)) >= KIND_LINE) { m.kind = MDK_HAZE; return; }
    if (livingGround(b.type) && flora && !harmsOf(m)) {
        const bool cold = w.meanC < 10 && m.warmC >= 2, dry = KINDS[b.type].grassBand > 0 && m.rainMore >= 0.4;   // (KI-483: a cool world's too)
        if (cold || dry) {   // its plants spreading (lush), its animals thriving on the new grazing (section 8)
            m.kind = MDK_GREENED; m.kindCold = cold;
            m.flora |= MP_LUSH; m.lush = std::max(m.lush, 0.5);
            if (kindHas(b.type, KT_FAUNA)) { m.fate.thrive = std::max(m.fate.thrive, 0.3); m.fate.fauna |= MF_THRIVING; m.fauna |= MF_THRIVING; }
        }
    }
}
}   // namespace

double deepGradientC(const StarSystem& sys, int body) {
    if (body < 0 || body >= (int)sys.bodies.size()) return 0;
    const Body& b = sys.bodies[body];
    double c = 30.0 * b.radiusKm / 6371.0;   // a world's heat flow goes as its radius (its radioactive rock's), Earth's crust 30 C a kilometre
    if (kindHas(b.type, KT_PLATES) || kindHas(b.type, KT_VENT_LIFE)) c *= 1.5;   // a hot inside: plates, vents
    if (b.parent >= 0 && b.parent < (int)sys.bodies.size() && kindHas(sys.bodies[b.parent].type, KT_GIANT)) c *= 1.5;   // a giant's tides
    return c;
}
double deepestKm(const StarSystem& sys, int body) {
    if (body < 0 || body >= (int)sys.bodies.size()) return SEED_DEPTH_KM[SEED_DEPTHS - 1];
    const double grad = deepGradientC(sys, body);
    if (grad <= 0) return SEED_DEPTH_KM[SEED_DEPTHS - 1];
    return clampd((10 - biotaMeanC(sys, sys.bodies[body])) / grad, SEED_DEPTH_KM[SEED_DEPTHS - 1], 120);
}
double iceShellKm(const Body& b) {
    if (!BIOTA[b.type].underIce) return 0;
    const double u = hash01(mix64(b.seed ^ 0x1CE5E11ULL));
    return std::round(b.type == PT_TIGER_STRIPED ? 2 + 6 * u : 3 + 22 * u);   // Enceladus's a few kilometres at its south pole, Europa's tens
}
std::vector<double> seedDepthsOf(const StarSystem& sys, int body) {
    std::vector<double> d = {0};
    if (body < 0 || body >= (int)sys.bodies.size()) return d;
    const double shell = iceShellKm(sys.bodies[body]);
    if (shell > 0) d.push_back(shell);
    else {
        const double stretch = deepestKm(sys, body) / SEED_DEPTH_KM[SEED_DEPTHS - 1];   // (J-16: 1 on an Earth)
        for (double km : SEED_DEPTH_KM) d.push_back(std::max(1.0, std::round(km * stretch)));
    }
    return d;
}
bool seedCloudDeck(const Body& b) { return !BIOTA[b.type].furnace && bodyHasAir(b) && (BIOTA[b.type].habitats & nicheBit(NI_CLOUD)); }
std::vector<double> seedHeightsOf(const StarSystem& sys, int body) {
    std::vector<double> h;
    if (body < 0 || body >= (int)sys.bodies.size() || !seedCloudDeck(sys.bodies[body])) return h;
    const Body& b = sys.bodies[body];
    const double groundC = placeMeanC(BodyGen::make(b), b.tempK, sys.sunless(), StarSystem::bodyFromLatLon(0, 0), 0);   // (as generated)
    for (double c : CLOUD_LEVEL_C) {
        const double km = std::round((groundC - c) / CLOUD_LAPSE_C);
        if (km >= CLOUD_BASE_KM && (h.empty() || km > h.back())) h.push_back(km);
    }
    if (h.empty()) h.push_back(CLOUD_BASE_KM);   // (a cool world's: the clouds' base alone, as the scoop's)
    return h;
}

SeedPlace seedPlaceOf(const StarSystem& sys, int body, const BodyGen& g, const Biota& B, double lat, double lon, double depthKm) {
    SeedPlace P;
    const Body& b = sys.bodies[body];
    P.unit = StarSystem::bodyFromLatLon(lat, lon);
    const SurfaceSample s = sampleSurface(g, P.unit, SEED_PLACE_M);
    Microhabitat& H = P.hab;
    H.place = placeKey(b.seed, P.unit, b.radiusKm);
    H.oldRock = B.second == LS_FOSSIL || B.stage == LS_FOSSIL;
    const int chem = B.chemistry != CH_NONE ? B.chemistry : BIOTA[b.type].chemistry;
    const bool wet = wetAt(s);
    P.heightM = wet ? s.water : s.height;
    const double meanC = placeMeanC(g, b.tempK, sys.sunless(), P.unit, P.heightM) + g.made.warmC;   // (KI-457: the warmth the lines before it made)
    auto put = [&](int where, int niche, const char* name, double c, bool dry) { P.where = where; H.niche = niche; P.name = name; H.meanC = c; P.dry = dry; };
    if (depthKm < 0 && seedCloudDeck(b)) {   // J-14: up in the clouds: the ground's warmth at its level, 8 C colder a kilometre up; the cloud's own layer (the scoop's)
        char name[48];
        std::snprintf(name, sizeof name, "THE CLOUDS %.0f KM UP", -depthKm);
        put(AT_CLOUDS, NI_CLOUD, "", placeMeanC(g, b.tempK, sys.sunless(), P.unit, 0) + g.made.warmC + CLOUD_LAPSE_C * depthKm, false);
        H.place = placeKey(b.seed ^ 0xC10D5C00ULL, P.unit, b.radiusKm - depthKm);
        H.salt = 0; P.depthKm = depthKm; P.name = name;
        H.meanC = std::round(H.meanC * 10) / 10;
        return P;
    }
    if (depthKm > 0 && s.material != MAT_LAVA) {   // J-13: down a drill hole, its water the world's inner warmth keeps, in the dark
        char name[48];
        H.shade = true;
        const double shell = iceShellKm(b);
        if (shell > 0) {   // a sea under the ice, at the ice's foot: a few degrees over its freezing (Europa's: Melosh et al. 2004), salt, a brine dwarf's brine
            std::snprintf(name, sizeof name, "THE SEA %.0f KM UNDER THE ICE", shell);
            put(AT_UNDER_ICE, NI_PLUME, "", 1 + 3 * hash01(mix64(b.seed ^ 0x5EA0ULL)), false);
            H.salt = b.type == PT_BRINE_DWARF ? 2 : 1; P.depthKm = shell;
        } else {   // the rock's pores, their water sea-salt (the deep crust's fluids: Lollar et al. 2014), never colder than the ground's own reading
            std::snprintf(name, sizeof name, "ROCK %.0f KM DOWN", depthKm);
            const SeedPlace top = seedPlaceOf(sys, body, g, B, lat, lon);
            put(AT_DEEP, NI_ROCK, "", std::max(top.hab.meanC, meanC + deepGradientC(sys, body) * depthKm), false);
            H.salt = 1; P.depthKm = depthKm;
        }
        P.name = name;
        if (H.meanC < WATER_COLDEST_C) H.salt = 2;   // (colder than any water but a brine stays liquid)
        H.meanC = std::round(H.meanC * 10) / 10;
        return P;
    }
    if (s.material == MAT_LAVA) put(AT_MOLTEN, NI_DUST, "MOLTEN ROCK", meanC, true);
    else if (wet) {   // the liquid: its salt and its warmth as a vial's (never under its freezing)
        H.salt = g.hasTrait(TR_EXOTIC_SEAS) && chem == CH_WATER ? 2 : (s.waterKind == 0 ? 1 : 0);
        if (g.hasTrait(TR_PINK_LAKES) && s.waterTint > 0.5) H.salt = 2;
        const double freezeC = chem == CH_HYDROCARBON ? -183 : (chem == CH_AMMONIA ? -97 : (H.salt >= 1 ? -2 : 0));
        put(s.waterKind == 0 ? AT_SEA : AT_LAKE, NI_LIQUID, s.waterKind == 0 ? "THE SEA" : "A LAKE", std::max(meanC, freezeC), false);
    } else {
        // the sea's edge: the sea within three kilometres of a shore's ground
        bool shore = false;
        if (s.material == MAT_SAND || s.material == MAT_MUD || s.material == MAT_DUST || s.material == MAT_GRASS || s.material == MAT_ROCK) {
            const double d = 3.0 / std::max(1.0, b.radiusKm);
            const double dl[4][2] = {{d, 0}, {-d, 0}, {0, d}, {0, -d}};
            for (int k = 0; k < 4 && !shore; k++) {
                const SurfaceSample n = sampleSurface(g, StarSystem::bodyFromLatLon(clampd(lat + dl[k][0], -PI / 2, PI / 2), lon + dl[k][1] / std::max(0.05, std::cos(lat))), SEED_PLACE_M);
                shore = wetAt(n) && n.waterKind == 0;
            }
        }
        if (shore) { H.salt = 1; put(AT_SHORE, NI_SILT, "THE SEA SHORE", meanC, false); }
        else switch (s.material) {   // the ground's own, as the sampling names it
            case MAT_SNOW: H.bloom = s.snowTint > 0.5; put(AT_SNOW, NI_SNOW, H.bloom ? "RED SNOW" : "SNOW", clampd(meanC + 8, -40, 0), false); break;
            case MAT_ICE: put(AT_ICE, NI_ICE, "ICE", clampd(meanC + 8, -40, 0), false); break;
            case MAT_SALT: case MAT_POTASH: H.salt = 2; put(AT_SALT, NI_SALT, "A SALT CRUST", meanC, true); break;
            case MAT_BRINE: H.salt = 2; put(AT_SALT, NI_SALT, "A BRINE POOL", std::max(meanC, -10.0), false); break;
            case MAT_MATS: H.bloom = true; put(AT_LAND, NI_MAT, "MATS ON A WET FLAT", meanC, false); break;
            case MAT_STROMATOLITE: put(AT_LAND, NI_MAT, "A STROMATOLITE FLAT", meanC, false); break;
            case MAT_CRUST: put(AT_LAND, NI_CRUST, "A CRUST ON STONE", meanC, true); break;
            case MAT_MOSS: put(AT_LAND, NI_CRUST, "MOSS", meanC, true); break;
            case MAT_FOREST: H.shade = true; put(AT_LAND, NI_SOIL, "SOIL UNDER TREES", meanC, true); break;
            case MAT_GRASS: put(AT_LAND, NI_SOIL, "SOIL UNDER GRASS", meanC, true); break;
            case MAT_MUD: put(AT_LAND, NI_SILT, "MUD", meanC, false); break;
            case MAT_SAND: put(AT_LAND, B.habitats & nicheBit(NI_SOIL) ? NI_SOIL : NI_DUST, "SAND", meanC, true); break;
            case MAT_SULPHUR: put(AT_LAND, NI_DUST, "SULPHUR", meanC, true); break;
            case MAT_DUST: put(AT_LAND, NI_DUST, g.hasAir ? "DUST" : "REGOLITH", meanC, true); break;
            default:
                if (b.type == PT_QUARTZ && s.material == MAT_QUARTZ) put(AT_LAND, NI_ROCK, "TRANSLUCENT STONE", meanC, true);
                else if (rockFamily(s.material)) put(AT_LAND, NI_ROCK, "BARE ROCK", meanC, true);
                else put(AT_LAND, NI_DUST, g.hasAir ? "DUST" : "REGOLITH", meanC, true);
                break;
        }
    }
    // the visible life's ground no hotter than its limits were drawn round, and a tenth of a degree (the sampling's rules)
    const bool seen = livingGround(b.type) || kindHas(b.type, KT_FLORA | KT_FAUNA | KT_MICROBIAL | KT_VENT_LIFE);
    if (seen && (H.niche == NI_MAT || H.niche == NI_SOIL || H.niche == NI_CRUST || H.niche == NI_SILT) && H.meanC > 50) H.meanC = 50;
    if (chem == CH_WATER && H.meanC < WATER_COLDEST_C) H.salt = 2;   // J-13: colder than any water but a brine stays liquid (Mars's: Martinez and Renno 2013)
    H.meanC = std::round(H.meanC * 10) / 10;
    return P;
}

SeedVerdict seedPlaceFacts(const StarSystem& sys, int body, double lat, double lon, double depthKm, const MadeWorld& before) {
    SeedVerdict V;
    if (body < 0 || body >= (int)sys.bodies.size()) { V.fate = SF_UNVIABLE; return V; }
    const Body& b = sys.bodies[body];
    BodyGen g = BodyGen::make(b);
    if (before.on()) g.made = before;   // KI-457: the place as the lines before it left it (its ground, its seas, its warmth)
    V.water = g.matColor[MAT_WATER];
    V.forest = g.matColor[MAT_FOREST]; V.reefs = g.hasTrait(TR_REEFS);   // J-05
    const auto L = lifeOf(sys);
    const Biota& B = L->bodies[body];
    V.place = seedPlaceOf(sys, body, g, B, lat, lon, depthKm);
    const int cls = sys.sunless() ? STAR_ROGUE : (sys.ofCompanion(body) ? sys.companionStar().cls : sys.star.cls);
    V.medium = mediumOfPlace(B, b.type, cls, V.place.hab, V.place.hab.meanC);
    V.medium.tempC = std::round(V.place.hab.meanC) + 0.0;   // J-13: the place at its own warmth (a flask's cell's range never held a world's); (J-14: never -0)
    const int chem = V.place.deep() ? CH_WATER : (V.place.aloft() ? CH_ACID : (B.chemistry != CH_NONE ? B.chemistry : BIOTA[b.type].chemistry));
    const int n = V.place.hab.niche;
    const bool oxygenAir = bodyHasAir(b) && (KINDS[b.type].shapes & KS_OXYGEN_AIR);
    V.oxygen = !oxygenAir ? 0 : (n == NI_SILT || n == NI_MAT || n == NI_VENT || n == NI_SPRING ? 1 : 2);
    V.ph = placePH(b.type, chem, V.place.hab);
    V.doseGy = placeDoseGy(b, g, cls, n);
    if (V.place.deep()) {   // J-13: down a drill hole: water whatever the world's liquid, the dark, the rock's hydrogen (Stevens and McKinley 1995; Enceladus's
        // plumes: Waite et al. 2017), no oxygen, the rock's own radioactivity
        V.medium.cell = CC_WATER; V.medium.light = CL_DARK; V.medium.energy = CE_HYDROGEN; V.medium.salt = clampi(V.place.hab.salt, 0, 2);
        V.oxygen = 0; V.doseGy = 0.01; V.ph = 8;   // (the deep's water a little alkaline, whatever the ground's: Lollar et al. 2014)
    }
    // J-14: up in the clouds: their droplets sulphuric acid whatever the world's life (Venus's: Seager et al. 2021), fresh, the sun's light, the air's dose
    if (V.place.aloft()) { V.medium.cell = CC_ACID; V.medium.salt = 0; }
    if (BIOTA[b.type].furnace) V.fate = SF_FURNACE;
    return V;
}

SeedVerdict seedVerdictOf(const StarSystem& sys, int body, const SeedLine& line, double lat, double lon, double depthKm, const MadeWorld& before,
                          const std::vector<int>* gone) {
    SeedVerdict V = seedPlaceFacts(sys, body, lat, lon, depthKm, before);
    if (body < 0 || body >= (int)sys.bodies.size()) return V;
    const Body& b = sys.bodies[body];
    const auto L = lifeOf(sys);
    const Biota& B = L->bodies[body];
    const int chem = V.place.deep() ? CH_WATER : (V.place.aloft() ? CH_ACID : (B.chemistry != CH_NONE ? B.chemistry : BIOTA[b.type].chemistry));
    V.doseTaken = line.kind.s.doseGy * (1 + 40 * line.kind.world.shield);
    if (!line.valid) { V.fate = SF_UNVIABLE; return V; }
    if (V.fate == SF_FURNACE) return V;
    // the natives at the place (what a vial there holds alive), each as a flask takes it, and how the line meets each
    struct Native { int species = -1; double cells = 0; CultureKind kind; int pair = PA_NONE, lead = -1; };
    std::vector<Native> natives;
    bool fed = false;
    {
        const std::vector<Morph> forms = morphsOf(sys, body);
        const Sample S = sampleOf(B, V.place.hab);
        for (const Held& h : S.held) {
            if (h.state != SS_LIVING || h.cells < 1 || h.species < 0 || h.species >= (int)B.species.size()) continue;
            if (gone && std::find(gone->begin(), gone->end(), h.species) != gone->end()) continue;   // (J-16: driven out by a line before it)
            Native nv; nv.species = h.species; nv.cells = h.cells;
            nv.kind = cultureKindOf(B, h.species, h.species < (int)forms.size() ? 2 * forms[h.species].bodyUm() : 2.0);
            if (chemistryCell(nv.kind.chemistry) != V.medium.cell) continue;   // (J-13: a native of another liquid shares no deep water with it)
            nv.pair = pairOf(line.kind, nv.kind, nv.lead);
            if ((nv.pair == PA_CYCLE && nv.lead == 0) || (nv.pair == PA_FEED && nv.lead == 1)) fed = true;   // its prey, or its feeder's leavings
            natives.push_back(nv);
        }
    }
    V.growth = growthIn(line.kind, V.medium, fed);
    if (V.growth.fit != GF_GROWS) { V.fate = fateOfFit(V.growth.fit); return V; }
    if (V.oxygen == 2 && line.kind.s.oxygen == 0) { V.fate = SF_OXYGEN; return V; }
    if (V.oxygen == 0 && line.kind.s.oxygen == 2) { V.fate = SF_NO_OXYGEN; return V; }
    if (chem != CH_HYDROCARBON) {   // (a methane sea has no pH)
        if (V.ph < line.kind.s.phLo - 0.5) { V.fate = SF_ACID; return V; }
        if (V.ph > line.kind.s.phHi + 0.5) { V.fate = SF_ALKALINE; return V; }
    }
    if (V.doseGy > V.doseTaken) { V.fate = SF_DOSE; return V; }
    if (V.place.dry && line.kind.world.dry <= 0) { V.fate = SF_DRIED; return V; }
    // the natives: one that competes and grows there a tenth faster outgrows it (its genes' price in its rate), one slower it outgrows, and
    // between they live side by side; it eats its prey; its poison kills them
    double mostCells = -1;
    for (const Native& nv : natives) {
        bool beats = false;
        if (nv.pair == PA_POISON || nv.pair == PA_CYCLE) beats = nv.lead == 0;
        else if (nv.pair == PA_COMPETE) {
            const Growth ng = growthIn(nv.kind, V.medium);
            if (ng.fit == GF_GROWS && ng.mu > 1.1 * V.growth.mu) { V.native = nv.species; V.pair = nv.pair; V.fate = SF_OUTGROWN; return V; }
            beats = ng.fit != GF_GROWS || V.growth.mu > ng.mu;
        }
        if (nv.pair == PA_POISON && nv.lead == 1) { V.native = nv.species; V.pair = nv.pair; V.fate = SF_OUTGROWN; return V; }
        if (!beats) continue;
        V.beaten.push_back(nv.species); V.beatenHow.push_back(nv.pair);   // (J-12: how)
        if (nv.cells > mostCells) { mostCells = nv.cells; V.native = nv.species; V.pair = nv.pair; }
    }
    // its pace: by its doubling there against two hours' (a slow grower half as fast at most, a quick one half again), its lift's on top
    // (R-449: its doubling in a flask a fifth of Z-05's, so two hours a fifth too: the pace as before)
    const double doubling = V.growth.mu > 0 ? std::log(2.0) / V.growth.mu : 1e9;
    V.pace = clampd(std::pow(7200.0 * FLASK_QUICK / doubling, 0.3), 0.5, 1.5) * (1 + line.kind.world.lift);
    V.drift = seedDriftOf(b.seed, line.key, V.place.hab.place, line.genes, V.doseGy);
    return V;
}

std::string seedVerdictWords(const SeedVerdict& v) {
    if (v.takes()) return "CULTURE GROWS";
    return std::string("CULTURE DIED, ") + SEED_FATE_NAMES[clampi(v.fate, 0, SF_COUNT - 1)];
}

namespace {
std::string sayf(const char* f, ...) {
    char buf[160];
    va_list a; va_start(a, f); std::vsnprintf(buf, sizeof buf, f, a); va_end(a);
    return buf;
}
const char* liquidWords(int cell) { return cell == CC_CRYOSTAT ? "LIQUID METHANE" : CULTURE_CELL_NAMES[clampi(cell, 0, CC_COUNT - 1)]; }
const char* saltWords(int s) { return s <= 0 ? "FRESH" : (s == 1 ? "SEA" : "BRINE"); }
std::string saltsWords(int lo, int hi) { return lo == hi ? std::string(saltWords(lo)) : std::string(saltWords(lo)) + " TO " + saltWords(hi); }
std::string foodWords(const Microbe& s) {
    switch (s.way) {
        case LW_LIGHT: return "LIGHT";
        case LW_CHEMICAL: return s.substrate > SU_NONE && s.substrate < SU_COUNT ? SUBSTRATE_NAMES[s.substrate] : "NOTHING";
        case LW_DEAD: return "DEAD MATTER";
        case LW_GRAZER: case LW_PREDATOR: return "SMALLER CELLS";
        default: return "NOTHING";
    }
}
const char* energyWords(int e) { return e == CE_BROTH ? "DEAD MATTER" : (e == CE_NONE ? "NOTHING" : CULTURE_ENERGY_NAMES[clampi(e, 0, CE_COUNT - 1)]); }
// the food a place gives: its medium's, and an acid's iron and sulphide (J-16)
std::string placeFoodWords(const Medium& m) {
    if (m.cell != CC_ACID || m.energy == CE_IRON || m.energy == CE_SULPHIDE) return energyWords(m.energy);
    return sayf("%s, IRON, SULPHIDE", energyWords(m.energy));
}
double placeC(const SeedVerdict& v) { return std::round(v.place.hab.meanC) + 0.0; }   // its reading, not the medium's (J-14: + 0.0, never "-0 C")
std::string gyWords(double gy) { return gy < 10 ? sayf("%.1f", gy) : sayf("%.0f", gy); }
}   // namespace

std::string seedNeedsWords(const CultureKind& k) {
    const Microbe& s = k.s;
    return sayf("%s, %s, %s, %.0f TO %.0f C", liquidWords(chemistryCell(k.chemistry)), foodWords(s).c_str(), saltsWords(s.saltLo, s.saltHi).c_str(), s.tMin, s.tMax) +
           (k.world.dry > 0 ? ", DRY OK" : "");
}

std::string seedPlaceWords(const SeedVerdict& v) {
    if (v.fate == SF_FURNACE) return "A FURNACE: MOLTEN GROUND";
    const Medium& m = v.medium;
    return sayf("%s, %s, %s, %s, %.0f C", liquidWords(m.cell), placeFoodWords(m).c_str(), m.light == CL_DARK ? "DARK" : "LIT", saltWords(m.salt), placeC(v)) +
           (v.place.dry ? ", DRY" : "");
}

std::string seedWhyWords(const SeedVerdict& v, const CultureKind& k) {
    const Microbe& s = k.s;
    const Medium& m = v.medium;
    switch (v.fate) {
        case SF_GROWS: return "CULTURE GROWS";
        case SF_FURNACE: return "A FURNACE: NOTHING LIVES ON ITS GROUND";
        case SF_SOLVENT: return sayf("WRONG LIQUID: IT LIVES IN %s, THE PLACE HOLDS %s", liquidWords(chemistryCell(k.chemistry)), liquidWords(m.cell));
        case SF_HOT: return sayf("TOO HOT: IT GROWS UP TO %.0f C, THE PLACE IS %.0f C", s.tMax, placeC(v));
        case SF_COLD: return sayf("TOO COLD: IT GROWS FROM %.0f C, THE PLACE IS %.0f C", s.tMin, placeC(v));
        case SF_FROZEN: return sayf("FROZEN: ITS OWN ICE FORMS AT %.0f C THERE", placeC(v));
        case SF_SALT: return sayf("WRONG SALT: IT LIVES IN %s WATER, THE PLACE IS %s", saltsWords(s.saltLo, s.saltHi).c_str(), saltWords(m.salt));
        case SF_HUNGRY:
            if (s.way == LW_GRAZER || s.way == LW_PREDATOR) return "NO FOOD: IT EATS SMALLER CELLS, NONE THERE IT CAN CATCH";
            return sayf("NO FOOD: IT EATS %s, THE PLACE GIVES %s", foodWords(s).c_str(), placeFoodWords(m).c_str());
        case SF_DARK: return "NO LIGHT: IT FEEDS ON LIGHT, THE PLACE IS DARK";
        case SF_UNVIABLE: return "DOES NOT LIVE: A VITAL GENE IS BROKEN";
        case SF_BURNT: return "BURNT: THE ULTRAVIOLET THERE KILLS IT";
        case SF_OXYGEN: return "OXYGEN KILLED IT: IT LIVES WITHOUT OXYGEN";
        case SF_NO_OXYGEN: return "NO OXYGEN: IT BREATHES OXYGEN, THE PLACE HAS NONE";
        case SF_ACID: return sayf("TOO ACID: IT TAKES PH %.1f AND OVER, THE PLACE IS PH %.1f", s.phLo - 0.5, v.ph);
        case SF_ALKALINE: return sayf("TOO ALKALINE: IT TAKES UP TO PH %.1f, THE PLACE IS PH %.1f", s.phHi + 0.5, v.ph);
        case SF_DOSE: return "RADIATION: IT TAKES " + gyWords(v.doseTaken) + " GY A YEAR, THE PLACE GETS " + gyWords(v.doseGy);
        case SF_DRIED: return "DRIED OUT: DRY GROUND, AND IT CANNOT LIVE DRY";
        case SF_OUTGROWN: return "OUTGROWN: A NATIVE THERE GROWS FASTER THAN IT";
        default: return seedVerdictWords(v);
    }
}

std::vector<GeneEdit> seedDriftOf(uint64_t worldSeed, const std::string& lineKey, uint64_t placeKey, const MicrobeGenes& genes, double doseGy) {
    std::vector<GeneEdit> out;
    if (!genes.valid || genes.genes.empty()) return out;
    Rng r(mix64(worldSeed ^ 0xD81F7E5EEDULL) ^ mix64(fnv1a(lineKey) + placeKey * 0x9E3779B97F4A7C15ULL));
    if (r.uni() >= (doseGy > 30 ? 0.6 : 0.3)) return out;   // a strong ultraviolet changes it twice as often
    const int n = r.uni() < 0.3 ? 2 : 1;
    std::vector<int> can;   // its genes the wild can change: any but a vital one, not cut
    for (size_t i = 0; i < genes.genes.size(); i++) {
        const MicrobeGene& G = genes.genes[i];
        if (G.kind != GN_VITAL && !G.cut && !G.part.empty()) can.push_back((int)i);
    }
    const char* abc = alphabetOf(genes.letters);
    const int letters = (int)std::strlen(abc);
    for (int k = 0; k < n && !can.empty(); k++) {
        const int j = std::min((int)can.size() - 1, (int)(r.uni() * can.size()));
        const MicrobeGene& G = genes.genes[can[j]];
        can.erase(can.begin() + j);
        GeneEdit e; e.place = G.place;
        if (r.uni() < 0.3) e.cut = true;   // broken
        else {                             // a letter stepped to another of the alphabet
            e.part = G.part;
            const int at = std::min((int)e.part.size() - 1, (int)(r.uni() * e.part.size()));
            const char* p = std::strchr(abc, e.part[at]);
            const int was = p ? (int)(p - abc) : 0;
            e.part[at] = abc[(was + 1 + (int)(r.uni() * (letters - 1))) % letters];
        }
        out.push_back(e);
    }
    std::sort(out.begin(), out.end(), [](const GeneEdit& a, const GeneEdit& c) { return a.place < c.place; });
    return out;
}

SeedLine seedWildLine(const SeedLine& line, const std::vector<GeneEdit>& drift) {
    SeedLine w = line;
    if (drift.empty() || !line.valid || !line.genes.valid) return w;
    applyEdits(w.kind, w.form, w.genes, drift);
    return w;
}

// J-04: the share of a world's surface a line lives on (its spread's habitat at 96 points spread evenly over the sphere, read at 20 km) and the
// world's mean warmth as generated over the same points (at the water's level or the ground's)
// J-11: and the share of its land an overgrowth of the line could lie on (land that is not lava, snow or ice, within its band of warmth)
// KI-462: its plants count too, where a host of plants or a wood rotter lives in them; KI-457: its band read at the warmth the lines before it
// made (`warmC`), the world's mean kept as generated (what a runaway and the ice pass)
// J-13: a deep line's share: under a sea under the ice, all of it; in the rock, wherever its band of warmth lies between half a kilometre down and
// the drill's deepest (the world's gradient under the surface's warmth); its gases reach the air at DEEP_GAS_SHARE of it, and it grows over nothing
// J-14: a cloud line's: wherever its band of warmth lies in the deck over the point, from its lowest height to its highest; all of it in the air
// J-16 (KI-487): and the warmth of its waters and of its land over the same points (`SeedVerdict::wetLoC` and the rest), lava aside
static void seedHabitatOf(const StarSystem& sys, int body, const MadeSpread& sp, double freezeC, double warmC, SeedVerdict& V) {
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    MadeWorld m; m.tempK = b.tempK; m.sunless = sys.sunless(); m.freezeC = freezeC; m.warmC = warmC;
    const double gradC = sp.deep == MADE_DEEP_ROCK ? deepGradientC(sys, body) : 0.0, deepKm = sp.deep == MADE_DEEP_ROCK ? deepestKm(sys, body) : 0.0;
    const std::vector<double> deck = sp.deep == MADE_CLOUDS ? seedHeightsOf(sys, body) : std::vector<double>();
    constexpr int N = 96;
    int lives = 0, land = 0, grows = 0, room = 0; double sum = 0;
    for (int i = 0; i < N; i++) {
        const double z = 1 - (2 * i + 1) / (double)N, r = std::sqrt(std::max(0.0, 1 - z * z)), phi = i * 2.399963229728653;   // the golden angle
        const Vec3 u(r * std::cos(phi), r * std::sin(phi), z);
        const SurfaceSample smp = sampleSurface(g, u, 20000);
        const bool wet = madeWetAt(smp);
        const int salt = wet ? madeSaltAt(g, smp) : 0;
        const double c0 = placeMeanC(g, b.tempK, m.sunless, u, wet ? smp.water : smp.height);
        sum += c0;
        if (sp.deep == MADE_UNDER_ICE) { lives++; continue; }
        if (sp.deep == MADE_CLOUDS) {
            const double c = placeMeanC(g, b.tempK, m.sunless, u, 0) + warmC;
            lives += !deck.empty() && c - CLOUD_LAPSE_C * deck.back() < sp.tHi && c - CLOUD_LAPSE_C * deck.front() > sp.tLo;
            continue;
        }
        if (sp.deep == MADE_DEEP_ROCK) {
            const double top = c0 + warmC + 0.5 * gradC, bottom = c0 + warmC + deepKm * gradC;   // (J-16: the drill's deepest on this world)
            lives += smp.material != MAT_LAVA && top < sp.tHi && bottom > sp.tLo;
            continue;
        }
        const double c = madeMeanAt(g, m, u, smp, wet, salt);
        if (smp.material != MAT_LAVA) {
            double& lo = wet ? V.wetLoC : V.dryLoC; double& hi = wet ? V.wetHiC : V.dryHiC;
            lo = std::min(lo, c); hi = std::max(hi, c);
        }
        const bool plants = sp.plants && !wet && madeClassOf(g, smp.material, false) == MC_PLANTS && c > sp.tLo && c < sp.tHi;
        if (smp.material != MAT_LAVA && (madeLives(sp, wet, salt, c) || plants)) lives++;
        room += smp.material != MAT_LAVA && smp.material != MAT_ICE && (wet ? ((sp.waters >> clampi(salt, 0, 2)) & 1) != 0 : (sp.land || (sp.plants && madeClassOf(g, smp.material, false) == MC_PLANTS)));
        if (!wet && smp.material != MAT_LAVA) { land++; grows += smp.material != MAT_SNOW && smp.material != MAT_ICE && c >= sp.tLo && c <= sp.tHi; }
    }
    V.share = lives / (double)N * (sp.deep == MADE_DEEP_ROCK || sp.deep == MADE_UNDER_ICE ? DEEP_GAS_SHARE : 1.0); V.worldC = sum / N;
    V.room = sp.deep != MADE_SURFACE ? 1.0 : room / (double)N;
    // J-16 (KI-498): a line that took lives at least where it went down (a ring of seas the points miss), and its room holds that
    if (sp.deep == MADE_SURFACE && lives == 0) V.share = 1.0 / N;
    V.room = std::max(V.room, V.share);
    V.growLand = land ? grows / (double)land : 0.0;
}

void seedingRun(const StarSystem& sys, int body, Seeding& s, const std::vector<Seeding>* earlier) {
    MadeWorld before;   // KI-457: the world as the lines before it had made it when it went down (none: as generated)
    if (earlier && !earlier->empty()) before = madeWorldOf(sys, body, *earlier, s.clock);
    std::vector<int> gone;   // J-16 (KI-487): the natives the lines before it drove out by the time it went down
    if (earlier)
        for (const Seeding& e : *earlier)
            for (const DiedOut& d : seedingDiedOut(lifeOf(sys)->bodies[body], e, s.clock)) gone.push_back(d.species);
    s.verdict = seedVerdictOf(sys, body, s.sent, s.lat, s.lon, s.depthKm, before, &gone);
    s.wild = s.verdict.takes() ? seedWildLine(s.sent, s.verdict.drift) : s.sent;
    s.run = true;
    if (!s.verdict.takes()) return;
    const MadeSpread sp = spreadOf(s);
    const double freezeC = freezeOf(s.wild.kind.chemistry);
    seedHabitatOf(sys, body, sp, freezeC, before.warmC, s.verdict);
    // J-16 (KI-499): its share read again at the warmth its own gas makes with the lines before it (every line at its end), and kept where that
    // is more: a line that opens its world to itself spreads and gives off more until it settles; one whose warmth drives it back keeps the air
    // it made (its gas stays: Daisyworld's feedback, with no undoing); six rounds at most
    if (sp.deep != MADE_SURFACE) return;   // (a deep or a cloud line's share is not the surface's warmth's)
    std::vector<Seeding> with = earlier ? *earlier : std::vector<Seeding>();
    with.push_back(s);
    for (int round = 0; round < 6; round++) {
        with.back().verdict = s.verdict;
        SeedVerdict V = s.verdict;
        V.wetLoC = V.dryLoC = 1e9; V.wetHiC = V.dryHiC = -1e9;
        seedHabitatOf(sys, body, sp, freezeC, madeWorldOf(sys, body, with, 1e18).warmC, V);
        if (V.share <= s.verdict.share + 0.5 / 96) break;
        s.verdict.share = V.share; s.verdict.growLand = V.growLand;
        s.verdict.wetLoC = V.wetLoC; s.verdict.wetHiC = V.wetHiC; s.verdict.dryLoC = V.dryLoC; s.verdict.dryHiC = V.dryHiC;
    }
}

double seedShareAt(const StarSystem& sys, int body, const Seeding& s, double warmC) {
    if (!s.run || !s.verdict.takes()) return 0;
    SeedVerdict V = s.verdict;
    seedHabitatOf(sys, body, spreadOf(s), freezeOf(s.wild.kind.chemistry), warmC, V);
    return V.share;
}

int seedingStep(const Seeding& s, double clock) {
    if (clock < s.clock) return -1;
    const double pace = s.run ? s.verdict.pace : 1.0;
    return (int)std::min<double>(MADE_STEPS, std::floor((clock - s.clock) * pace / MADE_STEP_S));
}

std::vector<DiedOut> seedingDiedOut(const Biota& B, const Seeding& s, double clock) {
    std::vector<DiedOut> out;
    if (!s.run || !s.verdict.takes() || seedingStep(s, clock) < MADE_STEPS) return out;
    const MadeSpread sp = spreadOf(s);
    for (size_t k = 0; k < s.verdict.beaten.size(); k++) {
        const int n = s.verdict.beaten[k];
        if (n < 0 || n >= (int)B.species.size()) continue;
        const Microbe& m = B.species[n];
        bool only = true;   // (it lives in no niche or salt the line does not)
        if (sp.deep != MADE_SURFACE) {   // (J-13: a deep one's alone, any salt; J-14: a cloud line's the clouds')
            const int alone = sp.deep == MADE_UNDER_ICE ? NI_PLUME : (sp.deep == MADE_CLOUDS ? NI_CLOUD : NI_ROCK);
            only = (m.niches & ~nicheBit(alone)) == 0;
        }
        for (int ni = 0; ni < NI_COUNT && only && sp.deep == MADE_SURFACE; ni++) {
            const bool wet = ni == NI_LIQUID || ni == NI_SILT || ni == NI_MAT || ni == NI_SPRING || ni == NI_VENT;   // (as a vial's microhabitat reads them)
            if ((m.niches & nicheBit(ni)) && !wet) only = sp.land;
        }
        for (int salt = std::max(0, m.saltLo); salt <= std::min(2, m.saltHi) && only && sp.deep == MADE_SURFACE; salt++) only = (sp.waters >> salt) & 1;
        for (int ni = 0; ni < NI_COUNT && only && sp.deep == MADE_SURFACE; ni++) {   // J-16 (KI-487): a part of its band of warmth the world has, the line's not
            if (!(m.niches & nicheBit(ni))) continue;
            const bool wet = ni == NI_LIQUID || ni == NI_SILT || ni == NI_MAT || ni == NI_SPRING || ni == NI_VENT;
            const double lo = std::max(m.tMin, wet ? s.verdict.wetLoC : s.verdict.dryLoC), hi = std::min(m.tMax, wet ? s.verdict.wetHiC : s.verdict.dryHiC);
            if (lo < hi && (lo < sp.tLo || hi > sp.tHi)) only = false;
        }
        if (!only) continue;
        DiedOut d; d.species = n; d.how = k < s.verdict.beatenHow.size() ? s.verdict.beatenHow[k] : PA_COMPETE;
        out.push_back(d);
    }
    return out;
}

bool seedingLivesIn(const Seeding& s, const Microhabitat& H) {
    if (!s.run || !s.verdict.takes()) return false;
    const MadeSpread sp = spreadOf(s);
    const int n = H.niche;
    if (sp.deep == MADE_CLOUDS) return n == NI_CLOUD && H.meanC > sp.tLo && H.meanC < sp.tHi;   // J-14: a cloud line in a vial of its clouds (the scoop's)
    if (sp.deep != MADE_SURFACE) return sp.deep == MADE_UNDER_ICE && n == NI_PLUME;   // J-13: a deep line in no vial of the surface; a sea's in its plumes'
    const bool wet = n == NI_LIQUID || n == NI_SILT || n == NI_MAT || n == NI_SPRING || n == NI_VENT;   // (as a vial's microhabitat reads them)
    if (madeLives(sp, wet, H.salt, H.meanC)) return true;
    return sp.plants && n == NI_SOIL && H.meanC > sp.tLo && H.meanC < sp.tHi;
}

const char* diedOutWords(int how) { return how == PA_POISON ? "POISONED BY" : (how == PA_CYCLE ? "EATEN BY" : "OUTGROWN BY"); }

MadeWorld madeWorldOf(const StarSystem& sys, int body, const std::vector<Seeding>& seedings, double clock) {
    MadeWorld m;
    if (body < 0 || body >= (int)sys.bodies.size()) return m;
    const Body& b = sys.bodies[body];
    std::vector<const Seeding*> took;   // J-04: the lines that took and went down by the clock, for the air
    int lineSteps[MADE_LINES] = {-1, -1, -1};   // J-08: each spread's steps on past MADE_STEPS (the remains' age)
    const bool fauna = kindHas(b.type, KT_FAUNA), land = kindHas(b.type, KT_FLORA), sea = kindHas(b.type, KT_SEA);
    for (const Seeding& s : seedings) {
        if (m.lines >= MADE_LINES) break;
        if (s.run && !s.verdict.takes()) continue;   // a culture that died there made nothing
        const int step = seedingStep(s, clock);
        if (step < 0) continue;
        lineSteps[m.lines] = (int)std::min<double>(1e6, std::floor((clock - s.clock) * (s.run ? s.verdict.pace : 1.0) / MADE_STEP_S));
        MadeSpread& sp = m.spread[m.lines++];
        LineLooks L;
        sp = spreadOf(s, &L);
        sp.reach = madeReachOfStep(step, b.radiusKm);
        m.step = step;
        if (!s.run) continue;   // (a seeding read no further: its spread alone)
        took.push_back(&s);
        const CultureKind& k = s.wild.kind;
        const bool dyes = k.world.dyes(), glows = k.s.glows;
        m.freezeC = freezeOf(k.chemistry);
        if (step < 1) continue;   // (its looks from its first step)
        if (sp.looks) {
            if (!m.tinted) { m.tinted = true; m.tintOn = MADE_ON_WATER; m.tint = s.verdict.water; }
            if (dyes) { m.dye = k.world.dye; m.tint = lerp(s.verdict.water, dyeColour(k.world.dye) * 0.8f, (float)(0.35 + 0.3 * clampd(k.world.dyeRate, 0, 1))); }   // (its colour in the water: a red tide is a rust)
            if (glows) { m.glow = 1; m.glowTint = GLOW_COLOURS[fnv1a(s.line) % 3]; }
            // J-05: a soup or a jelly thickens a dye's colour, else its own (a milk's, a rust's) is the waters' colour; a foam's leaves theirs
            const bool thick = L.water == MWL_SOUP || L.water == MWL_JELLY;
            if (dyes && thick) m.tint = lerp(m.tint, dyeColour(k.world.dye) * 0.8f, 0.4f);
            else if (!dyes && L.water != MWL_NONE && L.water != MWL_FOAM) m.tint = L.waterTint;
            if (L.water != MWL_NONE || dyes) m.waterLook = dyes && !thick ? MWL_DYE : L.water;
            {   // KI-460: its own waters' colour where it lies (a second line's its own beside it), and its own glow
                RGB own = s.verdict.water;
                if (dyes) own = lerp(s.verdict.water, dyeColour(k.world.dye) * 0.8f, (float)(0.35 + 0.3 * clampd(k.world.dyeRate, 0, 1)));
                if (dyes && thick) own = lerp(own, dyeColour(k.world.dye) * 0.8f, 0.4f);
                else if (!dyes && L.water != MWL_NONE && L.water != MWL_FOAM) own = L.waterTint;
                sp.waterTinted = true; sp.waterTint = own; sp.waterColours = dyes || (L.water != MWL_NONE && L.water != MWL_FOAM);
                if (glows) { sp.glow = 1; sp.glowTint = GLOW_COLOURS[fnv1a(s.line) % 3]; }
            }
            if (L.foam > 0) { m.foam = std::max(m.foam, L.foam); m.foamTint = L.foamTint; }
            m.jelly = std::max(m.jelly, L.jelly);
        }
        // J-05: its land's, its snow's and its plants' looks (the latest line's in each; the plants' where the world has plants), its traits
        if (!kindHas(b.type, KT_FLORA)) { L.plant = MadeLook(); L.flora = 0; L.galls = L.rot = 0; sp.makes &= ~(1 << MC_PLANTS); }
        m.seas |= L.seas; m.grounds |= L.grounds; m.flora |= L.flora;
        sp.look[MC_LAND] = L.land; sp.look[MC_SNOW] = L.snow; sp.look[MC_PLANTS] = L.plant;   // (KI-460: each line its own)
        if (L.plant.on && L.plantGlow > 0) { sp.glow = std::max(sp.glow, L.plantGlow); sp.glowTint = L.plantGlowTint; }
        if (L.land.on && L.landGlow > 0) { sp.glow = std::max(sp.glow, L.landGlow); sp.glowTint = L.landGlowTint; }   // J-16 (KI-506)
        if (L.land.on) { m.land = L.land; m.landGlow = L.landGlow; m.landGlowTint = L.landGlowTint; }
        if (L.land.on && L.land.kind == MGT_PITTED) sp.pits = clampi(lineSteps[m.lines - 1], 1, MADE_GROWTH_STEPS);   // J-10: its ruins crumble
        if (L.snow.on) m.snow = L.snow;
        if (L.plant.on) { m.plant = L.plant; m.plantShare = L.plantShare; m.plantGlow = L.plantGlow; m.plantGlowTint = L.plantGlowTint; }
        m.galls = std::max(m.galls, L.galls); m.rot = std::max(m.rot, L.rot);
        if (fauna && sp.deep == MADE_SURFACE) {   // J-08: what it does to the animals where it reached, and the steps since it began (J-13: none from the deep)
            sp.fate = lineFateOf(s, sp, L, step, land, sea);
            if (sp.fate.on()) sp.fate.age = clampi(lineSteps[m.lines - 1] - 2, 0, MADE_FATE_STEPS);
        }
        sp.growth = growthOf(s, lineSteps[m.lines - 1], b.radiusKm);   // J-09: what it grows into, creeping behind its spread
        if (sp.growth.on()) { m.growths |= 1 << sp.growth.type; m.growthLine = m.lines - 1; }
        MadeGrowth& G = sp.growth;
        if (G.on() && (G.type == OG_CARPETS || G.type == OG_LICHEN) && madeGreens(b.type)) {   // J-15: a light-feeder's carpet or lichen on a dry world grows grass and shrubs where it lies, behind its edge
            G.greens = true;
            G.greenReach = madeReachOfStep(std::min(G.steps - MADE_GREEN_AFTER, MADE_STEPS), b.radiusKm);
            G.thickReach = madeReachOfStep(std::min(G.steps - MADE_GREEN_THICK, MADE_STEPS), b.radiusKm);
            G.plants = k.s.pigment != PG_NONE ? pigmentColour(k.s.pigment) : G.colour;   // (their colour its pigment's, as its leaves would be; a dye's over it)
            if (k.world.dyes()) G.plants = lerp(G.plants, dyeColour(k.world.dye), 0.6f);
            G.colour = G.plants;   // (J-16, KI-509: its carpet the colour of its grass: one bank its line's in every view)
            m.green = std::max(m.green, madeCapShare(G.greenReach) * G.land);
        }
    }
    if (m.lines) { m.tempK = b.tempK; m.sunless = sys.sunless(); }
    madeAirOf(b, took, clock, m);   // J-04
    if (m.lines && (kindHas(b.type, KT_FAUNA) || m.grown())) m.radiusKm = b.radiusKm;   // (J-08, J-09: where the spreads had reached at each step)
    madeFaunaOf(b, took, clock, lineSteps, m);   // J-08
    madeKindOf(b, took, clock, m);   // J-11
    if (m.looks()) m.key = madeKey(m);
    return m;
}

MadeWorld madeTestLayer(const Vec3& place, double reach, int cover, double coverAlbedo, const RGB& tint) {
    MadeWorld m;
    m.step = 6; m.lines = 1;
    m.spread[0].place = normalize(place); m.spread[0].reach = reach;
    m.cover = cover; m.coverAlbedo = coverAlbedo;
    m.tinted = true; m.tint = tint;
    m.key = madeKey(m);
    return m;
}

std::vector<std::string> madeTraitWords(const MadeWorld& m, bool seas) {
    std::vector<std::string> w;
    if (!m.on()) return w;
    if (m.tinted && m.tintOn == MADE_ON_WATER && !m.dried() && !m.frozen()) {   // (a runaway's waters gone, their colours with them; J-11: a snowball's under ice)
        const char* waters = seas ? " SEAS" : " WATERS";
        if (m.dye > WD_NONE && m.dye < WD_COUNT && !(m.dye == WD_BLACK && (m.seas & MS_BLACK))) w.push_back(std::string(WORLD_DYE_NAMES[m.dye]) + waters);   // (J-05's BLACK SEAS)
        if (m.glow > 0) w.push_back(std::string("GLOWING") + waters);
    }
    // J-05: the waters' (none once a runaway dried them), the ground's (a coloured snow by its dye: RED SNOW), the plants' (new leaves by theirs)
    for (int i = 0; i < MADE_SEA_TRAITS; i++) if ((m.seas >> i) & 1 && !m.dried() && !m.frozen()) w.push_back(MADE_SEA_NAMES[i]);
    for (int i = 0; i < MADE_GROUND_TRAITS; i++) {
        if (!((m.grounds >> i) & 1)) continue;
        if ((1 << i) == MGT_COLOURED_SNOW && m.snow.dye > WD_NONE && m.snow.dye < WD_COUNT) w.push_back(std::string(WORLD_DYE_NAMES[m.snow.dye]) + " SNOW");
        else if ((1 << i) == MGT_COLOURED_CRACKS || (1 << i) == MGT_COLOURED_SEEPS) {   // J-13: by the dye (RED CRACKS); J-16: and the glow (GLOWING SEEPS)
            const char* what = (1 << i) == MGT_COLOURED_CRACKS ? " CRACKS" : " SEEPS";
            const bool dyed = m.land.dye > WD_NONE && m.land.dye < WD_COUNT && m.land.kind == (1 << i);
            if (dyed) w.push_back(std::string(WORLD_DYE_NAMES[m.land.dye]) + what);
            if (m.landGlow > 0 && m.land.kind == (1 << i)) w.push_back(std::string("GLOWING") + what);
            if (!dyed && !(m.landGlow > 0 && m.land.kind == (1 << i))) w.push_back(MADE_GROUND_NAMES[i]);
        } else w.push_back(MADE_GROUND_NAMES[i]);
    }
    for (int i = 0; i < MADE_PLANT_TRAITS; i++) {
        if (!((m.flora >> i) & 1)) continue;
        if ((1 << i) == MP_NEW_LEAVES && m.plant.dye > WD_NONE && m.plant.dye < WD_COUNT) w.push_back(std::string(WORLD_DYE_NAMES[m.plant.dye]) + " LEAVES");
        else w.push_back(MADE_PLANT_NAMES[i]);
    }
    for (int i = 1; i < OG_COUNT; i++) if ((m.growths >> i) & 1) w.push_back(MADE_GROWTH_NAMES[i]);   // J-09: the overgrowths after the plants
    if (m.green > 0) w.push_back("GRASS AND SHRUBS");   // J-15: a dry world's carpet's
    if (m.grown() && m.growthLine >= 0 && m.growthLine < MADE_LINES) {   // J-10: the latest's sounds (its squelch underfoot, its vents' breath)
        const MadeGrowth& G = m.spread[m.growthLine].growth;
        if (G.wet >= MADE_WET_GROUND) w.push_back("WET GROUND");
        if (madeGrowthBreathes(G.type)) w.push_back("BREATHING");
    }
    // J-04: the air's, a dyed cloud and rain by their dye (RED CLOUDS; BLOOD RAIN a red or crimson one's)
    for (int i = 0; i < MADE_AIR_TRAITS; i++) {
        const int bit = 1 << i;
        if (!(m.air & bit)) continue;
        if (bit == MA_DYE_CLOUDS && m.cloudDye > WD_NONE && m.cloudDye < WD_COUNT) w.push_back(std::string(WORLD_DYE_NAMES[m.cloudDye]) + " CLOUDS");
        else if (bit == MA_DYE_RAIN && m.rainDye > WD_NONE && m.rainDye < WD_COUNT)
            w.push_back(m.rainDye == WD_RED || m.rainDye == WD_CRIMSON ? std::string("BLOOD RAIN") : std::string(WORLD_DYE_NAMES[m.rainDye]) + " RAIN");
        else w.push_back(MADE_AIR_NAMES[i]);
    }
    // J-08: the animals' last (dyed hides by their dye: RED HIDES)
    for (int i = 0; i < MADE_FAUNA_TRAITS; i++) {
        const int bit = 1 << i;
        if (!(m.fauna & bit)) continue;
        int dye = m.fate.dye;
        for (int j = 0; j < m.lines && j < MADE_LINES && bit == MF_DYED; j++) if (m.spread[j].fate.fauna & MF_DYED) dye = m.spread[j].fate.dye;
        if (bit == MF_DYED && dye > WD_NONE && dye < WD_COUNT) w.push_back(std::string(WORLD_DYE_NAMES[dye]) + " HIDES");
        else w.push_back(MADE_FAUNA_NAMES[i]);
    }
    return w;
}

std::string madeAirWords(const MadeWorld& m) {
    std::string w;
    if (!m.on() || !m.aired()) return w;
    auto add = [&](const std::string& t) { w += (w.empty() ? "" : ", ") + t; };
    if (m.pressureK >= 1.05) add("THICKER");
    if (m.haze > 0) add("HAZY");
    const int c = (int)std::lround(m.warmC);
    if (c != 0) add(std::to_string(std::abs(c)) + (c > 0 ? " C WARMER" : " C COLDER") + " THAN IT WAS");
    return w;
}
