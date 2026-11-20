# Rules: the game's screens: the states' table, the ways in, the controller and a screen's life

Rules in force for how a game state is made, one line per rule with its log entries (`log/`, by release). A milestone that sets
or changes a rule edits its line here; a replaced rule moves to `superseded.md` (`docs/CLAUDE.md`). What the screens show and
their keys are in `rules-game.md`.

## The states' table
- A state is a row in `Game::STATE_ROWS` (states.cpp) holding its traits, read by `stateHas`; a trait that new code needs is a column, not a list [E-06], [U-02]
- The table is constant data in `GameState` order; the order and each row's update and picture are asserted at compile time [U-02]
- A new state goes before `GameState::COUNT`, which `STATE_COUNT` reads: state numbers are saved, and no count is kept by hand [U-06]
- The frame runs the current row (`update`, `updateShipMotion` if the row before the update has `ST_SHIP_MOVES`, the picture), no switch on the state [U-02]
- An overlay draws over `renderUnder` (the scene in `returnState`: the ground, the title over space, else space; no HUD) unless the row `covers` [U-02]
- A screen's sound stops in its row's `away` once the screen is left; `away` runs each frame for every row but the current one, in table order [U-02]
- Every update takes `(const Input&, double dt, double realDt)`; a row's methods live in that screen's file [U-02]
- The frame's other state tests belong to their subsystems (a captured key, the title's visit, the almanac, the probe's Shift) [U-02]

## The ways in
- A menu is a list of rows in display order, with the count read from the list (`rowsOf`): no menu holds a count or a numbered case [U-03]
- A list is constant data: a `static constexpr` table in a static `Game` method, with lambdas for the acts; each row is asserted whole [U-03]
- A cabin device is a row in `cabinDevices` (room, stand, screen shade) plus fixtures in the ship plan; a facing with no row won't compile [U-03], [I-01]
- A row's act keeps the calls of the case it replaced, in their order; an act that opens a screen calls that screen's opener [U-03]

## The controller
- A screen's controller page is its row's `padMenu`, in the screen's file: the tab name and lines, a map's buttons and sticks, a typing screen's keys [U-04]
- The general page goes by traits: a base, a scene (a base in play), the ground, `ST_CLOCK`, `ST_PHOTO`; pad.cpp tests only KEYS and the aim state [U-04]

## A screen's life
- `openScreen(S)` opens every screen: the base it opens from becomes its scene (`returnState`); then `openedFrom`, the row's `enter`, the state [U-05], [B-431]
- A scene's own state never reads `returnState` (the climb aboard and the title's Enter leave it stale); a save or photo reads it only off the scenes [B-431]
- The title is opened from the game it resumes (`openedFrom[TITLE]`); a save and a photo place the explorer by `placeNow()` [B-432]
- Esc, and any exit that goes where Esc goes, is `closeScreen()`, by the row's `back`: the scene (`returnState`), the opener or a fixed screen [U-05]
- A screen that closes as it opens the next one closes first, so its way back becomes the next screen's opener; an action's own exit is left as it is [U-05]
- A screen's own save lines, load and reset are columns of its row, run after the save's lines, for a line `load` does not know, and at the reset's end [U-05]

## Words
- Player text is plain English (the `plain-english` skill): things named, not "ITS"; no "THE" in titles, labels, menus or hints [R-446]
- Strings the code or the harness compares keep their words until both sides change together [R-446]

[E-06]: log/6-1.4.0.md
[U-02]: log/6-1.4.0.md
[U-03]: log/6-1.4.0.md
[U-04]: log/6-1.4.0.md
[U-05]: log/6-1.4.0.md
[U-06]: log/6-1.4.0.md
[B-431]: log/6-1.4.0.md
[B-432]: log/6-1.4.0.md
[I-01]: log/7-1.5.0.md
[R-446]: log/7-1.5.0.md
