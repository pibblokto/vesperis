// E-03: the scripted game: `flow` (an expedition), `settings`, `audio`, `zoom`, `parkings`, `tape`, `tail`, `auto`
#include "harness/harness.h"

// the flow's state: the game, its input, the clock, and what every stage does to them (run, press, shot); each stage is a
// member function, so that its code reads as it did in one function
struct Flow {
    TestGame game;
    Input in;
    double t0 = 0;
    void run(double seconds) {
        int n = (int)(seconds * 30);
        for (int i = 0; i < n; i++) { game.frame(in, 1.0 / 30); if (game.recording) game.recordFrame(); in.newFrame(); }
    }
    void press(int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; }
    void shot(const char* name) {
        std::string fn = std::string("shots/tests/flow_") + name + ".png";
        writePNG(fn.c_str(), game.output(), FBW, FBH);
        printf("%-14s state=%d status='%s'  (%.1f s)\n", name, (int)game.state, game.testStatus().c_str(), nowSec() - t0);
    }
    void title();   // the title, the attract mode, the credits, space, the cabin, the console and the ship's screen
    void decoder();   // the shard decoder on the back wall
    void cabin();   // the roof, the glass hull, the back wall's sets, the case, the telescope's set, the list, the data, the almanac
    void telescope();   // the telescope; then the help screen and the orbit view
    void radar();   // the signal radar
    void starmap();   // the star renamed, the star map and the galaxy, targeting, the flight and the arrival
    void landing();   // the landing and the ground: the approach, the descent, the walk, the buggy, the water
    void comet();   // a comet
    void belt();   // a belt
    void chart();   // the next jump chosen on the chart
    void probe();   // the probe
    void roads();   // the old roads on the landing zoom
    void reload();   // the saves loaded again: the newest, then slot 2 mid-descent
};

void Flow::title() {
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.shotsDir = "shots/tests/test_gallery";
    game.moviesDir = "shots/tests/test_movies"; game.recFixedTake = true;   // M6-05: the recorder writes into shots/, take 1 overwritten
    remove("shots/tests/test_guide.txt");
    applyTestScale(game);
    t0 = nowSec();
    run(0.5); shot("title");
    game.testAttract(30); run(0.3); shot("attract");
    press(KEY_C); run(0.1); shot("credits"); press(KEY_ESCAPE); run(0.1);
    press(KEY_ENTER);
    for (int i = 0; i < (int)game.testSystem().bodies.size(); i++) game.testWaitMapOf(i);   // the system's maps first (made on their thread at a binary's own pace)
    run(1.0); shot("space"); printf("  %s\n", game.testDebugInfo().c_str());
    // walk to the GOES console, use it, then the front computer, the roof and the glass hull
    game.testCabinGoto(-1.2, 0.0, -PI / 2); run(0.2); shot("cabin"); printf("  facing %d\n", game.testCabinFacing());
    press(KEY_E); run(0.1); game.testTypeText("SL 4"); run(0.1); shot("console"); press(KEY_ESCAPE);
    game.testCabinGoto(0.0, 1.3, 0.0); run(0.2); press(KEY_E); run(0.1); shot("shipscreen"); press(KEY_ESCAPE);
}

void Flow::decoder() {
    {
        // C-06: the shard decoder on the back wall: empty; with three shards of Aieliaalas II in the guide (the first reading waits, the
        // words resolving); with ten (every shard re-read, the whole text)
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); shot("decoder"); printf("  facing %d (7 is the decoder)\n", game.testCabinFacing());   // looking down at the desk from 1.5 m
        press(KEY_E); run(0.1); shot("shards_none"); press(KEY_ESCAPE); run(0.1);
        for (int i = 0; i < 3; i++) game.guide.shards.insert("151,0,25/1/S" + std::to_string(i));
        press(KEY_E); run(0.1); shot("shards_worlds"); press(KEY_ENTER); run(0.1); shot("shards_list");
        // by index, S1 of the three being a piece; C-05: the words resolve as the recording speaks
        game.testShardsOpenIndex(2); run(1.2); shot("shards_decoding"); printf("  %s; the synth: %s\n", game.testShardsInfo().c_str(), game.audio.speech ? "a speech set" : "NO speech set");
        press(KEY_ENTER); run(0.2); shot("shards_text"); printf("  the wait skipped: %s\n", game.testShardsInfo().c_str());
        press(KEY_RIGHT); run(0.1); press(KEY_ENTER); run(0.2); printf("  the next, the wait skipped: %s\n", game.testShardsInfo().c_str());
        press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        for (int i = 3; i < 10; i++) game.guide.shards.insert("151,0,25/1/S" + std::to_string(i));
        press(KEY_E); run(0.1); press(KEY_ENTER); run(0.1); shot("shards_list10");
        game.testShardsOpenIndex(0); run(0.2); press(KEY_ENTER); run(0.2); shot("shards_full"); printf("  %s\n", game.testShardsInfo().c_str());
        // a read shard spoken again, the word being spoken underlined
        press(KEY_ENTER); run(1.5); shot("shards_speaking"); printf("  spoken again: %s; the synth: %s\n", game.testShardsInfo().c_str(), game.audio.speech ? "a speech set" : "NO speech set");
        press(KEY_SPACE); run(0.3); printf("  paused: %s\n", game.testShardsInfo().c_str());
        press(KEY_ESCAPE); run(0.1); printf("  after esc: level %d, the synth: %s\n", game.testShardsLevel(), game.audio.speech ? "a speech STILL set" : "stopped");
        {   // a long record (the longest of the people's first sixty, added to the guide) on the decoder: its opening spoken, Down turns the page
            int longest = -1, most = 0;
            { Star ms; StarSystem msys; if (starInSector(151, 0, 25, ms, true)) { msys.generate(ms); const Body& mb = msys.bodies[1]; BodyGen mg = BodyGen::make(mb); Lore mL = loreOf(msys, mb, mg);
                    for (int i = 0; i < 60 && i < mL.records; i++) {
                        Shard x = shardOf(msys, mb, mg, mL, i);
                        int nw = shardWords(x.text);
                        if (!x.music && !x.chart && nw > most) {
                            most = nw;
                            longest = i;
                        }
                    }
                }
            }
            if (longest >= 0) {
                game.guide.shards.insert("151,0,25/1/S" + std::to_string(longest));
                press(KEY_ESCAPE); run(0.1); press(KEY_ENTER); run(0.1);   // the world's list again, the long record in it
                if (game.testShardsOpenIndex(longest)) {
                    run(1.5); shot("shards_long"); printf("  a long record S%d (%d words), its opening spoken: %s\n", longest, most, game.testShardsInfo().c_str());
                    press(KEY_DOWN); run(0.2); shot("shards_long_page2"); printf("  Down: %s\n", game.testShardsInfo().c_str());
                    press(KEY_ENTER); run(1.0); printf("  Enter reads the page: %s; the synth: %s\n", game.testShardsInfo().c_str(), game.audio.speech ? "a speech set" : "NO speech set");
                    press(KEY_ESCAPE); run(0.1);
                } else printf("  the long record S%d could not be opened\n", longest);
            }
        }
        {   // the first piece of music among the ten plays on the screen, is named, and stops when the screen is left
            int mi = -1;
            for (int i = 0; i < 10 && mi < 0; i++) {
                Star ms;
                StarSystem msys;
                if (starInSector(151, 0, 25, ms, true)) {
                    msys.generate(ms);
                    const Body& mb = msys.bodies[1];
                    BodyGen mg = BodyGen::make(mb);
                    Lore mL = loreOf(msys, mb, mg);
                    if (shardOf(msys, mb, mg, mL, i).music) mi = i;
                }
            }
            if (mi >= 0 && game.testShardsOpenIndex(mi)) {
                run(2.0); shot("shards_music"); printf("  %s; the synth: %s\n", game.testShardsInfo().c_str(), game.audio.piece ? "a piece set" : "NO piece set");
                press(KEY_N); run(0.1); shot("shards_music_name"); game.testTypeText("THE RIVER AT NIGHT"); run(0.3); shot("shards_music_named");
                printf("  named: '%s'; %s\n", game.guide.names.count("151,0,25/1/S" + std::to_string(mi)) ? game.guide.names["151,0,25/1/S" + std::to_string(mi)].c_str() : "-",
                        game.testShardsInfo().c_str());
                press(KEY_ESCAPE); run(0.1); printf("  after esc: level %d, the synth: %s\n", game.testShardsLevel(), game.audio.piece ? "a piece STILL set" : "stopped");
            } else printf("  no piece of music among the first ten shards (index %d)\n", mi);
        }
        {   // the first star chart among the fifty, added to the guide and opened by index: the reading (the stars appear, then the
            // figures, then their names), the wait skipped, the chart held up, the view turned onto the star it marks in space, then put down
            int ci = -1;
            {
                Star ms;
                StarSystem msys;
                if (starInSector(151, 0, 25, ms, true)) {
                    msys.generate(ms);
                    BodyGen mg = BodyGen::make(msys.bodies[1]);
                    for (int i = 0; i < SHARDS_PER_WORLD && ci < 0; i++)
                        if (shardIsChart(mg, i)) ci = i;
                }
            }
            if (ci >= 0) {
                game.guide.shards.insert("151,0,25/1/S" + std::to_string(ci));
                press(KEY_ESCAPE); run(0.1); press(KEY_ENTER); run(0.1);   // the world's list again, the chart in it
                if (game.testShardsOpenIndex(ci)) {
                    run(2.2); shot("shards_chart_reading"); printf("  %s\n", game.testChartInfo().c_str());
                    press(KEY_ENTER); run(0.2); shot("shards_chart"); printf("  the wait skipped: %s\n", game.testChartInfo().c_str());
                    press(KEY_O); run(0.1); press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
                    game.testAimAtChartMark(); run(0.3); shot("chart_overlay"); printf("  held up in space, the view on the marked star: %s\n", game.testChartInfo().c_str());
                    game.testAimAtChartMark(0); run(0.3); shot("chart_overlay_figure"); printf("  the view on its first figure: %s\n", game.testChartInfo().c_str());
                    game.testCabinGoto(0.0, -0.8, PI, -0.65);
                    run(0.1);
                    press(KEY_E);
                    run(0.1);
                    press(KEY_ENTER);
                    run(0.1);
                    game.testShardsOpenIndex(ci);
                    run(0.1);
                    press(KEY_O);
                    run(0.1);
                    printf("  put down: %s\n", game.testChartInfo().c_str());
                    press(KEY_ESCAPE); run(0.1);
                } else printf("  the chart S%d could not be opened\n", ci);
            } else printf("  no star chart among the fifty\n");
        }
        {   // the world's last recording added to the guide and read (the wait skipped): the log notes it, the list's timeline closes
            int li = -1; { Star ms; StarSystem msys; if (starInSector(151, 0, 25, ms, true)) { msys.generate(ms); li = lastShardOf(BodyGen::make(msys.bodies[1])); } }
            if (li >= 0) {
                game.guide.shards.insert("151,0,25/1/S" + std::to_string(li));
                press(KEY_ESCAPE); run(0.1); press(KEY_ENTER); run(0.1);   // the world's list again, the last in it
                if (game.testShardsOpenIndex(li)) {
                    run(0.3);
                    press(KEY_ENTER);
                    run(0.2);
                    shot("shards_last");
                    printf("  the last recording S%d read: %s; worlds ended %zu; last log: %s\n", li, game.testShardsInfo().c_str(), game.guide.ended.size(),
                            game.guide.log.empty() ? "-" : game.guide.log.back().text.c_str());
                    press(KEY_ESCAPE); run(0.1); shot("shards_timeline");
                    // the world's pages: Tab to the timeline of its eras, Tab to its history as the shards read tell it, Tab back to the records
                    press(KEY_TAB); run(0.1); shot("shards_eras");
                    press(KEY_TAB); run(0.1); shot("shards_history"); printf("  %s\n", game.testHistoryInfo().c_str());
                    { const std::vector<std::string> hl = game.testLegendLines(); for (size_t k = 0; k < hl.size() && k < 6; k++) printf("    %s\n", hl[k].c_str()); }
                    press(KEY_TAB); run(0.1); printf("  back on page %d\n", game.testShardsPage());
                } else printf("  the last recording S%d could not be opened\n", li);
            } else printf("  no last recording found\n");
        }
        {   // a world of two peoples on the decoder: three shards of each people of Eleinewai I (152 0 92, the Ghotokhor and the
            // Khokurt) added to the guide, the decoder opened again lists the world once a people, named; the second people's list opened
            for (int i = 0; i < 3; i++) { game.guide.shards.insert("152,0,92/0/S" + std::to_string(i)); game.guide.shards.insert("152,0,92/0/S" + std::to_string(shardWorldIndex(1, i))); }
            press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
            game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
            shot("shards_peoples"); printf("  the worlds' list with a world of two peoples: %s\n", game.testShardsInfo().c_str());
            press(KEY_DOWN); press(KEY_DOWN); run(0.1); press(KEY_ENTER); run(0.1);
            shot("shards_second_people"); printf("  the second people's list: %s\n", game.testShardsInfo().c_str());
        }
        {   // lending: the guide so far (the shards taken and read above, the piece named) saved as the inbox file, then a fresh guide in
            // the same game imports it (the names into the inbox, the shards found and read lent): its decoder lists the world with the lent rows
            // in cyan and plays the friend's piece under the friend's name; the guide is put back after (a second `Game` alive beside the first
            // corrupts the first: the harness keeps to one)
            press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
            game.guide.save("shots/tests/test_guide_inbox.txt");
            Guide kept = game.guide; game.guide = Guide(); game.guide.genVersion = kept.genVersion;
            game.importInboxFile();
            printf("  a fresh guide imports the inbox: '%s'; %zu names in the inbox, %zu shards lent\n", game.testStatus().c_str(), game.guide.inbox.size(), game.guide.lent.size());
            game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_ENTER); run(0.1);
            shot("shards_lent"); printf("  the lent world's list: %s\n", game.testShardsInfo().c_str());
            int pi = -1;
            {
                Star ms;
                StarSystem msys;
                if (starInSector(151, 0, 25, ms, true)) {
                    msys.generate(ms);
                    BodyGen mg = BodyGen::make(msys.bodies[1]);
                    for (int i = 0; i < SHARDS_PER_WORLD && pi < 0; i++)
                        if (shardIsMusic(mg, i) && game.guide.lent.count("151,0,25/1/S" + std::to_string(i))) pi = i;
                }
            }
            if (pi >= 0 && game.testShardsOpenIndex(pi)) { run(0.5); shot("shards_lent_piece"); printf("  the friend's piece S%d: %s\n", pi, game.testShardsInfo().c_str()); }
            else printf("  no lent piece to open\n");
            press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
            game.guide = kept; game.guide.save(game.guidePath);
            remove("shots/tests/test_guide_inbox.txt");
        }
        {   // the ship's player: Tab at the decoder's worlds turns to it (the pieces and recordings held, what the radar caught); the first
            // piece played and left playing as the decoder closes: the radar below holds it, the flight carries it on the space HUD, the landing stops it
            game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
            press(KEY_TAB); run(0.1);
            const int items = game.testPlayerItems();
            int first = -1; for (int i = 0; i < items && first < 0; i++) if (game.testPlayerItemLine(i).find(" music") != std::string::npos) first = i;
            for (int k = 0; k < first; k++) press(KEY_DOWN);
            if (first >= 0) press(KEY_ENTER);
            run(0.3); shot("player"); printf("  the player's page: %d items, item %d played: %s\n", items, first, game.testPlayerInfo().c_str());
            press(KEY_TAB); run(0.1); press(KEY_ESCAPE); run(0.1);   // the worlds' page again (the one the decoder opens on), closed: the piece plays on
            printf("  the decoder closed: %s\n", game.testPlayerInfo().c_str());
        }
        press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        printf("  decoder: state %d after leaving, %zu shards decoded in the guide\n", (int)game.state, game.guide.decoded.size());
    }
}

