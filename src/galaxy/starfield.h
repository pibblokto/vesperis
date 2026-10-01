// The galaxy: a hashed, infinite field of sectors. Each sector may hold one
// star whose position, class and seed are pure functions of the sector index.
#pragma once
#include "core/types.h"
#include "core/rng.h"
#include <cstdint>
#include <string>
#include <vector>

enum StarClass {
    STAR_YELLOW = 0,     // S00 medium yellow star, Sun-like
    STAR_ORANGE,         // S01 orange dwarf, small and warm
    STAR_BLUE_GIANT,     // S02 blue giant, huge, harsh light, wide systems
    STAR_RED_GIANT,      // S03 red giant, enormous dim red disc
    STAR_WHITE_DWARF,    // S04 white dwarf, tiny bright remnant, frozen worlds
    STAR_PULSAR,         // S05 pulsar, spinning neutron star, pulsing radiation
    STAR_CLASS_COUNT
};

struct StarClassInfo {
    const char* code;
    const char* name;
    const char* description;
    RGB color;             // colour of the light
    double radiusKm;       // typical radius
    double radiusVar;      // +- fraction
    double luminosity;     // relative to the yellow star
    double massFactor;     // relative to the yellow star (orbital periods)
    int maxPlanets;
    double rarity;         // relative weight
    double firstOrbitMult; // first orbit radius in star radii
    double minFirstOrbitKm;
};
extern const StarClassInfo STAR_CLASSES[STAR_CLASS_COUNT];

constexpr double SECTOR_KM = 1.5e10;         // one sector = one in-game "light year"
constexpr double AU_GAME_KM = 2.2e7;         // reference orbit used for temperature estimates

struct Star {
    int64_t sx = 0, sy = 0, sz = 0;
    Vec3 pos;              // galactic coordinates, km
    int cls = 0;
    double radiusKm = 7e5;
    double luminosity = 1;
    double massFactor = 1;
    RGB color;
    uint64_t seed = 0;
    std::string name;
    double pulseHz = 0;    // pulsars
    bool valid = false;
};

