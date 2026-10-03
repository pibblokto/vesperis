// C-01: the ruins of a world. The monoliths of the old ones (M4-07: 5% of the cells of a 2 km latitude/longitude grid on
// felisian, quartz and ocean worlds hold one structure) and, on a world that had a civilisation (TR_CIVILISATION: one
// felisian or desert world in eight), the settlements of that people: hamlets, villages and towns of buildings with walls,
// doorways, windows, roofs and inner rooms, in a style the whole world shares (`Culture`), decayed by their age. One
// deterministic function of the grid cell (the body's seed and the cell's indices), read by the surface view (the drawing,
// the colliders), the landmark finder, the landing map and the harness: a settlement is where it is whatever the landing
// site or the local frame. The layout is in metres east and north of the settlement's centre; the pieces (`RuinElem`) are
// boxes and domes over the ground at their own foot, so a settlement follows the terrain and no ground is flattened.
#pragma once
#include "planetmap.h"
#include <vector>
#include <string>
#include <cstdint>

enum RuinKind { RK_COLUMNS = 0, RK_CUBE, RK_DOME, RK_WALLS, RK_GIANT_CUBE, RK_SETTLEMENT };
enum SettlementClass { SC_HAMLET = 0, SC_VILLAGE, SC_TOWN, SC_MONUMENT };   // a monument: one building alone (a dead desert world's, in the cells the old ones' monoliths take elsewhere)
enum SettlementPlan { SP_CLUSTER = 0, SP_STREET, SP_RADIAL, SP_GRID, SP_LONE };
enum BuildingKind { BK_HOUSE = 0, BK_HALL, BK_TOWER, BK_ROTUNDA, BK_GATE, BK_PLATFORM, BK_COLONNADE, BK_STELA, BK_RUBBLE, BK_WALL,
                    BK_QUAY, BK_MOLE, BK_BOLLARD, BK_TERRACE, BK_CISTERN, BK_WINDWALL, BK_CAIRN, BK_COUNT };   // C-08: what the terrain asked for
extern const char* const SETTLEMENT_CLASS_NAMES[4];   // "HAMLET", "VILLAGE", "TOWN", "MONUMENT"
extern const char* const BUILDING_KIND_NAMES[BK_COUNT];

// the culture of a world: what every settlement of it shares, from the body's seed
struct Culture {
    int style = 0;          // 0 smooth walls, 1 striated (the rock's meso tile)
    int family = 0;         // the walls' material family: 0 the rock, 1 the sand (adobe)
    double tall = 1.0;      // the proportion of its walls (0.75 squat .. 1.3 tall)
    double decay = 0.5;     // the base decay of its ruins (0 whole .. 1 fallen)
    bool stoneRoofs = true; // false: the roofs were timber and none is left
    int plan = SP_GRID;     // the towns' plan: a grid of streets or rings round the centre
    bool towers = true;     // the culture built towers
    double buried = 0;      // how deep its walls sank into the ground (metres at full decay): a desert's drifted sand
    // C-08 weathering by age: how long ago the people ended (years, 200 to 40,000, log-uniform from the seed) and how wet the
    // world is (0 a desert .. 1 a rainy felisian); the base decay is theirs now (`decayOf`), the sand drifts grow with the age
    double ageYears = 2000, wet = 0.5;
    // C-09 the roads (`galaxy/roads.*`): half the width (metres, 2.5-4; broader between towns), paved with stone (the rock
    // family's material) or beaten (the ground's own, bare and darker), and the share of their length that is gone (the
    // age's and the rain's: a tenth of a young desert people's, over half of a twenty-millennia people's on a wet world)
    double roadHalf = 3; bool paved = true; double roadWear = 0.2;
    int people = 0;         // C-13: which of the world's peoples this is (0 the first; 1 the second, where the world had two)
};
// C-13: the peoples of a world. One world in five of those that had a civilisation had two, each with a culture, a lore, a
// tongue, a voice, a music and fifty recordings of its own: `peopleGen` is the generator a people's draws read (the world's
// own for the first people, the world's with its seed mixed for the second, so every draw of C-01..C-12 that read the body's
// seed is the people's; it is never handed to the planet function: the terrain is the world's). The two lived on either side
// of a great circle (`peopleDivide`): of twelve drawn from the seed the one that crosses the least land, so where the world has
// two continents the divide runs through the sea between them; `peopleAt` says whose side a point is. Whether they ended
// together is a coin of the world's (`peoplesEndedTogether`): the second people's age is then the first's
int peoplesOf(const BodyGen& g);                          // 0 without the trait, else 1 or 2
BodyGen peopleGen(const BodyGen& g, int people);          // the generator a people's draws read
Vec3 peopleDivide(const BodyGen& g);                      // the pole of the great circle between the two (the first people's side is dot >= 0); found once a world and kept
int peopleAt(const BodyGen& g, double lat, double lon);   // which people lived at a point (0 on a world of one)
bool peoplesEndedTogether(const BodyGen& g);              // one catastrophe for both
Culture cultureOf(const BodyGen& g, int people = 0);      // a people's culture (C-13: the second's by its own draws, its age the first's when they ended together)
double decayOf(double ageYears, double wet);   // 0.05..0.95: a village of two centuries stands, one of thirty millennia is foundations

