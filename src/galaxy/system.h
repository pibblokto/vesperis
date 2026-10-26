// Planetary systems: bodies, orbits, spin. Positions are analytic functions of
// game time so the sky above a landing site is always consistent with where
// the bodies really are.
#pragma once
#include "starfield.h"
#include "kinds.h"
#include <memory>
#include <vector>

// Bumped whenever a generator changes what a star, system or surface looks like (M9-17).
// Star positions are integer hashes and never move; saves and guides record the version.
constexpr int GEN_VERSION = 16;   // what each bump changed: docs/decisions/generations.md

// P-00: the kinds of body are rows of KINDS (galaxy/kinds.h): the enum, what each kind is and how it is drawn, made and shown
// an ice giant is a small blue giant (a Neptune: under 45,000 km, its globe blue); its kind is derived, nothing generated moves (P-07: a gas
// giant's, not a made giant's: the azure giant is blue for another reason)
struct Body;
bool isIceGiant(const Body& b);
// P-01 (GEN 16): the kinds made from the old draws. `pickPlanetType` draws the first PT_DRAWN_COUNT kinds as generation 15 did and every
// draw of the system is made as the drawn kind's (the orbits, the moons and their number, the rings, the twins: every body keeps its index);
// then `kindOf` may make the world another kind: the first made kind (in id order) whose row has a source the world qualifies for and whose
// hash of the world's seed falls under that source's share. Never a world with a people (C-01's hash is read first: `peopleHash`)
struct KindFacts { int drawn = 0; double tempK = 0; int starClass = 0; uint32_t body = KB_PLANET; bool locked = false, sync = false; uint64_t seed = 0, starSeed = 0; int parent = -1; uint32_t moon = 0; bool paired = false; };   // locked: one face to its star (M5-08's: its day its year, a round orbit); sync: its day its year; P-02: starSeed the system's star's (a source `byStar` hashes it); P-03: parent the kind a moon's planet drew (-1 none); P-06: moon what a moon is among its planet's (`MoonFact`; 0 not a moon); P-07: paired a double planet's first half (no giant)
int kindOf(const KindFacts& f);
// E-07: whether a world's facts fit a made kind's source, its hash aside: `kindOf`'s test, and the harness's (`salt scan`); `giant`: the
// source's kind is a giant's, which a double planet's halves never take
bool kindSourceFits(const KindSource& S, bool giant, const KindFacts& f);
bool peopleHash(uint64_t seed);   // C-01: one world in eight of the kinds a people may live on had one (the hash `BodyGen::make` reads)
// P-04: whether a world of a kind had a people: C-01's hash on the felisian and desert worlds, a dead world's own (its row's share and salt:
// C-01's never fires on a made world, `kindOf` keeps those felisian)
bool peoplesHash(int type, uint64_t seed);
// D-05: a drifter's people. One in four of the frozen felisian and the desert drifters kept one, on the drifter itself (never on its moons), by a
// hash of its own (a frozen felisian's its row's salt, a desert's DRIFTER_PEOPLE_SALT): whether the drifter of that kind and seed did
constexpr double DRIFTER_PEOPLE_SHARE = 0.25;
constexpr uint64_t DRIFTER_PEOPLE_SALT = 0xD05DE5E70B0B0001ULL;
bool drifterPeoplesHash(int type, uint64_t seed);
RGB madeKindColour(int type, uint64_t seed);   // P-01: a made kind's colour from afar (a world the harness stages a kind on, P-04)

