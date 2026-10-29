// P-00: the kind table and the trait table (galaxy/kinds.h). Built at compile time; a row's id is its index, and the checks at the
// end refuse a table with a row missing, a name unset or too long, or an accent on a kind with flora.
#include "kinds.h"
#include "starfield.h"

namespace {
using namespace tint;
constexpr void zones(KindDef& k, double a, double b, double c, double d, double e) { k.zones[0] = a; k.zones[1] = b; k.zones[2] = c; k.zones[3] = d; k.zones[4] = e; }
constexpr void rule(KindDef& k, int r, double mul, double add = 0) { k.rules[r] = WeightOp{mul, add}; }
constexpr void moons(KindDef& k, double mul, double add = 0) { rule(k, WR_MOON_OF_WORLD, mul, add); rule(k, WR_MOON_OF_GIANT, mul, add); }
constexpr KindChannel ch(float base, float var = 0, bool flip = false) { return KindChannel{base, var, flip}; }
constexpr void colour(KindDef& k, KindChannel r, KindChannel g, KindChannel b) { k.colour.ch[0] = r; k.colour.ch[1] = g; k.colour.ch[2] = b; }
constexpr void palette4(KindDef& k, RGB a, RGB b, RGB c, RGB d) { k.colour.palette = true; k.colour.pal[0] = a; k.colour.pal[1] = b; k.colour.pal[2] = c; k.colour.pal[3] = d; }
constexpr void draw(KindDef& k, int field, double lo, double span, int kind = DK_UNI) { k.draws[k.nDraws++] = GenDraw{field, lo, span, kind}; }
constexpr void mix(KindDef& k, int mat, int mode, RGB c = RGB(), float t = 0, RGB d = RGB()) { k.palette[k.nPalette++] = MatMix{mat, mode, c, d, t}; }
constexpr KindTint tintOf(int mode, RGB c = RGB(), RGB var = RGB()) { return KindTint{mode, c, var}; }
constexpr void crater(KindDef& k, double cell, bool cellRel, double dens, int densMode, double depth, bool depthRel, bool sight = false) {
    k.craters[k.nCraters++] = CraterGrid{cell, cellRel, dens, densMode, depth, depthRel, sight};
}
constexpr void sentence(KindDef& k, int rule, const char* a, const char* b = nullptr, const char* c = nullptr) { k.sentenceRule = rule; k.sentence[0] = a; k.sentence[1] = b; k.sentence[2] = c; }
constexpr void identity(KindDef& k, int id, const char* name, const char* shortName, const char* description, double rMin, double rMax, bool landable, bool atmosphere, int maxMoons, double albedo) {
    k.id = id; k.name = name; k.shortName = shortName; k.description = description; k.minRadiusKm = rMin; k.maxRadiusKm = rMax;
    k.landable = landable; k.atmosphere = atmosphere; k.maxMoons = maxMoons; k.albedo = albedo;
}
constexpr void night(KindDef& k, RGB zen, RGB hor) { k.ownNight = true; k.nightZenith = zen; k.nightHorizon = hor; }
constexpr uint32_t of(int a, int b = -1, int c = -1) { return (1u << a) | (b >= 0 ? 1u << b : 0u) | (c >= 0 ? 1u << c : 0u); }   // P-01: a set of kinds or star classes
// the stars a giant can have lived and wandered close to
constexpr uint32_t LIVE_STARS = ((1u << STAR_DRAWN_COUNT) - 1) & ~(of(STAR_WHITE_DWARF, STAR_PULSAR, STAR_NEUTRON) | of(STAR_BLACK_HOLE, STAR_PROTOSTAR, STAR_WOLF_RAYET));
constexpr void source(KindDef& k, uint32_t from, double tLo, double tHi, uint32_t bodies, uint32_t stars, int lock, double share, bool byStar = false, uint32_t parents = 0, uint32_t moon = 0) {
    k.sources[k.nSources++] = KindSource{from, tLo, tHi, bodies, stars, lock, share, byStar, parents, moon};
}
// P-02: the stars still burning their hydrogen (the giants' worlds are P-04's)
constexpr uint32_t MAIN_SEQUENCE = of(STAR_YELLOW, STAR_ORANGE, STAR_RED_DWARF) | of(STAR_BLUE_WHITE, STAR_BLUE_GIANT);
constexpr uint32_t ALL_BODIES = KB_PLANET | KB_COMPANION_WORLD | KB_TWIN | KB_MOON;
constexpr uint32_t DEAD_STARS = of(STAR_WHITE_DWARF, STAR_PULSAR, STAR_NEUTRON);   // P-03: no light to live by (a black hole's worlds keep no air: S-06)
constexpr void fixedSky(KindDef& k, RGB zen, RGB hor) { k.sky = SKY_FIXED; k.zenith = zen; k.horizon = hor; }
constexpr uint32_t KEEPS_AIR = ((1u << STAR_DRAWN_COUNT) - 1) & ~of(STAR_BLACK_HOLE, STAR_WOLF_RAYET);   // P-05: no air survives a black hole's or a Wolf-Rayet wind's system (S-05, S-06)
constexpr uint32_t WORLD_KINDS = ((1u << PT_DRAWN_COUNT) - 1) & ~of(PT_GASGIANT, PT_SUBSTELLAR);       // P-05: a moon's planet that is no giant (a world's moon)
constexpr uint32_t GIANT_KINDS = of(PT_GASGIANT, PT_SUBSTELLAR);                                          // P-06: a giant's moon
constexpr void rocks(KindDef& k, double density, double scale) { k.rockDensity = density; k.rockScale = scale; }
// P-04: a dead world is the living kind it was under the present's cover: the living kind's draws in its order, its relief, its greenhouse
// and its tints (the sky's three draws kept), so `BodyGen::make` makes the same ground in both times (`thenWorld`); its traits the processes
// that shaped that ground and still show (no flora, no weather, no freeze, no steaming geysers)
constexpr void thenGround(KindDef& k, const KindDef& f) {
    k.thenKind = f.id;
    for (int i = 0; i < f.nDraws; i++) k.draws[i] = f.draws[i];
    k.nDraws = f.nDraws;
    k.relief = f.relief; k.greenhouseK = f.greenhouseK; k.color2 = f.color2; k.skyTint = f.skyTint;
    k.shapes = f.shapes & ~(KS_FLORA | KS_STORMS | KS_HAZE | KS_FREEZE | KS_HYDROTHERMAL);
    k.ground = f.ground;
}

constexpr std::array<KindDef, PT_COUNT> buildKinds() {
    std::array<KindDef, PT_COUNT> K{};
    // B-430: every row's rules set in order before any kind sets its own. GCC 9.4, the Linux builds' compiler, keeps the rows' default
    // rules as one range they all share and cuts it where a row sets a rule past its first (`rule`): the other rows lost their rules
    // after the cut (built 0, not 1, so their draw weights changed), and reading one in place crashed it (`kindsOk`, KI-423)
    for (KindDef& k : K) for (WeightOp& w : k.rules) w = WeightOp{};
    {   KindDef& k = K[PT_MOLTEN];
        identity(k, PT_MOLTEN, "MOLTEN", "MOLTEN", "medium size, internally hot, unstable surface, no atmosphere.", 2000, 4200, true, false, 1, 0.25);
        k.tags = KT_LAVA;
        k.shapes = KS_EXTENSION | KS_BASALTIC | KS_FLOOD_LAVA | KS_IMPACTS | KS_PLUMES;
        zones(k, 40, 9, 0, 0, 0);
        rule(k, WR_PULSAR, 0.3); rule(k, WR_NEUTRON, 0.3); rule(k, WR_BLACK_HOLE, 0); rule(k, WR_PROTOSTAR, 1, 25); rule(k, WR_RED_GIANT, 1.5); rule(k, WR_ORANGE_GIANT, 1.3);
        moons(k, 0.7); rule(k, WR_WOLF_RAYET, 1, 5);
        colour(k, ch(0.85f), ch(0.35f, 0.2f), ch(0.15f));
        sentence(k, SR_COIN, "world of cooling crust, split by glowing cracks and lava lakes", "molten world whose surface never sets, red seams between black plates");
        draw(k, GF_LAVA_LEVEL, -100, -500); draw(k, GF_VOLCANOES, 0.2, 0.5); draw(k, GF_MOUNTAIN, 1200, 1500); draw(k, GF_HILL, 150, 200);
        k.color2 = tintOf(TINT_CONST, RGB(1.0f, 0.55f, 0.1f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.15; k.relief.mtn = 3.7; k.relief.hill = 0.42;   // O6-07: 3.0 read a 0.23 median and 1309 m over 16 km with the cliff bands' benches
        mix(k, MAT_BASALT, MM_BASE_MIX, RGB(0.30f, 0.26f, 0.24f), 0.15f);
        k.famRep[FAM_ROCK] = MAT_BASALT;
        k.ground = GR_MOLTEN;
        k.lavaHorizon = true; k.horizonGlow = RGB(0.12f, 0.06f, 0.04f); k.fog = 30000;
        rocks(k, 0.7, 1.1);
        k.tempAdd = 180;
        k.events = EV_ERUPTIONS; k.rumble = 0.4;
    }
    {   KindDef& k = K[PT_CRATERED];
        identity(k, PT_CRATERED, "CRATERED", "CRATERED", "small, solid, dusty, craterized, no atmosphere.", 900, 3200, true, false, 1, 0.20);
        k.tags = KT_GLASSABLE;
        k.shapes = KS_SCARPS | KS_BASALTIC | KS_FLOOD_LAVA | KS_IMPACTS;
        zones(k, 30, 18, 8, 10, 16);
        rule(k, WR_DEAD, 1, 20); rule(k, WR_BLACK_HOLE, 1, 10); rule(k, WR_BLUE_WHITE, 1.3); moons(k, 1.4); rule(k, WR_WOLF_RAYET, 1.5);
        colour(k, ch(0.62f, 0.1f), ch(0.60f), ch(0.56f));
        sentence(k, SR_CRATERS, "airless world saturated with craters, bright rays crossing dark maria", "dusty, airless world of old craters and quiet plains");
        draw(k, GF_MOUNTAIN, 500, 900); draw(k, GF_HILL, 80, 120); draw(k, GF_CRATER_DENSITY, 0.35, 0.4);
        k.color2 = tintOf(TINT_CONST, RGB(0.3f, 0.3f, 0.3f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.6; k.relief.mtn = 1.5; k.relief.hill = 0.4;   // lunar highlands roll
        mix(k, MAT_DUST, MM_BASE_MIX, DUST_WARM, 0.5f); mix(k, MAT_BASALT, MM_BASE_MIX, RGB(0.30f, 0.29f, 0.28f), 0.3f);
        k.famRep[FAM_ROCK] = MAT_BASALT;
        k.ground = GR_CRATERED; k.plainK = 0.5; k.gullyK = 0.3;   // their plains are crater fields
        crater(k, 0.55, true, 0.6, CD_REL, 1, true); crater(k, 0.18, true, 1, CD_REL, 1, true); crater(k, 0.06, true, 1, CD_REL, 1, true, true);
        crater(k, 12.0, false, 1.2, CD_REL, 1, true, true); crater(k, 2.5, false, 1.5, CD_REL, 1, true, true); crater(k, 0.5, false, 0, CD_SECONDARY, 0.8, true);
        crater(k, 0.1, false, 0.9, CD_ABS, 0.6, true); crater(k, 0.025, false, 0.8, CD_ABS, 0.5, true);
        k.fog = 45000; k.floorFamily = FAM_SAND;
        rocks(k, 1.1, 1.1);
    }
    {   KindDef& k = K[PT_VENUSIAN];
        identity(k, PT_VENUSIAN, "VENUSIAN", "VENUSIAN", "medium size, solid, thick atmosphere, fully covered by clouds.", 4800, 7200, true, true, 2, 0.75);
        k.tags = KT_OPAQUE_DECK | KT_CLOUD_SWIRL | KT_PLATES | KT_TWIN | KT_FLAT;
        k.shapes = KS_INCISION | KS_EXTENSION | KS_SCARPS | KS_BASALTIC | KS_FLOOD_LAVA | KS_PLUMES | KS_STORMS;
        zones(k, 18, 26, 5, 0, 0);
        rule(k, WR_DEAD, 0); rule(k, WR_PROTOSTAR, 0); moons(k, 0.15);
        colour(k, ch(0.95f), ch(0.85f, -0.15f), ch(0.55f, 0.2f));
        sentence(k, SR_ONE, "world smothered in clouds, a furnace of basalt domes below");
        draw(k, GF_MOUNTAIN, 1000, 1200); draw(k, GF_HILL, 200, 200); draw(k, GF_CLOUD_COVER, 1.0, 0, DK_CONST);
        k.color2 = tintOf(TINT_CONST, RGB(0.9f, 0.8f, 0.6f)); k.skyTint = tintOf(TINT_CONST, RGB(0.95f, 0.85f, 0.55f));
        k.relief.mtn = 1.9; k.relief.hill = 0.55;
        mix(k, MAT_CLOUD, MM_BASE); mix(k, MAT_BASALT, MM_BASE_MIX, RGB(0.42f, 0.34f, 0.24f), 0.2f);   // the ground under the deck is warm and dim
        k.famRep[FAM_ROCK] = MAT_BASALT;
        k.ground = GR_VENUSIAN;
        fixedSky(k, RGB(0.85f, 0.7f, 0.45f), RGB(0.95f, 0.85f, 0.6f));
        k.fog = 700; k.skyDensity = 1.3;
        night(k, RGB(0.08f, 0.06f, 0.04f), RGB(0.12f, 0.09f, 0.06f));
        rocks(k, 0.6, 1.4);
        k.pressureLo = 60; k.pressureSpan = 35; k.tempOffset = 420; k.wind = 4;
    }
    {   KindDef& k = K[PT_FELISIAN];
        identity(k, PT_FELISIAN, "FELISIAN", "FELISIAN", "medium size, felisian, breathable atmosphere, suitable for life.", 4800, 7600, true, true, 3, 0.35);
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FLORA | KT_FAUNA | KT_PEOPLE | KT_TWIN;
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES |
                   KS_FLOOD_LAVA | KS_HYDROTHERMAL | KS_OPEN_SEA | KS_FREEZE | KS_CONTINENTS | KS_FLORA | KS_STORMS | KS_HAZE | KS_SOILS;
        zones(k, 0, 0, 48, 0, 0);
        rule(k, WR_DEAD, 0); rule(k, WR_PROTOSTAR, 0); rule(k, WR_RED_GIANT, 0.25); rule(k, WR_BLUE_GIANT, 0.5); rule(k, WR_BLUE_WHITE, 0.6); rule(k, WR_ORANGE_GIANT, 0.5);
        rule(k, WR_CARBON_STAR, 0.2); rule(k, WR_MOON_OF_WORLD, 0.15); rule(k, WR_MOON_OF_GIANT, 0.6);
        colour(k, ch(0.30f, 0.15f), ch(0.62f), ch(0.55f, 0.25f, true));
        sentence(k, SR_SEAS, "living world of wide oceans, scattered continents and river valleys", "living world of dry continents, inland seas and long mountain chains",
                 "living world of continents and seas, forests along its rivers");
        k.greenhouseK = 33;   // a living world's air keeps it 33 degrees over its equilibrium temperature (Earth's greenhouse)
        draw(k, GF_SEA_LEVEL, -0.35, 0.55); draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 150, 250); draw(k, GF_ICE_CAP, 62, 18); draw(k, GF_SNOW_LINE, 2500, 2500);
        draw(k, GF_MOISTURE, 0, 0.25, DK_SYM); draw(k, GF_CLOUD_COVER, 0.25, 0.4);   // more water than land on average; B-321: mountains 2500-5500 before, on a base half as high now
        k.color2 = tintOf(TINT_CONST, RGB(0.2f, 0.45f, 0.75f)); k.skyTint = tintOf(TINT_DRAWN, RGB(0.45f, 0.6f, 0.9f), RGB(0.15f, 0.15f, 0.1f));
        k.relief.mtn = 4.0; k.relief.hill = 0.42;   // its ridge blend is the softest (0.4 + 0.6 rel), so it needs the most amplitude per grade
        // the base hue is the blend seen from afar; the ground is rock and sand, not teal
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.52f, 0.46f, 0.38f), 0.2f); mix(k, MAT_SAND, MM_BASE_MIX, SAND_TAN, 0.15f);
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FELISIAN;
        k.fog = 12000; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.06f), RGB(0.06f, 0.06f, 0.1f));
        rocks(k, 0.35, 0.9); k.snowAlbedo = 0.95;
        k.pressureLo = 0.7; k.pressureSpan = 0.6;
        k.rain = RAIN_CLOUDS; k.rainThr = 0.72; k.rainSpan = 0.25;
    }
    {   KindDef& k = K[PT_ROCKY];
        identity(k, PT_ROCKY, "ROCKY", "ROCKY", "medium size, rocky, creased, no atmosphere.", 2800, 6000, true, false, 2, 0.18);
        k.tags = KT_PLATES | KT_TWIN | KT_GLASSABLE;
        k.shapes = KS_INCISION | KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_PEDIMENTS | KS_BASALTIC | KS_BRITTLE | KS_FLOOD_LAVA | KS_IMPACTS | KS_SOILS;
        zones(k, 7, 18, 9, 12, 7);
        rule(k, WR_NEUTRON, 1.3); rule(k, WR_BLACK_HOLE, 1.3); rule(k, WR_PROTOSTAR, 1.2); rule(k, WR_BLUE_WHITE, 1.4); moons(k, 1.5); rule(k, WR_WOLF_RAYET, 1.3);
        colour(k, ch(0.55f, 0.1f), ch(0.47f), ch(0.40f));
        sentence(k, SR_ONE, "creased, rocky world of ridges, rifts and boulder fields");
        draw(k, GF_MOUNTAIN, 1500, 2000); draw(k, GF_HILL, 150, 200); draw(k, GF_BOULDERS, 0.2, 0.6); draw(k, GF_CRATER_DENSITY, 0.05, 0.1);
        k.color2 = tintOf(TINT_CONST, RGB(0.35f, 0.3f, 0.28f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.2; k.relief.mtn = 2.7;   // 2.4 read a 0.24 median with the benches
        k.ground = GR_ROCKY;
        crater(k, 20.0, false, 1, CD_REL, 0.6, false, true);
        k.fog = 45000;
        rocks(k, 1.6, 1.6);
    }
    {   KindDef& k = K[PT_THINATMO];
        identity(k, PT_THINATMO, "THIN ATMOSPHERE", "THIN ATMOS", "small, solid, thin atmosphere.", 2400, 4600, true, true, 2, 0.30);
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FRONTS | KT_DUST | KT_TWIN | KT_GLASSABLE | KT_EXOTIC_TWILIGHT;
        k.shapes = KS_INCISION | KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES | KS_FLOOD_LAVA |
                   KS_IMPACTS | KS_COLD_SEAS | KS_STORMS | KS_HAZE | KS_SOILS;
        zones(k, 0, 10, 14, 18, 0);
        rule(k, WR_DEAD, 0.3); rule(k, WR_BLACK_HOLE, 0); rule(k, WR_PROTOSTAR, 0.3); rule(k, WR_BLUE_WHITE, 1.2); rule(k, WR_MOON_OF_GIANT, 1.4);
        palette4(k, RGB(0.85f, 0.45f, 0.30f), RGB(0.55f, 0.75f, 0.45f), RGB(0.65f, 0.50f, 0.80f), RGB(0.60f, 0.62f, 0.66f));   // red/orange, green, purple, grey
        sentence(k, SR_DUNES, "world of thin air, red-brown dune seas and shield volcanoes", "world of thin air, canyons and dust plains under an exotic sky");
        draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 120, 200); draw(k, GF_CRATER_DENSITY, 0.15, 0.25); draw(k, GF_DUNE_AMP, 4, 14); draw(k, GF_ICE_CAP, 70, 15);
        draw(k, GF_CLOUD_COVER, 0.08, 0.2);
        k.color2 = tintOf(TINT_CONST, RGB(0.92f, 0.92f, 0.98f)); k.skyTint = tintOf(TINT_THIN_AIR);
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.15; k.relief.mtn = 2.3;
        mix(k, MAT_ROCK, MM_BASE_MIX, ROCK_GREY, 0.65f); mix(k, MAT_DUST, MM_BASE_MIX, DUST_WARM, 0.6f); mix(k, MAT_BASALT, MM_BASE_MIX, BASALT_DARK, 0.4f);
        mix(k, MAT_ICE, MM_SKY_MIX, RGB(0.85f, 0.88f, 0.95f), 0.15f);   // exotic colours are the point of the kind
        k.ground = GR_THIN_AIR; k.plainK = 0.6; k.gullyK = 0.6; k.drainArea = 2.2;
        crater(k, 0.2, true, 0.7, CD_REL, 0.7, true); crater(k, 0.05, true, 1, CD_REL, 0.6, true, true); crater(k, 8.0, false, 0.9, CD_REL, 0.5, true, true);
        crater(k, 1.5, false, 0.9, CD_REL, 0.4, false); crater(k, 0.3, false, 0.5, CD_ABS, 0.3, false); crater(k, 0.06, false, 0.4, CD_ABS, 0.25, false);
        k.sky = SKY_THIN; k.fog = 14000; k.skyDensity = 0.5; k.clouds = true;
        night(k, RGB(0.01f, 0.01f, 0.02f), RGB(0.03f, 0.02f, 0.03f));
        k.floorFamily = FAM_SAND; rocks(k, 1.2, 0.9); k.aurora = AU_THIN;
        k.pressureLo = 0.006; k.pressureSpan = 0.04; k.tempOffset = -25; k.wind = 25; k.dustThreshold = 0.35;
        k.events = EV_STORM_DEVILS;
    }
    {   KindDef& k = K[PT_GASGIANT];
        identity(k, PT_GASGIANT, "GAS GIANT", "GAS GIANT", "large, not consistent, covered with dense clouds.", 24000, 72000, false, true, 12, 0.55);
        k.density = 0.25; k.ringChance = 0.6;
        k.tags = KT_GIANT | KT_GAS_GIANT | KT_NO_SIGHTS;
        zones(k, 0, 0, 0, 28, 34);
        rule(k, WR_PULSAR, 0.3); rule(k, WR_NEUTRON, 0); rule(k, WR_BLACK_HOLE, 0); rule(k, WR_PROTOSTAR, 0.2); moons(k, 0); rule(k, WR_WOLF_RAYET, 0.4); rule(k, WR_SMALL, 0.2);
        palette4(k, RGB(0.92f, 0.78f, 0.55f), RGB(0.55f, 0.65f, 0.90f), RGB(0.85f, 0.60f, 0.45f), RGB(0.70f, 0.80f, 0.85f));
        sentence(k, SR_ONE, "of banded clouds and drifting storms, with no surface to land on");
        draw(k, GF_BAND_COUNT, 5, 9);
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.7f, 0.7f, 0.8f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
    }
    {   KindDef& k = K[PT_ICY];
        identity(k, PT_ICY, "ICY", "ICY", "small, solid, icy surface, no atmosphere.", 900, 2600, true, false, 2, 0.70);
        k.density = 0.55;
        k.tags = KT_FROZEN;
        k.shapes = KS_INCISION | KS_EXTENSION | KS_SCARPS | KS_BRITTLE | KS_FROST | KS_IMPACTS | KS_SPIRES | KS_HYDROTHERMAL | KS_COLD_SEAS;
        zones(k, 0, 0, 0, 18, 32);
        rule(k, WR_DEAD, 1, 15); rule(k, WR_PROTOSTAR, 0.6); rule(k, WR_BLUE_WHITE, 1.3); rule(k, WR_ORANGE_GIANT, 0.7); moons(k, 0.9); rule(k, WR_WOLF_RAYET, 0.5);
        colour(k, ch(0.80f), ch(0.88f), ch(1.00f));
        sentence(k, SR_FRACTURES, "icy world, its crust broken into plates by dark fractures", "icy world with a smooth, cratered crust and bright cryovolcanic domes");
        draw(k, GF_MOUNTAIN, 200, 400); draw(k, GF_HILL, 40, 60); draw(k, GF_FRACTURE, 0.03, 0.05); draw(k, GF_CRATER_DENSITY, 0.08, 0.15);
        k.color2 = tintOf(TINT_CONST, RGB(0.45f, 0.5f, 0.6f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.5; k.relief.flat = 0.14; k.relief.mtn = 1.8; k.relief.hill = 0.6;   // chaos plains between the cracks
        k.relief.cliffAmount = 0.5; k.relief.cliffStep = 0.6;
        mix(k, MAT_ICE, MM_BASE_MIX, ICE_BLUE, 0.6f); mix(k, MAT_ROCK, MM_CONST, RGB(0.42f, 0.42f, 0.50f));   // the dark rock of the cracks
        k.exoticLo = -60; k.exoticSpan = -80; k.exoticColour = RGB(0.32f, 0.28f, 0.42f);   // brine
        k.ground = GR_ICE; k.plainK = 0.5; k.gullyK = 0.4;
        crater(k, 0.15, true, 1, CD_REL, 0.5, false); crater(k, 6.0, false, 1, CD_REL, 0.4, false, true); crater(k, 0.8, false, 1.2, CD_REL, 0.3, false);
        crater(k, 0.15, false, 0.35, CD_ABS, 0.25, false);
        k.fog = 45000; k.floorFamily = FAM_SNOW;
        rocks(k, 0.3, 1.3);
    }
    {   KindDef& k = K[PT_QUARTZ];
        identity(k, PT_QUARTZ, "QUARTZ", "QUARTZ", "medium size, surface is mainly native quartz, oxygen atmosphere.", 3800, 6200, true, true, 3, 0.80);
        k.tags = KT_FRONTS | KT_TWIN | KT_CRYSTAL | KT_CLUSTERS | KT_FLAT;
        k.shapes = KS_LAYERED | KS_PEDIMENTS | KS_FLOOD_LAVA | KS_SPIRES | KS_HAZE;
        zones(k, 0, 6, 8, 5, 0);
        rule(k, WR_DEAD, 0); rule(k, WR_PROTOSTAR, 0); rule(k, WR_MOON_OF_WORLD, 0.3);
        colour(k, ch(0.95f), ch(0.85f, 0.1f), ch(0.90f));
        sentence(k, SR_ONE, "milky quartz world glinting under a thin oxygen sky");
        draw(k, GF_MOUNTAIN, 700, 700); draw(k, GF_HILL, 250, 250); draw(k, GF_CRATER_DENSITY, 0.05, 0.08); draw(k, GF_CLOUD_COVER, 0.12, 0.2);
        k.color2 = tintOf(TINT_CONST, RGB(1.0f, 0.95f, 0.98f)); k.skyTint = tintOf(TINT_CONST, RGB(0.98f, 0.95f, 0.98f));
        k.relief.ridgeOp = RO_SET; k.relief.ridge = 0.05; k.relief.hybrid = 0.8; k.relief.flat = 0.11; k.relief.mtn = 1.2; k.relief.hill = 0.34;   // milky mounds
        k.relief.cliffAmount = 0.5; k.relief.cliffStep = 0.6;
        mix(k, MAT_QUARTZ, MM_BASE_MIX, QUARTZ_PALE, 0.45f);
        k.famRep[FAM_SAND] = MAT_QUARTZ;
        k.ground = GR_QUARTZ;
        crater(k, 15.0, false, 1, CD_REL, 0.5, false, true);
        fixedSky(k, RGB(0.9f, 0.88f, 0.92f), RGB(1, 0.98f, 1));
        k.fog = 5000; k.skyDensity = 1.15; k.clouds = true;
        night(k, RGB(0.05f, 0.04f, 0.06f), RGB(0.1f, 0.08f, 0.1f));
        k.floorFamily = FAM_SAND; rocks(k, 0.5, 0.8);
        k.pressureLo = 0.4; k.pressureSpan = 0.4;
    }
    {   KindDef& k = K[PT_OCEAN];
        identity(k, PT_OCEAN, "OCEAN", "OCEAN", "medium size, one global sea under a thick sky, ice floes toward the poles, no land.", 5000, 7800, true, true, 2, 0.30);
        k.tags = KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FAUNA | KT_TWIN | KT_NO_SIGHTS;
        k.shapes = KS_OPEN_SEA | KS_FREEZE | KS_STORMS | KS_HAZE;
        zones(k, 0, 4, 11, 0, 0);
        rule(k, WR_DEAD, 0); rule(k, WR_PROTOSTAR, 0); rule(k, WR_ORANGE_GIANT, 2.0); moons(k, 0.3);
        colour(k, ch(0.25f), ch(0.45f, 0.1f), ch(0.80f));
        sentence(k, SR_ONE, "water world without land, ice floes drifting toward its poles");
        draw(k, GF_ICE_CAP, 40, 25); draw(k, GF_CLOUD_COVER, 0.4, 0.4); draw(k, GF_HILL, 2, 3);
        k.color2 = tintOf(TINT_CONST, RGB(0.15f, 0.35f, 0.7f)); k.skyTint = tintOf(TINT_CONST, RGB(0.5f, 0.65f, 0.9f));
        k.relief.flat = 0; k.relief.hill = 0; k.relief.mtn = 0;
        mix(k, MAT_WATER, MM_C2_MIX, RGB(), 0.3f); mix(k, MAT_ICE, MM_CONST, RGB(0.85f, 0.9f, 0.98f));
        k.ground = GR_OCEAN;
        k.seaSky = true; k.fog = 9000; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.06f), RGB(0.06f, 0.06f, 0.1f));
        k.pressureLo = 1.2; k.pressureSpan = 1.5;
    }
    {   KindDef& k = K[PT_METAL];
        identity(k, PT_METAL, "METAL", "METAL", "small, dense, an iron surface that glints in the sun, craterized, no atmosphere.", 1200, 3200, true, false, 1, 0.12);
        k.tags = KT_GLASSABLE;
        k.shapes = KS_INCISION | KS_SCARPS | KS_BRITTLE | KS_FLOOD_LAVA | KS_IMPACTS;
        zones(k, 12, 7, 0, 0, 0);
        rule(k, WR_DEAD, 1, 6); rule(k, WR_NEUTRON, 1, 14); rule(k, WR_BLACK_HOLE, 1, 10); rule(k, WR_CARBON_STAR, 1.3); rule(k, WR_WOLF_RAYET, 1, 8);
        colour(k, ch(0.45f), ch(0.42f), ch(0.40f, 0.05f));
        sentence(k, SR_ONE, "iron world, dark and dense, its craters glinting like hammered steel");
        draw(k, GF_MOUNTAIN, 600, 900); draw(k, GF_HILL, 80, 120); draw(k, GF_CRATER_DENSITY, 0.3, 0.4); draw(k, GF_CRATER_DEPTH, 1.0, 0.5);
        k.color2 = tintOf(TINT_CONST, RGB(0.6f, 0.58f, 0.55f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.3; k.relief.mtn = 2.3;
        mix(k, MAT_METAL, MM_BASE_MIX, METAL_GREY, 0.6f);
        k.famRep[FAM_ROCK] = MAT_METAL;
        k.ground = GR_METAL; k.plainK = 0.5; k.gullyK = 0.3;
        crater(k, 0.3, true, 0.7, CD_REL, 1, true); crater(k, 0.08, true, 1, CD_REL, 1, true, true); crater(k, 8.0, false, 1.2, CD_REL, 1, true, true);
        crater(k, 1.2, false, 1.4, CD_REL, 0.9, true); crater(k, 0.2, false, 0.8, CD_ABS, 0.7, true); crater(k, 0.04, false, 0.8, CD_ABS, 0.5, true);
        k.fog = 45000;
        rocks(k, 1.3, 1.2);
    }
    {   KindDef& k = K[PT_VOLCANIC];
        identity(k, PT_VOLCANIC, "VOLCANIC", "VOLCANIC", "small, tidally heated, sulphur plains and active cones, no atmosphere.", 1200, 2600, true, false, 0, 0.50);
        k.tags = KT_LAVA | KT_INNER_MOON | KT_FLAT;
        k.shapes = KS_EXTENSION | KS_BASALTIC | KS_FLOOD_LAVA | KS_PLUMES | KS_HYDROTHERMAL;
        rule(k, WR_MOON_OF_GIANT, 0, 14);   // tides heat a giant's inner moons
        colour(k, ch(0.85f), ch(0.75f, 0.1f), ch(0.30f));
        sentence(k, SR_ONE, "tidally heated moon of sulphur plains and active cones");
        draw(k, GF_VOLCANOES, 0.4, 0.5); draw(k, GF_LAVA_LEVEL, -400, -400); draw(k, GF_MOUNTAIN, 800, 900); draw(k, GF_HILL, 100, 150);
        k.color2 = tintOf(TINT_CONST, RGB(0.95f, 0.8f, 0.3f));
        k.relief.mtn = 1.9; k.relief.hill = 0.85;
        mix(k, MAT_SULPHUR, MM_BASE_MIX, SULPHUR_YELLOW, 0.5f); mix(k, MAT_BASALT, MM_CONST, RGB(0.30f, 0.27f, 0.25f));
        k.famRep[FAM_SAND] = MAT_SULPHUR; k.famRep[FAM_ROCK] = MAT_BASALT;
        k.ground = GR_VOLCANIC;
        k.lavaHorizon = true; k.horizonGlow = RGB(0.08f, 0.05f, 0.02f); k.fog = 30000;
        k.floorFamily = FAM_SAND; rocks(k, 0.8, 1.0);
        k.tempAdd = 60;
        k.events = EV_ERUPTIONS;
    }
    {   KindDef& k = K[PT_CARBON];
        identity(k, PT_CARBON, "CARBON", "CARBON", "medium size, black graphite plains with crusts of diamond, no atmosphere.", 2500, 5000, true, false, 1, 0.06);
        k.tags = KT_GLASSABLE | KT_CRYSTAL | KT_FLAT;
        k.shapes = KS_LAYERED | KS_SCARPS | KS_IMPACTS | KS_SPIRES | KS_COLD_SEAS;
        zones(k, 0, 0, 0, 0, 6);
        rule(k, WR_DEAD, 1, 8); rule(k, WR_CARBON_STAR, 3, 10); rule(k, WR_WOLF_RAYET, 0.5);   // the soot falls on every world
        colour(k, ch(0.18f), ch(0.17f), ch(0.17f));
        sentence(k, SR_ONE, "black world of graphite plains and diamond crusts");
        draw(k, GF_MOUNTAIN, 500, 700); draw(k, GF_HILL, 60, 90); draw(k, GF_CRATER_DENSITY, 0.15, 0.2);
        k.color2 = tintOf(TINT_CONST, RGB(0.9f, 0.95f, 1.0f));
        k.relief.mtn = 1.4; k.relief.hill = 0.32;
        mix(k, MAT_QUARTZ, MM_CONST, RGB(0.9f, 0.95f, 1.0f)); mix(k, MAT_GRAPHITE, MM_BASE_MIX, GRAPHITE, 0.5f);   // diamond crusts
        k.famRep[FAM_SAND] = MAT_QUARTZ; k.famRep[FAM_ROCK] = MAT_GRAPHITE;
        k.exoticLo = -150; k.exoticSpan = -300; k.exoticColour = RGB(0.07f, 0.06f, 0.06f);   // tar
        k.ground = GR_CARBON;
        crater(k, 0.15, true, 1, CD_REL, 0.6, false); crater(k, 5.0, false, 1, CD_REL, 0.5, false, true); crater(k, 0.4, false, 0.6, CD_ABS, 0.4, false);
        k.fog = 45000;
        rocks(k, 0.9, 1.1);
    }
    {   KindDef& k = K[PT_SUBSTELLAR];
        identity(k, PT_SUBSTELLAR, "SUBSTELLAR", "SUBSTELLAR", "very large, a failed star glowing dull red with its own heat, dense banded clouds.", 60000, 95000, false, true, 8, 0.30);
        k.density = 0.9;
        k.tags = KT_GIANT | KT_GLOWS | KT_NO_SIGHTS;
        zones(k, 0, 0, 0, 4, 4);   // brown dwarfs live in the cold outer system
        rule(k, WR_NEUTRON, 0.3); rule(k, WR_BLACK_HOLE, 0.5); rule(k, WR_PROTOSTAR, 0.5); moons(k, 0); rule(k, WR_WOLF_RAYET, 0.5); rule(k, WR_SMALL, 0.2);
        colour(k, ch(0.62f), ch(0.16f, 0.08f), ch(0.06f));
        sentence(k, SR_ONE, "failed star, banded and stormy, glowing dull red with the heat it was born with");
        draw(k, GF_BAND_COUNT, 3, 5);   // fewer, broader bands
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.5f, 0.4f, 0.4f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
    }
    {   KindDef& k = K[PT_COMET];
        identity(k, PT_COMET, "COMET", "COMET", "tiny, a mountain of ice and dust on a long eccentric orbit, trailing a tail near the star.", 2, 12, true, false, 0, 0.06);
        k.density = 0.3;
        k.tags = KT_FROZEN | KT_NO_SIGHTS;
        k.shapes = KS_BRITTLE | KS_FROST | KS_SPIRES;
        colour(k, ch(0.75f), ch(0.8f), ch(0.85f));
        sentence(k, SR_ONE, "mountain of ice and dust that grows a tail each time it dives past the star");
        draw(k, GF_MOUNTAIN, 150, 250); draw(k, GF_HILL, 30, 40); draw(k, GF_FRACTURE, 0.04, 0.05); draw(k, GF_CRATER_DENSITY, 0.3, 0.3);   // a battered icy hill
        k.color2 = tintOf(TINT_CONST, RGB(0.45f, 0.5f, 0.6f));
        k.relief.cliffAmount = 0;
        mix(k, MAT_ICE, MM_BASE_MIX, ICE_BLUE, 0.5f); mix(k, MAT_ROCK, MM_CONST, RGB(0.30f, 0.30f, 0.34f));
        k.ground = GR_ICE;
        crater(k, 0.15, true, 1, CD_REL, 0.5, false); crater(k, 6.0, false, 1, CD_REL, 0.4, false); crater(k, 0.8, false, 1.2, CD_REL, 0.3, false);
        crater(k, 0.15, false, 0.35, CD_ABS, 0.25, false);
        k.fog = 16000;   // cut down by the activity (dust in the coma) in computeEnvironment
        rocks(k, 1.4, 1.2);   // boulders of dirty ice
        k.events = EV_COMET_JETS;
    }
    {   KindDef& k = K[PT_COMPANION];
        identity(k, PT_COMPANION, "COMPANION STAR", "COMPANION", "a second sun bound to the primary, with worlds of its own.", 9000, 4600000, false, false, 4, 1.0);
        k.tags = KT_NO_SIGHTS;
        sentence(k, SR_ONE, "second sun");   // `describeBody` tells a companion by its class
    }
    {   KindDef& k = K[PT_EUROPAN];
        identity(k, PT_EUROPAN, "EUROPAN", "EUROPAN", "small, an ice shell over a hidden ocean, crossed by red cracks that vent water into space, no atmosphere.", 1200, 2600, true, false, 0, 0.65);
        k.density = 0.55;
        k.tags = KT_FLAT;
        k.shapes = KS_BRITTLE | KS_FROST | KS_SPIRES;
        zones(k, 0, 0, 0, 12, 14);
        rule(k, WR_NEUTRON, 0.5); rule(k, WR_BLACK_HOLE, 0.5); rule(k, WR_PROTOSTAR, 0); rule(k, WR_ORANGE_GIANT, 1.5); rule(k, WR_MOON_OF_GIANT, 2.0); rule(k, WR_WOLF_RAYET, 0.3);
        colour(k, ch(0.78f), ch(0.74f, -0.06f), ch(0.66f));
        sentence(
                k, SR_COIN, "shell of ice over a hidden ocean, its cracks stained red and venting water into space", "ice moon of long double ridges and broken rafts, geysers rising from the cracks");
        draw(k, GF_MOUNTAIN, 80, 60); draw(k, GF_HILL, 15, 15); draw(k, GF_CRATER_DENSITY, 0.03, 0.05);
        k.color2 = tintOf(TINT_CONST, RGB(0.55f, 0.32f, 0.2f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.3; k.relief.flat = 0.05; k.relief.hill = 0.3; k.relief.mtn = 0.5;   // a shell a few tens of metres in relief
        k.relief.cliffAmount = 0.5; k.relief.cliffStep = 0.6;
        // the lineae's stain is the dust
        mix(k, MAT_ICE, MM_BASE_MIX, RGB(0.90f, 0.88f, 0.82f), 0.15f); mix(k, MAT_DUST, MM_CONST, RGB(0.55f, 0.32f, 0.20f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.45f, 0.40f, 0.38f));
        k.ground = GR_EUROPAN; k.plainK = 0.3; k.gullyK = 0.3;
        crater(k, 0.1, true, 0.05, CD_ABS, 0.4, false); crater(k, 2.0, false, 0.08, CD_ABS, 0.3, false);
        k.fog = 45000; k.floorFamily = FAM_SNOW;
        rocks(k, 0.25, 1.0);   // ice blocks by the chaos
        k.events = EV_CRACK_VENTS; k.eventMaterial = MAT_DUST;
    }
    {   KindDef& k = K[PT_TECTONIC];
        identity(k, PT_TECTONIC, "TECTONIC", "TECTONIC", "medium size, seismically active: rift fissures, fountains of lava, quakes, a thin sulphurous sky.", 2400, 5200, true, true, 2, 0.28);
        k.tags = KT_LAVA | KT_PLATES | KT_TWIN | KT_INNER_MOON;
        k.shapes = KS_INCISION | KS_SCARPS | KS_BASALTIC | KS_FLOOD_LAVA | KS_PLUMES | KS_HYDROTHERMAL | KS_SOILS;
        zones(k, 6, 9, 3, 0, 0);
        rule(k, WR_PROTOSTAR, 1, 4); rule(k, WR_MOON_OF_GIANT, 1, 6);
        colour(k, ch(0.36f), ch(0.28f), ch(0.22f, 0.05f));
        sentence(k, SR_COIN, "restless world of rift valleys, fissures running with lava and ground that never stops shaking",
                "world torn by its own crust, fountains of lava along the rifts under a sulphur sky");
        draw(k, GF_MOUNTAIN, 1500, 1500); draw(k, GF_HILL, 150, 150); draw(k, GF_VOLCANOES, 0.3, 0.5); draw(k, GF_CRATER_DENSITY, 0.05, 0.05); draw(k, GF_CLOUD_COVER, 0.1, 0.2);
        k.color2 = tintOf(TINT_CONST, RGB(1.0f, 0.5f, 0.1f)); k.skyTint = tintOf(TINT_CONST, RGB(0.85f, 0.7f, 0.45f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.3; k.relief.mtn = 2.6; k.relief.hill = 0.7;   // fault-block ranges
        mix(k, MAT_BASALT, MM_CONST, RGB(0.24f, 0.21f, 0.19f)); mix(k, MAT_DUST, MM_CONST, RGB(0.50f, 0.46f, 0.42f)); mix(k, MAT_SULPHUR, MM_BASE_MIX, SULPHUR_YELLOW, 0.25f);
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.42f, 0.36f, 0.30f), 0.3f);   // ash plains pale over the dark basalt
        k.famRep[FAM_ROCK] = MAT_BASALT; k.accent[0] = MAT_SULPHUR;   // the vents' sulphur in its own yellow (it was the ash's grey on the ground and the globe)
        k.ground = GR_TECTONIC;
        crater(k, 30.0, false, 0.08, CD_ABS, 0.5, false, true);
        fixedSky(k, RGB(0.40f, 0.32f, 0.24f), RGB(0.80f, 0.60f, 0.38f));   // thin, sulphurous
        k.fog = 15000; k.skyDensity = 0.6; k.clouds = true;
        night(k, RGB(0.04f, 0.02f, 0.01f), RGB(0.09f, 0.04f, 0.02f));   // the glow of the fissures
        rocks(k, 1.0, 1.2); k.aurora = AU_SULPHUR;
        k.pressureLo = 0.08; k.pressureSpan = 0.3; k.tempOffset = 65; k.tempSwing = 14;   // the ground itself is warm
        k.events = EV_FOUNTAINS | EV_QUAKES | EV_FUMAROLES; k.eventMaterial = MAT_LAVA; k.rumble = 0.35; k.rumbleQuake = 0.6;
    }
    {   KindDef& k = K[PT_DESERT];
        identity(k, PT_DESERT, "DESERT", "DESERT", "medium size, dry, dune seas, mesas and salt pans under a dusty ochre sky.", 3000, 6400, true, true, 2, 0.40);
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FRONTS | KT_DUST | KT_PEOPLE | KT_DRY | KT_DEAD_SEAS | KT_TWIN | KT_GLASSABLE | KT_FLAT;
        k.shapes = KS_INCISION | KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_PEDIMENTS | KS_BASALTIC | KS_WIND | KS_EVAPORITES | KS_FLOOD_LAVA | KS_IMPACTS | KS_STORMS |
                   KS_HAZE | KS_SOILS;
        zones(k, 0, 11, 13, 0, 0);
        rule(k, WR_DEAD, 0.2); rule(k, WR_BLACK_HOLE, 0); rule(k, WR_PROTOSTAR, 0); moons(k, 0.3);
        colour(k, ch(0.80f), ch(0.62f, -0.08f), ch(0.38f));
        sentence(k, SR_DEAD_SEAS, "world of dry seabeds and dead river courses under a dusty sky, salt where its seas were", "dry world of dune seas, mesas and salt pans under a dusty sky",
                 "desert world of canyons and ergs, dust devils crossing its plains");
        draw(k, GF_MOUNTAIN, 1200, 1800); draw(k, GF_HILL, 120, 180); draw(k, GF_CRATER_DENSITY, 0.06, 0.1); draw(k, GF_DUNE_AMP, 10, 22); draw(k, GF_CLOUD_COVER, 0.05, 0.15);
        k.color2 = tintOf(TINT_CONST, RGB(0.95f, 0.85f, 0.6f)); k.skyTint = tintOf(TINT_CONST, RGB(0.85f, 0.72f, 0.5f));
        k.relief.mtn = 2.0; k.relief.hill = 0.65;
        mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.78f, 0.60f, 0.38f), 0.4f); mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.90f, 0.75f, 0.45f), 0.25f); mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.55f, 0.42f, 0.30f), 0.3f);
        k.famRep[FAM_SAND] = MAT_SAND;
        k.ground = GR_DESERT; k.drainArea = 2.2;
        crater(k, 0.1, true, 0.35, CD_REL, 0.35, false); crater(k, 3.0, false, 0.6, CD_REL, 0.3, false, true);
        fixedSky(k, RGB(0.62f, 0.55f, 0.45f), RGB(0.90f, 0.78f, 0.58f));   // a dusty ochre sky
        k.fog = 9000; k.skyDensity = 1.1; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.04f), RGB(0.06f, 0.05f, 0.05f));
        k.floorFamily = FAM_SAND; rocks(k, 0.9, 1.0); k.aurora = AU_DESERT;
        k.pressureLo = 0.3; k.pressureSpan = 0.7; k.tempOffset = 22; k.tempSwing = 32; k.wind = 18; k.dustThreshold = 0.45;   // hot days, cold nights; R-307: rarer dust storms
        k.events = EV_DEVILS;
    }
    {   KindDef& k = K[PT_HYDROCARBON];
        identity(k, PT_HYDROCARBON, "HYDROCARBON", "HYDROCARB", "medium size, cold, an opaque orange haze over seas of methane and dunes of tar sand.", 2200, 4400, true, true, 1, 0.22);
        k.tags = KT_OPAQUE_DECK | KT_DRAINAGE | KT_WET | KT_FLAT;
        k.shapes = KS_FROST | KS_WIND | KS_IMPACTS | KS_STORMS;
        zones(k, 0, 0, 0, 11, 6);
        rule(k, WR_DEAD, 0.3); rule(k, WR_NEUTRON, 0); rule(k, WR_BLACK_HOLE, 0); rule(k, WR_PROTOSTAR, 0); moons(k, 0.5);
        colour(k, ch(0.75f), ch(0.50f), ch(0.20f, 0.08f));
        sentence(k, SR_ONE, "cold world under an orange haze, seas of methane and dunes of tar");
        draw(k, GF_MOUNTAIN, 400, 400); draw(k, GF_HILL, 60, 60); draw(k, GF_CRATER_DENSITY, 0.04, 0.05); draw(k, GF_DUNE_AMP, 15, 25); draw(k, GF_CLOUD_COVER, 0.3, 0.3);
        draw(k, GF_LIQUID_LEVEL, 0, 0, DK_CONST);   // the methane sea at the reference level
        k.color2 = tintOf(TINT_CONST, RGB(0.3f, 0.18f, 0.06f)); k.skyTint = tintOf(TINT_CONST, RGB(0.85f, 0.55f, 0.2f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.4; k.relief.hybrid = 0.8; k.relief.flat = 0.08; k.relief.hill = 0.35; k.relief.mtn = 1.2;   // rounded hills, flat lowlands
        k.relief.cliffAmount = 0.5; k.relief.cliffStep = 0.6;
        mix(k, MAT_ICE, MM_CONST, RGB(0.62f, 0.50f, 0.32f)); mix(k, MAT_SAND, MM_CONST, RGB(0.36f, 0.24f, 0.12f)); mix(k, MAT_WATER, MM_CONST, RGB(0.14f, 0.10f, 0.05f));
        mix(k, MAT_CLOUD, MM_CONST, RGB(0.85f, 0.55f, 0.22f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.5f, 0.42f, 0.3f));   // methane (near black), tholin, the haze deck
        k.famRep[FAM_SAND] = MAT_SAND;
        k.ground = GR_HYDROCARBON; k.plainK = 0.7; k.gullyK = 0.8; k.drainArea = 1.5;
        crater(k, 40.0, false, 0.06, CD_ABS, 0.4, false);
        fixedSky(k, RGB(0.70f, 0.42f, 0.14f), RGB(0.92f, 0.62f, 0.28f));   // the orange haze deck
        k.fog = 2500; k.skyDensity = 1.6;   // an opaque deck: no cloud layer, no sun disc
        night(k, RGB(0.03f, 0.02f, 0.01f), RGB(0.05f, 0.03f, 0.02f));
        rocks(k, 0.3, 0.9);
        k.pressureLo = 1.4; k.pressureSpan = 0.3; k.tempOffset = -75; k.tempSwing = 3; k.wind = 3;   // the haze evens everything out
        k.rain = RAIN_DRIZZLE;   // a methane drizzle now and then
    }
    {   KindDef& k = K[PT_BOMBARDED];
        identity(k, PT_BOMBARDED, "BOMBARDED", "BOMBARDED", "small, airless, under a rain of meteorites: fresh craters and bright rays.", 700, 2200, true, false, 0, 0.18);
        k.tags = KT_GLASSABLE;
        k.shapes = KS_SCARPS | KS_BRITTLE | KS_IMPACTS;
        zones(k, 9, 5, 3, 3, 4);
        rule(k, WR_DEAD, 1, 6); rule(k, WR_NEUTRON, 1, 8); rule(k, WR_BLACK_HOLE, 1, 6); rule(k, WR_PROTOSTAR, 1, 20); moons(k, 1.5); rule(k, WR_WOLF_RAYET, 1, 25);
        colour(k, ch(0.40f), ch(0.38f), ch(0.36f, 0.05f));
        sentence(k, SR_ONE, "young airless world under a rain of meteorites, its craters fresh and rayed");
        draw(k, GF_MOUNTAIN, 600, 900); draw(k, GF_HILL, 80, 120); draw(k, GF_CRATER_DENSITY, 0.8, 0.2); draw(k, GF_CRATER_DEPTH, 1.0, 0.4);
        k.color2 = tintOf(TINT_CONST, RGB(0.3f, 0.3f, 0.3f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.6; k.relief.mtn = 1.5; k.relief.hill = 0.4;
        mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.46f, 0.45f, 0.44f), 0.3f); mix(k, MAT_ROCK, MM_CONST, RGB(0.44f, 0.42f, 0.40f));
        k.ground = GR_BOMBARDED; k.plainK = 0.5; k.gullyK = 0.3;
        crater(k, 0.3, true, 0.5, CD_ABS, 1.0, false); crater(k, 0.08, true, 0.7, CD_ABS, 1.0, false, true); crater(k, 8.0, false, 0.9, CD_ABS, 1.0, false, true);
        crater(k, 1.2, false, 1.0, CD_ABS, 0.9, false); crater(k, 0.2, false, 0.9, CD_ABS, 0.8, false); crater(k, 0.04, false, 0.8, CD_ABS, 0.6, false);
        k.fog = 45000; k.floorFamily = FAM_SAND;
        rocks(k, 1.8, 1.3);   // ejecta boulders
        k.events = EV_STRIKES;
    }
    {   KindDef& k = K[PT_ACIDIC];
        identity(k, PT_ACIDIC, "ACIDIC", "ACIDIC", "medium size, corrosive: seas of acid, bleached karst and sulphur crusts under a yellow-green sky.", 3800, 6600, true, true, 2, 0.55);
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_TWIN | KT_FLAT;
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_EVAPORITES | KS_PLUMES | KS_SPIRES | KS_HYDROTHERMAL | KS_STORMS | KS_HAZE | KS_SOILS;
        zones(k, 0, 9, 5, 0, 0);
        rule(k, WR_DEAD, 0); rule(k, WR_PROTOSTAR, 0); moons(k, 0.3);
        colour(k, ch(0.78f), ch(0.78f), ch(0.45f, 0.1f));
        sentence(k, SR_COIN, "corrosive world of acid seas and bleached karst under a yellow-green sky", "world whose rain eats the rock, sulphur crusts along its acid shores");
        draw(k, GF_MOUNTAIN, 900, 1200); draw(k, GF_HILL, 150, 200); draw(k, GF_CRATER_DENSITY, 0.04, 0.05); draw(k, GF_CLOUD_COVER, 0.5, 0.35);
        draw(k, GF_LIQUID_LEVEL, 0, 0, DK_CONST);   // the acid sea at the reference level
        k.color2 = tintOf(TINT_CONST, RGB(0.55f, 0.6f, 0.2f)); k.skyTint = tintOf(TINT_CONST, RGB(0.8f, 0.85f, 0.45f));
        k.relief.mtn = 1.8; k.relief.hill = 0.65;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.80f, 0.78f, 0.60f), 0.25f); mix(k, MAT_SULPHUR, MM_MIX, SULPHUR_YELLOW, 0.5f, RGB(0.9f, 0.85f, 0.5f));
        mix(k, MAT_WATER, MM_CONST, RGB(0.26f, 0.32f, 0.10f));   // bleached rock, dilute acid (dark, so the map and the ground agree)
        k.famRep[FAM_SAND] = MAT_SULPHUR;
        k.ground = GR_ACIDIC; k.plainK = 0.6; k.gullyK = 1.0; k.drainArea = 1.5;   // its rained-on plains read a 0.048 median grade against the flat class's 0.04
        crater(k, 20.0, false, 0.05, CD_ABS, 0.4, false);
        fixedSky(k, RGB(0.62f, 0.66f, 0.32f), RGB(0.88f, 0.88f, 0.50f));   // yellow-green murk
        k.fog = 2500; k.skyDensity = 1.3; k.clouds = true;
        night(k, RGB(0.03f, 0.04f, 0.02f), RGB(0.06f, 0.06f, 0.03f));
        rocks(k, 0.6, 1.0); k.aurora = AU_ACID;
        k.pressureLo = 2.0; k.pressureSpan = 3.0; k.tempOffset = 55; k.tempSwing = 6;
        k.rain = RAIN_CLOUDS; k.rainThr = 0.6; k.rainSpan = 0.3;   // the acid rains often
    }
    // GEN 16: the furnace worlds. No weight of their own: each is made from the worlds of the inner orbits that drew another kind
    // (`kindOf`: the sources below), every draw of the system made as the drawn kind's
    {   KindDef& k = K[PT_MAGMA_OCEAN];
        identity(k, PT_MAGMA_OCEAN, "MAGMA OCEAN", "MAGMA SEA", "large, locked to its star: a sea of magma under the sun, black glass on the night side, no atmosphere to speak of.", 4000, 9000, true,
                false, 1, 0.15);
        k.family = "furnace"; k.density = 1.3;   // K2-141b, 55 Cancri e: super-Earths, denser than the rock of an Earth
        k.tags = KT_LAVA;
        k.shapes = KS_EXTENSION | KS_BASALTIC | KS_FLOOD_LAVA | KS_PLUMES;
        k.salt = 0x3A6A0CEA00000001ULL; k.untilted = true;
        source(k, of(PT_MOLTEN), 360, 1e9, KB_PLANET, 0, 1, 0.9);   // the hottest molten worlds, locked to their star (the game's inner orbits reach 350-500 K)
        colour(k, ch(0.70f, 0.12f), ch(0.27f, 0.08f), ch(0.14f));
        sentence(k, SR_ONE, "world whose day side is a sea of magma, its night side black glass under a frost of rock");
        draw(k, GF_SEA_REACH, 60, 15); draw(k, GF_MOUNTAIN, 600, 800); draw(k, GF_HILL, 100, 150); draw(k, GF_VOLCANOES, 0.15, 0.3); draw(k, GF_CRATER_DENSITY, 0.04, 0.06);
        k.color2 = tintOf(TINT_CONST, RGB(1.0f, 0.5f, 0.1f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.1; k.relief.mtn = 2.4; k.relief.hill = 0.42;
        mix(k, MAT_BASALT, MM_BASE_MIX, RGB(0.22f, 0.20f, 0.19f), 0.12f); mix(k, MAT_GLASS, MM_CONST, RGB(0.11f, 0.12f, 0.14f));   // the crust; the night side's glass
        mix(k, MAT_SALT, MM_CONST, RGB(0.86f, 0.83f, 0.70f));   // the frost of condensed rock: sodium and silica
        k.famRep[FAM_ROCK] = MAT_BASALT; k.famRep[FAM_SNOW] = MAT_SALT; k.accent[0] = MAT_GLASS;   // the glass in its own hue (bank 2 is the lava)
        k.ground = GR_MAGMA;
        crater(k, 6.0, false, 0.5, CD_REL, 0.4, false, true); crater(k, 0.8, false, 0.6, CD_REL, 0.3, false);
        k.lavaHorizon = true; k.horizonGlow = RGB(0.16f, 0.07f, 0.03f); k.fog = 30000;
        rocks(k, 0.6, 1.0);
        k.tempAdd = 600; k.nightSideC = -150;   // the sea's heat on the day side; no air carries it round to the night
        k.events = EV_FOUNTAINS; k.eventMaterial = MAT_LAVA; k.rumble = 0.45;   // fire fountains along the shore
    }
    {   KindDef& k = K[PT_CHTHONIAN];
        identity(k, PT_CHTHONIAN, "CHTHONIAN", "CHTHONIAN", "large and heavy: the bare core of a giant that came too close to its star, a thin remnant air, a ring of what it lost.", 10000, 19000,
                true, true, 2, 0.12);
        k.family = "furnace"; k.density = 1.7; k.ringChance = 0.67;   // TOI-849b: forty Earth masses in three Earth radii; 2.7-5 g underfoot
        k.tags = KT_FLAT;
        k.shapes = KS_EXTENSION | KS_SCARPS | KS_FLOOD_LAVA | KS_IMPACTS | KS_PLUMES;
        k.salt = 0xC7470A1A00000002ULL;
        source(k, of(PT_MOLTEN, PT_METAL), 400, 1e9, KB_PLANET | KB_COMPANION_WORLD, LIVE_STARS, 0, 0.25);   // a Wolf-Rayet wind or a dead star's blast would take its air
        colour(k, ch(0.34f, 0.06f), ch(0.29f, 0.04f), ch(0.27f));
        sentence(k, SR_ONE, "bare core of a giant whose envelope its star boiled away, smooth and heavy under a thin pale sky");
        draw(k, GF_MOUNTAIN, 300, 400); draw(k, GF_HILL, 40, 60); draw(k, GF_CRATER_DENSITY, 0.04, 0.06);
        k.color2 = tintOf(TINT_CONST, RGB(0.30f, 0.28f, 0.27f)); k.skyTint = tintOf(TINT_CONST, RGB(0.75f, 0.74f, 0.76f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.5; k.relief.hybrid = 0.85; k.relief.flat = 0.07; k.relief.hill = 0.35; k.relief.mtn = 1.3;   // the weight keeps it low and smooth
        k.relief.cliffAmount = 0.4; k.relief.cliffStep = 0.6;
        mix(k, MAT_BASALT, MM_BASE_MIX, RGB(0.25f, 0.21f, 0.18f), 0.2f); mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.60f, 0.54f, 0.46f), 0.2f);   // iron basalt, the basins' dust
        mix(k, MAT_METAL, MM_CONST, RGB(0.50f, 0.44f, 0.38f));   // the iron where it shows
        k.famRep[FAM_ROCK] = MAT_BASALT; k.accent[0] = MAT_METAL;
        k.ground = GR_CHTHONIAN; k.plainK = 0.5; k.gullyK = 0.2;
        crater(k, 8.0, false, 0.6, CD_REL, 0.3, false, true); crater(k, 1.0, false, 0.5, CD_REL, 0.25, false);
        k.sky = SKY_THIN; k.fog = 25000; k.skyDensity = 0.55;   // thin and pale: a dark zenith over a pale rim
        night(k, RGB(0.01f, 0.01f, 0.015f), RGB(0.03f, 0.025f, 0.025f));
        rocks(k, 0.8, 1.2); k.aurora = AU_THIN;
        k.pressureLo = 0.01; k.pressureSpan = 0.08; k.tempOffset = 10; k.tempSwing = 40; k.wind = 6;
    }
    {   KindDef& k = K[PT_GLASS];
        identity(k, PT_GLASS, "GLASS", "GLASS", "small, airless and scorched: plains of black volcanic glass, fields of glass beads, tektites glinting.", 1500, 5000, true, false, 1, 0.10);
        k.family = "furnace";
        k.tags = KT_FLAT;
        k.shapes = KS_SCARPS | KS_BASALTIC | KS_BRITTLE | KS_FLOOD_LAVA | KS_IMPACTS;
        k.salt = 0x61A55B0D00000003ULL;
        source(k, of(PT_CRATERED, PT_BOMBARDED), 400, 1e9, KB_PLANET | KB_COMPANION_WORLD | KB_MOON, 0, 0, 0.4);   // flash-heated by flares and impacts
        colour(k, ch(0.20f, 0.06f), ch(0.21f, 0.05f), ch(0.22f, 0.04f));
        sentence(k, SR_ONE, "airless world of black glass plains and fields of glass beads, tektites glinting at a low sun");
        draw(k, GF_MOUNTAIN, 500, 800); draw(k, GF_HILL, 80, 120); draw(k, GF_CRATER_DENSITY, 0.25, 0.35); draw(k, GF_VOLCANOES, 0.2, 0.3);
        k.color2 = tintOf(TINT_CONST, RGB(0.15f, 0.16f, 0.18f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.6; k.relief.mtn = 1.5; k.relief.hill = 0.4;
        mix(k, MAT_GLASS, MM_BASE_MIX, RGB(0.08f, 0.09f, 0.10f), 0.15f); mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.36f, 0.34f, 0.33f), 0.3f);   // the sheets; the highlands' regolith
        mix(k, MAT_SAND, MM_CONST, RGB(0.42f, 0.25f, 0.12f)); mix(k, MAT_BASALT, MM_BASE_MIX, RGB(0.24f, 0.22f, 0.21f), 0.2f);   // the beads: the orange glass of the fire fountains
        k.famRep[FAM_ROCK] = MAT_BASALT; k.famRep[FAM_WATER] = MAT_GLASS; k.accent[0] = MAT_SAND;   // the glass in bank 2 (no sea, no lava), the beads their own hue
        k.ground = GR_GLASS; k.plainK = 0.5; k.gullyK = 0.3;
        crater(k, 0.55, true, 0.5, CD_REL, 1, true); crater(k, 0.18, true, 1, CD_REL, 1, true); crater(k, 0.06, true, 1, CD_REL, 1, true, true);
        crater(k, 12.0, false, 1.0, CD_REL, 1, true, true); crater(k, 2.5, false, 1.2, CD_REL, 1, true, true); crater(k, 0.3, false, 0.6, CD_ABS, 0.5, true);
        k.fog = 45000; k.floorFamily = FAM_SAND;
        rocks(k, 1.4, 0.6);   // tektites and shards of glass
    }
    {   KindDef& k = K[PT_STEAM];
        identity(k, PT_STEAM, "STEAM", "STEAM", "medium to large, past the runaway greenhouse: its ocean a white sky of steam, seas boiling at the poles, the lowlands lost in fog.", 5000, 11000, true,
                true, 2, 0.75);
        k.family = "furnace"; k.density = 0.9;
        k.tags = KT_OPAQUE_DECK | KT_PLATES | KT_DRAINAGE | KT_WET;
        k.shapes = KS_INCISION | KS_EXTENSION | KS_SCARPS | KS_BASALTIC | KS_HYDROTHERMAL | KS_STORMS;
        k.salt = 0x57EA3B0100000004ULL;
        source(k, of(PT_OCEAN), 330, 600, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN, 0, -1, 0.6);      // a wet world past the runaway limit (Kasting 1988); a world
        source(k, of(PT_FELISIAN), 330, 600, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN, 0, -1, 0.4);   // whose day is its year keeps a deck of cloud under its sun that
        source(k, of(PT_VENUSIAN), 345, 520, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN, 0, -1, 0.12);  // holds it back (Yang, Cowan and Abbot 2013)
        colour(k, ch(0.92f), ch(0.92f, 0.02f), ch(0.93f, 0.04f));
        sentence(k, SR_ONE, "world inside its runaway greenhouse, its ocean a white sky of steam, seas boiling at its poles and its lowlands lost in fog");
        draw(k, GF_MOUNTAIN, 1500, 2000); draw(k, GF_HILL, 150, 200); draw(k, GF_ICE_CAP, 58, 14); draw(k, GF_CLOUD_COVER, 1.0, 0, DK_CONST);   // the polar seas from this latitude
        k.color2 = tintOf(TINT_CONST, RGB(0.9f, 0.9f, 0.9f)); k.skyTint = tintOf(TINT_CONST, RGB(0.94f, 0.94f, 0.93f));
        k.relief.mtn = 2.4; k.relief.hill = 0.55;
        mix(k, MAT_CLOUD, MM_CONST, RGB(0.93f, 0.93f, 0.94f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.40f, 0.38f, 0.36f)); mix(k, MAT_MUD, MM_CONST, RGB(0.50f, 0.43f, 0.35f));
        mix(k, MAT_WATER, MM_CONST, RGB(0.40f, 0.52f, 0.54f)); mix(k, MAT_SALT, MM_CONST, RGB(0.90f, 0.88f, 0.82f));   // hot pale water, the vents' sinter
        k.famRep[FAM_SAND] = MAT_MUD; k.famRep[FAM_SNOW] = MAT_SALT;
        k.ground = GR_STEAM; k.plainK = 0.8; k.gullyK = 1.0;
        crater(k, 30.0, false, 0.04, CD_ABS, 0.4, false);
        fixedSky(k, RGB(0.56f, 0.56f, 0.57f), RGB(0.82f, 0.82f, 0.81f));   // inside the white: a grey overcast at the ground
        k.fog = 2200; k.skyDensity = 1.4; k.lowFogM = 0; k.lowFogSpan = 1200;   // the lowlands are the supercritical fog
        night(k, RGB(0.04f, 0.04f, 0.04f), RGB(0.06f, 0.06f, 0.06f));
        rocks(k, 0.7, 1.1);
        k.pressureLo = 100; k.pressureSpan = 200; k.tempOffset = 260; k.tempSwing = 3; k.wind = 4;   // hundreds of bars of steam
        k.rain = RAIN_DRIZZLE;   // scalding rain now and then
        k.events = EV_STEAM_BURSTS; k.eventMaterial = MAT_SALT;
    }
    {   KindDef& k = K[PT_SALT];
        identity(k, PT_SALT, "SALT", "SALT", "medium size: an ocean that dried away, white crusts to the horizon, brine pools and salt pillars under a thin dry sky.", 3000, 6500, true, true, 2, 0.70);
        k.family = "furnace";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FRONTS | KT_DUST | KT_FLAT;
        k.shapes = KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_PEDIMENTS | KS_WIND | KS_EVAPORITES | KS_STORMS;   // no great basin: the salt fills it
        k.salt = 0x5A17ED0000000005ULL;
        source(k, of(PT_DESERT), 300, 520, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN | KB_MOON, 0, 0, 0.3);   // the Messinian crisis, the Danakil, Mars' chloride basins
        colour(k, ch(0.90f), ch(0.87f, -0.05f), ch(0.82f, -0.08f));
        sentence(k, SR_ONE, "world whose ocean dried away, white salt to the horizon ringed by the bands of its last shores");
        // the old sea's level (metres)
        draw(k, GF_SEA_LEVEL, 100, 300); draw(k, GF_MOUNTAIN, 1000, 1400); draw(k, GF_HILL, 100, 150); draw(k, GF_CRATER_DENSITY, 0.04, 0.06); draw(k, GF_DUNE_AMP, 6, 10);
        draw(k, GF_CLOUD_COVER, 0.02, 0.05);
        k.color2 = tintOf(TINT_CONST, RGB(0.95f, 0.93f, 0.88f)); k.skyTint = tintOf(TINT_CONST, RGB(0.70f, 0.76f, 0.88f));
        k.relief.mtn = 1.8; k.relief.hill = 0.6;
        mix(k, MAT_SALT, MM_BASE_MIX, RGB(0.95f, 0.94f, 0.91f), 0.1f); mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.86f, 0.80f, 0.68f), 0.2f);   // halite; gypsum
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.62f, 0.56f, 0.50f), 0.3f); mix(k, MAT_POTASH, MM_CONST, RGB(0.76f, 0.46f, 0.42f));
        mix(k, MAT_BRINE, MM_BASE_MIX, RGB(0.58f, 0.62f, 0.36f), 0.2f);   // iron and sulphur in the brine
        k.famRep[FAM_SNOW] = MAT_SALT; k.accent[0] = MAT_BRINE; k.accent[1] = MAT_POTASH;
        k.ground = GR_SALT; k.drainArea = 2.2;
        crater(k, 0.1, true, 0.3, CD_REL, 0.3, false); crater(k, 3.0, false, 0.5, CD_REL, 0.3, false, true);
        fixedSky(k, RGB(0.52f, 0.60f, 0.76f), RGB(0.90f, 0.89f, 0.86f));   // thin, dry and bright over the white
        k.fog = 20000; k.skyDensity = 0.9; k.clouds = true;
        night(k, RGB(0.015f, 0.02f, 0.04f), RGB(0.05f, 0.05f, 0.06f));
        k.floorFamily = FAM_SNOW; rocks(k, 0.4, 0.9); k.aurora = AU_DESERT;
        k.pressureLo = 0.08; k.pressureSpan = 0.3; k.tempOffset = 20; k.tempSwing = 28; k.wind = 14; k.dustThreshold = 0.5;
        k.events = EV_STORM_DEVILS;
    }
    {   KindDef& k = K[PT_RUST];
        identity(k, PT_RUST, "RUST", "RUST", "medium size: its water lost to space and its oxygen left behind, every stone rusted red under a pale sky.", 3500, 7500, true, true, 2, 0.28);
        k.family = "furnace";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FRONTS | KT_DUST | KT_FLAT;
        k.shapes = KS_INCISION | KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_PEDIMENTS | KS_BASALTIC | KS_WIND | KS_EVAPORITES | KS_FLOOD_LAVA | KS_IMPACTS | KS_STORMS;
        k.salt = 0x2B57F00D00000006ULL;
        source(k, of(PT_DESERT), 190, 345, KB_PLANET | KB_TWIN | KB_MOON, of(STAR_RED_DWARF), 0, 0.3);   // Luger and Barnes 2015: a red dwarf's young glare
        source(k, of(PT_FELISIAN), 190, 345, KB_PLANET | KB_TWIN | KB_MOON, of(STAR_RED_DWARF), 0, 0.1);   // split the oceans of its habitable zone
        source(k, of(PT_THINATMO), 190, 345, KB_PLANET | KB_TWIN | KB_MOON, of(STAR_RED_DWARF), 0, 0.08);
        colour(k, ch(0.62f, 0.12f), ch(0.30f, 0.06f), ch(0.22f, 0.05f));
        sentence(k, SR_COIN, "world whose water was lost to space, its oxygen left behind and every stone rusted red", "red world of dry valleys and iron dust, its seas gone with their hydrogen");
        draw(k, GF_MOUNTAIN, 1200, 1800); draw(k, GF_HILL, 120, 180); draw(k, GF_CRATER_DENSITY, 0.06, 0.1); draw(k, GF_DUNE_AMP, 8, 16); draw(k, GF_CLOUD_COVER, 0.03, 0.1);
        k.color2 = tintOf(TINT_CONST, RGB(0.72f, 0.40f, 0.26f)); k.skyTint = tintOf(TINT_CONST, RGB(0.85f, 0.70f, 0.60f));
        k.relief.mtn = 2.0; k.relief.hill = 0.65;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.64f, 0.40f, 0.32f), 0.15f); mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.84f, 0.52f, 0.33f), 0.1f);   // purple-brown rock, red dust
        mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.78f, 0.45f, 0.28f), 0.1f); mix(k, MAT_SALT, MM_CONST, RGB(0.86f, 0.74f, 0.68f)); mix(k, MAT_BASALT, MM_BASE_MIX, RGB(0.28f, 0.16f, 0.14f), 0.2f);
        k.famRep[FAM_SAND] = MAT_SAND;
        k.ground = GR_DESERT; k.drainArea = 2.2;   // the desert's ground: dry valleys, mesas, ergs, playas
        crater(k, 0.1, true, 0.35, CD_REL, 0.35, false); crater(k, 3.0, false, 0.6, CD_REL, 0.3, false, true);
        fixedSky(k, RGB(0.80f, 0.71f, 0.65f), RGB(0.95f, 0.86f, 0.78f));   // pale over the red dust
        k.fog = 10000; k.skyDensity = 1.05; k.clouds = true;
        night(k, RGB(0.02f, 0.015f, 0.02f), RGB(0.06f, 0.04f, 0.04f));
        k.floorFamily = FAM_SAND; rocks(k, 0.9, 1.0);   // an oxygen air: its aurora green under red
        k.pressureLo = 0.4; k.pressureSpan = 2.0; k.tempOffset = 22; k.tempSwing = 26; k.wind = 14; k.dustThreshold = 0.45;
        k.events = EV_DEVILS;
    }
    // the living worlds. Made from the felisian worlds whose day is not their year (a locked world is P-03's) round the stars still
    // burning their hydrogen (a giant's are P-04's), the lichen world from the thin airs; none holds a people (their own would move every
    // people's hash). The five living grounds are the felisian layer by their rows: the climate (its warmth, its contrast from the equator
    // to the poles, its day's swing), the seas (their level and depth, the islands), the ice, the moisture, and two ways of their own (the
    // archean's microbes, the tundra's ice sheets); the lichen world is the thin air's ground with its crusts and mosses
    {   KindDef& k = K[PT_ARCHEAN];
        identity(k, PT_ARCHEAN, "ARCHEAN", "ARCHEAN", "medium size, young life: microbial mats on its flats, stromatolites in its shallows, a green sea under an orange haze.", 4800, 7600, true, true,
                3, 0.30);
        k.family = "living";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_TWIN | KT_MICROBIAL;   // no plants, no animals
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES |
                   // no flora to be giant or dead; no soils (no oxygen to redden them, no shells for chalk)
                   KS_FLOOD_LAVA | KS_HYDROTHERMAL | KS_OPEN_SEA | KS_FREEZE | KS_CONTINENTS | KS_STORMS | KS_HAZE;
        k.salt = 0xA2C4EA0E00000007ULL;
        source(k, of(PT_FELISIAN), 0, 1e9, ALL_BODIES, of(STAR_BLUE_GIANT, STAR_BLUE_WHITE), -1, 0.6, true);   // a short-lived star's worlds are young
        source(k, of(PT_FELISIAN), 0, 1e9, ALL_BODIES, of(STAR_YELLOW, STAR_ORANGE, STAR_RED_DWARF), -1, 0.07, true);   // a young star, by its hash: all its living worlds young
        colour(k, ch(0.72f, 0.06f), ch(0.52f, 0.06f), ch(0.32f, 0.05f));   // the pale orange dot (Arney et al. 2016)
        sentence(k, SR_COIN, "young world of green seas under an orange haze, its shallows banded with mats of microbes",
                "world of bare grey continents and stromatolite reefs, its only life the mats along its shores");
        k.greenhouseK = 38;   // a methane greenhouse over a faint young sun
        draw(k, GF_SEA_LEVEL, -0.05, 0.35); draw(k, GF_MOUNTAIN, 1200, 2400); draw(k, GF_HILL, 150, 250); draw(k, GF_ICE_CAP, 64, 18); draw(k, GF_SNOW_LINE, 2500, 2500);
        draw(k, GF_MOISTURE, 0, 0.2, DK_SYM); draw(k, GF_CLOUD_COVER, 0.25, 0.35); draw(k, GF_ISLANDS, 0.3, 0.25);   // small continents, island arcs
        k.color2 = tintOf(TINT_CONST, RGB(0.18f, 0.40f, 0.26f)); k.skyTint = tintOf(TINT_CONST, RGB(0.86f, 0.60f, 0.32f));
        k.relief.mtn = 3.6; k.relief.hill = 0.42;
        // bare grey and brown land, warm in the haze's light
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.46f, 0.44f, 0.42f), 0.3f); mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.58f, 0.54f, 0.47f), 0.3f); mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.50f, 0.45f, 0.39f), 0.3f);
        mix(k, MAT_WATER, MM_CONST, RGB(0.14f, 0.33f, 0.20f)); mix(k, MAT_STROMATOLITE, MM_CONST, RGB(0.58f, 0.54f, 0.40f));   // a sea green with iron; the domes' pale carbonate
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_MATS; k.accent[1] = MAT_STROMATOLITE;   // the mats' hue is the world's (`materialPalette`)
        k.ground = GR_FELISIAN;
        fixedSky(k, RGB(0.66f, 0.44f, 0.22f), RGB(0.92f, 0.70f, 0.44f));   // the organic haze of a methane air
        k.fog = 7000; k.skyDensity = 1.25; k.clouds = true;
        night(k, RGB(0.03f, 0.02f, 0.02f), RGB(0.07f, 0.05f, 0.04f));
        rocks(k, 0.6, 1.0); k.snowAlbedo = 0.95; k.aurora = AU_THIN;   // no oxygen's green
        k.pressureLo = 0.8; k.pressureSpan = 1.2;
        k.rain = RAIN_CLOUDS; k.rainThr = 0.72; k.rainSpan = 0.25;
    }
    {   KindDef& k = K[PT_HOTHOUSE];
        identity(k, PT_HOTHOUSE, "HOTHOUSE", "HOTHOUSE", "medium size, a hothouse climate: no ice at either pole, forest and swamp from coast to pole, warm shallow seas, storms.", 4800, 7600, true,
                true, 3, 0.33);
        k.family = "living";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FLORA | KT_FAUNA | KT_TWIN;
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_PEDIMENTS | KS_BASALTIC | KS_WIND | KS_EVAPORITES | KS_FLOOD_LAVA | KS_HYDROTHERMAL |
                   KS_OPEN_SEA | KS_CONTINENTS | KS_FLORA | KS_STORMS | KS_HAZE | KS_SOILS;   // no glaciers, no frost, no freeze
        k.salt = 0x407B0E5E00000008ULL;
        source(k, of(PT_FELISIAN), 285, 1e9, ALL_BODIES, MAIN_SEQUENCE, -1, 0.35);   // the warm edge: the Eocene's and the Cretaceous's climate
        colour(k, ch(0.26f, 0.10f), ch(0.58f, 0.06f), ch(0.48f, 0.15f));
        sentence(k, SR_COIN, "hothouse world without ice, forest and swamp from its coasts to its poles, warm seas and daily storms",
                "steaming living world of swamp and rainforest under a humid haze, no ice at either pole");
        k.greenhouseK = 33; k.latGradient = 0.45;   // the poles mild, the equator little warmer than a felisian world's
        draw(k, GF_SEA_LEVEL, -0.25, 0.5); draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 150, 250); draw(k, GF_ICE_CAP, 110, 0, DK_CONST); draw(k, GF_SNOW_LINE, 5000, 2500);
        draw(k, GF_MOISTURE, 0.08, 0.22); draw(k, GF_CLOUD_COVER, 0.45, 0.35);   // no ice cap; wet
        k.color2 = tintOf(TINT_CONST, RGB(0.15f, 0.45f, 0.62f)); k.skyTint = tintOf(TINT_DRAWN, RGB(0.55f, 0.66f, 0.86f), RGB(0.12f, 0.10f, 0.08f));
        k.relief.mtn = 4.0; k.relief.hill = 0.42;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.50f, 0.44f, 0.36f), 0.2f); mix(k, MAT_SAND, MM_BASE_MIX, SAND_TAN, 0.15f); mix(k, MAT_WATER, MM_CONST, RGB(0.10f, 0.34f, 0.50f));   // warm green-blue seas
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FELISIAN;
        k.fog = 8000; k.skyDensity = 1.15; k.clouds = true;   // a humid haze
        night(k, RGB(0.02f, 0.02f, 0.06f), RGB(0.06f, 0.06f, 0.1f));
        rocks(k, 0.3, 0.9); k.snowAlbedo = 0.95;
        k.pressureLo = 1.0; k.pressureSpan = 0.8; k.wind = 12;
        k.rain = RAIN_CLOUDS; k.rainThr = 0.6; k.rainSpan = 0.25;   // rain most afternoons
        k.daySwing = 6;
        k.favourTrait = TR_GIANT_FLORA; k.favourChance = 0.35;
    }
    {   KindDef& k = K[PT_SUPERHABITABLE];
        identity(k, PT_SUPERHABITABLE, "SUPERHABITABLE", "SUPERHABIT", "large, warmer and wetter than a felisian world: shallow seas, a thousand islands, a thick air, life everywhere.", 7000, 9500,
                true, true, 3, 0.30);
        k.family = "living"; k.density = 1.1;   // Heller and Armstrong 2014: two or three Earth masses round an orange dwarf
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FLORA | KT_FAUNA | KT_TWIN;
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES |
                   KS_FLOOD_LAVA | KS_HYDROTHERMAL | KS_OPEN_SEA | KS_FLORA | KS_STORMS | KS_HAZE | KS_SOILS;   // no one great continent, no snowball
        k.salt = 0x5E9E2AB100000009ULL;
        source(k, of(PT_FELISIAN), 230, 330, KB_PLANET | KB_COMPANION_WORLD, of(STAR_ORANGE), -1, 0.55);   // the commonest living world of an orange dwarf
        colour(k, ch(0.22f, 0.10f), ch(0.58f, 0.08f), ch(0.62f, 0.15f));
        sentence(k, SR_COIN, "large living world of shallow turquoise seas and a thousand forested islands under a thick air",
                "warm wet world of archipelagos and wide shelves, reefs and forest everywhere");
        k.greenhouseK = 45; k.seaDepth = 0.12;   // a little warmer; seas of shelves (half of them under 300 m)
        draw(k, GF_SEA_LEVEL, 0.12, 0.14); draw(k, GF_MOUNTAIN, 1200, 1800); draw(k, GF_HILL, 120, 200); draw(k, GF_ICE_CAP, 74, 14); draw(k, GF_SNOW_LINE, 3500, 2500);
        // more sea, low old mountains, the land in archipelagos
        draw(k, GF_MOISTURE, 0.06, 0.18); draw(k, GF_CLOUD_COVER, 0.32, 0.35); draw(k, GF_ISLANDS, 0.4, 0.3); draw(k, GF_CONT_SCALE, 0.3, 0.15);
        k.color2 = tintOf(TINT_CONST, RGB(0.12f, 0.50f, 0.62f)); k.skyTint = tintOf(TINT_DRAWN, RGB(0.42f, 0.58f, 0.88f), RGB(0.12f, 0.12f, 0.08f));
        k.relief.mtn = 3.2; k.relief.hill = 0.40;
        // reef sand; turquoise
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.52f, 0.46f, 0.38f), 0.2f); mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.88f, 0.82f, 0.64f), 0.12f); mix(k, MAT_WATER, MM_CONST, RGB(0.06f, 0.36f, 0.50f));
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FELISIAN;
        k.fog = 10000; k.skyDensity = 1.2; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.06f), RGB(0.06f, 0.06f, 0.1f));
        rocks(k, 0.3, 0.9); k.snowAlbedo = 0.95;
        k.pressureLo = 1.5; k.pressureSpan = 1.5;   // a thick air
        k.rain = RAIN_CLOUDS; k.rainThr = 0.66; k.rainSpan = 0.25;
        k.daySwing = 7;
    }
    {   KindDef& k = K[PT_STEPPE];
        identity(k, PT_STEPPE, "STEPPE", "STEPPE", "medium size, a dry living world: grass to the horizon, small seas and salt lakes, forest only along the rivers.", 4800, 7600, true, true, 3, 0.38);
        k.family = "living";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FLORA | KT_FAUNA | KT_TWIN;
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES |
                   KS_FLOOD_LAVA | KS_HYDROTHERMAL | KS_CONTINENTS | KS_FLORA | KS_STORMS | KS_HAZE | KS_SOILS;   // no open sea
        k.salt = 0x57E99E020000000AULL;   // pinned: the home world (Heilya II, 268 K) stays felisian
        source(k, of(PT_FELISIAN), 262, 1e9, ALL_BODIES, MAIN_SEQUENCE, -1, 0.25);   // Abe et al. 2011: a land world keeps its water nearer its star than an ocean world
        colour(k, ch(0.55f, 0.12f), ch(0.55f, 0.05f), ch(0.38f, 0.10f));
        sentence(k, SR_COIN, "dry living world of grassland to the horizon, small inland seas and salt lakes, forest only along its rivers",
                "world of steppe and savanna under a wide sky, its seas small and salt, its trees along the rivers");
        k.greenhouseK = 33;
        draw(k, GF_SEA_LEVEL, -0.65, 0.25); draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 150, 250); draw(k, GF_ICE_CAP, 68, 16); draw(k, GF_SNOW_LINE, 3000, 2500);
        draw(k, GF_MOISTURE, -0.1, 0.08); draw(k, GF_CLOUD_COVER, 0.12, 0.2);   // small seas; dry
        k.color2 = tintOf(TINT_CONST, RGB(0.2f, 0.45f, 0.7f)); k.skyTint = tintOf(TINT_DRAWN, RGB(0.50f, 0.64f, 0.90f), RGB(0.12f, 0.10f, 0.06f));
        k.relief.mtn = 4.0; k.relief.hill = 0.45;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.56f, 0.48f, 0.38f), 0.2f); mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.84f, 0.72f, 0.50f), 0.15f);
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FELISIAN;
        k.fog = 16000; k.skyDensity = 0.95; k.clouds = true;   // dry clear air
        night(k, RGB(0.02f, 0.02f, 0.06f), RGB(0.06f, 0.06f, 0.1f));
        rocks(k, 0.45, 0.9); k.snowAlbedo = 0.95;
        k.pressureLo = 0.7; k.pressureSpan = 0.6; k.wind = 16;
        k.rain = RAIN_CLOUDS; k.rainThr = 0.8; k.rainSpan = 0.2;
        k.daySwing = 18;   // big swings: little water to hold the heat
        k.grassBand = 0.12;   // the grass from the desert's edge to the rivers' forests
        k.fauna = FA_HERDS;
    }
    {   KindDef& k = K[PT_TUNDRA];
        identity(k, PT_TUNDRA, "TUNDRA", "TUNDRA", "medium size, in an ice age: ice sheets down to the middle latitudes, taiga and tundra below them, sea ice and dry cold winds.", 4800, 7600, true,
                true, 3, 0.45);
        k.family = "living";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FLORA | KT_FAUNA | KT_TWIN | KT_ICE_SHEETS;
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES |
                   KS_FLOOD_LAVA | KS_HYDROTHERMAL | KS_OPEN_SEA | KS_FREEZE | KS_CONTINENTS | KS_FLORA | KS_STORMS | KS_HAZE | KS_SOILS;
        k.salt = 0x70D2A0000000000BULL;
        source(k, of(PT_FELISIAN), 212, 248, ALL_BODIES, MAIN_SEQUENCE, -1, 0.4);   // the cold edge: the last glacial maximum
        colour(k, ch(0.58f, 0.10f), ch(0.66f, 0.05f), ch(0.70f, 0.08f));
        sentence(k, SR_COIN, "world in an ice age, ice sheets down to its middle latitudes and taiga and tundra below them",
                "cold living world of ice sheets, sea ice and wide tundra swept by dry winds");
        k.greenhouseK = 22; k.latGradient = 1.1;   // colder, and the poles colder still against the equator (a glacial world's)
        draw(k, GF_SEA_LEVEL, -0.45, 0.5); draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 150, 250); draw(k, GF_ICE_CAP, 44, 8); draw(k, GF_SNOW_LINE, 1500, 1500);
        draw(k, GF_MOISTURE, -0.1, 0.16); draw(k, GF_CLOUD_COVER, 0.2, 0.3);   // the seas low (their water in the ice), the sheets to 44-52 degrees, glaciers in every range
        k.color2 = tintOf(TINT_CONST, RGB(0.25f, 0.42f, 0.62f)); k.skyTint = tintOf(TINT_DRAWN, RGB(0.55f, 0.68f, 0.92f), RGB(0.10f, 0.10f, 0.06f));
        k.relief.mtn = 4.0; k.relief.hill = 0.42;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.50f, 0.47f, 0.42f), 0.2f); mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.70f, 0.64f, 0.50f), 0.15f);   // outwash and loess
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FELISIAN;
        k.fog = 14000; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.06f), RGB(0.06f, 0.06f, 0.1f));
        rocks(k, 0.45, 1.0); k.snowAlbedo = 0.95;
        k.pressureLo = 0.7; k.pressureSpan = 0.6; k.wind = 20;   // the ice sheets' dry winds
        k.rain = RAIN_CLOUDS; k.rainThr = 0.76; k.rainSpan = 0.25;
        k.daySwing = 8; k.floraFamily = 1;   // the taiga's conifers
        k.fauna = FA_COLD;
    }
    {   KindDef& k = K[PT_LICHEN];
        identity(k, PT_LICHEN, "LICHEN", "LICHEN", "small, cold and dry: bare rock and gravel crusted with lichen, moss in the wet hollows, nothing taller than a hand under a thin clear sky.", 2400,
                4600, true, true, 2, 0.28);
        k.family = "living";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FRONTS | KT_DUST | KT_TWIN;   // the thin air's dry valleys; no plants to stand, no animals
        k.shapes = KS_INCISION | KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES | KS_FLOOD_LAVA |
                   KS_IMPACTS | KS_STORMS | KS_SOILS;   // the thin air's without a haze or a sea of something else
        k.salt = 0x11C4E0000000000CULL;
        source(k, of(PT_THINATMO), 205, 260, ALL_BODIES, MAIN_SEQUENCE, 0, 0.3);   // the Dry Valleys' life (Friedmann 1982), the first land life's crusts
        colour(k, ch(0.52f, 0.08f), ch(0.48f, 0.06f), ch(0.42f, 0.06f));
        sentence(k, SR_COIN, "cold dry world of bare rock and gravel, lichen crusting its stones and moss in its wet hollows",
                "world of thin clear air where nothing grows taller than a hand, crusts of lichen orange and black on its rocks");
        draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 120, 200); draw(k, GF_CRATER_DENSITY, 0.1, 0.2); draw(k, GF_DUNE_AMP, 3, 8); draw(k, GF_ICE_CAP, 66, 14);
        draw(k, GF_CLOUD_COVER, 0.08, 0.2);
        k.color2 = tintOf(TINT_CONST, RGB(0.92f, 0.92f, 0.98f)); k.skyTint = tintOf(TINT_CONST, RGB(0.52f, 0.64f, 0.88f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.15; k.relief.mtn = 2.3;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.50f, 0.46f, 0.42f), 0.4f); mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.58f, 0.53f, 0.46f), 0.35f); mix(k, MAT_BASALT, MM_BASE_MIX, BASALT_DARK, 0.4f);
        mix(k, MAT_ICE, MM_CONST, RGB(0.86f, 0.90f, 0.96f)); mix(k, MAT_CRUST, MM_CONST, RGB(0.78f, 0.45f, 0.14f)); mix(k, MAT_MOSS, MM_CONST, RGB(0.30f, 0.40f, 0.15f));   // orange crusts, olive moss
        k.accent[0] = MAT_CRUST; k.accent[1] = MAT_MOSS;
        k.ground = GR_LICHEN; k.plainK = 0.6; k.gullyK = 0.6; k.drainArea = 2.2;
        crater(k, 0.2, true, 0.7, CD_REL, 0.7, true); crater(k, 0.05, true, 1, CD_REL, 0.6, true, true); crater(k, 8.0, false, 0.9, CD_REL, 0.5, true, true);
        crater(k, 1.5, false, 0.9, CD_REL, 0.4, false); crater(k, 0.3, false, 0.5, CD_ABS, 0.3, false); crater(k, 0.06, false, 0.4, CD_ABS, 0.25, false);
        k.sky = SKY_THIN; k.fog = 20000; k.skyDensity = 0.55; k.clouds = true;
        night(k, RGB(0.01f, 0.01f, 0.025f), RGB(0.03f, 0.03f, 0.05f));
        k.floorFamily = FAM_SAND; rocks(k, 1.2, 0.9); k.aurora = AU_THIN;
        k.pressureLo = 0.03; k.pressureSpan = 0.12; k.tempOffset = 8; k.tempSwing = 24; k.wind = 18; k.dustThreshold = 0.5;   // thicker than a thin air's: water melts on a summer noon
        k.events = EV_STORM_DEVILS;
    }
    // the locked and the deep. The red dwarfs' locked worlds (an eyeball where it is cold, a terminator world where it is warm; a world
    // whose twin takes its lock is made again as one whose day is not its year: `generate`), the water worlds (a hycean's deep sea, an ammonia
    // sea where water would freeze, a snowball where the living world froze to its equator) and the warm rock of the dead stars and the brown
    // dwarfs (life on the ground's heat). None holds a people
    {   KindDef& k = K[PT_EYEBALL];
        identity(k, PT_EYEBALL, "EYEBALL", "EYEBALL", "medium size, locked to a red dwarf: frozen over but for one round open sea under the fixed sun, the pupil of an eye.", 4800, 7800, true, true, 2,
                0.55);
        k.family = "deep";
        k.tags = KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FAUNA | KT_TWIN | KT_NO_SIGHTS | KT_FIXED_SUN;   // the ocean's, its swimmers in the pupil
        k.shapes = KS_STORMS | KS_HAZE;
        k.salt = 0xE7EBA1110000000DULL; k.untilted = true;   // pinned: the regress's Wyariothmai II and III stay oceans; the tides took its tilt with its day (the sun stands over the pupil)
        source(k, of(PT_OCEAN), 200, 290, KB_PLANET, of(STAR_RED_DWARF), 1, 0.6);      // Pierrehumbert 2011: a sea locked to a red dwarf
        source(k, of(PT_FELISIAN), 200, 232, KB_PLANET, of(STAR_RED_DWARF), 1, 0.4);   // freezes over but for the sea under its fixed sun
        source(k, of(PT_ICY), 165, 205, KB_PLANET, of(STAR_RED_DWARF), 1, 0.4);
        colour(k, ch(0.80f, 0.04f), ch(0.84f, 0.03f), ch(0.90f, 0.02f));
        sentence(k, SR_ONE, "world locked to its red sun and frozen over but for one round sea under it, the pupil of an eye of ice");
        draw(k, GF_SEA_REACH, 24, 30); draw(k, GF_CLOUD_COVER, 0.3, 0.35); draw(k, GF_HILL, 2, 3);   // the pupil's reach (degrees), the swell
        k.color2 = tintOf(TINT_CONST, RGB(0.15f, 0.35f, 0.7f)); k.skyTint = tintOf(TINT_CONST, RGB(0.55f, 0.65f, 0.88f));
        k.relief.flat = 0; k.relief.hill = 0; k.relief.mtn = 0;
        mix(k, MAT_WATER, MM_C2_MIX, RGB(), 0.3f); mix(k, MAT_ICE, MM_CONST, RGB(0.78f, 0.86f, 0.96f)); mix(k, MAT_SNOW, MM_CONST, RGB(0.93f, 0.94f, 0.98f));
        k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_EYEBALL;
        k.seaSky = true; k.fog = 9000; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.05f), RGB(0.05f, 0.05f, 0.09f));
        k.floorFamily = FAM_SNOW; k.snowAlbedo = 0.92;
        k.pressureLo = 0.8; k.pressureSpan = 1.2; k.tempOffset = -1.8; k.tempSwing = 34; k.wind = 14;   // the sea freezes at the pupil's edge
        k.reachClimate = true;
    }
    {   KindDef& k = K[PT_TERMINATOR];
        identity(k, PT_TERMINATOR, "TERMINATOR", "TERMINATOR", "medium size, locked: a desert under the fixed sun, an ice sheet over the night side, a ring of rivers and life between.", 4800, 7600,
                true, true, 2, 0.42);
        k.family = "deep";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_FLORA | KT_FAUNA | KT_TWIN | KT_ICE_SHEETS | KT_FIXED_SUN;
        k.shapes = KS_INCISION | KS_LAYERED | KS_SOLUTION | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_EVAPORITES |
                   KS_FLOOD_LAVA | KS_HYDROTHERMAL | KS_CONTINENTS | KS_FLORA | KS_STORMS | KS_SOILS;   // no open sea round it, no freeze to the tropics, no haze
        k.salt = 0x7E2A17A80000000EULL; k.untilted = true;   // pinned: the regress's Wyariothmai I and Feeyeithis II and III stay felisian
        source(k, of(PT_FELISIAN), 232, 1e9, KB_PLANET, 0, 1, 0.5);   // Lobo et al. 2023: a locked world with little water keeps a temperate
        source(k, of(PT_DESERT), 220, 1e9, KB_PLANET, 0, 1, 0.45);    // ring between its scorched day and its frozen night
        colour(k, ch(0.72f, 0.08f), ch(0.64f, 0.05f), ch(0.52f, 0.06f));
        sentence(k, SR_COIN, "world locked to its sun, a desert of dunes and salt under it and an ice sheet over its night, a ring of rivers and life between",
                 "world of a sun that never moves, rivers of meltwater running from the ice of its night into the desert of its day, a green ring along them");
        k.greenhouseK = 10; k.subContrast = 55; k.dayDry = 1.0; k.ringWet = 0.35; k.daySeasDry = true; k.sheetByCold = true;   // a drier air's greenhouse: the ring near the terminator
        draw(k, GF_SEA_LEVEL, -0.6, 0.3); draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 150, 250); draw(k, GF_ICE_CAP, 95, 0, DK_CONST); draw(k, GF_SNOW_LINE, 3500, 2500);
        draw(k, GF_MOISTURE, 0, 0.2, DK_SYM); draw(k, GF_CLOUD_COVER, 0.15, 0.3);   // little water: small seas; no polar caps (its poles are on the ring)
        k.color2 = tintOf(TINT_CONST, RGB(0.2f, 0.42f, 0.68f)); k.skyTint = tintOf(TINT_DRAWN, RGB(0.50f, 0.62f, 0.90f), RGB(0.12f, 0.10f, 0.06f));
        k.relief.mtn = 4.0; k.relief.hill = 0.42;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.56f, 0.47f, 0.38f), 0.2f); mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.86f, 0.74f, 0.52f), 0.15f); mix(k, MAT_SALT, MM_CONST, RGB(0.92f, 0.90f, 0.84f));
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FELISIAN;
        k.fog = 14000; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.06f), RGB(0.06f, 0.06f, 0.1f));
        rocks(k, 0.45, 0.9); k.snowAlbedo = 0.95;
        k.pressureLo = 0.6; k.pressureSpan = 0.8; k.wind = 16;
        k.rain = RAIN_CLOUDS; k.rainThr = 0.78; k.rainSpan = 0.22;
        k.daySwing = 0;   // the sun never moves: the climate is the place's
    }
    {   KindDef& k = K[PT_HYCEAN];
        identity(k, PT_HYCEAN, "HYCEAN", "HYCEAN", "large: one deep warm ocean under a hydrogen sky, no land, a pale hazy horizon.", 10000, 16500, true, true, 3, 0.45);
        k.family = "deep"; k.density = 0.6;   // Madhusudhan et al. 2021: two to two and a half Earth radii of rock and water under hydrogen
        k.tags = KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_TWIN | KT_NO_SIGHTS;   // no swimmers: the sheet says what is seen
        k.shapes = KS_STORMS | KS_HAZE;
        k.salt = 0x4CEA40000000000FULL;
        source(k, of(PT_OCEAN), 250, 400, KB_PLANET | KB_COMPANION_WORLD, 0, 0, 0.35);
        colour(k, ch(0.80f, 0.05f), ch(0.80f, 0.04f), ch(0.72f, 0.06f));
        sentence(k, SR_ONE, "large world of one deep warm ocean under a hydrogen sky, its horizon lost in a cream-white haze");
        draw(k, GF_ICE_CAP, 110, 0, DK_CONST); draw(k, GF_CLOUD_COVER, 0.5, 0.4); draw(k, GF_HILL, 3, 4);   // no floes; a long swell
        k.color2 = tintOf(TINT_CONST, RGB(0.08f, 0.30f, 0.34f)); k.skyTint = tintOf(TINT_CONST, RGB(0.92f, 0.88f, 0.76f));
        k.relief.flat = 0; k.relief.hill = 0; k.relief.mtn = 0;
        mix(k, MAT_WATER, MM_CONST, RGB(0.06f, 0.24f, 0.26f)); mix(k, MAT_ICE, MM_CONST, RGB(0.85f, 0.9f, 0.96f));   // a dark teal sea
        k.ground = GR_OCEAN; k.evenClimate = true;
        fixedSky(k, RGB(0.80f, 0.78f, 0.68f), RGB(0.95f, 0.93f, 0.86f));   // the hydrogen's haze, cream-white
        k.fog = 3500; k.skyDensity = 1.45; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.02f), RGB(0.05f, 0.05f, 0.04f));
        k.pressureLo = 20; k.pressureSpan = 80; k.tempOffset = 30; k.tempSwing = 4; k.wind = 8;   // tens of bars of hydrogen even out the day
        k.rain = RAIN_CLOUDS; k.rainThr = 0.6; k.rainSpan = 0.3;
    }
    {   KindDef& k = K[PT_AMMONIA];
        identity(k, PT_AMMONIA, "AMMONIA", "AMMONIA", "medium size, cold: seas of ammonia water that never freeze, slabs of ice drifting on them, frost on the hills, a yellow-brown haze.", 3000, 7000,
                true, true, 1, 0.45);
        k.family = "deep"; k.density = 0.6;   // ice and rock
        k.tags = KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_FLAT | KT_FROZEN;
        k.shapes = KS_INCISION | KS_BRITTLE | KS_FROST | KS_IMPACTS | KS_HYDROTHERMAL | KS_STORMS | KS_HAZE;
        k.salt = 0xA3304A0000000010ULL;
        source(k, of(PT_OCEAN), 195, 232, ALL_BODIES, 0, 0, 0.35);        // ammonia and water stay liquid together to 176 K (Titan's buried
        source(k, of(PT_HYDROCARBON), 150, 212, ALL_BODIES, 0, 0, 0.25);  // ocean holds it): the cold seas of the water worlds, the methane
        source(k, of(PT_ICY), 165, 232, ALL_BODIES, 0, 0, 0.08);          // worlds' warmer kin, the ice worlds that kept an air
        colour(k, ch(0.70f, 0.06f), ch(0.62f, 0.05f), ch(0.44f, 0.06f));
        sentence(k, SR_ONE, "cold world under a yellow-brown haze, seas of ammonia water that never freeze and slabs of ice drifting on them");
        draw(k, GF_MOUNTAIN, 500, 600); draw(k, GF_HILL, 60, 80); draw(k, GF_CRATER_DENSITY, 0.03, 0.05); draw(k, GF_SNOW_LINE, 250, 500); draw(k, GF_CLOUD_COVER, 0.35, 0.3);
        draw(k, GF_LIQUID_LEVEL, 0, 0, DK_CONST);   // the sea at the reference level; the frost over the snow line (metres)
        k.color2 = tintOf(TINT_CONST, RGB(0.40f, 0.32f, 0.16f)); k.skyTint = tintOf(TINT_CONST, RGB(0.82f, 0.66f, 0.38f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.4; k.relief.hybrid = 0.8; k.relief.flat = 0.08; k.relief.hill = 0.35; k.relief.mtn = 1.2;   // rounded hills of ice and rock
        k.relief.cliffAmount = 0.5; k.relief.cliffStep = 0.6;
        mix(k, MAT_ROCK, MM_CONST, RGB(0.52f, 0.48f, 0.42f)); mix(k, MAT_SNOW, MM_CONST, RGB(0.93f, 0.91f, 0.82f)); mix(k, MAT_ICE, MM_CONST, RGB(0.82f, 0.86f, 0.90f));
        mix(k, MAT_WATER, MM_CONST, RGB(0.34f, 0.42f, 0.48f)); mix(k, MAT_SAND, MM_CONST, RGB(0.40f, 0.34f, 0.26f));   // ice-rock, ammonia frost, the slabs; the grey-blue sea; the shores' silt
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_AMMONIA; k.plainK = 0.7; k.gullyK = 0.8; k.drainArea = 1.5;
        crater(k, 30.0, false, 0.05, CD_ABS, 0.4, false);
        fixedSky(k, RGB(0.62f, 0.50f, 0.30f), RGB(0.86f, 0.72f, 0.46f));   // the haze the sun shows through dimly
        k.fog = 3500; k.skyDensity = 1.5;
        night(k, RGB(0.03f, 0.025f, 0.015f), RGB(0.05f, 0.04f, 0.03f));
        rocks(k, 0.4, 1.0); k.aurora = AU_THIN;
        k.pressureLo = 1.0; k.pressureSpan = 2.0; k.tempOffset = -8; k.tempSwing = 4; k.wind = 5;
        k.rain = RAIN_DRIZZLE;   // ammonia drizzle now and then
    }
    {   KindDef& k = K[PT_SNOWBALL];
        identity(k, PT_SNOWBALL, "SNOWBALL", "SNOWBALL", "medium size: frozen to the equator, ice sheets over the land and the seas, nunataks, a thin belt of open sea, blizzards.", 4800, 7800, true,
                true, 3, 0.72);
        k.family = "deep";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_WET | KT_FRONTS | KT_FRONT_RAIN | KT_WATER_WEATHER | KT_SEA | KT_TWIN | KT_ICE_SHEETS;   // nothing on it lives that the eye can see
        k.shapes = KS_INCISION | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_BASALTIC | KS_FROST | KS_WIND | KS_FLOOD_LAVA | KS_HYDROTHERMAL | KS_STORMS | KS_HAZE;
        k.salt = 0x5A0BA11100000011ULL;   // pinned: the regress's and the bench's forest, Feeyeithis IV, stays felisian
        source(k, of(PT_FELISIAN), 195, 222, ALL_BODIES, MAIN_SEQUENCE, -1, 0.5);   // Kirschvink 1992, Hoffman 1998: Earth froze to its equator twice
        source(k, of(PT_OCEAN), 195, 240, ALL_BODIES, 0, -1, 0.35);
        colour(k, ch(0.86f, 0.04f), ch(0.89f, 0.03f), ch(0.94f, 0.03f));
        sentence(k, SR_COIN, "world frozen to its equator, ice sheets over its land and its seas, nunataks standing out of them, a thin belt of open sea at the equator",
                 "white world of sea glaciers and blizzards, its only open water a belt round its equator");
        k.greenhouseK = -45; k.latGradient = 0.9;   // the ice's white keeps it frozen
        draw(k, GF_SEA_LEVEL, -0.35, 0.5); draw(k, GF_MOUNTAIN, 1500, 2500); draw(k, GF_HILL, 150, 250); draw(k, GF_ICE_CAP, 4, 8); draw(k, GF_SNOW_LINE, 300, 600);
        draw(k, GF_MOISTURE, -0.1, 0.15); draw(k, GF_CLOUD_COVER, 0.35, 0.35);   // the sheets from 4-12 degrees, the sea ice from 8-16
        k.color2 = tintOf(TINT_CONST, RGB(0.3f, 0.45f, 0.65f)); k.skyTint = tintOf(TINT_DRAWN, RGB(0.58f, 0.70f, 0.94f), RGB(0.08f, 0.08f, 0.05f));
        k.relief.mtn = 4.0; k.relief.hill = 0.42;
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.40f, 0.40f, 0.42f), 0.15f); mix(k, MAT_SAND, MM_BASE_MIX, RGB(0.60f, 0.58f, 0.54f), 0.15f); mix(k, MAT_SNOW, MM_CONST, RGB(0.95f, 0.96f, 0.99f));
        mix(k, MAT_ICE, MM_CONST, RGB(0.68f, 0.79f, 0.92f)); mix(k, MAT_WATER, MM_CONST, RGB(0.07f, 0.19f, 0.34f));   // dark nunataks, moraine, blue ice, the cold sea
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FELISIAN; k.nunataks = true; k.seaGlaciers = true; k.sheetM = 1300;   // a sheet thin enough for the ranges to stand out of it
        k.fog = 9000; k.clouds = true;
        night(k, RGB(0.02f, 0.02f, 0.05f), RGB(0.06f, 0.06f, 0.1f));
        rocks(k, 0.5, 1.0); k.snowAlbedo = 0.95;
        k.pressureLo = 0.7; k.pressureSpan = 0.6; k.wind = 24;   // blizzards
        k.rain = RAIN_CLOUDS; k.rainThr = 0.7; k.rainSpan = 0.25;
        k.daySwing = 6;
    }
    {   KindDef& k = K[PT_CHEMOTROPH];
        identity(k, PT_CHEMOTROPH, "CHEMOTROPH", "CHEMOTROPH",
                "medium size, dim and warmed from within: hot springs on dark rock, pale mats of a life fed by their heat, not by light, spores glowing at night.", 2400, 5200, true, true, 2, 0.22);
        k.family = "deep";
        k.tags = KT_PLATES | KT_TWIN | KT_VENT_LIFE;
        k.shapes = KS_INCISION | KS_EXTENSION | KS_SCARPS | KS_BASALTIC | KS_FROST | KS_FLOOD_LAVA | KS_PLUMES | KS_HYDROTHERMAL;
        k.salt = 0xC4E3070000000012ULL;
        source(k, of(PT_TECTONIC), 0, 1e9, ALL_BODIES, DEAD_STARS, 0, 0.6);   // Corliss 1979: the vents' life lives on chemistry, not light: a dead
        source(k, of(PT_ROCKY), 0, 1e9, ALL_BODIES, DEAD_STARS, 0, 0.12);     // star's world or a brown dwarf's moon, warmed from within
        source(k, of(PT_TECTONIC, PT_ROCKY), 0, 1e9, KB_MOON, 0, 0, 0.3, false, of(PT_SUBSTELLAR));
        colour(k, ch(0.26f, 0.04f), ch(0.24f, 0.03f), ch(0.23f, 0.03f));
        sentence(k, SR_COIN, "dim world warmed from within, hot springs steaming on its dark rock, pale mats spreading from them and spores glowing over them at night",
                 "world of life without light, springs and fumaroles along its rifts, their mats white and pink on the black basalt");
        draw(k, GF_MOUNTAIN, 1200, 1200); draw(k, GF_HILL, 150, 150); draw(k, GF_VOLCANOES, 0.3, 0.5); draw(k, GF_CRATER_DENSITY, 0.05, 0.05); draw(k, GF_CLOUD_COVER, 0.1, 0.2);
        k.color2 = tintOf(TINT_CONST, RGB(0.85f, 0.82f, 0.78f)); k.skyTint = tintOf(TINT_CONST, RGB(0.36f, 0.34f, 0.38f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.3; k.relief.mtn = 2.6; k.relief.hill = 0.7;   // the tectonic's fault blocks
        mix(k, MAT_BASALT, MM_CONST, RGB(0.20f, 0.18f, 0.17f)); mix(k, MAT_DUST, MM_CONST, RGB(0.40f, 0.38f, 0.36f)); mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.34f, 0.31f, 0.28f), 0.3f);
        // sinter; the springs' blue; frost
        mix(k, MAT_SALT, MM_CONST, RGB(0.84f, 0.80f, 0.70f)); mix(k, MAT_BRINE, MM_CONST, RGB(0.20f, 0.52f, 0.58f)); mix(k, MAT_SNOW, MM_CONST, RGB(0.86f, 0.87f, 0.90f));
        // the mats' hue is the world's (`materialPalette`)
        k.famRep[FAM_ROCK] = MAT_BASALT; k.famRep[FAM_WATER] = MAT_BRINE; k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_MATS; k.accent[1] = MAT_SALT;
        k.ground = GR_CHEMOTROPH;
        crater(k, 30.0, false, 0.08, CD_ABS, 0.5, false, true);
        fixedSky(k, RGB(0.10f, 0.10f, 0.12f), RGB(0.30f, 0.28f, 0.30f));   // thin and dark under a dim star
        k.fog = 12000; k.skyDensity = 0.6; k.clouds = true;
        night(k, RGB(0.01f, 0.01f, 0.012f), RGB(0.03f, 0.025f, 0.03f));
        rocks(k, 1.0, 1.2); k.aurora = AU_SULPHUR;
        k.pressureLo = 0.1; k.pressureSpan = 0.4; k.tempOffset = 30; k.tempSwing = 6; k.wind = 6;   // the ground's own heat in a thin air
        k.events = EV_SPRINGS; k.eventMaterial = MAT_SALT;
    }
    {   KindDef& k = K[PT_FROZEN_FELISIAN];
        identity(k, PT_FROZEN_FELISIAN, "FROZEN FELISIAN", "FROZEN",
                "medium size: a living world that lost its sun, its seas frozen at their level, its forests standing dead and white with frost, its air fallen as snow.", 4800, 7600, true, true, 3,
                0.72);
        k.family = "dead";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_PEOPLE | KT_TWIN;
        k.salt = 0xF20E2E0000000013ULL;
        k.elsewhere = true;   // made where no system is: P-05's wanderers and D's drifters (the harness stages one on a felisian world)
        colour(k, ch(0.82f, 0.05f), ch(0.86f, 0.04f), ch(0.94f, 0.03f));
        sentence(k, SR_COIN, "dead world that lost its sun, its seas frozen flat at their level and its forests standing white with frost",
                 "frozen world of dead forests and seas of ice under a black sky, its air fallen as snow toward its poles");
        thenGround(k, K[PT_FELISIAN]);
        k.cover = DC_FROZEN; k.peopleShare = 0.25; k.peopleSalt = 0xF20E2E0B0B0B0001ULL;   // D-05's drifters: one in four kept a people (a wanderer none)
        mix(k, MAT_SNOW, MM_CONST, RGB(0.90f, 0.93f, 0.98f)); mix(k, MAT_ICE, MM_CONST, RGB(0.58f, 0.68f, 0.84f)); mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.36f, 0.36f, 0.38f), 0.15f);
        mix(k, MAT_DUST, MM_CONST, RGB(0.34f, 0.31f, 0.28f)); mix(k, MAT_SAND, MM_CONST, RGB(0.52f, 0.49f, 0.44f));   // frost, the seas' ice, dark rock, the dead soil where the frost is thin
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_ICE;   // the seas' ice blue in a bank of its own, the frost white
        fixedSky(k, RGB(0.02f, 0.03f, 0.07f), RGB(0.10f, 0.13f, 0.22f));   // the air too thin to scatter: black-blue
        k.fog = 40000; k.skyDensity = 0.25;
        night(k, RGB(0.004f, 0.005f, 0.01f), RGB(0.01f, 0.012f, 0.02f));
        k.floorFamily = FAM_SNOW; rocks(k, 0.35, 0.9); k.snowAlbedo = 0.92; k.aurora = AU_THIN;
        k.pressureLo = 0.005; k.pressureSpan = 0.03; k.tempOffset = 0; k.tempSwing = 2; k.wind = 2;   // what air is left above the frozen
    }
    {   KindDef& k = K[PT_SCORCHED_FELISIAN];
        identity(k, PT_SCORCHED_FELISIAN, "SCORCHED FELISIAN", "SCORCHED",
                "medium size: a living world baked by its swelling sun, its seas boiled off to salt, its forests turned to stone, cracked clay and dust under a thin hot sky.", 4800, 7600, true, true,
                3, 0.48);
        k.family = "dead";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FRONTS | KT_DUST | KT_PEOPLE | KT_DRY | KT_TWIN;
        k.salt = 0x5C0BC4ED00000014ULL;
        source(k, of(PT_FELISIAN), 0, 1e9, ALL_BODIES, of(STAR_RED_GIANT, STAR_CARBON), 0, 0.75);   // Earth in a billion years (the brightening sun ends the oceans,
        source(k, of(PT_FELISIAN), 0, 1e9, ALL_BODIES, of(STAR_ORANGE_GIANT), 0, 0.5);              // the red giant's tip the rest): a giant's old living worlds
        source(k, of(PT_ROCKY, PT_DESERT), 180, 260, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN, of(STAR_WHITE_DWARF), 0, 0.4);   // and the same worlds after the giant, round what is left of it: cold
        colour(k, ch(0.74f, 0.08f), ch(0.62f, 0.06f), ch(0.48f, 0.06f));
        sentence(k, SR_COIN, "dead world baked by its swelling sun, its old seabeds white with salt far below the shores, its forests turned to stone",
                 "scorched world of dry ocean basins, cracked clay and stone forests under a thin hot sky");
        thenGround(k, K[PT_FELISIAN]);
        k.cover = DC_SCORCHED; k.peopleShare = 0.125; k.peopleSalt = 0x5C0BC4ED0B0B0001ULL;   // one world in eight had a people (C-01's share)
        mix(k, MAT_ROCK, MM_BASE_MIX, RGB(0.56f, 0.48f, 0.40f), 0.25f); mix(k, MAT_SAND, MM_CONST, RGB(0.80f, 0.70f, 0.52f)); mix(k, MAT_DUST, MM_CONST, RGB(0.66f, 0.56f, 0.44f));
        mix(k, MAT_SALT, MM_CONST, RGB(0.94f, 0.93f, 0.89f)); mix(k, MAT_MUD, MM_CONST, RGB(0.60f, 0.40f, 0.28f)); mix(k, MAT_STROMATOLITE, MM_CONST, RGB(0.86f, 0.82f, 0.70f));
        mix(k, MAT_SNOW, MM_CONST, RGB(0.90f, 0.92f, 0.95f));   // the old seas' salt, the soils' clay, the shelves' reef limestone, frost where it is cold
        k.famRep[FAM_SAND] = MAT_SAND; k.famRep[FAM_SNOW] = MAT_SALT; k.accent[0] = MAT_MUD; k.accent[1] = MAT_STROMATOLITE;   // the clay and the reefs in banks of their own
        fixedSky(k, RGB(0.58f, 0.50f, 0.42f), RGB(0.86f, 0.75f, 0.60f));   // thin and hot, a dusty pale under the giant
        k.fog = 14000; k.skyDensity = 1.0;
        night(k, RGB(0.02f, 0.015f, 0.012f), RGB(0.045f, 0.035f, 0.025f));
        k.floorFamily = FAM_SAND; rocks(k, 0.6, 1.0); k.aurora = AU_DESERT;
        k.pressureLo = 0.05; k.pressureSpan = 0.25; k.tempOffset = 15; k.tempSwing = 18; k.wind = 14; k.dustThreshold = 0.55;
        k.events = EV_DEVILS;
    }
    // the far cold. The coldest worlds made from the icy, the cratered, the europan and the thin-air worlds of the outer orbits by their
    // temperature (Pluto's nitrogen under 60 K, Eris's frost under 45, Mars' dry ice at 140-200, Ceres's brine at 120-260), Triton's plumes on the
    // giants' captured moons first; and the wanderer, a rogue world a star caught, made by its own step of `generate` (no source here)
    {   KindDef& k = K[PT_NITROGEN];
        identity(k, PT_NITROGEN, "NITROGEN", "NITROGEN", "small, far out: plains of nitrogen ice churned into cells, mountains of water ice afloat at their edges, dark red lands, a thin blue haze.",
                900, 2500, true, true, 2, 0.62);
        k.family = "cold"; k.density = 0.6;   // Pluto: rock under a crust of water ice
        k.tags = KT_FROZEN;
        k.shapes = KS_SCARPS | KS_BRITTLE | KS_FROST | KS_SPIRES;   // its great basin is its own (the nitrogen plain's)
        k.salt = 0x41720C0000000015ULL;
        source(k, of(PT_ICY), 0, 60, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN, KEEPS_AIR, 0, 0.5);   // New Horizons' Pluto (2015): nitrogen ice that flows and
        source(k, of(PT_ICY), 0, 60, KB_MOON, KEEPS_AIR, 0, 0.5, false, WORLD_KINDS);                // churns, a thin air of it over the ground (a giant's moons are the cryovolcanic's)
        colour(k, ch(0.80f, 0.06f), ch(0.70f, 0.05f), ch(0.60f, 0.05f));   // cream and pale orange, dark red at the equator
        sentence(k, SR_COIN, "far world of nitrogen plains churned into cells, mountains of water ice afloat at their edges and dark red lands beyond",
                 "cold world of nitrogen glaciers and dark red tholin under a thin blue haze, blades of methane ice along its ridges");
        draw(k, GF_SEA_LEVEL, -650, 450); draw(k, GF_MOUNTAIN, 700, 900); draw(k, GF_HILL, 80, 120); draw(k, GF_CRATER_DENSITY, 0.05, 0.1);   // the plains' level (metres)
        k.color2 = tintOf(TINT_CONST, RGB(0.30f, 0.45f, 0.80f)); k.skyTint = tintOf(TINT_CONST, RGB(0.30f, 0.45f, 0.85f));   // the haze's blue
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.6; k.relief.mtn = 1.7; k.relief.hill = 0.5; k.relief.cliffAmount = 0.6; k.relief.cliffStep = 0.7;
        mix(k, MAT_ICE, MM_CONST, RGB(0.93f, 0.89f, 0.80f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.68f, 0.62f, 0.56f)); mix(k, MAT_DUST, MM_CONST, RGB(0.36f, 0.18f, 0.12f));
        mix(k, MAT_SNOW, MM_CONST, RGB(0.95f, 0.91f, 0.92f));   // the plains' nitrogen, the crust's water ice, tholin, methane frost
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_ICE;   // the nitrogen in a bank of its own
        k.ground = GR_NITROGEN; k.plainK = 0.4; k.gullyK = 0.2;
        crater(k, 0.15, true, 0.5, CD_REL, 0.45, false); crater(k, 6.0, false, 0.7, CD_REL, 0.35, false, true); crater(k, 0.8, false, 0.7, CD_REL, 0.3, false);
        fixedSky(k, RGB(0.012f, 0.016f, 0.03f), RGB(0.16f, 0.26f, 0.50f));   // black overhead, the haze's blue at the horizon
        k.fog = 40000; k.skyDensity = 0.12; k.limbHaze = 26;   // a sky too thin to scatter but for the haze's layers low over the horizon
        night(k, RGB(0.003f, 0.004f, 0.008f), RGB(0.012f, 0.016f, 0.03f));
        k.floorFamily = FAM_GRASS; rocks(k, 0.35, 1.2); k.snowAlbedo = 0.9; k.aurora = AU_THIN;
        k.pressureLo = 0.00001; k.pressureSpan = 0.00002; k.tempOffset = 0; k.tempSwing = 4; k.wind = 2;   // ten millionths of a bar
    }
    {   KindDef& k = K[PT_CRYOVOLCANIC];
        identity(k, PT_CRYOVOLCANIC, "CRYOVOLCANIC", "CRYOVOLCAN",
                "small, a captured moon: dimpled ground like a melon's skin, a pink frost cap, dark plumes rising kilometres into a thin sky and streaming downwind.", 1000, 2000, true, true, 0, 0.72);
        k.family = "cold"; k.density = 0.65;   // Triton: rock and ice
        k.tags = KT_FROZEN | KT_FLAT;
        k.shapes = KS_EXTENSION | KS_BRITTLE | KS_FROST | KS_SPIRES;
        k.salt = 0xC2E0FEED00000016ULL;
        source(k, of(PT_ICY), 0, 120, KB_CAPTURED, KEEPS_AIR, 0, 0.8);   // Voyager 2's Triton (1989): a captured moon on a retrograde orbit, its tides warming it (the captured first)
        source(k, of(PT_ICY, PT_EUROPAN), 0, 80, KB_MOON, KEEPS_AIR, 0, 0.2, false, of(PT_GASGIANT, PT_SUBSTELLAR));   // and the coldest ice moons of the giants
        colour(k, ch(0.88f, 0.04f), ch(0.80f, 0.04f), ch(0.78f, 0.04f));   // pink-white
        sentence(k, SR_COIN, "captured moon of dimpled ground like a melon's skin, a pink frost cap at its pole and dark plumes rising kilometres into its thin sky",
                 "cold moon of frozen nitrogen whose geysers lay dark streaks downwind across its pink frost");
        // the cap's edge (south of it), the plumes' share
        draw(k, GF_MOUNTAIN, 250, 300); draw(k, GF_HILL, 40, 60); draw(k, GF_CRATER_DENSITY, 0.02, 0.04); draw(k, GF_ICE_CAP, 12, 26); draw(k, GF_VOLCANOES, 0.3, 0.5);
        k.color2 = tintOf(TINT_CONST, RGB(0.22f, 0.20f, 0.20f)); k.skyTint = tintOf(TINT_CONST, RGB(0.55f, 0.55f, 0.65f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.4; k.relief.flat = 0.06; k.relief.hill = 0.3; k.relief.mtn = 0.6; k.relief.cliffAmount = 0.5; k.relief.cliffStep = 0.6;
        mix(k, MAT_ICE, MM_CONST, RGB(0.70f, 0.71f, 0.64f)); mix(k, MAT_SNOW, MM_CONST, RGB(0.95f, 0.85f, 0.84f)); mix(k, MAT_DUST, MM_CONST, RGB(0.24f, 0.22f, 0.22f));
        mix(k, MAT_ROCK, MM_CONST, RGB(0.58f, 0.62f, 0.67f));   // the dimpled ice grey-green, the cap's nitrogen frost pink, the plumes' streaks, the frozen lakes' ice
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_ICE;
        k.ground = GR_CRYOVOLCANIC; k.plainK = 0.3; k.gullyK = 0.2;
        crater(k, 0.1, true, 0.08, CD_ABS, 0.4, false); crater(k, 8.0, false, 0.6, CD_REL, 0.3, false, true); crater(k, 1.0, false, 0.5, CD_REL, 0.25, false);   // young ground
        fixedSky(k, RGB(0.012f, 0.012f, 0.02f), RGB(0.10f, 0.11f, 0.16f));
        k.fog = 45000; k.skyDensity = 0.1; k.limbHaze = 12;
        night(k, RGB(0.003f, 0.003f, 0.006f), RGB(0.01f, 0.01f, 0.016f));
        k.floorFamily = FAM_GRASS; rocks(k, 0.2, 1.0); k.aurora = AU_THIN;
        k.pressureLo = 0.000014; k.pressureSpan = 0.00001; k.tempOffset = 0; k.tempSwing = 3; k.wind = 8;   // the plumes' tops lean on the wind
        k.events = EV_PLUMES; k.eventMaterial = MAT_SNOW;   // the plumes rise from the cap
    }
    {   KindDef& k = K[PT_DRY_ICE];
        identity(k, PT_DRY_ICE, "DRY ICE", "DRY ICE",
                "small and cold under a thin sky of carbon dioxide: caps of dry ice pitted like cheese, spider-shaped channels and dark fans laid by the jets of spring.", 2000, 4500, true, true, 2,
                0.45);
        k.family = "cold";
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FRONTS | KT_DUST | KT_TWIN;
        k.shapes = KS_INCISION | KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_GLACIERS | KS_PEDIMENTS | KS_BASALTIC | KS_FROST | KS_WIND | KS_FLOOD_LAVA | KS_IMPACTS | KS_SPIRES;
        k.salt = 0xD21C0E0300000017ULL;   // pinned: the regress's Seapheeseeri II and I-a, Iaphiath I and II, Feeyeithis V and home's Heilya V and IV-a stay as drawn
        source(k, of(PT_THINATMO), 140, 200, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN, KEEPS_AIR, 0, 0.35);   // Mars' south polar cap (Piqueux 2003, Kieffer 2006): an air
        source(k, of(PT_THINATMO), 140, 200, KB_MOON, KEEPS_AIR, 0, 0.2);                                    // cold enough to freeze out at its poles, spring breaking the ice
        source(k, of(PT_ICY), 140, 200, KB_PLANET | KB_COMPANION_WORLD, KEEPS_AIR, 0, 0.1);
        colour(k, ch(0.72f, 0.08f), ch(0.62f, 0.06f), ch(0.56f, 0.06f));   // ochre lands, white caps
        sentence(k, SR_COIN, "cold world of thin air whose caps of dry ice reach its middle latitudes, pitted like cheese, spider-shaped channels and dark fans round their edges",
                 "world of carbon dioxide frost and ochre dust, its caps breaking each spring into jets that lay dark fans on the ice");
        draw(k, GF_MOUNTAIN, 1200, 2000); draw(k, GF_HILL, 120, 200); draw(k, GF_CRATER_DENSITY, 0.15, 0.25); draw(k, GF_DUNE_AMP, 4, 10); draw(k, GF_ICE_CAP, 50, 14);
        draw(k, GF_CLOUD_COVER, 0.05, 0.15);
        k.color2 = tintOf(TINT_CONST, RGB(0.92f, 0.92f, 0.96f)); k.skyTint = tintOf(TINT_CONST, RGB(0.70f, 0.62f, 0.55f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.15; k.relief.mtn = 2.3;
        mix(k, MAT_ROCK, MM_BASE_MIX, ROCK_GREY, 0.6f); mix(k, MAT_DUST, MM_BASE_MIX, RGB(0.72f, 0.58f, 0.42f), 0.4f); mix(k, MAT_BASALT, MM_CONST, RGB(0.17f, 0.15f, 0.14f));
        mix(k, MAT_ICE, MM_CONST, RGB(0.93f, 0.94f, 0.96f)); mix(k, MAT_SNOW, MM_CONST, RGB(0.86f, 0.80f, 0.76f));   // the residual cap's dry ice; the seasonal frost, dusty
        k.famRep[FAM_SNOW] = MAT_ICE; k.accent[0] = MAT_BASALT; k.accent[1] = MAT_SNOW;   // the fans' dark dust and the spring frost in banks of their own
        k.ground = GR_DRY_ICE; k.plainK = 0.6; k.gullyK = 0.6; k.drainArea = 2.2;
        crater(k, 0.2, true, 0.7, CD_REL, 0.7, true); crater(k, 0.05, true, 1, CD_REL, 0.6, true, true); crater(k, 8.0, false, 0.9, CD_REL, 0.5, true, true);
        crater(k, 1.5, false, 0.9, CD_REL, 0.4, false); crater(k, 0.3, false, 0.5, CD_ABS, 0.3, false); crater(k, 0.06, false, 0.4, CD_ABS, 0.25, false);
        fixedSky(k, RGB(0.32f, 0.27f, 0.24f), RGB(0.70f, 0.60f, 0.50f));   // thin and dusty, a butterscotch horizon
        k.fog = 18000; k.skyDensity = 0.5; k.clouds = true;
        night(k, RGB(0.01f, 0.01f, 0.015f), RGB(0.03f, 0.025f, 0.025f));
        k.floorFamily = FAM_SAND; rocks(k, 1.0, 0.9); k.aurora = AU_THIN;
        k.pressureLo = 0.004; k.pressureSpan = 0.008; k.tempOffset = -5; k.tempSwing = 30; k.wind = 16; k.dustThreshold = 0.45;
        k.events = EV_SPRING_JETS | EV_STORM_DEVILS; k.eventMaterial = MAT_SNOW;   // the jets through the spring frost
    }
    {   KindDef& k = K[PT_WANDERER];
        identity(k, PT_WANDERER, "WANDERER", "WANDERER", "small to medium: a rogue world a star caught on a wild orbit, an old world's ground under its air fallen as snow, domes of new ice.", 2000,
                6000, true, false, 0, 0.75);
        k.family = "cold"; k.density = 0.85;
        k.tags = KT_PLATES | KT_DRAINAGE | KT_FROZEN;
        k.shapes = KS_INCISION | KS_LAYERED | KS_EXTENSION | KS_SCARPS | KS_PEDIMENTS | KS_FROST | KS_PLUMES;   // the landforms whose walls stand out of the snow (no lava sheets or sand under it)
        k.salt = 0x3A9DE70000000018ULL; k.elsewhere = true;   // made by the wanderers' own step of `generate`, never from a drawn kind
        colour(k, ch(0.86f, 0.04f), ch(0.86f, 0.04f), ch(0.88f, 0.05f));
        sentence(k, SR_COIN, "rogue world a star caught on a wild orbit, the valleys and craters of an old world under its air fallen as snow",
                 "frozen wanderer whose air lies on its ground as drifts of nitrogen snow, domes of new ice rising through it");
        draw(k, GF_MOUNTAIN, 1200, 1800); draw(k, GF_HILL, 120, 200); draw(k, GF_CRATER_DENSITY, 0.12, 0.2); draw(k, GF_DUNE_AMP, 3, 6); draw(k, GF_ICE_CAP, 95, 0, DK_CONST);
        draw(k, GF_SNOW_LINE, 6, 10); draw(k, GF_VOLCANOES, 0.1, 0.25);   // the fallen air's depth on the open ground (metres), the domes' share
        k.color2 = tintOf(TINT_CONST, RGB(0.88f, 0.90f, 0.95f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.15; k.relief.mtn = 2.3;
        mix(k, MAT_SNOW, MM_CONST, RGB(0.93f, 0.92f, 0.90f)); mix(k, MAT_ICE, MM_CONST, RGB(0.70f, 0.80f, 0.92f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.32f, 0.30f, 0.29f));
        mix(k, MAT_DUST, MM_CONST, RGB(0.40f, 0.37f, 0.35f)); mix(k, MAT_BASALT, MM_CONST, RGB(0.22f, 0.21f, 0.21f));   // nitrogen and carbon monoxide snow, the domes' new ice, the old rock
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_ICE;
        k.ground = GR_WANDERER; k.plainK = 0.5; k.gullyK = 0.4; k.drainArea = 2.2;
        crater(k, 0.2, true, 0.6, CD_REL, 0.6, true); crater(k, 0.05, true, 1, CD_REL, 0.5, true, true); crater(k, 8.0, false, 0.8, CD_REL, 0.4, true, true);
        crater(k, 1.5, false, 0.8, CD_REL, 0.35, false); crater(k, 0.3, false, 0.4, CD_ABS, 0.25, false);
        k.fog = 45000; k.floorFamily = FAM_SNOW; rocks(k, 0.15, 1.1); k.snowAlbedo = 0.9;   // the boulders under the snow but a few
    }
    {   KindDef& k = K[PT_BRINE_DWARF];
        identity(k, PT_BRINE_DWARF, "BRINE DWARF", "BRINE DWRF",
                "small, dark rock and ice: bright salt on the floors of its young craters, one lone mountain of ice streaked with salt, no atmosphere.", 900, 2000, true, false, 1, 0.12);
        k.family = "cold"; k.density = 0.75;   // Ceres: rock and ice
        k.shapes = KS_SCARPS | KS_BRITTLE | KS_FROST | KS_EVAPORITES;   // no flooded basin: no lava ever came up here, only brine
        k.salt = 0xB21E0D0000000019ULL;
        source(k, of(PT_CRATERED), 120, 260, KB_PLANET | KB_COMPANION_WORLD, 0, 0, 0.15);   // Dawn's Ceres (2015-18): a dwarf of rock and ice whose brine wells up
        source(k, of(PT_ICY), 120, 260, KB_PLANET | KB_COMPANION_WORLD, 0, 0, 0.2);         // into its young craters and dries to salt
        colour(k, ch(0.30f, 0.03f), ch(0.29f, 0.03f), ch(0.28f, 0.03f));   // dark grey
        sentence(k, SR_COIN, "dark dwarf of rock and ice, bright salt on the floors of its young craters and one lone mountain of ice standing on its plains",
                 "airless world of dark craters, the youngest of them floored with salt where brine welled up from below");
        draw(k, GF_MOUNTAIN, 400, 600); draw(k, GF_HILL, 60, 100); draw(k, GF_CRATER_DENSITY, 0.4, 0.3); draw(k, GF_CRATER_DEPTH, 0.8, 0.4);
        k.color2 = tintOf(TINT_CONST, RGB(0.95f, 0.94f, 0.90f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.6; k.relief.mtn = 1.4; k.relief.hill = 0.4;
        mix(k, MAT_DUST, MM_CONST, RGB(0.25f, 0.24f, 0.23f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.31f, 0.30f, 0.29f)); mix(k, MAT_SALT, MM_CONST, RGB(0.96f, 0.95f, 0.92f));
        mix(k, MAT_ICE, MM_CONST, RGB(0.58f, 0.58f, 0.60f));   // carbon-dark regolith, the walls, the faculae's sodium carbonate, the mountain's salty ice
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_SALT;
        k.ground = GR_BRINE; k.plainK = 0.5; k.gullyK = 0.3;
        crater(k, 0.4, true, 0.6, CD_REL, 1, true); crater(k, 0.12, true, 1, CD_REL, 1, true, true); crater(k, 10.0, false, 1.1, CD_REL, 1, true, true);
        crater(k, 2.0, false, 1.3, CD_REL, 1, true, true); crater(k, 0.4, false, 0, CD_SECONDARY, 0.8, true); crater(k, 0.08, false, 0.8, CD_ABS, 0.5, true);
        k.fog = 45000; k.floorFamily = FAM_SAND;
        rocks(k, 0.9, 1.0);
    }
    {   KindDef& k = K[PT_FROST_DWARF];
        identity(k, PT_FROST_DWARF, "FROST DWARF", "FROST DWRF",
                "small, the coldest: the brightest ground there is, a fresh coat of methane frost, red streaks of tholin, sublimation pits, almost no air.", 600, 1500, true, false, 1, 0.92);
        k.family = "cold"; k.density = 0.65;
        k.tags = KT_FROZEN | KT_FLAT;
        k.shapes = KS_FROST | KS_SPIRES;   // the fresh frost hides any scarp
        k.salt = 0xF205DA0000000020ULL;
        source(k, of(PT_ICY, PT_CRATERED), 0, 45, KB_PLANET | KB_COMPANION_WORLD | KB_TWIN, 0, 0, 0.8);   // Eris (an albedo of 0.96) and Makemake: their air frozen
        source(k, of(PT_ICY, PT_CRATERED), 0, 45, KB_MOON, 0, 0, 0.8, false, WORLD_KINDS);                  // onto the ground as a fresh frost, far from their sun
        colour(k, ch(0.95f, 0.02f), ch(0.93f, 0.02f), ch(0.92f, 0.03f));
        sentence(k, SR_COIN, "the coldest kind of world, a fresh coat of methane frost brighter than any snow, red streaks of tholin across it",
                 "far dwarf of bright frost and few craters, its air frozen onto its ground, sublimation pits in fields");
        draw(k, GF_MOUNTAIN, 200, 300); draw(k, GF_HILL, 30, 50); draw(k, GF_CRATER_DENSITY, 0.03, 0.05);
        k.color2 = tintOf(TINT_CONST, RGB(0.66f, 0.40f, 0.30f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.4; k.relief.hybrid = 0.8; k.relief.flat = 0.06; k.relief.hill = 0.3; k.relief.mtn = 0.8;
        k.relief.cliffAmount = 0.4; k.relief.cliffStep = 0.6;
        mix(k, MAT_SNOW, MM_CONST, RGB(0.97f, 0.96f, 0.95f)); mix(k, MAT_ICE, MM_CONST, RGB(0.86f, 0.89f, 0.94f)); mix(k, MAT_DUST, MM_CONST, RGB(0.66f, 0.40f, 0.30f));
        mix(k, MAT_ROCK, MM_CONST, RGB(0.52f, 0.50f, 0.52f));   // methane frost, a nitrogen glaze, tholin, the pits' walls
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_SNOW;
        k.ground = GR_FROST; k.plainK = 0.3; k.gullyK = 0.2;
        crater(k, 0.15, true, 0.3, CD_REL, 0.35, false); crater(k, 5.0, false, 0.4, CD_REL, 0.3, false, true); crater(k, 0.8, false, 0.3, CD_REL, 0.25, false);
        k.fog = 45000; k.floorFamily = FAM_SNOW; rocks(k, 0.15, 0.8); k.snowAlbedo = 0.96;
    }
    // the strange moons. Made from the moons of their kinds by what they are among their planet's moons (`MoonFact`): Ganymede's lanes on a
    // giant's large moons of ice, Iapetus's two faces on its outer moons that face it, Enceladus's tiger stripes on its inner ones, Miranda's patchwork
    // on the moons in a resonance, Hyperion's pits on the small moons that tumble; airless, their ground each its own
    {   KindDef& k = K[PT_GROOVED_ICE];
        identity(k, PT_GROOVED_ICE, "GROOVED ICE", "GROOVED",
                "medium, a giant's large moon of ice: dark old ground in polygons cut by bright lanes of parallel grooves, frost at its poles, no atmosphere.", 1500, 2700, true, false, 0, 0.43);
        k.family = "moons"; k.density = 0.6;   // Ganymede: ice and rock, the largest moon
        k.tags = KT_FROZEN;
        k.shapes = KS_EXTENSION | KS_FROST;   // the lanes are a crust pulled apart (no escarpments: their walls' lines would cross the lanes' pattern)
        k.salt = 0x6A00EDEC00000021ULL;
        source(k, of(PT_ICY), 0, 1e9, KB_MOON, 0, 0, 0.35, false, GIANT_KINDS, MF_LARGE);   // Galileo's Ganymede (1996-2000): the sulci, lanes of grooved bright ice
        colour(k, ch(0.56f, 0.05f), ch(0.55f, 0.05f), ch(0.53f, 0.05f));   // grey, the lanes bright
        sentence(k, SR_COIN, "large ice moon of dark old polygons cut by bright lanes of parallel grooves, frost at its poles",
                 "moon of ice whose dark ancient crust split into polygons, bright ice grooved in parallel ridges filling the lanes between them");
        draw(k, GF_MOUNTAIN, 400, 600); draw(k, GF_HILL, 60, 100); draw(k, GF_CRATER_DENSITY, 0.35, 0.3); draw(k, GF_ICE_CAP, 38, 14);   // the frost's edge (degrees)
        k.color2 = tintOf(TINT_CONST, RGB(0.74f, 0.73f, 0.70f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.5; k.relief.flat = 0.08; k.relief.hill = 0.4; k.relief.mtn = 1.2; k.relief.cliffAmount = 0.5; k.relief.cliffStep = 0.6;
        mix(k, MAT_DUST, MM_CONST, RGB(0.34f, 0.31f, 0.27f)); mix(k, MAT_ICE, MM_CONST, RGB(0.74f, 0.73f, 0.70f)); mix(k, MAT_SNOW, MM_CONST, RGB(0.88f, 0.90f, 0.93f));
        mix(k, MAT_ROCK, MM_CONST, RGB(0.46f, 0.44f, 0.41f));   // the dark terrain's regolith, the lanes' ice, the polar frost, the walls
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_ICE;   // the lanes' ice in a bank of its own
        k.ground = GR_GROOVED; k.plainK = 0.4; k.gullyK = 0.2;
        crater(k, 0.4, true, 0.4, CD_REL, 0.8, true); crater(k, 0.1, true, 1, CD_REL, 0.8, true, true); crater(k, 10.0, false, 1.2, CD_REL, 0.7, true, true);
        crater(k, 2.0, false, 1.0, CD_REL, 0.6, true); crater(k, 0.4, false, 0.6, CD_ABS, 0.4, false); crater(k, 0.08, false, 0.5, CD_ABS, 0.3, false);
        k.fog = 45000; k.floorFamily = FAM_SAND; rocks(k, 0.3, 1.1);
    }
    {   KindDef& k = K[PT_TWO_FACED];
        identity(k, PT_TWO_FACED, "TWO-FACED", "TWO-FACED",
                "medium, an outer moon of ice facing its giant: the face it leads with black as coal, the other white as snow, a ridge along its equator, no atmosphere.", 700, 1500, true, false, 0,
                0.3);
        k.family = "moons"; k.density = 0.5;   // Iapetus: mostly ice
        k.tags = KT_FROZEN;
        k.shapes = KS_SCARPS | KS_FROST;
        k.salt = 0x7F0FACED00000022ULL;
        source(k, of(PT_ICY), 60, 1e9, KB_MOON, 0, 0, 0.14, false, GIANT_KINDS, MF_OUTER | MF_FACING);   // Cassini's Iapetus (2004-07): dark dust swept up on the face it
                                                                                                           // leads with, the ice sublimed off it and frozen onto the other (Spencer and Denk 2010)
        colour(k, ch(0.50f, 0.04f), ch(0.46f, 0.04f), ch(0.42f, 0.04f));
        sentence(k, SR_COIN, "outer moon whose leading face is black as coal and whose trailing face is white as snow, a ridge kilometres high along its equator",
                 "moon of ice that sweeps dark dust onto the face it leads with, the ice gone from that face and frozen onto the other, a great ridge round its middle");
        draw(k, GF_MOUNTAIN, 500, 700); draw(k, GF_HILL, 80, 120); draw(k, GF_CRATER_DENSITY, 0.45, 0.3);
        k.color2 = tintOf(TINT_CONST, RGB(0.14f, 0.10f, 0.08f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.6; k.relief.mtn = 1.5; k.relief.hill = 0.45; k.relief.cliffAmount = 0.6; k.relief.cliffStep = 0.7;
        mix(k, MAT_DUST, MM_CONST, RGB(0.17f, 0.11f, 0.08f)); mix(k, MAT_ICE, MM_CONST, RGB(0.84f, 0.83f, 0.80f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.55f, 0.53f, 0.50f));   // the dark dust red-brown
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_ICE;
        k.ground = GR_TWO_FACED; k.plainK = 0.5; k.gullyK = 0.2;
        crater(k, 0.5, true, 0.6, CD_REL, 0.9, true); crater(k, 0.15, true, 1, CD_REL, 0.9, true, true); crater(k, 12.0, false, 1.2, CD_REL, 0.8, true, true);
        crater(k, 2.5, false, 1.3, CD_REL, 0.7, true); crater(k, 0.5, false, 0.8, CD_ABS, 0.5, false); crater(k, 0.1, false, 0.6, CD_ABS, 0.4, false);
        k.fog = 45000; k.floorFamily = FAM_SNOW; rocks(k, 0.35, 1.1);
    }
    {   KindDef& k = K[PT_TIGER_STRIPED];
        identity(k, PT_TIGER_STRIPED, "TIGER-STRIPED", "TIGER",
                "small, an inner moon of a giant: the whitest ice, smooth snow plains, four rifts at its south pole venting curtains of ice, no atmosphere.", 600, 900, true, false, 0, 0.95);
        k.family = "moons"; k.density = 0.6;   // Enceladus: rock under a shell of ice over a sea
        k.tags = KT_FROZEN | KT_FLAT;
        k.shapes = KS_EXTENSION | KS_FROST | KS_SPIRES;
        k.salt = 0x716E5EA000000023ULL;
        source(k, of(PT_ICY, PT_EUROPAN), 0, 1e9, KB_MOON, 0, 0, 0.45, false, GIANT_KINDS, MF_INNER);   // Cassini's Enceladus (2005-15): a giant's inner moon, its tides
                                                                                                           // warming a sea under the south pole that breaks out through four rifts
        colour(k, ch(0.95f, 0.02f), ch(0.96f, 0.02f), ch(0.98f, 0.02f));
        sentence(k, SR_COIN, "small moon of the whitest ice, smooth snow plains, four great rifts at its south pole venting curtains of ice into space",
                 "inner moon under snow fallen from its own jets, four tiger stripes at its south pole breathing ice into the sky");
        draw(k, GF_MOUNTAIN, 150, 200); draw(k, GF_HILL, 30, 50); draw(k, GF_CRATER_DENSITY, 0.2, 0.3); draw(k, GF_ICE_CAP, 52, 8);   // the south polar terrain's edge (degrees)
        k.color2 = tintOf(TINT_CONST, RGB(0.60f, 0.74f, 0.86f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.4; k.relief.flat = 0.06; k.relief.hill = 0.3; k.relief.mtn = 0.7; k.relief.cliffAmount = 0.4; k.relief.cliffStep = 0.6;
        // the stripes' coarse ice blue
        mix(k, MAT_SNOW, MM_CONST, RGB(0.97f, 0.97f, 0.98f)); mix(k, MAT_ICE, MM_CONST, RGB(0.60f, 0.74f, 0.86f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.76f, 0.78f, 0.82f));
        k.famRep[FAM_SNOW] = MAT_SNOW; k.accent[0] = MAT_ICE;
        k.ground = GR_TIGER; k.plainK = 0.3; k.gullyK = 0.2;
        crater(k, 0.12, true, 0.6, CD_REL, 0.5, false); crater(k, 6.0, false, 0.8, CD_REL, 0.5, false, true); crater(k, 1.0, false, 0.8, CD_REL, 0.4, false);
        crater(k, 0.2, false, 0.4, CD_ABS, 0.3, false);
        k.fog = 45000; k.floorFamily = FAM_SNOW; rocks(k, 0.15, 0.9); k.snowAlbedo = 0.98;
        k.events = EV_CURTAINS; k.eventMaterial = MAT_ICE;   // the curtains rise from the stripes
    }
    {   KindDef& k = K[PT_PATCHWORK];
        identity(k, PT_PATCHWORK, "PATCHWORK", "PATCHWORK",
                "small, a moon broken and put back together: terrains in patches, coronae ringed by parallel ridges, chasms, a cliff twenty kilometres high, no atmosphere.", 600, 1500, true, false, 0,
                0.32);
        k.family = "moons"; k.density = 0.55;   // Miranda: ice and rock
        k.tags = KT_FROZEN;
        k.shapes = KS_EXTENSION | KS_SCARPS | KS_BRITTLE;
        k.salt = 0x9A7C4E0000000024ULL;
        source(k, of(PT_ICY, PT_ROCKY), 0, 200, KB_MOON, 0, 0, 0.08, false, 0, MF_RESONANT);   // Voyager 2's Miranda (1986): remade by the tides of a past resonance
        colour(k, ch(0.62f, 0.04f), ch(0.61f, 0.04f), ch(0.60f, 0.04f));
        sentence(k, SR_COIN, "small moon broken and put back together, terrains in patches, coronae ringed by parallel ridges and a cliff twenty kilometres high",
                 "moon of unlike ground fitted together, ovals of grooved bands in old cratered country, chasms, one cliff that would take minutes to fall");
        draw(k, GF_MOUNTAIN, 400, 600); draw(k, GF_HILL, 60, 100); draw(k, GF_CRATER_DENSITY, 0.35, 0.3);
        k.color2 = tintOf(TINT_CONST, RGB(0.36f, 0.35f, 0.34f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.6; k.relief.mtn = 1.4; k.relief.hill = 0.45; k.relief.cliffAmount = 0.6; k.relief.cliffStep = 0.7;
        // grey ice and its dark bands
        mix(k, MAT_DUST, MM_CONST, RGB(0.38f, 0.37f, 0.36f)); mix(k, MAT_ICE, MM_CONST, RGB(0.78f, 0.78f, 0.77f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.52f, 0.51f, 0.50f));
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_ICE;
        k.ground = GR_PATCHWORK; k.plainK = 0.5; k.gullyK = 0.2;
        crater(k, 0.3, true, 0.5, CD_REL, 0.8, true); crater(k, 8.0, false, 1.0, CD_REL, 0.7, true, true); crater(k, 1.5, false, 1.0, CD_REL, 0.6, true);
        crater(k, 0.3, false, 0.6, CD_ABS, 0.4, false);
        k.fog = 45000; k.floorFamily = FAM_SNOW; rocks(k, 0.45, 1.2);
    }
    {   KindDef& k = K[PT_SPONGE];
        identity(k, PT_SPONGE, "SPONGE", "SPONGE", "small and porous, tumbling: deep sharp-rimmed pits packed together like a sponge's, dark dust in their floors, no atmosphere.", 600, 900, true,
                false, 0, 0.3);
        k.family = "moons"; k.density = 0.3;   // Hyperion: four tenths of it void
        k.tags = KT_FROZEN;
        k.shapes = KS_FROST;
        k.salt = 0x5904E00100000025ULL;   // pinned: the regress's Wyariothmai V-b stays as drawn
        source(k, of(PT_CRATERED, PT_ICY), 0, 200, KB_MOON, 0, 0, 0.3, false, 0, MF_SMALL | MF_TURNING);   // Cassini's Hyperion (2005): a porous moon tumbling as it goes,
                                                                                                             // its craters crushed in rather than thrown out (Thomas et al. 2007)
        colour(k, ch(0.56f, 0.04f), ch(0.50f, 0.04f), ch(0.44f, 0.04f));
        sentence(k, SR_ONE, "small porous moon of deep sharp-rimmed pits packed together like a sponge's, dark dust pooled in their floors");
        draw(k, GF_MOUNTAIN, 200, 300); draw(k, GF_HILL, 40, 60);
        k.color2 = tintOf(TINT_CONST, RGB(0.20f, 0.13f, 0.09f));
        k.relief.ridgeOp = RO_MUL; k.relief.ridge = 0.5; k.relief.flat = 0.05; k.relief.hill = 0.25; k.relief.mtn = 0.6; k.relief.cliffAmount = 0.3; k.relief.cliffStep = 0.5;
        mix(k, MAT_DUST, MM_CONST, RGB(0.22f, 0.14f, 0.10f)); mix(k, MAT_ICE, MM_CONST, RGB(0.70f, 0.68f, 0.64f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.56f, 0.54f, 0.50f));   // the floors' dark red-brown
        k.famRep[FAM_SAND] = MAT_DUST; k.famRep[FAM_SNOW] = MAT_ICE;
        k.ground = GR_SPONGE; k.plainK = 0.2; k.gullyK = 0.1;
        k.fog = 45000; k.floorFamily = FAM_SNOW; rocks(k, 0.25, 1.0);
    }
    // the giants. Sudarsky et al. (2000): a giant's looks follow its warmth: ammonia clouds (class I, the gas giant as drawn), water clouds
    // (II), no cloud at all (III), the alkali metals' dark air (IV), clouds of rock and iron (V); on the game's scale (its inner orbits reach
    // 350-500 K, a gas giant is drawn under 205 K): a drawn gas giant over 165 K is water-cloud, over 290 K azure, and the giants of the warm
    // zones are made from the worlds of those orbits (their moons the drawn world's: few), a hot Jupiter only from a world locked to its star
    // (one that close is) and round a sun-like star about one in a hundred (Wright et al. 2012); the gas dwarfs from the warm worlds (the
    // sub-Neptunes, Kepler's commonest planets: Fulton et al. 2017), the super-puffs from a young star's giants (Kepler-51's three)
    {   KindDef& k = K[PT_SUPER_PUFF];
        identity(k, PT_SUPER_PUFF, "SUPER-PUFF", "SUPER-PUFF",
                "very large and nearly weightless: a few Earths of mass swollen to a giant's size round a young star, a vast pale haze ballooning over its core.", 40000, 100000, false, true, 12,
                0.35);
        k.family = "giants"; k.density = 0.012; k.ringChance = 0.6;   // Kepler-51 b, c, d (Masuda 2014, Libby-Roberts et al. 2020): under 0.1 g/cm3
        k.tags = KT_GIANT | KT_GAS_GIANT | KT_NO_SIGHTS;
        k.salt = 0x5AFE9F0F00000026ULL;
        source(k, of(PT_GASGIANT), 0, 1e9, KB_PLANET | KB_COMPANION_WORLD, of(STAR_PROTOSTAR), 0, 0.6);                      // still swollen from its birth
        source(k, of(PT_GASGIANT), 0, 1e9, KB_PLANET | KB_COMPANION_WORLD, of(STAR_BLUE_GIANT, STAR_BLUE_WHITE), 0, 0.08);   // a short-lived star's
        source(k, of(PT_GASGIANT), 0, 1e9, KB_PLANET | KB_COMPANION_WORLD, of(STAR_YELLOW, STAR_ORANGE, STAR_RED_DWARF), 0, 0.03, true);   // a young star's, all alike
        palette4(k, RGB(0.86f, 0.75f, 0.60f), RGB(0.84f, 0.72f, 0.68f), RGB(0.80f, 0.78f, 0.72f), RGB(0.82f, 0.80f, 0.64f));   // pale hazes: amber, rose, grey, straw
        sentence(k, SR_ONE, "of almost no weight round a young star, a vast pale haze ballooning far over a small core");
        draw(k, GF_BAND_COUNT, 3, 2);
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.8f, 0.8f, 0.8f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
        k.giantClass = GC_PUFF;
    }
    {   KindDef& k = K[PT_WATER_CLOUD];
        identity(k, PT_WATER_CLOUD, "WATER-CLOUD GIANT", "WATERCLOUD", "large, warm enough for clouds of water: a bright white globe, its bands soft, its storms towering.", 24000, 72000, false, true,
                12, 0.81);
        k.family = "giants"; k.density = 0.25; k.ringChance = 0.6;
        k.tags = KT_GIANT | KT_GAS_GIANT | KT_NO_SIGHTS;
        k.salt = 0x4A7E2C3100000027ULL;   // pinned: B-427's Wolf-Rayet giant (Syaniapheela II) and the probe's Thilawiane III stay as drawn
        source(k, of(PT_GASGIANT), 165, 290, KB_PLANET | KB_COMPANION_WORLD, 0, 0, 0.75);           // too warm for ammonia to condense: water's clouds on top (47 UMa b)
        source(k, WORLD_KINDS, 205, 290, KB_PLANET | KB_COMPANION_WORLD, MAIN_SEQUENCE, 0, 0.01);   // a giant of the temperate orbits
        palette4(k, RGB(0.93f, 0.93f, 0.95f), RGB(0.95f, 0.93f, 0.88f), RGB(0.89f, 0.92f, 0.96f), RGB(0.94f, 0.91f, 0.90f));   // whites: cool, cream, blue, rose
        sentence(k, SR_ONE, "warm enough for clouds of water, a bright white globe of soft bands and towering storms");
        draw(k, GF_BAND_COUNT, 5, 9);
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.8f, 0.8f, 0.85f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
        k.giantClass = GC_WATER;
    }
    {   KindDef& k = K[PT_AZURE];
        identity(k, PT_AZURE, "AZURE GIANT", "AZURE", "large, too warm for any cloud: a deep blue globe, faint bands, its clear air lit far down.", 24000, 72000, false, true, 12, 0.12);
        k.family = "giants"; k.density = 0.22; k.ringChance = 0.15;
        k.tags = KT_GIANT | KT_GAS_GIANT | KT_NO_SIGHTS;
        k.salt = 0xA2B3E0C100000028ULL;
        source(k, of(PT_GASGIANT), 290, 1e9, KB_PLANET | KB_COMPANION_WORLD, 0, 0, 1.0);              // a drawn giant this warm (a Wolf-Rayet's far one)
        source(k, WORLD_KINDS, 290, 360, KB_PLANET | KB_COMPANION_WORLD, MAIN_SEQUENCE, 0, 0.015);    // the warm Jupiters: blue as Rayleigh's sky, the red taken by sodium
        palette4(k, RGB(0.20f, 0.40f, 0.86f), RGB(0.16f, 0.32f, 0.78f), RGB(0.25f, 0.48f, 0.88f), RGB(0.19f, 0.36f, 0.72f));
        sentence(k, SR_ONE, "too warm for any cloud to form, a deep blue globe whose clear air the light falls far into");
        draw(k, GF_BAND_COUNT, 5, 8);
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.7f, 0.75f, 0.9f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
        k.giantClass = GC_AZURE; k.oneBarTLo = 290; k.oneBarTHi = 360; k.oneBarLo = 600; k.oneBarHi = 1000;   // the real class's 350-800 K
    }
    {   KindDef& k = K[PT_ALKALI];
        identity(k, PT_ALKALI, "ALKALI GIANT", "ALKALI", "large, locked close to its star: a dark globe of sodium and potassium air that reflects almost nothing, its night side glowing dull red.",
                60000, 125000, false, true, 4, 0.03);
        k.family = "giants"; k.density = 0.08; k.ringChance = 0.05; k.untilted = true;   // the hot Jupiters: swollen (HD 209458b), their tilt taken with their spin
        k.tags = KT_GIANT | KT_GAS_GIANT | KT_NO_SIGHTS;
        k.salt = 0x7A1CA11A00000029ULL;
        source(k, WORLD_KINDS, 360, 420, KB_PLANET, of(STAR_YELLOW, STAR_BLUE_WHITE, STAR_BLUE_GIANT), 1, 0.22);   // TrES-2b (Kipping and Spiegel 2011): darker
        source(k, WORLD_KINDS, 360, 420, KB_PLANET, of(STAR_ORANGE), 1, 0.08);                                     // than coal; fewer round the smaller stars
        source(k, WORLD_KINDS, 360, 420, KB_PLANET, of(STAR_RED_DWARF), 1, 0.015);                                 // (a red dwarf's rare: Johnson et al. 2010)
        palette4(k, RGB(0.14f, 0.17f, 0.30f), RGB(0.18f, 0.15f, 0.27f), RGB(0.12f, 0.16f, 0.25f), RGB(0.19f, 0.17f, 0.23f));   // the little it gives back: a dark slate blue
        sentence(k, SR_ONE, "close to its star, darker than coal with sodium and potassium in its air, its night side glowing dull red");
        draw(k, GF_BAND_COUNT, 3, 3);
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.7f, 0.7f, 0.8f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
        k.giantClass = GC_ALKALI; k.oneBarTLo = 360; k.oneBarTHi = 420; k.oneBarLo = 1100; k.oneBarHi = 1500;   // 900-1,400 K
    }
    {   KindDef& k = K[PT_SILICATE];
        identity(k, PT_SILICATE, "SILICATE GIANT", "SILICATE", "large and glowing, locked close to its star: the hottest giants, under clouds of rock and iron, iron rain on its night side.", 65000,
                130000, false, true, 4, 0.30);
        k.family = "giants"; k.density = 0.07; k.ringChance = 0.03; k.untilted = true;
        k.tags = KT_GIANT | KT_GAS_GIANT | KT_NO_SIGHTS;
        k.salt = 0x51C1CA7E0000002AULL;
        source(k, WORLD_KINDS, 420, 1e9, KB_PLANET, MAIN_SEQUENCE, 1, 0.15);   // WASP-76b (Ehrenreich et al. 2020): its iron rains out on the night side; KELT-9b
        palette4(k, RGB(0.62f, 0.54f, 0.46f), RGB(0.58f, 0.50f, 0.44f), RGB(0.66f, 0.56f, 0.44f), RGB(0.56f, 0.52f, 0.50f));   // clouds of rock: grey-tan
        sentence(k, SR_ONE, "hotter than a furnace under clouds of rock and iron, its night side glowing where the iron rains");
        draw(k, GF_BAND_COUNT, 3, 3);
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.8f, 0.7f, 0.6f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
        k.giantClass = GC_SILICATE; k.oneBarTLo = 420; k.oneBarTHi = 560; k.oneBarLo = 1800; k.oneBarHi = 2600;   // over 1,400 K
    }
    {   KindDef& k = K[PT_GAS_DWARF];
        identity(k, PT_GAS_DWARF, "GAS DWARF", "GAS DWARF", "medium to large: a small world under a deep hydrogen envelope, a pale hazy globe without bands.", 10000, 24000, false, true, 3, 0.40);
        k.family = "giants"; k.density = 0.3; k.ringChance = 0.08;   // 2-4 Earths across at 4-10 Earths' mass
        k.tags = KT_GIANT | KT_GAS_GIANT | KT_NO_SIGHTS;
        k.salt = 0x6D3A2F0D0000002BULL;   // pinned: the probe's Wailie II stays as drawn
        source(k, WORLD_KINDS, 205, 420, KB_PLANET | KB_COMPANION_WORLD, MAIN_SEQUENCE, 0, 0.03);   // the sub-Neptunes over the radius valley (GJ 1214b's flat haze)
        palette4(k, RGB(0.78f, 0.82f, 0.86f), RGB(0.84f, 0.82f, 0.74f), RGB(0.74f, 0.82f, 0.80f), RGB(0.80f, 0.78f, 0.84f));   // pale hazes: grey-blue, cream, sage, lilac
        sentence(k, SR_ONE, "drowned in a deep hydrogen envelope, a pale and hazy globe without bands");
        draw(k, GF_BAND_COUNT, 3, 3);
        k.color2 = tintOf(TINT_BODY_SCALED, RGB(0.85f, 0.85f, 0.85f));
        k.famRep[FAM_ROCK] = MAT_GAS;
        k.ground = GR_GIANT;
        k.giantClass = GC_DWARF;
    }
    // the drifters' own kind. Stevenson (1999): an embryo of Earth's mass thrown out of its system young can keep the hydrogen it was
    // born in, a hundred to ten thousand bars of it, and under that air the heat of its own rock keeps the surface over the melting point of
    // water: an ocean in a night that never ends. Made from a rocky drifter by the dark's table (`drifterBody`, a share by its salt), never
    // by `kindOf`: its moons and its voice are the rocky drifter's it was drawn as
    {   KindDef& k = K[PT_DARK_OCEAN];
        identity(k, PT_DARK_OCEAN, "DARK OCEAN", "DARK OCEAN",
                "medium to large: no sun, kept warm by its own heat under a thick hydrogen sky, a liquid sea in endless night, islands of black basalt steaming at their vents.", 5000, 9000, true,
                true, 2, 0.06);
        k.family = "dark"; k.density = 0.9;   // rock and water
        k.tags = KT_SEA | KT_TWIN;
        k.shapes = KS_BASALTIC | KS_HYDROTHERMAL | KS_JOINTING | KS_PHREATIC;   // the islands' cinder cones, geysers, columns and maars
        k.salt = 0xDA2C0CEA0000002CULL; k.elsewhere = true;   // the share of the rocky drifters that kept their hydrogen
        colour(k, ch(0.05f, 0.02f), ch(0.07f, 0.02f), ch(0.10f, 0.03f));   // black, a little blue in its water
        sentence(k, SR_COIN, "sunless world of one sea kept warm by its own heat under a hydrogen sky hundreds of bars deep, islands of black basalt steaming at their vents",
                 "dark ocean world whose hydrogen air holds in its rock's heat, its sea liquid in a night that never ends round islands of black basalt");
        draw(k, GF_HILL, 2, 3); draw(k, GF_VOLCANOES, 0.45, 0.45); draw(k, GF_MOUNTAIN, 600, 1200);   // the swell, the shields' share of their cells, their height over the floor's
        k.color2 = tintOf(TINT_CONST, RGB(0.03f, 0.06f, 0.08f));
        k.relief.ridgeOp = RO_ADD; k.relief.ridge = 0.2; k.relief.flat = 0.05; k.relief.hill = 0.5; k.relief.mtn = 2.2;   // the islands' flanks
        mix(k, MAT_WATER, MM_CONST, RGB(0.03f, 0.07f, 0.09f)); mix(k, MAT_BASALT, MM_CONST, RGB(0.16f, 0.15f, 0.145f)); mix(k, MAT_SAND, MM_CONST, RGB(0.24f, 0.23f, 0.22f));
        mix(k, MAT_SULPHUR, MM_CONST, RGB(0.78f, 0.74f, 0.56f)); mix(k, MAT_ROCK, MM_CONST, RGB(0.26f, 0.25f, 0.24f));   // the sea, the basalt, its black sand, the vents' crusts, the old flows
        k.famRep[FAM_ROCK] = MAT_BASALT; k.famRep[FAM_SAND] = MAT_SAND; k.accent[0] = MAT_SULPHUR;   // the crusts in a bank of their own
        k.ground = GR_DARK_OCEAN;
        fixedSky(k, RGB(0.004f, 0.005f, 0.008f), RGB(0.012f, 0.014f, 0.020f));   // never lit: its day's colours are its night's
        k.fog = 5000; k.skyDensity = 1.6;
        night(k, RGB(0.002f, 0.003f, 0.005f), RGB(0.008f, 0.010f, 0.014f));
        rocks(k, 0.5, 1.0); k.aurora = AU_THIN;
        k.pressureLo = 80; k.pressureSpan = 520; k.tempOffset = 0; k.tempSwing = 0; k.wind = 5;   // hundreds of bars: the air hardly moves, and no day moves it
        k.airFreezeK = 0; k.seaFreezeK = 0;   // hydrogen never freezes, and the sea's warmth is the heat the hydrogen holds in
        k.starLight = 0.35;   // hundreds of bars of hydrogen scatter the blue out of the starlight (the stars dimmed and reddened)
        k.events = EV_FUMAROLES; k.eventMaterial = MAT_SULPHUR;
    }
    // the eleven new landforms' processes, kind by kind (R-304's eighteen keep theirs: `traitEligible` reads the new bits for the new
    // traits alone). A great shield where no plates carry the hot spot off; Venus' tesserae; Noctis' labyrinth and the inverted rivers on
    // the dry worlds that had water; Mercury's hollows on the airless rock under a hot sun; the crater chains of the cratered moons; the
    // penitentes on the ices under a strong sun and the living worlds' dry heights; the columns, the mud volcanoes, the pingos and the maars
    // where lava, wet sediments, permafrost and groundwater are. The dead felisian worlds keep their living kind's that still show (no
    // blades without a sun to sublime them; a scorched world's pingos thawed and its old rivers stand as ridges)
    for (int t : {PT_VENUSIAN, PT_ROCKY, PT_THINATMO, PT_DESERT, PT_ACIDIC, PT_RUST, PT_DRY_ICE, PT_LICHEN, PT_ARCHEAN}) K[t].shapes |= KS_HOT_SPOT;
    for (int t : {PT_VENUSIAN, PT_ACIDIC, PT_CHTHONIAN, PT_ROCKY}) K[t].shapes |= KS_FOLDING;
    for (int t : {PT_THINATMO, PT_DESERT, PT_RUST, PT_DRY_ICE, PT_LICHEN, PT_ROCKY}) K[t].shapes |= KS_COLLAPSE;
    for (int t : {PT_CRATERED, PT_METAL, PT_BOMBARDED, PT_CARBON, PT_GLASS, PT_ROCKY}) K[t].shapes |= KS_VOLATILES;
    for (int t : {PT_CRATERED, PT_ICY, PT_ROCKY, PT_BOMBARDED, PT_GROOVED_ICE, PT_BRINE_DWARF, PT_TWO_FACED}) K[t].shapes |= KS_TIDAL_BREAKUP;
    for (int t : {PT_EUROPAN, PT_NITROGEN, PT_ICY, PT_DRY_ICE, PT_FROST_DWARF, PT_CRYOVOLCANIC, PT_LICHEN, PT_FELISIAN, PT_ARCHEAN, PT_SUPERHABITABLE, PT_STEPPE,
                  PT_TUNDRA, PT_TERMINATOR, PT_SNOWBALL}) K[t].shapes |= KS_SUBLIMATION;
    for (int t : {PT_FELISIAN, PT_ARCHEAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR, PT_SNOWBALL, PT_VENUSIAN, PT_ROCKY, PT_THINATMO,
                  PT_VOLCANIC, PT_TECTONIC, PT_DESERT, PT_RUST, PT_LICHEN, PT_CHEMOTROPH}) K[t].shapes |= KS_JOINTING;
    for (int t : {PT_FELISIAN, PT_ARCHEAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR, PT_THINATMO, PT_DESERT, PT_RUST, PT_LICHEN,
                  PT_CHEMOTROPH, PT_STEAM, PT_HYDROCARBON}) K[t].shapes |= KS_MUD;
    for (int t : {PT_THINATMO, PT_DESERT, PT_RUST, PT_DRY_ICE, PT_LICHEN, PT_SALT, PT_SCORCHED_FELISIAN}) K[t].shapes |= KS_EXHUMATION;
    for (int t : {PT_FELISIAN, PT_ARCHEAN, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR, PT_SNOWBALL, PT_LICHEN, PT_THINATMO, PT_DRY_ICE}) K[t].shapes |= KS_PERMAFROST;
    for (int t : {PT_FELISIAN, PT_ARCHEAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR, PT_STEAM, PT_CHEMOTROPH, PT_THINATMO, PT_DESERT,
                  PT_LICHEN, PT_TECTONIC, PT_RUST}) K[t].shapes |= KS_PHREATIC;
    // the worlds it rains on off the living ground, their landforms' mean moisture: a hydrocarbon world's methane rain rare (Titan's dry
    // tropics hold its dune seas), an ammonia world's and an acid world's more often, a steam world's the wettest
    K[PT_HYDROCARBON].landMoist = 0.25; K[PT_AMMONIA].landMoist = 0.4; K[PT_ACIDIC].landMoist = 0.55; K[PT_STEAM].landMoist = 0.6;
    K[PT_FROZEN_FELISIAN].shapes |= K[PT_FELISIAN].shapes & (KS_JOINTING | KS_MUD | KS_PERMAFROST | KS_PHREATIC);
    K[PT_SCORCHED_FELISIAN].shapes |= K[PT_FELISIAN].shapes & (KS_JOINTING | KS_MUD | KS_PHREATIC);
    // the water, the sky, the life and the ground's traits, kind by kind. The tides on the seas with shores (water, methane, acid and
    // ammonia: Titan's lakes have Saturn's), the hot springs where groundwater meets a heat (the living worlds, the snowball's ice, the acid
    // world's Dallol, the tectonic and the desert worlds), the pink lakes in the dry basins of the worlds with life (the archean's halophiles
    // first of all), the milky seas, the blooms and the reefs in the seas of life (the reefs warm: a fact of the world's, `BodyGen::make`); the
    // noctilucent clouds where a cold upper air holds water or carbon dioxide (Mars' mesospheric clouds), the halos where ice crystals hang
    // under a sun the sky shows, the airglow in the oxygen airs, the fog seas over the valleys of the humid airs that see a dawn, the aurorae
    // in every air the aurora lights (a strong field the world's fact, R-402); the fungal forests where forests are, the petrified ones on the
    // worlds that dried; the rainbow strata in the sediments the water laid and the air oxidised, the gypsum dunes where an evaporite basin
    // meets the wind, the meteorites where a dry or frozen ground keeps them and shows them (deserts, thin airs, ice sheets: Antarctica's blue ice)
    constexpr int LIVING[] = {PT_FELISIAN, PT_ARCHEAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR};
    for (int t : LIVING) K[t].shapes |= KS_TIDES | KS_SPRINGS | KS_GLOWING_SEA | KS_MESOSPHERE | KS_ICE_CRYSTALS | KS_AURORAE | KS_BLOOMS;
    for (int t : {PT_HYDROCARBON, PT_ACIDIC, PT_AMMONIA}) K[t].shapes |= KS_TIDES;
    for (int t : {PT_SNOWBALL, PT_ACIDIC, PT_TECTONIC, PT_DESERT}) K[t].shapes |= KS_SPRINGS;
    for (int t : {PT_FELISIAN, PT_STEPPE, PT_ARCHEAN, PT_TERMINATOR}) K[t].shapes |= KS_HALOPHILES;
    for (int t : {PT_EYEBALL, PT_OCEAN}) K[t].shapes |= KS_GLOWING_SEA | KS_BLOOMS;
    for (int t : {PT_OCEAN, PT_SNOWBALL, PT_THINATMO, PT_DRY_ICE, PT_LICHEN, PT_DESERT, PT_QUARTZ, PT_RUST}) K[t].shapes |= KS_MESOSPHERE;
    K[PT_TERMINATOR].shapes &= ~KS_MESOSPHERE;   // a fixed sun: no twilight but along the terminator
    for (int t : {PT_OCEAN, PT_QUARTZ, PT_THINATMO, PT_DESERT, PT_SALT, PT_RUST, PT_LICHEN, PT_EYEBALL, PT_SNOWBALL, PT_FROZEN_FELISIAN, PT_DRY_ICE, PT_AMMONIA,
                  PT_HYCEAN}) K[t].shapes |= KS_ICE_CRYSTALS;
    for (int t : {PT_FELISIAN, PT_QUARTZ, PT_RUST, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR, PT_EYEBALL, PT_OCEAN, PT_LICHEN, PT_DESERT})
        K[t].shapes |= KS_OXYGEN_AIR;
    for (int t : {PT_FELISIAN, PT_ARCHEAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_SNOWBALL}) K[t].shapes |= KS_VALLEY_FOG;
    for (int t : {PT_THINATMO, PT_QUARTZ, PT_OCEAN, PT_TECTONIC, PT_DESERT, PT_ACIDIC, PT_CHTHONIAN, PT_SALT, PT_RUST, PT_LICHEN, PT_EYEBALL, PT_HYCEAN, PT_AMMONIA,
                  PT_SNOWBALL, PT_CHEMOTROPH, PT_FROZEN_FELISIAN, PT_SCORCHED_FELISIAN, PT_DRY_ICE}) K[t].shapes |= KS_AURORAE;
    for (int t : {PT_FELISIAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TERMINATOR}) K[t].shapes |= KS_CORALS;
    for (int t : {PT_FELISIAN, PT_HOTHOUSE, PT_SUPERHABITABLE, PT_STEPPE, PT_TUNDRA, PT_TERMINATOR}) K[t].shapes |= KS_FUNGI;
    for (int t : {PT_DESERT, PT_RUST, PT_SALT, PT_STEPPE}) K[t].shapes |= KS_DRIED_FORESTS;
    for (int t : {PT_FELISIAN, PT_STEPPE, PT_TERMINATOR, PT_DESERT, PT_RUST, PT_SALT, PT_THINATMO, PT_LICHEN, PT_SCORCHED_FELISIAN}) K[t].shapes |= KS_SEDIMENTS;
    for (int t : {PT_FELISIAN, PT_STEPPE, PT_TERMINATOR, PT_THINATMO, PT_DESERT, PT_SALT, PT_RUST, PT_LICHEN, PT_SCORCHED_FELISIAN}) K[t].shapes |= KS_GYPSUM;
    for (int t : {PT_THINATMO, PT_DESERT, PT_SALT, PT_RUST, PT_LICHEN, PT_DRY_ICE, PT_CHTHONIAN, PT_SNOWBALL, PT_TUNDRA, PT_FROZEN_FELISIAN, PT_SCORCHED_FELISIAN,
                  PT_WANDERER}) K[t].shapes |= KS_METEORITES;   // where wind or ice gathers them on a ground they stand out of (an airless world's every stone is one)
    // the sounds of the ground. The singing dunes where dunes of dry sand stand under an air (the deserts and the dry lowlands of the
    // living worlds: the Namib's, the Mojave's, the Gobi's Khongoryn Els; Mars' and Titan's dune seas; not a salt world's gypsum, which is
    // silent as White Sands is, nor a lichen world's crusted dust, nor a scorched world's: its dunes were never dry); the singing ice on the cold worlds with an air and ice to stand on (the sea
    // and lake ice, the sheets, the caps); the quakes where a crust still moves under an air (the plates' worlds: Earth's, Venus', Mars'
    // marsquakes) and on the moons a giant's tides keep hot (Io's, Enceladus', Triton's), but not the tectonic world's, whose quakes are its
    // kind's, nor the airless rock's and ice's (the Moon's shallow moonquakes come a few a year, not every few minutes)
    for (int t : {PT_DESERT, PT_RUST, PT_THINATMO, PT_HYDROCARBON, PT_FELISIAN, PT_STEPPE, PT_TERMINATOR, PT_ARCHEAN}) K[t].shapes |= KS_SINGING_SAND;
    for (int t : {PT_SNOWBALL, PT_TUNDRA, PT_FROZEN_FELISIAN, PT_TERMINATOR, PT_EYEBALL, PT_AMMONIA, PT_THINATMO, PT_LICHEN, PT_DRY_ICE}) K[t].shapes |= KS_ICE_SONG;
    for (int t = 0; t < PT_COUNT; t++)
        if (K[t].landable && (K[t].tags & KT_PLATES) && K[t].pressureLo + K[t].pressureSpan > 0.001 && !(K[t].events & EV_QUAKES)) K[t].shapes |= KS_SEISMIC;
    for (int t : {PT_VOLCANIC, PT_TIGER_STRIPED, PT_CRYOVOLCANIC}) K[t].shapes |= KS_SEISMIC;
    // what freezes in the dark. A carbon dioxide or a sulphurous air falls as snow under 150 K at the pressures the game's thin airs have
    // (Mars' poles at 145 K); a nitrogen air under a methane haze holds to 60 (Titan's at 94 K); hydrogen never. Water's seas freeze at 271 K,
    // a brine's at 250, ammonia water's at 176, methane's at 90, a tar sea sets under 300
    for (int t = 0; t < PT_COUNT; t++) if (K[t].tags & KT_GIANT) K[t].airFreezeK = 0;
    K[PT_HYDROCARBON].airFreezeK = 60; K[PT_HYCEAN].airFreezeK = 0;
    K[PT_HYDROCARBON].seaFreezeK = 90; K[PT_AMMONIA].seaFreezeK = 176; K[PT_ICY].seaFreezeK = 250; K[PT_CARBON].seaFreezeK = 300;
    return K;
}

constexpr bool sameText(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
constexpr int textLength(const char* a) { int n = 0; while (a[n]) n++; return n; }
constexpr bool kindsOk(const std::array<KindDef, PT_COUNT>& K) {
    for (int i = 0; i < PT_COUNT; i++) {
        const KindDef& k = K[i];
        if (k.id != i || !k.name || !k.shortName || !k.description || !k.sentence[0] || textLength(k.shortName) > 10) return false;
        if (k.minRadiusKm <= 0 || k.maxRadiusKm < k.minRadiusKm || k.nDraws > MAX_GEN_DRAWS || k.nPalette > MAX_MAT_MIXES || k.nCraters > MAX_CRATER_GRIDS) return false;
        if ((k.accent[0] >= 0 || k.accent[1] >= 0) && (k.tags & KT_FLORA)) return false;   // a kind with flora keeps its grass and forest banks
        if (k.sentenceRule == SR_SEAS || k.sentenceRule == SR_DEAD_SEAS) { if (!k.sentence[1] || !k.sentence[2]) return false; }
        else if (k.sentenceRule != SR_ONE && !k.sentence[1]) return false;
        if (k.landable && k.ground == GR_NONE) return false;
        if (k.floraFamily >= 10 || !(k.latGradient > 0) || !(k.seaDepth > 0) || ((k.tags & KT_MICROBIAL) && (k.tags & KT_FLORA))) return false;   // P-02: ten silhouettes; microbes are no flora
        for (int j = 0; j < i; j++) if (sameText(k.name, K[j].name) || sameText(k.shortName, K[j].shortName)) return false;
        if (k.giantClass != GC_NONE && (!(k.tags & KT_GIANT) || k.landable || k.giantClass >= GC_COUNT)) return false;   // a class is a giant's
        if (k.oneBarLo > 0 && !(k.oneBarTHi > k.oneBarTLo && k.oneBarHi >= k.oneBarLo && k.giantClass != GC_NONE)) return false;
        if (k.landMoist >= 0 && (!(k.tags & KT_WET) || k.ground == GR_FELISIAN || k.landMoist > 1)) return false;   // a wet world's off the living ground
        if (i < PT_DRAWN_COUNT) { if (k.nSources != 0 || k.family || k.giantClass != GC_NONE) return false; continue; }   // a drawn kind has no sources
        if ((k.tags & KT_GIANT) && k.giantClass == GC_NONE) return false;   // a made giant has its class
        // a made kind has no weight in the draw (no zone, no rule that adds), a family, a salt of its own and its sources over drawn kinds
        // (P-04: none where it is made only where no system is)
        if (!k.family || !k.salt || (k.nSources < 1) != k.elsewhere || k.nSources > MAX_KIND_SOURCES) return false;
        // a dead world's
        if (k.thenKind >= 0 && (k.thenKind >= PT_DRAWN_COUNT || k.cover == DC_NONE || !(k.tags & KT_PEOPLE) || !(k.peopleShare > 0) || !k.peopleSalt || (k.tags & (KT_FLORA | KT_FAUNA)))) return false;
        if ((k.cover != DC_NONE) != (k.thenKind >= 0)) return false;
        for (int z = 0; z < 5; z++) if (k.zones[z] != 0) return false;
        for (int r = 0; r < WR_COUNT; r++) if (k.rules[r].mul != 1 || k.rules[r].add != 0) return false;
        for (int s = 0; s < k.nSources; s++) if (!k.sources[s].from || (k.sources[s].from >> PT_DRAWN_COUNT) || !k.sources[s].bodies || !(k.sources[s].share > 0)) return false;
        // a planet's kinds name a moon's source; P-06: so do its facts among its planet's moons
        for (int s = 0; s < k.nSources; s++) if (((k.sources[s].parents || k.sources[s].moon) && k.sources[s].bodies != KB_MOON) || (k.sources[s].parents >> PT_DRAWN_COUNT)) return false;
        for (int j = PT_DRAWN_COUNT; j < i; j++) if (K[j].salt == k.salt) return false;
    }
    return true;
}
constexpr std::array<KindDef, PT_COUNT> BUILT_KINDS = buildKinds();
// B-430: the check reads a table built for it: GCC 9.4 copies a constexpr variable whole at each read of it (6.6 GB for BUILT_KINDS's)
static_assert(kindsOk(buildKinds()), "the kind table: a row missing, out of order, or incomplete");

constexpr void trait(std::array<TraitDef, TR_COUNT>& T, int id, const char* name, const char* phrase, int cls, uint64_t needs, bool colour = false, int excludes = TR_NONE) {
    T[id] = TraitDef{id, name, phrase, cls, needs, colour, excludes};
}
// Q-01: a new slot's landform (it moves the ground: never on a world with a people)
constexpr void landform(std::array<TraitDef, TR_COUNT>& T, int id, const char* name, const char* phrase, uint64_t needs, int excludes = TR_NONE) {
    T[id] = TraitDef{id, name, phrase, TC_LANDFORM, needs, false, excludes, TK_LANDFORM, true};
}
// Q-02: a new slot's trait of the water, the sky, the life or the ground (`moves`: it moves the ground or what stands on it, or takes one of the
// peoples' palette banks on the ground: never on a world with a people nor the home world)
constexpr void quality(std::array<TraitDef, TR_COUNT>& T, int id, const char* name, const char* phrase, uint64_t needs, int kind, bool moves, bool colour = false, int excludes = TR_NONE) {
    T[id] = TraitDef{id, name, phrase, TC_WORLD, needs, colour, excludes, kind, moves};
}
// D-04: a drifter's own (`TK_DARK`): given, never drawn (`moves`: it moves the ground or what stands on it)
constexpr void dark(std::array<TraitDef, TR_COUNT>& T, int id, const char* name, const char* phrase, bool moves) {
    T[id] = TraitDef{id, name, phrase, TC_GIVEN, 0, false, TR_NONE, TK_DARK, moves};
}
constexpr std::array<TraitDef, TR_COUNT> buildTraits() {
    std::array<TraitDef, TR_COUNT> T{};
    trait(T, TR_NONE, "", "", TC_GIVEN, 0);
    trait(T, TR_CANYONS, "CANYON LANDS", ", cut by canyons", TC_LANDFORM, KS_INCISION);
    trait(T, TR_MESAS, "BADLANDS", ", its dry country carved into mesas and buttes", TC_LANDFORM, KS_LAYERED);
    trait(T, TR_KARST, "KARST TOWERS", ", stone towers rising from its wet lowlands", TC_LANDFORM, KS_SOLUTION);
    trait(T, TR_RIFT, "GREAT RIFT", ", split by a great rift valley", TC_LANDFORM, KS_EXTENSION);
    trait(T, TR_ESCARPMENTS, "ESCARPMENTS", ", stepped by long escarpments", TC_LANDFORM, KS_SCARPS);
    trait(T, TR_GLACIAL, "GLACIATED", ", scoured by glaciers, its cold coasts fjords", TC_LANDFORM, KS_GLACIERS);
    trait(T, TR_INSELBERGS, "INSELBERGS", ", lone mountains standing on its plains", TC_LANDFORM, KS_PEDIMENTS);
    trait(T, TR_CINDER_FIELD, "CINDER FIELDS", ", fields of cinder cones and fresh lava", TC_LANDFORM, KS_BASALTIC);
    trait(T, TR_CHAOS, "CHAOS TERRAIN", ", its crust broken into tilted blocks", TC_LANDFORM, KS_BRITTLE);
    trait(T, TR_POLYGONS, "PATTERNED GROUND", ", its cold ground patterned in polygons", TC_LANDFORM, KS_FROST);
    trait(T, TR_YARDANGS, "YARDANGS", ", ridges carved by the wind", TC_LANDFORM, KS_WIND);
    trait(T, TR_ERG, "DUNE SEAS", ", seas of giant dunes", TC_LANDFORM, KS_WIND);
    trait(T, TR_SALT_FLATS, "SALT FLATS", ", white salt flats in its basins", TC_LANDFORM, KS_EVAPORITES);
    trait(T, TR_TRAPS, "TRAP TERRACES", ", stepped terraces of old lava", TC_LANDFORM, KS_FLOOD_LAVA);
    trait(T, TR_GREAT_BASIN, "GREAT BASIN", ", one vast impact basin", TC_LANDFORM, KS_IMPACTS);
    trait(T, TR_CORONAE, "CORONAE", ", ringed by coronae", TC_LANDFORM, KS_PLUMES);
    trait(T, TR_SPIRES, "SPIRE FIELDS", ", fields of spires", TC_LANDFORM, KS_SPIRES, false, TR_PENITENTES);   // Q-01: its ice spires are penitentes
    trait(T, TR_GEYSERS, "GEYSER BASINS", ", geyser basins steaming", TC_LANDFORM, KS_HYDROTHERMAL);
    trait(T, TR_ARCHIPELAGO, "ARCHIPELAGO", ", an archipelago of islands", TC_WORLD, KS_OPEN_SEA, false, TR_PANGAEA);
    trait(T, TR_PANGAEA, "PANGAEA", ", one great continent", TC_WORLD, KS_CONTINENTS, false, TR_ARCHIPELAGO);
    trait(T, TR_LAKELAND, "LAKE COUNTRY", ", lakes everywhere", TC_WORLD, KS_CONTINENTS);
    // its own process (every kind with an open sea had it; a hothouse has the sea and not the freeze)
    trait(T, TR_SNOWBALL, "SNOWBALL", ", frozen down to the middle latitudes", TC_WORLD, KS_FREEZE);
    trait(T, TR_EXOTIC_SEAS, "EXOTIC SEAS", ", seas of something that is not water", TC_WORLD, KS_COLD_SEAS);
    trait(T, TR_STORMS, "STORM WORLD", ", storms that never end", TC_WORLD, KS_STORMS);
    trait(T, TR_HAZE, "HAZE", ", under a thick haze", TC_WORLD, KS_HAZE);
    trait(T, TR_GIANT_FLORA, "GIANT FLORA", ", flora of giant size", TC_WORLD, KS_FLORA, false, TR_DEAD_FOREST);
    trait(T, TR_LUMINOUS_FLORA, "LUMINOUS FLORA", ", flora that glows at night", TC_WORLD, KS_FLORA);
    trait(T, TR_DEAD_FOREST, "DEAD FORESTS", ", its forests dead and grey", TC_WORLD, KS_FLORA, false, TR_GIANT_FLORA);
    trait(T, TR_RED_SOIL, "RED SOILS", ", red soils", TC_WORLD, KS_SOILS, true);
    trait(T, TR_BLACK_SAND, "BLACK SANDS", ", black sands", TC_WORLD, KS_SOILS, true);
    trait(T, TR_CHALK, "CHALK LANDS", ", chalk-white ground", TC_WORLD, KS_SOILS, true);
    trait(T, TR_GLASSED, "GLASSED", ", its plains fused to glass by the blast that made its star", TC_GIVEN, 0);
    trait(T, TR_CIVILISATION, "DEAD CIVILISATION", ", the ruins of a people that is gone", TC_GIVEN, 0);
    landform(T, TR_GREAT_SHIELD, "GREAT SHIELD", ", one great shield volcano towering over its plains", KS_HOT_SPOT);   // Olympus Mons
    landform(T, TR_TESSERAE, "TESSERAE", ", its oldest highlands woven with ridges and troughs", KS_FOLDING);   // Venus' Alpha and Ovda Regio
    landform(T, TR_LABYRINTH, "LABYRINTH", ", a labyrinth of fault troughs cut into its heights", KS_COLLAPSE);   // Noctis Labyrinthus
    landform(T, TR_HOLLOWS, "HOLLOWS", ", bright hollows where its rock breathed out a volatile", KS_VOLATILES);   // Mercury's
    landform(T, TR_CRATER_CHAINS, "CRATER CHAINS", ", lines of craters where broken comets fell", KS_TIDAL_BREAKUP);   // Ganymede's and Callisto's catenae
    landform(T, TR_PENITENTES, "PENITENTES", ", its ice standing in blades toward the sun", KS_SUBLIMATION, TR_SPIRES);   // the high Andes; Pluto's bladed terrain
    landform(T, TR_COLUMNS, "COLUMNAR BASALT", ", cliffs of basalt cracked into columns", KS_JOINTING);   // the Giant's Causeway
    landform(T, TR_MUD_VOLCANOES, "MUD VOLCANOES", ", mud volcanoes bubbling on its plains", KS_MUD);   // Azerbaijan's
    landform(T, TR_INVERTED_RIVERS, "INVERTED RIVERS", ", the beds of its old rivers standing as ridges", KS_EXHUMATION);   // Mars' Aeolis Dorsa
    landform(T, TR_PINGOS, "PINGOS", ", hills of ice heaved up on its frozen plains", KS_PERMAFROST);   // the Mackenzie delta's
    landform(T, TR_MAARS, "MAARS", ", round craters blown by steam", KS_PHREATIC);   // the Eifel's
    quality(T, TR_GREAT_TIDES, "GREAT TIDES", ", great tides running out over kilometres of flats", KS_TIDES, TK_WATER, true);   // the Bay of Fundy's
    quality(T, TR_HOT_SPRINGS, "HOT SPRINGS", ", hot springs in terraced pools of many colours", KS_SPRINGS, TK_WATER, true);   // Pamukkale, the Grand Prismatic
    quality(T, TR_PINK_LAKES, "PINK LAKES", ", its salt lakes pink", KS_HALOPHILES, TK_WATER, true);   // Lake Hillier, Lake Retba
    quality(T, TR_MILKY_SEAS, "MILKY SEAS", ", seas that glow milky white at night", KS_GLOWING_SEA, TK_WATER, false);   // Miller et al. 2005
    quality(T, TR_NOCTILUCENT, "NOCTILUCENT CLOUDS", ", silver clouds shining high in its summer twilights", KS_MESOSPHERE, TK_SKY, false);
    quality(T, TR_ICE_HALOS, "ICE HALOS", ", rings and mock suns round its sun", KS_ICE_CRYSTALS, TK_SKY, false);
    quality(T, TR_AIRGLOW, "AIRGLOW", ", its night sky glowing green", KS_OXYGEN_AIR, TK_SKY, false);
    quality(T, TR_FOG_SEAS, "FOG SEAS", ", its valleys filled with fog at dawn", KS_VALLEY_FOG, TK_SKY, false);
    quality(T, TR_GREAT_AURORAE, "GREAT AURORAE", ", aurorae reaching down to its tropics", KS_AURORAE, TK_SKY, false);
    quality(T, TR_REEFS, "BARRIER REEFS", ", reefs ringing its coasts", KS_CORALS, TK_LIFE, true);   // the Great Barrier Reef
    quality(T, TR_FUNGAL, "FUNGAL FORESTS", ", forests of pale fungi", KS_FUNGI, TK_LIFE, true, false, TR_DEAD_FOREST);
    quality(T, TR_PETRIFIED, "PETRIFIED FORESTS", ", the trunks of its old forests lying turned to stone", KS_DRIED_FORESTS, TK_LIFE, true);   // Arizona's
    quality(T, TR_BLOOMS, "ALGAL BLOOMS", ", its seas streaked with blooms", KS_BLOOMS, TK_LIFE, true);   // red tides
    quality(T, TR_STRATA, "RAINBOW STRATA", ", hills banded in many colours", KS_SEDIMENTS, TK_GROUND, true);   // Zhangye Danxia, the Painted Hills
    quality(T, TR_GYPSUM, "GYPSUM SANDS", ", dunes of white gypsum", KS_GYPSUM, TK_GROUND, true, true);   // White Sands
    quality(T, TR_METEORITES, "METEORITE FIELDS", ", fields strewn with iron meteorites", KS_METEORITES, TK_GROUND, true);   // Campo del Cielo
    // the sounds of the ground (they move nothing: a world with a people and the home world may take them)
    quality(T, TR_SINGING_DUNES, "SINGING DUNES", ", dunes that boom when their sand slides", KS_SINGING_SAND, TK_SOUND, false);   // the Namib's, Kelso's, Khongoryn Els
    quality(T, TR_SINGING_ICE, "SINGING ICE", ", ice that sings as it cracks and settles", KS_ICE_SONG, TK_SOUND, false);   // lake ice's pings and whoops
    quality(T, TR_QUAKES, "QUAKES", ", its ground shaken by quakes every few minutes", KS_SEISMIC, TK_SOUND, false);
    // the drifters' own, given by what a body in the dark is (`darkTraitsOf`), never drawn: its air fallen as snow, a young interior's lava
    // lakes, a dead people's lights still burning (D-05's), a dark ocean's life
    dark(T, TR_FROZEN_AIR, "FROZEN AIR", ", its air fallen as snow on its ground", true);
    dark(T, TR_EMBERS, "EMBERS", ", lakes of lava glowing in its endless night", true);
    dark(T, TR_LIGHTS_LEFT_ON, "LIGHTS LEFT ON", ", the lights of a dead people's cities still burning", false);
    dark(T, TR_LIGHTS_IN_WATER, "LIGHTS IN THE WATER", ", faint lights drifting in its dark sea", false);
    return T;
}
constexpr bool traitsOk(const std::array<TraitDef, TR_COUNT>& T) {
    for (int i = 0; i < TR_COUNT; i++) {
        const TraitDef& t = T[i];
        if (t.id != i || !t.name || !t.phrase || (t.cls != TC_GIVEN && !t.needs)) return false;
        if (t.excludes != TR_NONE && T[t.excludes].excludes != i && (t.kind == TK_FIRST || T[t.excludes].kind != TK_FIRST)) return false;   // an exclusion goes
        // both ways (Q-02: a new slot's of one of R-304's one way, the new slots being drawn after the first three)
        if (t.kind < TK_FIRST || t.kind >= TK_COUNT || (t.kind != TK_FIRST) != (i > TR_CIVILISATION) || (t.kind != TK_FIRST && (t.cls == TC_GIVEN) != (t.kind == TK_DARK))) return false;   // R-304's
        // traits first (the first three slots draw them alone), the new slots' after the given ones, each of a kind (D-04: the drifters' own given)
        if ((t.kind == TK_LANDFORM) != (i > TR_CIVILISATION && t.cls == TC_LANDFORM) || (t.kind == TK_LANDFORM && !t.moves)) return false;
    }
    return true;
}
constexpr std::array<TraitDef, TR_COUNT> BUILT_TRAITS = buildTraits();
static_assert(traitsOk(BUILT_TRAITS), "the trait table: a row missing, out of order, or incomplete");
}   // namespace

const char* const GROUND_NAMES[] = {"none", "felisian", "cratered", "molten", "venusian", "rocky", "thin air", "ice", "quartz", "ocean", "europan",
                                    "tectonic", "desert", "hydrocarbon", "bombarded", "acidic", "metal", "volcanic", "carbon", "giant",
                                    "magma", "chthonian", "glass", "steam", "salt", "lichen", "eyeball", "ammonia", "chemotroph",
                                    "nitrogen", "cryovolcanic", "dry ice", "wanderer", "brine", "frost",
                                    "grooved", "two-faced", "tiger", "patchwork", "sponge", "dark ocean"};
static_assert(sizeof(GROUND_NAMES) / sizeof(GROUND_NAMES[0]) == GR_COUNT, "a name for every ground function");
const std::array<KindDef, PT_COUNT> KINDS = BUILT_KINDS;
const std::array<TraitDef, TR_COUNT> TRAITS = BUILT_TRAITS;

int landableTypeCount() { int n = 0; for (int i = 0; i < PT_COUNT; i++) if (KINDS[i].landable) n++; return n; }
