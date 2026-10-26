#pragma once
// E-08: SurfaceView's helper types (surface_view.h holds the class): the explorer (`Player`), the vehicles (`Buggy`, `Drone`),
// the environment at the place and the nearest front's line, the flocks and the creatures, the trees and the logs, and a
// settlement in the view's metres (`Ruin`)
#include "site.h"
#include "galaxy/fronts.h"
#include "peoples/ruins.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

struct Player {
    double x = 0, z = 0;      // local metres (east, north)
    double y = 0;             // feet height (metres, reference level)
    double yaw = 0, pitch = 0;
    double vx = 0, vz = 0, vy = 0;
    bool onGround = false;
    bool swimming = false;
    double eyeHeight = 1.65;
    double speedMul = 1;
    // M8 movement state
    double stamina = 100;     // 0..100
    bool sprinting = false;
    double sprintRamp = 0;    // 0..1 acceleration into the sprint
    bool winded = false;
    double stridePhase = 0, bobY = 0, bobX = 0, landDip = 0;
    bool crouch = false, hindLegs = false;
    int autoWalk = 0;         // 0 off, 1..9 held forward speed preset
    double jetHeat = 0;       // 0..100
    bool jetOn = false;
    double altAboveGround = 0;
    bool diving = false, underwater = false;
    double swimStamina = 100;
    double lastVy = 0;
    double fovKick = 0;       // degrees added while sprinting
};

// M8-06: a small buggy carried by the capsule; N4 rebuilt it, N4-06 (R-204) closed the hull: the driver sees the
// world through the camera pod on the nose (`SurfaceView::render`, `cameraFeed`), never a cabin
struct Buggy {
    bool deployed = false;
    double x = 0, z = 0, y = 0, heading = 0, speed = 0, steer = 0, vy = 0, pitch = 0, roll = 0;
    double wheelSpin = 0, odometer = 0, vibration = 0, unfold = 0;
    bool airborne = false, lights = false;
    struct Track { float x, z; };
    std::vector<Track> tracks;
    int trackHead = 0;
    double trackAccum = 0;
    struct Puff { float x, y, z, age; uint8_t kind = 0; };   // N4-04: 0 dust, 1 snow, 2 mud
    std::vector<Puff> puffs;
    // N4
    double wheelY[4] = {0, 0, 0, 0};   // suspension travel per wheel (FL? no: 0 rear-left, 1 front-left, 2 rear-right, 3 front-right), metres
    bool brake = false;                // braking or handbrake (tail lights)
    double skid = 0;                   // 0..1 sideways slide (sound)
    double dust = 0;                   // 0..1 dirt on the body
    double topSpeed = 0;               // m/s, this deployment
    int gear = 0;                      // 0..2 engine pitch step
    double airT = 0;                   // seconds airborne
    double groundY = -1e9;             // B-307: the ground contact height of the previous frame (the slope's vertical speed)
    double thud = 0;                   // >0: a landing or collision thump for the synth (consumed by the game)
    double bumpT = 0;                  // seconds until the next small-rock bump can register
    double jolt = 0;                   // N4-06: a decaying dip of the nose camera after a thump (smooth, never random)
};

// R-403 (2026-10-02): the drone. A second vehicle carried by the capsule, built like the buggy (one closed hull with the
// sensor band, the camera pod on the nose, the mast; four ducted thrust pods on arms instead of wheels, two skids) and
// flown through its nose camera (`SurfaceView::render`, `cameraFeed`): `F` unfolds it at the capsule, `E` gets in, Space
// lifts it, W/S thrust fore and aft to 320 km/h, A/D turn (the hull banks), Space/Shift climb and descend, Shift onto the
// ground lands it; the camera's gimbal keeps the picture level. The ruins are hours away by buggy and minutes by air
struct Drone {
    bool deployed = false;
    double x = 0, z = 0, y = 0, heading = 0, speed = 0, vy = 0, pitch = 0, roll = 0, steer = 0;
    double odometer = 0, vibration = 0, unfold = 0, topSpeed = 0, flightT = 0;
    bool landed = true, lights = false, ceiling = false;
    double altAboveGround = 0;   // metres from the skids to the ground (or the water) below
    double rotor = 0;            // 0..1 the pods' spin (idle on the ground, full in a climb at speed): the sound and the fans' blur
    double fanSpin = 0;          // radians, the drawn fans
    double thud = 0, jolt = 0;   // a landing or a collision for the synth and a dip of the camera (consumed by the game, as the buggy's)
    double bumpT = 0;
    std::vector<Buggy::Puff> puffs;   // the downwash's dust, snow or spray when hovering low
    double puffAccum = 0;
};

