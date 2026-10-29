---
name: harness
description: Add or change a vesperis_test mode or a unit check in src/harness - the MODES row and its area's file, the unit's groups (UNIT_GROUPS), scene codes, outputs in shots/tests, and the traps (shared test_guide.txt, maps made on threads, a group's starting scale). Use before writing harness code.
---

# Add or change a mode or a unit check

The map is `src/harness/CLAUDE.md`; `./vesperis_test help` lists every mode with its file, `./vesperis_test unit list` every group.

## A mode

1. Its function goes in its area's file (`modes_<area>.cpp`, `civ_<topic>.cpp`): `int runX(int argc, char** argv)`, its comment
   above it saying what it prints and writes. A helper only it uses is `static`; one another file calls is declared in
   `harness.h` under the file's name.
2. Its row in `MODES` (`main.cpp`): name, help's group, the arguments, one line, the file, the function (a lambda when it needs
   the arguments read: copy the forms of the rows round it). Then `./vesperis_test help md > docs/reference/harness/INDEX.md`
   (`make check` compares), and what it does at length in `docs/reference/08-testing-and-tools.md`.
3. Outputs: `shots/tests/<mode>_*.png|wav|txt`, relative to the working directory; print what a reader checks (counts, names,
   the files written). A mode that draws bodies waits for their maps first (`game.testWaitMapOf(i)`, `mapIfReady`).
4. If it belongs in the proof, a line in `tools/prove.items` (the `prove` skill: its expected files, masks for its timings).

## A unit check

1. Find its group (`unit list`; the groups' files are `unit_*.cpp`, the peoples' `*Checks` in their `civ_*` files) and put the
   check in the group's function, in a block of its own with its id and what it asserts: `check("name", ok, detail)`.
2. A check over a region states the count it expects, so a thinner galaxy fails instead of testing less. A check whose world
   or place is pinned takes it from `test_worlds.h`.
3. A new group: its function in the right `unit_*.cpp`, declared in `harness.h`, and a row of `UNIT_GROUPS` (`unit.cpp`) in the
   unit's order with the scale and mush the groups before it leave (a scripted game leaves 4x and mush 3; `fprintf` the two
   globals at each group's start in a full run to see them).
4. Run it alone first, quietly: `./vesperis_test unit <group> -q` (only failures and the count); then the whole unit once,
   under `caffeinate -i`, alone. `tools/unitchecks.py` writes `docs/reference/harness/unit-checks.md` again (`make check`
   compares).

## A scene

A row of `CMP_SCENES` (`scenes_table.h`) with its `SceneCode` (`harness.h`; append a code, never renumber one). A new code
needs its finder in `setupSceneForType`'s chain (a `SceneFind` member when it is longer than a few lines) and, below
`SCENE_STAINED_CRACK`, an exclusion from the trait family's range. Check it by eye (`look` skill) and by `scene <name>`'s print.

## Traps

- The unit and the flow both write `shots/tests/test_guide.txt`: run them one after the other. No `make` or `git stash` while
  a run is in flight. A `Game`'s settings set the global render scale (4x): set what a check needs, don't inherit it.
- One `Game` alive at a time; keep the flow's steps before its probe block (game time shifts the later frames).
- Pictures that print a time (landforms, the towns' map) differ between runs; mask or skip them in an item.
- No dynamically initialised global, nothing self-registering: link order must not move start-up.
