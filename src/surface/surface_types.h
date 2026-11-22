#pragma once
// E-08: SurfaceView's helper types (surface_view.h holds the class): the explorer (`Player`), the vehicles (`Buggy`, `Drone`),
// the environment at the place and the nearest front's line, the flocks and the creatures, the trees and the logs, and a
// settlement in the view's metres (`Ruin`)
#include "site.h"
#include "animal_mesh.h"
#include "galaxy/fronts.h"
#include "peoples/ruins.h"
#include "life/samples.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
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
    // movement state
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
    struct Puff { float x, y, z, age; uint8_t kind = 0; };   // 0 dust, 1 snow, 2 mud
    std::vector<Puff> puffs;
    double wheelY[4] = {0, 0, 0, 0};   // suspension travel per wheel (FL? no: 0 rear-left, 1 front-left, 2 rear-right, 3 front-right), metres
    bool brake = false;                // braking or handbrake (tail lights)
    double skid = 0;                   // 0..1 sideways slide (sound)
    double dust = 0;                   // 0..1 dirt on the body
    double topSpeed = 0;               // m/s, this deployment
    int gear = 0;                      // 0..2 engine pitch step
    double airT = 0;                   // seconds airborne
    double groundY = -1e9;             // the ground contact height of the previous frame (the slope's vertical speed)
    double thud = 0;                   // >0: a landing or collision thump for the synth (consumed by the game)
    double bumpT = 0;                  // seconds until the next small-rock bump can register
    double jolt = 0;                   // a decaying dip of the nose camera after a thump (smooth, never random)
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
    SunInfo sun2;               // the companion star, when hasSun2
    bool hasSun2 = false;
    double sun2Share = 0;       // 0..1 share of the light coming from the companion
    double glow = 1;            // the blended light's glow in the sky, 0..1 of its own sun's (a far sun alone up makes a faint one)
    double primaryRel = 1;      // the primary's light beside the world's own sun, 0..1 (1 but on a world of the companion: the primary is its far sun)
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
    // weather
    double snow = 0;            // 0..1 falling snow
    double dust = 0;            // 0..1 dust storm (thin-atmosphere worlds)
    double fogBank = 0;         // 0..1 fog patch
    double hail = 0;            // 0..1
    double aurora = 0;          // 0..1 strength of the aurora tonight
    double auroraStorm = 0;     // the star's storm tonight, 0..1
    double flare = 0;           // a red dwarf's flare now, 0..1 (the exposure opens past its clamp by it)
    double ringShadow = 0;      // the world's ring between the site and the sun (0..0.85)
    double cometActivity = 0;   // on a comet, how hard the nucleus vents (0 far from the star .. 1 at periapsis)
    // the fronts at the place: the pattern's cloud alone, the fronts' cloud, their rain (or dust) and cold here, and the
    // nearest front's line (metres ahead of the place, positive while it has not come; 1e18 none within reach)
    double cloudPattern = 0, frontCloud = 0, frontRain = 0, frontCold = 0, frontAhead = 1e18;
    // TR_FOG_SEAS: the valley fog of the morning now (0..1) and its top (metres; -1e9 none)
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
    int species = -1;                                     // the flyer species of the bestiary
    bool perched = false;                                 // resting in a tree
    double perchX = 0, perchZ = 0, perchY = 0, perchR = 2;
    int wraps = 0;                                        // J-07: a floater's comings in again upwind
};
// N3 creature behaviour states. J-07: the ways' (a browser's neck up in a tree, a hunter stalking and chasing, a scavenger trailing a
// hunter, a burrower under the ground, a climber up a trunk, one on its ground facing the explorer and charging)
enum CritterState { CS_IDLE = 0, CS_WALK, CS_GRAZE, CS_DRINK, CS_REST, CS_ALERT, CS_FLEE, CS_CURIOUS, CS_BROWSE, CS_STALK, CS_CHASE, CS_TRAIL,
                    CS_UNDER, CS_CLIMB, CS_DISPLAY, CS_CHARGE, CS_STATES };
