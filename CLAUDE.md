# Vesperis

A first-person, goal-less space exploration game in C++17, heavily inspired by Noctis IV: software-rendered at 320x200
(scaled 1x-4x) through palette intensity ramps and a smoothing filter; raylib is only the window, input and audio.
Everything is deterministic from hashed coordinates: the same star, planet and square metre of ground is always the same.

## Build and run

```
make -j8                      # ./vesperis (the game) and ./vesperis_test (the harness); raylib 5.x (brew install raylib)
make check                    # the layout: raylib only in the platform, the layering, the maps and generated docs true
./vesperis_test help          # every mode with its arguments and its file
./vesperis_test unit          # the unit checks: a few minutes at ~280% CPU; `unit list`, `unit <group> -q`
./vesperis_test regress       # 24 determinism and frame hashes against tests/regress_baseline.txt
./vesperis_test flow          # a scripted expedition: read its status lines, not only the exit code
./vesperis_test bench check   # the frame's time budgets: alone, under caffeinate -i
```

* No argument runs `stars` and an unknown mode exits 0. The modes are `MODES` in `src/harness/main.cpp`, listed in
  `docs/reference/harness/INDEX.md` (generated) and described at length in `docs/reference/08-testing-and-tools.md`.
* Outputs land in `shots/tests/` (gitignored, ~2 GB: never copy it whole; the `look` skill reads them); `shots/` itself
  holds the player's screenshots.
* `VESPERIS_TRACE=1` prints init stages and frame sections; `VESPERIS_THREADS=1` is the serial baseline.

## Where things are

```
src/core        framebuffer and palette, raster, font, noise, rng.h (the hashes), png, input, parallel (the pool)
src/galaxy      the worlds: starfield, system (GEN_VERSION), kinds, planetmap (the planet function), drainage,
                landmarks, almanac, nights, fronts, probe, drifters
src/peoples     the peoples: peoples, history, plan, towns, works, buildings, ruins, forms, genome, records, shards,
                genres, grammar (+ grammar/**/*.vg), tongue, voice, music, charts, instruments, signals, drones
src/space       space_view (+ _sun, _globe, _bodies), probe_view
src/surface     site, surface_view (+ _env _places _walk _palette _terrain _sky _effects), flora, creatures, buggy
src/game        Game: game.h, game_types.h, *_state.h; a file per screen or device, audio, saves
src/harness     vesperis_test: main.cpp's MODES (`help`), modes_*, civ_*, unit_* (the unit), scenes_*, test_game (TestGame)
src/main_raylib.cpp, src/platform_raylib.cpp   the window, input and audio: the only raylib code
tests/ packaging/ music/   the regress baseline; the release builds (RELEASE.md); the tape's tracks
tools/          prove (the proof battery), its helpers, png.py and wav.py, check.py, mover.py and samecode.py (moves proven)
docs/           STATUS.md (the state), decisions/ (the rules, the log), reference/, bugs, requests, ideas/, archive/
build/          objects (all `make clean` removes), prove/ (the snapshots), toolchains/, perf-baselines/, old kits
```

Each source directory's CLAUDE.md names its files and their traps, and `docs/reference/lessons.md` holds the lessons
behind them by area.
The E series (`docs/ideas/PLAN-reorganisation.md`) reorganises this tree; the maps change with it.

## Rules

* Never include `raylib.h` in game code (its `PI` and `KEY_*` names clash): `src/platform.h` is the boundary.
* One code path: no prototype or HD variants lingering; experiments live in a scratch build and go once the decision is
  recorded in the decisions log.
* Nothing has a name until the explorer gives one (`starNameOf`, `bodyNameOf` return the guide's name or UNKNOWN).
  Maps show only what the explorer marked (`guide.marks`); the scanner hints, it never names. The UI stays terse.
* A change to what a star, a system, a body or a surface is bumps `GEN_VERSION` (`src/galaxy/system.h`) once per series,
  with its row in `docs/decisions/generations.md`; a `regress bless` comes after the bump, or for a drawing change the log
  records. Persisted enums, drawn tables and salts are append-only (`rules-galaxy.md`).
* Before changing something, read its area's rules: `docs/decisions/rules-<area>.md` (each under 6 KB). The entries behind
  them are in `docs/decisions/log/`: grep the id, never read a log whole.

## Proving a change

* `tools/prove compare <base> [items]` runs the battery (`tools/prove.items`: every output the game makes, the unit, the
  flow) against the snapshot `build/prove/<base>`; `objects` for a comment-only step, `bench` for a hot path. The skills
  `prove`, `bench` and `look` say how (`docs/reference/harness/proof.md`); the `prover` agent runs it.
* The unit and the flow both write `shots/tests/test_guide.txt`: run them one after the other (the flow repeats only
  from the same start). Never `make` or `git stash` while a harness run is in flight.
* Long runs: `caffeinate -i`, in the background, output to a file, and alone (the Mac sleeps while the user is away;
  frames and timings taken beside other jobs move).

## Working with the user

* One milestone at a time: the next starts on the user's "proceed" (the `milestone` skill; `docs/STATUS.md` names it). The
  plan's defaults settle open questions; ask only what blocks, one question with a recommendation. A short status line
  when the user has not heard from you a while.
* Ask before Docker (it starts the user's own containers) or opening the game window; a milestone's own `bench check`
  runs without asking.
* Nothing is committed unless asked. Commits and tags carry a random author and committer (an 8-hex name, a
  12-hex@example.com email), never the user's identity, and no session link: the guards refuse one.
* The report at the end: what changed and where, what was proven, what the user decides next.

## Recording work

What each milestone, bug or request writes, and where: `docs/CLAUDE.md` (the skills `milestone`, `bug` and `release`).

## Shell and machine

* zsh: unquoted variables are not word-split (use arrays or `${=w}`); quote a word that starts with `=`.
* No `cmake` (CMakeLists.txt serves the other platforms), no `timeout`, no numpy (`tools/wav.py` reads WAVs).
* `.claude/hooks/guard.py` refuses a whole Read of a file over 100 KB, a session link in a commit, tag, PR or
  release, `rm -rf build`, and a `make clean` whose Makefile would take build/ whole (a checkout before E-02).

## Release

The `release` skill: six zips (macOS, Linux and Windows, arm64 and x86_64) from a tag `vX.Y.Z`, the steps and traps in
`packaging/RELEASE.md` (Linux needs Docker: ask).
