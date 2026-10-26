# Plan: the repo reorganised for its readers (the E series)

Written 2026-10-23 on the user's request, the day 1.4.0's sixteen milestones were committed (92e580a): "go through the repo
and analyze how it can be optimized for coding agents to let use up less context and to make code more modular, readable,
clean", with more CLAUDE.md files, "map where is what", the skills that would help (existing ones and our own), and "a plan
with milestones for reorganizing things, splitting them", done in this release before the next features.

How it was made:
- **Five readers surveyed the repo:** the harness, the engine with the grammar, the game layer, the docs with the memory, and
  Claude Code's own documentation.
- **The deciding claims were measured again:** compile times, object identity, the almanac's start-up order, the include graph.
- **A sceptic reviewed the draft against the tree** and found sixteen corrections, all taken.

**The short version.** Nothing is wrong with what the game does; the problem is what an agent must read to change it.
- **The harness** is one 2.1 MB file, a quarter of all the C++. A whole Read of it stops at the tool's 25k-token cap after
  about 1,000 lines, 4% of it. Any edit to it recompiles for 35-39 s on one core.
- **game.h** is 129 KB, and a quarter of it is harness hooks.
- **The rules** an agent is told to read before changing anything sit in a 667 KB log with no index.
- **The memory:** every session loads a 3.5 KB line of 1.3.0's history, and a milestone recalls 140 KB more, most of it in
  lines too long for the Read tool.
- **The proof:** every milestone builds its proof again by hand in gitignored `build/<id>/`. There are 20 copies of
  `verify.sh` and 10 of the fingerprint tool, 9.4 GB in all, and `make clean` would delete them with the toolchains.

The plan cuts this down in thirteen milestones, E-01 to E-13. Nothing the game makes, draws, plays or saves changes in any of
them: no generation bump, no bless, and every step proven identical by a proof kit that E-02 makes permanent.

## 1. Where it stands, measured (2026-10-23, at 92e580a)

| What | Size | What it costs an agent |
| --- | --- | --- |
| Loaded every session: CLAUDE.md + MEMORY.md | 5.7 + 6.5 KB | ≈ 3k tokens; MEMORY.md's line 3 is 3.5 KB of 1.3.0's history |
| Memory recalled on a milestone | milestone-workflow.md 140 KB (102 KB of it in 30 lines over 2,000 characters), release-1-4-0.md 41 KB | ≈ 45k tokens, and the Read tool cuts the long lines; 252 KB in 12 files, mostly logs the repo already keeps |
| The harness, `src/main_headless.cpp` | 25,399 lines, 2.12 MB, 83 modes | a whole Read stops at 25k tokens, 4% in; `testUnit` is one 6,482-line function |
| Files over 100 KB, each a full 25k-token Read that shows only part of it | src: harness 2,124; buildings 339; planetmap 326; surface_view 323; history 238; works 168; kinds 159; ruins 146; instrument_plans 135; space_view 134; game.h 129; music, forms, genres, grammar, autoexplore 101-109. docs: 10-decisions 667; PROGRESS 309; 12-civilisations 294; 08-testing 232; PLAN-performance 131; 07 130; 06 123; 05 110; KNOWN-ISSUES 105 | |
| `src/game/game.h` | 1,410 lines, 129 KB: 41% comments, 333 lines of harness hooks | read for every game task; 26 of the 34 game files include it |
| Lines the Read tool cuts off (over 2,000 characters) | `system.h:12` (5,860, the GEN_VERSION history), `shards.cpp:401` (2,575, the GRAMMAR_FILES row) | the line an agent must edit to add a grammar file cannot be read whole |
| Docs | 164 files, 3.4 MB; 99 lines over 2 KB (393 KB) | one grep can return a 42 KB line |
| Writing per milestone | ≈ 23 KB of docs and 2.6 KB of memory | D-05 is told in 14 files (≈ 41 KB), its story three times |
| The proof per milestone | made again in `build/<id>/` each time | none of it exists in a fresh worktree |
| Wrong guidance | CLAUDE.md's "`./vesperis_test` lists every mode" (it runs `stars`); docs/README.md's iteration 1 and generation 6; R-421/R-422 statuses | an agent trusts it |

Eight everyday tasks, what an agent reads before it can act. These are the survey's estimates; E-01 measures them as subagent
runs, and E-13 runs the same prompts again.

| Task | Today | After the series |
| --- | --- | --- |
| Add a unit check | ≈ 180 KB: greps of a 2 MB file, ~1,000 lines round `testUnit`, the unit's 96 KB printout | ≈ 30-60 KB: one group's file, `unit <group> -q` (≈ 2 KB) |
| Add a harness mode | ≈ 110 KB: main's 635-line dispatch, a neighbour, 08's table | ≈ 40 KB: the registry, one domain file; the index regenerated |
| Trace a radar path (the lock to the speech) | ≈ 185 KB: game.h and radar.cpp | ≈ 50-80 KB: `radar_state.h`, a slim game.h, radar.cpp |
| Add a game screen | ≈ 240 KB; 6-9 files, about 14 places | ≈ 100 KB: the state table and a checklist |
| Find the rule before changing something | 40-80 KB of decision sections | 4-6 KB: `rules-<area>.md` |
| Prove a milestone unchanged | ≈ 100-150 KB: write verify.sh, build the fingerprint tools, read 30 outputs | ≈ 2 KB: `tools/prove compare`, run by the prover agent |
| Start a milestone | ≈ 200 KB: memory, PROGRESS's Next, the plan | ≈ 15 KB: STATUS.md, the milestone's section, the milestone skill |
| Change the records' grammar | ≈ 60-120 KB: find the file among 107, the cut-off GRAMMAR_FILES line, the unit's printout | ≈ 25 KB: `grammar where`, the shelf's file, `unit grammar -q` |

### What a move can break (each checked against the tree)

1. **The almanac's start-up order.** `fronts.cpp:70` and `nights.cpp:69` add their finders from global constructors, so link
   order decides the order of equal-time events after `almanacOf`'s stable sort. Renaming, splitting or moving a galaxy file
   could flip it. E-02 makes the order explicit before anything moves.
2. **Draws in one expression.** About 120 statements draw twice from one `Rng` in one expression, whose evaluation order C++
   leaves open (clang goes left to right). They are left textually untouched: a helper extracted round one keeps the order.
3. **Rng copies.** `Rng` is copyable, so a helper that takes it by value forks the stream. Extracted helpers take `Rng&`, and
   a scratch build with the copy constructor deleted lists every copy.
4. **Floating point.** Clang (the Mac and Windows builds) decides fused multiply-adds per expression, so moving whole functions
   between files changes no bit. Changing an expression's shape can: wrapping `a*b` in a helper, adding a temporary, merging
   near-duplicates.

   GCC 9 at -O3 (the Linux builds) fuses after inlining, so splits and link-order changes can move Linux bits. In F-09 the
   arm64 container's GCC reproduced the Mac's 22 regress hashes exactly; the amd64 container differed in six, as the x86_64
   Mac does in three.

   The release scripts never build the harness. So after E-10 (the last split that moves inlining and link order), one Docker
   session (asked first) builds the old snapshot's tree and the new tree in the arm64 container, blesses the old one into a
   copy, and runs the new one against it, with the workers and with one thread.
5. **State that belongs to one file.** Stateful statics, `thread_local` guards (`planetmap.cpp`'s `t_noBowls`) and
   function-local statics stay in one file. They are never copied into a shared header, where each file would get its own.
6. **Persisted enums and member order.** `GameState`, `ShipState::Mode` and `AutoProfile` are saved as numbers, so they are
   append-only. Data members are never reordered (the initialisation and destruction order of threads and caches).
7. **Run orders.** The unit's groups and every mode's printout keep today's order and flushing. Registries are explicit
   tables, never self-registering constructors.
8. **Link order sets the order of destruction at exit too.** `drainage.cpp:464`'s `Reaper` joins its threads at exit. Moving
   modules into a new directory moves their objects after it, so E-09 runs the unit and the flow once under ThreadSanitizer.
9. **Near-duplicates that differ.** `plan.cpp`'s `toward` clamps and `works.cpp`'s does not; there are two `bearingOf`s. They
   are merged only as named variants.
10. **The grammar.**
    - No file name or position enters a hash, but an alternative's key hashes its line's text. So a `.vg` line is never
      rewrapped.
    - `@apart` works per file and a rule cannot span files, so files move whole, in GRAMMAR_FILES' order.
    - File names are printed (the unit's "the largest" line, `grammar where`'s file:line lists), so E-11's renames are expected
      changes there, compared through a rename map.
    - The harness hardcodes `"src/galaxy/grammar"` five times. Two of its checks pass without reading anything when the folder
      moves: one skips files it cannot open, the other lists an empty directory. So they are made to require every file.
11. **The frame's order and resets by assignment.** The frame runs `padFrame` … `autoRender` in a fixed order, and five
    structs are reset by assigning a fresh one (`radar=`, `probe=`, `bay=`, `icase=`, `tele=`). A field moved into one of them
    gains a reset; a thread moved into one would terminate the game.
12. **The builds list their directories.** The Makefile globs one level per directory. `CMakeLists.txt` lists `CORE_SOURCES`
    and the `-Werror` `STRICT_SOURCES`. The release scripts (`build-mac.sh:13`, `build-linux-inner.sh:17`, `build-win.sh:14`)
    each `find` named directories. A new engine directory must go into all of them, or the release link fails at release time
    and 24 modules lose `-Werror`. The harness's directory never enters a release build.
13. **What the proofs do not cover yet.** Regress covers 5 systems, 10 maps, 3 sites and 6 frames. The unit's pinned hash
    covers the age and scene worlds' histories, record catalogues, chart marks and texts. Nothing pins the buildings, ruin
    layouts, works, roads, plan, music, instruments, form colliders or tongues. The battery compares the radar's WAVs but not
    the music, voice, medium, instrument, sound, deck or probe WAVs, nor the saves, guides and settings the flow and the
    regress write. E-02 closes all of that before anything moves.
14. **The old kits' traps.**
    - The kits link `build/<dir>/*.o`. If make ever wrote its objects elsewhere, they would prove the old code against itself.
    - `perf-baselines/tools/flowrun.sh` hardcodes the repo's path and `rsync --delete`s into `shots/tests`.
    - The harness reads `tests/regress_baseline.txt` and the grammar relative to its working directory.
15. **Speed.** The dev build has no LTO, so a small static helper moved out of its file stops being inlined. Helpers the split
    shares go into the module's internal header as `inline`, and hot-path splits get bench pairs.

**Measured for this plan:**
- **Engine objects are reproducible.** Compiling an engine file twice gives byte-identical objects, the same as make's
  (`buildings`, `history` and `surface_view` tried). Nothing in src uses `-g`, `__FILE__`, `__LINE__`, `assert` or
  `__DATE__`. So a comment-only or whitespace-only change in the engine or the game is proven by comparing objects.
- **The harness object is not.** Two compiles of the harness differ in 21 bytes (the memory's "testUnit compiles
  nondeterministically"). It is proven by behaviour, and tried again once E-03 has split it.
- **Compile times.** The harness takes 35-39 s on one core; a 339 KB engine file takes 1.2 s.
- **The Read tool's limits.** A Read without a range stops at 25k tokens and cuts lines over 2,000 characters (the sceptic's
  test on the 130 KB `07-game-flow` doc: 69 of its 356 lines).
- **src/galaxy's two halves.** The civilisation half (24 modules, 31.3k lines) uses the worlds half (12 modules, 12.9k
  lines). Only two world files use the civilisation half: `landmarks.cpp` (peoples, plan, ruins, works) and `planetmap.cpp`
  (peoples).

