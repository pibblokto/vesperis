# Plan: a screen in one row (the U series)

Written 2026-10-26 on the user's request (R-424). E-13 had measured the game-screen task at fourteen places across the game and
the harness, 13% dearer than at E-01. Told what a screen-heavy feature costs, the user answered "That's sad, I will have tons of
those.", then "Define those milestones, add them and prepare everything so I can write proceed and you will".

How it was made:
- **Three readers surveyed the tree:** the controller (pad.cpp); the ways into the screens (the menus, the flight computer, the
  cabin, the openers, the return variables); and the battery's coverage of each state.
- **The frame and the state table** were read whole. What a step can break is listed with each milestone, from those readings.

**The short version.** A screen today is a `GameState`, its row of traits, and code in about fourteen places that nothing ties
together:
- **The frame** (`Game::frame`) dispatches by two `switch`es on the state; nine cases hold their screen's code inline.
- **The ways in** are six lists kept in step by index:
  - the Esc menu's labels, count and `case` numbers sit in two files;
  - the guide menu's actions are numbered cases, and two of its labels are replaced by index;
  - the flight computer keeps its rows' counts twice;
  - the settings and the slots keep their counts by hand;
  - the cabin's devices are spread over six places. Two of them misread a device without its entry: a null pointer in one,
    a read past the end in the other.
