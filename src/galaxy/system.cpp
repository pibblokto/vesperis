#include "system.h"
#include "core/rng.h"
#include "core/noise.h"
#include <cmath>

const PlanetTypeInfo PLANET_TYPES[PT_COUNT] = {
    {"MOLTEN", "medium size, internally hot, unstable surface, no atmosphere.", 2000, 4200, true, false, 1, 0.25},
    {"CRATERED", "small, solid, dusty, craterized, no atmosphere.", 900, 3200, true, false, 1, 0.20},
    {"VENUSIAN", "medium size, solid, thick atmosphere, fully covered by clouds.", 4800, 7200, true, true, 2, 0.75},
    {"FELISIAN", "medium size, felisian, breathable atmosphere, suitable for life.", 4800, 7600, true, true, 3, 0.35},
    {"ROCKY", "medium size, rocky, creased, no atmosphere.", 2800, 6000, true, false, 2, 0.18},
    {"THIN ATMOSPHERE", "small, solid, thin atmosphere.", 2400, 4600, true, true, 2, 0.30},
    {"GAS GIANT", "large, not consistent, covered with dense clouds.", 24000, 72000, false, true, 12, 0.55},
    {"ICY", "small, solid, icy surface, no atmosphere.", 900, 2600, true, false, 2, 0.70},
    {"QUARTZ", "medium size, surface is mainly native quartz, oxygen atmosphere.", 3800, 6200, true, true, 3, 0.80},
    {"OCEAN", "medium size, one global sea under a thick sky, ice floes toward the poles, no land.", 5000, 7800, true, true, 2, 0.30},
    {"METAL", "small, dense, an iron surface that glints in the sun, craterized, no atmosphere.", 1200, 3200, true, false, 1, 0.12},
    {"VOLCANIC", "small, tidally heated, sulphur plains and active cones, no atmosphere.", 1200, 2600, true, false, 0, 0.50},
    {"CARBON", "medium size, black graphite plains with crusts of diamond, no atmosphere.", 2500, 5000, true, false, 1, 0.06},
    {"SUBSTELLAR", "very large, a failed star glowing dull red with its own heat, dense banded clouds.", 60000, 95000, false, true, 8, 0.30},
    {"COMET", "tiny, a mountain of ice and dust on a long eccentric orbit, trailing a tail near the star.", 2, 12, true, false, 0, 0.06},
    {"COMPANION STAR", "a second sun bound to the primary, with worlds of its own.", 9000, 4600000, false, false, 4, 1.0},
    {"EUROPAN", "small, an ice shell over a hidden ocean, crossed by red cracks that vent water into space, no atmosphere.", 1200, 2600, true, false, 0, 0.65},
    {"TECTONIC", "medium size, seismically active: rift fissures, fountains of lava, quakes, a thin sulphurous sky.", 2400, 5200, true, true, 2, 0.28},
    {"DESERT", "medium size, dry, dune seas, mesas and salt pans under a dusty ochre sky.", 3000, 6400, true, true, 2, 0.40},
    {"HYDROCARBON", "medium size, cold, an opaque orange haze over seas of methane and dunes of tar sand.", 2200, 4400, true, true, 1, 0.22},
    {"BOMBARDED", "small, airless, under a rain of meteorites: fresh craters and bright rays.", 700, 2200, true, false, 0, 0.18},
    {"ACIDIC", "medium size, corrosive: seas of acid, bleached karst and sulphur crusts under a yellow-green sky.", 3800, 6600, true, true, 2, 0.55},
};

