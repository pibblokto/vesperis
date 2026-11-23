# src/game: the game

`Game` owns the states, the ship, the guide and the audio state. A screen or device is a file of `Game` methods, declared in game.h
under a `// ----` banner naming that file (the harness's hooks: src/harness/test_game.h). Past 70 KB of game.h, `Game`'s shape is
revisited (the user's threshold since 2026-11-19, and PLAN-screens.md's 60 KB before it: KI-445). Over 50 KB (read by range): cabin,
chart_draw, game.h, shards_screen. ship_plan is 125 KB, refused whole by the guard.

## Files
- `game`: `newGame`, `frame` (its order: 07-game-flow-and-controls.md, "The frame"), `handleGlobalKeys`, `refuse`, `resetExpedition`.
  `states`: `Game::STATE_ROWS`, a row per state. `game_types.h`: `GameState` and its traits, `CabinFacing`, `TextKind`, the rows of the
  ways in (`MenuRow`, `ShipRow`, `SettingRow`, `CabinDevice`)
- Each subsystem's state is a `Game` member: `landing_state`, `pad_state`, `chart_state`, `shard_world`, `radar_state`, `player_state`,
  `case_state`, `drone_state`, `telescope_state`, `probe_state`, `autoexplore_state`, `tape_state`, `aboard_state` (I-04: prints,
  garden; R-428: the wall as hung; R-429: the beds' growth), `search_state` (Z-03: vials), `lab_state` (Z-04: the microscope; Z-05:
  flasks, the lab clock). Shared helpers of split subsystems: `autoexplore_internal`, `probe_internal`, `radar_internal`,
  `audio_internal`
- Flight and scenes: `autopilot` (SPACE), `scene_draw`, `hud`, `surface_play` (the capsule, the ground; R-434: Shift+O for the activity
  vision, `VISION_ACTIVITY`; J-07: the animals' record, `animalsSeen`, `animalKey`, `animalNameOf`, and N naming the one aimed at;
  J-08: the record's marks and REMAINS OF, the dusk insects quiet in silence; J-10: `lifeSoundsTo`, nothing living heard in silence,
  a squelch on a wet cover, an overgrowth's sounds in `groundSoundsTo`),
  `sector_map`, `search` (Z-03: the kit's vials, the scoop, the cold store (any number since R-432), their save lines)
- The laboratory (Z-04): `microscope` is the state `MICROSCOPE`: the vial list, a vial on the stage, the view, the four things done to a
  slide, tracking, what is seen (`microscopeSee`: the guide saves it as `microbe` lines), the sheet's lines, its pad. `microscope_draw`
  draws the slide in the framebuffer, its HUD, the list and the sheet. `cabin_life`'s `drawStore` lights the cold store's door. Z-05:
  `incubator` is the state `INCUBATOR`: flasks and what goes in them, the medium's controls, the lab clock (`updateLab`, in the frame
  beside the drones' update), and save lines through its state row's hooks. `incubator_draw` draws its screen: flasks, a curve, the
  chooser. `cabin_life` has `drawIncubator` (B-459: a flask past the ship's banks in the nearest dealt colour's, `ShipBanks::bankNear`).
  The microscope's list takes flasks too (a drop of a culture on the
  stage). Z-06: `sequencer` is the state `SEQUENCER`: it reads a species in SEQUENCE_S on the lab clock and tells the end when the clock
  reaches it (`sequencerTick` in `updateLab`). It also has the sheet's kin, the log's KIN, a made species' watermark sent to the
  decoder, and its `seq` line. `tree_screen` is the state `TREE` (Y-01), a tree per page: the ship's own, each world with a species
  read, each folder holding a culture. Lab lines are twigs: Backspace takes one off, A puts it back (`treeoff`). Enter on a kin's world
  makes its star the remote target. `cabin_life` has `drawTreeWall`. The decoder takes watermarks through `shardWatermarkAdd`. Z-07:
  `catalogue` is the state `CATALOGUE`, a row of the guide: every species with a sheet, and those on sheets a friend lent, by world or
  by kin, with a sheet beside its form drawn live. J-12: by world, a seeded world's head names its made kind, its natives a line drove out
  read DIED OUT, the lines found wild there follow (`CR_WILD`), then any world's animals seen (`CR_ANIMAL`, a sheet in words). The
  statistics' LIFE page is in `guide_screens` (`renderStatsLife`; J-12: the seeded worlds, `catalogueCountsMake`). `lab_lists`
  (R-432, R-433) has the lists in folders: vials in a folder per world, culture folders, the choosers' folders with their keys and rows
  (R-441: the choosers' two tabs, `chooserTabs`; B-442: rows that look alike told apart, `apartRows`). It also has the culture
  collection: every species and strain a flask grows is kept (`keepCulture` from `updateLab`, keeping B-441's pair together, once).
  R-440 adds keeping by hand: `keepOpen`, `keepKeys`, `keptAdd`, a copy into a folder from K at the microscope or a flask, and C in the
  lists. Also the collection's folders, a kept culture on the stage, and its `keptfolder` and `kept` lines, saved through the
  incubator's. Y-02: `editor` is the state `EDITOR`, opened with E at the bow bench past the sequencer. A culture the sequencer has read
  goes on, its genes on the genesis map. A gene's letters are changed or the gene cut, and the line is written in EDIT_S on the lab
  clock into an empty flask (`editorTick` in `updateLab`). The editor also has words for lines (`lineTag`: S a strain, E an edited line;
  `lineMoveWords`), learning what a gene does (`geneSeen` from `flaskLearn`, `scopeLearn`, `scopeAnswerLearn`; `strainReadLearn` from
  the sequencer), and its `editor` lines. Y-03: K puts a kin beside the culture (`editorKinPut`), M moves the kin's gene in
  (`editorMove`), D makes a copy (`editorCopy`), and the editor says what a gene needs (`geneNeedSay`). `geneDonor` and `geneFar` are in
  `incubator`. Y-04: S sets a gene's switch (`editorSwitch`), and `flaskLineIn` gives a flask's line in a state. R-445: W opens the
  keep chooser for the new strain's folder (`KeepPick::write`, the parent's folder first), then a name entry (`TEXT_LINE`, the parent's
  name and the new strain's number typed in); `editorWriteBegin` starts the writing, and `editorTick` names the strain and keeps it in
  that folder. The save keeps both in the `editorwriting` line. J-15: a cut STOP's line says what the line overgrows as and whether it
  greens a desert (`overgrowthTypeOf`). J-01: `flaskShow` (`incubator`) says what a flask's world genes show
  (bubbles, a dye, a scum, slime, a sediment, clumps, an overgrowth, ice, a poisoned or a gene-taking other); the screen's words and
  picture (`renderFlaskPicture`) and the room's flasks (`flaskLook`, homes 49 to 62) read it, and `flaskLearn` learns from it. The
  editor's map is on two pages (Page Up and Page Down; the arrows walk across both).
- The ship: `ship_plan` has I-01's rows. I-02: three decks and eight spaces (rooms, ways (the arches, the stair's landings, the lift),
  windows, fixtures, lamps and views), the mesh built once from them, the walk and the stair, the routes, the sight, dealing the banks.
  I-03: finishes, lamplight on the vertices, sun patches, the hull outside. I-04: the explorer's things. `cabin` draws and walks the
  plan. `cabin_life` (I-04) fills in the explorer's things: the photographs' prints, the shelves' shards, the case's instruments, the
  deck, the garden (R-429: E trims a bed), the core, the capsule and its drop. `wall_screen` (R-428): the state `WALL`, the quarters'
  wall as the explorer hung it. `vehicles_draw` (R-427): the buggy and the drone on their platforms in the maintenance room, each in its
  ground shape, built from the bay drones' primitives.
- Screens: `cabin` (with SHIPSCREEN, CONSOLE), `menus`, `data_sheet` (J-11: a made kind's line, "FLESH WORLD, ONCE FELISIAN", and
  sentence), `guide_screens` (GUIDE, TEXT_ENTRY, LOG...), `star_map`,
  `galaxy_chart`, `landing_map` + `landing_map_build`, `persistence` (saves, SLOTS), `pad` (the controller)
- The guide: `guide` (guide.txt), `names` (`starNameOf`, `bodyNameOf`, `logEvent`, `noteVisit`; B-447: `kindNameNow`, `kindShortNow`,
  `kindWordsNow`, a body's kind as it is now, its made kind's where it took one), `marks` (the scanner)
- Devices: `radar` + `radar_camera`, `telescope`, `probe` + `probe_feed` + `probe_records`, `drones` + `drone_draw` + `seed_trip`
  (J-03: S on the bay, the state `SEEDING`: a culture alive in a flask, a place on the world's map, drone 1 sent; the culture let go at
  its place, the verdict its report; a world's seedings worked out once, `seedingsOf`; a vial's lines, `vialLines`; J-13: Page Down and
  Page Up its depth down a drill hole, `seedPageDepthKm`, the bay's `seedDepth`; J-14: Page Up on up into a cloud deck, a negative
  `pageDepth` and `seedDepth`), the decoder
  `shards_screen` + `history_screen` + `chart_draw` + `medium_draw`, `player` (R-431: the state `PLAYER` at the deck, a folder per
  world, music only), the case `instruments_screen` + `instrument_draw`, `almanac`, `autoexplore` + `autoexplore_space` +
  `autoexplore_ground` + `autoexplore_hands`, `tape` + `audio_deck`
- Sound: `audio` (`AudioState`, the synth; J-10: an overgrowth's voices, `growthSample`), `audio_radar`, `audio_music`, `audio_medium`,
  `audio_rogue`. `settings`, and `ui`
  (`drawPanel`, `menuStep`, `wrapText`, the HUD's colours)

## Rules and traps
- A new screen: the cookbook's "Add a screen" names its five places. A new state goes before `GameState::COUNT`.
- Persisted enums are append-only (`GameState`, `ShipState::Mode`, `AutoProfile`, guide.h's `MARK_*` numbers). One save line per field,
  and `load` ignores unknown keys. `Game`'s data members are never reordered.
- A state is a row (`padMenu`, `enter`, `back`), a way in is a row of a list, and a cabin device is a row in `cabinDevices`
  (rules-game-screens.md). Ask `stateHas(state, ST_...)`, and give a trait new code needs a column. A screen opens by `openScreen(S)`
  and closes by `closeScreen()`. Only a landing, the title, a load or an action's own exit sets `state =`.
- `resetExpedition` resets `radar`, `icase`, `chartHeld`, `probe`, `bay` and `tele` by assigning fresh ones: a field moved into one is
  reset with it, and a thread in one would end the game.
- A text entry draws over its opener (`openedFrom`) and goes back to it. The guide closes before it opens one.
- Where the game is (a save's place, a photo's caption) is `placeNow()`, never `state` or `returnState` alone: a screen's scene is the
  base it opened from, and the title's game is the place the title was opened from (B-431, B-432).
- The synth reads `AudioState`'s pointers after `frame`: their owner clears them before what they point at can move. The harness never
  runs the synth, so a screen keeps its own clock (`pieceT`) and re-syncs to the synth's.
- A world's map is asked for (`mapIfReady`), never awaited. The ship's frame takes its sight first (`cabinSight`, before the space view,
  which it skips with no window in view). It projects the seen rooms' pieces once, each room clipped to its box (`drawCabinPieces` into
  `cabinFrame.shapes`, the rack's by `drawDroneRack`), then each band fills them, and no band reads another's. Banks are dealt once the
  outside is drawn (`cabinDeal`; B-437: the outside in the glass's box is sunk into bank 7 and read through its own palette, and the
  pool is every bank but 0, 7, 8 and 10). Draw a device's screen in its dealt bank, never its home number, and nothing in 7. I-03: the
  outside first (its box is the glass in view, so the panes keep it), a room's sun patches after its pieces (`BLEND_ADD` on their plane;
  B-434: cast onto the surfaces' own pieces round their holes, `Builder::receives`, never a whole wall: a patch over an arch shows from
  beyond). Daylight is taken once a frame (`cabinDaylight`). A piece's light is the lamps at its vertices times the cabin light level
  (U), so a ramp never carries the light. I-04: a piece the frame fills (PIECE_SHELF and on) is drawn where it comes, lit by
  `shipLampLight` as the vertices are. The prints come from their thread (`printsCollect`), never awaited. During the hatch's drop the
  frame sees through the drop's camera (`cabinSeen`), and the explorer's place is kept.
- A place in the ship is the plan's (a view, a device's stand, a fixture's row), never a number in code. Drawing moved into the rows
  keeps each expression's shape (lessons.md, "Proving and timing").
- In the lab's rows a species with no name is shown by its look (`flaskSpeciesWords`, B-442), never as UNNAMED alone, or two of a vial
  would read alike. The same goes for the tree (`treeLeaves`, B-443), and the sequencer names a species in the tree's words
  (`treeWords`). K's folder chooser (`keep`) is the game's and opens over the microscope's slide or the incubator's page. Each of the
  two closes it on entering.
- J-03: a world's seedings are worked out once by its key (`seedingsAt`, the line read from its key as a flask's) and made again only
  when the guide's `seeded` lines change; every frame's layer (`madeNow`) is arithmetic on them. The home world is the pinned home
  star's first felisian planet, never `guide.home` (the explorer's chosen home). A seed trip is drone 1 alone (the others stay docked):
  its culture is let go as the drone arrives (the `seeded` line, the verdict kept for its report), and a recall before that sends
  nothing. A vial keeps the guide's clock at its take (`vialclock`, a seeded guide's), so a line spread after it is not in it.
- The tree's `leaves` are every species read (used for the LIFE page's geneses and the sequencer's words). A page's rows are `shown`
  (`treePage`), made again whenever the page or what is on it changes (a line taken off or put back, a folder's culture). Y-01.
- The incubator's save lines go through its state row (`save`, `load`, `reset`). A flask's culture is made again from its species and
  events (`flaskMake` reads the kinds from their worlds) whenever they change or after a load, never saved. The flasks' tones are
  home numbers past the banks (32-47): ask for them only for what is drawn, in front of the eye, after the screens near them.
- An edited line is a strain of its species (recorded as SM_EDIT with its edits): every reader of strains reads it. `flaskKind` applies
  the lines from the wild kind in order (a strain's move, an edit's genes: `applyEdits` on the genes at that point). The parent's kind
  and form are made with the flask (`Flask::parentKind`): the line is compared with them as it grows and when its drop is looked at. A
  strain arisen in the flask since ends the comparison. The line's answers on the slide go to no sheet (they would put a line's
  behaviour on its species' sheet). Y-02. Y-03: a moved gene's donor is kept by key and read again wherever the line is made
  (`geneDonor` from `flaskKind`, a few levels deep), and the line's price (`CultureKind::burden`) is set there from the wild kind. A
  filled gap is among a flask's changed genes (`changedKind`: their kinds). Y-04: switches are priced there too. A switched line (and
  its parent) is read in the flask's state (`flaskLineIn`, with the state in `CultureOne::state`), both for its drop (kept as
  `MicroscopeState::lineState`) and for learning. A gene whose only change is its switch, and which is on in that state, is a candidate
  for nothing (`Flask::switchOnly`). The incubator's DOUBLING IN uses the segment's rate (`CultureOne::mu`), and the rest the old
  `growthIn`.
