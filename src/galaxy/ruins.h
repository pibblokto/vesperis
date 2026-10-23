// C-01: the ruins of a world. On a world that had a civilisation (TR_CIVILISATION: one felisian or desert world in eight), the
// settlements of that people: hamlets, villages and towns of buildings with walls, doorways, windows, roofs and inner rooms, in a
// style the whole world shares (`Culture`), decayed by their age. One deterministic function of a 2 km latitude/longitude grid's
// cell (the body's seed and the cell's indices), read by the surface view (the drawing, the colliders), the landmark finder, the
// landing map and the harness: a settlement is where it is whatever the landing site or the local frame. The layout is in metres
// east and north of the settlement's centre; the pieces (`Form`) are forms (A-02: C-01's boxes and domes among them) over the
// ground at their own foot, so a settlement follows the terrain and no ground is flattened. B-417: M4-07's monoliths of the old
// ones (columns, cubes, domes and walls in one cell in twenty of every felisian, quartz and ocean world, a people there or not)
// are gone: a world's ruins are its people's.
#pragma once
#include "planetmap.h"
#include "forms.h"
#include "genome.h"
#include <vector>
#include <string>
#include <cstdint>

enum SettlementClass { SC_HAMLET = 0, SC_VILLAGE, SC_TOWN, SC_MONUMENT,   // a monument: one building alone in the countryside
                       SC_CITY, SC_CAPITAL,                                  // T-01: the plan's cities and its capital (galaxy/plan.h), their cores spanning cells
                       SC_WORKS };                                           // T-01: a cell that holds only a network's works (a maglev's pylons): no shards, no graves
enum SettlementPlan { SP_CLUSTER = 0, SP_STREET, SP_RADIAL, SP_GRID, SP_LONE, SP_GREAT };   // A-03: SP_GREAT the futuristic age's few great structures
enum BuildingKind { BK_HOUSE = 0, BK_HALL, BK_TOWER, BK_ROTUNDA, BK_GATE, BK_PLATFORM, BK_COLONNADE, BK_STELA, BK_RUBBLE, BK_WALL,
                    BK_QUAY, BK_MOLE, BK_BOLLARD, BK_TERRACE, BK_CISTERN, BK_WINDWALL, BK_CAIRN,   // C-08: what the terrain asked for
                    // A-03: the ages' own (galaxy/buildings.h); C-01's ten are the medieval age's
                    BK_HUT, BK_LONGHOUSE, BK_TENT_RING, BK_STONE_CIRCLE, BK_MENHIR, BK_DOLMEN, BK_PALISADE,                       // prehistoric
                    BK_TEMPLE, BK_WALL_TOWER,                                                                                    // medieval
                    BK_ROW, BK_TENEMENT, BK_VILLA, BK_CHURCH, BK_TOWN_HALL, BK_STATION, BK_MILL, BK_GASOMETER, BK_WATER_TOWER,   // industrial
                    BK_HEADFRAME, BK_OBSERVATORY,
                    BK_BLOCK, BK_SUBURB, BK_HIGHRISE, BK_STADIUM, BK_MAST, BK_POWER, BK_TANKS,                                   // modern
                    BK_ARCOLOGY, BK_HABITAT, BK_SPIRE, BK_RING, BK_MAGLEV, BK_WRECK, BK_POD,                                     // futuristic
                    BK_SPIKE, BK_HALO, BK_PILLARS, BK_SHAFT, BK_MONOLITH,                                                        // far future
                    // A-04: the genome's: the focal accent (its style the `Focus`), a windmill (a medieval people's machine), an aqueduct (a
                    // people of the valleys), two towers joined by a bridge (a people of bridges), a tunnel's mouth (a people of tunnels)
                    BK_FOCUS, BK_WINDMILL, BK_AQUEDUCT, BK_TWIN, BK_PORTAL,
                    // T-02: a harbour's crane (style 0 an industrial people's jib crane, 1 a modern people's gantry), a container yard's stacks,
                    // a town's reservoir (style 0 a basin of masonry in its bank, 1 a round tank of concrete)
                    BK_CRANE, BK_CONTAINERS, BK_RESERVOIR,
                    // T-03: the works' (galaxy/works.h): earth (a barrow, a long barrow, a spoil heap, a bank, a ring bank: `style`), a dam's wall, a
                    // dish, a tomb, a redoubt or a casemate, a pillbox, dragon's teeth, a machine's wreck (a tank, a haul truck), an aircraft's, a
                    // ship's, a launch pad and its tower, a great hall (an assembly building, a mine's mill), a colossus, a wind turbine, a
                    // lighthouse, a lattice, a quarry's derrick, cut blocks, a sphere on its point, an elevator's anchor and its ribbon, a trilithon,
                    // a conveyor
                    BK_MOUND, BK_DAM, BK_DISH, BK_TOMB, BK_FORT, BK_BUNKER, BK_TEETH, BK_VEHICLE, BK_AIRCRAFT, BK_SHIP, BK_LAUNCHPAD, BK_HANGAR,
                    BK_COLOSSUS, BK_TURBINE, BK_LIGHTHOUSE, BK_LATTICE, BK_DERRICK, BK_BLOCKS, BK_SPHERE, BK_TETHER, BK_TRILITHON, BK_CONVEYOR,
                    // R-412: the people's street furniture (`style` its `Furniture`: a well, a lamp, a fountain, a figure on its plinth)
                    BK_FURNITURE,
                    // D-05: a people that lost its sun: a dome over the warm ground (`style` 0 an industrial people's iron ribs and glass on a
                    // ring of brick, 1 a modern people's steel and glass on a ring of concrete, 2 the far future's whole), a glasshouse (0 an
                    // industrial people's iron and glass on low walls of brick, 1 a modern people's tunnel of hoops, 2 a later people's vault)
                    BK_DOME, BK_GREENHOUSE,
                    BK_COUNT };