struct Body {
    int index = 0, parent = -1;   // parent -1 => planet, else index of parent planet
    int type = PT_CRATERED;
    int drawnType = -1;           // P-01 (GEN 16): the kind `pickPlanetType` drew, whose draws the system made (`kindOf` may have made it `type`); -1 the same
    double drawnOrbitKm = -1;     // a moon's orbit as drawn where a made world pushed it out (the companion keeps clear of the drawn orbits); -1 the same
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
    double thenTempK = -1;        // a dead world's in its people's time (its then-world's climate: `BodyGen::make`); -1 its own
    int moonCount = 0;
    RGB color;                    // ramp colour seen from space
    Vec3 spinAxis, ref0, ref1;    // spin frame (ref0/ref1 perpendicular to the axis)
    // eccentric orbits: orbitRadiusKm is the semi-major axis, orbitPhase0 the mean anomaly at t = 0
    double ecc = 0, argPeri = 0;
    // M5-01/02: companion stars and substellar objects shine (relative to the yellow star)
    double luminosity = 0;
    int starClass = -1;           // companion stars: their StarClass
    bool doublePlanet = false;    // one half of a double planet (both halves carry the flag)
    bool locked = false;          // one face toward its star; lockedDir is the sub-stellar direction in the body frame
    Vec3 lockedDir;
    bool glassed = false;         // a survivor of the supernova that left its neutron star: the surface fused to glass (`BodyGen::make` puts TR_GLASSED first)
    bool wanderer = false;        // a rogue world the star caught (the last body, `generate`'s own step): no people ever lived on it
    bool drifter = false;         // a body of a drifter's system (the drifter on the anchor and its moons): no people lived on it (D-05's are the drifters' own)
    bool former = false;          // a drifter at its former orbit round the star it left, and its moons (`formerSystemOf`): what its people's readers read
    double heatK = 0;             // the world's own warmth (K), added to the star's in quadrature (`tempK`): a drifter's by its kind, its moons' and their tides'; 0 everywhere else
    Vec3 apexDir;                 // a two-faced moon (which faces its planet): the direction it travels in its body frame (its leading face's middle, the apex); zero otherwise
    double tideForcingM = 0;      // the equilibrium tide its moons and its sun raise (metres, the game's scale: `tideForcingM`), set once the system is made
    // the sector of the star the body was generated round (`StarSystem::generate` sets it): a world's planet function alone can then find
    // its system again (`BodyGen::starSx..`, `historyOfWorld` in peoples/history.h: the history needs the star, the comets, the moons)
    int64_t starSx = 0, starSy = 0, starSz = 0;
    bool starKnown = false;
};
inline int drawnKind(const Body& b) { return b.drawnType >= 0 ? b.drawnType : b.type; }   // P-01: the kind whose draws the system made
// D-04: a body in the dark (a drifter's system) colder than its air's freezing point (`KindDef::airFreezeK`) has its air on its ground as snow
// (TR_FROZEN_AIR): a trace of pressure, a black sky, no wind and no weather. Every reader of whether a body has an air reads `bodyHasAir`
inline bool airFrozen(const Body& b) { const KindDef& K = KINDS[b.type]; return b.drifter && K.landable && K.atmosphere && K.airFreezeK > 0 && b.tempK < K.airFreezeK; }
inline bool bodyHasAir(const Body& b) { return KINDS[b.type].atmosphere && !airFrozen(b); }

// M9-12 asteroid belt in an orbit gap. O3 (R-302): the belt is a place the Stardrifter can fly to; its rocks are
// hashed in co-rotating cells (`beltRockAt`) so the same rock is found again whatever the time.
struct Belt {
    double innerKm = 0, outerKm = 0;
    double period = 1;        // s, at the middle radius
    double phase0 = 0;
    std::string name;
    // the angular rate at radius r follows the sparkle band's Kepler shear (inner edge runs at TAU/period)
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
    // a drifter's system (STAR_ROGUE): no sun. Its light is 0 everywhere (`lightAt`), its worlds have no season, and on the ground the
    // sun stands at the nadir with no light (`SurfaceSite::sun`), so every reader of the sun sees a night with no moon
    bool sunless() const { return star.cls == STAR_ROGUE; }
    // Absolute galactic position of body i at time t (seconds).
    Vec3 bodyPos(int i, double t) const;
    // orbital velocity in km/s (central difference over a small fraction of the period) and
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
    // "S00 YELLOW STAR", or "S00+S01 MULTIPLE" when a companion star exists
    std::string classString() const;
    // the companion as a Star (for the sun renderer); valid only when companion >= 0
    Star companionStar() const;
    // the light reaching body i: the brighter of the primary and the companion, its direction and factor
    struct Light { Vec3 dir; double factor; bool fromCompanion; };
    Light lightAt(const Vec3& posKm, double t) const;
    double outerOrbitKm() const;   // the widest planetary orbit (comets excluded)
    // the rocks of belt k live in rings of BELT_CELL_KM (radius index ir); each ring turns at its own rate, so the
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
    // the ring's radial density profile (256 samples from the inner to the outer edge), shared by the globe,
    // the sky arc and the ring shadow on the ground
    std::vector<float> ringProfileOf(int bi) const;
    // perceptual light factor of a star of luminosity L at distKm: (L (AU/d)^2)^0.25 clamped to 0.35..1.15 (B-425: `floor` 0 for the far sun of
    // a multiple, which lights only as much as it does)
    static double starLightFactor(double luminosity, double distKm, double floor = 0.35);
    // body i circles the companion (it is one of the companion's worlds, or a moon of one): its own sun is the companion, the primary its far one
    bool ofCompanion(int i) const;
    // (E - e sin E = M) solved by Newton; returns the true anomaly, r/a in rOverA
    static double trueAnomaly(double meanAnomaly, double ecc, double& rOverA);
};

