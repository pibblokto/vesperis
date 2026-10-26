// E-08: the frame's palette: the look a world's kind and palette give (`lookFor`), the banks' ramps (`setupPalette`: the
// ground's families, the sky, the water, the aurorae, the exposure) and the frame's lamps (`setupLamps`, `lampAt`)
#include "surface_internal.h"

// the ground colours come from the body's material palette (BodyGen::matColor, shared with the globe
// and the landing map); the sky, the fog, the night and the cloud layer are the kind's row
SurfaceLook lookFor(const BodyGen& g, const Star& star) {
    SurfaceLook L;
    const KindDef& K = KINDS[g.type];
    RGB lc = lerp(star.color, RGB(1, 1, 1), 0.5f);   // the star's light, softened (the globe and the map use the same tint, N1-05)
    RGB white(1, 1, 1);
    // the terrain is drawn to 53 km now, so the haze must let a clear day show it: felisian 12 km (was 5),
    // ocean 9, quartz 5, thin air 14, the airless worlds 45 (their "fog" is only the softening of distance)
    L.cloudsLayer = K.clouds;
    L.skyDensity = K.skyDensity;
    L.fogDistance = K.fog;
    L.nightZenith = K.ownNight ? K.nightZenith : RGB(0.01f, 0.01f, 0.03f);
    L.nightHorizon = K.ownNight ? K.nightHorizon : RGB(0.04f, 0.04f, 0.07f);
    L.zenith = RGB(0, 0, 0); L.horizon = RGB(0, 0, 0);
    typeSky(g, L.zenith, L.horizon);   // one pair per kind, shared with the globe and the landing map
    if (K.seaSky) { L.zenith = g.skyTint * 0.8f; L.horizon = lerp(g.skyTint, white, 0.5f); }   // the ocean's own sky over its sea
    if (K.lavaHorizon) L.horizon = K.horizonGlow;   // an airless horizon lit by the lava
    if (g.hasTrait(TR_HAZE)) { L.fogDistance *= 0.3; L.skyDensity *= 1.25; }
    if (g.hasTrait(TR_FROZEN_AIR)) { L.fogDistance = 45000; L.cloudsLayer = false; }   // its air on its ground, its sky an airless world's
    const RGB* m = g.matColor;
    L.ground = m[familyRep(g.type, FAM_ROCK)];
    L.sand = m[familyRep(g.type, FAM_SAND)];
    L.snow = m[familyRep(g.type, FAM_SNOW)];
    L.grass = m[familyRep(g.type, FAM_GRASS)];   // a kind's first accent where it lends the grass bank
    L.flora2 = g.vegColor2;
    // bank 2 (secondary): water where there is a sea, lava on hot worlds, else the bright crystal/ejecta tint
    if ((K.tags & KT_SEA) || g.liquidLevel > -1e8) L.secondary = m[MAT_WATER];
    else if (isLavaWorld(g.type)) L.secondary = m[MAT_LAVA];
    else if (g.hasTrait(TR_GLASSED)) L.secondary = m[MAT_GLASS];   // the sheets of glass own bank 2 (the family's rep is the water the palette coloured as glass)
    else if (K.famRep[FAM_WATER] != MAT_WATER) L.secondary = m[K.famRep[FAM_WATER]];   // a kind's own in the water family's bank (the glass world's glass)
    else if (K.tags & KT_CRYSTAL) L.secondary = lerp(m[MAT_QUARTZ], white, 0.4f);
    else L.secondary = lerp(L.ground, white, 0.5f);
    // bank 3 (tertiary): the forest on living worlds (P-00: a kind's second accent), elsewhere boulders a shade lighter than the ground
    L.tertiary = ((K.tags & KT_FLORA) || K.accent[1] >= 0) ? m[familyRep(g.type, FAM_FOREST)] : lerp(L.ground, white, 0.25f);
    L.ground = L.ground * lc; L.secondary = L.secondary * lc; L.tertiary = L.tertiary * lc;
    L.sand = L.sand * lc; L.snow = L.snow * lc; L.grass = L.grass * lc; L.flora2 = L.flora2 * lc;
    L.zenith = L.zenith * lc; L.horizon = L.horizon * lc;
    return L;
}

