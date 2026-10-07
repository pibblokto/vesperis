// X-01 (2026-10-07): a probe's fall into a giant. The giant's air as the probe meets it (the temperature by the pressure,
// the height over the one-bar level from the scale height, the decks, the winds of the globe's bands) and the probe's
// clock: the fall from the ship, the entry, the descent and its end. Everything is a function of the giant and the
// probe's own clock (real seconds since the launch, the relay's clock: the time warp does not run it), so the screen,
// the save and the harness read the same numbers. X-02 renders the stages (the bands and the decks below), X-03 gives
// each giant its own decks, storms and glow (the numbers here are the plain giant's), X-04 keeps the record.
#pragma once
#include "system.h"
#include "planetmap.h"
#include <vector>
#include <cstdint>
#include <string>

inline bool isProbeGiant(int type) { return type == PT_GASGIANT || type == PT_SUBSTELLAR; }   // what a probe can fall into

// X-03 (2026-10-07): each giant's own character. Its kind (a gas giant, an ice giant: a small blue one, galaxy/system.h, or a
// brown dwarf), its internal heat (a young giant's high, a brown dwarf's its own glow), and from them its decks: what condenses
// where the giant's adiabat crosses each cloud's temperature (methane's 80 K, hydrogen sulphide's 125, ammonia's 150, ammonium
// hydrosulphide's 200, water's 275, the salts' 700), the deeper the more there is of it, so a warm giant's decks are high and
// few and a cold one's deep, with methane on top. The storms (the globe's share, BodyGen::stormShare), the lightning, the deep's
// glow, the polar aurorae (an ice giant's off the pole), the clear-air holes, the diamond hail of a carbon-rich giant and, on
// one rare giant, something seen once in the deep: hashed and derived, so no system changes
enum GiantKind { GK_GAS = 0, GK_ICE, GK_BROWN };
extern const char* const GIANT_KIND_NAMES[3];
enum CloudSpecies { CS_METHANE = 0, CS_H2S, CS_AMMONIA, CS_NH4SH, CS_WATER, CS_SALT, CS_COUNT };
struct CloudSpeciesInfo {
    const char* deckName;   // the screen's stage name
    const char* name;
    double condK;           // the cloud's base on the adiabat (the plain abundance)
    double span;            // its base's pressure over its top's
    double tau;             // its optical depth
    RGB color;              // its ice or drops, before the light's colour
    bool convective;        // towers and lightning (water, the salts)
};
extern const CloudSpeciesInfo CLOUD_SPECIES[CS_COUNT];
constexpr int GIANT_MAX_DECKS = 3;
struct GiantCharacter {
    int kind = GK_GAS;
    double heat = 0.3;           // the internal heat 0..1 (the star's youth, the giant's mass; a brown dwarf's own)
    bool young = false;          // a hot young giant (a young star's, its heat 0.62 and over)
    double abundance = 1;        // the condensables against the plain giant's: the decks' bases deeper with more
    double storms = 1;           // the towers' share against the plain giant's (the globe's storm share, the heat)
    double lightning = 1;        // the flashes' rate against the plain giant's (0 without a convective deck)
    double glow = 0;             // the deep's own light under the lowest deck, against the sunlight over the clouds
    RGB glowCol = RGB(0.75f, 0.22f, 0.08f);
    double aurora = 0;           // the polar aurorae (0..1)
    double magColat = 0.1, magLon = 0, ovalRad = 0.3;   // the magnetic pole's colatitude and longitude, the oval's angular radius (rad)
    RGB auroraLow = RGB(0.85f, 0.25f, 0.65f), auroraHigh = RGB(0.45f, 0.2f, 0.85f);
    double holes = 0;            // the clear-air holes: the share of cells of the upper decks holding one (0 none)
    bool hail = false;           // diamond hail in the deep (a carbon-rich giant)
    bool sight = false;          // the rare giant: something seen once in the deep, never explained
    bool greatStorm = false;     // the globe's great storm (BodyGen::gsOn)
};

