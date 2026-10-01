// Planetary systems: bodies, orbits, spin. Positions are analytic functions of
// game time so the sky above a landing site is always consistent with where
// the bodies really are.
#pragma once
#include "starfield.h"
#include <vector>

// Bumped whenever a generator changes what a star, system or surface looks like (M9-17).
// Star positions are integer hashes and never move; saves and guides record the version.
constexpr int GEN_VERSION = 11;   // 11: G-01 the galaxy of 200 billion stars (`galaxyDensity`: every star's existence, class and name can change; the whole G/S series of 1.1.0 is this one bump); 10: O6-03..O6-06 drainage (rivers routed downhill by a flood of the ground, lakes at the fill levels of real basins, valleys carved to the rivers), the landform library (cliff bands, broken plains, scree, coasts by exposure, cirques and fjords, lava tongues, slip-face dunes, crater terraces) and the landmarks (heights, water and materials change on every solid surface; systems and star positions do not); 9: B-322 the ocean worlds' floes rise from a shelf sea through an ice foot, river and lake banks stand over their plane (heights change on ocean worlds and along felisian water; systems do not); 8: B-321 the felisian greenhouse (+33 K over the equilibrium temperature: biomes, ice caps and water move on every living world), B-320 rivers and lakes rebuilt, R-304/R-305 planet traits and landforms (every solid surface changes; systems and star positions do not); 7: B-314 the first moon at 4.5 radii and outside the rings (moon orbits and periods change; surfaces do not); 6: O6-02 the relief spectrum (every solid surface changes shape; systems and star positions do not); 5: O0-01 rings on 12% of solid planets; 4: N5-03 gas giant storm term; 3: M5 astronomy

enum PlanetType {
    PT_MOLTEN = 0,   // internally hot, lava lakes, no atmosphere
    PT_CRATERED,     // small, dusty, craterised, no atmosphere
    PT_VENUSIAN,     // thick atmosphere, fully covered by clouds
    PT_FELISIAN,     // breathable atmosphere, oceans, vegetation
    PT_ROCKY,        // creased rocky surface, boulders, no atmosphere
    PT_THINATMO,     // small, thin atmosphere, exotic colours
    PT_GASGIANT,     // large, not consistent, dense clouds; not landable
    PT_ICY,          // icy, fractured surface, no atmosphere
    PT_QUARTZ,       // milky quartz surface, oxygen atmosphere
    PT_OCEAN,        // M9-11: a global sea with ice floes, no land
    PT_METAL,        // M9-11: dark iron world, glinting, craterised
    PT_VOLCANIC,     // M9-11: tidally heated moon, sulphur plains, active cones
    PT_CARBON,       // M9-11: black graphite plains with diamond crusts
    PT_SUBSTELLAR,   // M5-02: a brown dwarf, glowing with its own heat, with a moon system; not landable
    PT_COMET,        // M5-07: a small icy nucleus on a long eccentric orbit, with a tail near the star; landable since O4 (R-303)
    PT_COMPANION,    // M5-01: a second sun bound to the primary, with worlds of its own
    PT_EUROPAN,      // R-307: an ice shell over a hidden ocean: lineae, chaos terrain, water geysers; no atmosphere
    PT_TECTONIC,     // R-307: a seismic world: rift valleys, fissures running with lava, lava fountains, quakes; a thin sulphurous sky
    PT_DESERT,       // R-307: a dry world under a dusty ochre sky: hamada, mesas, canyons, ergs, salt pans, dust devils
    PT_HYDROCARBON,  // R-307: a cold world under an opaque orange haze: methane seas, tholin dunes, drizzle
    PT_BOMBARDED,    // R-307: a young airless surface under a rain of meteorites: fresh craters, rays, strikes seen from the ground
    PT_ACIDIC,       // R-307: a corrosive world: bleached karst, sulphur crusts, seas of acid, a yellow-green sky that rains
    PT_COUNT
};