extern const char* const SETTLEMENT_CLASS_NAMES[7];   // "HAMLET", "VILLAGE", "TOWN", "MONUMENT", "CITY", "CAPITAL", "WORKS"
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
    // world is (0 a desert .. 1 a rainy felisian); the base decay is theirs now (`decayOf`), the sand drifts grow with the age.
    // A-01: the years are the profile's (`peopleAgeOf`, galaxy/peoples.h: C-08's span for the first two ages, longer for the late
    // ones), and the weathering reads them over the age's durability (`weatherYears`: a late people's works outlast the years)
    double ageYears = 2000, wet = 0.5;
    int age = 1;                 // A-01: the age the people reached (Age: 0 prehistoric .. 5 far future)
    double weatherYears = 2000;  // A-01: the years as the weather counts them (ageYears over `ageDurability`)
    // C-09 the roads (`galaxy/roads.*`): half the width (metres, 2.5-4; broader between towns), paved with stone (the rock
    // family's material) or beaten (the ground's own, bare and darker), and the share of their length that is gone (the
    // age's and the rain's: a tenth of a young desert people's, over half of a twenty-millennia people's on a wet world)
    double roadHalf = 3; bool paved = true; double roadWear = 0.2;
    int people = 0;         // C-13: which of the world's peoples this is (0 the first; 1 the second, where the world had two)
    // A-02: its colours and windows (galaxy/forms.h). A-03: the age's materials in its four slots. A-04: the genome's colours through
    // the age's materials (the structure, the accent, the roofing or metal and glass, timber or light: `culturePalette`, galaxy/buildings.h)
    FormPalette palette;
    // A-04: the architecture genome (galaxy/genome.h), the people's own through its ages; the ways below are it read through the age
    // (`cultureWays`, galaxy/buildings.h), and C-01's `tall`, `plan`, `towers`, `style` and `stoneRoofs` above are its proportion, its
    // streets, its appetite for height, its structure and its roofs now
    Genome genome;
    int technique = TQ_POST_LINTEL;   // the age's way of spanning (`techniqueOf`)
    // A-03, the genome's since A-04: the roof (`RoofForm`), the temple (0 a basilica with its tower, 1 a hall under a dome with minarets,
    // 2 a stepped mass, 3 a round temple under a dome or a cone), the huts (0 wattle on posts, 1 dry stone), the towers (0 a slab, 1
    // setbacks, 2 a cylinder, 3 a prism, 4 boxes cantilevered off a core, 5 a shell's waist), the great structure (the arcology: 0 a stepped
    // pyramid, 1 a cone, 2 grown; the far future's: 0 a spike, 1 a ring, 2 pillars, 3 a shaft, 4 a solid)
    int roof = RF_PITCHED, temple = 0, hut = 0, towerForm = 0, great = 0;
    double life = 1.0;   // A-04: its works' lifetimes over its age's materials' (built light 0.5 .. built to last 2: `GV_LASTING`)
    uint64_t seed = 0;   // T-02: the people's seed (`peopleGen(g, people).seed`): its ways read again in an older age (`cultureAtAge`)
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
// P-04: the world of a system its signal and its peoples' charts speak of: the first world with a people of C-01's kinds (felisian, desert:
// 1.3.2's, so none of theirs moves), else the first dead world with one; -1 none
int systemPeoplesWorld(const StarSystem& sys);
Culture cultureOf(const BodyGen& g, int people = 0);      // a people's culture (C-13: the second's by its own draws, its age the first's when they ended together)
// A-03: a people's culture as if it had reached `age` and ended `years` ago (its palette the age's): the harness's sheets of the ages
Culture cultureAs(const BodyGen& g, int people, int age, double years);
double decayOf(double ageYears, double wet);   // 0.05..0.95: a village of two centuries stands, one of thirty millennia is foundations (C-08; A-03: masonry's)
// T-02: a people's culture building in an age's way (its genome read through that age: the roofs, the temple, the huts, the materials'
// leanings) and weathered `extraYears` longer (a place left before the end; B-414: of its people's last age, the age a world shows); the
// palette stays the people's (its banks)
Culture cultureAtAge(const Culture& c, int age, double extraYears = 0);

