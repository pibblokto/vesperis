// First-person exploration of a landing site: sky, terrain, water, objects,
// weather, the landing capsule, and the explorer's movement.
#pragma once
#include "site.h"
#include "core/framebuffer.h"
#include "core/raster.h"
#include "core/input.h"
#include "space/space_view.h"
#include "textures.h"
#include "bestiary.h"
#include "galaxy/landmarks.h"
#include "galaxy/ruins.h"
#include "galaxy/shards.h"
#include "galaxy/fronts.h"
#include "galaxy/nights.h"
#include "galaxy/graves.h"
#include <set>
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

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
extern const char* CRITTER_STATE_NAMES[8];
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
struct LogInst { double x = 0, z = 0, heading = 0, len = 6, radius = 0.4; uint64_t seed = 0; };
// M4-07 a ruin or monolith; C-01 a settlement's cell (`galaxy/ruins.*`)
struct Ruin {
    double x, z, heading, size;
    int kind;    // RuinKind: 0 columns, 1 cube, 2 dome, 3 walls, 4 the giant cube, 5 a settlement
    int style;   // 0 smooth, 1 striated, 2 glowing lines
    const RuinSpec* spec = nullptr;   // C-01: the settlement's layout (the cell cache's)
};

class SurfaceView {
public:
    Proj proj = Proj::fromHFov(70);
    double mouseSens = 1.0;       // settings
    bool invertY = false;
    void setFov(double deg);
    double baseFov = 70;
    SurfaceSite site;
    Player player;
    SurfaceEnvironment env;
    LocalFront localFront;   // W-03: the nearest front's line here (the sector map draws it; the sky, the ground and the fog read it)
    // W-03: the extra extinction of the ray from the camera to a point: the wall of rain (or dust) of the front between them
    double rainWall(double x, double z, double dist) const { return localFront.on ? localFront.wall(localFront.ahead0, localFront.ahead(x, z), dist, env.frontRain) : 0.0; }
    double capsuleX = 0, capsuleZ = 0, capsuleY = 0;
    double cameraOverrideAlt = -1;   // >=0 during descent/ascent: camera altitude above ground
    // M6-05 photo mode: a free camera detached from the explorer
    bool freeCam = false;
    Vec3 freePos;
    double freeYaw = 0, freePitch = 0;
    void enterFreeCam();
    void leaveFreeCam() { freeCam = false; }
    void updateFreeCam(double dt, const Input& in);   // also refreshes the environment for the camera's spot
    double cameraOverridePitch = 0;
    bool valid = false;
    std::vector<Flock> flocks;
    std::vector<Critter> critters;
    int lastRocksDrawn = 0, lastRocksCandidates = 0;
    int shadingMode = 2;             // terrain shading: 0 flat cells (the polymap look), 1 per-vertex shade snapped to 12 steps (M10-12), 2 smooth (B-311, default)
    bool sprintToggleMode = false;   // settings: Shift toggles the sprint instead of holding it
    double sprintMultiplier = 3.2;   // settings: sprint speed factor on 1 g
    Buggy buggy;
    bool inBuggy = false, chaseCam = false;
    Drone drone;                    // R-403
    bool inDrone = false;
    bool inVehicle() const { return inBuggy || inDrone; }
    bool hasWaypoint = false;
    double wpX = 0, wpZ = 0;
    // kind: 0 rock, 1 trunk, 2 log, 3 ruin; y0..y1 the span it blocks (absolute metres): what has risen above a wall's top
    // clears it (B-405: the jetpack met every wall of a town as an invisible one, whatever its height)
    struct Collider { double x, z, r; int kind = 0; double y0 = -1e9, y1 = 1e9; };
    void collectColliders(double x, double z, std::vector<Collider>& out);   // rocks and trunks near (x, z)
    bool ruinAt(int gLat, int gLon, Ruin& r) const;   // M4-07: the ruin of a 2 km latitude/longitude cell, in local metres
    void ruinCellAt(double x, double z, int& gLat, int& gLon) const;
    // C-01: the ruins of the cells round the site, each built once from `ruinOfCell` (a town's layout is not cheap) with its
    // pieces at the three levels of detail; cleared at a landing or a re-anchor (the local metres change)
    struct RuinCell { bool has = false; Ruin r; RuinSpec spec; std::vector<RuinElem> elems[3]; std::vector<ShardSite> shards; std::vector<Grave> graves; mutable std::vector<float> base; mutable bool baseDone = false; };   // C-03: the settlement's shards; C-12: its graves (their stones among the pieces of lod 0)
    mutable std::unordered_map<uint64_t, RuinCell> ruinCells;
    Culture cultures[2]; int peoples = 1;           // C-01: the world's, set at init; C-13: one a people (a settlement's is `cultures[spec.people]`)
    Lore lores[2];                                  // C-12: the world's names, span and calendar (`loreQuick`, no chart marks), for the graves; empty on a world without a people; C-13: one a people
    double lastRuinMs = 0; int lastRuinElems = 0;   // tests: the ruins' draw time and the pieces drawn
    double roadBuildX = 0, roadBuildZ = 0;          // C-09: where the site's roads were last grown (`SurfaceSite::ensureRoads`)
    const RuinCell* ruinCell(int gLat, int gLon) const;
    // C-08 (KI-345): the settlements within reach of a point (at most four, within their radius plus 50 m) and whether a point lies in
    // one of their buildings (its rectangle plus a margin): the rocks, the logs and the rock colliders keep out of the rooms
    int settlementsNear(double x, double z, const RuinCell* near[4]) const;
    static bool inSettlementBuilding(const RuinCell& c, double x, double z, double margin);
    void drawSettlement(Framebuffer& fb, const Ruin& r, const RuinCell& cell, double dist);
    // C-03: the shards (`galaxy/shards.h`): the world's the explorer already has (the game fills it from the guide at a landing
    // and a load; every copy of one is then neither drawn nor offered), the one within reach this frame (E takes it), the draw
    std::set<int> shardsFound;
    struct NearShard { int index = -1; int place = 0; int sclass = 0; double x = 0, z = 0, dist = 0; };
    NearShard nearShard;
    void findNearShard();
    void drawShards(Framebuffer& fb, double t);
    // C-12: the grave within 2.2 m of the explorer this frame (the stone's name and years as the HUD reads them; the game records it in the guide the first time)
    struct NearGrave { int k = -1; uint64_t id = 0; int sclass = 0; std::string line, hud; double x = 0, z = 0, dist = 0; };   // `line` "Thaelu, years 140 to 212 of Kethra" (the log), `hud` "Thaelu, 140-212"
    NearGrave nearGrave;
    void findNearGrave();
    int lastShardsDrawn = 0;
    // the ruins of the cells within `reach` cells of (x, z): fn(ruin, cell, ring), ring the cell's distance in cells
    template <class F> void forNearbyRuins(double x, double z, int reach, F fn) const {
        if (!worldHasRuins(site.gen)) return;
        double lat, lon; site.latLonAt(x, z, lat, lon);
        double dLat = ruinCellLat(site.gen);
        int gLat = (int)std::floor(lat / dLat);
        for (int dl = -reach; dl <= reach; dl++) {
            int row = gLat + dl;
            double latc = (row + 0.5) * dLat;
            if (std::fabs(latc) > PI / 2 - dLat) continue;
            double dLon = dLat / std::max(std::cos(latc), 0.05);
            int g2 = (int)std::floor(lon / dLon);
            for (int dd = -reach; dd <= reach; dd++) { const RuinCell* c = ruinCell(row, g2 + dd); if (c->has) fn(c->r, *c, std::max(std::abs(dl), std::abs(dd))); }
        }
    }
    // N2 vegetation: every tree and log of a 16 m cell, deterministic; the canopy density field (veg x clearings)
    void forTrees(int cx, int cz, const std::function<void(const TreeInst&)>& fn);
    void forLogs(int cx, int cz, const std::function<void(const LogInst&)>& fn);
    double canopyDensityAt(double x, double z, double veg) const;
    double seasonPhaseLocal() const;               // -1 deep winter .. +1 high summer for this site's hemisphere (0.5 without tilt)
    double testCanopyCoverage(double radiusM);      // share of forest-biome ground within the radius under a canopy
    int lastTreesDrawn = 0, lastFloraPoints = 0, lastFloraPolys = 0, lastNearTrees = 0, lastMidTrees = 0, lastFarTrees = 0, lastTreesHidden = 0;
    int lastTreesVisited = 0, lastTreesListed = 0;   // B-310 (bench): trees listed this frame, and visits summed over the band threads (the duplication of the per-tree work)
    double lastFloraMs[4] = {0, 0, 0, 0};   // trees, logs and undergrowth, meadows/shores/fireflies, deserts and tundra (bench)
    double lastRenderMs[6] = {0, 0, 0, 0, 0, 0};   // B-310 (bench): palette and sky; floor and the vertex warm pass; the terrain bands; reflections to objects; flora; the rest
    double highlightUntil = -1;                    // M4-05: X highlights creatures
    // N3 creatures: the world's bestiary, the site's herds (planLife is the deterministic spawn, shared with the survey).
    // B-316: a herd is either one of the landing's (planLife: within 180 m of the capsule, `origin`) or the herd of a
    // habitat cell (512 m squares of the body's latitude and longitude, hashed: a chance by biome, a species, a size),
    // spawned when the explorer comes within 900 m of the cell's centre and forgotten beyond 1300 m, so there are animals
    // wherever the biome has them, not only round the capsule (they used to live at the landing spot alone, and the
    // re-anchor at 50 km cleared them for good)
    struct Herd { double cx = 0, cz = 0; int gLat = 0, gLon = 0; bool origin = false; };
    std::vector<Herd> herds;
    Bestiary bestiary;
    static void planLife(const SurfaceSite& s, const Bestiary& B, uint64_t seed, std::vector<Critter>& critters, std::vector<Flock>& flocks, std::vector<Herd>& herds);
    void spawnLife(double lat, double lon);
    static void lifeCellAt(const SurfaceSite& s, double x, double z, int& gLat, int& gLon);
    // the herd a habitat cell holds, if any: its centre (local metres), species and size; deterministic
    static bool planCellHerd(const SurfaceSite& s, const Bestiary& B, int gLat, int gLon, Herd& h, int& species, int& n);
    void updateHabitat();
    double habitatT = -1e9;
    std::set<int> seenSpecies;                     // species sighted within 60 m this landing
    std::vector<std::string> sightings;            // "name|plan|distance" entries for the guide (consumed by the game)
    struct CallCue { int kind; double pitch; };
    std::vector<CallCue> pendingCalls;             // creature calls for the synth (consumed by the game)
    int callsFired = 0;
    double hoofLevel = 0;                          // hoof and paw steps of moving animals within 20 m, 0..1
    double lastLifeMs[2] = {0, 0};                 // update, draw (bench)
    double lastAuroraP0 = 0;   // B-403: km poleward of the site to the auroral oval's centre at the last frame (diagnostics)
    static double auroraOvalLat(const Body& b) { return 59.0 + 8.0 * magneticField(b) + 6.0 * (unitFromHash(hashCombine(b.seed, 0xA17)) - 0.5); }   // B-403/R-402: the auroral oval's latitude on a world: nearer the pole on a strong field
    bool testGotoHerd(double dist);                // stand south of herd 0, facing it
    std::string testHerdStates() const;            // "GRAZING 3 FLEEING 2 (calls 1)"
    bool projectPoint(double x, double y, double z, double& sx, double& sy) const;   // world -> framebuffer pixels
    Mat3 testCamWorld(double t) const { return camLocal * site.localFrame(t); }        // tests: the world->view camera of the last frame
    double stepPhase = 0;                          // last integer stride (audio steps)
    int stepMaterial = MAT_ROCK;
    bool deployBuggy();              // B near the capsule: a new buggy, any old one is scrapped (R-204)
    bool toggleBuggy();              // E: enter or leave
    // N4-06 (R-204): the nose camera's gimbal range: you cannot look back from inside the buggy
    static constexpr double CAM_PAN = 70 * DEG, CAM_TILT_DOWN = 35 * DEG, CAM_TILT_UP = 25 * DEG;
    Vec3 buggyCameraMount() const;                // the nose camera's lens in local metres
    void cameraFeed(Framebuffer& fb, double t);   // the CCTV look of the nose camera's picture (after the mush, before the RGB conversion)
    double buggyDist() const { return std::sqrt((buggy.x - player.x) * (buggy.x - player.x) + (buggy.z - player.z) * (buggy.z - player.z)); }
    // R-403: the drone (`surface/drone.cpp`): `F` near the capsule unfolds a new one (any old one is scrapped), `E` on the ground gets in or out
    bool deployDrone();
    bool toggleDrone();
    static constexpr double DRONE_TILT_DOWN = 60 * DEG, DRONE_TILT_UP = 25 * DEG;   // the drone's camera tilts further down: the ground is what you look for
    static constexpr double DRONE_TOP_SPEED = 320.0 / 3.6, DRONE_CEILING = 400.0;    // m/s; metres above the ground
    Vec3 droneCameraMount() const;
    double droneDist() const { return std::sqrt((drone.x - player.x) * (drone.x - player.x) + (drone.z - player.z) * (drone.z - player.z)); }
    // O1 (B-302): the rangefinder: march the camera's line of sight over the terrain caches; returns the distance to the
    // ground (or the water) it hits within maxDist, -1 for the sky. The last frame's answer is kept for the HUD and for M
    double rangeToGround(double maxDist, double& hitX, double& hitZ, bool& water);
    double lastRange = -1, lastRangeX = 0, lastRangeZ = 0; bool lastRangeWater = false;
    bool viewUnderwater = false;   // B-322: the eye is under the drawn water surface this frame (the palette and the fog follow it)
    std::vector<MeteorShower> showers;   // W-04: the world's meteor showers (`meteorShowersOf`, once a landing)
    double zodiDust = 0;                 // W-04: the dust of the world's sky for the zodiacal light (`zodiacalDust`, once a landing)
    // W-04: the shower meteors alight at t: their tail and head as local unit directions and the head's brightness (shades)
    struct ShowerMeteor { Vec3 tail, head; double bright = 0; int shower = 0; };
    void showerMeteorsAt(double t, std::vector<ShowerMeteor>& out) const;
    double drawRadiusM() const { return ringR[3] ? ringR[3] * 2048.0 : (ringR[2] ? ringR[2] * 512.0 : ringR[1] * 64.0); }   // how far the terrain is drawn