void Flow::cabin() {
    press(KEY_PAGE_UP); run(0.3); shot("roof"); press(KEY_PAGE_DOWN); run(0.1);
    // B-411: the glass hull with the light on, across the room: what stands in it solid, a sheen on the glass
    press(KEY_Y); game.testCabinGoto(1.4, -0.9, -0.35, -0.2); run(0.6); shot("cabin_glass_lit");
    press(KEY_U); run(1.0); shot("cabin_glass"); press(KEY_Y); press(KEY_U); run(0.1);
    game.testCabinGoto(0.0, 0.2, PI, -0.42); run(0.2); shot("cabin_back");   // the back wall's three sets in their colours (the decoder, the radar, the telescope), the head bent to the desks
    game.testCabinGoto(-1.1, -1.38, -PI / 2, -0.55); run(0.2); shot("cabin_case"); printf("  facing %d (10 is the instruments' case)\n", game.testCabinFacing());   // the case on the left wall
    // the telescope's set, from where its screen is in the frame
    game.testCabinGoto(1.55, -0.95, PI, -0.6); run(0.2); shot("cabin_telescope"); printf("  facing %d (9 is the telescope)\n", game.testCabinFacing());
    press(KEY_E); run(0.3); shot("telescope_set"); printf("  from the set: %s\n", game.testTelescopeInfo().c_str()); press(KEY_Z); run(0.1);
    press(KEY_TAB); run(0.1); shot("list"); press(KEY_ESCAPE);
    press(KEY_I); run(0.1); shot("data"); press(KEY_SPACE);
    // the system's events from the parking
    in.down[KEY_LEFT_CONTROL] = true; press(KEY_T); in.down[KEY_LEFT_CONTROL] = false; run(0.1); shot("almanac_ship"); printf("  %s\n", game.testAlmanacInfo().c_str()); press(KEY_ESCAPE); run(0.1);
}

void Flow::telescope() {
    {   // W-06: the telescope: the parked world at x2 (tracked from the start), the widest other body tracked at the power that fits it, named, photographed (the gallery's first picture)
        game.testAimAtBody(game.testParkedBody()); press(KEY_Z); run(0.3); shot("telescope"); printf("  %s\n", game.testTelescopeInfo().c_str());
        int other = game.testTelescopePick();
        if (other >= 0) {
            press(KEY_ENTER); game.testAimAtBody(other); run(0.1); press(KEY_ENTER); run(0.1);   // let the world go, aim at the other body, track it
            double sx, sy, rpx;
            for (int i = 0; i < 9 && game.testBodyScreen(other, sx, sy, rpx) && 2 * rpx * 2 < 0.5 * FBH; i++) { press(KEY_EQUAL); run(0.05); }
            run(0.3); shot("telescope_moon"); printf("  %s\n", game.testTelescopeInfo().c_str());
            int steps = 0;   // deeper until the eyepiece's step is finer than the map's texel and the plate begins (a small far moon needs a few)
            while (game.testTelescopePlateProgress() < 0 && steps < 6) { press(KEY_EQUAL); run(0.05); steps++; }
            printf("  %d steps deeper: %s\n", steps, game.testTelescopePlate().c_str());
            // the plate read out whole: the moon's ground from the function
            game.testTelescopeSettle(); shot("telescope_plate"); printf("  settled: %s\n  %s\n", game.testTelescopePlate().c_str(), game.testTelescopeInfo().c_str());
            for (int i = 0; i < 2; i++) { press(KEY_EQUAL); run(0.05); }
            game.testTelescopeSettle(); shot("telescope_plate_deep"); printf("  two steps deeper, settled: %s\n  %s\n", game.testTelescopePlate().c_str(), game.testTelescopeInfo().c_str());
            for (int i = 0; i < steps + 2; i++) { press(KEY_MINUS); run(0.05); }
            // the typing ends with Enter
            press(KEY_N); run(0.1); shot("telescope_name"); game.testTypeText("THE NEAR MOON"); run(0.2); shot("telescope_named"); printf("  named: %s\n", game.testTelescopeLine(true).c_str());
        }
        press(KEY_P); run(0.05);
        if (game.wantsScreenshot) {   // the photo as the platform takes it: the frame and the caption beside it
            game.wantsScreenshot = false; mkdir("shots/tests/test_gallery", 0755);
            writePNG("shots/tests/test_gallery/screenshot_1.png", game.output(), FBW, FBH);
            { FILE* sc = fopen("shots/tests/test_gallery/screenshot_1.txt", "w"); if (sc) { fprintf(sc, "%s\n", game.screenshotCaption().c_str()); fclose(sc); } }
            game.noteScreenshot();
            printf("  photo: %s\n", game.screenshotCaption().c_str());
        }
        press(KEY_Z); run(0.1); printf("  stowed: %s\n", game.testTelescopeInfo().c_str());
        // a second of Shift+Right carries the ship round the world, the world kept in the window (after the telescope, whose moon is lit
        // from the parking it had)
        game.testAimAtBody(game.testParkedBody()); game.testCabinGoto(0.0, -1.0, 0.0, 0.0); run(0.2);
        in.down[KEY_LEFT_SHIFT] = true; in.down[KEY_RIGHT] = true; run(1.0); in.down[KEY_RIGHT] = false; in.down[KEY_LEFT_SHIFT] = false;
        shot("orbit_turn"); printf("  Shift+Right for a second: '%s'\n", game.testStatus().c_str());
    }
    press(KEY_H); run(0.1); shot("help"); press(KEY_ESCAPE);
    press(KEY_O); run(0.5);
}