namespace {
constexpr double ORBIT_K = 8.0e-6;         // planet period = K * r^1.5 / sqrt(mass)
constexpr double MOON_K = 1.26e-3;

double equilibriumTemp(double luminosity, double orbitKm) {
    return 278.0 * std::pow(luminosity, 0.25) * std::sqrt(AU_GAME_KM / orbitKm);
}

int pickPlanetType(Rng& rng, double tempK, int starClass, bool isMoon, int parentType, double sizeRand) {
    double w[PT_COUNT] = {0};
    // R-307 (GEN 9): the six new types take their share of every zone (bombarded worlds near the star, tectonic and acidic
    // ones in the warm zones, deserts across the middle, hydrocarbon and europan worlds in the cold)
    if (tempK > 600) { w[PT_MOLTEN] = 40; w[PT_CRATERED] = 30; w[PT_VENUSIAN] = 18; w[PT_ROCKY] = 7; w[PT_METAL] = 12; w[PT_BOMBARDED] = 9; w[PT_TECTONIC] = 6; }
    else if (tempK > 345) { w[PT_VENUSIAN] = 26; w[PT_CRATERED] = 18; w[PT_ROCKY] = 18; w[PT_THINATMO] = 10; w[PT_MOLTEN] = 9; w[PT_QUARTZ] = 6; w[PT_METAL] = 7; w[PT_OCEAN] = 4; w[PT_TECTONIC] = 9; w[PT_DESERT] = 11; w[PT_ACIDIC] = 9; w[PT_BOMBARDED] = 5; }
    else if (tempK > 205) { w[PT_FELISIAN] = 48; w[PT_THINATMO] = 14; w[PT_ROCKY] = 9; w[PT_CRATERED] = 8; w[PT_QUARTZ] = 8; w[PT_VENUSIAN] = 5; w[PT_OCEAN] = 11; w[PT_DESERT] = 13; w[PT_ACIDIC] = 5; w[PT_TECTONIC] = 3; w[PT_BOMBARDED] = 3; }
    else if (tempK > 120) { w[PT_THINATMO] = 18; w[PT_ICY] = 18; w[PT_GASGIANT] = 28; w[PT_ROCKY] = 12; w[PT_CRATERED] = 10; w[PT_QUARTZ] = 5; w[PT_HYDROCARBON] = 11; w[PT_EUROPAN] = 12; w[PT_BOMBARDED] = 3; }
    else { w[PT_ICY] = 32; w[PT_GASGIANT] = 34; w[PT_CRATERED] = 16; w[PT_ROCKY] = 7; w[PT_CARBON] = 6; w[PT_EUROPAN] = 14; w[PT_HYDROCARBON] = 6; w[PT_BOMBARDED] = 4; }   // M9-16 tuning
    if (tempK <= 205) w[PT_SUBSTELLAR] = 4;   // M5-02: brown dwarfs live in the cold outer system
    if (starClass == STAR_WHITE_DWARF || starClass == STAR_PULSAR || starClass == STAR_NEUTRON || starClass == STAR_BLACK_HOLE) {   // S-06: a black hole's are the dead stars' and less
        w[PT_FELISIAN] = 0; w[PT_VENUSIAN] = 0; w[PT_QUARTZ] = 0; w[PT_OCEAN] = 0; w[PT_THINATMO] *= 0.3;
        w[PT_DESERT] *= 0.2; w[PT_ACIDIC] = 0; w[PT_HYDROCARBON] *= 0.3;   // R-307: the dead star stripped the thick airs
        w[PT_CRATERED] += 20; w[PT_ICY] += 15; w[PT_CARBON] += 8; w[PT_METAL] += 6; w[PT_BOMBARDED] += 6;
        if (starClass == STAR_PULSAR) { w[PT_GASGIANT] *= 0.3; w[PT_MOLTEN] *= 0.3; }
        if (starClass == STAR_NEUTRON) { w[PT_GASGIANT] = 0; w[PT_SUBSTELLAR] *= 0.3; w[PT_MOLTEN] *= 0.3; w[PT_HYDROCARBON] = 0; w[PT_EUROPAN] *= 0.5; w[PT_METAL] += 14; w[PT_BOMBARDED] += 8; w[PT_ROCKY] *= 1.3; }   // S-03: the survivors of the supernova: bare cores, captured rocks; no giant kept its envelope
        if (starClass == STAR_BLACK_HOLE) { w[PT_GASGIANT] = 0; w[PT_SUBSTELLAR] *= 0.5; w[PT_MOLTEN] = 0; w[PT_THINATMO] = 0; w[PT_DESERT] = 0; w[PT_HYDROCARBON] = 0; w[PT_EUROPAN] *= 0.5; w[PT_CRATERED] += 10; w[PT_ROCKY] *= 1.3; w[PT_METAL] += 10; w[PT_BOMBARDED] += 6; }   // S-06: wandering rocks and ice, nothing with an air
    }
    if (starClass == STAR_PROTOSTAR) {   // S-04: worlds still forming: molten and bombarded, no air or sea had time to come, no giant has finished gathering
        w[PT_FELISIAN] = 0; w[PT_VENUSIAN] = 0; w[PT_QUARTZ] = 0; w[PT_OCEAN] = 0; w[PT_ACIDIC] = 0; w[PT_DESERT] = 0; w[PT_HYDROCARBON] = 0; w[PT_EUROPAN] = 0;
        w[PT_THINATMO] *= 0.3; w[PT_GASGIANT] *= 0.2; w[PT_SUBSTELLAR] *= 0.5; w[PT_ICY] *= 0.6; w[PT_ROCKY] *= 1.2;
        w[PT_MOLTEN] += 25; w[PT_BOMBARDED] += 20; w[PT_TECTONIC] += 4;
    }
    if (starClass == STAR_RED_GIANT) { w[PT_FELISIAN] *= 0.25; w[PT_MOLTEN] *= 1.5; }
    if (starClass == STAR_BLUE_GIANT) { w[PT_FELISIAN] *= 0.5; }
    // S-01: the varieties' worlds
    if (starClass == STAR_BLUE_WHITE) { w[PT_FELISIAN] *= 0.6; w[PT_ROCKY] *= 1.4; w[PT_CRATERED] *= 1.3; w[PT_ICY] *= 1.3; w[PT_THINATMO] *= 1.2; }   // bare rock and ice under the ultraviolet
    if (starClass == STAR_ORANGE_GIANT) { w[PT_FELISIAN] *= 0.5; w[PT_OCEAN] *= 2.0; w[PT_EUROPAN] *= 1.5; w[PT_MOLTEN] *= 1.3; w[PT_ICY] *= 0.7; }   // the thawed ice worlds of an old star
    if (starClass == STAR_CARBON) { w[PT_FELISIAN] *= 0.2; w[PT_CARBON] = w[PT_CARBON] * 3 + 10; w[PT_METAL] *= 1.3; }   // the soot falls on every world
    if (isMoon) {
        // M9-16: moons draw from their own table: fewer identical icy moons, more rocky and thin-air ones
        w[PT_GASGIANT] = 0; w[PT_SUBSTELLAR] = 0; w[PT_OCEAN] *= 0.3;
        w[PT_VENUSIAN] *= 0.15;
        if (parentType == PT_SUBSTELLAR) parentType = PT_GASGIANT;   // M5-02: the same moon table as gas giants
        if (parentType != PT_GASGIANT) { w[PT_FELISIAN] *= 0.15; w[PT_QUARTZ] *= 0.3; } else { w[PT_FELISIAN] *= 0.6; w[PT_THINATMO] *= 1.4; w[PT_VOLCANIC] = 14; w[PT_EUROPAN] *= 2.0; w[PT_TECTONIC] += 6; }   // R-307: tides heat a giant's inner moons
        w[PT_CRATERED] *= 1.4; w[PT_ICY] *= 0.9; w[PT_ROCKY] *= 1.5; w[PT_MOLTEN] *= 0.7;
        w[PT_DESERT] *= 0.3; w[PT_ACIDIC] *= 0.3; w[PT_HYDROCARBON] *= 0.5; w[PT_BOMBARDED] *= 1.5;
    }
    if (starClass == STAR_WOLF_RAYET) {   // S-05: the wind stripped every atmosphere and sublimed the near ice; what is left is bare, cratered and bombarded
        // (after the moon rule, which offers a giant's moons a tectonic sky)
        for (int t = 0; t < PT_COUNT; t++) if (PLANET_TYPES[t].atmosphere && t != PT_GASGIANT && t != PT_SUBSTELLAR) w[t] = 0;
        w[PT_GASGIANT] *= 0.4; w[PT_SUBSTELLAR] *= 0.5; w[PT_EUROPAN] *= 0.3; w[PT_ICY] *= 0.5; w[PT_CARBON] *= 0.5;
        w[PT_BOMBARDED] += 25; w[PT_CRATERED] *= 1.5; w[PT_ROCKY] *= 1.3; w[PT_METAL] += 8; w[PT_MOLTEN] += 5;
    }
    if (sizeRand < 0.3) { w[PT_GASGIANT] *= 0.2; w[PT_SUBSTELLAR] *= 0.2; }
    return rng.pick(w, PT_COUNT);
}

RGB typeColor(int type, Rng& rng) {
    float h = (float)rng.uni();
    switch (type) {
        case PT_MOLTEN: return RGB(0.85f, 0.35f + 0.2f * h, 0.15f);
        case PT_CRATERED: return RGB(0.62f + 0.1f * h, 0.60f, 0.56f);
        case PT_VENUSIAN: return RGB(0.95f, 0.85f - 0.15f * h, 0.55f + 0.2f * h);
        case PT_FELISIAN: return RGB(0.30f + 0.15f * h, 0.62f, 0.55f + 0.25f * (1 - h));
        case PT_ROCKY: return RGB(0.55f + 0.1f * h, 0.47f, 0.40f);
        case PT_THINATMO: {
            int k = rng.irange(4);
            if (k == 0) return RGB(0.85f, 0.45f, 0.30f);          // red/orange
            if (k == 1) return RGB(0.55f, 0.75f, 0.45f);          // green
            if (k == 2) return RGB(0.65f, 0.50f, 0.80f);          // purple
            return RGB(0.60f, 0.62f, 0.66f);                      // grey
        }
        case PT_GASGIANT: {
            int k = rng.irange(4);
            if (k == 0) return RGB(0.92f, 0.78f, 0.55f);
            if (k == 1) return RGB(0.55f, 0.65f, 0.90f);
            if (k == 2) return RGB(0.85f, 0.60f, 0.45f);
            return RGB(0.70f, 0.80f, 0.85f);
        }
        case PT_ICY: return RGB(0.80f, 0.88f, 1.00f);
        case PT_QUARTZ: return RGB(0.95f, 0.85f + 0.1f * h, 0.90f);
        case PT_OCEAN: return RGB(0.25f, 0.45f + 0.1f * h, 0.80f);
        case PT_METAL: return RGB(0.45f, 0.42f, 0.40f + 0.05f * h);
        case PT_VOLCANIC: return RGB(0.85f, 0.75f + 0.1f * h, 0.30f);
        case PT_CARBON: return RGB(0.18f, 0.17f, 0.17f);
        case PT_SUBSTELLAR: return RGB(0.62f, 0.16f + 0.08f * h, 0.06f);
        case PT_COMET: return RGB(0.75f, 0.8f, 0.85f);
        case PT_EUROPAN: return RGB(0.78f, 0.74f - 0.06f * h, 0.66f);
        case PT_TECTONIC: return RGB(0.36f, 0.28f, 0.22f + 0.05f * h);
        case PT_DESERT: return RGB(0.80f, 0.62f - 0.08f * h, 0.38f);
        case PT_HYDROCARBON: return RGB(0.75f, 0.50f, 0.20f + 0.08f * h);
        case PT_BOMBARDED: return RGB(0.40f, 0.38f, 0.36f + 0.05f * h);
        case PT_ACIDIC: return RGB(0.78f, 0.78f, 0.45f + 0.1f * h);
    }
    return RGB(0.6f, 0.6f, 0.6f);
}
}