struct GiantAtmosphere {
    bool valid = false;
    double t1K = 210;            // the temperature at one bar: the equilibrium's 1.5 warmed by the giant's heat, 60 K at least
    double gravity = 24;         // m/s^2
    double tropoBar = 0.15;      // the tropopause: the coldest level, the adiabat below it, a warming stratosphere above
    double hazeBar = 0.25;       // the upper haze's top
    int decks = 2;               // the decks within the probe's reach, from the top down (X-03: one to three)
    int species[GIANT_MAX_DECKS] = {CS_AMMONIA, CS_WATER, CS_WATER};
    double top[GIANT_MAX_DECKS] = {0.6, 4.0, 4.0}, base[GIANT_MAX_DECKS] = {1.6, 9.0, 9.0};   // bar
    double windPeak = 150;       // m/s at one bar on the strongest band
    double bandCount = 8;        // the globe's bands (`BodyGen::bandCount`)
    double driftSign = 1;        // the sense the globe's bands drift with (the turn's: a retrograde giant's bands run the other way)
    double heatK = 750;          // the heat that ends a probe
    GiantCharacter ch;
};
GiantCharacter giantCharacterOf(const StarSystem& sys, int bodyIndex);
GiantAtmosphere giantAtmosphereOf(const StarSystem& sys, int bodyIndex);
double giantTemperatureK(const GiantAtmosphere& a, double bar);
double giantAltitudeKm(const GiantAtmosphere& a, double bar);          // over the one-bar level (below it negative)
double giantWindEast(const GiantAtmosphere& a, double latRad, double bar);   // m/s toward the east (negative: toward the west)
// the deck a pressure lies in (-1 none), the deck the probe last left above it (-1 none), and the convective deck nearest under
// a pressure (or holding it; -1 none)
int giantDeckAt(const GiantAtmosphere& a, double bar);
int giantLayerAt(const GiantAtmosphere& a, double bar);   // in order down the fall: 0 over the haze, 1 the haze, 2 + 2k in deck k, 3 + 2k under it
int giantDeckAbove(const GiantAtmosphere& a, double bar);
int giantStormDeckFrom(const GiantAtmosphere& a, double bar);
// X-03: the giant's character as lines for the harness (the first: its kind and heat, then its decks, its weather, its rare features)
std::string giantCharacterText(const GiantAtmosphere& a);

// the stages of a fall, in order
// (X-03: PS_DECK1 is the first deck, PS_DECK2 any deck under it, PS_BETWEEN the clear air between two; `giantStageName` names
// a deck by its cloud)
enum ProbeStage { PS_FALL = 0, PS_ENTRY, PS_ABOVE, PS_HAZE, PS_DECK1, PS_BETWEEN, PS_DECK2, PS_DEEP, PS_END, PS_COUNT };
extern const char* const PROBE_STAGE_NAMES[PS_COUNT];
int giantStageAt(const GiantAtmosphere& a, double bar);   // PS_ABOVE .. PS_DEEP
const char* giantStageName(const GiantAtmosphere& a, double bar);   // the screen's: the deck's own name in a deck

