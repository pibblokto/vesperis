// T-01 (2026-10-09): a people's map of its world (`docs/archive/ideas/IDEAS-civilisation-expansion.md`, section 5). Until T-01 every
// 2 km cell of a civilised world was hashed on its own (C-01: a third of them held a settlement, the whole world over). The plan
// reads the world once, coarsely (`PlanGrid`: the planet function on a grid of half a degree, about 165,000 cells: the land of the
// people's time, the height, the roughness, the climate, the coast's distance, the rivers of a flow over the grid, the fertility,
// the coal and the ore), scores where each age would settle (water and good land always; hills for defence in the medieval age;
// coal, ore and rivers in the industrial; the coasts in the modern; anything the futuristic and far-future peoples chose; the
// genome's relation to the land on top: a people of ridges, valleys, cliffs, stilts...), grows the people's territory from its
// capital over the land it could reach until it holds the share its profile says (`PeopleExtent::held`), and lays down its places
// by rank: the capital, the cities and the towns (a prehistoric people's gathering place and the seats of its bands, a futuristic
// people's few great places, the far future's one to three works), spaced as the age travelled, each with its rank in the founding
// (for the history, H-02), its population at the height and the core the ground draws (`radiusM`, `buildings`). The countryside
// (hamlets, villages, the market towns) stays the 2 km cells' own, by a density the plan gives each cell (`planCountryside`: the
// territory, its land, the nearness of its towns), so no settlement stands outside the land a people held. The networks join the
// places as the age travelled (paths, roads, rail and canals, highways, maglev; a sea crossing where the way was over water): the
// relative neighbourhood graph of the places and its minimum spanning tree, the trunk between the capital and the cities; a way's
// waypoints (`planEdgeWay`) settle on dry, gentle ground every few kilometres and the ground walks the legs between them as C-09
// walked its roads. The cells read the plan (`ruinOfCell`), the view draws a city whose core spans cells whole, the maps and the
// telescope draw the cores and the networks where the ground has them. Built once a world (`worldPlanOf`, cached), on a thread of
// its own at the approach (`worldPlanPrefetch`); deterministic (the grid's samples are independent, the rest serial).
#pragma once
#include "ruins.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cstdint>

// the coarse reading: equal-area rows of cells (a row of half a degree, its cells as wide as it is tall)
struct PlanGrid {
    int rows = 0;
    double dLat = 0, R = 0;                // the row's height (radians), the world's radius (km)
    std::vector<int> rowStart, rowCols;    // the first cell and the count of each row
    int cells = 0;
    std::vector<float> height;             // metres
    std::vector<uint8_t> land;             // dry land in the people's time (no sea, lake or old sea)
    std::vector<uint8_t> biome, material;
    std::vector<float> relief;             // the planet function's mountain-ness, 0..1
    std::vector<float> veg, tempC, fert;   // the vegetation, the mean temperature, the fertility (0..1)
    std::vector<float> coastKm;            // to the nearest water cell (0 in the water)
    std::vector<float> river;              // the rain gathered from upstream, in cells' worth (a big river over 40)
    std::vector<int> down;                 // where the cell drains (-1 the sea or a sink)
    std::vector<float> res;                // coal and ore (0..1)
    std::vector<uint8_t> side;             // whose side of C-13's divide (0, 1)
    double landKm2 = 0;                    // the land's area
    int rowOf(double lat) const;
    int cellOf(double lat, double lon) const;
    int cellOf(const Vec3& unit) const;
    void rowCol(int cell, int& i, int& j) const;
    void latLonOf(int cell, double& lat, double& lon) const;
    Vec3 unitOf(int cell) const;
    double areaKm2(int cell) const;
    int neighbours(int cell, int out[8]) const;   // the row's two, the rows above and below at the same longitude and their sides
    double sizeKm() const { return dLat * R; }
};
PlanGrid readPlanGrid(const BodyGen& g, bool parallel = false);

// the networks' kinds by the age: a prehistoric people's paths, roads, an industrial people's rail and canals, a modern people's
// highways, a futuristic people's maglev on its pylons; a crossing where the way was over water (not drawn)
enum NetKind { NK_PATH = 0, NK_ROAD, NK_RAIL, NK_CANAL, NK_HIGHWAY, NK_MAGLEV, NK_SEA, NK_COUNT };
extern const char* const NET_KIND_NAMES[NK_COUNT];   // "path", "road", "rail", "canal", "highway", "maglev", "sea crossing"
extern const char* const NET_KIND_HUD[NK_COUNT];     // the HUD's: "PATH", "ROAD", "RAILWAY", "CANAL", "HIGHWAY", "MAGLEV", "CROSSING"
extern const uint32_t NET_KIND_COLOURS[NK_COUNT];    // the maps' (0xAABBGGRR): a path brown, a road pale, rail red, a canal blue, a highway orange, maglev magenta, a crossing grey-blue

