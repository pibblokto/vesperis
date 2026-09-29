// Game orchestration: states, the Stardrifter's autopilot, HUD and menus.
#pragma once
#include "core/framebuffer.h"
#include "core/font.h"
#include "core/input.h"
#include "galaxy/starfield.h"
#include "galaxy/system.h"
#include "space/space_view.h"
#include "surface/surface_view.h"
#include "audio.h"
#include "settings.h"
#include "guide.h"
#include <string>
#include <vector>
#include <cmath>

enum class GameState { TITLE, SPACE, LANDING_MAP, DESCENT, SURFACE, ASCENT, HELP, MENU, SYSTEM_LIST, DATA, SETTINGS, SLOTS,
                       GUIDE, TEXT_ENTRY, STAR_MAP, LOG, STATS, GALLERY, SHIPSCREEN, CONSOLE, SECTOR_MAP, KEYS };

// M2: the explorer inside the Stardrifter
struct Cabin {
    double x = 0.4, z = -1.6, yaw = 0, pitch = 0;   // ship-local metres; yaw 0 looks out of the front window
    bool light = true, depolarised = false, onRoof = false;
    double lightLevel = 1;
    int facing = 0;   // what E would use
};

struct ShipState {
    Vec3 pos;
    double yaw = 0, pitch = 0;
    enum Mode { STANDBY, VIMANA, APPROACH, PARKED };
    Mode mode = STANDBY;
    bool hasRemote = false;
    Star remote;
    int localTarget = -1;
    int parkedBody = -1;
    Vec3 parkDir;             // unit vector from body centre to the ship (world)
    double parkDist = 0;      // km
    bool orbiting = true;
    Vec3 flightFrom, flightTo;
    double flightT = 0, flightDur = 1;
    bool targeting = false;   // remote crosshair mode
    double aimYaw0 = 0, aimPitch0 = 0;   // orientation at approach start
    // O3 (R-302): a belt as the local target or the parking place. The ship parks beside one rock of the belt, named
    // by its co-rotating cell and index so the same rock is there again after a load, six rock radii off on its sunlit side
    int targetBelt = -1, parkedBelt = -1;
    int64_t rockIr = 0, rockIa = 0, rockIy = 0; int rockM = 0;
    double rockDist = 0;      // km from the rock's centre
};

