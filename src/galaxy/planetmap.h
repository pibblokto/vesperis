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

// P-00: the materials, the traits and the material families are in galaxy/kinds.h (the kind and trait tables read them)
struct BodyGen;
extern const char* const MATERIAL_NAMES[MAT_COUNT];
std::string traitList(const BodyGen& g);   // R-304: "CANYON LANDS, SALT FLATS" or ""
// N1-01: the family a material belongs to (its nature: the works and the roads read it); `matFamilyOn` is the family it is drawn in
// on a kind (its bank in every view): a kind without flora may lend the grass and forest families to its two accents
int matFamily(int material);
int matFamilyOn(int type, int material);
// the material that stands for a family on a kind (rock: basalt on volcanic worlds, graphite on carbon worlds...: the row's `famRep`, an accent for grass and forest)
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
extern const char* const BIOME_NAMES[BIO_COUNT];

struct SurfaceSample {
    double height = 0;      // metres above the reference level (sea level where it exists)
    int material = MAT_ROCK;
    double albedo = 0.4;    // 0..1 brightness seen from space, before lighting
    double veg = 0;         // vegetation density 0..1
    double glow = 0;        // emissive 0..1 (lava)
    double relief = 0;      // 0..1 mountain-ness (used for texture/rock placement)
    double water = -1e9;    // height of the water surface here (sea 0, lakes and rivers above), -1e9 none (M9-04)
    int biome = BIO_NONE;
    double landform = 0;    // how much the traits' landforms shape this point (metres moved, plus 40 x flatten), for the galleries
    int waterKind = 0;      // 0 none or the sea, 1 a river, 2 a lake, 3 a wetland pool, 4 a rift lake (the water plane here, wet or bank)
    double shore = 1e9;     // signed distance to the water's edge in metres where the water is a feature with an outline (rivers,
                            // lakes): negative in the water, positive on the bank; 1e9 unknown (the sea, pools: the height over the level stands in)
    double scree = 0;       // 0..1 loose rock: the talus at a cliff's foot, the steep flanks (boulder fields, no flora)
    double oldSea = 0;      // 0..1 the dry bed of a dead desert world's sea (the drainage's outlet, the finders' shore)
    int landformTrait = 0;  // the trait whose landform marks this point (the last to set `LandOut::mark`), 0 none: the scanner's sights by trait
    // the water's colours of the world's traits here, 0..1 (`qualityWater`): the water kind's (a pink lake, a milky sea's patch that glows at
    // night) and the life kind's (an algal bloom's streak, a barrier reef's lagoon); the surface, the globe and the maps draw them in their own
    // colours (`BodyGen::waterTintColour`, `lifeTintColour`)
    double waterTint = 0, lifeTint = 0;
    double groundTint = 0;  // the ground trait's field here, 0..1: a meteorites' strewn field's stones (the surface's objects), a rainbow stratum's accent
    double slipFace = 0;    // how much this point is a sand dune's slip face, 0..1 (its lee, downwind of the crest, where the sand slides: the singing
                            // dunes' hum; the dune field's share times the lee's, whatever the face's steepness; not a gypsum's dunes, nor snow's drifts)
};