## 2. Targets

- **Always loaded:** at most 8 KB per session (CLAUDE.md ≤ 6.5 KB with the map, MEMORY.md ≤ 1.5 KB).
- **Source files:** none over 1,500 lines or 120 KB, apart from those named in the map with a reason: kinds.cpp (one table) and
  instrument_plans.cpp (independent functions).
- **Harness functions:** none over 400 lines, apart from the unit groups' bodies, which are lists of independent check blocks.
- **game.h:** at most 45 KB.
- **The unit:** one group runs alone by name, and prints only its failures with `-q`.
- **Rules:** any rule found in 6 KB or less; every harness mode in one generated index.
- **The proof:** two commands and a summary of 30 lines or fewer, run by a subagent.
- **Doc writing:** at most 8 KB per milestone, in no more than six places.
- **Rebuilds:** a harness edit rebuilds in 5 s or less.
- **Navigation:**
  - every source directory has a CLAUDE.md of at most 3 KB naming each of its files, enforced by `make check`;
  - the LSP answers definitions, references and a file's symbols.
- **"Level" for the bench:** every row's best-of ratio within 0.97-1.03 over four pairs, with the noisy rows named (the
  buggy's 2x).

## 3. The rules every milestone keeps

1. **Nothing the game makes, draws, plays or saves changes.**
   - GEN_VERSION stays 16, there is no `regress bless`, and the save format and every persisted enum stay as they are.
   - Each step is proven against the previous snapshot by the proof kit (section 5).
2. **Moves and edits are separate steps.**
   - A move step moves whole functions, tables and their comment blocks, in their original order, and nothing else. A mover
     script's `--verify` reassembles the old file from the new ones to prove it.
   - Edits (helpers, registries, tables) come after, one at a time, each with its own equivalence check.
3. **Whole functions move between .cpp files.**
   - No function that was out of line becomes inline in a header.
   - A file-local helper that the split shares goes into the module's internal header as `inline` (it was inlinable before).
   - No new dynamically initialised global, and no self-registering constructor.
4. **Append-only stays append-only.** Persisted enums, drawn tables, salts, and grammar rule names and lines are never renamed
   or reordered when their name or position enters a hash.
5. **Each milestone leaves its directories' CLAUDE.md and the root map true** (`make check` holds it from E-02). It records
   itself in the touch points of the day: today's until E-04, the lean set after it.
6. **Workflow.**
   - One milestone at a time; the next waits for the user's "proceed".
   - Ask before Docker or the game window.
   - Long runs go under `caffeinate -i` in the background, with output to a file, and alone.

## 4. The milestones

| # | Id | Item | Size |
| --- | --- | --- | --- |
| 1 | E-01 | What every session loads: the memory, the root CLAUDE.md with the map, the directory notes, the guards; the eight tasks measured | M |
| 2 | E-02 | The proof kept: fingerprints of every module, `tools/prove`, the almanac's order explicit, a safe `make clean`, `make check`, clangd | L |
| 3 | E-03 | The harness in pieces: `src/harness/`, a mode registry, the unit by groups, the scenes by family | L |
| 4 | E-04 | The docs for their readers: rules by area, the log by release, STATUS, the harness index, an archive, lean touch points | L |
| 5 | E-05 | The game's header: the hooks into the harness, game.h by subsystem, the platform's asserts | L |
| 6 | E-06 | The game's files: the mixed files split, the state table, the new-screen checklist | M |
| 7 | E-07 | The worlds: planetmap split, the engine headers' hook sections, `salt scan` | L |
| 8 | E-08 | The views: surface_view and space_view split | L |
| 9 | E-09 | The peoples' directory: `src/peoples/` with its grammar; buildings, works and ruins split | L |
| 10 | E-10 | The peoples' texts: history, genres, grammar.cpp, forms and music split, `lore.h`; the Linux check | L |
| 11 | E-11 | The grammar's shelves: a manifest, folders by role, files named by topic, `grammar where` | M |
| 12 | E-12 | The sweep: shared helpers, dead code, comments by the policy, long lines, the warnings | M |
| 13 | E-13 | The close: the CLAUDE.md tree audited, the eight tasks measured again, the old kits | S-M |

A fourteenth, offered but not planned: **E-14, the game's leaf subsystems as classes.** Tape, almanac, player and marks would
take a `Game&`, piloting "more modular" beyond files for the one class that stays a god class. It is the user's call.

**Why this order:**
- **E-01:** every session after it is cheaper, its guards protect the rest, and it touches no code.
- **E-02:** everything after it is proven with its kit.
- **E-03:** the harness is the largest single cost, and E-05's hooks move into it.
- **E-04:** its rules index, its touch points and its skills serve every milestone after it.
- **E-05 to E-11:** they follow how often their areas change. The game and the views are the hottest (`surface_view.cpp` is in
  15 of 31 commits, `planetmap.cpp` in 8); the civilisation half changed least.
- **E-10 before the Linux check:** it is the last milestone that moves inlining or link order.
- **E-12:** it sweeps what the moves leave.
- **E-13:** it measures the result.

### E-01. What every session loads (M; no code)

**The memory:**
- **MEMORY.md** becomes an index of about six lines, ≈ 1 KB:
  - the user's workflow;
  - the release identity;
  - no session links;
  - the bench in the milestone;
  - the screenshot's world;
  - "the state: STATUS; the history: the docs and git".
