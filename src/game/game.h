// Game: the one class of the game (game.cpp's `frame` runs it). Its types are game_types.h, its subsystems' state the
// *_state.h headers beside it; the harness reaches it through src/harness/test_game.h's TestGame. A function's comment is at
// its definition. The sections, each under a `// ----` banner naming its files, in order:
//   public (the platform's and the harness's): the frame; photos; the attract mode; settings and keys; the controller; auto-explore;
//     the tape; the files, the guide, names; marks and the scanner; what the ground holds; the window and sound; saves; the state
//     and the clock; the galaxy chart's frame; the harness's knobs
//   protected: the world and the ship; the landing map; the screens; the controller; the slots; the guide's screens; the galaxy
//     chart; the gallery; the cabin; the shard decoder; star charts; the radar; the decoder's records; the player; the case; the
//     drone bay; the sector map; the guide's targets; each state's update and render; jumps; belts; the epoch; the almanac; the
//     telescope; the probe and its recordings; the global keys; auto-explore; the tape
#pragma once
#include "core/framebuffer.h"
#include "core/font.h"
#include "core/input.h"
#include "galaxy/starfield.h"
#include "galaxy/drifters.h"
#include "galaxy/system.h"
#include "peoples/music.h"
#include "peoples/voice.h"
#include "peoples/signals.h"
#include "peoples/charts.h"
#include "galaxy/almanac.h"
#include "galaxy/fronts.h"
#include "galaxy/probe.h"
#include "peoples/legends.h"
#include "peoples/drones.h"
#include "space/space_view.h"
#include "surface/surface_view.h"
#include "audio.h"
#include "settings.h"
#include "guide.h"
#include "game_types.h"
#include "landing_state.h"
#include "pad_state.h"
#include "chart_state.h"
#include "shard_world.h"
#include "radar_state.h"
#include "player_state.h"
#include "case_state.h"
#include "drone_state.h"
#include "telescope_state.h"
#include "probe_state.h"
#include "autoexplore_state.h"
#include "tape_state.h"
#include <string>
#include <vector>
#include <set>
#include <deque>
#include <functional>
#include <cmath>
#include <atomic>
#include <thread>

