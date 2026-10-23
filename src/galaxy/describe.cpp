// M9-14: generated one-sentence descriptions of bodies and stars.
#include "planetmap.h"
#include "core/rng.h"
#include <cmath>

namespace {
const char* sizeWord(double r, int type) {
    if (KINDS[type].giantClass == GC_DWARF) return "a small world";   // P-07: its sentence goes on from there
    if (kindHas(type, KT_GAS_GIANT)) return r > 50000 ? "a giant" : "a modest giant";
    if (r < 1500) return "a tiny";
    if (r < 3000) return "a small";
    if (r < 5500) return "a medium-sized";
    return "a large";
}
}

std::string describeBody(const Body& b, const BodyGen& g) {
    Rng r(b.seed ^ 0xDE5C);
    if (b.type == PT_COMPANION) return std::string("The second sun of the system, a ") + STAR_CLASSES[b.starClass].name + " bound to the primary" + (b.moonCount > 0 ? ", with worlds of its own." : ".");
    std::string s = std::string(sizeWord(b.radiusKm, b.type)) + " ";
    // P-00: the kind's phrases (KindDef::sentence) by its rule: one, a coin, or the body's own parameter
    const KindDef& K = KINDS[b.type];
    switch (K.sentenceRule) {
        case SR_COIN: s += r.chance(0.5) ? K.sentence[0] : K.sentence[1]; break;
        case SR_CRATERS: s += g.craterDensity > 0.55 ? K.sentence[0] : K.sentence[1]; break;
        case SR_DUNES: s += g.duneAmp > 12 ? K.sentence[0] : K.sentence[1]; break;
        case SR_FRACTURES: s += g.fractureWidth > 0.06 ? K.sentence[0] : K.sentence[1]; break;
        case SR_SEAS: s += g.seaLevel < -0.1 ? K.sentence[0] : (g.seaLevel > 0.1 ? K.sentence[1] : K.sentence[2]); break;
        case SR_DEAD_SEAS: s += g.hasTrait(TR_CIVILISATION) ? K.sentence[0] : (r.chance(0.5) ? K.sentence[1] : K.sentence[2]); break;   // C-01
        default: s += K.sentence[0]; break;
    }
    for (int k = 0; k < TRAIT_SLOTS; k++) if (g.traits[k]) s += TRAITS[g.traits[k]].phrase;   // R-304 (Q-01: the six slots)
    if (b.doublePlanet) s += b.parent >= 0 ? ", the smaller half of a double planet" : ", the larger half of a double planet";
    else if (b.parent >= 0) s += ", a moon";
    if (b.locked && livingGround(b.type) && !kindHas(b.type, KT_FIXED_SUN)) s += ", ice on its night side and deserts under its sun";   // P-03: a fixed sun's kind says it its own way
    double gg = b.gravity / 9.8;
    if (gg < 0.25) s += ", where a jump takes you high";
    else if (gg > 1.8) s += ", heavy underfoot";
    if (b.tempK > 500) s += ", scorched by its star";
    else if (b.tempK < 120) s += ", deep in the cold";
    if (b.rings) s += ", wearing a ring system";
    if (magneticClass(magneticField(b)) == 3 && K.atmosphere && !hasOpaqueDeck(b.type) && !g.hasTrait(TR_GREAT_AURORAE)) s += ", its poles crowned with aurorae";   // R-402 (Q-02: the trait's phrase says more)
    if (b.moonCount >= 3) s += ", with a family of moons";
    if (b.ecc > 0.15) s += ", on a markedly oval orbit";
    if (b.parent < 0 && std::fabs(1.0 - b.rotPeriod / b.orbitPeriod) < 0.02) s += ", one face forever toward the sun";
    else if (b.rotPeriod < 0) s += ", spinning the wrong way";
    s += ".";
    return s;
}

const char* const BODY_FACT_NAMES[BF_COUNT] = {"SIDEWAYS SPIN", "RETROGRADE SPIN", "RACING DAY", "LOCKED FACE", "ECCENTRIC YEAR", "STRONG FIELD", "NO FIELD",
                                                "CAPTURED", "TWIN", "RINGED"};