struct Building {
    int kind = BK_HOUSE;
    double x = 0, z = 0;    // metres east and north of the settlement's centre
    double heading = 0;     // radians from north, clockwise: the front (the wall with the door, side 0) faces this way
    double hw = 3, hd = 4;  // half extents across the heading (x') and along it (z')
    double hWall = 3;       // the walls' height when they stood (metres)
    double thick = 0.5;     // the walls' thickness
    double decay = 0.5;     // 0 whole .. 1 fallen
    int door = 0;           // the side with the doorway: 0 front (+z'), 1 right (+x'), 2 back (-z'), 3 left (-x'); -1 none
    uint64_t seed = 0;
};
// one drawn piece: a box (walls, columns, slabs, blocks) or a dome; metres from the settlement's centre
struct RuinElem {
    int shape = 0;          // 0 a box, 1 a dome (a hemisphere of radius hx over y0; `broken` the share of its height that fell)
    double x = 0, z = 0, heading = 0;
    double hx = 0.5, hz = 0.5;   // half extents across and along the heading
    double y0 = 0, y1 = 1;       // bottom and top over the ground at (x, z); a piece with y0 under 0.01 stands on the ground
    int part = 0;           // 0 a wall, 1 a column, 2 a slab (a roof, a platform's tier, a step), 3 a block (rubble), 4 a lintel, 5 a stela
    bool glyphs = false;    // the front face carries a line of glyphs
    double broken = 0;      // a dome: the share of its height that fell (0 whole)
    int building = -1;
};
// C-08: what the terrain round a settlement says (`readSite`: samples of the planet function at 64 m, no tile built), read once
// when the layout is made and kept with it: the harbour, the terraces, the cisterns, the wind walls and the cairns come from it
struct SiteRead {
    double slope = 0;       // the grade across the settlement (rise over run) and the heading the ground falls toward
    double downhill = 0;
    double shoreDist = 1e9; // metres from the centre to the nearest water (the sea, a lake, a river) or a dead desert's old sea; 1e9 none within reach
    double shoreDir = 0;    // the heading toward it
    bool shoreDry = false;  // the shore is the old sea's (dry now)
    double relief = 0;      // the planet function's mountain-ness at the centre (0..1)
    double ridgeDir = 0;    // the heading of the highest ground round the settlement
    double wind = 0;        // the prevailing wind's heading here (where it blows from), from the world's wind angle, the hemisphere and the band
    bool desert = false;
};
struct RuinSpec {
    int kind = RK_CUBE;     // RuinKind
    int style = 0;          // 0 smooth, 1 striated, 2 glowing lines (the monoliths' styles; a settlement's is its culture's)
    double size = 10;       // a monolith's size; a settlement's radius (metres)
    double lat = 0, lon = 0;   // the centre (radians)
    double heading = 0;
    uint64_t seed = 0;
    int people = 0;         // C-13: which of the world's peoples lived here (`peopleAt`): its culture, its lore, the fifty its shards are from
    int sclass = 0;         // SettlementClass
    int plan = 0;           // SettlementPlan
    bool walled = false;    // a town wall round it
    std::vector<Building> buildings;
    SiteRead site;          // C-08: what the terrain said
    bool harbour = false, terraced = false, cistern = false, windwall = false, cairns = false;   // C-08: the features it got
};
bool worldHasRuins(const BodyGen& g);   // felisian, quartz and ocean worlds (the old ones) and the civilisations' worlds
inline bool worldHadCivilisation(const BodyGen& g) { return g.hasTrait(TR_CIVILISATION); }
// the 2 km grid (M4-07): the cell of a point (gLon unwrapped, from the longitude given), and a row's number of cells
void ruinCellOf(const BodyGen& g, double lat, double lon, int& gLat, int& gLon);
int ruinCellsAround(const BodyGen& g, int gLat);
double ruinCellLat(const BodyGen& g);   // the grid's cell size in radians of latitude
// the ruin of a cell: false when the cell holds none (95% of the cells on a world of the old ones, 65% on a civilisation's).
// A civilisation's settlements rank town, village, hamlet, monument for the finders
// `withBuildings` false leaves the settlement's layout out (the landmark finder needs the class and the radius alone)
bool ruinOfCell(const BodyGen& g, int gLat, int gLon, RuinSpec& out, bool withBuildings = true);
SiteRead readSite(const BodyGen& g, const RuinSpec& r);   // C-08: the terrain round a settlement (its centre, heading and radius must be set)
std::string featureList(const RuinSpec& r);               // C-08: "a harbour, terraces" for the harness and the finders ("" none)
// C-08 (KI-345): a landing point inside a settlement's radius plus `marginM` is moved out radially to that distance (the capsule
// sets down outside the walls); true when it moved. Reads the cell and its eight neighbours without their layouts
bool settlementClearance(const BodyGen& g, double& lat, double& lon, double marginM = 30);
inline int settlementRank(int sclass) { return sclass == SC_MONUMENT ? 0 : sclass + 1; }
// the ground allows it: not under water, and a settlement not across a slope steeper than a quarter (five samples of the
// planet function at 64 m)
bool ruinSiteOk(const BodyGen& g, const RuinSpec& r);
// C-12: a point of the settlement lies outside every building's footprint widened by `margin` (the graves' ground)
bool settlementClear(const RuinSpec& r, double x, double z, double margin);
// the drawn pieces of a settlement at a level of detail: 0 whole (the walls with their doorways and windows, the roofs, the
// columns, the rubble), 1 the walls as one box each, 2 one box per building
void ruinElements(const RuinSpec& r, const Culture& c, int lod, std::vector<RuinElem>& out);
// C-03: the doorway's midpoint of a building with one (a house's, a hall's or a tower's door wall, a rotunda's missing
// segment), in the settlement's metres; false for the rest. The same draws of the building's seed as `ruinElements`
bool buildingDoor(const Building& b, double& dx, double& dz);
