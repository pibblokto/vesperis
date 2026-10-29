// A landing site: the local tangent frame on a body, height/material caches at
// several levels of detail, and the astronomy of the place (where the sun and
// the other bodies are in the local sky, what time of day it is).
#pragma once
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include "peoples/roads.h"
#include "peoples/works.h"
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

struct TerrainVertex {
    float h = 0;         // metres, relative to reference level
    float albedo = 0.4f;
    float veg = 0;
    uint8_t material = MAT_ROCK;
    uint8_t glow = 0;
    uint8_t biome = 0;
    float water = -1e9f;   // local water surface height (sea, lake, river), M9-04
    float shore = 1e9f;    // signed distance to the water's edge (metres, negative in the water), 1e9 unknown
    uint8_t scree = 0;     // loose rock 0..255 (boulder fields under the cliffs)
    uint8_t road = 0;      // an old road's share of this vertex's footprint (0..255), what the wear left of it
    uint8_t wtint = 0, ltint = 0;   // the water's trait colours here (`SurfaceSample::waterTint`, `lifeTint`), 0..255
    uint8_t gtint = 0;              // the ground trait's field (`SurfaceSample::groundTint`): a strewn field's stones, a stratum's accent
    int32_t cx = 0x7fffffff, cz = 0x7fffffff;   // tags
};

// C-09: an old road of the site in local metres (`SurfaceSite::ensureRoads`): its centre line, the metres along it at each
// point, its half width and its bounds
struct SiteRoad {
    uint64_t id = 0;
    float half = 3;
    bool paved = true; float wear = 0.2f;   // the builders' (a road between two peoples' settlements is the bigger settlement's people's)
    int kind = 1;                            // the network's kind (peoples/plan.h's NetKind: a path, a road, rail, a canal, a highway)
    std::vector<float> x, z, along;
    float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
};

// T-02: a town's ground drawn into the terrain (`SurfaceSite::addTownMarks`, from the settlements the view laid out): a street (a segment and its
// half width, paved or beaten), a square (a disc), a field (a rectangle across and along its heading: its strips' or its plot's tone, its
// edge a hedge, a wall or a bank) or an irrigation circle, in local metres, with what the years left of it
struct GroundMark {
    int kind = 0;            // 0 a street, 1 a square, 2 a field, 3 an irrigation circle
    int sub = 0;             // a street or a square: 2 asphalt (the modern age), 1 paved, 0 beaten; a field: its `FieldKind` (peoples/towns.h)
    double ax = 0, az = 0, bx = 0, bz = 0;   // a street's ends; a square's, a field's or a circle's centre in (ax, az)
    double half = 0, r = 0, hw = 0, hd = 0, heading = 0;
    float left = 1;          // what the years left of it (1 .. 0)
    uint32_t id = 0;
};

// T-03: a work's ground (peoples/works.h's `GroundEdit`) in the site's local metres, with what the years left of it (a ditch silted, a
// crater filled, lines grown over; a rock cut, a flattened plain and a reservoir keep) and its bounds
struct SiteEdit {
    int kind = 0, tone = 0;
    double x = 0, z = 0, heading = 0, hw = 0, hd = 0, r = 0, level = 0, depth = 0, step = 0, width = 0, width2 = -1, bank = 0;
    float left = 1, fresh = 0;   // what the years left of it; how fresh a cut still looks (1 new .. 0 weathered)
    double x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    std::vector<float> pts;
    double gx0 = 0, gz0 = 0, cell = 0; int nx = 0, nz = 0;
    std::vector<uint8_t> mask;
};

// B-322: the water's edge on the mesh. A cell whose corners lie on both sides of the edge is drawn as pieces cut along the
// edge, the cut at the water level: the shoreline is then the contour of the shore distance across the cell (exact for a
// straight bank), not the place where the two triangles happen to cross the plane: a river's bank wall drops metres within a
// cell, so that crossing hugs the bank vertices and makes every shore a stair of cells from the air and a sawtooth of
// triangles on the walk. `shoreValue` is the corner's side of the edge as the mesh reads it;
// `shoreSplitTriangle` cuts one triangle into its wet and dry polygons (fan order from [0], the same in the drawing and in
// `groundHeight`)
struct TerrainCache;
// the corner's side of the edge: its outline distance where known, else its height over its water, else over its neighbours' water; 1e9
// with no water near
double vertexShore(TerrainCache& c, int cx, int cz);
double cacheHeightAt(TerrainCache& c, double x, double z);   // the height of the drawn mesh at a point: the cell's two triangles, cut at the water's edge
struct ShorePt { double x, z, h; int a, b; double t; };   // a corner (a == b, t = 0) or the point at t along the edge a -> b
constexpr double SHORE_LIP = 0.09;   // O6-03: the dry piece's edge stands this much over the cut (the wet piece's lies 3 cm under the water), so the sheet never covers the bank
struct ShoreSplit { ShorePt wet[4], dry[4]; int nWet = 0, nDry = 0; };
void shoreSplitTriangle(const double x[3], const double z[3], const double h[3], const double wl[3], const double sh[3], ShoreSplit& out);

