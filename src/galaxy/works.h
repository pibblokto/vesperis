// T-03 (2026-10-09): the works (`docs/ideas/IDEAS-civilisation-expansion.md`, section 6: "it can be virtually anything"). Until T-03 a
// people's works stood in its towns (the mills, the stations, the harbours, a modern city's stadium and power station) or were its plan's
// (a far-future people's one to three structures, a futuristic people's maglev). Everything else people made outside their towns is here,
// each placed by a rule of the terrain or by its people's history (H-02), and built in the way of its people's last age (B-414: a world shows
// only the age its people reached; a work of a place left before that age, a battle of an earlier age, a later people's old stones are gone):
//
// * by the terrain: a quarry in the rock its town was built of (a face cut into the hillside in benches, an open pit; a prehistoric
//   people's flint pits), a mine where the plan read coal and ore (adits, headframes and their spoil, an open pit), a dam at a valley's
//   narrows holding its reservoir, an aqueduct from a spring on higher ground carried over the low ground on arches, an observatory on the
//   highest summit near its town, a shrine where its people's holy place was (a summit, a spring, a cave, a grove, the open sky) and the
//   pilgrims' way to it, a necropolis on the slope facing its city, lines and figures on a flat dry plain, a henge or rows of standing
//   stones on open ground, a fortified line along the frontier, a launch site near the equator, an antenna field on a high plain, wrecks
//   on the shore, a colossus over its capital or at its harbour's mouth, a terraformed basin, wind farms on the ridges, a lighthouse on
//   a headland, the far future's inexplicable (a plain made flat, pillars to the horizon, a mountain cut into terraces, a lattice over a
//   valley, a sphere on its point);
// * by the history: a battlefield between the two sides of a war where the history fought its battle (cairns over the dead, barrows and
//   a memorial, trenches and shell craters, craters and the wrecks of machines, glassed craters), a dam, a mine, an aqueduct, an
//   observatory, a lighthouse, a launch site where the history finished one, the last shelter by the place the collapse ended in.
// * D-05, the works of the end of a people that lost its sun: the last shelter a hall into the rock by the warmest ground, a shelter by each
//   of the last places it left before (their last records with them), domes over the warm ground by those places (a medieval people's
//   vaulted hall over its spring, the later ages' ribs of iron or steel and their glass, a futuristic people's habitat) with their glasshouses
//   in rows, and the graves of the last generations by the last shelter; the last city is its last place, the lights left on (D-04) its
//   late peoples'.
//
// The world's works are made once (`worldWorksOf`, cached, on the plan's thread after the histories): each a place, a size, its age and
// the years before the end it was left, the numbers its rule found (a dam's floor, crest and reservoir), and what it does to the ground
// (`GroundEdit`: a cut, a pit, a ditch, a crater, a flattened plain, a reservoir's water, lines). The surface draws the ground's edits into
// the terrain (`SurfaceSite::addWorkEdits`) and the work's pieces as a settlement's (`workSpec`: buildings of the works' kinds, galaxy/
// buildings.h); the scanner hears them (WORKS, galaxy/landmarks.h) and the explorer marks them.
//
// B-418: the works of the land. T-03 made every work once a world, each rule a handful a people (24 quarries, 4 necropolises, 4 wrecks)
// for the hundreds of places of a people that held a third of its world: from nine landings in ten the scanner heard none within 40 km.
// The world's works are the history's and the world's great ones now (the battlefields, the dams, the aqueducts, the observatories, the
// lighthouses the history lit, the launch sites, the holy place and its way, the colossus, the fortified line, the antenna field, the
// terraformed basins, the far future's, the lines on the plains, the last shelter, the mines the history sank); a people's everyday works
// are its land's (`landWorkOf`): a grid of cells of LAND_CELL_M, a cell holding one where the countryside's density (`planCountryside`:
// the people's land, thicker near its places) by its age's appetite draws it, the kind what the cell's ground offered that age (a
// quarry where its rock shows, flint pits on a prehistoric people's open ground, a mine where the plan read coal and ore, a necropolis
// facing the settlement it served, standing stones, a wreck on a shore or a plain, a wind farm on a ridge, a shrine where the people's
// holy place would be, a lighthouse on a headland), sited by the kind's rule inside the cell (its reach and a margin inside, so no two
// touch), clear of the settlements, the cities' cores and the world's works. Made when first asked and kept (a few milliseconds a work).
#pragma once
#include "ruins.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

