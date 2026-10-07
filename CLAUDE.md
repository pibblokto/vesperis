# Vesperis

A first-person, goal-less space exploration game in C++17, software-rendered at 320x200
(scaled 1x-4x) through palette intensity ramps and a smoothing filter; raylib is only the
window, input and audio layer. Heavily inspired by Noctis IV. Everything is deterministic
from hashed coordinates: the same star, planet and square metre of ground is always the same.

## Build and verify

```
make -j8                 # ./vesperis (the game) and ./vesperis_test (headless harness); needs raylib 5.x (brew install raylib)
./vesperis_test unit     # unit tests
./vesperis_test flow     # a scripted expedition; frames land in shots/tests/flow_*.png
./vesperis_test regress  # 22 determinism and frame hashes against tests/regress_baseline.txt
./vesperis_test          # lists every mode (surface, space, maps, bench check, terrain, landforms, scene <name>, spot ...)
```

* Verify headlessly: the harness dumps PNGs into `shots/tests/` (gitignored); open them with
  the Read tool, at most ~8 images per response. `shots/` itself is for the player's screenshots.
* `regress bless` only after a deliberate change of what is generated or drawn; bump
  `GEN_VERSION` in `src/galaxy/system.h` when the generation changes (saves and pinned
  test sites move with it) and note it in `docs/reference/10-decisions.md`.
* `VESPERIS_TRACE=1` prints init stages and frame sections; `VESPERIS_THREADS=1` is the serial baseline.
* No cmake binary on the dev Mac (CMakeLists.txt is for other platforms); no `timeout`
  command (run long harness modes in the background with the output in a file).
  zsh does not word-split unquoted variables (`for m in "fuzz 1500"` is one argument).

## Layout

`src/core` (framebuffer, palette, rasteriser, font, noise, PNG, input), `src/galaxy` (star
field and classes, system generator, the planet function, drainage, landmarks, maps),
`src/space` (suns, globes, rings, sky bodies), `src/surface` (landing site, terrain rings,
water, flora, creatures, buggy), `src/game` (states, autopilot, HUD, menus, guide, save/load,
settings, audio), `src/platform_raylib.cpp`, `src/main_headless.cpp` (the harness).

## Rules

* Never include `raylib.h` in game code: it defines `PI` and `KEY_*` names that clash; the
  platform layer behind `src/platform.h` is the only place.
* One code path: no prototype or HD variants lingering in the tree; experiments live in a
  scratch build and are removed once the decision is recorded in `10-decisions.md`.
* Names: nothing has a name until the explorer gives one (`starNameOf`, `bodyNameOf`
  return the guide's name or UNKNOWN; generated names stay internal).
* Maps show only what the explorer marked (`guide.marks`); the scanner hints, it never names.
* Keep the UI terse: the title screen is the name and the choices; no feature blurbs.
* Work per milestone and wait for the user's "proceed" before starting the next one.

## Docs

`docs/README.md` explains the workflow: bug reports in `docs/iteration-3/bugs/` (`B-NNN`),
requests in `docs/iteration-3/plans/requests/` (`R-NNN`), roadmap and `PROGRESS.md` there,
reference notes in `docs/reference/` (read `10-decisions.md` before changing a rule).
`docs/ideas/` holds the discussed-but-unscheduled ideas and the two generation plans
(galaxy scale, star classes: one generation bump together, before players keep saves).

## Release

Tag `vX.Y.Z`, release through `gh api`, six zips (macOS arm64/x86_64 with `Vesperis.app`,
Linux x86_64/arm64 built on Ubuntu 20.04, Windows x86_64/arm64 with llvm-mingw), raylib 5.5
built from source per target. Version lives in `CMakeLists.txt` and `packaging/make_app.sh`.
Commits in this repository carry a randomised author, not the user's identity.
