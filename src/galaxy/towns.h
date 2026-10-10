// T-02 (2026-10-09): towns by age (`docs/ideas/IDEAS-civilisation-expansion.md`, section 6). Until T-02 a settlement was C-01's plans
// (a cluster, a street, rings, a grid of 17 x 15 m blocks) scaled by the age, its streets the gaps between its houses. A town has a plan
// now: streets laid as its people laid them (`GV_STREETS`: a grid, spokes and rings, rings, an organic web of lanes, one street along a
// line, streets on the contour, a street with its branches) and as its age laid them (a medieval town's crooked and narrow, an
// industrial quarter's straight with back lanes behind its rows, a modern city's blocks of a hundred metres, its ring road and the
// suburbs' cul-de-sacs beyond), in layers: the old core the history founded it in (a medieval core inside a modern city, its wall's
// line become a boulevard), then the ring each later age built. Its buildings stand on the streets with their fronts to them
// (`ruins.cpp`'s layout), by district: the centre and its square, the old core, the houses, the works (an industrial town's mills,
// gasworks and station; a modern one's tanks and power), the harbour (a quay, a dock and its cranes, a container port), the towers
// of a modern city's middle, the suburbs; the height and the density falling from the centre to an edge (a wall, suburbs thinning
// into fields, nothing). Outside it its fields (a camp's plots, a medieval town's strips, enclosed fields, a modern people's
// irrigation circles), and terraced fields on a slope. The streets, the squares and the fields are drawn into the ground
// (`SurfaceSite`'s ground marks); the streets are kept clear of every building and its colliders (the unit).
#pragma once
#include "ruins.h"

enum StreetRank { SR_LANE = 0, SR_STREET, SR_MAIN, SR_AVENUE, SR_RING, SR_COUNT };
enum District { DI_NONE = 0, DI_CENTRE, DI_CORE, DI_HOUSES, DI_WORKS, DI_HARBOUR, DI_TOWERS, DI_SUBURBS, DI_COUNT };
enum FieldKind { FI_PLOT = 0, FI_STRIP, FI_WALLED, FI_ENCLOSED, FI_CIRCLE, FI_TERRACE, FI_COUNT };
extern const char* const STREET_RANK_NAMES[SR_COUNT];   // "lane", "street", "main street", "avenue", "ring road"
extern const char* const DISTRICT_NAMES[DI_COUNT];      // "", "the centre", "the old core", "the houses", "the works", "the harbour", "the towers", "the suburbs"
extern const char* const FIELD_KIND_NAMES[FI_COUNT];    // "plots", "strips", "walled fields", "enclosed fields", "irrigation circles", "terraced fields"

// a street's half width by its rank and the age that laid it (a medieval lane a metre and a half, a modern ring road thirteen)
double streetHalf(int rank, int age);
// the spacing of a pattern's parallel streets in an age (a medieval block about 36 m, an industrial 62, a modern city's 105)
double streetSpacing(int age);

// what a town's plan reads
struct TownSpec {
    double R = 100;                  // the radius its buildings reach
    double h0 = 0;                   // its axis (radians from north, clockwise)
    int sclass = SC_TOWN;
    int pattern = 0;                 // the people's streets (`Streets`, galaxy/genome.h)
    int number = 5;                  // the people's sacred number (spokes, rings)
    std::vector<TownLayer> layers;   // inside out: the old core first, the last the town's own age
    std::vector<double> exits;       // the bearings of the ways that leave it (the plan's), radians from north
    double plaza = 20;               // the centre's square's radius
    bool walled = false;             // the medieval layer's wall: its lesser streets end inside it, a street runs round inside it, the main
    double wallR = 0;                // streets go out by its gates
    int wallK = -1;
    double spacing = 1;              // the blocks' size over the age's (a city whose buildings could not line its streets lays wider blocks)
    bool shore = false;              // the streets kept to the land side of a line: (x, z) . (shoreNx, shoreNz) <= shoreLimit (the water's edge
    double shoreNx = 0, shoreNz = 1, shoreLimit = 1e9;   // less the harbour's room)
    uint64_t seed = 0;
};
// the streets and the squares of a town's plan (deterministic from its spec); every street within the radius plus a little, none
// shorter than eight metres
void townStreets(const TownSpec& T, std::vector<Street>& streets, std::vector<Plaza>& plazas);
// the fields round a settlement of an age out to `reach` metres past its edge (plots, strips in furlongs, walled or enclosed fields,
// irrigation circles on flat dry ground, terraces on a slope): what its site says (`SiteRead`)
void townFields(const TownSpec& T, int age, const SiteRead& site, bool desert, std::vector<FieldPatch>& out);

// the geometry the layout and the unit read: the gap between a street's corridor (its half width) and a rectangle round (x, z) of half
// extents hw across the heading and hd along it (negative: they overlap); a point's distance to a street's centre line
double streetRectGap(const Street& s, double x, double z, double heading, double hw, double hd);
double streetPointDist(const Street& s, double x, double z);
// the streets near a point, from a grid of cells (the layout's queries: a building's footprint against the streets, the depth a lot has)
struct StreetGrid {
    double cell = 24, x0 = 0, z0 = 0; int nx = 0, nz = 0;
    std::vector<std::vector<int>> cells;
    void build(const std::vector<Street>& st, double reach);
    template <class F> void near(double xa, double za, double xb, double zb, F fn) const {
        if (nx == 0) return;
        const int i0 = std::max(0, (int)std::floor((std::min(xa, xb) - x0) / cell)), i1 = std::min(nx - 1, (int)std::floor((std::max(xa, xb) - x0) / cell));
        const int j0 = std::max(0, (int)std::floor((std::min(za, zb) - z0) / cell)), j1 = std::min(nz - 1, (int)std::floor((std::max(za, zb) - z0) / cell));
        for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) for (int k : cells[(size_t)j * nx + i]) fn(k);
    }
};
// a building's footprint (its rectangle widened by `margin`) crosses a street's corridor: the first street it crosses, -1 none
int buildingStreet(const RuinSpec& r, const Building& b, double margin);
// the streets joined into one web (segments that meet or cross): the share of their length in the web of the longest
double streetsJoined(const std::vector<Street>& st);