struct Building {
    int kind = BK_HOUSE;
    double x = 0, z = 0;    // metres east and north of the settlement's centre
    double heading = 0;     // radians from north, clockwise: the front (the wall with the door, side 0) faces this way
    double hw = 3, hd = 4;  // half extents across the heading (x') and along it (z')
    double hWall = 3;       // the walls' height when they stood (metres)
    double thick = 0.5;     // the walls' thickness
    double luck = 0;        // A-03: its fortune against the weather (the settlement's and its own, -0.4..0.4): a material half gone is more
                            // or less gone in it (`buildingDecay`, galaxy/buildings.h); C-01's one decay is its materials' now
    int door = 0;           // the side with the doorway: 0 front (+z'), 1 right (+x'), 2 back (-z'), 3 left (-x'); -1 none
    int age = 1;            // A-03: the age whose way built it (its kind's; B-414: its people's last, a work's pieces their work's)
    int style = 0;          // A-03: the kind's form its culture chose (a temple: `Culture::temple`; a tower: `towerForm`), so a room is known without the culture
    // A-04: what its people's genome made of it that its room reads (so a room is still known without the culture): its plan's form
    // (`PlanForm`: a rectangle, a circle, a polygon of `sides`, an ellipse, an L, a court), what it stands on (`Foundation`), and its
    // doorway in the middle of its wall (a people of bilateral symmetry, or of bulging walls)
    int form = PF_RECT, sides = 0, found = 0;
    bool centred = false;
    uint64_t seed = 0;
    // T-02: the town's district it stands in (`District`, galaxy/towns.h) and how many years before its people's end it was left (a town
    // the history says was left early: its decay counts them too; 0 used to the end)
    int district = 0;
    double before = 0;
    // T-03: a work whose forms stand at heights over the reference level, not over the ground at their foot (a dam's wall from its valley's
    // floor to its crest, an aqueduct's arches to its channel): the height they rise from (-1e9: the ground's, as every building's)
    double base = -1e9;
    // R-412: how deep its front yard is to the street it fronts (a town's lots; 0 none known): what bounds it, where its people bounded yards;
    // whether it stands in a row or a compound (its neighbours at its sides: no turrets on its corners, no garden behind it)
    double yard = 0;
    bool joined = false;
};
// A-04: what a building stands on, by its people's relation to the land: the ground; stilts (its room the ground under its floor); a
// mound (B-416: nothing drawn round it: its earth bank, as wide as the building's longest side, ran over its neighbours); a pit it is
// sunk in; a cut in the rock round it; a terrace's kerb (B-416: the pits, the cuts and the kerbs the hand-made ages')
enum Foundation { FD_GROUND = 0, FD_STILTS, FD_MOUND, FD_PIT, FD_CUT, FD_TERRACE };
// the drawn pieces are `Form`s (A-02, galaxy/forms.h): C-01's boxes (walls, columns, slabs, blocks) and domes among the forms of the ages
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
    // P-04: the present's (a dead world's people built on its then-world): what the wind drifts against the walls (0 nothing, 1 sand on a dry
    // world, 2 snow on a frozen one) and a shore whose sea is ice now
    int drift = 0;
    bool shoreIce = false;
};
// T-02 (galaxy/towns.h): a town's plan. A street of it (a segment, metres from the settlement's centre, its half width, its rank: a lane,
// a street, a main street, an avenue, a ring road; the age that laid it and whether it was paved); an open square; a field outside it (a
// strip, a walled field, a plot, an irrigation circle, a terrace); a layer (the ring of the town an age built: the old core inside)
struct Street { double ax = 0, az = 0, bx = 0, bz = 0, half = 2.5; int rank = 1, age = 1; bool paved = true; };
struct Plaza { double x = 0, z = 0, r = 10; int age = 1; };
struct FieldPatch { double x = 0, z = 0, hw = 20, hd = 20, heading = 0; int kind = 0, age = 1; };
struct TownLayer { int age = 1; double r0 = 0, r1 = 0; };
struct RuinSpec {
    double size = 10;       // the settlement's radius (metres)
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
    int harbourKind = 0;      // T-02: the harbour by the age: 0 a quay (C-08's), 1 a dock and its cranes, 2 a container port
    bool reservoir = false;   // T-02: an industrial or a modern town's reservoir
    // T-01: a place of the world's plan (galaxy/plan.h): its rank in the people's founding (-1 the countryside's, hashed in its cell) and
    // the buildings its core holds (a capital's and a city's: the layout's count; 0 the class's own draw)
    int rank = -1, wanted = 0;
    // T-02: the town's plan (galaxy/towns.h): its streets, its squares, its fields, its layers by age (B-414: one, its people's last); the age
    // it was built in (-1 its people's: B-414, every settlement's; a work's pieces their work's), and the years before the end it was left
    std::vector<Street> streets;
    std::vector<Plaza> plazas;
    std::vector<FieldPatch> fields;
    std::vector<TownLayer> layers;
    int buildAge = -1;        // -1 the people's own age
    double before = 0;        // years before the people's end it was left (0: used to the end)
    int founded = 0;          // the year of its founding in the people's count (the history's; 0 unknown: the countryside's)
    int work = -1;            // T-03: a work's pieces (`WorkKind`, galaxy/works.h; `sclass` SC_WORKS), -1 a settlement's
};
inline bool worldHadCivilisation(const BodyGen& g) { return g.hasTrait(TR_CIVILISATION); }   // the worlds with ruins (B-417: no others)
// the 2 km grid (M4-07): the cell of a point (gLon unwrapped, from the longitude given), and a row's number of cells
void ruinCellOf(const BodyGen& g, double lat, double lon, int& gLat, int& gLon);
int ruinCellsAround(const BodyGen& g, int gLat);
// T-01 (a C-09 fix): the cell of row `gLatTo` at the longitude of cell (gLatFrom, gLonFrom)'s centre: rows have their own longitude steps, so
// far from the meridian the same index in the next row lies cells away (at 50 degrees and an index of 5,000, two cells and a half)
int ruinCellLonIn(const BodyGen& g, int gLatFrom, int gLonFrom, int gLatTo);
double ruinCellLat(const BodyGen& g);   // the grid's cell size in radians of latitude
// the settlement of a cell: false when the cell holds none (most cells: outside the people's land all of them). The settlements rank
// town, village, hamlet, monument for the finders
// `withBuildings` false leaves the settlement's layout out (the landmark finder needs the class and the radius alone)
bool ruinOfCell(const BodyGen& g, int gLat, int gLon, RuinSpec& out, bool withBuildings = true);
// A-03: a settlement of the given class laid out at a cell's spot in the given culture (the harness's sheets of the ages)
void settlementOf(const BodyGen& g, int gLat, int gLon, const Culture& c, int sclass, RuinSpec& out);
SiteRead readSite(const BodyGen& g, const RuinSpec& r);   // C-08: the terrain round a settlement (its centre, heading and radius must be set)
std::string featureList(const RuinSpec& r);               // C-08: "a harbour, terraces" for the harness and the finders ("" none)
// C-08 (KI-345): a landing point inside a settlement's radius plus `marginM` is moved out radially to that distance (the capsule
// sets down outside the walls); true when it moved. Reads the cell and its eight neighbours without their layouts
bool settlementClearance(const BodyGen& g, double& lat, double& lon, double marginM = 30);
inline int settlementRank(int sclass) { return sclass == SC_MONUMENT || sclass == SC_WORKS ? 0 : (sclass == SC_CITY ? 4 : (sclass == SC_CAPITAL ? 5 : sclass + 1)); }
inline bool townOrGreater(int sclass) { return sclass == SC_TOWN || sclass == SC_CITY || sclass == SC_CAPITAL; }   // T-01: a town's ways in a city's core
// T-01: the radius a settlement of a class has in a culture from its cell's seed (the layout's first draws: the plan sites a town by it)
double settlementRadiusOf(const Culture& c, int sclass, uint64_t seed);
// the ground allows it: not under water, and a settlement not across a slope steeper than a quarter (five samples of the
// planet function at 64 m)
bool ruinSiteOk(const BodyGen& g, const RuinSpec& r);
// C-12: a point of the settlement lies outside every building's footprint widened by `margin` (the graves' ground)
bool settlementClear(const RuinSpec& r, double x, double z, double margin);
// the drawn pieces of a settlement at a level of detail: 0 whole (the walls with their doorways and windows, the roofs, the
// columns, the rubble), 1 the walls as one box each, 2 one box per building. A-03: each building its age's (`buildingForms`,
// galaxy/buildings.h)
void ruinElements(const RuinSpec& r, const Culture& c, int lod, std::vector<Form>& out);
// C-03: the doorway's midpoint of a building with one, in the settlement's metres; false for the rest. A-03: its room's
// (`buildingRoom`, galaxy/buildings.h)
bool buildingDoor(const Building& b, double& dx, double& dz);