void StarSystem::generate(const Star& s) {
    star = s;
    bodies.clear();
    belts.clear();
    companion = -1;
    valid = s.valid;
    if (!valid) return;
    const StarClassInfo& ci = STAR_CLASSES[s.cls];
    Rng rng(s.seed ^ 0x51A7E11ULL);
    int np = (rng.irange(ci.maxPlanets + 1) + rng.irange(ci.maxPlanets + 1) + 1) / 2;
    if (s.cls == STAR_PULSAR && rng.chance(0.3)) np = 0;
    if (s.cls == STAR_NEUTRON && rng.chance(0.25)) np = 0;   // S-03: a quarter of the neutron stars keep nothing but debris
    if (s.cls == STAR_PROTOSTAR && rng.chance(0.35)) np = 0;   // S-04: a third of the protostars have no world yet, only the disc
    if (s.cls == STAR_WOLF_RAYET && rng.chance(0.4)) np = 0;   // S-05: two Wolf-Rayet systems in five are rubble
    if (s.cls == STAR_BLACK_HOLE && rng.chance(0.5)) np = 0;   // S-06: half the black holes keep nothing but rubble
    // orbit radii: Kepler-like accumulation as in the original, limited beyond the 8th orbit
    double key = std::max(s.radiusKm * ci.firstOrbitMult, ci.minFirstOrbitKm) * (0.8 + 0.5 * rng.uni());
    // S-05: a Wolf-Rayet star keeps, two times in five, one giant far out beyond its bare worlds (its zones within four orbits are
    // too hot for one to form), the last body at 2.5-3.5 times the next orbit's key: its envelope streams away in the wind
    int farGiant = (s.cls == STAR_WOLF_RAYET && rng.chance(0.45)) ? np : -1;
    if (farGiant >= 0) np++;
    for (int n = 0; n < np; n++) {
        Body b;
        b.index = (int)bodies.size();
        b.parent = -1;
        b.seed = hashCombine(s.seed, 1000 + n);
        if (n == farGiant) key *= 2.5 + rng.uni();
        b.orbitRadiusKm = key * (1.0 + rng.sym(0.08));
        b.tempK = equilibriumTemp(s.luminosity, b.orbitRadiusKm);
        double sizeRand = rng.uni();
        b.type = n == farGiant ? PT_GASGIANT : pickPlanetType(rng, b.tempK, s.cls, false, -1, sizeRand);
        const PlanetTypeInfo& pt = PLANET_TYPES[b.type];
        b.radiusKm = pt.minRadiusKm + (pt.maxRadiusKm - pt.minRadiusKm) * sizeRand;
        double densityRel = b.type == PT_GASGIANT ? 0.25 : ((b.type == PT_ICY || b.type == PT_EUROPAN) ? 0.55 : (b.type == PT_SUBSTELLAR ? 0.9 : 1.0));
        if (b.type == PT_SUBSTELLAR) b.luminosity = 0.0008 + 0.004 * rng.uni();   // M5-02: it warms its moons and glows at night
        b.massEarths = std::pow(b.radiusKm / 6371.0, 3) * densityRel;
        b.gravity = 9.8 * b.massEarths / std::pow(b.radiusKm / 6371.0, 2);
        b.orbitPeriod = ORBIT_K * std::pow(b.orbitRadiusKm, 1.5) / std::sqrt(s.massFactor);
        b.orbitPhase0 = rng.range(0, TAU);
        b.orbitIncl = rng.sym(4 * DEG) * (1 + rng.uni());
        b.orbitNode = rng.range(0, TAU);
        // rotation: inner planets tend to be tidally locked (S-01: a red dwarf's first three nearly always; their orbits are rounded below)
        bool lockedNear = s.cls == STAR_RED_DWARF ? (n < 3 && rng.chance(0.9)) : (n == 0 && rng.chance(0.5));
        if (lockedNear) b.rotPeriod = b.orbitPeriod;
        else b.rotPeriod = 1200 + 12000 * rng.uni() * rng.uni();
        if (rng.chance(0.08) && !(lockedNear && s.cls == STAR_RED_DWARF)) b.rotPeriod = -b.rotPeriod;   // retrograde (S-01: not a red dwarf's locked world)
        b.rotPhase0 = rng.range(0, TAU);
        double tiltRoll = rng.uni();
        b.axialTilt = tiltRoll < 0.6 ? rng.range(0, 12 * DEG) : (tiltRoll < 0.93 ? rng.range(12 * DEG, 35 * DEG) : rng.range(35 * DEG, 95 * DEG));
        b.axisAzimuth = rng.range(0, TAU);
        b.rings = (b.type == PT_GASGIANT) ? rng.chance(0.6) : rng.chance(0.12);   // GEN 5 (O0-01): one solid world in eight wears rings
        b.ringInner = 1.35 + 0.6 * rng.uni();
        b.ringOuter = b.ringInner + 0.5 + 1.6 * rng.uni();
        b.color = typeColor(b.type, rng);
        {   // M5-03 eccentricity from its own stream: most orbits are nearly round, a few are clearly oval
            Rng er(b.seed ^ 0x0ECCULL);
            b.ecc = er.chance(0.6) ? er.range(0, 0.06) : (er.chance(0.8) ? er.range(0.06, 0.15) : er.range(0.15, 0.3));
            b.argPeri = er.range(0, TAU);
        }
        if (s.cls == STAR_RED_DWARF && n < 3) b.ecc *= 0.3;   // S-01: the tides of a red dwarf round the near orbits, so the locked face holds (M5-08 asks for e <= 0.05)
        b.name = s.name + " " + romanNumeral(n + 1);
        bodies.push_back(b);
        double next = b.orbitRadiusKm * (1.32 + 0.45 * rng.uni());   // M9-16: tighter spacing, more worlds in the temperate band
        if (b.type == PT_GASGIANT || b.type == PT_SUBSTELLAR) next *= 1.25;
        key = n < 8 ? next : b.orbitRadiusKm * (1.18 + 0.12 * rng.uni());
    }
    if (s.cls == STAR_NEUTRON) {   // S-03: the blast fused the surfaces of the outermost one or two survivors (the near ones were vaporised; ice took none)
        int want = unitFromHash(mix64(s.seed ^ 0x61A55ULL)) < 0.5 ? 2 : 1, marked = 0;
        for (int n = (int)bodies.size() - 1; n >= 0 && marked < want; n--) {
            int ty = bodies[n].type;
            if (ty == PT_CRATERED || ty == PT_ROCKY || ty == PT_METAL || ty == PT_BOMBARDED || ty == PT_THINATMO || ty == PT_DESERT || ty == PT_CARBON) { bodies[n].glassed = true; marked++; }
        }
    }
    // M9-12 asteroid belts in wide gaps (and sometimes beyond the last planet); S-03: a neutron star's system is mostly debris;
    // S-04: a protostar's is the disc itself: a belt inside the first orbit, one in nearly every gap and one beyond
    if (s.cls == STAR_PROTOSTAR && !bodies.empty() && rng.chance(0.7)) {
        Belt bl;
        bl.innerKm = std::max(bodies[0].orbitRadiusKm * 0.45, s.radiusKm * 3.0); bl.outerKm = bodies[0].orbitRadiusKm * 0.8;
        double mid = 0.5 * (bl.innerKm + bl.outerKm);
        bl.period = ORBIT_K * std::pow(mid, 1.5) / std::sqrt(s.massFactor);
        bl.phase0 = rng.range(0, TAU);
        bl.name = s.name + " belt " + romanNumeral((int)belts.size() + 1);
        belts.push_back(bl);
    }
    for (int n = 0; n + 1 < (int)bodies.size(); n++) {
        double r0 = bodies[n].orbitRadiusKm, r1 = bodies[n + 1].orbitRadiusKm;
        if ((r1 / r0 > 1.65 && rng.chance(0.55)) || rng.chance(s.cls == STAR_NEUTRON || s.cls == STAR_BLACK_HOLE ? 0.5 : (s.cls == STAR_PROTOSTAR ? 0.9 : (s.cls == STAR_WOLF_RAYET ? 0.35 : 0.12)))) {   // S-05: the rubble of the stripped worlds; S-06: a black hole's like a neutron star's
            Belt bl;
            bl.innerKm = r0 * 1.22; bl.outerKm = r1 * 0.82;
            double mid = 0.5 * (bl.innerKm + bl.outerKm);
            bl.period = ORBIT_K * std::pow(mid, 1.5) / std::sqrt(s.massFactor);
            bl.phase0 = rng.range(0, TAU);
            bl.name = s.name + " belt " + romanNumeral((int)belts.size() + 1);
            belts.push_back(bl);
        }
    }
    if ((!bodies.empty() || s.cls == STAR_NEUTRON || s.cls == STAR_PROTOSTAR || s.cls == STAR_WOLF_RAYET || s.cls == STAR_BLACK_HOLE) && rng.chance(s.cls == STAR_NEUTRON || s.cls == STAR_BLACK_HOLE ? 0.75 : (s.cls == STAR_PROTOSTAR ? 0.95 : (s.cls == STAR_WOLF_RAYET ? 0.5 : 0.3)))) {
        Belt bl;
        double last = bodies.empty() ? ci.minFirstOrbitKm * 2.0 : bodies.back().orbitRadiusKm;   // S-03: a neutron star without worlds keeps its debris; S-04: a protostar's disc
        bl.innerKm = last * 1.5; bl.outerKm = last * 2.2;
        double mid = 0.5 * (bl.innerKm + bl.outerKm);
        bl.period = ORBIT_K * std::pow(mid, 1.5) / std::sqrt(s.massFactor);
        bl.phase0 = rng.range(0, TAU);
        bl.name = s.name + " belt " + romanNumeral((int)belts.size() + 1);
        belts.push_back(bl);
    }
    // moons
    int npl = (int)bodies.size();
    for (int n = 0; n < npl; n++) {
        Body p = bodies[n];   // copy: push_back below would invalidate a reference
        const PlanetTypeInfo& pt = PLANET_TYPES[p.type];
        int maxm = pt.maxMoons;
        bool giant = p.type == PT_GASGIANT || p.type == PT_SUBSTELLAR;
        int nm = maxm > 0 ? (rng.irange(maxm + 1) + rng.irange(maxm + 1)) / 2 : 0;
        if (giant) nm = 2 + rng.irange(maxm - 1);
        if (n == 0 && !giant) nm = std::min(nm, 1);
        if ((int)bodies.size() + nm > 60) nm = 60 - (int)bodies.size();
        // M5-09: about 7% of the solid mid-sized planets are double planets: a twin of 0.55-0.95 radii at 5-9 radii, both locked to each other
        bool twin = (p.type == PT_ROCKY || p.type == PT_FELISIAN || p.type == PT_VENUSIAN || p.type == PT_QUARTZ || p.type == PT_OCEAN || p.type == PT_THINATMO || p.type == PT_DESERT || p.type == PT_ACIDIC || p.type == PT_TECTONIC) && rng.chance(0.07) && (int)bodies.size() < 59;
        if (twin) nm = std::max(nm, 1);
        p.moonCount = nm;
        bodies[n].moonCount = nm;
        // B-314 (GEN 7): the first moon orbits at 4.5-6 radii, and outside the rings by a third of their outer radius. From
        // 2.8 radii the parent's disc was 42 degrees across (60% of the view), and a moon inside the rings (they reach
        // 4 radii) saw them fill the sky; from 4.5 radii the disc is 26 degrees, a wall of a planet still, and the rings
        // a band (M5-09 had 2.8-4.2 radii; 3.5-6.5 before that). The same random draw, so the rest of the system holds
        double mkey = p.radiusKm * std::max(4.5 + 1.5 * rng.uni(), p.rings ? p.ringOuter * 1.35 : 0.0);
        double prevMoonR = 0;
        for (int c = 0; c < nm; c++) {
            if (c == 0 && twin) {
                Body m;
                m.index = (int)bodies.size();
                m.parent = n;
                m.seed = hashCombine(p.seed, 500);
                m.orbitRadiusKm = p.radiusKm * (5.0 + 4.0 * rng.uni());
                m.tempK = p.tempK * (0.95 + 0.1 * rng.uni());
                m.type = pickPlanetType(rng, m.tempK, s.cls, false, -1, 0.5);
                if (m.type == PT_GASGIANT || m.type == PT_SUBSTELLAR) m.type = PT_ROCKY;
                m.radiusKm = p.radiusKm * (0.55 + 0.4 * rng.uni());
                double densityRel = (m.type == PT_ICY || m.type == PT_EUROPAN) ? 0.55 : 1.0;
                m.massEarths = std::pow(m.radiusKm / 6371.0, 3) * densityRel;
                m.gravity = 9.8 * m.massEarths / std::pow(m.radiusKm / 6371.0, 2);
                double pmass = std::pow(p.radiusKm / 6371.0, 2.5) + std::pow(m.radiusKm / 6371.0, 2.5);
                m.orbitPeriod = MOON_K * std::pow(m.orbitRadiusKm, 1.5) / std::sqrt(std::max(0.05, pmass));
                m.orbitPhase0 = rng.range(0, TAU);
                m.orbitIncl = rng.sym(3 * DEG);
                m.orbitNode = rng.range(0, TAU);
                m.rotPeriod = m.orbitPeriod;
                bodies[n].rotPeriod = m.orbitPeriod;   // the planet too: they always show each other the same face
                m.rotPhase0 = rng.range(0, TAU);
                m.axialTilt = rng.range(0, 6 * DEG);
                m.axisAzimuth = rng.range(0, TAU);
                m.color = typeColor(m.type, rng);
                m.name = p.name + "-" + (char)('a' + c);
                m.doublePlanet = true;
                bodies[n].doublePlanet = true;
                bodies.push_back(m);
                prevMoonR = m.orbitRadiusKm;
                mkey = m.orbitRadiusKm * (2.2 + 0.8 * rng.uni());
                continue;
            }
            Body m;
            m.index = (int)bodies.size();
            m.parent = n;
            m.seed = hashCombine(p.seed, 500 + c);
            m.orbitRadiusKm = mkey * (1.0 + rng.sym(0.05));
            // M9-12 resonances: every other moon locks its period to the previous one (2:1 or 3:2)
            if (c >= 1 && rng.chance(0.45)) m.orbitRadiusKm = std::max(prevMoonR * 1.2, prevMoonR * std::pow(rng.chance(0.5) ? 2.0 : 1.5, 2.0 / 3.0));
            prevMoonR = m.orbitRadiusKm;
            m.tempK = p.tempK * (0.9 + 0.1 * rng.uni());
            if (giant && c < 2) m.tempK += 60.0 * (2 - c);   // M9-12 tidal heating of the inner moons
            if (p.type == PT_SUBSTELLAR) m.tempK = std::pow(std::pow(m.tempK, 4) + std::pow(equilibriumTemp(p.luminosity, m.orbitRadiusKm), 4), 0.25);   // M5-02: warmed by the parent's glow
            double sizeRand = rng.uni() * rng.uni();
            if (c == 0 && !giant && rng.chance(0.5)) sizeRand = 0.5 + 0.5 * rng.uni();   // M5-09: half the first moons are big
            m.type = pickPlanetType(rng, m.tempK, s.cls, true, p.type, sizeRand);
            if ((m.type == PT_VOLCANIC || m.type == PT_TECTONIC) && c >= 2) m.type = PT_ICY;   // only the inner moons are heated enough (R-307: tectonic moons too)
            const PlanetTypeInfo& mt = PLANET_TYPES[m.type];
            double maxR = giant ? 6500.0 : p.radiusKm * 0.5;   // M5-09: the largest moon may reach half the planet's radius (0.42 before)
            m.radiusKm = std::min(maxR, mt.minRadiusKm + (mt.maxRadiusKm - mt.minRadiusKm) * sizeRand);
            if (m.radiusKm < 600) m.radiusKm = 600 + 400 * rng.uni();
            double densityRel = (m.type == PT_ICY || m.type == PT_EUROPAN) ? 0.55 : 1.0;
            m.massEarths = std::pow(m.radiusKm / 6371.0, 3) * densityRel;
            m.gravity = 9.8 * m.massEarths / std::pow(m.radiusKm / 6371.0, 2);
            double pmass = std::pow(p.radiusKm / 6371.0, 2.5) * (giant ? 0.6 : 1.0);
            m.orbitPeriod = MOON_K * std::pow(m.orbitRadiusKm, 1.5) / std::sqrt(std::max(0.05, pmass));
            m.orbitPhase0 = rng.range(0, TAU);
            m.orbitIncl = rng.sym(6 * DEG);
            { Rng er(m.seed ^ 0x0ECCULL); m.ecc = er.range(0, 0.04); m.argPeri = er.range(0, TAU); }   // M5-03
            m.orbitNode = rng.range(0, TAU);
            m.rotPeriod = rng.chance(0.7) ? m.orbitPeriod : 900 + 7000 * rng.uni();
            m.rotPhase0 = rng.range(0, TAU);
            m.axialTilt = rng.range(0, 8 * DEG);
            m.axisAzimuth = rng.range(0, TAU);
            m.rings = false;
            m.color = typeColor(m.type, rng);
            m.name = p.name + "-" + (char)('a' + c);
            bodies.push_back(m);
            double grow = c < 2 ? (giant ? 1.35 + 0.5 * rng.uni() : 1.6 + 0.8 * rng.uni()) : (1.25 + 0.3 * rng.uni());   // M5-09: giants' inner moons closer together
            mkey = m.orbitRadiusKm * grow;
        }
        // M9-12 captured moons: gas giants sometimes hold a small body far out on a tilted, retrograde orbit
        if (giant && rng.chance(0.18) && (int)bodies.size() < 60) {
            Body m;
            m.index = (int)bodies.size();
            m.parent = n;
            m.seed = hashCombine(p.seed, 590);
            m.orbitRadiusKm = p.radiusKm * (20 + 20 * rng.uni());
            m.tempK = p.tempK;
            m.type = rng.chance(0.5) ? PT_CRATERED : PT_ICY;
            m.radiusKm = 300 + 600 * rng.uni();
            double densityRel = (m.type == PT_ICY || m.type == PT_EUROPAN) ? 0.55 : 1.0;
            m.massEarths = std::pow(m.radiusKm / 6371.0, 3) * densityRel;
            m.gravity = 9.8 * m.massEarths / std::pow(m.radiusKm / 6371.0, 2);
            double pmass = std::pow(p.radiusKm / 6371.0, 2.5) * 0.6;
            m.orbitPeriod = -MOON_K * std::pow(m.orbitRadiusKm, 1.5) / std::sqrt(std::max(0.05, pmass));   // retrograde
            m.orbitPhase0 = rng.range(0, TAU);
            m.orbitIncl = rng.range(30 * DEG, 60 * DEG);
            m.orbitNode = rng.range(0, TAU);
            m.rotPeriod = 900 + 7000 * rng.uni();
            m.rotPhase0 = rng.range(0, TAU);
            m.axialTilt = rng.range(0, 40 * DEG);
            m.axisAzimuth = rng.range(0, TAU);
            m.color = typeColor(m.type, rng);
            m.name = p.name + "-" + (char)('a' + nm);
            { Rng er(m.seed ^ 0x0ECCULL); m.ecc = er.range(0.1, 0.4); m.argPeri = er.range(0, TAU); }   // M5-03: captured orbits are oval
            bodies.push_back(m);
            bodies[n].moonCount++;
        }
    }
    // M5-07 comets: one or two per system on long, oval, tilted orbits that dive in past the planets
    {
        Rng cr(s.seed ^ 0xC0E7ULL);
        int nc = cr.chance(0.45) ? 1 + cr.irange(2) : 0;
        double outer = bodies.empty() ? ci.minFirstOrbitKm * 12 : bodies.back().orbitRadiusKm;
        for (int k = 0; k < nc && (int)bodies.size() < 60; k++) {
            Body c;
            c.index = (int)bodies.size();
            c.parent = -1;
            c.type = PT_COMET;
            c.seed = hashCombine(s.seed, 700 + k);
            c.radiusKm = 2.0 + 10.0 * cr.uni();
            c.massEarths = std::pow(c.radiusKm / 6371.0, 3) * 0.3;
            c.gravity = 9.8 * c.massEarths / std::pow(c.radiusKm / 6371.0, 2);
            c.ecc = 0.6 + 0.33 * cr.uni();
            c.orbitRadiusKm = std::max(outer * (0.35 + 1.2 * cr.uni()), s.radiusKm * 4.0 / (1 - c.ecc));
            c.orbitPeriod = ORBIT_K * std::pow(c.orbitRadiusKm, 1.5) / std::sqrt(s.massFactor);
            c.orbitPhase0 = cr.range(0, TAU);
            c.argPeri = cr.range(0, TAU);
            c.orbitIncl = cr.sym(40 * DEG);
            c.orbitNode = cr.range(0, TAU);
            c.rotPeriod = 3600 * (3 + 27 * cr.uni());
            c.rotPhase0 = cr.range(0, TAU);
            c.axialTilt = cr.range(0, 60 * DEG);
            c.axisAzimuth = cr.range(0, TAU);
            c.tempK = equilibriumTemp(s.luminosity, c.orbitRadiusKm);
            c.color = typeColor(PT_COMET, cr);
            c.name = s.name + " comet " + romanNumeral(k + 1);
            bodies.push_back(c);
        }
    }
    // M5-01 companion stars: a second sun as a body, close in (the planets circle both) or far out with worlds of its own
    {
        Rng kr(s.seed ^ 0xB1A2ULL);
        static const double pMultiple[STAR_CLASS_COUNT] = {0.18, 0.2, 0.28, 0.12, 0.24, 0.15, 0.22, 0.26, 0.14, 0.1, 0.2, 0.35, 0.4, 0.5};   // S-01: the varieties' chances; S-03: the neutron star's; S-04: young stars come in pairs; S-05: most Wolf-Rayet stars have one; S-06: half the black holes drink a companion
        if (kr.chance(pMultiple[s.cls]) && (int)bodies.size() < 56) {
            double w[STAR_CLASS_COUNT] = {1.0, 1.6, (s.cls == STAR_BLUE_GIANT || s.cls == STAR_BLUE_WHITE || s.cls == STAR_WOLF_RAYET) ? 0.6 : 0.0, s.cls == STAR_BLACK_HOLE ? 0.0 : 0.15, 0.9, 0.05};   // S-06: a black hole's companion is close, never a giant   // the families; a variety takes its share below
            int cls = starVariety(kr.pick(w, STAR_CLASS_COUNT), galaxyRegion(s.sx, s.sy, s.sz), hashCombine(s.seed, 901));   // S-01
            const StarClassInfo& cc = STAR_CLASSES[cls];
            Body k;
            k.index = (int)bodies.size();
            k.parent = -1;
            k.type = PT_COMPANION;
            k.starClass = cls;
            k.seed = hashCombine(s.seed, 900);
            k.radiusKm = cc.radiusKm * (1 + kr.sym(cc.radiusVar));
            k.luminosity = cc.luminosity * std::pow(k.radiusKm / cc.radiusKm, 2.0) * (0.7 + 0.6 * kr.uni());
            k.massEarths = cc.massFactor * 333000.0;
            k.gravity = 274.0 * cc.massFactor / std::pow(k.radiusKm / 7.0e5, 2);
            k.color = cc.color;
            k.tempK = 5000;
            double firstOrbit = bodies.empty() || bodies[0].type == PT_COMET ? ci.minFirstOrbitKm : bodies[0].orbitRadiusKm;
            double closeR = (s.radiusKm + k.radiusKm) * (3.0 + 3.0 * kr.uni());
            bool close = (kr.chance(0.35) && closeR < 0.4 * firstOrbit) || s.cls == STAR_BLACK_HOLE;   // S-06: a black hole's companion is close, being drawn out
            k.orbitRadiusKm = close ? closeR : outerOrbitKm() * (2.5 + 5.0 * kr.uni());
            k.orbitPeriod = ORBIT_K * std::pow(k.orbitRadiusKm, 1.5) / std::sqrt(s.massFactor + cc.massFactor);
            k.orbitPhase0 = kr.range(0, TAU);
            k.orbitIncl = kr.sym(close ? 3 * DEG : 25 * DEG);
            k.orbitNode = kr.range(0, TAU);
            { Rng er(k.seed ^ 0x0ECCULL); k.ecc = close ? er.range(0, 0.1) : er.range(0.05, 0.45); k.argPeri = er.range(0, TAU); }
            k.rotPeriod = 3600 * 24 * (5 + 20 * kr.uni());
            k.axialTilt = kr.range(0, 10 * DEG);
            k.axisAzimuth = kr.range(0, TAU);
            k.name = s.name + " B";
            companion = k.index;
            // its own planets when it orbits far out: up to three, lit by both suns
            int np2 = close ? 0 : kr.irange(4);
            if ((int)bodies.size() + np2 > 60) np2 = 0;
            k.moonCount = np2;
            bodies.push_back(k);
            double key2 = std::max(k.radiusKm * cc.firstOrbitMult, cc.minFirstOrbitKm) * (0.8 + 0.5 * kr.uni());
            for (int n = 0; n < np2; n++) {
                Body b;
                b.index = (int)bodies.size();
                b.parent = companion;
                b.seed = hashCombine(k.seed, 1000 + n);
                b.orbitRadiusKm = key2 * (1.0 + kr.sym(0.08));
                double tA = equilibriumTemp(k.luminosity, b.orbitRadiusKm), tB = equilibriumTemp(s.luminosity, k.orbitRadiusKm);
                b.tempK = std::pow(std::pow(tA, 4) + std::pow(tB, 4), 0.25);
                double sizeRand = kr.uni();
                b.type = pickPlanetType(kr, b.tempK, cls, false, -1, sizeRand);
                if (b.type == PT_SUBSTELLAR) b.type = PT_GASGIANT;
                const PlanetTypeInfo& pt = PLANET_TYPES[b.type];
                b.radiusKm = pt.minRadiusKm + (pt.maxRadiusKm - pt.minRadiusKm) * sizeRand;
                double densityRel = b.type == PT_GASGIANT ? 0.25 : ((b.type == PT_ICY || b.type == PT_EUROPAN) ? 0.55 : 1.0);
                b.massEarths = std::pow(b.radiusKm / 6371.0, 3) * densityRel;
                b.gravity = 9.8 * b.massEarths / std::pow(b.radiusKm / 6371.0, 2);
                b.orbitPeriod = ORBIT_K * std::pow(b.orbitRadiusKm, 1.5) / std::sqrt(cc.massFactor);
                b.orbitPhase0 = kr.range(0, TAU);
                b.orbitIncl = kr.sym(4 * DEG);
                b.orbitNode = kr.range(0, TAU);
                b.rotPeriod = (n == 0 && kr.chance(0.5)) ? b.orbitPeriod : 1200 + 12000 * kr.uni() * kr.uni();
                b.rotPhase0 = kr.range(0, TAU);
                b.axialTilt = kr.range(0, 30 * DEG);
                b.axisAzimuth = kr.range(0, TAU);
                b.rings = b.type == PT_GASGIANT && kr.chance(0.5);
                b.ringInner = 1.35 + 0.6 * kr.uni();
                b.ringOuter = b.ringInner + 0.5 + 1.6 * kr.uni();
                b.color = typeColor(b.type, kr);
                { Rng er(b.seed ^ 0x0ECCULL); b.ecc = er.range(0, 0.1); b.argPeri = er.range(0, TAU); }
                b.name = s.name + " B " + romanNumeral(n + 1);
                bodies.push_back(b);
                key2 = b.orbitRadiusKm * (1.4 + 0.5 * kr.uni());
            }
        }
    }
    // spin frames
    for (Body& b : bodies) {
        Vec3 up(0, 1, 0);
        Vec3 tiltDir(std::cos(b.axisAzimuth), 0, std::sin(b.axisAzimuth));
        b.spinAxis = normalize(up * std::cos(b.axialTilt) + tiltDir * std::sin(b.axialTilt));
        Vec3 any = std::fabs(b.spinAxis.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        b.ref0 = normalize(cross(any, b.spinAxis));
        b.ref1 = cross(b.spinAxis, b.ref0);
    }
    // M5-08: planets whose day equals their year keep one face toward their sun; remember that face
    for (int i = 0; i < (int)bodies.size(); i++) {
        Body& b = bodies[i];
        if (b.parent >= 0 || b.type == PT_COMET || b.type == PT_COMPANION || b.ecc > 0.05) continue;
        if (std::fabs(1.0 - b.rotPeriod / b.orbitPeriod) > 0.02) continue;
        b.locked = true;
        b.lockedDir = bodyFrame(i, 0) * normalize(star.pos - bodyPos(i, 0));
    }
}

// O3: belt rocks. The belt is cut into rings of BELT_CELL_KM in radius; each ring turns as one at the Kepler rate of its
// middle radius (so rings shear against each other, the inner ones outrunning the outer, but a ring keeps its
// formation), and is cut into arc cells of BELT_CELL_KM round the star and height cells. A rock is a hash of its cell,
// so it keeps its identity at any time.
void StarSystem::beltCellOf(int k, const Vec3& posKm, double t, int64_t& ir, int64_t& ia, int64_t& iy) const {
    Vec3 rel = posKm - star.pos;
    double r = std::sqrt(rel.x * rel.x + rel.z * rel.z);
    ir = (int64_t)std::floor(r / BELT_CELL_KM);
    ia = beltArcCell(k, ir, std::atan2(rel.z, rel.x), t);
    iy = (int64_t)std::floor(rel.y / BELT_CELL_KM);
}

int64_t StarSystem::beltArcCell(int k, int64_t ir, double angWorld, double t) const {
    const Belt& bl = belts[k];
    double rRing = (ir + 0.5) * BELT_CELL_KM;
    int64_t n = std::max<int64_t>(1, (int64_t)std::llround(TAU * rRing / BELT_CELL_KM));
    double a = wrap2pi(angWorld - bl.rateAt(rRing) * t);   // the co-rotating angle of this ring
    int64_t ia = (int64_t)std::floor(a * rRing / BELT_CELL_KM);
    return ((ia % n) + n) % n;
}

bool StarSystem::beltRockAt(int k, int64_t ir, int64_t ia, int64_t iy, int m, double t, BeltRock& out) const {
    if (k < 0 || k >= (int)belts.size() || m < 0 || m >= BELT_ROCKS_PER_CELL) return false;
    const Belt& bl = belts[k];
    double r0 = ir * BELT_CELL_KM;
    if (r0 + BELT_CELL_KM < bl.innerKm || r0 > bl.outerKm) return false;
    double rRing = r0 + 0.5 * BELT_CELL_KM;
    int64_t n = std::max<int64_t>(1, (int64_t)std::llround(TAU * rRing / BELT_CELL_KM));
    ia = ((ia % n) + n) % n;
    uint64_t h = hash3i(ir, ia, iy, star.seed ^ (0xBE17ULL + (uint64_t)k * 0x9E37ULL));
    double thick = 0.015 * (r0 + 0.5 * BELT_CELL_KM);   // the belt is thin: |y| within 1.5% of the radius
    if (std::fabs((iy + 0.5) * BELT_CELL_KM) > thick + BELT_CELL_KM) return false;
    int nr = 1 + (int)(unitFromHash(h) * BELT_ROCKS_PER_CELL);   // 1..3 rocks
    double edge = smoothstep(0.0, 0.12, (r0 - bl.innerKm) / (bl.outerKm - bl.innerKm)) * smoothstep(1.0, 0.88, (r0 - bl.innerKm) / (bl.outerKm - bl.innerKm));
    if (unitFromHash(mix64(h + 99)) > 0.35 + 0.65 * edge) nr = std::max(1, nr - 2);   // sparser toward the edges
    if (m >= nr) return false;
    uint64_t hr = mix64(h + 17 * (m + 1));
    double u0 = unitFromHash(hr), u1 = unitFromHash(mix64(hr + 1)), u2 = unitFromHash(mix64(hr + 2)), u3 = unitFromHash(mix64(hr + 3));
    double r = r0 + u0 * BELT_CELL_KM;
    double y = (iy + u2) * BELT_CELL_KM;
    if (std::fabs(y) > thick) return false;
    double ang = (ia + u1) * BELT_CELL_KM / rRing + bl.rateAt(rRing) * t;   // the ring's own rate: the cell mapping stays exact
    out.pos = star.pos + Vec3(std::cos(ang) * r, y, std::sin(ang) * r);
    out.radiusKm = 0.25 + 12.0 * std::pow(u3, 4.0) + (u3 > 0.9995 ? 25.0 : 0.0);   // most under a kilometre, the biggest in reach about 10 km, one in two thousand a 35 km mountain
    out.seed = hr;
    double a1 = unitFromHash(mix64(hr + 4)) * TAU, a2 = unitFromHash(mix64(hr + 5)) * PI;
    out.axis = Vec3(std::cos(a1) * std::sin(a2), std::cos(a2), std::sin(a1) * std::sin(a2));
    out.tumble = unitFromHash(mix64(hr + 6)) * TAU + t * TAU / (1800.0 + 7200.0 * unitFromHash(mix64(hr + 7)));   // one turn in half an hour to two and a half
    out.ir = ir; out.ia = ia; out.iy = iy; out.m = m;
    return true;
}

// O0-01 (M5-04): the ring profile, hashed per body, with gaps carved by shepherd moons inside the ring
std::vector<float> StarSystem::ringProfileOf(int bi) const {
    const Body& b = bodies[bi];
    std::vector<float> prof(256);
    Rng rng(b.seed ^ 0x51465ULL);
    uint64_t sd = rng.next();
    for (int i = 0; i < 256; i++) {
        double x = i / 256.0;
        double n = 0.5 + 0.5 * fbm2(x * 9.0, 0.37, sd, 4);
        double gaps = std::pow(0.5 + 0.5 * std::sin(x * 40.0 + 3.0 * gnoise2(x * 5, 1.5, sd + 9)), 6.0);
        double v = clampd(n * (1.0 - 0.7 * gaps), 0, 1);
        v *= smoothstep(0.0, 0.06, x) * smoothstep(1.0, 0.85, x);
        for (int j = 0; j < (int)bodies.size(); j++) {
            const Body& mo = bodies[j];
            if (mo.parent != bi) continue;
            double rr = mo.orbitRadiusKm / b.radiusKm;
            if (rr < b.ringInner || rr > b.ringOuter) continue;
            double u = (rr - b.ringInner) / (b.ringOuter - b.ringInner);
            v *= 1 - 0.92 * std::exp(-((x - u) * (x - u)) / (0.018 * 0.018));
        }
        prof[i] = (float)v;
    }
    return prof;
}

double StarSystem::outerOrbitKm() const {
    double r = 0;
    for (const Body& b : bodies) if (b.parent < 0 && b.type != PT_COMET) r = std::max(r, b.orbitRadiusKm);
    return r > 0 ? r : STAR_CLASSES[star.cls].minFirstOrbitKm * 12;
}

double StarSystem::trueAnomaly(double M, double e, double& rOverA) {
    if (e < 1e-9) { rOverA = 1; return M; }
    double E = M + e * std::sin(M);
    for (int i = 0; i < 12; i++) {
        double f = E - e * std::sin(E) - M, fp = 1 - e * std::cos(E);
        double d = f / fp;
        E -= d;
        if (std::fabs(d) < 1e-12) break;
    }
    rOverA = 1 - e * std::cos(E);
    return 2 * std::atan2(std::sqrt(1 + e) * std::sin(E * 0.5), std::sqrt(1 - e) * std::cos(E * 0.5));
}

std::string StarSystem::classString() const {
    if (companion >= 0 && companion < (int)bodies.size())
        return std::string(STAR_CLASSES[star.cls].code) + "+" + STAR_CLASSES[bodies[companion].starClass].code + " MULTIPLE";
    return std::string(STAR_CLASSES[star.cls].code) + " " + STAR_CLASSES[star.cls].name;
}

Star StarSystem::companionStar() const {
    Star k;
    if (companion < 0 || companion >= (int)bodies.size()) return k;
    const Body& b = bodies[companion];
    k.cls = b.starClass; k.radiusKm = b.radiusKm; k.luminosity = b.luminosity; k.color = b.color; k.seed = b.seed;
    k.massFactor = STAR_CLASSES[b.starClass].massFactor; k.name = b.name; k.valid = true;
    k.pulseHz = b.starClass == STAR_PULSAR ? 0.7 : 0;
    return k;
}

double StarSystem::starLightFactor(double luminosity, double distKm) {
    double f = std::pow(luminosity * (AU_GAME_KM / distKm) * (AU_GAME_KM / distKm), 0.25);
    return clampd(f, 0.35, 1.15);
}

StarSystem::Light StarSystem::lightAt(const Vec3& posKm, double t) const {
    Light l;
    double d1 = std::max(length(star.pos - posKm), 1.0);
    l.dir = (star.pos - posKm) / d1;
    double raw1 = star.luminosity / (d1 * d1);
    l.factor = starLightFactor(star.luminosity, d1);
    l.fromCompanion = false;
    if (companion >= 0 && companion < (int)bodies.size()) {
        Vec3 cp = bodyPos(companion, t);
        double d2 = std::max(length(cp - posKm), 1.0);
        double raw2 = bodies[companion].luminosity / (d2 * d2);
        if (raw2 > raw1) { l.dir = (cp - posKm) / d2; l.factor = starLightFactor(bodies[companion].luminosity, d2); l.fromCompanion = true; }
    }
    return l;
}

Vec3 StarSystem::parentPos(int i, double t) const {
    const Body& b = bodies[i];
    if (b.parent < 0) return star.pos;
    return bodyPos(b.parent, t);
}

Vec3 StarSystem::bodyPos(int i, double t) const {
    const Body& b = bodies[i];
    double M = b.orbitPhase0 + TAU * t / b.orbitPeriod;
    double rOverA = 1;
    double nu = b.ecc > 1e-9 ? trueAnomaly(std::fmod(M, TAU), b.ecc, rOverA) : M;   // M5-03 Kepler orbits
    double ph = nu + b.argPeri;
    Vec3 u(std::cos(b.orbitNode), 0, std::sin(b.orbitNode));
    Vec3 v0 = cross(Vec3(0, 1, 0), u);
    Vec3 v = v0 * std::cos(b.orbitIncl) + Vec3(0, 1, 0) * std::sin(b.orbitIncl);
    Vec3 rel = (u * std::cos(ph) + v * std::sin(ph)) * (b.orbitRadiusKm * rOverA);
    return parentPos(i, t) + rel;
}

Vec3 StarSystem::bodyVel(int i, double t) const {
    double h = std::max(1.0, std::fabs(bodies[i].orbitPeriod) * 1e-4);
    return (bodyPos(i, t + h) - bodyPos(i, t - h)) / (2 * h);
}

double StarSystem::meanAnomaly(int i, double t) const {
    const Body& b = bodies[i];
    double M = std::fmod(b.orbitPhase0 + TAU * t / b.orbitPeriod, TAU);
    return M < 0 ? M + TAU : M;
}

double StarSystem::rotationAngle(int i, double t) const {
    const Body& b = bodies[i];
    return b.rotPhase0 + TAU * t / b.rotPeriod;
}

Mat3 StarSystem::bodyFrame(int i, double t) const {
    const Body& b = bodies[i];
    double th = rotationAngle(i, t);
    Vec3 E0 = b.ref0 * std::cos(th) + b.ref1 * std::sin(th);
    Vec3 E1 = b.ref0 * -std::sin(th) + b.ref1 * std::cos(th);
    return Mat3::fromRows(E0, E1, b.spinAxis);
}

Vec3 StarSystem::bodyFromLatLon(double lat, double lon) {
    return Vec3(std::cos(lat) * std::cos(lon), std::cos(lat) * std::sin(lon), std::sin(lat));
}
void StarSystem::latLonFromBody(const Vec3& b, double& lat, double& lon) {
    lat = std::asin(clampd(b.z, -1, 1));
    lon = std::atan2(b.y, b.x);
}

Vec3 StarSystem::surfacePointWorld(int i, double t, double latRad, double lonRad, double altitudeKm) const {
    Mat3 f = bodyFrame(i, t);
    Vec3 bv = bodyFromLatLon(latRad, lonRad);
    Vec3 w = f.transposed() * bv;
    return bodyPos(i, t) + w * (bodies[i].radiusKm + altitudeKm);
}

double StarSystem::seasonOf(int i, double t) const {
    const Body& b = bodies[i];
    Vec3 toStar = normalize(parentPos(i, t) - bodyPos(i, t));
    if (b.parent >= 0) toStar = normalize(star.pos - bodyPos(i, t));
    return dot(b.spinAxis, toStar);   // the north pole leaning toward the sun: northern summer
}

std::string StarSystem::bodyLabel(int i) const {
    const Body& b = bodies[i];
    if (b.type == PT_COMPANION) return b.name + " (COMPANION STAR, " + STAR_CLASSES[b.starClass].name + ")";
    if (b.doublePlanet && b.parent >= 0) return b.name + " (" + PLANET_TYPES[b.type].name + ", twin planet)";
    return b.name + " (" + PLANET_TYPES[b.type].name + (b.parent >= 0 ? (bodies[b.parent].type == PT_COMPANION ? ", of the companion)" : ", moon)") : ")");
}

// R-402: the magnetic field, the star's storms and the aurora they make
const char* const MAGNETIC_CLASS_NAMES[4] = {"NONE", "WEAK", "MODERATE", "STRONG"};

bool isIceGiant(const Body& b) { return b.type == PT_GASGIANT && b.radiusKm < 45000 && b.color.b > b.color.r + 0.05f; }

double cometRefKm(const StarSystem& sys) {
    return sys.bodies.empty() ? AU_GAME_KM : std::max(AU_GAME_KM * 0.5, sys.bodies[0].type == PT_COMET ? AU_GAME_KM : sys.bodies[0].orbitRadiusKm);
}

double cometActivityAt(const StarSystem& sys, double distKm) {
    double refKm = cometRefKm(sys);
    return clampd((refKm * 1.6 / distKm) * (refKm * 1.6 / distKm), 0, 1);
}

double magneticField(const Body& b) {
    double u = unitFromHash(hashCombine(b.seed, 0x3A6F));
    double hours = std::fabs(b.rotPeriod) / 3600.0;
    double spin = clampd(1.4 - 0.8 * std::log10(std::max(hours, 1.0)), 0, 1);   // 1 under 3 h, 0.6 at 10 h, 0.3 at a day, 0 past 56 h
    double size = clampd((b.radiusKm - 1500) / 5000.0, 0, 1);
    return clampd(0.9 * u + 0.3 * spin + 0.25 * size - 0.35 - (b.locked ? 0.3 : 0.0), 0, 1);
}

int magneticClass(double mag) { return mag < 0.08 ? 0 : (mag < 0.3 ? 1 : (mag < 0.6 ? 2 : 3)); }

double auroralStorm(const Star& s, double t) {
    double slow = gnoise2(t / 2.6e5, 1.7, s.seed ^ 0xA5A5), fast = gnoise2(t / 1.5e4, 4.1, s.seed ^ 0x5A5A);   // three days; four hours
    double storm = clampd((0.5 + 0.5 * slow + 0.2 * fast - 0.58) / 0.3, 0, 1);
    return s.cls == STAR_RED_DWARF ? std::max(storm, 0.8 * starFlare(s, t)) : storm;   // S-01: a red dwarf's flare is a storm of its own
}

double starActivity(int cls) {   // S-01: flare stars and hot stars drive the nights; S-03: the neutron star's x-rays nearly as much as the pulsar's beams
    switch (cls) {
        case STAR_BLUE_GIANT: return 1.0;
        case STAR_PULSAR: return 0.9;
        case STAR_NEUTRON: return 0.85;
        case STAR_PROTOSTAR: return 0.9;   // S-04: a young star's wind and flares
        case STAR_WOLF_RAYET: return 1.0;   // S-05: the wind that strips its worlds
        case STAR_BLACK_HOLE: return 0.9;   // S-06: the disc's x-rays
        case STAR_RED_DWARF: return 0.8;
        case STAR_BLUE_WHITE: return 0.7;
        case STAR_ORANGE: return 0.55;
        case STAR_YELLOW: return 0.45;
        default: return 0.2;
    }
}

double auroraPotentialAt(const StarSystem& sys, const Body& b, double storm) {
    if (!PLANET_TYPES[b.type].atmosphere || hasOpaqueDeck(b.type)) return 0;
    double activity = starActivity(sys.star.cls);   // S-02: the one table
    double mag = magneticField(b);
    double steady = 0.6 * smoothstep(0.5, 1.0, mag);                                            // a strong field keeps a glow every night
    double driven = std::pow(storm, 0.7) * smoothstep(0.03, 0.4, mag) * (0.7 + 0.3 * mag);     // the storm lights the rest
    return activity * clampd(steady + driven, 0, 1);
}

double auroraPotential(const StarSystem& sys, const Body& b, double t) { return auroraPotentialAt(sys, b, auroralStorm(sys.star, t)); }
