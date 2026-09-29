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

double galaxyDensity(double sx, double sy, double sz);   // 0..1, in sector units
// M9-13 regions of the galaxy: where a sector sits decides star classes, names and the backdrop.
enum GalaxyRegion { REGION_CORE = 0, REGION_BULGE, REGION_ARM, REGION_DISK, REGION_HALO, REGION_CLUSTER, REGION_NEBULA, REGION_COUNT };
extern const char* REGION_NAMES[REGION_COUNT];
int galaxyRegion(int64_t sx, int64_t sy, int64_t sz);
// The galaxy seen from inside: star density integrated along rays from an observer (sector
// units) into a lon x lat map normalised to 1; sampled bilinearly by direction (M1-09 / M10-14).
void buildGalaxyBand(const Vec3& obsSectors, std::vector<float>& map, int W, int H);
// N5-01: nebula patches of the star-forming regions around an observer (sector coordinates): a direction, an
// angular radius (radians), a tone (0 blue, 1 red, 2 white) and a strength; nebulaGlow is their glow 0..1 in a direction
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
