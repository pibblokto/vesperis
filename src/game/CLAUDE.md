# src/game: the game

`Game` owns the states, the ship, the guide and the audio state; a screen or device is a file of `Game` methods, declared in
game.h under a `// ----` banner naming that file (the harness's hooks: src/harness/test_game.h). Over 50 KB (read by
range): chart_draw, shards_screen.

## Files
- `game`: `newGame`, `frame` (its order: 07-game-flow-and-controls.md, "The frame"), `handleGlobalKeys`, `refuse`,
  `resetExpedition`. `states`: `Game::STATE_ROWS`, a row a state. `game_types.h`: `GameState` and its traits, `CabinFacing`,
  `TextKind`, the rows of the ways in (`MenuRow`, `ShipRow`, `SettingRow`, `CabinDevice`)
- A subsystem's state, a `Game` member: `landing_state`, `pad_state`, `chart_state`, `shard_world`, `radar_state`,
  `player_state`, `case_state`, `drone_state`, `telescope_state`, `probe_state`, `autoexplore_state`, `tape_state`; a split
  one's shared helpers: `autoexplore_internal`, `probe_internal`, `radar_internal`, `audio_internal`
- Flight, the scenes: `autopilot` (SPACE), `scene_draw`, `hud`, `surface_play` (the capsule, the ground), `sector_map`
- Screens: `cabin` (with SHIPSCREEN, CONSOLE), `menus`, `data_sheet`, `guide_screens` (GUIDE, TEXT_ENTRY, LOG...),
  `star_map`, `galaxy_chart`, `landing_map` + `landing_map_build`, `persistence` (saves, SLOTS), `pad` (the controller)
- The guide: `guide` (guide.txt), `names` (`starNameOf`, `bodyNameOf`, `logEvent`, `noteVisit`), `marks` (the scanner)
- Devices: `radar` + `radar_camera`, `telescope`, `probe` + `probe_feed` + `probe_records`, `drones` + `drone_draw`, the
  decoder `shards_screen` + `history_screen` + `chart_draw` + `medium_draw`, `player`, the case `instruments_screen` +
  `instrument_draw`, `almanac`, `autoexplore` + `autoexplore_space` + `autoexplore_ground` + `autoexplore_hands`, `tape` +
  `audio_deck`
- Sound: `audio` (`AudioState`, the synth), `audio_radar`, `audio_music`, `audio_medium`, `audio_rogue`. `settings`; `ui`
  (`drawPanel`, `menuStep`, `wrapText`, the HUD's colours)

## Rules and traps
- A new screen: the cookbook's "Add a screen" names its five places; a new state goes before `GameState::COUNT`.
- Persisted enums are append-only (`GameState`, `ShipState::Mode`, `AutoProfile`, guide.h's `MARK_*` numbers); one save line
  per field, and `load` ignores unknown keys. `Game`'s data members are never reordered.
- A state is its row (its `padMenu`, `enter`, `back`), a way in a row of its list, a cabin device its row in `cabinDevices`
  (rules-game-screens.md): ask `stateHas(state, ST_...)`; a trait new code needs is a column. A screen opens by
  `openScreen(S)` and closes by `closeScreen()`; `state =` is only a landing's, the title's, a load's, an action's own exit.
- `resetExpedition` resets `radar`, `icase`, `chartHeld`, `probe`, `bay` and `tele` by assigning fresh ones: a field moved into
  one is reset with it, and a thread in one would end the game.
- A text entry draws over its opener (`openedFrom`) and goes back to it; the guide closes before it opens one.
- Where the game is (a save's place, a photo's caption) is `placeNow()`, never `state` or `returnState` alone: a screen's
  scene is the base it opened from, the title's game the place it was opened from (B-431, B-432).
- The synth reads `AudioState`'s pointers after `frame`: their owner clears them before what they point at can move. The
  harness never runs the synth, so a screen keeps its own clock (`pieceT`) and re-syncs to the synth's.
- A world's map is asked for (`mapIfReady`), never awaited. `drawCabinBand` runs a band a job: none reads another's.

More (the HUD's colours, sound): `docs/reference/lessons.md`, "Game, screens and sound".