enum WorkKind {
    WO_QUARRY = 0, WO_MINE, WO_DAM, WO_AQUEDUCT, WO_OBSERVATORY, WO_SHRINE, WO_WAY, WO_NECROPOLIS, WO_GEOGLYPH, WO_HENGE, WO_FORT_LINE,
    WO_BATTLEFIELD, WO_LAUNCH, WO_ANTENNA, WO_WRECK, WO_COLOSSUS, WO_TERRAFORM, WO_SHELTER, WO_STRANGE, WO_WINDFARM, WO_LIGHTHOUSE,
    WO_FORT,   // B-418: a fort on a height in its age's way (`style`: 0 a hill fort's rings of bank, 1 a castle, 2 a redoubt, 3 pillboxes round a blockhouse)
    // D-05: a people that lost its sun's: a dome over the warm ground by its last places (`style`: 0 a medieval people's hall over its spring, 1 an
    // industrial people's of iron and glass, 2 a modern people's of steel and glass, 3 a futuristic people's habitat, 4 the far future's), and its
    // glasshouses in rows beside it
    WO_DOME, WO_GREENHOUSE,
    WO_COUNT
};
extern const char* const WORK_KIND_NAMES[WO_COUNT];   // "a quarry", "a mine", ... "a pilgrims' way" (the harness)
extern const char* const WORK_HEARD[WO_COUNT];        // what the scanner hears: "A QUARRY", "A MINE", "A DAM", ... "SOMETHING MADE"

// what a work does to the ground, in metres east and north of the work's centre (the surface draws it: `SurfaceSite::addWorkEdits`)
enum GroundEditKind {
    GE_CUT = 0,    // a face cut into a hillside: the floor at `level` from its front edge into the hill along `heading`, in benches `depth` high and
                   // `step` deep (a rectangle of half width `hw` across, `hd` along)
    GE_PIT,        // an open pit: its floor at `level` out to `hw`, then its benches (`depth` high, `step` wide) up to its rim at `r` (a flint pit's
                   // shaft: `hw` 0, the floor to the rim)
    GE_DITCH,      // a ditch along `pts`, `width` across and `depth` deep, a bank `bank` high on its far side (+) or on both (-)
    GE_BOWL,       // a crater of radius `r`, `depth` deep with its rim; `tone` scorched or glassed
    GE_FLAT,       // the ground made flat at `level` within `r` (its edge a cliff where the ground was higher, a bank where lower; `width` the edge)
    GE_WATER,      // a reservoir: the water at `level` over the cells of `mask` (a grid of `nx` x `nz` cells of `cell` metres from (gx0, gz0); 1 the water's,
                   // 2 its bank's ring); `tone` ET_DARK: drained (its dam breached), the old bed alone
    GE_LINE,       // lines on the ground along `pts`, `width` across (`width2` at the far end of a two-point line: a trapezoid), `tone`
    GE_TERRACES,   // a mountain cut into terraces: within `r` of its middle, ring bands `step` wide each cut flat at its level (`pts`, the outermost
                   // first: the lowest ground of its ring, rising inward)
    GE_COUNT
};
enum EditTone { ET_CLEARED = 0, ET_DARK, ET_GLASS, ET_PAVED };
struct GroundEdit {
    int kind = GE_LINE;
    double x = 0, z = 0, heading = 0;
    double hw = 0, hd = 0, r = 0;
    double level = 0, depth = 0, step = 0;
    double width = 0, width2 = -1, bank = 0;
    int tone = ET_CLEARED;
    bool fades = true;            // the years fill it and grow over it (a ditch, a crater, a line); a rock cut, a flattened plain and the water keep
    std::vector<float> pts;       // x, z pairs
    double gx0 = 0, gz0 = 0, cell = 0; int nx = 0, nz = 0;
    std::vector<uint8_t> mask;
};

