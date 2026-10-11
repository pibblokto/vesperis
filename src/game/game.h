// Game orchestration: states, the Stardrifter's autopilot, HUD and menus.
#pragma once
#include "core/framebuffer.h"
#include "core/font.h"
#include "core/input.h"
#include "galaxy/starfield.h"
#include "galaxy/system.h"
#include "galaxy/music.h"
#include "galaxy/voice.h"
#include "galaxy/signals.h"
#include "galaxy/charts.h"
#include "galaxy/almanac.h"
#include "galaxy/fronts.h"
#include "galaxy/probe.h"
#include "galaxy/legends.h"
#include "galaxy/drones.h"
#include "space/space_view.h"
#include "surface/surface_view.h"
#include "audio.h"
#include "settings.h"
#include "guide.h"
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <atomic>
#include <thread>

enum class GameState { TITLE, SPACE, LANDING_MAP, DESCENT, SURFACE, ASCENT, HELP, MENU, SYSTEM_LIST, DATA, SETTINGS, SLOTS,
                       GUIDE, TEXT_ENTRY, STAR_MAP, LOG, STATS, GALLERY, SHIPSCREEN, CONSOLE, SECTOR_MAP, KEYS, SHARDS, ALMANAC, PROBE, RECORDING,
                       INSTRUMENTS,    // C-11: the instruments' case
                       DRONES };       // C-18: the drone bay's screen

// C-18: a drone of the bay as a model in its own metres (x its right, y up, z its nose; its feet at y 0), drawn filled on its rack in the
// cabin and as lines on the bay's screen (game/drone_draw.cpp): a short hull with the dark band of its eye, four ducted fans on arms, two
// arms under its nose with three-fingered hands, a hold, its number in lamps on its back and a beacon
struct DronePose {
    double arms = 0;     // 0 folded under the belly .. 1 reaching down and forward
    double grip = 0;     // 0 the fingers open .. 1 closed on a find
    double spin = 0;     // the fans' angle (radians)
    double blur = 0;     // 0 still blades .. 1 a disc
    int number = 1;      // its lamps (one, two, three)
    bool lit = true;     // its eye and its beacon lit
    double blink = 0;    // the beacon's clock (seconds)
};
enum DroneMat { DM_PAINT = 0, DM_DARK, DM_METAL, DM_ACCENT, DM_EYE, DM_BEACON, DM_NUMBER };
struct DronePrim {
    int kind = 0;        // 0 a face (`n` points), 1 a tube from p[0] to p[1] of radius `r`, 2 a lamp at p[0] (`r` its size in pixels at a metre)
    int n = 0; Vec3 p[6];
    Vec3 nrm;            // a face's outward normal
    double tint = 1;     // a face's paint
    int mat = DM_PAINT;
    double r = 0.01;
};
void droneModel(const DronePose& pose, std::vector<DronePrim>& out);
constexpr double DRONE_SEARCH_S = 120;     // C-18 (the user's, 2026-10-11): two minutes down there, from the first place a drone searches to its last
constexpr double DRONE_COOLDOWN_S = 300;   // C-18: five minutes charging from the drones' return to the next launch (real time; the user's, 2026-10-11:
                                           // "2 minutes + 5 minutes recharge", at first five minutes from launch to launch)

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

// C-17: an instrument's parts drawn as it lies, seen from above, in a box of the canvas's logical units (the case's picture, the harness's
// sheets: game/instrument_draw.cpp): lit from the upper left, its courses glowing as `glow` says, a ruler of its size when `ruler`
void drawInstrumentParts(RGBCanvas& c, int x0, int y0, int w, int h, const std::vector<InstrumentPart>& parts, const FormPalette& pal, const std::vector<double>& glow, double t, bool ruler);

// H-06: a record's kind (RC_*, galaxy/records.h) and its glyph on the decoder's pages (a page, a note, a star; game/history_screen.cpp)
int recordKindOf(const Shard& s);
void drawRecordKindGlyph(RGBCanvas& fb, int x, int y, int kind, uint32_t col);

