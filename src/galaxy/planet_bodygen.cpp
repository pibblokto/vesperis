// E-07: a body's generator (`BodyGen::make`): the material ramps and families, the palette, a giant's globe, the flora's
// colour, the drifters' traits, the traits a world takes; the climate's temperature and the ice sheets
#include "planet_internal.h"
#include "peoples/peoples.h"

constexpr double MatRamp::pos[6];

MatRamp materialRamp(const RGB& c, const RGB* skyLight, double day) {
    RGB white(1, 1, 1);
    MatRamp r;
    RGB dark = c * 0.25f;
    if (skyLight) dark = lerp(dark, *skyLight * 0.55f + c * 0.12f, (float)(0.6 * day));
    r.stop[0] = RGB(0, 0, 0); r.stop[1] = dark; r.stop[2] = lerp(dark, c, 0.55f); r.stop[3] = c; r.stop[4] = lerp(c, white, 0.5f); r.stop[5] = white;
    return r;
}

RGB materialRampColor(const MatRamp& r, double stop) {
    if (stop <= 0) return r.stop[0];
    if (stop >= 63) return r.stop[5];
    int k = 0;
    while (k < 4 && stop > MatRamp::pos[k + 1]) k++;
    return lerp(r.stop[k], r.stop[k + 1], (float)((stop - MatRamp::pos[k]) / (MatRamp::pos[k + 1] - MatRamp::pos[k])));
}

int matFamily(int material) {
    switch (material) {
        case MAT_WATER: case MAT_GLASS: case MAT_BRINE: return FAM_WATER;   // S-03: the glass takes the water family's bank (bank 2, the secondary) in the glass's colour; a glassed world has no sea
        case MAT_FOREST: return FAM_FOREST;
        case MAT_GRASS: case MAT_MATS: case MAT_MOSS: return FAM_GRASS;   // P-02: where they are no kind's accent, the mats and the moss are the grass's
        case MAT_SAND: case MAT_DUST: case MAT_SULPHUR: case MAT_QUARTZ: case MAT_MUD: return FAM_SAND;
        case MAT_SNOW: case MAT_ICE: case MAT_CLOUD: case MAT_SALT: case MAT_POTASH: return FAM_SNOW;
        default: return FAM_ROCK;   // rock, basalt, lava, gas, metal, graphite; P-02: the stromatolites and the crusts
    }
}

// P-00: a kind's accents are drawn in the grass and forest families (their banks: 10 and 3 on the ground, the globe's own)
int matFamilyOn(int type, int material) {
    const KindDef& K = KINDS[type];
    if (material == K.accent[0]) return FAM_GRASS;
    if (material == K.accent[1]) return FAM_FOREST;
    return matFamily(material);
}

int familyRep(int type, int family) {
    const KindDef& K = KINDS[type];
    if (family == FAM_GRASS && K.accent[0] >= 0) return K.accent[0];
    if (family == FAM_FOREST && K.accent[1] >= 0) return K.accent[1];
    return K.famRep[family >= 0 && family < FAM_COUNT ? family : FAM_ROCK];
}

