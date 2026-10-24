# Vesperis

A first-person, goal-less space exploration game in C++17, heavily inspired by Noctis IV: software-rendered at 320x200
(scaled 1x-4x) through palette intensity ramps and a smoothing filter; raylib is only the window, input and audio.
Everything is deterministic from hashed coordinates: the same star, planet and square metre of ground is always the same.

## Build and run

```
make -j8                      # ./vesperis (the game) and ./vesperis_test (the harness); raylib 5.x (brew install raylib)
make check                    # the layout: raylib only in the platform, the layering, no line over 2,000, the maps true
./vesperis_test unit          # the unit checks: a few minutes at ~280% CPU
./vesperis_test regress       # 24 determinism and frame hashes against tests/regress_baseline.txt
./vesperis_test flow          # a scripted expedition: read its status lines, not only the exit code
./vesperis_test bench check   # the frame's time budgets: alone, under caffeinate -i
```

* No argument runs `stars` (the star statistics) and an unknown mode exits 0: there is no mode list. The modes are
  described in `docs/reference/08-testing-and-tools.md`; the harness's `main()` dispatches them.
* Outputs land in `shots/tests/` (gitignored, ~2 GB: never copy it whole; the `look` skill reads them); `shots/` itself
  holds the player's screenshots.
* `VESPERIS_TRACE=1` prints init stages and frame sections; `VESPERIS_THREADS=1` is the serial baseline.

## Where things are

```
src/core        framebuffer and palette, raster, font, noise, rng.h (the hashes), png, input, parallel (the pool)
src/galaxy      the worlds: starfield, system (GEN_VERSION), kinds, planetmap (the planet function), drainage,
                landmarks, almanac, nights, fronts, probe, drifters; the peoples: peoples, history, plan, towns,
                works, buildings, ruins, forms, genome, records, shards, genres, grammar (+ grammar/*.vg),
                tongue, voice, music, charts, instruments, signals, drones
src/space       space_view (stars, suns, globes, belts), probe_view
src/surface     site (the landing site), surface_view (sky, terrain, water, walking), flora, creatures, buggy
src/game        Game (game.h), game.cpp's frame, one file per screen or device, audio, save and load
src/main_headless.cpp   the harness (every mode and the unit), 2 MB: grep -n and read ranges, never whole
src/harness     the harness's own files (the fingerprints); E-03 moves main_headless.cpp here
src/main_raylib.cpp, src/platform_raylib.cpp   the window, input and audio: the only raylib code
tests/ packaging/ music/   the regress baseline; the release builds (RELEASE.md); the tape's tracks
tools/          prove (the proof battery, prove.items), its helpers, png.py and wav.py (pictures and sounds), check.py
docs/           plans, bugs, requests, reference notes, the decisions log: see docs/CLAUDE.md
build/          objects (all `make clean` removes), prove/ (the snapshots), toolchains/, perf-baselines/, old kits
```

Each source directory's CLAUDE.md names its files and their traps (the harness's: `.claude/rules/harness.md`), and
`docs/reference/lessons.md` holds the lessons behind them by area.
The E series (`docs/ideas/PLAN-reorganisation.md`) reorganises this tree in 1.4.0, and the maps change with it.

## Rules

* Never include `raylib.h` in game code (its `PI` and `KEY_*` names clash): `src/platform.h` is the boundary.
* One code path: no prototype or HD variants lingering; experiments live in a scratch build and go once the decision is
  recorded in `docs/reference/10-decisions.md`.
* Nothing has a name until the explorer gives one (`starNameOf`, `bodyNameOf` return the guide's name or UNKNOWN).
  Maps show only what the explorer marked (`guide.marks`); the scanner hints, it never names. The UI stays terse.
* A change to what is generated or drawn bumps `GEN_VERSION` (`src/galaxy/system.h`) once per series, is noted in
  `10-decisions.md` and `reference/generations.md`, and only then `regress bless`. Persisted enums, drawn tables and salts are append-only.
* Before changing a rule, read its section of `10-decisions.md` (`grep -n '^## '` is the index; never read it whole).

## Proving a change

* `tools/prove compare <base> [items]` runs the battery (`tools/prove.items`: the fingerprints, the regress both ways,
  every scene and screen, the sweeps, every WAV, the unit, the flow) against the snapshot `build/prove/<base>`; `objects`
  for a comment-only step, `bench` for a hot path. The skills `prove`, `bench` and `look` say how; `prover` runs it.
* The unit and the flow both write `shots/tests/test_guide.txt`: run them one after the other (the flow repeats only
  from the same start). Never `make` or `git stash` while a harness run is in flight.
* Long runs: `caffeinate -i`, in the background, output to a file, and alone (the Mac sleeps while the user is away;
  frames and timings taken beside other jobs move).

## Working with the user

* One milestone at a time: the next starts on the user's "proceed". The plan's defaults settle open questions; ask only
  what blocks, one question with a recommendation. A short status line when the user has not heard from you a while.
* Ask before Docker (it starts the user's own containers) or opening the game window; a milestone's own `bench check`
  runs without asking.
* Nothing is committed unless asked. Commits and tags carry a random author and committer (an 8-hex name, a
  12-hex@example.com email), never the user's identity, and no session link: the guards refuse one.
* The report at the end: what changed and where, what was proven, what the user decides next.

## Recording work

What each milestone, bug or request writes, and where: `docs/CLAUDE.md`.

## Shell and machine

* zsh: unquoted variables are not word-split (use arrays or `${=w}`); quote a word that starts with `=`.
* No `cmake` (CMakeLists.txt serves the other platforms), no `timeout`, no numpy (`tools/wav.py` reads WAVs).
* `.claude/hooks/guard.py` refuses a whole Read of a file over 100 KB, a session link in a commit, tag, PR or
  release, `rm -rf build`, and a `make clean` whose Makefile would take build/ whole (a checkout before E-02).

## Release

Six zips (macOS, Linux and Windows, arm64 and x86_64) released through `gh api` from a tag `vX.Y.Z`; the version lives in
`CMakeLists.txt` and `packaging/make_app.sh`; the steps and traps: `packaging/RELEASE.md` (Linux needs Docker: ask).
