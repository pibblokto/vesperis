// M9-14: generated one-sentence descriptions of bodies and stars.
#include "planetmap.h"
#include "core/rng.h"
#include <cmath>

namespace {
const char* sizeWord(double r, int type) {
    if (type == PT_GASGIANT) return r > 50000 ? "a giant" : "a modest giant";
    if (r < 1500) return "a tiny";
    if (r < 3000) return "a small";
    if (r < 5500) return "a medium-sized";
    return "a large";
}
}

std::string describeBody(const Body& b, const BodyGen& g) {
    Rng r(b.seed ^ 0xDE5C);
    std::string s = std::string(sizeWord(b.radiusKm, b.type)) + " ";
    switch (b.type) {
        case PT_MOLTEN: s += r.chance(0.5) ? "world of cooling crust, split by glowing cracks and lava lakes" : "molten world whose surface never sets, red seams between black plates"; break;
        case PT_CRATERED: s += g.craterDensity > 0.55 ? "airless world saturated with craters, bright rays crossing dark maria" : "dusty, airless world of old craters and quiet plains"; break;
        case PT_VENUSIAN: s += "world smothered in clouds, a furnace of basalt domes below"; break;
        case PT_FELISIAN: s += g.seaLevel < -0.1 ? "living world of wide oceans, scattered continents and river valleys" : (g.seaLevel > 0.1 ? "living world of dry continents, inland seas and long mountain chains" : "living world of continents and seas, forests along its rivers"); break;
        case PT_ROCKY: s += "creased, rocky world of ridges, rifts and boulder fields"; break;
        case PT_THINATMO: s += g.duneAmp > 12 ? "world of thin air, red-brown dune seas and shield volcanoes" : "world of thin air, canyons and dust plains under an exotic sky"; break;
        case PT_GASGIANT: s += "of banded clouds and drifting storms, with no surface to land on"; break;
        case PT_ICY: s += g.fractureWidth > 0.06 ? "icy world, its crust broken into plates by dark fractures" : "icy world with a smooth, cratered crust and bright cryovolcanic domes"; break;
        case PT_QUARTZ: s += "milky quartz world glinting under a thin oxygen sky"; break;
        case PT_OCEAN: s += "water world without land, ice floes drifting toward its poles"; break;
        case PT_METAL: s += "iron world, dark and dense, its craters glinting like hammered steel"; break;
        case PT_VOLCANIC: s += "tidally heated moon of sulphur plains and active cones"; break;
        case PT_CARBON: s += "black world of graphite plains and diamond crusts"; break;
        case PT_SUBSTELLAR: s += "failed star, banded and stormy, glowing dull red with the heat it was born with"; break;
        case PT_COMET: s += "mountain of ice and dust that grows a tail each time it dives past the star"; break;
        case PT_EUROPAN: s += r.chance(0.5) ? "shell of ice over a hidden ocean, its cracks stained red and venting water into space" : "ice moon of long double ridges and broken rafts, geysers rising from the cracks"; break;
        case PT_TECTONIC: s += r.chance(0.5) ? "restless world of rift valleys, fissures running with lava and ground that never stops shaking" : "world torn by its own crust, fountains of lava along the rifts under a sulphur sky"; break;
        case PT_DESERT: s += r.chance(0.5) ? "dry world of dune seas, mesas and salt pans under a dusty sky" : "desert world of canyons and ergs, dust devils crossing its plains"; break;
        case PT_HYDROCARBON: s += "cold world under an orange haze, seas of methane and dunes of tar"; break;
        case PT_BOMBARDED: s += "young airless world under a rain of meteorites, its craters fresh and rayed"; break;
        case PT_ACIDIC: s += r.chance(0.5) ? "corrosive world of acid seas and bleached karst under a yellow-green sky" : "world whose rain eats the rock, sulphur crusts along its acid shores"; break;
        case PT_COMPANION: return std::string("The second sun of the system, a ") + STAR_CLASSES[b.starClass].name + " bound to the primary" + (b.moonCount > 0 ? ", with worlds of its own." : ".");
        default: s += "world"; break;
    }
    for (int k = 0; k < 3; k++) if (g.traits[k]) s += traitPhrase(g.traits[k]);   // R-304
    if (b.doublePlanet) s += b.parent >= 0 ? ", the smaller half of a double planet" : ", the larger half of a double planet";
    else if (b.parent >= 0) s += ", a moon";
    if (b.locked && b.type == PT_FELISIAN) s += ", ice on its night side and deserts under its sun";
    double gg = b.gravity / 9.8;
    if (gg < 0.25) s += ", where a jump takes you high";
    else if (gg > 1.8) s += ", heavy underfoot";
    if (b.tempK > 500) s += ", scorched by its star";
    else if (b.tempK < 120) s += ", deep in the cold";
    if (b.rings) s += ", wearing a ring system";
    if (magneticClass(magneticField(b)) == 3 && PLANET_TYPES[b.type].atmosphere && !hasOpaqueDeck(b.type)) s += ", its poles crowned with aurorae";   // R-402
    if (b.moonCount >= 3) s += ", with a family of moons";
    if (b.ecc > 0.15) s += ", on a markedly oval orbit";
    if (b.parent < 0 && std::fabs(1.0 - b.rotPeriod / b.orbitPeriod) < 0.02) s += ", one face forever toward the sun";
    else if (b.rotPeriod < 0) s += ", spinning the wrong way";
    s += ".";
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