- **milestone-workflow.md (140 KB, read through Bash: 30 of its lines are over the Read tool's limit).** Its ≈ 12 KB of
  standing rules (the touch points, the harness traps, the generation rules, the screen, audio and font rules, the text, music
  and speech lessons, the zsh traps) go to the CLAUDE.md files and a `user-workflow.md`. Its history (1.1.0 to 1.3.0, already
  in the repo) goes.
- **The other files:**
  - `release-1-4-0.md`: its eleven lessons move out, and its log goes (PROGRESS and ITERATION-1.4.0 hold it); it keeps the
    state.
  - `performance-plan.md`: its lessons move to the proof notes.
  - `release-recipe.md`: keeps ≤ 3 KB of account and identity rules; its steps go to `packaging/RELEASE.md`, which E-04's
    release skill points to.
  - `release-1-3-2.md`, `bug-fix-recording.md`, `harness-caffeinate.md` and `dark-frames-measure.md`: folded in and deleted.
- **A copy of the old memory directory** is kept first in `build/memory-2026-10-23/` (gitignored, never loaded).

**The root CLAUDE.md** is rewritten: at most 150 lines and 6.5 KB.
- Sections: what Vesperis is; build and run; the map (section 7); the rules; proving unchanged; working with the user;
  recording work; shell and machine; release.
- It corrects the mode-list claim: no argument runs `stars`, and an unknown mode exits 0.

**The directory notes** (≤ 3 KB each, contents in section 7): src/core, src/galaxy, src/galaxy/grammar, src/space, src/surface,
src/game and docs.
- As built: src/galaxy's and src/game's notes are about 6 KB (36 modules and 39 files), until E-06 and E-09 split them.
- The harness's traps go in a path-scoped rule, `.claude/rules/harness.md` (`paths: src/main_headless.cpp`), until E-03 makes it
  `src/harness/CLAUDE.md`: a `src/CLAUDE.md` would load with every source file.
- Each holds a one-line-per-file map of what is there today, and its invariants and traps.
- The maps change as each area is split.

**The guards** are hooks in `.claude/settings.json` (section 6), in place before the code moves.

**The eight tasks measured.** Section 1's tasks run as read-only subagent prompts ("plan this change: name the files and
lines; don't edit"), and each run's tokens are recorded. The prompts are kept, so E-13 runs them again unchanged.

**Asked of the user:**
- standing leave to commit each step (section 8);
- installing the clangd-lsp plugin (for E-02) and, if wanted, session-report;
- a new session after the hooks land, because hooks and plugins are read at a session's start.

**Done when:**
- MEMORY.md is at most 1.5 KB and 8 lines; the memory is at most 10 KB in 7 files or fewer.
- CLAUDE.md is at most 6.5 KB.
- Each of the memory's 78 "lesson" lines has found a home (a checklist in the decision entry).
- In a new session, the hooks refuse a whole read of the harness and a commit whose message carries a session link.
- The eight tasks' baseline is recorded.

**Done 2026-10-23:** the log's E-01 entry (`docs/decisions/log/6-1.4.0.md`); the baseline is section 11.

### E-02. The proof kept (L)

In this order, so that nothing is ever proven against stale objects:

1. **The fingerprint modes and `tools/prove`.** Only the harness changes; the library's objects are untouched.
   - The old kit is run once more (D-05's `verify.sh` against D-05's outputs) to prove this binary.
   - The new `fingerprint bodies` and `fingerprint peoples` are checked against fp5's and recdump's values for the same worlds.
2. **`prove snapshot e02-base`.**
3. **The almanac.**
   - findFronts and findShowers get external linkage (declared in `fronts.h` and `nights.h`) and are listed in `registry()` in
     today's order (fronts, then showers).
   - The two registrars go.
   - Proven by `prove compare e02-base`: the almanac's sample, the regress.
4. **The build.**
   - `make clean` removes only the object directories derived from the source directories, plus the top-level objects and the
     binaries. It is tried in a scratch worktree first.
   - The objects stay where they are, so the old kits keep linking the objects they expect.
   - `CPPFLAGS` is split from `CXXFLAGS`, so passing your own flags no longer drops `-Isrc -MMD`.
   - `make check`:
     - no raylib include outside the platform file;
     - the layering: core includes no other directory; galaxy, space and surface include no game/ or platform.h;
     - no line in src over 2,000 characters;
     - every file named in its directory's CLAUDE.md.
   - The two lines over 2,000 characters go:
     - system.h:12's GEN_VERSION history moves to `docs/decisions/generations.md`, a comment-only change proven by
       `prove objects`; E-04 moves that page into `decisions/`.
     - shards.cpp:401 is wrapped, a whitespace-only change.
   - The `lakes` warning is fixed.
5. **clangd.**
   - `compile_flags.txt` is committed: `-xc++`, so headers parse as C++, plus `-std=c++17 -Isrc -pthread` and the warnings.
   - With the user's plugin installed, the LSP answers in a new session.
6. **Skills and agent:** the `prove`, `bench` and `look` skills (with `tools/png.py`: crops, montages, stats) and the
   `prover` agent.

**The kit:**
- **Fingerprint modes** (`fingerprint bodies|stream|peoples|modules [n] [out]`):
  - `bodies`, `stream` and `peoples` promote fp5, the draw-stream tool and recdump.
  - `modules` hashes, for a fixed sample of worlds, what nothing pins today: buildings and ruin layouts, works, roads and the
    plan, music, instruments, form colliders, tongues, flora placements, drainage tiles and the almanac.
  - Lines are keyed (the body or people, then fields by name), so a new field adds a column instead of changing every line.
  - Also `scene list`.
- **`tools/prove`** is one zsh script. Its battery (section 5) is listed once in `tools/prove.items`: each item gives the
  command, its outputs with their expected count, and how to compare them (png, masked text, wav, bytes, keyed).
  - `prove snapshot <name>`:
    - copies the binary into `build/prove/<name>/`;
    - records every item's outputs there, from a scratch working directory with `src/`, `tests/` and `music/` linked in, and
      stdout and stderr kept apart;
    - keeps its own copy of the flow's starting state (not flowrun.sh's hardcoded path and `rsync --delete`).
  - `prove compare <name> [items]`: runs the battery with the current binary and prints ≤ 30 lines.
  - `prove objects <name>`: compares engine and game object files, for comment-only and whitespace-only steps.
  - `prove bench <A> <B> [pairs]`: AB/BA pairs from paths of equal length, best-of and median ratios.
  - `PROVE_HOME` lets a worktree find the main checkout's snapshots.
  - Helpers: `tools/benchcmp.py` and `tools/keydiff.py`.

**Done when:**
- `prove compare e02` from a clean tree reports everything the same.
- `make clean` in a scratch worktree removes only objects and binaries.
- `make check` passes.
- The LSP answers a definition and a symbols request in a .cpp and a .h.
- The skills and the agent are in `.claude/`.

**Done 2026-10-24:** the log's E-02 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- The fingerprint modes are the harness's first own files, `src/harness/` (linked only into vesperis_test). `fingerprint
  bodies` adds to fp5's hash the fields E-01's baseline found it missing (`gen2`, `ground2`) and the drifters nearest home, so
  all 60 kinds; `fingerprint modules` covers 16 peoples' worlds, 22 drained land sites and the almanacs of 102 systems.
- The base is two snapshots: `e02-base`, the tree's binary at the start, and `e02-base2`, e02-base's saved engine objects
  relinked with E-02's final harness (`PROVE_BIN`), which gives the new fingerprint columns a base.
- The flow starts from an empty `shots/tests`: it makes every save it loads, so no saved state is kept. Items carry their own
  masks (`mask=`) and their known noise (`noise=`, with the reason): the unit's six scanner and map frames and its autosave move
  with the pace of the threads it waits on. The flow and the unit run twice at a snapshot.
- The comment-only and whitespace-only steps and the new Makefile came before the almanac, so `prove objects` compared them
  with e02-base's own objects (92 the same). The Makefile takes flags from its command line only (Homebrew's LLVM exports a
  `CPPFLAGS`) and sets `CXX`, whose `?=` never applied.
- The guard refuses a `make clean` only where the Makefile still removes `build/` whole.
- The LSP waits on the user's plugin and a new session; `clangd --check` parses a .cpp and a .h with `compile_flags.txt`.

### E-03. The harness in pieces (L)

**`src/main_headless.cpp` becomes `src/harness/`:** about 36 .cpp files, median ≈ 700 lines.
- **Shared:**
  - `harness.h`: includes, Check, Mode, UnitGroup, the globals, shared declarations.
  - `test_worlds.{h,cpp}`: the AGE_WORLDS, TEST_DRIFTERS and PIN_* tables, and the pinned worlds named.
  - `scenes.h` with `scenes_table`, `scenes_setup` and `scenes_special`.
  - `frames.cpp`, `audio_util.cpp`.
  - `main.cpp`: start-up, the registry, `help`.
- **Mode files:**
  - `modes_galaxy`, `_space`, `_probe`, `_surface`, `_kinds`, `_terrain`, `_signals`, `_game`, `_input`, `_bench`, `_proof`.
  - `civ_forms`, `_genome`, `_walls`, `_plan`, `_works`, `_decoder`, `_instruments`, `_grammar`, `_peoples`. Each keeps its
    `*Checks`.
- **Unit files:** `unit.cpp` plus `unit_basics`, `_kinds`, `_drifters`, `_signals`, `_navigation`, `_civ`, `_sky`, `_proof`.

**The method:**
1. `git mv` alone, proven.
2. `tools/mover.py` moves one domain per step. It is aware of braces and raw strings; a manifest maps each entity to its file;
   whole entities move with their comments, in order; `--verify` reassembles the old file and diffs it.
3. main's inline blocks become functions: scene, capsule, spot, matlod, landmarks, drain, water, climate.
4. `runGameFlow` (595 lines) and `runBench` (529) split into their stages.
5. The registry.
6. The unit groups.

The library's objects never recompile in this milestone, so only the harness can differ.

**The registry** is one explicit table: name, group, usage, help, file, run.
- `help` prints it; `help md` writes the docs' index.
- No argument still runs `stars`, and an unknown mode still exits 0. Changing either is a later, deliberate step.

**The unit:**
- `UNIT_GROUPS` lists 30 groups in today's order. `unit` runs them all, its printout identical but for the timings.
- `unit list`, `unit <names>`, and `-q` (only failures and the summary).
- Blocks that share locals stay together: coldFirst, moonFirst, the basics' Star and StarSystem.

**The scenes:** `setupSceneForType` (1,096 lines, about 60 branches on magic `wantMat` codes from -3 to -105) gets named codes
(an enum with the same values) and one placement function per family, about 20 functions of 30-100 lines.

**Helpers after the moves** (each alone and proven):
- `TestWorld` (62 copies);
- `scanSystems` (97 sector-scan loops, each early exit kept);
- the scripted game's `run` and `press` helpers (110 lambdas in 20 variants, kept as named variants);
- `findScene`;
- one `Check`, keeping both of today's printouts.

**Makefile and CMake:** a HARNESS_SRC glob, with the core objects first in the link.

**Docs and skill:**
- `src/harness/CLAUDE.md`:
  - the registry, the groups, the scene codes and the test worlds;
  - where outputs go;
  - the concurrency traps: `test_guide.txt` is shared by unit and flow; no make or stash during a run;
  - reading PNG and WAV with plain Python (no PIL or numpy on the Mac).
