// A-03 (2026-10-09): what each age builds. The buildings of the six ages (`docs/ideas/IDEAS-civilisation-expansion.md`, section
// 2), each a generator of A-02's forms (`galaxy/forms.h`) from its building's seed: the prehistoric age's huts, longhouses, tent
// rings, stone circles, menhirs, dolmens and palisades; the medieval age's houses, halls, keeps, temples, walls and gates (C-01's
// kinds, roofed); the industrial age's terraced rows, tenements, villas, churches, town halls, stations, mills with their
// chimneys, gasometers, water towers, headframes and observatories; the modern age's blocks, suburbs, towers, stadiums, masts,
// power stations and tanks; the futuristic age's arcologies, habitat domes, spires, rings, maglev lines, wrecks and pods; the far
// future's single structures. A kind is its age's way of building, so a people's older works can be built in their own age's
// way wherever T-02 lays them down (`Building::age`).
//
// The decay is the material's (`materialDecay`): timber, thatch and hide gone in centuries (post holes, rings of stones), earth
// kept by a desert and melted by rain, dry stone and megaliths standing tens of millennia (leaning, fallen), mortared stone and
// brick C-08's model, iron rusting to skeletons, concrete falling over centuries to a few millennia (a tower to its stump), mass
// concrete lasting, glass gone at once (empty frames), composites lasting millennia then crumbling, the far future's materials
// not weathering at all. Each building reads the years since its people's end and the world's wet, and its own luck.
//
// Every building with a room (most of them: a hut, a house, a temple's nave, a tower's lobby, a dome's portal) says where it is
// (`buildingRoom`): its floor, its doorway, and the spots where a record can lie (C-03's shards now, T-04's records later).
//
// A-04: the people's genome (galaxy/genome.h) shapes every kind: its plan's form, its roof, its walls' profile and dress, its tops, its
// towers' sections, its motif, its colours; and the genome's own works: the focal accent (`BK_FOCUS`), a windmill, an aqueduct, two
// towers joined by a bridge, a tunnel's mouth.
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

// A-04: a people's palette, its genome's colours through its age's materials: the structure (the ground's rock tinted toward its base
// colour, or painted in it; its brick, its concrete, its composite), the accent, the roofing or the glass in its tint, timber or (the
// futuristic and the far future's) its light; its windows (its openings' shape, rhythm and share of the wall, by the age's bays and
// storeys; none before the medieval age; the far future's lit lines or lamps)
FormPalette culturePalette(const Culture& c, uint64_t seed);
// A-04: a culture's ways from its genome and its age (`Culture::technique`, `roof`, `temple`, `hut`, `towerForm`, `great`, `life`, C-01's
// `tall`, `plan`, `towers`, `style`, `stoneRoofs`) and its palette; `cultureOf` and `cultureAs` call it, and the harness after setting a
// variable
void cultureWays(Culture& c, uint64_t peopleSeed);
