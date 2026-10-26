// E-03: vesperis_test's start: `scale=N` taken off the command line, shots/tests made, and the mode the first argument names run
// from MODES, the table `help` prints (each row names the file its mode is in)
#include "harness/harness.h"
#include "harness/test_worlds.h"

int g_testScale = 1;   // `scale=N` on the command line renders every Game-based mode at that scale

// E-03: the modes, help's order: main() runs the row whose name is the first argument (no argument: `stars`)
static int runHelp(int argc, char** argv);
static const Mode MODES[] = {
        {"stars", "galaxy", "", "the star and body statistics round the start region, four systems in detail (no argument runs it)", "modes_galaxy.cpp",
                [](int argc, char** argv) -> int {
                    testStars();
                    return 0;
                }},
        {"galaxy", "galaxy", "", "G-01: the density function in numbers, the home sectors, the sky from above the home star", "modes_galaxy.cpp",
                [](int argc, char** argv) -> int {
                    runGalaxy();
                    return 0;
                }},
        {"chart", "galaxy", "", "G-05: the chart's column against the field, its cost, and the star map's galaxy page at every level", "modes_galaxy.cpp",
                [](int argc, char** argv) -> int { return runChart(); }},
        {"crossing", "galaxy", "", "G-07: two long flights from home through the cabin, ahead and aside", "modes_galaxy.cpp", [](int argc, char** argv) -> int { return runCrossing(); }},
        {"home", "galaxy", "", "G-02: the new-game search's candidates, each parked at its first living world and timed", "modes_galaxy.cpp",
                [](int argc, char** argv) -> int {
                    runHome(); return 0;
                }},
        {"survey", "galaxy", "[n]", "M9-01: distributions over n bodies near the start (2,000): types, heat, gravity, moons, materials, the sky", "modes_galaxy.cpp",
                [](int argc, char** argv) -> int {
                    runSurvey(argc > 2 ? atoi(argv[2]) : 2000);
                    return 0;
                }},
        {"maps", "space", "", "a planet map per type", "modes_space.cpp",
                [](int argc, char** argv) -> int {
                    renderMaps();
                    return 0;
                }},
        {"space", "space", "", "each sun class from its first orbit, the globes, rings and belts, the telescope's and the fronts' frames", "modes_space.cpp",
                [](int argc, char** argv) -> int {
                    renderSpace();
                    return 0;
                }},
        {"moonsky", "space", "", "the sky from giants' moons, an eclipse, ringed and substellar skies; a binary's ground", "modes_space.cpp",
                [](int argc, char** argv) -> int {
                    renderMoonSky();
                    {
                        SpaceRenderer sr2;
                        StarNeighborhood nb2;
                        renderBinarySurface(sr2, nb2);
                    }
                    return 0;
                }},
        {"almanac", "space", "[<sx> <sy> <sz> <body> [latDeg lonDeg [t [ship]]]]", "W-01: a place's almanac and its cost (the unit's three test places by default)", "modes_space.cpp", runAlmanac},
        {"showers", "space", "[<sx> <sy> <sz> <body>]", "W-04: the meteor showers of the systems near home, or a world's", "modes_space.cpp", runShowers},
        {"seasons", "space", "", "M5-06: the day at 82 N of a tilted felisian world at both solstices", "modes_space.cpp",
                [](int argc, char** argv) -> int {
                    testSeasons();
                    return 0;
                }},
        {"probe", "space", "[<sx> <sy> <sz> <body>] [chute] [night] [wav] | giants | stability [scale]",
                "X-01: a giant's air and a probe's fall into it; X-03: the giants' characters; X-02: the decks held still", "modes_probe.cpp",
                [](int argc, char** argv) -> int {
                    if (argc > 2 && std::string(argv[2]) == "stability") return runProbeStability();
                    if (argc > 2 && std::string(argv[2]) == "giants") return runProbeGiants();
                    return runProbe(argc, argv);
                }},
        {"surface", "surface", "[type]", "every landable type (or one) at noon, sunset, night and at 82 degrees", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    renderSurface(argc > 2 ? atoi(argv[2]) : -1);
                    return 0;
                }},
        {"scene", "surface", "<name>|list [scale] [warm]", "a scene of scenes_table.h, its numbers printed; `list` names them all", "modes_surface.cpp", runScene},
        {"capsule", "surface", "[scene]", "B-318: a scene's ground from the descending capsule at eight heights, sky through the ground counted", "modes_surface.cpp", runCapsule},
        {"spot", "surface", "<sx> <sy> <sz> <body> <latDeg> <lonDeg> <t> <swim|ascent|stand|dump> [pitchDeg] [yawDeg] [alt]", "B-322: a frame at an exact place, from a save's lines",
                "modes_surface.cpp", runSpot},
        {"consistency", "surface", "", "N0-02: for every landable type the landing map, the globe and the ground at one point", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    runConsistency();
                    return 0;
                }},
        {"drive", "surface", "", "N4-02: the buggy driven on open ground, frames from the seat", "modes_surface.cpp", [](int argc, char** argv) -> int { return runDrive(); }},
        {"bestiary", "surface", "", "N3-01: the bestiary of the first living worlds near the start, made twice", "modes_surface.cpp", [](int argc, char** argv) -> int { return runBestiary(); }},
        {"encounters", "surface", "[n]", "N3-04: the share of n random felisian sites with a herd within 200 m, by biome", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    runEncounters(argc > 2 ? atoi(argv[2]) : 300);
                    return 0;
                }},
        {"landmaps", "surface", "", "the landing map of a body of each landable type, its darkest shadow", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    renderLandingMaps();
                    return 0;
                }},
        {"stability", "surface", "[scene] [metres per frame] [scale] | flight [scale] | skyline [scale]",
                "B-309: what moves in a scene's frames; G-07: the band in the flights; A-02: the city's skyline", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    if (argc > 2 && std::string(argv[2]) == "flight") {
                        if (argc > 3) setFramebufferScale(atoi(argv[3]));
                        return runFlightStability();
                    }
                    if (argc > 2 && std::string(argv[2]) == "skyline") {
                        const char* a[] = {argv[0], "forms", "stability", argc > 3 ? argv[3] : "2"};
                        return runForms(4, (char**)a);
                    }
                    // B-309; B-313: `stability <scene> [metres per frame] [scale]`
                    if (argc > 4) setFramebufferScale(atoi(argv[4]));
                    return runStability(argc > 2 ? argv[2] : nullptr, argc > 3 ? atof(argv[3]) : 0.0);
                }},
        {"compare", "surface", "", "B-310: eight scenes in six configurations of the render scale and the mush", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    renderCompare();
                    return 0;
                }},
        {"sheet", "surface", "", "B-310: compare's frames as the window shows them, a sheet a scene", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    composeSheets();
                    return 0;
                }},
        {"gallery", "surface", "[type]", "M9-01: sixteen random landing sites of a type as a contact sheet", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    renderGallery(argc > 2 ? atoi(argv[2]) : PT_FELISIAN);
                    return 0;
                }},
        {"reanchor", "surface", "[comet [scale]]", "KI-007: a 51 km walk across the site's re-anchor; F-01: the comet's re-anchor timed", "modes_surface.cpp",
                [](int argc, char** argv) -> int {
                    if (argc > 2 && std::string(argv[2]) == "comet") return runReanchorComet(argc, argv);
                    testReanchor();
                    return 0;
                }},
        {"terrain", "terrain", "[type]", "O6-01: 48 land sites a landable type on four grids, judged by relief class", "modes_terrain.cpp",
                [](int argc, char** argv) -> int {
                    if (argc > 2) g_terrainType = atoi(argv[2]);
                    return runTerrainStats();
                }},
        {"landforms", "terrain", "<type> [flat|hills|mountains] | sites", "O6-01: the landform gallery, or the fixed review sites", "modes_terrain.cpp", runLandforms},
        {"descent", "terrain", "[latDeg lonDeg]", "B-307: the buggy down the steepest descent at the pinned mountain site", "modes_terrain.cpp",
                [](int argc, char** argv) -> int {
                    return runDescent(argc > 3 ? atof(argv[2]) : PIN_MOUNTAIN_LAT, argc > 3 ? atof(argv[3]) : PIN_MOUNTAIN_LON);   // B-307; G-04: `descent [latDeg lonDeg]`
                }},
        {"matlod", "terrain", "[all|<type>] [bodies] [sites] [halfKm] [stepM] | spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> [halfKm] [stepM]",
                "B-404: the materials and heights at the five ring scales, point by point", "modes_terrain.cpp", runMatlod},
        {"landmarks", "terrain", "[type]", "O6-06, R-408: the sights round 24 random land sites of a type", "modes_terrain.cpp", runLandmarks},
        {"drain", "terrain", "[type]", "O6-03: the drainage round the first land sites of a type: its cost, channels and lakes", "modes_terrain.cpp", runDrain},
        {"water", "terrain", "[n]", "B-320: the inland water of the first n felisian worlds at 16 m", "modes_terrain.cpp", runWater},
        {"climate", "terrain", "[n]", "B-321: the first n felisian worlds' climates, biome shares and land ice", "modes_terrain.cpp", runClimate},
        {"kinds", "kinds", "[name|ground|family]", "P-00: a sheet a kind of world (or a ground's kinds, a family's)", "modes_kinds.cpp", runKinds},
        {"traits", "kinds", "[name|new|karst]", "R-304, Q-01: the first body with each landform trait, its sheet", "modes_kinds.cpp",
                [](int argc, char** argv) -> int {
                    renderTraits(argc > 2 ? argv[2] : nullptr);
                    return 0;
                }},
        {"tides", "kinds", "[scene]", "Q-02: the great tides of the scan region's first worlds that have them", "modes_kinds.cpp", runTides},
        {"datasheet", "kinds", "[n]", "Q-01: the n data sheets that run longest, in space and on the ground, a page a frame", "modes_kinds.cpp", runDataSheet},
        {"salt", "kinds", "scan [name|ground|family] [tries]", "E-07: the made kinds against the protected worlds as kindOf walks them: what each reaches and takes, the first salt taking none",
                "modes_kinds.cpp", runSaltScan},
        {"signals", "radio", "[<sx> <sy> <sz>] [reach] [xG] | wav", "C-07: the far signals heard from a sector; their WAVs", "modes_signals.cpp", runSignals},
        {"voice", "radio", "<sx> <sz> <body> [index] [wav]", "C-05: a world's voice, a record spoken", "modes_signals.cpp", runVoice},
        {"sounds", "radio", "", "Q-03: the ground's sounds through the synth: the dunes, the ice, a quake", "modes_signals.cpp", runSounds},
        {"rogues", "radio", "[ly] | heard | voices | dark", "D-01: the drifters near home; D-02 to D-04: heard, their voices, their dark", "modes_rogues.cpp", runRogues},
        {"peoples", "peoples", "ages | fresh | map | scorched | rogues | <sx> <sz> <body> [second]",
                "C-13, A-01: the peoples near home by age, a world's peoples, the scorched worlds' and the drifters'", "civ_peoples.cpp", runPeoples},
        {"history", "peoples", "<sx> <sz> <body> [second] | ages | check", "H-02: a people's history, printed as legends", "civ_peoples.cpp", runHistory},
        {"genome", "peoples", "sheet | street | dress | ages | reach | distinct | pair | print | ...", "A-04: the peoples' genome, its architecture sheets", "civ_genome.cpp", runGenome},
        {"forms", "peoples", "sheet | city | skyline | stability | check [scale]", "A-02: the forms' sheet, the test city, its skyline", "civ_forms.cpp", runForms},
        {"plan", "peoples", "<sx> <sz> <body> [second] | ages | view", "T-01: a world's plan for each age", "civ_plan.cpp", runPlan},
        {"towns", "peoples", "<sx> <sz> <body> [second] | ages | view | check", "T-02: a people's towns", "civ_plan.cpp", runTowns},
        {"roads", "peoples", "<sx> <sz> <body> [latDeg lonDeg]", "C-09: a world's roads", "civ_plan.cpp", runRoads},
        {"works", "peoples", "<sx> <sz> <body> [second] | ages | map | gallery | view | rogue <k> [ly] | check", "T-03: a people's works", "civ_works.cpp", runWorks},
        {"ruins", "peoples", "<sx> <sz> <body> [latDeg lonDeg] | ages | kinds | check", "C-01: a world's culture and the ruins round a point", "civ_lore.cpp", runRuins},
        {"shards", "peoples", "<sx> <sz> <body> [second] | drifters [n] [held] | rogue", "C-02: a people's fifty records as the decoder reads them", "civ_lore.cpp", runShards},
        {"charts", "peoples", "<sx> <sz> <body> | ages | rogues | check", "C-10: the star charts, in each age's way", "civ_lore.cpp", runCharts},
        {"graves", "peoples", "<sx> <sz> <body> [latDeg lonDeg] [cells]", "C-12: a world's calendar, its last recording and its graves", "civ_lore.cpp", runGraves},
        {"music", "peoples", "<sx> <sz> <body> [index] [wav] [age=N] [tone=N] | ages [wav]", "C-04: a people's music", "civ_music.cpp", runMusic},
        {"records", "peoples", "<sx> <sz> <body> [second] | ages | scenes | check", "T-04: a people's records and where they lie", "civ_records.cpp", runRecords},
        {"decoder", "peoples", "[ages] | counts | pages | wav | check", "H-05, H-06: the decoder's counts, pages and media", "civ_records.cpp", runDecoder},
        {"instruments", "peoples", "[ages] | <sx> <sz> <body> [second] | scenes | sheet | plans | distinct | wav | check", "C-11, C-17: the peoples' instruments, their makers and the player",
                "civ_instruments.cpp", runInstruments},
        {"grammar", "peoples", "[dir D] [embedded] [near N] [print N|all] [rogues[=ly]] | check | where [<rule>|<prefix>.*|check|md] | schema",
                "H-03: the records' grammar read from the tree; E-11: where a rule is, what it may name", "civ_grammar.cpp", runGrammar},
        {"tongues", "peoples", "[n]", "H-04: the tongues of the n peoples nearest home", "civ_grammar.cpp", runTongues},
        {"walls", "peoples", "forms [peoples] [years ...] | one | at | place | reach | view | walk | check", "B-420, B-421: every piece against its colliders", "civ_walls.cpp", runWalls},
        {"flow", "game", "", "the scripted expedition through every screen and device; read its status lines", "modes_game.cpp",
                [](int argc, char** argv) -> int {
                    runGameFlow();
                    return 0;
                }},
        {"settings", "game", "", "the settings file round trip and the settings screen", "modes_game.cpp",
                [](int argc, char** argv) -> int {
                    testSettings();
                    return 0;
                }},
        {"audio", "game", "", "M4-09: every family of sound through the synth into a WAV", "modes_game.cpp",
                [](int argc, char** argv) -> int {
                    testAudio();
                    return 0;
                }},
        {"input", "game", "", "M6-02, R-418: the key map and the controller", "modes_input.cpp", [](int argc, char** argv) -> int { return testInput(); }},
        {"tour", "game", "[state ...]", "U-01: every state opened as the game opens it, its controller menu, its keys and rows; Esc last", "modes_tour.cpp", runTour},
        {"fuzz", "game", "[n]", "M0-08: random keys and mouse for n frames (10,000) across every state", "modes_input.cpp",
                [](int argc, char** argv) -> int { return runFuzz(argc > 2 ? atoi(argv[2]) : 10000); }},
        {"zoom", "game", "<sx> <sz> <body> <latDeg> <lonDeg> [scale]", "C-09: the landing map's zoom window over a point", "modes_game.cpp", runZoom},
        {"drones", "game", "[frames] | check | world <sx> <sz> <body>", "C-18: the drone bay in a game", "modes_drones.cpp", runDrones},
        {"parkings", "game", "", "B-428: twelve parkings of the scenes that were crowded", "modes_game.cpp", [](int argc, char** argv) -> int { return runParkings(); }},
        {"tape", "game", "[dir]", "R-420: auto-explore's tape, its tracks and the deck's sequences", "modes_game.cpp", runTape},
        {"tail", "game", "[<sx> <sy> <sz> <body>]", "B-427: a Wolf-Rayet star's giant and the tail its wind strips", "modes_game.cpp", runTail},
        {"auto", "game", "[profile] [minutes] [seed] [cockpit]", "R-419: an expedition played by auto-explore", "modes_game.cpp", runAuto},
        {"unit", "proof", "[list] [-q] [group ...]", "the unit's checks, all or by group (`unit list`); -q: only the failures", "unit.cpp", testUnit},
        {"regress", "proof", "[bless]", "M0-03: the determinism hashes against tests/regress_baseline.txt (bless: with GEN_VERSION only)", "modes_proof.cpp",
                [](int argc, char** argv) -> int { return runRegress(argc > 2 && std::string(argv[2]) == "bless"); }},
        {"fingerprint", "proof", "bodies | stream | peoples | modules [n] [out] [full]", "E-02: what the generators make, as keyed lines (tools/prove)", "fingerprint.cpp", runFingerprint},
        {"screens", "proof", "[scale ...]", "F-05: the game's frames the regress never hashes", "modes_proof.cpp", runScreens},
        {"pngdiff", "proof", "<a.png> <b.png> [diff.png]", "F-01: two PNGs pixel for pixel", "modes_proof.cpp", runPngDiff},
        {"bothways", "proof", "<scene> [scale]", "F-01: a scene by the workers and by one thread", "modes_proof.cpp", runBothWays},
        {"suncone", "proof", "", "F-05: the sun's cone test swept to the frame's corner", "modes_proof.cpp", [](int argc, char** argv) -> int { return runSunCone(); }},
        {"bench", "bench", "[check [slack]] | forest | noise", "the frame's costs; `check` against the 38 budgets", "modes_bench.cpp",
                [](int argc, char** argv) -> int {
                    if (argc > 2 && std::string(argv[2]) == "noise") return runBenchNoise();
                    // `bench check <factor>`
                    return argc > 2 && std::string(argv[2]) == "forest" ? runBenchForest() : runBench(argc > 2 && std::string(argv[2]) == "check", argc > 3 ? std::max(0.1, atof(argv[3])) : 1.0);
                }},
        {"pool", "bench", "", "F-06: the pool's dispatch timed and its dealing checked", "modes_bench.cpp", [](int argc, char** argv) -> int { return runPool(); }},
        {"load", "bench", "landing | map | arrival | drive | capital | probe | roads [seconds] [scale] [age]", "B-422: the cores kept busy and the pace", "modes_bench.cpp", runLoad},
        {"soak", "bench", "[minutes] [scale]", "F-01: a long expedition at 4x", "modes_bench.cpp", runSoak},
        {"help", "harness", "[md]", "this list; `md` as the docs' index", "main.cpp", runHelp},
};