class Game {
public:
    Game();
    void frame(const Input& in, double realDt);
    const uint32_t* output() const { return rgbBuf.data(); }
    bool wantsQuit = false;
    bool wantsScreenshot = false;
    bool wantsFullscreenToggle = false;
    // M6-05 photo tools: photo mode (HUD off, free camera on the surface), a three-view panorama, a frame recorder
    bool photoMode = false;
    bool wantsPanorama = false;
    bool recording = false;
    int recTake = 0, recFrame = 0;
    std::string moviesDir = "movies";
    bool recFixedTake = false;        // tests: always take 1, frames overwritten
    void renderPanorama(std::vector<uint32_t>& out, int& w, int& h);   // three views side by side, no HUD
    bool recordFrame();                                              // writes movies/take_NNN/frame_NNNNN.png
    void togglePhotoMode();
    void toggleRecording();
    // M6-06 attract mode: after 20 s idle on the title the camera tours the bodies of the system
    double titleIdle = 0, attractT = 0;
    bool attractActive() const { return titleIdle > 20 && sys.valid && !sys.bodies.empty(); }
    void testAttract(double idleSecs) { titleIdle = idleSecs; }
    // M7-02 / M7-01 counters for the benchmark
    const SpaceRenderer::MapStats& testMapStats() const { return spaceR.mapStats; }
    size_t testMapsHeld() const { return spaceR.mapsHeld(); }
    long testGenMicros() const { return spaceR.genMicros.load(); }
    int testAheadCells() const { return surf.testAheadCells(); }
    bool macHints = false;        // set by the platform layer on macOS: mention Fn for the F keys once
    Settings settings;
    // N5-05 key bindings screen: the platform layer hands the key map and the raw (unmapped) key pressed this frame
    KeyMap* keymap = nullptr;
    KeyMap ownKeymap;
    int rawKey = -1;
    std::string keysPath = "vesperis_keys.txt";
    std::string settingsPath = "vesperis_settings.txt";
    Guide guide;                  // M3: names, log, statistics (guide.txt)
    std::string guidePath = "guide.txt";
    std::string shotsDir = "shots";
    std::string screenshotCaption() const;   // for the sidecar text next to a screenshot
    void noteScreenshot() { guide.screenshots++; guide.save(guidePath); }
    std::string starNameOf(const Star& s) const;   // R-401: the explorer's (or the inbox's) name, else "UNKNOWN"
    std::string bodyNameOf(int bi) const;
    bool starNamed(const Star& s) const;
    bool bodyNamed(int bi) const;
    std::string beltNameOf(int k) const;
    std::string bodyLabelOf(int i) const;          // "<name> (<type>, moon)": the local target's line
    std::string starKeyOf(const Star& s) const;
    std::string landmarkKey(const Landmark& L) const;          // O6-06: "<body key>/L<id>" on the surface's body
    std::string landmarkName(const Landmark& L) const;         // the explorer's name for it, else empty (R-401)
    std::string landmarkLabel(const Landmark& L) const;        // "<name> - PEAK" or "UNNAMED PEAK"
    void noteLandmarks();                                      // the first sight within a kilometre goes to the log
    std::vector<Landmark> zoomLandmarks;                       // the landing zoom's sights (built with the zoom)
    int testLandmarksHere() const { return (int)surf.landmarks.size(); }
    bool nameIsForeign(const std::string& key) const;
    int wantsWindowScale = 0;     // >0: the platform layer resizes the window to 320*s x 200*s
    void loadFromDisk();          // settings and save file (the constructor touches no files)
    void applySettings();
    bool mouseCaptureWanted() const;
    AudioState audio;
    // Saves: three slots plus an autosave, all named from `savePrefix` (M0-04).
    std::string savePrefix = "vesperis_save";
    int currentSlot = 1;
    std::string slotPath(int n) const { return n == 0 ? savePrefix + "_auto.txt" : savePrefix + "_" + std::to_string(n) + ".txt"; }
    bool save(const std::string& path);
    bool saveSlot(int n);
    bool autosave();
    bool load(const std::string& path);
    bool loadSlot(int n);
    bool loadNewest();
    std::string saveSummary(const std::string& path) const;   // one line for the slot picker, "" if absent
    void newGame();
    GameState state = GameState::TITLE;
    double gameTime() const { return t; }
    // for the headless harness
    void setState(GameState s) { state = s; }
    void testAimAtNearestStar();
    void testAimAtBody(int body);
    int testLandableBody() const;
    void testParkAt(const Star& s, int body);   // arrive at a star, park at a body, open the landing map
    void testParkAtBelt(const Star& s, int k);  // O3: arrive at a star and park beside a rock of belt k at once
    std::string cometMotionString(int bi) const;   // N0-01: "740 KM/S, PERIAPSIS IN 3.2 H"
    static const char* shortPlan(int plan);        // N3: "QUADRUPED", "BROWSER", ...
    bool testGotoHerd(double dist) { return surf.testGotoHerd(dist); }
    double testBuggySpeed() const { return std::fabs(surf.buggy.speed); }
    bool testBuggyAirborne() const { return surf.buggy.airborne; }
    const double* testRenderMs() const { return surf.lastRenderMs; }   // B-310: the surface render's section times
    bool testDriveSetup();   // N4: deploy the buggy beside the capsule, get in (for the bench)
    double testDriveOpen();  // N4: move the buggy (with the explorer in it) to open ground within 800 m, pointed down the longest clear run; returns the run in metres
    std::string testHerdStates() const { return surf.testHerdStates(); }
    uint32_t testLandingMapTexel(int tx, int ty, bool clouds);   // N0-02: base colour of the parked body's landing map (full daylight)
    double testMapShadowMax();                                    // O0-04: the darkest shadow on the landing map now (0..1)
    void testSetTime(double tt) { t = tt; }
    const char* testReliefClass(double lat, double lon);         // N1-06: FLAT / HILLS / MOUNTAINS at a site of the parked body
    static double siteSlope(const SurfaceSite& probe);           // N1-06: steepest rise over run around a site
    static double parkDistanceFor(const Body& b);  // 3.5 radii; outside the rings; 12 radii for a comet (N0-01)
    std::string testStatus() const { return statusMsg; }
    std::string testDebugInfo() const;
    std::string testHealth() const;   // "" when every number is finite
    double testPlayerStamina() const { return surf.player.stamina; }
    double testPlayerSpeed() const { return std::sqrt(surf.player.vx * surf.player.vx + surf.player.vz * surf.player.vz); }
    double testPlayerAlt() const { return surf.player.altAboveGround; }
    void testWalkToBuggy() { if (surf.buggy.deployed) { surf.player.x = surf.buggy.x + 1.5; surf.player.z = surf.buggy.z; } }
    std::string testBuggyInfo() const;
    std::string testRangeInfo() const { return "range=" + std::to_string((int)surf.lastRange) + " water=" + std::to_string((int)surf.lastRangeWater); }   // O1
    int testParkedBelt() const { return ship.parkedBelt; }                                                                  // O3
    std::string testBeltDraw() const;   // O3: rocks drawn last frame and the bank-20 pixels in the framebuffer
    std::string testMarksInfo() const { return "trail=" + std::to_string(surf.trail.size()) + " waypoint=" + std::to_string((int)surf.hasWaypoint) + " zoom=" + std::to_string(sectorZoom) + "/" + std::to_string(sectorMaxLevel()); }   // B-303, O1-04
    double testEscapeVelocity() const { return surf.site.escapeVelocity; }                                                  // O4
    void testTypeText(const std::string& s);   // feeds text entry as key presses
    std::string testStarName() const { return sys.valid ? starNameOf(sys.star) : ""; }
    std::string testStarGenName() const { return sys.valid ? sys.star.name : ""; }   // the generator's name (never shown, R-401)
    bool testDrainageReady() const;   // O6-03: the drainage tiles of the landing site are in the cache
    int testBodyType(int bi) const { return bi >= 0 && bi < (int)sys.bodies.size() ? sys.bodies[bi].type : -1; }
    std::string testBodyTypes() const { std::string r; for (auto& b : sys.bodies) r += std::to_string(b.index) + ":" + PLANET_TYPES[b.type].name + (b.parent >= 0 ? "(moon) " : " "); return r; }
    void testCabinGoto(double x, double z, double yaw) { cabin.x = x; cabin.z = z; cabin.yaw = yaw; cabin.pitch = 0; }
    int testCabinFacing() const { return cabin.facing; }
    int testLogEntries() const { return (int)guide.log.size(); }
    Framebuffer fb;

private:
    std::vector<uint32_t> rgbBuf;
    RGBCanvas canvas;
    GameState returnState = GameState::SPACE;
    double t = 3.6e6;
    double timeWarp = 1;
    double realTime = 0;
    StarNeighborhood nb;
    StarSystem sys;
    SpaceRenderer spaceR;
    SurfaceView surf;
    ShipState ship;
    bool hasSave = false;
    double landLat = 0, landLon = 0;   // radians
    double drainLat = 1e9, drainLon = 1e9;   // O6-03: the site whose drainage tiles were last asked for ahead of the landing
    int landBody = -1;
    // O2 (R-301): the landing map's zoom into the cursor's sector (Z): a window two sectors wide and one tall, sampled
    // from the planet function at the far ring's detail, with the sector grid drawn over it
    int landZoom = 0;
    static constexpr int ZW = 256, ZH = 128;
    std::vector<uint32_t> zoomImg; std::vector<uint8_t> zoomEmissive; std::vector<Vec3> zoomUnit;
    double zoomLat = 1e9, zoomLon = 1e9, zoomHalfLat = 0, zoomHalfLon = 0;
    bool zoomDry = false; double zoomPrefetchT = -1e9;   // O6-03: the zoom was drawn without the drainage (its tiles were not built); rebuilt when they are
    uint64_t zoomSeed = 0;
    void buildLandingZoom();
    bool landingZoomAllowed() const;
    // O0-04 (B-306): the shadows of moons, the parent and the rings on the landing map and its zoom, as the globe shows them
    std::vector<uint8_t> mapShadow, zoomShadow;
    int mapShadowBody = -1; double mapShadowT = -1e18, mapShadowReal = -1e18;
    void buildShadowMap(const std::vector<Vec3>& units, std::vector<uint8_t>& out);
    void refreshMapShadows(const std::vector<Vec3>& unitLUT);
    int lastSecX = -1, lastSecY = -1;   // the sector the explorer was in last frame (crossings are announced)
    // the one colouring of every map of a world (the landing map, its zoom, the sector map): the body's material ramps
    // under the star's light, the ground's exposure rule and a hillshade
    void mapRampsFor(const Body& b, MatRamp* ramps, double& lf, bool& atmo);
    static uint32_t mapColor(const MatRamp* ramps, int mat, double albedo, double lf, bool atmo, double shade);
    double transT = 0;
    double fade = 1;
    std::string statusMsg;
    double statusUntil = 0;
    std::string statusNext;       // shown when the current status expires
    double statusNextSecs = 4;
    bool macHintShown = false;
    int targetCycle = -1;         // N in targeting mode cycles the nearest stars
    bool fieldAmp = false;        // F: field amplificator (brighter star field), M1-10
    double arrivalFlash = 0;      // white flash when a Vimana flight ends
    bool mapClouds = true;        // landing map cloud overlay (C)
    uint64_t mapBaseSeed = 0;     // body seed the cached base was built for
    bool mapBaseClouds = true;
    std::vector<uint32_t> mapBase;
    std::vector<uint8_t> mapEmissive;
    int menuSel = 0;
    int settingsSel = 0;
    int keysSel = 0;               // N5-05 key bindings screen
    bool keysCapture = false;
    void updateKeysScreen(const Input& in);
    void renderKeysScreen();
    int slotSel = 0;
    bool slotLoadMode = false;
    std::vector<std::string> slotSummaries;
    double autosaveTimer = 0;
    int listSel = 0, listScroll = 0;
    // M3 guide screens
    int guideSel = 0, logPage = 0, gallerySel = 0, galleryLoaded = -1, textKind = 0, textBody = -1, mapPick = -1, mapClassMask = 63;
    GameState guideReturn = GameState::SPACE;
    std::string textPrompt, textBuffer;
    double mapYaw = 0, mapPitch = 0, mapZoom = 1;
    std::vector<std::string> galleryFiles;
    std::vector<uint32_t> galleryPix; int galleryW = 0, galleryH = 0; bool galleryOk = false; std::string galleryCaption;
    double walkMax = 0, lastWalkX = 0, lastWalkZ = 0, lastOdometer = 0;
    double drivenLanding = 0;     // metres driven this landing, over every buggy deployed (the launch log)
    bool visitNoted = false;
    // M2 cabin
    Cabin cabin;
    GrainTexture cabinGrain;
    int shipScreenPage = 0, shipScreenSel = 0;
    std::vector<std::string> consoleLines;
    std::string consoleInput;
    Mat3 viewBasis() const;
    void cabinPalette();
    void updateCabin(const Input& in, double realDt);
    void drawCabin();
    void renderCabinHUD();
    void updateShipScreen(const Input& in); void renderShipScreen();
    void openConsole(); void consolePrint(const std::string& line); void consoleCommand(const std::string& raw); void updateConsole(const Input& in); void renderConsole();
    void deployCapsule(); void toggleVimana();
    // M4-02 sector map, M4-10 vision modes
    int visionMode = 0;
    int sectorZoom = 1;
    std::vector<uint32_t> sectorImg; int sectorImgW = 100, sectorImgH = 80; double sectorImgX = 1e9, sectorImgZ = 1e9; int sectorImgZoom = -1, sectorImgEpoch = -1;
    static constexpr int SECTOR_LEVELS = 6;   // O1-04: 2, 8, 32, 128, 512 and 2048 km wide (the widest levels only on big worlds)
    int sectorMaxLevel() const;
    void updateSectorMap(const Input& in); void renderSectorMap();
    void applyVision();
    void openGuide(); void updateGuideMenu(const Input& in); void renderGuideMenu();
    void beginTextEntry(const std::string& prompt, int kind, const std::string& initial); void updateTextEntry(const Input& in); void renderTextEntry();
    void openStarMap(); void updateStarMap(const Input& in); void renderStarMap();
    void updateLog(const Input& in); void renderLog(); void renderStats();
    void openGallery(); void updateGallery(const Input& in); void renderGallery();
    void setRemoteStar(const Star& s); void targetStarByName(const std::string& name); void targetPreviousStar(); void targetHome();
    void logEvent(const std::string& kind, const std::string& text); void noteVisit();
    int helpPage = 0;
    double titleYaw = 0;
    double lastRealDt = 1.0 / 60;
    double flashTimer = 0;
    std::string arrivalNote;
    SurfaceSite probeSite;
    double probeLat = 1e9, probeLon = 1e9;
    int probeBody = -1;

