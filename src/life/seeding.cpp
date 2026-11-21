// J-02: a world's seedings and the layer they make of it (life/seeding.h); J-03: the place, the verdict, the drift and the spread
#include "seeding.h"
#include "sequences.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstring>

const char* const SEED_FATE_NAMES[SF_COUNT] = {"GROWS", "A FURNACE", "WRONG LIQUID", "TOO HOT", "TOO COLD", "WRONG SALT", "NO FOOD", "NO LIGHT", "FROZEN",
                                               "DOES NOT LIVE", "BURNT", "OXYGEN KILLED IT", "NO OXYGEN", "TOO ACID", "TOO ALKALINE", "RADIATION", "DRIED OUT",
                                               "OUTGROWN BY THE NATIVES"};

const char* const MADE_AIR_NAMES[MADE_AIR_TRAITS] = {"THICK AIR", "RUNAWAY", "ORANGE HAZE", "SULPHUR AIR", "OZONE", "THIN OZONE", "COLD HAZE",
                                                      "GLOWING CLOUDS", "COLOURED CLOUDS", "COLOURED RAIN", "SEA SMOKE", "ENDLESS RAIN", "SPORE CLOUDS"};
const char* const MADE_SEA_NAMES[MADE_SEA_TRAITS] = {"PURPLE SEAS", "BLACK SEAS", "MILK SEAS", "FOAM SEAS", "JELLY SEAS", "SOUP SEAS", "SOUR SEAS",
                                                      "RUST RIVERS", "BOILING SEAS"};
const char* const MADE_GROUND_NAMES[MADE_GROUND_TRAITS] = {"COLOURED SNOW", "DARK ICE", "CRUSTS", "MATS", "STROMATOLITES", "RUST GROUND", "CHALK GROUND",
                                                            "GLASS SANDS", "SULPHUR CRUSTS", "PITTED STONE", "SLIME"};
const char* const MADE_PLANT_NAMES[MADE_PLANT_TRAITS] = {"BLIGHT", "ROT", "FROST BURN", "GALLS", "NEW LEAVES", "GLOWING FORESTS", "LUSH", "BLEACHED"};

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
    const LineLooks L = looksOf(s, sp);
    sp.looks = k.world.dyes() || k.s.glows || L.water != MWL_NONE;
    sp.makes = (L.land.on ? 1 << MC_LAND : 0) | (L.snow.on ? 1 << MC_SNOW : 0) | (L.plant.on ? 1 << MC_PLANTS : 0);
    sp.plants = L.plants; sp.landOn = L.landOn; sp.lookWaters = L.waterOn; sp.cover = L.cover; sp.coverAlbedo = L.coverAlbedo; sp.coldC = L.coldC;
    if (looks) *looks = L;
    return sp;
}

