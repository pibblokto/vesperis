// First-person exploration of a landing site: sky, terrain, water, objects,
// weather, the landing capsule, and the explorer's movement.
// E-08: `SurfaceView`, its helper types in surface_types.h. The class holds its data first, in the order it is made and
// destroyed (never reordered: the threads and the caches), with the types it holds; then its functions under a `// ----`
// banner for each file that defines them (surface_view.cpp's, then the environment's, the places', the walk's, the
// palette's, the terrain's, the sky's, the effects', flora.cpp's, creatures.cpp's, buggy.cpp's, drone.cpp's,
// forms_draw.cpp's), and last the harness's
#pragma once
#include "site.h"
#include "core/framebuffer.h"
#include "core/raster.h"
#include "core/input.h"
#include "space/space_view.h"
#include "textures.h"
#include "bestiary.h"
#include "animal_mesh.h"
#include "galaxy/landmarks.h"
#include "peoples/ruins.h"
#include "peoples/records.h"   // the records (peoples/shards.h with them)
#include "galaxy/fronts.h"
#include "galaxy/nights.h"
#include "peoples/graves.h"
#include "peoples/instruments.h"
#include <set>
#include <deque>
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include "surface_types.h"

struct Rng;
class SurfaceView {
public:
    // ---- the data, in the order it is made and destroyed (never reordered: the threads and the caches), and the types it holds
    Proj proj = Proj::fromHFov(70);
    double mouseSens = 1.0;       // settings
    bool invertY = false;
    double baseFov = 70;
    SurfaceSite site;
    Player player;
    SurfaceEnvironment env;
    LocalFront localFront;   // W-03: the nearest front's line here (the sector map draws it; the sky, the ground and the fog read it)
    double fogFloorM = 0, fogFloorX = 1e18, fogFloorZ = 1e18;   // the valleys' floor round the explorer (`refreshFogFloor`)
    double capsuleX = 0, capsuleZ = 0, capsuleY = 0;
    double cameraOverrideAlt = -1;   // >=0 during descent/ascent: camera altitude above ground
    // photo mode: a free camera detached from the explorer
    bool freeCam = false;
    Vec3 freePos;
    double freeYaw = 0, freePitch = 0;
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
    Drone drone;
    bool inDrone = false;
    // the explorer's helmet lamp (U; the pad's commands), on foot: a cone of light from the eye along the view, bright to a dozen metres
    // and falling off to nothing at 60 m, on every night side as in a drifter's dark; and the capsule's floodlights on the way down. The
    // frame's lights (`setupLamps`) are added into the ground's vertex light and every object's (`lampAt`), by night alone as the vehicles'
    // lights are, so on a dayside they add nothing
    bool lamp = false;
    bool uvLamp = false;        // Z-03: the lamp's ultraviolet (Shift+U on foot): the living films glow within its reach where nothing outshines them
    bool sight = false;         // R-434: the activity vision (the game's sixth vision, Shift+O): the ground and the water near coloured by their activity
    bool floodlights = false;   // the capsule's floodlights lit (the game sets it while it comes down and goes up; a harness's raised camera has none)
    struct LampLight { Vec3 pos, dir; double cosIn = 1, cosOut = 1, cosSpill = 1, nearM = 1, reach0 = 0, reach = 0, power = 0; };
    LampLight lamps[2];
    int nLamps = 0;
    bool hasWaypoint = false;
    double wpX = 0, wpZ = 0;
    // kind: 0 rock, 1 trunk, 2 log, 3 ruin; y0..y1 the span it blocks (absolute metres): what has risen above a wall's top
    // clears it (B-405: the jetpack met every wall of a town as an invisible one, whatever its height)
    struct Collider { double x, z, r; int kind = 0; double y0 = -1e9, y1 = 1e9; };
    // what a cell's enumerator gives the drawing and the colliders (`forRocks`, `forDomes`, `forChimneys`, `forMeteorites`)
    struct RockInst { double x = 0, z = 0, size = 0, gy = 0; uint64_t h = 0; };
    // what the explorer stands on: the ground, or a heap of earth or rubble, a bank, a barrow or a stand's tier under it (`formWalkable`:
    // their drawn surface, each with its mesh at its foot, gathered once a frame near the explorer); `blocked` where such a surface stands more
    // than a step (`STEP_M`) over the feet (the explorer walks into it, not onto it)
    static constexpr double STEP_M = 0.5;
    struct Support { const Form* f = nullptr; double x = 0, z = 0, gy = 0, skirt = 0, reach = 0; };
    struct SupportMesh { FormMesh m; double skirt = -1; };
    std::vector<Support> supports;
    std::unordered_map<const Form*, SupportMesh> supportMeshes;   // each in its own frame (its foot at the origin), while its cell is kept
    // the ruins of the cells round the site, each built once from `ruinOfCell` (a town's layout is not cheap) with its
    // pieces at the three levels of detail; cleared at a landing or a re-anchor (the local metres change)
    // `groups[l][b]` the first of building b's pieces in `elems[l]` (b == the buildings' count: the graves' stones, to the
    // end), so each building is drawn at its own level of detail; `foot[l][i]` the drawn ground under piece i at the middle and far
    // levels, read off the coarse ring that draws it there (its own water, and under a big piece's footprint the lowest and the
    // highest: it stands on the highest, its foot down to the lowest), kept while that ring draws it
    struct FootCache { int ring = -1; float y = 0, water = -1e9f, lo = 0, hi = 0; };
    // a settlement in view: its ruin and spec, its forms by level of detail and their groups, its shards and its graves (their stones
    // among the pieces of lod 0), their feet, `drawn` the frame that gathered it, `marked` its ground in the site's marks; each record's
    // object (`recordForms`, peoples/records.h: site k's from recordFirst[k] to recordFirst[k + 1]); the instruments beside its music
    // records (each site's id, -1 none; their objects as the records'); `watered` its records laid again with the water as drawn
    // (`waterRecords`)
    struct RuinCell {
        bool has = false;
        bool placed = false;
        Ruin r;
        RuinSpec spec;
        std::vector<Form> elems[3];
        std::vector<int> groups[3];
        std::vector<ShardSite> shards;
        std::vector<Grave> graves;
        mutable std::vector<FootCache> foot[3];
        mutable uint32_t drawn = 0;
        mutable bool marked = false;
        std::vector<Form> recordForms;
        std::vector<int> recordFirst;
        std::vector<InstrumentSite> instruments;
        std::vector<int> instrumentIds;
        std::vector<Form> instrumentForms;
        std::vector<int> instrumentFirst;
        mutable bool watered = false;
        // J-10: the openings its walls leave (lod 0) and how high an overgrowth's cover has climbed each building (the last: the graves' stones; -1
        // none), its pieces of the cover among each building's (`overgrowCell`)
        std::vector<FormOpening> openings;
        std::vector<float> climb;
        int growthLine = -1;   // KI-473: the line whose cover lies at its middle (its cover's colour and bank)
        std::vector<int8_t> lines;   // KI-501: each building's, the line whose cover lies at its own place (the last: the graves')
        // KI-477: the way the cover came from (a unit vector in the local metres toward its line's place; 0 where it rose round the settlement)
        float towardX = 0, towardZ = 0;
        uint64_t key = 0;   // KI-479: the cell's own key, the world's (a settlement's grid cell, a work's id): its membranes' keys are its
    };
    mutable std::unordered_map<uint64_t, RuinCell> ruinCells;
    Culture cultures[2]; int peoples = 1;           // the world's, set at init; C-13: one a people (a settlement's is `cultures[spec.people]`)
    Lore lores[2];                                  // the world's names, span and calendar (`loreQuick`, no chart marks), for the graves; empty on a world without a people; C-13: one a people
    double lastRuinMs = 0; int lastRuinElems = 0;   // tests: the ruins' draw time and the pieces drawn
    double roadBuildX = 0, roadBuildZ = 0;          // where the site's roads were last grown (`SurfaceSite::ensureRoads`)
    static constexpr int SETTLEMENTS_NEAR = 8;   // eight (four filled up where a capital's harbour reached past its radius among its neighbours)
    FormPalette palettes[2];                        // each people's colours and windows (from its culture; the harness may set them)
    std::deque<RuinCell> placedCells;               // tests: settlements the harness placed in the view, drawn and walked into as the cells' are
    int testForceLod = -1;                          // tests: every building at this level of detail (the sheet's meshes up close), -1 by distance
    int lastFormPolys = 0, lastFormLines = 0, lastSilhouettes = 0;   // tests: what the last frame drew of the forms
    // the shards (`peoples/shards.h`): the world's the explorer already has (the game fills it from the guide at a landing
    // and a load; every copy of one is then neither drawn nor offered), the one within reach this frame (E takes it), the draw
    // the records (`peoples/records.h`): each an object of its medium (a carved bone, a codex, a disc, a crystal, a painted wall: `recordForms`)
    // drawn with the settlement's forms within 40 m and a point of light over it within 250 m; a painted wall stays painted once recorded
    std::set<int> shardsFound;
    struct NearShard { int index = -1; int place = 0; int sclass = 0; int medium = RM_STONE; int work = -1; bool wall = false, archive = false, last = false; double x = 0, z = 0, dist = 0; };
    NearShard nearShard;
    // the grave within 2.2 m of the explorer this frame (the stone's name and years as the HUD reads them; the game records it in the guide the first time)
    // `line` "Thaelu, years 140 to 212 of Kethra" (the log), `hud` "Thaelu, 140-212"
    struct NearGrave { int k = -1; uint64_t id = 0; int sclass = 0; std::string line, hud; double x = 0, z = 0, dist = 0; };
    NearGrave nearGrave;
    // the instruments (peoples/instruments.h): each people's tradition and its instruments by age (set at init from the lore), the ones the
    // explorer already has from this world (the game fills it from the guide, as the shards), the one within reach this frame (E takes it)
    Tradition traditions[2]; PeopleInstruments instrumentSets[2];
    std::set<int> instrumentsFound;
    struct NearInstrument { int id = -1; std::string name; int people = 0; double x = 0, z = 0, dist = 0; };   // its name (its make's)
    NearInstrument nearInstrument;
    int lastShardsDrawn = 0, lastRecordForms = 0;   // the records' objects' forms the last frame gathered
    // the world plan's places within PLAN_NEAR_M of the explorer (peoples/plan.h): their anchor cells and where their cores stand,
    // refreshed as the explorer moves a kilometre (`refreshPlanNear`): a city's core spans cells, so the cells round a point reach it from
    // farther than its own; a far city's towers stand on the skyline (`drawRuins` gathers them to PLAN_NEAR_M)
    struct PlanNear { int gLat = 0, gLon = 0; double x = 0, z = 0, radius = 0; };
    std::vector<PlanNear> planNear;
    double planNearX = 1e18, planNearZ = 1e18;
    static constexpr double PLAN_NEAR_M = 24000;
    // the world's works (peoples/works.h) within PLAN_NEAR_M of the explorer, refreshed with the plan's places: each laid out once as a
    // settlement's cell (`workCell`: its pieces at the three levels, a necropolis's graves), visited by `forNearbyRuins` as the cells' are;
    // their ground put into the site's (`SurfaceSite::addWorkEdits`) by `ensureTownMarks`, each once. The world's works are taken when ready
    // (built on the plan's thread after the histories): a render never waits for them. B-418: and the land's (`worksAroundIfReady`), made as
    // the explorer comes within LAND_NEAR_M of their cells (the works are drawn within a few kilometres of their edge, their ground put in within
    // six: a cell made on the main thread costs a few milliseconds)
    static constexpr double LAND_NEAR_M = 8000;
    struct WorkNear { std::shared_ptr<const Work> work; double x = 0, z = 0, radius = 0; };
    std::vector<WorkNear> workNear;
    std::shared_ptr<const WorldWorks> worksHere;
    mutable std::unordered_map<uint64_t, RuinCell> workCells;
    std::unordered_set<uint64_t> workEdited;
    // KI-479: the membranes the explorer tore this landing (`membraneKey`: the cell's key and the opening's index), drawn slit and never torn
    // again; kept across a re-anchor (the cells are laid out again), emptied by `init`
    std::unordered_set<uint64_t> tornMembranes;
    // KI-476: the structures' lattice fixed to the world, its cells' structures placed once a landing (in its local metres; emptied with the
    // layer's key, at `init` and on a re-anchor)
    struct GrowthCell { double x = 0, z = 0; std::vector<GrowthInst> s; };
    std::unordered_map<uint64_t, GrowthCell> growthLattice;
    std::unordered_map<uint64_t, std::vector<GrowthInst>> growthOwned;   // each local cell's: those whose middles lie in it
    std::unordered_map<uint64_t, bool> growthCorners;   // KI-490: whether the cover holds each world cell's corner (`growthCornerHolds`)
    uint64_t growthLatticeKey = 0;
    // KI-480: the layer the ruins read, the world now (the game hands it on as the steps pass, `setMadeNow`), and the key the cells were laid
    // out and grown for (another: they are laid out again on the main thread); KI-491: the ground takes it too where what it draws moved
    // (`adoptMadeNow`)
    MadeWorld ruinsMade;
    uint64_t ruinsKey = 0;
    bool madeHanded = false;   // KI-491: a world now handed on that the ground has not looked at yet
    double markCheckX = 1e18, markCheckZ = 1e18;
    int lastTreesDrawn = 0, lastFloraPoints = 0, lastFloraPolys = 0, lastNearTrees = 0, lastMidTrees = 0, lastFarTrees = 0, lastTreesHidden = 0;
    int lastTreesVisited = 0, lastTreesListed = 0;   // bench: trees listed this frame, and visits summed over the band threads (the duplication of the per-tree work)
    double lastFloraMs[2] = {0, 0};   // the trees, the cells (logs, undergrowth, meadows, shores, deserts) (bench; the worst band's)
    // bench: palette and sky; the vertex warm pass; the terrain (the bin, the reflections' gather, the bands: floor, rings, reflections;
    // F-05); footprints to objects; flora; the rest
    double lastRenderMs[6] = {0, 0, 0, 0, 0, 0};
    int lastWarmMisses = 0;   // tests: the cells the last frame's warm pass sampled, its rings together
    static constexpr int FILL_SERIAL_MAX = 64;   // a ring's misses over this are sampled on the pool (warmTerrainLOD)
    // the bands of the last frame's terrain and flora passes (their rows and ms, the bench's skew); under VESPERIS_TRACE the
    // terrain's also split: the water corners' set-up, the fills (the ground's cells with their set-up, the walls, the water's
    // quads and foam), the cells walked (F-04: the binned cells the band read) and drawn (those reaching its rows) and wet; the
    // rest of a band's ms is its walk. The flora's: the trees visited and drawn
    struct BandTrace { int y0 = 0, y1 = 0; double ms = 0, waterMs = 0, fillMs = 0; int walked = 0, drawn = 0, wet = 0; };
    static constexpr int TRACE_BANDS = 64;   // the terrain's chunks (thirty-odd at 4x), the flora's bands
    BandTrace lastTerrainBands[TRACE_BANDS], lastFloraBands[TRACE_BANDS];
    int lastTerrainBandN = 0, lastFloraBandN = 0;
    // bench, tests: the cells binned, those every band draws (a corner behind the near plane, foam), the corners the bin found unwarmed (0)
    int lastTerrainCells = 0, lastTerrainAllRows = 0, lastStaleCorners = 0;
    double lastTerrainBinMs = 0;
    double lastRoadsMs = 0; int roadsBuilds = 0;    // the site's roads grown as the explorer goes (`ensureRoads` on the main thread): the last one's ms, how many
    double lastFloraListMs = 0;                     // the flora's cells and trees listed this frame (on the main thread, before the bands; a settlement's buildings tested against every tree)
    double highlightUntil = -1;                    // X highlights creatures
    // creatures: the world's bestiary, the site's herds (planLife is the deterministic spawn, shared with the survey).
    // a herd is either one of the landing's (planLife: within 180 m of the capsule, `origin`) or the herd of a
    // habitat cell (512 m squares of the body's latitude and longitude, hashed: a chance by biome, a species, a size),
    // spawned when the explorer comes within 900 m of the cell's centre and forgotten beyond 1300 m, so there are animals
    // wherever the biome has them, not only round the capsule (the landing spot)
    // J-08: `empty`, a cell held for its remains alone (none of its herd living, or no herd)
    struct Herd { double cx = 0, cz = 0; int gLat = 0, gLon = 0; bool origin = false, empty = false; };
    std::vector<Herd> herds;
    Bestiary bestiary;
    double habitatT = -1e9;
    std::set<int> seenSpecies;                     // species sighted this landing (J-07: drawn whole, in view and near)
    // J-07: the first sightings of the landing, for the guide (consumed by the game): the species, what it was doing, how far
    struct Sighting { int species = -1, doing = 0; double dist = 0; };
    std::vector<Sighting> sightings;
    struct CallCue { int kind; double pitch; };
    std::vector<CallCue> pendingCalls;             // creature calls for the synth (consumed by the game)
    int callsFired = 0;
    double hoofLevel = 0;                          // hoof and paw steps of moving animals within 20 m, 0..1
    double lastLifeMs[2] = {0, 0};                 // update, draw (bench)
    double lastAuroraP0 = 0;   // km poleward of the site to the auroral oval's centre at the last frame (diagnostics)
    double stepPhase = 0;                          // last integer stride (audio steps)
    int stepMaterial = MAT_ROCK;
    // the nose camera's gimbal range: you cannot look back from inside the buggy
    static constexpr double CAM_PAN = 70 * DEG, CAM_TILT_DOWN = 35 * DEG, CAM_TILT_UP = 25 * DEG;
    static constexpr double DRONE_TILT_DOWN = 60 * DEG, DRONE_TILT_UP = 25 * DEG;   // the drone's camera tilts further down: the ground is what you look for
    static constexpr double DRONE_TOP_SPEED = 320.0 / 3.6, DRONE_CEILING = 400.0;    // m/s; metres above the ground
    double lastRange = -1, lastRangeX = 0, lastRangeZ = 0; bool lastRangeWater = false;
    bool viewUnderwater = false;   // the eye is under the drawn water surface this frame (the palette and the fog follow it)
    std::vector<MeteorShower> showers;   // the world's meteor showers (`meteorShowersOf`, once a landing)
    bool lavaGlow = false;               // lava lights the ground and fills bank 2 here (a lava world's, or a drifter's ember lakes: TR_EMBERS), once a landing
    bool waterLights = false;            // a dark ocean's lights in its water (TR_LIGHTS_IN_WATER: the glow's bank 15, drifting)
    double zodiDust = 0;                 // the dust of the world's sky for the zodiacal light (`zodiacalDust`, once a landing)
    // the shower meteors alight at t: their tail and head as local unit directions and the head's brightness (shades)
    struct ShowerMeteor { Vec3 tail, head; double bright = 0; int shower = 0; };
private:
    GrainTexture grain, grainFine;
    GrainTexture grainEdge, grainMacro;   // the material-boundary wobble (64 x 64) and the broad tone tile (128 x 128)
    GrainTexture growthVeins; bool growthVeinsBuilt = false;   // KI-492: the cover's veins on a ruin's faces, as the ground's (`buildGrowthVeins`)
    static constexpr double GROWTH_VEIN_SCALE = 128 / 36.0;   // (its texels a metre: a tile four of the ground's 9 m cells across)
    GrainTexture leafTile, leafEdge, barkTile;   // the vegetation's tiles (textures.h)
    TreeLook looks[10];                          // per silhouette on this planet
    std::vector<Vec3> dirLUT;
    std::vector<float> cloudGrid;
    std::vector<float> cloudGlowGrid;   // J-04: per cell of the cloud grid, a seeded world's glowing clouds' light at night
    std::vector<float> auroraGrid;   // per cell of the cloud grid, the aurora's shade and its tops' share (the colour), drawn over the sky after the stars
    double lastWindUpdate = -1;
    double windDriftX = 0, windDriftZ = 0, lastEnvT = -1;   // the clouds' drift integrates the wind over time (metres)
    double lightningFlash = 0;
    Rng weatherRng{1};
    std::vector<Front> fronts; double frontsT = -1e18; uint64_t frontsSeed = 0;   // the world's fronts alive now (listed again every two minutes)
    std::vector<float> zodiGrid;         // the zodiacal light on the sky's cell grid this frame (shades), interpolated per pixel

