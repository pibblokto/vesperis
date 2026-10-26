# Rules: the game's screens: the states' table, the ways in and the controller

The rules in force for how a game state is made, a line each with the entries behind them (`log/`, by release). A milestone that
sets or changes one edits its line here; a replaced one moves to `superseded.md` (`docs/CLAUDE.md`). What the screens show and
their keys: `rules-game.md`.

## The states' table
- A state is its row in `Game::STATE_ROWS` (states.cpp): its traits, asked by `stateHas`; a trait new code needs is a column, not a list [E-06], [U-02]
- The table is constant data in GameState's order, its order and each row's update and picture asserted at compile time [U-02]
- The frame runs the current row: its `update`, `updateShipMotion` on `ST_SHIP_MOVES` (by the row before it), its picture; no switch on the state [U-02]
- An overlay draws over `renderUnder` (`returnState`'s scene: the ground, the title over space, else space; no HUD) unless its `covers` [U-02]
- A screen's sound stops off it in its row's `away`, run each frame for every row but the current, in the table's order [U-02]
- Every update takes `(const Input&, double dt, double realDt)`; a row's methods live in its screen's file [U-02]
- The frame's other tests of a state are their subsystems' (a key captured, the title's visit, the almanac, the probe's Shift) [U-02]

## The ways in
- A menu is a list of rows in its order, its count read from the list (`rowsOf`): no menu holds a count or a numbered case [U-03]
- A list is constant data, a `static constexpr` table in a Game static method, its acts lambdas; each row whole, asserted [U-03]
- A cabin device is its row in `cabinDevices` plus its picture in `drawCabinBand`; a facing without its row fails to compile [U-03]
- A row's act keeps its case's calls and their order; an act that opens a screen calls its opener [U-03]

## The controller
- A screen's controller is its row's `padMenu` in its file: its tab's name and lines, a map's buttons and sticks, a typing screen's keys [U-04]
- The general page goes by traits: a base, a scene (a base in play), the ground, `ST_CLOCK`, `ST_PHOTO`; pad.cpp tests only KEYS and the aim's state [U-04]

[E-06]: log/6-1.4.0.md
[U-02]: log/6-1.4.0.md
[U-03]: log/6-1.4.0.md
[U-04]: log/6-1.4.0.md