// J-04: how a world's air answers what its lines give it (IDEAS-seeded-worlds.md section 5): whether it has air, oxygen in it and methane
// (the archean's haze, a hydrocarbon world's); its seas of water, a living ground's climate (its ice and snow move), its plants, its clouds and
// whether they rain; its pressure (the HUD's), its mean warmth as generated; and three hashed values: how much a gas warms it, how much its
// seas take up, and its haze's colour
struct AirWorld {
    bool air = false, oxygen = false, methane = false, wet = false, living = false, flora = false, clouds = false, rains = false;
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
    w.flora = (K.tags & KT_FLORA) != 0;
    w.clouds = w.air && K.clouds;
    w.rains = w.air && (K.rain != RAIN_NONE || (K.tags & (KT_FRONT_RAIN | KT_WATER_WEATHER)));
    w.pressure = K.pressureSpan > 0 ? K.pressureLo + K.pressureSpan * hash01(b.seed) : 0;
    w.meanC = worldC;
    w.warmBy = 0.6 + 0.9 * hash01(mix64(b.seed ^ 0xA1E0A2ULL));
    w.seaTake = hash01(mix64(b.seed ^ 0xA1E0A3ULL));
    w.hazeLook = (int)(hash01(mix64(b.seed ^ 0xA1E0A4ULL)) * 4) & 3;
    return w;
}
// what the living lines give the air at a clock: each gas's amount, and what their lift carries up (a glow, a dye, ice nuclei, spores), each
// by its gene's amount times its answer (`livedOf`), lift's by its lift too; the latest line's colours
struct AirSum {
    double gas[WG_COUNT] = {};
    double glow = 0, dye = 0, frost = 0, spores = 0;
    int dyeOf = WD_NONE, glowOf = 0, sporeOf = 0;
    double darkIce = 0, chalk = 0;   // J-05: the lines that darken the ice and whiten the waters, by their answer
};
// the share of the world a line lives on that gives its gas's whole answer, and the air's answer by the line's step: none before it held
// the region (step 6), all of it once it reached the whole world
constexpr double AIR_SHARE = 0.15;
double livedOf(const Seeding& s, int step) { return clampd(s.verdict.share / AIR_SHARE, 0, 1) * smoothstep(6.0, (double)MADE_STEPS, (double)step); }
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
        const double up = k.world.lift * lived;
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
double roundTo(double v, double q) { return std::round(v / q) * q; }

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
    // a runaway: the first step boundary of any line at which the world's mean passed RUNAWAY_C, its steps since (at the clock's own pace)
    if (w.living && w.wet && w.meanC + warm >= RUNAWAY_C) {
        double onset = clock;
        std::vector<double> marks;
        for (const Seeding* s : took) {
            const double pace = s->verdict.pace > 0 ? s->verdict.pace : 1.0;
            for (int j = 1; j <= MADE_STEPS; j++) { const double c = s->clock + j * MADE_STEP_S / pace + 1e-6; if (c <= clock) marks.push_back(c); }
        }
        std::sort(marks.begin(), marks.end());
        for (double c : marks) if (w.meanC + airWarmthOf(w, airSumAt(took, c)) >= RUNAWAY_C) { onset = c; break; }
        const int steps = (int)std::floor((clock - onset) / MADE_STEP_S + 1e-9);
        m.runaway = steps < RUNAWAY_STEAM ? (steps + 1) / (double)RUNAWAY_STEAM : 2.0;
        warm += 25 * std::min(1.0, m.runaway);   // the steam's own greenhouse
        dP += 1.5 * std::min(1.0, m.runaway);
        m.air |= MA_RUNAWAY;
    }
    m.warmC = std::fabs(warm) < 0.5 ? 0.0 : roundTo(warm, 0.1);
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
    if (!w.oxygen && a.gas[WG_OXYGEN] >= 0.3) { m.air |= MA_OZONE; m.ozone = 1; }
    else if (w.oxygen && a.gas[WG_N2O] + a.gas[WG_H2S] >= 0.3) { m.air |= MA_THIN_OZONE; m.ozone = -1; }
    if (w.living && w.flora) m.wilt = roundTo(std::max(smoothstep(5.0, 18.0, m.warmC), m.runaway >= 1 ? 1.0 : 0.0), 0.01);
    // a fog on the seas: dimethyl sulphide's smoke, a runaway's steam while its seas last
    const double smoke = w.wet ? smoothstep(0.1, 0.5, a.gas[WG_DMS]) : 0.0;
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
    const bool waterSeas = w.wet && b.type != PT_HYDROCARBON && b.type != PT_AMMONIA;
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
}   // namespace

SeedPlace seedPlaceOf(const StarSystem& sys, int body, const BodyGen& g, const Biota& B, double lat, double lon) {
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
    const double meanC = placeMeanC(g, b.tempK, sys.sunless(), P.unit, P.heightM);
    auto put = [&](int where, int niche, const char* name, double c, bool dry) { P.where = where; H.niche = niche; P.name = name; H.meanC = c; P.dry = dry; };
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
    H.meanC = std::round(H.meanC * 10) / 10;
    return P;
}

