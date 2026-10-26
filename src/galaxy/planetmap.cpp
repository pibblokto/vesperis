#include "planetmap.h"
#include "planet_internal.h"

const char* const MATERIAL_NAMES[] = {"ROCK", "SAND", "GRASS", "FOREST", "SNOW", "WATER", "LAVA", "ICE", "CLOUD",
                                      "QUARTZ", "BASALT", "DUST", "GAS", "IRON", "SULPHUR", "GRAPHITE", "SALT", "GLASS", "MUD", "POTASH", "BRINE",
                                      "MATS", "STROMATOLITE", "CRUST", "MOSS"};
static_assert(sizeof(MATERIAL_NAMES) / sizeof(MATERIAL_NAMES[0]) == MAT_COUNT, "a name for every material");
// R-307: one sky pair per kind with air (P-00: the row's: from its tint, a thin air's, or fixed), shared by the ground, the globe and the map
bool typeSky(const BodyGen& g, RGB& zen, RGB& hor) {
    const KindDef& K = KINDS[g.type];
    if (!K.atmosphere || (K.tags & KT_GIANT)) return false;
    RGB white(1, 1, 1);
    switch (K.sky) {
        case SKY_THIN: zen = g.skyTint * 0.35f; hor = g.skyTint * 0.75f; break;
        case SKY_FIXED: zen = K.zenith; hor = K.horizon; break;
        default: zen = g.skyTint * 0.75f; hor = lerp(g.skyTint, white, 0.55f); break;
    }
    return true;
}
std::string traitList(const BodyGen& g) {
    std::string s;
    for (int k = 0; k < TRAIT_SLOTS; k++) if (g.traits[k]) { if (!s.empty()) s += ", "; s += TRAITS[g.traits[k]].name; }
    return s;
}
const char* const BIOME_NAMES[BIO_COUNT] = {"", "TROPICAL FOREST", "SAVANNA", "DESERT", "TEMPERATE FOREST", "GRASSLAND", "TAIGA", "TUNDRA", "WETLAND", "ALPINE", "ICE SHEET", "OCEAN"};

const char* reliefClass(double slope) { return slope < 0.05 ? "FLAT" : (slope < 0.2 ? "HILLS" : "MOUNTAINS"); }   // O6-02: 0.04 / 0.15 before the spectrum

double sampleCloudPattern(const BodyGen& g, double lon, double lat) {
    if (g.cloudCover <= 0) return 0;
    // sample on the sphere so the pattern wraps seamlessly
    Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
    double n = fbm3(u * g.cloudScale, g.sG, 4, 2.1, 0.55);
    double bands = 0.15 * std::sin(lat * 6.0 + n * 3.0);
    double v = n * 0.5 + 0.5 + bands;
    if (kindHas(g.type, KT_CLOUD_SWIRL)) {
        double swirl = fbm3(u * (g.cloudScale * 2.0) + Vec3(0, 0, lat * 3.0), g.sG + 5, 3);
        return clampd(0.55 + 0.45 * (0.5 * n + 0.5 * swirl) + bands, 0.15, 1.0);
    }
    double thr = 1.0 - g.cloudCover;
    return clampd((v - thr) / std::max(0.05, 1.0 - thr) * 1.6, 0.0, 1.0);
}