// N1-01 (B-202): one palette per body. The base hue is the body's colour seen from afar (typeColor); each
// material mixes its own tint with it, so a red thin-atmosphere world is red underfoot and a felisian
// world is green forest, straw grass, tan sand and white snow in every view. P-00: a kind's own mixes are its row's
static void materialPalette(BodyGen& g) {
    using namespace tint;
    RGB base = g.color;
    const KindDef& K = KINDS[g.type];
    bool atmo = K.atmosphere;
    RGB* m = g.matColor;
    RGB veg = (g.vegColor.r + g.vegColor.g + g.vegColor.b > 0.05f) ? g.vegColor : RGB(0.25f, 0.5f, 0.15f);   // worlds without flora still own a colour
    m[MAT_ROCK] = lerp(ROCK_GREY, base, 0.5f);
    m[MAT_BASALT] = lerp(BASALT_DARK, base, 0.25f);
    m[MAT_DUST] = lerp(DUST_WARM, base, 0.45f);
    m[MAT_SAND] = lerp(SAND_TAN, base, 0.3f);
    m[MAT_GRASS] = lerp(veg, RGB(0.72f, 0.66f, 0.32f), 0.4f);   // straw in the green
    m[MAT_FOREST] = veg * 0.85f;
    m[MAT_SNOW] = atmo ? lerp(SNOW_WHITE, g.skyTint, 0.1f) : SNOW_WHITE;
    m[MAT_ICE] = atmo ? lerp(ICE_BLUE, g.skyTint, 0.15f) : ICE_BLUE;
    m[MAT_WATER] = atmo ? lerp(WATER, g.skyTint * 0.55f, 0.25f) : WATER;
    m[MAT_LAVA] = LAVA;
    m[MAT_CLOUD] = CLOUD_WHITE;
    m[MAT_QUARTZ] = lerp(QUARTZ_PALE, base, 0.3f);
    m[MAT_METAL] = lerp(METAL_GREY, base, 0.5f);
    m[MAT_SULPHUR] = lerp(SULPHUR_YELLOW, base, 0.4f);
    m[MAT_GRAPHITE] = lerp(GRAPHITE, base, 0.3f);
    m[MAT_GAS] = base;
    m[MAT_SALT] = lerp(RGB(0.93f, 0.92f, 0.87f), base, 0.08f);   // R-305: playas and sinter
    m[MAT_GLASS] = RGB(0.50f, 0.66f, 0.52f);   // S-03: pale green glass (the glassed worlds set their own below)
    m[MAT_MUD] = lerp(RGB(0.42f, 0.36f, 0.30f), base, 0.2f);   // P-01: the kinds that use them set their own
    m[MAT_POTASH] = RGB(0.76f, 0.46f, 0.42f);
    m[MAT_BRINE] = lerp(RGB(0.46f, 0.60f, 0.30f), base, 0.15f);
    m[MAT_MATS] = RGB(0.46f, 0.20f, 0.34f);   // P-02: the kinds that use them set their own
    m[MAT_STROMATOLITE] = lerp(RGB(0.58f, 0.54f, 0.40f), base, 0.2f);
    m[MAT_CRUST] = RGB(0.78f, 0.45f, 0.14f);
    m[MAT_MOSS] = RGB(0.30f, 0.40f, 0.15f);
    for (int i = 0; i < K.nPalette; i++) {
        const MatMix& x = K.palette[i];
        switch (x.mode) {
            case MM_BASE_MIX: m[x.mat] = lerp(x.c, base, x.t); break;
            case MM_SKY_MIX: m[x.mat] = lerp(x.c, g.skyTint, x.t); break;
            case MM_C2_MIX: m[x.mat] = lerp(g.color2, base, x.t); break;
            case MM_MIX: m[x.mat] = lerp(x.c, x.d, x.t); break;
            case MM_BASE: m[x.mat] = base; break;
            default: m[x.mat] = x.c; break;
        }
    }
    // P-02: the microbes' hue is the world's (the purple of the retinal mats, the orange of the carotenoids, the green-bronze of the
    // first photosynthesis: DasSarma and Schwieterman 2018), as is the lichen's (orange, the map lichen's yellow-green, a rust red)
    if (K.tags & KT_MICROBIAL) {
        static const RGB mats[3] = {RGB(0.46f, 0.20f, 0.34f), RGB(0.62f, 0.34f, 0.14f), RGB(0.44f, 0.46f, 0.18f)};
        m[MAT_MATS] = mats[(int)(unitFromHash(mix64(g.seed ^ 0x3A75ULL)) * 3) % 3];
    }
    if (K.tags & KT_VENT_LIFE) {   // P-03: the vents' mats pale (the white of the sulphur oxidisers, a cream, the pink of the thermophiles)
        static const RGB vent[3] = {RGB(0.86f, 0.85f, 0.80f), RGB(0.86f, 0.76f, 0.56f), RGB(0.84f, 0.60f, 0.62f)};
        m[MAT_MATS] = vent[(int)(unitFromHash(mix64(g.seed ^ 0xC4E3ULL)) * 3) % 3];
    }
    if (K.accent[0] == MAT_CRUST) {
        static const RGB crusts[3] = {RGB(0.78f, 0.45f, 0.14f), RGB(0.60f, 0.64f, 0.24f), RGB(0.62f, 0.28f, 0.16f)};
        m[MAT_CRUST] = crusts[(int)(unitFromHash(mix64(g.seed ^ 0xC2057ULL)) * 3) % 3];
    }
    // R-304: the colour traits tint the ground family, the exotic seas colour their liquid
    if (g.hasTrait(TR_RED_SOIL)) { RGB red(0.62f, 0.30f, 0.16f); m[MAT_ROCK] = lerp(m[MAT_ROCK], red, 0.5f); m[MAT_SAND] = lerp(m[MAT_SAND], red, 0.55f); m[MAT_DUST] = lerp(m[MAT_DUST], red, 0.6f); m[MAT_BASALT] = lerp(m[MAT_BASALT], red, 0.25f); }
    if (g.hasTrait(TR_BLACK_SAND)) { RGB blk(0.13f, 0.12f, 0.13f); m[MAT_SAND] = lerp(m[MAT_SAND], blk, 0.8f); m[MAT_DUST] = lerp(m[MAT_DUST], blk, 0.7f); m[MAT_ROCK] = lerp(m[MAT_ROCK], blk, 0.35f); }
    if (g.hasTrait(TR_CHALK)) { RGB chalk(0.9f, 0.88f, 0.8f); m[MAT_ROCK] = lerp(m[MAT_ROCK], chalk, 0.6f); m[MAT_SAND] = lerp(m[MAT_SAND], chalk, 0.5f); m[MAT_DUST] = lerp(m[MAT_DUST], chalk, 0.6f); }
    if (g.hasTrait(TR_EXOTIC_SEAS)) m[MAT_WATER] = K.exoticColour;   // tar, brine, methane
    // Q-02: the fungal forests' caps pale (cream, ochre, white, a lilac grey: the forest family's colour, so the globe and the maps see it; the
    // second family's is `make`'s), the hot springs' mats in the thermophiles' orange (the rings round the vents, `springOf`)
    if (g.hasTrait(TR_FUNGAL)) {
        static const RGB F[4] = {RGB(0.86f, 0.80f, 0.64f), RGB(0.80f, 0.60f, 0.30f), RGB(0.90f, 0.88f, 0.84f), RGB(0.74f, 0.64f, 0.70f)};
        m[MAT_FOREST] = F[(int)(unitFromHash(mix64(g.seed ^ 0xF0A6ULL)) * 4) % 4];
    }
    if (g.hasTrait(TR_HOT_SPRINGS)) m[MAT_MATS] = RGB(0.82f, 0.46f, 0.12f);
    if (g.hasTrait(TR_GLASSED)) {   // S-03: the sheets of pale green glass take the water family's colour (the globe, the maps and bank 2 read it); the ground round them is scorched
        RGB glass = lerp(RGB(0.50f, 0.66f, 0.52f), base, 0.2f), scorched(0.26f, 0.23f, 0.21f);
        m[MAT_GLASS] = glass; m[MAT_WATER] = glass;
        m[MAT_ROCK] = lerp(m[MAT_ROCK], scorched, 0.45f); m[MAT_DUST] = lerp(m[MAT_DUST], scorched, 0.45f); m[MAT_SAND] = lerp(m[MAT_SAND], scorched, 0.4f);
        m[MAT_BASALT] = lerp(m[MAT_BASALT], scorched, 0.3f); m[MAT_METAL] = lerp(m[MAT_METAL], scorched, 0.3f); m[MAT_GRAPHITE] = lerp(m[MAT_GRAPHITE], scorched, 0.2f);
    }
    if (g.hasTrait(TR_FROZEN_AIR)) {   // D-04: its air's frost on every stone (pale, a little blue), its snow the nitrogen's and the oxygen's white
        const RGB frost(0.80f, 0.84f, 0.90f), snow(0.90f, 0.92f, 0.96f);
        for (int mat : {MAT_ROCK, MAT_BASALT, MAT_DUST, MAT_SAND, MAT_SULPHUR, MAT_SALT, MAT_METAL, MAT_GRAPHITE, MAT_MUD, MAT_CRUST}) m[mat] = lerp(m[mat], frost, 0.35f);
        m[MAT_SNOW] = snow; m[MAT_ICE] = lerp(m[MAT_ICE], snow, 0.3f);
    }
}