struct PlanPlace {
    int rank = 0;               // its index in the people's list: the order of the founding (0 the capital)
    int sclass = SC_TOWN;       // SC_CAPITAL, SC_CITY, SC_TOWN; SC_VILLAGE a prehistoric band's seat; SC_MONUMENT a far-future work
    int plan = -1;              // the settlement's plan where the plan decides it (SP_GREAT, SP_LONE), else -1 (the culture's)
    double lat = 0, lon = 0;
    Vec3 unit;
    double radiusM = 100;       // the core the ground draws (the settlement's radius)
    int buildings = 0;          // how many the core holds (the capital's and the cities': the layout's count; 0 the class's own draw)
    double population = 0;     // at the people's height
    double score = 0;           // the ground's for the age (0..1)
    double remote = 0;          // how far into the territory from the capital (0 .. 1 the frontier)
    int gLat = 0, gLon = 0;     // the 2 km cell it stands in (the longitude wrapped): the cell's settlement is this place
    uint64_t seed = 0;          // that cell's (C-01's hash): the layout's stream
    int people = 0;
    int cell = 0;               // the grid's cell
    bool coast = false, river = false;   // on a coast, on a river (the coarse reading's)
};
struct PlanEdge {
    int a = 0, b = 0;           // the places' ranks
    int kind = NK_ROAD;
    bool trunk = false;         // between the capital and the cities (the spanning tree of the great places)
    double lengthKm = 0;        // as the crow flies
    uint64_t id = 0;
};
struct PeoplePlan {
    int people = 0, age = 0;
    double areaKm2 = 0, heldShare = 0;    // the territory's land and its share of the world's
    double buildMs = 0;
    std::vector<PlanPlace> places;        // by rank
    std::vector<PlanEdge> edges;
    std::vector<uint8_t> territory;       // per grid cell: 1 the people's
    std::vector<float> remote;            // per grid cell: the cost of the way from the capital, 0..1 (-1 outside)
    std::vector<float> score;             // per grid cell: the ground's score for the age
    int count(int sclass) const;
};
struct WorldPlan {
    uint64_t seed = 0;
    int peoples = 0;
    double buildMs = 0, gridMs = 0;
    PlanGrid grid;
    PeoplePlan plans[2];
    std::vector<uint8_t> countryside;     // per grid cell: the countryside's density (0 .. 255) for its side's people
    // the places by their 2 km cell, and per grid cell the places and the networks' edges in reach of it (people << 24 | index)
    std::unordered_map<uint64_t, uint32_t> placeAt;
    std::vector<std::vector<uint32_t>> placesIn, edgesIn;
    bool works = false;                   // a people's ways need works in the cells they cross (a futuristic people's maglev on its pylons)
    // `planEdgeWay`'s waypoints, made when first asked
    mutable std::mutex wayMutex;
    mutable std::unordered_map<uint64_t, std::vector<Vec3>> ways;
};

// the world's plan: cached by the world's seed (the last four); built here unless a prefetch is on its way (then waited for)
std::shared_ptr<const WorldPlan> worldPlanOf(const BodyGen& g);
std::shared_ptr<const WorldPlan> worldPlanIfReady(const BodyGen& g);   // null while it is being built or was never asked for
void worldPlanPrefetch(const BodyGen& g);                               // the approach: built on a thread of its own
bool worldPlanBuilt(uint64_t seed);                                     // tests: in the cache and ready
void worldPlanThreadsJoin();   // C-18: the prefetches' threads waited for (at exit, before the caches they write are destroyed)
void worldPlanClearCache();                                             // tests: the next ask builds again
// the ground's score for an age on each grid cell of a people's side (0 .. 1, the side's best 1; `PeoplePlan::score` for its own age):
// H-02 reads an older age's to say which of the places were old (`only`: the cells to score, the best of them 1; the rest 0)
void planScores(const BodyGen& g, const PlanGrid& grid, int people, int age, std::vector<float>& out, const std::vector<uint8_t>* only = nullptr);

// the cells' reading: the place whose core stands in a 2 km cell; the countryside's density at a point (0 .. 1: the people's
// territory, its ground, the nearness of its places); whether a point is within a place's core (plus a margin)
const PlanPlace* planPlaceIn(const WorldPlan& P, int gLat, int gLonWrapped);
double planCountryside(const WorldPlan& P, double lat, double lon, int people);
const PlanPlace* planCoreAt(const WorldPlan& P, const Vec3& unit, double marginM);
// the places whose core comes within `radiusM` of a point (both peoples'), and the edges whose way can
void planPlacesNear(const WorldPlan& P, const Vec3& unit, double radiusM, std::vector<const PlanPlace*>& out);
struct PlanEdgeRef { int people = 0, index = 0; const PlanEdge* e = nullptr; };
void planEdgesNear(const WorldPlan& P, const Vec3& unit, double radiusM, std::vector<PlanEdgeRef>& out);
// an edge's way: its waypoints from the one place's edge to the other's, every few kilometres on dry, gentle ground (a maglev's on
// its straight line), made once and kept; the ground walks the legs between them
const std::vector<Vec3>& planEdgeWay(const BodyGen& g, const WorldPlan& P, const PlanEdgeRef& r);
constexpr double PLAN_LEG_KM = 3.0;   // the waypoints' spacing
// the works of the ways that cross a 2 km cell (`gLon` wrapped): a maglev's spans of about 150 m on its pylons, each where its middle lies in
// the cell (the cell's settlement carries them, or the cell holds them alone: `SC_WORKS`)
struct PlanSpan { Vec3 a, b; double h = 20; uint64_t seed = 0; int people = 0; };
void planSpansIn(const BodyGen& g, const WorldPlan& P, int gLat, int gLon, std::vector<PlanSpan>& out);

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do
// a whole plan built now, not cached (the harness: `parallel` reads the grid on the workers; the unit compares it with the cached)
std::shared_ptr<WorldPlan> buildWorldPlan(const BodyGen& g, bool parallel = false);
// a people's plan as if it had reached `age` (the harness's plans of the ages over one grid)
void planPeople(const BodyGen& g, const PlanGrid& grid, int people, int age, PeoplePlan& out);