// P-00: the kind's ground function, then the solid grounds' common end (the felisian layer keeps its own)
// the water's trait colours at a sample (B-404's rule: unfaded fields, so every ring, the map and the globe agree). A pink lake's whole water
// and a white crust of salt on its shore; a milky sea's patches of glowing bacteria, tens of thousands of square kilometres each (Miller et
// al. 2005); a bloom's streaks drawn out by the currents (a warped field's ridges) where the sea is rich, more on the shallows; on the seas
// and, but where they are pink, the lakes
static void qualityWater(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s) {
    const bool pink = g.hasTrait(TR_PINK_LAKES), milky = g.hasTrait(TR_MILKY_SEAS), blooms = g.hasTrait(TR_BLOOMS), lights = g.hasTrait(TR_LIGHTS_IN_WATER);
    if (!pink && !milky && !blooms && !lights) return;
    const bool wet = s.water > -1e8 && (s.material == MAT_WATER || s.height < s.water);
    const bool lake = s.waterKind == 2 || s.waterKind == 3 || s.waterKind == 4, sea = s.waterKind == 0;
    const Vec3 p = unit * g.R;
    // the crust on its shore
    if (pink && !wet && lake && s.height < s.water + 0.5 + 0.4 * fbm3(p / 0.05, g.sG + 301, 2) && s.material != MAT_SNOW && s.material != MAT_ICE) { s.material = MAT_SALT; s.albedo = 0.78; }
    if (!wet) return;
    if (pink && lake) s.waterTint = 1;
    if (milky && sea) s.waterTint = smoothstep(0.22, 0.42, fbm3(p / (g.R * 0.16), g.sG + 303, 2));   // a few patches of a few hundred kilometres
    if (lights && sea) {   // a dark ocean's life, round the vents its volcanoes' flanks hold (the islands' shallows, under 500 m) and in shoals some hundreds of
        // kilometres across over the deep (what the globe shows), lit in patches of a hundred metres or two
        const double shoal = smoothstep(-0.05, 0.25, fbm3(p / (g.R * 0.05), g.sG + 313, 2)), region = std::max(shoal, smoothstep(-500.0, -60.0, s.height));
        // a quarter of its water lit; from afar the shoals' cores' faint sheen
        if (region > 0.01) s.waterTint = matVisible(0.1, detailM) ? region * smoothstep(0.16, 0.42, fbm3(p / 0.3, g.sG + 314, 3)) : 0.2 * smoothstep(0.4, 0.9, shoal);
    }
    if (blooms && (sea || (lake && !pink))) {
        // swirls a few hundred kilometres across (the warped field's bands, wide enough for the globe to show), their edges frayed into the
        // currents' filaments where the eye is near enough to see them
        // two scales: the bloom's regions, swirls hundreds of kilometres across (a warped field, what the globe shows: their streaks blur into
        // a tint there), and inside them the streaks drawn out by the currents, filaments some 12 km apart where the eye resolves them
        const Vec3 q = p / (g.R * 0.05);
        const Vec3 w(fbm3(q * 0.5, g.sG + 305, 2), fbm3(q * 0.5 + Vec3(5.2, 1.3, 7.7), g.sG + 306, 2), fbm3(q * 0.5 + Vec3(2.8, 9.1, 3.4), g.sG + 307, 2));
        const double region = smoothstep(0.0, 0.25, fbm3(q + w * 2.0, g.sG + 308, 3)) * (sea ? 0.65 + 0.35 * smoothstep(-400.0, -60.0, s.height) : 1.0);
        if (region > 0.01) {
            constexpr double STREAK_KM = 12.0;
            if (matVisible(STREAK_KM * 0.3, detailM)) {
                const double n2 = fbm3(p / STREAK_KM + w * 6.0, g.sG + 310, 3) + (matVisible(0.6, detailM) ? 0.25 * fbm3(p / 1.4 + w * 9.0, g.sG + 311, 2) : 0.0);
                s.lifeTint = region * (0.2 + 0.95 * smoothstep(0.7, 0.9, 1.0 - std::fabs(n2)));   // the filaments along the field's zero lines
            } else s.lifeTint = 0.8 * region;
        }
    }
}

// Q-02 (TR_GREAT_TIDES): the tidal flats. The local spring range (the trait's at the equator, the bulges' cos^2 of the latitude) decides a band
// round the sea's level: the ground within six ranges of it is pressed toward the level (h' = S + H x (0.2 + 0.8 x^4), x the height over
// the level in units of H: the intertidal zone takes the ground of two ranges either side, a flat kilometres wide on a gentle coast, the
// coast steeper above it). Between low and high water the flat is wet sand under the sea's plane (its `water` the mean level, so the surface's
// tide moves the sheet over it), cut by creeks; the sea's own samples under low water stay sea. Rivers and lakes keep their own water
double tideRangeAt(const BodyGen& g, const Vec3& unit) { return g.tideRangeM * (1 - unit.z * unit.z); }
static void tidalFlat(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s) {
    const double Rl = tideRangeAt(g, unit);
    if (Rl < 0.5) return;
    const double S = g.liquidLevel > -1e8 ? g.liquidLevel : 0.0, H = 6.0 * Rl;
    const bool inland = s.waterKind != 0 && s.water > S + 0.01;
    if (inland || s.material == MAT_LAVA || s.material == MAT_ICE || (s.water > -1e8 && std::fabs(s.water - S) > 0.01 && s.water < S)) return;
    const double x = (s.height - S) / H;
    if (std::fabs(x) >= 1) return;
    const double x4 = x * x * x * x;
    double h = S + H * x * (0.2 + 0.8 * x4);
    if (std::fabs(h - S) < 0.52 * Rl) {
        const Vec3 p = unit * g.R;
        if (matVisible(0.01, detailM)) {   // the creeks: a dendrite of channels a metre deep draining the flat toward the sea
            const double n = 1.0 - std::fabs(fbm3(p / 0.35 + Vec3(fbm3(p / 0.9, g.sG + 331, 2), 0, 0), g.sG + 330, 3));
            h -= 1.2 * std::pow(clampd(n, 0, 1), 8.0) * smoothstep(0.52 * Rl, 0.2 * Rl, std::fabs(h - S)) * lodFade(0.02, detailM);
        }
        s.material = MAT_SAND; s.albedo = 0.3 + 0.06 * fbm3(p / 0.2, g.sG + 332, 2); s.water = S; s.waterKind = 0; s.shore = 1e9; s.veg = 0;
        if (s.biome != BIO_OCEAN && h < S) s.biome = BIO_OCEAN;   // the flat under the mean level is the sea's (the rivers end at it)
    } else if (h < S && s.material != MAT_WATER) { s.material = MAT_WATER; s.water = S; s.biome = BIO_OCEAN; s.albedo = 0.36; }
    s.height = h;
}