void Flow::radar() {
    {   // C-07: the signal radar: on at the start, a sweep of the sky (the scope filling), the view turned onto the first people's
        // signal within reach by the hook (the recording heard through the static), the hold, the lock, Enter targets its star, off
        // the set beside the decoder, looked down at from 1.6 m
        game.testCabinGoto(-1.4, -1.1, PI, -0.6); run(0.2); shot("radar_set"); printf("  facing %d (8 is the radar set)\n", game.testCabinFacing());
        press(KEY_E); run(0.3); shot("radar_on"); printf("  %s\n", game.testRadarInfo().c_str());   // E on the set switches the receiver on
        game.testCabinGoto(0.4, -1.6, 0.0, 0.0);
        in.down[KEY_RIGHT] = true; run(2.0); in.down[KEY_RIGHT] = false; run(0.3); shot("radar_sweep");
        int who = -1;
        for (int i = 0; i < game.testRadarSignals() && who < 0; i++) if (game.testRadarKind(i) == SIG_PEOPLE) who = i;
        if (who < 0 && game.testRadarSignals() > 0) who = 0;
        if (who >= 0 && game.testAimAtSignal(who)) {
            run(1.5); shot("radar_hold"); printf("  %s\n", game.testRadarInfo().c_str());
            run(5.5); shot("radar_lock"); printf("  %s\n", game.testRadarInfo().c_str());
            press(KEY_ENTER);
            run(1.0);
            shot("radar_flight");
            printf("  Enter: '%s'; %s; the synth: %s\n", game.testStatus().c_str(), game.testRadarInfo().c_str(),
                    game.audio.radio ? "STILL the radar's" : "released");   // the Vimana flight to the signal's star, the camera left
            press(KEY_V); run(0.3); printf("  the flight aborted for the rest of the flow: '%s' (%s)\n", game.testStatus().c_str(), game.testDebugInfo().c_str());
            // the gain up to x32 (the far scan a slice a frame), the view onto the farthest people's signal x32 hears, the longer
            // hold, the lock with its thousands of light years, Enter flies there; V aborts at once, so the flow goes on from where
            // C-07's flight left it
            press(KEY_B); run(0.1);
            for (int i = 0; i < 5; i++) press(KEY_EQUAL);
            int frames = 0; while (!game.testRadarDeepReady() && frames < 60) { run(1.0 / 30); frames++; }
            run(0.5); shot("radar_gain"); printf("  the gain at x32 after %d frames of the far scan: %s\n", frames, game.testRadarInfo().c_str());
            int far = -1; double farLy = 0;
            for (int i = 0; i < game.testRadarSignals(); i++) if (game.testRadarKind(i) == SIG_PEOPLE && game.testRadarDist(i) > farLy) { farLy = game.testRadarDist(i); far = i; }
            if (far >= 0 && game.testAimAtSignal(far)) {
                run(2.0); shot("radar_far_hold"); printf("  %s\n", game.testRadarInfo().c_str());
                double at = 2.0; while (game.testRadarLocked() < 0 && at < 16) { run(0.5); at += 0.5; }
                shot("radar_far_lock"); printf("  locked after %.1f s: %s\n", at, game.testRadarInfo().c_str());
                press(KEY_ENTER); run(0.1); shot("radar_far_flight"); printf("  Enter: '%s'\n", game.testStatus().c_str());
                press(KEY_V); run(0.3); printf("  the far flight aborted at once: '%s' (%s)\n", game.testStatus().c_str(), game.testDebugInfo().c_str());
            } else printf("  no far people's signal at x32\n");
        } else printf("  no signal within reach of the start\n");
    }
}

void Flow::starmap() {
    // M3: rename the star through the guide, look at the star map
    press(KEY_G); run(0.1); shot("guide"); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("text_entry");
    for (int i = 0; i < 20; i++) press(KEY_BACKSPACE);
    game.testTypeText("HOME OF CATS"); run(0.1);
    printf("  renamed star: '%s'\n", game.testStarName().c_str());
    press(KEY_M); run(0.3); shot("starmap");
    press(KEY_TAB); run(0.2); shot("starmap_galaxy"); printf("  the galaxy chart: page %d, level %d, built in %.1f ms\n", game.testMapPage(), 0, game.testChartBuildMs());   // the whole disc
    for (int i = 0; i < 4; i++) press(KEY_EQUAL);
    run(0.2); shot("starmap_galaxy_x16"); printf("  four steps in: built in %.1f ms, the ship's crosshair at %d %d\n", game.testChartBuildMs(), game.testChartYouX(), game.testChartYouY());
    for (int i = 0; i < 4; i++) press(KEY_MINUS);
    press(KEY_TAB); run(0.1); press(KEY_ESCAPE);
    press(KEY_R); press(KEY_N); run(0.2); shot("targeting");
    press(KEY_ENTER); run(0.2);
    press(KEY_V); run(3.0); shot("vimana"); printf("  the player in the flight: %s; its line '%s'\n", game.testPlayerInfo().c_str(), game.testPlayerHud().c_str());
    press(KEY_M); run(0.3); shot("starmap_flight"); printf("  M in the flight: state %d, page %d\n", (int)game.state, game.testMapPage()); press(KEY_ESCAPE);   // the galaxy, the line flown
    run(12.0); shot("arrived"); printf("  the player on arrival: %s\n", game.testPlayerInfo().c_str());
}

