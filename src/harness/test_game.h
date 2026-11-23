// TestGame: a Game with the hooks the harness reads and sets it through (test*, `setState`). The game's own code calls
// none of them; the four knobs it does read are Game's `harness` (game_types.h). Every Game the harness makes is a TestGame,
// and the hooks reach Game's protected state; their bodies, when not here, are test_game.cpp's
#pragma once
#include "game/game.h"
#include <chrono>
#include <string>
#include <thread>
#include <vector>

class TestGame : public Game {
public:
    // B-440: a test game's files are the harness's, under shots/tests: the working directory's guide, saves, settings, keys, controller
    // map, photographs and films are the player's when the harness runs in the repo root (a check's second game once saved its guide
    // over theirs). A check that keeps its own sets them beside these; the unit's `files` group holds the player's untouched
    TestGame() {
        guidePath = "shots/tests/game_guide.txt"; savePrefix = "shots/tests/game_save"; settingsPath = "shots/tests/game_settings.txt";
        keysPath = "shots/tests/game_keys.txt"; padPath = "shots/tests/game_pad.txt"; shotsDir = "shots/tests/game_shots"; moviesDir = "shots/tests/game_movies";
    }
    // ---- the title's attract mode and the bench's counters (game.cpp; M7-01, M7-02)
    void testAttract(double idleSecs) { titleIdle = idleSecs; }
    const SpaceRenderer::MapStats& testMapStats() const { return spaceR.mapStats; }
    size_t testMapsHeld() const { return spaceR.mapsHeld(); }
    int testMapsMaking() const { return spaceR.mapsMaking(); }   // B-439: the maps on their threads now (the bench's rows aboard wait for them)
    long testGenMicros() const { return spaceR.genMicros.load(); }
    int testAheadCells() const { return surf.testAheadCells(); }
    // ---- the controller: the command menu, the keyboard, the controller page (pad.cpp)
    bool testPadMenu() const { return padUI.menu; }
    int testPadPage() const { return padUI.page; }
    bool testPadKeyboard() const { return padKeyboardShown(); }
    std::string testPadItems(int page) const;   // the menu's page as its lines ("label|key")
    int testPadCount(int page) const { return (int)padItems(page).size(); }
    std::string testPadCommand(int page, int i, std::vector<std::pair<int, int>>& keys);   // U-06: a line run as the controller runs it
    std::string testPadLegend() const;          // the buttons' legend of the context
    int testKeysPage() const { return keysPage; }
    int testPadCapture() const { return padUI.capture; }
    int testPadScreenSel() const { return padUI.screenSel; }
    // ---- auto-explore (autoexplore.cpp, autoexplore_space.cpp, autoexplore_ground.cpp, autoexplore_hands.cpp)
    std::string testAutoInfo() const;                 // the profile, the focus, the step, the scene and what it has done so far
    std::string testAutoDoing() const { return ax.on ? ax.doing : std::string(); }
    std::string testAutoTown() const { return ax.townLine; }
    int testAutoCount(int what) const;                // 0 stars, 1 parkings, 2 landings, 3 probes, 4 records, 5 outings, 6 steps given up, 7 unsticks, 8 samples
    // Z-07: on the ground (a frame of it after `autoStart`), auto-explore's outing to the water planned now (a spring's first where one is in
    // reach), its first look round passed, none left after it: false when none is in reach
    bool testAutoWater();
    bool testNearWater(double withinM, double& x, double& z);   // Z-07: a dry spot 80 m short of the nearest water within reach (rings every 40 m)
    Vec3 testViewDir() const;                         // the eye's direction this frame (the galaxy's in space, the ground's on it)
    std::string testAngles() const;                   // the ship's and the head's angles, the ship's mode
    void testOpenAuto() { returnState = state; openScreen(GameState::AUTO); }
    int testAutoSel() const { return autoSel; }
    // ---- auto-explore's tape (tape.cpp)
    std::string testTapeDir() const { return tape.dir; }   // the folder the tape was read from
    std::string testTapeInfo() const;                      // the phase, the track, the music's level, the duck, the caption
    size_t testTapeTracks() { tapeList(); return tape.tracks.size(); }
    std::string testTapeTrack(size_t i) const;             // a track as listed: file | author | title | gain
    // ---- the shard decoder and its history pages (shards_screen.cpp, history_screen.cpp)
    std::string testShardsInfo() const;
    std::string testHistoryInfo();                                             // H-06: the decoder's pages for the harness (the page, the told events, the age, the eras)
    const ToldHistory& testTold() { ensureTold(); return told; }               // what the open world's read records told
    std::vector<std::string> testLegendLines();                                // the history page's rows as the screen shows them (the words as read)
    int testShardsPage() const { return shardsPage; }
    int testShardSel() const { return shardHeld.empty() ? -1 : shardHeld[std::max(0, std::min((int)shardHeld.size() - 1, shardSel))]; }   // the record under the cursor (its local index)
    int testShardCursor() const { return shardSel; }                           // the cursor's place among the records held (the timeline's)
    int testShardsFilter() const { return shardsFilter; }                      // the kind the records' page shows (0 every kind)
    int testShardsHeld() const { return (int)shardHeld.size(); }
    int testLegendSel() const { return legendSel; }                            // the history's event under the cursor
    // ---- the signal radar (radar.cpp)
    std::string testRadarInfo() const;         // C-07: the radar's state for the harness
    bool testAimAtSignal(int which);           // turn the ship (the cabin view forward) onto signal `which` of the radar's list (far first, then local)
    int testRadarSignals() const { return (int)radar.all.size(); }
    int testRadarLocked() const { return radar.locked; }
    int testRadarKind(int which) const { return which >= 0 && which < (int)radar.all.size() ? radar.all[which].kind : -1; }
    int testRadarProgramme() const { return radar.prog.kind; }   // the programme on the air (RP_*, -1 none)
    bool testRadarLocal(int which) const { return which >= 0 && which < (int)radar.all.size() && radar.all[which].local; }
    bool testRadarOn() const { return radar.on; }
    bool testRadarSector(int which, int64_t& sx, int64_t& sy, int64_t& sz) const {
        if (which < 0 || which >= (int)radar.all.size()) return false;
        sx = radar.all[which].star.sx;
        sy = radar.all[which].star.sy;
        sz = radar.all[which].star.sz;
        return true;
    }
    double testRadarDist(int which) const { return which >= 0 && which < (int)radar.all.size() ? radar.all[which].distLy : -1; }
    double testRadarNeedGain(int which) const { return which >= 0 && which < (int)radar.all.size() ? radar.all[which].needGain : -1; }
    int testRadarGainStep() const { return radar.gainStep; }
    bool testRadarDeepReady() const { return radar.deepReady; }
    bool testRadarOverload() const { return radar.overload; }
    double testRadarLockSecs() const { return radar.lockSecs; }
    double testRadarReading() const { return radar.reading; }
    int testRadarUnder() const { return radar.under; }
    bool testRadarRise(Vec3& dir) const { dir = radar.rise.dir; return radar.rise.on; }
    int testRadarNothings() const { return radar.nothings; }
    double testRadarHold() const { return radar.holdT; }
    int testRadarWords() const { return radar.words; }
    bool testRadarWordOn() const { return radar.wordT >= 0; }
    std::string testRadarWordSaid() const { return radar.wordSaid; }
    double testRadarFloor() const { return radar.floorNow; }
    std::string testRadarWordKey() const { return radar.wordKey; }
    Vec3 testRadarDir(int which) const { return which >= 0 && which < (int)radar.all.size() ? normalize(radar.all[which].pos - ship.pos) : Vec3(0, 0, 1); }
    bool testAimAtDir(const Vec3& d) {
        Vec3 f = normalize(d);
        ship.yaw = std::atan2(f.x, f.z);
        ship.pitch = std::asin(std::max(-1.0, std::min(1.0, f.y)));
        cabin.yaw = 0;
        cabin.pitch = 0;
        return true;
    }
    bool testCivilisedHere() const;    // a people's world in the system the ship is at
    bool testShardsOpenIndex(int idx);   // at the world's list, opens the shard of that index (a piece plays)
    // ---- the ship's player and the instruments taken (player.cpp, instruments_screen.cpp)
    int testPlayerItems();                     // the player's list, built afresh: its items
    std::string testPlayerItemLine(int i) const;
    bool testPlayerPlay(int item);             // plays an item of the list from its start
    std::string testPlayerInfo() const;        // what plays, its clock, held / paused / waiting, the synth's pointers
    int testPlayerPlaying() const { return player.playing; }
    bool testPlayerHeld() const { return player.held; }
    bool testPlayerWaiting() const { return player.waiting; }   // KI-410: its world's records still being written on their thread
    double testPlayerT() const { return player.t; }
    int testPlayerFinished() const { return player.finished; }
    void testPlayerSkipTo(double t) { player.t = t; player.syncT = -1; }   // the player's clock moved on (a long piece's end without its minutes)
    bool testPlayerOwnsPiece() const { return audio.piece && audio.piece == &player.world.piece; }
    int testPlayerLevel() const { return player.level; }            // R-431: its screen's: 0 the worlds, 1 a world's pieces
    int testPlayerWorlds() const { return (int)player.worldFirst.size(); }
    std::string testPlayerHud() const { return playerHudLine(); }
    int testShipMode() const { return (int)ship.mode; }
    int testWalkToInstrument(int age = -1);    // the explorer before the nearest instrument not taken (of that age, -1 any), facing it: its id, -1 none
    int testNearInstrumentId() const { return surf.nearInstrument.id; }
    std::string testInstrumentInfo() const;    // the case: the one on the screen, its timbre, its register, the notes played
    // ---- the case's worlds (instruments_screen.cpp)
    int testCaseLevel() const { return icase.level; }
    int testCaseWorldSel() const { return icase.worldSel; }
    int testCaseWorlds() const { return (int)icase.worlds.size(); }
    int testCaseWorldItems(int w) const { return w >= 0 && w < (int)icase.worldCounts.size() ? icase.worldCounts[w] : -1; }
    std::string testCaseWorldKey(int w) const { return w >= 0 && w < (int)icase.worlds.size() ? icase.worlds[w] : std::string(); }
    int testCaseWorldWhich(int w) const { return w >= 0 && w < (int)icase.worldWhich.size() ? icase.worldWhich[w] : -1; }
    int testCaseKeys() const { return (int)icase.keys.size(); }
    std::string testCaseKey(int i) const { return i >= 0 && i < (int)icase.keys.size() ? icase.keys[i] : std::string(); }
    // ---- the drone bay (drones.cpp, drone_draw.cpp)
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
    int testShardsListed() const { return (int)shardHeld.size(); }            // C-18: the open world's records the decoder lists
    int testDronesWorldItems(int kind = -1, bool unheld = false);   // the parked world's items (of a kind; not held), read now
    double testDroneRackX(int k) const;          // where a drone stands in the cabin (its rack's slot, the launch moving it), its height
    double testDroneRackY(int k) const;
    std::string testDroneState(int k) const { return k >= 0 && k < DRONE_COUNT ? droneStateLine(k) : std::string(); }
    void testOpenDroneBay() { openScreen(GameState::DRONES); }
    void testOpenShards() { openScreen(GameState::SHARDS); }   // the decoder, which takes the synth
    bool testDroneBayMapped() const { return bay.mapW > 0; }   // the screen's map of its world drawn
    int testShardsLevel() const { return shardsLevel; }                        // the decoder's state for the harness
    // ---- star charts (chart_draw.cpp)
    std::string testChartInfo() const;         // C-10: the chart on the screen and the one held up
    bool testAimAtChartMark(int figure = -1);  // turn the ship onto the held chart's marked star (or onto figure `figure`'s brightest star)
    bool testRemote(int64_t& sx, int64_t& sy, int64_t& sz) const { if (!ship.hasRemote) return false; sx = ship.remote.sx; sy = ship.remote.sy; sz = ship.remote.sz; return true; }
    int testChartLines() const { return chartLinesDrawn; }
    bool testChartUp() const { return chartUp; }
    // of the held chart's stars, how many the sky here draws
    int testChartStarsInSky() const { int n = 0; for (const ChartStar& c : chartHeld.stars) for (const Star& s : nb.stars) if (s.seed == c.star.seed) { n++; break; } return n; }
    // ---- marks, the scanner, the ground under the feet (marks.cpp)
    int testEchoes(int kind = -1) const { int n = 0; for (const auto& e : surf.echoes) if (kind < 0 || e.lm.kind == kind) n++; return n; }
    bool testScanned() const { return surf.scanned; }
    int testScanMode() const { return scanMode; }
    std::string testScanInfo();                                // the mode, the echoes, the nearest (true and heard), the marks here
    bool testEchoTrue(double& x, double& z, double& radius);   // the nearest echo of the mode's true place
    void testMapCursor(double x, double z) { mapCurX = x; mapCurZ = z; }
    int testShownMark() const { return guide.shownMark; }
    bool testMarkOnScreen(int mi, int& lx, int& ly) {       // B-412: where a mark's diamond over the view stands (logical pixels); false behind the eye
        for (const SiteMark& m : marksHere())
            if (m.index == mi) {
                double sx, sy;
                if (!surf.projectPoint(m.x, m.y + 6, m.z, sx, sy)) return false;
                lx = (int)(sx / FB_SCALE);
                ly = (int)(sy / FB_SCALE);
                return true;
            }
        return false;
    }
    void testMarkLocal(int mi, double& x, double& z) const { const SurfaceMark& m = guide.marks[mi]; surf.site.localAt(StarSystem::bodyFromLatLon(m.lat, m.lon), x, z); }
    int testMarksHere(int& dry) { dry = 0; for (const SiteMark& m : marksHere()) dry += m.dry ? 1 : 0; return (int)marksHere().size(); }   // the world's marks, and those read without their rivers
    bool testMarkGround(int mi, double& y, double& wet) {   // a mark's ground as the marks hold it, and the ground there with the rivers (ask once its tiles are built)
        for (const SiteMark& m : marksHere()) if (m.index == mi) { y = m.y; const TerrainVertex v = surf.site.sampleAt(m.x, m.z, 64.0); wet = std::max((double)v.h, (double)v.water); return true; }
        return false;
    }
    const BodyGen& testSiteGen() const { return surf.site.gen; }
    void testWalkTo(double x, double z) { surf.player.x = x; surf.player.z = z; surf.player.y = surf.site.surfaceHeight(x, z); }
    double testPlayerX() const { return surf.player.x; }
    double testRange() const { return surf.lastRange; }
    double testRangeX() const { return surf.lastRangeX; }
    double testRangeZ() const { return surf.lastRangeZ; }
    void testLatLonAt(double x, double z, double& lat, double& lon) const { surf.site.latLonAt(x, z, lat, lon); }
    double testPlayerZ() const { return surf.player.z; }
    // ---- the state set, the landing map, what the ground holds, parking (landing_map.cpp, landing_map_build.cpp, game.cpp)
    void setState(GameState s) { state = s; }
    GameState testScene() const { return returnState; }   // B-431: the scene an overlay is drawn over and closes to
    void testAimAtBody(int body);
    int testZoomRoads() const { return (int)zoomRoads.size(); }   // C-09: the roads drawn on the zoom
    const double* testZoomBuildMs() const { return zoomBuildMs; } int testZoomBuilds() const { return zoomBuilds; }   // the zoom's last build in parts, the builds so far
    int testZoomCores() const { return zoomCores; }                // the cores drawn on the zoom
    std::string testLandPeoplesInfo();                               // the landing map's peoples: what is known, what the layer drew
    void testSurfaceFace(double yaw, double pitch) { surf.player.yaw = yaw; surf.player.pitch = pitch; }   // the bench: the explorer's look
    const SurfaceView& testSurface() const { return surf; }   // the bench: the forms drawn
    int testZoomWays(int kind) const { int n = 0; for (int k : zoomRoadKinds) if (k == kind) n++; return n; }   // the zoom's lines of a kind
    std::string testRoadInfo() const;                             // the site's roads, the road under the explorer, the roads met, the zoom's lines
    void testBuildZoom() { buildLandingZoom(); }
    bool testWalkToRoad(double withinM = 400);                    // the explorer set on the nearest road's centre line
    std::string testGraveInfo() const;                            // the grave within reach, the guide's graves, the last log line
    bool testWalkToShard();                                       // the explorer set a metre from the nearest settlement's first shard the guide does not hold
    int testPeoples() const { return surf.peoples; }              // the peoples of the world under the feet
    int testNearShardIndex() const { return surf.nearShard.index; }   // the shard within reach (its world index), -1 none
    // the explorer set before the nearest record of a medium (-1 any) the guide does not hold, looking at it (an archive's from its row's
    // middle when `archive`); its world index, -1 none. What the frame drew of the records' objects
    int testWalkToRecord(int medium = -1, bool archive = false);
    int testNearShardMedium() const { return surf.nearShard.index >= 0 ? surf.nearShard.medium : -1; }
    int testRecordFormsDrawn() const { return surf.lastRecordForms; }
    bool testWalkToGrave();                                       // the explorer set a metre in front of the nearest settlement's first grave, facing it
    bool testWalkToWork(double withinM = 1e9);                    // the explorer set 150 m from the nearest work the scanner hears (the frame re-anchored and the scan asked again from there)
    void testLandCursor(double latDeg, double lonDeg) { landLat = latDeg * DEG; landLon = lonDeg * DEG; }   // the landing map's cursor
    int testDataSheetBottom() const { return dataSheetBottom; }   // the data sheet's lowest next line's top over its pages, as last drawn
    int testDataSheetPages() const { return dataSheetPages; }
    const std::vector<std::string>& testDataSheetText() const { return dataSheetText; }   // B-496: its lines over its pages, as last drawn
    void testRenderDataSheet() { renderDataSheet(); }   // B-496: the local target's sheet drawn over the canvas as it is
    void testTelescopeName(int bi) { tele.target = bi; telescopeName(); }   // B-498: N through the telescope on a body (its text entry opened)
    const std::string& testTextPrompt() const { return textPrompt; }
    void testLandCursorAt(double& latDeg, double& lonDeg, int& zoom) const { latDeg = landLat / DEG; lonDeg = landLon / DEG; zoom = landZoom; }
    void testSectorMapAt(double& x, double& z, int& zoom) const { x = mapCurX; z = mapCurZ; zoom = sectorZoom; }   // the sector map's cursor (local metres) and level
    void testStarMapAt(double& yaw, double& zoom, int& mask, int& level, int& galaxyMarks) const {
        yaw = mapYaw;
        zoom = mapZoom;
        mask = mapClassMask;
        level = gchart.level;
        galaxyMarks = (int)guide.galaxyMarks.size();
    }
    void testLandSite();   // the landing map's cursor on a lit land site chosen from the body's seed (R is seeded by the clock, J follows the moon); S-01: level enough for the buggy
    void testWaitMap();    // the landing map's world mapped before the test's next frame (the game's screen reads it on its thread; `testParkAt` waits)
    void testWaitMapOf(int bi);   // any body's map made before the test's next frame (`testParkAt` waits for a landable world's only)
    void testWaitMaps();   // KI-446: every body's map made for the season at the clock, before the test's next frame (after a new place)
    bool testMapReadyNow() { return landBody >= 0 && landBody < (int)sys.bodies.size() && spaceR.mapIfReady(sys.bodies[landBody]) != nullptr; }
    int testLandableBody() const;
    // arrive at a star, park at a body, open the landing map (F-02: its world mapped first unless `waitMap` is false); J-04: the parking over the
    // night side where `nightSide` (the star behind the world)
    void testParkAt(const Star& s, int body, bool waitMap = true, bool nightSide = false);
    void testParkAtBelt(const Star& s, int k);  // arrive at a star and park beside a rock of belt k at once
    // ---- the buggy, herds, the landing map's colours, fronts and shadows (game.cpp, landing_map.cpp, landing_map_build.cpp)
    bool testGotoHerd(double dist) { return surf.testGotoHerd(dist); }
    uint64_t testKillCritter(int i) { return surf.testKillCritter(i); }   // B-489: a herd's member caught as a hunt catches it (its seed)
    double testBuggySpeed() const { return std::fabs(surf.buggy.speed); }
    bool testBuggyAirborne() const { return surf.buggy.airborne; }
    const double* testRenderMs() const { return surf.lastRenderMs; }   // B-310: the surface render's section times
    bool testDriveSetup();   // deploy the buggy beside the capsule, get in (for the bench)
    double testDriveOpen();  // move the buggy (with the explorer in it) to open ground within 800 m, pointed down the longest clear run; returns the run in metres
    std::string testHerdStates() const { return surf.testHerdStates(); }
    uint32_t testLandingMapTexel(int tx, int ty, bool clouds);   // base colour of the parked body's landing map (full daylight)
    double testFrontBandAt(double latDeg, double lonDeg);        // the fronts' cloud on the landing map's overlay at a place of the parked body now (0..1)
    std::string testFrontInfo() const;                           // the forecast line and the fronts' numbers at the feet
    std::string testForecastLine() const { return forecastLine(); }
    double testMapShadowMax();                                    // the darkest shadow on the landing map now (0..1)
    void testSetTime(double tt) { t = tt; }
    double testT() const { return t; }   // J-10: the game's clock
    const char* testReliefClass(double lat, double lon);         // FLAT / HILLS / MOUNTAINS at a site of the parked body
    // ---- the status, the explorer, the vehicles, the cabin, flight and the aim
    std::string testStatus() const { return statusMsg; }
    bool testStatusOn() const { return realTime < statusUntil && !statusMsg.empty(); }   // J-13: a status line showing
    std::string testStatusNext() const { return statusNext; }   // T-04: what is said when the status now shown expires
    std::string testDebugInfo() const;
    std::string testHealth() const;   // "" when every number is finite
    double testPlayerStamina() const { return surf.player.stamina; }
    double testPlayerSpeed() const { return std::sqrt(surf.player.vx * surf.player.vx + surf.player.vz * surf.player.vz); }
    double testPlayerAlt() const { return surf.player.altAboveGround; }
    std::vector<Vial>& testKit() { return kit; }               // Z-03: the kit's vials and the cold store's
    std::string testVialLine(const Vial& v) const { return vialLine(v); }   // Z-07: a take's line, as the HUD said it
    std::vector<Vial>& testColdStore() { return coldStore; }
    // Z-04: the microscope: its state, a vial put on its stage, the slide's clock skipped (its hook: a frame closed-form in it), the view set
    // (its way, zoom step, place and focus, the magnification snapped to the step's), the frame's specimens made, the species under the
    // crosshair, a species' sheet and key, the status cleared
    MicroscopeState& testScope() { return scope; }
    bool testScopeStage(int vial) { return microscopeStage(vial); }
    void testScopeSkip(double s) { scope.t += s; }
    void testScopeView(int seeing, int step, double cx, double cy, double focus) {
        scope.seeing = seeing; scope.step = step; scope.logMag = std::log(microscopeStepMag(seeing, step));
        scope.cx = cx; scope.cy = cy; scope.focus = focus; scope.tracking = false;
    }
    void testScopeFrame() { microscopeFrame(); }
    int testScopeUnder() const { return microscopeSpeciesUnder(); }
    double testScopeMag() const { return microscopeMag(); }
    std::vector<std::string> testSheetLines(int species) const { return sheetLines(species); }
    std::string testSpeciesKey(int species) const { return speciesKey(species); }
    void testClearStatus() { statusMsg.clear(); statusUntil = 0; }
    std::pair<std::string, double> testStatusKept() const { return {statusMsg, statusUntil}; }   // Z-06: a status kept across a step and put back
    void testStatusPut(const std::pair<std::string, double>& s) { statusMsg = s.first; statusUntil = s.second; }
    // Z-05: the incubator: the lab's state and the screen's, a vial's species put in a flask (or a flask's culture carried over), the lab's clock
    // run on (what it comes to said and logged as a frame's would: its skip hook), a flask's culture now, its controls, a drop on the
    // microscope's stage
    LabState& testLab() { return lab; }
    double& testBayClock() { return bay.clock; }   // the drones' clock, the lab's kind
    IncubatorState& testIncub() { return incub; }
    bool testFlaskPut(int flask, int vial, int species) { IncubatorChoice c; c.vial = vial; c.species = species; return flaskPut(flask, c); }
    bool testFlaskCarry(int flask, int from) { IncubatorChoice c; c.flask = from; return flaskPut(flask, c); }
    void testLabSkip(double s) { updateLab(s); }
    CultureNow testFlaskNow(int i) { if (!lab.flasks[i].made) flaskMake(i); return flaskNow(i); }
    FlaskShow testFlaskShow(int i) { if (!lab.flasks[i].made) flaskMake(i); return flaskShow(i); }   // J-01
    void testFlaskSet(int i, const Medium& m) { flaskSet(i, m); }
    void testFlaskFresh(int i) { flaskFresh(i); }
    void testFlaskSeen(int i) { flaskSeen(i); }
    bool testScopeStageFlask(int i) { return microscopeStageFlask(i); }
    void testLabSave(std::string& lines) const { incubatorSave(lines); }
    // R-432, R-433: the lists' folders, the collection's own, a kept culture on the stage, into a flask; the choosers made
    std::vector<LabFolder> testVialFolders() const { return vialFolders(); }
    std::vector<LabFolder> testCultureFolders() const { return cultureFolders(); }
    int testFolderNew(const std::string& name) { return folderNew(name); }
    bool testFolderRemove(int id) { return folderRemove(id); }
    void testKeptMove(int k, int id) { keptMove(k, id); }
    void testKeptDiscard(int k) { keptDiscard(k); }
    bool testScopeStageKept(int k) { return microscopeStageKept(k); }
    bool testFlaskPutKept(int flask, int k) { IncubatorChoice c; c.kept = k; return flaskPut(flask, c); }
    void testFlaskEmpty(int i) { flaskEmpty(i); }
    // R-435 to R-438: the slide's told answers forgotten (a frame with no history: the closed-form check's), the lines waiting; the choosers made
    void testScopeFresh() { scope.told.clear(); scope.said.clear(); for (int d = 0; d < SD_COUNT; d++) { scope.tried[d] = -1; scope.answered[d] = scope.judged[d] = false; } }
    std::vector<std::string>& testScopeSaid() { return scope.said; }
    std::vector<IncubatorChoice>& testIncubatorChoices() { incubatorChoices(); return incub.choices; }
    std::vector<SequencerChoice>& testSequencerChoices() { sequencerChoices(); return lab.seq.choices; }
    // B-442, R-440 to R-442: K's folder chooser (what it keeps), a kept culture copied to a folder, any species' sheet lines
    KeepPick& testKeep() { return keep; }
    const std::string& testTextBuffer() const { return textBuffer; }   // R-445: what a text entry holds, and what it names
    int testTextKind() const { return textKind; }
    void testKeptCopy(int k, int id) { keptCopy(k, id); }
    std::vector<std::string> testSheetLinesOf(const std::string& key, const Microbe& s, const Morph& m, const MicrobeSheet& seen) const { return sheetLinesOf(key, s, m, "A WORLD", seen); }
    // Z-06: the sequencer: a species put on it (its world and index), its save line, its result told on the lab's clock (the skip hook's); the tree
    // built from the guide, its leaves, a leaf's nearest kin; the decoder's world opened (a watermark among its records)
    bool testSeqPut(int64_t sx, int64_t sy, int64_t sz, int body, int species) { FlaskSpecies w; w.sx = sx; w.sy = sy; w.sz = sz; w.body = body; w.species = species; return sequencerPut(w); }
    bool testSeqPutLine(const FlaskSpecies& w) { return sequencerPut(w); }   // Y-02: a line read as it is
    void testSeqSave(std::string& lines) const { sequencerSave(lines); }
    bool testSeqLoad(const std::string& line) { return sequencerLoad(line); }
    TreeState& testTree() { treeBuild(); return tree; }
    static std::string testTreeLine(const TreeLeaf& L, bool world = true) { return treeLeafLine(L, world); }   // B-443: as drawn
    int testTreeNearest(int i, double& gyr) const { return treeNearest(i, gyr); }
    // Y-02: the gene editor: its state, a culture put on it (read or not), a gene's edit, the writing begun, its chooser, its save lines; a line's
    // kind, form and genes; an edited line's drop looked at (its learning)
    EditorState& testEditor() { return lab.edit; }
    bool testEditorPut(const FlaskSpecies& w, const Medium& m, bool read) {
        EditorChoice c; c.who = w; c.medium = m; c.asWorld = m; c.from = "THE TEST"; c.read = read; return editorPut(c);
    }
    void testEditorSet(int place, bool cut, const std::string& part) { editorSet(place, cut, part); }
    // Y-03: a kin put beside it (its chooser's row where it has one, else as read), the kin's chooser
    bool testEditorKin(const FlaskSpecies& w) {
        for (const EditorChoice& c : lab.edit.choices) if (c.who.sameSpecies(w) && c.who.strain == w.strain) return editorKinPut(c);
        EditorChoice c; c.who = w; c.from = "THE TEST"; c.read = true; return editorKinPut(c);
    }
    std::vector<EditorChoice>& testEditorKinChoices() { editorChoices(true); return lab.edit.choices; }
    std::string testFlaskKey(const FlaskSpecies& w) const { return flaskSpeciesKey(w); }
    bool testEditorWrite(int folder = 0, const std::string& name = "") {   // W, then its folder and its name given (R-445; "" unnamed)
        if (!editorWrite()) return false;
        keep.open = false; lab.edit.writeFolder = folder;
        return editorWriteBegin(name);
    }
    std::vector<EditorChoice>& testEditorChoices() { editorChoices(); return lab.edit.choices; }
    std::vector<std::string> testEditorLetterChoices() { return editorLetterChoices(); }   // R-450
    void testEditorSave(std::string& lines) const { editorSave(lines); }
    bool testEditorLoad(const std::string& line) { return editorLoad(line); }
    bool testFlaskKind(const FlaskSpecies& w, CultureKind& k, Morph& m, MicrobeGenes* g) const { double slept = 0; return flaskKind(w, k, m, slept, g); }
    std::string testLineTag(const FlaskSpecies& w) const { return lineTag(w); }
    std::string testGeneWords(const FlaskSpecies& w, int place) const { return geneWords(w, place); }
    std::string testGeneKey(const FlaskSpecies& w, int place) const { return geneKey(w, place); }
    // Y-01: a page of the tree shown (its rows made again), a species' page, the page's years
    void testTreePage(int p) { tree.page = p; tree.sel = tree.top = 0; treePage(); }
    std::string testTreePageWhere(int p) const { return p >= 0 && p < (int)tree.pages.size() ? treePageWhere(tree.pages[p]) : std::string(); }   // B-497
    void testTreeShow(const std::string& key) { treeShow(key); }
    bool testSpeciesOfKey(const std::string& key, FlaskSpecies& w) const { return speciesOfKey(key, w); }
    double testTreeYears() const { return treeYears(); }
    bool testShardWorld(const std::string& key, int which) { return openShardWorld(key, which); }
    ShardWorld& testShardWorldNow() { return shardWorld; }
    void testListShardsHeld() { listShardsHeld(); }
    std::vector<int>& testShardHeld() { return shardHeld; }
    std::vector<std::string>& testShardWorldKeys() { listShardWorlds(); return shardWorldKeys; }
    // Z-07: the catalogue's state (its rows, its page, its way of seeing) and the statistics' LIFE page
    CatalogueState& testCat() { return cat; }
    std::string testCatLine(const CatalogueRow& r) { return catalogueLine(r); }   // J-12: a row's words
    void testCatCounts() { catalogueCountsMake(); }   // J-12: the LIFE page's seeded worlds
    bool testSeqPutWild(const SequencerChoice& c) { return sequencerPut(c.who, c.wild, c.wildKey); }   // J-12: a line found wild put on the sequencer
    bool testStatsLife() const { return statsLife; }
    int testVisionMode() const { return visionMode; }   // R-434: the activity vision's VISION_ACTIVITY
    void testWalkToBuggy() { if (surf.buggy.deployed) { surf.player.x = surf.buggy.x + 1.5; surf.player.z = surf.buggy.z; } }
    std::string testBuggyInfo() const;
    // the drone
    void testWalkToDrone() { if (surf.drone.deployed) { surf.player.x = surf.drone.x + 1.5; surf.player.z = surf.drone.z; } }
    std::string testDroneInfo() const;
    double testDroneSpeed() const { return std::fabs(surf.drone.speed); }
    double testDroneAlt() const { return surf.drone.altAboveGround; }
    bool testDroneLanded() const { return surf.drone.landed; }
    bool testFlySetup(double altM);   // deploy the drone beside the capsule, get in, lift it to altM over the ground (for the bench)
    void testSetPitch(double p) { surf.player.pitch = p; }
    // J-04: the photo mode's free camera from the eye, raised `up` metres, looking along a yaw and a pitch
    void testFreeCamUp(double up, double yaw, double pitch) { surf.enterFreeCam(); surf.freePos.y += up; surf.freeYaw = yaw; surf.freePitch = pitch; }
    void testSetYaw(double y) { surf.player.yaw = y; }
    double testPitch() const { return surf.player.pitch; }   // the flow's look put back
    std::string testRangeInfo() const { return "range=" + std::to_string((int)surf.lastRange) + " water=" + std::to_string((int)surf.lastRangeWater); }
    int testParkedBelt() const { return ship.parkedBelt; }
    std::string testBeltDraw() const;   // rocks drawn last frame and the bank-20 pixels in the framebuffer
    std::string testMarksInfo() const { return "waypoint=" + std::to_string((int)surf.hasWaypoint) + " zoom=" + std::to_string(sectorZoom) + "/" + std::to_string(sectorMaxLevel()); }
    double testEscapeVelocity() const { return surf.site.escapeVelocity; }
    void testTypeText(const std::string& s);   // feeds text entry as key presses
    std::string testStarName() const { return sys.valid ? starNameOf(sys.star) : ""; }
    std::string testStarGenName() const { return sys.valid ? sys.star.name : ""; }   // the generator's name (never shown, R-401)
    bool testDrainageReady() const;   // the drainage tiles of the landing site are in the cache
    int testBodyType(int bi) const { return bi >= 0 && bi < (int)sys.bodies.size() ? sys.bodies[bi].type : -1; }
    std::string testBodyTypes() const { std::string r; for (auto& b : sys.bodies) r += std::to_string(b.index) + ":" + KINDS[b.type].name + (b.parent >= 0 ? "(moon) " : " "); return r; }
    // I-01: the explorer stood where the plan says (the harness holds no place of its own): at a view by its name (ship_plan.cpp's; a
    // name not there ends the run), or where a device is faced from (its row's stand)
    void testCabinView(const char* name) {
        const ShipView& v = shipView(name);
        if (strcmp(v.name, name) != 0) { fprintf(stderr, "no view \"%s\" in the ship's plan\n", name); exit(2); }
        testCabinPlace(v.room, v.at);
    }
    void testCabinAtDevice(int facing) {
        const CabinDevice& d = cabinDevices().row[facing];
        testCabinPlace(d.room, d.stand);
    }
    // I-02: the head turned to a direction of the galaxy from the window on its side (the great window ahead, a flank's long window aside)
    void testCabinLookOut(const Vec3& dir) {
        const Vec3 d = cameraBasis(ship.yaw, ship.pitch) * dir;
        testCabinView(d.z > std::fabs(d.x) ? "THE FRONT WINDOW" : (d.x > 0 ? "THE RIGHT WINDOW" : "THE LEFT WINDOW"));
        testCabinLook(dir);
    }
    void testCabinPlace(int room, const ShipStand& at) { cabin.placeAt(shipPlaceOf(room, at.x, at.z)); cabin.yaw = at.yaw; cabin.pitch = at.pitch; }   // I-02: a room and a stand
    int testCabinRoom() const { return cabin.room; }
    ShipPlace testCabinPlaceNow() const { return cabin.place(); }
    double testCabinFloor() const { return shipFloorAt(cabin.place()); }
    const CabinFrame& testCabinFrame() const { return cabinFrame; }
    // a step of auto-explore's walk aboard toward a room's place, its hands made into the frame's input (`ship walk`): true there, facing
    bool testShipWalkStep(int room, double x, double z, double yaw, Input& in, double dt) {
        ax.clock += dt; ax.taps.clear(); memset(ax.hold, 0, sizeof(ax.hold)); ax.wantFwd = ax.wantSide = 0; ax.lookSet = false; ax.shipAim = false;
        const bool there = autoCabinWalk(room, x, z, yaw);
        autoHands(in, dt);
        return there;
    }
    void testCabinAt(double& x, double& z, double& yaw) const { x = cabin.x; z = cabin.z; yaw = cabin.yaw; }
    bool testShipScreenRow(const std::string& label) {   // the flight computer's row Enter acts on, by its label (U-06): false if none
        const RowList<ShipPage>& pages = shipPages();
        for (int p = 0; p < pages.count; p++)
            for (int i = 0; i < pages.row[p].rows.count; i++)
                if (label == pages.row[p].rows.row[i].label) { shipScreenPage = p; shipScreenSel = i; return true; }
        return false;
    }
    double testFlightDur() const { return ship.flightDur; }   // the Vimana flight's length (real seconds)
    double testFlightT() const { return ship.flightT; }
    bool testFlying() const { return ship.mode == ShipState::VIMANA; }
    void testFlightSkip(double tt) { if (ship.mode == ShipState::VIMANA) ship.flightT = tt; }   // the flight's clock set (the band's keyframes catch up)
    void testStartJump(double ly, const Vec3& dir) { startJump(ly, dir); }   // R-407's jump along a direction, plotted and flown
    void testCabinTurn(double yaw, double pitch) { cabin.yaw = yaw; cabin.pitch = pitch; }   // the head turned in the cabin
    // I-03: the ship turned so the sun stands at a way in the ship's metres (toward it); false with no sun
    bool testSunAt(const Vec3& want) {
        if (!sys.valid || sys.sunless()) return false;
        const Vec3 d = sys.lightAt(ship.pos, t).dir, w = normalize(want);
        double bestYaw = 0, bestPitch = 0, best = 1e30;
        for (int step = 0; step < 4; step++) {   // a grid, then finer grids round the best
            const double span = step == 0 ? PI : PI / std::pow(12.0, step), y0 = step == 0 ? 0 : bestYaw, p0 = step == 0 ? 0 : bestPitch;
            for (int i = -24; i <= 24; i++)
                for (int j = -24; j <= 24; j++) {
                    const double yaw = y0 + span * i / 24, pitch = clampd(p0 + span / 2 * j / 24, -PI / 2, PI / 2);
                    const double e = length(cameraBasis(yaw, pitch) * d - w);
                    if (e < best) { best = e; bestYaw = yaw; bestPitch = pitch; }
                }
        }
        ship.yaw = bestYaw; ship.pitch = bestPitch;
        return best < 0.01;
    }
    void testShipTurnBy(double dyaw) { ship.yaw += dyaw; }
    double testShipYaw() const { return ship.yaw; }
    double testShipPitch() const { return ship.pitch; }
    void testShipAttitude(double yaw, double pitch) { ship.yaw = yaw; ship.pitch = pitch; }
    // the head turned toward a direction of the galaxy, level (at the windows)
    void testCabinLook(const Vec3& dir) { const Vec3 d = cameraBasis(ship.yaw, ship.pitch) * dir; cabin.yaw = std::atan2(d.x, d.z); cabin.pitch = 0; }
    const SpaceRenderer::FlightBand& testFlightBand() const { return spaceR.fband; }
    long testParkedBandBuilds() const { return spaceR.parkedBandBuilds; }
    bool testAiming() const { return ship.targeting; }
    std::string testJumpDigits() const { return jumpDigits; }
    Vec3 testAimDirection() const { return aimDirection(); }
    std::string testAimLabel(const Star& s) const { Star full; starInSector(s.sx, s.sy, s.sz, full, true); return aimStarLabel(full); }
    const StarNeighborhood& testNeighborhood() const { return nb; }
    Vec3 testShipPos() const { return ship.pos; }
    // ---- the galaxy chart (galaxy_chart.cpp)
    int testMapPage() const { return mapPage; }
    void testChartLevel(int level) { gchart.level = std::max(0, std::min(CHART_LEVELS - 1, level)); }
    void testChartFollow() { gchart.follow = true; }
    double testChartBuildMs() const { return gchart.buildMs; }
    int testChartYouX() const { return gchart.youX; }
    int testChartYouY() const { return gchart.youY; }
    void testArriveAt(const Star& s) { arriveAtStar(s, Vec3(0, 0, -1)); }   // the arrival as a Vimana flight ends it
    void testSetRemote(const Star& s) { setRemoteStar(s); }                     // D-01: the remote target set as the guide's targeting sets it
    bool testRemoteStar(Star& out) const { out = ship.remote; return ship.hasRemote; }
    void testPlaceShip(const Vec3& posKm) { ship.pos = posKm; ship.mode = ShipState::STANDBY; nb.update(ship.pos); }   // the ship set down anywhere (the harness's chart)
    // the chart's cursor and plot
    void testChartCursor(const Vec3& p) {
        gchart.onShip = false;
        gchart.plotHere = false;
        gchart.curX = p.x;
        gchart.curY = p.y;
        gchart.curZ = p.z;
        if (gchart.level > 0) {
            gchart.follow = false;
            gchart.cx = p.x;
            gchart.cz = p.z;
        }
    }
    Vec3 testChartCursorAt() const { return chartCursor(); }
    bool testChartOnShip() const { return gchart.onShip; }
    bool testChartPlotted() const { return chartPlotted(); }
    bool testChartLook() const { return gchart.look; }
    const StarNeighborhood& testLookStars() const { return gchart.lookNb; }
    int testChartCurX() const { return gchart.curPX; }
    int testChartCurY() const { return gchart.curPY; }
    double testJumpShort() const { return jumpPlot.shortLy; }
    int testMapPick() const { return mapPick; }
    int testCabinFacing() const { return cabin.facing; }   // 7 is the shard decoder
    int testCabinLights() const { return (cabin.light ? 1 : 0) | (cabin.depolarised ? 2 : 0); }   // 1 the light on, 2 the hull glass
    int testLogEntries() const { return (int)guide.log.size(); }
    // ---- life aboard (cabin_life.cpp, I-04): the prints made and waited for (only the harness waits), their count; the drive's level; the
    // descent's clock (the hatch's drop under 0); the shelves' worlds; the garden's home and its growth
    void testWaitPrints() {   // (Y-03: again while stale: an ask while an older job runs is refused, its prints the old ones)
        for (int guard = 0; guard < 4; guard++) {
            if (prints.stale || prints.dir != shotsDir) printsAsk();
            while (prints.busy.load()) std::this_thread::sleep_for(std::chrono::milliseconds(2));
            printsCollect();
            if (!prints.stale && prints.dir == shotsDir) break;
        }
    }
    int testPrints() const { return (int)prints.shown.size(); }
    int testPrintAt(int k) const { for (const ShipPrint& p : prints.shown) if (p.frame == k) return p.number; return -1; }   // R-428: frame k's print
    bool testWallChosen() const { return wall.chosen; }
    int testWallNumber(int k) const { return wallNumber(k); }   // what frame k shows (the newest twelve until the explorer hangs one)
    int testWallLevel() const { return wall.level; }
    void testCabinDrive(double d) { cabin.drive = d; }
    static constexpr double DROP_SECONDS = Game::DROP_S;
    double testTransT() const { return transT; }
    int testShelfWorlds() const { return (int)shelfCounts.size(); }
    bool testGardenHome() const { return garden.ok; }
    double testGardenGrowth(int bed = GARDEN_BEDS - 1) const { return gardenGrowth(bed); }   // R-429: a bed's (the last: `THE GARDEN`'s view)
    double testPlayed() const { return played; }
    void testSetPlayed(double s) { played = s; }
    void testGardenPlanted() { played = 0; for (double& b : bedTrimmed) b = 0; }   // the time played and every bed as at an expedition's start
    // ---- the almanac (almanac.cpp)
    bool testAlmanacRunning() const { return almanacRunning; }
    double testAlmanacTarget() const { return almanacRunT; }
    double testTimeWarp() const { return timeWarp; }
    int testAlmanacEvents() const { return (int)almanacEvents.size(); }
    std::string testAlmanacInfo() const;                 // the place, its state line, the events with their countdowns
    bool testAlmanacSelect(int kind, int body = -1);     // the open menu's selection onto the first event of a kind (of a body; kind -1 any); false when none
    // ---- J-02: the seeded worlds' layers (game.cpp's `madeTick`): the harness's test layer on a world of the system, the guide's clock
    SpaceRenderer& testSpaceR() { return spaceR; }   // KI-461, KI-475: its discs' made banks
    void testMadeSet(int bi, const MadeWorld& m) { madeTest[Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, bi)] = m; madeTick(0); }
    void testMadeClear() { madeTest.clear(); madeTick(0); }
    void testClockSkip(double s) { madeTick(s); }   // the guide's clock moved on in one step (a frame moves it by its real seconds)
    MadeWorld testMadeNow(int bi) { return madeNow(bi); }
    // J-03: the seed page's cursor set (and what lies there read); the bay; a world's seedings worked out; a vial's seeded lines
    void testSeedPage(double lat, double lon) { bay.pageLat = lat; bay.pageLon = lon; seedPagePlace(); }
    std::string testSeedCulture() const { return seedCultureWords(bay.pagePick); }
    std::string testSeedWhere() const { return bay.pageWhere; }   // R-451: the page's THERE
    void testUnpark() { ship.mode = ShipState::STANDBY; ship.parkedBody = -1; }   // out of orbit, where it was
    bool testBayReady() const { return droneBayReady(); }
    using Game::VialLine;
    const DroneBay& testBay() const { return bay; }
    const std::vector<Seeding>& testSeedings(int bi) { return seedingsOf(bi); }
    std::vector<VialLine> testVialLines(const Vial& v, Sample& smp) { return vialLines(v, sys, smp); }
    std::string testMadeWords(int bi) { return madeWords(bi); }
    uint64_t testMadeKeyHeld(int bi) const { return sys.valid && bi >= 0 && bi < (int)sys.bodies.size() ? spaceR.madeOf(sys.bodies[bi].seed).key : 0; }
    uint64_t testGenMadeKey(int bi) { return spaceR.genFor(sys.bodies[bi]).made.key; }   // the generator the views read
    const PlanetMap& testMapOf(int bi) { return spaceR.mapFor(sys.bodies[bi]); }
    uint64_t testSiteMadeKey() const { return surf.site.gen.made.key; }
    // ---- the telescope (telescope.cpp)
    bool testTelescopeOn() const { return tele.on; }
    double testTelescopeZoom() const { return telescopeZoom(); }
    int testTelescopeTarget() const { return tele.target; }          // a body index, TELE_SUN, TELE_STAR or TELE_NONE
    int testTelescopeTrack() const { return tele.track; }
    std::string testTelescopeInfo() const;                           // the magnification, the field, the reticle's line, what is tracked, the attitude
    std::string testTelescopeLine(bool plain = false) const { return telescopeTargetLine(plain); }
    std::string testTelescopeLineOf(int bi) { const int was = tele.target; tele.target = bi; const std::string l = telescopeTargetLine(); tele.target = was; return l; }
    std::string testTreeWorldWords(const std::string& world, int kind) { tree.kinds[world] = kind; return treeWorldWords(world); }   // KI-482
    bool testBodyScreen(int body, double& sx, double& sy, double& rpx) const;   // where the last frame drew a body (framebuffer pixels); false when behind
    int testParkedBody() const { return ship.parkedBody; }
    int testLocalTarget() const { return ship.localTarget; }   // D-02: the drifter targeted on arrival
    void testSetLocalTarget(int b) { ship.localTarget = b; ship.targetBelt = -1; }   // B-501: a body of the system the local target
    double testBodyRadiusKm(int body) const { return sys.valid && body >= 0 && body < (int)sys.bodies.size() ? sys.bodies[body].radiusKm : 0; }
    int testTelescopePick() const;                                   // the body with the widest disc from the ship, the parked one left out (-1 none)
    void testAimAtSun() { if (sys.valid) { Vec3 f = normalize(sys.star.pos - ship.pos); ship.yaw = std::atan2(f.x, f.z); ship.pitch = std::asin(clampd(f.y, -1, 1)); } }
    bool testTelescopeStabilised() const { return telescopeStabilised(); }
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
    int testScreenColours(double& minDist, int& banks) const;    // the computers with a colour, their banks, the smallest distance between two
    // ---- the giant probe (probe.cpp, probe_feed.cpp)
    bool testProbeActive() const { return probe.active; }
    bool testProbeEnded() const { return probe.active && probe.endClock >= 0; }
    int testProbeStage() const;
    double testProbeClock() const { return probe.clock; }
    double testProbeBar() const;
    double testProbeEndClock() const;
    std::string testProbeInfo() const;
    std::string testProbeViewInfo() const;   // X-02: the last probe frame's layers, coverage and section costs
    void testProbeSkip(double toClock);         // the probe's clock set forward (the end found on the way, as the frame would)
    int testProbeStageAt(double clock) const { return probe.active ? probeStageAt(probe.atm, probe.flight, clock) : -1; }
    double testProbeBarAt(double clock) const { return probe.active ? probeBarAt(probe.flight, clock) : 0; }
    void testProbeChute() {
        if (probe.active && probe.endClock < 0 && probe.flight.chuteOpen < 0 && probe.clock >= probeDescentStart()) {
            probe.flight.chuteOpen = probe.clock;
            probe.flight.chuteClose = probeChuteCloseAt(probe.flight, probe.clock);
        }
    }
    void testProbeLook(double yaw, double pitch) { probe.camYaw = yaw; probe.camPitch = pitch; }
    double testProbeFlash(double clock) const;    // the lightning's strength in the probe's picture at a clock (0 none)
    bool testProbeBolt(double clock) const;       // whether that flash throws a channel through the clear air
    bool testProbeAimed(double& lat, double& lon) const { lat = probe.aimLat; lon = probe.aimLon; return probe.active; }   // where the probe was sent
    bool testProbeHolePath() const { return probe.active && probe.holePath; }
    const GiantAtmosphere& testProbeAtm() const { return probe.atm; }
    double testGameTime() const { return t; }
    void testAdvanceTime(double dt) { t += dt; }
    double testSunHeight() const { return surf.env.sun.dirLocal.y; }   // the sun's height over the horizon (its direction's up component)
    const StarSystem& testSystem() const { return sys; }
    void testProbeRelease() { probeRelease(); }
    double testProbeSightClock() const { return probe.active ? probe.sightClock : -1; }
    // ---- recordings, the gallery, the data sheet's profiles (probe_records.cpp, guide_screens.cpp)
    void testRecordingSeek(double clock, bool playing) { replay.clock = clock; replay.playing = playing; }
    double testRecordingClock() const { return replay.clock; }
    void testOpenGallery() { returnState = GameState::SPACE; openScreen(GameState::GALLERY); }   // as the guide opens it on the ship
    int testGalleryItems() const { return (int)galleryItems.size(); }
    int testGalleryRec(int i) const { return i >= 0 && i < (int)galleryItems.size() ? galleryItems[i].rec : -2; }
    int testGallerySel() const { return gallerySel; }
    void testGalleryPick(int i) { gallerySel = i; }
    void testProbeLookAt(double yaw, double pitch) { probe.camYaw = yaw; probe.camPitch = pitch; }
    // ---- the tour (U-01, modes_tour.cpp): the open menu's rows by label, and what the open screen has selected
    std::vector<std::string> testMenuRows() const;   // the Esc menu's, the settings' (BACK last), the guide's, the flight computer's page; none elsewhere
    int testMenuSel() const;                         // that menu's selected row (-1 without one)
    std::string testSelection() const;               // the open screen's row, page or level as the tour prints it (no return variable)
    void testSettle();                               // what threads build waited for at one instant: the system's maps, the scanner's search
};