    void init(const StarSystem* sys, int bodyIndex, double lat, double lon, double t);
    void update(double dt, const Input& in, double t, bool controlsEnabled);
    void render(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr, double fade);
    void relocateCapsule(double x, double z);
    // Fill the terrain caches around the site in coarse-to-fine order within a time budget (KI-009).
    void prefetch(double budgetMs);
    // Move the site frame to the player's position (KI-007); local coordinates are rebuilt.
    void reanchor();
private:
    GrainTexture grain, grainFine;
    GrainTexture grainEdge, grainMacro;   // B-311: the material-boundary wobble (64 x 64) and the broad tone tile (128 x 128)
    GrainTexture leafTile, leafEdge, barkTile;   // B-315: the vegetation's tiles (textures.h)
    TreeLook looks[10];                          // B-315: per silhouette on this planet
    void buildLooks();
    std::vector<Vec3> dirLUT;
    std::vector<float> cloudGrid;
    std::vector<float> auroraGrid;   // B-403: per cell of the cloud grid, the aurora's shade and its tops' share (the colour), drawn over the sky after the stars
    double lastWindUpdate = -1;
    double windDriftX = 0, windDriftZ = 0, lastEnvT = -1;   // B-206: the clouds' drift integrates the wind over time (metres)
    double lightningFlash = 0;
    Rng weatherRng{1};
    std::vector<Front> fronts; double frontsT = -1e18; uint64_t frontsSeed = 0;   // W-03: the world's fronts alive now (listed again every two minutes)
    std::vector<float> zodiGrid;         // W-04: the zodiacal light on the sky's cell grid this frame (shades), interpolated per pixel