struct PlanetTypeInfo {
    const char* name;
    const char* description;
    double minRadiusKm, maxRadiusKm;
    bool landable;
    bool atmosphere;
    int maxMoons;
    double albedo;
};
extern const PlanetTypeInfo PLANET_TYPES[PT_COUNT];
// R-307: the type groups the renderers branch on
inline bool hasOpaqueDeck(int t) { return t == PT_VENUSIAN || t == PT_HYDROCARBON; }   // the globe and the sky are one cloud or haze deck; no sun disc, no cast shadows
inline bool isLavaWorld(int t) { return t == PT_MOLTEN || t == PT_VOLCANIC || t == PT_TECTONIC; }   // bank 2 is lava, the ground glows by it, eruptions
inline int landableTypeCount() { int n = 0; for (int i = 0; i < PT_COUNT; i++) if (PLANET_TYPES[i].landable) n++; return n; }

struct Body {
    int index = 0, parent = -1;   // parent -1 => planet, else index of parent planet
    int type = PT_CRATERED;
    uint64_t seed = 0;
    std::string name;
    double radiusKm = 3000;
    double massEarths = 1;
    double gravity = 9.8;         // m/s^2
    double orbitRadiusKm = 1e7;
    double orbitPeriod = 3600;    // s
    double orbitPhase0 = 0;
    double orbitIncl = 0;         // rad
    double orbitNode = 0;         // rad
    double rotPeriod = 3600;      // s (negative = retrograde)
    double rotPhase0 = 0;
    double axialTilt = 0;         // rad
    double axisAzimuth = 0;       // rad
    bool rings = false;
    double ringInner = 1.5, ringOuter = 2.5;   // in body radii
    double tempK = 280;           // equilibrium temperature (day side, sub-solar average)
    int moonCount = 0;
    RGB color;                    // ramp colour seen from space
    Vec3 spinAxis, ref0, ref1;    // spin frame (ref0/ref1 perpendicular to the axis)
    // M5-03 eccentric orbits: orbitRadiusKm is the semi-major axis, orbitPhase0 the mean anomaly at t = 0
    double ecc = 0, argPeri = 0;
    // M5-01/02: companion stars and substellar objects shine (relative to the yellow star)
    double luminosity = 0;
    int starClass = -1;           // companion stars: their StarClass
    bool doublePlanet = false;    // M5-09: one half of a double planet (both halves carry the flag)
    bool locked = false;          // M5-08: one face toward its star; lockedDir is the sub-stellar direction in the body frame
    Vec3 lockedDir;
};

// M9-12 asteroid belt in an orbit gap. O3 (R-302): the belt is a place the Stardrifter can fly to; its rocks are
// hashed in co-rotating cells (`beltRockAt`) so the same rock is found again whatever the time.
struct Belt {
    double innerKm = 0, outerKm = 0;
    double period = 1;        // s, at the middle radius
    double phase0 = 0;
    std::string name;
    // O3: the angular rate at radius r follows the sparkle band's Kepler shear (inner edge runs at TAU/period)
    double rateAt(double rKm) const { return TAU / period * std::pow(innerKm / std::max(rKm, 1.0), 1.5); }
};
// O3: one rock of a belt: cells of BELT_CELL_KM in co-rotating cylindrical space (radius, arc, height), up to
// BELT_ROCKS_PER_CELL rocks each; `beltRockAt` gives rock m of cell (ir, ia, iy) at time t, false when the cell has fewer
constexpr double BELT_CELL_KM = 400.0;
constexpr int BELT_ROCKS_PER_CELL = 3;
struct BeltRock {
    Vec3 pos;            // world km at the time asked for
    double radiusKm = 1;
    uint64_t seed = 0;
    double tumble = 0;   // radians, turning with time
    Vec3 axis;           // tumble axis
    int64_t ir = 0, ia = 0, iy = 0; int m = 0;   // identity
};

struct StarSystem {
    Star star;
    std::vector<Body> bodies;
    std::vector<Belt> belts;
    bool valid = false;
    int companion = -1;           // M5-01: index of the companion star body, -1 for a single star