struct BodyGen {
    int type = PT_CRATERED;
    uint64_t seed = 0;
    // R-413: the world's own seed (`make`): a people's generator (C-13's `peopleGen`, or a copy with the people's seed) changes `seed`
    // alone and keeps it, so it can be read as its world's people (a second people's age is its first's: `peopleAgeOf`)
    uint64_t worldSeed = 0;
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
    // GEN 12: a giant's character on its globe (hashed, the rng stream stays): the share of the storms' cells that hold a
    // storm (the plain giant's 0.15), the bands' contrast (an ice giant's faint under its haze), and a great storm, an oval the size
    // of a moon at its own latitude (its centre in the pattern's frame, its semi-axes in radians of arc east and north, +1 bright
    // or -1 dark); `giantJetAt` (galaxy/probe.h) turns it whole with its own latitude's jet
    double stormShare = 0.15, bandContrast = 1;
    bool gsOn = false; double gsLat = 0, gsLon = 0, gsA = 0.1, gsB = 0.06, gsTone = 1;
    RGB color, color2;      // primary/secondary tints
    RGB skyTint;            // for atmospheres
    RGB vegColor;           // vegetation (felisian)
    RGB vegColor2;          // the second flora family's colour (hashed, related to vegColor)
    // tectonics, M9-08 dunes
    bool tectonic = false;  // plates shape the surface
    double plateK = 0.8;    // worley frequency on the unit sphere (about 4 pi k^2 plates)
    double chainAmp = 2500; // height of convergent mountain chains, metres
    double duneWl = 0.25;   // dune wavelength, km
    double windAngle = 0;   // prevailing wind, radians
    int duneStyle = 0;      // 0 transverse, 1 barchan, 2 star
    double season = 0;      // +1 northern summer .. -1 northern winter, scaled by the tilt
    double tempBias = 0;    // climate offset from the body's equilibrium temperature, degrees
    double latGradient = 1; // the living climate's contrast from the equator to the poles (the kind's: a hothouse's flat, an ice age's steep)
    double seaDepth = 1;    // the living ground's seas' depth factor (the kind's: a superhabitable world's shelf seas)
    double grassBand = 0;   // the living biomes' grass band widened (the kind's: a steppe's)
    double subContrast = 45;   // a locked living world's day-night contrast (M5-08's; the kind's: a terminator world's sharper)
    double riverDensity = 0.6;
    double lakeDensity = 0.3;    // the share of 7 km cells that hold a lake (in wet, low country)
    int traits[TRAIT_SLOTS] = {0, 0, 0, 0, 0, 0};   // TR_NONE-padded; Q-01: the first three R-304's, the last three by kind of trait (each TR_NONE-padded)
    bool hasTrait(int t) const { for (int k = 0; k < TRAIT_SLOTS; k++) if (traits[k] == t) return true; return false; }
    bool anyTrait() const { for (int k = 0; k < TRAIT_SLOTS; k++) if (traits[k]) return true; return false; }
    double liquidLevel = -1e9;   // TR_EXOTIC_SEAS: the level of a liquid that is not water (metres); -1e9 none
    double tideRangeM = 0;       // TR_GREAT_TIDES: the spring range on the coast at the equator (`tideRangeOf`); 0 no tides (`tideRangeAt`: a place's)
    RGB waterTintColour, lifeTintColour;   // the pink lakes' pink, the milky seas' glow; the blooms' red or green, the lagoons' turquoise
    double seaReachDeg = 0;      // a magma ocean's sea reaches this far from the substellar point (degrees)
    double contScale = 1;        // TR_PANGAEA: the continent field's wavelength factor
    double islandDensity = 0.18; // TR_ARCHIPELAGO: the volcanic islands' density
    double oldSeaM = -1e9;       // TR_CIVILISATION on a desert: the level its seas stood at (metres), dry now; -1e9 none
    int floraFamily = 0, floraFamily2 = 0;   // silhouettes: 0 dome, 1 cone, 2 umbrella, 3 tiered giant, 4 fibrous stalk, 5 fern tree, 6 mushroom tree; B-315: 7 weeping, 8 candelabra, 9 spire
    bool locked = false;    // one face toward the star; the climate follows the sub-stellar point
    Vec3 lockedDir;         // body-frame unit vector of the sub-stellar point
    Vec3 apexDir;           // a two-faced moon: the body-frame direction it travels (`Body::apexDir`; zero otherwise)
    double selfGlow = 0;    // substellar objects shine on their own (0..1)
    RGB matColor[MAT_COUNT];   // the colour of every material on this body (no star light), used by globe, map and ground
    // the relief spectrum (hashed from the seed; the rng stream stays as in generation 5). A self-affine sum of
    // octaves from reliefWl0 km down to the sampling scale, amplitude a0 (wl / wl0)^H, whose a0 is reliefFlat on the
    // plains, reliefHill in hill country and reliefMtn in the mountains (metres); see `reliefSum` in planetmap.cpp.
    double reliefWl0 = 16;      // km, the longest wavelength (a quarter of the radius at most)
    double reliefH = 0.85;      // spectral exponent: 1 keeps the same slope at every scale, lower makes fine scales rougher
    double reliefRidge = 0.5;   // 0 rolling (fbm) .. 1 ridged (sharp crests, wide valleys)
    double reliefWarp = 0.3;    // the coordinate is folded by this fraction of reliefWl0 (curved ridges)
    double reliefErosion = 4;   // octave weight 1 / (1 + reliefErosion slope^2): steep ground stops gathering detail
    double reliefHybrid = 0.6;  // 0..1 how much low ground is smoothed (valley floors)
    double reliefFlat = 55, reliefHill = 300, reliefMtn = 900;
    double cliffStep = 40;      // the height of the cliff bands on steep ground (metres), hashed
    double cliffAmount = 0.5;   // the share of the steep ground that is cliffed (0..1)
    uint64_t sA = 1, sB = 2, sC = 3, sD = 4, sE = 5, sF = 6, sG = 7;
    // the body's star's sector and its index in the system (`Body::starSx..`): a reader of the world that needs its system (the
    // history: the star, the comets, the moons) regenerates it from these (`historyOfWorld`, peoples/history.h); never read by the terrain
    int64_t starSx = 0, starSy = 0, starSz = 0;
    int bodyIndex = -1;
    bool starKnown = false;
    // a dead world (`deadKind`): its present's mean temperature at the sea's level, as the HUD reads it without the day (its climate,
    // `tempBias`, is its then-world's: the ground is the same in both times); a then-world's copy (`thenWorld`) keeps the dead kind it is now
    double nowTempC = 0;
    int deadAs = -1;
    // a body of a system with no sun (a drifter and its moons): its own traits given (`darkTraitsOf`), the ground's dark cover read
    // (`darkGround`: its liquid frozen at its level where `frozenSeas`, its air's snow, a young interior's lava lakes)
    bool dark = false, frozenSeas = false;
    // a body of a drifter's system: a people's world there lost its sun (its then-world keeps the flag: the profile, the history and the
    // records read the loss through it)
    bool drifter = false;
    static BodyGen make(const Body& b, double season = 0);
};
// D-04: the drifters' own traits a body in the dark is given (into `out`, at most three: FROZEN AIR where its air froze, EMBERS on a lava
// world and on a share of the rock drifters, LIGHTS IN THE WATER on a share of the dark oceans, LIGHTS LEFT ON over a late people's world (D-05:
// a futuristic or far-future drifter's people's)); how many
int darkTraitsOf(const Body& b, int* out);
constexpr double EMBERS_SHARE = 0.3;            // of the rock drifters (a young interior, still erupting)
constexpr double LIGHTS_IN_WATER_SHARE = 0.5;   // of the dark oceans (if anything lives there)
// B-419: what is made on a world is the world's, not its season's. The terrain the explorer sees keeps the season (the winter
// hemisphere's ice cap and snow line); a reader of the world's made things (a people's plan, its extent, its histories, settlements,
// roads and works, the scanner's sights) reads it without (the cap and the line at their means): the generator itself where its season
// is 0, else a copy without it in `tmp`
inline const BodyGen& unseasoned(const BodyGen& g, BodyGen& tmp) { if (g.season == 0) return g; tmp = g; tmp.season = 0; return tmp; }
// P-04: the world in its people's time. A dead world (`deadKind`) was the living kind of its row's `thenKind`: the same seed, draws and
// relief, so the same ground (its heights the same in both times: every town stands where it stood), without the present's cover (its seas
// water, its forests living, its climate theirs). Every reader of a people's world (its plan, extent, culture, histories, settlements,
// roads, works, records and instruments) reads this one; the drawing reads the world as it is. D-05: a drifter's people's world without the
// dark's (its seas not frozen, its air not fallen: `dark` off), a desert drifter's climate its then-world's already (`Body::thenTempK`).
// Every other world's is `unseasoned`
inline const BodyGen& thenWorld(const BodyGen& g, BodyGen& tmp) {
    if (!deadKind(g.type)) {
        if (!(g.dark && g.hasTrait(TR_CIVILISATION))) return unseasoned(g, tmp);
        tmp = g; tmp.season = 0; tmp.dark = false; tmp.frozenSeas = false;   // a drifter's people's world (a desert) had its sun
        return tmp;
    }
    tmp = g; tmp.deadAs = g.type; tmp.type = KINDS[g.type].thenKind; tmp.season = 0;
    tmp.dark = false; tmp.frozenSeas = false;   // in its people's time it had its sun
    return tmp;
}
inline int nowKind(const BodyGen& g) { return g.deadAs >= 0 ? g.deadAs : g.type; }   // P-04: the kind the explorer walks (a then-world's: the dead kind)