    Mat3 camLocal;            // local -> view
    Vec3 camPos;              // local metres (x, y=eye height abs, z)
    double invTwoR = 0;       // 1 / (2 R): the curvature drop per metre squared (planets)
    int ringR[4] = {40, 36, 26, 26};   // O1/O4: cells drawn per terrain ring (lod0..lod3), capped on small bodies; 0 = ring not drawn
    int ringN = 30;                    // O6-02: cells of the near ring (4 m; 120 m), 0 when not drawn
    double nearBlend = 0;              // O6-02: 1 while the near ring is drawn (B-313: at every speed now; it used to fade out above 12 m/s, and the 4 m relief rising as the buggy stopped read as the hills changing shape), 0 when it is not (no ring, or the capsule high up)
    void computeRings();
    void updateNearRing(double dt);
    int frameStamp = 0;
    int prefetchStage = 0, prefetchRing = 0;
    // M7-01: a worker samples the terrain ahead of the walking direction into a staging buffer; the main
    // thread copies it into the caches (sampleAt is pure, so the result is the same whoever computes it)
    std::thread aheadThread;
    std::atomic<bool> aheadBusy{false};
    std::vector<std::pair<int, int>> aheadCells;
    std::vector<TerrainVertex> aheadOut;
    int aheadCache = 0;
    TerrainCache* aheadTarget = nullptr;   // O6-02: the cache the pending job fills (near, lod0 or lod1)
    int aheadStat = 0;                 // cells delivered by the worker (for bench)
    void prefetchAhead();
    void joinAhead();
public:
    ~SurfaceView() { joinAhead(); joinScan(); }
    SurfaceView() = default;
    SurfaceView(const SurfaceView&) = delete;
    SurfaceView& operator=(const SurfaceView&) = delete;
    int testAheadCells() const { return aheadStat; }
private:
    // M10-05 material meso tiles, built lazily per landing
    GrainTexture mesoTiles[MAT_COUNT];
    bool mesoBuilt[MAT_COUNT] = {false};
    const GrainTexture* mesoFor(int material);
    int curRadius = 40;              // ring radius of the LOD being drawn (limits the shadow march)
    // footprints: ring buffer of recent steps (x, z, heading)
public:
    // M4-08 eruptions on molten and volcanic worlds; R-307: kind 1 is a tectonic world's lava fountain (taller, longer),
    // kind 2 a meteorite strike on a bombarded world (a flash, then the ejecta)
    struct Eruption { double x, z, start; int kind = 0; };
    std::vector<Eruption> eruptions;
    double nextEruption = 30;
    double eruptionCue = 0;     // >0: an eruption just started (audio)
    // R-307: the ground's shaking on a tectonic world (0..1, decays), the next quake's countdown; the camera trembles by it
    double quake = 0, nextQuake = 45;
    // R-307: dust devils on a desert world: columns of dust that wander with the wind for a few minutes
    struct DustDevil { double x, z, height, radius, age, life, phase; };
    std::vector<DustDevil> devils;
    double nextDevil = 20;
    // M4-02 trail of where you walked (one point per 25 m); cleared by a new landing (B-303), carried across a re-anchor
    std::vector<std::pair<float, float>> trail;
    int siteEpoch = 0;   // B-303: counts landings and re-anchors, so cached maps of the site (the sector map image) know to rebuild
    double drainCheckT = -1e9;   // O6-03: when the drainage tiles ahead were last checked
    // R-408: the surface scanner's echoes: every sight the world can hold within SCAN_RANGE of the scan's centre (the cells' kinds
    // by `sightsNear`, every ruin by `ruinsNear`), searched on a thread of their own while the game wants them (`scanWanted`: the
    // scanner is on) at a landing, after a re-anchor and when the explorer has gone SCAN_STEP from the last centre; in local metres
    struct ScanEcho { Landmark lm; double x = 0, z = 0; };
    static constexpr double SCAN_RANGE = 40000, SCAN_STEP = 5000;
    std::vector<ScanEcho> echoes;
    bool scanWanted = false, scanned = false;   // the game's wish; a scan has come in for this site
    Vec3 scanU;                                 // where the last scan was asked (body-frame unit: a re-anchor keeps it)
    double scanMs = 0;                          // what the last scan cost on its thread
    void startScan();              // the search of the sights round the explorer, on a thread of its own
    void collectScan();            // the main thread takes a finished scan (update)
    void joinScan();
    std::thread scanThread; std::atomic<bool> scanBusy{false}; std::vector<Landmark> scanOut; double scanOutMs = 0;
private:
    struct Footprint { float x, z, heading; };
    std::vector<Footprint> footprints;
    int footHead = 0;
    double stepAccum = 0, lastStepX = 0, lastStepZ = 0;
    bool footLeft = false;
    double castShadowLit(TerrainCache& cache, int cx, int cz, double h, const Vec3& sd);
    // per-frame cloud layer parameters for ground shadows (M10-14)
    bool cloudShadows = false;
    // B-309: the sun's screen position and radius this frame (framebuffer pixels) and the strength of the veiling glare
    // drawn over the finished picture (0 = none: below the horizon, eclipsed, behind the camera)
    double sunSx = -1, sunSy = -1, sunRpx = 0, sunVeil = 0;
    void drawVeil(Framebuffer& fb);
    double cloudWx = 0, cloudWz = 0, cloudThr = 1;
    double lastT = 0;
    // M10-14 galactic band: integrated star density over galactic directions (lon x lat map)
    std::vector<float> bandMap;
    NebulaPatch nebP[16]; int nebN = 0;   // N5-01
    void buildBandMap();
    double bandAt(const Vec3& dirWorld) const;
    void drawBlobShadow(Framebuffer& fb, double x, double z, double radius, double height);
    void drawBlobShadowFrom(Framebuffer& fb, double x, double z, double radius, double height, const Vec3& sunDir, double share);
    void drawFootprints(Framebuffer& fb);
    void drawReflections(Framebuffer& fb);   // M1-04: mirrored terrain into water pixels
    void updateBuggy(double dt, const Input& in, double t);
    void drawBuggy(Framebuffer& fb, double t);
    void buggyFrame(Vec3& fwdT, Vec3& sideT, Vec3& upT) const;   // the hull's axes with its pitch and roll
    void drawPuffs(Framebuffer& fb, const std::vector<Buggy::Puff>& puffs);   // the dust, snow and spray thrown up (the buggy's wheels, the drone's downwash)
    void updateDrone(double dt, const Input& in, double t);   // R-403
    void drawDrone(Framebuffer& fb, double t);
    void droneFrame(Vec3& fwdT, Vec3& sideT, Vec3& upT) const;
public:
    double findOpenRun(double& x, double& z, double& heading);   // N4: open ground within 800 m of the explorer and its longest clear run (metres)
private:
    void drawWaypointLine(Framebuffer& fb);
    void drawRuins(Framebuffer& fb, double t);
    void updateWalking(double dt, const Input& in, double t, bool controlsEnabled);
    struct VtxCache { int stamp = -1; RVert v; uint8_t bank = 0; bool front = false; float sy = 0; float wy = -1e9f; float hy = 0; float sh = 1e9f; };   // B-322: hy = the height as drawn (morphed), sh = the side of the water's edge as drawn (`vertexShore`, morphed), for the shore cut and the water's depth
    double coarseShoreAt(TerrainCache& c, double x, double z);   // B-322: the coarser ring's shore value at (x, z), bilinear; 1e9 where a corner has none   // B-310: front/sy = the projected row, for the band cull; B-320: wy = the water level as drawn (morphed with the ground), -1e9 none
    double coarseWaterAt(TerrainCache& c, double x, double z);   // B-320: the coarser ring's water level at (x, z), bilinear over its wet corners; -1e9 none
    std::vector<VtxCache> vc0, vc1, vc2, vc3, vcN;
    void drawCometJets(Framebuffer& fb, double t);   // O4: dust jets venting from the sunlit ground of an active comet
    void drawGeysers(Framebuffer& fb, double t);     // R-307: the geysers (europan cracks, geyser basins, tectonic fumaroles and steady fountains)
    // R-307: one jet: a dense core column, a spray crown and a falling veil, `heightM` and `widthM` at full strength
    struct JetSpec { double x, z; uint64_t h; double dist; double heightM, widthM; int bank; bool lava; };
    void drawJet(Framebuffer& fb, double t, const JetSpec& j, double strength);
    static double jetStrength(uint64_t h, double t, bool constant, double cycle, double duty);   // 0 off .. 1 full (1.35 in the first seconds of a burst)
    void drawFountains(Framebuffer& fb, double t);   // R-307: the lava fountains and the meteorite strikes (the eruption list), the dust devils
    double heightNear(double x, double z, double dist, double& water);   // the cached height (and water level) at a distance from the camera, coarser far away