// J-07: what an animal is seen doing (the guide keeps a bit of each a species was seen doing): a land animal's state, then the air's and the
// water's
// J-08: feeding at remains (a hunter's, a scavenger's: a land state numbered after the doings, whose bits the guide keeps), and remains seen
enum AnimalDoing { DO_FLYING = CS_STATES, DO_PERCHED, DO_SWARMING, DO_SOARING, DO_DRIFTING, DO_SWIMMING, DO_LEAPING, DO_FEEDING, DO_DEAD, DO_COUNT };
constexpr int CS_FEED = DO_FEEDING;
extern const char* const DOING_NAMES[DO_COUNT];
// J-08: what the record marks a species seen with (the guide keeps the bits): dying (its remains or a sick one seen), gone (remains where none
// of it lives), changed (a changed young seen)
enum AnimalMark { AM_DYING = 1, AM_GONE = 2, AM_CHANGED = 4 };
// J-09: an overgrowth's structures (`SurfaceView::forGrowths`): the shared ones, then each type's own (a fungus's caps and shelves, a slime
// mould's creeping veins, a coral's spires, a sponge's vases, hair's waving tufts, the rising bladders)
constexpr double GROWTH_BREATH_RATE = 0.8;   // J-10: a vent's lips breathe at this many radians a second (the drawing and its sound)
enum GrowthShape { GS_MOUND = 0, GS_RIDGE, GS_BLISTER, GS_SAC, GS_STALK, GS_FOLD, GS_VENT, GS_ARCH, GS_TEETH, GS_MEMBRANE, GS_POLYP, GS_CAP, GS_SHELF,
                   GS_VEINS, GS_SPIRE, GS_VASE, GS_HAIR, GS_BLADDER, GS_COUNT };