SurfaceSample sampleSurface(const BodyGen& g, const Vec3& unit, double detailM) {
    SurfaceSample s;
    GroundCtx c(g, unit, detailM, s);
    const int ground = c.K.ground;
    if (g.tectonic && ground != GR_FELISIAN) c.tec = tectonicsAt(unit, g);
    if (ground != GR_GIANT && ground != GR_OCEAN && ground != GR_EYEBALL && ground != GR_NONE && ground != GR_FELISIAN) {
        c.massif = smoothstep(0.2, 0.6, c.rg(std::min(110.0, c.R * 0.3), g.sC + 6, 3));
        c.hillsF = smoothstep(-0.15, 0.45, c.fb(std::min(c.R * 0.06, 400.0), g.sB + 6, 3));
    }
    switch (ground) {
    case GR_FELISIAN: felisianSample(g, unit, detailM, s); break;
    case GR_CRATERED: groundCratered(c); break;
    case GR_MOLTEN: groundMolten(c); break;
    case GR_VENUSIAN: groundVenusian(c); break;
    case GR_ROCKY: groundRocky(c); break;
    case GR_THIN_AIR: groundThinAir(c); break;
    case GR_ICE: groundIce(c); break;
    case GR_QUARTZ: groundQuartz(c); break;
    case GR_OCEAN: groundOcean(c); break;
    case GR_EUROPAN: groundEuropan(c); break;
    case GR_TECTONIC: groundTectonic(c); break;
    case GR_DESERT: groundDesert(c); break;
    case GR_HYDROCARBON: groundHydrocarbon(c); break;
    case GR_BOMBARDED: groundBombarded(c); break;
    case GR_ACIDIC: groundAcidic(c); break;
    case GR_METAL: groundMetal(c); break;
    case GR_VOLCANIC: groundVolcanic(c); break;
    case GR_CARBON: groundCarbon(c); break;
    case GR_GIANT: groundGiant(c); break;
    case GR_MAGMA: groundMagma(c); break;
    case GR_CHTHONIAN: groundChthonian(c); break;
    case GR_GLASS: groundGlass(c); break;
    case GR_STEAM: groundSteam(c); break;
    case GR_SALT: groundSalt(c); break;
    case GR_LICHEN: groundLichen(c); break;
    case GR_EYEBALL: groundEyeball(c); break;
    case GR_AMMONIA: groundAmmonia(c); break;
    case GR_CHEMOTROPH: groundChemotroph(c); break;
    case GR_NITROGEN: groundNitrogen(c); break;
    case GR_CRYOVOLCANIC: groundCryovolcanic(c); break;
    case GR_DRY_ICE: groundDryIce(c); break;
    case GR_WANDERER: groundWanderer(c); break;
    case GR_BRINE: groundBrine(c); break;
    case GR_FROST: groundFrost(c); break;
    case GR_GROOVED: groundGrooved(c); break;
    case GR_TWO_FACED: groundTwoFaced(c); break;
    case GR_TIGER: groundTiger(c); break;
    case GR_PATCHWORK: groundPatchwork(c); break;
    case GR_SPONGE: groundSponge(c); break;
    case GR_DARK_OCEAN: groundDarkOcean(c); break;
    default: break;
    }
    if (ground != GR_FELISIAN) {
        const bool wetType = kindHas(g.type, KT_WET);
        const DrainApply& DA = c.DA;
        const LandOut& lo = c.lo;
        const double cliffV = c.cliffV;
        s.scree = clampd(c.talusV, 0, 1);
        if (cliffV > 0.4 && s.material != MAT_WATER && s.material != MAT_LAVA && s.material != MAT_ICE && s.material != MAT_SALT) {   // a riser is bare rock, whatever lies on the plain around it
            if (s.material == MAT_DUST || s.material == MAT_SAND || s.material == MAT_SULPHUR) s.material = familyRep(g.type, FAM_ROCK);
            s.albedo *= 1 - 0.15 * cliffV;
        }
        s.relief = clampd(s.relief + 0.5 * cliffV, 0, 1);
        if (DA.kind && wetType) {   // O6-03: the liquid's plane under the banks, the edge (B-322) for the mesh's shore cut
            if (s.material != MAT_WATER && s.material != MAT_LAVA && DA.zone > 0 && s.water < -1e8) s.water = DA.level;
            if (s.water > -1e8 && DA.zone > 0) { s.waterKind = DA.kind; s.shore = DA.shore; }
        }
        if (wetType && s.water < -1e8 && DA.shoreOut < 1e8) s.shore = DA.shoreOut;
        s.landform = 1000 * lo.mark + std::fabs(lo.dhSmooth) + std::fabs(lo.dhFine) + 40 * lo.flatten;
        s.landformTrait = lo.mark >= 0.5 ? lo.markTrait : 0;
        s.groundTint = lo.tint;
        s.slipFace = std::max(s.slipFace, lo.slip);   // a dune sea's
        if (lo.life > 0 && s.material != MAT_WATER) s.lifeTint = lo.life;
        // a landform's material (rock walls, basalt sheets, salt, crystal) over the kind's own, never over lava
        if (lo.mat >= 0 && s.material != MAT_LAVA && s.material != MAT_WATER) { s.material = lo.mat; if (lo.albedo >= 0) s.albedo = lo.albedo; }
        // TR_EXOTIC_SEAS: the ground under the liquid's level is sea
        if (g.liquidLevel > -1e8 && s.height < g.liquidLevel && s.material != MAT_LAVA) { s.material = MAT_WATER; s.water = g.liquidLevel; s.albedo = 0.22; s.glow = 0; }
    }
    if (g.tideRangeM > 0) tidalFlat(g, unit, detailM, s);
    qualityWater(g, unit, detailM, s);
    if (g.dark) darkGround(c);
    if (s.material != MAT_LAVA) s.albedo = clampd(s.albedo, 0.03, 1.0);
    return s;
}