- The `harness` skill.

**Done when:**
- No harness file is over 1,500 lines, and no function over 400 apart from the unit groups' bodies.
- `help` lists every mode with its file.
- Every group gives the same results alone as in the full run, apart from the named exceptions:
  - the three timing checks that depend on warm caches (C-15's, R-408's, C-09's);
  - what follows `grammarChecks`' reload of the grammar from disk;
  - the records check's reading of the `test_settings.txt` that `settings` writes.
- `prove compare` reports everything the same.
- A one-file harness edit rebuilds in 5 s or less.
- The harness objects' reproducibility is tried again.

**Done 2026-10-24:** the log's E-03 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- 43 files in `src/harness/` beside E-02's three (the largest 1,334 lines, 94 KB), named a little apart from the list above: `civ_lore` (shards,
  charts, ruins, graves), `civ_music`, `civ_records` (records and the decoder), `modes_rogues`, `modes_drones`, and the unit's
  `unit_civ`, `unit_peoples`, `unit_sky`, `unit_probe` (`unit_civ` and `unit_sky` were over 100 KB of long lines).
- The method as planned: the `git mv`, the linkage edit (a shared static lost `static`, its default arguments moved into its new
  declaration), `harness.h` and two tables' headers, one domain a step by `tools/mover.py`, each step's IR compared per function by
  the new `tools/samecode.py` (at -O0 and without exceptions: with them clang's `invoke`s follow its emission order).
- 41 unit groups, not 30: the `*Checks` are groups of their own. Each states the render scale and mush the groups before it leave
  (a scripted `Game` sets 4x), so every group alone gives the full run's results.
- The flow's and the bench's stages are member functions of `Flow` and `Bench` (their lambdas and shared locals the members), so
  their code reads as before; the scenes' finders likewise of `SceneFind`, whose `continue`s to the next body clang's AST found.
- The scenes' codes are `SCENE_*` (`SC_` was the settlements'). The modes the battery leaves out were proven by an A/B battery
  (`PROVE_ITEMS`), recorded before the edits.
- `TestWorld` and `scanSystems` went to E-12: the 112 star lookups take about 25 forms of failure and the 97 scans their own
  ranges, orders and exits, so each site's control flow would change, for two or three lines a site.
- 37 of the 40 harness objects compile the same twice (clang varies `voicePrint`, `unitGiants`, `unitSky`).

### E-04. The docs for their readers (L)

**First, the long lines:** docs 06, 07 and 08 are rewrapped (38 lines over 2 KB, about 200 KB). The Read tool cuts such lines,
so this comes before anything is distilled from them.

**`docs/decisions/`:**
- `log/`: seven straight moves of 10-decisions' line ranges, named so they sort in order (`0-iterations-1-2.md`,
  `1-1.1.0.md` … `6-1.4.0.md`). Headings are unchanged, so `grep '^## B-428'` still works, and `cat log/*.md` equals the old
  file.
- `rules-<area>.md` ×8: one line per current rule (≤ 160 characters) with its ids and a link to its log entry. The areas:
  - rendering and the frame;
  - galaxy and generation;
  - planet and surface;
  - civilisations and grammar;
  - game and UI;
  - audio;
  - harness and proof;
  - process and release.

  Distilling them means reading the 667 KB once, by ranges, split by release across subagents.
- `superseded.md`.
- `generations.md`: E-02's page, generations 1-16, one row each. CLAUDE.md's GEN_VERSION rule points to it.
- `reference/10-decisions.md` becomes a stub that points there.

**STATUS.md** (≤ 2 KB: the release, the branch, what is done, what is next, the next free ids) replaces PROGRESS.md's
"Next". PROGRESS.md moves to the archive whole.

**`reference/harness/`:**
- `INDEX.md`, generated by `help md`, one line per mode.
- `unit-checks.md`, generated.
- `proof.md`: the recipe and the F-01 tools, from 08 and the memory.

08-testing-and-tools.md keeps only what is not generated.

**12-civilisations.md becomes `reference/civilisations/`**, five files: peoples and ages, the built world, records and
grammar, sound, radar. Each gets a contents block.

**`docs/archive/`:**
- **Now:**
  - the finished plans: ITERATION-1.2.0 and 1.3.0, PLAN-performance, PLAN-galaxy-scale, both RESEARCH-records files, and the
    civilisation-expansion, ruins/shards/signals and gas-giant-probe ideas;
  - iterations 1 and 2;
  - iteration 3's O plans;
  - PROGRESS.
- **At the release:** ITERATION-1.4.0 and IDEAS-worlds-and-wanderers.
- **Links:** they are rewritten: 87 to docs/ideas, 163 to 10-decisions, 39 to iteration-3, and 27 in src comments. The src
  comments change in comment-only commits, proven by `prove objects`.

**Every reference note:**
- gets a contents block;
- has its remaining lines over 2 KB rewrapped by a script. Afterwards the `## ` headings, the bold-led bullets and the word
  counts are compared (F-07's rewrap once merged entries).

**docs/README.md** is rewritten: the map and the process, ≤ 3 KB.
- The bug and request TEMPLATEs match what is actually written.
- The stale statuses are fixed (R-421, R-422, the "(uncommitted)" lines).

**The lean touch points**, in `docs/CLAUDE.md` and the skills:
- **A milestone writes:**
  - one log entry of ≤ 4 KB: the decisions and why, plus one Verified, one Moved and one Left line;
  - its rules lines;
  - the reference notes, edited in place, with no dated additions;
  - the generated indexes, regenerated;
  - a KNOWN-ISSUES row of ≤ 300 B if something is left;
  - STATUS.md.
- **MILESTONES and the requests' statuses** are set once, at the close.
- **A bug or a request:** its B or R file is the one narrative. It writes a log entry only if a rule changed.

**Skills:** `milestone`, `bug`, `release`.

**Done when:**
- Every word of the old docs is somewhere: a word count per moved file.
- A rule is found in 6 KB or less.
- No doc line is over 2 KB.
- `make check` finds `help md` equal to INDEX.md.

**Done 2026-10-24:** the log's E-04 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- The log's files are named by their releases: `0-1.0.0.md` holds the table and everything before 1.0.0 (iteration 3's O
  series too), so not `0-iterations-1-2.md`.
- 15 rules files, not 8: seven areas needed a second (`rules-<area>-<topic>.md`) to keep each under 6 KB; `decisions/README.md`
  says which holds what. 486 rules from 1,040 candidates; 110 superseded.
- 08's mode tables are lists (a table's row cannot wrap); KNOWN-ISSUES' four long rows go on below its table.
- The contents blocks are generated (`tools/contents.py`) and `make check` holds them, `INDEX.md`, `unit-checks.md` and the
  docs' line lengths; it builds the harness first.
- `PLAN-star-classes.md` (done in 1.1.0) went to the archive too; `10-decisions.md` and `12-civilisations.md` stay as maps.
- The src comments' paths (twenty headers) proven by `prove objects` (90 the same).

### E-05. The game's header (L)

**The hooks out:**
- `class TestGame : public Game` goes in `src/harness/test_game.{h,cpp}`, and game.h's private part becomes protected.
- 333 hook lines leave game.h, and 709 leave 19 .cpp files (game.cpp loses 39% of its bytes). The 17 hooks never called go.
- The four knobs that production code reads (`testProbeQuiet`, `testAim*`) stay, as a small `HarnessKnobs` member.

**game.h by subsystem:**
- The nested state structs move into `src/game/*_state.h`: game_types, shard_world, radar, probe, drone, case, player,
  telescope, autoexplore, tape, pad, chart, landing.
- Each subsystem gets a banner naming its files, and an index sits at the top.
- Declarations become one line each; the prose moves to the definitions.
- Data members are never reordered, and `using X = ::X;` stays for the names used outside Game.

**The platform:**
- `static_assert`s on literal values for the key codes and the pad's array sizes, in `platform_raylib.cpp` (the only file
  that sees raylib).
- `#ifdef RAYLIB_H` with `#error` in core/input.h.

**Done when:**
- game.h is at most 45 KB.
- `prove compare` reports everything the same: the screens at every scale, the flow, every WAV.

**Done 2026-10-24:** the log's E-05 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- The order: the state structs out first, so their headers could take the six file-local helpers the hooks shared with their
  files (as `inline`, rule 3); then the hooks' definitions moved; then TestGame; the comments last. Each step proven alone.
- 316 hooks are TestGame's (the 333 less the 17 never called, plus `setState`, the harness's alone); the four knobs are
  `Game::harness`, a `HarnessKnobs` at their offsets.
- Twelve state headers and `game_types.h`: `shard_world.h` named by its struct; the drone's model in `drone_state.h`,
  `AutoProfile` in `autoexplore_state.h`. The one-line nested structs (`SiteMark`, `GalleryItem`, `JumpPlot`...) stay in the
  class.
- A declaration keeps its line (several on one line stay so); 133 comments moved to their definitions, 56 dropped where the
  definition's said it already. game.h 42 KB; game.cpp 50 to 28 KB.
- `main_raylib.cpp` holds the platform's arrays to input.h's constants as well: it is where they are copied.

### E-06. The game's files (M)

**The mixed files split:**
- autoexplore → core, `_space`, `_ground`, `_hands`, plus an internal header;
- probe → probe, probe_feed, probe_records;
- audio's radar receiver (lines 92-671) → audio_radar.cpp;
- game_states → surface_play, menus, data_sheet;
- guide_screens → names, star_map, and the rest;
- hud's sector map → sector_map.cpp.