// E-03: `help`: every mode by group with its arguments and its file; `help md`: the same as the docs' index (a table a group)
static int runHelp(int argc, char** argv) {
    const bool md = argc > 2 && std::string(argv[2]) == "md";
    const char* last = "";
    if (md) printf("# The harness's modes\n\nWritten by `vesperis_test help md` from `src/harness/main.cpp`'s `MODES`: edit the table, not "
                   "this page. `vesperis_test <mode> [arguments]`; `scale=N` anywhere renders at N x; no mode runs `stars`.\n");
    else printf("vesperis_test <mode> [arguments]   (scale=N anywhere: render at N x; no mode: stars)\n");
    for (const Mode& m : MODES) {
        if (strcmp(m.group, last) != 0) {
            if (md) printf("\n## %s\n\n| Mode | Arguments | What it does | File |\n| --- | --- | --- | --- |\n", m.group);
            else printf("\n%s\n", m.group);
            last = m.group;
        }
        if (md) {
            std::string usage;
            for (const char* c = m.usage; *c; c++) usage += *c == '|' ? std::string("\\|") : std::string(1, *c);
            printf("| `%s` | %s | %s | %s |\n", m.name, usage.empty() ? "" : ("`" + usage + "`").c_str(), m.help, m.file);
        } else {
            printf("  %s%s%s   (%s)\n      %s\n", m.name, *m.usage ? " " : "", m.usage, m.file, m.help);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    std::atexit(worldPlanThreadsJoin);   // C-18: a plan's prefetch still writing its histories and works is waited for before their caches go
    makeDir("shots");
    makeDir("shots/tests");   // every harness output lands here; shots/ itself holds the player's screenshots
    Game::musicDirDefault = "shots/tests/tape_none";   // a player's own music folder plays in no check (`tape` reads it on purpose)
    // `scale=N` anywhere on the command line renders at that resolution scale
    std::vector<char*> args;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "scale=", 6) == 0) g_testScale = atoi(argv[i] + 6); else args.push_back(argv[i]);
    }
    argc = (int)args.size(); argv = args.data();
    if (g_testScale < 1) g_testScale = 1;
    if (g_testScale > 4) g_testScale = 4;
    setFramebufferScale(g_testScale);
    std::string mode = argc > 1 ? argv[1] : "stars";
    for (const Mode& m : MODES)
        if (mode == m.name) return m.run(argc, argv);
    printf("unknown mode\n");   // and exits 0, as it always did (a script cannot tell; E-03 leaves it)
    return 0;
}