// the probe's clock. The fall from the ship takes PROBE_FALL_S, the entry PROBE_ENTRY_S (the pressure from a millionth of a
// bar to a thousandth); then the fourth root of the pressure grows evenly with the clock (q = P^0.25: an e-fold of pressure
// takes PROBE_TAU1 x P^0.25 seconds, longer in the denser air), a quarter as fast under the chute, which opens once and
// holds while the pressure grows CHUTE_SPAN-fold (or until it is cut); the end is the hull's pressure or the heat
constexpr double PROBE_FALL_S = 36, PROBE_ENTRY_S = 14;
constexpr double PROBE_TAU1 = 50;            // seconds an e-fold of pressure takes at one bar, falling free
constexpr double PROBE_CHUTE_SLOW = 4;       // the chute's quarter speed
constexpr double PROBE_CHUTE_SPAN = 4.5;     // the pressure grows this many times under the chute before it tears
constexpr double PROBE_TOP_BAR = 1e-3, PROBE_ENTRY_BAR = 1e-6;
struct ProbeFlight {
    double chuteOpen = -1, chuteClose = -1;  // clocks (-1: not opened); chuteClose is set when it opens (it tears) or is cut
    double crushBar = 25;                    // the hull's limit, drawn per probe (`probeCrushBar`)
};
double probeCrushBar(uint64_t seed);                       // 18-32 bar
double probeDescentStart();                                // the clock the descent starts at (the fall and the entry behind)
double probeBarAt(const ProbeFlight& f, double clock);     // the pressure at a clock (the entry's within it; the top's before)
double probeEndBar(const GiantAtmosphere& a, const ProbeFlight& f);     // where it ends: the hull's limit or the heat's, the shallower
double probeEndClock(const GiantAtmosphere& a, const ProbeFlight& f);   // the clock the end comes at
double probeChuteCloseAt(const ProbeFlight& f, double openClock);       // the clock the chute tears at when it opens at `openClock`
int probeStageAt(const GiantAtmosphere& a, const ProbeFlight& f, double clock);   // PS_FALL .. PS_END
// the light of the sun that reaches the depth (1 over the clouds, a few hundredths in the water deck, nothing deeper); before the
// reddening. `cut`: the decks a clear-air hole takes away over the probe (a bit each)
double giantSunlightAt(const GiantAtmosphere& a, double bar, unsigned cut = 0);
// X-02: the clear air's optical depth from the top down to a pressure (the decks' own are theirs): a little in the
// stratosphere, the upper haze over the first deck, the clear bands' haze, the deep's. The view's haze and the sunlight's share it
double giantHazeTau(const GiantAtmosphere& a, double bar);
// X-03: the deep's own glow come up to a pressure (against the sunlight over the clouds): the glow under the lowest deck,
// through each deck under the pressure in turn (`cut` as above)
double giantGlowAt(const GiantAtmosphere& a, double bar, unsigned cut = 0);

// X-02 (2026-10-07): the globe's jets, the turn of a giant's pattern at a latitude by a time (M9-10; space_view's globe and
// the descent read the same): a band drifts by 6% of the giant's turn times sin(lat x bands)
double giantJetTurn(const Body& b, double bandCount, double lat, double t);
// X-03: the turn at a place (body-frame latitude and longitude): the great storm turns whole with its own latitude's jet, its
// surroundings easing into it, so it keeps its shape while the bands round it shear
double giantJetAt(const Body& b, const BodyGen& g, double lat, double lon, double t);

// X-02: the giant's bands round a place as the globe paints them: the planet function (galaxy/planetmap) on a grid of the
// tangent plane (km east and north of the place, out to halfKm), at the jets' drift of a time, so the descent falls into
// the band and the storm the telescope aimed at: the albedo (zones bright, belts dark) and the storms' term. X-03: the great
// storm's oval in the plane (its centre, its east axis, its semi-axes), for its dome and its wall
struct GiantBands {
    double halfKm = 0; int n = 0;
    std::vector<float> alb, storm;
    bool gsOn = false; double gsX = 0, gsZ = 0, gsEx = 1, gsEz = 0, gsAKm = 1, gsBKm = 1, gsTone = 1;
    bool valid() const { return n > 1; }
    void at(double xKm, double zKm, double& albOut, double& stormOut) const;   // bilinear, held at the edge
    double greatStormAt(double xKm, double zKm, double* angle = nullptr) const;   // the oval's distance (1 its edge; 9 far or none), the angle round it
};
GiantBands giantBandsOf(const Body& b, const BodyGen& g, double lat, double lon, double t, double halfKm, int n);

// X-02: a quick gradient noise for the clouds (a 32-bit hash, single-precision corners; about [-0.75, 0.75] like gnoise2): the
// decks are sampled a hundred thousand times a frame at kilometre scales, where the planet function's double-precision
// noise buys nothing
float cloudNoise2(double x, double y, uint32_t seed);
float cloudNoise3(double x, double y, double z, uint32_t seed);