landing_map, shards_screen and radar are split too if the reading shows it pays (landing_map_build, shard_world,
radar_camera).

**The state table:**
- One row per GameState (simulating, overlay, radar on, telescope on, dark radio, saved, pad context …) replaces ten
  hand-kept lists that differ subtly.
- First a unit check that the table equals the old predicates for all 29 states; then the switch.
- The render calls keep their number and order.

**The note:** `src/game/CLAUDE.md`:
- the map;
- the frame order;
- the new-screen checklist (the cookbook's recipe names 2 of the ~14 places; it is corrected).

**Done when:**
- No game file is over 1,500 lines, and the files named by subsystem tell where things are (`updateSurface` is no longer in
  game_states.cpp, nor `renderSpace` in hud.cpp).
- `prove compare` reports everything the same.

**Done 2026-10-24:** the log's E-06 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- The moves first (`tools/mover.py`, `tools/samecode.py`), then each module's helpers placed: one a single file uses stays
  file-local there, one its files share goes to the module's `_internal.h` as `inline` (rule 3).
- game_states.cpp is menus.cpp, its surface and data screens moved out; hud.cpp's scenes went to `scene_draw.cpp`.
- landing_map and radar split (their builders, their camera's picture: each behind a few calls); shards_screen did not,
  its world being 98 lines behind nine of the screen's calls.
- game.h's banners name the new files; four sections regrouped by file (declarations only).
- The table: nine traits as flags, the controller's context, the name. The radar's and the telescope's lists were one:
  one trait. The render switch's overlay labels became its `default`; the update switch is unchanged.

### E-07. The worlds (L)

- **planetmap.cpp (326 KB)** splits into:
  - `planet_features`;
  - `planet_bodygen`;
  - `planet_landforms` (`landformsAt` kept whole);
  - `planet_felisian`;
  - `planet_grounds` and `planet_grounds2`;
  - `planetmap.cpp`, the dispatch, about 180 lines;
  - `planet_internal.h`, about 50 helpers, with `octs`, `lodFade`, `featFade` and `scaleVisible` inline.

  planetmap.h's finders move to `planet_features.h`.
- **Every engine header** puts its harness-only declarations in a trailing section: 108 of the 608 functions are used only by
  the harness.
- **The galaxy and core notes** get their maps filled in.
- **`salt scan`:** a harness mode and the `world-kind` skill. 1.4.0 wrote that scanner five times.
- **Bench pairs** on the terrain.

**Done when:**
- No file in src/galaxy is over 1,500 lines, except kinds.cpp and the civilisation files E-09 and E-10 split.
- `prove compare` reports everything the same.
- The bench is level.

**Done 2026-10-24:** the log's E-07 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- `tools/mover.py` moves the entities inside an anonymous namespace one by one (`flat`, `open`, `close`, `drop`): the planet
  files and E-09/E-10's big namespaces need it.
- planet_internal.h holds the inline helpers and templates as they were, the types the files pass, and 52 functions declared
  there and defined once (the fields, the relief, `landformsAt`, `felisianSample`, the grounds): a split that crosses an inlined
  call is timed, not assumed (`fingerprint bodies` in pairs; the zoom's sampling on Heilya I).
- The hook sections move a comment group whole and only when every function in it is the harness's: 51 entities in 21 headers;
  a group that holds a function the game needs stays.
- `salt scan` reads `kindOf`'s own facts (recorded while the harness asks) and its test (`kindSourceFits`), and walks the kinds
  as `kindOf` does, so it cannot drift from it.

### E-08. The views (L)

- **surface_view.cpp (323 KB, the hottest file)** splits into:
  - `surface_view`: init, streaming, update, render;
  - `surface_env`, `surface_places`, `surface_walk`, `surface_palette`, `surface_terrain`;
  - `surface_sky` (`drawSky`'s 643 lines kept whole);
  - `surface_effects`;
  - an internal header.
- **surface_view.h:** its eleven helper structs go to `surface_types.h`, and the declarations are grouped by file, hooks last;
  the members are untouched.
- **space_view.cpp** splits into `space_view`, `space_sun`, `space_globe`, `space_bodies`.
- **The notes:** src/surface and src/space get their maps filled in.
- **Bench pairs** on the sky, the globe and the terrain.

**Done when:**
- No file in src/surface or src/space is over 1,500 lines.
- `prove compare` reports everything the same.
- The bench is level.

**Done 2026-10-24:** the log's E-08 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- surface_view.cpp in eight files and space_view.cpp in four, each with its `_internal.h`, the files chosen so that every call
  the compiler inlined between the old file's functions stays in one file (clang's `-Rpass=inline` read before and after): six
  cold calls cross in the surface, one in space.
- surface_view.h's types in `surface_types.h` (the eleven structs, the creatures' states and their names). The class holds its
  data first, never reordered, then its functions under a banner for each file that defines them, the harness's nine last
  (`build/e08/step2/regroup.py`, proven item by item); `drawFountains`, declared and never defined, went (E-12's list had it).
- The surface note is 4.4 KB: E-13 trims the notes over 3 KB.

### E-09. The peoples' directory (L)

**`src/peoples/`** takes the civilisation half: 24 modules, 31.3k lines, and the grammar folder, moved whole with `git mv`.
- The modules:
  - peoples, history, plan, towns, works, buildings, ruins, forms, genome, roads, graves;
  - records, shards, genres, grammar, tongue, voice, legends;
  - music, charts, instruments, instrument_plans;
  - signals, drones.
- src/galaxy keeps the worlds. `landmarks.cpp` and `planetmap.cpp` stay the named exceptions in `make check`'s layering rule.

**Everything that names the directory:**
- the includes, in about 47 files;
- the Makefile's directories;
- CMake's `CORE_SOURCES` and `STRICT_SOURCES`;
- the three release scripts' `find` lists;
- the harness's five grammar paths, with both grammar checks made to require every file.

**The order of destruction at exit** moves with the link order, so the unit and the flow run once under ThreadSanitizer.

**The splits:**
- buildings.cpp (339 KB) → `buildings_frame`, `_dress`, `_medieval`, `_ages`, `_works` and buildings.cpp, with an internal
  header.
- works.cpp → `works_sites`, `_land`, `_pieces` (`workSpec`, a 530-line function), and works.cpp.
- ruins.cpp → `ruins_towns`, `ruins_settlement` and ruins.cpp, with `ruins_layout.h`.

**The note:** `src/peoples/CLAUDE.md`.

**Done when:**
- `build-mac.sh arm64` and `build-win.sh x86_64` build (no Docker needed).
- `prove compare` reports everything the same, including `fingerprint modules` and `peoples`.
- The bench is level.

**Done 2026-10-25:** the log's E-09 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- The move by `git mv`, its includes and comments rewritten by a script; `make check` holds the halves apart with
  `planet_bodygen.cpp` the second exception (E-07 moved planetmap.cpp's read of the peoples there).
- buildings.cpp in six files, works.cpp in four, ruins.cpp in three, each with its internal header (`ruins_layout.h` for the
  ruins). Inlined calls that now cross: `ageExpressed` (public, through `Gen::gv`), the decay functions, the dispatch's
  generators, the works' finders, `cultureAtAge`; twelve bench pairs level but two single runs, the generation items as fast.
- `tools/mover.py` takes a banner's namespace line as a wrapper; `tools/clash.py` checks the names a split makes global.
- The peoples' and the galaxy's notes are 4.8 and 4.7 KB: E-13 trims the notes over 3 KB.

### E-10. The peoples' texts (L)

**The splits:**
- history.cpp:
  - first `history_names.cpp` and `history_tables.h`;
  - then the builder's phases: `history_builder.h`, `_setup`, `_events`, `_years`.
- genres' schema → `genres_schema.cpp`.
- grammar.cpp → `grammar_parse`, `grammar_words` (with the checks) and grammar.cpp (the expansion), with an internal header.
- forms → `forms_colliders.cpp`.
- music → its per-age builders in `music_ages.cpp`.
- `lore.h` (Lore, loreSeed) comes out of shards.h, which six headers include only for that.

**Then the Linux check** (hazard 4), in one Docker session asked for first.

**Done when:**
- No file in src/peoples is over 1,500 lines, except instrument_plans.
- `prove compare` reports everything the same.
- The arm64 container's new tree matches its old tree, with the workers and with one thread.
- The bench is level.

**Done 2026-10-25:** the log's E-10 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- history.cpp in seven files (its tables and its builder in two headers, the builder's phases in three), genres.cpp in two
  (the schema's .h and .cpp), grammar.cpp in three over an internal header, forms.cpp and music.cpp in two each; the largest
  left is genres.cpp (1,212 lines).
- `lore.h` serves four headers (graves, charts, history, music): the others that include shards.h use its records.
- The inlined calls that now cross are cold (the history's phases, a piece's builders), timed level or a few microseconds.
- The Linux check: GCC 9.4 cannot compile kinds.cpp's table check (KI-423); with it taken out, the regress and the peoples'
  fingerprint match e09's and 11f2c8f's in the arm64 container, both ways. The splits moved Linux's last bits in some ground
  samples and buildings (GCC fuses multiply-adds after inlining); the Mac's and Windows' clang keep theirs.

### E-11. The grammar's shelves (M)

**A manifest, `grammar/files.inc`,** becomes GRAMMAR_FILES' initializer.
- Three lines per file: `{"name",`, then `#include "…"`, then `},`. It is append-only.
- The 107 `SRC_` variables go.

**Folders by role:**
- `core/`: records, world, people, culture ×3, lexicon, history, charts;
- `registers/`: annal, letter_events, news, song, log;
- `ages/<age>/`;
- `genres/`: letters, diary, ending;
- `peoples/sunlost/`.

**Files named by topic:** a number today only means "continued for the 16 KB cap" (medieval3 is the sermon; sunlost11 and 12
continue 6 and 7).
- The disk loader and the unit's directory check walk the folders recursively, in the same order, and require every file.
- Files move whole and nothing merges, because `@apart` works per file and the rule order matters.
- The printouts that name files are compared through a rename map.

**`grammar/CLAUDE.md`:**
- the layout;
- the directives (`@apart`, `@instead`, `@root`);
- the cap (MSVC's literal limit);
- the text rules from the memory;
- a generated table of rule prefixes and their files.

**New modes:**
- `grammar where <rule|prefix.*>`: where a rule is defined, its alternatives, every alternative and root that reaches it, and
  the C++ conventions that build names.
- `grammar schema`.

**Skill:** `grammar`.

**Done when:**
- The unit's pinned hash 0x13212ef73adea507 holds.
- `fingerprint peoples` is the same.
- `grammar where` answers for the 409 rules used from other files and for the names built in C++.

**Done 2026-10-25:** the log's E-11 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- `files.inc` replaced the 107 `SRC_` variables and every object compiled the same; the loader and the unit's check share one
  walk (`shardGrammarTree`).
- The shelves hold 9, 24, 53, 9 and 12 files, named by topic, uniquely; the station's log (`genre.log`, the modern age's alone) went
  to `ages/modern/`, not the registers. The files' own comments name the new files.
- The table of rule prefixes and their files is a page of its own (`docs/reference/civilisations/grammar-files.md`, 17 KB,
  `grammar where md`, held by `make check`), so the note that loads with every grammar edit stays under 4 KB.
- `grammar where` answers for all 3,223 rules: 409 named by a mark from another file, 603 names the C++ builds. `grammar schema`
  prints the words each fact and field takes (`recordSchemaKeys`).
- Found: the picture register is a root no call expands (KI-424).

### E-12. The sweep (M)

**Shared helpers** (each proven by the fingerprints, the expression's shape kept):
- from E-03, the harness's: `TestWorld` (a pinned world's star, system and body: 112 lookups, each keeping its own message and
  exit) and `scanSystems` (97 sector scans, each keeping its range, order and early exits), proven by the battery and the A/B of
  the modes it leaves out (`PROVE_ITEMS`);
- `nowMs` ×7 → `core/clock.h`;
- `fnv1a` ×5 → `core/rng.h`;
- the sphere helpers of plan, works and landmarks → a shared header, with both `toward`s named;
- `lowerOf` ×3;
- `hash01` ×3 (instruments' different `h01` renamed apart).

**Dead code:** about 18 items referenced nowhere, among them a declaration never defined (`site.h`'s `waterHeight`;
`surface_view.h`'s `drawFountains` went in E-08).

**The game's helpers:**
- `enum Beep` with `refuse(msg)` (191 sites);
- `menuStep` (25 sites);
- `drawPanel` (about 45 boxes, coordinates exact);
- `resetExpedition`, keeping the two resets' differences;
- enums for the cabin's facings and the text-entry kinds;
- `const char* const` tables.

**Rng copies:** a scratch build with the copy constructor deleted lists every copy into the rules note. None is changed
unless it is a bug.

**Comments by the policy:**
- Say what is true now and why: units, invariants, traps.
- At most one decision pointer per section; bare tags (≈ 370 in the engine and the game) and history narration go.
- Header openings of 10 lines or fewer.
- These are comment-only commits, proven by `prove objects`.

**Long lines:**
- Lines over 200 characters in C++ (≈ 1,100 in the engine and the game, more in the harness) are wrapped in whitespace-only
  commits and listed in `.git-blame-ignore-revs`.
- `.vg` lines are never wrapped: an alternative's key hashes its line.
- Engine and game wraps are proven by `prove objects`. Harness wraps too where its objects are reproducible (E-03: 37 of 40;
  `modes_signals`, `unit_kinds` and `unit_sky` by `prove compare`).
- `make check` then refuses any C++ line over 300.

**Warnings:** the Makefile build has none, and the strict directories (core, galaxy, peoples) build under `-Werror` as
CMake's do.

**Done when:** the comment and wrap commits are proven as above, and `prove compare` reports everything the same for the rest.

**Done 2026-10-26:** the log's E-12 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- Dead code: 24 items (the linker's dead lists of both binaries, crossed with the sources), `waterHeight` among them.
- The shared helpers as listed, the sphere's in `core/sphere.h` (`towardUpTo`, `towardThrough`); the game's `refuse` 57 sites,
  `menuStep` 13 lists, `drawPanel` 14 panels (the blended ones with their edge; the plan's 45 counted every box call).
- The harness: `scanSystems` is a range mirroring the two loops (52 scans); the plan's `TestWorld` became `systemAt` (82 lookups), a
  function keeping each site's names and failure. Scans with a break at their level, a star filter or another stop stay loops.
- Rng copies: two, both meant (system.cpp's moons drawn again, music_ages.cpp's refrain), in `rules-galaxy.md`.
- Committed with E-03 to E-11 on the user's word, from each step's tree (`build/e12/trees.txt`): the comment and wrap steps are
  commits of their own, listed in `.git-blame-ignore-revs`.

### E-13. The close (S-M)

- **The CLAUDE.md tree:** audited with claude-md-improver, and every directory's map checked by `make check`.
- **The measurement:** the eight tasks run again with E-01's prompts, and the tokens compared; the result goes in the log.
- **Memory and docs:** the memory takes its final shape; the 1.4.0 docs close (the ITERATION rows, MILESTONES, STATUS).
- **The old proof kits** (9.4 GB in `build/p00` … `build/d05`) are deleted on the user's word. `perf-baselines`' binaries and
  the series' snapshots stay.

**Done 2026-10-26:** the log's E-13 entry (`docs/decisions/log/6-1.4.0.md`). As built:
- The notes: every directory's at 3 KB or under and the root at 6.6 KB (the ten went from 45.1 to 33.2 KB). The checklists and
  the frame's order moved to the cookbook and 07, and nine stale `file:line` references became names.
- The measurement: 1,076,138 tokens for the eight (E-01's 1,328,318: -19%) and 356 tool uses (399), in section 11.
- Section 2's targets are met (the numbers are in the log); installing the LSP is the user's.
- The battery found `tape` racing the home system's maps; it now waits for them, as E-02 made the other modes wait.
- The old kits wait for the user's word.

## 5. The proof, every milestone the same

- **At the start:** the previous milestone's snapshot exists (`build/prove/<id>`).
- **After each step:**
  - a move step: `prove compare` on the items it can touch;
  - a comment or whitespace step: `prove objects`;
  - a hot path: bench pairs.
- **At the close:**
  - the whole battery reports everything the same;
  - `unit` passes;
  - the flow passes;
  - `bench check` is within all 38 budgets and level in pairs;
  - then `prove snapshot <id>`.
- **The battery:**
  - `fingerprint bodies`, `stream`, `peoples` and `modules`;
  - `regress` with the workers and with one thread;
  - `screens`, every scene, the sweeps and the data sheets;
  - `signals` with every radar WAV;
  - the music, voice, medium, instrument, sound, deck, probe and audio WAVs;
  - `rogues heard`, `rogues voices` and `rogues dark`;
  - the saves, guides and settings the flow and the regress write, byte for byte;
  - `unit` with its timings masked;
  - the flow from the snapshot's own starting state.
- **Each item counts its files**, so a renamed or missing output fails instead of being skipped.
- **How it runs:** through the `prover` agent, so its output stays out of the main context; under `caffeinate -i`; and alone.
  D-05's sweep frames moved only when run beside other jobs.

## 6. Skills, agents, hooks and plugins

### Existing ones to use

| What | Kind | Here |
| --- | --- | --- |
| clangd-lsp (official) | plugin, an LSP for C/C++ | definitions, references, a file's symbols without reading it; needs clangd (installed) and E-02's `compile_flags.txt`; the user installs it |
| claude-md-management (enabled) | plugin: `claude-md-improver`, `/revise-claude-md` | audits the CLAUDE.md tree at E-13 and each release; at a session's end it puts lessons into the CLAUDE.md files, not the memory |
| skill-creator (available) | skill | writes our skills and tries each on a sample task |
| session-report (official) | plugin | tokens per session and per prompt, beside E-01's and E-13's subagent measurements |
| Explore (built in) | subagent | "where is X" sweeps whose file dumps stay out of the main context |
| pr-review-toolkit, `/code-review` | plugin, built in | the release PR |
| find-skills (installed) | skill | when a new need appears |

**Not recommended:**
- commit-commands: it commits under the user's identity with its own trailers; ours carry a random identity and no session
  link.
- feature-dev: its flow is not the milestone workflow.
- code-simplifier: its instructions are written for JavaScript and React, and our UI code draws pixels; at most report-only.
- ralph-loop: not useful here.
- hookify: our hooks are simpler to write directly.

### Our own (`.claude/skills/<name>/SKILL.md`, in git)

Until a skill is used, only its name and description load.

| Skill | Its trigger | Inside | Lands in |
| --- | --- | --- | --- |
| `prove` | prove a change leaves everything Vesperis makes identical | when to snapshot and compare, what each item proves, the known noise (frames beside other jobs, the flow, timing masks), what to do on a difference, the report's lines | E-02 |
| `bench` | measure the frame's cost against the previous binary | caffeinate, paths of equal length, AB/BA pairs, best-of and median, the 38 budgets, "level", the noisy rows | E-02 |
| `look` | inspect the harness's pictures and sounds | which PNGs to open (8 at most per response), crops and stats with `tools/png.py`, dark frames measured not eyeballed, WAV stats, pngdiff | E-02 |
| `harness` | add or change a mode or a unit check | the registry row, the group's file, outputs in `shots/tests`, the generated index, the traps | E-03 |
| `milestone` | run one milestone from "proceed" to the report | read STATUS and the milestone's section only; the defaults; the done-when; the proof; the touch points; the report's shape; wait | E-04 |
| `bug` | record or fix a reported bug | next ids, the resolved-style report, cause, fix, verification, touch points, one question with a recommendation | E-04 |
| `release` | cut a release | branch, versions, PR, tag, six builds, uploads; random author, committer and tagger; no session link; ask before Docker; Linux builds last, one at a time | E-04 |
| `world-kind` | add a kind of world, a trait or anything drawn from the hashes | the kinds row, salts, the protected worlds and `salt scan`, the generation protocol, scenes, the proof | E-07 |
| `grammar` | write or change the records' grammar | the shelves, directives, guards, append-only lines, `@apart`, the cap, `grammar where`, the checks, reading fifty outputs | E-11 |

### Subagents (`.claude/agents/`)

`prover` runs `tools/prove` in the background under `caffeinate -i`.
- It returns 30 lines or fewer: everything the same, or each difference with its file.
- Tools: Bash and Read. A cheaper model is enough for it.

### Hooks (`.claude/settings.json`, in git, PreToolUse; from E-01)

- **Read** of a file over 100 KB without offset or limit is refused, with "read a range, grep -n, or the LSP's symbols; the
  map is in CLAUDE.md". Over 100 KB a whole Read pays the full 25k-token cap and still shows only part of the file.
- **Session links:** `git commit`, `git tag`, `gh pr create|edit`, `gh release create|edit` and `gh api` calls are refused when
  they carry `claude.ai/code` or `Claude-Session`. The check covers the message files they name (`-F`, `--body-file`), and it
  leaves greps and logs alone. The no-session-links rule is enforced, not remembered.
- **Build files:** a `make clean` whose Makefile still removes `build/` whole (a checkout before E-02) is refused, and an rm of
  `build/` itself always; cleaning inside `build/…` works.

Hooks and plugins are read when a session starts, so each lands with a new session.

## 7. The CLAUDE.md tree and the map

The root map, as E-01 writes it (the `→` parts arrive with their milestones):

```
src/core          framebuffer (and the palette), raster, font, noise, rng.h (hashes), png, input, parallel (the worker
                  pool), types.h, fs.h
src/galaxy        the worlds: starfield, system (bodies, orbits, GEN_VERSION), kinds (the kind table), planetmap → planet_*
                  (the planet function, grounds, landforms), drainage, landmarks, describe, almanac/nights/fronts, probe,
                  drifters
src/peoples       → E-09: peoples (ages), history, plan, towns, works, buildings, ruins, forms, genome, roads, graves;
                  records, shards, genres, grammar (+ grammar/*.vg), tongue, voice, legends; music, charts, instruments,
                  instrument_plans; signals, drones
src/space         space_view (stars, suns, globes, belts) → space_*, probe_view
src/surface       site (the landing site's caches), surface_view → surface_* (sky, terrain, water, walking, effects),
                  flora, creatures, bestiary, buggy, drone, forms_draw, textures
src/game          Game: game.h → *_state.h; game.cpp's frame order; one file per screen or device
src/harness       vesperis_test: main.cpp's MODES and `help`, modes_*, civ_*, unit_* (the unit's groups), scenes_*, test_worlds.h
src/main_raylib.cpp, src/platform*   the window, input and audio: the only raylib code
tools/            → E-02: prove (snapshot, compare, objects, bench), benchcmp.py, keydiff.py, png.py, mover.py
tests/            regress_baseline.txt
packaging/        the release builds (toolchains.sh, build-*.sh: they list the engine's directories)
music/            the tape's four tracks
docs/             STATUS.md (now); decisions/ (rules by area, log by release); reference/ (how things work,
                  harness/INDEX.md); iteration-3/bugs, requests; ideas/ (the backlog); archive/
build/            the objects (all `make clean` removes), prove/<name>/, perf-baselines/, toolchains/
shots/tests/      every harness output (≈ 2 GB; never copy it whole)
```

The directory notes, at most 3 KB each, mostly from the memory's lessons and 10-decisions' standing rules. Each opens with a
one-line-per-file map that `make check` holds true.

- **src/core:**
  - Pixels are the same at any thread count: each pass is row-pure or band-pinned.
  - `WorkerPool::run` is not reentrant, and only the main thread writes the caches and the framebuffer.
  - The look: 16-bit pixels, 32 banks of 2048, the mush.
  - Doubles everywhere.
  - The font is 6 px a character, 52 characters a line.
  - `pngFlush` comes before reading a picture back.
  - After touching parallel.cpp: `pool` and a ThreadSanitizer build.
- **src/galaxy:**
  - Hash discipline:
    - each decision draws from its own salt;
    - drawn tables are append-only;
    - a seeded shuffle with a count, not a coin per item;
    - coprime strides for hash chains;
    - galaxy-wide properties per sector cell.
  - The generation protocol and the protected worlds: a people's world and the home world never change kind; Heilya II stays
    felisian; the regress and bench worlds don't move.
  - The views' shared features read the body, the system and BodyGen, never a site.
  - 05's consistency rules.
  - Rng is passed by reference, and an expression's shape is kept.
- **src/peoples:**
  - The records stay byte-identical except where meant (`fingerprint peoples`).
  - The CAUSE_SUN_LOST pattern: append outside the drawn range.
  - `@apart` for a people's own words.
  - The link-order and destruction-order notes.
- **src/peoples/grammar** (src/galaxy/grammar until E-09): the format, the directives, the cap, the text rules (singular
  slots, three pronoun cases, no "the" on mass nouns, read fifty outputs), `grammar where`, and never rewrapping a line.
- **src/space:**
  - `BackdropKey`: every new input to the backdrop pass goes into it.
  - What is in the sky is where the maths puts it.
  - The sun is drawn by angle per pixel; a globe is culled only behind the camera.
  - Depth is in metres; the banks follow 02's table.
- **src/surface:**
  - One planet function, and a material never follows a faded value.
  - The caches are in local metres, cleared at init and on reanchor, and refilled from the guide.
  - Nothing far off is computed on the main thread (B-422).
  - The colliders are the walls drawn.
  - The checklist for adding a vehicle.
- **src/game:**
  - The frame order and the new-screen checklist.
  - Persisted enums are append-only.
  - `guideReturn`, and the AudioState pointer contract.
  - The UI is terse; nothing is named until the explorer names it; maps show only marks.
  - One save line per field; no raylib.h.
- **src/harness:**
  - The registry, the groups, the scene codes and the test worlds.
  - Outputs go in `shots/tests`.
  - The concurrency traps.
  - Deterministic modes against the flow.
- **docs:**
  - What is live and what is archive.
  - The lean touch points and the next free ids.
  - Edit in place, with lines of 140 characters or fewer, and log headings in the form `## ID (date): title`.
  - Read heading trees first, and never read a log whole.
  - STATUS.md is the one place for the current state.

`.claude/rules/` (path-scoped rules, which load when matching files are touched) is kept for anything that crosses
directories. Nothing does today.

## 8. Decided by default (the user may change them)

1. **The series:** the letter E, unused so far. Thirteen milestones after D-05, in 1.4.0, before its release. Generation 16 is
   unchanged.
2. **Commits:** asked at E-01, as standing leave to commit each step. Moves and edits go in separate commits, each under a
   random author and committer, with no session link. The whitespace-only commits are listed in `.git-blame-ignore-revs`,
   which needs their hashes. Without that leave, one commit per milestone when the user asks.
3. **The memory's histories:** deleted after a local copy; their lessons move to the CLAUDE.md files.
4. **PROGRESS.md:** archived whole; STATUS.md and the log by release carry on.
5. **`src/peoples/`:** the civilisation half's directory, under that name.
6. **The objects stay where they are:** `make clean` derives its directories instead of moving the objects.
7. **The old proof kits:** deleted at E-13, on the user's word.
8. **The hooks:** they refuse whole reads over 100 KB and session links in commits, tags, PRs and releases.
9. **Comments:** an id stays as one pointer per section; bare tags and history narration go.

## 9. Considered and set aside

- **clang-format over the tree:** it rewrites every line and buries `git blame`, for nothing an agent needs beyond the long
  lines E-12 wraps.
- **Moving the objects to `build/obj/`:** the old kits would link stale objects and prove the old code against itself.
- **Self-registering modes or unit groups:** link order would decide the run order.
- **Subdirectories inside src/game and src/galaxy beyond src/peoples:** the globs, CMake, the release scripts and the include
  paths churn for little; prefixes and maps do the job.
- **Merging the grammar's continuation files:** the rule order would change. Lifting the 16 KB cap with literal breaks is
  possible later, under the fingerprint.
- **Precompiled headers:** they help build time, not context, and the split already brings a harness edit under 5 s.
- **CMake as the main build:** the Makefile is 44 lines and the release scripts work.
- **Rewriting the reference notes' dated additions as one present-tense text:** worth doing, but a writing job of its own
  (about 1.1 MB). E-04 adds contents blocks and rewraps; the rewrite goes note by note, when each is next touched.

## 10. Found on the way, not part of the series

- **Evaluation order of draws.** About 120 expressions draw twice from one Rng with an unspecified evaluation order: ruins 55,
  works 29, history 8, instrument_plans 6, music 5. GCC may evaluate a call's arguments right to left. The arm64 container
  matched the Mac where the regress looks, but the regress does not look at most of these. They are worth a bug of their own.
- **Exit codes.** An unknown harness mode exits 0, so a script cannot tell it failed.
- **A hidden file dependency.** The unit's records check reads the `test_settings.txt` that the `settings` mode writes; from an
  empty `shots/tests` the unit still passes, on the defaults (E-02).
- **`-Werror`:** CMake uses it on core and galaxy only; the Makefile nowhere.
- **D-05's latent grammar faults:** 'new New', 'old old woman', 'at at', 'Darvar Darvar'.
- **The radar's heard list** (found by E-01's baseline, section 11): `radarLock` marks a slot heard without checking that its
  programme speaks.
- **The decision table's generation rows stop at 14:** generations 15 (B-428) and 16 (P-01) have entries but no rows (E-04:
  done, `docs/decisions/generations.md` has a row a generation).
## 11. The eight tasks, measured

E-01's baseline, taken 2026-10-23 at 92e580a (with this plan uncommitted) before E-01 changed anything a session loads. Each task
ran as a read-only `Plan` subagent on Opus, all eight at once; E-13 runs the same prompts the same way. The preamble, the same for
all eight:

> This is a measurement run for the Vesperis repository at /Users/piblokto/vesperis (a C++17 game). Work out the task below the
> way you would before making the change, then stop and report. Rules: do not edit or create any file; do not build; do not run
> `vesperis` or `vesperis_test`; read-only shell commands (ls, grep, sed -n, wc, head, git log, git show) and the Read and Grep
> tools are fine. Your report, at most 40 lines: the answer (files, functions and line numbers, in order), then a last line
> starting "Read:" listing the files you opened with roughly how much of each.

The tasks, each prompt's second paragraph verbatim:

1. **Add a unit check.** "Task: add a unit check that, for 500 star systems (picked the way the existing system checks pick
   them), no two planets of one system orbit within 1% of each other's orbit radius. Where does the check go, which existing
   functions does it call, how is it run, and how would you run only it?"
2. **Add a harness mode.** "Task: add a harness mode `orbits <n>` to `vesperis_test` that prints, for n star systems, one line
   per body with its orbit radius, period and kind. Name every place that must change: the mode's function, the dispatch, and
   any list or documentation of the modes."
3. **Trace a radar path.** "Task: trace what happens from the moment the ship's radar locks onto a signal to the moment the
   signal's speech is heard. Name each function in order, with its file and line."
4. **Add a game screen.** "Task: add a new game screen, a ship's logbook opened from the cabin's menu, listing the systems
   visited with the date of each visit; Escape closes it, and the log is saved with the game. Name every file and place that
   must change, in order."
5. **Find the rule.** "Task: before changing how ruins are placed on a world, find every recorded rule or decision that
   constrains that change. Give each one briefly with its file and line."
6. **Prove unchanged.** "Task: a refactor will move functions out of `src/galaxy/planetmap.cpp` into new files and must change
   nothing the game generates, draws or plays. Plan how to prove it: what to record before the change, the exact commands after
   it, and what each comparison covers."
7. **Start a milestone.** "Task: the user has just said "proceed". Work out which milestone comes next, what it asks for and how
   it is proven done, and give your first three steps. Name the files you read to decide."
8. **Change the grammar.** "Task: in the records' grammar, let the texts of a seafaring people sometimes mention the tide. Name
   the grammar file(s) and lines you would change, the rules a grammar line must keep, and the checks to run afterwards."

| Task | Tokens | Tool uses | Minutes | What it read |
| --- | ---: | ---: | ---: | --- |
| 1. Unit check | 133,960 | 45 | 8.5 | 11 ranges of the harness (~650 lines), system.h/.cpp, 08 |
| 2. Harness mode | 99,047 | 36 | 6.6 | 4 ranges of the harness (~350 lines), system.h, 08, README |
| 3. Radar path | 147,892 | 32 | 8.1 | radar.cpp, audio.cpp (~440 lines each), signals, voice, game.cpp/.h, main_raylib, 9 more |
| 4. Game screen | 176,495 | 65 | 9.5 | 17 files: game.h, game.cpp, cabin, persistence, guide, the harness, 07, 09 |
| 5. Find the rule | 318,415 | 89 | 12.3 | 10-decisions (~1,000 of 4,171 lines in 9 ranges), 12-civilisations, ruins, plan, 11 docs |
| 6. Prove unchanged | 186,845 | 62 | 15.4 | the old kits (verify.sh, fp5.cpp), planetmap.h, ~550 lines of the harness, 08, this plan |
| 7. Start a milestone | 73,182 | 20 | 7.0 | CLAUDE.md, docs/README, this plan (~370 lines), the docs' diffs |
| 8. Change the grammar | 192,482 | 50 | 13.9 | grammar.h, two .vg files whole, 8 in part, genres, grammar.cpp, 12-civilisations (~330 lines) |
| **All eight** | **1,328,318** | **399** | | |

Tokens are the subagent's own total as the Agent tool reports it. Every run also loaded the root CLAUDE.md; none could run the
harness, so the "after" column of section 1 counts reading only.

What the runs found on the way:
- **A likely bug (not fixed):** `radarLock` (radar.cpp:361) marks the locked slot heard without checking that its programme
  speaks, while `radarStartContent` (:196) checks, as signals.h:145's rule asks. So a lock during a machine programme logs a
  recording as heard whose words were never on the air.
- **Wrong guidance, fixed in E-01:** CLAUDE.md's and README.md's claim that `./vesperis_test` alone lists the modes.
- **For E-02's fingerprints:** fp5's sample hash stops at `oldSea` (no `landformTrait`, tints, `slipFace`), its BodyGen hash misses
  about twenty fields, and its scan reaches 59 of 60 kinds and no dark body; the regress's map hash skips `wtint`/`ltint`;
  `pngdiff` walks only the first directory's names (count files both ways); the scene table has 109 rows, `scene_list.txt` 67.
  E-02 took them all: `gen2`, `ground2` and the dark bodies in `fingerprint bodies`, and `tools/prove` counts files both ways and
  runs every scene through `scene list`; the regress's map hash stays as it is (it is blessed, and the fingerprints read the tints).
- **`charts rogues`** named a drifter's chart's wanderers from the drifter's own system and read past its bodies (their indices are
  the former system's): fixed in E-02, the harness's printout only.
- **The probe's photos** (the `probe` mode and the unit) go to `shots/` itself as `probe_1.png`, `probe_2.png`..., beside the
  player's screenshots, over a photo of that name.
- **The unit's frames** `unit_r408_*`, `unit_b412_*` and `unit_t03_scan` are not repeatable: their trees and grass sway with a
  clock that the unit's waits on threads advance.
- **A scene makes an empty `shots/tests/shots/tests`** (a path joined twice).

**E-13's run** (2026-10-26): the same eight prompts the same way, at 0288255 with E-13's notes in the tree.

| Task | Tokens | Against E-01 | Tool uses | Minutes | What it read |
| --- | ---: | ---: | ---: | ---: | --- |
| 1. Unit check | 136,727 | +2% | 42 | 5.9 | the harness skill and note, unit.cpp, harness.h (~170), unit_basics (464), system.cpp |
| 2. Harness mode | 97,052 | -2% | 33 | 3.9 | the harness skill and note, main.cpp, harness.h, modes_galaxy (~290), 08 (~120) |
| 3. Radar path | 125,761 | -15% | 37 | 4.3 | the game and peoples notes, radar.cpp, audio.cpp (~350), audio_radar, signals |
| 4. Game screen | 200,212 | +13% | 82 | 9.9 | the cookbook's recipe, game_types.h, game.cpp, persistence.cpp, cabin, pad, 07 |
| 5. Find the rule | 195,516 | -39% | 65 | 8.3 | eleven rules files (seven whole), the notes, ruins.h, three logs (~270 lines) |
| 6. Prove unchanged | 117,806 | -37% | 30 | 8.3 | planetmap.cpp, tools/prove and its items, proof.md, samecode, mover |
| 7. Start a milestone | 68,921 | -6% | 23 | 4.1 | CLAUDE.md, the milestone skill, STATUS, MILESTONES (~75), this plan (~36) |
| 8. Change the grammar | 134,143 | -30% | 44 | 8.9 | the grammar skill and note, its rules, grammar-files.md, two .vg in part |
| **All eight** | **1,076,138** | **-19%** | **356** | **53.6** | |

The answers hold up against the tree. The reading took the new routes: the rules by area instead of 10-decisions (task 5), the
proof's recipe and tools instead of the old kits (task 6), and the grammar's skill and note instead of two .vg files whole (task
8). Task 4 read more and named more places: fourteen, among them the controller's commands, a `screens` capture and a unit
check. The minutes are wall-clock, with eight at once, so they move with the service. Found: E-01's radar finding is still in the
code (KI-425); `sampleSurface` inlines `qualityWater`, `tidalFlat` and `tideRangeAt`, so a split keeps them in its file.
