// O6-06: landmarks, the sights of a world. Every cell of a grid of about 14 km holds at most one landmark, placed by the
// terrain itself and never dropped on flat noise: the highest point of a mountain cell (PEAK), the biggest crater of a
// crater field (CRATER), the rim of a canyon (CANYON RIM), a mesa's cap (MESA), a geyser basin's vents (GEYSER FIELD), a
// crystal field on a quartz or carbon world (CRYSTAL FIELD). A landmark has a kind, a place, a size, a name generated
// from the world's name and its id, and a hash id that is the same wherever it is asked for, so the maps, the
// rangefinder, the log and the guide all speak of the same thing. `landmarksNear` searches the cells round a point and
// returns what it finds (a cell's search samples the planet function at 500 m and refines: about a millisecond a cell).
#pragma once
#include "planetmap.h"
#include <string>
#include <vector>

enum LandmarkKind { LM_PEAK = 0, LM_MESA, LM_CANYON, LM_CRATER, LM_GEYSERS, LM_CRYSTALS, LM_LAKE, LM_RUIN, LM_COUNT };
extern const char* LANDMARK_KIND_NAMES[LM_COUNT];   // "PEAK", "MESA", ...
extern const char* LANDMARK_KIND_SYMBOLS[LM_COUNT]; // one glyph for the maps

struct Landmark {
    int kind = LM_PEAK;
    Vec3 unit;              // body-frame unit vector of its centre (the summit, the crater's centre, the rim's point...)
    double radiusM = 500;   // its size: the summit's massif, the crater's rim, the field's extent
    double heightM = 0;     // the ground at the centre (the peak's height; a crater's floor)
    double prominenceM = 0; // a peak's rise over the cell, a mesa's cap over the plain, a crater's depth
    uint64_t id = 0;        // hash of the cell and the body: stable
    int sub = 0;            // C-01: LM_RUIN: 0 a monolith of the old ones, 1 a hamlet, 2 a village, 3 a town (`SettlementClass` + 1)
    std::string name;       // generated: "MOUNT KEIRA", "CRATER ANSHE", ...
};

// the landmarks of the grid cells within radiusM of a point (the cells' own places may lie a little beyond); deterministic
void landmarksNear(const BodyGen& g, const std::string& bodyName, const Vec3& unit, double radiusM, std::vector<Landmark>& out, bool buildTiles = true);   // buildTiles false: the lakes are read from the drainage tiles already built (the landing map)
// the landmark of one grid cell, if any (false when the cell holds none); the cell of a point via `landmarkCellOf`
void landmarkCellOf(const BodyGen& g, const Vec3& unit, int& ci, int& cj);
bool landmarkOfCell(const BodyGen& g, const std::string& bodyName, int ci, int cj, Landmark& out, bool buildTiles = true);
double landmarkCellDeg(const BodyGen& g);   // the grid's cell size in degrees of latitude
int landmarkCellsSearched();                // tests: how many cells were searched (not served from the cache) so far
double landmarkScanMs(int part);            // tests: time spent so far in the cells' scan (0), the lake pass (1), the rest (2)