// J-09: one structure: its shape, its foot and the ground there, its radius (a stalk's, a rib's or a post's girth, a tuft's or a cluster's
// reach), its height (a bladder's float), its heading and its length along it (a ridge's, a fold's, an arch's span, a row's, a membrane's,
// the veins'), its hash; whether the explorer walks round it (`collides`: what stands over a step)
struct GrowthInst {
    int shape = GS_MOUND;
    double x = 0, z = 0, gy = 0, r = 0, h = 0, heading = 0, len = 0;
    uint64_t hs = 0;
    bool collides = false;
};
double growthReachOf(const GrowthInst& S);                                // how far it reaches over the ground from its foot
bool growthHolds(const GrowthInst& S, double x, double z, double margin);  // whether a point lies under it (within `margin` of its reach)
// the points it stands on (a round one's foot, a row's teeth, an arch's ribs' feet, along a ridge, a fold or a membrane) with the disc each
// stands in the way with: the colliders' and the drawing's
struct GrowthFoot { double x = 0, z = 0, r = 0; };
constexpr int GROWTH_FEET = 24;
int growthFeet(const GrowthInst& S, GrowthFoot* out, int max);
extern const char* const ANIMAL_MARK_NAMES[3];
// J-08: an animal dead on the ground as far as it has gone (`RemainsStage`, animal_mesh.h's: how it is posed and drawn)
extern const char* const REMAINS_NAMES[RS_COUNT];
// J-08: remains: a die-off's (its habitat cell's, a pure function of the cell: `SurfaceView::cellRemains`), a kill's, old bones on any world. Its
// species, where it lies and which way, its size, its stage, its own hash, its cell (dropped with the cell); on high ground with stalks (a
// sprouting one's), a kill (the hunters feed at it: `eaten` the seconds they fed, past which it is sunk to its ribs), old bones
struct Remains {
    int species = -1;
    double x = 0, z = 0, heading = 0, size = 1;
    int stage = RS_FRESH;
    uint64_t seed = 0;
    int gLat = 0, gLon = 0;
    bool cell = true, stalks = false, kill = false, old = false;
    double eaten = 0;
};
struct Critter {
    double x = 0, z = 0, heading = 0, speed = 0, timer = 0, size = 1;
    uint64_t seed = 0;
    int kind = 0;             // 0 grazer, 1 crawler (M4-05; now 1 for hexapods)
    int herd = 0;
    double fleeT = 0;
    int species = -1;         // index into the bestiary
    int state = CS_IDLE;
    double gaitPhase = 0;     // stride cycles walked
    double headPitch = 0;     // 0 head up .. 1 grazing
    double lift = 0;          // 0 standing .. 1 lying
    double calledT = -100;    // time of the last call
    struct Track { float x, z; };
    std::vector<Track> tracks; int trackHead = 0; double trackAccum = 0;
    // J-07: the step (its gait and the metres a cycle at its speed) and the ways' postures and places
    int gait = 0; double stride = 1;
    double sink = 0, reach = 0, crouch = 0;   // going under 0..1, a browser's neck up 0..1, a stalker low 0..1
    double climbY = 0;                        // how high up its trunk
    double treeX = 0, treeZ = 0, treeR = 0, treeTop = 0, treeGy = 0;   // the trunk it browses or climbs: its foot, radius, how high it goes, its ground
    double homeX = 0, homeZ = 0;              // its burrow's mouth; a territorial's ground
    uint64_t quarry = 0;                      // a hunter's quarry, a scavenger's hunter (their seeds); 0 none
    double hungerT = 0;                       // till a hunter hunts again, a territorial charges again
    double calmT = 0;                         // a climber's or a burrower's time since the explorer was near
    // J-08: its fate: a sickness it shows (an MF_ bit of galaxy/made.h, 0 none), born changed (its species' changes: the changed rig), the remains
    // it feeds at (their seed, 0 none), a crazed or a stumbling one's fall (seconds left), a hunt's one roll for the kill made, killed (gone after
    // the frame)
    int sick = 0;
    bool changed = false;
    uint64_t food = 0;
    double fallT = 0;
    bool rolled = false, killed = false;
    int64_t fallWin = -1;                     // the four seconds' window its last fall came in
};
// N2: one tree, placed by SurfaceView::forTrees (drawing, colliders, coverage and creatures read the same list)
struct TreeInst {
    double x = 0, z = 0, gy = 0;   // position and ground height (local metres)
    double h = 8, r = 3, cy = 5;   // height, canopy radius, canopy centre height above the ground
    double lx = 0, lz = 0;         // the lean: where the canopy centre sits relative to the foot (metres)
    int fam = 0;                   // silhouette: 0 dome, 1 cone, 2 umbrella, 3 tiered giant, 4 fibrous stalk, 5 fern tree, 6 mushroom tree, 7 weeping, 8 candelabra, 9 spire
    int bank = 3;                  // leaf bank: 3 forest, 5 second family / autumn, 10 straw variant
    bool dead = false, bare = false;   // a dead skeleton; deciduous in winter
    bool stump = false;            // a stone stump of a scorched world's petrified forest (a broken trunk, no branches)
    bool made = false, galls = false;   // J-05: in a made plants' look (its leaves in its bank: a blight, new leaves, a glow), swellings on it
    bool wrapped = false;          // J-09: wrapped in an overgrowth (its trunk and canopy in its bank and tile)
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
// I-04: a silhouette's look on a planet of a seed (`SurfaceView::buildLooks`' hash) and its shape (its height's range, its canopy's radius
// and middle as shares of the height; flora.cpp's), for the ship's garden of the home world's plants
TreeLook treeLookOf(uint64_t seed, int fam);
struct FloraShape { double h0, h1, r, cy; };
FloraShape floraShapeOf(int fam);
struct LogInst { double x = 0, z = 0, heading = 0, len = 6, radius = 0.4; uint64_t seed = 0; bool stone = false; };   // Q-02: stone, a petrified forest's (its own bank)
// C-01: a settlement's cell (`peoples/ruins.*`) in the view's metres (B-417: M4-07's monoliths of the old ones are gone)
struct Ruin {
    double x, z, heading, size;
    const RuinSpec* spec = nullptr;   // the settlement's layout (the cell cache's)
    double reach = 0;                 // how far its pieces reach from its middle (its harbour, a fallen tower's top): past its radius at times
};

// Z-03: what a vial takes at a place of the ground (`SurfaceView::readSample`): its microhabitat (a function of the square metre: the planet
// function, the springs' and vents' finders, the ruins' walls), its name as the HUD gives it ("WET SILT AT A LAKE'S EDGE"), the temperature read
// at the take, and where it is on the body; `refusal` where nothing can be taken there (molten ground)
struct SampleRead {
    bool ok = false;
    const char* refusal = nullptr;
    Microhabitat hab;
    std::string name;
    double readingC = 0;
    Vec3 unit;
    // R-434: read without the ruins' walls (the sight's reads on the pool): the walls' check left to the main thread, the place's mean kept for it
    bool wallsAsked = false;
    double placeC = 0;
};