void PlanetMap::generate(const BodyGen& g) {
    valid = false;
    height.assign(W * H, 0.f);
    material.assign(W * H, 0);
    albedo.assign(W * H, 0);
    cloud.assign(W * H, 0);
    veg.assign(W * H, 0);
    const bool tints = g.hasTrait(TR_PINK_LAKES) || g.hasTrait(TR_MILKY_SEAS) || g.hasTrait(TR_BLOOMS) || g.hasTrait(TR_REEFS) || g.hasTrait(TR_LIGHTS_IN_WATER);   // Q-02; D-04
    wtint.assign(tints ? W * H : 0, 0); ltint.assign(tints ? W * H : 0, 0);
    double detail = TAU * g.R * 1000.0 / W * 0.6;
    for (int y = 0; y < H; y++) {
        double lat = (0.5 - (y + 0.5) / H) * PI;
        for (int x = 0; x < W; x++) {
            double lon = ((x + 0.5) / W) * TAU - PI;
            Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
            SurfaceSample s = sampleSurface(g, u, detail);
            int i = y * W + x;
            height[i] = (float)s.height;
            material[i] = (uint8_t)s.material;
            albedo[i] = (uint8_t)clampi((int)(s.albedo * 255 + 0.5), 0, 255);
            veg[i] = (uint8_t)clampi((int)(s.veg * 255 + 0.5), 0, 255);
            if (tints) { wtint[i] = (uint8_t)clampi((int)(s.waterTint * 255 + 0.5), 0, 255); ltint[i] = (uint8_t)clampi((int)(s.lifeTint * 255 + 0.5), 0, 255); }
            cloud[i] = (uint8_t)clampi((int)(sampleCloudPattern(g, lon, lat) * 255 + 0.5), 0, 255);
            // a polar vortex pattern on gas giants
            if (kindHas(g.type, KT_GAS_GIANT) && std::fabs(lat) > 76 * DEG)
                albedo[i] = (uint8_t)clampi((int)(albedo[i] * (1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat)))), 0, 255);
        }
    }
    seed = g.seed;
    valid = true;
}