class Game {
public:
    // ---- the frame, and what the platform reads and asks (game.cpp, main_raylib.cpp)
    Game();
    ~Game();   // R-408: joins the landmark names' migration if one is running
    void frame(const Input& in, double realDt);
    const uint32_t* output() const { return rgbBuf.data(); }
    bool wantsQuit = false;
    bool wantsScreenshot = false;
    bool wantsFullscreenToggle = false;
    // ---- photos: photo mode, the three-view panorama, the frame recorder (game.cpp)
    bool photoMode = false;
    bool wantsPanorama = false;
    bool recording = false;
    int recTake = 0, recFrame = 0;
    std::string moviesDir = "movies";
    bool recFixedTake = false;        // tests: always take 1, frames overwritten
    void renderPanorama(std::vector<uint32_t>& out, int& w, int& h);
    bool recordFrame();
    void togglePhotoMode();
    void toggleRecording();
    // ---- the title's attract mode: after 20 s idle the camera tours the system (game.cpp, scene_draw.cpp)
    double titleIdle = 0, attractT = 0;
    bool attractActive() const { return titleIdle > 20 && sys.valid && !sys.bodies.empty(); }
    // ---- settings and keys (settings.h, menus.cpp)
    bool macHints = false;        // set by the platform layer on macOS: mention Fn for the F keys once
    Settings settings;
    // N5-05 key bindings screen: the platform layer hands the key map and the raw (unmapped) key pressed this frame
    KeyMap* keymap = nullptr;
    KeyMap ownKeymap;
    int rawKey = -1;
    std::string keysPath = "vesperis_keys.txt";
    // ---- the controller (pad.cpp, pad_state.h)
    PadMap padMap;
    std::string padPath = "vesperis_pad.txt";
    bool padInUse() const { return padUI.present && padUI.active; }   // a controller is the device used last
    // ---- auto-explore (autoexplore.cpp, autoexplore_state.h)
    bool autoOn() const { return ax.on; }
    void autoStart(int profile, uint64_t seed = 0);
    void autoStop();
    // ---- auto-explore's tape (tape.cpp, audio_deck.cpp; tape_state.h)
    static inline std::string musicDirDefault = "music";   // the harness points it away from a player's folder
    std::string musicDir = musicDirDefault;                // the explorer's music: its playlist.txt's tracks, then its other sound files
    std::vector<std::string> musicDirsGame;                // the game's own tape, where it may lie: played when the explorer's folder has none
    void tapeGameDirs(const std::string& appDir);
    bool tapeOn() const { return tape.phase != 0; }
    double tapeCaption(std::string& author, std::string& title) const;
    // ---- the files, the guide, names (guide.h, names.cpp)
    std::string settingsPath = "vesperis_settings.txt";
    Guide guide;                  // M3: names, log, statistics (guide.txt)
    std::string guidePath = "guide.txt";
    std::string shotsDir = "shots";
    std::string screenshotCaption() const;
    void noteScreenshot() { guide.screenshots++; guide.save(guidePath); }
    std::string starNameOf(const Star& s) const;
    std::string bodyNameOf(int bi) const;
    bool starNamed(const Star& s) const;
    bool bodyNamed(int bi) const;
    std::string beltNameOf(int k) const;
    std::string bodyLabelOf(int i) const;
    std::string starKeyOf(const Star& s) const;
    // ---- marks and the surface scanner (marks.cpp)
    int scanMode = -1;                 // -1 off, else the LandmarkKind the scanner listens for
    int dataSheetBottom = 0;           // Q-01: the data sheet's lowest next line's top over its pages, as last drawn (`testDataSheetBottom`)
    int dataSheetPages = 1;            // Q-01: the body's sheet's pages, as last drawn (RIGHT and LEFT step through them, then the probes' profiles)
    bool scanSkipMarked = true;        // a sight with one of the explorer's marks on it is left out
    std::string worldKeyOf(int bi) const { return Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, bi); }
    struct SiteMark { int index = -1; double x = 0, z = 0, y = 0; bool dry = false; };   // a mark of the surface's world in local metres, its ground's height (B-422: dry, read without the rivers until their tiles are built)
    std::vector<SiteMark> siteMarks; int siteMarksEpoch = -1, siteMarksBody = -1, marksVersion = 0, siteMarksVersion = -1; size_t siteMarksCount = 0; double siteMarksDryT = 0;
    const std::vector<SiteMark>& marksHere();
    int markNear(double x, double z, double radiusM);
    std::string markLabel(int mi) const;
    bool echoMarked(const SurfaceView::ScanEcho& e);
    void echoHeardAt(const SurfaceView::ScanEcho& e, double& x, double& z) const;
    const SurfaceView::ScanEcho* scanNearest(double& hx, double& hz);
    bool scanModeHere(int mode) const;
    void scanCycle(); void scanToggleSkip();
    std::string echoDetail(const SurfaceView::ScanEcho& e) const;
    void drawScanHUD();
    void drawCompassMark(double bearing, uint32_t col, bool filled);
    const SiteMark* shownMarkHere();
    void showMark(int mi);
    void drawShownMark();
    void beginMark(double x, double z);
    void beginRenameMark(int mi);
    int textMark = -1; double textMarkLat = 0, textMarkLon = 0; int textMarkKind = MARK_PLACE;   // what the entry names: a mark (-1: a new one at the place)
    void finishMark(const std::string& name);
    void removeMark(int mi);
    void migrateLandmarkNames();
    void collectMigration();
    struct MigrationJob { std::string key; int pass = 0; bool found = false; Landmark lm; std::string body; };
    std::thread migrateThread; std::atomic<bool> migrateBusy{false}; std::vector<MigrationJob> migrateOut;
    std::set<std::string> migrateTried;                        // the names asked once (one not found again is not asked every frame)
    // the sector map's cursor (local metres), a removal armed (the mark, until when), the mark Tab went to
    double mapCurX = 0, mapCurZ = 0, mapDelUntil = -1; int mapDelMark = -1, mapTabMark = -1;
    // ---- what the ground holds, taken: shards, instruments, roads, graves (names.cpp, shards_screen.cpp)
    std::string shardKey(int index) const;
    void syncShards();
    std::string instrumentKey(int id) const;
    void takeInstrument();
    std::set<uint64_t> roadsMet;                               // C-09: the old roads met on this landing (a status and a log line the first time on each)
    void takeShard();
    std::string graveKey(uint64_t id) const;
    bool shardsPending() const;
    // ---- the window, settings, the synth's state (game.cpp, settings.cpp, names.cpp)
    bool nameIsForeign(const std::string& key) const;
    int wantsWindowScale = 0;     // >0: the platform layer resizes the window to 320*s x 200*s
    void loadFromDisk();
    void applySettings();
    bool mouseCaptureWanted() const;
    AudioState audio;
    // ---- saves: three slots and an autosave, named from `savePrefix` (persistence.cpp)
    std::string savePrefix = "vesperis_save";
    int currentSlot = 1;
    std::string slotPath(int n) const { return n == 0 ? savePrefix + "_auto.txt" : savePrefix + "_" + std::to_string(n) + ".txt"; }
    bool save(const std::string& path);
    bool saveSlot(int n);
    bool autosave();
    bool load(const std::string& path);
    bool loadSlot(int n);
    bool loadNewest();
    std::string saveSummary(const std::string& path) const;
    void newGame();
    void resetExpedition(bool freshTelescope);
    // ---- the state, the clock, and what the harness shares
    GameState state = GameState::TITLE;
    double gameTime() const { return t; }
    void importInboxFile();
    std::string cometMotionString(int bi) const;
    static const char* shortPlan(int plan);
    static double siteSlope(const SurfaceSite& probe);
    static double parkDistanceFor(const Body& b);
    // ---- the galaxy chart's frame in logical pixels: the face-on disc, the edge-on strip, the levels (galaxy_chart.cpp)
    static constexpr int CHART_LEVELS = 9, CHART_FACE_Y0 = 22, CHART_FACE_H = 128, CHART_EDGE_Y0 = 152, CHART_EDGE_H = 22;
    static constexpr double CHART_TALL_LY = 110000;   // the face's height at level 0: the whole disc
    static double chartScale(int level) { return CHART_TALL_LY / std::ldexp(1.0, level) / CHART_FACE_H; }   // light years a logical pixel
    // ---- what the harness sets that the game reads, and the frame's pixels
    HarnessKnobs harness;                         // what the harness sets that the game reads (game_types.h)
    Framebuffer fb;