struct Work {
    int kind = WO_QUARRY;
    int age = 1;               // the age whose way built it (B-414: its people's last, whatever era made it: a world shows that age alone)
    int people = 0, style = 0;
    double lat = 0, lon = 0;   // its middle (radians)
    Vec3 unit;
    double heading = 0;        // its axis (radians from north, clockwise)
    double radiusM = 100;      // its reach: the view's and the scanner's
    uint64_t seed = 0, id = 0;
    int place = -1;            // the history's place it served or was fought by (-1 none), and that place's rank in the plan
    int planRank = -1;
    int event = -1;            // the history's event that made it (a work finished, a battle, the end); -1 its rule's own
    int year = 0;              // the year it was made or fought in the people's count (0 the rule's: worked to the end)
    double before = 0;         // the years before the people's end it was left (a battlefield's from its battle)
    double ground = 0;         // the ground at its middle (the planet function at 64 m, the rivers left out)
    // the rule's numbers: a dam's floor and crest (metres over the reference), its axis and its crest's length, its reservoir's level; a
    // quarry's floor; an aqueduct's length; a battlefield's war and its two sides' places
    double floor = 0, crest = 0, level = 0, length = 0, width = 0, height = 0, axis = 0;
    int war = -1;
    Vec3 sideA, sideB;
    bool heard = true;         // the scanner hears it (a pilgrims' way's stretches and a line's lesser stretches are not)
    bool land = false;         // B-418: one of the land's (`landWorkOf`), not of the world's list
    uint32_t group = 0;        // the stretches of one line (a fortified line, a pilgrims' way, pillars to the horizon) share it
    std::vector<GroundEdit> edits;
};

struct WorldWorks {
    uint64_t seed = 0;
    int peoples = 0;
    double Rm = 6e6;           // the world's radius (metres)
    std::vector<Work> works;
    double buildMs = 0;
    int counts[2][WO_COUNT] = {};
};

// a world's works: cached by the world's seed (the last four), built here (after its plan and its peoples' histories) unless the plan's
// prefetch is building them (then waited for); null-free: a world without a people has none
std::shared_ptr<const WorldWorks> worldWorksOf(const BodyGen& g);
std::shared_ptr<const WorldWorks> worldWorksIfReady(const BodyGen& g);   // null while being built or never asked for (a render path)
std::shared_ptr<WorldWorks> buildWorldWorks(const BodyGen& g);           // built now, not cached (the unit compares it with the cached)
void worldWorksClearCache();                                             // and the land's
// B-418: the land's works. The grid: rows of LAND_CELL_M of latitude, each row's cells at least as wide (`landCellOf`, the longitude as
// given; `landCellsAround`, a row's count). A cell's work, null where it holds none: made when first asked (the world's works waited for:
// the land's keep clear of them) and kept, the last two worlds'; `buildLandWork` makes it now, not kept (the unit compares the two)
constexpr double LAND_CELL_M = 8000;
void landCellOf(const BodyGen& g, double lat, double lon, int& gLat, int& gLon);
int landCellsAround(const BodyGen& g, int gLat);
std::shared_ptr<const Work> landWorkOf(const BodyGen& g, int gLat, int gLon);
std::shared_ptr<const Work> buildLandWork(const BodyGen& g, int gLat, int gLon);
// the works of the world and of the land whose reach comes within `radiusM` of a point (the scanner's, the landing's, the harness's): the
// world's waited for; `IfReady` false while they are being built, nothing made (the view's, which never waits). `landRadiusM` (if given)
// the land's own reach (the view's nearer: it draws the works within a few kilometres)
void worksAround(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<std::shared_ptr<const Work>>& out);
bool worksAroundIfReady(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<std::shared_ptr<const Work>>& out, double landRadiusM = -1);
// a work's pieces as a settlement's (`RuinSpec::work` its kind, `sclass` SC_WORKS): buildings of the works' kinds about its middle, in the
// way of its age; a necropolis's grave plots in `fields` (C-12's stones, galaxy/graves.h)
void workSpec(const BodyGen& g, const Work& w, RuinSpec& out);
// a dam whose wall is half gone (its material's decay, as its generator reads it in `c`, its people's culture): its reservoir drained, its bed left
bool damBreached(const Work& w, const Culture& c);
// the harness's line: "a dam (modern, the Veathiash's) at 12.31 N 40.02 E, 84 m over its floor, 610 m long; its reservoir at 412 m"
std::string workLine(const Work& w);
// how far into the ground a ditch or a crater still reaches after `years` on a world of `wet` (1 new .. 0.1: silted, a hollow); `line`: how much of a
// line on the ground the years left (the stones swept aside stay aside on a dry plain for millennia)
double editLeft(double years, double wet, bool line = false);