struct SurfaceSite;

struct TerrainCache {
    int cellSize = 16;         // metres
    int logN = 7;              // 128 x 128 torus
    std::vector<TerrainVertex> v;
    const SurfaceSite* site = nullptr;
    void init(const SurfaceSite* s, int cell, int logn);
    // vertex at lattice coords (cx, cz) in cells; computed on demand (F-07: the hit inline, the miss's sampling out of line)
    const TerrainVertex& at(int cx, int cz) {
        TerrainVertex& t = v[index(cx, cz)];
        if (t.cx != cx || t.cz != cz) fill(t, cx, cz);
        return t;
    }
    void fill(TerrainVertex& t, int cx, int cz);
    inline int index(int cx, int cz) const { int n = 1 << logN; return ((cz & (n - 1)) << logN) | (cx & (n - 1)); }
    void invalidate(double x0, double z0, double x1, double z1);   // the vertices in a box sampled again when next asked for
};

struct SunInfo {
    Vec3 dirLocal;      // unit, local ENU (x east, y up, z north)
    double altitude;    // radians, positive above horizon
    double azimuth;     // radians, 0 = north, clockwise
    double lightFactor; // brightness of the star at this distance
    double angularRadius;
    double dayFraction; // 0..1 local time of day (0.5 = noon)
    double eclipse = 0; // fraction of the sun's disc covered by another body (M1-08)
    double ringShadow = 0;   // density of the world's own ring between the site and the sun (0 = clear)
    RGB color;          // light colour (M5-01: differs for the companion sun)
};

struct SurfaceSite {
    const StarSystem* sys = nullptr;
    int body = -1;
    BodyGen gen;
    double lat0 = 0, lon0 = 0;    // radians
    double R = 6000e3;            // metres
    Vec3 up0, east0, north0;      // body-frame basis at the site
    bool atmosphere = false;
    bool hasWater = false;        // sea level exists (felisian)
    double seaLevel = 0;          // metres
    double lavaLevel = -1e9;
    double gravity = 9.8;
    TerrainCache lod0, lod1, lod2, lod3;   // 16, 64, 512 and (O1) 2048 m cells
    TerrainCache lodN;                     // the near ring, 4 m cells (a 256 x 256 torus, one kilometre across)
    // the near ring is drawn within nearR cells of (nearX, nearZ) while nearBlend is 1 (0: not drawn); B-313: its
    // outer nearBand cells morph onto lod0 (SurfaceView::vertexOf), and groundHeight follows the same blend so the feet,
    // the wheels, the rocks and the trees sit on the ground that is drawn. The view sets these every frame before the
    // movement update.
    double nearBlend = 0, nearX = 0, nearZ = 0; int nearR = 0, nearBand = 0;
    // lod0's own morph onto lod1 (B-313) across its outer band, relative to the camera: `groundHeight` follows it,
    // so a tree, a rock or an animal 200-600 m off stands on the ground that is drawn and not a few metres above or below
    // it (else they sink and surface as the band sweeps over them, far trees flickering in and out of the ground)
    double camX = 0, camZ = 0; int lod0R = 0, lod0Band = 0;
    bool inNearRing(double x, double z) const { double dx = (x - nearX) / 4.0, dz = (z - nearZ) / 4.0; return nearBlend > 0 && dx * dx + dz * dz < (double)nearR * nearR; }
    // 0 at the core .. 1 at the edge
    double nearMorph(double x, double z) const { double dx = (x - nearX) / 4.0, dz = (z - nearZ) / 4.0; return smoothstep(nearR - nearBand, nearR - 0.5, std::sqrt(dx * dx + dz * dz)); }
    bool smallBody = false;      // R under 300 km (comets): exact sphere geometry, no far ring, no floor
    double escapeVelocity = 1e9; // m/s, sqrt(2 g R)
    std::vector<float> ringProf; // the ring's density profile when the body has rings (else empty)
    // the old roads within the near rings (`peoples/roads.*`), in local metres, with the culture's paving and wear. The
    // view builds them after `init` (and again at a reanchor); a probe site does not, so its vertices carry no road.
    // `sampleAt` draws them into the vertices: the road's share of the footprint (`TerrainVertex::road`), the paving as the
    // material on the fine rings, the ground bare and darker, the tone alone on the coarse ones
    std::vector<SiteRoad> roads;
    double roadsMs = 0;
    std::unordered_set<uint64_t> roadIds, roadCells;   // the roads walked and the cells whose pairs were listed
    RoadNodeCache roadNodes;
    Culture roadCultures[2];                           // the world's peoples' (one, or two), each road built as its people built
    void clearRoads();
    // the roads of the cells within ROAD_BLOCK cells of a point, added to what is built (the view calls it at `init`, at a
    // reanchor and when the explorer has moved two cells from the last call, with its worker joined: the vertices read the
    // list on the render threads and nothing may grow it under them)
    static constexpr int ROAD_BLOCK = 5;
    void ensureRoads(double x, double z);
    // the nearest road within `within` metres of a point: its distance, the metres along it, its heading there (radians
    // from north, either way) and the road; false when none
    bool roadAt(double x, double z, double within, double& dist, double& along, double& heading, const SiteRoad*& road) const;
    // 0..1: the road's bed at a point (within its half width, what the wear left; fading over a metre beyond), for the
    // flora, the rocks and the explorer's feet
    double roadCover(double x, double z) const;
    // the towns' ground (`GroundMark`), found by 32 m cells; `addTownMarks` adds a settlement's (its streets, squares and fields at its
    // centre's local metres, the years since it was left and the world's wet fading them) and makes the fine rings' vertices under it be
    // sampled again; the view adds them with its worker joined (the vertices read the list), and clears them with the roads
    std::vector<GroundMark> marks;
    std::unordered_map<int64_t, std::vector<int>> markCells;
    void clearMarks() { marks.clear(); markCells.clear(); }
    void addTownMarks(const RuinSpec& spec, double ox, double oz, double years, double wet, double wear);
    double markCover(double x, double z) const;   // 0..1: a street's or a square's bed at a point (the flora and the rocks keep off it)
    void applyMarks(double x, double z, double detailM, TerrainVertex& t) const;
    // the works' ground (`SiteEdit`): a quarry's cut and a pit, ditches, craters, a plain made flat, a reservoir's water, lines, a
    // mountain's terraces; the small ones found by 64 m cells, the great ones (over 600 m) by their bounds. `addWorkEdits` adds a work's (its
    // middle at local (ox, oz), the years since it was left and the world's wet fading them) and makes every ring's vertices under it be
    // sampled again; the view adds them with its worker joined, and clears them with the roads
    std::vector<SiteEdit> edits;
    std::unordered_map<int64_t, std::vector<int>> editCells;
    std::vector<int> bigEdits;
    void clearEdits() { edits.clear(); editCells.clear(); bigEdits.clear(); }
    void addWorkEdits(const Work& w, double ox, double oz, double years, double wet);
    void applyEdits(double x, double z, double detailM, TerrainVertex& t) const;
    double editCover(double x, double z) const;   // 0..1: a fresh cut's floor, a line's bed, a plain made flat (the flora and the rocks keep off it)

