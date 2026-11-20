# Rules: the harness and the proof

Rules in force for vesperis_test, its checks and the proof that a change changes nothing, one line per rule with its log entries
(`log/`, by release). A milestone that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md`
(`docs/CLAUDE.md`). This file holds the harness code, its checks and the tour. Scenes and test worlds are in
`rules-harness-scenes.md`; regress, the battery and the bench are in `rules-harness-proof.md`.

## The harness's code
- Only one `Game` exists at a time (the game has globals): the harness swaps a fresh guide in and back out, and never builds a second [C-14]
- Harness code lives in `src/harness/`, linked only into vesperis_test: a new file joins `HARNESS_SRC` and CMake's `HARNESS_SOURCES` [E-02], [E-03]
- Every mode is a `MODES` row in `main.cpp` (name, group, arguments, line, file, function); no argument runs `stars`, an unknown mode exits 0 [E-01], [E-03]
- No extern harness symbol shares an engine symbol's name; the harness has no dynamically initialised global of its own [E-03]
- The harness builds `TestGame`s (`test_game.h`), and the hooks live there, never in game files; the game reads only `harness` [E-05]
- A `TestGame`'s files (guide, saves, settings, keys, pad, photos, films) default to `shots/tests/`: the working directory's files are the player's [B-440]
- Shared declarations go in `harness.h` (sections by file), pins in `test_worlds.h`, scenes in `scenes_table.h`; a file over 100 KB is split [E-03]
- Scripted games move by `pressKey` and `runFrames`, the unit and `<mode> check` share one `Check`, and `findScene` finds scenes [E-03]
- Outputs go to `shots/tests/` (~2 GB: never copied whole), not the player's `shots/`; the flow's charts are `starmap_*`, not `chart_*` [O7-01], [F-01], [G-06]
- The harness reads every grammar `.vg` from disk, so an edit reprints without a rebuild; the unit holds the file list to the build's [H-01], [H-04 second pass]
- The harness has no platform: the window's frame is traced (`VESPERIS_TRACE=1 ./vesperis`), not benched [F-01], [F-08]

## The unit, the flow and the tour
- The unit is `UNIT_GROUPS` (run order: `unit list`); each group starts at its scale and mush from the full run, so a group alone gives the same results [E-03]
- A check over a region states the count it expects, so a thinner galaxy fails instead of testing less [E-01], [E-03]
- The unit's last group, `files`, checks that the player's files in the working directory are as the unit found them before its first group [B-440]
- A target owned by an unfinished item prints `FAIL(<item>)` and is not counted; the exit code follows the targets in force [O6-01]
- Unit checks about giants read every giant (made classes too), not just the gas giant type [P-07]
- The flow only reports: a landing in the sea or a buggy at 0 km/h still exits 0, so read its status lines [G-04]
- The flow lands by `testLandSite` (lit, level, dry, from the body's seed) and drives only briefly: `drive` is the long-drive test [G-04], [S-01], [N4-02]
- The flow's game runs its stages before the stages that make games of their own (KI-450); a new stage goes where no later frame moves [Y-02]
- `descent` holds B-307's bar (grade 0.45, nothing airborne; checked every 2 m, 3 m clear of colliders) at the mountain pin (KI-343: it fails) [G-04], [S-01]
- `tour` opens each stop's state as the game does, then every row by `openScreen` or a named setup, and runs the stop's commands [U-01], [U-06]
- A new screen needs no harness edit: the tour's rows and `unit states` read the table, and a scene or a world set up first is a stop [U-06]
- The tour enters a menu row by its label (`selectRow`, the game's own rows), never by counting, and so do `settings` and `input` [U-01], [U-03]

## Frames that repeat
- A harness frame that draws a body first waits for its map (`testWaitMapOf`, `mapIfReady`); without the wait, code placement can flip it [E-02], [E-03]
- A test waits for a thread at one instant (dt 0: `testWaitMap`, `testPlayerWaiting`), so a faster binary moves no clock [F-02], [F-05], [F-07]
- A render sets the renderer's season before warming a map (`warmMaps` takes its t); `testParkAt` does both (`waitMap` false: no wait) [P-01], [F-02], [B-438]
- `bothways` and the unit draw a frame at one instant (`frame(in, 0)`) twice with the workers, then on one thread: any difference fails [F-01], [B-423]
- The unit holds frames the same on 1 to 8 threads (both-ways, `threads`): no dispatch off the main thread, no stale corner [A-02], [B-423], [F-04], [F-06]
- A change to what threads share runs clean under ThreadSanitizer (the touched scenes, `screens`, `pool`) [F-02..F-07]
- The tour waits after every frame for what threads build (`testSettle`: the system's maps, the raised cache, the scanner) [U-01]

[O6-01]: log/0-1.0.0.md
[N4-02]: log/0-1.0.0.md
[O7-01]: log/0-1.0.0.md
[G-04]: log/1-1.1.0.md
[S-01]: log/1-1.1.0.md
[C-14]: log/1-1.1.0.md
[G-06]: log/3-1.3.0.md
[A-02]: log/3-1.3.0.md
[H-01]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[F-01]: log/4-1.3.1.md
[B-423]: log/4-1.3.1.md
[F-02]: log/4-1.3.1.md
[F-02..F-07]: log/4-1.3.1.md
[F-04]: log/4-1.3.1.md
[F-05]: log/4-1.3.1.md
[F-06]: log/4-1.3.1.md
[F-07]: log/4-1.3.1.md
[F-08]: log/4-1.3.1.md
[P-01]: log/6-1.4.0.md
[P-07]: log/6-1.4.0.md
[E-01]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
[E-03]: log/6-1.4.0.md
[E-05]: log/6-1.4.0.md
[U-01]: log/6-1.4.0.md
[U-06]: log/6-1.4.0.md
[U-03]: log/6-1.4.0.md
[B-438]: log/7-1.5.0.md
[B-440]: log/7-1.5.0.md
[Y-02]: log/7-1.5.0.md