struct SurfaceEnvironment {
    SunInfo sun;                // the light used for shading: the primary, or both suns blended (M5-01)
    SunInfo sunDisc;            // the primary star as seen (its disc, flares, eclipses)
    SunInfo sun2;               // M5-01: the companion star, when hasSun2
    bool hasSun2 = false;
    double sun2Share = 0;       // 0..1 share of the light coming from the companion
    double glow = 1;            // B-425: the blended light's glow in the sky, 0..1 of its own sun's (a far sun alone up makes a faint one)
    double primaryRel = 1;      // B-425: the primary's light beside the world's own sun, 0..1 (1 but on a world of the companion: the primary is its far sun)
    RGB lightColor;             // colour of the blended light
    double skyBrightness = 0;   // 0..1
    double temperatureC = 0;
    double pressureAtm = 0;
    double windKnots = 0;
    double windDir = 0;         // radians
    double rain = 0;            // 0..1
    double cloudCover = 0;
    double fogDistance = 5000;
    double localTime = 0.5;     // 0..1
    double latDeg = 0, lonDeg = 0;
    double altitude = 0;        // metres above reference
    double capsuleDist = 0, capsuleBearing = 0;
    bool nearCapsule = false;
    // M10-09: the brightest body above the horizon at night (planetshine / moonlight)
    Vec3 moonDir;               // local ENU
    double moonLight = 0;       // 0..0.5 relative to full sunlight
    RGB moonColor;
    int moonBody = -1;
    // M4-06 weather
    double snow = 0;            // 0..1 falling snow
    double dust = 0;            // 0..1 dust storm (thin-atmosphere worlds)
    double fogBank = 0;         // 0..1 fog patch
    double hail = 0;            // 0..1
    double aurora = 0;          // 0..1 strength of the aurora tonight
    double auroraStorm = 0;     // R-402: the star's storm tonight, 0..1
    double flare = 0;           // S-01: a red dwarf's flare now, 0..1 (the exposure opens past its clamp by it)
    double ringShadow = 0;      // O0-01: the world's ring between the site and the sun (0..0.85)
    double cometActivity = 0;   // O4: on a comet, how hard the nucleus vents (0 far from the star .. 1 at periapsis)
    // W-03: the fronts at the place: the pattern's cloud alone, the fronts' cloud, their rain (or dust) and cold here, and the
    // nearest front's line (metres ahead of the place, positive while it has not come; 1e18 none within reach)
    double cloudPattern = 0, frontCloud = 0, frontRain = 0, frontCold = 0, frontAhead = 1e18;
    // Q-02 (TR_FOG_SEAS): the valley fog of the morning now (0..1) and its top (metres; -1e9 none)
    double fogSea = 0, fogSeaTop = -1e9;
};

// W-03: the nearest front's line in the site's own metres. `ahead(x, z)` is the distance ahead of its rain line (positive
// where the front has not come yet), so the sky's deck, the ground's cloud shadow, the wall of rain and the curtain under
// the deck are all read from one line, the same line the forecast and the almanac time
struct LocalFront {
    bool on = false;
    double nx = 0, nz = 0;               // the front's motion in local metres (unit)
    double x0 = 0, z0 = 0, ahead0 = 0;   // the player's place and the distance ahead there
    double width = 1, gain = 1, strength = 1;   // the band's scale, the life x lateral gain, the peak precipitation before the gain
    double tex = 1;                      // the band's texture at the place (the deck's outer parts thin by it; its core and the rain do not)
    bool dust = false;
    double rainVis = 1500;               // the visibility inside the band's rain (dust: 400 m)
    double ahead(double x, double z) const { return ahead0 + (x - x0) * nx + (z - z0) * nz; }
    double cloud(double x, double z) const { return on ? frontCloudAt(ahead(x, z) / width, tex) * gain : 0.0; }
    double veil(double x, double z) const { return on ? frontVeilProfile(ahead(x, z) / width) * gain : 0.0; }
    double precip(double x, double z) const { return on ? frontRainProfile(ahead(x, z) / width) * strength * gain : 0.0; }
    // the extinction of a ray between two distances ahead (the rain as a box from the line to its end, at three quarters
    // of the peak, less the rain already in the fog at the camera), per the ray's length
    double wall(double sCam, double sP, double dist, double rainCam) const {
        if (!on) return 0;
        double rho = 0.75 * strength * gain - rainCam;
        if (rho <= 0.01) return 0;
        double s0 = FRONT_RAIN_END * width, s1 = 0;
        double lo = std::max(std::min(sCam, sP), s0), hi = std::min(std::max(sCam, sP), s1);
        if (hi <= lo) return 0;
        double span = std::fabs(sP - sCam);
        double frac = span < 1 ? ((sCam >= s0 && sCam <= s1) ? 1.0 : 0.0) : (hi - lo) / span;
        return frac * dist * rho / rainVis;
    }
};