- **Opening and closing** go through 16 openers, five return variables (and a sixth for the almanac's scene), and 111
  assignments of the state.
- **The controller** names each screen and lists its commands in two separate switches; its general page tests the state in
  eight hand-written places.
- **The harness** pictures 11 of the 29 states in `screens`, and no battery item draws the keys screen. The `settings` item has
  pictured the load page since R-419 added a row above SETTINGS (KI-426): a list counted by index broke, and nothing noticed.

The series makes a screen its row. The row holds the state's traits; the methods that open, run, draw and close it; its
controller's commands; and its save lines. The frame, the menus, the controller and the harness all read that one table. A new
screen is then its row, its file and its way in. Nothing the game makes, draws, plays or saves changes, and the battery proves
it at every step.

## 1. Where it stands (2026-10-26, at 31cd9f0)

| What | Where | A new screen today |
| --- | --- | --- |
| The state | `GameState` and `STATE_ROWS` (game_types.h) | the value and its row; the count and order are asserted |
| Its update | game.cpp's `switch (state)`: 29 labels | a case |
| Its picture | the render `switch`: the scenes' cases, then `default`'s chain of 19 `if`s and an `else` | a link in the chain |
| Its sound off its screen | three checks at the frame's start | a line |
| The Esc menu | labels in menus.cpp (`items[]`); the count `8` and `case 0`-`7` in game.cpp | three edits in step |
| The guide menu | `GUIDE_ITEMS` (16 rows); `case 0`-`14`, with `default` as BACK | a label, and a shifted case each |
| The flight computer | `PAGE0`-`PAGE2` (11, 11 and 5 rows); `counts[]`; the counts again in the render | four edits in step |
| The settings, the slots | `SETTINGS_ITEMS = 19`, `case 0`-`17` and `item == 18`; the slots' `4 : 3`, written twice | edits in step |
| The cabin's devices | `CabinFacing`, `SCREENS`, `spots[]`, E's actions, the HUD's `names[]`, `FACING[]` | six places |
| Opening and closing | 16 openers, five return variables and the almanac's scene, 111 assignments | an opener and exits, by hand |
| The controller | pad.cpp: `padContextName` (15 cases), `padItems` (18 cases), the maps' buttons | a name and a case |
| auto-explore | an overlay needs nothing (it taps Esc); a driven scene needs an `AS_*` code in four places | nothing for an overlay |
| The save | the descent's phase is the state's number; otherwise placed by `ST_GROUND`, `ST_IN_PLAY` | its lines in `save`, `load` |
| The harness | `screens` (24 captures by hand), `flow` (hard-coded), `unit states` (29 names listed) | a capture, a hook, counts raised |
| The docs | 07's states and controls, the help pages (`renderHelp`), README's keys, the cookbook | their lines |

Notes on the rows:
- **Update:** nine cases hold code inline (TITLE, SPACE, DESCENT and ASCENT, SURFACE, HELP, MENU, STATS, SYSTEM_LIST, DATA). Six
  call `updateShipMotion(dt)` after their own update.
- **Picture:** TEXT_ENTRY is special-cased by `guideReturn` five ways. No code asks `ST_OVERLAY`; the `default` stands for it.
- **Sound:** the three checks stop the decoder's piece, the decoder's voice and the case's instrument.
- **Guide menu:** labels 1 and 2 are replaced by index in the render. The cookbook said 17 rows, and the code's comment still
  says seventeen.
- **Flight computer:** page 1's status column is a `switch (i)` whose `default` is the drone bay.
- **Cabin:** `SCREENS` holds each device's bank and colour, and fills a missing row with zeros without a warning. The facing is
  chosen from `updateCabin`'s `spots[]`. Two tables misread a device without its entry:
  - the controller's `FACING[]` gives a null pointer;
  - the HUD's `names[]` has no size, so it is read past its end.

  The wall clamps and `drawCabinBand`'s literal ids are more places a device touches.
- **Opening and closing:**
  - The five return variables are `returnState`, `guideReturn`, `keysReturn`, `almanacReturn` and `autoBack`, and
    `almanacScene` is a sixth origin. Nine states have no opener of their own.
  - Of the 111 assignments of `state` in src/game, 86 assign a literal (70 of them outside the openers). The other 25 assign
    a return variable or an expression: they are the closes.
- **The save:** a save writes the descent's and the ascent's phase as the state's number, and a probe being watched reloads
  into PROBE. `GameState`'s numbers are therefore in the saves.
- **Controller:** its general page tests the state in eight places. One is `ST_PHOTO` written out by hand. Another,
  `SURFACE || (SPACE && inSystem)`, equals no trait.

**The battery, state by state:**
- Every state but KEYS has a frame in some item.
- Input is never driven in ASCENT, and only Esc is pressed in HELP, SETTINGS, LOG and STATS.
- INSTRUMENTS has frames only in the unit. SHIPSCREEN's rows are pressed only in the unit, with no frame taken.
- `fuzz` and `input` (the keys screen, the controller's menus) are in no battery item.

**The game-screen task** (PLAN-reorganisation.md section 11, task 4: a ship's logbook opened from the cabin's menu, saved with
the game):
- E-01: 176,495 tokens, 65 tool uses, 17 files read.
- E-13: 200,212 tokens, 82 tool uses, fourteen places named.

## 2. Targets

- **A new screen** touches five places in the code: its `GameState` value, its row, its file, its banner in game.h, and its way
  in as a row of that list. None of them is a count or a case number kept in step with another. Its help lines and the docs
  follow.
- **Shapes left behind:**
  - the frame has no `switch` on the state;
  - pad.cpp has no list of states;
  - no menu has a literal count or a numbered case.
- **The harness** pictures and drives every state from its row. A new screen is covered with no harness edit, and the unit fails a
  row with a missing piece.
- **The game-screen task** (section 7) names at most seven places (fourteen at E-13), with no count or case number among them.
- **game.h** is at most 48 KB at the close (43.9 KB now). The rows' methods add about a line a screen: the nine updates made
  methods, the screens' controller lists and their openers.
- **Nothing** the game makes, draws, plays or saves changes.

## 3. The rules every milestone keeps

1. **Nothing the game makes, draws, plays or saves changes.**
   - GEN_VERSION stays 16, with no `regress bless`.
   - The save format and the persisted enums stay as they are, `GameState`'s numbers among them.
   - Each step is proven against the snapshot before it.
2. **One edit at a time, each proven.**
   - A case made a row keeps its calls and their order.
   - A list made rows keeps its order, labels and geometry.
   - A hand list of states replaced by a trait is first proven equal to it, for every state, by the unit.
3. **Tables are constant data.**
   - No new dynamically initialised global, and no self-registering constructor: link order would then order the table.
   - `Game`'s data members are appended, never reordered (the threads' and the caches' order).
4. **A trait new code needs is a column** (E-06). A row's methods live in the screen's own file.
5. **Each milestone leaves its notes true:** src/game's note, the cookbook's recipe, and 07's frame and states. `make check` holds
   the maps.
6. **Workflow:**
   - one milestone at a time, on the user's "proceed";
   - Docker and the game window are asked about first;
   - long runs go under `caffeinate -i`, in the background, alone.

## 4. The milestones

| # | Id | Item | Size |
| --- | --- | --- | --- |
| 1 | U-01 | The proof reaches every screen: a tour of every state, its keys and its controller menu; `settings` mended | L |
| 2 | U-02 | The screen table: each state's update, picture and sound in its row; the frame without a switch | L |
| 3 | U-03 | The ways in as rows: the Esc menu, the guide menu, the flight computer, the settings, the cabin's devices | L |
| 4 | U-04 | The controller from the row: a screen's commands, name and buttons; the general page by traits | M |
| 5 | U-05 | A screen's life: one way in and one way back, its own save lines and its reset | L |
| 6 | U-06 | The close: the harness reads the table, the recipe, the two tasks measured again | M |

**Why this order:**
- **U-01:** every step after it is proven with it. Today the battery cannot see most of what U-02 to U-05 change.
- **U-02:** the table is what the rest fill.
- **U-03 and U-04:** the ways in and the controller are where a screen is reached. Once U-02 exists, each is a column of the row.
- **U-05:** the riskiest change. It comes after the rows hold everything else, with the tour checking every way back.
- **U-06:** the harness can read the rows once every screen opens through its row.

### U-01. The proof reaches every screen (L)

**The tour** (`vesperis_test tour`, a mode in a file of its own in src/harness):
- For each state, in the table's order and from the same start (the home system, parked at its first world, every map waited
  for), the tour does five things:
  - opens the state the way the game does, through a harness list of openers (one per state) until U-06 reads the rows;
  - pictures it;
  - opens the controller's command menu and records each page as a picture and as its lines;
  - presses the state's keys one per frame: its help's keys, then its menu's rows by label, then Esc last;
  - after each key, writes the state's name, what is selected and the frame's hash, with a picture at each change of state.
- Every row of every menu is entered by its label, never by counting: the Esc menu, the guide menu, the flight computer's pages,
  and E at each of the cabin's devices.
- The states nothing drives today get their keys:
  - ASCENT;
  - HELP's page turns;
  - SETTINGS: each row, with a value changed and kept;
  - KEYS: its pages, and a binding captured;
  - LOG, and STATS (K);
  - INSTRUMENTS, and SHIPSCREEN's rows.
- It prints only what a step must keep. The return variables show only as where Esc goes, so folding them later (U-05) changes
  no line.
- Esc is pressed a bounded number of times, so a screen that will not let go is recorded, not waited on (KI-427's loop between a
  recording and the gallery was one, until B-429).
- **It is deterministic,** built the way the flow is: it runs `twice` in the battery, and a file that moves is named with its
  cause, then fixed or masked.
  - Each state starts from a fresh game at the same start. The rows that leave that start (NEW EXPEDITION, QUIT, VIMANA,
    DEPLOY, SAVE) run last in their state's run.
  - Its saves, guide, settings, keys, controller and shots go to paths of its own under shots/tests/, emptied first. The slots
    and the gallery then list a known set, and its keys file is not the one `screens` uses.
  - What reads the wall clock is kept out or fixed:
    - a save's `saved` time, and the stamps on marks and lent items;
    - the recordings' order;
    - the CLOCK setting's real time;
    - auto-explore's seed. AUTO's Enter stays with the unit's `auto` group.
  - What threads build is waited for: the maps, the drones' trip, and the marks collected at the frame.
  - The synth stays off, as it always is in the harness.

**The battery:**
- `tour` and `input` (the controller's pages and the keys screen) join `tools/prove.items`.
- **KI-426 mended:** the `settings` item finds SETTINGS by its label. Its picture and its +5 to the field of view are the
  settings screen's again.

**The baselines for the measurement:**
- E-13's run of the game-screen task still holds: the game's code has not changed since.
- The cabin-device task (section 7) is measured the same way, before any code changes.

**Done when:**
- Each of the 29 states has its picture, its keys and its controller menu in the battery, and every menu row is entered.
- `tour` gives the same output twice, and with one thread.
- `tools/prove compare b429` reports every item the same except the mended `settings` and the new items.
- `tools/prove snapshot u01` is taken.

### U-02. The screen table (L)

**The table:**
- `STATE_ROWS` moves after `class Game` to `src/game/states.cpp`, because a row names Game's methods.
  - It stays constant data, one row per state in `GameState`'s order, and that order is asserted.
  - `stateHas`, `statePad` and `stateName` read it as they do now.
- Its columns at this step: the name, the traits, the controller's context, `update`, `render`, `covers` and `away`. U-03 to U-05
  add theirs.

**The update:**
- Every state's update takes one signature, `(const Input& in, double dt, double realDt)`. The build has
  `-Wno-unused-parameter`.
- The nine inline cases become methods in their screens' files:
  - TITLE, HELP and MENU in menus.cpp;
  - SPACE's keys in autopilot.cpp;
  - DESCENT, ASCENT and SURFACE's keys in surface_play.cpp;
  - STATS in guide_screens.cpp;
  - SYSTEM_LIST and DATA in data_sheet.cpp.
- `updateShipMotion(dt)` after the update becomes a trait, `ST_SHIP_MOVES`, held by STAR_MAP, SHIPSCREEN, CONSOLE, SHARDS,
  INSTRUMENTS and DRONES.
  - The trait is read from the row taken before the update. A screen that closes itself therefore still moves the ship that
    frame, as it does now.
  - TITLE's own call (with the real time, partway through its keys) and the ground's photo mode's stay inside their methods.

**The picture:**
- A scene's row draws its scene, with its HUD.
- An overlay's row (`ST_OVERLAY`) draws over the scene. The frame draws that scene first, unless the row's `covers` says the
  overlay covers the whole frame right now: the galaxy chart, or a text entry over the chart, the probe or a recording.
- The scene under an overlay is today's choice on `returnState`, kept as one method: the ground when it is SURFACE, the title
  over space when it is TITLE, and space otherwise. It is not a scene row's picture, which would add that scene's HUD.
- TEXT_ENTRY's row draws what it names over (the decoder, the sector map) itself, as the chain does now. A text entry opened
  from the guide menu draws the scene under it, not the guide, and one over the star map's first page draws no map: as now.
- ALMANAC's row draws the landing map under it even while auto-explore plays: its own choice, not LANDING_MAP's row's.
- The galaxy chart's `keepFlightBand`, run before the chart is drawn, keeps the flight's band and its sound.

**The sound off its screen:**
- The row's `away` runs each frame for each row whose state is not current, in the table's order.
- That keeps today's order: the decoder's piece and voice, then the case's instrument (lines 281-283 now).
- SHARDS's `away` keeps the piece through any text entry, where the piece is named, but not the voice: lines 281-282's tests,
  moved whole.
- The radar and the telescope stay stowed by `ST_STOWS`.
- The frame's other tests of a state stay where they are, because each belongs to its subsystem, not to a screen:
  - the keys screen capturing a key;
  - the title's visit;
  - the almanac's refusals, and its run, which does not run under its own screen;
  - the probe's Shift.

**What it can break:**
- The order of the calls inside a case, and an early `break` (SPACE's and SURFACE's keys).
- The scene under an overlay: the ground when `returnState` is SURFACE, the title over space from TITLE, and space otherwise.
- The galaxy chart's `keepFlightBand` before its picture.
- A text entry over the guide menu, which draws no guide under it.
- The bench: the frame's dispatch is a hot path.
- GCC 9.4, the Linux builds' compiler, which crashed on kinds.cpp's compile-time table check (KI-423).
  - The pattern was checked on 2026-10-26 with Apple clang 16: a `constexpr` definition of `Game`'s static table in its own
    `.cpp`, naming protected methods. It lands in read-only data with its order asserted at compile time.
  - If GCC 9.4 trips on the table, its order check becomes a unit check. The Linux build checks this, with Docker asked first.

**Done when:**
- `Game::frame` has no `switch (state)` and no chain of states, and every state's row names its update and its picture.
- `tools/prove compare u01` reports everything the same.
- `bench check` is within its budgets, and pairs are level on the frames' rows.

### U-03. The ways in as rows (L)

- **The Esc menu:** one list of rows (label, action), read by its update, its picture and Enter. The box is sized from the
  count: eight rows, the same pixels.
- **The guide menu:** each row has a label, an action, and whether the ground refuses it.
  - The two labels that name the star or the world come from a method instead.
  - The comment that says seventeen goes; there are 16 rows.
- **The flight computer:** each page is a list of rows: label, action, its status on page 1, and whether it stays on the screen.
  The counts are read from the lists.
- **The settings and the slots:**
  - The settings become rows: a label, its value's text, and its change by Left and Right. They replace `SETTINGS_ITEMS`, the
    numbered cases, and the `names[]` and `values[]` kept in step.
  - The slots' count (four to load, three to save) is written once.
- **The cabin's devices:** one row per facing, holding:
  - its spot (x, z and reach);
  - the HUD's name, and the controller's "USE" line;
  - its screen's bank and colour;
  - its action on E.

  `updateCabin`, the HUD, the controller and the drawing all read that row, and a facing without one fails to compile. Today the
  HUD reads past its `names[]` and `SCREENS` fills with zeros. A device's own picture in `drawCabinBand` stays its code.
- A row whose action opens a screen names that screen's opener; U-05 makes it `openScreen`.

**What it can break:**
- Each label drawn where it was: the box's size, its place by `i`.
- A setting's value text and its step, and the settings file it writes.
- The ground's refusals.
- The flight computer's rows that stay on the screen, and the two that set `guideReturn` before forcing the state (`targetHome`,
  the previous star).
- The cabin's wall clamps.

**Done when:**
- No menu holds a count or a numbered case.
- A cabin device is one row plus its drawing.
- `tools/prove compare u02` reports everything the same.

### U-04. The controller from the row (M)

- **A screen's commands and name** (page 0) become the row's `padItems`, a method beside the screen's other methods.
  - It builds its lines through a small `PadMenu` in pad_state.h (`item`, `typed`, the title), in place of pad.cpp's lambdas.
  - The state cases of `padContextName` and the switch in `padItems` go. The names chosen by context stay.
- **The maps' buttons** become the row's `padButtons`: the landing map, the sector map, and the star map's two pages. The
  legend's list of the three maps is read from it.
- **Page 1 (GENERAL):** each of its eight tests of the state becomes a trait once the unit proves the two equal for every
  state; one is `ST_PHOTO` already. A test that reads more than the state (`SURFACE || (SPACE && inSystem)`) stays a test,
  written once as a named method.
- **The two dead names** (THE CAPSULE, AUTO-EXPLORE) go: no page 0 shows them.

**What it can break:**
- The hint matched to a button by its key and modifiers.
- The order of the lines.
- modes_input's pinned labels and legends.
- "Skip page 0 when it is empty", which is written in four places.

**Done when:**
- pad.cpp has no `switch (state)` and no list of states.
- `tools/prove compare u03` reports everything the same.

### U-05. A screen's life (L)

Where a screen came from and where it goes back are two things today, and the plan keeps them apart:
- **The scene** is what an overlay is drawn over and where a finished action lands: `returnState`, only ever TITLE, SPACE or
  SURFACE. It stays, named as the scene.
- **The way back** is where Esc goes. Today five variables hold it (`guideReturn`, `keysReturn`, `almanacReturn`, `autoBack`,
  and `returnState` again for some screens), plus `almanacScene` for the almanac's picture.

**One way in:** `openScreen(state)`, what the 16 openers share.
- It runs the row's `enter` (its fields reset, its devices stowed), records where the screen was opened from, and sets `state`.
- A screen that closes as it opens the next one passes on its own way back instead of itself. Examples are the guide menu's
  rows, the flight computer's rows (it sets SPACE first today), and the Esc menu's HELP.
- The game's openers and U-03's rows call it, and the nine states without an opener get one.

**One way back:** the row's `back` says where Esc goes, and `closeScreen()` follows it. The value is one of three:
- **its opener:** KEYS, AUTO, a text entry;
- **the scene:** HELP (even from the Esc menu), MENU, the guide, and the log, statistics and gallery the guide opens;
- **a fixed screen:** SETTINGS always goes back to MENU, even when the flight computer opened it.

An exit that is an action's, not Esc's, stays the action's own: a slot saved goes to the scene, and AUTO's Enter starts the run.

**Folding the variables:**
- Each variable is checked against its row's `back` over every path the tour takes. One that always agrees folds into it.
- One that does not stays, and its row says why. The reads that are not ways back stay with it:
  - `guideReturn` read as "on the ground" (the guide's refusals);
  - `guideReturn` read as `targetHome`'s destination;
  - the almanac opened from DATA going back past DATA;
  - KEYS drawn over the scene but closed to its opener.
- **`guideReturn` folds now that KI-427 is fixed** (B-429, 2026-10-26). Naming a storm in a recording left it at RECORDING, and
  the gallery's Esc looped back to the recording. The recording now sets the gallery's way back, the ship's, again as it returns.

**The assignments:**
- The 70 literal assignments outside the openers become `openScreen` (an entry) or `closeScreen` (an exit that goes back), or
  stay. What stays: the landing and the ascent, the title, a load, and a screen that stays on.
- The 25 that assign a variable are the closes this folds.
- The log counts each kind before and after.

**A screen's own save lines and its reset:**
- The row's `save` runs after the save's existing lines, and its `load` gets each key that `load` does not know.
- The row's `reset` runs at the end of `resetExpedition`.
- No existing line moves, since a save is compared byte for byte. These hooks are for the screens to come.

**What it can break:**
- Where Esc goes from each screen; the tour checks every one.
- The scene under an overlay, and the scene a finished action lands in.
- `guideReturn` set before `targetHome` forces the state.
- The flow's stages, and a load into a screen (the descent's phase, a watched probe).

**Done when:**
- Every overlay opens through `openScreen` and closes through its row's `back`, or its row says why not.
- The assignments left are counted by kind.
- `tools/prove compare u04` reports everything the same.

### U-06. The close (M)

- **The harness reads the table.**
  - The tour opens each state through its row (`openScreen`), presses its controller's commands, and takes its pictures.
  - A state that needs a world set up first keeps its setup in the tour, by name: the descent, the probe, a recording.
  - `screens` keeps its captures.
  - A new overlay is pictured and driven with no harness edit.
- **The unit checks the rows.** `unit states` walks every row instead of a list of 29. Each row must have:
  - its name, an update and a picture;
  - a way in, or none by design;
  - controller commands, or none.

  The unit also checks that the tour reached the row.
- **The recipe:**
  - the cookbook's "Add a game state" becomes "Add a screen": five places in the code, then the docs;
  - 07's frame and states, src/game's note and the lessons are rewritten as they now are.
- **The two tasks** (section 7) are measured again.

**Done when:**
- Section 2's targets are met, with their numbers.
- From a clean build: the whole battery against u05, `bench check` and `make check` pass.
- `tools/prove snapshot u06` is taken.

## 5. The proof, every milestone the same

- **The base:** `build/prove/b429` (B-429's fix on e13) until U-01 takes `u01`. Each milestone's snapshot is the next one's base.
- **After each step:** `tools/prove compare <base>` on the items the step can touch: `tour`, `screens`, `flow`, `input`, `unit`
  and `settings`.
- **At the close:**
  - the whole battery;
  - `bench check`, plus pairs on the frames at U-02;
  - `make check`;
  - then `tools/prove snapshot <id>`.
- **How it runs:** through the `prover` agent, under `caffeinate -i`, alone (`docs/reference/harness/proof.md`).
- **The disk:** each snapshot takes about 0.8 GB, and a compare's outputs as much again; the series takes six snapshots.
  - The E series' snapshots `build/prove/e02` to `e12` (11.6 GB) were deleted on the user's word on 2026-10-26: 15 GB free.
  - e13 and b429 stay, and the old kits `build/p00` to `build/d05` (8.9 GB) stay until the user says.

## 6. Decided by default (the user may change them)

1. **The series:** the letter U, unused so far. Six milestones in 1.4.0, on its branch and its open PR, before the release.
   Generation 16 is unchanged.
2. **Screens stay Game's methods:** a row names them as member function pointers. They are not made classes (section 8).
3. **The table** lives after `Game` in `src/game/states.cpp`. The trait flags and `GameState` stay in game_types.h.
4. **The tour** is a mode of its own, beside `screens` and `flow`. Its keys come from a harness list until U-06.
5. **Commits:** one per milestone when the user asks, under a random author and committer, with no session link.
6. **The measurement:** the game-screen task and a cabin-device task, each measured before (E-13's run; U-01) and at U-06.

## 7. The two tasks

Each runs the way section 11 of PLAN-reorganisation.md ran its eight:
- a read-only `Plan` subagent on Opus;
- that section's preamble, verbatim;
- the tokens, tool uses and minutes taken from the task's notification.

The tasks:
- **The game-screen task** is that plan's task 4, verbatim: "Task: add a new game screen, a ship's logbook opened from the cabin's
  menu, listing the systems visited with the date of each visit; Escape closes it, and the log is saved with the game. Name every
  file and place that must change, in order." E-01: 176,495 tokens. E-13: 200,212 tokens, fourteen places.
- **The cabin-device task** is new: "Task: add a device to the cabin, a barometer on its right wall: E at it opens its screen,
  which plots the air pressure of the world the ship is parked at over the last day; the controller's command menu offers its
  commands, and Escape closes it. Name every file and place that must change, in order."

**The cabin-device task, before** (U-01, 2026-10-26, before any code changed; a `Plan` agent on Opus, alone): 232,940 tokens,
86 tool uses, 13.5 minutes. It named ten places in the game's code:
- game_types.h: the state's value, its row, the count, the facing and its `SCREENS` row;
- the pressure beside the fronts; game.h's banner; the new file; the frame's two switches;
- cabin.cpp: the palette's loop, `spots[]`, E's switch, the band's dial, the HUD's unsized `names[]`, `PAGE1` with `counts[]` and
  the render's `n` kept in step, the status column's `default`;
- pad.cpp: the screen's name, `FACING[]` and its commands; the help's first page.

Besides them, five in the harness and the docs. Found on the way: no palette bank is free in the cabin (all 32 taken).

## 8. Considered and set aside

- **Screens as classes behind a virtual interface.**
  - Code outside `Game` would need Game's internals opened: made public, or each screen nested in `Game`. Every line of it would
    then go through a `Game&`.
  - The 29 screens of today would need forwarders or a rewrite.
  - A row of Game's methods gives a new screen the same places without that churn.
  - The price is game.h: each new screen adds its banner and a few lines. Worth revisiting if game.h passes 60 KB as screens
    are added.
- **A screen stack** (each screen pushed, Esc popping): Esc does not go back one screen everywhere today.
  - HELP opened from the Esc menu closes to the scene.
  - SETTINGS goes back to the Esc menu even when the flight computer opened it.
  - The guide's screens close to the scene, not to the guide.

  A stack would move where Esc goes. U-05's `back` column says each screen's way back as it is today.
- **Self-registering screens:** link order would order the table, and rule 3 forbids the dynamic initialisation.
- **An enter and a leave on each change of state, for the sounds:** the frame's checks run every frame and also catch a state
  changed partway through one. The row's `away` keeps them as they are.
- **Moving the existing save lines into the rows:** their order is the save's bytes.
- **A toolkit for drawing the screens** (lists, panels, scrolling): `ui.h`'s `drawPanel`, `menuStep` and `wrapText` serve. A
  toolkit would be a job of its own, with pixels at stake.

## 9. Found on the way

- **KI-427, a trap:** naming a storm in a recording (N) set `guideReturn` to RECORDING, and nothing restored it.
  - From then on, Esc or Enter in the recording went to the gallery, and Esc or Enter in the gallery went back to the
    recording. The explorer could not get out without quitting.
  - Fixed on the user's word as B-429 (2026-10-26), before U-01: the recording sets the gallery's way back again.
- **KI-426:** the `settings` item pictures the load page and never changes the field of view. It counts three rows down the Esc
  menu, and R-419 put AUTO-EXPLORE above SETTINGS. U-01 mends it.
- **The guide menu's row count:** the cookbook said 17 rows; there are 16, and it was corrected on 2026-10-26. The code's comment
  still says seventeen; U-03 removes it.
- **The cabin's devices:** a facing without its entries gives the controller a null pointer that "USE " is added to, and makes
  the HUD read past its `names[]`. U-03 closes both.
- **Dead and unread:** two of the controller's screen names are shown by no page (U-04), and no code asked `ST_OVERLAY` (U-02:
  the frame's picture asks it now).
- **KI-428 (U-01's survey):** a T typed at the GOES console also multiplies the time warp, and Ctrl+T there opens the almanac: the
  frame asks whether a state simulates, not whether it types. Offered, not fixed.
- **The almanac's commands:** while a run goes, the controller offers STOP THE RUN (T), which the almanac's screen ignores
  (KI-431, offered at U-04).
- **KI-429:** the settings screen's title overlaps its first row (seen once U-01 mended `settings`' picture). Offered, not fixed.
- **U-02, the table's definition:** clang reads a static member's out-of-class `constexpr` definition as an inline variable, emitted
  only where it is used; GCC 15 reads it as a plain one. states.cpp reads the table out of line, so it holds the one definition
  either way. GCC 9.4, the Linux builds' compiler, is unchecked on it until Docker is asked (after KI-423).
- **U-03, on the way:** 07 named eight of the computers' colours and banks 21-27, two devices behind (C-11, C-18). The game's
  note counted game.cpp and audio.cpp among the files over 50 KB (16 and 33 KB now). Both are true now. The unit still presses
  two of the flight computer's rows by number (`testShipScreenRow(0, 2)` and `(0, 4)`); U-06 finds them by label.
- **U-04, on the way:** one method writes a screen's page and, on a map, its buttons and sticks: the plan had a second column,
  but a map's buttons share their state with its lines' words. The data sheet's test reads traits and the system (a scene, on
  the ground or in a system), so it needed no method. The console's keyboard line moved into its row (`typing`). pad.cpp still
  tests KEYS, the screen its setup line opens (offered there, the line would make the screen its own way back), and the aim's
  SPACE (`ship.targeting` lasts off it).
- **U-05, on the way:** the scene (`returnState`) is set by the keys that open a screen from a scene, not by the cabin's screens:
  after H at the title, the flight computer, the console, the drone bay and the case draw over the title; after H or Esc on the
  ground and the ascent, over the ground (KI-432). H at the title also sets it, so a game saved on the ground and opened again
  resumes in space (KI-433). The ship's screens, the landing map and the probe keep SPACE as their way back, not the scene, for
  it; `openScreen` setting the scene from a base mends KI-432 in one line. `returnState` keeps its name: U-04's scene is a base
  in play, and the title is a base. The row's save hooks default to null (U-03's lesson), so no row writes them yet.