// R-402: the aurora's colours per world: the lower curtain's and its tops', by the air and a hash (oxygen airs green with red or
// violet tops, as on Earth; thin air green or teal under violet; a desert's lime under pink; a sulphurous sky blue under
// magenta; an acid sky gold under orange)
static void auroraColours(const Body& b, RGB& low, RGB& high) {
    double u = unitFromHash(hashCombine(b.seed, 0xA0C));
    const RGB green(0.1f, 0.45f, 0.2f), teal(0.1f, 0.42f, 0.36f), lime(0.3f, 0.5f, 0.12f), blue(0.15f, 0.3f, 0.7f), gold(0.55f, 0.48f, 0.12f);
    const RGB red(0.55f, 0.12f, 0.16f), violet(0.4f, 0.18f, 0.6f), pink(0.6f, 0.22f, 0.4f), magenta(0.6f, 0.12f, 0.5f), orange(0.7f, 0.3f, 0.08f);
    switch (KINDS[b.type].aurora) {   // the kind's style
        case AU_THIN: low = u < 0.5 ? green : teal; high = violet; break;
        case AU_DESERT: low = u < 0.5 ? lime : green; high = pink; break;
        case AU_SULPHUR: low = blue; high = magenta; break;
        case AU_ACID: low = gold; high = orange; break;
        default: low = u < 0.7 ? green : (u < 0.85 ? teal : lime); high = u < 0.6 ? red : (u < 0.85 ? violet : pink); break;
    }
}

