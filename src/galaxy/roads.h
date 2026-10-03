#pragma once
// C-09 (2026-10-03): roads that still go somewhere. "A dead civilisation's roads survive as faint lines in the terrain
// (compacted ground, a different material), readable from the landing zoom. Following one on foot or by buggy leads between
// the ruins of a world: a network to trace rather than points to find" (`docs/ideas/IDEAS-ruins-shards-signals.md`,
// section 4). The network is a function of the settlements (C-01's ruin grid, `galaxy/ruins.*`) and the terrain, read from
// the galaxy level like the settlements themselves, so the surface view, the landing zoom and the harness see one network:
// the sited settlements within two cells of each other are joined where no third settlement lies closer to both than they
// are to each other (the relative neighbourhood graph: every settlement reaches its nearest neighbour, a town gathers
// several roads, the graph is planar and the same from either end), and each road is a walk from one settlement's edge to
// the other's in fifty-metre steps that takes the gentlest of five headings (the grade and the turn weighed against the way
// to the goal) and never steps into the sea, a lake or a dead desert's old sea, which stood when the roads were used. The
// culture's roads are paved (the rock family's material) or beaten (the ground's own, bare and darker), broader between
// towns, and worn by the age: a share of the length is gone in gaps along the way (`roadLeft`), so a road of two centuries
// is a line and one of twenty millennia a trace. The surface draws the road into its terrain vertices (`SurfaceSite::
// ensureRoads`, `sampleAt`), the landing zoom draws the network's lines (`Game::buildLandingZoom`), the harness prints it
// (`vesperis_test roads`).
#include "ruins.h"
#include <unordered_map>

constexpr int ROAD_REACH = 2;              // cells: a settlement is joined to the sited settlements within two cells of its own (about five kilometres)
constexpr double ROAD_STEP_M = 50;         // the walk's step
constexpr double ROAD_MAX_M = 6500;        // no road longer than this as the crow flies
constexpr int ROAD_WALK_MAX = 320;         // steps before a walk gives up (sixteen kilometres of wandering)
constexpr double ROAD_WAVER_M = 9;         // the waver across the way over a plain (metres, over a few hundred along)

struct RoadNode {           // a sited settlement, without its layout
    int gLat = 0, gLon = 0; // the ruin grid's cell (gLon wrapped)
    double lat = 0, lon = 0, size = 12, heading = 0;
    int sclass = 0, plan = 0;
    uint64_t seed = 0;
    int people = 0;         // C-13: the settlement's people (`RuinSpec::people`)
    Vec3 unit;              // body-frame unit vector of the centre
};
struct Road {
    uint64_t id = 0;        // of the two cells, the same from either end
    RoadNode a, b;          // a's cell sorts before b's
    int people = 0;         // C-13: whose build it is (`roadPeopleOf`): the width, the paving and the wear are that people's culture's
    std::vector<Vec3> pts;  // the centre line, unit vectors from a's edge to b's edge, about ROAD_STEP_M apart
    double lengthM = 0, straightM = 0;   // along the way, and between the edges as the crow flies
    double maxGrade = 0;    // the steepest step (rise over run)
    double halfWidth = 3;   // metres: the culture's, broader between towns
    int bends = 0;          // steps that were not the straight one
};

struct RoadPair { RoadNode a, b; uint64_t id = 0; };   // two joined settlements, a's cell before b's, not walked yet

// the sited settlement of a cell, if any (no layout: the class, the size, the heading and the plan a road needs, and
// C-01's site test without the drainage)
bool roadNodeOfCell(const BodyGen& g, int gLat, int gLon, RoadNode& out);
uint64_t roadCellKey(const BodyGen& g, int gLat, int gLon);   // the cell's key, the longitude wrapped
// whether two nodes are joined: within ROAD_REACH cells and ROAD_MAX_M, and no third of `others` closer to both
bool roadJoined(const BodyGen& g, const RoadNode& a, const RoadNode& b, const std::vector<RoadNode>& others);
// the way between two joined nodes: the walk (false when every way stepped into water); `c` the culture (the width)
bool roadWay(const BodyGen& g, const Culture& c, const RoadNode& a, const RoadNode& b, Road& out);
// C-13: whose build a road is: the two ends' people, or, between two peoples' settlements, the bigger settlement's (the first
// people's when they are of a class); the culture handed to `roadWay` should be that people's
int roadPeopleOf(const RoadNode& a, const RoadNode& b);
// the settlements of the cells looked at so far (the view's growing network, the zoom, the harness): each cell's node is
// found once; `pairsOf` lists the joined pairs with an end in a cell (each pair is judged by the union of the two ends'
// blocks, so it is the same pair from either end and carries one id)
struct RoadNodeCache {
    const BodyGen* g = nullptr;
    std::unordered_map<uint64_t, std::pair<bool, RoadNode>> cells;
    void reset(const BodyGen* gg) { g = gg; cells.clear(); }
    bool node(int gLat, int gLon, RoadNode& out);
    void block(int gLat, int gLon, std::vector<RoadNode>& out);   // the nodes within ROAD_REACH cells, the cell's own included
    void joinedTo(const RoadNode& a, std::vector<RoadNode>& out);
    void pairsOf(int gLat, int gLon, std::vector<RoadPair>& out);
};
// the roads of one settlement's cell, walked: every road with an end there (none when no settlement is sited there)
void roadsOfCell(const BodyGen& g, int gLat, int gLon, std::vector<Road>& out);
// the roads with a point within `radiusM` of a point, each once, sorted by id; `parallel` walks them on the worker pool
void roadsNear(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<Road>& out, bool parallel = false);
// the metres between two points of a world
double roadDistanceM(const BodyGen& g, const Vec3& a, const Vec3& b);
// how much of a road is left `alongM` metres from its start: 1 whole .. 0 gone (the wear's gaps: a noise along the way
// under the culture's `roadWear`, the gaps 40-120 m long with soft ends)
double roadLeft(uint64_t id, double alongM, double wear);
// the road's id from its two cells (either order)
uint64_t roadIdOf(const BodyGen& g, int gLatA, int gLonA, int gLatB, int gLonB);