// O4/M5-07: a comet's activity at distKm from its star (0..1, the square of a reference orbit over the distance): the tail and
// the coma seen from space, the vents and the coma on the ground, and W-04's showers (the stream it sheds) read it
double cometActivityAt(const StarSystem& sys, double distKm);
double cometRefKm(const StarSystem& sys);   // its reference orbit: the first planet's (half an AU at least), an AU without planets

// R-402: a world's magnetic field (0..1), hashed from its seed and biased by its spin and size (the dynamo of a big, fast-spinning
// world; a locked world has little): a derived property, so no system changes. Classes: 0 none, 1 weak, 2 moderate, 3 strong
double magneticField(const Body& b);
int magneticClass(double mag);
extern const char* const MAGNETIC_CLASS_NAMES[4];
// R-402: the star's weather, 0..1: storms over days with substorms over hours; and a world's auroral potential (0..1 before the
// latitude, the darkness and the night's own variation): the star's activity by class, the steady glow a strong field keeps
// and what the storm drives. `auroraPotentialAt` takes the storm level itself (0 a quiet night, 1 a great storm)
double auroralStorm(const Star& s, double t);
// S-02: the star's activity by class (0..1), the one table the aurora reads (R-402, S-01): a star of 0.7 and over is "active"
// (the pulsar, the blue giant, the blue-white star, the red dwarf with its flares, the neutron star)
double starActivity(int cls);
double auroraPotentialAt(const StarSystem& sys, const Body& b, double storm);
// Q-02: the tides (TR_GREAT_TIDES). A world's tidal forcing: the equilibrium tide (m / M) R^4 / d^3 in metres that its moons and its sun raise,
// summed over the bodies whose pull turns over it (none from a body its face keeps toward it: a twin's, a locked world's sun, a facing moon's
// planet); on the game's scale it is large (its orbits are close: a moon at 14 radii, an AU of 22 million km), so the trait's coastal spring
// range is a compression of it (5 m at 300 m of forcing, 4 m more each tenfold, 16 at most); `tidePulls` the bodies and their shares of
// the forcing (-1 the star), `tideOffsetM` the sea's offset from its mean at a point (body frame) and a time: each pull's P2 bulge less its
// mean over the day at that latitude, so the range is the trait's at the equator under a pull in the equator's plane, none at a pole
struct TidePull { int body = -1; double share = 0, forcingM = 0; };
int tidePulls(const StarSystem& sys, int i, TidePull* out, int maxOut);
double tideForcingM(const StarSystem& sys, int i);
double tideRangeOf(double forcingM);
constexpr double TIDE_FORCING_MIN = 300;   // the trait's forcing at the least (37% of the scan region's sea worlds)
double tideOffsetM(const StarSystem& sys, int i, const Vec3& unitBody, double t, double rangeM);
double auroraPotential(const StarSystem& sys, const Body& b, double t);

// D-05: the people who lost their sun. A drifter with a people (`drifterHadPeople`: the drifter itself, a frozen felisian or a desert, by
// `drifterPeoplesHash`) lived in the system it left: `formerSystemOf` is that star's system as `generate` makes it (the star D-01's way back
// reaches, `drifterPast`), with the drifter at its former orbit in the system's temperate band appended after its own bodies (the orbit where
// the star's light gave its then-world's warmth, `Body::thenTempK`: its year; its eccentricity and its spin its own) and the moons that came
// with it after it (`Body::former` on all of them); null when no star was found (none within 3,000 ly of home). Cached by the drifter (the
// last eight). Every reader of a people's world (the profile, the lore and the calendar, the history, the records and their world's facts,
// the charts) reads it through `peoplesWorldOf`: a drifter's people's world at its former orbit, every other world as it is; `hold` keeps
// the former system alive while the reader holds the pointers
inline bool drifterHadPeople(const Body& b) { return b.drifter && b.parent < 0 && drifterPeoplesHash(b.type, b.seed); }
std::shared_ptr<const StarSystem> formerSystemOf(const StarSystem& dark);
struct PeoplesWorld { const StarSystem* sys = nullptr; const Body* b = nullptr; std::shared_ptr<const StarSystem> hold; };
PeoplesWorld peoplesWorldOf(const StarSystem& sys, const Body& b);

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do
// `salt scan`'s: while set (this thread's), `kindOf` appends to it each world it reads that may be made (a people's world never)
extern thread_local std::vector<KindFacts>* t_kindFactsSeen;
