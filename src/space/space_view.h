// Rendering of space: the hashed star field, the local sun, planets and moons
// as ray-traced globes textured from their planet maps, rings, and the Vimana
// flight streaks.
#pragma once
#include "core/framebuffer.h"
#include "core/raster.h"
#include "galaxy/starfield.h"
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

struct BodyScreenInfo {
    int body = -1;
    double sx = 0, sy = 0, radiusPx = 0;
    double distKm = 0;
    bool inFront = false;
};

// Draw the neighbourhood stars as points. `observer` in km, `cam` world->view.
// intensityScale dims them (daylight); whiteBank is the ramp for ordinary stars; coloured=true
// uses banks 4/5 for blue/red stars; skyMask (with a threshold in shade units) hides stars
// where the sky is bright.
void drawStarField(Framebuffer& fb, const std::vector<Star>& stars, const Vec3& observer, const Mat3& cam,
                   const Proj& pj, double intensityScale, int whiteBank, bool coloured, const Pix* skyMask,
                   double skyMaskThreshold, double streak, const Vec3& streakDirView, double twinkle = 0, double time = 0);

struct SpaceContext {
    const StarSystem* sys = nullptr;
    const std::vector<Star>* stars = nullptr;
    double t = 0;             // game time, seconds
    Vec3 shipPos;
    Mat3 cam;                 // world->view
    bool vimana = false;
    Vec3 vimanaDirView;       // direction of travel in view space
    double vimanaSpeed = 0;   // 0..1
    int bankBodyA = -1, bankBodyB = -1;
    double pulsePhase = 0;
    int excludeBody = -1;     // body the observer stands on
    bool skyMode = false;     // draw into a sky: bank 1, keep brighter existing pixels
    double starIntensity = 1; // field amplification (targeting mode)
    double skyDark = 1;       // sky mode: 1 at night, 0 by day (comet tails and wandering stars fade in daylight)
    // W-06 (the user's review): the body the telescope's plate is built for (-1 none) and the samples a frame the
    // build may take (0: the whole plate now, for a photo or a harness frame)
    int detailBody = -1;
    int detailBudget = 8000;
};

// W-06 (the user's review, 2026-10-06): the telescope's plate. The world map (512 x 256 texels, 11-44 km each) is the
// planet function sampled coarsely; at the eyepiece's powers a texel spans pixels and the globe blurs. The plate is the
// same function sampled again at the eyepiece's own footprint: a body-fixed patch round the aim point, an orthographic
// grid in the body's frame (`u0` the centre, `e1`/`e2` the axes, `texel` the step in radii: a = dot(p, e1), b = dot(p, e2),
// the point at sqrt(1 - a^2 - b^2) u0 + a e1 + b e2), each cell the planet function at `texelM` metres of detail (the
// height, the material, the albedo, the storms, the cloud pattern at the drift of the build). Built progressively in
// blocks of 8, 4, 2 and 1 texels (a block's origin stands for the block until its cells are sampled themselves) within a
// budget of samples a frame, so the picture forms at once and sharpens over a second; `drawGlobe` reads it in place of
// the map wherever a cell is filled, the map standing in elsewhere; the previous plate is kept as the fallback while a
// new one builds (a zoom step, the aim leaving the plate, the rivers' tiles arriving). Body-fixed, it survives the body's
// turn and the ship's orbit: the lookup turns the point, not the plate.
struct DetailPlate {
    int body = -1; uint64_t seed = 0; int seasonBucket = -1;
    Vec3 u0, e1, e2;
    double texel = 0, texelM = 0;
    int W = 0, H = 0;
    std::vector<float> height;
    std::vector<uint8_t> material, albedo, cloud, veg, level, exact;   // level: the block size a cell's values came from (0 unfilled); exact: the cell holds its own sample
    double cloudDrift0 = 0;    // the cloud drift (radians of longitude) the cells were sampled with
    int pass = 0, next = 0;    // the block size pass (0..3: 8, 4, 2, 1; 4 done) and the next block of it
    bool noDrainage = false;   // sampled without the rivers and lakes (their tiles were not built yet)
    bool drainageWanted = false;
    double radiusM = 0;        // the ground radius the plate covers (what the drainage prefetch is asked for)
    long samples = 0;
    static constexpr int BLOCKS[4] = {8, 4, 2, 1};
    bool valid() const { return W > 0; }
    bool done() const { return pass >= 4; }
    double progress() const;   // 0..1 by the samples' share
};