    void init(const StarSystem* s, int bodyIndex, double lat, double lon, double t = 0);   // t: the season
    // `init` without the five terrain caches (106,496 vertices): a site that is sampled (`sampleAt`), never drawn, such as the
    // landing map's probe of its cursor; drawing it would index empty caches
    void initFrame(const StarSystem* s, int bodyIndex, double lat, double lon, double t = 0);
    double season = 0;
    // local metres -> body-frame unit vector (azimuthal equidistant mapping)
    Vec3 unitAt(double x, double z) const;
    void latLonAt(double x, double z, double& lat, double& lon) const;
    // inverse of unitAt: local metres of a body-frame unit vector
    void localAt(const Vec3& unit, double& x, double& z) const;
    // local ENU frame at time t (rows east, up, north) mapping world -> local
    Mat3 localFrame(double t) const;
    Vec3 worldPos(double t, double x, double z, double alt) const;
    SunInfo sun(double t) const;
    bool sun2(double t, SunInfo& out) const;   // the companion star as a second sun; false for single stars
    // ground height (metres): the near ring's cells where it is drawn (O6-02), else bilinear on lod0
    double groundHeight(double x, double z);
    double groundHeightCoarse(double x, double z);   // lod0 alone (what the near ring blends from)
    // TR_GREAT_TIDES: the sea's offset from its mean level now (the view sets it each frame: `tideOffsetM`). The water as it stands now
    // adds it to the sea's vertices (`waterOf`, `waterNowAt`: the drawing, the shore's cut, the explorer, the vehicles, the rangefinder); what is
    // placed on the ground reads the mean (`waterAt`: a tree or a stone stays where it is whatever the tide)
    double tideM = 0;
    bool seaVertex(const TerrainVertex& t) const { return hasWater && t.water > -1e8f && std::fabs(t.water - (float)seaLevel) < 0.01f && t.shore >= 1e8f; }
    double waterOf(const TerrainVertex& t) const { return t.water > -1e8f ? (double)t.water + (tideM != 0 && seaVertex(t) ? tideM : 0.0) : -1e9; }
    double waterNowAt(double x, double z);
    // local water surface (sea, lake or river) at x, z; -1e9 where there is none (bilinear on lod0)
    double waterAt(double x, double z);
    // ground or water, whichever is higher
    double surfaceHeight(double x, double z) { return std::max(groundHeight(x, z), waterAt(x, z)); }
    TerrainVertex sampleAt(double x, double z, double detailM) const;
};
