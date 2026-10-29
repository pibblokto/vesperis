# Proving a change

How to show that a change leaves everything Vesperis makes, draws, plays and saves the same, and the tools for it. The skills
`prove`, `bench` and `look` (`.claude/skills/`) give the steps to follow; the `prover` agent runs the battery and returns its
verdict. The modes are listed in `INDEX.md`, the unit's checks in `unit-checks.md`; what each mode does at length is
`../08-testing-and-tools.md`.

<!-- contents -->
Contents:
- The recipe
- F-01's tools (2026-10-12)
- E-02: the fingerprints and `tools/prove` (2026-10-23)
<!-- /contents -->

## The recipe

- **Before the first step:** the previous milestone's snapshot is the base (`ls build/prove`); with none, take one before
  touching anything: `tools/prove snapshot <id>-base`.
- **After each step:** a move or an edit, `tools/prove compare <base> <the items it can touch>`; a comment-only or
  whitespace-only step, `tools/prove objects <base>` (the engine's and the game's objects are the same between compiles; three
  of the harness's are not, modes_signals.cpp's `voicePrint`, unit_kinds.cpp's `unitGiants` and unit_sky.cpp's, so the harness
  is proven by its behaviour); a hot path, `tools/prove bench <A> <B>` in pairs.
  A move of whole functions: `tools/mover.py` moves them and rebuilds the old file from the new ones, `tools/samecode.py`
  compares every function's IR before and after (E-03, `../08-testing-and-tools.md`).