// X-02: the decks as fields round the aim point (x km east, z km north; heights in km over the one-bar level), from the top
// down (X-03: one to three, each its own cloud). A top is the band's lift (the first deck's: zones high, belts low, storms
// higher still, from the globe's albedo; the great storm's dome and its wall), cloud streets along the bands, billows (the
// folds of |noise|, finer octaves dropped under the sampling's spacing) and the towers (thunderheads, one at most in a cell
// of the deck's grid: more in the belts and round the storms, tall in a convective deck, as many as the giant's storms
// make); a base hangs in pouches. Inside a deck the density follows its top and base with a soft edge and a structure of
// layers and pockets. The relief scales with the scale height (`sh`, the plain giant's 25 km = 1). X-03: the clear-air holes
// through the decks over the last (hashed in cells, and the descent's own when it falls through one)
struct GiantDecks {
    struct Tower { bool on = false; double cx = 0, cz = 0, r = 0, h = 0; };
    struct Hole { bool on = false; double cx = 0, cz = 0, r = 0; };
    const GiantBands* bands = nullptr;
    uint64_t seed = 0;
    double sh = 1;                              // the relief's scale
    int n = 2;                                  // the decks
    int species[GIANT_MAX_DECKS] = {CS_AMMONIA, CS_WATER, CS_WATER};
    double lvTop[GIANT_MAX_DECKS] = {0, 0, 0}, lvBase[GIANT_MAX_DECKS] = {0, 0, 0};   // the decks' nominal top and base (km over one bar)
    double cell[GIANT_MAX_DECKS] = {52, 44, 44};   // the towers' grid (km)
    double storms = 1, calm = 1;                // the giant's towers against the plain giant's; its billows' share (an ice giant's smoother)
    double holes = 0, holeCell = 600;           // the hashed holes' share of cells, the cells (km)
    Hole path[GIANT_MAX_DECKS];                 // the descent's own hole in a deck (km in its frame; set by the caller)
    void init(const GiantAtmosphere& a, const GiantBands* b, uint64_t seed);
    bool convective(int k) const { return CLOUD_SPECIES[species[k]].convective; }
    double lift(int k, double x, double z) const;                                // the band's lift of a top
    double top(int k, double x, double z, double lodKm, double* low = nullptr) const;   // `low`: the top without its billows
    double base(int k, double x, double z, double lodKm, double* swell = nullptr) const;   // `swell`: the base without its pouches
    Tower towerOf(int k, int64_t ci, int64_t cj) const;
    double towerAt(int k, double x, double z, double* inside = nullptr) const;   // a tower's rise over the deck (km); `inside` 0..1 toward its axis
    double towerShadow(int k, double x, double y, double z, const Vec3& sun, double maxKm) const;   // 0 lit .. 1 in a tower's shadow
    double density(int k, double x, double y, double z, double topKm, double baseKm) const;   // per km, given the top and the base there
    double holeAt(int k, double x, double z) const;   // 0 the deck is there .. 1 a hole (its wall between); the last deck has none
    bool holeable(int k) const;   // a deck that can open: one over another not far under it (a hole is a shaft to the next deck, not a chasm)
    // which decks a probe at a height (km over one bar) is among, each deck's place under it given (`px`, `pz`): the deck under it (a
    // top to look down on; a deck's own top stays the floor through its upper half), the deck over it (an underside), the deck it is
    // in (a fog). A deck with a hole where the probe is is not over it: over its level or in its shaft (down to the next whole deck's
    // top) the floor is that deck, the hole in it. The view draws by it (X-02) and the profile names the layers by it (X-04)
    struct Layers { int floor = -1, ceil = -1, fog = -1; bool floorFar = false, skyOpen = false, inHole = false; };
    Layers layersAt(double z, const double* px, const double* pz) const;
};
// X-04: the layer a probe found itself in (giantLayerAt's numbers by the decks' fields where it was: 0 over the haze, 1 the haze,
// 2 + 2k in deck k, 3 + 2k under it, the last's the deep; GIANT_HOLE_LAYER + k in a clear-air hole through deck k) and its names
constexpr int GIANT_HOLE_LAYER = 8;
int giantFoundLayer(const GiantAtmosphere& a, const GiantDecks::Layers& L, double bar);
std::string giantLayerName(const GiantAtmosphere& a, int layer, bool brief);   // brief: 14 characters at most (a chart's column)

// X-04: the storm a probe falls into at its aim (body-frame latitude and longitude, the bands' time): 2 the great storm, 1 one of the
// globe's storm cells (`id` the cell's: a storm keeps it while it lives, so the explorer's name for it is found again), 0 none
int giantStormAt(const Body& b, const BodyGen& g, double lat, double lon, double t, uint64_t& id);