// X-03 (GEN 12): a giant's character on its globe, hashed from its seed (the rng stream stays): its share of storms (most
// between a twentieth and a quarter of the storm cells, an ice giant's a few hundredths, a brown dwarf's a tenth or so), an
// ice giant's bands faint under its methane haze, and one giant in four a great storm (an ice giant's dark like a Neptune's,
// a brown dwarf's one in six): an oval of 10,000-24,000 km at 12-40 degrees of latitude, half to three quarters as tall as long
// P-07: a made giant's by its class: a water-cloud giant's storms many and towering (water's convection) under soft bands, an azure giant's
// faint and nearly stormless (no cloud to make them), a hot giant's few broad bands, a silicate giant's patchy clouds of rock, a gas dwarf's
// haze hiding all but a trace of them, a super-puff's faint; only water's convection builds a great storm
static void giantGlobe(const Body& b, BodyGen& g) {
    const int cls = KINDS[b.type].giantClass;
    const bool ice = isIceGiant(b), bd = kindHas(b.type, KT_GLOWS);
    const double u = unitFromHash(mix64(b.seed ^ 0x57A2ULL));
    g.stormShare = ice ? 0.02 + 0.04 * u : (bd ? 0.07 + 0.12 * u : 0.04 + 0.24 * std::pow(u, 1.3));
    g.bandContrast = ice ? 0.45 + 0.2 * unitFromHash(mix64(b.seed ^ 0x57A3ULL)) : 1.0;
    if (cls != GC_NONE) {
        static const double STORMS[GC_COUNT][2] = {{0, 0}, {0.12, 0.2}, {0.01, 0.02}, {0, 0}, {0.06, 0.1}, {0, 0}, {0, 0}};
        static const double BANDS[GC_COUNT][2] = {{1, 0}, {0.45, 0.2}, {0.25, 0.15}, {0.35, 0.15}, {0.5, 0.2}, {0.06, 0.06}, {0.12, 0.1}};
        g.stormShare = STORMS[cls][0] + STORMS[cls][1] * u;
        g.bandContrast = BANDS[cls][0] + BANDS[cls][1] * unitFromHash(mix64(b.seed ^ 0x57A3ULL));
    }
    const uint64_t h = mix64(b.seed ^ 0x6A57ULL);
    g.gsOn = unitFromHash(h) < (bd ? 0.17 : (cls == GC_NONE || cls == GC_WATER ? 0.25 : 0.0));
    if (!g.gsOn) return;
    const double aKm = std::min(5000 + 7000 * unitFromHash(mix64(h ^ 1)), 0.2 * b.radiusKm);
    g.gsA = aKm / b.radiusKm;
    g.gsB = g.gsA * (0.5 + 0.22 * unitFromHash(mix64(h ^ 2)));
    g.gsLat = (12 + 28 * unitFromHash(mix64(h ^ 3))) * DEG * (unitFromHash(mix64(h ^ 4)) < 0.5 ? -1 : 1);
    g.gsLon = (unitFromHash(mix64(h ^ 5)) - 0.5) * TAU;
    g.gsTone = ice || bd ? -1 : (unitFromHash(mix64(h ^ 6)) < 0.6 ? 1 : -1);
}

