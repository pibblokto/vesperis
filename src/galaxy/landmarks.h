// O6-06: the sights of a world, found in the terrain itself, never dropped on flat noise. Every cell of a grid of about 14 km is
// asked for its best candidate of each kind: the highest point standing over the cell (PEAK), the biggest crater of a crater
// field (CRATER), the rim of a canyon (CANYON RIM), a mesa's cap (MESA), a geyser basin's vents (GEYSER FIELD), a crystal field
// on a quartz or carbon world (CRYSTAL FIELD), a kept lake of the drainage (LAKE), the biggest ruin (RUINS). A sight has a kind,
// a place, a size and a hash id of its cell that is the same wherever it is asked for. R-408 (2026-10-07): the sights are the
// surface scanner's echoes, each kind read on its own (`sightsOfCell`, `sightsNear`) with every ruin of the 2 km grid listed
// apart (`ruinsNear`); they are not markers: the maps show the explorer's own marks (game/guide.h's `SurfaceMark`).
// O6-06's one landmark a cell (its order of kinds) is kept for the names given to them before (`legacyLandmarkOf`)
#pragma once
#include "planetmap.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>

enum LandmarkKind { LM_PEAK = 0, LM_MESA, LM_CANYON, LM_CRATER, LM_GEYSERS, LM_CRYSTALS, LM_LAKE, LM_RUIN, LM_WORK,   // LM_WORK a people's work (peoples/works.h)
                    // the new landforms' sights, one each (`landformSight`): a great shield's caldera, a tessera's crest, a labyrinth's floor, a
                    // cluster of hollows, a chain's crater, a field of penitentes, a colonnade, a mud volcano, an old river's ridge, a pingo, a maar
                    LM_CALDERA, LM_TESSERA, LM_LABYRINTH, LM_HOLLOWS, LM_CHAIN, LM_PENITENTES, LM_COLUMNS, LM_MUD, LM_RIDGE, LM_PINGO, LM_MAAR,
                    LM_METEORITES,   // a strewn field of iron meteorites (TR_METEORITES)
                    LM_COUNT };
// Q-01: the sight a landform trait gives (LM_COUNT none); Q-02: a strewn field's
int landformSight(int trait);
extern const char* const LANDMARK_KIND_NAMES[LM_COUNT];   // "PEAK", "MESA", ...
extern const char* const LANDMARK_KIND_SYMBOLS[LM_COUNT]; // one glyph for the maps

struct Landmark {
    int kind = LM_PEAK;
    Vec3 unit;              // body-frame unit vector of its centre (the summit, the crater's centre, the rim's point...)
    double radiusM = 500;   // its size: the summit's massif, the crater's rim, the field's extent, the settlement's radius
    double heightM = 0;     // the ground at the centre (the peak's height; a crater's floor)
    double prominenceM = 0; // a peak's rise over the cell, a mesa's cap over the plain, a crater's depth, a ruin's span
    uint64_t id = 0;        // the hash of its cell and body (a ruin of `ruinsNear`: its 2 km cell's seed): stable
    int sub = 0;            // LM_RUIN: its `SettlementClass` (0 a hamlet, 1 a village, 2 a town, 3 a monument, 4 a city, 5 a capital; B-417: no monolith);
                            // LM_WORK: its `WorkKind`
};

// the sights of one cell, each kind on its own; `hiH` and `mean` are the cell's scan (its highest sample, its mean height); `lake0` is
// O6-06's lake (the centroid of every lake cell of the drainage, a playa's too), kept for the legacy pick: the scanner's lake is water
struct CellSights { bool has[LM_COUNT] = {}; Landmark lm[LM_COUNT]; double hiH = 0, mean = 0; bool hasLake0 = false; Landmark lake0; };
// a cell's sights (cached; deterministic): false when the world has none or the cell lies beyond 80 degrees. `buildTiles` false:
// the lakes are read from the drainage tiles already built (a verdict without them is not kept)
bool sightsOfCell(const BodyGen& g, int ci, int cj, CellSights& out, bool buildTiles = true);
// O6-06's one landmark of the cell: the first kind of its order (crater, canyon rim, mesa, geysers, crystals, a lake unless a peak
// stands 200 m over the cell, ruins unless one stands 150 m, the peak), for the names older guides gave them
bool legacyLandmarkOf(const CellSights& s, Landmark& out);
// R-408: the cells whose ids' low 32 bits are among `ids` (the old `<body>/L<id>` keys), one pass over the world's grid
void cellsOfLegacyIds(const BodyGen& g, const std::set<uint32_t>& ids, std::map<uint32_t, std::pair<int, int>>& out);
// the sights of the kinds in `kindMask` (bits of LandmarkKind) of the cells within `radiusM` of a point, each within the radius
void sightsNear(const BodyGen& g, const Vec3& unit, double radiusM, unsigned kindMask, std::vector<Landmark>& out, bool buildTiles = true);
// every settlement of the 2 km grid within `radiusM` (as the surface view keeps them); T-01: and
// the world plan's places heard by their size from farther (a capital from 400 km, a city from 200: peoples/plan.h)
void ruinsNear(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<Landmark>& out);
// T-03: the works the scanner hears within `radiusM` (peoples/works.h; a line's lesser stretches and a pilgrims' way are not heard): built here
// if the world's works are not yet (the scanner's thread may wait); B-418: the land's works with them
void worksHeardNear(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<Landmark>& out);
// the kinds a world can hold (bits of LandmarkKind): the scanner's modes there
unsigned sightKindsOf(const BodyGen& g);
int landmarkCellsSearched();                // tests: how many cells were searched (not served from the cache) so far
double landmarkScanMs(int part);            // tests: time spent so far in the cells' scan (0), the lake pass (1), the rest (2)

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do

// the cell of a point, the grid's cell size in degrees of latitude
void landmarkCellOf(const BodyGen& g, const Vec3& unit, int& ci, int& cj);
double landmarkCellDeg(const BodyGen& g);