void SurfaceView::setupPalette(Framebuffer& fb) {
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    double day = env.skyBrightness;
    double sinAlt = env.sun.dirLocal.y;
    RGB white(1, 1, 1);
    RGB starC = lerp(env.lightColor, white, 0.5f);   // M5-01: both suns tint the light
    RGB zen = lerp(L.nightZenith, L.zenith, (float)day);
    RGB hor = lerp(L.nightHorizon, L.horizon, (float)day);
    // sunset tint near the horizon
    double sunset = (1 - smoothstep(0.02, 0.22, sinAlt)) * smoothstep(-0.16, -0.03, sinAlt);
    if (site.atmosphere) {
        RGB sunsetC = RGB(0.95f, 0.45f, 0.2f) * lerp(env.lightColor, white, 0.5f);
        if (kindHas(site.gen.type, KT_EXOTIC_TWILIGHT)) sunsetC = lerp(sunsetC, site.gen.skyTint * 1.3f, 0.65f);   // exotic twilight
        hor = lerp(hor, sunsetC, (float)(sunset * 0.8));
        zen = lerp(zen, sunsetC * 0.4f, (float)(sunset * 0.25));
    }
    double rainDim = (1 - 0.35 * env.rain) * (1 - 0.3 * env.frontCloud);   // a front's deck is a storm's: the sky's colours dim under it
    hor = hor * (float)rainDim; zen = zen * (float)rainDim;
    RGB fog = site.atmosphere ? hor : lerp(L.ground, white, 0.55f);
    RGB glow = lerp(hor, starC, 0.5f);
    RGB sunC = site.atmosphere ? lerp(starC, white, 0.6f) : lerp(env.sunDisc.color, white, 0.35f);
    if (env.hasSun2) {   // bank 12: the companion's disc and glow in its own colour
        RGB kc = env.sun2.color;
        RGB kd = site.atmosphere ? lerp(lerp(kc, white, 0.5f), white, 0.6f) : lerp(kc, white, 0.35f);
        setRamp(fb.pal, 12, {{0, RGB(0, 0, 0)}, {20, kc * 0.45f}, {44, kc}, {56, kd}, {63, lerp(kd, white, 0.5f)}});
    }
    if (env.moonBody >= 0 && kindHas(site.sys->bodies[env.moonBody].type, KT_GLOWS)) {   // bank 13: a glowing parent in the sky
        RGB g = site.sys->bodies[env.moonBody].color;
        setRamp(fb.pal, 13, {{0, RGB(0, 0, 0)}, {14, g * 0.4f}, {34, g}, {50, lerp(g, RGB(1, 0.55f, 0.25f), 0.6f)}, {63, RGB(1, 0.8f, 0.5f)}});
    }
    float lf = (float)clampd(env.sun.lightFactor, 0.5, 1.15 * (1 + 0.25 * env.flare));   // a flare opens the exposure a quarter past its clamp
    // M10-07: the dark end of a material ramp takes the sky's colour (hemisphere light), the lit end
    // is warmed at sunset by the low sun, and an overcast sky flattens the contrast
    RGB skyLight = lerp(zen, hor, 0.5f);
    double overcast = site.atmosphere ? env.cloudCover : 0.0;
    RGB sunsetTint = lerp(RGB(1, 1, 1), RGB(1.15f, 0.72f, 0.5f), (float)(site.atmosphere ? sunset * 0.7 : 0.0));
    auto matRamp = [&](int bank, RGB c) {
        // the shared material ramp (hemisphere light included), then the surface's own terms: moonlight
        // in the dark end, the sunset tint and the overcast flattening in the lit end, the fog colour at the top
        MatRamp r = materialRamp(c, site.atmosphere && env.pressureAtm >= 0.001 ? &skyLight : nullptr, day);   // an air of millionths of a bar lights nothing
        RGB dark = r.stop[1];
        // planetshine lifts the dark end at night only (N1-05: on airless worlds `day` is the sky, which is always dark, so use the sun)
        double night = site.atmosphere ? (1 - day) : (1 - smoothstep(-0.05, 0.1, sinAlt));
        if (env.moonLight > 0.01) dark = lerp(dark, env.moonColor * (0.22f + 0.3f * (float)env.moonLight), (float)(night * clampd(env.moonLight * 2.5, 0, 0.7)));
        RGB lit = c * sunsetTint * (float)(1.0 - 0.12 * overcast);
        RGB mid = lerp(dark, lit, 0.55f);
        setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {14, dark}, {34, mid}, {47, lit}, {56, lerp(lit, white, 0.5f)}, {63, fog}});
    };
    matRamp(0, L.ground);
    if (lavaGlow) {
        setRamp(fb.pal, 2, {{0, RGB(0, 0, 0)}, {14, RGB(0.16f, 0.03f, 0.01f)}, {26, RGB(0.42f, 0.06f, 0)}, {40, RGB(0.8f, 0.14f, 0)}, {52, RGB(1, 0.5f, 0.05f)}, {60, RGB(1, 0.9f, 0.5f)}, {63, RGB(1, 1, 0.85f)}});
    } else if (site.hasWater) {   // the sea's ramp for every liquid (methane, acid, brine), not the living worlds' alone
        float dayF = 0.25f + 0.75f * (float)day;   // the water goes dark with the sky (the shade does not carry the night)
        RGB w = L.secondary * (0.7f + 0.3f * lf) * dayF;
        RGB wb = lerp(hor, white, 0.2f);
        setRamp(fb.pal, 2, {{0, RGB(0, 0, 0)}, {16, w * 0.3f}, {34, w * 0.7f}, {46, w}, {56, lerp(w, wb, 0.6f)}, {63, fog}});
    } else {
        matRamp(2, L.secondary);
    }
    matRamp(3, L.tertiary);
    if (site.gen.hasTrait(TR_LUMINOUS_FLORA) && day < 0.5) {   // the leaves carry their own light at night
        RGB g3 = L.tertiary * 1.3f, gl = lerp(g3, RGB(0.6f, 1.0f, 0.7f), 0.35f);
        float k = (float)(1 - day * 2);
        setRamp(fb.pal, 3, {{0, RGB(0, 0, 0)}, {14, lerp(L.tertiary * 0.25f, g3 * 0.5f, k)}, {34, lerp(L.tertiary * 0.6f, g3, k)}, {47, lerp(L.tertiary, gl, k)}, {56, lerp(L.tertiary, white, 0.5f)}, {63, white}});
    }
    // family banks: snow/ice 8 (bank 2 stays water-only for the reflection pass), sand/dust/sulphur/quartz 9, grass 10
    matRamp(8, L.snow);
    matRamp(9, L.sand);
    matRamp(10, L.grass);
    // the buggy: bank 6 its hull (the ship's grey-blue), bank 7 chrome (tubes, rims, dials), bank 14 warning red (tail lights, needles)
    matRamp(6, RGB(0.52f, 0.55f, 0.6f));
    setRamp(fb.pal, 7, {{0, RGB(0, 0, 0)}, {20, RGB(0.42f, 0.44f, 0.5f)}, {44, RGB(0.82f, 0.84f, 0.88f)}, {63, white}});
    setRamp(fb.pal, 14, {{0, RGB(0, 0, 0)}, {14, RGB(0.3f, 0.05f, 0.03f)}, {40, RGB(0.9f, 0.15f, 0.1f)}, {63, RGB(1, 0.65f, 0.5f)}});
    {   // bank 5: the second flora family's colour; deciduous leaves turn in the cold shoulder seasons
        double ph = seasonPhaseLocal();
        RGB f2 = (ph < 0 && ph >= -0.6) ? lerp(site.gen.vegColor, RGB(0.85f, 0.5f, 0.12f), 0.65f) * lerp(site.sys->star.color, white, 0.5f) : L.flora2;
        matRamp(5, f2);
    }
    {   // aurora: bank 11 its lower colour, bank 21 (R-402) its tops' colour, per world; B-403: their dark end is the night
        // sky's own colour, so the curtains fade into the sky without a hue edge (a thin atmosphere's night sky is brown, and
        // black-green against it cut the curtains' faint top into patches)
        RGB low, high; auroraColours(site.sys->bodies[site.body], low, high);
        setRamp(fb.pal, 11, {{0, zen}, {10, lerp(zen, low, 0.4f)}, {30, low}, {63, lerp(low, white, 0.55f)}});
        setRamp(fb.pal, 21, {{0, zen}, {10, lerp(zen, high, 0.4f)}, {30, high}, {63, lerp(high, white, 0.45f)}});
    }
    {   // bank 16: bark, a brown of the planet's own (toward the second flora colour); bank 17: cut wood, pale. On
        // the ground bank, whose dark stops take the sky's colour, trunks would be black and branches grey-blue
        RGB bark = lerp(RGB(0.36f, 0.25f, 0.15f), site.gen.vegColor2 * 0.55f, 0.3f), wood(0.74f, 0.62f, 0.44f);
        const int cover = KINDS[site.gen.type].cover;   // a frozen world's dead wood white with frost, a scorched world's turned to stone
        if (cover == DC_FROZEN) { bark = RGB(0.80f, 0.84f, 0.90f); wood = RGB(0.86f, 0.88f, 0.92f); }
        if (cover == DC_SCORCHED) { bark = RGB(0.58f, 0.48f, 0.40f); wood = RGB(0.74f, 0.56f, 0.46f); }
        if (site.gen.hasTrait(TR_FUNGAL)) { bark = lerp(site.gen.matColor[MAT_FOREST], RGB(0.92f, 0.90f, 0.86f), 0.6f); wood = RGB(0.90f, 0.86f, 0.78f); }   // the stems pale
        matRamp(16, bark);
        matRamp(17, wood);
    }
    // banks 29-31: the records' (peoples/records.h): bone, parchment and paper pale; wax, leather, vinyl and dark glass; a painting's ochre
    // (clay is the cut wood's bank 17, metal the chrome's 7)
    matRamp(29, RGB(0.86f, 0.82f, 0.7f));
    matRamp(30, RGB(0.2f, 0.16f, 0.13f));
    matRamp(31, RGB(0.66f, 0.3f, 0.15f));
    {   // the peoples' palettes (peoples/forms.h): the structure, the accent and metal and glass through the material ramp (the
        // structure the ground's own where the people built of it: C-01's walls of the rock, or the sand of adobe, the same ramps
        // as banks 0 and 9), light a ramp of its own that glows whatever the sun (its top the fog's, as every ramp's)
        RGB lcol = lerp(site.sys->star.color, white, 0.5f);
        for (int p = 0; p < 2; p++) {
            const FormPalette& P = palettes[p];
            // a slot of the ground's own tinted toward the people's colour (`tint`); slot 3 light only where the palette glows (the
            // futuristic and the far future's), else a material (timber, rust)
            for (int k = 0; k < 3; k++) matRamp(FORM_BANKS[p][k], P.own[k] ? lerp(cultures[p].family == 1 ? L.sand : L.ground, P.colour[k] * lcol, P.tint[k]) : P.colour[k] * lcol);
            RGB c = P.colour[3];
            if (P.glow) setRamp(fb.pal, FORM_BANKS[p][3], {{0, RGB(0, 0, 0)}, {24, c * 0.3f}, {46, c}, {56, lerp(c, white, 0.45f)}, {63, fog}});
            else matRamp(FORM_BANKS[p][3], c * lcol);
        }
    }
    if (site.gen.type == PT_COMET) setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {18, RGB(0.10f, 0.14f, 0.30f)}, {40, RGB(0.32f, 0.48f, 0.85f)}, {63, RGB(0.7f, 0.8f, 1.0f)}});   // bank 15: the ion tail overhead
    else if (KINDS[site.gen.type].events & EV_SPRINGS) setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {20, RGB(0.06f, 0.16f, 0.13f)}, {44, RGB(0.36f, 0.88f, 0.74f)}, {63, RGB(0.80f, 1.0f, 0.94f)}});   // bank 15: the spores' own light
    {   // (after the peoples' banks: it takes the second people's on worlds that have none): the water's trait colours, each a water's ramp in its own colour (a pink lake's, a bloom's, a lagoon's), and the milky sea's glow at
        // night (its dark end the night sea's, its light the bacteria's pale blue)
        tintBankW = tintBankL = -1; tintGlow = 0;
        const float dayF = 0.25f + 0.75f * (float)day;
        auto waterTint = [&](int bank, RGB c) {
            RGB w = c * (0.7f + 0.3f * lf) * dayF, wb = lerp(hor, white, 0.2f);
            setRamp(fb.pal, bank, {{0, RGB(0, 0, 0)}, {16, w * 0.3f}, {34, w * 0.7f}, {46, w}, {56, lerp(w, wb, 0.6f)}, {63, fog}});
        };
        // the peoples' banks are the traits' on a world without a people (its traits that take them move its ground: never on a people's), but
        // where the harness stages a people's forms (A-02's tests) they keep theirs
        const bool formsStaged = !placedCells.empty();
        if (site.gen.hasTrait(TR_PINK_LAKES) && !formsStaged) { tintBankW = 25; waterTint(25, site.gen.waterTintColour); }
        tintBankSprings = -1;
        if (site.gen.hasTrait(TR_HOT_SPRINGS) && !formsStaged) { tintBankSprings = 25; matRamp(25, site.gen.matColor[MAT_MATS] * lerp(site.sys->star.color, white, 0.5f)); }   // the springs' mats
        if (site.gen.hasTrait(TR_MILKY_SEAS)) {
            tintGlow = 1 - smoothstep(0.04, 0.3, day);
            if (tintGlow > 0.02) {
                tintBankW = 15;
                const RGB m = site.gen.waterTintColour, dark = L.secondary * 0.25f * dayF;
                setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {14, dark}, {28, lerp(dark, m * 0.45f, (float)tintGlow)}, {44, lerp(dark, m * 0.85f, (float)tintGlow)}, {63, lerp(dark, lerp(m, white, 0.35f), (float)tintGlow)}});
            }
        }
        if (waterLights) {   // a dark ocean's life, its own light: faint in the black water, in patches that drift (the sheet's)
            tintGlow = 1; tintBankW = 15;
            const RGB m = site.gen.waterTintColour, dark = L.secondary * 0.25f * dayF;
            setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {14, dark}, {28, lerp(dark, m * 0.3f, 0.8f)}, {44, m * 0.62f}, {63, lerp(m, white, 0.25f)}});
        }
        if (site.gen.hasTrait(TR_LIGHTS_LEFT_ON)) setRamp(fb.pal, 15, {{0, RGB(0, 0, 0)}, {22, RGB(0.30f, 0.20f, 0.08f)}, {44, RGB(0.86f, 0.62f, 0.30f)}, {58, RGB(1.0f, 0.86f, 0.56f)}, {63, RGB(1, 0.97f, 0.86f)}});   // the lamps' and the windows' light
        if ((site.gen.hasTrait(TR_BLOOMS) || site.gen.hasTrait(TR_REEFS)) && !formsStaged) { tintBankL = 26; waterTint(26, site.gen.hasTrait(TR_BLOOMS) ? lerp(site.gen.matColor[MAT_WATER], site.gen.lifeTintColour, 0.75f) : site.gen.lifeTintColour); }   // a bloom the water's own colour turned
        tintBankStrata = -1;
        if (site.gen.hasTrait(TR_STRATA) && !formsStaged) {   // the rainbow strata's accent: a red, an orange or a green by the world
            static const RGB A[3] = {RGB(0.74f, 0.30f, 0.20f), RGB(0.82f, 0.52f, 0.20f), RGB(0.46f, 0.56f, 0.36f)};
            tintBankStrata = 27; matRamp(27, A[(int)(hash01(mix64(site.gen.seed ^ 0x57A7BULL)) * 3) % 3] * lerp(site.sys->star.color, white, 0.5f));
        }
        if (site.gen.hasTrait(TR_PETRIFIED) && !formsStaged) {   // the stone trunks' jasper: red-brown, ochre or a purple grey by the world
            static const RGB J[3] = {RGB(0.62f, 0.32f, 0.22f), RGB(0.70f, 0.52f, 0.28f), RGB(0.54f, 0.42f, 0.52f)};
            matRamp(26, J[(int)(hash01(mix64(site.gen.seed ^ 0x9E7AULL)) * 3) % 3] * lerp(site.sys->star.color, white, 0.5f));
        }
    }
    // bank 4: stars (their own ramp, not the ground's); D-04: reddened through a thick air (the share it lets through)
    {
        const float red = site.atmosphere ? (float)(1.0 - KINDS[site.gen.type].starLight) : 0.0f;
        setRamp(fb.pal, 4, {{0, RGB(0, 0, 0)}, {40, lerp(RGB(0.55f, 0.57f, 0.66f), RGB(0.66f, 0.46f, 0.36f), red)}, {63, lerp(RGB(0.88f, 0.9f, 0.97f), RGB(0.98f, 0.80f, 0.64f), red)}});
    }
    {   // bank 22: the galactic band and the nebula patches, the unresolved starlight, laid over the sky at night by
        // `drawBand`: from the night sky's own colour at the dark end (no hue edge where it fades into the sky) to a pale
        // star-grey, a little of the horizon's tint through an atmosphere
        RGB pale = lerp(RGB(0.55f, 0.57f, 0.66f), hor, site.atmosphere ? 0.25f : 0.0f);
        if (site.sys->star.cls == STAR_PROTOSTAR) {   // the sky's glow takes the protostar's cloud's tone
            int tone = starNebulaTone(site.sys->star);
            pale = lerp(pale, tone == 0 ? RGB(0.45f, 0.55f, 0.85f) : (tone == 1 ? RGB(0.85f, 0.50f, 0.45f) : RGB(0.70f, 0.70f, 0.75f)), 0.6f);
        }
        // an airless sky lies in this bank all over, and the sun's disc and glow are drawn in it (`drawSky`): the band's
        // own shades (under 20) keep the star-grey, the glow round the sun turns to the sun's colour above them, and from 55 up
        // the stops are the sky bank's, so the disc is the colour it was
        if (site.atmosphere || site.sys->star.cls == STAR_PROTOSTAR) setRamp(fb.pal, 22, {{0, site.atmosphere ? zen : RGB(0, 0, 0)}, {40, pale}, {63, lerp(pale, white, 0.4f)}});
        else {
            auto lumOf = [](RGB c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; };
            RGB sunN = sunC * (lumOf(pale) / std::max(0.05f, lumOf(sunC)));   // the sun's hue at the pale grey's light
            setRamp(fb.pal, 22, {{0, RGB(0, 0, 0)}, {20, pale * 0.5f}, {40, lerp(pale, sunN, 0.7f)}, {55, sunC * 0.9f}, {60, sunC}, {63, lerp(sunC, white, 0.45f)}});
        }
    }
    // sky bank: zenith .. horizon .. glow .. sun/star white
    if (site.atmosphere) {
        setRamp(fb.pal, 1, {{0, zen}, {40, hor}, {50, glow}, {56, lerp(glow, sunC, 0.6f)}, {63, sunC}});
        // at night the upper part must still serve stars: brighten toward white
        if (day < 0.5) {
            RGB starWhite = RGB(0.8f, 0.82f, 0.9f);
            setRamp(fb.pal, 1, {{0, zen}, {36, hor}, {44, lerp(hor, starWhite, 0.25f)}, {54, lerp(hor, starWhite, (float)(0.6 + 0.3 * day))}, {63, lerp(starWhite, sunC, (float)day)}});
        }
        // bank 23: a front's deck and the rain under it. The sky bank runs from the zenith's colour to the horizon's, and on
        // a world whose two are close (a red dwarf's mauve) nothing drawn in it can be dark; this ramp shares the sky's stops
        // from the horizon up (the lit tops look the same in either bank) and goes down to a storm's grey instead of the zenith
        {
            RGB stormGrey = RGB(0.40f, 0.40f, 0.44f) * (float)rainDim;
            RGB g1 = lerp(zen, stormGrey, (float)day), g0 = g1 * 0.6f;
            if (day < 0.5) setRamp(fb.pal, 23, {{0, g0}, {18, g1}, {36, hor}, {44, lerp(hor, RGB(0.8f, 0.82f, 0.9f), 0.25f)}, {54, lerp(hor, RGB(0.8f, 0.82f, 0.9f), (float)(0.6 + 0.3 * day))}, {63, lerp(RGB(0.8f, 0.82f, 0.9f), sunC, (float)day)}});
            else setRamp(fb.pal, 23, {{0, g0}, {18, g1}, {36, lerp(g1, hor, 0.8f)}, {40, hor}, {50, glow}, {56, lerp(glow, sunC, 0.6f)}, {63, sunC}});
        }
    } else {
        RGB grey(0.42f, 0.43f, 0.48f);
        setRamp(fb.pal, 1, {{0, RGB(0, 0, 0)}, {30, RGB(0.05f, 0.05f, 0.07f)}, {44, lerp(grey, sunC, 0.3f) * 0.8f}, {50, lerp(grey, sunC, 0.5f)}, {55, sunC * 0.9f}, {60, sunC}, {63, lerp(sunC, white, 0.45f)}});
    }
    if (lightningFlash > 0) {
        for (int i = 0; i < (int)sizeof(fb.pal); i++) fb.pal[i] = (uint8_t)std::min(255.0, fb.pal[i] + 120 * lightningFlash);
    }
    if (env.dust > 0.05) {
        // dust storm: everything goes the colour of the dust, the sky included
        RGB dc = site.gen.color * 0.85f;
        for (int i = 0; i < (int)sizeof(fb.pal); i += 3) {
            double f = 0.7 * env.dust;
            fb.pal[i] = (uint8_t)(fb.pal[i] * (1 - f) + dc.r * 255 * f * 0.9); fb.pal[i + 1] = (uint8_t)(fb.pal[i + 1] * (1 - f) + dc.g * 255 * f * 0.9); fb.pal[i + 2] = (uint8_t)(fb.pal[i + 2] * (1 - f) + dc.b * 255 * f * 0.9);
        }
    }
    if (player.underwater || viewUnderwater) {
        // everything seen through water goes blue-green and dim
        for (int i = 0; i < (int)sizeof(fb.pal); i += 3) {
            fb.pal[i] = (uint8_t)(fb.pal[i] * 0.35); fb.pal[i + 1] = (uint8_t)(fb.pal[i + 1] * 0.75 + 20); fb.pal[i + 2] = (uint8_t)std::min(255.0, fb.pal[i + 2] * 0.9 + 45);
        }
    }
}

