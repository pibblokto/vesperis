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
    ~SpaceRenderer();
    // M9-09: maps and generators follow the season; call before rendering a system at time t
    void setSeason(const StarSystem& sys, double t);
private:
    double curSeason[64] = {0};
    int seasonBucket[64] = {0};
public:
    // Draw all bodies of the system into a sky (from a surface), excluding one.
    void drawSkyBodies(Framebuffer& fb, const SpaceContext& c);
    // Sun disc and glow shared with the surface view, drawn by angle (B-308): `dirView` is the unit direction to the
    // star in view space, `angR` its angular radius (may be tiny: under 0.8 px it is a point).
    static void drawSun(Framebuffer& fb, const Star& star, const Vec3& dirView, double angR, double t,
                        double intensity, int bank, bool atmosphere, double atmosHaze, bool depthTest, const Proj& pj);
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
};
