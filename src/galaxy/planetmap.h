// The planet function: one deterministic description of every point of every
// body, from continental scale down to metres. The orbital view samples it
// coarsely to build an equirectangular map; the landing code samples the very
// same function at full detail, so what you walk on is what you saw from orbit.
#pragma once
#include "system.h"
#include <vector>
#include <string>
#include <atomic>
#include <cmath>

enum Material {
    MAT_ROCK = 0, MAT_SAND, MAT_GRASS, MAT_FOREST, MAT_SNOW, MAT_WATER, MAT_LAVA, MAT_ICE, MAT_CLOUD, MAT_QUARTZ,
    MAT_BASALT, MAT_DUST, MAT_GAS, MAT_METAL, MAT_SULPHUR, MAT_GRAPHITE, MAT_SALT, MAT_COUNT   // R-305: salt (playas, sinter)
};

// R-304: the traits of a body: up to three, drawn by type from the seed; each changes the ground, the water, the climate,
// the flora or the colours (`landformsAt`, `BodyGen::make`, `materialPalette`, the flora and the look). TRAIT_NAMES for
// the data sheet, `traitPhrase` for the description.
enum Trait {
    TR_NONE = 0,
    TR_CANYONS, TR_MESAS, TR_KARST, TR_RIFT, TR_ESCARPMENTS, TR_GLACIAL, TR_INSELBERGS, TR_CINDER_FIELD, TR_CHAOS, TR_POLYGONS,
    TR_YARDANGS, TR_ERG, TR_SALT_FLATS, TR_TRAPS, TR_GREAT_BASIN, TR_CORONAE, TR_SPIRES, TR_GEYSERS,
    TR_ARCHIPELAGO, TR_PANGAEA, TR_LAKELAND, TR_SNOWBALL, TR_EXOTIC_SEAS, TR_STORMS, TR_HAZE,
    TR_GIANT_FLORA, TR_LUMINOUS_FLORA, TR_DEAD_FOREST, TR_RED_SOIL, TR_BLACK_SAND, TR_CHALK,
    TR_COUNT
};
extern const char* TRAIT_NAMES[TR_COUNT];
const char* traitPhrase(int trait);          // ", cut by canyons"
bool traitEligible(int type, int trait);
extern const char* MATERIAL_NAMES[MAT_COUNT];

// N1-01: material families share a palette bank in every view (rock, water, forest, grass, sand, snow)
enum MatFamily { FAM_ROCK = 0, FAM_WATER, FAM_FOREST, FAM_GRASS, FAM_SAND, FAM_SNOW, FAM_COUNT };
int matFamily(int material);
// the material that stands for a family on a body type (rock: basalt on volcanic worlds, graphite on carbon worlds...)
int familyRep(int type, int family);
// N1-05: the one exposure rule: palette stop (0..63) of a fully lit surface of this albedo at noon, in every view
inline double noonStop(double albedo) { double v = 0.45 + 0.75 * albedo; return 50.0 * (v < 0.5 ? 0.5 : (v > 1.2 ? 1.2 : v)); }
// the same rule under a light of 0..1 (the star's light factor times the lambert term): the ground's curve
inline double exposureStop(double albedo, double light, bool atmosphere) { return noonStop(albedo) * std::pow(light < 0 ? 0 : light, atmosphere ? 0.65 : 0.55); }
// the material ramp every view uses: stops at 0 (black), 14 (dark), 34 (mid), 47 (c, fully lit), 56 (c->white), 63 (white).
// With a sky (`skyLight`, the zenith/horizon mix) the dark end takes the sky's colour: hemisphere light (M10-07), the
// same from orbit as on the ground, scaled by `day` (0 night .. 1 day).
struct MatRamp { RGB stop[6]; static constexpr double pos[6] = {0, 14, 34, 47, 56, 63}; };
MatRamp materialRamp(const RGB& c, const RGB* skyLight, double day);
RGB materialRampColor(const MatRamp& r, double stop);
inline RGB materialRampColor(const RGB& c, double stop) { return materialRampColor(materialRamp(c, nullptr, 1.0), stop); }
// N1-06: FLAT / HILLS / MOUNTAINS from a slope (rise over run, dimensionless)
const char* reliefClass(double slope);

// M9-05 biomes of habitable worlds
enum Biome { BIO_NONE = 0, BIO_TROPICAL, BIO_SAVANNA, BIO_DESERT, BIO_TEMPERATE, BIO_GRASSLAND, BIO_TAIGA, BIO_TUNDRA, BIO_WETLAND, BIO_ALPINE, BIO_ICE, BIO_OCEAN, BIO_COUNT };
extern const char* BIOME_NAMES[BIO_COUNT];