void Flow::landing() {
    int lb = game.testLandableBody(); bool parkedByTest = false;
    printf("landable body %d of %s (the generator calls this star %s)\n", lb, game.testBodyTypes().c_str(), game.testStarGenName().c_str());
    if (lb >= 0 && game.testBodyType(lb) == PT_COMET) {
        // O6-03: the nearest system holds nothing but comets since GEN 9; the walk, the buggy and the water below need a world
        // with gravity: park at the first felisian world of the scan instead (the approach frames come from the comet section)
        StarSystem fs; int fb = -1;
        if (firstBodyOfType(PT_FELISIAN, fs, fb)) {
            game.testParkAt(fs.star, fb);
            lb = fb;
            run(0.5);
            parkedByTest = true;
            printf("  only comets here: parked at %s body %d for the landing\n", fs.star.name.c_str(), fb);
        }
    }
    if (lb >= 0) {
        // approach unless the test parked the ship itself (the old check compared the star's generated name with
        // the scan's first felisian system, Skeatoltdos, which the comet case parks at; a pinned name breaks with the galaxy)
        if (!parkedByTest) {
            game.testAimAtBody(lb);
            press(KEY_TAB); run(0.1);
            for (int i = 0; i < lb; i++) press(KEY_DOWN);
            press(KEY_ENTER); run(2.0); shot("approach");
            run(9.0); shot("orbit"); printf("  %s\n", game.testDebugInfo().c_str());
            press(KEY_C); game.testWaitMap(); run(0.2);   // the map's frames below (the screen reads the world on the map's thread)
        }
        if (game.testBodyType(lb) != PT_FELISIAN) {
            // the landing below (the walk, the herd, the water) wants a living world: the arrival's world gave the approach
            // frames; park at the scan's first felisian world now (the pinned home's nearest star holds none)
            StarSystem fs; int fb = -1;
            if (firstBodyOfType(PT_FELISIAN, fs, fb)) {
                game.testParkAt(fs.star, fb);
                lb = fb;
                run(0.5);
                printf("  no living world at this star: parked at %s body %d for the landing\n", fs.star.name.c_str(), fb);
            }
        }
        shot("landing_map");
        press(KEY_R); run(0.1); shot("landing_map2");
        in.down[KEY_LEFT_CONTROL] = true;
        press(KEY_T);
        in.down[KEY_LEFT_CONTROL] = false;
        run(0.1);
        shot("almanac_site");
        printf("  %s\n", game.testAlmanacInfo().c_str());
        press(KEY_ESCAPE);
        run(0.1);   // the cursor's site
        press(KEY_Z); run(0.3); shot("landing_zoom"); press(KEY_Z); run(0.1);   // the sector zoom
        press(KEY_J); run(0.1); shot("landing_sky");
        game.testLandSite(); run(0.1);   // a lit land site from the body's seed (J leaves the cursor under the moon: in the sea at Heilya II; R is seeded by the clock)
        press(KEY_ENTER); run(2.0); shot("descent"); printf("  the player at the descent: %s\n", game.testPlayerInfo().c_str());   // a landing stops it
        game.currentSlot = 2; in.down[KEY_LEFT_CONTROL] = true; press(KEY_S); in.down[KEY_LEFT_CONTROL] = false;   // save mid-descent to slot 2
        game.currentSlot = 1;
        run(6.0); shot("surface");
        printf("  %s\n", game.testRangeInfo().c_str());   // the rangefinder
        printf("  fronts: %s\n", game.testFrontInfo().c_str());   // the forecast at the feet
        in.down[KEY_W] = true; run(3.0); in.down[KEY_W] = false;
        in.mouseDx = 400; game.frame(in, 1.0 / 30); in.newFrame();
        run(0.5); shot("surface2");
        press(KEY_TAB); run(0.2); shot("surface_readings"); press(KEY_TAB); run(0.1);   // the readings (Tab) over the view and away again
        // sprint, jump + jetpack, the buggy
        in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true; run(2.5); shot("sprint"); in.down[KEY_LEFT_SHIFT] = false; in.down[KEY_W] = false;
        printf("  sprint: stamina %.0f, speed %.1f m/s\n", game.testPlayerStamina(), game.testPlayerSpeed());
        press(KEY_SPACE); in.down[KEY_SPACE] = true; run(2.5); shot("jetpack"); in.down[KEY_SPACE] = false;
        printf("  jetpack: %.1f m above ground\n", game.testPlayerAlt());
        run(4.0);
        press(KEY_K); run(0.3);                      // capsule next to us
        press(KEY_B); run(2.5);                      // unfold
        game.testWalkToBuggy();
        press(KEY_E); run(0.2); shot("buggy_seat");
        // a long drive on open ground; frames at speed, airborne if it happens, the chase view
        {
            bool airShot = false; double top = 0;
            double openRun = game.testDriveOpen();
            if (openRun > 0) printf("  open run of %.0f m found for the drive\n", openRun);
            else printf("  no open run near this landing (forest): a short smoke drive; vesperis_test drive is the long one\n");
            in.down[KEY_W] = true;
            for (int i = 0; i < 30 * 45; i++) {
                game.frame(in, 1.0 / 30); in.newFrame();
                if (i == 30 * 8) { in.down[KEY_D] = true; }
                if (i == 30 * 9) { in.down[KEY_D] = false; }
                if (i == 30 * 20) shot("buggy_drive");
                if (i == 30 * 21 || i == 30 * 22) in.pressed[KEY_TAB] = true;   // the buggy's readings
                if (i == 30 * 21 + 6) shot("buggy_readings");
                top = std::max(top, game.testBuggySpeed());
                if (!airShot && game.testBuggyAirborne()) { shot("buggy_air"); airShot = true; }
            }
            printf("  drive: %s, top %.0f km/h%s\n", game.testBuggyInfo().c_str(), top * 3.6, airShot ? ", airborne once" : "");
        }
        press(KEY_V); run(0.2); shot("buggy_chase"); in.down[KEY_W] = false; run(1.5);
        printf("  buggy: %s\n", game.testBuggyInfo().c_str());
        press(KEY_E); run(0.5);
        {   // the capsule always comes; B at it unfolds a new buggy and scraps the old one
            press(KEY_K); run(0.3);
            bool recalled = game.testStatus().find("RECALLED") != std::string::npos;
            press(KEY_B); run(0.3);
            bool scrapped = game.testStatus().find("SCRAPPED") != std::string::npos;
            printf("  recall with the buggy out: %s; B redeploys: %s (%s)\n", recalled ? "ok" : "FAIL", scrapped ? "ok" : "FAIL", game.testBuggyInfo().c_str());
        }
        {   // the drone: F unfolds it at the capsule, E gets in, Space lifts it and climbs, a flight at full thrust with a turn,
            // the chase view, S brakes it, Shift brings it down and lands it, E out; the capsule is recalled beside the landing
            press(KEY_F); run(2.5);
            bool unfolded = game.testDroneInfo().find("deployed=1") != std::string::npos;
            game.testWalkToDrone(); press(KEY_E); run(0.2); shot("drone_seat");
            bool seated = game.testDroneInfo().find("in=1") != std::string::npos;
            in.down[KEY_SPACE] = true; run(6.0);   // lift off, 6 s of climb
            double top = 0, altMax = 0; bool lifted = !game.testDroneLanded();
            in.down[KEY_W] = true;
            for (int i = 0; i < 30 * 16; i++) {
                game.frame(in, 1.0 / 30); in.newFrame();
                if (i == 30 * 6) in.down[KEY_SPACE] = false;
                if (i == 30 * 9) in.down[KEY_D] = true;
                if (i == 30 * 11) in.down[KEY_D] = false;
                if (i == 30 * 12) shot("drone_flight");
                if (i == 30 * 13 || i == 30 * 14) in.pressed[KEY_TAB] = true;   // the drone's readings
                if (i == 30 * 13 + 6) shot("drone_readings");
                top = std::max(top, game.testDroneSpeed()); altMax = std::max(altMax, game.testDroneAlt());
            }
            press(KEY_V); run(0.2); shot("drone_chase"); press(KEY_V); run(0.1);
            in.down[KEY_W] = false; in.down[KEY_S] = true;
            int n = 0; while (game.testDroneSpeed() > 2 && n++ < 30 * 30) { game.frame(in, 1.0 / 30); in.newFrame(); }
            in.down[KEY_S] = false; in.down[KEY_LEFT_SHIFT] = true;
            int landT = 0; while (!game.testDroneLanded() && landT++ < 30 * 90) { game.frame(in, 1.0 / 30); in.newFrame(); }
            in.down[KEY_LEFT_SHIFT] = false; run(0.3);
            bool landed = game.testDroneLanded();
            press(KEY_E); run(0.3);
            bool out = game.testDroneInfo().find("in=0") != std::string::npos;
            printf("  drone: unfolded %s, in %s, lifted %s, top %.0f km/h, %.0f m up, landed %s after %.0f s, out %s; %s\n", unfolded ? "ok" : "FAIL", seated ? "ok" : "FAIL", lifted ? "ok" : "FAIL",
                    top * 3.6, altMax, landed ? "ok" : "FAIL", landT / 30.0, out ? "ok" : "FAIL", game.testDroneInfo().c_str());
            press(KEY_K); run(0.3);
        }
        press(KEY_M); run(0.2);
        press(KEY_N); run(0.2); shot("sectormap");
        for (int i = 0; i < 5; i++) {   // out to the widest level this world allows, a frame per level (B-402: the grid's step)
            press(KEY_MINUS); run(0.3);
            if (i < 4) shot((std::string("sectormap_z") + std::to_string(i + 1)).c_str());
        }
        shot("sectormap_wide"); printf("  %s\n", game.testMarksInfo().c_str());
        for (int i = 0; i < 5; i++) press(KEY_EQUAL);
        press(KEY_ESCAPE);
        {   // the scanner (R) through the world's modes to the first, its lines and the compass's mark; a mark at the crosshair (L), then
            // the sector map with the mark and the echoes; the mark removed again from the map (Delete twice) so the flow's guide stays as it was.
            // the mark is not over the view until the map's Shift+Enter shows it (boxed on the map), then over the view with its
            // diamond on the compass
            press(KEY_R);
            // the scan's thread waited for at one instant (frames at dt 0): frames of 1/30 s while it worked put a faster binary's
            // clock ahead (more frames in the scan's 50 ms), and every picture after this one with it
            for (int i = 0; i < 3000 && !game.testScanned(); i++) { game.frame(in, 0); in.newFrame(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
            run(0.2); shot("scan_hud"); printf("  scanner: %s\n", game.testScanInfo().c_str());
            const double pitch0 = game.testPitch();
            game.testSetPitch(-0.06); run(0.1);   // the crosshair on the ground ahead (level, it rests on the sky and L marks the feet)
            press(KEY_L); game.testTypeText("FLOW MARK"); run(0.3); shot("scan_marked");
            game.testSetPitch(pitch0);
            press(KEY_N); run(0.2);
            int fm = -1; for (int i = 0; i < (int)game.guide.marks.size(); i++) if (game.guide.marks[i].name == "FLOW MARK") fm = i;
            double mx = 0, mz = 0;
            if (fm >= 0) {
                game.testMarkLocal(fm, mx, mz);
                game.testMapCursor(mx, mz);
                in.down[KEY_LEFT_SHIFT] = true;
                press(KEY_ENTER);
                in.down[KEY_LEFT_SHIFT] = false;
                game.testMapCursor(mx + 1500, mz + 900);
            }   // the cursor off it: the shown mark's own box
            run(0.2); shot("scan_map");
            press(KEY_ESCAPE); run(0.3); shot("mark_shown");
            printf("  the mark shown over the view %s, %.0f m off\n", fm >= 0 && game.testShownMark() == fm ? "yes" : "NO", std::hypot(mx - game.testPlayerX(), mz - game.testPlayerZ()));
            press(KEY_N); run(0.1);
            if (fm >= 0) { game.testMapCursor(mx, mz); press(KEY_DELETE); press(KEY_DELETE); }
            printf("  the mark placed at the crosshair %s, removed from the map %s\n", fm >= 0 ? "yes" : "NO", fm >= 0 && (int)game.guide.marks.size() == fm ? "yes" : "no");
            press(KEY_ESCAPE);
            while (game.testScanMode() >= 0) press(KEY_R);   // the scanner off again
        }
        press(KEY_X); run(0.2); shot("creatures");
        // walk up to the first herd and log how it reacts
        if (game.testGotoHerd(34)) {
            run(0.5); shot("herd_approach"); printf("  herd before: %s\n", game.testHerdStates().c_str());
            in.down[KEY_W] = true; run(4.0); in.down[KEY_W] = false; run(0.5);
            press(KEY_X); run(0.1); shot("herd_flee"); printf("  herd after walking in: %s\n", game.testHerdStates().c_str());
        } else printf("  no herd at this site\n");
        // photo tools: photo mode with the free camera, a panorama, three recorded frames
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_P); in.down[KEY_LEFT_CONTROL] = false; run(0.1); shot("photo");
        in.down[KEY_W] = true; in.down[KEY_SPACE] = true; run(0.6); in.down[KEY_W] = false; in.down[KEY_SPACE] = false; shot("photo_flight");
        {
            std::vector<uint32_t> pano; int pw, ph;
            game.renderPanorama(pano, pw, ph);
            writePNG("shots/tests/flow_panorama.png", pano.data(), pw, ph);
            printf("panorama %dx%d -> shots/flow_panorama.png\n", pw, ph);
        }
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_P); in.down[KEY_LEFT_CONTROL] = false; run(0.1);
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_R); in.down[KEY_LEFT_CONTROL] = false; run(0.1);
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_R); in.down[KEY_LEFT_CONTROL] = false;
        printf("recorder: take %d, %d frames%s\n", game.recTake, game.recFrame, game.recFrame >= 3 ? " (ok)" : " FAIL");
        {   // the almanac under the feet, Enter on its first event, the run watched until the clock lands on it (the time-lapse frame of M5-06 was here)
            in.down[KEY_LEFT_CONTROL] = true; press(KEY_T); in.down[KEY_LEFT_CONTROL] = false; run(0.1); shot("almanac"); printf("  %s\n", game.testAlmanacInfo().c_str());
            press(KEY_ENTER); run(0.3); shot("almanac_run");
            double target = game.testAlmanacTarget(), tStart = game.gameTime(), peak = 0; int frames = 0;
            while (game.testAlmanacRunning() && frames++ < 30 * 300) { game.frame(in, 1.0 / 30); in.newFrame(); peak = std::max(peak, game.testTimeWarp()); }
            double off = game.gameTime() - target, warpAfter = game.testTimeWarp();
            run(0.2); shot("almanac_arrived");
            printf("  the run: %.0f s of game time in %d frames at a peak of x%.0f, %.3f s off the event, warp x%.0f, '%s'\n", target - tStart, frames, peak, off, warpAfter,
                    game.testStatus().c_str());
        }
        press(KEY_V); press(KEY_V); run(0.1); shot("supervision"); press(KEY_V); press(KEY_V); press(KEY_V);
        // a note, the log, the statistics and the gallery
        press(KEY_G); press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER); game.testTypeText("STRANGE RED TREES HERE"); run(0.1);
        press(KEY_J); run(0.1); shot("log"); press(KEY_ESCAPE);
        press(KEY_G); for (int i = 0; i < 6; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("stats"); press(KEY_ESCAPE);
        mkdir("shots/tests/test_gallery", 0755);
        writePNG("shots/tests/test_gallery/screenshot_0.png", game.output(), FBW, FBH);
        { FILE* sc = fopen("shots/tests/test_gallery/screenshot_0.txt", "w"); if (sc) { fprintf(sc, "%s\n", game.screenshotCaption().c_str()); fclose(sc); } }
        press(KEY_G); for (int i = 0; i < 5; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("gallery"); press(KEY_ESCAPE);
        printf("  log entries: %d\n", game.testLogEntries());
        for (int i = 0; i < 3; i++) press(KEY_T);
        run(4.0); shot("surface3");
        press(KEY_I); run(0.1); shot("surface_data"); printf("  fronts: %s\n", game.testFrontInfo().c_str()); press(KEY_SPACE);   // the data sheet's FORECAST line
        press(KEY_ESCAPE); run(0.1); shot("menu");
        press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("auto"); press(KEY_ESCAPE); run(0.1);   // auto-explore's chooser, Esc back to the menu (on its row)
        press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("settings"); press(KEY_ESCAPE); press(KEY_ESCAPE);
        press(KEY_ESCAPE); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("slots"); press(KEY_ENTER); run(0.1);   // menu > save > slot 1 (R-419: AUTO-EXPLORE the second row)
        press(KEY_K); run(0.5);
        press(KEY_Q); run(3.0); shot("ascent");
        run(6.0); shot("back_in_space");
        {   // a second landing (same world, another site) starts with no buggy
            printf("  before the second landing: %s\n", game.testMarksInfo().c_str());
            press(KEY_C); run(0.3); press(KEY_ENTER); run(8.0); shot("second_landing");
            bool none = game.testBuggyInfo().find("deployed=0") != std::string::npos;
            printf("  second landing: %s -> %s\n", game.testBuggyInfo().c_str(), none ? "no buggy (ok)" : "FAIL: the buggy carried over");
            std::string marks = game.testMarksInfo();   // the waypoint of the first landing must be gone (R-414: no trail any more)
            bool fresh = marks.find("waypoint=0") != std::string::npos;
            press(KEY_N); run(0.2); shot("second_sectormap"); press(KEY_ESCAPE);
            printf("  marks after the second landing: %s -> %s\n", marks.c_str(), fresh ? "fresh (ok)" : "FAIL: the old landing's marks carried over");
            press(KEY_K); run(0.3); press(KEY_Q); run(9.0);   // back to space for the rest of the flow
        }
    }
}

