# src/game: the game

`Game` (game.h, 129 KB: read by range) owns the states, the ship, the guide and the audio state; each screen or device is a
file of `Game` methods. Sizes are in KB.

## Files
- `game.h` + `game.cpp` (50): `GameState`, `class Game`; `newGame`, `frame`, `handleGlobalKeys`
- `game_states.cpp` (55): `updateSurface`, the descent; the title, help, menu, settings, keys and DATA screens
- `autopilot.cpp` (28): SPACE's input and flight: `updateSpace`, `plotJump`, `startApproach`, parking
- `cabin.cpp` (45): the cabin (`updateCabin`, `drawCabin`), the flight computer (SHIPSCREEN), the CONSOLE
- `hud.cpp` (66): `renderSpace`, `renderSurfaceScene`, the space and surface HUDs, SECTOR_MAP
- `guide.h/.cpp` (26): `Guide` (guide.txt): names, the log, marks, shards, probes
- `guide_screens.cpp` (48): `starNameOf`; GUIDE, TEXT_ENTRY, STAR_MAP, LOG, STATS, GALLERY
- `galaxy_chart.cpp` (46): the star map's galaxy page: `renderGalaxyChart`, `chartEnter`, marks
- `landing_map.cpp` (63): `beginLanding`, `updateLandingMap`, `renderLandingMap`
- `marks.cpp` (21): surface marks and the scanner (R): `beginMark`, `scanCycle`, `drawScanHUD`
- `radar.cpp` (59): the signal radar's camera (B): `updateRadar`, `radarLock`, `radarStartContent`, `darkRadio`
- `telescope.cpp` (23): the telescope (Z): `telescopeToggle`, `updateTelescope`, `renderTelescopeHUD`
- `probe.cpp` (92): the giant probe (PROBE) and its recordings (RECORDING): `launchProbe`, `updateProbe`
- `drones.cpp` (33) + `drone_draw.cpp` (28): the drone bay (DRONES): `droneBayLaunch`, `updateDrones`; the drones drawn
- `shards_screen.cpp` (59): the shard decoder (SHARDS): `openShards`, `updateShards`, `renderShards`, `buildShardWorld`
- `history_screen.cpp` (23): the decoder's Tab pages: `renderShardTimeline`, `renderShardHistory`
- `player.cpp` (18): the ship's player: `playerPlay`, `updatePlayer`
- `chart_draw.cpp` (64): star-chart records: `renderChart`, `drawChartOverlay`
- `medium_draw.cpp` (16): a record's medium named and drawn: `mediumLine`, `drawMediumPanel`
- `instruments_screen.cpp` (20) + `instrument_draw.cpp` (19): the case (INSTRUMENTS); an instrument drawn from its parts
- `almanac.cpp` (9): the almanac (Ctrl+T): `buildAlmanac`, `updateAlmanac`, `almanacRunTo`
- `autoexplore.cpp` (101): auto-explore (AUTO): `autoStart`, the `autoPlan*` steps, `autoHands`
- `tape.cpp` (13) + `audio_deck.cpp` (14): auto-explore's music tape (M) and its cassette deck
- `audio.h/.cpp` (112): `AudioState`; `AudioSynth::render`: the mix, speech, radar, probe
- `audio_music.cpp` (28), `audio_medium.cpp` (10), `audio_rogue.cpp` (34): pieces and instruments, a record's medium, drifters' voices
- `pad.cpp` (54): the controller: `padFrame`, the command menu (`padItems`), the CONTROLS page
- `persistence.cpp` (22): the save file (`save`, `load`), `saveSlot`, autosave, the SLOTS screen
- `settings.h/.cpp` (8): `Settings`, vesperis_settings.txt; `ui.h/.cpp` (5): `fmt`, `upper`, `wrapText`, the HUD colours

## The frame (`Game::frame`, game.cpp:267)
`padFrame` → auto-explore's input → `tapeUpdate` → `handleGlobalKeys` → screens' sounds stopped off their states → `noteVisit` →
the clock (`simulating`, time warp, the almanac) → autosave → `updateProbe`, `updateDrones` → the update `switch (state)` →
`probeAudio`, `darkRadio`, `updatePlayer` → the render `switch` (overlays draw `returnState`'s scene first) → `autoRender` or
`renderPad`.

## A new state or screen touches
`GameState` (append only), the open function (sets `state`, `returnState`, `guideReturn`), the update switch, the render switch
(the overlay group and the if/else chain), `simulating` if time runs, the stops at game.cpp:278-282 and `mouseCaptureWanted`,
`darkRadio`'s list (radar.cpp:84), pad.cpp's context, names and `padItems`, `autoScene` (autoexplore.cpp:119), the save's
`inPlay`/`onSurface` (persistence.cpp:170), and its way in: a cabin `facing` spot (cabin.cpp:23-128), a flight computer row
(`PAGE1`/`PAGE2` appended, `counts[]` and `n` both), the Esc menu (`% 8` and `items[]`) or the guide menu (`GUIDE_ITEMS`, the
shifted `case` numbers, 17 rows at 9 px). In the harness: the `STATES[]` names, fuzz's `states[16]`, a `screens` capture, a test hook.

## Rules and traps
- Persisted enums are append-only: `GameState`, `ShipState::Mode`, `AutoProfile`; mark numbers are fixed (guide.h:54).
  One save line per field; `load` ignores unknown keys.
- `radar`, `icase`, `chartHeld`, `probe`, `bay` and `tele` are reset by assigning a fresh one (`newGame`, `load`): a field moved
  into one gains a reset, and a thread in one would end the game.
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