// B-321: the climate's mean surface temperature (degrees C) at a latitude, height and season on a living world: the
// biome table and the HUD read the same number. `sub` is dot(unit, lockedDir) on a locked world (0 otherwise).
double climateTempC(const BodyGen& g, double absLatDeg, double heightM, double winter, double sub);
double tideRangeAt(const BodyGen& g, const Vec3& unit);   // Q-02: the spring range at a place (the trait's at the equator, by cos^2 of the latitude)
// P-03: how far a point lies inside an ice sheet's edge (0 at the edge, 1 where the sheet is full, under 0 outside): the tundra's and
// the snowball's by latitude (14 degrees from the edge to the full sheet), a terminator world's by its climate at the sea's level (the edge
// at -4 C, full at -29, wandering by seven degrees: the night side's)
double iceSheetIn(const BodyGen& g, const Vec3& unit);

// R-307: the zenith and horizon colours of a type's sky by day (false: no sky); the surface, the globe's hemisphere light
// and the landing map read the same pair
bool typeSky(const BodyGen& g, RGB& zenith, RGB& horizon);
// M9-14: one generated sentence about a body or a star.
std::string describeBody(const Body& b, const BodyGen& g);
std::string describeStar(const Star& s);
// Q-01: the facts of a body, read from its orbit and its spin (no draw): SIDEWAYS SPIN (a tilt over 60 degrees), RETROGRADE SPIN, RACING DAY (a
// day under half an hour of the game's clock: its days are 20 minutes to 4 hours, the fastest twelfth), LOCKED FACE (one face to its sun),
// ECCENTRIC YEAR (its year's orbit, a moon's its planet's, over 0.2), STRONG FIELD and NO FIELD (R-402's classes), CAPTURED (a moon on a
// retrograde orbit), TWIN (a double planet), RINGED; "SIDEWAYS SPIN, RINGED" or ""
enum BodyFact { BF_SIDEWAYS = 0, BF_RETROGRADE, BF_RACING, BF_LOCKED, BF_ECCENTRIC, BF_STRONG_FIELD, BF_NO_FIELD, BF_CAPTURED, BF_TWIN, BF_RINGED, BF_COUNT };
extern const char* const BODY_FACT_NAMES[BF_COUNT];
unsigned bodyFacts(const StarSystem& sys, int bi);   // bits of BodyFact
std::string factList(const StarSystem& sys, int bi);
// Season name for a latitude ("NORTHERN SUMMER", ...), "" when the tilt is negligible.
std::string seasonName(double season, double latRad, double axialTilt);