// P-00: the living worlds' flora colour, drawn after the kind's own draws
static RGB floraColour(Rng& rng) {
    double k = rng.uni();
    if (k < 0.55) return RGB(0.2f + 0.15f * (float)rng.uni(), 0.5f + 0.2f * (float)rng.uni(), 0.12f);
    else if (k < 0.7) return RGB(0.55f, 0.15f, 0.2f);          // red-purple flora
    else if (k < 0.82) return RGB(0.3f, 0.25f, 0.65f);         // blue-violet
    else if (k < 0.92) return RGB(0.7f, 0.6f, 0.15f);          // yellow-ochre
    else return RGB(0.15f, 0.55f, 0.6f);                       // teal
}

// Q-02: a new slot's trait that needs a fact of the world besides a process of its kind: the great tides a forcing (`Body::tideForcingM`), the
// great aurorae a strong field (R-402's class) and an air, the barrier reefs a warm sea (22 C at the equator's shore). Q-03: the singing dunes
// and the singing ice an air to carry them
// D-04: what a body in the dark lacks for the traits that need it: a sun (to sublime its ice into blades or its rock's volatiles, to shine through
// ice crystals or on a cold upper air's clouds, to warm and cool ice into song and valleys into fog) and the life a sun feeds (reefs, fungi,
// blooms, the salt lakes' and the glowing seas' microbes); where its air froze, what the air and a liquid open to it make (storms, haze,
// airglow, aurorae, singing sand, springs, mud, geysers); where its seas froze, the tides
constexpr uint64_t DARK_LACKS = KS_SUBLIMATION | KS_VOLATILES | KS_ICE_CRYSTALS | KS_MESOSPHERE | KS_ICE_SONG | KS_VALLEY_FOG | KS_CORALS | KS_FUNGI | KS_BLOOMS |
                                KS_HALOPHILES | KS_GLOWING_SEA;
constexpr uint64_t FROZEN_AIR_LACKS = KS_STORMS | KS_HAZE | KS_OXYGEN_AIR | KS_AURORAE | KS_SINGING_SAND | KS_SPRINGS | KS_MUD | KS_HYDROTHERMAL;
static bool frozenSeasOf(const Body& b) { return b.drifter && KINDS[b.type].seaFreezeK > 0 && b.tempK < KINDS[b.type].seaFreezeK; }
static uint64_t shapesOf(const Body& b) {
    uint64_t s = KINDS[b.type].shapes;
    if (b.drifter) s &= ~DARK_LACKS;
    if (airFrozen(b)) s &= ~FROZEN_AIR_LACKS;
    if (frozenSeasOf(b)) s &= ~KS_TIDES;
    return s;
}

int darkTraitsOf(const Body& b, int* out) {
    int n = 0;
    const int t = b.type;
    if (!b.drifter || !KINDS[t].landable) return 0;
    if (airFrozen(b)) out[n++] = TR_FROZEN_AIR;
    const bool rock = t == PT_ROCKY || t == PT_CRATERED || t == PT_METAL || t == PT_CARBON || t == PT_DESERT || t == PT_FROZEN_FELISIAN;   // a rock drifter (not ice)
    if (kindHas(t, KT_LAVA) || (b.parent < 0 && rock && unitFromHash(mix64(b.seed ^ 0xE3BE25ULL)) < EMBERS_SHARE)) out[n++] = TR_EMBERS;
    if (t == PT_DARK_OCEAN && unitFromHash(mix64(b.seed ^ 0x119A7E25ULL)) < LIGHTS_IN_WATER_SHARE) out[n++] = TR_LIGHTS_IN_WATER;
    if (drifterHadPeople(b) && peopleFirstAgeOf(b.seed) >= AGE_FUTURISTIC) out[n++] = TR_LIGHTS_LEFT_ON;   // D-05: a late people's works kept themselves lit
    return n;
}

static bool traitFits(const Body& b, const BodyGen& g, int t) {
    switch (t) {
        case TR_GREAT_TIDES: return b.tideForcingM >= TIDE_FORCING_MIN;
        case TR_GREAT_AURORAE: return magneticClass(magneticField(b)) == 3 && KINDS[b.type].pressureLo + KINDS[b.type].pressureSpan > 0.001;
        case TR_REEFS: return climateTempC(g, 0, 0, 0, 0) - (g.hasTrait(TR_SNOWBALL) ? 6.0 : 0.0) >= 22;
        case TR_SINGING_DUNES: case TR_SINGING_ICE: return KINDS[b.type].pressureLo + KINDS[b.type].pressureSpan > 0.001;
        default: return true;
    }
}

