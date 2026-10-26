# Rules: the harness and the proof

The rules in force for vesperis_test, its checks and the proof that a change changes nothing, a line each with the entries
behind them (`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to
`superseded.md` (`docs/CLAUDE.md`). This file holds the harness, its checks, scenes and test worlds; the regress, the battery
and the bench are in `rules-harness-proof.md`.

## The harness's code
- One `Game` lives at a time (the game's globals): the harness swaps a fresh guide in and back, never builds a second [C-14]
- Harness code lives in `src/harness/`, linked only into vesperis_test: a new file joins `HARNESS_SRC` and CMake's `HARNESS_SOURCES` [E-02], [E-03]
- Every mode is a `MODES` row in `main.cpp` (name, group, arguments, line, file, function); no argument runs `stars`, an unknown one exits 0 [E-01], [E-03]
- No extern harness symbol shares an engine symbol's name; the harness has no dynamically initialised global of its own [E-03]
- The harness builds `TestGame`s (`test_game.h`): the hooks are its, none in game files; the game reads only `harness` [E-05]
- Shared declarations go in `harness.h` (sections by file), pins in `test_worlds.h`, scenes in `scenes_table.h`; a file over 100 KB is split [E-03]
- Scripted games move by `pressKey`, `runFrames`; the unit and `<mode> check` share one `Check`; `findScene` finds scenes [E-03]
- Outputs go to `shots/tests/` (~2 GB: never copied whole), not the player's `shots/`; the flow's charts are `starmap_*`, not `chart_*` [O7-01], [F-01], [G-06]
- The harness reads every grammar `.vg` from disk (an edit reprints without a rebuild); the unit holds the list to the build's [H-01], [H-04 second pass]
- The harness has no platform: the window's frame is traced (`VESPERIS_TRACE=1 ./vesperis`), not benched [F-01], [F-08]

## The unit and the flow
- The unit is `UNIT_GROUPS` (41, in run order), each stating the scale and mush it starts at in the full run: a group alone gives the same results [E-03]
- A check over a region states the count it expects, so a thinner galaxy fails instead of testing less [E-01], [E-03]
- A target an unfinished item owns prints `FAIL(<item>)` and is not counted: the exit code follows the targets in force [O6-01]
- Unit checks about giants read every giant (the made classes too), not the gas giant's type [P-07]
- The flow only reports: a landing in the sea or a buggy at 0 km/h still exits 0, so its status lines are read [G-04]
- The flow lands by `testLandSite` (lit, level, dry, from the body's seed) and drives only briefly: `drive` is the long-drive test [G-04], [S-01], [N4-02]
- `descent` holds B-307's bar (grade 0.45, nothing airborne; checked every 2 m, 3 m clear of colliders) at the mountain pin (KI-343: it fails) [G-04], [S-01]

## Scenes and test worlds
- A new scene appends a `SceneCode` (values fixed), its finder in the chain, outside the trait range (`<= SCENE_STAINED_CRACK`) [S-04], [P-02..P-06], [E-03]
- Finders read the light as the view does (`effectiveSunAlt`), scanning the year (`findTime`) and, on a locked world, the longitude [G-04]
- A scene's site shows what it is named for (its light, its canopy): a placed site keeps its longitude, a miss moves to the next body [G-04], [S-01]
- Every scene prints its site (body, sector, latitude, longitude, light); a small moon's stand is within its 1-2 km horizon [G-04], [P-06]
- `forTypeBodies` and the scenes try every real world before a staged one (`stageElsewhere`) [P-05]
- Scans of every people's world read `KT_PEOPLE`; the C-01 test worlds' and the scene worlds' searches keep C-01's kinds [P-04]
- Pinned sites (`PIN_*`, on Wyariothmai I) go stale as the ground moves: re-pin them on lit ground (`landforms sites search`) [G-04], [S-01], [W-03]
- The unit and the scenes stand on the civilisation test worlds Aieliaalas II, Leileashphail III and Eleinewai I (152 0 92, two peoples) [C-08], [C-12], [C-13]
- `AGE_WORLDS` pins the nearest people of each age on sector y 0 (modern: Yeesheleemiola II-b's first); `peoples ages` says when one moves [A-01], [R-413]
- The history checks' worlds of two: Leanmya V-a (met) and Hiameshseine I-a (conquest); the dam check also reads Shioleesein I-a [R-413]

## Frames that repeat
- A harness frame drawing a body waits for its map first (`testWaitMapOf`, `mapIfReady`): unwaited, code placement can flip it [E-02], [E-03]
- A test waits for a thread at one instant (dt 0: `testWaitMap`, `testPlayerWaiting`), so a faster binary moves no clock [F-02], [F-05], [F-07]
- A render sets the renderer's season before warming a body's map; `testParkAt` does both (with `waitMap` false it does not wait) [P-01], [F-02]
- `bothways` and the unit draw a frame at one instant (`frame(in, 0)`) twice by the workers, then by one thread: any difference fails [F-01], [B-423]
- The unit holds frames the same by 1 to 8 threads (both-ways, `threads`), no dispatch off the main thread, no stale corner [A-02], [B-423], [F-04], [F-06]
- A change to what threads share runs clean under ThreadSanitizer (the touched scenes, `screens`, `pool`) [F-02..F-07]

[O6-01]: log/0-1.0.0.md
[N4-02]: log/0-1.0.0.md
[O7-01]: log/0-1.0.0.md
[G-04]: log/1-1.1.0.md
[S-01]: log/1-1.1.0.md
[S-04]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-12]: log/1-1.1.0.md
[C-13]: log/1-1.1.0.md
[C-14]: log/1-1.1.0.md
[W-03]: log/2-1.2.0.md
[G-06]: log/3-1.3.0.md
[A-01]: log/3-1.3.0.md
[A-02]: log/3-1.3.0.md
[H-01]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[R-413]: log/3-1.3.0.md
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
[P-02..P-06]: log/6-1.4.0.md
[P-04]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[P-06]: log/6-1.4.0.md
[P-07]: log/6-1.4.0.md
[E-01]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
[E-03]: log/6-1.4.0.md
[E-05]: log/6-1.4.0.md