// D-04: the frame's lights. The helmet lamp a cone from the eye along the view: full within 11 degrees of its axis, gone at 22, a faint spill
// to 45; its light 3 at the eye falling as 1 / (1 + (d / 16 m)^2), to nothing from 42 m to 60 m. The capsule's floodlights on the way
// down (and up) a wide cone down and ahead from it, from 500 m. Both scaled by the night (a dayside's sun over three degrees up: none)
void SurfaceView::setupLamps() {
    nLamps = 0;
    const double night = 1 - smoothstep(-0.03, 0.06, env.sun.dirLocal.y);
    if (night < 0.01) return;
    if (lamp && !inVehicle() && cameraOverrideAlt < 0 && !freeCam) {
        LampLight& L = lamps[nLamps++];
        L.pos = camPos; L.dir = Vec3(camLocal.m[2][0], camLocal.m[2][1], camLocal.m[2][2]);
        L.cosIn = std::cos(11 * DEG); L.cosOut = std::cos(22 * DEG); L.cosSpill = std::cos(45 * DEG);
        L.nearM = 16; L.reach0 = 42; L.reach = 60; L.power = 3.0 * night;
    }
    if (floodlights && cameraOverrideAlt >= 0 && cameraOverrideAlt < 500) {
        LampLight& L = lamps[nLamps++];
        L.pos = camPos; L.dir = normalize(Vec3(camLocal.m[2][0], 0, camLocal.m[2][2]) * 0.6 + Vec3(0, -1, 0));   // down and ahead, where it comes down
        L.cosIn = std::cos(40 * DEG); L.cosOut = std::cos(70 * DEG); L.cosSpill = std::cos(85 * DEG);
        L.nearM = 140; L.reach0 = 480; L.reach = 650; L.power = 1.4 * night;
    }
}
double SurfaceView::lampAt(const Vec3& p, const Vec3* n) const {
    double sum = 0;
    for (int i = 0; i < nLamps; i++) {
        const LampLight& L = lamps[i];
        const Vec3 d = p - L.pos;
        const double d2 = dot(d, d);
        if (d2 > L.reach * L.reach || d2 < 1e-6) continue;
        const double dist = std::sqrt(d2);
        const Vec3 u = d / dist;
        const double c = dot(u, L.dir);
        if (c <= L.cosSpill) continue;
        const double core = smoothstep(L.cosOut, L.cosIn, c), cone = core + 0.22 * smoothstep(L.cosSpill, L.cosOut, c) * (1 - core);
        const double fall = (1 - smoothstep(L.reach0, L.reach, dist)) / (1 + d2 / (L.nearM * L.nearM));
        sum += L.power * cone * fall * (n ? 0.35 + 0.65 * std::max(0.0, -dot(*n, u)) : 0.8);   // a rough ground's grains catch a grazing light (no face lit at all only edge-on)
    }
    return std::min(sum, 1.0);   // a noon's light at the most: a face turned to the lamp near the eye glares no whiter than a sunlit one
}