BodyGen BodyGen::make(const Body& b, double season) {
    BodyGen g;
    const KindDef& K = KINDS[b.type];
    g.type = b.type;
    g.seed = b.seed; g.worldSeed = b.seed;   // R-413
    g.R = b.radiusKm;
    g.season = K.cover != DC_NONE ? 0.0 : season;   // P-04: a dead world's ground keeps no season (its climate is its then-world's)
    g.starSx = b.starSx; g.starSy = b.starSy; g.starSz = b.starSz; g.starKnown = b.starKnown; g.bodyIndex = b.index;   // T-02
    // B-321: a living world's air keeps it 33 degrees over its equilibrium temperature (Earth's greenhouse), so an Earth-like
    // orbit gives an Earth-like climate; without it two thirds of the felisian land was ice sheet (`vesperis_test encounters`).
    // P-04: a dead world's the climate it had in its people's time (`Body::thenTempK`), its present's mean apart
    g.tempBias = ((b.thenTempK > 0 ? b.thenTempK : b.tempK) + K.greenhouseK - 288.0) * 0.3;
    if (K.cover != DC_NONE) g.nowTempC = (b.tempK - 273.15) * 0.75 + K.tempOffset;
    Rng rng(b.seed ^ 0xB0D7ULL);
    g.sA = rng.next(); g.sB = rng.next(); g.sC = rng.next(); g.sD = rng.next(); g.sE = rng.next(); g.sF = rng.next();
    g.sG = rng.next();
    g.color = b.color;
    g.roughness = 0.3 + 0.7 * rng.uni();
    g.craterDensity = 0.25 + 0.35 * rng.uni();
    g.craterDepth = 0.7 + 0.6 * rng.uni();
    g.cloudScale = 2.5 + 3.0 * rng.uni();
    g.plateK = 0.65 + 0.4 * rng.uni();   // 5-12 plates
    g.chainAmp = 1800 + 1800 * rng.uni();
    g.windAngle = rng.range(0, TAU);
    g.duneWl = 0.12 + 0.25 * rng.uni();
    g.duneStyle = rng.irange(3);
    g.tectonic = (K.tags & KT_PLATES) != 0;
    g.riverDensity = 0.4 + 0.5 * rng.uni();
    g.lakeDensity = 0.35 + 0.45 * unitFromHash(mix64(b.seed ^ 0x1A4EULL));   // B-320: hashed (the rng stream stays)
    g.floraFamily = (int)(unitFromHash(mix64(b.seed ^ 0xF10A)) * 10);    // hashed, not drawn: the rng stream stays as in GEN_VERSION 2; B-315: ten silhouettes
    g.floraFamily2 = (int)(unitFromHash(mix64(b.seed ^ 0xF10B)) * 10);
    if (g.floraFamily2 == g.floraFamily) g.floraFamily2 = (g.floraFamily + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10D)) * 9)) % 10;
    if (K.floraFamily >= 0) {   // P-02: the kind's own first silhouette (the tundra's conifers), the second kept apart from it
        g.floraFamily = K.floraFamily;
        if (g.floraFamily2 == g.floraFamily) g.floraFamily2 = (g.floraFamily + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10DULL)) * 9)) % 10;
    }
    g.latGradient = K.latGradient; g.seaDepth = K.seaDepth; g.grassBand = K.grassBand;   // P-02
    g.subContrast = K.subContrast;   // P-03
    g.locked = b.locked; g.lockedDir = b.lockedDir; g.apexDir = b.apexDir;   // P-06
    g.selfGlow = (K.tags & KT_GLOWS) ? clampd(b.luminosity / 0.003, 0.55, 1.0) : 0.0;
    // P-00: the kind's draws in its order (the stream is the type switch's of generation 15, field by field), then its colours: the
    // second tint, the sky's (three draws on a living world), the flora's (a living world's), a giant's globe
    for (int i = 0; i < K.nDraws; i++) {
        const GenDraw& d = K.draws[i];
        double v = d.kind == DK_CONST ? d.lo : (d.kind == DK_SYM ? rng.sym(d.span) : d.lo + d.span * rng.uni());
        switch (d.field) {
            case GF_SEA_LEVEL: g.seaLevel = v; break;
            case GF_MOUNTAIN: g.mountainAmp = v; break;
            case GF_HILL: g.hillAmp = v; break;
            case GF_ICE_CAP: g.iceCapLat = v; break;
            case GF_SNOW_LINE: g.snowLine = v; break;
            case GF_MOISTURE: g.moistureBias = v; break;
            case GF_CLOUD_COVER: g.cloudCover = v; break;
            case GF_LAVA_LEVEL: g.lavaLevel = v; break;
            case GF_VOLCANOES: g.volcanoes = v; break;
            case GF_CRATER_DENSITY: g.craterDensity = v; break;
            case GF_CRATER_DEPTH: g.craterDepth = v; break;
            case GF_BOULDERS: g.boulderDensity = v; break;
            case GF_DUNE_AMP: g.duneAmp = v; break;
            case GF_FRACTURE: g.fractureWidth = v; break;
            case GF_BAND_COUNT: g.bandCount = v; break;
            case GF_LIQUID_LEVEL: g.liquidLevel = v; break;
            case GF_SEA_REACH: g.seaReachDeg = v; break;
            case GF_ISLANDS: g.islandDensity = v; break;
            case GF_CONT_SCALE: g.contScale = v; break;
            default: break;
        }
    }
    if (K.color2.mode == TINT_CONST) g.color2 = K.color2.c;
    else if (K.color2.mode == TINT_BODY_SCALED) g.color2 = RGB(b.color.r * K.color2.c.r, b.color.g * K.color2.c.g, b.color.b * K.color2.c.b);
    switch (K.skyTint.mode) {
        case TINT_CONST: g.skyTint = K.skyTint.c; break;
        case TINT_THIN_AIR: { RGB c = b.color; g.skyTint = RGB(0.6f * c.b + 0.4f * c.r, 0.5f * c.g + 0.3f, 0.7f * c.r + 0.3f * c.b); break; }
        case TINT_DRAWN: g.skyTint = RGB(K.skyTint.c.r + K.skyTint.var.r * (float)rng.uni(), K.skyTint.c.g + K.skyTint.var.g * (float)rng.uni(), K.skyTint.c.b + K.skyTint.var.b * (float)rng.uni()); break;
        default: break;
    }
    if (K.tags & KT_FLORA) g.vegColor = floraColour(rng);
    if (K.tags & KT_GIANT) giantGlobe(b, g);
    {   // R-304: the traits, hashed (the rng stream stays as in generation 7): none for a fifth of the bodies, one for two
        // fifths, two for a bit over a quarter, three for the rest, drawn from the kind's list without repeats; one colour
        // trait at most, pangaea and archipelago never together (P-00: the traits' rows say which are landforms and which exclude which)
        auto tu = [&](int k) { return unitFromHash(mix64(b.seed ^ (0x7A17ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
        // D-04: a body in the dark by the processes it has there (`shapesOf`: no sun, perhaps no air); every other by its kind's
        const uint64_t shapes = shapesOf(b);
        auto eligible = [&](int t) { return TRAITS[t].cls != TC_GIVEN && (shapes & TRAITS[t].needs) != 0; };
        int eligL[TR_COUNT], eligW[TR_COUNT]; int nl = 0, nw = 0;   // the landform traits and the world traits, each in id order
        for (int t = 1; t < TR_COUNT; t++) if (eligible(t) && TRAITS[t].kind == TK_FIRST) { if (TRAITS[t].cls == TC_LANDFORM) eligL[nl++] = t; else eligW[nw++] = t; }   // Q-01: R-304's alone
        double u0 = tu(0);
        int n = u0 < 0.2 ? 0 : (u0 < 0.6 ? 1 : (u0 < 0.88 ? 2 : 3));
        int got = 0;
        for (int k = 0; k < 12 && got < n && nl + nw > 0; k++) {
            bool landform = nw == 0 || (nl > 0 && tu(30 + k) < 0.6);
            int t = landform ? eligL[(int)(tu(1 + k) * nl) % nl] : eligW[(int)(tu(1 + k) * nw) % nw];
            bool ok = true;
            for (int j = 0; j < got; j++) {
                int o = g.traits[j];
                if (o == t) ok = false;
                if (TRAITS[t].colour && TRAITS[o].colour) ok = false;
                if (TRAITS[t].excludes == o) ok = false;   // pangaea and archipelago, giant flora and dead forests
            }
            if (ok) g.traits[got++] = t;
        }
        if (K.favourTrait != TR_NONE && got < 3 && eligible(K.favourTrait) && unitFromHash(mix64(b.seed ^ 0xFA70ULL)) < K.favourChance) {   // P-02: a trait
            // the kind takes more often (the hothouse's giant flora), in a free slot where the drawn ones allow it
            const int t = K.favourTrait; bool ok = true;
            for (int j = 0; j < got; j++) { const int o = g.traits[j]; if (o == t || TRAITS[t].excludes == o || (TRAITS[t].colour && TRAITS[o].colour)) ok = false; }
            if (ok) g.traits[got++] = t;
        }
        if (b.glassed) {   // S-03: a survivor of its neutron star's supernova: the glass comes first and the exotic seas boiled off (the glass owns bank 2)
            int keep[3] = {0, 0, 0}, nk = 0;
            for (int k = 0; k < 3 && nk < 2; k++) if (g.traits[k] && g.traits[k] != TR_EXOTIC_SEAS) keep[nk++] = g.traits[k];
            g.traits[0] = TR_GLASSED; g.traits[1] = keep[0]; g.traits[2] = keep[1];
        }
        if (!b.wanderer && (b.drifter ? drifterHadPeople(b) : peoplesHash(b.type, b.seed))) {   // C-01: a people lived here (one world in eight of the kinds
            // a people may live on, by a hash of its own, so the other worlds keep their traits; P-04: a dead world's by its own; P-05: never on a
            // wanderer; D-05: a drifter's by its own, one frozen felisian or desert drifter in four): the trait goes first (after the glass), the third
            // trait drops
            int keep[3] = {0, 0, 0}, nk = 0;
            for (int k = 0; k < 3 && nk < 2; k++) if (g.traits[k] && g.traits[k] != TR_CIVILISATION) keep[nk++] = g.traits[k];
            if (keep[0] == TR_GLASSED) { g.traits[0] = TR_GLASSED; g.traits[1] = TR_CIVILISATION; g.traits[2] = keep[1]; }
            else { g.traits[0] = TR_CIVILISATION; g.traits[1] = keep[0]; g.traits[2] = keep[1]; }
            if (K.tags & KT_DEAD_SEAS) {   // the post-apocalyptic desert: its seas stood at a level of their own and its rivers ran to them
                g.oldSeaM = -120.0 + 220.0 * tu(22);
                g.riverDensity = std::max(g.riverDensity, 0.8);
            }
        }
        {   // Q-01: the three new slots, drawn by hashes of their own after the first three (which stay as R-304 drew them): how many as R-304's
            // (none for a fifth of the worlds, one for two fifths, two for a bit over a quarter, three for the rest), each from a kind of trait of its
            // own that fits the world (the kinds in an order hashed for it, the first that hold a trait the slots before allow: one colour trait at
            // most, no trait with one it excludes), the trait from its kind by a hash; a world with a people takes none that moves its ground (its
            // towns stand where 1.3.2 put them), nor the home world (where every expedition starts: its ground as the explorer first knew it)
            auto nu = [&](int k) { return unitFromHash(mix64(b.seed ^ (0x51075ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
            const double u = nu(0);
            // D-04: a body in the dark takes its own traits first (`darkTraitsOf`), the draw the slots they leave
            int own[3]; const int nOwn = darkTraitsOf(b, own);
            for (int k = 0; k < nOwn; k++) g.traits[3 + k] = own[k];
            const int want = std::min(u < 0.2 ? 0 : (u < 0.6 ? 1 : (u < 0.88 ? 2 : 3)), 3 - nOwn);
            const bool home = b.starKnown && b.starSx == HOME_SX && b.starSy == HOME_SY && b.starSz == HOME_SZ && b.parent < 0 && b.type == PT_FELISIAN;   // the game's start (`Game::newGame`)
            const bool kept = home || (!b.wanderer && (b.drifter ? drifterHadPeople(b) : peoplesHash(b.type, b.seed)));   // its ground stays (D-05: a drifter's people's too)
            int order[TK_COUNT]; double key[TK_COUNT]; int nk = 0;
            for (int k = TK_LANDFORM; k < TK_DARK; k++) {   // the kinds in the world's order (an insertion by their hashes; D-04: the dark's own are given)
                const double v = nu(1 + k); int i = nk++;
                while (i > 0 && key[i - 1] > v) { order[i] = order[i - 1]; key[i] = key[i - 1]; i--; }
                order[i] = k; key[i] = v;
            }
            for (int i = 0, got = 0; i < nk && got < want; i++) {
                int elig[TR_COUNT], n = 0;
                for (int t = TR_CIVILISATION + 1; t < TR_COUNT; t++) {
                    if (TRAITS[t].kind != order[i] || (kept && TRAITS[t].moves) || !eligible(t) || !traitFits(b, g, t)) continue;
                    bool ok = true;
                    for (int j = 0; j < TRAIT_SLOTS && ok; j++) { const int o = g.traits[j]; if (o && (o == t || TRAITS[t].excludes == o || TRAITS[o].excludes == t || (TRAITS[t].colour && TRAITS[o].colour))) ok = false; }
                    if (ok) elig[n++] = t;
                }
                if (n > 0) g.traits[3 + nOwn + got++] = elig[(int)(nu(10 + order[i]) * n) % n];
            }
        }
        for (int k = 0; k < TRAIT_SLOTS; k++) switch (g.traits[k]) {
            case TR_ARCHIPELAGO: g.seaLevel += 0.3; g.islandDensity = std::max(g.islandDensity, 0.5); break;   // P-02: a kind's own islands may be more
            case TR_PANGAEA: g.contScale = 1.8; g.seaLevel -= 0.05; break;
            case TR_LAKELAND: g.lakeDensity = 0.85; g.riverDensity = 0.9; break;
            case TR_SNOWBALL: g.iceCapLat = 28 + 12 * tu(20); g.snowLine *= 0.55; g.tempBias -= 6; break;
            case TR_EXOTIC_SEAS: g.liquidLevel = K.exoticLo + K.exoticSpan * tu(21); break;   // tar, brine, methane
            case TR_STORMS: g.cloudCover = std::min(1.0, g.cloudCover + 0.35); break;
            case TR_DEAD_FOREST: g.vegColor = lerp(g.vegColor, RGB(0.45f, 0.42f, 0.35f), 0.65f); break;
            case TR_GREAT_TIDES: g.tideRangeM = tideRangeOf(b.tideForcingM); break;   // Q-02
            case TR_PINK_LAKES: { static const RGB P[3] = {RGB(0.92f, 0.52f, 0.70f), RGB(0.86f, 0.38f, 0.48f), RGB(0.80f, 0.30f, 0.34f)}; g.waterTintColour = P[(int)(tu(23) * 3) % 3]; break; }   // Hillier's, Retba's, Natron's
            case TR_MILKY_SEAS: g.waterTintColour = RGB(0.66f, 0.80f, 0.90f); break;
            case TR_BLOOMS: { static const RGB B[3] = {RGB(0.58f, 0.20f, 0.13f), RGB(0.26f, 0.52f, 0.20f), RGB(0.62f, 0.38f, 0.14f)}; g.lifeTintColour = B[(int)(tu(24) * 3) % 3]; break; }   // a red tide, a green, a brown
            case TR_REEFS: g.lifeTintColour = RGB(0.22f, 0.70f, 0.72f); break;   // the lagoons' turquoise
            case TR_LIGHTS_IN_WATER: { static const RGB L[3] = {RGB(0.30f, 0.86f, 0.80f), RGB(0.36f, 0.62f, 0.95f), RGB(0.52f, 0.92f, 0.46f)}; g.waterTintColour = L[(int)(tu(25) * 3) % 3]; break; }   // D-04: a blue-green, a blue, a green
            default: break;
        }
    }
    if (b.drifter) {   // D-04: in the dark; its liquid frozen at its level where it is colder than its kind's (a dark ocean's never: its heat is its sea's)
        g.dark = true; g.drifter = true;   // D-05
        g.frozenSeas = frozenSeasOf(b) && (kindHas(b.type, KT_SEA) || kindHas(b.type, KT_WET) || g.liquidLevel > -1e8);
    }
    {   // N2-05: the second family's colour, a related hue from the family palettes (hashed, the rng stream untouched)
        static const RGB pal[5] = {RGB(0.28f, 0.6f, 0.12f), RGB(0.55f, 0.15f, 0.2f), RGB(0.3f, 0.25f, 0.65f), RGB(0.7f, 0.6f, 0.15f), RGB(0.15f, 0.55f, 0.6f)};
        RGB base = (g.vegColor.r + g.vegColor.g + g.vegColor.b > 0.05f) ? g.vegColor : RGB(0.25f, 0.5f, 0.15f);
        int nearest = 0; float bestD = 1e9f;
        for (int k = 0; k < 5; k++) { float d = (pal[k].r - base.r) * (pal[k].r - base.r) + (pal[k].g - base.g) * (pal[k].g - base.g) + (pal[k].b - base.b) * (pal[k].b - base.b); if (d < bestD) { bestD = d; nearest = k; } }
        RGB other = pal[(nearest + 1 + (int)(unitFromHash(mix64(b.seed ^ 0xF10CULL)) * 4)) % 5];   // a different family palette
        g.vegColor2 = lerp(base, other, 0.6f);
        if (g.hasTrait(TR_FUNGAL)) {   // Q-02: the second family's caps another pale (by its own hash)
            static const RGB F2[4] = {RGB(0.92f, 0.84f, 0.70f), RGB(0.70f, 0.50f, 0.32f), RGB(0.84f, 0.82f, 0.86f), RGB(0.86f, 0.62f, 0.56f)};
            g.vegColor2 = F2[(int)(unitFromHash(mix64(b.seed ^ 0xF0A7ULL)) * 4) % 4];
        }
    }
    {   // O6-02: the relief spectrum's parameters, hashed (the rng stream stays as in generation 5); the amplitudes scale
        // with the base wavelength, which a small body caps, so a comet's relief is a small world's
        auto hu = [&](int k) { return unitFromHash(mix64(b.seed ^ (0x0616ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
        g.reliefWl0 = std::min(12.0 + 8.0 * hu(1), b.radiusKm * 0.25);
        g.reliefH = 1.04 + 0.1 * hu(2);
        g.reliefRidge = 0.35 + 0.4 * hu(3);
        g.reliefWarp = 0.2 + 0.25 * hu(4);
        g.reliefErosion = 2.5 + 3.0 * hu(5);
        g.reliefHybrid = 0.4 + 0.4 * hu(6);
        // P-00: the kind's grades (KindRelief): its ridge changed, its low ground's smoothing, metres per 16 km of base wavelength on the
        // plains, the hills and the mountains (before the ridge normalisation), its cliff bands scaled (a shell of ice and a soft world
        // have fewer and lower ones; a nucleus none)
        const KindRelief& KR = K.relief;
        double k = 1000.0 * g.reliefWl0 / 16.0, flat = KR.flat, hill = KR.hill, mtn = KR.mtn;
        switch (KR.ridgeOp) {
            case RO_ADD: g.reliefRidge += KR.ridge; break;
            case RO_MUL: g.reliefRidge *= KR.ridge; break;
            case RO_SET: g.reliefRidge = KR.ridge; break;
            default: break;
        }
        if (KR.hybrid >= 0) g.reliefHybrid = KR.hybrid;
        g.reliefRidge = clampd(g.reliefRidge, 0, 1);
        g.reliefFlat = flat * k; g.reliefHill = hill * k; g.reliefMtn = mtn * k;
        // O6-04: the cliff bands
        g.cliffStep = 60 + 70 * hu(7);
        g.cliffAmount = 0.45 + 0.4 * hu(8);
        if (KR.cliffAmount != 1) g.cliffAmount *= KR.cliffAmount;
        if (KR.cliffStep != 1) g.cliffStep *= KR.cliffStep;
    }
    materialPalette(g);   // N1-01
    return g;
}

double iceSheetEdgeDeg(const BodyGen& g, const Vec3& unit) { return g.iceCapLat + 6.0 * gnoise3(unit * g.R / (g.R * 0.15), g.sF + 7); }   // `felisianSample`'s cap edge without its season

double iceSheetIn(const BodyGen& g, const Vec3& unit) {
    const double absLatDeg = std::fabs(std::asin(clampd(unit.z, -1, 1))) / DEG;
    if (KINDS[g.type].sheetByCold && g.locked) return (-4.0 - climateTempC(g, absLatDeg, 0, 0, dot(unit, g.lockedDir)) + 7.0 * gnoise3(unit * g.R / (g.R * 0.15), g.sF + 7)) / 25.0;   // its edge wandering as the tundra's
    return (absLatDeg - iceSheetEdgeDeg(g, unit)) / 14.0;
}

double climateTempC(const BodyGen& g, double absLatDeg, double heightM, double winter, double sub) {
    // B-321: Earth's mean temperature against latitude runs 27 C at the equator, 15 at 35 degrees, 5 at 50, -2 at 60,
    // -9 at 70: `-18 + 46 cos(lat)^1.5`; the old line `28 - 0.55 lat` was five degrees too cold in the middle latitudes
    double c = std::cos(absLatDeg * DEG);
    double L = -18.0 + 46.0 * c * std::sqrt(c);
    if (g.latGradient != 1) L = 15.0 + (L - 15.0) * g.latGradient;   // P-02: a kind's contrast round Earth's mean (a hothouse's mild poles)
    double T = L - 6.5 * std::max(0.0, heightM) / 1000.0 + g.tempBias - 6.0 * winter;
    if (g.locked) T += g.subContrast * sub - 6.0 + 0.35 * absLatDeg;   // M5-08: a permanent day side and night side; the twilight belt between keeps the mild climate (P-03: the kind's contrast, 45 degrees but a terminator world's)
    return T;
}