- The editor's letters are set with the arrows, never typed: P (a photograph) and T (the warp) are the game's own in a screen that
  simulates. The editor's commands C, D, K, M, N, S and W are none of those, Ctrl+K is the scanlines' alone, and S walks back only
  aboard, not in a screen. A line said on the status is 51 characters at most (`UW - 12`): gene findings wait their turn
  (`EditorState::said`, `geneSay` in `updateLab`), what was seen and then what it says (`statusNext`), each logged at once (GENE). Y-02.
- The sequencer's save line goes through its state row (`seq`, with a line's number after it since Y-02). What is on the sequencer is
  made again (`sequencerMake`: the lineage, a made species' watermark), never saved. A sequenced species' lineage and form are kept by
  key (`tree.known`, `tree.forms`: `lineageOfKey`, `formOfKey`), its world drawn once a game for both (`kinDraw`, B-443). A made
  species' watermark sits after its people's records in the decoder (`ShardWorld::watermark`): the language's count is `recordsHeld()`
  (the watermark teaches no word), the list's `shardHeld.size()`. The history page never reads it (it tells no event).
- The catalogue draws a sheet's form in a framebuffer of its own (`cat.fb`: every bank set for its way of seeing) and copies the middle
  into the canvas. It reads sheets through `catalogueSheet` (the explorer's, else one a friend lent: `guide.lentMicrobes`, cyan, in no
  count) and a sheet's lines through `sheetLinesOf` (the microscope's). Every count of microbes is the explorer's own and matches the
  other screens (the LIFE page's geneses are the tree's: it builds the tree first). A sample is counted and logged where it is taken
  (`sampleTaken`). J-12: a world's species and forms are drawn once (`catalogueWorld`, kept in `cat.worlds`); its made kind and the
  natives died out are made again with every list, at the guide's clock. A line found wild is the guide's `wildFound` record
  (`<world>/L<k>`: the world's k-th seeding), made by the microscope (`scope.wild`, a vial's line's key) and read by the sequencer
  (`wildKey`, the save's `seqwildkey`). `vialLines` drops the natives a line drove out (`seedingDiedOut`) from every vial of the world.
- The microscope's slide is made again from its vial (`microscopeStage`), a flask's culture (`microscopeStageFlask`) or a kept culture
  (`microscopeStageKept`, R-433; with its partner since B-441). Each fills `scope.species`, `keys` and `worlds`, which the sheet and the
  record read, and the drop's liquid range (`stageLo`, `stageHi`), which bounds the warmth typed after H (R-438), in digits as for a
  flask (R-435). The folders in its lists are made from the cold store and the lab each frame (`vialFolders`, `cultureFolders`): a row
  is the index of a vial, a flask or a kept culture by its folder's id (FOLDER_WORLD, FOLDER_FLASKS, a collection's), never stored. A
  frame's specimens are made in the update (`microscopeFrame`) and only drawn by the render (a text entry over it draws the last ones).
  The microscope's clock is the slide's own (real seconds on its page, stopped under the electron view). The harness skips it
  (`testScopeSkip`) or sets the view (`testScopeView`). R-437: what the kinds in view answer (`microscopeAnswers`, after
  `microscopeSee`) is the game's, not the slide's: told once a slide (`scope.told`), its lines queued (`scope.said`), the sheets'
  `answers` saved at most two seconds apart. A frame closed-form in the clock forgets them first (`testScopeFresh`).

More (the HUD's colours, sound): `docs/reference/lessons.md`, "Game, screens and sound".
