#include "planetmap.h"
#include "planet_internal.h"

const char* const MATERIAL_NAMES[] = {"ROCK", "SAND", "GRASS", "FOREST", "SNOW", "WATER", "LAVA", "ICE", "CLOUD",
                                      "QUARTZ", "BASALT", "DUST", "GAS", "IRON", "SULPHUR", "GRAPHITE", "SALT", "GLASS", "MUD", "POTASH", "BRINE",
                                      "MATS", "STROMATOLITE", "CRUST", "MOSS", "TISSUE", "THREADS", "HARD GROWTH"};
static_assert(sizeof(MATERIAL_NAMES) / sizeof(MATERIAL_NAMES[0]) == MAT_COUNT, "a name for every material");
// R-307: one sky pair per kind with air (P-00: the row's: from its tint, a thin air's, or fixed), shared by the ground, the globe and the map.
// J-04: a seeded world's made air over it: an ozone's deeper blue overhead or a thin one's violet, a thick air pale to the horizon, a haze's
// colour, a sulphur air's yellow at the horizon
bool typeSky(const BodyGen& g, RGB& zen, RGB& hor) {
    const KindDef& K = KINDS[g.type];
    if (!K.atmosphere || (K.tags & KT_GIANT)) return false;
    RGB white(1, 1, 1);
    switch (K.sky) {
        case SKY_THIN: zen = g.skyTint * 0.35f; hor = g.skyTint * 0.75f; break;
        case SKY_FIXED: zen = K.zenith; hor = K.horizon; break;
        default: zen = g.skyTint * 0.75f; hor = lerp(g.skyTint, white, 0.55f); break;
    }
    const MadeWorld& m = g.made;
    if (!m.aired()) return true;
    if (m.ozone > 0) zen = lerp(zen, RGB(0.22f, 0.36f, 0.78f), 0.2f);
    else if (m.ozone < 0) zen = lerp(zen, RGB(0.32f, 0.24f, 0.64f), 0.25f);
    if (m.thick > 0) { hor = lerp(hor, white, (float)(0.35 * m.thick)); zen = lerp(zen, hor, (float)(0.25 * m.thick)); }
    if (m.haze > 0) { zen = lerp(zen, m.hazeTint * 0.72f, (float)(0.8 * m.haze)); hor = lerp(hor, m.hazeTint, (float)(0.9 * m.haze)); }
    if (m.sulphur > 0) hor = lerp(hor, RGB(0.86f, 0.76f, 0.32f), (float)(0.65 * m.sulphur));
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
    const double cover = g.made.cloudMore > 0 ? g.cloudCover + (1 - g.cloudCover) * g.made.cloudMore : g.cloudCover;   // J-04: an endless rain's, a runaway's steam
    double thr = 1.0 - cover;
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

// Z-03 (TR_SNOW_ALGAE): red snow. The algae bloom in the melting surface of the summer snowfields in patches some tens of metres across
// (watermelon snow: Hoham and Remias 2020), thicker in the regions where the melt lingers (a field of a few kilometres); the patches where the eye
// resolves them, the snow white from further (B-404's rule: the unfaded field, so every ring that shows them agrees)
static void snowAlgae(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s) {
    if (s.material != MAT_SNOW || !matVisible(0.025, detailM)) return;
    const Vec3 p = unit * g.R;
    const double region = smoothstep(-0.1, 0.35, fbm3(p / 2.5, g.sG + 341, 2));
    if (region <= 0) return;
    s.snowTint = region * smoothstep(-0.15, 0.35, 0.7 * fbm3(p / 0.024, g.sG + 342, 3) + 0.35 * fbm3(p / 0.010, g.sG + 343, 2));   // patches of cells, not single cells
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

// J-02: a made world's last pass, after every other (galaxy/made.h): the land its seedings' spreads reached under the layer's cover and in
// its tint, never the lava or what lies under a lake or a river. Its share is the spreads' reach, which never fades with the detail, so
// the globe, the maps and the ground draw one edge. J-03: a tint on the waters (a dye's, a glow's) where the spreads' lines live: the
// waters of their salt, the land, the band of warmth (a spread's share where its line lives, the greatest of those that colour)
double madeMeanAt(const BodyGen& g, const MadeWorld& m, const Vec3& unit, const SurfaceSample& s, bool wet, int salt) {
    const double c = placeMeanC(g, m.tempK, m.sunless, unit, wet ? s.water : s.height) + m.warmC;   // (KI-457: at the warmth its air made)
    return wet ? std::max(c, m.freezeC - (salt >= 1 ? 2.0 : 0.0)) : c;   // a sea's never under its freezing
}

// J-05: the plants' colours made lush (their own hue deepened) or bleached (toward a pale yellow, as chlorosis leaves them), then J-04's heat
static RGB madePlantColour(const MadeWorld& m, RGB c, const RGB& straw) {
    if (m.lush > 0) {
        const float y = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
        const RGB deep = RGB(std::max(0.0f, y + 2.0f * (c.r - y)), std::max(0.0f, y + 2.0f * (c.g - y)), std::max(0.0f, y + 2.0f * (c.b - y))) * 0.82f;
        c = lerp(c, deep, (float)m.lush);
    }
    if (m.bleached > 0) c = lerp(c, RGB(0.82f, 0.80f, 0.60f), (float)(0.7 * m.bleached));
    if (m.wilt > 0) c = lerp(c, straw, (float)(0.75 * m.wilt));
    return c;
}
// J-11: a made kind's dead cover in the dead kind's colours: its sand's and snow's families the scorched felisian's (sand, the old seas' salt) or
// the frozen felisian's (the dead soil, the frost), its rock, waters and plants the world's own
// KI-484: a made snowball's sea ice the frozen felisian's blue, in the grass bank as its own is (`matFamilyOnGen`; no grass is left under the frost)
static bool deadCoverColour(int cover, int mat, RGB& c) {
    const int fam = matFamily(mat);
    if (cover == DC_FROZEN && (mat == MAT_ICE || fam == FAM_GRASS)) {
        const KindDef& F = KINDS[PT_FROZEN_FELISIAN];
        for (int i = 0; i < F.nPalette; i++)
            if (F.palette[i].mat == MAT_ICE && F.palette[i].mode == MM_CONST) { c = F.palette[i].c; return true; }
    }
    if (cover == DC_NONE || (fam != FAM_SAND && fam != FAM_SNOW)) return false;
    const KindDef& K = KINDS[cover == DC_SCORCHED ? PT_SCORCHED_FELISIAN : PT_FROZEN_FELISIAN];
    for (int i = 0; i < K.nPalette; i++)
        if (K.palette[i].mat == K.famRep[fam] && K.palette[i].mode == MM_CONST) { c = K.palette[i].c; return true; }
    return false;
}
RGB madeMatColour(const BodyGen& g, int mat, int line) {
    const RGB c = g.matColor[mat];
    const MadeWorld& m = g.made;
    if (madeGrowthMaterial(mat) && m.growthLine >= 0 && m.growthLine < MADE_LINES) {   // J-09: the overgrowth's (KI-473: the line's own where it lies)
        const bool own = line >= 0 && line < m.lines && line < MADE_LINES && m.spread[line].growth.on();
        return m.spread[own ? line : m.growthLine].growth.colour;
    }
    if (mat == MAT_GRASS && m.green > 0 && madeGreens(g.type)) {   // J-15: a carpet's grass and shrubs in its own colour (its line's where it lies)
        int k = line >= 0 && line < m.lines && line < MADE_LINES && m.spread[line].growth.greens ? line : -1;
        for (int i = std::min(m.lines, MADE_LINES) - 1; i >= 0 && k < 0; i--) if (m.spread[i].growth.greens) k = i;
        if (k >= 0) return m.spread[k].growth.plants;
    }
    RGB dead;
    if (m.kind != MDK_NONE && deadCoverColour(m.deadCover(), mat, dead)) return dead;   // J-11
    if (mat == MAT_WATER && m.seaShade > 0) return lerp(c, m.seaShadeTint, (float)(0.8 * m.seaShade));   // J-05: every water turned by a sulphide
    if (m.rustGround > 0 && (mat == MAT_ROCK || mat == MAT_BASALT || mat == MAT_SAND || mat == MAT_DUST || mat == MAT_MUD))
        return lerp(c, RGB(0.62f, 0.30f, 0.16f), (float)(0.6 * m.rustGround));   // J-05: an oxygenless world's ground rusted by an oxygen
    if ((m.wilt <= 0 && m.lush <= 0 && m.bleached <= 0) || (mat != MAT_GRASS && mat != MAT_FOREST) || !kindHas(g.type, KT_FLORA)) return c;
    return madePlantColour(m, c, mat == MAT_FOREST ? RGB(0.50f, 0.42f, 0.25f) : RGB(0.64f, 0.54f, 0.32f));
}
RGB madeFlora2(const BodyGen& g) {
    const MadeWorld& m = g.made;
    if ((m.wilt <= 0 && m.lush <= 0 && m.bleached <= 0) || !kindHas(g.type, KT_FLORA)) return g.vegColor2;
    return madePlantColour(m, g.vegColor2, RGB(0.56f, 0.46f, 0.28f));
}
RGB madeLifeTint(const BodyGen& g) {
    if (g.made.sour <= 0 || !g.hasTrait(TR_REEFS)) return g.lifeTintColour;
    return lerp(g.lifeTintColour, RGB(0.86f, 0.88f, 0.84f), (float)(0.75 * g.made.sour));   // J-05: a sour sea's reefs bleached
}
RGB madeColourAt(const BodyGen& g, int mat, double share, int line) {
    const MadeWorld& m = g.made;
    const MadeSpread* sp = line >= 0 && line < m.lines && line < MADE_LINES ? &m.spread[line] : nullptr;
    if (share > 0.5 && mat != MAT_LAVA && !madeGrowthMaterial(mat)) {   // (J-09: an overgrowth's cover its own colour)
        if (mat == MAT_WATER || (m.tinted && (m.tintOn & MADE_ON_LAND))) { if (m.tinted) return mat == MAT_WATER && sp && sp->waterTinted ? sp->waterTint : m.tint; }
        else {
            const int cls = madeClassOf(g, mat, false);
            const MadeLook* L = sp && sp->look[cls].on ? &sp->look[cls] : m.lookOf(cls);
            if (L && L->tinted) return L->tint;
        }
    }
    return madeMatColour(g, mat, line);
}

// J-05: the salt of the water a shore lies by (a river's or a lake's bank, the sea's flats), -1 none: ground within MADE_SHORE_M over it
static int madeShoreSalt(const BodyGen& g, const SurfaceSample& s) {
    if (s.water > -1e8 && s.height >= s.water && s.height < s.water + MADE_SHORE_M) return madeSaltAt(g, s);
    if (kindHas(g.type, KT_SEA) && s.waterKind == 0) {
        const double S = g.liquidLevel > -1e8 ? g.liquidLevel : 0.0;
        if (s.height >= S && s.height < S + MADE_SHORE_M) return 1;
    }
    return -1;
}
// J-05: whether a land look lies on this ground (`MadeLandOn`): bare ground, the shores (with a water's salt), rock, sand and dust
static bool madeLandOn(int on, int mat, int shoreSalt) {
    if (on == 0) return true;
    if (on & ML_SHORE) return shoreSalt >= 0;
    if (on & ML_ROCK) return mat == MAT_ROCK || mat == MAT_BASALT || mat == MAT_GRAPHITE || mat == MAT_METAL || mat == MAT_QUARTZ;
    if (on & ML_SAND) return mat == MAT_SAND || mat == MAT_DUST;
    return true;   // bare ground: any land of the class
}

// J-09: an overgrowth's cover where it has crept past a half of its share (`madeGrowthAt`, its edge ragged): on the land its line lives on by its
// warmth (never water, lava, snow or ice), a film's on the wet ground by its waters alone. The forest's density is kept under it, so its trees
// stand wrapped in it; elsewhere the plants are gone under it
static void madeGrowth(const BodyGen& g, const Vec3& unit, SurfaceSample& s) {
    const MadeWorld& m = g.made;
    if (s.material == MAT_LAVA || s.material == MAT_SNOW || s.material == MAT_ICE || madeWetAt(s)) return;
    int line;
    const double k = madeGrowthAt(m, unit, g.seed, line);
    if (k < 0.5 || line < 0) return;
    const MadeSpread& sp = m.spread[line];
    const MadeGrowth& G = sp.growth;
    if (G.type == OG_FILMS && madeShoreSalt(g, s) < 0) return;
    if (sp.tLo > -1e8 || sp.tHi < 1e8) {
        const double c = madeMeanAt(g, m, unit, s, false, 0);
        if (c < sp.tLo || c > sp.tHi) return;
    }
    const bool forest = s.material == MAT_FOREST;
    s.material = G.material; s.madeLine = line;   // (KI-473: its own colour where it lies)
    s.albedo = clampd(0.25 + 0.6 * (0.2126 * G.colour.r + 0.7152 * G.colour.g + 0.0722 * G.colour.b), 0.05, 0.9);
    s.scree = 0;
    if (!forest) s.veg = 0;
    if (G.greens) {   // J-15: a carpet on a dry world grows grass and shrubs where it lies, from MADE_GREEN_AFTER steps behind its edge, thicker after
        double thick;
        if (madeGreenAt(m, line, unit, g.seed, thick) >= 0.5) { s.material = MAT_GRASS; s.veg = 0.32 + 0.33 * thick; s.albedo = 0.36 + 0.2 * s.veg; s.madeTint = 0; }   // (over a land look)
    }
}

static void madeGround(const BodyGen& g, const Vec3& unit, SurfaceSample& s) {
    if (s.material == MAT_LAVA) return;
    const bool wet = madeWetAt(s);   // (J-11: a runaway's dried waters are a made hothouse's, its scorched cover laid by the living ground)
    const MadeWorld& m = g.made;
    if (!wet || (m.tintOn & MADE_ON_WATER)) {   // (the cover never lies on water)
        const int salt = wet ? madeSaltAt(g, s) : 0;
        // KI-460: the line whose colour lies here: of those that colour it (a dye, a look), the one of the greatest share, the latest on a tie;
        // where none does, the one that lights it
        double k = 0, kc = 0, meanC = 0;
        int bestW = -1, bestC = -1;
        bool warmth = false;
        for (int i = std::min(m.lines, MADE_LINES) - 1; i >= 0 && kc < 1; i--) {
            const MadeSpread& sp = m.spread[i];
            if (!sp.looks || (wet && !((sp.lookWaters >> clampi(salt, 0, 2)) & 1))) continue;   // (J-05: a rust's look on its fresh waters alone)
            const double r = madeSpreadAt(m, i, unit, g.seed);
            if (r <= k && (!sp.waterColours || r <= kc)) continue;
            if ((sp.tLo > -1e8 || sp.tHi < 1e8) && !warmth) { meanC = madeMeanAt(g, m, unit, s, wet, salt); warmth = true; }   // the place's mean warmth, once
            if (!madeLives(sp, wet, salt, meanC)) continue;
            if (r > k) { k = r; bestW = i; }
            if (sp.waterColours && r > kc) { kc = r; bestC = i; }
        }
        const bool coloured = bestC >= 0 && kc > 0.5;
        if (coloured) bestW = bestC;
        if (k > 0) {
            if (!wet && m.cover >= 0 && k >= 0.5) { s.material = m.cover; s.albedo = m.coverAlbedo; s.veg = 0; s.scree = 0; }
            if (m.tinted && (m.tintOn & (wet ? MADE_ON_WATER : MADE_ON_LAND))) { s.madeTint = coloured ? kc : k; s.madeLine = bestW; }   // (KI-460: its line's own colour at its share)
        }
    }
    if (wet || (m.tinted && (m.tintOn & MADE_ON_LAND))) return;   // (the test layer's tint is its land's look)
    // J-05: the land's, the snow's and the plants' looks where a line that makes them lives: its land (bare ground, the shores of its waters,
    // rock, sand), its snow and ice, its plants (where frosts come for a frost's burn), at its share; over a half its cover and its albedo
    const int cls = madeClassOf(g, s.material, false);
    const MadeLook* L = m.lookOf(cls);
    if (!L || !L->on) return;
    const int shoreSalt = cls == MC_LAND ? madeShoreSalt(g, s) : -1;
    // KI-460: the line whose look lies here: of those whose look shows a colour (or a forest's glow), the one of the greatest share, the latest
    // on a tie; else the one of the greatest share
    double k = 0, kt = 0, meanC = 0;
    int best = -1, bestT = -1;
    bool warmth = false;
    for (int i = std::min(m.lines, MADE_LINES) - 1; i >= 0 && kt < 1; i--) {
        const MadeSpread& sp = m.spread[i];
        if (!((sp.makes >> cls) & 1) || (cls == MC_LAND && !madeLandOn(sp.landOn, s.material, shoreSalt))) continue;
        const double r = madeSpreadAt(m, i, unit, g.seed);
        const bool shows = sp.look[cls].tinted || (cls == MC_PLANTS && sp.plantsGlow() > 0);
        if (r <= k && (!shows || r <= kt)) continue;
        if (!warmth) { meanC = madeMeanAt(g, m, unit, s, false, 0); warmth = true; }
        bool lives;
        if (cls == MC_PLANTS) lives = sp.plants && meanC > sp.tLo && meanC < sp.tHi && meanC < sp.coldC;
        else if (cls == MC_LAND && (sp.landOn & ML_SHORE)) lives = ((sp.waters >> clampi(shoreSalt, 0, 2)) & 1) && meanC > sp.tLo && meanC < sp.tHi;
        else lives = madeLives(sp, false, 0, meanC);
        if (!lives) continue;
        if (r > k) { k = r; best = i; }
        if (shows && r > kt) { kt = r; bestT = i; }
    }
    if (bestT >= 0 && kt > 0.5) { best = bestT; k = kt; }
    const MadeLook* own = best >= 0 && m.spread[best].look[cls].on ? &m.spread[best].look[cls] : L;   // KI-460: the line's own look where it lies
    if (k > 0 && cls == MC_PLANTS && own->kind == MP_BLIGHT && own->variant < 2) {   // a blight in spots (a hundred metres or so) or stands (a kilometre or two)
        const Vec3 p = unit * g.R;
        k *= own->variant == 0 ? smoothstep(0.2, 0.3, fbm3(p / 0.12, g.sG + 351, 2)) : smoothstep(-0.05, 0.05, fbm3(p / 1.5, g.sG + 352, 3));
    }
    if (k > 0 && cls == MC_LAND && own->kind == MGT_COLOURED_SEEPS)   // J-16 (KI-506): a rock line's seeps in patches of a few hundred metres
        k *= smoothstep(0.08, 0.2, fbm3(unit * g.R / 0.35, g.sG + 353, 3));
    if (k <= 0) return;
    s.madeTint = k; s.madeLine = best;
    if (k < 0.5) return;
    if (cls == MC_LAND && m.spread[best].cover >= 0) { s.material = m.spread[best].cover; s.albedo = m.spread[best].coverAlbedo; s.veg = 0; s.scree = 0; }
    if (own->albedo > 0) s.albedo = cls == MC_SNOW ? std::min(s.albedo, own->albedo) : own->albedo;   // a chalk's white, a glass's, a dark ice's grey
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
        if (g.liquidLevel > -1e8 && s.height < g.liquidLevel && s.material != MAT_LAVA && !(g.made.on() && g.made.dried())) {   // (KI-458: none on a made hothouse)
            s.material = MAT_WATER; s.water = g.liquidLevel; s.albedo = 0.22; s.glow = 0;
        }
    }
    if (g.tideRangeM > 0 && !(g.made.on() && g.made.deadCover() != DC_NONE)) tidalFlat(g, unit, detailM, s);   // (J-11: no tide on a made kind's dry or frozen seas)
    qualityWater(g, unit, detailM, s);
    if (g.hasTrait(TR_SNOW_ALGAE)) snowAlgae(g, unit, detailM, s);
    if (g.dark) darkGround(c);
    if (g.made.on() && g.made.ground()) madeGround(g, unit, s);   // J-02: last of all
    if (g.made.on() && g.made.grown()) madeGrowth(g, unit, s);    // J-09: an overgrowth's cover over the rest
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
    const bool madeTints = g.made.on() && g.made.shares();   // J-02; J-05: the land's, the snow's and the plants' looks too
    mtint.assign(madeTints ? W * H : 0, 0);
    mline.assign(madeTints || (g.made.on() && g.made.grown()) ? W * H : 0, 0);
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
            if (madeTints) mtint[i] = (uint8_t)clampi((int)(s.madeTint * 255 + 0.5), 0, 255);
            if (!mline.empty()) mline[i] = (uint8_t)(s.madeLine + 1);
            cloud[i] = (uint8_t)clampi((int)(sampleCloudPattern(g, lon, lat) * 255 + 0.5), 0, 255);
            // a polar vortex pattern on gas giants
            if (kindHas(g.type, KT_GAS_GIANT) && std::fabs(lat) > 76 * DEG)
                albedo[i] = (uint8_t)clampi((int)(albedo[i] * (1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat)))), 0, 255);
        }
    }
    seed = g.seed;
    madeKey = g.made.key;
    valid = true;
}