- **A screen, a menu or a key** (U-01): `tour` opens every state as the game opens it, from a fresh game each, and records its
  controller's menu, its keys, its menus' rows by label and where Esc goes; a step that touches a screen compares `tour`,
  `screens`, `flow`, `input` and `settings` (the unit's `states` group holds the traits and each row). U-06: then every row,
  opened by its row (`openScreen`) or its setup by name, its controller's commands run: a new screen needs no stop.
- **A name that moves** (E-05): a type taken out of its class, a class renamed: `tools/samecode.py --rename
  Game::RadarState=RadarState` reads both sides' names demangled (Homebrew's `llvm-cxxfilt` reads every name clang makes;
  Xcode's `c++filt` misses some) with the renames applied; a file-local helper a split shares (E-06) compares under `--rename
  '(anonymous namespace)::lockCone=lockCone'`. **A data change that keeps the layout** (fields gathered into a struct at their
  offsets): the -O0 IR numbers the fields, so samecode sees every function that touches a later field differ; prove it instead
  by the record layout (`clang++ -Xclang -fdump-record-layouts`: every field at its offset) and the -O2 objects compared
  function by function (`objdump -d -r`, a branch's raw target dropped): only the constructor moves.
- **A split's namespaces and names** (E-09): `tools/mover.py`'s flat mode takes a namespace's line or brace with a banner over
  it as a wrapper too (the banner an entity of its own, moving with it), so a target's `open` or `close` can pair with it. A
  name a split makes global (a helper or a type into its internal header) is checked by `tools/clash.py <header>`: an inline
  function or a type's member of the same mangled name in another object would be merged by the linker, silently.
- **A split's cold calls and a header's includes** (E-10): an inlined call a split moves off a frame's path (a history's phases,
  a piece's builders) is timed by a scratch program linked against the build's objects and against the base snapshot's
  (`build/prove/<id>/objects`), the least of its rounds, in alternating pairs. A header that includes less (`lore.h` for
  shards.h) is proven by objects: the tree as it was compiled again at the Makefile's flags, every object compared.
- **Files renamed, data files' comments** (E-11): a table of data compiled in (the grammar's files) is dumped by a scratch program
  linked against the build's objects and the base's, by index; a printout that names a renamed file is compared after the base's
  copy is put through the rename map (old name to new); a comment edited in a data file is proven by its lines (the same count,
  only `#` lines differ) and by what the data makes (`grammar out F`: the texts byte for byte). A constant hoisted out of a function
  is `samecode.py --rename '<function>::NAME=NAME'`.
- **A sweep's steps** (E-12): a step's base is the tree after the step before (a private index's `git write-tree`: HEAD, the index and
  the refs untouched), so `samecode.py` compares that step alone (`SAMECODE_ALL=1` lists every function that differs: each must hold
  an edited site). A comment-only or whitespace-only step on a harness object that varies between compiles is proven by its
  preprocessed source (`clang++ -E -P`) against the base's, the same, or the same with adjacent string literals joined (a literal
  split in two compiles alike). A harness edit the battery cannot see is proven by an A/B items file (`PROVE_ITEMS`) recorded
  with the binary before it. A comment-only or whitespace-only step is a commit of its own, listed in `.git-blame-ignore-revs`
  (GitHub's blame skips it; here `git blame --ignore-revs-file .git-blame-ignore-revs`); a commit made from a step's tree is
  `git commit-tree <tree> -p <parent>`.
- **At the close:** from a clean build (`make clean && make -j8`) the whole battery, `tools/prove compare <base>`, the unit and
  the flow inside it; `bench check` and its pairs; then `tools/prove snapshot <id>`, the next milestone's base.
- **Alone, awake, in the background:** `caffeinate -i`, the output to a file, nothing else building or running. The Mac sleeps
  while the user is away and the harness reads the wall clock (T-01's plan timing, the radar's wait for records): a sleep
  stalls a check into failing. Frames drawn beside other jobs move. Never `make` or `git stash` while a run is in flight;
  `tools/prove` refuses a binary older than its sources.
- **The bench:** binaries compared from paths of one length (the launch path moves rows: P-04; `tools/prove bench` copies them
  to `./vt_pa` and `./vt_pb`); the sector map's 4x row moves by the launch name alone, so swap the two names for one pair
  before calling a row of under about 10% real. Two builds of the harness can differ with no source change: map the differing
  bytes to functions (`nm -n`, `cmp -l`) before suspecting a stale object.
- **Pictures:** a dark frame is measured, not judged by eye (`tools/png.py stats`, crops): the image viewer stretches dark
  pictures, and B-403's "holes" were invisible at the game's own brightness.
- **A restored file:** `cp -p` keeps its old time, so make rebuilds nothing and the binary keeps the change; `touch` it
  (E-06's trial of a broken row).
- **On a difference:** run the item again alone (noise does not repeat); look at it (`tools/png.py diff`, `tools/keydiff.py`);
  find its cause before going on. Never mask a difference away, never take a snapshot over one.

## F-01's tools (2026-10-12)

F-01, the performance plan's first milestone, made these to prove the F series' changes bit for bit; the rest of its modes are
in `../08-testing-and-tools.md`.

* `pngdiff <a.png> <b.png> [diff.png]`: two harness PNGs pixel for pixel: the count that differ (and their share), the largest
  step of one channel, the first differing pixel, the rows they span and how many fall in each eighth of the height; with a third
  name the differing pixels in red over the first picture at a third of its brightness. Exit code 1 when they differ (output:
  stdout, the diff image)
* `pngdiff <dirA> <dirB>`: every PNG of A against B's of the same name, the differing and missing ones listed, then the counts;
  exit code = differing + missing. The milestone's proof: keep the previous `vesperis_test`, run `unit`, `regress` and `flow` (and
  `scene`s at 2x/4x) with each binary, copy `shots/tests` aside after each, compare the two directories (output: stdout)
* `bothways <scene> [scale]`: a `scene` drawn by the workers twice, by one thread (`parallelThreadsOverride`) and by the workers
  again, in one run: B-310's contract on any scene (`shots/tests/bothways_<scene>_<scale>x_{workers,serial,diff}.png`, the flora's
  bands printed); exit code 1 when the frames differ or the workers' frame is not the same twice (output: stdout, PNGs)