class SpaceRenderer {
public:
    static int dbgX, dbgY;   // debug: print shading terms for this pixel
    Proj proj = Proj::fromHFov(70);
    std::vector<BodyScreenInfo> bodyInfo;
    void render(Framebuffer& fb, const SpaceContext& c);
    // M5-07/N0-01: a comet's coma, its ion tail with streaming knots, its curved dust tail and, when the
    // nucleus is a disc of `nucleusRpx` pixels, jets from its sunlit side (the nucleus is drawn by drawGlobe)
    void drawCometTail(Framebuffer& fb, const SpaceContext& c, int bi, int bank, double nucleusRpx = 0);
    // S-05: a gas giant (or a substellar object) of a Wolf-Rayet star, its envelope streaming away from the star in the wind
    void drawStrippedTail(Framebuffer& fb, const SpaceContext& c, int bi, int bank);
    // S-06: a black hole's companion being drawn out: its gas streams from the limb facing the hole into the disc
    void drawAccretionStream(Framebuffer& fb, const SpaceContext& c, int bi, int bank);
    // S-06: the black hole in place of a sun: the shadow, the lensed sky behind it, the accretion disc and the jet
    static void drawBlackHole(Framebuffer& fb, const Star& star, const Vec3& dirView, double angR, double t, double intensity,
                              int bank, bool depthTest, const Proj& pj, const Vec3* discUp);
    // N0-01: position (0..1 along the ion tail) of knot k at time t; one traversal every 40 s
    static double cometKnot(int k, double t);
    // Palette banks: 0 = stars (white), 1 = sun, 2 = body A (rock family), 3 = body B (rock), 6/7 = their water,
    // body A's forest 11, sand 13, snow 14, grass 15; body B's forest 16, sand 17, snow 18, grass 19 (O0-02, B-305:
    // each world with a disc has its own six family banks since the pixel carries 32 banks); 12 = the companion
    // star (M5-01); 20 = belt rocks (O3); 8-10 belong to the cabin.
    void setupPalette(Framebuffer& fb, const StarSystem* sys, int bodyA, int bodyB, double fade);
    // N1-02: the bank a globe pixel of this material goes to, given the body's rock bank (2 or 3; 0 = a far grey body)
    static int globeBank(int bodyBank, int material);
    // the ramp of a material colour, shared by the globe and the landing map: {0 black, 14 0.25c, 34 0.6c, 47 c, 56 c->white, 63 white}
    static void materialRamp(uint8_t* pal, int bank, RGB c, const RGB* skyLight);
    const PlanetMap& mapFor(const Body& b);          // synchronous: waits for a background generation
    const PlanetMap* mapIfReady(const Body& b);      // starts a background generation, nullptr until ready (M9-15)
    const BodyGen& genFor(const Body& b);
    // W-03: the fronts' cloud at t on the map's grid (`buildFrontMap`), for the globe and the landing map; nullptr for a world
    // without fronts; built again when the clock has moved four minutes (a front moves a texel in an hour). `coarse` flags the
    // 16 x 16 texel blocks that hold any band (a texel's neighbours counted), so a globe pixel off every band costs one byte
    struct FrontMap { uint64_t seed = 0; double t = -1e18; std::vector<uint8_t> cloud, coarse; };
    static constexpr int FRONT_BLOCK = 16;
    const FrontMap* frontsFor(const Body& b, double t);
    ~SpaceRenderer();
    // M9-09: maps and generators follow the season; call before rendering a system at time t
    void setSeason(const StarSystem& sys, double t);
private:
    FrontMap frontMaps[4]; int frontMapNext = 0;
    static void frontCoarse(FrontMap& fm);
    double curSeason[64] = {0};
    int seasonBucket[64] = {0};
public:
    // Draw all bodies of the system into a sky (from a surface), excluding one.
    void drawSkyBodies(Framebuffer& fb, const SpaceContext& c);
    // Sun disc and glow shared with the surface view, drawn by angle (B-308): `dirView` is the unit direction to the
    // star in view space, `angR` its angular radius (may be tiny: under 0.8 px it is a point).
    // S-04: `discUp`, the orbital plane's normal in view space, lets a protostar draw its dust disc as a band along the plane
    static void drawSun(Framebuffer& fb, const Star& star, const Vec3& dirView, double angR, double t,
                        double intensity, int bank, bool atmosphere, double atmosHaze, bool depthTest, const Proj& pj, const Vec3* discUp = nullptr);
    static double lightFactor(double luminosity, double distKm);
    // M1-03: flare discs along the line from the sun through the screen centre.
    static void drawLensFlare(Framebuffer& fb, double sx, double sy, double sunRadiusPx, double intensity, int bank, const Proj& pj);
private:
    std::map<uint64_t, std::unique_ptr<PlanetMap>> maps;
    std::map<uint64_t, std::unique_ptr<std::thread>> mapThreads;
    std::map<uint64_t, uint64_t> mapLastUse;   // M7-02 LRU stamps
    uint64_t useStamp = 0;
public:
    // M7-02: cache policy and profiling counters (printed by `vesperis_test bench`)
    int mapCacheCap = 24;
    struct MapStats { int generated = 0, hits = 0, evicted = 0; double genMs = 0; } mapStats;
    std::atomic<long> genMicros{0};
    size_t mapsHeld() const { return maps.size(); }
private:
    std::map<uint64_t, BodyGen> gens;
    std::map<uint64_t, std::vector<float>> ringProfiles;
    std::vector<float> bandMap;   // M1-09 galactic backdrop for the current neighbourhood
    Vec3 bandPos; bool bandValid = false;
public:
    const std::vector<float>& ringProfile(const StarSystem& sys, int bi);   // M5-04: the surface draws ring arcs with it
private:
    void startMap(const Body& b, int bucket);
    void joinMap(uint64_t seed);
    void drawGlobe(Framebuffer& fb, const SpaceContext& c, int bi, int bank, BodyScreenInfo& info);
    void drawBeltRocks(Framebuffer& fb, const SpaceContext& c, int k);   // O3 (R-302): the rocks around a ship inside belt k (bank 20)
public:
    int lastBeltRocks = 0, lastBeltMeshes = 0;   // bench: rocks drawn last frame (points and meshes)
    // W-06: the telescope's plates: the current one and the previous (the fallback while the current builds)
    DetailPlate plates[2];
    int plateCur = 0;
    long plateSamplesFrame = 0;                 // samples taken by the last render
    long plateBegun = 0;                        // plates begun since the start (the harness reads it: the stabiliser begins none while it holds)
    void updatePlate(const SpaceContext& c);    // called by `render` before the globes: keeps, restarts or fills the plate
    const DetailPlate& plate() const { return plates[plateCur]; }
    double plateProgress() const { return plates[plateCur].valid() ? plates[plateCur].progress() : 0; }
    std::string plateInfo() const;
    // the plate's cell centre as a body-frame unit vector (the harness recomputes the function there)
    static Vec3 plateCellUnit(const DetailPlate& p, int i, int j);
private:
    void plateBegin(DetailPlate& p, int bi, uint64_t seed, int bucket, const Vec3& u0, const Vec3& e1, const Vec3& e2, double texel, double R, double cloudDrift, double halfA, double halfB);
    void plateFill(DetailPlate& p, const BodyGen& g, int budget);
};