SeedVerdict seedVerdictOf(const StarSystem& sys, int body, const SeedLine& line, double lat, double lon) {
    SeedVerdict V;
    if (body < 0 || body >= (int)sys.bodies.size()) { V.fate = SF_UNVIABLE; return V; }
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    V.water = g.matColor[MAT_WATER];
    V.forest = g.matColor[MAT_FOREST]; V.reefs = g.hasTrait(TR_REEFS);   // J-05
    const auto L = lifeOf(sys);
    const Biota& B = L->bodies[body];
    V.place = seedPlaceOf(sys, body, g, B, lat, lon);
    const int cls = sys.sunless() ? STAR_ROGUE : (sys.ofCompanion(body) ? sys.companionStar().cls : sys.star.cls);
    V.medium = mediumOfPlace(B, b.type, cls, V.place.hab, V.place.hab.meanC);
    const int chem = B.chemistry != CH_NONE ? B.chemistry : BIOTA[b.type].chemistry;
    const int n = V.place.hab.niche;
    const bool oxygenAir = bodyHasAir(b) && (KINDS[b.type].shapes & KS_OXYGEN_AIR);
    V.oxygen = !oxygenAir ? 0 : (n == NI_SILT || n == NI_MAT || n == NI_VENT || n == NI_SPRING ? 1 : 2);
    V.ph = placePH(b.type, chem, V.place.hab);
    V.doseGy = placeDoseGy(b, g, cls, n);
    V.doseTaken = line.kind.s.doseGy * (1 + 40 * line.kind.world.shield);
    if (!line.valid) { V.fate = SF_UNVIABLE; return V; }
    if (BIOTA[b.type].furnace) { V.fate = SF_FURNACE; return V; }
    // the natives at the place (what a vial there holds alive), each as a flask takes it, and how the line meets each
    struct Native { int species = -1; double cells = 0; CultureKind kind; int pair = PA_NONE, lead = -1; };
    std::vector<Native> natives;
    bool fed = false;
    {
        const std::vector<Morph> forms = morphsOf(sys, body);
        const Sample S = sampleOf(B, V.place.hab);
        for (const Held& h : S.held) {
            if (h.state != SS_LIVING || h.cells < 1 || h.species < 0 || h.species >= (int)B.species.size()) continue;
            Native nv; nv.species = h.species; nv.cells = h.cells;
            nv.kind = cultureKindOf(B, h.species, h.species < (int)forms.size() ? 2 * forms[h.species].bodyUm() : 2.0);
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
        V.beaten.push_back(nv.species);
        if (nv.cells > mostCells) { mostCells = nv.cells; V.native = nv.species; V.pair = nv.pair; }
    }
    // its pace: by its doubling there against two hours' (a slow grower half as fast at most, a quick one half again), its lift's on top
    const double doubling = V.growth.mu > 0 ? std::log(2.0) / V.growth.mu : 1e9;
    V.pace = clampd(std::pow(7200.0 / doubling, 0.3), 0.5, 1.5) * (1 + line.kind.world.lift);
    V.drift = seedDriftOf(b.seed, line.key, V.place.hab.place, line.genes, V.doseGy);
    return V;
}

std::string seedVerdictWords(const SeedVerdict& v) {
    if (v.takes()) return "CULTURE GROWS";
    return std::string("CULTURE DIED, ") + SEED_FATE_NAMES[clampi(v.fate, 0, SF_COUNT - 1)];
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
static void seedHabitatOf(const StarSystem& sys, int body, const MadeSpread& sp, double freezeC, double& share, double& meanC) {
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    MadeWorld m; m.tempK = b.tempK; m.sunless = sys.sunless(); m.freezeC = freezeC;
    constexpr int N = 96;
    int lives = 0; double sum = 0;
    for (int i = 0; i < N; i++) {
        const double z = 1 - (2 * i + 1) / (double)N, r = std::sqrt(std::max(0.0, 1 - z * z)), phi = i * 2.399963229728653;   // the golden angle
        const Vec3 u(r * std::cos(phi), r * std::sin(phi), z);
        const SurfaceSample smp = sampleSurface(g, u, 20000);
        const bool wet = madeWetAt(smp);
        const int salt = wet ? madeSaltAt(g, smp) : 0;
        sum += placeMeanC(g, b.tempK, m.sunless, u, wet ? smp.water : smp.height);
        if (smp.material != MAT_LAVA && madeLives(sp, wet, salt, madeMeanAt(g, m, u, smp, wet, salt))) lives++;
    }
    share = lives / (double)N; meanC = sum / N;
}

void seedingRun(const StarSystem& sys, int body, Seeding& s) {
    s.verdict = seedVerdictOf(sys, body, s.sent, s.lat, s.lon);
    s.wild = s.verdict.takes() ? seedWildLine(s.sent, s.verdict.drift) : s.sent;
    s.run = true;
    if (s.verdict.takes()) seedHabitatOf(sys, body, spreadOf(s), freezeOf(s.wild.kind.chemistry), s.verdict.share, s.verdict.worldC);
}

int seedingStep(const Seeding& s, double clock) {
    if (clock < s.clock) return -1;
    const double pace = s.run ? s.verdict.pace : 1.0;
    return (int)std::min<double>(MADE_STEPS, std::floor((clock - s.clock) * pace / MADE_STEP_S));
}

MadeWorld madeWorldOf(const StarSystem& sys, int body, const std::vector<Seeding>& seedings, double clock) {
    MadeWorld m;
    if (body < 0 || body >= (int)sys.bodies.size()) return m;
    const Body& b = sys.bodies[body];
    std::vector<const Seeding*> took;   // J-04: the lines that took and went down by the clock, for the air
    for (const Seeding& s : seedings) {
        if (m.lines >= MADE_LINES) break;
        if (s.run && !s.verdict.takes()) continue;   // a culture that died there made nothing
        const int step = seedingStep(s, clock);
        if (step < 0) continue;
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
            if (L.foam > 0) { m.foam = std::max(m.foam, L.foam); m.foamTint = L.foamTint; }
            m.jelly = std::max(m.jelly, L.jelly);
        }
        // J-05: its land's, its snow's and its plants' looks (the latest line's in each; the plants' where the world has plants), its traits
        if (!kindHas(b.type, KT_FLORA)) { L.plant = MadeLook(); L.flora = 0; L.galls = L.rot = 0; sp.makes &= ~(1 << MC_PLANTS); }
        m.seas |= L.seas; m.grounds |= L.grounds; m.flora |= L.flora;
        if (L.land.on) m.land = L.land;
        if (L.snow.on) m.snow = L.snow;
        if (L.plant.on) { m.plant = L.plant; m.plantShare = L.plantShare; m.plantGlow = L.plantGlow; m.plantGlowTint = L.plantGlowTint; }
        m.galls = std::max(m.galls, L.galls); m.rot = std::max(m.rot, L.rot);
    }
    if (m.lines) { m.tempK = b.tempK; m.sunless = sys.sunless(); }
    madeAirOf(b, took, clock, m);   // J-04
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
    if (m.tinted && m.tintOn == MADE_ON_WATER && !m.dried()) {   // (a runaway's waters gone, their colours with them)
        const char* waters = seas ? " SEAS" : " WATERS";
        if (m.dye > WD_NONE && m.dye < WD_COUNT && !(m.dye == WD_BLACK && (m.seas & MS_BLACK))) w.push_back(std::string(WORLD_DYE_NAMES[m.dye]) + waters);   // (J-05's BLACK SEAS)
        if (m.glow > 0) w.push_back(std::string("GLOWING") + waters);
    }
    // J-05: the waters' (none once a runaway dried them), the ground's (a coloured snow by its dye: RED SNOW), the plants' (new leaves by theirs)
    for (int i = 0; i < MADE_SEA_TRAITS; i++) if ((m.seas >> i) & 1 && !m.dried()) w.push_back(MADE_SEA_NAMES[i]);
    for (int i = 0; i < MADE_GROUND_TRAITS; i++) {
        if (!((m.grounds >> i) & 1)) continue;
        if ((1 << i) == MGT_COLOURED_SNOW && m.snow.dye > WD_NONE && m.snow.dye < WD_COUNT) w.push_back(std::string(WORLD_DYE_NAMES[m.snow.dye]) + " SNOW");
        else w.push_back(MADE_GROUND_NAMES[i]);
    }
    for (int i = 0; i < MADE_PLANT_TRAITS; i++) {
        if (!((m.flora >> i) & 1)) continue;
        if ((1 << i) == MP_NEW_LEAVES && m.plant.dye > WD_NONE && m.plant.dye < WD_COUNT) w.push_back(std::string(WORLD_DYE_NAMES[m.plant.dye]) + " LEAVES");
        else w.push_back(MADE_PLANT_NAMES[i]);
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