* `screens [scale ...]`: F-05: the game's own frames the regress never hashes, each drawn at one instant (`frame(in, 0)`) once it
  has held still for half a second (a world's map or a tile arriving changes it), at 1x, 2x, 3x and 4x (or the scales named), by
  the workers and by one thread: the cabin (the light on and off, the hull glass dark and lit, the roof, the decoder's desk, the
  case, the drones' rack), the console, the ship's screen, the help, the guide, the radar camera, the modern test world's landing
  map and zoom, the drone bay's screen, the rack with the drones leaving, the drones outside the window, and on the home world the
  descent, the ground, the menu, the sector map near and wide and the nose camera (96 pictures); the waits for a thread's work run
  at one instant too, so two runs of one binary write the same pictures (`pngdiff` two binaries' sets: the proof for the cabin,
  the screens and the feeds). Exit code 1 when one thread draws a frame differently (output:
  `shots/tests/screens/<name>_<scale>x.png`, stdout (a single frame's times, a hint))
* `suncone`: F-05: the sun drawn alone (`SpaceRenderer::drawSun` on a cleared frame) with its direction swept toward the frame's
  corner across the angle where its glow's cone leaves the frame (61 steps of 1e-4 and 61 of 1e-6 radians round it, and 101 from
  half the cone inside to half outside), seven classes (the protostar's strip and the black hole's lens among them), three sizes,
  with an atmosphere and without, at 1x and 4x: a hash per case and one of every frame, for two binaries to agree on (the cone
  test, `coneMissesFrame`) (output: stdout)

**The proof of a bit-identical change** (F-01's method, every F milestone; F-10's batch, which was allowed to move the hashes, kept
them all): keep the previous `vesperis_test`; `regress` and `VESPERIS_THREADS=1 regress` (24 unchanged, 22 before F-11);
`scene <name> 2` and `4` for the regress scenes and the frames the change touches, written by each binary and compared with `pngdiff`
(deterministic: identical or it is a change); `unit` (its both-ways check) and `bothways` on the scenes the change touches. The `flow`
is not deterministic between runs of one binary: 44 of its 166 frames varied between two runs from the same files (the threads deliver
drainage tiles, works and scans at different frames; the slots, the log, the gallery and the drone bay's charge read the wall clock),
and the unit's six scanner and marks frames depend on what a previous run left in `shots/tests`. Compare them only from the same
restored state (the non-PNG files: `rsync --exclude '*.png'`), with a baseline-against-baseline run beside the new one as the noise
floor; copy only the PNGs a run writes (`shots/tests` is about 2 GB). A wait that runs frames until a thread is done counts frames by
the binary's speed: F-05 holds the flow's wait for the scanner at one instant (frames at dt 0, as `screens` waits), which had put a
faster binary's clock, and every picture after the scanner, ahead.

## E-02: the fingerprints and `tools/prove` (2026-10-23)

The E series' proof (`docs/archive/ideas/PLAN-reorganisation.md`, section 5), kept in the tree instead of made again in each milestone's
`build/<id>/`. The fingerprint modes are the harness's first own files (`src/harness/`).

* `fingerprint bodies [n] [out]`: Q-03's fp5 as a mode: every body of four scan regions (150..340 x 20..160 at sy 0, and three far
  squares) as a keyed line, its key the sector and the body: the body, its label, its made generator, its data sheet's sentence,
  its traits, its sky, its climate and 24 samples of its ground at four scales, a hash each, and `all`, fp5's hash of them all;
  `gen2` and `ground2`, the generator's and the ground's fields fp5 left out (E-01's baseline found them); the drifters nearest
  home with their moons (`dark` lines: the dark bodies and the dark ocean, which no scan reaches, so all 60 kinds); three bodies
  of each drained kind with the drainage on. `n` takes every n-th sector column. 44,780 lines in about 35 s (output:
  `shots/tests/fingerprint_bodies.txt`)
* `fingerprint stream [n] [out] [full]`: P-01's draw-stream tool as a mode: every system of the scan region and of a strip of far
  sectors of every class (its name, class and bodies), and each body's draws and what a made kind sets, hashed (`full` prints
  them). 6,866 systems and 65,714 bodies in under a second (output: `shots/tests/fingerprint_stream.txt`)
* `fingerprint peoples [n] [out] [full]`: D-05's recdump as a mode: every people of its fourteen fixed worlds and of every n-th
  (6) people's world of the plane round home: the profile, the lore with its chart marks, the culture, the history, the record
  catalogue, the homes, every record and every text, a hash each (`full` writes recdump's text, byte for byte). 63 peoples in
  about 18 s (output: `shots/tests/fingerprint_peoples.txt`)
* `fingerprint modules [n] [out] [full]`: for a fixed sample, what nothing else pins: of 16 peoples' worlds (the age and scene
  worlds, the scorched ones, the two test drifters) each world's plan, each people's ways and roads, its places and the
  countryside round its capital laid out as the view lays a cell (the layout, the town, the pieces at three levels, their
  colliders, meshes and profiles, the rooms, the decay, the graves, records and instruments), the works, a town and a village of
  each age, the music and its pieces, the instruments, the tongue and voice, the genome; on 22 land sites of the drained kinds the
  trees, logs, boulders, colliders, ground and drainage; the almanac of home and of every 40th system of the scan region (on the
  ground and parked), with the showers, dust and fronts; and the tables. 4,607 lines in about 33 s; `full` adds each line's text
  beneath it (output: `shots/tests/fingerprint_modules.txt`)
* `scene list`: every comparison scene's name, in the table's order (109) (output: stdout)

A keyed line is `<what> <key...> <field>=<value> ...`. `tools/keydiff.py <a> <b>` compares two files key by key and field by
field; a field one side lacks is a column added or dropped and is reported apart, so a step that adds a field still proves the
others.

**`tools/prove`** keeps a snapshot of the binary, the objects and every item's outputs in `build/prove/<name>/`:
- `snapshot <name>`, `compare <name> [items]` (at most 30 lines), `objects <name>` (the engine's and the game's objects, for a
  comment-only step; the harness's own object is not reproducible and is skipped), `bench <A> <B> [pairs]` (AB/BA pairs of
  `bench check` from paths of one length, `tools/benchcmp.py`), `items`.
- The battery is `tools/prove.items`, one item a line (`name | command | expect | options`). Each item runs alone in a scratch
  working directory with `src/`, `tests/` and `music/` linked in and an empty `shots/tests`, the binary linked as
  `./vesperis_test`. Every file it writes is compared (`tools/provecmp.py`): fingerprints by key, text after the masks (timings,
  the saves' `saved <unix time>`, and an item's own `mask=<regex>`), PNGs pixel by pixel (`tools/png.py`), the rest byte for
  byte; with its exit code, printouts and counts (`expect`).
- An item marked `twice` runs twice at the snapshot, and what moved between its runs is skipped (its `NOISE`); `noise=<glob>`
  names an item's files known to move, with the reason in the items file. The flow starts from an empty `shots/tests` (it makes
  every save it loads) and two runs draw the same 179 files. The unit's six scanner and map frames (R-408, B-412, T-03) and its
  autosave move between runs: it runs frames until a thread has found the cells or the scan, so the trees and grass sway, and
  the clock ends, where the threads' pace leaves them. Its printout (the 403 checks) and its other files are compared.
- `datasheet`, `surface` (the sweeps), `rogues heard` and the unit's lent piece wait for every body's map before a frame (E-02), as
  the flow and the regress do (F-07): the maps are made on their thread at the binary's own pace, and a frame drawn before one
  would move between runs by a faint disc or a dim background.
- `PROVE_BIN=<binary>` runs a new snapshot or a compare with another binary: E-02 relinked e02-base's saved engine objects with
  its final harness (`e02-base2`) to give the new fingerprint columns a base.
- `PROVE_HOME` (the main checkout's `build/prove` by default, found through git) lets a worktree use those snapshots.

The skills `prove`, `bench` and `look` (`.claude/skills/`) say when and how; the `prover` agent (`.claude/agents/`) runs the
battery and returns its verdict. `tools/png.py` (stats, crop, montage, diff) and `tools/wav.py` (stats, events, pitch) read the
outputs without numpy. `make check` (`tools/check.py`) holds the layout: raylib only in the platform file, the layering, no C++
line in src over 300 characters (E-12), every source file named in its directory's CLAUDE.md.