void Flow::comet() {
    // N0-01 (B-201): park at the first comet near the start; the HUD shows its speed and plunge, the data sheet its distance now
    {
        bool found = false;
        for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return !found; })) {
            for (int bi = 0; bi < (int)sys.bodies.size() && !found; bi++) {
                if (sys.bodies[bi].type != PT_COMET) continue;
                game.testParkAt(s, bi); run(0.5); shot("comet_map");   // comets are landable, so parking opens the landing map
                press(KEY_ESCAPE); run(0.3); shot("comet");
                printf("  comet HUD: %s\n", game.cometMotionString(bi).c_str());
                press(KEY_I); run(0.1); shot("comet_data"); press(KEY_SPACE);
                press(KEY_TAB); run(0.1); shot("comet_list"); press(KEY_ESCAPE);
                // land on the nucleus: microgravity, no buggy, floating jumps that Ctrl brings down, the tail overhead
                press(KEY_C); run(0.2); press(KEY_ENTER); run(8.0); shot("comet_surface");
                printf("  on the comet: escape velocity %.1f m/s, %s, '%s'\n", game.testEscapeVelocity(), game.testRangeInfo().c_str(), game.testStatus().c_str());
                press(KEY_B); run(0.2); printf("  B on the comet: '%s'\n", game.testStatus().c_str());
                in.down[KEY_W] = true; run(3.0); printf("  walking at %.2f m/s (the cap is %.2f)\n", game.testPlayerSpeed(), 0.3 * game.testEscapeVelocity()); in.down[KEY_W] = false;
                press(KEY_SPACE); run(4.0); shot("comet_jump"); double up = game.testPlayerAlt();
                in.down[KEY_LEFT_CONTROL] = true; run(3.0); in.down[KEY_LEFT_CONTROL] = false;
                printf("  a jump: %.2f m up after 4 s, %.2f m after 3 s of Ctrl\n", up, game.testPlayerAlt());
                in.mouseDy = -700; game.frame(in, 1.0 / 30); in.newFrame(); run(0.2); shot("comet_sky");
                in.mouseDy = 700; game.frame(in, 1.0 / 30); in.newFrame();
                press(KEY_N); run(0.2); shot("comet_sectormap"); press(KEY_ESCAPE);
                press(KEY_K); run(0.3); press(KEY_Q); run(9.0);
                found = true;
            }
        }
    }
}

void Flow::belt() {
    // O3 (R-302): fly to the first belt near the start, park beside one of its rocks; the capsule refuses; the sheet
    {
        bool found = false;
        for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return !found; })) {
            if (sys.belts.empty()) continue;
            game.testParkAt(s, -1); run(0.3);
            press(KEY_TAB); run(0.1);
            for (int i = 0; i < (int)sys.bodies.size(); i++) press(KEY_DOWN);
            shot("belt_list");
            press(KEY_ENTER); run(3.0); shot("belt_approach");
            run(12.0); shot("belt");
            printf("  belt: %s; parked belt %d; '%s'\n", game.testDebugInfo().c_str(), game.testParkedBelt(), game.testStatus().c_str());
            press(KEY_I); run(0.1); shot("belt_data"); press(KEY_SPACE);
            press(KEY_C); run(0.2); printf("  C in the belt: '%s'\n", game.testStatus().c_str());
            press(KEY_X); run(0.3); shot("belt_rock");
            printf("  %s\n", game.testBeltDraw().c_str());
            found = true;
        }
        if (!found) printf("belt: none found\n");
    }
}

void Flow::chart() {
    {   // G-06: the next jump chosen on the chart. From where the flow left the ship, the cursor 3,000 ly coreward in the plane: Enter plots,
        // Tab looks at the stars round its end, Tab back, Enter flies; at the arrival the chart again, the ship's crosshair at its new place
        press(KEY_M); if (game.testMapPage() != 1) press(KEY_TAB);
        game.testChartLevel(3); run(0.05);
        const Vec3 h = game.testShipPos() / SECTOR_KM;
        const Vec3 toCore = normalize(Vec3(GALAXY_CENTRE_SX - h.x, 0, GALAXY_CENTRE_SZ - h.z)), aim = Vec3(h.x, 0, h.z) + toCore * 3000;
        game.testChartCursor(aim); run(0.1); shot("starmap_cursor");
        press(KEY_ENTER); run(0.1); shot("starmap_plotted");
        int64_t rx = 0, ry = 0, rz = 0; game.testRemote(rx, ry, rz);
        printf("  the chart's cursor 3,000 ly coreward: plotted %d, the star at %lld %lld %lld\n", game.testChartPlotted(), (long long)rx, (long long)ry, (long long)rz);
        press(KEY_TAB); run(0.1); shot("starmap_look"); printf("  Tab: the look %d, %zu stars round the plotted end\n", game.testChartLook(), game.testLookStars().stars.size());
        press(KEY_TAB); run(0.05);
        press(KEY_ENTER); const double dur = game.testFlightDur(); run(1.0); shot("starmap_jumped"); run(dur);
        press(KEY_M); run(0.1); shot("starmap_arrived");
        const Vec3 at = game.testShipPos() / SECTOR_KM;
        printf("  flown in %.1f s: arrived %d, %.1f ly from the point; the chart's crosshair at %d %d\n", dur, game.testSystem().valid, length(at - aim), game.testChartYouX(), game.testChartYouY());
        press(KEY_ESCAPE); run(0.05);
    }
}

