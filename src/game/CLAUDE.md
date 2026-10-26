# src/game: the game

`Game` (game.h, 42 KB) owns the states, the ship, the guide and the audio state; each screen or device is a file of `Game`
methods. game.h opens with an index of its sections, each under a `// ----` banner naming its files; a function's comment is
at its definition. What each state is, is its row in the states' table (game_types.h). The harness's hooks are TestGame's
(src/harness/test_game.h), none in this directory. Sizes are in KB.

## Files
- `game.h` + `game.cpp` (28): `class Game`; `newGame`, `frame`, `handleGlobalKeys`
- `game_types.h`: `GameState` and its table (`STATE_ROWS`, `stateHas`), `Cabin`, `CabinFrame`, `ShipState`, `HarnessKnobs`, the
  cabin's banks and `SCREENS`, the free functions the harness calls too. The subsystems' state, each a `Game` member:
  `landing_state.h`, `pad_state.h`, `chart_state.h`, `shard_world.h`, `radar_state.h`, `player_state.h`, `case_state.h`,
  `drone_state.h` (and the drone's model), `telescope_state.h`, `probe_state.h` (the lightning's `flashAt`),
  `autoexplore_state.h`, `tape_state.h`. A split subsystem's shared helpers: `autoexplore_internal.h`, `probe_internal.h`,
  `radar_internal.h`, `audio_internal.h`
- `autopilot.cpp` (29): SPACE's input and flight: `updateSpace`, `plotJump`, `startApproach`, parking
- `scene_draw.cpp` (7): the scenes, `renderSpace` and `renderSurfaceScene`, the vision modes; `hud.cpp` (36): the HUDs
- `surface_play.cpp` (20): the capsule's descent and ascent, the ground (`updateSurface`); `sector_map.cpp` (22): SECTOR_MAP
- `cabin.cpp` (42): the cabin (`updateCabin`, `drawCabin`), the flight computer (SHIPSCREEN), the CONSOLE
- `menus.cpp` (15): the title, HELP, MENU, SETTINGS and KEYS; `data_sheet.cpp` (19): SYSTEM_LIST, DATA and its forecast
- `guide.h/.cpp` (25): `Guide` (guide.txt): names, the log, marks, shards, probes
- `names.cpp` (12): `starNameOf`, `bodyNameOf`, the guide's keys, what is taken (`takeShard`), `logEvent`, `noteVisit`
- `guide_screens.cpp` (23): GUIDE, TEXT_ENTRY, LOG, STATS, GALLERY; `star_map.cpp` (14): STAR_MAP, the targets
- `galaxy_chart.cpp` (46): the star map's galaxy page: `renderGalaxyChart`, `chartEnter`, marks
- `landing_map.cpp` (42): `beginLanding`, `updateLandingMap`, `renderLandingMap`; `landing_map_build.cpp` (18): its pictures
- `marks.cpp` (21): surface marks and the scanner (R): `beginMark`, `scanCycle`, `drawScanHUD`
- `radar.cpp` (38): the signal radar (B): `updateRadar`, `radarLock`, `darkRadio`; `radar_camera.cpp` (17): its picture
- `telescope.cpp` (19): the telescope (Z): `telescopeToggle`, `updateTelescope`, `renderTelescopeHUD`
- `probe.cpp` (25): the giant probe (PROBE): `launchProbe`, `updateProbe`; `probe_feed.cpp` (28): its camera;
  `probe_records.cpp` (31): its track and profile, the recordings (RECORDING)
- `drones.cpp` (30) + `drone_draw.cpp` (28): the drone bay (DRONES): `droneBayLaunch`, `updateDrones`; the drones drawn
- `shards_screen.cpp` (57): the shard decoder (SHARDS): `openShards`, `updateShards`, `renderShards`, `buildShardWorld`;
  `history_screen.cpp` (21): its Tab pages; `chart_draw.cpp` (61): star-chart records; `medium_draw.cpp` (15): a medium
- `player.cpp` (17): the ship's player; `instruments_screen.cpp` (19) + `instrument_draw.cpp` (18): the case (INSTRUMENTS)
- `almanac.cpp` (8): the almanac (Ctrl+T): `buildAlmanac`, `updateAlmanac`, `almanacRunTo`
- `autoexplore.cpp` (12): auto-explore (AUTO): `autoStart`, the mind, the chooser; its `autoPlan*` steps in
  `autoexplore_space.cpp` (29) and `autoexplore_ground.cpp` (29); `autoexplore_hands.cpp` (27): `autoHands`