    Mat3 camLocal;            // local -> view
    Vec3 camPos;              // local metres (x, y=eye height abs, z)
    double invTwoR = 0;       // 1 / (2 R): the curvature drop per metre squared (planets)
    int ringR[4] = {40, 36, 26, 26};   // cells drawn per terrain ring (lod0..lod3), capped on small bodies; 0 = ring not drawn
    int ringN = 30;                    // cells of the near ring (4 m; 120 m), 0 when not drawn
    // 1 while the near ring is drawn (B-313: at every speed: faded out above 12 m/s, the 4 m relief rising as the buggy stops would read as
    // the hills changing shape), 0 when it is not (no ring, or the capsule high up)
    double nearBlend = 0;
    int frameStamp = 0;
    int anchorFrame = -100;   // the frame of the last re-anchor (the trace follows the refill)
    int prefetchStage = 0, prefetchRing = 0;
    // a worker samples the terrain ahead of the walking direction into a staging buffer; the main
    // thread copies it into the caches (sampleAt is pure, so the result is the same whoever computes it)
    std::thread aheadThread;
    std::atomic<bool> aheadBusy{false};
    std::vector<std::pair<int, int>> aheadCells;
    std::vector<TerrainVertex> aheadOut;
    int aheadCache = 0;
    TerrainCache* aheadTarget = nullptr;   // the cache the pending job fills (near, lod0 or lod1)
    int aheadStat = 0;                 // cells delivered by the worker (for bench)
    // KI-491: the rings sampled again on a worker once the ground took the world now (`adoptMadeNow`): each ring's cells round the camera (the
    // others dropped, sampled when next asked for) and the corners of the structures' world cells in view; copied in on the main thread once
    // done (`takeResample`), the frames between drawing the ground as it was; `joinAhead` stops it, the cells not reached dropped
    struct ResampleCell { int ring, cx, cz; };
    static constexpr int RESAMPLE_FRAMES = 45;   // (taken on the 45th frame after, waiting if need be: the same frame however fast it ran;
                                                 // three quarters of a second at 60 frames a second)
    int resampleFrames = 0;
    std::thread resampleThread;
    std::atomic<bool> resampleStop{false};
    std::atomic<size_t> resampleDone{0}, resampleCornersDone{0};
    std::vector<ResampleCell> resampleCells;
    std::vector<TerrainVertex> resampleOut;
    std::vector<uint64_t> resampleCorners;   // (latticeKey: face, i, j)
    std::vector<uint8_t> resampleHolds;
    void startResample();
    void takeResample();
    // M10-05 material meso tiles, built lazily per landing
    GrainTexture mesoTiles[MAT_COUNT];
    GrainTexture madeCrustTiles[3]; bool madeCrustBuilt[3] = {false, false, false};   // KI-493: a crust line's crust in each of its forms
    bool mesoBuilt[MAT_COUNT] = {false};
    int curRadius = 40;              // ring radius of the LOD being drawn (limits the shadow march)
public:
    // eruptions on molten and volcanic worlds; R-307: kind 1 is a tectonic world's lava fountain (taller, longer),
    // kind 2 a meteorite strike on a bombarded world (a flash, then the ejecta)
    struct Eruption { double x, z, start; int kind = 0; };
    std::vector<Eruption> eruptions;
    double nextEruption = 30;
    double eruptionCue = 0;     // >0: an eruption just started (audio)
    // the ground's shaking on a tectonic world (0..1, decays), the next quake's countdown; the camera trembles by it. Q-03: and on a world
    // with QUAKES; `quakeCue` a quake's onset for the sound (its strength, consumed by the game)
    double quake = 0, nextQuake = 45, quakeCue = 0;
    // the sounds of the ground the world's traits give (`updateGroundSongs`; game/audio.cpp sings them): a slip face's sand sliding under
    // the explorer or the buggy (0..1, held a while as the sand runs on) and the note it hums at (`duneSongHz`), the ice's song round the
    // explorer (0..1: the ice near, the day warming or cooling it)
    double duneSong = 0, duneHz = 80, iceSong = 0;
    // J-10: an overgrowth's sounds round the explorer (`updateGrowthSongs`): its vents breathing near (0..1), its tissue creaking (0..1: its tall and
    // hard structures near, in the wind), its hair hissing (0..1); `tearCue` a membrane pushed through (one-shot: its strength, consumed by the game)
    double ventSong = 0, creakSong = 0, hairSong = 0, tearCue = 0;
    // dust devils on a desert world: columns of dust that wander with the wind for a few minutes
    struct DustDevil { double x, z, height, radius, age, life, phase; };
    std::vector<DustDevil> devils;
    double nextDevil = 20;
    int siteEpoch = 0;   // counts landings and re-anchors, so cached maps of the site (the sector map image) know to rebuild
    double drainCheckT = -1e9;   // when the drainage tiles ahead were last checked
    // the surface scanner's echoes: every sight the world can hold within SCAN_RANGE of the scan's centre (the cells' kinds
    // by `sightsNear`, every ruin by `ruinsNear`), searched on a thread of their own while the game wants them (`scanWanted`: the
    // scanner is on) at a landing, after a re-anchor and when the explorer has gone SCAN_STEP from the last centre; in local metres
    struct ScanEcho { Landmark lm; double x = 0, z = 0; };
    static constexpr double SCAN_RANGE = 40000, SCAN_STEP = 5000;
    std::vector<ScanEcho> echoes;
    bool scanWanted = false, scanned = false;   // the game's wish; a scan has come in for this site
    Vec3 scanU;                                 // where the last scan was asked (body-frame unit: a re-anchor keeps it)
    double scanMs = 0;                          // what the last scan cost on its thread
    std::thread scanThread; std::atomic<bool> scanBusy{false}; std::vector<Landmark> scanOut; double scanOutMs = 0;
private:
    // footprints: ring buffer of recent steps (x, z, heading)
    struct Footprint { float x, z, heading; };
    std::vector<Footprint> footprints;
    int footHead = 0;
    double stepAccum = 0, lastStepX = 0, lastStepZ = 0;
    bool footLeft = false;
    // per-frame cloud layer parameters for ground shadows (M10-14)
    bool cloudShadows = false;
    // the banks the water's trait colours are drawn in this frame (`setupPalette`): 25 a pink lake's, 15 the milky sea's glow at night (a
    // milky sea's world has no springs' spores in it), 26 a bloom's or a reef's lagoon (the second people's banks: no people on those worlds); -1 none
    int tintBankW = -1, tintBankL = -1;
    int tintBankSprings = -1;   // the hot springs' mats (25: a world's water trait is the springs or a pink lake, never both)
    int tintBankStrata = -1;    // the rainbow strata's accent layers (27)
    int tintBankSnow = -1;      // Z-03: red snow's algae (28)
    int tintBankMade = -1;      // J-02: a made world's tint over the land (a bank the world leaves free: `setupPalette`'s pool); J-03: or its waters
    double tintGlowMade = 0;    // J-03: the made waters' glow tonight (a glowing line's; 0 by day)
    int madeBanks[MC_COUNT] = {-1, -1, -1, -1};   // J-05: the land's, the snow's and the plants' made looks, each class's latest line's (the waters' is `tintBankMade`)
    // KI-460, KI-473: each line's own bank where its colour differs (the latest's where none was left): its waters' and their glow tonight, its
    // land's, snow's and plants' looks (by `MadeClass`), its overgrowth's with its glow tonight and its wetness; -1 none
    int tintLineBank[MADE_LINES] = {-1, -1, -1}; double tintLineGlow[MADE_LINES] = {0, 0, 0};
    int madeLineBank[MC_COUNT][MADE_LINES] = {{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}};
    int growthLineBank[MADE_LINES] = {-1, -1, -1}; double growthLineGlow[MADE_LINES] = {0, 0, 0}, growthLineWet[MADE_LINES] = {0, 0, 0};
    // KI-460, KI-473: a line's own bank where it lies (the vertex's `mline` - 1; -1 or none dealt: the latest line's)
    int growthBankOf(int line) const { return line >= 0 && line < MADE_LINES && growthLineBank[line] >= 0 ? growthLineBank[line] : growthBank; }
    int plantsBankOf(int line) const { return line >= 0 && line < MADE_LINES && madeLineBank[MC_PLANTS][line] >= 0 ? madeLineBank[MC_PLANTS][line] : madeBanks[MC_PLANTS]; }
    // J-15: a desert's carpet's grass and shrubs: whether any lie on the world now, and whether a cell's grass is theirs (drawn in the grass
    // bank in their colour, with no trees)
    bool greening() const { return site.gen.made.green > 0 && madeGreens(site.gen.type); }
    bool greenedAt(const TerrainVertex& tv) const { return tv.material == MAT_GRASS && tv.mline > 0 && greening(); }
    int madeRainBank = -1;      // KI-459: a dyed rain's bank and its puddles' (the waters', else one of its own; none: the made clouds' 23, else the sky's)
    double madeGlowPlants = 0;  // J-05: a glowing forest's light tonight (0 by day)
    double madeGlowLand = 0;    // J-16 (KI-506): a deep line's glowing cracks' or seeps' light tonight (0 by day)
    int madeFoamBank = -1;      // J-05: a foam's colour on the shores (a lent bank left free, else the snow's white)
    bool madeOnSea = false;     // J-05: the made waters' tint lies on the sea (a line with a waters' look lives in it): the tide's flooded flats take it
    bool madeCloudBank = false; // J-04: a seeded world's clouds drawn in bank 23 (coloured by day, glowing at night: `setupPalette`)
    // Z-03: the films under the lamp's ultraviolet: the world's biota (its systems' kept alive), each niche's density, the glow's colour; this
    // frame's grid of the near ring's corners round the eye (their height and their films' glow), the luminance of every bank's shades
    std::shared_ptr<const SystemLife> life;
    double filmD[NI_COUNT] = {0};
    Vec3 filmColour;
    static constexpr int UV_GRID = 17;   // 4 m corners: 64 m across
    int uvCx0 = 0, uvCz0 = 0;
    float uvH[UV_GRID * UV_GRID], uvS[UV_GRID * UV_GRID];
    bool uvFrame = false;   // the pass runs this frame (the lamp on, on foot, a film anywhere in the grid)
    double tintGlow = 0;   // the milky sea's glow tonight (0 by day)
    // the sun's screen position and radius this frame (framebuffer pixels) and the strength of the veiling glare
    // drawn over the finished picture (0 = none: below the horizon, eclipsed, behind the camera)
    double sunSx = -1, sunSy = -1, sunRpx = 0, sunVeil = 0;
    double cloudWx = 0, cloudWz = 0, cloudThr = 1;
    double lastT = 0;
    // M10-14 galactic band: integrated star density over galactic directions (lon x lat map)
    std::vector<float> bandMap;
    NebulaPatch nebP[16]; int nebN = 0;
    // mirrored terrain into water pixels. F-05: gathered once a frame (serial: during the descent the cells it reads are not warmed
    // and the caches fill on a miss), each polygon's vertices kept with the rows it can fill, and drawn by the terrain's band threads after
    // the rings (every pixel blended by the same polygons in the same order, far to near)
    struct ReflPoly { int first, n, y0, y1; };   // its vertices at reflVerts[first, first + n); the rows y0..y1 (inclusive)
    std::vector<RVert> reflVerts;
    std::vector<ReflPoly> reflPolys;
    // front/sy = the projected row, for the band cull; wy = the water level as drawn (morphed with the ground), -1e9 none; hy = the height
    // as drawn (morphed), sh = the side of the water's edge as drawn (`vertexShore`, morphed), for the shore cut and the water's depth;
    // wv = the water sheet's corner here when the vertex has its own level (`waterCorner`)
    struct VtxCache { int stamp = -1; RVert v; uint8_t bank = 0; bool front = false; float sy = 0; float wy = -1e9f; float hy = 0; float sh = 1e9f; RVert wv; };
    std::vector<VtxCache> vc0, vc1, vc2, vc3, vcN;
    // one jet: a dense core column, a spray crown and a falling veil, `heightM` and `widthM` at full strength
    struct JetSpec { double x, z; uint64_t h; double dist; double heightM, widthM; int bank; bool lava; double shadeAdd = 0; };   // shadeAdd darkens a jet of dust
    double floorH = 0;      // this frame's, for the floor and the galactic band's horizon
    std::vector<std::pair<int, int>> fillMisses;   // the warm's list of its disc's misses (kept: no allocation a frame)
    // geomorphing: while a ring is warmed, the next coarser ring (already warmed) and the width of the morph band
    // in cells; a vertex within the band takes the coarser ring's interpolated height and shade in proportion to its
    // (continuous) distance from the camera, so the two surfaces coincide at the ring's edge and the finer relief rises
    // as the edge sweeps over the ground instead of popping cell by cell
    TerrainCache* morphCoarse = nullptr;
    std::vector<VtxCache>* morphVc = nullptr;
    double morphBand = 0;
    // the coarser ring's drawn surface at (x, z): its cell, the position within it, which of the two triangles, the height
    struct CoarseHit { int X, Z; double fx, fz; bool lower; double h; };
    // the terrain's cells binned once a frame. After the warm, one pass on the pool (by rows of cells) runs the culls each band
    // thread would run on every cell (the distance, the cover, the frustum, the corners behind the camera) and keeps each cell
    // that draws, in the draw's order, with the rows its pieces can touch (the ground's triangles or its shore cut, the water's
    // sheet, the outermost ring's walls; projected as the rasteriser projects, a row of slack each side). A band thread then draws,
    // in that order, the cells that reach its rows: every pixel is written by the same cells in the same order as before
    struct TerrainCellRef { int cx, cz; int16_t y0, y1; };   // y0..y1: the rows, inclusive (INT16_MIN..INT16_MAX: every band)
    struct TerrainBin { std::vector<TerrainCellRef> cells; std::vector<int> count; int cols = 0; };   // row j's cells at [j * cols, j * cols + count[j])
    TerrainBin terrainBins[5];
    struct FloraStats { int treesDrawn = 0, floraPoints = 0, floraPolys = 0, nearTrees = 0, midTrees = 0, farTrees = 0, treesHidden = 0, treesVisited = 0; double ms[2] = {0, 0}; };
    // the flora cells in view (near to far) and their trees, listed once per frame by drawFlora with the rows a
    // tree can touch (its trunk, canopy, sway and shadow), so a band thread places only the trees that reach its rows
    struct FloraCell { float d; int cx, cz; };
    struct TreeRef { TreeInst T; float dist; int y0, y1; };
    std::vector<FloraCell> floraCells;
    std::vector<TreeRef> treeList;
    // the forms' frame: the items gathered (a form, its foot, its level of detail, the people whose banks it takes; a
    // silhouette at level 3), the chunks the workers build them into (the polygons and lines in view space, lit and fogged,
    // with the rows they touch), the raster parameters of the peoples' banks and windows
    struct FormItem { const Form* f; Vec3 foot; double skirt, d, fog; int lod, people; float climb = -1; int gline = -1; };   // J-10: the cover's climb on it (-1
    // none); KI-473: its settlement's growing line
    struct FormPolyOut { int first, n, rp, y0, y1; float s2 = 1e9f; };   // (KI-492: its second bank's shade offset, over 1e8 its raster's own)
    struct FormLineOut { RVert a, b; int bank, thick, y0, y1; };
    struct FormChunk { std::vector<RVert> v; std::vector<FormPolyOut> polys; std::vector<FormLineOut> lines; int silhouettes = 0; };
    std::vector<FormItem> formItems;
    std::vector<FormChunk> formChunks;
    std::vector<RasterParams> formRps;
    FacePattern formPatterns[2][4];
    double waterWdx = 0, waterWdz = 0, waterSunUp = 0;   // the frame's wind direction (sin, cos) and the sun's rise, for waterCorner
    // R-434: the activity vision (sampling.cpp): the square metres round the eye read as a vial reads them, in a ring SIGHT_N metres a side
    // anchored in the local metres (a slot its metre's key and the heat of what is at work there: 0 none, -1 not read), the sea level they were
    // read at (a tide past a quarter metre reads them again); this frame's grid of the drawn ground's or water's height round the eye (corners
    // 2 m apart); the pass this frame (the vision on, on the ground)
    static constexpr int SIGHT_N = 128, SIGHT_GRID = 45;
    std::vector<int64_t> sightKey;
    std::vector<float> sightHeat;
    double sightSea = -1e30;
    int sightCx0 = 0, sightCz0 = 0;
    float sightTop[SIGHT_GRID * SIGHT_GRID];
    bool sightFrame = false;
    // J-06: the animals drawn: each species' rig (built at the landing from the bestiary, in its order), the world's two hide banks
    // (`setupPalette`), this frame's animals to pose (its rig, its pose's inputs, its detail, its pixels a metre, its fog), each posed on the
    // pool into a shape list of its own and the lists filled in bands in their order
    std::vector<AnimalRig> rigs;
    int hideBanks[2] = {3, 0};
    // J-07: what it does (-1 the harness's); J-08: its fate drawn (the changed rig, remains' stage, a sickness, a changed young's marks, its
    // hash) and the record's marks it gives (`AnimalMark` bits)
    struct LifeJob {
        int rig = 0; AnimalPoseIn in; int detail = 0; double pxPerM = 1, fog = 0; int doing = -1; double dist = 0; bool eyeshine = false;
        bool changed = false; int dead = 0, sickness = 0, marks = 0; uint64_t looks = 0, seed = 0;
        int over = -1;   // J-09: remains an overgrowth took in, drawn in its bank (-1: in the hides')
    };
    std::vector<LifeJob> lifeJobs;
    std::vector<ShapeList> lifeLists;
    // the harness's specimens (`bestiary plates`): a species each where it is put, standing (0), aloft (1) or afloat (2); the game puts none
    // J-07: a gait (-1 still) at a phase; J-08: remains' stage, a sickness, a changed young's changes drawn (its species' changed rig)
    struct Specimen {
        int species = 0; Vec3 at; double heading = 0; int how = 0; double flap = 0.3; int gait = -1; double phase = 0, stride = 0; bool running = false;
        int dead = 0, sickness = 0; bool changed = false; bool night = false; double scale = 1;
    };
    std::vector<Specimen> specimens;
public:
    // J-08: the animals' fates on a seeded world (none on a world nobody seeded): each species' changes on its world (`animalChangesOf`, from
    // every line's steering) and its changed young's rig (its own where it has none), the remains on the ground (the cells', the kills', old
    // bones), the bank of a dyed hide or a new colour (-1 none), whether every animal is gone (silence)
    struct SpeciesChanges { int changes[3] = {-1, -1, -1}; int n = 0; uint64_t drawn = 0; };
    std::vector<SpeciesChanges> speciesChanges;
    std::vector<AnimalRig> changedRigs;
    std::vector<Remains> remains;
    // B-470: the landing's clock (its seconds since the landing, as the guide's runs on the ground: real time) and its dead kept on the body
    // (cleared at the landing): a kill's quarry or a sprouting one fallen, by its seed, its herd's cell (`cell` false: a landing's herd, never
    // forgotten), where it lies (its unit vector), its species, heading and size, its death on the clock, how long the hunters fed at it, with
    // stalks, and whether its remains are in `remains` now. B-489: the world's kept by the guide from the landings before come first (`deadIn`,
    // died before the landing: under 0), this landing's after them in the order they died, which the game copies to the guide
    double lifeClock = 0;
    struct DeadKept {
        uint64_t seed = 0; int gLat = 0, gLon = 0; bool cell = false; Vec3 unit; int species = -1;
        double heading = 0, size = 1, diedAt = 0, eaten = 0; bool stalks = false, shown = false;
    };
    std::vector<DeadKept> deadKept;
    // B-490: a sprouting flyer falling dead from its flock (its species, where, how fast it falls, its heading and size, its seed, sick and
    // changed), its remains kept where it lands; the swimmers found dead out of reach of a shore (not looked for again)
    struct FlockFall { int species = -1; double x = 0, y = 0, z = 0, vy = 0, heading = 0, size = 1; uint64_t seed = 0; int sick = 0; bool changed = false; };
    std::vector<FlockFall> flockFalls;
    std::vector<uint64_t> swimmersSunk;
    // B-467: a 100 m cell's place of a swimmer of the shallows, kept for the landing (local metres; cleared at the landing and a new anchor): a
    // stretch of the water 0.9 m deep it swims along, its points' count
    struct ShallowsPath { bool ok = false; int n = 0; double x[7] = {}, z[7] = {}; };
    std::unordered_map<uint64_t, ShallowsPath> shallowsPaths;
    int fateBank = -1;
    // J-09: an overgrowth's bank (-1 none: its cover in the rock's), its veins' light tonight (0 by day) and its wetness (the gloss)
    int growthBank = -1;
    double growthGlow = 0, growthWet = 0;
    // a structure the frame draws, chosen on the main thread: its size in 1x pixels, the ground's slope at its foot (its parts stand on
    // that plane)
    struct GrowthDraw { GrowthInst S; double px = 0, sx = 0, sz = 0; };
    std::vector<GrowthDraw> growthDraws;    // (the frame's structures, their shapes made in sixteen lists on the pool, filled in bands)
    ShapeList growthLists[16];
    int lastGrowthsDrawn = 0;
    bool silence = false;
public:
    // J-07: what the landing saw of each species (its doings' bits, the nearest it came, the most in view at once; `fresh` since the game last
    // read it), and the animal the crosshair rests on this frame (its species, -1 none; how far)
    // J-08: and the marks it was seen with (`AnimalMark` bits: remains or a sick one, none of it living there, a changed young)
    struct SeenAnimal { uint32_t doing = 0; double nearest = 1e9; int most = 0; bool fresh = false; uint32_t marks = 0; };
    std::vector<SeenAnimal> lifeSeen;
    struct LifeTarget { int species = -1; double dist = 0; };
    LifeTarget lifeTarget;
    struct LifeDrawn { int species = -1, doing = 0; double dist = 0, sx = 0, sy = 0, r = 0; int marks = 0; };   // this frame's animals drawn whole (the record's, the target's)
    std::vector<LifeDrawn> lifeDrawn;

public:
    // ---- the landing, the frame and the camera; the terrain's streaming, the scanner's thread (surface_view.cpp)
    ~SurfaceView() { joinAhead(); joinScan(); }
    SurfaceView() = default;
    SurfaceView(const SurfaceView&) = delete;
    SurfaceView& operator=(const SurfaceView&) = delete;
    void init(const StarSystem* sys, int bodyIndex, double lat, double lon, double t);
    void update(double dt, const Input& in, double t, bool controlsEnabled);
    void render(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr, double fade);
    void relocateCapsule(double x, double z);
    // Fill the terrain caches around the site in coarse-to-fine order within a time budget (KI-009).
    void prefetch(double budgetMs);
    // Move the site frame to the player's position (KI-007); local coordinates are rebuilt.
    void reanchor();
    void setFov(double deg);
    void enterFreeCam();
    void leaveFreeCam() { freeCam = false; }
    void updateFreeCam(double dt, const Input& in);   // also refreshes the environment for the camera's spot
    bool inVehicle() const { return inBuggy || inDrone; }
    std::string bandTraceLine(bool flora) const;   // the bands as one line (the harness, the trace)
    bool projectPoint(double x, double y, double z, double& sx, double& sy) const;   // world -> framebuffer pixels
    Mat3 testCamWorld(double t) const { return camLocal * site.localFrame(t); }        // tests: the world->view camera of the last frame
    void startScan();              // the search of the sights round the explorer, on a thread of its own
    void collectScan();            // the main thread takes a finished scan (update)
    void joinScan();
private:
    void computeRings();
    void updateNearRing(double dt);
    void prefetchAhead();
    void joinAhead();
    void drawWaypointLine(Framebuffer& fb);
    void buildDirLUT();
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

public:
    // ---- the environment: the light and the weather, the extinction of a ray, the fog's floor, the ground's songs (surface_env.cpp)
    // W-03: the extra extinction of the ray from the camera to a point: the wall of rain (or dust) of the front between them
    // TR_FOG_SEAS: the fog sea a ray from an eye over its top crosses down to a point under it, as an extinction like the rain's (its
    // depth along the ray per 30 m of fog; the top uneven by a few metres); nothing from an eye inside it (the fog's distance does that)
    double fogSeaWall(double x, double y, double z, double dist) const {
        if (env.fogSea <= 0 || camPos.y <= env.fogSeaTop) return 0.0;
        const double top = env.fogSeaTop + 6.0 * gnoise2(x / 260.0, z / 260.0, site.gen.seed + 0xF06);
        return y >= top ? 0.0 : (top - y) * dist / std::max(1.0, camPos.y - y) / 30.0;
    }
    void refreshFogFloor();
    double rainWall(double x, double z, double dist) const { return localFront.on ? localFront.wall(localFront.ahead0, localFront.ahead(x, z), dist, env.frontRain) : 0.0; }
    void updateGroundSongs(double dt, double t);
    void updateGrowthSongs(double dt, double px0, double pz0);   // J-10 (overgrowth.cpp): (px0, pz0) where the explorer stood before this frame's move
    static double duneSongHz(const BodyGen& g, double gravity);   // the grains' note: Andreotti's 0.4 sqrt(g / d), d 150-320 microns by the world
private:
    void computeEnvironment(double t);

public:
    // ---- the peoples' places: the cells, the marks, the plan's places; the records, the instruments and the graves (surface_places.cpp)
    bool ruinAt(int gLat, int gLon, Ruin& r) const;   // the settlement of a 2 km latitude/longitude cell, in local metres
    void ruinCellAt(double x, double z, int& gLat, int& gLon) const;
    const RuinCell* ruinCell(int gLat, int gLon) const;
    void findNearShard();
    void drawShards(Framebuffer& fb, double t);
    static void recordObjects(RuinCell& c);   // T-04: the cell's records' objects from its sites
    void gatherRecords();                     // the records' objects in reach into the frame's forms
    void findNearGrave();
    void findNearInstrument();
    void instrumentObjects(RuinCell& c) const;   // the cell's instruments' ids and objects from its sites
    // the cells within 600 m of the explorer lay their records and instruments again, once, with the water as drawn (their rivers and
    // lakes: the generation's placing reads the sea alone, a frame cannot wait for the rivers' tiles); the glints are drawn within 250 m
    void waterRecords();
    void refreshPlanNear();
    const RuinCell* workCell(const Work& w) const;
    // the towns in reach put their streets, squares and fields into the ground (`SurfaceSite::addTownMarks`), checked as the explorer moves
    // (T-03: and the works their cuts, pits, ditches, craters, flats, reservoirs and lines)
    void ensureTownMarks();
    // the ruins of the cells within `reach` cells of (x, z): fn(ruin, cell, ring), ring the cell's distance in cells; T-01: and a plan's
    // place whose core comes within that reach from a cell farther off
    template <class F> void forNearbyRuins(double x, double z, int reach, F fn) const {
        if (!worldHadCivilisation(site.gen)) return;
        double lat, lon; site.latLonAt(x, z, lat, lon);
        double dLat = ruinCellLat(site.gen);
        int gLat = (int)std::floor(lat / dLat);
        uint64_t seen[64]; int nSeen = 0;
        for (int dl = -reach; dl <= reach; dl++) {
            int row = gLat + dl;
            double latc = (row + 0.5) * dLat;
            if (std::fabs(latc) > PI / 2 - dLat) continue;
            double dLon = dLat / std::max(std::cos(latc), 0.05);
            int g2 = (int)std::floor(lon / dLon);
            const int nLon = std::max(1, ruinCellsAround(site.gen, row));
            for (int dd = -reach; dd <= reach; dd++) {
                if (nSeen < 64) seen[nSeen++] = ((uint64_t)(uint32_t)(row + 0x100000) << 32) | (uint32_t)((((g2 + dd) % nLon) + nLon) % nLon);
                const RuinCell* c = ruinCell(row, g2 + dd); if (c->has) fn(c->r, *c, std::max(std::abs(dl), std::abs(dd)));
            }
        }
        const double cellM = dLat * site.R;
        for (const PlanNear& pn : planNear) {
            const double d = std::hypot(pn.x - x, pn.z - z) - pn.radius;
            if (d > reach * cellM) continue;
            const uint64_t key = ((uint64_t)(uint32_t)(pn.gLat + 0x100000) << 32) | (uint32_t)pn.gLon;
            bool done = false; for (int i = 0; i < nSeen && !done; i++) done = seen[i] == key;
            if (done) continue;
            const RuinCell* c = ruinCell(pn.gLat, pn.gLon);
            if (c->has) fn(c->r, *c, std::max(1, (int)std::ceil(std::max(0.0, d) / cellM)));
        }
        for (const WorkNear& wn : workNear) {   // the works
            const double d = std::hypot(wn.x - x, wn.z - z) - wn.radius;
            if (d > reach * cellM) continue;
            const RuinCell* c = workCell(*wn.work);
            if (c && c->has) fn(c->r, *c, std::max(1, (int)std::ceil(std::max(0.0, d) / cellM)));
        }
    }
private:
    void drawRuins(Framebuffer& fb);

public:
    // ---- on foot: the walk, the colliders and the supports, the boulders' enumerators and the objects drawn, the capsule, the shadows, the steps (surface_walk.cpp)
    void collectColliders(double x, double z, std::vector<Collider>& out);   // rocks and trunks near (x, z)
    // B-420: a 16 m cell's boulders, one enumerator for the drawing and the colliders (none in a room, on a road's or a street's bed, on a
    // work's ground, under water); `keep` a cheap test before the rest; returns the cell's candidates
    int forRocks(int cx, int cz, const std::function<bool(double, double, double)>& keep, const std::function<void(const RockInst&)>& fn);
    // a 16 m cell's stromatolites (a microbial world's, KT_MICROBIAL): low domes 0.7-3.2 m across packed on its reef flats, a few on its
    // mats, standing from the water's edge; the drawing and the colliders both (`RockInst`: size the radius, gy the foot)
    void forDomes(int cx, int cz, const std::function<void(const RockInst&)>& fn);
    void forChimneys(int cx, int cz, const std::function<void(const RockInst&)>& fn);   // the chemotroph's sinter chimneys a 16 m cell (KT_VENT_LIFE)
    // J-09: a 16 m cell's overgrowth structures (KI-476: those of the world's lattice whose middles lie in it: KI-490, of a world cell whose
    // four corners its cover holds; none in water, a room, on a road, a street or a work's ground, none overlapping); the drawing, the
    // colliders and the trees read it (a tree tests the cells round it too: a structure can reach over a cell's edge)
    void forGrowths(int cx, int cz, const std::function<void(const GrowthInst&)>& fn);
    const GrowthCell& growthCellOf(int face, int i, int j, int N);   // KI-476: a world cell's structures (`growthLattice`)
    const std::vector<GrowthInst>& growthsOwned(int cx, int cz);       // KI-476: a local cell's, before what the landing lays is tested
    bool growthOn() const;                                             // a layer with an overgrowth
    // KI-489: the lattice's world cell a unit vector lies in (its face, its two indices), returning the cells a face side has
    static int growthLatticeAt(const Vec3& unit, double radiusM, int& face, int& i, int& j);
    bool growthCornerHolds(int face, int ci, int cj, int N);           // KI-490: the cover at a world cell's corner, out of the water
    bool growthCornerAt(int face, double al, double be) const;         // (the same read, kept nowhere: the resample's worker)
    void buildGrowthVeins();                                           // KI-492: `growthVeins`, a landing's
    int growthAt(double x, double z, double margin);   // KI-476: the shape of the structure standing at local (x, z), -1 none (a vial's)
    void growthColliders(const GrowthInst& S, std::vector<Collider>& out) const;   // the discs it stands in the way with
    // a structure the frame draws (`GrowthDraw`); its shapes into a list in the overgrowth's bank and tile, on the pool (it reads the
    // view and never the ground)
    bool growthVisible(const GrowthInst& S, double& px) const;
    void drawGrowth(ShapeSink& out, const GrowthDraw& D, double t, double ambient, double sunUp, double lf, const GrainTexture* tile) const;
    // KI-481: a point of the overgrowth lit, a structure's or a ruin's cover and its pieces: the sky by how much it looks up, the sun, the lamp, a
    // wet one's gloss toward the eye (`half`: the sun's and the eye's halfway), `boost` shades over (the fog not yet in); and a face's night light
    // where it glows (a fifth or so of them, `face` its hash, pulsing slowly; 0 by day, on a face that does not glow, on a world where none does)
    double growthShade(const Vec3& p, Vec3 n, const Vec3& half, double ambient, double sunUp, double lf, double boost, int line = -1) const;   // (KI-473: its line's)
    double growthGlowShade(uint64_t face, double t, int line = -1) const;
    void forMeteorites(int cx, int cz, const std::function<void(const RockInst&)>& fn);   // a strewn field's iron masses a 16 m cell (TR_METEORITES)
    void collectSupports(double x, double z, double reach = 6);
    double floorAt(double x, double z, double feet, bool* blocked = nullptr);
    void settlementColliders(const Ruin& r, const RuinCell& cell, double x, double z, std::vector<Collider>& out);
    // the settlements within reach of a point (at most four, within their radius plus 50 m) and whether a point lies in
    // one of their buildings (its rectangle plus a margin): the rocks, the logs and the rock colliders keep out of the rooms
    int settlementsNear(double x, double z, const RuinCell* near[SETTLEMENTS_NEAR]) const;
    static bool inSettlementBuilding(const RuinCell& c, double x, double z, double margin);
    bool deployBuggy();              // B near the capsule: a new buggy, any old one is scrapped
    bool toggleBuggy();              // E: enter or leave
private:
    void drawBlobShadow(Framebuffer& fb, double x, double z, double radius, double height);
    void drawBlobShadowFrom(Framebuffer& fb, double x, double z, double radius, double height, const Vec3& sunDir, double share);
    void drawFootprints(Framebuffer& fb);
    void updateWalking(double dt, const Input& in, double t, bool controlsEnabled);
    void drawObjects(Framebuffer& fb, double t);
    void drawCapsule(Framebuffer& fb, double t);

public:
    // ---- the palette and the lamps (surface_palette.cpp)
    void setupLamps();
    double lampAt(const Vec3& p, const Vec3* n = nullptr) const;   // the light the frame's lamps add at p (local metres), facing them by n where given
private:
    void setupPalette(Framebuffer& fb);

public:
    // ---- Z-03: what a vial takes at a place (its microhabitat, its name, the temperature read), the films under the lamp's ultraviolet (sampling.cpp)
    SampleRead readSample(double x, double z, bool onWater, bool walls = true);   // R-434: `walls` false leaves the ruins' to the main thread
    double placeMeanC(double x, double z, double h) const;   // the place's own mean temperature the year round
    bool wallNear(double x, double z);                        // an old wall's foot within 1.2 m (the ruins' cells: the main thread's)
    Microhabitat wallsDone(const SampleRead& r, double x, double z);   // R-434: a read without the walls finished with them (the main thread)
    static float sightHeatOf(const Sample& s);               // R-434: the heat a vial's reading shows (0 none at work, 0.25-0.55 faint, 0.62-1 strong)
    float sightHeatAt(double x, double z) const;             // the square metre's as read so far (-1 not yet)
    void clearSight();
    const Biota& biota() const;                               // the site's world's
    static int filmNicheOf(const TerrainVertex& v, bool springs, double sea, double& share);   // the films a vertex holds: their niche (-1 none) and its share
    double seaLevelNow() const;
private:
    void setupFilmGlow();             // at the landing: the films' densities and colour
    void prepareFilmGlow();           // a frame's grid (the main thread, before the pass)
    void drawFilmGlow(Framebuffer& fb);   // the pass: after the old result, absent to the bit when off
    void prepareSight();              // R-434: the square metres not yet read (the nearest first, on the pool), the frame's grid
    void drawSight(Framebuffer& fb);  // the pass: the whole frame into the vision's ramp (bank 15), absent to the bit when off
public:
    // ---- the terrain rings: the material tiles, the vertex, the warm, the bin and the draw, the reflections, the floor, the cached ground (surface_terrain.cpp)
    // O1 (B-302): the rangefinder: march the camera's line of sight over the terrain caches; returns the distance to the
    // ground (or the water) it hits within maxDist, -1 for the sky. The last frame's answer is kept for the HUD and for M
    double rangeToGround(double maxDist, double& hitX, double& hitZ, bool& water);
    double drawRadiusM() const { return ringR[3] ? ringR[3] * 2048.0 : (ringR[2] ? ringR[2] * 512.0 : ringR[1] * 64.0); }   // how far the terrain is drawn
private:
    const GrainTexture* mesoFor(int material);
    int growthTexture(int material) const;   // J-09: an overgrowth's tile's look (-1 the hashed one)
    // KI-493: a vertex's tile: a crust line's crust (its look's share over a half) in its line's form, a world's own crust as the world's
    int madeCrustForm(const TerrainVertex& tv) const;   // -1 for any other ground
    const GrainTexture* tileOf(const TerrainVertex& tv);   // (the warm makes a made crust's first: a band thread never makes one)
    void buildMesoTiles(uint32_t want);   // the tiles of a set of materials not made yet, a material to a worker
    double castShadowLit(TerrainCache& cache, int cx, int cz, double h, const Vec3& sd);
    void gatherReflections();
    void drawReflectionBand(Framebuffer& fb, int bandY0, int bandY1) const;
    // the coarser ring's shore value at (x, z), bilinear; 1e9 where a corner has none
    double coarseShoreAt(TerrainCache& c, double x, double z);
    double coarseWaterAt(TerrainCache& c, double x, double z);   // the coarser ring's water level at (x, z), bilinear over its wet corners; -1e9 none
    double heightNear(double x, double z, double dist, double& water);   // the cached height (and water level) at a distance from the camera, coarser far away
    double drawnGround(double x, double z, double dist, double& water, int& ring);   // the drawn ground: the near rings' within lod0's reach, a coarse ring's surface beyond (`ring` 0..3)
    int drawnRing(double dist) const { return dist < ringR[0] * 16.0 * 0.9 ? 0 : (dist < ringR[1] * 64.0 * 0.9 ? 1 : (dist < ringR[2] * 512.0 * 0.9 || !ringR[3] ? 2 : 3)); }
    void drawFloor(Framebuffer& fb);
    double floorHeight();   // the floor's height beyond the last ring: the sea, or 300 m under the lowest far cell (0 on a small body)
    // the terrain is drawn in parallel bands of rows: warmTerrainLOD fills the vertex cache and the material tiles first,
    // then every band thread draws all rings clipped to its rows (F-04: the cells binned by the rows they reach, binTerrainLOD),
    // reading only. The frame is identical to the serial loop's.
    // the cells the warm samples (`cache.at`) are its disc's misses; over FILL_SERIAL_MAX of them (a landing's first frame, a
    // re-anchor, a hop: thousands) they are sampled on the pool, each by one worker into its own slot, the pool's join the barrier.
    // the vertices are made on the pool by rows of vertices, each by the thread of its row
    void warmTerrainLOD(TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM, TerrainCache* coarse, std::vector<VtxCache>* coarseVc, int band);
    bool cellCovered(double cs, int cx, int cz, double coverM) const;   // the cell lies wholly under the finer ring (every corner within coverM of the camera)
    CoarseHit coarseAt(TerrainCache& c, double x, double z);
    void binTerrainLOD(TerrainBin& bin, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM, bool water, bool skirt,
                       int rowB, int rowE, int& stale, int& allRows);
    void drawTerrainLOD(Framebuffer& fb, const TerrainBin& bin, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM,
                        bool water, double t, int bandY0, int bandY1, bool skirt, BandTrace* bt);
    const RVert& vertexOf(TerrainCache& cache, std::vector<VtxCache>& vcache, int cx, int cz, uint8_t& bank);
    // a corner of a cell's water sheet at (x, z): its place in view and its shade from the level `wl` and the drawn ground `hy`
    // under it. A vertex with its own level keeps its corner (`VtxCache::wv`, made by vertexOf in the warm, once, not by four cells
    // and every band again); a corner without one takes its cell's mean level and is made by the cell
    RVert waterCorner(double x, double z, double wl, double hy, double jelly = 0) const;   // J-05: `jelly` a jelly sea's share here (its slow heavy waves)
    void materialLook(const TerrainVertex& v, uint8_t& bank, double& albedo) const;
    // the points of a terrain cell's pieces that are not its corners, the same for the draw and the bin: a shore cut's point (O6-03:
    // the dry piece's edge SHORE_LIP over the water), the foot of the outermost ring's wall under a corner (KI-006: a cell and a half down)
    Vec3 shoreCutView(const ShorePt& p, bool dry) const { return toView(p.x, p.h + (dry ? SHORE_LIP : 0.0), p.z); }
    Vec3 skirtFoot(TerrainCache& cache, double x, double z) const {
        const double cs = cache.cellSize, skirt = cs * 1.5;
        const TerrainVertex& tv = cache.at((int)std::lround(x / cs), (int)std::lround(z / cs));
        return toView(x, tv.h - skirt, z);
    }

public:
    // ---- the sky: the galaxy's band, the aurora's oval, the sky drawn, the shower meteors, the veiling glare (surface_sky.cpp)
    double auroraOvalNow(const Body& b) const { return auroraOvalLat(b) - (site.gen.hasTrait(TR_GREAT_AURORAE) ? 22.0 : 0.0); }   // Q-02: a great aurorae's oval reaches the middle latitudes
    // the auroral oval's latitude on a world: nearer the pole on a strong field
    static double auroraOvalLat(const Body& b) { return 59.0 + 8.0 * magneticField(b) + 6.0 * (unitFromHash(hashCombine(b.seed, 0xA17)) - 0.5); }
    void showerMeteorsAt(double t, std::vector<ShowerMeteor>& out) const;
private:
    void drawVeil(Framebuffer& fb);
    void buildBandMap();
    double bandAt(const Vec3& dirWorld) const;
    void drawSky(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr);

public:
    // ---- what the world does round the explorer: the vents' jets, the geysers, the spores, the plumes, the curtains, the lamps left lit, the weather (surface_effects.cpp)
    static double basinJetStrength(uint64_t id, double t);   // R-307: the strength of a geyser basin's vent (its constant/cycle/duty by hash), for the drawing and the tests
    static double burstJetStrength(uint64_t id, double t);   // a steam vent's bursts (EV_STEAM_BURSTS: its cycle and duty by hash)
    bool springJetsNow(double t) const;              // the local spring with the sun up: the hemisphere's insolation rising from its winter
    static double springJetStrength(uint64_t id, double t);   // a spider's jet (its cycle and duty by hash)
    static bool plumeActive(uint64_t id) { return unitFromHash(mix64(id + 1)) < 0.5; }   // half the plume vents are active
private:
    void drawCometJets(Framebuffer& fb, double t);   // dust jets venting from the sunlit ground of an active comet
    void drawGeysers(Framebuffer& fb, double t);     // the geysers (europan cracks, geyser basins, tectonic fumaroles and steady fountains)
    void drawSpores(Framebuffer& fb, double t);      // the chemotroph's glowing spores over its springs at night (EV_SPRINGS)
    void drawMadeAir(Framebuffer& fb, double t);     // J-04: a seeded world's dyed rain's puddles and the spores in its air
    void drawMadeGround(Framebuffer& fb, double t);  // J-05: its foam, its boiling seas, glints, pits, a dark ice's meltwater
    void drawPlumes(Framebuffer& fb, double t);      // the cryovolcanic world's dark plumes and their flags (EV_PLUMES)
    void drawSpringJets(Framebuffer& fb, double t);  // the dry ice world's jets through its spring frost (EV_SPRING_JETS)
    void drawCurtains(Framebuffer& fb, double t);    // the tiger stripes' curtains of ice and the jets along them near the explorer (EV_CURTAINS)
    void drawJet(Framebuffer& fb, double t, const JetSpec& j, double strength);
    static double jetStrength(uint64_t h, double t, bool constant, double cycle, double duty);   // 0 off .. 1 full (1.35 in the first seconds of a burst)
    void drawStreetLamps(Framebuffer& fb);           // TR_LIGHTS_LEFT_ON: a dead people's lamps still lit along its streets
    void drawWeather(Framebuffer& fb, double t);

public:
    // ---- the flora: the trees and the logs, the canopy, the looks, the draw in bands (flora.cpp)
    // N2 vegetation: every tree and log of a 16 m cell, deterministic; the canopy density field (veg x clearings)
    void forTrees(int cx, int cz, const std::function<void(const TreeInst&)>& fn);
    void forLogs(int cx, int cz, const std::function<void(const LogInst&)>& fn);
    double canopyDensityAt(double x, double z, double veg, bool lush = true) const;   // (KI-463: `lush`, a lush world's denser, under 1)
    double lushK() const;
    double seasonPhaseLocal() const;               // -1 deep winter .. +1 high summer for this site's hemisphere (0.5 without tilt)
private:
    void buildLooks();
    void drawFlora(Framebuffer& fb, double t);     // trees, undergrowth, meadows, reeds, lily pads, cacti, lichen, fireflies (B-310: in parallel bands)
    void drawFloraBand(Framebuffer& fb, double t, int bandY0, int bandY1, FloraStats& st);

public:
    // ---- the creatures: the herds and the flocks, their habitat cells, their update and draw (creatures.cpp)
    static void planLife(const SurfaceSite& s, const Bestiary& B, uint64_t seed, std::vector<Critter>& critters, std::vector<Flock>& flocks, std::vector<Herd>& herds);
    static void planFlocks(const SurfaceSite& s, const Bestiary& B, Rng& r, int biome, std::vector<Flock>& flocks);
    void lifeAnchored(double lat, double lon);   // B-470: the frame anchored again at the explorer (`reanchor`): the flocks there, the cells' again
    void spawnLife(double lat, double lon);
    static void lifeCellAt(const SurfaceSite& s, double x, double z, int& gLat, int& gLon);
    // the herd a habitat cell holds, if any: its centre (local metres), species and size; deterministic
    static bool planCellHerd(const SurfaceSite& s, const Bestiary& B, int gLat, int gLon, Herd& h, int& species, int& n);
    void updateHabitat();
    // J-08: a species' fate at a point (local metres) on this world's layer (none on a world nobody seeded)
    AnimalFate fateAt(int species, double x, double z) const;
    // J-08: a habitat cell's remains, a pure function of the cell (and the world's layer): its herd's dead where they lived and at the water (on
    // high ground for a sprouting one), a few old bones (any world), the flyers fallen in it, the swimmers washed up on its shore. `hd`, `spi`,
    // `n` its herd (`planCellHerd`'s; null none)
    void cellRemains(int gLat, int gLon, const Herd* hd, int spi, int n, std::vector<Remains>& out);
    // J-08: a herd's members as a cell draws them (`n` of them, a share of the last dead: their places are the remains')
    void cellMembers(const Herd& hd, int spi, int n, uint64_t hs, std::vector<Critter>& out) const;
    // J-08: remains' stage by their fate's age where they lie, their hash, a flesh rotter's pace (old bones: bones or scattered)
    int remainsStageOf(uint64_t seed, bool old, double age) const;
    int remainsStageAfter(double steps) const;   // B-470: the stage `steps` after a death (a kill's, a fall's)
    void restageRemains();                       // B-470: every remains' stage by the clock now
    bool keptDead(uint64_t seed) const;
    void deadIn(const std::vector<DeadKept>& kept);   // B-489: the dead its world kept from the landings before (the guide's), at the landing
    void testRemainsAdd(const Remains& r) { remains.push_back(r); }
    void trunkOf(const TreeInst& T, double& r0, double& r1, double& top, double& lx, double& lz) const;   // B-468: a trunk's radii, top and lean
    double trunkRadius(const Critter& c, double y) const;                      // B-468: a climber's trunk's girth `y` over its foot
    void trunkPoint(const Critter& c, double y, double& x, double& z) const;   // and the bark on its side there
    uint64_t testKillCritter(int i);              // critter `i` caught as a hunt catches it, gone at once: its seed (0 none)
    double testClimbSide(int i, double want, double k) { return climbSide((size_t)i, want, bestiary.species[critters[i].species], k); }   // B-468
    void testLifeClock(double s) { lifeClock += s; }
    bool testShallowsSpot(int ccx, int ccz, double& x, double& z);   // a 100 m cell's place of a swimmer of the shallows (local metres)
    const ShallowsPath& testShallowsPath(int ccx, int ccz);         // B-492: and its stretch
    void climbPose(const Critter& c, AnimalPoseIn& in) const;      // B-491: a climber on its trunk's pose: its side, the girth, the axis there
    bool testClimbTree(double x, double z, const Species& sp, double k, TreeInst& out) { return climbTreeNear(x, z, sp, k, out); }   // B-491
private:
    Remains killCritter(Critter& q, bool fell = false);
    Remains keepDead(int species, double x, double z, double heading, double size, uint64_t seed, double diedAt);   // B-490: where no herd holds it
    double climbSide(size_t self, double want, const Species& sp, double k);
    bool shallowsSpot(int ccx, int ccz, uint64_t hs, double& cx, double& cz);
    const ShallowsPath& shallowsPathOf(int ccx, int ccz, uint64_t hs);
    void drawLife(Framebuffer& fb, double t);
    void updateLife(double dt, double t);
    bool treeNear(double x, double z, double radius, TreeInst& out, bool any = false);   // J-07: the nearest live tree within `radius` (`any`: dead too)
    bool climbTreeNear(double x, double z, const Species& sp, double k, TreeInst& out);   // B-491: a climber's: the nearest within 25 m it can hold
    void trunkGirth(const TreeInst& T, double& w0, double& w1, double& trunkTop) const;   // a trunk's widths and top as drawn (flora.cpp)
public:
    // J-07: the swimmers at the surface or leaping within two 100 m cells of a point at a time (drawLife's; the harness frames them): the
    // species, the spot's seed, where, the water's level, the heading (and its vector), how far up (`surf` -1..1), a leap's share (under 1)
    struct SwimmerAt {
        int species = -1; uint64_t seed = 0; double x = 0, z = 0, water = 0, heading = 0, hx = 0, hz = 1, surf = 0, leap = 9;
        int sick = 0; bool changed = false; double sizeK = 1;   // B-469: its sickness shown, born changed (a giant's or a dwarf's size)
        double fellAt = 0;   // B-490: a sprouting one's fall on the landing's clock (`dead`'s: under 0 before the landing)
    };
    // B-490: `dead`, when given, takes the sprouting ones past their fall (gone from the water)
    void swimmersNear(double px, double pz, double t, std::vector<SwimmerAt>& out, std::vector<SwimmerAt>* dead = nullptr);
    AnimalFate testSwimmerFate;   // B-490: the harness's (the game sets none): when on, every swimmer's fate, a fate no line brings there
    // B-490: a flock's members where they fly at a time (drawLife's, and the update's for a sprouting one's fall), from the flock's own draws:
    // each one's place, sickness and changed young, and what they make of its flight; `gone` a sprouting one past its fall, `lying` a crazed
    // one fallen to the ground a moment, `aloft` whether it flies (a night one over its resting flock)
    struct FlockMember { double x = 0, y = 0, z = 0, ph = 0, wf = 6; int sick = 0; bool changed = false, aloft = true, lying = false, gone = false; uint64_t seed = 0; };
    void flockMembers(const Flock& f, double t, std::vector<FlockMember>& out);
    bool testTreeNear(double x, double z, double radius, TreeInst& out, bool any = false) { return treeNear(x, z, radius, out, any); }
private:
    void lifeSaw(const std::vector<LifeDrawn>& drawn);                  // J-07: the record of what was drawn, the first sightings, the target

public:
    // ---- the buggy (buggy.cpp)
    Vec3 buggyCameraMount() const;                // the nose camera's lens in local metres
    void cameraFeed(Framebuffer& fb, double t);   // the CCTV look of the nose camera's picture (after the mush, before the RGB conversion)
    double buggyDist() const { return std::sqrt((buggy.x - player.x) * (buggy.x - player.x) + (buggy.z - player.z) * (buggy.z - player.z)); }
private:
    void updateBuggy(double dt, const Input& in, double t);
    void drawBuggy(Framebuffer& fb, double t);
    void buggyFrame(Vec3& fwdT, Vec3& sideT, Vec3& upT) const;   // the hull's axes with its pitch and roll
    void drawPuffs(Framebuffer& fb, const std::vector<Buggy::Puff>& puffs);   // the dust, snow and spray thrown up (the buggy's wheels, the drone's downwash)

public:
    // ---- the drone (drone.cpp)
    // R-403: the drone (`surface/drone.cpp`): `F` near the capsule unfolds a new one (any old one is scrapped), `E` on the ground gets in or out
    bool deployDrone();
    bool toggleDrone();
    Vec3 droneCameraMount() const;
    double droneDist() const { return std::sqrt((drone.x - player.x) * (drone.x - player.x) + (drone.z - player.z) * (drone.z - player.z)); }
private:
    void updateDrone(double dt, const Input& in, double t);
    void drawDrone(Framebuffer& fb, double t);
    void droneFrame(Vec3& fwdT, Vec3& sideT, Vec3& upT) const;

public:
    // ---- a settlement's forms gathered, built and drawn (forms_draw.cpp)
    // B-420: where a settlement's piece stands drawn near (its level 0): false where it is not drawn (its foot under water: a harbour's work
    // stands at the water's level); its foot's height (a big piece: the highest ground under its footprint) and how far its sides reach down
    // under it (a metre; a big piece's to the lowest ground under it). The drawing and the colliders both
    bool pieceFootNear(const Form& e, double ex, double ez, double& gy, double& skirt);
    // the settlements' forms (peoples/forms.h) are gathered from every cell in reach (and the harness's placed ones), each
    // building at its own level of detail by its distance (whole within 350 m, simpler to 1.3 km, coarse to 4 km, beyond that
    // its silhouette where it stands tall enough to show), then built and lit in chunks on the workers and drawn in bands
    // of rows (B-310's contract: the pixels the serial loop's)
    void gatherSettlement(const Ruin& r, const RuinCell& cell, double dist);
    void drawFormItems(Framebuffer& fb);
    static void groupPieces(RuinCell& c);   // `groups` from the pieces' building indices (B-420: and the cell's `Ruin::reach`)
    // ---- J-10: a seeded world's overgrowth on its ruins (overgrowth_ruins.cpp)
    // a cell's cover: how high it has climbed each building (from the steps since the overgrowth's edge passed it), and its pieces among each
    // building's at level 0 (membranes across the openings it climbed past, nodules on the walls under it, drapes from the tops it nearly
    // reached); what an earlier layer grew taken out first. A pure function of the cell and the layer
    void overgrowCell(RuinCell& c) const;
    // KI-477: the cover's height on one piece of building b (`x`, `z` its middle in the settlement's metres, `seed` its own; -1 none): the
    // building's climb, more on its side toward the way the cover came (its edge passed there first), less on the far side, a little more or
    // less by the piece
    static double pieceClimb(const RuinCell& c, int b, double x, double z, uint32_t seed);
    static uint64_t membraneKey(const RuinCell& c, int opening);   // KI-479: a membrane's key (`tornMembranes`)
    // a stone rotter's share of a building at local (x, z) (0..1: its line's share there and its steps since its spread came; 0 where no line
    // pits stone, or its line cannot live there: KI-480's band of warmth); each building's `stoneRot` when its cell's pieces are laid out
    double stoneRotOf(double x, double z) const;
    bool stoneRotsOf(RuinCell& c) const;   // every building's of a cell; whether one changed
    // a settlement's or a work's pieces laid out (its culture's, with each building's stone rot), its graves' stones among them
    void layRuinPieces(RuinCell& c) const;
    // KI-480: the world now handed on (`ruinsMade`); a changed one lays the cells' pieces out again where a stone rotter's share moved, and
    // grows their cover again (`regrowRuins`, the ahead worker joined)
    void setMadeNow(const MadeWorld& m) { if (m.key != ruinsMade.key) { ruinsMade = m; madeHanded = true; } }
    // KI-491: the ground takes the world now handed on where what its samples read moved (its cover, its looks, its waters, its structures:
    // `madeGroundKey`), the rings sampled again once; the animals keep the landing's herds. A step that moved only the animals' fates or the
    // ruins' steps changes nothing here
    void adoptMadeNow();
    static uint64_t madeGroundKey(const MadeWorld& m);   // the layer's key without what the ground's samples never read
    void regrowRuins();