struct SurfaceSample {
    double height = 0;      // metres above the reference level (sea level where it exists)
    int material = MAT_ROCK;
    double albedo = 0.4;    // 0..1 brightness seen from space, before lighting
    double veg = 0;         // vegetation density 0..1
    double glow = 0;        // emissive 0..1 (lava)
    double relief = 0;      // 0..1 mountain-ness (used for texture/rock placement)
    double water = -1e9;    // height of the water surface here (sea 0, lakes and rivers above), -1e9 none (M9-04)
    int biome = BIO_NONE;   // M9-05
    double landform = 0;    // R-305: how much the traits' landforms shape this point (metres moved, plus 40 x flatten), for the galleries
    int waterKind = 0;      // B-320: 0 none or the sea, 1 a river, 2 a lake, 3 a wetland pool, 4 a rift lake (the water plane here, wet or bank)
    double shore = 1e9;     // B-322: signed distance to the water's edge in metres where the water is a feature with an outline (rivers,
                            // lakes): negative in the water, positive on the bank; 1e9 unknown (the sea, pools: the height over the level stands in)
    double scree = 0;       // O6-04: 0..1 loose rock: the talus at a cliff's foot, the steep flanks (boulder fields, no flora)
};

struct BodyGen {
    int type = PT_CRATERED;
    uint64_t seed = 0;
    double R = 3000;        // km
    double seaLevel = 0;    // felisian: -1..1 continent threshold
    double mountainAmp = 3000, hillAmp = 200, detailAmp = 20;
    double craterDensity = 0.3, craterDepth = 1.0;
    double iceCapLat = 72;  // degrees
    double snowLine = 3500; // metres at the equator
    double moistureBias = 0;
    double lavaLevel = -300;
    double fractureWidth = 0.05;
    double duneAmp = 0;
    double cloudCover = 0;  // 0..1
    double cloudScale = 3;  // pattern frequency
    double roughness = 0.5;
    double boulderDensity = 0.3;
    double volcanoes = 0.4;
    double bandCount = 8;   // gas giants
    RGB color, color2;      // primary/secondary tints
    RGB skyTint;            // for atmospheres
    RGB vegColor;           // vegetation (felisian)
    RGB vegColor2;          // N2-05: the second flora family's colour (hashed, related to vegColor)
    // M9-02 tectonics, M9-08 dunes
    bool tectonic = false;  // plates shape the surface
    double plateK = 0.8;    // worley frequency on the unit sphere (about 4 pi k^2 plates)
    double chainAmp = 2500; // height of convergent mountain chains, metres
    double duneWl = 0.25;   // dune wavelength, km
    double windAngle = 0;   // prevailing wind, radians
    int duneStyle = 0;      // 0 transverse, 1 barchan, 2 star
    double season = 0;      // M9-09: +1 northern summer .. -1 northern winter, scaled by the tilt
    double tempBias = 0;    // M9-05: climate offset from the body's equilibrium temperature, degrees
    double riverDensity = 0.6;
    double lakeDensity = 0.3;    // B-320: the share of 7 km cells that hold a lake (in wet, low country)
    int traits[3] = {0, 0, 0};   // R-304: TR_NONE-padded
    bool hasTrait(int t) const { return traits[0] == t || traits[1] == t || traits[2] == t; }
    double liquidLevel = -1e9;   // R-304 (TR_EXOTIC_SEAS): the level of a liquid that is not water (metres); -1e9 none
    double contScale = 1;        // R-304 (TR_PANGAEA): the continent field's wavelength factor
    double islandDensity = 0.18; // R-304 (TR_ARCHIPELAGO): the volcanic islands' density
    int floraFamily = 0, floraFamily2 = 0;   // N2-01 silhouettes: 0 dome, 1 cone, 2 umbrella, 3 tiered giant, 4 fibrous stalk, 5 fern tree, 6 mushroom tree; B-315: 7 weeping, 8 candelabra, 9 spire
    bool locked = false;    // M5-08: one face toward the star; the climate follows the sub-stellar point
    Vec3 lockedDir;         // body-frame unit vector of the sub-stellar point
    double selfGlow = 0;    // M5-02: substellar objects shine on their own (0..1)
    RGB matColor[MAT_COUNT];   // N1-01: the colour of every material on this body (no star light), used by globe, map and ground
    // O6-02: the relief spectrum (hashed from the seed; the rng stream stays as in generation 5). A self-affine sum of
    // octaves from reliefWl0 km down to the sampling scale, amplitude a0 (wl / wl0)^H, whose a0 is reliefFlat on the
    // plains, reliefHill in hill country and reliefMtn in the mountains (metres); see `reliefSum` in planetmap.cpp.
    double reliefWl0 = 16;      // km, the longest wavelength (a quarter of the radius at most)
    double reliefH = 0.85;      // spectral exponent: 1 keeps the same slope at every scale, lower makes fine scales rougher
    double reliefRidge = 0.5;   // 0 rolling (fbm) .. 1 ridged (sharp crests, wide valleys)
    double reliefWarp = 0.3;    // the coordinate is folded by this fraction of reliefWl0 (curved ridges)
    double reliefErosion = 4;   // octave weight 1 / (1 + reliefErosion slope^2): steep ground stops gathering detail
    double reliefHybrid = 0.6;  // 0..1 how much low ground is smoothed (valley floors)
    double reliefFlat = 55, reliefHill = 300, reliefMtn = 900;
    double cliffStep = 40;      // O6-04: the height of the cliff bands on steep ground (metres), hashed
    double cliffAmount = 0.5;   // O6-04: the share of the steep ground that is cliffed (0..1)
    uint64_t sA = 1, sB = 2, sC = 3, sD = 4, sE = 5, sF = 6, sG = 7;
    static BodyGen make(const Body& b, double season = 0);
};