class Game {
public:
    Game();
    ~Game();   // R-408: joins the landmark names' migration if one is running
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
    // R-408 (game/marks.cpp): the explorer's marks (Guide::marks) and the surface scanner. `R` on the ground cycles the scanner
    // through the kinds the world can hold (off, ruins, lakes, peaks, craters, canyons, mesas, geysers, crystals), Shift+R leaves
    // the marked sights out or takes them in; `L` marks what the crosshair rests on; the sector map's cursor places, names and
    // removes marks (B-412: and shows one over the view). O6-06's landmarks are no longer markers: a sight is an echo of the scanner
    // until the explorer marks it
    int scanMode = -1;                 // -1 off, else the LandmarkKind the scanner listens for
    bool scanSkipMarked = true;        // a sight with one of the explorer's marks on it is left out
    std::string worldKeyOf(int bi) const { return Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, bi); }
    struct SiteMark { int index = -1; double x = 0, z = 0, y = 0; bool dry = false; };   // a mark of the surface's world in local metres, its ground's height (B-422: dry, read without the rivers until their tiles are built)
    std::vector<SiteMark> siteMarks; int siteMarksEpoch = -1, siteMarksBody = -1, marksVersion = 0, siteMarksVersion = -1; size_t siteMarksCount = 0; double siteMarksDryT = 0;
    const std::vector<SiteMark>& marksHere();                  // the surface's world's marks (rebuilt when the marks, the landing or the frame change)
    int markNear(double x, double z, double radiusM);          // the mark of this world nearest a point within a radius (its index in Guide::marks), -1 none
    std::string markLabel(int mi) const;                       // its name, else "A MARK" / "A MARK: RUINS"
    bool echoMarked(const SurfaceView::ScanEcho& e);           // one of the explorer's own marks stands on that sight
    void echoHeardAt(const SurfaceView::ScanEcho& e, double& x, double& z) const;   // where the scanner hears it: off by up to 8% of its distance
    const SurfaceView::ScanEcho* scanNearest(double& hx, double& hz);   // the nearest sight of the mode (the marked left out, unless taken in), heard at
    bool scanModeHere(int mode) const;                         // the world can hold that kind
    void scanCycle(); void scanToggleSkip();
    std::string echoDetail(const SurfaceView::ScanEcho& e) const;   // "A TOWN", "1240 M HIGH", "2.3 KM ACROSS"
    void drawScanHUD();                                        // the scanner's lines at the top left and its mark on the compass
    void drawCompassMark(double bearing, uint32_t col, bool filled);   // a diamond on the compass strip at a bearing, an arrow at its end beyond sixty degrees
    // B-412: one mark at a time over the view, none until the explorer picks it (the sector map's Shift+Enter or Shift+click on a
    // mark, again to hide it): the user found every mark within 25 km drawn over the view, their names run into one another
    const SiteMark* shownMarkHere();                           // the shown mark (Guide::shownMark) when it stands on the surface's world
    void showMark(int mi);                                     // that mark shown over the view in place of any other; the shown one hidden
    void drawShownMark();                                      // its diamond over the ground with the name and the distance, its diamond on the compass
    void beginMark(double x, double z);                        // a new mark at a place of the site: the entry for its name
    void beginRenameMark(int mi);
    int textMark = -1; double textMarkLat = 0, textMarkLon = 0; int textMarkKind = MARK_PLACE;   // what the entry names: a mark (-1: a new one at the place)
    void finishMark(const std::string& name);                  // the entry's Enter
    void removeMark(int mi);
    void migrateLandmarkNames();                               // R-408: the sights the explorer named before become marks where they stand (this system's worlds), found on a thread
    void collectMigration();                                   // the main thread puts the found ones in the guide (every frame)
    struct MigrationJob { std::string key; int pass = 0; bool found = false; Landmark lm; std::string body; };
    std::thread migrateThread; std::atomic<bool> migrateBusy{false}; std::vector<MigrationJob> migrateOut;
    std::set<std::string> migrateTried;                        // the names asked once (one not found again is not asked every frame)
    // the sector map's cursor (local metres), a removal armed (the mark, until when), the mark Tab went to
    double mapCurX = 0, mapCurZ = 0, mapDelUntil = -1; int mapDelMark = -1, mapTabMark = -1;
    std::string shardKey(int index) const;                     // C-03: "<body key>/S<index>" on the surface's body
    void syncShards();                                         // C-03: the view hides this world's shards the guide already holds (C-11: and its instruments)
    std::string instrumentKey(int id) const;                   // C-11: "<body key>/I<id>" on the surface's body
    void takeInstrument();                                     // C-11: the instrument within reach into the guide and the cabin's case
    std::set<uint64_t> roadsMet;                               // C-09: the old roads met on this landing (a status and a log line the first time on each)
    void takeShard();                                          // C-03: E at a shard: the guide, the log, the status
    std::string graveKey(uint64_t id) const;                   // C-12: "<body key>/G<id>" on the surface's body
    bool shardsPending() const;                                // C-06: a shard the decoder has not read at its world's current share (the decoder's screen glows)
    std::string testShardsInfo() const;
    std::string testHistoryInfo();                                             // H-06: the decoder's pages for the harness (the page, the told events, the age, the eras)
    const ToldHistory& testTold() { ensureTold(); return told; }               // H-06: what the open world's read records told
    std::vector<std::string> testLegendLines();                                // H-06: the history page's rows as the screen shows them (the words as read)
    int testShardsPage() const { return shardsPage; }
    int testShardSel() const { return shardHeld.empty() ? -1 : shardHeld[std::max(0, std::min((int)shardHeld.size() - 1, shardSel))]; }   // H-06: the record under the cursor (its local index)
    int testShardCursor() const { return shardSel; }                           // R-416: the cursor's place among the records held (the timeline's)
    int testShardsFilter() const { return shardsFilter; }                      // R-417: the kind the records' page shows (0 every kind)
    int testShardsHeld() const { return (int)shardHeld.size(); }
    int testLegendSel() const { return legendSel; }                            // R-416: the history's event under the cursor
    std::string testRadarInfo() const;         // C-07: the radar's state for the harness
    bool testAimAtSignal(int which);           // C-07: turn the ship (the cabin view forward) onto signal `which` of the radar's list (far first, then local)
    int testRadarSignals() const { return (int)radar.all.size(); }
    int testRadarLocked() const { return radar.locked; }
    int testRadarKind(int which) const { return which >= 0 && which < (int)radar.all.size() ? radar.all[which].kind : -1; }
    int testRadarProgramme() const { return radar.prog.kind; }   // the programme on the air (RP_*, -1 none)
    bool testRadarLocal(int which) const { return which >= 0 && which < (int)radar.all.size() && radar.all[which].local; }
    bool testRadarOn() const { return radar.on; }
    bool testRadarSector(int which, int64_t& sx, int64_t& sy, int64_t& sz) const { if (which < 0 || which >= (int)radar.all.size()) return false; sx = radar.all[which].star.sx; sy = radar.all[which].star.sy; sz = radar.all[which].star.sz; return true; }
    double testRadarDist(int which) const { return which >= 0 && which < (int)radar.all.size() ? radar.all[which].distLy : -1; }   // C-15
    double testRadarNeedGain(int which) const { return which >= 0 && which < (int)radar.all.size() ? radar.all[which].needGain : -1; }
    int testRadarGainStep() const { return radar.gainStep; }
    bool testRadarDeepReady() const { return radar.deepReady; }
    bool testRadarOverload() const { return radar.overload; }
    double testRadarLockSecs() const { return radar.lockSecs; }
    double testRadarReading() const { return radar.reading; }
    int testRadarUnder() const { return radar.under; }                     // C-16
    bool testRadarRise(Vec3& dir) const { dir = radar.rise.dir; return radar.rise.on; }
    int testRadarRises() const { return radar.rises; }
    int testRadarNothings() const { return radar.nothings; }
    double testRadarHold() const { return radar.holdT; }
    int testRadarWords() const { return radar.words; }
    bool testRadarWordOn() const { return radar.wordT >= 0; }
    std::string testRadarWordSaid() const { return radar.wordSaid; }
    double testRadarFloor() const { return radar.floorNow; }
    std::string testRadarWordKey() const { return radar.wordKey; }
    double testRadarSky() const { return radar.sky; }
    Vec3 testRadarDir(int which) const { return which >= 0 && which < (int)radar.all.size() ? normalize(radar.all[which].pos - ship.pos) : Vec3(0, 0, 1); }
    bool testAimAtDir(const Vec3& d) { Vec3 f = normalize(d); ship.yaw = std::atan2(f.x, f.z); ship.pitch = std::asin(std::max(-1.0, std::min(1.0, f.y))); cabin.yaw = 0; cabin.pitch = 0; return true; }
    bool testCivilisedHere() const;    // C-07: a people's world in the system the ship is at
    bool testShardsOpenIndex(int idx);   // C-04: at the world's list, opens the shard of that index (a piece plays)
    // C-11 (tests): the ship's player and the instruments' case
    int testPlayerItems();                     // the player's list, built afresh: its items
    std::string testPlayerItemLine(int i) const;
    bool testPlayerPlay(int item);             // plays an item of the list from its start
    std::string testPlayerInfo() const;        // what plays, its clock, held / paused / waiting, the synth's pointers
    int testPlayerPlaying() const { return player.playing; }
    bool testPlayerHeld() const { return player.held; }
    double testPlayerT() const { return player.t; }
    int testPlayerFinished() const { return player.finished; }
    void testPlayerSkipTo(double t) { player.t = t; player.syncT = -1; }   // the player's clock moved on (a long piece's end without its minutes)
    bool testPlayerOwnsPiece() const { return audio.piece && audio.piece == &player.world.piece; }
    bool testPlayerOwnsSpeech() const { return audio.speech && audio.speech == &player.world.speech; }
    int testShardsTop() const { return shardsTop; }
    std::string testPlayerHud() const { return playerHudLine(); }
    int testShipMode() const { return (int)ship.mode; }
    int testWalkToInstrument(int age = -1);    // the explorer before the nearest instrument not taken (of that age, -1 any), facing it: its id, -1 none
    int testNearInstrumentId() const { return surf.nearInstrument.id; }
    std::string testInstrumentInfo() const;    // the case: the one on the screen, its timbre, its register, the notes played
    int testInstrumentCount() const { return (int)guide.instruments.size(); }
    // C-18 (tests): the case's worlds, and the drone bay
    int testCaseLevel() const { return icase.level; }
    int testCaseWorldSel() const { return icase.worldSel; }
    int testCaseWorlds() const { return (int)icase.worlds.size(); }
    int testCaseWorldItems(int w) const { return w >= 0 && w < (int)icase.worldCounts.size() ? icase.worldCounts[w] : -1; }
    std::string testCaseWorldKey(int w) const { return w >= 0 && w < (int)icase.worlds.size() ? icase.worlds[w] : std::string(); }
    int testCaseWorldWhich(int w) const { return w >= 0 && w < (int)icase.worldWhich.size() ? icase.worldWhich[w] : -1; }
    int testCaseKeys() const { return (int)icase.keys.size(); }
    std::string testCaseKey(int i) const { return i >= 0 && i < (int)icase.keys.size() ? icase.keys[i] : std::string(); }
    bool testDronesLaunch();                    // the bay's send (its screen's Enter): true when they went
    void testDronesSettle();                    // the world's items read now (the thread waited for) and the haul drawn
    void testDronesSkip(double clock);          // the bay's clock set forward in steps (the docks, the deliveries on the way, as the frames would)
    std::string testDronesInfo() const;         // the trip: each drone's state, stops and finds; the cooldown; the world's items
    int testDronesFinds(int kind = -1) const;   // the last trip's finds (of a DroneFindKind), aboard or not
    int testDronesAboard() const;               // of them, aboard
    int testDronesWanted(int k) const { return k >= 0 && k < DRONE_COUNT ? bay.run[k].wanted : 0; }
    int testDronesOfDrone(int k) const { return k >= 0 && k < DRONE_COUNT ? (int)bay.run[k].finds.size() : 0; }
    std::string testDronesFindKey(int k, int i) const;   // a find's guide key ("<body>/S<index>", "<body>/I<id>")
    double testDronesClock() const { return bay.clock; }
    bool testDronesOut() const { return bay.out; }
    bool testDronesPlanned() const { return bay.planned; }
    bool testDronesReady() const { return droneBayReady(); }
    double testDronesCooldown() const { return droneCooldownLeft(); }
    double testDronesBack() const { return bay.back; }
    int testShardsListed() const { return (int)shardHeld.size(); }            // C-18: the open world's records the decoder lists
    int testDronesWorldItems(int kind = -1, bool unheld = false);   // the parked world's items (of a kind; not held), read now
    double testDroneRackX(int k) const;          // where a drone stands in the cabin (its rack's slot, the launch moving it), its height
    double testDroneRackY(int k) const;
    std::string testDroneState(int k) const { return k >= 0 && k < DRONE_COUNT ? droneStateLine(k) : std::string(); }
    void testOpenDroneBay() { openDroneBay(); }
    void testShardsTop(int top) { shardsTop = top; }
    int testShardsLevel() const { return shardsLevel; }                        // C-06: the decoder's state for the harness
    std::string testChartInfo() const;         // C-10: the chart on the screen and the one held up
    bool testAimAtChartMark(int figure = -1);  // C-10: turn the ship onto the held chart's marked star (or onto figure `figure`'s brightest star)
    bool testRemote(int64_t& sx, int64_t& sy, int64_t& sz) const { if (!ship.hasRemote) return false; sx = ship.remote.sx; sy = ship.remote.sy; sz = ship.remote.sz; return true; }
    int testChartLines() const { return chartLinesDrawn; }
    bool testChartUp() const { return chartUp; }
    int testChartStarsInSky() const { int n = 0; for (const ChartStar& c : chartHeld.stars) for (const Star& s : nb.stars) if (s.seed == c.star.seed) { n++; break; } return n; }   // of the held chart's stars, how many the sky here draws
    int testEchoes(int kind = -1) const { int n = 0; for (const auto& e : surf.echoes) if (kind < 0 || e.lm.kind == kind) n++; return n; }   // R-408
    bool testScanned() const { return surf.scanned; }
    int testScanMode() const { return scanMode; }
    std::string testScanInfo();                                // the mode, the echoes, the nearest (true and heard), the marks here
    bool testEchoTrue(double& x, double& z, double& radius);   // the nearest echo of the mode's true place
    void testMapCursor(double x, double z) { mapCurX = x; mapCurZ = z; }
    int testShownMark() const { return guide.shownMark; }   // B-412
    bool testMarkOnScreen(int mi, int& lx, int& ly) {       // B-412: where a mark's diamond over the view stands (logical pixels); false behind the eye
        for (const SiteMark& m : marksHere()) if (m.index == mi) { double sx, sy; if (!surf.projectPoint(m.x, m.y + 6, m.z, sx, sy)) return false; lx = (int)(sx / FB_SCALE); ly = (int)(sy / FB_SCALE); return true; }
        return false;
    }
    void testMarkLocal(int mi, double& x, double& z) const { const SurfaceMark& m = guide.marks[mi]; surf.site.localAt(StarSystem::bodyFromLatLon(m.lat, m.lon), x, z); }
    int testMarksHere(int& dry) { dry = 0; for (const SiteMark& m : marksHere()) dry += m.dry ? 1 : 0; return (int)marksHere().size(); }   // B-422: the world's marks, and those read without their rivers
    bool testMarkGround(int mi, double& y, double& wet) {   // B-422: a mark's ground as the marks hold it, and the ground there with the rivers (ask once its tiles are built)
        for (const SiteMark& m : marksHere()) if (m.index == mi) { y = m.y; const TerrainVertex v = surf.site.sampleAt(m.x, m.z, 64.0); wet = std::max((double)v.h, (double)v.water); return true; }
        return false;
    }
    const BodyGen& testSiteGen() const { return surf.site.gen; }   // B-422
    void testWalkTo(double x, double z) { surf.player.x = x; surf.player.z = z; surf.player.y = surf.site.surfaceHeight(x, z); }
    double testPlayerX() const { return surf.player.x; }
    double testRange() const { return surf.lastRange; }
    double testRangeX() const { return surf.lastRangeX; }
    double testRangeZ() const { return surf.lastRangeZ; }
    void testLatLonAt(double x, double z, double& lat, double& lon) const { surf.site.latLonAt(x, z, lat, lon); }
    double testPlayerZ() const { return surf.player.z; }
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
    int testZoomRoads() const { return (int)zoomRoads.size(); }   // C-09: the roads drawn on the zoom
    int testZoomCores() const { return zoomCores; }                // T-01: the cores drawn on the zoom
    std::string testLandPeoplesInfo();                               // R-415: the landing map's peoples: what is known, what the layer drew
    void testSurfaceFace(double yaw, double pitch) { surf.player.yaw = yaw; surf.player.pitch = pitch; }   // T-01 (the bench): the explorer's look
    const SurfaceView& testSurface() const { return surf; }   // R-412 (the bench): the forms drawn
    int testZoomWays(int kind) const { int n = 0; for (int k : zoomRoadKinds) if (k == kind) n++; return n; }   // T-01: the zoom's lines of a kind
    std::string testRoadInfo() const;                             // C-09: the site's roads, the road under the explorer, the roads met, the zoom's lines
    void testBuildZoom() { buildLandingZoom(); }
    bool testWalkToRoad(double withinM = 400);                    // C-09: the explorer set on the nearest road's centre line
    std::string testGraveInfo() const;                            // C-12: the grave within reach, the guide's graves, the last log line
    bool testWalkToShard();                                       // C-13: the explorer set a metre from the nearest settlement's first shard the guide does not hold
    void importInboxFile();                                       // C-14: the guide menu's "import an inbox file" (the harness calls it too): a friend's export beside the guide
    int testPeoples() const { return surf.peoples; }              // C-13: the peoples of the world under the feet
    int testNearShardIndex() const { return surf.nearShard.index; }   // C-13: the shard within reach (its world index), -1 none
    // T-04: the explorer set before the nearest record of a medium (-1 any) the guide does not hold, looking at it (an archive's from its row's
    // middle when `archive`); its world index, -1 none. What the frame drew of the records' objects
    int testWalkToRecord(int medium = -1, bool archive = false);
    int testNearShardMedium() const { return surf.nearShard.index >= 0 ? surf.nearShard.medium : -1; }
    int testRecordFormsDrawn() const { return surf.lastRecordForms; }
    bool testWalkToGrave();                                       // C-12: the explorer set a metre in front of the nearest settlement's first grave, facing it
    bool testWalkToWork(double withinM = 1e9);                    // T-03: the explorer set 150 m from the nearest work the scanner hears (the frame re-anchored and the scan asked again from there)
    void testLandCursor(double latDeg, double lonDeg) { landLat = latDeg * DEG; landLon = lonDeg * DEG; }   // C-09: the landing map's cursor
    void testLandSite();   // G-04: the landing map's cursor on a lit land site chosen from the body's seed (R is seeded by the clock, J follows the moon); S-01: level enough for the buggy
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
    double testFrontBandAt(double latDeg, double lonDeg);        // W-03: the fronts' cloud on the landing map's overlay at a place of the parked body now (0..1)
    std::string testFrontInfo() const;                           // W-03: the forecast line and the fronts' numbers at the feet
    std::string testForecastLine() const { return forecastLine(); }
    double testMapShadowMax();                                    // O0-04: the darkest shadow on the landing map now (0..1)
    void testSetTime(double tt) { t = tt; }
    const char* testReliefClass(double lat, double lon);         // N1-06: FLAT / HILLS / MOUNTAINS at a site of the parked body
    static double siteSlope(const SurfaceSite& probe);           // N1-06: steepest rise over run around a site
    static double parkDistanceFor(const Body& b);  // 3.5 radii; outside the rings; 12 radii for a comet (N0-01)
    std::string testStatus() const { return statusMsg; }
    std::string testStatusNext() const { return statusNext; }   // T-04: what is said when the status now shown expires
    std::string testDebugInfo() const;
    std::string testHealth() const;   // "" when every number is finite
    double testPlayerStamina() const { return surf.player.stamina; }
    double testPlayerSpeed() const { return std::sqrt(surf.player.vx * surf.player.vx + surf.player.vz * surf.player.vz); }
    double testPlayerAlt() const { return surf.player.altAboveGround; }
    void testWalkToBuggy() { if (surf.buggy.deployed) { surf.player.x = surf.buggy.x + 1.5; surf.player.z = surf.buggy.z; } }
    std::string testBuggyInfo() const;
    // R-403: the drone
    void testWalkToDrone() { if (surf.drone.deployed) { surf.player.x = surf.drone.x + 1.5; surf.player.z = surf.drone.z; } }
    std::string testDroneInfo() const;
    double testDroneSpeed() const { return std::fabs(surf.drone.speed); }
    double testDroneAlt() const { return surf.drone.altAboveGround; }
    bool testDroneLanded() const { return surf.drone.landed; }
    bool testFlySetup(double altM);   // deploy the drone beside the capsule, get in, lift it to altM over the ground (for the bench)
    void testSetPitch(double p) { surf.player.pitch = p; }
    void testSetYaw(double y) { surf.player.yaw = y; }
    double testPitch() const { return surf.player.pitch; }   // B-412: the flow's look put back
    double testYaw() const { return surf.player.yaw; }
    std::string testRangeInfo() const { return "range=" + std::to_string((int)surf.lastRange) + " water=" + std::to_string((int)surf.lastRangeWater); }   // O1
    int testParkedBelt() const { return ship.parkedBelt; }                                                                  // O3
    std::string testBeltDraw() const;   // O3: rocks drawn last frame and the bank-20 pixels in the framebuffer
    std::string testMarksInfo() const { return "waypoint=" + std::to_string((int)surf.hasWaypoint) + " zoom=" + std::to_string(sectorZoom) + "/" + std::to_string(sectorMaxLevel()); }   // B-303, O1-04
    double testEscapeVelocity() const { return surf.site.escapeVelocity; }                                                  // O4
    void testTypeText(const std::string& s);   // feeds text entry as key presses
    std::string testStarName() const { return sys.valid ? starNameOf(sys.star) : ""; }
    std::string testStarGenName() const { return sys.valid ? sys.star.name : ""; }   // the generator's name (never shown, R-401)
    bool testDrainageReady() const;   // O6-03: the drainage tiles of the landing site are in the cache
    int testBodyType(int bi) const { return bi >= 0 && bi < (int)sys.bodies.size() ? sys.bodies[bi].type : -1; }
    std::string testBodyTypes() const { std::string r; for (auto& b : sys.bodies) r += std::to_string(b.index) + ":" + PLANET_TYPES[b.type].name + (b.parent >= 0 ? "(moon) " : " "); return r; }
    void testCabinGoto(double x, double z, double yaw, double pitch = 0) { cabin.x = x; cabin.z = z; cabin.yaw = yaw; cabin.pitch = pitch; }
    void testShipScreenRow(int page, int sel) { shipScreenPage = page; shipScreenSel = sel; }   // B-410: the flight computer's row Enter acts on
    double testFlightDur() const { return ship.flightDur; }   // R-407: the Vimana flight's length (real seconds)
    double testFlightT() const { return ship.flightT; }
    bool testFlying() const { return ship.mode == ShipState::VIMANA; }
    void testFlightSkip(double tt) { if (ship.mode == ShipState::VIMANA) ship.flightT = tt; }   // G-07: the flight's clock set (the band's keyframes catch up)
    void testStartJump(double ly, const Vec3& dir) { startJump(ly, dir); }   // G-07: R-407's jump along a direction, plotted and flown
    void testCabinTurn(double yaw, double pitch) { cabin.yaw = yaw; cabin.pitch = pitch; }   // G-07: the head turned in the cabin
    void testCabinLook(const Vec3& dir) { const Vec3 d = cameraBasis(ship.yaw, ship.pitch) * dir; cabin.yaw = std::atan2(d.x, d.z); cabin.pitch = 0; }   // G-07: the head turned toward a direction of the galaxy, level (at the windows)
    const SpaceRenderer::FlightBand& testFlightBand() const { return spaceR.fband; }
    long testParkedBandBuilds() const { return spaceR.parkedBandBuilds; }
    bool testAiming() const { return ship.targeting; }
    std::string testJumpDigits() const { return jumpDigits; }
    Vec3 testAimDirection() const { return aimDirection(); }
    std::string testAimLabel(const Star& s) const { Star full; starInSector(s.sx, s.sy, s.sz, full, true); return aimStarLabel(full); }
    const StarNeighborhood& testNeighborhood() const { return nb; }
    Vec3 testShipPos() const { return ship.pos; }
    // G-05: the galaxy chart's frame (logical pixels: the face-on disc over the width, the edge-on strip under it) and its levels
    static constexpr int CHART_LEVELS = 9, CHART_FACE_Y0 = 22, CHART_FACE_H = 128, CHART_EDGE_Y0 = 152, CHART_EDGE_H = 22;
    static constexpr double CHART_TALL_LY = 110000;   // the face's height at level 0: the whole disc
    static double chartScale(int level) { return CHART_TALL_LY / std::ldexp(1.0, level) / CHART_FACE_H; }   // light years a logical pixel
    // G-05: the galaxy chart for the harness
    int testMapPage() const { return mapPage; }
    void testChartLevel(int level) { gchart.level = std::max(0, std::min(CHART_LEVELS - 1, level)); }
    void testChartFollow() { gchart.follow = true; }
    double testChartBuildMs() const { return gchart.buildMs; }
    int testChartYouX() const { return gchart.youX; }
    int testChartYouY() const { return gchart.youY; }
    void testArriveAt(const Star& s) { arriveAtStar(s, Vec3(0, 0, -1)); }   // the arrival as a Vimana flight ends it
    void testPlaceShip(const Vec3& posKm) { ship.pos = posKm; ship.mode = ShipState::STANDBY; nb.update(ship.pos); }   // the ship set down anywhere (the harness's chart)
    // G-06: the chart's cursor and plot for the harness
    void testChartCursor(const Vec3& p) { gchart.onShip = false; gchart.plotHere = false; gchart.curX = p.x; gchart.curY = p.y; gchart.curZ = p.z; if (gchart.level > 0) { gchart.follow = false; gchart.cx = p.x; gchart.cz = p.z; } }
    Vec3 testChartCursorAt() const { return chartCursor(); }
    bool testChartOnShip() const { return gchart.onShip; }
    bool testChartPlotted() const { return chartPlotted(); }
    bool testChartLook() const { return gchart.look; }
    const StarNeighborhood& testLookStars() const { return gchart.lookNb; }
    int testChartCurX() const { return gchart.curPX; }
    int testChartCurY() const { return gchart.curPY; }
    double testJumpShort() const { return jumpPlot.shortLy; }
    int testMapPick() const { return mapPick; }
    int testCabinFacing() const { return cabin.facing; }   // 7 is the shard decoder (C-06)
    int testCabinLights() const { return (cabin.light ? 1 : 0) | (cabin.depolarised ? 2 : 0); }   // B-411: 1 the light on, 2 the hull glass
    int testLogEntries() const { return (int)guide.log.size(); }
    // W-01: the almanac for the harness
    bool testAlmanacRunning() const { return almanacRunning; }
    double testAlmanacTarget() const { return almanacRunT; }
    double testTimeWarp() const { return timeWarp; }
    int testAlmanacEvents() const { return (int)almanacEvents.size(); }
    std::string testAlmanacInfo() const;                 // the place, its state line, the events with their countdowns
    bool testAlmanacSelect(int kind, int body = -1);     // the open menu's selection onto the first event of a kind (of a body; kind -1 any); false when none
    // W-06: the telescope for the harness
    bool testTelescopeOn() const { return tele.on; }
    double testTelescopeZoom() const { return telescopeZoom(); }
    int testTelescopeTarget() const { return tele.target; }          // a body index, TELE_SUN, TELE_STAR or TELE_NONE
    int testTelescopeTrack() const { return tele.track; }
    std::string testTelescopeInfo() const;                           // the magnification, the field, the reticle's line, what is tracked, the attitude
    std::string testTelescopeLine(bool plain = false) const { return telescopeTargetLine(plain); }
    bool testBodyScreen(int body, double& sx, double& sy, double& rpx) const;   // where the last frame drew a body (framebuffer pixels); false when behind
    int testParkedBody() const { return ship.parkedBody; }
    double testBodyRadiusKm(int body) const { return sys.valid && body >= 0 && body < (int)sys.bodies.size() ? sys.bodies[body].radiusKm : 0; }
    int testTelescopePick() const;                                   // the body with the widest disc from the ship, the parked one left out (-1 none)
    void testAimAtSun() { if (sys.valid) { Vec3 f = normalize(sys.star.pos - ship.pos); ship.yaw = std::atan2(f.x, f.z); ship.pitch = std::asin(clampd(f.y, -1, 1)); } }
    bool testTelescopeStabilised() const { return telescopeStabilised(); }   // R-405
    bool testTelescopeGround(double& lat, double& lon) const;        // the ground under the reticle of the parked body (radians); false when the central ray misses it
    bool testGroundScreen(int body, double lat, double lon, double& sx, double& sy) const;   // where a point of a body's ground falls in the frame (framebuffer pixels); false when behind
    bool testSubShip(double& lat, double& lon) const;                // the ground under the parked ship (radians); false when not parked at a body
    Vec3 testParkDir() const { return ship.parkDir; }
    Vec3 testSpinAxis(int body) const { return sys.valid && body >= 0 && body < (int)sys.bodies.size() ? sys.bodies[body].spinAxis : Vec3(0, 1, 0); }
    long testPlateBegun() const;                     // how many plates the renderer has begun
    void testTelescopeSettle();                      // one frame with the plate read out whole
    std::string testTelescopePlate() const;          // the plate's state (the renderer's line)
    double testTelescopePlateProgress() const;       // 0..1; -1 without a plate body
    void testPlateCheck(int n, double& maxAlb, int& matMiss, double& maxH);   // n of the plate's own cells against the planet function sampled again
    int testScreenColours(double& minDist) const;    // R-404: the number of computers with a colour and the smallest distance between two of them
    // X-01: the probe for the harness
    bool testProbeActive() const { return probe.active; }
    bool testProbeEnded() const { return probe.active && probe.endClock >= 0; }
    int testProbeStage() const;
    double testProbeClock() const { return probe.clock; }
    double testProbeBar() const;
    double testProbeEndClock() const;
    std::string testProbeInfo() const;
    std::string testProbeViewInfo() const;   // X-02: the last probe frame's layers, coverage and section costs
    void testProbeSkip(double toClock);         // the probe's clock set forward (the end found on the way, as the frame would)
    bool testProbeAim(double& lat, double& lon) const { return probeAimPoint(lat, lon); }   // where C would send a probe now
    int testProbeGiant() const;                  // the first giant of the system (-1 none)
    int testProbeStageAt(double clock) const { return probe.active ? probeStageAt(probe.atm, probe.flight, clock) : -1; }
    double testProbeBarAt(double clock) const { return probe.active ? probeBarAt(probe.flight, clock) : 0; }
    void testProbeChute() { if (probe.active && probe.endClock < 0 && probe.flight.chuteOpen < 0 && probe.clock >= probeDescentStart()) { probe.flight.chuteOpen = probe.clock; probe.flight.chuteClose = probeChuteCloseAt(probe.flight, probe.clock); } }
    void testProbeLook(double yaw, double pitch) { probe.camYaw = yaw; probe.camPitch = pitch; }
    double testProbeFlash(double clock) const;    // the lightning's strength in the probe's picture at a clock (0 none)
    bool testProbeBolt(double clock) const;       // X-02: whether that flash throws a channel through the clear air
    bool testProbeQuiet = false;                  // X-02: the harness's stability frames without the camera's shake and the link's static
    bool testProbeAimed(double& lat, double& lon) const { lat = probe.aimLat; lon = probe.aimLon; return probe.active; }   // where the probe was sent
    bool testAimOverride = false; double testAimLat = 0, testAimLon = 0;   // X-03: the harness's aim for C (as if the telescope's reticle were there)
    std::string testProbeCharacter() const { return probe.active ? giantCharacterText(probe.atm) : std::string(); }
    bool testProbeHolePath() const { return probe.active && probe.holePath; }
    const GiantAtmosphere& testProbeAtm() const { return probe.atm; }
    double testGameTime() const { return t; }
    void testAdvanceTime(double dt) { t += dt; }
    double testSunHeight() const { return surf.env.sun.dirLocal.y; }   // T-04: the sun's height over the horizon (its direction's up component)
    const StarSystem& testSystem() const { return sys; }
    void testProbeRelease() { probeRelease(); }
    double testProbeSightClock() const { return probe.active ? probe.sightClock : -1; }
    // X-04: the records, the gallery, the recording's screen, the data sheet's profiles
    bool testOpenRecording(int rec) { return openRecording(rec); }
    void testRecordingSeek(double clock, bool playing) { replay.clock = clock; replay.playing = playing; }
    double testRecordingClock() const { return replay.clock; }
    void testOpenGallery() { guideReturn = GameState::SPACE; openGallery(); }
    int testGalleryItems() const { return (int)galleryItems.size(); }
    int testGalleryRec(int i) const { return i >= 0 && i < (int)galleryItems.size() ? galleryItems[i].rec : -2; }
    int testGallerySel() const { return gallerySel; }
    void testDataSheet(int page) { returnState = GameState::SPACE; state = GameState::DATA; dataPage = page; }
    int testProbeLayerAt(double clock) const { return probeLayerAt(clock); }
    std::string testProbeStormKey() const { return probe.stormKey; }
    int testProbeId() const { return probe.id; }
    void testProbeLookAt(double yaw, double pitch) { probe.camYaw = yaw; probe.camPitch = pitch; }
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
    double landSeenLat = 1e9, landSeenLon = 1e9, landMovedT = -1e9;   // B-422: where the cursor was last frame and when it last moved (its tiles asked for once it rests)
    int landBody = -1;
    // O2 (R-301): the landing map's zoom into the cursor's sector (Z): a window two sectors wide and one tall, sampled
    // from the planet function at the far ring's detail, with the sector grid drawn over it
    int landZoom = 0;
    static constexpr int ZW = 256, ZH = 128;
    std::vector<uint32_t> zoomImg; std::vector<uint8_t> zoomEmissive; std::vector<Vec3> zoomUnit;
    std::vector<uint8_t> zoomFront; double zoomFrontT = -1e18, zoomFrontLat = 1e9, zoomFrontLon = 1e9;   // W-03: the fronts' bands over the window
    double zoomLat = 1e9, zoomLon = 1e9, zoomHalfLat = 0, zoomHalfLon = 0;
    bool zoomDry = false; double zoomPrefetchT = -1e9;   // O6-03: the zoom was drawn without the drainage (its tiles were not built); rebuilt when they are
    uint64_t zoomSeed = 0;
    std::vector<std::vector<std::pair<float, float>>> zoomRoads;   // C-09: the old roads of the window (texel coordinates), drawn as faint lines over it
    std::vector<int> zoomRoadKinds;                                  // T-01: each line's kind (galaxy/plan.h's NetKind: C-09's lanes are roads)
    int zoomCores = 0;                                               // T-01: the plan's places whose cores the window shows
    // R-415: the landing map's peoples (V, off by default): the old roads and the cores over the zoom, the explorer's marks of ruins and works,
    // the places the records held were kept at, and a people's whole land, its places and its ways once all its records are held
    bool landPeoples = false;
    std::vector<uint32_t> zoomCivImg;                                // the zoom with the places' cores (`zoomImg` the land alone)
    struct ZoomPlace { int people = 0, rank = 0, sclass = 0; Vec3 unit; double radiusT = 0; };
    std::vector<ZoomPlace> zoomPlaces;                               // the plan's places in the window (their core's radius in texels)
    struct LandKnown {
        bool ready = false;                    // the world's items read (`droneWorldAsk`: the drone bay's reading of a world's records and their homes)
        int peoples = 0;
        std::string name[2];                   // "THE JENGNE" once a record of theirs is held, else ""
        int held[2] = {0, 0}, total[2] = {0, 0};
        std::set<int> ranks[2];                // the plan's places the held records were kept at
        bool all(int k) const { return k >= 0 && k < peoples && total[k] > 0 && held[k] >= total[k]; }
    };
    LandKnown landKnownOf(int body);
    uint64_t landTerrSeed = 0;                             // the world view's lands of the peoples, per logical pixel (built for a people whose records are all held)
    std::vector<uint8_t> landTerr;                         // bit 0 the first people's land, 1 its frontier; bits 2 and 3 the second's
    void drawLandPeoples(int ox, int oy, int cx, int cy);   // the layer over the map (the world's or the zoom's), the cursor at (cx, cy)
    struct { int places = 0, ways = 0, land = 0, marks = 0, rings = 0, capital = 0; } landDrawn;   // what the layer drew last (the harness)
    double zoomRoadsMs = 0;                                          // C-09: what the window's network cost at the last build
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
    int guideSel = 0, logPage = 0, gallerySel = 0, galleryLoaded = -1, textKind = 0, textBody = -1, mapPick = -1, mapClassMask = (1 << STAR_CLASS_COUNT) - 1;
    GameState guideReturn = GameState::SPACE;
    std::string textPrompt, textBuffer;
    double mapYaw = 0, mapPitch = 0, mapZoom = 1;
    // G-05: the galaxy chart, the star map's second page (game/galaxy_chart.cpp). `Tab` turns the map between the neighbourhood and
    // the galaxy; the map opens on the page it was left on, and on the galaxy in a Vimana flight
    int mapPage = 0;   // 0 the neighbourhood, 1 the galaxy
    struct GalaxyChart {
        int level = 0;                      // 0: the whole disc in the face's height; each level halves it
        bool follow = true;                 // the view centred on the ship (C or Space); a pan lets it go
        double cx = 0, cz = 0;              // the view's centre while it does not follow (sector units)
        std::vector<uint32_t> face, edge;   // the view's images as built last: the disc face-on and edge-on, at the logical resolution
        int builtLevel = -1; long long builtI = 0, builtJ = 0; double builtHalf = 0;
        double buildMs = 0;                 // the last build's time (the harness prints it)
        int youX = -9999, youY = -9999;     // where the ship's crosshair was drawn last frame (logical pixels)
        // G-06: the cursor (the arrows, Shift faster, the mouse; the view moves when it nears the face's edge; PgUp/PgDn its height over
        // the plane, on the strip). On the ship it moves with it; let go, it lies in the plane unless its height is what moved
        bool onShip = true;
        double curX = 0, curY = 0, curZ = 0;   // the cursor's point while it is not on the ship (sector units)
        bool plotHere = false;              // a jump plotted at the cursor's point (Enter); a move of the cursor lets it go
        int stop = -1;                      // N's last stop
        bool look = false;                  // the neighbourhood page shows the plotted end (the look before the leap), not the ship's
        StarNeighborhood lookNb;            // the stars round the plotted end
        Vec3 lookCentre;                    // the plotted star (km)
        int delMark = -1; double delUntil = 0;   // Delete twice removes the mark under the cursor
        int curPX = -9999, curPY = -9999;   // where the cursor was drawn last frame (logical pixels; -9999 on the ship)
    } gchart;
    void chartCentre(int level, double& cx, double& cz) const;   // the view's centre at a level (sector units), before the pixel grid
    void chartView(long long& I, long long& J) const;   // the view's centre in pixels of its level (the pixel grid anchored in the galaxy)
    bool chartProject(double sx, double sz, int& px, int& py) const;   // a place (sector units) on the face, logical pixels; false off the face
    void updateGalaxyChart(const Input& in);
    void renderGalaxyChart();
    // G-06: the next jump chosen on the chart (game/galaxy_chart.cpp). Enter plots the jump to the cursor's point (`plotJump`), Enter again
    // flies it; on a plotted jump Tab looks at the stars round its end (the neighbourhood page round the plotted star, where another can be
    // taken); N steps the cursor through what the explorer has on the chart; L marks the place under it (`Guide::galaxyMarks`)
    static constexpr double CHART_REACH_LY = 54000;    // the cursor's reach from the galactic centre (the whole disc's face)
    static constexpr double CHART_HEIGHT_LY = 30000;   // and over or under the plane
    Vec3 chartCursor() const;                 // the cursor's point (sector units): the ship's while it sits on it
    bool chartPlotted() const;                // a jump plotted at the cursor, still the remote target
    void chartLetGo(bool height);             // the cursor leaves the ship (to the plane unless its height is what moves); a plot lets go
    void chartUnplot();                       // the plot let go (the target stays), its status with it
    void chartEnter();                        // Enter: plot, or fly the plotted jump
    struct ChartStop { Vec3 p; std::string label; uint32_t col = 0; };
    std::vector<ChartStop> chartStops() const;   // N's stops: the locked signals, home, the named stars, the marks, the systems visited (nearest first in each)
    void chartNextStop(int dir);
    int galaxyMarkNear(double sx, double sz, double px) const;   // the explorer's mark of the galaxy within `px` pixels of a place at the chart's level, -1 none
    struct ChartThere { std::string label; uint32_t col = 0; int kind = 0, mark = -1; std::string key; };   // kind: 0 nothing, 1 a mark, 2 a signal, 3 home, 4 a named star, 5 a star visited
    ChartThere chartThere(const Vec3& p) const;   // what the explorer has within four pixels of a place (the first of the kinds)
    std::string galaxyPlace(const Vec3& p, bool height) const;   // "SPIRAL ARM, 13,040 LY FROM THE CORE" (and the height over the plane)
    void beginGalaxyMark(int mi);             // the entry for a new mark at the cursor (-1) or a mark's new name
    void finishGalaxyMark(const std::string& name);
    int textGalaxyMark = -1; Vec3 textGalaxyAt; std::string textGalaxyLine;
    bool drawMapStatus();                     // the status over the map's key line (true when one is shown)
    const StarNeighborhood& mapStars() const { return gchart.look ? gchart.lookNb : nb; }   // the neighbourhood page's stars and centre
    Vec3 mapCentre() const { return gchart.look ? gchart.lookCentre : ship.pos; }
    struct GalleryItem { std::string file; int rec = -1; long long when = 0; };   // X-04: a photograph, or a probe's recording (`rec`: its record; `file` its final image, "" for a lent one: drawn)
    std::vector<GalleryItem> galleryItems;
    std::vector<uint32_t> galleryPix; int galleryW = 0, galleryH = 0; bool galleryOk = false; std::string galleryCaption;
    double walkMax = 0, lastWalkX = 0, lastWalkZ = 0, lastOdometer = 0;
    double drivenLanding = 0;     // metres driven this landing, over every buggy deployed (the launch log)
    double flownLanding = 0;      // R-403: metres flown this landing
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
    // C-06 the shard decoder (game/shards_screen.cpp): the worlds the guide holds shards of, the one open with its fifty and its language, the shard on the screen
    struct ShardWorld { std::string key; bool valid = false; StarSystem sys; int body = -1; Lore lore; Tongue tongue; Language lang; std::vector<Shard> shards;
                        int which = 0, peoples = 1, base = 0;   // C-13: which of the world's peoples this is, how many the world had, the guide index of this people's first shard (its records count from zero here; T-04: `lore.records` of them)
                        Tradition tradition; std::vector<int> pieceForm; std::vector<double> pieceSeconds; Piece piece;    // C-04: its music, the forms of its pieces, the piece on the screen
                        Voice voice; Speech speech;                                                                      // C-05: its voice and the shard on the screen spoken
                        Voice readVoice;                                                                                 // H-05: the ship reading a record with no voice of its own (`voiceRead`)
                        StarChart chart;                                                                                 // C-10: the chart on the screen
                        bool pending = false;                                                                            // H-04: its records are being written on a thread (the radar does not wait)
                        std::shared_ptr<const History> history; std::shared_ptr<const ShardTexts> texts; };              // H-06: the decoder's: its history and its records' texts (what each told)
    ShardWorld shardWorld;
    // H-06: the decoder reads a history: a world's records on three pages (Tab): its records by category (Left/Right: every kind, the texts,
    // the music, the charts), the timeline of its eras with the records held on it, and its history as the records read so far tell it
    // (galaxy/legends.h: the events told in the history's order under the eras they fell in, the persons only as the records named them,
    // the age once told, the last recording closing it); Enter on an event opens the record that told it
    int shardsPage = 0;                          // 0 the records, 1 the timeline, 2 the history
    int shardsFilter = 0;                        // the records' page: 0 every kind, 1 the texts, 2 the music, 3 the star charts
    struct LegendRow { int kind = 0; int item = -1; int year = 0; bool first = false; std::string text; std::vector<DecodedWord> words; };   // 0 a heading, 1 an event's line (`item` of `told.events`; `first` its first row), 2 the close, 3 a way of the people (`text` its label)
    ToldHistory told; bool toldValid = false; double toldMs = 0;   // what the read records told (built when the world's pages are opened, again after a reading; how long it took)
    std::vector<LegendRow> legendRows; std::vector<int> legendFirstRow;   // the history page's rows; each told event's first row
    int legendSel = 0, legendTop = 0;            // the event under the cursor (of `told.events`), the first row shown
    struct { int dir = 0; double t = 0, acc = 0; } shardRun;   // R-416: Shift with an arrow held runs the timeline's and the history's cursor
    int shardRunSteps(const Input& in, double realDt, bool leftRight);   // the steps it takes this frame (back negative); 0 without Shift
    std::vector<double> eraX0, eraX1;            // the timeline's eras across its width (each era's share by the people's records in it)
    void ensureTold();                           // `told`, the history page's rows and the timeline's eras for the open world (no-op while valid)
    bool shardInFilter(int local) const;         // a record of the kind the records' page shows
    void stepShard(int dir, bool stop = false);  // the cursor to the next record held of the kind shown (stop: a single step stops at the ends too)
    int toldRecordOf(int item) const;            // the record (of `shardHeld`'s order) that told an event first, -1 none held
    void renderShardRecords(); void renderShardTimeline(); void renderShardHistory(); void drawShardPagesStrip(const std::string& right);
    std::string recordSummary(int local, int maxChars) const;   // a record as the list names it ("A LETTER: ...", a piece's form, a chart's caption)

    std::vector<std::string> shardWorldKeys, shardWorldLabels; std::vector<int> shardWorldWhich;   // C-13: an entry a people whose shards are held, labelled with the people's name where the world had two
    std::vector<int> shardWorldTotals;           // T-04: how many records each entry's people left (`recordCountOf`)
    std::vector<int> shardHeld;                  // the open world's shards held, in the order of their years
    size_t shardsListed = 0;                     // C-18: the guide's shards and lent ones when the lists were made (the drones bring more while they are open)
    void listShardWorlds();                      // C-18: `shardWorldKeys` and the rest from the guide, the cursor kept on its world
    void listShardsHeld();                       // C-18: `shardHeld` of the open world, the cursor kept on its record
    int shardsLevel = 0, shardWorldSel = 0, shardSel = 0;
    std::vector<DecodedWord> shardWords;         // the shard on the screen as the computer reads it
    // H-04: a long text in pages (the words' places on the screen and where each page starts); the voice speaks the page on the
    // screen (the opening first), from `speechFrom` (the first of its words) to `speechTo`
    struct WordAt { int page = 0, x = 0, y = 0, w = 0; };
    std::vector<WordAt> shardLayout; std::vector<int> shardPageStart; int shardPage = 0, speechFrom = 0, speechTo = 0;
    void layoutShardText(int x0, int y0, int maxX, int maxY);
    void speakShardPage(int page);
    bool shardFresh = false;                     // C-05: a first reading at this share: the words resolve as the recording speaks them, and its end reads the shard
    double speechT = -1, speechSyncT = -1; bool speechPaused = false;   // C-05: the game's own clock into the speech (synced to the synth's when it runs), paused
    void startSpeech(); void stopSpeech(); bool speechEnded() const; void markShardRead();
    // H-05: what a record was made on, named and drawn on the decoder (game/medium_draw.cpp): the line under the date (a recording names its
    // voice, a record with no sound of its own says the ship reads or plays it) and the object at the right of it, turning while it plays
    std::string mediumLine(const Shard& s) const;
    void drawMediumPanel(int x0, int y0, const Shard& s, bool playing, double t);
    // C-04 a piece on the screen: the game's own clock into it (synced to the synth's beat when the synth runs), paused, the shard being named
    double pieceT = -1, pieceSyncBeat = -1, humBeforePiece = -1; bool piecePaused = false;
    std::string textShardKey;
    std::string textStormKey;   // X-04: the storm a text entry of kind 9 names
    void startPiece(); void stopPiece(); double pieceBeatNow() const; bool pieceEnded() const;
    bool openShardWorld(const std::string& key, int which = 0);
    bool buildShardWorld(ShardWorld& w, const Star& s, int bi, int which = 0, bool wait = true);   // C-07: a world's shards, language, voice and tradition, for the decoder and the radar alike (H-04: `wait` false: not until its records are written, on a thread)
    void prefetchShardWorld(int sel);            // H-04: the decoder's worlds list asks for the selected world's records early
    // C-10 the star charts (galaxy/charts.h): a chart shard draws itself on the decoder (a first reading at this share over
    // CHART_READ_S seconds, the stars, then the figures, then their names), Enter targets the star it marks, O holds it up to the
    // sky: the figures' lines are drawn between the real stars on the space view and the surface's night sky from wherever the
    // ship is, with the marked star's diamond, until it is put down
    static constexpr double CHART_READ_S = 4.0;
    double chartT = -1;                              // the reading's clock (-1 none)
    StarChart chartHeld; bool chartUp = false; std::string chartHeldKey, chartHeldPeople; int chartHeldIndex = -1;
    std::vector<std::string> chartHeldLabels;        // the figures' names as the computer read them when the chart was held up
    int chartLinesDrawn = 0;                         // the figures' lines on the screen last frame (the harness)
    void renderChart();
    void drawChartOverlay(const Mat3& cam, const Vec3& obs, const Proj& pj, const Mat3* localFrame);   // `localFrame`: on the surface, to keep the lines above the horizon
    std::string figureLabel(const std::string& name, int held) const;   // "the hunter" as the computer reads it with that many shards of the world held
    void holdChart(bool up);
    // C-07 the signal radar (game/radar.cpp): switched on in space, the receiver follows the view; the far signals are scanned
    // once per sector of the ship, the system's own every frame; the meter reads the beam's gains over the static, a hold within
    // three degrees locks, Enter on a lock targets the source
    struct RadarState {
        bool on = false;
        std::vector<Signal> far, local, all;    // the transmitters and loud pulsars within reach of the scan's sector; the system's own; both with this frame's directions
        std::vector<double> gain, angle;        // per entry of `all`: the beam's gain times the strength, and the angle off the beam
        int64_t scanSx = 1 << 30, scanSy = 0, scanSz = 0; int scanCells = 0; double scanMs = 0;
        double reading = 0, noiseWalk = 0, traceAccum = 0;
        std::vector<float> trace;               // the meter's last four seconds, for the scope
        int best = -1; double bestGain = 0, bestAngle = 0;
        double holdT = 0, lockSecs = 3;         // the hold within the lock's cone, and what it needs
        int locked = -1; uint64_t lockedSeed = 0; int lockedKind = -1; double lostT = 0;
        double clarity = 0;                     // how much of the signal comes through the static (the synth's radarSignal)
        ShardWorld world; uint64_t worldSeed = 0; int worldWhich = -1;   // the people's world in the beam, built once (C-13: once per people on the air)
        uint64_t pendingSeed = 0; double pendingAt = -10;               // H-04: a world whose records are being written (the carrier alone until they are)
        int shard = -1; double contentT = -1, contentSecs = 0; bool contentMusic = false; double awayT = 0;
        Programme prog; int loopI = 0; double loopAt = 0;   // the programme on the air (galaxy/signals.h), the repeat a loop or a numbers station is at and when it began
        Rng rng{0x5161};
        // C-15 the gain: the dial (SIGNAL_GAINS, x1 to x32; kept while the radar is off, not saved), what it hears beyond 200 ly from the
        // scan's sector (the whole dial's at once, each by its `needGain`), found a slice a frame by `deep` (LISTENING DEEPER meanwhile)
        int gainStep = 0;
        std::vector<Signal> deep; SignalScan deepScan; bool deepRunning = false, deepReady = false; double deepMs = 0; int deepFrames = 0;
        std::vector<double> level;              // per entry of `all`: the level at the beam's centre (C-07's strength at x1; 1 and over overloads)
        int aim = -1; double aimAngle = 0;      // the loudest within the lock cone: the one the hold is on
        bool overload = false;                  // the aim overloads the receiver: the meter pinned, the hold refused
        double sum = 0, floorNow = 0;           // the beam's sum over the signals and the static's floor this frame (the harness)
        // C-16 what the gain lets in: the age of the slot's people on the air, the band's brightness in the beam (the galaxy's own
        // noise), the signal under the one heard (an index into `all`: the loudest other in the wide lobe), the rolling band's phase
        int progAge = -1; double sky = 0; int under = -1; double rollPh = 0;
        // a false rise: the noise's ghost in a direction near the beam at the top steps, living some seconds; held, the hold fills
        // part way (`holdMax` of it) and empties (NOTHING THERE for `nothingT`); never a lock
        struct FalseRise { bool on = false, spent = false, held = false; Vec3 dir; double t = 0, life = 8, peak = 0.2, holdMax = 0.6; };
        FalseRise rise; double riseNext = 20, riseGain = 0, riseAngle = PI, nothingT = 0; int rises = 0, riseHolds = 0, nothings = 0;
        // a word of a recording heard before (`Guide::heard`) coming back out of the static at the top steps while nothing is on the
        // air: its world (when it is not the radar's), the word spoken in the world's voice, when the next may come
        ShardWorld wordWorld; Speech wordSpeech; Voice wordVoice; double wordNext = 30, wordT = -1; int words = 0; std::string wordKey, wordSaid;
    };
    RadarState radar;
    void radarToggle(); void radarOff(); void radarScan(); bool updateRadar(const Input& in, double realDt);
    void radarGainStep(int dir);        // C-15: the dial a step up or down
    void radarDeepStep();               // C-15: a slice of the far scan (begun when the gain is up and the list is not this sector's)
    double radarSkyAt(const Vec3& dir) const;   // C-16: the band's brightness round a direction (the beam's width), 0..1
    void radarRiseStep(const Vec3& fwd, double realDt);   // C-16: a false rise born, aged and gone
    void radarWordStep(double realDt); void radarWordStop();   // C-16: a heard word back out of the static
    double radarGain() const { return SIGNAL_GAINS[std::max(0, std::min(SIGNAL_GAIN_STEPS - 1, radar.gainStep))]; }
    void renderRadarCamera();           // the camera's frame over the picture: the scope, the cone, the readout
    void radarCameraFeed(Framebuffer& fb);   // the picture as the radar camera gives it (the palette cold, static by the clarity), before the mush
    void radarLock(int idx); void radarAccept(); void radarStartContent(const Signal& s, bool next); void radarRepeatContent(); void radarStopContent();
    int shardsHeldOf(const std::string& key, int which = 0) const;   // C-13: of that people's fifty (C-14: the explorer's own and the lent)
    int lentOf(const std::string& key, int which = 0) const;         // C-14: how many of that people's fifty are lent and not taken
    bool shardLent(int local) const;                                 // C-14: the open people's shard is a friend's (lent by the inbox, not taken since)
    const std::string* shardNameOf(const std::string& key, bool& foreign) const;   // C-14: a piece's name, the explorer's own or the inbox's (`foreign`); null when none
    std::string heldShardKey(int local) const;      // C-13: the guide's key of the open people's shard (its world index)
    std::string endedKey() const;                   // C-13: the open people's record in `Guide::ended` (the second people's under "/P1")
    std::string shardWorldTitle(int maxName) const; // C-13: the world's name, and the people's where the world had two
    std::string worldNameOfKey(const std::string& key) const;
    void openShards(); void openShard(int sel); void updateShards(const Input& in, double realDt); void renderShards(); void renderPiece();
    // C-11 the ship's player (game/player.cpp): the pieces and the recordings held and the recordings the radar caught (`Guide::heard`), listed
    // on the decoder's player page (Tab at its worlds) and played on wherever the ship goes, a line on the space HUD, until a landing. Held
    // while the decoder, the radar or the gramophone has the synth (its place kept) and taken up where it was; one after another down the list
    struct PlayerItem {
        std::string world; int which = 0, local = 0;   // the guide's world key, the people, its record
        bool music = false, heard = false;             // a piece; caught by the radar (and not held as a recording)
        int medium = -1, year = 0, form = -1;          // what it was made on; its year; a piece's form
        double seconds = 0;                            // a piece's length (a recording's once built)
        std::string people;                            // "THE KHOSA"
    };
    struct ShipPlayer {
        std::vector<PlayerItem> items; int sel = 0, top = 0;
        int playing = -1;                              // the item playing (of `items`), -1 none
        bool paused = false, held = false, waiting = false;   // paused by the explorer; held while another has the synth; its world's records still being written
        ShardWorld world;                              // the playing item's world: the synth's piece, tradition, speech and voice point into it
        double t = 0, secs = 0, syncT = -1;            // the game's clock into it (synced to the synth's while it runs), its length
        std::string title, people;                     // the HUD's ("A LULLABY", "THE KHOSA": the people left out where the line has no room)
        int finished = 0;                              // items played to their end since the game began (the harness)
        int guideVersion = -1;                         // the list's (rebuilt when the guide's shards change)
        std::map<std::string, std::pair<int, double>> pieceInfo;   // a piece's form and length by its key, worked out once
    };
    ShipPlayer player;
    int shardsTop = 0;                                 // the decoder's first level: 0 the worlds, 1 the player
    void playerBuildList();
    void playerPlay(int item);                         // from its start
    void playerStart();                                // the playing item onto the synth from `player.t`
    void playerStop();
    bool playerOwns() const;                           // the synth's piece or speech is the player's
    void updatePlayer(double realDt);                  // every frame: the landing stops it, another's sound holds it, its end moves it on
    void renderPlayerPage();
    std::string playerHudLine() const;
    static std::string playerItemKey(const PlayerItem& it);                            // its guide key ("<body>/S<index>")
    static std::string playerItemTitle(const PlayerItem& it, const std::string* name);  // "A LULLABY", "'THE RIVER'", "A WAX CYLINDER", "AIR: A RECORDING"
    // C-11 the instruments' case (game/instruments_screen.cpp), on the cabin's left wall: the instruments taken (`Guide::instruments`), the one
    // on the screen with its people's tradition (the synth's timbre points into it) and colours; a key a degree of its scale (1 to 9, 0),
    // Left/Right its register; the gramophone's disc through its horn
    struct InstrumentCase {
        // C-18: the case opens on its worlds, a folder each as the decoder's (the user's: "you open specific planet as a folder"), and a
        // world's instruments inside it: Enter opens the world, Esc closes it. A world of two peoples a folder each people, as the decoder
        // lists them (the user's: "split instruments by civilization as a decoder does")
        int level = 0;                                 // 0 the worlds, 1 the open world's instruments
        std::vector<std::string> worlds;               // each folder's world ("<body key>"), the one under the ship selected
        std::vector<int> worldWhich;                   // and its people (0, or 1 the second of a world of two)
        std::vector<int> worldCounts;                  // how many of each the case holds
        std::vector<std::string> worldPeoples;         // whose they are on a world of two ("THE GREIP"), else ""
        int worldSel = 0;
        std::vector<std::string> keys; int sel = 0;    // the open world's instruments: the guide's keys, by id
        std::vector<std::string> names;                // each one's name ("A LUTE")
        std::string key; bool valid = false;           // the one on the screen
        Tradition tradition; PeopleInstruments set; Instrument inst; FormPalette palette; std::string people;
        int regShift = 0;
        bool held[10] = {};                            // its keys down (1..9, 0)
        std::vector<double> glow;                      // per course of the picture: a note just played, fading
        struct Played { int degree = 0; double t = 0; };
        std::vector<Played> played;                    // the last notes (the roll under the picture)
        int notes = 0;                                 // the notes played since the case opened (the harness)
        Piece disc; bool discOn = false; double discT = -1;   // the gramophone's disc playing (its clock, the game's own)
        double humBefore = -1;
    };
    InstrumentCase icase;
    void openInstruments(); bool loadInstrument(int sel); void updateInstruments(const Input& in, double realDt); void renderInstruments(); void stopInstrument();
    void openCaseWorld(int w);                         // C-18: a world's instruments onto the case's list (the first on the screen); a people's of a world of two
    void renderCaseWorlds();                           // C-18: the case's worlds
    // C-18 the drone bay (game/drones.cpp, game/drone_draw.cpp): three drones with hands on a rack against the left wall by the front window
    // (the cabin's `facing` 11, its screen `GameState::DRONES`, the devices page's row 10). Parked at a people's world the bay sends them
    // down; each brings two to eight of the world's records (texts, music, star charts) and instruments the explorer does not hold, from
    // the places that keep them (galaxy/drones.h), into the guide as it docks. The trip and the bay's charge run on real time (the warp does
    // not hurry them): two minutes searching, five minutes charging from their return. Leaving the parking calls them back with what they
    // found so far. A trip is saved and resumed (the `bay` lines)
    struct DroneFindRec { int kind = 0, index = 0, people = 0, medium = -1, place = -1; double t = 0; std::string name; bool aboard = false; };
    struct DroneStopRec { int place = -1, sclass = -1; double lat = 0, lon = 0; double arrive = 0, leave = 0; };
    struct DroneRun {
        double out = 0;                                // the clock it leaves the rack at
        double down = 0, up = 0, dock = 0;             // it reaches its first stop, leaves its last, is back on its rack
        bool docked = true;
        int wanted = 0;                                // what it set out to bring (2 to 8)
        std::vector<DroneStopRec> stops;
        std::vector<DroneFindRec> finds;               // in the order found
    };
    struct DroneBay {
        double clock = -1;                             // real seconds since the last launch (-1: none since the bay was new)
        bool out = false;                              // a trip on: from the launch until the last drone docks
        bool planned = false;                          // its haul drawn (the world's items read)
        bool recalled = false;                         // the last trip was called back
        uint64_t starSeed = 0; int body = -1; std::string worldKey, worldName;
        uint64_t seed = 0;
        double back = 0;                               // the clock when the last trip ended (its last dock, or the recall): the charge runs from it
        double subLat = 0, subLon = 0;                 // the ground under the ship at the launch (radians)
        DroneRun run[DRONE_COUNT];
        // the parked world's items (`droneWorldOf`, read on the thread below)
        DroneWorld world; bool worldValid = false; uint64_t worldStar = 0; int worldBody = -1;
        // the screen: the world's map (its key), the finds list's scroll
        std::vector<uint32_t> map; uint64_t mapSeed = 0; int mapW = 0, mapH = 0;
        int listTop = 0;
    };
    DroneBay bay;
    std::thread droneJob; std::atomic<bool> droneJobBusy{false}; DroneWorld droneJobOut; uint64_t droneJobStar = 0; int droneJobBody = -1;
    void openDroneBay(); void updateDroneBay(const Input& in, double realDt); void renderDroneBay();
    void droneBayLaunch();                             // the drones sent down (or the refusal's status)
    void updateDrones(double realDt);                  // every simulating frame: the thread's result, the clock, the docks, the recall
    void droneWorldAsk(int body);                      // the world's items asked for on the thread (no-op when held or on its way)
    void droneCollect();                               // the thread's result taken (once it is done)
    bool droneWorldHere(uint64_t starSeed, int body) const { return bay.worldValid && bay.worldStar == starSeed && bay.worldBody == body; }
    void dronePlanTrip();                              // the haul drawn and each drone's clock laid out (the world's items read)
    void droneDock(int k);                             // a drone back on its rack: its finds into the guide
    void droneRecall();                                // the ship left the parking: the drones aboard with what they had found
    void droneTripEnd();                               // the last docked: the log, the status
    void droneFindsSeen();                             // R-414: the rack's screen closed: a trip back aboard leaves no list behind
    bool droneBayHere() const;                         // the ship is parked at the trip's world
    bool droneBayReady() const;                        // not out and charged (five minutes from the last trip's end)
    double droneCooldownLeft() const;
    double droneBackIn() const;                        // seconds until the last of them docks (-1: not out, or its trip not laid out yet)
    std::string droneFindKey(const DroneFindRec& f) const;
    std::string droneFindLabel(const DroneFindRec& f) const;   // "A CLAY TABLET", "A WAX CYLINDER", "A SHELL PIPE"
    std::string droneStateLine(int k) const;           // "DESCENDING 4,200 KM", "SEARCHING A CITY", "ABOARD"
    bool dronePosition(int k, double& lat, double& lon) const;   // where on the world the drone is (false: aboard or in the bay's hatch)
    DronePose dronePoseAt(int k) const;                // its arms, its hands, its fans now
    bool droneRackPlace(int k, double& x, double& lift, double& blur) const;   // where it stands in the cabin (its shelf, on its way through the rack's port), its lift and its fans' blur; false while away
    void drawDroneRack(const Mat3& look, const Vec3& eye);   // the rack and the drones on it, in the cabin (`drawCabin`)
    void drawDroneLights();                            // the drones leaving the ship and coming back, as lights in the space view
    void drawDroneMesh(const Mat3& look, const Vec3& eye, const Vec3& origin, const Vec3& fwd, const Vec3& up, const DronePose& pose, const Vec3* sun);   // a drone's model in the cabin's frame
    void drawDroneWire(int x0, int y0, int w, int h, const DronePose& pose, double yaw, uint32_t col);   // a drone in lines on the screen
    void buildDroneMap(int body);                      // the screen's map of a world of this system
    void playInstrumentDegree(int degree, int key);    // a note of the one on the screen (`key` holds it; 0 a note of its own length)
    void drawInstrumentPicture(int x0, int y0, int w, int h, const Instrument& I, const Tradition& T, const FormPalette& pal, const std::vector<double>& glow, double t);   // C-17: its parts (`drawInstrumentParts`)
    void deployCapsule(); void toggleVimana();
    // M4-02 sector map, M4-10 vision modes
    int visionMode = 0;
    bool readings = false;   // R-414: the readings over the surface's view (Tab): the place, the sky, the air, the ground, the vehicle
    void drawReadings();
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
    void keepFlightBand();   // G-07: the band's keyframes built on while the galaxy page covers a flight
    void renderSpaceHUD();
    void updateLandingMap(const Input& in, double dt);
    void renderLandingMap();
    void updateDescentAscent(const Input& in, double dt, double realDt);
    void updateSurface(const Input& in, double dt, double realDt);
    // W-03: the forecast at the feet: the next fronts within five days (`frontsAhead`), listed again every thirty seconds of game time
    std::vector<FrontForecast> forecast; double forecastT = -1e18, forecastReal = -1e18; int forecastBody = -1;
    void updateForecast();
    std::string forecastLine() const;   // "RAIN FROM THE SW IN 2 H 05 MIN", "IN A FRONT, CLEARING IN 1 H 05 MIN", "NO FRONT WITHIN 5 DAYS"; "" on a world without fronts
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
    int dataPage = 0;                     // X-04: the data sheet's page (0 the sheet, 1.. a giant's probes' profiles)
    std::vector<int> dataProbes() const;  // the records of the sheet's giant
    void renderProbeProfile(int rec);     // X-04: a probe's profile on its giant's data sheet
    void drawVisor(uint32_t col);
    void drawCommonHUD(bool warp = true);   // X-04: a recording's screen leaves out the game's warp
    void arriveAtStar(const Star& s, const Vec3& fromDir);
    void beginLanding();
    void lockRemoteTarget();
    void nearestStars(int n, std::vector<const Star*>& out) const;
    void cycleTargetStar();
    void buildLandingMapBase();
    int pickBodyNearCrosshair();
    void startApproach(int body);
    // R-407: a jump of any length along the aim: in the aim (R) the digits typed make the distance in light years (`jumpDigits`,
    // to JUMP_MAX_LY), Enter jumps: the star nearest the line's end (galaxy/starfield.h's `jumpTarget`) becomes the remote target
    // and the Vimana flies at once. The console's JUMP runs one along the ship's nose
    std::string jumpDigits;
    Vec3 aimDirection() const;                     // the crosshair's direction in the world (the view's forward)
    void startJump(double ly, const Vec3& dir);
    // G-06: a jump plotted before it is flown (the galaxy chart's Enter plots, its second Enter flies; the aim and the console's JUMP do
    // both at once): the star nearest the line's end made the remote target, the line remembered for the log (its length and way, how
    // far short of its end the void sent it, where from, whether the chart plotted it). The flight's start logs the JUMP line
    // (`toggleVimana`, so V flies a plotted target the same way); any other target forgets it (`setRemoteStar`)
    struct JumpPlot { uint64_t seed = 0; double ly = 0, shortLy = 0; std::string way, from; bool onChart = false; } jumpPlot;
    bool plotJump(double ly, const Vec3& dir, bool onChart);   // false when refused (in a flight, no star along the line, the current star)
    void flyJump();                                            // the plotted jump flown (the Vimana and its status)
    std::string aimStarLabel(const Star& full) const;   // a marker's label in the aim (its name once it has one, else the distance; VISITED)
    std::vector<std::string> worldNamesAt(const std::string& starKey) const;   // the names given to a star's worlds (the star map's card)
    // O3 (R-302): belts as destinations
    bool pickBeltRock(int k, double at, int64_t& ir, int64_t& ia, int64_t& iy, int& m) const;   // the biggest rock near the ship's angle round the star
    bool beltRockNow(BeltRock& rk) const;          // the target's or the parking place's rock at the current time
    Vec3 beltParkPos(double at) const;             // the parking point beside that rock
    void startApproachBelt(int k);
    std::string localTargetLabel() const;          // the body's or the belt's name
    void choosePaletteBodies(int& a, int& b);
    std::string epocString() const;
    static std::string epocOf(double secs);   // X-04: a time's (a recording's)
    std::string epocFullString() const;   // M5-05: EPOC:SINISTER.MEDIUS.DEXTER
    static double wallClockT();            // M5-05: game seconds from the wall clock (epoch 2026-01-01 UTC)
    // W-01 the almanac (game/almanac.cpp): Ctrl+T, the data sheet's T and the flight computer's devices page open the list
    // of the next events at the place (the feet, the landing map's cursor, the ship's parking) with their countdowns and a
    // free row; Enter runs the clock to one, the warp falling as the moment nears (`almanacWarpFor`) until the clock lands
    // on it. M5-06's 25 s time-lapse went with it (the free row runs for any span)
    AlmanacPlace almanacAt; std::vector<AlmanacEvent> almanacEvents; double almanacBuiltT = -1e18;
    int almanacScene = 0;                  // 0 the ship, 1 the ground under the feet, 2 the landing map's cursor
    GameState almanacReturn = GameState::SPACE;
    int almanacSel = 0, almanacScroll = 0, almanacFree = 6;
    bool almanacRunning = false; double almanacRunT = 0; AlmanacEvent almanacRunEvent; std::string almanacRunLabel;
    void openAlmanac(); void buildAlmanac(); void updateAlmanac(const Input& in); void renderAlmanac();
    void almanacRunTo(const AlmanacEvent& e); void almanacStop(bool arrived);
    static double almanacWarpFor(double remaining);
    std::string almanacLabelOf(const AlmanacEvent& e) const;
    std::string almanacStateLine() const;
    // W-06 the telescope (game/telescope.cpp): Z in space (the devices page too) magnifies the ship's own view by powers of two,
    // x2 to x1024 (the focal length multiplied, the cabin left behind as in the radar camera), the mouse and the arrows steering
    // at a rate that falls with the magnification; the reticle names what it rests on (a body, the sun, a star of the
    // neighbourhood), Enter tracks a body (the ship re-aimed at it every frame, the mouse moving the offset), N names it, P
    // photographs it with the target in the caption. Not saved
    enum { TELE_NONE = -1, TELE_SUN = -2, TELE_STAR = -3 };
    struct TelescopeState { bool on = false; int step = 1; int track = TELE_NONE; double offYaw = 0, offPitch = 0; int target = TELE_NONE; Star targetStar; double targetRangeKm = 0, targetWidth = 0;
                            int frameBody = -1; Vec3 parkBF, viewBF; double heldYaw = 0, heldPitch = 0; };   // R-405, the stabiliser: the parked body whose frame holds the ship's parking and the view's axis (body-frame unit vectors; the attitude last set, so a turn from elsewhere is taken up); -1 when not parked at a body
    TelescopeState tele;
    static constexpr int TELE_MAX_STEP = 10;   // x1024
    double telescopeZoom() const { return tele.on ? (double)(1 << tele.step) : 1.0; }
    Proj telescopeProj() const; double telescopeFieldDeg() const;
    void telescopeToggle(); void telescopeOff(); void telescopeZoomStep(int dir); void telescopeTrack(int id, bool recentre = true); void telescopeAim(); void telescopeName();
    void telescopeHoldFrame(); void telescopeHoldView(); bool telescopeStabilised() const;   // R-405: the stabiliser (the parking and the view held in the parked body's frame while the eye is at the eyepiece)
    void orbitShip(double east, double north, double dt);   // R-405: Shift+arrows carry the parked ship round its world, the window keeping the world where it was
    void updateTelescope(const Input& in, double realDt); void telescopeFindTarget(); void renderTelescopeHUD();
    std::string telescopeTargetName(int id) const; std::string telescopeTargetLine(bool plain = false) const;   // `plain`: for a text file (DEG for the glyph)
    Star textStar;   // the star a text entry of kind 8 names (one seen through the telescope)
    // the user's review of W-06: the eyepiece shows the planet function at its own footprint through the renderer's plate
    // (`DetailPlate`, space_view.h), built for the body the eyepiece rests on within a budget of samples a frame
    static constexpr int TELE_PLATE_BUDGET = 8000;
    bool telePlateFinish = false;   // the next space frame reads the plate out whole (a photo, the harness)
    int telescopePlateBody() const; // the tracked body, else the one under the reticle, else -1
    // X-01 the probe (game/probe.cpp): C at a giant (the landing screen in the cabin, the flight computer's deploy row) launches a
    // probe to the ground under the telescope's reticle, else under the ship; the screen is its camera (`GameState::PROBE`, CAM 05):
    // the fall from the ship, the entry, the descent through the decks (galaxy/probe.h's clock, which runs on real time: the warp
    // does not hurry it) with its readouts, the chute (Space, once), photographs, and the end: the last frame held under the
    // loss-of-signal hiss until the screen is left. The ship holds its parking meanwhile: leaving it cuts the relay. Unlimited
    // probes (the user's answer at X-01's start), one falling at a time; a falling probe is saved and resumed
    struct ProbeState {
        bool active = false;                 // launched and not let go (falling, or its last frame held)
        int body = -1; uint64_t starSeed = 0;
        double aimLat = 0, aimLon = 0;       // the entry point (radians, the body's frame)
        double clock = 0;                    // real seconds since the launch (the relay's clock)
        double launchT = 0;                  // the game time at the launch
        Vec3 startRel;                       // the ship's place at the launch from the giant's centre (km)
        ProbeFlight flight;                  // the chute's clocks, the hull's limit
        GiantAtmosphere atm;
        uint64_t seed = 0;
        double headYaw = 0, descYaw = 0;     // local azimuths: the heading it comes in along (the fall's camera), the descent's camera (the sun's quarter)
        double camYaw = 0, camPitch = 0;     // the camera's turn from its default (the mouse, the arrows)
        double endClock = -1, endT = 0; int endCause = 0;   // the end: its clock, its game time, 1 the hull, 2 the heat
        double endBar = 0, endKm = 0, endK = 0;
        std::vector<uint32_t> lastFrame;     // the frame held at the end
        int lastFlashSlot = -1;              // the lightning's last slot heard (the thunder's trigger)
        bool tornNoted = false;              // the chute's tearing announced
        bool fast = false;                   // R-406: Shift held on the probe's screen this frame (set in Game::frame before the clock runs): eight times as fast
        GiantBands bands;                    // X-02: the globe's bands round the aim at the launch (built at the launch and after a load)
        GiantDecks decks;                    // X-02: the decks' fields (its bands pointer refreshed before every use)
        bool holePath = false;               // X-03: this descent falls through a clear-air hole (the decks' `path` holes over its way)
        double sightClock = -1;              // X-03: on the rare giant, the clock the sight passes the lamp at (-1 none)
        // X-04: what comes back (the record, galaxy-side numbers above, is kept in the guide at the end or the cut)
        int id = 0;                          // its number (PROBE 3)
        std::vector<double> camTrack;        // the explorer's turns of its camera where they changed (clock, yaw, pitch)
        std::vector<double> timeTrack;       // the game's time along the fall where its pace changed (clock, t)
        double prevClock = -1, prevT = 0, prevPace = -1;            // the clock's track: the last frame's, the pace of the segment from the last key (-1 not yet)
        double prevCamClock = 0, prevYaw = 0, prevPitch = 0;        // the camera's: the last frame's
        double prevHoleClock = 0;                                   // the clock the hole was looked for up to
        unsigned notes = 0;                  // the discoveries logged (bits: the storm, its wall, the aurora, a hole, hail)
        int stormKind = 0; std::string stormKey;   // the storm under the aim (1 one of the globe's, 2 the great storm) and its key
    };
    ProbeState probe;
    void probeTrack();                       // X-04: every frame the probe falls: the tracks, the discoveries for the log
    void probeKeep(int cause);               // X-04: at the end or the cut: the record (the profile, the final image) into the guide
    void probeProfileOf(ProbeRecord& r) const;   // the profile and the layers the probe found, down to the record's end
    double probeLightAt(double clock, double tt) const;   // the light at the probe (of the sunlight over the clouds at noon)
    int probeLayerAt(double clock) const;    // the layer the probe found itself in at a clock (galaxy/probe.h's `giantFoundLayer`)
    std::string probeStormName() const;      // the storm's name, else "" (the great storm, a storm: the explorer's or the inbox's)
    std::string nameOfKey(const std::string& key) const;   // a key's name (yours, else the inbox's), else UNKNOWN: a world of another system
    // X-04: a probe's recording on the screen (`GameState::RECORDING`, opened from the gallery): the descent drawn again from the
    // record's numbers by the probe's own functions, the record's probe (and another star's system and sky) swapped in for the call
    struct ReplayState {
        int rec = -1;                        // the record's index in guide.probes
        ProbeState p;
        bool here = true;                    // its giant is of the system the ship is in (else `sys` and `nb` below)
        StarSystem sys; StarNeighborhood nb;
        double clock = 0; bool playing = true, fast = false;   // fast: Shift held, eight times as fast
        double lookYaw = 0, lookPitch = 0;   // the viewer's own turn over the recording's
        std::vector<uint32_t> last;          // its last frame (drawn once)
    };
    ReplayState replay;
    template <class F> void asRecording(F f);                // the call with the recording's probe, system and sky in place
    bool recordingContext(int rec);          // the record's probe, its system when it is another star's: false when it cannot be drawn
    bool openRecording(int rec);             // the screen, from the start
    void updateRecording(const Input& in, double realDt);
    void renderRecording(); void renderRecordingHUD(double tt);
    void recordingAudio();
    void recordingPoster(int rec, std::vector<uint32_t>& out);   // its last frame (a lent recording's gallery picture)
    double recordTimeAt(const ProbeRecord& r, double clock) const;
    void recordCamAt(const ProbeRecord& r, double clock, double& yaw, double& pitch) const;
    double recordDepthU(const ProbeRecord& r, double clock) const;   // the depth bar's place of a clock (0 the launch .. 1 the end)
    double recordClockAtU(const ProbeRecord& r, double u) const;
    std::string recordTitle(const ProbeRecord& r) const;   // "PROBE 3 INTO <giant>"
    std::string recordLine(const ProbeRecord& r) const;    // the date, the depth and the end
    bool probeAimPoint(double& lat, double& lon) const;   // the telescope's reticle on the parked giant, else the ground under the ship
    bool viewGround(int body, double& lat, double& lon) const;   // the body's ground under the view's centre (W-06's reticle), false when the ray misses
    void launchProbe(); void watchProbe(); void probeCut(const char* why); void probeEnd(); void probeRelease();
    void updateProbe(double realDt);                       // every simulating frame: the clock, the chute's tearing, the end, the relay's hold
    void updateProbeScreen(const Input& in, double dt, double realDt);
    void renderProbe();
    void renderProbeHUD(const ProbeRecord* rec = nullptr, double clock = 0, double tt = 0);   // X-04: a recording's (`rec`: at its clock and time)
    void probeHiss();                                      // the last frame's hiss over rgbBuf (the end held on the screen)
    void probeSound(double clock, bool watching, bool ended);   // X-04: the relay's levels at a clock (the live probe's, a recording's)
    void renderProbeFeed(double clock, double tt);         // the probe's picture at a clock and a game time into rgbBuf
    void probeAudio();                                     // the relay's sound levels for this frame
    bool probeHere() const;                                // the ship is parked at the probe's giant (the relay holds)
    Vec3 probeWorldPos(double clock, double tt, Vec3& up) const;
    std::string probeCaption() const;
    void probeEnsureClouds();                // X-02: the bands and the decks of a probe launched or loaded
    double probeDeckDrift(int k, double clock) const;   // X-02: km a deck has slid east under the probe since the descent began
    bool probeStormWall(double& dist, double& ex, double& ez) const;   // X-03: the great storm's wall within sight of the aim
    bool probeAuroraWay(double& ex, double& ez, double& p0) const;     // X-03: the way to the nearer magnetic pole, the oval's distance
    std::string probeStageLabel(double clock) const;                  // X-03: the screen's stage name (a deck by its cloud)
    std::string distanceString(double km) const;
    void handleGlobalKeys(const Input& in);
    Vec3 shipWorldPos() const { return ship.pos; }
};
