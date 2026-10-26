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
    struct RuinCell { bool has = false; bool placed = false; Ruin r; RuinSpec spec; std::vector<Form> elems[3]; std::vector<int> groups[3]; std::vector<ShardSite> shards; std::vector<Grave> graves; mutable std::vector<FootCache> foot[3]; mutable uint32_t drawn = 0; mutable bool marked = false;
                      std::vector<Form> recordForms; std::vector<int> recordFirst;
                      std::vector<InstrumentSite> instruments; std::vector<int> instrumentIds; std::vector<Form> instrumentForms; std::vector<int> instrumentFirst;
                      mutable bool watered = false; };   // its records laid again with the water as drawn (`waterRecords`)   // C-11: the instruments beside its music records (each site's id, -1 none; its objects as the records')   // T-04: each record's object (`recordForms`, peoples/records.h): site k's from recordFirst[k] to recordFirst[k + 1]   // T-02: `marked` its ground in the site's marks   // T-01: `drawn` the frame that gathered it   // C-03: the settlement's shards; C-12: its graves (their stones among the pieces of lod 0)
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
    struct NearGrave { int k = -1; uint64_t id = 0; int sclass = 0; std::string line, hud; double x = 0, z = 0, dist = 0; };   // `line` "Thaelu, years 140 to 212 of Kethra" (the log), `hud` "Thaelu, 140-212"
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
    double markCheckX = 1e18, markCheckZ = 1e18;
    int lastTreesDrawn = 0, lastFloraPoints = 0, lastFloraPolys = 0, lastNearTrees = 0, lastMidTrees = 0, lastFarTrees = 0, lastTreesHidden = 0;
    int lastTreesVisited = 0, lastTreesListed = 0;   // bench: trees listed this frame, and visits summed over the band threads (the duplication of the per-tree work)
    double lastFloraMs[2] = {0, 0};   // the trees, the cells (logs, undergrowth, meadows, shores, deserts) (bench; the worst band's)
    double lastRenderMs[6] = {0, 0, 0, 0, 0, 0};   // bench: palette and sky; the vertex warm pass; the terrain (the bin, the reflections' gather, the bands: floor, rings, reflections; F-05); footprints to objects; flora; the rest
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
    int lastTerrainCells = 0, lastTerrainAllRows = 0, lastStaleCorners = 0;   // bench, tests: the cells binned, those every band draws (a corner behind the near plane, foam), the corners the bin found unwarmed (0)
    double lastTerrainBinMs = 0;
    double lastRoadsMs = 0; int roadsBuilds = 0;    // the site's roads grown as the explorer goes (`ensureRoads` on the main thread): the last one's ms, how many
    double lastFloraListMs = 0;                     // the flora's cells and trees listed this frame (on the main thread, before the bands; a settlement's buildings tested against every tree)
    double highlightUntil = -1;                    // X highlights creatures
    // creatures: the world's bestiary, the site's herds (planLife is the deterministic spawn, shared with the survey).
    // a herd is either one of the landing's (planLife: within 180 m of the capsule, `origin`) or the herd of a
    // habitat cell (512 m squares of the body's latitude and longitude, hashed: a chance by biome, a species, a size),
    // spawned when the explorer comes within 900 m of the cell's centre and forgotten beyond 1300 m, so there are animals
    // wherever the biome has them, not only round the capsule (the landing spot)
    struct Herd { double cx = 0, cz = 0; int gLat = 0, gLon = 0; bool origin = false; };
    std::vector<Herd> herds;
    Bestiary bestiary;
    double habitatT = -1e9;
    std::set<int> seenSpecies;                     // species sighted within 60 m this landing
    std::vector<std::string> sightings;            // "name|plan|distance" entries for the guide (consumed by the game)
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
    GrainTexture leafTile, leafEdge, barkTile;   // the vegetation's tiles (textures.h)
    TreeLook looks[10];                          // per silhouette on this planet
    std::vector<Vec3> dirLUT;
    std::vector<float> cloudGrid;
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
    double nearBlend = 0;              // 1 while the near ring is drawn (B-313: at every speed: faded out above 12 m/s, the 4 m relief rising as the buggy stops would read as the hills changing shape), 0 when it is not (no ring, or the capsule high up)
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
    // M10-05 material meso tiles, built lazily per landing
    GrainTexture mesoTiles[MAT_COUNT];
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
    struct VtxCache { int stamp = -1; RVert v; uint8_t bank = 0; bool front = false; float sy = 0; float wy = -1e9f; float hy = 0; float sh = 1e9f; RVert wv; };   // hy = the height as drawn (morphed), sh = the side of the water's edge as drawn (`vertexShore`, morphed), for the shore cut and the water's depth; F-04: wv = the water sheet's corner here when the vertex has its own level (`waterCorner`)
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
    struct FormItem { const Form* f; Vec3 foot; double skirt, d, fog; int lod, people; };
    struct FormPolyOut { int first, n, rp, y0, y1; };
    struct FormLineOut { RVert a, b; int bank, thick, y0, y1; };
    struct FormChunk { std::vector<RVert> v; std::vector<FormPolyOut> polys; std::vector<FormLineOut> lines; int silhouettes = 0; };
    std::vector<FormItem> formItems;
    std::vector<FormChunk> formChunks;
    std::vector<RasterParams> formRps;
    FacePattern formPatterns[2][4];
    double waterWdx = 0, waterWdz = 0, waterSunUp = 0;   // the frame's wind direction (sin, cos) and the sun's rise, for waterCorner

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
    // ---- the terrain rings: the material tiles, the vertex, the warm, the bin and the draw, the reflections, the floor, the cached ground (surface_terrain.cpp)
    // O1 (B-302): the rangefinder: march the camera's line of sight over the terrain caches; returns the distance to the
    // ground (or the water) it hits within maxDist, -1 for the sky. The last frame's answer is kept for the HUD and for M
    double rangeToGround(double maxDist, double& hitX, double& hitZ, bool& water);
    double drawRadiusM() const { return ringR[3] ? ringR[3] * 2048.0 : (ringR[2] ? ringR[2] * 512.0 : ringR[1] * 64.0); }   // how far the terrain is drawn
