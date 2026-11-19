# src/game: the game

`Game` owns the states, the ship, the guide and the audio state; a screen or device is a file of `Game` methods, declared in
game.h under a `// ----` banner naming that file (the harness's hooks: src/harness/test_game.h). Past 70 KB of game.h, `Game`'s
shape is revisited (the user's mark since 2026-11-19, PLAN-screens.md's 60 KB before it: KI-445). Over 50 KB (read by range):
cabin, chart_draw, game.h, shards_screen; ship_plan is 125 KB, refused whole by the guard.

## Files
- `game`: `newGame`, `frame` (its order: 07-game-flow-and-controls.md, "The frame"), `handleGlobalKeys`, `refuse`,
  `resetExpedition`. `states`: `Game::STATE_ROWS`, a row a state. `game_types.h`: `GameState` and its traits, `CabinFacing`,
  `TextKind`, the rows of the ways in (`MenuRow`, `ShipRow`, `SettingRow`, `CabinDevice`)
- A subsystem's state, a `Game` member: `landing_state`, `pad_state`, `chart_state`, `shard_world`, `radar_state`,
  `player_state`, `case_state`, `drone_state`, `telescope_state`, `probe_state`, `autoexplore_state`, `tape_state`,
  `aboard_state` (I-04: the prints, the garden; R-428: the wall as hung; R-429: the beds' growth), `search_state` (Z-03: the vials), `lab_state` (Z-04: the microscope's; Z-05: the flasks, the jars, the lab's clock); a split
  one's shared helpers: `autoexplore_internal`, `probe_internal`, `radar_internal`, `audio_internal`
- Flight, the scenes: `autopilot` (SPACE), `scene_draw`, `hud`, `surface_play` (the capsule, the ground; R-434: Shift+O the activity
  vision, `VISION_ACTIVITY`), `sector_map`; `search` (Z-03: the kit's vials, the scoop, the cold store (R-432: any number), their
  save lines)
- The laboratory (Z-04): `microscope` (the state `MICROSCOPE`: the vials' list, a vial on the stage, the view, the four things done,
  tracking, what is seen (`microscopeSee`: the guide's `microbe` lines), the sheet's lines, its pad) and `microscope_draw` (the slide in
  the framebuffer, its HUD, the list, the sheet); `cabin_life`'s `drawStore` lights the cold store's door. Z-05: `incubator` (the state
  `INCUBATOR`: the flasks, what goes in them, the medium's controls, the lab's clock (`updateLab`, the frame's beside the drones'), the
  jars, the save's lines through its row's hooks) and `incubator_draw` (its screen: the flasks, a curve, the jars, the chooser);
  `cabin_life`'s `drawIncubator` and `drawJars`; the microscope's list takes the flasks (a drop of a culture on the stage). Z-06: `sequencer`
  (the state `SEQUENCER`: a species read in SEQUENCE_S on the lab's clock, its end told as the clock reaches it (`sequencerTick` in
  `updateLab`), the sheet's kin, the log's KIN, a made species' watermark to the decoder; its `seq` line) and `tree_screen` (the state `TREE`:
  every species read and the ship's own by kinship, Enter a kin's star the remote target); `cabin_life`'s `drawTreeWall`; the decoder's
  watermarks (`shardWatermarkAdd`). Z-07: `catalogue` (the state `CATALOGUE`, the guide's row: every species with a sheet and a friend's
  lent, by world or by kin, a sheet beside its form drawn live; `guide_screens`' LIFE page on the statistics (`renderStatsLife`)).
  `lab_lists` (R-432, R-433): the lists in folders (the vials a folder a world, the cultures' folders, the choosers' folders and their
  keys and rows) and the culture collection (every species and strain a flask grows, kept: `keepCulture` from `updateLab`; its folders;
  a kept culture on the stage; its `keptfolder` and `kept` lines through the incubator's)
- The ship: `ship_plan` (I-01's rows; I-02's three decks and eight spaces: rooms, ways (the arches, the stair's landings, the lift),
  windows, fixtures, lamps and views; the mesh built once from them; the walk and the stair; the routes; the sight; the banks'
  dealing; I-03's finishes, the lamps' light on the vertices, the sun's patches, the hull outside; I-04's explorer's things),
  drawn and walked by `cabin`; `cabin_life` (I-04) fills the explorer's things (the photographs' prints, the shelves' shards, the
  case's instruments, the deck, the garden (R-429: a bed trimmed by E), the core, the capsule and its drop); `wall_screen` (R-428:
  the state `WALL`, the quarters' wall hung by the explorer); `vehicles_draw` (R-427: the buggy and the drone on their platforms
  in the maintenance room, the ground's shapes in the bay drones' primitives)
- Screens: `cabin` (with SHIPSCREEN, CONSOLE), `menus`, `data_sheet`, `guide_screens` (GUIDE, TEXT_ENTRY, LOG...),
  `star_map`, `galaxy_chart`, `landing_map` + `landing_map_build`, `persistence` (saves, SLOTS), `pad` (the controller)
- The guide: `guide` (guide.txt), `names` (`starNameOf`, `bodyNameOf`, `logEvent`, `noteVisit`), `marks` (the scanner)
- Devices: `radar` + `radar_camera`, `telescope`, `probe` + `probe_feed` + `probe_records`, `drones` + `drone_draw`, the
  decoder `shards_screen` + `history_screen` + `chart_draw` + `medium_draw`, `player` (R-431: the state `PLAYER` at the deck, a
  folder a world, music alone), the case `instruments_screen` +
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
- A world's map is asked for (`mapIfReady`), never awaited. The ship's frame takes its sight first (`cabinSight`, before the
  space view, which it skips with no window in view), projects the seen rooms' pieces once, each room clipped to its box
  (`drawCabinPieces` into `cabinFrame.shapes`, the rack's by `drawDroneRack`), then each band fills them: none reads another's.
  Its banks are dealt once the outside is drawn (`cabinDeal`; B-437: the outside in the glass's box sunk into bank 7, read through its
  own palette, the pool every bank but 0, 7, 8 and 10): draw a device's screen in its dealt bank, never its home number, nothing in 7. I-03: the outside first
  (its box the glass in view, so the panes keep it), a room's sun patches after its pieces (`BLEND_ADD` on their plane; B-434: cast onto
  the surfaces' own pieces round their holes, `Builder::receives`, never a whole wall: a patch over an arch shows from beyond), the daylight
  taken once a frame (`cabinDaylight`); a piece's light is its vertices' lamps' x U's level, so a ramp never carries the light.
  I-04: a piece the frame fills (PIECE_SHELF and on) is drawn where it comes, lit by `shipLampLight` as the vertices are; the
  prints come from their thread (`printsCollect`), never awaited; the hatch's drop is the frame's eye (`cabinSeen`), the
  explorer's place kept.
- A place in the ship is the plan's (a view, a device's stand, a fixture's row), never a number in code; drawing moved into the
  rows keeps each expression's shape (lessons.md, "Proving and timing").
- The incubator's lines are its state row's (`save`, `load`, `reset`); a flask's culture is made again from its species and events
  (`flaskMake`: its kinds read from their worlds) whenever they change or after a load, never saved. The lab's tones (the flasks', the
  jars') are homes past the banks (32-47): ask them only for what is drawn, in front of the eye, after the screens near them.
- The sequencer's line is its state row's (`seq`); what is on it is made again (`sequencerMake`: its lineage, a made one's watermark), never
  saved. A sequenced species' lineage is kept by key (`tree.known`, `lineageOfKey`): its world drawn once a game. A made species' watermark
  sits after its people's records in the decoder (`ShardWorld::watermark`): the language's count is `recordsHeld()` (it teaches no word), the
  list's `shardHeld.size()`; the history page never reads it (it tells no event).
- The catalogue draws a sheet's form in a framebuffer of its own (`cat.fb`: every bank its way's), its middle copied into the canvas; it
  reads the sheets through `catalogueSheet` (the explorer's, else a friend's lent: `guide.lentMicrobes`, cyan, in no count) and a sheet's
  lines through `sheetLinesOf` (the microscope's). Every count of the living small is the explorer's own and the other screens' (the LIFE
  page's geneses the tree's: it builds the tree first). A sample is counted and logged where it is taken (`sampleTaken`).
- The microscope's slide is made again from its vial (`microscopeStage`), a flask's culture (`microscopeStageFlask`) or a kept culture
  (`microscopeStageKept`, R-433); each fills `scope.species`, `keys` and `worlds`, which the sheet and the record read. Its lists' folders
  are made from the cold store and the lab each frame (`vialFolders`, `cultureFolders`): a row is a vial's, a flask's or a kept culture's
  index by its folder's id (FOLDER_WORLD, FOLDER_FLASKS, a collection's), never stored. Its frame's specimens are made in the update
  (`microscopeFrame`) and only drawn by the render (a text entry over it draws the last ones). Its clock is the slide's own (real
  seconds on its page, stopped under the electron); the harness skips it (`testScopeSkip`) or sets the view (`testScopeView`).

More (the HUD's colours, sound): `docs/reference/lessons.md`, "Game, screens and sound".