    // ---- the harness's: vesperis_test calls these; the game never does
    const GrainTexture* testMesoFor(int material) { return mesoFor(material); }   // KI-463: a material's tile on this landing
    const GrainTexture* testTileOf(const TerrainVertex& tv) { const int f = madeCrustForm(tv); if (f >= 0 && !madeCrustBuilt[f]) buildMadeCrust(f); return tileOf(tv); }
    void buildMadeCrust(int form) { buildMesoTile(madeCrustTiles[form], MAT_CRUST, site.gen.seed, form); madeCrustBuilt[form] = true; }   // KI-493
    // KI-477 to KI-481: the ruins' faces the last frame drew wholly under an overgrowth's cover, those with windows grown shut (whole or across
    // the cover's edge), and the cover's faces and pieces lifted into its night light
    void testCoverFaces(int& covered, int& shut, int& lit) const;
    void testWaitResample() { if (resampleThread.joinable()) { resampleThread.join(); takeResample(); } }   // KI-491: the worker's rings in
    // KI-492, KI-501: the frame's faces of a ruin's cover: those wholly under it of the latest growing line and of the others, each with its veins'
    // light at night where its line glows; those across its edge, their cover in its own light
    void testCoverLight(int& whole, int& wholeVeined, int& other, int& otherVeined, int& across, int& acrossLit) const;
    int testSightMissing() const;   // R-434: the square metres within the activity vision's reach not yet read
    void testSightCounts(int& read, int& faint, int& strong) const;   // those read (within reach), faint and strong
    // A-02 (tests): place a settlement of forms with its centre at local (x, z): its layout (the buildings' footprints) and its
    // pieces at the three levels of detail; drawn at any distance (the skyline), its colliders walked into. `testClearPlaced` empties them
    void testPlaceSettlement(double x, double z, const RuinSpec& spec, const std::vector<Form> pieces[3], const std::vector<FormOpening>* openings = nullptr);
    // J-10 (tests): every kept cell's cover grown again for the layer set since (the pieces themselves stay as laid out)
    void testRegrowRuins();
    void testClearPlaced() { placedCells.clear(); supportMeshes.clear(); supports.clear(); }
    double testCanopyCoverage(double radiusM);      // share of forest-biome ground within the radius under a canopy
    bool testGotoHerd(double dist);                // stand south of herd 0, facing it
    std::string testHerdStates() const;            // "GRAZING 3 FLEEING 2 (calls 1)"
    // J-06: one species put alone where it is wanted, standing (0), aloft (1) or afloat (2) (`bestiary plates`); the world's hide banks
    void testSpecimen(int species, const Vec3& at, double heading, int how, double flap) { specimens.assign(1, Specimen{species, at, heading, how, flap}); }
    void testSpecimenAdd(int species, const Vec3& at, double heading, int how, double flap) { specimens.push_back(Specimen{species, at, heading, how, flap}); }
    void testNoSpecimens() { specimens.clear(); }
    // J-07: the one put last stepping in a gait at a phase of its cycle (`bestiary ways`' gaits)
    void testSpecimenStep(int gait, double phase, double stride, bool running) {
        if (specimens.empty()) return;
        Specimen& s = specimens.back(); s.gait = gait; s.phase = phase; s.stride = stride; s.running = running;
    }
    // J-08: the one put last as remains at a stage, sick, a changed young (its species' changed rig), at a scale of its species' size
    void testSpecimenFate(int dead, int sickness, bool changed, double scale) {
        if (specimens.empty()) return;
        Specimen& s = specimens.back(); s.dead = dead; s.sickness = sickness; s.changed = changed; s.scale = scale;
    }
    int testHideBank(int k) const { return hideBanks[k & 1]; }
    int testAheadCells() const { return aheadStat; }
    int testMadeBank(int cls) const { return cls >= 0 && cls < MC_COUNT ? madeBanks[cls] : -1; }   // J-05: a made look's bank (-1 none)
    // KI-459 to KI-475: a line's own banks (its waters', its overgrowth's), the latest line's, the hides', the dyed rain's and the fate's
    int testTintLineBank(int line) const { return line >= 0 && line < MADE_LINES ? tintLineBank[line] : -1; }
    int testGrowthLineBank(int line) const { return line >= 0 && line < MADE_LINES ? growthLineBank[line] : -1; }
    int testGrowthBank() const { return growthBank; }
    int testTintBank() const { return tintBankMade; }
    int testRainBank() const { return madeRainBank >= 0 ? madeRainBank : (madeCloudBank ? 23 : -1); }
    double findOpenRun(double& x, double& z, double& heading);   // open ground within 800 m of the explorer and its longest clear run (metres)
    // tests: after a render, per ring edge (near/lod0, lod0/lod1, lod1/lod2, lod2/lod3), the largest gap in metres
    // between the finer ring's edge vertices as drawn and the coarser ring's surface there (`seam`; the rings meet when
    // it is 0) and the gap the unmorphed vertices would have had (`raw`: what popped before). -1 where the edge is not drawn
    void testRingSeams(double seam[4], double raw[4]);
    void testRings(int r[5]) const { for (int k = 0; k < 4; k++) r[k] = ringR[k]; r[4] = ringN; }   // tests: the cells drawn per ring, lod0..lod3, then the near ring
};