- `tape.cpp` (12) + `audio_deck.cpp` (14): auto-explore's music tape (M) and its cassette deck
- `audio.h/.cpp` (64): `AudioState`; `AudioSynth::render`: the mix, speech, the probe, the ground; `audio_radar.cpp` (45):
  the radar's receiver; `audio_music.cpp` (28), `audio_medium.cpp` (10), `audio_rogue.cpp` (33): pieces and instruments, a
  record's medium, drifters' voices
- `pad.cpp` (52): the controller: `padFrame`, the command menu (`padItems`), the CONTROLS page
- `persistence.cpp` (22): the save file (`save`, `load`), `saveSlot`, autosave, the SLOTS screen
- `settings.h/.cpp` (8): `Settings`, vesperis_settings.txt; `ui.h/.cpp` (5): `fmt`, `upper`, `wrapText`, the HUD colours

## The frame (`Game::frame`, game.cpp)
`padFrame` → auto-explore's input → `tapeUpdate` → `handleGlobalKeys` (not while `ST_TYPING`) → screens' sounds stopped off
their states, the cameras off on `ST_STOWS` → `noteVisit` → the clock (`ST_SIMULATES`: time warp, the almanac) → autosave →
`updateProbe`, `updateDrones` → the update `switch (state)` → `probeAudio`, `darkRadio`, `updatePlayer` → the render `switch`
(a scene's case; `default`, the `ST_OVERLAY` states, draws `returnState`'s scene first) → `autoRender` or `renderPad`.

## A new state or screen touches
- `GameState` (append only) and its row in `STATE_ROWS`: its traits (the clock, an overlay, the cameras stowed, the dark radio
  silent, a save's place, the mouse, the photo tools, typing, the ground), its controller context, its name; `STATE_COUNT`
  names the last state
- its file of `Game` methods, declared in game.h under a banner naming the file; the open function sets `state`,
  `returnState` and `guideReturn`
- the update switch; a scene's case in the render switch (an overlay is drawn in the `default`'s if/else chain)
- the sounds stopped at the frame's start if it plays one; pad.cpp's screen names and `padItems`; `autoScene`
- its way in: a cabin `facing` spot (a `CabinFacing`: `updateCabin`'s spots, its colour in `SCREENS`), a flight computer row (`PAGE1`/`PAGE2`
  appended, `counts[]` and `n` both), the Esc menu (`% 8` and `items[]`) or the guide menu (`GUIDE_ITEMS`, the shifted
  `case` numbers, 17 rows at 9 px)
- in the harness: a `screens` capture, a TestGame hook if a check needs one

## Rules and traps
- Persisted enums are append-only: `GameState`, `ShipState::Mode`, `AutoProfile`; mark numbers are fixed (guide.h:54).
  One save line per field; `load` ignores unknown keys. `Game`'s data members are never reordered.
- A state's traits are its row: code asks `stateHas(state, ST_...)`, and a trait new code needs is a column, not a list of
  states; unit `states` holds the first 29 rows to the lists they replaced.
- A split subsystem's files share helpers through its `_internal.h` (`inline`); a helper one file uses stays in that file.
- `radar`, `icase`, `chartHeld`, `probe`, `bay` and `tele` are reset by assigning a fresh one (`resetExpedition`): a field moved
  into one gains a reset, and a thread in one would end the game.
- A screen's helpers (ui.h): `drawPanel` (its box and edge), `menuStep` (Up and Down round a list), `refuse` (the status line and
  `BEEP_ERROR`); a beep is a `Beep`, a text entry's kind a `TextKind`.
- A text entry opened from a screen sets `guideReturn` to it; the screen's own Esc names its return state; TEXT_ENTRY's render
  draws the screen under the box when `guideReturn` is that screen.
- Sound: the synth reads `AudioState` pointers on the main thread after `frame` (`platformAudioPump`). Their owner clears them
  before what they point at can move (a world reload); `frame` clears them off the screen's state. The harness never runs the
  synth, so a screen keeps its own clock (`pieceT`) and re-syncs to the synth's (`pieceBeat`). Set a new voice's levels from a
  rendered WAV first.
- A world's map is asked for (`mapIfReady`), never waited for; the landing map's zoom never computes tiles.
- `drawCabinBand` runs per row band on the pool: nothing in it writes what another band reads.
- The UI: terse; nothing is named until the explorer names it; maps show only marks; 52 characters a line. The HUD's colours at
  1x are green (120, 230, 150), white (230, 235, 240) and amber (240, 190, 90): count them to judge an underline.
- An instrument to "look through" is a camera mode (a feed over the frame, the cabin gone), not a HUD overlay. A mystery signal's
  readout withholds what sent it: the sound carries it.

More, with the numbers: `docs/reference/lessons.md`, "Game, screens and sound".