private:
    const GrainTexture* mesoFor(int material);
    void buildMesoTiles(uint32_t want);   // the tiles of a set of materials not made yet, a material to a worker
    double castShadowLit(TerrainCache& cache, int cx, int cz, double h, const Vec3& sd);
    void gatherReflections();
    void drawReflectionBand(Framebuffer& fb, int bandY0, int bandY1) const;
    double coarseShoreAt(TerrainCache& c, double x, double z);   // the coarser ring's shore value at (x, z), bilinear; 1e9 where a corner has none   // B-310: front/sy = the projected row, for the band cull; B-320: wy = the water level as drawn (morphed with the ground), -1e9 none
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
    RVert waterCorner(double x, double z, double wl, double hy) const;
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
    static double auroraOvalLat(const Body& b) { return 59.0 + 8.0 * magneticField(b) + 6.0 * (unitFromHash(hashCombine(b.seed, 0xA17)) - 0.5); }   // the auroral oval's latitude on a world: nearer the pole on a strong field
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
    double canopyDensityAt(double x, double z, double veg) const;
    double seasonPhaseLocal() const;               // -1 deep winter .. +1 high summer for this site's hemisphere (0.5 without tilt)
private:
    void buildLooks();
    void drawFlora(Framebuffer& fb, double t);     // trees, undergrowth, meadows, reeds, lily pads, cacti, lichen, fireflies (B-310: in parallel bands)
    void drawFloraBand(Framebuffer& fb, double t, int bandY0, int bandY1, FloraStats& st);

public:
    // ---- the creatures: the herds and the flocks, their habitat cells, their update and draw (creatures.cpp)
    static void planLife(const SurfaceSite& s, const Bestiary& B, uint64_t seed, std::vector<Critter>& critters, std::vector<Flock>& flocks, std::vector<Herd>& herds);
    void spawnLife(double lat, double lon);
    static void lifeCellAt(const SurfaceSite& s, double x, double z, int& gLat, int& gLon);
    // the herd a habitat cell holds, if any: its centre (local metres), species and size; deterministic
    static bool planCellHerd(const SurfaceSite& s, const Bestiary& B, int gLat, int gLon, Herd& h, int& species, int& n);
    void updateHabitat();
private:
    void drawLife(Framebuffer& fb, double t);
    void updateLife(double dt, double t);

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

    // ---- the harness's: vesperis_test calls these; the game never does
    // A-02 (tests): place a settlement of forms with its centre at local (x, z): its layout (the buildings' footprints) and its
    // pieces at the three levels of detail; drawn at any distance (the skyline), its colliders walked into. `testClearPlaced` empties them
    void testPlaceSettlement(double x, double z, const RuinSpec& spec, const std::vector<Form> pieces[3]);
    void testClearPlaced() { placedCells.clear(); supportMeshes.clear(); supports.clear(); }
    double testCanopyCoverage(double radiusM);      // share of forest-biome ground within the radius under a canopy
    bool testGotoHerd(double dist);                // stand south of herd 0, facing it
    std::string testHerdStates() const;            // "GRAZING 3 FLEEING 2 (calls 1)"
    int testAheadCells() const { return aheadStat; }
    double findOpenRun(double& x, double& z, double& heading);   // open ground within 800 m of the explorer and its longest clear run (metres)
    // tests: after a render, per ring edge (near/lod0, lod0/lod1, lod1/lod2, lod2/lod3), the largest gap in metres
    // between the finer ring's edge vertices as drawn and the coarser ring's surface there (`seam`; the rings meet when
    // it is 0) and the gap the unmorphed vertices would have had (`raw`: what popped before). -1 where the edge is not drawn
    void testRingSeams(double seam[4], double raw[4]);
    void testRings(int r[5]) const { for (int k = 0; k < 4; k++) r[k] = ringR[k]; r[4] = ringN; }   // tests: the cells drawn per ring, lod0..lod3, then the near ring
};