// G-01 (2026-10-01): a galaxy of about 200 billion stars with the Milky Way's proportions (`docs/ideas/PLAN-galaxy-scale.md`).
// Sector coordinates are heliocentric like the real galactic frame: the expedition's home sectors stay where they always
// were and the galactic centre sits at GALAXY_CENTRE (one sector = one light year, so the disc reaches 50,000 sectors).
constexpr double GALAXY_CENTRE_SX = 12000, GALAXY_CENTRE_SZ = -5500;   // home (sector 185, 0, 45) is 13,050 ly out, on an arm
// G-02: the home star, pinned (`vesperis_test home` lists the candidates of the new-game search with what the pick needs);
// Game::newGame falls back to the search of sectors 176..196 x 36..56 when a generation change takes it away
constexpr int64_t HOME_SX = 195, HOME_SY = 0, HOME_SZ = 46;
constexpr double GALAXY_DISC_SCALE = 8000;      // the exponential disc's scale length, ly
constexpr double GALAXY_DISC_PEAK = 0.68;       // the disc's midplane density at the centre, stars per sector
constexpr double GALAXY_THICK_FAR = 250, GALAXY_THICK_NEAR = 1200, GALAXY_THICK_SCALE = 4000;   // scale height far + near e^(-r/scale)
constexpr double GALAXY_BULGE_R = 2500, GALAXY_BULGE_PEAK = 0.9;   // a gaussian bulge, half as thick as it is wide
constexpr int GALAXY_ARMS = 4;                  // logarithmic arms of 12 degrees pitch: the next arm out is 1.4 times the radius away
constexpr double GALAXY_ARM_R0 = 1000;          // the arms' phase reference radius
constexpr double GALAXY_CAP = 0.97;             // at most one star per sector: the core is nearly solid stars
struct GalaxyTerms { double r, h, disk, bulge, arm; };   // radius from the centre, scale height, the disc and bulge densities, the arm factor 0..1
GalaxyTerms galaxyTerms(double sx, double sy, double sz);
// G-02: clusters. Globular clusters: about 150 in a halo distribution round the centre (a 2,000 ly cell holds one with
// probability 0.5 / (1 + (r / 6000)^2)^2), 30-100 ly across, old stars; open clusters: in the disc and mostly along the
// arms (a 100 ly cell holds one with probability 0.2 (0.4 + 0.6 arm) e^(-|y|/h) min(1, disc / 0.1)), 10-30 ly across,
// young stars. Both are Plummer-like knots with a core of half the radius (the cap at the centre, a fifth of it at the
// edge, about 1.2 R^3 stars: 1,000 in an open cluster 20 ly across, 100,000 in a globular of 90), so dense that the
// sky fills with stars inside one.
constexpr double GLOBULAR_CELL = 2000, OPEN_CLUSTER_CELL = 100;
bool globularInCell(int64_t cx, int64_t cy, int64_t cz, Vec3& centre, double& radius);      // cells relative to the galactic centre; the centre in sector coordinates
bool openClusterInCell(int64_t cx, int64_t cy, int64_t cz, Vec3& centre, double& radius);
int clusterAt(double sx, double sy, double sz, double& dens);   // 0 none, 1 open, 2 globular; dens: the cluster's density at the point
double galaxyDensity(double sx, double sy, double sz);   // 0..1, in sector units
// M9-13 regions of the galaxy: where a sector sits decides star classes, names and the backdrop.
enum GalaxyRegion { REGION_CORE = 0, REGION_BULGE, REGION_ARM, REGION_DISK, REGION_HALO, REGION_CLUSTER, REGION_NEBULA, REGION_OPEN, REGION_COUNT };   // G-02: REGION_OPEN, an open cluster of the disc
extern const char* REGION_NAMES[REGION_COUNT];
int galaxyRegion(int64_t sx, int64_t sy, int64_t sz);
// G-03: the galaxy seen from inside. `galaxyColumn` integrates the light of the unresolved stars along a direction from
// an observer (sector units) behind the dust: the column of stars seen through the extinction in front of each of them
// (`tau`: the optical depth of the whole path). `buildGalaxyBand` fills a lon x lat map (BAND_MAP_W x BAND_MAP_H) with
// the brightness `(column / BAND_EXPOSURE)^BAND_GAMMA`, soft-clipped at 1, on a fixed scale, so the same place always
// looks the same: from home the bulge reads ~0.9 beside the rift's 0.65, the plane 0.35-0.45, the poles 0.05; inside
// the core the sky saturates, from the halo the disc is a faint whirl round a bright centre. Sampled bilinearly by
// direction (M1-09 / M10-14).
constexpr int BAND_MAP_W = 128, BAND_MAP_H = 64;   // 2.8 degrees per cell: the band's cusp and the rifts of the near dust show, the far field is smooth anyway
constexpr double GALAXY_DUST = 1.0e-3;   // the dust's extinction per light year at the disc's centre in the plane: a thousandth at home (the real disc's magnitude per kiloparsec), four to the core, one to the anticentre
constexpr double BAND_EXPOSURE = 2600, BAND_GAMMA = 0.7;   // the column that reads 0.84: the bulge from home is 3,000, the anticentre 560, the pole 44
double galaxyColumn(const Vec3& obsSectors, const Vec3& dir, double& tau);
double bandToneOf(double column);   // the brightness 0..1 of a column
void buildGalaxyBand(const Vec3& obsSectors, std::vector<float>& map, int W, int H);
// N5-01: nebula patches of the star-forming regions around an observer (sector coordinates): a direction, an
// angular radius (radians), a tone (0 blue, 1 red, 2 white) and a strength; nebulaGlow is their glow 0..1 in a direction.
// G-03: the patches belong to star-forming complexes, one in a NEBULA_CELL ly column of the arms with probability
// NEBULA_RATE, its 3-6 patches within NEBULA_RADIUS ly of a centre near the plane; the complex is the STAR-FORMING
// REGION of `galaxyRegion` (within NEBULA_RADIUS of the centre) and its patches fade out at NEBULA_SEEN ly (the 14 ly
// cells at 15% of N5-01 put three to six patches in nearly every sky of the arm).
constexpr double NEBULA_CELL = 42, NEBULA_RATE = 0.12, NEBULA_RADIUS = 12, NEBULA_SEEN = 42;
bool nebulaInCell(int64_t cx, int64_t cz, Vec3& centre);   // the complex of a cell (sector units), if the cell holds one
struct NebulaPatch { Vec3 dir; double radius = 0.1; int tone = 2; double inten = 0.5; };
int nebulaPatches(const Vec3& obsSectors, NebulaPatch* out, int maxN);
double nebulaGlow(const NebulaPatch* p, int n, const Vec3& dir, int& tone);
double sampleGalaxyBand(const std::vector<float>& map, int W, int H, const Vec3& dir);
bool starInSector(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName = true);

// Cache of the stars around a position (a cube of sectors), refreshed when the
// centre sector changes.
struct StarNeighborhood {
    int64_t cx = 1 << 30, cy = 0, cz = 0;
    int radius = 10;
    std::vector<Star> stars;
    void update(const Vec3& posKm);
    // nearest star to a position (or nullptr)
    const Star* nearest(const Vec3& posKm, double* distOut = nullptr) const;
};
std::string generateName(uint64_t seed, int style = 0);   // style 0 plain, 1 hard (core/halo), 2 flowing (arms/nebulae)
std::string romanNumeral(int n);
inline uint64_t sectorSeed(int64_t sx, int64_t sy, int64_t sz) { return hash3i(sx, sy, sz, 0x5EED5EEDULL); }
inline int64_t sectorOf(double km) { return (int64_t)std::floor(km / SECTOR_KM); }
