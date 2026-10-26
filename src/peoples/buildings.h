// What each age builds (A-03; docs/reference/civilisations/built-world.md): the buildings of the six ages, each a generator of A-02's forms
// (peoples/forms.h) from its building's seed, from the prehistoric huts and stone circles to the far future's single structures. A kind is
// its age's way of building (`Building::age`). The decay is the material's (`materialDecay`: timber gone in centuries, dry stone standing
// tens of millennia, iron rusting to skeletons, concrete falling to its stump, glass gone at once, the far future's not weathering), read
// with the years since the end, the world's wet and the building's own luck. A building with a room says where it is (`buildingRoom`: its
// floor, its doorway, the spots where a record can lie). The people's genome (peoples/genome.h) shapes every kind and adds its own works
// (the focal accent `BK_FOCUS`, a windmill, an aqueduct, towers joined by a bridge, a tunnel's mouth).
#pragma once
#include "ruins.h"

enum BuiltMaterial { MT_TIMBER = 0, MT_EARTH, MT_DRYSTONE, MT_MASONRY, MT_IRON, MT_CONCRETE, MT_MASS, MT_GLASS, MT_COMPOSITE, MT_ETERNAL, MT_COUNT };
extern const char* const BUILT_MATERIAL_NAMES[MT_COUNT];   // "timber", "earth", "dry stone", "masonry", ...
// how much of a material the weather has taken after `years` on a world of `wet` (0 a desert .. 1 a rainy felisian): 0 whole,
// 1 gone (a trace). Each a log-logistic in the years: the years to half gone at a middling rain, the decades from a tenth to
// nine tenths, how much faster the rain takes it. Masonry is C-08's `decayOf` to the digit (without its clamp)
double materialDecay(int material, double years, double wet);
// the same for one building of a culture: its people's years since the end and its world's wet, its own luck spread where the
// material is half gone (a material whole or gone is so for every building)
double buildingDecay(const Building& b, const Culture& c, int material);

// the age a kind belongs to (C-01's dwellings and towers the medieval age's; -1 the works every age built its own way: a gate, a wall,
// a stela, a heap, C-08's features), and its name for a record's place ("A HUT", "A TEMPLE", "THE FOOT OF A STANDING STONE")
int buildingAge(int kind);
const char* buildingPlaceName(int kind);

// a building's pieces at a level of detail (0 whole, 1 the walls as one box each, 2 the building as a few boxes), each tagged
// with its material (`Form::material`); the drifts of sand on a desert against the walls the wind meets (C-08). The pieces of
// a settlement are `ruinElements` (ruins.h), building by building
void buildingForms(const Building& b, const Culture& c, const SiteRead& st, int lod, std::vector<Form>& out, int bi);

// a building's room: its floor in the building's frame (a rectangle of half extents hw x hd round (cx, cz), or a circle of radius
// r), its doorway's midpoint in the settlement's metres (none for an open ring or a foot), the spots a record can lie at in the
// building's frame, the best first, and whether they are at its foot (outside) rather than in a room
struct BuildingRoom {
    bool round = false, outside = false, door = false;
    double cx = 0, cz = 0, hw = 0, hd = 0, r = 0;
    double doorX = 0, doorZ = 0;           // the settlement's metres
    double doorAx = 1, doorAz = 0, doorL = 0;   // the doorway's wall: its axis and half length (the unit's walk along it)
    std::vector<std::pair<double, double>> spots;
};
bool buildingRoom(const Building& b, BuildingRoom& out);
// A-04: a culture's ways from its genome and its age (`Culture::technique`, `roof`, `temple`, `hut`, `towerForm`, `great`, `life`, C-01's
// `tall`, `plan`, `towers`, `style`, `stoneRoofs`) and its palette; `cultureOf` and `cultureAs` call it, and the harness after setting a
// variable
void cultureWays(Culture& c, uint64_t peopleSeed);

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do

// a people's palette, its genome's colours through its age's materials: the structure (the ground's rock tinted toward its base
// colour, or painted in it; its brick, its concrete, its composite), the accent, the roofing or the glass in its tint, timber or (the
// futuristic and the far future's) its light; its windows (its openings' shape, rhythm and share of the wall, by the age's bays and
// storeys; none before the medieval age; the far future's lit lines or lamps)
FormPalette culturePalette(const Culture& c, uint64_t seed);
// B-415: a genome variable as the people's age let it build: the age speaks first on its everyday buildings, the people on its great ones
// (`great`: its temples, churches, town halls, palaces, keeps, focal accent and monuments); the generators read the genome through it
int ageExpressed(const Culture& c, int var, bool great);