unsigned bodyFacts(const StarSystem& sys, int bi) {
    if (bi < 0 || bi >= (int)sys.bodies.size()) return 0;
    const Body& b = sys.bodies[bi];
    if (b.type == PT_COMPANION) return 0;
    unsigned f = 0;
    const bool moon = b.parent >= 0 && sys.bodies[b.parent].type != PT_COMPANION;   // a world of the companion has a year of its own
    if (b.axialTilt > 60 * DEG) f |= 1u << BF_SIDEWAYS;
    if (b.rotPeriod < 0) f |= 1u << BF_RETROGRADE;
    if (!b.locked && std::fabs(b.rotPeriod) < 1800) f |= 1u << BF_RACING;
    if (b.locked) f |= 1u << BF_LOCKED;
    if ((moon ? sys.bodies[b.parent].ecc : b.ecc) > 0.2) f |= 1u << BF_ECCENTRIC;
    const int mc = magneticClass(magneticField(b));
    if (mc == 3) f |= 1u << BF_STRONG_FIELD;
    else if (mc == 0) f |= 1u << BF_NO_FIELD;
    if (moon && b.orbitPeriod < 0) f |= 1u << BF_CAPTURED;
    if (b.doublePlanet) f |= 1u << BF_TWIN;
    if (b.rings) f |= 1u << BF_RINGED;
    return f;
}

std::string factList(const StarSystem& sys, int bi) {
    const unsigned f = bodyFacts(sys, bi);
    std::string s;
    for (int k = 0; k < BF_COUNT; k++) if ((f >> k) & 1u) { if (!s.empty()) s += ", "; s += BODY_FACT_NAMES[k]; }
    return s;
}

std::string describeStar(const Star& s) {
    Rng r(s.seed ^ 0x57A2);
    std::string d;
    switch (s.cls) {
        case STAR_YELLOW: d = r.chance(0.5) ? "A steady yellow star of the kind that keeps water liquid" : "A yellow star, warm and unhurried"; break;
        case STAR_ORANGE: d = "An orange dwarf, small and patient, its worlds huddled close"; break;
        case STAR_BLUE_GIANT: d = "A blue giant, young and violent, flooding its wide system with hard light"; break;
        case STAR_RED_GIANT: d = "A swollen red giant that has already eaten its inner planets"; break;
        case STAR_WHITE_DWARF: d = "A white dwarf, the dense ember of a dead star, lighting frozen worlds"; break;
        case STAR_PULSAR: d = "A pulsar, a spinning corpse whose beams sweep the system"; break;
        case STAR_RED_DWARF: d = r.chance(0.5) ? "A red dwarf, small and cool, its near worlds turning one face to it" : "A red dwarf, the commonest star there is, given to sudden flares"; break;   // S-01
        case STAR_BLUE_WHITE: d = "A blue-white star, hot and short-lived, its wide system bare under the ultraviolet"; break;
        case STAR_ORANGE_GIANT: d = "An orange giant, an old star grown large, its warmth reaching out among what were ice worlds"; break;
        case STAR_CARBON: d = "A carbon star, a ruby giant wrapped in its own soot, every world under its ember light"; break;
        case STAR_NEUTRON: d = r.chance(0.5) ? "A neutron star, the quiet corpse of a supernova, a point of white light over what survived it" : "A neutron star, faint to the eye and fierce in x-rays, its outer worlds glassed by the blast that made it"; break;   // S-03
        case STAR_PROTOSTAR: d = r.chance(0.5) ? "A protostar, not yet done being born, wrapped in the cloud it fell out of, its disc of dust seen edge-on" : "A young star still gathering itself, its light soft through the dust of the disc that will be its worlds"; break;   // S-04
        case STAR_WOLF_RAYET: d = r.chance(0.5) ? "A Wolf-Rayet star, a massive star blowing itself away, blinding inside the ring of gas it has shed" : "A Wolf-Rayet star, its wind stripping what is left of its worlds, the shell it threw off a ring round it in the sky"; break;   // S-05
        case STAR_BLACK_HOLE: d = "A black hole of " + std::to_string((int)(s.massFactor + 0.5)) + " suns' mass, " + (r.chance(0.5) ? "a hole in the sky ringed by the bent light of the stars behind it" : "dark but for the disc of gas it drinks, the stars behind it drawn into arcs"); break;   // S-06
        case STAR_ROGUE: return "No star: the world drifts alone between the stars, warmed by nothing but its own heat.";   // D-01
        default: d = "A star"; break;
    }
    if (s.luminosity > 30) d += ", fiercely bright";
    else if (s.luminosity < 0.2 && s.cls != STAR_RED_DWARF && s.cls != STAR_CARBON && s.cls != STAR_NEUTRON && s.cls != STAR_BLACK_HOLE) d += ", dim";   // S-01: the class says so already
    return d + ".";
}

std::string seasonName(double season, double latRad, double axialTilt) {
    if (axialTilt < 3 * DEG) return "";
    double local = latRad >= 0 ? season : -season;   // + this hemisphere leans toward the sun
    double f = local / std::max(0.05, std::sin(axialTilt));
    const char* hemi = latRad >= 0 ? "NORTHERN" : "SOUTHERN";
    const char* part = f > 0.6 ? "SUMMER" : (f < -0.6 ? "WINTER" : (f > 0 ? "SPRING" : "AUTUMN"));
    return std::string(hemi) + " " + part;
}