void Flow::probe() {
    {   // X-01: the probe. Park at the first gas giant of the scan, the telescope's reticle on its disc; C sends the probe to the ground
        // under it and the screen is its camera: the fall, the entry, the clouds, the chute; saved in the descent to slot 3 (the newest
        // save: the load below resumes it); Esc to the cabin with it falling (the space HUD's line), C back to it, Shift held a second
        // (R-406), a flash between the decks (X-02); on to the end, the last frame held under the hiss, Enter gives control back
        Star gs; int gi = -1;
        for (int64_t x = 150; x < 200 && gi < 0; x++) for (int64_t z = 20; z < 80 && gi < 0; z++) {
            Star s; StarSystem gsys; if (!systemAt(x, 0, z, s, gsys, true)) continue;
            for (int i = 0; i < (int)gsys.bodies.size(); i++) if (gsys.bodies[i].type == PT_GASGIANT) { gs = s; gi = i; break; }
        }
        if (gi >= 0) {
            game.testParkAt(gs, gi); game.testWaitMapOf(gi); run(0.3);   // the giant's map first (made at a binary's own pace, the shot had it or not)
            game.testAimAtBody(gi); press(KEY_Z); in.down[KEY_UP] = true; run(0.25); in.down[KEY_UP] = false; run(0.2); shot("probe_aim"); printf("  %s\n", game.testTelescopeInfo().c_str());
            press(KEY_C); run(2.0); shot("probe_fall"); printf("  C: '%s'; %s\n", game.testStatus().c_str(), game.testProbeInfo().c_str());
            game.testProbeSkip(PROBE_FALL_S + 5); run(0.1); shot("probe_entry");
            game.testProbeSkip(probeDescentStart() + 60); run(0.1); shot("probe_clouds"); printf("  %s\n", game.testProbeInfo().c_str());
            game.testProbeSkip(probeDescentStart() + 115); press(KEY_SPACE); run(0.5); shot("probe_chute"); printf("  Space: '%s'; %s\n", game.testStatus().c_str(), game.testProbeInfo().c_str());
            game.currentSlot = 3; in.down[KEY_LEFT_CONTROL] = true; press(KEY_S); in.down[KEY_LEFT_CONTROL] = false; game.currentSlot = 1;   // saved in the descent
            press(KEY_ESCAPE); run(0.3); shot("probe_cabin"); printf("  Esc: state %d, '%s'\n", (int)game.state, game.testStatus().c_str());
            press(KEY_C); run(0.2); printf("  C again: state %d (24 the probe's screen)\n", (int)game.state);
            {   // Shift held a second runs the relay's clock eight seconds; X-02: the clear band between the decks, a flash in its floor
                double c0 = game.testProbeClock(); in.down[KEY_LEFT_SHIFT] = true; run(1.0); shot("probe_fast"); in.down[KEY_LEFT_SHIFT] = false;
                printf("  Shift held a second (R-406): the clock %.1f -> %.1f s\n", c0, game.testProbeClock());
                double cb = -1;
                for (double c = game.testProbeClock(); c < game.testProbeEndClock() && cb < 0; c += 1.0 / 30) if (game.testProbeStageAt(c) == PS_BETWEEN && game.testProbeFlash(c) > 0.3) cb = c;
                if (cb > 0) { game.testProbeSkip(cb - 1.0 / 30); run(1.0 / 30); shot("probe_between"); printf("  %s\n  %s\n", game.testProbeInfo().c_str(), game.testProbeViewInfo().c_str()); }
            }
            game.testProbeSkip(1e6); run(0.3); shot("probe_lost"); printf("  the end: %s; log: %s\n", game.testProbeInfo().c_str(), game.guide.log.empty() ? "-" : game.guide.log.back().text.c_str());
            press(KEY_ENTER); run(0.2); printf("  Enter: state %d, probe %s, '%s'\n", (int)game.state, game.testProbeActive() ? "STILL ACTIVE" : "let go", game.testStatus().c_str());
            // what came back. The gallery's newest item is its recording (its final image); Enter plays it on the screen, the
            // arrows go down it by depth; the statistics count it; the giant's data sheet draws its profile
            press(KEY_G); for (int i = 0; i < 5; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.2); shot("probe_gallery");   // the menu lost its landmark row
            printf("  gallery (X-04): %d items, the newest %s; '%s'\n", game.testGalleryItems(), game.testGalleryRec(game.testGallerySel()) >= 0 ? "the probe's recording" : "A PHOTOGRAPH",
                    game.guide.probes.empty() ? "no record" : game.guide.probes.back().body.c_str());
            press(KEY_ENTER);
            run(2.0);
            shot("probe_recording");
            printf("  Enter: state %d (25 the recording), clock %.1f s, '%s'\n", (int)game.state, game.testRecordingClock(), game.testStatus().c_str());
            in.down[KEY_RIGHT] = true; run(1.4); in.down[KEY_RIGHT] = false; run(0.1); shot("probe_recording_deep"); printf("  Right held: clock %.1f s\n", game.testRecordingClock());
            press(KEY_ESCAPE); run(0.1); press(KEY_ESCAPE); run(0.1);
            press(KEY_G); for (int i = 0; i < 6; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("probe_stats");
            printf("  statistics (X-04): %d probes sent, the deepest %.0f km\n", game.guide.probesSent, game.guide.deepestProbeKm());
            press(KEY_ESCAPE); run(0.1);
            press(KEY_I); run(0.1); press(KEY_RIGHT); run(0.1); shot("probe_profile"); press(KEY_ESCAPE); run(0.1);
        } else printf("probe: no gas giant in the scan\n");
    }
}

void Flow::roads() {
    {   // C-09: the old roads on the landing zoom of Aieliaalas II (T-01: round a town of its people's plan, the cores and the plan's ways with them):
        // the network's faint lines over the window (R-415: with the peoples, V)
        Star as; if (starInSector(151, 0, 25, as, true)) {
            game.testParkAt(as, 1); run(0.3);
            StarSystem asys; asys.generate(as);
            double la = 12.0 * DEG, lo = 4.22 * DEG; planPlacePoint(BodyGen::make(asys.bodies[1]), 0, la, lo);
            game.testLandCursor(la / DEG, lo / DEG); run(0.2);
            press(KEY_Z);
            press(KEY_V);
            run(0.5);
            shot("roads_zoom");
            printf("  %s; the plan's cores %d, its roads %d; %s\n", game.testRoadInfo().c_str(), game.testZoomCores(), game.testZoomWays(NK_ROAD), game.testLandPeoplesInfo().c_str());
            press(KEY_V); press(KEY_Z); run(0.1); press(KEY_ESCAPE); run(0.1);
            // the drone bay over the same world: the rack, its screen, the drones sent (out of their ports), down there searching, back
            game.setState(GameState::SPACE); game.testCabinGoto(-0.3, 1.435, -PI / 2, -0.38); run(0.2);
            shot("drones_rack"); printf("  facing %d (11 is the drone bay)\n", game.testCabinFacing());
            press(KEY_E); run(0.1); game.testDronesSettle(); run(0.1); shot("drones_bay");
            press(KEY_ENTER); game.testDronesSettle(); run(1.0); shot("drones_launch");
            const std::string di = game.testDronesInfo();
            const size_t dp = di.find(" down "); const double down = dp != std::string::npos ? atof(di.c_str() + dp + 6) : 20;
            game.testDronesSkip(down + 4); game.testOpenDroneBay(); run(0.05); shot("drones_search");
            game.testDronesSkip(DRONE_SEARCH_S + 60); run(0.05); shot("drones_back");
            printf("  the drones: %d finds aboard (%d texts, %d music, %d charts, %d instruments); '%s'\n", game.testDronesAboard(), game.testDronesFinds(DF_TEXT), game.testDronesFinds(DF_MUSIC),
                    game.testDronesFinds(DF_CHART), game.testDronesFinds(DF_INSTRUMENT), game.testStatus().c_str());
            press(KEY_ESCAPE); run(0.1);
        }
    }
}

void Flow::reload() {
    TestGame g2; g2.savePrefix = "shots/tests/test_save"; g2.settingsPath = "shots/tests/test_settings.txt"; g2.guidePath = "shots/tests/test_guide.txt";
    g2.guide.load(g2.guidePath);
    printf("load: %d (newest -> slot %d, state %d)\n", (int)g2.loadNewest(), g2.currentSlot, (int)g2.state);
    printf("  guide reloaded: %zu names, %d log entries, %zu shards and %zu decoded, star '%s'\n", g2.guide.names.size(), g2.testLogEntries(), g2.guide.shards.size(), g2.guide.decoded.size(),
            g2.testStarName().c_str());   // C-06: the decoded lines round-trip
    for (int i = 0; i < 30; i++) { g2.frame(in, 1.0 / 30); in.newFrame(); }
    writePNG("shots/tests/flow_loaded.png", g2.output(), FBW, FBH);
    printf("  the newest save is the probe's (X-01): state %d, %s\n", (int)g2.state, g2.testProbeInfo().c_str());
    TestGame g3; g3.savePrefix = "shots/tests/test_save"; g3.settingsPath = "shots/tests/test_settings.txt"; g3.guidePath = "shots/tests/test_guide.txt";
    bool ok3 = g3.loadSlot(2);
    int st0 = (int)g3.state;
    for (int i = 0; i < 30 * 8; i++) { g3.frame(in, 1.0 / 30); in.newFrame(); }
    printf("load slot 2 (mid-descent): %d state %d -> after 8 s state %d; autosave: '%s'\n", (int)ok3, st0, (int)g3.state, g3.saveSummary(g3.slotPath(0)).c_str());
    printf("flow done in %.1f s\n", nowSec() - t0);
}

void runGameFlow() {
    Flow f;
    f.title();
    f.decoder();
    f.cabin();
    f.telescope();
    f.radar();
    f.starmap();
    f.landing();
    f.comet();
    f.belt();
    f.chart();
    f.probe();
    f.roads();
    f.reload();
}

// Settings file round trip and the settings screen (M0-02).
void testSettings() {
    Settings s;
    s.mouseSensitivity = 1.7; s.invertY = true; s.fovDeg = 85; s.masterVolume = 0.35; s.scanlines = false; s.windowScale = 3;
    s.save("shots/tests/test_settings.txt");
    Settings r;
    bool ok = r.load("shots/tests/test_settings.txt");
    bool same = ok && std::fabs(r.mouseSensitivity - 1.7) < 1e-9 && r.invertY && r.fovDeg == 85 && std::fabs(r.masterVolume - 0.35) < 1e-9 && !r.scanlines && r.windowScale == 3;
    printf("settings round trip: %s\n", same ? "ok" : "MISMATCH");
    TestGame game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";   // every harness Game keeps off the real guide.txt (2026-09-27)
    game.loadFromDisk();
    printf("loaded into game: fov=%.0f sens=%.1f scale=%d\n", game.settings.fovDeg, game.settings.mouseSensitivity, game.settings.windowScale);
    Input in;
    auto press = [&](int key) { pressKey(game, in, key); };
    press(KEY_ENTER); press(KEY_ESCAPE);
    const bool found = selectRow(game, in, "SETTINGS");   // U-01 (KI-426): by its label (three rows down was the load page after R-419)
    press(KEY_ENTER);
    const bool fov = selectRow(game, in, "FIELD OF VIEW");
    press(KEY_RIGHT);   // field of view +5
    if (!found || !fov) printf("settings: the rows not found (SETTINGS %d, FIELD OF VIEW %d)\n", (int)found, (int)fov);
    writePNG("shots/tests/settings.png", game.output(), FBW, FBH);
    press(KEY_ESCAPE);
    Settings r2; r2.load("shots/tests/test_settings.txt");
    printf("after +5 on the screen: fov=%.0f (file %.0f) state=%d\n", game.settings.fovDeg, r2.fovDeg, (int)game.state);
    Settings d;
    d.save("shots/tests/test_settings.txt");   // leave defaults for the other tests
}

// M4-09: render a few seconds of every sound family into a WAV file and check for NaN and clipping.
void testAudio() {
    AudioSynth synth;
    const int sr = 22050;
    std::vector<int16_t> pcm;
    int nan = 0, clip = 0;
    struct Stage { const char* name; double hum, engine, wind, tone, rain, lava, surf, birds, breath; int step; double leaves; };
    const Stage stages[] = {{"ship", 0.6, 0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0}, {"vimana", 0.6, 0.9, 0, 0.5, 0, 0, 0, 0, 0, 0, 0}, {"shore", 0.05, 0, 0.4, 0.25, 0, 0, 0.6, 0.6, 0, 1, 0},
                            {"desert storm", 0.05, 0, 1.0, 0.6, 0, 0, 0, 0, 0.4, 2, 0}, {"mountain rain", 0.05, 0, 0.6, 1.0, 0.8, 0, 0, 0, 0, 3, 0}, {"lava", 0.05, 0, 0, 0.5, 0, 0.6, 0, 0, 0, 0, 0},
                            {"forest", 0.05, 0, 0.35, 0.5, 0, 0, 0, 0.4, 0, 1, 0.8},
                            {"buggy", 0.05, 0.6, 0.5, 0.5, 0, 0, 0, 0, 0, 0, 0}};   // (engine, rpm and skid set below)
    std::vector<float> buf(2048);
    for (const Stage& sg : stages) {
        AudioState st;
        st.hum = sg.hum;
        st.engine = sg.engine;
        st.wind = sg.wind;
        st.windTone = sg.tone;
        st.rain = sg.rain;
        st.lava = sg.lava;
        st.surf = sg.surf;
        st.birds = sg.birds;
        st.breath = sg.breath;
        st.leaves = sg.leaves;
        if (std::string(sg.name) == "buggy") { st.engineRpm = 0.7; st.skid = 0.4; st.thud = 0.8; }
        int frames = 0;
        while (frames < sr * 3) {
            if ((frames / 2048) % 6 == 2 && sg.step) st.step = sg.step;
            if (frames == 0 && sg.engine == 0 && sg.lava == 0) st.beep = BEEP_LOCK;
            synth.render(buf.data(), (int)buf.size(), sr, st);
            for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32000)); }
            frames += (int)buf.size();
        }
        printf("audio stage %-14s ok\n", sg.name);
    }
    FILE* f = fopen("shots/tests/audio_test.wav", "wb");
    if (f) {
        uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
        fwrite("RIFF", 1, 4, f);
        fwrite(&riffLen, 4, 1, f);
        fwrite("WAVEfmt ", 1, 8, f);
        fwrite(&fmtLen, 4, 1, f);
        fwrite(&fmtTag, 2, 1, f);
        fwrite(&ch, 2, 1, f);
        fwrite(&rate, 4, 1, f);
        fwrite(&byteRate, 4, 1, f);
        fwrite(&blockAlign, 2, 1, f);
        fwrite(&bits, 2, 1, f);
        fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
    }
    printf("audio: %zu samples, %d non-finite, %d clipped -> shots/audio_test.wav\n", pcm.size(), nan, clip);
}