struct Flock {
    double cx = 0, cz = 0, alt = 60, radius = 80, phase = 0, speed = 0.1, drift = 0;
    int n = 10;
    uint64_t seed = 0;
    double cruiseAlt = 60, restTimer = 90, landed = 0;   // M4-05: flocks land and rest
    int species = -1;                                     // N3: the flyer species of the bestiary
    bool perched = false;                                 // N3: resting in a tree
    double perchX = 0, perchZ = 0, perchY = 0, perchR = 2;
};
// N3 creature behaviour states
enum CritterState { CS_IDLE = 0, CS_WALK, CS_GRAZE, CS_DRINK, CS_REST, CS_ALERT, CS_FLEE, CS_CURIOUS };
extern const char* const CRITTER_STATE_NAMES[8];
struct Critter {
    double x = 0, z = 0, heading = 0, speed = 0, timer = 0, size = 1;
    uint64_t seed = 0;
    int kind = 0;             // 0 grazer, 1 crawler (M4-05; now 1 for hexapods)
    int herd = 0;
    double fleeT = 0;
    // N3
    int species = -1;         // index into the bestiary
    int state = CS_IDLE;
    double gaitPhase = 0;     // stride cycles walked
    double headPitch = 0;     // 0 head up .. 1 grazing
    double lift = 0;          // 0 standing .. 1 lying
    double calledT = -100;    // time of the last call
    struct Track { float x, z; };
    std::vector<Track> tracks; int trackHead = 0; double trackAccum = 0;
};
// N2: one tree, placed by SurfaceView::forTrees (drawing, colliders, coverage and creatures read the same list)
struct TreeInst {
    double x = 0, z = 0, gy = 0;   // position and ground height (local metres)
    double h = 8, r = 3, cy = 5;   // height, canopy radius, canopy centre height above the ground
    double lx = 0, lz = 0;         // B-315: the lean: where the canopy centre sits relative to the foot (metres)
    int fam = 0;                   // silhouette: 0 dome, 1 cone, 2 umbrella, 3 tiered giant, 4 fibrous stalk, 5 fern tree, 6 mushroom tree, 7 weeping, 8 candelabra, 9 spire
    int bank = 3;                  // leaf bank: 3 forest, 5 second family / autumn, 10 straw variant
    bool dead = false, bare = false;   // a dead skeleton; deciduous in winter
    bool stump = false;            // P-04: a stone stump of a scorched world's petrified forest (a broken trunk, no branches)
    uint64_t seed = 0;
    double dens = 0;               // the canopy density field at its cell (1 = deep forest)
};
// B-315: how one silhouette looks on this planet (hashed from the seed and the family), so two worlds with dome trees
// do not share one tree: the canopy's proportions, the clusters, the trunk, the lean, the leaf edge, the tone
struct TreeLook {
    double aspect = 1;     // canopy width factor
    double clusterR = 0.5; // a cluster's radius as a fraction of the canopy radius
    double count = 1;      // cluster count factor
    double trunkW = 1;     // trunk thickness factor
    double lean = 0;       // radians, the trunk's tilt
    double ragged = 1.0;   // the rim noise amplitude (0.7 tidy .. 1.5 shaggy)
    double tone = 1;       // leaf brightness factor
    double droop = 0;      // 0..1, the clusters hang (weeping) or the fronds arch
};
struct LogInst { double x = 0, z = 0, heading = 0, len = 6, radius = 0.4; uint64_t seed = 0; bool stone = false; };   // Q-02: stone, a petrified forest's (its own bank)
// C-01: a settlement's cell (`peoples/ruins.*`) in the view's metres (B-417: M4-07's monoliths of the old ones are gone)
struct Ruin {
    double x, z, heading, size;
    const RuinSpec* spec = nullptr;   // the settlement's layout (the cell cache's)
    double reach = 0;                 // B-420: how far its pieces reach from its middle (its harbour, a fallen tower's top): past its radius at times
};
