// A landing site: the local tangent frame on a body, height/material caches at
// several levels of detail, and the astronomy of the place (where the sun and
// the other bodies are in the local sky, what time of day it is).
#pragma once
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include <vector>
#include <algorithm>

struct TerrainVertex {
    float h = 0;         // metres, relative to reference level
    float albedo = 0.4f;
    float veg = 0;
    uint8_t material = MAT_ROCK;
    uint8_t glow = 0;
    uint8_t biome = 0;
    float water = -1e9f;   // local water surface height (sea, lake, river), M9-04
    float shore = 1e9f;    // B-322: signed distance to the water's edge (metres, negative in the water), 1e9 unknown
    uint8_t scree = 0;     // O6-04: loose rock 0..255 (boulder fields under the cliffs)
    int32_t cx = 0x7fffffff, cz = 0x7fffffff;   // tags
};

// B-322: the water's edge on the mesh. A cell whose corners lie on both sides of the edge is drawn as pieces cut along the
// edge, the cut at the water level: the shoreline is then the contour of the shore distance across the cell (exact for a
// straight bank), not the place where the two triangles happen to cross the plane. It used to be the latter: a river's bank
// wall drops metres within a cell, so the crossing hugged the bank vertices and every shore was a stair of cells from the
// air (KI-321) and a sawtooth of triangles on the walk. `shoreValue` is the corner's side of the edge as the mesh reads it;
// `shoreSplitTriangle` cuts one triangle into its wet and dry polygons (fan order from [0], the same in the drawing and in
// `groundHeight`)
struct TerrainCache;
double vertexShore(TerrainCache& c, int cx, int cz);   // the corner's side of the edge: its outline distance where known, else its height over its water, else over its neighbours' water; 1e9 with no water near
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
    // vertex at lattice coords (cx, cz) in cells; computed on demand
    const TerrainVertex& at(int cx, int cz);
    inline int index(int cx, int cz) const { int n = 1 << logN; return ((cz & (n - 1)) << logN) | (cx & (n - 1)); }
};

struct SunInfo {
    Vec3 dirLocal;      // unit, local ENU (x east, y up, z north)
    double altitude;    // radians, positive above horizon
    double azimuth;     // radians, 0 = north, clockwise
    double lightFactor; // brightness of the star at this distance
    double angularRadius;
    double dayFraction; // 0..1 local time of day (0.5 = noon)
    double eclipse = 0; // fraction of the sun's disc covered by another body (M1-08)
    double ringShadow = 0;   // O0-01: density of the world's own ring between the site and the sun (0 = clear)
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
    TerrainCache lodN;                     // O6-02: the near ring, 4 m cells (a 256 x 256 torus, one kilometre across)
    // O6-02: the near ring is drawn within nearR cells of (nearX, nearZ) while nearBlend is 1 (0: not drawn); B-313: its
    // outer nearBand cells morph onto lod0 (SurfaceView::vertexOf), and groundHeight follows the same blend so the feet,
    // the wheels, the rocks and the trees sit on the ground that is drawn. The view sets these every frame before the
    // movement update.
    double nearBlend = 0, nearX = 0, nearZ = 0; int nearR = 0, nearBand = 0;
    // B-319: lod0's own morph onto lod1 (B-313) across its outer band, relative to the camera: `groundHeight` follows it,
    // so a tree, a rock or an animal 200-600 m off stands on the ground that is drawn and not a few metres above or below
    // it (they used to sink and surface as the band swept over them, and far trees flickered in and out of the ground)
    double camX = 0, camZ = 0; int lod0R = 0, lod0Band = 0;
    bool inNearRing(double x, double z) const { double dx = (x - nearX) / 4.0, dz = (z - nearZ) / 4.0; return nearBlend > 0 && dx * dx + dz * dz < (double)nearR * nearR; }
    double nearMorph(double x, double z) const { double dx = (x - nearX) / 4.0, dz = (z - nearZ) / 4.0; return smoothstep(nearR - nearBand, nearR - 0.5, std::sqrt(dx * dx + dz * dz)); }   // 0 at the core .. 1 at the edge
    bool smallBody = false;      // O4: R under 300 km (comets): exact sphere geometry, no far ring, no floor
    double escapeVelocity = 1e9; // m/s, sqrt(2 g R)
    std::vector<float> ringProf; // O0-01: the ring's density profile when the body has rings (else empty)

    void init(const StarSystem* s, int bodyIndex, double lat, double lon, double t = 0);   // t: the season (M9-09)
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
    bool sun2(double t, SunInfo& out) const;   // M5-01: the companion star as a second sun; false for single stars
    // ground height (metres): the near ring's cells where it is drawn (O6-02), else bilinear on lod0
    double groundHeight(double x, double z);
    double groundHeightCoarse(double x, double z);   // lod0 alone (what the near ring blends from)
    double waterHeight() const { return hasWater ? seaLevel : -1e9; }   // the sea
    // local water surface (sea, lake or river) at x, z; -1e9 where there is none (bilinear on lod0)
    double waterAt(double x, double z);
    // ground or water, whichever is higher
    double surfaceHeight(double x, double z) { return std::max(groundHeight(x, z), waterAt(x, z)); }
    TerrainVertex sampleAt(double x, double z, double detailM) const;
};