// C-09: `zoom <sx> <sz> <body index> <latDeg> <lonDeg> [scale]`: the landing map's zoom window over a point of a world at a scale (default
// 4), with the steps timed (the park, the cursor, Z, half a second of frames, Z and Esc): `shots/tests/zoom_<name>.png`; the roads drawn
// and their cost printed. `zoom 151 25 1 12 4.22` is Aieliaalas II's land region, `zoom 153 70 2 11.46 40.11` Leileashphail III's
int runZoom(int argc, char** argv) {
    if (argc < 7) { printf("zoom <sx> <sz> <body index> <latDeg|site|capital> <lonDeg> [scale]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    const std::string at = argv[5];   // `site` keeps the landing map's own cursor, `capital` the first people's capital's core
    double latDeg = atof(argv[5]), lonDeg = atof(argv[6]); int scale = argc > 7 ? atoi(argv[7]) : 4;
    Star s; if (!starInSector(sx, 0, sz, s, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    TestGame game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.guidePath = "shots/tests/test_guide.txt";
    game.newGame(); game.setState(GameState::SPACE);
    game.settings.renderScale = scale; game.applySettings();
    Input in;
    double t0 = nowSec();
    auto run = [&](double secs) { runFrames(game, in, secs); };
    auto press = [&](int key) { pressKey(game, in, key); };
    auto tick = [&](const char* what) { printf("  %-28s %7.2f s\n", what, nowSec() - t0); };
    game.testParkAt(s, bi); run(0.3); tick("parked, the landing map");
    if (at == "capital") {
        StarSystem zs; zs.generate(s);
        const BodyGen zg = BodyGen::make(zs.bodies[bi]);
        for (double tw0 = nowSec(); nowSec() - tw0 < 5.0 && !worldPlanBuilt(zg.seed);) run(0.1);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(zg);
        if (P->peoples > 0 && !P->plans[0].places.empty()) { latDeg = P->plans[0].places[0].lat / DEG; lonDeg = P->plans[0].places[0].lon / DEG; }
    }
    if (at != "site") game.testLandCursor(latDeg, lonDeg);
    run(0.2); tick("the cursor set");
    press(KEY_Z); tick("Z: the zoom built");
    { const double* z = game.testZoomBuildMs(); printf("  the build (F-01): samples %.1f, colours %.1f, roads %.1f, plan ways %.1f, places %.1f ms\n", z[0], z[1], z[2], z[3], z[4]); }
    run(0.5); tick("half a second of frames");
    std::string nm = game.testStarName(); for (char& ch : nm) if (ch == ' ') ch = '_';
    std::string fn = fmt("shots/tests/zoom_%lld_%lld_%d.png", (long long)sx, (long long)sz, bi);
    writePNG(fn.c_str(), game.output(), FBW, FBH);
    printf("  %s\n  %s -> %s\n", game.testRoadInfo().c_str(), game.testStatus().c_str(), fn.c_str());
    press(KEY_Z); press(KEY_ESCAPE); run(0.1); tick("Z off, Esc to space");
    return 0;
}

// F-01: `load landing|map|arrival|drive|capital|probe|roads [seconds] [scale] [age]`: B-422's measure in the harness: the frames paced at 60 a second as the
// window paces them, and every second the cores the process kept busy (its CPU seconds over the wall's), its threads alive and running
// (the mean of six samples), the frames' mean and worst. `landing`: the descent from the landing map (its drainage waited for up to 3 s)
// and the first minute on the ground; `map`: the landing map with the cursor tapped every 0.15 s; `arrival`: a star never seen, its maps
// generating; `drive`: full throttle from 400 m outside the modern test world's capital (the roads' rebuilds and their worst ms);
// `capital`: standing 400 m outside an age test world's capital (the industrial's by default, felisian), facing it, the flora's listing
// timed (a settlement's buildings tested against every tree); `probe`: the launch from the ringed test giant (the handover, KI-361);
// `roads`: on foot near the modern capital, moved two ruin cells at a time: each rebuild of the site's roads on the main thread timed
// a Wolf-Rayet star's gas giant and the envelope its wind strips (S-05): parked at the giant, carried round it by Shift and the
// arrows as a player is, a frame every 45 degrees; then from 12 and 40 radii, from a moon, through the cabin's window and at 1x.
// `tail [sx sy sz body]` (the user's giant by default)
// the parked ship's views near home of the scenes that were crowded (a solid world with a big moon, a twin, a giant with
// four moons or more, a giant's moon): parked as a player is, then from the same place the eyes on the biggest other disc
int runParkings() {
    struct Pick { int64_t x, z; int body; const char* what; };
    std::vector<Pick> picks;
    int nBig = 0, nGiant = 0, nTwin = 0, nGm = 0;
    for (auto& [x, z, s, sys] : scanSystems(185, 215, 36, 66, [&] { return picks.size() < 12; })) {
        for (const Body& b : sys.bodies) {
            if (b.type == PT_COMPANION || b.type == PT_COMET || b.parent >= 0) continue;
            const bool giant = b.type == PT_GASGIANT || b.type == PT_SUBSTELLAR;
            int moons = 0; double bigM = 0; bool twin = false;
            for (const Body& m : sys.bodies) if (m.parent == b.index) { moons++; bigM = std::max(bigM, m.radiusKm / b.radiusKm); twin |= m.doublePlanet; }
            if (twin && nTwin < 2) { picks.push_back({x, z, b.index, "twin"}); nTwin++; break; }
            if (!giant && bigM > 0.3 && nBig < 4) { picks.push_back({x, z, b.index, "big moon"}); nBig++; break; }
            if (giant && moons >= 4 && nGiant < 3) { picks.push_back({x, z, b.index, "giant"}); nGiant++; break; }
            if (giant && moons >= 2 && nGm < 3) { for (const Body& m : sys.bodies) if (m.parent == b.index) { picks.push_back({x, z, m.index, "giant's moon"}); nGm++; break; } break; }
        }
    }
    TestGame game;
    game.savePrefix = "shots/tests/parkings_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.guidePath = "shots/tests/parkings_guide.txt";
    remove(game.guidePath.c_str());
    game.newGame(); game.settings.cabin = false; game.applySettings();
    Input in;
    auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
    int k = 0;
    for (const Pick& pk : picks) {
        Star s; starInSector(pk.x, 0, pk.z, s);
        game.testParkAt(s, pk.body, false);
        run(0.2);
        // the landing map closed
        if (KINDS[game.testSystem().bodies[pk.body].type].landable) { in.pressed[KEY_ESCAPE] = true; in.down[KEY_ESCAPE] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[KEY_ESCAPE] = false; }
        run(0.5);
        const StarSystem& sys = game.testSystem(); const Body& b = sys.bodies[pk.body];
        std::string ms;
        for (const Body& m : sys.bodies)
            if (m.parent == (b.parent >= 0 ? b.parent : b.index) && m.type != PT_COMPANION)
                ms += fmt(" %.1fR/%.2f", m.orbitRadiusKm / sys.bodies[m.parent].radiusKm, m.radiusKm / sys.bodies[m.parent].radiusKm);
        writePNG(fmt("shots/tests/parkings_%02d_a.png", k).c_str(), game.output(), FBW, FBH);
        // from the same place, the eyes on the biggest other disc in the sky (the look round a parked player gives)
        const double t = game.testGameTime(); const Vec3 P = game.testShipPos();
        int nb = -1; double nbA = 0;
        for (int j = 0; j < (int)sys.bodies.size(); j++) {
            if (j == pk.body || sys.bodies[j].type == PT_COMPANION) continue;
            const double a = std::asin(clampd(sys.bodies[j].radiusKm / length(sys.bodyPos(j, t) - P), 0, 1)) / DEG;
            if (a > nbA) { nbA = a; nb = j; }
        }
        const double sep = nb >= 0 ? std::acos(clampd(dot(normalize(sys.bodyPos(nb, t) - P), normalize(sys.bodyPos(pk.body, t) - P)), -1, 1)) / DEG : 0;
        game.testPlaceShip(P); game.testAimAtBody(nb); run(0.3);
        writePNG(fmt("shots/tests/parkings_%02d_b.png", k).c_str(), game.output(), FBW, FBH);
        printf("parkings %02d: %s, %lld 0 %lld body %d (%s, R %.0f km); its family's moons (orbit / size in the planet's radii):%s\n", k, pk.what, (long long)pk.x, (long long)pk.z, pk.body,
                KINDS[b.type].name, b.radiusKm, ms.c_str());
        if (nb >= 0)
            printf("  the biggest other disc: body %d (%s), %.0f km off, %.2f degrees wide, %.0f degrees from the world's centre -> shots/tests/parkings_%02d_a.png, _b.png\n", nb,
                    KINDS[sys.bodies[nb].type].name, length(sys.bodyPos(nb, t) - P), 2 * nbA, sep, k);
        k++;
    }
    return 0;
}

// R-420: `tape [dir]`: auto-explore's tape. The tracks as read from `music/` (or dir), with their credits and gains; the deck's four sequences
// for three seeds into shots/tests/tape_deck_<kind>_<seed>.wav (22,050 Hz mono, as the synth plays them); then auto-explore from home with
// that tape (the platform's side played by the harness, each track 12 s): the caption fading in, whole, fading out and gone, the next
// track's (shots/tests/tape_NN.png), the chooser with the tape's line and the words for an empty folder
int runTape(int argc, char** argv) {
    const std::string dir = argc > 2 ? argv[2] : "music";
    TestGame game;
    game.savePrefix = "shots/tests/tape_save"; game.settingsPath = "shots/tests/tape_settings.txt"; game.guidePath = "shots/tests/tape_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
    remove(game.guidePath.c_str());
    applyTestScale(game);
    game.settings.autoMusic = false; game.settings.autoTrack = 0; game.settings.autoTrackAt = -1;
    game.musicDir = dir;
    const size_t n = game.testTapeTracks();
    printf("tape: %zu tracks in %s/\n", n, dir.c_str());
    for (size_t i = 0; i < n; i++) printf("  %zu. %s\n", i + 1, game.testTapeTrack(i).c_str());
    static const char* const KN[DECK_KINDS] = {"", "insert", "change", "play", "stop"};
    for (int kind = DECK_INSERT; kind < DECK_KINDS; kind++)
        for (uint32_t seed = 1; seed <= 3; seed++) {
            const std::vector<float> a = deckRender(kind, seed, 22050);
            std::vector<int16_t> pcm(a.size());
            for (size_t i = 0; i < a.size(); i++) pcm[i] = (int16_t)std::lround(std::max(-1.0f, std::min(1.0f, a[i] * 0.8f)) * 32000);   // at the default master
            const std::string fn = fmt("shots/tests/tape_deck_%s_%u.wav", KN[kind], seed);
            writeWav16(fn, pcm, 22050);
            if (seed == 1) printf("  deck %-6s the tape at %.2f s of %.2f -> %s (and _2, _3)\n", KN[kind], deckTapeAt(kind, seed), deckLength(kind, seed), fn.c_str());
        }
    game.setState(GameState::SPACE);
    game.testOpenAuto();
    { Input in; game.frame(in, 1.0 / 30); }
    writePNG("shots/tests/tape_00.png", game.output(), FBW, FBH);   // the chooser
    if (n == 0) { pngFlush(); printf("tape: no tracks, so no captions\n"); return 0; }
    game.autoStart(AP_PLANETFALL, 3);
    for (const Body& ob : game.testSystem().bodies) if (ob.type != PT_COMPANION) game.testWaitMapOf(ob.index);   // every map first (made on their thread at a binary's own pace)
    TapePump pump; pump.len = 12;
    Input in; const double dt = 1.0 / 30;
    auto frame = [&]() { game.frame(in, dt); in.newFrame(); pump.step(game.audio, dt); };
    in.pressed[KEY_M] = in.down[KEY_M] = true; frame(); in.down[KEY_M] = false;
    int shot = 1;
    auto snap = [&](const char* what) {
        std::string au, ti;
        const double a = game.tapeCaption(au, ti);
        printf("  tape_%02d: %s (caption %.2f: %s / %s)\n", shot, what, a, au.c_str(), ti.c_str());
        writePNG(fmt("shots/tests/tape_%02d.png", shot++).c_str(), game.output(), FBW, FBH);
    };
    while (!pump.playing) frame();
    for (int i = 0; i < 6; i++) frame();
    snap("fading in");
    for (int i = 0; i < 45; i++) frame();
    snap("whole");
    for (int i = 0; i < 150; i++) frame();
    snap("fading out");
    for (int i = 0; i < 40; i++) frame();
    snap("gone");
    if (n > 1) { while (pump.decks[DECK_CHANGE] == 0) frame(); while (!pump.playing) frame(); for (int i = 0; i < 45; i++) frame(); snap("the next track"); }
    game.musicDir = "shots/tests/tape_none";
    in.pressed[KEY_M] = in.down[KEY_M] = true; frame(); in.down[KEY_M] = false;   // the stop key
    for (int i = 0; i < 15; i++) frame();
    in.pressed[KEY_M] = in.down[KEY_M] = true; frame(); in.down[KEY_M] = false;   // an empty folder
    for (int i = 0; i < 30; i++) frame();
    snap("an empty folder");
    pngFlush();
    printf("tape: %s\n", game.testTapeInfo().c_str());
    return 0;
}

int runTail(int argc, char** argv) {
    const long long sx = argc > 2 ? atoll(argv[2]) : 18071, sy = argc > 3 ? atoll(argv[3]) : 1, sz = argc > 4 ? atoll(argv[4]) : -19950;
    const int body = argc > 5 ? atoi(argv[5]) : 1;
    Star s;
    if (!starInSector(sx, sy, sz, s, true)) { printf("tail: no star at %lld %lld %lld\n", sx, sy, sz); return 1; }
    TestGame game;
    game.savePrefix = "shots/tests/tail_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.guidePath = "shots/tests/tail_guide.txt";
    remove(game.guidePath.c_str());
    game.newGame(); game.settings.cabin = false; game.applySettings();
    game.testParkAt(s, body, false);
    Input in;
    auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
    run(1.0);
    const StarSystem& sys = game.testSystem();
    printf("tail: class S%02d, body %d of %d: %s R %.0f km; %s\n", sys.star.cls, body, (int)sys.bodies.size(), KINDS[sys.bodies[body].type].name, sys.bodies[body].radiusKm,
            game.testBodyTypes().c_str());
    for (int k = 0; k < 8; k++) {
        if (k > 0) { in.down[KEY_LEFT_SHIFT] = true; in.down[KEY_RIGHT] = true; run(45.0 / 20.0); in.down[KEY_LEFT_SHIFT] = false; in.down[KEY_RIGHT] = false; run(0.5); }
        int top = 0, bright = 0, tail = 0; double tmax = 0, tsum = 0;
        for (int i = 0; i < FBW * FBH; i++) {
            const double v = shadeOf(game.fb.idx[i]);
            if (v >= 62) top++;
            if (v >= 50) bright++;
            if (bankOf(game.fb.idx[i]) == 2 && game.fb.invz[i] < 1e-12f) { tail++; tmax = std::max(tmax, v); tsum += v; }   // the giant's bank off any body: its tail
        }
        const std::string path = fmt("shots/tests/tail_%d.png", k * 45);
        writePNG(path.c_str(), game.output(), FBW, FBH);
        printf("  %3d degrees east: pixels at the ramp's top %d, over shade 50 %d; the tail's %d pixels, shade %.1f at most, %.1f mean -> %s\n", k * 45, top, bright, tail, tmax,
                tail ? tsum / tail : 0.0, path.c_str());
    }
    const double R = sys.bodies[body].radiusKm;
    for (double kR : {12.0, 40.0}) {   // farther: the ship set down beside the giant, the tail across the view
        const Vec3 bp = sys.bodyPos(body, game.testGameTime()), away = normalize(bp - sys.star.pos);
        const Vec3 sideV = normalize(cross(away, Vec3(0, 1, 0)));
        game.testPlaceShip(bp + normalize(sideV - away * 0.3) * (R * kR));
        game.testAimAtBody(body);
        run(0.5);
        const std::string path = fmt("shots/tests/tail_far%.0f.png", kR);
        writePNG(path.c_str(), game.output(), FBW, FBH);
        printf("  from %.0f radii -> %s\n", kR, path.c_str());
    }
    {   // a moon's sky: 40 degrees from the point under the giant, looking at it
        StarSystem msys; msys.generate(s);
        int moon = -1;
        for (const Body& m : msys.bodies) if (m.parent == body && m.type != PT_COMPANION) { moon = m.index; break; }
        if (moon >= 0) {
            double t = game.testGameTime();   // the moon at quadrature (the sun 90 degrees from the giant in its sky): the tail side-on
            for (double t0 = t, best = 2, dt = 0; dt < 3 * 86400; dt += 30) {
                const double cq = std::fabs(dot(normalize(msys.star.pos - msys.bodyPos(moon, t0 + dt)), normalize(msys.bodyPos(body, t0 + dt) - msys.bodyPos(moon, t0 + dt))));
                if (cq < best) { best = cq; t = t0 + dt; }
                if (best < 0.01) break;
            }
            const Mat3 F = msys.bodyFrame(moon, t);
            const Vec3 up = F * normalize(msys.bodyPos(body, t) - msys.bodyPos(moon, t)), sunB = F * normalize(msys.star.pos - msys.bodyPos(moon, t));
            Vec3 e = sunB - up * dot(sunB, up); if (length(e) < 1e-6) e = Vec3(1, 0, 0);
            e = -normalize(e);   // away from the sun: the giant 50 degrees up, the sun under the horizon
            const Vec3 u = normalize(up * std::cos(40 * DEG) + e * std::sin(40 * DEG));
            double la, lo; StarSystem::latLonFromBody(u, la, lo);
            SpaceRenderer sr; StarNeighborhood snb; SurfaceView sv;
            sv.init(&msys, moon, la, lo, t);
            snb.update(sv.site.worldPos(t, 0, 0, 0));
            Input in;
            for (int k = 0; k < 10; k++) sv.update(0.016, in, t, false);
            const Vec3 l = sv.site.localFrame(t) * normalize(msys.bodyPos(body, t) - sv.site.worldPos(t, sv.player.x, sv.player.z, 0));
            sv.player.yaw = std::atan2(l.x, l.z); sv.player.pitch = std::asin(clampd(l.y, -1, 1)) + 22 * DEG;   // the giant low in the frame, the tail rising from it
            Framebuffer fb; sv.render(fb, t, snb.stars, sr, 1.0); fb.mush(2);
            saveFB(fb, "shots/tests/tail_sky.png");
            printf("  from moon %d (%s), lat %.1f lon %.1f, the giant %.0f degrees up -> shots/tests/tail_sky.png\n", moon, KINDS[msys.bodies[moon].type].name, la / DEG, lo / DEG,
                    std::asin(clampd(l.y, -1, 1)) / DEG);
        }
    }
    {   // the cabin's front window and one thread at 1x, 135 degrees east of the parking
        game.settings.cabin = true; game.applySettings();
        game.testParkAt(s, body, false);
        in.down[KEY_LEFT_SHIFT] = true; in.down[KEY_RIGHT] = true; run(135.0 / 20.0); in.down[KEY_LEFT_SHIFT] = false; in.down[KEY_RIGHT] = false; run(0.5);
        writePNG("shots/tests/tail_cabin.png", game.output(), FBW, FBH);
        game.settings.renderScale = 1; game.applySettings(); run(0.3);
        writePNG("shots/tests/tail_1x.png", game.output(), FBW, FBH);
        printf("  the cabin at 135 degrees east -> shots/tests/tail_cabin.png, at 1x -> shots/tests/tail_1x.png\n");
    }
    return 0;
}

// R-419: `auto [profile] [minutes] [seed] [cockpit]` (`scale=N` for the frames): a fresh expedition from home played by auto-explore for that many
// minutes of frames (30 a second, drawn as fast as they come: the threads' work arrives when it is done, as on a slow machine), its steps
// as they change, a frame every 20 s into shots/tests/auto_<profile>_NNN.png; at the end what it did and how the eye moved: the cuts (a
// scene's change, the roof deck, the telescope, a vehicle, an arrival: a turn over 12 degrees in a frame), the fastest turn outside them
// and the share of the frames turning faster than 60 degrees a second
int runAuto(int argc, char** argv) {
    int profile = AP_WANDERER;
    if (argc > 2) { std::string a = upper(argv[2]); for (int i = 0; i < AP_COUNT; i++) if (a == AUTO_PROFILE_NAMES[i] || a == std::to_string(i)) profile = i; }
    const double minutes = argc > 3 ? atof(argv[3]) : 10.0;
    const uint64_t seed = argc > 4 ? strtoull(argv[4], nullptr, 10) : 1;
    std::string lc = AUTO_PROFILE_NAMES[profile]; for (char& c : lc) c = (char)tolower((unsigned char)c);
    TestGame game;   // each profile its own files (two may run side by side)
    game.savePrefix = "shots/tests/auto_save_" + lc;
    game.settingsPath = "shots/tests/auto_settings_" + lc + ".txt";
    game.guidePath = "shots/tests/auto_guide_" + lc + ".txt";
    game.keysPath = "shots/tests/test_keys.txt";
    remove(game.guidePath.c_str()); remove((game.savePrefix + "_auto.txt").c_str());
    applyTestScale(game);   // `scale=N` (the game's own setting would draw at 4x)
    if (argc > 5 && std::string(argv[5]) == "cockpit") { game.settings.cabin = false; game.applySettings(); lc += "_cockpit"; }   // the classic cockpit's view: the look is the ship's nose
    game.setState(GameState::SPACE);
    game.autoStart(profile, seed);
    printf("auto: %s for %.1f minutes of frames (seed %llu) at %dx\n", AUTO_PROFILE_NAMES[profile], minutes, (unsigned long long)seed, FB_SCALE);
    Input in;
    const double tick = 1.0 / 30;
    const long frames = (long)(minutes * 60 * 30);
    std::string lastDoing, lastTown; int lastRecords = 0; int lastState = -1, shotN = 0, cuts = 0; long fast = 0, counted = 0; double maxTurn = 0, maxTurnAt = 0; Vec3 prev; bool havePrev = false;
    struct Turn { double rate, at; int state, mode; std::string doing; };
    std::vector<Turn> turns;   // the fastest, each a second apart
    const double t0 = nowSec();
    for (long f = 0; f < frames && game.autoOn(); f++) {
        game.frame(in, tick); in.newFrame();
        const double clock = f * tick;
        const int st = (int)game.state;
        const Vec3 v = game.testViewDir();
        if (havePrev) {
            const double a = std::acos(clampd(dot(v, prev), -1, 1)) / DEG;
            if (st != lastState || a > 12) cuts++;   // no hand turns 360 degrees a second: the game's own snaps (a vehicle's seat, the eyepiece)
            else {
                counted++; if (a * 30 > 60) fast++; if (a * 30 > maxTurn) { maxTurn = a * 30; maxTurnAt = clock; }
                if (a * 30 > 45 && std::getenv("VESPERIS_AUTOTRACE")) printf("    trace %.2f s: %.0f deg/s, %s\n", clock, a * 30, game.testAngles().c_str());
                if (a * 30 > 45) {
                    bool merged = false;
                    for (Turn& T : turns) if (std::fabs(T.at - clock) < 1) { if (a * 30 > T.rate) T = {a * 30, clock, st, game.testShipMode(), game.testAutoDoing()}; merged = true; }
                    if (!merged) turns.push_back({a * 30, clock, st, game.testShipMode(), game.testAutoDoing()});
                }
            }
        }
        prev = v; havePrev = true;
        const std::string doing = game.testAutoDoing();
        if (game.testAutoCount(4) != lastRecords) { printf("  %3d:%02d  a record taken (%d)\n", (int)clock / 60, (int)clock % 60, game.testAutoCount(4)); lastRecords = game.testAutoCount(4); }
        if (game.testAutoTown() != lastTown) { printf("  %3d:%02d  %s\n", (int)clock / 60, (int)clock % 60, game.testAutoTown().c_str()); lastTown = game.testAutoTown(); }
        if (doing != lastDoing || st != lastState) {
            printf("  %3d:%02d  %-11s %s\n", (int)clock / 60, (int)clock % 60, stateName((GameState)std::min(st, STATE_COUNT - 1)), doing.c_str());
            fflush(stdout);
            lastDoing = doing;
        }
        lastState = st;
        if (f % (30 * 20) == 0) { writePNG(fmt("shots/tests/auto_%s_%03d.png", lc.c_str(), shotN++).c_str(), game.output(), FBW, FBH); }
        static const char* stepShots = std::getenv("VESPERIS_AUTOSHOT");   // a step's frames twice a second (`the fine approach`), forty at most
        static int stepShotN = 0;
        if (stepShots && doing == stepShots && f % 15 == 0 && stepShotN < 40) writePNG(fmt("shots/tests/auto_%s_step_%02d.png", lc.c_str(), stepShotN++).c_str(), game.output(), FBW, FBH);
    }
    pngFlush();
    printf("auto: %s; %.0f s of frames in %.0f s\n", game.testAutoInfo().c_str(), frames * tick, nowSec() - t0);
    printf("auto: the eye: %d cuts; outside them the fastest turn %.0f deg/s (at %d:%02d), %.2f%% of %ld frames over 60 deg/s; %d frames written\n", cuts, maxTurn, (int)maxTurnAt / 60,
            (int)maxTurnAt % 60, counted ? 100.0 * fast / counted : 0.0, counted, shotN);
    std::sort(turns.begin(), turns.end(), [](const Turn& a, const Turn& b) { return a.rate > b.rate; });
    for (size_t i = 0; i < turns.size() && i < 8; i++)
        printf("  a turn of %.0f deg/s at %d:%02d (%s, ship mode %d): %s\n", turns[i].rate, (int)turns[i].at / 60, (int)turns[i].at % 60,
                stateName((GameState)std::min(turns[i].state, STATE_COUNT - 1)), turns[i].mode, turns[i].doing.c_str());
    return 0;
}