    void generate(const Star& s);
    // Absolute galactic position of body i at time t (seconds).
    Vec3 bodyPos(int i, double t) const;
    // N0-01: orbital velocity in km/s (central difference over a small fraction of the period) and
    // the mean anomaly in 0..TAU (0 = periapsis, below PI = receding from the parent)
    Vec3 bodyVel(int i, double t) const;
    double meanAnomaly(int i, double t) const;
    Vec3 parentPos(int i, double t) const;   // star or parent planet position
    double rotationAngle(int i, double t) const;
    // World->body-frame rotation: rows (E0(t), E1(t), axis). Body-frame unit
    // vector (cos lat cos lon, cos lat sin lon, sin lat).
    Mat3 bodyFrame(int i, double t) const;
    Vec3 surfacePointWorld(int i, double t, double latRad, double lonRad, double altitudeKm) const;
    int planetCount() const { int n = 0; for (auto& b : bodies) if (b.parent < 0) n++; return n; }
    std::string bodyLabel(int i) const;
    // Season of body i at time t: +1 northern summer, -1 northern winter (scaled by the tilt), M9-09.
    double seasonOf(int i, double t) const;
    static void latLonFromBody(const Vec3& b, double& lat, double& lon);
    static Vec3 bodyFromLatLon(double lat, double lon);
    // M5-01: "S00 YELLOW STAR", or "S00+S01 MULTIPLE" when a companion star exists
    std::string classString() const;
    // M5-01: the companion as a Star (for the sun renderer); valid only when companion >= 0
    Star companionStar() const;
    // the light reaching body i: the brighter of the primary and the companion, its direction and factor
    struct Light { Vec3 dir; double factor; bool fromCompanion; };
    Light lightAt(const Vec3& posKm, double t) const;
    double outerOrbitKm() const;   // the widest planetary orbit (comets excluded)
    // O3: the rocks of belt k live in rings of BELT_CELL_KM (radius index ir); each ring turns at its own rate, so the
    // arc cell of a world angle depends on the ring (beltArcCell); beltCellOf gives a position's own ring, arc and
    // height cells; beltRockAt fills rock m of a cell (false when the cell holds fewer than m+1 rocks); forBeltRocksNear
    // visits every rock within a few cells of a position, ring by ring
    void beltCellOf(int k, const Vec3& posKm, double t, int64_t& ir, int64_t& ia, int64_t& iy) const;
    int64_t beltArcCell(int k, int64_t ir, double angWorld, double t) const;
    bool beltRockAt(int k, int64_t ir, int64_t ia, int64_t iy, int m, double t, BeltRock& out) const;
    template <class F> void forBeltRocksNear(int k, const Vec3& posKm, double t, int reachR, int reachA, int reachY, F fn) const {
        int64_t ir, ia, iy;
        beltCellOf(k, posKm, t, ir, ia, iy);
        Vec3 rel = posKm - star.pos;
        double ang = std::atan2(rel.z, rel.x);
        for (int dr = -reachR; dr <= reachR; dr++) {
            int64_t ic = beltArcCell(k, ir + dr, ang, t);
            for (int da = -reachA; da <= reachA; da++)
                for (int dy = -reachY; dy <= reachY; dy++)
                    for (int m = 0; m < BELT_ROCKS_PER_CELL; m++) { BeltRock rk; if (beltRockAt(k, ir + dr, ic + da, iy + dy, m, t, rk)) fn(rk); }
        }
    }
    // O0-01: the ring's radial density profile (256 samples from the inner to the outer edge), shared by the globe,
    // the sky arc and the ring shadow on the ground
    std::vector<float> ringProfileOf(int bi) const;
    // perceptual light factor of a star of luminosity L at distKm: (L (AU/d)^2)^0.25 clamped to 0.35..1.15
    static double starLightFactor(double luminosity, double distKm);
    // M5-03: (E - e sin E = M) solved by Newton; returns the true anomaly, r/a in rOverA
    static double trueAnomaly(double meanAnomaly, double ecc, double& rOverA);
};

// R-402: a world's magnetic field (0..1), hashed from its seed and biased by its spin and size (the dynamo of a big, fast-spinning
// world; a locked world has little): a derived property, so no system changes. Classes: 0 none, 1 weak, 2 moderate, 3 strong
double magneticField(const Body& b);
int magneticClass(double mag);
extern const char* const MAGNETIC_CLASS_NAMES[4];
// R-402: the star's weather, 0..1: storms over days with substorms over hours; and a world's auroral potential (0..1 before the
// latitude, the darkness and the night's own variation): the star's activity by class, the steady glow a strong field keeps
// and what the storm drives. `auroraPotentialAt` takes the storm level itself (0 a quiet night, 1 a great storm)
double auroralStorm(const Star& s, double t);
double auroraPotentialAt(const StarSystem& sys, const Body& b, double storm);
double auroraPotential(const StarSystem& sys, const Body& b, double t);