protected:
    // ---- the world and the ship: the clock, the stars, the system, the views (game.cpp, autopilot.cpp)
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
    // ---- the landing map: its cursor, zoom, peoples layer and shadows (landing_map.cpp, landing_map_build.cpp, landing_state.h)
    double landLat = 0, landLon = 0;   // radians
    double drainLat = 1e9, drainLon = 1e9;   // O6-03: the site whose drainage tiles were last asked for ahead of the landing
    double landSeenLat = 1e9, landSeenLon = 1e9, landMovedT = -1e9;   // B-422: where the cursor was last frame and when it last moved (its tiles asked for once it rests)
    int landBody = -1;
    int landZoom = 0;
    static constexpr int ZW = 256, ZH = 128;
    std::vector<uint32_t> zoomImg; std::vector<uint8_t> zoomEmissive; std::vector<Vec3> zoomUnit;
    std::vector<uint8_t> zoomFront; double zoomFrontT = -1e18, zoomFrontLat = 1e9, zoomFrontLon = 1e9;   // W-03: the fronts' bands over the window
    double zoomLat = 1e9, zoomLon = 1e9, zoomHalfLat = 0, zoomHalfLon = 0;
    bool zoomDry = false; double zoomPrefetchT = -1e9;   // O6-03: the zoom was drawn without the drainage (its tiles were not built); rebuilt when they are
    uint64_t zoomSeed = 0;
    std::vector<std::vector<std::pair<float, float>>> zoomRoads;   // C-09: the old roads of the window (texel coordinates), drawn as faint lines over it
    std::vector<int> zoomRoadKinds;                                  // T-01: each line's kind (peoples/plan.h's NetKind: C-09's lanes are roads)
    int zoomCores = 0;                                               // T-01: the plan's places whose cores the window shows
    bool landPeoples = false;
    std::vector<uint32_t> zoomCivImg;                                // the zoom with the places' cores (`zoomImg` the land alone)
    std::vector<ZoomPlace> zoomPlaces;                               // the plan's places in the window (their core's radius in texels)
    using LandKnown = ::LandKnown;
    LandKnown landKnownOf(int body);
    uint64_t landTerrSeed = 0;                             // the world view's lands of the peoples, per logical pixel (built for a people whose records are all held)
    std::vector<uint8_t> landTerr;                         // bit 0 the first people's land, 1 its frontier; bits 2 and 3 the second's
    void drawLandPeoples(int ox, int oy, int cx, int cy);
    struct { int places = 0, ways = 0, land = 0, marks = 0, rings = 0, capital = 0; } landDrawn;   // what the layer drew last (the harness)
    double zoomRoadsMs = 0;                                          // C-09: what the window's network cost at the last build
    double zoomBuildMs[5] = {0, 0, 0, 0, 0}; int zoomBuilds = 0;   // F-01: the last build's samples, colours, roads, plan ways and places' cores (ms); builds so far
    void buildLandingZoom();
    bool landingZoomAllowed() const;
    std::vector<uint8_t> mapShadow, zoomShadow;
    int mapShadowBody = -1; double mapShadowT = -1e18, mapShadowReal = -1e18;
    void buildShadowMap(const std::vector<Vec3>& units, std::vector<uint8_t>& out);
    void refreshMapShadows(const std::vector<Vec3>& unitLUT);
    int lastSecX = -1, lastSecY = -1;   // the sector the explorer was in last frame (crossings are announced)
    void mapRampsFor(const Body& b, MatRamp* ramps, double& lf, bool& atmo);
    static uint32_t mapColor(const MatRamp* ramps, int mat, double albedo, double lf, bool atmo, double shade, double survey = -1);
    static double surveyHeight(double h, double lo, double hi) { return hi > lo ? clampd((h - lo) / (hi - lo), 0, 1) : 0.5; }
    static void surveyRange(const float* h, size_t n, double& lo, double& hi);
    // ---- the screens: fades, the status line, menus, settings, CONTROLS (menus.cpp)
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
    int keysPage = 0;              // R-418: the CONTROLS screen's pages, 0 the keyboard, 1 the controller (Tab)
    GameState keysReturn = GameState::SETTINGS;   // where it goes back to (the command menu opens it from anywhere)
    void updateKeysScreen(const Input& in);
    void renderKeysScreen();
    // ---- the controller (pad.cpp, pad_state.h)
    using PadItem = ::PadItem;
    PadUI padUI;
    PadState lastPad;                         // the controller as last read (the controller page draws it live)
    void padFrame(Input& in, double realDt);
    int padContext() const;
    void padBindings(int ctx, std::vector<PadBind>& out) const;
    std::string padContextName(int ctx) const;
    std::vector<PadItem> padItems(int page) const;
    void padRunItem(const PadItem& it);
    void padMenuUpdate(const bool* pressed, double realDt);
    bool padKeyboardShown() const;
    void padKeyboardUpdate(const bool* pressed, bool menuPage);
    void padPress(int key, int mods);
    void renderPad();
    void renderPadKeyboard(int x0, int y0, bool menuPage);
    std::string padLegend(int ctx, int width) const;
    void openPadScreen();
    void updatePadScreen(const Input& in); void renderPadScreen();
    // ---- the slots and the system list (persistence.cpp, data_sheet.cpp)
    int slotSel = 0;
    bool slotLoadMode = false;
    std::vector<std::string> slotSummaries;
    double autosaveTimer = 0;
    int listSel = 0, listScroll = 0;
    // ---- the guide's screens (guide_screens.cpp, star_map.cpp)
    int guideSel = 0, logPage = 0, gallerySel = 0, galleryLoaded = -1, textKind = 0, textBody = -1, mapPick = -1, mapClassMask = (1 << STAR_DRAWN_COUNT) - 1;
    GameState guideReturn = GameState::SPACE;
    bool statsKinds = false;   // P-00: the statistics' second page, the world types walked (offered when they outgrow their line)
    std::string textPrompt, textBuffer;
    double mapYaw = 0, mapPitch = 0, mapZoom = 1;
    // ---- the galaxy chart, the star map's second page (galaxy_chart.cpp, chart_state.h)
    int mapPage = 0;   // 0 the neighbourhood, 1 the galaxy
    GalaxyChart gchart;
    void chartCentre(int level, double& cx, double& cz) const;
    void chartView(long long& I, long long& J) const;
    bool chartProject(double sx, double sz, int& px, int& py) const;
    void updateGalaxyChart(const Input& in);
    void renderGalaxyChart();
    static constexpr double CHART_REACH_LY = 54000;    // the cursor's reach from the galactic centre (the whole disc's face)
    static constexpr double CHART_HEIGHT_LY = 30000;   // and over or under the plane
    Vec3 chartCursor() const;
    bool chartPlotted() const;
    void chartLetGo(bool height);
    void chartUnplot();
    void chartEnter();
    using ChartStop = ::ChartStop;
    std::vector<ChartStop> chartStops() const;
    void chartNextStop(int dir);
    int galaxyMarkNear(double sx, double sz, double px) const;
    using ChartThere = ::ChartThere;
    ChartThere chartThere(const Vec3& p) const;
    bool chartDrifterAt(const Vec3& p, Star& out) const;
    std::string galaxyPlace(const Vec3& p, bool height) const;
    void beginGalaxyMark(int mi);
    void finishGalaxyMark(const std::string& name);
    int textGalaxyMark = -1; Vec3 textGalaxyAt; std::string textGalaxyLine;
    bool drawMapStatus();
    const StarNeighborhood& mapStars() const { return gchart.look ? gchart.lookNb : nb; }   // the neighbourhood page's stars and centre
    Vec3 mapCentre() const { return gchart.look ? gchart.lookCentre : ship.pos; }
    // ---- the gallery and the walk's statistics (guide_screens.cpp)
    struct GalleryItem { std::string file; int rec = -1; long long when = 0; };   // X-04: a photograph, or a probe's recording (`rec`: its record; `file` its final image, "" for a lent one: drawn)
    std::vector<GalleryItem> galleryItems;
    std::vector<uint32_t> galleryPix; int galleryW = 0, galleryH = 0; bool galleryOk = false; std::string galleryCaption;
    double walkMax = 0, lastWalkX = 0, lastWalkZ = 0, lastOdometer = 0;
    double drivenLanding = 0;     // metres driven this landing, over every buggy deployed (the launch log)
    double flownLanding = 0;      // R-403: metres flown this landing
    bool visitNoted = false;
    // ---- the cabin, the flight computer and the console (cabin.cpp)
    Cabin cabin;
    GrainTexture cabinGrain;
    int shipScreenPage = 0, shipScreenSel = 0;
    std::vector<std::string> consoleLines;
    std::string consoleInput;
    Mat3 viewBasis() const;
    void cabinPalette();
    void updateCabin(const Input& in, double realDt);
    void drawCabin();
    void drawCabinBand(const CabinFrame& f);
    void renderCabinHUD();
    void updateShipScreen(const Input& in); void renderShipScreen();
    void openConsole(); void consolePrint(const std::string& line); void consoleCommand(const std::string& raw); void updateConsole(const Input& in); void renderConsole();
    // ---- the shard decoder (shards_screen.cpp, history_screen.cpp, medium_draw.cpp; shard_world.h)
    ShardWorld shardWorld;
    int shardsPage = 0;                          // 0 the records, 1 the timeline, 2 the history
    int shardsFilter = 0;                        // the records' page: 0 every kind, 1 the texts, 2 the music, 3 the star charts
    struct LegendRow { int kind = 0; int item = -1; int year = 0; bool first = false; std::string text; std::vector<DecodedWord> words; };   // 0 a heading, 1 an event's line (`item` of `told.events`; `first` its first row), 2 the close, 3 a way of the people (`text` its label)
    ToldHistory told; bool toldValid = false; double toldMs = 0;   // what the read records told (built when the world's pages are opened, again after a reading; how long it took)
    std::vector<LegendRow> legendRows; std::vector<int> legendFirstRow;   // the history page's rows; each told event's first row
    int legendSel = 0, legendTop = 0;            // the event under the cursor (of `told.events`), the first row shown
    struct { int dir = 0; double t = 0, acc = 0; } shardRun;   // R-416: Shift with an arrow held runs the timeline's and the history's cursor
    int shardRunSteps(const Input& in, double realDt, bool leftRight);
    std::vector<double> eraX0, eraX1;            // the timeline's eras across its width (each era's share by the people's records in it)
    void ensureTold();
    bool shardInFilter(int local) const;
    void stepShard(int dir, bool stop = false);
    int toldRecordOf(int item) const;
    void renderShardRecords(); void renderShardTimeline(); void renderShardHistory(); void drawShardPagesStrip(const std::string& right);
    std::string recordSummary(int local, int maxChars) const;

    std::vector<std::string> shardWorldKeys, shardWorldLabels; std::vector<int> shardWorldWhich;   // C-13: an entry a people whose shards are held, labelled with the people's name where the world had two
    std::vector<int> shardWorldTotals;           // T-04: how many records each entry's people left (`recordCountOf`)
    std::vector<int> shardHeld;                  // the open world's shards held, in the order of their years
    size_t shardsListed = 0;                     // C-18: the guide's shards and lent ones when the lists were made (the drones bring more while they are open)
    void listShardWorlds();
    void listShardsHeld();
    int shardsLevel = 0, shardWorldSel = 0, shardSel = 0;
    std::vector<DecodedWord> shardWords;         // the shard on the screen as the computer reads it
    struct WordAt { int page = 0, x = 0, y = 0, w = 0; };
    std::vector<WordAt> shardLayout; std::vector<int> shardPageStart; int shardPage = 0, speechFrom = 0, speechTo = 0;
    void layoutShardText(int x0, int y0, int maxX, int maxY);
    void speakShardPage(int page);
    bool shardFresh = false;                     // C-05: a first reading at this share: the words resolve as the recording speaks them, and its end reads the shard
    double speechT = -1, speechSyncT = -1; bool speechPaused = false;   // C-05: the game's own clock into the speech (synced to the synth's when it runs), paused
    void startSpeech(); void stopSpeech(); bool speechEnded() const; void markShardRead();
    std::string mediumLine(const Shard& s) const;
    void drawMediumPanel(int x0, int y0, const Shard& s, bool playing, double t);
    // C-04 a piece on the screen: the game's own clock into it (synced to the synth's beat when the synth runs), paused, the shard being named
    double pieceT = -1, pieceSyncBeat = -1, humBeforePiece = -1; bool piecePaused = false;
    std::string textShardKey;
    std::string textStormKey;   // X-04: the storm a text entry of kind 9 names
    void startPiece(); void stopPiece(); double pieceBeatNow() const; bool pieceEnded() const;
    bool openShardWorld(const std::string& key, int which = 0);
    bool buildShardWorld(ShardWorld& w, const Star& s, int bi, int which = 0, bool wait = true);
    void prefetchShardWorld(int sel);
    // ---- star charts: drawn on the decoder, held up to the sky (chart_draw.cpp)
    static constexpr double CHART_READ_S = 4.0;
    double chartT = -1;                              // the reading's clock (-1 none)
    StarChart chartHeld; bool chartUp = false; std::string chartHeldKey, chartHeldPeople; int chartHeldIndex = -1;
    std::vector<std::string> chartHeldLabels;        // the figures' names as the computer read them when the chart was held up
    int chartLinesDrawn = 0;                         // the figures' lines on the screen last frame (the harness)
    void renderChart();
    void drawChartOverlay(const Mat3& cam, const Vec3& obs, const Proj& pj, const Mat3* localFrame);
    std::string figureLabel(const std::string& name, int held) const;
    void holdChart(bool up);
    // ---- the signal radar and the dark radio (radar.cpp, radar_camera.cpp, radar_state.h)
    RadarState radar;
    void radarToggle(); void radarOff(); void radarScan(); bool updateRadar(const Input& in, double realDt);
    void darkRadio(); bool darkRadioOn = false; uint64_t darkRadioSeed = 0; RogueVoice darkRadioVoice;
    int darkRadioBeacon = -2; double darkRadioBeaconAt = -10;   // D-05: the beacon's machine under it (-1 none, -2 not decided yet), last asked when
    void radarGainStep(int dir);
    void radarDeepStep();
    double radarSkyAt(const Vec3& dir) const;
    void radarRiseStep(const Vec3& fwd, double realDt);
    void radarWordStep(double realDt); void radarWordStop();
    double radarGain() const { return SIGNAL_GAINS[std::max(0, std::min(SIGNAL_GAIN_STEPS - 1, radar.gainStep))]; }
    void renderRadarCamera();
    void radarCameraFeed(Framebuffer& fb);
    void radarLock(int idx); void radarAccept(); void radarStartContent(const Signal& s, bool next); void radarRepeatContent(); void radarStopContent();
    void radarStartBeacon(const Signal& s);
    // ---- the decoder's worlds and records (shards_screen.cpp)
    int shardsHeldOf(const std::string& key, int which = 0) const;
    int lentOf(const std::string& key, int which = 0) const;
    bool shardLent(int local) const;
    const std::string* shardNameOf(const std::string& key, bool& foreign) const;
    std::string heldShardKey(int local) const;
    std::string endedKey() const;
    std::string shardWorldTitle(int maxName) const;
    std::string worldNameOfKey(const std::string& key) const;
    void openShards(); void openShard(int sel); void updateShards(const Input& in, double realDt); void renderShards(); void renderPiece();
    // ---- the ship's player (player.cpp, player_state.h)
    ShipPlayer player;
    int shardsTop = 0;                                 // the decoder's first level: 0 the worlds, 1 the player
    void playerBuildList();
    void playerPlay(int item);
    void playerStart();
    void playerStop();
    bool playerOwns() const;
    void updatePlayer(double realDt);
    void renderPlayerPage();
    std::string playerHudLine() const;
    static std::string playerItemKey(const PlayerItem& it);
    static std::string playerItemTitle(const PlayerItem& it, const std::string* name);
    // ---- the instruments' case (instruments_screen.cpp, instrument_draw.cpp; case_state.h)
    InstrumentCase icase;
    void openInstruments(); bool loadInstrument(int sel); void updateInstruments(const Input& in, double realDt); void renderInstruments(); void stopInstrument();
    void openCaseWorld(int w);
    void renderCaseWorlds();
    // ---- the drone bay (drones.cpp, drone_draw.cpp; drone_state.h)
    DroneBay bay;
    std::thread droneJob; std::atomic<bool> droneJobBusy{false}; DroneWorld droneJobOut; uint64_t droneJobStar = 0; int droneJobBody = -1;
    void openDroneBay(); void updateDroneBay(const Input& in, double realDt); void renderDroneBay();
    void droneBayLaunch();
    void updateDrones(double realDt);
    void droneWorldAsk(int body);
    void droneCollect();
    bool droneWorldHere(uint64_t starSeed, int body) const { return bay.worldValid && bay.worldStar == starSeed && bay.worldBody == body; }
    void dronePlanTrip();
    void droneDock(int k);
    void droneRecall();
    void droneTripEnd();
    void droneFindsSeen();
    bool droneBayHere() const;
    bool droneBayReady() const;
    double droneCooldownLeft() const;
    double droneBackIn() const;
    std::string droneFindKey(const DroneFindRec& f) const;
    std::string droneFindLabel(const DroneFindRec& f) const;
    std::string droneStateLine(int k) const;
    bool dronePosition(int k, double& lat, double& lon) const;
    DronePose dronePoseAt(int k) const;
    bool droneRackPlace(int k, double& x, double& lift, double& blur) const;
    void droneRackFrame(CabinFrame& f) const;
    void drawDroneRack(const CabinFrame& f);
    void drawDroneLights();
    void drawDroneMesh(const Mat3& look, const Vec3& eye, const Vec3& origin, const Vec3& fwd, const Vec3& up, const DronePose& pose, const Vec3* sun);
    void drawDronePrims(const Mat3& look, const Vec3& eye, const Vec3& origin, const Vec3& fwd, const Vec3& up, const DronePose& pose, const Vec3* sun, const std::vector<DronePrim>& prims);
    void drawDroneWire(int x0, int y0, int w, int h, const DronePose& pose, double yaw, uint32_t col);
    void buildDroneMap(int body);
    int droneBayWorld() const;
    // ---- the case's instrument played and drawn (instruments_screen.cpp, instrument_draw.cpp)
    void playInstrumentDegree(int degree, int key);
    void drawInstrumentPicture(int x0, int y0, int w, int h, const Instrument& I, const Tradition& T, const FormPalette& pal, const std::vector<double>& glow, double t);
    // ---- the capsule's launch and the Vimana (cabin.cpp)
    void deployCapsule(); void toggleVimana();
    // ---- the surface's sector map, vision modes and readings (sector_map.cpp, scene_draw.cpp, hud.cpp)
    int visionMode = 0;
    bool readings = false;   // R-414: the readings over the surface's view (Tab): the place, the sky, the air, the ground, the vehicle
    void drawReadings();
    int sectorZoom = 1;
    std::vector<uint32_t> sectorImg; int sectorImgW = 100, sectorImgH = 80; double sectorImgX = 1e9, sectorImgZ = 1e9; int sectorImgZoom = -1, sectorImgEpoch = -1;
    static constexpr int SECTOR_LEVELS = 6;   // O1-04: 2, 8, 32, 128, 512 and 2048 km wide (the widest levels only on big worlds)
    int sectorMaxLevel() const;
    void updateSectorMap(const Input& in); void renderSectorMap();
    void applyVision();
    // ---- the guide's screens: the menu, text entry, the log, statistics, the gallery (guide_screens.cpp)
    void openGuide(); void updateGuideMenu(const Input& in); void renderGuideMenu();
    void beginTextEntry(const std::string& prompt, int kind, const std::string& initial); void updateTextEntry(const Input& in); void renderTextEntry();
    void updateLog(const Input& in); void renderLog(); void renderStats();
    std::string statsKindsLine() const; bool statsKindsOverflow() const;
    void openGallery(); void updateGallery(const Input& in); void renderGallery();
    // ---- the star map and the targets (star_map.cpp); the log's events and the visits (names.cpp)
    void openStarMap(); void updateStarMap(const Input& in); void renderStarMap();
    void setRemoteStar(const Star& s); void targetStarByName(const std::string& name); void targetPreviousStar(); void targetHome();
    void logEvent(const std::string& kind, const std::string& text); void noteVisit();
    // ---- the title's and help's state, the arrival, the landing map's probe site
    int helpPage = 0;
    double titleYaw = 0;
    double lastRealDt = 1.0 / 60;
    double flashTimer = 0;
    std::string arrivalNote;
    SurfaceSite probeSite;
    double probeLat = 1e9, probeLon = 1e9;
    int probeBody = -1;

    // ---- space: its update, scene and HUD (autopilot.cpp, scene_draw.cpp, hud.cpp; the status line game.cpp)
    void status(const std::string& m, double secs = 4);
    void refuse(const std::string& m, double secs = 4);   // the status line and the error beep
    void updateSpace(const Input& in, double dt, double realDt);
    void updateShipMotion(double dt);
    void renderSpace();
    void keepFlightBand();
    void renderSpaceHUD();
    void drawVisor(uint32_t col);
    void drawCommonHUD(bool warp = true);
    void arriveAtStar(const Star& s, const Vec3& fromDir);
    void lockRemoteTarget();
    void nearestStars(int n, std::vector<const Star*>& out) const;
    void cycleTargetStar();
    int pickBodyNearCrosshair();
    void startApproach(int body);
    // ---- the landing map's screen (landing_map.cpp, landing_map_build.cpp)
    void updateLandingMap(const Input& in, double dt);
    void renderLandingMap();
    void beginLanding();
    void buildLandingMapBase();
    // ---- the capsule and the ground: their update, scene and HUD (surface_play.cpp, scene_draw.cpp, hud.cpp)
    void updateDescentAscent(const Input& in, double dt, double realDt);
    void updateSurface(const Input& in, double dt, double realDt);
    void renderSurfaceScene();
    void renderSurfaceHUD();
    // ---- the system list and the data sheet, its forecast and the probes' profiles (data_sheet.cpp, probe_records.cpp)
    // W-03: the forecast at the feet: the next fronts within five days (`frontsAhead`), listed again every thirty seconds of game time
    std::vector<FrontForecast> forecast; double forecastT = -1e18, forecastReal = -1e18; int forecastBody = -1;
    void updateForecast();
    std::string forecastLine() const;
    void renderSystemList();
    void renderDataSheet();
    int dataPage = 0;                     // X-04: the data sheet's page (0 the sheet, 1.. a giant's probes' profiles)
    std::vector<int> dataProbes() const;
    void renderProbeProfile(int rec);
    // ---- the title, help, the menu and the settings (menus.cpp); the slots (persistence.cpp)
    void renderTitle();
    void renderHelp();
    void renderMenu();
    void updateSettingsScreen(const Input& in);
    void renderSettings();
    void adjustSetting(int item, int dir);
    void openSlots(bool loadMode);
    void updateSlots(const Input& in);
    void renderSlots();
    // ---- jumps along the aim and from the chart (autopilot.cpp, hud.cpp, star_map.cpp)
    std::string jumpDigits;
    Vec3 aimDirection() const;
    void startJump(double ly, const Vec3& dir);
    struct JumpPlot { uint64_t seed = 0; double ly = 0, shortLy = 0; std::string way, from; bool onChart = false; } jumpPlot;
    bool plotJump(double ly, const Vec3& dir, bool onChart, const Star* place = nullptr);
    void flyJump();
    std::string aimStarLabel(const Star& full) const;
    std::vector<std::string> worldNamesAt(const std::string& starKey) const;
    // ---- belts as destinations (autopilot.cpp)
    bool pickBeltRock(int k, double at, int64_t& ir, int64_t& ia, int64_t& iy, int& m) const;
    bool beltRockNow(BeltRock& rk) const;
    Vec3 beltParkPos(double at) const;
    void startApproachBelt(int k);
    std::string localTargetLabel() const;
    // ---- the palette's bodies, the epoch's strings (autopilot.cpp, game.cpp)
    void choosePaletteBodies(int& a, int& b);
    std::string epocString() const;
    static std::string epocOf(double secs);
    std::string epocFullString() const;
    static double wallClockT();
    // ---- the almanac (almanac.cpp)
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
    // ---- the telescope (telescope.cpp, telescope_state.h)
    TelescopeState tele;
    static constexpr int TELE_MAX_STEP = 10;   // x1024
    double telescopeZoom() const { return tele.on ? (double)(1 << tele.step) : 1.0; }
    Proj telescopeProj() const; double telescopeFieldDeg() const;
    void telescopeToggle(); void telescopeOff(); void telescopeZoomStep(int dir); void telescopeTrack(int id, bool recentre = true); void telescopeAim(); void telescopeName();
    void telescopeHoldFrame(); void telescopeHoldView(); bool telescopeStabilised() const;
    void orbitShip(double east, double north, double dt);
    void updateTelescope(const Input& in, double realDt); void telescopeFindTarget(); void renderTelescopeHUD();
    std::string telescopeTargetName(int id) const; std::string telescopeTargetLine(bool plain = false) const;
    Star textStar;   // the star a text entry of kind 8 names (one seen through the telescope)
    // the user's review of W-06: the eyepiece shows the planet function at its own footprint through the renderer's plate
    // (`DetailPlate`, space_view.h), built for the body the eyepiece rests on within a budget of samples a frame
    static constexpr int TELE_PLATE_BUDGET = 8000;
    bool telePlateFinish = false;   // the next space frame reads the plate out whole (a photo, the harness)
    int telescopePlateBody() const;
    bool viewGround(int body, double& lat, double& lon) const;
    // ---- the giant probe (probe.cpp, probe_state.h)
    ProbeState probe;
    bool probeAimPoint(double& lat, double& lon) const;
    void launchProbe(); void watchProbe(); void probeCut(const char* why); void probeEnd(); void probeRelease();
    void updateProbe(double realDt);
    void updateProbeScreen(const Input& in, double dt, double realDt);
    void renderProbe();
    void renderProbeHUD(const ProbeRecord* rec = nullptr, double clock = 0, double tt = 0);
    void probeHiss();
    void probeSound(double clock, bool watching, bool ended);
    void probeAudio();
    bool probeHere() const;
    std::string probeStageLabel(double clock) const;
    // ---- the probe's camera (probe_feed.cpp)
    void renderProbeFeed(double clock, double tt);
    Vec3 probeWorldPos(double clock, double tt, Vec3& up) const;
    void probeEnsureClouds();
    double probeDeckDrift(int k, double clock) const;
    bool probeStormWall(double& dist, double& ex, double& ez) const;
    bool probeAuroraWay(double& ex, double& ez, double& p0) const;
    // ---- what the probe sends back: its track and profile, its recordings on the screen (probe_records.cpp)
    void probeTrack();
    void probeKeep(int cause);
    void probeProfileOf(ProbeRecord& r) const;
    double probeLightAt(double clock, double tt) const;
    int probeLayerAt(double clock) const;
    std::string probeStormName() const;
    std::string nameOfKey(const std::string& key) const;
    ReplayState replay;
    template <class F> void asRecording(F f);
    bool recordingContext(int rec);
    bool openRecording(int rec);
    void updateRecording(const Input& in, double realDt);
    void renderRecording();
    void recordingAudio();
    void recordingPoster(int rec, std::vector<uint32_t>& out);
    double recordTimeAt(const ProbeRecord& r, double clock) const;
    void recordCamAt(const ProbeRecord& r, double clock, double& yaw, double& pitch) const;
    double recordDepthU(const ProbeRecord& r, double clock) const;
    double recordClockAtU(const ProbeRecord& r, double u) const;
    std::string recordTitle(const ProbeRecord& r) const;
    std::string recordLine(const ProbeRecord& r) const;
    std::string probeCaption() const;
    // ---- the global keys and the ship's place (game.cpp)
    std::string distanceString(double km) const;
    void handleGlobalKeys(const Input& in);
    // ---- auto-explore: its beginning and end, its mind, the chooser (autoexplore.cpp, autoexplore_state.h)
    AutoState ax;
    int autoSel = 0; GameState autoBack = GameState::MENU;   // the chooser's row; where Esc goes back to
    bool autoTakeover(Input& in, double realDt);
    void autoInput(Input& in, double realDt);
    int autoScene() const; int autoLookFrame() const;
    void autoRun(double dt);
    void autoPush(const char* what, double maxT, std::function<bool(double)> fn, bool timed = false);
    void autoPlan();
    void openAutoScreen(GameState back); void updateAutoScreen(const Input& in); void renderAutoScreen();
    // ---- auto-explore's plans in space and on the ground (autoexplore_space.cpp, autoexplore_ground.cpp)
    void autoPlanSpace(); void autoPlanArrival(); void autoPlanVisit(int b); void autoPlanParked(int b); void autoPlanRoof(int b); void autoPlanTelescope(int b);
    void autoPlanLanding(int b); void autoPlanProbeLaunch(); void autoPlanFlight(); void autoPlanMove(); void autoPlanRadar(); void autoPlanJump(double ly, const Vec3& dir);
    bool autoPickStar(Star& out); int autoPickBody(); int autoWhim(); bool autoChooseSite(int b, bool byPlace, bool underGiant, bool low); void autoLandingMap();
    std::vector<int> autoBodiesInView(int n);
    void autoPlanProbe();
    void autoPlanGround(); bool autoPlanOuting(int kind); void autoPlanLeave(); void autoPlanVehicleOut(int which);
    bool autoOutingVista(); bool autoOutingWater(); bool autoOutingLife(); bool autoOutingSight(); bool autoOutingDrive(); bool autoOutingFly(); bool autoOutingRuins(); bool autoOutingSky();
    void autoPlanTravel(double x, double z, double r, int how, double circle = 0);
    void autoPlanTown(double cx, double cz, double size);
    bool autoFindRise(double& x, double& z); bool autoBigSky(Vec3& dirLocal); bool autoDryAt(double x, double z);
    bool autoRecordAt(int index, double& x, double& z); int autoNearestRecord(double cx, double cz, double radius, double& x, double& z);
    const Critter* autoCritter(uint64_t seed) const;
    // ---- auto-explore's hands (autoexplore_hands.cpp)
    void autoTap(int key); void autoHold(int key); bool autoEvery(double secs); void autoSprint(bool want);
    void autoLook(double yaw, double pitch, double pace = 1); void autoLookSpace(const Vec3& dir, double pace = 1); void autoLookLocal(const Vec3& dir, double pace = 1);
    void autoLookAt(double x, double y, double z, double pace = 1);
    void autoAimShip(const Vec3& dir); double autoShipOff(const Vec3& dir) const;
    void autoWindowLook(double pace); void autoEyesFront(double pace); void autoGazeOverBody(int b); void autoGazeRound(double pace);
    bool autoAtWindow() const; bool autoCabinWalk(double x, double z, double yaw); bool autoCabinToCapsule();
    int autoWalkTo(double x, double z, double r, double speed, double dt, double replanMove = 0.5);
    void autoWay(double x0, double z0, double x1, double z1, std::vector<std::pair<double, double>>& out);
    int autoDrive(double x, double z, double r, double cruise, double dt); bool autoDriveClear(double x, double z, double heading);
    int autoFly(double x, double z, double r, double alt, double cruise, double dt); void autoCircle(double cx, double cz, double radius, double alt);
    void autoHands(Input& in, double dt);
    void autoRender();
    // ---- auto-explore's tape (tape.cpp, audio_deck.cpp; tape_state.h)
    TapeState tape;
    void tapeList();
    static void tapeRead(const std::string& dir, std::vector<TapeTrack>& out);
    void tapeToggle();
    void tapeStart(bool asked);
    void tapeHalt(bool stopKey);
    void tapeDeck(int kind);
    void tapeUpdate(double realDt);
    void tapeRender();
};
