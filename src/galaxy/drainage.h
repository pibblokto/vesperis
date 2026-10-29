// The drainage (O6-03): rivers running downhill to a lake or the sea, valleys joining into a tree, lakes at the fill levels of real
// depressions. A precomputed layer of the planet function, a one-degree tile at a time on a lattice of about 400 m (`latticeN0` cells a
// degree), from the planet function sampled at that scale without the drainage: a priority flood (every depression filled to its spill), D8
// flow and area accumulation, over the tile's core and a margin of 0.3 degrees; two tiles' estimates of a cell are blended by its distance
// to their edges (`drainageAt`). Tiles are cached (an LRU of a few dozen), made by the first sample that needs one or ahead by a landing
// (`drainagePrefetch`). A channel is a Catmull-Rom curve through jittered nodes, meandering on flat reaches, its water a metre under its
// nodes' filled ground (never rising along the flow); a filled depression is a lake whose shore is its level's contour.
#pragma once
#include "planetmap.h"

// what the drainage says about a point (metres; the levels absolute, like heights)
struct DrainInfo {
    bool near = false;         // a channel lies within the neighbourhood (whatever its size)
    bool any = false;          // and it counts at this detail (`pres` > 0)
    double dist = 1e9;         // metres to the channel that shapes this point (the one whose floodplain reaches it most, else the nearest)
    double halfWidth = 0;      // the channel's half-width at this detail (never under 0.45 x the detail)
    double depth = 0;          // the channel's depth under its banks
    double level = -1e9;       // the water surface here: the reaches' levels blended by their nearness (continuous across a meander's neck)
    double pres = 0;           // 0..1 how much of a channel it is: small streams fade at coarse detail and in dry country
    double acc = 0;            // km^2 drained by that channel here
    double slope = 0;          // the channel's grade along the flow (rise over run)
    double valley = 0;         // 0..1 how far the ground here melts toward `valleyLevel` + 1: the V of the valley round the channel
    double valleyLevel = -1e9; // the level of the channel whose valley this is
    double lake = 0;           // 0..1 a filled depression here (the mask of the flood's lake cells)
    double lakeLevel = -1e9;   // its surface
    double wetness = 0;        // 0..1 how near a valley floor (the moisture of the floor, the greener bottomland)
};
bool typeHasDrainage(int type);            // felisian, desert, thin-atmosphere, hydrocarbon and acidic worlds
void setDrainageEnabled(bool on);          // the harness: whole-planet statistics sample without it (a tile per sample would take hours)
bool drainageEnabled();
// the drainage at a point, at this sampling scale (none above 2048 m: the world map has no rivers)
DrainInfo drainageAt(const BodyGen& g, const Vec3& unit, double detailM, double moist = 0.5);   // moist 0..1: a dry country's streams need a bigger catchment
// compute the tiles within radiusM of a point now (async: on a thread of their own; a sample that needs one first waits for it)
void drainagePrefetch(const BodyGen& g, const Vec3& unit, double radiusM, bool async);
bool drainageReady(const BodyGen& g, const Vec3& unit, double radiusM);   // every tile within the radius is in the cache
int drainageTilesHeld();
int drainageTilesBuilt();                 // tests: the tiles computed since the start (B-422: none for the places a moving cursor passes)
// the flood's own cell under a point: true when it is a kept lake, with its level (the own tile alone, no neighbourhood: cheap)
bool drainageLakeCell(const BodyGen& g, const Vec3& unit, double& level, bool buildTiles, bool& known);   // known: false when the tile is not built and buildTiles is false
// a scope in which this thread's samples read the planet function without the drainage (the landmark search's coarse
// scan; the landing map's site probe while the site's tiles are not built yet: a probe must not build them)
struct DrainageOff { explicit DrainageOff(bool off = true); ~DrainageOff(); bool off; };
int drainageLatticeN0(double radiusKm);    // cells per degree of the lattice of a world of that radius

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do
// v after `steps` of `v = v - 0.005f`, bit for bit what that loop computes, a binade at a time (the routing's descent sinking a
// pit of two cells; the unit checks it against the loop)
float drainageSink(float v, long steps);
// tests: the flood's own numbers over a window of the lattice round a point: channel cells, lake cells, the largest
// accumulation (km^2), the share of channel cells whose downstream neighbour lies lower or level
struct DrainStats { int cells = 0, channels = 0, lakes = 0, sea = 0; double maxAcc = 0; int monotone = 0, tested = 0; double msBuild = 0; int lakesKept = 0, lakesBreached = 0; };
DrainStats drainageStats(const BodyGen& g, const Vec3& unit, int halfCells);