    void status(const std::string& m, double secs = 4);
    void updateSpace(const Input& in, double dt, double realDt);
    void updateShipMotion(double dt);
    void renderSpace();
    void renderSpaceHUD();
    void updateLandingMap(const Input& in, double dt);
    void renderLandingMap();
    void updateDescentAscent(const Input& in, double dt, double realDt);
    void updateSurface(const Input& in, double dt, double realDt);
    void renderSurfaceScene();
    void renderSurfaceHUD();
    void renderTitle();
    void renderHelp();
    void renderMenu();
    void updateSettingsScreen(const Input& in);
    void renderSettings();
    void openSlots(bool loadMode);
    void updateSlots(const Input& in);
    void renderSlots();
    void adjustSetting(int item, int dir);
    void renderSystemList();
    void renderDataSheet();
    void drawVisor(uint32_t col);
    void drawCommonHUD();
    void arriveAtStar(const Star& s, const Vec3& fromDir);
    void beginLanding();
    void lockRemoteTarget();
    void nearestStars(int n, std::vector<const Star*>& out) const;
    void cycleTargetStar();
    void buildLandingMapBase();
    int pickBodyNearCrosshair();
    void startApproach(int body);
    // O3 (R-302): belts as destinations
    bool pickBeltRock(int k, double at, int64_t& ir, int64_t& ia, int64_t& iy, int& m) const;   // the biggest rock near the ship's angle round the star
    bool beltRockNow(BeltRock& rk) const;          // the target's or the parking place's rock at the current time
    Vec3 beltParkPos(double at) const;             // the parking point beside that rock
    void startApproachBelt(int k);
    std::string localTargetLabel() const;          // the body's or the belt's name
    void choosePaletteBodies(int& a, int& b);
    std::string epocString() const;
    std::string epocFullString() const;   // M5-05: EPOC:SINISTER.MEDIUS.DEXTER
    static double wallClockT();            // M5-05: game seconds from the wall clock (epoch 2026-01-01 UTC)
    bool timeLapse = false;                // M5-06: Ctrl+T runs the clock at x600 for 25 s, then back to x1
    double timeLapseUntil = 0;
    std::string distanceString(double km) const;
    void handleGlobalKeys(const Input& in);
    Vec3 shipWorldPos() const { return ship.pos; }
};