// unit: body-frame unit vector. detailMeters: smallest feature scale to include.
SurfaceSample sampleSurface(const BodyGen& g, const Vec3& unit, double detailMeters);
// Static cloud pattern (0..1) at lon/lat; animate by offsetting lon.
double sampleCloudPattern(const BodyGen& g, double lon, double lat);

struct PlanetMap {
    static constexpr int W = 512, H = 256;   // M9-15 (was 256x128)
    std::vector<float> height;
    std::vector<uint8_t> material;
    std::vector<uint8_t> albedo;   // 0..255
    std::vector<uint8_t> cloud;    // 0..255
    std::vector<uint8_t> veg;
    std::vector<uint8_t> wtint, ltint;   // the water's trait colours (`SurfaceSample::waterTint`, `lifeTint`), 0..255; empty on a world with none
    uint64_t seed = 0;
    std::atomic<bool> valid{false};     // set when a (possibly background) generation finished
    std::atomic<bool> generating{false};
    int seasonBucket = -1;
    PlanetMap() = default;
    PlanetMap(const PlanetMap&) = delete;
    PlanetMap& operator=(const PlanetMap&) = delete;
    void generate(const BodyGen& g);
    // the lookups inline (the globe reads seven a pixel; a call each from another file reloaded the data and wrapped the
    // longitude again)
    int texelIndex(double lon, double lat) const {   // nearest texel
        double fx = (wrap2pi(lon + PI) / TAU) * W;
        double fy = (0.5 - lat / PI) * H;
        int x = clampi((int)fx, 0, W - 1), y = clampi((int)fy, 0, H - 1);
        return y * W + x;
    }
    // bilinear albedo/cloud/height at lon/lat (radians)
    double albedoAt(double lon, double lat) const { return bilinear(albedo, lon, lat) / 255.0; }
    double cloudAt(double lon, double lat) const { return bilinear(cloud, lon, lat) / 255.0; }
    double heightAt(double lon, double lat) const { return bilinear(height, lon, lat); }
    int materialAt(double lon, double lat) const { return material[texelIndex(lon, lat)]; }
    double waterTintAt(int i) const { return wtint.empty() ? 0.0 : wtint[i] / 255.0; }   // (a texel's index)
    double lifeTintAt(int i) const { return ltint.empty() ? 0.0 : ltint[i] / 255.0; }

private:
    template <class T>
    static double bilinear(const std::vector<T>& v, double lon, double lat) {
        double fx = (wrap2pi(lon + PI) / TAU) * W - 0.5;
        double fy = (0.5 - lat / PI) * H - 0.5;
        int x0 = ifloor(fx), y0 = ifloor(fy);
        double tx = fx - x0, ty = fy - y0;
        int x1 = (x0 + 1) & (W - 1);
        x0 &= (W - 1);
        int y1 = clampi(y0 + 1, 0, H - 1);
        y0 = clampi(y0, 0, H - 1);
        double a = v[y0 * W + x0], b = v[y0 * W + x1];
        double c = v[y1 * W + x0], d = v[y1 * W + x1];
        return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
    }
};

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do

// an ice age's sheet (KT_ICE_SHEETS): the latitude (degrees) its edge reaches at a point, without the season (the ground is the
// world's); poleward of it the land lies under the sheet
double iceSheetEdgeDeg(const BodyGen& g, const Vec3& unit);
// O6-02: the relief spectrum alone at a point (metres, and the slope of the sum as rise over run), for tests
struct Relief { double h = 0, slope = 0; };
Relief reliefAt(const BodyGen& g, const Vec3& unit, double a0, double ridge, double detailMeters);
// F-01 (`bench noise`): one of the planet function's terms that walk the features' 27 cells (the round hills of a jittered grid on a
// shell round the sphere: 2 km cells, metres at p, km), for timing that walk and hashing its results
double benchFeatureField(const Vec3& p, double R, uint64_t seed);