    void computeEnvironment(double t);
    void setupPalette(Framebuffer& fb);
    void buildDirLUT();
    void drawSky(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr);
    void drawFloor(Framebuffer& fb);
    double floorHeight();   // the floor's height beyond the last ring: the sea, or 300 m under the lowest far cell (0 on a small body)
    double floorH = 0;      // B-408: this frame's, for the floor and the galactic band's horizon
    // B-310: the terrain is drawn in parallel bands of rows: warmTerrainLOD fills the vertex cache and the material tiles
    // serially, then every band thread draws all rings clipped to its rows (drawTerrainLOD with bandY0/bandY1 skips
    // cells wholly outside the band), reading only. The frame is identical to the serial loop's.
    void warmTerrainLOD(TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM, TerrainCache* coarse, std::vector<VtxCache>* coarseVc, int band);
    bool cellCovered(double cs, int cx, int cz, double coverM) const;   // B-318: the cell lies wholly under the finer ring (every corner within coverM of the camera)
    // B-313 geomorphing: while a ring is warmed, the next coarser ring (already warmed) and the width of the morph band
    // in cells; a vertex within the band takes the coarser ring's interpolated height and shade in proportion to its
    // (continuous) distance from the camera, so the two surfaces coincide at the ring's edge and the finer relief rises
    // as the edge sweeps over the ground instead of popping cell by cell
    TerrainCache* morphCoarse = nullptr;
    std::vector<VtxCache>* morphVc = nullptr;
    double morphBand = 0;
    // the coarser ring's drawn surface at (x, z): its cell, the position within it, which of the two triangles, the height
    struct CoarseHit { int X, Z; double fx, fz; bool lower; double h; };
    CoarseHit coarseAt(TerrainCache& c, double x, double z);
public:
    static double basinJetStrength(uint64_t id, double t);   // R-307: the strength of a geyser basin's vent (its constant/cycle/duty by hash), for the drawing and the tests
    // B-313 (tests): after a render, per ring edge (near/lod0, lod0/lod1, lod1/lod2, lod2/lod3), the largest gap in metres
    // between the finer ring's edge vertices as drawn and the coarser ring's surface there (`seam`; the rings meet when
    // it is 0) and the gap the unmorphed vertices would have had (`raw`: what popped before). -1 where the edge is not drawn
    void testRingSeams(double seam[4], double raw[4]);
private:
    void drawTerrainLOD(Framebuffer& fb, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM,
                        bool water, double t, int bandY0 = 0, int bandY1 = 1 << 30, bool skirt = true);
    void drawObjects(Framebuffer& fb, double t);
    void drawFlora(Framebuffer& fb, double t);     // N2: trees, undergrowth, meadows, reeds, lily pads, cacti, lichen, fireflies (B-310: in parallel bands)
    struct FloraStats { int treesDrawn = 0, floraPoints = 0, floraPolys = 0, nearTrees = 0, midTrees = 0, farTrees = 0, treesHidden = 0, treesVisited = 0; double ms[4] = {0, 0, 0, 0}; };
    void drawFloraBand(Framebuffer& fb, double t, int bandY0, int bandY1, FloraStats& st);
    // B-310: the flora cells in view (near to far) and their trees, listed once per frame by drawFlora with the rows a
    // tree can touch (its trunk, canopy, sway and shadow), so a band thread places only the trees that reach its rows
    struct FloraCell { float d; int cx, cz; };
    struct TreeRef { TreeInst T; float dist; int y0, y1; };
    std::vector<FloraCell> floraCells;
    std::vector<TreeRef> treeList;
    void drawCapsule(Framebuffer& fb, double t);
    void drawWeather(Framebuffer& fb, double t);
    void drawLife(Framebuffer& fb, double t);
    void updateLife(double dt, double t);
    const RVert& vertexOf(TerrainCache& cache, std::vector<VtxCache>& vcache, int cx, int cz, uint8_t& bank);
    void materialLook(const TerrainVertex& v, uint8_t& bank, double& albedo) const;
    // Local metres -> view. The ground curves away from the camera in every direction (M1-07); since O1 the curvature is
    // applied here, so rocks, trees, the capsule and the buggy sit on the curved ground too, not only the terrain mesh.
    // Planets: the paraboloid drop d^2 / 2R. Small bodies (O4, comets): the exact sphere seen from the camera's own
    // vertical: positions foreshorten with the angle and the far side curves under.
    Vec3 toView(double x, double y, double z) const {
        double dx = x - camPos.x, dz = z - camPos.z, dy = y - camPos.y;
        double d2 = dx * dx + dz * dz;
        if (site.smallBody) {
            double d = std::sqrt(d2);
            if (d > 1e-9) { double ang = d / site.R, rr = site.R + y, s = rr * std::sin(ang) / d; dx *= s; dz *= s; dy = rr * std::cos(ang) - (site.R + camPos.y); }
        } else dy -= d2 * invTwoR;
        return camLocal * Vec3(dx, dy, dz);
    }
};