// B-321: the climate's mean surface temperature (degrees C) at a latitude, height and season on a living world: the
// biome table and the HUD read the same number. `sub` is dot(unit, lockedDir) on a locked world (0 otherwise).
double climateTempC(const BodyGen& g, double absLatDeg, double heightM, double winter, double sub);

// R-307: the zenith and horizon colours of a type's sky by day (false: no sky); the surface, the globe's hemisphere light
// and the landing map read the same pair
bool typeSky(const BodyGen& g, RGB& zenith, RGB& horizon);
// R-307: the vents of the geyser basins (TR_GEYSERS) near a point: the centres of the sinter mounds (unit vectors) and their
// radii in km, the same features the landform builds, so a surface can put an erupting geyser at each
struct GeyserVent { Vec3 unit; double radiusKm; uint64_t id; };
void geyserVents(const BodyGen& g, const Vec3& unit, std::vector<GeyserVent>& out);
// R-304: "CANYON LANDS, SALT FLATS" or ""
std::string traitList(const BodyGen& g);
// M9-14: one generated sentence about a body or a star.
std::string describeBody(const Body& b, const BodyGen& g);
std::string describeStar(const Star& s);
// Season name for a latitude ("NORTHERN SUMMER", ...), "" when the tilt is negligible.
std::string seasonName(double season, double latRad, double axialTilt);

// unit: body-frame unit vector. detailMeters: smallest feature scale to include.
SurfaceSample sampleSurface(const BodyGen& g, const Vec3& unit, double detailMeters);
// O6-02: the relief spectrum alone at a point (metres, and the slope of the sum as rise over run), for tests
struct Relief { double h = 0, slope = 0; };
Relief reliefAt(const BodyGen& g, const Vec3& unit, double a0, double ridge, double detailMeters);
// Static cloud pattern (0..1) at lon/lat; animate by offsetting lon.
double sampleCloudPattern(const BodyGen& g, double lon, double lat);

struct PlanetMap {
    static constexpr int W = 512, H = 256;   // M9-15 (was 256x128)
    std::vector<float> height;
    std::vector<uint8_t> material;
    std::vector<uint8_t> albedo;   // 0..255
    std::vector<uint8_t> cloud;    // 0..255
    std::vector<uint8_t> veg;
    uint64_t seed = 0;
    std::atomic<bool> valid{false};     // set when a (possibly background) generation finished
    std::atomic<bool> generating{false};
    int seasonBucket = -1;
    PlanetMap() = default;
    PlanetMap(const PlanetMap&) = delete;
    PlanetMap& operator=(const PlanetMap&) = delete;
    void generate(const BodyGen& g);
    int texelIndex(double lon, double lat) const;    // nearest texel
    // bilinear albedo/cloud/height at lon/lat (radians)
    double albedoAt(double lon, double lat) const;
    double cloudAt(double lon, double lat) const;
    double heightAt(double lon, double lat) const;
    int materialAt(double lon, double lat) const;
};
