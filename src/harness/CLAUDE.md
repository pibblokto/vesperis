# src/harness: vesperis_test (no window, no raylib)

Only `vesperis_test` links it. `vesperis_test help` lists every mode with its arguments and its file; `help md` writes
`docs/reference/harness/INDEX.md` and `tools/unitchecks.py` the unit's `unit-checks.md` (`make check` compares both).

- `main.cpp`: start-up (`scale=N`, `shots/tests`), `MODES` (the table `help` prints).
- `harness.h`: engine headers, `Mode`, `Check`, `UnitGroup`, `SceneCode`, `systemAt` (a star and its system) and `scanSystems` (the
  sectors' systems in the two loops' order); each file's shared functions under its name.
- `test_game.h`, `test_game.cpp`: `TestGame`, the Game the harness builds, and its hooks (`test*`, `setState`).
- `test_worlds.h`: the pinned worlds and sites, the protected systems (`salt scan`). `scenes_table.h`: the scenes (`CMP_SCENES`,
  `findScene`), placed by `scenes_setup.cpp` (`setupSceneForType`: its finders by code family, `SceneFind`) and
  `scenes_special.cpp` (fixed places).
- `frames.cpp`, `audio_util.cpp`: frames to PNG and compared, the clock, the test scale, scripted moves (`pressKey`,
  `runFrames`, `selectRow`: a menu's row by its label); the receiver headless, WAVs, an FFT, a pitch.
- Modes by area: `modes_galaxy`, `modes_space`, `modes_probe`, `modes_surface`, `modes_terrain`, `modes_kinds`,
  `modes_signals`, `modes_rogues`, `modes_drones`, `modes_game` (the flow, `Flow`), `modes_input`, `modes_tour` (every
  state toured, `Tour`, its `STOPS`), `modes_bench` (`Bench`'s stages), `modes_proof`.
- The peoples' modes, each with its `*Checks`: `civ_forms`, `civ_genome`, `civ_walls`, `civ_plan`, `civ_works`,
  `civ_peoples`, `civ_lore`, `civ_music`, `civ_records`, `civ_instruments`, `civ_grammar`.
- `unit.cpp`: `UNIT_GROUPS`, `unit [list] [-q] [group ...]`; groups in `unit_basics`, `unit_kinds`, `unit_drifters`,
  `unit_signals`, `unit_navigation`, `unit_civ`, `unit_peoples`, `unit_sky`, `unit_probe`, `unit_proof`.
- `fingerprint.cpp`, `fingerprint.h`, `fingerprint_modules.cpp`: `fingerprint ...`, keyed lines for `tools/prove`.

Rules:
- A mode: a `MODES` row (name, group, arguments, one line, file, function), its function in its area's file. No argument
  runs `stars`; an unknown mode prints "unknown mode" and exits 0.
- A new `GameState` is a stop in `modes_tour.cpp`'s `STOPS` (its opener and keys; the count and order are asserted).
- A check goes in its group's function; a new group is a `UNIT_GROUPS` row in the unit's order with the scale and mush the
  groups before it leave (a scripted game leaves 4x). A check over a region states the count it expects.
- A new scene code: a `SceneCode`, a finder in the chain, and out of the trait family's `<= SCENE_STAINED_CRACK` range.
- Outputs go to `shots/tests/`; `tests/` and the grammar are read from the working directory (the repo root, or
  `tools/prove`'s scratch directory). `records check` reads the `test_settings.txt` that `settings` writes.
- The unit and the flow both write `test_guide.txt` (the root note's rule). A frame of bodies waits for their maps
  (`testWaitMapOf`, `mapIfReady`).
- Pictures and sounds: the `look` skill.
- No dynamically initialised globals and nothing self-registering. More: `docs/reference/lessons.md`, "The harness".
